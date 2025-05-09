@echo off

set currentDir=%~dp0
set buildSetting=Debug

set compare=yes

cd %currentDir%..\modules\uniDrcModules\uniDrcInterfaceEncoderCmdl\bin\%1

del ..\..\outputData\*.bit

for /D %%i in (1 37 38 39) do (
  uniDrcInterfaceEncoderCmdl -of ..\..\outputData\uniDrcInterface%%i_AMD1.bit -cfg2 %%i -v 0
)

if "%compare%" == "yes" (
  for /D %%i in (1 37 38 39) do (
    fc /B ..\..\TestDataOut\uniDrcInterface%%i_AMD1.bit ..\..\outputData\uniDrcInterface%%i_AMD1.bit >NUL && echo Files are identical || echo Files are different
  )
)

cd %currentDir%

pause()