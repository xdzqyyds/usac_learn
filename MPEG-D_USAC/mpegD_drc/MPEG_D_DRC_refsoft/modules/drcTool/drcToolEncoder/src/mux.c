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
#include <math.h>
#include "drcToolEncoder.h"
#include "tables.h"
#include "gainEnc.h"
#if AMD1_SYNTAX
#include "uniDrcFilterBank.h"
#include "uniDrcEq.h"
#endif
#include "mux.h"

extern const float downmixCoeff[];
extern const float downmixCoeffLfe[];

#if AMD1_SYNTAX
#define INV_LOG10_2  3.32192809488736f /* 1.0 / log10(2.0) */

extern const float channelWeight[];
extern const float downmixCoeffV1[];
extern const float eqSlopeTable[];
extern const float eqGainDeltaTable[];
extern const float zeroPoleRadiusTable[];
extern const float zeroPoleAngleTable[];
#endif /* AMD1_SYNTAX */

static int putBits(wobitbufHandle bitstream, unsigned int nBits, int data) {
#if DEBUG_BITSTREAM
    printf("nBitsRequested = %2d Value = 0x%02X\n",nBits, data);
#endif
    return wobitbuf_WriteBits(bitstream, data, nBits);
};
/* ====================================================================================
                        multiplex of in-stream DRC configuration
 ====================================================================================*/

#if AMD1_SYNTAX
int
getDrcComplexityLevel (UniDrcConfig* uniDrcConfig,
                       DrcEncParams* encParams,
                       DrcInstructionsUniDrc* drcInstructionsUniDrc)
{
    int c, g, k, i, group;
    int gainSetIndex, bandCount;
    float wMod, cplx;
    GainModifiers* gainModifiers = NULL;
    ShapeFilterBlockParams* shapeFilterBlockParams = NULL;
    GainSetParams* gainSetParams = NULL;
    int channelCount = uniDrcConfig->channelLayout.baseChannelCount;
    
    if ((drcInstructionsUniDrc->drcApplyToDownmix != 0) &&
        (drcInstructionsUniDrc->downmixId != 0) &&
        (drcInstructionsUniDrc->downmixId != 0x7F) &&
        (drcInstructionsUniDrc->additionalDownmixIdCount == 0))
    {
#if AMD2_COR3
        channelCount = -1;
        for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
        {
            if (drcInstructionsUniDrc->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) 
            {
                channelCount = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /* targetChannelCountFromDownmixId */
                break;
            }
        }
#if AMD1_SYNTAX
        if (i == uniDrcConfig->downmixInstructionsCount)
        {
            if (uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1Present) 
            {
                for (i=0; i<uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1Count; i++)
                {
                    if (drcInstructionsUniDrc->downmixId == uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1[i].downmixId) 
                    {
                        channelCount = uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1[i].targetChannelCount;  /* targetChannelCountFromDownmixId */
                        break;
                    }
                }
            }
        }
#endif /* AMD1_SYNTAX */
        if (channelCount == -1)
        {
            fprintf(stderr,"ERROR: downmix ID %d not found\n", drcInstructionsUniDrc->downmixId);
            return(UNEXPECTED_ERROR);
        }
#else
        for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
        {
            if (drcInstructionsUniDrc->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) break;
        }
        if (i == uniDrcConfig->downmixInstructionsCount)
        {
            fprintf(stderr,"ERROR: downmix ID not found\n");
            return(UNEXPECTED_ERROR);
        }
        channelCount = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /*  targetChannelCountFromDownmixId*/
#endif /* AMD2_COR3 */
    }
    else if (drcInstructionsUniDrc->drcApplyToDownmix != 0 && ((drcInstructionsUniDrc->downmixId == 0x7F) || (drcInstructionsUniDrc->additionalDownmixIdCount != 0)))
    {
        channelCount = 1;
    }
    
    drcInstructionsUniDrc->drcChannelCount = channelCount;
    group = 0;
    for (c=0; c<drcInstructionsUniDrc->drcChannelCount; c++) {
        int gainSetIndex = drcInstructionsUniDrc->gainSetIndex[c];
        int skipSet = FALSE;
        if (gainSetIndex >= 0) {
            for (k=c-1; k>=0; k--) {
                if (drcInstructionsUniDrc->gainSetIndex[k] == gainSetIndex) {
                    drcInstructionsUniDrc->channelGroupForChannel[c] = drcInstructionsUniDrc->channelGroupForChannel[k];
                    skipSet = TRUE;
                }
            }
            if (skipSet == FALSE) {
                drcInstructionsUniDrc->channelGroupForChannel[c] = group;
                group++;
            }
        }
        else {
            drcInstructionsUniDrc->channelGroupForChannel[c] = -1;
        }
    }
    if (group != drcInstructionsUniDrc->nDrcChannelGroups) {
        fprintf(stderr, "ERROR: Number of channel groups not matching %d %d\n", group, drcInstructionsUniDrc->nDrcChannelGroups);
        return (-1);
    }
    
    /* count nChannelsPerChannelGroup */
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++) {
        drcInstructionsUniDrc->nChannelsPerChannelGroup[g] = 0;
        for (c=0; c<drcInstructionsUniDrc->drcChannelCount; c++) {
            if (drcInstructionsUniDrc->channelGroupForChannel[c] == g) {
                drcInstructionsUniDrc->nChannelsPerChannelGroup[g]++;
            }
        }
    }
    
    cplx = 0.0f;
    
    if (encParams->domain == TIME_DOMAIN){
        wMod = COMPLEXITY_W_MOD_TIME;
    }
    else {
        wMod = COMPLEXITY_W_MOD_SUBBAND;
    }
    for (c=0; c<drcInstructionsUniDrc->drcChannelCount; c++) {
        if (drcInstructionsUniDrc->gainSetIndex[c] >= 0) {
            cplx += wMod;
        }
    }
    
    if (encParams->domain == TIME_DOMAIN){
        FilterBanks filterBanks;
        
        initAllFilterBanks(&uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0],
                           drcInstructionsUniDrc,
                           &filterBanks);
        
        cplx += COMPLEXITY_W_IIR * filterBanks.complexity; /* Qi: time-domain filter bank for all channels */
        removeAllFilterBanks(&filterBanks);
        
        for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++) {
            gainModifiers = &drcInstructionsUniDrc->gainModifiers[g];
            if (gainModifiers->shapeFilterPresent == 1) {
                float cplx_tmp = 0.0f;
                shapeFilterBlockParams = &uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].shapeFilterBlockParams[gainModifiers->shapeFilterIndex]; /* assuming index 0 is correct */
                if (shapeFilterBlockParams->lfCutFilterPresent == 1) {
                    cplx_tmp += COMPLEXITY_W_SHAPE;
                }
                if (shapeFilterBlockParams->lfBoostFilterPresent == 1) {
                    cplx_tmp += COMPLEXITY_W_SHAPE;
                }
                if (shapeFilterBlockParams->hfCutFilterPresent == 1) {
                    cplx_tmp += COMPLEXITY_W_SHAPE * 2.0f;
                }
                if (shapeFilterBlockParams->hfBoostFilterPresent == 1) {
                    cplx_tmp += COMPLEXITY_W_SHAPE * 2.0f;
                }
                cplx += cplx_tmp * drcInstructionsUniDrc->nChannelsPerChannelGroup[g];
            }
        }
    }
    else {
        gainSetParams = uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams; /* assuming index 0 is correct */
        for (c=0; c<drcInstructionsUniDrc->drcChannelCount; c++) {
            gainSetIndex = drcInstructionsUniDrc->gainSetIndex[c];
            if (gainSetIndex >= 0) {
                if (gainSetParams[gainSetIndex].drcBandType == 1) {
                    bandCount = gainSetParams[gainSetIndex].bandCount;
                    if (bandCount > 1) {
                        cplx += COMPLEXITY_W_LAP * bandCount;
                    }
                }
            }
        }
    }
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++) {
        int gainSetIndexOffset = 0;
        gainSetIndex = -1;
        for (c=0; c<drcInstructionsUniDrc->drcChannelCount; c++) {
            if (drcInstructionsUniDrc->channelGroupForChannel[c] == g) {
                gainSetIndex = drcInstructionsUniDrc->gainSetIndex[c];
                break;
            }
        }
        if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0) { /* TODO: include V0 count in second step? */
            gainSetIndexOffset = uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetCount;
            gainSetParams = uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams; /* assuming index 0 is correct */
        }
        if (gainSetIndex < gainSetIndexOffset) {
            if (encParams->domain == TIME_DOMAIN){
                if (gainSetParams[gainSetIndex].gainInterpolationType == GAIN_INTERPOLATION_TYPE_SPLINE) {
                    cplx += COMPLEXITY_W_SPLINE;
                }
                if (gainSetParams[gainSetIndex].gainInterpolationType == GAIN_INTERPOLATION_TYPE_LINEAR) {
                    cplx += COMPLEXITY_W_LINEAR;
                }
            }
        }
        else {
            /* parametric DRC */
            int parametricDrcType;
            int channelCountSideChain;
            ParametricDrcInstructions* parametricDrcInstructions = NULL;
            ParametricDrcGainSetParams* parametricDrcGainSetParams = &uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetIndex-gainSetIndexOffset];
            for (i=0; i<uniDrcConfig->uniDrcConfigExt.parametricDrcInstructionsCount; i++) {
                if (parametricDrcGainSetParams->parametricDrcId == uniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[i].parametricDrcId) {
                    parametricDrcInstructions = &uniDrcConfig->uniDrcConfigExt.parametricDrcInstructions[i];
                    break;
                }
            }
            if (parametricDrcInstructions->parametricDrcPresetIdPresent) {
                switch (parametricDrcInstructions->parametricDrcPresetId) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        parametricDrcType = PARAM_DRC_TYPE_FF;
                        break;
                    case 5:
                        parametricDrcType = PARAM_DRC_TYPE_LIM;
                        break;
                    default:
                        fprintf(stderr,"ERROR: unknown parametricDrcPresetId\n");
                        return(UNEXPECTED_ERROR);
                        break;
                }
            } else {
                parametricDrcType = parametricDrcInstructions->parametricDrcType;
            }
            channelCountSideChain = drcInstructionsUniDrc->nChannelsPerChannelGroup[g];
            if (parametricDrcGainSetParams->sideChainConfigType == 1) {
                int channelCountFromDownmixId;
                int channelCount = 0;
                if (parametricDrcGainSetParams->downmixId == 0x0) {
                    channelCountFromDownmixId = uniDrcConfig->channelLayout.baseChannelCount;
                } else if (parametricDrcGainSetParams->downmixId == 0x7F) {
                    channelCountFromDownmixId = 1;
                } else {
                    for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
                    {
                        if (parametricDrcGainSetParams->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) break;
                    }
                    if (i == uniDrcConfig->downmixInstructionsCount)
                    {
                        fprintf(stderr,"ERROR: downmix ID not found\n");
                        return(UNEXPECTED_ERROR);
                    }
                    channelCountFromDownmixId = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /*  channelCountFromDownmixId*/
                }
                
                for (i=0; i<channelCountFromDownmixId; i++) {
                    if (parametricDrcGainSetParams->levelEstimChannelWeight[i] != 0) {
                        channelCount++;
                    }
                }
                channelCountSideChain = channelCount;
            }
            if (encParams->domain == TIME_DOMAIN){
                if (parametricDrcType == PARAM_DRC_TYPE_FF) {
                    int weightingFilterOrder = 2;
                    if (parametricDrcInstructions->parametricDrcPresetIdPresent == 0) {
                        if (parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimKWeightingType == 0) {
                            weightingFilterOrder = 0;
                        } else if (parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimKWeightingType == 1) {
                            weightingFilterOrder = 1;
                        }
                    }
                    cplx += channelCountSideChain * (COMPLEXITY_W_PARAM_DRC_FILT * weightingFilterOrder + 1) + 3;
                }
                else if (parametricDrcType == PARAM_DRC_TYPE_LIM) {
                    float ratio = 1.0f;
                    if (parametricDrcInstructions->parametricDrcLookAheadPresent == 1) {
                        ratio = (float) parametricDrcInstructions->parametricDrcLookAhead / (float) PARAM_DRC_TYPE_LIM_ATTACK_DEFAULT;
                    }
                    cplx += channelCountSideChain * COMPLEXITY_W_PARAM_LIM_FILT + COMPLEXITY_W_PARAM_DRC_ATTACK * sqrt(ratio);
                }
            }
            else {
                /* Sub-band domain */
                if (parametricDrcType == PARAM_DRC_TYPE_FF) {
                    cplx += channelCountSideChain * COMPLEXITY_W_PARAM_DRC_SUBBAND;
                }
            }
        }
    }
    
    if (drcInstructionsUniDrc->downmixId == 0x7F)
    {
        channelCount = uniDrcConfig->channelLayout.baseChannelCount;
    }
    cplx = log10(cplx / channelCount) / log10(2.0f);
    drcInstructionsUniDrc->drcSetComplexityLevel = (int) max(0, ceil(cplx));
    if (drcInstructionsUniDrc->drcSetComplexityLevel > DRC_COMPLEXITY_LEVEL_MAX) {
        fprintf(stderr, "ERROR: DRC set exceeds complexity limit of 15: %d\n", drcInstructionsUniDrc->drcSetComplexityLevel);
        return(-1);
    }
    return(0);
}

int
getEqComplexityLevel (UniDrcConfigExt* uniDrcConfigExt,
                      DrcEncParams* drcParams,
                      EqInstructions* eqInstructions)
{
    int err, subBandDomainMode;
    EqSet* eqSet = NULL;
    
    if ((drcParams->domain == TIME_DOMAIN) &&
        (eqInstructions->tdFilterCascadePresent == 0) &&
        (eqInstructions->subbandGainsPresent == 0))
    {
        eqInstructions->eqSetComplexityLevel = 0;
        return (0);
    }

    subBandDomainMode = SUBBAND_DOMAIN_MODE_OFF;
    if (eqInstructions->subbandGainsPresent == 1)
    {
        subBandDomainMode = SUBBAND_DOMAIN_MODE_QMF64; /* any subband mode can be used */
    }
    
    err = createEqSet(&eqSet);
    if (err) return (err);

    err = deriveEqSet (&uniDrcConfigExt->eqCoefficients,
                       eqInstructions,
                       drcParams->sampleRate,
                       drcParams->drcFrameSize,
                       subBandDomainMode,
                       eqSet);
    if (err) return (err);

    err = getEqComplexity (eqSet, &eqInstructions->eqSetComplexityLevel);
    if (err) return (err);
    
    err = removeEqSet(&eqSet);
    if (err) return (err);

    return(0);
}

int
encodeDownmixCoefficient(float downmixCoefficient, float downmixOffset)
{
    int code, i;
    const float* coeffTable;
    float coeffDb;
    
    coeffTable = downmixCoeffV1;
    coeffDb = 20.0f * (float)log10(downmixCoefficient) - downmixOffset;
    
    if (coeffDb < coeffTable[30])
    {
        code = 31;
    }
    else
    {
        i = 0;
        while (coeffDb < coeffTable[i]) i++;
        if ((i>0) && (coeffDb > 0.5f * (coeffTable[i-1] + coeffTable[i]))) i--;
        code = i;
    }
    return code;
}

int
writeDownmixCoefficientV1(wobitbufHandle bitstream,
                          const float downmixCoefficient[],
                          const int baseChannelCount,
                          const int targetChannelCount)
{
    int err=0, i, k, bsDownmixOffset = 0, code;
    float downmixOffset[3];
    float tmp;
    float quantErr, quantErrMin;
    const float* coeffTable;

    coeffTable = downmixCoeffV1;
    tmp = log10((float)targetChannelCount/(float)baseChannelCount);
    downmixOffset[0] = 0.0f;
    downmixOffset[1] = 0.5f * floor (0.5f + 20.0f * tmp);
    downmixOffset[2] = 0.5f * floor (0.5f + 40.0f * tmp);
    
    quantErrMin = 1000.0f;
    for (i=0; i<3; i++) {
        quantErr = 0.0f;
        for (k=0; k<targetChannelCount * baseChannelCount; k++)
        {
            code = encodeDownmixCoefficient(downmixCoefficient[k], downmixOffset[i]);
            quantErr += fabs (20.0f * log10(downmixCoefficient[k]) - (coeffTable[code] + downmixOffset[i]));
        }
        if (quantErrMin > quantErr) {
            quantErrMin = quantErr;
            bsDownmixOffset = i;
        }
    }
    
    err = putBits(bitstream, 4, bsDownmixOffset);
    if (err) return(err);
    
    for (k=0; k<targetChannelCount * baseChannelCount; k++)
    {
        code = encodeDownmixCoefficient(downmixCoefficient[k], downmixOffset[bsDownmixOffset]);
        err = putBits(bitstream, 5, code);
        if (err) return(err);
    }
    return (0);
}
#endif /* AMD1_SYNTAX */

int
encDownmixCoefficient(const float downmixCoefficient,
                      const int isLfeChannel,
                      int* codeSize,
                      int* code)
{
    int i;
    const float* coeffTable;
    float coeffDb;
    
    coeffDb = 20.0f * (float)log10(downmixCoefficient);
    
    if (isLfeChannel == TRUE)
    {
        coeffTable = downmixCoeffLfe;
    }
    else
    {
        coeffTable = downmixCoeff;
    }
    if (coeffDb < coeffTable[14])
    {
        *code = 15;
    }
    else
    {
        i = 0;
        while (coeffDb < coeffTable[i]) i++;
        if ((i>0) && (coeffDb > 0.5f * (coeffTable[i-1] + coeffTable[i]))) i--;
        *code = i;
    }
    *codeSize = 4;
    return (0);
}

static int
encPeak(const float peak,
        int* code,
        int* codeSize)
{
    int bits;
    
    bits = ((int) (0.5f + 32.0f * (20.0f - peak) + 10000.0f)) - 10000;
    bits = min(0x0FFF, bits);
    bits = max(0x1, bits);
    
    *code = bits;
    *codeSize = 12;

    return (0);
}


int
encMethodValue(const int methodDefinition,
               const float methodValue,
               int* codeSize,
               int* code)
{
    int bits;
    switch (methodDefinition)
    {
        case METHOD_DEFINITION_UNKNOWN_OTHER:
        case METHOD_DEFINITION_PROGRAM_LOUDNESS:
        case METHOD_DEFINITION_ANCHOR_LOUDNESS:
        case METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE:
        case METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX:
        case METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX:
            bits = ((int) (0.5f + 4.0f * (methodValue + 57.75f) + 10000.0f)) - 10000;
            bits = min(0x0FF, bits);
            bits = max(0x0, bits);
            *codeSize = 8;
            break;
        case METHOD_DEFINITION_LOUDNESS_RANGE:
            if (methodValue < 0.0f)
                bits = 0;
            else if (methodValue <= 32.0f)
                bits = (int) (4.0f * methodValue + 0.5f);
            else if (methodValue <= 70.0f)
                bits = ((int)(2.0f * (methodValue - 32.0f) + 0.5f)) + 128;
            else if (methodValue < 121.0f)
                bits = ((int) ((methodValue - 70.0f) + 0.5f)) + 204;
            else
                bits = 255;
            *codeSize = 8;
            break;
        case METHOD_DEFINITION_MIXING_LEVEL:
            bits = (int)(0.5f + methodValue - 80.0f);
            bits = min(0x1F, bits);
            bits = max(0x0, bits);
            *codeSize = 5;
            break;
        case METHOD_DEFINITION_ROOM_TYPE:
            bits = (int)(0.5f + methodValue);
            if (bits > 0x2)
            {
                fprintf(stderr, "ERROR: methodDefinition %d has unexpected methodValue: %d\n", methodDefinition, bits);
                return (UNEXPECTED_ERROR);
            }
            bits = min(0x2, bits);
            bits = max(0x0, bits);
            *codeSize = 2;
            break;
        case METHOD_DEFINITION_SHORT_TERM_LOUDNESS:
            bits = ((int) (0.5f + 2.0f * (methodValue + 116.f) + 10000.0f)) - 10000;
            bits = min(0x0FF, bits);
            bits = max(0x0, bits);
            *codeSize = 8;
            break;
        default:
        {
            fprintf(stderr, "ERROR: methodDefinition unexpected value: %d\n", methodDefinition);
            return (UNEXPECTED_ERROR);
        }
    }
    *code = bits;
    return(0);
}

static int
quantizeDuckingScaling(DuckingModifiers* duckingModifiers)
{
    float delta;
    int mu;
    
    if (duckingModifiers->duckingScalingPresent)
    {
        delta = duckingModifiers->duckingScaling - 1.0f;
        
        if (delta > 0.0f)
        {
            mu = - 1 + (int) (0.5f + 8.0f * delta);
            if (mu == -1)
            {
                duckingModifiers->duckingScalingQuantized =  1.0f;
                duckingModifiers->duckingScalingPresent =  FALSE;
            }
            else
            {
                mu = min(7, mu);
                mu = max(0, mu);
                duckingModifiers->duckingScalingQuantized =  1.0f + 0.125f * (1.0f + mu);
            }
        }
        else
        {
            mu = - 1 + (int) (0.5f - 8.0f * delta);
            if (mu == -1)
            {
                duckingModifiers->duckingScalingQuantized =  1.0f;
                duckingModifiers->duckingScalingPresent =  FALSE;
            }
            else
            {
                mu = min(7, mu);
                mu = max(0, mu);
                duckingModifiers->duckingScalingQuantized =  1.0f - 0.125f * (1.0f + mu);
            }
        }
    }
    else
    {
        duckingModifiers->duckingScalingQuantized =  1.0f;
    }
    return (0);
}


int
encDuckingScaling(const float scaling,
                  int* bits,
                  float* scalingQuantized,
                  int *removeScalingValue)
{
    float delta;
    int mu, sigma;
    
    *removeScalingValue = FALSE;
    delta = scaling - 1.0f;
    
    if (delta > 0.0f)
    {
        mu = - 1 + (int) (0.5f + 8.0f * delta);
        sigma = 0;
        *bits = 0;
    }
    else
    {
        mu = - 1 + (int) (0.5f - 8.0f * delta);
        sigma = -1;
        *bits = 1 << 3;
    }
    if (mu == -1)
    {
        *scalingQuantized = 1.0f;
        *removeScalingValue = TRUE;
    }
    else
    {
        mu = min(7, mu);
        mu = max(0, mu);
        *bits += mu;
        if (sigma == 0)
        {
            *scalingQuantized = 1.0f + 0.125f * (1.0f + mu);
        }
        else
        {
            *scalingQuantized = 1.0f - 0.125f * (1.0f + mu);
        }
    }
    return (0);
}

int
encDuckingModifiers(wobitbufHandle bitstream,
                    DuckingModifiers* duckingModifiers)
{
    int err, bits;
    int removeScalingValue;
    
    if (duckingModifiers->duckingScalingPresent == TRUE)
    {
        err = encDuckingScaling (duckingModifiers->duckingScaling, &bits, &(duckingModifiers->duckingScalingQuantized), &removeScalingValue);
        if (err) return(err);
        
        if (removeScalingValue)
        {
            duckingModifiers->duckingScalingPresent = FALSE;
        }
        
        err = putBits(bitstream, 1, duckingModifiers->duckingScalingPresent);
        if (err) return(err);
        
        if(duckingModifiers->duckingScalingPresent)
        {
            err = putBits(bitstream, 4, bits);
            if (err) return(err);
        }
    }
    else
    {
        err = putBits(bitstream, 1, duckingModifiers->duckingScalingPresent);
        if (err) return(err);
    }
    return (0);
}

