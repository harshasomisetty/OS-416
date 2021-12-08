#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>


#include "block.h"
#include "global.h"
  
typedef unsigned char* bitmap_t;

struct superblock {
    uint32_t	magic_num;			/* magic number */
    uint16_t	max_inum;			/* maximum inode number */
    uint16_t	max_dnum;			/* maximum data block number */
    uint32_t	i_bitmap_blk;		/* start address of inode bitmap */
    uint32_t	d_bitmap_blk;		/* start address of data block bitmap */
    uint32_t	i_start_blk;		/* start address of inode region */
    uint32_t	d_start_blk;		/* start address of data block region */
};
  
struct inode {
    uint16_t	ino;				/* inode number */
    uint16_t	valid;				/* validity of the inode */
    uint32_t	size;				/* size of the file */
    uint32_t	type;				/* type of the file */
    uint32_t	link;				/* link count */
    int			direct_ptr[16];		/* direct pointer to data block */
    int			indirect_ptr[8];	/* indirect pointer to data block */
    struct stat	vstat;				/* inode stat */
};

bitmap_t inodeBitmap, dataBitmap;
struct superblock * super; 

char buf[BLOCK_SIZE];

/* code to init super block, bitmaps */
void setup_fs(){
    
    int i = 0;
    for (i = 0; i < BLOCK_SIZE*DATA_BITMAP_SIZE; i++){
        inodeBitmap[i] = 0;
        dataBitmap[i] = 0;
    }

    /* inodeBitmap = (bitmap_t) calloc(sizeof(BLOCK_SIZE * INODE_BITMAP_SIZE), 1); */
    /* dataBitmap = (bitmap_t) calloc(sizeof(BLOCK_SIZE * DATA_BITMAP_SIZE), 1); */
    
    super = (struct superblock *) malloc(sizeof(struct superblock));
    super->magic_num = MAGIC_NUM;
    super->max_inum = INODE_BLOCK_RESERVE - 1;
    super->max_dnum = DATA_BLOCK_RESERVE - 1;
    super->i_bitmap_blk = BLOCK_SIZE;
    super->d_bitmap_blk = BLOCK_SIZE * 2;
    super->i_start_blk = BLOCK_SIZE * 3;
    super->d_start_blk = BLOCK_SIZE * 3 + BLOCK_SIZE * INODE_BLOCK_RESERVE;

    dev_init(DISKFILE);
    bio_write(SUPERBLOCK_INDEX, super);
    bio_write(INODE_MAP_INDEX, inodeBitmap);
    bio_write(DATA_MAP_INDEX, dataBitmap);

}

void test_inode_write(){

    int new_inode_num = get_avail_ino();
    printf("Available inode: %d\n", new_inode_num);
    struct inode * node = (struct inode *) malloc(sizeof(struct inode));
    readi(new_inode_num, node);
    
    node->ino = new_inode_num;
    node->valid = 1;
    node->size = 3;
    node->type = 0;
    node->link = 0;
    node->direct_ptr[0] = get_avail_blkno();
    node->direct_ptr[1] = get_avail_blkno();
    node->direct_ptr[2] = get_avail_blkno();

    writei(node->ino, node);

    struct inode * reloadedNode = (struct inode *) malloc(sizeof(struct inode));

    readi(new_inode_num, reloadedNode);
    printf("Reloaded #: %d\n", reloadedNode->ino);
    printf("Reloaded valid: %d\n", reloadedNode->valid);
    printf("Reloaded size: %d\n", reloadedNode->size);

    
}

int test_simple_block(){
    
    int tries = 2;
    char testString[]  = "testing writing i";
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        int blk_index = get_avail_blkno();
        
        bio_write(DATA_BLOCK_RESERVE_INDEX+blk_index, testString );
    }

    /* should print buffer read: testing writing i */
    
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        bio_read(DATA_BLOCK_RESERVE_INDEX+i, buf);
        if (strcmp(testString, buf)){
            perror("incorrect memory stored");
            return -1;
        }
    }

    if (tries == get_avail_blkno(dataBitmap)){
        printf("passed test_simple_block\n");
        return 0;
    } else{
        perror("not reading blocks properly\n");
        return -1;
    }

}

int main(int argc, char **argv){
    
    inodeBitmap = (bitmap_t) malloc(BLOCK_SIZE * INODE_BITMAP_SIZE);
    dataBitmap = (bitmap_t) malloc(BLOCK_SIZE * DATA_BITMAP_SIZE);

    setup_fs();

    /* test_inode_write(); */

    test_simple_block();

    free(super);
    free(inodeBitmap);
    free(dataBitmap);
}

