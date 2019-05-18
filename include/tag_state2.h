#ifndef TAG_STATE_H
#define TAG_STATE_H

#include <sys/types.h> /* pid_t */
#include <unistd.h>  // syscall()
#include <sys/syscall.h> // SYS_gettid

#ifdef __cplusplus
#include <unordered_map> // unordered_map
#include <iostream> // std::cout
#include <memory> /* std::shared_ptr */
#include <mutex> /* std::mutex */
#endif

#include "tag_gpu2.h" /* meta_job_t */

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

#ifdef __cplusplus
struct TagState {
	std::shared_ptr<meta_job_t> init_meta_job;
	std::mutex hm_lock;
	std::unordered_map<pid_t, std::shared_ptr<meta_job_t> >tid_to_meta_job;
	pid_t tag_pid;

	TagState();
	TagState(const meta_job_t *inp_init_meta_job);
	~TagState();

	/* Retrieve locally hashed current job metadata template:
	 * If tid has no entry in hashmap, 
	 * 	one is created copied from init_meta_job 
	 * Else, return pointer to stored in hashmap for tid
	 */
	meta_job_t *get_local_meta_job_for_tid(pid_t tid);

	// Request permission to run on GPU
	int acquire_gpu(pid_t tid, double slacktime, bool first_flag, bool shareable_flag);
	int release_gpu(pid_t tid);

	// Prepare to collect metrics during run of job
	void start_timer();
	void end_timer();

	/* Stats retrieval */
	double get_wc_exec_time_for_tid(pid_t tid) const ;  // Returns -1 if not yet known
	double get_max_wc_exec_time() const;				// Returns -1 if not yet known
	int64_t get_required_mem_for_tid(pid_t tid) const; // Returns -1 if not yet known
};
#endif

/* Implement the C-exposed interface for TagState objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateTagStateObj(const meta_job_t *inp_init_meta_job); 
void DestroyTagStateObj(void *tag_obj);
meta_job_t *TagState_get_local_meta_job_for_tid(void *tag_obj, pid_t tid);
int TagState_acquire_gpu(void *tag_obj, pid_t tid, double slacktime, bool first_flag, bool shareable_flag);
int TagState_release_gpu(void *tag_obj, pid_t tid);
void TagState_start_timer(void *tag_obj);
void TagState_end_timer(void *tag_obj);
double TagState_get_wc_exec_time_for_tid(void *tag_obj, pid_t tid);
uint64_t TagState_get_required_mem_for_tid(void *tag_obj, pid_t tid);
#ifdef __cplusplus
}
#endif

#endif /* TAG_STATE_H */

