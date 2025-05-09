/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Markus Multrus

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
$Id: usac_config.c,v 1.17.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "allHandles.h"

#include "obj_descr.h"           /* structs */
#include "sac_dec_interface.h"
#include "spatial_bitstreamreader.h"
#include "usac_channelconf.h"

#include "bitstream.h"
#include "common_m4a.h"

#include "usac_config.h"

#ifndef min
#define min(a,b) ((a<b)?a:b)
#endif

#ifndef max
#define max(a,b) ((a>b)?a:b)
#endif


USAC_SBR_RATIO_INDEX UsacConfig_GetSbrRatioIndex(unsigned int coreSbrFrameLengthIndex){

  USAC_SBR_RATIO_INDEX sbrRatioIndex = USAC_SBR_RATIO_INDEX_INVALID;

  switch(coreSbrFrameLengthIndex){
  case 0:
  case 1:
    sbrRatioIndex = USAC_SBR_RATIO_INDEX_NO_SBR;
    break;
  case 2:
    sbrRatioIndex = USAC_SBR_RATIO_INDEX_8_3;
    break;
  case 3:
    sbrRatioIndex = USAC_SBR_RATIO_INDEX_2_1;
    break;
  case 4:
    sbrRatioIndex = USAC_SBR_RATIO_INDEX_4_1;
    break;
  default:
    break;
  }  

  return sbrRatioIndex;
}

USAC_OUT_FRAMELENGTH UsacConfig_GetOutputFrameLength(unsigned int coreSbrFrameLengthIndex){

  USAC_OUT_FRAMELENGTH outputFrameLength = USAC_OUT_FRAMELENGTH_INVALID;

  switch(coreSbrFrameLengthIndex){
  case 0:
    outputFrameLength = USAC_OUT_FRAMELENGTH_768;
    break;
  case 1:
    outputFrameLength = USAC_OUT_FRAMELENGTH_1024;
    break;
  case 2:
  case 3:
    outputFrameLength = USAC_OUT_FRAMELENGTH_2048;
    break;
  case 4:
    outputFrameLength = USAC_OUT_FRAMELENGTH_4096;
    break;
  default:
    break;
  }  

  return outputFrameLength;
}


int UsacConfig_GetMps212NumSlots(unsigned int coreSbrFrameLengthIndex){

  int mps212NumSlots = -1;

  switch(coreSbrFrameLengthIndex){
  case 0:
  case 1:
    break;
  case 2:
  case 3:
    mps212NumSlots = 32;
    break;
  case 4:
    mps212NumSlots = 64;
    break;
  default:
    break;
  }  

  return mps212NumSlots;
}


static void initUsacChannelConfig(USAC_CHANNEL_CONFIG *pUsacChannelConfig, int WriteFlag){

  if(WriteFlag == 0) {
    int i;

    pUsacChannelConfig->numOutChannels = 0;
  
    for(i=0; i<USAC_MAX_NUM_OUT_CHANNELS; i++){
      pUsacChannelConfig->outputChannelPos[i] = USAC_OUTPUT_CHANNEL_POS_NA;
    }
  }

  return;
}

static void initUsacCoreConfig(USAC_CORE_CONFIG *pUsacCoreConfig){

  pUsacCoreConfig->tw_mdct.length      = 1;
  pUsacCoreConfig->noiseFilling.length = 1;

  return;
}

static void initUsacSbrHeader(USAC_SBR_HEADER *pUsacSbrHeader){
  
  pUsacSbrHeader->start_freq.length     = 4;
  pUsacSbrHeader->stop_freq.length      = 4;
  pUsacSbrHeader->header_extra1.length  = 1;
  pUsacSbrHeader->header_extra2.length  = 1;
  pUsacSbrHeader->freq_scale.length     = 2;
  pUsacSbrHeader->alter_scale.length    = 1;
  pUsacSbrHeader->noise_bands.length    = 2;
  pUsacSbrHeader->limiter_bands.length  = 2;
  pUsacSbrHeader->limiter_gains.length  = 2;
  pUsacSbrHeader->interpol_freq.length  = 1;
  pUsacSbrHeader->smoothing_mode.length = 1;

  return;
}

static void initUsacSbrConfig(USAC_SBR_CONFIG *pUsacSbrConfig){

  pUsacSbrConfig->harmonicSBR.length = 1;
  pUsacSbrConfig->bs_interTes.length = 1;
  pUsacSbrConfig->bs_pvc.length      = 1;
  initUsacSbrHeader(&(pUsacSbrConfig->sbrDfltHeader));

  return;
}

static void initUsacMps212Config(USAC_MPS212_CONFIG *pUsacMps212Config){

  pUsacMps212Config->bsFreqRes.length              = 3;
  pUsacMps212Config->bsFixedGainDMX.length         = 3;
  pUsacMps212Config->bsTempShapeConfig.length      = 2;
  pUsacMps212Config->bsDecorrConfig.length         = 2;
  pUsacMps212Config->bsHighRateMode.length         = 1;
  pUsacMps212Config->bsPhaseCoding.length          = 1;
  pUsacMps212Config->bsOttBandsPhasePresent.length = 1;
  pUsacMps212Config->bsOttBandsPhase.length        = 5;
  pUsacMps212Config->bsResidualBands.length        = 5;
  pUsacMps212Config->bsPseudoLr.length             = 1;
  pUsacMps212Config->bsEnvQuantMode.length         = 1;

  return;
}

