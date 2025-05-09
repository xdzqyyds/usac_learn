/**********************************************************************
MPEG-4 Audio VM
Encoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Source file: enc_par_dmy.c

 

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
20-jun-96   HP    dummy core
14-aug-96   HP    added EncParInfo(), EncParFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "enc_par.h"		/* encoder cores */


/* ---------- declarations ---------- */

#define PROGVER "parametric encoder core dummy"


/* ---------- functions ---------- */


/* EncParInfo() */
/* Get info about parametric encoder core. */

char *EncParInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* EncParInit() */
/* Init parametric encoder core. */

void EncParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,	/* out: system interface */
  int mainDebugLevel)
{
  CommonExit(1,"EncParInit: dummy");
}


/* EncParFrame() */
/* Encode one audio frame into one bit stream frame with */
/* parametric encoder core. */

void EncParFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  ENC_FRAME_DATA* frameData)	/* in/out: system interface */
{
  CommonExit(1,"EncParFrame: dummy");
}


/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree (void)
{
  CommonExit(1,"EncParFree: dummy");
}


/* EncHvxcInit() */
/* Init HVXC encoder core. */

void EncHvxcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,	/* out: system interface(AI 990616) */
  int mainDebugLevel  
  )
{
  CommonExit(1,"EncHvxcInit: dummy");
}


/* EncHvxcFrame() */
/* Encode one audio frame into one bit stream frame with */
/* HVXC encoder core. */

void EncHvxcFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  ENC_FRAME_DATA* frameData	/* in/out: system interface(AI 990616) */
  )
{
  CommonExit(1,"EncHvxcFrame: dummy");
}


/* EncHvxcFree() */
/* Free memory allocated by HVXC encoder core. */

void EncHvxcFree (void)
{
  CommonExit(1,"EncHvxcFree: dummy");
}


/* end of enc_par_dmy.c */

