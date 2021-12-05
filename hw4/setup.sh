#!/bin/bash

gcc -Wall -c block.c
gcc -Wall -c disk_setup.c
gcc -o setup block.o disk_setup.o
./setup