static void initUsacSceConfig(USAC_SCE_CONFIG *pUsacSceConfig){
  
  initUsacCoreConfig(&(pUsacSceConfig->usacCoreConfig));
  initUsacSbrConfig(&(pUsacSceConfig->usacSbrConfig));
  
  return;
}

static void initUsacCpeConfig(USAC_CPE_CONFIG *pUsacCpeConfig){
  
  initUsacCoreConfig(&(pUsacCpeConfig->usacCoreConfig));
  initUsacSbrConfig(&(pUsacCpeConfig->usacSbrConfig));
  pUsacCpeConfig->stereoConfigIndex.length = 2;
  initUsacMps212Config(&(pUsacCpeConfig->usacMps212Config));
  
  return;
}

static void initUsacLfeConfig(USAC_LFE_CONFIG *pUsacLfeConfig){
  
  initUsacCoreConfig(&(pUsacLfeConfig->usacCoreConfig));
  
  return;
}

static void initUsacExtConfig(USAC_EXT_CONFIG *pUsacExtConfig){
  
  pUsacExtConfig->usacExtElementDefaultLengthPresent.length = 1;
  pUsacExtConfig->usacExtElementPayloadFrag.length          = 1;
  
  return;
}

static void initUsacElementConfig(USAC_ELEMENT_CONFIG *pUsacElementConfig){

  initUsacSceConfig(&(pUsacElementConfig->usacSceConfig));
  initUsacCpeConfig(&(pUsacElementConfig->usacCpeConfig));
  initUsacLfeConfig(&(pUsacElementConfig->usacLfeConfig));
  initUsacExtConfig(&(pUsacElementConfig->usacExtConfig));
  
  return;
}

static void initUsacDecoderConfig(USAC_DECODER_CONFIG *pUsacDecoderConfig, int WriteFlag){
  int i;

  if(WriteFlag == 0) pUsacDecoderConfig->numElements = 0;

  for(i=0; i<USAC_MAX_ELEMENTS; i++){
    if(WriteFlag == 0) pUsacDecoderConfig->usacElementType[i] = USAC_ELEMENT_TYPE_INVALID;
    initUsacElementConfig(&(pUsacDecoderConfig->usacElementConfig[i]));
  }

  return;
}


static void initUsacConfigExtension(USAC_CONFIG_EXTENSION *pUsacConfigExtension){

  /* nothing to do for now ... */

  return;
}

void UsacConfig_Init( USAC_CONFIG *usacConf, int WriteFlag )
{

  usacConf->usacSamplingFrequencyIndex.length =  5;
  usacConf->usacSamplingFrequency.length      = 24;
  usacConf->coreSbrFrameLengthIndex.length    =  3;
  usacConf->frameLength.length                =  1;
  usacConf->channelConfigurationIndex.length  =  5;
  
  initUsacChannelConfig(&(usacConf->usacChannelConfig), WriteFlag);

  initUsacDecoderConfig(&(usacConf->usacDecoderConfig), WriteFlag);

  usacConf->usacConfigExtensionPresent.length = 1;
  initUsacConfigExtension(&(usacConf->usacConfigExtension));

  return;
}

const long UsacSamplingFrequencyTable[32] =
  {
    96000,                 /* 0x00 */
    88200,                 /* 0x01 */
    64000,                 /* 0x02 */
    48000,                 /* 0x03 */
    44100,                 /* 0x04 */
    32000,                 /* 0x05 */
    24000,                 /* 0x06 */
    22050,                 /* 0x07 */
    16000,                 /* 0x08 */
    12000,                 /* 0x09 */
    11025,                 /* 0x0a */
    8000,                  /* 0x0b */
    7350,                  /* 0x0c */
    -1 /* reserved */,     /* 0x0d */
    -1 /* reserved */,     /* 0x0e */
    57600,                 /* 0x0f */
    51200,                 /* 0x10 */
    40000,                 /* 0x11 */
    38400,                 /* 0x12 */
    34150,                 /* 0x13 */
    28800,                 /* 0x14 */
    25600,                 /* 0x15 */
    20000,                 /* 0x16 */
    19200,                 /* 0x17 */
    17075,                 /* 0x18 */
    14400,                 /* 0x19 */
    12800,                 /* 0x1a */
    9600,                  /* 0x1b */
    -1 /* reserved */,     /* 0x1c */
    -1 /* reserved */,     /* 0x1d */
    -1 /* reserved */,     /* 0x1e */
     0 /* escape value */, /* 0x1f */
  };

int UsacConfig_ReadEscapedValue(HANDLE_BSBITSTREAM bitStream, unsigned int *pValue, unsigned int nBits1, unsigned int nBits2, unsigned int nBits3){

  int bitCount = 0;
  unsigned long value = 0;
  unsigned long valueAdd = 0;
  unsigned long maxValue1 = (1 << nBits1) - 1;
  unsigned long maxValue2 = (1 << nBits2) - 1;

  bitCount+=BsRWBitWrapper(bitStream, &value, nBits1, 0);
  if(value == maxValue1){
    bitCount+=BsRWBitWrapper(bitStream, &valueAdd, nBits2, 0);
    value += valueAdd;

    if(valueAdd == maxValue2){
      bitCount+=BsRWBitWrapper(bitStream, &valueAdd, nBits3, 0);
      value += valueAdd;
    }
  }

  *pValue = value;

  return bitCount;
}

