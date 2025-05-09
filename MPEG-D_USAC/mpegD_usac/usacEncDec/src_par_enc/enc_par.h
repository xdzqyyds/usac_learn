/**********************************************************************
MPEG-4 Audio VM
Encoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Akira Inoue (Sony Corporation) and
Yuji Maeda (Sony Corporation) 

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



Header file: enc_par.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
SE    Sebastien Etienne, Jean Bernard Rault CCETT <jbrault@ccett.fr>

Changes:
14-jun-96   HP    first version
18-jun-96   HP    added bit reservoir handling
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
09-aug-96   HP    added EncXxxInfo(), EncXxxFree()
15-aug-96   HP    changed EncXxxInit(), EncXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
19-feb-97   HP    added include <stdio.h>
08-apr-97   BT    added DEncG729Init() EncG729Frame()
22-apr-99   HP    created from enc.h
**********************************************************************/


#ifndef _enc_par_h_
#define _enc_par_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"

#include "obj_descr.h"		/* AI 990616 */
#include "encoder.h"

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* EncHvxcInfo() */
/* Get info about HVXC encoder core. */

char *EncHvxcInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


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
  );

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
  );

/* EncHvxcFree() */
/* Free memory allocated by HVXC encoder core. */

void EncHvxcFree (void);


/* EncParInfo() */
/* Get info about parametric encoder core. */

char *EncParInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


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
  int mainDebugLevel);


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
  ENC_FRAME_DATA* frameData);	/* in/out: system interface */


/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree (void);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _enc_par_h_ */

/* end of enc_par.h */
