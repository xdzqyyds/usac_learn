
/*

This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
This software module is an implementation of a part of one or more
MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
standard (ISO/IEC 14496-3).
ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
free license to this software module or modifications thereof for use
in hardware or software products claiming conformance to the MPEG-4
Audio standards (ISO/IEC 14496-3).
Those intending to use this software module in hardware or software
products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company,
the subsequent editors and their companies, and ISO/IEC have no
liability for use of this software module or modifications thereof in
an implementation.
Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
conforming products. The original developer retains full right to use
the code for his/her own purpose, assign or donate the code to a third
party and to inhibit third party from using the code for non MPEG-4
Audio (ISO/IEC 14496-3) conforming products.
This copyright notice must be included in all copies or derivative works.

Copyright (c)1996.

*/

/**********************************************************************
Dummy version of hvxc lpc functions

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
11-nov-97   HP    born
12-nov-97   HP    updated to new CELP
**********************************************************************/

#include <stdio.h>

#include "common_m4a.h"
#include "phi_priv.h"	/* PRIV */

#ifdef __cplusplus
extern "C" {
#endif


void
PAN_InitLpcAnalysisEncoder
(
long  order,                    /* In:  Order of LPC                    */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
  CommonExit(1,"PAN_InitLpcAnalysisEncoder: dummy");
}

void
PAN_FreeLpcAnalysisEncoder
(
 PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
 )
{
  CommonExit(1,"PAN_FreeLpcAnalysisEncoder: dummy");
}


void	PHI_Init_Private_Data(PHI_PRIV_TYPE	*PHI_Priv)
{
  CommonExit(1,"PHI_Init_Private_Data: dummy");
}

#ifdef __cplusplus
}
#endif

/* end of hvxc_lpc_dmy.c */
