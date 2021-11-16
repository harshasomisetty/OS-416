#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNTER_VALUE (1UL << 24)

int counter = 0;

void* counterFunction(void* arg) {
	int i = 0;
	for (i = 0; i < COUNTER_VALUE; i++)
		counter++;
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
		pthread_create(&(threads[i]), NULL, counterFunction, NULL);
	
	for (i = 0; i < threadCount; i++)
		pthread_join(threads[i], NULL);
        
	free(threads);
	clock_gettime(CLOCK_REALTIME, &end);

        long total_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
	printf(
		"Counter finish in: %lu ms\n", 
                total_time
	);
	printf("The value of the counter should be: %ld\n", threadCount * COUNTER_VALUE);
	printf("The value of the counter is: %d\n", counter);
        
        FILE *fp = fopen("./results.org", "a");
        fprintf(fp, "| %d | %lu |\n", atoi(argv[1]), total_time);
        fclose(fp);

	return 0;
}


