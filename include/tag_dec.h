#include <csignal> /* raise, SIGSEGV */
#include <cstring> /* memcpy */
#include <sys/types.h> /* pid_t */
#include <memory> /* std::shared_ptr, std::make_shared */
#include <mutex> /* std::unique_lock, std::mutex */
#include <unordered_map> // unordered_map
#include <unistd.h>  // syscall()
#include <sys/syscall.h> // SYS_gettid
#include <iostream> // std::cout

#include "tag_gpu.h" /* meta_job_t */

#define gettid() syscall(SYS_gettid)

/* Interface for getting meta_job_t from g_tid_curr_job */
#ifdef __cplusplus
extern "C" {
#endif 
meta_job_t *get_running_meta_job_for_tid(pid_t tid);

int set_running_job_for_tid(pid_t tid, meta_job_t *mj);

#ifdef __cplusplus
}
#endif

struct TagState {
	std::shared_ptr<meta_job_t> init_meta_job;
	std::mutex hm_lock;
	std::unordered_map<pid_t, std::shared_ptr<meta_job_t> >tid_to_meta_job;

	TagState();
	TagState(const meta_job_t *inp_init_meta_job);
	~TagState();

	/* Retrieve locally hashed current job metadata template:
	 * If tid has no entry in hashmap, 
	 * 	one is created copied from init_meta_job 
	 * Else, return pointer to stored in hashmap for tid
	 */
	meta_job_t *get_local_meta_job_for_tid(pid_t tid);

	// Use meta job to construct call to tag_end
	int tag_end_from_meta_job(meta_job_t *mj);
		
	// Use meta job to construct call tag_begin 
	int tag_begin_from_meta_job(meta_job_t *mj);

	int acquire_gpu();
	int release_gpu();
	void start_timer();
	void end_timer();
};

template <class> struct TagDecorator;

template <class R, class... Args>
struct TagDecorator<R(Args ...)>
{
	std::function<R(Args ...)> f_;
	TagState t_state;

	//TagDecorator(std::function<R(Args ...)> f) :
	//	f_(f) {}

	TagDecorator(std::function<R(Args ...)> f,
			const meta_job_t *inp_init_meta_job) :
		f_(f), t_state(inp_init_meta_job) {}

	R operator()(Args ... args)
	{
		std::cout << "Calling the decorated function.\n";
		/* Tag work begin */
		R res;
		if (t_state.acquire_gpu() < 0) {
			std::cout << "Aborting job, not permitted by server to run!\n" << std::endl;
			raise(SIGSEGV); // TODO: Can this be more graceful?
		}

		// Time and do work
		t_state.start_timer();
		res = f_(args...);
		t_state.end_timer();

		/* Tag work end */
		t_state.release_gpu();
		return res;
	}
};

template<class R, class... Args>
std::shared_ptr<TagDecorator<R(Args...)> > tag_decorator(R (*f)(Args ...), 
		                                     const meta_job_t *inp_init_meta_job)
{
	  return std::make_shared<TagDecorator<R(Args...)> >
		  (std::function<R(Args...)>(f), inp_init_meta_job);
}

