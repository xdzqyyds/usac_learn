@echo off

set currentDir=%~dp0
set uniDrcSelectionProcessCmdl=.\%1\uniDrcSelectionProcessCmdl.exe
set uniDrcGainDecoderCmdl=.\%1\uniDrcGainDecoderCmdl.exe

set compare=yes
set gainsOnly=no

cd %currentDir%..\modules\uniDrcModules\uniDrcSelectionProcessCmdl\bin\
del ..\outputData\*.txt
cd %currentDir%..\modules\uniDrcModules\uniDrcGainDecoderCmdl\bin\
del ..\outputData\*.wav
del ..\outputData\*.qmf
del ..\outputData\*.stft

if "%2" == "MPEGH" (
	cd %currentDir%..\modules\uniDrcModules\uniDrcSelectionProcessCmdl\bin\
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -cp 9 -of ..\outputData\mpegh3daSelProcTransferData9.txt -v 0
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -cp 10 -of ..\outputData\mpegh3daSelProcTransferData10.txt -v 0
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig4.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet4.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet4.txt -cp 19 -of ..\outputData\mpegh3daSelProcTransferData19.txt -v 0
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig4.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet4.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet4.txt -cp 20 -of ..\outputData\mpegh3daSelProcTransferData20.txt -v 0
	
	cd %currentDir%..\modules\uniDrcModules\uniDrcGainDecoderCmdl\bin\
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio5chSinesWN.wav -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData9.txt -of ..\outputData\audioOut1_mpegh3da_chain.wav -decId 0 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio5chSinesWN.wav -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData10.txt -of ..\outputData\audioOut2_mpegh3da_chain.wav -decId 0 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio5chSinesWN_QMF64 -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData10.txt -of ..\outputData\audioOut2_mpegh3da_chain_QMF64 -decId 0 -gdt 1 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio5chSinesWN_STFT256 -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig2.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet2.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain2.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet2.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData10.txt -of ..\outputData\audioOut2_mpegh3da_chain_STFT256 -decId 0 -gdt 2 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chNoise.wav -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig4.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet4.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain4.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet4.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData19.txt -of ..\outputData\audioOut3_mpegh3da_chain.wav -decId 3 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chNoise.wav -ic ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcConfig4.bit -il ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daLoudnessInfoSet4.bit -ig ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daUniDrcGain4.bit -dms ..\..\..\drcTool\drcToolEncoder\TestDataOut\mpegh3daDownmixMatrixSet4.txt -is ..\..\uniDrcSelectionProcessCmdl\outputData\mpegh3daSelProcTransferData20.txt -of ..\outputData\audioOut4_mpegh3da_chain.wav -decId 2 -v 0

) else (
	cd %currentDir%..\modules\uniDrcModules\uniDrcSelectionProcessCmdl\bin\
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1.bit -ii ..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -of ..\outputData\selProcTransferData1.txt -v 0
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain2.bit -ii ..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -of ..\outputData\selProcTransferData2.txt -v 0
	%uniDrcSelectionProcessCmdl% -if ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain3.bit -ii ..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -of ..\outputData\selProcTransferData3.txt -v 0
	
	cd %currentDir%..\modules\uniDrcModules\uniDrcGainDecoderCmdl\bin\
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chSines.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData1.txt -of ..\outputData\audioOut1_chain.wav -decId 0 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chSines.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1_ld.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData1.txt -of ..\outputData\audioOut1_ld_chain.wav -decId 0 -dm 1 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chSines_QMF64 -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData1.txt -of ..\outputData\audioOut1_chain_QMF64 -decId 0 -gdt 1 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chSines_STFT256 -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData1.txt -of ..\outputData\audioOut1_chain_STFT256 -decId 0 -gdt 2 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio2chSines.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain2.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData2.txt -of ..\outputData\audioOut2_chain_tmp.wav -decId 0 -v 0
	%uniDrcGainDecoderCmdl% -if ..\outputData\audioOut2_chain_tmp.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain2.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData2.txt -of ..\outputData\audioOut2_chain.wav -decId 4 -v 0
	%uniDrcGainDecoderCmdl% -if ..\..\..\drcTool\drcToolDecoder\TestDataIn\audio5chSinesWN.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain3.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData3.txt -of ..\outputData\audioOut3_chain_tmp.wav -decId 0 -v 0
	%uniDrcGainDecoderCmdl% -if ..\outputData\audioOut3_chain_tmp.wav -ib ..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain3.bit -is ..\..\uniDrcSelectionProcessCmdl\outputData\selProcTransferData3.txt -of ..\outputData\audioOut3_chain.wav -decId 4
)

set grepCommand=findstr /C:"File A = File B" /C:"Max Diff" /C:"WAVE file"
if "%compare%" == "yes" (
  cd %currentDir%..\modules\uniDrcModules\uniDrcGainDecoderCmdl\bin\
  if "%2" == "MPEGH" (
    for /D %%i in (1 2 3 4) do (
      CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut%%i_mpegh3da.wav ..\outputData\audioOut%%i_mpegh3da_chain.wav | %grepCommand%
    )
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut2_mpegh3da_QMF64_real.qmf ..\outputData\audioOut2_mpegh3da_chain_QMF64_real.qmf | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut2_mpegh3da_QMF64_imag.qmf ..\outputData\audioOut2_mpegh3da_chain_QMF64_imag.qmf | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut2_mpegh3da_STFT256_real.stft ..\outputData\audioOut2_mpegh3da_chain_STFT256_real.stft | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut2_mpegh3da_STFT256_imag.stft ..\outputData\audioOut2_mpegh3da_chain_STFT256_imag.stft | %grepCommand%
  ) else (
    for /D %%i in (1 1_ld 2 3) do (
      CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut%%i.wav ..\outputData\audioOut%%i_chain.wav | %grepCommand%
    )
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut1_QMF64_real.qmf ..\outputData\audioOut1_chain_QMF64_real.qmf | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut1_QMF64_imag.qmf ..\outputData\audioOut1_chain_QMF64_imag.qmf | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut1_STFT256_real.stft ..\outputData\audioOut1_chain_STFT256_real.stft | %grepCommand%
    CompAudio ..\..\..\drcTool\drcToolDecoder\TestDataOut\audioOut1_STFT256_imag.stft ..\outputData\audioOut1_chain_STFT256_imag.stft | %grepCommand%
  )
)

cd %currentDir%

pause()