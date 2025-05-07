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
#include <string.h>
#include <math.h>

#include "uniDrcSelectionProcess.h"
#include "uniDrcSelectionProcess_drcSetSelection.h"

static int effectTypesRequestable[] = {
    EFFECT_BIT_NIGHT,
    EFFECT_BIT_NOISY,
    EFFECT_BIT_LIMITED,
    EFFECT_BIT_LOWLEVEL,
    EFFECT_BIT_DIALOG,
    EFFECT_BIT_GENERAL_COMPR,
    EFFECT_BIT_EXPAND,
    EFFECT_BIT_ARTISTIC
};

static int drcCharacteristicOrderDefault [][3] =
{
    {1, 2,  -1},
    {2, 3,   1},
    {3, 4,   2},
    {4, 5,   3},
    {5, 6,   4},
    {6, 5,  -1},
    {7, 9,  -1},
    {8, 10, -1},
    {9, 7,  -1},
    {10, 8, -1},
    {11, 10, 9}
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if DEBUG_DRC_SELECTION
int
printEffectTypes(const int drcSetEffect)
{
    int i;

    printf("   DRC effect types:            ");
    for (i=0; i<EFFECT_BIT_COUNT; i++)
    {
        if ((drcSetEffect >> i) & 1)
        {
            switch (1<<i) {
                case EFFECT_BIT_NIGHT:
                    printf("Night");
                    break;
                case EFFECT_BIT_NOISY:
                    printf("Noisy");
                    break;
                case EFFECT_BIT_LIMITED:
                    printf("Limited");
                    break;
                case EFFECT_BIT_LOWLEVEL:
                    printf("LowLevel");
                    break;
                case EFFECT_BIT_DIALOG:
                    printf("Dialog");
                    break;
                case EFFECT_BIT_GENERAL_COMPR:
                    printf("General");
                    break;
                case EFFECT_BIT_EXPAND:
                    printf("Expand");
                    break;
                case EFFECT_BIT_ARTISTIC:
                    printf("Artistic");
                    break;
                case EFFECT_BIT_CLIPPING:
                    printf("Clipping");
                    break;
                case EFFECT_BIT_FADE:
                    printf("Fading");
                    break;
                case EFFECT_BIT_DUCK_OTHER:
                case EFFECT_BIT_DUCK_SELF:
                    printf("Ducking");
                    break;
                default:
                    break;
            }
            printf(" ");
        }
    }
    printf("\n");
    return(0);
}

int
displaySelectedDrcSets(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    int i, drcInstructionsIndex;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;

    printf("=========================================\n");
    printf("Levels and Gain:\n");
    printf("   loudnessNormalizationGainDb %5.2f dB\n", hUniDrcSelProcStruct->uniDrcSelProcOutput.loudnessNormalizationGainDb);
    printf("   outputPeakLevel             %5.2f dB\n", hUniDrcSelProcStruct->uniDrcSelProcOutput.outputPeakLevelDb);
    if (hUniDrcSelProcStruct->uniDrcSelProcOutput.outputLoudness != UNDEFINED_LOUDNESS_VALUE)
    {
        printf("   outputLoudness              %5.2f dB\n", hUniDrcSelProcStruct->uniDrcSelProcOutput.outputLoudness);
    }
    printf("=========================================\n");

    for (i=0; i<4; i++)
    {
        if (i==0)
        {
            printf("Selected DRC:\n");
        }
        if (i==1)
        {
            printf("-----------------------------------------\n");
            printf("Dependent DRC:\n");
        }
        if (i==2)
        {
            printf("-----------------------------------------\n");
            printf("Fading:\n");
        }
        if (i==3)
        {
            printf("-----------------------------------------\n");
            printf("Ducking:\n");
        }
        drcInstructionsIndex = hUniDrcSelProcStruct->subDrc[i].drcInstructionsIndex;
        if (drcInstructionsIndex >= 0)
        {
            drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[drcInstructionsIndex]);
            printf("   drcSetId                     %d\n", drcInstructionsUniDrc->drcSetId);
            if (drcInstructionsUniDrc->drcSetEffect == 0 && drcInstructionsUniDrc->drcSetId < 0)
            {
                printf("   DRC effect types:            None \n");
            }
            else
            {
                printEffectTypes(drcInstructionsUniDrc->drcSetEffect);
            }
        }
    }
    printf("=========================================\n");
#if MPEG_D_DRC_EXTENSION_V1
    for (i=0; i<2; i++)
    {
        int eqInstructionsIndex;
        if (i==0)
        {
            printf("Selected EQ:\n");
        }
        if (i==1)
        {
            printf("-----------------------------------------\n");
            printf("Dependent EQ:\n");
        }
        eqInstructionsIndex = hUniDrcSelProcStruct->subEq[i].eqInstructionsIndex;
        if (eqInstructionsIndex >= 0)
        {
            EqInstructions* eqInstructions = &(hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[eqInstructionsIndex]);
            printf("   eqSetId                      %d\n", eqInstructions->eqSetId);
            switch (eqInstructions->eqSetPurpose) {
                case EQ_PURPOSE_EQ_OFF:
                    printf("   EQ purpose:                  EQ off\n");
                    break;
                case EQ_PURPOSE_DEFAULT:
                    printf("   EQ purpose:                  Default\n");
                    break;
                case EQ_PURPOSE_LARGE_ROOM:
                    printf("   EQ purpose:                  Large room\n");
                    break;
                case EQ_PURPOSE_SMALL_SPACE:
                    printf("   EQ purpose:                  Small space\n");
                    break;
                case EQ_PURPOSE_AVERAGE_ROOM:
                    printf("   EQ purpose:                  Avereage room\n");
                    break;
                case EQ_PURPOSE_CAR_CABIN:
                    printf("   EQ purpose:                  Car cabin\n");
                    break;
                case EQ_PURPOSE_HEADPHONES:
                    printf("   EQ purpose:                  Headphones\n");
                    break;
                case EQ_PURPOSE_LATE_NIGHT:
                    printf("   EQ purpose:                  Late night\n");
                    break;
                default:
                    printf("   EQ purpose:                  Unknown \n");
            }
        }
    }
    printf("=========================================\n");
    {
        int loudEqInstructionsIndex;
        printf("Selected loudness EQ:\n");
        loudEqInstructionsIndex = hUniDrcSelProcStruct->loudEqInstructionsIndexSelected;
        if (loudEqInstructionsIndex >= 0)
        {
            LoudEqInstructions* loudEqInstructions = &(hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructions[loudEqInstructionsIndex]);
            printf("   loudEqSetId                  %d\n", loudEqInstructions->loudEqSetId);
        }
    }
    printf("=========================================\n");
#endif
    return (0);
}

#endif

int
validateDrcFeatureRequest(HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams)
{
    int i,j;

    for (i=0; i<hUniDrcSelProcParams->numDrcFeatureRequests; i++)
    {
        switch (hUniDrcSelProcParams->drcFeatureRequestType[i]) {
            case MATCH_EFFECT_TYPE:
                for (j=0; j<hUniDrcSelProcParams->numDrcEffectTypeRequestsDesired[i]; j++)
                {
                    if (hUniDrcSelProcParams->drcEffectTypeRequest[i][j] == EFFECT_TYPE_REQUESTED_NONE)
                    {
                        if (hUniDrcSelProcParams->numDrcEffectTypeRequestsDesired[i] > 1)
                        {
                            fprintf(stderr, "ERROR: a desired effect type of no compression cannot be combined with more desired effect types (count=%d)\n", hUniDrcSelProcParams->numDrcEffectTypeRequestsDesired[i]);
                            return (UNEXPECTED_ERROR);
                        }
                    }
                }
                break;
            case MATCH_DYNAMIC_RANGE:
                break;
            case MATCH_DRC_CHARACTERISTIC:
                break;
            default:
                fprintf(stderr, "ERROR: invalid matching process selected: %d\n", hUniDrcSelProcParams->drcFeatureRequestType[i]);
                return (UNEXPECTED_ERROR);
                break;
        }
    }
    return (0);
}

int
findDrcInstructionsUniDrcForId(UniDrcConfig* uniDrcConfig,
                               const int drcSetIdRequested,
                               DrcInstructionsUniDrc** drcInstructionsUniDrc)
{
    int i;
    for (i=0; i<uniDrcConfig->drcInstructionsUniDrcCount; i++)
    {
        if (drcSetIdRequested == uniDrcConfig->drcInstructionsUniDrc[i].drcSetId) break;
    }
    if (i == uniDrcConfig->drcInstructionsUniDrcCount) {
        fprintf(stderr, "ERROR: requested DRC set not found %d\n", drcSetIdRequested);
        return (UNEXPECTED_ERROR);
    }
    *drcInstructionsUniDrc = &uniDrcConfig->drcInstructionsUniDrc[i];
    return (0);
}

#if MPEG_D_DRC_EXTENSION_V1
int
findLoudEqInstructionsIndexForId(UniDrcConfig* uniDrcConfig,
                                 const int loudEqSetIdRequested,
                                 int* instructionsIndex)
{
    int i;
    if (loudEqSetIdRequested > 0)
    {
        for (i=0; i<uniDrcConfig->uniDrcConfigExt.loudEqInstructionsCount; i++)
        {
            if (uniDrcConfig->uniDrcConfigExt.loudEqInstructions[i].loudEqSetId == loudEqSetIdRequested) break;
        }
        if (i == uniDrcConfig->uniDrcConfigExt.loudEqInstructionsCount) {
            fprintf(stderr, "ERROR: requested loudness EQ set not found %d\n", loudEqSetIdRequested);
            return (UNEXPECTED_ERROR);
        }
        *instructionsIndex = i;
    }
    else
    {
        *instructionsIndex = -1;
    }
    return (0);
}


int
findEqInstructionsForId(UniDrcConfig* uniDrcConfig,
                        const int eqSetIdRequested,
                        EqInstructions** eqInstructions)
{
    int i;
    for (i=0; i<uniDrcConfig->uniDrcConfigExt.eqInstructionsCount; i++)
    {
        if (eqSetIdRequested == uniDrcConfig->uniDrcConfigExt.eqInstructions[i].eqSetId) break;
    }
    if (i == uniDrcConfig->uniDrcConfigExt.eqInstructionsCount) {
        fprintf(stderr, "ERROR: requested EQ set not found %d\n", eqSetIdRequested);
        return (UNEXPECTED_ERROR);
    }
    *eqInstructions = &uniDrcConfig->uniDrcConfigExt.eqInstructions[i];
    return (0);
}

int
findDownmixForId(UniDrcConfig* uniDrcConfig,
                 const int downmixIdRequested,
                 DownmixInstructions** downmixInstructions)
{
    int i;
    for (i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
    {
        if (downmixIdRequested == uniDrcConfig->downmixInstructions[i].downmixId) break;
    }
    if (i == uniDrcConfig->downmixInstructionsCount) {
        fprintf(stderr, "ERROR: requested downmix ID not found %d\n", downmixIdRequested);
        return (UNEXPECTED_ERROR);
    }
    *downmixInstructions = &uniDrcConfig->downmixInstructions[i];
    return (0);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */

int
mapRequestedEffectToBitIndex(const int effectTypeRequested,
                             int* effectBitIndex)
{
    switch (effectTypeRequested) {
        case EFFECT_TYPE_REQUESTED_NONE:
            *effectBitIndex = EFFECT_BIT_NONE;
            break;
        case EFFECT_TYPE_REQUESTED_NIGHT:
            *effectBitIndex = EFFECT_BIT_NIGHT;
            break;
        case EFFECT_TYPE_REQUESTED_NOISY:
            *effectBitIndex = EFFECT_BIT_NOISY;
            break;
        case EFFECT_TYPE_REQUESTED_LIMITED:
            *effectBitIndex = EFFECT_BIT_LIMITED;
            break;
        case EFFECT_TYPE_REQUESTED_LOWLEVEL:
            *effectBitIndex = EFFECT_BIT_LOWLEVEL;
            break;
        case EFFECT_TYPE_REQUESTED_DIALOG:
            *effectBitIndex = EFFECT_BIT_DIALOG;
            break;
        case EFFECT_TYPE_REQUESTED_GENERAL_COMPR:
            *effectBitIndex = EFFECT_BIT_GENERAL_COMPR;
            break;
        case EFFECT_TYPE_REQUESTED_EXPAND:
            *effectBitIndex = EFFECT_BIT_EXPAND;
            break;
        case EFFECT_TYPE_REQUESTED_ARTISTIC:
            *effectBitIndex = EFFECT_BIT_ARTISTIC;
            break;

        default:
            fprintf(stderr, "ERROR: Request DRC set effect type is not supported %d\n", effectTypeRequested);
            return (UNEXPECTED_ERROR);

            break;
    }
    return (0);
}

int
getFadingDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = -1;
    if (hUniDrcSelProcStruct->uniDrcSelProcParams.albumMode == 0)
    {
        int n;
        DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;
        for (n=0; n<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrcCount; n++)
        {
            drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[n]);

            if (drcInstructionsUniDrc->drcSetEffect & EFFECT_BIT_FADE)
            {
                if (drcInstructionsUniDrc->downmixId[0] == ID_FOR_ANY_DOWNMIX)
                {
#if MPEG_H_SYNTAX
                    if (drcInstructionsUniDrc->drcInstructionsType == 0)
                    {
                        hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = n;
                    }
                    else if (drcInstructionsUniDrc->drcInstructionsType == 2)
                    {
                        int m;
                        for (m=0; m<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested; m++)
                        {
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupIdRequested[m] == drcInstructionsUniDrc->mae_groupID) {
                                hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = n;
                                break;
                            }
                        }
                    }
                    else if (drcInstructionsUniDrc->drcInstructionsType == 3)
                    {
                        int m;
                        for (m=0; m<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested; m++)
                        {
                            if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequested[m] == drcInstructionsUniDrc->mae_groupPresetID) {
                                hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = n;
                                break;
                            }
                        }
                    }
                    else
                    {
                        fprintf(stderr, "WARNING: drcInstructionsUniDrc->drcInstructionsType = %d is not allowed. This should have been catched before.\n",drcInstructionsUniDrc->drcInstructionsType);
                    }
#else
                    hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = n;
#endif
                }
                else
                {
                    fprintf(stderr, "ERROR: Fading DRC must have a downmix ID of ID_FOR_ANY_DOWNMIX. (is %x)", drcInstructionsUniDrc->downmixId[0]);
                    return (UNEXPECTED_ERROR);
                }
            }
        }
    }
    return (0);
}

