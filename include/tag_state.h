#ifndef TAG_STATE_H
#define TAG_STATE_H

#include <sys/types.h> /* pid_t */
#include <memory> /* std::shared_ptr */
#include <mutex> /* std::mutex */
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

/* Implement the C-exposed interface for TagState objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateTagStateObj(const meta_job_t *inp_init_meta_job); 
void DestroyTagStateObj(void *tag_obj);
meta_job_t *TagState_get_local_meta_job_for_tid(void *tag_obj, pid_t tid);
int TagState_tag_end_from_meta_job(void *tag_obj, meta_job_t *mj);
int TagState_tag_begin_from_meta_job(void *tag_obj, meta_job_t *mj);
int TagState_acquire_gpu(void *tag_obj);
int TagState_release_gpu(void *tag_obj);
void TagState_start_timer(void *tag_obj);
void TagState_end_timer(void *tag_obj);
#ifdef __cplusplus
}
#endif

#endif /* TAG_STATE_H */

