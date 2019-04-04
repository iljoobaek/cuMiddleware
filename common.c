#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>           // for mode constants 
#include <errno.h>
#include <unistd.h>          
#include <fcntl.h>              // for O_ constants, such as "O_RDWR"
#include "mid_common.h"

int mmap_global_jobs_queue(int *fd, void **addr)
{
	// Get fd for shared JOBS_QUEUE memory region
    errno = 0;              //  automatically set when error occurs
    *fd = shm_init(JOB_MEM_NAME); 
    if (*fd == -1){
        perror("[Error] in mmap_global_jobs_queue: shm_init failed");
        return -1;
    }

	// Init jobs queue to contain 10 job structs - it's basically an array
    if (ftruncate(*fd, JOB_MEM_SIZE) != 0) {
        perror("ftruncate");
    }

    *addr = mmap(  NULL,
                        JOB_MEM_SIZE,
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED,
                        *fd,
                        0);

    if (*addr == MAP_FAILED) {
        perror("[Error] in mmap_global_jobs_queue: mmap");
        return -1;
    }
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
