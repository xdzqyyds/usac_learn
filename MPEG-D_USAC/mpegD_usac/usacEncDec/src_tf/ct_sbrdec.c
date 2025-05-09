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
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrdec.c,v 1.37.2.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr decoder $Revision: 1.37.2.1 $
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <assert.h>

#ifdef SONY_PVC
#include "sony_pvcprepro.h"
#endif  /* SONY_PVC */

#include "ct_sbrdec.h"
#include "ct_envextr.h"
#include "ct_envcalc.h"
#include "ct_sbrdecoder.h"
#include "ct_freqsca.h"
#include "ct_hfgen.h"
#include "ct_polyphase.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif
#include "harmsbrPhasevoc.h"
#include "harmsbrQmftrans.h"
#include "common_m4a.h"

#ifndef HYBRID_FILTER_DELAY
#define HYBRID_FILTER_DELAY  6
#endif

#ifndef FALSE
#define FALSE              0
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


#define CODEC_XDELAY 32


/* Definition for the time being */
struct hbeTransposer {
  int    xOverQmf[MAX_NUM_PATCHES]; /* For limiterband calculation */
  int    maxStretch;
};

static void copyHarmonicSpectrum(int xOverQmf[MAX_NUM_PATCHES],float qmfReal[][64],float qmfImag[][64], int noCols, int maxStretch) {

     int patchBands;
   int patch,band,col,target, sourceBands, i;
     int numPatches =0;

     for(i=1; i<MAX_NUM_PATCHES; i++) {
         if(xOverQmf[i] != 0) {
             numPatches++;
         }
     }

     for (patch = (maxStretch-1); patch < numPatches; patch++) {
         patchBands = xOverQmf[patch+1]-xOverQmf[patch];
         target = xOverQmf[patch];
         sourceBands = xOverQmf[maxStretch-1] - xOverQmf[maxStretch-2];

         while (patchBands > 0) {
             int numBands = sourceBands;
             int startBand = xOverQmf[maxStretch-1]-1;
             if(target+numBands >= xOverQmf[patch+1]) {
                 numBands = xOverQmf[patch+1]-target;
             }
             if( ( ( (target+numBands-1)%2) + ((xOverQmf[maxStretch-1]-1)%2) )%2 ) {
                 if(numBands == sourceBands) {
                     numBands--;
                 } else {
                     startBand--;
                 }
             }
         for (col=0;col<noCols;col++) {
                 int i=0;
           for (band=numBands;band>0;band--) {
             if((target+band-1<64) && (target+band-1<xOverQmf[patch+1])){
               qmfReal[col][target+band-1] = qmfReal[col][startBand-i];
               qmfImag[col][target+band-1] = qmfImag[col][startBand-i];
                         i++;
             }
           }
         }
             target += numBands;
             patchBands -= numBands;
         }
   }
}

static int FreqBandTable[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][2][MAX_FREQ_COEFFS + 1];
static int NSfb[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][2];
static int FreqBandTableNoise[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][MAX_NOISE_COEFFS + 1];
static int NoNoiseBands[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];                                 /* Number of noisebands */
static int V_k_master[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][MAX_FREQ_COEFFS + 1];              /* Master BandTable which freqBandTable is derived from*/
static int Num_Master[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];                                   /* Number of bands in V_k_master*/

static int Reset[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];

static float codecQmfBufferPVReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];
static float codecQmfBufferPVImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];

static float codecQmfBufferReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80+2*CODEC_XDELAY][64];
static float codecQmfBufferImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80+2*CODEC_XDELAY][64];

static float codecQmfBufferPVMpsReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];
static float codecQmfBufferPVMpsImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];

static float codecQmfBufferMpsReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];
static float codecQmfBufferMpsImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];

static struct SBR_DEC
{
  int coreCodecFrameSize;
  int outSampleRate;

  int numBandsAnaQmf;
  int numBandsSynQmf;
  int sbrRatioIndex;

  int startIndexCodecQmf;
  int lowBandAddSamples;
  int noCols;
  int qmfBufLen;
  int bufWriteOffs;
  int hybQmfOffset;
  int sbrOffset;
  int bufReadOffs;

