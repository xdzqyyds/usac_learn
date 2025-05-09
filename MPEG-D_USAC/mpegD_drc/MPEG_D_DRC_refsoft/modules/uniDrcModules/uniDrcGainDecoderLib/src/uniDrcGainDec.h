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

#ifndef _UNI_DRC_GAIN_DEC_H_
#define _UNI_DRC_GAIN_DEC_H_

#include "uniDrcGainDecoder_api.h"

#if AMD1_SYNTAX
#include "parametricDrcDecoder.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    Node node;
    Node nodePrevious;
    float lpcmGains[2 * AUDIO_CODEC_FRAME_SIZE_MAX + MAX_SIGNAL_DELAY];
} BufferForInterpolation;

typedef struct {
#if DRC_GAIN_DEBUG_FILE
    FILE* fGainDebug;
#endif
    int bufferForInterpolationCount;
    BufferForInterpolation* bufferForInterpolation;
} GainBuffer;

typedef struct {
    GainBuffer gainBuffer[SEL_DRC_COUNT];
} DrcGainBuffers;

typedef struct {
    int    gainInterpolationType;
    int    drcEffectAllowsGainModification;
    int    drcEffectIsDucking;
    int    drcEffectIsClipping;
    DuckingModifiers* duckingModifiers;
    GainModifiers* gainModifiers;
#if MPEG_D_DRC_EXTENSION_V1
    int drcCharacteristicPresent;
    int drcSourceCharacteristicFormatIsCICP;
    int sourceDrcCharacteristic;
    SplitDrcCharacteristic* splitSourceCharacteristicLeft;
    SplitDrcCharacteristic* splitSourceCharacteristicRight;
    int drcTargetCharacteristicFormatIsCICP;
    int targetDrcCharacteristic;
    SplitDrcCharacteristic* splitTargetCharacteristicLeft;
    SplitDrcCharacteristic* splitTargetCharacteristicRight;
    int interpolationForLoudEq;
#endif /* MPEG_D_DRC_EXTENSION_V1 */
    int    limiterPeakTargetPresent;
    float  limiterPeakTarget;
    float  loudnessNormalizationGainDb;
    int    deltaTmin;
    int    characteristicIndex;
    float  compress;
    float  boost;
} InterpolationParams;


typedef struct {
    int drcInstructionsIndex;
    int drcCoefficientsIndex;
    int downmixInstructionsIndex;
} SelDrc;

/* Major DRC tool parameters */
typedef struct {
    int audioSampleRate;
    int deltaTminDefault;
    int drcFrameSize;
    int delayMode;
    int subBandDomainMode;
    int gainDelaySamples;
#if AMD1_SYNTAX
    int parametricDrcDelay;
    int eqDelay;
#endif /* AMD1_SYNTAX */
    int audioDelaySamples;
    int drcSetCounter;
    int multiBandSelDrcIndex;

    /* selected DRC sets in gain decoder */
    SelDrc selDrc[SEL_DRC_COUNT];

#if MPEG_H_SYNTAX
    int channelOffset;
    int numChannelsProcess;
#endif
} DrcParams;

int
initGainBuffer(const int index,
               const int gainElementCount,
               const int drcFrameSize,
               GainBuffer* gainBuffer);

int
initDrcGainBuffers (UniDrcConfig* uniDrcConfig,
                    DrcParams* drcParams,
                    DrcGainBuffers* drcGainBuffers);

int
removeGainBuffer (GainBuffer* gainBuffer);

int
removeDrcGainBuffers (DrcParams* drcParams,
                      DrcGainBuffers* drcGainBuffers);

int
advanceBuffer(const int drcFrameSize,
              GainBuffer* gainBuffer);

#if MPEG_D_DRC_EXTENSION_V1
int
compressorIO_sigmoid(SplitDrcCharacteristic* splitDrcCharacteristic,
                     const float inLevelDb,
                     float* outGainDb);
int
compressorIO_sigmoid_inverse(SplitDrcCharacteristic* splitDrcCharacteristic,
                             const float gainDb,
                             float* inLev);
int
compressorIO_nodesLeft(SplitDrcCharacteristic* splitDrcCharacteristic,
                       const float inLevelDb,
                       float *outGainDb);
int
compressorIO_nodesRight(SplitDrcCharacteristic* splitDrcCharacteristic,
                        const float inLevelDb,
                        float *outGainDb);
int
compressorIO_nodes_inverse(SplitDrcCharacteristic* splitDrcCharacteristic,
                           const float gainDb,
                           float *inLev);
#endif /* MPEG_D_DRC_EXTENSION_V1 */

int
mapGain (
#if MPEG_D_DRC_EXTENSION_V1
         SplitDrcCharacteristic* splitDrcCharacteristicSource,
         SplitDrcCharacteristic* splitDrcCharacteristicTarget,
#endif /* MPEG_D_DRC_EXTENSION_V1 */
         const float gainInDb,
         float* gainOutDb);

int
toLinear (InterpolationParams* interpolationParams,
          const int     drcBand,
          const float   gainDb,
          const float   slopeDb,
          float*        gainLin,
          float*        slopeLin);

int
interpolateDrcGain(InterpolationParams* interpolationParams,
                   const int    drcBand,
                   const int    tGainStep,
                   const float  gain0,
                   const float  gain1,
                   const float  slope0,
                   const float  slope1,
                   float* result);

int
concatenateSegments(const int drcFrameSize,
                    const int drcBand,
                    InterpolationParams* interpolationParams,
                    SplineNodes* splineNodes,
                    BufferForInterpolation* bufferForInterpolation);

int
getDrcGain (HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
            HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
            HANDLE_UNI_DRC_GAIN              hUniDrcGain,
            const float                      compress,
            const float                      boost,
            const int                        characteristicIndex,
            const float                      loudnessNormalizationGainDb,
            const int                        subDrcIndex,
            DrcGainBuffers*                  drcGainBuffers);

#if MPEG_D_DRC_EXTENSION_V1
int
getEqLoudness (HANDLE_LOUDNESS_EQ_STRUCT        hLoudnessEq,
               HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
               HANDLE_UNI_DRC_GAIN              hUniDrcGain);
#endif /* MPEG_D_DRC_EXTENSION_V1 */


#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_GAIN_DEC_H_ */
