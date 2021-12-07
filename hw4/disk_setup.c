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

bitmap_t inodeBitmap, dataBitmap;
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


int abstractIndex(int realIndex) {
	return realIndex + 1;
}

int realIndex(int abstractIndex){
	return abstractIndex - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// BITMAP OPERATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
* 
*	get_avail_ino searches the inodeBitmap for the first bit that is set to zero in the map.
*	The index of the bit is the real inode number. However, we shift it up by one and return it
*	as an abstract inode number so that a 0 as a inode number can mean null. 
*
*/

int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	bio_read(INODE_MAP_INDEX, inodeBitmap);
	
	// Step 2: Traverse inode bitmap to find an available slot
	int i = 0;
	for (i = 0; i < BLOCK_SIZE * INODE_BITMAP_SIZE; i++) //we start at 1 because we want 0 to mean null
		if (get_bitmap(inodeBitmap, i) == 0) {
			set_bitmap(inodeBitmap, i);
			break;
		}

	// Step 3: Update inode bitmap and write to disk 
	bio_write(INODE_MAP_INDEX, inodeBitmap);
	
	return abstractIndex(i); 
}

/* 
* 
*	get_avail_blkno searches the dataBitmap for the first bit that is set to zero in the map.
*	The index of the bit is the real data block number. However, we shift it up by one and return it
*	as an abstract data block number so that a 0 as a data block number can mean null. 
*
*/


int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	bio_read(DATA_MAP_INDEX, dataBitmap);

	// Step 2: Traverse data block bitmap to find an available slot
	int i = 0;
	for (i = 0; i < BLOCK_SIZE * DATA_BITMAP_SIZE; i++) //we start at 1 because we want 0 to mean null. 
		if (get_bitmap(dataBitmap, i) == 0) {
			set_bitmap(dataBitmap, i);
			break;
		}
	
	// Step 3: Update data block bitmap and write to disk 
	bio_write(DATA_MAP_INDEX, dataBitmap);

	return abstractIndex(i); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// INODE OPERATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
* 
*	readi takes in an abstract inode number and a structure to fill with information. It 
*	first converts the abstract inode number into a real inode number before checking the 
*	disk at the proper block number and offset for an inode. It then copies the inode from the
*	disk and into the structure. 
*
*/

int readi(uint16_t ino, struct inode *inode) {

	// Step 1: Get the inode's on-disk block number
	int inode_block_num = INODE_BLOCK_RESERVE_INDEX + realIndex(ino) / 16;

	// Step 2: Get offset of the inode in the inode on-disk block
	int inode_block_num_offset = inode_block_num + ((realIndex(ino) % 16) * INODE_SIZE);

	// Step 3: Read the block from disk and then copy into inode structure
	char* inode_block_ptr = malloc(BLOCK_SIZE);
	bio_read(inode_block_num, inode_block_ptr);
	memcpy(inode, inode_block_ptr + inode_block_num_offset, INODE_SIZE);
	free(inode_block_ptr);

	return 0;
}

/* 
* 
*	writei takes in an abstract inode number and an inode structure to write. It first converts
*	the abstract inode number into a real inode number before finding the proper block number
*	and offset of the inode in memory. We load up the block from disk before copying the inode
*	into that block and writing it the disk.  
*
*/

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int inode_block_num = INODE_BLOCK_RESERVE_INDEX + realIndex(ino) / 16;

	// Step 2: Get the offset in the block where this inode resides on disk
	int inode_block_num_offset = inode_block_num + ((realIndex(ino) % 16) * INODE_SIZE);

	// Step 3: Write inode to disk 
	char* inode_block_ptr = malloc(BLOCK_SIZE);
	bio_read(inode_block_num, inode_block_ptr);
	memcpy(inode_block_ptr + inode_block_num_offset, inode, INODE_SIZE);
	bio_write(inode_block_num, inode_block_ptr);
	free(inode_block_ptr);
	
	return 0;
}




int main(int argc, char **argv){
    inodeBitmap = (bitmap_t) malloc(BLOCK_SIZE * INODE_BITMAP_SIZE);
    dataBitmap = (bitmap_t) malloc(BLOCK_SIZE * DATA_BITMAP_SIZE);
    int i = 0;
    for (i = 0; i < BLOCK_SIZE*DATA_BITMAP_SIZE; i++){
        inodeBitmap[i] = 0;
        dataBitmap[i] = 0;
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
    bio_write(INODE_MAP_INDEX, inodeBitmap);
    bio_write(DATA_MAP_INDEX, dataBitmap);

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

	for (i = 0; i < 3; i++)
		printf("Reloaded Block #: %d\n", reloadedNode->direct_ptr[i]);


    free(super);
    free(inodeBitmap);
    free(dataBitmap);
}

