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
configureEnc11_Amd1(EncParams* encParams,
              UniDrcConfig* encConfig,
              LoudnessInfoSet* encLoudnessInfoSet,
              UniDrcGainExt* encGainExtension)
{
    int n, m;
    int b, g, s, ch;
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
    loudnessInfoSetExtEq->loudnessInfoV1Count = 1;
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
    
    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;

    /***********  drcCoefficientsUniDrcV1  *************/
    uniDrcConfigExt->drcCoefficientsUniDrcV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcLocation = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSizePresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSize = encParams->frameSize;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicLeftPresent = 0;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicLeftCount = 0;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicRightPresent = 0;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicRightCount = 0;
        
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFiltersPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterCount = 1;
        s=1; /* filter for index 0 is not transmitted */
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutFilterPresent = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutParams.cornerFreqIndex = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutParams.filterStrengthIndex = 2;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostFilterPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostParams.cornerFreqIndex = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostParams.filterStrengthIndex = 0;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutFilterPresent = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutParams.cornerFreqIndex = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutParams.filterStrengthIndex = 1;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostFilterPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostParams.cornerFreqIndex = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostParams.filterStrengthIndex = 0;
        }
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSequenceCount = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetCount = 1;
        s=0;
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_REGULAR;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].fullFrame = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeAlignment = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeDeltaMinPresent = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].deltaTmin = 32;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].bandCount = 1;
            b=0;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 0;
            }
        }
    }
    /***********  drcInstructionsUniDrcV1  *************/
    uniDrcConfigExt->drcInstructionsUniDrcV1Count = 2;
    n=0;
    {
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetId = 1;
     /* uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetComplexityLevel = 5; will be computed */
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].downmixIdPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].downmixId = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcApplyToDownmix = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].additionalDownmixIdPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].additionalDownmixIdCount = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcLocation = 1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].dependsOnDrcSetPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].dependsOnDrcSet = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].noIndependentUse = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].requiresEq = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetEffect = EFFECT_BIT_NOISY + EFFECT_BIT_LIMITED;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueUpper = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLowerPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].nDrcChannelGroups = 1;
        g=0;
        {
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterPresent = 1;
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterIndex = 1;
            b=0;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;

                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
        }
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTargetPresent = 1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTarget = -3.0f;
    }
    n=1;
    {
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetId = 2;
     /* uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetComplexityLevel = 5;  will be computed */
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].downmixIdPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].downmixId = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcApplyToDownmix = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].additionalDownmixIdPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].additionalDownmixIdCount = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcLocation = 1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].dependsOnDrcSetPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].dependsOnDrcSet = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].noIndependentUse = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].requiresEq = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetEffect = EFFECT_BIT_NIGHT;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueUpper = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLowerPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].nDrcChannelGroups = 1;
        g=0;
        {
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterPresent = 1;
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterIndex = 1;
            b=0;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;
                
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 0.66f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
        }
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTargetPresent = 1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTarget = -3.0f;
    }
    
    
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