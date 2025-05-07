@echo off

set currentDir=%~dp0
set drcDecoder=.\%1\drcToolDecoder.exe

set compare=yes
set uniDrcInterface=no

cd %currentDir%..\modules\drcTool\drcToolDecoder\bin\

del ..\outputData\*.wav
del ..\outputData\*.qmf
del ..\outputData\*.stft

if "%uniDrcInterface%" == "yes" (
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1_ld.bit -of ..\outputData\audioOut1_ld.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit  -dm 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines_QMF64 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1_QMF64  -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit  -dt 2 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines_STFT256 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1_STFT256 -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -dt 3 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain2.bit    -of ..\outputData\audioOut2.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain3.bit    -of ..\outputData\audioOut3.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain4.bit    -of ..\outputData\audioOut4.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface2.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut5.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface34.bit -v 0

	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut6.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface32.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut7.wav    -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface33.bit -v 0
	
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ic ..\..\drcToolEncoder\TestDataOut\uniDrcConfig1.bit -il ..\..\drcToolEncoder\TestDataOut\loudnessInfoSet1.bit -ig ..\..\drcToolEncoder\TestDataOut\uniDrcGain1.bit -of ..\outputData\audioOut1_splittedBs.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1.bit -v 0
) else (
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1.wav    -cp 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1_ld.bit -of ..\outputData\audioOut1_ld.wav -cp 1  -dm 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines_QMF64 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1_QMF64  -cp 1  -dt 2 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines_STFT256 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1.bit    -of ..\outputData\audioOut1_STFT256 -cp 1  -dt 3 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ib ..\..\drcToolEncoder\TestDataOut\DrcGain2.bit    -of ..\outputData\audioOut2.wav    -cp 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain3.bit    -of ..\outputData\audioOut3.wav    -cp 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain4.bit    -of ..\outputData\audioOut4.wav    -cp 2 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut5.wav    -cp 34 -v 0
	                                                                                                                                           
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut6.wav    -cp 32 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSinesWN.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6.bit    -of ..\outputData\audioOut7.wav    -cp 33 -v 0
	
	%drcDecoder% -if ..\TestDataIn\audio2chSines.wav   -ic ..\..\drcToolEncoder\TestDataOut\uniDrcConfig1.bit -il ..\..\drcToolEncoder\TestDataOut\loudnessInfoSet1.bit -ig ..\..\drcToolEncoder\TestDataOut\uniDrcGain1.bit -of ..\outputData\audioOut1_splittedBs.wav -cp 1 -v 0
)

set grepCommand=findstr /C:"File A = File B" /C:"Max Diff" /C:"WAVE file"
if "%compare%" == "yes" (
  for /D %%i in (1 1_ld 2 3 4 5) do (
    CompAudio ..\TestDataOut\audioOut%%i.wav ..\outputData\audioOut%%i.wav | %grepCommand%
  )
  CompAudio ..\TestDataOut\audioOut1_QMF64_real.qmf ..\outputData\audioOut1_QMF64_real.qmf | %grepCommand%
  CompAudio ..\TestDataOut\audioOut1_QMF64_imag.qmf ..\outputData\audioOut1_QMF64_imag.qmf | %grepCommand%
  CompAudio ..\TestDataOut\audioOut1_STFT256_real.stft ..\outputData\audioOut1_STFT256_real.stft | %grepCommand%
  CompAudio ..\TestDataOut\audioOut1_STFT256_imag.stft ..\outputData\audioOut1_STFT256_imag.stft | %grepCommand%
  CompAudio ..\outputData\audioOut6.wav ..\outputData\audioOut7.wav | %grepCommand%
  CompAudio ..\outputData\audioOut1.wav ..\outputData\audioOut1_splittedBs.wav | %grepCommand%
)

cd %currentDir%

pause()