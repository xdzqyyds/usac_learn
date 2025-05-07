#!/bin/bash

currentDirUsac=$(pwd)
mp4Dir=$currentDirUsac/../example
operatingSystem=$(uname -s)
platform=C11
target=$(uname -m)
DebugLevel=1
clobberOn=0
buildCommand="/Rebuild"
executeDecoding=1
checkout=0
vcmTool="SVN"
verbose=0

# loop over arguments
for var in "$@"
do
  if [ "$var" == "Clean" ]; then clobberOn=1; fi
  if [ "$var" == "c89" ]; then platform=C89; fi
  if [ "$var" == "Release" ]; then DebugLevel=0; fi
  if [ "$var" == "nDec" ]; then executeDecoding=0; fi
  if [ "$var" == "co" ]; then checkout=1; fi
  if [ "$var" == "GIT" ]; then vcmTool="GIT"; fi
  if [ "$var" == "v" ]; then verbose=1; fi
done

# set compiler specific options
buildMode=D
OptimLevel=0
clobber=""
if [ $DebugLevel -eq 0 ]; then
    buildMode=R
    OptimLevel=3
fi
if [ $clobberOn -eq 1 ]; then
    clobber="clobber"
    buildCommand="/Clean"
fi
DebugLevelDRC=1
OptimLevelDRC=0
DebugLevelAFsp=0
OptimLevelAFsp=3

# set directories and libtsplite library
if [ $operatingSystem == "Darwin" ]; then
  libtsplite=libtsplite_Mac.a
else
  if [ $target == "x86_64" ]; then
    libtsplite=libtsplite_x86_64.a
  else
    libtsplite=libtsplite.a
  fi
fi
if [ $DebugLevel -eq 0 ]; then
  buildOutDirUnix=$operatingSystem\_$target\_O$OptimLevel
else
  buildOutDirUnix=$operatingSystem\_$target\_$buildMode\_O$OptimLevel
fi
buildOutDirUnixDRC=$operatingSystem\_$target\_D\_O$OptimLevelDRC
buildDir=$currentDirUsac/executables_$buildOutDirUnix\_$platform
buildDirAFsp=$currentDirUsac/../tools/AFsp

# echo flags and options
DATE=`date +%Y-%m-%d`
echo -e "******************* MPEG-D USAC Audio Coder - Edition 2.3 *********************"
echo -e "*                                                                             *"
echo -e "*                               Unix OS compiler                              *"
echo -e "*                                                                             *"
echo -e "*                                  $DATE                                 *"
echo -e "*                                                                             *"
echo -e "*    This software may only be used in the development of the MPEG USAC Audio *"
echo -e "*    standard, ISO/IEC 23003-3 or in conforming implementations or products.  *"
echo -e "*                                                                             *"
echo -e "*******************************************************************************\n"
echo -e "Using $platform compiler [$operatingSystem $target].\n"
echo -e "compile flags CO:   $buildCommand DEBUG=$DebugLevel OPTIM=$OptimLevel $target"
echo -e "compile flags DRC:  $buildCommand DEBUG=$DebugLevelDRC OPTIM=$OptimLevelDRC $target"
echo -e "compile flags AFsp: $buildCommand DEBUG=$DebugLevelAFsp OPTIM=$OptimLevelAFsp $target\n"

# create output directory
if [ ! -d "$buildDir" ]; then
  mkdir "$buildDir"
