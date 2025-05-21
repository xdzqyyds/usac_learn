@echo off
setlocal

for %%R in (44100 48000 32000) do (
    for %%C in (1 2) do (
        echo Generating %%R_%%C.wav ...
        ffmpeg -f lavfi -i "sine=frequency=1000:duration=1:sample_rate=%%R" -ac %%C -ar %%R -sample_fmt s16 -y %%R_%%C.wav
    )
)

echo All .wav files generated successfully.
pause
