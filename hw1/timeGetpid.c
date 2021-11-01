#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

// Place any necessary global variables here

int main(int argc, char *argv[]){

    struct timeval tv;
    struct timeval tv2;
    
    gettimeofday(&tv, NULL);

    int syscall_num = 100000;
    int b = 0;
    for (int i = 0; i < syscall_num; i++){
      b = getpid();
    }
    
    
    gettimeofday(&tv2, NULL);
    double time = tv2.tv_usec - tv.tv_usec;
    printf("Syscalls performed: %d\n", syscall_num);
    printf("Total Elapsed Time: %f microseconds\n", time);
    printf("Average Time Per Syscall: %f microseconds", (time/syscall_num)); 
    return(0);  
}
