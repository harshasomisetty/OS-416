#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "block.h"
#define BLOCKSIZE 4096
  
typedef unsigned char* bitmap_t;
#define DISKFILE "benchmark/DISKFILE"
  
bitmap_t bitmap;
char buf[BLOCKSIZE];
int main(int argc, char **argv){


    /* bitmap = malloc(BLOCKSIZE); */
    printf("hi\n");
    /* printf("bit %d\n", get_bitmap(bitmap, 2)); */
    printf("disk %d\n", BLOCKSIZE);
    dev_init(DISKFILE);

    /* bio_write(1, "hi"); */

    bio_read(1, buf);
    printf("buffer read %s\n", buf);
}

