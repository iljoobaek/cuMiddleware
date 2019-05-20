/*
 * Simplified mid.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>           // for mode constants
#include <unistd.h>
#include <fcntl.h>              // for O_ constants, such as "O_RDWR"
#include <signal.h>             // sigint can trigger the clean up process;
#include <dlfcn.h>                             // dlsym, RTLD_DEFAULT

#include <algorithm>	// std::find
#include <cassert>		// assert()
#include <queue>		// std::queue, std::priority_queue, std::vector
#include <vector>			// std::vector
#include <memory>		// std::shared_ptr
#include <unordered_map>	// std::unordered_map

#include "mid_structs.h"
#include "mid_queue.h"
#include "mid_common.h"

// Only keep last 100 completed jobs
#define MAX_COMPLETED_QUEUE_SIZE 100

struct CompareJobSlack {
public:
	/* One job is higher priority than another if its slacktime is <= to others */
    bool operator()(job_t *&j1, job_t *& j2) {
		return j2->slacktime <= j1->slacktime;
    }
};
struct CompareJobPtr {
	job_t *lhs;
	public: 
		CompareJobPtr(job_t *_lhs) : lhs(_lhs) {}
		bool operator()(job_t *&rhs) {
			return lhs->pid == rhs->pid \
				&& lhs->tid == rhs->tid \
				&& strcmp(lhs->job_name, rhs->job_name) == 0 ;
		}
};

static uint64_t max_gpu_memory_available; // In B
static uint64_t gpu_memory_available;	// In Bytes

static std::priority_queue<job_t *,\
		std::vector<job_t *>, CompareJobSlack> jobs_queued_pr;
static std::queue<job_t*> jobs_queued_first;
static std::vector<job_t*> jobs_executing;
static std::queue<job_t*> jobs_completed;
static std::unordered_map<pid_t, int> running_pid_jobs;	// How many jobs per pid concurrently running on GPU
static int gpu_excl_jobs = 0;	// How many jobs are running on GPU with non-shareable flag


static int GJ_fd;
static global_jobs_t *GJ;
// -------------------------- Server Side Functions ----------------------------


void destroy_global_jobs(int GJ_fd)
{
	if (GJ_fd >= 0)
	{
		close(GJ_fd);
		shm_unlink(GLOBAL_MEM_NAME);
	}
}

static bool continue_flag = true;
void handle_sigint(int ign)
{
	// Just flip the continue flag
	fprintf(stderr, "Caught SIGINT, not continuing ...\n");
	continue_flag = false;
}

int job_release_gpu(job_t *comp_job) {
	if (!comp_job) {
		fprintf(stderr, "Couldn't release job because of bad pointer!\n");
		return -1;
	}

	// Verify release of memory
	if (gpu_memory_available + comp_job->required_mem < gpu_memory_available) {
		// Something's wrong, overflow occurred when releasing memory
		fprintf(stderr, "Overflow occurred when releasing gpu for job with name %s (wpm %lu, avail %lu)\n!",\
				comp_job->job_name, comp_job->required_mem, gpu_memory_available);
		return -2;
	}

	// Acquired memory
	int acquired_mem = comp_job->required_mem ? comp_job->required_mem : max_gpu_memory_available;

	// Verify decrement number of jobs running under job's pid
	auto it = running_pid_jobs.find(comp_job->pid);
	if (it == running_pid_jobs.end()) {
		fprintf(stderr, "Couldn't release job because cannot find is running pid (%d)!\n", comp_job->pid);
		return -2;
	} else {
		if (running_pid_jobs[comp_job->pid]< 1) {
			fprintf(stderr, "Couldn't release job because too running pid jobs for pid (%d)!\n", comp_job->pid);
			return -2;
		}
	}
	// If job was non-shared, decrement number of gpu_excl_jobs
	fprintf(stderr, "Completed job has attrs: (%d, %d, %s, %ld, %d, %d)\n",
			comp_job->pid, comp_job->tid, comp_job->job_name,
			comp_job->slacktime, comp_job->first_flag, comp_job->shareable_flag);
	if (!comp_job->shareable_flag) {
		if (gpu_excl_jobs < 1) {
			fprintf(stderr, "Couldn't release job because too few gpu_excl_jobs!\n");
			return -2;
		}
		gpu_excl_jobs--;
	}

	// Actually release memory and reduce running_pid_jobs
	gpu_memory_available += acquired_mem;
	running_pid_jobs[comp_job->pid]--;
	return 0;
}

// Helper function for bookkeeping of allocating gpu resources for job
void alloc_gpu_for_job(job_t *j) {
	if (j->required_mem == 0) {
		// Allocate all of gpu
		gpu_memory_available = 0;
	} else {
		gpu_memory_available -= j->required_mem;
	}

	auto it = running_pid_jobs.find(j->pid);
	if (it != running_pid_jobs.end()) {
		// Increment number of threads running for job's pid
		running_pid_jobs[j->pid]++;
	} else {
		running_pid_jobs[j->pid] = 1;
	}

	if (!j->shareable_flag) { // TODO
		gpu_excl_jobs++;
	}
	return;
}

