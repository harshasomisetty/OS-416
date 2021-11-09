#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define UNLOCKED 0
#define LOCKED 1

#define COUNTER_VALUE (1UL << 24)

int counter = 0;
int lock = UNLOCKED;

int test_and_set(int* lock_ptr){
    int prev = *lock_ptr;
    *lock_ptr = LOCKED;
    return prev;
}

void* critical_section(void* arg){
    while(test_and_set(&lock) == 1);
    counter = counter + 1;
    lock = UNLOCKED;
}

int main(int argc, char** argv) {
    
    if (argc < 2) {
        printf("Error: empty thread count parameter\n");
        abort();
    }
    
    int threadCount = atoi(argv[1]), i = 0;
    pthread_t* threads = (pthread_t*) malloc(threadCount * sizeof(pthread_t));
    
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    
    for (i = 0; i < threadCount; i++)
        pthread_create(&(threads[i]), NULL, critical_section, NULL);
    
    for (i = 0; i < threadCount; i++)
        pthread_join(threads[i], NULL);
    
    clock_gettime(CLOCK_REALTIME, &end);
    
    printf(
           "Counter finish in: %lu ms\n",
           (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000
           );
    printf("The value of the counter should be: %ld\n", threadCount * COUNTER_VALUE);
    printf("The value of the counter is: %d\n", counter);
    
    pthread_mutex_destroy(&lock);
    free(threads);
    return 0;

}