static int writeEscapedValue(HANDLE_BSBITSTREAM bitStream, unsigned int value, unsigned int nBits1, unsigned int nBits2, unsigned int nBits3, unsigned int WriteFlag){

  int bitCount = 0;
  unsigned long valueLeft  = value;
  unsigned long valueWrite = 0;
  unsigned long maxValue1 = (1 << nBits1) - 1;
  unsigned long maxValue2 = (1 << nBits2) - 1;
  unsigned long maxValue3 = (1 << nBits3) - 1;

  valueWrite = min(valueLeft, maxValue1);
  bitCount+=BsRWBitWrapper(bitStream, &valueWrite, nBits1, WriteFlag);

  if(valueWrite == maxValue1){
    valueLeft = valueLeft - valueWrite;

    valueWrite = min(valueLeft, maxValue2);
    bitCount+=BsRWBitWrapper(bitStream, &valueWrite, nBits2, WriteFlag);

    if(valueWrite == maxValue2){
      valueLeft = valueLeft - valueWrite;

      valueWrite = min(valueLeft, maxValue3);
      bitCount+=BsRWBitWrapper(bitStream, &valueWrite, nBits3, WriteFlag);
      assert( (valueLeft - valueWrite) == 0 );
    }
  }

  return bitCount;
}

static int advanceUsacChannelConfig(HANDLE_BSBITSTREAM bitStream, USAC_CHANNEL_CONFIG *pUsacChannelConfig, int WriteFlag){
  int bitCount = 0;
  
  if(pUsacChannelConfig){
    unsigned int i;

    if(WriteFlag){
      bitCount += writeEscapedValue(bitStream, pUsacChannelConfig->numOutChannels, 5, 8, 16, WriteFlag);
    } else {
      bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacChannelConfig->numOutChannels), 5, 8, 16);
    }

    for(i=0; i<pUsacChannelConfig->numOutChannels; i++){
      unsigned long tmp = pUsacChannelConfig->outputChannelPos[i];
      bitCount+=BsRWBitWrapper(bitStream, &tmp, 5, WriteFlag);
      if(!WriteFlag) pUsacChannelConfig->outputChannelPos[i] = (USAC_OUTPUT_CHANNEL_POS) tmp;
      ObjDescPrintf(WriteFlag, "   audioSpC->usac.usacChannelConfig.outputChannelPos[%d]   : %ld", i, pUsacChannelConfig->outputChannelPos[i]);    
    }
  }

  return bitCount;
}

