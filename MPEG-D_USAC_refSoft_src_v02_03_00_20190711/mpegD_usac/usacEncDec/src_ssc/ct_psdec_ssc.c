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
Copyright (c) ISO/IEC 2003.

 $Id: ct_psdec_ssc.c,v 1.1.1.1 2009-05-29 14:10:15 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  parametric stereo decoder $Revision: 1.1.1.1 $
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "SSC_System.h"
#include "Log.h"
#include "ct_psdec_ssc.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


/* undefine the following to enable the "unrestricted" version of the PS tool */
/* #define BASELINE_PS */
#undef BASELINE_PS


typedef const char (*Huffman)[2];

static const double IPD_STEP_SIZE = (double)(SSC_PI/NO_IPD_STEPS);

/* FIX_BORDER can have 0, 1, 2, 4 envelops */
static const int aFixNoEnvDecode[4] = {0, 1, 2, 4};


/* IID & ICC Huffman codebooks */
static const char aBookPsIidTimeDecode[28][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    { -66,   4 },
  { -62,   5 },    { -67,   6 },    { -61,   7 },    { -68,   8 },
  { -60,   9 },    { -69,  10 },    { -59,  11 },    { -70,  12 },
  { -58,  13 },    { -57,  14 },    { -71,  15 },    {  16,  17 },
  { -56, -72 },    {  18,  21 },    {  19,  20 },    { -55, -78 },
  { -77, -76 },    {  22,  25 },    {  23,  24 },    { -75, -74 },
  { -73, -54 },    {  26,  27 },    { -53, -52 },    { -51, -50 }
};

static const char aBookPsIidFreqDecode[28][2] = {
  { -64,   1 },    {   2,   3 },    { -63, -65 },    {   4,   5 },
  { -62, -66 },    {   6,   7 },    { -61, -67 },    {   8,   9 },
  { -68, -60 },    { -59,  10 },    { -69,  11 },    { -58,  12 },
  { -70,  13 },    { -71,  14 },    { -57,  15 },    {  16,  17 },
  { -56, -72 },    {  18,  19 },    { -55, -54 },    {  20,  21 },
  { -73, -53 },    {  22,  24 },    { -74,  23 },    { -75, -78 },
  {  25,  26 },    { -77, -76 },    { -52,  27 },    { -51, -50 }
};

static const char aBookPsIccTimeDecode[14][2] = {
  { -64,   1 },    { -63,   2 },    { -65,   3 },    { -62,   4 },
  { -66,   5 },    { -61,   6 },    { -67,   7 },    { -60,   8 },
  { -68,   9 },    { -59,  10 },    { -69,  11 },    { -58,  12 },
  { -70,  13 },    { -71, -57 }
};

static const char aBookPsIccFreqDecode[14][2] = {
  { -64,   1 },    { -63,   2 },    { -65,   3 },    { -62,   4 },
  { -66,   5 },    { -61,   6 },    { -67,   7 },    { -60,   8 },
  { -59,   9 },    { -68,  10 },    { -58,  11 },    { -69,  12 },
  { -57,  13 },    { -70, -71 }
};

/* IID-fine/IPD/OPD Huffman codebooks */

static const char aBookPsIidFineTimeDecode[60][2] = {
  {   1, -64 },    { -63,   2 },    {   3, -65 },    {   4,  59 },
  {   5,   7 },    {   6, -67 },    { -68, -60 },    { -61,   8 },
  {   9,  11 },    { -59,  10 },    { -70, -58 },    {  12,  41 },
  {  13,  20 },    {  14, -71 },    { -55,  15 },    { -53,  16 },
  {  17, -77 },    {  18,  19 },    { -85, -84 },    { -46, -45 },
  { -57,  21 },    {  22,  40 },    {  23,  29 },    { -51,  24 },
  {  25,  26 },    { -83, -82 },    {  27,  28 },    { -90, -38 },
  { -92, -91 },    {  30,  37 },    {  31,  34 },    {  32,  33 },
  { -35, -34 },    { -37, -36 },    {  35,  36 },    { -94, -93 },
  { -89, -39 },    {  38, -79 },    {  39, -81 },    { -88, -40 },
  { -74, -54 },    {  42, -69 },    {  43,  44 },    { -72, -56 },
  {  45,  52 },    {  46,  50 },    {  47, -76 },    { -49,  48 },
  { -47,  49 },    { -87, -41 },    { -52,  51 },    { -78, -50 },
  {  53, -73 },    {  54, -75 },    {  55,  57 },    {  56, -80 },
  { -86, -42 },    { -48,  58 },    { -44, -43 },    { -66, -62 }
};

static const char aBookPsIidFineFreqDecode[60][2] = {
  {   1, -64 },    {   2,   4 },    {   3, -65 },    { -66, -62 },
  { -63,   5 },    {   6,   7 },    { -67, -61 },    {   8,   9 },
  { -68, -60 },    {  10,  11 },    { -69, -59 },    {  12,  13 },
  { -70, -58 },    {  14,  18 },    { -57,  15 },    {  16, -72 },
  { -54,  17 },    { -75, -53 },    {  19,  37 },    { -56,  20 },
  {  21, -73 },    {  22,  29 },    {  23, -76 },    {  24, -78 },
  {  25,  28 },    {  26,  27 },    { -85, -43 },    { -83, -45 },
  { -81, -47 },    { -52,  30 },    { -50,  31 },    {  32, -79 },
  {  33,  34 },    { -82, -46 },    {  35,  36 },    { -90, -89 },
  { -92, -91 },    {  38, -71 },    { -55,  39 },    {  40, -74 },
  {  41,  50 },    {  42, -77 },    { -49,  43 },    {  44,  47 },
  {  45,  46 },    { -86, -42 },    { -88, -87 },    {  48,  49 },
  { -39, -38 },    { -41, -40 },    { -51,  51 },    {  52,  59 },
  {  53,  56 },    {  54,  55 },    { -35, -34 },    { -37, -36 },
  {  57,  58 },    { -94, -93 },    { -84, -44 },    { -80, -48 }
};

static const char aBookPsIpdTimeDecode[7][2] = {
  {   1, -64 },    {   2,   6 },    {   3,   5 },    { -59,   4 },
  { -60, -61 },    { -62, -58 },    { -63, -57 }
};

static const char aBookPsIpdFreqDecode[7][2] = {
  {   1, -64 },    {   2,   4 },    { -63,   3 },    { -60, -59 },
  {   5,   6 },    { -61, -58 },    { -62, -57 }
};

static const char aBookPsOpdTimeDecode[7][2] = {
  {   1, -64 },    {   2,   6 },    {   3,   4 },    { -59, -62 },
  { -58,   5 },    { -60, -61 },    { -63, -57 }
};

static const char aBookPsOpdFreqDecode[7][2] = {
  {   1, -64 },    {   2,   3 },    { -57, -63 },    {   4,   5 },
  { -61, -58 },    { -62,   6 },    { -59, -60 }
};

static const double p8_13_20[13] =
{
  0.00746082949812,  0.02270420949825,  0.04546865930473,  0.07266113929591,
  0.09885108575264,  0.11793710567217,  0.125,             0.11793710567217,
  0.09885108575264,  0.07266113929591,  0.04546865930473,  0.02270420949825,
  0.00746082949812
};


static const double p2_13_20[13] =
{
  0.0,               0.01899487526049,  0.0,              -0.07293139167538,
  0.0,               0.30596630545168,  0.5,               0.30596630545168,
  0.0,              -0.07293139167538,  0.0,               0.01899487526049,
  0.0
};

static const double p12_13_34[13] =
{
  0.04081179924692,  0.03812810994926,  0.05144908135699,  0.06399831151592,
  0.07428313801106,  0.08100347892914,  0.08333333333333,  0.08100347892914,
  0.07428313801106,  0.06399831151592,  0.05144908135699,  0.03812810994926,
  0.04081179924692
};

static const double p8_13_34[13] =
{
  0.01565675600122,  0.03752716391991,  0.05417891378782,  0.08417044116767,
  0.10307344158036,  0.12222452249753,  0.12500000000000,  0.12222452249753,
  0.10307344158036,  0.08417044116767,  0.05417891378782,  0.03752716391991,
  0.01565675600122
};

static const double p4_13_34[13] =
{
 -0.05908211155639, -0.04871498374946,  0.0,               0.07778723915851,
  0.16486303567403,  0.23279856662996,  0.25,              0.23279856662996,
  0.16486303567403,  0.07778723915851,  0.0,              -0.04871498374946,
 -0.05908211155639
};

static int psTablesInitialized = 0;

static double aAllpassLinkDecaySer[NO_SERIAL_ALLPASS_LINKS];

static const int   aAllpassLinkDelaySer[] = {    3,     4,     5};
static const double aAllpassLinkFractDelay = 0.39f;
static const double aAllpassLinkFractDelaySer[] = {0.43f, 0.75f, 0.347f};

static int delayIndexQmf[NO_QMF_CHANNELS] = {
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
};

static const int groupBorders20[NO_IID_GROUPS + 1] =
{
     6,  7,  0,  1,  2,  3, /* 6 subqmf subbands - 0th qmf subband */
     9,  8,                 /* 2 subqmf subbands - 1st qmf subband */
    10, 11,                 /* 2 subqmf subbands - 2nd qmf subband */
     3,  4,  5,  6,  7,  8,  9,  11, 14, 18, 23, 35, 64
};

static const int groupBorders34[NO_IID_GROUPS_HI_RES + 1] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, /* 12 subqmf subbands - 0th qmf subband */
     12, 13, 14, 15, 16, 17, 18, 19,                 /*  8 subqmf subbands - 1st qmf subband */
     20, 21, 22, 23,                                 /*  4 subqmf subbands - 2nd qmf subband */
     24, 25, 26, 27,                                 /*  4 subqmf subbands - 3nd qmf subband */
     28, 29, 30, 31,                                 /*  4 subqmf subbands - 4nd qmf subband */
     32-27, 33-27, 34-27, 35-27, 36-27, 37-27, 38-27, 40-27, 42-27, 44-27, 46-27, 48-27, 51-27, 54-27, 57-27, 60-27, 64-27, 68-27, 91-27
};

static const int bins2groupMap20[NO_IID_GROUPS] =
{
  ( NEGATE_IPD_MASK | 1 ), ( NEGATE_IPD_MASK | 0 ),
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
};

static const int bins2groupMap34[NO_IID_GROUPS_HI_RES] =
{
  0,  1,  2,  3,  4,  5,  6,  6,  7, ( NEGATE_IPD_MASK | 2 ), ( NEGATE_IPD_MASK | 1 ), ( NEGATE_IPD_MASK | 0 ),
  10, 10, 4,  5,  6,  7,  8,  9,
  10, 11, 12, 9,
  14, 11, 12, 13,
  14, 15, 16, 13,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};

static const int quantizedIIDs[NO_IID_STEPS] =
{
    2, 4, 7, 10, 14, 18, 25
};
static const int quantizedIIDsFine[NO_IID_STEPS_FINE] =
{
    2, 4, 6, 8, 10, 13, 16, 19, 22, 25, 30, 35, 40, 45, 50
};

/* precalculated scale factors for whole iid range */
static double scaleFactors[NO_IID_LEVELS];
static double scaleFactorsFine[NO_IID_LEVELS_FINE];

static double quantizedRHOs[NO_ICC_STEPS] =
{
    1.0f, 0.937f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.589f, -1.0f
};

/* precalculated values of alpha for every rho */
static double alphas[NO_ICC_LEVELS];


const int aNoIidBins[3] = {NO_LOW_RES_IID_BINS,
                           NO_MID_RES_IID_BINS,
                           NO_HI_RES_IID_BINS};

const int aNoIccBins[3] = {NO_LOW_RES_ICC_BINS,
                           NO_MID_RES_ICC_BINS,
                           NO_HI_RES_ICC_BINS};

const int aNoIpdBins[3] = {NO_LOW_RES_IPD_BINS,
                           NO_MID_RES_IPD_BINS,
                           NO_HI_RES_IPD_BINS};

static void **callocMatrix(int rows, int cols, size_t size);
static void freeMatrix(void **aaMatrix, int rows);

static void deCorrelate( HANDLE_PS_DEC h_ps_d,
                         double **rIntBufferLeft, double **iIntBufferLeft,
                         double **rIntBufferRight, double **iIntBufferRight);

static void applyRotation( HANDLE_PS_DEC pms,
                           double **qmfLeftReal , double **qmfLeftImag,
                           double **qmfRightReal, double **qmfRightImag );


/***************************************************************************/
/*!

  \brief  Allocates a matrix in memory with elements initialized to 0
  Elements can be addressed as myMatrix[row][col]

  \return pointer to first vector

****************************************************************************/
static void **
callocMatrix(int rows,    /*!< Number of rows in matrix*/
             int cols,    /*!< Number of columns in matrix */
             size_t size) /*!< Length in bytes of each element */
{
  int i;
  void **pp;

  pp =(void **) calloc(rows, sizeof(void*));
  if (pp == NULL)
    return NULL;
  for (i=0 ; i<rows ; i++) {
    pp[i] = calloc(cols, size);
    if (pp[i] == NULL) {
      for (i-- ; i>=0 ; i--)
        free(pp[i]);
      free(pp);
      return NULL;
    }
  }
  return (void**)pp;
}

/***************************************************************************/
/*!
  \brief  Free all memory of a matrix.

  Should be used with callocMatrix()

****************************************************************************/
static void
freeMatrix(void **aaMatrix, /*!< Previously allocated matrix to be freed */
           int rows)        /*!< Number of rows in matrix */
{
  int i;

  for (i=0 ; i<rows ; i++) {
    free (aaMatrix[i]);
  }
  free (aaMatrix);
}

static void
clearMatrix(void **aaMatrix, /*!< Previously allocated matrix to be cleared */
            int rows,    /*!< Number of rows in matrix*/
            int cols,    /*!< Number of columns in matrix */
            size_t size) /*!< Length in bytes of each element */
{
  int i;

  for (i=0 ; i<rows ; i++) {
    memset(aaMatrix[i],0,cols*size);
  }
}



/*******************************************************************************
 Functionname:  kChannelFiltering
 *******************************************************************************

 Description:   k-channel complex-valued filtering with 6-tap delay.

 Arguments:

 Return:        none

*******************************************************************************/
static void kChannelFiltering( const double *pQmfReal,
			       const double *pQmfImag,
			       double **mHybridReal,
			       double **mHybridImag,
			       int nSamples,
			       int k,
			       int bCplx,
			       const double *p)
{
  int i, n, q;
  double real, imag;
  double modr, modi;

  for(i = 0; i < nSamples; i++) {
    /* FIR filter. */
    for(q = 0; q < k; q++) {
      real = 0;
      imag = 0;
      for(n = 0; n < 13; n++) {
        if (bCplx)
        {
          modr = (double) cos(SSC_PI*(n-6)/k*(1+2*q));
          modi = (double) -sin(SSC_PI*(n-6)/k*(1+2*q));
        }
        else{
          modr = (double) cos(2*SSC_PI*q*(n-6)/k);
          modi = 0;
        }
        real += p[n] * ( pQmfReal[n+i] * modr - pQmfImag[n+i] * modi );
        imag += p[n] * ( pQmfImag[n+i] * modr + pQmfReal[n+i] * modi );
      }
      mHybridReal[i][q] = real;
      mHybridImag[i][q] = imag;
    }
  }
}


