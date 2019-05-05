#include <sys/types.h> /* pid_t */
#include <mutex> /* std::unique_lock, std::mutex */
#include <unordered_map> // unordered_map

#include "tag_dec.h" // *_running_meta_job_for_tid, TagDecorator<R(Args...)>, tag_decorator()
#include "tag_gpu.h" /* meta_job_t */

static std::mutex hm_lock;
static std::unordered_map<pid_t, meta_job_t *> g_tid_curr_job;

/* Interface for getting meta_job_t from g_tid_curr_job */
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

