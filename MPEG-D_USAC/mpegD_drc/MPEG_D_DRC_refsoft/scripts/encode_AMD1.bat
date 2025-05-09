@echo off

set currentDir=%~dp0
set drcEncoder=.\bin\%1\drcToolEncoder.exe

set compare=yes

cd %currentDir%..\modules\drcTool\drcToolEncoder\
del .\outputData\*_AMD1.bit

%drcEncoder% 7 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain1_AMD1.bit
%drcEncoder% 8 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain2_AMD1.bit
%drcEncoder% 9 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain3_AMD1.bit
%drcEncoder% 10 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain4_AMD1.bit
%drcEncoder% 11 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain5_AMD1.bit
%drcEncoder% 12 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain6_AMD1.bit
%drcEncoder% 13 .\TestDataIn\DrcGain1vector_AMD1.dat .\outputData\DrcGain7_AMD1.bit
%drcEncoder% 14 .\TestDataIn\DrcGain1vectorA_AMD1.dat .\outputData\DrcGain8_AMD1.bit
%drcEncoder% 15 .\TestDataIn\DrcGain3vectors_AMD1.dat .\outputData\DrcGain9_AMD1.bit
%drcEncoder% 16 .\TestDataIn\DrcGain3vectors_AMD1.dat .\outputData\DrcGain10_AMD1.bit
%drcEncoder% 17 .\TestDataIn\DrcGain1vectorA_AMD1.dat .\outputData\DrcGain11_AMD1.bit
%drcEncoder% 18 .\TestDataIn\DrcGain1vectorA_AMD1.dat .\outputData\DrcGain12_AMD1.bit
%drcEncoder% 19 dummy.dat                             .\outputData\DrcGain13_AMD1.bit
%drcEncoder% 20 .\TestDataIn\DrcGain1vectorA_AMD1.dat .\outputData\DrcGain14_AMD1.bit

if "%compare%" == "yes" (
  for /D %%i in (1 2 3 4 5 6 7 8 9 10 11 12 13 14) do (
    fc /b .\TestDataOut\DrcGain%%i_AMD1.bit .\outputData\DrcGain%%i_AMD1.bit >NUL && echo Files are identical || echo Files are different
  )
)

cd %currentDir%

pause()