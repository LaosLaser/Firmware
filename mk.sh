#!/bin/bash
#
# mk.sh
# build laoslaser binary
# Assumes all is setup according to the readme.md file
#
export LD_PRELOAD=
export GIT_HASH=`git rev-parse --short=6 HEAD`
TARGET=`date +laoslaser-%Y-%m-%d-$GIT_HASH.bin`
echo $GIT_HASH
echo $TARGET
cd mbed
python workspace_tools/build.py -m LPC1768 -t GCC_ARM -r -e -u 
python workspace_tools/make.py -m LPC1768 -t GCC_ARM  -c -n laser -D __GIT_HASH=\"$GIT_HASH\" -v
#python workspace_tools/make.py -m LPC1768 -t GCC_ARM  -n iotest
cp /home/pbrier/Documents/git/Firmware/mbed/build/test/LPC1768/GCC_ARM/laser/laser.bin  ../$TARGET
sync

