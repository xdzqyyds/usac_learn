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
configureEnc5_Amd1(EncParams* encParams,
              UniDrcConfig* encConfig,
              LoudnessInfoSet* encLoudnessInfoSet,
              UniDrcGainExt* encGainExtension)
{
    int n, m;
#if AMD1_SYNTAX
    int b, c, k, e, g, i, s, ch;
    LoudnessInfoSetExtEq* loudnessInfoSetExtEq = {0};
    UniDrcConfigExt* uniDrcConfigExt = {0};
    EqCoefficients* eqCoefficients = {0};
#endif /* AMD1_SYNTAX */
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

    /***********  drcCoefficientsUniDrcV1  *************/
    uniDrcConfigExt->drcCoefficientsUniDrcV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcLocation = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSizePresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcFrameSize = encParams->frameSize;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicLeftPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicLeftCount = 1;
        c=1; /* characteristic at index 0 is not transmitted */
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].characteristicFormat = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].bsIoRatio = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].bsGain = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].bsExp = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].flipSign = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].characteristicNodeCount = 4;
            k = 0;  /* node at index 0 is pre-defined (not transmitted) */
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeLevel[k] = -31.0f; /* fixed */
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeGain[k] = 0.0f; /* fixed */
            }
            k = 1;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeLevel[k] = -35.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeGain[k] = 5.0f;
            }
            k = 2;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeLevel[k] = -45.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeGain[k] = 10.0f;
            }
            k = 3;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeLevel[k] = -60.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeGain[k] = 17.0f;
            }
            k = 4;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeLevel[k] = -65.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicLeft[c].nodeGain[k] = 23.0f;
            }
        }
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].drcCharacteristicRightPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].characteristicRightCount = 1;
        c=1; /* characteristic at index 0 is not transmitted */
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].characteristicFormat = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].bsIoRatio = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].bsGain = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].bsExp = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].flipSign = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].characteristicNodeCount = 4;
            k = 0;  /* node at index 0 is pre-defined (not transmitted) */
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeLevel[k] = -31.0f; /* fixed */
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeGain[k] = 0.0f; /* fixed */
            }
            k = 1;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeLevel[k] = -25.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeGain[k] = -5.0f;
            }
            k = 2;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeLevel[k] = -20.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeGain[k] = -10.0f;
            }
            k = 3;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeLevel[k] = -10.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeGain[k] = -17.0f;
            }
            k = 4;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeLevel[k] = -5.0f;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].splitCharacteristicRight[c].nodeGain[k] = -23.0f;
            }
        }
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFiltersPresent = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterCount = 1;
        s=1; /* filter for index 0 is not transmitted */
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutFilterPresent = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutParams.cornerFreqIndex = 2;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfCutParams.filterStrengthIndex = 2;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostFilterPresent = 1;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostParams.cornerFreqIndex = 2;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].lfBoostParams.filterStrengthIndex = 2;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutFilterPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutParams.cornerFreqIndex = 2;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfCutParams.filterStrengthIndex = 2;
            
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostFilterPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostParams.cornerFreqIndex = 2;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].shapeFilterBlockParams[s].hfBoostParams.filterStrengthIndex = 2;
        }
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSequenceCount = 1;
        uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetCount = 1;
        s=0;
        {
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainCodingProfile = GAIN_CODING_PROFILE_REGULAR;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainInterpolationType = GAIN_INTERPOLATION_TYPE_SPLINE;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].fullFrame = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeAlignment = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].timeDeltaMinPresent = 0;
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].deltaTmin = getDeltaTmin(encParams->sampleRate);
            uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].bandCount = 1;
            b=0;
            {
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].gainSequenceIndex = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicPresent = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicFormatIsCICP = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristic = 0;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicLeftIndex = 1;
                uniDrcConfigExt->drcCoefficientsUniDrcV1[n].gainSetParams[s].gainParams[b].drcCharacteristicRightIndex = 1;
            }
        }
    }

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

    uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
    /***********  drcInstructionsUniDrcV1  *************/
    uniDrcConfigExt->drcInstructionsUniDrcV1Count = 1;
    n=0;
    {
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetId = 1;
     /* uniDrcConfigExt->drcInstructionsUniDrcV1[n].drcSetComplexityLevel = 4; will be computed */
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
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=3;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;
        ch=4;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainSetIndex[ch] = 0;

        uniDrcConfigExt->drcInstructionsUniDrcV1[n].nDrcChannelGroups = 1;
        g=0;
        {
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterPresent = 1;
            uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].shapeFilterIndex = 1;
            b=0;
            {
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftPresent[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicLeftIndex[b] = 1;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].targetCharacteristicRightIndex[b] = 0;

                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainScalingPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].attenuationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].amplificationScaling[b] = 1.0f;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffsetPresent[b] = 0;
                uniDrcConfigExt->drcInstructionsUniDrcV1[n].gainModifiers[g].gainOffset[b] = 0.0f;
            }
        }
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTargetPresent = 0;
        uniDrcConfigExt->drcInstructionsUniDrcV1[n].limiterPeakTarget = 0.0f;
    }
    
    
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
    eqCoefficients->eqDelayMax        = 8;
    eqCoefficients->uniqueFilterBlockCount = 4;
    b=0;
    {
        eqCoefficients->filterBlock[b].filterElementCount = 1;
        e=0;
        {
            eqCoefficients->filterBlock[b].filterElement[e].filterElementIndex = 0;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGainPresent = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGain = -45.0f;
        }
    }
    b=1;
    {
        eqCoefficients->filterBlock[b].filterElementCount = 1;
        e=0;
        {
            eqCoefficients->filterBlock[b].filterElement[e].filterElementIndex = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGainPresent = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGain = -18.0f;
        }
    }
    b=2;
    {
        eqCoefficients->filterBlock[b].filterElementCount = 2;
        e=0;
        {
            eqCoefficients->filterBlock[b].filterElement[e].filterElementIndex = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGainPresent = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGain = -35.0f;
        }
        e=1;
        {
            eqCoefficients->filterBlock[b].filterElement[e].filterElementIndex = 2;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGainPresent = 0;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGain = 0.0f;
        }
    }
    b=3;
    {
        eqCoefficients->filterBlock[b].filterElementCount = 1;
        e=0;
        {
            eqCoefficients->filterBlock[b].filterElement[e].filterElementIndex = 3;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGainPresent = 1;
            eqCoefficients->filterBlock[b].filterElement[e].filterElementGain = -20.0f;
        }
    }
    
    
    
    eqCoefficients->uniqueTdFilterElementCount = 4;
    e=0;
    {
        eqCoefficients->uniqueTdFilterElement[e].eqFilterFormat = 0;
        eqCoefficients->uniqueTdFilterElement[e].realZeroRadiusOneCount   = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[0]    = 1;
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[1]    = 0;
        }
        eqCoefficients->uniqueTdFilterElement[e].realZeroCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realZeroRadius[0]    = -0.5f;
        }
        eqCoefficients->uniqueTdFilterElement[e].genericZeroCount       = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].genericZeroRadius[0] = 0.7f;
            eqCoefficients->uniqueTdFilterElement[e].genericZeroAngle[0]  = 0.25f;
        }
        eqCoefficients->uniqueTdFilterElement[e].realPoleCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realPoleRadius[0]    = -0.2f;
        }
        eqCoefficients->uniqueTdFilterElement[e].complexPoleCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].complexPoleRadius[0] = 0.85f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleAngle[0]  = 0.15f;
        }
        eqCoefficients->uniqueTdFilterElement[e].firFilterOrder = 0;
        eqCoefficients->uniqueTdFilterElement[e].firSymmetry = 0;
        eqCoefficients->uniqueTdFilterElement[e].firCoefficient[0] = 0.0f;
    }
    e=1;
    {
        eqCoefficients->uniqueTdFilterElement[e].eqFilterFormat = 0;
        eqCoefficients->uniqueTdFilterElement[e].realZeroRadiusOneCount   = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[0]    = 1;
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[1]    = 0;
        }
        eqCoefficients->uniqueTdFilterElement[e].realZeroCount          = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].realZeroRadius[0]    = 0.5f;
        }
        eqCoefficients->uniqueTdFilterElement[e].genericZeroCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].genericZeroRadius[0] = 0.5f;
            eqCoefficients->uniqueTdFilterElement[e].genericZeroAngle[0]  = 0.85f;
        }
        eqCoefficients->uniqueTdFilterElement[e].realPoleCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realPoleRadius[0]    = -0.3f;
        }
        eqCoefficients->uniqueTdFilterElement[e].complexPoleCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].complexPoleRadius[0] = 0.65f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleAngle[0]  = 0.75f;
        }
        eqCoefficients->uniqueTdFilterElement[e].firFilterOrder = 0;
        eqCoefficients->uniqueTdFilterElement[e].firSymmetry = 0;
        eqCoefficients->uniqueTdFilterElement[e].firCoefficient[0] = 0.0f;
    }
    e=2;
    {
        eqCoefficients->uniqueTdFilterElement[e].eqFilterFormat = 0;
        eqCoefficients->uniqueTdFilterElement[e].realZeroRadiusOneCount   = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[0]    = 1;
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[1]    = 0;
        }
        eqCoefficients->uniqueTdFilterElement[e].realZeroCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realZeroRadius[0]    = 0.5f;
        }
        eqCoefficients->uniqueTdFilterElement[e].genericZeroCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].genericZeroRadius[0] = 0.55f;
            eqCoefficients->uniqueTdFilterElement[e].genericZeroAngle[0]  = 0.15f;
        }
        eqCoefficients->uniqueTdFilterElement[e].realPoleCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realPoleRadius[0]    = 0.85f;
        }
        eqCoefficients->uniqueTdFilterElement[e].complexPoleCount       = 2;
        {
            eqCoefficients->uniqueTdFilterElement[e].complexPoleRadius[0] = 0.9f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleAngle[0]  = 0.25f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleRadius[1] = 0.6f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleAngle[1]  = 0.15f;
        }
        eqCoefficients->uniqueTdFilterElement[e].firFilterOrder = 0;
        eqCoefficients->uniqueTdFilterElement[e].firSymmetry = 0;
        eqCoefficients->uniqueTdFilterElement[e].firCoefficient[0] = 0.0f;
    }
    e=3;
    {
        eqCoefficients->uniqueTdFilterElement[e].eqFilterFormat = 0;
        eqCoefficients->uniqueTdFilterElement[e].realZeroRadiusOneCount   = 0;
        {
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[0]    = 1;
            eqCoefficients->uniqueTdFilterElement[e].zeroSign[1]    = 0;
        }
        eqCoefficients->uniqueTdFilterElement[e].realZeroCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realZeroRadius[0]    = -0.5f;
        }
        eqCoefficients->uniqueTdFilterElement[e].genericZeroCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].genericZeroRadius[0] = 0.65f;
            eqCoefficients->uniqueTdFilterElement[e].genericZeroAngle[0]  = 0.05f;
        }
        eqCoefficients->uniqueTdFilterElement[e].realPoleCount          = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].realPoleRadius[0]    = 0.85f;
        }
        eqCoefficients->uniqueTdFilterElement[e].complexPoleCount       = 1;
        {
            eqCoefficients->uniqueTdFilterElement[e].complexPoleRadius[0] = 0.9f;
            eqCoefficients->uniqueTdFilterElement[e].complexPoleAngle[0]  = 0.75f;
        }
        eqCoefficients->uniqueTdFilterElement[e].firFilterOrder = 0;
        eqCoefficients->uniqueTdFilterElement[e].firSymmetry = 0;
        eqCoefficients->uniqueTdFilterElement[e].firCoefficient[0] = 0.0f;
    }
    
    eqCoefficients->uniqueEqSubbandGainsCount = 1;
    eqCoefficients->eqSubbandGainRepresentation = 1;
    eqCoefficients->eqSubbandGainFormat = GAINFORMAT_UNIFORM;
    eqCoefficients->eqSubbandGainCount = 64;
    
    s=0;
    {
        EqSubbandGainSpline* spline = &eqCoefficients->eqSubbandGainSpline[s];
        spline->eqGainInitial = 10.0f;
        spline->eqSlope[0] = -1.0f;
        n=1;
        {
            spline->eqSlope[n] = -1.0f;
            spline->eqGainDelta[n] = -1.0f;
            spline->eqFreqDelta[n] = 10;
        }
        n++;
        {
            spline->eqSlope[n] = -1.0f;
            spline->eqGainDelta[n] = 5.0f;
            spline->eqFreqDelta[n] = 10;
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
            spline->eqGainDelta[n] = -5.0f;
            spline->eqFreqDelta[n] = 1;
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
        /* eqInstructions->eqSetComplexityLevel = 7;  value will be computed */
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
        eqInstructions->tdFilterCascadePresent = 1;
        g=0;
        {
            eqInstructions->tdFilterCascade.eqCascadeGainPresent[g] = 1;
            eqInstructions->tdFilterCascade.eqCascadeGain[g] = -4;
            eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockCount = 1;
            {
                eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockIndex[0] = 0;
            }
        }
        g=1;
        {
            eqInstructions->tdFilterCascade.eqCascadeGainPresent[g] = 1;
            eqInstructions->tdFilterCascade.eqCascadeGain[g] = -6;
            eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockCount = 1;
            {
                eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockIndex[0] = 1;
            }
        }
        g=2;
        {
            eqInstructions->tdFilterCascade.eqCascadeGainPresent[g] = 1;
            eqInstructions->tdFilterCascade.eqCascadeGain[g] = -50;
            eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockCount = 2;
            {
                eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockIndex[0] = 1;
                eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockIndex[1] = 2;
            }
        }
        g=3;
        {
            eqInstructions->tdFilterCascade.eqCascadeGainPresent[g] = 0;
            eqInstructions->tdFilterCascade.eqCascadeGain[g] = 0;
            eqInstructions->tdFilterCascade.filterBlockRefs[g].filterBlockCount = 0;
            {
            }
        }
        eqInstructions->tdFilterCascade.eqPhaseAlignmentPresent = 1;
        
        eqInstructions->tdFilterCascade.eqPhaseAlignment[0][1] = 1;
        eqInstructions->tdFilterCascade.eqPhaseAlignment[0][2] = 1;
        eqInstructions->tdFilterCascade.eqPhaseAlignment[0][3] = 1;
        eqInstructions->tdFilterCascade.eqPhaseAlignment[1][2] = 1;
        eqInstructions->tdFilterCascade.eqPhaseAlignment[1][3] = 1;
        eqInstructions->tdFilterCascade.eqPhaseAlignment[2][3] = 1;
        eqInstructions->subbandGainsPresent = 0;
        g=0;
        {
            eqInstructions->subbandGainsIndex[g] = 0;
        }
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