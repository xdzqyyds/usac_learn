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




#include "sac_bitdec.h"
#include "sac_bitinput.h"
#include "sac_resdecode.h"
#include "sac_resdefs.h"
#include "sac_bitinput.h"
#include "sac_nlc_dec.h"
#include "sac_hybfilter.h"

#include <assert.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifdef PARTIALLY_COMPLEX
#define MAX_LP_RESIDUAL_BANDS_28    (12)
#endif

typedef struct {
  int numInputChannels;
  int numOutputChannels;
  int numOttBoxes;
  int numTttBoxes;
  int ottModeLfe[MAX_NUM_OTT];
} TREEPROPERTIES;



static int freqResTable[] = {0, 28, 20, 14, 10, 7, 5, 4};

static int tempShapeChanTable[][8] = { {5, 5, 4, 6, 6, 4, 4, 2}, {5, 5, 5, 7, 7, 4, 4, 2} };

static TREEPROPERTIES treePropertyTable[] = {
  {1, 6, 5, 0, {0, 0, 0, 0, 1}},
  {1, 6, 5, 0, {0, 0, 1, 0, 0}},
  {2, 6, 3, 1, {1, 0, 0, 0, 0}},
  {2, 8, 5, 1, {1, 0, 0, 0, 0}},
  {2, 8, 5, 1, {1, 0, 0, 0, 0}},
  {6, 8, 2, 0, {0, 0, 0, 0, 0}},
  {6, 8, 2, 0, {0, 0, 0, 0, 0}},
  {1, 2, 1, 0, {0, 0, 0, 0, 0}}
};

static int kernels_4_to_71[MAX_HYBRID_BANDS][2] = {
  { 0, -1}, { 0, -1}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}, { 1,  1},
  { 1,  1}, { 1,  1}, { 1,  1}, { 1,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1},
  { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1},
  { 2,  1}, { 2,  1}, { 2,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}
};

static int kernels_5_to_71[MAX_HYBRID_BANDS][2] = {
  { 0, -1}, { 0, -1}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}, { 1,  1}, { 1,  1}, { 1,  1},
  { 1,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 2,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1},
  { 3,  1}, { 3,  1}, { 3,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}, { 4,  1}
};

static int kernels_7_to_71[MAX_HYBRID_BANDS][2] = {
  { 0, -1}, { 0, -1}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}, { 1,  1}, { 1,  1}, { 2,  1},
  { 2,  1}, { 2,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 3,  1}, { 4,  1}, { 4,  1}, { 4,  1},
  { 4,  1}, { 4,  1}, { 4,  1}, { 5,  1}, { 5,  1}, { 5,  1}, { 5,  1}, { 5,  1}, { 5,  1},
  { 5,  1}, { 5,  1}, { 5,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1},
  { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}, { 6,  1}
};

static int kernels_10_to_71[MAX_HYBRID_BANDS][2] = {
  { 0, -1}, { 0, -1}, { 0,  1}, { 0,  1}, { 1,  1}, { 1,  1}, { 2,  1}, { 2,  1}, { 3,  1},
  { 3,  1}, { 4,  1}, { 4,  1}, { 5,  1}, { 5,  1}, { 6,  1}, { 6,  1}, { 7,  1}, { 7,  1},
  { 7,  1}, { 7,  1}, { 7,  1}, { 8,  1}, { 8,  1}, { 8,  1}, { 8,  1}, { 8,  1}, { 8,  1},
  { 8,  1}, { 8,  1}, { 8,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}, { 9,  1}
};

static int kernels_14_to_71[MAX_HYBRID_BANDS][2] = {
  { 0, -1}, { 0, -1}, { 0,  1}, { 0,  1}, { 1,  1}, { 1,  1}, { 2,  1}, { 3,  1}, { 4,  1},
  { 4,  1}, { 5,  1}, { 6,  1}, { 6,  1}, { 7,  1}, { 7,  1}, { 8,  1}, { 8,  1}, { 8,  1},
  { 9,  1}, { 9,  1}, { 9,  1}, {10,  1}, {10,  1}, {10,  1}, {10,  1}, {11,  1}, {11,  1},
  {11,  1}, {11,  1}, {11,  1}, {12,  1}, {12,  1}, {12,  1}, {12,  1}, {12,  1}, {12,  1},
  {12,  1}, {12,  1}, {12,  1}, {12,  1}, {12,  1}, {12,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1},
  {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}, {13,  1}
};

static int kernels_20_to_71[MAX_HYBRID_BANDS][2] = {
  { 1, -1}, { 0, -1}, { 0,  1}, { 1,  1}, { 2,  1}, { 3,  1}, { 4,  1}, { 5,  1}, { 6,  1},
  { 7,  1}, { 8,  1}, { 9,  1}, {10,  1}, {11,  1}, {12,  1}, {13,  1}, {14,  1}, {14,  1},
  {15,  1}, {15,  1}, {15,  1}, {16,  1}, {16,  1}, {16,  1}, {16,  1}, {17,  1}, {17,  1},
  {17,  1}, {17,  1}, {17,  1}, {18,  1}, {18,  1}, {18,  1}, {18,  1}, {18,  1}, {18,  1},
  {18,  1}, {18,  1}, {18,  1}, {18,  1}, {18,  1}, {18,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1},
  {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}, {19,  1}
};

static int kernels_28_to_71[MAX_HYBRID_BANDS][2] = {
  { 1, -1}, { 0, -1}, { 0,  1}, { 1,  1}, { 2,  1}, { 3,  1}, { 4,  1}, { 5,  1}, { 6,  1},
  { 7,  1}, { 8,  1}, { 9,  1}, {10,  1}, {11,  1}, {12,  1}, {13,  1}, {14,  1}, {15,  1},
  {16,  1}, {17,  1}, {17,  1}, {18,  1}, {18,  1}, {19,  1}, {19,  1}, {20,  1}, {20,  1},
  {21,  1}, {21,  1}, {21,  1}, {22,  1}, {22,  1}, {22,  1}, {23,  1}, {23,  1}, {23,  1},
  {23,  1}, {24,  1}, {24,  1}, {24,  1}, {24,  1}, {24,  1}, {25,  1}, {25,  1}, {25,  1},
  {25,  1}, {25,  1}, {25,  1}, {26,  1}, {26,  1}, {26,  1}, {26,  1}, {26,  1}, {26,  1},
  {26,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1},
  {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}, {27,  1}
};

static int mapping_4_to_28[MAX_PARAMETER_BANDS] = {
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3
};

static int mapping_5_to_28[MAX_PARAMETER_BANDS] = {
  0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4
};

static int mapping_7_to_28[MAX_PARAMETER_BANDS] = {
  0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6
};

static int mapping_10_to_28[MAX_PARAMETER_BANDS] = {
  0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9
};

static int mapping_14_to_28[MAX_PARAMETER_BANDS] = {
  0, 0, 1, 1, 2, 3, 4, 4, 5, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 13, 13, 13
};

static int mapping_20_to_28[MAX_PARAMETER_BANDS] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 18, 19, 19, 19
};

static float surroundGainTable[]  = {1.000000f, 1.189207f, 1.414213f, 1.681792f, 2.000000f};
static float lfeGainTable[]       = {1.000000f, 3.162277f, 10.000000f, 31.622776f, 100.000000f};
static float clipGainTable[]      = {1.000000f, 1.189207f, 1.414213f, 1.681792f, 2.000000f, 2.378414f, 2.828427f, 4.000000f};

static int pbStrideTable[] = {1, 2, 5, 28};

static float dequantCLD[] =       {-150.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0, -16.0, -13.0, -10.0, -8.0, -6.0, -4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 13.0, 16.0, 19.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 150.0};

static float dequantCPC[] =       {-2.0f, -1.9f, -1.8f, -1.7f, -1.6f, -1.5f, -1.4f, -1.3f, -1.2f, -1.1f, -1.0f, -0.9f, -0.8f, -0.7f, -0.6f, -0.5f, -0.4f, -0.3f, -0.2f, -0.1f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f, 2.1f, 2.2f, 2.3f, 2.4f, 2.5f, 2.6f, 2.7f, 2.8f, 2.9f, 3.0};

static float dequantICC[] = {1.0000f, 0.9370f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.5890f, -0.9900f};

static float dequantIPD[] = {0.f, 0.392699082f, 0.785398163f, 1.178097245f, 1.570796327f, 1.963495408f, 2.35619449f, 2.748893572f, 3.141592654f, 3.534291735f, 3.926990817f, 4.319689899f, 4.71238898f, 5.105088062f, 5.497787144f, 5.890486225f};

static int smgTimeTable[] = {64, 128, 256, 512};


static void longmult1(unsigned short a[], unsigned short b, unsigned short d[], int len)
{
  int k;
  unsigned long tmp;
  unsigned long b0 = (unsigned long)b;

  tmp  = ((unsigned long)a[0])*b0;
  d[0]  = (unsigned short)tmp;

  for (k=1; k < len; k++)
  {
    tmp  = (tmp >>16) + ((unsigned long)a[k])*b0; 
    d[k]  = (unsigned short)tmp;                                 
  }

}

static void longdiv(unsigned short b[], unsigned short a, unsigned short d[], unsigned short *pr, int len)
{
  unsigned long r;
  unsigned long tmp;
  int k;

  assert (a != 0);

  r = 0;                                                         
                                                                 
  for (k=len-1; k >= 0; k--)
  {
    tmp = ((unsigned long)b[k]) + (r << 16);                     
                                                                 
    if (tmp)
    {
       d[k] = (unsigned short) (tmp/a);                         
       r = tmp - d[k]*a;                                         
    }
    else
    {
      d[k] = 0;                                                  
    }
  }
  *pr = (unsigned short)r;                                       

}

static short long_norm_l (int x)
{
  short bits = 0;

  if (x != 0)
  {
    if (x < 0)
    {
        x = ~x;
        if (x == 0)  return 31;
    }
    for (bits = 0; x < (int) 0x40000000L; bits++)
    {
      x <<= 1;
    }
  }

  return (bits);
}

static void longsub(unsigned short a[], unsigned short b[], int lena, int lenb)
{
    int h;
    long carry = 0;

    assert(lena >= lenb);
    for (h=0; h < lenb; h++)
    {
        carry += ((unsigned long)a[h]) - ((unsigned long)b[h]);
        a[h]  = (unsigned short) carry;
        carry = carry >> 16;
    }

    for (; h < lena; h++)
    {
        carry = ((unsigned long)a[h]) + carry;
        a[h] = (unsigned short) carry;
        carry = carry >> 16;
    }

    assert(carry == 0);   /* carry != 0 indicates subtraction underflow, e.g. b > a */
    return;
}

static int longcntbits(unsigned short a[], int len)
{
  int k;
  int bits = 0;

  for (k=len-1; k>=0; k--)
  {
                                                               
    if (a[k] != 0)
    {
      bits = k*16 + 31 - long_norm_l((int)a[k]);             
      break;
    }
  }

  return bits;
}

static int longcompare(unsigned short a[], unsigned short b[], int len)
{
   int i;
   
   for (i=len-1; i > 0; i--)
   {
      if (a[i] != b[i])
         break;
   }
   return (a[i] >= b[i]) ? 1 : 0;
}

