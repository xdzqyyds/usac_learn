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
$Id: usac_config.h,v 1.12.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/
#ifndef __INCLUDED_USAC_CONFIG_H
#define __INCLUDED_USAC_CONFIG_H

#include "allHandles.h"
#include "obj_descr.h"           /* structs */
#include "common_m4a.h"
#include "usac_channelconf.h"

/* note: restriction of USAC_MAX_ELEMENTS here does not correspond 
   to any profile or max. allowed element number in bitstream      */
#define USAC_MAX_ELEMENTS (16)

/* note: restriction of USAC_MAX_CONFIG_EXTENSIONS here does not correspond 
   to any profile or max. allowed element number in bitstream      */
#define USAC_MAX_CONFIG_EXTENSIONS (16)

typedef enum {
  USAC_ELEMENT_TYPE_INVALID = -1, /* use this for initialization only */
  USAC_ELEMENT_TYPE_SCE     = 0,
  USAC_ELEMENT_TYPE_CPE     = 1,
  USAC_ELEMENT_TYPE_LFE     = 2,
  USAC_ELEMENT_TYPE_EXT     = 3
} USAC_ELEMENT_TYPE;

typedef enum {
  USAC_SBR_RATIO_INDEX_INVALID = -1,
  USAC_SBR_RATIO_INDEX_NO_SBR  = 0,
  USAC_SBR_RATIO_INDEX_4_1     = 1,
  USAC_SBR_RATIO_INDEX_8_3     = 2,
  USAC_SBR_RATIO_INDEX_2_1     = 3
} USAC_SBR_RATIO_INDEX;

typedef enum {
  USAC_OUT_FRAMELENGTH_INVALID = -1,
  USAC_OUT_FRAMELENGTH_768     =  768,
  USAC_OUT_FRAMELENGTH_1024    = 1024,
  USAC_OUT_FRAMELENGTH_2048    = 2048,
  USAC_OUT_FRAMELENGTH_4096    = 4096
} USAC_OUT_FRAMELENGTH;

typedef enum {
  USAC_ID_EXT_ELE_FILL         = 0,
  USAC_ID_EXT_ELE_MPEGS        = 1,
  USAC_ID_EXT_ELE_SAOC         = 2,
  USAC_ID_EXT_ELE_AUDIOPREROLL = 3,
  USAC_ID_EXT_ELE_UNI_DRC      = 4
  /* elements 5 - 127 reserved for ISO use                */
  /* elements 128 and higher reserved for outside ISO use */
} USAC_ID_EXT_ELE;

typedef enum {
  USAC_SYNCFRAME_NOSYNC,
  USAC_SYNCFRAME_INDEPENDENT_FRAME,
  USAC_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME
} USAC_SYNCFRAME_TYPE;

typedef enum {
  USAC_CONFIG_EXT_TYPE_FILL     = 0,
  USAC_CONFIG_EXT_LOUDNESS_INFO = 2
  ,USAC_CONFIG_EXT_STREAM_ID    = 7
  /* elements 1 - 127 reserved for ISO use                */
  /* elements 128 and higher reserved for outside ISO use */
} USAC_CONFIG_EXT_TYPE;

typedef struct { 
  DESCR_ELE tw_mdct;
  DESCR_ELE noiseFilling;
} USAC_CORE_CONFIG;

typedef struct {
  DESCR_ELE start_freq;
  DESCR_ELE stop_freq;
  DESCR_ELE header_extra1;
  DESCR_ELE header_extra2;
  DESCR_ELE freq_scale;
  DESCR_ELE alter_scale;
  DESCR_ELE noise_bands;
  DESCR_ELE limiter_bands;
  DESCR_ELE limiter_gains;
  DESCR_ELE interpol_freq;
  DESCR_ELE smoothing_mode;
} USAC_SBR_HEADER;

typedef struct { 
  DESCR_ELE harmonicSBR;
  DESCR_ELE bs_interTes;
  DESCR_ELE bs_pvc;
  USAC_SBR_HEADER sbrDfltHeader;
} USAC_SBR_CONFIG;

typedef struct {
  DESCR_ELE bsFreqRes;
  DESCR_ELE bsFixedGainDMX;
  DESCR_ELE bsTempShapeConfig;
  DESCR_ELE bsDecorrConfig;
  DESCR_ELE bsHighRateMode;
  DESCR_ELE bsPhaseCoding;
  DESCR_ELE bsOttBandsPhasePresent;
  DESCR_ELE bsOttBandsPhase;
  DESCR_ELE bsResidualBands;
  DESCR_ELE bsPseudoLr;
  DESCR_ELE bsEnvQuantMode;
} USAC_MPS212_CONFIG;

