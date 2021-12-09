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
//#include "global.h"
  


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
#define DIRECT_PTR_ARR_SIZE 16
#define VALID 1
#define INVALID 0

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

struct dirent {
	uint16_t ino;					/* inode number of the directory entry */
	uint16_t valid;					/* validity of the directory entry */
	char name[252];					/* name of the directory entry */
};


bitmap_t inodeBitmap, dataBitmap;
struct superblock * super; 

char buf[BLOCK_SIZE];


int abstractIndex(int realIndex) {
	return realIndex + 1;
}

int realIndex(int abstractIndex){
	return abstractIndex - 1;
}


/*
 * bitmap operations
 */

void set_bitmap(bitmap_t b, int i) {
    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
    b[i / 8] &= ~(1 << (i & 7));
}

uint8_t get_bitmap(bitmap_t b, int i) {
    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// DIRECTORY OPERATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
* 
*	dir_find takes in an abstract inode number, a file name, a name length, and a struct dirent to fill. 
*	It takes the abstract inode number and reads the directory that inode with the readi function. 
*	It then iterates through all of the blocks the inode points to; it takes each abstract data block
*	pointer, checks if its equal to 0 (null) or the data bitmap says that its invalid, before looping through
*	the entire block for dirent structures. If the dirent structure is valid and the filenames
*	match, we have a match. 
*
*/

int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

	// Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode * dir = (struct inode *) malloc(sizeof(struct inode));
	readi(ino, dir);

	// Step 2: Get data block of current directory from inode
	// Step 3: Read directory's data block and check each directory entry.
	//If the name matches, then copy directory entry to dirent structure

	//go through each block of the directory. For each block, look at each dirent entry. 
	//if the dirent entry matches, stop iterating

	int i, j, realBlockIndex;
	char* block_ptr = malloc(BLOCK_SIZE);

	for(int i = 0; i < DIRECT_PTR_ARR_SIZE; i++){
		
		realBlockIndex = realIndex(dir->direct_ptr[i]);
		if (dir->direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0)
			continue;

		bio_read(realBlockIndex, block_ptr);
		
		for(j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++){
			memcpy(dirent, block_ptr + j * sizeof(struct dirent), sizeof(struct dirent));
			if (dirent->valid != INVALID && strcmp(fname, dirent->name) == 0){
				free(dir);
				free(block_ptr);
				return 0;
			}
		}
	}

	free(dir);
	free(block_ptr);
	return 0;
}


