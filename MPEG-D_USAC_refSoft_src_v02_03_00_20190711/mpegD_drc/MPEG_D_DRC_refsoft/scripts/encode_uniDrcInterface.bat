@echo off

set currentDir=%~dp0

set compare=yes
set MPEGH=no

if "%2" == "MPEGH" (
set MPEGH=yes
)

cd %currentDir%..\modules\uniDrcModules\uniDrcInterfaceEncoderCmdl\bin\%1

del ..\..\outputData\*.bit

if "%MPEGH%" == "yes" (
  for /L %%i in (1,1,20) do (
	  uniDrcInterfaceEncoderCmdl -of ..\..\outputData\mpegh3daUniDrcInterface%%i.bit -mpegh3daParams ..\..\outputData\mpegh3daParams%%i.txt -cfg2 %%i -v 0
	)
) else (
  for /L %%i in (1,1,36) do (
	  uniDrcInterfaceEncoderCmdl -of ..\..\outputData\uniDrcInterface%%i.bit -cfg2 %%i -v 0
	)
)

if "%compare%" == "yes" (
  if "%MPEGH%" == "yes" (
    for /L %%i in (1,1,20) do (
      fc /B ..\..\TestDataOut\mpegh3daUniDrcInterface%%i.bit ..\..\outputData\mpegh3daUniDrcInterface%%i.bit >NUL && echo Files are identical || echo Files are different
      fc /B ..\..\TestDataOut\mpegh3daParams%%i.txt ..\..\outputData\mpegh3daParams%%i.txt >NUL && echo Files are identical || echo Files are different

    )
  ) else (
    for /L %%i in (1,1,36) do (
      fc /B ..\..\TestDataOut\uniDrcInterface%%i.bit ..\..\outputData\uniDrcInterface%%i.bit >NUL && echo Files are identical || echo Files are different
    )
  )
)

cd %currentDir%

pause()