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
configureEnc6_Amd1(EncParams* encParams,
              UniDrcConfig* encConfig,
              LoudnessInfoSet* encLoudnessInfoSet,
              UniDrcGainExt* encGainExtension)
{
    int n, m;
#if AMD1_SYNTAX
    int b, c, k, g, i, s, ch;
    LoudnessInfoSetExtEq* loudnessInfoSetExtEq = {0};
    UniDrcConfigExt* uniDrcConfigExt = {0};
    EqCoefficients* eqCoefficients = {0};
#endif /* AMD1_SYNTAX */
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
    encLoudnessInfoSet->loudnessInfoCount = 1;
    n=0;
    {
        encLoudnessInfoSet->loudnessInfo[n].drcSetId = 1;
        encLoudnessInfoSet->loudnessInfo[n].downmixId = 1;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].samplePeakLevel = 0.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelPresent = 1;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevel = 1.0f;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelMeasurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
        encLoudnessInfoSet->loudnessInfo[n].truePeakLevelReliability = RELIABILITY_ACCURATE;
        encLoudnessInfoSet->loudnessInfo[n].measurementCount = 2;
        m=0;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -10.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
        m=1;
        {
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].methodValue = -6.0f;
            encLoudnessInfoSet->loudnessInfo[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_EBU_R_128;
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
    
    
    /***********  loudnessInfoSetExt  *************/
#if AMD1_SYNTAX
    
    encLoudnessInfoSet->loudnessInfoSetExtPresent                   = 1;
    encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[0]   = UNIDRCLOUDEXT_EQ;
    encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[1]   = UNIDRCLOUDEXT_TERM;
    
    /***********  loudnessInfo  *************/
    loudnessInfoSetExtEq = & encLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq;
    loudnessInfoSetExtEq->loudnessInfoV1Count = 1;
    n=0;
    {
        loudnessInfoSetExtEq->loudnessInfoV1[n].drcSetId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].eqSetId = 2;
        loudnessInfoSetExtEq->loudnessInfoV1[n].downmixId = 0;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].samplePeakLevel = -5.0f;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevelPresent = 1;
        loudnessInfoSetExtEq->loudnessInfoV1[n].truePeakLevel = 2.0f;
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
    loudnessInfoSetExtEq->loudnessInfoV1AlbumCount = 2;
    n=0;
    {
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].drcSetId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].eqSetId = 1;
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
    n=1;
    {
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].drcSetId = 1;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].eqSetId = 2;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].downmixId = 2;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].samplePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].truePeakLevelPresent = 0;
        loudnessInfoSetExtEq->loudnessInfoV1Album[n].measurementCount = 1;
        m=0;
        {
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].methodDefinition = METHOD_DEFINITION_PROGRAM_LOUDNESS;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].methodValue = -8.0f;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].measurementSystem = MEASUREMENT_SYSTEM_BS_1770_3;
            loudnessInfoSetExtEq->loudnessInfoV1Album[n].loudnessMeasure[m].reliability = RELIABILITY_ACCURATE;
        }
    }
#else
    encLoudnessInfoSet->loudnessInfoSetExtPresent = 0;
#endif /* AMD1_SYNTAX */
   

    
    /***********  channelLayout  *************/
    encConfig->channelLayout.baseChannelCount = 5;
    encConfig->channelLayout.layoutSignalingPresent = 0;
    encConfig->channelLayout.definedLayout = 0;
    encConfig->channelLayout.speakerPosition[0] = 0;
    
    /***********  downmixInstructions  *************/
    encConfig->downmixInstructionsCount = 0;
    
    
    /***********  uniDrcConfigExt  *************/
    
