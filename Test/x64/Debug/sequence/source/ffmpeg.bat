@echo off
for %%F in (*.wav) do (
  echo ==== %%F ====
  ffprobe -v error -select_streams a:0 -show_entries stream=codec_name,sample_rate,channels,duration -of default=noprint_wrappers=1:nokey=0 %%F
  echo.
)

pause