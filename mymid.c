
/*
 * Simplified mid.c
 * Just initializes the global JOB_QUEUE (preallocates a few job structs)
 * TODO: Initialize a global DATA_QUEUE to use with JOB_QUEUE to pass
 * * Note: mmap'd data should hold only INDICES, not pointers
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>           // for mode constants 
#include <unistd.h>          
#include <fcntl.h>              // for O_ constants, such as "O_RDWR"
#include <signal.h>             // sigint can trigger the clean up process;
#include <dlfcn.h>				// dlsym, RTLD_DEFAULT

#include "mid_structs.h"
#include "mid_queue.h"
#include "mid_common.h"

static job_t *global_jobs_q;
static int jobs_q_fd;
// -------------------------- Server Side Functions ----------------------------

int init_global_jobs_queue()
{
	int res = mmap_global_jobs_queue(&jobs_q_fd, (void**)&global_jobs_q);
	if (res < 0)
	{
		fprintf(stderr, "Failed to mmap global jobs queue!\n");
		return -1;
	}
	// Init the queue starting at mapped GLOBAL_JOBS_QUEUE addr
	queue_init(global_jobs_q); 
	return 0;
}

void destroy_global_jobs_queue()
{
	if (jobs_q_fd >= 0)
	{
		// Valid fd means preperly init'd
		queue_destroy(global_jobs_q);
		shm_destroy(JOB_MEM_NAME, jobs_q_fd);
	}
}

static bool continue_flag = true;
void handle_sigint(int ign)
{
	// Just flip the continue flag
	continue_flag = false;
}

void handle_job(job_t *c_job)
{
	// First, read the function name from job object
	const char *fn_name = (const char *)c_job->function_name;
	pid_t pid = c_job->pid;
	fprintf(stdout, "Handling job with function_name: %s, pid %d\n", fn_name, pid);

	// For now, just detect that we can find the kernel definition
	// And use PID to kill client
	void *handle = dlopen("./tf_kernels.so", RTLD_NOW);
	char *error;
	if ((error = dlerror()) != NULL) 
	{
		fprintf(stderr, "%s\n", error);
	}
	void *kernel_fn = dlsym(handle, fn_name);
	if (!kernel_fn)
	{
		fprintf(stderr, "Could not find kernel in dynamically loaded libraries!\n");
	}
	else
	{
		fprintf(stdout, "Found kernel at addr: %p\n", kernel_fn);
	}
	fprintf(stdout, "After 3 seconds, sending SIGCONT to client at pid %d\n...",	pid);
	sleep(3);
	kill(pid, SIGCONT);

	return;
}

int main(int argc, char **argv)
{
	fprintf(stdout, "Starting up middleware main...\n");

	int res;
	if ((res=init_global_jobs_queue()) < 0)
	{
		fprintf(stderr, "Failed to init global jobs queue");
		return EXIT_FAILURE;
	}

	// Set up signal handler
	signal(SIGINT, handle_sigint);

	// Global jobs queue has been initialized
	// Begin waiting for jobs to be enqueued (sample every 250ms)
	while (continue_flag)
	{
		while (queue_length(global_jobs_q) == 0)
		{
			if (!continue_flag)
			{
				// Break immediately
				break;
			}
			usleep(SLEEP_MICROSECONDS);
		}
		if (queue_length(global_jobs_q) > 0)
		{
			fprintf(stdout, "Found a job in the job queue\n");
			
			job_t cpy_job;
			if ((res = dequeue_job(global_jobs_q, &cpy_job)) < 0)
			{
				fprintf(stderr, "Failed to dequeue job! Exiting\n");
				continue_flag = false;
			}

			// Handle job synchronously for now, TODO
			handle_job(&cpy_job);
		}
	}
	destroy_global_jobs_queue();
	
}
