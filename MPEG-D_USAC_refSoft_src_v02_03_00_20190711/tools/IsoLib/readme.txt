************************** MPEG-D USAC Audio Coder - Edition 2.3 ***************************
*                                                                                          *
*                   MPEG-D USAC Audio Reference Software - readme ISOBMFF                  *
*                                                                                          *
*                                        2019-01-17                                        *
*                                                                                          *
********************************************************************************************


(1) Installing MPEG-4 Base Media File Format (ISOBMFF) software
============================================================================================
 The ISO Base Media File Format (ISOBMFF) is an integral part of the MPEG-D USAC Audio
 reference  software. It provides the interface to the mp4 container in which the MPEG-D
 USAC Audio streams are embedded. The ISOBMFF is standardized in ISO/IEC 14996-12 and hosted
 on GitHub.

 GitHub space:      https://github.com/MPEGGroup/isobmff
 download site:     https://github.com/MPEGGroup/isobmff/archive/master.zip

 Unpack the downloaded file and copy/paste the libisomediafile library into this folder.

 The directory structure should look as follows (relative to the main readme.txt):
 ./tools/IsoLib/libisomediafile/linux
                               /macosx
                               /src
                               /w32

 The Microsoft Visual Studio VS2008 solution libisomedia.sln shall be located at:
   ./tools/IsoLib/libisomediafile/w32/libisomediafile/VS2008/libisomedia.sln

 The Microsoft Visual Studio VS2012 solution libisomedia.sln shall be located at:
   ./tools/IsoLib/libisomediafile/w32/libisomediafile/VS2012/libisomedia.sln

 The Unix makefile "Makefile" shall be located at:
   ./tools/IsoLib/libisomediafile/linux/libisomediafile/Makefile

