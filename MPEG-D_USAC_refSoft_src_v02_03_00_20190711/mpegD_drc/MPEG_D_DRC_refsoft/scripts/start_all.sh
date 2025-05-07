#! /bin/bash

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

./compile.sh clobber
if [ $? -ne 0 ]
then
	echo Script start_all.sh failed.
	exit -1
fi
./encode.sh $1
./encode_uniDrcInterface.sh $1
./decode.sh $1
./test_selectionProcess.sh $1

./compile.sh clobber MPEGH
if [ $? -ne 0 ]
then
	echo Script start_all.sh failed.
	exit -1
fi
./encode.sh $1 MPEGH
./encode_uniDrcInterface.sh $1 MPEGH
./decode_cmdlToolChain.sh $1 MPEGH
./test_selectionProcess.sh $1 MPEGH

./compile.sh clobber AMD1
if [ $? -ne 0 ]
then
	echo Script start_all.sh failed.
	exit -1
fi
./encode_AMD1.sh $1
./encode_uniDrcInterface_AMD1.sh $1
./decode_AMD1.sh $1
./test_selectionProcess_AMD1.sh $1


