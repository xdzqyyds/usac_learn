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
#include "drcToolEncoder.h"
#include "userConfig.h"

#if !MPEG_H_SYNTAX

#if AMD1_SYNTAX

int
configureEnc13_Amd1(EncParams* encParams,
              UniDrcConfig* encConfig,
              LoudnessInfoSet* encLoudnessInfoSet,
              UniDrcGainExt* encGainExtension)
{
    int n, m;
    LoudnessInfoSetExtEq* loudnessInfoSetExtEq = {0};
    UniDrcConfigExt* uniDrcConfigExt = {0};
    encParams->frameSize = 1024;
    encParams->sampleRate = 48000;
    encParams->delayMode = DELAY_MODE_REGULAR_DELAY;
    encParams->domain = TIME_DOMAIN;

    encConfig->sampleRatePresent = 1;
    encConfig->sampleRate = encParams->sampleRate;
 
    /***********  drcInstructionsUniDrc  *************/
    encConfig->drcInstructionsUniDrcCount = 0;
    
    /***********  drcCoefficientsUniDrc  *************/
    encConfig->drcCoefficientsUniDrcCount = 0;
    
    /***********  loudnessInfo  *************/
    encLoudnessInfoSet->loudnessInfoCount = 0;
    
    /***********  loudnessInfoAlbum  *************/
    encLoudnessInfoSet->loudnessInfoAlbumCount = 0;
    
    /***********  loudnessInfoSetExt  *************/
    encLoudnessInfoSet->loudnessInfoSetExtPresent                   = 1;
    encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[0]   = UNIDRCLOUDEXT_EQ;
    encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[1]   = UNIDRCLOUDEXT_TERM;
    
    /***********  loudnessInfo  *************/
    loudnessInfoSetExtEq = & encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq;
    loudnessInfoSetExtEq->loudnessInfoV1Count = 2;
    n=0;
    {
        loudnessInfoSetExtEq->loudnessInfoV1[n].drcSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].eqSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].downmixId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevel = 0.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevel = -5.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        loudnessInfoSetExtEq->loudnessInfoV1[n].measurementCount = 6;
        m=0;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -15.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_ANCHOR_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -18.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_UNKNOWN_OTHER;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -10.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -6.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -7.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_LOUDNESS_RANGE;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = 10.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=1;
    {
        loudnessInfoSetExtEq->loudnessInfoV1[n].drcSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].eqSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].downmixId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevel = 0.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevel = -2.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        loudnessInfoSetExtEq->loudnessInfoV1[n].measurementCount = 6;
        m=0;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -14.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_ANCHOR_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -17.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_UNKNOWN_OTHER;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -9.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -5.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -6.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m++;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_LOUDNESS_RANGE;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = 9.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  loudnessInfoV1Album  *************/
    loudnessInfoSetExtEq->loudnessInfoV1AlbumCount = 0;
    
    /***********  channelLayout  *************/
    encConfig->channelLayout.baseChannelCount = 2;
    encConfig->channelLayout.layoutSignalingPresent = 0;
    encConfig->channelLayout.definedLayout = 0;
    encConfig->channelLayout.speakerPosition[0] = 0;
    
    /***********  downmixInstructions  *************/
    encConfig->downmixInstructionsCount = 0;
    
    /***********  uniDrcConfigExt  *************/
    
    encConfig->uniDrcConfigExtPresent                   = 1;
    encConfig->uniDrcConfigExt.uniDrcConfigExtType[0]   = UNIDRCCONFEXT_V1;
    encConfig->uniDrcConfigExt.uniDrcConfigExtType[1]   = UNIDRCCONFEXT_TERM;

    uniDrcConfigExt = &encConfig->uniDrcConfigExt;
    
    /***********  downmixInstructionsV1  *************/
    uniDrcConfigExt->downmixInstructionsV1Present = 1;
    uniDrcConfigExt->downmixInstructionsV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->downmixInstructionsV1[n].downmixId = 1;
        uniDrcConfigExt->downmixInstructionsV1[n].targetChannelCount = 1;
        uniDrcConfigExt->downmixInstructionsV1[n].targetLayout = 0;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficientsPresent = 1;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[0] = 0.5f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[1] = 0.5f;
    }
    
    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 0;

    /***********  drcCoefficientsUniDrcV1  *************/
    uniDrcConfigExt->drcCoefficientsUniDrcV1Count = 0;
    if (uniDrcConfigExt->drcCoefficientsUniDrcV1Count == 0) {
        encParams->frameCount        = 10000;
    } else {
        encParams->frameCount        = 0;
    }

    /***********  drcInstructionsUniDrcV1  *************/
    uniDrcConfigExt->drcInstructionsUniDrcV1Count = 0;
    
    /***********  loudEqInstructions  *************/
    uniDrcConfigExt->loudEqInstructionsPresent = 0;

    /***********  EQ  *************/
    uniDrcConfigExt->eqPresent = 0;
    
    encConfig->drcDescriptionBasicPresent = 0;
    encGainExtension->uniDrcGainExtPresent = 0;

    return (0);
}

#endif /* AMD1_SYNTAX */

#endif