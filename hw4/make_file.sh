#!/bin/bash
END=1000
echo "hi" > /tmp/hs884/mountdir/file2
for i in $( eval echo {0..$END} )
do
    echo "$i jlkj" >> /tmp/hs884/mountdir/file2
done