static int getUsacChannelConfigFromIndex(USAC_CHANNEL_CONFIG *pUsacChannelConfig, unsigned int channelConfigurationIndex){
  int nChannels = -1;

  if(pUsacChannelConfig){
    switch(channelConfigurationIndex){
    case 1:
      pUsacChannelConfig->numOutChannels = 1;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      break;
    case 2:
      pUsacChannelConfig->numOutChannels = 2;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_R;
      break;
    case 3:
      pUsacChannelConfig->numOutChannels = 3;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      break;
    case 4:
      pUsacChannelConfig->numOutChannels = 4;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_CS;
      break;
    case 5:
      pUsacChannelConfig->numOutChannels = 5;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[4] = USAC_OUTPUT_CHANNEL_POS_RS;
      break;
    case 6:
      pUsacChannelConfig->numOutChannels = 6;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[4] = USAC_OUTPUT_CHANNEL_POS_RS;
      pUsacChannelConfig->outputChannelPos[5] = USAC_OUTPUT_CHANNEL_POS_LFE;
      break;
    case 7:
      pUsacChannelConfig->numOutChannels = 8;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_LC;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_RC;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[4] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[5] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[6] = USAC_OUTPUT_CHANNEL_POS_RS;
      pUsacChannelConfig->outputChannelPos[7] = USAC_OUTPUT_CHANNEL_POS_LFE;
      break;
    case 8:
      pUsacChannelConfig->numOutChannels = 2;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_NA;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_NA;
      break;
    case 9:
      pUsacChannelConfig->numOutChannels = 3;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_CS;
      break;
    case 10:
      pUsacChannelConfig->numOutChannels = 4;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_RS;
      break;
    case 11:
      pUsacChannelConfig->numOutChannels = 7;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[4] = USAC_OUTPUT_CHANNEL_POS_RS;
      pUsacChannelConfig->outputChannelPos[5] = USAC_OUTPUT_CHANNEL_POS_CS;
      pUsacChannelConfig->outputChannelPos[6] = USAC_OUTPUT_CHANNEL_POS_LFE;
      break;
    case 12:
      pUsacChannelConfig->numOutChannels = 8;
      pUsacChannelConfig->outputChannelPos[0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[1] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[2] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[3] = USAC_OUTPUT_CHANNEL_POS_LS;
      pUsacChannelConfig->outputChannelPos[4] = USAC_OUTPUT_CHANNEL_POS_RS;
      pUsacChannelConfig->outputChannelPos[5] = USAC_OUTPUT_CHANNEL_POS_LSR;
      pUsacChannelConfig->outputChannelPos[6] = USAC_OUTPUT_CHANNEL_POS_RSR;
      pUsacChannelConfig->outputChannelPos[7] = USAC_OUTPUT_CHANNEL_POS_LFE;
      break;
    case 13:
      pUsacChannelConfig->numOutChannels = 24;
      pUsacChannelConfig->outputChannelPos[ 0] = USAC_OUTPUT_CHANNEL_POS_C;
      pUsacChannelConfig->outputChannelPos[ 1] = USAC_OUTPUT_CHANNEL_POS_LC;
      pUsacChannelConfig->outputChannelPos[ 2] = USAC_OUTPUT_CHANNEL_POS_RC;
      pUsacChannelConfig->outputChannelPos[ 3] = USAC_OUTPUT_CHANNEL_POS_L;
      pUsacChannelConfig->outputChannelPos[ 4] = USAC_OUTPUT_CHANNEL_POS_R;
      pUsacChannelConfig->outputChannelPos[ 5] = USAC_OUTPUT_CHANNEL_POS_LSS;
      pUsacChannelConfig->outputChannelPos[ 6] = USAC_OUTPUT_CHANNEL_POS_RSS;
      pUsacChannelConfig->outputChannelPos[ 7] = USAC_OUTPUT_CHANNEL_POS_LSR;
      pUsacChannelConfig->outputChannelPos[ 8] = USAC_OUTPUT_CHANNEL_POS_RSR;
      pUsacChannelConfig->outputChannelPos[ 9] = USAC_OUTPUT_CHANNEL_POS_CS;
      pUsacChannelConfig->outputChannelPos[10] = USAC_OUTPUT_CHANNEL_POS_LFE;
      pUsacChannelConfig->outputChannelPos[11] = USAC_OUTPUT_CHANNEL_POS_LFE2;
      pUsacChannelConfig->outputChannelPos[12] = USAC_OUTPUT_CHANNEL_POS_CV;
      pUsacChannelConfig->outputChannelPos[13] = USAC_OUTPUT_CHANNEL_POS_LV;
      pUsacChannelConfig->outputChannelPos[14] = USAC_OUTPUT_CHANNEL_POS_RV;
      pUsacChannelConfig->outputChannelPos[15] = USAC_OUTPUT_CHANNEL_POS_LVSS;
      pUsacChannelConfig->outputChannelPos[16] = USAC_OUTPUT_CHANNEL_POS_RVSS;
      pUsacChannelConfig->outputChannelPos[17] = USAC_OUTPUT_CHANNEL_POS_TS;
      pUsacChannelConfig->outputChannelPos[18] = USAC_OUTPUT_CHANNEL_POS_LVR;
      pUsacChannelConfig->outputChannelPos[19] = USAC_OUTPUT_CHANNEL_POS_RVR;
      pUsacChannelConfig->outputChannelPos[20] = USAC_OUTPUT_CHANNEL_POS_CVR;
      pUsacChannelConfig->outputChannelPos[21] = USAC_OUTPUT_CHANNEL_POS_CB;
      pUsacChannelConfig->outputChannelPos[22] = USAC_OUTPUT_CHANNEL_POS_LB;
      pUsacChannelConfig->outputChannelPos[23] = USAC_OUTPUT_CHANNEL_POS_RB;
      break;
    default:
      assert(0);
      break;
    }
  }

  return nChannels;
}

static int advanceSbrHeader(HANDLE_BSBITSTREAM bitStream, USAC_SBR_HEADER *pSbrHeader, int WriteFlag){

  int bitCount = 0;

  /* start_freq */
  bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->start_freq.value), pSbrHeader->start_freq.length, WriteFlag);

  /* stop_freq */
  bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->stop_freq.value), pSbrHeader->stop_freq.length, WriteFlag);

  /* header_extra1 */
  bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->header_extra1.value), pSbrHeader->header_extra1.length, WriteFlag);

  /* header_extra2 */
  bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->header_extra2.value), pSbrHeader->header_extra2.length, WriteFlag);

  if(pSbrHeader->header_extra1.value){
    
    /* freq_scale */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->freq_scale.value), pSbrHeader->freq_scale.length, WriteFlag);

    /* alter_scale */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->alter_scale.value), pSbrHeader->alter_scale.length, WriteFlag);

    /* noise_bands */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->noise_bands.value), pSbrHeader->noise_bands.length, WriteFlag);
  }

  if(pSbrHeader->header_extra2.value){

    /* limiter_bands */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->limiter_bands.value), pSbrHeader->limiter_bands.length, WriteFlag);

    /* limiter_gains */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->limiter_gains.value), pSbrHeader->limiter_gains.length, WriteFlag);

    /* interpol_freq */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->interpol_freq.value), pSbrHeader->interpol_freq.length, WriteFlag);

    /* smoothing_mode */
    bitCount += BsRWBitWrapper(bitStream, &(pSbrHeader->smoothing_mode.value), pSbrHeader->smoothing_mode.length, WriteFlag);
  }

  return bitCount;
}

