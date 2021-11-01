#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

// Place any necessary global variables here

int main(int argc, char *argv[]){

    struct timeval tv;
    struct timeval tv2;
    
    gettimeofday(&tv, NULL);

    int syscall_num = 128000;
    int b = 0;

    /* To generate file of 512 mb */
    /* dd if=/dev/zero of=bigfile bs=512000000 count=1 */
    
    int fd = open("bigfile", O_RDONLY);
    char data[4000];
    printf("%d\n", fd);
    for (int i = 0; i < syscall_num; i++){
      
      b = read(fd, data, 4000);

    }

    int close(int fd);
    
    gettimeofday(&tv2, NULL);
    double time = tv2.tv_usec - tv.tv_usec;
    printf("Syscalls performed: %d\n", syscall_num);
    printf("Total Elapsed Time: %f microseconds\n", time);
    printf("Average Time Per Syscall: %f microseconds\n", (double) (time/syscall_num)); 
    return(0);  
}

