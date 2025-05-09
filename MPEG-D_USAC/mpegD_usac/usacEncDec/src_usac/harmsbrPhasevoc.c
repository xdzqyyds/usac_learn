/*******************************************************************************
This software module was originally developed by

Dolby Laboratories, Fraunhofer IIS, VoiceAge Corp.


Initial author:

and edited by:

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

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp.
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: harmsbrPhasevoc.c,v 1.23 2011-08-18 14:21:03 mul Exp $
*******************************************************************************/

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "proto_func.h"
#include "harmsbrPhasevoc.h"
#include "harmsbrWingen.h"
#include "harmsbrMathlib.h"
#include "ct_polyphase.h"

#ifndef MAX_NUM_PATCHES
#include "ct_hfgen.h"
#endif

#ifndef MAX_STRETCH
#define MAX_STRETCH 4
#endif

#ifndef LO
#define LO 0
#endif
#ifndef HI
#define HI 1
#endif

static const float qThr = 4.0f;
static const int   inHopSizeDivisor = 8;              /* Relation of hop size and input window size */
static const float fdOversamplingFactor = (2.0f+1)/2; /* FD oversampling factor */
static const int   transSamp[2] = { 12, 18 };         /* FD transition samples */

static const int startSubband2kL[33] =
{
  0, 0, 0, 0, 0, 0, 0,  2,  2,  2,  4,  4,  4,  4,  4,  6,
  6, 6, 8, 8, 8, 8, 8, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12
};

extern const double stdSbrDecoderFilterbankCoefficients[640];


/**************************************************************************/
/*!  \brief     Parameters used for Phase Vocoder                         */
/**************************************************************************/
struct hbeTransposer {

  int xOverQmf[MAX_NUM_PATCHES];
  int maxStretch;           /* Maximal stretching factor */
  int inHopSize;            /* Analysis hopsize */
  int fftSize[2];           /* Nominal analysis FFT size */
  int bSbr41;


  float *analysisWin;       /* Phase Vocoder Analysis Window for FFT */
  float *synthesisWin;      /* Phase Vocoder Synthesis Window for OLA */

  float *spectrum;          /* FFT values in cartesian space */
  float *spectrumTransposed;/* Transposed spectrum */
  float *mag;               /* FFT magnitudes */
  float *phase;             /* FFT angles */

  float ***fdWin;            /* FD windows */
  int   **xOverBin;          /* xOverBin array */

  float *inBuf;
  float *outBuf;

  int anaFFTSize[2];        /* Analysis FFT length */
  int synFFTSize[2];        /* Synthesis FFT length */
  int outHopSize;

  int sStart;
  int synthSize;
  double synthBuffer[1280];
  struct N NN;
  double *synthesisProtFilter;

  int aStart;
  int analySize;
  double analaBuffer[640];
  struct M2 MM;
  double *analysisProtFilter;
};

static float *trigPtrFft[2]    = { NULL };
static float *trigPtrSynFft[2] = { NULL };


static void multFLOAT(const float *X, const float *Y, float *Z, int n)
{
  int i;
  for (i = 0; i < n; i++) {
    Z[i] = X[i] * Y[i];
  }
}