int
encGainModifiers(wobitbufHandle bitstream,
                 const int version,
                 const int bandCount,
                 GainModifiers* gainModifiers)
{
    int err, tmp, sign;
    
#if AMD1_SYNTAX
    int b;
    if (version == 0)
    {
        err = putBits(bitstream, 1, gainModifiers->gainScalingPresent[0]);
        if (err) return(err);
        
        if(gainModifiers->gainScalingPresent[0])
        {
            tmp = (int)(0.5f + 8.0f * gainModifiers->attenuationScaling[0]);
            err = putBits(bitstream, 4, tmp);
            if (err) return(err);
            tmp = (int)(0.5f + 8.0f * gainModifiers->amplificationScaling[0]);
            err = putBits(bitstream, 4, tmp);
            if (err) return(err);
        }
        err = putBits(bitstream, 1, gainModifiers->gainOffsetPresent[0]);
        if (err) return(err);
        if(gainModifiers->gainOffsetPresent[0])
        {
            if (gainModifiers->gainOffset[0] < 0.0f)
            {
                tmp = (int)(0.5f + max(0.0f, - 4.0f * gainModifiers->gainOffset[0] - 1.0f));
                sign = 1;
            }
            else
            {
                tmp = (int)(0.5f + max(0.0f, 4.0f * gainModifiers->gainOffset[0] - 1.0f));
                sign = 0;
            }
            err = putBits(bitstream, 1, sign);
            if (err) return(err);
            err = putBits(bitstream, 5, tmp);
            if (err) return(err);
        }
        
    }
    else if (version == 1) {
        for (b=0; b<bandCount; b++) {
            err = putBits(bitstream, 1, gainModifiers->targetCharacteristicLeftPresent[b]);
            if (err) return(err);
            
            if (gainModifiers->targetCharacteristicLeftPresent[b])
            {
                err = putBits(bitstream, 4, gainModifiers->targetCharacteristicLeftIndex[b]);
                if (err) return(err);
            }
            err = putBits(bitstream, 1, gainModifiers->targetCharacteristicRightPresent[b]);
            if (err) return(err);
            
            if (gainModifiers->targetCharacteristicRightPresent[b])
            {
                err = putBits(bitstream, 4, gainModifiers->targetCharacteristicRightIndex[b]);
                if (err) return(err);
            }
            err = putBits(bitstream, 1, gainModifiers->gainScalingPresent[b]);
            if (err) return(err);
            
            if(gainModifiers->gainScalingPresent[b])
            {
                tmp = (int)(0.5f + 8.0f * gainModifiers->attenuationScaling[b]);
                err = putBits(bitstream, 4, tmp);
                if (err) return(err);
                tmp = (int)(0.5f + 8.0f * gainModifiers->amplificationScaling[b]);
                err = putBits(bitstream, 4, tmp);
                if (err) return(err);
            }
            err = putBits(bitstream, 1, gainModifiers->gainOffsetPresent[b]);
            if (err) return(err);
            if(gainModifiers->gainOffsetPresent[b])
            {
                if (gainModifiers->gainOffset[b] < 0.0f)
                {
                    tmp = (int)(0.5f + max(0.0f, - 4.0f * gainModifiers->gainOffset[b] - 1.0f));
                    sign = 1;
                }
                else
                {
                    tmp = (int)(0.5f + max(0.0f, 4.0f * gainModifiers->gainOffset[b] - 1.0f));
                    sign = 0;
                }
                err = putBits(bitstream, 1, sign);
                if (err) return(err);
                err = putBits(bitstream, 5, tmp);
                if (err) return(err);
            }
        }
        if (bandCount == 1)
        {
            err = putBits(bitstream, 1, gainModifiers->shapeFilterPresent);
            if (err) return(err);
            
            if (gainModifiers->shapeFilterPresent)
            {
                err = putBits(bitstream, 4, gainModifiers->shapeFilterIndex);
                if (err) return(err);
            }
        }
    }
#else /* !AMD1_SYNTAX */
    err = putBits(bitstream, 1, gainModifiers->gainScalingPresent[0]);
    if (err) return(err);
    
    if(gainModifiers->gainScalingPresent[0])
    {
        tmp = (int)(0.5f + 8.0f * gainModifiers->attenuationScaling[0]);
        err = putBits(bitstream, 4, tmp);
        if (err) return(err);
        tmp = (int)(0.5f + 8.0f * gainModifiers->amplificationScaling[0]);
        err = putBits(bitstream, 4, tmp);
        if (err) return(err);
    }
    err = putBits(bitstream, 1, gainModifiers->gainOffsetPresent[0]);
    if (err) return(err);
    if(gainModifiers->gainOffsetPresent[0])
    {
        if (gainModifiers->gainOffset[0] < 0.0f)
        {
            tmp = (int)(0.5f + max(0.0f, - 4.0f * gainModifiers->gainOffset[0] - 1.0f));
            sign = 1;
        }
        else
        {
            tmp = (int)(0.5f + max(0.0f, 4.0f * gainModifiers->gainOffset[0] - 1.0f));
            sign = 0;
        }
        err = putBits(bitstream, 1, sign);
        if (err) return(err);
        err = putBits(bitstream, 5, tmp);
        if (err) return(err);
    }
#endif /* AMD1_SYNTAX */
    return (0);
}

int
writeLoudnessMeasure(wobitbufHandle bitstream,
                     LoudnessMeasure* loudnessMeasure)
{
    int err, code, codeSize;
    
    err = putBits(bitstream, 4, loudnessMeasure->methodDefinition);
    if (err) return(err);
    
    err = encMethodValue(loudnessMeasure->methodDefinition,
                         loudnessMeasure->methodValue,
                         &codeSize,
                         &code);
    if (err) return(err);
    
    err = putBits(bitstream, codeSize, code);
    if (err) return(err);
    err = putBits(bitstream, 4, loudnessMeasure->measurementSystem);
    if (err) return(err);
    err = putBits(bitstream, 2, loudnessMeasure->reliability);
    if (err) return(err);
    return (0);
}

int
writeLoudnessInfo(wobitbufHandle bitstream,
                  const int version,
                  LoudnessInfo* loudnessInfo)
{
    int err, i, code, codeSize;
    
    err = putBits(bitstream, 6, loudnessInfo->drcSetId);
    if (err) return(err);
#if AMD1_SYNTAX
    if (version >= 1) {
        err = putBits(bitstream, 6, loudnessInfo->eqSetId);
        if (err) return(err);
    }
#endif
    err = putBits(bitstream, 7, loudnessInfo->downmixId);
    if (err) return(err);
    
    err = putBits(bitstream, 1, loudnessInfo->samplePeakLevelPresent);
    if (err) return(err);
    if (loudnessInfo->samplePeakLevelPresent) {
        err = encPeak(loudnessInfo->samplePeakLevel, &code, &codeSize);
        if (err) return(err);
        
        err = putBits(bitstream, codeSize, code);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, loudnessInfo->truePeakLevelPresent);
    if (err) return(err);
    if (loudnessInfo->truePeakLevelPresent) {
        err = encPeak(loudnessInfo->truePeakLevel, &code, &codeSize);
        if (err) return(err);
        
        err = putBits(bitstream, codeSize, code);
        if (err) return(err);
        err = putBits(bitstream, 4, loudnessInfo->truePeakLevelMeasurementSystem);
        if (err) return(err);
        err = putBits(bitstream, 2, loudnessInfo->truePeakLevelReliability);
        if (err) return(err);
    }

    err = putBits(bitstream, 4, loudnessInfo->measurementCount);
    if (err) return(err);
    
    for (i=0; i<loudnessInfo->measurementCount; i++)
    {
        err = writeLoudnessMeasure(bitstream, &(loudnessInfo->loudnessMeasure[i]));
        if (err) return(err);
    }
   
    return(0);
}

static int
writeDrcInstructionsBasic(wobitbufHandle bitstream,
                          DrcInstructionsBasic* drcInstructionsBasic)
{
    int err, i, tmp;
    
    err = putBits(bitstream, 6, drcInstructionsBasic->drcSetId);
    if (err) return(err);
    err = putBits(bitstream, 4, drcInstructionsBasic->drcLocation);
    if (err) return(err);
    err = putBits(bitstream, 7, drcInstructionsBasic->downmixId);
    if (err) return(err);
    err = putBits(bitstream, 1, drcInstructionsBasic->additionalDownmixIdPresent);
    if (err) return(err);
    if (drcInstructionsBasic->additionalDownmixIdPresent) {
        err = putBits(bitstream, 3, drcInstructionsBasic->additionalDownmixIdCount);
        if (err) return(err);
        for(i=0; i<drcInstructionsBasic->additionalDownmixIdCount; i++)
        {
            err = putBits(bitstream, 7, drcInstructionsBasic->additionalDownmixId[i]);
            if (err) return(err);
        }
    } else {
        drcInstructionsBasic->additionalDownmixIdCount = 0;
    }

    err = putBits(bitstream, 16, drcInstructionsBasic->drcSetEffect);
    if (err) return(err);

    if ((drcInstructionsBasic->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) == 0)
    {
        err = putBits(bitstream, 1, drcInstructionsBasic->limiterPeakTargetPresent);
        if (err) return(err);
        if (drcInstructionsBasic->limiterPeakTargetPresent)
        {
            tmp = (int)(0.5f - 8.0f * drcInstructionsBasic->limiterPeakTarget);
            tmp = max (0, tmp);
            tmp = min (0xFF, tmp);
            err = putBits(bitstream, 8, tmp);
            if (err) return(err);
        }
    }

    err = putBits(bitstream, 1, drcInstructionsBasic->drcSetTargetLoudnessPresent);
    if (err) return(err);
    if (drcInstructionsBasic->drcSetTargetLoudnessPresent == 1)
    {
        err = putBits(bitstream, 6, drcInstructionsBasic->drcSetTargetLoudnessValueUpper+63);
        if (err) return(err);
        err = putBits(bitstream, 1, drcInstructionsBasic->drcSetTargetLoudnessValueLowerPresent);
        if (err) return(err);
        if (drcInstructionsBasic->drcSetTargetLoudnessValueLowerPresent == 1)
        {
            err = putBits(bitstream, 6, drcInstructionsBasic->drcSetTargetLoudnessValueLower+63);
            if (err) return(err);
        }
    }

    return(0);
}

static int
writeDrcInstructionsUniDrc(wobitbufHandle bitstream,
                           const int version,
                           UniDrcConfig* uniDrcConfig,
                           DrcEncParams* encParams,
                           DrcInstructionsUniDrc* drcInstructionsUniDrc)
{
    int err, c, g, i, n, k, tmp, channelCount;
    int uniqueIndex[CHANNEL_COUNT_MAX];
    float uniqueScaling[CHANNEL_COUNT_MAX];
    int match;
    int bsSequenceIndex, sequenceIndexPrev, repeatSequenceCount;
    int duckingSequence, idx;
    int repeatParametersCount;
    DuckingModifiers* duckingModifiers;
    float duckingScalingQuantizedPrev, factor;
    
    err = putBits(bitstream, 6, drcInstructionsUniDrc->drcSetId);
    if (err) return(err);
#if AMD1_SYNTAX
    if (version == 1) {
        err = getDrcComplexityLevel(uniDrcConfig, encParams, drcInstructionsUniDrc);
        if (err) return(err);

        err = putBits(bitstream, 4, drcInstructionsUniDrc->drcSetComplexityLevel);
        if (err) return(err);
    }
#endif /* AMD1_SYNTAX */
    err = putBits(bitstream, 4, drcInstructionsUniDrc->drcLocation);
    if (err) return(err);
#if AMD1_SYNTAX
    if (version == 1) {
        err = putBits(bitstream, 1, drcInstructionsUniDrc->downmixIdPresent);
        if (err) return(err);
    } else {
        drcInstructionsUniDrc->downmixIdPresent = 1;
    }
    if (drcInstructionsUniDrc->downmixIdPresent == 1)
#endif /* AMD1_SYNTAX */
    {
		err = putBits(bitstream, 7, drcInstructionsUniDrc->downmixId);
		if (err) return(err);
#if AMD1_SYNTAX
        if (version == 1) {
            err = putBits(bitstream, 1, drcInstructionsUniDrc->drcApplyToDownmix);
            if (err) return(err);
        }
#endif /* AMD1_SYNTAX */
		err = putBits(bitstream, 1, drcInstructionsUniDrc->additionalDownmixIdPresent);
		if (err) return(err);
		if (drcInstructionsUniDrc->additionalDownmixIdPresent) {
			err = putBits(bitstream, 3, drcInstructionsUniDrc->additionalDownmixIdCount);
			if (err) return(err);
			for(i=0; i<drcInstructionsUniDrc->additionalDownmixIdCount; i++)
			{
				err = putBits(bitstream, 7, drcInstructionsUniDrc->additionalDownmixId[i]);
				if (err) return(err);
			}
		} else {
			drcInstructionsUniDrc->additionalDownmixIdCount = 0;
		}
    }
#if AMD1_SYNTAX
    else {
        drcInstructionsUniDrc->downmixId = 0;
    }
#endif /* AMD1_SYNTAX */
    err = putBits(bitstream, 16, drcInstructionsUniDrc->drcSetEffect);
    if (err) return(err);
    if ((drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) == 0)
    {
        err = putBits(bitstream, 1, drcInstructionsUniDrc->limiterPeakTargetPresent);
        if (err) return(err);
        if (drcInstructionsUniDrc->limiterPeakTargetPresent)
        {
            tmp = (int)(0.5f - 8.0f * drcInstructionsUniDrc->limiterPeakTarget);
            tmp = max (0, tmp);
            tmp = min (0xFF, tmp);
            err = putBits(bitstream, 8, tmp);
            if (err) return(err);
        }
    }

    err = putBits(bitstream, 1, drcInstructionsUniDrc->drcSetTargetLoudnessPresent);
    if (err) return(err);
    if (drcInstructionsUniDrc->drcSetTargetLoudnessPresent == 1)
    {
        err = putBits(bitstream, 6, drcInstructionsUniDrc->drcSetTargetLoudnessValueUpper+63);
        if (err) return(err);
        err = putBits(bitstream, 1, drcInstructionsUniDrc->drcSetTargetLoudnessValueLowerPresent);
        if (err) return(err);
        if (drcInstructionsUniDrc->drcSetTargetLoudnessValueLowerPresent == 1)
        {
            err = putBits(bitstream, 6, drcInstructionsUniDrc->drcSetTargetLoudnessValueLower+63);
            if (err) return(err);
        }
    }

    err = putBits(bitstream, 1, drcInstructionsUniDrc->dependsOnDrcSetPresent);
    if (err) return(err);
 
    if (drcInstructionsUniDrc->dependsOnDrcSetPresent) {
        err = putBits(bitstream, 6, drcInstructionsUniDrc->dependsOnDrcSet);
        if (err) return(err);
    }
    else
    {
        err = putBits(bitstream, 1, drcInstructionsUniDrc->noIndependentUse);
        if (err) return(err);
    }

#if AMD1_SYNTAX
    if (version == 1) {
        err = putBits(bitstream, 1, drcInstructionsUniDrc->requiresEq);
        if (err) return(err);
    }
#endif /* AMD1_SYNTAX */
    channelCount = uniDrcConfig->channelLayout.baseChannelCount;
    
    if (drcInstructionsUniDrc->drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF))
    {
        i=0;
        while (i<channelCount)
        {
            duckingModifiers = drcInstructionsUniDrc->duckingModifiersForChannel;
            bsSequenceIndex = drcInstructionsUniDrc->gainSetIndex[i] + 1;
            err = putBits(bitstream, 6, bsSequenceIndex);
            if (err) return (err);
            
            err = encDuckingModifiers(bitstream, &(duckingModifiers[i]));
            if (err) return (err);
            duckingScalingQuantizedPrev = duckingModifiers[i].duckingScalingQuantized;
            sequenceIndexPrev = drcInstructionsUniDrc->gainSetIndex[i];

            i++;
            if (i < channelCount) 
            {
                err = quantizeDuckingScaling(&(duckingModifiers[i]));
                if (err) return (err);
            }
            repeatParametersCount = 0;
            while ((i < channelCount) && (repeatParametersCount <= 32) &&
                   (sequenceIndexPrev == drcInstructionsUniDrc->gainSetIndex[i]) &&
                   (duckingScalingQuantizedPrev == duckingModifiers[i].duckingScalingQuantized))
            {
                repeatParametersCount++;
                i++;
                if (i < channelCount) 
                {
                    err = quantizeDuckingScaling(&(duckingModifiers[i]));
                    if (err) return (err);
                }
            }
            if (repeatParametersCount > 0)
            {
                err = putBits(bitstream, 1, 1);  /* set repeatParameters to 1 */
                if (err) return(err);
                err = putBits(bitstream, 5, repeatParametersCount-1);  /* set repeatParametersCount */
                if (err) return(err);
            }
            else
            {
                err = putBits(bitstream, 1, 0);  /* set repeatParameters to 0 */
                if (err) return(err);
            }
        }

        for (c=0; c<CHANNEL_COUNT_MAX; c++)
        {
            uniqueIndex[c] = -10;
            uniqueScaling[c] = -10.0f;
        }
        duckingSequence = -1;
        g = 0;
        if (drcInstructionsUniDrc->drcSetEffect & EFFECT_BIT_DUCK_OTHER) {
            for (c=0; c<channelCount; c++)
            {
                match = FALSE;
                idx = drcInstructionsUniDrc->gainSetIndex[c];
                factor = drcInstructionsUniDrc->duckingModifiersForChannel[c].duckingScaling;
                for (n=0; n<g; n++) {
                    if (((idx >= 0) && (uniqueIndex[n] == idx)) || ((idx < 0) && (uniqueScaling[n] == factor)))
                    {
                        /* channel belongs to existing group */
                        match = TRUE;
                        break;
                    }
                }
                if (match == FALSE) {
                    if (idx < 0)
                    {
                        /* channel belongs to new group */
                        uniqueIndex[g] = idx;
                        uniqueScaling[g] = factor;
                        g++;
                    }
                    else
                    {
                        /* this group contains the ducking gain sequence that must be applied to all other groups */
                        if ((duckingSequence > 0) && (duckingSequence != idx))
                        {
                            fprintf(stderr, "ERROR: DRC for ducking can only have one ducking gain sequence.\n");
                            return(UNEXPECTED_ERROR);
                        }
                        duckingSequence = idx;
                    }
                }
            }
            drcInstructionsUniDrc->nDrcChannelGroups = g;
            if (duckingSequence < 0)
            {
                fprintf(stderr, "ERROR: Ducking sequence expected but not found.\n");
                return(UNEXPECTED_ERROR);
            }
        } else if (drcInstructionsUniDrc->drcSetEffect & EFFECT_BIT_DUCK_SELF) {
            for (c=0; c<channelCount; c++)
            {
                match = FALSE;
                idx = drcInstructionsUniDrc->gainSetIndex[c];
                factor = drcInstructionsUniDrc->duckingModifiersForChannel[c].duckingScaling;
                for (n=0; n<g; n++) {
                    if ((idx >= 0) && (uniqueIndex[n] == idx) && (uniqueScaling[n] == factor))
                    {
                        /* channel belongs to existing group */
                        match = TRUE;
                        break;
                    }
                }
                if (match == FALSE) {
                    if (idx >= 0)
                    {
                        /* channel belongs to new group */
                        uniqueIndex[g] = idx;
                        uniqueScaling[g] = factor;
                        g++;
                    }
                    else
                    {
                        /* this group contains the overlay content */
                    }
                }
            }
            drcInstructionsUniDrc->nDrcChannelGroups = g;
        }
        
        for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
        {
            if (drcInstructionsUniDrc->drcSetEffect & EFFECT_BIT_DUCK_OTHER) {
                drcInstructionsUniDrc->gainSetIndexForChannelGroup[g] = -1;
            } else if (drcInstructionsUniDrc->drcSetEffect & EFFECT_BIT_DUCK_SELF) {
                drcInstructionsUniDrc->gainSetIndexForChannelGroup[g] = uniqueIndex[g];
            }
            drcInstructionsUniDrc->duckingModifiersForChannelGroup[g].duckingScaling = uniqueScaling[g];
            if (uniqueScaling[g] != 1.0f)
            {
                drcInstructionsUniDrc->duckingModifiersForChannelGroup[g].duckingScalingPresent = TRUE;
            }
            else
            {
                drcInstructionsUniDrc->duckingModifiersForChannelGroup[g].duckingScalingPresent = FALSE;
            }
        }
    }
    else
    {
        if (
#if AMD1_SYNTAX
            (version == 0 || drcInstructionsUniDrc->drcApplyToDownmix != 0) &&
#endif /* AMD1_SYNTAX */
            (drcInstructionsUniDrc->downmixId != 0) && (drcInstructionsUniDrc->downmixId != 0x7F) && (drcInstructionsUniDrc->additionalDownmixIdCount == 0))
        {
#if AMD2_COR3
            channelCount = -1;
            for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
            {
                if (drcInstructionsUniDrc->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) 
                {
                    channelCount = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /* targetChannelCountFromDownmixId */
                    break;
                }
            }
#if AMD1_SYNTAX
            if (i == uniDrcConfig->downmixInstructionsCount)
            {
                if (uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1Present) 
                {
                    for (i=0; i<uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1Count; i++)
                    {
                        if (drcInstructionsUniDrc->downmixId == uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1[i].downmixId) 
                        {
                            channelCount = uniDrcConfig->uniDrcConfigExt.downmixInstructionsV1[i].targetChannelCount;  /* targetChannelCountFromDownmixId */
                            break;
                        }
                    }
                }
            }
#endif /* AMD1_SYNTAX */
            if (channelCount == -1)
            {
                fprintf(stderr,"ERROR: downmix ID %d not found\n", drcInstructionsUniDrc->downmixId);
                return(UNEXPECTED_ERROR);
            }
#else
            for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
            {
                if (drcInstructionsUniDrc->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) break;
            }
            if (i == uniDrcConfig->downmixInstructionsCount)
            {
                fprintf(stderr,"ERROR: downmix ID not found\n");
                return(UNEXPECTED_ERROR);
            }
            channelCount = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /*  targetChannelCountFromDownmixId*/
#endif /* AMD2_COR3 */
        }
        else if (
#if AMD1_SYNTAX
            (version == 0 || drcInstructionsUniDrc->drcApplyToDownmix != 0) &&
#endif /* AMD1_SYNTAX */
            ((drcInstructionsUniDrc->downmixId == 0x7F) || (drcInstructionsUniDrc->additionalDownmixIdCount != 0)))
        {
            channelCount = 1;
        }
        
        i=0;
        while (i<channelCount)
        {
            bsSequenceIndex = drcInstructionsUniDrc->gainSetIndex[i] + 1;
            err = putBits(bitstream, 6, bsSequenceIndex);
            if (err) return (err);
            sequenceIndexPrev = drcInstructionsUniDrc->gainSetIndex[i];
            i++;
            repeatSequenceCount = 0;
            while ((i < channelCount) && (sequenceIndexPrev == drcInstructionsUniDrc->gainSetIndex[i]) && (repeatSequenceCount < 32))
            {
                repeatSequenceCount++;
                i++;
            }
            if (repeatSequenceCount > 0)
            {
                err = putBits(bitstream, 1, 1);  /* set repeatSequenceIndex to 1 */
                if (err) return(err);
                err = putBits(bitstream, 5, repeatSequenceCount-1);  /* set repeatSequenceIndex */
                if (err) return(err);
            }
            else
            {
                err = putBits(bitstream, 1, 0);  /* set repeatSequenceIndex to 0 */
                if (err) return(err);
            }
        }
        for (i=0; i<CHANNEL_COUNT_MAX; i++)
        {
            uniqueIndex[i] = -1;
        }
        k = 0;
        for (i=0; i<channelCount; i++)
        {
            int tmp = drcInstructionsUniDrc->gainSetIndex[i];
            if (tmp>=0)
            {
                match = FALSE;
                for (n=0; n<k; n++)
                {
                    if (uniqueIndex[n] == tmp) match = TRUE;
                }
                if (match == FALSE)
                {
                    uniqueIndex[k] = tmp;
                    k++;
                }
            }
        }
        drcInstructionsUniDrc->nDrcChannelGroups = k;
        for (i=0; i<drcInstructionsUniDrc->nDrcChannelGroups; i++)
        {
            /* the DRC channel groups are ordered according to increasing channel indices */
            /* Whenever a new gainSetIndex appears the channel group index is incremented */
            /* The same order has to be observed for all subsequent processing */
            int bandCount = 0;
            
            drcInstructionsUniDrc->gainSetIndexForChannelGroup[i] = uniqueIndex[i];
            /* retrieve band count */
            /* TODO: if DrcCoefficients for V0 and V1 are present, it must be know where the gain set is located */
            if (uniDrcConfig->drcCoefficientsUniDrcCount > 0)
            {
                if (uniDrcConfig->drcCoefficientsUniDrc[0].gainSetCount <= drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]) {
                    bandCount = 1;
                }
                else {
                    bandCount = uniDrcConfig->drcCoefficientsUniDrc[0].gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]].bandCount;
                }
            }
#if AMD1_SYNTAX
            else if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0)
            {
                if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetCount <= drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]) {
                    bandCount = 1;
                }
                else {
                    bandCount = uniDrcConfig->uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]].bandCount;
                }
            }
            else {
                bandCount = 1; /* only parametric DRCs in payload */
            }
#endif /* AMD1_SYNTAX */
            err = encGainModifiers(bitstream, version, bandCount, &(drcInstructionsUniDrc->gainModifiers[i]));
            if (err) return (err);
        }
    }
    return(0);
}

#ifdef LEVELING_SUPPORT
static int getNumDuckingOnlyDrcSets(DrcInstructionsUniDrc const* drcInstructionsUniDrc, unsigned int drcInstructionsUniDrcCount)
{
    unsigned int num_ducking_only_drc_sets = 0;
    for(int i = 0; i < drcInstructionsUniDrcCount; ++i)
    {
        num_ducking_only_drc_sets += !!drcInstructionsUniDrc[i].duckingOnlyDrcSet;
    }

    return num_ducking_only_drc_sets;
}

static int writeLoudnessLevelingExtension(wobitbufHandle bitstream, UniDrcConfig* uniDrcConfig, DrcEncParams* encParams)
{
#if MPEG_H_SYNTAX
    int const drcInstructionsUniDrcCount = uniDrcConfig->drcInstructionsUniDrcCount;
    int const version = 0;
#else
    int const drcInstructionsUniDrcCount = uniDrcConfig->uniDrcConfigExt.drcInstructionsUniDrcV1Count;
    int const version = 1;
#endif

    for (int i = 0; i < drcInstructionsUniDrcCount; i++) {

#if MPEG_H_SYNTAX
        DrcInstructionsUniDrc* instr = &uniDrcConfig->drcInstructionsUniDrc[i];
#else
        DrcInstructionsUniDrc* instr = &uniDrcConfig->uniDrcConfigExt.drcInstructionsUniDrcV1[i];
#endif

        if (!(instr->drcSetEffect & (1 << 11))) {
            continue;
        }

        int err = putBits(bitstream, 1, instr->levelingPresent);
        if (err) return err;

        if (instr->levelingPresent) {
            /* If leveling is present, the next DRC set may be a ducking-only set */
            if (i < drcInstructionsUniDrcCount - 1)
            {
#if MPEG_H_SYNTAX
                instr = &uniDrcConfig->drcInstructionsUniDrc[++i];
#else
                instr = &uniDrcConfig->uniDrcConfigExt.drcInstructionsUniDrcV1[++i];
#endif
                err = putBits(bitstream, 1, instr->duckingOnlyDrcSet);
                if (err) return err;

                if (instr->duckingOnlyDrcSet) {
                    err = writeDrcInstructionsUniDrc(bitstream, version, uniDrcConfig, encParams, instr);
                    if (err) return(err);
                }
            }
            else
            {
                /* No ducking only DRC set */
                err = putBits(bitstream, 1, 0);
                if (err) return err;
            }
        }
    }

    return 0;
}
#endif

