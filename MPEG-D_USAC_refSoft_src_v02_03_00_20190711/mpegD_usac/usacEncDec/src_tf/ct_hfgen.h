/****************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3
standards for reference purposes and its performance may not have been
optimized. This software module is an implementation of one or more tools as
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications
thereof for use in products claiming conformance to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International
Standards. ISO/IEC gives users the same free license to this software module or
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its
use may infringe existing patents. ISO/IEC have no liability for use of this
software module or modifications thereof. Copyright is not released for
products that do not conform to audiovisual and image-coding related ITU
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its
own purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_hfgen.h,v 1.7.4.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  HF Generator $Revision: 1.7.4.1 $
*/

#ifndef __CT_HFGEN_H
#define __CT_HFGEN_H

#include "ct_envextr.h"

#define MAX_NUM_PATCHES   6

struct PATCH {
  int noOfPatches;
  int targetStartBand[MAX_NUM_PATCHES+1];
};

struct ACORR_COEFS {
  float  r11r;
  float  r01r;
  float  r01i;
  float  r02r;
  float  r02i;
  float  r12r;
  float  r12i;
  float  r22r;
  float  det;
};

void
generateHF (float sourceBufferReal[][64],
            float sourceBufferImag[][64],
            float sourceBufferPVReal[][64],
            float sourceBufferPVImag[][64],
            int      bCELPMode,
            int      bUseHBE,
            float targetBufferReal[][64],
            float targetBufferImag[][64],
            INVF_MODE *invFiltMode,
            INVF_MODE *prevInvFiltMode,
            int *invFiltBandTable,
            int noInvFiltBands,
            int highBandStartSb,
            int *v_k_master,
            int numMaster,
            int fs,
            int* frameInfo,
#ifdef LOW_POWER_SBR
            float *degreeAlias,
#endif
            int channel, int el
#ifdef AAC_ELD
            ,int numTimeSlots
            ,int ldsbr
#endif
            ,int bPreFlattening
            ,int bSbr41
            );


#endif /* CT_HFGEN_H */
