#!/bin/bash

if [ -z $1 ]
then
  echo first argument must be OS name/platform string
  echo example calls:
  echo $0 Linux_x86_64
  echo $0 Windows_AMD64
  echo $0 macOS_x86_64
  echo $0 macOS_arm64
  exit -1
fi

binaryFolder=$1
currentDir=$(pwd)

compare=yes
MPEGH=no
if [ -n $2 ]; then
	if [ "$2" = "MPEGH" ]
	then
		MPEGH=yes
	fi
fi

cd ../modules/uniDrcModules/uniDrcInterfaceEncoderCmdl/outputData
find . -name '*.bit' -type f -delete
cd $currentDir

cd ../modules/uniDrcModules/uniDrcInterfaceEncoderCmdl/bin/${binaryFolder}

if [ $MPEGH = yes ]
then
  for i in {1..20}
  do
	  ./uniDrcInterfaceEncoderCmdl -of ../../outputData/mpegh3daUniDrcInterface${i}.bit -mpegh3daParams ../../outputData/mpegh3daParams${i}.txt -cfg2 ${i} -v 0
	done
else
  for i in {1..37}
  do
	  ./uniDrcInterfaceEncoderCmdl -of ../../outputData/uniDrcInterface${i}.bit -cfg2 ${i} -v 0
	done
fi

if [ $compare = yes ]
then
  if [ $MPEGH = yes ]
  then
    for i in {1..20}
    do
      diff -s ../../TestDataOut/mpegh3daUniDrcInterface${i}.bit ../../outputData/mpegh3daUniDrcInterface${i}.bit
    done
    for i in {1..20}
    do
      diff -s ../../TestDataOut/mpegh3daParams${i}.txt ../../outputData/mpegh3daParams${i}.txt
    done
  else
    for i in {1..37}
    do
      diff -s ../../TestDataOut/uniDrcInterface${i}.bit ../../outputData/uniDrcInterface${i}.bit
    done
  fi
fi

cd $currentDir