int
getDuckingDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    int drcInstructionsIndex;
    int n, k;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;

    int downmixIdRequested = hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId;

    hUniDrcSelProcStruct->subDrc[3].drcInstructionsIndex = -1;
    drcInstructionsIndex = -1;
    drcInstructionsUniDrc = NULL;

    for (n=0; n<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrcCount; n++)
    {
        drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[n]);

        if (drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF))
        {
            for (k=0; k<drcInstructionsUniDrc->downmixIdCount; k++) {
                if (drcInstructionsUniDrc->downmixId[k] == downmixIdRequested)
                {
#ifdef LEVELING_SUPPORT
                    /* Loudness leveling is disabled */
                    if(drcInstructionsUniDrc->levelingPresent && !hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessLevelingOn) continue;
                    /* Only select the ducking-only-set if loudness leveling is disabled */
                    if(drcInstructionsUniDrc->duckingOnlyDrcSet && hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessLevelingOn) continue;
#endif
#if !MPEG_H_SYNTAX
                    drcInstructionsIndex = n;
#endif
                }
            }
        }
    }
    if (drcInstructionsIndex == -1)
    {
        for (n=0; n<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrcCount; n++)
        {
            drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[n]);

            if (drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF))
            {
                for (k=0; k<drcInstructionsUniDrc->downmixIdCount; k++) {
                    if (drcInstructionsUniDrc->downmixId[k] == ID_FOR_BASE_LAYOUT)
                    {
#ifdef LEVELING_SUPPORT
                        /* Loudness leveling is disabled */
                        if(drcInstructionsUniDrc->levelingPresent && !hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessLevelingOn) continue;
                        /* Only select the ducking-only-set if loudness leveling is disabled */
                        if(drcInstructionsUniDrc->duckingOnlyDrcSet && hUniDrcSelProcStruct->uniDrcSelProcParams.loudnessLevelingOn) continue;
#endif
#if MPEG_H_SYNTAX
                        if (drcInstructionsUniDrc->drcInstructionsType == 0)
                        {
                            drcInstructionsIndex = n;
                        }
                        else if (drcInstructionsUniDrc->drcInstructionsType == 2)
                        {
                            int m;
                            for (m=0; m<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupIdsRequested; m++)
                            {
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupIdRequested[m] == drcInstructionsUniDrc->mae_groupID) {
                                    drcInstructionsIndex = n;
                                    break;
                                }
                            }
                        }
                        else if (drcInstructionsUniDrc->drcInstructionsType == 3)
                        {
                            int m;
                            for (m=0; m<hUniDrcSelProcStruct->uniDrcSelProcParams.numGroupPresetIdsRequested; m++)
                            {
                                if (hUniDrcSelProcStruct->uniDrcSelProcParams.groupPresetIdRequested[m] == drcInstructionsUniDrc->mae_groupPresetID) {
                                    drcInstructionsIndex = n;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            fprintf(stderr, "WARNING: drcInstructionsUniDrc->drcInstructionsType = %d is not allowed. This should have been catched before.\n",drcInstructionsUniDrc->drcInstructionsType);
                        }
#else
                        drcInstructionsIndex = n;
#endif
                    }
                }
            }
        }
    }
    if (drcInstructionsIndex > -1)
    {
        hUniDrcSelProcStruct->subDrc[2].drcInstructionsIndex = -1;                    /* this discards any fading DRC */
        hUniDrcSelProcStruct->subDrc[3].drcInstructionsIndex = drcInstructionsIndex;  /* activates the ducking DRC set */
    }
    return (0);
}

int
getSelectedDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                  const int drcSetIdSelected)
{
    int n, i;

    DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;

    /* choose correct DRC instruction */
    for(n=0; n<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus; n++)
    {
        if (hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[n].drcSetId == drcSetIdSelected) break;
    }
    if (n == hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus)
    {
        fprintf(stderr, "ERROR: selected DRC set ID is not available %d.\n", drcSetIdSelected);
        fprintf(stderr, "Available DRC set IDs: ");
        for (i=0; i<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus; i++)
        {
            fprintf(stderr, "%d ", hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[i].drcSetId);
        }
        fprintf(stderr, "\n");
        return(EXTERNAL_ERROR);
    }
    hUniDrcSelProcStruct->drcInstructionsIndexSelected = n;
    drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[hUniDrcSelProcStruct->drcInstructionsIndexSelected]);

    hUniDrcSelProcStruct->subDrc[0].drcInstructionsIndex = hUniDrcSelProcStruct->drcInstructionsIndexSelected;
    return (0);
}

int
getDependentDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;
    drcInstructionsUniDrc = &(hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[hUniDrcSelProcStruct->drcInstructionsIndexSelected]);

    if (drcInstructionsUniDrc->dependsOnDrcSetPresent == TRUE)
    {
        int n, i;
        int dependsOnDrcSetID = drcInstructionsUniDrc->dependsOnDrcSet;

        for (n=0; n<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus; n++)
        {
            if (hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[n].drcSetId == dependsOnDrcSetID) break;
        }
        if (n == hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus)
        {
            fprintf(stderr, "ERROR: depending DRC set ID not available %d.\n", dependsOnDrcSetID);
            fprintf(stderr, "Available DRC set IDs: ");
            for (i=0; i<hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsCountPlus; i++)
            {
                fprintf(stderr, "%d ", hUniDrcSelProcStruct->uniDrcConfig.drcInstructionsUniDrc[i].drcSetId);
            }
            fprintf(stderr, "\n");
            return(UNEXPECTED_ERROR);
        }
        hUniDrcSelProcStruct->subDrc[1].drcInstructionsIndex = n;
    }
    else
    {
        hUniDrcSelProcStruct->subDrc[1].drcInstructionsIndex = -1;
    }
    return (0);
}

int
getDrcInstructionsDependent(const UniDrcConfig* uniDrcConfig,
                            const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                            DrcInstructionsUniDrc** drcInstructionsDependent)
{
    int j;
    DrcInstructionsUniDrc* dependentDrc = NULL;
    for (j=0; j<uniDrcConfig->drcInstructionsUniDrcCount; j++)
    {
        dependentDrc = (DrcInstructionsUniDrc*) &(uniDrcConfig->drcInstructionsUniDrc[j]);
        if (dependentDrc->drcSetId == drcInstructionsUniDrc->dependsOnDrcSet)
        {
            break;
        }
    }
    if (j == uniDrcConfig->drcInstructionsUniDrcCount)
    {
        fprintf(stderr, "ERROR: dependent DRC set not found %d\n", drcInstructionsUniDrc->dependsOnDrcSet);
        return (UNEXPECTED_ERROR);
    }
    if (dependentDrc->dependsOnDrcSetPresent == TRUE)
    {
        fprintf(stderr, "ERROR: dependind DRC set cannot depend on a third DRC set %d\n", dependentDrc->dependsOnDrcSet);
        return (UNEXPECTED_ERROR);
    }
    *drcInstructionsDependent = dependentDrc;
    return(0);
}

#if MPEG_D_DRC_EXTENSION_V1

int
getSelectedEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                  const int eqSetIdSelected)
{
    int n, i;

    hUniDrcSelProcStruct->eqInstructionsIndexSelected = -1;

    if (eqSetIdSelected > 0)
    {
        /* choose correct DRC instruction */
        for(n=0; n<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount; n++)
        {
            if (hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqSetId == eqSetIdSelected) break;
        }
        if (n == hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount)
        {
            fprintf(stderr, "ERROR: selected EQ set ID is not available %d.\n", eqSetIdSelected);
            fprintf(stderr, "Available EQ set IDs: ");
            for (i=0; i<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount; i++)
            {
                fprintf(stderr, "%d ", hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqSetId);
            }
            fprintf(stderr, "\n");
            return(EXTERNAL_ERROR);
        }
        hUniDrcSelProcStruct->eqInstructionsIndexSelected = n;
        if (hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqApplyToDownmix == 1)
        {
            hUniDrcSelProcStruct->subEq[1].eqInstructionsIndex = hUniDrcSelProcStruct->eqInstructionsIndexSelected;
        }
        else
        {
            hUniDrcSelProcStruct->subEq[0].eqInstructionsIndex = hUniDrcSelProcStruct->eqInstructionsIndexSelected;
        }
    }
    return (0);
}

int
getDependentEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct)
{
    EqInstructions* eqInstructions = NULL;

    if (hUniDrcSelProcStruct->eqInstructionsIndexSelected >= 0)
    {
        eqInstructions = &(hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[hUniDrcSelProcStruct->eqInstructionsIndexSelected]);

        if (eqInstructions->dependsOnEqSetPresent == TRUE)
        {
            int n, i;
            int dependsOnEqSetID = eqInstructions->dependsOnEqSet;

            for(n=0; n<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount; n++)
            {
                if (hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqSetId == dependsOnEqSetID) break;
            }
            if (n == hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount)
            {
                fprintf(stderr, "ERROR: depending EQ set ID not available %d.\n", dependsOnEqSetID);
                fprintf(stderr, "Available EQ set IDs: ");
                for (i=0; i<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructionsCount; i++)
                {
                    fprintf(stderr, "%d ", hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqSetId);
                }
                fprintf(stderr, "\n");
                return(UNEXPECTED_ERROR);
            }
            if (hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.eqInstructions[n].eqApplyToDownmix == 1)
            {
                hUniDrcSelProcStruct->subEq[1].eqInstructionsIndex = n;
            }
            else
            {
                hUniDrcSelProcStruct->subEq[0].eqInstructionsIndex = n;
            }
        }
    }
    return (0);
}

int
getSelectedLoudEqSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                     const int loudEqSetIdSelected)
{
    int n, i;

    hUniDrcSelProcStruct->loudEqInstructionsIndexSelected = -1;

    if (loudEqSetIdSelected > 0)
    {
        /* choose correct DRC instruction */
        for(n=0; n<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructionsCount; n++)
        {
            if (hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructions[n].loudEqSetId == loudEqSetIdSelected) break;
        }
        if (n == hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructionsCount)
        {
            fprintf(stderr, "ERROR: selected loudness EQ set ID is not available %d.\n", loudEqSetIdSelected);
            fprintf(stderr, "Available loudness EQ set IDs: ");
            for (i=0; i<hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructionsCount; i++)
            {
                fprintf(stderr, "%d ", hUniDrcSelProcStruct->uniDrcConfig.uniDrcConfigExt.loudEqInstructions[n].loudEqSetId);
            }
            fprintf(stderr, "\n");
            return(EXTERNAL_ERROR);
        }
        hUniDrcSelProcStruct->loudEqInstructionsIndexSelected = n;
    }
    return (0);
}

