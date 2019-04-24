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
// submit a job to the global jobs_queued queue; update total_count, 
// the jobs_list itself, 
void submit_job(global_jobs_t *gj, job_t *new_job, char *job_shm_name){

	// First, grab the requests_q_lock to modify job_name list
	pthread_mutex_lock(&(gj->requests_q_lock));
	
	// Then modify job_shm_names 
    memcpy(gj->job_shm_names[gj->total_count], job_shm_name,
		   JOB_MEM_NAME_MAX_LEN);    
    gj->total_count += 1;

	// Lastly, unlock
	pthread_mutex_unlock(&(gj->requests_q_lock));
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
bool job_is_in_queue(job_t *j, job_t **queue) {
	if (!j || !queue || !(*queue)) {
		// Bad input pointers!
		fprintf(stderr, "remove_from_queue: bad input pointer(s)!\n");
		return -1;
	}

	// Walk the job queue to find j
	job_t *c = *queue;
	while (c != NULL) {
		if (jobs_equal(c, j)) {
			return true;
		}
		c = c->next;	
	}
	return false;
}

/*
 * Does not assume rmj is in queue, returns failure if not found.
 * Assumes rmj could only be found once in queue, maximum.
 *
 * Returns 0 no removal, -1 on error, -2 if job not in queue
 */
int remove_from_queue(job_t *rmj, job_t **queue) {
	if (!rmj || !queue || !(*queue)) {
		// Bad input pointers!
		fprintf(stderr, "remove_from_queue: bad input pointer(s)!\n");
		return -1;
	}

	// First check that rmj is in queue
	if (!job_is_in_queue(rmj, queue)) {
		fprintf(stderr, "remove from queue: job (%s) is not in queue\n", rmj->job_name);
		return -2;
	}
	
	// Walk and update the linked list
	job_t *p = *queue;
	if (jobs_equal(p, rmj)) {
		// Modify queue to point to second queue element
		*queue = (*queue)->next;
		return 0;
	}
	while (p->next != NULL) {
		// Decrement all preceding jobs' ll_size-s by 1
		p->ll_size --;
		job_t *c = p->next;
		if (jobs_equal(c, rmj)) {
			// Remove c
			p->next = c->next;
			return 0;
		}
	}
	// This shouldn't happen
	fprintf(stderr, "remove from queue: somehow missed job (%s) in queue!\n",
			rmj->job_name);
	return -2;
}

int enqueue_job(job_t *new_job, job_t **queue){

    // Check for bad queue input
    if (queue == NULL || *queue == NULL) {
        return -1;
    } 
	if (new_job == NULL) {
		return -1;
	}

	// Ensure new_job is well-formed
	new_job->ll_size = 0;
  	new_job->next = NULL;

	// Walk to end of queue, place job there
    job_t *job_ptr = *queue;
    while (job_ptr->next != NULL){
        // walk to the tail of the jobs list;
        // and change the pointer to point to the new_job
		job_ptr->ll_size++;
        job_ptr = job_ptr->next;
    }
    // arrived at the last job; 
	job_ptr->ll_size++;
    job_ptr->next = new_job;

    return 0;
}

int dequeue_job_from_queue(job_t **queue, job_t **j_dest)
{
	if (queue == NULL || *queue == NULL) {
		fprintf(stderr, "Invalid queue as input to dequeue_job_from_queue\n");
		return -1;
	}
	if ((*queue)->ll_size < 1) {
		// Can't dequeue!
		fprintf(stderr, "Can't dequeue from job queue!\n");
		return -1;
	}
	if (j_dest == NULL) {
		// Can't save dequeued job
		fprintf(stderr, "Invalid pointer to save dequeued job!\n");
		return -1;
	}

	// Separate head of queue from rest of queue
	*j_dest = *queue;

	// Modify queue to point to second queue element
	*queue = (*queue)->next;
	(*j_dest)->ll_size = 0;
	(*j_dest)->next = NULL;

	return 0;
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
int peek_job_queued_at_i(global_jobs_t *gj, job_t **qd_job, int i)
{
	assert(gj != NULL);
	assert(gj->total_count > 0);
	assert(gj->total_count > i);

	// Grab the name from job_shm_names 
	char *job_mem_name = gj->job_shm_names[i];
	
	return get_shared_job(job_mem_name, qd_job);
}

