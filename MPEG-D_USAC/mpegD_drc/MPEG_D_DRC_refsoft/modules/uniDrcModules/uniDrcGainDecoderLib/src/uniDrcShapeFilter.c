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
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "uniDrcGainDecoder.h"

#if MPEG_D_DRC_EXTENSION_V1

extern const float shapeFilterY1BoundLfTable[][3];
extern const float shapeFilterY1BoundHfTable[][3];
extern const float shapeFilterGainOffsetLfTable[][3];
extern const float shapeFilterGainOffsetHfTable[][3];
extern const float shapeFilterRadiusLfTable[];
extern const float shapeFilterRadiusHfTable[];
extern const float shapeFilterCutoffFreqNormHfTable[];

int
adaptShapeFilter(const float drcGain,
                 ShapeFilter* shapeFilter)
{
    float warpedGain, x1, y1;
    switch (shapeFilter->type) {
        case SHAPE_FILTER_TYPE_OFF:
            return(0);
        case SHAPE_FILTER_TYPE_LF_CUT:
        case SHAPE_FILTER_TYPE_HF_CUT:
            if (drcGain < 1.0f) warpedGain = -1.0f;
            else warpedGain = (drcGain - 1.0f) / (drcGain - 1.0f + shapeFilter->gainOffset);
            x1 = shapeFilter->a1;
            break;
        case SHAPE_FILTER_TYPE_LF_BOOST:
        case SHAPE_FILTER_TYPE_HF_BOOST:
            if (drcGain >= 1.0f) warpedGain = -1.0f;
            else warpedGain = (1.0f - drcGain) / (1.0f + drcGain * (shapeFilter->gainOffset - 1.0f));
            x1 = shapeFilter->b1;
            break;
        default:
            return(0);
            break;
    }
    if (warpedGain <= 0.0f)
    {
        y1 = x1;
    }
    else if (warpedGain < shapeFilter->warpedGainMax) {
        y1 = x1 + shapeFilter->factor * warpedGain;
    }
    else
    {
        y1 = shapeFilter->y1Bound;
    }
    switch (shapeFilter->type) {
        case SHAPE_FILTER_TYPE_HF_CUT:
            shapeFilter->gNorm = shapeFilter->coeffSum / (shapeFilter->partialCoeffSum + y1);
        case SHAPE_FILTER_TYPE_LF_CUT:
            shapeFilter->b1 = y1;
            break;
        case SHAPE_FILTER_TYPE_HF_BOOST:
            shapeFilter->gNorm = (shapeFilter->partialCoeffSum + y1) / shapeFilter->coeffSum;
        case SHAPE_FILTER_TYPE_LF_BOOST:
            shapeFilter->a1 = y1;
            break;
        default:
            break;
    }
    return (0);
}

int
adaptShapeFilterBlock(const float drcGain,
                      ShapeFilterBlock* shapeFilterBlock)
{
    int err = 0, i;

    shapeFilterBlock->drcGainLast = drcGain;
    for (i=0; i<4; i++) {
        err = adaptShapeFilter(drcGain, &shapeFilterBlock->shapeFilter[i]);
    }
    return (0);
}

int
resetShapeFilter(ShapeFilter* shapeFilter)
{
    int c;
    for (c=0; c<CHANNEL_COUNT_MAX; c++)
    {
        shapeFilter->audioInState1[c] = 0.0f;
        shapeFilter->audioInState2[c] = 0.0f;
        shapeFilter->audioOutState1[c] = 0.0f;
        shapeFilter->audioOutState2[c] = 0.0f;
    }
    return (0);
}

int
resetShapeFilterBlock(ShapeFilterBlock* shapeFilterBlock)
{
    int i;
    shapeFilterBlock->drcGainLast = -1.0f;
    adaptShapeFilterBlock(1.0f, shapeFilterBlock);
    for (i=0; i<4; i++) {
        resetShapeFilter(&shapeFilterBlock->shapeFilter[i]);
    }
    return (0);
}

int
initShapeFilterCoeff(ShapeFilterParams* params,
                     const int shapeFilterType,
                     ShapeFilter* shapeFilter)
{
    float x1;
    float x2 = 0.0f;
    float radius;
    
    shapeFilter->type = shapeFilterType;
    switch (shapeFilterType) {
        case SHAPE_FILTER_TYPE_LF_CUT:
        case SHAPE_FILTER_TYPE_LF_BOOST:
            shapeFilter->gainOffset = shapeFilterGainOffsetLfTable[params->cornerFreqIndex][params->filterStrengthIndex];
            shapeFilter->y1Bound = shapeFilterY1BoundLfTable[params->cornerFreqIndex][params->filterStrengthIndex];
            x1 = - shapeFilterRadiusLfTable[params->cornerFreqIndex];
            break;
        case SHAPE_FILTER_TYPE_HF_CUT:
        case SHAPE_FILTER_TYPE_HF_BOOST:
            shapeFilter->gainOffset = shapeFilterGainOffsetHfTable[params->cornerFreqIndex][params->filterStrengthIndex];
            shapeFilter->y1Bound = shapeFilterY1BoundHfTable[params->cornerFreqIndex][params->filterStrengthIndex];
            radius = shapeFilterRadiusHfTable[params->cornerFreqIndex];
            x1 = - 2.0f * radius * cos(2.0f * M_PI * shapeFilterCutoffFreqNormHfTable[params->cornerFreqIndex]);
            x2 = radius * radius;
            break;
            
        default:
            break;
    }
    shapeFilter->warpedGainMax = SHAPE_FILTER_DRC_GAIN_MAX_MINUS_ONE / (SHAPE_FILTER_DRC_GAIN_MAX_MINUS_ONE + shapeFilter->gainOffset);
    shapeFilter->factor = (shapeFilter->y1Bound - x1) / shapeFilter->warpedGainMax;
    
    switch (shapeFilterType) {
        case SHAPE_FILTER_TYPE_HF_CUT:
            shapeFilter->coeffSum = 1.0f + x1 + x2;
            shapeFilter->partialCoeffSum = 1.0f + x2;
            shapeFilter->a1 = x1;
            shapeFilter->a2 = x2;
            shapeFilter->b2 = x2;
            break;
        case SHAPE_FILTER_TYPE_LF_CUT:
            shapeFilter->a1 = x1;
            break;
        case SHAPE_FILTER_TYPE_HF_BOOST:
            shapeFilter->coeffSum = 1.0f + x1 + x2;
            shapeFilter->partialCoeffSum = 1.0f + x2;
            shapeFilter->b1 = x1;
            shapeFilter->b2 = x2;
            shapeFilter->a2 = x2;
            break;
        case SHAPE_FILTER_TYPE_LF_BOOST:
            shapeFilter->b1 = x1;
            break;
            
        default:
            break;
    }
    return (0);
}

