/*******************************************************************************
This software module was originally developed by

Dolby Laboratories, Fraunhofer IIS, Panasonic.


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
$Id: harmsbrQmftrans.c,v 1.15 2011-08-18 14:21:03 mul Exp $
*******************************************************************************/

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "proto_func.h"
#include "harmsbrQmftrans.h"
#include "harmsbrWingen.h"
#include "harmsbrMathlib.h"
#include "ct_polyphase.h"
#include "ct_sbrconst.h" /* PI */


static const float qThrQMF = 1.0f;
static const float pmin = 1.0f;

static const int qmfWinLen = QMF_WIN_LEN;
static const int nQmfSynthChannels = NO_QMF_SYNTH_CHANNELS;

static const int startSubband2kL[33] =
{
  0, 0, 0, 0, 0, 0, 0,  2,  2,  2,  4,  4,  4,  4,  4,  6,
  6, 6, 8, 8, 8, 8, 8, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12
};

extern const double stdSbrDecoderFilterbankCoefficients[640];

#ifndef MAX_NUM_PATCHES
#include "ct_hfgen.h"
#endif


/**************************************************************************/
/*!  \brief     Parameters used for Phase Vocoder                         */
/**************************************************************************/

struct hbeTransposer {
  int xOverQmf[MAX_NUM_PATCHES];

  int maxStretch;
  int timeDomainWinLen;
  int qmfInBufSize;  
  int qmfOutBufSize;   
  int noCols;
  int startBand;
  int stopBand;
  int bSbr41;
 
  float *inBuf;  

  float **qmfInBufReal;
  float **qmfInBufImag;
  float **qmfOutBufReal;
  float **qmfOutBufImag;

  int kstart;
  int synthSize;
  double synthBuffer[1280];
  double analaBuffer[640];
  double *synthesisProtFilter;
  double *analysisProtFilter;

  struct M2 MM;
  struct N  NN;
};


/**************************************************************************/
/*!  \brief     Creation of Phase Vocoder Handle                          */
/**************************************************************************/
int
QmfTransposerCreate(HANDLE_HBE_TRANSPOSER *hQmfTransposer,                   
                    const int frameSize,          /* Analysis FFT length */
                    int bSbr41)
{

  HANDLE_HBE_TRANSPOSER hQmfTran;
  
  int i;

  if (hQmfTransposer) {
        
    /* Memory allocation */
    /*--------------------------------------------------------------------------------------------*/
    hQmfTran = (HANDLE_HBE_TRANSPOSER) calloc(1, sizeof(struct hbeTransposer) );
  
    hQmfTran->timeDomainWinLen = frameSize;
    if (frameSize == 768) {
      hQmfTran->noCols = (8*frameSize/3) / nQmfSynthChannels;
    } else {
      hQmfTran->noCols = (bSbr41+1)*2*frameSize / nQmfSynthChannels;
    }
    hQmfTran->qmfInBufSize = hQmfTran->noCols/2 + qmfWinLen - 1;  
    hQmfTran->qmfOutBufSize = 2*hQmfTran->qmfInBufSize;   

    hQmfTran->inBuf = (float *) calloc (frameSize+nQmfSynthChannels, sizeof(float)); /* frameSize + 2*32 */
    if (hQmfTran->inBuf == NULL) {
      QmfTransposerClose(hQmfTran);
      return -1;
    }
  
    hQmfTran->qmfInBufReal = (float**) calloc(hQmfTran->qmfInBufSize, sizeof(float*));
    hQmfTran->qmfInBufImag = (float**) calloc(hQmfTran->qmfInBufSize, sizeof(float*));

    if(hQmfTran->qmfInBufReal == NULL) {
        QmfTransposerClose(hQmfTran);
        return -1;
    }
    if (hQmfTran->qmfInBufImag == NULL) {
        QmfTransposerClose(hQmfTran);
        return -1;
    }
    
    for(i=0; i<hQmfTran->qmfInBufSize; i++) {
        hQmfTran->qmfInBufReal[i] = (float*) calloc(nQmfSynthChannels, sizeof(float));
        hQmfTran->qmfInBufImag[i] = (float*) calloc(nQmfSynthChannels, sizeof(float));
        if (hQmfTran->qmfInBufReal[i] == NULL) {
            QmfTransposerClose(hQmfTran);
            return -1;
        }
        if (hQmfTran->qmfInBufImag[i] == NULL ) {
            QmfTransposerClose(hQmfTran);
            return -1;
        }
    }

    hQmfTran->qmfOutBufReal = (float**) calloc(hQmfTran->qmfOutBufSize, sizeof(float*));
    hQmfTran->qmfOutBufImag = (float**) calloc(hQmfTran->qmfOutBufSize, sizeof(float*));
    if(hQmfTran->qmfOutBufReal == NULL) {
        QmfTransposerClose(hQmfTran);
        return -1;
    }
    if(hQmfTran->qmfOutBufImag == NULL) {
        QmfTransposerClose(hQmfTran);
        return -1;
    }

    for(i=0; i<hQmfTran->qmfOutBufSize; i++) {
        hQmfTran->qmfOutBufReal[i] = (float*) calloc(nQmfSynthChannels, sizeof(float));
        hQmfTran->qmfOutBufImag[i] = (float*) calloc(nQmfSynthChannels, sizeof(float));
        if(hQmfTran->qmfOutBufReal[i] == NULL) {
            QmfTransposerClose(hQmfTran);
            return -1;
        }              
        if(hQmfTran->qmfOutBufImag[i] == NULL) {
          QmfTransposerClose(hQmfTran);
          return -1;
        }
    }

    hQmfTran->bSbr41 = bSbr41;
  
    *hQmfTransposer = hQmfTran;
  }

  return 0;
}