/**************************************************************************/
/*!  \brief     Creation of Phase Vocoder Handle                          */
/**************************************************************************/
int
PhaseVocoderCreate(HANDLE_HBE_TRANSPOSER *hPhaseVocoder,
                   const int fftSize,
                   int bSbr41)          /* Analysis FFT length */
{
  HANDLE_HBE_TRANSPOSER hPhVoc;

  int stretch = 0;
  int i;

  if (hPhaseVocoder) {
    hPhVoc = (HANDLE_HBE_TRANSPOSER) calloc(1, sizeof(struct hbeTransposer) );

    hPhVoc->fftSize[0] = fftSize;
    hPhVoc->fftSize[1] = (int) (fdOversamplingFactor * fftSize);

    if (NULL == (hPhVoc->analysisWin     = (float*)  calloc(fftSize, sizeof(float)))) return -1;
    if (fftSize == 768) {
      if (NULL == (hPhVoc->synthesisWin    = (float*)  calloc(4*fftSize/3, sizeof(float)))) return -1;
    } else {
      if (NULL == (hPhVoc->synthesisWin    = (float*)  calloc((bSbr41+1)*fftSize, sizeof(float)))) return -1;    
    }
    if (NULL == (hPhVoc->spectrum        = (float*)  calloc(hPhVoc->fftSize[1], sizeof(float)))) return -1;

    if (fftSize == 768) {
      if (NULL == (hPhVoc->spectrumTransposed   = (float*)  calloc(4*hPhVoc->fftSize[1]/3+2, sizeof(float)))) return -1;
    } else {
      if (NULL == (hPhVoc->spectrumTransposed   = (float*)  calloc((bSbr41+1)*hPhVoc->fftSize[1]+2, sizeof(float)))) return -1;
    }
    if (fftSize == 768) {
      if (NULL == (hPhVoc->mag             = (float*)  calloc(4*(hPhVoc->fftSize[1]/2)/3+2, sizeof(float)))) return -1;
      if (NULL == (hPhVoc->phase           = (float*)  calloc(4*(hPhVoc->fftSize[1]/2)/3+2, sizeof(float)))) return -1;
    } else {
      if (NULL == (hPhVoc->mag             = (float*)  calloc((bSbr41+1)*(hPhVoc->fftSize[1]/2)+2, sizeof(float)))) return -1;
      if (NULL == (hPhVoc->phase           = (float*)  calloc((bSbr41+1)*(hPhVoc->fftSize[1]/2)+2, sizeof(float)))) return -1;
    }
    if (NULL == (hPhVoc->inBuf           = (float*)  calloc(2*fftSize,   sizeof(float)))) return -1;
    if (fftSize == 768) {
      if (NULL == (hPhVoc->outBuf         = (float*) calloc(4*4*fftSize/3, sizeof(float)))) return -1;
    } else {
      if (NULL == (hPhVoc->outBuf         = (float*) calloc((bSbr41+1)*4*fftSize, sizeof(float)))) return -1;
    }
    if (NULL == (hPhVoc->fdWin          = (float***) calloc(MAX_STRETCH-1, sizeof(float**)))) return -1;
    if (NULL == (hPhVoc->xOverBin         = (int**) calloc(MAX_STRETCH, sizeof(int*))))  return -1;

    for (stretch = 0; stretch < MAX_STRETCH; stretch++) {
        hPhVoc->xOverBin[stretch] = (int*) calloc(2, sizeof(int));
    };
    for (stretch = 2;stretch <= MAX_STRETCH;stretch++) {
      hPhVoc->fdWin[stretch-2] = (float**) calloc(2, sizeof(float*));
      for (i=0; i<2; i++) {
        hPhVoc->fdWin[stretch-2][i] = (float*) calloc(hPhVoc->fftSize[i]+1, sizeof(float));
      }
    }
    /*--------------------------------------------------------------------------------------------*/

    hPhVoc->bSbr41 = bSbr41;
    *hPhaseVocoder = hPhVoc;
  }

  return 0;
}

