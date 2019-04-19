/* 
 * 18980 - Structs.h
 *      A header file that declares all of the data structures and constants 
 *      used in this project
 * Elim Zhang 
 * Nov. 29, 2018
 * ---------------------------------------------------------------
 * modified from trial_vec_add.cu from ../
 * 
 * To compile: gcc -Wall -Werror -o mid mid.c 
 *
*/
#ifndef STRUCTS_H
#define STRUCTS_H

#include <pthread.h> // pthread_mutex_t, pthread_cond_t
#include <time.h> // time_t

#define MAX_NAME_LENGTH 250

#define GLOBAL_JOBS_MAX_JOBS 100
#define JOB_MEM_NAME_MAX_LEN 25

// ------------------------ Data structures ------------------------
// struct for a list of parameters to launch kernel
typedef struct launch_params {
    int num_blocks;
    int num_threads;
} launch_params_t;

// struct for a list of kernel function parameters
typedef struct kernel_params {
    int num_args;
    void *data_list;                    // linked list of data structs,
                                        // with pointer to next argument
} kernel_params_t;

// a struct for passing data between client and server
typedef struct data {
    void *next_arg;                 // address of data array?
    void *data;
} data_t;

// a struct for passing data between client and server
typedef struct function {
	char *start_addr; 			// kust of addresses of different functions?
	int num_funcs;
} func_t;

enum job_type {QUEUED, COMPLETED};

// struct for a job, or a single GPU request;
typedef struct job {
	// job metadata
    pid_t tid;                      // tid (since client may be multithreaded)
    int priority;

	char job_name[MAX_NAME_LENGTH];
    double last_peak_mem;
	double avg_peak_mem;
	double worst_peak_mem;
    double last_exec_time;
   	double avg_execu_time;
    double worst_exec_time;
	unsigned int run_count;
	time_t deadline;
	enum job_type req_type;			// 'queued' or 'completed'
	
	// Client-side/server-side attrs - communication properties
	pthread_cond_t client_wake;		// cond_var used to block client until ready
	bool client_exec_allowed;		// flag determining whether client should execute when woken

	// Server-side attributes - linked-list properties
	int ll_size;						// TODO: number of next elements + 1, 0 if empty
    job *next;                     // this is a linked list
} job_t;



// GLOBAL struct that keeps track of all the jobs submitted to middleware, and
//  the list of completed jobs with their results that are passed back to 
//  clients 

typedef struct global_jobs {
    int is_active;                  // 1 after the server is started;
    volatile int is_empty;          // 1 when there's no jobs in the queue;
    int total_count;

	// Statically alloc'd array of client-created shared memory
	// object names, at which the job queue for this client can be found
	char job_names[GLOBAL_JOBS_MAX_JOBS][JOB_MEM_NAME_MAX_LEN];
	pthread_mutex_t requests_q_lock;

} global_jobs_t;





#endif