static void SpatialDecParseExtensionConfig(HANDLE_S_BITINPUT bitstream,
                                           SPATIAL_BS_CONFIG *config,
                                           int numOttBoxes,
                                           int numTttBoxes,
                                           int numOutChan,
                                           int bitsAvailable, 
                                           int *pnBitsRead) {


  int i, ch, idx, tmp, tmpOpen, sacExtLen, bitsRead, nFillBits;
  int ba = bitsAvailable;

  config->sacExtCnt = 0;

  while (ba >= 8) {

    config->bsSacExtType[config->sacExtCnt] = s_GetBits(bitstream, 4);
    ba -= 4;

    sacExtLen = s_GetBits(bitstream, 4);
    ba -= 4;
    if (sacExtLen == 15) {
      sacExtLen += s_GetBits(bitstream, 8);
      ba -= 8;
      if (sacExtLen == 15+255) {
        sacExtLen += s_GetBits(bitstream, 16);
        ba -= 16;
      }
    }

	if ( sacExtLen <= 0 ) {
      break;
    }

    tmp = s_getNumBits(bitstream);
	
    switch (config->bsSacExtType[config->sacExtCnt]) {
    case 0:
      config->bsResidualCoding = 1;

      config->bsResidualSamplingFreqIndex = s_GetBits(bitstream,4);
      config->bsResidualFramesPerSpatialFrame = s_GetBits(bitstream,2);

      for(i = 0; i < numOttBoxes+numTttBoxes; i++) {
        config->bsResidualPresent[i] = s_GetBits(bitstream, 1);
        if(config->bsResidualPresent[i]) {
          config->bsResidualBands[i] = s_GetBits(bitstream, 5);
        }
      }
      break;


    case 1:
      config->bsArbitraryDownmix = 2;

      config->bsArbitraryDownmixResidualSamplingFreqIndex = s_GetBits(bitstream,4);
      config->bsArbitraryDownmixResidualFramesPerSpatialFrame = s_GetBits(bitstream,2);
      config->bsArbitraryDownmixResidualBands = s_GetBits(bitstream, 5);

      break;


    case 2:
      config->arbitraryTree = 1;

      config->numOutChanAT  = 0;
      config->numOttBoxesAT = 0;
      for(ch=0; ch<numOutChan; ch++) {
        tmpOpen = 1;
        idx = 0;
        while(tmpOpen > 0) {
          config->bsOttBoxPresentAT[ch][idx] = s_GetBits(bitstream, 1);
          if (config->bsOttBoxPresentAT[ch][idx]) {
            config->numOttBoxesAT++;
            tmpOpen++;
          }
          else {
            config->numOutChanAT++;
            tmpOpen--;
          }
          idx++;
        }
      }

      for(i=0; i<config->numOttBoxesAT; i++) {
        config->bsOttDefaultCldAT[i] = s_GetBits(bitstream, 1);
        config->bsOttModeLfeAT[i] = s_GetBits(bitstream, 1);
        if (config->bsOttModeLfeAT[i]) {
          config->bsOttBandsAT[i] = s_GetBits(bitstream, 5);
        }
        else {
          config->bsOttBandsAT[i] = freqResTable[config->bsFreqRes];
        }
      }

      for(i=0; i<config->numOutChanAT; i++) {
        config->bsOutputChannelPosAT[i] = s_GetBits(bitstream, 5);
      }

      break;

    default:
      ;  
    }

    
    bitsRead = s_getNumBits(bitstream) - tmp;
    nFillBits = 8*sacExtLen - bitsRead;

    while(nFillBits > 7) {
      s_GetBits(bitstream, 8);
      nFillBits -= 8;
    }
    if(nFillBits > 0) {
      s_GetBits(bitstream, nFillBits);
    }

    ba -= 8*sacExtLen;
    config->sacExtCnt++;
  }

  *pnBitsRead = bitsAvailable - ba;

}


void GetUsacMps212SpecificConfig(spatialDec* self, int bsFrameLength, int bsResidualCoding, SAC_DEC_USAC_MPS212_CONFIG *pUsacMps212Config){

  SPATIAL_BS_CONFIG *config = &(self->bsConfig);
  int i, ottModeLfe[MAX_NUM_OTT];

  config->bsFrameLength = bsFrameLength;
  config->bsFreqRes = pUsacMps212Config->bsFreqRes;

  config->bsTreeConfig = 7;  /* only MPS212 in USAC  */

  if(config->bsTreeConfig != 15) {
    self->numOttBoxes = treePropertyTable[config->bsTreeConfig].numOttBoxes;
    self->numTttBoxes = treePropertyTable[config->bsTreeConfig].numTttBoxes;
    self->numInputChannels = treePropertyTable[config->bsTreeConfig].numInputChannels;
    self->numOutputChannels = treePropertyTable[config->bsTreeConfig].numOutputChannels;
    for(i = 0; i < MAX_NUM_OTT; i++) {
      ottModeLfe[i] = treePropertyTable[config->bsTreeConfig].ottModeLfe[i];
    }
  }
  
  config->bsQuantMode        = 0;
  config->bsOneIcc           = 0;
  config->bsArbitraryDownmix = 0;
  config->bsFixedGainSur     = 2;
  config->bsFixedGainLFE     = 1;
  config->bsFixedGainDMX     = pUsacMps212Config->bsFixedGainDMX;
  config->bsMatrixMode       = 0;


  config->bsTempShapeConfig    = pUsacMps212Config->bsTempShapeConfig;
  config->bsDecorrConfig       = pUsacMps212Config->bsDecorrConfig;

  config->bs3DaudioMode      = 0;
  
  config->bsHighRateMode = pUsacMps212Config->bsHighRateMode;
  config->bsPhaseCoding  = pUsacMps212Config->bsPhaseCoding;
  
  config->bsOttBandsPhasePresent = pUsacMps212Config->bsOttBandsPhasePresent;
  if(config->bsOttBandsPhasePresent){
    config->bsOttBandsPhase = pUsacMps212Config->bsOttBandsPhase;
  }
  
  config->bsResidualCoding = bsResidualCoding;
  if (config->bsResidualCoding) {
    config->bsResidualSamplingFreqIndex     = 0;
    config->bsResidualFramesPerSpatialFrame = 0;
    config->bsResidualPresent[0]            = 1;
    config->bsResidualBands[0]              = pUsacMps212Config->bsResidualBands;
    
    if (config->bsPhaseCoding) {
      config->bsPhaseCoding = 2;
    }
#ifdef RESIDUAL_CODING_WO_PHASE_NOFIX
    else {
      assert(0); /* undefined behavior */
    }
#endif
  }
  
  if (config->bsTempShapeConfig == 2) {
    config->bsEnvQuantMode = pUsacMps212Config->bsEnvQuantMode;
  }

  return;
}

void SpatialDecParseSpecificConfig(spatialDec* self, int sacHeaderLen, int *pnBitsRead, int bsFrameLength, int bsResidualCoding) {

  HANDLE_S_BITINPUT bitstream = self->bitstream;
  SPATIAL_BS_CONFIG *config = &(self->bsConfig);
  int i, hc, hb, numHeaderBits, ottModeLfe[MAX_NUM_OTT];

  int tmp = s_getNumBits(self->bitstream);
  int bitsAvailable = 8*sacHeaderLen;
  int nBitsReadExtConfig = 0;
  int bsPseudoLr;

  config->bsTreeConfig = 7;  /* only MPS212 in USAC  */

  if(config->bsTreeConfig != 7){
    config->bsFrameLength = s_GetBits(bitstream, 7);
  } else {
    config->bsFrameLength = bsFrameLength;
  }
  config->bsFreqRes = s_GetBits(bitstream, 3);

  if(config->bsTreeConfig != 15) {
    self->numOttBoxes = treePropertyTable[config->bsTreeConfig].numOttBoxes;
    self->numTttBoxes = treePropertyTable[config->bsTreeConfig].numTttBoxes;
    self->numInputChannels = treePropertyTable[config->bsTreeConfig].numInputChannels;
    self->numOutputChannels = treePropertyTable[config->bsTreeConfig].numOutputChannels;
    for(i = 0; i < MAX_NUM_OTT; i++) {
      ottModeLfe[i] = treePropertyTable[config->bsTreeConfig].ottModeLfe[i];
    }
  }
  
  if (config->bsTreeConfig == 7) {
    config->bsQuantMode        = 0;
    config->bsOneIcc           = 0;
    config->bsArbitraryDownmix = 0;
    config->bsFixedGainSur     = 2;
    config->bsFixedGainLFE     = 1;
    config->bsFixedGainDMX     = s_GetBits(bitstream, 3);
    config->bsMatrixMode       = 0;
  }
  else {
    config->bsQuantMode        = s_GetBits(bitstream, 2);
    config->bsOneIcc           = s_GetBits(bitstream, 1);
    config->bsArbitraryDownmix = s_GetBits(bitstream, 1);
    config->bsFixedGainSur     = s_GetBits(bitstream, 3);
    config->bsFixedGainLFE     = s_GetBits(bitstream, 3);
    config->bsFixedGainDMX     = s_GetBits(bitstream, 3);
    config->bsMatrixMode       = s_GetBits(bitstream, 1);    
  }

  config->bsTempShapeConfig    = s_GetBits(bitstream, 2);
  config->bsDecorrConfig       = s_GetBits(bitstream, 2);

  if(config->bsTreeConfig == 7){
    config->bs3DaudioMode      = 0;

    config->bsHighRateMode = s_GetBits(bitstream, 1);
    config->bsPhaseCoding = s_GetBits(bitstream, 1);

    config->bsOttBandsPhasePresent = s_GetBits(bitstream, 1);
    if(config->bsOttBandsPhasePresent){
      config->bsOttBandsPhase = s_GetBits(bitstream, 5);
    }

    config->bsResidualCoding = bsResidualCoding;

    if (config->bsResidualCoding) {
      config->bsResidualSamplingFreqIndex = 0;
      config->bsResidualFramesPerSpatialFrame = 0;
      config->bsResidualPresent[0] = 1;
      config->bsResidualBands[0] = s_GetBits(bitstream, 5);
      
      bsPseudoLr = s_GetBits(bitstream, 1);

      if (config->bsPhaseCoding) {
        config->bsPhaseCoding = 2;
      }
#ifdef RESIDUAL_CODING_WO_PHASE_NOFIX
      else {
        assert(0); /* undefined behavior */
      }
#endif
    }
  }
  else {
    config->bs3DaudioMode      = s_GetBits(bitstream, 1);

    for(i = 0; i < self->numOttBoxes; i++) {
      if(ottModeLfe[i]) {
        config->bsOttBands[i] = s_GetBits(bitstream, 5);
      }
    }

    for(i = 0; i < self->numTttBoxes; i++) {
      config->bsTttDualMode[i] = s_GetBits(bitstream, 1);
      config->bsTttModeLow[i] = s_GetBits(bitstream, 3);
      if(config->bsTttDualMode[i]) {
        config->bsTttModeHigh[i] = s_GetBits(bitstream, 3);
        config->bsTttBandsLow[i] = s_GetBits(bitstream, 5);
      }
    }
  }

  if (config->bsTempShapeConfig == 2) {
    config->bsEnvQuantMode = s_GetBits(bitstream, 1);
  }

  if (config->bsTreeConfig != 7) {
    if (config->bs3DaudioMode) {
      config->bs3DaudioHRTFset = s_GetBits(bitstream, 2);
      if (config->bs3DaudioHRTFset == 0) {
        config->bsHRTFfreqRes    = s_GetBits(bitstream, 3);
        config->bsHRTFnumChan    = s_GetBits(bitstream, 4);
        config->bsHRTFasymmetric = s_GetBits(bitstream, 1);

        config->HRTFnumBand = freqResTable[config->bsHRTFfreqRes];

        for(hc=0; hc<config->bsHRTFnumChan; hc++) {
          for(hb=0; hb<config->HRTFnumBand; hb++) {
            config->bsHRTFlevelLeft[hc][hb] = s_GetBits(bitstream, 6);
          }
          for(hb=0; hb<config->HRTFnumBand; hb++) {
            config->bsHRTFlevelRight[hc][hb] = config->bsHRTFasymmetric ?
              s_GetBits(bitstream, 6) : config->bsHRTFlevelLeft[hc][hb];
          }
          config->bsHRTFphase[hc] = s_GetBits(bitstream, 1);
          for(hb=0; hb<config->HRTFnumBand; hb++) {
            config->bsHRTFphaseLR[hc][hb] = config->bsHRTFphase[hc] ? s_GetBits(bitstream, 6) : 0;
          }
          config->bsHRTFicc[hc] = s_GetBits(bitstream, 1);
          for(hb=0; hb<config->HRTFnumBand; hb++) {
            config->bsHRTFiccLR[hc][hb] = config->bsHRTFicc[hc] ? s_GetBits(bitstream, 6) : 0;
          }
        }
      }
    }

    s_byteAlign(self->bitstream);
  }
  
  numHeaderBits = s_getNumBits(self->bitstream) - tmp;
  bitsAvailable -= numHeaderBits;

  if(config->bsTreeConfig == 7) {
    nBitsReadExtConfig = 0;
    *pnBitsRead = numHeaderBits;
  }
  else {
    SpatialDecParseExtensionConfig(bitstream,
                                 config,
                                 self->numOttBoxes,
                                 self->numTttBoxes,
                                 self->numOutputChannels,
                                 bitsAvailable, 
                                 &nBitsReadExtConfig);
    *pnBitsRead = 8*sacHeaderLen + bitsAvailable - nBitsReadExtConfig;
  }
} 




void SpatialDecDefaultSpecificConfig(spatialDec* self) {

  SPATIAL_BS_CONFIG *config = &(self->bsConfig);
  int i, ottModeLfe[MAX_NUM_OTT];

  config->bsFrameLength = 31;
  config->bsFreqRes = 1;
  config->bsTreeConfig = 2;
  assert(config->bsTreeConfig < 6);
  if(config->bsTreeConfig != 15) {
    self->numOttBoxes = treePropertyTable[config->bsTreeConfig].numOttBoxes;
    self->numTttBoxes = treePropertyTable[config->bsTreeConfig].numTttBoxes;
    self->numInputChannels = treePropertyTable[config->bsTreeConfig].numInputChannels;
    self->numOutputChannels = treePropertyTable[config->bsTreeConfig].numOutputChannels;
    for(i = 0; i < MAX_NUM_OTT; i++) {
      ottModeLfe[i] = treePropertyTable[config->bsTreeConfig].ottModeLfe[i];
    }
  }
  config->bsQuantMode = 0;
  config->bsOneIcc = 0;
  config->bsArbitraryDownmix = 0;
  config->bsResidualCoding = 0;
  config->bsSmoothConfig = 0;
  config->bsFixedGainSur = 2;
  config->bsFixedGainLFE = 1;
  config->bsFixedGainDMX = 0;
  config->bsMatrixMode = 1;
  config->bsTempShapeConfig = 0;
  config->bsDecorrConfig = 0;
  if(config->bsTreeConfig == 15) {
    assert(0);
  }
  for(i = 0; i < self->numOttBoxes; i++) {
    if(ottModeLfe[i]) {
      config->bsOttBands[i] = 28;
    }
  }
  for(i = 0; i < self->numTttBoxes; i++) {
    config->bsTttDualMode[i] = 0;
    config->bsTttModeLow[i] = 1;
    if(config->bsTttDualMode[i]) {
      config->bsTttModeHigh[i] = 1;
      config->bsTttBandsLow[i] = 28;
    }
  }
} 



