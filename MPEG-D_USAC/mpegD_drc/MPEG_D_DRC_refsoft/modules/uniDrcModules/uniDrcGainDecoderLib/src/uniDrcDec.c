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
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#include <stdio.h>
#include "uniDrcCommon.h"
#include "uniDrc.h"
#include "uniDrcTables.h"
#include "uniDrcGainDec.h"
#include "uniDrcFilterBank.h"
#include "uniDrcMultiBand.h"
#if MPEG_D_DRC_EXTENSION_V1
#include "uniDrcShapeFilter.h"
#endif /* MPEG_D_DRC_EXTENSION_V1 */
#include "uniDrcProcessAudio.h"
#include "uniDrcDec.h"

int
initDrcParams(const int audioFrameSize,
              const int audioSampleRate,
              const int gainDelaySamples,
              const int delayMode,
              const int subBandDomainMode,
              DrcParams* drcParams)
{
    int k;
    if (audioFrameSize < 1)
    {
        fprintf(stderr, "ERROR: the audio frame size must be positive.\n");
        return(PARAM_ERROR);
    }
    
    if (audioSampleRate < 1000)
    {
        fprintf(stderr, "ERROR: audio sample rates below 1000 Hz are not supported.\n");
        return(PARAM_ERROR);
    }

    if (audioFrameSize > AUDIO_CODEC_FRAME_SIZE_MAX)
    {
        fprintf(stderr, "ERROR: the audio frame size is too large for this implementation: %d.\n", audioFrameSize);
        fprintf(stderr, "Increase the value of AUDIO_CODEC_FRAME_SIZE_MAX.\n");
        return(PARAM_ERROR);
    }
    drcParams->drcFrameSize = audioFrameSize;
    
    if (drcParams->drcFrameSize < 0.001f * audioSampleRate)
    {
        fprintf(stderr, "ERROR: DRC frame size must be at least 1ms.\n");
        return(PARAM_ERROR);
    }

    drcParams->audioSampleRate  = audioSampleRate;
    drcParams->deltaTminDefault = getDeltaTmin(audioSampleRate);
    if (drcParams->deltaTminDefault > drcParams->drcFrameSize)
    {
        fprintf(stderr, "ERROR: DRC time interval (deltaTmin) cannot exceed audio frame size. %d %d\n", drcParams->deltaTminDefault, drcParams->drcFrameSize);
        return(PARAM_ERROR);
    }

    if ((delayMode != DELAY_MODE_REGULAR_DELAY) && (delayMode != DELAY_MODE_LOW_DELAY))
    {
      fprintf(stderr, "ERROR: invalid delay mode.\n");
      return(PARAM_ERROR);
    }
    drcParams->delayMode = delayMode;

    drcParams->drcSetCounter = 0;
    drcParams->multiBandSelDrcIndex = -1;
    for (k = 0; k < SEL_DRC_COUNT; k++)
    {
        drcParams->selDrc[k].drcInstructionsIndex = -1;
        drcParams->selDrc[k].downmixInstructionsIndex = -1;
        drcParams->selDrc[k].drcCoefficientsIndex = -1;
    }
    if ((gainDelaySamples > MAX_SIGNAL_DELAY) || (gainDelaySamples < 0))
    {
        fprintf(stderr, "ERROR: sample delay must not exceed the maximum value of %i samples or the minimum of 0 samples. Current value: %i\n", MAX_SIGNAL_DELAY, gainDelaySamples);
        return(PARAM_ERROR);
    }
    else
    {
        drcParams->gainDelaySamples = gainDelaySamples;
    }
    
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_OFF:
        case SUBBAND_DOMAIN_MODE_QMF64:
        case SUBBAND_DOMAIN_MODE_QMF71:
        case SUBBAND_DOMAIN_MODE_STFT256:
            drcParams->subBandDomainMode = subBandDomainMode;
            break;
        default:
            fprintf(stderr, "ERROR: subBandDomainMode = %d currently not supported.\n", subBandDomainMode);
            return(PARAM_ERROR);
            break;
    }
    
