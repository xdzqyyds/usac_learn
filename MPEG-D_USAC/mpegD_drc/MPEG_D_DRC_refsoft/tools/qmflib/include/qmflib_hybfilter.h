/*******************************************************************************
 This software module was originally developed by
 
 Coding Technologies, Fraunhofer IIS, Philips
 
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
 
 Coding Technologies, Fraunhofer IIS, Philips retain full right to
 modify and use the code for its (their) own purpose, assign or donate the code
 to a third party and to inhibit third parties from using the code for products
 that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
 International Standards. This copyright notice must be included in all copies or
 derivative works.
 
 Copyright (c) ISO/IEC 2007.
 *******************************************************************************/

#ifndef __QMFLIB_HYBFILTER_H__
#define __QMFLIB_HYBFILTER_H__

#include "qmflib_const.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
  QMFLIB_HYBRID_OFF,
  QMFLIB_HYBRID_THREE_TO_TEN,
  QMFLIB_HYBRID_THREE_TO_SIXTEEN
  
} QMFLIB_HYBRID_FILTER_MODE;


typedef struct QMFLIB_HYBRID_FILTER_STATE
{
  float bufferLFReal[QMFLIB_QMF_BANDS_TO_HYBRID][QMFLIB_BUFFER_LEN_LF];
  float bufferLFImag[QMFLIB_QMF_BANDS_TO_HYBRID][QMFLIB_BUFFER_LEN_LF];
  float bufferHFReal[QMFLIB_MAX_NUM_QMF_BANDS][QMFLIB_BUFFER_LEN_HF];
  float bufferHFImag[QMFLIB_MAX_NUM_QMF_BANDS][QMFLIB_BUFFER_LEN_HF];
  
} QMFLIB_HYBRID_FILTER_STATE;

int QMFlib_GetHybridSubbands(int qmfSubbands, QMFLIB_HYBRID_FILTER_MODE hybridMode);
int QMFlib_GetQmfSubband(int hybridSubband, QMFLIB_HYBRID_FILTER_MODE hybridMode, int LdMode);
int QMFlib_GetParameterPhase(int hybridSubband);
void QMFlib_InitAnaHybFilterbank(QMFLIB_HYBRID_FILTER_STATE *hybState);

void QMFlib_ApplyAnaHybFilterbank(QMFLIB_HYBRID_FILTER_STATE *hybState,
                                  QMFLIB_HYBRID_FILTER_MODE hybridMode,
                                  int nrBands,
                                  int delayAlign, /* 0 -> align HF with LF part, 1 -> no delay alignment for HF part */
                                  float mQmfReal[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mQmfImag[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mHybridReal[QMFLIB_MAX_HYBRID_BANDS],
                                  float mHybridImag[QMFLIB_MAX_HYBRID_BANDS]);

void QMFlib_ApplySynHybFilterbank(
                                  int nrBands,
                                  QMFLIB_HYBRID_FILTER_MODE hybridMode,
                                  float mHybridReal[QMFLIB_MAX_HYBRID_BANDS],                              
                                  float mHybridImag[QMFLIB_MAX_HYBRID_BANDS],
                                  float mQmfReal[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mQmfImag[QMFLIB_MAX_NUM_QMF_BANDS]);

#ifdef __cplusplus
}
#endif
#endif
