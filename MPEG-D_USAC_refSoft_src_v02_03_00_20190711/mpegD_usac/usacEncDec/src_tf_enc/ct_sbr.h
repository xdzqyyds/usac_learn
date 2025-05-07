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

 $Id: ct_sbr.h,v 1.12 2011-06-29 11:39:21 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr encoding functions $Revision: 1.12 $
*/

/* 20060107 */

#ifndef _CT_SBR_H
#define _CT_SBR_H

#undef PARAMETRICSTEREO /*temporally disabled for BSAC Extensions*/

#include "bitstream.h"
#include "ct_polyphase_enc.h"
#include "ct_sbrdecoder.h"
#ifdef PARAMETRICSTEREO
#include "ct_psenc.h"
#endif

#ifdef SONY_PVC
#include "sony_pvcenc_prepro.h"
#endif /* SONY_PVC */

#ifdef SONY_PVC_ENC
#include "sony_pvcenc.h"
#endif

#ifdef SONY_PVC_ENC
#define DELAY (4697-313)
#else
#define DELAY 4697
#endif
#define TIME_BUF_SIZE_41 (640 + (64-1)*64 + 2*DELAY)
#define TIME_BUF_SIZE_21 (640 + (32-1)*64 + DELAY)

#define MAX_ENVELOPES           16

#define MAX_FREQ_COEFFS         58

#define MAX_NOISE_ENVELOPES      2
#define MAX_NOISE_COEFFS         5

typedef enum {
  SBR_TF,
  SBR_USAC, 
  SBR_USAC_HARM
} SBR_MODE;

typedef struct {

  int sendHdrCnt;
  int sendHdrFrame;

  int startFreq;
  int stopFreq;
  int numBands;
  int *freqBandTable;
  int numNoiseBands;

  SBR_RATIO_INDEX sbrRatioIndex;

  int bufOffset;
  float timeBuffer[TIME_BUF_SIZE_41];

  int envData[MAX_ENVELOPES][MAX_FREQ_COEFFS];
  int noiseData[MAX_NOISE_ENVELOPES][MAX_NOISE_COEFFS];

  int count;
  int esc_count;
  int fillBits;
  int totalBits;

  struct SBR_ENCODER_ANA_FILTERBANK sbrFbank;

#ifdef PARAMETRICSTEREO
  int psAvailable;
  PS_DATA psData;
#endif

  int sbrOversamplingFlag;
  int sbrPitchInBins;

  int bs_interTes;

#ifdef SONY_PVC_ENC
  unsigned int bs_pvc;
  int switchPvcFlg;
  PVC_DATA_STRUCT hPVC;
#endif
} SBR_CHAN;

void SbrInit(int bitRate, int samplingFreq, int nChan, SBR_RATIO_INDEX sbrRatioIndex, SBR_CHAN *sbrChan);

int SbrEncodeSingleChannel_BSAC(SBR_CHAN *sbrChan,
                                SBR_MODE sbrMode,
                           float *samples
#ifdef PARAMETRICSTEREO
                           , float *samplesRight
#endif
#ifdef SONY_PVC_ENC
                           , int *pvc_mode
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
							,int const bUsacIndependenceFlag
#endif
#endif
                           );
/*
int SbrEncodeChannelPair(SBR_CHAN *sbrChanL,
                         SBR_CHAN *sbrChanR,
                         float *samplesL,
                         float *samplesR);
*/
int SbrEncodeChannelPair_BSAC(SBR_CHAN *sbrChanL,
                              SBR_CHAN *sbrChanR,
                              SBR_MODE sbrMode,
                              float *samplesL,
                              float *samplesR, 
                              int bUsacIndependenceFlag);


int WriteSbrSCE(SBR_CHAN *sbrChan,
                SBR_MODE sbrMode,
                HANDLE_BSBITSTREAM fixedStream
#ifdef SONY_PVC_ENC
		,int pvc_mode
#endif
                ,int const bUsacIndependenceFlag
                );  /* 20060107 */

int WriteSbrCPE(SBR_CHAN *sbrChanL,
                SBR_CHAN *sbrChanR,
                SBR_MODE sbrMode,
                HANDLE_BSBITSTREAM fixedStream,  /* 20060107 */
                int const bUsacIndependenceFlag);

#endif
