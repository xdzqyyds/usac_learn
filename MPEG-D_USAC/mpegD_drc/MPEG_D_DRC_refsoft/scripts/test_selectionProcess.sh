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
uniDrcInterface=yes
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
    if [ $uniDrcInterface = yes ]
    then
   		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface1.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams1.txt -of ../../outputData/mpegh3daSelProcTransferData1.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface2.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams2.txt -of ../../outputData/mpegh3daSelProcTransferData2.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface3.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams3.txt -of ../../outputData/mpegh3daSelProcTransferData3.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface4.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams4.txt -of ../../outputData/mpegh3daSelProcTransferData4.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface5.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams5.txt -of ../../outputData/mpegh3daSelProcTransferData5.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface6.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams6.txt -of ../../outputData/mpegh3daSelProcTransferData6.txt -v 0	 
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface7.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams7.txt -of ../../outputData/mpegh3daSelProcTransferData7.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface8.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams8.txt -of ../../outputData/mpegh3daSelProcTransferData8.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface9.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams9.txt -of ../../outputData/mpegh3daSelProcTransferData9.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface10.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams10.txt -of ../../outputData/mpegh3daSelProcTransferData10.txt -v 0		
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface11.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams11.txt -of ../../outputData/mpegh3daSelProcTransferData11.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface12.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams12.txt -of ../../outputData/mpegh3daSelProcTransferData12.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface13.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams13.txt -of ../../outputData/mpegh3daSelProcTransferData13.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface14.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams14.txt -of ../../outputData/mpegh3daSelProcTransferData14.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface15.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams15.txt -of ../../outputData/mpegh3daSelProcTransferData15.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface16.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams16.txt -of ../../outputData/mpegh3daSelProcTransferData16.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface17.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams17.txt -of ../../outputData/mpegh3daSelProcTransferData17.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface18.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams18.txt -of ../../outputData/mpegh3daSelProcTransferData18.txt -v 0		
	    ./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface19.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams19.txt -of ../../outputData/mpegh3daSelProcTransferData19.txt -v 0
     	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daUniDrcInterface20.bit -mpegh3daParams ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/mpegh3daParams20.txt -of ../../outputData/mpegh3daSelProcTransferData20.txt -v 0
    else
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 1 -of ../../outputData/mpegh3daSelProcTransferData1.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 2 -of ../../outputData/mpegh3daSelProcTransferData2.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 3 -of ../../outputData/mpegh3daSelProcTransferData3.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 4 -of ../../outputData/mpegh3daSelProcTransferData4.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 5 -of ../../outputData/mpegh3daSelProcTransferData5.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet1.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet1.txt -cp 6 -of ../../outputData/mpegh3daSelProcTransferData6.txt -v 0	 
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 7 -of ../../outputData/mpegh3daSelProcTransferData7.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 8 -of ../../outputData/mpegh3daSelProcTransferData8.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 9 -of ../../outputData/mpegh3daSelProcTransferData9.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 10 -of ../../outputData/mpegh3daSelProcTransferData10.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 11 -of ../../outputData/mpegh3daSelProcTransferData11.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 12 -of ../../outputData/mpegh3daSelProcTransferData12.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 13 -of ../../outputData/mpegh3daSelProcTransferData13.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 14 -of ../../outputData/mpegh3daSelProcTransferData14.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 15 -of ../../outputData/mpegh3daSelProcTransferData15.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 16 -of ../../outputData/mpegh3daSelProcTransferData16.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 17 -of ../../outputData/mpegh3daSelProcTransferData17.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig3.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet3.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet3.txt -cp 18 -of ../../outputData/mpegh3daSelProcTransferData18.txt -v 0		
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -cp 19 -of ../../outputData/mpegh3daSelProcTransferData19.txt -v 0
		./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -cp 20 -of ../../outputData/mpegh3daSelProcTransferData20.txt -v 0
	fi
else
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData2.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData3.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain4.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface2.bit -of ../../outputData/selProcTransferData4.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface3.bit -of ../../outputData/selProcTransferData5.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface4.bit -of ../../outputData/selProcTransferData6.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface5.bit -of ../../outputData/selProcTransferData7.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface6.bit -of ../../outputData/selProcTransferData8.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface7.bit -of ../../outputData/selProcTransferData9.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface8.bit -of ../../outputData/selProcTransferData10.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface9.bit -of ../../outputData/selProcTransferData11.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface10.bit -of ../../outputData/selProcTransferData12.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface11.bit -of ../../outputData/selProcTransferData13.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface12.bit -of ../../outputData/selProcTransferData14.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface13.bit -of ../../outputData/selProcTransferData15.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface14.bit -of ../../outputData/selProcTransferData16.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface15.bit -of ../../outputData/selProcTransferData17.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface16.bit -of ../../outputData/selProcTransferData18.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface17.bit -of ../../outputData/selProcTransferData19.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface18.bit -of ../../outputData/selProcTransferData20.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface19.bit -of ../../outputData/selProcTransferData21.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface20.bit -of ../../outputData/selProcTransferData22.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface21.bit -of ../../outputData/selProcTransferData23.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain5.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface22.bit -of ../../outputData/selProcTransferData24.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface23.bit -of ../../outputData/selProcTransferData25.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface24.bit -of ../../outputData/selProcTransferData26.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface25.bit -of ../../outputData/selProcTransferData27.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface26.bit -of ../../outputData/selProcTransferData28.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface27.bit -of ../../outputData/selProcTransferData29.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface28.bit -of ../../outputData/selProcTransferData30.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface29.bit -of ../../outputData/selProcTransferData31.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface30.bit -of ../../outputData/selProcTransferData32.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface31.bit -of ../../outputData/selProcTransferData33.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface32.bit -of ../../outputData/selProcTransferData34.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface33.bit -of ../../outputData/selProcTransferData35.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface34.bit -of ../../outputData/selProcTransferData36.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface35.bit -of ../../outputData/selProcTransferData37.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain6.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface36.bit -of ../../outputData/selProcTransferData38.txt -v 0

	# splitted bitstream file format	
	./uniDrcSelectionProcessCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/uniDrcConfig1.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/loudnessInfoSet1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData1_splittedBs.txt -v 0
fi

if [ $compare = yes ]
then
	if [ $MPEGH = yes ]
  	then
   	  	for i in {1..20}
        do
		    diff -s ../../TestDataOut/mpegh3daSelProcTransferData${i}.txt ../../outputData/mpegh3daSelProcTransferData${i}.txt
  		done
	else
		for i in {1..38}
  		do
    		diff -s ../../TestDataOut/selProcTransferData${i}.txt ../../outputData/selProcTransferData${i}.txt
		done
   		diff -s ../../TestDataOut/selProcTransferData1.txt ../../outputData/selProcTransferData1_splittedBs.txt
	fi
fi

cd $currentDir
