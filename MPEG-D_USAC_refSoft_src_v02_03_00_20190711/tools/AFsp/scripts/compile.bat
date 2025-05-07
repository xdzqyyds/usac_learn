@echo off
setlocal enabledelayedexpansion

set "currentDir3daAFsp=%~dp0"
set platform=VS11
set target=x86
set /a cntOK=0
set buildMode=Release

::loop over arguments
:parseFlags
if "%1"=="" goto endParseFlags

if "%1"=="Release" set buildMode=Release
if "%1"=="Debug" set buildMode=Debug
if "%1"=="VS2008" set platform=VS90
if "%1"=="x64" set target=x64

shift
GOTO parseFlags
:endParseFlags

::set Microsoft Visual Studio Common Tools variable
if "%platform%"=="VS11" (
  set "VScommonTools3da=%VS110COMNTOOLS%"
  set vsVersion=2012
) else (
  set "VScommonTools3da=%VS90COMNTOOLS%"
  set vsVersion=2008
)

::set directories
if "%target%"=="x64" (
  if "%buildMode%"=="Debug" (
    set libtsp_library=libtsplite_MTd_x64.lib
    set libAO_library=libAO_MTd_x64.lib
  ) else (
    set libtsp_library=libtsplite_x64.lib
    set libAO_library=libAO_x64.lib
  )
) else (
  if "%buildMode%"=="Debug" (
    set libtsp_library=libtsplite_MTd.lib
    set libAO_library=libAO_MTd.lib
  ) else (
    set libtsp_library=libtsplite.lib
    set libAO_library=libAO.lib
  )
)

set libtspOutDir=%currentDir3daAFsp%\..\lib
set libAOOutDir=%currentDir3daAFsp%\..\MSVC\lib
set ResampAudioOutDir=%currentDir3daAFsp%\..\MSVC\bin

set libtspDir=%currentDir3daAFsp%\libtsp_%target%_VS%vsVersion%
set libAODir=%currentDir3daAFsp%\libAO_%target%_VS%vsVersion%
set ResampAudioDir=%currentDir3daAFsp%\ResampAudio_%target%_VS%vsVersion%

set "libtsp_library_path=%libtspOutDir%\%libtsp_library%"
set "libAO_library_path=%libAOOutDir%\%libAO_library%"
set "ResampAudio_binary_path=%ResampAudioOutDir%\ResampAudio.exe"

set "libtsp_src_path==%currentDir3daAFsp%\..\libtsp"
set "libAO_src_path==%currentDir3daAFsp%\..\libAO"
set "ResampAudio_src_path==%currentDir3daAFsp%\..\audio\ResampAudio"

::set compiler and linker executables
set cl_x86=%VScommonTools3da%..\..\VC\bin\cl.exe
set cl_x64=%VScommonTools3da%..\..\VC\bin\amd64\cl.exe

set lib_x86=%VScommonTools3da%..\..\VC\bin\lib.exe
set lib_x64=%VScommonTools3da%..\..\VC\bin\amd64\lib.exe

set link_x86=%VScommonTools3da%..\..\VC\bin\link.exe
set link_x64=%VScommonTools3da%..\..\VC\bin\amd64\link.exe

::set compiler and linker specific options
if "%buildMode%"=="Debug" (
  set RuntimeLibrary=/MTd
) else (
  set RuntimeLibrary=/MT
)

if "%target%"=="x64" (
  set machineType=X64
  set "cl_command=%cl_x64%"
  set "lib_command=%lib_x64%"
  set "link_command=%link_x64%"
  set "clSettingCommon=/c /GS /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /Ob1 /fp:precise /D "WIN32" /D "NDEBUG" /D "_VC80_UPGRADE=0x0600" /errorReport:prompt /GF /WX- /Zc:forScope /Gd %RuntimeLibrary% /EHsc /nologo /I"..\..\include""
) else (
  set machineType=X86
  set "cl_command=%cl_x86%"
  set "lib_command=%lib_x86%"
  set "link_command=%link_x86%"
  set "clSettingCommon=/analyze- /Oy- /c /GS /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /Ob1 /fp:precise /D "WIN32" /D "NDEBUG" /D "_VC80_UPGRADE=0x0600" /errorReport:prompt /GF /WX- /Zc:forScope /Gd %RuntimeLibrary% /EHsc /nologo /I"..\..\include""
)

set "clSettingsLib=/D "_WINDOWS""
set "clSettingsExe=/D "_CONSOLE" /D "_MBCS""
set "libSettings=/NOLOGO /MACHINE:%machineType%"
set "linkSettings=/MANIFEST /DYNAMICBASE:NO /MACHINE:%machineType% /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /ERRORREPORT:PROMPT /NOLOGO /TLBID:1"
set "linkDependencies="kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "%libAO_library_path%" "%libtsp_library_path%""

