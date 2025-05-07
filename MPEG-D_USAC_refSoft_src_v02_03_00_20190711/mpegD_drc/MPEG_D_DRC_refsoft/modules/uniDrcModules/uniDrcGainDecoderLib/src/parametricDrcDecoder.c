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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "uniDrcGainDecoder.h"
#include "parametricDrcDecoder.h"

#if AMD1_SYNTAX

#define PI 3.14159265f

#ifndef max
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#endif

/***********************************************************************************/
/* init functions */
/***********************************************************************************/

int
initParametricDrc(const int                      drcFrameSize,
                  const int                        sampleRate,
                  int                       subBandDomainMode,
                  PARAMETRIC_DRC_PARAMS *hParametricDrcParams)
{
    
    hParametricDrcParams->drcFrameSize              = drcFrameSize;
    hParametricDrcParams->sampleRate                = sampleRate;
    hParametricDrcParams->subBandDomainMode         = subBandDomainMode;
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            hParametricDrcParams->subBandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_OFF:
            hParametricDrcParams->subBandCount              = 0;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            hParametricDrcParams->subBandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
#if DEBUG_WARNINGS
            fprintf(stderr, "WARNING: subBandDomainMode QMF71 currently not fully supported in parametric DRC tool.\n");
#endif
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            hParametricDrcParams->subBandCount              = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
#if DEBUG_WARNINGS
            fprintf(stderr, "WARNING: subBandDomainMode STFT256 currently not fully supported in parametric DRC tool.\n");
#endif
            break;
        default:
            fprintf(stderr, "WARNING: subBandDomainMode = %d currently not fully supported in parametric DRC tool.\n",subBandDomainMode);
            break;
    }
    
    return 0;
}