int
writeGainParams(wobitbufHandle bitstream,
                const int version,
                const int bandCount,
                const int drcBandType,
                GainParams gainParams[])
{
    int err, i;
#if AMD1_SYNTAX
    if (version == 1)
    {
        int indexPresent;
        int gainSequenceIndexLast = -100;
        for (i=0; i<bandCount; i++)
        {
            if (gainParams[i].gainSequenceIndex == gainSequenceIndexLast + 1) {
                indexPresent = 0;
            }
            else
            {
                indexPresent = 1;
            }
            err = putBits(bitstream, 1, indexPresent);
            if (err) return(err);
            if (indexPresent==1) {
                err = putBits(bitstream, 6, gainParams[i].gainSequenceIndex);
                if (err) return(err);
                gainSequenceIndexLast = gainParams[i].gainSequenceIndex;
            }
            
            err = putBits(bitstream, 1, gainParams[i].drcCharacteristicPresent);
            if (err) return(err);
            if (gainParams[i].drcCharacteristicPresent)
            {
                err = putBits(bitstream, 1, gainParams[i].drcCharacteristicFormatIsCICP);
                if (err) return(err);
                if (gainParams[i].drcCharacteristicFormatIsCICP==1)
                {
                    err = putBits(bitstream, 7, gainParams[i].drcCharacteristic);
                    if (err) return(err);
                }
                else
                {
                    err = putBits(bitstream, 4, gainParams[i].drcCharacteristicLeftIndex);
                    if (err) return(err);
                    err = putBits(bitstream, 4, gainParams[i].drcCharacteristicRightIndex);
                    if (err) return(err);
                }
            }
        }
    } else
#endif /* AMD1_SYNTAX */
    {
        for(i=0; i<bandCount; i++)
        {
            err = putBits(bitstream, 7, gainParams[i].drcCharacteristic);
            if (err) return(err);
        }
    }
    for(i=1; i<bandCount; i++)
    {
        if (drcBandType) {
            err = putBits(bitstream, 4, gainParams[i].crossoverFreqIndex);
            if (err) return(err);
        } else {
            err = putBits(bitstream, 10, gainParams[i].startSubBandIndex);
            if (err) return(err);
        }
        if (err) return(err);
    }
    return(0);
}

int
writeGainSetParams(wobitbufHandle bitstream,
                   const int version,
                   GainSetParams* gainSetParams)
{
    int err;
    
    err = putBits(bitstream, 2, gainSetParams->gainCodingProfile);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->gainInterpolationType);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->fullFrame);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->timeAlignment);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->timeDeltaMinPresent);
    if (err) return(err);
    if (gainSetParams->timeDeltaMinPresent) {
        err = putBits(bitstream, 11, gainSetParams->deltaTmin-1);
        if (err) return(err);
    }
    if (gainSetParams->gainCodingProfile != GAIN_CODING_PROFILE_CONSTANT)
    {
        err = putBits(bitstream, 4, gainSetParams->bandCount);
        if (err) return(err);
        if (gainSetParams->bandCount>1) {
            err = putBits(bitstream, 1, gainSetParams->drcBandType);
            if (err) return(err);
        }
        err = writeGainParams(bitstream, version, gainSetParams->bandCount, gainSetParams->drcBandType, gainSetParams->gainParams);
        if (err) return(err);
    }
    return(0);
}

static int
writeDrcCoefficientsBasic(wobitbufHandle bitstream,
                          DrcCoefficientsBasic* drcCoefficientsBasic)
{
    int err;
    
    err = putBits(bitstream, 4, drcCoefficientsBasic->drcLocation);
    if (err) return(err);
    err = putBits(bitstream, 7, drcCoefficientsBasic->drcCharacteristic);
    if (err) return(err);

    return(0);
}

#if AMD1_SYNTAX
static int writeSplitDrcChracateristic(wobitbufHandle bitstream,
                                       const int side,
                                       SplitDrcCharacteristic* splitCharacteristic)
{
    int err, i;
    err = putBits(bitstream, 1, splitCharacteristic->characteristicFormat);
    if (err) return(err);
    if (splitCharacteristic->characteristicFormat == 0) {
        err = putBits(bitstream, 6, splitCharacteristic->bsGain);
        if (err) return(err);
        err = putBits(bitstream, 4, splitCharacteristic->bsIoRatio);
        if (err) return(err);
        err = putBits(bitstream, 4, splitCharacteristic->bsExp);
        if (err) return(err);
        err = putBits(bitstream, 1, splitCharacteristic->flipSign);
        if (err) return(err);
    }
    else
    {
        float bsNodeLevelPrevious = DRC_INPUT_LOUDNESS_TARGET;
        int bsNodeLevelDelta;
        int bsNodeGain;
        err = putBits(bitstream, 2, splitCharacteristic->characteristicNodeCount-1);
        if (err) return(err);
        for (i=1; i<=splitCharacteristic->characteristicNodeCount; i++) {
            bsNodeLevelDelta = floor (fabs(splitCharacteristic->nodeLevel[i] - bsNodeLevelPrevious) + 0.5f) - 1;
            if (bsNodeLevelDelta < 0) bsNodeLevelDelta = 0;
            if (bsNodeLevelDelta > 31) bsNodeLevelDelta = 31;
            if (side == LEFT_SIDE) {
                bsNodeLevelPrevious = bsNodeLevelPrevious - (bsNodeLevelDelta + 1);
            }
            else {
                bsNodeLevelPrevious = bsNodeLevelPrevious + (bsNodeLevelDelta + 1);
            }
            err = putBits(bitstream, 5, bsNodeLevelDelta);
            if (err) return(err);
            bsNodeGain = floor ((splitCharacteristic->nodeGain[i] + 64.0f) * 2.0f + 0.5f);
            if (bsNodeGain < 0) bsNodeGain = 0;
            if (bsNodeGain > 255) bsNodeGain = 255;
            err = putBits(bitstream, 8, bsNodeGain);
            if (err) return(err);
        }
    }
    return (0);
}

static int writeShapeFilterBlockParams(wobitbufHandle bitstream, ShapeFilterBlockParams* shapeFilterBlockParams)
{
    int err;
    
    err = putBits(bitstream, 1, shapeFilterBlockParams->lfCutFilterPresent);
    if (err) return(err);
    if (shapeFilterBlockParams->lfCutFilterPresent == 1) {
        err = putBits(bitstream, 3, shapeFilterBlockParams->lfCutParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->lfCutParams.filterStrengthIndex);
        if (err) return(err);
    }
    err = putBits(bitstream, 1, shapeFilterBlockParams->lfBoostFilterPresent);
    if (err) return(err);
    if (shapeFilterBlockParams->lfBoostFilterPresent == 1) {
        err = putBits(bitstream, 3, shapeFilterBlockParams->lfBoostParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->lfBoostParams.filterStrengthIndex);
        if (err) return(err);
    }
    err = putBits(bitstream, 1, shapeFilterBlockParams->hfCutFilterPresent);
    if (err) return(err);
    if (shapeFilterBlockParams->hfCutFilterPresent == 1) {
        err = putBits(bitstream, 3, shapeFilterBlockParams->hfCutParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->hfCutParams.filterStrengthIndex);
        if (err) return(err);
    }
    err = putBits(bitstream, 1, shapeFilterBlockParams->hfBoostFilterPresent);
    if (err) return(err);
    if (shapeFilterBlockParams->hfBoostFilterPresent == 1) {
        err = putBits(bitstream, 3, shapeFilterBlockParams->hfBoostParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->hfBoostParams.filterStrengthIndex);
        if (err) return(err);
    }
    return (0);
}
#endif  /* AMD1_SYNTAX */

static int
writeDrcCoefficientsUniDrc(wobitbufHandle bitstream,
                           const int version,
                           DrcCoefficientsUniDrc* drcCoefficientsUniDrc)
{
    int err, i;
    
    err = putBits(bitstream, 4, drcCoefficientsUniDrc->drcLocation);
    if (err) return(err);
    
    err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcFrameSizePresent);
    if (err) return(err);
    
    if(drcCoefficientsUniDrc->drcFrameSizePresent)
    {
        err = putBits(bitstream, 15, (drcCoefficientsUniDrc->drcFrameSize-1));
        if (err) return(err);
    }
#if AMD1_SYNTAX
    if (version == 1) {
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcCharacteristicLeftPresent);
        if (err) return(err);
        if(drcCoefficientsUniDrc->drcCharacteristicLeftPresent)
        {
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->characteristicLeftCount);
            if (err) return(err);
            for (i=1; i<=drcCoefficientsUniDrc->characteristicLeftCount; i++)
            {
                err = writeSplitDrcChracateristic(bitstream, LEFT_SIDE, &drcCoefficientsUniDrc->splitCharacteristicLeft[i]);
                if (err) return(err);
            }
        }
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcCharacteristicRightPresent);
        if (err) return(err);
        if(drcCoefficientsUniDrc->drcCharacteristicRightPresent)
        {
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->characteristicRightCount);
            if (err) return(err);
            for (i=1; i<=drcCoefficientsUniDrc->characteristicRightCount; i++)
            {
                err = writeSplitDrcChracateristic(bitstream, RIGHT_SIDE, &drcCoefficientsUniDrc->splitCharacteristicRight[i]);
                if (err) return(err);
            }
        }
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->shapeFiltersPresent);
        if (err) return(err);
        if(drcCoefficientsUniDrc->shapeFiltersPresent)
        {
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->shapeFilterCount);
            if (err) return(err);
            for (i=1; i<=drcCoefficientsUniDrc->shapeFilterCount; i++)
            {
                err = writeShapeFilterBlockParams(bitstream, &drcCoefficientsUniDrc->shapeFilterBlockParams[i]);
                if (err) return(err);
            }
        }
        err = putBits(bitstream, 6, drcCoefficientsUniDrc->gainSequenceCount);
        if (err) return(err);
        err = putBits(bitstream, 6, drcCoefficientsUniDrc->gainSetCount);
        if (err) return(err);
        for(i=0; i<drcCoefficientsUniDrc->gainSetCount; i++)
        {
            err = writeGainSetParams(bitstream, version, &(drcCoefficientsUniDrc->gainSetParams[i]));
            if (err) return (err);
        }
    } else
#endif /* !AMD1_SYNTAX */
    {
        err = putBits(bitstream, 6, drcCoefficientsUniDrc->gainSetCount);
        if (err) return(err);
        for(i=0; i<drcCoefficientsUniDrc->gainSetCount; i++)
        {
            err = writeGainSetParams(bitstream, version, &(drcCoefficientsUniDrc->gainSetParams[i]));
            if (err) return (err);
        }
    }
    return(0);
}

int
writeDownmixInstructions(wobitbufHandle bitstream,
                         const int version,
                         DrcEncParams* drcParams,
                         DownmixInstructions* downmixInstructions)
{
    int err, i, code, codeSize;
    
    err = putBits(bitstream, 7, downmixInstructions->downmixId);
    if (err) return(err);
    err = putBits(bitstream, 7, downmixInstructions->targetChannelCount);
    if (err) return(err);
    err = putBits(bitstream, 8, downmixInstructions->targetLayout);
    if (err) return(err);
    err = putBits(bitstream, 1, downmixInstructions->downmixCoefficientsPresent);
    if (err) return(err);
    
    if (downmixInstructions->downmixCoefficientsPresent)
    {
#if AMD1_SYNTAX
        if (version == 1) {
            err = writeDownmixCoefficientV1(bitstream,
                                            downmixInstructions->downmixCoefficient,
                                            drcParams->baseChannelCount,
                                            downmixInstructions->targetChannelCount);
            if (err) return(err);
        }
        else
#endif /* AMD1_SYNTAX */
        {
            /* TODO need LFE info */
            int isLfeChannel = FALSE;
            for (i=0; i<downmixInstructions->targetChannelCount * drcParams->baseChannelCount; i++)  /*???*/
            {
                err = encDownmixCoefficient(downmixInstructions->downmixCoefficient[i], isLfeChannel, &codeSize, &code);
                err = putBits(bitstream, codeSize, code);
                if (err) return(err);
            }
        }
    }
    return(0);
}

int
writeChannelLayout(wobitbufHandle bitstream,
                   ChannelLayout* channelLayout)
{
    int err, i;
    
    err = putBits(bitstream, 7, channelLayout->baseChannelCount);
    if (err) return(err);

#if !MPEG_H_SYNTAX
    err = putBits(bitstream, 1, channelLayout->layoutSignalingPresent);
    if (err) return(err);
    
    if (channelLayout->layoutSignalingPresent) {
        err = putBits(bitstream, 8, channelLayout->definedLayout);
        if (err) return(err);
        
        if (channelLayout->definedLayout == 0)
        {
            for (i=0; i<channelLayout->baseChannelCount; i++)
            {
                err = putBits(bitstream, 7, channelLayout->speakerPosition[i]);
                if (err) return(err);
            }
        }
    }
#endif
    
    return(0);
}

#if AMD1_SYNTAX
int
encChannelWeight(const float channelWeightLin,
                 int* codeSize,
                 int* code)
{
    int i;
    const float* channelWeightTable;
    float channelWeightDb;
    
    channelWeightDb = 20.0f * (float)log10(channelWeightLin);
    channelWeightTable = channelWeight;
    
    if (channelWeightDb < channelWeightTable[14])
    {
        *code = 15;
    }
    else
    {
        i = 0;
        while (channelWeightDb < channelWeightTable[i]) i++;
        if ((i>0) && (channelWeightDb > 0.5f * (channelWeightTable[i-1] + channelWeightTable[i]))) i--;
        *code = i;
    }
    *codeSize = 4;
    return (0);
}

int
writeParametricDrcGainSetParams(wobitbufHandle bitstream,
                                UniDrcConfig* uniDrcConfig,
                                ParametricDrcGainSetParams* parametricDrcGainSetParams)
{
    int err = 0, i = 0, codeSize = 0, code = 0;
    
    err = putBits(bitstream, 4, parametricDrcGainSetParams->parametricDrcId);
    if (err) return(err);
    
    err = putBits(bitstream, 3, parametricDrcGainSetParams->sideChainConfigType);
    if (err) return(err);
    
    if (parametricDrcGainSetParams->sideChainConfigType==1) {
        err = putBits(bitstream, 7, parametricDrcGainSetParams->downmixId);
        if (err) return(err);
        
        err = putBits(bitstream, 1, parametricDrcGainSetParams->levelEstimChannelWeightFormat);
        if (err) return(err);
        
        if (parametricDrcGainSetParams->downmixId == 0x0) {
            parametricDrcGainSetParams->channelCountFromDownmixId = uniDrcConfig->channelLayout.baseChannelCount;
        } else if (parametricDrcGainSetParams->downmixId == 0x7F) {
            parametricDrcGainSetParams->channelCountFromDownmixId = 1;
        } else {
            for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
            {
                if (parametricDrcGainSetParams->downmixId == uniDrcConfig->downmixInstructions[i].downmixId) break;
            }
            if (i == uniDrcConfig->downmixInstructionsCount)
            {
                fprintf(stderr,"ERROR: downmix ID not found\n");
                return(UNEXPECTED_ERROR);
            }
            parametricDrcGainSetParams->channelCountFromDownmixId = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /*  channelCountFromDownmixId*/
        }
        for (i=0; i<parametricDrcGainSetParams->channelCountFromDownmixId; i++) {
            if (parametricDrcGainSetParams->levelEstimChannelWeightFormat == 0) {
                if (parametricDrcGainSetParams->levelEstimChannelWeight[i] == 0) {
                    code = 0;
                } else {
                    code = 1;
                }
                err = putBits(bitstream, 1, code);
                if (err) return(err);
            } else {
                err = encChannelWeight(parametricDrcGainSetParams->levelEstimChannelWeight[i], &codeSize, &code);
                err = putBits(bitstream, codeSize, code);
                if (err) return(err);
            }
        }
    }
    
    err = putBits(bitstream, 1, parametricDrcGainSetParams->drcInputLoudnessPresent);
    if (err) return(err);
    
    if (parametricDrcGainSetParams->drcInputLoudnessPresent) {
        code = ((int) (0.5f + 4.0f * (parametricDrcGainSetParams->drcInputLoudness + 57.75f) + 10000.0f)) - 10000;
        code = min(0x0FF, code);
        code = max(0x0, code);
        err = putBits(bitstream, 8, code);
        if (err) return(err);
    }
    return 0;

}

int
writeParametricDrcTypeFeedForward(wobitbufHandle bitstream,
                                  int drcFrameSizeParametricDrc,
                                  ParametricDrcTypeFeedForward* parametricDrcTypeFeedForward)
{
    int err = 0, i = 0, code = 0;
    
    err = putBits(bitstream, 2, parametricDrcTypeFeedForward->levelEstimKWeightingType);
    if (err) return(err);
    
    err = putBits(bitstream, 1, parametricDrcTypeFeedForward->levelEstimIntegrationTimePresent);
    if (err) return(err);
    
    if (parametricDrcTypeFeedForward->levelEstimIntegrationTimePresent) {
        code = ((float)parametricDrcTypeFeedForward->levelEstimIntegrationTime / drcFrameSizeParametricDrc + 0.5f) - 1;
        err = putBits(bitstream, 6, code);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, parametricDrcTypeFeedForward->drcCurveDefinitionType);
    if (err) return(err);
    
    if (parametricDrcTypeFeedForward->drcCurveDefinitionType == 0) {
        err = putBits(bitstream, 7, parametricDrcTypeFeedForward->drcCharacteristic);
        if (err) return(err);
    } else {
        err = putBits(bitstream, 3, parametricDrcTypeFeedForward->nodeCount-2);
        if (err) return(err);
        
        for (i=0; i<parametricDrcTypeFeedForward->nodeCount; i++) {
            if (i==0) {
                code = -11-parametricDrcTypeFeedForward->nodeLevel[0];
                err = putBits(bitstream, 6, code);
                if (err) return(err);
            } else {
                code = parametricDrcTypeFeedForward->nodeLevel[i] - parametricDrcTypeFeedForward->nodeLevel[i-1] - 1;
                err = putBits(bitstream, 5, code);
                if (err) return(err);
            }
            code = parametricDrcTypeFeedForward->nodeGain[i]+39;
            err = putBits(bitstream, 6, code);
            if (err) return(err);
        }
    }
    
    err = putBits(bitstream, 1, parametricDrcTypeFeedForward->drcGainSmoothParametersPresent);
    if (err) return(err);
    
    if (parametricDrcTypeFeedForward->drcGainSmoothParametersPresent) {
        code = parametricDrcTypeFeedForward->gainSmoothAttackTimeSlow*0.2;
        err = putBits(bitstream, 8, code);
        if (err) return(err);
        
        code = parametricDrcTypeFeedForward->gainSmoothReleaseTimeSlow*0.025;
        err = putBits(bitstream, 8, code);
        if (err) return(err);
        
        err = putBits(bitstream, 1, parametricDrcTypeFeedForward->gainSmoothTimeFastPresent);
        if (err) return(err);
        
        if (parametricDrcTypeFeedForward->gainSmoothTimeFastPresent) {
            code = parametricDrcTypeFeedForward->gainSmoothAttackTimeFast*0.2;
            err = putBits(bitstream, 8, code);
            if (err) return(err);
            
            code = parametricDrcTypeFeedForward->gainSmoothReleaseTimeFast*0.05;
            err = putBits(bitstream, 8, code);
            if (err) return(err);
            
            err = putBits(bitstream, 1, parametricDrcTypeFeedForward->gainSmoothThresholdPresent);
            if (err) return(err);
            
            if (parametricDrcTypeFeedForward->gainSmoothThresholdPresent) {
                if (parametricDrcTypeFeedForward->gainSmoothAttackThreshold > 30) {
                    code = 31;
                } else {
                    code = parametricDrcTypeFeedForward->gainSmoothAttackThreshold;
                }
                err = putBits(bitstream, 5, code);
                if (err) return(err);
                
                if (parametricDrcTypeFeedForward->gainSmoothReleaseThreshold > 30) {
                    code = 31;
                } else {
                    code = parametricDrcTypeFeedForward->gainSmoothReleaseThreshold;
                }
                err = putBits(bitstream, 5, code);
                if (err) return(err);
            }
        }
        
        err = putBits(bitstream, 1, parametricDrcTypeFeedForward->gainSmoothHoldOffCountPresent);
        if (err) return(err);
        
        if (parametricDrcTypeFeedForward->gainSmoothHoldOffCountPresent) {
            err = putBits(bitstream, 7, parametricDrcTypeFeedForward->gainSmoothHoldOff);
            if (err) return(err);
        }
    }
    
    return 0;
}

#ifdef AMD1_PARAMETRIC_LIMITER
int
writeParametricDrcTypeLim(wobitbufHandle bitstream,
                          ParametricDrcTypeLim* parametricDrcTypeLim)
{
    int err = 0, tmp = 0;
    
    err = putBits(bitstream, 1, parametricDrcTypeLim->parametricLimThresholdPresent);
    if (err) return(err);
    
    if (parametricDrcTypeLim->parametricLimThresholdPresent) {
        tmp = (int)(0.5f - 8.0f * parametricDrcTypeLim->parametricLimThreshold);
        tmp = max (0, tmp);
        tmp = min (0xFF, tmp);
        err = putBits(bitstream, 8, tmp);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, parametricDrcTypeLim->parametricLimReleasePresent);
    if (err) return(err);
    
    if (parametricDrcTypeLim->parametricLimReleasePresent) {
        tmp = parametricDrcTypeLim->parametricLimRelease*0.1;
        err = putBits(bitstream, 8, tmp);
        if (err) return(err);
    }
    
    return 0;
}
#endif

int
writeParametricDrcInstructions(wobitbufHandle bitstream,
                               int drcFrameSizeParametricDrc,
                               DrcEncParams* drcParams,
                               UniDrcConfig* uniDrcConfig,
                               ParametricDrcInstructions* parametricDrcInstructions)
{
    int err = 0, bitSize = 0, lenSizeBits = 0, bitSizeLen = 0;
    
    err = putBits(bitstream, 4, parametricDrcInstructions->parametricDrcId);
    if (err) return(err);
    
    err = putBits(bitstream, 1, parametricDrcInstructions->parametricDrcLookAheadPresent);
    if (err) return(err);
    
    if (parametricDrcInstructions->parametricDrcLookAheadPresent) {
        err = putBits(bitstream, 7, parametricDrcInstructions->parametricDrcLookAhead);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, parametricDrcInstructions->parametricDrcPresetIdPresent);
    if (err) return(err);
    
    if (parametricDrcInstructions->parametricDrcPresetIdPresent) {
        err = putBits(bitstream, 7, parametricDrcInstructions->parametricDrcPresetId);
        if (err) return(err);
    } else {
        err = putBits(bitstream, 3, parametricDrcInstructions->parametricDrcType);
        if (err) return(err);
        
        if (parametricDrcInstructions->parametricDrcType == PARAM_DRC_TYPE_FF) {
            err = writeParametricDrcTypeFeedForward(bitstream, drcFrameSizeParametricDrc, &(parametricDrcInstructions->parametricDrcTypeFeedForward));
            if (err) return(err);
#ifdef AMD1_PARAMETRIC_LIMITER
        } else if (parametricDrcInstructions->parametricDrcType == PARAM_DRC_TYPE_LIM) {
            err = writeParametricDrcTypeLim(bitstream, &(parametricDrcInstructions->parametricDrcTypeLim));
            if (err) return(err);
#endif
        } else {
            bitSize     = parametricDrcInstructions->lenBitSize - 1;
            lenSizeBits = (int)(log((float)bitSize)/log(2.f))+1;
            bitSizeLen  = lenSizeBits - 4;
            err = putBits(bitstream, 4, bitSizeLen);
            if (err) return(err);
            err = putBits(bitstream, bitSize, bitSize);
            if (err) return(err);
            switch(parametricDrcInstructions->parametricDrcType)
            {
                /* add future extensions here */
                default:
                    fprintf(stderr,"ERROR: Unknown parametricDrcType\n");
                    return(UNEXPECTED_ERROR);
            }
        }
    }
    
    return 0;
}

