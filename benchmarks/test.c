#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <pthread.h>
#include "../mypthread.h"


/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
int thread_num = 10;
void *myThreadFun(void *vargp)
{
    sleep(1);
    printf("Printing GeeksQuiz from Thread \n");
    return NULL;
}
void test1(){

    pthread_t thread_id;
    printf("Before1 Thread\n");
    pthread_create(&thread_id, NULL, myThreadFun, NULL);
    printf("sdf %x\n", thread_id);
    //    pthread_join(thread_id, NULL);
    printf("After Thread\n");
}

void test2(){
    pthread_t * thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
    
    for (int i = 0; i < thread_num; i++){
        pthread_create(&thread[i], NULL, &myThreadFun, NULL);
    }
    printQueue();
}
int main()
{

    test2();

    exit(0);
}


