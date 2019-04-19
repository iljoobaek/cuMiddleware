static job_t *jobs_executing;
static job_t *jobs_compeleted;
// -------------------------- Server Side Functions ----------------------------


void destroy_global_jobs()
{
	if (GJ_fd >= 0)
	{
		// Valid fd means preperly init'd
		queue_destroy(jobs_queued);
		queue_destroy(jobs_executing);
		queue_destroy(jobs_completed);
		shm_destroy(GLOBAL_MEM_NAME, GJ_fd);
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
	if ((res=init_global_jobs(&GJ_fd, &GJ)) < 0)
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
		while (queue_length(GJ) == 0)
		{
			/* If empty, first check if continue. Then sleep */
			if (!continue_flag)
			{
				// Break immediately
				break;
			}
			usleep(SLEEP_MICROSECONDS);
		}
		/* Empty GJ->__jobs_queued */
		while (GJ->total_count > 0)
		{
			fprintf(stdout, "Found a job in the job queue\n");
			
			job_t cpy_job;
			if ((res = peek_next_job_queued(GJ, &cpy_job)) < 0)
			{
				fprintf(stderr, "Failed to dequeue job! Exiting\n");
				continue_flag = false;
			}

			// Handle job synchronously for now, TODO
			handle_job(&cpy_job);
		}
	}
	destroy_global_jobs();
	
}
