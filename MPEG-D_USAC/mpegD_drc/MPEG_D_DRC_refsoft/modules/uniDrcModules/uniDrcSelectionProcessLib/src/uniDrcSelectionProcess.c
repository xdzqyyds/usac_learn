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
#include <math.h>
#include <string.h>

#include "uniDrcSelectionProcess.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
   
/* open */
int
openUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT *phUniDrcSelProcStruct)
{
    int err = 0;
    UNI_DRC_SEL_PROC_STRUCT *hUniDrcSelProcStruct = NULL;
    hUniDrcSelProcStruct = (HANDLE_UNI_DRC_SEL_PROC_STRUCT)calloc(1,sizeof(struct T_UNI_DRC_SEL_PROC_STRUCT));
    
    hUniDrcSelProcStruct->firstFrame = 1;
    hUniDrcSelProcStruct->uniDrcConfigHasChanged = 0;
    hUniDrcSelProcStruct->loudnessInfoSetHasChanged = 0;
    
    *phUniDrcSelProcStruct = hUniDrcSelProcStruct;
    
    return err;
}
    
/* init */
int
initUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                           HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
                           HANDLE_UNI_DRC_INTERFACE hUniDrcInterface,
                           const int subbandDomainMode)
{
    int err = 0;
    
    /* error check */
    if (hUniDrcSelProcStruct == NULL) {
        return 1;
    }
    
    /* init params for first frame */
    if (hUniDrcSelProcStruct->firstFrame == 1) {
        /* default params */
        err = uniDrcSelectionProcessInit_defaultParams(hUniDrcSelProcStruct);
        if (err) return (err);
    }
    
    /* selection process params (if available) */
    /* NOTE: alternative API functions could be added to change single parameters if necessary */
    err = uniDrcSelectionProcessInit_uniDrcSelProcParams(hUniDrcSelProcStruct,
                                                         hUniDrcSelProcParams);
    if (err) return (err);
#if MPEG_D_DRC_EXTENSION_V1
    {
        int i;
        hUniDrcSelProcStruct->drcSetIdIsPermitted[0] = 1;
        for (i=1; i<DRC_INSTRUCTIONS_COUNT_MAX; i++)
        {
            hUniDrcSelProcStruct->drcSetIdIsPermitted[i] = 0;
        }
        
        hUniDrcSelProcStruct->eqSetIdIsPermitted[0] = 1;
        for (i=1; i<EQ_INSTRUCTIONS_COUNT_MAX; i++)
        {
            hUniDrcSelProcStruct->eqSetIdIsPermitted[i] = 0;
        }
    }
#endif
    /* uniDrcInterface params (if available) */
    err = uniDrcSelectionProcessInit_uniDrcInterfaceParams(hUniDrcSelProcStruct,
                                                           hUniDrcInterface);
    if (err) return (err);
    
    /* subband domain mode */
    hUniDrcSelProcStruct->subbandDomainMode = subbandDomainMode;

    return 0;
}

   
#if MPEG_H_SYNTAX
int
setMpeghParamsUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                       int numGroupIdsRequested,
                                       int *groupIdRequested,
                                       int numGroupPresetIdsRequested,
                                       int *groupPresetIdRequested,
                                       int *numMembersGroupPresetIdsRequested,
                                       int groupPresetIdRequestedPreference)
{
    
    int i;
    
    /* groupId request */
    if (hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested != numGroupIdsRequested)
    {
        hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested = numGroupIdsRequested;
        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
    }
    for (i=0; i<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested; i++)
    {
        if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupIdRequested[i] != groupIdRequested[i])
        {
            hUniDrcSelProcStruct->uniDrcSelProcParams.groupIdRequested[i] = groupIdRequested[i];
            hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
        }
    }
    
    /* groupPresetId request */
    if (hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested != numGroupPresetIdsRequested)
    {
        hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested = numGroupPresetIdsRequested;
        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
    }
    for (i=0; i<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested; i++)
    {
        if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequested[i] != groupPresetIdRequested[i])
        {
            hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequested[i] = groupPresetIdRequested[i];
            hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
        }
        if (hUniDrcSelProcStruct->uniDrcSelProcParams.numMembersGroupPresetIdsRequested[i] != numMembersGroupPresetIdsRequested[i])
        {
            hUniDrcSelProcStruct->uniDrcSelProcParams.numMembersGroupPresetIdsRequested[i] = numMembersGroupPresetIdsRequested[i];
            hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
        }
    }
    if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequestedPreference != groupPresetIdRequestedPreference)
    {
        hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequestedPreference = groupPresetIdRequestedPreference;
        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 1;
    }
    
    return 0;
}
#endif
    
