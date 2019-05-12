#ifndef COMMON_H
#define COMMON_H

#include <stdio.h> 		// *printf
#include <assert.h>		// assert()
#include "mid_structs.h" // global_jobs_t
// ------------------------ Defines ------------------------
#define DEBUG_FN(fn, ...) \
{\
	int res;\
	if ((res = fn(__VA_ARGS__)) < 0) { \
		fprintf(stderr, "DEBUG_FN: %s returned %d!\n", #fn, res);\
		assert(false);\
	}\
}


#define GLOBAL_MEM_NAME "global_mem"
#define JOB_MEM_NAME "job_mem"
#define DATA_MEM_NAME "data_mem"

#define SLEEP_MICROSECONDS (unsigned int)250 
#define GLOBAL_MEM_SIZE sizeof(global_jobs_t)

/* Jobs will be saved in shared memory objects named like: <name>_<tid> */
#define JOB_SHM_NAME(tid, name_s) #name_s ## _ ## #tid

int shm_init(const char *name);
int shm_destroy(const char *name, int fd);
int init_global_jobs(int *fd, global_jobs_t **addr);
int get_shared_job(const char *name, job_t **save_job);

bool jobs_equal(job_t *a, job_t *b);
int build_shared_job(pid_t tid, const char *job_name,
						uint64_t lpm, uint64_t apm, uint64_t wpm,
						double let, double aet, double wet,
						unsigned int run_count,
						time_t deadline,
						enum job_type req_type,
						job_t **save_new_job,
						char *copy_shm_name);
int destroy_shared_job(job_t **shared_job_ptr);

#endif

