/***********************************************************************************
 
 This software module was originally developed by 
 
 Fraunhofer IIS
 
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
  
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using 
 the code for products that do not conform to MPEG-related ITU Recommendations and/or 
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works. 
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#ifndef _PARAMETRIC_DRC_DECODER_H_
#define _PARAMETRIC_DRC_DECODER_H_

#if AMD1_SYNTAX

#include "uniDrcCommon_api.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
/* biquad filter structs */
typedef struct T_FILTER_COEFF_2ND_ORDER {   /* coefficients */
    float b0, b1, b2;
    float a1, a2;
} FILTER_COEFF_2ND_ORDER;

typedef struct T_FILTER_STATE_2ND_ORDER {   /* states */
    float z1, z2;
} FILTER_STATE_2ND_ORDER;
    
/* parametric DRC structs */
typedef struct T_PARAMETRIC_DRC_TYPE_FF_PARAMS {
    
    /* level estimation */
    int audioChannelCount;
    int frameSize;
    int subBandDomainMode;
    int subBandCount;
    int levelEstimIntegrationTime;
    int levelEstimFrameIndex;
    int levelEstimFrameCount;
    float level[PARAM_DRC_TYPE_FF_LEVEL_ESTIM_FRAME_COUNT_MAX];
    int startUpPhase;
    float levelEstimChannelWeight[CHANNEL_COUNT_MAX];
    int levelEstimKWeightingType;
    FILTER_COEFF_2ND_ORDER preFilterCoeff;
    FILTER_COEFF_2ND_ORDER rlbFilterCoeff;
    FILTER_STATE_2ND_ORDER preFilterState[CHANNEL_COUNT_MAX];
    FILTER_STATE_2ND_ORDER rlbFilterState[CHANNEL_COUNT_MAX];
    float weightingFilter[AUDIO_CODEC_SUBBAND_COUNT_MAX];
    int subBandCompensationType;
    FILTER_COEFF_2ND_ORDER filterCoeffSubBand;
    FILTER_STATE_2ND_ORDER filterStateSubBandReal[CHANNEL_COUNT_MAX];
    FILTER_STATE_2ND_ORDER filterStateSubBandImag[CHANNEL_COUNT_MAX];
    float referenceLevelParametricDrc;
    
    /* gain mapping */
    int nodeCount;
    int nodeLevel[PARAM_DRC_TYPE_FF_NODE_COUNT_MAX];
    int nodeGain[PARAM_DRC_TYPE_FF_NODE_COUNT_MAX];
    
    /* gain smoothing */
    float gainSmoothAttackAlphaSlow;
    float gainSmoothReleaseAlphaSlow;
    float gainSmoothAttackAlphaFast;
    float gainSmoothReleaseAlphaFast;
    int gainSmoothAttackThreshold;
    int gainSmoothReleaseThreshold;
    int gainSmoothHoldOffCount;
    float levelSmoothDb;
    float gainSmoothDb;
    int holdCounter;
    
} PARAMETRIC_DRC_TYPE_FF_PARAMS;
    
#ifdef AMD1_PARAMETRIC_LIMITER
typedef struct T_PARAMETRIC_DRC_TYPE_LIM_PARAMS {
    
    int audioChannelCount;
    int frameSize;
    float levelEstimChannelWeight[CHANNEL_COUNT_MAX];
    
    unsigned int  attack;
    float         attackConst;
    float         releaseConst;
    float         attackMs;
    float         releaseMs;
    float         threshold;
    unsigned int  channels;
    unsigned int  sampleRate;
    float         cor;
    float*        maxBuf;
    float*        maxBufSlow;
    unsigned int  maxBufIdx;
    unsigned int  maxBufSlowIdx;
    unsigned int  secLen;
    unsigned int  nMaxBufSec;
    unsigned int  maxBufSecIdx;
    unsigned int  maxBufSecCtr;
    double        smoothState0;
    
} PARAMETRIC_DRC_TYPE_LIM_PARAMS;
#endif
    
typedef struct T_PARAMETRIC_DRC_INSTANCE_PARAMS {
    int disableParamtricDrc;
    int parametricDrcType;
    SplineNodes splineNodes;
    PARAMETRIC_DRC_TYPE_FF_PARAMS parametricDrcTypeFeedForwardParams;
#ifdef AMD1_PARAMETRIC_LIMITER
    PARAMETRIC_DRC_TYPE_LIM_PARAMS parametricDrcTypeLimParams;
#endif
} PARAMETRIC_DRC_INSTANCE_PARAMS;
    
