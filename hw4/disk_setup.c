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
struct dirent * testDirent;
struct dirent * readDirent;
struct inode * node;
     

/* this code creates a test file system */
/* the first found inode is DIR_TYPE, is basically the main */
/* this node has 2 direct pointers to 2 blocks */
/* each block has a dirent inside */
/* both dirent entries have the same inode value, need to edit */
    
void setup_fs_files(){
    printf("in setup_files\n");

    testDirent = (struct dirent *) malloc(sizeof(struct dirent));

    new_inode_num = get_avail_ino();
    block = get_avail_blkno();

    readi(new_inode_num, node);

    node->ino = new_inode_num;
    node->valid = 1;
    node->size = 3; //idk how to calc size
    node->type = DIR_TYPE;
    node->link = 0;


    node->direct_ptr[0] = block;

    writei(node->ino, node);


    dir_name = "testDir";

    testDirent->ino = new_inode_num;
    testDirent->valid = 1;
    memcpy(testDirent->name, dir_name, 8);

    
    bio_write(realIndex(block), testDirent);

    /* new block of file */
    block2 = realIndex(get_avail_blkno());
    node->direct_ptr[1] = abstractIndex(block2);
    /* printf("2 setup block ind %d\n", block2); */
    
    /* print_arr(node->direct_ptr); //remove */
    
    writei(node->ino, node);

    dir_name = "testDir_childdir";

    testDirent->ino = new_inode_num;
    testDirent->valid = 1;
    memcpy(testDirent->name, dir_name, 17);

    
    bio_write(block2, testDirent);
    
    free(testDirent);
    /* free(node); */
    
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

    setup_fs_files();


}
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


int test_dir_find(){

    dir_find(block, "testDir", 17, readDirent);

}

int test_dir_add(){
    
    int tries = 2;
    char testString[]  = "testing writing i";
    for(int i = 0; i<tries; i++){
        testString[16] = i+'0';
        int blk_index = get_avail_blkno();
        
        bio_write(DATA_BLOCK_RESERVE_INDEX+ (blk_index - 1), testString );
    }

    uint16_t f_ino = (uint16_t) get_avail_ino();
    bio_read(0, buf);
    
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
    node = (struct inode *) malloc(sizeof(struct inode));
    readDirent = (struct dirent *) malloc(sizeof(struct dirent));

    printf("running test dir\n");
    setup_fs();



    /* Test_inode_write(); */

    /* test_simple_block(); */

    test_dir_find();
    /* test_dir_add(); */

    /* test_dir_functions(); */
    
    free(super);
    free(inodeBitmap);
    free(dataBitmap);
    free(readDirent);
    
    printf("\n\n\n*****\n\n\n");
}

