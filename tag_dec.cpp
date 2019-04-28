#include <time.h>       /* time_t */
#include <sys/types.h> /* pid_t */
#include <cstring> /* strncpy */
#include <functional> /*std::functional*/
#include "tag_gpu.h" /* tag_begin, tag_end */

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
	uint64_t last_peak_mem;
	uint64_t avg_peak_mem;
	uint64_t worst_peak_mem;
	double last_exec_time;
	double avg_exec_time;
	double worst_exec_time; 
	unsigned int run_count;
	time_t deadline;
	job_t *shm_job;

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

