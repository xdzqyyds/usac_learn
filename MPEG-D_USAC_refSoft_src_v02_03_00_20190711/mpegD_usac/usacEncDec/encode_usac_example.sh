#! /bin/bash

# Please make sure that the ResampAudio executable resides in your $PATH
# or copy the binary to the same folder where you would like to run this
# script.

# e.g.: export PATH=/usr/AFsp-v8r2/bin:$PATH

ENCODER=./usacEnc
DECODER=./usacDec

# get input file
if [ $# -lt 1 ]
then
  echo "Input File missing"
  exit 1
else
  inFile=$1
fi

tmpFile=tmp.wav
itemName=`basename $inFile .wav`


# Example for 12kbps
encFile=${itemName}_12.mp4
decFile=${itemName}_12.wav

ResampAudio -s 25600 $inFile $tmpFile
$ENCODER -m usac -c "-usac_switched -hSBR -br 12000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile

# Example for 16kbps
encFile=${itemName}_16.mp4
decFile=${itemName}_16.wav

ResampAudio -s 28800 $inFile $tmpFile
$ENCODER -m usac -c "-usac_switched -hSBR -br 16000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile

# Example for 20kbps
encFile=${itemName}_20.mp4
decFile=${itemName}_20.wav

ResampAudio -s 32000 $inFile $tmpFile
$ENCODER -m usac -c "-usac_switched -br 20000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile

# Example for 24kbps
encFile=${itemName}_24.mp4
decFile=${itemName}_24.wav

ResampAudio -s 34150 $inFile $tmpFile
$ENCODER -m usac -c "-usac_switched -br 24000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile

# Example for 32kbps
encFile=${itemName}_32.mp4
decFile=${itemName}_32.wav

ResampAudio -s 40000 $inFile $tmpFile
$ENCODER -m usac -c "-usac_switched -ipd -br 32000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile

# Example for 64kbps
encFile=${itemName}_64.mp4
decFile=${itemName}_64.wav

ResampAudio -s 48000 $inFile $tmpFile
$ENCODER -m usac -c "-usac_fd -usac_tw -br 64000" -o $encFile $tmpFile
$DECODER -o $decFile $encFile
