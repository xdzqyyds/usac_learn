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

#ifndef SAC_ENC_H
#define SAC_ENC_H

#include "stdio.h"

#include "sac_polyphase_enc.h"
#include "sac_hybfilter_enc.h"
#include "sac_bitstream_enc.h"

typedef struct sSpatialEnc {
  int treeConfig;
  int outputChannels;
  int timeSlots;
  int frameSize;
  int lowBitrateMode;
  int qmfBands;
  
  int decoderDelay;
  int nBitstreamDelayBuffer;
  int nBitstreamBufferRead;
  int nBitstreamBufferWrite;
  unsigned char **ppBitstreamDelayBuffer;
  int *pnOutputBits;
  int nOutputBufferDelay; 

  FILE *pTsdDataFile;

  SAC_POLYPHASE_SYN_FILTERBANK *synfilterbank[2];
  SAC_POLYPHASE_ANA_FILTERBANK *anafilterbank[6];  
  tHybFilterState hybState[6];
  BSF_INSTANCE *bitstreamFormatter;
} tSpatialEnc,*HANDLE_SPATIAL_ENC;

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

} SAC_ENC_USAC_MPS212_CONFIG;

tSpatialEnc *SpatialEncOpen(int treeConfig, int timeSlots, int sampleRate, int *bufferSize, Stream *bitstream, int flag_ipd, int flag_mpsres, char* tsdInputFileName);

void SpatialEncApply(tSpatialEnc *self, float *audioInput, float *audioOutput, Stream *bitstream, int bUsacIndepencencyFlag);

void SpatialEncClose(tSpatialEnc *self);

void SpatialEncWriteSSC(tSpatialEnc *self, Stream *bitstream);

void SpatialEncGetUsacMps212Config(tSpatialEnc *self, SAC_ENC_USAC_MPS212_CONFIG *pUsacMps212Config);

void SpaceEncInitDelayCompensation(tSpatialEnc *self, const int delay, const int APRFrames);

#endif
