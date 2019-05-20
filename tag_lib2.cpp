#include <time.h>       /* time_t */
#include <sys/mman.h>  /* shm_unlink, munmap */
#include <sys/types.h> /* pid_t */
#include <err.h>		// err
#include <string.h>	// strncpy
#include <stdlib.h> // malloc, free
#include <stdint.h>	// int64_t
#include "mid_structs2.h" /* global_jobs_t, job_t, JOB_MEM_NAME_MAX_LEN */
#include "mid_common2.h"	/* build/destroy_shared_job */
#include "mid_queue.h" /* submit_job */
#include "tag_gpu2.h" // fn's impl.'d in this file

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

#ifdef __cplusplus
extern "C" {
#endif

/* Implement interface for meta_job struct */
void *CreateMetaJob(pid_t tid, const char *job_name,
		uint64_t lpm, uint64_t apm, uint64_t wpm,
		int64_t let, int64_t aet, int64_t wet,
		unsigned int run_count) {
	meta_job_t *mj = (meta_job_t *)calloc(1, sizeof (meta_job_t));
	mj->tid = tid;
	strncpy(&(mj->job_name[0]), job_name, JOB_MEM_NAME_MAX_LEN);
	mj->last_peak_mem = lpm;
	mj->avg_peak_mem = apm;
	mj->worst_peak_mem = wpm;
	mj->last_exec_time = let;
	mj->avg_exec_time = aet;
	mj->worst_exec_time = wet;
	mj->run_count = run_count;

	return (void *)mj;
}

void DestroyMetaJob(void *mj) {
	free(mj);
}

int tag_job_begin(pid_t pid, pid_t tid, const char* job_name,
		int64_t slacktime, bool first_flag, bool shareable_flag,
		uint64_t required_mem) {
	/* First, init global jobs if not already */
	if (gb_jobs == NULL) {
		TAG_DEBUG_FN(init_global_jobs, &gb_fd, &gb_jobs);
	}

	/* Next, build a job_t in shared memory at shared memory location */
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	job_t *tagged_job;
	TAG_DEBUG_FN(build_shared_job, pid, tid, job_name,
			slacktime, first_flag, shareable_flag, required_mem,
			QUEUED,
			&tagged_job,
			job_shm_name);

	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	//fprintf(stdout, "Client submitting job to server\n");
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	//fprintf(stdout, "Client cond waiting\n");
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	//fprintf(stdout, "Client thread woke up! Exec flag %d\n",
	//		tagged_job->client_exec_allowed);
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	// Save exec flag
	bool exec_allowed = tagged_job->client_exec_allowed;

	/* On wake, unlink and unmap this shared memory,
	 * removing clients reference to shm object */
	shm_unlink(job_shm_name); // TODO not doing anything
	//fprintf(stdout, "Destroying shared job\n");
	TAG_DEBUG_FN(destroy_shared_job, &tagged_job);
	//fprintf(stdout, "Destroyed shared job\n");

	/* check execution flag - return 0 on success (can run) or -1 on error
	 * or unallowed to run.
	 */
	if (!exec_allowed){
		fprintf(stderr, "(TID: %d) Aborting job with name %s!\n", tid, job_name);
		return -1;
	} else {
		//fprintf(stdout, "Client continuing ...\n");
		return 0;
	}
}

/* TODO:
 * Reuse shared memory for tag_begin()
 * Why not just set COMPLETED flag on shared job, and unmap the job?
 * Then the server just needs to look for completed jobs every server cycle
 * but you can avoid client cond_wait latency and memory mapping
 */
int tag_job_end(pid_t pid, pid_t tid, const char *job_name) {
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	job_t *tagged_job;
	//fprintf(stdout, "Client finished work, communicating to server\n");
	TAG_DEBUG_FN(build_shared_job, pid, tid, job_name,
			0.0, false, false, 0L,	// dummy-data
			COMPLETED,
			&tagged_job,
			job_shm_name);

	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	//fprintf(stdout, "Client submitting COMPLETED job to server\n");
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	//fprintf(stdout, "Client cond waiting\n");
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	//fprintf(stdout, "Client thread woke up! Exec flag %d\n",
	//		tagged_job->client_exec_allowed);
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	/* On wake, unlink this shared memory, removing clients reference to shm object */
	shm_unlink(job_shm_name); // TODO: not doing anything

	/* Unmap the shared job built for server on clientside */
	TAG_DEBUG_FN(destroy_shared_job, &tagged_job);
	return 0;
}
#ifdef __cplusplus
}
#endif

