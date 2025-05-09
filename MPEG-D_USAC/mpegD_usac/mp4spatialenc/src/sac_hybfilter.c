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

#include<math.h>
#include"sac_hybfilter_enc.h"

const float sacHybFilterCoef8_enc[PROTO_LEN] = {
   0.00746082949812f,   0.02270420949825f,   0.04546865930473f,
   0.07266113929591f,   0.09885108575264f,   0.11793710567217f,
   0.12500000000000f,   0.11793710567217f,   0.09885108575264f,
   0.07266113929591f,   0.04546865930473f,   0.02270420949825f,
   0.00746082949812f};

const float sacHybFilterCoef2_enc[PROTO_LEN] = {
                0.0f,   0.01899487526049f,                0.0f,
  -0.07293139167538f,                0.0f,   0.30596630545168f,
                0.5f,   0.30596630545168f,                0.0f,
  -0.07293139167538f,                0.0f,   0.01899487526049f,
                0.0f};



static void sacKChannelFilteringAsym( const float *pQmfReal,
                                      const float *pQmfImag,
                                      float mHybridReal[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS],
                                      float mHybridImag[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS],
                                      int nSamples,
                                      int k,
                                      int bCplx,
                                      const float *p,
                                      int pLen,
                                      float offset);


void SacInitAnaHybFilterbank_enc(tHybFilterState *hybState)
{
    int k, n;

    for (k=0; k<MAX_QMF_BANDS_TO_HYBRID; k++)
    {
        for (n=0; n< PROTO_LEN-1+MAX_TIME_SLOTS; n++)
        {
            hybState->bufferLFReal[k][n] = 0.0;
            hybState->bufferLFImag[k][n] = 0.0;
        }
    }

    for (k=0; k<NUM_QMF_BANDS; k++)
    {
        for (n=0; n<(PROTO_LEN-1)/2+MAX_TIME_SLOTS; n++)
        {
            hybState->bufferHFReal[k][n] = 0.0;
            hybState->bufferHFImag[k][n] = 0.0;
        }
    }
}

void SacInitSynHybFilterbank_enc()
{
    ;
}

void SacApplyAnaHybFilterbank_enc(tHybFilterState *hybState,
                              float mQmfReal[MAX_TIME_SLOTS][NUM_QMF_BANDS],
                              float mQmfImag[MAX_TIME_SLOTS][NUM_QMF_BANDS],
                              int nrSamples,
                              float mHybridReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              float mHybridImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS])
{
    int nrSamplesShiftLF;
    int nrSamplesShiftHF;
    int nrQmfBandsLF;
    int k,n;

    float mTempOutputReal[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS];
    float mTempOutputImag[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS];

    nrSamplesShiftLF = BUFFER_LEN_LF - nrSamples;
    nrSamplesShiftHF = BUFFER_LEN_HF - nrSamples;

    nrQmfBandsLF = 3;


    for(k=0;k<nrQmfBandsLF;k++)
    {
        for(n=0;n<nrSamplesShiftLF;n++)
        {
            hybState->bufferLFReal[k][n] = hybState->bufferLFReal[k][n+nrSamples];
            hybState->bufferLFImag[k][n] = hybState->bufferLFImag[k][n+nrSamples];
        }
    }

    for(k=0;k<nrQmfBandsLF;k++)
    {
        for(n=0;n<nrSamples;n++)
        {
            hybState->bufferLFReal[k][n+nrSamplesShiftLF] = mQmfReal[n][k];
            hybState->bufferLFImag[k][n+nrSamplesShiftLF] = mQmfImag[n][k];
        }
    }


    for(k=0;k<NUM_QMF_BANDS-nrQmfBandsLF;k++)
    {
        for(n=0;n<nrSamplesShiftHF;n++)
        {
            hybState->bufferHFReal[k][n] = hybState->bufferHFReal[k][n+nrSamples];
            hybState->bufferHFImag[k][n] = hybState->bufferHFImag[k][n+nrSamples];
        }
    }

    for(k=0;k<NUM_QMF_BANDS-nrQmfBandsLF;k++)
    {
        for(n=0;n<nrSamples;n++)
        {
            hybState->bufferHFReal[k][n+nrSamplesShiftHF] = mQmfReal[n][k+nrQmfBandsLF];
            hybState->bufferHFImag[k][n+nrSamplesShiftHF] = mQmfImag[n][k+nrQmfBandsLF];
        }
    }


    sacKChannelFilteringAsym(&(hybState->bufferLFReal[0][nrSamplesShiftLF+1-PROTO_LEN]),
                             &(hybState->bufferLFImag[0][nrSamplesShiftLF+1-PROTO_LEN]),
                             mTempOutputReal,
                             mTempOutputImag,
                             nrSamples,
                             8,
                             1,
                             sacHybFilterCoef8_enc,
                             PROTO_LEN,
                             (float)(PROTO_LEN-1)/2);

    for (k=0; k<2; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k] = mTempOutputReal[k+6][n];
            mHybridImag[n][k] = mTempOutputImag[k+6][n];
        }
    }
    for (k=0; k<2; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k+2] = mTempOutputReal[k][n];
            mHybridImag[n][k+2] = mTempOutputImag[k][n];
        }
    }
    for (k=0; k<2; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k+4] = mTempOutputReal[k+2][n];
            mHybridImag[n][k+4] = mTempOutputImag[k+2][n];

            mHybridReal[n][k+4] += mTempOutputReal[5-k][n];
            mHybridImag[n][k+4] += mTempOutputImag[5-k][n];
        }
    }

    sacKChannelFilteringAsym(&(hybState->bufferLFReal[1][nrSamplesShiftLF+1-PROTO_LEN]),
                             &(hybState->bufferLFImag[1][nrSamplesShiftLF+1-PROTO_LEN]),
                             mTempOutputReal,
                             mTempOutputImag,
                             nrSamples,
                             2,
                             0,
                             sacHybFilterCoef2_enc,
                             PROTO_LEN,
                             (float)(PROTO_LEN-1)/2);

    for (k=0; k<2; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k+6] = mTempOutputReal[1-k][n];
            mHybridImag[n][k+6] = mTempOutputImag[1-k][n];
        }
    }

    sacKChannelFilteringAsym(&(hybState->bufferLFReal[2][nrSamplesShiftLF+1-PROTO_LEN]),
                             &(hybState->bufferLFImag[2][nrSamplesShiftLF+1-PROTO_LEN]),
                             mTempOutputReal,
                             mTempOutputImag,
                             nrSamples,
                             2,
                             0,
                             sacHybFilterCoef2_enc,
                             PROTO_LEN,
                             (float)(PROTO_LEN-1)/2);

    for (k=0; k<2; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k+8] = mTempOutputReal[k][n];
            mHybridImag[n][k+8] = mTempOutputImag[k][n];
        }
    }

    for (k=0; k<61; k++)
    {
        for (n=0; n<nrSamples; n++)
        {
            mHybridReal[n][k+10] = hybState->bufferHFReal[k][n+nrSamplesShiftHF-(PROTO_LEN-1)/2];
            mHybridImag[n][k+10] = hybState->bufferHFImag[k][n+nrSamplesShiftHF-(PROTO_LEN-1)/2];
        }
    }
}

