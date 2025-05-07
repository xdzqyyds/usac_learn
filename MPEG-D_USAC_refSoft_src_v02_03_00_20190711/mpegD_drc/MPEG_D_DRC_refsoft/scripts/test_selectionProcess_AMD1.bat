@echo off

set currentDir=%~dp0

set compare=yes
set MPEGH=no

if "%1" == "MPEGH" (
set MPEGH=yes
)

cd %currentDir%..\modules\uniDrcModules\uniDrcSelectionProcessCmdl\bin\%1
del ..\..\outputData\*.txt

uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain1_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -of ..\..\outputData\selProcTransferData1_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain2_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -of ..\..\outputData\selProcTransferData2_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain3_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface1_AMD1.bit -of ..\..\outputData\selProcTransferData3_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain4_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -of ..\..\outputData\selProcTransferData4_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain5_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -of ..\..\outputData\selProcTransferData5_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain6_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface37_AMD1.bit -of ..\..\outputData\selProcTransferData6_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain7_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface39_AMD1.bit -of ..\..\outputData\selProcTransferData7_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain8_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -of ..\..\outputData\selProcTransferData8_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain9_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -of ..\..\outputData\selProcTransferData9_AMD1.txt -v 0
uniDrcSelectionProcessCmdl -if ..\..\..\..\drcTool\drcToolEncoder\TestDataOut\DrcGain10_AMD1.bit -ii ..\..\..\uniDrcInterfaceEncoderCmdl\TestDataOut\uniDrcInterface38_AMD1.bit -of ..\..\outputData\selProcTransferData10_AMD1.txt -v 0

if "%compare%" == "yes" (
  for /L %%i in (1,1,10) do (
    fc /L ..\..\TestDataOut\selProcTransferData%%i_AMD1.txt ..\..\outputData\selProcTransferData%%i_AMD1.txt
  )  
)

cd %currentDir%

pause()
