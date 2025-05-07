/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - individual lines: quantisation & dequantisation



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



Source file: indilineqnt.c


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
04-nov-96   HP    fixed bug in enhancement freq. quant. bit alloc.
23-jun-97   HP    moved ILQ stuff to c file
03-jul-97   HP    fixed minor bug (debug output) in IndiLineEnvDequant()
16-nov-97   HP    harm/indi quant mode
16-feb-98   HP    improved noiseFreq
17-feb-98   HP    fixed fnumbite error
30-mar-98   BE/HP lineValid
31-mar-98   HP    completed IndiLineDecodeFree()
26-jan-01   BE    float -> double
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "indilinecom.h"	/* indiline common module */
#include "indilineqnt.h"	/* indiline quantiser module */

#include "indilinecfg.h"	/* indiline quantiser configuration */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define MINENVAMPL 0.1


/* ---------- declarations (data structures) ---------- */

#define MAX_FBITSE 7		/* max num bits for freq in enhancement strm */

/* status variables and arrays for individual line quantisation (ILQ) */

struct ILQstatusStruct		/* ILQ status handle struct */
{
  /* general parameters */
  int debugMode;		/* debug mode (0=off) */
  int quantMode;		/* quant mode */
  int enhaQuantMode;		/* enha quant mode */
  double maxLineAmpl;		/* max. allowed amplitude */
  int maxNumLine;		/* maximum number of lines */

  int bsFormat;			/* bitstream format	*/

  /* quantiser parameters */
  double freqMin;
  double freqMax;
  double amplMin;
  double amplMax;
  double deltaFreqMax;
  double deltaAmplMax;
  double envAngle45Time;

  int freqNumBit;
  int amplNumBit;
  int freqNumStep;
  int amplNumStep;
  double freqRange;
  double amplRange;
  double freqRelStep;

  int deltaFreqNumBit;
  int deltaAmplNumBit;
  int deltaFreqNumStep;
  int deltaAmplNumStep;
  double deltaFreqRange;
  double deltaAmplRange;
  
  int maxFIndex;

  int envTmNumBit;
  int envAtkNumBit;
  int envDecNumBit;
  int envTmNumStep;
  int envAtkNumStep;
  int envDecNumStep;

  int freqThrE[MAX_FBITSE];
  int freqNumStepE[MAX_FBITSE];

  int phaseNumBitE;
  int phaseNumStepE;
  double phaseRangeE;

  int envTmNumBitE;
  int envAtkNumBitE;
  int envDecNumBitE;
  int envTmNumStepE;
  int envAtkNumStepE;
  int envDecNumStepE;

  /* pointers to variable content arrays */
  double *quantLineAmplE;	/* line end amplitude  [0..maxNumLine-1] */
  int *freqEnhaNumBit;		/* number of f enh. bits (dequant only) */

  /* pointers to arrays for quantiser return values */
  /* baisc bitstream */
  int *lineParaPred;		/* line predecessor idx in prev frame */
				/*   [0..maxNumLine-1] */
  unsigned long *lineParaIndex;	/* line para bits  [0..maxNumLine-1] */
  int *lineParaNumBit;		/* num line para bits  [0..maxNumLine-1] */

  int *lineParaFIndex;		/* Frequency index	*/
  int *lineParaAIndex;		/* Amplitude index	*/
	
  /* enhancement bitstream */
  unsigned long *lineParaEnhaIndex;
				/* line para bits  [0..maxNumLine-1] */
  int *lineParaEnhaNumBit;	/* num line para bits  [0..maxNumLine-1] */

  /* line transmission sequence */
  int *lineSeq;			/* line transm sequence  [0..maxNumLine-1] */

  /* pointers to arrays for dequantiser return values */
  /* dequantised envelope parameters */
  double *quantEnvPara;		/* envelope parameters  [0..ENVPARANUM-1] */
  double *quantEnvParaEnha;	/* envelope parameters (incl. enha. bits) */
				/*   [0..ENVPARANUM-1] */

  /* dequantised line parameters */
  double *quantLineFreq;		/* line frequency  [0..maxNumLine-1] */
  double *quantLineAmpl;		/* line amplitude  [0..maxNumLine-1] */
  int *quantLineEnv;		/* line envelope flag  [0..maxNumLine-1] */
  int *quantLinePred;		/* line predecessor idx in prev frame */
				/*   [0..maxNumLine-1] */
  double *quantLineFreqEnha;	/* line frequency (incl. enhancement bits) */
				/*   [0..maxNumLine-1] */
  double *quantLineAmplEnha;	/* line amplitude (incl. enhancement bits) */
				/*   [0..maxNumLine-1] */
  double *quantLinePhase;	/* line phase (from enhancement bits) */
				/*   [0..maxNumLine-1] */

  /* variables for IndiLineEnvDequant() memory */
  /* required by IndiLineQuant() and IndiLineDequant() */
  double quantEnvAmplS;		/* env ampl start */
  double quantEnvAmplE;		/* env ampl end */

  /* variables and pointers to arrays for frame-to-frame memory */
  int prevNumLine;		/* num lines transmitted */
  int *prevLineSeq;		/* line transm sequence  [0..maxNumLine-1] */
  double *prevQuantLineFreq;	/* line frequency  [0..maxNumLine-1] */
  double *prevQuantLineAmplE;	/* line end amplitude  [0..maxNumLine-1] */
  int *prevFreqEnhaNumBit;	/* number of f enh. bits from prev. frame */

  int *prevLineParaFIndex;	/* prev. Frequency index	*/
  int *prevLineParaAIndex;	/* prev. Amplitude index	*/

  int *tmpPrevLineParaFIndex;	/* temporary prev. Frequency index	*/
  int *tmpPrevLineParaAIndex;	/* temporary prev. Amplitude index	*/

};

int freqEnhaThr[3][MAX_FBITSE] = {{ 588,  733,  878, 1023, 1023, 1023, 1023},
				  { 954, 1159, 1365, 1570, 1776, 1981, 2047},
				  {1177, 1467, 1757, 2047, 2047, 2047, 2047}};

int freqEnhaThrBsf4[2][MAX_FBITSE] = {{ 159,  269,  380,  491,  602,  713,  890},
				      {1243, 1511, 1779, 2047, 2047, 2047, 2047}};


/* ---------- functions ---------- */

/* IndiLineQuantInit() */
/* Init individual lines quantisation & dequantisation. */

