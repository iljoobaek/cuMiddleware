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
#include <semaphore.h>			// sem_t, sem_*()

#include <algorithm>	// std::find
#include <cassert>		// assert()
#include <queue>		// std::queue, std::priority_queue, std::vector
#include <vector>			// std::vector
#include <memory>		// std::shared_ptr
#include <unordered_map>	// std::unordered_map

#include "mid_structs.h"
#include "mid_queue.h"
#include "mid_common.h"

// Helper define for slacktime threshold check
#define SLACKTIME_THRESHOLD (5*SLEEP_MICROSECONDS)
#define CURR_SLACKTIME_PQ_EMPTY_OFFSET (rel_priority_period_count*SLEEP_MICROSECONDS)
#define WITHIN_SLACKTIME_THRESHOLD(s) \
	(s < (int64_t)CURR_SLACKTIME_PQ_EMPTY_OFFSET + SLACKTIME_THRESHOLD)

/* Define comparison functor to pass to priorityqueue template parameter */
typedef bool(*CompareJobFunc)(job_t *&j1, job_t *&j2);

/* One job is higher priority than another if its slacktime is <= to others */
bool CompareJobSlack(job_t *&j1, job_t *&j2) {
	return j2->slacktime_us <= j1->slacktime_us;
}
/* One job is higher priority than another if its period is <= to others */
bool CompareJobPeriod(job_t *&j1, job_t *&j2) {
	return j2->frame_period_us <= j1->frame_period_us;
}
/* One job is higher priority than another if its deadline is <= to others */
bool CompareJobDeadline(job_t *&j1, job_t *&j2) {
	return j2->deadline_us <= j1->deadline_us;
}
/* One job is higher priority than another if its jobid is <= to others */
bool CompareJobFIFO(job_t *&j1, job_t *&j2) {
	return j2->jobid <= j1->jobid;
}

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
static bool use_slack;					// static flag set by cmdline args
static bool use_deadline;				// static flag set by cmdline args
static bool use_period;					// static flag set by cmdline args
static bool use_fifo;					// static flag set by cmdline args

// Initialize job priority queue - FIFO policy is default
static std::priority_queue<job_t *,\
		std::vector<job_t *>, CompareJobFunc> pq_jobs;
static std::queue<job_t*> fifo_jobs;
static std::vector<job_t*> executing_jobs;
static std::queue<job_t*> completed_jobs;
static std::unordered_map<pid_t, int> running_pid_jobs;	// How many jobs per pid concurrently running on GPU
static int gpu_excl_jobs = 0;	// How many jobs are running on GPU with non-shareable flag
// In order to maintain a relative ordering of old and new jobs on the pq
// (which is ordered by slacktimes, which is relative to an absolute client
// frame deadline), we must keep a counter for periods in which the pq is not
// empty.
static int rel_priority_period_count = 0;


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
	if (gpu_memory_available + comp_job->required_mem_b < gpu_memory_available) {
		// Something's wrong, overflow occurred when releasing memory
		fprintf(stderr, "Overflow occurred when releasing gpu for job with name %s (wpm %lu, avail %lu)\n!",\
				comp_job->job_name, comp_job->required_mem_b, gpu_memory_available);
		return -2;
	}

	// Acquired memory
	int acquired_mem = comp_job->required_mem_b ? comp_job->required_mem_b : max_gpu_memory_available;

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
	// Remove pid from running_pid_jobs if its tid count is 0
	if (running_pid_jobs[comp_job->pid] == 0) {
		running_pid_jobs.erase(it);
	}
	return 0;
}

