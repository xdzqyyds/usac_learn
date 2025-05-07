************************** MPEG-D USAC Audio Coder - Edition 2.3 ***************************
*                                                                                          *
*                    MPEG-D USAC Audio Reference Software - readme AFsp                    *
*                                                                                          *
*                                        2019-01-17                                        *
*                                                                                          *
********************************************************************************************


(1) Installing AFsp File Programs and Routines
============================================================================================
 To provide support for a variety of audio file formats, the MPEG-D USAC Audio reference
 software uses the AFsp package for audio file i/o and resampling. The AFsp
 package is written by Prof. Peter Kabal <peter.kabal@mcgill.ca> and is
 available via download:

 required version:  AFsp-v9r0.tar.gz
 download site:     http://www.mmsp.ece.mcgill.ca/Documents/Downloads/AFsp/

 NOTE: Make sure to use version v9r0 (AFsp-v9r0.tar.gz)!

 Unpack the downloaded file and copy/paste the whole AFsp content into this folder.

 The directory structure should look as follows (relative to the main readme.txt):
 ./modules/tools/AFsp/audio
                     /filters
                     /include
                     /lib
                     /libAO
                     /libtsp
                     /man
                     /MSVC
                     /scripts

 The Microsoft Visual Studio solution MSVC.sln shall be located at:
   ./tools/AFsp/MSVC/MSVC.sln

 The Unix makefile "Makefile" shall be located at:
   ./tools/AFsp/Makefile

 On Linux/Mac systems the access permissions may have to be set:
 chmod -R u+rwx ./tools/AFsp/

 Please do not touch the directory "scripts" and the containing script compile.bat.