ILQstatus *IndiLineQuantInit (
	int quantMode,			/* in: quant mode */
					/*     (-1, 0..5) */
	int enhaQuantMode,		/* in: enha quant mode */
					/*     (0..3) */
	double maxLineAmpl,		/* in: max line amplitude */
	int maxNumLine,			/* in: max num lines */
	int onlyDequant,		/* in: 0=init quant & dequant */
					/*     1=init dequant */
	int maxFIndex,			/* in: max. freq. index	*/
	int debugMode,			/* in: debug mode (0=off) */
					/* returns: ILQ status handle */
	int bsf)			/* bitstream format	*/
{
  ILQstatus *ILQ;
  int numBit;
/*   double maxFreqDev,relFreqDev,freqThr; */

  if ((ILQ = (ILQstatus *) malloc(sizeof(ILQstatus)))==NULL)
    IndiLineExit("IndiLineQuantInit: memory allocation error");

  ILQ->maxLineAmpl = maxLineAmpl;
  ILQ->quantMode = quantMode;
  ILQ->enhaQuantMode = enhaQuantMode;
  ILQ->maxNumLine = maxNumLine;
  ILQ->debugMode = debugMode;
  ILQ->maxFIndex = maxFIndex;
  ILQ->bsFormat = bsf;

  /* allocate memory for arrays */
  if (onlyDequant) {
    ILQ->lineParaPred = NULL;
    ILQ->lineParaIndex = NULL;
    ILQ->lineParaNumBit = NULL;
    ILQ->lineParaEnhaIndex = NULL;
    ILQ->lineParaEnhaNumBit = NULL;
    ILQ->lineSeq = NULL;
    ILQ->prevLineSeq = NULL;
    ILQ->lineParaFIndex = NULL;
    ILQ->lineParaAIndex = NULL;
  }
  else if
    (
     /* arrays for quantiser return values */
     (ILQ->lineParaPred =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->lineParaIndex =
      (unsigned long*)malloc(ILQ->maxNumLine*sizeof(unsigned long)))==NULL ||
     (ILQ->lineParaNumBit =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->lineParaEnhaIndex =
      (unsigned long*)malloc(ILQ->maxNumLine*sizeof(unsigned long)))==NULL ||
     (ILQ->lineParaEnhaNumBit =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->lineSeq =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     /* arrays for frame-to-frame memory */
     (ILQ->prevLineSeq =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->lineParaFIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->lineParaAIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL)

    IndiLineExit("IndiLineQuantInit: memory allocation error");
  if
    (
     /* pointers to variable content arrays */
     (ILQ->quantLineAmplE =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->freqEnhaNumBit =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     /* arrays for dequantiser return values */
     (ILQ->quantEnvPara =
      (double*)malloc(ENVPARANUM*sizeof(double)))==NULL ||
     (ILQ->quantEnvParaEnha =
      (double*)malloc(ENVPARANUM*sizeof(double)))==NULL ||
     (ILQ->quantLineFreq =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->quantLineAmpl =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->quantLineEnv =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->quantLinePred =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->quantLineFreqEnha =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->quantLineAmplEnha =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->quantLinePhase =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     /* arrays for frame-to-frame memory */
     (ILQ->prevQuantLineFreq =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->prevQuantLineAmplE =
      (double*)malloc(ILQ->maxNumLine*sizeof(double)))==NULL ||
     (ILQ->prevFreqEnhaNumBit =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->prevLineParaFIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->prevLineParaAIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->tmpPrevLineParaFIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL ||
     (ILQ->tmpPrevLineParaAIndex =
      (int*)malloc(ILQ->maxNumLine*sizeof(int)))==NULL)
    IndiLineExit("IndiLineQuantInit: memory allocation error");

  /* clear frame-to-frame memory */
  ILQ->prevNumLine = 0;

  /* select quantisers */
  ILQ->envTmNumBit = ILQ_TMBITS;
  ILQ->envAtkNumBit = ILQ_ATKBITS;
  ILQ->envDecNumBit = ILQ_DECBITS;
  ILQ->envAngle45Time = ILQ_ENV45;
  switch (quantMode) {
  case -1:
    ILQ->freqNumBit = ILQ_FBITS_16;
    ILQ->amplNumBit = ILQ_ABITS_16;
    ILQ->freqMin = ILQ_FMIN_16;
    ILQ->freqMax = ILQ_FMAX_16;
    break;
  case 0:
    ILQ->freqNumBit = 10;
    ILQ->amplNumBit = 4;
    ILQ->freqMin = 30.;
    ILQ->freqMax = 4000.;
    break;
  case 1:
    ILQ->freqNumBit = 10;
    ILQ->amplNumBit = 6;
    ILQ->freqMin = 30.;
    ILQ->freqMax = 4000.;
    break;
  case 2:
  case 4:
    ILQ->freqNumBit = 11;
    ILQ->amplNumBit = 5;
    ILQ->freqMin = 20.;
    ILQ->freqMax = 20000.;
    break;
  case 3:
  case 5:
    ILQ->freqNumBit = 11;
    ILQ->amplNumBit = 6;
    ILQ->freqMin = 20.;
    ILQ->freqMax = 20000.;
    break;
  default:
    IndiLineExit("IndiLineQuantInit: wrong quant mode %d",quantMode);
  }
  ILQ->amplMin = ILQ_AMIN;
  ILQ->amplMax = ILQ_AMAX;
  ILQ->deltaFreqNumBit = ILQ_DFBITS;
  ILQ->deltaAmplNumBit = ILQ_DABITS;
  ILQ->deltaFreqMax = ILQ_DFMAX;
  ILQ->deltaAmplMax = ILQ_DAMAX;
  ILQ->envTmNumBitE = ILQ_TMBITSE;
  ILQ->envAtkNumBitE = ILQ_ATKBITSE;
  ILQ->envDecNumBitE = ILQ_DECBITSE;
  ILQ->phaseNumBitE = ILQ_PBITSE;

  /* init quantisers */
  ILQ->freqNumStep = 1<<ILQ->freqNumBit;
  ILQ->freqRange = log(ILQ->freqMax/ILQ->freqMin);
  ILQ->amplNumStep = 1<<ILQ->amplNumBit;
  ILQ->amplRange = log(ILQ->amplMax/ILQ->amplMin);

  ILQ->deltaFreqNumStep = 1<<ILQ->deltaFreqNumBit;
  ILQ->deltaFreqRange = 2*ILQ->deltaFreqMax;
  ILQ->deltaAmplNumStep = 1<<ILQ->deltaAmplNumBit;
  ILQ->deltaAmplRange = log(ILQ->deltaAmplMax*ILQ->deltaAmplMax);

  ILQ->phaseNumStepE = 1<<ILQ->phaseNumBitE;
  ILQ->phaseRangeE = 2*M_PI;

  ILQ->freqRelStep = exp(1./ILQ->freqNumStep*ILQ->freqRange);

  /*
  maxFreqDev = ILQ_MAXPHASEDEV*(double)(fSampleQuant)/frameLenQuant;
  relFreqDev = (ILQ->freqRelStep-1)*.5;
  numBit = 0;
  do {
    if (numBit>=MAX_FBITSE)
      IndiLineExit("IndiLineQuantInit: internal error");
    ILQ->freqNumStepE[numBit] = 1<<numBit;
    freqThr = maxFreqDev/relFreqDev*ILQ->freqNumStepE[numBit];
    ILQ->freqThrE[numBit] = (int)(log(freqThr/ILQ->freqMin)/ILQ->freqRange*
				  ILQ->freqNumStep);
    if (ILQ->debugMode)
      printf("fNumBitE=%d fthr=%9.3f fthrE=%d\n",
	     numBit,freqThr,ILQ->freqThrE[numBit]);
    numBit++;
  } while (freqThr <= ILQ->freqMax);
  */

  if ( bsf<4 )
  {
    for (numBit=0; numBit<MAX_FBITSE; numBit++) {
      ILQ->freqThrE[numBit] = freqEnhaThr[(ILQ->quantMode<=1)?0:1]
	[min(MAX_FBITSE-1,numBit+3-ILQ->enhaQuantMode)];
      ILQ->freqNumStepE[numBit] = 1<<numBit;
      if (ILQ->debugMode)
	printf("fNumBitE=%d fthrE=%d fthr=%9.3f\n",
	       numBit,ILQ->freqThrE[numBit],
	       exp((ILQ->freqThrE[numBit]+.5)/ILQ->freqNumStep*ILQ->freqRange)*
	       ILQ->freqMin);
    }
  }
  else
  {
    for (numBit=0; numBit<MAX_FBITSE; numBit++) {
      ILQ->freqThrE[numBit] = freqEnhaThrBsf4[0][min(MAX_FBITSE-1,numBit+3-ILQ->enhaQuantMode)];
      ILQ->freqNumStepE[numBit] = 1<<numBit;
      if (ILQ->debugMode)
	printf("fNumBitE=%d fthrE=%d fthr=%9.3f\n",
	       numBit,ILQ->freqThrE[numBit],
	       exp((ILQ->freqThrE[numBit]+.5)/ILQ->freqNumStep*ILQ->freqRange)*
	       ILQ->freqMin);
    }
  }


  ILQ->envTmNumStep = 1<<ILQ->envTmNumBit;
  ILQ->envAtkNumStep = 1<<ILQ->envAtkNumBit;
  ILQ->envDecNumStep = 1<<ILQ->envDecNumBit;

  ILQ->envTmNumStepE = 1<<ILQ->envTmNumBitE;
  ILQ->envAtkNumStepE = 1<<ILQ->envAtkNumBitE;
  ILQ->envDecNumStepE = 1<<ILQ->envDecNumBitE;


  if (ILQ->debugMode)
    printf("quant step size: f=%9.6f a=%9.6f df=%9.6f da=%9.6f\n",
	   exp(1./ILQ->freqNumStep*ILQ->freqRange),
	   exp(1./ILQ->amplNumStep*ILQ->amplRange),
	   ILQ->deltaFreqRange/ILQ->deltaFreqNumStep,
	   exp(1./ILQ->deltaAmplNumStep*ILQ->deltaAmplRange));

  if (ILQ->debugMode)
    printf("maxa=%9.3f maxnumlin=%d quantMode=%d enhaQuantMode=%d \n",
	   ILQ->maxLineAmpl,ILQ->maxNumLine,
	   ILQ->quantMode,ILQ->enhaQuantMode);

  return ILQ;
}


/* IndiLineNumBit */
/* Number of bits in basic bitstream for quantised individual */
/* line parameters and envelope parameters. */
/* If ILQ=NULL: minimum number of bits. */

void IndiLineNumBit (
	ILQstatus *ILQ,			/* in: ILQ status handle */
					/*     or NULL */
					/* BASIC BITSTREAM: */
	int *envNumBit,			/* out: num bits envelope */
	int *newLineNumBit,		/* out: num bits new line */
	int *contLineNumBit)		/* out: num bits continued line */
{
  if (ILQ==NULL) {
    *envNumBit = ILQ_TMBITS+ILQ_ATKBITS+ILQ_DECBITS;
    *newLineNumBit = ILQ_ABITS_8+ILQ_FBITS_8;
    *contLineNumBit = ILQ_DABITS+ILQ_DFBITS;
  }
  else {
    *envNumBit = ILQ->envTmNumBit+ILQ->envAtkNumBit+ILQ->envDecNumBit;
    *newLineNumBit = ILQ->amplNumBit+ILQ->freqNumBit;
    *contLineNumBit = ILQ->deltaAmplNumBit+ILQ->deltaFreqNumBit;
  }
}


static int GetEnhNumBit(int *freqThrE,int fint)
{
	register int	fNumBitE;

	fNumBitE = 0;
	while (fNumBitE<MAX_FBITSE && fint>freqThrE[fNumBitE])
	    fNumBitE++;
	if (fNumBitE==MAX_FBITSE)
	  IndiLineExit("GetEnhNumBit: fnumbite error");

	return fNumBitE;
}


/* IndiLineEnhaNumBit */
/* Number of bits in enhancement bitstream for quantised */
/* envelope parameters and individual line parameters. */

void IndiLineEnhaNumBit (
	ILQstatus *ILQ,			/* ILQ status handle */
					/* BASIC BITSTREAM: */
	int numLine,			/* in: num lines */
	int *lineSeq,			/* in: line transm sequence */
					/*     [0..numLine-1] */
					/*     or NULL if seq OK */
	int *lineParaPred,		/* in: line predecessor idx */
					/*     in prev frame */
					/*     [0..numLine-1] */
	unsigned long *lineParaIndex,	/* in: line para bits */
					/*     [0..numLine-1] */
	int *lineParaNumBit,		/* in: num line para bits */
					/*     [0..numLine-1] */
					/* ENHANCEMENT BITSTREAM: */
	int *envEnhaNumBit,		/* out: num bits envelope */
	int **lineEnhaNumBit)		/* out: num bits line */
					/*      [0..numLine-1] */
{
  int fint;
  int fNumBitE;
  int i,iTrans;

  if (lineEnhaNumBit==NULL)
    IndiLineExit("IndiLineEnhaNumBit: quant not init'ed");

  *envEnhaNumBit = ILQ->envTmNumBitE+ILQ->envAtkNumBitE+ILQ->envDecNumBitE;

  for (iTrans=0;iTrans<numLine;iTrans++) {
    if (lineSeq==NULL)
      i = iTrans;
    else
      i = lineSeq[iTrans];
    if (!lineParaPred[i]) {
      /* new line */
      /* HP20001210 wg. bsf6
      if (lineParaNumBit[i] != ILQ->amplNumBit+ILQ->freqNumBit)
	IndiLineExit("IndiLineEnhaNumBit: bitnum error (new)");
      */

      if ( ((long) lineParaIndex[i])<0 )
	fint = -1024-((long) lineParaIndex[i]);
      else
	fint = lineParaIndex[i] & ((1<<ILQ->freqNumBit)-1);

      fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);

      ILQ->lineParaEnhaNumBit[i] = ILQ->phaseNumBitE;
      if (fNumBitE > 0)
	ILQ->lineParaEnhaNumBit[i] += fNumBitE;
    }
    else {
      /* continued line */

      if ( ((long) lineParaIndex[i])<0 )
      {
	fint = -1024-((long) lineParaIndex[i]);
	fint+= ILQ->prevLineParaFIndex[lineParaPred[i]-1];
	fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);
      }
      else
	fNumBitE = ILQ->prevFreqEnhaNumBit[lineParaPred[i]-1];

      /* HP20001210 wg. bsf6
      if (lineParaNumBit[i] != ILQ->deltaAmplNumBit+ILQ->deltaFreqNumBit)
	IndiLineExit("IndiLineEnhaNumBit: bitnum error (cont)");
      */
      ILQ->lineParaEnhaNumBit[i] = ILQ->phaseNumBitE + fNumBitE;
    }
  }

  /* set pointers to arrays with return values */
  *lineEnhaNumBit = ILQ->lineParaEnhaNumBit;
}


/* IndiLineEnvQuant */
/* Quantisation of individual envelope parameters. */

void IndiLineEnvQuant (
	ILQstatus *ILQ,			/* ILQ status handle */
	double *envPara,			/* in: envelope parameters */
					/*     [0..ENVPARANUM-1] */
					/* BASIC BITSTREAM: */
	int *envParaFlag,		/* out: env flag (1 bit) */
	unsigned int *envParaIndex,	/* out: envelope bits */
	int *envParaNumBit,		/* out: num envelope bits */
					/* ENHANCEMENT BITSTREAM: */
	unsigned int *envParaEnhaIndex,	/* out: envelope bits */
	int *envParaEnhaNumBit)		/* out: num envelope bits */
{
  double tmq   = 0;
  double atkq  = 0;
  double decq  = 0;
  int tmint   = 0;
  int atkint  = 0;
  int decint  = 0;
  int tmintE  = 0;
  int atkintE = 0;
  int decintE = 0;
  double envAngle1 = 0;
  double envAngle2 = 0;

  /* decide whether to use envelope */
  *envParaFlag = 0;
  if (envPara[1]!=0 || envPara[2]!=0)
    *envParaFlag = 1;
  
  if (*envParaFlag) {
    /* quantise envelope parameters */
    envAngle1 = atan(envPara[1]*ILQ->envAngle45Time)/(M_PI/2);
    envAngle2 = atan(envPara[2]*ILQ->envAngle45Time)/(M_PI/2);

    tmint = (int)(envPara[0]*ILQ->envTmNumStep);
    tmint = max(min(tmint,ILQ->envTmNumStep-1),0);
    tmq = (tmint+.5)/ILQ->envTmNumStep;
    tmintE = (int)(((envPara[0]-tmq)*ILQ->envTmNumStep+.5)*ILQ->envTmNumStepE);
    tmintE = max(min(tmintE,ILQ->envTmNumStepE-1),0);
    if (envPara[1] == 0) {
      atkint = atkintE = 0;
      atkq = 0;
    }
    else {
      atkint = (int)(envAngle1*(ILQ->envAtkNumStep-1)+1); 
      atkint = max(min(atkint,ILQ->envAtkNumStep-1),0);
      atkq = (atkint-.5)/(ILQ->envAtkNumStep-1);
      atkintE = (int)(((envAngle1-atkq)*ILQ->envAtkNumStep+.5)*
		      ILQ->envAtkNumStepE);
      atkintE = max(min(atkintE,ILQ->envAtkNumStepE-1),0);
    }
    if (envPara[2] == 0) {
      decint = decintE = 0;
      decq = 0;
    }
    else {
      decint = (int)(envAngle2*(ILQ->envDecNumStep-1)+1); 
      decint = max(min(decint,ILQ->envDecNumStep-1),0);
      decq = (decint-.5)/(ILQ->envDecNumStep-1);
      decintE = (int)(((envAngle2-decq)*ILQ->envDecNumStep+.5)*
		      ILQ->envDecNumStepE);
      decintE = max(min(decintE,ILQ->envDecNumStepE-1),0);
    }
    
    if(atkint==0 && decint==0)
      *envParaFlag = 0;
  }
  
  if (*envParaFlag) {
    /* encode envelope parameters */
    *envParaIndex = 0;
    *envParaNumBit = 0;
    *envParaIndex = *envParaIndex<<ILQ->envTmNumBit | tmint;
    *envParaNumBit += ILQ->envTmNumBit;
    *envParaIndex = *envParaIndex<<ILQ->envAtkNumBit | atkint;
    *envParaNumBit += ILQ->envAtkNumBit;
    *envParaIndex = *envParaIndex<<ILQ->envDecNumBit | decint;
    *envParaNumBit += ILQ->envDecNumBit;
    *envParaEnhaIndex = 0;
    *envParaEnhaNumBit = 0;
    *envParaEnhaIndex = *envParaEnhaIndex<<ILQ->envTmNumBitE | tmintE;
    *envParaEnhaNumBit += ILQ->envTmNumBitE;
    *envParaEnhaIndex = *envParaEnhaIndex<<ILQ->envAtkNumBitE | atkintE;
    *envParaEnhaNumBit += ILQ->envAtkNumBitE;
    *envParaEnhaIndex = *envParaEnhaIndex<<ILQ->envDecNumBitE | decintE;
    *envParaEnhaNumBit += ILQ->envDecNumBitE;
    
    if (ILQ->debugMode) {
      printf("tm=%f atk=%f dec=%f\n",
	     envPara[0],envPara[1],envPara[2]);
      printf("tm=%f i=%d i2=%d q=%f\n",
	     envPara[0],tmint,tmintE,tmq);
      printf("atkang=%f i=%d i2=%d q=%f\n",
	     envAngle1,atkint,atkintE,atkq);
      printf("decang=%f i=%d i2=%d q=%f\n",
	     envAngle2,decint,decintE,decq);
    }
  }
  else {
    *envParaIndex=0;
    *envParaNumBit=0;
    *envParaEnhaIndex=0;
    *envParaEnhaNumBit=0;

    if (ILQ->debugMode)
      printf("envParaFlag==0\n");
  }
}


/* IndiLineEnvDequant */
/* Dequantisation of individual envelope parameters from basic bitstream. */
/* Enhancement bitstream utilised if envParaEnhaNumBit != 0. */

void IndiLineEnvDequant (
	ILQstatus *ILQ,			/* ILQ status handle */
					/* BASIC BITSTREAM: */
	int envParaFlag,		/* in: envelope flag (1 bit) */
	unsigned int envParaIndex,	/* in: envelope bits */
	int envParaNumBit,		/* in: num envelope bits */
					/* ENHANCEMENT BITSTREAM: */
	unsigned int envParaEnhaIndex,	/* in: envelope bits */
	int envParaEnhaNumBit,		/* in: num envelope bits */
					/*     or 0 */
	double **quantEnvPara,		/* out: envelope parameters */
					/*      [0..ENVPARANUM-1] */
	double **quantEnvParaEnha)	/* out: envelope parameters */
					/*      (incl. enhancement bits) */
					/*      [0..ENVPARANUM-1] */
					/*      or NULL */
{
  int tmint,atkint,decint;
  int tmintE,atkintE,decintE;
  double envAngle1 = 0;
  double envAngle2 = 0;

  if (envParaFlag) {
    /* decode envelope parameters */
    if (envParaNumBit != ILQ->envTmNumBit+ILQ->envAtkNumBit+ILQ->envDecNumBit)
      IndiLineExit("IndiLineEnvDequant: bitnum error");
    tmint = (envParaIndex>>(ILQ->envAtkNumBit+ILQ->envDecNumBit)) &
      ((1<<ILQ->envTmNumBit)-1);
    atkint = (envParaIndex>>ILQ->envDecNumBit) & ((1<<ILQ->envAtkNumBit)-1);
    decint = envParaIndex & ((1<<ILQ->envDecNumBit)-1);

    /* dequantise envelope parameters */
    ILQ->quantEnvPara[0] = (tmint+.5)/ILQ->envTmNumStep;
    if (atkint==0)
      ILQ->quantEnvPara[1] = 0;
    else {
      envAngle1 = (atkint-.5)/(ILQ->envAtkNumStep-1);
      ILQ->quantEnvPara[1] = tan(envAngle1*M_PI/2)/ILQ->envAngle45Time;
    }
    if (decint==0)
      ILQ->quantEnvPara[2] = 0;
    else {
      envAngle2 = (decint-.5)/(ILQ->envDecNumStep-1);
      ILQ->quantEnvPara[2] = tan(envAngle2*M_PI/2)/ILQ->envAngle45Time;
    }

    if (envParaEnhaNumBit != 0) {
      /* decode envelope enhancement parameters */
      if (envParaEnhaNumBit != ILQ->envTmNumBitE+ILQ->envAtkNumBitE+
	  ILQ->envDecNumBitE)
	IndiLineExit("IndiLineEnvDequant: bitnum error (enha)");
      tmintE = (envParaEnhaIndex>>(ILQ->envAtkNumBitE+ILQ->envDecNumBitE)) &
	((1<<ILQ->envTmNumBitE)-1);
      atkintE = (envParaEnhaIndex>>ILQ->envDecNumBitE) &
	((1<<ILQ->envAtkNumBitE)-1);
      decintE = envParaEnhaIndex & ((1<<ILQ->envDecNumBitE)-1);

      /* dequantise envelope enhancement parameters */
      ILQ->quantEnvParaEnha[0] = ILQ->quantEnvPara[0]+
	((tmintE+.5)/ILQ->envTmNumStepE-.5)/ILQ->envTmNumStep;
      if (atkint==0)
	ILQ->quantEnvParaEnha[1] = 0;
      else {
	envAngle1 += ((atkintE+.5)/ILQ->envAtkNumStepE-.5)/
	  (ILQ->envAtkNumStep-1);
	ILQ->quantEnvParaEnha[1] = tan(envAngle1*M_PI/2)/ILQ->envAngle45Time;
      }
      if (decint==0)
	ILQ->quantEnvParaEnha[2] = 0;
      else {
	envAngle2 += ((decintE+.5)/ILQ->envDecNumStepE-.5)/
	  (ILQ->envDecNumStep-1);
	ILQ->quantEnvParaEnha[2] = tan(envAngle2*M_PI/2)/ILQ->envAngle45Time;
      }
    }
  }
  else {	/* if (envParaFlag) */
    ILQ->quantEnvPara[0] = 0;
    ILQ->quantEnvPara[1] = 0;
    ILQ->quantEnvPara[2] = 0;
    if (envParaEnhaNumBit != 0) {
      ILQ->quantEnvParaEnha[0] = 0;
      ILQ->quantEnvParaEnha[1] = 0;
      ILQ->quantEnvParaEnha[2] = 0;
    }
  }

  /* set pointers to arrays with return values */
  *quantEnvPara = ILQ->quantEnvPara;
  if (quantEnvParaEnha != NULL)
    *quantEnvParaEnha = ILQ->quantEnvParaEnha;

  /* copy parameters into IndiLineEnvDequant() memory */
  ILQ->quantEnvAmplS = max(MINENVAMPL,
			   1-(ILQ->quantEnvPara[0]*ILQ->quantEnvPara[1]));
  ILQ->quantEnvAmplE = max(MINENVAMPL,
			   1-((1-ILQ->quantEnvPara[0])*ILQ->quantEnvPara[2]));

  if (ILQ->debugMode) {
    printf("tm=%f atk=%f dec=%f as=%f ae=%f\n",
	   ILQ->quantEnvPara[0],ILQ->quantEnvPara[1],ILQ->quantEnvPara[2],
	   ILQ->quantEnvAmplS,ILQ->quantEnvAmplE);
    if (envParaEnhaNumBit != 0)
    printf("enha: tm=%f atk=%f dec=%f\n",
	   ILQ->quantEnvParaEnha[0],ILQ->quantEnvParaEnha[1],
	   ILQ->quantEnvParaEnha[2]);
  }
}


static int FQuantOld(ILQstatus *ILQ,double f)
{
	register int	fint;

	if ( f < ILQ->freqMin ) return 0;

	fint = (int) (log(f/ILQ->freqMin)/ILQ->freqRange*
			ILQ->freqNumStep);
	fint = max(min(fint,ILQ->freqNumStep-1),0);

	return fint;
}

static int AQuantOld(ILQstatus *ILQ,double a)
{
	register int	aint;

	if ( a < ILQ->maxLineAmpl*ILQ->amplMin ) return 0;

	aint = (int)(log(a/ILQ->maxLineAmpl/ILQ->amplMin)/
		     ILQ->amplRange*ILQ->amplNumStep);
	aint = max(min(aint,ILQ->amplNumStep-1),0);

	return aint;
}

static double ScaleFToBark(double f)
{
	if ( f<500.0 ) return f*(STEPSPERBARK/100.0);
	
	return (double) (5.0*STEPSPERBARK)+(log(f*(1.0/500.0))*(STEPSPERBARK/0.2));
}

static int FQuant(ILQstatus *ILQ,double f)
{
	register int	fint;

	fint = (int) ScaleFToBark(f);

	if ( fint<0 ) fint=0;
	if ( fint>=ILQ->maxFIndex ) fint=ILQ->maxFIndex-1;

	return fint;
}

static int FQuantEnh(double f,int fint,int fNumBitE)
{
	register int	h = 1<<fNumBitE;
	register int	fintE;
	register double	fq;

	fq = ScaleFToBark(f)-((double) fint);	/* 0..1	*/

	fintE = (int) (fq*((double) h));
	if ( fintE< 0 ) fintE=0;
	if ( fintE>=h ) fintE=h-1;

	return fintE;
}

int AQuant(ILQstatus *ILQ,double a)
{
	register int	aint;

	if ( ILQ->bsFormat<5 )
	{
		if ( a<=0.0 ) return 0;
		aint = (int)(log(a*(1.0/ILQ->maxLineAmpl))*(20.0/LOG10*STEPSPERDB)+64.0);
	}
	else
	{
		if ( a<=0.0 ) return 63;
		aint = (int)(log(ILQ->maxLineAmpl/a) * (20.0/LOG10*STEPSPERDB));
	}

	aint = max(min(aint,63),0);

	return aint;
}

/* IndiLineQuant */
/* Quantisation of individual line parameters. */

void IndiLineQuant (
	ILQstatus *ILQ,			/* ILQ status handle */
					/* Requires: */
					/*   PREVIOUS FRAME: */
					/*   dequantised envelope and lines */
					/*   CURRENT FRAME: */
					/*   dequantised envelope */
	int numLine,			/* in: num lines */
	double *lineFreq,		/* in: line frequency */
					/*     [0..numLine-1] */
	double *lineAmpl,		/* in: line amplitude */
					/*     [0..numLine-1] */
	double *linePhase,		/* in: line phase */
					/*     [0..NumLine-1] */
	int *lineEnv,			/* in: line envelope flag */
					/*     [0..numLine-1] */
	int *linePred,			/* in: line predecessor idx */
					/*     [0..numLine-1] */
					/* BASIC BITSTREAM: */
	int **lineParaPred,		/* out: line predecessor idx */
					/*      in prev transm frame */
					/*      [0..numLine-1] */
	unsigned long **lineParaIndex,	/* out: line para bits */
					/*      [0..numLine-1] */
	int **lineParaNumBit,		/* out: num line para bits */
					/*      [0..numLine-1] */
					/* ENHANCEMENT BITSTREAM: */
	unsigned long **lineParaEnhaIndex,
					/* out: line para bits */
					/*      [0..numLine-1] */
	int **lineParaEnhaNumBit,	/* out: num line para bits */
					/*      [0..numLine-1] */
	int **lineParaFIndex,		/* out: line para freq. index if != NULL */
					/*      [0..numLine-1] */
	int **lineParaAIndex,		/* out: line para amp. index if ...FIndex!=NULL) */
					/*      [0..numLine-1] */
	int **prevLineParaFIndex,	/* out: prev. line para freq. index if ...FIndex!=NULL */
					/*      [0..numLine-1] */
	int **prevLineParaAIndex)	/* out: prev. line para amp. index if ...FIndex!=NULL) */
					/*      [0..numLine-1] */

{
  int fint = 0;
  int aint,dfint,daint;
  int fintE = 0;
  int pintE;
  int fNumBitE;
  double fq,freqRelStep;
  int i,iPred;

  if (lineParaPred==NULL)
    IndiLineExit("IndiLineQuant: quant not init'ed");

  /* quantise lines */
  for (i=0;i<numLine;i++) {
    if (!(linePred[i] && linePred[i]-1<ILQ->prevNumLine)) {

      /* quantise new line */

      if ( lineParaFIndex )
      {
	aint=AQuant(ILQ,lineAmpl[i]);
	fint=FQuant(ILQ,lineFreq[i]);
      }
      else
      {
	aint=AQuantOld(ILQ,lineAmpl[i]);
	fint=FQuantOld(ILQ,lineFreq[i]);
      }

      fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);

      /*>>>>>>>>>>*/
      if (ILQ->debugMode>=4) {
	printf("ENHA %2d: n, fNumBitE=%d",i,fNumBitE);
      }
      /*<<<<<<<<<<*/

      if (fNumBitE > 0) {
	if ( lineParaFIndex )
	{
	  fintE=FQuantEnh(lineFreq[i],fint,fNumBitE);
	}
	else
	{
	  fq = exp((fint+.5)/ILQ->freqNumStep*ILQ->freqRange)*ILQ->freqMin;
	  fintE = (int)(((lineFreq[i]/fq-1)/(ILQ->freqRelStep-1)+.5)*
		        ILQ->freqNumStepE[fNumBitE]);
	  fintE = max(min(fintE,ILQ->freqNumStepE[fNumBitE]-1),0);
	}
      }

      /*>>>>>>>>>>*/
      if (ILQ->debugMode>=4) {
	if (fNumBitE > 0)
	  printf(", fintE=%d\n",fintE);
	else
	  printf("\n");
      }
      /*<<<<<<<<<<*/

      pintE = (int)((linePhase[i]/ILQ->phaseRangeE+.5)*
		    ILQ->phaseNumStepE);
      pintE = max(min(pintE,ILQ->phaseNumStepE-1),0);
      
      /* encode new line */
      ILQ->lineParaPred[i] = 0;
      ILQ->lineParaIndex[i] = 0;
      ILQ->lineParaNumBit[i] = 0;

      if ( lineParaFIndex )
      {
	ILQ->lineParaFIndex[i]=fint;
	ILQ->lineParaAIndex[i]=aint;
	ILQ->lineParaIndex[i] = (long) (-1024-fint);
      }
      else
      {
	ILQ->lineParaIndex[i] = ILQ->lineParaIndex[i]<<ILQ->amplNumBit | aint;
	ILQ->lineParaNumBit[i] += ILQ->amplNumBit;
	ILQ->lineParaIndex[i] = ILQ->lineParaIndex[i]<<ILQ->freqNumBit | fint;
	ILQ->lineParaNumBit[i] += ILQ->freqNumBit;
      }
      ILQ->lineParaEnhaIndex[i] = 0;
      ILQ->lineParaEnhaNumBit[i] = 0;
      if (fNumBitE > 0) {
	ILQ->lineParaEnhaIndex[i] =
	  ILQ->lineParaEnhaIndex[i]<<fNumBitE | fintE;
	ILQ->lineParaEnhaNumBit[i] += fNumBitE;
      }
      ILQ->lineParaEnhaIndex[i] =
	ILQ->lineParaEnhaIndex[i]<<ILQ->phaseNumBitE | pintE;
      ILQ->lineParaEnhaNumBit[i] += ILQ->phaseNumBitE;
    }
    else {
      /* quantise continued line */
      
      iPred=0;
      while (ILQ->prevLineSeq[iPred]!=linePred[i]-1 && iPred<ILQ->prevNumLine)
	iPred++;
      if (iPred>=ILQ->prevNumLine) 
	IndiLineExit("IndiLineQuant: pred error");

      ILQ->lineParaPred[i] = iPred+1;
      daint = (int)((log(lineAmpl[i]*((lineEnv[i])?ILQ->quantEnvAmplS:1)/
			 ILQ->prevQuantLineAmplE[iPred])/
		     ILQ->deltaAmplRange+.5)*ILQ->deltaAmplNumStep-.5);
      daint = max(min(daint,ILQ->deltaAmplNumStep-1),0);
      dfint = (int)(((lineFreq[i]/ILQ->prevQuantLineFreq[iPred]-1)/
		     ILQ->deltaFreqRange+.5)*ILQ->deltaFreqNumStep-.5);
      dfint = max(min(dfint,ILQ->deltaFreqNumStep-1),0);


      if ( lineParaFIndex )
      {
        fint = FQuant(ILQ,lineFreq[i]);
        aint = AQuant(ILQ,lineAmpl[i]);

	ILQ->lineParaFIndex[i] = fint-ILQ->prevLineParaFIndex[iPred];
	ILQ->lineParaAIndex[i] = aint-ILQ->prevLineParaAIndex[iPred];
	ILQ->lineParaIndex[i] = (long) (-1024-ILQ->lineParaFIndex[i]);

	fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);	  
      }
      else
      {
	fNumBitE = ILQ->prevFreqEnhaNumBit[iPred];
      }

      /*>>>>>>>>>>*/
      if (ILQ->debugMode>=4) {
	printf("ENHA %2d: c, fNumBitE=%d",i,fNumBitE);
      }
      /*<<<<<<<<<<*/

      if (fNumBitE > 0) {

	if ( lineParaFIndex )
	{
	  fintE=FQuantEnh(lineFreq[i],fint,fNumBitE);
	}
	else
	{
	  fq = ILQ->prevQuantLineFreq[iPred]*
	    (1+((((dfint+1.0)/ILQ->deltaFreqNumStep)-.5)*ILQ->deltaFreqRange));
	  freqRelStep = (ILQ->deltaFreqRange/ILQ->deltaFreqNumStep
		         *ILQ->prevQuantLineFreq[iPred]/fq)+1;
	  fintE = (int)(((lineFreq[i]/fq-1)/(freqRelStep-1)+.5)*
		        ILQ->freqNumStepE[fNumBitE]);
	  fintE = max(min(fintE,ILQ->freqNumStepE[fNumBitE]-1),0);
	}
      }

      /*>>>>>>>>>>*/
      if (ILQ->debugMode>=4) {
	if (fNumBitE > 0)
	  printf(", fintE=%d\n",fintE);
	else
	  printf("\n");
      }
      /*<<<<<<<<<<*/

      pintE = (int)((linePhase[i]/ILQ->phaseRangeE+.5)*
		    ILQ->phaseNumStepE);
      pintE = max(min(pintE,ILQ->phaseNumStepE-1),0);
      
      /* encode continued line */

      ILQ->lineParaNumBit[i] = 0;

      if ( lineParaFIndex==NULL )
      {
	ILQ->lineParaIndex[i] = 0;
	ILQ->lineParaIndex[i] =
	ILQ->lineParaIndex[i]<<ILQ->deltaAmplNumBit | daint;
	ILQ->lineParaNumBit[i] += ILQ->deltaAmplNumBit;
	ILQ->lineParaIndex[i] =
	  ILQ->lineParaIndex[i]<<ILQ->deltaFreqNumBit | dfint;
	ILQ->lineParaNumBit[i] += ILQ->deltaFreqNumBit;
      }

      ILQ->lineParaEnhaIndex[i] = 0;
      ILQ->lineParaEnhaNumBit[i] = 0;
      if (fNumBitE > 0) {
	ILQ->lineParaEnhaIndex[i] =
	  ILQ->lineParaEnhaIndex[i]<<fNumBitE | fintE;
	ILQ->lineParaEnhaNumBit[i] += fNumBitE;
      }
      ILQ->lineParaEnhaIndex[i] =
	ILQ->lineParaEnhaIndex[i]<<ILQ->phaseNumBitE | pintE;
      ILQ->lineParaEnhaNumBit[i] += ILQ->phaseNumBitE;
    }
  }

  /* set pointers to arrays with return values */
  *lineParaPred = ILQ->lineParaPred;
  *lineParaIndex = ILQ->lineParaIndex;
  *lineParaNumBit = ILQ->lineParaNumBit;
  *lineParaEnhaIndex = ILQ->lineParaEnhaIndex;
  *lineParaEnhaNumBit = ILQ->lineParaEnhaNumBit;

  if ( lineParaFIndex )	{
    *lineParaFIndex=ILQ->lineParaFIndex;
    *lineParaAIndex=ILQ->lineParaAIndex;
    *prevLineParaFIndex=ILQ->prevLineParaFIndex;
    *prevLineParaAIndex=ILQ->prevLineParaAIndex;
  }
}


/* IndiLineSequence */
/* Determine transmission sequence for numLine lines. */

void IndiLineSequence (
	ILQstatus *ILQ,			/* ILQ status handle */
	int numLine,			/* in: num lines to transmit */
	int *lineParaPred,		/* in: line predecessor idx */
					/*     in prev transm frame */
					/*     [0..numLine-1] */
	int *lineParaFIndex,		/* in: line freq. index, used to */
					/*     sort not continued lines  */
					/*     [0..numLine-1]		 */
	int **lineSeq,			/* out: line transm sequence */
					/*      [0..numLine-1] */
	int *layNumLine,
	int numLayer)
{
  int il0,il1,il,ilPrev,ilMin,ilMax,ilTrans,numCont,layer;
  int ilCont = 0;
  il1=0;
  for (layer=0; layer<numLayer; layer++)
  {
    il0=il1;
    il1=layNumLine[layer];

    numCont = 0;
    ilMax = 0;
    for (il=il0;il<il1;il++)
      if (lineParaPred[il]) {
	numCont++;
	if (lineParaPred[il]>ilMax)
	  ilMax = lineParaPred[il];
      }

    ilPrev=0;
    for (ilTrans=il0; ilTrans<il0+numCont; ilTrans++) {
      ilMin = ilMax;
      for (il=il0;il<il1;il++)
	if (lineParaPred[il]>ilPrev && lineParaPred[il]<=ilMin) {
	  ilMin = lineParaPred[il];
	  ilCont = il;
	}
      ilPrev = ilMin;
      ILQ->lineSeq[ilTrans] = ilCont;
    }

    for (il=il0;il<il1;il++)
      if (!lineParaPred[il])
	ILQ->lineSeq[ilTrans++] = il;

    if ( lineParaFIndex )		/* sort by frequency	*/
    {
  	for ( il=il0+numCont; il<il1; il++)
  	{
  		int i,m = il;

  		for (i=il+1; i<il1; i++)
  		{
  			if ( lineParaFIndex[ILQ->lineSeq[i]] < lineParaFIndex[ILQ->lineSeq[m]] )
  				m=i;
  		}
  		i=ILQ->lineSeq[m];
  		ILQ->lineSeq[m]=ILQ->lineSeq[il];
  		ILQ->lineSeq[il]=i;
  	}
    }
  }


  if (ILQ->debugMode) {
    printf("idx:");
    for (il=0;il<numLine;il++)
      printf(" %2d",il);
    printf("\n");
    printf("seq:");
    for (il=0;il<numLine;il++)
      printf(" %2d",ILQ->lineSeq[il]);
    printf("\n");
    printf("pre:");
    for (il=0;il<numLine;il++)
      printf(" %2d",lineParaPred[ILQ->lineSeq[il]]);
    printf("\n");
  }

  /* set pointers to arrays with return values */
  *lineSeq = ILQ->lineSeq;
}


static double ScaleBarkToF(double x)
{
	if ( x<(5.0*STEPSPERBARK) ) return x*(100.0/STEPSPERBARK);

	return 500.0*exp((0.2/STEPSPERBARK)*(x-((double) (5*STEPSPERBARK))));
}

static double FDeQuant(int fi)
{
	register double	h = ((double) fi)+0.5;

	return ScaleBarkToF(h);
}

static double FDeQuantEnh(int fi,int efi,int ebits)
{
	register double	h = ((double) fi);
	register double	e = ((double) efi)+0.5;

	e/=(double) (1<<ebits);
	h+=e;

	return ScaleBarkToF(h);
}

double ADeQuant(ILQstatus *ILQ,int ai)
{
	register double	h = (double) ai;

	if ( ILQ->bsFormat<5 )
	{
		h = h+(0.5-64.0);
	}
	else
	{
		h = -0.5-h;
	}

	return ILQ->maxLineAmpl*exp((LOG10/(STEPSPERDB*20.0))*h);
}


/* IndiLineDequant */
/* Dequantisation of individual line parameters from basic bitstream. */
/* Enhancement bitstream utilised if lineParaEnhaNumBit != NULL. */

void IndiLineDequant (
	ILQstatus *ILQ,			/* ILQ status handle */
					/* Requires: */
					/*   PREVIOUS FRAME: */
					/*   dequantised envelope & lines */
					/*   CURRENT FRAME: */
					/*   dequantised envelope */
	int numLine,			/* in: num lines */
	int *lineSeq,			/* in: line transm sequence */
					/*     [0..numLine-1] */
					/*     or NULL if seq OK */
					/* BASIC BITSTREAM: */
	int *lineParaPred,		/* in: line predecessor idx */
					/*     in prev frame */
					/*     [0..numLine-1] */
	int *lineValid,			/* in: line validity flag */
					/*     [0..numLine-1] */
					/*     or NULL if all valid */
	int *lineParaEnv,		/* in: line para env flag */
					/*     [0..numLine-1] */
	unsigned long *lineParaIndex,	/* in: line para bits */
					/*     [0..numLine-1] */
	int *lineParaNumBit,		/* in: num line para bits */
					/*     [0..numLine-1] */

	int *lineParaFIndex,		/* in: line para freq index */
					/*     [0..numLine-1] */
	int *lineParaAIndex,		/* in: line para amp index */
					/*     [0..numLine-1] */
	int calcContIndex,		/* in: cont. index calc. flag */
					/*     0: don't accumulate */

					/* ENHANCEMENT BITSTREAM: */
	unsigned long *lineParaEnhaIndex,
					/* in: line para bits */
					/*     [0..numLine-1] */
					/*     or NULL */
	int *lineParaEnhaNumBit,	/* in: num line para bits */
					/*     [0..numLine-1] */
					/*     or NULL */
	double **quantLineFreq,		/* out: line frequency */
					/*      [0..numLine-1] */
	double **quantLineAmpl,		/* out: line amplitude */
					/*      [0..numLine-1] */
	int **quantLineEnv,		/* out: line envelope flag */
					/*      [0..numLine-1] */
	int **quantLinePred,		/* out: line predecessor idx */
					/*      in prev frame */
					/*      [0..numLine-1] */
	double **quantLineFreqEnha,	/* out: line frequency */
					/*      (incl. enhancement bits) */
					/*      [0..numLine-1] */
					/*      or NULL */
	double **quantLineAmplEnha,	/* out: line amplitude*/
					/*      (incl. enhancement bits) */
					/*      [0..numLine-1] */
					/*      or NULL */
	double **quantLinePhase)		/* out: line phase */
					/*      (from enhancement bits) */
					/*      [0..numLine-1] */
					/*      or NULL */
{
  int fint = 0;
  int aint,dfint,daint;
  int fintE = 0;
  int pintE;
  int fNumBitE;
  int i,iTrans,iPred;
  int iline;
  double freqRelStep;

  if (lineSeq!=NULL && lineValid!=NULL)
    IndiLineExit("IndiLineDequant: "
		 "lineSeq and lineValid cannot be used simultaneously!");

  /* dequantise lines */
  for (iTrans=0;iTrans<numLine;iTrans++) {
    if (lineSeq==NULL)
      i = iTrans;
    else
      i = lineSeq[iTrans];
    if (!lineParaPred[i]) {
      /* decode new line parameters */

      if ( lineParaFIndex )
      {
	ILQ->tmpPrevLineParaFIndex[iTrans]=fint=lineParaFIndex[i];
	ILQ->tmpPrevLineParaAIndex[iTrans]=aint=lineParaAIndex[i];

	/* dequantise new line parameters */
	if (lineValid==NULL || lineValid[i]) {
	  ILQ->quantLineFreq[iTrans] = FDeQuant(fint);
	  ILQ->quantLineAmpl[iTrans] = ADeQuant(ILQ,aint);
	}
	else {
	  ILQ->quantLineFreq[iTrans] = 0.0;
	  ILQ->quantLineAmpl[iTrans] = 0.0;
	}
      }
      else
      {
	if (lineParaNumBit[i] != ILQ->amplNumBit+ILQ->freqNumBit)
	  IndiLineExit("IndiLineDequant: bitnum error (new)");
	aint = (lineParaIndex[i]>>(ILQ->freqNumBit)) & ((1<<ILQ->amplNumBit)-1);
	fint = lineParaIndex[i] & ((1<<ILQ->freqNumBit)-1);

	ILQ->quantLineAmpl[iTrans] = exp((aint+.5)/ILQ->amplNumStep*ILQ->amplRange)*
	  ILQ->amplMin*ILQ->maxLineAmpl;
	ILQ->quantLineFreq[iTrans] = exp((fint+.5)/ILQ->freqNumStep*ILQ->freqRange)*
	  ILQ->freqMin;
      }

      ILQ->quantLinePred[iTrans] = 0;

      /* bit numbers for new line enhancement parameters */
      if(lineValid==NULL || lineValid[i])
	fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);	  
      else
	fNumBitE = 0;

      if (lineParaEnhaNumBit != NULL &&
	 lineParaEnhaNumBit[i] /* HP20010215 */ ) {

	/*>>>>>>>>>>*/
	if (ILQ->debugMode>=4) {
	  printf("ENHA %2d: n, fNumBitE=%d",i,fNumBitE);
	}
	/*<<<<<<<<<<*/

	/* decode new line enhancement parameters */
	if (lineParaEnhaNumBit[i] != fNumBitE+ILQ->phaseNumBitE)
	  IndiLineExit("IndiLineDequant: bitnum error (new,enha)");
	if (fNumBitE > 0)
	  fintE = (lineParaEnhaIndex[i]>>(ILQ->phaseNumBitE)) &
	    ((1<<fNumBitE)-1);

	/*>>>>>>>>>>*/
	if (ILQ->debugMode>=4) {
	  if (fNumBitE > 0)
	    printf(", fintE=%d\n",fintE);
	  else
	    printf("\n");
	}
	/*<<<<<<<<<<*/

	pintE = lineParaEnhaIndex[i] & ((1<<ILQ->phaseNumBitE)-1);
	/* dequantise new line enhancement parameters */
	if (fNumBitE > 0)
	{
	  if ( lineParaFIndex ) 
	  {
	    ILQ->quantLineFreqEnha[iTrans] = FDeQuantEnh(fint,fintE,fNumBitE);
	  }
	  else
	  {
	    ILQ->quantLineFreqEnha[iTrans] = ILQ->quantLineFreq[iTrans]*
	      (1+((fintE+.5)/ILQ->freqNumStepE[fNumBitE]-.5)*
	       (ILQ->freqRelStep-1));
	  }
	}
	else
	  ILQ->quantLineFreqEnha[iTrans] = ILQ->quantLineFreq[iTrans];
	ILQ->quantLinePhase[iTrans] = ((pintE+.5)/ILQ->phaseNumStepE-.5)*
	  ILQ->phaseRangeE;
      }
    }
    else {
      /* decode continued line parameters */

      /* dequantise continued line parameters */
      iPred = lineParaPred[i]-1;
      ILQ->quantLinePred[iTrans] = iPred+1;

      if ( lineParaFIndex ) {
	if (calcContIndex) {
	  ILQ->tmpPrevLineParaFIndex[iTrans] = fint
	    = ILQ->prevLineParaFIndex[iPred]+lineParaFIndex[i];
	  ILQ->tmpPrevLineParaAIndex[iTrans] = aint
	    = ILQ->prevLineParaAIndex[iPred]+lineParaAIndex[i];
	}
	else {
	  ILQ->tmpPrevLineParaFIndex[iTrans] = fint = lineParaFIndex[i];
	  ILQ->tmpPrevLineParaAIndex[iTrans] = aint = lineParaAIndex[i];
	}
	if (lineValid==NULL || lineValid[i]) {
	  ILQ->quantLineFreq[iTrans] = FDeQuant(fint);
	  ILQ->quantLineAmpl[iTrans] = ADeQuant(ILQ,aint);
	}
	else {
	  ILQ->quantLineFreq[iTrans] = 0.0;
	  ILQ->quantLineAmpl[iTrans] = 0.0;
	}

	/* bit numbers for continued line enhancement parameters */
	if(lineValid==NULL || lineValid[i])
	  fNumBitE = GetEnhNumBit(ILQ->freqThrE,fint);	  
	else
	  fNumBitE = 0;
      }
      else
      {
	if (lineParaNumBit[i] != ILQ->deltaAmplNumBit+ILQ->deltaFreqNumBit)
	  IndiLineExit("IndiLineDequant: bitnum error (cont)");
	daint = (lineParaIndex[i]>>(ILQ->deltaFreqNumBit)) &
	  ((1<<ILQ->deltaAmplNumBit)-1);
	dfint = lineParaIndex[i] & ((1<<ILQ->deltaFreqNumBit)-1);

	if (lineValid==NULL || lineValid[i]) {
	  ILQ->quantLineAmpl[iTrans] = ILQ->prevQuantLineAmplE[iPred]*
	    exp(((daint+1.0)/ILQ->deltaAmplNumStep-.5)*ILQ->deltaAmplRange)/
	    ((lineParaEnv[i])?ILQ->quantEnvAmplS:1);
	  ILQ->quantLineFreq[iTrans] = ILQ->prevQuantLineFreq[iPred]*
	    (1+((((dfint+1.0)/ILQ->deltaFreqNumStep)-.5)*ILQ->deltaFreqRange));
	}
	else {
	  ILQ->quantLineAmpl[iTrans] = 0.0;
	  ILQ->quantLineFreq[iTrans] = 0.0;
        }

	/* bit numbers for continued line enhancement parameters */
	if(lineValid==NULL || lineValid[i])
	  fNumBitE = ILQ->prevFreqEnhaNumBit[iPred];
	else
	  fNumBitE = 0;
      }

      if (lineParaEnhaNumBit != NULL &&
	 lineParaEnhaNumBit[i] /* HP20010215 */ ) {

	/*>>>>>>>>>>*/
	if (ILQ->debugMode>=4) {
	  printf("ENHA %2d: c, fNumBitE=%d",i,fNumBitE);
	}
	/*<<<<<<<<<<*/


	/* decode continued line enhancement parameters */
	if (lineParaEnhaNumBit[i] != ILQ->phaseNumBitE+fNumBitE)
	  IndiLineExit("IndiLineDequant: bitnum error (cont,enha)");
	if (fNumBitE > 0)
	  fintE = (lineParaEnhaIndex[i]>>(ILQ->phaseNumBitE)) &
	    ((1<<fNumBitE)-1);

	/*>>>>>>>>>>*/
	if (ILQ->debugMode>=4) {
	  if (fNumBitE > 0)
	    printf(", fintE=%d\n",fintE);
	  else
	    printf("\n");
	}
	/*<<<<<<<<<<*/

	pintE = lineParaEnhaIndex[i] & ((1<<ILQ->phaseNumBitE)-1);

	/* dequantise continued line enhancement parameters */
	if (fNumBitE > 0) {

	  if ( lineParaFIndex )
	  {
	    ILQ->quantLineFreqEnha[iTrans] = FDeQuantEnh(fint,fintE,fNumBitE);
	  }
	  else
	  {
	    freqRelStep = (ILQ->deltaFreqRange/ILQ->deltaFreqNumStep
			   *ILQ->prevQuantLineFreq[iPred]
			   /ILQ->quantLineFreq[iTrans])+1;

	    ILQ->quantLineFreqEnha[iTrans] = ILQ->quantLineFreq[iTrans]*
	      (1+((fintE+.5)/ILQ->freqNumStepE[fNumBitE]-.5)*
	       (ILQ->freqRelStep-1));
	  }
	}
	else
	  ILQ->quantLineFreqEnha[iTrans] = ILQ->quantLineFreq[iTrans];

	ILQ->quantLinePhase[iTrans] = ((pintE+.5)/ILQ->phaseNumStepE-.5)*
	  ILQ->phaseRangeE;
      }
    }

    ILQ->freqEnhaNumBit[iTrans] = fNumBitE;
    if (lineParaEnhaNumBit != NULL)
      ILQ->quantLineAmplEnha[iTrans] = ILQ->quantLineAmpl[iTrans];
    ILQ->quantLineEnv[iTrans] = lineParaEnv[i];
    if (ILQ->quantLineEnv[iTrans])
      ILQ->quantLineAmplE[iTrans] =
	ILQ->quantLineAmpl[iTrans]*ILQ->quantEnvAmplE;
    else
      ILQ->quantLineAmplE[iTrans] = ILQ->quantLineAmpl[iTrans];
  }

  for (iTrans=0;iTrans<numLine;iTrans++) {
    ILQ->prevLineParaFIndex[iTrans] = ILQ->tmpPrevLineParaFIndex[iTrans];
    ILQ->prevLineParaAIndex[iTrans] = ILQ->tmpPrevLineParaAIndex[iTrans];
  }

  /* set pointers to arrays with return values */
  *quantLineFreq = ILQ->quantLineFreq;
  *quantLineAmpl = ILQ->quantLineAmpl;
  *quantLineEnv = ILQ->quantLineEnv;
  *quantLinePred = ILQ->quantLinePred;
  if (quantLineFreqEnha != NULL)
    *quantLineFreqEnha = ILQ->quantLineFreqEnha;
  if (quantLineAmplEnha != NULL)
    *quantLineAmplEnha = ILQ->quantLineAmplEnha;
  if (quantLinePhase != NULL)
    *quantLinePhase = ILQ->quantLinePhase;

  /* copy parameters into frame-to-frame memory */
  ILQ->prevNumLine = numLine;
  for (iline=0;iline<numLine;iline++) {
    ILQ->prevQuantLineFreq[iline] = ILQ->quantLineFreq[iline];
    ILQ->prevQuantLineAmplE[iline] = ILQ->quantLineAmplE[iline];
    ILQ->prevFreqEnhaNumBit[iline] = ILQ->freqEnhaNumBit[iline];
  }
  if (ILQ->prevLineSeq!=NULL)
    for (iline=0;iline<numLine;iline++)
      ILQ->prevLineSeq[iline] = (lineSeq==NULL) ? iline : lineSeq[iline];
}

