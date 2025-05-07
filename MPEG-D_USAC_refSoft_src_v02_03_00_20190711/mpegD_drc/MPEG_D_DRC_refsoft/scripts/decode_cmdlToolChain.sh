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

MPEGH=no
if [ -n $2 ]; then
	if [ "$2" = "MPEGH" ]
	then
		MPEGH=yes
	fi
fi

currentDir=$(pwd)
compare=yes
gainsOnly=no

cd ../modules/uniDrcModules/uniDrcSelectionProcessCmdl/outputData
find . -name '*.txt' -type f -delete
cd $currentDir
cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData
find . -name '*.wav' -type f -delete
find . -name '*.qmf' -type f -delete
find . -name '*.stft' -type f -delete
cd $currentDir

if [ $MPEGH = yes ]
then
	# test points 
	# selection process
	cd ../modules/uniDrcModules/uniDrcSelectionProcessCmdl/bin/${binaryFolder}
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 9 -of ../../outputData/mpegh3daSelProcTransferData9.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -cp 10 -of ../../outputData/mpegh3daSelProcTransferData10.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -cp 19 -of ../../outputData/mpegh3daSelProcTransferData19.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -cp 20 -of ../../outputData/mpegh3daSelProcTransferData20.txt -v 0
	cd $currentDir
	
	# gains + application 
	cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/bin/${binaryFolder}
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio5chSinesWN.wav -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData9.txt -of ../../outputData/audioOut1_mpegh3da_chain.wav -decId 0 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio5chSinesWN.wav -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_chain.wav -decId 0 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio5chSinesWN_QMF64 -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_chain_QMF64 -decId 0 -gdt 1 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio5chSinesWN_STFT256 -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_chain_STFT256 -decId 0 -gdt 2 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chNoise.wav -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData19.txt -of ../../outputData/audioOut3_mpegh3da_chain.wav -decId 3 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chNoise.wav -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData20.txt -of ../../outputData/audioOut4_mpegh3da_chain.wav -decId 2 -v 0

	cd $currentDir
	
	# QMF64
	#cd ../tools/qmflib/make/bin/${binaryFolder}
	#./qmflib_example -if ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_chain_QMF64 -of ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_chain_QMF64.wav -outputBitdepth 32
	#cd $currentDir
	
	# STFT256
	#cd ../tools/fftlib/make/bin/${binaryFolder}
	#./fftlib_example -if ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_chain_STFT256 -of ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_chain_STFT256.wav
	#cd $currentDir

	if [ $gainsOnly = yes ]
	then
		# gains only decoding (for chosen sequences)
		cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/bin/${binaryFolder}
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData9.txt -of ../../outputData/audioOut1_mpegh3da_gainsOnly.wav -decId 0 -gdt 3 -acc 5 -v 0
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_gainsOnly.wav -decId 0 -gdt 3 -acc 5 -v 0
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_gainsOnly_QMF64 -decId 0 -gdt 4 -acc 5 -v 0
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig2.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet2.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain2.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet2.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData10.txt -of ../../outputData/audioOut2_mpegh3da_gainsOnly_STFT256 -decId 0 -gdt 5 -acc 5 -v 0
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData19.txt -of ../../outputData/audioOut3_mpegh3da_gainsOnly.wav -decId 3 -gdt 3 -acc 2 -v 0
		./uniDrcGainDecoderCmdl -ic ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig4.bit -il ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet4.bit -ig ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcGain4.bit -dms ../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daDownmixMatrixSet4.txt -is ../../../uniDrcSelectionProcessCmdl/outputData/mpegh3daSelProcTransferData20.txt -of ../../outputData/audioOut4_mpegh3da_gainsOnly.wav -decId 2 -gdt 3 -acc 2 -v 0
		cd $currentDir
		
		# QMF64
		#cd ../tools/qmflib/make/bin/${binaryFolder}
		#./qmflib_example -if ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_gainsOnly_QMF64 -of ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_gainsOnly_QMF64.wav -outputBitdepth 32
		#cd $currentDir
	
		# STFT256
		#cd ../tools/fftlib/make/bin/${binaryFolder}
		#./fftlib_example -if ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_gainsOnly_STFT256 -of ../../../../../modules/uniDrcModules/uniDrcGainDecoderCmdl/outputData/audioOut2_mpegh3da_gainsOnly_STFT256.wav
		#cd $currentDir		
	fi