static int advanceUsacSbrConfig(HANDLE_BSBITSTREAM bitStream, USAC_SBR_CONFIG *pUsacSbrConfig, int WriteFlag){

  int bitCount = 0;

  /* harmonicSBR */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacSbrConfig->harmonicSBR.value), pUsacSbrConfig->harmonicSBR.length, WriteFlag);

  /* bs_interTes */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacSbrConfig->bs_interTes.value), pUsacSbrConfig->bs_interTes.length, WriteFlag);

  /* bs_pvc */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacSbrConfig->bs_pvc.value), pUsacSbrConfig->bs_pvc.length, WriteFlag);

  /* SbrDfltHeader */
  bitCount += advanceSbrHeader(bitStream, &(pUsacSbrConfig->sbrDfltHeader), WriteFlag);

  return bitCount;
}

static int advanceUsacCoreConfig(HANDLE_BSBITSTREAM bitStream, USAC_CORE_CONFIG *pUsacCoreConfig, int WriteFlag){

  int bitCount = 0;
  
  /* tw_mdct */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacCoreConfig->tw_mdct.value), pUsacCoreConfig->tw_mdct.length, WriteFlag);
  
  /* noiseFilling */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacCoreConfig->noiseFilling.value), pUsacCoreConfig->noiseFilling.length, WriteFlag);

  return bitCount;
}

static int advanceUsacSceConfig(HANDLE_BSBITSTREAM bitStream, USAC_SCE_CONFIG *pUsacSceConfig, int sbrRatioIndex, int WriteFlag){

  int bitCount = 0;

  bitCount += advanceUsacCoreConfig(bitStream, &(pUsacSceConfig->usacCoreConfig), WriteFlag);
  
  if(sbrRatioIndex > 0){
    bitCount += advanceUsacSbrConfig(bitStream, &(pUsacSceConfig->usacSbrConfig), WriteFlag);
  }

  return bitCount;
}

static int advanceUsacLfeConfig(HANDLE_BSBITSTREAM bitStream, USAC_LFE_CONFIG *pUsacLfeConfig, int WriteFlag){

  int bitCount = 0;

  if(!WriteFlag){
    pUsacLfeConfig->usacCoreConfig.tw_mdct.value = 0;
    pUsacLfeConfig->usacCoreConfig.noiseFilling.value = 0;
  }

  return bitCount;
}

static int advanceUsacExtConfig(HANDLE_BSBITSTREAM bitStream, USAC_EXT_CONFIG *pUsacExtConfig, int WriteFlag){

  int bitCount = 0;
  unsigned int i, nBytesPayloadRead = 0;
  unsigned int dummy = 0;
  unsigned long tmpReadPayload = 0;
  DESCR_ELE tmp;
  tmp.length = 8;

  /* usacExtElementType */
  if(WriteFlag){
    bitCount += writeEscapedValue(bitStream, pUsacExtConfig->usacExtElementType, 4, 8, 16, WriteFlag);
  } else {
    bitCount += UsacConfig_ReadEscapedValue(bitStream, &dummy, 4, 8, 16);
    pUsacExtConfig->usacExtElementType = (USAC_ID_EXT_ELE)dummy;
  }

  /* usacExtElementConfigLength */
  if(WriteFlag){
    bitCount += writeEscapedValue(bitStream, pUsacExtConfig->usacExtElementConfigLength, 4, 8, 16, WriteFlag);
  } else {
    bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacExtConfig->usacExtElementConfigLength), 4, 8, 16);
  }

  /* usacExtElementDefaultLengthPresent */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacExtConfig->usacExtElementDefaultLengthPresent.value), pUsacExtConfig->usacExtElementDefaultLengthPresent.length, WriteFlag);

  if(pUsacExtConfig->usacExtElementDefaultLengthPresent.value){
    if(WriteFlag){
      bitCount += writeEscapedValue(bitStream, pUsacExtConfig->usacExtElementDefaultLength-1, 8, 16, 0, WriteFlag);
    } else {
      bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacExtConfig->usacExtElementDefaultLength), 8, 16, 0);
      pUsacExtConfig->usacExtElementDefaultLength += 1;
    }
  } else {
    if(!WriteFlag) pUsacExtConfig->usacExtElementDefaultLength = 0;
  }
    
  /* usacExtElementDefaultLengthPresent */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacExtConfig->usacExtElementPayloadFrag.value), pUsacExtConfig->usacExtElementPayloadFrag.length, WriteFlag);

  switch(pUsacExtConfig->usacExtElementType){
  case USAC_ID_EXT_ELE_FILL:
  case USAC_ID_EXT_ELE_AUDIOPREROLL:
    /* no configuration payload */
    break;
  case USAC_ID_EXT_ELE_UNI_DRC:
    for (i = 0; i < pUsacExtConfig->usacExtElementConfigLength; i++) {
      if (!WriteFlag){
        bitCount += BsRWBitWrapper(bitStream, &tmpReadPayload, 8, WriteFlag);
        pUsacExtConfig->usacExtElementConfigPayload[i] = (unsigned char)tmpReadPayload;
      } else {
        unsigned long val = pUsacExtConfig->usacExtElementConfigPayload[i];
        bitCount += BsRWBitWrapper(bitStream, &val, 8, WriteFlag);
      }
    }
    nBytesPayloadRead = i;
    break;
  default:
    for (i = 0; i < pUsacExtConfig->usacExtElementConfigLength; i++) {
      if (!WriteFlag){
        bitCount += BsRWBitWrapper(bitStream, &tmpReadPayload, 8, WriteFlag);
        pUsacExtConfig->usacExtElementConfigPayload[i] = (unsigned char)tmpReadPayload;
      } else {
        /* writing not implemented */
        assert(0);
      }
    }
    nBytesPayloadRead = i;
    break;
  }

  assert(nBytesPayloadRead == pUsacExtConfig->usacExtElementConfigLength);

  return bitCount;
}