int
selectLoudEq(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
             int loudnessEqRequest,
             const int downmixIdRequested,
             const int drcSetIdRequested,
             const int eqSetIdRequested,
             int* loudEqIdSelected)
{
    int i, c, d, e;

    *loudEqIdSelected = 0;
#if AMD2_COR3
    if (loudnessEqRequest != LOUD_EQ_REQUEST_OFF)
#endif
    {
        for (i=0; i<hUniDrcConfig->uniDrcConfigExt.loudEqInstructionsCount; i++)
        {
            LoudEqInstructions* loudEqInstructions = &hUniDrcConfig->uniDrcConfigExt.loudEqInstructions[i];
            if (loudEqInstructions->drcLocation == LOCATION_SELECTED) {
                for (d=0; d<loudEqInstructions->downmixIdCount; d++)
                {
                    if ((loudEqInstructions->downmixId[d] == downmixIdRequested) || (loudEqInstructions->downmixId[d] == ID_FOR_ANY_DOWNMIX)) {
                        for (c=0; c<loudEqInstructions->drcSetIdCount; c++)
                        {
                            if ((loudEqInstructions->drcSetId[c] == drcSetIdRequested) || (loudEqInstructions->drcSetId[c] == ID_FOR_ANY_DRC)) {
                                for (e=0; e<loudEqInstructions->eqSetIdCount; e++)
                                {
                                    if ((loudEqInstructions->eqSetId[e] == eqSetIdRequested) || (loudEqInstructions->eqSetId[e] == ID_FOR_ANY_EQ)) {
                                        *loudEqIdSelected = loudEqInstructions->loudEqSetId;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return (0);
}

int
matchEqSet(UniDrcConfig* uniDrcConfig,
           const int downmixId,
           const int drcSetId,
           const int* eqSetIdIsPermitted,
           int* matchingEqSetCount,
           int* matchingEqSetIndex)
{
    EqInstructions* eqInstructions = NULL;
    int i, k, n;
    int match = FALSE;
    *matchingEqSetCount = 0;
    for (i=0; i<uniDrcConfig->uniDrcConfigExt.eqInstructionsCount; i++)
    {
        eqInstructions = &uniDrcConfig->uniDrcConfigExt.eqInstructions[i];

        if (eqInstructions->dependsOnEqSetPresent == 0) {
            if (eqInstructions->noIndependentEqUse == 1) continue;
        }
        if (eqSetIdIsPermitted[eqInstructions->eqSetId] == 0) continue;
        for (k=0; k<eqInstructions->downmixIdCount; k++) {
            if ((eqInstructions->downmixId[k] == ID_FOR_ANY_DOWNMIX) || (downmixId == eqInstructions->downmixId[k])) {
                for (n=0; n<eqInstructions->drcSetIdCount; n++) {
                    if ((eqInstructions->drcSetId[n] == ID_FOR_ANY_DRC) || (drcSetId == eqInstructions->drcSetId[n])) {
                        match = TRUE;
                        matchingEqSetIndex[*matchingEqSetCount] = i;
                        (*matchingEqSetCount)++;
                    }
                }
            }
        }
    }
    return(0);
}

int
matchEqSetPurpose(UniDrcConfig* uniDrcConfig,
                  const int eqSetIdRequested,
                  const int eqSetPurposeRequested,
                  const int* eqSetIdIsPermitted,
                  int* matchFound)
{
    int i;
    EqInstructions* eqInstructions = NULL;
    *matchFound = FALSE;


    for (i=0; i<uniDrcConfig->uniDrcConfigExt.eqInstructionsCount; i++)
    {
        eqInstructions = &uniDrcConfig->uniDrcConfigExt.eqInstructions[i];

        if (eqInstructions->dependsOnEqSetPresent == 0) {
            if (eqSetIdIsPermitted[eqInstructions->eqSetId] == 0) continue;
        }
        if (eqSetIdIsPermitted[eqInstructions->eqSetId] == 0) continue;
        if ((eqInstructions->eqSetId == eqSetIdRequested) && (eqInstructions->eqSetPurpose & eqSetPurposeRequested))
        {
            *matchFound = TRUE;
        }
    }
    return(0);
}

int
findEqSetsForNoCompression(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                           const int downmixIdRequested,
                           int* noCompressionEqCount,
                           int* noCompressionEqId)
{
    int i, d, k, c;
    k=0;
    for (i=0; i<hUniDrcConfig->uniDrcConfigExt.eqInstructionsCount; i++)
    {
        EqInstructions* eqInstructions = &hUniDrcConfig->uniDrcConfigExt.eqInstructions[i];
        for (d=0; d<eqInstructions->downmixIdCount; d++)
        {
            if (downmixIdRequested == eqInstructions->downmixId[d])
            {
                for (c=0; c<eqInstructions->drcSetIdCount; c++)
                {
                    if ((eqInstructions->drcSetId[c] == ID_FOR_ANY_DRC) || (eqInstructions->drcSetId[c] == 0))
                    {
                        noCompressionEqId[k] = eqInstructions->eqSetId;
                        k++;
                    }
                }
            }
        }
    }
    *noCompressionEqCount = k;
    return (0);
}

#endif /* MPEG_D_DRC_EXTENSION_V1 */

int
selectDrcsWithoutCompressionEffects (HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                                     int* matchFound,
                                     int* selectionCandidateCount,
                                     SelectionCandidateInfo* selectionCandidateInfo)
{
    int i, k, n;
    int selectionCandidateStep2Count=0;
    SelectionCandidateInfo selectionCandidateInfoStep2[SELECTION_CANDIDATE_COUNT_MAX];
    int effectTypesRequestableSize;
    int match;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;

    effectTypesRequestableSize = sizeof(effectTypesRequestable) / sizeof(int);

    k=0;
    for (i=0; i < *selectionCandidateCount; i++)
    {
        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[i].drcInstructionsIndex]);

        match = TRUE;
        for (n=0; n<effectTypesRequestableSize; n++)
        {
            if ((drcInstructionsUniDrc->drcSetEffect & effectTypesRequestable[n]) != 0x0)
            {
                match = FALSE;
            }
        }
        if (match == TRUE)
        {
            memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
            k++;
        }
    }
    selectionCandidateStep2Count = k;

    if (selectionCandidateStep2Count > 0)
    {
        *matchFound = TRUE;
        for (i=0; i<selectionCandidateStep2Count; i++)
        {
            memcpy(&selectionCandidateInfo[i], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
            *selectionCandidateCount = selectionCandidateStep2Count;
        }
    }
    else
    {
        *matchFound = FALSE;
    }

    return (0);
}

int
matchEffectTypeAttempt(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       const int effectTypeRequested,
                       const int stateRequested,
                       int* matchFound,
                       int* selectionCandidateCount,
                       SelectionCandidateInfo* selectionCandidateInfo)
{
    int i, k, err;
    int selectionCandidateStep2Count=0;
    SelectionCandidateInfo selectionCandidateInfoStep2[SELECTION_CANDIDATE_COUNT_MAX];
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    DrcInstructionsUniDrc* drcInstructionsDependent;
    int effectBitIndex;

    err = mapRequestedEffectToBitIndex(effectTypeRequested, &effectBitIndex);
    if (err) return (err);

    if (effectBitIndex == EFFECT_BIT_NONE)
    {
        err = selectDrcsWithoutCompressionEffects (hUniDrcConfig, matchFound, selectionCandidateCount, selectionCandidateInfo);
        if (err) return (err);
    }
    else
    {
        k=0;
        for (i=0; i < *selectionCandidateCount; i++)
        {
            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[i].drcInstructionsIndex]);
            if (drcInstructionsUniDrc->dependsOnDrcSetPresent == TRUE)
            {
                err = getDrcInstructionsDependent(hUniDrcConfig, drcInstructionsUniDrc, &drcInstructionsDependent);
                if (err) return (err);

                if (stateRequested == TRUE)
                {
                    if (((drcInstructionsUniDrc->drcSetEffect & effectBitIndex) != 0x0) ||
                        ((drcInstructionsDependent->drcSetEffect & effectBitIndex) != 0x0))
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                else
                {
                    if (((drcInstructionsUniDrc->drcSetEffect & effectBitIndex) == 0x0) &&
                        ((drcInstructionsDependent->drcSetEffect & effectBitIndex) == 0x0))
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
            }
            else
            {
                if (stateRequested == TRUE)
                {
                    if ((drcInstructionsUniDrc->drcSetEffect & effectBitIndex) != 0x0)
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                else
                {
                    if ((drcInstructionsUniDrc->drcSetEffect & effectBitIndex) == 0x0)
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
            }
        }
        selectionCandidateStep2Count = k;

        if (selectionCandidateStep2Count > 0)
        {
            *matchFound = TRUE;
            for (i=0; i<selectionCandidateStep2Count; i++)
            {
                *selectionCandidateCount = selectionCandidateStep2Count;
                memcpy(&selectionCandidateInfo[i], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
            }
        }
        else
        {
            *matchFound = FALSE;
        }
    }
    return(0);
}

int
matchEffectTypes(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                 const int effectTypeRequestedTotalCount,
                 const int effectTypeRequestedDesiredCount,
                 const int* effectTypeRequested,
                 int* selectionCandidateCount,
                 SelectionCandidateInfo* selectionCandidateInfo)
{
    int k, err;
    int matchFound = FALSE;
    int stateRequested;
    int desiredEffectTypeFound, fallbackEffectTypeFound;

    desiredEffectTypeFound = FALSE;
    fallbackEffectTypeFound = FALSE;
    k = 0;
    while (k<effectTypeRequestedDesiredCount)
    {
        stateRequested = TRUE;
        err = matchEffectTypeAttempt(hUniDrcConfig, effectTypeRequested[k], stateRequested, &matchFound, selectionCandidateCount, selectionCandidateInfo);
        if (err) return (err);
        if (matchFound) desiredEffectTypeFound = TRUE;
        k++;
    }
    if (desiredEffectTypeFound == FALSE)
    {
        while ((k<effectTypeRequestedTotalCount) && (matchFound == FALSE))
        {
            stateRequested = TRUE;
            err = matchEffectTypeAttempt(hUniDrcConfig, effectTypeRequested[k], stateRequested, &matchFound, selectionCandidateCount, selectionCandidateInfo);
            if (err) return (err);
            if (matchFound) fallbackEffectTypeFound = TRUE;
            k++;
        }
    }
    if ((desiredEffectTypeFound == FALSE) && (fallbackEffectTypeFound == FALSE))
    {
#if DEBUG_WARNINGS
        fprintf(stderr, "WARNING: no DRC found that matches the requested effect type(s). Request ignored.\n");
#endif
    }

    return(0);
}

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
                  SelectionCandidateInfo* selectionCandidateInfo)
{
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    int err, i, k;
    int loudnessPeakToAverageValuePresent;
    float loudnessPeakToAverageValue;
    float deviationMin = 1000.0f;
    int selected[DRC_INSTRUCTIONS_COUNT_MAX];
    k=0;
    for (i=0; i < *selectionCandidateCount; i++)
    {
        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[i].drcInstructionsIndex]);

        err = getLoudnessPeakToAverageValue(
                                            hLoudnessInfoSet,
                                            drcInstructionsUniDrc,
                                            downmixIdRequested[selectionCandidateInfo[i].downmixIdRequestIndex],
                                            dynamicRangeMeasurementType,
                                            albumMode,
                                            &loudnessPeakToAverageValuePresent,
                                            &loudnessPeakToAverageValue);
        if (err) return (err);

        if (loudnessPeakToAverageValuePresent == TRUE)
        {
            if (dynamicRangeRequestedIsRange == TRUE)
            {
                if ((loudnessPeakToAverageValue >= dynamicRangeMinRequested) && (loudnessPeakToAverageValue <= dynamicRangeMaxRequested))
                {
                    selected[k] = i;
                    k++;
                }
            }
            else
            {
                float deviation = (float)fabs((double)(dynamicRangeRequested - loudnessPeakToAverageValue));
                if (deviationMin >= deviation)
                {
                    if (deviationMin > deviation)
                    {
                        deviationMin = deviation;
                        k = 0;
                    }
                    selected[k] = i;
                    k++;
                }
            }
        }
    }
    if (k>0)
    {
        for (i=0; i<k; i++)
        {
            memcpy(&selectionCandidateInfo[i], &selectionCandidateInfo[selected[i]], sizeof(SelectionCandidateInfo));
        }
        *selectionCandidateCount = k;
    }
    else
    {
#if DEBUG_WARNINGS
        fprintf(stderr, "WARNING: DRC selection found no DRC set matching the requested dynamic range. Request is ignored.\n");
#endif
    }
    return(0);
}

int
matchDrcCharacteristicAttempt(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                              const int drcCharacteristicRequest,
                              int* matchFound,
                              int* selectionCandidateCount,
                              SelectionCandidateInfo* selectionCandidateInfo)
{
    int i, k, n, b, m;
    int refCount;
    int drcCharacteristic;
    float matchCount;
    int drcCharacteristicRequest1;
    int drcCharacteristicRequest2;
    int drcCharacteristicRequest3;

    DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;
    DrcCoefficientsUniDrc* drcCoefficientsUniDrc = NULL;
    GainSetParams* gainSetParams                 = NULL;
    *matchFound = FALSE;

    if (drcCharacteristicRequest < 1)
    {
        fprintf(stderr, "ERROR: Requested DRC characteristic index must be 1 or larger.\n");
        return(UNEXPECTED_ERROR);
    }
    if(drcCharacteristicRequest < 12)
    {
        drcCharacteristicRequest1 = drcCharacteristicOrderDefault[drcCharacteristicRequest-1][0];
        drcCharacteristicRequest2 = drcCharacteristicOrderDefault[drcCharacteristicRequest-1][1];
        drcCharacteristicRequest3 = drcCharacteristicOrderDefault[drcCharacteristicRequest-1][2];
    }
    else
    {
        drcCharacteristicRequest1 = drcCharacteristicRequest;
        drcCharacteristicRequest2 = -1;
        drcCharacteristicRequest3 = -1;
    }

    if (hUniDrcConfig->drcCoefficientsUniDrcCount) {
        for (i=0; i<hUniDrcConfig->drcCoefficientsUniDrcCount; i++)
        {
            drcCoefficientsUniDrc = &(hUniDrcConfig->drcCoefficientsUniDrc[i]);
            if (drcCoefficientsUniDrc->drcLocation == LOCATION_SELECTED) break;
            /* TODO: support of ISOBMFF also includes drcCoefficientsUniDrc with drcLocation<0 */
        }

        if (i == hUniDrcConfig->drcCoefficientsUniDrcCount)
        {
            fprintf(stderr, "ERROR: no drcCoefficientsUniDrc found for drcLocation LOCATION_SELECTED (uniDrc)\n");
            return (UNEXPECTED_ERROR);
        }
    }

    n = 0;
    for (i=0; i < *selectionCandidateCount; i++)
    {
        refCount = 0;
        matchCount = 0;

        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[i].drcInstructionsIndex]);
        for (k=0; k<drcInstructionsUniDrc->nDrcChannelGroups; k++)
        {
            gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[k]]);
            for (b=0; b<gainSetParams->bandCount; b++)
            {
                refCount++;
                drcCharacteristic = gainSetParams->gainParams[b].drcCharacteristic;
                if      ( drcCharacteristic == drcCharacteristicRequest1) matchCount += 1.0f;
                else if ( drcCharacteristic == drcCharacteristicRequest2) matchCount += 0.75f;
                else if ( drcCharacteristic == drcCharacteristicRequest3) matchCount += 0.5f;
            }
        }
        if (drcInstructionsUniDrc->dependsOnDrcSetPresent == TRUE)
        {
            int dependsOnDrcSet = drcInstructionsUniDrc->dependsOnDrcSet;
            for (m=0; m<hUniDrcConfig->drcInstructionsUniDrcCount; m++)
            {
                if (hUniDrcConfig->drcInstructionsUniDrc[m].drcSetId == dependsOnDrcSet) break;
            }
            if (m == hUniDrcConfig->drcInstructionsUniDrcCount)
            {
                fprintf(stderr, "ERROR: depending DRC set not present. dependsOnDrcSet = %d\n", dependsOnDrcSet);
                return (UNEXPECTED_ERROR);
            }
            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[m]);
            if ((drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_FADE | EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) == 0)
            {
                if (drcInstructionsUniDrc->drcSetEffect != EFFECT_BIT_CLIPPING) {
                    for (k=0; k<drcInstructionsUniDrc->nDrcChannelGroups; k++)
                    {
                        gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[k]]);
                        for (b=0; b<gainSetParams->bandCount; b++)
                        {
                            refCount++;
                            drcCharacteristic = gainSetParams->gainParams[b].drcCharacteristic;
                            if      ( drcCharacteristic == drcCharacteristicRequest1) matchCount += 1.0f;
                            else if ( drcCharacteristic == drcCharacteristicRequest2) matchCount += 0.75f;
                            else if ( drcCharacteristic == drcCharacteristicRequest3) matchCount += 0.5;
                        }
                    }
                }
            }
        }
        if ((refCount > 0) && (((float)matchCount) > 0.5f * refCount))
        {
            memcpy(&selectionCandidateInfo[n], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
            n++;
        }
    }
    if (n > 0)
    {
        *selectionCandidateCount = n;
        *matchFound = TRUE;
    }
    else
    {
#if DEBUG_WARNINGS
        fprintf(stderr, "WARNING: no DRC set found that matches the requested DRC characteristic. Request was ignored.\n");
#endif
    }

    return(0);
}

int
matchDrcCharacteristic(HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                       const int drcCharacteristicRequested,
                       int* selectionCandidateCount,
                       SelectionCandidateInfo* selectionCandidateInfo)
{
    int k, err;
    int matchFound = FALSE;

    int* drcCharacteristicOrder = drcCharacteristicOrderDefault[drcCharacteristicRequested-1];
    int drcCharacteristicOrderCount = sizeof(drcCharacteristicOrderDefault[drcCharacteristicRequested]) / sizeof(int);
    k = 0;
    while ((k<drcCharacteristicOrderCount) && (matchFound == FALSE) && (drcCharacteristicOrder[k] > 0))
    {
        err = matchDrcCharacteristicAttempt(hUniDrcConfig,
                                            drcCharacteristicOrder[k],
                                            &matchFound,
                                            selectionCandidateCount,
                                            selectionCandidateInfo);
        if (err) return (err);
        k++;
    }
    return (0);
}

/* this function exists in 3 identical copies in different libraries. */
int
selectDrcCoefficients3(UniDrcConfig* uniDrcConfig,
                      const int location,
                      DrcCoefficientsUniDrc** drcCoefficientsUniDrc)
{
    int n;
    int cV1 = -1;
    int cV0 = -1;
    for(n=0; n<uniDrcConfig->drcCoefficientsUniDrcCount; n++)
    {
        if (uniDrcConfig->drcCoefficientsUniDrc[n].drcLocation == location)
        {
            if (uniDrcConfig->drcCoefficientsUniDrc[n].version == 0)
            {
                cV0 = n;
            }
            else
            {
                cV1 = n;
            }
        }
    }
#if MPEG_D_DRC_EXTENSION_V1 || AMD1_SYNTAX
    if (cV1 >= 0) {
        *drcCoefficientsUniDrc = &(uniDrcConfig->drcCoefficientsUniDrc[cV1]);
    }
    else if (cV0 >= 0) {
        *drcCoefficientsUniDrc = &(uniDrcConfig->drcCoefficientsUniDrc[cV0]);
    }
    else {
        *drcCoefficientsUniDrc = NULL; /* parametric DRC only (after bitstream parsing is complete this condition is redundant --> see generateVirtualGainSetsForParametricDrc() */
    }
#else
    if (cV0 >= 0) {
        *drcCoefficientsUniDrc = &(uniDrcConfig->drcCoefficientsUniDrc[cV0]);
    }
    else
    {
        *drcCoefficientsUniDrc = NULL; /* only loudnessInfoSet() */
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 || AMD1_SYNTAX */
    return (0);
}

#if MPEG_D_DRC_EXTENSION_V1

    /* Preliminary elimination of DRCs that are too complexity */
    /* More may be eliminated after the selection process */
int
manageDrcComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                     HANDLE_UNI_DRC_CONFIG hUniDrcConfig)
{
    int i, j, err, channelCount;
    int numBandsTooLarge = 0;
    float complexityDrcPrelim;
    DrcCoefficientsUniDrc* drcCoefficientsUniDrc;
    float complexitySupportedTotal = pow(2.0f, hUniDrcSelProcStruct->complexityLevelSupportedTotal);
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    DrcInstructionsUniDrc* drcInstructionsUniDrcDependent;
    HANDLE_UNI_DRC_SEL_PROC_OUTPUT uniDrcSelProcOutput = &hUniDrcSelProcStruct->uniDrcSelProcOutput;
    HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams = &hUniDrcSelProcStruct->uniDrcSelProcParams;

    err = selectDrcCoefficients3(hUniDrcConfig, LOCATION_SELECTED, &drcCoefficientsUniDrc);
    if (err) return err;

    for (i=0; i<hUniDrcConfig->drcInstructionsUniDrcCount; i++)
    {
        drcInstructionsUniDrc = &hUniDrcConfig->drcInstructionsUniDrc[i];
        if (drcInstructionsUniDrc->noIndependentUse) continue;

        numBandsTooLarge = 0;
#if AMD2_COR3
        if (uniDrcSelProcOutput->targetChannelCount == 0) {
            if (drcInstructionsUniDrc->drcApplyToDownmix == 1)
            {
                channelCount = hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountPrelim;
            }
            else
            {
                channelCount = hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount;
            }
        }
        else
#endif
        {
            if (drcInstructionsUniDrc->drcApplyToDownmix == 1)
            {
                channelCount = uniDrcSelProcOutput->targetChannelCount;
            }
            else
            {
                channelCount = uniDrcSelProcOutput->baseChannelCount;
            }
        }
        if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
        {
            for (j=0; j<drcInstructionsUniDrc->nDrcChannelGroups; j++)
            {
                GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[j]]);
                if (gainSetParams->bandCount > hUniDrcSelProcParams->numBandsSupported)
                {
                    numBandsTooLarge = 1;
                }
                else
                {
                    if (gainSetParams->bandCount > 4)
                    {
                        /* Add complexity for analysis and synthesis QMF bank here, if supported */
                    }
                }
            }
        }
        complexityDrcPrelim = channelCount * (1 << drcInstructionsUniDrc->drcSetComplexityLevel);

        if (drcInstructionsUniDrc->dependsOnDrcSet > 0)
        {
            err = findDrcInstructionsUniDrcForId(hUniDrcConfig, drcInstructionsUniDrc->dependsOnDrcSet, &drcInstructionsUniDrcDependent);
            if (err) return (err);
#if AMD2_COR3
            if (uniDrcSelProcOutput->targetChannelCount == 0) {
                if (drcInstructionsUniDrc->drcApplyToDownmix == 1)
                {
                    channelCount = hUniDrcSelProcStruct->uniDrcSelProcParams.targetChannelCountPrelim;
                }
                else
                {
                    channelCount = hUniDrcSelProcStruct->uniDrcSelProcParams.baseChannelCount;
                }
            }
            else
#endif
            {
                if (drcInstructionsUniDrcDependent->drcApplyToDownmix == 1)
                {
                    channelCount = uniDrcSelProcOutput->targetChannelCount;
                }
                else
                {
                    channelCount = uniDrcSelProcOutput->baseChannelCount;
                }
            }
            if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
            {
                for (j=0; j<drcInstructionsUniDrc->nDrcChannelGroups; j++)
                {
                    GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrcDependent->gainSetIndexForChannelGroup[j]]);
                    if (gainSetParams->bandCount > hUniDrcSelProcParams->numBandsSupported)
                    {
                        numBandsTooLarge = 1;
                    }
                    else
                    {
                        if (gainSetParams->bandCount > 4)
                        {
                            /* Add complexity for analysis and synthesis QMF bank here, if supported */
                        }
                    }
                }
            }
            complexityDrcPrelim += channelCount * (1 << drcInstructionsUniDrcDependent->drcSetComplexityLevel);
        }

        complexityDrcPrelim *= hUniDrcConfig->sampleRate / 48000.0;

        if ((complexityDrcPrelim <= complexitySupportedTotal) && (numBandsTooLarge == 0))
        {
            hUniDrcSelProcStruct->drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId] = 1;
        }
    }
    return (0);
}

    /* Preliminary elimination of EQs that are too complexity */
    /* More may be eliminated after the selection process */
