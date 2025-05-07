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
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/


#ifndef _DRC_TOOL_API_H_
#define _DRC_TOOL_API_H_

#include "uniDrcCommon_api.h"
#include "uniDrcCommon.h"
#include "uniDrc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Handles to parameter structs */
typedef struct T_UNI_DRC_GAIN_DEC_STRUCTS     *HANDLE_UNI_DRC_GAIN_DEC_STRUCTS;
typedef struct T_LOUDNESS_EQ_STRUCT           *HANDLE_LOUDNESS_EQ_STRUCT;
    
/* init functions */
int drcDecOpen(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS *phUniDrcGainDecStructs,
               HANDLE_UNI_DRC_CONFIG *phUniDrcConfig,
               HANDLE_LOUDNESS_INFO_SET *phLoudnessInfoSet,
               HANDLE_UNI_DRC_GAIN *phUniDrcGain);

int drcDecInit(   const int                        audioFrameSize,
                  const int                        audioSampleRate,
                  const int                        gainDelaySamples,
                  const int                        delayMode,
                  const int                        subBandDomainMode,
                  HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs);

int drcDecInitAfterConfig(    const int                        audioChannelCount,
                              const int*                       drcSetIdProcessed,
                              const int*                       downmixIdProcessed,
                              const int                        numSetsProcessed,
#if MPEG_D_DRC_EXTENSION_V1
                              const int                        eqSetIdProcessed,
#endif
#if MPEG_H_SYNTAX
                              const int                        channelOffset,
                              const int                        numChannelsProcess,
#endif
                              HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                              HANDLE_UNI_DRC_CONFIG            hUniDrcConfig
#if AMD1_SYNTAX
                            , HANDLE_LOUDNESS_INFO_SET         hLoudnessInfoSet
#endif
                        );

/* close functions */
int drcDecClose( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS *phUniDrcGainDecStructs,
                 HANDLE_UNI_DRC_CONFIG *phUniDrcConfig,
                 HANDLE_LOUDNESS_INFO_SET *phLoudnessInfoSet,
                 HANDLE_UNI_DRC_GAIN *phUniDrcGain
#if MPEG_D_DRC_EXTENSION_V1
                , HANDLE_LOUDNESS_EQ_STRUCT* hLoudnessEq
#endif
                );


/* DRC process function (implies decoding and applying gain) */
int drcProcessTime( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                    HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                    HANDLE_UNI_DRC_GAIN              hUniDrcGain,
                    float*                           audioIOBuffer[],
                    const float                      loudnessNormalizationGainDb,
                    const float                      boost,
                    const float                      compress,
                    const int                        drcCharacteristic);

int drcProcessFreq( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                    HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                    HANDLE_UNI_DRC_GAIN              hUniDrcGain,
                    float*                           audioIOBufferReal[],
                    float*                           audioIOBufferImag[],
                    const float                      loudnessNormalizationGainDb,
                    const float                      boost,
                    const float                      compress,
                    const int                        drcCharacteristic);

#if MPEG_D_DRC_EXTENSION_V1

int
initLoudEq(HANDLE_LOUDNESS_EQ_STRUCT* loudnessEq,
           HANDLE_UNI_DRC_GAIN_DEC_STRUCTS hDrcDec,
           UniDrcConfigExt* uniDrcConfigExt,
           int selectedLoudEqId,
           float mixingLevel);

int
removeLoudEq(HANDLE_LOUDNESS_EQ_STRUCT* loudnessEq);

int
updateLoudnessEqualizerParams(HANDLE_LOUDNESS_EQ_STRUCT loudnessEq,
                              HANDLE_UNI_DRC_INTERFACE hUniDrcInterface);

int
processLoudEq(HANDLE_LOUDNESS_EQ_STRUCT        hLoudnessEq,
              HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
              HANDLE_UNI_DRC_GAIN              hUniDrcGain);

#endif /* MPEG_D_DRC_EXTENSION_V1 */

#if AMD1_SYNTAX
/* get parametric DRC delay */
int
getParametricDrcDelay(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                      HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                      int* parametricDrcDelay,
                      int* parametricDrcDelayMax);
    
/* get EQ delay */
int
getEqDelay(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
           HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
           int* eqDelay,
           int* eqDelayMax);

#if AMD2_COR2
int
getFilterBanksComplexity (HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                          int                   drcInstructionsV1Index,
                          float*                complexity);
#endif
#endif
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DRC_TOOL_API_H_ */