else
	# test points 
	# selection process
	cd ../modules/uniDrcModules/uniDrcSelectionProcessCmdl/bin/${binaryFolder}
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData1.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData2.txt -v 0
	./uniDrcSelectionProcessCmdl -if ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3.bit -ii ../../../uniDrcInterfaceEncoderCmdl/TestDataOut/uniDrcInterface1.bit -of ../../outputData/selProcTransferData3.txt -v 0
	cd $currentDir
	
	# gains + application 
	cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/bin/${binaryFolder}
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chSines.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData1.txt -of ../../outputData/audioOut1_chain.wav -decId 0 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chSines.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1_ld.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData1.txt -of ../../outputData/audioOut1_ld_chain.wav -decId 0 -dm 1 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chSines_QMF64 -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData1.txt -of ../../outputData/audioOut1_chain_QMF64 -decId 0 -gdt 1 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chSines_STFT256 -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData1.txt -of ../../outputData/audioOut1_chain_STFT256 -decId 0 -gdt 2 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio2chSines.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData2.txt -of ../../outputData/audioOut2_chain_tmp.wav -decId 0 -v 0
	./uniDrcGainDecoderCmdl -if ../../outputData/audioOut2_chain_tmp.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData2.txt -of ../../outputData/audioOut2_chain.wav -decId 4 -v 0
	./uniDrcGainDecoderCmdl -if ../../../../drcTool/drcToolDecoder/TestDataIn/audio5chSinesWN.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData3.txt -of ../../outputData/audioOut3_chain_tmp.wav -decId 0 -v 0
	./uniDrcGainDecoderCmdl -if ../../outputData/audioOut3_chain_tmp.wav -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData3.txt -of ../../outputData/audioOut3_chain.wav -decId 4 -v 0
	cd $currentDir

	# gains only decoding (for chosen sequences)
	if [ $gainsOnly = yes ]
	then
		cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/bin/${binaryFolder}
		./uniDrcGainDecoderCmdl -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain1.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData1.txt -of ../../outputData/audioOut1_gainsOnly.wav -decId 0 -gdt 3 -acc 2 -v 0
		./uniDrcGainDecoderCmdl -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain2.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData2.txt -of ../../outputData/audioOut2_gainsOnly.wav -decId 0 -gdt 3 -acc 2 -v 0
		./uniDrcGainDecoderCmdl -ib ../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain3.bit -is ../../../uniDrcSelectionProcessCmdl/outputData/selProcTransferData3.txt -of ../../outputData/audioOut3_gainsOnly.wav -decId 0 -gdt 3 -acc 5 -v 0
		cd $currentDir
	fi
fi

if [ $compare = yes ]
then
  GREPCOMMAND="grep -e 'File A = File B' -e 'WAVE file' -e 'Max Diff'"
  cd ../modules/uniDrcModules/uniDrcGainDecoderCmdl/bin/${binaryFolder}
  if [ $MPEGH = yes ]
  then
    for i in 1 2 3 4
    do
      CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut${i}_mpegh3da.wav ../../outputData/audioOut${i}_mpegh3da_chain.wav | eval $GREPCOMMAND
    done
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut2_mpegh3da_QMF64_real.qmf ../../outputData/audioOut2_mpegh3da_chain_QMF64_real.qmf | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut2_mpegh3da_QMF64_imag.qmf ../../outputData/audioOut2_mpegh3da_chain_QMF64_imag.qmf | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut2_mpegh3da_STFT256_real.stft ../../outputData/audioOut2_mpegh3da_chain_STFT256_real.stft | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut2_mpegh3da_STFT256_imag.stft ../../outputData/audioOut2_mpegh3da_chain_STFT256_imag.stft | eval $GREPCOMMAND
  else
    for i in 1 1_ld 2 3
    do
      CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut${i}.wav ../../outputData/audioOut${i}_chain.wav | eval $GREPCOMMAND
    done
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut1_QMF64_real.qmf ../../outputData/audioOut1_chain_QMF64_real.qmf | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut1_QMF64_imag.qmf ../../outputData/audioOut1_chain_QMF64_imag.qmf | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut1_STFT256_real.stft ../../outputData/audioOut1_chain_STFT256_real.stft | eval $GREPCOMMAND
    CompAudio ../../../../drcTool/drcToolDecoder/TestDataOut/audioOut1_STFT256_imag.stft ../../outputData/audioOut1_chain_STFT256_imag.stft | eval $GREPCOMMAND
  fi
  cd $currentDir
fi

