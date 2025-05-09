/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Akira Inoue   (Sony Corporation) and
Yuji Maeda    (Sony Corporation)
Markus Werner (SEED / Software Development Karlsruhe) 

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

Copyright (c) 2000.

$Id: dec_par.h,v 1.1.1.1 2009-05-29 14:10:13 mul Exp $
Parametric decoder module
**********************************************************************/
#ifndef _dec_par_h_
#define _dec_par_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"

#include "dec_hvxc.h"

/* ---------- structures ---------- */

/* para decoder status struct   HP 20000330 */

typedef struct ParaDecStatusStruct ParaDecStatus;
typedef struct ParaDecStatusStruct PARA_DATA;

struct ParaDecStatusStruct
{
  /* HP 20000330 */

  float speedFact;
  float pitchFact;
 
  float **sampleBuf;
  int frameNumSample;

  int delayNumSample;
  int frameMaxNumSample;

  /* configuration variables */
  int dummy;
};

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* DecHvxcInfo() */
/* Get info about HVXC decoder core. */

char *DecHvxcInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */

/* DecHvxcInitNew() */
/* Init HVXC decoder core for system interface(AI 990616) */

HvxcDecStatus *DecHvxcInit(
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
);		

/* DecHvxcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core - system interface(AI 990616) */

void DecHvxcFrame(
  FRAME_DATA*  fD,
  HvxcDecStatus* hvxcD,		/* in: HVXC decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  );

/* DecHvxcFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree (
  HvxcDecStatus *HDS		/* in: HVXC decoder status handle */
);


/* DecParInfo() */
/* Get info about parametric decoder core. */

char *DecParInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */



/* DecParInit() */
/* Init parametric decoder core for system interface   HP 20000330 */

ParaDecStatus *DecParInit(
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
);		

/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core - system interface   HP 20000330 */

void DecParFrame(
  FRAME_DATA*  fD,
  ParaDecStatus* paraD,		/* in: para decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  int epFlag,                   /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  );


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree (ParaDecStatus *paraD);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_par_h_ */

/* end of dec_par.h */



