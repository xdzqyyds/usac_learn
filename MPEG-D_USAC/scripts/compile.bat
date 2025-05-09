@echo off
setlocal enabledelayedexpansion

set currentDirUsac=%~dp0
set mp4Dir=%currentDirUsac%..\example
set compilerVS09="%VS90COMNTOOLS%\..\IDE\devenv.com"
set compilerVS11="%VS110COMNTOOLS%\..\IDE\devenv.com"
set compilerVS11x="MSBuild"
set platform=VS11
set target=Win32
set buildMode=Debug
set executeDecoding=1
set checkout=0
set vcmTool=SVN
set verbose=0

:: loop over arguments
:parseFlags
if "%1"=="" goto endParseFlags

if "%1"=="Release" set buildMode=Release
if "%1"=="VS2008" set platform=VS09
if "%1"=="VS2012x" set platform=VS11x
if "%1"=="x64" set target=x64
if "%1"=="nDec" set executeDecoding=0
if "%1"=="co" set checkout=1
if "%1"=="GIT" set vcmTool=GIT
if "%1"=="v" set verbose=1

shift
GOTO parseFlags
:endParseFlags

:: set compiler specific options
if "%platform%"=="VS11" (
  :: Visual Studio 2012 Professional
  set compile=%compilerVS11%
  set vsVersion=2012
  set vsVersionPostfix=_VS!vsVersion!
  set vsProjExt=vcxproj
) else (
  if "%platform%"=="VS11x" (
    :: Visual Studio 2012 Express
    set compile=%compilerVS11x%
    set vsVersion=2012
    set vsVersionPostfix=_VS!vsVersion!
    set vsProjExt=vcxproj
  ) else (
    :: Visual Studio 2008 Professional
    set compile=%compilerVS09%
    set vsVersion=2008
    set vsVersionPostfix=
    set vsProjExt=vcproj
  )
)

if "%platform%"=="VS11x" (
  :: Visual Studio Express
  set buildCommand=/t:Rebuild
  set buildSettingCO=/p:Configuration=%buildMode% /p:Platform=%target%
  set buildSettingDRC=/p:Configuration=%buildMode% /p:Platform=%target%
  set buildSettingAFsp=/p:Configuration=%buildMode% /p:Platform=%target%
) else (
  :: Visual Studio Professional
  set buildCommand=/Rebuild
  set buildSettingCO="%buildMode%|%target%"
  set buildSettingDRC="%buildMode%|%target%"
  set buildSettingAFsp="%buildMode%|%target%"
)

set flagsCO=%buildCommand% %buildSettingCO%
set flagsDRC=%buildCommand% %buildSettingDRC%
set flagsAFsp=%buildCommand% %buildSettingAFsp%

:: set directories and libtsplite library
if "%target%"=="x64" (
  set buildOutDir=x64\%buildMode%
  set libtsplite=libtsplite_x64.lib
) else (
  set buildOutDir="%buildMode%"
  set libtsplite=libtsplite.lib
)
set buildOutDirWin32="%target%_%buildMode%"
if "%platform%"=="VS11x" (
  set buildDirCleanString=%currentDirUsac%executables_%target%_%buildMode%_VS%vsVersion%express
) else (
  set buildDirCleanString=%currentDirUsac%executables_%target%_%buildMode%_VS%vsVersion%
)
set buildDir="%buildDirCleanString%"
set buildDirAFsp="%currentDirUsac%\..\tools\AFsp\lib"