/* process */
int
processUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                              HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                              HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                              HANDLE_UNI_DRC_SEL_PROC_OUTPUT hUniDrcSelProcOutput)
{
    int i, err, drcSetIdSelected, activeDrcSetIndex;
#if MPEG_D_DRC_EXTENSION_V1
    int eqSetIdSelected;
    int loudEqSetIdSelected;
#endif
    
    if (hUniDrcConfig != NULL)
    {
        if ( memcmp ( &hUniDrcSelProcStruct->uniDrcConfig, hUniDrcConfig, sizeof(UniDrcConfig) ))
        {
            /* UniDrcConfig has changed */
            hUniDrcSelProcStruct->uniDrcConfig = *hUniDrcConfig;
            hUniDrcSelProcStruct->uniDrcConfigHasChanged = 1;
            
            /* check if baseChannelCount and/or baseLayout are defined in bitstream */
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount != hUniDrcSelProcStruct->uniDrcConfig.channelLayout.baseChannelCount)
            {
#if DEBUG_WARNINGS
                fprintf(stderr,"WARNING: Configured baseChannelCount differs from baseChannelCount in bitstream. Overwriting parameter ...\n");
#endif
                hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount = hUniDrcSelProcStruct->uniDrcConfig.channelLayout.baseChannelCount;
            }
            if (hUniDrcSelProcStruct->uniDrcConfig.channelLayout.layoutSignalingPresent == 1 && hUniDrcSelProcStruct->uniDrcSelProcParams.baseLayout != hUniDrcSelProcStruct->uniDrcConfig.channelLayout.definedLayout)
            {
#if DEBUG_WARNINGS
                fprintf(stderr,"WARNING: Configured baseLayout differs from baseLayout in bitstream. Overwriting parameter ...\n");
#endif
                hUniDrcSelProcStruct->uniDrcSelProcParams.baseLayout = hUniDrcSelProcStruct->uniDrcConfig.channelLayout.definedLayout;
            }
        }
        else
        {
            hUniDrcSelProcStruct->uniDrcConfigHasChanged = 0;
        }
    }
    if (hLoudnessInfoSet != NULL)
    {
        if ( memcmp ( &hUniDrcSelProcStruct->loudnessInfoSet, hLoudnessInfoSet, sizeof(LoudnessInfoSet) ))
        {
            /* LoudnessInfoSet has changed */
            hUniDrcSelProcStruct->loudnessInfoSet = *hLoudnessInfoSet;
            hUniDrcSelProcStruct->loudnessInfoSetHasChanged = 1;
        }
        else
        {
            hUniDrcSelProcStruct->loudnessInfoSetHasChanged = 0;
        }
    }

    /* map targetLayoutRequested or targetChannelCountRequested to downmixIdRequested */
    if ((hUniDrcSelProcStruct->uniDrcConfigHasChanged && hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType != 0) ||
        (hUniDrcSelProcStruct->selectionProcessRequestHasChanged && hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType != 0) ||
        (hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType == 0 && hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0))
    {
        err = mapTargetConfigRequestToDownmixId(hUniDrcSelProcStruct,
                                                &hUniDrcSelProcStruct->uniDrcConfig);
        if (err) return (err);
    }
    
#if MPEG_H_SYNTAX
    /* virtual DRC instuctions for MPEG-H 3DA for downmixIdRequested != 0 */
    if (hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] != 0) {
        hUniDrcSelProcStruct->uniDrcConfig.downmixInstructions[0].downmixId = hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0];
        hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus-1].downmixId[0] = hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0];
    } else {
        hUniDrcSelProcStruct->uniDrcConfig.downmixInstructions[0].downmixId = -1;
        hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus-1].downmixId[0] = -1;
    }
