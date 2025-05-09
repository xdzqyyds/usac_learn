/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:


and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "enc_usac.h"		/* encoder cores */


/* ---------- declarations ---------- */

#define PROGVER "USAC encoder dummy"


/* ---------- functions ---------- */


/* EncUsacInfo() */
/* Get info about USAC encoder. */
char *EncUsacInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* EncUsacInit() */
/* Init USAC encoder. */

void EncUsacInit (
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
  CommonExit(1,"EncUsacInit: dummy module");
}


/* EncTfFrame() */
/* Encode one audio frame into one bit stream frame with */
/* t/f-based encoder core. */

void EncUsacFrame (
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
  CommonExit(1,"EncUsacFrame: dummy module");
}


/* EncUsacfFree() */
/* Free memory allocated by USAC encoder. */

void EncUsacFree (void)
{
  CommonExit(1,"EncUsacFree: dummy module");
}


/* end of enc_usac_dmy.c */

