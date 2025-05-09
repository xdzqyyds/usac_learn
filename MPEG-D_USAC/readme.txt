************************** MPEG-D USAC Audio Coder - Edition 2.3 ***************************
*                                                                                          *
*                      MPEG-D USAC Audio Reference Software - readme                       *
*                                                                                          *
*                                        2019-01-17                                        *
*                                                                                          *
********************************************************************************************


(1) Content of this package
============================================================================================
 - MPEG-D USAC Audio reference software containing the core coder written in ANSI C/C++
 - Makefiles/project files for Linux/Mac and Microsoft Visual Studio 2008/2012


(2) Requirements to successfully build and run the reference software
============================================================================================
 - Additional software packages:
   . AFsp Audio File Programs and Routines
   . ISOBMFF reference software (libisomediafile)
   . MPEG-D DRC reference software
 - Linux/Mac:
   . GNU make
   . gcc 4.4.5 or higher
 - Windows:
   . Microsoft Visual Studio Studio 2012
   . Microsoft Windows SDK


(2.1) Installing AFsp File Programs and Routines
============================================================================================
(a) General instructions:
-------------------------

 To provide support for a variety of audio file formats, the MPEG-D USAC Audio reference
 software uses the AFsp package for audio file i/o. The AFsp package is written by
 Prof. Peter Kabal <peter.kabal@mcgill.ca> and is available via download:

 required version:  AFsp-v9r0.tar.gz
 download site:     http://www.mmsp.ece.mcgill.ca/Documents/Downloads/AFsp/

 NOTE: Make sure to use version v9r0 (AFsp-v9r0.tar.gz)!

 Unpack the downloaded file and copy/paste the whole AFsp content into the folder
 ./tools/AFsp/

 - such that MSVC.sln is located at:
   ./tools/AFsp/MSVC/MSVC.sln

 - and such that the Unix makefile "Makefile" is located at:
   ./tools/AFsp/Makefile

 For more details, see ./tools/AFsp/readme.txt.

(b) Build the libtsplite library:
---------------------------------

 The AFsp library will be build automatically together with the MPEG-D USAC Audio
 reference software on all platforms.


(2.2) Installing MPEG-4 Base Media File Format (ISOBMFF) software
============================================================================================
(a) General instructions:
-------------------------

 The ISO Base Media File Format (ISOBMFF) is an integral part of the MPEG-D USAC Audio
 reference  software. It provides the interface to the mp4 container in which the MPEG-D
 USAC Audio streams are embedded. The ISOBMFF is standardized in ISO/IEC 14996-12 and hosted
 on GitHub.

 GitHub space:      https://github.com/MPEGGroup/isobmff
 download site:     https://github.com/MPEGGroup/isobmff/archive/master.zip

 Unpack the downloaded file and copy/paste the sub-directory
 ./isobmff-master/IsoLib/libisomediafile into the folder ./tools/IsoLib/

 - such that the corresponding VS2008 solution libisomedia.sln is located at:
   ./tools/IsoLib/libisomediafile/w32/libisomediafile/VS2008/libisomedia.sln

 - and such that the corresponding VS2012 solution libisomedia.sln is located at:
   ./tools/IsoLib/libisomediafile/w32/libisomediafile/VS2012/libisomedia.sln

 - and such that the corresponding UNIX makefile "Makefile" is located at:
   ./tools/IsoLib/libisomediafile/linux/libisomediafile/Makefile

 For more details, see ./tools/IsoLib/readme.txt.

 NOTE: The ISOBMFF software can also be automatically installed, see (3)(d).

(b) Build the MPEG-4 FF reference software:
-------------------------------------------

 The MPEG-4 FF reference software will be build automatically together with the
 MPEG-D USAC Audio reference software on all platforms.


(2.3) Installing MPEG-D DRC reference software
============================================================================================
(a) General instructions:
-------------------------

 Copy the unzipped folder /MPEG_D_DRC_refsoft from
 w17642 - Defect Report on ISO/IEC 23003-4:2015/AMD 2, DRC Reference Software
 into the folder:

 ./mpegD_drc/

 - such that the corresponding compile.bat is located at:
   ./mpegD_drc/MPEG_D_DRC_refsoft/scripts/compile.bat

 - and such that corresponding compile.sh is located at:
   ./mpegD_drc/MPEG_D_DRC_refsoft/scripts/compile.sh

 For more details, see ./mpegD_drc/readme.txt.

(b) Build the MPEG-D DRC reference software:
--------------------------------------------

 The MPEG-D DRC reference software will be build automatically together with the
 MPEG-D USAC Audio reference software on all platforms.


