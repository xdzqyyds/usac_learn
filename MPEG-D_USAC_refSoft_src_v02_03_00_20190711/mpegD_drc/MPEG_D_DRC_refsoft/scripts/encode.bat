@echo off

set currentDir=%~dp0
set drcEncoder=.\bin\%1\drcToolEncoder.exe

set compare=yes

cd %currentDir%..\modules\drcTool\drcToolEncoder\

del .\outputData\*.bit

if "%2" == "MPEGH" (
  %drcEncoder% 1 .\TestDataIn\DrcGain8vectors.dat .\outputData\mpegh3daUniDrcConfig1.bit .\outputData\mpegh3daLoudnessInfoSet1.bit .\outputData\mpegh3daUniDrcGain1.bit
  %drcEncoder% 2 .\TestDataIn\DrcGain4vectors.dat .\outputData\mpegh3daUniDrcConfig2.bit .\outputData\mpegh3daLoudnessInfoSet2.bit .\outputData\mpegh3daUniDrcGain2.bit
  %drcEncoder% 3 .\TestDataIn\DrcGain1vector.dat .\outputData\mpegh3daUniDrcConfig3.bit .\outputData\mpegh3daLoudnessInfoSet3.bit .\outputData\mpegh3daUniDrcGain3.bit
  %drcEncoder% 4 .\TestDataIn\DrcGain7vectors.dat .\outputData\mpegh3daUniDrcConfig4.bit .\outputData\mpegh3daLoudnessInfoSet4.bit .\outputData\mpegh3daUniDrcGain4.bit
) else (
  %drcEncoder% 1 .\TestDataIn\DrcGain1vector.dat  .\outputData\DrcGain1.bit
  %drcEncoder% 1_ld .\TestDataIn\DrcGain1vector.dat  .\outputData\DrcGain1_ld.bit
  %drcEncoder% 2 .\TestDataIn\DrcGain3vectors.dat .\outputData\DrcGain2.bit
  %drcEncoder% 3 .\TestDataIn\DrcGain7vectors.dat .\outputData\DrcGain3.bit
  %drcEncoder% 4 .\TestDataIn\DrcGain7vectors.dat .\outputData\DrcGain4.bit
  %drcEncoder% 5 .\TestDataIn\DrcGain8vectors.dat .\outputData\DrcGain5.bit
  %drcEncoder% 6 .\TestDataIn\DrcGain8vectors.dat .\outputData\DrcGain6.bit

  %drcEncoder% 1 .\TestDataIn\DrcGain1vector.dat  .\outputData\uniDrcConfig1.bit .\outputData\loudnessInfoSet1.bit .\outputData\uniDrcGain1.bit
)

if "%compare%" == "yes" (
  if "%2" == "MPEGH" (
    for /D %%i in (1 2 3 4) do (
      fc /b .\TestDataOut\mpegh3daUniDrcConfig%%i.bit .\outputData\mpegh3daUniDrcConfig%%i.bit >NUL && echo Files are identical || echo Files are different
      fc /b .\TestDataOut\mpegh3daLoudnessInfoSet%%i.bit .\outputData\mpegh3daLoudnessInfoSet%%i.bit >NUL && echo Files are identical || echo Files are different
      fc /b .\TestDataOut\mpegh3daUniDrcGain%%i.bit .\outputData\mpegh3daUniDrcGain%%i.bit >NUL && echo Files are identical || echo Files are different
    )
  ) else (
    for /D %%i in (1 1_ld 2 3 4 5 6) do (
      fc /b .\TestDataOut\DrcGain%%i.bit .\outputData\DrcGain%%i.bit >NUL && echo Files are identical || echo Files are different
    )
    for /D %%i in (1) do (
      fc /b .\TestDataOut\uniDrcConfig%%i.bit .\outputData\uniDrcConfig%%i.bit >NUL && echo Files are identical || echo Files are different
      fc /b .\TestDataOut\loudnessInfoSet%%i.bit .\outputData\loudnessInfoSet%%i.bit >NUL && echo Files are identical || echo Files are different
      fc /b .\TestDataOut\uniDrcGain%%i.bit .\outputData\uniDrcGain%%i.bit >NUL && echo Files are identical || echo Files are different
    )
  )
)

cd %currentDir%

pause()