int
initParametricDrcAfterConfig(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                             HANDLE_LOUDNESS_INFO_SET   hLoudnessInfoSet,
                             PARAMETRIC_DRC_PARAMS *hParametricDrcParams)
{
    
    int err = 0, instanceIndex = 0, gainSetIndex = 0, sideChainConfigType = 0, downmixId = 0, channelCountFromDownmixId = 0, L = 0;
    
    hParametricDrcParams->drcFrameSizeParametricDrc = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSize;
    hParametricDrcParams->resetParametricDrc        = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.resetParametricDrc;
    hParametricDrcParams->nNodes                    = hParametricDrcParams->drcFrameSize/hParametricDrcParams->drcFrameSizeParametricDrc;

    switch (hParametricDrcParams->subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
            break;
        case SUBBAND_DOMAIN_MODE_OFF:
        default:
            L = 0;
            break;
    }
    if (hParametricDrcParams->subBandDomainMode != SUBBAND_DOMAIN_MODE_OFF && hParametricDrcParams->drcFrameSizeParametricDrc != L) {
        fprintf(stderr, "ERROR: drcFrameSizeParametricDrc (%d) != AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR (%d).\n",hParametricDrcParams->drcFrameSizeParametricDrc,L);
        return (EXTERNAL_ERROR);
    }
    
    for (instanceIndex=0; instanceIndex<hParametricDrcParams->parametricDrcInstanceCount; instanceIndex++) {
        
        gainSetIndex        = hParametricDrcParams->gainSetIndex[instanceIndex];
        sideChainConfigType = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].sideChainConfigType;
        downmixId           = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].downmixId;
        
        if (sideChainConfigType==1 && downmixId == hParametricDrcParams->downmixIdFromDrcInstructions[instanceIndex]) {
            channelCountFromDownmixId = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].channelCountFromDownmixId;
        } else {
            channelCountFromDownmixId = 0;
        }
               
        if (hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].drcInputLoudnessPresent == 0) {
            int n = 0, m = 0, drcInputLoudnessFound = 0;
            float drcInputLoudness = 0.f;

            /* TODO: add group support for MPEG-H 3DA */
            /* TODO: extend selection strategy */
            for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
            {
                LoudnessInfo* loudnessInfo = &hLoudnessInfoSet->loudnessInfo[n];
                if (hParametricDrcParams->downmixIdFromDrcInstructions[instanceIndex] == loudnessInfo->downmixId)
                {
                    if (0 == loudnessInfo->drcSetId)
                    {
                        for (m=0; m<loudnessInfo->measurementCount; m++)
                        {
                            if (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_PROGRAM_LOUDNESS)
                            {
                                drcInputLoudness = loudnessInfo->loudnessMeasure[m].methodValue;
                                drcInputLoudnessFound = 1;
                                break;
                            }
                        }
                        if (drcInputLoudnessFound == 0) {
                            for (m=0; m<loudnessInfo->measurementCount; m++)
                            {
                                if (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_ANCHOR_LOUDNESS)
                                {
                                    drcInputLoudness = loudnessInfo->loudnessMeasure[m].methodValue;
                                    drcInputLoudnessFound = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (drcInputLoudnessFound == 0)
            {
                for (n=0; n<hLoudnessInfoSet->loudnessInfoCount; n++)
                {
                    LoudnessInfo* loudnessInfo = &hLoudnessInfoSet->loudnessInfo[n];
                    if (0 == loudnessInfo->downmixId)
                    {
                        if (0 == loudnessInfo->drcSetId)
                        {
                            for (m=0; m<loudnessInfo->measurementCount; m++)
                            {
                                if (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_PROGRAM_LOUDNESS)
                                {
                                    drcInputLoudness = loudnessInfo->loudnessMeasure[m].methodValue;
                                    drcInputLoudnessFound = 1;
                                    break;
                                }
                            }
                            if (drcInputLoudnessFound == 0) {
                                for (m=0; m<loudnessInfo->measurementCount; m++)
                                {
                                    if (loudnessInfo->loudnessMeasure[m].methodDefinition == METHOD_DEFINITION_ANCHOR_LOUDNESS)
                                    {
                                        drcInputLoudness = loudnessInfo->loudnessMeasure[m].methodValue;
                                        drcInputLoudnessFound = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (drcInputLoudnessFound == 0) {
                fprintf(stderr, "ERROR: drcInputLoudness not found in loudnessInfoSet.\n");
                return (UNEXPECTED_ERROR);
            } else {
                hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex].drcInputLoudness = drcInputLoudness;
            }
        }
        
        initParametricDrcInstance(hUniDrcConfig,
                                  instanceIndex,
                                  channelCountFromDownmixId,
                                  hParametricDrcParams);
        if (err) return (err);
    }
    
    return 0;
}

int
initParametricDrcInstance(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                          const int                     instanceIndex,
                          const int         channelCountFromDownmixId,
                          PARAMETRIC_DRC_PARAMS* hParametricDrcParams)
{
    int err = 0;
    
    int parametricDrcIndex = hParametricDrcParams->parametricDrcIndex[instanceIndex];
    ParametricDrcInstructions *hParametricDrcInstructions       = &(hUniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[parametricDrcIndex]);

    hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].disableParamtricDrc    = hParametricDrcInstructions->disableParamtricDrc;
    hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType      = hParametricDrcInstructions->parametricDrcType;
    hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].splineNodes.nNodes     = hParametricDrcParams->nNodes;
    
    if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].disableParamtricDrc == 0) {
        
        if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType == PARAM_DRC_TYPE_FF) {
            
            err = initParametricDrcTypeFeedForward(hUniDrcConfig,
                                                   instanceIndex,
                                                   channelCountFromDownmixId,
                                                   hParametricDrcParams);
            if (err) return (err);
            
#ifdef AMD1_PARAMETRIC_LIMITER
        } else if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType == PARAM_DRC_TYPE_LIM) {
            
            hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].splineNodes.nNodes = hParametricDrcParams->drcFrameSize;
            
            err = initParametricDrcTypeLim(hUniDrcConfig,
                                           instanceIndex,
                                           channelCountFromDownmixId,
                                           hParametricDrcParams);
            if (err) return (err);
#endif
            
        } else {
            
            fprintf(stderr, "ERROR: Unknown parametricDrcType = %d.\n",hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType);
            return (UNEXPECTED_ERROR);
            
        }
    }

    return 0;
}

int
initParametricDrcTypeFeedForward(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                                 const int                     instanceIndex,
                                 const int         channelCountFromDownmixId,
                                 PARAMETRIC_DRC_PARAMS* hParametricDrcParams)
{
    int err = 0, i = 0;
    
    int parametricDrcIndex = hParametricDrcParams->parametricDrcIndex[instanceIndex];
    int gainSetIndex    = hParametricDrcParams->gainSetIndex[instanceIndex];
    int* channelMap     = hParametricDrcParams->channelMap[instanceIndex];
    
    DrcCoefficientsParametricDrc* hDrcCoefficientsParametricDrcBs = &(hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc);
    ParametricDrcTypeFeedForward* hParametricDrcTypeFeedForwardBs = &(hUniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[parametricDrcIndex].parametricDrcTypeFeedForward);
    PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams = &(hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcTypeFeedForwardParams);
    
    /* level estimation */
    hParametricDrcTypeFeedForwardParams->frameSize                 = hParametricDrcParams->drcFrameSizeParametricDrc;
    hParametricDrcTypeFeedForwardParams->subBandDomainMode         = hParametricDrcParams->subBandDomainMode;
    hParametricDrcTypeFeedForwardParams->subBandCount              = hParametricDrcParams->subBandCount;
    hParametricDrcTypeFeedForwardParams->subBandCompensationType   = 0;
    
    if (hParametricDrcTypeFeedForwardParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_QMF64) {
        if (hParametricDrcParams->sampleRate == 48000) {
            hParametricDrcTypeFeedForwardParams->subBandCompensationType = 1;
        } else {
            /* support of other sampling rates than 48000 might be missing */
            return UNEXPECTED_ERROR;
        }
    }

    hParametricDrcTypeFeedForwardParams->audioChannelCount         = hParametricDrcParams->audioChannelCount;
    hParametricDrcTypeFeedForwardParams->levelEstimKWeightingType  = hParametricDrcTypeFeedForwardBs->levelEstimKWeightingType;
    hParametricDrcTypeFeedForwardParams->levelEstimIntegrationTime = hParametricDrcTypeFeedForwardBs->levelEstimIntegrationTime;
    hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex      = 0;
    hParametricDrcTypeFeedForwardParams->levelEstimFrameCount      = hParametricDrcTypeFeedForwardBs->levelEstimIntegrationTime/hParametricDrcTypeFeedForwardParams->frameSize;
    for (i=0; i<PARAM_DRC_TYPE_FF_LEVEL_ESTIM_FRAME_COUNT_MAX; i++) {
        hParametricDrcTypeFeedForwardParams->level[i] = 0.f;
    }
    if (channelCountFromDownmixId != 0) {
        for (i=0; i<channelCountFromDownmixId; i++) {
            hParametricDrcTypeFeedForwardParams->levelEstimChannelWeight[i] = hDrcCoefficientsParametricDrcBs->parametricDrcGainSetParams[gainSetIndex].levelEstimChannelWeight[i];
        }
    } else {
        if (hDrcCoefficientsParametricDrcBs->parametricDrcGainSetParams[gainSetIndex].sideChainConfigType == 2) {
            for (i=0; i<hParametricDrcTypeFeedForwardParams->audioChannelCount; i++) {
                hParametricDrcTypeFeedForwardParams->levelEstimChannelWeight[i] = (float)channelMap[i]; /* TODO: add BS.1770-3 weights here */
            }
        } else {
            for (i=0; i<hParametricDrcTypeFeedForwardParams->audioChannelCount; i++) {
                hParametricDrcTypeFeedForwardParams->levelEstimChannelWeight[i] = (float)channelMap[i];
            }
        }
    }

    if (hParametricDrcTypeFeedForwardParams->subBandDomainMode==SUBBAND_DOMAIN_MODE_OFF) {
        /* time domain filters */
        err = initLvlEstFilterTime(hParametricDrcTypeFeedForwardParams->levelEstimKWeightingType,
                                   hParametricDrcParams->sampleRate,
                                   &hParametricDrcTypeFeedForwardParams->preFilterCoeff,
                                   &hParametricDrcTypeFeedForwardParams->rlbFilterCoeff);
        if (err) return (err);    
    } else {
        /* subband domain filters */
        err = initLvlEstFilterSubBand(hParametricDrcTypeFeedForwardParams->levelEstimKWeightingType,
                                      hParametricDrcParams->sampleRate,
                                      hParametricDrcParams->subBandDomainMode,
                                      hParametricDrcParams->subBandCount,
                                      hParametricDrcTypeFeedForwardParams->subBandCompensationType,
                                      hParametricDrcTypeFeedForwardParams->weightingFilter,
                                      &hParametricDrcTypeFeedForwardParams->filterCoeffSubBand);
        if (err) return (err);
    }
    
    /* gain mapping */
    hParametricDrcTypeFeedForwardParams->nodeCount = hParametricDrcTypeFeedForwardBs->nodeCount;
    for (i=0; i<hParametricDrcTypeFeedForwardParams->nodeCount; i++) {
        hParametricDrcTypeFeedForwardParams->nodeLevel[i] = hParametricDrcTypeFeedForwardBs->nodeLevel[i];
        hParametricDrcTypeFeedForwardParams->nodeGain[i] = hParametricDrcTypeFeedForwardBs->nodeGain[i];
    }
    hParametricDrcTypeFeedForwardParams->referenceLevelParametricDrc = hDrcCoefficientsParametricDrcBs->parametricDrcGainSetParams[gainSetIndex].drcInputLoudness;
    
    /* gain smoothing */
    {
        int gainSmoothAttackTimeFast  = hParametricDrcTypeFeedForwardBs->gainSmoothAttackTimeFast;
        int gainSmoothReleaseTimeFast = hParametricDrcTypeFeedForwardBs->gainSmoothReleaseTimeFast;
        int gainSmoothAttackTimeSlow  = hParametricDrcTypeFeedForwardBs->gainSmoothAttackTimeSlow;
        int gainSmoothReleaseTimeSlow = hParametricDrcTypeFeedForwardBs->gainSmoothReleaseTimeSlow;
        int gainSmoothHoldOff         = hParametricDrcTypeFeedForwardBs->gainSmoothHoldOff;
        int sampleRate                = hParametricDrcParams->sampleRate;
        int drcFrameSizeParametricDrc = hParametricDrcParams->drcFrameSizeParametricDrc;
        
        hParametricDrcTypeFeedForwardParams->gainSmoothAttackAlphaFast  = 1 - (float)exp(-1.0 * drcFrameSizeParametricDrc / (gainSmoothAttackTimeFast * sampleRate * 0.001));
        hParametricDrcTypeFeedForwardParams->gainSmoothReleaseAlphaFast = 1 - (float)exp(-1.0 * drcFrameSizeParametricDrc / (gainSmoothReleaseTimeFast * sampleRate * 0.001));
        hParametricDrcTypeFeedForwardParams->gainSmoothAttackAlphaSlow  = 1 - (float)exp(-1.0 * drcFrameSizeParametricDrc / (gainSmoothAttackTimeSlow * sampleRate * 0.001));
        hParametricDrcTypeFeedForwardParams->gainSmoothReleaseAlphaSlow = 1 - (float)exp(-1.0 * drcFrameSizeParametricDrc / (gainSmoothReleaseTimeSlow * sampleRate * 0.001));
        hParametricDrcTypeFeedForwardParams->gainSmoothHoldOffCount     = gainSmoothHoldOff * 256 * sampleRate / (drcFrameSizeParametricDrc * 48000);
        hParametricDrcTypeFeedForwardParams->gainSmoothAttackThreshold  = hParametricDrcTypeFeedForwardBs->gainSmoothAttackThreshold;
        hParametricDrcTypeFeedForwardParams->gainSmoothReleaseThreshold = hParametricDrcTypeFeedForwardBs->gainSmoothReleaseThreshold;
    }
    
    /* reset */
    err = resetParametricDrcTypeFeedForward(instanceIndex,
                                            hParametricDrcTypeFeedForwardParams);
    if (err) return (err);
    
    return 0;
}

int
initLvlEstFilterTime(const int     levelEstimKWeightingType,
                     const int                   sampleRate,
                     FILTER_COEFF_2ND_ORDER* preFilterCoeff,
                     FILTER_COEFF_2ND_ORDER* rlbFilterCoeff)
{
    float w0, A, alpha, sinw0, cosw0, sqrtA;
    float b0, b1, b2, a0, a1, a2;
    
    if (levelEstimKWeightingType==2) {
        
        /* pre-filter (1500Hz, +4dB high shelf) */
        w0 = 2.0f*PI * 1500.0f / (float)sampleRate;
        sinw0 = (float)sin(w0);
        cosw0 = (float)cos(w0);
        A = 1.25892541f;    /* sqrt(10^(4/20)) */
        sqrtA = (float)sqrt(A);
        alpha = (float)(sinw0 * 0.5*sqrt(2));
        
        b0 =    A*( (A+1) + (A-1)*cosw0 + 2*sqrtA*alpha );
        b1 = -2*A*( (A-1) + (A+1)*cosw0 );
        b2 =    A*( (A+1) + (A-1)*cosw0 - 2*sqrtA*alpha );
        a0 =        (A+1) - (A-1)*cosw0 + 2*sqrtA*alpha;
        a1 =    2*( (A-1) - (A+1)*cosw0 );
        a2 =        (A+1) - (A-1)*cosw0 - 2*sqrtA*alpha;
        
        preFilterCoeff->b0 = b0/a0;
        preFilterCoeff->b1 = b1/a0;
        preFilterCoeff->b2 = b2/a0;
        preFilterCoeff->a1 = a1/a0;
        preFilterCoeff->a2 = a2/a0;
        
    }
    
    if (levelEstimKWeightingType == 1 || levelEstimKWeightingType == 2) {
        
        /* RLB filter (38Hz high pass, Q=0.5) */
        w0 = 2.0f*PI * 38.0f / (float)sampleRate;
        sinw0 = (float)sin(w0);
        cosw0 = (float)cos(w0);
        alpha = sinw0;
        
        b0 =  (1 + cosw0)/2;
        b1 = -(1 + cosw0);
        b2 =  (1 + cosw0)/2;
        a0 =   1 + alpha;
        a1 =  -2*cosw0;
        a2 =   1 - alpha;
        
        rlbFilterCoeff->b0 = b0/a0;
        rlbFilterCoeff->b1 = b1/a0;
        rlbFilterCoeff->b2 = b2/a0;
        rlbFilterCoeff->a1 = a1/a0;
        rlbFilterCoeff->a2 = a2/a0;
        
    }
    
    return 0;
}

int
initLvlEstFilterSubBand(const int         levelEstimKWeightingType,
                        const int                       sampleRate,
                        const int                subBandDomainMode,
                        const int                     subBandCount,
                        const int          subBandCompensationType,
                        float                     *weightingFilter,
                        FILTER_COEFF_2ND_ORDER* filterCoeffSubBand)
{
    float w0, A, alpha, sinw0, cosw0, sqrtA;
    float b0, b1, b2, a0, a1, a2;
    float numReal,numImag,denReal,denImag;
    float *f_bands_nrm;
    int b;
    
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            f_bands_nrm = f_bands_nrm_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            f_bands_nrm = f_bands_nrm_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            f_bands_nrm = f_bands_nrm_STFT256;
            break;
        default:
            fprintf(stderr, "ERROR: Center frequencies missing for subBandDomainMode = %d.\n",subBandDomainMode);
            return UNEXPECTED_ERROR;
            break;
    }
    
    /* init weighting filter */
    for (b=0; b<subBandCount; b++) {
        weightingFilter[b] = 1.f;
    }

    if (levelEstimKWeightingType == 2) {
        
        /* pre-filter (1500Hz, +4dB high shelf) */
        w0 = 2.0f*PI * 1500.0f / (float)sampleRate;
        sinw0 = (float)sin(w0);
        cosw0 = (float)cos(w0);
        A = 1.25892541f;    /* sqrt(10^(4/20)) */
        sqrtA = (float)sqrt(A);
        alpha = (float)(sinw0 * 0.5*sqrt(2));
        
        b0 =    A*( (A+1) + (A-1)*cosw0 + 2*sqrtA*alpha );
        b1 = -2*A*( (A-1) + (A+1)*cosw0 );
        b2 =    A*( (A+1) + (A-1)*cosw0 - 2*sqrtA*alpha );
        a0 =        (A+1) - (A-1)*cosw0 + 2*sqrtA*alpha;
        a1 =    2*( (A-1) - (A+1)*cosw0 );
        a2 =        (A+1) - (A-1)*cosw0 - 2*sqrtA*alpha;
        
        b0 = b0/a0;
        b1 = b1/a0;
        b2 = b2/a0;
        a1 = a1/a0;
        a2 = a2/a0;
        a0 = 1.f;
        
        for (b=0; b<subBandCount; b++) {
            
            /* computation for equidistant subbands
             numReal = b0 + b1 * cos(PI*(b+0.5f)*0.0156f) + b2 * cos(PI*(b+0.5f)*0.0312f);
             numImag = - b1 * sin(PI*(b+0.5f)*0.0156f) - b2 * sin(PI*(b+0.5f)*0.0312f);
             denReal = a0 + a1 * cos(PI*(b+0.5f)*0.0156f) + a2 * cos(PI*(b+0.5f)*0.0312f);
             denImag = - a1 * sin(PI*(b+0.5f)*0.0156f) - a2 * sin(PI*(b+0.5f)*0.0312f); */
            
            numReal = b0 + b1 * cos(PI*f_bands_nrm[b]) + b2 * cos(PI*2*f_bands_nrm[b]);
            numImag = - b1 * sin(PI*f_bands_nrm[b]) - b2 * sin(PI*2*f_bands_nrm[b]);
            denReal = a0 + a1 * cos(PI*f_bands_nrm[b]) + a2 * cos(PI*2*f_bands_nrm[b]);
            denImag = - a1 * sin(PI*f_bands_nrm[b]) - a2 * sin(PI*2*f_bands_nrm[b]);
            
            weightingFilter[b] *= sqrt((numReal*numReal+numImag*numImag)/(denReal*denReal+denImag*denImag));
        }
    }
    
    if (levelEstimKWeightingType == 1 || levelEstimKWeightingType == 2) {
        
        /* RLB filter (38Hz high pass, Q=0.5) */
        w0 = 2.0f*PI * 38.0f / (float)sampleRate;
        sinw0 = (float)sin(w0);
        cosw0 = (float)cos(w0);
        alpha = sinw0;
        
        b0 =  (1 + cosw0)/2;
        b1 = -(1 + cosw0);
        b2 =  (1 + cosw0)/2;
        a0 =   1 + alpha;
        a1 =  -2*cosw0;
        a2 =   1 - alpha;
        
        b0 = b0/a0;
        b1 = b1/a0;
        b2 = b2/a0;
        a1 = a1/a0;
        a2 = a2/a0;
        a0 = 1.f;
        
        for (b=0; b<subBandCount; b++) {
            
            if (!(subBandCompensationType==1 && b==0)) {
                numReal = b0 + b1 * cos(PI*f_bands_nrm[b]) + b2 * cos(PI*2*f_bands_nrm[b]);
                numImag = - b1 * sin(PI*f_bands_nrm[b]) - b2 * sin(PI*2*f_bands_nrm[b]);
                denReal = a0 + a1 * cos(PI*f_bands_nrm[b]) + a2 * cos(PI*2*f_bands_nrm[b]);
                denImag = - a1 * sin(PI*f_bands_nrm[b]) - a2 * sin(PI*2*f_bands_nrm[b]);
                
                weightingFilter[b] *= sqrt((numReal*numReal+numImag*numImag)/(denReal*denReal+denImag*denImag));
            }
        }
        
        if (subBandCompensationType==1) { /* QMF64 RLB compensation filter */
            
            /* RLB filter (38Hz high pass, Q=0.5) */
            w0 = 2.0f*PI * 38.0f / (float)sampleRate * AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
            sinw0 = (float)sin(w0);
            cosw0 = (float)cos(w0);
            alpha = sinw0;
            
            b0 =  (1 + cosw0)/2;
            b1 = -(1 + cosw0);
            b2 =  (1 + cosw0)/2;
            a0 =   1 + alpha;
            a1 =  -2*cosw0;
            a2 =   1 - alpha;
            
            filterCoeffSubBand->b0 = b0/a0;
            filterCoeffSubBand->b1 = b1/a0;
            filterCoeffSubBand->b2 = b2/a0;
            filterCoeffSubBand->a1 = a1/a0;
            filterCoeffSubBand->a2 = a2/a0;
        }
    }
    
    return 0;
}

#ifdef AMD1_PARAMETRIC_LIMITER
int
initParametricDrcTypeLim(HANDLE_UNI_DRC_CONFIG         hUniDrcConfig,
                         const int                     instanceIndex,
                         const int         channelCountFromDownmixId,
                         PARAMETRIC_DRC_PARAMS* hParametricDrcParams)
{
    int err = 0, i = 0;
    unsigned int attack, secLen;

    int parametricDrcIndex = hParametricDrcParams->parametricDrcIndex[instanceIndex];
    int gainSetIndex    = hParametricDrcParams->gainSetIndex[instanceIndex];
    int* channelMap     = hParametricDrcParams->channelMap[instanceIndex];
    
    DrcCoefficientsParametricDrc* hDrcCoefficientsParametricDrcBs = &(hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc);
    ParametricDrcTypeLim* hParametricDrcTypeLimBs = &(hUniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[parametricDrcIndex].parametricDrcTypeLim);
    PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams = &(hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcTypeLimParams);
    
    hParametricDrcTypeLimParams->frameSize                 = hParametricDrcParams->drcFrameSize;
    hParametricDrcTypeLimParams->audioChannelCount         = hParametricDrcParams->audioChannelCount;
    
    if (channelCountFromDownmixId != 0) {
        for (i=0; i<channelCountFromDownmixId; i++) {
            hParametricDrcTypeLimParams->levelEstimChannelWeight[i] = hDrcCoefficientsParametricDrcBs->parametricDrcGainSetParams[gainSetIndex].levelEstimChannelWeight[i];
        }
    } else {
        if (hDrcCoefficientsParametricDrcBs->parametricDrcGainSetParams[gainSetIndex].sideChainConfigType == 2) {
            for (i=0; i<hParametricDrcTypeLimParams->audioChannelCount; i++) {
                hParametricDrcTypeLimParams->levelEstimChannelWeight[i] = (float)channelMap[i]; /* TODO: add BS.1770-3 weights here */
            }
        } else {
            for (i=0; i<hParametricDrcTypeLimParams->audioChannelCount; i++) {
                hParametricDrcTypeLimParams->levelEstimChannelWeight[i] = (float)channelMap[i];
            }
        }
    }
    
    /* calc attack time in samples */
    attack = (unsigned int)(hParametricDrcTypeLimBs->parametricLimAttack * hParametricDrcParams->sampleRate / 1000);
    
    /* length of maxBuf sections */
    secLen = (unsigned int)sqrt(attack+1);
    
    hParametricDrcTypeLimParams->secLen = secLen;
    hParametricDrcTypeLimParams->nMaxBufSec = (attack+1)/secLen;
    if (hParametricDrcTypeLimParams->nMaxBufSec*secLen < (attack+1))
        hParametricDrcTypeLimParams->nMaxBufSec++; /* create a full section for the last samples */
    
    /* alloc maximum buffers */
    hParametricDrcTypeLimParams->maxBuf     = (float*)calloc(hParametricDrcTypeLimParams->nMaxBufSec * secLen, sizeof(float));
    hParametricDrcTypeLimParams->maxBufSlow = (float*)calloc(hParametricDrcTypeLimParams->nMaxBufSec, sizeof(float));
    
    /* init parameters & states */
    hParametricDrcTypeLimParams->maxBufIdx = 0;
    hParametricDrcTypeLimParams->maxBufSlowIdx = 0;
    hParametricDrcTypeLimParams->maxBufSecIdx = 0;
    hParametricDrcTypeLimParams->maxBufSecCtr = 0;
    
    hParametricDrcTypeLimParams->attackMs      = hParametricDrcTypeLimBs->parametricLimAttack;
    hParametricDrcTypeLimParams->releaseMs     = hParametricDrcTypeLimBs->parametricLimRelease;
    hParametricDrcTypeLimParams->attack        = attack;
    hParametricDrcTypeLimParams->attackConst   = (float)pow(0.1, 1.0 / (attack + 1));
    hParametricDrcTypeLimParams->releaseConst  = (float)pow(0.1, 1.0 / (hParametricDrcTypeLimBs->parametricLimRelease * hParametricDrcParams->sampleRate / 1000 + 1));
    hParametricDrcTypeLimParams->threshold     = (float)pow(10.0f, 0.05f * hParametricDrcTypeLimBs->parametricLimThreshold);
    hParametricDrcTypeLimParams->channels      = hParametricDrcTypeLimParams->audioChannelCount;
    hParametricDrcTypeLimParams->sampleRate    = hParametricDrcParams->sampleRate;
    
    hParametricDrcTypeLimParams->cor = 1.0f;
    hParametricDrcTypeLimParams->smoothState0 = 1.0;
    
    /* reset */
    err = resetParametricDrcTypeLim(instanceIndex,
                                    hParametricDrcTypeLimParams);
    if (err) return (err);
    
    return 0;
}
#endif

/***********************************************************************************/
/* reset functions */
/***********************************************************************************/

int
resetParametricDrcInstance(const int                                      instanceIndex,
                           PARAMETRIC_DRC_INSTANCE_PARAMS* hParametricDrcInstanceParams)
{
    int err = 0;
    
    if (hParametricDrcInstanceParams->parametricDrcType == PARAM_DRC_TYPE_FF) {
        
        PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams = &(hParametricDrcInstanceParams->parametricDrcTypeFeedForwardParams);
        err = resetParametricDrcTypeFeedForward(instanceIndex,
                                                hParametricDrcTypeFeedForwardParams);
        if (err) return (err);
        
#ifdef AMD1_PARAMETRIC_LIMITER
    } else if (hParametricDrcInstanceParams->parametricDrcType == PARAM_DRC_TYPE_LIM) {
        
        PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams = &(hParametricDrcInstanceParams->parametricDrcTypeLimParams);
        err = resetParametricDrcTypeLim(instanceIndex,
                                        hParametricDrcTypeLimParams);
        if (err) return (err);
        
#endif
        
    } else {
        
        fprintf(stderr, "ERROR: Unknown parametricDrcType = %d.\n",hParametricDrcInstanceParams->parametricDrcType);
        return (UNEXPECTED_ERROR);
        
    }

    return 0;
}

int
resetParametricDrcTypeFeedForward(const int                                            instanceIndex,
                                  PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams)
{
    int i = 0;
    
    hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex      = 0;
    hParametricDrcTypeFeedForwardParams->startUpPhase              = 1;
    for (i=0; i<PARAM_DRC_TYPE_FF_LEVEL_ESTIM_FRAME_COUNT_MAX; i++) {
        hParametricDrcTypeFeedForwardParams->level[i] = 0.f;
    }
    
    for (i=0; i<CHANNEL_COUNT_MAX; i++) {
        hParametricDrcTypeFeedForwardParams->preFilterState[i].z1 = 0.f;
        hParametricDrcTypeFeedForwardParams->preFilterState[i].z2 = 0.f;
        hParametricDrcTypeFeedForwardParams->rlbFilterState[i].z1 = 0.f;
        hParametricDrcTypeFeedForwardParams->rlbFilterState[i].z2 = 0.f;
        hParametricDrcTypeFeedForwardParams->filterStateSubBandReal[i].z1 = 0.f;
        hParametricDrcTypeFeedForwardParams->filterStateSubBandReal[i].z2 = 0.f;
        hParametricDrcTypeFeedForwardParams->filterStateSubBandImag[i].z1 = 0.f;
        hParametricDrcTypeFeedForwardParams->filterStateSubBandImag[i].z2 = 0.f;
    }
    
    hParametricDrcTypeFeedForwardParams->levelSmoothDb = -135.f;
    hParametricDrcTypeFeedForwardParams->gainSmoothDb  = 0.f;
    hParametricDrcTypeFeedForwardParams->holdCounter   = 0;
    
    return 0;
}

#ifdef AMD1_PARAMETRIC_LIMITER
int
resetParametricDrcTypeLim(const int                                     instanceIndex,
                          PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams)
{
    
    int i;
    
    hParametricDrcTypeLimParams->maxBufIdx = 0;
    hParametricDrcTypeLimParams->maxBufSlowIdx = 0;
    hParametricDrcTypeLimParams->maxBufSecIdx = 0;
    hParametricDrcTypeLimParams->maxBufSecCtr = 0;
    
    hParametricDrcTypeLimParams->cor = 1.0f;
    hParametricDrcTypeLimParams->smoothState0 = 1.0;
    
    for (i=0; i<hParametricDrcTypeLimParams->nMaxBufSec * hParametricDrcTypeLimParams->secLen; i++) {
        hParametricDrcTypeLimParams->maxBuf[i] = 0.f;
    }
    for (i=0; i<hParametricDrcTypeLimParams->nMaxBufSec; i++) {
        hParametricDrcTypeLimParams->maxBufSlow[i] = 0.f;
    }
    
    return 0;
}
#endif

/***********************************************************************************/
/* process functions */
/***********************************************************************************/

int
processParametricDrcInstance (float*                                       audioIOBuffer[],
                              float*                                   audioIOBufferReal[],
                              float*                                   audioIOBufferImag[],
                              PARAMETRIC_DRC_PARAMS*                  hParametricDrcParams,
                              PARAMETRIC_DRC_INSTANCE_PARAMS* hParametricDrcInstanceParams)
{
    int err = 0, i = 0;
    
    if (hParametricDrcInstanceParams->disableParamtricDrc) {
        
        for (i=0; i<hParametricDrcParams->nNodes; i++) {
            
            hParametricDrcInstanceParams->splineNodes.node[i].gainDb = 0.f;
            hParametricDrcInstanceParams->splineNodes.node[i].slope  = 0.f;
            hParametricDrcInstanceParams->splineNodes.node[i].time   = (i+1) * hParametricDrcParams->drcFrameSizeParametricDrc - 1;
            
        }
        
    } else {
        
        if (hParametricDrcInstanceParams->parametricDrcType == PARAM_DRC_TYPE_FF) {
            
            PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams = &(hParametricDrcInstanceParams->parametricDrcTypeFeedForwardParams);
            for (i=0; i<hParametricDrcParams->nNodes; i++) {
                err = processParametricDrcTypeFeedForward(audioIOBuffer,
                                                          audioIOBufferReal,
                                                          audioIOBufferImag,
                                                          i,
                                                          hParametricDrcTypeFeedForwardParams,
                                                          &hParametricDrcInstanceParams->splineNodes);
                if (err) return (err);
            }
            
#ifdef AMD1_PARAMETRIC_LIMITER
        } else if (hParametricDrcInstanceParams->parametricDrcType == PARAM_DRC_TYPE_LIM) {
            
            fprintf(stderr, "ERROR: parametricDrcType = %d has to be processed on full TD sample grid.\n",hParametricDrcInstanceParams->parametricDrcType);
            return (UNEXPECTED_ERROR);
#endif
        
        } else {
            
            fprintf(stderr, "ERROR: Unknown parametricDrcType = %d.\n",hParametricDrcInstanceParams->parametricDrcType);
            return (UNEXPECTED_ERROR);
            
        }
        
    }
    
    return 0;
}

int
processParametricDrcTypeFeedForward(float*                                             audioIOBuffer[],
                                    float*                                         audioIOBufferReal[],
                                    float*                                         audioIOBufferImag[],
                                    int                                                        nodeIdx,
                                    PARAMETRIC_DRC_TYPE_FF_PARAMS* hParametricDrcTypeFeedForwardParams,
                                    SplineNodes*                                           splineNodes)
{
    int c, t, b, n, i, offset;
    float x, y, z0, channelLevel, level, levelDb, gainDb, levelDelta, alpha;
    
    int frameSize = hParametricDrcTypeFeedForwardParams->frameSize;
    int subBandCount = hParametricDrcTypeFeedForwardParams->subBandCount;
    float *levelEstimChannelWeight = hParametricDrcTypeFeedForwardParams->levelEstimChannelWeight;
    int levelEstimKWeightingType = hParametricDrcTypeFeedForwardParams->levelEstimKWeightingType;
    
    FILTER_COEFF_2ND_ORDER preC = hParametricDrcTypeFeedForwardParams->preFilterCoeff;
    FILTER_COEFF_2ND_ORDER rlbC = hParametricDrcTypeFeedForwardParams->rlbFilterCoeff;
    FILTER_STATE_2ND_ORDER* preS = hParametricDrcTypeFeedForwardParams->preFilterState;
    FILTER_STATE_2ND_ORDER* rlbS = hParametricDrcTypeFeedForwardParams->rlbFilterState;

    FILTER_COEFF_2ND_ORDER rlbC_sb = hParametricDrcTypeFeedForwardParams->filterCoeffSubBand;
    FILTER_STATE_2ND_ORDER* rlbS_sbReal = hParametricDrcTypeFeedForwardParams->filterStateSubBandReal;
    FILTER_STATE_2ND_ORDER* rlbS_sbImag = hParametricDrcTypeFeedForwardParams->filterStateSubBandImag;
    float *weightingFilter = hParametricDrcTypeFeedForwardParams->weightingFilter;
    int subBandCompensationType = hParametricDrcTypeFeedForwardParams->subBandCompensationType;
        
    /* level estimation */
    if (audioIOBuffer != NULL) {
        
        level = 0;
        offset = nodeIdx * hParametricDrcTypeFeedForwardParams->frameSize;
        for(c=0; c<hParametricDrcTypeFeedForwardParams->audioChannelCount; c++) {
            channelLevel = 0.f;
            
            if (!levelEstimChannelWeight[c]) continue;
            
            if (levelEstimKWeightingType == 0) { /* pre-filter OFF, RLB filter OFF */
                
                for(t=0; t<frameSize; t++) {
                    
                    x = audioIOBuffer[c][offset+t];
                    
                    channelLevel += x * x;
                    
                }
                
            } else if (levelEstimKWeightingType == 1) { /* pre-filter OFF, RLB filter ON */
                
                for(t=0; t<frameSize; t++) {
                    
                    x = audioIOBuffer[c][offset+t];
                    
                    z0 = x - rlbC.a1 * rlbS[c].z1 - rlbC.a2 * rlbS[c].z2;
                    x = rlbC.b0 * z0 + rlbC.b1 * rlbS[c].z1 + rlbC.b2 * rlbS[c].z2;
                    rlbS[c].z2 = rlbS[c].z1;
                    rlbS[c].z1 = z0;
                    
                    /* add denormals handling if required */
                    
                    channelLevel += x * x;
                    
                }
                
            } else if (levelEstimKWeightingType == 2) { /* pre-filter ON, RLB filter ON */
                
                for(t=0; t<frameSize; t++) {
                    
                    x = audioIOBuffer[c][offset+t];
                    
                    z0 = x - preC.a1 * preS[c].z1 - preC.a2 * preS[c].z2;
                    x = preC.b0 * z0 + preC.b1 * preS[c].z1 + preC.b2 * preS[c].z2;
                    preS[c].z2 = preS[c].z1;
                    preS[c].z1 = z0;
                    
                    z0 = x - rlbC.a1 * rlbS[c].z1 - rlbC.a2 * rlbS[c].z2;
                    x = rlbC.b0 * z0 + rlbC.b1 * rlbS[c].z1 + rlbC.b2 * rlbS[c].z2;
                    rlbS[c].z2 = rlbS[c].z1;
                    rlbS[c].z1 = z0;
                    
                    /* add denormals handling if required */
                    
                    channelLevel += x * x;
                    
                }
                
            } else { /* reserved */
                
                fprintf(stderr, "ERROR: Unknown levelEstimKWeightingType = %d\n",levelEstimKWeightingType);
                return (UNEXPECTED_ERROR);
                
            }
            
            level += levelEstimChannelWeight[c] * channelLevel;
            
        }
        
    } else {
        
        level = 0;
        offset = nodeIdx * hParametricDrcTypeFeedForwardParams->subBandCount;
        for(c=0; c<hParametricDrcTypeFeedForwardParams->audioChannelCount; c++) {
            channelLevel = 0.f;
            
            if (!levelEstimChannelWeight[c]) continue;
            
            if (levelEstimKWeightingType == 0) { /* pre-filter OFF, RLB filter OFF */
                
                for(b=0; b<subBandCount; b++) {
                    
                    x = audioIOBufferReal[c][offset+b];
                    y = audioIOBufferImag[c][offset+b];
                    
                    channelLevel += x * x + y * y;
                    
                }
                
            } else if (levelEstimKWeightingType == 1) { /* pre-filter OFF, RLB filter ON */
                
                for(b=0; b<subBandCount; b++) {
                    
                    x = audioIOBufferReal[c][offset+b] * weightingFilter[b];
                    y = audioIOBufferImag[c][offset+b] * weightingFilter[b];
                    
                    if (b==0 && subBandCompensationType==1) {
                        z0 = x - rlbC_sb.a1 * rlbS_sbReal[c].z1 - rlbC_sb.a2 * rlbS_sbReal[c].z2;
                        x = rlbC_sb.b0 * z0 + rlbC_sb.b1 * rlbS_sbReal[c].z1 + rlbC_sb.b2 * rlbS_sbReal[c].z2;
                        rlbS_sbReal[c].z2 = rlbS_sbReal[c].z1;
                        rlbS_sbReal[c].z1 = z0;
                        
                        z0 = y - rlbC_sb.a1 * rlbS_sbImag[c].z1 - rlbC_sb.a2 * rlbS_sbImag[c].z2;
                        y = rlbC_sb.b0 * z0 + rlbC_sb.b1 * rlbS_sbImag[c].z1 + rlbC_sb.b2 * rlbS_sbImag[c].z2;
                        rlbS_sbImag[c].z2 = rlbS_sbImag[c].z1;
                        rlbS_sbImag[c].z1 = z0;
                        
                        /* add denormals handling if required */
                    }
                    
                    channelLevel += x * x + y * y;
                    
                }
                
            } else if (levelEstimKWeightingType == 2) { /* pre-filter ON, RLB filter ON */
               
                for(b=0; b<subBandCount; b++) {
                    
                    x = audioIOBufferReal[c][offset+b] * weightingFilter[b];
                    y = audioIOBufferImag[c][offset+b] * weightingFilter[b];
                    
                    if (b==0 && subBandCompensationType==1) {
                        z0 = x - rlbC_sb.a1 * rlbS_sbReal[c].z1 - rlbC_sb.a2 * rlbS_sbReal[c].z2;
                        x = rlbC_sb.b0 * z0 + rlbC_sb.b1 * rlbS_sbReal[c].z1 + rlbC_sb.b2 * rlbS_sbReal[c].z2;
                        rlbS_sbReal[c].z2 = rlbS_sbReal[c].z1;
                        rlbS_sbReal[c].z1 = z0;
                        
                        z0 = y - rlbC_sb.a1 * rlbS_sbImag[c].z1 - rlbC_sb.a2 * rlbS_sbImag[c].z2;
                        y = rlbC_sb.b0 * z0 + rlbC_sb.b1 * rlbS_sbImag[c].z1 + rlbC_sb.b2 * rlbS_sbImag[c].z2;
                        rlbS_sbImag[c].z2 = rlbS_sbImag[c].z1;
                        rlbS_sbImag[c].z1 = z0;
                        
                        /* add denormals handling if required */
                    }
                    
                    channelLevel += x * x + y * y;
                    
                }
                
            } else { /* reserved */
                
                fprintf(stderr, "ERROR: Unknown levelEstimKWeightingType = %d\n",levelEstimKWeightingType);
                return (UNEXPECTED_ERROR);
                
            }
            
            level += levelEstimChannelWeight[c] * channelLevel;
            
        }
        
        level /= subBandCount;
    }
    hParametricDrcTypeFeedForwardParams->level[hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex] = level;
    hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex++;
    
    /* integrate level */
    level = 0.f;
    if (hParametricDrcTypeFeedForwardParams->startUpPhase) {
        for (i=0; i<hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex; i++) {
            level += hParametricDrcTypeFeedForwardParams->level[i];
        }
        level /= hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex * hParametricDrcTypeFeedForwardParams->frameSize;
    } else {
        for (i=0; i<hParametricDrcTypeFeedForwardParams->levelEstimFrameCount; i++) {
            level += hParametricDrcTypeFeedForwardParams->level[i];
        }
        level /= hParametricDrcTypeFeedForwardParams->levelEstimIntegrationTime;
    }
    if (hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex == hParametricDrcTypeFeedForwardParams->levelEstimFrameCount) {
        hParametricDrcTypeFeedForwardParams->levelEstimFrameIndex = 0;
        hParametricDrcTypeFeedForwardParams->startUpPhase         = 0;
    }
    
    /* linear to dB */
    if (level < 1e-10f) level = 1e-10f;
    if (levelEstimKWeightingType == 2) {
        levelDb = -0.691f + 10 * (float)log10(level) + 3;
    } else {
        levelDb = 10 * (float)log10(level) + 3;
    }
    levelDb -= hParametricDrcTypeFeedForwardParams->referenceLevelParametricDrc;
    
    /* gain mapping */
    for(n=0; n<hParametricDrcTypeFeedForwardParams->nodeCount; n++) {
        if (levelDb <= (float)hParametricDrcTypeFeedForwardParams->nodeLevel[n]) {
            break;
        }
    }
    if (n == 0) {
        gainDb = (float)hParametricDrcTypeFeedForwardParams->nodeGain[n];
    } else if (n == hParametricDrcTypeFeedForwardParams->nodeCount) {
        gainDb = (float)hParametricDrcTypeFeedForwardParams->nodeGain[n-1] - levelDb + (float)hParametricDrcTypeFeedForwardParams->nodeLevel[n-1];
    } else {
        gainDb = (float)hParametricDrcTypeFeedForwardParams->nodeGain[n] + (levelDb - (float)hParametricDrcTypeFeedForwardParams->nodeLevel[n]) / (float)(hParametricDrcTypeFeedForwardParams->nodeLevel[n-1] - hParametricDrcTypeFeedForwardParams->nodeLevel[n]) * (float)(hParametricDrcTypeFeedForwardParams->nodeGain[n-1] - hParametricDrcTypeFeedForwardParams->nodeGain[n]);
    }
    
    /* gain smoothing */
    levelDelta = levelDb - hParametricDrcTypeFeedForwardParams->levelSmoothDb;
    if (gainDb < hParametricDrcTypeFeedForwardParams->gainSmoothDb) {
        if (levelDelta > hParametricDrcTypeFeedForwardParams->gainSmoothAttackThreshold) {
            alpha = hParametricDrcTypeFeedForwardParams->gainSmoothAttackAlphaFast;
        } else {
            alpha = hParametricDrcTypeFeedForwardParams->gainSmoothAttackAlphaSlow;
        }
    } else {
        if (levelDelta < -hParametricDrcTypeFeedForwardParams->gainSmoothReleaseThreshold) {
            alpha = hParametricDrcTypeFeedForwardParams->gainSmoothReleaseAlphaFast;
        } else {
            alpha = hParametricDrcTypeFeedForwardParams->gainSmoothReleaseAlphaSlow;
        }
    }
    if (gainDb < hParametricDrcTypeFeedForwardParams->gainSmoothDb || hParametricDrcTypeFeedForwardParams->holdCounter == 0) {
        hParametricDrcTypeFeedForwardParams->levelSmoothDb = (1-alpha) * hParametricDrcTypeFeedForwardParams->levelSmoothDb + alpha * levelDb;
        hParametricDrcTypeFeedForwardParams->gainSmoothDb = (1-alpha) * hParametricDrcTypeFeedForwardParams->gainSmoothDb + alpha * gainDb;
    }
    if (hParametricDrcTypeFeedForwardParams->holdCounter) {
        hParametricDrcTypeFeedForwardParams->holdCounter -= 1;
    }
    if (gainDb < hParametricDrcTypeFeedForwardParams->gainSmoothDb) {
        hParametricDrcTypeFeedForwardParams->holdCounter = hParametricDrcTypeFeedForwardParams->gainSmoothHoldOffCount;
    }
    
    /* copy to output struct */
#if PARAM_DRC_DEBUG
    splineNodes->node[nodeIdx].gainDb = hParametricDrcTypeFeedForwardParams->gainSmoothDb; /* levelDb, gainDb */
    printf("%f2.4\n",hParametricDrcTypeFeedForwardParams->gainSmoothDb);
    splineNodes->node[nodeIdx].time   = hParametricDrcTypeFeedForwardParams->frameSize + offset - 1;
#else
    splineNodes->node[nodeIdx].gainDb = hParametricDrcTypeFeedForwardParams->gainSmoothDb;
    splineNodes->node[nodeIdx].slope  = 0.f;
    splineNodes->node[nodeIdx].time   = hParametricDrcTypeFeedForwardParams->frameSize + offset - 1;
#endif
        
    return 0;
}

#ifdef AMD1_PARAMETRIC_LIMITER
int
processParametricDrcTypeLim(float*                                      audioIOBuffer[],
                            float                           loudnessNormalizationGainDb,
                            PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams,
                            float*                                            lpcmGains)
{
    unsigned int i, j;
    float tmp, gain, maximum, sectionMaximum;

    float loudnessNormalizationGain = (float)pow(10.0f, 0.05f * loudnessNormalizationGainDb);
    
    unsigned int channels       = hParametricDrcTypeLimParams->channels;
    unsigned int attack         = hParametricDrcTypeLimParams->attack;
    float        attackConst    = hParametricDrcTypeLimParams->attackConst;
    float        releaseConst   = hParametricDrcTypeLimParams->releaseConst;
    float        threshold      = hParametricDrcTypeLimParams->threshold;
    float* levelEstimChannelWeight = hParametricDrcTypeLimParams->levelEstimChannelWeight;
    
    float*       maxBuf         = hParametricDrcTypeLimParams->maxBuf;
    unsigned int maxBufIdx      = hParametricDrcTypeLimParams->maxBufIdx;
    float*       maxBufSlow     = hParametricDrcTypeLimParams->maxBufSlow;
    unsigned int maxBufSlowIdx  = hParametricDrcTypeLimParams->maxBufSlowIdx;
    unsigned int maxBufSecIdx   = hParametricDrcTypeLimParams->maxBufSecIdx;
    unsigned int maxBufSecCtr   = hParametricDrcTypeLimParams->maxBufSecCtr;
    unsigned int secLen         = hParametricDrcTypeLimParams->secLen;
    unsigned int nMaxBufSec     = hParametricDrcTypeLimParams->nMaxBufSec;
    
    float        cor            = hParametricDrcTypeLimParams->cor;
    double       smoothState0   = hParametricDrcTypeLimParams->smoothState0;
    
    for (i = 0; i < hParametricDrcTypeLimParams->frameSize; i++) {
        /* get maximum absolute sample value of all channels */
        tmp = threshold;
        for (j = 0; j < channels; j++) {
            if (!levelEstimChannelWeight[j]) continue;
            
            tmp = max(tmp, (float)fabs(loudnessNormalizationGain*levelEstimChannelWeight[j]*audioIOBuffer[j][i]));
        }
        
        /* running maximum over attack+1 samples */
        maxBuf[maxBufIdx] = tmp;
        
        /* search section of maxBuf */
        sectionMaximum = maxBuf[maxBufSecIdx];
        for (j = 1; j < secLen; j++) {
            if (maxBuf[maxBufSecIdx+j] > sectionMaximum) sectionMaximum = maxBuf[maxBufSecIdx+j];
        }
        
        /* find maximum of slow (downsampled) max buffer */
        maximum = sectionMaximum;
        for (j = 0; j < nMaxBufSec; j++) {
            if (maxBufSlow[j] > maximum) maximum = maxBufSlow[j];
        }
        
        maxBufIdx++;
        maxBufSecCtr++;
        
        /* if maxBuf section is finished, or end of maxBuf is reached,
         store the maximum of this section and open up a new one */
        if ((maxBufSecCtr >= secLen)||(maxBufIdx >= attack+1)) {
            maxBufSecCtr = 0;
            
            maxBufSlow[maxBufSlowIdx] = sectionMaximum;
            maxBufSlowIdx++;
            if (maxBufSlowIdx >= nMaxBufSec)
                maxBufSlowIdx = 0;
            maxBufSlow[maxBufSlowIdx] = 0.0f;  /* zero out the value representing the new section */
            
            maxBufSecIdx += secLen;
        }
        
        if (maxBufIdx >= (attack+1)) {
            maxBufIdx = 0;
            maxBufSecIdx = 0;
        }
        
        /* calc gain */
        if (maximum > threshold) {
            gain = threshold / maximum;
        }
        else {
            gain = 1;
        }
        
        /* gain smoothing */
        /* first order IIR filter with attack correction to avoid overshoots */
        
        /* correct the 'aiming' value of the exponential attack to avoid the remaining overshoot */
        if (gain < smoothState0) {
            cor = min(cor, (gain - 0.1f * (float)smoothState0) * 1.11111111f);
        }
        else {
            cor = gain;
        }
        
        /* smoothing filter */
        if (cor < smoothState0) {
            smoothState0 = attackConst * (smoothState0 - cor) + cor;  /* attack */
            smoothState0 = max(smoothState0, gain); /* avoid overshooting target */
        }
        else {
            smoothState0 = releaseConst * (smoothState0 - cor) + cor; /* release */
        }
        
        gain = (float)smoothState0;
        
        lpcmGains[i] = gain;
    }
    
    hParametricDrcTypeLimParams->maxBufIdx     = maxBufIdx;
    hParametricDrcTypeLimParams->maxBufSecIdx  = maxBufSecIdx;
    hParametricDrcTypeLimParams->maxBufSecCtr  = maxBufSecCtr;
    hParametricDrcTypeLimParams->maxBufSlowIdx = maxBufSlowIdx;
    
    hParametricDrcTypeLimParams->cor           = cor;
    hParametricDrcTypeLimParams->smoothState0  = smoothState0;
    
    return 0;
}
#endif

/* close functions */
int
removeParametricDrc(PARAMETRIC_DRC_PARAMS* hParametricDrcParams)
{
    int err = 0, instanceIndex = 0;
    
    for (instanceIndex=0; instanceIndex<hParametricDrcParams->parametricDrcInstanceCount; instanceIndex++) {
        
        removeParametricDrcInstance(instanceIndex,
                                    hParametricDrcParams);
        if (err) return (err);
    }
    
    return 0;
}

int
removeParametricDrcInstance(const int                     instanceIndex,
                            PARAMETRIC_DRC_PARAMS* hParametricDrcParams)
{
    if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].disableParamtricDrc == 0) {
        
        if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType == PARAM_DRC_TYPE_FF) {
            
            
#ifdef AMD1_PARAMETRIC_LIMITER
        } else if (hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType == PARAM_DRC_TYPE_LIM) {
            
            PARAMETRIC_DRC_TYPE_LIM_PARAMS* hParametricDrcTypeLimParams = &(hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcTypeLimParams);
            
            free(hParametricDrcTypeLimParams->maxBuf); hParametricDrcTypeLimParams->maxBuf = NULL;
            free(hParametricDrcTypeLimParams->maxBufSlow); hParametricDrcTypeLimParams->maxBufSlow = NULL;
#endif
            
        } else {
            
            fprintf(stderr, "ERROR: Unknown parametricDrcType = %d.\n",hParametricDrcParams->parametricDrcInstanceParams[instanceIndex].parametricDrcType);
            return (UNEXPECTED_ERROR);
            
        }
    }
    
    return 0;
}

/***********************************************************************************/

#endif /* AMD1_SYNTAX */
