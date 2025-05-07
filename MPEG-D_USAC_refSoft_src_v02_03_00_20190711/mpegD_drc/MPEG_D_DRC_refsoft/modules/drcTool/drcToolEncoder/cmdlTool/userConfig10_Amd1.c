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
configureEnc10_Amd1(EncParams* encParams,
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
    encParams->domain = SUBBAND_DOMAIN;
    
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
        loudnessInfoSetExtEq->loudnessInfoV1[n].drcSetId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].eqSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].downmixId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevel = -5.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevel = 0.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        loudnessInfoSetExtEq->loudnessInfoV1[n].measurementCount = 2;
        m=0;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -15.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m=1;
        {
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].methodValue = -3.0f;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
            loudnessInfoSetExtEq->loudnessInfoV1[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  loudnessInfoV1Album  *************/
    loudnessInfoSetExtEq->loudnessInfoV1AlbumCount = 1;
    n=0;
    {
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].drcSetId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].eqSetId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].downmixId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].samplePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].truePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].measurementCount = 1;
        m=0;
        {
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].methodValue = -12.0f;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  channelLayout  *************/
    encConfig->channelLayout.baseChannelCount = 5;
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
        
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFiltersPresent = 0;

        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSequenceCount = 3;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetCount = 1;
        s=0;
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_REGULAR;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].fullFrame = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeAlignment = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeDeltaMinPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].bandCount = 4;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].drcBandType = 0;
            b=0;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 2;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].crossoverFreqIndex = 0; /* for drcBandType == 1 */
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].startSubBandIndex = 0; /* for drcBandType == 0 */
            }
            b=1;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 2;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].crossoverFreqIndex = 4;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].startSubBandIndex = 4;
            }
            b=2;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].crossoverFreqIndex = 8;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].startSubBandIndex = 8;
            }
            b=3;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].crossoverFreqIndex = 15;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].startSubBandIndex = 15;
            }
        }
    }

    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
    /***********  drcInstructionsUniDrcV1  *************/
    uniDrcConfigExt->drcInstructionsUniDrcV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetId = 1;
        /* uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetComplexityLevel = 0; will be computed */
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
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetEffect = EFFECT_BIT_NIGHT + EFFECT_BIT_LIMITED;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueUpper = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLowerPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=1;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=2;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = -1;
        ch=3;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = -1;
        ch=4;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = -1;
        
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].nDrcChannelGroups = 1;
        g=0;
        {
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterPresent = 0;
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterIndex = 0;
            b=0;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;

                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 0.1f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 0.1f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
            b=1;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;
                
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
            b=2;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;
                
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 0.8f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
            b=3;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;
                
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 0.3f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
        }
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTargetPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTarget = 0.0f;
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