/* IndiLineQuantFree() */
/* Free memory allocated by IndiLineQuantInit(). */

void IndiLineQuantFree (
	ILQstatus *ILQ)			/* ILQ status handle */
{
  if (ILQ->lineParaPred != NULL) {
    free(ILQ->lineParaPred);
    free(ILQ->lineParaIndex);
    free(ILQ->lineParaNumBit);
    free(ILQ->lineParaEnhaIndex);
    free(ILQ->lineParaEnhaNumBit);
    free(ILQ->lineSeq);
    free(ILQ->prevLineSeq);
    free(ILQ->lineParaFIndex);
    free(ILQ->lineParaAIndex);
  }

  free(ILQ->quantLineAmplE);
  free(ILQ->freqEnhaNumBit);
  free(ILQ->quantEnvPara);
  free(ILQ->quantEnvParaEnha);
  free(ILQ->quantLineFreq);
  free(ILQ->quantLineAmpl);
  free(ILQ->quantLineEnv);
  free(ILQ->quantLinePred);
  free(ILQ->quantLineFreqEnha);
  free(ILQ->quantLineAmplEnha);
  free(ILQ->quantLinePhase);
  free(ILQ->prevQuantLineFreq);
  free(ILQ->prevQuantLineAmplE);
  free(ILQ->prevFreqEnhaNumBit);
  free(ILQ->prevLineParaFIndex);
  free(ILQ->prevLineParaAIndex);

  free(ILQ);
}


/* end of indilineqnt.c */