#if AMD1_SYNTAX
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
        uniDrcConfigExt->downmixInstructionsV1[n].targetChannelCount = 2;
        uniDrcConfigExt->downmixInstructionsV1[n].targetLayout = 0;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficientsPresent = 1;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*0+0] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*0+1] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*0+2] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*0+3] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*0+4] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*1+0] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*1+1] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*1+2] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*1+3] = 0.1f;
        uniDrcConfigExt->downmixInstructionsV1[n].downmixCoefficient[5*1+4] = 0.1f;
    }
    
    /***********  drcCoefficientsUniDrcV1  *************/
    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
    uniDrcConfigExt->drcCoefficientsUniDrcV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcLocation = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSizePresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSize = encParams->frameSize;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicLeftPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicLeftCount = 1;
        k=1;
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[k].characteristicFormat = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[k].bsGain = 20;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[k].bsIoRatio = 4;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[k].bsExp = 5;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[k].flipSign = 0;
        }
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicRightPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicRightCount = 1;
        k=1;
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[k].characteristicFormat = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[k].bsGain = -30;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[k].bsIoRatio = 5;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[k].bsExp = 7;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[k].flipSign = 0;
        }
        
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

    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
    
    
    uniDrcConfigExt->loudEqInstructionsPresent = 0;
    uniDrcConfigExt->eqPresent                 = 0;

    /***********  loudEqInstructions  *************/
    uniDrcConfigExt->loudEqInstructionsPresent = 1;
    uniDrcConfigExt->loudEqInstructionsCount = 1;
    n=0;
    {
        uniDrcConfigExt->loudEqInstructions[n].loudEqSetId = 1;
        uniDrcConfigExt->loudEqInstructions[n].drcLocation = 1;
        uniDrcConfigExt->loudEqInstructions[n].downmixIdPresent = 0;
        uniDrcConfigExt->loudEqInstructions[n].drcSetIdPresent = 1;
        uniDrcConfigExt->loudEqInstructions[n].drcSetId = 0x3F;
        uniDrcConfigExt->loudEqInstructions[n].eqSetIdPresent = 1;
        uniDrcConfigExt->loudEqInstructions[n].eqSetId = 0x3F;
        uniDrcConfigExt->loudEqInstructions[n].loudnessAfterDrc = 1;
        uniDrcConfigExt->loudEqInstructions[n].loudnessAfterEq = 1;
        uniDrcConfigExt->loudEqInstructions[n].loudEqGainSequenceCount = 1;
        s=0;
        {
            uniDrcConfigExt->loudEqInstructions[n].gainSequenceIndex[s] = 0;
            uniDrcConfigExt->loudEqInstructions[n].drcCharacteristicFormatIsCICP[s] = 0;
            uniDrcConfigExt->loudEqInstructions[n].drcCharacteristicLeftIndex[s] = 1;
            uniDrcConfigExt->loudEqInstructions[n].drcCharacteristicRightIndex[s] = 1;
            uniDrcConfigExt->loudEqInstructions[n].frequencyRangeIndex[s] = 0;
            uniDrcConfigExt->loudEqInstructions[n].loudEqScaling[s] = .5f;
            uniDrcConfigExt->loudEqInstructions[n].loudEqOffset[s] = -30.0f;
        }
    }

    uniDrcConfigExt->eqPresent = 1;
    /***********  eqCoefficients  *************/
    eqCoefficients = &uniDrcConfigExt->eqCoefficients;
    
    eqCoefficients->eqDelayMaxPresent = 1;
    eqCoefficients->eqDelayMax        = 0;
    eqCoefficients->uniqueFilterBlockCount = 0;
    eqCoefficients->uniqueTdFilterElementCount = 0;
    
    eqCoefficients->uniqueEqSubbandGainsCount = 4;
    eqCoefficients->eqSubbandGainRepresentation = 1;
    eqCoefficients->eqSubbandGainFormat = GAINFORMAT_UNIFORM;
    eqCoefficients->eqSubbandGainCount = 64;
    
    s=0;
    {
        EqSubbandGainSpline* spline = &eqCoefficients->eqSubbandGainSpline[s];
        spline->eqGainInitial = -50.0f;
        spline->eqSlope[0] = 0.0f;
        n=1;
        {
            spline->eqSlope[n] = 10.0f;
            spline->eqGainDelta[n] = 0.0f;
            spline->eqFreqDelta[n] = 10;
        }
        n++;
        {
            spline->eqSlope[n] = 20.0f;
            spline->eqGainDelta[n] = 22.0f;
            spline->eqFreqDelta[n] = 4;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 18.0f;
            spline->eqFreqDelta[n] = 4;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = -5.0f;
            spline->eqFreqDelta[n] = 4;
        }
        n++;
        {
            spline->eqSlope[n] = -5.0f;
            spline->eqGainDelta[n] = -30.0f;
            spline->eqFreqDelta[n] = 10;
        }
        n++;
        spline->nEqNodes = n;
    }
    s++;
    {
        EqSubbandGainSpline* spline = &eqCoefficients->eqSubbandGainSpline[s];
        spline->eqGainInitial = 10.0f;
        spline->eqSlope[0] = 10.0f;
        n=1;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 0.0f;
            spline->eqFreqDelta[n] = 15;
        }
        n++;
        {
            spline->eqSlope[n] = -10.0f;
            spline->eqGainDelta[n] = -8.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        {
            spline->eqSlope[n] = 5.0f;
            spline->eqGainDelta[n] = 8.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        spline->nEqNodes = n;
    }
    s++;
    {
        EqSubbandGainSpline* spline = &eqCoefficients->eqSubbandGainSpline[s];
        spline->eqGainInitial = 0.0f;
        spline->eqSlope[0] = 4.0f;
        n=1;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 5.0f;
            spline->eqFreqDelta[n] = 15;
        }
        n++;
        {
            spline->eqSlope[n] = 5.0f;
            spline->eqGainDelta[n] = 0.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        {
            spline->eqSlope[n] = 5.0f;
            spline->eqGainDelta[n] = -5.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 5.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        spline->nEqNodes = n;
    }
    s++;
    {
        EqSubbandGainSpline* spline = &eqCoefficients->eqSubbandGainSpline[s];
        spline->eqGainInitial = -10.0f;
        spline->eqSlope[0] = 0.0f;
        n=1;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = -5.0f;
            spline->eqFreqDelta[n] = 15;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 5.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = 0.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        {
            spline->eqSlope[n] = 0.0f;
            spline->eqGainDelta[n] = -5.0f;
            spline->eqFreqDelta[n] = 5;
        }
        n++;
        spline->nEqNodes = n;
    }
    
/*
     eqCoefficients->eqSubbandGainVector ...
*/
    
    /***********  eqInstructions  *************/
    uniDrcConfigExt->eqInstructionsCount = 2;

    i=0;
    {
        EqInstructions* eqInstructions = &uniDrcConfigExt->eqInstructions[i];
        eqInstructions->eqSetId = i+1;
        /* eqInstructions->eqSetComplexityLevel = 2; */
        eqInstructions->downmixIdPresent = 0;
        eqInstructions->eqApplyToDownmix = 0;
        eqInstructions->drcSetId = 0;
        eqInstructions->additionalDrcSetIdPresent = 0;
        eqInstructions->eqSetPurpose = EQ_PURPOSE_GENERIC;
        eqInstructions->dependsOnEqSetPresent = 0;
        eqInstructions->dependsOnEqSet = 0;
        eqInstructions->noIndependentEqUse = 0;
        eqInstructions->eqChannelCount = 5;
        eqInstructions->eqChannelGroupCount = 4;
        eqInstructions->eqChannelGroupForChannel[0] = 0;
        eqInstructions->eqChannelGroupForChannel[1] = 1;
        eqInstructions->eqChannelGroupForChannel[2] = 1;
        eqInstructions->eqChannelGroupForChannel[3] = 2;
        eqInstructions->eqChannelGroupForChannel[4] = 3;
        eqInstructions->tdFilterCascadePresent = 0;
        eqInstructions->subbandGainsPresent = 1;
        
        eqInstructions->subbandGainsIndex[0] = 3;
        eqInstructions->subbandGainsIndex[1] = 2;
        eqInstructions->subbandGainsIndex[2] = 1;
        eqInstructions->subbandGainsIndex[3] = 0;

        eqInstructions->eqTransitionDurationPresent = 1;
        eqInstructions->eqTransitionDuration = 0.5f;
    }
    i=1;
    {
        EqInstructions* eqInstructions = &uniDrcConfigExt->eqInstructions[i];
        eqInstructions->eqSetId = i+1;
        /* eqInstructions->eqSetComplexityLevel = 2; */
        eqInstructions->downmixIdPresent = 0;
        eqInstructions->eqApplyToDownmix = 0;
        eqInstructions->drcSetId = 1;
        eqInstructions->additionalDrcSetIdPresent = 0;
        eqInstructions->eqSetPurpose = EQ_PURPOSE_LARGE_ROOM;
        eqInstructions->dependsOnEqSetPresent = 0;
        eqInstructions->dependsOnEqSet = 0;
        eqInstructions->noIndependentEqUse = 0;
        eqInstructions->eqChannelCount = 5;
        eqInstructions->eqChannelGroupCount = 1;
        eqInstructions->eqChannelGroupForChannel[0] = 0;
        eqInstructions->eqChannelGroupForChannel[1] = 0;
        eqInstructions->eqChannelGroupForChannel[2] = 0;
        eqInstructions->eqChannelGroupForChannel[3] = 0;
        eqInstructions->eqChannelGroupForChannel[4] = 0;
        eqInstructions->tdFilterCascadePresent = 0;
        eqInstructions->subbandGainsPresent = 1;
        g=0;
        {
            eqInstructions->subbandGainsIndex[g] = 0;
        }
        eqInstructions->eqTransitionDurationPresent = 1;
        eqInstructions->eqTransitionDuration = 0.5f;
    }
#else
    encConfig->uniDrcConfigExtPresent  = 0;
#endif /* AMD1_SYNTAX */
        
    encConfig->drcDescriptionBasicPresent = 0;
    encGainExtension->uniDrcGainExtPresent = 0;

    return (0);
}

#endif /* AMD1_SYNTAX */

#endif
