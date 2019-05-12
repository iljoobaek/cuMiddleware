#ifndef TAG_DEC_H
#define TAG_DEC_H

#include <csignal> /* raise, SIGSEGV */
#include <cstring> /* memcpy */
#include <sys/types.h> /* pid_t */
#include <memory> /* std::shared_ptr, std::make_shared */
#include <mutex> /* std::unique_lock, std::mutex */
#include <unordered_map> // unordered_map
#include <iostream> // std::cout
#include <functional> // std::function

#include "tag_state.h" // TagState struct
#include "tag_gpu.h" /* meta_job_t */

template <class> struct TagDecorator;

template <class R, class... Args>
struct TagDecorator<R(Args ...)>
{
	std::function<R(Args ...)> f_;
	TagState t_state;

	TagDecorator(std::function<R(Args ...)> f) :
		f_(f) {}

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

#endif /* TAG_DEC_H */