/**************************************************************************/
/*!  \brief     Reinit Phase Vocoder if xover has been changed            */
/**************************************************************************/
void PhaseVocoderReInit(HANDLE_HBE_TRANSPOSER hPhaseVocoder,
                        int FreqBandTable[2][MAX_FREQ_COEFFS + 1],
                        int NSfb[2],
                        int bSbr41)
{
  float *dataInput  = hPhaseVocoder->inBuf;
  int sfb;
  int patch;
  int i;
  int k,l,L,tempStart;
  float fbRatio;
  int stopPatch;

  int startBand = FreqBandTable[LO][0];
  int stopBand  = FreqBandTable[LO][NSfb[LO]];
  
  hPhaseVocoder->synthSize = 4*((startBand+4)/8+1);
  hPhaseVocoder->sStart = startSubband2kL[startBand];

  hPhaseVocoder->bSbr41 = bSbr41;

  if (bSbr41) {
    if(hPhaseVocoder->sStart + hPhaseVocoder->synthSize > 16)
      hPhaseVocoder->sStart = 16-hPhaseVocoder->synthSize;

    fbRatio = hPhaseVocoder->synthSize/16.0f;
  } else if(hPhaseVocoder->fftSize[0] == 768) {
    if(hPhaseVocoder->sStart + hPhaseVocoder->synthSize > 24)
      hPhaseVocoder->sStart = 24-hPhaseVocoder->synthSize;

    fbRatio = hPhaseVocoder->synthSize/24.0f;
  } else {
    fbRatio = hPhaseVocoder->synthSize/32.0f;
  }

  hPhaseVocoder->anaFFTSize[0] = (int) (fbRatio*hPhaseVocoder->fftSize[0]);
  hPhaseVocoder->anaFFTSize[1] = (int) (fbRatio*hPhaseVocoder->fftSize[1]);

  hPhaseVocoder->inHopSize     = hPhaseVocoder->anaFFTSize[0]/inHopSizeDivisor;

  DestroySineTable(trigPtrFft[0]);
  DestroySineTable(trigPtrFft[1]);

  trigPtrFft[0] = CreateSineTable(hPhaseVocoder->anaFFTSize[0]);
  trigPtrFft[1] = CreateSineTable(hPhaseVocoder->anaFFTSize[1]);

  for (i=0; i<hPhaseVocoder->anaFFTSize[0]; i++) {
    const double gain = 1.0;

    hPhaseVocoder->analysisWin[i] = (float) (gain*sin( acos(-1.0)/hPhaseVocoder->anaFFTSize[0] * (i + 0.5)));
  }

  memset(hPhaseVocoder->synthBuffer, 0, 1280*sizeof(double));
  L = hPhaseVocoder->synthSize;

  for ( l=0; l<2*L; l++ ) {
    for ( k=0; k<L; k++ ) {
      hPhaseVocoder->NN.real[l][k] = 1.0/L * cos(PI/(2*L)*(k+0.5)*(2*l-L));
    }
  }

  if(hPhaseVocoder->synthesisProtFilter) free(hPhaseVocoder->synthesisProtFilter);
  hPhaseVocoder->synthesisProtFilter = interpolPrototype( stdSbrDecoderFilterbankCoefficients,
                                                          640, 64, L, 0);

  tempStart = 2*((startBand-1)/2); /* Largest start band */
  hPhaseVocoder->analySize = 4*((min(64, stopBand+1)-tempStart-1)/4+1);   /* Quantize in steps of 4 */
  hPhaseVocoder->aStart    = tempStart - max(0,tempStart+hPhaseVocoder->analySize-64);

  fbRatio = hPhaseVocoder->analySize/64.0f;

  if (bSbr41) {
    fbRatio *= 2;
  } else if (hPhaseVocoder->fftSize[0] == 768) {
    fbRatio *= (4.f/3.f);
  }

  hPhaseVocoder->synFFTSize[0] = (int) (fbRatio*hPhaseVocoder->fftSize[0]);
  hPhaseVocoder->synFFTSize[1] = (int) (fbRatio*hPhaseVocoder->fftSize[1]);

  hPhaseVocoder->outHopSize  = 2*hPhaseVocoder->synFFTSize[0]/inHopSizeDivisor;

  DestroySineTable(trigPtrSynFft[0]);
  DestroySineTable(trigPtrSynFft[1]);

  trigPtrSynFft[0] = CreateSineTable(hPhaseVocoder->synFFTSize[0]);
  trigPtrSynFft[1] = CreateSineTable(hPhaseVocoder->synFFTSize[1]);

    /* create windows analysis and synthesis */
  for (i=0; i<hPhaseVocoder->synFFTSize[0]; i++) {
    const double gain = 1.0;

    hPhaseVocoder->synthesisWin[i] = (float) (gain*sin( acos(-1.0)/hPhaseVocoder->synFFTSize[0] * (i + 0.5)));
  }

  memset(hPhaseVocoder->analaBuffer, 0, 640*sizeof(double));
  L = hPhaseVocoder->analySize;
  for ( k=0; k<L; k++ ) {
    for ( l=0; l<2*L; l++ ) {
      hPhaseVocoder->MM.real[k][l] = cos(PI/(2*L)*((k+0.5)*(2*l-L/64.0)-L/64.0*hPhaseVocoder->aStart));
      hPhaseVocoder->MM.imag[k][l] = sin(PI/(2*L)*((k+0.5)*(2*l-L/64.0)-L/64.0*hPhaseVocoder->aStart));
    }
  }
  if(hPhaseVocoder->analysisProtFilter) free(hPhaseVocoder->analysisProtFilter);
  hPhaseVocoder->analysisProtFilter = interpolPrototype( stdSbrDecoderFilterbankCoefficients,
                                                         640, 64, L, 0);

  /* calculation of xOverBin array and xOverQmf array */ /* todo: maybe use 2*fftSize instead of fftSize for correct calculation of xoverbins */
  memset(hPhaseVocoder->xOverQmf, 0, MAX_NUM_PATCHES*sizeof(int)); /* global */
  for (i=0; i< MAX_STRETCH; i++) {
    memset(hPhaseVocoder->xOverBin[i], 0, 2*sizeof(int));
  }
  sfb=0;
  if (bSbr41) {
    stopPatch = MAX_NUM_PATCHES;
    hPhaseVocoder->maxStretch = MAX_STRETCH;
  } else {
    stopPatch = MAX_STRETCH;
  }
  for(patch=1; patch <= stopPatch; patch++) {
    while(sfb <= NSfb[LO] && FreqBandTable[LO][sfb] <= patch*startBand) sfb++;
    if(sfb <= NSfb[LO])
    {
      /* If the distance is larger than three QMF bands - try aligning to high resolution frequency bands instead. */
      if( (patch*startBand-FreqBandTable[LO][sfb-1]) <= 3)
      {
        hPhaseVocoder->xOverQmf[patch-1] = FreqBandTable[LO][sfb-1];
        if( bSbr41 && (patch <= MAX_STRETCH)) {
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * 2 * FreqBandTable[LO][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * 2 * FreqBandTable[LO][sfb-1]/128 + 0.5);
        } else if((hPhaseVocoder->fftSize[0] == 768) && (patch <= MAX_STRETCH)) {
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * (4.f/3.f) * FreqBandTable[LO][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * (4.f/3.f) * FreqBandTable[LO][sfb-1]/128 + 0.5);  
        } else if(patch <= MAX_STRETCH){
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * FreqBandTable[LO][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * FreqBandTable[LO][sfb-1]/128 + 0.5);
        }
      }
      else
      {
        int sfb=0;
        while(sfb <= NSfb[HI] && FreqBandTable[HI][sfb] <= patch*startBand) sfb++;
        hPhaseVocoder->xOverQmf[patch-1] = FreqBandTable[HI][sfb-1];
        if( bSbr41 && (patch <= MAX_STRETCH)) {
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * 2 * FreqBandTable[HI][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * 2 * FreqBandTable[HI][sfb-1]/128 + 0.5);
        } else if((hPhaseVocoder->fftSize[0] == 768) && (patch <= MAX_STRETCH)) {
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * (4.f/3.f) * FreqBandTable[HI][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * (4.f/3.f) * FreqBandTable[HI][sfb-1]/128 + 0.5);
        } else if(patch <= MAX_STRETCH) {
          hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * FreqBandTable[HI][sfb-1]/128 + 0.5);
          hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * FreqBandTable[HI][sfb-1]/128 + 0.5);
        }
      }
    }
    else
    {
      hPhaseVocoder->xOverQmf[patch-1]    = stopBand;
      if( bSbr41 && (patch <= MAX_STRETCH)) {
        hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * 2 *stopBand/128 + 0.5);
        hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * 2 *stopBand/128 + 0.5);      
      } else if((hPhaseVocoder->fftSize[0] == 768) && (patch <= MAX_STRETCH)) {
        hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * (4.f/3.f) * stopBand/128 + 0.5);
        hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * (4.f/3.f) * stopBand/128 + 0.5);  
      } else if(patch <= MAX_STRETCH) {
        hPhaseVocoder->xOverBin[patch-1][0] = (int) (hPhaseVocoder->fftSize[0] * stopBand/128 + 0.5);
        hPhaseVocoder->xOverBin[patch-1][1] = (int) (hPhaseVocoder->fftSize[1] * stopBand/128 + 0.5);
      }
      hPhaseVocoder->maxStretch = min(patch, MAX_STRETCH);
      break;
    }
  }

  /* calculation of frequency domain windows */
  for (patch=0; patch<hPhaseVocoder->maxStretch-1; patch++) {
    for (i=0; i<2; i++) {
      createFreqDomainWin(hPhaseVocoder->fdWin[patch][i],
                          hPhaseVocoder->xOverBin[patch][i],
                          hPhaseVocoder->xOverBin[patch+1][i],
                          transSamp[i],
                          hPhaseVocoder->fftSize[i]);
    }
  }

}