void SpatialDecInitResidual(spatialDec* self) {
  if ((self->residualCoding != 0) || (self->arbitraryDownmix == 2)) {
    initResidualCore(self);
  }
}



static void
coarse2fine( int* data, DATA_TYPE dataType, int startBand, int numBands ) {

  int i;

  for( i=startBand; i<startBand+numBands; i++ ) {
    data[i] <<= 1;
  }

  if (dataType == t_CLD) {
    for( i=startBand; i<startBand+numBands; i++ ) {
      if(data[i] == -14)
        data[i] = -15;
      else if(data[i] == 14)
        data[i] = 15;
    }
  }
}


static void
fine2coarse( int* data, DATA_TYPE dataType, int startBand, int numBands ) {

  int i;

  for( i=startBand; i<startBand+numBands; i++ ) {
    if ( dataType == t_CLD )
      data[i] /= 2;
    else
      data[i] >>= 1;
  }

}



static int
getStrideMap(int freqResStride, int startBand, int stopBand, int* aStrides) {

  int i, pb, pbStride, dataBands, strOffset;

  pbStride = pbStrideTable[freqResStride];
  dataBands = (stopBand-startBand-1)/pbStride+1;

  aStrides[0] = startBand;
  for(pb=1; pb<=dataBands; pb++) {
    aStrides[pb] = aStrides[pb-1]+pbStride;
  }
  strOffset = 0;
  while(aStrides[dataBands] > stopBand) {
    if (strOffset < dataBands) strOffset++;
    else strOffset = 1;

    for(i=strOffset; i<=dataBands; i++) {
      aStrides[i]--;
    }
  }

  return dataBands;
}




static void ecDataDec(HANDLE_SPATIAL_DEC self,
                      HANDLE_S_BITINPUT    bitstream,
                      LOSSLESSDATA       *llData,
                      int data[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
                      int lastdata[MAX_NUM_OTT][MAX_PARAMETER_BANDS],
                      int datatype,
                      int boxIdx,
                      int paramIdx,
                      int startBand,
                      int stopBand,
                      int defaultValue)
{
  int i, j, pb, dataSets, setIdx, bsDataPair, dataBands, oldQuantCoarseXXX;
  int aStrides[MAX_PARAMETER_BANDS+1] = {0};

  SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);

  dataSets = 0;
  for(i = 0; i < self->numParameterSets; i++) {
    llData->bsXXXDataMode[paramIdx][i] = s_GetBits(bitstream, 2);

    if( (i==self->numParameterSets-1 && llData->bsXXXDataMode[paramIdx][i] == 2) ||
         (self->residualCoding && llData->bsXXXDataMode[paramIdx][i]==2) ||
         (i==0 && frame->bsIndependencyFlag && llData->bsXXXDataMode[paramIdx][i]!=0 && llData->bsXXXDataMode[paramIdx][i]!=3)) {
           CommonExit(1, "Invalid value of bsXXXdataMode in this context");
    }

    if (llData->bsXXXDataMode[paramIdx][i] == 3) {
      dataSets++;
    }
  }

  setIdx = 0;
  bsDataPair = 0;
  oldQuantCoarseXXX = llData->bsQuantCoarseXXXprev[paramIdx];

  for(i = 0; i < self->numParameterSets; i++) {
    if (llData->bsXXXDataMode[paramIdx][i] == 0) {
      for(pb = startBand; pb < stopBand; pb++ ) {
        lastdata[boxIdx][pb] = defaultValue;
      }
        
      oldQuantCoarseXXX = 0;
    }


    if (llData->bsXXXDataMode[paramIdx][i] == 3) {
      if (bsDataPair) {
        bsDataPair = 0;
      }
      else {
        bsDataPair = s_GetBits(bitstream, 1);
        llData->bsQuantCoarseXXX[paramIdx][setIdx] = s_GetBits(bitstream, 1);
        llData->bsFreqResStrideXXX[paramIdx][setIdx] = s_GetBits(bitstream, 2);

        if (llData->bsQuantCoarseXXX[paramIdx][setIdx] != oldQuantCoarseXXX) {
          if (oldQuantCoarseXXX) {
            coarse2fine( lastdata[boxIdx], datatype, startBand, stopBand-startBand );
          }
          else {
            fine2coarse( lastdata[boxIdx], datatype, startBand, stopBand-startBand );
          }
        }

        dataBands = getStrideMap(llData->bsFreqResStrideXXX[paramIdx][setIdx], startBand, stopBand, aStrides);

        for(pb=0; pb<dataBands; pb++) {
          lastdata[boxIdx][startBand+pb] = lastdata[boxIdx][aStrides[pb]];
        }

        EcDataPairDec(bitstream, data[boxIdx], lastdata[boxIdx], datatype, setIdx, startBand, dataBands, bsDataPair, 
                      llData->bsQuantCoarseXXX[paramIdx][setIdx], frame->bsIndependencyFlag && (i == 0) );

        for(pb=0; pb<dataBands; pb++) {
          for(j=aStrides[pb]; j<aStrides[pb+1]; j++) {
            if (datatype == t_IPD) {
              if (llData->bsQuantCoarseXXX[paramIdx][setIdx]) {
                lastdata[boxIdx][j] = data[boxIdx][setIdx+bsDataPair][startBand+pb]&7;
              } else {
                lastdata[boxIdx][j] = data[boxIdx][setIdx+bsDataPair][startBand+pb]&15;
              }
              if (lastdata[boxIdx][j]>32767) {
                fprintf(stderr,"\nerror in modulo");
                getchar();
              } 
            }
            else {
              lastdata[boxIdx][j] = data[boxIdx][setIdx+bsDataPair][startBand+pb];
            }
          }
        }

        oldQuantCoarseXXX = llData->bsQuantCoarseXXX[paramIdx][setIdx];

        if(bsDataPair) {
          llData->bsQuantCoarseXXX[paramIdx][setIdx+1] = llData->bsQuantCoarseXXX[paramIdx][setIdx];
          llData->bsFreqResStrideXXX[paramIdx][setIdx+1] = llData->bsFreqResStrideXXX[paramIdx][setIdx];
        }
        setIdx += bsDataPair+1;
      }
    }
  }
}



static void
parseArbitraryDownmixData(spatialDec* self) {

  SPATIAL_BS_FRAME *frame = &self->bsFrame[self->curFrameBs];
  HANDLE_S_BITINPUT bitstream = self->bitstream;
  int offset = self->numOttBoxes + 4*self->numTttBoxes;
  int ch;

  
  for (ch = 0; ch < self->numInputChannels; ch++) {
    ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpArbdmxGainIdx, frame->cmpArbdmxGainIdxPrev, t_CLD, ch, offset + ch, 0, self->bitstreamParameterBands,
              self->arbdmxGainDefault);
  }

} 




static int decodeIccDiffCode() {
  int value=0;
  int count=0;
  while((s_getbits(1)==0) && (count++ <7))
    value++;

  return value;
}



static void
parseResidualData(spatialDec* self) {

  int ich, ch;
  int rfpsf;
  int ps;
  int pb;
  int resReorder[] = { 0, 1, 2, 5, 3, 4 };

  SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);
  SPATIAL_BS_CONFIG *config = &(self->bsConfig);

  for (ch=0; ch<MAX_RESIDUAL_CHANNELS; ch++) {
    for(rfpsf=0; rfpsf <MAX_RESIDUAL_FRAMES; rfpsf++) {
      self->resMdctPresent[ch][rfpsf]=0;
    }
  }

  for (ich=0; ich<self->numOttBoxes + self->numTttBoxes; ich++) {
#ifdef RES_REORDER
    
    if (self->treeConfig == TREE_7271 || self->treeConfig == TREE_7272) {
      ch = resReorder[ich];
    }
    else {
      ch = ich;
    }
#else
    ch = ich;
#endif
    if (config->bsResidualBands[ch]>0) {
      if (ch < self->numOttBoxes) {
        for (ps=0; ps < self->numParameterSets; ps++) {
          frame->resData.bsIccDiffPresent[ch][ps] = s_getbits(1);
          if (frame->resData.bsIccDiffPresent[ch][ps]) {
            for (pb=0; pb<config->bsResidualBands[ch]; pb++) {
              frame->resData.bsIccDiff[ch][ps][pb] = decodeIccDiffCode();
              frame->ottICCdiffidx[ch][ps][pb]     = frame->resData.bsIccDiff[ch][ps][pb];
#ifdef VERIFICATION_TEST_COMPATIBILITY
              frame->ottICCdiffidxPrev[ch][pb] = frame->ottICCdiffidx[ch][ps][pb];
#endif
            }
          }
        }
      }
      for (rfpsf=0; rfpsf < self->residualFramesPerSpatialFrame; rfpsf++) {
        BLOCKTYPE bt = decodeIcs(self,ch, rfpsf,0);

        if ((bt == EIGHTSHORT_WINDOW) &&
            ((self->updQMF ==18) ||
             (self->updQMF ==24) ||
             (self->updQMF ==30))) {
          decodeIcs(self,ch, rfpsf,1);
        }
      }
    }
  }
} 




static void SpatialDecParseExtensionFrame(spatialDec* self) {

  int i, fr, gr, offset, ch, rfpsf;
  int extNum, sacExtType, sacExtLen, tmp, bitsRead, nFillBits;
  int channelGrouping[MAX_INPUT_CHANNELS];

  SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);

  for (ch=0; ch<MAX_RESIDUAL_CHANNELS; ch++) {
    for(rfpsf=0; rfpsf <MAX_RESIDUAL_FRAMES; rfpsf++) {
      self->resMdctPresent[ch][rfpsf]=0;
    }
  }

  for(extNum=0; extNum<self->bsConfig.sacExtCnt; extNum++) {

    sacExtType = self->bsConfig.bsSacExtType[extNum];

    if (sacExtType < 12) {
      sacExtLen = s_GetBits(self->bitstream, 8);
      if (sacExtLen == 255) {
        sacExtLen += s_GetBits(self->bitstream, 16);
      }

      tmp = s_getNumBits(self->bitstream);

      switch (sacExtType) {
      case 0:  
        parseResidualData(self);
        break;

      case 1:  
        switch (self->numInputChannels) {
        case 1:
          channelGrouping[0] = 1;
          break;
        case 2:
          channelGrouping[0] = 2;
          break;
        case 6:
          channelGrouping[0] = 2;
          channelGrouping[1] = 2;
          channelGrouping[2] = 2;
          break;
        default:
          assert(0);
          break;
        }

        offset = self->numOttBoxes + self->numTttBoxes;

        for (ch = 0, gr = 0; ch < self->numInputChannels; ch += channelGrouping[gr++]) {
          frame->bsArbitraryDownmixResidualAbs[ch] = s_GetBits(self->bitstream, 1);
          frame->bsArbitraryDownmixResidualAlphaUpdateSet[ch] = s_GetBits(self->bitstream, 1);

          if (channelGrouping[gr] == 1) {
            for (fr = 0; fr < self->arbdmxFramesPerSpatialFrame; fr++) {
              BLOCKTYPE bt = decodeIcs(self, offset + ch, fr, 0);

              if ((bt == EIGHTSHORT_WINDOW) &&
                  ((self->arbdmxUpdQMF ==18) ||
                   (self->arbdmxUpdQMF ==24) ||
                   (self->arbdmxUpdQMF ==30))) {
                decodeIcs(self, offset + ch, fr, 1);
              }
            }
          } else {
            frame->bsArbitraryDownmixResidualAbs[ch+1] = frame->bsArbitraryDownmixResidualAbs[ch];
            frame->bsArbitraryDownmixResidualAlphaUpdateSet[ch+1] = frame->bsArbitraryDownmixResidualAlphaUpdateSet[ch];

            for (fr = 0; fr < self->arbdmxFramesPerSpatialFrame; fr++) {
              BLOCKTYPE bt = decodeCpe(self, offset + ch, fr, 0);

              if ((bt == EIGHTSHORT_WINDOW) &&
                  ((self->arbdmxUpdQMF ==18) ||
                   (self->arbdmxUpdQMF ==24) ||
                   (self->arbdmxUpdQMF ==30))) {
                decodeCpe(self, offset + ch, fr, 1);
              }
            }
          }
        }

        break;

      case 2:  
        for(i=0; i<self->bsConfig.numOttBoxesAT; i++) {
          ecDataDec(self, self->bitstream, &frame->CLDLosslessData, frame->cmpOttCLDidx, frame->cmpOttCLDidxPrev, t_CLD, self->numOttBoxes+i, self->numOttBoxes+i, 0, self->bsConfig.bsOttBandsAT[i], self->bsConfig.bsOttDefaultCldAT[i]);
        }

        break;

      default:
        ;  
      }

      
      bitsRead = s_getNumBits(self->bitstream) - tmp;
      nFillBits = 8*sacExtLen - bitsRead;

      while(nFillBits > 7) {
        s_GetBits(self->bitstream, 8);
        nFillBits -= 8;
      }
      if(nFillBits > 0) {
        s_GetBits(self->bitstream, nFillBits);
      }
    }
  }

}


