************************** MPEG-D USAC Audio Coder - Edition 2.3 ***************************
*                                                                                          *
*                   MPEG-D USAC Audio Reference Software - readme SSNRCD                   *
*                                                                                          *
*                                        2019-01-17                                        *
*                                                                                          *
********************************************************************************************

(1) Overview
============================================================================================
 This tool implements the MPEG-2 conformance test for AAC, certain MPEG-4 conformance tests,
 MPEG-D USAC conformance tests and MPEG-H 3D Audio conformance tests according to:
 - ISO/IEC 13818-4
 - ISO/IEC 14496-4
 - ISO/IEC 23003-3
 - ISO/IEC 23008-9

 The conformance testing is based on the following calculated values:
 - RMS value of the difference signal
 - maximum absolute difference
 - segmental SNR
 - average cepstral distortion

(2) build instructions
============================================================================================
 The tools ssnrcd and mpegaudioconf are build together with the MPEG-D USAC Audio reference
 software when executing the corresponding compile scripts compile.bat/compile.sh.

(a) Linux/Mac:
--------------

 - Change directory into scripts folder (cd ./scripts).
 - Execute ./compile.sh to automatically compile all modules and tools.
 - For individual building of ssnrcd and mpegaudioconf:
   . Change directory into ./tools/ssnrcd/make
   . Execute make
 - for building ssnrcd only:
   . Change directory into ./tools/ssnrcd/make
   . Execute make ssnrcd

 - for building mpegaudioconf only:
   . Change directory into ./tools/ssnrcd/make
   . Execute make mpegaudioconf

(b) Windows:
------------

 - Change directory into scripts folder (cd ./scripts).
 - Execute compile.bat to automatically compile all modules and tools.

(3) Usage
============================================================================================
 Both tools ssnrcd and mpegaudioconf share the same command line parameters:

 ssnrcd [options] <reference inputfile> <inputfile under test>
 mpegaudioconf [options] <reference inputfile> <inputfile under test>

 Valid options are:
  -a           Show distribution of bit accuracy over all samples
  -d <num>     Time-offset in samples between reference inputfile
               and inputfile under test. Positive value means
               inputfile under test is delayed over <num> samples
               and the first <num>  samples of that input file
               under test are skipped. Negative values are.
               permitted.
               Default: 0 samples.
  -e [<n>]     Exponential output of values scaled to 2^n. If no
               n is given, bit resolution of file under test will
               be used.
  -k <num>     Resolution for calculation of RMS and max. abs.
               difference sample, expressed in bits.
               Valid range:
                 [1...24]
               Default: 16 bits.
  -s <num>     Threshold for the Segmental SNR criterion in dB.
               <num> shall be an integer value the range
                 [0...100].
               Default: 30 dB.
  -l <num>     Analysis segment length in samples.
               Default: 320 samples.
  -t <num>     Conformance criteria to be applied.
                 0 : No criterion (default)
                 1 : RMS + Max. absolute difference sample
                     using K bit resolution (see -k option)
                 2 : Segmental SNR + avg. Cepstral Distortion
                 4 : Number of bits accuracy reached for RMS
                     criterion
                 7 : enable all test critaria
  -v [<num>]   Verbose level.
                 1 : Progress report (default)
                 2 : Show segments with accuracy below K bit
                     resolution (see -k option)
                 3 : Samples with accuracy below K bit
                     resolution (see -k option)
                 4 : Show accuracy of all samples
  -p <num> <frameSize> PNS conformance test
          <num>:
                 1 : PNS-1 (spectral PNS test for long  blocks)
                 2 : PNS-2 (spectral PNS test for short blocks)
                 3 : PNS-3 (temporal PNS test for short blocks)
          <frameSize>:
                 480 : for testCase 1
                 512 : for testCase 1
                 960 : for testCase 1|2|3
                1024 : for testCase 1|2|3

(3) Constraints
============================================================================================
 If the input files have different lengths, only the number of samples in  the shorter file
 will be used. The additional samples at the end of the  longer file will be ignored.

 Segmental SNR and cepstral distortion are calculated only for segments  with reference
 signal power in the range [-50...-15] dB. Any partial  segments at the end of the input
 signal are padded with zeroes when  calculating segmental SNR and cepstral distortion.
 The length of the  segments can be set by using the -l option.

 Both input files must have the same number of audio channels and the  same sampling rate.
 Audio files in wav, aiff, aifc and au format are accepted. The maximum number of audio
 channels is 64.