int
manageEqComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                    HANDLE_UNI_DRC_CONFIG hUniDrcConfig)
{
    int k, n, m, err;
    int eqComplexityPrimary = 0;
    int eqComplexityDependent = 0;
    int eqChannelCountPrimary = 0, eqChannelCountDependent = 0;
    float complexityTotalEq;
    UniDrcConfig* uniDrcConfig = &hUniDrcSelProcStruct->uniDrcConfig;
    HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams = &hUniDrcSelProcStruct->uniDrcSelProcParams;
    float complexitySupportedTotal = pow(2.0f, hUniDrcSelProcStruct->complexityLevelSupportedTotal);

    for (n=0; n<uniDrcConfig->uniDrcConfigExt.eqInstructionsCount; n++)
    {
        EqInstructions* eqInstructions = &hUniDrcConfig->uniDrcConfigExt.eqInstructions[n];

        eqChannelCountPrimary = hUniDrcSelProcParams->baseChannelCount;
        eqChannelCountDependent = hUniDrcSelProcParams->baseChannelCount;

        eqComplexityPrimary = 1 << eqInstructions->eqSetComplexityLevel;
        if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
        {
            if (eqInstructions->tdFilterCascadePresent == 0)
            {
                eqComplexityPrimary = 0;
            }
        }
        else
        {
            if (eqInstructions->tdFilterCascadePresent == 1)
            {
                eqComplexityPrimary = 2.5;
            }
        }
        if (eqInstructions->eqApplyToDownmix == 1)
        {
            if (eqInstructions->downmixId[0] == ID_FOR_ANY_DOWNMIX)
            {
                eqChannelCountPrimary = hUniDrcSelProcParams->targetChannelCountPrelim;
            }
            else
            {
                for (k=0; k<hUniDrcConfig->downmixInstructionsCount; k++)
                {
                    for (m=0; m<eqInstructions->downmixIdCount; m++)
                    {
                        if (hUniDrcConfig->downmixInstructions[k].downmixId == eqInstructions->downmixId[m])
                        {
                            if (eqChannelCountPrimary > hUniDrcConfig->downmixInstructions[k].targetChannelCount) {
                                eqChannelCountPrimary = hUniDrcConfig->downmixInstructions[k].targetChannelCount;
                            }
                        }
                    }
                }
            }
        }
        complexityTotalEq = eqChannelCountPrimary * eqComplexityPrimary;

        if (eqInstructions->dependsOnEqSetPresent > 0)
        {
            EqInstructions* eqInstructionsDependent;
            err = findEqInstructionsForId(uniDrcConfig, eqInstructions->dependsOnEqSet, &eqInstructionsDependent);
            if (err) return (err);
            eqComplexityDependent = 1 << eqInstructionsDependent->eqSetComplexityLevel;
            if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
            {
                if (eqInstructions->tdFilterCascadePresent == 0)
                {
                    eqComplexityDependent = 0;
                }
            }
            else
            {
                if (eqInstructions->tdFilterCascadePresent == 1)
                {
                    eqComplexityDependent = 2.5;
                }
            }
            if (eqInstructionsDependent->eqApplyToDownmix == 1)
            {
                if (eqInstructionsDependent->downmixId[0] == ID_FOR_ANY_DOWNMIX)
                {
                    eqChannelCountDependent = hUniDrcSelProcParams->targetChannelCountPrelim;
                }
                else
                {
                    for (k=0; k<hUniDrcConfig->downmixInstructionsCount; k++)
                    {
                        for (m=0; m<eqInstructions->downmixIdCount; m++)
                        {
                            if (hUniDrcConfig->downmixInstructions[k].downmixId == eqInstructionsDependent->downmixId[m])
                            {
                                if (eqChannelCountDependent > hUniDrcConfig->downmixInstructions[k].targetChannelCount) {
                                    eqChannelCountDependent = hUniDrcConfig->downmixInstructions[k].targetChannelCount;
                                }
                            }
                        }
                    }
                }
            }
            complexityTotalEq += eqChannelCountDependent * eqComplexityDependent;
        }

        hUniDrcSelProcStruct->eqSetIdIsPermitted[eqInstructions->eqSetId] = 0;
#if EQ_IS_SUPPORTED
        complexityTotalEq *= hUniDrcConfig->sampleRate / 48000.0;

        if (complexityTotalEq <= complexitySupportedTotal)
        {
            hUniDrcSelProcStruct->eqSetIdIsPermitted[eqInstructions->eqSetId] = 1;
        }
#endif
    }
    return 0;
}