else
  rm -rf $buildDir/*
fi
if [ ! -d "$buildDirAFsp/lib" ]; then
  mkdir "$buildDirAFsp/lib"
else
  rm -rf $buildDirAFsp/lib/*.a
fi

# checkout IsoLib
if [ $checkout -eq 1 ]; then
  echo -e "downloading IsoLib from GitHub...\n"
  cd $currentDirUsac/../tools/IsoLib/scripts/
  ./checkout.sh -p v $vcmTool > $currentDirUsac/coIsoLib.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat coIsoLib.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  fi
  cd $currentDirUsac
fi

# check add-on libraries
if [ ! -f $currentDirUsac/../tools/AFsp/Makefile ]; then
  echo -e "AFsp is not existing."
  echo -e "Please download http://www.mmsp.ece.mcgill.ca/Documents/Downloads/AFsp/.\n"
  exit 1
fi
if [ ! -f $currentDirUsac/../mpegD_drc/MPEG_D_DRC_refsoft/scripts/compile.sh ]; then
  echo -e "MPEG-D DRC is not existing."
  echo -e "Please add MPEG-D DRC reference software as in ISO/IEC 23003-4.\n"
  exit 1
fi
if [ ! -f $currentDirUsac/../tools/IsoLib/libisomediafile/linux/libisomediafile/Makefile ]; then
  echo -e "ISOBMFF is not existing."
  echo -e "Please download https://github.com/MPEGGroup/isobmff/archive/master.zip.\n"
  exit 1
fi

# compile AFsp library
echo Compiling libtsplite
  make -C  $currentDirUsac/../tools/AFsp/ DEBUG=$DebugLevelAFsp OPTIM=$OptimLevelAFsp > AFspLib.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat AFspLib.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    mv $buildDirAFsp/lib/libtsplite.a $buildDirAFsp/lib/$libtsplite
  fi

# compile MPEG-D USAC Audio modules
echo Compiling core Encoder
  if [ $clobberOn -eq 1 ] ; then
    make -C $currentDirUsac/../mpegD_usac/usacEncDec/make $clobber DEBUG=$DebugLevel OPTIM=$OptimLevel > coreEnc.log 2>&1
  else
    make -C $currentDirUsac/../mpegD_usac/usacEncDec/make $clobber usacEnc DEBUG=$DebugLevel OPTIM=$OptimLevel > coreEnc.log 2>&1
  fi
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat coreEnc.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    cp $currentDirUsac/../mpegD_usac/usacEncDec/bin/$buildOutDirUnix\v2_c1_eptool_sbr/usacEnc $buildDir/usacEnc
  fi

echo Compiling core Decoder
  if [ $clobberOn -eq 1 ] ; then
    make -C $currentDirUsac/../mpegD_usac/usacEncDec/make $clobber DEBUG=$DebugLevel OPTIM=$OptimLevel > coreDec.log 2>&1
  else
    make -C $currentDirUsac/../mpegD_usac/usacEncDec/make $clobber usacDec DEBUG=$DebugLevel OPTIM=$OptimLevel > coreDec.log 2>&1
  fi
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat coreDec.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    cp $currentDirUsac/../mpegD_usac/usacEncDec/bin/$buildOutDirUnix\v2_c1_eptool_sbr/usacDec $buildDir/usacDec
  fi

echo Compiling wavCutter
  make -C $currentDirUsac/../tools/wavCutter/wavCutterCmdl/make/ DEBUG=$DebugLevel OPTIM=$OptimLevel  $clobber > wavCutter.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat wavCutter.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    cp $currentDirUsac/../tools/wavCutter/wavCutterCmdl/bin/$buildOutDirUnix/wavCutterCmdl $buildDir/wavCutterCmdl
  fi

echo Compiling ssnrcd
  make -C $currentDirUsac/../tools/ssnrcd/make/ ssnrcd DEBUG=$DebugLevel OPTIM=$OptimLevel  $clobber > ssnrcd.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat ssnrcd.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    cp $currentDirUsac/../tools/ssnrcd/bin/$buildOutDirUnix/ssnrcd $buildDir/ssnrcd
  fi

# compile DRC decoder
echo Compiling drcCoder
  chmod -R u+rwx $currentDirUsac/../mpegD_drc/MPEG_D_DRC_refsoft
  cd $currentDirUsac/../mpegD_drc/MPEG_D_DRC_refsoft/scripts/
  currentDirDrc=$currentDirUsac/../mpegD_drc/MPEG_D_DRC_refsoft/scripts/

  ./compile.sh AMD1 WRITE_FRAMESIZE $clobber > DrcTool.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat DrcTool.log; fi
    echo "       Build failed. Check corresponding log file for more information."
    exit 1
  else
    cp $currentDirDrc/../modules/drcTool/drcToolDecoder/bin/$buildOutDirUnixDRC/drcToolDecoder $buildDir/drcToolDecoder
    cp $currentDirDrc/../modules/drcTool/drcToolEncoder/bin/$buildOutDirUnixDRC/drcToolEncoder $buildDir/drcToolEncoder
  fi

# perform example decoding of mp4 bitstreams
if [ $executeDecoding -eq 1 ]; then
  cd $currentDirUsac
  echo -e "\nDecoding example__C1_3_FD.mp4"
  $buildDir/usacDec -if "$mp4Dir/example__C1_3_FD.mp4" -of "$mp4Dir/example__C1_3_FD.wav" > decoding.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat decoding.log; fi
    echo "       Decoding failed. Check corresponding log file for more information."
    exit 1
  fi

  echo -e "Decoding example__C2_3_FD.mp4"
  $buildDir/usacDec -if "$mp4Dir/example__C2_3_FD.mp4" -of "$mp4Dir/example__C2_3_FD.wav" >> decoding.log 2>&1
  if [ $? -ne 0 ]; then
    if [ $verbose -eq 1 ]; then cat decoding.log; fi
    echo "       Decoding failed. Check corresponding log file for more information."
    exit 1
  fi
fi

echo -e "\nAll executables are collected in:"
echo -e $buildDir/
if [ $executeDecoding -eq 1 ]; then
  echo -e "\nAll decoded .wav files are collected in:"
  echo -e $mp4Dir/
fi
echo -e "\n       Build successful."
exit 0
