// File:	mypthread.c
// List all group member's name: Harsha Somisetty
// username of iLab: hs884
// iLab Server: ilab3.cs.rutgers.edu

#include "mypthread.h"

int thread_count = 0; // start id at 0,  main thread id 0, other at 1

static pthread_node * t_queue = NULL; // Queue of all threads
static pthread_node * t_finished = NULL;
pthread_node * currentN = NULL; // Currently running thread
static int lastAction = 0;

static ucontext_t * schedulerC = NULL; // context of scheduler thred
static tcb * mainT;

static struct sigaction sa; // action handler
static struct itimerval timer; // timer to switch back to scheduler
static struct itimerval deletetimer; // timer to switch back to scheduler

static mypthread_mutex_t * mutex = NULL;


// predeclaring functions
static void schedule();
pthread_node* newNode(tcb* thread);
pthread_node* search();
pthread_node* pop(int tid);
void print_queue();
void push(pthread_node * node);
pthread_node* search_finished(mypthread_t id);

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
    if (schedulerC == NULL){
        makeScheduler();
    }

    thread_count++;
    printf("creating thread %d\n", thread_count);
    
    tcb* thread_new = (tcb*) malloc(sizeof(tcb)); // data
    

    thread_new->id = thread_count;
    thread_new->status = READY;
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
    currentN->data->status = FINISHED;

    if (value_ptr){
        currentN->data->value_ptr = value_ptr;
    }

    
    //add to finsihed queue
    pthread_node * exited = currentN;
    exited->next = t_finished;
    t_finished = exited;
//    currentN = (pthread_node *) NULL;

    swapcontext(exited->data->context, schedulerC);
  
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    //printf("in join func\n");
    pthread_node * checking = search_finished(thread);
        

    if (!checking){
        currentN->data->elapsed = currentN->data->elapsed + 2;
        mypthread_yield();
    }else{
        if (checking->data->value_ptr){
            *value_ptr = checking->data->value_ptr;
        }
        //clean this
    }

        return 0;
};


/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {


    mutex->cur_thread = NULL;
    mutex->block = NULL;
    mutex->destroyed = 0;

	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
    
    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    if (mutex->destroyed == 1){
        printf("cant lock destroyed mutex\n");
        exit(0);
    }
    if (mutex->cur_thread == currentN){
        printf("already locked by this thread");
        exit(0);
    }
    if (mutex->cur_thread != NULL){

        currentN->next = mutex->block;
        mutex->block = currentN;
        currentN->data->status = BLOCKED;
        swapcontext(currentN->data->context, schedulerC);
    }
    mutex->cur_thread = currentN;

    return 0;
        

};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {

    setitimer(ITIMER_REAL, &deletetimer, NULL); 
    if (mutex->destroyed == 1){
        printf("cant unlock destroyed mutex");
    }
    if (mutex->cur_thread != currentN){
        printf("cannt unlock mutex locked by other thread\n");
    }

    else{

            
        pthread_node * temp = mutex->block;
        while(temp != NULL){
            temp->data->status = READY;
            push(temp);
            temp = temp->next;
        }
        mutex->cur_thread = NULL;


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


void print_queue(){
    int count = 0;
    pthread_node * ptr = t_queue;
    printf("nodes: (%d %d), " ,currentN->data->id, currentN->data->elapsed);
    while (ptr){
        count++;
        printf("(%d %d), ", ptr->data->id, ptr->data->elapsed);
        ptr = ptr->next;
    }
    printf(". Length: %d\n", count);
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {

    currentN->data->elapsed++; //increment time elapsed
    
    /*if (currentN->data->id == 0){
        currentN->data->elapsed = 20;
    }*/
    
    pthread_node * first = search(1);
    if (first){
        //printf("first: %d\n", first->data->id);
    }
    // printf("thread %d has elapsed %d\n", currentN->data->id, currentN->data->elapsed);

    //print_queue(); // printing queue before selecting new

    if (currentN->data->status != FINISHED || currentN->data->status != BLOCKED){

        push(currentN); // if ended, need to exit
    }
    currentN = pop(-1); // gets the next best thread to execute

    // printf("after pushing");
    // print_queue(); // printing queue after selecting new

    setitimer(ITIMER_REAL, &timer, NULL);    
    setcontext(currentN->data->context);
    
}

static void schedule() {
    // context switch here every timer interrupt

    // printf("in scheduler \n");
    sched_stcf();

}

static void switchScheduler(int signo, siginfo_t *info, void *context){

    if (swapcontext(currentN->data->context, schedulerC) == -1)
        handle_error("Failed Scheduler Context Switch");
}

void makeScheduler(){
    // printf("making initial scheduler\n");
    schedulerC = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(schedulerC) == -1)
        handle_error("Schedule Context Creation Error");
        
    schedulerC->uc_stack.ss_sp = malloc(STACKSIZE);
    schedulerC->uc_stack.ss_size = STACKSIZE;

    makecontext(schedulerC, &schedule, 0);

    // add main thread to queue so rest of the main can execute

    mainT = (tcb *) malloc(sizeof(tcb));
    mainT->id = 0;
    mainT->status = READY;
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
    if (node != NULL){
        pthread_node * ptr = t_queue;
    
        if (t_queue == NULL){
            t_queue = node;
        } else{

            while(ptr->next != NULL && node->data->elapsed > ptr->next->data->elapsed ){
                ptr = ptr->next;
            }
        
            if (ptr == t_queue) { // insert node at t_queue of queue
                node->next = ptr->next;
                ptr->next = node;
            }else{
                node->next = ptr->next;
                ptr->next = node;
            }
        }
    }
}

pthread_node* pop(int tid){
    if (tid == -1){
        pthread_node * next = t_queue;
        t_queue = t_queue->next;
        return next;
    }else{
        pthread_node * ptr = t_queue;
        while (ptr != NULL
                && ptr->next != NULL
                && ptr->next->data->id != tid
                ) {
            ptr=ptr->next;
        }
        pthread_node * target = ptr->next;
        ptr->next = ptr->next->next;
        return target;
    }
       
}

pthread_node* search(int id){

    pthread_node * ptr = t_queue;
    if (ptr != NULL
            && ptr->data->id != id
            ) {
        ptr=ptr->next;
    }
    return ptr;
}

pthread_node* search_finished(mypthread_t id){

    pthread_node * ptr = t_finished;
    int tid = (int) id;
    if (ptr == NULL){
        return NULL;
    }
    while (ptr != NULL
            && ptr->data->id != tid
            ) {
        ptr=ptr->next;
    }
    if (ptr->data->id == tid){
        return ptr;
    }else{
        return NULL;
    }
}