  int sbStopCodec[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
  int lowSubband[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
  int prevLowSubband[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
  int highSubband[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
  int noSubbands[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];

#ifdef SBR_SCALABLE
  int prevMaxQmfSubbandAac[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
#endif

  float ****qmfBufferReal;
  float ****qmfBufferImag;

#ifndef STEREO_HBE_NOFIX
  HANDLE_HBE_TRANSPOSER hHbeTransposer[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
#else
  HANDLE_HBE_TRANSPOSER hHbeTransposer[MAX_NUM_ELEMENTS];
#endif

  int bUseHBE[MAX_NUM_ELEMENTS];
  int bUseHQ;
  int bSbrFromMps;
  HANDLE_SBR_FRAME_DATA hFrameData[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
} sbrDec;

static void
resetSbrFreqBandTables(SBR_HEADER_DATA *headerData,
                       int samplingFreq,
                       int channel, int el,
                       float upsampleFac);

static void (*ptrHbeApplyFunc)(HANDLE_HBE_TRANSPOSER,
                               float [][64], float [][64], int,
                               float [][64], float [][64], int, int);

static void copyFLOAT(const float * X, float * Z, int n)
{
  memmove(Z,X,sizeof(float)*n);
}

void
sbr_dec_from_mps(int channel, int el,
                 float qmfOutputReal[][MAX_NUM_QMF_BANDS],
                 float qmfOutputImag[][MAX_NUM_QMF_BANDS]) {
  int i, k;
  HANDLE_SBR_FRAME_DATA hFrameData = sbrDec.hFrameData[el][channel];
  int *frameInfo = hFrameData->frameInfo;
  int noCols;

  static float sbrQmfBufferMpsReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];
  static float sbrQmfBufferMpsImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];

  if (!sbrDec.bSbrFromMps) {
    return;
  }

  noCols = sbrDec.noCols;


#ifndef RESET_HF_NOFIX
  for ( i = 0; i < noCols; i++ ) {
    for ( k = 0; k < sbrDec.lowSubband[el][channel]; k++ ) {
      codecQmfBufferMpsReal[el][channel][sbrDec.sbrOffset+i][k] = qmfOutputReal[i][k];
      codecQmfBufferMpsImag[el][channel][sbrDec.sbrOffset+i][k] = qmfOutputImag[i][k];

      sbrQmfBufferMpsReal[el][channel][sbrDec.bufReadOffs+i][k] = codecQmfBufferMpsReal[el][channel][sbrDec.bufReadOffs+i][k];
      sbrQmfBufferMpsImag[el][channel][sbrDec.bufReadOffs+i][k] = codecQmfBufferMpsReal[el][channel][sbrDec.bufReadOffs+i][k];

    }
  } 
#else
  for ( i = 0; i < noCols; i++ ) {
    for ( k = 0; k < sbrDec.lowSubband[channel]; k++ ) {
      codecQmfBufferMpsReal[el][channel][sbrDec.sbrOffset+i][k] = qmfOutputReal[i][k];
      codecQmfBufferMpsImag[el][channel][sbrDec.sbrOffset+i][k] = qmfOutputImag[i][k];
    }
  }
#endif

  if (Reset[el][channel]) {
    int l;
    int startBand = sbrDec.prevLowSubband[el][channel];
    int stopBand  = sbrDec.numBandsAnaQmf;
    int startSlot = sbrDec.bufReadOffs + hFrameData->rate * frameInfo[1];
    
    for (l=startSlot; l< sbrDec.sbrOffset; l++) {
      for (k=startBand; k<stopBand; k++) {
        codecQmfBufferMpsReal[el][channel][l][k] = 0.0;
        codecQmfBufferMpsImag[el][channel][l][k] = 0.0;
      }
    }

    /*
      Clear LPC-coefficients (last 2 timeslots before FIXFIX-border)
    */
        
    for (l=0; l < sbrDec.bufReadOffs; l++) {
      for ( k=startBand; k<stopBand; k++) {
        codecQmfBufferMpsReal[el][channel][l][k] = 0.0;
        codecQmfBufferMpsImag[el][channel][l][k] = 0.0;
      }
    }

  }
  sbrDec.prevLowSubband[el][channel] = sbrDec.lowSubband[el][channel];


  /*
    Inverse filtering of lowband + HF generation
  */
  generateHF ( codecQmfBufferMpsReal[el][channel] + sbrDec.bufReadOffs,
               codecQmfBufferMpsImag[el][channel] + sbrDec.bufReadOffs,
               codecQmfBufferPVMpsReal[el][channel] + sbrDec.bufReadOffs,
               codecQmfBufferPVMpsImag[el][channel] + sbrDec.bufReadOffs,
               hFrameData->sbrPatchingMode,
               sbrDec.bUseHBE[el],
               sbrQmfBufferMpsReal[el][channel] + sbrDec.bufReadOffs,
               sbrQmfBufferMpsImag[el][channel] + sbrDec.bufReadOffs,
               hFrameData->sbr_invf_mode,
               hFrameData->sbr_invf_mode_prev,
               &FreqBandTableNoise[el][channel][1],
               NoNoiseBands[el][channel],
               sbrDec.lowSubband[el][channel],
               V_k_master[el][channel],
               Num_Master[el][channel],
               sbrDec.outSampleRate,
               frameInfo,
#ifdef LOW_POWER_SBR
               degreeAlias[channel],
#endif
               channel, el
#ifdef AAC_ELD
               ,hFrameData->numTimeSlots
               ,hFrameData->ldsbr
#endif
               ,hFrameData->sbr_header.bPreFlattening
               ,(sbrDec.sbrRatioIndex==SBR_RATIO_INDEX_4_1?1:0)
               );



#ifdef LOW_POWER_SBR
  for (i = 2*hFrameData->frameInfo[1]; i<2*hFrameData->frameInfo[hFrameData->frameInfo[0]+1]; i++) {
    memset (sbrQmfBufferMpsReal[channel][sbrDec.bufReadOffs+i], 0, sbrDec.lowSubband[channel] * sizeof (float));
  }
#endif

  /*
    Adjust envelope of current frame.
  */
  calculateSbrEnvelope (hFrameData,
                        sbrQmfBufferMpsReal[el][channel] + sbrDec.bufReadOffs,
                        sbrQmfBufferMpsImag[el][channel] + sbrDec.bufReadOffs,
                        codecQmfBufferMpsReal[el][channel] + sbrDec.bufReadOffs,
                        codecQmfBufferMpsImag[el][channel] + sbrDec.bufReadOffs,
                        FreqBandTable[el][channel],
                        NSfb[el][channel],
                        FreqBandTableNoise[el][channel],
                        NoNoiseBands[el][channel],
                        Reset[el][channel],
#ifdef LOW_POWER_SBR
                        degreeAlias[el][channel],
#endif
                        channel,
                        el,
#ifndef STEREO_HBE_NOFIX
                        sbrDec.hHbeTransposer[el][channel]->xOverQmf,
#else
                        sbrDec.hHbeTransposer[el]->xOverQmf,
#endif
                        (sbrDec.sbrRatioIndex==SBR_RATIO_INDEX_4_1?1:0)
#ifdef SONY_PVC_DEC
                        ,USE_ORIG_SBR
                        ,NULL
                        ,-1
                        ,USE_ORIG_SBR
#endif /* SONY_PVC_DEC */
                        );


  for ( i = 0; i < noCols; i++ ) {
    for ( k = 0; k < sbrDec.lowSubband[el][channel]; k++ ) {
      qmfOutputReal[i][k] = codecQmfBufferMpsReal[el][channel][sbrDec.bufReadOffs+i][k];
      qmfOutputImag[i][k] = codecQmfBufferMpsImag[el][channel][sbrDec.bufReadOffs+i][k];
    }

    for ( k = sbrDec.lowSubband[el][channel]; k < 64; k++ ) {
      qmfOutputReal[i][k] = sbrQmfBufferMpsReal[el][channel][sbrDec.bufReadOffs+i][k];
      qmfOutputImag[i][k] = sbrQmfBufferMpsImag[el][channel][sbrDec.bufReadOffs+i][k];
    }
  }

  /*
    Update Buffers
  */
  for (i = 0; i < sbrDec.sbrOffset; i++) {
    memmove (codecQmfBufferMpsReal[el][channel][i],
             codecQmfBufferMpsReal[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (codecQmfBufferMpsImag[el][channel][i],
             codecQmfBufferMpsImag[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (sbrQmfBufferMpsReal[el][channel][i],
             sbrQmfBufferMpsReal[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (sbrQmfBufferMpsImag[el][channel][i],
             sbrQmfBufferMpsImag[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));
  }

  Reset[el][channel] = 0;
}


/*******************************************************************************
 Functionname:  sbr_dec
 *******************************************************************************

 Description:   sbr decoder core function

 Arguments:

 Return:        none

*******************************************************************************/
void
sbr_dec ( float* ftimeInPtr,
          float* ftimeOutPtr,
          HANDLE_SBR_FRAME_DATA hFrameData,
          int applyProcessing,
          int channel,
          int el
#ifdef SBR_SCALABLE
          ,int maxSfbFreqLine
#endif
          ,int bDownSampledSbr
#ifdef PARAMETRICSTEREO
          ,int sbrEnablePS,
          float* ftimeOutPtrPS,
          HANDLE_PS_DEC hParametricStereoDec
#endif
          ,int stereoConfigIndex
          /* 20060107 */
          ,int	core_bandwidth
          ,int	bUsedBSAC
#ifdef SONY_PVC_DEC
          ,HANDLE_PVC_DATA	hPvcData
          ,int sbr_mode
          ,int varlen
#endif /* SONY_PVC_DEC */
          )
{
  int i, k;
  int frameMove = sbrDec.lowBandAddSamples;
  int numBandsAnaQmf = sbrDec.numBandsAnaQmf;
  int numBandsSynQmf = sbrDec.numBandsSynQmf;

  float ***qmfBufferReal = &sbrDec.qmfBufferReal[el][channel];
  float ***qmfBufferImag = &sbrDec.qmfBufferImag[el][channel];

  static float lowBandBuffer[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][2624];   /* 2048 + 576 */

  static float sbrQmfBufferReal[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];
  static float sbrQmfBufferImag[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][80][64];

  int *frameInfo = NULL;

#ifdef SONY_PVC_DEC
  int j;
  float pvcQmfBuffer[64][32];
  float pvcTimeSlotQmfBuffer[16*32];
  float pvcDecOut[16*64];
  
  int pvc_flg;
  unsigned char pvc_rate;
#else /* SONY_PVC_DEC */
#endif /* SONY_PVC_DEC */

  /* 20060107 */
  int	core_syn_ch_index =0;
  
  int codecXDelay = (sbrDec.bUseHBE[el] ? CODEC_XDELAY : 0);

#ifdef LOW_POWER_SBR
  float degreeAlias[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][64];
#endif

#ifdef SBR_SCALABLE
  int maxQmfSubbandAac = (int) ( maxSfbFreqLine*32.0f /(sbrDec.coreCodecFrameSize));
#endif

#ifdef SONY_PVC_DEC
  static int prev_sbr_mode = USE_ORIG_SBR;
#endif /* SONY_PVC_DEC */

  sbrDec.bSbrFromMps = (stereoConfigIndex == 3)?1:0;
  sbrDec.hFrameData[el][channel] = hFrameData;

  if(sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
      codecXDelay = 2*codecXDelay;
  }

  if(applyProcessing)
    frameInfo = hFrameData->frameInfo;

  /* 20060107 */
  if (bUsedBSAC==1) {
    if (core_bandwidth !=0) {
      core_syn_ch_index = 64 * (core_bandwidth*2 -1) / (float)sbrDec.outSampleRate;
      core_syn_ch_index = max(core_syn_ch_index-1,sbrDec.lowSubband[el][channel]);
    }
  }
  else if(sbrDec.bUseHBE[el] || sbrDec.bSbrFromMps) {
    core_syn_ch_index = numBandsAnaQmf;
  } else {
    core_syn_ch_index = sbrDec.lowSubband[el][channel];
  }

  if (sbrDec.bSbrFromMps == 1) {
    sbrDec.bufWriteOffs = sbrDec.hybQmfOffset;
  } else {
    sbrDec.bufWriteOffs = sbrDec.sbrOffset;
  }

  /*
    Update buffers
  */
  memmove (lowBandBuffer[el][channel], lowBandBuffer[el][channel] + sbrDec.coreCodecFrameSize,
           frameMove * sizeof (float));

  for (i = 0 ; i < sbrDec.coreCodecFrameSize ; i++) {
    lowBandBuffer[el][channel] [frameMove + i] = (float) ftimeInPtr [i] ;
  }

  /* For critical sampling, buffer updates are done on function entry
     because the reset function needs the unaltered codecQmfBuffer. */
  for (i = 0; i < sbrDec.bufWriteOffs + codecXDelay; i++) {
    memmove (codecQmfBufferReal[el][channel][i],
             codecQmfBufferReal[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (codecQmfBufferImag[el][channel][i],
             codecQmfBufferImag[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));
  }

  for (i = 0; i < sbrDec.bufWriteOffs; i++) {
    memmove (sbrQmfBufferReal[el][channel][i],
             sbrQmfBufferReal[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (sbrQmfBufferImag[el][channel][i],
             sbrQmfBufferImag[el][channel][sbrDec.noCols + i],
             64 * sizeof (float));
  }

  if (sbrDec.bUseHBE[el]) {
    for (i = 0; i < sbrDec.bufWriteOffs; i++) {
      memmove (codecQmfBufferPVReal[el][channel][i],
               codecQmfBufferPVReal[el][channel][sbrDec.noCols + i],
               64 * sizeof (float));
      memmove (codecQmfBufferPVImag[el][channel][i],
               codecQmfBufferPVImag[el][channel][sbrDec.noCols + i],
               64 * sizeof (float));
      }
  }

  /*
    low band codec signal subband filtering
  */
  if (applyProcessing){
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      CalculateSbrAnaFilterbank( lowBandBuffer[el][channel] + sbrDec.startIndexCodecQmf + i * numBandsAnaQmf,
                                 codecQmfBufferReal[el][channel][sbrDec.bufWriteOffs + i + codecXDelay],
                                 codecQmfBufferImag[el][channel][sbrDec.bufWriteOffs + i + codecXDelay],
                                 channel,
                                 el,
#ifdef SBR_SCALABLE
                                 numBandsAnaQmf,
#else
                                 core_syn_ch_index,  /* 20060107 */
#endif
                                 numBandsAnaQmf
#ifdef AAC_ELD
                                 ,hFrameData->ldsbr
#endif
                                 );
    }
  }
  else {
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      CalculateSbrAnaFilterbank( lowBandBuffer[el][channel] + sbrDec.startIndexCodecQmf + i * numBandsAnaQmf,
                                 codecQmfBufferReal[el][channel][sbrDec.bufWriteOffs + i + codecXDelay],
                                 codecQmfBufferImag[el][channel][sbrDec.bufWriteOffs + i + codecXDelay],
                                 channel,
                                 el,
                                 numBandsAnaQmf,
                                 numBandsAnaQmf
#ifdef AAC_ELD
                                ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif
                                 );
    }
  }
  if (applyProcessing){
    if (sbrDec.bUseHBE[el]) {
      int xposDelay = codecXDelay;
      if(sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
        if(sbrDec.bUseHQ) {
          xposDelay = codecXDelay - 8;
        } else {
          xposDelay = codecXDelay - 32;
        }
      }
      
      ptrHbeApplyFunc(
#ifndef STEREO_HBE_NOFIX
                      sbrDec.hHbeTransposer[el][channel],
#else
                      sbrDec.hHbeTransposer[el],
#endif
                      codecQmfBufferReal[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                      codecQmfBufferImag[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                      sbrDec.noCols,
                      codecQmfBufferPVReal[el][channel] + sbrDec.bufWriteOffs,
                      codecQmfBufferPVImag[el][channel] + sbrDec.bufWriteOffs,
                      hFrameData->sbrOversamplingFlag,
                      hFrameData->sbrPitchInBins);

      if (sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
#ifndef STEREO_HBE_NOFIX
        copyHarmonicSpectrum(sbrDec.hHbeTransposer[el][channel]->xOverQmf,codecQmfBufferPVReal[el][channel]+sbrDec.bufWriteOffs,codecQmfBufferPVImag[el][channel]+sbrDec.bufWriteOffs, sbrDec.noCols, sbrDec.hHbeTransposer[el][channel]->maxStretch);
#else
        copyHarmonicSpectrum(sbrDec.hHbeTransposer[el]->xOverQmf,codecQmfBufferPVReal[el][channel]+sbrDec.bufWriteOffs,codecQmfBufferPVImag[el][channel]+sbrDec.bufWriteOffs, sbrDec.noCols, sbrDec.hHbeTransposer[el]->maxStretch);
#endif
      }
    }
  }

#ifdef SONY_PVC_DEC
	/* Preapare QMF energy for PVC decoder */
	if(sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1){

  		for(i = 0; i < sbrDec.noCols; i++){
			for(j = 0; j < 16; j++){
				pvcQmfBuffer[i][j] = codecQmfBufferReal[el][channel][sbrDec.bufReadOffs + i][j] * codecQmfBufferReal[el][channel][sbrDec.bufReadOffs + i][j]
					+ codecQmfBufferImag[el][channel][sbrDec.bufReadOffs + i][j] * codecQmfBufferImag[el][channel][sbrDec.bufReadOffs + i][j];
			}
		}

		for(i = 0; i < 16; i++){
			for(j = 0; j < 16; j++){
				pvcTimeSlotQmfBuffer[16*i+j] = (pvcQmfBuffer[4*i][j]+pvcQmfBuffer[4*i+1][j]
												+pvcQmfBuffer[4*i+2][j]+pvcQmfBuffer[4*i+3][j])/4.0f;
			}
		}
	}else{
		for(i = 0; i < sbrDec.noCols; i++){
			for(j = 0; j < 32; j++){
			    pvcQmfBuffer[i][j] = codecQmfBufferReal[el][channel][sbrDec.bufReadOffs + i][j] * codecQmfBufferReal[el][channel][sbrDec.bufReadOffs + i][j]
					+ codecQmfBufferImag[el][channel][sbrDec.bufReadOffs + i][j] * codecQmfBufferImag[el][channel][sbrDec.bufReadOffs + i][j];
			}
		}

		for(i = 0; i < 16; i++){
			for(j = 0; j < 32; j++){
				pvcTimeSlotQmfBuffer[32*i+j] = (pvcQmfBuffer[2*i][j]+pvcQmfBuffer[2*i+1][j])/2.0f;
			}
		}
	}
#endif /* SONY_PVC_DEC */

  if (!sbrDec.bSbrFromMps && applyProcessing){
    /*
      Inverse filtering of lowband + HF generation
    */
    generateHF ( codecQmfBufferReal[el][channel] + sbrDec.bufReadOffs,
                 codecQmfBufferImag[el][channel] + sbrDec.bufReadOffs,
                 codecQmfBufferPVReal[el][channel] + sbrDec.bufReadOffs,
                 codecQmfBufferPVImag[el][channel] + sbrDec.bufReadOffs,
                 hFrameData->sbrPatchingMode,
                 sbrDec.bUseHBE[el],
                 sbrQmfBufferReal[el][channel] + sbrDec.bufReadOffs,
                 sbrQmfBufferImag[el][channel] + sbrDec.bufReadOffs,
                 hFrameData->sbr_invf_mode,
                 hFrameData->sbr_invf_mode_prev,
                 &FreqBandTableNoise[el][channel][1],
                 NoNoiseBands[el][channel],
                 sbrDec.lowSubband[el][channel],
                 V_k_master[el][channel],
                 Num_Master[el][channel],
                 sbrDec.outSampleRate,
                 frameInfo,
#ifdef LOW_POWER_SBR
                 degreeAlias[el][channel],
#endif
                 channel, el
#ifdef AAC_ELD
                 ,hFrameData->numTimeSlots
                 ,hFrameData->ldsbr
#endif
                 ,hFrameData->sbr_header.bPreFlattening
                 ,(sbrDec.sbrRatioIndex==SBR_RATIO_INDEX_4_1?1:0)
                 );
    
#ifdef LOW_POWER_SBR
    for (i = 2*hFrameData->frameInfo[1]; i<2*hFrameData->frameInfo[hFrameData->frameInfo[0]+1]; i++) {
      memset (sbrQmfBufferReal[channel][sbrDec.bufReadOffs+i], 0, sbrDec.lowSubband[channel] * sizeof (float));
    }
#endif

#ifdef SONY_PVC_DEC
    if(sbr_mode == USE_PVC_SBR){
      pvc_flg = 1;
    }else{
      pvc_flg = 0;
    }
    if (sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
      pvc_rate = 4;
    } else {
      pvc_rate = 2;
    }
    pvc_decode_frame(hPvcData,
                     pvc_flg, 
                     pvc_rate,
                     sbrDec.lowSubband[el][channel],
                     hFrameData->pvc_frameInfo[1],
                     pvcTimeSlotQmfBuffer,
                     pvcDecOut
                     );
#endif /* SONY_PVC_DEC */
    
    /*
      Adjust envelope of current frame.
    */
    calculateSbrEnvelope (hFrameData,
                          sbrQmfBufferReal[el][channel] + sbrDec.bufReadOffs,
                          sbrQmfBufferImag[el][channel] + sbrDec.bufReadOffs,
                          codecQmfBufferReal[el][channel] + sbrDec.bufReadOffs,
                          codecQmfBufferImag[el][channel] + sbrDec.bufReadOffs,
                          FreqBandTable[el][channel],
                          NSfb[el][channel],
                          FreqBandTableNoise[el][channel],
                          NoNoiseBands[el][channel],
                          Reset[el][channel],
#ifdef LOW_POWER_SBR
                          degreeAlias[el][channel],
#endif
                          channel,
                          el,
#ifndef STEREO_HBE_NOFIX
                          sbrDec.hHbeTransposer[el][channel]->xOverQmf,
#else
                          sbrDec.hHbeTransposer[el]->xOverQmf,
#endif
                          (sbrDec.sbrRatioIndex==SBR_RATIO_INDEX_4_1?1:0)
#ifdef SONY_PVC_DEC
                          ,sbr_mode
                          ,pvcDecOut
                          ,varlen
                          ,prev_sbr_mode
#endif /* SONY_PVC_DEC */
                          );
    
  }
  else {
    /* no sbr, set high band buffers to zero */
    for (i = 0; i < 64; i++) {
      memset (sbrQmfBufferReal[el][channel][i], 0, 64 * sizeof (float));
      memset (sbrQmfBufferImag[el][channel][i], 0, 64 * sizeof (float));
    }
  }

  /*
    Synthesis subband filtering.
  */
  for ( i = 0; i < sbrDec.noCols; i++ ) {
    int   xoverBand;

    if (applyProcessing) {
      int stopBorder;
      if (sbrDec.sbrRatioIndex==SBR_RATIO_INDEX_4_1) {
        stopBorder = 4*hFrameData->frameInfo[1];
      } else {
        stopBorder = 2*hFrameData->frameInfo[1];
      }
#ifdef SBR_SCALABLE
      if ( i < stopBorder )
        xoverBand = max(sbrDec.prevLowSubband[channel],sbrDec.prevMaxQmfSubbandAac[channel]);
      else
        xoverBand = max(sbrDec.lowSubband[channel],maxQmfSubbandAac);
#else
      if ( i < stopBorder )
        xoverBand = sbrDec.prevLowSubband[el][channel];
      else
        xoverBand = sbrDec.lowSubband[el][channel];

      /* 20060107 */
      if (bUsedBSAC)
        xoverBand = core_syn_ch_index;

#endif
    }
    else{
      xoverBand = numBandsAnaQmf;
    }

    for ( k = 0; k < 64; k++ ) {
      int kmax;

      qmfBufferReal[0][i][k] = 0.0;
      qmfBufferImag[0][i][k] = 0.0;
      if (sbrDec.bSbrFromMps) {
        kmax = numBandsAnaQmf;
      }
      else {
        kmax = xoverBand;
      }

      if ( k < kmax) {
#ifdef OLD_SBR_MPS_ALIGNMENT
        if (sbrDec.bSbrFromMps) {
          qmfBufferReal[0][i][k] += codecQmfBufferReal[channel][sbrDec.bufReadOffs+i+HYBRID_FILTER_DELAY][k];
          qmfBufferImag[0][i][k] += codecQmfBufferImag[channel][sbrDec.bufReadOffs+i+HYBRID_FILTER_DELAY][k];
        }
        else{
          qmfBufferReal[0][i][k] += codecQmfBufferReal[channel][sbrDec.bufReadOffs+i][k];
          qmfBufferImag[0][i][k] += codecQmfBufferImag[channel][sbrDec.bufReadOffs+i][k];
        }
#else
        if (k < 3 && stereoConfigIndex > 0) {
          qmfBufferReal[0][i][k] += codecQmfBufferReal[el][channel][sbrDec.bufReadOffs+i+HYBRID_FILTER_DELAY][k];
          qmfBufferImag[0][i][k] += codecQmfBufferImag[el][channel][sbrDec.bufReadOffs+i+HYBRID_FILTER_DELAY][k];
        }
        else{
          qmfBufferReal[0][i][k] += codecQmfBufferReal[el][channel][sbrDec.bufReadOffs+i][k];
          qmfBufferImag[0][i][k] += codecQmfBufferImag[el][channel][sbrDec.bufReadOffs+i][k];
        }
#endif
      }

      if (!sbrDec.bSbrFromMps){
        if ( k >= xoverBand
#ifdef LOW_POWER_SBR
             - 1
#endif
             )
        {
          qmfBufferReal[0][i][k] += sbrQmfBufferReal[el][channel][sbrDec.bufReadOffs+i][k];
          qmfBufferImag[0][i][k] += sbrQmfBufferImag[el][channel][sbrDec.bufReadOffs+i][k];
        }
      }
    }
  }

#ifdef PARAMETRICSTEREO
  if (sbrEnablePS) {
    for ( i = 32; i < 32+6; i++ ) {
      for ( k = 0; k < 5; k++ ) {
        qmfBufferReal[0][i][k] = codecQmfBufferReal[channel][sbrDec.bufReadOffs+i][k];
        qmfBufferImag[0][i][k] = codecQmfBufferImag[channel][sbrDec.bufReadOffs+i][k];
      }
    }
  }


  /*
    Parametric stereo processing.
  */
  if (sbrEnablePS && applyProcessing) {
    tfApplyPsFrame(hParametricStereoDec,
                   qmfBufferReal[0],
                   qmfBufferImag[0],
                   qmfBufferReal[1],
                   qmfBufferImag[1],
                   sbrDec.highSubband[0]);
  }
  else {
    /* no PS, output L=R=M */
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      for ( k = 0; k < 64; k++ ) {
        qmfBufferReal[1][i][k] = qmfBufferReal[0][i][k];
        qmfBufferImag[1][i][k] = qmfBufferImag[0][i][k];
      }
    }
  }
#endif

  /*
    Synthesis subband filtering.
  */

  for ( i = 0; i < sbrDec.noCols; i++ ) {
    if(bDownSampledSbr){
      CalculateSbrSynFilterbank(qmfBufferReal[0][i],
                                qmfBufferImag[0][i],
                                ftimeOutPtr + i * numBandsSynQmf,
                                bDownSampledSbr,
                                channel,
                                el
#ifdef AAC_ELD
                                ,hFrameData->ldsbr
#endif
                                );
    }
    else{
      CalculateSbrSynFilterbank(qmfBufferReal[0][i],
                                qmfBufferImag[0][i],
                                ftimeOutPtr + i * numBandsSynQmf,
                                bDownSampledSbr,
                                channel,
                                el
#ifdef AAC_ELD
                                ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif
                                );

    }

#ifdef PARAMETRICSTEREO
    if (sbrEnablePS) {

      if(bDownSampledSbr){
        CalculateSbrSynFilterbank(qmfBufferReal[1][i],
                                  qmfBufferImag[1][i],
                                  ftimeOutPtrPS + i * numBandsSynQmf,
                                  bDownSampledSbr,
                                  channel+1
#ifdef AAC_ELD
                                  ,hFrameData->ldsbr
#endif
          );
      }
      else{
        CalculateSbrSynFilterbank(qmfBufferReal[1][i],
                                  qmfBufferImag[1][i],
                                  ftimeOutPtrPS + i * numBandsSynQmf,
                                  bDownSampledSbr,
                                  channel+1
#ifdef AAC_ELD
                                  ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif
          );
      }
    }
#endif
  }

#ifdef SONY_PVC_DEC
  prev_sbr_mode = sbr_mode;
#endif /* SONY_PVC_DEC */


  if (!sbrDec.bSbrFromMps)
    Reset[el][channel] = 0;

  if(applyProcessing && !sbrDec.bSbrFromMps){
    sbrDec.prevLowSubband[el][channel] = sbrDec.lowSubband[el][channel];

#ifdef SBR_SCALABLE
    sbrDec.prevMaxQmfSubbandAac[channel] = maxQmfSubbandAac;
#endif
  }
}


/*******************************************************************************
 Functionname:  initSbrDec
 *******************************************************************************

 Description:   initializes sbr decoder structure

 Arguments:

 Return:        none

*******************************************************************************/
void
initSbrDec ( int codecSampleRate,
             int bDownSampledSbr,
             int coreCodecFrameSize,
             SBR_RATIO_INDEX sbrRatioIndex
#ifdef AAC_ELD
             ,int ldsbr
#endif
             ,int * bUseHBE
             ,int   bUseHQtransposer
             )
{
  int outFrameSize,i, el;
#ifdef AAC_ELD
  int upsampleFac;
#endif
  int numChannelsAnalysisQmf = 0;
  int numChannelsSynthesisQmf = bDownSampledSbr?32:64;
  int qmfBufLen;

  switch(sbrRatioIndex){
  case SBR_RATIO_INDEX_2_1:
    numChannelsAnalysisQmf = 32;
    sbrDec.outSampleRate = (int) (2 * codecSampleRate);
    outFrameSize = (int) (2 * coreCodecFrameSize);
    break;
  case SBR_RATIO_INDEX_8_3:
    numChannelsAnalysisQmf = 24;
    sbrDec.outSampleRate = (int) (2 * codecSampleRate); /* codecSampleRate == 4/3 of actual sampling rate */
    outFrameSize = (int) ((8 * coreCodecFrameSize)/3);
    break;
  case SBR_RATIO_INDEX_4_1:
    numChannelsAnalysisQmf = 16;
    sbrDec.outSampleRate = (int) (4 * codecSampleRate);
    outFrameSize = (int) (4 * coreCodecFrameSize);
    break;
  default:
    assert(0);
    break;
  }

  sbrDec.numBandsAnaQmf     = numChannelsAnalysisQmf;
  sbrDec.numBandsSynQmf     = numChannelsSynthesisQmf;
  sbrDec.coreCodecFrameSize = coreCodecFrameSize;
  sbrDec.sbrRatioIndex      = sbrRatioIndex;

  for(el = 0; el < MAX_NUM_ELEMENTS; el++){
    for(i=0;i<MAX_NUM_CHANNELS;i++){
      sbrDec.prevLowSubband[el][i] = 64;
      sbrDec.sbStopCodec[el][i]    = 64;
    }
  }


  /* set sbr sampling frequency */

#ifdef AAC_ELD
  InitSbrAnaFilterbank (ldsbr, numChannelsAnalysisQmf);
#else
  InitSbrAnaFilterbank (numChannelsAnalysisQmf);
#endif
  InitSbrSynFilterbank (bDownSampledSbr
#ifdef AAC_ELD
      ,ldsbr
#endif
        );


#ifdef AAC_ELD
  if(ldsbr==1 && upsampleFac==2) {
    qmfBufLen = 32;
  } else {
#endif
    if(sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
      qmfBufLen = 64+16;
    } else {
      qmfBufLen = 64+6;
    }
#ifdef AAC_ELD
  }
#endif

  /* allocate qmf memory */
  {
    int el, ch, i;
    sbrDec.qmfBufferReal  = (float ****) calloc (MAX_NUM_ELEMENTS, sizeof(float ***));
    sbrDec.qmfBufferImag  = (float ****) calloc (MAX_NUM_ELEMENTS, sizeof(float ***));
    for(el = 0; el < MAX_NUM_ELEMENTS; el++){
      sbrDec.qmfBufferReal[el] = (float***) calloc(MAX_NUM_CHANNELS, sizeof(float**));
      sbrDec.qmfBufferImag[el] = (float***) calloc(MAX_NUM_CHANNELS, sizeof(float**));
      for(ch = 0; ch < MAX_NUM_CHANNELS; ch++){
        sbrDec.qmfBufferReal[el][ch] = (float**) calloc(qmfBufLen, sizeof(float*));
        sbrDec.qmfBufferImag[el][ch] = (float**) calloc(qmfBufLen, sizeof(float*));
        for(i = 0; i < qmfBufLen; i++){
          sbrDec.qmfBufferReal[el][ch][i] = (float*) calloc(qmfBufLen, sizeof(float));
          sbrDec.qmfBufferImag[el][ch][i] = (float*) calloc(qmfBufLen, sizeof(float));
        }
      }
    }
  }

  /* Direct assignments */

  sbrDec.noCols             = outFrameSize / 64;
#ifdef AAC_ELD
  if(ldsbr==1) {
    sbrDec.bufWriteOffs       = 2;
  } else {
#endif

    if (sbrRatioIndex == SBR_RATIO_INDEX_4_1 ) {
      sbrDec.sbrOffset       = 2*6+2;
    } else {
      sbrDec.sbrOffset       = 6+2;
    }
    sbrDec.hybQmfOffset      = 6+2;

#ifdef AAC_ELD
  }
#endif
  sbrDec.bufReadOffs        = 2;
  sbrDec.qmfBufLen          = sbrDec.noCols + sbrDec.bufWriteOffs;
  sbrDec.lowBandAddSamples  = (10*numChannelsAnalysisQmf) - numChannelsAnalysisQmf; /* filter length - no. channels */
  sbrDec.startIndexCodecQmf = 0;

  sbrDec.bUseHQ  = bUseHQtransposer;

  for(el = 0; el < MAX_NUM_ELEMENTS; el++){
    sbrDec.bUseHBE[el] = bUseHBE[el];


    if (bUseHBE[el] != 0) {

      if(bUseHQtransposer)
      {
#ifndef STEREO_HBE_NOFIX
        PhaseVocoderCreate(&sbrDec.hHbeTransposer[el][0],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);

        PhaseVocoderCreate(&sbrDec.hHbeTransposer[el][1],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#else
        PhaseVocoderCreate(&sbrDec.hHbeTransposer[el],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#endif

        ptrHbeApplyFunc = &PhaseVocoderApply;
      }
      else
      {

#ifndef STEREO_HBE_NOFIX
        QmfTransposerCreate(&sbrDec.hHbeTransposer[el][0],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
        QmfTransposerCreate(&sbrDec.hHbeTransposer[el][1],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#else
        QmfTransposerCreate(&sbrDec.hHbeTransposer[el],
            sbrDec.coreCodecFrameSize,
            sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#endif

        ptrHbeApplyFunc = &QmfTransposerApply;
      }

#ifndef STEREO_HBE_NOFIX
      if (sbrDec.hHbeTransposer[el][0] == NULL) {
        CommonExit(-1, "PhaseVocoderCreate/QmfTransposerCreate failed.");
        return;
      }

      if (sbrDec.hHbeTransposer[el][1] == NULL) {
        CommonExit(-1, "PhaseVocoderCreate/QmfTransposerCreate failed.");
        return;
      }
#else
      if (sbrDec.hHbeTransposer[el] == NULL) {
        CommonExit(-1, "PhaseVocoderCreate/QmfTransposerCreate failed.");
        return;
      }
#endif

    } else {
#ifndef STEREO_HBE_NOFIX
      sbrDec.hHbeTransposer[el][0] = NULL;
      sbrDec.hHbeTransposer[el][1] = NULL;
#else
      sbrDec.hHbeTransposer[el] = NULL;
#endif
    }
  }


}


/*******************************************************************************
 Functionname:  deleteSbrDec
 *******************************************************************************

 Description:   frees dynamically allocated memory

 Arguments:

 Return:        none

*******************************************************************************/
void deleteSbrDec(void){

  int i, j, k;
  if(sbrDec.qmfBufferReal){
    for(i = 0; i < MAX_NUM_ELEMENTS; i++){
      if(sbrDec.qmfBufferReal[i]){
        for(j = 0; j < MAX_NUM_CHANNELS; j++){
          if(sbrDec.qmfBufferReal[i][j]){
            for(k = 0; k < sbrDec.qmfBufLen; k++){
              free(sbrDec.qmfBufferReal[i][j][k]);
            }
            free(sbrDec.qmfBufferReal[i][j]);
          }
        }
        free(sbrDec.qmfBufferReal[i]);
      }
    }
    free(sbrDec.qmfBufferReal);
  }

  if(sbrDec.qmfBufferImag){
      for(i = 0; i < MAX_NUM_ELEMENTS; i++){
        if(sbrDec.qmfBufferImag[i]){
          for(j = 0; j < MAX_NUM_CHANNELS; j++){
            if(sbrDec.qmfBufferImag[i][j]){
              for(k = 0; k < sbrDec.qmfBufLen; k++){
                free(sbrDec.qmfBufferImag[i][j][k]);
              }
              free(sbrDec.qmfBufferImag[i][j]);
            }
          }
          free(sbrDec.qmfBufferImag[i]);
        }
      }
      free(sbrDec.qmfBufferImag);
    }

#ifndef STEREO_HBE_NOFIX
  for(i = 0; i < MAX_NUM_ELEMENTS; i++){
    int ch = 0;
    for(ch = 0; ch < 2; ch++){
      if(sbrDec.hHbeTransposer[i][ch] != NULL){
        if(sbrDec.bUseHQ){
          PhaseVocoderClose(sbrDec.hHbeTransposer[i][ch]);
        } else {
          QmfTransposerClose(sbrDec.hHbeTransposer[i][ch]);        
        }
      }
    }
  }
#else
  for(i = 0; i < MAX_NUM_ELEMENTS; i++){
    if(sbrDec.hHbeTransposer[i]){
      if(sbrDec.bUseHQ)
        PhaseVocoderClose(sbrDec.hHbeTransposer[i]);
      else
        QmfTransposerClose(sbrDec.hHbeTransposer[i]);
    }
  }
#endif

}

/*******************************************************************************
 Functionname:  resetSbrDec
 *******************************************************************************

 Description:   resets sbr decoder structure

 Arguments:

 Return:        errorCode, noError if successful

 *******************************************************************************/
void
resetSbrDec ( HANDLE_SBR_FRAME_DATA hFrameData,
              float upsampleFac,
              int channel,
              int el)
{
  int ch,l,k;
  int startBand, stopBand;
  int startSlot = sbrDec.bufReadOffs;

  Reset[el][channel] = 1;

  resetSbrFreqBandTables ( &(hFrameData->sbr_header),
                           sbrDec.outSampleRate,
                           channel,
                           el,
                           upsampleFac);

  sbrDec.sbStopCodec[el][channel] = sbrDec.lowSubband[el][channel];

  if (upsampleFac == 4) {
    if (sbrDec.sbStopCodec[el][channel] > (int) (upsampleFac * 16) ) {
      sbrDec.sbStopCodec[el][channel] = (int) (upsampleFac * 16);
    }
  } else {
    if (sbrDec.sbStopCodec[el][channel] > (int) (upsampleFac * 32) ) {
      sbrDec.sbStopCodec[el][channel] = (int) (upsampleFac * 32);
    }
  }

  hFrameData->nSfb[LO] = NSfb[el][channel][LO];
  hFrameData->nSfb[HI] = NSfb[el][channel][HI];
  hFrameData->nNfb     = hFrameData->sbr_header.noNoiseBands;
  hFrameData->offset   = 2* hFrameData->nSfb[LO] - hFrameData->nSfb[HI];

  startBand = sbrDec.prevLowSubband[el][channel];
  stopBand  = sbrDec.lowSubband[el][channel];

  {
    int elIdx = 0;
    for(elIdx = 0; elIdx < MAX_NUM_ELEMENTS; elIdx++){
      if (!sbrDec.bUseHBE[elIdx]) {
        for(ch=0; ch<MAX_NUM_CHANNELS; ch++){
          for (l=0; l < sbrDec.bufReadOffs; l++) {
            for ( k=startBand; k<stopBand; k++) {
              codecQmfBufferReal[elIdx][ch][l][k] = 0.0;
              codecQmfBufferImag[elIdx][ch][l][k] = 0.0;
            }
          }
          for (l=startSlot; l < sbrDec.bufWriteOffs; l++) {
            for ( k=startBand; k<stopBand; k++) {
              codecQmfBufferReal[elIdx][ch][l][k] = 0.0;
              codecQmfBufferImag[elIdx][ch][l][k] = 0.0;
            }
          }
        }
      }
    }
  }
#ifndef STEREO_HBE_NOFIX
  if (sbrDec.bUseHBE[el] && sbrDec.hHbeTransposer[el][channel])
#else
  if (sbrDec.bUseHBE[el] && sbrDec.hHbeTransposer[el])
#endif
  {
    int k,i;

#ifndef STEREO_HBE_NOFIX
    if(sbrDec.bUseHQ)
      PhaseVocoderReInit(sbrDec.hHbeTransposer[el][channel],
                         FreqBandTable[el][channel],
                         NSfb[el][channel],
                         sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
    else
      QmfTransposerReInit(sbrDec.hHbeTransposer[el][channel],
                         FreqBandTable[el][channel],
                         NSfb[el][channel],
                         sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#else
    if(sbrDec.bUseHQ)
      PhaseVocoderReInit(sbrDec.hHbeTransposer[el],
                         FreqBandTable[el][channel],
                         NSfb[el][channel],
                         sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
    else
      QmfTransposerReInit(sbrDec.hHbeTransposer[el],
                         FreqBandTable[el][channel],
                         NSfb[el][channel],
                         sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1?1:0);
#endif


    for (k=0;k<2;k++) { 
      if(!((sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) && (!sbrDec.bUseHQ) && (k==0))) {
        int xposDelay = sbrDec.noCols*k;
        if(sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
          if(sbrDec.bUseHQ) {
            xposDelay = sbrDec.noCols*k - 8;
          } else {
            xposDelay = sbrDec.noCols*k - 32;
          }
        }

        for (i = 0; i < sbrDec.bufWriteOffs; i++) {
          memmove (codecQmfBufferPVReal[el][channel][i],
                   codecQmfBufferPVReal[el][channel][sbrDec.noCols + i],
                   64 * sizeof (float));
          memmove (codecQmfBufferPVImag[el][channel][i],
                   codecQmfBufferPVImag[el][channel][sbrDec.noCols + i],
                   64 * sizeof (float));
        }
  
#ifndef STEREO_HBE_NOFIX
        ptrHbeApplyFunc(sbrDec.hHbeTransposer[el][channel],
                        codecQmfBufferReal[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                        codecQmfBufferImag[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                        sbrDec.noCols,
                        codecQmfBufferPVReal[el][channel] + sbrDec.bufWriteOffs,
                        codecQmfBufferPVImag[el][channel] + sbrDec.bufWriteOffs,
                        hFrameData->sbrOversamplingFlag,
                        hFrameData->sbrPitchInBins);
#else
        ptrHbeApplyFunc(sbrDec.hHbeTransposer[el],
                        codecQmfBufferReal[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                        codecQmfBufferImag[el][channel] + sbrDec.bufWriteOffs + xposDelay,
                        sbrDec.noCols,
                        codecQmfBufferPVReal[el][channel] + sbrDec.bufWriteOffs,
                        codecQmfBufferPVImag[el][channel] + sbrDec.bufWriteOffs,
                        hFrameData->sbrOversamplingFlag,
                        hFrameData->sbrPitchInBins);
#endif

        if (sbrDec.sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
#ifndef STEREO_HBE_NOFIX
          copyHarmonicSpectrum(sbrDec.hHbeTransposer[el][channel]->xOverQmf,codecQmfBufferPVReal[el][channel]+sbrDec.bufWriteOffs,codecQmfBufferPVImag[el][channel]+sbrDec.bufWriteOffs, sbrDec.noCols, sbrDec.hHbeTransposer[el][channel]->maxStretch);
#else
          copyHarmonicSpectrum(sbrDec.hHbeTransposer[el]->xOverQmf,codecQmfBufferPVReal[el][channel]+sbrDec.bufWriteOffs,codecQmfBufferPVImag[el][channel]+sbrDec.bufWriteOffs, sbrDec.noCols, sbrDec.hHbeTransposer[el]->maxStretch);
#endif
        }
      }
    }
  }

}


/*******************************************************************************
 Functionname:  resetSbrFreqBandTables
 *******************************************************************************

 Description:
 Arguments:
 Return:        errorCode, noError if successful
 Revised:

*******************************************************************************/
void
resetSbrFreqBandTables ( SBR_HEADER_DATA *headerData,
                         int samplingFreq,
                         int channel, int el,
                         float upsampleFac)
{

  int lsbM, lsb, usb;


  /*Calculate master frequency function */
  sbrdecFindStartAndStopBand(samplingFreq,
                             headerData->startFreq,
                             headerData->stopFreq,
                             upsampleFac,
                             &lsbM, &usb);

  sbrdecUpdateFreqScale(V_k_master[el][channel], &Num_Master[el][channel],
                        lsbM, usb, headerData->freqScale,
                        headerData->alterScale, 0, upsampleFac);


  /*Derive Hiresolution from master frequency function*/
  sbrdecUpdateHiRes(FreqBandTable[el][channel][HI], &NSfb[el][channel][HI],
                    V_k_master[el][channel], Num_Master[el][channel],
                    headerData->xover_band );
  /*Derive  Loresolution from Hiresolution*/
  sbrdecUpdateLoRes(FreqBandTable[el][channel][LO], &NSfb[el][channel][LO],
                    FreqBandTable[el][channel][HI], NSfb[el][channel][HI]);


  lsb = FreqBandTable[el][channel][LOW_RES][0];
  usb = FreqBandTable[el][channel][LOW_RES][NSfb[el][channel][LOW_RES]];

  sbrDec.lowSubband[el][channel]  = lsb;
  sbrDec.highSubband[el][channel] = usb;
  sbrDec.noSubbands[el][channel]  = usb - lsb;

  /*Calculate number of noise bands*/
  if(headerData->noise_bands == 0)
    {
      NoNoiseBands[el][channel] = 1;
    }
  else /* Calculate nor of noise bands 1,2 or 3 bands/octave */
    {
      NoNoiseBands[el][channel] = (int) ( headerData->noise_bands * log((double) usb / lsb) / log(2.0) + 0.5 );

      if( NoNoiseBands[el][channel] == 0) {
        NoNoiseBands[el][channel] = 1;
      }
    }

  headerData->noNoiseBands = NoNoiseBands[el][channel];

  /* Get noise bands */
  sbrdecDownSampleLoRes(FreqBandTableNoise[el][channel],
                        NoNoiseBands[el][channel],
                        FreqBandTable[el][channel][LO],
                        NSfb[el][channel][LO]);

}


void sbrDecGetQmfSamples(int numChannels, int el,
                         float ***qmfBufferReal, /* [ch][timeSlots][QMF Bands] */
                         float ***qmfBufferImag  /* [ch][timeSlots][QMF Bands] */
                         )
{
  int ch;
  assert(numChannels <= 2);

  for(ch=0; ch<numChannels; ch++){
    qmfBufferReal[ch] = sbrDec.qmfBufferReal[el][ch];
    qmfBufferImag[ch] = sbrDec.qmfBufferImag[el][ch];
  }
}

