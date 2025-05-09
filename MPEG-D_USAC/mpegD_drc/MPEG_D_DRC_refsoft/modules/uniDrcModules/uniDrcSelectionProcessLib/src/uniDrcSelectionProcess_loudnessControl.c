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
#include <math.h>

#include "uniDrcSelectionProcess.h"
#include "uniDrcSelectionProcess_loudnessControl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* NOTE: For MPEG-H 3DA, the result of getSignalPeakLevel() respresents only an approximation and not an accurate signal peak level. */
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
                   int explicitPeakInformationPresent[])
{
    int c, d, i, k, n, baseChannelCount, mode;
    int prelimCount;
    int peakCount = 0;
    int loudnessInfoCount = 0;
    LoudnessInfo* loudnessInfo;
    float sum, maxSum;
    int drcSetIdRequested = drcInstructionsUniDrc->drcSetId;
    int loudnessDrcSetIdRequested;
    int matchFound = FALSE;

    float signalPeakLevelTmp;
    eqSetId[0] = 0;
    signalPeakLevel[0] = 0.0f;
    explicitPeakInformationPresent[0] = 0;
    
    k=0;
    if (drcSetIdRequested < 0)
    {
        for (k=0; k<noCompressionEqCount; k++)
        {
            eqSetId[k] = noCompressionEqId[k];
            signalPeakLevel[k] = 0.0f;
            explicitPeakInformationPresent[k] = FALSE;
        }
    }
    eqSetId[k] = 0;
    signalPeakLevel[k] = 0.0f;
    explicitPeakInformationPresent[k] = FALSE;
    k++;
    
    prelimCount = k;

    if (drcSetIdRequested < 0)
    {
        loudnessDrcSetIdRequested = 0;
    }
    else
    {
        loudnessDrcSetIdRequested = drcSetIdRequested;
    }

    if (albumMode == TRUE)
    {
        mode = 1;
        loudnessInfoCount = hLoudnessInfoSet->loudnessInfoAlbumCount;
    }
    else
    {
        mode = 0;
        loudnessInfoCount = hLoudnessInfoSet->loudnessInfoCount;
    }
    
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
    if (loudnessDrcSetIdRequested == 0) {
        for (n=0; n<loudnessInfoCount; n++)
        {
            if (mode == 1)
            {
                loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
            }
            else
            {
                loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
            }
            if (loudnessDrcSetIdRequested == loudnessInfo->drcSetId && downmixIdRequested == loudnessInfo->downmixId)
            {
                if (loudnessInfo->truePeakLevelPresent)
                {
#if MPEG_D_DRC_EXTENSION_V1
                    eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                    eqSetId[peakCount] = 0;
#endif
                    if (loudnessInfo->loudnessInfoType == 3 && hUniDrcSelProcParams->groupPresetIdRequestedPreference == loudnessInfo->mae_groupPresetID) {
                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                        explicitPeakInformationPresent[peakCount] = 1;
                        matchFound = TRUE;
                        peakCount++;
                    }
                    if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 3) {
                        for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                            if (hUniDrcSelProcParams->groupPresetIdRequested[i] == loudnessInfo->mae_groupPresetID) {
                                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                                explicitPeakInformationPresent[peakCount] = 1;
                                matchFound = TRUE;
                                peakCount++;
                                break;
                            }
                        }
                    }
                    if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 2) {
                        for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                            if (hUniDrcSelProcParams->groupIdRequested[i] == loudnessInfo->mae_groupID) {
                                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                                explicitPeakInformationPresent[peakCount] = 1;
                                matchFound = TRUE;
                                peakCount++;
                                break;
                            }
                        }
                    }
                }
                if (matchFound == FALSE) {
                    if (loudnessInfo->samplePeakLevelPresent)
                    {
#if MPEG_D_DRC_EXTENSION_V1
                        eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                        eqSetId[peakCount] = 0;
#endif
                        if (loudnessInfo->loudnessInfoType == 3 && hUniDrcSelProcParams->groupPresetIdRequestedPreference == loudnessInfo->mae_groupPresetID) {
                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                            explicitPeakInformationPresent[peakCount] = 1;
                            matchFound = TRUE;
                            peakCount++;
                        }
                        if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 3) {
                            for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                                if (hUniDrcSelProcParams->groupPresetIdRequested[i] == loudnessInfo->mae_groupPresetID) {
                                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                                    explicitPeakInformationPresent[peakCount] = 1;
                                    matchFound = TRUE;
                                    peakCount++;
                                    break;
                                }
                            }
                        }
                        if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 2) {
                            for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                                if (hUniDrcSelProcParams->groupIdRequested[i] == loudnessInfo->mae_groupID) {
                                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                                    explicitPeakInformationPresent[peakCount] = 1;
                                    matchFound = TRUE;
                                    peakCount++;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (matchFound == FALSE) {
#endif
#endif
    for (n=0; n<loudnessInfoCount; n++)
    {
        if (mode == 1)
        {
            loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
        }
        else
        {
            loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
        }
        if (loudnessDrcSetIdRequested == loudnessInfo->drcSetId && downmixIdRequested == loudnessInfo->downmixId)
        {
            if (loudnessInfo->truePeakLevelPresent)
            {
#if MPEG_D_DRC_EXTENSION_V1
                eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                explicitPeakInformationPresent[peakCount] = 1;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                explicitPeakInformationPresent[peakCount] = 1;
#endif
                matchFound = TRUE;
                peakCount++;
#else
                if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                    (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                    (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                    signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                    explicitPeakInformationPresent[peakCount] = 1;
#endif
                    matchFound = TRUE;
                    peakCount++;
                }
#endif
            }
            if (matchFound == FALSE) {
                if (loudnessInfo->samplePeakLevelPresent)
                {
#if MPEG_D_DRC_EXTENSION_V1
                    eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                    eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                    explicitPeakInformationPresent[peakCount] = 1;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                    explicitPeakInformationPresent[peakCount] = 1;
#endif
                    matchFound = TRUE;
                    peakCount++;
#else
                    if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                        explicitPeakInformationPresent[peakCount] = 1;
#endif
                        matchFound = TRUE;
                        peakCount++;
                    }
#endif
                }
            }
            /*                break; */
        }
    }
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
    }
#endif
#endif
    if (matchFound == FALSE) {
        for (n=0; n<loudnessInfoCount; n++)
        {
            if (mode == 1)
            {
                loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
            }
            else
            {
                loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
            }
            if (ID_FOR_ANY_DRC == loudnessInfo->drcSetId && downmixIdRequested == loudnessInfo->downmixId)
            {
                if (loudnessInfo->truePeakLevelPresent)
                {
#if MPEG_D_DRC_EXTENSION_V1
                    eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                    eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                    signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                    explicitPeakInformationPresent[peakCount] = 1;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                    signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                    explicitPeakInformationPresent[peakCount] = 1;
#endif
                    matchFound = TRUE;
                    peakCount++;
#else
                    if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel;
                        explicitPeakInformationPresent[peakCount] = 1;
#endif
                        matchFound = TRUE;
                        peakCount++;
                    }
#endif
                }
                if (matchFound == FALSE) {
                    if (loudnessInfo->samplePeakLevelPresent)
                    {
#if MPEG_D_DRC_EXTENSION_V1
                        eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                        eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                        explicitPeakInformationPresent[peakCount] = 1;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                        explicitPeakInformationPresent[peakCount] = 1;
#endif
                        matchFound = TRUE;
                        peakCount++;
#else
                        if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                            (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                            (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel;
                            explicitPeakInformationPresent[peakCount] = 1;
#endif
                            matchFound = TRUE;
                            peakCount++;
                        }
#endif
                    }
                }
                /*                break; */
            }
        }
    }
    if (matchFound == FALSE) {
        if (downmixIdRequested == drcInstructionsUniDrc->downmixId[0] || ID_FOR_ANY_DOWNMIX == drcInstructionsUniDrc->downmixId[0])
        {
            if (drcInstructionsUniDrc->limiterPeakTargetPresent)
            {
#if MPEG_D_DRC_EXTENSION_V1
                if (drcInstructionsUniDrc->requiresEq == 1)
                {
                    for (d=0; d<hUniDrcConfig->uniDrcConfigExt.eqInstructionsCount; d++) {
                        EqInstructions* eqInstructions = &hUniDrcConfig->uniDrcConfigExt.eqInstructions[d];
                        for (c=0; c<eqInstructions->drcSetIdCount; c++) {
                            if ((eqInstructions->drcSetId[c] == loudnessDrcSetIdRequested) || (eqInstructions->drcSetId[c] == ID_FOR_ANY_DRC)) {
                                for (i=0; i<eqInstructions->downmixIdCount; i++) {
                                    if ((eqInstructions->downmixId[i] == downmixIdRequested) || (eqInstructions->downmixId[i] == ID_FOR_ANY_DOWNMIX)) {
                                        eqSetId[peakCount] = eqInstructions->eqSetId;
                                        signalPeakLevel[peakCount] = drcInstructionsUniDrc->limiterPeakTarget;
                                        explicitPeakInformationPresent[peakCount] = 1;
                                        matchFound = TRUE;
                                        peakCount++;
                                    }
                                }
                            }
                        }
                    }
                }
                else
#endif
                {
                    eqSetId[peakCount] = 0;
                    signalPeakLevel[peakCount] = drcInstructionsUniDrc->limiterPeakTarget;
                    explicitPeakInformationPresent[peakCount] = 1;
                    matchFound = TRUE;
                    peakCount++;
                }
            }
        }
        if (matchFound == FALSE) {
            /* TODO: can we start the loop at 0 and remove the coded above this? */
            for (i=1; i<drcInstructionsUniDrc->downmixIdCount; i++)
            {
                if (downmixIdRequested == drcInstructionsUniDrc->downmixId[i])
                {
                    if (drcInstructionsUniDrc->limiterPeakTargetPresent)
                    {
#if MPEG_D_DRC_EXTENSION_V1
                        if (drcInstructionsUniDrc->requiresEq == 1)
                        {
                            for (d=0; d<hUniDrcConfig->uniDrcConfigExt.eqInstructionsCount; d++) {
                                EqInstructions* eqInstructions = &hUniDrcConfig->uniDrcConfigExt.eqInstructions[d];
                                for (c=0; c<eqInstructions->drcSetIdCount; c++) {
                                    if ((eqInstructions->drcSetId[c] == loudnessDrcSetIdRequested) || (eqInstructions->drcSetId[c] == ID_FOR_ANY_DRC)) {
                                        for (k=0; k<eqInstructions->downmixIdCount; k++) {
                                            if ((eqInstructions->downmixId[k] == downmixIdRequested) || (eqInstructions->downmixId[k] == ID_FOR_ANY_DOWNMIX)) {
                                                eqSetId[peakCount] = eqInstructions->eqSetId;
                                                signalPeakLevel[peakCount] = drcInstructionsUniDrc->limiterPeakTarget;
                                                explicitPeakInformationPresent[peakCount] = 1;
                                                matchFound = TRUE;
                                                peakCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
#endif
                        {
                            eqSetId[peakCount] = 0;
                            signalPeakLevel[peakCount] = drcInstructionsUniDrc->limiterPeakTarget;
                            explicitPeakInformationPresent[peakCount] = 1;
                            matchFound = TRUE;
                            peakCount++;
                        }
                    }
                }
            }
        }
    }
    if (matchFound == FALSE) {
        if (downmixIdRequested != ID_FOR_BASE_LAYOUT) {
            signalPeakLevelTmp = 0.f;
            for (i=0; i<hUniDrcConfig->downmixInstructionsCount; i++)
            {
                if (hUniDrcConfig->downmixInstructions[i].downmixId == downmixIdRequested)
                {
                    if (hUniDrcConfig->downmixInstructions[i].downmixCoefficientsPresent)
                    {
                        baseChannelCount = hUniDrcConfig->channelLayout.baseChannelCount;
                        maxSum = 0.0f;
                        for (c=0; c<hUniDrcConfig->downmixInstructions[i].targetChannelCount; c++)
                        {
                            sum = 0.0f;
                            for (d=0; d<baseChannelCount; d++)
                            {
                                sum += hUniDrcConfig->downmixInstructions[i].downmixCoefficient[c * baseChannelCount + d];
                            }
                            if (maxSum < sum) maxSum = sum;
                        }
                        signalPeakLevelTmp = 20.0f * (float)log10(maxSum);
                    } else {
#if DEBUG_WARNINGS
                        fprintf(stderr, "WARNING: downmix coefficients missing. Estimation of signalPeakLevel might not be accurate.");
#endif
                    }
                    break;
                }
            }
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
            if (loudnessDrcSetIdRequested == 0) {
                for (n=0; n<loudnessInfoCount; n++)
                {
                    if (mode == 1)
                    {
                        loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
                    }
                    else
                    {
                        loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
                    }
                    if (loudnessDrcSetIdRequested == loudnessInfo->drcSetId && ID_FOR_BASE_LAYOUT == loudnessInfo->downmixId) {
                        if (loudnessInfo->truePeakLevelPresent)
                        {
#if MPEG_D_DRC_EXTENSION_V1
                            eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                            eqSetId[peakCount] = 0;
#endif
                            if (loudnessInfo->loudnessInfoType == 3 && hUniDrcSelProcParams->groupPresetIdRequestedPreference == loudnessInfo->mae_groupPresetID) {
                                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                                explicitPeakInformationPresent[peakCount] = 0;
                                matchFound = TRUE;
                                peakCount++;
                            }
                            if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 3) {
                                for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                                    if (hUniDrcSelProcParams->groupPresetIdRequested[i] == loudnessInfo->mae_groupPresetID) {
                                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                                        explicitPeakInformationPresent[peakCount] = 0;
                                        matchFound = TRUE;
                                        peakCount++;
                                        break;
                                    }
                                }
                            }
                            if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 2) {
                                for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                                    if (hUniDrcSelProcParams->groupIdRequested[i] == loudnessInfo->mae_groupID) {
                                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                                        explicitPeakInformationPresent[peakCount] = 0;
                                        matchFound = TRUE;
                                        peakCount++;
                                        break;
                                    }
                                }
                            }
                        }
                        if (matchFound == FALSE) {
                            if (loudnessInfo->samplePeakLevelPresent)
                            {
#if MPEG_D_DRC_EXTENSION_V1
                                eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                                eqSetId[peakCount] = 0;
#endif
                                if (loudnessInfo->loudnessInfoType == 3 && hUniDrcSelProcParams->groupPresetIdRequestedPreference == loudnessInfo->mae_groupPresetID) {
                                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                    explicitPeakInformationPresent[peakCount] = 0;
                                    matchFound = TRUE;
                                    peakCount++;
                                }
                                if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 3) {
                                    for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                                        if (hUniDrcSelProcParams->groupPresetIdRequested[i] == loudnessInfo->mae_groupPresetID) {
                                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                            explicitPeakInformationPresent[peakCount] = 0;
                                            matchFound = TRUE;
                                            peakCount++;
                                            break;
                                        }
                                    }
                                }
                                if (matchFound == FALSE && loudnessInfo->loudnessInfoType == 2) {
                                    for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                                        if (hUniDrcSelProcParams->groupIdRequested[i] == loudnessInfo->mae_groupID) {
                                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                            explicitPeakInformationPresent[peakCount] = 0;
                                            matchFound = TRUE;
                                            peakCount++;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (matchFound == FALSE) {
#endif
#endif
            for (n=0; n<loudnessInfoCount; n++)
            {
                if (mode == 1)
                {
                    loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
                }
                else
                {
                    loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
                }
                if (loudnessDrcSetIdRequested == loudnessInfo->drcSetId && ID_FOR_BASE_LAYOUT == loudnessInfo->downmixId) {
                    if (loudnessInfo->truePeakLevelPresent)
                    {
#if MPEG_D_DRC_EXTENSION_V1
                        eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                        eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                        explicitPeakInformationPresent[peakCount] = 0;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                        signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                        explicitPeakInformationPresent[peakCount] = 0;
#endif
                        matchFound = TRUE;
                        peakCount++;
#else
                        if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                            (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                            (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                            explicitPeakInformationPresent[peakCount] = 0;
#endif
                            matchFound = TRUE;
                            peakCount++;
                        }
#endif
                    }
                    if (matchFound == FALSE) {
                        if (loudnessInfo->samplePeakLevelPresent)
                        {
#if MPEG_D_DRC_EXTENSION_V1
                            eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                            eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                            explicitPeakInformationPresent[peakCount] = 0;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                            explicitPeakInformationPresent[peakCount] = 0;
#endif
                            matchFound = TRUE;
                            peakCount++;
#else
                            if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                                (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                                (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                                signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                explicitPeakInformationPresent[peakCount] = 0;
#endif
                                matchFound = TRUE;
                                peakCount++;
                            }
#endif
                        }
                    }
                }
            }
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
            }
#endif
#endif
            if (matchFound == FALSE) {
                for (n=0; n<loudnessInfoCount; n++)
                {
                    if (mode == 1)
                    {
                        loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
                    }
                    else
                    {
                        loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
                    }
                    if (ID_FOR_ANY_DRC == loudnessInfo->drcSetId && ID_FOR_BASE_LAYOUT == loudnessInfo->downmixId) {
                        if (loudnessInfo->truePeakLevelPresent)
                        {
#if MPEG_D_DRC_EXTENSION_V1
                            eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                            eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                            explicitPeakInformationPresent[peakCount] = 0;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                            signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                            explicitPeakInformationPresent[peakCount] = 0;
#endif
                            matchFound = TRUE;
                            peakCount++;
#else
                            if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                                (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                                (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                                signalPeakLevel[peakCount] = loudnessInfo->truePeakLevel + signalPeakLevelTmp;
                                explicitPeakInformationPresent[peakCount] = 0;
#endif
                                matchFound = TRUE;
                                peakCount++;
                            }
#endif
                        }
                        if (matchFound == FALSE) {
                            if (loudnessInfo->samplePeakLevelPresent)
                            {
#if MPEG_D_DRC_EXTENSION_V1
                                eqSetId[peakCount] = loudnessInfo->eqSetId;
#else
                                eqSetId[peakCount] = 0;
#endif
#ifndef AMD2_COR1
                                signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                explicitPeakInformationPresent[peakCount] = 0;
#endif
#if !MPEG_H_SYNTAX
#ifdef AMD2_COR1
                                signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                explicitPeakInformationPresent[peakCount] = 0;
#endif
                                matchFound = TRUE;
                                peakCount++;
#else
                                if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                                    (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                                    (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
#ifdef AMD2_COR1
                                    signalPeakLevel[peakCount] = loudnessInfo->samplePeakLevel + signalPeakLevelTmp;
                                    explicitPeakInformationPresent[peakCount] = 0;
#endif
                                    matchFound = TRUE;
                                    peakCount++;
                                }
#endif
                            }
                        }
                    }
                }
            }
            if (matchFound == FALSE) { /* TODO: which DRC set should be found here? There should be no other DRC set with the same drcSetId (loudnessDrcSetIdRequested) but downmixId=0, should it? (fb: perhaps if there are multiple downmixIds in the DRC instructions?) */
                DrcInstructionsUniDrc* drcInstructionsUniDrcTmp;
                for (n=0; n<hUniDrcConfig->drcInstructionsCountPlus; n++) {
                    drcInstructionsUniDrcTmp = &hUniDrcConfig->drcInstructionsUniDrc[n];
                    if (loudnessDrcSetIdRequested == drcInstructionsUniDrcTmp->drcSetId) {
                        for (k=0; k<drcInstructionsUniDrcTmp->downmixIdCount; k++) {
                            if (ID_FOR_BASE_LAYOUT == drcInstructionsUniDrcTmp->downmixId[k]) {
                                if (drcInstructionsUniDrcTmp->limiterPeakTargetPresent) {
                                    eqSetId[peakCount] = -1; /* not specified */
                                    signalPeakLevel[peakCount] = drcInstructionsUniDrcTmp->limiterPeakTarget + signalPeakLevelTmp;
                                    explicitPeakInformationPresent[peakCount] = 0;
                                    matchFound = TRUE;
                                    peakCount++;
                                }
                            }
                            break;
                        }
                    }
                }
            }
#ifdef AMD2_COR1
            if (matchFound == FALSE) {
                *signalPeakLevel = *signalPeakLevel + signalPeakLevelTmp;
            }
#endif
        }
    }
    if (peakCount > 0)
    {
        *peakInfoCount = peakCount;
    }
    else
    {
        *peakInfoCount = prelimCount;
    }
    return (0);
}
    
    
int
extractLoudnessPeakToAverageValue(LoudnessInfo* loudnessInfo,
                                  const int     dynamicRangeMeasurementType,
                                  int *         loudnessPeakToAverageValuePresent,
                                  float*        loudnessPeakToAverageValue)
{
    int k;
    int programLoudnessPresent = FALSE;
    int peakLoudnessPresent = FALSE;
    int matchMeasureProgramLoudness = 0;
    int matchMeasurePeakLoudness = 0;
    float programLoudness = 0.0f;
    float peakLoudness = 0.0f;
    LoudnessMeasure* loudnessMeasure = NULL;

    /* system index                   OTHER  EBU_R_128  BS_1770_4  BS_1770_4_PRE_PROC  USER EXPERT_PANEL BS_1771_1 RESERVED_A RESERVED_B RESERVED_C RESERVED_D RESERVED_E */
    int systemBonusProgramLoudness[] = {0,     0,         1,         0,                  0,   0,           0,        2,         3,         4,         0,         0};
    int systemBonusPeakLoudness[]    = {0,     7,         0,         0,                  0,   0,           6,        5,         4,         3,         2,         1};
    

    for (k=0; k<loudnessInfo->measurementCount; k++)
    {
        loudnessMeasure = &(loudnessInfo->loudnessMeasure[k]);
        if (loudnessMeasure->methodDefinition == METHOD_DEFINITION_PROGRAM_LOUDNESS)
        {
            if (matchMeasureProgramLoudness < systemBonusProgramLoudness[loudnessMeasure->measurementSystem])
            {
                programLoudness = loudnessMeasure->methodValue;
                programLoudnessPresent = TRUE;
                matchMeasureProgramLoudness = systemBonusProgramLoudness[loudnessMeasure->measurementSystem];
            }
        }
        switch (dynamicRangeMeasurementType) {
            case SHORT_TERM_LOUDNESS_TO_AVG:
                if (loudnessMeasure->methodDefinition == METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX)
                {
                    if (matchMeasurePeakLoudness < systemBonusPeakLoudness[loudnessMeasure->measurementSystem])
                    {
                        peakLoudness = loudnessMeasure->methodValue;
                        peakLoudnessPresent = TRUE;
                        matchMeasurePeakLoudness = systemBonusPeakLoudness[loudnessMeasure->measurementSystem];
                    }
                }
                break;
                
            case MOMENTARY_LOUDNESS_TO_AVG:
                if (loudnessMeasure->methodDefinition == METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX)
                {
                    if (matchMeasurePeakLoudness < systemBonusPeakLoudness[loudnessMeasure->measurementSystem])
                    {
                        peakLoudness = loudnessMeasure->methodValue;
                        peakLoudnessPresent = TRUE;
                        matchMeasurePeakLoudness = systemBonusPeakLoudness[loudnessMeasure->measurementSystem];
                    }
                }
                break;
                
            case TOP_OF_LOUDNESS_RANGE_TO_AVG:
                if (loudnessMeasure->methodDefinition == METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE)
                {
                    if (matchMeasurePeakLoudness < systemBonusPeakLoudness[loudnessMeasure->measurementSystem])
                    {
                        peakLoudness = loudnessMeasure->methodValue;
                        peakLoudnessPresent = TRUE;
                        matchMeasurePeakLoudness = systemBonusPeakLoudness[loudnessMeasure->measurementSystem];
                    }
                }
                break;
                
            default:
                fprintf(stderr, "ERROR: undefined dynamic range measurement requested %d\n", dynamicRangeMeasurementType);
                return (UNEXPECTED_ERROR);
                
                break;
        }
    }
    if ((programLoudnessPresent == TRUE) && (peakLoudnessPresent == TRUE))
    {
        *loudnessPeakToAverageValue = peakLoudness - programLoudness;
        *loudnessPeakToAverageValuePresent = TRUE;
    }
    return (0);
}


int
getLoudnessPeakToAverageValue(
                              HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                              DrcInstructionsUniDrc* drcInstructionsUniDrc,
                              const int downmixIdRequested,
                              const int dynamicRangeMeasurementType,
                              const int albumMode,
                              int* loudnessPeakToAverageValuePresent,
                              float* loudnessPeakToAverageValue)
{
    int n, err;
    int drcSetId = max(0,drcInstructionsUniDrc->drcSetId);
    
    *loudnessPeakToAverageValuePresent = FALSE;

    if (albumMode == TRUE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoAlbumCount; n++)
        {
            LoudnessInfo* loudnessInfo = &(hLoudnessInfoSet->loudnessInfoAlbum[n]);
            if (drcSetId == loudnessInfo->drcSetId)
            {
                if (downmixIdRequested == loudnessInfo->downmixId)
                {
#if !MPEG_H_SYNTAX
                    err = extractLoudnessPeakToAverageValue(loudnessInfo,
                                                            dynamicRangeMeasurementType,
                                                            loudnessPeakToAverageValuePresent,
                                                            loudnessPeakToAverageValue);
                    if (err) return (err);
#else
                    if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
                            err = extractLoudnessPeakToAverageValue(loudnessInfo,
                                                                    dynamicRangeMeasurementType,
                                                                    loudnessPeakToAverageValuePresent,
                                                                    loudnessPeakToAverageValue);
                            if (err) return (err);
                    }
#endif
                }
            }
        }
    }
    if (*loudnessPeakToAverageValuePresent == FALSE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
        {
            LoudnessInfo* loudnessInfo = &(hLoudnessInfoSet->loudnessInfo[n]);
            if (drcSetId == loudnessInfo->drcSetId)
            {
                if (downmixIdRequested == loudnessInfo->downmixId)
                {
#if !MPEG_H_SYNTAX
                    err = extractLoudnessPeakToAverageValue(loudnessInfo,
                                                            dynamicRangeMeasurementType,
                                                            loudnessPeakToAverageValuePresent,
                                                            loudnessPeakToAverageValue);
                    if (err) return (err);
#else
                    if ((drcInstructionsUniDrc->drcInstructionsType == 3 && loudnessInfo->loudnessInfoType == 3 && drcInstructionsUniDrc->mae_groupPresetID == loudnessInfo->mae_groupPresetID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 2 && loudnessInfo->loudnessInfoType == 2 && drcInstructionsUniDrc->mae_groupID == loudnessInfo->mae_groupID) ||
                        (drcInstructionsUniDrc->drcInstructionsType == 0 && loudnessInfo->loudnessInfoType == 0)) {
                            err = extractLoudnessPeakToAverageValue(loudnessInfo,
                                                                    dynamicRangeMeasurementType,
                                                                    loudnessPeakToAverageValuePresent,
                                                                    loudnessPeakToAverageValue);
                            if (err) return (err);
                    }
#endif
                }
            }
        }
    }
    return (0);
}

int
checkIfOverallLoudnessPresent(const LoudnessInfo* loudnessInfo,
                              int* loudnessInfoPresent)
{
    int m;
    
    *loudnessInfoPresent = FALSE;
    for (m=0; m<loudnessInfo->measurementCount; m++)
    {
        if ((loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_PROGRAM_LOUDNESS) ||
            (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_ANCHOR_LOUDNESS))
        {
            *loudnessInfoPresent = TRUE;
        }
    }
    return (0);
}

#if MPEG_D_DRC_EXTENSION_V1
#define MIXING_LEVEL_DEFAULT 85.0f
int
getMixingLevel(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams,
               HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
               const int downmixIdRequested,
               const int drcSetIdRequested,
               const int eqSetIdRequested,
               float* mixingLevel)
{
    int n, k, infoCount;
    int albumMode = hUniDrcSelProcParams->albumMode;
    int loudnessDrcSetIdRequested;
    LoudnessInfo* loudnessInfo;
    
    *mixingLevel = MIXING_LEVEL_DEFAULT;
    if (drcSetIdRequested < 0)
    {
        loudnessDrcSetIdRequested = 0;
    }
    else
    {
        loudnessDrcSetIdRequested = drcSetIdRequested;
    }
    if (albumMode == TRUE)
    {
        infoCount = hLoudnessInfoSet->loudnessInfoAlbumCount;
        loudnessInfo = hLoudnessInfoSet->loudnessInfoAlbum;
    }
    else
    {
        infoCount = hLoudnessInfoSet->loudnessInfoCount;
        loudnessInfo = hLoudnessInfoSet->loudnessInfo;
    }
    for (n=0; n<infoCount; n++)
    {
        if ((downmixIdRequested == loudnessInfo[n].downmixId) || (ID_FOR_ANY_DOWNMIX == loudnessInfo[n].downmixId))
        {
            if (loudnessDrcSetIdRequested == loudnessInfo[n].drcSetId)
            {
                if (eqSetIdRequested == loudnessInfo[n].eqSetId)
                {
                    for (k=0; k<loudnessInfo[n].measurementCount; k++)
                    {
                        if (loudnessInfo[n].loudnessMeasure[k].methodDefinition == METHOD_DEFINITION_MIXING_LEVEL)
                        {
                            *mixingLevel = loudnessInfo[n].loudnessMeasure[k].methodValue;
                            break;
                        }
                    }
                }
            }
        }
    }
    return(0);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */
    
int
matchAndCheckLoudnessInfo(const int loudnessInfoCount,
                          LoudnessInfo* loudnessInfo,
                          const int downmixIdRequested,
                          const int drcSetIdRequested,
#if MPEG_H_SYNTAX
                          const int groupIdRequested,
                          const int groupPresetIdRequested,
                          const int numGroupIdsRequestedExternal,
                          const int* groupIdRequestedExternal,
                          const int numGroupPresetIdsRequestedExternal,
                          const int* groupPresetIdRequestedExternal,
                          const int* numMembersGroupPresetIdsRequestedExternal,
                          const int groupPresetIdRequestedPreferenceExternal,
#endif
                          int* infoCount,
                          LoudnessInfo* loudnessInfoMatching[])
{
    int n, err;
    int loudnessInfoPresent;
    for (n=0; n<loudnessInfoCount; n++)
    {
        if (downmixIdRequested == loudnessInfo[n].downmixId)
        {
            if (drcSetIdRequested == loudnessInfo[n].drcSetId)
            {
                err = checkIfOverallLoudnessPresent(&(loudnessInfo[n]), &loudnessInfoPresent);
                if (err) return (err);
                if (loudnessInfoPresent)
                {
                    loudnessInfoMatching[*infoCount] = &(loudnessInfo[n]);
                    (*infoCount)++;
                }
            }
        }
    }
#if MPEG_H_SYNTAX
    {
        int n = 0, i = 0, k = 0, l = 0, m = 0;
        int loudnessInfoType3Match[LOUDNESS_INFO_COUNT_MAX];
        int loudnessInfoType3MatchNumMembers[LOUDNESS_INFO_COUNT_MAX];
        int loudnessInfoType2Match[LOUDNESS_INFO_COUNT_MAX];
        int loudnessInfoType0Match[LOUDNESS_INFO_COUNT_MAX];
        int maxNumMembersGroupPresetIds = 0;
        int maxGroupId = 0;
        int largestGroupPresetId = 0;
        
        /* categorize loudnessInfoMatching dependent on loudnessInfoType */
        for (n=0; n<*infoCount; n++)
        {
            if (loudnessInfoMatching[n]->loudnessInfoType == 3) {
                for (i=0; i<numGroupPresetIdsRequestedExternal; i++) {
                    if (groupPresetIdRequestedExternal[i] == loudnessInfoMatching[n]->mae_groupPresetID) {
                        loudnessInfoType3Match[k] = n;
                        loudnessInfoType3MatchNumMembers[k] = numMembersGroupPresetIdsRequestedExternal[i];
                        k++;
                    }
                }
            } else if (loudnessInfoMatching[n]->loudnessInfoType == 2) {
                for (i=0; i<numGroupIdsRequestedExternal; i++) {
                    if (groupIdRequestedExternal[i] == loudnessInfoMatching[n]->mae_groupID) {
                        loudnessInfoType2Match[l] = n;
                        l++;
                    }
                }
            } else if (loudnessInfoMatching[n]->loudnessInfoType == 0) {
                loudnessInfoType0Match[m] = n;
                m++;
            }
        }
        *infoCount = 0;
        
        /* groupPresetId match */
        if (k != 0) {
            /* full match */
            if (groupPresetIdRequested != -1) {
                for (n=0; n<k; n++) {
                    if (groupPresetIdRequested == loudnessInfoMatching[loudnessInfoType3Match[n]]->mae_groupPresetID) {
                        loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType3Match[n]];
                        (*infoCount)++;
                    }
                }
            }
            /* preference match */
            if (*infoCount == 0 && groupPresetIdRequestedPreferenceExternal != -1) {
                for (n=0; n<k; n++) {
                    if (groupPresetIdRequestedPreferenceExternal == loudnessInfoMatching[loudnessInfoType3Match[n]]->mae_groupPresetID) {
                        loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType3Match[n]];
                        (*infoCount)++;
                    }
                }
            }
            /* other match */
            if (*infoCount == 0) {
                for (n=0; n<k; n++) {
                    /* choose groupPreset with largest number of members */
                    if (maxNumMembersGroupPresetIds <= loudnessInfoType3MatchNumMembers[n]) {
                        if (maxNumMembersGroupPresetIds < loudnessInfoType3MatchNumMembers[n]) {
                            maxNumMembersGroupPresetIds = loudnessInfoType3MatchNumMembers[n];
                            *infoCount = 0;
                        }
                        /* choose largest groupPresetId */
                        if (largestGroupPresetId <= loudnessInfoMatching[loudnessInfoType3Match[n]]->mae_groupPresetID) {
                            if (largestGroupPresetId < loudnessInfoMatching[loudnessInfoType3Match[n]]->mae_groupPresetID) {
                                largestGroupPresetId = loudnessInfoMatching[loudnessInfoType3Match[n]]->mae_groupPresetID;
                                *infoCount = 0;
                            }
                            loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType3Match[n]];
                            (*infoCount)++;
                        }
                    }
                }
            }
        }
        
        /* groupId match */
        if (*infoCount == 0 && l != 0) {
            /* full match */
            if (groupIdRequested != -1) {
                for (n=0; n<l; n++) {
                    if (groupIdRequested == loudnessInfoMatching[loudnessInfoType2Match[n]]->mae_groupID) {
                        loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType2Match[n]];
                        (*infoCount)++;
                    }
                }
            }
            /* other match */
            if (*infoCount == 0) {
                for (n=0; n<l; n++) {
                    if (maxGroupId <= loudnessInfoMatching[loudnessInfoType2Match[n]]->mae_groupID) {
                        if (maxGroupId < loudnessInfoMatching[loudnessInfoType2Match[n]]->mae_groupID) {
                            maxGroupId = loudnessInfoMatching[loudnessInfoType2Match[n]]->mae_groupID;
                            *infoCount = 0;
                        }
                        loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType2Match[n]];
                        (*infoCount)++;
                    }
                }
            }
        }
        
        /* other match without groupId and groupPresetId */
        if (*infoCount == 0 && m != 0) {
            for (n=0; n<m; n++) {
                loudnessInfoMatching[*infoCount] = loudnessInfoMatching[loudnessInfoType0Match[n]];
                (*infoCount)++;
            }
        }
    }
#endif /* MPEG_H_SYNTAX */
    return (0);
}

int
matchAndCheckLoudnessPayloads(const int loudnessInfoCount,
                              LoudnessInfo* loudnessInfo,
                              const int downmixIdRequested,
                              const int drcSetIdRequested,
#if MPEG_H_SYNTAX
                              const int groupIdRequested,
                              const int groupPresetIdRequested,
                              const int numGroupIdsRequestedExternal,
                              const int* groupIdRequestedExternal,
                              const int numGroupPresetIdsRequestedExternal,
                              const int* groupPresetIdRequestedExternal,
                              const int* numMembersGroupPresetIdsRequestedExternal,
                              const int groupPresetIdRequestedPreferenceExternal,
#endif
                              int* infoCount,
                              LoudnessInfo* loudnessInfoMatching[])
{
    int err = 0;
    
    /* find the applicable loudness information. Check fallbacks if neccessary */
#if !MPEG_H_SYNTAX
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, drcSetIdRequested,   infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, drcSetIdRequested,   infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, ID_FOR_ANY_DRC,      infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, ID_FOR_NO_DRC,       infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, ID_FOR_ANY_DRC,      infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, ID_FOR_NO_DRC,       infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, drcSetIdRequested,   infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, ID_FOR_ANY_DRC,      infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, ID_FOR_NO_DRC,       infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
#else
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, drcSetIdRequested, groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, drcSetIdRequested, groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, ID_FOR_ANY_DRC,    groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, downmixIdRequested, ID_FOR_NO_DRC,     groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, ID_FOR_ANY_DRC,    groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_ANY_DOWNMIX, ID_FOR_NO_DRC,     groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, drcSetIdRequested, groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, ID_FOR_ANY_DRC,    groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
    err = matchAndCheckLoudnessInfo(loudnessInfoCount, loudnessInfo, ID_FOR_BASE_LAYOUT, ID_FOR_NO_DRC,     groupIdRequested, groupPresetIdRequested, numGroupIdsRequestedExternal,
                                    groupIdRequestedExternal, numGroupPresetIdsRequestedExternal, groupPresetIdRequestedExternal, numMembersGroupPresetIdsRequestedExternal, groupPresetIdRequestedPreferenceExternal, infoCount, loudnessInfoMatching); if (err || *infoCount) goto matchEnd;
#endif
matchEnd:
    return (err);
}

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
                            int* infoCount,
                            LoudnessInfo* loudnessInfoMatching[])
{
    int err;
    int loudnessDrcSetIdRequested;
    
    *infoCount = 0;
    if (drcSetIdRequested < 0)
    {
        loudnessDrcSetIdRequested = ID_FOR_NO_DRC;
    }
    else
    {
        loudnessDrcSetIdRequested = drcSetIdRequested;
    }
    if (hUniDrcSelProcParams->albumMode == TRUE)
    {
        err = matchAndCheckLoudnessPayloads(hLoudnessInfoSet->loudnessInfoAlbumCount,
                                            hLoudnessInfoSet->loudnessInfoAlbum,
                                            downmixIdRequested,
                                            loudnessDrcSetIdRequested,
#if MPEG_H_SYNTAX
                                            groupIdRequested,
                                            groupPresetIdRequested,
                                            hUniDrcSelProcParams->numGroupIdsRequested,
                                            hUniDrcSelProcParams->groupIdRequested,
                                            hUniDrcSelProcParams->numGroupPresetIdsRequested,
                                            hUniDrcSelProcParams->groupPresetIdRequested,
                                            hUniDrcSelProcParams->numMembersGroupPresetIdsRequested,
                                            hUniDrcSelProcParams->groupPresetIdRequestedPreference,
#endif
                                            infoCount,
                                            loudnessInfoMatching);
        if (err) return (err);
        
    }
    /* now without albumMode */
    /* look for full match (current state) */
    if (*infoCount == 0)
    {
        err = matchAndCheckLoudnessPayloads(hLoudnessInfoSet->loudnessInfoCount,
                                            hLoudnessInfoSet->loudnessInfo,
                                            downmixIdRequested,
                                            loudnessDrcSetIdRequested,
#if MPEG_H_SYNTAX
                                            groupIdRequested,
                                            groupPresetIdRequested,
                                            hUniDrcSelProcParams->numGroupIdsRequested,
                                            hUniDrcSelProcParams->groupIdRequested,
                                            hUniDrcSelProcParams->numGroupPresetIdsRequested,
                                            hUniDrcSelProcParams->groupPresetIdRequested,
                                            hUniDrcSelProcParams->numMembersGroupPresetIdsRequested,
                                            hUniDrcSelProcParams->groupPresetIdRequestedPreference,
#endif
                                            infoCount,
                                            loudnessInfoMatching);
        if (err) return (err);
    }
    *overallLoudnessInfoPresent = (*infoCount > 0);
    return(0);
}


int
getHighPassLoudnessAdjustment(const LoudnessInfo* loudnessInfo,
                              int* loudnessHighPassAdjustPresent,
                              float* loudnessHighPassAdjust)
{
    int m, k;
    
    *loudnessHighPassAdjustPresent = FALSE;
    *loudnessHighPassAdjust = 0.0f;
    for (m=0; m<loudnessInfo->measurementCount; m++)
    {
        if (loudnessInfo->loudnessMeasure[m].measurementSystem == MEASUREMENT_SYSTEM_BS_1770_4_PRE_PROCESSING)
        {
            for (k=0; k<loudnessInfo->measurementCount; k++)
            {
                if (loudnessInfo->loudnessMeasure[k].measurementSystem == MEASUREMENT_SYSTEM_BS_1770_4)
                {
                    if (loudnessInfo->loudnessMeasure[m].methodDefinition == loudnessInfo->loudnessMeasure[k].methodDefinition)
                    {
                        *loudnessHighPassAdjustPresent = TRUE;
                        *loudnessHighPassAdjust = loudnessInfo->loudnessMeasure[m].methodValue - loudnessInfo->loudnessMeasure[k].methodValue;
                    }
                }
            }
        }
    }
    return (0);
}

int
findHighPassLoudnessAdjustment(
                               HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet,
                               const int downmixIdRequested,
                               const int drcSetIdRequested,
                               const int albumMode,
                               const float deviceCutoffFreq,
                               int* loudnessHighPassAdjustPresent,
                               float* loudnessHighPassAdjust)
{
    int n, err;
    int loudnessDrcSetIdRequested;
    
    if (drcSetIdRequested < 0)
    {
        loudnessDrcSetIdRequested = 0;
    }
    else
    {
        loudnessDrcSetIdRequested = drcSetIdRequested;
    }
    
    *loudnessHighPassAdjustPresent = FALSE;
    
    if (albumMode == TRUE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoAlbumCount; n++)
        {
            if ((downmixIdRequested == hLoudnessInfoSet->loudnessInfoAlbum[n].downmixId) || (ID_FOR_ANY_DOWNMIX == hLoudnessInfoSet->loudnessInfoAlbum[n].downmixId))
            {
                if (loudnessDrcSetIdRequested == hLoudnessInfoSet->loudnessInfoAlbum[n].drcSetId)
                {
                    err = getHighPassLoudnessAdjustment(&(hLoudnessInfoSet->loudnessInfo[n]), loudnessHighPassAdjustPresent, loudnessHighPassAdjust);
                    if (err) return (err);
                }
            }
        }
    }
    if (*loudnessHighPassAdjustPresent == FALSE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
        {
            if ((downmixIdRequested == hLoudnessInfoSet->loudnessInfo[n].downmixId) || (ID_FOR_ANY_DOWNMIX == hLoudnessInfoSet->loudnessInfo[n].downmixId))
            {
                if (loudnessDrcSetIdRequested == hLoudnessInfoSet->loudnessInfo[n].drcSetId)
                {
                    err = getHighPassLoudnessAdjustment(&(hLoudnessInfoSet->loudnessInfo[n]), loudnessHighPassAdjustPresent, loudnessHighPassAdjust);
                    if (err) return (err);
                }
            }
        }
    }
    if (*loudnessHighPassAdjustPresent == FALSE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
        {
            if (ID_FOR_BASE_LAYOUT == hLoudnessInfoSet->loudnessInfo[n].downmixId) /* base layout */
            {
                if (loudnessDrcSetIdRequested == hLoudnessInfoSet->loudnessInfo[n].drcSetId)
                {
                    err = getHighPassLoudnessAdjustment(&(hLoudnessInfoSet->loudnessInfo[n]), loudnessHighPassAdjustPresent, loudnessHighPassAdjust);
                    if (err) return (err);
                }
            }
        }
    }
    if (*loudnessHighPassAdjustPresent == FALSE)
    {
        for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
        {
            if (ID_FOR_BASE_LAYOUT == hLoudnessInfoSet->loudnessInfo[n].downmixId) /* base layout */
            {
                if (0 == hLoudnessInfoSet->loudnessInfo[n].drcSetId)
                {
                    err = getHighPassLoudnessAdjustment(&(hLoudnessInfoSet->loudnessInfo[n]), loudnessHighPassAdjustPresent, loudnessHighPassAdjust);
                    if (err) return (err);
                }
            }
        }
    }
    if (*loudnessHighPassAdjustPresent == FALSE)
    {
        *loudnessHighPassAdjust = 0.0f;
    }
    else
    {
        *loudnessHighPassAdjust *= (max(20.0f, min(500.0f, deviceCutoffFreq)) - 20.0f) / (500.0f - 20.0f);
    }
    return(0);
}
    
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
                     float  loudness[])
{
    int err, k, infoCount = 0, prelimCount;
    int loudnessHighPassAdjustPresent;
    int overallLoudnessInfoPresent;
    float preProcAdjust;
    
    k=0;
    if (drcSetIdRequested < 0)
    {
        for (k=0; k<noCompressionEqCount; k++)
        {
            eqSetId[k] = noCompressionEqId[k];
            loudness[k] = UNDEFINED_LOUDNESS_VALUE;
            loudnessNormalizationGainDb[k] = 0.0f;
        }
    }
    eqSetId[k] = 0;
    loudness[k] = UNDEFINED_LOUDNESS_VALUE;
    loudnessNormalizationGainDb[k] = 0.0f;
    k++;
    
    prelimCount = k;

    /* loudnessNormalizationOn == 1 */
    if (hUniDrcSelProcParams->loudnessNormalizationOn == TRUE)
    {
        int n;
        LoudnessInfo* loudnessInfo[16];
        err = findInfoWithOverallLoudness(hUniDrcSelProcParams,
                                          hLoudnessInfoSet,
                                          downmixIdRequested,
                                          drcSetIdRequested,
#if MPEG_H_SYNTAX
                                          groupIdRequested,
                                          groupPresetIdRequested,
#endif
                                          &overallLoudnessInfoPresent,
                                          &infoCount,
                                          loudnessInfo);
        if (err) return (err);
        
        if (overallLoudnessInfoPresent == TRUE)
        {
            /* default values */
            int requestedMethodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            int otherMethodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            int requestedMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_4;
            int requestedPreprocessing = FALSE;     /* no preprocessing */
            
            /* system index          OTHER  EBU_R_128  BS_1770_4  BS_1770_4_PRE_PROC  USER EXPERT_PANEL BS_1771_1 RESERVED_A RESERVED_B RESERVED_C RESERVED_D RESERVED_E */
            int systemBonus0[]      = {0,     0,         0,         0,                  0,   0,           0,        0,         0,         0,         0,         0};
            int systemBonus1770[]   = {0,     0,         8,         0,                  1,   3,           0,        5,         6,         7,         4,         2};
            int systemBonusUser[]   = {0,     0,         1,         0,                  8,   5,           0,        2,         3,         4,         6,         7};
            int systemBonusExpert[] = {0,     0,         3,         0,                  1,   8,           0,        4,         5,         6,         7,         2};
            int systemBonusResA[]   = {0,     0,         5,         0,                  1,   3,           0,        8,         6,         7,         4,         2};
            int systemBonusResB[]   = {0,     0,         5,         0,                  1,   3,           0,        6,         8,         7,         4,         2};
            int systemBonusResC[]   = {0,     0,         5,         0,                  1,   3,           0,        6,         7,         8,         4,         2};
            int systemBonusResD[]   = {0,     0,         3,         0,                  1,   7,           0,        4,         5,         6,         8,         2};
            int systemBonusResE[]   = {0,     0,         1,         0,                  7,   5,           0,        2,         3,         4,         6,         8};
            int* systemBonus = systemBonus0;
            
            int matchMeasure;
            float methodValue = 0;
            
            /* map user request parameters to internal parameters */
            switch (hUniDrcSelProcParams->loudnessMeasurementMethod) {
                case USER_METHOD_DEFINITION_DEFAULT:
                case USER_METHOD_DEFINITION_PROGRAM_LOUDNESS:
                    requestedMethodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
                    otherMethodDefinition  = METHOD_DEFINITION_ANCHOR_LOUDNESS;
                    break;
                case USER_METHOD_DEFINITION_ANCHOR_LOUDNESS:
                    requestedMethodDefinition = METHOD_DEFINITION_ANCHOR_LOUDNESS;
                    otherMethodDefinition  = METHOD_DEFINITION_PROGRAM_LOUDNESS;
                    break;
                    
                default:
                    fprintf(stderr, "ERROR: requested measurement method undefined %d\n", hUniDrcSelProcParams->loudnessMeasurementMethod);
                    return (UNEXPECTED_ERROR);
                    break;
            }
            
            switch (hUniDrcSelProcParams->loudnessMeasurementSystem) {
                case USER_MEASUREMENT_SYSTEM_DEFAULT:
                case USER_MEASUREMENT_SYSTEM_BS_1770_4:
                    requestedMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_4;
                    systemBonus = systemBonus1770;
                    break;
                case USER_MEASUREMENT_SYSTEM_USER:
                    requestedMeasurementSystem = MEASUREMENT_SYSTEM_USER;
                    systemBonus = systemBonusUser;
                    break;
                case USER_MEASUREMENT_SYSTEM_EXPERT_PANEL:
                    requestedMeasurementSystem = MEASUREMENT_SYSTEM_EXPERT_PANEL;
                    systemBonus = systemBonusExpert;
                    break;
                case USER_MEASUREMENT_SYSTEM_RESERVED_A:
                    requestedMeasurementSystem = USER_MEASUREMENT_SYSTEM_RESERVED_A;
                    systemBonus = systemBonusResA;
                    break;
                case USER_MEASUREMENT_SYSTEM_RESERVED_B:
                    requestedMeasurementSystem = USER_MEASUREMENT_SYSTEM_RESERVED_B;
                    systemBonus = systemBonusResB;
                    break;
                case USER_MEASUREMENT_SYSTEM_RESERVED_C:
                    requestedMeasurementSystem = USER_MEASUREMENT_SYSTEM_RESERVED_C;
                    systemBonus = systemBonusResC;
                    break;
                case USER_MEASUREMENT_SYSTEM_RESERVED_D:
                    requestedMeasurementSystem = USER_MEASUREMENT_SYSTEM_RESERVED_D;
                    systemBonus = systemBonusResD;
                    break;
                case USER_MEASUREMENT_SYSTEM_RESERVED_E:
                    requestedMeasurementSystem = USER_MEASUREMENT_SYSTEM_RESERVED_E;
                    systemBonus = systemBonusResE;
                    break;
                    
                default:
                    fprintf(stderr, "ERROR: requested measurement system undefined %d\n", hUniDrcSelProcParams->loudnessMeasurementSystem);
                    return (UNEXPECTED_ERROR);
                    break;
            }
            
            switch (hUniDrcSelProcParams->loudnessMeasurementPreProc) {
                case USER_LOUDNESS_PREPROCESSING_DEFAULT:
                case USER_LOUDNESS_PREPROCESSING_OFF:
                    requestedPreprocessing = FALSE;
                    break;
                case USER_LOUDNESS_PREPROCESSING_HIGHPASS:
                    requestedPreprocessing = TRUE;
                    break;
                    
                default:
                    fprintf(stderr, "ERROR: requested pre-processing  undefined %d\n", hUniDrcSelProcParams->loudnessMeasurementPreProc);
                    return (UNEXPECTED_ERROR);
                    break;
            }
            
            for (k=0; k<infoCount; k++)
            {
                matchMeasure = -1;
                for (n=0; n<loudnessInfo[k]->measurementCount; n++)
                {
                    LoudnessMeasure* loudnessMeasure = &(loudnessInfo[k]->loudnessMeasure[n]);
                    if (matchMeasure < systemBonus[loudnessMeasure->measurementSystem] && requestedMethodDefinition == loudnessMeasure->methodDefinition)
                    {
                        methodValue = loudnessMeasure->methodValue;
                        matchMeasure = systemBonus[loudnessMeasure->measurementSystem];
                    }
                }
                if (matchMeasure == -1) /* other method */
                {
                    for (n=0; n<loudnessInfo[k]->measurementCount; n++)
                    {
                        LoudnessMeasure* loudnessMeasure = &(loudnessInfo[k]->loudnessMeasure[n]);
                        if (matchMeasure < systemBonus[loudnessMeasure->measurementSystem] && otherMethodDefinition == loudnessMeasure->methodDefinition)
                        {
                            methodValue = loudnessMeasure->methodValue;
                            matchMeasure = systemBonus[loudnessMeasure->measurementSystem];
                        }
                    }
                }
                
                /* compute the loudness adjustment when the playback system has high-pass characteristic */
                if (requestedPreprocessing == TRUE)
                {
                    err = findHighPassLoudnessAdjustment(hLoudnessInfoSet, downmixIdRequested, drcSetIdRequested, hUniDrcSelProcParams->albumMode, (float)hUniDrcSelProcParams->deviceCutOffFrequency,
                                                         &loudnessHighPassAdjustPresent, &preProcAdjust);
                    if (err) return (err);
                    
                    if (loudnessHighPassAdjustPresent == FALSE)
                    {
                        preProcAdjust = -2.0f;
                    }
                    methodValue += preProcAdjust;
                }
#if MPEG_D_DRC_EXTENSION_V1
                eqSetId[k] = loudnessInfo[k]->eqSetId;
#else
                eqSetId[k] = 0;
#endif
                loudnessNormalizationGainDb[k] = hUniDrcSelProcParams->targetLoudness - methodValue;
                loudness[k] = methodValue;
            }
        }
    }
    if (infoCount > 0)
    {
        *loudnessInfoCount = infoCount;
    }
    else
    {
        *loudnessInfoCount = prelimCount;
    }

    return (0);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
