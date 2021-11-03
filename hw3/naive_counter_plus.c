#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNTER_VALUE (1UL << 24)

int counter = 0;
pthread_mutex_t lock;

void* counterFunction(void* arg) {
    int i = 0;
    for (i = 0; i < COUNTER_VALUE; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, char** argv) {
    
    if (argc < 2) {
        printf("Error: empty thread count parameter\n");
        abort();
    }
    
    int threadCount = atoi(argv[1]), i = 0;
    pthread_t* threads = (pthread_t*) malloc(threadCount * sizeof(pthread_t));
    pthread_mutex_init(&lock, NULL);
    
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    
    for (i = 0; i < threadCount; i++)
        pthread_create(&(threads[i]), NULL, counterFunction, NULL);
    
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