::echo flags and options
echo ***************** MPEG 3D Audio Coder - Reference Model 8 *********************
echo *                                                                             *
echo *                               Windows compiler                              *
echo *                                                                             *
echo *                                  %DATE%                                 *
echo *                                                                             *
echo *    This software may only be used in the development of the MPEG 3D Audio   *
echo *    standard, ISO/IEC 23008-3 or in conforming implementations or products.  *
echo *                                                                             *
echo *******************************************************************************
echo.
echo Using %platform% compiler and linker [Microsoft Visual Studio %vsVersion%].
echo.

if "%platform%"=="VS11" (
  echo Setting environment for using Microsoft Visual Studio %vsVersion% %target% tools.
)

::load VS2012 environment
if "%target%"=="x64" (
  call "%VScommonTools3da%..\..\VC\vcvarsall" amd64
) else (
  call "%VScommonTools3da%..\..\VC\vcvarsall" x86
)

::create output directories
if not exist %libtspOutDir% (
  mkdir %libtspOutDir%
) else (
  del /s /f %libtspOutDir%\*.lib >nul 2>&1
)

if not exist %libAOOutDir% (
  mkdir %libAOOutDir%
) else (
  del /s /f %libAOOutDir%\*.lib >nul 2>&1
)

if not exist %ResampAudioOutDir% (
  mkdir %ResampAudioOutDir%
) else (
  del /s /f %ResampAudioOutDir%\*.exe >nul 2>&1
)

if not exist %libtspDir% (
  mkdir %libtspDir%
) else (
  del /s /f %libtspDir%\*.obj >nul 2>&1
  del /s /f %libtspDir%\*.lib >nul 2>&1
  del /s /f %libtspDir%\*.txt >nul 2>&1
)

if not exist %libAODir% (
  mkdir %libAODir%
) else (
  del /s /f %libAODir%\*.obj >nul 2>&1
  del /s /f %libAODir%\*.lib >nul 2>&1
  del /s /f %libAODir%\*.txt >nul 2>&1
)

if not exist %ResampAudioDir% (
  mkdir %ResampAudioDir%
) else (
  del /s /f %ResampAudioDir%\*.obj >nul 2>&1
  del /s /f %ResampAudioDir%\*.txt >nul 2>&1
)

::collect all source files
cd %libtsp_src_path%
for /r %%i in (*.c) do echo %%i >> %libtspDir%\libtsp.txt

cd %libAO_src_path%
for /r %%i in (*.c) do echo %%i >> %libAODir%\libAO.txt

cd %ResampAudio_src_path%
for /r %%i in (*.c) do echo %%i >> %ResampAudioDir%\ResampAudio.txt

::compile and link libtsplite
echo.
echo ------ Rebuild started: Project: libtsp, Configuration: Release %target% ------
echo.
  cd %libtspDir%
  "%cl_command%" %clSettingCommon% %clSettingsLib% @libtsp.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  for /r %%i in (*.obj) do echo %%i >> obj.txt

  "%lib_command%" %libSettings% /OUT:%libtsp_library_path% @obj.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  set /a cntOK+=1

::compile and link libAO
echo.
echo ------ Rebuild started: Project: libAO, Configuration: Release %target% ------
echo.
  cd %libAODir%
  "%cl_command%" %clSettingCommon% %clSettingsLib% @libAO.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  for /r %%i in (*.obj) do echo %%i >> obj.txt

  "%lib_command%" %libSettings% /OUT:%libAO_library_path% @obj.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  set /a cntOK+=1

::compile and link ResampAudio
echo.
echo ------ Rebuild started: Project: ResampAudio, Configuration: Release %target% ------
echo.
  cd %ResampAudioDir%
  "%cl_command%" %clSettingCommon% %clSettingsExe% @ResampAudio.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  for /r %%i in (*.obj) do echo %%i >> obj.txt

  "%link_command%" %linkSettings% %linkDependencies% /OUT:%ResampAudio_binary_path% @obj.txt
  if %errorlevel% NEQ 0 goto BUILDERROR

  set /a cntOK+=1

goto END

:BUILDERROR
::calculate statistic
set /a cntSKIPP=2-%cntOK%

echo.
echo ========== Rebuild: %cntOK% succeeded, 1 failed, %cntSKIPP% skipped ==========
exit /B 1

:END
echo.
echo ========== Rebuild: %cntOK% succeeded, 0 failed, 0 skipped ==========
