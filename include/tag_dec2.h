#ifndef TAG_DEC_H
#define TAG_DEC_H

#ifdef __cplusplus

#include <csignal> /* raise, SIGSEGV */
#include <cstring> /* memcpy */
#include <sys/types.h> /* pid_t */
#include <memory> /* std::shared_ptr, std::make_shared */
#include <mutex> /* std::unique_lock, std::mutex */
#include <unordered_map> // unordered_map
#include <iostream> // std::cout
#include <functional> // std::function
#include <stdexcept> // std::runtime_error

#include "tag_state2.h" // TagState struct, gettid()
#include "tag_gpu2.h" /* meta_job_t */
#include "tag_frame.h" // FrameJob, FrameController


template <class> struct TagDecorator;

template <class R, class... Args>
struct FrameJobDecorator<R(Args ...)>
{
	std::function<R(Args ...)> f_;
	std::shared_ptr<FrameController> fc;
	int fj_id;

	/* 
	 * Constructor:
	 * Saves function being decorated and registers new FrameJob with
	 * FrameController object
	 */
	FrameJobDecorator(std::function<R(Args ...)> f, FrameController *_fc, const char *frame_job_name, bool shareable) :
		f_(f), fc(_fc) {
		// Get frame_job job_id by registering new job into FrameController
		fj_id = fc->register_frame_job(frame_job_name, shareable);
		if (fj_id < 0) {
			string err_msg = "Could not register frame job: ";
			throw std::runtime_error(err_msg + frame_job_name + "!");
		}
	}

	R operator()(Args ... args)
	{
		std::cout << "Calling the decorated function.\n";
		/* Tag work begin */
		R res;
		pid_t tid = gettid();
		int prep_val = fc->prepare_job_by_id(fj_id, tid);
		if (prep_val < 0) {
			// Cannot run job either because of error, dropping frame, or not permitted
			if (prep_val == -1) {
				std::cout << "Error preparing job!\n" << std::endl;
				throw std::runtime_error("Error preparing job!\n");
			} else if (prep_val == -2) {
				std::cout << "Aborting job, not permitted by server to run!\n" << std::endl;
				throw AbortJobException("Job not permitted by server to run!\n");
			} else {
				std::cout << "Skipping remaining jobs for this frame!\n" << std::endl;
				throw DroppedFrameException("Skipping remaining jobs for this frame!");
			}
		}

		// Do work 
		res = f_(args...);

		/* Release job resources before returning */
		fc->release_job_by_id(fj_id, tid);
		return res;
	}
};

template<class R, class... Args>
std::shared_ptr<FrameJobDecorator<R(Args...)> > tag_decorator(R (*f)(Args ...), 
		                                     					const char *fj_name,
																bool shareable)
{
	  return std::make_shared<FrameJobDecorator<R(Args...)> >
		  (std::function<R(Args...)>(f), fj_name, shareable);
}
#endif /* __cplusplus */

#endif /* TAG_DEC_H */
