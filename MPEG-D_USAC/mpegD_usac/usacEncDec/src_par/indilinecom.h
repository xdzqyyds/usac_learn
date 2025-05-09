/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - individual lines: common module



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

Header file: indilinecom.h


Required libraries:
(none)

Required modules:
common.o

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
01-sep-96   HP    first version based on indiline.c
10-sep-96   BE
11-sep-96   HP
06-jun-97   HP    added M_PI
30-mar-98   BE/HP initPrevNumLine
01-Mar-2002 HP    increasing constant's precision to double or better
**********************************************************************/


#ifndef _indilinecom_h_
#define _indilinecom_h_


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations ---------- */

#define ENVPARANUM 3

#define MAX_LAYER	8

#ifdef M_PI
#undef M_PI
#endif
/* #define M_PI 3.14159265358979323846 */
#define M_PI 3.1415926535897932384626433832795029  /* pi */

#define PNL_UNKNOWN (-1)	/* number of previous lines unknown */
#define NUMLINE_INVALID (-1)	/* number of indiv. lines invalid */
#define HARM_INVALID (-1)	/* harmonic invalid */
#define NOISE_INVALID (128)	/* noise invalid */



#define	MAXAMPLDIFF	50

/*
 * parameters for the LPC-coding of the residual noise
 *
 */

/* maximum Power amplification of the synthesis filter	*/
/* amplification is limited to keep the filter off the	*/
/* area of instability and gives a limitation for	*/
/* the resulting LARs as well				*/

#define lpc_max_amp		10000.0	


/* Parameters for encoding the LPC parameters	*/
/* Attention:					*/
/* if this is changed the bitstreams will be	*/
/* incompatible to the old ones			*/


typedef struct {
	float	mean;
	float	corr;	/* Correlation coefficient			*/
	float	qstep;	/* quantizer step size				*/
	float	rpos;	/* representative value relative position	*/
	int	xbits;	/* extra bits for refinement			*/
} Tlar_enc_par;


/* Filterlength and oversampling of the resampling filter	*/
/* used for pitch-change in the LPC-noise-synthesis		*/

#define LPCsynLPlen	8
#define LPCsynLPos	4


/*
 * parameters for the LPC-coding of harmonic tones
 *
 */

#define HSTRETCHQ	(0.002/32.0)	/* Quantizer step for hstretch		*/
#define HSTRETCHMAX	17		/* dont't change - limited by coding scheme	*/

#define HQSTEP	0.15

typedef struct {
	int		zstart;		/* start index of zero-coding	*/
	int		nhcp;		/* number of coding parameters	*/
	Tlar_enc_par	*hcp;		/* coding parameter table	*/
	int		numParaTable[16]; /* Number of parameters	*/
} TLPCEncPara;


#define LPCMaxNoisePara	26
#define LPCMaxHarmPara	26

#define LPCMaxNumPara		(LPCMaxNoisePara<LPCMaxHarmPara?LPCMaxHarmPara:LPCMaxNoisePara)


#define STEPSPERBARK 32
#define STEPSPERDB (1.0/1.5)
/* #define LOG10 (2.302585093) */
#define LOG10 2.3025850929940456840179914546843642  /* log_e 10 */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* IndiLineExit() */
/* Print "indiline" error message to stderr and exit program. */
void IndiLineExit(
  char *message,		/* in: error message */
  ...);				/* in: args as for printf */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _indilinecom_h_ */

/* end of indilinecom.h */
