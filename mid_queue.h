/* 
 * 18980 - queue 
 *      Function prototypes for both clients and server; * ---------------------------------------------------------------
 * 
 * 
 * To compile: gcc -Wall -Werror -o mid mid.c 
 *
*/
#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include "mid_structs.h"

// Constants
#define JOB_QUEUE_LENGTH 10

// ----------------------------- Client Functions ------------------------------
// submit a job to the global jobs_submitted queue; update total_count, is_empty,
// the jobs_list itself, 
void submit_job(global_jobs_t *gj, job_t *new_job, char *job_shm_name);

// is_job_finished: checks if a job is within the jobs_completed list.
// returns 1 if client's job has finished, with job_address pointer pointing to
// the results 
int is_job_finished(int job_id, void *jobs_list);

// is_queue_full: returns 1 if the queue is full and the client has to wait; 
int is_queue_full();

// ---------------------------- Server Functions -------------------------------
int remove_from_queue(job_t *rmj, job_t **queue);

int enqueue_job(job_t *new_job, job_t **queue);
int dequeue_job_from_queue(job_t **queue, job_t **j_dest);

// unmap every job in the queue, including each job structs themselves. 
void unmap_queue(job_t *jobs_list);

int peek_job_queued_at_i(global_jobs_t *gb_jobs, job_t **qd_job, int i);

#endif