int SpatialDecResetBitstream(spatialDec* self, 
                             HANDLE_BYTE_READER bitstream){

	int error = 0;
	if(self->bitstream){
       s_bitinput_close(self->bitstream);
	   self->bitstream = NULL;
	}

    if(NULL == (self->bitstream = s_bitinput_open(bitstream))){
      error = -1;
	}

	return error;
}


void SpatialDecParseFrame(spatialDec* self, int *pnBitsRead, int bUsacIndependencyFlag) {

  int i, bsFramingType, dataBands, bsTempShapeEnable, numTempShapeChan;
  int tttOff, ps, pg, ts, pb;
  int bsEnvShapeData[MAX_TIME_SLOTS];
  int tmp;
#ifdef VERIFICATION_TEST_COMPATIBILITY
  int s, ps_ott;
  int aStrides[MAX_PARAMETER_BANDS+1];
#endif
  static int fcnt=0;



  SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);
  HANDLE_S_BITINPUT bitstream = self->bitstream;
  int bsNumOutputChannels = treePropertyTable[self->treeConfig].numOutputChannels;

  fcnt++;

  tmp = s_getNumBits(bitstream);

  if (self->parseNextBitstreamFrame == 0) return;

  self->numParameterSetsPrev = self->numParameterSets;
  
  if (self->bsHighRateMode) {
    bsFramingType = s_GetBits(bitstream, 1);
    self->numParameterSets = s_GetBits(bitstream, 3) + 1;
  }
  else {
    bsFramingType = 0;
    self->numParameterSets = 1;
  }

  for(i = 0; i < self->numParameterSets; i++) {
    if(bsFramingType) {
      int bitsParamSlot = 0;
      while((1<<bitsParamSlot) < self->timeSlots) bitsParamSlot++;
      self->paramSlot[i] = s_GetBits(bitstream, bitsParamSlot);
    } else {
      self->paramSlot[i] = (int)ceil((float)self->timeSlots * (float)(i+1) / (float)self->numParameterSets) - 1;
    }
  }

  if(!bUsacIndependencyFlag){
    frame->bsIndependencyFlag = s_GetBits(bitstream, 1);
  } else {
    frame->bsIndependencyFlag = 1;
  }
    
  for(i = 0; i < self->numOttBoxes; i++) {
    ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpOttCLDidx, frame->cmpOttCLDidxPrev, t_CLD, i, i, 0, self->bitstreamOttBands[i], self->ottCLDdefault[i]);
  }
#ifndef VERIFICATION_TEST_COMPATIBILITY
  if(self->oneIcc) {
    ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpOttICCidx, frame->cmpOttICCidxPrev, t_ICC, 0, 0, 0, self->bitstreamParameterBands, self->ICCdefault);
  } else {
    for(i = 0; i < self->numOttBoxes; i++) {
      if(!self->ottModeLfe[i]) {
        ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpOttICCidx, frame->cmpOttICCidxPrev, t_ICC, i, i, 0, self->bitstreamOttBands[i], self->ICCdefault);
      }
    }
  }
#else
  if(self->oneIcc) {
    ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpOttICCtempidx, frame->cmpOttICCtempidxPrev, t_ICC, 0, 0, 0, self->bitstreamParameterBands, self->ICCdefault);
  } else {
    for(i = 0; i < self->numOttBoxes; i++) {
      if(!self->ottModeLfe[i]) {
        ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpOttICCtempidx, frame->cmpOttICCtempidxPrev, t_ICC, i, i, 0, self->bitstreamOttBands[i], self->ICCdefault);
      }
    }
  }
#endif

  if (self->bsConfig.bsPhaseCoding) {
    self->bsPhaseMode = s_GetBits(bitstream, 1);
 
    if(!self->bsPhaseMode){
      for(pb=0; pb<self->numOttBandsIPD; pb++) {
        frame->cmpOttIPDidxPrev[0][pb] = 0;
        for(i = 0; i < self->numParameterSets; i++) {
          frame->cmpOttIPDidx[0][i][pb] = 0;
          self->bsFrame[self->curFrameBs].ottIPDidx[0][i][pb] = 0;
        }
        self->bsFrame[self->curFrameBs].ottIPDidxPrev[0][pb] = 0;
      }
      self->OpdSmoothingMode = 0;
    }
    else {
      self->OpdSmoothingMode = s_GetBits(bitstream, 1);
      ecDataDec(self, bitstream, &frame->IPDLosslessData, frame->cmpOttIPDidx, frame->cmpOttIPDidxPrev, t_IPD, 0, 0, 0, self->numOttBandsIPD, self->IPDdefault);
    }
  }
#ifndef RESIDUAL_CODING_WO_PHASE_NOFIX      
  else {
    self->bsPhaseMode = 0;
    for(pb=0; pb<self->numOttBandsIPD; pb++) {
      frame->cmpOttIPDidxPrev[0][pb] = 0;
      for(i = 0; i < self->numParameterSets; i++) {
        frame->cmpOttIPDidx[0][i][pb] = 0;
        self->bsFrame[self->curFrameBs].ottIPDidx[0][i][pb] = 0;
      }
      self->bsFrame[self->curFrameBs].ottIPDidxPrev[0][pb] = 0;
    }
    self->OpdSmoothingMode = 0;
  }
#endif /* RESIDUAL_CODING_WO_PHASE_NOFIX */
  
  tttOff = self->numOttBoxes;
  for(i = 0; i < self->numTttBoxes; i++) {
    if(self->tttConfig[0][i].mode < 2) {
      ecDataDec(self, bitstream, &frame->CPCLosslessData, frame->cmpTttCPC1idx, frame->cmpTttCPC1idxPrev, t_CPC, i, tttOff+4*i  , self->tttConfig[0][i].bitstreamStartBand, self->tttConfig[0][i].bitstreamStopBand, self->CPCdefault);
      ecDataDec(self, bitstream, &frame->CPCLosslessData, frame->cmpTttCPC2idx, frame->cmpTttCPC2idxPrev, t_CPC, i, tttOff+4*i+1, self->tttConfig[0][i].bitstreamStartBand, self->tttConfig[0][i].bitstreamStopBand, self->CPCdefault);
      ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpTttICCidx , frame->cmpTttICCidxPrev, t_ICC, i, tttOff+4*i  , self->tttConfig[0][i].bitstreamStartBand, self->tttConfig[0][i].bitstreamStopBand, self->ICCdefault);
    } else {
      ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpTttCLD1idx, frame->cmpTttCLD1idxPrev, t_CLD, i, tttOff+4*i, self->tttConfig[0][i].bitstreamStartBand, self->tttConfig[0][i].bitstreamStopBand, self->tttCLD1default[i]);
      ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpTttCLD2idx, frame->cmpTttCLD2idxPrev, t_CLD, i, tttOff+4*i+1, self->tttConfig[0][i].bitstreamStartBand, self->tttConfig[0][i].bitstreamStopBand, self->tttCLD2default[i]);
    }

    if(self->tttConfig[1][i].bitstreamStartBand < self->tttConfig[1][i].bitstreamStopBand) {
      if(self->tttConfig[1][i].mode < 2) {
        ecDataDec(self, bitstream, &frame->CPCLosslessData, frame->cmpTttCPC1idx, frame->cmpTttCPC1idxPrev, t_CPC, i, tttOff+4*i+2, self->tttConfig[1][i].bitstreamStartBand, self->tttConfig[1][i].bitstreamStopBand, self->CPCdefault);
        ecDataDec(self, bitstream, &frame->CPCLosslessData, frame->cmpTttCPC2idx, frame->cmpTttCPC2idxPrev, t_CPC, i, tttOff+4*i+3, self->tttConfig[1][i].bitstreamStartBand, self->tttConfig[1][i].bitstreamStopBand, self->CPCdefault);
        ecDataDec(self, bitstream, &frame->ICCLosslessData, frame->cmpTttICCidx, frame->cmpTttICCidxPrev , t_ICC, i, tttOff+4*i+2, self->tttConfig[1][i].bitstreamStartBand, self->tttConfig[1][i].bitstreamStopBand, self->ICCdefault);
      } else {
        ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpTttCLD1idx, frame->cmpTttCLD1idxPrev, t_CLD, i, tttOff+4*i+2, self->tttConfig[1][i].bitstreamStartBand, self->tttConfig[1][i].bitstreamStopBand, self->tttCLD1default[i]);
        ecDataDec(self, bitstream, &frame->CLDLosslessData, frame->cmpTttCLD2idx, frame->cmpTttCLD2idxPrev, t_CLD, i, tttOff+4*i+3, self->tttConfig[1][i].bitstreamStartBand, self->tttConfig[1][i].bitstreamStopBand, self->tttCLD2default[i]);
      }
    }
  }

  
  frame->bsSmoothControl = 1;  
  if (frame->bsSmoothControl) {
    if (self->bsHighRateMode){
      for (ps = 0; ps < self->numParameterSets; ps++) {
        frame->bsSmoothMode[ps] = s_GetBits(bitstream, 2);
        if (frame->bsSmoothMode[ps] >= 2) {
          frame->bsSmoothTime[ps] = s_GetBits(bitstream, 2);
        }
        if (frame->bsSmoothMode[ps] == 3) {
          frame->bsFreqResStrideSmg[ps] = s_GetBits(bitstream, 2);
          dataBands = (self->bitstreamParameterBands - 1) / pbStrideTable[frame->bsFreqResStrideSmg[ps]] + 1;
          for (pg = 0; pg < dataBands; pg++) {
            frame->bsSmgData[ps][pg] = s_GetBits(bitstream, 1);
          }
        }
      }
    }
    else{
	  for (ps = 0; ps < self->numParameterSets; ps++) {
        frame->bsSmoothMode[ps] = 0;
      }
    }
  }
  
  for(i = 0; i < bsNumOutputChannels; i++) {
    self->tempShapeEnableChannelSTP[i] = 0;
    self->tempShapeEnableChannelGES[i] = 0;
  }

  self->bsTsdEnable = 0;
  if (self->bsConfig.bsTempShapeConfig == 3) {
    self->bsTsdEnable = s_GetBits(bitstream, 1);
  }
  else if (self->bsConfig.bsTempShapeConfig != 0) {
    bsTempShapeEnable = s_GetBits(bitstream, 1);
    if(bsTempShapeEnable){
      numTempShapeChan = tempShapeChanTable[self->bsConfig.bsTempShapeConfig-1][self->bsConfig.bsTreeConfig];
      switch (self->tempShapeConfig) {
      case 1:
        for(i = 0; i < numTempShapeChan; i++) {
          self->tempShapeEnableChannelSTP[i] = s_GetBits(bitstream, 1);
        }
        break;
      case 2:
        for(i = 0; i < numTempShapeChan; i++) {
          self->tempShapeEnableChannelGES[i] = s_GetBits(bitstream, 1);
        }
        for (i = 0; i < numTempShapeChan; i++) {
          if (self->tempShapeEnableChannelGES[i]) {
            huff_dec_reshape(bitstream, bsEnvShapeData, self->timeSlots);
            for (ts = 0; ts < self->timeSlots; ts++) {
              self->envShapeData[i][ts] = (float)pow(2, (float)bsEnvShapeData[ts] / (self->envQuantMode+2) - 1);
            }
          }
        }
        break;
      default:
        assert(0);
      }
    }
  }

  if (self->upmixType == 2) {
    for(i = 0; i < bsNumOutputChannels; i++) {
      self->tempShapeEnableChannelSTP[i] = 0;
      self->tempShapeEnableChannelGES[i] = 0;
    }
  }

  
  /* read Tsd data */
  if(self->bsTsdEnable){
    unsigned short s[4];
    unsigned short c[5];
    unsigned short b;
    unsigned short r[1];  

    int k, h;

    /* get number of transient slots */
    if(self->timeSlots < 7)
      self->nBitsTrSlots = 1;
    else if(self->timeSlots < 13)
      self->nBitsTrSlots = 2;
    else if(self->timeSlots < 25)
      self->nBitsTrSlots = 3;
    else if(self->timeSlots < 49)
      self->nBitsTrSlots = 4;
    else
      self->nBitsTrSlots = 5;
    
    self->TsdNumTrSlots = s_GetBits(bitstream, self->nBitsTrSlots); 
    self->TsdNumTrSlots++;
   
    {  /* calculate code word length */   
      int N = self->timeSlots;  

      c[0] = 1;
      for(i=1; i<5 ; i++)
        c[i] = 0;
	 
      for (k=0; k<self->TsdNumTrSlots; k++) {
        b = N-k;
        longmult1(c,b,c,5);    /* c *= N-k; */
        b = k+1;
        longdiv(c,b,c,r,5);    /* c /= k+1; */
      }
       
      b = 1;
      longsub(c,&b,5,1);
      self->TsdCodewordLength = longcntbits(c,5) ; /* TsdCodewordLength = ceil(log(c)/log(2)); */ 
    }
 
    /* read code word */
    if (self->TsdCodewordLength>48){ 
      s[3] = s_GetBits(bitstream, self->TsdCodewordLength-48);
      s[2] = s_GetBits(bitstream, 16);
      s[1] = s_GetBits(bitstream, 16);
      s[0] = s_GetBits(bitstream, 16);     
    }
    else if (self->TsdCodewordLength>32){ 
      s[3] = 0;
#ifdef TSD_CW_NOFIX
      s[3] = s_GetBits(bitstream, self->TsdCodewordLength-32);
#else
      s[2] = s_GetBits(bitstream, self->TsdCodewordLength-32);
#endif
      s[1] = s_GetBits(bitstream, 16);
      s[0] = s_GetBits(bitstream, 16);     
    } 
    else if (self->TsdCodewordLength>16){ 
      s[3] = 0;
      s[2] = 0;
      s[1] = s_GetBits(bitstream, self->TsdCodewordLength-16);
      s[0] = s_GetBits(bitstream, 16);     
    } 
    else{
      s[3] = 0;
      s[2] = 0;
      s[1] = 0;          
      s[0] = s_GetBits(bitstream, self->TsdCodewordLength);
    }
    
    { /* decode transient slot position code word */
      int p = self->TsdNumTrSlots;
	 
      for(i=0; i<self->timeSlots ; i++) /* initialize */
        self->TsdSepData[i] = 0;

      /* go through all slots */
      for (k=self->timeSlots-1; k>=0; k--)
      {
        if (p > k) {
          for (;k>=0; k--)
            self->TsdSepData[k]=1;
          break;
        }
            
        c[0] = k-p+1;
        for(i=1; i<5 ; i++)
          c[i] = 0;
	    
        for (h=2; h<=p; h++) {
          b = k-p+h;
          longmult1(c,b,c,5);     /* c *= k - p + h; */
          b = h;
          longdiv(c,b,c,r,5);     /* c /= h; */
        }
	           
        if (longcompare(s,c,4)) {  /* (s >= (int)c) */
          longsub(s,c,4,4);      /* s -= c; */
          self->TsdSepData[k]=1;
          p--;
          if (p == 0)
            break;
        }
      }
    } 
 
    /* read phase data */
    for(i=0; i<self->timeSlots ; i++){   
      if(self->TsdSepData[i])
        self->bsTsdTrPhaseData[i] = s_GetBits(bitstream, 3);
    }

  } /* end of TSD frame */
  
  if (self->arbitraryDownmix != 0) {
    parseArbitraryDownmixData(self);
  }

  
  if (self->treeConfig != 7){
    s_byteAlign(self->bitstream);

    SpatialDecParseExtensionFrame(self);
  }

  

