/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"
#include "global.h"

char diskfile_path[PATH_MAX];
FILE *file;

// Declare your in-memory data structures here
char buf[BLOCK_SIZE];
bitmap_t inodeBitmap, dataBitmap;
struct superblock * super; 

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
    int i;
    for (i = 1; i < BLOCK_SIZE * INODE_BITMAP_SIZE; i++) //we start at 1 because we want 0 to mean null
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


/* list out files in a dir */
int dir_list(uint16_t ino, struct dirent *dirent) {

    // Step 1: Call readi() to get the inode using ino (inode number of current directory)
    struct inode * dir = (struct inode *) malloc(sizeof(struct inode));
    readi(ino, dir);

    // Step 2: Get data block of current directory from inode
    // Step 3: Read directory's data block and check each directory entry.
    //If the name matches, then copy directory entry to dirent structure

    //go through each block of the directory. For each block, look at each dirent entry. 
    //if the dirent entry matches, stop iterating

    int realBlockIndex;

    printf("\nListing files in Dir: %d\n", ino );
        
    for(int i = 0; i < DIRECT_PTR_ARR_SIZE; i++){
		
        realBlockIndex = realIndex(dir->direct_ptr[i]);
        if (dir->direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0)
            continue;
        
        bio_read(realBlockIndex, buf);
		
        for(int j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++){
            memcpy(dirent, buf + j * sizeof(struct dirent), sizeof(struct dirent));
            if (dirent->valid != INVALID){
                printf("%d: %s\n", dirent->ino, dirent->name);
            }
        }
    }
    printf("\n\n");
    free(dir);
    return -1;
}


