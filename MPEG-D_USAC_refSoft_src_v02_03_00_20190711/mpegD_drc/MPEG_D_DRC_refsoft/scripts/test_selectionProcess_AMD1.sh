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

compare=yes
MPEGH=no
if [ -n $2 ]; then
	if [ "$2" = "MPEGH" ]
	then
		MPEGH=yes
	fi
fi

currentDir=$(pwd)

cd ../modules/uniDrcModules/uniDrcSelectionProcessCmdl/outputData
find . -name '*.txt' -type f -delete
cd $currentDir

cd ../modules/uniDrcModules/uniDrcSelectionProcessCmdl/bin/${binaryFolder}

if [ $MPEGH = yes ]
then
  exit -1
else
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -of ../../outputData/selProcTransferData1_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -of ../../outputData/selProcTransferData2_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -of ../../outputData/selProcTransferData3_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain4_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -of ../../outputData/selProcTransferData4_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -of ../../outputData/selProcTransferData5_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -of ../../outputData/selProcTransferData6_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain7_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface39_AMD1.bit -of ../../outputData/selProcTransferData7_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain8_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -of ../../outputData/selProcTransferData8_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain9_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -of ../../outputData/selProcTransferData9_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain10_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -of ../../outputData/selProcTransferData10_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain11_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -of ../../outputData/selProcTransferData11_AMD1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain12_AMD1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -of ../../outputData/selProcTransferData12_AMD1.txt -v 0
fi

if [ $compare = yes ]
then
	for i in {1..12}
  	do
     	diff -s ../../TestDataOut/selProcTransferData${i}_AMD1.txt ../../outputData/selProcTransferData${i}_AMD1.txt
	done
fi

cd $currentDir
