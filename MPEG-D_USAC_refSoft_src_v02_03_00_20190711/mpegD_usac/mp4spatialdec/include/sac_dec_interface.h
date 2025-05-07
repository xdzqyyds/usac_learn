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



#ifndef SAC_DEC_INTERFACE_H
#define SAC_DEC_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>

typedef struct {

  int bsFreqRes;
  int bsFixedGainDMX;
  int bsTempShapeConfig;
  int bsDecorrConfig;
  int bsHighRateMode;
  int bsPhaseCoding;
  int bsOttBandsPhasePresent;
  int bsOttBandsPhase;
  int bsResidualBands;
  int bsPseudoLr;
  int bsEnvQuantMode;

} SAC_DEC_USAC_MPS212_CONFIG;

typedef struct spatialDec_struct spatialDec, *HANDLE_SPATIAL_DEC;

typedef struct spatialBsConfig_struct spatialBsConfig, *HANDLE_BS_CONFIG;

typedef struct byteReader_struct BYTE_READER, *HANDLE_BYTE_READER;

struct byteReader_struct {
    int (*ReadByte)(HANDLE_BYTE_READER byteReader);
};

#ifdef HRTF_DYNAMIC_UPDATE

typedef struct {
  float *powerLeft;
  float *powerRight;
  float *icc;
  float *ipd;
  float *impulseLeft;
  float *impulseRight;
} SPATIAL_HRTF_DIRECTIONAL_DATA;

typedef struct {
  int sampleRate;
  int parameterBands;
  int impulseLength;
  int qmfBands;
  int azimuths;
  SPATIAL_HRTF_DIRECTIONAL_DATA *directional;
  int tauLfLsLeft;
  int tauLfLsRight;
  int tauRfRsLeft;
  int tauRfRsRight;
} SPATIAL_HRTF_DATA;

typedef struct hrtfSource_struct HRTF_SOURCE, *HANDLE_HRTF_SOURCE;

struct hrtfSource_struct {
    SPATIAL_HRTF_DATA* (*GetHRTF)(HANDLE_HRTF_SOURCE hrtfSource);
};

#endif 


spatialDec* SpatialDecOpen(HANDLE_BYTE_READER bitstream,
                           char* spatialPcmOutFileName,
                           int samplingFreq,
                           int nInChannels,
                           int* pTimeSlots,
                           int nQmfBands,
                           int bsFrameLength,
                           int bsResidualCoding,
                           int nDecType,
                           int nUpmixType,
                           int nBinauralQuality,
                           int bStandaloneSsc,
                           int nBitsAvailable, 
                           int *pnBitsRead, 
                           SAC_DEC_USAC_MPS212_CONFIG *pUsacMps212Config);

int SpatialDecResetBitstream(spatialDec* self, 
                             HANDLE_BYTE_READER bitstream);

void SpatialDecParseFrame(spatialDec* self, 
                          int *pnBitsRead, 
                          int     bUsacIndependencyFlag
                          );

void SpatialDecApplyFrame(spatialDec* self,
                          int inTimeSlots,
                          float** pointers[4],
                          float** ppTimeOut, 
                          float   scalingFactor, int el
                          );

void SpatialDecClose(spatialDec* self);

float SpatialGetClipProtectGain(spatialDec* self);

int SpatialGetQmfBands(spatialDec *self);

int SpatialDecGetNumOutputChannels(spatialDec *self);

int SpatialDecGetNumTimeSlots(spatialDec *self);

#ifdef __cplusplus
}
#endif

#endif 
