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

#ifndef _UNI_DRC_SELECTION_H_
#define _UNI_DRC_SELECTION_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
 
    
#define SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH        (1<<0)
#define SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT       (1<<1)
    
typedef struct {
    int drcInstructionsIndex;
    int downmixIdRequestIndex;
    int eqSetId;
    float outputPeakLevel;
    float loudnessNormalizationGainDbAdjusted;
    float outputLoudness;
    float mixingLevel;
    int selectionFlags;
} SelectionCandidateInfo;

#if DEBUG_DRC_SELECTION
int
printEffectTypes(const int drcSetEffect);

int
displaySelectedDrcSets(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct);
#endif

int
validateDrcFeatureRequest(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams);
    
int
mapRequestedEffectToBitIndex(const int effectTypeRequested,
                             int* effectBitIndex);

int
getFadingDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct);

int
getDuckingDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct);

int
getSelectedDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                  const int drcSetIdSelected);

int
getDependentDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct);

int
getDrcInstructionsDependent(const UniDrcConfig* uniDrcConfig,
                            const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                            DrcInstructionsUniDrc** drcInstructionsDependent);

#if MPEG_D_DRC_EXTENSION_V1
int
manageDrcComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                     HANDLE_UNI_DRC_CONFIG hUniDrcConfig);

int
manageEqComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                    HANDLE_UNI_DRC_CONFIG hUniDrcConfig);

int
manageComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                  HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                  int* repeatSelection);

int
getSelectedEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                 const int eqSetIdSelected);
int
getDependentEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct);

int
getSelectedLoudEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                     const int loudEqSetIdSelected);
#endif  /* MPEG_D_DRC_EXTENSION_V1  */
    
int
selectDrcsWithoutCompressionEffects (HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                                     int* matchFound,
                                     int* selectionCandidateCount,
                                     SelectionCandidateInfo* selectionCandidateInfo);

int
matchEffectTypeAttempt(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       const int effectTypeRequested,
                       const int stateRequested,
                       int* matchFound,
                       int* selectionCandidateCount,
                       SelectionCandidateInfo* selectionCandidateInfo);

int
matchEffectTypes(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                 const int effectTypeRequestedTotalCount,
                 const int effectTypeRequestedDesiredCount,
                 const int* effectTypeRequested,
                 int* selectionCandidateCount,
                 SelectionCandidateInfo* selectionCandidateInfo);

int
matchDynamicRange(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                  HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                  const int dynamicRangeMeasurementType,
                  const int dynamicRangeRequestedIsRange,
                  const float dynamicRangeRequested,
                  const float dynamicRangeMinRequested,
                  const float dynamicRangeMaxRequested,
                  const int*  downmixIdRequested,
                  const int albumMode,
                  int* selectionCandidateCount,
                  SelectionCandidateInfo* selectionCandidateInfo);

int
matchDrcCharacteristicAttempt(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                              const int drcCharacteristicRequest,
                              int* matchFound,
                              int* selectionCandidateCount,
                              SelectionCandidateInfo* selectionCandidateInfo);

int
matchDrcCharacteristic(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       const int drcCharacteristicRequested,
                       int* selectionCandidateCount,
                       SelectionCandidateInfo* selectionCandidateInfo);
    
int
selectDrcCoefficients3(UniDrcConfig* uniDrcConfig,
                       const int location,
                       DrcCoefficientsUniDrc** drcCoefficientsUniDrc);

int
drcSetPreselection(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                   HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                   HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                   int restrictToDrcWithAlbumLoudness,
#if MPEG_D_DRC_EXTENSION_V1
                   int* drcSetIdIsPermitted,
                   int* eqSetIdIsPermitted,
#endif
                   int* selectionCandidateCount,
                   SelectionCandidateInfo* selectionCandidateInfo);

int
drcSetFinalSelection(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                     HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                     int* selectionCandidateCount,
                     SelectionCandidateInfo* selectionCandidateInfo
#if MPEG_D_DRC_EXTENSION_V1
                     , int* eqSetIdIsPermitted
                     , int* selectedEqSetCount
                     , int selectedEqSets[][2]
#endif
);
    
int
selectDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
             int* drcSetIdSelected
#if MPEG_D_DRC_EXTENSION_V1
             , int* eqSetIdSelected
             , int* loudEqIdSelected
#endif
            );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNI_DRC_SELECTION_H_ */
