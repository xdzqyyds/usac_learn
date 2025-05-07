/**********************************************************************
MPEG-4 Audio VM
Encoder core (LPC-based)



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



Source file: enc_lpc_dmy.c

 

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
20-jun-96   HP    dummy core
14-aug-96   HP    added EncLpcInfo(), EncLpcFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "enc_lpc.h"		/* encoder cores */


/* ---------- declarations ---------- */

#define PROGVER "LPC-based encoder core dummy"


/* ---------- functions ---------- */


/* EncLpcInfo() */
/* Get info about LPC-based encoder core. */
char *EncLpcInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* EncLpcInit() */
/* Init LPC-based encoder core. */

void EncLpcInit (int                numChannel,               /* in: num audio channels           */
                 float              fSample,                  /* in: sampling frequency [Hz]      */
                 float              bitRate,                  /* in: total bit rate [bit/sec]     */
                 char*              encPara,                  /* in: encoder parameter string     */
                 int                varBitRate,               /* in: variable bit rate            */
                 int*               frameNumSample,           /* out: num samples per frame       */
                 int*               delayNumSample,           /* out: encoder delay (num samples) */
                 HANDLE_BSBITBUFFER        bitHeader,                /* out: header for bit stream       */
                 ENC_FRAME_DATA*    frameData,               /* out: system interface            */
                 int                mainDebugLevel 
                 )
{
  CommonExit(1,"EncLpcInit: dummy");
}


/* EncLpcFrame() */
/* Encode one audio frame into one bit stream frame with */
/* LPC-based encoder core. */

void EncLpcFrame (float          **sampleBuf,       /* in: audio frame samples  sampleBuf[numChannel][frameNumSample]      */
                  HANDLE_BSBITBUFFER bitBuf,           /* out: bit stream frame or NULL during encoder start up               */
                  ENC_FRAME_DATA *frameData)
{
  CommonExit(1,"EncLpcFrame: dummy");
}


/* EncLpcFree() */
/* Free memory allocated by LPC-based encoder core. */

void EncLpcFree (void)
{
  CommonExit(1,"EncLpcFree: dummy");
}


/* end of enc_lpc_dmy.c */

