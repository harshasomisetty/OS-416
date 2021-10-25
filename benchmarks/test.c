#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <pthread.h>
#include "../mypthread.h"


pthread_t tid[2];
int counter;
pthread_mutex_t lock;


void *myThreadFun(void *vargp)
{
    sleep(2);
    printf("myThreadFun run \n");
    return NULL;
}

void *myThreadFun1(void *vargp)
{

    printf("exitied quick\n");
    pthread_exit(NULL);

}

void test1(){

    pthread_t thread_id;
    printf("Before1 Thread\n");
    pthread_create(&thread_id, NULL, myThreadFun, NULL);
    printf("sdf %x\n", thread_id);
    //    pthread_join(thread_id, NULL);
    pthread_join(thread_id, NULL);
}


/* void exit_test(){ */
/*     pthread_t thread1, thread2; */
/*     pthread_create(&thread1, NULL, myThreadFun, NULL); */
/*     pthread_create(&thread2, NULL, myThreadFun1, NULL); */


/*     pthread_join(thread2, NULL); */
/*     pthread_join(thread1, NULL); */
/*     printf("done main\n"); */
/* } */

void long_sleep(){
    printf("vishnu\n");
    sleep(10);
    printf("v done\n");
    pthread_exit(NULL);
}
void printing_thread(int num){
    for(int i = 0; i < 3*num; i++){
        printf("in thread %d, count %d\n", num, i);
        //printf("second %d\n", num);
    }
    pthread_exit(NULL);

}
void two_threads(){
    
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, printing_thread, 1);

    printf("thread 1\n");

    pthread_join(thread1, NULL);
    
    pthread_create(&thread2, NULL, printing_thread, 1);
    printf("main sleeping\n");
//    sleep(13);
    printf("main done\n");
}


void multiple_threads(int thread_num){
    
    pthread_t * thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
    
    for (int i = 0; i < thread_num; i++){
        pthread_create(&thread[i], NULL, &printing_thread, i);
    }

    pthread_t lll;
    pthread_create(&lll, NULL, &long_sleep, NULL);

     for (int i = 0; i < thread_num; ++i){ 
         printf("joining %d\n", i);
         pthread_join(thread[i], NULL); 
     } 
    pthread_join(lll, NULL);

}

void* trythis(void* arg)
{
    pthread_mutex_lock(&lock);
          
    unsigned long i = 0;
    counter += 1;
    printf("\n Job %d has started\n", counter);
                      
    for (i = 0; i < (0xFFFFFFFF); i++);
                  
    printf("\n Job %d has finished\n", counter);
                              
    pthread_mutex_unlock(&lock);
                                  
    return NULL;
}

void mutex_test(){
    int i = 0;
    int error;
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    while (i < 2) {
        error = pthread_create(&(tid[i]),
                               NULL,
                              &trythis, NULL);
        if (error != 0)
            printf("\nThread can't be created :[%s]",
                   strerror(error));
        i++;
    }
      
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    
    pthread_mutex_destroy(&lock);
}
int main()
{
    //    test1();
    //    exit_test();
    
//    multiple_threads(3);
    mutex_test();
    //two_threads();
    exit(0);
}