#ifndef VERIFICATION_TEST_COMPATIBILITY
  for(i=0; i<self->numOttBoxes; i++) {
    for(ps=0; ps<self->numParameterSets; ps++) {
      if(!frame->resData.bsIccDiffPresent[i][ps] || (self->upmixType == 2) || (self->upmixType == 3)) {
        for(pb=0; pb<self->bitstreamParameterBands; pb++) {
          self->bsFrame->ottICCdiffidx[i][ps][pb] = 0;
        }
      }

#ifdef PARTIALLY_COMPLEX
      if(frame->resData.bsIccDiffPresent[i][ps]) {
        for(pb=self->resBands[i]; pb<self->bitstreamParameterBands; pb++) {
          self->bsFrame->ottICCdiffidx[i][ps][pb] = 0;
        }
      }
#endif
    }
  }
#else
  for (i=0;i<self->numOttBoxes;i++) {
    ps_ott=0;
    for (ps=0;ps<self->numParameterSets;ps++) {
      if (frame->resData.bsIccDiffPresent[i][ps] && (self->upmixType != 2)) {
        dataBands = getStrideMap(frame->ICCLosslessData.bsFreqResStrideXXX[i][ps], 0, self->bitstreamParameterBands, aStrides);
        for (pb=0; pb<dataBands; pb++) {
          for (s=aStrides[pb]; s<aStrides[pb+1]; s++) {
            self->bsFrame->cmpOttICCidx[i][ps_ott][s] = self->bsFrame->cmpOttICCtempidx[i][ps_ott][pb];
          }
        }
        frame->ICCLosslessData.bsFreqResStrideXXX[i][ps] = 0;
        for (pb=0; pb<self->resBands[i]; pb++) {
          self->bsFrame->cmpOttICCidx[i][ps_ott][pb] += self->bsFrame->ottICCdiffidx[i][ps][pb];
          self->bsFrame->cmpOttICCidxPrev[i][pb] = self->bsFrame->cmpOttICCtempidxPrev[i][pb] + self->bsFrame->ottICCdiffidxPrev[i][pb];
        }
      }
      else {
        for (pb=0; pb<self->bitstreamParameterBands; pb++) {
          self->bsFrame->cmpOttICCidx[i][ps_ott][pb] = self->bsFrame->cmpOttICCtempidx[i][ps_ott][pb];
          self->bsFrame->cmpOttICCidxPrev[i][pb] = self->bsFrame->cmpOttICCtempidxPrev[i][pb];
        }
      }
      if (frame->ICCLosslessData.bsXXXDataMode[i][ps] == 3){
        ps_ott++;
      }
      for (pb=0; pb<self->bitstreamParameterBands; pb++) {
        self->bsFrame->ottICCdiffidx[i][ps][pb] = 0;
      }
    }
  }
#endif

  self->parseNextBitstreamFrame =0;

  if(pnBitsRead){
    *pnBitsRead = s_getNumBits(bitstream) - tmp;
  }

/*
  {
	  static int sumBits=0;
	  static int fcnt=0;
	  FILE *ptrBR = fopen("bitrate.txt","w");
	  
	  fcnt++;
	  sumBits += *pnBitsRead;

	  //fprintf(ptrBR,"%f\n",(float)sumBits/(float)fcnt/2048.*40000./1000.);
	  fprintf(ptrBR,"%f\n",(float)sumBits/(float)fcnt/2048.*34150./1000.);
	  fclose(ptrBR);
  }
*/
} 



static void
createMapping(int aMap[MAX_PARAMETER_BANDS+1],
              int startBand,
              int stopBand,
              int stride){

  int inBands, outBands , bandsAchived , bandsDiff , incr , k , i;
  int vDk[MAX_PARAMETER_BANDS+1];
  inBands     = stopBand-startBand;
  outBands    = (inBands-1)/stride+1;
  if( outBands < 1 ) {
    outBands = 1;
  }

  bandsAchived = outBands*stride;
  bandsDiff    = inBands - bandsAchived;
  for( i = 0; i < outBands; i++ ) {
    vDk[i] = stride;
  }

  if( bandsDiff > 0 ) {
    incr =-1;
    k=outBands-1;
  } else {
    incr =1;
    k=0;
  }

  while(bandsDiff != 0 ) {
    vDk[k] = vDk[k] - incr;
    k = k + incr;
    bandsDiff = bandsDiff + incr;
    if(k >= outBands) {
      if( bandsDiff > 0 ) {
           k=outBands-1;
      } else if ( bandsDiff < 0) {
           k=0;
      }
    }
  }
  aMap[0] = startBand;
  for( i = 0; i < outBands; i++ ) {
    aMap[i+1] = aMap[i] + vDk[i];
  }
} 



static void
mapFrequency( int  * pInput, 
              int  * pOutput,
              int  * pMap,     
              int  dataBands)  
{
  int i,j,startBand,stopBand, value;
  int startBand0 = pMap[0];

  for(i = 0; i < dataBands; i++) {
    value = pInput[i+startBand0];

    startBand = pMap[i];
    stopBand  = pMap[i+1];
    for(j = startBand; j < stopBand; j++) {
      pOutput[j] = value;
    }
  }
}

static float
deq( int value,
    int paramType)
{
  switch( paramType )
  {
  case t_CLD:
    return dequantCLD[value + 15];

  case t_ICC:
    return dequantICC[value];

  case t_CPC:
    return dequantCPC[value + 20];

  case t_IPD:
	assert((value%16)<16);
    return dequantIPD[(value&15)];

  default:
    assert(0);
    return 0.0;
  }
}

static float
factorFunct(float ottVsTotDb,
            int quantMode)
{
  float dbDiff;
  float xLinear = 0;
  float addFactor = 0;
  float xFactor = 0;
  float maxFactor = 0;
  float dy;
  float factor;

  assert(ottVsTotDb<=0);
  dbDiff = -ottVsTotDb;

  switch (quantMode) {
  case 0:
    return (1);
    break;
  case 1:
    xLinear = 1;
    addFactor = 5;
    xFactor = 21;
    maxFactor = 5;
    break;
  case 2:
    xLinear = 1;
    addFactor = 8;
    xFactor = 25;
    maxFactor = 8;
    break;
  default:
    assert(0);
    break;
  }

  dy = (addFactor - 1)/(xFactor - xLinear);
  if (dbDiff>xLinear) {
    factor = (dbDiff - xLinear) * dy + 1;
  }
  else {
    factor = 1;
  }
  factor = min(maxFactor,factor);
  return (factor);
}



static void
factorCLD(int *idx,
          float ottVsTotDb,
          float *ottVsTotDb1,
          float *ottVsTotDb2,
          int quantMode)
{
  float dbe = 10/(float)log(10);
  float factor;
  float cld;
  float nrg;
  float c1;
  float c2;
  int cldIdx;

  factor = factorFunct(ottVsTotDb,quantMode);

  cldIdx = (int)(*idx * factor + 15.5);
  cldIdx -= 15;

  cldIdx = min(cldIdx, 15);
  cldIdx = max(cldIdx, -15);

  *idx = cldIdx;

  cld = deq(*idx,t_CLD);
  nrg = (float)exp(ottVsTotDb/dbe);
  c1  = (float)exp(cld/dbe)/(1+(float)exp(cld/dbe));
  c2  = 1/(1+(float)exp(cld/dbe));
  (*ottVsTotDb1) = dbe* (float)log(c1*nrg);
  (*ottVsTotDb2) = dbe* (float)log(c2*nrg);
}



