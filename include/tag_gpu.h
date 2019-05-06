#ifndef TAG_GPU_H
#define TAG_GPU_H

#include <stdint.h> // uint64_t
#include <time.h> // time_t
#include "mid_structs.h" // MAX_NAME_LENGTH

/* Meta-data structs that describe stats for a repeated job_t struct */
typedef struct meta_job {
	// job_t metadata
    pid_t tid;                      // tid (since client may be multithreaded)
    int priority;

	char job_name[MAX_NAME_LENGTH];
    uint64_t last_peak_mem;			// In bytes
	uint64_t avg_peak_mem;
	uint64_t worst_peak_mem;
    double last_exec_time;
   	double avg_exec_time;
    double worst_exec_time;
	unsigned int run_count;
	time_t deadline;

	// running metadata fields
	uint64_t c_mem_util;
} meta_job_t;

/* C-exposed Interface for easier manipulation/creation of a meta_job */
#ifdef __cplusplus
extern "C" {
#endif
meta_job_t *CreateMetaJob(pid_t tid, int priority, const char *job_name, 
		uint64_t lpm, uint64_t apm, uint64_t wpm,
		double let, double aet, double wet,
		unsigned int run_count, time_t deadline);
void DestroyMetaJob(meta_job_t *mj);
#ifdef __cplusplus
}
#endif


/*
 * In order to communicate GPU execution metadata and intent to server,
 * we wrap any amount of work on the GPU (i.e. a function) with "raw"
 * tag_begin()/tag_begin() interface:
 *
 * tag_begin() tells server metadata about intended GPU execution, blocks
 ** until the server allows execution (may be sharing the GPU device)
 * tag_begin() is called after client work is completed to notify server
 ** that client releases GPU resources.
 *
 * See tag_decorator() wrapper function below to use stateful wrapping of 
 * repeated work-unit execution - wrapper will track metadata for subsequent
 * tag_begin() calls for the same work.
 *              */
#ifdef __cplusplus
extern "C" {
#endif
int tag_begin(pid_t tid, const char* unit_name,
			uint64_t last_peak_mem, 
			uint64_t avg_peak_mem, 
			uint64_t worst_peak_mem, 
			double last_exec_time,
			double avg_exec_time, 
			double worst_exec_time, 
			unsigned int run_count, 
			time_t deadline
		);

int tag_end(pid_t tid, const char *unit_name);
#ifdef __cplusplus
}
#endif

#endif /* TAG_GPU_H */