/*******************************************************************************
 Functionname:  HybridAnalysis
 *******************************************************************************

 Description:

 Arguments:

 Return:        none

*******************************************************************************/
void
HybridAnalysis ( const double **mQmfReal,   /*!< The real part of the QMF-matrix.  */
                 const double **mQmfImag,   /*!< The imaginary part of the QMF-matrix. */
                 double **mHybridReal,      /*!< The real part of the hybrid-matrix.  */
                 double **mHybridImag,      /*!< The imaginary part of the hybrid-matrix.  */
                 HANDLE_HYBRID hHybrid,    /*!< Handle to HYBRID struct. */
                 int usedStereoBands)      /*!< indicates which 8 band filter to use */

{
  int    k, n, band;
  HYBRID_RES hybridRes;
  int    frameSize  = hHybrid->frameSize;
  int    chOffset = 0;
  static first[2] = {1, 1};

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    hybridRes = hHybrid->pResolution[band];

    /* Create working buffer. */
    /* Copy stored samples to working buffer. */
    memcpy(hHybrid->pWorkReal, hHybrid->mQmfBufferReal[band],
           hHybrid->qmfBufferMove * sizeof(double));
    memcpy(hHybrid->pWorkImag, hHybrid->mQmfBufferImag[band],
           hHybrid->qmfBufferMove * sizeof(double));

    if (first[usedStereoBands])
    {
        /* Initialise working buffer. */
        for(n = 0; n < HYBRID_FILTER_DELAY; n++) {
          hHybrid->pWorkReal [hHybrid->qmfBufferMove - HYBRID_FILTER_DELAY + n] = mQmfReal [n] [band];
          hHybrid->pWorkImag [hHybrid->qmfBufferMove - HYBRID_FILTER_DELAY + n] = mQmfImag [n] [band];
        }
    }
    /* Append new samples to working buffer. */
    for(n = 0; n < frameSize; n++) {
      hHybrid->pWorkReal [hHybrid->qmfBufferMove + n] = mQmfReal [n + HYBRID_FILTER_DELAY] [band];
      hHybrid->pWorkImag [hHybrid->qmfBufferMove + n] = mQmfImag [n + HYBRID_FILTER_DELAY] [band];
    }

    /* Store samples for next frame. */
    memcpy(hHybrid->mQmfBufferReal[band], hHybrid->pWorkReal + frameSize,
           hHybrid->qmfBufferMove * sizeof(double));
    memcpy(hHybrid->mQmfBufferImag[band], hHybrid->pWorkImag + frameSize,
           hHybrid->qmfBufferMove * sizeof(double));


   if (mHybridReal) {
    /* actual filtering only if output signal requested */
    switch(hybridRes) {
    case HYBRID_2_REAL:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         2,
                         REAL,
                         p2_13_20);
      break;
    case HYBRID_4_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         4,
                         CPLX,
                         p4_13_34);
     break;
    case HYBRID_8_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         8,
                         CPLX,
                         (usedStereoBands==MODE_BANDS34)?p8_13_34:p8_13_20);
      break;
    case HYBRID_12_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         12,
                         CPLX,
                         p12_13_34);
      break;
    default:
      assert(0);
    }

    for(n = 0; n < frameSize; n++) {
      for(k = 0; k < (int)hybridRes; k++) {
        mHybridReal [n] [chOffset + k] = hHybrid->mTempReal [n] [k];
        mHybridImag [n] [chOffset + k] = hHybrid->mTempImag [n] [k];
      }
    }
    chOffset += hybridRes;
   } /* if (mHybridReal) */
  }
  if (first[usedStereoBands])
  {
    first[usedStereoBands] = 0;
  }
}

/*******************************************************************************
 Functionname:  HybridSynthesis
 *******************************************************************************

 Description:

 Arguments:

 Return:        none

*******************************************************************************/
void
HybridSynthesis ( const double **mHybridReal,   /*!< The real part of the hybrid-matrix.  */
                  const double **mHybridImag,   /*!< The imaginary part of the hybrid-matrix. */
                  double **mQmfReal,            /*!< The real part of the QMF-matrix.  */
                  double **mQmfImag,            /*!< The imaginary part of the QMF-matrix.  */
                  HANDLE_HYBRID hHybrid )      /*!< Handle to HYBRID struct. */
{
  int  k, n, band;
  HYBRID_RES hybridRes;
  int  frameSize  = hHybrid->frameSize;
  int  chOffset = 0;

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    hybridRes = hHybrid->pResolution[band];

    for(n = 0; n < frameSize; n++) {

      mQmfReal [n] [band] = mQmfImag [n] [band] = 0;

      for(k = 0; k < (int)hybridRes; k++) {
        mQmfReal [n] [band] += mHybridReal [n] [chOffset + k];
        mQmfImag [n] [band] += mHybridImag [n] [chOffset + k];
      }
    }
    chOffset += hybridRes;
  }
}

/*******************************************************************************
 Functionname:  HybridSynthesis
 *******************************************************************************

 Description:

 Arguments:

 Return:        none

*******************************************************************************/
HANDLE_ERROR_INFO
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,  /*!< Pointer to handle to HYBRID struct. */
                         int frameSize,            /*!< Framesize (in Qmf súbband samples). */
                         int noBands,              /*!< Number of Qmf bands for hybrid filtering. */
                         const int *pResolution )  /*!< Resolution in Qmf bands (length noBands). */
{
  int i;
  int maxNoChannels = HYBRID_12_CPLX;
  HANDLE_HYBRID hs;

  *phHybrid = NULL;

  hs = (HANDLE_HYBRID) calloc (1, sizeof (HYBRID));

  if (hs == NULL)
    return 1;

  hs->pResolution = (int *) malloc (noBands * sizeof (int));

  for (i = 0; i < noBands; i++) {
    hs->pResolution[i] = pResolution[i];
    if( pResolution[i] != HYBRID_12_CPLX &&
        pResolution[i] != HYBRID_8_CPLX &&
        pResolution[i] != HYBRID_2_REAL &&
        pResolution[i] != HYBRID_4_CPLX )
      return 1;
    if(pResolution[i] > maxNoChannels)
      maxNoChannels = pResolution[i];
  }

  hs->nQmfBands     = noBands;
  hs->frameSize     = frameSize;
  hs->frameSizeInit = frameSize;
  hs->qmfBufferMove = HYBRID_FILTER_LENGTH - 1;

  hs->pWorkReal = (double *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (double));
  hs->pWorkImag = (double *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (double));
  if (hs->pWorkReal == NULL || hs->pWorkImag == NULL)
    return 1;

  /* Allocate buffers */
  hs->mQmfBufferReal = (double **) malloc (noBands * sizeof (double *));
  hs->mQmfBufferImag = (double **) malloc (noBands * sizeof (double *));
  if (hs->mQmfBufferReal == NULL || hs->mQmfBufferImag == NULL)
    return 1;
  for (i = 0; i < noBands; i++) {
    hs->mQmfBufferReal[i] = (double *) calloc (hs->qmfBufferMove, sizeof (double));
    hs->mQmfBufferImag[i] = (double *) calloc (hs->qmfBufferMove, sizeof (double));
    if (hs->mQmfBufferReal[i] == NULL || hs->mQmfBufferImag[i] == NULL)
      return 1;
  }

  hs->mTempReal = (double **) malloc (frameSize * sizeof (double *));
  hs->mTempImag = (double **) malloc (frameSize * sizeof (double *));
  if (hs->mTempReal == NULL || hs->mTempImag == NULL)
    return 1;
  for (i = 0; i < frameSize; i++) {
    hs->mTempReal[i] = (double *) calloc (maxNoChannels, sizeof (double));
    hs->mTempImag[i] = (double *) calloc (maxNoChannels, sizeof (double));
    if (hs->mTempReal[i] == NULL || hs->mTempImag[i] == NULL)
      return 1;
  }

  *phHybrid = hs;
  return 0;
}

void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid ) /*!< Pointer to handle to HYBRID struct. */
{
  int i;

  if (*phHybrid) {

    HANDLE_HYBRID hs = *phHybrid;
    int noBands    = hs->nQmfBands;
    int frameSize  = hs->frameSizeInit;

    if(hs->pResolution)
      free(hs->pResolution);

    for (i = 0; i < frameSize; i++) {
      if(hs->mTempReal[i])
        free(hs->mTempReal[i]);
      if(hs->mTempImag[i])
        free(hs->mTempImag[i]);
    }

    if(hs->mTempReal)
     free(hs->mTempReal);
    if(hs->mTempImag)
     free(hs->mTempImag);

    for (i = 0; i < noBands; i++) {
      if(hs->mQmfBufferReal[i])
        free(hs->mQmfBufferReal[i]);
      if(hs->mQmfBufferImag[i])
        free(hs->mQmfBufferImag[i]);
    }

    if(hs->mQmfBufferReal)
      free(hs->mQmfBufferReal);
    if(hs->mQmfBufferImag)
      free(hs->mQmfBufferImag);

    if(hs->pWorkReal)
      free(hs->pWorkReal);
    if(hs->pWorkImag)
      free(hs->pWorkImag);

    free (hs);
    *phHybrid = NULL;
  }
}

static int
decode_huff_cw (Huffman h,                   /*!< pointer to huffman codebook table */
                SSC_BITS *hBitBuf,           /*!< Handle to Bitbuffer */
                int *length)                 /*!< length of huffman codeword */
{
  int bit, index = 0;
  int bit_count = 0;

  while (index >= 0) {
    bit = SSC_BITS_Read(hBitBuf, 1);
    bit_count++;
    index = h[index][bit];
  }
  *length = bit_count;
  return( index+64 ); /* Add offset */
}


/* helper function */
static int
limitMinMax(int i,
            int min,
            int max)
{
  if (i<min)
    return min;
  else if (i>max)
    return max;
  else
    return i;
}


/***************************************************************************/
/*!
  \brief  Decodes delta values in-place and updates
          data buffers according to quantization classes.

  When delta coded in frequency the first element is deltacode from zero.
  aIndex buffer is decoded from delta values to actual values.

  \return none

****************************************************************************/
static void
deltaDecodeArray(int enable,
                 int *aIndex,
                 int *aPrevFrameIndex,
                 int DtDf,
                 int nrElements,   /* as conveyed in bitstream */
                                   /* output array size: nrElements*stride */
                 int stride,       /* 1=dflt, 2=half freq. resolution */
                 int minIdx,
                 int maxIdx)
{
  int i;

  /* Delta decode */
  if ( enable==1 ) {
    if (DtDf == 0)  {   /* Delta coded in freq */
      aIndex[0] = 0 + aIndex[0];
      aIndex[0] = limitMinMax(aIndex[0],minIdx,maxIdx);
      for (i = 1; i < nrElements; i++) {
        aIndex[i] = aIndex[i-1] + aIndex[i];
        aIndex[i] = limitMinMax(aIndex[i],minIdx,maxIdx);
      }
    }
    else { /* Delta time */
      for (i = 0; i < nrElements; i++) {
        aIndex[i] = aPrevFrameIndex[i*stride] + aIndex[i];
        aIndex[i] = limitMinMax(aIndex[i],minIdx,maxIdx);
      }
    }
  }
  else { /* No data is sent, set index to zero */
    for (i = 0; i < nrElements; i++) {
      aIndex[i] = 0;
    }
  }
  if (stride==2) {
    for (i=nrElements*stride-1; i>0; i--) {
      aIndex[i] = aIndex[i/stride];
    }
  }
}

static void
deltaDecodeIpdOpd(int enable,
               int *aIndex,
               int *aPrevFrameIndex,
               int DtDf,
               int nrElements,   /* as conveyed in bitstream */
                                 /* output array size: nrElements*stride */
               int stride,       /* 1=dflt, 2=half freq. resolution */
               int moduloIdx)
{
  int i;

  /* Delta modulo decode */
  if ( enable==1 ) {
    if (DtDf == 0)  {   /* Delta coded in freq */
      aIndex[0] = 0 + aIndex[0];
      aIndex[0] %= moduloIdx;
      for (i = 1; i < nrElements; i++) {
        aIndex[i] = aIndex[i-1] + aIndex[i];
        aIndex[i] %= moduloIdx;
      }
    }
    else { /* Delta time */
      for (i = 0; i < nrElements; i++) {
        aIndex[i] = aPrevFrameIndex[i*stride] + aIndex[i];
        aIndex[i] %= moduloIdx;
      }
    }
  }
  else { /* No data is sent, set index to zero */
    for (i = 0; i < nrElements; i++) {
      aIndex[i] = 0;
    }
  }
  if (stride==2) {
    aIndex[nrElements*stride] = 0;
    for (i=nrElements*stride-1; i>0; i--) {
      aIndex[i] = aIndex[i/stride];
    }
  }
}

static void extractExtendedPsData(HANDLE_PS_DEC  h_ps_d,      /*!< Parametric Stereo Decoder */
                                  SSC_BITS * hBitBuf)         /*!< Handle to the bit buffer */
{
  int     parseIdx;

  parseIdx = h_ps_d->parsertarget;

  if (h_ps_d->aOcsParams[parseIdx].bEnableExt) {
    int cnt;
    int i;
    int nBitsLeft;
    int reserved_ps;
    int env;
    int dtFlag;
    int bin;
    int length;
    Huffman CurrentTable;

    cnt = SSC_BITS_Read(hBitBuf, PS_EXTENSION_SIZE_BITS);
    if (cnt == (1<<PS_EXTENSION_SIZE_BITS)-1)
      cnt += SSC_BITS_Read(hBitBuf, PS_EXTENSION_ESC_COUNT_BITS);

    nBitsLeft = 8 * cnt;
    while (nBitsLeft > 7) {
      int extension_id = SSC_BITS_Read(hBitBuf, PS_EXTENSION_ID_BITS);
      nBitsLeft -= PS_EXTENSION_ID_BITS;

      switch(extension_id) {

      case EXTENSION_ID_IPDOPD_CODING:
        h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd = SSC_BITS_Read(hBitBuf, 1);
        nBitsLeft -= 1;
        if (h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd) {
          for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
            dtFlag = (int)SSC_BITS_Read (hBitBuf, 1);
            nBitsLeft -= 1;
            if (!dtFlag)
              CurrentTable = (Huffman)&aBookPsIpdFreqDecode;
            else
              CurrentTable = (Huffman)&aBookPsIpdTimeDecode;
            for (bin = 0; bin < aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd]; bin++)
            {
              h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env][bin] = decode_huff_cw(CurrentTable,hBitBuf, &length);
              nBitsLeft -= length;
            }
            h_ps_d->aOcsParams[parseIdx].abIpdDtFlag[env] = dtFlag;
#ifdef LOG_STEREO
            LOG_Printf("Ipd bins envelope %d\n", env  );
            LOG_Printf("ipd_dt      : %d\n", dtFlag  );
            for (bin = 0; bin < aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd]; bin++)
            {
                LOG_Printf(" IPD  - [%d] = %d\n", bin, h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env][bin]);
            }
#endif
            dtFlag = (int)SSC_BITS_Read (hBitBuf, 1);
            nBitsLeft -= 1;
            if (!dtFlag)
              CurrentTable = (Huffman)&aBookPsOpdFreqDecode;
            else
              CurrentTable = (Huffman)&aBookPsOpdTimeDecode;
            for (bin = 0; bin < aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd]; bin++)
            {
              h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env][bin] = decode_huff_cw(CurrentTable,hBitBuf, &length);
              nBitsLeft -= length;
            }
            h_ps_d->aOcsParams[parseIdx].abOpdDtFlag[env] = dtFlag;