static int advanceUsacMps212Config(HANDLE_BSBITSTREAM bitStream, USAC_MPS212_CONFIG *pUsacMps212Config, int stereoConfigIndex, int WriteFlag){

  int bitCount = 0;
  int bsResidualCoding = (stereoConfigIndex > 1)?1:0;

  /* bsFreqRes */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsFreqRes.value), pUsacMps212Config->bsFreqRes.length, WriteFlag);
  
  /* bsFixedGainDMX */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsFixedGainDMX.value), pUsacMps212Config->bsFixedGainDMX.length, WriteFlag);

  /* bsTempShapeConfig */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsTempShapeConfig.value), pUsacMps212Config->bsTempShapeConfig.length, WriteFlag);

  /* bsDecorrConfig */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsDecorrConfig.value), pUsacMps212Config->bsDecorrConfig.length, WriteFlag);

  /* bsHighRateMode */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsHighRateMode.value), pUsacMps212Config->bsHighRateMode.length, WriteFlag);

  /* bsPhaseCoding */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsPhaseCoding.value), pUsacMps212Config->bsPhaseCoding.length, WriteFlag);

  /* bsOttBandsPhasePresent */
  bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsOttBandsPhasePresent.value), pUsacMps212Config->bsOttBandsPhasePresent.length, WriteFlag);

  if(pUsacMps212Config->bsOttBandsPhasePresent.value){
    /* bsOttBandsPhase */
    bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsOttBandsPhase.value), pUsacMps212Config->bsOttBandsPhase.length, WriteFlag);
  }

  if(bsResidualCoding){
    /* bsResidualBands */
    bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsResidualBands.value), pUsacMps212Config->bsResidualBands.length, WriteFlag);
    if(!WriteFlag) pUsacMps212Config->bsOttBandsPhase.value = max(pUsacMps212Config->bsOttBandsPhase.value, pUsacMps212Config->bsResidualBands.value);

    /* bsPseudoLr */
    bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsPseudoLr.value), pUsacMps212Config->bsPseudoLr.length, WriteFlag);
  }

  if(pUsacMps212Config->bsTempShapeConfig.value == 2){
    /* bsResidualBands */
    bitCount += BsRWBitWrapper(bitStream, &(pUsacMps212Config->bsEnvQuantMode.value), pUsacMps212Config->bsEnvQuantMode.length, WriteFlag);
  }

  return bitCount;
}

static int advanceUsacCpeConfig(HANDLE_BSBITSTREAM bitStream, USAC_CPE_CONFIG *pUsacCpeConfig, int sbrRatioIndex, int WriteFlag){

  int bitCount = 0;

  bitCount += advanceUsacCoreConfig(bitStream, &(pUsacCpeConfig->usacCoreConfig), WriteFlag);
  
  if(sbrRatioIndex > 0){
    bitCount += advanceUsacSbrConfig(bitStream, &(pUsacCpeConfig->usacSbrConfig), WriteFlag);

    /* stereoConfigIndex */
    bitCount += BsRWBitWrapper(bitStream, &(pUsacCpeConfig->stereoConfigIndex.value), pUsacCpeConfig->stereoConfigIndex.length, WriteFlag);
  } else {
    if(!WriteFlag) pUsacCpeConfig->stereoConfigIndex.value = 0;
  }

  if(pUsacCpeConfig->stereoConfigIndex.value > 0){
    bitCount += advanceUsacMps212Config(bitStream, &(pUsacCpeConfig->usacMps212Config), pUsacCpeConfig->stereoConfigIndex.value, WriteFlag);
  }

  return bitCount;
}

static int advanceUsacDecoderConfig(HANDLE_BSBITSTREAM bitStream, USAC_DECODER_CONFIG *pUsacDecoderConfig, int sbrRatioIndex, int WriteFlag){

  int bitCount = 0;
  unsigned int elemIdx = 0;

  if(WriteFlag){
    bitCount += writeEscapedValue(bitStream, pUsacDecoderConfig->numElements-1, 4, 8, 16, WriteFlag);
  } else {
    bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacDecoderConfig->numElements), 4, 8, 16);
    pUsacDecoderConfig->numElements += 1;
  }

  for(elemIdx = 0; elemIdx < pUsacDecoderConfig->numElements; elemIdx++){
    unsigned long tmp = pUsacDecoderConfig->usacElementType[elemIdx];
    bitCount += BsRWBitWrapper(bitStream, &tmp, 2, WriteFlag);
    if(!WriteFlag) pUsacDecoderConfig->usacElementType[elemIdx] = (USAC_ELEMENT_TYPE) tmp;

    switch(pUsacDecoderConfig->usacElementType[elemIdx]){
    case USAC_ELEMENT_TYPE_SCE:
      bitCount += advanceUsacSceConfig(bitStream, &(pUsacDecoderConfig->usacElementConfig[elemIdx].usacSceConfig), sbrRatioIndex, WriteFlag);
      break;
    case USAC_ELEMENT_TYPE_CPE:
      bitCount += advanceUsacCpeConfig(bitStream, &(pUsacDecoderConfig->usacElementConfig[elemIdx].usacCpeConfig), sbrRatioIndex, WriteFlag);
      break;
    case USAC_ELEMENT_TYPE_LFE:
      bitCount += advanceUsacLfeConfig(bitStream, &(pUsacDecoderConfig->usacElementConfig[elemIdx].usacLfeConfig), WriteFlag);
      break;
    case USAC_ELEMENT_TYPE_EXT:
      bitCount += advanceUsacExtConfig(bitStream, &(pUsacDecoderConfig->usacElementConfig[elemIdx].usacExtConfig), WriteFlag);
      break;
    default:
      assert(0);
      break;
    }
  }

  return bitCount;
}