(3) MPEG-D USAC Audio Reference Software build instructions
============================================================================================
(a) Linux/Mac:
--------------

 - Install AFsp and MPEG-D DRC reference software as explained above.
 - Set access permissions if required.
 - Change directory into scripts folder (cd ./scripts).
 - Execute ./compile.sh to automatically compile all modules and tools.
 - Change directory into ./executables_* folder and check if all 6 files are listed (ls -l).

(b) Windows:
------------

 - Install AFsp and MPEG-D DRC reference software as explained above.
 - Make sure that the environment variable VS110COMNTOOLS is present and set according
   to your Visual Studio 2012 installation.
 - Change directory into scripts folder (cd scripts).
 - Execute compile.bat to automatically compile all modules and tools.
 - Change directory into .\executables_* folder and check if all 6 files are listed (dir).

(c) general notes:
------------------

 NOTE: A successful build will exit with code 0 and show the message:
       "Build successful."
       A failed build will exit with code 1 and show the message:
       "Build failed."
 NOTE: The path of the MPEG-D USAC Audio reference software directory must not
       contain any white spaces!
 NOTE: To compile single modules open compile.bat/compile.sh to see how single modules are
       compiled.

(d) build options (advanced users):
-----------------------------------

 The Unix compile script (compile.sh) features the following options:
 - ./compile.sh Clean   : The "Clean" option deletes all intermediate files, libraries
                          and executables which were created during the build process.
 - ./compile.sh Release : The "Release" option build the MPEG-D USAC Audio reference software
                          in release mode such that compiler specific optimizations are used.
 - ./compile.sh c89     : The "c89" option builds the MPEG-D USAC Audio reference software in
                          a C89 compliant mode. Please see (3)(e)!
 - ./compile.sh co      : Checkout required software such as the libisomediafile. An installed
                          SVN and/or GIT software is required to use this option! The
                          additional options "SVN" and "GIT" may be used to determine the
                          version control management system to use, e.g.
                          ./compile.sh co GIT
                          Default: SVN

 The Windows compile script (compile.bat) features the following options:
 - compile.bat x64      : The "x64" option builds the MPEG-D USAC Audio reference software for
                          64-bit Windows operating systems. Default is Win32 (32-bit).
 - compile.bat Release  : The "Release" option build the MPEG-D USAC Audio reference software
                          in release mode such that compiler specific optimizations are used.
 - compile.bat VS2008   : The "VS2008" option builds the MPEG-D USAC Audio reference software
                          using the Microsoft Visual Studio 2008 compiler. Please see (3)(e)!
 - compile.bat co       : Checkout required software such as the libisomediafile. An installed
                          SVN and/or GIT software is required to use this option! The
                          additional options "SVN" and "GIT" may be used to determine the
                          version control management system to use, e.g.
                          compile.bat co GIT
                          Default: SVN

 NOTE: The use of these options is for advanced users. Please note that the compile scripts
       compile.sh/compile.bat are configured in a way to fit the conformance requirements.
       Use any of these options at your own risk.

(e) compatibility to previous compiler versions (advanced users):
-----------------------------------------------------------------

 The MPEG-D reference software is in parts backward compatible to older compiler versions:
 - Linux/Mac
   . gcc 3.3.5 or higher
   . In order to run the compatibility mode, call: ./compile.sh c89
 - Windows
   . Microsoft Visual Studio Studio 2008 (requires environment variable VS90COMNTOOLS)
   . In order to run the compatibility mode, call: compile.bat VS2008


(4) Quick start: Decoding using the usacDec
============================================================================================
 After running the appropriate compile script, change directory into executables_* folder.
 Then, call usacDec[.exe] without any arguments to see all available command line
 arguments.

 A typical call requires a minimum of two parameters: input file (mp4), output file, e.g.:
 - Linux/Mac: ./usacDec -if <input>.mp4 -of <output>.wav
 - Windows:   usacDec.exe -if <input>.mp4 -of <output>.wav


(4.1) Example Decoding
============================================================================================
(a) Get MPEG-D USAC Audio conformance streams:
----------------------------------------------

 Download MPEG-D USAC Audio conformance streams as defined in:
   ISO/IEC 23003-3 Unified speech and audio coding

 from the ISO supplied Uniform Resource Name (URN):
   http://standards.iso.org/iso-iec/23003/-3/ed-2/en

 and put the files (*.mp4 and *.wav files) e.g.:
 *.mp4 in ./MPEGD_USAC_mp4 folder
 *.wav in  ./MPEGD_USAC_ref folder

(b) Decode MPEG-D USAC Audio conformance streams:
-------------------------------------------------

 Change directory into executables_* folder (cd ./scripts/executables_*) and call
 usacDec[.exe] as described above (4).

