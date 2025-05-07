@echo off

set currentDir=%~dp0
set drcDecoder=.\%1\drcToolDecoder.exe

set compare=yes
set uniDrcInterface=no

cd %currentDir%..\modules\drcTool\drcToolDecoder\bin

del ..\outputData\*_AMD1.wav

if "%uniDrcInterface%" == "yes" (
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1_AMD1.bit -of ..\outputData\audioOut1_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain2_AMD1.bit -of ..\outputData\audioOut2_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain3_AMD1.bit -of ..\outputData\audioOut3_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -v 0	
	%drcDecoder% -if ..\TestDataIn\audio2chSweep.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain4_AMD1.bit -of ..\outputData\audioOut4_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSweep.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain5_AMD1.bit -of ..\outputData\audioOut5_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chConst_QMF64.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6_AMD1.bit -of ..\outputData\audioOut6_AMD1_QMF64.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -dt 2 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain7_AMD1.bit -of ..\outputData\audioOut7_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface39_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain8_AMD1.bit -of ..\outputData\audioOut8_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio8chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain9_AMD1.bit -of ..\outputData\audioOut9_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chConst_QMF64.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain10_AMD1.bit -of ..\outputData\audioOut10_AMD1_QMF64.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -dt 2 -v 0	
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain11_AMD1.bit -of ..\outputData\audioOut11_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chConst_QMF64.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain12_AMD1.bit -of ..\outputData\audioOut12_AMD1_QMF64.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -dt 2 -v 0 -af 2048
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain13_AMD1.bit -of ..\outputData\audioOut13_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface39_AMD1.bit -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain14_AMD1.bit -of ..\outputData\audioOut14_AMD1.wav -ii ..\..\..\uniDrcModules\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -v 0

) else (                                                                                                                               
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain1_AMD1.bit -of ..\outputData\audioOut1_AMD1.wav -cp 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain2_AMD1.bit -of ..\outputData\audioOut2_AMD1.wav -cp 1 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain3_AMD1.bit -of ..\outputData\audioOut3_AMD1.wav -cp 1 -v 0	
	%drcDecoder% -if ..\TestDataIn\audio2chSweep.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain4_AMD1.bit -of ..\outputData\audioOut4_AMD1.wav -cp 37 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chSweep.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain5_AMD1.bit -of ..\outputData\audioOut5_AMD1.wav -cp 37 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chConst_QMF64 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain6_AMD1.bit -of ..\outputData\audioOut6_AMD1_QMF64 -cp 37 -dt 2 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chModulated.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain7_AMD1.bit -of ..\outputData\audioOut7_AMD1.wav -cp 39 -v 0	
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain8_AMD1.bit -of ..\outputData\audioOut8_AMD1.wav -cp 38 -v 0
	%drcDecoder% -if ..\TestDataIn\audio8chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain9_AMD1.bit -of ..\outputData\audioOut9_AMD1.wav -cp 38 -v 0
	%drcDecoder% -if ..\TestDataIn\audio5chConst_QMF64 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain10_AMD1.bit -of ..\outputData\audioOut10_AMD1_QMF64 -cp 38 -dt 2 -v 0	
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain11_AMD1.bit -of ..\outputData\audioOut11_AMD1.wav -cp 38 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chConst_QMF64 -ib ..\..\drcToolEncoder\TestDataOut\DrcGain12_AMD1.bit -of ..\outputData\audioOut12_AMD1_QMF64 -cp 38 -dt 2 -v 0 -af 2048
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain13_AMD1.bit -of ..\outputData\audioOut13_AMD1.wav -cp 39 -v 0
	%drcDecoder% -if ..\TestDataIn\audio2chNoise.wav -ib ..\..\drcToolEncoder\TestDataOut\DrcGain14_AMD1.bit -of ..\outputData\audioOut14_AMD1.wav -cp 38 -v 0
)

set grepCommand=findstr /C:"File A = File B" /C:"Max Diff" /C:"WAVE file"
if "%compare%" == "yes" (
  for /D %%i in (1 2 3 4 5 7 8 9 11 13 14) do (
    CompAudio ..\TestDataOut\audioOut%%i_AMD1.wav ..\outputData\audioOut%%i_AMD1.wav | %grepCommand%
  )
  for /D %%i in (6 10 12) do (
    CompAudio ..\TestDataOut\audioOut%%i_AMD1_QMF64_real.qmf ..\outputData\audioOut%%i_AMD1_QMF64_real.qmf | %grepCommand%
    CompAudio ..\TestDataOut\audioOut%%i_AMD1_QMF64_imag.qmf ..\outputData\audioOut%%i_AMD1_QMF64_imag.qmf | %grepCommand%
  )
)

cd %currentDir%

pause()