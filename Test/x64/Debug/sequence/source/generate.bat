@echo off
for %%R in (12000 16000 24000) do (
  for %%C in (1 2) do (
    ffmpeg -f lavfi -i "sine=frequency=440:duration=1:sample_rate=%%R" -ac %%C -acodec pcm_s16le %%R_%%C.wav
  )
)