// Helper function for bookkeeping of allocating gpu resources for job
void alloc_gpu_for_job(job_t *j) {
	if (j->required_mem_b == 0) {
		// Allocate all of gpu
		gpu_memory_available = 0;
	} else {
		gpu_memory_available -= j->required_mem_b;
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
 * AND
 * 3) (if use_slace) job's slacktime below a threshold relative to server period
 * Returns 0 on success, -1 on wait signal, -2 on abort signal for job
 */
int job_acquire_gpu(job_t *j) {
	if (!j) return -2;

	bool can_alloc_mem = false;
	if (j->required_mem_b > max_gpu_memory_available) {
		// Must abort job, can never run on the GPU
		return -2;
	} else {
		if (j->required_mem_b == 0 && gpu_memory_available == max_gpu_memory_available) {
			can_alloc_mem = true;
		} else if (j->required_mem_b < gpu_memory_available) {
			can_alloc_mem = true;
		} else {
			can_alloc_mem = false;
		}
	}
	if (!can_alloc_mem) {
		// Must wait for jobs to free up GPU mem
		return -1;
	}

	bool should_run_now = false;
	if (use_slack) {
		if (j->noslack_flag) {
			should_run_now = true;
		} else {
			// Whether job should run now depends on slacktime threshold
			should_run_now = WITHIN_SLACKTIME_THRESHOLD(j->slacktime_us);
		}
		if (!should_run_now) {
			// Must wait for slacktime to be within running threshold
			return -1;
		}
	} else { should_run_now = true; }


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
				can_run_now = false;
			}
		} else {
			// Job is first of its process to run next to other jobs
			if (j->shareable_flag) {
				if (gpu_excl_jobs == 0) {
					// Can run if no other jobs are not-shareable
					can_run_now = true;
				} else {
					can_run_now = false;
				}
			} else {
				// Job can't share gpu with other processes
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
	return sem_post(&(aj->client_wake));
}

/* Wake client with ability to run job */
int trigger_job(job_t *tj) {
	if (!tj) return -1;

	// Set client's execution flag to run
	tj->client_exec_allowed = true;
	return sem_post(&(tj->client_wake));
}

int main(int argc, char **argv)
{
	/* Initialize the scheduling policy from commandline arguments */
	const char *usage = "Usage: ./mid --policy [rms, edf, lsf, fifo]";
	CompareJobFunc cmp_fn;
	use_slack = false;
	use_deadline = false;
	use_fifo = false;
	use_period = false;
	if (argc == 3) {
		if (strcmp(argv[1], "--policy") == 0) {
			if ( (strcmp(argv[2], "rms") == 0) ) {
				fprintf(stdout, "Using RMS scheduler...\n");
				use_period = true;
				cmp_fn = &CompareJobPeriod;
			} else if (strcmp(argv[2], "edf") == 0) {
				fprintf(stdout, "Using EDF scheduler...\n");
				use_deadline = true;
				cmp_fn = &CompareJobDeadline;
			} else if (strcmp(argv[2], "lsf") == 0) {
				fprintf(stdout, "Using LSF scheduler...\n");
				use_slack = true;
				cmp_fn = &CompareJobSlack;
			} else if (strcmp(argv[2], "fifo") == 0) {
				fprintf(stdout, "Using FIFO scheduler...\n");
				use_fifo = true;
				cmp_fn = &CompareJobFIFO;
			} else {
				fprintf(stderr, "%s\n", usage);
				return -1;
			}
		} else {
			fprintf(stderr, "%s\n", usage);
			return -1;
		}
	} else {
		fprintf(stderr, "%s\n", usage);
		return -1;
	}
	pq_jobs = std::priority_queue<job_t *,std::vector<job_t *>, CompareJobFunc> (cmp_fn);

	(void)argc; (void) argv;
	fprintf(stdout, "Starting up middleware main...\n");

	// Initialize middleware state
	max_gpu_memory_available = 1<<30; // 1 GB
	gpu_memory_available = max_gpu_memory_available;
	uint64_t jobc = 0;
	fprintf(stdout, "GPU Memory has %lu bytes available at init.\n", gpu_memory_available);

	int res;
	if ((res=init_global_jobs(&GJ_fd, &GJ, true)) < 0)
	{
		fprintf(stderr, "Failed to init global jobs queue");
		return EXIT_FAILURE;
	}

	// Set up signal handler
	signal(SIGINT, handle_sigint);

	// Init flags controlling when jobs_queued_* can't be emptied becase
	// GPU is at capacity
	bool queued_wait_for_complete = false;

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
			if (q_job->req_type == QUEUED) {
				// Assign monotonic jobid to queued job - careful of overflow
				// TODO: use external counter file
				q_job->jobid = jobc++;
				if ((use_slack || use_deadline) && q_job->noslack_flag) {
					// Use fifo queue if need slack or deadline but first time
					// running job
					fifo_jobs.push(q_job);
				} else {
					if (use_slack && rel_priority_period_count) {
						// Locally modify the relative priority of this job
						// compared to older jobs on the pq
						q_job->slacktime_us += rel_priority_period_count*SLEEP_MICROSECONDS;
					}
					pq_jobs.push(q_job);
					// Just updated pq, can try to process next job immediately
				}
			}
			else {
				completed_jobs.push(q_job);
			}

			// Continue onto next job_shm_name to process
			i++;
		}
		// Reset total count
		GJ->total_count = 0;
		pthread_mutex_unlock(&(GJ->requests_q_lock));

		/* Handle all completed jobs first to release GPU resources */
		while (completed_jobs.size()) {
			/* Dequeue job from completed */
			job_t *compl_job = completed_jobs.front();
			completed_jobs.pop();

			/* Handle completed jobs */
			// First, get original job from executing queue
			auto it = std::find_if(executing_jobs.begin(),
					executing_jobs.end(),
					CompareJobPtr(compl_job));
			assert(it != executing_jobs.end());
			job_t *orig_job = executing_jobs[it - executing_jobs.begin()];

			// Next, release job and remove from executing queue
			if (job_release_gpu(orig_job) == 0) {
				// Remove job from executing_jobs on successful release
				executing_jobs.erase(it);

				/* TODO: Avoid having client sleep waiting for server */
				// NOTE: It must be the compl_job the client is holding on to
				sem_post(&(compl_job->client_wake));

				// Reset flags since a job just released gpu resources
				queued_wait_for_complete = false;

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
		while (!queued_wait_for_complete &&
				fifo_jobs.size()) {
			/* Peek at job from jobs_queued */
			job_t *q_job = fifo_jobs.front();

			/* Handle queued jobs */
			int res = job_acquire_gpu(q_job);
			if (res < 0) {
				if (res == -1) {
					// Failed to acquire GPU, must wait for other jobs to complete
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
				// Adds q_job to executing_jobs on success
				executing_jobs.push_back(q_job);

				// Wake client to trigger execution
				fprintf(stdout, "\tJob (%s, pid=%d, tid=%d) can execute!\n", q_job->job_name, q_job->pid, q_job->tid);
				if (trigger_job(q_job) < 0) {
					fprintf(stderr, "\tFailed to wake client!\n");
				}
			}
			// Actually pop job off queue
			fifo_jobs.pop();
		}

		if (use_slack && pq_jobs.size()) {
			// LSF-Priority queue is not empty, increment the rel_priority_period_count
			rel_priority_period_count++;
		}
		/*
		 * Lastly, run as many jobs (according to priority) as can fit on GPU.
		 */
		while (pq_jobs.size()) {
			/* Peek at top job from pq_jobs */
			job_t *q_job = pq_jobs.top();

			/* Handle queued jobs */
			int res = job_acquire_gpu(q_job);
			if (res < 0) {
				if (res == -1) {
					// Failed to acquire GPU, must wait for other jobs to complete
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
				// Adds q_job to executing_jobs on success
				executing_jobs.push_back(q_job);

				// Wake client to trigger execution
				fprintf(stdout, "\tJob (%s, pid=%d, tid=%d) can execute!\n", q_job->job_name, q_job->pid, q_job->tid);
				if (trigger_job(q_job) < 0) {
					fprintf(stderr, "\tFailed to wake client!\n");
				}
			}
			// Pop job off priority-queue 
			pq_jobs.pop();

			if (pq_jobs.size() == 0) {
				// Reset the relative priority period counter
				rel_priority_period_count = 0;
			}
		}
		// Sleep before starting loop again
		usleep(SLEEP_MICROSECONDS);
	}

	fprintf(stdout, "Cleaning up server...\n");
	destroy_global_jobs(GJ_fd);
	return 0;
}
