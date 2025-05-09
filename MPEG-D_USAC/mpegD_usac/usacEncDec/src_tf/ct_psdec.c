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

 $Id: ct_psdec.c,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $

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

#include "ct_psdec.h"
#include "ct_sbrconst.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


/* undefine the following to enable the "unrestricted" version of the PS tool */
#define BASELINE_PS
/* #undef BASELINE_PS */


typedef const char (*Huffman)[2];

static const float IPD_STEP_SIZE = (float)(PI/NO_IPD_STEPS);

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

static const float p8_13_20[13] =
{
  0.00746082949812f,  0.02270420949825f,  0.04546865930473f,  0.07266113929591f,
  0.09885108575264f,  0.11793710567217f,  0.125f,             0.11793710567217f,
  0.09885108575264f,  0.07266113929591f,  0.04546865930473f,  0.02270420949825f,
  0.00746082949812f
};

static const float p2_13_20[13] =
{
  0.0f,               0.01899487526049f,  0.0f,              -0.07293139167538f,
  0.0f,               0.30596630545168f,  0.5f,               0.30596630545168f,
  0.0f,              -0.07293139167538f,  0.0f,               0.01899487526049f,
  0.0f
};

static const float p12_13_34[13] =
{
  0.04081179924692f,  0.03812810994926f,  0.05144908135699f,  0.06399831151592f,
  0.07428313801106f,  0.08100347892914f,  0.08333333333333f,  0.08100347892914f,
  0.07428313801106f,  0.06399831151592f,  0.05144908135699f,  0.03812810994926f,
  0.04081179924692f
};

static const float p8_13_34[13] =
{
  0.01565675600122f,  0.03752716391991f,  0.05417891378782f,  0.08417044116767f,
  0.10307344158036f,  0.12222452249753f,  0.12500000000000f,  0.12222452249753f,
  0.10307344158036f,  0.08417044116767f,  0.05417891378782f,  0.03752716391991f,
  0.01565675600122f
};

static const float p4_13_34[13] =
{
 -0.05908211155639f, -0.04871498374946f,  0.0f,               0.07778723915851f,
  0.16486303567403f,  0.23279856662996f,  0.25f,              0.23279856662996f,
  0.16486303567403f,  0.07778723915851f,  0.0f,              -0.04871498374946f,
 -0.05908211155639f
};

static int psTablesInitialized = 0;

static float aAllpassLinkDecaySer[NO_SERIAL_ALLPASS_LINKS];

static const int   aAllpassLinkDelaySer[] = {    3,     4,     5};
static const float aAllpassLinkFractDelay = 0.39f;
static const float aAllpassLinkFractDelaySer[] = {0.43f, 0.75f, 0.347f};

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
static float scaleFactors[NO_IID_LEVELS];
static float scaleFactorsFine[NO_IID_LEVELS_FINE];

static float quantizedRHOs[NO_ICC_STEPS] =
{
    1.0f, 0.937f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.589f, -1.0f
};

/* precalculated values of alpha for every rho */
static float alphas[NO_ICC_LEVELS];


static const int aNoIidBins[3] = {NO_LOW_RES_IID_BINS,
                                  NO_MID_RES_IID_BINS,
                                  NO_HI_RES_IID_BINS};

static const int aNoIccBins[3] = {NO_LOW_RES_ICC_BINS,
                                  NO_MID_RES_ICC_BINS,
                                  NO_HI_RES_ICC_BINS};

static const int aNoIpdBins[3] = {NO_LOW_RES_IPD_BINS,
                                  NO_MID_RES_IPD_BINS,
                                  NO_HI_RES_IPD_BINS};

static void **callocMatrix(int rows, int cols, size_t size);
static void freeMatrix(void **aaMatrix, int rows);

static void deCorrelate( HANDLE_PS_DEC h_ps_d,
                         float **rIntBufferLeft, float **iIntBufferLeft,
                         float **rIntBufferRight, float **iIntBufferRight);

static void applyRotation( HANDLE_PS_DEC pms,
                           float **qmfLeftReal , float **qmfLeftImag,
                           float **qmfRightReal, float **qmfRightImag );

static void
HybridAnalysis ( const float **mQmfReal,
                 const float **mQmfImag,
                 float **mHybridReal,
                 float **mHybridImag,
                 HANDLE_HYBRID hHybrid,
                 int bUse34StereoBands );

static void
HybridSynthesis ( const float **mHybridReal,  
                  const float **mHybridImag,  
                  float **mQmfReal,            
                  float **mQmfImag,           
                  HANDLE_HYBRID hHybrid );

static HANDLE_ERROR_INFO
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,
                         int frameSize,
                         int noBands,
                         const int *pResolution );

static void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid );

static int
CreatePsDec(HANDLE_PS_DEC *h_PS_DEC,
            int fs,
            unsigned int noQmfChans,
            unsigned int noSubSamples,
            int psMode);

static void ResetPsDec( HANDLE_PS_DEC pms );
static void ResetPsDeCor( HANDLE_PS_DEC pms );

static void
DeletePsDec(HANDLE_PS_DEC * h_ps_d_p);


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
static void kChannelFiltering( const float *pQmfReal,
                               const float *pQmfImag,
                               float **mHybridReal,
                               float **mHybridImag,
                               int nSamples,
                               int k,
                               int bCplx,
                               const float *p)
{
  int i, n, q;
  float real, imag;
  float modr, modi;

  for(i = 0; i < nSamples; i++) {
    /* FIR filter. */
    for(q = 0; q < k; q++) {
      real = 0;
      imag = 0;
      for(n = 0; n < 13; n++) {
        if (bCplx)
        {
          modr = (float) cos(PI*(n-6)/k*(1+2*q));
          modi = (float) -sin(PI*(n-6)/k*(1+2*q));
        }
        else{
          modr = (float) cos(2*PI*q*(n-6)/k);
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
static void
HybridAnalysis ( const float **mQmfReal,   /*!< The real part of the QMF-matrix.  */
                 const float **mQmfImag,   /*!< The imaginary part of the QMF-matrix. */
                 float **mHybridReal,      /*!< The real part of the hybrid-matrix. If NULL, only interal buffers are updated but no is output generated.  */
                 float **mHybridImag,      /*!< The imaginary part of the hybrid-matrix.  */
                 HANDLE_HYBRID hHybrid,    /*!< Handle to HYBRID struct. */
                 int bUse34StereoBands)    /*!< indicates which 8 band filter to use */

{
  int  k, n, band;
  HYBRID_RES hybridRes;
  int  frameSize  = hHybrid->frameSize;
  int  chOffset = 0;

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    hybridRes = hHybrid->pResolution[band];

    /* Create working buffer. */
    /* Copy stored samples to working buffer. */
    memcpy(hHybrid->pWorkReal, hHybrid->mQmfBufferReal[band],
           hHybrid->qmfBufferMove * sizeof(float));
    memcpy(hHybrid->pWorkImag, hHybrid->mQmfBufferImag[band],
           hHybrid->qmfBufferMove * sizeof(float));

    /* Append new samples to working buffer. */
    for(n = 0; n < frameSize; n++) {
      hHybrid->pWorkReal [hHybrid->qmfBufferMove + n] = mQmfReal [n + HYBRID_FILTER_DELAY] [band];
      hHybrid->pWorkImag [hHybrid->qmfBufferMove + n] = mQmfImag [n + HYBRID_FILTER_DELAY] [band];
    }

    /* Store samples for next frame. */
    memcpy(hHybrid->mQmfBufferReal[band], hHybrid->pWorkReal + frameSize,
           hHybrid->qmfBufferMove * sizeof(float));
    memcpy(hHybrid->mQmfBufferImag[band], hHybrid->pWorkImag + frameSize,
           hHybrid->qmfBufferMove * sizeof(float));


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
                         p2_13_20); /* only used for 10/20 bands dec */
      break;
    case HYBRID_4_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         4,
                         CPLX,
                         p4_13_34); /* only used for 34 bands dec */
     break;
    case HYBRID_8_CPLX:
      kChannelFiltering( hHybrid->pWorkReal,
                         hHybrid->pWorkImag,
                         hHybrid->mTempReal,
                         hHybrid->mTempImag,
                         frameSize,
                         8,
                         CPLX,
                         bUse34StereoBands?p8_13_34:p8_13_20);
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
}

/*******************************************************************************
 Functionname:  HybridSynthesis
 *******************************************************************************
 
 Description:   
 
 Arguments:     

 Return:        none

*******************************************************************************/
static void
HybridSynthesis ( const float **mHybridReal,   /*!< The real part of the hybrid-matrix.  */
                  const float **mHybridImag,   /*!< The imaginary part of the hybrid-matrix. */
                  float **mQmfReal,            /*!< The real part of the QMF-matrix.  */
                  float **mQmfImag,            /*!< The imaginary part of the QMF-matrix.  */
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
static HANDLE_ERROR_INFO
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,  /*!< Pointer to handle to HYBRID struct. */
                         int frameSize,            /*!< Framesize (in Qmf súbband samples). */
                         int noBands,              /*!< Number of Qmf bands for hybrid filtering. */
                         const int *pResolution )  /*!< Resolution in Qmf bands (length noBands). */
{
  int i;
  int maxNoChannels = 0;
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
  hs->qmfBufferMove = HYBRID_FILTER_LENGTH - 1;

  hs->pWorkReal = (float *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (float));
  hs->pWorkImag = (float *)
                  malloc ((frameSize + hs->qmfBufferMove) * sizeof (float));
  if (hs->pWorkReal == NULL || hs->pWorkImag == NULL)
    return 1;

  /* Allocate buffers */
  hs->mQmfBufferReal = (float **) malloc (noBands * sizeof (float *));
  hs->mQmfBufferImag = (float **) malloc (noBands * sizeof (float *));
  if (hs->mQmfBufferReal == NULL || hs->mQmfBufferImag == NULL)
    return 1;
  for (i = 0; i < noBands; i++) {
    hs->mQmfBufferReal[i] = (float *) calloc (hs->qmfBufferMove, sizeof (float));
    hs->mQmfBufferImag[i] = (float *) calloc (hs->qmfBufferMove, sizeof (float));
    if (hs->mQmfBufferReal[i] == NULL || hs->mQmfBufferImag[i] == NULL)
      return 1;
  }

  hs->mTempReal = (float **) malloc (frameSize * sizeof (float *));
  hs->mTempImag = (float **) malloc (frameSize * sizeof (float *));
  if (hs->mTempReal == NULL || hs->mTempImag == NULL)
    return 1;
  for (i = 0; i < frameSize; i++) {
    hs->mTempReal[i] = (float *) calloc (maxNoChannels, sizeof (float));
    hs->mTempImag[i] = (float *) calloc (maxNoChannels, sizeof (float));
    if (hs->mTempReal[i] == NULL || hs->mTempImag[i] == NULL)
      return 1;
  }

  *phHybrid = hs;
  return 0;
}

static void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid ) /*!< Pointer to handle to HYBRID struct. */
{
  int i;

  if (*phHybrid) {

    HANDLE_HYBRID hs = *phHybrid;
    int noBands    = hs->nQmfBands;
    int frameSize  = hs->frameSize;

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
                HANDLE_BIT_BUFFER hBitBuf,   /*!< Handle to Bitbuffer */
                int *length)                 /*!< length of huffman codeword (or NULL) */
{
  int bit, index = 0;
  int bitCount = 0;

  while (index >= 0) {
    bit = BufGetBits (hBitBuf, 1);
    bitCount++;
    index = h[index][bit];
  }
  if (length) {
    *length = bitCount;
  }
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
deltaDecodeModulo(int enable,
                  int *aIndex,      /* must not be negative for modulo decoding */
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
    for (i=nrElements*stride-1; i>0; i--) {
      aIndex[i] = aIndex[i/stride];
    }
  }
}

static void extractExtendedPsData(HANDLE_PS_DEC  h_ps_d,      /*!< Parametric Stereo Decoder */
                                  HANDLE_BIT_BUFFER hBitBuf)     /*!< Handle to the bit buffer */
{
  if (h_ps_d->bEnableExt) {
    int cnt;
    int i;
    int nBitsLeft;
    int reserved_ps;
    int env;
    int dtFlag;
    int bin;
    int length;
    Huffman CurrentTable;

    cnt = BufGetBits(hBitBuf, PS_EXTENSION_SIZE_BITS);
    if (cnt == (1<<PS_EXTENSION_SIZE_BITS)-1)
      cnt += BufGetBits(hBitBuf, PS_EXTENSION_ESC_COUNT_BITS);

    nBitsLeft = 8 * cnt;
    while (nBitsLeft > 7) {
      int extension_id = BufGetBits(hBitBuf, PS_EXTENSION_ID_BITS);
      nBitsLeft -= PS_EXTENSION_ID_BITS;

      switch(extension_id) {

      case EXTENSION_ID_IPDOPD_CODING:
        h_ps_d->bEnableIpdOpd = BufGetBits(hBitBuf, 1);
        nBitsLeft -= 1;
        if (h_ps_d->bEnableIpdOpd) {
          for (env=0; env<h_ps_d->noEnv; env++) {
            dtFlag = (int)BufGetBits (hBitBuf, 1);
            nBitsLeft -= 1;
            if (!dtFlag)
              CurrentTable = (Huffman)&aBookPsIpdFreqDecode;
            else
              CurrentTable = (Huffman)&aBookPsIpdTimeDecode;
            for (bin = 0; bin < aNoIpdBins[h_ps_d->freqResIpd]; bin++) {
              h_ps_d->aaIpdIndex[env][bin] = decode_huff_cw(CurrentTable,hBitBuf,&length);
              nBitsLeft -= length;
            }
            h_ps_d->abIpdDtFlag[env] = dtFlag;
            dtFlag = (int)BufGetBits (hBitBuf, 1);
            nBitsLeft -= 1;
            if (!dtFlag)
              CurrentTable = (Huffman)&aBookPsOpdFreqDecode;
            else
              CurrentTable = (Huffman)&aBookPsOpdTimeDecode;
            for (bin = 0; bin < aNoIpdBins[h_ps_d->freqResIpd]; bin++) {
              h_ps_d->aaOpdIndex[env][bin] = decode_huff_cw(CurrentTable,hBitBuf,&length);
              nBitsLeft -= length;
            }
            h_ps_d->abOpdDtFlag[env] = dtFlag;
          }
        }
        reserved_ps = BufGetBits (hBitBuf, 1);
        nBitsLeft -= 1;
        if (reserved_ps != 0)
          assert(0);
        break;

      default:
        /* An unknown extension id causes the remaining extension data
           to be skipped */
        cnt = nBitsLeft >> 3; /* number of remaining bytes */
        for (i=0; i<cnt; i++)
          BufGetBits(hBitBuf, 8);
        nBitsLeft -= cnt * 8;
        break;
      }
    }
    /* read fill bits for byte alignment */
    BufGetBits(hBitBuf, nBitsLeft);
  } /* h_ps_d->bEnableExt */
}

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

static void map20IndexTo34 (int *aIndex, int noBins)
{
  int aTemp[NO_HI_RES_BINS];
  int i;

  aTemp[0] = aIndex[0];
  aTemp[1] = (aIndex[0] + aIndex[1])/2;
  aTemp[2] = aIndex[1];
  aTemp[3] = aIndex[2];
  aTemp[4] = (aIndex[2] + aIndex[3])/2;
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
  /* For IPD/OPD it stops here */

  if (noBins == NO_HI_RES_BINS)
  {
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
  }

  for(i = 0; i<noBins; i++) {
    aIndex[i] = aTemp[i];
  }
}


static void map34FloatTo20 (float *aIndex)
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

static void map20FloatTo34 (float *aIndex)
{
  float aTemp[NO_HI_RES_BINS];
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
tfDecodePs(struct PS_DEC *h_ps_d) /*!< */
{
  int gr, env;

  if (!h_ps_d->bPsDataAvail) {
    /* no new PS data available (e.g. frame loss) */
    /* => keep latest data constant (i.e. FIX with noEnv=0) */
    h_ps_d->noEnv = 0;
  }

  for (env=0; env<h_ps_d->noEnv; env++) {
    int *aPrevIidIndex;
    int *aPrevIccIndex;
    int *aPrevIpdIndex;
    int *aPrevOpdIndex;

    int noIidSteps = h_ps_d->bFineIidQ?NO_IID_STEPS_FINE:NO_IID_STEPS;

    if (env==0) {
      aPrevIidIndex = h_ps_d->aIidPrevFrameIndex;
      aPrevIccIndex = h_ps_d->aIccPrevFrameIndex;
      aPrevIpdIndex = h_ps_d->aIpdPrevFrameIndex;
      aPrevOpdIndex = h_ps_d->aOpdPrevFrameIndex;
    }
    else {
      aPrevIidIndex = h_ps_d->aaIidIndex[env-1];
      aPrevIccIndex = h_ps_d->aaIccIndex[env-1];
      aPrevIpdIndex = h_ps_d->aaIpdIndex[env-1];
      aPrevOpdIndex = h_ps_d->aaOpdIndex[env-1];
    }

    deltaDecodeArray(h_ps_d->bEnableIid && !(h_ps_d->psMode & 0x0001),
                     h_ps_d->aaIidIndex[env],
                     aPrevIidIndex,
                     h_ps_d->abIidDtFlag[env],
                     aNoIidBins[h_ps_d->freqResIid],
                     (h_ps_d->freqResIid)?1:2,
                     -noIidSteps,
                     noIidSteps);

    deltaDecodeArray(h_ps_d->bEnableIcc && !(h_ps_d->psMode & 0x0002),
                     h_ps_d->aaIccIndex[env],
                     aPrevIccIndex,
                     h_ps_d->abIccDtFlag[env],
                     aNoIccBins[h_ps_d->freqResIcc],
                     (h_ps_d->freqResIcc)?1:2,
                     0,
                     NO_ICC_STEPS-1);

    deltaDecodeModulo(h_ps_d->bEnableIpdOpd && !(h_ps_d->psMode & 0x0004),
                     h_ps_d->aaIpdIndex[env],
                     aPrevIpdIndex,
                     h_ps_d->abIpdDtFlag[env],
                     aNoIpdBins[h_ps_d->freqResIpd],
                     (h_ps_d->freqResIpd)?1:2,
                     NO_IPD_STEPS);

    deltaDecodeModulo(h_ps_d->bEnableIpdOpd && !(h_ps_d->psMode & 0x0004),
                     h_ps_d->aaOpdIndex[env],
                     aPrevOpdIndex,
                     h_ps_d->abOpdDtFlag[env],
                     aNoIpdBins[h_ps_d->freqResIpd],
                     (h_ps_d->freqResIpd)?1:2,
                     NO_IPD_STEPS);


  }   /* for (env=0; env<h_ps_d->noEnv; env++) */

  /* handling of FIX noEnv=0 */
  if (h_ps_d->noEnv==0) {
    /* set noEnv=1, keep last parameters or force 0 if not enabled */
    h_ps_d->noEnv = 1;
    
    if (h_ps_d->bEnableIid) {
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aaIidIndex[h_ps_d->noEnv-1][gr] =
          h_ps_d->aIidPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aaIidIndex[h_ps_d->noEnv-1][gr] = 0;
      }
    }
    
    if (h_ps_d->bEnableIcc) {
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aaIccIndex[h_ps_d->noEnv-1][gr] =
          h_ps_d->aIccPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aaIccIndex[h_ps_d->noEnv-1][gr] = 0;
      }
    }
    
    if (h_ps_d->bEnableIpdOpd) {
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aaIpdIndex[h_ps_d->noEnv-1][gr] =
           h_ps_d->aIpdPrevFrameIndex[gr];
      }
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aaOpdIndex[h_ps_d->noEnv-1][gr] =
           h_ps_d->aOpdPrevFrameIndex[gr];
      }
    }
    else {
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aaIpdIndex[h_ps_d->noEnv-1][gr] = 0;
        h_ps_d->aaOpdIndex[h_ps_d->noEnv-1][gr] = 0;
      }
    }
  }

  /* Update previous frame index buffers */
  for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
    h_ps_d->aIidPrevFrameIndex[gr] =
      h_ps_d->aaIidIndex[h_ps_d->noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
    h_ps_d->aIccPrevFrameIndex[gr] =
      h_ps_d->aaIccIndex[h_ps_d->noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
    h_ps_d->aIpdPrevFrameIndex[gr] =
      h_ps_d->aaIpdIndex[h_ps_d->noEnv-1][gr];
  }
  for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
    h_ps_d->aOpdPrevFrameIndex[gr] =
      h_ps_d->aaOpdIndex[h_ps_d->noEnv-1][gr];
  }

  /* PS data from bitstream (if avail) was decoded now */
  h_ps_d->bPsDataAvail = 0;

  /* debug code, forcing indices to 0 (= PS off) if requested by psMode */
  for (env=0; env<h_ps_d->noEnv; env++) {
    if (h_ps_d->psMode & 0x0100)
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aaIidIndex[env][gr] = 0;
      }
    if (h_ps_d->psMode & 0x0200)
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aaIccIndex[env][gr] = 0;
      }
    if (h_ps_d->psMode & 0x0400)
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aaIpdIndex[env][gr] = 0;
        h_ps_d->aaOpdIndex[env][gr] = 0;
      }
  }

  /* handling of env borders for FIX & VAR */
  if (h_ps_d->bFrameClass == 0) {
    /* FIX_BORDERS NoEnv=0,1,2,4 */
    h_ps_d->aEnvStartStop[0] = 0;
    for (env=1; env<h_ps_d->noEnv; env++) {
      h_ps_d->aEnvStartStop[env] =
        (env * h_ps_d->noSubSamples) / h_ps_d->noEnv;
    }
    h_ps_d->aEnvStartStop[h_ps_d->noEnv] = h_ps_d->noSubSamples;
    /* 1024 (32 slots) env borders:  0, 8, 16, 24, 32 */
    /*  960 (30 slots) env borders:  0, 7, 15, 22, 30 */
  }
  else {   /* if (h_ps_d->bFrameClass == 0) */
    /* VAR_BORDERS NoEnv=1,2,3,4 */
    h_ps_d->aEnvStartStop[0] = 0;

    /* handle case aEnvStartStop[noEnv]<noSubSample for VAR_BORDERS by
       duplicating last PS parameters and incrementing noEnv */
    if (h_ps_d->aEnvStartStop[h_ps_d->noEnv] <
        (int)h_ps_d->noSubSamples) {
      for (gr = 0; gr < NO_HI_RES_IID_BINS; gr++) {
        h_ps_d->aaIidIndex[h_ps_d->noEnv][gr] =
          h_ps_d->aaIidIndex[h_ps_d->noEnv-1][gr];
      }
      for (gr = 0; gr < NO_HI_RES_ICC_BINS; gr++) {
        h_ps_d->aaIccIndex[h_ps_d->noEnv][gr] =
          h_ps_d->aaIccIndex[h_ps_d->noEnv-1][gr];
      }
      for (gr = 0; gr < NO_HI_RES_IPD_BINS; gr++) {
        h_ps_d->aaIpdIndex[h_ps_d->noEnv][gr] =
          h_ps_d->aaIpdIndex[h_ps_d->noEnv-1][gr];
        h_ps_d->aaOpdIndex[h_ps_d->noEnv][gr] =
          h_ps_d->aaOpdIndex[h_ps_d->noEnv-1][gr];
      }
      h_ps_d->noEnv++;
      h_ps_d->aEnvStartStop[h_ps_d->noEnv] = h_ps_d->noSubSamples;
    }

    /* enforce strictly monotonic increasing borders */
    for (env=1; env<h_ps_d->noEnv; env++) {
      int thr;
      thr = h_ps_d->noSubSamples - (h_ps_d->noEnv - env);
      if (h_ps_d->aEnvStartStop[env] > thr) {
        h_ps_d->aEnvStartStop[env] = thr;
      }
      else {
        thr = h_ps_d->aEnvStartStop[env-1]+1;
        if (h_ps_d->aEnvStartStop[env] < thr) {
          h_ps_d->aEnvStartStop[env] = thr;
        }
      }
    }
  }   /* if (h_ps_d->bFrameClass == 0) ... else */

  /* copy data prior to possible 20<->34 in-place mapping */
  for (env=0; env<h_ps_d->noEnv; env++) {
    int i;
    for (i=0; i<NO_HI_RES_IID_BINS; i++) {
      h_ps_d->aaIidIndexMapped[env][i] = h_ps_d->aaIidIndex[env][i];
    }
    for (i=0; i<NO_HI_RES_ICC_BINS; i++) {
      h_ps_d->aaIccIndexMapped[env][i] = h_ps_d->aaIccIndex[env][i];
    }
    for (i=0; i<NO_HI_RES_IPD_BINS; i++) {
      h_ps_d->aaIpdIndexMapped[env][i] = h_ps_d->aaIpdIndex[env][i];
      h_ps_d->aaOpdIndexMapped[env][i] = h_ps_d->aaOpdIndex[env][i];
    }
  }
#ifdef BASELINE_PS
  /* MPEG baseline PS */
  for (env=0; env<h_ps_d->noEnv; env++) {
    int i;
    if (h_ps_d->freqResIid == 2)
      map34IndexTo20 (h_ps_d->aaIidIndexMapped[env], NO_HI_RES_IID_BINS);
    if (h_ps_d->freqResIcc == 2)
      map34IndexTo20 (h_ps_d->aaIccIndexMapped[env], NO_HI_RES_ICC_BINS);
    /* disable IPD/OPD */
    for (i=0; i<NO_HI_RES_IPD_BINS; i++) {
      h_ps_d->aaIpdIndexMapped[env][i] = 0;
      h_ps_d->aaOpdIndexMapped[env][i] = 0;
    }
  }
#else
  /* MPEG unrestricted PS */
  if (h_ps_d->bUse34StereoBands) {
    for (env=0; env<h_ps_d->noEnv; env++) {
      if (h_ps_d->freqResIid < 2)
        map20IndexTo34 (h_ps_d->aaIidIndexMapped[env], NO_HI_RES_IID_BINS);
      if (h_ps_d->freqResIcc < 2)
        map20IndexTo34 (h_ps_d->aaIccIndexMapped[env], NO_HI_RES_ICC_BINS);
      if (h_ps_d->freqResIpd < 2) {
        map20IndexTo34 (h_ps_d->aaIpdIndexMapped[env], NO_HI_RES_IPD_BINS);
        map20IndexTo34 (h_ps_d->aaOpdIndexMapped[env], NO_HI_RES_IPD_BINS);
      }
    }
  }
#endif /* BASELINE_PS */

}


/***************************************************************************/
/*!

  \brief  Reads parametric stereo data from bitstream

  \return

****************************************************************************/
unsigned int
tfReadPsData (HANDLE_PS_DEC h_ps_d, /*!< handle to struct PS_DEC*/
            HANDLE_BIT_BUFFER hBitBuf,  /*!< handle to struct BIT_BUF*/
            int nBitsLeft               /*!< max number of bits available*/
           )
{
  int     gr, env;
  int     dtFlag;
  int     startbits;
  Huffman CurrentTable;
  int     bEnableHeader;

  if (!h_ps_d)
    return 0;

  startbits = GetNrBitsAvailable(hBitBuf);

  bEnableHeader = (int) BufGetBits (hBitBuf, 1);

  /* Read header */
  if (bEnableHeader) {
    h_ps_d->bEnableIid = (int) BufGetBits (hBitBuf, 1);
    if (h_ps_d->bEnableIid)
      h_ps_d->modeIid = (int) BufGetBits (hBitBuf, 3);

    h_ps_d->bEnableIcc = (int) BufGetBits (hBitBuf, 1);
    if (h_ps_d->bEnableIcc)
      h_ps_d->modeIcc = (int) BufGetBits (hBitBuf, 3);

    h_ps_d->bEnableExt = (int) BufGetBits (hBitBuf, 1);
  }

  h_ps_d->bFrameClass = (int) BufGetBits (hBitBuf, 1);
  if (h_ps_d->bFrameClass == 0) {
    /* FIX_BORDERS NoEnv=0,1,2,4 */
    h_ps_d->noEnv = aFixNoEnvDecode[(int) BufGetBits (hBitBuf, 2)];
    /* all additional handling of env borders is now in DecodePs() */
  }
  else {
    /* VAR_BORDERS NoEnv=1,2,3,4 */
    h_ps_d->noEnv = 1+(int) BufGetBits (hBitBuf, 2);
    for (env=1; env<h_ps_d->noEnv+1; env++)
      h_ps_d->aEnvStartStop[env] = ((int) BufGetBits (hBitBuf, 5)) + 1;
    /* all additional handling of env borders is now in DecodePs() */
  }

  /* verify that IID & ICC modes (quant grid, freq res) are supported */
  if ((h_ps_d->modeIid > 5) || (h_ps_d->modeIcc > 5)) {
    /* no useful PS data could be read from bitstream */
    h_ps_d->bPsDataAvail = 0;
    /* discard all remaining bits */
    nBitsLeft -= startbits - GetNrBitsAvailable(hBitBuf);
    while (nBitsLeft) {
      int i = nBitsLeft;
      if (i>8) {
        i = 8;
      }
      BufGetBits (hBitBuf, i);
      nBitsLeft -= i;
    }
    return (startbits - GetNrBitsAvailable(hBitBuf));
  }

  if (h_ps_d->modeIid > 2){
    h_ps_d->freqResIid = h_ps_d->modeIid-3;
    h_ps_d->bFineIidQ = 1;
  }
  else{
    h_ps_d->freqResIid = h_ps_d->modeIid;
    h_ps_d->bFineIidQ = 0;
  }
  h_ps_d->freqResIpd = h_ps_d->freqResIid;

  if (h_ps_d->modeIcc > 2){
    h_ps_d->freqResIcc = h_ps_d->modeIcc-3;
    h_ps_d->bUsePcaRot = 1;
  }
  else{
    h_ps_d->freqResIcc = h_ps_d->modeIcc;
    h_ps_d->bUsePcaRot = 0;
  }

  if (h_ps_d->bEnableIid || h_ps_d->bEnableIcc ){
    h_ps_d->bUse34StereoBands = ((h_ps_d->bEnableIid &&
                                  (h_ps_d->freqResIid == 2)) ||
                                 (h_ps_d->bEnableIcc &&
                                  (h_ps_d->freqResIcc == 2)));
  }

#ifdef BASELINE_PS
  h_ps_d->bUse34StereoBands = 0;
  h_ps_d->bUsePcaRot = 0;
#endif

  /* Extract IID data */
  if (h_ps_d->bEnableIid) {
    for (env=0; env<h_ps_d->noEnv; env++) {
      dtFlag = (int)BufGetBits (hBitBuf, 1);
      if (!dtFlag)
      {
        if (h_ps_d->bFineIidQ)
          CurrentTable = (Huffman)&aBookPsIidFineFreqDecode;
        else
          CurrentTable = (Huffman)&aBookPsIidFreqDecode;
      }
      else
      {
        if (h_ps_d->bFineIidQ)
          CurrentTable = (Huffman)&aBookPsIidFineTimeDecode;
        else
          CurrentTable = (Huffman)&aBookPsIidTimeDecode;
      }

      for (gr = 0; gr < aNoIidBins[h_ps_d->freqResIid]; gr++)
        h_ps_d->aaIidIndex[env][gr] = decode_huff_cw(CurrentTable,hBitBuf,NULL);
      h_ps_d->abIidDtFlag[env] = dtFlag;
    }
  }

  /* Extract ICC data */
  if (h_ps_d->bEnableIcc) {
    for (env=0; env<h_ps_d->noEnv; env++) {
      dtFlag = (int)BufGetBits (hBitBuf, 1);
      if (!dtFlag)
        CurrentTable = (Huffman)&aBookPsIccFreqDecode;
      else
        CurrentTable = (Huffman)&aBookPsIccTimeDecode;

      for (gr = 0; gr < aNoIccBins[h_ps_d->freqResIcc]; gr++)
        h_ps_d->aaIccIndex[env][gr] = decode_huff_cw(CurrentTable,hBitBuf,NULL);
      h_ps_d->abIccDtFlag[env] = dtFlag;
    }
  }

  /* Extract IPD/OPD data */
  extractExtendedPsData(h_ps_d, hBitBuf);

  /* new PS data was read from bitstream */
  h_ps_d->bPsDataAvail = 1;

  return (startbits - GetNrBitsAvailable(hBitBuf));
}

static void
initPsTables (HANDLE_PS_DEC h_ps_d)
{
  unsigned int i, j;

  float hybridCenterFreq20[12] = {0.5/4,  1.5/4,  2.5/4,  3.5/4,
                              4.5/4*0,  5.5/4*0, -1.5/4, -0.5/4,
                                  3.5/2,  2.5/2,  4.5/2,  5.5/2};

  float hybridCenterFreq34[32] = { 1.0f/12,    3.0f/12,   5.0f/12,   7.0f/12,
                                   9.0f/12,   11.0f/12,  13.0f/12,  15.0f/12,
                                  17.0f/12,   -5.0f/12,  -3.0f/12,  -1.0f/12,
                                  17.0f/8,    19.0f/8,    5.0f/8,    7.0f/8,
                                   9.0f/8,    11.0f/8,   13.0f/8,   15.0f/8,
                                   9.0f/4,    11.0f/4,   13.0f/4,    7.0f/4,
                                  17.0f/4,    11.0f/4,   13.0f/4,   15.0f/4,
                                  17.0f/4,    19.0f/4,   21.0f/4,   15.0f/4};



  if (psTablesInitialized)
    return;

  for (j=0 ; j < NO_QMF_CHANNELS ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImQmf[j] = (float)sin(-PI*(j+0.5)*(aAllpassLinkFractDelay));
    h_ps_d->aaFractDelayPhaseFactorReQmf[j] = (float)cos(PI*(j+0.5)*(aAllpassLinkFractDelay));
  }
  for (j=0 ; j < NO_SUB_QMF_CHANNELS ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImSubQmf20[j]= (float)sin(-PI*hybridCenterFreq20[j]*aAllpassLinkFractDelay);
    h_ps_d->aaFractDelayPhaseFactorReSubQmf20[j] = (float)cos(PI*hybridCenterFreq20[j]*aAllpassLinkFractDelay);
  }
  for (j=0 ; j < NO_SUB_QMF_CHANNELS_HI_RES ; j++) {
    h_ps_d->aaFractDelayPhaseFactorImSubQmf34[j] = (float)sin(-PI*hybridCenterFreq34[j]*aAllpassLinkFractDelay);
    h_ps_d->aaFractDelayPhaseFactorReSubQmf34[j] = (float)cos(PI*hybridCenterFreq34[j]*aAllpassLinkFractDelay);
  }

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {

    aAllpassLinkDecaySer[i] = (float)exp(-aAllpassLinkDelaySer[i]/ALLPASS_SERIAL_TIME_CONST);

    for (j=0 ; j < NO_QMF_CHANNELS ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImQmf[j][i] = (float)sin(-PI*(j+0.5)*(aAllpassLinkFractDelaySer[i]));
      h_ps_d->aaFractDelayPhaseFactorSerReQmf[j][i] = (float)cos(PI*(j+0.5)*(aAllpassLinkFractDelaySer[i]));
    }
    for (j=0 ; j < NO_SUB_QMF_CHANNELS ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20[j][i] = (float)sin(-PI*hybridCenterFreq20[j]*aAllpassLinkFractDelaySer[i]);
      h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20[j][i] = (float)cos(PI*hybridCenterFreq20[j]*aAllpassLinkFractDelaySer[i]);
    }
    for (j=0 ; j < NO_SUB_QMF_CHANNELS_HI_RES ; j++) {
      h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34[j][i] = (float)sin(-PI*hybridCenterFreq34[j]*aAllpassLinkFractDelaySer[i]);
      h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34[j][i] = (float)cos(PI*hybridCenterFreq34[j]*aAllpassLinkFractDelaySer[i]);
    }
  }
/* scaleFactors */
    /* precalculate scale factors for the negative iids */
    for ( i = NO_IID_STEPS - 1, j = 0; j < NO_IID_STEPS; i--, j++ )
    {
        scaleFactors[j] = ( float )sqrt( 2.0 / ( 1.0 + pow( 10.0, -quantizedIIDs[i] / 10.0 ) ) );
    }

    /* iid = 0 */
    scaleFactors[j++] = 1.0;

    /* precalculate scale factors for the positive iids */
    for ( i = 0; i < NO_IID_STEPS; i++, j++ )
    {
        scaleFactors[j] = ( float )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDs[i] / 10.0 ) ) );
    }
    for ( i = 0; i < NO_IID_STEPS_FINE; i++, j++ )
    {
        scaleFactorsFine[j] = ( float )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDsFine[i] / 10.0 ) ) );
    }
/* scaleFactorsFine */
    /* precalculate scale factors for the negative iids */
    for ( i = NO_IID_STEPS_FINE - 1, j = 0; j < NO_IID_STEPS_FINE; i--, j++ )
    {
        scaleFactorsFine[j] = ( float )sqrt( 2.0 / ( 1.0 + pow( 10.0, -quantizedIIDsFine[i] / 10.0 ) ) );
    }
    /* iid = 0 */
    scaleFactorsFine[j++] = 1.0;
    /* precalculate scale factors for the positive iids */
    for ( i = 0; i < NO_IID_STEPS_FINE; i++, j++ )
    {
        scaleFactorsFine[j] = ( float )sqrt( 2.0 / ( 1.0 + pow( 10.0, quantizedIIDsFine[i] / 10.0 ) ) );
    }

    /* precalculate alphas */
    for ( i = 0; i < NO_ICC_LEVELS; i++ )
    {
        alphas[i] = ( float )( 0.5 * acos ( quantizedRHOs[i] ) );
    }

  psTablesInitialized = 1;
}


/***************************************************************************/
/*!
  \brief  Creates one instance of the PS_DEC struct

  \return Error info

****************************************************************************/
static int
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

   /* initialisation */
  h_ps_d->noSubSamples = noSubSamples;   /* col */
  h_ps_d->noChannels = noQmfChans;   /* row */
  h_ps_d->psMode = psMode;   /* 0: no override */

  /* explicitly init state variables to safe values (until first ps header arrives) */
  h_ps_d->bPsDataAvail = 0;
  h_ps_d->bEnableIid = 0;
  h_ps_d->modeIid = 0;
  h_ps_d->bEnableIcc = 0;
  h_ps_d->modeIcc = 0;
  h_ps_d->bEnableExt = 0;
  h_ps_d->bEnableIpdOpd = 0;

  h_ps_d->freqResIid = 0;
  h_ps_d->bFineIidQ = 0;
  h_ps_d->freqResIcc = 0;
  h_ps_d->bUsePcaRot = 0;
  h_ps_d->freqResIpd = 0;
  h_ps_d->bUse34StereoBands = 0;
  h_ps_d->bUse34StereoBandsPrev = 0;

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
  h_ps_d->mHybridRealLeft  = (float **) calloc (h_ps_d->noSubSamples, sizeof (float *));
  h_ps_d->mHybridImagLeft  = (float **) calloc (h_ps_d->noSubSamples, sizeof (float *));
  h_ps_d->mHybridRealRight = (float **) calloc (h_ps_d->noSubSamples, sizeof (float *));
  h_ps_d->mHybridImagRight = (float **) calloc (h_ps_d->noSubSamples, sizeof (float *));

  if (h_ps_d->mHybridRealLeft  == NULL || h_ps_d->mHybridImagLeft  == NULL ||
      h_ps_d->mHybridRealRight == NULL || h_ps_d->mHybridImagRight == NULL) {
    DeletePsDec (&h_ps_d);
    return 1;
  }

  for (i = 0; i < h_ps_d->noSubSamples; i++) {

    h_ps_d->mHybridRealLeft[i] =
      (float *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (float));
    h_ps_d->mHybridImagLeft[i] =
      (float *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (float));
    h_ps_d->mHybridRealRight[i] =
      (float *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (float));
    h_ps_d->mHybridImagRight[i] =
      (float *) calloc (NO_SUB_QMF_CHANNELS_HI_RES, sizeof (float));

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
  h_ps_d->firstDelaySb = 23;   /* 24th QMF first with Delay */

  h_ps_d->aaRealDelayBufferQmf = (float **)callocMatrix( h_ps_d->noSampleDelay,
                                                         noQmfChans, sizeof(float));
  if (h_ps_d->aaRealDelayBufferQmf == NULL)
    return 1;

  h_ps_d->aaImagDelayBufferQmf = (float **)callocMatrix(h_ps_d->noSampleDelay,
                                                         noQmfChans, sizeof(float));
  if (h_ps_d->aaImagDelayBufferQmf == NULL)
    return 1;

  h_ps_d->aaRealDelayBufferSubQmf = (float **)callocMatrix( h_ps_d->noSampleDelay,
                                                         NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaRealDelayBufferSubQmf == NULL)
    return 1;

  h_ps_d->aaImagDelayBufferSubQmf = (float **)callocMatrix(h_ps_d->noSampleDelay,
                                                         NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaImagDelayBufferSubQmf == NULL)
    return 1;

  /* allpass filter */
  h_ps_d->aaFractDelayPhaseFactorReQmf = (float*)calloc(NO_QMF_CHANNELS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorReQmf == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImQmf = (float*)calloc(NO_QMF_CHANNELS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorImQmf == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReQmf = (float**)callocMatrix(NO_QMF_CHANNELS,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerReQmf == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImQmf = (float**)callocMatrix(NO_QMF_CHANNELS,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerImQmf == NULL)
    return 1;



  h_ps_d->aaFractDelayPhaseFactorReSubQmf20 = (float*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorReSubQmf20 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImSubQmf20 = (float*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorImSubQmf20 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 = (float**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerReSubQmf20 == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 = (float**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerImSubQmf20 == NULL)
    return 1;


 
  h_ps_d->aaFractDelayPhaseFactorReSubQmf34 = (float*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorReSubQmf34 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorImSubQmf34 = (float*)calloc(NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorImSubQmf34 == NULL)
    return 1;

  h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 = (float**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34 == NULL)
    return 1;
  h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 = (float**)callocMatrix(NO_SUB_QMF_CHANNELS_HI_RES,
                                                                     NO_SERIAL_ALLPASS_LINKS, sizeof(float));
  if (h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34 == NULL)
    return 1;



  initPsTables(h_ps_d);

  h_ps_d->PeakDecayFactorFast = 0.765928338364649f;

  h_ps_d->intFilterCoeff = 1.0f - NRG_INT_COEFF;

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {
    h_ps_d->aDelayRBufIndexSer[i] = 0;
    h_ps_d->aNoSampleDelayRSer[i] = aAllpassLinkDelaySer[i];
    
    h_ps_d->aaaRealDelayRBufferSerQmf[i] = (float**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                                 noQmfChans, sizeof(float));
    if (h_ps_d->aaaRealDelayRBufferSerQmf[i] == NULL)
      return 1;
    h_ps_d->aaaImagDelayRBufferSerQmf[i] = (float**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                                 noQmfChans, sizeof(float));
    if (h_ps_d->aaaImagDelayRBufferSerQmf[i] == NULL)
      return 1;
    
    h_ps_d->aaaRealDelayRBufferSerSubQmf[i] = (float**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                                    NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
    if (h_ps_d->aaaRealDelayRBufferSerSubQmf[i] == NULL)
      return 1;
    h_ps_d->aaaImagDelayRBufferSerSubQmf[i] = (float**)callocMatrix(h_ps_d->aNoSampleDelayRSer[i],
                                                                    NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
    if (h_ps_d->aaaImagDelayRBufferSerSubQmf[i] == NULL)
      return 1;
  }

  ResetPsDec( h_ps_d );

  ResetPsDeCor( h_ps_d );

  *h_PS_DEC=h_ps_d;
  return 0;
} /*END CreatePsDec */


static void ResetPsDec( HANDLE_PS_DEC pms )  /*!< pointer to the module state */
{
  int i;

  for ( i = 0; i < NO_HI_RES_BINS; i++ )
  {
    pms->h11rPrev[i] = 1.0f;
    pms->h12rPrev[i] = 1.0f;
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


static void ResetPsDeCor( HANDLE_PS_DEC pms )  /*!< pointer to the module state */
{
  int i;

  for (i=0 ; i < NO_HI_RES_BINS ; i++) {
    pms->aPeakDecayFastBin[i] = 0.0f;
    pms->aPrevNrgBin[i] = 0.0f;
    pms->aPrevPeakDiffBin[i] = 0.0f;
  }

  clearMatrix((void**)pms->aaRealDelayBufferQmf, pms->noSampleDelay, pms->noChannels, sizeof(float));
  clearMatrix((void**)pms->aaImagDelayBufferQmf, pms->noSampleDelay, pms->noChannels, sizeof(float));
  clearMatrix((void**)pms->aaRealDelayBufferSubQmf, pms->noSampleDelay, NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  clearMatrix((void**)pms->aaImagDelayBufferSubQmf, pms->noSampleDelay, NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS ; i++) {
    clearMatrix((void**)pms->aaaRealDelayRBufferSerQmf[i], pms->aNoSampleDelayRSer[i], pms->noChannels, sizeof(float));
    clearMatrix((void**)pms->aaaImagDelayRBufferSerQmf[i], pms->aNoSampleDelayRSer[i], pms->noChannels, sizeof(float));
    clearMatrix((void**)pms->aaaRealDelayRBufferSerSubQmf[i], pms->aNoSampleDelayRSer[i], NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
    clearMatrix((void**)pms->aaaImagDelayRBufferSerSubQmf[i], pms->aNoSampleDelayRSer[i], NO_SUB_QMF_CHANNELS_HI_RES, sizeof(float));
  }

}


/***************************************************************************/
/*!
  \brief  Deletes one instance of the PS_DEC struct

****************************************************************************/
static void
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

    /* Free hybrid filterbank. */
    for (i = 0; i < h_ps_d->noSubSamples; i++) {
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
tfApplyPsFrame(HANDLE_PS_DEC h_ps_d,        /*!< */
             float **rIntBufferLeft,      /*!< input, modified */
             float **iIntBufferLeft,      /*!< input, modified */
             float **rIntBufferRight,     /*!< output */
             float **iIntBufferRight,     /*!< output */
             int usb)                     /*!< */

{
  unsigned int sb;
  int i,k;

  /* First chose hybrid filterbank. 20 or 34 band case */
  if (h_ps_d->bUse34StereoBands) {
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

  for (sb = usb; sb < h_ps_d->noChannels; sb++) {
    for (i=0 ; i<NO_SERIAL_ALLPASS_LINKS ; i++) {
      for (k=0 ; k < h_ps_d->aNoSampleDelayRSer[i]; k++) {
        h_ps_d->aaaRealDelayRBufferSerQmf[i][k][sb] = 0;
        h_ps_d->aaaImagDelayRBufferSerQmf[i][k][sb] = 0;
      }
    }
    for (k=0 ; k < h_ps_d->noSampleDelay; k++) {
      h_ps_d->aaRealDelayBufferQmf[k][sb] = 0;
      h_ps_d->aaImagDelayBufferQmf[k][sb] = 0;
    }
  }

  /* update both hybrid filterbank states to enable dynamic switching */
  HybridAnalysis ( (const float**)rIntBufferLeft,
                   (const float**)iIntBufferLeft,
                   (h_ps_d->bUse34StereoBands)?(float**)NULL:h_ps_d->mHybridRealLeft,
                   h_ps_d->mHybridImagLeft,
                   h_ps_d->hHybrid20,
                   0);
  HybridAnalysis ( (const float**)rIntBufferLeft,
                   (const float**)iIntBufferLeft,
                   (h_ps_d->bUse34StereoBands)?h_ps_d->mHybridRealLeft:(float**)NULL,
                   h_ps_d->mHybridImagLeft,
                   h_ps_d->hHybrid34,
                   1);


  if (!h_ps_d->bUse34StereoBands) {
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
    }

    if (!(h_ps_d->psMode & 0x0040)) {
      applyRotation(h_ps_d,
                    rIntBufferLeft,
                    iIntBufferLeft,
                    rIntBufferRight,
                    iIntBufferRight);
    }
  }

  HybridSynthesis ( (const float**)h_ps_d->mHybridRealLeft,
                    (const float**)h_ps_d->mHybridImagLeft,
                    rIntBufferLeft,
                    iIntBufferLeft,
                    h_ps_d->pHybrid );

  HybridSynthesis ( (const float**)h_ps_d->mHybridRealRight,
                    (const float**)h_ps_d->mHybridImagRight,
                    rIntBufferRight,
                    iIntBufferRight,
                    h_ps_d->pHybrid );

  /* track freq. res. switching */
  h_ps_d->bUse34StereoBandsPrev = h_ps_d->bUse34StereoBands;

}/* END DecodePsFrame */


/***************************************************************************/
/*!
  \brief  Generate decorrelated side channel using allpass/delay

****************************************************************************/
static void
deCorrelate(HANDLE_PS_DEC h_ps_d, /*!< */
            float **rIntBufferLeft, /*!< Pointer to mid channel real part of QMF matrix*/
            float **iIntBufferLeft, /*!< Pointer to mid channel imag part of QMF matrix*/
            float **rIntBufferRight,/*!< Pointer to side channel real part of QMF matrix*/
            float **iIntBufferRight /*!< Pointer to side channel imag part of QMF matrix*/
            )
{
  int sb, maxsb, gr, k;
  int m;
  int tempDelay = 0;
  int aTempDelayRSer[NO_SERIAL_ALLPASS_LINKS];
  float iInputLeft;
  float rInputLeft;
  float peakDiff, nrg, transRatio;

  float **aaLeftReal;
  float **aaLeftImag;
  float **aaRightReal;
  float **aaRightImag;

  float ***pppRealDelayRBufferSer;
  float ***pppImagDelayRBufferSer;
  float **ppRealDelayBuffer;
  float **ppImagDelayBuffer;

  float *ppFractDelayPhaseFactorRe;
  float *ppFractDelayPhaseFactorIm;
  float **ppFractDelayPhaseFactorSerRe;
  float **ppFractDelayPhaseFactorSerIm;

  int *pNoSampleDelayDelay = NULL;
  int *pDelayBufIndexDelay = NULL;

  float aaPower[32][NO_HI_RES_BINS];
  float aaTransRatio[32][NO_HI_RES_BINS];
  int bin;


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
  if (h_ps_d->bUse34StereoBands){
    ppFractDelayPhaseFactorRe = h_ps_d->aaFractDelayPhaseFactorReSubQmf34;
    ppFractDelayPhaseFactorIm = h_ps_d->aaFractDelayPhaseFactorImSubQmf34;
    ppFractDelayPhaseFactorSerRe = h_ps_d->aaFractDelayPhaseFactorSerReSubQmf34;
    ppFractDelayPhaseFactorSerIm = h_ps_d->aaFractDelayPhaseFactorSerImSubQmf34;
  }
  else{
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
      for (k=h_ps_d->aEnvStartStop[0] ; k < h_ps_d->aEnvStartStop[ h_ps_d->noEnv] ; k++) {

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
    for (k=h_ps_d->aEnvStartStop[0] ; k < h_ps_d->aEnvStartStop[ h_ps_d->noEnv] ; k++) {
      float q=1.5f;

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
        aaTransRatio[k][bin] = 1.0f;
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
      float decayScaleFactor;
      float decay_cutoff;

      /* Determine decay cutoff factor */
      if (h_ps_d->bUse34StereoBands) {
	decay_cutoff = DECAY_CUTOFF_HI_RES;
      }
      else {
	decay_cutoff = DECAY_CUTOFF;
      }

      if (gr < h_ps_d->noSubQmfGroups || sb <= decay_cutoff)
        decayScaleFactor = 1.0f;
      else
        decayScaleFactor = 1.0f + decay_cutoff * DECAY_SLOPE - DECAY_SLOPE * sb;

      decayScaleFactor = max(decayScaleFactor, 0.0f);

      /* set delay indices */
      tempDelay = h_ps_d->delayBufIndex;
      for (k=0; k<NO_SERIAL_ALLPASS_LINKS ; k++)
        aTempDelayRSer[k] = h_ps_d->aDelayRBufIndexSer[k];

      /* k = subsample */
      for (k=h_ps_d->aEnvStartStop[0] ; k < h_ps_d->aEnvStartStop[ h_ps_d->noEnv] ; k++) {
        float rTmp, iTmp, rTmp0, iTmp0, rR0, iR0;

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
applyRotation( HANDLE_PS_DEC      pms,          /*!< pointer to the moodule state */
               float            **qmfLeftReal,  /*!< pointer to left channel real part of QMF matrix*/
               float            **qmfLeftImag,  /*!< pointer to left channel imag part of QMF matrix*/
               float            **qmfRightReal, /*!< pointer to right channel real part of QMF matrix*/
               float            **qmfRightImag  /*!< pointer to right channel imag part of QMF matrix*/
             )
{
  int     i;
  int     group;
  int     bin = 0;
  int     subband, maxSubband;
  int     env;
  float **hybrLeftReal;
  float **hybrLeftImag;
  float **hybrRightReal;
  float **hybrRightImag;
  float   scaleL, scaleR;
  float   alpha, beta;
  float   ipd, opd;
  float   ipd1, opd1;
  float   ipd2, opd2;
  float   phi1, phi2;
  float   h11r, h12r, h21r, h22r;
  float   h11i, h12i, h21i, h22i;
  float   H11r, H12r, H21r, H22r;
  float   H11i, H12i, H21i, H22i;
  float   deltaH11r, deltaH12r, deltaH21r, deltaH22r;
  float   deltaH11i, deltaH12i, deltaH21i, deltaH22i;
  float   tempLeftReal, tempLeftImag;
  float   tempRightReal, tempRightImag;
  int     ipdBins;
  int     L;
  float  *pScaleFactors;
  const int *pQuantizedIIDs;
  int     noIidSteps;

  if (pms->bFineIidQ) {
    noIidSteps = NO_IID_STEPS_FINE;
    pScaleFactors = scaleFactorsFine;
    pQuantizedIIDs = quantizedIIDsFine;
  }
  else {
    noIidSteps = NO_IID_STEPS;
    pScaleFactors = scaleFactors;
    pQuantizedIIDs = quantizedIIDs;
  }
  
  /* Check on half resolution */
  if (pms->freqResIpd == 0) {
    ipdBins = aNoIpdBins[1];
  }
  else {
    ipdBins = aNoIpdBins[pms->freqResIpd];
  }
  
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

      /*
      map20IndexTo34(pms->aIpdIndexMapped_1, NO_HI_RES_IPD_BINS);
      map20IndexTo34(pms->aOpdIndexMapped_1, NO_HI_RES_IPD_BINS);
      map20IndexTo34(pms->aIpdIndexMapped_2, NO_HI_RES_IPD_BINS);
      map20IndexTo34(pms->aOpdIndexMapped_2, NO_HI_RES_IPD_BINS);
      */

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

      /*
      map34IndexTo20(pms->aIpdIndexMapped_1, NO_HI_RES_IPD_BINS);
      map34IndexTo20(pms->aOpdIndexMapped_1, NO_HI_RES_IPD_BINS);
      map34IndexTo20(pms->aIpdIndexMapped_2, NO_HI_RES_IPD_BINS);
      map34IndexTo20(pms->aOpdIndexMapped_2, NO_HI_RES_IPD_BINS);
      */

      /* reset ipd/opd smoother history (instead of mapping) */
      memset (pms->aIpdIndexMapped_1, 0, sizeof(pms->aIpdIndexMapped_1));
      memset (pms->aOpdIndexMapped_1, 0, sizeof(pms->aOpdIndexMapped_1));
      memset (pms->aIpdIndexMapped_2, 0, sizeof(pms->aIpdIndexMapped_2));
      memset (pms->aOpdIndexMapped_2, 0, sizeof(pms->aOpdIndexMapped_2));
    }
  }

  for ( env = 0; env < pms->noEnv; env++ ) {

    /* dequantize and decode */
    for ( bin = 0; bin < pms->noBins; bin++ ) {

      if (!pms->bUsePcaRot) {
        /* type 'A' rotation */
        scaleR = pScaleFactors[noIidSteps + pms->aaIidIndexMapped[env][bin]];
        scaleL = pScaleFactors[noIidSteps - pms->aaIidIndexMapped[env][bin]];

        alpha  = alphas[pms->aaIccIndexMapped[env][bin]];

        beta   = alpha * ( scaleR - scaleL ) / PSC_SQRT2F;

        h11r = ( float )( scaleL * cos( beta + alpha ) );
        h12r = ( float )( scaleR * cos( beta - alpha ) );
        h21r = ( float )( scaleL * sin( beta + alpha ) );
        h22r = ( float )( scaleR * sin( beta - alpha ) );
      }
      else {
        /* type 'B' rotation */
        float c, rho, mu, alpha, gamma;
        int i;

        i = pms->aaIidIndexMapped[env][bin];
        c = (float)pow(10.0,
                ((i)?(((i>0)?1:-1)*pQuantizedIIDs[((i>0)?i:-i)-1]):0.)/20.0);
        rho = quantizedRHOs[pms->aaIccIndexMapped[env][bin]];
        rho = max(rho, 0.05f);

        if (rho==0.0f && c==1.) {
          alpha = (float)PI/4.0f;
        }
        else {
          if (rho<=0.05f) {
            rho = 0.05f;
          }
          alpha = 0.5f*(float)atan((2.0f*c*rho)/(c*c-1.0f));

          if (alpha<0.) {
            alpha += (float)PI/2.0f;
          }
        }
        mu = c+1.0f/c;
        mu = 1+(4.0f*rho*rho-4.0f)/(mu*mu);
        gamma = (float)atan(sqrt((1.0f-sqrt(mu))/(1.0f+sqrt(mu))));
          
        h11r = ( float )( sqrt(2.) *  cos(alpha) * cos(gamma) );
        h12r = ( float )( sqrt(2.) *  sin(alpha) * cos(gamma) );
        h21r = ( float )( sqrt(2.) * -sin(alpha) * sin(gamma) );
        h22r = ( float )( sqrt(2.) *  cos(alpha) * sin(gamma) );
      }

      if ( bin >= aNoIpdBins[pms->freqResIpd] ) {
        h11i = h12i = h21i = h22i = 0.0f;
      }
      else {
        /* ipd/opd smoothing */
        ipd  = ( IPD_SCALE_FACTOR * 2.0f ) * pms->aaIpdIndexMapped[env][bin];
        opd  = ( OPD_SCALE_FACTOR * 2.0f ) * pms->aaOpdIndexMapped[env][bin];
        ipd1 = ( IPD_SCALE_FACTOR * 2.0f ) * pms->aIpdIndexMapped_1[bin];
        opd1 = ( OPD_SCALE_FACTOR * 2.0f ) * pms->aOpdIndexMapped_1[bin];
        ipd2 = ( IPD_SCALE_FACTOR * 2.0f ) * pms->aIpdIndexMapped_2[bin];
        opd2 = ( OPD_SCALE_FACTOR * 2.0f ) * pms->aOpdIndexMapped_2[bin];

        tempLeftReal  = cos(ipd);
        tempLeftImag  = sin(ipd);
        tempRightReal = cos(opd);
        tempRightImag = sin(opd);

        tempLeftReal  += PHASE_SMOOTH_HIST1 * cos(ipd1);
        tempLeftImag  += PHASE_SMOOTH_HIST1 * sin(ipd1);
        tempRightReal += PHASE_SMOOTH_HIST1 * cos(opd1);
        tempRightImag += PHASE_SMOOTH_HIST1 * sin(opd1);

        tempLeftReal  += PHASE_SMOOTH_HIST2 * cos(ipd2);
        tempLeftImag  += PHASE_SMOOTH_HIST2 * sin(ipd2);
        tempRightReal += PHASE_SMOOTH_HIST2 * cos(opd2);
        tempRightImag += PHASE_SMOOTH_HIST2 * sin(opd2);

        ipd = atan2( tempLeftImag , tempLeftReal  );
        opd = atan2( tempRightImag, tempRightReal );

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
      L = pms->aEnvStartStop[env + 1] - pms->aEnvStartStop[env];

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

      for ( i = pms->aEnvStartStop[env]; i < pms->aEnvStartStop[env + 1]; i++ ) {
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

    for ( bin = 0; bin < aNoIpdBins[pms->freqResIpd]; bin++ ) {
      pms->aIpdIndexMapped_2[bin] = pms->aIpdIndexMapped_1[bin];
      pms->aOpdIndexMapped_2[bin] = pms->aOpdIndexMapped_1[bin];
      pms->aIpdIndexMapped_1[bin] = pms->aaIpdIndexMapped[env][bin];
      pms->aOpdIndexMapped_1[bin] = pms->aaOpdIndexMapped[env][bin];
    }

  } /* envelopes loop */
}


void
tfInitPsDec(HANDLE_PS_DEC *hParametricStereoDec, int noSubSamples)
{
  CreatePsDec(hParametricStereoDec,
              0,
              64,
              noSubSamples,
              0);
}

/*--*/
