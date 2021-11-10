#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNTER_VALUE (1UL << 24)
#define THRESH 50

int globalCounter = 0;
pthread_mutex_t mut;

void* counterFunction(void* arg) {
	int i = 0, counter = 0;
	for (i = 0; i < COUNTER_VALUE; i++) {
		counter++;
		if (counter > THRESH) {
			pthread_mutex_lock(&mut);
			globalCounter += counter;
			counter = 0;
			pthread_mutex_unlock(&mut);
		}
	}
	pthread_mutex_lock(&mut);
	globalCounter += counter;
	pthread_mutex_unlock(&mut);
}

int main(int argc, char** argv) {

	if (argc < 2) {
		printf("Error: empty thread count parameter\n");
		abort();
	}

	int threadCount = atoi(argv[1]), i = 0;
	pthread_t* threads = (pthread_t*) malloc(threadCount * sizeof(pthread_t));
	pthread_mutex_init(&mut, NULL);

	struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
	
	for (i = 0; i < threadCount; i++) 
		pthread_create(&(threads[i]), NULL, counterFunction, NULL);
	
	for (i = 0; i < threadCount; i++)
		pthread_join(threads[i], NULL);

	clock_gettime(CLOCK_REALTIME, &end);
    
	long int supposedAnswer = threadCount * COUNTER_VALUE;

	printf(
		"Counter finish in: %lu ms\n", 
		(end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000
	);
	printf("The value of the counter should be: %ld\n", supposedAnswer);
	printf("The value of the counter is: %d\n", globalCounter);
	printf("The difference is %ld\n", supposedAnswer - globalCounter);

	pthread_mutex_destroy(&mut);
	free(threads);
	return 0;
}