static void
mapIndexData(LOSSLESSDATA *llData,
             float  outputData[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             int    outputIdxData[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             int    cmpIdxData[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             int    diffIdxData[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             int    xttIdx,
             int    idxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS],
             int    paramIdx,
             int    paramType,
             int    startBand,
             int    stopBand,
             int    defaultValue,
             int    numParameterSets,
             int    *paramSlot,
             int    extendFrame,
             int    quantMode,
             float  ottVsTotDbIn[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             float  ottVsTotDb1[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
             float  ottVsTotDb2[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]
             )
{

  int aParamSlots[MAX_PARAMETER_SETS];
  int aInterpolate[MAX_PARAMETER_SETS];

  int dataSets;
  int aMap[MAX_PARAMETER_BANDS+1];

  int setIdx,i, band, parmSlot;
  int dataBands, stride;
  int ps,pb;

  int i1, i2, x1, xi, x2;


  dataSets = 0;
  for( i = 0 ; i < numParameterSets; i++ ) {
    if(llData->bsXXXDataMode[paramIdx][i] == 3) {
      aParamSlots[dataSets] = i;
      dataSets++;
    }
  }

  setIdx = 0;

  for( i = 0 ; i < numParameterSets; i++ ) {

    if(llData->bsXXXDataMode[paramIdx][i] == 0) {
      llData->nocmpQuantCoarseXXX[paramIdx][i] = 0;
      for(band = startBand ; band < stopBand; band++) {
        outputIdxData[xttIdx][i][band] = defaultValue;
      }
      for(band = startBand ; band < stopBand; band++) {
        idxPrev[xttIdx][band] = outputIdxData[xttIdx][i][band];
      }

      /* Because the idxPrev are also set to the defaultValue -> signalize fine */
      llData->bsQuantCoarseXXXprev[paramIdx] = 0;
    }

    if(llData->bsXXXDataMode[paramIdx][i] == 1) {
      for(band = startBand ; band < stopBand; band++) {
        outputIdxData[xttIdx][i][band] = idxPrev[xttIdx][band];
      }
      llData->nocmpQuantCoarseXXX[paramIdx][i] = llData->bsQuantCoarseXXXprev[paramIdx];
    }

    if(llData->bsXXXDataMode[paramIdx][i] == 2) {
      for(band = startBand ; band < stopBand; band++) {
        outputIdxData[xttIdx][i][band] = idxPrev[xttIdx][band];
      }
      llData->nocmpQuantCoarseXXX[paramIdx][i] = llData->bsQuantCoarseXXXprev[paramIdx];
      aInterpolate[i] = 1;
    } else {
      aInterpolate[i] = 0;
    }

    if(llData->bsXXXDataMode[paramIdx][i] == 3) {
      parmSlot    = aParamSlots[setIdx];
      stride      = pbStrideTable[llData->bsFreqResStrideXXX[paramIdx][setIdx]];
      dataBands   = (stopBand - startBand - 1)/stride + 1;
      createMapping(aMap, startBand, stopBand, stride);
      mapFrequency( &cmpIdxData[xttIdx][setIdx][0],
                    &outputIdxData[xttIdx][parmSlot][0],
                    aMap,
                    dataBands);

      for(band = startBand ; band < stopBand; band++) {
        idxPrev[xttIdx][band] = outputIdxData[xttIdx][parmSlot][band];
      }

      llData->bsQuantCoarseXXXprev[paramIdx] = llData->bsQuantCoarseXXX[paramIdx][setIdx];
      llData->nocmpQuantCoarseXXX[paramIdx][i] = llData->bsQuantCoarseXXX[paramIdx][setIdx];

      setIdx++;
    }

    if(diffIdxData != NULL) {
      for(band=startBand; band<stopBand; band++) {
        outputIdxData[xttIdx][i][band] += diffIdxData[xttIdx][i][band];
      }
    }
  }

  for( i = 0 ; i < numParameterSets; i++ ) {
    if(llData->nocmpQuantCoarseXXX[paramIdx][i] == 1) {
      /* Map all coarse data to fine */
      coarse2fine(outputIdxData[xttIdx][i], paramType, startBand, stopBand-startBand);
      llData->nocmpQuantCoarseXXX[paramIdx][i] = 0;
    }
  }  

  i1 = -1;
  x1 = 0;
  i2 = 0;
  for( i = 0 ; i < numParameterSets; i++ ) {

    if(aInterpolate[i] != 1) {
      i1 = i;
    }
    i2 = i;
    while(aInterpolate[i2] == 1){
      i2++;
    }
    if(i1==-1){
	x1 =0;
        i1 =0;
	}else{
    x1 = paramSlot[i1];
	}
    xi = paramSlot[i];
    x2 = paramSlot[i2];

    if( aInterpolate[i] == 1 ) {
      assert(i2 < numParameterSets);
      for(band = startBand ; band < stopBand; band++) {
        int yi,y1,y2;
        y1 = outputIdxData[xttIdx][i1][band];
        y2 = outputIdxData[xttIdx][i2][band];
        if (paramType == t_IPD) {
          if (y2 - y1 > 8) y1 += 16;
          if (y1 - y2 > 8) y2 += 16;
        
          yi = (y1 + (xi-x1)*(y2-y1)/(x2-x1)) % 16;
        }
        else {
          yi = y1 + (xi-x1)*(y2-y1)/(x2-x1);
        }
        outputIdxData[xttIdx][i][band] = yi;
      }
    }
  } 

  for( ps = 0 ; ps < numParameterSets; ps++ ) {
    if (quantMode && (paramType == t_CLD)) {
      assert(ottVsTotDbIn);
      assert(ottVsTotDb1);
      assert(ottVsTotDb2);

      for(pb = startBand ; pb < stopBand; pb++) {
        factorCLD(&(outputIdxData[xttIdx][ps][pb]),
                  ottVsTotDbIn[ps][pb],
                  &(ottVsTotDb1[ps][pb]),
                  &(ottVsTotDb2[ps][pb]),
                  quantMode);
      }
	}
      
    for(band = startBand ; band < stopBand; band++) 
      outputData[xttIdx][ps][band] = deq(outputIdxData[xttIdx][ps][band],paramType);
  }

  if(extendFrame){

    if (paramType == t_IPD) {
      llData->bsQuantCoarseXXX[paramIdx][numParameterSets] = llData->bsQuantCoarseXXX[paramIdx][numParameterSets - 1];
    }

    for(band = startBand ; band < stopBand; band++) {
      outputData[xttIdx][numParameterSets][band] = outputData[xttIdx][numParameterSets - 1][band];
    }
  }

}



static int *
getParametersMapping(int bsParameterBands) {

  int *mapping = NULL;

  switch (bsParameterBands) {
  case 4:
    mapping = mapping_4_to_28;
    break;
  case 5:
    mapping = mapping_5_to_28;
    break;
  case 7:
    mapping = mapping_7_to_28;
    break;
  case 10:
    mapping = mapping_10_to_28;
    break;
  case 14:
    mapping = mapping_14_to_28;
    break;
  case 20:
    mapping = mapping_20_to_28;
    break;
  }

  return mapping;
}



static int
mapNumberOfBandsTo28Bands(int bands, int bsParameterBands) {

  int *mapping = getParametersMapping(bsParameterBands);
  int bands28 = bands;
  int pb;

  if (mapping != NULL) {
    for (pb = 0; pb < MAX_PARAMETER_BANDS; pb++) {
      if (mapping[pb] == bands) {
        break;
      }
    }

    bands28 = pb;
  }

  return bands28;
}



static void
mapFloatDataTo28Bands(float* data, int bsParameterBands) {

  int *mapping = getParametersMapping(bsParameterBands);
  int pb;

  if (mapping != NULL) {
    for (pb = MAX_PARAMETER_BANDS - 1; pb >= 0; pb--) {
      data[pb] = data[mapping[pb]];
    }
  }
}



static void
decodeAndMapFrameOtt(HANDLE_SPATIAL_DEC self) {
  SPATIAL_BS_FRAME *pCurBs = &(self->bsFrame[self->curFrameBs]);

  int i,numParameterSets,ottIdx, band;
  int numOttBoxes;

  int ps,pb;

  float totDb[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_fc[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_s[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_f[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_c[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_lr[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_l[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float ottVsTotDb_r[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tmp1[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tmp2[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];

  pCurBs      = &self->bsFrame[self->curFrameBs];
  numOttBoxes = self->numOttBoxes;

  for (ps=0; ps<self->numParameterSets; ps++) {
    for (pb=0; pb<self->bitstreamParameterBands; pb++) {
      totDb[ps][pb] = 0;
    }
  }
  switch (self->treeConfig) {
  case TREE_5151:
    assert(numOttBoxes==5);
    i = 0;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 totDb,
                 ottVsTotDb_fc,
                 ottVsTotDb_s);
    i = 1;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_fc,
                 ottVsTotDb_f,
                 ottVsTotDb_c);
    i = 2;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_s,
                 tmp1,
                 tmp2);
    i = 3;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_f,
                 tmp1,
                 tmp2);
    i = 4;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 totDb,
                 tmp1,
                 tmp2);
    break;

  case TREE_5152:
    i = 0;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 totDb,
                 ottVsTotDb_lr,
                 ottVsTotDb_c);
    i = 1;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_lr,
                 ottVsTotDb_l,
                 ottVsTotDb_r);
    i = 2;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 totDb,
                 tmp1,
                 tmp2);
    i = 3;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_l,
                 tmp1,
                 tmp2);
    i = 4;
    mapIndexData(&pCurBs->CLDLosslessData,   
                 self->     ottCLD,
                 pCurBs->   ottCLDidx,       
                 pCurBs->cmpOttCLDidx,       
                 NULL,                       
                 i,                          
                 pCurBs->   ottCLDidxPrev,   
                 i,
                 t_CLD,
                 0,                          
                 self->bitstreamOttBands[i], 
                 self->ottCLDdefault[i],     
                 self->numParameterSets,     
                 self->paramSlot,
                 self->extendFrame,
                 self->quantMode,
                 ottVsTotDb_r,
                 tmp1,
                 tmp2);
    break;

  default:
    if (self->treeConfig != TREE_525) {
      assert(self->quantMode==0);
    }
    for(i = 0; i < numOttBoxes ; i++ ) {
      mapIndexData(&pCurBs->CLDLosslessData,   
                   self->     ottCLD,
                   pCurBs->   ottCLDidx,       
                   pCurBs->cmpOttCLDidx,       
                   NULL,                       
                   i,                          
                   pCurBs->   ottCLDidxPrev,   
                   i,
                   t_CLD,
                   0,                          
                   self->bitstreamOttBands[i], 
                   self->ottCLDdefault[i],     
                   self->numParameterSets,     
                   self->paramSlot,
                   self->extendFrame,
                   (self->treeConfig == TREE_525) ? 0 : self->quantMode,
                   NULL,
                   NULL,
                   NULL);
    } 
    break;
  } 


  
  if(  self->oneIcc == 1 ) {

    
    if( self->extendFrame == 0 ) {
      numParameterSets = self->numParameterSets;
    } else {
      numParameterSets = self->numParameterSets + 1;
    }

    for(ottIdx = 1; ottIdx < numOttBoxes ; ottIdx++ ) {
      
      if (self->ottModeLfe[ottIdx] == 0) {
        for( i = 0 ; i < numParameterSets; i++ ) {
          for(band = 0 ; band < self->bitstreamParameterBands; band++) {
            pCurBs->cmpOttICCidx[ottIdx][i][band] = pCurBs->cmpOttICCidx[0][i][band];
          }
        }
      }
    }

    for(ottIdx = 0; ottIdx < numOttBoxes ; ottIdx++ ) {
      
      if( self->ottModeLfe[ottIdx] == 0) {
        mapIndexData(&pCurBs->ICCLosslessData,      
                     self->     ottICC,
                     pCurBs->   ottICCidx,          
                     pCurBs->cmpOttICCidx,          
                     pCurBs->ottICCdiffidx,         
                     ottIdx,                        
                     pCurBs->   ottICCidxPrev,      
                     0,                             
                     t_ICC,
                     0,                             
                     self->bitstreamOttBands[ottIdx],
                     self->ICCdefault,              
                     self->numParameterSets,        
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);
      }
    }

  } else {
    for(ottIdx = 0; ottIdx < numOttBoxes ; ottIdx++ ) {
      
      if( self->ottModeLfe[ottIdx] == 0) {
        mapIndexData(&pCurBs->ICCLosslessData,   
                     self->     ottICC,
                     pCurBs->   ottICCidx,       
                     pCurBs->cmpOttICCidx,       
                     pCurBs->ottICCdiffidx,      
                     ottIdx,                          
                     pCurBs->   ottICCidxPrev,   
                     ottIdx,
                     t_ICC,
                     0,                          
                     self->bitstreamOttBands[ottIdx], 
                     self->ICCdefault,           
                     self->numParameterSets,     
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

      }
    }
  }

  if((self->treeConfig == TREE_212)&&(self->bsConfig.bsPhaseCoding))
    for(ottIdx = 0; ottIdx < numOttBoxes ; ottIdx++ ) {
		  
		  mapIndexData(&pCurBs->IPDLosslessData,      
			  self->     ottIPD,
			  pCurBs->   ottIPDidx,          
			  pCurBs->cmpOttIPDidx,          
			  NULL,
			  ottIdx,                        
			  pCurBs->   ottIPDidxPrev,      
			  ottIdx,                             
			  t_IPD,
			  0,                             
			  self->numOttBandsIPD,
			  self->IPDdefault,              
			  self->numParameterSets,        
			  self->paramSlot,
			  self->extendFrame,
			  self->quantMode,
			  NULL,
			  NULL,
			  NULL);
	}

  if (self->upmixType == 2) {
    int numParameterSets = self->numParameterSets;

    if (self->extendFrame) {
      numParameterSets++;
    }

    for(ottIdx = 0; ottIdx < self->numOttBoxes; ottIdx++) {
      for (ps = 0; ps < numParameterSets; ps++) {
        mapFloatDataTo28Bands(self->ottCLD[ottIdx][ps], self->bitstreamParameterBands);
        mapFloatDataTo28Bands(self->ottICC[ottIdx][ps], self->bitstreamParameterBands);
      }
    }
  }
} 



static void
decodeAndMapFrameTtt(HANDLE_SPATIAL_DEC self) {
  SPATIAL_BS_FRAME *pCurBs = &(self->bsFrame[self->curFrameBs]);

  int numBands;

  int i, j, offset;
  int numTttBoxes;
  

  pCurBs     = &self->bsFrame[self->curFrameBs];
  numBands   = self->bitstreamParameterBands;
  offset     = self->numOttBoxes;
  numTttBoxes= self->numTttBoxes;


  
  for(i = 0; i < numTttBoxes ; i++ ) {
    for(j = 0; (j < 2) && self->tttConfig[j][i].startBand < self->tttConfig[j][i].stopBand; j++ ) {
      if( self->tttConfig[j][i].mode < 2 ) {
        
        mapIndexData(&pCurBs->CPCLosslessData,                 
                     self->     tttCPC1,
                     pCurBs->   tttCPC1idx,                    
                     pCurBs->cmpTttCPC1idx,                    
                     NULL,                                     
                     i,                                        
                     pCurBs->   tttCPC1idxPrev,                
                     offset + 4 * i + 2 * j,
                     t_CPC,
                     self->tttConfig[j][i].bitstreamStartBand, 
                     self->tttConfig[j][i].bitstreamStopBand,  
                     self->CPCdefault,                         
                     self->numParameterSets,                   
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

        
        mapIndexData(&pCurBs->CPCLosslessData,                 
                     self->     tttCPC2,
                     pCurBs->   tttCPC2idx,                    
                     pCurBs->cmpTttCPC2idx,                    
                     NULL,                                     
                     i,                                        
                     pCurBs->   tttCPC2idxPrev,                
                     offset + 4 * i + 1 + 2 * j,
                     t_CPC,
                     self->tttConfig[j][i].bitstreamStartBand, 
                     self->tttConfig[j][i].bitstreamStopBand,  
                     self->CPCdefault,                         
                     self->numParameterSets,                   
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

        
        mapIndexData(&pCurBs->ICCLosslessData,                 
                     self->     tttICC,
                     pCurBs->   tttICCidx,                     
                     pCurBs->cmpTttICCidx,                     
                     NULL,                                     
                     i,                                        
                     pCurBs->   tttICCidxPrev,                 
                     offset + 4 * i + 2 * j ,
                     t_ICC,
                     self->tttConfig[j][i].bitstreamStartBand, 
                     self->tttConfig[j][i].bitstreamStopBand,  
                     self->ICCdefault,                         
                     self->numParameterSets,                   
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

      } else {
        
        mapIndexData(&pCurBs->CLDLosslessData,                 
                     self->     tttCLD1,
                     pCurBs->   tttCLD1idx,                    
                     pCurBs->cmpTttCLD1idx,                    
                     NULL,                                     
                     i,                                        
                     pCurBs->   tttCLD1idxPrev,                
                     offset + 4 * i + 2 * j,
                     t_CLD,
                     self->tttConfig[j][i].bitstreamStartBand, 
                     self->tttConfig[j][i].bitstreamStopBand,  
                     self->tttCLD1default[i],                  
                     self->numParameterSets,                   
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

        
        mapIndexData(&pCurBs->CLDLosslessData,                 
                     self->     tttCLD2,
                     pCurBs->   tttCLD2idx,                    
                     pCurBs->cmpTttCLD2idx,                    
                     NULL,                                     
                     i,                                        
                     pCurBs->   tttCLD2idxPrev,                
                     offset + 4 * i + 1 + 2 * j,
                     t_CLD,
                     self->tttConfig[j][i].bitstreamStartBand, 
                     self->tttConfig[j][i].bitstreamStopBand,  
                     self->tttCLD2default[i],                  
                     self->numParameterSets,                   
                     self->paramSlot,
                     self->extendFrame,
                     self->quantMode,
                     NULL,
                     NULL,
                     NULL);

      }
    }

    if (self->upmixType == 2) {
      int numParameterSets = self->numParameterSets;
      int ps;

      if (self->extendFrame) {
        numParameterSets++;
      }

      for (ps = 0; ps < numParameterSets; ps++) {
        mapFloatDataTo28Bands(self->tttCPC1[i][ps], self->bitstreamParameterBands);
        mapFloatDataTo28Bands(self->tttCPC2[i][ps], self->bitstreamParameterBands);
        mapFloatDataTo28Bands(self->tttCLD1[i][ps], self->bitstreamParameterBands);
        mapFloatDataTo28Bands(self->tttCLD2[i][ps], self->bitstreamParameterBands);
        mapFloatDataTo28Bands(self->tttICC[i][ps], self->bitstreamParameterBands);
      }
    }
  }
}



static void
decodeAndMapFrameSmg(HANDLE_SPATIAL_DEC self)
{
    int ps, pb, pg, pbStride, dataBands, pbStart, pbStop, aGroupToBand[MAX_PARAMETER_BANDS+1];
    SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);

    self->smoothControl = frame->bsSmoothControl;

    if (self->smoothControl) {
        for (ps = 0; ps < self->numParameterSets; ps++) {
            switch (frame->bsSmoothMode[ps]) {
            case 0: 
                self->smgTime[ps] = 256;
                for (pb = 0; pb < self->bitstreamParameterBands; pb++) {
                    self->smgData[ps][pb] = 0;
                }
                break;

            case 1: 
                if (ps > 0)
                    self->smgTime[ps] = self->smgTime[ps-1];
                else
                    self->smgTime[ps] = self->smoothState.prevSmgTime;

                for (pb = 0; pb < self->bitstreamParameterBands; pb++) {
                    if (ps > 0)
                        self->smgData[ps][pb] = self->smgData[ps-1][pb];
                    else
                        self->smgData[ps][pb] = self->smoothState.prevSmgData[pb];
                }
                break;

            case 2: 
                self->smgTime[ps] = smgTimeTable[frame->bsSmoothTime[ps]];
                for (pb = 0; pb < self->bitstreamParameterBands; pb++) {
                    self->smgData[ps][pb] = 1;
                }
                break;

            case 3: 
                self->smgTime[ps] = smgTimeTable[frame->bsSmoothTime[ps]];
                pbStride = pbStrideTable[frame->bsFreqResStrideSmg[ps]];
                dataBands = (self->bitstreamParameterBands - 1) / pbStride + 1;
                createMapping(aGroupToBand, 0, self->bitstreamParameterBands, pbStride);
                for (pg = 0; pg < dataBands; pg++) {
                    pbStart = aGroupToBand[pg];
                    pbStop  = aGroupToBand[pg+1];
                    for (pb = pbStart; pb < pbStop; pb++) {
                        self->smgData[ps][pb] = frame->bsSmgData[ps][pg];
                    }
                }
                break;
            }
        }

        
        self->smoothState.prevSmgTime = self->smgTime[self->numParameterSets - 1];
        for (pb = 0; pb < self->bitstreamParameterBands; pb++) {
            self->smoothState.prevSmgData[pb] = self->smgData[self->numParameterSets - 1][pb];
        }

        if (self->extendFrame) {
            self->smgTime[self->numParameterSets] = self->smgTime[self->numParameterSets-1];
            for (pb = 0; pb < self->bitstreamParameterBands; pb++) {
                self->smgData[self->numParameterSets][pb] = self->smgData[self->numParameterSets-1][pb];
            }
        }

        
        if (self->upmixType == 2) {
            int *mapping = getParametersMapping(self->bitstreamParameterBands);

            if (mapping != NULL) {
                int numParameterSets = self->numParameterSets;

                if (self->extendFrame) {
                    numParameterSets++;
                }

                for (ps = 0; ps < numParameterSets; ps++) {
                    for (pb = MAX_PARAMETER_BANDS - 1; pb >= 0; pb--) {
                        self->smgData[ps][pb] = self->smgData[ps][mapping[pb]];
                    }
                }
            }
        }
    }
}



static void
decodeAndMapFrameArbdmx(HANDLE_SPATIAL_DEC self) {

  SPATIAL_BS_FRAME *frame = &self->bsFrame[self->curFrameBs];
  int offset = self->numOttBoxes + 4 * self->numTttBoxes;
  int ch;

  for (ch = 0; ch < self->numInputChannels; ch++ ) {
    mapIndexData(&frame->CLDLosslessData,       
                 self->    arbdmxGain,          
                 frame->   arbdmxGainIdx,       
                 frame->cmpArbdmxGainIdx,       
                 NULL,                          
                 ch,                            
                 frame->   arbdmxGainIdxPrev,   
                 offset + ch,                   
                 t_CLD,
                 0,                             
                 self->bitstreamParameterBands, 
                 self->arbdmxGainDefault,       
                 self->numParameterSets,        
                 self->paramSlot,
                 self->extendFrame,
                 0,
                 NULL,
                 NULL,
                 NULL);

    self->arbdmxResidualAbs[ch] = frame->bsArbitraryDownmixResidualAbs[ch];
    self->arbdmxAlphaUpdSet[ch] = frame->bsArbitraryDownmixResidualAlphaUpdateSet[ch];

    if (self->upmixType == 2) {
      int numParameterSets = self->numParameterSets;
      int ps;

      if (self->extendFrame) {
        numParameterSets++;
      }

      for (ps = 0; ps < numParameterSets; ps++) {
        mapFloatDataTo28Bands(self->arbdmxGain[ch][ps], self->bitstreamParameterBands);
      }
    }
  }
} 



static void
decodeAndMapFrameArbTree(HANDLE_SPATIAL_DEC self) {

  SPATIAL_BS_FRAME  *pCurBs  = &(self->bsFrame[self->curFrameBs]);
  SPATIAL_BS_CONFIG *pConfig = &(self->bsConfig);
  int offset = self->numOttBoxes;

  int i;

  for(i = 0; i < pConfig->numOttBoxesAT; i++ ) {
    mapIndexData(&pCurBs->CLDLosslessData,      
                 self->ottCLD,                  
                 pCurBs->ottCLDidx,             
                 pCurBs->cmpOttCLDidx,          
                 NULL,                          
                 offset+i,                      
                 pCurBs->ottCLDidxPrev,         
                 offset+i,                      
                 t_CLD,                         
                 0,                             
                 pConfig->bsOttBandsAT[i],      
                 pConfig->bsOttDefaultCldAT[i], 
                 self->numParameterSets,        
                 self->paramSlot,               
                 self->extendFrame,             
                 self->quantMode,               
                 NULL,                          
                 NULL,                          
                 NULL);                         
  }

} 



void SpatialDecDecodeFrame(spatialDec* self) {

  if (self->parseNextBitstreamFrame == 1) return;

  self->extendFrame = 0;
  if(self->paramSlot[self->numParameterSets-1] != self->timeSlots-1){
    self->extendFrame = 1;
  }

  
  decodeAndMapFrameOtt(self);
  decodeAndMapFrameTtt(self);

  decodeAndMapFrameSmg(self);

  if (self->bsConfig.arbitraryTree != 0) {
    decodeAndMapFrameArbTree(self);
  }

  if (self->arbitraryDownmix != 0) {
    decodeAndMapFrameArbdmx(self);
  }

  if(self->extendFrame){
    self->numParameterSets++;
    self->paramSlot[self->numParameterSets-1] = self->timeSlots-1;
  }
} 




void SpatialDecDecodeHeader(spatialDec* self) {

  int i, ch, pb;
  SPATIAL_BS_CONFIG *config = &(self->bsConfig);

  self->timeSlots   = config->bsFrameLength+1;
  self->frameLength = self->timeSlots * self->qmfBands;
  self->bitstreamParameterBands = freqResTable[config->bsFreqRes];

  self->hybridBands       = SacGetHybridSubbands(self->qmfBands);
  self->tp_hybBandBorder  = 12;

  if (self->upmixType == 2) {
    self->numParameterBands = MAX_PARAMETER_BANDS;
  }
  else {
    self->numParameterBands = self->bitstreamParameterBands;
  }

  switch(self->numParameterBands){
  case 4:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_4_to_71[i][0];
      self->kernels[i][1] = kernels_4_to_71[i][1];
    }
    break;
  case 5:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_5_to_71[i][0];
      self->kernels[i][1] = kernels_5_to_71[i][1];
    }
    break;
  case 7:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_7_to_71[i][0];
      self->kernels[i][1] = kernels_7_to_71[i][1];
    }
    break;
  case 10:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_10_to_71[i][0];
      self->kernels[i][1] = kernels_10_to_71[i][1];
    }
    break;
  case 14:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_14_to_71[i][0];
      self->kernels[i][1] = kernels_14_to_71[i][1];
    }
    break;
  case 20:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_20_to_71[i][0];
      self->kernels[i][1] = kernels_20_to_71[i][1];
    }
    break;
  case 28:
    for(i=0; i < self->hybridBands; i++){
      self->kernels[i][0] = kernels_28_to_71[i][0];
      self->kernels[i][1] = kernels_28_to_71[i][1];
    }
    break;
  default:
    CommonExit(1,"unsupported numParameterBands\n");
    break;
  };

  self->treeConfig = config->bsTreeConfig;
  
  self->numOttBoxes = treePropertyTable[self->treeConfig].numOttBoxes;
  self->numTttBoxes = treePropertyTable[self->treeConfig].numTttBoxes;
  self->numInputChannels = treePropertyTable[self->treeConfig].numInputChannels;
  self->numOutputChannels = treePropertyTable[self->treeConfig].numOutputChannels;
  self->quantMode = config->bsQuantMode;
  self->oneIcc = config->bsOneIcc;
  self->arbitraryDownmix = config->bsArbitraryDownmix;
  self->residualCoding = config->bsResidualCoding;
  self->smoothConfig = config->bsSmoothConfig;
  self->surroundGain = surroundGainTable[config->bsFixedGainSur];
  self->lfeGain = lfeGainTable[config->bsFixedGainLFE];
  self->clipProtectGain = clipGainTable[config->bsFixedGainDMX];
  self->mtxInversion = config->bsMatrixMode;
  self->tempShapeConfig = config->bsTempShapeConfig;
  self->decorrConfig = config->bsDecorrConfig;
  self->envQuantMode = config->bsEnvQuantMode;

  if (self->upmixType == 2) {
    self->numOutputChannels = 2;
    self->decorrConfig = 0;
  }

  if (self->upmixType == 3) {
    self->numOutputChannels = 2;
  }


  if(self->bsConfig.arbitraryTree == 1)
    self->numOutputChannelsAT = self->bsConfig.numOutChanAT;
  else
    self->numOutputChannelsAT = self->numOutputChannels;

  self->_3DStereoInversion = config->bs3DaudioMode;

  for(i = 0; i < self->numOttBoxes; i++) {

    if (treePropertyTable[self->treeConfig].ottModeLfe[i]) {
      self->bitstreamOttBands[i] = config->bsOttBands[i];
      self->ottModeLfe[i] = 1;
    } else {
      self->bitstreamOttBands[i] = self->bitstreamParameterBands;
      self->ottModeLfe[i] = 0;
    }

    if (self->upmixType == 2) {
      self->numOttBands[i] = mapNumberOfBandsTo28Bands(self->bitstreamOttBands[i], self->bitstreamParameterBands);
    }
    else {
      self->numOttBands[i] = self->bitstreamOttBands[i];

      if(config->bsOttBandsPhasePresent){
         self->numOttBandsIPD = config->bsOttBandsPhase;
      } else {
        switch(self->numParameterBands){
        case 4:
        case 5:
          self->numOttBandsIPD = 2;
          break;
        case 7:
          self->numOttBandsIPD = 3;
          break;
        case 10:
          self->numOttBandsIPD = 5;
          break;
        case 14:
          self->numOttBandsIPD = 7;
          break;
        case 20:
        case 28:
          self->numOttBandsIPD = 10;
          break;
        default:
          assert(0);
          break;
        }

        if (config->bsResidualCoding && (self->numOttBandsIPD < config->bsResidualBands[i])) {
          self->numOttBandsIPD = config->bsResidualBands[i];
        }
      } 
    }
  }
  for(i = 0; i < self->numTttBoxes; i++) {
    self->tttConfig[0][i].mode = config->bsTttModeLow[i];
    self->tttConfig[1][i].mode = config->bsTttModeHigh[i];
    self->tttConfig[0][i].bitstreamStartBand = 0;
    self->tttConfig[1][i].bitstreamStopBand = self->bitstreamParameterBands;

    if(config->bsTttDualMode[i]) {
      self->tttConfig[0][i].bitstreamStopBand = config->bsTttBandsLow[i];
      self->tttConfig[1][i].bitstreamStartBand = config->bsTttBandsLow[i];
    } else {
      self->tttConfig[0][i].bitstreamStopBand = self->bitstreamParameterBands;
      self->tttConfig[1][i].bitstreamStartBand = self->bitstreamParameterBands;
    }

    if (self->upmixType == 2) {
      self->tttConfig[0][i].startBand = mapNumberOfBandsTo28Bands(self->tttConfig[0][i].bitstreamStartBand, self->bitstreamParameterBands);
      self->tttConfig[0][i].stopBand  = mapNumberOfBandsTo28Bands(self->tttConfig[0][i].bitstreamStopBand , self->bitstreamParameterBands);
      self->tttConfig[1][i].startBand = mapNumberOfBandsTo28Bands(self->tttConfig[1][i].bitstreamStartBand, self->bitstreamParameterBands);
      self->tttConfig[1][i].stopBand  = mapNumberOfBandsTo28Bands(self->tttConfig[1][i].bitstreamStopBand , self->bitstreamParameterBands);
    }
    else {
      self->tttConfig[0][i].startBand = self->tttConfig[0][i].bitstreamStartBand;
      self->tttConfig[0][i].stopBand  = self->tttConfig[0][i].bitstreamStopBand;
      self->tttConfig[1][i].startBand = self->tttConfig[1][i].bitstreamStartBand;
      self->tttConfig[1][i].stopBand  = self->tttConfig[1][i].bitstreamStopBand;
    }
  }
  self->residualCoding = config->bsResidualCoding;
  self->numResidualSignals = 0;
  if(self->residualCoding) {
    for(i = 0; i < self->numTttBoxes + self->numOttBoxes; i++) {
      if(config->bsResidualPresent[i]) {
        self->resBands[i] = config->bsResidualBands[i];
#ifdef PARTIALLY_COMPLEX
        if (self->bitstreamParameterBands == MAX_PARAMETER_BANDS) {
          self->resBands[i] = min(self->resBands[i], MAX_LP_RESIDUAL_BANDS_28);
        }
        else {
          self->resBands[i] = min(self->resBands[i], getParametersMapping(self->bitstreamParameterBands)[MAX_LP_RESIDUAL_BANDS_28]);
        }
#endif
        self->numResidualSignals++;
      } else {
        self->resBands[i] = 0;
      }

      if (self->upmixType == 2 || self->upmixType == 3) {
        self->resBands[i] = 0;
      }
    }
  }

  self->residualFramesPerSpatialFrame = self->bsConfig.bsResidualFramesPerSpatialFrame+1;
  if(self->residualFramesPerSpatialFrame > 0) {
    self->updQMF = (int) (self->bsConfig.bsFrameLength+1)/(self->bsConfig.bsResidualFramesPerSpatialFrame+1);
  }

  self->arbdmxResidualBands = config->bsArbitraryDownmixResidualBands;
#ifdef PARTIALLY_COMPLEX
  if (self->bitstreamParameterBands == MAX_PARAMETER_BANDS) {
    self->arbdmxResidualBands = min(self->arbdmxResidualBands, MAX_LP_RESIDUAL_BANDS_28);
  }
  else {
    self->arbdmxResidualBands = min(self->arbdmxResidualBands, getParametersMapping(self->bitstreamParameterBands)[MAX_LP_RESIDUAL_BANDS_28]);
  }
#endif
  self->arbdmxFramesPerSpatialFrame = config->bsArbitraryDownmixResidualFramesPerSpatialFrame+1;
  if (self->arbdmxFramesPerSpatialFrame > 0) {
    self->arbdmxUpdQMF = self->timeSlots / self->arbdmxFramesPerSpatialFrame;
  }

  self->CPCdefault = 10;
  self->tttCLD1default[0] = 15;
  self->tttCLD2default[0] = 0;
  self->ICCdefault = 0;
  self->arbdmxGainDefault = 0;

  self->IPDdefault = 0;

  if(self->_3DStereoInversion) {
    if(config->bs3DaudioHRTFset == 0) {
      self->_3DStereoData.hrtfParameterBands = freqResTable[config->bsHRTFfreqRes];
      self->_3DStereoData.hrtfNumChannels    = config->bsHRTFnumChan;
      for(ch = 0; ch < self->_3DStereoData.hrtfNumChannels; ch++) {
        HRTF_DIRECTIONAL_DATA *hrtf = &self->_3DStereoData.directional[ch];
        for(pb = 0; pb < self->_3DStereoData.hrtfParameterBands; pb++) {
          hrtf->powerL[pb] = (float) pow(10.f, (config->bsHRTFlevelLeft [ch][pb]-16)/20.f);
          hrtf->powerR[pb] = (float) pow(10.f, (config->bsHRTFlevelRight[ch][pb]-16)/20.f);
          hrtf->ipd   [pb] = (float) config->bsHRTFphaseLR[ch][pb]*((float)C_PI/16.f);
          hrtf->icc   [pb] = dequantICC[config->bsHRTFiccLR[ch][pb]];
        }
      }
    }
    else {
      assert(0);
    }
  }

  switch (self->treeConfig) {
  case TREE_212:
    self->numDirektSignals = 1;
    self->numDecorSignals = 1;
    if (self->upmixType == 2) {
      self->numDecorSignals = 1;
    }
    if (self->upmixType == 3) {
      self->numDecorSignals = 1;
    }

    self->numXChannels = 1;
    if (self->arbitraryDownmix == 2) {
      self->numXChannels += 1;
    }
    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
	self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 0;
    self->ottCLDdefault[0] = 0;
    break;	
  case TREE_5151:
    self->numDirektSignals = 1;
    self->numDecorSignals = 4;

    if (self->upmixType == 2) {
      self->numDecorSignals = 1;
    }

    if (self->upmixType == 3) {
      self->numDecorSignals = 3;
    }

    self->numXChannels = 1;
    if (self->arbitraryDownmix == 2) {
      self->numXChannels += 1;
    }
    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
    self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 0;
    self->ottCLDdefault[0] = 15;
    self->ottCLDdefault[1] = 15;
    self->ottCLDdefault[2] = 0;
    self->ottCLDdefault[3] = 0;
    self->ottCLDdefault[4] = 15;
    break;
  case TREE_5152:
    self->numDirektSignals = 1;
    self->numDecorSignals = 4;

    if (self->upmixType == 2) {
      self->numDecorSignals = 1;
    }

    if (self->upmixType == 3) {
      self->numDecorSignals = 2;
    }

    self->numXChannels = 1;
    if (self->arbitraryDownmix == 2) {
      self->numXChannels += 1;
    }
    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
    self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 0;
    self->ottCLDdefault[0] = 15;
    self->ottCLDdefault[1] = 0;
    self->ottCLDdefault[2] = 15;
    self->ottCLDdefault[3] = 15;
    self->ottCLDdefault[4] = 15;
    break;
  case TREE_525:
    self->numDirektSignals = 3;

    for (i=0; i<2; i++){
      switch (self->tttConfig[i][0].mode) {
        case 0:
#ifdef PARTIALLY_COMPLEX
          self->tttConfig[i][0].useTTTdecorr = 0;
#else
          self->tttConfig[i][0].useTTTdecorr = 1;
#endif
          self->numDecorSignals = 3;
          break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          self->tttConfig[i][0].useTTTdecorr = 0;
          self->numDecorSignals = 2;
          break;
        default:
          assert( self->bsConfig.bsTttModeLow[0] > 1 );
          break;
      }
    }

    if (self->residualCoding == 1) {
      self->numXChannels = 3;
    } else {
      self->numXChannels = 2;
    }

    if (self->arbitraryDownmix == 2) {
      self->numXChannels = 5;
    }

    if (self->upmixType == 2) {
      self->numDirektSignals = 2;
      self->numDecorSignals = 0;
      self->numXChannels = 2;

      if (self->arbitraryDownmix == 2)
      {
        self->numDirektSignals = 4;
        self->numXChannels = 5;
      }
    }

    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
    self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 1;
    self->ottCLDdefault[0] = 15;
    self->ottCLDdefault[1] = 15;
    self->ottCLDdefault[2] = 15;
    break;
  case TREE_7271:
  case TREE_7272:
    self->numDirektSignals = 3;

    for (i = 0; i < 2; i++) {
      switch (self->tttConfig[i][0].mode) {
        case 0:
#ifdef PARTIALLY_COMPLEX
          self->tttConfig[i][0].useTTTdecorr = 0;
#else
          self->tttConfig[i][0].useTTTdecorr = 1;
#endif
          self->numDecorSignals = 5;
          break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          self->tttConfig[i][0].useTTTdecorr = 0;
          self->numDecorSignals = 5;
          break;
        default:
          assert( self->bsConfig.bsTttModeLow[0] > 1 );
          break;
      }
    }

    if ( self->residualCoding == 1 ) {
      self->numXChannels = 3;
    } else {
      self->numXChannels = 2;
    }

    if (self->arbitraryDownmix == 2) {
      self->numXChannels = 5;
    }

    if (self->upmixType == 2) {
      self->numDirektSignals = 2;
      self->numDecorSignals = 0;
      self->numXChannels = 2;

      if (self->arbitraryDownmix == 2)
      {
        self->numDirektSignals = 4;
        self->numXChannels = 5;
      }
    }

    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
    self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 1;
    self->ottCLDdefault[0] = 15;
    self->ottCLDdefault[1] = 15;
    self->ottCLDdefault[2] = 15;
    self->ottCLDdefault[3] = 15;
    self->ottCLDdefault[4] = 15;
    break;
  case TREE_7571:
  case TREE_7572:
    self->numDirektSignals = 6;
    self->numDecorSignals = 2;
    self->numXChannels = 6;
    self->numVChannels = self->numDirektSignals+self->numDecorSignals;
    self->numWChannels = self->numVChannels;
    self->wStartResidualIdx = 0;
    self->ottCLDdefault[0] = 15;
    self->ottCLDdefault[1] = 15;
    break;
  default:
    CommonExit(1,"bad treeConfig\n");
    break;
  }

  if(self->treeConfig == 7){
    self->bsHighRateMode = config->bsHighRateMode;
  }
}



void SpatialGetDequantTables(float **cld, float **icc, float **cpc) {
  *cld = dequantCLD;
  *icc = dequantICC;
  *cpc = dequantCPC;
}

float SpatialDecQuantizeCLD(float v) {
  int i = 1;
  float vmin = dequantCLD[0]; 
  double dmin = fabs(v - vmin);

  do {  
    if (fabs(v - dequantCLD[i]) < dmin) {
      dmin = fabs(v - dequantCLD[i]);
      vmin = dequantCLD[i];
    }
  } while (dequantCLD[i++] < 149.0f);

  return vmin;

}

float SpatialDecQuantizeICC(float v) {
  int i = 1;
  float vmin = dequantICC[0];
  double dmin = fabs(v - vmin);

  do {  
    if (fabs(v - dequantICC[i]) < dmin) {
      dmin = fabs(v - dequantICC[i]);
      vmin = dequantICC[i];
    }
  } while (dequantICC[i++] > -0.98f);

  return vmin;

}

