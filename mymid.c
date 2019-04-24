/*
 * Simplified mid.c
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

#include "mid_structs.h"
#include "mid_queue.h"
#include "mid_common.h"

static uint64_t max_gpu_memory_available; // In B
static uint64_t gpu_memory_available;	// In Bytes
static job_t *jobs_executing;
static job_t *jobs_completed;
// -------------------------- Server Side Functions ----------------------------


// TODO: Better name and interface
void destroy_global_jobs()
{
	if (GJ_fd >= 0)
	{
		// Valid fd means preperly init'd
		queue_destroy(jobs_queued);
		queue_destroy(jobs_executing);
		queue_destroy(jobs_completed);
		shm_destroy(GLOBAL_MEM_NAME, GJ_fd);
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
	
	// Assumes that job is coming from jobs_completed
	if (gpu_memory_available + comp_job->worst_peak_mem < gpu_memory_available) {
		// Something's wrong, overflow occurred when releasing memory
		fprintf(stderr, "Overflow occurred when releasing gpu for job with name %s (wpm %d, avail%d)\n!",\
				comp_job->job_name, comp_job->worst_peak_mem, gpu_memory_available);
		return -2;
	}
	
	gpu_memory_available += comp_job->worst_peak_mem;
	return 0;
}

/* 
 * Use j's metadata (worst_peak_mem) to determine whether j can run immediately
 * : if worst_peak_mem is 0, then this job must run exclusively on the GPU to collect metadata
 * : else, compare worst_peak_mem with current gpu_memory_available (and max_gpu_memory_available)
 *  to determine whether job can run immediately (or at all).
 *
 *  Returns 0 on success, -1 when job must wait, -2 when job must abort
 */
int job_acquire_gpu(job_t *j) {
	if (!j) { 
		fprintf(stderr, "Couldn't acquire job because of bad pointer!\n");
		return -2; 
	}

	if (j->worst_peak_mem == 0) {
		// This job has never run before, needs to run exclusively
		if (max_gpu_memory_available > 0 &&\
				gpu_memory_available == max_gpu_memory_available) {
			return 0;
		}
	} else {
		// Check if job can ever run first
		if (j->worst_peak_mem > max_gpu_memory_available) {
			fprintf(stderr, "Job with name %s must abort (wpm %d vs. max %d)!\n",\
					j->job_name, j->worst_peak_mem, max_gpu_memory_available);
			return -2;
		} else if (j->worst_peak_mem > gpu_memory_available) {
			// Job must wait to run
			fprintf(stderr, "Job with name %s must wait (wpm %d vs. avail %d)\n",\
					j->job_name, j->worst_peak_mem, gpu_memory_available);
			return -1;
		} else {
			// Job can run immediately
			fprintf(stderr, "Job with name %s can run (wpm %d vs. avail %d, new avail %d)\n",\
					j->job_name, j->worst_peak_mem, gpu_memory_available,
					gpu_memory_available - j->worst_peak_mem);
			gpu_memory_available -= j->worst_peak_mem;
			return 0;
		}
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
	if (!aj) return -1;

	// Set client's execution flag to run
	tj->client_exec_allowed = true;
	pthread_cond_signal(&(tj->client_wake));
	return 0;
}

int main(int argc, char **argv)
{
	fprintf(stdout, "Starting up middleware main...\n");

	max_gpu_memory_available = 1<<30; // 1 GB
	gpu_memory_available = max_gpu_memory_available;
	fprintf(stdout, "GPU Memory has %d bytes available at init." % gpu_memory_available);

	int res;
	if ((res=init_global_jobs(&GJ_fd, &GJ)) < 0)
	{
		fprintf(stderr, "Failed to init global jobs queue");
		return EXIT_FAILURE;
	}

	// Set up signal handler
	signal(SIGINT, handle_sigint);

	// Init flag controlling when jobs_queued can't be emptied becase
	// GPU is at capacity
	bool queued_wait_for_complete = false;

	// Global jobs queue has been initialized
	// Begin waiting for job_shm_names to be enqueued (sample every 250ms)
	while (continue_flag)
	{
		// Grab job_shm_names lock before emptying
		pthread_mutex_lock(&(GJ->requests_q_lock));
		int i=0;
		while (GJ->total_count > 0)
		{
			int res;
			job_t *q_job;
			if ((res=peek_job_queued_at_i(GJ, &q_job, i)) < 0)
			{
				fprintf(stderr, "Failed to peek job at idx %d. Continuing...\n" % i);
			}
			
			// Enqueue job request to right queue
			job_t *queue=NULL;
			char *qtype = NULL;
			if (q_job->req_type == QUEUED) {
				qtype = "QUEUED";
				queue = jobs_executing;
			}
			else {
				qtype = "COMPLETED";
				queue = jobs_completed;
			}
			if (enqueue_job(q_job, &queue) < 0) {
				fprintf(stderr, "Failed to enqueue job! Job type %d", q_job->req_type);
			}
			fprintf(stdout, "Job (%s, pid %d) added to %s queue.\n", q_job->job_name,\
					q_job->pid, qtype);

			// Continue onto next job_shm_name to process
			i++;
		}
		pthread_mutex_unlock(&(GJ->requests_q_lock));

		/* Handle all completed jobs first to release GPU resources */
		while (jobs_completed->ll_size > 0) {
			/* Dequeue job from completed */
			job_t *compl_job = NULL;
			dequeue_job_from_queue(&jobs_completed, &compl_job);

			/* Handle completed jobs */
			if (job_release_gpu(compl_job) == 0) {
				remove_from_queue(compl_job, &&jobs_executing);
				fprintf(stdout, "Removed compl_job (%s, %d) from JOBS_EXECUTING.\n",\
						compl_job->job_name, compl_job->pid);
				// Reset flag since a job just released gpu resources
				queued_wait_for_complete = false;
			}
		}

		/* As long as jobs_queued aren't waiting for release of GPU 
		 * resources, trigger execution of as many queued jobs as
		 * can fit on GPU.
		 */
		while (!queued_wait_for_complete && jobs_queued->ll_size > 0) {
			/* Dequeue job from jobs_queued */
			job_t *q_job = NULL;
			dequeue_job_from_queue(&jobs_queued, &q_job);

			/* Handle queued jobs */
			fprintf(stdout, "Handling queued job: %s, %d\n", q_job->job_name, q_job->pid);
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
				}
			} else {
				// Adds q_job to jobs_executing on success
				enqueue_job(q_job, &jobs_executing);

				// Wake client to trigger execution
				fprintf(stdout, "\tJob can run, waking client now");
				trigger_job(q_job);
			}
		}

		// Sleep before starting loop again
		usleep(SLEEP_MICROSECONDS);
	}

	// TODO: Better name and interface
	fprintf(stdout, "Cleaning up server...\n");
	destroy_global_jobs();
	return 0;
}
