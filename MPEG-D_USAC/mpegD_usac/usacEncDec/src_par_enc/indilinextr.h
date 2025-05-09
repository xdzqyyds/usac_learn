/**********************************************************************
MPEG4 Audio VM Module
parameter based codec - HILN: harmonic/individual lines and noise
                              extraction



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

Copyright (c) 1997.



Header file: indilinextr.h


Required libraries:
libuhd_psy.a		indiline psychoacoustic model

Required modules:
indilinecom.o		indiline common module
uhd_fft.o		FFT and ODFT module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
01-sep-96   HP    first version based on indiline.c
10-sep-96   BE
11-sep-96   HP
26-sep-96   HP    new indiline module interfaces
29-nov-96   HP    updated to modular code structure by BE
10-feb-97   HP    updated to modular freq. estim. by BE
27-feb-97   HP    added IndiLineExtractFree()
10-apr-97   HP    harmonic stuff ...
18-apr-97   HP    noisy stuff ...
19-jun-97   HP    improved env/harm/noise interface
16-feb-98   HP    improved noiseFreq
09-apr-98   HP    ILXconfig
24-feb-99   NM/HP LPC noise
10-sep-99   HP    prev/nextSamples
29-jan-01   BE    float -> double
**********************************************************************/


#ifndef _indilinextr_h_
#define _indilinextr_h_

#include "indilinecom.h"	/* indiline common module */

/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

typedef struct ILXstatusStruct ILXstatus;	/* ILX status handle */

typedef struct ILXconfigStruct ILXconfig;	/* ILX configuration */

struct ILXconfigStruct
{
  float dummy1;
  float dummy2;
  int dummy3;
  float dummy4;
  int dummy5;
  int dummy6;
  int dummy7;
  char dummy8;
  float dummy9;
  int dummy10;
  float dummy11;
  int dummy12;
  int dummy13;
  float dummy14;
  float dummy15;
  float dummy21;
  int dummy16;
  float dummy17;
  float dummy18;
  float dummy19;
  float dummy20;
  float dummy22;
  int dummy23;
  float dummy24;
  float dummy25;
  int dummy26;
};


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* IndiLineExtractInit() */
/* Init individual lines extraction. */

ILXstatus *IndiLineExtractInit (
  int frameLen,			/* in: samples per frame */
  double fSample,		/* in: sampling frequency */
  double maxLineFreq,		/* in: max line frequency */
  double maxLineAmpl,		/* in: max line amplitude */
  int maxNumLine,		/* in: max num lines */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumHarm,		/* in: max num harmonic tones */
  int maxNumHarmLine,		/* in: max num lines of harmonic tone */
  int maxNumNoisePara,		/* in: max num noise parameters */
  double maxNoiseFreq,		/* in: max noise frequency */
  ILXconfig *cfg,		/* in: ILX configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int bsFormat,			/* in: bitstream format	*/
				/*     <4: DCT-noise  >=4: LPC-noise */
  int prevSamples,		/* in: num samples of previous frame */
  int nextSamples);		/* in: num samples of next frame */
				/* returns: ILX status handle */


/* IndiLineExtract() */
/* Extraction of individual lines and envelope from the current */
/* residual signal. */

void IndiLineExtract (
  ILXstatus *ILX,		/* in: ILX status handle */
  float *residualSignal,	/* in: residual signal */
				/*  [0..prevSamples+frameLen+nextSamples-1] */
				/*  prevSamples  previous frame, */
				/*  frameLen     current frame, */
				/*  nextSamples  next frame */
  float *synthSignal,		/* in: synthesised signal */
				/*  [as residualSignal] */
				/*  (currently not used) */
  float *finePitchPeriod,	/* in: (currently not used) */
  float *slowHarm,		/* in: (currently not used) */
  float *LPCcoef,		/* in: (currently not used) */
  int numLine,			/* in: num lines to extract */
				/*     -1 to disable extraction */
  int *numEnv,			/* out: num envelopes */
  double ***envPara,		/* out: envelope parameters */
				/*      [0..numEnv-1][0..ENVPARANUM-1] */
  int *numLineExtract,		/* out: num lines extracted */
  double **lineFreq,		/* out: line frequency [Hz] */
				/*      [0..numLine-1] */
  double **lineAmpl,		/* out: line amplitude */
				/*      [0..numLine-1] */
  double **linePhase,		/* out: line phase [rad] */
				/*      [0..numLine-1] */
  int **lineEnv,		/* out: line envelope flag/idx */
				/*      [0..numLine-1] */
  int **linePred,		/* out: line predecessor idx */
				/*      [0..numLine-1] */
  int *numHarm,			/* out: num harmonic tones */
  int **numHarmLine,		/* out: num lines of harmonic tone */
				/*      [0..numHarm-1] */
  double **harmFreq,		/* out: harm fundamental frequency [Hz] */
				/*      [0..numHarm-1] */
  double **harmFreqStretch,	/* out: harm freq stretch ratio */
				/*      [0..numHarm-1] */
  int **harmLineIdx,		/* out: harm line para start idx */
				/*      [0..numHarm-1] */
				/*      ampl: lineAmpl[idx] etc. */
				/*      idx=harmLineIdx+(0..numHarmLine-1) */
  int **harmEnv,		/* out: harm envelope flag/idx */
				/*      [0..numHarm-1] */
  int **harmPred,		/* out: harm tone predecessor idx */
				/*      [0..numHarm-1] */
  double **harmRate,		/* out: bitrate for harm data (0..1) */
				/*      [0..numHarm-1] */
  int *numNoisePara,		/* out: num noise parameter */
  double *noiseFreq,		/* out: max noise freq (bandwidth) [Hz] */
  double **noisePara,		/* out: noise parameter (DCT) */
				/*      DCT[] / StdDev+RefCoef[] */
				/*      [0..numNoisePara-1] */
  int *noiseEnv,		/* out: noise envelope flag/idx */
  double *noiseRate);		/* out: bitrate for noise data (0..1) */


/* IndiLineExtractFree() */
/* Free memory allocated by IndiLineExtractInit(). */

void IndiLineExtractFree (
  ILXstatus *ILX);		/* in: ILX status handle */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilinextr_h_ */

/* end of indilinextr.h */