int
manageComplexity (HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                  HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                  int* repeatSelection)
{
    int i, j, p, err;
    int channelCount;
    int numBandsTooLarge = 0;
    int drcRequiresEq;
    float complexityEq;
    float complexityDrcTotal = 0.0f;
    float complexityEqTotal = 0.0f;
    float freqNorm = hUniDrcConfig->sampleRate / 48000.0f;
    DrcCoefficientsUniDrc* drcCoefficientsUniDrc;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    HANDLE_UNI_DRC_SEL_PROC_OUTPUT uniDrcSelProcOutput = &hUniDrcSelProcStruct->uniDrcSelProcOutput;
    HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams = &hUniDrcSelProcStruct->uniDrcSelProcParams;
    float complexitySupportedTotal = pow(2.0f, hUniDrcSelProcStruct->complexityLevelSupportedTotal);

    err = selectDrcCoefficients3(hUniDrcConfig, LOCATION_SELECTED, &drcCoefficientsUniDrc);
    if (err) return err;

    /* estimation of total DRC processing complexity */
    for (p=0; p<4; p++)   /* 0: primary DRC, 1: dependent DRC, 2: fading DRC, 3: ducking DRC */
    {
        if (hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p] <= 0) continue;
        err = findDrcInstructionsUniDrcForId(hUniDrcConfig, hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p], &drcInstructionsUniDrc);
        if (err) return (err);

        if (drcInstructionsUniDrc->drcApplyToDownmix == 1)
        {
            channelCount = uniDrcSelProcOutput->targetChannelCount;
        }
        else
        {
            channelCount = uniDrcSelProcOutput->baseChannelCount;
        }
        if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
        {
            for (j=0; j<drcInstructionsUniDrc->nDrcChannelGroups; j++)
            {
                GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[j]]);
                if (gainSetParams->bandCount > hUniDrcSelProcParams->numBandsSupported)
                {
                    if (p<2) {
                        numBandsTooLarge = 1;
                    }
                    else {
                        /* Disable ducking / fading DRC */
                        fprintf(stderr, "WARNING: Ducking or fading DRC exceeeds supported band count. DRCSetID=%d disabled.\n", drcInstructionsUniDrc->drcSetId);
                        hUniDrcSelProcStruct->drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId] = 0;
                        hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p] = 0;
                    }
                }
                else
                {
                    if (gainSetParams->bandCount > 4)
                    {
                        /* Add complexity for analysis and synthesis QMF bank here, if supported */
                    }
                }
            }
        }
        complexityDrcTotal += channelCount * (1 << drcInstructionsUniDrc->drcSetComplexityLevel);
    }

    /* estimation of downmixing complexity */
    if (uniDrcSelProcOutput->activeDownmixId > 0)
    {
        float complexityPerCoeff;
        DownmixInstructions* downmixInstructions;

        if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
        {
            complexityPerCoeff = 1.0f;
        }
        else
        {
            complexityPerCoeff = 2.0f;
        }
        findDownmixForId(hUniDrcConfig, uniDrcSelProcOutput->activeDownmixId, &downmixInstructions);
        if (downmixInstructions->downmixCoefficientsPresent == 1)
        {
            for (i=0; i<uniDrcSelProcOutput->baseChannelCount; i++)
            {
                for (j=0; j<uniDrcSelProcOutput->targetChannelCount; j++)
                {
                    if (uniDrcSelProcOutput->downmixMatrix[i][j] != 0.0f)
                    {
                        complexityDrcTotal += complexityPerCoeff;
                    }
                }
            }
        }
        else
        {
            /* add standard downmix here */
        }
    }

    /* estimation of total EQ processing complexity */
    for (p=0; p<2; p++)
    {
        if(hUniDrcSelProcStruct->subEq[p].eqInstructionsIndex >= 0)
        {
            EqInstructions* eqInstructions = &hUniDrcConfig->uniDrcConfigExt.eqInstructions[hUniDrcSelProcStruct->subEq[p].eqInstructionsIndex];
            if (p==0)
            {
                channelCount = uniDrcSelProcOutput->baseChannelCount;
            }
            else
            {
                channelCount = uniDrcSelProcOutput->targetChannelCount;
            }

            complexityEq = 1 << eqInstructions->eqSetComplexityLevel;
            if (hUniDrcSelProcStruct->subbandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
            {
                if (eqInstructions->tdFilterCascadePresent == 0)
                {
                    complexityEq = 0.0;
                }
            }
            else
            {
                if (eqInstructions->tdFilterCascadePresent == 1)
                {
                    complexityEq = 2.5;
                }
            }

            complexityEqTotal += channelCount * complexityEq;
        }
    }

    complexityDrcTotal *= freqNorm;
    complexityEqTotal *= freqNorm;

    if (numBandsTooLarge == 1)
    {
        /* remove primary DRC and reiterate selection process */
        if (hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[0] > 0)
        {
            err = findDrcInstructionsUniDrcForId(hUniDrcConfig, hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[0], &drcInstructionsUniDrc);
            if (err) return (err);

            hUniDrcSelProcStruct->drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId] = 0;
        }
        *repeatSelection = 1;
    }
    else
    {
        if (complexityDrcTotal + complexityEqTotal <= complexitySupportedTotal)
        {
            *repeatSelection = 0;
        }
        else
        {
            drcRequiresEq = 0;
            for (p=0; p<2; p++)   /* 0: primary DRC, 1: dependent DRC */
            {
                if (hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p] <= 0) continue;
                err = findDrcInstructionsUniDrcForId(hUniDrcConfig, hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p], &drcInstructionsUniDrc);
                if (err) return (err);
                if (drcInstructionsUniDrc->requiresEq == 1)
                {
                    drcRequiresEq = 1;
                }
            }
            if ((drcRequiresEq == 0) && (complexityDrcTotal <= complexitySupportedTotal))
            {
                /* disable EQ */
                for (p=0; p<2; p++)
                {
                    hUniDrcSelProcStruct->subEq[p].eqInstructionsIndex = 0;
                }
                *repeatSelection = 0;
            }
            else
            {
                /* remove primary DRC and reiterate selection process */
                if (hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[0] > 0)
                {
                    err = findDrcInstructionsUniDrcForId(hUniDrcConfig, hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[0], &drcInstructionsUniDrc);
                    if (err) return (err);

                    hUniDrcSelProcStruct->drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId] = 0;
                }
                else
                {
                    for (p=2; p<4; p++) {
                        /* Disable ducking / fading DRC */
                        fprintf(stderr, "WARNING: Ducking or fading DRC exceeeds supported complexity level. DRCSetID=%d disabled.\n", drcInstructionsUniDrc->drcSetId);
                        hUniDrcSelProcStruct->drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId] = 0;
                        hUniDrcSelProcStruct->uniDrcSelProcOutput.selectedDrcSetIds[p] = 0;
                    }
                }
                *repeatSelection = 1;
            }
        }
    }

    if (*repeatSelection == 1)
    {
        /* discard output */
        memset (&hUniDrcSelProcStruct->uniDrcSelProcOutput, 0, sizeof(UniDrcSelProcOutput));
    }
    return (0);
}
#endif

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
                   SelectionCandidateInfo* selectionCandidateInfo)
{
    int i, j, k, l, d, n, err;
    int downmixIdMatch   = 0;
    int location = LOCATION_SELECTED;

#if MPEG_H_SYNTAX
    int groupIdRequested;
    int groupPresetIdRequested;
    int omitPreSelectionBasedOnRequestedGroupId;
#endif
    int selectionCandidateStep2Count;
    SelectionCandidateInfo selectionCandidateInfoStep2[SELECTION_CANDIDATE_COUNT_MAX];

    int numDownmixIdRequests = hUniDrcSelProcParams->numDownmixIdRequests;
    int* downmixIdRequested  = hUniDrcSelProcParams->downmixIdRequested;
    float outputPeakLevelMax = hUniDrcSelProcParams->outputPeakLevelMax;
    int loudnessDeviationMax = hUniDrcSelProcParams->loudnessDeviationMax;

    float outputPeakLevelMin = 1000.0f;
    float adjustment;
    int loudnessDrcSetIdRequested;

    int noCompressionEqCount = 0;
    int noCompressionEqId[16];

    int loudnessInfoCount = 0;
    int eqSetIdForLoudness[16];
    float loudnessNormalizationGainDb[16];
    float loudness[16];
    int peakInfoCount;
    int eqSetIdForPeak[16];
    float signalPeakLevel[16];
    int explicitPeakInformationPresent[16];

    DrcCoefficientsUniDrc* drcCoefficientsUniDrc = NULL;
    DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;

    err = selectDrcCoefficients3(hUniDrcConfig, location, &drcCoefficientsUniDrc);
    if (err) return err;
    /* TODO: support of ISOBMFF also includes drcCoefficientsUniDrc with drcLocation<0 */

    /* DRC set pre-selection according to Section 6.3.2 of ISO/IEC 23003-4 */
    k=0;
    for (d=0; d<numDownmixIdRequests; d++)
    {
#if MPEG_D_DRC_EXTENSION_V1
        /* find EQ sets applicable to configurations with no compression */
        err = findEqSetsForNoCompression(hUniDrcConfig,
                                         downmixIdRequested[d],
                                         &noCompressionEqCount,
                                         noCompressionEqId);
        if (err) return (err);
#endif
        for (i=0; i<hUniDrcConfig->drcInstructionsCountPlus; i++)
        {
            downmixIdMatch = FALSE;
            /* TODO: check if location == LOCATION_SELECTED */
            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[i]);

            /* downmixId of DRC set */
            for (j=0; j<drcInstructionsUniDrc->downmixIdCount; j++)
            {
                if ((drcInstructionsUniDrc->downmixId[j] == downmixIdRequested[d]) ||
                    ((drcInstructionsUniDrc->downmixId[j] == ID_FOR_BASE_LAYOUT) && (drcInstructionsUniDrc->drcSetId > 0)) ||
                    (drcInstructionsUniDrc->downmixId[j] == ID_FOR_ANY_DOWNMIX))
                {
                    downmixIdMatch = TRUE;
                }
            }
            if (downmixIdMatch == TRUE)                                                        /* preselection step #1,2,3 */
            {
                if (hUniDrcSelProcParams->dynamicRangeControlOn == 1)
                {
                    if ((drcInstructionsUniDrc->drcSetEffect != EFFECT_BIT_FADE) &&         /* preselection step #4 */
                        (drcInstructionsUniDrc->drcSetEffect != EFFECT_BIT_DUCK_OTHER) &&
                        (drcInstructionsUniDrc->drcSetEffect != EFFECT_BIT_DUCK_SELF) &&
                        (drcInstructionsUniDrc->drcSetEffect != 0 || drcInstructionsUniDrc->drcSetId < 0) &&       /* preselection step #4 */
                        (((drcInstructionsUniDrc->dependsOnDrcSetPresent == 0) && (drcInstructionsUniDrc->noIndependentUse == 0)) ||  /* preselection step #6 */
                         (drcInstructionsUniDrc->dependsOnDrcSetPresent == 1)) )
                    {
                        int drcIsPermitted = 1;
 #if MPEG_D_DRC_EXTENSION_V1
                        if (drcInstructionsUniDrc->drcSetId > 0)
                        {
                            drcIsPermitted = drcSetIdIsPermitted[drcInstructionsUniDrc->drcSetId];
                        }
#else
                        for (j=0; j<drcInstructionsUniDrc->nDrcChannelGroups; j++)
                        {
                            GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[j]]);
                            if (gainSetParams->bandCount > hUniDrcSelProcParams->numBandsSupported)
                            {
                                drcIsPermitted = 0;
                            }
                        }
                        if (drcInstructionsUniDrc->dependsOnDrcSet > 0)
                        {
                            DrcInstructionsUniDrc* drcInstructionsUniDrcDependent;
                            err = findDrcInstructionsUniDrcForId(hUniDrcConfig, drcInstructionsUniDrc->dependsOnDrcSet, &drcInstructionsUniDrcDependent);
                            if (err) return (err);
                            for (j=0; j<drcInstructionsUniDrc->nDrcChannelGroups; j++)
                            {
                                GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrcDependent->gainSetIndexForChannelGroup[j]]);
                                if (gainSetParams->bandCount > hUniDrcSelProcParams->numBandsSupported)
                                {
                                    drcIsPermitted = 0;
                                }
                            }
                        }
#endif
                        if (drcIsPermitted == 1)                                       /* preselection step #5 */
                        {
#if MPEG_H_SYNTAX
							if (drcInstructionsUniDrc->drcInstructionsType == 3) {
								groupIdRequested = -1;
								groupPresetIdRequested = drcInstructionsUniDrc->mae_groupPresetID;
							} else if (drcInstructionsUniDrc->drcInstructionsType == 2) {
								groupIdRequested = drcInstructionsUniDrc->mae_groupID;
								groupPresetIdRequested = -1;
							} else {
								groupIdRequested = -1;
								groupPresetIdRequested = -1;
							}
#endif
                            err = initLoudnessControl (hUniDrcSelProcParams,
                                                       hLoudnessInfoSet,
                                                       downmixIdRequested[d],
                                                       drcInstructionsUniDrc->drcSetId,
#if MPEG_H_SYNTAX
													   groupIdRequested,
													   groupPresetIdRequested,
#endif
                                                       noCompressionEqCount,
                                                       noCompressionEqId,
                                                       &loudnessInfoCount,
                                                       eqSetIdForLoudness,
                                                       loudnessNormalizationGainDb,
                                                       loudness);
                            if (err) return (err);

                            err = getSignalPeakLevel(hUniDrcConfig,
                                                     hLoudnessInfoSet,
                                                     drcInstructionsUniDrc,
                                                     downmixIdRequested[d],
                                                     hUniDrcSelProcParams->albumMode,
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
                                                     hUniDrcSelProcParams,
#endif
#endif
                                                     noCompressionEqCount,
                                                     noCompressionEqId,
                                                     &peakInfoCount,
                                                     eqSetIdForPeak,
                                                     signalPeakLevel,
                                                     explicitPeakInformationPresent);
                            if (err) return (err);

                            for (l=0; l<loudnessInfoCount; l++)
                            {
                                int matchFound = FALSE;
                                int p;
                                /* loudnessNormalizationGainDb*/
                                selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted = loudnessNormalizationGainDb[l];

                                /* truncate if larger than loudnessNormalizationGainDbMax */
                                selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted = min(selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted, hUniDrcSelProcParams->loudnessNormalizationGainDbMax);

                                if (loudness[l] != UNDEFINED_LOUDNESS_VALUE)
                                {
                                    selectionCandidateInfo[k].outputLoudness = loudness[l] + selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted;
                                }
                                else
                                {
                                    selectionCandidateInfo[k].outputLoudness = UNDEFINED_LOUDNESS_VALUE;
                                }

                                /* outputPeakLevel and outputLoudness */
                                for (p=0; p<peakInfoCount; p++)
                                {
                                    if (eqSetIdForPeak[p] == eqSetIdForLoudness[l])
                                    {
#if MPEG_D_DRC_EXTENSION_V1
                                        if (eqSetIdIsPermitted[eqSetIdForPeak[p]] == 1)
#endif
                                        {
                                            matchFound = TRUE;
                                            break;
                                        }
                                    }
                                }
                                if (matchFound == TRUE)
                                {
                                    selectionCandidateInfo[k].outputPeakLevel = signalPeakLevel[p] + selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted;
                                }
                                else
                                {
                                    selectionCandidateInfo[k].outputPeakLevel = selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted;
                                }
#if MPEG_D_DRC_EXTENSION_V1
                                if ((drcInstructionsUniDrc->requiresEq == 1) && (eqSetIdIsPermitted[eqSetIdForLoudness[l]] == 0)) continue; /* ignore candidate */
#endif
                                selectionCandidateInfo[k].drcInstructionsIndex = i;
                                selectionCandidateInfo[k].downmixIdRequestIndex = d;
                                selectionCandidateInfo[k].eqSetId = eqSetIdForLoudness[l];
                                if (explicitPeakInformationPresent[p] == 1)
                                {
                                    selectionCandidateInfo[k].selectionFlags = SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT;
                                }
                                else
                                {
                                    selectionCandidateInfo[k].selectionFlags = 0;
                                }
#if MPEG_D_DRC_EXTENSION_V1
                                getMixingLevel(hUniDrcSelProcParams,
                                               hLoudnessInfoSet,
                                               downmixIdRequested[d],
                                               drcInstructionsUniDrc->drcSetId,
                                               eqSetIdForLoudness[l],
                                               &selectionCandidateInfo[k].mixingLevel);
#endif
                                /* preselection step #7 */
                                if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent &&
                                    ((hUniDrcSelProcParams->loudnessNormalizationOn &&
                                      drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper >= hUniDrcSelProcParams->targetLoudness &&
                                      drcInstructionsUniDrc->drcSetTargetLoudnessValueLower < hUniDrcSelProcParams->targetLoudness) ||
                                     !hUniDrcSelProcParams->loudnessNormalizationOn)) {
                                    selectionCandidateInfo[k].selectionFlags |= SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH;
                                    if (!explicitPeakInformationPresent[p])
                                    {
                                        /* explicit peak information is missing; estimating peak from other information */
                                        if (hUniDrcSelProcParams->loudnessNormalizationOn) {
                                            selectionCandidateInfo[k].outputPeakLevel = hUniDrcSelProcParams->targetLoudness - drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper;
                                        }
                                        else {
                                            selectionCandidateInfo[k].outputPeakLevel = 0.0f;
                                        }
                                    }
                                }
                                if ((selectionCandidateInfo[k].selectionFlags & (SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH | SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT) || !drcInstructionsUniDrc->drcSetTargetLoudnessPresent))
                                {
                                    k++;
                                } else {
                                /* ignore candidate */
                                }
                            }
                        }
                    }
                }
                else   /* dynamicRangeControlOn == 0 */
                {
                    if (drcInstructionsUniDrc->drcSetId < 0) /* only virtual DRCs can be used */
                    {

#if MPEG_H_SYNTAX
                        groupIdRequested = -1;
                        groupPresetIdRequested = -1;
#endif

                        err = initLoudnessControl (hUniDrcSelProcParams,
                                                   hLoudnessInfoSet,
                                                   downmixIdRequested[d],
                                                   drcInstructionsUniDrc->drcSetId,
#if MPEG_H_SYNTAX
                                                   groupIdRequested,
                                                   groupPresetIdRequested,
#endif
                                                   noCompressionEqCount,
                                                   noCompressionEqId,
                                                   &loudnessInfoCount,
                                                   eqSetIdForLoudness,
                                                   loudnessNormalizationGainDb,
                                                   loudness);
                        if (err) return (err);

                        /* getSignalPeakLevel */
                        err = getSignalPeakLevel(hUniDrcConfig,
                                                 hLoudnessInfoSet,
                                                 drcInstructionsUniDrc,
                                                 downmixIdRequested[d],
                                                 hUniDrcSelProcParams->albumMode,
#ifdef AMD2_COR1
#if MPEG_H_SYNTAX
                                                 hUniDrcSelProcParams,
#endif
#endif
                                                 noCompressionEqCount,
                                                 noCompressionEqId,
                                                 &peakInfoCount,
                                                 eqSetIdForPeak,
                                                 signalPeakLevel,
                                                 explicitPeakInformationPresent);
                        if (err) return (err);
                        for (l=0; l<loudnessInfoCount; l++)
                        {
                            int matchFound = FALSE;
                            int p;
                            for (p=0; p<peakInfoCount; p++)
                            {
                                if (eqSetIdForPeak[p] == eqSetIdForLoudness[l])
                                {
#if MPEG_D_DRC_EXTENSION_V1
                                    if (eqSetIdIsPermitted[eqSetIdForPeak[p]] == 1)
#endif
                                    {
                                        matchFound = TRUE;
                                        break;
                                    }
                                }
                            }
                            if (matchFound == TRUE)
                            {
                                /* loudnessNormalizationGainDb*/
                                adjustment = max(0.0f, signalPeakLevel[p] + loudnessNormalizationGainDb[l] - hUniDrcSelProcParams->outputPeakLevelMax);
                                adjustment = min(adjustment, max(0.0f, loudnessDeviationMax));
                                selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted = loudnessNormalizationGainDb[l] - adjustment;

                                /* truncate if larger than loudnessNormalizationGainDbMax */
                                selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted = min(selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted, hUniDrcSelProcParams->loudnessNormalizationGainDbMax);

                                /* outputPeakLevel and outputLoudness */
                                selectionCandidateInfo[k].outputPeakLevel = signalPeakLevel[p] + selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted;
                                if (loudness[l] != UNDEFINED_LOUDNESS_VALUE)
                                {
                                    selectionCandidateInfo[k].outputLoudness = loudness[l] + selectionCandidateInfo[k].loudnessNormalizationGainDbAdjusted;
                                }
                                else
                                {
                                    selectionCandidateInfo[k].outputLoudness = UNDEFINED_LOUDNESS_VALUE;
                                }
                                selectionCandidateInfo[k].drcInstructionsIndex = i;
                                selectionCandidateInfo[k].downmixIdRequestIndex = d;
                                selectionCandidateInfo[k].eqSetId = eqSetIdForLoudness[l];
                                if (explicitPeakInformationPresent[p]==1)
                                {
                                    selectionCandidateInfo[k].selectionFlags = SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT;
                                }
                                else
                                {
                                    selectionCandidateInfo[k].selectionFlags = 0;
                                }
#if MPEG_D_DRC_EXTENSION_V1
                                getMixingLevel(hUniDrcSelProcParams,
                                               hLoudnessInfoSet,
                                               downmixIdRequested[d],
                                               drcInstructionsUniDrc->drcSetId,
                                               eqSetIdForLoudness[l],
                                               &selectionCandidateInfo[k].mixingLevel);
#endif
                                k++;
                            }
                        }
                    }
                }
            }
        }
    }
    *selectionCandidateCount = k;

    /* error check */
    if (*selectionCandidateCount == 0)
    {
        fprintf(stderr, "WARNING: no DRC sets available for requested downmixId/s.\n");
    }
    else if (*selectionCandidateCount > SELECTION_CANDIDATE_COUNT_MAX)
    {
        fprintf(stderr, "ERROR: selectionCandidateCount exceeds maximum. Please increase SELECTION_CONDIDATE_COUNT_MAX  %d\n", *selectionCandidateCount);
        return UNEXPECTED_ERROR;
    }
    else if (hUniDrcSelProcParams->dynamicRangeControlOn == 1)
    {
#if MPEG_D_DRC_EXTENSION_V1
        n = 0;
        for (k=0; k<*selectionCandidateCount; k++)
        {
            /* preselection step #7 */
#if EQ_IS_SUPPORTED
            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex]);

            if (hUniDrcSelProcParams->eqSetPurposeRequest != EQ_PURPOSE_EQ_OFF)
            {
                int matchingEqSetCount = 0;
                int matchingEqInstrucionsIndex[64];
                err = matchEqSet(hUniDrcConfig,
                                 downmixIdRequested[selectionCandidateInfo[k].downmixIdRequestIndex],
                                 drcInstructionsUniDrc->drcSetId,
                                 eqSetIdIsPermitted,
                                 &matchingEqSetCount,
                                 matchingEqInstrucionsIndex);
                if (err) return(err);
                for (j=0; j<matchingEqSetCount; j++) {
                    memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    selectionCandidateInfoStep2[n].eqSetId = hUniDrcConfig->uniDrcConfigExt.eqInstructions[matchingEqInstrucionsIndex[j]].eqSetId;
                    n++;
                }
            }
