#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>           // for mode constants 
#include <errno.h>
#include <unistd.h>          
#include <fcntl.h>              // for O_ constants, such as "O_RDWR"
#include <string.h>				// memset(), strncpy
#include <stdio.h>				// *printf, perror
#include "mid_structs.h"		// global_jobs_t, GLOBAL_JOBS_MAX*, JOB_MEM_NAME_MAX_LEN
#include "mid_common.h"

int init_global_jobs(int *fd, global_jobs_t **addr)
{
	// Get fd for shared GLOBAL_MEM_SIZE memory region
    errno = 0;              //  automatically set when error occurs
    *fd = shm_init(GLOBAL_MEM_NAME); 
    if (*fd == -1){
        perror("[Error] in mmap_global_jobs_queue: shm_init failed");
        return -1;
    }

	// Init GLOBAL_MEM_SIZE
    if (ftruncate(*fd, GLOBAL_MEM_SIZE) != 0) {
        perror("ftruncate");
    }

    *addr = mmap(  NULL,
                        GLOBAL_MEM_SIZE,
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED,
                        *fd,
                        0);

    if (*addr == MAP_FAILED) {
        perror("[Error] in mmap_global_jobs_queue: mmap");
        return -1;
    }

	/* Initialize the global_jobs_t struct in shared memory */
	global_jobs_t *gj = (global_jobs_t *) (*addr);
	gj->is_active = 1;
	gj->total_count = 0;
	// Zero out the job_shm_names array
	memset(gj->job_shm_names, 0, GLOBAL_JOBS_MAX_JOBS * JOB_MEM_NAME_MAX_LEN);

	// Initialize the global lock on the job_shm_names list
	pthread_mutexattr_t mutex_attr;

	// Initialize requests_q_lock to be PROCESS_SHARED
	(void) pthread_mutexattr_init(&mutex_attr);
	(void) pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

	(void) pthread_mutex_init(&(gj->requests_q_lock), &mutex_attr);
	
	return 0;
}

// helper functions for creating space in shared memory: 
// shm_init: returns the file descriptor after creating a shared memory. returns
//  -1 on error; 
int shm_init(const char *name){
    errno = 0;              //  automatically set when error occurs
    // open/create POSIX shared memory; if it doesn't exist, create one. 
    int fd;
    fd = shm_open(name, O_RDWR, 0660); 
    if (errno == ENOENT){
        fprintf(stdout, "[INFO] shared memory is intialized for the first time\n");
        fd = shm_open(name, O_RDWR|O_CREAT, 0660);
    }
    if (fd == -1){
        perror("[Error] in shm_init: shm_open");
        return -1;
    }
    
    return fd;
}

/* shm_destroy: close the file descriptor, and unlink shared memory;
* returns 0 if successful, -1 otherwise
*/ 
int shm_destroy(const char *name, int fd) {
    // close the file descriptor
    if (close(fd)){
        perror("[Error] in shm_destroy: closing file descriptor");
        return -1; 
    }
    // unlink shared memory
    if (shm_unlink(name)){
        perror("[Error] in shm_destroy: unlinking shm");
        return -1;
    }
    return 0;
}

/* Note: Not thread-safe, assumes only one job queued per tid */
int get_shared_job(const char *name, job_t **save_job)
{
	// Get fd for shared memory region designated by name
    errno = 0;              //  automatically set when error occurs
    int fd = shm_init(name); 
    if (fd == -1){
        perror("[Error] in get_shared_job: shm_init failed");
        return -1;
    }

	// Init shm to have size of job_t struct
    if (ftruncate(fd, sizeof(job_t)) != 0) {
        perror("ftruncate");
    }

	// Mmap the job_t object stored in shared memory
	job_t *shm_job = mmap(  NULL,
                        sizeof(job_t),
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        0);

    if (shm_job == MAP_FAILED) {
        perror("[Error] in mmap_global_jobs_queue: mmap");
        return -1;
    }

	// Client opened the shared memory, it should still have a reference to 
	// This only destroys local connection to shared memory object
	shm_destroy(name, fd);
	*save_job = shm_job;
 	return 0;	
}

bool jobs_equal(job_t *a, job_t *b) {
	return ((a && b) \
			&& a->tid == b->tid \
			&& (strcmp(a->job_name, b->job_name) == 0));
}

int build_job(pid_t tid, const char *job_name,
						uint64_t lpm, uint64_t apm, uint64_t wpm,
						double let, double aet, double wet,
						unsigned int run_count,
						time_t deadline,
						enum job_type req_type,
						job_t *shm_job) {
	// Fail on bad mapped job
	if (!shm_job) return -1;
	
	// Init job with metadata
	shm_job->tid = tid;
	shm_job->priority = 0; // TODO
	strncpy(shm_job->job_name, job_name, JOB_MEM_NAME_MAX_LEN);
	shm_job->last_peak_mem = lpm;
	shm_job->avg_peak_mem = apm;
	shm_job->worst_peak_mem = wpm;
	shm_job->last_exec_time = let;
	shm_job->avg_exec_time = aet;
	shm_job->worst_exec_time = wet;
	shm_job->run_count = run_count;
	shm_job->deadline = deadline;
	shm_job->req_type = req_type;

	// Init client-server locks and state
	pthread_condattr_t cond_attr;
	pthread_mutexattr_t mutex_attr;

	// Initialize client_wake cond_var to be PROCESS_SHARED
	(void) pthread_condattr_init(&cond_attr);
	(void) pthread_mutexattr_init(&mutex_attr);

	(void) pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
	(void) pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

	(void) pthread_mutex_init(&(shm_job->own_job), &mutex_attr);
	(void) pthread_cond_init(&(shm_job->client_wake), &cond_attr);

	shm_job->client_exec_allowed = true;

	// Init linked-list attrs of new job
	shm_job->ll_size = 1;
	shm_job->next = NULL;

 	return 0;	
}

int build_shared_job(pid_t tid, const char *job_name,
						uint64_t lpm, uint64_t apm, uint64_t wpm,
						double let, double aet, double wet,
						unsigned int run_count,
						time_t deadline,
						enum job_type req_type,
						job_t **save_new_job,
						char *copy_shm_name) {
	// First construct name for shared memory region
	char job_shm_name[JOB_MEM_NAME_MAX_LEN];
	sprintf(job_shm_name, "%s_%d", job_name, tid);

	// Get fd for shared memory region designated 
    errno = 0;              //  automatically set when error occurs
    int fd = shm_init(job_shm_name); 
    if (fd == -1){
        perror("[Error] in get_shared_job: shm_init failed");
        return -1;
    }

	// Init job_t-sized shm object 
    if (ftruncate(fd, sizeof(job_t)) != 0) {
		close(fd);
        perror("ftruncate");
    }


	// Mmap the job_t object stored in shared memory
	job_t *shm_job = mmap(  NULL,
                        sizeof(job_t),
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        0);

	// Close opened fd now that job is mapped
	DEBUG_FN(close, fd);

    if (shm_job == MAP_FAILED) {
        perror("[Error] in mmap_global_jobs_queue: mmap");
        return -1;
    }
	
	// Save job and job_shm_name for caller
	DEBUG_FN(build_job, tid, job_name,
						lpm, apm, wpm,
						let, aet, wet,
						run_count,
						deadline,
						req_type, shm_job);
	*save_new_job = shm_job;
	strncpy(copy_shm_name, job_shm_name, JOB_MEM_NAME_MAX_LEN);
	return 0;
}
