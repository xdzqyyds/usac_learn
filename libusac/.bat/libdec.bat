@echo off
setlocal

REM 切换到脚本所在目录，确保相对路径可用
cd /d "%~dp0"

REM 执行解码器
..\exe\xaacdec.exe ^
  -ifile:"..\encoded\encoded.es" ^
  -ofile:"..\decoded\decoded.wav" ^
  -imeta:"..\encoded\encoded.txt" ^
  -mp4:1 ^
  –target_loudness: -31

endlocal
pause

::-esbr_hq_flag:0              是否启用eSBR高质量模式
::-esbr_ps_flag:0              是否启用eSBR + PS
::–target_loudness:            目标响度水平，默认为-24dB
::-peak_limiter_off_flag:0     是否启用峰值限制器,默认为0（启用）