#endif
            if (drcInstructionsUniDrc->requiresEq == 0) {
                memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                selectionCandidateInfoStep2[n].eqSetId = 0;
                n++;
            }
        }
        for (k=0; k<n; k++)
        {
            memcpy(&selectionCandidateInfo[k], &selectionCandidateInfoStep2[k], sizeof(SelectionCandidateInfo));
        }
        *selectionCandidateCount = n;
#endif /* MPEG_D_DRC_EXTENSION_V1 */
        n = 0;
        for (k=0; k<*selectionCandidateCount; k++)
        {
            if ((selectionCandidateInfo[k].selectionFlags & SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH) &&
                !(selectionCandidateInfo[k].selectionFlags & SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT)) {
                /* preselection step #7 (#8 for AMD1) */
                memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                n++;
            } else {
                /* preselection step #8 (#9 for AMD1) */
                if (selectionCandidateInfo[k].outputPeakLevel <= outputPeakLevelMax) {
                    memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    n++;
                }
                if (selectionCandidateInfo[k].outputPeakLevel < outputPeakLevelMin)
                {
#if MPEG_H_SYNTAX
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcInstructionsType == 3) {
                        for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                            if (drcInstructionsUniDrc->mae_groupPresetID == hUniDrcSelProcParams->groupPresetIdRequested[i])
                            {
                                outputPeakLevelMin = selectionCandidateInfo[k].outputPeakLevel;
                                break;
                            }
                        }
                    } else if (drcInstructionsUniDrc->drcInstructionsType == 2) {
                        for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                            if (drcInstructionsUniDrc->mae_groupID == hUniDrcSelProcParams->groupIdRequested[i])
                            {
                                outputPeakLevelMin = selectionCandidateInfo[k].outputPeakLevel;
                                break;
                            }
                        }

                    } else {
                        outputPeakLevelMin = selectionCandidateInfo[k].outputPeakLevel;
                    }
#else
                    outputPeakLevelMin = selectionCandidateInfo[k].outputPeakLevel;
#endif /* MPEG_H_SYNTAX */
                }
            }
        }
        selectionCandidateStep2Count = n;
        /* preselection step #8 (part 2) (#9 for AMD1) */
        if (selectionCandidateStep2Count == 0)
        {
            n = 0;
            for (k=0; k<*selectionCandidateCount; k++)
            {
                /* select all DRC sets with matching drcSetTargetLoudness, but explicit peak information */
                if ((selectionCandidateInfo[k].selectionFlags & SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH) &&
                    (selectionCandidateInfo[k].selectionFlags & SELECTION_FLAG_EXPLICIT_PEAK_INFO_PRESENT)) {
                    memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    n++;
                }
            }
            selectionCandidateStep2Count = n;
        }
        /* preselection step #8 (part 3) (#9 for AMD1) */
        if (selectionCandidateStep2Count == 0)
        {
            n = 0;
            for (k=0; k<*selectionCandidateCount; k++)
            {
                /* select all DRC sets that exceed outputPeakLevelMin + 1dB; apply loudnessDeviationMax parameter */
                if (selectionCandidateInfoStep2[k].outputPeakLevel < outputPeakLevelMin + 1.0f)
                {
                    /*if (loudnessNormalizationGainDbAdjusted[s][t] > 0.0f)
                    {*/
                        memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                        adjustment = max(0.0f, selectionCandidateInfoStep2[n].outputPeakLevel - outputPeakLevelMax);
                        adjustment = min(adjustment, max(0.0f, loudnessDeviationMax));
                        selectionCandidateInfoStep2[n].loudnessNormalizationGainDbAdjusted -= adjustment;
                        selectionCandidateInfoStep2[n].outputPeakLevel -= adjustment;
                        selectionCandidateInfoStep2[n].outputLoudness -= adjustment;
                    /*}*/
                    n++;
                }
            }
            selectionCandidateStep2Count = n;
       }
#if MPEG_H_SYNTAX
        /* pre-selection step #9 and #10 (MPEG-H selection based on groupPresetId and groupId)  (#10 and #11 for AMD1) */