#if AMD1_SYNTAX
    drcParams->parametricDrcDelay = 0;
    drcParams->eqDelay = 0;
#endif
    
    return (0);
}

/* this function exists in 3 identical copies in different libraries. */
int
selectDrcCoefficients2(UniDrcConfig* uniDrcConfig,
                       const int location,
                       DrcCoefficientsUniDrc** drcCoefficientsUniDrc,
                       int* drcCoefficientsSelected)
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
                *drcCoefficientsSelected = cV0;
            }
            else
            {
                cV1 = n;
                *drcCoefficientsSelected = cV1;
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
        int i;
        fprintf(stderr, "ERROR: no DrcCoefficient block available for drcLocation %d.\n", location);
        fprintf(stderr, "Available DRC set drcLocation: ");
        for (i=0; i<uniDrcConfig->drcCoefficientsUniDrcCount; i++)
        {
            fprintf(stderr, "location %d version %d", uniDrcConfig->drcCoefficientsUniDrc[i].drcLocation, uniDrcConfig->drcCoefficientsUniDrc[i].version);
        }
        fprintf(stderr, "\n");
        return(EXTERNAL_ERROR);
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 || AMD1_SYNTAX */
    return (0);
}

int
initSelectedDrcSet(UniDrcConfig* uniDrcConfig,
                   DrcParams* drcParams,
#if AMD1_SYNTAX
                   PARAMETRIC_DRC_PARAMS* hParametricDrcParams,
#endif
                   const int audioChannelCount,
                   const int drcSetIdSelected,
                   const int downmixIdSelected,
                   FilterBanks* filterBanks,
                   OverlapParams* overlapParams
#if MPEG_D_DRC_EXTENSION_V1
                   , ShapeFilterBlock* shapeFilterBlock
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                    )
{
    int g, n, i, c, err = 0;
    int channelCount = 0;
    
    DrcInstructionsUniDrc* drcInstructionsUniDrc = NULL;
    DrcCoefficientsUniDrc* drcCoefficientsUniDrc = NULL;
    int selectedDrcIsMultiband = FALSE;
    int drcInstructionsSelected = -1;
    int downmixInstructionsSelected = -1;
    int drcCoefficientsSelected = -1;
    int location = LOCATION_SELECTED;
#if AMD1_SYNTAX
    hParametricDrcParams->parametricDrcInstanceCount = 0;
#endif /* AMD1_SYNTAX */
    
    if (uniDrcConfig->drcCoefficientsUniDrcCount && uniDrcConfig->drcCoefficientsUniDrc->drcFrameSizePresent)
    {
        if (uniDrcConfig->drcCoefficientsUniDrc->drcFrameSize != drcParams->drcFrameSize)
        {
            fprintf(stderr, "ERROR: audio frame size from command line does not match the DRC frame size in the bitstream. %d %d\n", drcParams->drcFrameSize, uniDrcConfig->drcCoefficientsUniDrc->drcFrameSize);
            return(PARAM_ERROR);
        }
    }

    /* choose correct DRC instruction */
    for(n=0; n<uniDrcConfig->drcInstructionsCountPlus; n++)
    {
        if (uniDrcConfig->drcInstructionsUniDrc[n].drcSetId == drcSetIdSelected) break;
    }
    if (n == uniDrcConfig->drcInstructionsCountPlus)
    {
        fprintf(stderr, "ERROR: selected DRC set ID is not available %d.\n", drcSetIdSelected);
        fprintf(stderr, "Available DRC set IDs: ");
        for (i=0; i<uniDrcConfig->drcInstructionsCountPlus; i++)
        {
            fprintf(stderr, "%d ", uniDrcConfig->drcInstructionsUniDrc[i].drcSetId);
        }
        fprintf(stderr, "\n");
        return(EXTERNAL_ERROR);
    }
    drcInstructionsSelected = n;
    drcInstructionsUniDrc = &(uniDrcConfig->drcInstructionsUniDrc[drcInstructionsSelected]);

    if (downmixIdSelected == ID_FOR_BASE_LAYOUT) {
        channelCount = uniDrcConfig->channelLayout.baseChannelCount;
        if (channelCount != audioChannelCount) {
            fprintf(stderr, "ERROR: audioChannelCount != baseChannelCount (%d != %d)\n", audioChannelCount, channelCount);
        }
    } else if (downmixIdSelected == ID_FOR_ANY_DOWNMIX) {
        channelCount = audioChannelCount; /* no sanity check possible, audioChannelCount must be correct */
    } else {
#if MPEG_H_SYNTAX
        for(n=0; n<uniDrcConfig->mpegh3daDownmixInstructionsCount; n++)
        {
            if (uniDrcConfig->mpegh3daDownmixInstructions[n].downmixId == downmixIdSelected) break;
        }
        if (n == uniDrcConfig->mpegh3daDownmixInstructionsCount)
        {
            fprintf(stderr, "ERROR: no matching downmixInstructions found for downmixId = %d\n", downmixIdSelected);
            return (UNEXPECTED_ERROR);
        }
        channelCount = uniDrcConfig->mpegh3daDownmixInstructions[n].targetChannelCount;
        if (channelCount != audioChannelCount) {
            fprintf(stderr, "ERROR: audioChannelCount != targetChannelCount (%d != %d)\n", audioChannelCount, channelCount);
        }
        downmixInstructionsSelected = n;
#else
        for(n=0; n<uniDrcConfig->downmixInstructionsCount; n++)
        {
            if (uniDrcConfig->downmixInstructions[n].downmixId == downmixIdSelected) break;
        }
        if (n == uniDrcConfig->downmixInstructionsCount)
        {
            fprintf(stderr, "ERROR: no matching downmixInstructions found for downmixId = %d\n", downmixIdSelected);
            return (UNEXPECTED_ERROR);
        }
        channelCount = uniDrcConfig->downmixInstructions[n].targetChannelCount;
        if (channelCount != audioChannelCount) {
            fprintf(stderr, "ERROR: audioChannelCount != targetChannelCount (%d != %d)\n", audioChannelCount, channelCount);
        }
        downmixInstructionsSelected = n;
#endif
    } 
    drcInstructionsUniDrc->audioChannelCount = channelCount;
    
    if (drcInstructionsUniDrc->drcSetId <= 0)
    {
        drcCoefficientsSelected = 0; /* no gain sequence */
    }
    else
    {
        err = selectDrcCoefficients2(uniDrcConfig, location, &drcCoefficientsUniDrc, &drcCoefficientsSelected);
    }
    
    /* keep indices in drcParams->selDrc[xyz] */
    drcParams->selDrc[drcParams->drcSetCounter].drcInstructionsIndex = drcInstructionsSelected;
    drcParams->selDrc[drcParams->drcSetCounter].downmixInstructionsIndex = downmixInstructionsSelected; /* TODO: still actively used anywhere? */
    drcParams->selDrc[drcParams->drcSetCounter].drcCoefficientsIndex = drcCoefficientsSelected;
    
    /* here we can assign a channel group to each physical channel in a DRC set with downmixId=0x7F */
    if ((drcInstructionsUniDrc->downmixId[0] == ID_FOR_ANY_DOWNMIX) || (drcInstructionsUniDrc->downmixIdCount > 1)) {
        int idx = drcInstructionsUniDrc->gainSetIndex[0];
        for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++) {
            drcInstructionsUniDrc->channelGroupForChannel[c] = (idx >= 0) ? 0 : -1; /* only one channel group 0 */
        }
    }
    
    /* count nChannelsPerChannelGroup */
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++) {
        drcInstructionsUniDrc->nChannelsPerChannelGroup[g] = 0;
        for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++) {
            if (drcInstructionsUniDrc->channelGroupForChannel[c] == g) {
                drcInstructionsUniDrc->nChannelsPerChannelGroup[g]++;
            }
        }
    }
    
    /* multibandAudioSignalCount */
    if (drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) {
        drcInstructionsUniDrc->multibandAudioSignalCount = drcInstructionsUniDrc->audioChannelCount;
    } else {
        drcInstructionsUniDrc->multibandAudioSignalCount = 0;
        for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++) {
            g = drcInstructionsUniDrc->channelGroupForChannel[c];
            if (g < 0) {
                drcInstructionsUniDrc->multibandAudioSignalCount++;  /* (no DRC for this channel) */
            } else {
                drcInstructionsUniDrc->multibandAudioSignalCount += drcInstructionsUniDrc->bandCountForChannelGroup[g];
            }
        }
    }
    
    /* this check is necessary for later function initAllFilterBanks */
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
    {
#if AMD1_SYNTAX
        if (g==0) {
            drcInstructionsUniDrc->parametricDrcLookAheadSamplesMax = 0;
        }
        if (drcInstructionsUniDrc->channelGroupIsParametricDrc[g] == 0) {
#endif /* AMD1_SYNTAX */
            if (drcInstructionsUniDrc->bandCountForChannelGroup[g] > 1)
            {
                if (drcParams->multiBandSelDrcIndex != -1)
                {
                    fprintf(stderr, "ERROR: Not more than one DRC can be a multi-band DRC if two DRCs are applied to the time-domain signal.\n");
                    return(UNEXPECTED_ERROR);
                }
                selectedDrcIsMultiband = TRUE;
            }
#if AMD1_SYNTAX
        } else {
            int gainSetIndex    = drcInstructionsUniDrc->gainSetIndexForChannelGroupParametricDrc[g];
            int parametricDrcId = uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].parametricDrcId;
            int parametricDrcLookAheadSamples = 0;
            ParametricDrcInstructions* parametricDrcInstructions;
            
            for(i=0; i<uniDrcConfig->uniDrcConfigExt.parametricDrcInstructionsCount; i++)
            {
                if (parametricDrcId == uniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[i].parametricDrcId) break;
            }
            if (i == uniDrcConfig->uniDrcConfigExt.parametricDrcInstructionsCount)
            {
                fprintf(stderr, "ERROR: parametricDrcInstructions expected but not found.\n");
                return(UNEXPECTED_ERROR);
            }
            parametricDrcInstructions = &uniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[i];

            hParametricDrcParams->parametricDrcIndex[hParametricDrcParams->parametricDrcInstanceCount]           = i;
            hParametricDrcParams->gainSetIndex[hParametricDrcParams->parametricDrcInstanceCount]                 = gainSetIndex;
            if (drcInstructionsUniDrc->drcApplyToDownmix == 0) {
                hParametricDrcParams->downmixIdFromDrcInstructions[hParametricDrcParams->parametricDrcInstanceCount] = ID_FOR_BASE_LAYOUT;
            } else {
                if (drcInstructionsUniDrc->downmixIdCount > 1) {
                    hParametricDrcParams->downmixIdFromDrcInstructions[hParametricDrcParams->parametricDrcInstanceCount] = ID_FOR_ANY_DOWNMIX;
                } else {
                    hParametricDrcParams->downmixIdFromDrcInstructions[hParametricDrcParams->parametricDrcInstanceCount] = drcInstructionsUniDrc->downmixId[0];
                }
            }
            hParametricDrcParams->audioChannelCount                                                              = channelCount;
            for (i=0; i<hParametricDrcParams->audioChannelCount;i++) {
                if (drcInstructionsUniDrc->channelGroupForChannel[i] == g) {
                    hParametricDrcParams->channelMap[hParametricDrcParams->parametricDrcInstanceCount][i] = 1;
                } else {
                    hParametricDrcParams->channelMap[hParametricDrcParams->parametricDrcInstanceCount][i] = 0;
                }
            }
            drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] = 0;
            if (parametricDrcInstructions->parametricDrcLookAheadPresent) {
                parametricDrcLookAheadSamples = (int)((float)parametricDrcInstructions->parametricDrcLookAhead*(float)hParametricDrcParams->sampleRate*0.001f);
            } else {
                if (parametricDrcInstructions->parametricDrcType == PARAM_DRC_TYPE_FF) {
                    parametricDrcLookAheadSamples = (int)(10.0f*(float)hParametricDrcParams->sampleRate*0.001f);
#ifdef AMD1_PARAMETRIC_LIMITER
                } else if (parametricDrcInstructions->parametricDrcType == PARAM_DRC_TYPE_LIM) {
                    parametricDrcLookAheadSamples = (int)(5.0f*(float)hParametricDrcParams->sampleRate*0.001f);
#endif
                } else {
                    fprintf(stderr, "ERROR: Unknown parametricDrcType = %d.\n",parametricDrcInstructions->parametricDrcType);
                    return (UNEXPECTED_ERROR);
                }
            }
            drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] = parametricDrcLookAheadSamples;
            if (drcInstructionsUniDrc->parametricDrcLookAheadSamplesMax < drcInstructionsUniDrc->parametricDrcLookAheadSamples[g]) {
                drcInstructionsUniDrc->parametricDrcLookAheadSamplesMax = drcInstructionsUniDrc->parametricDrcLookAheadSamples[g];
            }
            hParametricDrcParams->parametricDrcInstanceCount += 1;
            selectedDrcIsMultiband = FALSE;
        }
