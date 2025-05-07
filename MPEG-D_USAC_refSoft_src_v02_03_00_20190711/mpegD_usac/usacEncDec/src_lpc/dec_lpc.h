/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by
Markus Werner       (SEED / Software Development Karlsruhe) 

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

$Id: dec_lpc.h,v 1.1.1.1 2009-05-29 14:10:11 mul Exp $
LPC Coder module
**********************************************************************/


#ifndef _dec_lpc_h_
#define _dec_lpc_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"

#include "obj_descr.h"
#include "lpc_common.h"         /* TDS */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* DecLpcInfo() */
/* Get info about LPC-based decoder core. */

char *DecLpcInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecLpcInit() */
/* Init LPC-based decoder core. */

void DecLpcInit(
  char *decPara,                   /* in: decoder parameter string     */
  FRAME_DATA*  frameData,
  LPC_DATA*    lpcData
  ) ;            /* out: decoder delay (num samples) */


/* DecLpcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* LPC-based decoder core. */


void DecLpcFrame(
  FRAME_DATA*  frameData,
  LPC_DATA*    lpcData,
  int *usedNumBit);        /* out: num bits used for this frame         */

/* DecLpcFree() */
/* Free memory allocated by LPC-based decoder core. */

void DecLpcFree (LPC_DATA*       lpcData);

int lpcframelength( CELP_SPECIFIC_CONFIG *celpConf, int layer );
int erlpcframelength( CELP_SPECIFIC_CONFIG *erCelpConf, int layer );

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_lpc_h_ */

/* end of dec_lpc.h */