/**************************************************************************/
/*!  \brief     Close Phase Vocoder                                       */
/**************************************************************************/
void PhaseVocoderClose(HANDLE_HBE_TRANSPOSER hPhaseVocoder)
{
  int stretch = 0;
  int i;

  DestroySineTable(trigPtrFft[0]);
  DestroySineTable(trigPtrFft[1]);

  DestroySineTable(trigPtrSynFft[0]);
  DestroySineTable(trigPtrSynFft[1]);
  free(hPhaseVocoder->synthesisProtFilter);
  free(hPhaseVocoder->analysisProtFilter);

  free(hPhaseVocoder->analysisWin);
  free(hPhaseVocoder->synthesisWin);

  free(hPhaseVocoder->spectrum);
  free(hPhaseVocoder->spectrumTransposed);
  free(hPhaseVocoder->mag);
  free(hPhaseVocoder->phase);
  free(hPhaseVocoder->inBuf);

  for (stretch = 0; stretch < MAX_STRETCH; stretch++) {
    free(hPhaseVocoder->xOverBin[stretch]);
  }
  free(hPhaseVocoder->xOverBin);

  for (stretch = 2; stretch <= MAX_STRETCH; stretch++) {
    for (i=0; i<2; i++) {
      free(hPhaseVocoder->fdWin[stretch-2][i]);
    }
    free(hPhaseVocoder->fdWin[stretch-2]);
  }

  free(hPhaseVocoder->outBuf);
  free(hPhaseVocoder->fdWin);

  free(hPhaseVocoder);
}


