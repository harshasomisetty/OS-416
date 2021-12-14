#!/bin/bash

make clean >/dev/null
make >/dev/null
cd benchmark
make clean >/dev/null
make >/dev/null
rm -rf /tmp/hs884/mountdir/*
./simple_test
rm -rf /tmp/hs884/mountdir/*
./test_case
rm -rf /tmp/hs884/mountdir/*
./bitmap_check
rm -rf /tmp/hs884/mountdir/*
cd ..
rm *.o
