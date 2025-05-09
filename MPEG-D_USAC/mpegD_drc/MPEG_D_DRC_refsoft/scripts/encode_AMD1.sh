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

cd ../modules/drcTool/drcToolEncoder/outputData
find . -name '*_AMD1.bit' -type f -delete
cd $currentDir

cd ../modules/drcTool/drcToolEncoder/bin/${binaryFolder}

./drcToolEncoder 7 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain1_AMD1.bit
./drcToolEncoder 8 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain2_AMD1.bit
./drcToolEncoder 9 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain3_AMD1.bit
./drcToolEncoder 10 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain4_AMD1.bit
./drcToolEncoder 11 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain5_AMD1.bit
./drcToolEncoder 12 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain6_AMD1.bit
./drcToolEncoder 13 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain7_AMD1.bit
./drcToolEncoder 14 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain8_AMD1.bit
./drcToolEncoder 15 ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/DrcGain9_AMD1.bit
./drcToolEncoder 16 ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/DrcGain10_AMD1.bit
./drcToolEncoder 17 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain11_AMD1.bit
./drcToolEncoder 18 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain12_AMD1.bit
./drcToolEncoder 19 dummy.dat                                  ../../outputData/DrcGain13_AMD1.bit
./drcToolEncoder 20 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain14_AMD1.bit

./drcToolEncoder ../../TestDataIn/userConfig1_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain1_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig2_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain2_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig3_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain3_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig4_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain4_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig5_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain5_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig6_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain6_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig7_Amd1.xml ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/DrcGain7_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig8_Amd1.xml ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain8_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig9_Amd1.xml ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/DrcGain9_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig10_Amd1.xml ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/DrcGain10_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig11_Amd1.xml ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain11_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig12_Amd1.xml ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain12_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig13_Amd1.xml dummy.dat                                  ../../outputData/DrcGain13_AMD1_xmlConf.bit
./drcToolEncoder ../../TestDataIn/userConfig14_Amd1.xml ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/DrcGain14_AMD1_xmlConf.bit

./drcToolEncoder 7 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/ffDrc1_AMD1.bit  ../../outputData/ffLoudness1_AMD1.bit  ../../outputData/ffDrcGain1_AMD1.bit ff_default
./drcToolEncoder 8 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/ffDrc2_AMD1.bit  ../../outputData/ffLoudness2_AMD1.bit  ../../outputData/ffDrcGain2_AMD1.bit ff_default
./drcToolEncoder 9 ../../TestDataIn/DrcGain1vector_AMD1.dat  ../../outputData/ffDrc3_AMD1.bit  ../../outputData/ffLoudness3_AMD1.bit  ../../outputData/ffDrcGain3_AMD1.bit ff_default
./drcToolEncoder 14 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/ffDrc8_AMD1.bit  ../../outputData/ffLoudness8_AMD1.bit  ../../outputData/ffDrcGain8_AMD1.bit ff_default
./drcToolEncoder 15 ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/ffDrc9_AMD1.bit  ../../outputData/ffLoudness9_AMD1.bit  ../../outputData/ffDrcGain9_AMD1.bit ff_default
./drcToolEncoder 16 ../../TestDataIn/DrcGain3vectors_AMD1.dat  ../../outputData/ffDrc10_AMD1.bit  ../../outputData/ffLoudness10_AMD1.bit  ../../outputData/ffDrcGain10_AMD1.bit ff_default
./drcToolEncoder 17 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/ffDrc11_AMD1.bit  ../../outputData/ffLoudness11_AMD1.bit  ../../outputData/ffDrcGain11_AMD1.bit ff_default
./drcToolEncoder 18 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/ffDrc12_AMD1.bit  ../../outputData/ffLoudness12_AMD1.bit  ../../outputData/ffDrcGain12_AMD1.bit ff_default
./drcToolEncoder 19 dummy.dat                                  ../../outputData/ffDrc13_AMD1.bit  ../../outputData/ffLoudness13_AMD1.bit  ../../outputData/ffDrcGain13_AMD1.bit ff_default
./drcToolEncoder 20 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/ffDrc14_AMD1.bit  ../../outputData/ffLoudness14_AMD1.bit  ../../outputData/ffDrcGain14_AMD1.bit ff_default
./drcToolEncoder 14 ../../TestDataIn/DrcGain1vectorA_AMD1.dat  ../../outputData/ffDrc8_AMD1.bit  ../../outputData/ffLoudness8_AMD1.bit  ../../outputData/ffDrcGain8_AMD1_split.bit ff_split

if [ $compare = yes ]
then
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14
  do
    diff -s ../../TestDataOut/DrcGain${i}_AMD1.bit ../../outputData/DrcGain${i}_AMD1.bit
  done
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14
  do
    diff -s ../../TestDataOut/DrcGain${i}_AMD1.bit ../../outputData/DrcGain${i}_AMD1_xmlConf.bit
  done
  for i in 1 2 3 8 9 10 11 12 13 14
  do
    diff -s ../../TestDataOut/ffDrc${i}_AMD1.bit ../../outputData/ffDrc${i}_AMD1.bit
    diff -s ../../TestDataOut/ffLoudness${i}_AMD1.bit ../../outputData/ffLoudness${i}_AMD1.bit
    diff -s ../../TestDataOut/ffDrcGain${i}_AMD1.bit ../../outputData/ffDrcGain${i}_AMD1.bit
  done
  diff -s ../../TestDataOut/ffDrcGain8_AMD1_split.bit ../../outputData/ffDrcGain8_AMD1_split.bit
fi

cd $currentDir
