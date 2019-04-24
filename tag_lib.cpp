#include <iostream>

#include <time.h>       /* time_t */
#include <sys/types.h> /* pid_t */
#include <functional> /*std::functional*/
#include <cstring> /* strncpy */
#include "tag_gpu.h" /* tag_begin, tag_end */


/*
 * TODO: Interface v2.0
 * tag_begin(metadata, job_t **shm_job)
 * - creates a shared memory object to store (and persist throughout tag routine)
 *   a shared job_t struct with the server process
 *
 * tag_begin(job_t **shm_job)
 * - marks shared job as COMPLETED, unmaps job from local address space
 */


static int gb_fd = 0;
static global_jobs_t *gb_jobs = NULL;

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
		DEBUG_FN(init_global_jobs, &fd, &gb_jobs);
	}

	/* Next, build a job_t in shared memory at shared memory location */
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	job_t *tagged_job;	
	DEBUG_FN(build_shared_job, tid, unit_name, 
			last_peak_mem, avg_peak_mem, worst_peak_mem,
			last_exec_time, avg_exec_time, worst_exec_time,
			run_count, deadline, QUEUED,
			&tagged_job,
			job_shm_name);


	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	/* On wake, unlink this shared memory, removing clients reference to shm object */	
	DEBUG_FN(shm_unlink, job_shm_name);

	/* check execution flag - return 0 on success (can run) or -1 on error
	 * or unallowed to run.
	 */
	if (!tagged_job->client_exec_allowed){
		fprintf(stderr, "(TID: %d) Aborting job with name %s!\n", tid, unit_name);
		return -1;
	} else {
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
	DEBUG_FN(build_shared_job, tid, unit_name, 
			last_peak_mem, avg_peak_mem, worst_peak_mem,
			last_exec_time, avg_exec_time, worst_exec_time,
			run_count, deadline, COMPLETED,
			&tagged_job,
			job_shm_name);


	/* Enqueue name of shared memory object holding job_t using middleaware interface */
	submit_job(gb_jobs, tagged_job, job_shm_name);

	/* Cond_wait on job_t (SHARED PROCESS) condition variable, will wake when server notifies */
	pthread_cond_wait(&(tagged_job->client_wake), &(tagged_job->own_job));
	// Awake, it's fine to unlock own_job
	pthread_mutex_unlock(&(tagged_job->own_job));

	/* On wake, unlink this shared memory, removing clients reference to shm object */	
	DEBUG_FN(shm_unlink, job_shm_name);

	return 0;
}

/*
 * From stack overflow:
 * https://stackoverflow.com/questions/30679445/python-like-c-decorators 
 */
template <class> struct TagDecorator;

template <class R, class... Args>
struct TagDecorator<R(Args ...)>
{
	std::function<R(Args ...)> f_;
	pid_t tid;
	const char *work_name;
	time_t deadline;

    TagDecorator(std::function<R(Args ...)> f, 
			     pid_t tid, const char *work_name,
			     time_t deadline=0L) : 
			 f_(f), tid(tid), work_name(work_name){}

    R operator()(Args ... args)
	{
		std::cout << "Calling the decorated function.\n";

		if (tag_begin(...) == 0) {
			// This runs when server allows
			R res = f_(args...);

			// Notify server of end of work
			tag_end(...);
			return res;
		} else {
			// Job is aborted, raise error for client 
			fprintf(stderr, "Job (%s) was aborted! Raising SIGSEGV!\n", work_name);
			raise(SIGSEGV);
		}
    }
};

template<class R, class... Args>
TagDecorator<R(Args...)> tag_decorator(R (*f)(Args ...),
									   pid_t tid,
									   const char *work_name)
{
	return TagDecorator<R(Args...)>(std::function<R(Args...)>(f), tid, work_name);
}
