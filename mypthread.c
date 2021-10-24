// File:	mypthread.c
// List all group member's name: Harsha Somisetty
// username of iLab: hs884
// iLab Server: ilab3.cs.rutgers.edu

#include "mypthread.h"

int thread_count = 0; // start id at 0,  main thread id 0, other at 1

pthread_node * head = NULL; // Queue of all threads
pthread_node * currentN = NULL; // Currently running thread

static ucontext_t * schedulerC = NULL; // context of scheduler thred
static tcb * mainT;

static struct sigaction sa; // action handler
static struct itimerval timer; // timer to switch back to scheduler

static void schedule();
pthread_node* newNode(tcb* thread);
pthread_node* pop();
void push(pthread_node * node);

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)



int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {

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

    if (schedulerC == NULL){
        makeScheduler();
    }
        
    push(newNode(thread_new)); // add thread as a node to queue 
    *thread = thread_new->id; // returning thread id basically

    sleep(1);
    printf("done with thread %d\n", thread_count);
    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {
    
    swapcontext(currentN->data->context, schedulerC);

    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    // Deallocated any dynamic memory created when starting  thread

    //    setcontext(schedulerC);
    printf("exiting?");
    
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

    // wait for a specific thread to terminate
    // de-allocate any dynamic memory created by the joining thread
    printf("trying to join\n");
    setcontext(currentN->data->context);
    return 0;
};

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
    printf("scheduling algo\n");
    //    sigaction(SIGALRM, &sa, NULL);

    currentN = pop();

    setcontext(currentN->data->context);
    
}

static void schedule() {
    // context switch here every timer interrupt

    printf("got into schedule");
    //    sched_stcf();

}

static void switchScheduler(int signo, siginfo_t *info, void *context){

    //    schedulerC->uc_mcontext = ((ucontext_t*) context)->uc_mcontext;
    ucontext_t* interrupted = (ucontext_t*) context;

    sigset_t mask;
    int ret = sigprocmask(0, NULL, &mask);
    assert(!ret);
    assert(sigismember(&mask, SIGALRM));

    printf("Handler with interrupt thread state at %p\n",  context);
  

    schedulerC->uc_mcontext = interrupted->uc_mcontext;
   
    //    push(currentN);
    if ( (
   //    swapcontext(currentN->data->context, schedulerC);

          swapcontext(currentN->data->context, schedulerC)
    //       setcontext(schedulerC);
        ) == -1)
        handle_error("context");
    setitimer(ITIMER_REAL, &timer, NULL);    
}

void makeScheduler(){
    schedulerC = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(schedulerC) == -1)
        handle_error("scheduler error");
    
        
    schedulerC->uc_stack.ss_sp = malloc(STACKSIZE);
    schedulerC->uc_stack.ss_size = STACKSIZE;
    printf("making schedule\n");
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
    timer.it_value.tv_usec = 250000;
    
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
    
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
    pthread_node * next = head;
    head = head->next;
    return next;
}