(c) Compare MPEG-D USAC Audio conformance streams:
--------------------------------------------------

 The correctness of the decoded output can be checked with either the conformance
 tool ssnrcd, or with the AFsp tool compAudio.

 The AFsp audio tools are located at
 - Linux/Mac: ./tools/AFsp/bin
 - Windows:   ./tools/AFsp/MSVC/bin

 Use the compAudio command for comparing two audio files:
 - Linux/Mac: ./compAudio <reference>.wav <MPEG-D_USAC_decoded>.wav
 - Windows:   compAudio.exe <reference>.wav <MPEG-D_USAC_decoded>.wav

 The ssnrcd tool is part of the MPEG-D USAC Audio reference software and will be build
 automatically together with the MPEG-D USAC Audio reference software on all platforms.
 
 Use the following ssnrcd command for comparing two audio files:
 ./ssnrcd <reference>.wav <MPEG-D_USAC_decoded>.wav

(5) Tools overview
============================================================================================
 The reference software modules make use of different helper tools which are listed below:

 - AFsp library as described above

 - libisomediafile (Library)
   . Library for reading and writing MP4 files

 - ssnrcd (Commandline)
   . Audio Conformance Verification according to ISO/IEC 13818-4, ISO/IEC 14496-4,
     ISO/IEC 23003-3, ISO/IEC 23008-9
   . see ./tools/ssnrcd/readme.txt for more details

 - wavCutterCmdl (Commandline)
   . Uses the wavIO library to process wave and qmf files
   . Used to cut wave files (remove/add delay in the beginning and remove flushing samples
     at the end of file)

 - wavIO (Library)
   . Library for reading and writing PCM data or qmf data into wave files


(6) Modules overview
============================================================================================
 The reference software is based on different separate modules which are listed below:

 - mpegD_usac (Commandline)
   . USAC Reference Software based core encoder and decoder implementation

 - mpegD_drc (Commandline/Library)
   . DRC module for decoding DRC sequences and applying them to the audio signal


