REM @echo off
REM  Batch script example for running the usac reference software

REM # Please make sure that the ResampAudio executable resides in your %Path%
REM # or copy the binary to the same folder where you would like to run this
REM # script.


set ENCODER=usacEnc.exe
set DECODER=usacDec.exe
set tmpFile=tmp.wav


REM get input file
set inFile=%1%



set encFile=encoded_12.mp4
set decFile=decoded_12.mp4

ResampAudio -s 25600 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_switched -hSBR -br 12000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%


set encFile=encoded_16.mp4
set decFile=decoded_16.wav
	
ResampAudio -s 28800 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_switched -hSBR -br 16000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%

set encFile=encoded_20.mp4
set decFile=decoded_20.wav

ResampAudio -s 32000 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_switched -br 20000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%

set encFile=encoded_24.mp4
set decFile=decoded_24.wav

ResampAudio -s 34150 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_switched -br 24000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%

set encFile=encoded_32.mp4
set decFile=decoded_32.wav

ResampAudio -s 40000 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_switched -ipd -br 32000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%

set encFile=encoded_64.mp4
set decFile=decoded_64.wav

ResampAudio -s 48000 %inFile% %tmpFile%
%ENCODER% -m usac -c "-usac_fd -usac_tw -br 64000" -o %encFile% %tmpFile%
%DECODER% -o %decFile% %encFile%

pause