/**************************************************************************/
/*!  \brief     Apply Phase Vocoder                                       */
/**************************************************************************/
void PhaseVocoderApply(HANDLE_HBE_TRANSPOSER hPhaseVocoder,
                       float                 qmfBufferCodecReal[][64],
                       float                 qmfBufferCodecImag[][64],
                       int                   nColsIn,
                       float                 qmfBufferPVReal[][64],
                       float                 qmfBufferPVImag[][64],
                       int                   bOverSampling,
                       int                   pitchInBins)
{
  int inOffset   = 0;
  int outOffset  = 0;
  int inHopSize  = hPhaseVocoder->inHopSize;
  int fftSize    = hPhaseVocoder->fftSize[bOverSampling];

  int outHopSize = hPhaseVocoder->outHopSize;
  int nSamplesIn = nColsIn*hPhaseVocoder->synthSize;
  int anaFFTSize = hPhaseVocoder->anaFFTSize[bOverSampling];
  int synFFTSize = hPhaseVocoder->synFFTSize[bOverSampling];

  int anaPadSize = (anaFFTSize-hPhaseVocoder->anaFFTSize[0])/2;
  int synPadSize = (synFFTSize-hPhaseVocoder->synFFTSize[0])/2;

  float *dataInput          = hPhaseVocoder->inBuf;
  float *dataOutput         = hPhaseVocoder->outBuf;
  float *spectrum           = hPhaseVocoder->spectrum;
  float *spectrumTransposed = hPhaseVocoder->spectrumTransposed;
  float *mag                = hPhaseVocoder->mag;
  float *phase              = hPhaseVocoder->phase;
  float ***fdWin            = hPhaseVocoder->fdWin;
  int i,k,T;

  int anaFFToffset = hPhaseVocoder->sStart * fftSize / 32;
  int synFFToffset = hPhaseVocoder->aStart * fftSize / 64;

  int tr, ti, mTr;
  float mVal;
  /* pitchInBins is given with the resolution of a 1536 point FFT */
  double mainIndex  = fftSize * pitchInBins / 1536.0;
  int p;
  int bSbr41 = hPhaseVocoder->bSbr41;

  if(bSbr41) {
    anaFFToffset *= 2;
    synFFToffset *= 2;
  } else if (hPhaseVocoder->fftSize[0] == 768) {
    anaFFToffset *= (4.f/3.f);
    synFFToffset *= (4.f/3.f);
    mainIndex    *= (4.0/3.0);
  }

  p = (int) mainIndex; 
  copyFLOAT(dataInput+hPhaseVocoder->anaFFTSize[0], dataInput, hPhaseVocoder->anaFFTSize[0]);

  for (i=0; i<nColsIn; i++) {
    float qmfBufferCodecTemp[32];
    for (k=0;k<hPhaseVocoder->synthSize;k++)
    {
      int ki=hPhaseVocoder->sStart+k;
      qmfBufferCodecTemp[k] = cos(PI/2*((ki+0.5)*191/64.0-hPhaseVocoder->sStart)) * qmfBufferCodecReal[i][ki] +
                              sin(PI/2*((ki+0.5)*191/64.0-hPhaseVocoder->sStart)) * qmfBufferCodecImag[i][ki];
    }

    RQMFsynthesis(qmfBufferCodecTemp,
                  dataInput+hPhaseVocoder->anaFFTSize[0]+(i-1)*hPhaseVocoder->synthSize,
                  hPhaseVocoder->synthSize,
                  hPhaseVocoder->synthBuffer,
                  &hPhaseVocoder->NN,
                  hPhaseVocoder->synthesisProtFilter);
  }
  
  copyFLOAT(dataOutput+2*hPhaseVocoder->synFFTSize[0], dataOutput, 2*hPhaseVocoder->synFFTSize[0]);
  memset(dataOutput+2*hPhaseVocoder->synFFTSize[0], 0, 2*hPhaseVocoder->synFFTSize[0]*sizeof(float));

  while(inOffset<nSamplesIn) {

    memset(spectrum,           0, fftSize*sizeof(float));
    if(hPhaseVocoder->fftSize[0] == 768) {
      memset(spectrumTransposed, 0, (4*fftSize/3+2)*sizeof(float));
    } else {
      memset(spectrumTransposed, 0, ((1+bSbr41)*fftSize+2)*sizeof(float));
    }

    memset(mag,   0, (fftSize/2+2)*sizeof(float));
    memset(phase, 0, (fftSize/2+2)*sizeof(float));

    multFLOAT(dataInput+inOffset, hPhaseVocoder->analysisWin, spectrum+anaPadSize+anaFFToffset, hPhaseVocoder->anaFFTSize[0]);
    fftshift(spectrum+anaFFToffset, anaFFTSize);
    RFFTN(spectrum+anaFFToffset, trigPtrFft[bOverSampling], anaFFTSize, -1);
    karth2polar(spectrum+anaFFToffset, mag+anaFFToffset/2, phase+anaFFToffset/2, anaFFTSize);

    for(T=2; T<=hPhaseVocoder->maxStretch; T++)
    {
      int utk;
      float ptk, magT, phaseT;
      int outTransformSize;

      /* 0<i<fftSize/2 */
      if(hPhaseVocoder->fftSize[0] == 768) {
        outTransformSize = 4*(fftSize/2)/3;
      } else {
        outTransformSize = (bSbr41+1)*(fftSize/2);
      }

      for(i=0; i<=outTransformSize; i++) {
        utk = 2*i/T;
        ptk = (2.0*i/T)-utk;
        magT   = fdWin[T-2][bOverSampling][i] * pow(mag[utk], 1.0f-ptk) * pow(mag[utk+1], ptk);
        phaseT = T*((1-ptk)*phase[utk] + ptk*phase[utk+1]);
        spectrumTransposed[2*i]   += magT * cos(phaseT);
        spectrumTransposed[2*i+1] += magT * sin(phaseT);

        if(p>0)
        {
          mVal = 0;
          for(tr=1; tr<T; tr++)
          {
            float temp;
            ti = (int) ( (2.0*i-tr*mainIndex)/T + 0.5 );
            if((ti<0)||(ti+p > fftSize/2)) continue;
            temp = min(mag[ti], mag[ti+p]);
            if(temp > mVal)
            {
              mVal = temp;
              mTr  = tr;
              utk  = ti;
            }
          } /* for tr */
        
          if(mVal > qThr*mag[2*i/T])
          {
            float r = (float) mTr/T;
            magT   = fdWin[T-2][bOverSampling][i] * pow(mag[utk], 1.0f-r) * pow(mag[utk+p], r);
            phaseT = (T-mTr)*phase[utk] + mTr*phase[utk+p];
            spectrumTransposed[2*i]   += magT * cos(phaseT);
            spectrumTransposed[2*i+1] += magT * sin(phaseT);
          }
        } /* p */
      } /* for i */

    } /* for T */

    spectrumTransposed[synFFToffset+1] = spectrumTransposed[synFFToffset+synFFTSize]; /* Move Nyquist bin to bin 1 for RFFTN */
    RFFTN(spectrumTransposed+synFFToffset, trigPtrSynFft[bOverSampling], synFFTSize, +1);
    fftshift(spectrumTransposed+synFFToffset, synFFTSize);
    multFLOAT(spectrumTransposed+synPadSize+synFFToffset, hPhaseVocoder->synthesisWin, spectrumTransposed+synPadSize+synFFToffset, hPhaseVocoder->synFFTSize[0]);
    for(i=0; i<hPhaseVocoder->synFFTSize[0]; i++){
      dataOutput[outOffset + i] += spectrumTransposed[synPadSize+synFFToffset+i];
    }

    inOffset  += inHopSize;
    outOffset += outHopSize;

  } /* while(inOffset<nSamplesIn) */

  for (i=0; i<nColsIn; i++) {
    CQMFanalysis(dataOutput+i*hPhaseVocoder->analySize+1,
                 &qmfBufferPVReal[i][hPhaseVocoder->aStart],
                 &qmfBufferPVImag[i][hPhaseVocoder->aStart],
                 hPhaseVocoder->analySize,
                 hPhaseVocoder->analaBuffer,
                 &hPhaseVocoder->MM,
                 hPhaseVocoder->analysisProtFilter);
  }
}