/*
 * A job can acquire the gpu under the following conditions:
 * 1) Job's memory requirement fits on memory available for GPU
 * AND 
 * 2) job can appropriately share the GPU with other threads or pids
 * Returns 0 on success, -1 on wait signal, -2 on abort signal for job
 */
int job_acquire_gpu(job_t *j) {
	if (!j) return -2;

	bool can_alloc_mem = false;
	if (j->required_mem > max_gpu_memory_available) {
		// Must abort job, can never run on the GPU
		return -2;
	} else {
		if (j->required_mem == 0 && gpu_memory_available == max_gpu_memory_available) {
			can_alloc_mem = true;
		} else if (j->required_mem < gpu_memory_available) {
			can_alloc_mem = true;
		} else {
			can_alloc_mem = false;
		}
	}
	if (!can_alloc_mem) {
		// Must wait for jobs to free up GPU mem
		fprintf(stderr, "Job must wait, not enough memory to run yet!\n");
		return -1;
	}

	bool can_run_now = false;
	if (running_pid_jobs.size() == 0) {
		can_run_now = true;
	} else {
		auto it = running_pid_jobs.find(j->pid);
		if (it != running_pid_jobs.end()) {
			// Job shares pid with running job
			if (j->shareable_flag) {
				can_run_now = true;
			} else if (running_pid_jobs.size() == 1 && !j->shareable_flag) {
				can_run_now = true;
			} else {
				// Job is not shareable and multiple pids running, must wait
				fprintf(stderr, "Job is NOT shareable, (1) and there are other pids running\n");
				can_run_now = false;
			}
		} else {
			// Job is first of its process to run next to other jobs
			if (j->shareable_flag) {
				fprintf(stdout, "SHARING GPU!\n");
				if (gpu_excl_jobs == 0) {
					// Can run if no other jobs are not-shareable
					can_run_now = true;
				} else {
					fprintf(stderr, "Job IS shareable, and there are other exclusive jobs present\n");
					can_run_now = false;
				}
			} else {
				// Job can't share gpu with other processes
				fprintf(stderr, "Job is NOT shareable, (2) and there are other pids running on GPU\n");
				can_run_now = false;
			}
		}
	}
	if (!can_run_now) {
		return -1;
	} else {
		alloc_gpu_for_job(j);
		return 0;
	}
}

/* Wake client, instruct to abort job */
int abort_job(job_t *aj) {
	if (!aj) return -1;

	// Set client's execution flag to abort
	aj->client_exec_allowed = false;
	pthread_cond_signal(&(aj->client_wake));
	return 0;
}

/* Wake client with ability to run job */
int trigger_job(job_t *tj) {
	if (!tj) return -1;

	// Set client's execution flag to run
	tj->client_exec_allowed = true;
	pthread_cond_signal(&(tj->client_wake));
	return 0;
}