(7) MPEG-D USAC Encoder Reference Implementation
============================================================================================
 An example reference implementation for the MPEG-D USAC Encoder can be found in

 /mpegD_usac/usacEncDec/win32/usacEnc

 NOTE: The encoder shares its source code with the decoder software. Therefore the path
       to the corresponding Visual Studio solutions is given here. Please check compile.sh
       to get information on the build of the encoder software.

 The encoder supports three different operating modes:
 - operating mode supporting only frequency domain coding (similar to AAC):
    This operating mode can be enabled by the commandline argument
    '-usac_fd'
 - operating mode supporting only LPC domain coding:
    This operating mode can be enabled by the commandline argument
    '-usac_td'
 - operating mode switching between frequency domain and LPC domain
    coding mode: This operating mode can be enabled by the commandline
    argument '-usac_switched'

 Further on, it is required to set a bitrate. The bitrate is set via the '-br' commandline
 switch and is given in bits per second (bps).

 Stereo coding via MPEG Surround is activated automatically for stereo input and
 datarates < 64 kbps. For stereo input and datarates >= 64 kbps, discrete stereo coding is
 activated. Phase coding in MPEG Surround is activated by the command line switch '-ipd'.
 Unified Stereo Coding is activated by the commandline switch '-mps_res'.
 Applause Coding is activated by the command line switch '-tsd applause_classifier.dat'.

 The time warped filterbank is activated by the command line switch '-usac_tw'.

 Dynamic Range Control (DRC) encoding is activated by specifying an appropriate DRC config file
 and DRC gain curve with the command line switches '-drcConfigFile' and '-drcGainFile'.

 Examples:
  - usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 32000 
    -> switched operating mode
    -> bitrate: 32kbps
    -> output-file: output_switched.mp4
    -> input-file: all_mono.wav
    -> no usage of MPEG Surround

  - usacEnc -if example__DRC_noSbr_FD_input.wav -of output_applyDrc.mp4 -usac_fd -sbrRatioIndex 0
    -br 64000 -drcConfigFile example__DRC_noSbr_FD_config.xml -drcGainFile example__DRC_noSbr_FD_gains.dat
    -> Frequency domain operating mode
    -> bitrate: 64kbps
    -> SBR disabled
    -> output-file: output_applyDrc.mp4
    -> input-file: example__DRC_noSbr_FD_input.wav
       (see: ./MPEG-D/Part03-Unified_Speech_and_Audio_Coding/USAC_Test_Vectors/trunk/USAC_m43493_DrcEncoding.zip)
    -> DRC Encoding activated
    -> drcToolEncoder input-files: example__DRC_noSbr_FD_config.xml
                                   example__DRC_noSbr_FD_gains.dat
       (see: ./MPEG-D/Part03-Unified_Speech_and_Audio_Coding/USAC_Test_Vectors/trunk/USAC_m43493_DrcEncoding.zip)

  - usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 32000 -sbrRatioIndex 2 
    -> switched operating mode
    -> bitrate: 32kbps
    -> 8:3 SBR system, core-coder frame length of 768
    -> output-file: output_switched.mp4
    -> input-file: all_mono.wav
    -> no usage of MPEG Surround

  - usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 16000 -sbrRatioIndex 1
    -> switched operating mode
    -> bitrate: 16kbps
    -> 4:1 SBR system, core-coder frame length of 1024
    -> output-file: output_switched.mp4
    -> input-file: all_mono.wav
    -> no usage of MPEG Surround

  - usacEnc -if all_mono.wav -of output_lpcDomain.mp4 -usac_td -br 30000
    -> LPC domain operating mode
    -> bitrate: 30kbps
    -> output-file: output_lpcDomain.mp4
    -> input-file: all_mono.wav
    -> no usage of MPEG Surround

  - usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -br 32000
    -> Frequency domain operating mode
    -> bitrate: 32kbps
    -> output-file: output_freqDomain.mp4
    -> input-file: all_stereo.wav
    -> MPEG Surround automatically activated

  - usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -usac_tw -br 64000
    -> Frequency domain operating mode
    -> bitrate: 64kbps
    -> output-file: output_freqDomain.mp4
    -> input-file: all_stereo.wav
    -> discrete stereo coding activated
    -> time warped filter bank activated

  - usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -ipd -br 32000
    -> Frequency domain operating mode
    -> bitrate: 32kbps
    -> output-file: output_freqDomain.mp4
    -> input-file: all_stereo.wav
    -> MPEG Surround automatically activated
    -> Phase coding in MPEG Surround activated

  - usacEnc -if all_stereo -of output_uniStereo.mp4 -usac_fd -br 64000 -ipd -mps_res
    -> Frequency domain operating mode
    -> bitrate: 64kbps
    -> output-file: output_uniStereo.mp4
    -> input-file: all_stereo.wav
    -> Unified Stereo Coding activated

  - usacEnc -if all_applause.wav -of output_applCoding.mp4 -usac_fd -br 32000 -tsd all_applause_TsdData_32s.dat
    -> Frequency domain operating mode
    -> bitrate: 32kbps
    -> output-file: output_applCoding.mp4
    -> input-file: all_applause.wav
      (see ./MPEG-D/Part03-Unified_Speech_and_Audio_Coding/USAC_Test_Vectors/trunk/USAC_m19311_Applause_Originals.zip)
    -> Applause Coding activated
    -> Applause classifier: all_applause_TsdData_32s.dat 
      (see ./MPEG-D/Part03-Unified_Speech_and_Audio_Coding/USAC_Test_Vectors/trunk/USAC_m19311_Applause_Originals.zip)

 Note: The encoder does not include an automatic resampling of the audio input, i.e. the
 sampling rate of the encoded item will be identical to the sampling rate of input.
 Reasonable sampling rate/bitrate combinates should be chosen by the user, especially when
 operating the encoder at lower bitrates. Combinations of sampling rate/bitrate not supported
 by the LPC domain operating mode will output the error message
 "EncUsacInit: no td coding mode found for the user defined parameters".

 In order to supply the encoder with a wav file with an appropriate sampling rate a
 resampling step with the help of an external tool may be required. The free resampling
 tool "ResampAudio" is designed to resample audio wav files and can be used for this
 purpose. ResampAudio is part of the AFsp package, which is also required to build the USAC
 Reference Software. Please refer to the documentation which comes with the AFsp software
 package for instructions how to build ResampAudio on your platform.


(8) MPEG-D USAC Decoder Reference Implementation
============================================================================================
 The reference implementation for the MPEG-D USAC decoder can be found in

 /mpegD_usac/usacEncDec/win32/usacDec

 NOTE: The decoder shares its source code with the encoder software. Therefore the path
       to the corresponding Visual Studio solutions is given here. Please check compile.sh
       to get information on the build of the decoder software.


(9) Technical Support
============================================================================================
 For technical support to build and run the MPEG-D USAC reference software,
 please direct your correspondence to:
 christian.neukam@iis.fraunhofer.de
 christian.ertel@iis-extern.fraunhofer.de
 mpeg-h-3da-maintenance@iis.fraunhofer.de


(10) Revision History
============================================================================================
 See file changes.txt for a complete list of changes


(c) Fraunhofer IIS, VoiceAge Corp., 2008-10-08 - 2018-04-11
