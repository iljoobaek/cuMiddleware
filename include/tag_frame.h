#ifndef TAG_FRAME_H
#define TAG_FRAME_H

#include <sys/types.h> /* pid_t */
#ifdef __cplusplus
#include <dequeue> 	// std::dequeue
#include <mutex> /* std::mutex */
#endif

#include "tag_state.h" /* TagState */

#ifdef __cplusplus
struct FrameJob {
	std::string job_name;
	TagState job_state; 		// Current per-thread execution stats of job
	bool shareable;				// Determines whether job can share GPU with other processes

	// (sampled) w.c. expected exec time for this job for all threads using assoc.'d job_state
	// -1 if not yet initialized
	double wc_exec_time;
	// (sampled) w.c. expected exec time for remaining jobs in frame
	// -1 if not yet initialized
	double wc_remaining_exec_time;

	FrameJob(const char* _job_name, bool shareable_flag);
	~FrameJob();
	
	/* Expected execution stats retrieval */
	double get_wc_remaining_exec_time();
	double get_expected_exec_time_for_tid(pid_t tid);
	uint64_t get_expected_required_mem_for_tid(pid_t tid);

	/* Sample wc_exec_time */
	// This is run every N
	void update_wc_exec_time();
	void set_wc_remaining_exec_time(double rem_exec_time);

	/* Job preparation/release */
	// Returns -1 if job must be aborted, 0 on successful preparation
	// On success, starts timer for TagState object
	int prepare_job(pid_t tid, double slacktime, bool first_flag);
	int release_job(pid_t tid);
}
#endif

/* C-exposed interface for FrameJob */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateFrameJobObj(const char *fj_name, const void* tag_state_obj); 
void DestroyFrameJobObj(void *fj_obj);
double FrameJob_get_expected_exec_time_for_tid(void *fj_obj, pid_t tid);
uint64_t FrameJob_get_expected_required_mem_for_tid(void *fj_obj, pid_t tid);
double FrameJob_get_wc_remaining_exec_time(void *fj_obj);
void FrameJob_update_wc_exec_time(void *fj_obj);
void FrameJob_set_wc_remaining_exec_time(void *fj_obj, double rem_exec_time);
int FrameJob_prepare_job(void *fj_obj, pid_t tid, double slacktime, bool first_flag);
int FrameJob_release_job(void *fj_obj, pid_t tid);
#ifdef __cplusplus
}
#endif


/* 
 * Object to contain run-time execution stats collection for registered frame-jobs
 * Facilitates computation of expected slack-time for frame-jobs, which are
 * sent to server to assist in job prioritization.
 * Object interface is thread-safe.
 */
#define SAMPLE_WC_PERIOD 10 // Sample worst-case metrics every 10 frames

#ifdef __cplusplus
struct FrameController {
	std::deque<std::shared_ptr<FrameJob> > frame_jobs;
	std::string task_name;
	float desired_fps;
	float desired_frame_drn;
	time_t frame_start_time;
	time_t frame_deadline;
	unsigned int runcount;
	std::mutex ctrl_lock;

	FrameController(const char *frame_task_name, float desired_fps);
	~FrameController();

	/* Registration calls to characterize work within a "frame" of repetitive work*/
	// Jobs are designated by job_id (int)
	int register_frame_job(const char *frame_job_name, bool shareable); // -1 on error, valid job id on success
	int unregister_frame_job(int job_id); // -1 on error, 0 on success

	// Indicate the beginning and end of a frame
	void frame_start();
	void frame_end();

	/* Job preparation and release by job_id */
	// returns 0 on success, -1 bad job_id, -2 if job must abort, -3 if dropping frame
	// Note (TODO): not threadsafe! Assumes registration/unregistration will never overlap execution
	int prepare_job_by_id(int job_id, pid_t tid);
	// Returns 0 on successfully releasing GPU for job
	int release_job_by_id(int job_id, pid_t tid);

	private:
		/*
		 * Slack-time computation at runtime relative to current absolute deadline
		 * which is only valid between frame_start/end() calls
		 * Sets input pointers on success (returning 0) with slacktime and 
		 * whether slacktime is initialized or not yet known
		 * Returns -1 on error, undefined values for input pointers
		 */
		int compute_frame_job_slacktime(pid_t tid, int job_id,
				double *slacktime_p, bool *initialize_flag_p);
};
#endif

/* Implement the C-exposed interface for FrameController objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateFrameControllerObj(const char *frame_task_name, float desired_fps);
void DestroyFrameControllerObj(void *fc_obj);
int FrameController_register_frame_job(void *fc_obj, const char *fj_name, bool shareable);
int FrameController_unregister_frame_job(void *fc_obj, int job_id);
void FrameController_frame_start(void *fc_obj);
void FrameController_frame_end(void *fc_obj);
int FrameController_prepare_job_by_id(void *fc_obj, int job_id, pid_t tid);
int FrameController_release_job_by_id(void *fc_obj, int job_id, pid_t tid);
#ifdef __cplusplus
}
#endif

#endif /* TAG_FRAME_H */