/*
*
*	dir_add takes in a directory inode, an abstract file inode number, a file name, and a name length. 
*	First, we need to make sure that the filename is unique in the directory. To do this, we iterate through
*	each of the directory inodes's - ones that don't have an abstract data block number of 0 and that are still 
*	valid according to the data bitmap. We check every valid dirent structure in them to make sure the file 
*	name has not been taken. While we do this, we also mark the first empty block we see as well as the first
*	invalid dirent structure. If we can't find an empty block nor an invalid entry, then the directory is full. 
*	If we find an empty block pointer, we need to allocate a new block to the directory inode and then make a dirent
*	structure inside. Otherwise, we simply edit the invalid dirent structure we find. 
*
*/

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	// Step 2: Check if fname (directory name) is already used in other entries

	int i, j, realBlockIndex, firstEmptyBlockIndex = -1, firstEmptyEntryIndex = -1, firstEmptyEntryBlockIndex = -1;
	char* block_ptr = malloc(BLOCK_SIZE);
	struct dirent * entry = (struct dirent *) malloc(sizeof(struct dirent));
	struct dirent * firstEmptyEntry = NULL;
	for (i = 0; i < DIRECT_PTR_ARR_SIZE; i++) {
		realBlockIndex = realIndex(dir_inode.direct_ptr[i]);
		if (dir_inode.direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0) {
			firstEmptyBlockIndex = firstEmptyBlockIndex == -1 ? i : firstEmptyBlockIndex; 
			continue;
		}
		bio_read(realBlockIndex, block_ptr);

		for (j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {
			memcpy(entry, block_ptr + j*sizeof(struct dirent), sizeof(struct dirent));
			if (entry->valid != INVALID && strcmp(entry->name, fname) == 0) {
				free(block_ptr);
				free(entry);
				return -1;
			}
			if (firstEmptyEntry == NULL && entry->valid == INVALID) {
				firstEmptyEntry = entry;
				firstEmptyEntryIndex = j;
				firstEmptyEntryBlockIndex = i;
			}			
		}
	}

	// Step 3: Add directory entry in dir_inode's data block and write to disk
	// Allocate a new data block for this directory if it does not exist
	// Update directory inode
	// Write directory entry

	if (firstEmptyEntry == NULL && firstEmptyBlockIndex == -1) {
		//directory is full, end here
		free(block_ptr);
		free(entry);
		return -1;
	}
	else if (firstEmptyEntry == NULL) {
		//all blocks are full, need to allocate a new one...
		dir_inode.direct_ptr[firstEmptyBlockIndex] = get_avail_blkno();
		bio_read(realIndex(dir_inode.direct_ptr[firstEmptyBlockIndex]), block_ptr);
		firstEmptyEntry = (struct dirent *) malloc(sizeof(struct dirent));
		firstEmptyEntry->ino = f_ino;
		firstEmptyEntry->valid = 1;
		strncpy(firstEmptyEntry->name, fname, name_len);
		memcpy(block_ptr, firstEmptyEntry, sizeof(struct dirent));
		bio_write(realIndex(dir_inode.direct_ptr[firstEmptyBlockIndex]), block_ptr);
		writei(dir_inode.ino, &dir_inode);
	}
	else {
		bio_read(realIndex(dir_inode.direct_ptr[firstEmptyEntryBlockIndex]), block_ptr);
		firstEmptyEntry->ino = f_ino;
		firstEmptyEntry->valid = 1;
		strncpy(firstEmptyEntry->name, fname, name_len);
		memcpy(block_ptr + firstEmptyEntryIndex * sizeof(struct dirent), firstEmptyEntry, sizeof(struct dirent));
		bio_write(realIndex(dir_inode.direct_ptr[firstEmptyEntryBlockIndex]), block_ptr);
	}

	return 0;
}

/*
*
*	dir_remove takes in a directory inode, a file name, and a length of that name. We iterate through the
*	valid data blocks that the directory inode points to, and then check every dirent structure inside. 
*	If we find a match, we set the entry to invalid and then we write the change back to the disk.
*
*/