#endif
    
    /* only process for changed uniDrcConfig() and/or changed loudnessInfoSet() and/or changed selectionProcessRequest */
    if (hUniDrcSelProcStruct->uniDrcConfigHasChanged || hUniDrcSelProcStruct->loudnessInfoSetHasChanged || hUniDrcSelProcStruct->selectionProcessRequestHasChanged) {

#if MPEG_D_DRC_EXTENSION_V1
        int repeatSelection = 1;
        int loopCount = 0;
        
        err = manageDrcComplexity (hUniDrcSelProcStruct, hUniDrcConfig);
        if (err) return(err);
        err = manageEqComplexity (hUniDrcSelProcStruct, hUniDrcConfig);
        if (err) return(err);
        while (repeatSelection==1)
        {
#endif
            err = selectDrcSet(hUniDrcSelProcStruct,
                               &drcSetIdSelected
#if MPEG_D_DRC_EXTENSION_V1
                               , &eqSetIdSelected
                               , &loudEqSetIdSelected
#endif
                               );
            if (err) return (err);
            
            err = getSelectedDrcSet(hUniDrcSelProcStruct,
                                    drcSetIdSelected);
            if (err) return (err);
            
            err = getDependentDrcSet(hUniDrcSelProcStruct);
            if (err) return (err);
            
            err = getFadingDrcSet(hUniDrcSelProcStruct);
            if (err) return (err);
            
            err = getDuckingDrcSet(hUniDrcSelProcStruct);
            if (err) return (err);
            
#if MPEG_D_DRC_EXTENSION_V1
            hUniDrcSelProcStruct->subEq[0].eqInstructionsIndex = -1;
            hUniDrcSelProcStruct->subEq[1].eqInstructionsIndex = -1;
            
            err = getSelectedEqSet(hUniDrcSelProcStruct, eqSetIdSelected);
            if (err) return (err);
            
            err = getDependentEqSet(hUniDrcSelProcStruct);
            if (err) return (err);
            
            err = getSelectedLoudEqSet(hUniDrcSelProcStruct, loudEqSetIdSelected);
            if (err) return (err);
#endif
        
            /* implement DRC set order according to Section 6.3.5 of ISO/IEC 23003-4: Ducking/Fading -> Dependent -> Selected */
            activeDrcSetIndex = 0;
            for (i=SUB_DRC_COUNT-1; i>=0; i--) {  /* count reverse */
                int drcInstructionsIndex = hUniDrcSelProcStruct->subDrc[i].drcInstructionsIndex;
#if AMD2_COR3
                if (drcInstructionsIndex >= 0)
#endif
                {
                    DrcInstructionsUniDrc drcInstructionsUniDrc;

                    drcInstructionsUniDrc = hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[drcInstructionsIndex];
#if AMD2_COR3
                    if (drcInstructionsUniDrc.drcSetId > 0) /* exclude virtual DRC sets with drcSetId < 0 */
#else
                    if (drcInstructionsIndex >= 0 && drcInstructionsUniDrc.drcSetId > 0) /* exclude virtual DRC sets with drcSetId < 0 */
#endif
                    {
                        hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[activeDrcSetIndex] = drcInstructionsUniDrc.drcSetId;

                        /* force ducking DRC set to be processed on base layout */
                        if ((i==3) && (drcInstructionsUniDrc.drcSetEffect & (EFFECT_BIT_DUCK_SELF | EFFECT_BIT_DUCK_OTHER))) { /* TODO: second condition necessary? drcInstructionsIndex from subDrc[3] can only be ducking */
                            hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDownmixIds[activeDrcSetIndex] = 0;
                        }
                        else {
                            if (drcInstructionsUniDrc.drcApplyToDownmix == 1) {
                                hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDownmixIds[activeDrcSetIndex] = drcInstructionsUniDrc.downmixId[0];
                            } else {
                                hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDownmixIds[activeDrcSetIndex] = 0;
                            }
                        }

                        activeDrcSetIndex++;
                    }
                }
            }
            if (activeDrcSetIndex <= 3)
            {
                hUniDrcSelProcStruct->uniDrcSelProcOutput.numSelectedDrcSets = activeDrcSetIndex;
            } else {
                fprintf(stderr, "WARNING: More than three selected DRC sets are not allowed.\n");
                hUniDrcSelProcStruct->uniDrcSelProcOutput.numSelectedDrcSets = -1;
                return(UNEXPECTED_ERROR);
            }
            
            /* select downmix matrix */
            err = selectDownmixMatrix(hUniDrcSelProcStruct,
                                      &hUniDrcSelProcStruct->uniDrcConfig);
            if (err) return (err);
            
#if MPEG_D_DRC_EXTENSION_V1
            err = manageComplexity (hUniDrcSelProcStruct, hUniDrcConfig, &repeatSelection);
            if (err) return(err);
            
            loopCount++;
            if (loopCount > 100)
            {
                fprintf(stderr, "ERROR: selection process takes more iterations than expected %d.\n", loopCount);
                return (UNEXPECTED_ERROR);
            }
        }
#endif

#if DEBUG_DRC_SELECTION
#if MPEG_D_DRC_EXTENSION_V1
        if (loopCount>1)
        {
            printf("Used %d iterations for selection.\n", loopCount);
        }
#endif
        displaySelectedDrcSets(hUniDrcSelProcStruct);
        displaySelectedDownmixMatrix(hUniDrcSelProcStruct,
                                     &hUniDrcSelProcStruct->uniDrcConfig);
#endif
        hUniDrcSelProcStruct->selectionProcessRequestHasChanged = 0;
        /* transfer DRC decoder user parameters */
        hUniDrcSelProcStruct->uniDrcSelProcOutput.boost = hUniDrcSelProcStruct->uniDrcSelProcParams.boost;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.compress = hUniDrcSelProcStruct->uniDrcSelProcParams.compress;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.drcCharacteristicTarget = hUniDrcSelProcStruct->uniDrcSelProcParams.drcCharacteristicTarget;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.loudnessNormalizationGainDb += hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessNormalizationGainModificationDb;
        
#if MPEG_H_SYNTAX
        {
            int m;
            LoudnessInfo* loudnessInfo = NULL;
            hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudnessCount = 0;
            for (i=0; i<hUniDrcSelProcStruct->loudnessInfoSet.loudnessInfoCount; i++) {
                if (ID_FOR_BASE_LAYOUT == hUniDrcSelProcStruct->loudnessInfoSet.loudnessInfo[i].downmixId) /* base layout */
                {
                    if (0 == hUniDrcSelProcStruct->loudnessInfoSet.loudnessInfo[i].drcSetId) /* independent of drcSetID */
                    {
                        if (hUniDrcSelProcStruct->loudnessInfoSet.loudnessInfo[i].loudnessInfoType == 1) { /* group loudness */
                            
                            loudnessInfo = &hUniDrcSelProcStruct->loudnessInfoSet.loudnessInfo[i];
                            for (m=0; m<loudnessInfo->measurementCount; m++)
                            {
                                /* assumption: program and anchor loudness are equivalent for groups */
                                if ((loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_PROGRAM_LOUDNESS) ||
                                    (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_ANCHOR_LOUDNESS))
                                {
                                    hUniDrcSelProcStruct->uniDrcSelProcOutput.groupId[hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudnessCount] = loudnessInfo->mae_groupID;
                                    hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudness[hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudnessCount] = loudnessInfo->loudnessMeasure[m].methodValue;
                                    hUniDrcSelProcStruct->uniDrcSelProcOutput.groupIdLoudnessCount +=1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
#endif
#if MPEG_D_DRC_EXTENSION_V1
        for (i=0; i<2; i++)  /* for NUM_GAIN_DEC_INSTANCES */
        {
            hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedEqSetIds[i] = hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[hUniDrcSelProcStruct->subEq[i].eqInstructionsIndex].eqSetId;
        }
        if (hUniDrcSelProcStruct->loudEqInstructionsIndexSelected >= 0)
        {
            hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedLoudEqId = hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructions[hUniDrcSelProcStruct->loudEqInstructionsIndexSelected].loudEqSetId;
        }
        else
        {
            hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedLoudEqId = 0;
        }
#endif
    }
    
    /* return selection process output */
    *hUniDrcSelProcOutput = hUniDrcSelProcStruct->uniDrcSelProcOutput;
    
    return 0;
}
    
/* close */
int closeUniDrcSelectionProcess(HANDLE_UNI_DRC_SEL_PROC_STRUCT *phUniDrcSelProcStruct)
{
    if ( *phUniDrcSelProcStruct != NULL )
    {
        free( *phUniDrcSelProcStruct );
        *phUniDrcSelProcStruct = NULL;
    }
    return 0;
}

/* map target config request to downmixId */
int mapTargetConfigRequestToDownmixId(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                      HANDLE_UNI_DRC_CONFIG hUniDrcConfig)
{
    int i, downmixInstructionsCount;
    int targetChannelCountPrelim = hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount;
    
    hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 0;
    switch (hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType) {
        case 0:
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0)
            {
                hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] = 0;
                hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 1;
            }
            break;
        case 1:
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetLayoutRequested == hUniDrcSelProcStruct->uniDrcSelProcParams.baseLayout)
            {
                hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] = 0;
                hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 1;
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0) {
                downmixInstructionsCount = hUniDrcSelProcStruct->uniDrcConfig.downmixInstructionsCount;
                for (i=0; i<downmixInstructionsCount; i++)
                {
                    DownmixInstructions* downmixInstructions = &(hUniDrcConfig->downmixInstructions[i]);
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetLayoutRequested == downmixInstructions->targetLayout)
                    {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests] = downmixInstructions->downmixId;
                        hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests += 1;
                        targetChannelCountPrelim = downmixInstructions->targetChannelCount;
                    }
                }
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.baseLayout == -1)
            {
                fprintf(stderr,"WARNING: BaseLayout CICP index is undefined.\n");
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0)
            {
                fprintf(stderr,"WARNING: downmixId based on targetLayout CICP index not found. Requested downmixId is set to 0x0\n");
                hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] = 0;
                hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 1;
            }
            break;
        case 2:
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountRequested == hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount)
            {
                hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] = 0;
                hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 1;
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0)
            {
                downmixInstructionsCount = hUniDrcSelProcStruct->uniDrcConfig.downmixInstructionsCount;
                for (i=0; i<downmixInstructionsCount; i++)
                {
                    DownmixInstructions* downmixInstructions = &(hUniDrcConfig->downmixInstructions[i]);
                    if (hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountRequested == downmixInstructions->targetChannelCount) {
                        hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests] = downmixInstructions->downmixId;
                        hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests += 1;
                        targetChannelCountPrelim = downmixInstructions->targetChannelCount;
                    }
                }
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount == -1)
            {
                fprintf(stderr,"WARNING: BaseChannelCount is undefined.\n");
            }
            if (hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests == 0)
            {
                fprintf(stderr,"WARNING: downmixId based on targetChannelCount not found. Requested downmixId is set to 0x0\n");
                hUniDrcSelProcStruct->uniDrcSelProcParams.downmixIdRequested[0] = 0;
                hUniDrcSelProcStruct->uniDrcSelProcParams.numDownmixIdRequests = 1;
            }
            break;
        default:
            fprintf(stderr,"WARNING: Mapping of targetConfigRequestType = %d is not supported!\n",hUniDrcSelProcStruct->uniDrcSelProcParams.targetConfigRequestType);
            return UNEXPECTED_ERROR;
            break;
    }
    hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountPrelim = targetChannelCountPrelim;

    return 0;
}