static int advanceUsacConfigExtension(HANDLE_BSBITSTREAM bitStream, USAC_CONFIG_EXTENSION *pUsacConfigExtension, int WriteFlag){
  int bitCount = 0;
  unsigned int confExtIdx;
  unsigned int dummy;
  unsigned long dummy_long;
  unsigned int i;

  if (WriteFlag) {
    bitCount += writeEscapedValue(bitStream, pUsacConfigExtension->numConfigExtensions-1, 2, 4, 8, WriteFlag);
  } else {
    bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacConfigExtension->numConfigExtensions), 2, 4, 8);
    pUsacConfigExtension->numConfigExtensions += 1;
  }  

  for (confExtIdx = 0; confExtIdx < pUsacConfigExtension->numConfigExtensions; confExtIdx++) {
    unsigned long fillByte = 0xa5;
    /* usacConfigExtType */
    if (WriteFlag) {
      bitCount += writeEscapedValue(bitStream, pUsacConfigExtension->usacConfigExtType[confExtIdx], 4, 8, 16, WriteFlag);
    } else {
      bitCount += UsacConfig_ReadEscapedValue(bitStream, &dummy, 4, 8, 16);
      pUsacConfigExtension->usacConfigExtType[confExtIdx] = (USAC_CONFIG_EXT_TYPE)dummy;
    }  

    /* usacConfigExtLength */
    if (WriteFlag) {
      bitCount += writeEscapedValue(bitStream, pUsacConfigExtension->usacConfigExtLength[confExtIdx], 4, 8, 16, WriteFlag);
    } else {
      bitCount += UsacConfig_ReadEscapedValue(bitStream, &(pUsacConfigExtension->usacConfigExtLength[confExtIdx]), 4, 8, 16);
    }  

    switch (pUsacConfigExtension->usacConfigExtType[confExtIdx]) {
    case USAC_CONFIG_EXT_TYPE_FILL:
      for (i = 0; i < pUsacConfigExtension->usacConfigExtLength[confExtIdx]; i++) {
        bitCount += BsRWBitWrapper(bitStream, &fillByte, 8, WriteFlag);

        if (!WriteFlag) {
          assert(fillByte == 0xa5);
        }
      }
      break;

    default:
      if (!WriteFlag) {
        for (i = 0; i < pUsacConfigExtension->usacConfigExtLength[confExtIdx]; i++) {
          bitCount += BsRWBitWrapper(bitStream, &dummy_long, 8, WriteFlag);
          pUsacConfigExtension->usacConfigExt[confExtIdx][i] = (unsigned char)dummy_long;
        }
      } else {
        for (i = 0; i < pUsacConfigExtension->usacConfigExtLength[confExtIdx]; i++) {
          dummy_long = (unsigned long)pUsacConfigExtension->usacConfigExt[confExtIdx][i];
          bitCount += BsRWBitWrapper(bitStream, &dummy_long, 8, WriteFlag);
        }
      }
      break;
    }
  }

  return bitCount;
}


