
// File:	mypthread.c

// List all group member's name: Harsha Somisetty, Hugo De Moraes
// username of iLab: hs884
// iLab Server: ilab3.cs.rutgers.edu

#include "mypthread.h"


// INITAILIZE ALL YOUR VARIABLES HERE
int thread_count = 0;

pthread_node * head = NULL;
pthread_node * currentN = NULL;

ucontext_t * schedulerC = NULL;

pthread_node* newNode(tcb* thread){
    pthread_node *node = (struct pthread_node*) malloc(sizeof(pthread_node)); // node or listitem
    (*node).data = thread;
    (*node).data->elapsed = 0;
    (*node).next = NULL;
    return node;
}

// circular queue of thread
void push(pthread_node * node){
    if (node != NULL){
        pthread_node * ptr = head;
    
        if (head == NULL){
            head = node;
        } else{

            while(ptr->next != NULL && ptr->data->elapsed <= node->data->elapsed){
                ptr = ptr->next;
            }
        
            if (ptr == head) { // insert node at head of queue
                node->next = ptr->next;
                ptr->next = node;
            }else{
                node->next = ptr->next;
                ptr->next = node;
            }
        }
    }
}

pthread_node* pop(){
    push(currentN);
    pthread_node * next = head;
    head = head->next;
    return next;
}

int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {

    if (schedulerC == NULL){
        makeScheduler();
    }
    thread_count++;
    printf("creating thread %d\n", thread_count);
    
    tcb* thread_new = (tcb*) malloc(sizeof(tcb)); // data
    

    thread_new->id = thread_count;
    thread_new->status = SCHEDULED;
    
    // Thread Context, make pointer to addess of tcb context

    thread_new->context = (ucontext_t*) malloc(sizeof(ucontext_t)); // context

    getcontext(thread_new->context);
    
    thread_new->context->uc_stack.ss_sp = malloc(STACKSIZE); // stack
    thread_new->context->uc_stack.ss_size = STACKSIZE;
    thread_new->context->uc_link = schedulerC;    

    makecontext( thread_new->context, (void (*)()) function, 1, arg);
    
    push(newNode(thread_new));
    *thread = thread_new->id;
    currentN = pop();
    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context
    swapcontext(currentN->data->context, schedulerC);
	// YOUR CODE HERE
    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread


    //    setcontext(schedulerC);
    printf("exiting?");

    
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread
    //dealloc node, but
    printf("trying to join\n");
    setcontext(currentN->data->context);
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

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)
    printf("scheduling");


    setcontext(currentN->data->context);
    
}


static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

    sched_stcf();

}


void makeScheduler(){
    schedulerC = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(schedulerC);
    schedulerC->uc_stack.ss_sp = malloc(STACKSIZE);
    schedulerC->uc_stack.ss_size = STACKSIZE;
    printf("shceduling\n");
    makecontext(schedulerC, &schedule, 0);
    
}


// Feel free to add any other functions you need

// YOUR CODE HERE
