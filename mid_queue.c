/* 
 * 18980 - Middleware
 *      Middleware that follows server-client architecture, and handles clients'
 #      GPU requests by launching CUDA streams for them
 * Elim Zhang 
 * Nov. 27, 2018
 * ---------------------------------------------------------------
 * 
 * To compile: gcc -Wall -Werror -o queue queue.c 
 *
*/

#include <string.h>
#include <assert.h>
#include "mid_common.h"
#include "mid_structs.h"
#include "mid_queue.h"

// ------------------------------ Client Functions ------------------------------
// visible to client: 
// submit a job to the global jobs_queued queue; update total_count, is_empty, 
// the jobs_list itself, 
void submit_job(void *gb_jobs, void *new_job, char *job_name){
    global_jobs_t *global_jobs = (global_jobs_t *)gb_jobs;

    memcpy(global_jobs->job_names[global_jobs->total_count], job_name,
		   JOB_MEM_NAME_MAX_LEN);    

    global_jobs->is_empty = 0;
    global_jobs->total_count += 1;
    return;
}

// is_job_finished: checks if a job is within the jobs_completed list.
// returns 1 if client's job has finished, with job_address pointer pointing to
// the results 
int is_job_finished(int job_id, void *jobs_list){
    return 0;
}

// is_queue_full: returns 1 if the queue is full and the client has to wait; 
int is_queue_full(){
    return 0;
}


// ---------------------------- Server Functions -------------------------------
// TODO: Currently being unused
// from jobs_queued list, remove max_job from the linked list 
void remove_from_queue(global_jobs_t *global_jobs, job_t *job_to_remove){
    job_t *job = global_jobs->__jobs_queued; 
    job_t *prev_job = NULL;

    // update the linked list; 
    while (job!= NULL){
        if (job == job_to_remove){
            // found the job; just remove it
            // edge: head of the list
            if (prev_job == NULL){
                global_jobs->__jobs_queued = (job_t *)job->next;
            } else {
                prev_job->next = job->next; 
            }
            break;
        }
        // traverse to the next job;
        job = (job_t *)job->next;
        prev_job = job;
    }
    return;
}

// TODO: Currently being unused
void enqueue_jobs_completed(void *gb_jobs, void *new_job){
    global_jobs_t *global_jobs = (global_jobs_t *)gb_jobs;

    // edge: first item in the queue: 
    if (global_jobs->__jobs_completed == NULL){
        global_jobs->__jobs_completed = (job_t *)new_job;
        return;
    } 
    
    job_t *last_job_ptr = global_jobs->__jobs_completed; 
    while (last_job_ptr->next != NULL){
        // walk to the tail of the jobs list;
        // and change the pointer to point to the new_job
        last_job_ptr = (job_t *)last_job_ptr->next;
    }
    // arrived at the last job; 
    last_job_ptr->next = (job_t *)new_job;
    return;
}

void enqueue_jobs_executing(void *gb_jobs, void *new_job){
    global_jobs_t *global_jobs = (global_jobs_t *)gb_jobs;

    // edge: first item in the queue: 
    if (global_jobs->__jobs_executing == NULL){
        global_jobs->__jobs_executing = (job_t *)new_job;
        return;
    } 
    
    job_t *last_job_ptr = global_jobs->__jobs_executing; 
    while (last_job_ptr->next != NULL){
        // walk to the tail of the jobs list;
        // and change the pointer to point to the new_job
        last_job_ptr = (job_t *)last_job_ptr->next;
    }
    // arrived at the last job; 
    last_job_ptr->next = (job_t *)new_job;
    return;
}

// TODO: Currently being unused
// free every job in the queue, including each job structs themselves. 
void free_queue(job_t *jobs_list){
    job_t *jobs_ptr = jobs_list;
    job_t *curr_job;
    while (jobs_ptr != NULL){
        curr_job = jobs_ptr;
        jobs_ptr = (job_t *)jobs_ptr->next;
        free(curr_job);
    }
    free(jobs_list);
    return;
}

/* TODO: Not thread-safe */
void submit_job(void *gb_jobs, void *new_job, char *job_name){
    global_jobs_t *global_jobs = (global_jobs_t *)gb_jobs;

    memcpy(global_jobs->job_names[global_jobs->total_count], job_name,
		   JOB_MEM_NAME_MAX_LEN);    

    global_jobs->is_empty = 0;
    global_jobs->total_count += 1;
    return;
}

/* TODO: Not thread-safe */
int peek_next_job_queued(void *gb_jobs, job_t **qd_job)
{
	global_jobs_t *gj = (global_jobs_t *)gb_jobs;

	assert(gj != NULL);
	assert(gj->total_count > 0);

	char *job_mem_name = &(gj->job_names[0]);

	return get_job_struct(job_mem_name, qd_job);
}