/* files a file and stores dirent header in passed in value */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

    // Step 1: Call readi() to get the inode using ino (inode number of current directory)
    struct inode * dir = (struct inode *) malloc(sizeof(struct inode));
    readi(ino, dir);

    // Step 2: Get data block of current directory from inode
    // Step 3: Read directory's data block and check each directory entry.
    //If the name matches, then copy directory entry to dirent structure

    //go through each block of the directory. For each block, look at each dirent entry. 
    //if the dirent entry matches, stop iterating

    int realBlockIndex;
    char* block_ptr = malloc(BLOCK_SIZE);

        
    for(int i = 0; i < DIRECT_PTR_ARR_SIZE; i++){
		
        realBlockIndex = realIndex(dir->direct_ptr[i]);
        if (dir->direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0)
            continue;
        

        bio_read(realBlockIndex, block_ptr);
		
        for(int j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++){
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
    return -1;
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

    int realBlockIndex, firstEmptyBlockIndex = -1, firstEmptyEntryIndex = -1, firstEmptyEntryBlockIndex = -1;
    char* block_ptr = malloc(BLOCK_SIZE);
    struct dirent * entry = (struct dirent *) malloc(sizeof(struct dirent));
    struct dirent * firstEmptyEntry = NULL;

    /* printf("dir info: ino: %d, dirptr %d\n", dir_inode.ino, dir_inode.direct_ptr[0]); */
    if (dir_find(dir_inode.ino, fname, name_len, entry) == 0){
        printf("Can't add, file already exists\n");
        return -1;
    }

    for (int i = 0; i < DIRECT_PTR_ARR_SIZE; i++) {
        realBlockIndex = realIndex(dir_inode.direct_ptr[i]);

        if (firstEmptyEntry != NULL)
            break;
        
        if (dir_inode.direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0) {
            /* printf("continued\n"); */
            firstEmptyBlockIndex = firstEmptyBlockIndex == -1 ? i : firstEmptyBlockIndex; 
            continue;
        }

        bio_read(realBlockIndex, block_ptr);

        /* printf("starting inner: %d\n", i); */
        for (int j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {

            memcpy(entry, block_ptr + j*sizeof(struct dirent), sizeof(struct dirent));
            /* printf("block entry in loop: %d\n", entry->valid); */
            if (entry->valid == INVALID) {
                /* printf("breaking add: %d, %d\n", i, j); */
                firstEmptyEntry = entry;
                firstEmptyEntryIndex = j;
                firstEmptyEntryBlockIndex = i;
                break;
            }			
        }
    }
    
    // Step 3: Add directory entry in dir_inode's data block and write to disk
    // Allocate a new data block for this directory if it does not exist
    // Update directory inode
    // Write directory entry
    if (firstEmptyEntry == NULL && firstEmptyBlockIndex == -1) {        //directory is full, end here
        free(block_ptr);
        free(entry);
        return -1;
    }
    else if (firstEmptyEntry == NULL) { // if one entry is invalid, replace with that
        printf("b\n");
        printf("b? %d, %d, %d\n", firstEmptyEntryIndex, firstEmptyEntryBlockIndex, 69);
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
    else { //space in directory, so just write ther
        /* printf("c\n"); */
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
                printf("Removing file\n");
                entry->valid = INVALID;
                memcpy(block_ptr + j * sizeof(struct dirent), entry, sizeof(struct dirent));
                bio_write(realBlockIndex, block_ptr);
                free(block_ptr);
                free(entry);
                return 0;
            }
        }
    }
    
    printf("No file to delete\n");
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

    char * token;
    char * rest = path;
    
    struct dirent * entry = (struct dirent *) malloc(sizeof(struct dirent));

    token = strtok_r(rest, "/", &rest);

    if (token == NULL){//if no more path left, means we are looking in currentdir
        readi(ino, inode);
        return 0;
    }

    // if more paths left
    if (dir_find(ino, token, strlen(token), entry) == 0)
        return get_node_by_path(rest, entry->ino, inode);
    else
        return -1;
    
}
/* 
 * Make file system
 */

/* this code creates a test file system */
/* the first found inode is DIR_TYPE, is basically the main */
/* this node has 2 direct pointers to 2 blocks */
/* each block has a dirent inside */
/* both dirent entries have the same inode value, need to edit */
    
void setup_fs_files(){
    printf("in setup_files\n");

    struct dirent * testDirent = (struct dirent *) malloc(sizeof(struct dirent));
    int new_inode_num, block, block2;
    struct inode * node = (struct inode *) malloc(sizeof(struct inode));
    
    new_inode_num = get_avail_ino();
    block = get_avail_blkno();
    readi(new_inode_num, node);

    
    node->ino = new_inode_num;
    node->valid = VALID;
    node->size = 3; //idk how to calc size
    node->type = DIR_TYPE;
    node->link = 0;


    node->direct_ptr[0] = block;

    writei(node->ino, node);
    printf("here\n");

    char dir_name[10] = "testDir";

    testDirent->ino = new_inode_num;
    testDirent->valid = 1;
    memcpy(testDirent->name, dir_name, 8);

    
    bio_write(realIndex(block), testDirent);

    /* new block of file */
    block2 = realIndex(get_avail_blkno());
    node->direct_ptr[1] = abstractIndex(block2);
    
    writei(node->ino, node);

    char dir_name2[10] = "childDir";

    testDirent->ino = new_inode_num;
    testDirent->valid = 1;
    memcpy(testDirent->name, dir_name2, 17);

    
    bio_write(block2, testDirent);
    
    free(testDirent);
    /* free(node); */
    printf("fisihed setup\n");
}


int tfs_mkfs() {
    printf("in tfs_mkfs\n");
    // Call dev_init() to initialize (Create) Diskfile
    dev_init(DISKFILE);
    
    // write superblock information
    
    super = (struct superblock *) malloc(sizeof(struct superblock));
    super->magic_num = MAGIC_NUM;
    super->max_inum = INODE_BLOCK_RESERVE - 1;
    super->max_dnum = DATA_BLOCK_RESERVE - 1;
    super->i_bitmap_blk = INODE_MAP_INDEX;
    super->d_bitmap_blk = DATA_MAP_INDEX;
    super->i_start_blk = INODE_BLOCK_RESERVE_INDEX;
    super->d_start_blk = DATA_BLOCK_RESERVE_INDEX;

    // initialize inode and data block bitmap
    inodeBitmap = (bitmap_t) malloc(BLOCK_SIZE * INODE_BITMAP_SIZE);
    dataBitmap = (bitmap_t) malloc(BLOCK_SIZE * DATA_BITMAP_SIZE);
    int i = 0;
    for (i = 0; i < BLOCK_SIZE*DATA_BITMAP_SIZE; i++){
        inodeBitmap[i] = 0;
        dataBitmap[i] = 0;
    }


    // update bitmap information and inode map for root directory
    struct inode * root = (struct inode *) malloc(sizeof(struct inode));
    root->ino = 1;
    root->valid = VALID;
    root->size = 0;
    root->type = DIR_TYPE;
    root->link = 2;
    root->direct_ptr[0] = get_avail_blkno();
    
    bio_write(SUPERBLOCK_INDEX, super);
    bio_write(INODE_MAP_INDEX, inodeBitmap);
    bio_write(DATA_MAP_INDEX, dataBitmap);

    writei(root->ino, root);

    printf("root ino at: %d\n", root->ino);

    
    /* setup_fs_files(); */
    return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

    // Step 1a: If disk file is not found, call mkfs
    printf("tfs init\n");
    if (file != fopen(DISKFILE, "r")){
        printf("making new\n");
        tfs_mkfs();
    } else{
    // Step 1b: If disk file is found, just initialize in-memory data structures
    // and read superblock from disk
        printf("reusing\n");
        dev_open(DISKFILE);
        bio_read(0, buf);
        memcpy(super, buf, sizeof(super));

        if (super->magic_num!= MAGIC_NUM){
            fprintf(stderr, "wrong magic");
            exit(0);
        }
           
        // TODO READ SUPERBLOCK
        inodeBitmap = (bitmap_t) malloc(sizeof(BLOCK_SIZE * INODE_BITMAP_SIZE));
        dataBitmap = (bitmap_t) malloc(sizeof(BLOCK_SIZE * DATA_BITMAP_SIZE));
    }


    return NULL;
}

static void tfs_destroy(void *userdata) {

    // Step 1: De-allocate in-memory data structures

    // TODO free super
    free(inodeBitmap);
    free(dataBitmap);
    
    // Step 2: Close diskfile
    fclose(file);
}

static int tfs_getattr(const char *path, struct stat *stbuf) {

    // Step 1: call get_node_by_path() to get inode from path
    /* get_node_by_path(path, stbuf->ino_t, reloadedNode); */
    // Step 2: fill attribute of file into stbuf from inode
    struct inode * cur_node = (struct inode *) malloc(sizeof(struct inode));

    get_node_by_path(path, 0, cur_node);

    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_ino = cur_node->ino;
    stbuf->st_nlink = cur_node->link;
    stbuf->st_size = cur_node->size;
    stbuf->st_blksize = BLOCK_SIZE;
    
    if(cur_node->type == FILE_TYPE) {
        stbuf->st_mode = S_IFREG | 0644;
    }
    else {
        stbuf->st_mode = S_IFDIR | 0755;
    }

    time(&stbuf->st_mtime);
    free(cur_node);
    return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

    struct inode * cur_node = (struct inode *) malloc(sizeof(struct inode));
    // Step 1: Call get_node_by_path() to get inode from path
    printf("in opendir\n");
    get_node_by_path(path, 1, cur_node);

    if (cur_node->type != DIR_TYPE){
        printf("not a dir: %d, %d, %d\n", cur_node->type, cur_node->ino, cur_node->valid);
        printf("path: %s\n", path);
        free(cur_node);
        return -1;
    }
    
    // Step 2: If not find, return -1
    free(cur_node);

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

    // Step 1: Call get_node_by_path() to get inode from path

    struct inode * cur_node = (struct inode *) malloc(sizeof(struct inode));
    // Step 1: Call get_node_by_path() to get inode from path
    printf("in readdir\n");
    get_node_by_path(path, 1, cur_node);

    if (cur_node->type != DIR_TYPE){
        printf("not a dir: %d\n", cur_node->type);
        free(cur_node);
        return -1;
    }
    
    // Step 2: Read directory entries from its data blocks, and copy them to filler

    struct dirent * dirent = (struct dirent *) malloc(sizeof(struct dirent));
    int realBlockIndex;
    for(int i = 0; i < DIRECT_PTR_ARR_SIZE; i++){
		
        realBlockIndex = realIndex(cur_node->direct_ptr[i]);
        if (cur_node->direct_ptr[i] == 0 || get_bitmap(dataBitmap, realBlockIndex) == 0)
            continue;
        
        bio_read(realBlockIndex, buf);
		
        for(int j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++){
            memcpy(dirent, buf + j * sizeof(struct dirent), sizeof(struct dirent));
            if (dirent->valid != INVALID){
                filler(buffer, dirent->name, NULL, 0);
            }
        }
    }

    free(dirent);
    return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name

    // Step 2: Call get_node_by_path() to get inode of parent directory

    // Step 3: Call get_avail_ino() to get an available inode number

    // Step 4: Call dir_add() to add directory entry of target directory to parent directory

    // Step 5: Update inode for target directory

    // Step 6: Call writei() to write inode to disk
	

    return 0;
}

static int tfs_rmdir(const char *path) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name

    // Step 2: Call get_node_by_path() to get inode of target directory

    // Step 3: Clear data block bitmap of target directory

    // Step 4: Clear inode bitmap and its data block

    // Step 5: Call get_node_by_path() to get inode of parent directory

    // Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

    return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target file name

    // Step 2: Call get_node_by_path() to get inode of parent directory

    // Step 3: Call get_avail_ino() to get an available inode number

    // Step 4: Call dir_add() to add directory entry of target file to parent directory

    // Step 5: Update inode for target file

    // Step 6: Call writei() to write inode to disk

    return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

    // Step 1: Call get_node_by_path() to get inode from path

    // Step 2: If not find, return -1

    return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    // Step 1: You could call get_node_by_path() to get inode from path

    // Step 2: Based on size and offset, read its data blocks from disk

    // Step 3: copy the correct amount of data from offset to buffer

    // Note: this function should return the amount of bytes you copied to buffer
    return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Step 1: You could call get_node_by_path() to get inode from path

    // Step 2: Based on size and offset, read its data blocks from disk

    // Step 3: Write the correct amount of data from offset to disk

    // Step 4: Update the inode info and write it to disk

    // Note: this function should return the amount of bytes you write to disk
    return size;
}

static int tfs_unlink(const char *path) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target file name

    // Step 2: Call get_node_by_path() to get inode of target file

    // Step 3: Clear data block bitmap of target file

    // Step 4: Clear inode bitmap and its data block

    // Step 5: Call get_node_by_path() to get inode of parent directory

    // Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

    return 0;
}

static int tfs_truncate(const char *path, off_t size) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
    .init	= tfs_init,
    .destroy	= tfs_destroy,

    .getattr	= tfs_getattr,
    .readdir	= tfs_readdir,
    .opendir	= tfs_opendir,
    .releasedir	= tfs_releasedir,
    .mkdir	= tfs_mkdir,
    .rmdir	= tfs_rmdir,

    .create	= tfs_create,
    .open	= tfs_open,
    .read 	= tfs_read,
    .write	= tfs_write,
    .unlink	= tfs_unlink,

    .truncate   = tfs_truncate,
    .flush      = tfs_flush,
    .utimens    = tfs_utimens,
    .release	= tfs_release
};


int main(int argc, char *argv[]) {
    int fuse_stat;
    getcwd(diskfile_path, PATH_MAX);
    strcat(diskfile_path, "/DISKFILE");
    return fuse_main(argc, argv, &tfs_ope, NULL);
}