int
writeDrcCoefficientsParametricDrc(wobitbufHandle bitstream,
                                  DrcEncParams* drcParams,
                                  UniDrcConfig* uniDrcConfig,
                                  DrcCoefficientsParametricDrc* drcCoefficientsParametricDrc)
{
    int err = 0, i = 0, bits = 0, mu = 0, nu = 0;
    float exp = 0.f;

    err = putBits(bitstream, 4, drcCoefficientsParametricDrc->drcLocation);
    if (err) return(err);
    
    exp = log(drcCoefficientsParametricDrc->parametricDrcFrameSize)/log(2);
    if (exp != (int)exp) {
        drcCoefficientsParametricDrc->parametricDrcFrameSizeFormat = 1;
    } else {
        drcCoefficientsParametricDrc->parametricDrcFrameSizeFormat = 0;
    }
    
    err = putBits(bitstream, 1, drcCoefficientsParametricDrc->parametricDrcFrameSizeFormat);
    if (err) return(err);
    
    if(drcCoefficientsParametricDrc->parametricDrcFrameSizeFormat)
    {
        err = putBits(bitstream, 15, (drcCoefficientsParametricDrc->parametricDrcFrameSize-1));
        if (err) return(err);
    } else {
        bits = (int)exp;
        err = putBits(bitstream, 4, bits);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, drcCoefficientsParametricDrc->parametricDrcDelayMaxPresent);
    if (err) return(err);
    if (drcCoefficientsParametricDrc->parametricDrcDelayMaxPresent == 1) {
        for (nu=0; nu<8; nu++) {
            mu = drcCoefficientsParametricDrc->parametricDrcDelayMax / (16 << nu);
            if (mu * (16 << nu) < drcCoefficientsParametricDrc->parametricDrcDelayMax) {
                mu++;
            }
            if (mu < 32) {
                break;
            }
        }
        if (nu == 8) {
            nu = 7;
            mu = 31;
        }
        /*printf("parametricDrcDelayMax (encoder) = %d, parametricDrcDelayMax (decoder) = %d\n",drcCoefficientsParametricDrc->parametricDrcDelayMax, (int)(16*mu*pow(2.f, nu)));*/
        err = putBits(bitstream, 5, mu);
        if (err) return(err);
        err = putBits(bitstream, 3, nu);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, drcCoefficientsParametricDrc->resetParametricDrc);
    if (err) return(err);
    
    err = putBits(bitstream, 6, drcCoefficientsParametricDrc->parametricDrcGainSetCount);
    if (err) return(err);
    
    for (i=0; i<drcCoefficientsParametricDrc->parametricDrcGainSetCount; i++) {
        err = writeParametricDrcGainSetParams(bitstream, uniDrcConfig, &(drcCoefficientsParametricDrc->parametricDrcGainSetParams[i]));
        if (err) return(err);
    }
    
    return(0);
}

int
writeLoudEqInstructions(wobitbufHandle bitstream,
                        DrcEncParams* drcParams,
                        LoudEqInstructions* loudEqInstructions)
{
    int err, i;
    
    err = putBits(bitstream, 4, loudEqInstructions->loudEqSetId);
    if (err) return(err);
    err = putBits(bitstream, 4, loudEqInstructions->drcLocation);
    if (err) return(err);
    err = putBits(bitstream, 1, loudEqInstructions->downmixIdPresent);
    if (err) return(err);
    if (loudEqInstructions->downmixIdPresent) {
        err = putBits(bitstream, 7, loudEqInstructions->downmixId);
        if (err) return(err);
        err = putBits(bitstream, 1, loudEqInstructions->additionalDownmixIdPresent);
        if (err) return(err);
        if (loudEqInstructions->additionalDownmixIdPresent) {
            err = putBits(bitstream, 7, loudEqInstructions->additionalDownmixIdCount);
            if (err) return(err);
            for(i=0; i<loudEqInstructions->additionalDownmixIdCount; i++)
            {
                err = putBits(bitstream, 7, loudEqInstructions->additionalDownmixId[i]);
                if (err) return(err);
            }
        } else {
            loudEqInstructions->additionalDownmixIdCount = 0;
        }
    }
    err = putBits(bitstream, 1, loudEqInstructions->drcSetIdPresent);
    if (err) return(err);
    if (loudEqInstructions->drcSetIdPresent) {
        err = putBits(bitstream, 6, loudEqInstructions->drcSetId);
        if (err) return(err);
        err = putBits(bitstream, 1, loudEqInstructions->additionalDrcSetIdPresent);
        if (err) return(err);
        if (loudEqInstructions->additionalDrcSetIdPresent) {
            err = putBits(bitstream, 6, loudEqInstructions->additionalDrcSetIdCount);
            if (err) return(err);
            for(i=0; i<loudEqInstructions->additionalDrcSetIdCount; i++)
            {
                err = putBits(bitstream, 6, loudEqInstructions->additionalDrcSetId[i]);
                if (err) return(err);
            }
        } else {
            loudEqInstructions->additionalDrcSetIdCount = 0;
        }
    }
    err = putBits(bitstream, 1, loudEqInstructions->eqSetIdPresent);
    if (err) return(err);
    if (loudEqInstructions->eqSetIdPresent) {
        err = putBits(bitstream, 6, loudEqInstructions->eqSetId);
        if (err) return(err);
        err = putBits(bitstream, 1, loudEqInstructions->additionalEqSetIdPresent);
        if (err) return(err);
        if (loudEqInstructions->additionalEqSetIdPresent) {
            err = putBits(bitstream, 6, loudEqInstructions->additionalEqSetIdCount);
            if (err) return(err);
            for(i=0; i<loudEqInstructions->additionalEqSetIdCount; i++)
            {
                err = putBits(bitstream, 6, loudEqInstructions->additionalEqSetId[i]);
                if (err) return(err);
            }
        } else {
            loudEqInstructions->additionalEqSetIdCount = 0;
        }
    }
    err = putBits(bitstream, 1, loudEqInstructions->loudnessAfterDrc);
    if (err) return(err);
    err = putBits(bitstream, 1, loudEqInstructions->loudnessAfterEq);
    if (err) return(err);
    err = putBits(bitstream, 6, loudEqInstructions->loudEqGainSequenceCount);
    if (err) return(err);
    for (i=0; i<loudEqInstructions->loudEqGainSequenceCount; i++) {
        int bsLoudEqOffset;
        int bsLoudEqScaling;
        err = putBits(bitstream, 6, loudEqInstructions->gainSequenceIndex[i]);
        if (err) return(err);
        err = putBits(bitstream, 1, loudEqInstructions->drcCharacteristicFormatIsCICP[i]);
        if (err) return(err);
        if (loudEqInstructions->drcCharacteristicFormatIsCICP[i] == 1) {
            err = putBits(bitstream, 7, loudEqInstructions->drcCharacteristic[i]);
            if (err) return(err);
        }
        else
        {
            err = putBits(bitstream, 4, loudEqInstructions->drcCharacteristicLeftIndex[i]);
            if (err) return(err);
            err = putBits(bitstream, 4, loudEqInstructions->drcCharacteristicRightIndex[i]);
            if (err) return(err);
        }
        err = putBits(bitstream, 6, loudEqInstructions->frequencyRangeIndex[i]);
        if (err) return(err);
        bsLoudEqScaling = floor (0.5f - 2.0f * INV_LOG10_2 * log10(loudEqInstructions->loudEqScaling[i]));
        if (bsLoudEqScaling < 0) bsLoudEqScaling = 0;
        else if (bsLoudEqScaling > 7) bsLoudEqScaling = 7;
        err = putBits(bitstream, 3, bsLoudEqScaling);
        if (err) return(err);
        bsLoudEqOffset = floor (0.5f + loudEqInstructions->loudEqOffset[i] / 1.5f + 16.0f);
        if (bsLoudEqOffset < 0) bsLoudEqOffset = 0;
        else if (bsLoudEqOffset > 31) bsLoudEqOffset = 31;
        err = putBits(bitstream, 5, bsLoudEqOffset);
        if (err) return(err);

    }
    return (0);
}

int
writeFilterElement(wobitbufHandle bitstream,
                   FilterElement* filterElement)
{
    int err;
    err = putBits(bitstream, 6, filterElement->filterElementIndex);
    if (err) return(err);
    err = putBits(bitstream, 1, filterElement->filterElementGainPresent);
    if (err) return(err);
    if (filterElement->filterElementGainPresent)
    {
        int bsfilterElementGain = floor (0.5f + 8.0f * (filterElement->filterElementGain + 96.0f));
        bsfilterElementGain = max(0, bsfilterElementGain);
        bsfilterElementGain = min(1023, bsfilterElementGain);
        err = putBits(bitstream, 10, bsfilterElementGain);
        if (err) return(err);
    }
    return(0);
}

int
writeFilterBlock(wobitbufHandle bitstream,
                 FilterBlock* filterBlock)
{
    int err, i;
    err = putBits(bitstream, 6, filterBlock->filterElementCount);
    if (err) return(err);
    
    for (i=0; i<filterBlock->filterElementCount; i++)
    {
        err = writeFilterElement(bitstream, &(filterBlock->filterElement[i]));
        if (err) return(err);
    }
    return(0);
}

int
encodeRadius(float radius,
             int* code)
{
    int i;
    float rho;
    
    if (radius <0.0f)
    {
        printf("Pole / zero radius cannot be negative for encoding\n");
        return (UNEXPECTED_ERROR);
    }
    rho = 1.0f - radius;
    if ((rho < 0.0f) || (rho > 1.0f))
    {
        printf("WARNING: Pole/zero radius out of range  %f\n", radius);
        if (rho < 0.0f) rho = 0.0f;
        if (rho > 1.0f) rho = 1.0f;
    }
    if (rho > zeroPoleRadiusTable[127]) rho = zeroPoleRadiusTable[127];
    i = 0;
    while (rho > zeroPoleRadiusTable[i]) i++;
    if (rho < 0.5f * (zeroPoleRadiusTable[i-1] + zeroPoleRadiusTable[i])) i--;
    *code = i;
    return (0);
}

int
encodeAngle(float angle)
{
    int i;
    
    if ((angle < 0.0f) || (angle > 1.0f))
    {
        printf("WARNING: Pole/zero angle out of range  %f\n", angle);
        if (angle < 0.0f) angle = 0.0f;
        if (angle > 1.0f) angle = 1.0f;
    }
    i = 0;
    while (angle > zeroPoleAngleTable[i]) i++;
    if (angle < 0.5f * (zeroPoleAngleTable[i-1] + zeroPoleAngleTable[i])) i--;
    return (i);
}

int
writeUniqueTdFilterElement(wobitbufHandle bitstream,
                           UniqueTdFilterElement* uniqueTdFilterElement)
{
    int err, i, sign, code;
    err = putBits(bitstream, 1, uniqueTdFilterElement->eqFilterFormat);
    if (err) return(err);
    if (uniqueTdFilterElement->eqFilterFormat == 0)
    {
        int bsRealZeroRadiusOneCount = uniqueTdFilterElement->realZeroRadiusOneCount / 2;
        if ((uniqueTdFilterElement->realZeroRadiusOneCount == 2 * bsRealZeroRadiusOneCount) &&
            (bsRealZeroRadiusOneCount < 8))
        {
            err = putBits(bitstream, 3, bsRealZeroRadiusOneCount);
            if (err) return(err);
        }
        else
        {
            printf("ERROR: realZeroRadiusOneCount must be even and smaller than 16:  %d\n", uniqueTdFilterElement->realZeroRadiusOneCount);
            return(UNEXPECTED_ERROR);
        }
        err = putBits(bitstream, 6, uniqueTdFilterElement->realZeroCount);
        if (err) return(err);
        err = putBits(bitstream, 6, uniqueTdFilterElement->genericZeroCount);
        if (err) return(err);
        err = putBits(bitstream, 4, uniqueTdFilterElement->realPoleCount);
        if (err) return(err);
        err = putBits(bitstream, 4, uniqueTdFilterElement->complexPoleCount);
        if (err) return(err);
        for (i=0; i<uniqueTdFilterElement->realZeroRadiusOneCount; i++)
        {
            err = putBits(bitstream, 1, uniqueTdFilterElement->zeroSign[i]);
            if (err) return(err);
        }
        for (i=0; i<uniqueTdFilterElement->realZeroCount; i++)
        {
            err = encodeRadius(fabs(uniqueTdFilterElement->realZeroRadius[i]), &code);
            if (err) return(err);
            err = putBits(bitstream, 7, code);
            if (err) return(err);
            if (uniqueTdFilterElement->realZeroRadius[i] < 0.0f) sign = 1;
            else sign = 0;
            err = putBits(bitstream, 1, sign);
            if (err) return(err);
        }
        for (i=0; i<uniqueTdFilterElement->genericZeroCount; i++)
        {
            err = encodeRadius(uniqueTdFilterElement->genericZeroRadius[i], &code);
            if (err) return(err);
            err = putBits(bitstream, 7, code);
            if (err) return(err);
            err = putBits(bitstream, 7, encodeAngle(uniqueTdFilterElement->genericZeroAngle[i]));
            if (err) return(err);
        }
        for (i=0; i<uniqueTdFilterElement->realPoleCount; i++)
        {
            err = encodeRadius(fabs(uniqueTdFilterElement->realPoleRadius[i]), &code);
            if (err) return(err);
            err = putBits(bitstream, 7, code);
            if (err) return(err);
            if (uniqueTdFilterElement->realPoleRadius[i] < 0.0f) sign = 1;
            else sign = 0;
            err = putBits(bitstream, 1, sign);
            if (err) return(err);
        }
        for (i=0; i<uniqueTdFilterElement->complexPoleCount; i++)
        {
            err = encodeRadius(uniqueTdFilterElement->complexPoleRadius[i], &code);
            if (err) return(err);
            err = putBits(bitstream, 7, code);
            if (err) return(err);
            err = putBits(bitstream, 7, encodeAngle(uniqueTdFilterElement->complexPoleAngle[i]));
            if (err) return(err);
        }
    }
    else
    {
        err = putBits(bitstream, 7, uniqueTdFilterElement->firFilterOrder);
        if (err) return(err);
        err = putBits(bitstream, 1, uniqueTdFilterElement->firSymmetry);
        if (err) return(err);
        for (i=0; i<uniqueTdFilterElement->firFilterOrder/2+1; i++)
        {
            int bsFirCoefficient;
            int sign;
            if (uniqueTdFilterElement->firCoefficient[i] < 0.0f)
            {
                sign = 1;
            }
            else
            {
                sign = 0;
            }
            err = putBits(bitstream, 1, sign);
            if (err) return(err);
            bsFirCoefficient = floor (0.5f - log10(fabs(uniqueTdFilterElement->firCoefficient[i])) / (0.05f * 0.0625f));
            if (bsFirCoefficient > 1023) bsFirCoefficient = 1023;
            if (bsFirCoefficient < 0) bsFirCoefficient = 0;
            err = putBits(bitstream, 10, bsFirCoefficient);
            if (err) return(err);
            
        }
    }
    return(0);
}

int
encodeEqSlope(float eqSlope,
              int* size,
              int* code)
{
    int i;
    if (fabs(eqSlope) < 0.5f)
    {
        *size = 1;
        *code = 1;
    }
    else
    {
        *size = 5;
        if (eqSlope <= -32.0f)
        {
            *code = 0;
        }
        else if(eqSlope > 32.0f)
        {
            *code = 15;
        }
        else
        {
            i=0;
            while (eqSlope > eqSlopeTable[i]) i++;
            if (eqSlope < 0.5f * (eqSlopeTable[i-1] + eqSlopeTable[i])) i--;
            *code = i;
        }
    }
    return (0);
}

int
encodeEqGainInitial(float eqGainInitial,
                    int* prefixCode,
                    int* size,
                    int* code)
{
    if ((eqGainInitial > -8.5f) && (eqGainInitial < 7.75f))
    {
        *prefixCode = 0;
        *size = 5;
        
        *code = floor(0.5f + 2.0f * (max(-8.0f, eqGainInitial) + 8.0f));
    }
    else if (eqGainInitial < 0.0f)
    {
        if (eqGainInitial > -17.0f)
        {
            *prefixCode = 1;
            *size = 4;
            *code = floor(0.5f + max(-16.0f, eqGainInitial) + 16.0f);
        }
        else if (eqGainInitial > -34.0f)
        {
            *prefixCode = 2;
            *size = 4;
            *code = floor(0.5f + 0.5f * (max(-32.0f, eqGainInitial) + 32.0f));
            
        }
        else
        {
            *prefixCode = 3;
            *size = 3;
            *code = floor(0.5f + 0.25f * (max(-64.0f, eqGainInitial) + 64.0f));
        }
    }
    else
    {
        if (eqGainInitial < 15.5f)
        {
            *prefixCode = 1;
            *size = 4;
            *code = floor(0.5f + eqGainInitial);
        }
        else
        {
            *prefixCode = 2;
            *size = 4;
            *code = floor(0.5f + 0.5f * min(30.0f, eqGainInitial));
        }
    }
    return(0);
}

int
encodeEqGainDelta(float eqGainDelta,
                  int* code)
{
    int i;
    if (eqGainDelta <= -22.0f)
    {
        *code = 0;
    }
    else if (eqGainDelta >= 32.0f)
    {
        *code = 31;
    }
    else
    {
        i=0;
        while (eqGainDelta > eqGainDeltaTable[i]) i++;
        if (eqGainDelta < 0.5f * (eqGainDeltaTable[i-1] + eqGainDeltaTable[i])) i--;
        *code = i;
    }
    return(0);
}

int
writeEqSubbandGainSpline(wobitbufHandle bitstream,
                         EqSubbandGainSpline* eqSubbandGainSpline)
{
    int err, i;
    int size;
    int code;
    int prefixCode;
    
    int bsEqNodeCount = eqSubbandGainSpline->nEqNodes - 2;
    err = putBits(bitstream, 5, bsEqNodeCount);
    if (err) return(err);
    for (i=0; i<eqSubbandGainSpline->nEqNodes; i++)
    {
        err = encodeEqSlope(eqSubbandGainSpline->eqSlope[i], &size, &code);
        if (err) return(err);
        err = putBits(bitstream, size, code);
        if (err) return(err);
    }
    for (i=1; i<eqSubbandGainSpline->nEqNodes; i++)
    {
        code = min (15, eqSubbandGainSpline->eqFreqDelta[i]-1);
        err = putBits(bitstream, 4, code);
        if (err) return(err);
    }
    err = encodeEqGainInitial(eqSubbandGainSpline->eqGainInitial, &prefixCode, &size, &code);
    if (err) return(err);
    err = putBits(bitstream, 2, prefixCode);
    if (err) return(err);
    err = putBits(bitstream, size, code);
    if (err) return(err);
    for (i=1; i<eqSubbandGainSpline->nEqNodes; i++)
    {
        err = encodeEqGainDelta(eqSubbandGainSpline->eqGainDelta[i], &code);
        if (err) return(err);
        err = putBits(bitstream, 5, code);
        if (err) return(err);
    }
    return(0);
}

int
writeEqSubbandGainVector(wobitbufHandle bitstream,
                         int eqSubbandGainCount,
                         EqSubbandGainVector* eqSubbandGainVector)
{
    int i=0, err, sign;
    for (i=0; i<eqSubbandGainCount; i++) {
        int bsEqSubbandGain = floor (0.5f + fabs(eqSubbandGainVector->eqSubbandGain[i] * 8.0f)); /*TODO: check index if index i is correct */
        if (eqSubbandGainVector->eqSubbandGain[i] < 0.0f)
        {
            sign = 1;
        }
        else
        {
            sign = 0;
        }
        err = putBits(bitstream, 1, sign);
        if (err) return(err);
        err = putBits(bitstream, 8, bsEqSubbandGain);
        if (err) return(err);
    }
    return(0);
}

int
writeEqCoefficients(wobitbufHandle bitstream,
                    DrcEncParams* drcParams,
                    EqCoefficients* eqCoefficients)
{
    int err = 0, i = 0, mu = 0, nu = 0;
    
    err = putBits(bitstream, 1, eqCoefficients->eqDelayMaxPresent);
    if (err) return(err);
    if (eqCoefficients->eqDelayMaxPresent == 1) {
        for (nu=0; nu<8; nu++) {
            mu = eqCoefficients->eqDelayMax / (16 << nu);
            if (mu * (16 << nu) < eqCoefficients->eqDelayMax) {
                mu++;
            }
            if (mu < 32) {
                break;
            }
        }
        if (nu == 8) {
            nu = 7;
            mu = 31;
        }
        /*printf("eqDelayMax (encoder) = %d, eqDelayMax (decoder) = %d\n",eqCoefficients->eqDelayMax, (int)(16*mu*pow(2.f, nu)));*/
        err = putBits(bitstream, 5, mu);
        if (err) return(err);
        err = putBits(bitstream, 3, nu);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 6, eqCoefficients->uniqueFilterBlockCount);
    if (err) return(err);
    for (i=0; i<eqCoefficients->uniqueFilterBlockCount; i++)
    {
        err = writeFilterBlock(bitstream, &(eqCoefficients->filterBlock[i]));
        if (err) return(err);
    }
    err = putBits(bitstream, 6, eqCoefficients->uniqueTdFilterElementCount);
    if (err) return(err);
    for (i=0; i<eqCoefficients->uniqueTdFilterElementCount; i++)
    {
        err = writeUniqueTdFilterElement(bitstream, &(eqCoefficients->uniqueTdFilterElement[i]));
        if (err) return(err);
    }
    err = putBits(bitstream, 6, eqCoefficients->uniqueEqSubbandGainsCount);
    if (err) return(err);
    
    if (eqCoefficients->uniqueEqSubbandGainsCount > 0)
    {
        int bsEqGainCount;
        err = putBits(bitstream, 1, eqCoefficients->eqSubbandGainRepresentation);
        if (err) return(err);
        err = putBits(bitstream, 4, eqCoefficients->eqSubbandGainFormat);
        if (err) return(err);
        switch (eqCoefficients->eqSubbandGainFormat)
        {
            case GAINFORMAT_QMF32:
                eqCoefficients->eqSubbandGainCount = 32;
                break;
            case GAINFORMAT_QMFHYBRID39:
                eqCoefficients->eqSubbandGainCount = 39;
                break;
            case GAINFORMAT_QMF64:
                eqCoefficients->eqSubbandGainCount = 64;
                break;
            case GAINFORMAT_QMFHYBRID71:
                eqCoefficients->eqSubbandGainCount = 71;
                break;
            case GAINFORMAT_QMF128:
                eqCoefficients->eqSubbandGainCount = 128;
                break;
            case GAINFORMAT_QMFHYBRID135:
                eqCoefficients->eqSubbandGainCount = 135;
                break;
            case GAINFORMAT_UNIFORM:
            default:
                bsEqGainCount = eqCoefficients->eqSubbandGainCount - 1;
                err = putBits(bitstream, 8, bsEqGainCount);
                if (err) return(err);
                break;
        }
        for (i=0; i<eqCoefficients->uniqueEqSubbandGainsCount; i++)
        {
            if (eqCoefficients->eqSubbandGainRepresentation == 1)
            {
                err = writeEqSubbandGainSpline(bitstream, &(eqCoefficients->eqSubbandGainSpline[i]));
                if (err) return(err);
            }
            else
            {
                err = writeEqSubbandGainVector(bitstream, eqCoefficients->eqSubbandGainCount, &(eqCoefficients->eqSubbandGainVector[i]));
                if (err) return(err);
            }
        }
    }
    return(0);
}

int
writeFilterBlockRefs(wobitbufHandle bitstream,
                     FilterBlockRefs* filterBlockRefs)
{
    int err, i;
    err = putBits(bitstream, 4, filterBlockRefs->filterBlockCount);
    if (err) return(err);
    for (i=0; i<filterBlockRefs->filterBlockCount; i++)
    {
        err = putBits(bitstream, 7, filterBlockRefs->filterBlockIndex[i]);
        if (err) return(err);
    }
    return (0);
}

int
writeTdFilterCascade(wobitbufHandle bitstream,
                     const int eqChannelGroupCount,
                     TdFilterCascade* tdFilterCascade)
{
    int i, k, err;
    
    for (i=0; i<eqChannelGroupCount; i++)
    {
        err = putBits(bitstream, 1, tdFilterCascade->eqCascadeGainPresent[i]);
        if (err) return(err);
        if (tdFilterCascade->eqCascadeGainPresent[i] == 1)
        {
            int bsEqCascadeGain = floor (0.5f + 8.0f * (tdFilterCascade->eqCascadeGain[i] + 96.0f));
            bsEqCascadeGain = max(0, bsEqCascadeGain);
            bsEqCascadeGain = min(1023, bsEqCascadeGain);
            err = putBits(bitstream, 10, bsEqCascadeGain);
            if (err) return(err);
        }
        err = writeFilterBlockRefs(bitstream, &(tdFilterCascade->filterBlockRefs[i]));
        if (err) return(err);
    }
    err = putBits(bitstream, 1, tdFilterCascade->eqPhaseAlignmentPresent);
    if (err) return(err);
    if (tdFilterCascade->eqPhaseAlignmentPresent == 1)
    {
        for (i=0; i<eqChannelGroupCount; i++)
        {
            for (k=i+1; k<eqChannelGroupCount; k++)
            {
                err = putBits(bitstream, 1, tdFilterCascade->eqPhaseAlignment[i][k]);
                if (err) return(err);
            }
        }
    }
    return(0);
}

