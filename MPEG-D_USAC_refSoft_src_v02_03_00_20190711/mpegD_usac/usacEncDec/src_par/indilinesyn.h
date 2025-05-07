/**********************************************************************
MPEG4 Audio VM Module
parameter based codec - individual lines: synthesis



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



Header file: indilinesyn.h


Required libraries:
(none)

Required modules:
indilinecom.o		indiline common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
01-sep-96   HP    first version based on indiline.c
10-sep-96   BE
11-sep-96   HP
26-sep-96   HP    new indiline module interfaces
17-jan-97   HP    fixed linePred>prevNumLine bug
22-apr-97   HP    noisy stuff ...
11-jun-97   HP    added IndiLineSynthFree()
23-jun-97   HP    improved env/harm/noise interface
06-apr-98   HP    ILSconfig
06-may-98   HP    linePhaseValid
24-feb-99   NM/HP LPC noise
25-jan-01   BE    float -> double
**********************************************************************/


#ifndef _indilinesyn_h_
#define _indilinesyn_h_

#include "indilinecom.h"	/* indiline common module */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

typedef struct ILSstatusStruct ILSstatus;	/* ILS status handle */

typedef struct ILSconfigStruct ILSconfig;	/* ILS configuration */

struct ILSconfigStruct
{
  int rndPhase;		/* random startphase (0=sin(0) 1=sin(rnd)) */
  int lineSynMode;	/* line syt. mode (0=soft-lpf 1=hard-lpf 2=fast) */
};


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ----- initialisation ----- */

/* IndiLineInitSynthInit() */
/* Init individual lines synthesiser. */

ILSstatus *IndiLineSynthInit (
  int maxFrameLen,		/* in: max num samples per frame */
  double fSample,		/* in: sampling frequency */
  int maxNumLine,		/* in: max num lines */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumNoisePara,		/* in: max num noise parameters */
  ILSconfig *cfg,		/* in: ILS configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int bsFormat);		/* in: bit stream format	*/
				/*     <4: DCT-noise  >=4: LPC-noise */
				/* returns: ILS status handle */


/* ----- synthesiser ----- */

/* IndiLineSynth() */
/* Synthesis of individual lines according to envelope and line parameters. */
/* enhaMode=0: For basic bitstream: phase information updated internally! */
/* enhaMode=1: For basic+enhancement bitstream: phase information required! */

void IndiLineSynth (
  ILSstatus *ILS,		/* ILS status handle */
  int enhaMode,			/* in: 0=basic mode  1=enhanced mode */
  int frameLen,			/* in: num samples in current frame */
  int numEnv,			/* in: num envelopes */
  double **envPara,		/* in: envelope parameters */
				/*     [0..numEnv-1][0..ENVPARANUM-1] */
  int numLine,			/* in: num lines */
  double *lineFreq,		/* in: line frequency */
				/*     [0..numLine-1] */
  double *lineAmpl,		/* in: line amplitude */
				/*     [0..numLine-1] */
  double *linePhase,		/* in: line phase */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     Note: values may be MODIFIED !!! */
  int *linePhaseValid,		/* in: line phase validity flag */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
  double *linePhaseDelta,	/* in: line phase deviation from */
				/*     linear sweep for cont lines */
				/*     [0..numLine-1] */
				/*     or NULL if not available */
				/*     (utilised only in enha mode) */
  int *lineEnv,			/* in: line envelope flag */
				/*     [0..numLine-1] */
  int *linePred,		/* in: line predecessor idx */
				/*     in prev frame */
				/*     [0..numLine-1] */
  int numNoisePara,		/* in: num noise parameter */
  double noiseFreq,		/* in: max noise freq (bandwidth) [Hz] */
  double *noisePara,		/* in: noise parameter */
				/*     DCT[] / StdDev+RefCoef[] */
				/*     [0..numNoisePara-1] */
  int noiseEnv,			/* in: noise envelope flag/idx */
  float *synthSignal);		/* out: synthesised signal */
				/*      [0..frameLen-1] */
				/*      2nd half previous frame and */
				/*      1st half current frame */


/* IndiLineSynthFree() */
/* Free memory allocated by IndiLineSynthInit(). */

void IndiLineSynthFree (
  ILSstatus *ILS);		/* ILS status handle */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilinesyn_h_ */

/* end of indilinesyn.h */
