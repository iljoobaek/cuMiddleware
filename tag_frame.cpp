#include <cstdio> /* fprintf */
#include <cstring> /* memcpy */
#include <deque> /* std::deque */
#include <sys/types.h> /* pid_t */
#include <mutex> /* std::unique_lock, std::mutex */
#include <chrono> /* std::chrono::system_clock::* */
#include <stdexcept> /* std::exception */

#include "tag_state2.h" // TagState
#include "tag_frame.h" // Implements fn's in this header file

/* Define FrameJob class implementation */
FrameJob::FrameJob(const char* _job_name, bool shareable_flag)
	:  job_name(_job_name), shareable(shareable_flag), wc_exec_time(-1.0),
	wc_remaining_exec_time(-1.0) {}

FrameJob::~FrameJob() {}
// Return expected execution time for thread using FrameJob's job_state
double FrameJob::get_expected_exec_time_for_tid(pid_t tid) {
	return job_state.get_wc_exec_time_for_tid(tid);
}
uint64_t FrameJob::get_expected_required_mem_for_tid(pid_t tid) {
	return job_state.get_required_mem_for_tid(tid);
}
double FrameJob::get_wc_remaining_exec_time() {
	return wc_remaining_exec_time;
}
void FrameJob::update_wc_exec_time() {
	wc_exec_time = job_state.get_max_wc_exec_time();
}
void FrameJob::set_wc_remaining_exec_time(double rem_exec_time) {
	wc_remaining_exec_time = rem_exec_time;
}
int FrameJob::prepare_job(pid_t tid, double slacktime, bool first_flag) {
	int res = job_state.acquire_gpu(tid, slacktime, first_flag, shareable);
	if (res==0) {
		job_state.start_timer();
	} else {
		fprintf(stderr, "Could not prepare job (%s)!\n", job_name.c_str());
	}
	return res;
}
int FrameJob::release_job(pid_t tid) {
	int res = job_state.release_gpu(tid);
	job_state.end_timer();
	return res;
}


/* Implement the C-exposed interface for FrameJob objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateFrameJobObj(const char *fj_name, bool shareable_flag) {
	return reinterpret_cast<void *>(new FrameJob(fj_name, shareable_flag));
}
void DestroyFrameJobObj(void *fj_obj) {
	delete reinterpret_cast<FrameJob *>(fj_obj);
}
double FrameJob_get_expected_exec_time_for_tid(void *fj_obj, pid_t tid) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->get_expected_exec_time_for_tid(tid);
}
uint64_t FrameJob_get_expected_required_mem_for_tid(void *fj_obj, pid_t tid) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->get_expected_required_mem_for_tid(tid);
}
double FrameJob_get_wc_remaining_exec_time(void *fj_obj) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->get_wc_remaining_exec_time();
}
void FrameJob_update_wc_exec_time(void *fj_obj) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->update_wc_exec_time();
}
void FrameJob_set_wc_remaining_exec_time(void *fj_obj, double ret) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->set_wc_remaining_exec_time(ret);
}
int FrameJob_prepare_job(void *fj_obj, pid_t tid, double slacktime, bool first_flag) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->prepare_job(tid, slacktime, first_flag);
}
int FrameJob_release_job(void *fj_obj, pid_t tid) {
	FrameJob *fj = reinterpret_cast<FrameJob *>(fj_obj);
	return fj->release_job(tid);
}
#ifdef __cplusplus
}
#endif 

/* 
 * Object to contain run-time execution stats collection for registered frame-jobs
 * Facilitates computation of expected slack-time for frame-jobs, which are
 * sent to server to assist in job prioritization.
 * Object interface is thread-safe.
 */
FrameController::FrameController(const char *_frame_task_name, float _desired_fps) \
	: task_name(_frame_task_name), desired_fps(_desired_fps),
	frame_start_time(0), frame_deadline(0), runcount(0) {
	if (desired_fps <= 0.0) {
		throw std::invalid_argument("Bad desired FPS, must be non-negative!");
	}

	// Compute desired frame duration
	desired_frame_drn = 1. / desired_fps;
}
FrameController::~FrameController() {
	frame_jobs.clear(); // Destroy all FrameJobs while emptying frame_jobs
}

/* 
 * Registering a frame job returns a valid job id, or -1 on error.
 * Shareable flag parameter determines whether job is designated to be able
 * to share GPU with other processes' jobs
 */
int FrameController::register_frame_job(const char *frame_job_name, bool shareable) {
	// Use unique_lock to manage ctrl_lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(ctrl_lock); // Locks on construction

	frame_jobs.push_back(std::make_shared<FrameJob>(frame_job_name, shareable));
	return frame_jobs.size() - 1;
	// Unlocks on destruction of lck
}
int FrameController::unregister_frame_job(int job_id) {
	if (job_id < 0 || job_id > frame_jobs.size()-1) {
		return -1;
	}
	// Use unique_lock to manage ctrl_lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(ctrl_lock); // Locks on construction

	frame_jobs.erase(frame_jobs.begin() + job_id);
	return 0;
	// Unlocks on destruction of lck
}

