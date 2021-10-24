#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <pthread.h>
#include "../mypthread.h"


int thread_num = 10;

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

void printing_thread(int num){
    for(int i = 0; i < 5; i++){
        printf("in thread %d, count %d\n", num, i);
        sleep(1);
    }

}

void multiple_threads(int thread_num){
    
    pthread_t * thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
    
    for (int i = 0; i < thread_num; i++){
        pthread_create(&thread[i], NULL, &printing_thread, i);
    }


    /* for (int i = 0; i < thread_num; ++i){ */
    /*     pthread_join(thread[i], NULL); */
    /* } */

    sleep(30);
}

void two_threads(){
    
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, printing_thread, 1);

    printf("thread 2\n");

    pthread_create(&thread2, NULL, printing_thread, 2);
    
    sleep(20);
}


int main()
{
    //    test1();
    //    exit_test();
    
    //multiple_threads(5);
    two_threads();
    exit(0);
}