int
initShapeFilterBlock(ShapeFilterBlockParams* shapeFilterBlockParams,
                     ShapeFilterBlock* shapeFilterBlock)
{
    int err = 0;
    if (shapeFilterBlockParams->lfCutFilterPresent) {
        err = initShapeFilterCoeff(&shapeFilterBlockParams->lfCutParams,
                                   SHAPE_FILTER_TYPE_LF_CUT,
                                   &(shapeFilterBlock->shapeFilter[0]));
        if (err) return (err);
    }
    else
    {
        shapeFilterBlock->shapeFilter[0].type = SHAPE_FILTER_TYPE_OFF;
    }
    if (shapeFilterBlockParams->lfBoostFilterPresent) {
        err = initShapeFilterCoeff(&shapeFilterBlockParams->lfBoostParams,
                                   SHAPE_FILTER_TYPE_LF_BOOST,
                                   &(shapeFilterBlock->shapeFilter[1]));
        if (err) return (err);
    }
    else
    {
        shapeFilterBlock->shapeFilter[1].type = SHAPE_FILTER_TYPE_OFF;
    }
    if (shapeFilterBlockParams->hfCutFilterPresent) {
        err = initShapeFilterCoeff(&shapeFilterBlockParams->hfCutParams,
                                   SHAPE_FILTER_TYPE_HF_CUT,
                                   &(shapeFilterBlock->shapeFilter[2]));
        if (err) return (err);
    }
    else
    {
        shapeFilterBlock->shapeFilter[2].type = SHAPE_FILTER_TYPE_OFF;
    }
    if (shapeFilterBlockParams->hfBoostFilterPresent) {
        err = initShapeFilterCoeff(&shapeFilterBlockParams->hfBoostParams,
                                   SHAPE_FILTER_TYPE_HF_BOOST,
                                   &(shapeFilterBlock->shapeFilter[3]));
        if (err) return (err);
    }
    else
    {
        shapeFilterBlock->shapeFilter[3].type = SHAPE_FILTER_TYPE_OFF;
    }
    resetShapeFilterBlock(shapeFilterBlock);
    shapeFilterBlock->shapeFilterBlockPresent = 1;
    return (0);
}

int
processShapeFilter(ShapeFilter* shapeFilter,
                   const int channel,
                   const float audioIn,
                   float* audioOut)
{
    switch (shapeFilter->type) {
        case SHAPE_FILTER_TYPE_LF_CUT:
        case SHAPE_FILTER_TYPE_LF_BOOST:
            audioOut[0] = audioIn + shapeFilter->b1 * shapeFilter->audioInState1[channel] - shapeFilter->a1 * shapeFilter->audioOutState1[channel];
            shapeFilter->audioInState1[channel]  = audioIn;
            shapeFilter->audioOutState1[channel] = audioOut[0];
            break;
        case SHAPE_FILTER_TYPE_HF_CUT:
        case SHAPE_FILTER_TYPE_HF_BOOST:
            audioOut[0] = shapeFilter->gNorm * audioIn + shapeFilter->b1 * shapeFilter->audioInState1[channel] + shapeFilter->b2 * shapeFilter->audioInState2[channel] - shapeFilter->a1 * shapeFilter->audioOutState1[channel] - shapeFilter->a2 * shapeFilter->audioOutState2[channel];
            shapeFilter->audioInState2[channel]  = shapeFilter->audioInState1[channel];
            shapeFilter->audioInState1[channel]  = shapeFilter->gNorm * audioIn;
            shapeFilter->audioOutState2[channel] = shapeFilter->audioOutState1[channel];
            shapeFilter->audioOutState1[channel] = audioOut[0];
            break;
        case SHAPE_FILTER_TYPE_OFF:
        default:
            audioOut[0] = audioIn;
            break;
    }
    return (0);
}

int
processShapeFilterBlock(ShapeFilterBlock* shapeFilterBlock,
                        const float drcGain,
                        const int channel,
                        const float audioIn,
                        float* audioOut)
{
    int i;
    if (shapeFilterBlock->shapeFilterBlockPresent) {
        float aIn = audioIn;
        for (i=0; i<4; i++) {
            processShapeFilter(&shapeFilterBlock->shapeFilter[i], channel, aIn, audioOut);
            aIn = *audioOut;
        }
    }
    else
    {
        *audioOut = audioIn;
    }
    return (0);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */
