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

#include <math.h>
#include "qmflib_hybfilter.h"




static const float QMFlib_HybFilterCoef8[QMFLIB_PROTO_LEN] = {
   0.00746082949812f,   0.02270420949825f,   0.04546865930473f,
   0.07266113929591f,   0.09885108575264f,   0.11793710567217f,
   0.12500000000000f,   0.11793710567217f,   0.09885108575264f,
   0.07266113929591f,   0.04546865930473f,   0.02270420949825f,
   0.00746082949812f};

static const float QMFlib_HybFilterCoef2[QMFLIB_PROTO_LEN] = {
                0.0f,   0.01899487526049f,                0.0f,
  -0.07293139167538f,                0.0f,   0.30596630545168f,
                0.5f,   0.30596630545168f,                0.0f,
  -0.07293139167538f,                0.0f,   0.01899487526049f,
                0.0f};


static const float QMFlib_HybFilterCoef4[QMFLIB_PROTO_LEN] = {
  -0.00305151927305f,  -0.00794862316203f,                0.0f,
  0.04318924038756f,   0.12542448210445f,   0.21227807049160f,
  0.25f,      0.21227807049160f,   0.12542448210445f,
  0.04318924038756f,                0.0f,  -0.00794862316203f,
  -0.00305151927305f};



static const int QMFlib_hybrid2qmf_THREE_TO_TEN[] = {
   0,   0,   0,   0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,
   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,
  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,
  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
 121, 122, 123, 124, 125, 126, 127
};


static const int QMFlib_hybrid2qmf_THREE_TO_SIXTEEN[] =
{
  
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1,
  2, 2, 2, 2,
  3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
  24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
  45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,
  66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,
  87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,
  106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,
  122,123,124,125,126,127
  
};




static void QMFlib_KChannelFilteringAsym( const float *pQmfReal,
                                      const float *pQmfImag,
                                          float mHybridReal[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF],
                                      float mHybridImag[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF],
                                      int k,
                                      int bCplx,
                                      const float *p,
                                      int pLen,
                                      float offset);


int QMFlib_GetHybridSubbands(int qmfSubbands, QMFLIB_HYBRID_FILTER_MODE hybridMode)
{
  if ( hybridMode == QMFLIB_HYBRID_THREE_TO_TEN )
    return qmfSubbands - QMFLIB_QMF_BANDS_TO_HYBRID + 10;
  else if ( hybridMode == QMFLIB_HYBRID_THREE_TO_SIXTEEN )
    return qmfSubbands - QMFLIB_QMF_BANDS_TO_HYBRID + 16;
  else
    return -1;
}

int QMFlib_GetQmfSubband(int hybridSubband, QMFLIB_HYBRID_FILTER_MODE hybridMode, int LdMode)
{
  if (LdMode == 1)
    return hybridSubband;
  else
    {
      if ( hybridMode == QMFLIB_HYBRID_THREE_TO_TEN )
        return QMFlib_hybrid2qmf_THREE_TO_TEN[hybridSubband];
      else if  ( hybridMode == QMFLIB_HYBRID_THREE_TO_SIXTEEN )
        return QMFlib_hybrid2qmf_THREE_TO_SIXTEEN[hybridSubband];
      else
        return -1;
    
    }
}

int QMFlib_GetParameterPhase(int hybridSubband)
{
  return (hybridSubband < 2)? -1: 1;
}


void QMFlib_InitAnaHybFilterbank(QMFLIB_HYBRID_FILTER_STATE *hybState)
{
  int k, n;

    
  for (k=0; k<QMFLIB_QMF_BANDS_TO_HYBRID; k++)
    {
      for (n=0; n< QMFLIB_BUFFER_LEN_LF; n++)
        {
          hybState->bufferLFReal[k][n] = 0.0f;
          hybState->bufferLFImag[k][n] = 0.0f;
        }
    }

    
  for (k=0; k<QMFLIB_MAX_NUM_QMF_BANDS; k++)
    {
      for (n=0; n<QMFLIB_BUFFER_LEN_HF; n++)
        {
          hybState->bufferHFReal[k][n] = 0.0;
          hybState->bufferHFImag[k][n] = 0.0;
        }
    }
}

void QMFlib_ApplyAnaHybFilterbank(QMFLIB_HYBRID_FILTER_STATE *hybState,
                                  QMFLIB_HYBRID_FILTER_MODE hybridMode,
                                  int nrBands,
                                  int delayAlign,
                                  float mQmfReal[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mQmfImag[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mHybridReal[QMFLIB_MAX_HYBRID_BANDS],
                                  float mHybridImag[QMFLIB_MAX_HYBRID_BANDS])
{
  int nrSamplesShiftLF;   
  int nrSamplesShiftHF;   
  int nrQmfBandsLF;       
  int k,n;

  float mTempOutputReal[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF];
  float mTempOutputImag[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF];

  nrSamplesShiftLF = QMFLIB_PROTO_LEN - 1;
  nrSamplesShiftHF = (QMFLIB_PROTO_LEN - 1) / 2;

  nrQmfBandsLF = QMFLIB_QMF_BANDS_TO_HYBRID;

    
 
  for(k=0;k<nrQmfBandsLF;k++)
    {
      for(n=0;n<nrSamplesShiftLF;n++)
        {
          hybState->bufferLFReal[k][n] = hybState->bufferLFReal[k][n+1];
          hybState->bufferLFImag[k][n] = hybState->bufferLFImag[k][n+1];
        }
    }

  for(k=0;k<nrQmfBandsLF;k++)      
    {
      hybState->bufferLFReal[k][nrSamplesShiftLF] = mQmfReal[k];
      hybState->bufferLFImag[k][nrSamplesShiftLF] = mQmfImag[k];
    }

    
  for(k=0;k<nrBands-nrQmfBandsLF;k++)
    {
      for(n=0;n<nrSamplesShiftHF;n++)
        {
          hybState->bufferHFReal[k][n] = hybState->bufferHFReal[k][n+1];
          hybState->bufferHFImag[k][n] = hybState->bufferHFImag[k][n+1];
        }
    }

  for(k=0;k<nrBands-nrQmfBandsLF;k++)      
    {
      hybState->bufferHFReal[k][nrSamplesShiftHF] = mQmfReal[k+nrQmfBandsLF];
      hybState->bufferHFImag[k][nrSamplesShiftHF] = mQmfImag[k+nrQmfBandsLF];
    }


  if (hybridMode == QMFLIB_HYBRID_THREE_TO_TEN)
    {
      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[0][0]),
                                   &(hybState->bufferLFImag[0][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   8,
                                   1,
                                   QMFlib_HybFilterCoef8,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);

      for (k=0; k<2; k++)                                      
        {
          mHybridReal[k] = mTempOutputReal[k+6];
          mHybridImag[k] = mTempOutputImag[k+6];
        }
      for (k=0; k<2; k++)                                      
        {
          mHybridReal[k+2] = mTempOutputReal[k];
          mHybridImag[k+2] = mTempOutputImag[k];
        }

      for (k=0; k<2; k++)                                      
        {
          mHybridReal[k+4] = mTempOutputReal[k+2];   
          mHybridImag[k+4] = mTempOutputImag[k+2];

          mHybridReal[k+4] += mTempOutputReal[5-k];
          mHybridImag[k+4] += mTempOutputImag[5-k];
        }

      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[1][0]),
                                   &(hybState->bufferLFImag[1][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   2,
                                   0,
                                   QMFlib_HybFilterCoef2,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);

      for (k=0; k<2; k++)                                      
        {
          mHybridReal[k+6] = mTempOutputReal[1-k];
          mHybridImag[k+6] = mTempOutputImag[1-k];
        }

      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[2][0]),
                                   &(hybState->bufferLFImag[2][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   2,
                                   0,
                                   QMFlib_HybFilterCoef2,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);

      for (k=0; k<2; k++)
        {
          mHybridReal[k+8] = mTempOutputReal[k];
          mHybridImag[k+8] = mTempOutputImag[k];
        }

      for (k=0; k<nrBands-nrQmfBandsLF; k++) 
        {
          if ( delayAlign )
            {
              mHybridReal[k+10] = hybState->bufferHFReal[k][0];
              mHybridImag[k+10] = hybState->bufferHFImag[k][0];
            }
          else
            {
              mHybridReal[k+10] = hybState->bufferHFReal[k][nrSamplesShiftHF];
              mHybridImag[k+10] = hybState->bufferHFImag[k][nrSamplesShiftHF];
            }
        }

    }
  
  else if (hybridMode == QMFLIB_HYBRID_THREE_TO_SIXTEEN)
    {
      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[0][0]),
                                   &(hybState->bufferLFImag[0][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   8,
                                   1,
                                   QMFlib_HybFilterCoef8,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);
      
      for (k=0; k<8; k++)
        {
          mHybridReal[k] = mTempOutputReal[k];
          mHybridImag[k] = mTempOutputImag[k];
        }
    
      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[1][0]),
                                   &(hybState->bufferLFImag[1][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   4,
                                   1,
                                   QMFlib_HybFilterCoef4,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);

      
      for (k=0; k<4; k++)
        {
          mHybridReal[k+8] = mTempOutputReal[k];
          mHybridImag[k+8] = mTempOutputImag[k];
        }
    
      QMFlib_KChannelFilteringAsym(&(hybState->bufferLFReal[2][0]),
                                   &(hybState->bufferLFImag[2][0]),
                                   mTempOutputReal,
                                   mTempOutputImag,
                                   4,
                                   1,
                                   QMFlib_HybFilterCoef4,
                                   QMFLIB_PROTO_LEN,
                                   (float)(QMFLIB_PROTO_LEN-1)/2);
      
      
      for (k=0; k<4; k++)
        {
          mHybridReal[k+12] = mTempOutputReal[k];
          mHybridImag[k+12] = mTempOutputImag[k];
        }
 
      for (k=0; k<nrBands-nrQmfBandsLF; k++) 
        {
          if ( delayAlign )
            {
              mHybridReal[k+16] = hybState->bufferHFReal[k][0];
              mHybridImag[k+16] = hybState->bufferHFImag[k][0];
            }
          else
            {
              mHybridReal[k+16] = hybState->bufferHFReal[k][nrSamplesShiftHF];
              mHybridImag[k+16] = hybState->bufferHFImag[k][nrSamplesShiftHF];
            }
        }
    }    
  else
    {
      return;
    }
}

void QMFlib_ApplySynHybFilterbank(int nrBands,
                                  QMFLIB_HYBRID_FILTER_MODE hybridMode,
                                  float mHybridReal[QMFLIB_MAX_HYBRID_BANDS],
                                  float mHybridImag[QMFLIB_MAX_HYBRID_BANDS],
                                  float mQmfReal[QMFLIB_MAX_NUM_QMF_BANDS],
                                  float mQmfImag[QMFLIB_MAX_NUM_QMF_BANDS])
{
  int k;
  
  if (hybridMode == QMFLIB_HYBRID_THREE_TO_TEN)
    {
      mQmfReal[0] = mHybridReal[0];
      mQmfImag[0] = mHybridImag[0];

      for (k=1; k<6; k++)
        {
          mQmfReal[0] += mHybridReal[k];
          mQmfImag[0] += mHybridImag[k];
        }

      mQmfReal[1] = mHybridReal[6] + mHybridReal[7];
      mQmfImag[1] = mHybridImag[6] + mHybridImag[7];

      mQmfReal[2] = mHybridReal[8] + mHybridReal[9];
      mQmfImag[2] = mHybridImag[8] + mHybridImag[9];

      for (k=3; k<nrBands; k++)
        {
          mQmfReal[k] = mHybridReal[k-3+10];
          mQmfImag[k] = mHybridImag[k-3+10];
        }
    }
  else if (hybridMode == QMFLIB_HYBRID_THREE_TO_SIXTEEN) 
    {
      mQmfReal[0] = mHybridReal[0];             
      mQmfImag[0] = mHybridImag[0];
      
      for (k=1; k<8; k++){
        mQmfReal[0] += mHybridReal[k];
        mQmfImag[0] += mHybridImag[k];
      }
      
      mQmfReal[1] = mHybridReal[8];             
      mQmfImag[1] = mHybridImag[8];
      
      mQmfReal[2] = mHybridReal[12];
      mQmfImag[2] = mHybridImag[12];
      
      for (k=1; k<4; k++){
        mQmfReal[1] += mHybridReal[k+8];
        mQmfImag[1] += mHybridImag[k+8];
        
        mQmfReal[2] += mHybridReal[k+12];
        mQmfImag[2] += mHybridImag[k+12];
      }
   
      for (k=3; k<nrBands; k++)
        {
          mQmfReal[k] = mHybridReal[k-3+16];
          mQmfImag[k] = mHybridImag[k-3+16];
        }
    }
  else
    {
      return;
    }
}


static void QMFlib_KChannelFilteringAsym( const float *pQmfReal,
                                          const float *pQmfImag,
                                          float mHybridReal[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF],
                                          float mHybridImag[QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF],
                                          int k,
                                          int bCplx,
                                          const float *p,
                                          int pLen,
                                          float offset)
{
  int n, q;
  float real, imag;
  float modr, modi;

  for(q = 0; q < k; q++) {
    real = 0.0;
    imag = 0.0;
    for(n = 0; n < pLen; n++) {
      if (bCplx)
        {
          modr = (float) cos(QMFLIB_PI*(n-offset)/k*(1+2*q));
          modi = (float) -sin(QMFLIB_PI*(n-offset)/k*(1+2*q));
        }
      else{
        modr = (float) cos(2*QMFLIB_PI*q*(n-offset)/k);
        modi = 0.0;
      }
      real += p[pLen-1-n] * ( pQmfReal[n] * modr - pQmfImag[n] * modi );
      imag += p[pLen-1-n] * ( pQmfImag[n] * modr + pQmfReal[n] * modi );
    }
    mHybridReal[q] = real;
    mHybridImag[q] = imag;
  }
}