#ifdef AMD2_COR1
        for (n=0; n<selectionCandidateStep2Count; n++)
        {
            memcpy(&selectionCandidateInfo[n], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
        }
        *selectionCandidateCount = selectionCandidateStep2Count;
        n = 0;
        omitPreSelectionBasedOnRequestedGroupId = 0;
        for (k=0; k<*selectionCandidateCount; k++)
#else
        n = 0;
        omitPreSelectionBasedOnRequestedGroupId = 0;
        for (k=0; k<selectionCandidateStep2Count; k++)
#endif
        {

            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex]);
            if (drcInstructionsUniDrc->drcInstructionsType == 0 || drcInstructionsUniDrc->drcInstructionsType == 2) {
                memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                n++;
            } else if (drcInstructionsUniDrc->drcInstructionsType == 3) {
                for (i=0; i<hUniDrcSelProcParams->numGroupPresetIdsRequested; i++) {
                    if (drcInstructionsUniDrc->mae_groupPresetID == hUniDrcSelProcParams->groupPresetIdRequested[i])
                    {
                        memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                        omitPreSelectionBasedOnRequestedGroupId = 1;
                        n++;
                    }
                }
            }
        }
        selectionCandidateStep2Count = n;
        if (omitPreSelectionBasedOnRequestedGroupId == 1)
        {
#ifdef AMD2_COR1
            for (n=0; n<selectionCandidateStep2Count; n++)
            {
                memcpy(&selectionCandidateInfo[n], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
            }
            *selectionCandidateCount = selectionCandidateStep2Count;
            n = 0;
            for (k=0; k<*selectionCandidateCount; k++)
#else
            n = 0;
            for (k=0; k<selectionCandidateStep2Count; k++)
#endif
            {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex]);
                if (drcInstructionsUniDrc->drcInstructionsType == 0 || drcInstructionsUniDrc->drcInstructionsType == 3) {
                    memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    n++;
                }
            }
            selectionCandidateStep2Count = n;
        } else {
#ifdef AMD2_COR1
            for (n=0; n<selectionCandidateStep2Count; n++)
            {
                memcpy(&selectionCandidateInfo[n], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
            }
            *selectionCandidateCount = selectionCandidateStep2Count;
            n = 0;
            for (k=0; k<*selectionCandidateCount; k++)
#else
            n = 0;
            for (k=0; k<selectionCandidateStep2Count; k++)
#endif
            {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex]);
                if (drcInstructionsUniDrc->drcInstructionsType == 0) {
                    memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    n++;
                } else if (drcInstructionsUniDrc->drcInstructionsType == 2) {
                    for (i=0; i<hUniDrcSelProcParams->numGroupIdsRequested; i++) {
                        if (drcInstructionsUniDrc->mae_groupID == hUniDrcSelProcParams->groupIdRequested[i])
                        {
                            memcpy(&selectionCandidateInfoStep2[n], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                            n++;
                        }
                    }
                }
            }
            selectionCandidateStep2Count = n;
        }
#endif /* MPEG_H_SYNTAX */
        for (n=0; n<selectionCandidateStep2Count; n++)
        {
            memcpy(&selectionCandidateInfo[n], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
        }
        *selectionCandidateCount = selectionCandidateStep2Count;
    }

    if (restrictToDrcWithAlbumLoudness == TRUE)
    {
#if ALBUM_MODE_DRC_FIX
        int v = 0; /* number of selected virtual DRC sets */
#endif
        j = 0;
        for (k=0; k<*selectionCandidateCount; k++)
        {
#if ALBUM_MODE_DRC_FIX
            loudnessDrcSetIdRequested = hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex].drcSetId;
            if (loudnessDrcSetIdRequested < 0)
            {
                /* keep virtual DRC set */
                memcpy(&selectionCandidateInfo[j], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                v++;
                j++;
                continue;
            }
#else
            loudnessDrcSetIdRequested = max(0, hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[k].drcInstructionsIndex].drcSetId);
#endif
            for (n=0; n<hLoudnessInfoSet->loudnessInfoAlbumCount; n++)
            {
                if (loudnessDrcSetIdRequested == hLoudnessInfoSet->loudnessInfoAlbum[n].drcSetId)
                {
                    memcpy(&selectionCandidateInfo[j], &selectionCandidateInfo[k], sizeof(SelectionCandidateInfo));
                    j++;
                    break;
                }
            }
        }
#if ALBUM_MODE_DRC_FIX
        if (v == j)
        {
            /* we have only virtual DRC sets. Dismiss the selection */
            *selectionCandidateCount = 0;
        }
        else
        {
            *selectionCandidateCount = j;
        }
#else
        *selectionCandidateCount = j;
#endif
    }
    /* End of Pre-selection.
     Results:
     Number of selected DRCs: selectedDrcInstructionsCount
     Indices for DRCinstructions (requested downmixId in second column): selectedDrcInstructions[][]
     Output peak level (for each requested downmixId): outputPeakLevel[][]
     Adjusted gain (for each requested downmixId): loudnessNormalizationGainDbAdjusted[][]
     Output loudness (for each requested downmixId): outputLoudness[][] */

    return (0);
}

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
                     )
{
    if (*selectionCandidateCount > 0)
    {
        int k, i, n, m, err;
        int selectionCandidateStep2Count;
        SelectionCandidateInfo selectionCandidateInfoStep2[SELECTION_CANDIDATE_COUNT_MAX];
        int drcSetIdMax;
        float outputLevelMax;
        float outputLevelMin;
        int effectCount, effectCountMin;
        int effectTypesRequestableSize;
        int drcSetTargetLoudnessValueUpperMin;
        DrcInstructionsUniDrc* drcInstructionsUniDrc;
        DrcInstructionsUniDrc* drcInstructionsDependent;

#if MPEG_H_SYNTAX
        int numMembersGroupPresetId = 0;
        int maxNumMembersGroupPresetIds = 0;
        int mae_groupPresetID_lowest = 1000;
#endif

#if MPEG_D_DRC_EXTENSION_V1 && EQ_IS_SUPPORTED
/* select DRC sets with EQ sets that match the request if avaiable */
        if (hUniDrcSelProcParams->eqSetPurposeRequest > 0) {
            int matchFound;
            int loopCount = 0;
            int eqPurposeRequested = hUniDrcSelProcParams->eqSetPurposeRequest;
            k = 0;
            while ((k==0) && (loopCount < 2))
            {
                for (i=0; i < *selectionCandidateCount; i++)
                {
                    matchEqSetPurpose(hUniDrcConfig,
                                      selectionCandidateInfo[i].eqSetId,
                                      eqPurposeRequested,
                                      eqSetIdIsPermitted,
                                      &matchFound);

                    if (matchFound > 0)
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                eqPurposeRequested = EQ_PURPOSE_DEFAULT;
                loopCount++;
            }
            if (k>0) {
                memcpy(&selectionCandidateInfo[0], &selectionCandidateInfoStep2[0], k * sizeof(SelectionCandidateInfo));
                *selectionCandidateCount = k;
            }
        }
#endif
        /* Find the minimum outputLevel */
        outputLevelMin = 10000.0f;
        k = 0;
        for (i=0; i < *selectionCandidateCount; i++)
        {
            if (outputLevelMin >= selectionCandidateInfo[i].outputPeakLevel)
            {
                if (outputLevelMin > selectionCandidateInfo[i].outputPeakLevel)
                {
                    outputLevelMin = selectionCandidateInfo[i].outputPeakLevel;
                    k = 0;
                }
                memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                k++;
            }
        }
        selectionCandidateStep2Count = k;

        /* if false, the DRC set with outputLevelMin is chosen */
        if (outputLevelMin <= 0.0f )
        {
            /* final selection based on peak value <= 0.0 */
            selectionCandidateStep2Count = *selectionCandidateCount;
            k = 0;
            for (i=0; i<selectionCandidateStep2Count; i++) {
                if (selectionCandidateInfo[i].outputPeakLevel <= 0.0f)
                {
                    memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfo[i], sizeof(SelectionCandidateInfo));
                    k++;
                }
            }
            selectionCandidateStep2Count = k;
#if MPEG_H_SYNTAX
            /* final MPEG-H selection based on groupPresetId and groupId */
            k = 0;
            for (i=0; i<selectionCandidateStep2Count; i++) {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                if (drcInstructionsUniDrc->drcInstructionsType == 3) {
                    k++;
                }
            }
            if (k!=0) {
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++) {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcInstructionsType == 3) {
                        if (hUniDrcSelProcParams->groupPresetIdRequestedPreference == drcInstructionsUniDrc->mae_groupPresetID) {
                            memcpy(&selectionCandidateInfoStep2[0], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                            k = 1;
                            break;
                        }
                    }
                }
                if (k == 0) {
                    for (i=0; i<selectionCandidateStep2Count; i++) {
                        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                        if (drcInstructionsUniDrc->drcInstructionsType == 3) {
                            for (n=0; n<hUniDrcSelProcParams->numGroupPresetIdsRequested; n++)
                            {
                                if (hUniDrcSelProcParams->groupPresetIdRequested[n] == drcInstructionsUniDrc->mae_groupPresetID) {
                                    numMembersGroupPresetId = hUniDrcSelProcParams->numMembersGroupPresetIdsRequested[n];
                                    break;
                                }
                            }
                            if (maxNumMembersGroupPresetIds <= numMembersGroupPresetId)
                            {
                                if (maxNumMembersGroupPresetIds < numMembersGroupPresetId)
                                {
                                    maxNumMembersGroupPresetIds = numMembersGroupPresetId;
                                    k = 0;
                                }
                                memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                                k++;
                            }
                        }
                    }
                }
                if (k > 1) {
#ifdef AMD2_COR1
                    m = k;
                    for (n=0; n<m; n++) {
                        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[n].drcInstructionsIndex]);
                        if (mae_groupPresetID_lowest >= drcInstructionsUniDrc->mae_groupPresetID)
                        {
                            if (mae_groupPresetID_lowest > drcInstructionsUniDrc->mae_groupPresetID)
                            {
                                mae_groupPresetID_lowest = drcInstructionsUniDrc->mae_groupPresetID;
                                k = 0;
                            }
                            memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
                            k++;
                        }
#else
                    for (n=0; n<k; n++) {
                        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[n].drcInstructionsIndex]);
                        if (mae_groupPresetID_lowest > drcInstructionsUniDrc->mae_groupPresetID)
                        {
                            mae_groupPresetID_lowest = drcInstructionsUniDrc->mae_groupPresetID;
                            memcpy(&selectionCandidateInfoStep2[0], &selectionCandidateInfoStep2[n], sizeof(SelectionCandidateInfo));
                            k = 1;
                        }
#endif
                    }
                }
                selectionCandidateStep2Count = k;
            } else {
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++) {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcInstructionsType == 2) {
                        k++;
                    }
                }
                if (k!=0) {
                    k = 0;
                    for (i=0; i<selectionCandidateStep2Count; i++) {
                        drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                        if (drcInstructionsUniDrc->drcInstructionsType == 2) {
                            memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                            k++;
                        }
                    }
                    selectionCandidateStep2Count = k;
                }
            }
