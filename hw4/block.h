/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	block.h
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#ifndef _BLOCK_H_
#define _BLOCK_H_

#define BLOCK_SIZE 4096

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

#define DIRECT_PTR_ARR_SIZE 16
#define VALID 1
#define INVALID 0

#define FILE_TYPE 1
#define DIR_TYPE 2

void dev_init(const char* diskfile_path);
int dev_open(const char* diskfile_path);
void dev_close();
int bio_read(const int block_num, void *buf);
int bio_write(const int block_num, const void *buf);

#endif
