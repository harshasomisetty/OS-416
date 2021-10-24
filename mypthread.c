// File:	mypthread.c
// List all group member's name: Harsha Somisetty
// username of iLab: hs884
// iLab Server: ilab3.cs.rutgers.edu

#include "mypthread.h"

int thread_count = 0; // start id at 0,  main thread id 0, other at 1

static pthread_node * head = NULL; // Queue of all threads
pthread_node * currentN = NULL; // Currently running thread

static ucontext_t * schedulerC = NULL; // context of scheduler thred
static tcb * mainT;

static struct sigaction sa; // action handler
static struct itimerval timer; // timer to switch back to scheduler
static struct itimerval deletetimer; // timer to switch back to scheduler

static mypthread_mutex_t * mutex = NULL;



// predeclaring functions
static void schedule();
pthread_node* newNode(tcb* thread);
pthread_node* pop();
void push(pthread_node * node);

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
    if (schedulerC == NULL){
        makeScheduler();
     //   sleep(.1);
    }

    thread_count++;
    printf("creating thread %d\n", thread_count);
    
    tcb* thread_new = (tcb*) malloc(sizeof(tcb)); // data
    

    thread_new->id = thread_count;
    thread_new->status = SCHEDULED;
    thread_new->elapsed = 1;

    thread_new->context =  (ucontext_t*) malloc(sizeof(ucontext_t)); // context

    getcontext(thread_new->context);
    
    thread_new->context->uc_stack.ss_sp = malloc(STACKSIZE); // stack
    thread_new->context->uc_stack.ss_size = STACKSIZE;
    thread_new->context->uc_link = schedulerC;    

    makecontext( thread_new->context, (void (*)()) function, 1, arg);

       
    push(newNode(thread_new)); // add thread as a node to queue 
    *thread = thread_new->id; // returning thread id basically

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {
    
    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    swapcontext(currentN->data->context, schedulerC);

    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    // Deallocated any dynamic memory created when starting  thread

    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    setcontext(schedulerC);
    printf("exiting?");
    
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    
    return 0;
};





/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {

    mutex->cur_thread = NULL;
    mutex->b_threads = NULL;
    mutex->destroyed = 0;

	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
    
    if (mutex->destroyed == 1){
        printf("cant lock destroyed mutex");
        exit(0);
    }
    if (mutex->cur_thread == currentN){
        printf("already locked by this thread");
        exit(0);
    }
    if (mutex->cur_thread != NULL){
        swapcontext(currentN->data->context, schedulerC);
    }
    mutex->cur_thread = currentN;

    return 0;
        

};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
    if (mutex->destroyed == 1){
        printf("cant unlock destroyed mutex");
    }
    if (mutex->cur_thread != currentN){
        printf("cannt unlokc mutex lokced by other thread");
    }

    else{
        mutex->cur_thread = NULL;
        
        //while(mutex->b_threads != NULL){
    }

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

    if (mutex->cur_thread != NULL){
        printf("cannot destroy locked mutex\n");
        exit(0);
    }
    else if (mutex->destroyed == 1){
        printf("cannot destroy destroyed mutex\n");
        exit(0);
    }
    else{
        mutex->destroyed = 1;
    }
	return 0;
};



/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
    currentN->data->elapsed++; //increment time elapsed
    
    if (currentN->data->id == 0){
        currentN->data->elapsed = 20;
    }
    int count = 0;
    pthread_node * ptr = head;
    printf("nodes: (%d %d), " ,currentN->data->id, currentN->data->elapsed);
    while (ptr){
        count++;
        printf("(%d %d), ", ptr->data->id, ptr->data->elapsed);
        ptr = ptr->next;
    }
    printf("\nnum of nodes, %d\n", count);


    printf("thread %d has elapsed %d\n", currentN->data->id, currentN->data->elapsed);

    //printf("current: %x\n", currentN);
    push(currentN); // if ended, need to exit
    //printf("head: %p\n", head);
    currentN = pop(); // gets the next best thread to execute

    ptr = head;
    printf("after: (%d %d), " ,currentN->data->id, currentN->data->elapsed);
    while (ptr){
        printf("(%d %d), ", ptr->data->id, ptr->data->elapsed);
        ptr = ptr->next;
    }
 
    setitimer(ITIMER_REAL, &timer, NULL);    
    setcontext(currentN->data->context);
    
}

static void schedule() {
    // context switch here every timer interrupt

    printf("got into schedulerrrrr \n");
    sched_stcf();

}

static void switchScheduler(int signo, siginfo_t *info, void *context){

    if (swapcontext(currentN->data->context, schedulerC) == -1)
        handle_error("Failed Scheduler Context Switch");
}

void makeScheduler(){
    printf("making initial scheduler\n");
    schedulerC = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(schedulerC) == -1)
        handle_error("Schedule Context Creation Error");
        
    schedulerC->uc_stack.ss_sp = malloc(STACKSIZE);
    schedulerC->uc_stack.ss_size = STACKSIZE;

    makecontext(schedulerC, &schedule, 0);

    // add main thread to queue so rest of the main can execute

    mainT = (tcb *) malloc(sizeof(tcb));
    mainT->id = 0;
    mainT->status = SCHEDULED;
    mainT->elapsed = 0;
    mainT->context = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext(mainT->context); //store current thread (main) context into this thread block
    
    currentN = newNode(mainT);
    
    // initing timer, every 25ms switches back to scheduler 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = sa.sa_flags | SA_SIGINFO;
    sa.sa_handler = NULL;
    sa.sa_sigaction = switchScheduler;

    sigaction(SIGALRM, &sa, NULL);
    
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 5000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;


    deletetimer.it_value.tv_sec = 0;
    deletetimer.it_value.tv_usec = 0;
    deletetimer.it_interval.tv_sec = 0;
    deletetimer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
    sleep(1);
}



























pthread_node* newNode(tcb* thread){
    pthread_node *node = (struct pthread_node*) malloc(sizeof(pthread_node)); // node or listitem
    (*node).data = thread;
    (*node).data->elapsed = 0;
    (*node).next = NULL;
    return node;
}

// circular queue of thread
void push(pthread_node * node){
    printf("pushing\n");
    if (node != NULL){
        pthread_node * ptr = head;
    
        if (head == NULL){
            head = node;
        } else{

            while(ptr->next != NULL && node->data->elapsed > ptr->next->data->elapsed ){
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
    pthread_node * next = head;
    head = head->next;
    return next;
}
