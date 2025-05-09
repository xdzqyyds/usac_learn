/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



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

$Id: dec_tf.h,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $
TF Coder module
**********************************************************************/


#ifndef _dec_tf_h_
#define _dec_tf_h_



#include "allHandles.h"
#include "lpc_common.h"         /* TDS */
#include "all.h"
#include "tns.h"
#include "huffdec2.h"
#include "resilience.h"
#include "flex_mux.h"
#include "ntt_conf.h"
#include "dec_hvxc.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* ---------- functions ---------- */
/* DecTfInfo() */
/* Get info about t/f-based decoder core. */

char *DecTfInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecTfInit() */
/* Init t/f-based decoder core. */

void DecTfInit(
               char*               decPara,          /* in: decoder parameter string */    
               char*               aacDebugStr,
               int                 epDebugLevel,
               HANDLE_DECODER_GENERAL hFault,
               FRAME_DATA*         frameData,
               TF_DATA*            tfData,
               LPC_DATA*           lpcData,
               int                 layer,
               char*               infoFileName
             );


/* DecTfFrame() */
/* Decode one bit stream frame into one audio frame with */
/* t/f-based decoder core. */
void DecTfFrame(int          numChannels, 
                FRAME_DATA*  fd, /* config data , obj descr. etc. */
                TF_DATA*     tfData, /* actual frame input/output data */
                LPC_DATA*    lpcData,
                HANDLE_DECODER_GENERAL hFault,/* additional data for error protection */
								int *numOutChannels /* SAMSUNG_2005-09-30 */
                );


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ( 
                TF_DATA *tfData,
                HANDLE_DECODER_GENERAL hFault
                );


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_tf_h_ */

/* end of dec_tf.h */
