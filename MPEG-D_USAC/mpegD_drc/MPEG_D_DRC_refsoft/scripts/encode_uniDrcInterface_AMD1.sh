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

cd ../modules/uniDrcModules/uniDrcInterfaceEncoderCmdl/outputData
find . -name '*.bit' -type f -delete
cd $currentDir

cd ../modules/uniDrcModules/uniDrcInterfaceEncoderCmdl/bin/${binaryFolder}

for i in 1 37 38 39
do
  ./uniDrcInterfaceEncoderCmdl -of ../../outputData/uniDrcInterface${i}_AMD1.bit -cfg2 ${i} -v 0
done

if [ $compare = yes ]
then
  for i in 1 37 38 39
  do
    diff -s ../../TestDataOut/uniDrcInterface${i}_AMD1.bit ../../outputData/uniDrcInterface${i}_AMD1.bit
  done  
fi

cd $currentDir