int
writeEqInstructions(wobitbufHandle bitstream,
                    UniDrcConfigExt* uniDrcConfigExt,
                    DrcEncParams* drcParams,
                    EqInstructions* eqInstructions)
{
    int err, i;
    err = putBits(bitstream, 6, eqInstructions->eqSetId);
    if (err) return(err);
    err = getEqComplexityLevel (uniDrcConfigExt, drcParams, eqInstructions);
    if (err) return(err);
    err = putBits(bitstream, 4, eqInstructions->eqSetComplexityLevel);
    if (err) return(err);
    err = putBits(bitstream, 1, eqInstructions->downmixIdPresent);
    if (err) return(err);
    if (eqInstructions->downmixIdPresent == 1)
    {
        err = putBits(bitstream, 7, eqInstructions->downmixId);
        if (err) return(err);
        err = putBits(bitstream, 1, eqInstructions->eqApplyToDownmix);
        if (err) return(err);
        err = putBits(bitstream, 1, eqInstructions->additionalDownmixIdPresent);
        if (err) return(err);
        if (eqInstructions->additionalDownmixIdPresent == 1)
        {
            err = putBits(bitstream, 7, eqInstructions->additionalDownmixIdCount);
            if (err) return(err);
            for (i=0; i<eqInstructions->additionalDownmixIdCount; i++)
            {
                err = putBits(bitstream, 7, eqInstructions->additionalDownmixId[i]);
                if (err) return(err);
            }
        }
    }
    err = putBits(bitstream, 6, eqInstructions->drcSetId);
    if (err) return(err);
    err = putBits(bitstream, 1, eqInstructions->additionalDrcSetIdPresent);
    if (err) return(err);
    if (eqInstructions->additionalDrcSetIdPresent == 1)
    {
        err = putBits(bitstream, 6, eqInstructions->additionalDrcSetIdCount);
        if (err) return(err);
        for (i=0; i<eqInstructions->additionalDrcSetIdCount; i++)
        {
            err = putBits(bitstream, 6, eqInstructions->additionalDrcSetId[i]);
            if (err) return(err);
        }
    }
    err = putBits(bitstream, 16, eqInstructions->eqSetPurpose);
    if (err) return(err);
    err = putBits(bitstream, 1, eqInstructions->dependsOnEqSetPresent);
    if (err) return(err);
    if (eqInstructions->dependsOnEqSetPresent == 1)
    {
        err = putBits(bitstream, 6, eqInstructions->dependsOnEqSet);
        if (err) return(err);
    }
    else
    {
        err = putBits(bitstream, 1, eqInstructions->noIndependentEqUse);
        if (err) return(err);
    }
    for (i=0; i<eqInstructions->eqChannelCount; i++)
    {
        err = putBits(bitstream, 7, eqInstructions->eqChannelGroupForChannel[i]);
        if (err) return(err);
    }
    err = putBits(bitstream, 1, eqInstructions->tdFilterCascadePresent);
    if (err) return(err);
    if (eqInstructions->tdFilterCascadePresent == 1)
    {
        err = writeTdFilterCascade(bitstream, eqInstructions->eqChannelGroupCount, &(eqInstructions->tdFilterCascade));
        if (err) return(err);
    }
    err = putBits(bitstream, 1, eqInstructions->subbandGainsPresent);
    if (err) return(err);
    if (eqInstructions->subbandGainsPresent == 1)
    {
        for (i=0; i<eqInstructions->eqChannelGroupCount; i++)
        {
            err = putBits(bitstream, 6, eqInstructions->subbandGainsIndex[i]);
            if (err) return(err);
        }
    }
    err = putBits(bitstream, 1, eqInstructions->eqTransitionDurationPresent);
    if (err) return(err);
    if (eqInstructions->eqTransitionDurationPresent == 1)
    {
        int bsEqTransitionDuration;
        float tmp = max (0.004f, eqInstructions->eqTransitionDuration);
        tmp = min(0.861f, tmp);
        bsEqTransitionDuration = floor(0.5f + 4.0f * (log10(1000.0f * tmp) / log10(2.0f) - 2.0f));
        err = putBits(bitstream, 5, bsEqTransitionDuration);
        if (err) return(err);
    }
    return (0);
}

#endif /* AMD1_SYNTAX */