/**************************************************************************/
/*!  \brief     Reinit Phase Vocoder if xover has been changed            */
/**************************************************************************/
void QmfTransposerReInit(HANDLE_HBE_TRANSPOSER hQmfTransposer,
                         int FreqBandTable[2][MAX_FREQ_COEFFS + 1],
                         int NSfb[2],
                         int bSbr41)
{
  int k, l, L, sfb, patch, stopPatch;

  if (hQmfTransposer != NULL) {

    hQmfTransposer->startBand = FreqBandTable[LO][0];
    hQmfTransposer->stopBand  = FreqBandTable[LO][NSfb[LO]];

    hQmfTransposer->synthSize = 4*((hQmfTransposer->startBand+4)/8+1); 
    hQmfTransposer->kstart = startSubband2kL[hQmfTransposer->startBand];

    hQmfTransposer->bSbr41 = bSbr41;

    if (bSbr41) {
      if(hQmfTransposer->kstart + hQmfTransposer->synthSize > 16)
        hQmfTransposer->kstart = 16-hQmfTransposer->synthSize;
    } else if (hQmfTransposer->timeDomainWinLen == 768) {
        if(hQmfTransposer->kstart + hQmfTransposer->synthSize > 24)
        hQmfTransposer->kstart = 24-hQmfTransposer->synthSize;
    }

   	memset(hQmfTransposer->synthBuffer, 0, 1280*sizeof(double));
  	L = hQmfTransposer->synthSize;
      
 	  for ( l=0; l<2*L; l++ ) {
    	for ( k=0; k<L; k++ ) {
      	hQmfTransposer->NN.real[l][k] = 1.0/L * cos(PI/(2*L)*(k+0.5)*(2*l-L));
			}
  	}
    if(hQmfTransposer->synthesisProtFilter) free(hQmfTransposer->synthesisProtFilter);
    hQmfTransposer->synthesisProtFilter = interpolPrototype( stdSbrDecoderFilterbankCoefficients,
                                                            640, 64, L, 0);

    memset(hQmfTransposer->analaBuffer, 0, 640*sizeof(double));
  	L = 2*hQmfTransposer->synthSize;
  	for ( k=0; k<L; k++ ) {
    	for ( l=0; l<2*L; l++ ) {
        hQmfTransposer->MM.real[k][l] = cos(PI/(2*L)*(k+0.5)*(2*l-2*L));
        hQmfTransposer->MM.imag[k][l] = sin(PI/(2*L)*(k+0.5)*(2*l-2*L));
    	}
  	}
    if(hQmfTransposer->analysisProtFilter) free(hQmfTransposer->analysisProtFilter);
    hQmfTransposer->analysisProtFilter = interpolPrototype( stdSbrDecoderFilterbankCoefficients,
                                                           640, 64, L, 0);

    memset(hQmfTransposer->xOverQmf, 0, MAX_NUM_PATCHES*sizeof(int)); /* global */
    sfb=0;
    if (bSbr41) {
      stopPatch = MAX_NUM_PATCHES;
      hQmfTransposer->maxStretch = MAX_STRETCH;
    } else {
      stopPatch = MAX_STRETCH;
    }

    for(patch=1; patch <= stopPatch; patch++) {
      while(sfb <= NSfb[LO] && FreqBandTable[LO][sfb] <= patch*hQmfTransposer->startBand) sfb++;
      if(sfb <= NSfb[LO])
      {
        /* If the distance is larger than three QMF bands - try aligning to high resolution frequency bands instead. */
        if( (patch*hQmfTransposer->startBand-FreqBandTable[LO][sfb-1]) <= 3) 
        {
          hQmfTransposer->xOverQmf[patch-1] = FreqBandTable[LO][sfb-1];
        }
        else
        {
          int sfb=0;
          while(sfb <= NSfb[HI] && FreqBandTable[HI][sfb] <= patch*hQmfTransposer->startBand) sfb++;
          hQmfTransposer->xOverQmf[patch-1] = FreqBandTable[HI][sfb-1];
        }
      }
      else
      {
        hQmfTransposer->xOverQmf[patch-1] = hQmfTransposer->stopBand;
        hQmfTransposer->maxStretch = min(patch, MAX_STRETCH);
        break;
      }
    }
	}
}

