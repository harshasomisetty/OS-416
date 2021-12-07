#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>


#include "block.h"

  
typedef unsigned char* bitmap_t;

#define DISKFILE "benchmark/DISKFILE"
#define BLOCK_SIZE 4096
#define INODE_BITMAP_SIZE 1
#define DATA_BITMAP_SIZE 1
#define INODE_BLOCK_RESERVE 482
#define INODE_BLOCK_RESERVE_INDEX 3
#define INODE_SIZE 256
#define DATA_BLOCK_RESERVE 7707
#define DATA_BLOCK_RESERVE_INDEX 485
#define SUPERBLOCK_INDEX 0
#define INODE_MAP_INDEX 1
#define DATA_MAP_INDEX 2
#define MAGIC_NUM 0x5C3A

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

bitmap_t inodemap, datamap;
struct superblock * super; 

char buf[BLOCK_SIZE];

void set_bitmap(bitmap_t b, int i) {
    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
    b[i / 8] &= ~(1 << (i & 7));
}

uint8_t get_bitmap(bitmap_t b, int i) {
    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}


int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	bio_read(INODE_MAP_INDEX, inodemap);
	
	// Step 2: Traverse inode bitmap to find an available slot
	int i = 0;
	for (i = 0; i < BLOCK_SIZE * INODE_BITMAP_SIZE; i++) 
		if (get_bitmap(inodemap, i) == 0) {
			set_bitmap(inodemap, i);
			break;
		}

	// Step 3: Update inode bitmap and write to disk 
	bio_write(INODE_MAP_INDEX, inodemap);
	
	return i;
}

int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	bio_read(DATA_MAP_INDEX, datamap);

	// Step 2: Traverse data block bitmap to find an available slot
	int i = 0;
	for (i = 0; i < BLOCK_SIZE * DATA_BITMAP_SIZE; i++) 
		if (get_bitmap(datamap, i) == 0) {
			set_bitmap(datamap, i);
			break;
		}
	
	// Step 3: Update data block bitmap and write to disk 
	bio_write(DATA_MAP_INDEX, datamap);

	return i;
}

int readi(uint16_t ino, struct inode *inode) {

	// Step 1: Get the inode's on-disk block number
	int inode_block_num = INODE_BLOCK_RESERVE_INDEX + ino / 16;

	// Step 2: Get offset of the inode in the inode on-disk block
	int inode_block_num_offset = inode_block_num + ((ino % 16) * INODE_SIZE);

	// Step 3: Read the block from disk and then copy into inode structure
	char* inode_block_ptr = malloc(BLOCK_SIZE);
	bio_read(inode_block_num, inode_block_ptr);
	memcpy(inode, inode_block_ptr + inode_block_num_offset, INODE_SIZE);
	free(inode_block_ptr);

	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int inode_block_num = INODE_BLOCK_RESERVE_INDEX + ino / 16;

	// Step 2: Get the offset in the block where this inode resides on disk
	int inode_block_num_offset = inode_block_num + ((ino % 16) * INODE_SIZE);

	// Step 3: Write inode to disk 
	char* inode_block_ptr = malloc(BLOCK_SIZE);
	bio_read(inode_block_num, inode_block_ptr);
	memcpy(inode_block_ptr + inode_block_num_offset, inode, INODE_SIZE);
	bio_write(inode_block_num, inode_block_ptr);
	free(inode_block_ptr);
	
	return 0;
}



int main(int argc, char **argv){
    inodemap = (bitmap_t) malloc(BLOCK_SIZE * INODE_BITMAP_SIZE);
    datamap = (bitmap_t) malloc(BLOCK_SIZE * DATA_BITMAP_SIZE);
    int i = 0;
    for (i = 0; i < BLOCK_SIZE*DATA_BITMAP_SIZE; i++){
        inodemap[i] = 0;
        datamap[i] = 0;
    }

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
    bio_write(INODE_MAP_INDEX, inodemap);
    bio_write(DATA_MAP_INDEX, datamap);

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


    free(super);
    free(inodemap);
    free(datamap);
}

