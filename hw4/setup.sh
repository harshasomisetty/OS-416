#!/bin/bash

gcc -w -c block.c
gcc -w -c tfs.c
gcc -w -c disk_setup.c
gcc -w -o setup block.o tfs.o disk_setup.o
./setup
rm *.o setup
