/**********************************************************************
MPEG-4 Audio VM
Common module


This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Header file: common.h

$Id: common.h,v 1.1.1.1 2009-05-29 14:10:14 mul Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-jun-96   HP    new module based on ErrorMsg() from cmdline module
25-jun-96   HP    added CommonFreeAlloc()
04-jul-96   HP    added stdlib.h
26-aug-96   HP    CVS
**********************************************************************/


#ifndef _common_h_
#define _common_h_

#include <stdlib.h>		/* typedef size_t */

#ifdef MPEG4V1
#define PROGVER "MPEG4V1 AAC Audio Encoder 01-dec-98"
#else
#define PROGVER "MPEG2 AAC Audio Encoder 01-dec-98"
#endif

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* if static memory allocation is used, this value tells the max. nr of 
   audio channels to be supported */

#define MAX_CHANNELS 9
#define MAX_CPL_CHANNELS 1
#define MAX_LFE_CHANNELS 1
#define MAX_TIME_CHANNELS (MAX_CHANNELS+MAX_CPL_CHANNELS+MAX_LFE_CHANNELS)

/* CommonFreeAlloc() */
/* Free previously allocated memory if present, then allocate new memory. */
void *CommonFreeAlloc(
  void **ptr,			/* in/out: addr of memory pointer */
  size_t size);			/* in: memory size */
				/* returns: memory pointer */
				/*          or NULL if error*/


/* CommonProgName() */
/* Set program name for error/warning message. */
void CommonProgName(
  char *progName);		/* in: program name */


/* CommonWarning() */
/* Print program name and warning message to stderr. */
void CommonWarning(
  char *message,		/* in: warning message */
  ...);				/* in: args as for printf */

/* CommonExit() */
/* Print program name and error message to stderr and exit program. */
void CommonExit(
  int errorCode,		/* in: error code for exit() */
  char *message,		/* in: error message */
  ...);				/* in: args as for printf */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _common_h_ */

/* end of common.h */