typedef struct {
  USAC_CORE_CONFIG usacCoreConfig;
  USAC_SBR_CONFIG  usacSbrConfig;
} USAC_SCE_CONFIG;

typedef struct {
  USAC_CORE_CONFIG   usacCoreConfig;
  USAC_SBR_CONFIG    usacSbrConfig;
  DESCR_ELE          stereoConfigIndex;
  USAC_MPS212_CONFIG usacMps212Config;
} USAC_CPE_CONFIG;

typedef struct {
  USAC_CORE_CONFIG usacCoreConfig;
} USAC_LFE_CONFIG;

typedef struct {
  USAC_ID_EXT_ELE usacExtElementType;
  unsigned int  usacExtElementConfigLength;
  DESCR_ELE     usacExtElementDefaultLengthPresent;
  unsigned int  usacExtElementDefaultLength;
  DESCR_ELE     usacExtElementPayloadFrag;
  unsigned char usacExtElementConfigPayload[6144/8];
} USAC_EXT_CONFIG;

typedef struct {
  USAC_SCE_CONFIG usacSceConfig;
  USAC_CPE_CONFIG usacCpeConfig;
  USAC_LFE_CONFIG usacLfeConfig;
  USAC_EXT_CONFIG usacExtConfig;
} USAC_ELEMENT_CONFIG;

typedef struct {
  unsigned int        numElements;
  USAC_ELEMENT_TYPE   usacElementType[USAC_MAX_ELEMENTS];
  USAC_ELEMENT_CONFIG usacElementConfig[USAC_MAX_ELEMENTS];
} USAC_DECODER_CONFIG;

typedef struct {
  unsigned int         numConfigExtensions;
  USAC_CONFIG_EXT_TYPE usacConfigExtType[USAC_MAX_CONFIG_EXTENSIONS];
  unsigned int         usacConfigExtLength[USAC_MAX_CONFIG_EXTENSIONS];
  unsigned char        usacConfigExt[USAC_MAX_CONFIG_EXTENSIONS][6144/8];
} USAC_CONFIG_EXTENSION;

typedef struct {
  unsigned int streamID_present;
  unsigned int streamID;
} USAC_STREAM_ID;

typedef struct {
  DESCR_ELE             usacSamplingFrequencyIndex;
  DESCR_ELE             usacSamplingFrequency;
  DESCR_ELE             coreSbrFrameLengthIndex;
  DESCR_ELE             frameLength;
  DESCR_ELE             channelConfigurationIndex;
  USAC_CHANNEL_CONFIG   usacChannelConfig;
  USAC_DECODER_CONFIG   usacDecoderConfig;
  DESCR_ELE             usacConfigExtensionPresent;
  USAC_CONFIG_EXTENSION usacConfigExtension;
  USAC_STREAM_ID        usacStreamId;
} USAC_CONFIG;



void UsacConfig_Init( USAC_CONFIG *usacConf, int WriteFlag );
int UsacConfig_ReadEscapedValue(HANDLE_BSBITSTREAM bitStream, unsigned int *pValue, unsigned int nBits1, unsigned int nBits2, unsigned int nBits3);

int UsacConfig_Advance(HANDLE_BSBITSTREAM bitStream, USAC_CONFIG *pUsacConfig, int WriteFlag);

USAC_SBR_RATIO_INDEX UsacConfig_GetSbrRatioIndex(unsigned int coreSbrFrameLengthIndex); 

USAC_OUT_FRAMELENGTH UsacConfig_GetOutputFrameLength(unsigned int coreSbrFrameLengthIndex);

int UsacConfig_GetMps212NumSlots(unsigned int coreSbrFrameLengthIndex);

USAC_ELEMENT_TYPE UsacConfig_GetUsacElementType(USAC_CONFIG *pUsacConfig, unsigned int elemIdx);

USAC_CORE_CONFIG * UsacConfig_GetUsacCoreConfig(USAC_CONFIG *pUsacConfig, unsigned int elemIdx);

USAC_SBR_CONFIG * UsacConfig_GetUsacSbrConfig(USAC_CONFIG *pUsacConfig, unsigned int elemIdx);

USAC_MPS212_CONFIG * UsacConfig_GetUsacMps212Config(USAC_CONFIG *pUsacConfig, unsigned int elemIdx);

int UsacConfig_GetStereoConfigIndex(USAC_CONFIG *pUsacConfig, unsigned int elemIdx);

#endif /* __INCLUDED_USAC_CONFIG_H */
