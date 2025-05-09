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

#ifndef _UNI_DRC_LOUDNESS_CONTROL_H_
#define _UNI_DRC_LOUDNESS_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MPEG_D_DRC_EXTENSION_V1
int
getMixingLevel(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
               HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
               const int downmixIdRequested,
               const int drcSetIdRequested,
               const int eqSetIdRequested,
               float* mixingLevel);
#endif
int
getSignalPeakLevel(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                   HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                   DrcInstructionsUniDrc* drcInstructionsUniDrc,
                   const int downmixIdRequested,
                   const int albumMode,
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
                   HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
#endif
#endif
                   const int noCompressionEqCount,
                   const int* noCompressionEqId,
                   int* peakInfoCount,
                   int eqSetId[],
                   float signalPeakLevel[],
                   int explicitPeakInformationPresent[]);

int
extractLoudnessPeakToAverageValue(LoudnessInfo* loudnessInfo,
                                  const int     dynamicRangeMeasurementType,
                                  int *         loudnessPeakToAverageValuePresent,
                                  float*        loudnessPeakToAverageValue);

int
getLoudnessPeakToAverageValue(
                              HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                              DrcInstructionsUniDrc* drcInstructionsUniDrc,
                              const int downmixIdRequested,
                              const int dynamicRangeMeasurementType,
                              const int albumMode,
                              int* loudnessPeakToAverageValuePresent,
                              float* loudnessPeakToAverageValue);

int
checkIfOverallLoudnessPresent(const LoudnessInfo* loudnessInfo,
                              int* loudnessInfoPresent);

int
findInfoWithOverallLoudness(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                            HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                            const int downmixIdRequested,
                            const int drcSetIdRequested,
#if MPEG_H_SYNTAX
                            const int groupIdRequested,
                            const int groupPresetIdRequested,
#endif
                            int* overallLoudnessInfoPresent,
                            int* loudnessInfoCount,
                            LoudnessInfo* loudnessInfo[]);

int
getHighPassLoudnessAdjustment(const LoudnessInfo* loudnessInfo,
                              int* loudnessHighPassAdjustPresent,
                              float* loudnessHighPassAdjust);

int
findHighPassLoudnessAdjustment(
                               HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                               const int downmixIdRequested,
                               const int drcSetIdRequested,
                               const int albumMode,
                               const float deviceCutoffFreq,
                               int* loudnessHighPassAdjustPresent,
                               float* loudnessHighPassAdjust);

int
initLoudnessControl (HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                     HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                     const int downmixIdRequested,
                     const int drcSetIdRequested,
#if MPEG_H_SYNTAX
                     const int groupIdRequested,
                     const int groupPresetIdRequested,
#endif
                     const int  noCompressionEqCount,
                     const int* noCompressionEqId,
                     int*   loudnessInfoCount,
                     int    eqSetId[],
                     float  loudnessNormalizationGainDb[],
                     float  loudness[]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNI_DRC_LOUDNESS_CONTROL_H_ */