#ifdef LOG_STEREO
            LOG_Printf("Opd bins envelope %d\n", env );
            LOG_Printf("opd_dt      : %d\n", dtFlag  );
            for (bin = 0; bin < aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd]; bin++)
            {
                LOG_Printf(" OPD  - [%d] = %d\n", bin, h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env][bin]);
            }
#endif
          }
        }
        reserved_ps = SSC_BITS_Read (hBitBuf, 1);
        nBitsLeft -= 1;
        if (reserved_ps != 0)
          assert(0);
        break;

      default:
        /* An unknown extension id causes the remaining extension data
           to be skipped */
        cnt = nBitsLeft >> 3; /* number of remaining bytes */
        for (i=0; i<cnt; i++)
          SSC_BITS_Read(hBitBuf, 8);
        nBitsLeft -= cnt * 8;
        break;
      }
    }
    /* read fill bits for byte alignment */
    SSC_BITS_Read(hBitBuf, nBitsLeft);
  } /* h_ps_d->bEnableExt */
}

#ifdef BASELINE_PS
static void map34IndexTo20 (int *aIndex, int noBins)
{
  aIndex[0]  = (2*aIndex[0]+aIndex[1])/3;
  aIndex[1]  = (aIndex[1]+2*aIndex[2])/3;
  aIndex[2]  = (2*aIndex[3]+aIndex[4])/3;
  aIndex[3]  = (aIndex[4]+2*aIndex[5])/3;
  aIndex[4]  = (aIndex[6]+aIndex[7])/2;
  aIndex[5]  = (aIndex[8]+aIndex[9])/2;
  aIndex[6]  = aIndex[10];
  aIndex[7]  = aIndex[11];
  aIndex[8]  = (aIndex[12]+aIndex[13])/2;
  aIndex[9]  = (aIndex[14]+aIndex[15])/2;
  aIndex[10] = aIndex[16];
  /* For IPD/OPD it stops here */

  if (noBins == NO_HI_RES_BINS)
  {
    aIndex[11] = aIndex[17];
    aIndex[12] = aIndex[18];
    aIndex[13] = aIndex[19];
    aIndex[14] = (aIndex[20]+aIndex[21])/2;
    aIndex[15] = (aIndex[22]+aIndex[23])/2;
    aIndex[16] = (aIndex[24]+aIndex[25])/2;
    aIndex[17] = (aIndex[26]+aIndex[27])/2;
    aIndex[18] = (aIndex[28]+aIndex[29]+aIndex[30]+aIndex[31])/4;
    aIndex[19] = (aIndex[32]+aIndex[33])/2;
  }
}
#else
static void map20IndexTo34 (int *aIndex, int noBins)
{
  /* Do not apply mapping for high bins of IPD/OPD */
  if (noBins == NO_HI_RES_BINS)
  {
    aIndex[33] = aIndex[19];
    aIndex[32] = aIndex[19];
    aIndex[31] = aIndex[18];
    aIndex[30] = aIndex[18];
    aIndex[29] = aIndex[18];
    aIndex[28] = aIndex[18];
    aIndex[27] = aIndex[17];
    aIndex[26] = aIndex[17];
    aIndex[25] = aIndex[16];
    aIndex[24] = aIndex[16];
    aIndex[23] = aIndex[15];
    aIndex[22] = aIndex[15];
    aIndex[21] = aIndex[14];
    aIndex[20] = aIndex[14];
    aIndex[19] = aIndex[13];
    aIndex[18] = aIndex[12];
    aIndex[17] = aIndex[11];
  }

  aIndex[16] = aIndex[10];
  aIndex[15] = aIndex[9];
  aIndex[14] = aIndex[9];
  aIndex[13] = aIndex[8];
  aIndex[12] = aIndex[8];
  aIndex[11] = aIndex[7];
  aIndex[10] = aIndex[6];
  aIndex[9] = aIndex[5];
  aIndex[8] = aIndex[5];
  aIndex[7] = aIndex[4];
  aIndex[6] = aIndex[4];
  aIndex[5] = aIndex[3];
  aIndex[4] = (aIndex[2] + aIndex[3])/2;
  aIndex[3] = aIndex[2];
  aIndex[2] = aIndex[1];
  aIndex[1] = (aIndex[0] + aIndex[1])/2;
  aIndex[0] = aIndex[0];


}
#endif /* BASELINE_PS */


static void map34FloatTo20 (double *aIndex)
{
  aIndex[0]  = (2*aIndex[0]+aIndex[1])/3.0;
  aIndex[1]  = (aIndex[1]+2*aIndex[2])/3.0;
  aIndex[2]  = (2*aIndex[3]+aIndex[4])/3.0;
  aIndex[3]  = (aIndex[4]+2*aIndex[5])/3.0;
  aIndex[4]  = (aIndex[6]+aIndex[7])/2.0;
  aIndex[5]  = (aIndex[8]+aIndex[9])/2.0;
  aIndex[6]  = aIndex[10];
  aIndex[7]  = aIndex[11];
  aIndex[8]  = (aIndex[12]+aIndex[13])/2.0;
  aIndex[9]  = (aIndex[14]+aIndex[15])/2.0;
  aIndex[10] = aIndex[16];
  aIndex[11] = aIndex[17];
  aIndex[12] = aIndex[18];
  aIndex[13] = aIndex[19];
  aIndex[14] = (aIndex[20]+aIndex[21])/2.0;
  aIndex[15] = (aIndex[22]+aIndex[23])/2.0;
  aIndex[16] = (aIndex[24]+aIndex[25])/2.0;
  aIndex[17] = (aIndex[26]+aIndex[27])/2.0;
  aIndex[18] = (aIndex[28]+aIndex[29]+aIndex[30]+aIndex[31])/4.0;
  aIndex[19] = (aIndex[32]+aIndex[33])/2.0;
}

static void map20FloatTo34 (double *aIndex)
{
  double aTemp[NO_HI_RES_BINS];
  int i;

  aTemp[0] = aIndex[0];
  aTemp[1] = (aIndex[0] + aIndex[1])/2.0;
  aTemp[2] = aIndex[1];
  aTemp[3] = aIndex[2];
  aTemp[4] = (aIndex[2] + aIndex[3])/2.0;
  aTemp[5] = aIndex[3];
  aTemp[6] = aIndex[4];
  aTemp[7] = aIndex[4];
  aTemp[8] = aIndex[5];
  aTemp[9] = aIndex[5];
  aTemp[10] = aIndex[6];
  aTemp[11] = aIndex[7];
  aTemp[12] = aIndex[8];
  aTemp[13] = aIndex[8];
  aTemp[14] = aIndex[9];
  aTemp[15] = aIndex[9];
  aTemp[16] = aIndex[10];
  aTemp[17] = aIndex[11];
  aTemp[18] = aIndex[12];
  aTemp[19] = aIndex[13];
  aTemp[20] = aIndex[14];
  aTemp[21] = aIndex[14];
  aTemp[22] = aIndex[15];
  aTemp[23] = aIndex[15];
  aTemp[24] = aIndex[16];
  aTemp[25] = aIndex[16];
  aTemp[26] = aIndex[17];
  aTemp[27] = aIndex[17];
  aTemp[28] = aIndex[18];
  aTemp[29] = aIndex[18];
  aTemp[30] = aIndex[18];
  aTemp[31] = aIndex[18];
  aTemp[32] = aIndex[19];
  aTemp[33] = aIndex[19];

  for(i = 0; i<34; i++) {
    aIndex[i] = aTemp[i];
  }
}


/***************************************************************************/
/*!
  \brief  Decodes delta coded IID, ICC, IPD and OPD indices

  \return none

****************************************************************************/
void
DecodePs(struct PS_DEC *h_ps_d) /*!< */
{
  int gr, env;
  unsigned int parseIdx;

  /* Set parser target to correct value */
  parseIdx = h_ps_d->parsertarget;

  /* Check for valid entry in pipe */
  assert( parseIdx < h_ps_d->pipe_length );

  /* Do nothing for an invalid frame */
  if (!h_ps_d->aOcsParams[parseIdx].bValidFrame)
  {
    return;
  }


  if (!h_ps_d->aOcsParams[parseIdx].bPsDataAvail) {
    /* no new PS data available (e.g. frame loss) */
    /* => keep latest data constant (i.e. FIX with noEnv=0) */
    h_ps_d->aOcsParams[parseIdx].noEnv = 0;
  }

  for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
    int *aPrevIidIndex;
    int *aPrevIccIndex;
    int *aPrevIpdIndex;
    int *aPrevOpdIndex;

    int noIidSteps = h_ps_d->aOcsParams[parseIdx].bFineIidQ?NO_IID_STEPS_FINE:NO_IID_STEPS;

    if (env==0) {
      aPrevIidIndex = h_ps_d->aIidPrevFrameIndex;
      aPrevIccIndex = h_ps_d->aIccPrevFrameIndex;
      aPrevIpdIndex = h_ps_d->aIpdPrevFrameIndex;
      aPrevOpdIndex = h_ps_d->aOpdPrevFrameIndex;
    }
    else {
      aPrevIidIndex = h_ps_d->aOcsParams[parseIdx].aaIidIndex[env-1];
      aPrevIccIndex = h_ps_d->aOcsParams[parseIdx].aaIccIndex[env-1];
      aPrevIpdIndex = h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env-1];
      aPrevOpdIndex = h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env-1];
    }

    deltaDecodeArray(h_ps_d->aOcsParams[parseIdx].bEnableIid && !(h_ps_d->psMode & 0x0001),
                     h_ps_d->aOcsParams[parseIdx].aaIidIndex[env],
                     aPrevIidIndex,
                     h_ps_d->aOcsParams[parseIdx].abIidDtFlag[env],
                     aNoIidBins[h_ps_d->aOcsParams[parseIdx].freqResIid],
                     (h_ps_d->aOcsParams[parseIdx].freqResIid)?1:2,
                     -noIidSteps,
                     noIidSteps);
#ifdef LOG_STEREO
    if (h_ps_d->aOcsParams[parseIdx].bEnableIid)
    {
      int bins = aNoIidBins[h_ps_d->aOcsParams[parseIdx].freqResIid];
      if (h_ps_d->aOcsParams[parseIdx].freqResIid == 0)
        bins *= 2;
      LOG_Printf("Iid decoded envelope %d\n", env  );

      for (gr = 0; gr < bins; gr++)
      {
        LOG_Printf(" decoded IID  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaIidIndex[env][gr]);
      }
    }
#endif

    deltaDecodeArray(h_ps_d->aOcsParams[parseIdx].bEnableIcc && !(h_ps_d->psMode & 0x0002),
                     h_ps_d->aOcsParams[parseIdx].aaIccIndex[env],
                     aPrevIccIndex,
                     h_ps_d->aOcsParams[parseIdx].abIccDtFlag[env],
                     aNoIccBins[h_ps_d->aOcsParams[parseIdx].freqResIcc],
                     (h_ps_d->aOcsParams[parseIdx].freqResIcc)?1:2,
                     0,
                     NO_ICC_STEPS-1);

#ifdef LOG_STEREO
    if (h_ps_d->aOcsParams[parseIdx].bEnableIcc)
    {
      int bins = aNoIccBins[h_ps_d->aOcsParams[parseIdx].freqResIcc];
      if (h_ps_d->aOcsParams[parseIdx].freqResIcc == 0)
        bins *= 2;
      LOG_Printf("Icc decoded envelope %d\n", env  );

      for (gr = 0; gr < bins; gr++)
      {
        LOG_Printf(" decoded ICC  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaIccIndex[env][gr]);
      }
    }
#endif

    deltaDecodeIpdOpd(h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd && !(h_ps_d->psMode & 0x0004),
                     h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env],
                     aPrevIpdIndex,
                     h_ps_d->aOcsParams[parseIdx].abIpdDtFlag[env],
                     aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd],
                     (h_ps_d->aOcsParams[parseIdx].freqResIpd)?1:2,
                     NO_IPD_STEPS);

    deltaDecodeIpdOpd(h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd && !(h_ps_d->psMode & 0x0004),
                     h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env],
                     aPrevOpdIndex,
                     h_ps_d->aOcsParams[parseIdx].abOpdDtFlag[env],
                     aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd],
                     (h_ps_d->aOcsParams[parseIdx].freqResIpd)?1:2,
                     NO_IPD_STEPS);

