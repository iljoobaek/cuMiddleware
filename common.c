#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>           // for mode constants 
#include <errno.h>
#include <unistd.h>          
#include <fcntl.h>              // for O_ constants, such as "O_RDWR"
#include <string.h>				// memset()
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
	gj->is_empty = 1;
	gj->__addr = *addr;
	gj->__shm_fd = *fd;
	gj->total_count = 0;
	gj->__name = GLOBAL_MEM_NAME;
	// Zero out the job_names array
	memset(gj->job_names, 0, GLOBAL_JOBS_MAX_JOBS * JOB_MEM_NAME_MAX_LEN);

	// Initialize the server-local 2 linked lists
	gj->__jobs_queued = NULL;
	gj->__jobs_completed = NULL;
	gj->__jobs_executing = NULL;
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
        // fprintf(stdout, fmt, ## args)
        printf("[INFO] shared memory is intialized for the first time\n");
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
int shm_destroy(const char *name, int fd){
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

int get_job_struct(const char *name, job_t **save_job)
{
	// Get fd for shared memory region designated by name
    errno = 0;              //  automatically set when error occurs
    int fd = shm_init(name); 
    if (fd == -1){
        perror("[Error] in get_job_struct: shm_init failed");
        return -1;
    }

	// Init GLOBAL_MEM_SIZE
    if (ftruncate(fd, name) != 0) {
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
