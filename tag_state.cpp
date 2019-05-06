#include <cstring> /* memcpy */
#include <sys/types.h> /* pid_t */
#include <mutex> /* std::unique_lock, std::mutex */
#include <unordered_map> // unordered_map

#include "tag_state.h" // *_running_meta_job_for_tid, TagState class
#include "tag_gpu.h" /* meta_job_t */

static std::mutex hm_lock;
static std::unordered_map<pid_t, meta_job_t *> g_tid_curr_job;

/* Interface for getting meta_job_t from g_tid_curr_job */
#ifdef __cplusplus
extern "C" {
#endif 
meta_job_t *get_running_meta_job_for_tid(pid_t tid) {
	// Use unique_lock to manage hashmap lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(hm_lock); // Locks on construction

	if (g_tid_curr_job.find(tid) == g_tid_curr_job.end()) {
		return NULL;
	} else {
		return g_tid_curr_job[tid];
	}
	// Unlocks mutex on destruction
}

int set_running_job_for_tid(pid_t tid, meta_job_t *mj) {
	// Use unique_lock to manage hashmap lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(hm_lock); // Locks on construction

	g_tid_curr_job[tid] = mj;
	return 0;
	// Unlocks on destruction
}

#ifdef __cplusplus
}
#endif

/* Define TagState class implementation */
	/* Default constructor */
TagState::TagState() : init_meta_job(std::make_shared<meta_job_t>()) {
	memset(init_meta_job.get(), 0, sizeof(meta_job_t));
}

TagState::TagState(const meta_job_t *inp_init_meta_job) : init_meta_job(std::make_shared<meta_job_t>()) {
	/* Init a template job for all threads calling this function */
	memcpy(init_meta_job.get(), inp_init_meta_job, sizeof(meta_job_t));
	init_meta_job->tid = 0;
	}

TagState::~TagState() { 
	tid_to_meta_job.clear();
}

/* Retrieve locally hashed current job metadata template:
 * If tid has no entry in hashmap, 
 * 	one is created copied from init_meta_job 
 * Else, return pointer to stored in hashmap for tid
 */
meta_job_t *TagState::get_local_meta_job_for_tid(pid_t tid) {
	// Use unique_lock to manage hashmap lock
	std::unique_lock<std::mutex> lck;
	lck = std::unique_lock<std::mutex>(hm_lock); // Locks on construction

	if (tid_to_meta_job.find(tid) == tid_to_meta_job.end()) {
		// No meta_job entry present, must create new one 
		// as a copy of init_meta_job
		std::shared_ptr<meta_job_t> new_mj =\
			std::shared_ptr<meta_job_t>(new meta_job_t);
		memcpy(new_mj.get(), init_meta_job.get(), sizeof(meta_job_t));

		// Tweak a few parameters of new_mj
		new_mj->tid = tid;

		tid_to_meta_job.insert(std::make_pair(tid, new_mj));
	}

	// Return the stored pointer in shared_ptr saved in hashmap
	return tid_to_meta_job[tid].get();
	// Unlocks on destruction of lck
}

	// Use meta job to construct call to tag_end
int TagState::tag_end_from_meta_job(meta_job_t *mj) {
	return tag_end(mj->tid, mj->job_name);
}
	
// Use meta job to construct call tag_begin 
int TagState::tag_begin_from_meta_job(meta_job_t *mj) {
	return tag_begin(mj->tid,
			mj->job_name,
			mj->last_peak_mem,
			mj->avg_peak_mem,
			mj->worst_peak_mem,
			mj->last_exec_time,
			mj->avg_exec_time,
			mj->worst_exec_time,
			mj->run_count,
			mj->deadline);
}

int TagState::acquire_gpu() {
	// First, retrieve tid and local copy of current metadata for job
	pid_t call_tid = gettid();
	meta_job_t *curr_meta_job = get_local_meta_job_for_tid(call_tid);

	// Set the global hashmap of tid->job metadata to point
	// to this thread's job (i.e. for this tid)
	set_running_job_for_tid(call_tid, curr_meta_job);

	if (tag_begin_from_meta_job(curr_meta_job) < 0) {
		// Job is aborted, raise error for client 
		fprintf(stderr, "Job (%s) was aborted! Raising SIGSEGV!\n", curr_meta_job->job_name);
		return -1;
	}	
	return 0;
}

int TagState::release_gpu() {
	// First, retrieve tid and local copy of current metadata for job
	pid_t call_tid = gettid();
	meta_job_t *curr_meta_job = get_local_meta_job_for_tid(call_tid);

	// Notify server of end of work
	TagState::tag_end_from_meta_job(curr_meta_job);
	
	curr_meta_job->run_count++;
	return 0;
}

void TagState::start_timer() {
	// TODO
}

void TagState::end_timer() {
	// TODO
}


/* Implement the C-exposed interface for TagState objects */
#ifdef __cplusplus
extern "C" {
#endif 
void *CreateTagStateObj(const meta_job_t *inp_init_meta_job) {
	return reinterpret_cast<void *>(new TagState(inp_init_meta_job));
}
void DestroyTagStateObj(void *tag_obj) {
	delete reinterpret_cast<TagState *>(tag_obj);
}
meta_job_t *TagState_get_local_meta_job_for_tid(void *tag_obj, pid_t tid) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->get_local_meta_job_for_tid(tid);
}
int TagState_tag_end_from_meta_job(void *tag_obj, meta_job_t *mj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->tag_end_from_meta_job(mj);
}
int TagState_tag_begin_from_meta_job(void *tag_obj, meta_job_t *mj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->tag_begin_from_meta_job(mj);
}
int TagState_acquire_gpu(void *tag_obj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->acquire_gpu();
}
int TagState_release_gpu(void *tag_obj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->release_gpu();
}
void TagState_start_timer(void *tag_obj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->start_timer();
}
void TagState_end_timer(void *tag_obj) {
	TagState *ts = reinterpret_cast<TagState *>(tag_obj);
	return ts->end_timer();
}
#ifdef __cplusplus
}
#endif 