#endif
            /* final selection based on downmixId */
            k = 0;
            for (i=0; i<selectionCandidateStep2Count; i++) {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                for (n=0; n<drcInstructionsUniDrc->downmixIdCount; n++) {
#ifdef AMD2_COR1
                    if (ID_FOR_BASE_LAYOUT != drcInstructionsUniDrc->downmixId[n] && ID_FOR_ANY_DOWNMIX != drcInstructionsUniDrc->downmixId[n]
                        && hUniDrcSelProcParams->downmixIdRequested[selectionCandidateInfoStep2[i].downmixIdRequestIndex] == drcInstructionsUniDrc->downmixId[n])
#else
                    if (hUniDrcSelProcParams->downmixIdRequested[selectionCandidateInfoStep2[i].downmixIdRequestIndex] == drcInstructionsUniDrc->downmixId[n])
#endif
                    {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
            }
            if (k > 0)
            {
                selectionCandidateStep2Count = k;
            }

            /* final selection based on number of requestable effect types except General */
            effectTypesRequestableSize = sizeof(effectTypesRequestable) / sizeof(int);
            effectCountMin = 100;
            k=0;
            for (i=0; i < selectionCandidateStep2Count; i++)
            {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                effectCount = 0;
                if (drcInstructionsUniDrc->dependsOnDrcSetPresent == TRUE)
                {
                    err = getDrcInstructionsDependent(hUniDrcConfig, drcInstructionsUniDrc, &drcInstructionsDependent);
                    if (err) return (err);

                    for (n=0; n<effectTypesRequestableSize; n++)
                    {
                        if (effectTypesRequestable[n] != EFFECT_BIT_GENERAL_COMPR)
                        {
                            if (((drcInstructionsUniDrc->drcSetEffect & effectTypesRequestable[n]) != 0x0) ||
                                ((drcInstructionsDependent->drcSetEffect & effectTypesRequestable[n]) != 0x0))
                            {
                                effectCount++;
                            }
                        }
                    }
                }
                else
                {
                    for (n=0; n<effectTypesRequestableSize; n++)
                    {
                        if (effectTypesRequestable[n] != EFFECT_BIT_GENERAL_COMPR)
                        {
                            if ((drcInstructionsUniDrc->drcSetEffect & effectTypesRequestable[n]) != 0x0)
                            {
                                effectCount++;
                            }
                        }
                    }
                }
                if (effectCountMin >= effectCount)
                {
                    if (effectCountMin > effectCount)
                    {
                        effectCountMin = effectCount;
                        k = 0;
                    }
                    memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                    k++;
                }
            }
            selectionCandidateStep2Count = k;

            /* final selection based on drcSetTargetLoudness selection */
            drcSetTargetLoudnessValueUpperMin = 100;
            k = 0;
            for (i=0; i<selectionCandidateStep2Count; i++) {
                if (selectionCandidateInfoStep2[i].selectionFlags & SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH) {
                    k++;
                }
            }
            if (k != 0 && k != selectionCandidateStep2Count)
            {
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++) {
                    if (!(selectionCandidateInfoStep2[i].selectionFlags & SELECTION_FLAG_DRC_TARGET_LOUDNESS_MATCH)) {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                selectionCandidateStep2Count = k;
            } else if (k == selectionCandidateStep2Count) {
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++)
                {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent != 1)
                    {
                        fprintf(stderr, "ERROR: drcSetTargetLoudnessPresent = 0. Selected DRC sets ordering shuffled.\n");
                        return UNEXPECTED_ERROR;
                    }
                    if (drcSetTargetLoudnessValueUpperMin >= drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                    {
                        if (drcSetTargetLoudnessValueUpperMin > drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                        {
                            drcSetTargetLoudnessValueUpperMin = drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper;
                            k = 0;
                        }
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                selectionCandidateStep2Count = k;
            }

#if AMD1_SYNTAX
#ifdef AMD1_PARAMETRIC_LIMITER
            /* final selection based on drcSetTargetLoudness presence */
            k = 0;
            for (i=0; i<selectionCandidateStep2Count; i++) {
                drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent && hUniDrcSelProcParams->loudnessNormalizationOn && drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper >= hUniDrcSelProcParams->targetLoudness && drcInstructionsUniDrc->drcSetTargetLoudnessValueLower < hUniDrcSelProcParams->targetLoudness) {
                    k++;
                }
            }
            if (k != 0 && k != selectionCandidateStep2Count)
            {
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++) {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent && hUniDrcSelProcParams->loudnessNormalizationOn && drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper >= hUniDrcSelProcParams->targetLoudness && drcInstructionsUniDrc->drcSetTargetLoudnessValueLower < hUniDrcSelProcParams->targetLoudness) {
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                selectionCandidateStep2Count = k;
                drcSetTargetLoudnessValueUpperMin = 100;
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++)
                {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent != 1)
                    {
                        fprintf(stderr, "ERROR: drcSetTargetLoudnessPresent = 0. Selected DRC sets ordering shuffled.\n");
                        return UNEXPECTED_ERROR;
                    }
                    if (drcSetTargetLoudnessValueUpperMin >= drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                    {
                        if (drcSetTargetLoudnessValueUpperMin > drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                        {
                            drcSetTargetLoudnessValueUpperMin = drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper;
                            k = 0;
                        }
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                selectionCandidateStep2Count = k;
            } else if (k == selectionCandidateStep2Count) {
                drcSetTargetLoudnessValueUpperMin = 100;
                k = 0;
                for (i=0; i<selectionCandidateStep2Count; i++)
                {
                    drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
                    if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent != 1)
                    {
                        fprintf(stderr, "ERROR: drcSetTargetLoudnessPresent = 0. Selected DRC sets ordering shuffled.\n");
                        return UNEXPECTED_ERROR;
                    }
                    if (drcSetTargetLoudnessValueUpperMin >= drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                    {
                        if (drcSetTargetLoudnessValueUpperMin > drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper)
                        {
                            drcSetTargetLoudnessValueUpperMin = drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper;
                            k = 0;
                        }
                        memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                        k++;
                    }
                }
                selectionCandidateStep2Count = k;
            }
#endif
#endif
            /* select the DRC sets with max. outputLevel <= 0.0 */
            k = 0;
            outputLevelMax = -1000.0f;
            for (i=0; i < selectionCandidateStep2Count; i++)
            {
                if ((selectionCandidateInfoStep2[i].outputPeakLevel <= 0.0f) && (outputLevelMax <= selectionCandidateInfoStep2[i].outputPeakLevel))
                {
                    if (outputLevelMax < selectionCandidateInfoStep2[i].outputPeakLevel)
                    {
                        outputLevelMax = selectionCandidateInfoStep2[i].outputPeakLevel;
                        k = 0;
                    }
                    memcpy(&selectionCandidateInfoStep2[k], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
                    k++;
                    outputLevelMax = selectionCandidateInfoStep2[i].outputPeakLevel;
                }
            }
            selectionCandidateStep2Count = k;
        }

        drcSetIdMax = -1000;
        for (i=0; i < selectionCandidateStep2Count; i++)
        {
            drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfoStep2[i].drcInstructionsIndex]);
            if (drcSetIdMax < drcInstructionsUniDrc->drcSetId)
            {
                drcSetIdMax = drcInstructionsUniDrc->drcSetId;
                memcpy(&selectionCandidateInfoStep2[0], &selectionCandidateInfoStep2[i], sizeof(SelectionCandidateInfo));
            }
        }
        memcpy(&selectionCandidateInfo[0], &selectionCandidateInfoStep2[0], sizeof(SelectionCandidateInfo));
        *selectionCandidateCount = 1;
    }
    else
    {
        fprintf(stderr, "WARNING: no DRC set found that matches the request\n");
        *selectionCandidateCount = 0;
        return(UNEXPECTED_ERROR);
    }
    return(0);
}

int
selectDrcSet(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
             int* drcSetIdSelected
#if MPEG_D_DRC_EXTENSION_V1
             , int* eqSetIdSelected
             , int* loudEqIdSelected
#endif
             )
{
    int i, err;

    HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams = &hUniDrcSelProcStruct->uniDrcSelProcParams;
    HANDLE_UNI_DRC_CONFIG hUniDrcConfig                 = &hUniDrcSelProcStruct->uniDrcConfig;
    HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet           = &hUniDrcSelProcStruct->loudnessInfoSet;

    int selectionCandidateCount = 0;
    SelectionCandidateInfo selectionCandidateInfo[SELECTION_CANDIDATE_COUNT_MAX];

#if MPEG_D_DRC_EXTENSION_V1
    int selectedEqSetCount = 0;
    int selectedEqSets[EQ_INSTRUCTIONS_COUNT_MAX][2];
#endif

    int restrictToDrcWithAlbumLoudness = FALSE;
    if (hUniDrcSelProcParams->albumMode == 1)
    {
        restrictToDrcWithAlbumLoudness = TRUE;
    }

    while (selectionCandidateCount == 0)
    {
        /* DRC set pre-selection according to Section 6.3.2 of ISO/IEC 23003-4 */
        drcSetPreselection(hUniDrcSelProcParams,
                           hUniDrcConfig,
                           hLoudnessInfoSet,
                           restrictToDrcWithAlbumLoudness,
#if MPEG_D_DRC_EXTENSION_V1
                           hUniDrcSelProcStruct->drcSetIdIsPermitted,
                           hUniDrcSelProcStruct->eqSetIdIsPermitted,
#endif
                           &selectionCandidateCount,
                           selectionCandidateInfo);

        if (selectionCandidateCount == 0)
        {
            if (restrictToDrcWithAlbumLoudness == TRUE)
            {
                restrictToDrcWithAlbumLoudness = FALSE;
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: no applicable DRC set found in pre-selection\n");
                return(UNEXPECTED_ERROR);
            }
        }

        /* if requested EFFECT_TYPE_REQUESTED_NONE -> numDrcEffectTypeRequestsDesired == 1 !!! */
        err = validateDrcFeatureRequest(hUniDrcSelProcParams);
        if (err) return (err);

        /* DRC set selection based on requests according to Section 6.3.3 of ISO/IEC 23003-4 */
        if (hUniDrcSelProcParams->dynamicRangeControlOn == 1)
        {
            if (hUniDrcSelProcParams->numDrcFeatureRequests > 0)
            {
                for (i=0; i<hUniDrcSelProcParams->numDrcFeatureRequests; i++)
                {
                    switch (hUniDrcSelProcParams->drcFeatureRequestType[i]) {
                        case MATCH_EFFECT_TYPE:
                            err = matchEffectTypes(hUniDrcConfig,
                                                   hUniDrcSelProcParams->numDrcEffectTypeRequests[i],
                                                   hUniDrcSelProcParams->numDrcEffectTypeRequestsDesired[i],
                                                   hUniDrcSelProcParams->drcEffectTypeRequest[i],
                                                   &selectionCandidateCount,
                                                   selectionCandidateInfo);
                            if (err) return (err);
                            break;
                        case MATCH_DYNAMIC_RANGE:
                            err = matchDynamicRange(hUniDrcConfig,
                                                    hLoudnessInfoSet,
                                                    hUniDrcSelProcParams->dynamicRangeMeasurementRequestType[i],
                                                    hUniDrcSelProcParams->dynamicRangeRequestedIsRange[i],
                                                    hUniDrcSelProcParams->dynamicRangeRequestValue[i],
                                                    hUniDrcSelProcParams->dynamicRangeRequestValueMin[i],
                                                    hUniDrcSelProcParams->dynamicRangeRequestValueMax[i],
                                                    hUniDrcSelProcParams->downmixIdRequested,
                                                    hUniDrcSelProcParams->albumMode,
                                                    &selectionCandidateCount,
                                                    selectionCandidateInfo);
                            if (err) return (err);
                            break;
                        case MATCH_DRC_CHARACTERISTIC:
                            err = matchDrcCharacteristic(hUniDrcConfig,
                                                         hUniDrcSelProcParams->drcCharacteristicRequest[i],
                                                         &selectionCandidateCount,
                                                         selectionCandidateInfo);
                            if (err) return (err);
                            break;

                        default:
                            fprintf(stderr, "ERROR: invalid matching process selected: %d\n", hUniDrcSelProcParams->drcFeatureRequestType[i]);
                            return (UNEXPECTED_ERROR);
                            break;
                    }
                }
            }
            else
            {
                int matchFound = FALSE;

                /* first, request effect type None */
                err = selectDrcsWithoutCompressionEffects (hUniDrcConfig, &matchFound, &selectionCandidateCount, selectionCandidateInfo);
                if (err) return (err);

                if (matchFound == FALSE)
                {
                    /* second, request effect type General with fallback Night, Noisy, Limited and LowLevel */
                    int numDrcEffectTypeRequests = 5;
                    int numDrcEffectTypeRequestsDesired = 1;
                    int drcEffectTypeRequest[5] = {EFFECT_TYPE_REQUESTED_GENERAL_COMPR, EFFECT_TYPE_REQUESTED_NIGHT, EFFECT_TYPE_REQUESTED_NOISY, EFFECT_TYPE_REQUESTED_LIMITED, EFFECT_TYPE_REQUESTED_LOWLEVEL};

                    err = matchEffectTypes(hUniDrcConfig,
                                           numDrcEffectTypeRequests,
                                           numDrcEffectTypeRequestsDesired,
                                           drcEffectTypeRequest,
                                           &selectionCandidateCount,
                                           selectionCandidateInfo);
                    if (err) return (err);
                }
            }

            /* final DRC set selection according to Section 6.3.4 of ISO/IEC 23003-4 */
            err = drcSetFinalSelection (hUniDrcConfig,
                                        hUniDrcSelProcParams,
                                        &selectionCandidateCount,
                                        selectionCandidateInfo
#if MPEG_D_DRC_EXTENSION_V1
                                        , hUniDrcSelProcStruct->eqSetIdIsPermitted
                                        , &selectedEqSetCount
                                        , selectedEqSets
#endif
                                        );
            if (err) return (err);
        } /* if dynamicRangeControlOn == 1 */

        /* TODO: Select EQ if dynamicRangeControlOn==0 */
        if (selectionCandidateCount == 0)
        {
            if (restrictToDrcWithAlbumLoudness == TRUE)
            {
                restrictToDrcWithAlbumLoudness = FALSE;
            }
            else
            {
                fprintf(stderr, "ERROR: no applicable DRC set found in pre-selection\n");
                return(UNEXPECTED_ERROR);
            }
        }
    }
    *drcSetIdSelected = hUniDrcConfig->drcInstructionsUniDrc[selectionCandidateInfo[0].drcInstructionsIndex].drcSetId;
#if MPEG_D_DRC_EXTENSION_V1
    *eqSetIdSelected = selectionCandidateInfo[0].eqSetId;
    selectLoudEq(hUniDrcConfig,
                 hUniDrcSelProcParams->loudnessEqRequest,
                 hUniDrcSelProcParams->downmixIdRequested[selectionCandidateInfo[0].downmixIdRequestIndex],
                 *drcSetIdSelected,
                 *eqSetIdSelected,
                 loudEqIdSelected);
#endif
    /* return signal characteristics */
    if (selectionCandidateCount > 0)
    {
        hUniDrcSelProcStruct->uniDrcSelProcOutput.loudnessNormalizationGainDb = selectionCandidateInfo[0].loudnessNormalizationGainDbAdjusted;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.outputPeakLevelDb = selectionCandidateInfo[0].outputPeakLevel;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.outputLoudness = selectionCandidateInfo[0].outputLoudness;
        hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId = hUniDrcSelProcParams->downmixIdRequested[selectionCandidateInfo[0].downmixIdRequestIndex];
#if MPEG_D_DRC_EXTENSION_V1
        hUniDrcSelProcStruct->uniDrcSelProcOutput.mixingLevel = selectionCandidateInfo[0].mixingLevel;
#endif
    }
    /*else
    {
    hUniDrcSelProcStruct->uniDrcSelProcOutput.loudnessNormalizationGainDb = 0.f;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.outputPeakLevelDb = 0.f;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.outputLoudness = UNDEFINED_LOUDNESS_VALUE;
    hUniDrcSelProcStruct->uniDrcSelProcOutput.activeDownmixId = hUniDrcSelProcParams->downmixIdRequested[0];
    }*/
    return(0);
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