#ifdef LOG_STEREO
    if (h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd)
    {
      int bins = aNoIpdBins[h_ps_d->aOcsParams[parseIdx].freqResIpd];
      if (h_ps_d->aOcsParams[parseIdx].freqResIpd == 0)
        bins *= 2;
      LOG_Printf("Ipd bins envelope %d\n", env  );
      for (gr = 0; gr < bins; gr++)
      {
          LOG_Printf(" decoded IPD  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env][gr]);
      }
      LOG_Printf("Opd bins envelope %d\n", env  );
      for (gr = 0; gr < bins; gr++)
      {
          LOG_Printf(" decoded OPD  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env][gr]);
      }
    }
#endif

  }   /* for (env=0; env<h_ps_d->noEnv; env++) */

  /* handling of FIX noEnv=0 */
  if (h_ps_d->aOcsParams[parseIdx].noEnv==0) {
    /* set noEnv=1, keep last parameters or force 0 if not enabled */
    h_ps_d->aOcsParams[parseIdx].noEnv = 1;

    if (h_ps_d->aOcsParams[parseIdx].bEnableIid) {
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIidIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] =
          h_ps_d->aIidPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIidIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] = 0;
      }
    }

    if (h_ps_d->aOcsParams[parseIdx].bEnableIcc) {
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIccIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] =
          h_ps_d->aIccPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIccIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] = 0;
      }
    }

    if (h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd) {
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] =
           h_ps_d->aIpdPrevFrameIndex[gr];
      }
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaOpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] =
           h_ps_d->aOpdPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] = 0;
        h_ps_d->aOcsParams[parseIdx].aaOpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr] = 0;
      }
    }
  }

  /* Update previous frame index buffers */
  for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
    h_ps_d->aIidPrevFrameIndex[gr] =
      h_ps_d->aOcsParams[parseIdx].aaIidIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
    h_ps_d->aIccPrevFrameIndex[gr] =
      h_ps_d->aOcsParams[parseIdx].aaIccIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
    h_ps_d->aIpdPrevFrameIndex[gr] =
      h_ps_d->aOcsParams[parseIdx].aaIpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
    h_ps_d->aOpdPrevFrameIndex[gr] =
      h_ps_d->aOcsParams[parseIdx].aaOpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
  }

  /* Save unmodified OCS parameters */
  h_ps_d->prevOcsParams = h_ps_d->aOcsParams[parseIdx];

  /* PS data from bitstream (if avail) was decoded now */
  h_ps_d->aOcsParams[parseIdx].bPsDataAvail = 0;

  /* debug code, forcing indices to 0 (= PS off) if requested by psMode */
  for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
    if (h_ps_d->psMode & 0x0100)
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIidIndex[env][gr] = 0;
      }
    if (h_ps_d->psMode & 0x0200)
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIccIndex[env][gr] = 0;
      }
    if (h_ps_d->psMode & 0x0400)
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env][gr] = 0;
        h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env][gr] = 0;
      }
  }

  /* handling of env borders for FIX & VAR */
  if (h_ps_d->aOcsParams[parseIdx].bFrameClass == 0) {
    /* FIX_BORDERS NoEnv=0,1,2,4 */
    h_ps_d->aOcsParams[parseIdx].aEnvStartStop[0] = 0;
    for (env=1; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
      h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] =
        (env * h_ps_d->noSubSamples) / h_ps_d->aOcsParams[parseIdx].noEnv;
    }
    h_ps_d->aOcsParams[parseIdx].aEnvStartStop[h_ps_d->aOcsParams[parseIdx].noEnv] = h_ps_d->noSubSamples;
    /* 1024 (32 slots) env borders:  0, 8, 16, 24, 32 */
    /*  960 (30 slots) env borders:  0, 7, 15, 22, 30 */
  }
  else {   /* if (h_ps_d->bFrameClass == 0) */
    /* VAR_BORDERS NoEnv=1,2,3,4 */
    h_ps_d->aOcsParams[parseIdx].aEnvStartStop[0] = 0;

    /* handle case aEnvStartStop[noEnv]<noSubSample for VAR_BORDERS by
       duplicating last PS parameters and incrementing noEnv */
    if (h_ps_d->aOcsParams[parseIdx].aEnvStartStop[h_ps_d->aOcsParams[parseIdx].noEnv] <
          (SSC_MAX_FRAME_SIZE/NO_QMF_CHANNELS)/2 )
    {

      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIidIndex[h_ps_d->aOcsParams[parseIdx].noEnv][gr] =
          h_ps_d->aOcsParams[parseIdx].aaIidIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
      }
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIccIndex[h_ps_d->aOcsParams[parseIdx].noEnv][gr] =
          h_ps_d->aOcsParams[parseIdx].aaIccIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
      }
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaIpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv][gr] =
          h_ps_d->aOcsParams[parseIdx].aaIpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
      }
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aOcsParams[parseIdx].aaOpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv][gr] =
          h_ps_d->aOcsParams[parseIdx].aaOpdIndex[h_ps_d->aOcsParams[parseIdx].noEnv-1][gr];
      }
      h_ps_d->aOcsParams[parseIdx].noEnv++;
      h_ps_d->aOcsParams[parseIdx].aEnvStartStop[h_ps_d->aOcsParams[parseIdx].noEnv] = h_ps_d->noSubSamples;
    }

    /* enforce strictly monotonic increasing borders */
    for (env=1; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
      int thr;
      thr = h_ps_d->noSubSamples - (h_ps_d->aOcsParams[parseIdx].noEnv - env);
      if (h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] > thr) {
        h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] = thr;
      }
      else {
        thr = h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env-1]+1;
        if (h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] < thr) {
          h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] = thr;
        }
      }
    }
  }   /* if (h_ps_d->bFrameClass == 0) ... else */

  /* copy data prior to possible 20<->34 in-place mapping */
  for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
    int i;
    for (i=0; i<NO_HI_RES_IID_BINS; i++) {
      h_ps_d->aOcsParams[parseIdx].aaIidIndexMapped[env][i] = h_ps_d->aOcsParams[parseIdx].aaIidIndex[env][i];
    }
    for (i=0; i<NO_HI_RES_ICC_BINS; i++) {
      h_ps_d->aOcsParams[parseIdx].aaIccIndexMapped[env][i] = h_ps_d->aOcsParams[parseIdx].aaIccIndex[env][i];
    }
    for (i=0; i<NO_HI_RES_IPD_BINS; i++) {
      h_ps_d->aOcsParams[parseIdx].aaIpdIndexMapped[env][i] = h_ps_d->aOcsParams[parseIdx].aaIpdIndex[env][i];
      h_ps_d->aOcsParams[parseIdx].aaOpdIndexMapped[env][i] = h_ps_d->aOcsParams[parseIdx].aaOpdIndex[env][i];
    }
  }

#ifdef BASELINE_PS
  /* MPEG baseline PS */
  for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
    int i;
    if (h_ps_d->aOcsParams[parseIdx].freqResIid == 2)
      map34IndexTo20 (h_ps_d->aOcsParams[parseIdx].aaIidIndexMapped[env], NO_HI_RES_IID_BINS);
    if (h_ps_d->aOcsParams[parseIdx].freqResIcc == 2)
      map34IndexTo20 (h_ps_d->aOcsParams[parseIdx].aaIccIndexMapped[env], NO_HI_RES_ICC_BINS);
    /* disable IPD/OPD */
    for (i=0; i<NO_HI_RES_IPD_BINS; i++) {
      h_ps_d->aOcsParams[parseIdx].aaIpdIndexMapped[env][i] = 0;
      h_ps_d->aOcsParams[parseIdx].aaOpdIndexMapped[env][i] = 0;
    }
  }
#else
  /* MPEG unrestricted PS */
  if (h_ps_d->aOcsParams[parseIdx].usedStereoBands == MODE_BANDS34)
  {
    for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
      if (h_ps_d->aOcsParams[parseIdx].freqResIid < 2)
        map20IndexTo34 (h_ps_d->aOcsParams[parseIdx].aaIidIndexMapped[env], NO_HI_RES_IID_BINS);
      if (h_ps_d->aOcsParams[parseIdx].freqResIcc < 2)
        map20IndexTo34 (h_ps_d->aOcsParams[parseIdx].aaIccIndexMapped[env], NO_HI_RES_ICC_BINS);
      if (h_ps_d->aOcsParams[parseIdx].freqResIpd < 2)
      {
        map20IndexTo34 (h_ps_d->aOcsParams[parseIdx].aaIpdIndexMapped[env], NO_HI_RES_IPD_BINS);
        map20IndexTo34 (h_ps_d->aOcsParams[parseIdx].aaOpdIndexMapped[env], NO_HI_RES_IPD_BINS);
      }
    }
  }
#endif /* BASELINE_PS */

  /* Point to next target in pipe */
  h_ps_d->parsertarget += 1;

}


/***************************************************************************/
/*!

  \brief  Reads parametric stereo data from bitstream

  \return

****************************************************************************/
unsigned int
ReadPsData (HANDLE_PS_DEC h_ps_d, /*!< handle to struct PS_DEC*/
            SSC_BITS * hBitBuf    /*!< handle to struct BIT_BUF*/
           )
{
  int     gr, env;
  int     length;
  int     dtFlag;
  int     startbits;
  Huffman CurrentTable;
  int     bEnableHeader;
  unsigned int parseIdx;

  if (!h_ps_d)
    return 0;

  /* Set parser target to correct value */
  parseIdx = h_ps_d->parsertarget;

  /* Check for valid entry in pipe */
  assert( parseIdx < h_ps_d->pipe_length );

  startbits = hBitBuf->Offset;

  bEnableHeader = (int) SSC_BITS_Read (hBitBuf, 1);

  /* Read header */
  if (bEnableHeader) {
    h_ps_d->aOcsParams[parseIdx].bEnableIid = (int) SSC_BITS_Read (hBitBuf, 1);
    if (h_ps_d->aOcsParams[parseIdx].bEnableIid)
      h_ps_d->aOcsParams[parseIdx].freqResIid = (int) SSC_BITS_Read (hBitBuf, 3);

    h_ps_d->aOcsParams[parseIdx].bEnableIcc = (int) SSC_BITS_Read (hBitBuf, 1);
    if (h_ps_d->aOcsParams[parseIdx].bEnableIcc)
      h_ps_d->aOcsParams[parseIdx].freqResIcc = (int) SSC_BITS_Read (hBitBuf, 3);

    h_ps_d->aOcsParams[parseIdx].bEnableExt = (int) SSC_BITS_Read (hBitBuf, 1);
    if (!h_ps_d->aOcsParams[parseIdx].bEnableExt)
          h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd = 0;

    /* verify that IID & ICC modes (quant grid, freq res) are supported */
    if ((h_ps_d->aOcsParams[parseIdx].freqResIid > 5) || (h_ps_d->aOcsParams[parseIdx].freqResIcc > 5)) {
      /* no useful PS data could be read from bitstream */
      h_ps_d->aOcsParams[parseIdx].bPsDataAvail = 0;

      /* Give error */
      return hBitBuf->Offset - startbits;
    }

    if (h_ps_d->aOcsParams[parseIdx].freqResIid > 2){
      h_ps_d->aOcsParams[parseIdx].bFineIidQ = 1;
      h_ps_d->aOcsParams[parseIdx].freqResIid -=3;
    }
    else{
      h_ps_d->aOcsParams[parseIdx].bFineIidQ = 0;
    }
    h_ps_d->aOcsParams[parseIdx].freqResIpd = h_ps_d->aOcsParams[parseIdx].freqResIid;

    if (h_ps_d->aOcsParams[parseIdx].freqResIcc > 2){
      h_ps_d->aOcsParams[parseIdx].bUsePcaRot = 1;
      h_ps_d->aOcsParams[parseIdx].freqResIcc -=3;
    }
    else{
      h_ps_d->aOcsParams[parseIdx].bUsePcaRot = 0;
    }

    if ((h_ps_d->aOcsParams[parseIdx].freqResIid == 2) ||
        (h_ps_d->aOcsParams[parseIdx].freqResIcc == 2) )
    {
        h_ps_d->aOcsParams[parseIdx].usedStereoBands = MODE_BANDS34;  /* 34 Stereo bands */
    }
    else
    {
        h_ps_d->aOcsParams[parseIdx].usedStereoBands = MODE_BANDS20;  /* 10/20 Stereo bands */
    }
#ifdef BASELINE_PS
    h_ps_d->aOcsParams[parseIdx].usedStereoBands = MODE_BANDS20;
    h_ps_d->aOcsParams[parseIdx].bUsePcaRot = 0;
#endif
  }
  else {
    h_ps_d->aOcsParams[parseIdx].bEnableIid      = h_ps_d->prevOcsParams.bEnableIid;
    h_ps_d->aOcsParams[parseIdx].freqResIid      = h_ps_d->prevOcsParams.freqResIid;
    h_ps_d->aOcsParams[parseIdx].bEnableIcc      = h_ps_d->prevOcsParams.bEnableIcc;
    h_ps_d->aOcsParams[parseIdx].freqResIcc      = h_ps_d->prevOcsParams.freqResIcc;
    h_ps_d->aOcsParams[parseIdx].bEnableExt      = h_ps_d->prevOcsParams.bEnableExt;
    h_ps_d->aOcsParams[parseIdx].bEnableIpdOpd   = h_ps_d->prevOcsParams.bEnableIpdOpd;
    h_ps_d->aOcsParams[parseIdx].bFineIidQ       = h_ps_d->prevOcsParams.bFineIidQ;
    h_ps_d->aOcsParams[parseIdx].bUsePcaRot      = h_ps_d->prevOcsParams.bUsePcaRot;
    h_ps_d->aOcsParams[parseIdx].freqResIpd      = h_ps_d->prevOcsParams.freqResIpd;
    h_ps_d->aOcsParams[parseIdx].usedStereoBands = h_ps_d->prevOcsParams.usedStereoBands;
  }

  h_ps_d->aOcsParams[parseIdx].bFrameClass = (int) SSC_BITS_Read (hBitBuf, 1);
  if (h_ps_d->aOcsParams[parseIdx].bFrameClass == 0) {
    /* FIX_BORDERS NoEnv=0,1,2,4 */
    h_ps_d->aOcsParams[parseIdx].noEnv = aFixNoEnvDecode[(int) SSC_BITS_Read (hBitBuf, 2)];
    /* all additional handling of env borders is now in DecodePs() */
  }
  else {
    /* VAR_BORDERS NoEnv=1,2,3,4 */
    h_ps_d->aOcsParams[parseIdx].noEnv = 1+(int) SSC_BITS_Read (hBitBuf, 2);
    for (env=1; env<h_ps_d->aOcsParams[parseIdx].noEnv+1; env++)
      h_ps_d->aOcsParams[parseIdx].aEnvStartStop[env] = ((int) SSC_BITS_Read (hBitBuf, 5)) + 1;
    /* all additional handling of env borders is now in DecodePs() */
  }

  /* Extract IID data */
  if (h_ps_d->aOcsParams[parseIdx].bEnableIid) {
    for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
      dtFlag = (int)SSC_BITS_Read (hBitBuf, 1);
      if (!dtFlag)
      {
        if (h_ps_d->aOcsParams[parseIdx].bFineIidQ)
          CurrentTable = (Huffman)&aBookPsIidFineFreqDecode;
        else
          CurrentTable = (Huffman)&aBookPsIidFreqDecode;
      }
      else
      {
        if (h_ps_d->aOcsParams[parseIdx].bFineIidQ)
          CurrentTable = (Huffman)&aBookPsIidFineTimeDecode;
        else
          CurrentTable = (Huffman)&aBookPsIidTimeDecode;
      }

      for (gr = 0; gr < aNoIidBins[h_ps_d->aOcsParams[parseIdx].freqResIid]; gr++)
        h_ps_d->aOcsParams[parseIdx].aaIidIndex[env][gr] = decode_huff_cw(CurrentTable,hBitBuf,&length);
      h_ps_d->aOcsParams[parseIdx].abIidDtFlag[env] = dtFlag;
#ifdef LOG_STEREO
      LOG_Printf("Iid bins envelope %d\n", env  );
      LOG_Printf("iid_dt      : %d\n", dtFlag  );

      for (gr = 0; gr < aNoIidBins[h_ps_d->aOcsParams[parseIdx].freqResIid]; gr++)
      {
        LOG_Printf(" IID  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaIidIndex[env][gr]);
      }
#endif
    }
  }

  /* Extract ICC data */
  if (h_ps_d->aOcsParams[parseIdx].bEnableIcc) {
    for (env=0; env<h_ps_d->aOcsParams[parseIdx].noEnv; env++) {
      dtFlag = (int)SSC_BITS_Read (hBitBuf, 1);
      if (!dtFlag)
        CurrentTable = (Huffman)&aBookPsIccFreqDecode;
      else
        CurrentTable = (Huffman)&aBookPsIccTimeDecode;

      for (gr = 0; gr < aNoIccBins[h_ps_d->aOcsParams[parseIdx].freqResIcc]; gr++)
        h_ps_d->aOcsParams[parseIdx].aaIccIndex[env][gr] = decode_huff_cw(CurrentTable,hBitBuf,&length);
      h_ps_d->aOcsParams[parseIdx].abIccDtFlag[env] = dtFlag;
#ifdef LOG_STEREO
      LOG_Printf("Icc bins envelope %d\n", env  );
      LOG_Printf("icc_dt      : %d\n", dtFlag  );

      for (gr = 0; gr < aNoIccBins[h_ps_d->aOcsParams[parseIdx].freqResIcc]; gr++)
      {
        LOG_Printf(" ICC  - [%d] = %d\n", gr, h_ps_d->aOcsParams[parseIdx].aaIccIndex[env][gr]);
      }
#endif
    }
  }

  /* Extract IPD/OPD data */
  extractExtendedPsData(h_ps_d, hBitBuf);

  /* new PS data was read from bitstream */
  h_ps_d->aOcsParams[parseIdx].bPsDataAvail = 1;
  h_ps_d->aOcsParams[parseIdx].bValidFrame = 1;

  /* return number of bit read from stream */
  return hBitBuf->Offset - startbits;
}

