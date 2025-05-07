/**********************************************************************
MPEG4 Audio VM Module
parameter based codec - HILN: harmonic/individual lines and noise
                              bitstream encoder



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



Header file: indilineenc.h

 

Required libraries:
(none)

Required modules:
bitstream.o		bit stream module
indilinecom.o		indiline common module
indilineqnt.o		indiline quantiser module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
25-sep-96   HP    first version based on enc_par.c
26-sep-96   HP    new indiline module interfaces
11-apr-97   HP    harmonic stuff ...
22-apr-97   HP    noisy stuff ...
20-jun-97   HP    improved env/harm/noise interface
16-nov-97   HP    harm/indi enha mode
16-feb-98   HP    improved noiseFreq
09-apr-98   HP    ILEconfig
24-feb-99   NM/HP LPC noise, bsf=4
30-jan-01   BE    float -> double
**********************************************************************/


#ifndef _indilineenc_h_
#define _indilineenc_h_

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

typedef struct ILEstatusStruct ILEstatus;	/* ILE status handle */

typedef struct ILEconfigStruct ILEconfig;	/* ILE configuration */

struct ILEconfigStruct
{
  int quantMode;		/* harm/indi quant mode (-1=auto 0..3) */
  int enhaQuantMode;		/* harm/indi enha quant mode (-1=auto 0..3) */
  int contMode;			/* harm/indi continue mode (0=hi, 1=hi&ii) */
  int encContMode;		/* indi encoder continue mode */
				/* (0=cont 1=no-cont) */
  int bsFormat;			/* HILN bitstream format (0=VM 1=9705 2=CD) */
  
  int phaseFlag;		/* harm/indi phase flag (0=random 1=determ) (needs bsf=5) */
  int maxPhaseIndi;		/* max num indi lines with phase (base layer) */
  int maxPhaseExt;		/* max num indi lines with phase (extension layer) */
  int maxPhaseHarm;		/* max num harm lines with phase */

  int dummy1a;
  int dummy1b;
  int dummy1c;
  int dummy2;
  int dummy3;
  float dummy4;
  float dummy5;
  int dummy6;
  int dummy7;
  float dummy8;
  float dummy9;
};


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* IndiLineEncodeInit() */
/* Init individual lines bitstream encoder. */

ILEstatus *IndiLineEncodeInit (
  int frameLenQuant,		/* in: samples per frame */
				/*     used for quantiser selection */
  double fSampleQuant,		/* in: sampling frequency */
				/*     used for quantiser selection */
  double maxLineAmpl,		/* in: max line amplitude */
  int enhaFlag,			/* in: enha flag (0=basic 1=enha) */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int maxNumLine,		/* in: max num lines */
				/*     if maxNumLine < 0 */
				/*     calc maxNumLineBS from frameMaxNumBit */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumHarm,		/* in: max num harmonic tones */
  int maxNumHarmLine,		/* in: max num lines of harmonic tone */
  int maxNumNoisePara,		/* in: max num noise parameters */
  ILEconfig *cfg,		/* in: ILE configuration */
				/*     or NULL */
  int debuglevel,		/* in: debug level (0=off) */
  HANDLE_BSBITSTREAM hdrStream,	/* out: bit stream header */
  int *maxNumLineBS,		/* out: max num lines in bitstream */
				/*      is maxNumLine if >= 0 */
  double *maxNoiseFreq);	/* out: max noise freq */
				/* returns: ILE status handle */


/* IndiLineEncodeFrame() */
/* Encode individual lines frame to bitstream. */

void IndiLineEncodeFrame (
  ILEstatus *ILE,		/* in: ILE status handle */
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
  int *lineEnv,			/* in: line envelope flag/idx */
				/*     [0..numLine-1] */
  int *linePred,		/* in: line predecessor idx */
				/*     [0..numLine-1] */
  int numHarm,			/* in: num harmonic tones */
  int *numHarmLine,		/* in: num lines of harmonic tone */
				/*     [0..numHarm-1] */
  double *harmFreq,		/* in: harm fundamental frequency [Hz] */
				/*     [0..numHarm-1] */
  double *harmFreqStretch,	/* in: harm freq stretch ratio */
				/*     [0..numHarm-1] */
  int *harmLineIdx,		/* in: harm line para start idx */
				/*     [0..numHarm-1] */
				/*     ampl: lineAmpl[idx] etc. */
				/*     idx=harmLineIdx+(0..numHarmLine-1) */
  int *harmEnv,			/* in: harm envelope flag/idx */
				/*     [0..numHarm-1] */
  int *harmPred,		/* in: harm tone predecessor idx */
				/*     [0..numHarm-1] */
  double *harmRate,		/* in: bitrate for harm data (0..1) */
				/*     [0..numHarm-1] */
  int numNoisePara,		/* in: num noise parameter */
  double noiseFreq,		/* in: max noise freq (bandwidth) [Hz] */
  double *noisePara,		/* in: noise parameter */
				/*     DCT[] / StdDev+RefCoef[] */
				/*     [0..numNoisePara-1] */
  int noiseEnv,			/* in: noise envelope flag/idx */
  double noiseRate,		/* in: bitrate for noise data (0..1) */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int numExtLayer,		/* in: num extension layers */
  int *extLayerNumBit,		/* in: num bits for extension layer(s) */
				/*     [0..numExtLayer-1] */
				/*     or NULL */
  HANDLE_BSBITSTREAM *streamESC,	/* out: bit stream (ESC 0..4) */
  HANDLE_BSBITSTREAM streamEnha,	/* out: enhancement bit stream */
  HANDLE_BSBITSTREAM *streamExt);	/* out: extension layer bit stream(s) */
				/*      [0..numExtLayer-1] */
				/*      or NULL */


/* IndiLineEncodeFree() */
/* Free memory allocated by IndiLineEncodeInit(). */

void IndiLineEncodeFree (
  ILEstatus *ILE);		/* in: ILE status handle */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilineenc_h_ */

/* end of indilineenc.h */