int UsacConfig_Advance(HANDLE_BSBITSTREAM bitStream, USAC_CONFIG *usacConf, int WriteFlag)
{
  int bitCount = 0;

  /*****************************************************************************************************************************************/
  /*                                                                                                                                       */
  /* General information, relevant for playout etc.                                                                                        */
  /*                                                                                                                                       */ 

  /* sampling rate */
  bitCount+=BsRWBitWrapper(bitStream, &(usacConf->usacSamplingFrequencyIndex.value), usacConf->usacSamplingFrequencyIndex.length, WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->usac.usacSamplingFrequencyIndex   : %ld", usacConf->usacSamplingFrequencyIndex.value);

  if(usacConf->usacSamplingFrequencyIndex.value == 0x1f){
    bitCount+=BsRWBitWrapper(bitStream, &(usacConf->usacSamplingFrequency.value), usacConf->usacSamplingFrequency.length, WriteFlag);
  } else {
    if(!WriteFlag){
      usacConf->usacSamplingFrequency.value = UsacSamplingFrequencyTable[usacConf->usacSamplingFrequencyIndex.value];
    }
  }

  /* frame length, SBR on/off */
  bitCount+=BsRWBitWrapper(bitStream, &(usacConf->coreSbrFrameLengthIndex.value), usacConf->coreSbrFrameLengthIndex.length, WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->usac.coreSbrFrameLengthIndex   : %ld", usacConf->coreSbrFrameLengthIndex.value);
  if(!WriteFlag) usacConf->frameLength.value = 0; 

  /* channel configuration */
  bitCount+=BsRWBitWrapper(bitStream, &(usacConf->channelConfigurationIndex.value), usacConf->channelConfigurationIndex.length, WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->usac.channelConfigurationIndex   : %ld", usacConf->channelConfigurationIndex.value);

  if(usacConf->channelConfigurationIndex.value == 0){
    bitCount += advanceUsacChannelConfig(bitStream, &(usacConf->usacChannelConfig), WriteFlag);
  } else {
    if(!WriteFlag) getUsacChannelConfigFromIndex(&(usacConf->usacChannelConfig), usacConf->channelConfigurationIndex.value);
  }

  /*****************************************************************************************************************************************/
  /*                                                                                                                                       */
  /* USAC decoder setup                                                                                                                    */
  /*                                                                                                                                       */ 

  bitCount += advanceUsacDecoderConfig(bitStream, &(usacConf->usacDecoderConfig), UsacConfig_GetSbrRatioIndex(usacConf->coreSbrFrameLengthIndex.value), WriteFlag);

  /*****************************************************************************************************************************************/
  /*                                                                                                                                       */
  /* Extension to config                                                                                                                   */
  /*                                                                                                                                       */ 

  bitCount += BsRWBitWrapper(bitStream, &(usacConf->usacConfigExtensionPresent.value), usacConf->usacConfigExtensionPresent.length,WriteFlag);
  if(usacConf->usacConfigExtensionPresent.value){
    bitCount += advanceUsacConfigExtension(bitStream, &(usacConf->usacConfigExtension), WriteFlag);
  }

  return bitCount;
}

USAC_ELEMENT_TYPE UsacConfig_GetUsacElementType(USAC_CONFIG *pUsacConfig, unsigned int elemIdx){

  USAC_ELEMENT_TYPE usacElementType = USAC_ELEMENT_TYPE_INVALID;

  if(pUsacConfig){
    if(elemIdx < pUsacConfig->usacDecoderConfig.numElements){
      usacElementType = pUsacConfig->usacDecoderConfig.usacElementType[elemIdx];
    }
  }

  return usacElementType;
}


USAC_CORE_CONFIG * UsacConfig_GetUsacCoreConfig(USAC_CONFIG *pUsacConfig, unsigned int elemIdx){

  USAC_CORE_CONFIG *pUsacCoreConfig = NULL;

  if(pUsacConfig){
    if(elemIdx < pUsacConfig->usacDecoderConfig.numElements){
      USAC_ELEMENT_TYPE elemType = UsacConfig_GetUsacElementType(pUsacConfig, elemIdx);

      switch(elemType){
      case USAC_ELEMENT_TYPE_SCE:
        pUsacCoreConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacSceConfig.usacCoreConfig);
        break;
      case USAC_ELEMENT_TYPE_CPE:
        pUsacCoreConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig.usacCoreConfig);
        break;
      case USAC_ELEMENT_TYPE_LFE:
        pUsacCoreConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacLfeConfig.usacCoreConfig);
        break;
      default:
        break;
      }
    }
  }

  return pUsacCoreConfig;
}


USAC_SBR_CONFIG * UsacConfig_GetUsacSbrConfig(USAC_CONFIG *pUsacConfig, unsigned int elemIdx){

  USAC_SBR_CONFIG *pUsacSbrConfig = NULL;

  if(pUsacConfig){
    if(elemIdx < pUsacConfig->usacDecoderConfig.numElements){
      USAC_ELEMENT_TYPE elemType = UsacConfig_GetUsacElementType(pUsacConfig, elemIdx);

      switch(elemType){
      case USAC_ELEMENT_TYPE_SCE:
        pUsacSbrConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacSceConfig.usacSbrConfig);
        break;
      case USAC_ELEMENT_TYPE_CPE:
        pUsacSbrConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig.usacSbrConfig);
        break;
      default:
        break;
      }
    }
  }

  return pUsacSbrConfig;
}


USAC_MPS212_CONFIG * UsacConfig_GetUsacMps212Config(USAC_CONFIG *pUsacConfig, unsigned int elemIdx){

  USAC_MPS212_CONFIG *pUsacMps212Config = NULL;

  if(pUsacConfig){
    if(elemIdx < pUsacConfig->usacDecoderConfig.numElements){
      USAC_ELEMENT_TYPE elemType = UsacConfig_GetUsacElementType(pUsacConfig, elemIdx);

      switch(elemType){
      case USAC_ELEMENT_TYPE_CPE:
        pUsacMps212Config = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig.usacMps212Config);
        break;
      default:
        break;
      }
    }
  }

  return pUsacMps212Config;
}


int UsacConfig_GetStereoConfigIndex(USAC_CONFIG *pUsacConfig, unsigned int elemIdx){

  int stereoConfigIndex = -1;

  if(pUsacConfig){
    if(elemIdx < pUsacConfig->usacDecoderConfig.numElements){
      USAC_ELEMENT_TYPE elemType = UsacConfig_GetUsacElementType(pUsacConfig, elemIdx);

      switch(elemType){
      case USAC_ELEMENT_TYPE_CPE:
        stereoConfigIndex = pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig.stereoConfigIndex.value;
        break;
      default:
        break;
      }
    }
  }

  return stereoConfigIndex;
}
