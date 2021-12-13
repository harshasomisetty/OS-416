#!/bin/bash

make clean >/dev/null
make >/dev/null
cd benchmark
make clean >/dev/null
make >/dev/null
./simple_test
cd ..
rm *.o
