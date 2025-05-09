/*
This software module was originally developed by 
Ralf Geiger (Fraunhofer IIS AEMT) 
in the course of development of the MPEG-4 extension 3 
audio scalable lossless (SLS) coding . This software module is an 
implementation of a part of one or more MPEG-4 Audio tools as 
specified by the MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. 

Copyright 2003.  
*/


#ifndef _INT_MDCT_DEFS_H_
#define _INT_MDCT_DEFS_H_

#define FRAME_LEN_LONG 1024
#define FRAME_LEN_SHORT 128
#define INTMDCT_BUFFER_SIZE (FRAME_LEN_LONG+FRAME_LEN_LONG/2)
#define SINE_DATA_SIZE 8192
#define SHIFT 30
#define SHIFT_FOR_ERROR_FEEDBACK 6

/* #define FLIP_GRP_SFB */

#ifndef ABS
#define ABS(A) ((A) < 0 ? (-A) : (A))
#endif

#ifdef WIN32
	typedef __int64 INT64;
#else
	#include <stdint.h>
	typedef int64_t INT64;
#endif


#define ONLY_LONG_WINDOW ONLY_LONG_SEQUENCE
#define LONG_START_WINDOW LONG_START_SEQUENCE
#define EIGHT_SHORT_WINDOW EIGHT_SHORT_SEQUENCE
#define LONG_STOP_WINDOW LONG_STOP_SEQUENCE
#define ONLY_LONG_WINDOW ONLY_LONG_SEQUENCE

#endif
