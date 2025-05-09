@echo off

set currentDir=%~dp0
set buildMode=Debug
::set silentMode=call_pause
set silentMode=silent

set AMD1=0
set MPEGH=0
set LOUDNESS_LEVELING=0
set WRITE_FRAMESIZE=0
set NODE_RESERVOIR=0

:parseFlags
IF "%1"=="" GOTO endParseFlags

IF "%1"=="Clean" set clean="--clean-first"
IF "%1"=="Release" set buildMode=Release

IF "%1"=="x64" set target="-A" "x64"

IF "%1"=="MPEGH" (
    set MPEGH=1
    set LOUDNESS_LEVELING=1
)
IF "%1"=="AMD1" (
    set AMD1=1
    set LOUDNESS_LEVELING=1
)
IF "%1"=="WRITE_FRAMESIZE" set WRITE_FRAMESIZE=1


shift
GOTO parseFlags
:endParseFlags

if not exist build mkdir build

cd build && cmake -DAMD1=%AMD1% -DMPEGH=%MPEGH% -DLOUDNESS_LEVELING=%LOUDNESS_LEVELING% -DWRITE_FRAMESIZE=%WRITE_FRAMESIZE% -DNODE_RESERVOIR=0 %target% ../..
if %errorlevel% NEQ 0 goto BUILDERROR

cmake --build . %clean%
if %errorlevel% NEQ 0 goto BUILDERROR

echo       DRC Build Successful.
if "%silentMode%"=="call_pause" pause()
GOTO END

:BUILDERROR
echo       Build Failed. Check corresponding log file for more information.
if "%silentMode%"=="call_pause" pause()
exit /B -1

:END
exit /B 0
