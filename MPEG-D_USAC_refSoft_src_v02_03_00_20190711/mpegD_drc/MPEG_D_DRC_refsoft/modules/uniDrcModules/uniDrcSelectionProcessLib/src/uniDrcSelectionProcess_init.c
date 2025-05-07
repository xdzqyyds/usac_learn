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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uniDrcSelectionProcess.h"
#include "uniDrcSelectionProcess_init.h"
#include "uniDrcSelectionProcess_drcSetSelection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
int
uniDrcSelectionProcessInit_defaultParams(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    if (hUniDrcSelProcStruct != NULL) {
        
        /* general */
        hUniDrcSelProcStruct->firstFrame = 0;
        
        /* system parameters */
        hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount = -1;
        hUniDrcSelProcStruct->uniDrcSelProcParams.baseLayout = -1;
        hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType = 0;
        hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 0;
        
        /* loudness normalization parameters */
        hUniDrcSelProcStruct->uniDrcSelProcParams.albumMode = FALSE;
#if !MPEG_H_SYNTAX
        hUniDrcSelProcStruct->uniDrcSelProcParams.peakLimiterPresent = FALSE;
#else
        hUniDrcSelProcStruct->uniDrcSelProcParams.peakLimiterPresent = TRUE;
#endif
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationOn = FALSE;
        hUniDrcSelProcStruct->uniDrcSelProcParams.targetLoudness = -24.0f;
#if !MPEG_H_SYNTAX
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessDeviationMax = LOUDNESS_DEVIATION_MAX_DEFAULT;
#else
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessDeviationMax = 0;
#endif
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementMethod = USER_METHOD_DEFINITION_DEFAULT;
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementSystem = USER_MEASUREMENT_SYSTEM_DEFAULT;
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementPreProc = USER_LOUDNESS_PREPROCESSING_DEFAULT;
        hUniDrcSelProcStruct->uniDrcSelProcParams.deviceCutOffFrequency = 500;
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainDbMax = LOUDNESS_NORMALIZATION_GAIN_MAX_DEFAULT; /* infinity as default */
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainModificationDb = 0.0f;
        hUniDrcSelProcStruct->uniDrcSelProcParams.outputPeakLevelMax = 0.0f;
        if (hUniDrcSelProcStruct->uniDrcSelProcParams.peakLimiterPresent == TRUE) {
#if !MPEG_H_SYNTAX
            hUniDrcSelProcStruct->uniDrcSelProcParams.outputPeakLevelMax = 6.0f;
#endif
        }
        
        /* dynamic range control parameters */
#ifdef AMD2_COR1
        hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn = FALSE;
#else
        hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn = TRUE;
#endif
        hUniDrcSelProcStruct->uniDrcSelProcParams.numBandsSupported = 4; /* assuming time domain DRC */
        hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcFeatureRequests = 0;
        
        /* other parameters */
        hUniDrcSelProcStruct->uniDrcSelProcParams.boost = 1.f;
        hUniDrcSelProcStruct->uniDrcSelProcParams.compress = 1.f;
        hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicTarget = 0;
        
        /* MPEG-H parameters */
#if MPEG_H_SYNTAX
        hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested = 0;
        hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested = 0;
#endif
#if MPEG_D_DRC_EXTENSION_V1
        hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessEqRequest = LOUD_EQ_REQUEST_OFF;
        hUniDrcSelProcStruct->uniDrcSelProcParams.eqSetPurposeRequest = EQ_PURPOSE_EQ_OFF;
        hUniDrcSelProcStruct->complexityLevelSupportedTotal = COMPLEXITY_LEVEL_SUPPORTED_TOTAL;
#endif
        
        /* user selected DRC */
        hUniDrcSelProcStruct->drcInstructionsIndexSelected = -1;
        hUniDrcSelProcStruct->drcCoefficientsIndexSelected = -1;
        hUniDrcSelProcStruct->downmixInstructionsIndexSelected = -1;
        
        /* subDrc */
        hUniDrcSelProcStruct->subDrc[0].drcInstructionsIndex = -1; /* DRC 0: Selected DRC */
        hUniDrcSelProcStruct->subDrc[1].drcInstructionsIndex = -1; /* DRC 1: Dependent DRC (DRC 0 depends on DRC 1) */
        hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = -1; /* DRC 2: Fading DRC */
        hUniDrcSelProcStruct->subDrc[3].drcInstructionsIndex = -1; /* DRC 3: Ducking DRC */
        
        /* output */
        hUniDrcSelProcStruct->uniDrcSelProcOutput.outputPeakLevelDb = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.loudnessNormalizationGainDb = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.outputLoudness = UNDEFINED_LOUDNESS_VALUE;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.numSelectedDrcSets = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.targetChannelCount = 0;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrixPresent = 0;
        
#if MPEG_H_SYNTAX
        /* group loudness */
        hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudnessCount = 0;
#endif
#if MPEG_D_DRC_EXTENSION_V1
        hUniDrcSelProcStruct->subEq[0].eqInstructionsIndex = -1; /* EQ 0: Selected EQ */
        hUniDrcSelProcStruct->subEq[1].eqInstructionsIndex = -1; /* EQ 1: Dependent EQ */
#endif
        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
    } else {
        return 1;
    }
    
    return 0;
}
    