/**************************************************************************/
/*!  \brief     Close Phase Vocoder                                       */
/**************************************************************************/
void QmfTransposerClose(HANDLE_HBE_TRANSPOSER hQmfTransposer) 
{
  int i;
  int stretch = 0;

 if (hQmfTransposer->inBuf) {
      free(hQmfTransposer->inBuf);
  }

  if(hQmfTransposer->qmfInBufReal) {
     for (i = 0; i < hQmfTransposer->qmfInBufSize; i++) {
        free(hQmfTransposer->qmfInBufReal[i]);
     }              
     free(hQmfTransposer->qmfInBufReal);
  }
  
  if(hQmfTransposer->qmfInBufImag) {
     for (i = 0; i < hQmfTransposer->qmfInBufSize; i++) {
        free(hQmfTransposer->qmfInBufImag[i]);
     }
     free(hQmfTransposer->qmfInBufImag);
  }

  if(hQmfTransposer->qmfOutBufReal) {
    for (i = 0; i < hQmfTransposer->qmfOutBufSize; i++) {
       free(hQmfTransposer->qmfOutBufReal[i]);
    }
    free(hQmfTransposer->qmfOutBufReal);
  }

  if(hQmfTransposer->qmfOutBufImag) {
    for (i = 0; i < hQmfTransposer->qmfOutBufSize; i++) {
       free(hQmfTransposer->qmfOutBufImag[i]);
    }
    free(hQmfTransposer->qmfOutBufImag);
  }

  if(hQmfTransposer->synthesisProtFilter)
    free(hQmfTransposer->synthesisProtFilter);

  if(hQmfTransposer->analysisProtFilter)
    free(hQmfTransposer->analysisProtFilter);
  
  free(hQmfTransposer);
}

