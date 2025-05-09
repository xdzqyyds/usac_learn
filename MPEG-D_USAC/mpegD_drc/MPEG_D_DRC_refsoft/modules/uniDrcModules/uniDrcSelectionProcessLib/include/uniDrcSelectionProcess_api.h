/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc. and Fraunhofer IIS
 
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
 
 Apple Inc. and Fraunhofer IIS retains full right to modify and use the code for its
 own purpose, assign or donate the code to a third party and to inhibit third parties
 from using the code for products that do not conform to MPEG-related ITU Recommenda-
 tions and/or ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/


#ifndef _UNI_DRC_SELECTION_PROCESS_API_H_
#define _UNI_DRC_SELECTION_PROCESS_API_H_

#include "uniDrcCommon_api.h"
#include "uniDrcCommon.h"

#define EFFECT_TYPE_REQUESTED_NONE                      0
#define EFFECT_TYPE_REQUESTED_NIGHT                     1
#define EFFECT_TYPE_REQUESTED_NOISY                     2
#define EFFECT_TYPE_REQUESTED_LIMITED                   3
#define EFFECT_TYPE_REQUESTED_LOWLEVEL                  4
#define EFFECT_TYPE_REQUESTED_DIALOG                    5
#define EFFECT_TYPE_REQUESTED_GENERAL_COMPR             6
#define EFFECT_TYPE_REQUESTED_EXPAND                    7
#define EFFECT_TYPE_REQUESTED_ARTISTIC                  8
#define EFFECT_TYPE_REQUESTED_COUNT                     9

#define MATCH_EFFECT_TYPE                               0
#define MATCH_DYNAMIC_RANGE                             1
#define MATCH_DRC_CHARACTERISTIC                        2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
/* handles */
typedef struct T_UNI_DRC_SEL_PROC_STRUCT *HANDLE_UNI_DRC_SEL_PROC_STRUCT;
typedef struct T_UNI_DRC_SEL_PROC_PARAMS *HANDLE_UNI_DRC_SEL_PROC_PARAMS;

typedef struct T_UNI_DRC_SEL_PROC_PARAMS{
    /* system parameters */
    int   baseChannelCount; /* optional */
    int   baseLayout; /* optional */
    int   targetConfigRequestType;
    int   numDownmixIdRequests;
    int   downmixIdRequested[MAX_NUM_DOWNMIX_ID_REQUESTS];
    int   targetLayoutRequested;
    int   targetChannelCountRequested;
    int   targetChannelCountPrelim;
    
    /* loudness normalization parameters */
    int   loudnessNormalizationOn;
    float targetLoudness;
    int   albumMode;
    int   peakLimiterPresent;
    int   loudnessDeviationMax; /* only 1 dB steps */
    int   loudnessMeasurementMethod;
    int   loudnessMeasurementSystem;
    int   loudnessMeasurementPreProc;
    int   deviceCutOffFrequency;
    float loudnessNormalizationGainDbMax;
    float loudnessNormalizationGainModificationDb;
    float outputPeakLevelMax;
    
    /* dynamic range control parameters */
    int   numBandsSupported;
    int   dynamicRangeControlOn;
    int   numDrcFeatureRequests;
    int   drcFeatureRequestType[MAX_NUM_DRC_FEATURE_REQUESTS];
    int   numDrcEffectTypeRequests[MAX_NUM_DRC_FEATURE_REQUESTS];
    int   numDrcEffectTypeRequestsDesired[MAX_NUM_DRC_FEATURE_REQUESTS];
    int   drcEffectTypeRequest[MAX_NUM_DRC_FEATURE_REQUESTS][MAX_NUM_DRC_EFFECT_TYPE_REQUESTS];
    int   dynamicRangeMeasurementRequestType[MAX_NUM_DRC_FEATURE_REQUESTS];
    int   dynamicRangeRequestedIsRange[MAX_NUM_DRC_FEATURE_REQUESTS];
    float dynamicRangeRequestValue[MAX_NUM_DRC_FEATURE_REQUESTS];
    float dynamicRangeRequestValueMin[MAX_NUM_DRC_FEATURE_REQUESTS];
    float dynamicRangeRequestValueMax[MAX_NUM_DRC_FEATURE_REQUESTS];
    int   drcCharacteristicRequest[MAX_NUM_DRC_FEATURE_REQUESTS];
    
    /* MPEG-H parameters */
#if MPEG_H_SYNTAX
    int numGroupIdsRequested;
    int groupIdRequested[MAX_NUM_GROUP_ID_REQUESTS];
    int numGroupPresetIdsRequested;
    int groupPresetIdRequested[MAX_NUM_GROUP_PRESET_ID_REQUESTS];
    int numMembersGroupPresetIdsRequested[MAX_NUM_MEMBERS_GROUP_PRESET];
    int groupPresetIdRequestedPreference;
#endif
    
#if MPEG_D_DRC_EXTENSION_V1
    int   loudnessEqRequest;
    float sensitivity;
    float playbackGain;
    int   eqSetPurposeRequest;
#endif

#ifdef LEVELING_SUPPORT
    int loudnessLevelingOn;
#endif
    
    /* other */
    float boost;
    float compress;
    int drcCharacteristicTarget;
} UniDrcSelProcParams;
    
/* open */
int
openUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT *phUniDrcSelProcStruct);
    
/* init */
int
initUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                           HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                           HANDLE_UNI_DRC_INTERFACE hUniDrcInterface,
                           const int subBandDomainMode);

#if MPEG_H_SYNTAX
/* set MPEG-H specific params */
int
setMpeghParamsUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                       int numGroupIdsRequested,
                                       int *groupIdRequested,
                                       int numGroupPresetIdsRequested,
                                       int *groupPresetIdRequested,
                                       int *numMembersGroupPresetIdsRequested,
                                       int groupPresetIdRequestedPreference);
#endif
   
/* process */
int
processUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                              HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                              HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                              HANDLE_UNI_DRC_SEL_PROC_OUTPUT hUniDrcSelProcOutput);

int
getMultiBandDrcPresent(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       int                   numSets[3],
                       int                   multiBandDrcPresent[3]);

int
findLoudEqInstructionsIndexForId(HANDLE_UNI_DRC_CONFIG uniDrcConfig,
                                 const int loudEqSetIdRequested,
                                 int* instructionsIndex);

/* close */
int closeUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT *phUniDrcSelProcStruct);
 
    
#endif /* _UNI_DRC_SELECTION_PROCESS_API_H_ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