void SacApplySynHybFilterbank_enc(float mHybridReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              float mHybridImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                              int nrSamples,
                              float mQmfReal[MAX_TIME_SLOTS][NUM_QMF_BANDS],
                              float mQmfImag[MAX_TIME_SLOTS][NUM_QMF_BANDS])
{
    int k, n;

    for (n=0; n<nrSamples; n++)
    {
        mQmfReal[n][0] = mHybridReal[n][0];
        mQmfImag[n][0] = mHybridImag[n][0];

        for (k=1; k<6; k++)
        {
            mQmfReal[n][0] += mHybridReal[n][k];
            mQmfImag[n][0] += mHybridImag[n][k];
        }

        mQmfReal[n][1] = mHybridReal[n][6] + mHybridReal[n][7];
        mQmfImag[n][1] = mHybridImag[n][6] + mHybridImag[n][7];

        mQmfReal[n][2] = mHybridReal[n][8] + mHybridReal[n][9];
        mQmfImag[n][2] = mHybridImag[n][8] + mHybridImag[n][9];

        for (k=0; k<61; k++)
        {
            mQmfReal[n][k+3] = mHybridReal[n][k+10];
            mQmfImag[n][k+3] = mHybridImag[n][k+10];
        }
    }
}


void SacCloseAnaHybFilterbank_enc()
{
    ;
}

void SacCloseSynHybFilterbank_enc()
{
    ;
}



static void sacKChannelFilteringAsym( const float *pQmfReal,
                                      const float *pQmfImag,
                                      float mHybridReal[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS],
                                      float mHybridImag[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS],
                                      int nSamples,
                                      int k,
                                      int bCplx,
                                      const float *p,
                                      int pLen,
                                      float offset)
{
    int i, n, q;
    float real, imag;
    float modr, modi;

    for(i = 0; i < nSamples; i++) {
        for(q = 0; q < k; q++) {
            real = 0.0;
            imag = 0.0;
            for(n = 0; n < pLen; n++) {
                if (bCplx)
                {
                    modr = (float) cos(PI*(n-offset)/k*(1+2*q));
                    modi = (float) -sin(PI*(n-offset)/k*(1+2*q));
                }
                else{
                    modr = (float) cos(2*PI*q*(n-offset)/k);
                    modi = 0.0;
                }
                real += p[pLen-1-n] * ( pQmfReal[n+i] * modr - pQmfImag[n+i] * modi );
                imag += p[pLen-1-n] * ( pQmfImag[n+i] * modr + pQmfReal[n+i] * modi );
            }
            mHybridReal[q][i] = real;
            mHybridImag[q][i] = imag;
        }
    }
}