static void
initPsTables (HANDLE_PS_DEC h_ps_d)
{
  unsigned int i, j;

  double hybridCenterFreq20[12] = {0.5/4,  1.5/4,  2.5/4,  3.5/4,
                              4.5/4*0,  5.5/4*0, -1.5/4, -0.5/4,
                                  3.5/2,  2.5/2,  4.5/2,  5.5/2};

  double hybridCenterFreq34[32] = {1/12,   3/12,   5/12,   7/12,
                                  9/12,  11/12,  13/12,  15/12,
                                  17/12, -5/12,  -3/12,  -1/12,
                                  17/8,   19/8,    5/8,    7/8,
                                  9/8,    11/8,   13/8,   15/8,
                                  9/4,    11/4,   13/4,    7/4,
                                  17/4,   11/4,   13/4,   15/4,
                                  17/4,   19/4,   21/4,   15/4};

  if (psTablesInitialized)
    return;

  for (j=0 ; j < NO_QMF_CHANNELS ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImQmf[j] = (double)sin(-SSC_PI*(j+0.5)*(aAllpassLinkFractDelay));
    h_ps_d->aaFractDelayPhaseFactorReQmf[j] = (double)cos(-SSC_PI*(j+0.5)*(aAllpassLinkFractDelay));
  }
  for (j=0 ; j < NO_SUB_QMF_CHANNELS ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImSubQmf20[j]= (double)sin(-SSC_PI*hybridCenterFreq20[j]*aAllpassLinkFractDelay);
    h_ps_d->aaFractDelayPhaseFactorReSubQmf20[j] = (double)cos(SSC_PI*hybridCenterFreq20[j]*aAllpassLinkFractDelay);
  }
  for (j=0 ; j < NO_SUB_QMF_CHANNELS_HI_RES ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImSubQmf34[j] = (double)sin(-SSC_PI*hybridCenterFreq34[j]*aAllpassLinkFractDelay);
    h_ps_d->aaFractDelayPhaseFactorReSubQmf34[j] = (double)cos(-SSC_PI*hybridCenterFreq34[j]*aAllpassLinkFractDelay);
  }

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {

    aAllpassLinkDecaySer[i] = (double)exp(-aAllpassLinkDelaySer[i]/ALLPASS_SERIAL_TIME_CONST);

    for (j=0 ; j < NO_QMF_CHANNELS ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImQmf[j][i] = (double)sin(-SSC_PI*(j+0.5)*(aAllpassLinkFractDelaySer[i]));
      h_ps_d->aaFractDelayPhaseFactorSerReQmf[j][i] = (double)cos(-SSC_PI*(j+0.5)*(aAllpassLinkFractDelaySer[i]));
    }

    for (j=0 ; j < NO_SUB_QMF_CHANNELS ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20[j][i] = (double)sin(-SSC_PI*hybridCenterFreq20[j]*aAllpassLinkFractDelaySer[i]);
      h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20[j][i] = (double)cos(SSC_PI*hybridCenterFreq20[j]*aAllpassLinkFractDelaySer[i]);
    }

    for (j=0 ; j < NO_SUB_QMF_CHANNELS_HI_RES ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34[j][i] = (double)sin(-SSC_PI*hybridCenterFreq34[j]*aAllpassLinkFractDelaySer[i]);
      h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34[j][i] = (double)cos(-SSC_PI*hybridCenterFreq34[j]*aAllpassLinkFractDelaySer[i]);
    }
  }
/* scaleFactors */
    /* precalculate scale factors for the negative iids */
    for ( i = NO_IID_STEPS - 1, j = 0; j < NO_IID_STEPS; i--, j++ )
    {
        scaleFactors[j] = ( double )sqrt( 2.0 / ( 1.0 + pow( 10.0, -quantizedIIDs[i] / 10.0 ) ) );
    }

    /* iid = 0 */
    scaleFactors[j++] = 1.0;

    /* precalculate scale factors for the positive iids */
    for ( i = 0; i < NO_IID_STEPS; i++, j++ )
    {
        scaleFactors[j] = ( double )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDs[i] / 10.0 ) ) );
    }
    for ( i = 0; i < NO_IID_STEPS_FINE; i++, j++ )
    {
        scaleFactorsFine[j] = ( double )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDsFine[i] / 10.0 ) ) );
    }
/* scaleFactorsFine */
    /* precalculate scale factors for the negative iids */
    for ( i = NO_IID_STEPS_FINE - 1, j = 0; j < NO_IID_STEPS_FINE; i--, j++ )
    {
        scaleFactorsFine[j] = ( double )sqrt( 2.0 / ( 1.0 + pow( 10.0, -quantizedIIDsFine[i] / 10.0 ) ) );
    }
    /* iid = 0 */
    scaleFactorsFine[j++] = 1.0;
    /* precalculate scale factors for the positive iids */
    for ( i = 0; i < NO_IID_STEPS_FINE; i++, j++ )
    {
        scaleFactorsFine[j] = ( double )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDsFine[i] / 10.0 ) ) );
    }

    /* precalculate alphas */
    for ( i = 0; i < NO_ICC_LEVELS; i++ )
    {
        alphas[i] = ( double )( 0.5 * acos ( quantizedRHOs[i] ) );
    }

  psTablesInitialized = 1;
}