#endif /* AMD1_SYNTAX */
    }
#if AMD1_SYNTAX
    if (drcParams->parametricDrcDelay != 0) {
        fprintf(stderr, "WARNING: Multiple DRC sets with parametric DRC active. Note that the current implementation does not support dependsOnDrcSet for those.\n");
	/* TODO: check whether feasable in future software update */
    }
    drcParams->parametricDrcDelay += drcInstructionsUniDrc->parametricDrcLookAheadSamplesMax;
#endif
    
    /* if MultiBand */
    if (selectedDrcIsMultiband == TRUE)
    {
        drcParams->multiBandSelDrcIndex = drcParams->drcSetCounter;
        /* decodingMode 0 */
        err = initAllFilterBanks(drcCoefficientsUniDrc,
                                 &(uniDrcConfig->drcInstructionsUniDrc[drcInstructionsSelected]),
                                 filterBanks);
        if (err) return (err);

        /* decodingMode 1 */
        err = initOverlapWeight (drcCoefficientsUniDrc,
                                 &(uniDrcConfig->drcInstructionsUniDrc[drcInstructionsSelected]),
                                 drcParams->subBandDomainMode,
                                 overlapParams);
        if (err) return (err);
    }
#if MPEG_D_DRC_EXTENSION_V1
    else
    {
#ifdef AMD2_COR1
        GainModifiers* gainModifiers = drcInstructionsUniDrc->gainModifiersForChannelGroup;
#else
        GainModifiers* gainModifiers = uniDrcConfig->drcInstructionsUniDrc->gainModifiersForChannelGroup;
#endif
        for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
        {
            if (gainModifiers[g].shapeFilterPresent == 1)
            {
                initShapeFilterBlock(&drcCoefficientsUniDrc->shapeFilterBlockParams[gainModifiers[g].shapeFilterIndex],
                                     &shapeFilterBlock[g]);
            }
            else
            {
                shapeFilterBlock[g].shapeFilterBlockPresent = 0;
            }
        }
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
    
    drcParams->drcSetCounter++;
    
    return (0);
}
