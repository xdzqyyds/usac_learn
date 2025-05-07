@echo off

set buildSetting=Win32
set platformId=%1

if "%1"=="" (
    echo first argument must be OS name/platform string
    echo example call:
    echo %0 Windows_AMD64
    GOTO BUILDERROR
)

echo | call compile Clean %buildSetting%
if %errorlevel% NEQ 0 goto BUILDERROR
echo | call encode %platformId%
echo | call encode_uniDrcInterface %platformId%
echo | call decode %platformId%
echo | call test_selectionProcess %platformId%

echo | call compile Clean MPEGH %buildSetting%
if %errorlevel% NEQ 0 goto BUILDERROR
echo | call encode %platformId% MPEGH
echo | call encode_uniDrcInterface %platformId% MPEGH
echo | call decode_cmdlToolChain %platformId% MPEGH
echo | call test_selectionProcess %platformId% MPEGH

echo | call compile Clean AMD1 %buildSetting%
if %errorlevel% NEQ 0 goto BUILDERROR
echo | call encode_AMD1 %platformId%
echo | call encode_uniDrcInterface_AMD1 %platformId%
echo | call decode_AMD1 %platformId%
echo | call test_selectionProcess_AMD1 %platformId%

echo       Script start_all.bat successful.
GOTO END

:BUILDERROR
echo       Script start_all.bat failed.
exit /B -1

:END
