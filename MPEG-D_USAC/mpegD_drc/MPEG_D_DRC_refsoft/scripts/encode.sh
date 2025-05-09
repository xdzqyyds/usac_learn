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

cd ../modules/drcTool/drcToolEncoder/outputData
find . -name '*.bit' -type f -delete
cd $currentDir

cd ../modules/drcTool/drcToolEncoder/bin/${binaryFolder}

if [ $MPEGH = yes ]
then
	./drcToolEncoder 1 ../../TestDataIn/DrcGain8vectors.dat ../../outputData/mpegh3daUniDrcConfig1.bit ../../outputData/mpegh3daLoudnessInfoSet1.bit ../../outputData/mpegh3daUniDrcGain1.bit
	./drcToolEncoder 2 ../../TestDataIn/DrcGain4vectors.dat ../../outputData/mpegh3daUniDrcConfig2.bit ../../outputData/mpegh3daLoudnessInfoSet2.bit ../../outputData/mpegh3daUniDrcGain2.bit
	./drcToolEncoder 3 ../../TestDataIn/DrcGain1vector.dat ../../outputData/mpegh3daUniDrcConfig3.bit ../../outputData/mpegh3daLoudnessInfoSet3.bit ../../outputData/mpegh3daUniDrcGain3.bit
	./drcToolEncoder 4 ../../TestDataIn/DrcGain7vectors.dat ../../outputData/mpegh3daUniDrcConfig4.bit ../../outputData/mpegh3daLoudnessInfoSet4.bit ../../outputData/mpegh3daUniDrcGain4.bit

	#./drcToolEncoder d ../../TestDataIn/DrcGain4vectors_H.dat ../../outputData/mpegh3daUniDrcConfig2_H.bit ../../outputData/mpegh3daLoudnessInfoSet2_H.bit ../../outputData/mpegh3daUniDrcGain2_H.bit
	
 	./drcToolEncoder ../../TestDataIn/userConfigMpegh1.xml ../../TestDataIn/DrcGain8vectors.dat ../../outputData/mpegh3daUniDrcConfig1_xmlConf.bit ../../outputData/mpegh3daLoudnessInfoSet1_xmlConf.bit ../../outputData/mpegh3daUniDrcGain1_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfigMpegh2.xml ../../TestDataIn/DrcGain4vectors.dat ../../outputData/mpegh3daUniDrcConfig2_xmlConf.bit ../../outputData/mpegh3daLoudnessInfoSet2_xmlConf.bit ../../outputData/mpegh3daUniDrcGain2_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfigMpegh3.xml ../../TestDataIn/DrcGain1vector.dat ../../outputData/mpegh3daUniDrcConfig3_xmlConf.bit ../../outputData/mpegh3daLoudnessInfoSet3_xmlConf.bit ../../outputData/mpegh3daUniDrcGain3_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfigMpegh4.xml ../../TestDataIn/DrcGain7vectors.dat ../../outputData/mpegh3daUniDrcConfig4_xmlConf.bit ../../outputData/mpegh3daLoudnessInfoSet4_xmlConf.bit ../../outputData/mpegh3daUniDrcGain4_xmlConf.bit	
else
	./drcToolEncoder 1 ../../TestDataIn/DrcGain1vector.dat  ../../outputData/DrcGain1.bit
	./drcToolEncoder 1_ld ../../TestDataIn/DrcGain1vector.dat  ../../outputData/DrcGain1_ld.bit
	./drcToolEncoder 2 ../../TestDataIn/DrcGain3vectors.dat ../../outputData/DrcGain2.bit
	./drcToolEncoder 3 ../../TestDataIn/DrcGain7vectors.dat ../../outputData/DrcGain3.bit
	./drcToolEncoder 4 ../../TestDataIn/DrcGain7vectors.dat ../../outputData/DrcGain4.bit
	./drcToolEncoder 5 ../../TestDataIn/DrcGain8vectors.dat ../../outputData/DrcGain5.bit
	./drcToolEncoder 6 ../../TestDataIn/DrcGain8vectors.dat ../../outputData/DrcGain6.bit
	
	./drcToolEncoder ../../TestDataIn/userConfig1.xml ../../TestDataIn/DrcGain1vector.dat  ../../outputData/DrcGain1_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig1_ld.xml ../../TestDataIn/DrcGain1vector.dat  ../../outputData/DrcGain1_ld_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig2.xml ../../TestDataIn/DrcGain3vectors.dat ../../outputData/DrcGain2_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig3.xml ../../TestDataIn/DrcGain7vectors.dat ../../outputData/DrcGain3_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig4.xml ../../TestDataIn/DrcGain7vectors.dat ../../outputData/DrcGain4_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig5.xml ../../TestDataIn/DrcGain8vectors.dat ../../outputData/DrcGain5_xmlConf.bit
	./drcToolEncoder ../../TestDataIn/userConfig6.xml ../../TestDataIn/DrcGain8vectors.dat ../../outputData/DrcGain6_xmlConf.bit
	
	# splitted bitstream file format	
	./drcToolEncoder 1 ../../TestDataIn/DrcGain1vector.dat  ../../outputData/uniDrcConfig1.bit ../../outputData/loudnessInfoSet1.bit ../../outputData/uniDrcGain1.bit
fi

if [ $compare = yes ]
then
  if [ $MPEGH = yes ]
  then
    for i in 1 2 3 4
    do
      diff -s ../../TestDataOut/mpegh3daUniDrcConfig${i}.bit ../../outputData/mpegh3daUniDrcConfig${i}.bit
      diff -s ../../TestDataOut/mpegh3daLoudnessInfoSet${i}.bit ../../outputData/mpegh3daLoudnessInfoSet${i}.bit
      diff -s ../../TestDataOut/mpegh3daUniDrcGain${i}.bit ../../outputData/mpegh3daUniDrcGain${i}.bit
    done
    for i in 1 2 3 4
    do
      diff -s ../../TestDataOut/mpegh3daUniDrcConfig${i}.bit ../../outputData/mpegh3daUniDrcConfig${i}_xmlConf.bit
      diff -s ../../TestDataOut/mpegh3daLoudnessInfoSet${i}.bit ../../outputData/mpegh3daLoudnessInfoSet${i}_xmlConf.bit
      diff -s ../../TestDataOut/mpegh3daUniDrcGain${i}.bit ../../outputData/mpegh3daUniDrcGain${i}_xmlConf.bit
    done
  else
    for i in 1 1_ld 2 3 4 5 6
    do
      diff -s ../../TestDataOut/DrcGain${i}.bit ../../outputData/DrcGain${i}.bit
    done
    for i in 1 1_ld 2 3 4 5 6
    do
      diff -s ../../TestDataOut/DrcGain${i}.bit ../../outputData/DrcGain${i}_xmlConf.bit
    done
    for i in 1
    do
      diff -s ../../TestDataOut/uniDrcConfig${i}.bit ../../outputData/uniDrcConfig${i}.bit
      diff -s ../../TestDataOut/loudnessInfoSet${i}.bit ../../outputData/loudnessInfoSet${i}.bit
      diff -s ../../TestDataOut/uniDrcGain${i}.bit ../../outputData/uniDrcGain${i}.bit
    done
  fi
fi

cd $currentDir
