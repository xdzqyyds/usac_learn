/**********************************************************************
MPEG4 Audio VM Module
parameter based codec - individual lines: bitstream decoder



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)

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



Header file: indilinedec.h

Required libraries:
(none)

Required modules:
indilinecom.o		indiline common module
indilinecom.o		indiline common module
indilineqnt.o		indiline quantiser module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
25-sep-96   HP    first version based on dec_par.c
26-sep-96   HP    new indiline module interfaces
11-apr-97   HP    harmonic stuff ...
22-apr-97   HP    noisy stuff ...
23-jun-97   HP    improved env/harm/noise interface
24-jun-97   HP    added IndiLineDecodeHarm()
16-nov-97   HP    harm/indi cont mode
30-mar-98   BE/HP initPrevNumLine
06-apr-98   HP    ILDconfig
06-may-98   HP    linePhaseValid
11-jun-98   HP    harmFreq optional in IndiLineDecodeHarm()
24-feb-99   NM/HP LPC noise, bsf=4
25-jan-01   BE    float -> double
**********************************************************************/


#ifndef _indilinedec_h_
#define _indilinedec_h_

#include "bitstream.h"		/* bit stream module */

#include "indilinecom.h"	/* indiline common module */
#include "indilineqnt.h"	/* indiline quantiser module */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

typedef struct ILDstatusStruct ILDstatus;	/* ILD status handle */

typedef struct ILDconfigStruct ILDconfig;	/* ILD configuration */

struct ILDconfigStruct
{
  int contModeTest;	/* harm/indi continue mode */
			/* (-1=bitstream 0=hi, 1=hi&ii) */
  int bsFormat;		/* HILN bitstream format (0=VM 1=9705 2=CD) */
  int noStretch;	/* disable stretching */
  float contdf;		/* line continuation df */
  float contda;		/* line continuation da */
};


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* IndiLineDecodeInit() */
/* Init individual lines bitstream decoder. */


ILDstatus *IndiLineDecodeInit (
  HANDLE_BSBITSTREAM hdrStream,	/* in: bit stream header */
  double maxLineAmpl,		/* in: max line amplitude */
  int initPrevNumLine,		/* in: number ind. lines in prev. frame */
  ILDconfig *cfg,		/* in: ILD configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int *maxNumLine,		/* out: max num lines in bitstream */
  int *maxNumEnv,		/* out: max num envelopes */
  int *maxNumHarm,		/* out: max num harmonic tones */
  int *maxNumHarmLine,		/* out: max num lines of harmonic tone */
  int *maxNumNoisePara,		/* out: max num noise parameters */
  int *maxNumAllLine,		/* out: max num harm+indi lines */
  int *enhaFlag,		/* out: enha flag (0=basic 1=enha) */
  int *frameLenQuant,		/* out: samples per frame */
				/*      used for quantiser selection */
  double *fSampleQuant);		/* out: sampling frequency */
				/*      used for quantiser selection */
				/* returns: ILD status handle */


/* IndiLineDecodeFrame() */
/* Decode individual lines frame from bitstream. */

void IndiLineDecodeFrame (
  ILDstatus *ILD,		/* in: ILD status handle */
  HANDLE_BSBITSTREAM streamESC[5],	/* in: bit stream (ESC 0..4) */
  HANDLE_BSBITSTREAM streamEnha,	/* in: enhancement bit stream */
				/*     or NULL for basic mode */
  int numExtLayer,		/* in: num extension layers */
  HANDLE_BSBITSTREAM *streamExt,	/* in: extension layer bit stream(s) */
				/*     [0..numExtLayer-1] */
				/*     or NULL */
  int *numEnv,			/* out: num envelopes */
  double ***envPara,		/* out: envelope parameters */
				/*      [0..numEnv-1][0..ENVPARANUM-1] */
  int *numLine,			/* out: num lines */
  double **lineFreq,		/* out: line frequencies */
				/*      [0..numLine-1] */
  double **lineAmpl,		/* out: line amplitudes */
				/*      [0..numLine-1] */
  double **linePhase,		/* out: line phases */
				/*      [0..numLine-1] */
				/*      (not used in basic mode) */
  int **linePhaseValid,		/* out: line phases validity flag */
				/*      [0..numLine-1] */
				/*      (not used in basic mode) */
  int **lineEnv,		/* out: line envelope flags */
				/*      [0..numLine-1] */
  int **linePred,		/* out: line predecessor indices */
				/*      [0..numLine-1] */
  int *numHarm,			/* out: num harmonic tones */
  int **numHarmLine,		/* out: num lines of harmonic tone */
				/*      [0..numHarm-1] */
  double **harmFreq,		/* out: harm fundamental frequency [Hz] */
				/*      [0..numHarm-1] */
  double **harmFreqStretch,	/* out: harm freq stretching */
				/*      [0..numHarm-1] */
  int **harmLineIdx,		/* out: harm line para start idx */
				/*      [0..numHarm-1] */
				/*      ampl: lineAmpl[idx] etc. */
				/*      idx=harmLineIdx+ */
				/*          (0..numHarmLine-1) */
  int **harmEnv,		/* out: harm envelope flag/idx */
				/*      [0..numHarm-1] */
  int **harmPred,		/* out: harm tone predecessor idx */
				/*      [0..numHarm-1] */
  int *numNoisePara,		/* out: num noise parameter */
  double *noiseFreq,		/* out: max noise freq (bandwidth) [Hz] */
  double **noisePara,		/* out: noise parameter */
				/*      DCT[] / StdDev+RefCoef[] */
				/*      [0..numNoisePara-1] */
  int *noiseEnv);		/* out: noise envelope flag/idx */


/* IndiLineDecodeHarm() */
/* Append harmonic lines and join with individual lines. */

void IndiLineDecodeHarm (
  ILDstatus *ILD,		/* in: ILD status handle */
  int numLine,			/* in: num lines */
  double *lineFreq,		/* in: line frequency */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  double *lineAmpl,		/* in: line amplitude */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  double *linePhase,		/* in: line phase */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     (NULL in basic mode) */
				/*     Note: values are MODIFIED !!! */
				/*     Note: values are MODIFIED !!! */
  int *linePhaseValid,		/* in: line phase validity flag */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     Note: values are MODIFIED !!! */
  int *lineEnv,			/* in: line envelope flag */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  int *linePred,		/* in: line predecessor idx */
				/*     in prev frame */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  int numHarm,			/* in: num harmonic tones */
  int *numHarmLine,		/* in: num lines of harmonic tone */
				/*     [0..numHarm-1] */
  double *harmFreq,		/* in: harm fundamental frequency [Hz] */
				/*     [0..numHarm-1] */
				/*     (NULL to copy freq&env from lineFreq) */
  double *harmFreqStretch,	/* in: harm freq stretch ratio */
				/*     [0..numHarm-1] */
  int *harmLineIdx,		/* in: harm line para start idx */
				/*     [0..numHarm-1] */
				/*     ampl: lineAmpl[idx] etc. */
				/*     idx=harmLineIdx+(0..numHarmLine-1) */
				/*     Note: values are MODIFIED !!! */
  int *harmEnv,			/* in: harm envelope flag/idx */
				/*     [0..numHarm-1] */
  int *harmPred,		/* in: harm tone predecessor idx */
				/*     [0..numHarm-1] */
  int *numAllLine);		/* out: num harm+indi lines */


/* IndiLineDecodeFree() */
/* Free memory allocated by IndiLineDecodeInit(). */

void IndiLineDecodeFree (
  ILDstatus *ILD);		/* in: ILD status handle */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilinedec_h_ */

/* end of indilinedec.h */