:: echo flags and options
echo ******************* MPEG-D USAC Audio Coder - Edition 2.3 *********************
echo *                                                                             *
echo *                               Windows compiler                              *
echo *                                                                             *
echo *                                  %DATE%                                 *
echo *                                                                             *
echo *    This software may only be used in the development of the MPEG USAC Audio *
echo *    standard, ISO/IEC 23003-3 or in conforming implementations or products.  *
echo *                                                                             *
echo *******************************************************************************
echo.
if "%platform%"=="VS11x" (
  echo Using %platform% compiler [Microsoft Visual Studio %vsVersion% Express].
  echo.
  if "%target%"=="x64" (
    echo x64 build is not supported with Microsoft Visual Studio %vsVersion% Express.
    goto BUILDERROR
  )
  echo Setting environment for using Microsoft Visual Studio %vsVersion% Express.
  call "%VS110COMNTOOLS%\VsDevCmd.bat"
  echo.
) else (
  echo Using %platform% compiler [Microsoft Visual Studio %vsVersion%].
  echo.
)
echo compile flags CO:   %flagsCO%
echo compile flags DRC:  %flagsDRC%
echo compile flags AFsp: %flagsAFsp%
echo.

:: create output directories
if not exist %buildDir% (
  mkdir %buildDir%
) else (
  del /s /f %buildDir%\*.exe >nul 2>&1
)
if not exist %buildDirAFsp% (
  mkdir %buildDirAFsp%
) else (
  del /s /f %buildDirAFsp%\%libtsplite% >nul 2>&1
)

:: checkout IsoLib
if %checkout% EQU 1 (
  echo downloading IsoLib from GitHub...
  echo.
  call %currentDirUsac%\..\tools\IsoLib\scripts\checkout.bat -p v %vcmTool% > coIsoLib.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type coIsoLib.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
)

:: check add-on libraries
if not exist %currentDirUsac%\..\tools\AFsp\MSVC\MSVC.sln (
  echo AFsp MSVC.sln is not existing.
  echo Please download http://www.mmsp.ece.mcgill.ca/Documents/Downloads/AFsp/
  exit /b 1
)
if not exist %currentDirUsac%\..\mpegD_drc\MPEG_D_DRC_refsoft\scripts\compile.bat (
  echo MPEG-D DRC is not existing.
  echo Please add MPEG-D DRC reference software as in ISO/IEC 23003-4.
  exit /b 1
)
if not exist %currentDirUsac%\..\tools\IsoLib\libisomediafile\w32\libisomediafile\VS%vsVersion%\libisomedia.%vsProjExt% (
  echo ISOBMFF is not existing.
  echo Please download https://github.com/MPEGGroup/isobmff/archive/master.zip.
  exit /b 1
)

:: compile AFsp library
echo Compiling libtsplite
  if "%platform%"=="VS11x" (
    if not exist %currentDirUsac%\..\tools\AFsp\MSVC\libtsplite\libtsplite.vcxproj (
      echo Please use Visual Studio %vsVersion% Express to now upgrade and to create a %buildMode% configuration:
      echo.  %currentDirUsac%..\tools\AFsp\MSVC\libtsplite\libtsplite.vcproj
      echo.
      pause
    )
    %compile% %currentDirUsac%\..\tools\AFsp\MSVC\libtsplite\libtsplite.vcxproj %flagsAFsp% > libtsplite.log
    copy %currentDirUsac%\..\tools\AFsp\MSVC\lib\%libtsplite% %buildDirAFsp%\%libtsplite% >> libtsplite.log
    if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type libtsplite.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
  ) else (
    call "%currentDirUsac%..\tools\AFsp\scripts\compile.bat" VS%vsVersion% %target% %buildMode% > libtsplite.log
    if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type libtsplite.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
  )

:: compile MPEG-D USAC Audio modules
echo Compiling core Encoder
  %compile% %currentDirUsac%\..\mpegD_usac\usacEncDec\win32\usacEnc\usacEnc%vsVersionPostfix%.sln %flagsCO% > coreEnc.log
  copy %currentDirUsac%\..\mpegD_usac\usacEncDec\win32\usacEnc\bin\%buildOutDirWin32%\usacEnc.exe %buildDir%\ >> coreEnc.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type coreEnc.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

