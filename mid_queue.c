
#include <assert.h>
#include <string.h>
#include "mid_common.h"
#include "mid_structs.h"
#include "mid_queue.h"

// TODO:
// 1) Global mutex in the global memory
// 2) Timeouts for queueing?

void queue_init(job_t *head)
{
	memset(head, 0, JOB_MEM_SIZE);
}

int queue_length(job_t *head)
{
	assert(head != NULL);
	return head->size;
}

void queue_destroy(job_t *head)
{
	// No need to free global memory array, 
	// memory will be unmapped on process termination
	memset(head, 0, JOB_MEM_SIZE);
}

int validate_job(job_t *new_job)
{
	// TODO: Better checks
	return new_job ? 1 : 0;
}

int enqueue_job(job_t *head, job_t *new_job)
{
	// Ensure that job queue isn't too big, block otherwise TODO
	// TODO <grab GLOBAL_JOB_QUEUE_LOCK>
	assert(queue_length(head) < JOB_QUEUE_LENGTH);

	// Check contents of new_job
	int is_valid;
	if ((is_valid = validate_job(new_job)) < 0)
	{
		fprintf(stderr, "validate new_job failed with err: %d", is_valid);
		return -1;
	}
	// Then walk queue, updating their sub-queue sizes
	job_t *curr= head;
	// Find the "last" job element in which to copy new_job 
	job_t *last;

	// Treat enqueue'ing first element differently
	if (curr->size== 0)
	{
		last = curr;
	}
	else
	{
		while (curr->size> 0)
		{
			// Increment sub-queue sizes
			curr->size ++;
			curr = (job_t *)curr->next;	
		}
		// Reached end of the queue	
		assert(head+head->size == curr);
		// Last element points to new job
		curr->size++;
		curr->next = curr+1;

		last = (job_t *)curr->next;
	}
	memcpy(last, new_job, sizeof(job_t));
	
	// Just in case, don't trust these values of new_job
	last->size = 1;
	last->next = NULL;
	return 0;
}

int dequeue_job(job_t *head, job_t *cpy_job)
{
	// Returns a copy of job at head of queue

	// TODO: I noticed there's going to be a lot of memory writes
	// because of the contiguous array-ness of the array
	if (queue_length(head) < 1)
	{
		return -1;
	}

	// Remove first job from queue
	job_t *deq_job = head;
	job_t *next_head = (job_t *)head->next;

	// Copy removed job to cpy_job
	memcpy(cpy_job, deq_job, sizeof(job_t));
	cpy_job->next = NULL;
	cpy_job->size= 0;

	// TODO: Detect when dequeuing first element, treat separately
	if (head->size == 1)
	{
		head->size = 0;
		head->next = NULL;	
	}
	else
	{
		// Move remaining jobs up from queue
		job_t *next_job = next_head;
		job_t *prev_job = head;
		while (next_job != NULL)
		{
			// Copy next to prev
			memcpy(prev_job, next_job, sizeof(job_t));
			prev_job->next = next_job;

			// Walk down list
			prev_job = next_job;
			next_job = (job_t *)next_job->next;
		}
	}

	return 0;
}
