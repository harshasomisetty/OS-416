#!/bin/bash

fusermount -u /tmp/hs884/mountdir
make clean
make
# mkdir /tmp/hs884/
# mkdir /tmp/hs884/mountdir/
./tfs -s -d /tmp/hs884/mountdir
rm *.o