int
uniDrcSelectionProcessInit_uniDrcSelProcParams(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                               HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams)
{
   
    if (hUniDrcSelProcStruct != NULL && hUniDrcSelProcParams != NULL) {
        
        /* NOTE: alternative API functions could be added to change single parameters if necessary */
        if ( memcmp ( &hUniDrcSelProcStruct->uniDrcSelProcParams, hUniDrcSelProcParams, sizeof(UniDrcSelProcParams) ))
        {
            /* request has changed */
            hUniDrcSelProcStruct->uniDrcSelProcParams = *hUniDrcSelProcParams;
            hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
        }
    }
    
    return 0;
}
    
int
uniDrcSelectionProcessInit_uniDrcInterfaceParams(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                                 HANDLE_UNI_DRC_INTERFACE hUniDrcInterface)
{
    int i,j;
    
    if (hUniDrcSelProcStruct != NULL && hUniDrcInterface != NULL) {
        
        /* systemInterface */
        if (hUniDrcInterface->systemInterfacePresent) {
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType != hUniDrcInterface->systemInterface.targetConfigRequestType) {
                hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType = hUniDrcInterface->systemInterface.targetConfigRequestType;
                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
            }
            switch (hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType) {
                case 0:
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests != hUniDrcInterface->systemInterface.numDownmixIdRequests) {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = hUniDrcInterface->systemInterface.numDownmixIdRequests;
                        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                    }
                    for (i=0; i<hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests; i++) {
                        if (hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[i] != hUniDrcInterface->systemInterface.downmixIdRequested[i]) {
                            hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[i] = hUniDrcInterface->systemInterface.downmixIdRequested[i];
                            hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                        }
                    }
                    break;
                case 1:
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetLayoutRequested != hUniDrcInterface->systemInterface.targetLayoutRequested) {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.targetLayoutRequested = hUniDrcInterface->systemInterface.targetLayoutRequested;
                        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                    }
                    break;
                case 2:
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountRequested != hUniDrcInterface->systemInterface.targetChannelCountRequested) {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountRequested = hUniDrcInterface->systemInterface.targetChannelCountRequested;
                        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                    }
                    break;
            }
        }
        /* loudnessNormalizationControlInterface */
        if (hUniDrcInterface->loudnessNormalizationControlInterfacePresent) {
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationOn != hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn) {
                hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationOn = hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn;
                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationOn) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetLoudness != hUniDrcInterface->loudnessNormalizationControlInterface.targetLoudness) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.targetLoudness = hUniDrcInterface->loudnessNormalizationControlInterface.targetLoudness;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
        }
        /* loudnessNormalizationParameterInterface */
        if (hUniDrcInterface->loudnessNormalizationParameterInterfacePresent) {
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.albumMode != hUniDrcInterface->loudnessNormalizationParameterInterface.albumMode) {
                hUniDrcSelProcStruct->uniDrcSelProcParams.albumMode = hUniDrcInterface->loudnessNormalizationParameterInterface.albumMode;
                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.peakLimiterPresent != hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent) {
                hUniDrcSelProcStruct->uniDrcSelProcParams.peakLimiterPresent = hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent;
                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessDeviationMax != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessDeviationMax = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementMethod != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementMethod = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementPreProc != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementPreProc = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementSystem != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessMeasurementSystem = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.deviceCutOffFrequency != hUniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.deviceCutOffFrequency = hUniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainDbMax != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainDbMax = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb ) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainModificationDb != hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainModificationDb = hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.outputPeakLevelMax != hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.outputPeakLevelMax = hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
        }
        /* dynamicRangeControlInterface */
        if (hUniDrcInterface->dynamicRangeControlInterfacePresent) {
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn != hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn) {
                hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn = hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn;
                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                if (!hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcFeatureRequests = 0;
                }
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeControlOn) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcFeatureRequests != hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcFeatureRequests = hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
                for (i=0; i<hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcFeatureRequests; i++) {
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.drcFeatureRequestType[i] != hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]) {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.drcFeatureRequestType[i] = hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i];
                        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                    }
                    switch (hUniDrcSelProcStruct->uniDrcSelProcParams.drcFeatureRequestType[i]) {
                        case 0:
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcEffectTypeRequests[i] != hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]) {
                                hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcEffectTypeRequests[i] = hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i];
                                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                            }
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcEffectTypeRequestsDesired[i] != hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[i]) {
                                hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcEffectTypeRequestsDesired[i] = hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[i];
                                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                            }
                            for (j=0; j<hUniDrcSelProcStruct->uniDrcSelProcParams.numDrcEffectTypeRequests[i]; j++) {
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.drcEffectTypeRequest[i][j] != hUniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[i][j]) {
                                    hUniDrcSelProcStruct->uniDrcSelProcParams.drcEffectTypeRequest[i][j] = hUniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[i][j];
                                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                                }
                            }
                            break;
                        case 1:
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeMeasurementRequestType[i] != hUniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[i]) {
                                hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeMeasurementRequestType[i] = hUniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[i];
                                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                            }
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestedIsRange[i] != hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i]) {
                                hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestedIsRange[i] = hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i];
                                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                            }
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestedIsRange[i] == 0) {
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValue[i] != hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[i]) {
                                    hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValue[i] = hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[i];
                                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                                }
                            } else {
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValueMin[i] != hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[i]) {
                                    hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValueMin[i] = hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[i];
                                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                                }
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValueMax[i] != hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[i]) {
                                    hUniDrcSelProcStruct->uniDrcSelProcParams.dynamicRangeRequestValueMax[i] = hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[i];
                                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                                }
                            }
                            break;
                        case 2:
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicRequest[i] != hUniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[i]) {
                                hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicRequest[i] = hUniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[i];
                                hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                            }
                            break;
                    }      
                }
            }
        }
        /* dynamicRangeControlParameterInterface */
        if (hUniDrcInterface->dynamicRangeControlParameterInterfacePresent) {
            if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.compress != hUniDrcInterface->dynamicRangeControlParameterInterface.compress) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.compress = hUniDrcInterface->dynamicRangeControlParameterInterface.compress;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.boost != hUniDrcInterface->dynamicRangeControlParameterInterface.boost) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.boost = hUniDrcInterface->dynamicRangeControlParameterInterface.boost;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
            if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget) {
                if (hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicTarget != hUniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicTarget = hUniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget;
                    hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
                }
            }
        }
