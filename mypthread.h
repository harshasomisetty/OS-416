// File:	mypthread_t.h

// List all group member's name:
// username of iLab: hnm21@ilab3
// iLab Server: ilab3.cs.rutgers.edu

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

//#define _GNU_SOURCE
#define _XOPEN_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */



/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

typedef uint mypthread_t;

#define READY 0           // e.g., thread->status = READY;
#define SCHEDULED 1
#define BLOCKED 2

#define STACKSIZE (1<<15)

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
        mypthread_t id;
        
        // thread status
        int status;
        
	// thread context
        ucontext_t * context;
        
	// thread stack, in context

        
	// thread priority is based on elapsed time
        int elapsed;
        
	// And more ...

} tcb;

/* priority queue nodes for thread control blocks */
typedef struct pthread_node {
        tcb* data;
        struct pthread_node* next;
} pthread_node;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */

	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE


/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);


void makeScheduler();

#define USE_MYTHREAD 1
#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
