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

#if MPEG_H_SYNTAX

int
configureEncMpegh1(EncParams* encParams,
                       UniDrcConfig* encConfig,
                       LoudnessInfoSet* encLoudnessInfoSet,
                       UniDrcGainExt* encGainExtension)
{
    int n, g, s, m, ch;
    
    encParams->frameSize = 1024;
    encParams->sampleRate = 48000;
    encParams->delayMode = DELAY_MODE_REGULAR_DELAY;
    
    encConfig->sampleRatePresent = 1;
    encConfig->sampleRate = encParams->sampleRate;
    encConfig->drcCoefficientsUniDrc->drcFrameSizePresent = 1;
    encConfig->drcCoefficientsUniDrc->drcFrameSize = encParams->frameSize;
    encConfig->loudnessInfoSetPresent = 0;
   
    /***********  drcInstructionsUniDrc  *************/
    encConfig->drcInstructionsUniDrcCount = 5;
    n=0;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = n+1;
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NIGHT;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = -20;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=2;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=3;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=4;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        for (g=0; g<encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups; g++)
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
        encConfig->drcInstructionsUniDrc[n].drcInstructionsType = 0;
        encConfig->drcInstructionsUniDrc[n].mae_groupID = 0;
        encConfig->drcInstructionsUniDrc[n].mae_groupPresetID = 0;
    }
    n=1;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = n+1;
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NIGHT;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = -20;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=2;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=3;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=4;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        for (g=0; g<encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups; g++)
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
        encConfig->drcInstructionsUniDrc[n].drcInstructionsType = 2;
        encConfig->drcInstructionsUniDrc[n].mae_groupID = 1;
        encConfig->drcInstructionsUniDrc[n].mae_groupPresetID = 0;
    }
    n=2;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = n+1;
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NOISY;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = -20;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=2;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=3;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=4;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        for (g=0; g<encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups; g++)
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
        encConfig->drcInstructionsUniDrc[n].drcInstructionsType = 2;
        encConfig->drcInstructionsUniDrc[n].mae_groupID = 2;
        encConfig->drcInstructionsUniDrc[n].mae_groupPresetID = 0;
    }
    n=3;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = n+1;
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NIGHT;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = -20;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=2;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=3;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        ch=4;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 1;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        for (g=0; g<encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups; g++)
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
        encConfig->drcInstructionsUniDrc[n].drcInstructionsType = 3;
        encConfig->drcInstructionsUniDrc[n].mae_groupID = 0;
        encConfig->drcInstructionsUniDrc[n].mae_groupPresetID = 1;
    }
    n=4;
    {
        encConfig->drcInstructionsUniDrc[n].drcSetId = n+1;
        encConfig->drcInstructionsUniDrc[n].downmixId = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdPresent = 0;
        encConfig->drcInstructionsUniDrc[n].additionalDownmixIdCount = 0;
        encConfig->drcInstructionsUniDrc[n].drcLocation = 1;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].dependsOnDrcSet = 0;
        encConfig->drcInstructionsUniDrc[n].noIndependentUse = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetEffect = EFFECT_BIT_NIGHT;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessPresent = 1;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueUpper = -23;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLowerPresent = 0;
        encConfig->drcInstructionsUniDrc[n].drcSetTargetLoudnessValueLower = 0;
        ch=0;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=1;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=2;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        ch=3;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = -1;
        ch=4;
        encConfig->drcInstructionsUniDrc[n].gainSetIndex[ch] = 0;
        
        encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups = 1;
        for (g=0; g<encConfig->drcInstructionsUniDrc[n].nDrcChannelGroups; g++)
        {
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainScalingPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].attenuationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].amplificationScaling[0] = 1.0f;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffsetPresent[0] = 0;
            encConfig->drcInstructionsUniDrc[n].gainModifiers[g].gainOffset[0] = 0.0f;
        }
        encConfig->drcInstructionsUniDrc[n].limiterPeakTargetPresent = 0;
        encConfig->drcInstructionsUniDrc[n].limiterPeakTarget = 0.0f;
        encConfig->drcInstructionsUniDrc[n].drcInstructionsType = 3;
        encConfig->drcInstructionsUniDrc[n].mae_groupID = 0;
        encConfig->drcInstructionsUniDrc[n].mae_groupPresetID = 2;
    }
    
    /***********  drcCoefficientsUniDrc  *************/
    encConfig->drcCoefficientsUniDrcCount = 1;
    n=0;
    {
        encConfig->drcCoefficientsUniDrc[n].drcLocation = 1;
        encConfig->drcCoefficientsUniDrc[n].gainSetCount = 5;
        s=0;
        {
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_REGULAR;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].fullFrame = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeAlignment = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeDeltaMinPresent = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].bandCount = 4;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].drcBandType = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].drcCharacteristic = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].crossoverFreqIndex = 0;  /* don't use this field, it is not transmitted */
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[1].drcCharacteristic = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[1].crossoverFreqIndex = 5;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[2].drcCharacteristic = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[2].crossoverFreqIndex = 10;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[3].drcCharacteristic = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[3].crossoverFreqIndex = 15;
        }
        s=1;
        {
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_REGULAR;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].fullFrame = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeAlignment = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeDeltaMinPresent = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].bandCount = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].drcCharacteristic = 3;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].crossoverFreqIndex = 0;  /* don't use this field, it is not transmitted */
        }
        s=2;
        {
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_CLIPPING;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].fullFrame = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeAlignment = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeDeltaMinPresent = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].bandCount = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].drcCharacteristic = 7;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].crossoverFreqIndex = 0;  /* don't use this field, it is not transmitted */
        }
        s=3;
        {
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_FADING;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].fullFrame = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeAlignment = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeDeltaMinPresent = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].bandCount = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].drcCharacteristic = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].crossoverFreqIndex = 0;  /* don't use this field, it is not transmitted */
        }
        s=4;
        {
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_DUCKING;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].fullFrame = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeAlignment = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].timeDeltaMinPresent = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].bandCount = 1;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].drcCharacteristic = 0;
            encConfig->drcCoefficientsUniDrc[n].gainSetParams[s].gainParams[0].crossoverFreqIndex = 0;  /* don't use this field, it is not transmitted */
        }
    }
    
    /***********  loudnessInfo  *************/
    encLoudnessInfoSet->loudnessInfoCount = 9;
    n=0;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 0;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 0;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -27.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=1;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 1;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 0;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -22.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=2;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 1;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 1;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -25.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=3;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 1;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 3;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -22.5f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=4;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 2;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 1;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -26.5f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=5;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 2;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 2;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -24.25f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=6;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 2;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 3;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 0;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = -1.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -13.75f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=7;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 3;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 0;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 1;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 0.5f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -16.5f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    n=8;
    {
        encLoudnessInfoSet->loudnessInfo[n].loudnessInfoType = 3;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupID = 0;
        encLoudnessInfoSet->loudnessInfo[n].mae_groupPresetID = 2;
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 0;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = -2.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -18.5f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
    
    /***********  loudnessInfoAlbum  *************/
    encLoudnessInfoSet->loudnessInfoAlbumCount = 1;
    n=0;
    {
        encLoudnessInfoSet->loudnessInfoAlbum[n].drcSetId = 1;
        encLoudnessInfoSet->loudnessInfoAlbum[n].downmixId = 2;
        encLoudnessInfoSet->loudnessInfoAlbum[n].samplePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfoAlbum[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfoAlbum[n].truePeakLevelPresent = 0;
        encLoudnessInfoSet->loudnessInfoAlbum[n].measurementCount = 1;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].methodDefinition = 0;  /* 0 = program loudness */
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].methodValue = -10.0f;
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].measurementSystem = 2; /* 2 = ITU-R BS.1770-3 */
            encLoudnessInfoSet->loudnessInfoAlbum[n].loudnessMeasure[m].reliability = 0;       /* 0 = unknown */
        }
    }
    
    /***********  channelLayout  *************/
    encConfig->channelLayout.baseChannelCount = 5;
    encConfig->channelLayout.layoutSignalingPresent = 0;
    encConfig->channelLayout.definedLayout = 0;
    encConfig->channelLayout.speakerPosition[0] = 0;
    
    /***********  downmixInstructions  *************/
    encConfig->downmixInstructionsCount = 3;
    n=0;
    {
        encConfig->downmixInstructions[n].downmixId = 1;
        encConfig->downmixInstructions[n].targetChannelCount = 2;
        encConfig->downmixInstructions[n].targetLayout = 0;
        encConfig->downmixInstructions[n].downmixCoefficientsPresent = 0;
    }
    n=1;
    {
        encConfig->downmixInstructions[n].downmixId = 2;
        encConfig->downmixInstructions[n].targetChannelCount = 4;
        encConfig->downmixInstructions[n].targetLayout = 0;
        encConfig->downmixInstructions[n].downmixCoefficientsPresent = 0;
    }
    n=2;
    {
        encConfig->downmixInstructions[n].downmixId = 3;
        encConfig->downmixInstructions[n].targetChannelCount = 2;
        encConfig->downmixInstructions[n].targetLayout = 0;
        encConfig->downmixInstructions[n].downmixCoefficientsPresent = 0;
    }

    encConfig->drcDescriptionBasicPresent               = 0;
    encConfig->uniDrcConfigExtPresent                   = 0;
    encLoudnessInfoSet->loudnessInfoSetExtPresent       = 0;
    encGainExtension->uniDrcGainExtPresent              = 0;

    return (0);
}

#endif
