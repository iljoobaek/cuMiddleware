#include <time.h>       /* time_t */
#include <sys/mman.h>  /* shm_unlink, munmap */
#include <sys/types.h> /* pid_t */
#include <err.h>		// err
#include <string.h>	// strncpy
#include <stdlib.h> // malloc, free
#include "tag_gpu.h" /* tag_begin, tag_end */
#include "mid_structs.h" /* global_jobs_t, job_t, JOB_MEM_NAME_MAX_LEN */
#include "mid_common.h"	/* build/destroy_shared_job */
#include "mid_queue.h" /* submit_job */
#include "tag_gpu.h" // fn's defined in this file

/*
 * TODO: Interface v2.0
 * tag_begin(metadata, job_t **shm_job)
 * - creates a shared memory object to store (and persist throughout tag routine)
 *   a shared job_t struct with the server process
 *
 * tag_begin(job_t **shm_job)
 * - marks shared job as COMPLETED, unmaps job from local address space
 */

#define TAG_DEBUG_FN(fn, ...) \
{\
	int res;\
	if ((res = fn(__VA_ARGS__)) < 0) { \
		fprintf(stderr, "TAG_DEBUG_FN: %s returned %d!\n", #fn, res);\
		assert(false);\
	}\
}

static int gb_fd = 0;
static global_jobs_t *gb_jobs = NULL;

/* Implement interface for meta_job struct */
meta_job_t *CreateMetaJob(pid_t tid, int priority, const char *job_name, 
		uint64_t lpm, uint64_t apm, uint64_t wpm,
		double let, double aet, double wet,
		unsigned int run_count, time_t deadline) {
	meta_job_t *mj = (meta_job_t *)calloc(0, sizeof (meta_job_t));
	mj->tid = tid;
	strncpy(mj->job_name, job_name, JOB_MEM_NAME_MAX_LEN);
	mj->last_peak_mem = lpm;
	mj->avg_peak_mem = apm;
	mj->worst_peak_mem = wpm;
	mj->last_exec_time = let;
	mj->avg_exec_time = aet;
	mj->worst_exec_time = wet;
	mj->run_count = run_count;
	mj->deadline = deadline;
	
	return mj;
}

void DestroyMetaJob(meta_job_t *mj) {
	free(mj);
}


int tag_begin(pid_t tid, const char* unit_name,
			uint64_t last_peak_mem, 
			uint64_t avg_peak_mem, 
			uint64_t worst_peak_mem, 
			double last_exec_time,
			double avg_exec_time, 
			double worst_exec_time, 
			unsigned int run_count, 
			time_t deadline
		) {
	/* First, init global jobs if not already */
	if (gb_jobs == NULL) {
		TAG_DEBUG_FN(init_global_jobs, &gb_fd, &gb_jobs);
	}

	/* Next, build a job_t in shared memory at shared memory location */
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	job_t *tagged_job;	
	TAG_DEBUG_FN(build_shared_job, tid, unit_name, 
			last_peak_mem, avg_peak_mem, worst_peak_mem,
			last_exec_time, avg_exec_time, worst_exec_time,
			run_count, deadline, QUEUED,
			&tagged_job,
			job_shm_name);


	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	fprintf(stdout, "Client submitting job to server\n");
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	fprintf(stdout, "Client cond waiting\n");
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	fprintf(stdout, "Client thread woke up! Exec flag %d\n",\
			tagged_job->client_exec_allowed);
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	// Save exec flag
	bool exec_allowed = tagged_job->client_exec_allowed;

	/* On wake, unlink and unmap this shared memory, 
	 * removing clients reference to shm object */	
	//shm_unlink(job_shm_name); // TODO not doing anything
	fprintf(stdout, "Destroying shared job\n");
	TAG_DEBUG_FN(destroy_shared_job, &tagged_job);
	fprintf(stdout, "Destroyed shared job\n");

	/* check execution flag - return 0 on success (can run) or -1 on error
	 * or unallowed to run.
	 */
	if (!exec_allowed){
		fprintf(stderr, "(TID: %d) Aborting job with name %s!\n", tid, unit_name);
		return -1;
	} else {
		fprintf(stdout, "Client continuing ...\n");
		return 0;
	}
}

/* TODO:
 * Reuse shared memory for tag_begin()
 * Why not just set COMPLETED flag on shared job, and unmap the job?
 * Then the server just needs to look for completed jobs every server cycle
 * but you can avoid client cond_wait latency and memory mapping
 */
int tag_end(pid_t tid, const char *unit_name) {
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	job_t *tagged_job;	
	fprintf(stdout, "Client finished work, communicating to server\n");
	TAG_DEBUG_FN(build_shared_job, tid, unit_name, 
			0L, 0L, 0L,	
			0.0, 0.0, 0.0,
			0, 0, COMPLETED,
			&tagged_job,
			job_shm_name);

	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	fprintf(stdout, "Client submitting COMPLETED job to server\n");
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	fprintf(stdout, "Client cond waiting\n");
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	fprintf(stdout, "Client thread woke up! Exec flag %d\n",\
			tagged_job->client_exec_allowed);
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	/* On wake, unlink this shared memory, removing clients reference to shm object */	
	shm_unlink(job_shm_name); // TODO: not doing anything

	/* Unmap the shared job built for server on clientside */
	TAG_DEBUG_FN(destroy_shared_job, &tagged_job);
	return 0;
}