echo Compiling core Decoder
  %compile% %currentDirUsac%\..\mpegD_usac\usacEncDec\win32\usacDec\usacDec%vsVersionPostfix%.sln %flagsCO% > coreDec.log
  copy %currentDirUsac%\..\mpegD_usac\usacEncDec\win32\usacDec\bin\%buildOutDirWin32%\usacDec.exe %buildDir%\ >> coreDec.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type coreDec.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

echo Compiling wavCutter
  %compile% %currentDirUsac%\..\tools\wavCutter\wavCutterCmdl\make\wavCutterCmdl%vsVersionPostfix%.sln %flagsCO% > wavCutter.log
  copy %currentDirUsac%\..\tools\wavCutter\wavCutterCmdl\make\%buildOutDir%\wavCutterCmdl.exe %buildDir%\ >> wavCutter.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type wavCutter.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

:: compile VS2012 only projects
if not "%platform%"=="VS09" (
  echo Compiling ssnrcd
    %compile% %currentDirUsac%\..\tools\ssnrcd\make\ssnrcd%vsVersionPostfix%.sln %flagsCO% > ssnrcd.log
    copy %currentDirUsac%\..\tools\ssnrcd\make\%buildOutDir%\ssnrcd\ssnrcd.exe %buildDir%\ >> ssnrcd.log
    if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type ssnrcd.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
)

:: compile DRC decoder
echo Compiling drcCoder
  cd %currentDirUsac%\..\mpegD_drc\MPEG_D_DRC_refsoft\scripts\
  if "%platform%"=="VS11x" (
    call compile AMD1 WRITE_FRAMESIZE %target% %buildMode% %buildCommand% silent VS%vsVersion%x > DrcTool.log
  ) else (
    call compile AMD1 WRITE_FRAMESIZE %target% %buildMode% %buildCommand% silent VS%vsVersion% > DrcTool.log
  )
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type DrcTool.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

  copy %currentDirUsac%\..\mpegD_drc\MPEG_D_DRC_refsoft\modules\drcTool\drcToolDecoder\bin\Windows_AMD64\drcToolDecoder.exe %buildDir%\ >> DrcTool.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type DrcTool.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

  copy %currentDirUsac%\..\mpegD_drc\MPEG_D_DRC_refsoft\modules\drcTool\drcToolEncoder\bin\Windows_AMD64\drcToolEncoder.exe %buildDir%\ >> DrcTool.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type DrcTool.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

  copy %currentDirUsac%\..\mpegD_drc\MPEG_D_DRC_refsoft\scripts\DrcTool.log %currentDirUsac%\DrcTool.log 1>nul
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type DrcTool.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))

:: perform example decoding of mp4 bitstreams
if %executeDecoding% EQU 1 (
  cd %currentDirUsac%
  echo.
  echo Decoding example__C1_3_FD.mp4
  %buildDir%\usacDec.exe -if %mp4Dir%\example__C1_3_FD.mp4 -of %mp4Dir%\example__C1_3_FD.wav > decoding.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type decoding.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
  echo Decoding example__C2_3_FD.mp4
  %buildDir%\usacDec.exe -if %mp4Dir%\example__C2_3_FD.mp4 -of %mp4Dir%\example__C2_3_FD.wav >> decoding.log
  if %errorlevel% NEQ 0 ( if %verbose% EQU 1 ( type decoding.log 2>nul & goto BUILDERROR ) else ( goto BUILDERROR ))
)

goto END

:BUILDERROR
echo.
echo       Build failed. Check corresponding log file for more information.
exit /B 1

:RUNERROR
echo.
echo       Decoding failed. Check corresponding log file for more information.
exit /B 1

:END
echo.
echo All executables are collected in:
echo %buildDirCleanString%\
if %executeDecoding% NEQ 0 (
  echo.
  echo All decoded .wav files are collected in:
  echo %mp4Dir%\
)
echo.
echo       All Build successful.
exit /B 0
