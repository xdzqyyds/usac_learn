/**********************************************************************
MPEG4 Audio VM Module
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



Header file: indilineqnt.h

 

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
23-jun-97   HP    moved ILQ stuff to c file
16-nov-97   HP    harm/indi quant mode
16-feb-98   HP    improved noiseFreq
30-mar-98   BE/HP lineValid
25-jan-01   BE    float -> double
**********************************************************************/


#ifndef _indilineqnt_h_
#define _indilineqnt_h_

#include "indilinecom.h"	/* indiline common module */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

typedef struct ILQstatusStruct ILQstatus;	/* ILQ status handle */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ----- initialisation ----- */

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
	int bsf);			/* bitstream format	*/


/* ----- quantisation & dequantisation ----- */

/* IndiLineNumBit() */
/* Determine number of bits in basic bitstream for quantised */
/* envelope parameters and individual line parameters. */
/* If ILQ=NULL: minimum number of bits. */

void IndiLineNumBit (
	ILQstatus *ILQ,			/* in: ILQ status handle */
					/*     or NULL */
					/* BASIC BITSTREAM: */
	int *envNumBit,			/* out: num bits envelope */
	int *newLineNumBit,		/* out: num bits new line */
	int *contLineNumBit);		/* out: num bits continued line */


/* IndiLineEnhaNumBit() */
/* Determine number of bits in enhancement bitstream for quantised */
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
	int **lineEnhaNumBit);		/* out: num bits line */
					/*      [0..numLine-1] */


/* IndiLineEnvQuant() */
/* Quantisation of individual envelope parameters. */

void IndiLineEnvQuant (
	ILQstatus *ILQ,			/* ILQ status handle */
	double *envPara,		/* in: envelope parameters */
					/*     [0..ENVPARANUM-1] */
					/* BASIC BITSTREAM: */
	int *envParaFlag,		/* out: env flag (1 bit) */
	unsigned int *envParaIndex,	/* out: envelope bits */
	int *envParaNumBit,		/* out: num envelope bits */
					/* ENHANCEMENT BITSTREAM: */
	unsigned int *envParaEnhaIndex,	/* out: envelope bits */
	int *envParaEnhaNumBit);	/* out: num envelope bits */


/* IndiLineEnvDequant() */
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
	double **quantEnvParaEnha);	/* out: envelope parameters */
					/*      (incl. enhancement bits) */
					/*      [0..ENVPARANUM-1] */
					/*      or NULL */


/* IndiLineQuant() */
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
					/*     [0..maxNumLine-1] */
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
	int **prevLineParaAIndex);	/* out: prev. line para amp. index if ...FIndex!=NULL) */
					/*      [0..numLine-1] */


/* IndiLineSequence() */
/* Determine transmission sequence for numLine lines. */

void IndiLineSequence (
	ILQstatus *ILQ,			/* ILQ status handle */
	int numLine,			/* in: num lines to transmit */
	int *lineParaPred,		/* in: line predecessor idx */
					/*     in prev transm frame */
					/*     [0..numLine-1] */
	int *lineParaFIndex,		/* in: line freq. index, used to */
					/*     sort not continued lines  */
					/*     [0..numLine-1]            */
	int **lineSeq,			/* out: line transm sequence */
					/*      [0..numLine-1] */
	int *layNumLine,
	int numLayer);


/* IndiLineDequant() */
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
	double **quantLinePhase);	/* out: line phase */
					/*      (from enhancement bits) */
					/*      [0..numLine-1] */
					/*      or NULL */


int AQuant(ILQstatus *ILQ,double a);
double ADeQuant(ILQstatus *ILQ,int ai);

/* ----- free memory ----- */

/* IndiLineQuantFree() */
/* Free memory allocated by IndiLineQuantInit(). */

void IndiLineQuantFree (
	ILQstatus *ILQ);		/* ILQ status handle */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilineqnt_h_ */

/* end of indilineqnt.h */