// frame_*() functions assumed to be called by ONE thread
// Indicate the beginning and end of a frame
// TODO
void FrameController::frame_start() {
	std::chrono::system_clock::time_point now_tp = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point deadline_tp = now_tp; // TODO
	frame_start_time = 0; // TODO
}
void FrameController::frame_end() {
	// TODO:
	// TODO: print actual FPS

	// Every sampling period, sample the wc execution metrics
	if (runcount % SAMPLE_WC_PERIOD == 0) {
		// Use unique_lock to manage ctrl_lock
		std::unique_lock<std::mutex> lck;
		lck = std::unique_lock<std::mutex>(ctrl_lock); // Locks on construction

		// Loop over all FrameJobs to udpate_wc_exec_time() for each
		for (int i=0; i<frame_jobs.size(); i++) {
			FrameJob *fji = frame_jobs[i];
			fji->update_wc_exec_time();
		}
		// Looping from end of frame-jobs, update remaining exec time for each
		double cum_wc_remaining_exec_time = 0.0;
		for (int j=frame_jobs.size()-1; j>=0; j--) {
			FrameJob *fjj = frame_jobs[j];
			fjj->wc_remaining_exec_time = cum_wc_remaining_exec_time;
			cum_wc_remaining_exec_time += fjj->wc_exec_time;
		}
	}
	runcount++;
}
int FrameController::prepare_job_by_id(int job_id, pid_t tid) {
	if (job_id < 0 || job_id >= frame_jobs.size()) {
		return -1;
	}
	// Retrieve frame_job
	std::shared_ptr<FrameJob> fj = frame_jobs[job_id];
	double slacktime;
	bool first_flag;
	/*
	 * Compute slacktime relative to absolute deadline,
	 * return -3 (dropping frame error) if slacktime is negative
	 */
	compute_frame_job_slacktime(tid, job_id, &slacktime, &first_flag);
	if (slacktime < 0.0) {
		return -3;
	}
	// Prepare job with slacktime
	int res = fj->prepare_job(tid, slacktime, first_flag);
	if (res < 0) {
		return -2;
	}
}
int FrameController::release_job_by_id(int job_id, pid_t tid) {
	if (job_id < 0 || job_id >= frame_jobs.size()) {
		return -1;
	}
	// Retrieve frame_job
	std::shared_ptr<FrameJob> fj = frame_jobs[job_id];
	return fj->release_job(tid);
}

/*
* Slack-time computation at runtime relative to current absolute deadline
* which is only valid between frame_start/end() calls
* Sets input pointers on success (returning 0) with slacktime and 
* whether slacktime is initialized or not yet known
* Returns -1 on error, undefined values for input pointers
*/
int FrameController::compute_frame_job_slacktime(pid_t tid, int job_id, 
						double *slacktime_p, bool *initialize_flag_p) {
	if (job_id < 0 || job_id >= frame_jobs.size()) {
		return -1;
	}
	// Use unique_lock to manage ctrl_lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(ctrl_lock); // Locks on construction

	// Get expected execution time for frame-job
	std::shared_ptr<FrameJob> running_fj = frame_jobs[job_id];
	double exp_exec_time = running_fj->get_expected_exec_time_for_tid(tid);
	double wc_remaining_exec_time = running_fj->wc_remaining_exec_time;
	if (exp_exec_time < 0 || wc_remaining_exec_time < 0) {
		*initialize_flag_p = false;
		return -1.0;
	}
	// TODO: compute "now"
	double t_now = 0.0;
	// Compute slacktime relative to frame_deadline and remaining frame exec time
	*slacktime_p = (frame_deadline - t_now) - \
					   (exp_exec_time + wc_remaining_exec_time);
	*initialize_flag_p = true;
	return 0;
}


/* Implement the C-exposed interface for FrameController objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateFrameControllerObj(const char *frame_task_name, float desired_fps) {
	return reinterpret_cast<void *>(
				new FrameController(frame_task_name, desired_fps));
}
void DestroyFrameControllerObj(void *fc_obj) {
	delete reinterpret_cast<FrameController *>(fc_obj);
}
int FrameController_register_frame_job(void *fc_obj, const char *fj_name, bool shareable) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->register_frame_job(fj_name, shareable);
}
int FrameController_unregister_frame_job(void *fc_obj, int job_id) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->unregister_frame_job(job_id);
}
void FrameController_frame_start(void *fc_obj) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->frame_start();
}
void FrameController_frame_end(void *fc_obj) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->frame_end();
}
int FrameController_prepare_job_by_id(void *fc_obj, int job_id, pid_t tid) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->prepare_job_by_id(job_id, tid);
}
int FrameController_release_job_by_id(void *fc_obj, int job_id, pid_t tid) {
	FrameController *fc = reinterpret_cast<FrameController *>(fc_obj);
	return fc->release_job_by_id(job_id, tid);
}

#ifdef __cplusplus
}
#endif 
