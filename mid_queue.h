/* 
 * 18980 - queue 
 *      Function prototypes for both clients and server; 
 * ---------------------------------------------------------------
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
int enqueue_job(job_t *head, job_t *new_job);
int dequeue_job(job_t *head, job_t *cpy_job);
void queue_init(job_t *head);
void queue_destroy(job_t *head);
int queue_length(job_t *head);
#endif

