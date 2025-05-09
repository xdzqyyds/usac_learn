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
#include <stdlib.h>
#include <math.h>
#include "uniDrcCommon.h"
#include "uniDrc.h"
#include "uniDrcFilterBank.h"
#include "uniDrcMultiBand.h"


#define LOG10_2_INV                 3.32192809f     /* 1.0 / log10(2.0) */
#define LR_FILTER_SLOPE  (-24.0f)        /* slope of Linkwitz/Riley filters in dB per octave */

extern const FilterBankParams filterBankParams[FILTER_BANK_PARAMETER_COUNT];


int
initFcenterNormSb(const int nSubbands,
#ifdef AMD2_COR1
                  const int subBandDomainMode,
#endif
                  float* fCenterNormSb)
{
    int s;
#ifdef AMD2_COR1
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
#endif
            for (s=0; s<nSubbands; s++) {
                fCenterNormSb[s] = (s + 0.5f) / (2.0f * nSubbands);
            }
#ifdef AMD2_COR1
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            for (s=0; s<nSubbands; s++) {
                fCenterNormSb[s] = s / (2.0f * nSubbands);
            }
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
        default:
            return (-1); /* not supported */
    }
#endif
    return (0);
}


int
generateSlope (const int audioDecoderSubbandCount,
               const float* fCenterNormSb,
               const float fCrossNormLo,
               const float fCrossNormHi,
               float* response)
{
    int s;
    float norm = 0.05f * LR_FILTER_SLOPE * LOG10_2_INV;
    
    for (s=0; s<audioDecoderSubbandCount; s++)
    {
        if (fCenterNormSb[s] < fCrossNormLo)
        {
#ifdef AMD2_COR1
            if (fCenterNormSb[s] > 0.0f)
#endif
                response[s] = (float)pow (10.0, norm * log10(fCrossNormLo / fCenterNormSb[s]));
#ifdef AMD2_COR1
            else
                response[s] = 0.0f;
#endif
        }
        else if (fCenterNormSb[s] < fCrossNormHi)
        {
            response[s] = 1.0f;
        }
        else
        {
            response[s] = (float)pow (10.0, norm * log10(fCenterNormSb[s] / fCrossNormHi));
        }
    }
    return (0);
}

int
generateOverlapWeights (const int nDrcBands,
                        const int drcBandType,
                        const GainParams* gainParams,
#ifdef AMD2_COR1
                        const int subBandDomainMode,
#else
                        const int audioDecoderSubbandCount,
#endif
                        OverlapParamsForGroup* overlapParamsForGroup)
{
    float fCenterNormSb[AUDIO_CODEC_SUBBAND_COUNT_MAX];
    float wNorm[AUDIO_CODEC_SUBBAND_COUNT_MAX];
    float fCrossNormLo, fCrossNormHi;
    int err, b, s, startSubBandIndex = 0, stopSubBandIndex = 0;
#ifdef AMD2_COR1
    int audioDecoderSubbandCount = 0;
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
            break;
    }
    err = initFcenterNormSb(audioDecoderSubbandCount, subBandDomainMode, fCenterNormSb);
#else
    err = initFcenterNormSb(audioDecoderSubbandCount, fCenterNormSb);
#endif
    
    if (drcBandType == 1) {        
        fCrossNormLo = 0.0f;
        for (b=0; b<nDrcBands; b++)
        {
            if (b<nDrcBands - 1)
            {
                fCrossNormHi = filterBankParams[gainParams[b+1].crossoverFreqIndex].fCrossNorm;
            }
            else
            {
                fCrossNormHi = 0.5f;
            }
            generateSlope (audioDecoderSubbandCount,
                           fCenterNormSb,
                           fCrossNormLo,
                           fCrossNormHi,
                           overlapParamsForGroup->overlapParamsForBand[b].overlapWeight);
            
            fCrossNormLo = fCrossNormHi;
        }
        for (s=0; s<audioDecoderSubbandCount; s++)
        {
            wNorm[s] = overlapParamsForGroup->overlapParamsForBand[0].overlapWeight[s];
            for (b=1; b<nDrcBands; b++)
            {
                wNorm[s] += overlapParamsForGroup->overlapParamsForBand[b].overlapWeight[s];
            }
        }
        
        for (s=0; s<audioDecoderSubbandCount; s++)
        {
            for (b=0; b<nDrcBands; b++)
            {
                overlapParamsForGroup->overlapParamsForBand[b].overlapWeight[s] /= wNorm[s];
            }
        }
    } else {
        startSubBandIndex = 0;
        for (b=0; b<nDrcBands; b++)
        {
            if (b < nDrcBands-1)
            {
                stopSubBandIndex = gainParams[b+1].startSubBandIndex-1;
            }
            else
            {
                stopSubBandIndex = audioDecoderSubbandCount-1;
            }
            for (s=0; s<audioDecoderSubbandCount; s++)
            {
                if (s >= startSubBandIndex && s <= stopSubBandIndex)
                {
                    overlapParamsForGroup->overlapParamsForBand[b].overlapWeight[s] = 1.0;
                }
                else
                {
                    overlapParamsForGroup->overlapParamsForBand[b].overlapWeight[s] = 0.0;
                }
            }
            startSubBandIndex = stopSubBandIndex+1;
        }
    }
    
    return (0);
}

int
initOverlapWeight (const DrcCoefficientsUniDrc* drcCoefficientsUniDrc,
                   const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                   const int subBandDomainMode,
                   OverlapParams* overlapParams)
{
    int err = 0, g;
#ifndef AMD2_COR1
    int audioDecoderSubbandCount = 0;
    switch (subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            audioDecoderSubbandCount              = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
            break;
    }
#endif
    /* allocate memory here if necessary */

    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
    {
        if (drcInstructionsUniDrc->bandCountForChannelGroup[g] > 1)
        {
            err = generateOverlapWeights(drcInstructionsUniDrc->bandCountForChannelGroup[g],
                                         drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].drcBandType,
                                         drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].gainParams,
#ifndef AMD2_COR1
                                         audioDecoderSubbandCount,
#else
                                         subBandDomainMode,
#endif
                                         &(overlapParams->overlapParamsForGroup[g]));
            if (err) return (err);
        }
    }
    
    return (0);
}

int
removeOverlapWeights (OverlapParams* overlapParams)
{
    /* just a placeholder */
    /* free memory here if necessary */
    return (0);
}



