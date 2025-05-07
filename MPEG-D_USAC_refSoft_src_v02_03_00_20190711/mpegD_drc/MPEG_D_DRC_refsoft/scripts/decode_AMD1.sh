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
find . -name '*_AMD1.wav' -type f -delete
cd $currentDir

cd ../modules/drcTool/drcToolDecoder/bin/${binaryFolder}

if [ $uniDrcInterface = yes ]
then
	# test points 
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain1_AMD1.bit -of ../../outputData/audioOut1_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain2_AMD1.bit -of ../../outputData/audioOut2_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain3_AMD1.bit -of ../../outputData/audioOut3_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSweep.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain4_AMD1.bit -of ../../outputData/audioOut4_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSweep.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain5_AMD1.bit -of ../../outputData/audioOut5_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain6_AMD1.bit -of ../../outputData/audioOut6_AMD1_QMF64 -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface37_AMD1.bit -dt 2 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain7_AMD1.bit -of ../../outputData/audioOut7_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface39_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain8_AMD1.bit -of ../../outputData/audioOut8_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio8chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain9_AMD1.bit -of ../../outputData/audioOut9_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain10_AMD1.bit -of ../../outputData/audioOut10_AMD1_QMF64 -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -dt 2 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain11_AMD1.bit -of ../../outputData/audioOut11_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain12_AMD1.bit -of ../../outputData/audioOut12_AMD1_QMF64 -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -dt 2 -v 0 -af 2048
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain13_AMD1.bit -of ../../outputData/audioOut13_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface39_AMD1.bit -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain14_AMD1.bit -of ../../outputData/audioOut14_AMD1.wav -ii ../../../../uniDrcModules/uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface38_AMD1.bit -v 0
else
	# test points 
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain1_AMD1.bit -of ../../outputData/audioOut1_AMD1.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain2_AMD1.bit -of ../../outputData/audioOut2_AMD1.wav -cp 1 -v 0
    ./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain3_AMD1.bit -of ../../outputData/audioOut3_AMD1.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chSweep.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain4_AMD1.bit -of ../../outputData/audioOut4_AMD1.wav -cp 37 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chSweep.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain5_AMD1.bit -of ../../outputData/audioOut5_AMD1.wav -cp 37 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain6_AMD1.bit -of ../../outputData/audioOut6_AMD1_QMF64 -cp 37 -dt 2 -v 0
    ./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/DrcGain7_AMD1.bit -of ../../outputData/audioOut7_AMD1.wav -cp 39 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain8_AMD1.bit -of ../../outputData/audioOut8_AMD1.wav -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio8chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain9_AMD1.bit -of ../../outputData/audioOut9_AMD1.wav -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain10_AMD1.bit -of ../../outputData/audioOut10_AMD1_QMF64 -cp 38 -dt 2 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain11_AMD1.bit -of ../../outputData/audioOut11_AMD1.wav  -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/DrcGain12_AMD1.bit -of ../../outputData/audioOut12_AMD1_QMF64  -cp 38 -dt 2 -v 0 -af 2048
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain13_AMD1.bit -of ../../outputData/audioOut13_AMD1.wav -cp 39 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/DrcGain14_AMD1.bit -of ../../outputData/audioOut14_AMD1.wav -cp 38 -v 0

	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/ffDrcGain1_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness1_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc1_AMD1.bit -of ../../outputData/audioOut1_AMD1_ff.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/ffDrcGain2_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness2_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc2_AMD1.bit -of ../../outputData/audioOut2_AMD1_ff.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chModulated.wav -ib ../../../drcToolEncoder/TestDataOut/ffDrcGain3_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness3_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc3_AMD1.bit -of ../../outputData/audioOut3_AMD1_ff.wav -cp 1 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain8_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness8_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc8_AMD1.bit -of ../../outputData/audioOut8_AMD1_ff.wav -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio8chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain9_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness9_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc9_AMD1.bit -of ../../outputData/audioOut9_AMD1_ff.wav -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio5chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain10_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness10_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc10_AMD1.bit -of ../../outputData/audioOut10_AMD1_ff_QMF64 -cp 38 -dt 2 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain11_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness11_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc11_AMD1.bit -of ../../outputData/audioOut11_AMD1_ff.wav  -cp 38 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chConst_QMF64 	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain12_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness12_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc12_AMD1.bit -of ../../outputData/audioOut12_AMD1_ff_QMF64 -cp 38 -dt 2 -v 0 -af 2048
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain13_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness13_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc13_AMD1.bit -of ../../outputData/audioOut13_AMD1_ff.wav -cp 39 -v 0
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ib ../../../drcToolEncoder/TestDataOut/ffDrcGain14_AMD1.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness14_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc14_AMD1.bit -of ../../outputData/audioOut14_AMD1_ff.wav -cp 38 -v 0		
	./drcToolDecoder -if ../../TestDataIn/audio2chNoise.wav   	-ig ../../../drcToolEncoder/TestDataOut/ffDrcGain8_AMD1_split.bit -ffLoudness ../../../drcToolEncoder/TestDataOut/ffLoudness8_AMD1.bit -ffDrc ../../../drcToolEncoder/TestDataOut/ffDrc8_AMD1.bit -of ../../outputData/audioOut8_AMD1_ff_split.wav -cp 38 -v 0
fi

if [ $compare = yes ]
then
  GREPCOMMAND="grep -e 'File A = File B' -e 'WAVE file' -e 'Max Diff'"
  for i in 1 2 3 4 5 7 8 9 11 13 14
  do
    CompAudio ../../TestDataOut/audioOut${i}_AMD1.wav ../../outputData/audioOut${i}_AMD1.wav | eval $GREPCOMMAND
  done
  for i in 6 10 12
  do
    CompAudio ../../TestDataOut/audioOut${i}_AMD1_QMF64_real.qmf ../../outputData/audioOut${i}_AMD1_QMF64_real.qmf | eval $GREPCOMMAND
    CompAudio ../../TestDataOut/audioOut${i}_AMD1_QMF64_imag.qmf ../../outputData/audioOut${i}_AMD1_QMF64_imag.qmf | eval $GREPCOMMAND
  done
  for i in 1 2 3 8 9 11 13 14
  do
    CompAudio ../../TestDataOut/audioOut${i}_AMD1.wav ../../outputData/audioOut${i}_AMD1_ff.wav | eval $GREPCOMMAND
  done
  CompAudio ../../TestDataOut/audioOut8_AMD1.wav ../../outputData/audioOut8_AMD1_ff_split.wav | eval $GREPCOMMAND
  for i in 10 12
  do
    CompAudio ../../TestDataOut/audioOut${i}_AMD1_QMF64_real.qmf ../../outputData/audioOut${i}_AMD1_ff_QMF64_real.qmf | eval $GREPCOMMAND
    CompAudio ../../TestDataOut/audioOut${i}_AMD1_QMF64_imag.qmf ../../outputData/audioOut${i}_AMD1_ff_QMF64_imag.qmf | eval $GREPCOMMAND
  done  
fi

cd $currentDir
