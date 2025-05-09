@echo off
setlocal

REM 切换到脚本所在目录，确保相对路径可用
cd /d "%~dp0"

REM 执行编码器
..\exe\xaacenc.exe ^
  -ifile:"..\sequence\44100_1411_2.wav" ^
  -ofile:"..\encoded\encoded.es" ^
  -br:64000 ^
  -aot:42 ^
  -adts:0 ^
  -ccfl_idx:3 ^
  -usac:0 ^
  -drc:1
endlocal
pause

::-br:64000                    设置编码后文件的比特率
::-aot:42                      使用USAC进行编码
::–esbr:1                      USAC下的增强型SBR频段复制技术，无论如何设置都默认开启
::-mps:1                       立体声技术，可以关闭
::-usac:1                      USAC编码模式   0自动切换   1使用FD   2使用TD
::-cmpx_pred:0                 复杂预测
::-tns:1                       时域噪声整形
::-nf:0                        噪声填充
::-adts:0                      启用ADTS头，USAC下必选的
::-max_out_buffer_per_ch:384   设置比特池的最大大小（-1到6144）
::-ccfl_idx:3                  设置USAC的帧长度和eSBR比率 1（1024帧长，禁用eSBR） 3 （1024帧长，e
::-pvc_enc:0                   启用或禁用 PVC 编码器，适用于 USAC
::-drc:0                       启用或禁用 DRC 编码器，适用于 USAC 
::-inter_tes_enc:0             启用或禁用 inter-TES 编码器，适用于 USAC 
::-harmonic_sbr:0              启用或禁用 Harmonic SBR 编码器，适用于 USAC
::-esbr_hq:0                   启用或禁用 高质量eSBR，适用于 USAC，仅在harmonic_sbr为1时有效