// File:	mypthread.c

// List all group member's name: Harsha Somisetty, Hugo De Moraes
// username of iLab: hs884
// iLab Server: ilab3.cs.rutgers.edu

#include "mypthread.h"


// INITAILIZE ALL YOUR VARIABLES HERE
int thread_count = 0;

pthread_node * head = NULL;


pthread_node* newNode(tcb* thread){
    pthread_node *node = (struct pthread_node*) malloc(sizeof(pthread_node));
    (*node).data = thread;
    (*node).data->elapsed = 0;
    (*node).next = NULL;
    return node;
}

// circular queue of thread
void push(tcb * thread){

    pthread_node * ptr = head;
    
    pthread_node * node = newNode(thread);
    if (head == NULL){
        head = node;
    } else{

        while(ptr->next != NULL &&
              ptr->next->data->elapsed < node->data->elapsed){
            ptr = ptr->next;
        }
        node->next = ptr->next;
        ptr->next = node;
    }
    
}

        
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {

    printf("creating thread");
    thread_count++;
    
    tcb* thread_new = (tcb*) malloc(sizeof(tcb));
    

    thread_new->id = thread_count;
    thread_new->status = SCHEDULED;
    
    // Thread Context, make pointer to addess of tcb context

    thread_new->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(thread_new->context);
    
    (*thread_new).context.uc_stack.ss_sp = malloc(STACKSIZE);
    (*thread_new).context.uc_ss_size = STACKSIZE;
    (*thread_new).context.uc_link = NULL;    

    makecontext( (*thread_new).context, (void (*)()) function, 1, arg);
    
    add_thread_queue(thread_new);
    *thread = 
    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

    printf("exiting thread");
    free(value_ptr);
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
    return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// YOUR CODE HERE

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}


// Feel free to add any other functions you need

// YOUR CODE HERE