int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	// Step 2: Check if fname exist
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	int i, j, realBlockIndex;
	char * block_ptr = malloc(BLOCK_SIZE);
	struct dirent * entry = (struct dirent *) malloc(sizeof(struct dirent));
	for (i = 0; i < DIRECT_PTR_ARR_SIZE; i++) {
		realBlockIndex = realIndex(dir_inode.direct_ptr[i]);
		if (dir_inode.direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0) 
			continue;
		bio_read(realBlockIndex, block_ptr);
		for (j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {
			memcpy(entry, block_ptr + j * sizeof(struct dirent), sizeof(struct dirent));
			if (entry->valid != INVALID && strcmp(fname, entry->name) == 0){
				entry->valid = INVALID;
				memcpy(block_ptr + j * sizeof(struct dirent), entry, sizeof(struct dirent));
				bio_write(realBlockIndex, block_ptr);
				free(block_ptr);
				free(entry);
				return 0;
			}
		}
	}

	free(block_ptr);
	free(entry);
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// NAMEI OPERATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////


int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	char* remPath = NULL, * cur = path;
	int i = 0;
	if (strlen(path) <= 1)
		return -1;
	
	for (i = 1; i < strlen(path); i++) {
		if (path[i] == '/'){
			cur = malloc(i - 1);
			memcpy(cur, path + 1, i - 1);
			remPath = path + i;
			break;
		}
	}
	if (remPath == NULL) {
		cur = malloc(i - 1);
		memcpy(cur, path + 1, i - 1);
	}

	int finalFlag = 0;
	struct dirent * entry = (struct dirent *) malloc(sizeof(struct dirent));
	dir_find(ino, cur, strlen(cur), entry);
	free(cur);
	if (entry->valid == INVALID) { //nothing found
		free(entry);
		return -1;
	}
	else if (remPath == NULL) 
		readi(entry->ino, inode);
	else 
		get_node_by_path(remPath, entry->ino, inode);

	free(entry);
	return 0;
}

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
        
        bio_write(DATA_BLOCK_RESERVE_INDEX+ (blk_index - 1), testString );
    }

    /* should print buffer read: testing writing i */
    
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        bio_read(DATA_BLOCK_RESERVE_INDEX+i, buf);
		printf("%s\n", buf);
        if (strcmp(testString, buf)){
            perror("incorrect memory stored");
            return -1;
        }
    }

    if (tries == realIndex(get_avail_blkno())){
        printf("passed test_simple_block\n");
        return 0;
    } else{
        perror("not reading blocks properly\n");
        return -1;
    }

}


void test_dir_functions() {
	struct inode* rootInode = (struct inode *) malloc(sizeof(struct inode));
	rootInode->ino = get_avail_ino();
	rootInode->size = 0;
	rootInode->valid = VALID;
	rootInode->type = 0;
    rootInode->link = 0;
	writei(rootInode->ino, rootInode);

	struct inode* fileDir = (struct inode *) malloc(sizeof(struct inode));
	fileDir->ino = get_avail_ino();
	fileDir->size = 0;
	fileDir->valid = VALID;
	fileDir->type = 0;
    fileDir->link = 0;
	writei(fileDir->ino, fileDir);

	struct inode* fileNode = (struct inode *) malloc(sizeof(struct inode));
	fileNode->ino = get_avail_ino();
	fileNode->size = 0;
	fileNode->valid = VALID;
	fileNode->type = 0;
    fileNode->link = 0;
	writei(fileNode->ino, fileNode);
	
	dir_add(*rootInode, fileDir->ino, "files", 5);
	dir_add(*fileDir, fileNode->ino, "file.txt", 8);
	struct dirent * entryOne = (struct dirent *) malloc(sizeof (struct dirent)),
		* entryTwo = (struct dirent *) malloc(sizeof (struct dirent));
	dir_find(rootInode->ino, "files", 5, entryOne);
	dir_find(fileDir->ino, "file.txt", 8, entryTwo);
	printf("Files Dir Inode: %d Entry One Inode: %d\n", fileDir->ino, entryOne->ino);
	printf("Files Dir name: %s\n", entryOne->name);
	printf("Text File Inode: %d Entry Two Inode: %d\n", fileNode->ino, entryTwo->ino);
	printf("Text File name: %s\n", entryTwo->name);
	
	struct inode * reloadedNode = (struct inode *) malloc(sizeof(struct inode));
	get_node_by_path("/files/file.txt", rootInode->ino, reloadedNode);
	printf("Node found by path: %d\n", reloadedNode->ino);

	free(rootInode);
	free(fileNode);
	free(entryOne);
	free(reloadedNode);
	free(entryTwo);
	free(fileDir);
}

int main(int argc, char **argv){
    
    inodeBitmap = (bitmap_t) malloc(BLOCK_SIZE * INODE_BITMAP_SIZE);
    dataBitmap = (bitmap_t) malloc(BLOCK_SIZE * DATA_BITMAP_SIZE);

    setup_fs();

    //test_inode_write(); 
	//test_simple_block();

	test_dir_functions();
	
	free(super);
    free(inodeBitmap);
    free(dataBitmap);
}