/***************************************************************************/
/*!
  \brief  Creates one instance of the PS_DEC struct

  \return Error info

****************************************************************************/
int
CreatePsDec(HANDLE_PS_DEC *h_PS_DEC,
            int fs,
            unsigned int noQmfChans,
            unsigned int noSubSamples,
            int psMode)
{
  unsigned int i;
  int *pErr;
  HANDLE_PS_DEC  h_ps_d;

  const unsigned int noQmfBandsInHybrid20 = 3;
  const unsigned int noQmfBandsInHybrid34 = 5;

  const int aHybridResolution20[] = { HYBRID_8_CPLX,
                                      HYBRID_2_REAL,
                                      HYBRID_2_REAL };

  const int aHybridResolution34[] = { HYBRID_12_CPLX,
                                      HYBRID_8_CPLX,
                                      HYBRID_4_CPLX,
                                      HYBRID_4_CPLX,
                                      HYBRID_4_CPLX };

  h_ps_d = (HANDLE_PS_DEC) calloc(1,sizeof(struct PS_DEC));
  if (h_ps_d == NULL)
    return 1;

  /* Suppress compiler warning */
  fs = fs;

  /* initialisation */
  h_ps_d->noSubSamples     = noSubSamples;   /* col */
  h_ps_d->noSubSamplesInit = noSubSamples;   /* col */
  h_ps_d->noChannels       = noQmfChans;   /* row */
  h_ps_d->psMode           = psMode;   /* 0: no override */
  h_ps_d->bUse34StereoBands = 0;
  h_ps_d->bUse34StereoBandsPrev = 0;

  /* Pipe structure */
  h_ps_d->parsertarget = DELAY;
  h_ps_d->pipe_length  = DELAY + INTERLEAVE;

  /* explicitly init state variables to safe values (until first ps header arrives) */
  for (i=0; i < h_ps_d->pipe_length; i++)
  {
      h_ps_d->aOcsParams[i].bValidFrame = 0;
      h_ps_d->aOcsParams[i].bPsDataAvail = 0;
      h_ps_d->aOcsParams[i].bEnableIid = 0;
      h_ps_d->aOcsParams[i].bEnableIcc = 0;
      h_ps_d->aOcsParams[i].bEnableExt = 0;
      h_ps_d->aOcsParams[i].bEnableIpdOpd = 0;
      h_ps_d->aOcsParams[i].freqResIid = 0;
      h_ps_d->aOcsParams[i].freqResIcc = 0;
  }

  /* Hybrid Filter Bank 1 creation. */
  pErr = (int*)CreateHybridFilterBank ( &h_ps_d->hHybrid20,
                                 h_ps_d->noSubSamples,
                                 noQmfBandsInHybrid20,
                                 aHybridResolution20 );

  /* Hybrid Filter Bank 2 creation. */
  pErr = (int*)CreateHybridFilterBank ( &h_ps_d->hHybrid34,
                                 h_ps_d->noSubSamples,
                                 noQmfBandsInHybrid34,
                                 aHybridResolution34 );

  /* Buffer allocation. */
  h_ps_d->mHybridRealLeft  = (double **) calloc (h_ps_d->noSubSamples, sizeof (double *));
  h_ps_d->mHybridImagLeft  = (double **) calloc (h_ps_d->noSubSamples, sizeof (double *));
  h_ps_d->mHybridRealRight = (double **) calloc (h_ps_d->noSubSamples, sizeof (double *));
  h_ps_d->mHybridImagRight = (double **) calloc (h_ps_d->noSubSamples, sizeof (double *));

  if (h_ps_d->mHybridRealLeft  == NULL || h_ps_d->mHybridImagLeft  == NULL ||
      h_ps_d->mHybridRealRight == NULL || h_ps_d->mHybridImagRight == NULL) {
    DeletePsDec (&h_ps_d);
    return 1;
  }

  for (i = 0; i < h_ps_d->noSubSamples; i++) {

    h_ps_d->mHybridRealLeft[i] =
      (double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (double));
    h_ps_d->mHybridImagLeft[i] =
      (double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (double));
    h_ps_d->mHybridRealRight[i] =
      (double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (double));
    h_ps_d->mHybridImagRight[i] =
      (double *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (double));

    if (h_ps_d->mHybridRealLeft[i] == NULL  || h_ps_d->mHybridImagLeft[i]  == NULL ||
        h_ps_d->mHybridRealRight[i] == NULL || h_ps_d->mHybridImagRight[i] == NULL) {
      DeletePsDec (&h_ps_d);
      return 1;
    }
  }

  /* Delay */
  h_ps_d->delayBufIndex   = 0;
  h_ps_d->noSampleDelayAllpass = 2;

  for (i=0 ; i < NO_QMF_CHANNELS ; i++) {
    h_ps_d->aDelayBufIndexDelayQmf[i] = 0;
    h_ps_d->aNoSampleDelayDelayQmf[i] = delayIndexQmf[i];
  }
  h_ps_d->noSampleDelay = h_ps_d->aNoSampleDelayDelayQmf[0];

  h_ps_d->firstDelayGr20 = SUBQMF_GROUPS;
  h_ps_d->firstDelayGr34 = SUBQMF_GROUPS_HI_RES;
  h_ps_d->firstDelaySb   = 23;   /* 24th QMF first with Delay */

  h_ps_d->aaRealDelayBufferQmf = (double **)callocMatrix( h_ps_d->noSampleDelay,
                                                         noQmfChans, sizeof(double));
  if (h_ps_d->aaRealDelayBufferQmf == NULL)
    return 1;

  h_ps_d->aaImagDelayBufferQmf = (double **)callocMatrix(h_ps_d->noSampleDelay,
                                                        noQmfChans, sizeof(double));
  if (h_ps_d->aaImagDelayBufferQmf == NULL)
    return 1;

  h_ps_d->aaRealDelayBufferSubQmf = (double **)callocMatrix( h_ps_d->noSampleDelay,
                                                         NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaRealDelayBufferSubQmf == NULL)
    return 1;

  h_ps_d->aaImagDelayBufferSubQmf = (double **)callocMatrix(h_ps_d->noSampleDelay,
                                                        NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaImagDelayBufferSubQmf == NULL)
    return 1;

  /* allpass filter */
  h_ps_d->aaFractDelayPhaseFactorReQmf = (double*)calloc(NO_QMF_CHANNELS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorReQmf == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImQmf = (double*)calloc(NO_QMF_CHANNELS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorImQmf == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReQmf = (double**)callocMatrix(NO_QMF_CHANNELS,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerReQmf == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImQmf = (double**)callocMatrix(NO_QMF_CHANNELS,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerImQmf == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorReSubQmf20 = (double*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorReSubQmf20 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImSubQmf20 = (double*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorImSubQmf20 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 = (double**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 = (double**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 == NULL)
    return 1;



  h_ps_d->aaFractDelayPhaseFactorReSubQmf34 = (double*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorReSubQmf34 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImSubQmf34 = (double*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorImSubQmf34 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 = (double**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 = (double**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(double));
  if (h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 == NULL)
    return 1;



  initPsTables(h_ps_d);

  {
    h_ps_d->PeakDecayFactorFast = 0.765928338364649;

    h_ps_d->intFilterCoeff = 1.0 - NRG_INT_COEFF;
    for (i=0 ; i < NO_HI_RES_BINS ; i++) {
      h_ps_d->aPeakDecayFastBin[i] = 0.0;
      h_ps_d->aPrevNrgBin[i] = 0.0;
      h_ps_d->aPrevPeakDiffBin[i] = 0.0;
    }

    for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {
      h_ps_d->aDelayRBufIndexSer[i] = 0;
      h_ps_d->aNoSampleDelayRSer[i] = aAllpassLinkDelaySer[i];

      h_ps_d->aaaRealDelayRBufferSerQmf[i] = (double**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                         noQmfChans, sizeof(double));
      if (h_ps_d->aaaRealDelayRBufferSerQmf[i] == NULL)
        return 1;
      h_ps_d->aaaImagDelayRBufferSerQmf[i] = (double**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                        noQmfChans, sizeof(double));
      if (h_ps_d->aaaImagDelayRBufferSerQmf[i] == NULL)
        return 1;

      h_ps_d->aaaRealDelayRBufferSerSubQmf[i] = (double**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                         NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
      if (h_ps_d->aaaRealDelayRBufferSerSubQmf[i] == NULL)
        return 1;
      h_ps_d->aaaImagDelayRBufferSerSubQmf[i] = (double**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                        NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
      if (h_ps_d->aaaImagDelayRBufferSerSubQmf[i] == NULL)
        return 1;
    }
  }

  ResetPsDec( h_ps_d );

  ResetPsDeCor( h_ps_d );

  *h_PS_DEC=h_ps_d;
  return 0;
} /*END CreatePsDec */


void ResetPsDec( HANDLE_PS_DEC pms )  /*!< pointer to the module state */
{
    int i;

    for ( i = 0; i < NO_HI_RES_BINS; i++ )
    {
        pms->h11rPrev[i] = 1.0;
        pms->h12rPrev[i] = 1.0;
    }
    memset( pms->h11iPrev, 0, sizeof( pms->h11iPrev ) );
    memset( pms->h12iPrev, 0, sizeof( pms->h12iPrev ) );
    memset( pms->h21rPrev, 0, sizeof( pms->h21rPrev ) );
    memset( pms->h22rPrev, 0, sizeof( pms->h22rPrev ) );
    memset( pms->h21iPrev, 0, sizeof( pms->h21iPrev ) );
    memset( pms->h22iPrev, 0, sizeof( pms->h22iPrev ) );

    memset (pms->aIpdIndexMapped_1, 0, sizeof(pms->aIpdIndexMapped_1));
    memset (pms->aOpdIndexMapped_1, 0, sizeof(pms->aOpdIndexMapped_1));
    memset (pms->aIpdIndexMapped_2, 0, sizeof(pms->aIpdIndexMapped_2));
    memset (pms->aOpdIndexMapped_2, 0, sizeof(pms->aOpdIndexMapped_2));
}


void ResetPsDeCor( HANDLE_PS_DEC pms )  /*!< pointer to the module state */
{
  int i;

  for (i=0 ; i < NO_HI_RES_BINS ; i++) {
    pms->aPeakDecayFastBin[i] = 0.0f;
    pms->aPrevNrgBin[i] = 0.0f;
    pms->aPrevPeakDiffBin[i] = 0.0f;
  }

  clearMatrix((void**)pms->aaRealDelayBufferQmf, pms->noSampleDelay, pms->noChannels, sizeof(double));
  clearMatrix((void**)pms->aaImagDelayBufferQmf, pms->noSampleDelay, pms->noChannels, sizeof(double));
  clearMatrix((void**)pms->aaRealDelayBufferSubQmf, pms->noSampleDelay, NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  clearMatrix((void**)pms->aaImagDelayBufferSubQmf, pms->noSampleDelay, NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {
    clearMatrix((void**)pms->aaaRealDelayRBufferSerQmf[i], pms->aNoSampleDelayRSer[i], pms->noChannels, sizeof(double));
    clearMatrix((void**)pms->aaaImagDelayRBufferSerQmf[i], pms->aNoSampleDelayRSer[i], pms->noChannels, sizeof(double));
    clearMatrix((void**)pms->aaaRealDelayRBufferSerSubQmf[i], pms->aNoSampleDelayRSer[i], NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
    clearMatrix((void**)pms->aaaImagDelayRBufferSerSubQmf[i], pms->aNoSampleDelayRSer[i], NO_SUB_QMF_CHANNELS_HI_RES, sizeof(double));
  }

}


/***************************************************************************/
/*!
  \brief  Deletes one instance of the PS_DEC struct

****************************************************************************/
void
DeletePsDec(HANDLE_PS_DEC * h_ps_d_p)
{
  unsigned int i;
  HANDLE_PS_DEC h_ps_d = *h_ps_d_p;

  if (h_ps_d) {
    /* Free delay buffers */
    if (h_ps_d->aaRealDelayBufferQmf) {
      freeMatrix((void**)h_ps_d->aaRealDelayBufferQmf,h_ps_d->noSampleDelay);
      h_ps_d->aaRealDelayBufferQmf = NULL;
    }
    if (h_ps_d->aaImagDelayBufferQmf) {
      freeMatrix((void**)h_ps_d->aaImagDelayBufferQmf, h_ps_d->noSampleDelay);
      h_ps_d->aaImagDelayBufferQmf = NULL;
    }
    if (h_ps_d->aaRealDelayBufferSubQmf) {
      freeMatrix((void**)h_ps_d->aaRealDelayBufferSubQmf,h_ps_d->noSampleDelay);
      h_ps_d->aaRealDelayBufferSubQmf = NULL;
    }
    if (h_ps_d->aaImagDelayBufferSubQmf) {
      freeMatrix((void**)h_ps_d->aaImagDelayBufferSubQmf, h_ps_d->noSampleDelay);
      h_ps_d->aaImagDelayBufferSubQmf = NULL;
    }

    /* allpass filter */
    for (i=0 ; i<NO_SERIAL_ALLPASS_LINKS ; i++) {
      if (h_ps_d->aaaRealDelayRBufferSerQmf[i]) {
        freeMatrix((void**)h_ps_d->aaaRealDelayRBufferSerQmf[i],h_ps_d->aNoSampleDelayRSer[i]);
        h_ps_d->aaaRealDelayRBufferSerQmf[i] = NULL;
      }
      if (h_ps_d->aaaImagDelayRBufferSerQmf[i]) {
        freeMatrix((void**)h_ps_d->aaaImagDelayRBufferSerQmf[i], h_ps_d->aNoSampleDelayRSer[i]);
        h_ps_d->aaaImagDelayRBufferSerQmf[i] = NULL;
      }
      if (h_ps_d->aaaRealDelayRBufferSerSubQmf[i]) {
        freeMatrix((void**)h_ps_d->aaaRealDelayRBufferSerSubQmf[i],h_ps_d->aNoSampleDelayRSer[i]);
        h_ps_d->aaaRealDelayRBufferSerSubQmf[i] = NULL;
      }
      if (h_ps_d->aaaImagDelayRBufferSerSubQmf[i]) {
        freeMatrix((void**)h_ps_d->aaaImagDelayRBufferSerSubQmf[i], h_ps_d->aNoSampleDelayRSer[i]);
        h_ps_d->aaaImagDelayRBufferSerSubQmf[i] = NULL;
      }
    }

    if ( h_ps_d->aaRealDelayBufferQmf ) {
        freeMatrix((void**)h_ps_d->aaRealDelayBufferQmf, h_ps_d->noSampleDelay);
        h_ps_d->aaRealDelayBufferQmf = NULL;
    }
    if ( h_ps_d->aaImagDelayBufferQmf ) {
        freeMatrix((void**)h_ps_d->aaImagDelayBufferQmf, h_ps_d->noSampleDelay);
        h_ps_d->aaImagDelayBufferQmf = NULL;
    }
    if ( h_ps_d->aaRealDelayBufferSubQmf ) {
        freeMatrix((void**)h_ps_d->aaRealDelayBufferSubQmf, h_ps_d->noSampleDelay);
        h_ps_d->aaRealDelayBufferSubQmf = NULL;
    }
    if ( h_ps_d->aaImagDelayBufferSubQmf ) {
        freeMatrix((void**)h_ps_d->aaImagDelayBufferSubQmf, h_ps_d->noSampleDelay);
        h_ps_d->aaImagDelayBufferSubQmf = NULL;
    }

    if ( h_ps_d->aaFractDelayPhaseFactorReQmf )
    {
        free(h_ps_d->aaFractDelayPhaseFactorReQmf);
        h_ps_d->aaFractDelayPhaseFactorReQmf = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorImQmf )
    {
        free(h_ps_d->aaFractDelayPhaseFactorImQmf);
        h_ps_d->aaFractDelayPhaseFactorImQmf = NULL;
    }

    if ( h_ps_d->aaFractDelayPhaseFactorSerReQmf ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerReQmf, NO_QMF_CHANNELS);
        h_ps_d->aaFractDelayPhaseFactorSerReQmf = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorSerImQmf ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerImQmf, NO_QMF_CHANNELS);
        h_ps_d->aaFractDelayPhaseFactorSerImQmf = NULL;
    }

    if ( h_ps_d->aaFractDelayPhaseFactorReSubQmf20 ) {
        free(h_ps_d->aaFractDelayPhaseFactorReSubQmf20);
        h_ps_d->aaFractDelayPhaseFactorReSubQmf20 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorImSubQmf20 ) {
        free(h_ps_d->aaFractDelayPhaseFactorImSubQmf20);
        h_ps_d->aaFractDelayPhaseFactorImSubQmf20 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20, NO_SUB_QMF_CHANNELS_HI_RES);
        h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20, NO_SUB_QMF_CHANNELS_HI_RES);
        h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 = NULL;
    }

    if ( h_ps_d->aaFractDelayPhaseFactorReSubQmf34 ) {
        free(h_ps_d->aaFractDelayPhaseFactorReSubQmf34);
        h_ps_d->aaFractDelayPhaseFactorReSubQmf34 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorImSubQmf34 ) {
        free(h_ps_d->aaFractDelayPhaseFactorImSubQmf34);
        h_ps_d->aaFractDelayPhaseFactorImSubQmf34 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34, NO_SUB_QMF_CHANNELS_HI_RES);
        h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 = NULL;
    }
    if ( h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 ) {
        freeMatrix((void**)h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34, NO_SUB_QMF_CHANNELS_HI_RES);
        h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 = NULL;
    }

    /* Free hybrid filterbank. */
    for (i = 0; i < h_ps_d->noSubSamplesInit; i++) {
      if(h_ps_d->mHybridRealLeft[i])
        free(h_ps_d->mHybridRealLeft[i]);
      if(h_ps_d->mHybridImagLeft[i])
        free(h_ps_d->mHybridImagLeft[i]);
      if(h_ps_d->mHybridRealRight[i])
        free(h_ps_d->mHybridRealRight[i]);
      if(h_ps_d->mHybridImagRight[i])
        free(h_ps_d->mHybridImagRight[i]);
    }

    if(h_ps_d->mHybridRealLeft)
      free(h_ps_d->mHybridRealLeft);
    if(h_ps_d->mHybridImagLeft)
      free(h_ps_d->mHybridImagLeft);
    if(h_ps_d->mHybridRealRight)
      free(h_ps_d->mHybridRealRight);
    if(h_ps_d->mHybridImagRight)
      free(h_ps_d->mHybridImagRight);

    DeleteHybridFilterBank ( &h_ps_d->hHybrid20 );
    DeleteHybridFilterBank ( &h_ps_d->hHybrid34 );

    free(*h_ps_d_p);
    *h_ps_d_p = NULL;
  }
}/*END DeletePsDec*/


/***************************************************************************/
/*!
  \brief  Applies IID, ICC, IPD and OPD parameters to the current frame.

  \return none

****************************************************************************/
void
ApplyPsFrame(HANDLE_PS_DEC h_ps_d,  /*!< */
             double **rIntBufferLeft,      /*!< input, modified */
             double **iIntBufferLeft,      /*!< input, modified */
             double **rIntBufferRight,     /*!< output */
             double **iIntBufferRight)     /*!< output */
{

  if (h_ps_d->aOcsParams[0].bValidFrame) {

      h_ps_d->bUse34StereoBands = h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS34;

      /* First chose hybrid filterbank. 10/20 or 34 band case */
      if (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS34){
        h_ps_d->pGroupBorders = groupBorders34;
        h_ps_d->pBins2groupMap = bins2groupMap34;
        h_ps_d->pHybrid = h_ps_d->hHybrid34;
        h_ps_d->noGroups = NO_IID_GROUPS_HI_RES;
        h_ps_d->noSubQmfGroups = SUBQMF_GROUPS_HI_RES;
        h_ps_d->noBins = NO_HI_RES_BINS;
        h_ps_d->firstDelayGr = h_ps_d->firstDelayGr34;
      }
      else {
        h_ps_d->pGroupBorders = groupBorders20;
        h_ps_d->pBins2groupMap = bins2groupMap20;
        h_ps_d->pHybrid = h_ps_d->hHybrid20;
        h_ps_d->noGroups = NO_IID_GROUPS;
        h_ps_d->noSubQmfGroups = SUBQMF_GROUPS;
        h_ps_d->noBins = NO_MID_RES_BINS;
        h_ps_d->firstDelayGr = h_ps_d->firstDelayGr20;
      }

  /* update both hybrid filterbank states to enable dynamic switching */
      HybridAnalysis ( (const double**)rIntBufferLeft,
                       (const double**)iIntBufferLeft,
                       (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS20)?h_ps_d->mHybridRealLeft:(double**)NULL,
                       h_ps_d->mHybridImagLeft,
                       h_ps_d->hHybrid20,
                       MODE_BANDS20 );
      HybridAnalysis ( (const double**)rIntBufferLeft,
                       (const double**)iIntBufferLeft,
                       (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS34)?h_ps_d->mHybridRealLeft:(double**)NULL,
                       h_ps_d->mHybridImagLeft,
                       h_ps_d->hHybrid34,
                       MODE_BANDS34 );
      /* Do this for 20 bands */
      if (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS20){
        /* group hybrid channels 3+4 -> 3 and 2+5 -> 2 */
        int k;
        for (k=0; k<(int)h_ps_d->noSubSamples; k++) {
          h_ps_d->mHybridRealLeft[k][3] += h_ps_d->mHybridRealLeft[k][4];
          h_ps_d->mHybridImagLeft[k][3] += h_ps_d->mHybridImagLeft[k][4];
          h_ps_d->mHybridRealLeft[k][4] = 0.;
          h_ps_d->mHybridImagLeft[k][4] = 0.;

          h_ps_d->mHybridRealLeft[k][2] += h_ps_d->mHybridRealLeft[k][5];
          h_ps_d->mHybridImagLeft[k][2] += h_ps_d->mHybridImagLeft[k][5];
          h_ps_d->mHybridRealLeft[k][5] = 0.;
          h_ps_d->mHybridImagLeft[k][5] = 0.;
          if (h_ps_d->bUse34StereoBandsPrev)
          {
            h_ps_d->mHybridRealRight[k][4] = h_ps_d->mHybridRealRight[k][5] = 0.;
            h_ps_d->mHybridImagRight[k][4] = h_ps_d->mHybridImagRight[k][5] = 0.;
          }
        }
      }


      if (h_ps_d->psMode & 0x0080) {
        /* copy m -> l=r */
        int i, j;
        for ( i = 0; i < MAX_NUM_COL; i++ ) {
          for ( j = 0; j < NO_QMF_CHANNELS; j++ ) {
            iIntBufferRight[i][j] = iIntBufferLeft[i][j];
            rIntBufferRight[i][j] = rIntBufferLeft[i][j];
          }
        }
        for ( i = 0; i < MAX_NUM_COL; i++ ) {
          for ( j = 0; j < NO_SUB_QMF_CHANNELS_HI_RES; j++ ) {
            h_ps_d->mHybridRealRight[i][j] = h_ps_d->mHybridRealLeft[i][j];
            h_ps_d->mHybridImagRight[i][j] = h_ps_d->mHybridImagLeft[i][j];
          }
        }
      }
      else {

        if (h_ps_d->psMode & 0x0002) {
          /* no ICC, just clear right channel */
          int i, j;
          for ( i = 0; i < MAX_NUM_COL; i++ ) {
            for ( j = 0; j < NO_QMF_CHANNELS; j++ ) {
              iIntBufferRight[i][j] = 0.;
              rIntBufferRight[i][j] = 0.;
            }
          }
          for ( i = 0; i < MAX_NUM_COL; i++ ) {
            for ( j = 0; j < NO_SUB_QMF_CHANNELS_HI_RES; j++ ) {
              h_ps_d->mHybridRealRight[i][j] = 0.;
              h_ps_d->mHybridImagRight[i][j] = 0.;
            }
          }
        }
        else {

          deCorrelate(h_ps_d,
                      rIntBufferLeft,
                      iIntBufferLeft,
                      rIntBufferRight,
                      iIntBufferRight);

          /* Zero the first three qmf samples for each slot */
          /* To maintain compatibility with SSC def. (last 3 reverb bands) */
          if ( h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS20)
          {
              unsigned int i, j;
              for ( i = 0; i < h_ps_d->noSubSamples; i++ ) {
                for ( j = 0; j < 3; j++ ) {
                  iIntBufferRight[i][j] = (double)0.;
                  rIntBufferRight[i][j] = (double)0.;
                }
              }
          }
        }



        if (!(h_ps_d->psMode & 0x0040)) {

          applyRotation(h_ps_d,
                        rIntBufferLeft,
                        iIntBufferLeft,
                        rIntBufferRight,
                        iIntBufferRight);
        }
      }

      HybridSynthesis ( (const double**)h_ps_d->mHybridRealLeft,
                        (const double**)h_ps_d->mHybridImagLeft,
                        rIntBufferLeft,
                        iIntBufferLeft,
                        h_ps_d->pHybrid );


      HybridSynthesis ( (const double**)h_ps_d->mHybridRealRight,
                        (const double**)h_ps_d->mHybridImagRight,
                        rIntBufferRight,
                        iIntBufferRight,
                        h_ps_d->pHybrid );


  }

  /* track freq. res. switching */
  h_ps_d->bUse34StereoBandsPrev = h_ps_d->bUse34StereoBands;

  /* Shift OCS parameter pipe by one slot */
  if (h_ps_d->pipe_length > 1)
  {
      memmove( &h_ps_d->aOcsParams[0], &h_ps_d->aOcsParams[1], ( h_ps_d->pipe_length - 1 ) * sizeof( OCS_PARAMETERS ) );
  }
  /* Reset parser target */
  h_ps_d->parsertarget = DELAY;


}/* END ApplyPsFrame */


/***************************************************************************/
/*!
  \brief  Generate decorrelated side channel using allpass/delay

****************************************************************************/
static void
deCorrelate(HANDLE_PS_DEC h_ps_d, /*!< */
            double **rIntBufferLeft, /*!< Pointer to mid channel real part of QMF matrix*/
            double **iIntBufferLeft, /*!< Pointer to mid channel imag part of QMF matrix*/
            double **rIntBufferRight,/*!< Pointer to side channel real part of QMF matrix*/
            double **iIntBufferRight /*!< Pointer to side channel imag part of QMF matrix*/
            )
{
  int sb, maxsb, gr, k;
  int m;
  int tempDelay = 0;
  int aTempDelayRSer[NO_SERIAL_ALLPASS_LINKS];
  double iInputLeft;
  double rInputLeft;
  double peakDiff, nrg, transRatio;

  double **aaLeftReal;
  double **aaLeftImag;
  double **aaRightReal;
  double **aaRightImag;

  double ***pppRealDelayRBufferSer;
  double ***pppImagDelayRBufferSer;
  double **ppRealDelayBuffer;
  double **ppImagDelayBuffer;

  double *ppFractDelayPhaseFactorRe;
  double *ppFractDelayPhaseFactorIm;
  double **ppFractDelayPhaseFactorSerRe;
  double **ppFractDelayPhaseFactorSerIm;

  int *pNoSampleDelayDelay;
  int *pDelayBufIndexDelay;

  double aaPower[32][NO_HI_RES_BINS];
  double aaTransRatio[32][NO_HI_RES_BINS];
  int bin;

  pDelayBufIndexDelay = h_ps_d->aDelayRBufIndexSer;
  pNoSampleDelayDelay = h_ps_d->aNoSampleDelayRSer;

  aaLeftReal = h_ps_d->mHybridRealLeft;
  aaLeftImag = h_ps_d->mHybridImagLeft;
  aaRightReal = h_ps_d->mHybridRealRight;
  aaRightImag = h_ps_d->mHybridImagRight;

  pppRealDelayRBufferSer = h_ps_d->aaaRealDelayRBufferSerSubQmf;
  pppImagDelayRBufferSer = h_ps_d->aaaImagDelayRBufferSerSubQmf;

  ppRealDelayBuffer = h_ps_d->aaRealDelayBufferSubQmf;
  ppImagDelayBuffer = h_ps_d->aaImagDelayBufferSubQmf;

  /* map state variables in case of freq. res. switching */
  if (h_ps_d->bUse34StereoBands != h_ps_d->bUse34StereoBandsPrev) {
    if (h_ps_d->bUse34StereoBands) {
      /* map 20->34 bands */
      /* reset decorrelator state (instead of mapping) */
      ResetPsDeCor(h_ps_d);
    }
    else {
      /* map 34->20 bands */
      /* reset decorrelator state (instead of mapping) */
      ResetPsDeCor(h_ps_d);
    }
  }

  /* chose hybrid filterbank: 20 or 34 band case */
  if (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS34){
    ppFractDelayPhaseFactorRe = h_ps_d->aaFractDelayPhaseFactorReSubQmf34;
    ppFractDelayPhaseFactorIm = h_ps_d->aaFractDelayPhaseFactorImSubQmf34;
    ppFractDelayPhaseFactorSerRe = h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34;
    ppFractDelayPhaseFactorSerIm = h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34;
  }
  else {
    ppFractDelayPhaseFactorRe = h_ps_d->aaFractDelayPhaseFactorReSubQmf20;
    ppFractDelayPhaseFactorIm = h_ps_d->aaFractDelayPhaseFactorImSubQmf20;
    ppFractDelayPhaseFactorSerRe = h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20;
    ppFractDelayPhaseFactorSerIm = h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20;
  }

  for (k=0; k < 32; k++) {
    for (bin=0; bin < NO_HI_RES_BINS; bin++) {
      aaPower[k][bin] = 0;
    }
  }

  for (gr=0; gr < h_ps_d->noGroups; gr++) {
    bin = ( ~NEGATE_IPD_MASK ) & h_ps_d->pBins2groupMap[gr];
    if (gr == h_ps_d->noSubQmfGroups) {
      aaLeftReal = rIntBufferLeft;
      aaLeftImag = iIntBufferLeft;
      aaRightReal = rIntBufferRight;
      aaRightImag = iIntBufferRight;
    }
    maxsb = (gr < h_ps_d->noSubQmfGroups)? h_ps_d->pGroupBorders[gr]+1: h_ps_d->pGroupBorders[gr+1];

    /* sb = subQMF/QMF subband */
    for (sb = h_ps_d->pGroupBorders[gr]; sb < maxsb; sb++) {

      /* k = subsample */
      for (k=h_ps_d->aOcsParams[0].aEnvStartStop[0] ; k < h_ps_d->aOcsParams[0].aEnvStartStop[ h_ps_d->aOcsParams[0].noEnv] ; k++) {
        rInputLeft = aaLeftReal[k][sb];
        iInputLeft = aaLeftImag[k][sb];
        aaPower[k][bin] += rInputLeft*rInputLeft + iInputLeft*iInputLeft;
      } /* k */
    } /* sb */
  } /* gr */

  aaLeftReal = h_ps_d->mHybridRealLeft;
  aaLeftImag = h_ps_d->mHybridImagLeft;
  aaRightReal = h_ps_d->mHybridRealRight;
  aaRightImag = h_ps_d->mHybridImagRight;


  for (bin=0; bin < h_ps_d->noBins; bin++) {
    for (k=h_ps_d->aOcsParams[0].aEnvStartStop[0] ; k < h_ps_d->aOcsParams[0].aEnvStartStop[ h_ps_d->aOcsParams[0].noEnv] ; k++) {
      double q=1.5;

      h_ps_d->aPeakDecayFastBin[bin] *= h_ps_d->PeakDecayFactorFast;
      if (h_ps_d->aPeakDecayFastBin[bin] < aaPower[k][bin])
        h_ps_d->aPeakDecayFastBin[bin] = aaPower[k][bin];

      /* filter(aPeakDecayFast-power) */
      peakDiff = h_ps_d->aPrevPeakDiffBin[bin];
      peakDiff += h_ps_d->intFilterCoeff *
        (h_ps_d->aPeakDecayFastBin[bin] - aaPower[k][bin] - h_ps_d->aPrevPeakDiffBin[bin]);
      h_ps_d->aPrevPeakDiffBin[bin] = peakDiff;

      /* filter(power) */
      nrg = h_ps_d->aPrevNrgBin[bin];
      nrg += h_ps_d->intFilterCoeff *
        (aaPower[k][bin] - h_ps_d->aPrevNrgBin[bin]);
      h_ps_d->aPrevNrgBin[bin] = nrg;
      if (q*peakDiff <= nrg){
        aaTransRatio[k][bin] = 1.0;
      }
      else{
        aaTransRatio[k][bin] = nrg / (q*peakDiff);
      }
    } /* k */
  } /* gr */


  /* gr = ICC group */
  for (gr=0; gr < h_ps_d->noGroups; gr++) {
    if (gr == h_ps_d->noSubQmfGroups) {
      aaLeftReal = rIntBufferLeft;
      aaLeftImag = iIntBufferLeft;
      aaRightReal = rIntBufferRight;
      aaRightImag = iIntBufferRight;

      pppRealDelayRBufferSer = h_ps_d->aaaRealDelayRBufferSerQmf;
      pppImagDelayRBufferSer = h_ps_d->aaaImagDelayRBufferSerQmf;

      ppRealDelayBuffer = h_ps_d->aaRealDelayBufferQmf;
      ppImagDelayBuffer = h_ps_d->aaImagDelayBufferQmf;

      ppFractDelayPhaseFactorRe = h_ps_d->aaFractDelayPhaseFactorReQmf;
      ppFractDelayPhaseFactorIm = h_ps_d->aaFractDelayPhaseFactorImQmf;

      ppFractDelayPhaseFactorSerRe = h_ps_d->aaFractDelayPhaseFactorSerReQmf;
      ppFractDelayPhaseFactorSerIm = h_ps_d->aaFractDelayPhaseFactorSerImQmf;

      pDelayBufIndexDelay = h_ps_d->aDelayBufIndexDelayQmf;
      pNoSampleDelayDelay = h_ps_d->aNoSampleDelayDelayQmf;
    }

    maxsb = (gr < h_ps_d->noSubQmfGroups)? h_ps_d->pGroupBorders[gr]+1: h_ps_d->pGroupBorders[gr+1];

    /* sb = subQMF/QMF subband */
    for (sb = h_ps_d->pGroupBorders[gr]; sb < maxsb; sb++) {
      double decayScaleFactor;
      double decay_cutoff;

      /* Determine decay cutoff factor */
      if (h_ps_d->aOcsParams[0].usedStereoBands == MODE_BANDS20) /* 10/20 bans configuration */
          decay_cutoff = DECAY_CUTOFF;
      else        /* 34 bands configuarion */
          decay_cutoff = DECAY_CUTOFF_HI_RES;

      if (gr < h_ps_d->noSubQmfGroups || sb <= DECAY_CUTOFF)
        decayScaleFactor = 1.0;
      else
        decayScaleFactor = 1.0 + decay_cutoff * DECAY_SLOPE - DECAY_SLOPE * sb;

      decayScaleFactor = max(decayScaleFactor, 0.0);

      /* set delay indices */
      tempDelay = h_ps_d->delayBufIndex;
      for (k=0; k<NO_SERIAL_ALLPASS_LINKS ; k++)
        aTempDelayRSer[k] = h_ps_d->aDelayRBufIndexSer[k];

      /* k = subsample */
      for (k=h_ps_d->aOcsParams[0].aEnvStartStop[0] ; k < h_ps_d->aOcsParams[0].aEnvStartStop[ h_ps_d->aOcsParams[0].noEnv] ; k++) {
        double rTmp, iTmp, rTmp0, iTmp0, rR0, iR0;

        rInputLeft = aaLeftReal[k][sb];
        iInputLeft = aaLeftImag[k][sb];

        if (gr >= h_ps_d->firstDelayGr &&
            sb >= h_ps_d->firstDelaySb &&
            gr >= h_ps_d->noSubQmfGroups) {
          /* Delay */
          /* Update delay buffers */
          rTmp = ppRealDelayBuffer[pDelayBufIndexDelay[sb]][sb];
          iTmp = ppImagDelayBuffer[pDelayBufIndexDelay[sb]][sb];
          rR0 = rTmp;
          iR0 = iTmp;
          ppRealDelayBuffer[pDelayBufIndexDelay[sb]][sb] = rInputLeft;
          ppImagDelayBuffer[pDelayBufIndexDelay[sb]][sb] = iInputLeft;
        }
        else {
          /* allpass filter */
          /* Update delay buffers */
          rTmp0 = ppRealDelayBuffer[tempDelay][sb];
          iTmp0 = ppImagDelayBuffer[tempDelay][sb];
          ppRealDelayBuffer[tempDelay][sb] = rInputLeft;
          ppImagDelayBuffer[tempDelay][sb] = iInputLeft;

          /* delay by fraction */
          rTmp = rTmp0*ppFractDelayPhaseFactorRe[sb] - iTmp0*ppFractDelayPhaseFactorIm[sb];
          iTmp = rTmp0*ppFractDelayPhaseFactorIm[sb] + iTmp0*ppFractDelayPhaseFactorRe[sb];

          rR0 = rTmp;
          iR0 = iTmp;
          for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++) {
            rTmp0 = pppRealDelayRBufferSer[m][aTempDelayRSer[m]][sb];
            iTmp0 = pppImagDelayRBufferSer[m][aTempDelayRSer[m]][sb];
            /* delay by fraction */
            rTmp = rTmp0*ppFractDelayPhaseFactorSerRe[sb][m] - iTmp0*ppFractDelayPhaseFactorSerIm[sb][m];
            iTmp = rTmp0*ppFractDelayPhaseFactorSerIm[sb][m] + iTmp0*ppFractDelayPhaseFactorSerRe[sb][m];

            rTmp += -decayScaleFactor * aAllpassLinkDecaySer[m] * rR0;
            iTmp += -decayScaleFactor * aAllpassLinkDecaySer[m] * iR0;
            pppRealDelayRBufferSer[m][aTempDelayRSer[m]][sb] = rR0 + decayScaleFactor * aAllpassLinkDecaySer[m] * rTmp;
            pppImagDelayRBufferSer[m][aTempDelayRSer[m]][sb] = iR0 + decayScaleFactor * aAllpassLinkDecaySer[m] * iTmp;
            rR0 = rTmp;
            iR0 = iTmp;
          }
        }

        bin = ( ~NEGATE_IPD_MASK ) & h_ps_d->pBins2groupMap[gr];
        transRatio = aaTransRatio[k][bin];

        /* duck if a past transient is found */
        aaRightReal[k][sb] = transRatio * rR0;
        aaRightImag[k][sb] = transRatio * iR0;

        /* Update delay buffer index */
        if (++tempDelay >= h_ps_d->noSampleDelayAllpass)
          tempDelay = 0;

        if (gr >= h_ps_d->firstDelayGr &&
            sb >= h_ps_d->firstDelaySb &&
            gr >= h_ps_d->noSubQmfGroups){
          if (++pDelayBufIndexDelay[sb] >= pNoSampleDelayDelay[sb]){
            pDelayBufIndexDelay[sb] = 0;
          }
        }

        for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++) {
          if (++aTempDelayRSer[m] >= h_ps_d->aNoSampleDelayRSer[m])
            aTempDelayRSer[m] = 0;
        }
      } /* k */
    } /* ch */
  } /* gr */

  h_ps_d->delayBufIndex = tempDelay;
  for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++)
    h_ps_d->aDelayRBufIndexSer[m] = aTempDelayRSer[m];
}


/***************************************************************************/
/*!
  \brief  Generates left and right signals

****************************************************************************/
static void
applyRotation( HANDLE_PS_DEC   pms,          /*!< pointer to the moodule state */
               double            **qmfLeftReal,  /*!< pointer to left channel real part of QMF matrix*/
               double            **qmfLeftImag,  /*!< pointer to left channel imag part of QMF matrix*/
               double            **qmfRightReal, /*!< pointer to right channel real part of QMF matrix*/
               double            **qmfRightImag  /*!< pointer to right channel imag part of QMF matrix*/
             )
{
    int     i;
    int     group;
    int     bin;
    int     subband, maxSubband;
    int     env;
    double **hybrLeftReal;
    double **hybrLeftImag;
    double **hybrRightReal;
    double **hybrRightImag;
    double   scaleL, scaleR;
    double   alpha, beta;
    double   ipd, opd;
    double   ipd0, opd0;
    double   ipd1, opd1;
    double   ipd2, opd2;
    double   ipds, opds;
    double   h11r, h12r, h21r, h22r;
    double   h11i, h12i, h21i, h22i;
    double   H11r, H12r, H21r, H22r;
    double   H11i, H12i, H21i, H22i;
    double   deltaH11r, deltaH12r, deltaH21r, deltaH22r;
    double   deltaH11i, deltaH12i, deltaH21i, deltaH22i;
    double   tempLeftReal, tempLeftImag;
    double   tempRightReal, tempRightImag;
    int      ipdBins;
    int      L;
    double  *pScaleFactors;
    const int *pQuantizedIIDs;
    int     noIidSteps;

    if (pms->aOcsParams[0].bFineIidQ)
    {
      noIidSteps = NO_IID_STEPS_FINE;
      pScaleFactors = scaleFactorsFine;
      pQuantizedIIDs = quantizedIIDsFine;
    }
    else{
      noIidSteps = NO_IID_STEPS;
      pScaleFactors = scaleFactors;
      pQuantizedIIDs = quantizedIIDs;
    }

    /* Check on half resolution */
    if (pms->aOcsParams[0].freqResIpd == 0)
    {
      ipdBins = aNoIpdBins[1];
    }
    else
    {
      ipdBins = aNoIpdBins[pms->aOcsParams[0].freqResIpd];
    }
    if (!pms->aOcsParams[0].bEnableIpdOpd) ipdBins = 0;

  /* map previous h and ipd/opd in case of freq. res. switching */
    if (pms->bUse34StereoBands != pms->bUse34StereoBandsPrev) {
    if (pms->bUse34StereoBands) {
      /* map 20->34 bands */
      map20FloatTo34(pms->h11rPrev);
      map20FloatTo34(pms->h12rPrev);
      map20FloatTo34(pms->h21rPrev);
      map20FloatTo34(pms->h22rPrev);

      map20FloatTo34(pms->h11iPrev);
      map20FloatTo34(pms->h12iPrev);
      map20FloatTo34(pms->h21iPrev);
      map20FloatTo34(pms->h22iPrev);
      /* reset ipd/opd smoother history (instead of mapping) */
      memset (pms->aIpdIndexMapped_1, 0, sizeof(pms->aIpdIndexMapped_1));
      memset (pms->aOpdIndexMapped_1, 0, sizeof(pms->aOpdIndexMapped_1));
      memset (pms->aIpdIndexMapped_2, 0, sizeof(pms->aIpdIndexMapped_2));
      memset (pms->aOpdIndexMapped_2, 0, sizeof(pms->aOpdIndexMapped_2));
    }
    else {
      /* map 34->20 bands */
      map34FloatTo20(pms->h11rPrev);
      map34FloatTo20(pms->h12rPrev);
      map34FloatTo20(pms->h21rPrev);
      map34FloatTo20(pms->h22rPrev);

      map34FloatTo20(pms->h11iPrev);
      map34FloatTo20(pms->h12iPrev);
      map34FloatTo20(pms->h21iPrev);
      map34FloatTo20(pms->h22iPrev);
      /* reset ipd/opd smoother history (instead of mapping) */
      memset (pms->aIpdIndexMapped_1, 0, sizeof(pms->aIpdIndexMapped_1));
      memset (pms->aOpdIndexMapped_1, 0, sizeof(pms->aOpdIndexMapped_1));
      memset (pms->aIpdIndexMapped_2, 0, sizeof(pms->aIpdIndexMapped_2));
      memset (pms->aOpdIndexMapped_2, 0, sizeof(pms->aOpdIndexMapped_2));
    }
  }

  for ( env = 0; env < pms->aOcsParams[0].noEnv; env++ ) {

    /* dequantize and decode */
    for ( bin = 0; bin < pms->noBins; bin++ ) {

      if (!pms->aOcsParams[0].bUsePcaRot) {
        /* type 'A' rotation */
        scaleR = pScaleFactors[noIidSteps + pms->aOcsParams[0].aaIidIndexMapped[env][bin]];
        scaleL = pScaleFactors[noIidSteps - pms->aOcsParams[0].aaIidIndexMapped[env][bin]];

        alpha  = alphas[pms->aOcsParams[0].aaIccIndexMapped[env][bin]];

        beta   = alpha * ( scaleR - scaleL ) / PSC_SQRT2F;

        h11r = scaleL * cos( beta + alpha );
        h12r = scaleR * cos( beta - alpha );
        h21r = scaleL * sin( beta + alpha );
        h22r = scaleR * sin( beta - alpha );
      }
      else {
        /* type 'B' rotation */
        double c, rho, mu, alpha, gamma;
        int i;

        i = pms->aOcsParams[0].aaIidIndexMapped[env][bin];
        c = pow(10.0,
                ((i)?(((i>0)?1:-1)*pQuantizedIIDs[((i>0)?i:-i)-1]):0.0)/20.0);
        rho = quantizedRHOs[pms->aOcsParams[0].aaIccIndexMapped[env][bin]];
        rho = max(rho, 0.05);

        if (rho==0.0 && c==1.0) {
          alpha = SSC_PI/4.0;
        }
        else {
          if (rho<=0.05) {
            rho = 0.05;
          }
          alpha = 0.5*atan((2.0*c*rho)/(c*c-1.0));

          if (alpha<0.0) {
            alpha += SSC_PI/2.0;
          }
        }
        mu = c+1.0/c;
        mu = 1.0+(4.0*rho*rho-4.0)/(mu*mu);
        gamma = atan(sqrt((1.0-sqrt(mu))/(1.0+sqrt(mu))));
          
        h11r = sqrt(2.0) *  cos(alpha) * cos(gamma);
        h12r = sqrt(2.0) *  sin(alpha) * cos(gamma);
        h21r = sqrt(2.0) * -sin(alpha) * sin(gamma);
        h22r = sqrt(2.0) *  cos(alpha) * sin(gamma);
      }


      if ( bin >= ipdBins ) {
        h11i = h12i = h21i = h22i = 0.0;
      }
      else {
        /* ipd/opd smoothing */
        ipd0 = ( IPD_SCALE_FACTOR * 2.0 ) * pms->aOcsParams[0].aaIpdIndexMapped[env][bin];
        opd0 = ( OPD_SCALE_FACTOR * 2.0 ) * pms->aOcsParams[0].aaOpdIndexMapped[env][bin];
        ipd1 = ( IPD_SCALE_FACTOR * 2.0 ) * pms->aIpdIndexMapped_1[bin];
        opd1 = ( OPD_SCALE_FACTOR * 2.0 ) * pms->aOpdIndexMapped_1[bin];
        ipd2 = ( IPD_SCALE_FACTOR * 2.0 ) * pms->aIpdIndexMapped_2[bin];
        opd2 = ( OPD_SCALE_FACTOR * 2.0 ) * pms->aOpdIndexMapped_2[bin];

        tempLeftReal  = cos(ipd0);
        tempLeftImag  = sin(ipd0);
        tempRightReal = cos(opd0);
        tempRightImag = sin(opd0);

        tempLeftReal  += PHASE_SMOOTH_HIST1 * cos(ipd1);
        tempLeftImag  += PHASE_SMOOTH_HIST1 * sin(ipd1);
        tempRightReal += PHASE_SMOOTH_HIST1 * cos(opd1);
        tempRightImag += PHASE_SMOOTH_HIST1 * sin(opd1);

        tempLeftReal  += PHASE_SMOOTH_HIST2 * cos(ipd2);
        tempLeftImag  += PHASE_SMOOTH_HIST2 * sin(ipd2);
        tempRightReal += PHASE_SMOOTH_HIST2 * cos(opd2);
        tempRightImag += PHASE_SMOOTH_HIST2 * sin(opd2);

        ipd = ipds = atan2( tempLeftImag , tempLeftReal  );
        opd = opds = atan2( tempRightImag, tempRightReal );

        /* phase rotation */
        tempLeftReal  = cos( opd );
        tempLeftImag  = sin( opd );
        opd -= ipd;
        tempRightReal = cos( opd );
        tempRightImag = sin( opd );

        h11i  = h11r * tempLeftImag;
        h12i  = h12r * tempRightImag;
        h21i  = h21r * tempLeftImag;
        h22i  = h22r * tempRightImag;

        h11r *= tempLeftReal;
        h12r *= tempRightReal;
        h21r *= tempLeftReal;
        h22r *= tempRightReal;
      }

      pms->h11rCurr[bin] = h11r;
      pms->h12rCurr[bin] = h12r;
      pms->h21rCurr[bin] = h21r;
      pms->h22rCurr[bin] = h22r;
      pms->h11iCurr[bin] = h11i;
      pms->h12iCurr[bin] = h12i;
      pms->h21iCurr[bin] = h21i;
      pms->h22iCurr[bin] = h22i;

    } /* bin loop */


    /* loop thru all groups ... */

    hybrLeftReal  = pms->mHybridRealLeft;
    hybrLeftImag  = pms->mHybridImagLeft;
    hybrRightReal = pms->mHybridRealRight;
    hybrRightImag = pms->mHybridImagRight;

    for ( group = 0; group < pms->noGroups; group++ ) {
      bin = ( ~NEGATE_IPD_MASK ) & pms->pBins2groupMap[group];

      if ( group == pms->noSubQmfGroups ) {
        /* continue in the qmf buffers */
        hybrLeftReal  = qmfLeftReal;
        hybrLeftImag  = qmfLeftImag;
        hybrRightReal = qmfRightReal;
        hybrRightImag = qmfRightImag;
      }

      /* use one channel per group in the subqmf domain */
      maxSubband = ( group < pms->noSubQmfGroups )? pms->pGroupBorders[group] + 1: pms->pGroupBorders[group + 1];

      /* length of the envelope (in time samples) */
      L = pms->aOcsParams[0].aEnvStartStop[env + 1] - pms->aOcsParams[0].aEnvStartStop[env];

      H11r  = pms->h11rPrev[bin];
      H12r  = pms->h12rPrev[bin];
      H21r  = pms->h21rPrev[bin];
      H22r  = pms->h22rPrev[bin];
      if ( ( NEGATE_IPD_MASK & pms->pBins2groupMap[group] ) != 0 ) {
        H11i  = -pms->h11iPrev[bin];
        H12i  = -pms->h12iPrev[bin];
        H21i  = -pms->h21iPrev[bin];
        H22i  = -pms->h22iPrev[bin];
      }
      else {
        H11i  = pms->h11iPrev[bin];
        H12i  = pms->h12iPrev[bin];
        H21i  = pms->h21iPrev[bin];
        H22i  = pms->h22iPrev[bin];
      }

      h11r  = pms->h11rCurr[bin];
      h12r  = pms->h12rCurr[bin];
      h21r  = pms->h21rCurr[bin];
      h22r  = pms->h22rCurr[bin];
      if ( ( NEGATE_IPD_MASK & pms->pBins2groupMap[group] ) != 0 ) {
        h11i  = -pms->h11iCurr[bin];
        h12i  = -pms->h12iCurr[bin];
        h21i  = -pms->h21iCurr[bin];
        h22i  = -pms->h22iCurr[bin];
      }
      else {
        h11i  = pms->h11iCurr[bin];
        h12i  = pms->h12iCurr[bin];
        h21i  = pms->h21iCurr[bin];
        h22i  = pms->h22iCurr[bin];
      }

      deltaH11r  = ( h11r - H11r ) / L;
      deltaH12r  = ( h12r - H12r ) / L;
      deltaH21r  = ( h21r - H21r ) / L;
      deltaH22r  = ( h22r - H22r ) / L;

      deltaH11i  = ( h11i - H11i ) / L;
      deltaH12i  = ( h12i - H12i ) / L;
      deltaH21i  = ( h21i - H21i ) / L;
      deltaH22i  = ( h22i - H22i ) / L;

      for ( i = pms->aOcsParams[0].aEnvStartStop[env]; i < pms->aOcsParams[0].aEnvStartStop[env + 1]; i++ ) {
        H11r += deltaH11r;
        H12r += deltaH12r;
        H21r += deltaH21r;
        H22r += deltaH22r;

        H11i += deltaH11i;
        H12i += deltaH12i;
        H21i += deltaH21i;
        H22i += deltaH22i;

        /* channel is an alias to the subband */
        for ( subband = pms->pGroupBorders[group]; subband < maxSubband; subband++ ) {
          tempLeftReal  = H11r * hybrLeftReal[i][subband]  - H11i * hybrLeftImag[i][subband] +
                          H21r * hybrRightReal[i][subband] - H21i * hybrRightImag[i][subband];

          tempLeftImag  = H11i * hybrLeftReal[i][subband]  + H11r * hybrLeftImag[i][subband] +
                          H21i * hybrRightReal[i][subband] + H21r * hybrRightImag[i][subband];

          tempRightReal = H12r * hybrLeftReal[i][subband]  - H12i * hybrLeftImag[i][subband] +
                          H22r * hybrRightReal[i][subband] - H22i * hybrRightImag[i][subband];

          tempRightImag = H12i * hybrLeftReal[i][subband]  + H12r * hybrLeftImag[i][subband] +
                          H22i * hybrRightReal[i][subband] + H22r * hybrRightImag[i][subband];

          hybrLeftReal [i][subband] = tempLeftReal;
          hybrLeftImag [i][subband] = tempLeftImag;
          hybrRightReal[i][subband] = tempRightReal;
          hybrRightImag[i][subband] = tempRightImag;
        } /* subband loop */
      } /* i (time sample) loop */
    } /* groups loop */

    /* update prev */
    for ( bin = 0; bin < pms->noBins; bin++ ) {
      pms->h11rPrev[bin] = pms->h11rCurr[bin];
      pms->h12rPrev[bin] = pms->h12rCurr[bin];
      pms->h21rPrev[bin] = pms->h21rCurr[bin];
      pms->h22rPrev[bin] = pms->h22rCurr[bin];

      pms->h11iPrev[bin] = pms->h11iCurr[bin];
      pms->h12iPrev[bin] = pms->h12iCurr[bin];
      pms->h21iPrev[bin] = pms->h21iCurr[bin];
      pms->h22iPrev[bin] = pms->h22iCurr[bin];

    }

    for ( bin = 0; bin < ipdBins; bin++ ) {
      pms->aIpdIndexMapped_2[bin] = pms->aIpdIndexMapped_1[bin];
      pms->aOpdIndexMapped_2[bin] = pms->aOpdIndexMapped_1[bin];
      pms->aIpdIndexMapped_1[bin] = pms->aOcsParams[0].aaIpdIndexMapped[env][bin];
      pms->aOpdIndexMapped_1[bin] = pms->aOcsParams[0].aaOpdIndexMapped[env][bin];
    }

  } /* envelopes loop */
}


int
initPsDec(HANDLE_PS_DEC *hParametricStereoDec, int noSubSamples)
{
  return CreatePsDec(hParametricStereoDec,
                     0,
                     NO_QMF_CHANNELS,
                     noSubSamples,
                     0);
}


/***************************************************************************/
/*!
  \brief  Changes the number of slot per frame (for time scale modification)

****************************************************************************/
void SetNumberOfSlots(HANDLE_PS_DEC h, unsigned int NumberOfSlots)
{
    unsigned int nEnv;

    h->hHybrid20->frameSize = NumberOfSlots;
    h->hHybrid34->frameSize = NumberOfSlots;
    h->noSubSamples         = NumberOfSlots;

    nEnv = h->aOcsParams[0].noEnv;
    if (nEnv > 0)
    {
        unsigned int n;
        double d;
        d =  (double)h->aOcsParams[0].aEnvStartStop[nEnv]/
             (double)NumberOfSlots;

        for (n = 1; n < nEnv; n++)
        {
            h->aOcsParams[0].aEnvStartStop[n] =
                (unsigned int)(((double)h->aOcsParams[0].aEnvStartStop[n]) * d+0.5);
        }
        h->aOcsParams[0].aEnvStartStop[nEnv] = NumberOfSlots;
    }
}



