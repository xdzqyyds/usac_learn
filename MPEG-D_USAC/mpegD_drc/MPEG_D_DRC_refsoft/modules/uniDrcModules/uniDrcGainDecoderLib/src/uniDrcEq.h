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
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#ifndef _UNI_DRC_EQ_H_
#define _UNI_DRC_EQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef COMPILE_FOR_DRC_ENCODER
#include "uniDrcGainDecoder.h"
#endif

#define EQ_CHANNEL_COUNT_MAX                        8
#define EQ_AUDIO_DELAY_MAX                          1024
#define EQ_FIR_FILTER_SIZE_MAX                      128
#define EQ_SUBBAND_COUNT_MAX                        256
#define EQ_INTERMEDIATE_2ND_ORDER_PARAMS_COUNT_MAX  32
#define EQ_INTERMEDIATE_PARAMETER_COUNT_MAX         32
#define EQ_FILTER_SECTION_COUNT_MAX                 8
#define EQ_FILTER_ELEMENT_COUNT_MAX                 4
#define EQ_FILTER_COUNT_MAX                         4
#define MATCHING_PHASE_FILTER_COUNT_MAX             32

#define EQ_FILTER_DOMAIN_NONE                       0
#define EQ_FILTER_DOMAIN_TIME                       (1<<0)
#define EQ_FILTER_DOMAIN_SUBBAND                    (1<<1)

#ifdef __cplusplus
extern "C"
{
#endif

#if AMD1_SYNTAX || MPEG_D_DRC_EXTENSION_V1

    /* The following structures hold the EQ filters that are applied */

typedef struct {
    int delay;
    float state[EQ_CHANNEL_COUNT_MAX][EQ_AUDIO_DELAY_MAX];
} AudioDelay;
    
typedef struct {
    float radius;
    float coeff[2];      /* c0 is not inlcuded. It has a value of 1.0 */
} SecondOrderFilterParams;

typedef struct {
    int coeffCount;
    float coeff[EQ_FIR_FILTER_SIZE_MAX];
    float state[EQ_CHANNEL_COUNT_MAX][EQ_FIR_FILTER_SIZE_MAX];
} FirFilter;

typedef struct {
    int eqFrameSizeSubband;
    int coeffCount;
    float subbandCoeff[EQ_SUBBAND_COUNT_MAX];
} SubbandFilter;

typedef struct {
    int filterFormat;
    int filterParamCountForZeros;
    SecondOrderFilterParams secondOrderFilterParamsForZeros[EQ_INTERMEDIATE_2ND_ORDER_PARAMS_COUNT_MAX];
    int filterParamCountForPoles;
    SecondOrderFilterParams secondOrderFilterParamsForPoles[EQ_INTERMEDIATE_2ND_ORDER_PARAMS_COUNT_MAX];
    int filterParamCountForFir;
    FirFilter firFilter;
} IntermediateFilterParams;

typedef struct {
    int intermediateFilterParamCount;
    IntermediateFilterParams intermediateFilterParams[EQ_INTERMEDIATE_PARAMETER_COUNT_MAX];
} IntermediateParams;

typedef struct {
    float stateIn1;
    float stateIn2;
    float stateOut1;
    float stateOut2;
} FilterSectionState;

typedef struct {
    float a1;
    float a2;
    float b1;
    float b2;
    FilterSectionState filterSectionState[EQ_CHANNEL_COUNT_MAX];
} FilterSection;

typedef struct {
    int memberCount;
    int memberIndex[EQ_CHANNEL_GROUP_COUNT_MAX];
} CascadeAlignmentGroup;
    
typedef struct {
    int isValid;
    int matchesFilterCount;
    int matchesFilter[EQ_FILTER_SECTION_COUNT_MAX];
    float gain;
    int sectionCount;
    FilterSection filterSection[EQ_FILTER_SECTION_COUNT_MAX];
    AudioDelay audioDelay;
} PhaseAlignmentFilter;
    
typedef PhaseAlignmentFilter MatchingPhaseFilter;

typedef struct {
    int matchesCascadeIndex;
    int allpassCount;
    MatchingPhaseFilter matchingPhaseFilter[MATCHING_PHASE_FILTER_COUNT_MAX];
} AllpassChain;
    
typedef struct {
    int sectionCount;
    FilterSection filterSection[EQ_FILTER_SECTION_COUNT_MAX];
    int firCoeffsPresent;
    FirFilter firFilter;
    AudioDelay audioDelay;
} PoleZeroFilter;

typedef struct {
    float elementGainLinear;
    int format;
    PoleZeroFilter poleZeroFilter;
    FirFilter firFilter;
    int phaseAlignmentFilterCount;
    PhaseAlignmentFilter phaseAlignmentFilter[EQ_FILTER_ELEMENT_COUNT_MAX];
} EqFilterElement;

typedef struct {
    int elementCount;
    EqFilterElement eqFilterElement[EQ_FILTER_ELEMENT_COUNT_MAX];
    MatchingPhaseFilter matchingPhaseFilterElement0;
} EqFilterBlock;

typedef struct {
    float cascadeGainLinear;
    int blockCount;
    EqFilterBlock eqFilterBlock[EQ_FILTER_BLOCK_COUNT_MAX];
    int phaseAlignmentFilterCount;
    PhaseAlignmentFilter phaseAlignmentFilter[EQ_FILTER_BLOCK_COUNT_MAX * EQ_FILTER_BLOCK_COUNT_MAX];
} FilterCascadeTDomain;

typedef struct {
    int domain;
    int audioChannelCount;
    int eqChannelGroupCount;
    int eqChannelGroupForChannel[EQ_CHANNEL_COUNT_MAX];
    FilterCascadeTDomain filterCascadeTDomain[EQ_CHANNEL_GROUP_COUNT_MAX];
    SubbandFilter subbandFilter[EQ_CHANNEL_GROUP_COUNT_MAX];
} EqSet;

int
createEqSet(EqSet** hEqSet);

int
removeEqSet(EqSet** hEqSet);

int
deriveEqFilter (EqCoefficients* eqCoefficients,
                EqInstructions* eqInstructions,
                const float audioSampleRate,
                const int drcFrameSize,
                const int subBandDomainMode,
                EqSet* eqSet);
    
int
deriveEqSet (EqCoefficients* eqCoefficients,
             EqInstructions* eqInstructions,
             const float audioSampleRate,
             const int drcFrameSize,
             const int subBandDomainMode,
             EqSet* eqSet);
int
getEqComplexity (EqSet* eqSet,
                 int* eqComplexityLevel);
int
getEqSetDelay (EqSet* eqSet,
               int* cascadeDelay);
int
processEqSetTimeDomain(EqSet* eqSet,
                       const int channel,
                       float audioIn,
                       float* audioOut);
int
processEqSetSubbandDomain(EqSet* eqSet,
                          const int channel,
                          float* subbandSampleIn,
                          float* subbandSampleOut);


#endif /* MPEG_D_DRC_EXTENSION_V1 */
#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_EQ_H_ */