#if MPEG_D_DRC_EXTENSION_V1
        if (hUniDrcInterface->uniDrcInterfaceExtensionPresent) {
            UniDrcInterfaceExtension* uniDrcInterfaceExtension = &hUniDrcInterface->uniDrcInterfaceExtension;
            if (uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent) {
                LoudnessEqParameterInterface* loudnessEqParameterInterface = &uniDrcInterfaceExtension->loudnessEqParameterInterface;
                if (loudnessEqParameterInterface->loudnessEqRequestPresent) {
                    hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessEqRequest = loudnessEqParameterInterface->loudnessEqRequest;
                    hUniDrcSelProcStruct->uniDrcSelProcParams.sensitivity = loudnessEqParameterInterface->sensitivity;
                    hUniDrcSelProcStruct->uniDrcSelProcParams.playbackGain = loudnessEqParameterInterface->playbackGain;
                }
            }
            if (uniDrcInterfaceExtension->equalizationControlInterfacePresent) {
                 hUniDrcSelProcStruct->uniDrcSelProcParams.eqSetPurposeRequest = uniDrcInterfaceExtension->equalizationControlInterface.eqSetPurposeRequest;
            }
        }
#endif

#ifdef LEVELING_SUPPORT
        if(hUniDrcInterface->uniDrcInterfaceExtension.levelingControlInterfacePresent) {
            hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessLevelingOn = hUniDrcInterface->uniDrcInterfaceExtension.levelingControlInterface.loudnessLevelingOn;
        }
#endif
    }
    
    return 0;
}
     
#ifdef __cplusplus
}
#endif /* __cplusplus */