static int
writeUniDrcConfigExtension(wobitbufHandle bitstream,
                           DrcEncParams* drcParams,
                           UniDrcConfig* uniDrcConfig,
                           UniDrcConfigExt* uniDrcConfigExt
)
{
    int err=0, k=0, tmp=0, i;
    int extSizeBits = 0, bitSizeLen = 0, bitSize = 0;

    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    wobitbuf bs;
    wobitbufHandle bitstreamTmp = &bs;
    
    err = putBits(bitstream, 4, uniDrcConfigExt->uniDrcConfigExtType[k]);
    if (err) return(err);
    while(uniDrcConfigExt->uniDrcConfigExtType[k] != UNIDRCCONFEXT_TERM)
    {
        /*  get bit count for extension payload; add future extensions here */
        switch(uniDrcConfigExt->uniDrcConfigExtType[k])
        {
#if AMD1_SYNTAX
            case UNIDRCCONFEXT_PARAM_DRC:
                wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS, 0);
                err = writeDrcCoefficientsParametricDrc(bitstreamTmp, drcParams, uniDrcConfig, &(uniDrcConfigExt->drcCoefficientsParametricDrc));
                if (err) return(err);
                err = putBits(bitstreamTmp, 4, uniDrcConfigExt->parametricDrcInstructionsCount);
                if (err) return(err);
                for (i=0; i<uniDrcConfigExt->parametricDrcInstructionsCount; i++) {
                    err = writeParametricDrcInstructions(bitstreamTmp, uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcFrameSize, drcParams, uniDrcConfig, &(uniDrcConfigExt->parametricDrcInstructions[i]));
                    if (err) return(err);
                }
                break;
            case UNIDRCCONFEXT_V1:
                {
                    int version = 1;
                    wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS, 0);
                    err = putBits(bitstreamTmp, 1, uniDrcConfigExt->downmixInstructionsV1Present);
                    if (err) return(err);
                    if (uniDrcConfigExt->downmixInstructionsV1Present == 1) {
                        err = putBits(bitstreamTmp, 7, uniDrcConfigExt->downmixInstructionsV1Count);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->downmixInstructionsV1Count; i++) {
                            err = writeDownmixInstructions(bitstreamTmp, version, drcParams, &(uniDrcConfigExt->downmixInstructionsV1[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstreamTmp, 1, uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present);
                    if (err) return(err);
                    if (uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present == 1) {
                        err = putBits(bitstreamTmp, 3, uniDrcConfigExt->drcCoefficientsUniDrcV1Count);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->drcCoefficientsUniDrcV1Count; i++) {
                            err = writeDrcCoefficientsUniDrc(bitstreamTmp, version, &(uniDrcConfigExt->drcCoefficientsUniDrcV1[i]));
                            if (err) return(err);
                        }
#ifdef LEVELING_SUPPORT
                        unsigned int const num_ducking_only_drc_sets = getNumDuckingOnlyDrcSets(uniDrcConfigExt->drcInstructionsUniDrcV1, uniDrcConfigExt->drcInstructionsUniDrcV1Count);
                        err = putBits(bitstreamTmp, 6, uniDrcConfigExt->drcInstructionsUniDrcV1Count - num_ducking_only_drc_sets);
#else
                        err = putBits(bitstreamTmp, 6, uniDrcConfigExt->drcInstructionsUniDrcV1Count);
#endif
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->drcInstructionsUniDrcV1Count; i++) {
#ifdef LEVELING_SUPPORT
                            if (uniDrcConfigExt->drcInstructionsUniDrcV1[i].duckingOnlyDrcSet) {
                                continue;
                            }
#endif
                            err = writeDrcInstructionsUniDrc(bitstreamTmp, version, uniDrcConfig, drcParams, &(uniDrcConfigExt->drcInstructionsUniDrcV1[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstreamTmp, 1, uniDrcConfigExt->loudEqInstructionsPresent);
                    if (err) return(err);
                    if (uniDrcConfigExt->loudEqInstructionsPresent == 1) {
                        err = putBits(bitstreamTmp, 4, uniDrcConfigExt->loudEqInstructionsCount);
                        if (err) return(err);
                        
                        for (i=0; i<uniDrcConfigExt->loudEqInstructionsCount; i++) {
                            writeLoudEqInstructions(bitstreamTmp, drcParams, &(uniDrcConfigExt->loudEqInstructions[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstreamTmp, 1, uniDrcConfigExt->eqPresent);
                    if (err) return(err);
                    if (uniDrcConfigExt->eqPresent == 1) {
                        writeEqCoefficients(bitstreamTmp, drcParams, &(uniDrcConfigExt->eqCoefficients));
                        if (err) return(err);
                        err = putBits(bitstreamTmp, 4, uniDrcConfigExt->eqInstructionsCount);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->eqInstructionsCount; i++) {
                            writeEqInstructions(bitstreamTmp, uniDrcConfigExt, drcParams, &(uniDrcConfigExt->eqInstructions[i]));
                            if (err) return(err);
                        }
                    }
                }
                break;
#endif /* AMD1_SYNTAX */
#ifdef LEVELING_SUPPORT
            case UNIDRCCONFEXT_LEVELING:
                wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS, 0);
                err = writeLoudnessLevelingExtension(bitstreamTmp, uniDrcConfig, drcParams);
                if (err) return(err);
                break;
#endif
            default:
                break;
        }
        uniDrcConfigExt->extBitSize[k] = wobitbuf_GetBitsWritten(bitstreamTmp);
        bitSize     = uniDrcConfigExt->extBitSize[k] - 1;
        extSizeBits = (int)(log((float)bitSize)/log(2.f))+1;
        extSizeBits = (extSizeBits < 4) ? 4 : extSizeBits;
        bitSizeLen  = extSizeBits - 4;
        err = putBits(bitstream, 4, bitSizeLen);
        if (err) return(err);
        err = putBits(bitstream, extSizeBits, bitSize);
        if (err) return(err);
        switch(uniDrcConfigExt->uniDrcConfigExtType[k])
        {
#if AMD1_SYNTAX
            case UNIDRCCONFEXT_PARAM_DRC:
                err = writeDrcCoefficientsParametricDrc(bitstream, drcParams, uniDrcConfig, &(uniDrcConfigExt->drcCoefficientsParametricDrc));
                if (err) return(err);
                err = putBits(bitstream, 4, uniDrcConfigExt->parametricDrcInstructionsCount);
                if (err) return(err);
                for (i=0; i<uniDrcConfigExt->parametricDrcInstructionsCount; i++) {
                    err = writeParametricDrcInstructions(bitstream, uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcFrameSize, drcParams, uniDrcConfig, &(uniDrcConfigExt->parametricDrcInstructions[i]));
                    if (err) return(err);
                }
                break;
            case UNIDRCCONFEXT_V1:
                {
                    int version = 1;
                    err = putBits(bitstream, 1, uniDrcConfigExt->downmixInstructionsV1Present);
                    if (err) return(err);
                    if (uniDrcConfigExt->downmixInstructionsV1Present == 1) {
                        err = putBits(bitstream, 7, uniDrcConfigExt->downmixInstructionsV1Count);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->downmixInstructionsV1Count; i++) {
                            err = writeDownmixInstructions(bitstream, version, drcParams, &(uniDrcConfigExt->downmixInstructionsV1[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstream, 1, uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present);
                    if (err) return(err);
                    if (uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present == 1) {
                        int version = 1;
                        err = putBits(bitstream, 3, uniDrcConfigExt->drcCoefficientsUniDrcV1Count);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->drcCoefficientsUniDrcV1Count; i++) {
                            err = writeDrcCoefficientsUniDrc(bitstream, version, &(uniDrcConfigExt->drcCoefficientsUniDrcV1[i]));
                            if (err) return(err);
                        }
#ifdef LEVELING_SUPPORT
                        unsigned int const num_ducking_only_drc_sets = getNumDuckingOnlyDrcSets(uniDrcConfigExt->drcInstructionsUniDrcV1, uniDrcConfigExt->drcInstructionsUniDrcV1Count);
                        err = putBits(bitstream, 6, uniDrcConfigExt->drcInstructionsUniDrcV1Count - num_ducking_only_drc_sets);
#else
                        err = putBits(bitstream, 6, uniDrcConfigExt->drcInstructionsUniDrcV1Count);
#endif
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->drcInstructionsUniDrcV1Count; i++) {
#ifdef LEVELING_SUPPORT
                            if(uniDrcConfigExt->drcInstructionsUniDrcV1[i].duckingOnlyDrcSet) {
                                continue;
                            }
#endif
                            err = writeDrcInstructionsUniDrc(bitstream, version, uniDrcConfig, drcParams, &(uniDrcConfigExt->drcInstructionsUniDrcV1[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstream, 1, uniDrcConfigExt->loudEqInstructionsPresent);
                    if (err) return(err);
                    if (uniDrcConfigExt->loudEqInstructionsPresent == 1) {
                        err = putBits(bitstream, 4, uniDrcConfigExt->loudEqInstructionsCount);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->loudEqInstructionsCount; i++) {
                            writeLoudEqInstructions(bitstream, drcParams, &(uniDrcConfigExt->loudEqInstructions[i]));
                            if (err) return(err);
                        }
                    }
                    err = putBits(bitstream, 1, uniDrcConfigExt->eqPresent);
                    if (err) return(err);
                    if (uniDrcConfigExt->eqPresent == 1) {
                        writeEqCoefficients(bitstream, drcParams, &(uniDrcConfigExt->eqCoefficients));
                        if (err) return(err);
                        err = putBits(bitstream, 4, uniDrcConfigExt->eqInstructionsCount);
                        if (err) return(err);
                        for (i=0; i<uniDrcConfigExt->eqInstructionsCount; i++) {
                            writeEqInstructions(bitstream, uniDrcConfigExt, drcParams, &(uniDrcConfigExt->eqInstructions[i]));
                            if (err) return(err);
                        }
                    }
                }
                break;
#endif /* AMD1_SYNTAX */
#ifdef LEVELING_SUPPORT
            case UNIDRCCONFEXT_LEVELING:
                err = writeLoudnessLevelingExtension(bitstream, uniDrcConfig, drcParams);
                if (err) return(err);
                break;
#endif
                /* add future extensions here */
            default:
                for(i = 0; i<uniDrcConfigExt->extBitSize[k]; i++)
                {
                    err = putBits(bitstream, 1, tmp);
                    if (err) return(err);
                }
        }
        k++;
        err = putBits(bitstream, 4, uniDrcConfigExt->uniDrcConfigExtType[k]);
        if (err) return(err);
    }
    
    return (0);
}


/* Parser for in-stream DRC configuration */
int
writeUniDrcConfig(HANDLE_DRC_ENCODER hDrcEnc,
                  unsigned char* bitstreamBuffer,
                  int* bitCount)
{
    int i, err;
    int version = 0;
    UniDrcConfig* uniDrcConfig = &(hDrcEnc->uniDrcConfig);
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);
    
#if !MPEG_H_SYNTAX
    err = putBits(bitstream, 1, uniDrcConfig->sampleRatePresent);
    if (err) return(err);
    
    if(uniDrcConfig->sampleRatePresent)
    {
        err = putBits(bitstream, 18, uniDrcConfig->sampleRate-1000);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 7, uniDrcConfig->downmixInstructionsCount);
    if (err) return(err);
    err = putBits(bitstream, 1, uniDrcConfig->drcDescriptionBasicPresent);
    if (err) return(err);
    if (uniDrcConfig->drcDescriptionBasicPresent)
    {
        err = putBits(bitstream, 3, uniDrcConfig->drcCoefficientsBasicCount);
        if (err) return(err);
        err = putBits(bitstream, 4, uniDrcConfig->drcInstructionsBasicCount);
        if (err) return(err);
    }
    else
    {
        uniDrcConfig->drcCoefficientsBasicCount=0;
        uniDrcConfig->drcInstructionsBasicCount=0;
    }
#endif
    err = putBits(bitstream, 3, uniDrcConfig->drcCoefficientsUniDrcCount);
    if (err) return(err);
    err = putBits(bitstream, 6, uniDrcConfig->drcInstructionsUniDrcCount
#if defined LEVELING_SUPPORT && MPEG_H_SYNTAX
        - getNumDuckingOnlyDrcSets(uniDrcConfig->drcInstructionsUniDrc, uniDrcConfig->drcInstructionsUniDrcCount)
#endif
    );
    if (err) return(err);

    err = writeChannelLayout(bitstream, &uniDrcConfig->channelLayout);
    if (err) return(err);

#if !MPEG_H_SYNTAX
    for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
    {
        err = writeDownmixInstructions(bitstream, version, hDrcEnc, &(uniDrcConfig->downmixInstructions[i]));
        if (err) return(err);
    }

    for(i=0; i<uniDrcConfig->drcCoefficientsBasicCount; i++)
    {
        err = writeDrcCoefficientsBasic(bitstream, &(uniDrcConfig->drcCoefficientsBasic[i]));
        if (err) return(err);
    }
    for(i=0; i<uniDrcConfig->drcInstructionsBasicCount; i++)
    {
        err = writeDrcInstructionsBasic(bitstream, &(uniDrcConfig->drcInstructionsBasic[i]));
        if (err) return(err);
    }
#endif
    for(i=0; i<uniDrcConfig->drcCoefficientsUniDrcCount; i++)
    {
        err = writeDrcCoefficientsUniDrc(bitstream, version, &(uniDrcConfig->drcCoefficientsUniDrc[i]));
        if (err) return(err);
    }
    for(i=0; i<uniDrcConfig->drcInstructionsUniDrcCount; i++)
    {
#if MPEG_H_SYNTAX
        if(uniDrcConfig->drcInstructionsUniDrc[i].duckingOnlyDrcSet) continue;

        if (uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType == 0)
            putBits(bitstream, 1, uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType);
        else
        {
            if (uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType == 1) {
                fprintf(stderr, "ERROR: drcInstructionsType=%d not allowed.\n", uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType);
                return (UNEXPECTED_ERROR);
            }
            putBits(bitstream, 2, uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType);
        }

        if (uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType == 2)
            putBits(bitstream, 7, uniDrcConfig->drcInstructionsUniDrc[i].mae_groupID);
        else if (uniDrcConfig->drcInstructionsUniDrc[i].drcInstructionsType == 3)
            putBits(bitstream, 5, uniDrcConfig->drcInstructionsUniDrc[i].mae_groupPresetID);
#endif
        err = writeDrcInstructionsUniDrc(bitstream, version, uniDrcConfig, hDrcEnc, &(uniDrcConfig->drcInstructionsUniDrc[i]));
        if (err) return(err);
    }

    err = putBits(bitstream, 1, uniDrcConfig->uniDrcConfigExtPresent);
    if (err) return(err);
    if (uniDrcConfig->uniDrcConfigExtPresent)
    {
        err = writeUniDrcConfigExtension(bitstream, hDrcEnc, uniDrcConfig, &(uniDrcConfig->uniDrcConfigExt));
        if (err) return(err);
    }
#if MPEG_H_SYNTAX
     putBits(bitstream, 1, uniDrcConfig->loudnessInfoSetPresent);
     if (uniDrcConfig->loudnessInfoSetPresent == 1)
     {
        err = writeLoudnessInfoSet(hDrcEnc, bitstreamBuffer, bitCount);
        if (err) return(err);
     }
#endif

    *bitCount += wobitbuf_GetBitsWritten(bitstream);

    return(0);
}


/* ====================================================================================
                           multiplex of DRC gain sequences
   ====================================================================================*/

int
encInitialGain(const int gainCodingProfile,
               float gainInitial,
               float* gainInitialQuant,
               int* codeSize,
               int* code)
{
    int sign, magn, bits, size;
    switch (gainCodingProfile)
    {
        case GAIN_CODING_PROFILE_REGULAR:
            if (gainInitial < 0.0f)
            {
                gainInitial = max(-1000.0f, gainInitial);
                magn = (int)(0.5f - 8.0f * gainInitial);
                magn = min(0xFF, magn);
                sign = 1;
                *gainInitialQuant = - magn * 0.125f;
            }
            else
            {
                gainInitial = min(1000.0f, gainInitial);
                magn = (int)(0.5f + 8.0f * gainInitial);
                magn = min(0xFF, magn);
                sign = 0;
                *gainInitialQuant = magn * 0.125f;
            }
            bits = (sign << 8) + magn;
            size = 9;
            break;
        case GAIN_CODING_PROFILE_FADING:
            if (gainInitial > -0.0625f)
            {
                sign = 0;
                *gainInitialQuant = 0.0f;
                bits = sign;
                size = 1;
           }
            else
            {
                sign = 1;
                gainInitial = max(-1000.0f, gainInitial);
                magn = (int)(- 1.0f + 0.5f - 8.0f * gainInitial);
                magn = min(0x3FF, magn);
                *gainInitialQuant = - (magn + 1) * 0.125f;
                bits = (sign << 10) + magn;
                size = 11;
            }
            break;
        case GAIN_CODING_PROFILE_CLIPPING:
            if (gainInitial > -0.0625f)
            {
                sign = 0;
                *gainInitialQuant = 0.0f;
                bits = sign;
                size = 1;
            }
            else
            {
                sign = 1;
                gainInitial = max(-1000.0f, gainInitial);
                magn = (int)(- 1.0f + 0.5f - 8.0f * gainInitial);
                magn = min(0xFF, magn);
                *gainInitialQuant = - (magn + 1) * 0.125f;
                bits = (sign << 8) + magn;
                size = 9;
            }
            break;
        case GAIN_CODING_PROFILE_CONSTANT:
            bits = 0;
            size = 0;
            break;
        default:
            return(UNEXPECTED_ERROR);
    }
    *code = bits;
    *codeSize = size;
    return (0);
}

int
writeSplineNodes(wobitbufHandle bitstream,
                 DrcEncParams* drcParams,
                 GainSetParams* gainSetParams,
                 DrcGroupForOutput* drcGroupForOutput)
{
    int err, n;
    
    err = putBits(bitstream, 1, drcGroupForOutput->codingMode);
    if (err) return(err);
    
    if (drcGroupForOutput->codingMode == 0)
    {
        err = putBits(bitstream, drcGroupForOutput->gainCodeLength[0], drcGroupForOutput->gainCode[0]);
        if (err) return(err);
    }
    else
    {
        bool frameEndFlag;
        if (drcGroupForOutput->nGainValues < 1)
        {
            fprintf(stderr, "ERROR: nGainValues must be larger than 0  (%d).\n", drcGroupForOutput->nGainValues);
            return (UNEXPECTED_ERROR);
        }
        for (n=0; n<drcGroupForOutput->nGainValues-1; n++)
        {
            err = putBits(bitstream, 1, 0x0);  /* end marker */
            if (err) return(err);
        }
        err = putBits(bitstream, 1, 0x1);  /* end marker */
        if (err) return(err);
        if (gainSetParams->gainInterpolationType == GAIN_INTERPOLATION_TYPE_SPLINE) {
            for (n=0; n<drcGroupForOutput->nGainValues; n++)
            {
                err = putBits(bitstream, drcGroupForOutput->slopeCodeSize[n], drcGroupForOutput->slopeCode[n]);
                if (err) return(err);
            }
        }
        else {  /* GAIN_INTERPOLATION_TYPE_LINEAR */
            for (n=0; n<drcGroupForOutput->nGainValues; n++)
            {
                /* Don't write slope */
            }
        }
        if (gainSetParams->fullFrame == 0)
        {
#ifdef NODE_RESERVOIR
            frameEndFlag = drcGroupForOutput->frameEndFlag;
#else
            /* check if last node is at the end of the frame */
            if (drcGroupForOutput->tsGainQuant[drcGroupForOutput->nGainValues-1] == (drcParams->drcFrameSize-1))
            {
               frameEndFlag = 1;
            }
            else
            {
               frameEndFlag = 0;
            }
#endif
            err = putBits(bitstream, 1, frameEndFlag);
            if (err) return(err);
        }
        else
        {
            frameEndFlag = 1;
        }

        for (n=0; n<drcGroupForOutput->nGainValues; n++)
        {
            if (n<(drcGroupForOutput->nGainValues-1) || !frameEndFlag) /* check for frameEndFlag at last node */
            {
                err = putBits(bitstream, drcGroupForOutput->timeDeltaCodeSize[n], drcGroupForOutput->timeDeltaCode[n]);
                if (err) return(err);
            }
        }

        for (n=0; n<drcGroupForOutput->nGainValues; n++)
        {
            err = putBits(bitstream, drcGroupForOutput->gainCodeLength[n], drcGroupForOutput->gainCode[n]);
            if (err) return(err);
        }
    }
    return(0);
}


int
writeDrcGainSequence(wobitbufHandle bitstream,
                     DrcEncParams* drcParams,
                     GainSetParams* gainSetParams,
                     DrcGroupForOutput* drcGroupForOutput)
{
    int err;
    
    err = writeSplineNodes(bitstream, drcParams, gainSetParams, drcGroupForOutput);
    if (err) return(err);

    return(0);
}

static int
writeUniDrcGainExtension(wobitbufHandle bitstream,
                         UniDrcGainExt* uniDrcGainExt)
{
    int err=0, k=0, tmp=0, i;
    int extSizeBits, bitSizeLen, bitSize;

    err = putBits(bitstream, 4, uniDrcGainExt->uniDrcGainExtType[k]);
    if (err) return(err);
    while(uniDrcGainExt->uniDrcGainExtType[k] != UNIDRCGAINEXT_TERM)
    {
        bitSize     = uniDrcGainExt->extBitSize[k] - 1;
        extSizeBits = (int)(log((float)bitSize)/log(2.f))+1;
        bitSizeLen  = extSizeBits - 4;
        err = putBits(bitstream, 3, bitSizeLen);
        if (err) return(err);
        err = putBits(bitstream, extSizeBits, bitSize);
        if (err) return(err);
        switch(uniDrcGainExt->uniDrcGainExtType[k])
        {
            /* add future extensions here */
            default:
                for(i = 0; i<uniDrcGainExt->extBitSize[k]; i++)
                {
                    err = putBits(bitstream, 1, tmp);
                    if (err) return(err);
                }
        }
        k++;
        err = putBits(bitstream, 4, uniDrcGainExt->uniDrcGainExtType[k]);
        if (err) return(err);
    }

    return (0);
}

int
writeUniDrcGain(HANDLE_DRC_ENCODER hDrcEnc,
                UniDrcGainExt uniDrcGainExtension,
                unsigned char* bitstreamBuffer,
                int* bitCount)
{
    int err=0, i;
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);
    
    for (i=0; i<hDrcEnc->nSequences; i++)
    {
        DrcGroupForOutput *drcGroupForOutput = &(hDrcEnc->drcGainSeqBuf[i].drcGroupForOutput);
        GainSetParams *gainSetParams = &(hDrcEnc->drcGainSeqBuf[i].gainSetParams);
        if (gainSetParams->gainCodingProfile < GAIN_CODING_PROFILE_CONSTANT)
        {
            err = writeDrcGainSequence(bitstream, hDrcEnc, gainSetParams, drcGroupForOutput);
        }
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, uniDrcGainExtension.uniDrcGainExtPresent);
    if (err) return(err);
    
    if (uniDrcGainExtension.uniDrcGainExtPresent)
    {
        err = writeUniDrcGainExtension(bitstream, &uniDrcGainExtension);
        if (err) return(err);
    }

    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return(0);
}

static int
writeLoudnessInfoSetExtension(wobitbufHandle bitstream,
                              LoudnessInfoSetExtension* loudnessInfoSetExtension)
{
    int err=0, k=0, tmp=0, i, version=1;
    int extSizeBits = 0, bitSizeLen = 0, bitSize = 0;
    
    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    wobitbuf bs;
    wobitbufHandle bitstreamTmp = &bs;
    wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS, 0);
    
    err = putBits(bitstream, 4, loudnessInfoSetExtension->loudnessInfoSetExtType[k]);
    if (err) return(err);
    while(loudnessInfoSetExtension->loudnessInfoSetExtType[k] != UNIDRCLOUDEXT_TERM)
    {
        /*  get bit count for extension payload; add future extensions here */
        switch(loudnessInfoSetExtension->loudnessInfoSetExtType[k])
        {
#if AMD1_SYNTAX
            case UNIDRCLOUDEXT_EQ:
                err = putBits(bitstreamTmp, 6, loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount);
                if (err) return(err);
                err = putBits(bitstreamTmp, 6, loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count);
                if (err) return(err);
                for (i=0; i<loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; i++) {
                    err = writeLoudnessInfo(bitstreamTmp, version, &(loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Album[i]));
                    if (err) return(err);
                }
                for (i=0; i<loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count; i++) {
                    err = writeLoudnessInfo(bitstreamTmp, version, &(loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1[i]));
                    if (err) return(err);
                }
                break;
#endif /* AMD1_SYNTAX */
            default:
                break;
        }
        loudnessInfoSetExtension->extBitSize[k] = wobitbuf_GetBitsWritten(bitstreamTmp);
        bitSize     = loudnessInfoSetExtension->extBitSize[k] - 1;
        extSizeBits = (int)(log((float)bitSize)/log(2.f))+1;
        bitSizeLen  = extSizeBits - 4;
        err = putBits(bitstream, 4, bitSizeLen);
        if (err) return(err);
        err = putBits(bitstream, extSizeBits, bitSize);
        if (err) return(err);
        switch(loudnessInfoSetExtension->loudnessInfoSetExtType[k])
        {
#if AMD1_SYNTAX
            case UNIDRCLOUDEXT_EQ:
                err = putBits(bitstream, 6, loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount);
                if (err) return(err);
                err = putBits(bitstream, 6, loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count);
                if (err) return(err);
                for (i=0; i<loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; i++) {
                    err = writeLoudnessInfo(bitstream, version, &(loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Album[i]));
                    if (err) return(err);
                }
                for (i=0; i<loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count; i++) {
                    err = writeLoudnessInfo(bitstream, version, &(loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1[i]));
                    if (err) return(err);
                }
                break;
#endif /* AMD1_SYNTAX */
                /* add future extensions here */
            default:
                for(i = 0; i<loudnessInfoSetExtension->extBitSize[k]; i++)
                {
                    err = putBits(bitstream, 1, tmp);  /* It should not be necessary to write any bits here */
                    if (err) return(err);
                }
        }
        k++;
        err = putBits(bitstream, 4, loudnessInfoSetExtension->loudnessInfoSetExtType[k]);
        if (err) return(err);
    }

    return (0);
}


int
writeLoudnessInfoSet(HANDLE_DRC_ENCODER hDrcEnc,
                     unsigned char* bitstreamBuffer,
                     int* bitCount)
{
    int i, err, version = 0;
#if MPEG_H_SYNTAX
    int bits;
#endif

    LoudnessInfoSet* loudnessInfoSet = &(hDrcEnc->loudnessInfoSet);
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);

#if MPEG_H_SYNTAX
    err = putBits(bitstream, 6, loudnessInfoSet->loudnessInfoCount);
    if (err) return(err);

    for(i=0; i<loudnessInfoSet->loudnessInfoCount; i++)
    {
        putBits(bitstream, 2, loudnessInfoSet->loudnessInfo[i].loudnessInfoType);
        if ((loudnessInfoSet->loudnessInfo[i].loudnessInfoType == 1) || (loudnessInfoSet->loudnessInfo[i].loudnessInfoType == 2))
            putBits(bitstream, 7, loudnessInfoSet->loudnessInfo[i].mae_groupID);
        else if (loudnessInfoSet->loudnessInfo[i].loudnessInfoType == 3)
            putBits(bitstream, 5, loudnessInfoSet->loudnessInfo[i].mae_groupPresetID);

        writeLoudnessInfo(bitstream, version, &loudnessInfoSet->loudnessInfo[i]);
    }
    
    /* loudnessInfoAlbumPresent */
    if(loudnessInfoSet->loudnessInfoAlbumCount == 0)
    {
        bits = 0;
        err = putBits(bitstream, 1, bits);
        if (err) return(err);
    }
    else
    {
        bits = 1;
        err = putBits(bitstream, 1, bits);
        if (err) return(err);
        
        err = putBits(bitstream, 6, loudnessInfoSet->loudnessInfoAlbumCount);
        if (err) return(err);
        for(i=0; i<loudnessInfoSet->loudnessInfoAlbumCount; i++)
        {
            writeLoudnessInfo(bitstream, version, &loudnessInfoSet->loudnessInfoAlbum[i]);
        }
    }

    err = putBits(bitstream, 1, loudnessInfoSet->loudnessInfoSetExtPresent);
    if (loudnessInfoSet->loudnessInfoSetExtPresent)
    {
        writeLoudnessInfoSetExtension(bitstream, &loudnessInfoSet->loudnessInfoSetExtension);
    }
#else /* MPEG_H_SYNTAX */
    err = putBits(bitstream, 6, loudnessInfoSet->loudnessInfoAlbumCount);
    if (err) return(err);
    err = putBits(bitstream, 6, loudnessInfoSet->loudnessInfoCount);
    if (err) return(err);

    for(i=0;i<loudnessInfoSet->loudnessInfoAlbumCount;i++)
    {
        err = writeLoudnessInfo(bitstream, version, &loudnessInfoSet->loudnessInfoAlbum[i]);
        if (err) return(err);
    }

    for(i=0;i<loudnessInfoSet->loudnessInfoCount;i++)
    {
        err = writeLoudnessInfo(bitstream, version, &loudnessInfoSet->loudnessInfo[i]);
        if (err) return(err);
    }

    err = putBits(bitstream, 1, loudnessInfoSet->loudnessInfoSetExtPresent);
    if (err) return(err);
    
    if(loudnessInfoSet->loudnessInfoSetExtPresent)
    {
        err = writeLoudnessInfoSetExtension(bitstream, &loudnessInfoSet->loudnessInfoSetExtension);
        if (err) return(err);
    }
#endif /* MPEG_H_SYNTAX */

    *bitCount += wobitbuf_GetBitsWritten(bitstream);

    return (0);
}

#ifdef DEBUG_OUTPUT_FORMAT
#if MPEG_H_SYNTAX
int
writeLoudnessInfoStruct(LoudnessInfo *loudnessInfo,
                        unsigned char* bitstreamBuffer,
                        int* bitCount)
{
    int err;
    
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);

    err = writeLoudnessInfo(bitstream, loudnessInfo);
    if (err) return(err);
    
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}
#endif
#endif /* DEBUG_OUTPUT_FORMAT */

/* ====================================================================================
         Write the uniDrc():
    The in-stream configuration appears in first frame, but can be repeated
 ====================================================================================*/

int
writeUniDrc(HANDLE_DRC_ENCODER hDrcEnc,
            const int includeLoudnessInfoSet,
            const int includeUniDrcConfig,
            UniDrcGainExt uniDrcGainExtension,
            unsigned char* bitstreamBuffer,
            int* bitCount)
{
    int err;
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);
    
    /* write loudnessInfoSetPresent bit */
    err = putBits(bitstream, 1, includeLoudnessInfoSet);
    if (err) return(err);
    *bitCount += 1;

    if (includeLoudnessInfoSet == TRUE)
    {
        /* write uniDrcConfigPresent bit */
        err = putBits(bitstream, 1, includeUniDrcConfig);
        if (err) return(err);
        *bitCount += 1;

        if (includeUniDrcConfig == TRUE)
        {
            err = writeUniDrcConfig(hDrcEnc, bitstreamBuffer, bitCount);
            if (err) return(err);
        }
        err = writeLoudnessInfoSet(hDrcEnc, bitstreamBuffer, bitCount);
        if (err) return(err);
    }
    
    err = writeUniDrcGain(hDrcEnc,
                          uniDrcGainExtension,
                          bitstreamBuffer,
                          bitCount);
    if (err) return(err);

    return (0);
}

/* ====================================================================================
 Write ISOBMFF configuration
 ====================================================================================*/
#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX

int
writeIsobmffLoudnessBaseBox( LoudnessInfo *loudnessInfo,
                             int loudnessBaseCount,
                             int version,
                             wobitbufHandle bitstream)
{
    int i, j, err, reserved = 0, code = 0, codeSize = 0;
    LoudnessInfo *loudnessInfoTmp;
    if (version >= 1) {
        err = putBits(bitstream, 2, reserved);
        if (err) return(err);
        err = putBits(bitstream, 6, loudnessBaseCount);
        if (err) return(err);
    } else {
        loudnessBaseCount = 1;
    }
    
    for (i=0; i<loudnessBaseCount; i++) {
        
        loudnessInfoTmp = &loudnessInfo[i];
        if (version >= 1) {
            err = putBits(bitstream, 2, reserved);
            if (err) return(err);
            err = putBits(bitstream, 6, loudnessInfo[i].eqSetId);
            if (err) return(err);
        }
        
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 7, loudnessInfo[i].downmixId);
        if (err) return(err);
        err = putBits(bitstream, 6, loudnessInfo[i].drcSetId);
        if (err) return(err);
        if (loudnessInfo[i].samplePeakLevelPresent == 1) {
            err = encPeak(loudnessInfo[i].samplePeakLevel, &code, &codeSize);
            if (err) return(err);
        } else {
            code = 0;
            codeSize = 12;
        }
        err = putBits(bitstream, codeSize, code);
        if (err) return(err);
        if (loudnessInfo[i].truePeakLevelPresent == 1) {
            err = encPeak(loudnessInfo[i].truePeakLevel, &code, &codeSize);
            if (err) return(err);
        } else {
            code = 0;
            codeSize = 12;
        }
        err = putBits(bitstream, codeSize, code);
        if (err) return(err);
        err = putBits(bitstream, 4, loudnessInfo[i].truePeakLevelMeasurementSystem);
        if (err) return(err);
        err = putBits(bitstream, 4, loudnessInfo[i].truePeakLevelReliability);
        if (err) return(err);
        err = putBits(bitstream, 8, loudnessInfo[i].measurementCount);
        if (err) return(err);
        
        for (j=0; j<loudnessInfo[i].measurementCount;j++) {
            err = putBits(bitstream, 8, loudnessInfo[i].loudnessMeasure[j].methodDefinition);
            if (err) return(err);
            err = encMethodValue(loudnessInfo[i].loudnessMeasure[j].methodDefinition,
                                 loudnessInfo[i].loudnessMeasure[j].methodValue,
                                 &codeSize,
                                 &code);
            err = putBits(bitstream, 8, code);
            if (err) return(err);
            err = putBits(bitstream, 4, loudnessInfo[i].loudnessMeasure[j].measurementSystem);
            if (err) return(err);
            err = putBits(bitstream, 4, loudnessInfo[i].loudnessMeasure[j].reliability);
            if (err) return(err);
        }
    }
    return (0);
}

int
writeIsobmffLoudnessInfo(LoudnessInfo *loudnessInfo,
                         int loudnessBaseCount,
                         int version,
                         int size,
                         char *type,
                         wobitbufHandle bitstream)
{
    int err, flags = 0;
    
    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    } else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    /* loudnessInfo */
    err = writeIsobmffLoudnessBaseBox( loudnessInfo,
                                       loudnessBaseCount,
                                       version,
                                       bitstream);
    if (err) return(err);
    
    return (0);
}

/* Writer for ISO base media file format 'ludt' box */
int
writeIsobmffLudt(HANDLE_DRC_ENCODER hDrcEnc,
                 unsigned char* bitstreamBuffer,
                 int size,
                 int* bitCount)
{
    int i, err, bitCountTmp = 0, loudnessInfoCount = 0;
    int size_alou = 0, size_tlou = 0;
    int version = 1; /* fixed to newest version */
    LoudnessInfo *loudnessInfoAlbum = NULL, *loudnessInfo = NULL;
    int loudnessInfoAlbumCount, loudnessInfoAlbumCountLoop, loudnessInfoCountLoop;
    
    char ludt[] = "ludt";
    char tlou[] = "tlou";
    char alou[] = "alou";
    
    LoudnessInfoSet* loudnessInfoSet = &(hDrcEnc->loudnessInfoSet);
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    
    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    wobitbuf bsTmp;
    wobitbufHandle bitstreamTmp = &bsTmp;
    
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);
    wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS, 0);
    
    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    } else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, ludt[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, ludt[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, ludt[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, ludt[3]);
    if (err) return(err);
    
    if(loudnessInfoSet->loudnessInfoSetExtPresent == 1) {
        loudnessInfoAlbum = loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Album;
        loudnessInfoAlbumCount = loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount;
        loudnessInfo = loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1;
        loudnessInfoCount = loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count;
    }
    else {
        loudnessInfoAlbum = loudnessInfoSet->loudnessInfoAlbum;
        loudnessInfoAlbumCount = loudnessInfoSet->loudnessInfoAlbumCount;
        loudnessInfo = loudnessInfoSet->loudnessInfo;
        loudnessInfoCount = loudnessInfoSet->loudnessInfoCount;
    }
    
    /* write 'alou' */
    if (version >= 1) {
        loudnessInfoAlbumCountLoop = 1;
    }
    else {
        loudnessInfoAlbumCountLoop = loudnessInfoAlbumCount;
    }
    
    for(i=0;i<loudnessInfoAlbumCountLoop;i++) {
        wobitbuf_Reset(bitstreamTmp);
        writeIsobmffLoudnessInfo(&loudnessInfoAlbum[i],
                                 loudnessInfoAlbumCount,
                                 version,
                                 -1,
                                 alou,
                                 bitstreamTmp);
        bitCountTmp = wobitbuf_GetBitsWritten(bitstreamTmp);
        size_alou = bitCountTmp / 8;
        writeIsobmffLoudnessInfo(&loudnessInfoAlbum[i],
                                 loudnessInfoAlbumCount,
                                 version,
                                 size_alou,
                                 alou,
                                 bitstream);
    }
    
    /* write 'tlou' */
    if (version >= 1) {
        loudnessInfoCountLoop = 1;
    }
    else {
        loudnessInfoCountLoop = loudnessInfoCount;
    }
    
    for(i=0;i<loudnessInfoCountLoop;i++) {
        wobitbuf_Reset(bitstreamTmp);
        writeIsobmffLoudnessInfo(&loudnessInfo[i],
                                 loudnessInfoCount,
                                 version,
                                 -1,
                                 tlou,
                                 bitstreamTmp);
        bitCountTmp = wobitbuf_GetBitsWritten(bitstreamTmp);
        size_tlou = bitCountTmp / 8;
        writeIsobmffLoudnessInfo(&loudnessInfo[i],
                                 loudnessInfoCount,
                                 version,
                                 size_tlou,
                                 tlou,
                                 bitstream);
    }
    
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}

/* Writer for ISO base media file format loudness */
int
writeIsobmffLoudness(HANDLE_DRC_ENCODER hDrcEnc,
                     unsigned char* bitstreamBuffer,
                     int* bitCount)
{
    int err, bitCountTmp = 0, size = 0;
    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    
    /* write ludt for getting size */
    size = -1;
    err = writeIsobmffLudt(hDrcEnc, bitstreamBufferTmp, size, &bitCountTmp);
    if (err) return(err);

    /* write ludt */
    size = bitCountTmp / 8;
    err = writeIsobmffLudt(hDrcEnc, bitstreamBuffer, size, bitCount);
    if (err) return(err);
    
    if (size != (*bitCount)/8) return (UNEXPECTED_ERROR);
    
    return (0);
}


/* Writer for ISO base media file format 'chnl' box */
int
writeIsobmffChnl(HANDLE_DRC_ENCODER hDrcEnc,
                 wobitbufHandle bitstream,
                 int size,
                 int* bitCount)
{
    int err, i, reserved = 0;
    int version = 1; /* fixed to newest version */
    int stream_structure = 1; /* channels only */
    int channelStructured = 1, objectStructured = 2, format_ordering = 0, channel_order_definition = 0, omitted_channels_present = 0, objectCount = 0;
    int layout_channel_count = hDrcEnc->uniDrcConfig.channelLayout.baseChannelCount; /* use baseChannelCount for channels only */
    int azimuth[SPEAKER_POS_COUNT_MAX] = {0}, elevation[SPEAKER_POS_COUNT_MAX] = {0};

    int flags = 0;
    
    char type[] = "chnl";
    
    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    if (version == 0) {
        err = putBits(bitstream, 8, stream_structure);
        if (err) return(err);
        if (stream_structure & channelStructured) {
            err = putBits(bitstream, 8, hDrcEnc->uniDrcConfig.channelLayout.definedLayout);
            if (err) return(err);
            if (hDrcEnc->uniDrcConfig.channelLayout.definedLayout==0) {
                for (i = 0; i<layout_channel_count; i++) {
                    err = putBits(bitstream, 8, hDrcEnc->uniDrcConfig.channelLayout.speakerPosition[i]);
                    if (err) return(err);
                    if (hDrcEnc->uniDrcConfig.channelLayout.speakerPosition[i] == 126) {
                        err = putBits(bitstream, 16, azimuth[i]);
                        if (err) return(err);
                        err = putBits(bitstream, 8, elevation[i]);
                        if (err) return(err);
                    }
                }
            } else {
                /* omittedChannelsMap not supported */
                err = putBits(bitstream, 32, 0);
                if (err) return(err);
                err = putBits(bitstream, 32, 0);
                if (err) return(err);
            }
        }
        if (stream_structure & objectStructured) {
            err = putBits(bitstream, 8, objectCount);
            if (err) return(err);
        }
    } else {
        err = putBits(bitstream, 4, stream_structure);
        if (err) return(err);
        err = putBits(bitstream, 4, format_ordering);
        if (err) return(err);
        err = putBits(bitstream, 8, hDrcEnc->uniDrcConfig.channelLayout.baseChannelCount);
        if (err) return(err);
        if (stream_structure & channelStructured) {
            err = putBits(bitstream, 8, hDrcEnc->uniDrcConfig.channelLayout.definedLayout);
            if (err) return(err);
            if (hDrcEnc->uniDrcConfig.channelLayout.definedLayout==0) {
                err = putBits(bitstream, 8, layout_channel_count);
                if (err) return(err);
                for (i = 0; i<layout_channel_count; i++) {
                    err = putBits(bitstream, 8, hDrcEnc->uniDrcConfig.channelLayout.speakerPosition[i]);
                    if (err) return(err);
                    if (hDrcEnc->uniDrcConfig.channelLayout.speakerPosition[i] == 126) {
                        err = putBits(bitstream, 16, azimuth[i]);
                        if (err) return(err);
                        err = putBits(bitstream, 8, elevation[i]);
                        if (err) return(err);
                    }
                }
            } else {
                err = putBits(bitstream, 4, reserved);
                if (err) return(err);
                err = putBits(bitstream, 3, channel_order_definition);
                if (err) return(err);
                err = putBits(bitstream, 1, omitted_channels_present);
                if (err) return(err);
                if (omitted_channels_present == 1) {
                    /* omittedChannelsMap not supported */
                    err = putBits(bitstream, 32, 0);
                    if (err) return(err);
                    err = putBits(bitstream, 32, 0);
                    if (err) return(err);
                }
            }
        }
        if (stream_structure & objectStructured) {
            /* object_count is derived from baseChannelCount */
        }
    }
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}

int
writeIsobmffDownmixCoefficientV1( wobitbufHandle bitstream,
                                  const float downmixCoefficient[],
                                  const int baseChannelCount,
                                  const int targetChannelCount)
{
    int err=0, i, k, bsDownmixOffset = 0, code, reserved = 0, size;
    float downmixOffset[3];
    float tmp;
    float quantErr, quantErrMin;
    const float* coeffTable;

    coeffTable = downmixCoeffV1;
    tmp = log10((float)targetChannelCount/(float)baseChannelCount);
    downmixOffset[0] = 0.0f;
    downmixOffset[1] = 0.5f * floor (0.5f + 20.0f * tmp);
    downmixOffset[2] = 0.5f * floor (0.5f + 40.0f * tmp);
    
    quantErrMin = 1000.0f;
    for (i=0; i<3; i++) {
        quantErr = 0.0f;
        for (k=0; k<targetChannelCount * baseChannelCount; k++)
        {
            code = encodeDownmixCoefficient(downmixCoefficient[k], downmixOffset[i]);
            quantErr += fabs (20.0f * log10(downmixCoefficient[k]) - (coeffTable[code] + downmixOffset[i]));
        }
        if (quantErrMin > quantErr) {
            quantErrMin = quantErr;
            bsDownmixOffset = i;
        }
    }
    
    err = putBits(bitstream, 4, bsDownmixOffset);
    if (err) return(err);
    
    for (k=0; k<targetChannelCount * baseChannelCount; k++)
    {
        code = encodeDownmixCoefficient(downmixCoefficient[k], downmixOffset[bsDownmixOffset]);
        err = putBits(bitstream, 5, code);
        if (err) return(err);
    }
    size = 4 + k*5;
    size = size%8;
    if(size) {
        err = putBits(bitstream, 8-size, reserved);
        if (err) return(err);
    }
    
    return (0);
}
int
writeIsobmffDownmixInstructions(DownmixInstructions* downmixInstructions,
                                int downmixInstructionsCount,
                                int version,
                                wobitbufHandle bitstream,
                                int baseChannelCount)
{
    int err = 0, reserved = 0, k = 0, in_stream;
    
    if (version >= 1) {
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 7, downmixInstructionsCount);
        if (err) return(err);
    }
    
    for(k = 0; k < downmixInstructionsCount; k++) {
        err = putBits(bitstream, 8, downmixInstructions[k].targetLayout);
        if (err) return(err);
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 7, downmixInstructions[k].targetChannelCount);
        if (err) return(err);
        
        in_stream = 0;
        if(downmixInstructions[k].downmixCoefficientsPresent == 0) {
            return -2;
        }
        err = putBits(bitstream, 1, in_stream);
        if (err) return(err);
        err = putBits(bitstream, 7, downmixInstructions[k].downmixId);
        if (err) return(err);
        
        if(in_stream == 0) {
            if (version == 1) {
                err = writeIsobmffDownmixCoefficientV1( bitstream,
                                                        downmixInstructions[k].downmixCoefficient,
                                                        baseChannelCount,
                                                        downmixInstructions[k].targetChannelCount);
                if (err) return(err);
            }
            else {
                /* TODO need LFE info */
                int isLfeChannel = FALSE, i, codeSize, code;
                for (i=0; i<downmixInstructions[k].targetChannelCount * baseChannelCount; i++)  /*???*/
                {
                    err = encDownmixCoefficient(downmixInstructions[k].downmixCoefficient[i], isLfeChannel, &codeSize, &code);
                    err = putBits(bitstream, codeSize, code);
                    if (err) return(err);
                }
            }
        }
    }
    
    return (0);
}

