#!/bin/bash
> DISKFILE
fusermount -u /tmp/sa1547/mountdir
make clean
make
# mkdir /tmp/hs884/
# mkdir /tmp/hs884/mountdir/
./tfs -s -d /tmp/sa1547/mountdir
rm *.o
