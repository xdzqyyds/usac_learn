
/*

  This software module was originally developed by

  Masayuki Nishiguchi, Kazuyuki Iijima, and Jun Matsumoto (Sony Corporation)
  
  and edited by
  
  Heiko Purnhagen (University of Hannover) and
  Yuji Maeda (University of Hannover)
  
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
Dummy version of hvxcDec.c
                                                                       
Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-feb-97   HP    born (based on hvxcDec.c)
11-nov-97   HP    updated
22-apr-99   HP    updated
**********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "common_m4a.h"
#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

#include "pan_par_const.h"


/* Init HVXC decoder */
/* return a pointer to decoder status handler(AI 990129) */

HvxcDecStatus *hvxc_decode_init(
                                int varMode,		/* in: HVXC variable rate mode */
                                int rateMode,		/* in: HVXC bit rate mode */
                                int extensionFlag,	/* in: the presense of verion 2 data */
                                /* version 2 data (YM 990616) */
                                int numClass,
                                /* version 2 data (YM 990728) */
                                int vrScalFlag,
                                int delayMode,		/* in: decoder delay mode */
                                int testMode		/* in: test_mode for decoder conformance testing */
                                )
{
  CommonExit(1,"hvxc_decode_init: dummy");
  return (HvxcDecStatus *)NULL;
}

int IPC_DecParams1st(
                     unsigned char	*encBit,
                     float		pchmod,
                     int		*idVUV2,
                     float		(*qLspQueue)[LPCORDER],
                     float		*pch2,
                     float		(*am2)[SAMPLE/2][3],
                     IdCelp		*idCelp2,
                     HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"IPC_DecParams1st: dummy");
  return 1;
}

int IPC_DecParams1stVR(
                       unsigned char   *encBit,
                       float           pchmod,
                       int             *idVUV2,
                       int             *bgnFlag2,
                       float           (*qLspQueue)[LPCORDER],
                       float           *pch2,
                       float           (*am2)[SAMPLE/2][3],
                       IdCelp          *idCelp2,
                       HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"IPC_DecParams1stVR: dummy");
  return 1;
}

void IPC_DecParams2nd(
                      int	fr0,
                      int	*idVUV2,
                      float	(*qLspQueue)[LPCORDER],
                      IdCelp	*idCelp2,
                      float	(*qLsp2)[LPCORDER],
                      float	(*qRes2)[FRM],
                      HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"IPC_DecParams2nd: dummy");
}

void IPC_DecParams2ndVR(
                        int     fr0,
                        int     *idVUV2,
                        float   (*qLspQueue)[LPCORDER],
                        IdCelp  *idCelp2,
                        float   (*qLsp2)[LPCORDER],
                        float   (*qRes2)[FRM],
                        HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"IPC_DecParams2ndVR: dummy");
}

void IPC_SynthSC(
                 int	idVUV,
                 float	*qLsp,
                 float	mfdpch,
                 float	(*am)[3],
                 float	*qRes,
                 short int	*frmBuf,
                 HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"IPC_SynthSC: dummy");
}

void hvxc_decode(
                 HvxcDecStatus *HDS,		/* in: pointer to decoder status */
                 unsigned char *encBit,	/* in: bit stream */
                 float *sampleBuf,		/* out: frameNumSample audio samples */
                 int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
                 float speedFact,		/* in: speed change factor */
                 float pitchFact,		/* in: pitch change factor */
                 int decMode			/* in: decode rate mode */
                 /* version2 data */
                 , int epFlag,                 /* in: error protection flag */
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData
                 )
{
  CommonExit(1,"hvxc_decode: dummy");
}

void hvxc_decode_free(
                      HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  CommonExit(1,"hvxc_decode_free: dummy");
}


/* end of hvxcDec_dmy.c */
