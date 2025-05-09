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
uniDrcInterface=no

cd ../modules/drcTool/drcToolDecoder/outputData
find . -name '*.wav' -type f -delete
find . -name '*.qmf' -type f -delete
find . -name '*.stft' -type f -delete
cd $currentDir

cd ../modules/drcTool/drcToolDecoder/bin/${binaryFolder}

if [ "$2" = "MPEGH" ]; then
	echo -e "\n-- $0: Nothing to do for MPEG-H?!?\n\n"
	exit 0
fi

if [ $uniDrcInterface = yes ]
then
	# test points 
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain1_ld.bit -of ../../outputData/audioOut1_ld.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -dm 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines_QMF64 -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1_QMF64 -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -dt 2 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines_STFT256 -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1_STFT256 -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -dt 3 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain2.bit -of ../../outputData/audioOut2.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain3.bit -of ../../outputData/audioOut3.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain4.bit -of ../../outputData/audioOut4.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface2.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut5.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface34.bit -v 0
	
	# comparison duck self and other
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut6.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface32.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut7.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface33.bit -v 0
	
	# splitted bitstream file format	
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ic ../../../drcToolEncoder/TestDataOut/uniDrcConfig1.bit -il ../../../drcToolEncoder/TestDataOut/loudnessInfoSet1.bit -ig ../../../drcToolEncoder/TestDataOut/uniDrcGain1.bit -of ../../outputData/audioOut1_splittedBs.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -v 0
else
	# test points 
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain1_ld.bit -of ../../outputData/audioOut1_ld.wav -cp 1 -dm 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSines_QMF64 -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1_QMF64 -cp 1 -dt 2 -v 0
    ./drcToolDecoder -if ../../TestDataIn/audio2chSines_STFT256 -ib ../../../drcToolEncoder/TestDataOut/DrcGain1.bit -of ../../outputData/audioOut1_STFT256 -cp 1 -dt 3 -v 0 
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ib ../../../drcToolEncoder/TestDataOut/DrcGain2.bit -of ../../outputData/audioOut2.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain3.bit -of ../../outputData/audioOut3.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain4.bit -of ../../outputData/audioOut4.wav -cp 2	-v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut5.wav -cp 34 -v 0

	# comparison duck self and other
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut6.wav -cp 32 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSinesWN.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain6.bit -of ../../outputData/audioOut7.wav -cp 33 -v 0

	# splitted bitstream file format	
	./drcToolDecoder -if ../../TestDataIn/audio2chSines.wav   -ic ../../../drcToolEncoder/TestDataOut/uniDrcConfig1.bit -il ../../../drcToolEncoder/TestDataOut/loudnessInfoSet1.bit -ig ../../../drcToolEncoder/TestDataOut/uniDrcGain1.bit -of ../../outputData/audioOut1_splittedBs.wav -cp 1 -v 0
fi

if [ $compare = yes ]
then
  GREPCOMMAND="grep -e 'File A = File B' -e 'WAVE file' -e 'Max Diff'"
  for i in 1 1_ld 2 3 4 5
  do
    CompAudio ../../TestDataOut/audioOut${i}.wav ../../outputData/audioOut${i}.wav | eval $GREPCOMMAND
  done
  CompAudio ../../TestDataOut/audioOut1_QMF64_real.qmf ../../outputData/audioOut1_QMF64_real.qmf | eval $GREPCOMMAND
  CompAudio ../../TestDataOut/audioOut1_QMF64_imag.qmf ../../outputData/audioOut1_QMF64_imag.qmf | eval $GREPCOMMAND
  CompAudio ../../TestDataOut/audioOut1_STFT256_real.stft ../../outputData/audioOut1_STFT256_real.stft | eval $GREPCOMMAND
  CompAudio ../../TestDataOut/audioOut1_STFT256_imag.stft ../../outputData/audioOut1_STFT256_imag.stft | eval $GREPCOMMAND
  CompAudio ../../outputData/audioOut6.wav ../../outputData/audioOut7.wav | eval $GREPCOMMAND
  CompAudio ../../TestDataOut/audioOut1.wav ../../outputData/audioOut1_splittedBs.wav | eval $GREPCOMMAND
fi

cd $currentDir
