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



#ifndef __HUFF_NODES_H__
#define __HUFF_NODES_H__

typedef struct {

  const int nodeTab[39][2];

} HUFF_RES_NODES;

typedef struct {

  const int nodeTab[30][2];

} HUFF_CLD_NOD_1D;

typedef struct {

  const int nodeTab[ 7][2];

} HUFF_ICC_NOD_1D;

typedef struct {

  const int nodeTab[50][2];

} HUFF_CPC_NOD_1D;


typedef struct {

  const int lav3[15][2];
  const int lav5[35][2];
  const int lav7[63][2];
  const int lav9[99][2];

} HUFF_CLD_NOD_2D;

typedef struct {

  const int lav1[ 3][2];
  const int lav3[15][2];
  const int lav5[35][2];
  const int lav7[63][2];

} HUFF_ICC_NOD_2D;

typedef struct {

  const int lav3 [ 15][2];
  const int lav6 [ 48][2];
  const int lav9 [ 99][2];
  const int lav12[168][2];

} HUFF_CPC_NOD_2D;


typedef struct {

  HUFF_CLD_NOD_1D h1D[3];
  HUFF_CLD_NOD_2D h2D[3][2];

} HUFF_CLD_NODES;

typedef struct {

  HUFF_ICC_NOD_1D h1D[3];
  HUFF_ICC_NOD_2D h2D[3][2];

} HUFF_ICC_NODES;

typedef struct {

  HUFF_CPC_NOD_1D h1D[3];
  HUFF_CPC_NOD_2D h2D[3][2];

} HUFF_CPC_NODES;


typedef struct {

  const int cld[30][2];
  const int icc[ 7][2];
  const int cpc[25][2];

} HUFF_PT0_NODES;

typedef struct {

  const int nodeTab[3][2];

} HUFF_LAV_NODES;


typedef struct {

  const int nodeTab[7][2];

} HUFF_IPD_NOD_1D;
typedef struct {

  const int lav1[ 3][2];
  const int lav3[15][2];
  const int lav5[35][2];
  const int lav7[63][2];

} HUFF_IPD_NOD_2D;

typedef struct {

  HUFF_IPD_NOD_1D hP0;
  HUFF_IPD_NOD_1D h1D[3];
  HUFF_IPD_NOD_2D h2D[3][2];

} HUFF_IPD_NODES;


#endif
