@echo off
setlocal enabledelayedexpansion

set currentDirIsobmff=%~dp0
set isobmffDir="%currentDirIsobmff%..\libisomediafile"
set gitBaseDir="%currentDirIsobmff%isobmff"
set vcmTool=vcmSVN
set promptRequest=1
set verbose=0

:: loop over arguments
:parseFlags
if "%1"=="" goto endParseFlags

if "%1"=="GIT" set vcmTool=vcmGIT
if "%1"=="-p" set promptRequest=0
if "%1"=="v" set verbose=1

shift
GOTO parseFlags
:endParseFlags

:: ask user for existing GitHub account
if %promptRequest% EQU 1 (
  set /p GitHubID="Is an GitHub ID registered? (Y/N)"

  if NOT !GitHubID!==y (
    if NOT !GitHubID!==Y (
     goto RUNERROR
    )
  )
)

:: create output directories
if exist %isobmffDir% (
  rmdir /s /q %isobmffDir%
)
mkdir %isobmffDir%

:: checkout the software
if "%vcmTool%"=="vcmSVN" (
  echo downloading IsoLib from GitHub using SVN...

  :: checkout IsoLib from GitHub via SVN
  if %verbose% EQU 1 (
    echo.
    svn co https://github.com/MPEGGroup/isobmff.git/trunk/IsoLib/libisomediafile %isobmffDir%
  ) else (
    svn co https://github.com/MPEGGroup/isobmff.git/trunk/IsoLib/libisomediafile %isobmffDir% >nul 2>&1
  )
) else (
  echo downloading IsoLib from GitHub using GIT...
  echo.

  :: checkout IsoLib from GitHub via GIT
  if exist %gitBaseDir% (
    rmdir /s /q %gitBaseDir%
  )
  git clone https://github.com/MPEGGroup/isobmff.git %gitBaseDir%

  :: copy libisomediafile in to the destination directory
  if %verbose% EQU 1 (
    xcopy /v /e %gitBaseDir%\IsoLib\libisomediafile %isobmffDir%
    if %errorlevel% NEQ 0 ( goto RUNERROR )
  ) else (
    xcopy /v /e /q %gitBaseDir%\IsoLib\libisomediafile %isobmffDir%
    if %errorlevel% NEQ 0 ( goto RUNERROR )
  )
)

goto END

:RUNERROR
echo.
echo       Checkout failed.
exit /B 1

:END
echo.
echo       Checkout complete.
exit /B 0