typedef struct T_PARAMETRIC_DRC_PARAMS {    
    int sampleRate;
    int audioChannelCount;
    int subBandDomainMode;
    int subBandCount;

    int nNodes;
    int drcFrameSize;
    int drcFrameSizeParametricDrc;
    int parametricDrcLookAheadSamplesMax;
    int resetParametricDrc;
    
    int parametricDrcInstanceCount;
    int parametricDrcIndex[PARAM_DRC_INSTANCE_COUNT_MAX]; /* can be different from parametricDrcId */
    int gainSetIndex[PARAM_DRC_INSTANCE_COUNT_MAX];
    int downmixIdFromDrcInstructions[PARAM_DRC_INSTANCE_COUNT_MAX];
    int channelMap[PARAM_DRC_INSTANCE_COUNT_MAX][CHANNEL_COUNT_MAX];

    PARAMETRIC_DRC_INSTANCE_PARAMS parametricDrcInstanceParams[PARAM_DRC_INSTANCE_COUNT_MAX];
    
#if MPEG_H_SYNTAX
    int channelOffset;
    int numChannelsProcess;
#endif
} PARAMETRIC_DRC_PARAMS;
    
/* init functions */
int initParametricDrc(const int                      drcFrameSize,
                      const int                        sampleRate,
#if MPEG_H_SYNTAX
                      const int                     channelOffset,
                      const int                numChannelsProcess,
#endif
                      int                       subBandDomainMode,
                      PARAMETRIC_DRC_PARAMS* hParametricDrcParams);
    
int initParametricDrcAfterConfig(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                                 HANDLE_LOUDNESS_INFO_SET   hLoudnessInfoSet,
                                 PARAMETRIC_DRC_PARAMS *hParametricDrcParams);
    
int
initParametricDrcInstance(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                          const int                     instanceIndex,
                          const int           channelCountOfDownmixId,
                          PARAMETRIC_DRC_PARAMS* hParametricDrcParams);
   
int
initParametricDrcTypeFeedForward(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                                 const int                     instanceIndex,
                                 const int           channelCountOfDownmixId,
                                 PARAMETRIC_DRC_PARAMS* hParametricDrcParams);
    
int
initLvlEstFilterTime(const int     levelEstimKWeightingType,
                     const int                   sampleRate,
                     FILTER_COEFF_2ND_ORDER* preFilterCoeff,
                     FILTER_COEFF_2ND_ORDER* rlbFilterCoeff);
    
int
initLvlEstFilterSubBand(const int         levelEstimKWeightingType,
                        const int                       sampleRate,
                        const int                subBandDomainMode,
                        const int                     subBandCount,
                        const int          subBandCompensationType,
                        float                     *weightingFilter,
                        FILTER_COEFF_2ND_ORDER* filterCoeffSubBand);

#ifdef AMD1_PARAMETRIC_LIMITER
int
initParametricDrcTypeLim(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                         const int                     instanceIndex,
                         const int         channelCountFromDownmixId,
                         PARAMETRIC_DRC_PARAMS* hParametricDrcParams);
#endif
    
/* reset functions */
int
resetParametricDrcInstance(const int                                      instanceIndex,
                           PARAMETRIC_DRC_INSTANCE_PARAMS* hParametricDrcInstanceParams);
    
int
resetParametricDrcTypeFeedForward(const int                                             instanceIndex,
                                  PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams);
    
#ifdef AMD1_PARAMETRIC_LIMITER
int
resetParametricDrcTypeLim(const int                                     instanceIndex,
                          PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams);
#endif
    
/* process functions */       
int
processParametricDrcInstance (float*                                       audioIOBuffer[],
                              float*                                   audioIOBufferReal[],
                              float*                                   audioIOBufferImag[],
                              PARAMETRIC_DRC_PARAMS*                  hParametricDrcParams,
                              PARAMETRIC_DRC_INSTANCE_PARAMS* hParametricDrcInstanceParams);

    
int
processParametricDrcTypeFeedForward(float*                                            audioIOBuffer[],
                                    float*                                        audioIOBufferReal[],
                                    float*                                        audioIOBufferImag[],
                                    int                                                        nodeIdx,
                                    PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams,
                                    SplineNodes*                                          splineNodes);

#ifdef AMD1_PARAMETRIC_LIMITER
int
processParametricDrcTypeLim(float*                                      audioIOBuffer[],
                            float                           loudnessNormalizationGainDb,
                            PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams,
                            float*                                            lpcmGains);
#endif
    
/* close functions */
int
removeParametricDrc(PARAMETRIC_DRC_PARAMS* hParametricDrcParams);
    
int
removeParametricDrcInstance(const int                     instanceIndex,
                            PARAMETRIC_DRC_PARAMS* hParametricDrcParams);

#ifdef __cplusplus
}
#endif
#endif /* AMD1_SYNTAX */
#endif /* _PARAMETRIC_DRC_DECODER_H_ */

