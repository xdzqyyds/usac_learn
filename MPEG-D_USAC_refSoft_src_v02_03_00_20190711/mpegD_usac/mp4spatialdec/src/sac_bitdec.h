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




#ifndef __SAC_BITDEC_H
#define __SAC_BITDEC_H

#include "sac_dec.h"

enum {
  TREE_5151 = 0,
  TREE_5152 = 1,
  TREE_525  = 2,
  TREE_7271 = 3,
  TREE_7272 = 4,
  TREE_7571 = 5,
  TREE_7572 = 6,
  TREE_212  = 7
};

enum {
  QUANT_FINE = 0,
  QUANT_EBQ1 = 1,
  QUANT_EBQ2 = 2
};

void GetUsacMps212SpecificConfig(spatialDec* self, int bsFrameLength, int bsResidualCoding, SAC_DEC_USAC_MPS212_CONFIG *pUsacMps212Config);

void SpatialDecParseSpecificConfig(spatialDec* self, int sacHeaderLen, int *pnBitsRead, int bsFrameLength, int bsResidualCoding);

void SpatialDecDefaultSpecificConfig(spatialDec* self);

void SpatialDecInitResidual(spatialDec* self);

void SpatialDecDecodeFrame(spatialDec* self);

void SpatialDecDecodeHeader(spatialDec* self);

void SpatialGetDequantTables(float **cld, float **icc, float **cpc);

float SpatialDecQuantizeCLD(float v);

float SpatialDecQuantizeICC(float v);

#endif
