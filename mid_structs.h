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

#define MAX_NAME_LENGTH 250

// ------------------------ Data structures ------------------------
// struct for a list of parameters to launch kernel
typedef struct launch_params {
    int num_blocks;
    int num_threads; 
} launch_params_t;

// struct for a list of kernel function parameters
// Assume 
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


// struct for a job, or a single GPU request;
typedef struct job {
    pid_t pid;                      // which client initiates this job? 
    //int job_id;                     // job num from the same client;
    char function_name[MAX_NAME_LENGTH]; 
    //char task_name[MAX_NAME_LENGTH];                  
    //int priority;
    //launch_params_t launch_params;
    //kernel_params_t kernel_params;
    //data_t *result_list;
    void *next;                     // this is a linked list
	int size;						// TODO: number of next elements + 1, 0 if empty
} job_t;



// GLOBAL struct that keeps track of all the jobs submitted to middleware, and
//  the list of completed jobs with their results that are passed back to 
//  clients 


// typedef struct global_jobs {
//     int is_active;                  // 1 after the server is started;
//     volatile int is_empty;          // 1 when there's no jobs in the queue; 
//     void *addr;                     // addr of this gloabl struct
//     int shm_fd; 
//     int total_count;  
//     char *name; 
//     job_t* jobs_submitted;          // pointer to the linked list of job_t structs
//     job_t* jobs_completed;         
// } global_jobs_t;    





#endif

