/**********************************************************************
MPEG-4 Audio VM
Encoder core (t/f-based)



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



Source file: enc_tf_dmy.c

 

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
20-jun-96   HP    dummy core
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
14-aug-96   HP    added EncTfInfo(), EncTfFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
01-apr-97   HP    updated EncTfInit() params
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "enc_tf.h"		/* encoder cores */


/* ---------- declarations ---------- */

#define PROGVER "t/f-based encoder core dummy"


/* ---------- functions ---------- */


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */
char *EncTfInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* EncTfInit() */
/* Init t/f-based encoder core. */

void EncTfInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int quantDebugLevel,          /* in: quantDebugLevel */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,
  TF_DATA* tfData,
  ntt_DATA* nttData,
  int mainDebugLevel
)
{
  CommonExit(1,"EncTfInit: dummy module");
}


/* EncTfFrame() */
/* Encode one audio frame into one bit stream frame with */
/* t/f-based encoder core. */

void EncTfFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  long bitRateLong,
  long numSampleLong,
  ENC_FRAME_DATA* frameData,
  TF_DATA* tfData,
  ntt_DATA* nttData)
{
  CommonExit(1,"EncTfFrame: dummy module");
}


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree (void)
{
  CommonExit(1,"EncTfFree: dummy module");
}


/* end of enc_tf_dmy.c */