void QmfTransposerApply(HANDLE_HBE_TRANSPOSER hQmfTransposer, 
                        float                 qmfBufferCodecReal[][64],
                        float                 qmfBufferCodecImag[][64],
                        int                   nColsIn,
                        float                 qmfBufferPVReal[][64],
                        float                 qmfBufferPVImag[][64],
                        int                   bOverSampling,
                        int                   pitchInBins)
{
  int i, k, delta, stretch, band, sourceband, r;
  int qmfVocoderColsIn = hQmfTransposer->noCols/2;
  int bSbr41 = hQmfTransposer->bSbr41;
  
  memcpy(hQmfTransposer->inBuf, hQmfTransposer->inBuf+hQmfTransposer->noCols*hQmfTransposer->synthSize, hQmfTransposer->synthSize*sizeof(float));

  for (i=0; i<nColsIn; i++) {
    float qmfBufferCodecTemp[32];
    for (k=0;k<hQmfTransposer->synthSize;k++)
    {
      int ki=hQmfTransposer->kstart+k;
      qmfBufferCodecTemp[k] = cos(PI/2*((ki+0.5)*191/64.0-hQmfTransposer->kstart)) * qmfBufferCodecReal[i][ki] +
                              sin(PI/2*((ki+0.5)*191/64.0-hQmfTransposer->kstart)) * qmfBufferCodecImag[i][ki];
    }
    RQMFsynthesis(qmfBufferCodecTemp,
                  hQmfTransposer->inBuf+(i+1)*hQmfTransposer->synthSize,
                  hQmfTransposer->synthSize,
                  hQmfTransposer->synthBuffer,
                  &hQmfTransposer->NN,
                  hQmfTransposer->synthesisProtFilter);
  }
  for(;i<hQmfTransposer->noCols;i++) {
    const float zeros[64] = { 0 };
    RQMFsynthesis(zeros,
                  hQmfTransposer->inBuf+(i+1)*hQmfTransposer->synthSize,
                  hQmfTransposer->synthSize,
                  hQmfTransposer->synthBuffer,
                  &hQmfTransposer->NN,
                  hQmfTransposer->synthesisProtFilter);
    }
   
  /* Update QMF input buffer */
  for(i=0; i<qmfWinLen-1;i++) {
    memcpy(hQmfTransposer->qmfInBufReal[i], hQmfTransposer->qmfInBufReal[i+qmfVocoderColsIn], nQmfSynthChannels*sizeof(float));
    memcpy(hQmfTransposer->qmfInBufImag[i], hQmfTransposer->qmfInBufImag[i+qmfVocoderColsIn], nQmfSynthChannels*sizeof(float));
  }

  for (i=0; i<qmfVocoderColsIn; i++) {

    memset(hQmfTransposer->qmfInBufReal[i+qmfWinLen-1], 0, nQmfSynthChannels*sizeof(float));
    memset(hQmfTransposer->qmfInBufImag[i+qmfWinLen-1], 0, nQmfSynthChannels*sizeof(float));

    CQMFanalysis( hQmfTransposer->inBuf+i*2*hQmfTransposer->synthSize+1,
                  &hQmfTransposer->qmfInBufReal[i+qmfWinLen-1][2*hQmfTransposer->kstart],
                  &hQmfTransposer->qmfInBufImag[i+qmfWinLen-1][2*hQmfTransposer->kstart],
                  2*hQmfTransposer->synthSize,
                  hQmfTransposer->analaBuffer,
                  &hQmfTransposer->MM,
                  hQmfTransposer->analysisProtFilter );
  }

  /* Update QMF output buffer */
  for(i=0; i<(hQmfTransposer->qmfOutBufSize-hQmfTransposer->noCols); i++) {
    memcpy(hQmfTransposer->qmfOutBufReal[i], hQmfTransposer->qmfOutBufReal[i+hQmfTransposer->noCols], nQmfSynthChannels*sizeof(float));
    memcpy(hQmfTransposer->qmfOutBufImag[i], hQmfTransposer->qmfOutBufImag[i+hQmfTransposer->noCols], nQmfSynthChannels*sizeof(float));
  }

  for(; i<hQmfTransposer->qmfOutBufSize; i++) {
    memset(hQmfTransposer->qmfOutBufReal[i], 0, nQmfSynthChannels*sizeof(float));        
    memset(hQmfTransposer->qmfOutBufImag[i], 0, nQmfSynthChannels*sizeof(float));
  }

  /* Time-stretch */
  delta = 1; 
  for(stretch=2; stretch <= hQmfTransposer->maxStretch; stretch++) {

    for(band = hQmfTransposer->xOverQmf[stretch-2]; band < hQmfTransposer->xOverQmf[stretch-1]; band++) {

      float twid, tmpReal, tmpImag, wingain;
      float hintReal[3][2], hintImag[3][2];
      const int slotOffset = 6; /* hQmfTransposer->winLen-6; */
      const int winLength[3] = { 10, 8, 6 };
      const int offset = 0;       /* slotOffset-winLength[0]/2; */
      float synthesisWinQmfReal[QMF_WIN_LEN];

      /* interpolation filters for 3rd order */
      sourceband = 2*band/stretch;  

      for(k = 0; k < 3; k++){
        for(i = 0; i < 2; i++){
          hintReal[k][i] = 0.56342741195967 * cos((acos(-1.0)/2.0)*(2.0*(float)i-1.0)*((float)sourceband+0.5+(float)k-1.0));
          hintImag[k][i] = 0.56342741195967 * sin((acos(-1.0)/2.0)*(2.0*(float)i-1.0)*((float)sourceband+0.5+(float)k-1.0));
        }
      }

      for(i=0; i<qmfWinLen; i++) {
        synthesisWinQmfReal[i] = 0.0f;
      }
      
      /* Rectangular windows */
      if(stretch == 2) {  
        for(i=0; i<winLength[0]/2; i++) {
          synthesisWinQmfReal[slotOffset+i]   = 1.0;
          synthesisWinQmfReal[slotOffset-1-i] = 1.0;
        }
        wingain = 5.0; /* sum of taps divided by two */
      } else if (stretch == 3) {
        for(i=0; i<winLength[1]/2; i++) {
          synthesisWinQmfReal[slotOffset+i]   = 1.4142f;
          synthesisWinQmfReal[slotOffset-1-i] = 1.4142f;
        }
        wingain = 5.6568f; /* sum of taps divided by two */
      } else if(stretch == 4) { /* radius 6, 11 taps half radius 3 */
        for(i=0; i<winLength[2]/2; i++) {
          synthesisWinQmfReal[slotOffset+i]   = 2.0;
          synthesisWinQmfReal[slotOffset-1-i] = 2.0;
        }
        wingain = 6.0; /* sum of taps divided by two */
      }

      for(i = 0; i < qmfVocoderColsIn; i+=delta ) {
        
        int k, filtflag, addrshift;
        float phi=0;
        float gammaVec0Real[32], gammaVec0Imag[32], gammaVec1Real[32], gammaVec1Imag[32];
        float gammaCenterReal, gammaCenterImag;
        float scalegain, p;
        float grainModReal[QMF_WIN_LEN], grainModImag[QMF_WIN_LEN];

        memset(gammaVec0Real, 0.f, 32*sizeof(float));
        memset(gammaVec0Imag, 0.f, 32*sizeof(float));
        memset(gammaVec1Real, 0.f, 32*sizeof(float));
        memset(gammaVec1Imag, 0.f, 32*sizeof(float));

        for(k=offset; k< qmfWinLen; k++) {
          if (stretch==4) {
            sourceband = band/2;  
            r = band - 2*sourceband;
            if (r == 0) {       
              if (2*(k-slotOffset) + slotOffset < qmfWinLen && 2*(k-slotOffset) + slotOffset +1 > 0) {
                gammaVec0Real[k] = hQmfTransposer->qmfInBufReal[i + 2*(k-slotOffset) + slotOffset][sourceband-1];
                gammaVec0Imag[k] = hQmfTransposer->qmfInBufImag[i + 2*(k-slotOffset) + slotOffset][sourceband-1]; 
                gammaVec1Real[k] = hQmfTransposer->qmfInBufReal[i + 2*(k-slotOffset) + slotOffset][sourceband];
                gammaVec1Imag[k] = hQmfTransposer->qmfInBufImag[i + 2*(k-slotOffset) + slotOffset][sourceband]; 
              }  
            } else { 
              if (2*(k-slotOffset) + slotOffset < qmfWinLen && 2*(k-slotOffset) + slotOffset +1 > 0) {
                gammaVec0Real[k] = hQmfTransposer->qmfInBufReal[i + 2*(k-slotOffset) + slotOffset][sourceband+1];
                gammaVec0Imag[k] = hQmfTransposer->qmfInBufImag[i + 2*(k-slotOffset) + slotOffset][sourceband+1];
                gammaVec1Real[k] = hQmfTransposer->qmfInBufReal[i + 2*(k-slotOffset) + slotOffset][sourceband];
                gammaVec1Imag[k] = hQmfTransposer->qmfInBufImag[i + 2*(k-slotOffset) + slotOffset][sourceband];
              }  
            }
          } else if (stretch==3){
            sourceband = 2*band/3;
            r = 2*band - 3*sourceband;  
            addrshift = (3*(k-slotOffset)+1000)/2 - 500;  
            filtflag  = 3*(k-slotOffset) - 2*addrshift;
                  
            if (r == 0 || r == 1) {
              if (addrshift + slotOffset < qmfWinLen && addrshift + slotOffset +1 > 0 && filtflag == 0){
                gammaVec0Real[k] = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband];
                gammaVec0Imag[k] = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband]; 
              } else if (addrshift +1 + slotOffset < qmfWinLen && addrshift + slotOffset +1 > 0 && filtflag == 1){ /* filter with two taps hintReal[3][2], hintImag[3][2]; */
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband];
                gammaVec0Real[k] = hintReal[1][1]*tmpReal-hintImag[1][1]*tmpImag;
                gammaVec0Imag[k] = hintImag[1][1]*tmpReal+hintReal[1][1]*tmpImag;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + 1 + slotOffset][sourceband];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + 1 + slotOffset][sourceband];
                gammaVec0Real[k] += hintReal[1][0]*tmpReal-hintImag[1][0]*tmpImag;
                gammaVec0Imag[k] += hintImag[1][0]*tmpReal+hintReal[1][0]*tmpImag; 
              }
              gammaVec1Real[k] = gammaVec0Real[k] ;
              gammaVec1Imag[k] = gammaVec0Imag[k] ; 
            } else { /* r = 2 */
              if (addrshift + slotOffset < qmfWinLen && addrshift + slotOffset +1 > 0 && filtflag == 0){
                gammaVec0Real[k] = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband+1];
                gammaVec0Imag[k] = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband+1]; 
                gammaVec1Real[k] = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband];
                gammaVec1Imag[k] = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband]; 
              } else if (addrshift +1 + slotOffset < qmfWinLen && addrshift + slotOffset +1 > 0 && filtflag == 1){ /* filter with two taps hintReal[3][2], hintImag[3][2]; */
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband];
                gammaVec1Real[k] = hintReal[1][1]*tmpReal-hintImag[1][1]*tmpImag;
                gammaVec1Imag[k] = hintImag[1][1]*tmpReal+hintReal[1][1]*tmpImag;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + 1 + slotOffset][sourceband];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + 1 + slotOffset][sourceband];
                gammaVec1Real[k] += hintReal[1][0]*tmpReal-hintImag[1][0]*tmpImag;
                gammaVec1Imag[k] += hintImag[1][0]*tmpReal+hintReal[1][0]*tmpImag; 
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][sourceband+1];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][sourceband+1];
                gammaVec0Real[k] = hintReal[2][1]*tmpReal-hintImag[2][1]*tmpImag;
                gammaVec0Imag[k] = hintImag[2][1]*tmpReal+hintReal[2][1]*tmpImag;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + 1 + slotOffset][sourceband+1];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + 1 + slotOffset][sourceband+1];
                gammaVec0Real[k] += hintReal[2][0]*tmpReal-hintImag[2][0]*tmpImag;
                gammaVec0Imag[k] += hintImag[2][0]*tmpReal+hintReal[2][0]*tmpImag; 
              }
            }
          } else {  /* placeholder for usual stretch 2 */
            gammaVec0Real[k] = hQmfTransposer->qmfInBufReal[i + k][band];
            gammaVec0Imag[k] = hQmfTransposer->qmfInBufImag[i + k][band];   
            gammaVec1Real[k] = hQmfTransposer->qmfInBufReal[i + k][band];
            gammaVec1Imag[k] = hQmfTransposer->qmfInBufImag[i + k][band];   
          }
           
          /* prenormalization version */
          scalegain = pow(gammaVec0Real[k]*gammaVec0Real[k]+gammaVec0Imag[k]*gammaVec0Imag[k]+pow(2.0,-52),
                          0.5*((1.0/(float)stretch)-1.0)); /* ( pow( absValue, (1-((float)1/(float)stretch)) ) + pow(2,-52) ) */
          gammaVec0Real[k] *= scalegain;
          gammaVec0Imag[k] *= scalegain;
          scalegain = pow(gammaVec1Real[k]*gammaVec1Real[k]+gammaVec1Imag[k]*gammaVec1Imag[k]+pow(2.0,-52),
                          0.5*((1.0/(float)stretch)-1.0)); /* ( pow( absValue, (1-((float)1/(float)stretch)) ) + pow(2,-52) ) */
          gammaVec1Real[k] *= scalegain;
          gammaVec1Imag[k] *= scalegain;

        } /* winLen */

        gammaCenterReal = gammaVec1Real[slotOffset];
        gammaCenterImag = gammaVec1Imag[slotOffset];
        tmpReal = gammaCenterReal;
        tmpImag = gammaCenterImag;
        /* gamma(F+1)^(T-1) */
        for(k=0; k< stretch-2; k++) {
          float tmp = gammaCenterReal;
          gammaCenterReal = gammaCenterReal*tmpReal - gammaCenterImag*tmpImag;
          gammaCenterImag = tmp*tmpImag + gammaCenterImag*tmpReal;
        }

        for(k=offset; k< qmfWinLen; k++) {
          grainModReal[k] = gammaVec0Real[k]*gammaCenterReal - gammaVec0Imag[k]*gammaCenterImag;
          grainModImag[k] = gammaVec0Real[k]*gammaCenterImag + gammaVec0Imag[k]*gammaCenterReal;
        }
        if (stretch==3 && r == 2){  /* add opposite term */

          gammaCenterReal = gammaVec0Real[slotOffset];
          gammaCenterImag = gammaVec0Imag[slotOffset];
          tmpReal = gammaCenterReal;
          tmpImag = gammaCenterImag;
          /* gamma(F+1)^(T-1) */
          for(k=0; k< stretch-2; k++) {
            float tmp = gammaCenterReal;
            gammaCenterReal = gammaCenterReal*tmpReal - gammaCenterImag*tmpImag;
            gammaCenterImag = tmp*tmpImag + gammaCenterImag*tmpReal;
          }

          for(k=offset; k<qmfWinLen; k++) {
            grainModReal[k] += gammaVec1Real[k]*gammaCenterReal - gammaVec1Imag[k]*gammaCenterImag;
            grainModImag[k] += gammaVec1Real[k]*gammaCenterImag + gammaVec1Imag[k]*gammaCenterReal;

            grainModReal[k] *= 0.5;
            grainModImag[k] *= 0.5;
          }
        }
      
        for(k=offset; k<qmfWinLen; k++) {
          /* synthesis windowing */
          grainModReal[k] *= synthesisWinQmfReal[k];                  
          grainModImag[k] *= synthesisWinQmfReal[k];
          /*OLA*/
          hQmfTransposer->qmfOutBufReal[i*2+(k-offset)][band] += grainModReal[k]/3;
          hQmfTransposer->qmfOutBufImag[i*2+(k-offset)][band] += grainModImag[k]/3;
        }

        /* pitchInBins is given with the resolution of a 768 bins FFT and we need 64 QMF units so factor 768/64 = 12 */
        p  = pitchInBins / (12.0f * (1+bSbr41));

        if(p >= pmin)
        {
          int tr, ti1, ti2, mTr, ts1, ts2;
          double f1; /* Should be a double to avoid MSVS6 compiler differences when changing debug info settings */ 
          float mVal, sqmag0, sqmag1, sqmag2, temp;
          sourceband = 2*band/stretch; /* consistent with the already computed for stretch = 3,4. */
          sqmag0 = hQmfTransposer->qmfInBufReal[i + slotOffset][sourceband]*hQmfTransposer->qmfInBufReal[i + slotOffset][sourceband] +
                   hQmfTransposer->qmfInBufImag[i + slotOffset][sourceband]*hQmfTransposer->qmfInBufImag[i + slotOffset][sourceband];
          mVal = 0;
          ts1  = ts2 = mTr = 0;
          
          for(tr=1; tr<stretch; tr++)
          {
            f1 = (2.0*band+1-tr*p)/stretch;
            ti1 = (int)f1;
            ti2 = (int)(f1+p);

            sqmag1 = hQmfTransposer->qmfInBufReal[i + slotOffset][ti1]*hQmfTransposer->qmfInBufReal[i + slotOffset][ti1] +
                     hQmfTransposer->qmfInBufImag[i + slotOffset][ti1]*hQmfTransposer->qmfInBufImag[i + slotOffset][ti1];
            sqmag2 = hQmfTransposer->qmfInBufReal[i + slotOffset][ti2]*hQmfTransposer->qmfInBufReal[i + slotOffset][ti2] +
                     hQmfTransposer->qmfInBufImag[i + slotOffset][ti2]*hQmfTransposer->qmfInBufImag[i + slotOffset][ti2];
            temp = min(sqmag1,sqmag2);
            
            if(temp > mVal)
            {
              mVal = temp;
              mTr  = tr;
              ts1  = ti1;
              ts2  = ti2;
            }
          }
        
          if(mVal > qThrQMF * qThrQMF * sqmag0 && ts1 >= 0 && ts2 < nQmfSynthChannels)
          {
            float gammaVecReal[2], gammaVecImag[2], gammaOutReal[2], gammaOutImag[2];
            float hReal[2], hImag[2];
            float D1, D2;          /* scaling  parameters */
            int   Tcenter, Tvec, idx;
          
            gammaCenterReal = 0;
            gammaCenterImag = 0;
            Tcenter = stretch - mTr;   /* default phase power parameters */
            Tvec    = mTr;
            if (stretch==2) /* 2 tap block creation design depends on stretch order */
            {
              D1 = 0; /* center from ts1 */
              D2 = 1; /* vector direct from ts2 */
              gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts1];
              gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts1];
              for (k=0; k<2; k++)
              {
                gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset-1+k][ts2];
                gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset-1+k][ts2];
              }
            }
            else if(stretch==4)
            {
              if (mTr == 1)
              {
                D1 = 0; /* center from ts1 */
                D2 = 2; /* vector downsampled from ts2 */
                gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts1];
                gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts1];
                for (k=0; k<2; k++)
                {
                  gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset+2*(k-1)][ts2];
                  gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset+2*(k-1)][ts2];
                }
              }
              else if (mTr == 2)
              {
                D1 = 0; /* center from ts1 */
                D2 = 1; /* vector from ts2 */
                gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts1];
                gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts1];
                for (k=0; k<2; k++)
                {
                  gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset+(k-1)][ts2];
                  gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset+(k-1)][ts2];
                }
              }
              else /* (mTr == 3) */
              {
                D1 = 2; /* vector downsampled from ts1 */
                D2 = 0; /* center from ts2 */
                Tcenter = mTr;   /* opposite phase power parameters as ts2 is center */
                Tvec    = stretch - mTr;
                gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts2];
                gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts2];
                for (k=0; k<2; k++)
                {
                  gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset+2*(k-1)][ts1];
                  gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset+2*(k-1)][ts1];
                }
              }
            }
            else /* (stretch==3) */
            {
              if (mTr == 1)
              {
                D1 = 0; /* center from ts1 */
                D2 = 1.5; /* vector downsampled 1.5 from ts2 */
                gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts1];
                gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts1];
                  /* interpolation filter for ts2 */
                for(k = 0; k < 2; k++)
                {
                  hReal[k] = 0.56342741195967 * cos((acos(-1.0)/2.0)*(2.0*(float)k-1.0)*((float)ts2+0.5));
                  hImag[k] = 0.56342741195967 * sin((acos(-1.0)/2.0)*(2.0*(float)k-1.0)*((float)ts2+0.5));
                }

                /*for (k=0; k<2; k++)
                {
                  gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset+1.5*(k-1)][ts2];
                  gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset+1.5*(k-1)][ts2];
                }*/
                gammaVecReal[1] = hQmfTransposer->qmfInBufReal[i + slotOffset][ts2];
                gammaVecImag[1] = hQmfTransposer->qmfInBufImag[i + slotOffset][ts2];

                addrshift = -2;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][ts2];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][ts2];
                gammaVecReal[0] = hReal[1]*tmpReal-hImag[1]*tmpImag;
                gammaVecImag[0] = hImag[1]*tmpReal+hReal[1]*tmpImag;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + 1 + slotOffset][ts2];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + 1 + slotOffset][ts2];
                gammaVecReal[0] += hReal[0]*tmpReal-hImag[0]*tmpImag;
                gammaVecImag[0] += hImag[0]*tmpReal+hReal[0]*tmpImag;
              }
              else /* (mTr == 2) */
              {
                D1 = 1.5; /* vector downsampled 1.5 from ts1 */
                D2 = 0; /* center from ts2 */
                Tcenter = mTr;   /* opposite phase power parameters as ts2 is center */
                Tvec    = stretch - mTr;

                gammaCenterReal = hQmfTransposer->qmfInBufReal[i + slotOffset][ts2];
                gammaCenterImag = hQmfTransposer->qmfInBufImag[i + slotOffset][ts2];
                  /* interpolation filter for ts1 */
                for(k = 0; k < 2; k++)
                {
                  hReal[k] = 0.56342741195967 * cos((acos(-1.0)/2.0)*(2.0*(float)k-1.0)*((float)ts1+0.5));
                  hImag[k] = 0.56342741195967 * sin((acos(-1.0)/2.0)*(2.0*(float)k-1.0)*((float)ts1+0.5));
                }

                /*for (k=0; k<2; k++)
                {
                  gammaVecReal[k] = hQmfTransposer->qmfInBufReal[i + slotOffset+1.5*(k-1)][ts1];
                  gammaVecImag[k] = hQmfTransposer->qmfInBufImag[i + slotOffset+1.5*(k-1)][ts1];
                }*/
                gammaVecReal[1] = hQmfTransposer->qmfInBufReal[i + slotOffset][ts1];
                gammaVecImag[1] = hQmfTransposer->qmfInBufImag[i + slotOffset][ts1];

                addrshift = -2;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + slotOffset][ts1];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + slotOffset][ts1];
                gammaVecReal[0] = hReal[1]*tmpReal-hImag[1]*tmpImag;
                gammaVecImag[0] = hImag[1]*tmpReal+hReal[1]*tmpImag;
                tmpReal = hQmfTransposer->qmfInBufReal[i + addrshift + 1 + slotOffset][ts1];
                tmpImag = hQmfTransposer->qmfInBufImag[i + addrshift + 1 + slotOffset][ts1];
                gammaVecReal[0] += hReal[0]*tmpReal-hImag[0]*tmpImag;
                gammaVecImag[0] += hImag[0]*tmpReal+hReal[0]*tmpImag;
              }
            }  /* stretch cases */

            /* parameter controlled phase modification parts */

            /* prenormalization of center */
            scalegain = pow(gammaCenterReal*gammaCenterReal+gammaCenterImag*gammaCenterImag+pow(2.0,-52),
                            0.5*((1.0/(float)stretch)-1.0)); /* ( pow( absValue, (1-((float)1/(float)stretch)) ) + pow(2,-52) ) */
            gammaCenterReal *= scalegain;
            gammaCenterImag *= scalegain;
            for (k=0; k<2; k++)
            { /* prenormalization of vector */
              scalegain = pow(gammaVecReal[k]*gammaVecReal[k]+gammaVecImag[k]*gammaVecImag[k]+pow(2.0,-52),
                              0.5*((1.0/(float)stretch)-1.0)); /* ( pow( absValue, (1-((float)1/(float)stretch)) ) + pow(2,-52) ) */
              gammaVecReal[k] *= scalegain;
              gammaVecImag[k] *= scalegain;
            }

            /*  Phase multiply center by Tcenter  */
            tmpReal = gammaCenterReal;
            tmpImag = gammaCenterImag;
            for(idx=0; idx< Tcenter-1; idx++)
            {
              float tmp = gammaCenterReal;
              gammaCenterReal = gammaCenterReal*tmpReal - gammaCenterImag*tmpImag;
              gammaCenterImag = tmp*tmpImag + gammaCenterImag*tmpReal;
            }

            /*  Phase multiply vector by Tvec */
            for (k=0; k<2; k++)
            {
              tmpReal = gammaVecReal[k];
              tmpImag = gammaVecImag[k];
              for(idx=0; idx< Tvec-1; idx++)
              {
                float tmp = gammaVecReal[k];
                gammaVecReal[k] = gammaVecReal[k]*tmpReal - gammaVecImag[k]*tmpImag;
                gammaVecImag[k] = tmp*tmpImag + gammaVecImag[k]*tmpReal;
              }
            }

            /*    Final multiplication of prepared parts  */
            for(k=0; k<2; k++)
            {
              gammaOutReal[k]= gammaVecReal[k]*gammaCenterReal - gammaVecImag[k]*gammaCenterImag;
              gammaOutImag[k]= gammaVecReal[k]*gammaCenterImag + gammaVecImag[k]*gammaCenterReal;
            }

            /* synthesis windowing off center entry with phasor       */
            /* wsyn = [exp(i*pi*(D2-D1)*p*rho*(T-rho)/T),1] * sum(w)/2;*/
            twid = acos(-1.0)*(D2-D1)*p*(float)mTr*(stretch-mTr)/stretch;
            tmpReal = gammaOutReal[0];
            tmpImag = gammaOutImag[0];
            gammaOutReal[0] = cos(twid) * tmpReal - sin(twid) * tmpImag;
            gammaOutImag[0] = cos(twid) * tmpImag + sin(twid) * tmpReal;

            /* OLA including window scaling by wingain/3 */
            for(k=0; k<2; k++)   /* need k=1 to correspond to grainModImag[slotOffset] -> out to i*2+(slotOffset-offset)  */
            {
              hQmfTransposer->qmfOutBufReal[i*2+(k+slotOffset-1-offset)][band] += (wingain/3.0)*gammaOutReal[k];
              hQmfTransposer->qmfOutBufImag[i*2+(k+slotOffset-1-offset)][band] += (wingain/3.0)*gammaOutImag[k];
            }
          } /* mVal > qThrQMF * qThrQMF * sqmag0 && ts1 > 0 && ts2 < 64 */
        } /* p >= pmin */
      } /* qmfVocoderColsIn */
    } /* for band */
  }/* for stretch */
    
  for(i = 0; i < hQmfTransposer->noCols; i++ ) {
    for(band = hQmfTransposer->startBand; band < hQmfTransposer->stopBand; band++) {

      float tout = -acos(-1.0)*(385.0*((float)band+0.5))/128.0;

      qmfBufferPVReal[i][band] = hQmfTransposer->qmfOutBufReal[i][band]*cos(tout) -
                                 hQmfTransposer->qmfOutBufImag[i][band]*sin(tout);
      qmfBufferPVImag[i][band] = hQmfTransposer->qmfOutBufReal[i][band]*sin(tout) +
                                 hQmfTransposer->qmfOutBufImag[i][band]*cos(tout);
    }
  }

}
