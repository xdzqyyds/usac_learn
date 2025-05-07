/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc.
 
 in the course of development of the ISO/IEC 23003-4 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-4 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-4 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4
 standard.
 
 Apple Inc. retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#ifndef DrcEnc_mux_h
#define DrcEnc_mux_h

#include "writeonlybitbuf.h"

#define DRC_GAIN_CODING_PROFILE0_MIN (-31.875f)   /* only for regular gain sequences */
#define DRC_GAIN_CODING_PROFILE0_MAX 31.875f

#define DRC_GAIN_CODING_PROFILE1_MIN (-128.0f)
#define DRC_GAIN_CODING_PROFILE1_MAX 0.0f

#define DRC_GAIN_CODING_PROFILE2_MIN (-32.0f)
#define DRC_GAIN_CODING_PROFILE2_MAX 0.0f
/* ====================================================================================
 Parsing of in-stream DRC configuration
 ====================================================================================*/

#if AMD1_SYNTAX
int
writeDownmixCoefficientV1(wobitbufHandle bitstream,
                          const float downmixCoefficient[],
                          const int baseChannelCount,
                          const int targetChannelCount);
#endif /* AMD1_SYNTAX */

int
encDownmixCoefficient(const float downmixCoefficient,
                      const int isLfeChannel,
                      int* codeSize,
                      int* code);

int
encDuckingScaling(const float scaling,
                  int* bits,
                  float* scalingQuantized,
                  int *removeScalingValue);

int
encDuckingModifiers(wobitbufHandle bitstream,
                    DuckingModifiers* duckingModifiers);

int
encGainModifiers(wobitbufHandle bitstream,
                 const int version,
                 const int bandCount,
                 GainModifiers* gainModifiers);
int
writeLoudnessMeasure(wobitbufHandle bitstream,
                     LoudnessMeasure* loudnessMeasure);
int
writeLoudnessInfo(wobitbufHandle bitstream,
                  const int version,
                  LoudnessInfo* loudnessInfo);
int
writeDrcInstructions(wobitbufHandle bitstream,
                     UniDrcConfig* uniDrcConfig,
                     DrcEncParams* drcParams,
                     DrcInstructionsUniDrc* drcInstructionsUniDrc);
int
writeSequenceParamsCharacteristics(wobitbufHandle bitstream,
                                   GainParams* gainParams);
int
writeSequenceParamsCrossoverFreqIndex(wobitbufHandle bitstream,
                                      GainParams* gainParams,
                                      int drcBandType);

int
writeGainSetParams(wobitbufHandle bitstream,
                   const int version,
                   GainSetParams* gainSetParams);

int
writeDrcCoefficients(wobitbufHandle bitstream,
                     DrcCoefficientsUniDrc* drcCoefficientsUniDrc);
int
writeDownmixInstructions(wobitbufHandle bitstream,
                         const int version,
                         DrcEncParams* drcParams,
                         DownmixInstructions* downmixInstructions);

int
writeChannelLayout(wobitbufHandle bitstream,
                   ChannelLayout* channelLayout);

/* ====================================================================================
 Parsing of DRC gain sequences
 ====================================================================================*/

int
encMethodValue(const int methodDefinition,
               const float methodValue,
               int* codeSize,
               int* code);

int
encInitialGain(const int gainCodingProfile,
               float gainInitial,
               float* gainInitialQuant,
               int* codeSize,
               int* code);

int
writeSplineNodes(wobitbufHandle bitstream,
                 DrcEncParams* drcParams,
                 GainSetParams* gainSetParams,
                 DrcGroupForOutput* drcGroupForOutput);
int
writeDrcGainSequence(wobitbufHandle bitstream,
                     DrcEncParams* drcParams,
                     GainSetParams* gainSetParams,
                     DrcGroupForOutput* drcGroupForOutput);

#endif