/* Writer for ISO base media file format 'dmix' box */
int
writeIsobmffDmix(HANDLE_DRC_ENCODER hDrcEnc,
                 wobitbufHandle bitstream,
                 int size,
                 int* bitCount)
{
    int err;
    int version = 1; /* fixed to newest version */
    int flags = 0;
    
    char type[] = "dmix";
    
    DownmixInstructions* downmixInstructions = hDrcEnc->uniDrcConfig.downmixInstructions;

    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    /* write downmixInstructions */
    if(hDrcEnc->uniDrcConfig.uniDrcConfigExtPresent == 1 && hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1Present) {
        writeIsobmffDownmixInstructions(hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1, hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1Count, version, bitstream, hDrcEnc->baseChannelCount);
    }
    else {
        writeIsobmffDownmixInstructions(hDrcEnc->uniDrcConfig.downmixInstructions, hDrcEnc->uniDrcConfig.downmixInstructionsCount, version, bitstream, hDrcEnc->baseChannelCount);
    }
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}

int
writeIsobmffSplitDrcCharacteristic(wobitbufHandle bitstream,
                                   const int side,
                                   SplitDrcCharacteristic* splitCharacteristic)
{
    int err, i, reserved = 0;
    err = putBits(bitstream, 7, reserved);
    if (err) return(err);
    err = putBits(bitstream, 1, splitCharacteristic->characteristicFormat);
    if (err) return(err);
    if (splitCharacteristic->characteristicFormat == 0) {
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 6, splitCharacteristic->bsGain); /*// TODO: bsGain? -/+? Is this correct? At the decoder there is a decision for left/right */
        if (err) return(err);
        err = putBits(bitstream, 4, splitCharacteristic->bsIoRatio);
        if (err) return(err);
        err = putBits(bitstream, 4, splitCharacteristic->bsExp);
        if (err) return(err);
        err = putBits(bitstream, 1, splitCharacteristic->flipSign);
        if (err) return(err);
    }
    else {
        float bsNodeLevelPrevious = DRC_INPUT_LOUDNESS_TARGET;
        int bsNodeLevelDelta;
        int bsNodeGain;
        err = putBits(bitstream, 6, reserved);
        if (err) return(err);
        err = putBits(bitstream, 2, splitCharacteristic->characteristicNodeCount-1);
        if (err) return(err);
        for (i=1; i<=splitCharacteristic->characteristicNodeCount; i++) {
            bsNodeLevelDelta = floor (fabs(splitCharacteristic->nodeLevel[i] - bsNodeLevelPrevious) + 0.5f) - 1;
            if (bsNodeLevelDelta < 0) bsNodeLevelDelta = 0;
            if (bsNodeLevelDelta > 31) bsNodeLevelDelta = 31;
            if (side == LEFT_SIDE) {
                bsNodeLevelPrevious = bsNodeLevelPrevious - (bsNodeLevelDelta + 1);
            }
            else {
                bsNodeLevelPrevious = bsNodeLevelPrevious + (bsNodeLevelDelta + 1);
            }
            err = putBits(bitstream, 3, reserved);
            if (err) return(err);
            err = putBits(bitstream, 5, bsNodeLevelDelta);
            if (err) return(err);
            bsNodeGain = floor ((splitCharacteristic->nodeGain[i] + 64.0f) * 2.0f + 0.5f);
            if (bsNodeGain < 0) bsNodeGain = 0;
            if (bsNodeGain > 255) bsNodeGain = 255;
            err = putBits(bitstream, 8, bsNodeGain);
            if (err) return(err);
        }
    }
    return (0);
}

static int writeIsobmffShapeFilterBlockParams(wobitbufHandle bitstream, ShapeFilterBlockParams* shapeFilterBlockParams)
{
    int err, reserved = 0;
    
    err = putBits(bitstream, 4, reserved);
    if (err) return(err);
    err = putBits(bitstream, 1, shapeFilterBlockParams->lfCutFilterPresent);
    if (err) return(err);
    err = putBits(bitstream, 1, shapeFilterBlockParams->lfBoostFilterPresent);
    if (err) return(err);
    err = putBits(bitstream, 1, shapeFilterBlockParams->hfCutFilterPresent);
    if (err) return(err);
    err = putBits(bitstream, 1, shapeFilterBlockParams->hfBoostFilterPresent);
    if (err) return(err);
    
    if (shapeFilterBlockParams->lfCutFilterPresent == 1) {
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 3, shapeFilterBlockParams->lfCutParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->lfCutParams.filterStrengthIndex);
        if (err) return(err);
    }

    if (shapeFilterBlockParams->lfBoostFilterPresent == 1) {
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 3, shapeFilterBlockParams->lfBoostParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->lfBoostParams.filterStrengthIndex);
        if (err) return(err);
    }
    if (shapeFilterBlockParams->hfCutFilterPresent == 1) {
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 3, shapeFilterBlockParams->hfCutParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->hfCutParams.filterStrengthIndex);
        if (err) return(err);
    }
    if (shapeFilterBlockParams->hfBoostFilterPresent == 1) {
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 3, shapeFilterBlockParams->hfBoostParams.cornerFreqIndex);
        if (err) return(err);
        err = putBits(bitstream, 2, shapeFilterBlockParams->hfBoostParams.filterStrengthIndex);
        if (err) return(err);
    }
    return (0);
}

int
writeIsobmffGainParamsCharacteristics(wobitbufHandle bitstream,
                                      int version,
                                      GainParams *gainParams)
{
    int err, reserved = 0;
    
    if (version == 0) {
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 7, gainParams->drcCharacteristic);
        if (err) return(err);
    }
    else {
        err = putBits(bitstream, 1, gainParams->drcCharacteristicPresent);
        if (err) return(err);
        err = putBits(bitstream, 1, gainParams->drcCharacteristicFormatIsCICP);
        if (err) return(err);
        if (gainParams->drcCharacteristicPresent) {
            if (gainParams->drcCharacteristicFormatIsCICP) {
                err = putBits(bitstream, 1, reserved);
                if (err) return(err);
                err = putBits(bitstream, 7, gainParams->drcCharacteristic);
                if (err) return(err);
            }
            else {
                err = putBits(bitstream, 4, gainParams->drcCharacteristicLeftIndex);
                if (err) return(err);
                err = putBits(bitstream, 4, gainParams->drcCharacteristicRightIndex);
                if (err) return(err);
            }
        }
    }
    
    return (0);
}

int
writeIsobmffGainParamsCrossoverFreqIndex(wobitbufHandle bitstream,
                                         int drcBandType,
                                         GainParams *gainParams)
{
    int err, reserved = 0;
    if (drcBandType == 1) {
        err = putBits(bitstream, 4, reserved);
        if (err) return(err);
        err = putBits(bitstream, 4, gainParams->crossoverFreqIndex);
        if (err) return(err);
    }
    else {
        err = putBits(bitstream, 6, reserved);
        if (err) return(err);
        err = putBits(bitstream, 10, gainParams->startSubBandIndex);
        if (err) return(err);
    }
    
    return (0);
}
int
writeIsobmffGainSetParams(wobitbufHandle bitstream,
                          const int version,
                          GainSetParams* gainSetParams)
{
    int err, reserved = 0, k;
    
    err = putBits(bitstream, 2, reserved);
    if (err) return(err);
    err = putBits(bitstream, 2, gainSetParams->gainCodingProfile);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->gainInterpolationType);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->fullFrame);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->timeAlignment);
    if (err) return(err);
    err = putBits(bitstream, 1, gainSetParams->timeDeltaMinPresent);
    if (err) return(err);
    if (gainSetParams->timeDeltaMinPresent) {
        err = putBits(bitstream, 5, reserved);
        if (err) return(err);
        err = putBits(bitstream, 11, gainSetParams->deltaTmin-1);
        if (err) return(err);
    }
    if (gainSetParams->gainCodingProfile != GAIN_CODING_PROFILE_CONSTANT) {
        err = putBits(bitstream, 3, reserved);
        if (err) return(err);
        err = putBits(bitstream, 4, gainSetParams->bandCount);
        if (err) return(err);
        err = putBits(bitstream, 1, gainSetParams->drcBandType);
        if (err) return(err);
        for(k = 0; k < gainSetParams->bandCount; k++) {
            if(version >= 1) {
                err = putBits(bitstream, 6, gainSetParams->gainParams[k].gainSequenceIndex);
                if (err) return(err);
            }
            err = writeIsobmffGainParamsCharacteristics(bitstream, version, &gainSetParams->gainParams[k]);
        }
        for(k = 1; k < gainSetParams->bandCount; k++) {
            err = writeIsobmffGainParamsCrossoverFreqIndex(bitstream, gainSetParams->drcBandType, &gainSetParams->gainParams[k]);
        }

    }
    return(0);
}

int
writeIsobmffDrcCoefficientsUniDrc(DrcCoefficientsUniDrc* drcCoefficientsUniDrc,
                                  int version,
                                  int delayMode,
                                  wobitbufHandle bitstream)
{
    int err = 0, reserved = 0, k = 0, i = 0;
    
    err = putBits(bitstream, 2, reserved);
    if (err) return(err);
    err = putBits(bitstream, 5, drcCoefficientsUniDrc->drcLocation);
    if (err) return(err);
    err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcFrameSizePresent);
    if (err) return(err);
    if(drcCoefficientsUniDrc->drcFrameSizePresent == 1) {
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 15, drcCoefficientsUniDrc->drcFrameSize-1);
        if (err) return(err);
    }
    
    if (version >= 1) {
        err = putBits(bitstream, 5, reserved);
        if (err) return(err);
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcCharacteristicLeftPresent);
        if (err) return(err);
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->drcCharacteristicRightPresent);
        if (err) return(err);
        err = putBits(bitstream, 1, drcCoefficientsUniDrc->shapeFiltersPresent);
        if (err) return(err);
        if(drcCoefficientsUniDrc->drcCharacteristicLeftPresent == 1) {
            err = putBits(bitstream, 4, reserved);
            if (err) return(err);
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->characteristicLeftCount);
            if (err) return(err);
            for (k = 1; k <= drcCoefficientsUniDrc->characteristicLeftCount; k++) {
                writeIsobmffSplitDrcCharacteristic(bitstream, LEFT_SIDE, &drcCoefficientsUniDrc->splitCharacteristicLeft[k]);
            }
        }
        
        if(drcCoefficientsUniDrc->drcCharacteristicRightPresent == 1) {
            err = putBits(bitstream, 4, reserved);
            if (err) return(err);
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->characteristicRightCount);
            if (err) return(err);
            for (k = 1; k <= drcCoefficientsUniDrc->characteristicRightCount; k++) {
                writeIsobmffSplitDrcCharacteristic(bitstream, RIGHT_SIDE, &drcCoefficientsUniDrc->splitCharacteristicRight[k]);
            }
        }
        
        if(drcCoefficientsUniDrc->shapeFiltersPresent)
        {
            err = putBits(bitstream, 4, reserved);
            if (err) return(err);
            err = putBits(bitstream, 4, drcCoefficientsUniDrc->shapeFilterCount);
            if (err) return(err);
            for (i=1; i<=drcCoefficientsUniDrc->shapeFilterCount; i++)
            {
                err = writeIsobmffShapeFilterBlockParams(bitstream, &drcCoefficientsUniDrc->shapeFilterBlockParams[i]);
                if (err) return(err);
            }
        }
    }
    
    err = putBits(bitstream, 1, reserved);
    if (err) return(err);
    err = putBits(bitstream, 1, delayMode);
    if (err) return(err);
    
    if(version >= 1) {
        err = putBits(bitstream, 2, reserved);
        if (err) return(err);
        err = putBits(bitstream, 6, drcCoefficientsUniDrc->gainSequenceCount);
        if (err) return(err);
    }
    err = putBits(bitstream, 6, drcCoefficientsUniDrc->gainSetCount);
    if (err) return(err);
    
    for(k = 0; k < drcCoefficientsUniDrc->gainSetCount; k++) {
        writeIsobmffGainSetParams(bitstream, version, &drcCoefficientsUniDrc->gainSetParams[k]);
    }
    
    return (0);
}

/* Writer for ISO base media file format 'udc2' box */
int
writeIsobmffUdc2(HANDLE_DRC_ENCODER hDrcEnc,
                 wobitbufHandle bitstream,
                 int size,
                 int* bitCount)
{
    int err;
    int version = 1; /* fixed to newest version */
    int flags = 0;
    
    char type[] = "udc2";

    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    /* write drcCoefficientsUniDrc */
    if(hDrcEnc->uniDrcConfig.uniDrcConfigExtPresent == 1 && hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
        writeIsobmffDrcCoefficientsUniDrc(&hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0], version, hDrcEnc->delayMode, bitstream);
    }
    else {
        writeIsobmffDrcCoefficientsUniDrc(&hDrcEnc->uniDrcConfig.drcCoefficientsUniDrc[0], version, hDrcEnc->delayMode, bitstream);
    }
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}

int
writeIsobmffGainModifiers( wobitbufHandle bitstream,
                           const int version,
                           const int bandCount,
                           GainModifiers* gainModifiers)
{
    int err, tmp, sign, reserved=0;
    int b;
    
    
    if (version >= 1) {
        for (b=0; b<bandCount; b++) {
            err = putBits(bitstream, 4, reserved);
            if (err) return(err);
            err = putBits(bitstream, 1, gainModifiers->targetCharacteristicLeftPresent[b]);
            if (err) return(err);
            err = putBits(bitstream, 1, gainModifiers->targetCharacteristicRightPresent[b]);
            if (err) return(err);
            err = putBits(bitstream, 1, gainModifiers->gainScalingPresent[b]);
            if (err) return(err);
            err = putBits(bitstream, 1, gainModifiers->gainOffsetPresent[b]);
            if (err) return(err);
            
            if (gainModifiers->targetCharacteristicLeftPresent[b])
            {
                err = putBits(bitstream, 4, reserved);
                if (err) return(err);
                err = putBits(bitstream, 4, gainModifiers->targetCharacteristicLeftIndex[b]);
                if (err) return(err);
            }
            
            if (gainModifiers->targetCharacteristicRightPresent[b])
            {
                err = putBits(bitstream, 4, reserved);
                if (err) return(err);
                err = putBits(bitstream, 4, gainModifiers->targetCharacteristicRightIndex[b]);
                if (err) return(err);
            }
            
            if(gainModifiers->gainScalingPresent[b])
            {
                tmp = (int)(0.5f + 8.0f * gainModifiers->attenuationScaling[b]);
                err = putBits(bitstream, 4, tmp);
                if (err) return(err);
                tmp = (int)(0.5f + 8.0f * gainModifiers->amplificationScaling[b]);
                err = putBits(bitstream, 4, tmp);
                if (err) return(err);
            }
            
            if(gainModifiers->gainOffsetPresent[b])
            {
                err = putBits(bitstream, 2, reserved);
                if (err) return(err);
                
                if (gainModifiers->gainOffset[b] < 0.0f)
                {
                    tmp = (int)(0.5f + max(0.0f, - 4.0f * gainModifiers->gainOffset[b] - 1.0f));
                    sign = 1;
                }
                else
                {
                    tmp = (int)(0.5f + max(0.0f, 4.0f * gainModifiers->gainOffset[b] - 1.0f));
                    sign = 0;
                }
                err = putBits(bitstream, 1, sign);
                if (err) return(err);
                err = putBits(bitstream, 5, tmp);
                if (err) return(err);
            }
        }
        if (bandCount == 1)
        {
            err = putBits(bitstream, 1, gainModifiers->shapeFilterPresent);
            if (err) return(err);
            
            if (gainModifiers->shapeFilterPresent)
            {
                err = putBits(bitstream, 3, reserved);
                if (err) return(err);
                
                err = putBits(bitstream, 4, gainModifiers->shapeFilterIndex);
                if (err) return(err);
            }
            else {
                err = putBits(bitstream, 7, reserved);
                if (err) return(err);
            }
        }
    }
    else {
        err = putBits(bitstream, 7, reserved);
        if (err) return(err);
        err = putBits(bitstream, 1, gainModifiers->gainScalingPresent[0]);
        if (err) return(err);
        
        if(gainModifiers->gainScalingPresent[0])
        {
            tmp = (int)(0.5f + 8.0f * gainModifiers->attenuationScaling[0]);
            err = putBits(bitstream, 4, tmp);
            if (err) return(err);
            tmp = (int)(0.5f + 8.0f * gainModifiers->amplificationScaling[0]);
            err = putBits(bitstream, 4, tmp);
            if (err) return(err);
        }
        
        err = putBits(bitstream, 7, reserved);
        if (err) return(err);
        err = putBits(bitstream, 1, gainModifiers->gainOffsetPresent[0]);
        if (err) return(err);
        if(gainModifiers->gainOffsetPresent[0])
        {
            if (gainModifiers->gainOffset[0] < 0.0f)
            {
                tmp = (int)(0.5f + max(0.0f, - 4.0f * gainModifiers->gainOffset[0] - 1.0f));
                sign = 1;
            }
            else
            {
                tmp = (int)(0.5f + max(0.0f, 4.0f * gainModifiers->gainOffset[0] - 1.0f));
                sign = 0;
            }
            err = putBits(bitstream, 2, reserved);
            if (err) return(err);
            err = putBits(bitstream, 1, sign);
            if (err) return(err);
            err = putBits(bitstream, 5, tmp);
            if (err) return(err);
        }
        
    }
    return (0);
}

int
deriveBandCountAndGainSetIndexForChannelGroups( DrcInstructionsUniDrc* drcInstructionsUniDrc,
                                                int version,
                                                DrcCoefficientsUniDrc *drcCoefficientsUniDrc,
                                                int drcCoefficientsUniDrcCount)
{
    int i, k, n;
    int uniqueIndex[CHANNEL_COUNT_MAX];
    int match;
    
    if(version == 0) {
        for( i = 0; i < drcInstructionsUniDrc->nDrcChannelGroups; i++) {
            drcInstructionsUniDrc->bandCountForChannelGroup[i] = 1;
        }
    }
    else {
        for (i=0; i<CHANNEL_COUNT_MAX; i++)
        {
            uniqueIndex[i] = -1;
        }
        k = 0;
        for (i=0; i<drcInstructionsUniDrc->drcChannelCount; i++)
        {
            int tmp = drcInstructionsUniDrc->gainSetIndex[i];
            if (tmp>=0)
            {
                match = FALSE;
                for (n=0; n<k; n++)
                {
                    if (uniqueIndex[n] == tmp) match = TRUE;
                }
                if (match == FALSE)
                {
                    uniqueIndex[k] = tmp;
                    k++;
                }
            }
        }
        
        /* sanity check for nDrcChannelGroups */
        if(drcInstructionsUniDrc->nDrcChannelGroups != k) {
            return -5;
        }
        
        for (i=0; i<drcInstructionsUniDrc->nDrcChannelGroups; i++)
        {
            /* the DRC channel groups are ordered according to increasing channel indices */
            /* Whenever a new gainSetIndex appears the channel group index is incremented */
            /* The same order has to be observed for all subsequent processing */
            
            drcInstructionsUniDrc->gainSetIndexForChannelGroup[i] = uniqueIndex[i];
            /* retrieve band count */
            if (drcCoefficientsUniDrcCount > 0)
            {
                if (drcCoefficientsUniDrc[0].gainSetCount <= drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]) {
                    drcInstructionsUniDrc->bandCountForChannelGroup[i] = 1;
                }
                else {
                    drcInstructionsUniDrc->bandCountForChannelGroup[i] = drcCoefficientsUniDrc[0].gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[i]].bandCount;
                }
            }
            else {
                drcInstructionsUniDrc->bandCountForChannelGroup[i] = 1; /* only parametric DRCs in payload */
            }
        }
    }
    
    return (0);
}


int
writeIsobmffDrcInstructionsUniDrc(DrcInstructionsUniDrc* drcInstructionsUniDrc,
                                  int version,
                                  int uniDrcInstructionsCount,
                                  DownmixInstructions* downmixInstructions,
                                  int downmixInstructionsCount,
                                  int baseChannelCount,
                                  DrcCoefficientsUniDrc *drcCoefficientsUniDrc,
                                  int drcCoefficientsUniDrcCount,
                                  wobitbufHandle bitstream)
{
    int err, reserved = 0, k, m, c;
    float delta;
    int mu, sigma;
    int group;
    
    if (version >= 1) {
        err = putBits(bitstream, 2, reserved);
        if (err) return(err);
        err = putBits(bitstream, 6, uniDrcInstructionsCount);
        if (err) return(err);
    }
    
    for (k = 0; k < uniDrcInstructionsCount; k++) {
        if (version == 0) {
            err = putBits(bitstream, 3, reserved);
            if (err) return(err);
            err = putBits(bitstream, 6, drcInstructionsUniDrc[k].drcSetId);
            if (err) return(err);
            err = putBits(bitstream, 5, drcInstructionsUniDrc[k].drcLocation);
            if (err) return(err);
            err = putBits(bitstream, 7, drcInstructionsUniDrc[k].downmixId);
            if (err) return(err);
            err = putBits(bitstream, 3, drcInstructionsUniDrc[k].additionalDownmixIdCount);
            if (err) return(err);
            for( m = 0; m < drcInstructionsUniDrc[k].additionalDownmixIdCount; m++) {
                err = putBits(bitstream, 1, reserved);
                if (err) return(err);
                err = putBits(bitstream, 7, drcInstructionsUniDrc[k].additionalDownmixId[m]);
                if (err) return(err);
            }
        }
        else {
            err = putBits(bitstream, 6, drcInstructionsUniDrc[k].drcSetId);
            if (err) return(err);
            err = putBits(bitstream, 4, drcInstructionsUniDrc[k].drcSetComplexityLevel);
            if (err) return(err);
            err = putBits(bitstream, 5, drcInstructionsUniDrc[k].drcLocation);
            if (err) return(err);
            err = putBits(bitstream, 1, drcInstructionsUniDrc[k].downmixIdPresent);
            if (err) return(err);
            if(drcInstructionsUniDrc[k].downmixIdPresent == 1) {
                err = putBits(bitstream, 5, reserved);
                if (err) return(err);
                err = putBits(bitstream, 7, drcInstructionsUniDrc[k].downmixId);
                if (err) return(err);
                err = putBits(bitstream, 1, drcInstructionsUniDrc[k].drcApplyToDownmix);
                if (err) return(err);
                err = putBits(bitstream, 3, drcInstructionsUniDrc[k].additionalDownmixIdCount);
                if (err) return(err);
                for( m = 0; m < drcInstructionsUniDrc[k].additionalDownmixIdCount; m++) {
                    err = putBits(bitstream, 1, reserved);
                    if (err) return(err);
                    err = putBits(bitstream, 7, drcInstructionsUniDrc[k].additionalDownmixId[m]);
                    if (err) return(err);
                }
            }
        }
        
        err = putBits(bitstream, 16, drcInstructionsUniDrc[k].drcSetEffect);
        if (err) return(err);
        
        if((drcInstructionsUniDrc[k].drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) == 0) {
            err = putBits(bitstream, 7, reserved);
            if (err) return(err);
            err = putBits(bitstream, 1, drcInstructionsUniDrc[k].limiterPeakTargetPresent);
            if (err) return(err);
            if(drcInstructionsUniDrc[k].limiterPeakTargetPresent == 1) {
                err = putBits(bitstream, 8, (int)(0.5f -8*drcInstructionsUniDrc[k].limiterPeakTarget));
                if (err) return(err);
            }
        }
        
        err = putBits(bitstream, 7, reserved);
        if (err) return(err);
        err = putBits(bitstream, 1, drcInstructionsUniDrc[k].drcSetTargetLoudnessPresent);
        if (err) return(err);
        
        if(drcInstructionsUniDrc[k].drcSetTargetLoudnessPresent == 1) {
            err = putBits(bitstream, 4, reserved);
            if (err) return(err);
            err = putBits(bitstream, 6, drcInstructionsUniDrc[k].drcSetTargetLoudnessValueUpper + 63);
            if (err) return(err);
            if(drcInstructionsUniDrc[k].drcSetTargetLoudnessValueLowerPresent == 0) {
                return -1;
            }
            err = putBits(bitstream, 6, drcInstructionsUniDrc[k].drcSetTargetLoudnessValueLower + 63);
            if (err) return(err);
        }
        
        if (version == 0) {
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
        }
        else {
            err = putBits(bitstream, 1, drcInstructionsUniDrc[k].requiresEq);
            if (err) return(err);
        }
        
        err = putBits(bitstream, 6, drcInstructionsUniDrc[k].dependsOnDrcSet);
        if (err) return(err);
        
        if(drcInstructionsUniDrc[k].dependsOnDrcSet == 0) {
            err = putBits(bitstream, 1, drcInstructionsUniDrc[k].noIndependentUse);
            if (err) return(err);
        }
        else {
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
        }
        
        if(drcInstructionsUniDrc[k].downmixId != 0 && drcInstructionsUniDrc[k].downmixId != 0x7F && (drcInstructionsUniDrc[k].additionalDownmixIdPresent == 0 || (drcInstructionsUniDrc[k].additionalDownmixIdPresent == 1 && drcInstructionsUniDrc[k].additionalDownmixIdCount == 0)) && ((version == 1 && drcInstructionsUniDrc[k].drcApplyToDownmix == 1) || version == 0)) {
            for(m=0; m< downmixInstructionsCount; m++)
            {
                if (drcInstructionsUniDrc->downmixId == downmixInstructions[m].downmixId) break;
            }
            if (m == downmixInstructionsCount)
            {
                fprintf(stderr, "ERROR: downmixInstructions expected but not found: %d\n", downmixInstructionsCount);
                return(UNEXPECTED_ERROR);
            }
            drcInstructionsUniDrc[k].drcChannelCount = downmixInstructions[m].targetChannelCount;  /*  targetChannelCountFromDownmixId*/
        }
        else if(drcInstructionsUniDrc[k].downmixId == 0x7F || (drcInstructionsUniDrc[k].additionalDownmixIdPresent != 0 && drcInstructionsUniDrc[k].additionalDownmixIdCount != 0) && ((version == 1 && drcInstructionsUniDrc[k].drcApplyToDownmix == 1) || version == 0)) {
            drcInstructionsUniDrc[k].drcChannelCount = 1;
        }
        else {
            drcInstructionsUniDrc[k].drcChannelCount = baseChannelCount;
        }
        
        err = putBits(bitstream, 8, drcInstructionsUniDrc[k].drcChannelCount);
        if (err) return(err);
        
        group = 0;
        for (c=0; c<drcInstructionsUniDrc[k].drcChannelCount; c++) {
            int gainSetIndex = drcInstructionsUniDrc[k].gainSetIndex[c];
            int skipSet = FALSE;
            if (gainSetIndex >= 0) {
                for (m=c-1; m>=0; m--) {
                    if (drcInstructionsUniDrc[k].gainSetIndex[m] == gainSetIndex) {
                        drcInstructionsUniDrc[k].channelGroupForChannel[c] = drcInstructionsUniDrc[k].channelGroupForChannel[m];
                        skipSet = TRUE;
                    }
                }
                if (skipSet == FALSE) {
                    drcInstructionsUniDrc[k].channelGroupForChannel[c] = group;
                    group++;
                }
            }
            else {
                drcInstructionsUniDrc[k].channelGroupForChannel[c] = -1;
            }
        }
        
        for( m = 0; m < drcInstructionsUniDrc[k].drcChannelCount; m++) {
            err = putBits(bitstream, 8, drcInstructionsUniDrc[k].channelGroupForChannel[m]+1);
            if (err) return(err);
        }
        
        deriveBandCountAndGainSetIndexForChannelGroups(&drcInstructionsUniDrc[k], version, drcCoefficientsUniDrc, drcCoefficientsUniDrcCount);
        
        for( m = 0; m < drcInstructionsUniDrc[k].nDrcChannelGroups; m++) {
            err = putBits(bitstream, 2, reserved);
            if (err) return(err);
            err = putBits(bitstream, 6, drcInstructionsUniDrc[k].gainSetIndexForChannelGroup[m]+1);
            if (err) return(err);
        }
        
        if(drcInstructionsUniDrc[k].drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) {
            if(drcInstructionsUniDrc[k].drcSetEffect & (EFFECT_BIT_DUCK_OTHER )) {
                for( m = 0; m < drcInstructionsUniDrc[k].nDrcChannelGroups; m++) {
                    err = putBits(bitstream, 7, reserved);
                    if (err) return(err);
                    
                    /* deactivate ducking scaling if gain is neutral */
                    if(drcInstructionsUniDrc[k].duckingModifiersForChannelGroup[m].duckingScalingPresent == 1) {
                        /* quantize ducking scaling */
                        delta = drcInstructionsUniDrc[k].duckingModifiersForChannelGroup[m].duckingScaling - 1.0f;
                        
                        if (delta > 0.0f)
                        {
                            mu = - 1 + (int) (0.5f + 8.0f * delta);
                            sigma = 0;
                        }
                        else
                        {
                            mu = - 1 + (int) (0.5f - 8.0f * delta);
                            sigma = 1;
                        }
                        if(mu == -1) {
                            drcInstructionsUniDrc[k].duckingModifiersForChannelGroup[m].duckingScalingPresent = 0;
                        }
                    }
                    
                    err = putBits(bitstream, 1, drcInstructionsUniDrc[k].duckingModifiersForChannelGroup[m].duckingScalingPresent);
                    if (err) return(err);
                    if(drcInstructionsUniDrc[k].duckingModifiersForChannelGroup[m].duckingScalingPresent == 1) {
                        err = putBits(bitstream, 4, reserved);
                        if (err) return(err);
                        err = putBits(bitstream, 1, sigma);
                        if (err) return(err);
                        err = putBits(bitstream, 3, mu);
                        if (err) return(err);
                    }
                }
            }
        }
        else {
            
            for( m = 0; m < drcInstructionsUniDrc[k].nDrcChannelGroups; m++) {
                if(version >= 1) {
                    err = putBits(bitstream, 4, reserved);
                    if (err) return(err);
                    err = putBits(bitstream, 4, drcInstructionsUniDrc[k].bandCountForChannelGroup[m]);
                    if (err) return(err);
                }
                
                writeIsobmffGainModifiers(bitstream, version, drcInstructionsUniDrc[k].bandCountForChannelGroup[m], &drcInstructionsUniDrc[k].gainModifiers[m]);
            }
        }
    }
    
    return (0);
}

