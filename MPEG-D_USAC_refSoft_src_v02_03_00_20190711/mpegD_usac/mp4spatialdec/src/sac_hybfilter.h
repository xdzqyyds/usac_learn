/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/

#ifndef __SAC_HYB_H
#define __SAC_HYB_H

#include "sac_dec.h"

int SacGetHybridSubbands(int qmfSubbands);
int SacGetQmfSubband(int hybridSubband);
int SacGetParameterPhase(int hybridSubband);
void SacInitAnaHybFilterbank(tHybFilterState *hybState);
void SacInitSynHybFilterbank(void);

void SacApplyAnaHybFilterbank(tHybFilterState *hybState,
                              float mQmfReal[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS],
                              float mQmfImag[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS],
                              int nrBands,
                              int nrSamples,
                              float mHybridReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              float mHybridImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]);

void SacApplySynHybFilterbank(float mHybridReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              float mHybridImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              int nrBands,
                              int nrSamples,
                              float mQmfReal[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS],
                              float mQmfImag[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS]);

void SacCloseAnaHybFilterbank(void);
void SacCloseSynHybFilterbank(void);

#endif
