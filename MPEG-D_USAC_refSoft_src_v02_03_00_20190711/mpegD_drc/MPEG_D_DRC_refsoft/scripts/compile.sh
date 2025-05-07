#!/bin/bash

clobber=""
MPEGH=0
AMD1=0
WRITE_FRAMESIZE=0
NODE_RESERVOIR=0
LOUDNESS_LEVELING=0

for p in $@
do
	if [ $p == "clobber" ]; then
		clobber="--clean-first"
        echo Cleaning up...
	fi
	if [ $p == "MPEGH" ]; then
		MPEGH=1
        LOUDNESS_LEVELING=1
		echo Compiling MPEGH...
	fi
	if [ $p == "AMD1" ]; then
		AMD1=1
        LOUDNESS_LEVELING=1
        echo Compiling AMD1...
	fi
	if [ $p == "WRITE_FRAMESIZE" ]; then
		WRITE_FRAMESIZE=1
        echo Compiling WRITE_FRAMESIZE...
        fi
	if [ $p == "NODE_RESERVOIR" ]; then
		NODE_RESERVOIR=1
        echo Compiling NODE_RESERVOIR...
	fi
done

currentDir=$(pwd)

mkdir -p build && cd build
cmake -DAMD1=$AMD1 -DMPEGH=$MPEGH -DLOUDNESS_LEVELING=$LOUDNESS_LEVELING -DWRITE_FRAMESIZE=$WRITE_FRAMESIZE -DNODE_RESERVOIR=$NODE_RESERVOIR ../.. && cmake --build . $clobber