/* select downmix matrix */
int selectDownmixMatrix(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                        HANDLE_UNI_DRC_CONFIG hUniDrcConfig)
{
    int i, j, n;
    
    hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount  = hUniDrcConfig->channelLayout.baseChannelCount;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.targetChannelCount = hUniDrcConfig->channelLayout.baseChannelCount;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.targetLayout = -1;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrixPresent = 0;
    hUniDrcSelProcStruct->downmixInstructionsIndexSelected = -1;
    
    if (hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId != 0) {
#if MPEG_H_SYNTAX
        for (n=0; n<hUniDrcConfig->mpegh3daDownmixInstructionsCount; n++) {
            DownmixInstructions* downmixInstructions = &(hUniDrcConfig->mpegh3daDownmixInstructions[n]);
#else
        for (n=0; n<hUniDrcConfig->downmixInstructionsCount; n++) {
            DownmixInstructions* downmixInstructions = &(hUniDrcConfig->downmixInstructions[n]);
#endif
            
            if (hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId == downmixInstructions->downmixId) {
                hUniDrcSelProcStruct->downmixInstructionsIndexSelected = n;
                hUniDrcSelProcStruct->uniDrcSelProcOutput.targetChannelCount = downmixInstructions->targetChannelCount;
                hUniDrcSelProcStruct->uniDrcSelProcOutput.targetLayout = downmixInstructions->targetLayout;
                if (downmixInstructions->downmixCoefficientsPresent) {
                    for( i =0 ; i < hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount; i++)
                    {
                        for( j =0 ; j < hUniDrcSelProcStruct->uniDrcSelProcOutput.targetChannelCount; j++)
                        {
                            hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrix[i][j] = downmixInstructions->downmixCoefficient[i+j*hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount];
                        }
                    }
                    hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrixPresent = 1;
                }
                break;
            }
        }
    }
    return 0;
}

#if DEBUG_DRC_SELECTION
/* display selected downmix matrix */
int displaySelectedDownmixMatrix(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                 HANDLE_UNI_DRC_CONFIG hUniDrcConfig)
{
    
    int i, j, k, targetChannelCountTmp;

    printf("Available downmix matrices (format: [inChans x outChans], linear gain):\n");
    for (i=0; i<hUniDrcConfig->downmixInstructionsCount; i++) {
        printf("     dmxMtx #%d: downmixId = %d, targetLayout = %d, targetChannelCount = %d\n", i, hUniDrcConfig->downmixInstructions[i].downmixId,hUniDrcConfig->downmixInstructions[i].targetLayout,hUniDrcConfig->downmixInstructions[i].targetChannelCount);
        
        targetChannelCountTmp = hUniDrcConfig->downmixInstructions[i].targetChannelCount;
        if (hUniDrcConfig->downmixInstructions[i].downmixCoefficientsPresent)
        {
            for( j =0 ; j < hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount; j++)
            {
                printf("          ");
                for( k =0 ; k < targetChannelCountTmp; k++)
                {
                    printf("%2.4f ", hUniDrcConfig->downmixInstructions[i].downmixCoefficient[j+k*hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount]);
                }
                printf("\n");
            }
        }
    }
    printf("-----------------------------------------\n");
    if (hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrixPresent == 0)
    {
        printf("Active dowmixId = %d\n",hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId);
        printf("     downmixCoefficientsPresent=0\n");
        
    } else {
        if (hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrixPresent)
        {
            printf("Active dowmixMatrix for downmixId = %d:\n",hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId);
            for( i =0 ; i < hUniDrcSelProcStruct->uniDrcSelProcOutput.baseChannelCount; i++)
            {
                printf("          ");
                for( j =0 ; j < hUniDrcSelProcStruct->uniDrcSelProcOutput.targetChannelCount; j++)
                {
                    printf("%2.4f ", hUniDrcSelProcStruct->uniDrcSelProcOutput.downmixMatrix[i][j]);
                }
                printf("\n");
            }
        }
    }
    printf("=========================================\n");
    
    return 0;
}
#endif

int
getMultiBandDrcPresent(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       int                   numSets[3],
                       int                   multiBandDrcPresent[3])
{
    int err=0, k, m;
    for(k=0; k<hUniDrcConfig->drcInstructionsUniDrcCount; k++)
    {
        if ((hUniDrcConfig->drcInstructionsUniDrc[k].downmixId[0] == ID_FOR_BASE_LAYOUT) || (hUniDrcConfig->drcInstructionsUniDrc[k].drcApplyToDownmix == 0))
        {
            numSets[0]++;
        }
        else if (hUniDrcConfig->drcInstructionsUniDrc[k].downmixId[0] == ID_FOR_ANY_DOWNMIX)
        {
            numSets[1]++;
        }
        else
        {
            numSets[2]++;
        }
        for (m=0; m<hUniDrcConfig->drcInstructionsUniDrc[k].nDrcChannelGroups; m++)
        {
#ifdef AMD2_COR1
            if (hUniDrcConfig->drcCoefficientsUniDrc[0].gainSetParams[hUniDrcConfig->drcInstructionsUniDrc[k].gainSetIndexForChannelGroup[m]].bandCount > 1)
#else
            if (hUniDrcConfig->drcCoefficientsUniDrc[0].gainSetParams[hUniDrcConfig->drcInstructionsUniDrc->gainSetIndexForChannelGroup[m]].bandCount > 1)
#endif
            {
                if ((hUniDrcConfig->drcInstructionsUniDrc[k].downmixId[0] == ID_FOR_BASE_LAYOUT) || (hUniDrcConfig->drcInstructionsUniDrc[k].drcApplyToDownmix == 0))
                {
                    multiBandDrcPresent[0] = 1;
                }
                else if (hUniDrcConfig->drcInstructionsUniDrc[k].downmixId[0] == ID_FOR_ANY_DOWNMIX)
                {
                    multiBandDrcPresent[1] = 1;
                }
                else
                {
                    multiBandDrcPresent[2] = 1;
                }
            }
        }
    }
    return err;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

