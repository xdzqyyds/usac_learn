/***********************************************************************************
 
 This software module was originally developed by
 
 Fraunhofer IIS
 
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
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#include <stdio.h>
#include "drcToolEncoder.h"
#include "userConfig.h"

#if !MPEG_H_SYNTAX

#if AMD1_SYNTAX

int
configureEnc7_Amd1(EncParams* encParams,
                   UniDrcConfig* encConfig,
                   LoudnessInfoSet* encLoudnessInfoSet,
                   UniDrcGainExt* encGainExtension)
{
    int n, g, s, m, ch;
    
    encParams->frameSize = 1024;
    encParams->sampleRate = 48000;
    encParams->delayMode = DELAY_MODE_REGULAR_DELAY;
    encParams->domain = TIME_DOMAIN;

    encConfig->sampleRatePresent = 1;
    encConfig->sampleRate = encParams->sampleRate;
    
    /***********  drcInstructionsUniDrc  *************/
    encConfig->drcInstructionsUniDrcCount = 3;
    n=0;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = 1;
        /*encConfig->drcInstructionsUniDrc[n].drcSetComplexityLevel = 5; will be computed */
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_GENERAL_COMPR;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        g=0;
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
    }
    n=1;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = 2;
        /*encConfig->drcInstructionsUniDrc[n].drcSetComplexityLevel = 4; will be computed */
        encConfig->drcInstructionsUniDrc[n].downmixId = 0x7F;
        encConfig->drcInstructionsUniDrc[n].downmixIdPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcApplyToDownmix = 1;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 1;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_LIMITED;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        g=0;
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 1;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = -14.f;
    }
    n=2;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = 3;
        /*encConfig->drcInstructionsUniDrc[n].drcSetComplexityLevel = 4; will be computed */
        encConfig->drcInstructionsUniDrc[n].downmixId = 0x7F;
        encConfig->drcInstructionsUniDrc[n].downmixIdPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcApplyToDownmix = 1;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 1;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NOISY;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        g=0;
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 1;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = -14.0f;
    }
    
    /***********  drcCoefficientsUniDrc  *************/
    encConfig->drcCoefficientsUniDrcCount = 0;
    n=0;
    {
        encConfig->drcCoefficientsUniDrc[n].drcLocation = 1;
        encConfig->drcCoefficientsUniDrc[n].gainSetCount = 0;
    }
    if (encConfig->drcCoefficientsUniDrcCount == 0) {
        encParams->parametricDrcOnly = 1;
        encParams->frameCount        = 10000;
    } else {
        encParams->parametricDrcOnly = 0;
        encParams->frameCount        = 0;
    }
    
    /***********  loudnessInfo  *************/
    encLoudnessInfoSet->loudnessInfoCount = 1;
    n=0;
    {
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -24.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  loudnessInfoAlbum  *************/
    encLoudnessInfoSet->loudnessInfoAlbumCount = 1;
    n=0;
    {
        encLoudnessInfoSet->loudnessInfoAlbum[n].drcSetId = 1;
        encLoudnessInfoSet->loudnessInfoAlbum[n].downmixId = 1;
        encLoudnessInfoSet->loudnessInfoAlbum[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfoAlbum[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfoAlbum[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].methodValue = -10.0f;
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  channelLayout  *************/
    encConfig->channelLayout.baseChannelCount = 2;
    encConfig->channelLayout.layoutSignalingPresent = 0;
    encConfig->channelLayout.definedLayout = 0;
    encConfig->channelLayout.speakerPosition[0] = 0;
    
    /***********  downmixInstructions  *************/
    encConfig->downmixInstructionsCount = 1;
    n=0;
    {
        encConfig->downmixInstructions[n].downmixId = 1;
        encConfig->downmixInstructions[n].targetChannelCount = 1;
        encConfig->downmixInstructions[n].targetLayout = 0;
        encConfig->downmixInstructions[n].downmixCoefficientsPresent = 1;
        encConfig->downmixInstructions[n].downmixCoefficient[0] = 0.5f;
        encConfig->downmixInstructions[n].downmixCoefficient[1] = 0.5f;
    }
    
    encConfig->drcDescriptionBasicPresent               = 0;
    encLoudnessInfoSet->loudnessInfoSetExtPresent       = 0;
    encGainExtension->uniDrcGainExtPresent              = 0;
    
    /***********  uniDrcConfigExt  *************/
    
    encConfig->uniDrcConfigExtPresent                   = 1;
    encConfig->uniDrcConfigExt.parametricDrcPresent     = 1;
    encConfig->uniDrcConfigExt.uniDrcConfigExtType[0]   = UNIDRCCONFEXT_PARAM_DRC;
    encConfig->uniDrcConfigExt.uniDrcConfigExtType[1]   = UNIDRCCONFEXT_TERM;
    
    /***********  drcCoefficientsParametricDrc  *************/
    
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.drcLocation = 1;
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSize = 64;
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMaxPresent = 1;
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMax        = 768;
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.resetParametricDrc = 0;
    
    /***********  parametricDrcSequenceParams  *************/
    encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetCount = 2;
    s=0;
    {
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].parametricDrcId = 7;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].sideChainConfigType = 0;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].downmixId = 0;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeightFormat = 0;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeight[0] = 1.f;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeight[1] = 1.f;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].drcInputLoudnessPresent = 1;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].drcInputLoudness = -24.f;
    }
    s=1;
    {
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].parametricDrcId = 5;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].sideChainConfigType = 0;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].downmixId = 0;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeightFormat = 0;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeight[0] = 1.f;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].levelEstimChannelWeight[1] = 1.f;
        
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].drcInputLoudnessPresent = 1;
        encConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[s].drcInputLoudness = -24.f;
    }
    
    /***********  parametricDrcInstructions  *************/
    encConfig->uniDrcConfigExt.parametricDrcInstructionsCount = 2;
    s=0;
    {
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcId = 7;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcLookAheadPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcLookAhead = 12;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcPresetIdPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcPresetId = 1;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcType = PARAM_DRC_TYPE_FF;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.levelEstimKWeightingType = 2;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.levelEstimIntegrationTimePresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.levelEstimIntegrationTime = 128;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.drcCurveDefinitionType = 1;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.drcCharacteristic = 8;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeCount = 5;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeLevel[0] = -12;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeLevel[1] = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeLevel[2] = 5;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeLevel[3] = 15;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeLevel[4] = 35;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeGain[0]  = 6;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeGain[1]  = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeGain[2]  = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeGain[3]  = -5;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.nodeGain[4]  = -24;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.drcGainSmoothParametersPresent = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothAttackTimeSlow = 100;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothReleaseTimeSlow = 3000;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothTimeFastPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothAttackTimeFast = 10;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothReleaseTimeFast = 1000;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothThresholdPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothAttackThreshold = 15;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothReleaseThreshold = 20;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothHoldOffCountPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeFeedForward.gainSmoothHoldOff = 10;
    }
    s=1;
    {
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcId = 5;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcLookAheadPresent = 1;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcLookAhead = 4;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcPresetIdPresent = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcPresetId = 0;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcType = PARAM_DRC_TYPE_LIM;
        
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeLim.parametricLimThresholdPresent = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeLim.parametricLimThreshold = -1.f;

        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeLim.parametricLimReleasePresent = 0;
        encConfig->uniDrcConfigExt.parametricDrcInstructions[s].parametricDrcTypeLim.parametricLimRelease = 50;
    }
    
    return (0);
}

#endif /* AMD1_SYNTAX */

#endif
