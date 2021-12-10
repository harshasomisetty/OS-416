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

struct dirent {
    uint16_t ino;					/* inode number of the directory entry */
    uint16_t valid;					/* validity of the directory entry */
    char name[252];					/* name of the directory entry */
};


bitmap_t inodeBitmap, dataBitmap;
struct superblock * super; 

char buf[BLOCK_SIZE];
struct dirent * dirent_buf;

int new_inode_num, block, block2;
char* dir_name;

struct dirent * readDirent;
struct inode * node;
     
int test_simple_block(){
    
    int tries = 2;
    char testString[]  = "testing writing i";
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        int blk_index = get_avail_blkno();
        
        bio_write(DATA_BLOCK_RESERVE_INDEX+blk_index, testString );
    }

    printf("reading");
    /* should print buffer read: testing writing i */
    
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        bio_read(DATA_BLOCK_RESERVE_INDEX+i, buf);
        printf("testString: %s\n", testString);
        printf("buf: %s\n", buf);
        /* if (strcmp(testString, buf) != 0){ */
        /*     perror("incorrect memory stored"); */
        /*     return -1; */
        /* } */
    }
    int avail_block = get_avail_blkno(dataBitmap);
    if (tries+1 == avail_block){
        printf("passed test_simple_block\n");
        return 0;
    } else{
        perror("not reading blocks properly\n");
        exit(-1);
    }

}

void test_inode_write(){

    int new_inode_num = get_avail_ino();
    printf("Available inode: %d\n", new_inode_num);
    struct inode * node = (struct inode *) malloc(sizeof(struct inode));
    readi(new_inode_num, node);
    
    node->ino = new_inode_num;
    node->valid = 1;
    node->size = 3;
    node->type = FILE_TYPE;
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


int test_dir_add(){
    
    int tries = 2;
    char testString[]  = "testing writing i";
    int blk_index;
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        blk_index = get_avail_blkno();
        
        bio_write(DATA_BLOCK_RESERVE_INDEX+ (blk_index - 1), testString );
    }

    uint16_t f_ino = (uint16_t) get_avail_ino();
    bio_read(DATA_BLOCK_RESERVE_INDEX+ (blk_index - 1), buf);
    printf("vuffer pre: %s\n", buf);
    
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        bio_read(DATA_BLOCK_RESERVE_INDEX+i, buf);
        printf("vuffer: %s\n", buf);
        if (strcmp(testString, buf)){
            errno = -1;
            perror("incorrect memory stored");
            return -1;
        }
    }

    if (tries == realIndex(get_avail_blkno())){
        printf("passed test_simple_block\n");
        return 0;
    } else{
        errno = -1;
        perror("not reading blocks properly\n");
        return -1;
    }

}

int test_dir_find(){

    dir_find(block, "testDir", 17, readDirent);

}


void test_dir_functions() {

    struct inode* rootInode = (struct inode *) malloc(sizeof(struct inode));

    rootInode->ino = get_avail_ino();
    rootInode->size = 0;
    rootInode->valid = VALID;
    rootInode->type = 0;
    rootInode->link = 0;
    rootInode->direct_ptr[0] = get_avail_blkno();
    writei(rootInode->ino, rootInode);

    /* readi(1, rootInode); */

    struct inode* fileDir = (struct inode *) malloc(sizeof(struct inode));
    fileDir->ino = get_avail_ino();
    fileDir->size = 0;
    fileDir->valid = VALID;
    fileDir->type = 0;
    fileDir->link = 0;
    fileDir->direct_ptr[0] = get_avail_blkno();
    writei(fileDir->ino, fileDir);

    printf("root ind: %d\n", rootInode->ino);
    printf("file ind: %d\n", fileDir->ino);
    /* struct inode* fileNode = (struct inode *) malloc(sizeof(struct inode)); */
    /* fileNode->ino = get_avail_ino(); */
    /* fileNode->size = 0; */
    /* fileNode->valid = VALID; */
    /* fileNode->type = 0; */
    /* fileNode->link = 0; */
    /* writei(fileNode->ino, fileNode); */
    
    char * testString = (char*)malloc(8*sizeof(char));
    for (int file_count=0; file_count<14; file_count++){
        sprintf(testString, "dir_%d", file_count);
        dir_add(*rootInode, fileDir->ino, testString, 8);
    }
    
    printf("added dirs\n");

    dir_remove(*rootInode, "dir_4", 7);
    printf("removed?\n");
    /* dir_list(rootInode->ino, readDirent); */
    dir_list(rootInode->ino, readDirent);
    dir_find(rootInode->ino, "dir_3", 6, readDirent);
    
    /* dir_add(*fileDir, fileNode->ino, "file.txt", 8); */
    /* struct dirent * entryOne = (struct dirent *) malloc(sizeof (struct dirent)), */
        /* * entryTwo = (struct dirent *) malloc(sizeof (struct dirent)); */
    /* dir_find(rootInode->ino, "files", 5, entryOne); */
    /* dir_find(fileDir->ino, "file.txt", 8, entryTwo); */
    /* printf("Files Dir Inode: %d Entry One Inode: %d\n", fileDir->ino, entryOne->ino); */
    /* printf("Files Dir name: %s\n", entryOne->name); */
    /* printf("Text File Inode: %d Entry Two Inode: %d\n", fileNode->ino, entryTwo->ino); */
    /* printf("Text File name: %s\n", entryTwo->name); */
	
    struct inode * reloadedNode = (struct inode *) malloc(sizeof(struct inode));
    /* get_node_by_path("/files/file.txt", rootInode->ino, reloadedNode); */
    get_node_by_path("/file9_3", rootInode->ino, reloadedNode);
    printf("Node found by path: %d\n", reloadedNode->ino);

    free(rootInode);
    /* free(fileNode); */
    /* free(entryOne); */
    free(reloadedNode);
    /* free(entryTwo); */
    free(testString);
    free(fileDir);
}

int main(int argc, char **argv){

                
    readDirent = (struct dirent *) malloc(sizeof(struct dirent));
    
    printf("running test dir\n");
    tfs_mkfs();

    printf("super stuff: %d, %d, %d\n", super->i_bitmap_blk, super->d_bitmap_blk, super->i_start_blk);
    /* test_inode_write(); */

    /* test_simple_block(); */

    /* test_dir_find(); */
    /* test_dir_add(); */

    test_dir_functions();
    
    free(super);
    free(inodeBitmap);
    free(dataBitmap);
    free(readDirent);
    
    printf("\n\n\n*****\n\n\n");
}

