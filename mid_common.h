#ifndef COMMON_H
#define COMMON_H

#include "mid_queue.h"
// ------------------------ Constants ------------------------
#define JOB_MEM_NAME "job_mem"
#define DATA_MEM_NAME "data_mem"

#define SLEEP_MICROSECONDS (unsigned int)250 
#define JOB_MEM_SIZE JOB_QUEUE_LENGTH*sizeof(job_t)

int shm_init(const char *name);
int shm_destroy(const char *name, int fd);
int mmap_global_jobs_queue(int *fd, void **addr);
#endif

