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

#ifndef __QMFLIB_QMFLIB_H__
#define __QMFLIB_QMFLIB_H__

#include "qmflib_const.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct QMFLIB_POLYPHASE_SYN_FILTERBANK QMFLIB_POLYPHASE_SYN_FILTERBANK;
typedef struct QMFLIB_POLYPHASE_ANA_FILTERBANK QMFLIB_POLYPHASE_ANA_FILTERBANK;

void
QMFlib_InitSynFilterbank(int resolution,
                         int LdMode);

void
QMFlib_OpenSynFilterbank(QMFLIB_POLYPHASE_SYN_FILTERBANK **self);


void
QMFlib_CloseSynFilterbank(QMFLIB_POLYPHASE_SYN_FILTERBANK *self);

void
QMFlib_CalculateSynFilterbank(QMFLIB_POLYPHASE_SYN_FILTERBANK *self,
                              float * Sr,
                              float * Si,
                              float * timeSig,
                              int LdMode);

void
QMFlib_InitAnaFilterbank(int resolution, int LdMode);

void
QMFlib_OpenAnaFilterbank(QMFLIB_POLYPHASE_ANA_FILTERBANK **self);

void
QMFlib_CloseAnaFilterbank(QMFLIB_POLYPHASE_ANA_FILTERBANK *self);

void
QMFlib_CalculateAnaFilterbank(QMFLIB_POLYPHASE_ANA_FILTERBANK *self,
                              float * timeSig,
                              float * Sr,
                              float * Si,
                              int	LdMode);

void
QMFlib_GetFilterbankPrototype(int resolution,
                              float * prototype,
                              int LdMode);

#ifdef __cplusplus
}
#endif
#endif