/* Writer for ISO base media file format 'udi2' box */
int
writeIsobmffUdi2(HANDLE_DRC_ENCODER hDrcEnc,
                 wobitbufHandle bitstream,
                 int size,
                 int* bitCount)
{
    int err;
    int version = 1; /* fixed to newest version */
    int flags = 0;
    
    char type[] = "udi2";
    
    int downmixInstructionsCount;
    DownmixInstructions *pDownmixInstructions = NULL;

    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    /* pass correct downmixInstructions */
    if(hDrcEnc->uniDrcConfig.uniDrcConfigExtPresent == 1 && hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1Present) {
        pDownmixInstructions = hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1;
        downmixInstructionsCount = hDrcEnc->uniDrcConfig.uniDrcConfigExt.downmixInstructionsV1Count;
    }
    else {
        pDownmixInstructions = hDrcEnc->uniDrcConfig.downmixInstructions;
        downmixInstructionsCount = hDrcEnc->uniDrcConfig.downmixInstructionsCount;
    }
    
    
    /* write drcInstructionsUniDrc */
    if(hDrcEnc->uniDrcConfig.uniDrcConfigExtPresent == 1 && hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
        writeIsobmffDrcInstructionsUniDrc(hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcInstructionsUniDrcV1,
                                          version,
                                          hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcInstructionsUniDrcV1Count,
                                          pDownmixInstructions,
                                          downmixInstructionsCount,
                                          hDrcEnc->baseChannelCount,
                                          hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1,
                                          hDrcEnc->uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count,
                                          bitstream);
    }
    else {
        writeIsobmffDrcInstructionsUniDrc(hDrcEnc->uniDrcConfig.drcInstructionsUniDrc,
                                          version,
                                          hDrcEnc->uniDrcConfig.drcInstructionsUniDrcCount,
                                          pDownmixInstructions,
                                          downmixInstructionsCount,
                                          hDrcEnc->baseChannelCount,
                                          hDrcEnc->uniDrcConfig.drcCoefficientsUniDrc,
                                          hDrcEnc->uniDrcConfig.drcCoefficientsUniDrcCount,
                                          bitstream);
    }
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}


int
writeIsobmffDrcCoefficientsParametricDrc( UniDrcConfig* uniDrcConfig,
                                          int size,
                                          int* bitCount,
                                          wobitbufHandle bitstream)
{
    int err, k, i;
    int flags = 0;
    int reserved = 0;
    int version = 0;
    int code, codeSize;
    float exp;
    
    char type[] = "pdc1";

    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    
    /* drcCoefficientsParametricDrc */
    err = putBits(bitstream, 1, reserved);
    if (err) return(err);
    
    err = putBits(bitstream, 5, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.drcLocation);
    if (err) return(err);
    
    
    exp = log(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSize)/log(2);
    if (exp != (int)exp) {
        uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSizeFormat = 1;
    } else {
        uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSizeFormat = 0;
    }
    
    err = putBits(bitstream, 1, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSizeFormat);
    if (err) return(err);
    err = putBits(bitstream, 1, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.resetParametricDrc);
    if (err) return(err);
    
    if(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSizeFormat == 0) {
        err = putBits(bitstream, 4, reserved);
        if (err) return(err);
        err = putBits(bitstream, 4, (int)exp);
        if (err) return(err);
    }
    else {
        err = putBits(bitstream, 1, reserved);
        if (err) return(err);
        err = putBits(bitstream, 15, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSize-1);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 1, reserved);
    if (err) return(err);
    err = putBits(bitstream, 1, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMaxPresent);
    if (err) return(err);
    if(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMaxPresent == 1) {
        int nu, mu;
        
        for (nu=0; nu<8; nu++) {
            mu = uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMax / (16 << nu);
            if (mu * (16 << nu) < uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMax) {
                mu++;
            }
            if (mu < 32) {
                break;
            }
        }
        if (nu == 8) {
            nu = 7;
            mu = 31;
        }
        /*printf("parametricDrcDelayMax (encoder) = %d, parametricDrcDelayMax (decoder) = %d\n",drcCoefficientsParametricDrc->parametricDrcDelayMax, (int)(16*mu*pow(2.f, nu)));*/
        err = putBits(bitstream, 5, mu);
        if (err) return(err);
        err = putBits(bitstream, 3, nu);
        if (err) return(err);
    }
    
    err = putBits(bitstream, 6, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetCount);
    if (err) return(err);
    
    for(k = 0; k < uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetCount; k++) {
        err = putBits(bitstream, 4, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].parametricDrcId);
        if (err) return(err);
        err = putBits(bitstream, 1, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].drcInputLoudnessPresent);
        if (err) return(err);
        err = putBits(bitstream, 3, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].sideChainConfigType);
        if (err) return(err);
        if(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].drcInputLoudnessPresent) {
            code = ((int) (0.5f + 4.0f * (uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].drcInputLoudness + 57.75f) + 10000.0f)) - 10000;
            code = min(0x0FF, code);
            code = max(0x0, code);
            
            err = putBits(bitstream, 8, code);
            if (err) return(err);
        }
        if(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].sideChainConfigType == 1) {
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
            err = putBits(bitstream, 7, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].downmixId);
            if (err) return(err);
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
            
            /* derive channel count from downmixId */
            if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].downmixId == 0x0) {
                uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].channelCountFromDownmixId = uniDrcConfig->channelLayout.baseChannelCount;
            } else if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].downmixId == 0x7F) {
                uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].channelCountFromDownmixId = 1;
            } else {
                for(i=0; i<uniDrcConfig->downmixInstructionsCount; i++)
                {
                    if (uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].downmixId == uniDrcConfig->downmixInstructions[i].downmixId) break;
                }
                if (i == uniDrcConfig->downmixInstructionsCount)
                {
                    fprintf(stderr,"ERROR: downmix ID not found\n");
                    return(UNEXPECTED_ERROR);
                }
                uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].channelCountFromDownmixId = uniDrcConfig->downmixInstructions[i].targetChannelCount;  /*  channelCountFromDownmixId*/
            }
            
            err = putBits(bitstream, 8, uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].channelCountFromDownmixId);
            if (err) return(err);
            
            for(i = 0; i < uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].channelCountFromDownmixId; i++) {
                err = encChannelWeight(uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcGainSetParams[k].levelEstimChannelWeight[i], &codeSize, &code);
                if (err) return(err);
                err = putBits(bitstream, codeSize, code);
                if (err) return(err);
            }
        }
    }
    
    
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}


int
writeIsobmffParametricDrcInstructions(UniDrcConfig* uniDrcConfig,
                                      int size,
                                      int* bitCount,
                                      wobitbufHandle bitstream)
{
    int err, k, i;
    int flags = 0;
    int reserved = 0;
    int version = 0;
    int code;
    ParametricDrcInstructions *pParametricDrcInstruction = uniDrcConfig->uniDrcConfigExt.parametricDrcInstructions;
    
    char type[] = "pdi1";

    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* version */
    err = putBits(bitstream, 8, version);
    if (err) return(err);
    
    /* flags */
    err = putBits(bitstream, 24, flags);
    if (err) return(err);
    
    
    /* parametricDrcInstructions */
    err = putBits(bitstream, 4, reserved);
    if (err) return(err);
    err = putBits(bitstream, 4, uniDrcConfig->uniDrcConfigExt.parametricDrcInstructionsCount);
    if (err) return(err);
    for(k = 0; k < uniDrcConfig->uniDrcConfigExt.parametricDrcInstructionsCount; k++) {
        err = putBits(bitstream, 2, reserved);
        if (err) return(err);
        err = putBits(bitstream, 4, pParametricDrcInstruction[k].parametricDrcId);
        if (err) return(err);
        err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcLookAheadPresent);
        if (err) return(err);
        err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcPresetIdPresent);
        if (err) return(err);
        if(pParametricDrcInstruction[k].parametricDrcLookAheadPresent == 1) {
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
            err = putBits(bitstream, 7, pParametricDrcInstruction[k].parametricDrcLookAhead);
            if (err) return(err);
        }
        if(pParametricDrcInstruction[k].parametricDrcPresetIdPresent == 1) {
            err = putBits(bitstream, 1, reserved);
            if (err) return(err);
            err = putBits(bitstream, 7, pParametricDrcInstruction[k].parametricDrcPresetId);
            if (err) return(err);
        }
        else {
            err = putBits(bitstream, 5, reserved);
            if (err) return(err);
            err = putBits(bitstream, 3, pParametricDrcInstruction[k].parametricDrcType);
            if (err) return(err);
            if(pParametricDrcInstruction[k].parametricDrcType == PARAM_DRC_TYPE_FF) {
                err = putBits(bitstream, 3, reserved);
                if (err) return(err);
                err = putBits(bitstream, 2, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.levelEstimKWeightingType);
                if (err) return(err);
                err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.levelEstimIntegrationTimePresent);
                if (err) return(err);
                err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.drcCurveDefinitionType);
                if (err) return(err);
                err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.drcGainSmoothParametersPresent);
                if (err) return(err);
                if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.levelEstimIntegrationTimePresent == 1) {
                    err = putBits(bitstream, 2, reserved);
                    if (err) return(err);
                    code = ((float)pParametricDrcInstruction[k].parametricDrcTypeFeedForward.levelEstimIntegrationTime / uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcFrameSize + 0.5f) - 1;
                    err = putBits(bitstream, 6, code);
                    if (err) return(err);
                }
                if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.drcCurveDefinitionType == 0) {
                    err = putBits(bitstream, 1, reserved);
                    if (err) return(err);
                    err = putBits(bitstream, 7, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.drcCharacteristic);
                    if (err) return(err);
                }
                else {
                    err = putBits(bitstream, 5, reserved);
                    if (err) return(err);
                    err = putBits(bitstream, 3, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeCount - 2);
                    if (err) return(err);
                    
                    for (i = 0; i < pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeCount; i++) {
                        if (i == 0) {
                            err = putBits(bitstream, 2, reserved);
                            if (err) return(err);
                            err = putBits(bitstream, 6, -pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeLevel[0]-11);
                            if (err) return(err);
                        }
                        else {
                            err = putBits(bitstream, 3, reserved);
                            if (err) return(err);
                            err = putBits(bitstream, 5, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeLevel[i] - pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeLevel[i-1] - 1);
                            if (err) return(err);
                        }
                        err = putBits(bitstream, 2, reserved);
                        if (err) return(err);
                        err = putBits(bitstream, 6, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.nodeGain[i]+39);
                        if (err) return(err);
                    }
                }
                
                if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.drcGainSmoothParametersPresent == 1) {
                    err = putBits(bitstream, 6, reserved);
                    if (err) return(err);
                    err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothTimeFastPresent);
                    if (err) return(err);
                    err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothHoldOffCountPresent);
                    if (err) return(err);
                    code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothAttackTimeSlow*0.2;
                    err = putBits(bitstream, 8, code);
                    if (err) return(err);
                    code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothReleaseTimeSlow*0.025,
                    err = putBits(bitstream, 8, code);
                    if (err) return(err);
                    
                    if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothTimeFastPresent == 1) {
                        code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothAttackTimeFast*0.2;
                        err = putBits(bitstream, 8, code);
                        if (err) return(err);
                        code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothReleaseTimeFast*0.05;
                        err = putBits(bitstream, 8, code);
                        if (err) return(err);
                        err = putBits(bitstream, 7, reserved);
                        if (err) return(err);
                        err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothThresholdPresent);
                        if (err) return(err);
                        if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothThresholdPresent == 1) {
                            err = putBits(bitstream, 3, reserved);
                            if (err) return(err);
                            
                            if (pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothAttackThreshold > 30) {
                                code = 31;
                            } else {
                                code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothAttackThreshold;
                            }
                            err = putBits(bitstream, 5, code);
                            if (err) return(err);
                            err = putBits(bitstream, 3, reserved);
                            if (err) return(err);

                            if (pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothReleaseThreshold > 30) {
                                code = 31;
                            } else {
                                code = pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothReleaseThreshold;
                            }
                            err = putBits(bitstream, 5, code);
                            if (err) return(err);
                        }
                    }
                    
                    if(pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothHoldOffCountPresent == 1) {
                        err = putBits(bitstream, 1, reserved);
                        if (err) return(err);
                        err = putBits(bitstream, 7, pParametricDrcInstruction[k].parametricDrcTypeFeedForward.gainSmoothHoldOff);
                        if (err) return(err);
                    }
                }
            }
            else if(pParametricDrcInstruction[k].parametricDrcType == PARAM_DRC_TYPE_LIM) {
                err = putBits(bitstream, 6, reserved);
                if (err) return(err);
                err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimThresholdPresent);
                if (err) return(err);
                err = putBits(bitstream, 1, pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimReleasePresent);
                if (err) return(err);
                if(pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimThresholdPresent == 1) {
                    code = (int)(0.5f - 8.0f * pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimThreshold);
                    code = max (0, code);
                    code = min (0xFF, code);
                    err = putBits(bitstream, 8, code);
                    if (err) return(err);
                }
                if(pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimReleasePresent) {
                    code = pParametricDrcInstruction[k].parametricDrcTypeLim.parametricLimRelease*0.1;
                    err = putBits(bitstream, 8, code);
                    if (err) return(err);
                }
            }
        }
    }
    
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}


/* Writer for ISO base media file format 'udex' box */
int
writeIsobmffUdex(HANDLE_DRC_ENCODER hDrcEnc,
                 wobitbufHandle bitstream,
                 int size,
                 int* bitCount)
{
    int err;
    int flags = 0;
    int bitCountTmp, bitCountTmp2, sizePdc1, sizePdi1;
    
    char type[] = "udex";
    
    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    wobitbuf bsTmp;
    wobitbufHandle bitstreamTmp = &bsTmp;

    wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);


    /* box size */
    if (size == 0 || size == 1) {
        /* currently unsupported */
        return -1;
    }
    else if (size == -1) {
        /* write simulation */
        size = 0;
    }
    err = putBits(bitstream, 32, size);
    if (err) return(err);
    
    /* box type */
    err = putBits(bitstream, 8, type[0]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[1]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[2]);
    if (err) return(err);
    err = putBits(bitstream, 8, type[3]);
    if (err) return(err);
    
    /* write drcCoefficientsParametricDrc 'pdc1' for getting size */
    bitCountTmp = 0;
    writeIsobmffDrcCoefficientsParametricDrc(&hDrcEnc->uniDrcConfig,
                                              -1,
                                              &bitCountTmp,
                                              bitstreamTmp);

    sizePdc1 = (bitCountTmp / 8);
    bitCountTmp = wobitbuf_GetBitsWritten(bitstream);
    bitCountTmp2 = 0;
    
    /* write drcCoefficientsParametricDrc 'pdc1' */
    writeIsobmffDrcCoefficientsParametricDrc(&hDrcEnc->uniDrcConfig,
                                              sizePdc1,
                                              &bitCountTmp2,
                                              bitstream);
    
    if (sizePdc1 != (bitCountTmp2-bitCountTmp)/8) return (UNEXPECTED_ERROR);
    
    wobitbuf_Reset(bitstreamTmp);
    
    /* write parametricDrcInstructions 'pdi1' for getting size */
    bitCountTmp = 0;
    writeIsobmffParametricDrcInstructions(&hDrcEnc->uniDrcConfig,
                                          -1,
                                          &bitCountTmp,
                                          bitstreamTmp);

    sizePdi1 = (bitCountTmp / 8);
    bitCountTmp = wobitbuf_GetBitsWritten(bitstream);
    bitCountTmp2 = 0;
    
    /* write parametricDrcInstructions 'pdi1' */
    writeIsobmffParametricDrcInstructions(&hDrcEnc->uniDrcConfig,
                                          sizePdi1,
                                          &bitCountTmp2,
                                          bitstream);
    
    if (sizePdi1 != (bitCountTmp2 - bitCountTmp)/8) return (UNEXPECTED_ERROR);
    
    
    *bitCount += wobitbuf_GetBitsWritten(bitstream);
    
    return (0);
}

/* Writer for ISO base media file format DRC config */
int
writeIsobmffDrc(HANDLE_DRC_ENCODER hDrcEnc,
                unsigned char* bitstreamBuffer,
                int* bitCount)
{
    int err, bitCountTmp = 0, sizeBit = 0;
    int sizeChnl, sizeDmix, sizeUdc2, sizeUdi2, sizeUdex;
    int sizeSum = 0;
    
    wobitbuf bs;
    wobitbufHandle bitstream = &bs;
    
    unsigned char bitstreamBufferTmp[MAX_DRC_PAYLOAD_BITS];
    wobitbuf bsTmp;
    wobitbufHandle bitstreamTmp = &bsTmp;
    
    wobitbuf_Init(bitstream, bitstreamBuffer, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);

    wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_DRC_PAYLOAD_BITS-*bitCount, *bitCount);
    
    /*** CHNL BOX */
    /* write "chnl" for getting size */
    sizeBit = -1;
    err = writeIsobmffChnl(hDrcEnc, bitstreamTmp, sizeBit, &bitCountTmp);
    if (err) return(err);

    /* write "chnl" */
    bitCountTmp -= sizeSum*8;
    sizeChnl = bitCountTmp / 8;
    err = writeIsobmffChnl(hDrcEnc, bitstream, sizeChnl, bitCount);
    if (err) return(err);
    
    bitCount -= sizeSum*8;
    sizeSum = sizeChnl;
    
    if (sizeSum != (*bitCount)/8) return (UNEXPECTED_ERROR);
    
    /*** DMIX BOX */
    /* write "dmix" for getting size */
    sizeBit = -1;
    err = writeIsobmffDmix(hDrcEnc, bitstreamTmp, sizeBit, &bitCountTmp);
    if (err) return(err);

    /* write "dmix" */
    bitCountTmp -= sizeSum*8;
    sizeDmix = (bitCountTmp / 8) - sizeSum;
    err = writeIsobmffDmix(hDrcEnc, bitstream, sizeDmix, bitCount);
    if (err) return(err);
    
    *bitCount -= sizeSum*8;
    sizeSum += sizeDmix;
    
    if (sizeSum != (*bitCount)/8) return (UNEXPECTED_ERROR);
    
    
    /*** UDC2 BOX */
    /* write "udc2" for getting size */
    sizeBit = -1;
    err = writeIsobmffUdc2(hDrcEnc, bitstreamTmp, sizeBit, &bitCountTmp);
    if (err) return(err);

    /* write "udc2" */
    bitCountTmp -= sizeSum*8;
    sizeUdc2 = (bitCountTmp / 8) - sizeSum;
    err = writeIsobmffUdc2(hDrcEnc, bitstream, sizeUdc2, bitCount);
    if (err) return(err);
    
    *bitCount -= sizeSum*8;
    sizeSum += sizeUdc2;
    
    if (sizeSum != (*bitCount)/8) return (UNEXPECTED_ERROR);
    
    
    /*** UDI2 BOX */
    /* write "udi2" for getting size */
    sizeBit = -1;
    err = writeIsobmffUdi2(hDrcEnc, bitstreamTmp, sizeBit, &bitCountTmp);
    if (err) return(err);

    /* write "udi2" */
    bitCountTmp -= sizeSum*8;
    sizeUdi2 = (bitCountTmp / 8) - sizeSum;
    err = writeIsobmffUdi2(hDrcEnc, bitstream, sizeUdi2, bitCount);
    if (err) return(err);
    
    *bitCount -= sizeSum*8;
    sizeSum += sizeUdi2;
    
    if (sizeSum != (*bitCount)/8) return (UNEXPECTED_ERROR);
    
    
    /*** UDEX BOX */
    /* write only, if data is available */
    if(hDrcEnc->uniDrcConfig.uniDrcConfigExtPresent == 1 && hDrcEnc->uniDrcConfig.uniDrcConfigExt.parametricDrcPresent == 1) {
        /* write "udex" for getting size */
        sizeBit = -1;
        err = writeIsobmffUdex(hDrcEnc, bitstreamTmp, sizeBit, &bitCountTmp);
        if (err) return(err);

        /* write "udex" */
        bitCountTmp -= sizeSum*8;
        sizeUdex = (bitCountTmp / 8) - sizeSum;
        err = writeIsobmffUdex(hDrcEnc, bitstream, sizeUdex, bitCount);
        if (err) return(err);
        
        *bitCount -= sizeSum*8;
        sizeSum += sizeUdex;

        if (sizeSum != (*bitCount)/8) return (UNEXPECTED_ERROR);
    }
    
    return (0);
}
#endif
#endif
#endif








