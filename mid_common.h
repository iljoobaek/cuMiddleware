#ifndef COMMON_H
#define COMMON_H

#include "mid_structs.h" // global_jobs_t
#include "mid_queue.h"
// ------------------------ Constants ------------------------
#define GLOBAL_MEM_NAME "global_mem"
#define JOB_MEM_NAME "job_mem"
#define DATA_MEM_NAME "data_mem"

#define SLEEP_MICROSECONDS (unsigned int)250 
#define GLOBAL_MEM_SIZE sizeof(global_jobs_t)

int shm_init(const char *name);
int shm_destroy(const char *name, int fd);
int init_global_jobs(int *fd, void **addr);
int get_job_struct(const char *name, job_t **save_job);
#endif

