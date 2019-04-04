#include <sys/types.h>  // pid_t
#include <string.h>
#include "wrapcuda.h"
#include "mid_common.h"
#include "mid_structs.h"
#include "mid_queue.h"

extern "C"
int queue_job_to_server(pid_t pid, const char *fn_name, job_t *global_jobs_q)
{
	job_t job;
	job.pid = pid;
	strncpy(job.function_name, fn_name, MAX_NAME_LENGTH);	

	return enqueue_job(global_jobs_q, &job);
}