int main(int argc, char **argv)
{
	(void)argc; (void) argv;
	fprintf(stdout, "Starting up middleware main...\n");

	max_gpu_memory_available = 1<<30; // 1 GB
	gpu_memory_available = max_gpu_memory_available;
	fprintf(stdout, "GPU Memory has %lu bytes available at init.\n", gpu_memory_available);

	int res;
	if ((res=init_global_jobs(&GJ_fd, &GJ)) < 0)
	{
		fprintf(stderr, "Failed to init global jobs queue");
		return EXIT_FAILURE;
	}

	// Set up signal handler
	signal(SIGINT, handle_sigint);

	// Init flags controlling when jobs_queued_* can't be emptied becase
	// GPU is at capacity
	bool queued_wait_for_complete = false;
	bool pq_wait_flag = false;

	// Global jobs queue has been initialized
	// Begin waiting for job_shm_names to be enqueued (sample every 250ms)
	while (continue_flag)
	{
		// Grab job_shm_names lock before emptying
		pthread_mutex_lock(&(GJ->requests_q_lock));
		int i=0;
		while ((GJ->total_count - i)> 0)
		{
			int res;
			job_t *q_job;
			if ((res=peek_job_queued_at_i(GJ, &q_job, i)) < 0)
			{
				fprintf(stderr, "Failed to peek job at idx %d. Continuing...\n", i);
			}

			// Enqueue job request to right queue
			const char *qtype = NULL;
			if (q_job->req_type == QUEUED) {
				if (q_job->first_flag) {
					qtype = "QUEUED_FIRST";
					jobs_queued_first.push(q_job);
				} else {
					qtype = "QUEUED_SLACK";
					jobs_queued_pr.push(q_job);
					// Just updated pq, can try to process next job immediately
					pq_wait_flag = false;
				}
			}
			else {
				qtype = "COMPLETED";

				jobs_completed.push(q_job);
			}
			fprintf(stdout, "Job (%s, tid %d) added to %s queue.\n", q_job->job_name,\
					q_job->tid, qtype);

			// Continue onto next job_shm_name to process
			i++;
		}
		// Reset total count
		GJ->total_count = 0;
		pthread_mutex_unlock(&(GJ->requests_q_lock));

		/* Handle all completed jobs first to release GPU resources */
		while (jobs_completed.size()) {
			/* Dequeue job from completed */
			fprintf(stdout, "Handling completed job!\n");
			job_t *compl_job = jobs_completed.front();
			jobs_completed.pop();

			/* Handle completed jobs */
			// First, get original job from executing queue
			auto it = std::find_if(jobs_executing.begin(),
					jobs_executing.end(),
					CompareJobPtr(compl_job));
			assert(it != jobs_executing.end());
			job_t *orig_job = jobs_executing[it - jobs_executing.begin()];

			// Next, release job and remove from executing queue
			if (job_release_gpu(orig_job) == 0) {
				// Remove job from jobs_executing on successful release
				jobs_executing.erase(it);
				fprintf(stdout, "Removed compl_job (%s, %d) from JOBS_EXECUTING.\n",\
						compl_job->job_name, compl_job->tid);

				/* TODO: Avoid having client sleep waiting for server */
				// NOTE: It must be the compl_job the client is holding on to
				pthread_cond_signal(&(compl_job->client_wake));

				// Reset flags since a job just released gpu resources
				queued_wait_for_complete = false;
				pq_wait_flag = false;

				// Destroy shared jobs
				destroy_shared_job(&orig_job);
				destroy_shared_job(&compl_job);
			} else  {
				fprintf(stderr, "Something went wrong releasing job's resources!\n");
			}

		}

		/*
		 * Next, run any jobs that have never run yet (and therefore have no priority).
		 */
		if (jobs_queued_first.size()) {
			fprintf(stdout, "jobs_queued_first size %d\n", (int)jobs_queued_first.size());
		}
		while (!queued_wait_for_complete &&
				jobs_queued_first.size()) {
			/* Peek at job from jobs_queued */
			job_t *q_job = jobs_queued_first.front();

			/* Handle queued jobs */
			fprintf(stdout, "Handling queued job: %s, %d\n", q_job->job_name, q_job->tid);
			int res = job_acquire_gpu(q_job);
			if (res < 0) {
				if (res == -1) {
					// Failed to acquire GPU, must wait for other jobs to complete
					fprintf(stdout, "\tJob must wait for job(s) to complete!\n");
					queued_wait_for_complete = true;

					break;
				} else {
					// Job is too big to fit on the GPU, instruct client to abort
					// job
					fprintf(stdout, "\tJob must ABORT!\n");
					abort_job(q_job);
					// Destroy shared job
					destroy_shared_job(&q_job);
				}
			} else {
				// Adds q_job to jobs_executing on success
				jobs_executing.push_back(q_job);
				fprintf(stdout, "jobs executing has size: %d\n", (int)jobs_executing.size());

				// Wake client to trigger execution
				fprintf(stdout, "\tJob added to JOBS_EXECUTING, waking client now\n");
				trigger_job(q_job);
			}
			// Actually pop job off queue
			jobs_queued_first.pop();
		}

		/*
		 * Lastly, run as many jobs (according to priority) as can fit on GPU.
		 */
		if (jobs_queued_pr.size()) {
			fprintf(stdout, "jobs_queued_pr size %d\n", (int)jobs_queued_pr.size());
		}
		while (!pq_wait_flag &&
				jobs_queued_pr.size()) {
			/* Peek at top job from jobs_queued_pr */
			job_t *q_job = jobs_queued_pr.top();

			/* Handle queued jobs */
			fprintf(stdout, "Handling queued job: %s, %d\n", q_job->job_name, q_job->tid);
			int res = job_acquire_gpu(q_job);
			if (res < 0) {
				if (res == -1) {
					// Failed to acquire GPU, must wait for other jobs to complete
					fprintf(stdout, "\tJob must wait for job(s) to complete!\n");
					pq_wait_flag = true;
					break;
				} else {
					// Job is too big to fit on the GPU, instruct client to abort
					// job
					fprintf(stdout, "\tJob must ABORT!\n");
					abort_job(q_job);
					// Destroy shared job
					destroy_shared_job(&q_job);
				}
			} else {
				// Adds q_job to jobs_executing on success
				jobs_executing.push_back(q_job);

				// Wake client to trigger execution
				fprintf(stdout, "\tJob added to JOBS_EXECUTING, waking client now\n");
				trigger_job(q_job);
			}
			// Pop job off priority-queue 
			jobs_queued_pr.pop();
		}

		// Sleep before starting loop again
		usleep(SLEEP_MICROSECONDS);
	}

	fprintf(stdout, "Cleaning up server...\n");
	destroy_global_jobs(GJ_fd);
	return 0;
}
