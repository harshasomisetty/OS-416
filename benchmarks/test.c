#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
//#include "../mypthread.h"


/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void *myThreadFun(void *vargp)
{
    sleep(1);
    printf("Printing GeeksQuiz from Thread \n");
    return NULL;
}
   
int main()
{
    pthread_t thread_id;
    printf("Before Thread\n");
    pthread_create(&thread_id, NULL, myThreadFun, NULL);
    printf("thread id %d\n", thread_id);
    pthread_join(thread_id, NULL);
    printf("After Thread\n");
    exit(0);
}


