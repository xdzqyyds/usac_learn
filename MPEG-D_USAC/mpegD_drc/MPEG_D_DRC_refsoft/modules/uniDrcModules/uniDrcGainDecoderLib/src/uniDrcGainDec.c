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
#include <string.h>
#include "uniDrcGainDecoder.h"

#if MPEG_D_DRC_EXTENSION_V1
extern const CicpSigmoidCharacteristicParam cicpSigmoidCharacteristicParamTable[];
#endif

int
initGainBuffer(const int index,
               const int gainElementCount,
               const int drcFrameSize,
               GainBuffer* gainBuffer)
{
    int i,j;
    
    if (gainElementCount > 0)
    {
        gainBuffer->bufferForInterpolation = (BufferForInterpolation*) calloc(gainElementCount, sizeof(BufferForInterpolation));
        gainBuffer->bufferForInterpolationCount = gainElementCount;
        for (i=0; i<gainBuffer->bufferForInterpolationCount; i++)
        {
            gainBuffer->bufferForInterpolation[i].node.time = 0;
            gainBuffer->bufferForInterpolation[i].nodePrevious.time = -1;
            gainBuffer->bufferForInterpolation[i].node.gainDb = 0.0f;
            gainBuffer->bufferForInterpolation[i].node.slope = 0.0f;
            
            for (j=0; j<2 * AUDIO_CODEC_FRAME_SIZE_MAX + MAX_SIGNAL_DELAY; j++)
            {
                gainBuffer->bufferForInterpolation[i].lpcmGains[j] = 1.f;
            }
        }
    }
#if DRC_GAIN_DEBUG_FILE
    char fName[50];
    sprintf(fName, "gainDebug%d.dat", index);
    gainBuffer->fGainDebug = fopen(fName, "wb");
    if (gainBuffer->fGainDebug == NULL)
    {
        fprintf(stderr, "ERROR cannot open file gainDebug.dat\n");
        return (UNEXPECTED_ERROR);
    }
#endif
    return (0);
}

int
initDrcGainBuffers (UniDrcConfig* uniDrcConfig,
                    DrcParams* drcParams,
                    DrcGainBuffers* drcGainBuffers)
{
    int n, err;
    
    for (n=0; n<SEL_DRC_COUNT; n++)
    {
        if (drcParams->selDrc[n].drcInstructionsIndex >= 0)
        {
            DrcInstructionsUniDrc* drcInstructionsUniDrc = &(uniDrcConfig->drcInstructionsUniDrc[drcParams->selDrc[n].drcInstructionsIndex]);
            
            err = initGainBuffer(n,
                                 drcInstructionsUniDrc->gainElementCount,
                                 drcParams->drcFrameSize,
                                 &(drcGainBuffers->gainBuffer[n]));
            if (err) return (err);
        }
    }
    return (0);
}



int
removeGainBuffer (GainBuffer* gainBuffer)
{
    if (gainBuffer->bufferForInterpolation)
    {
        free(gainBuffer->bufferForInterpolation);
        gainBuffer->bufferForInterpolation = NULL;
        gainBuffer->bufferForInterpolationCount = 0;
    }
#if DRC_GAIN_DEBUG_FILE
    fclose (gainBuffer->fGainDebug);
#endif
    return (0);
}

int
removeDrcGainBuffers (DrcParams* drcParams,
                      DrcGainBuffers* drcGainBuffers)
{
    int err = 0, n;
    for (n=0; n<SEL_DRC_COUNT; n++)
    {
        if (drcParams->selDrc[n].drcInstructionsIndex >= 0)
        {
            err = removeGainBuffer (&(drcGainBuffers->gainBuffer[n]));
        }
        if (err) return (err);
    }
    return (0);
}

#if MPEG_D_DRC_EXTENSION_V1
#ifdef AMD2_COR1
#define DOLBY_NODE_TABLE_SIZE 5
#define DOLBY_NODE_LEFT_COUNT 2
#define DOLBY_NODE_RIGHT_COUNT 4

typedef struct {
    float nodeLevel;
    float nodeGain;
} DolbyNode;

static const DolbyNode DolbyNodesTableLeft[DOLBY_NODE_TABLE_SIZE][DOLBY_NODE_LEFT_COUNT] = {
    {{-41.0f, 0.0f}, {-53.0f, 6.0f}}, /* 0 (7) */
    {{-43.0f, 6.0f}, {-44.0f, 6.0f}}, /* 1 (8) */
    {{-41.0f, 0.0f}, {-65.0f,12.0f}}, /* 2 (9) */
    {{-55.0f,12.0f}, {-56.0f,12.0f}}, /* 3 (10) */
    {{-50.0f,15.0f}, {-51.0f,15.0f}}, /* 4 (11) */
};
static const DolbyNode DolbyNodesTableRight[DOLBY_NODE_TABLE_SIZE][DOLBY_NODE_RIGHT_COUNT] = {
    {{-21.0f, 0.0f}, {-11.0f, -5.0f}, { 9.0f,-24.0f}, {19.0f, -34.0f}}, /* 0 (7) */
    {{-26.0f, 0.0f}, {-16.0f, -5.0f}, { 4.0f,-24.0f}, {14.0f, -34.0f}}, /* 1 (8) */
    {{-21.0f, 0.0f}, {  9.0f,-15.0f}, {19.0f,-25.0f}, {29.0f, -35.0f}}, /* 2 (9) */
    {{-26.0f, 0.0f}, {-16.0f, -5.0f}, { 4.0f,-24.0f}, {14.0f, -34.0f}}, /* 3 (10) */
    {{-26.0f, 0.0f}, {-16.0f, -5.0f}, { 4.0f,-24.0f}, {14.0f, -34.0f}}, /* 4 (11) */
};
#endif
int
convertCicpToSplitCharacteristic(const int cicpCharacteristic,
                                 SplitDrcCharacteristic* splitDrcCharacteristicLeft,
                                 SplitDrcCharacteristic* splitDrcCharacteristicRight)
{
    if ((cicpCharacteristic > 0) && (cicpCharacteristic < 7))
    {
#ifndef AMD2_COR1
        if (cicpCharacteristic == 1) {
            fprintf(stderr, "WARNING: CICP characteristic 1 is not invertible\n");
        }
#endif
        splitDrcCharacteristicLeft->characteristicFormat = 0;
        splitDrcCharacteristicLeft->ioRatio = cicpSigmoidCharacteristicParamTable[cicpCharacteristic-1].ioRatio;
        splitDrcCharacteristicLeft->gain = 32.0f;
        splitDrcCharacteristicLeft->exp = cicpSigmoidCharacteristicParamTable[cicpCharacteristic-1].expLo;
        splitDrcCharacteristicLeft->flipSign = 0;
        
        splitDrcCharacteristicRight->characteristicFormat = 0;
        splitDrcCharacteristicRight->ioRatio = cicpSigmoidCharacteristicParamTable[cicpCharacteristic-1].ioRatio;
        splitDrcCharacteristicRight->gain = -32.0f;
        splitDrcCharacteristicRight->exp = cicpSigmoidCharacteristicParamTable[cicpCharacteristic-1].expHi;
        splitDrcCharacteristicRight->flipSign = 0;
    }
#ifdef AMD2_COR1
    else if ((cicpCharacteristic > 6) && (cicpCharacteristic < 12)) {
        int dolbyIndex = cicpCharacteristic - 7;
        int i;
        splitDrcCharacteristicLeft->characteristicFormat = 1;
        splitDrcCharacteristicLeft->characteristicNodeCount = DOLBY_NODE_LEFT_COUNT;
        splitDrcCharacteristicLeft->nodeLevel[0] = -31.0f;
        splitDrcCharacteristicLeft->nodeGain[0] = 0.0f;
        
        for (i=0; i<DOLBY_NODE_LEFT_COUNT; i++) {
            splitDrcCharacteristicLeft->nodeLevel[i+1] = DolbyNodesTableLeft[dolbyIndex][i].nodeLevel;
            splitDrcCharacteristicLeft->nodeGain[i+1]  = DolbyNodesTableLeft[dolbyIndex][i].nodeGain;
        }
        splitDrcCharacteristicRight->characteristicFormat = 1;
        splitDrcCharacteristicRight->characteristicNodeCount = DOLBY_NODE_RIGHT_COUNT;
        splitDrcCharacteristicRight->nodeLevel[0] = -31.0f;
        splitDrcCharacteristicRight->nodeGain[0] = 0.0f;
        
        for (i=0; i<DOLBY_NODE_RIGHT_COUNT; i++) {
            splitDrcCharacteristicRight->nodeLevel[i+1] = DolbyNodesTableRight[dolbyIndex][i].nodeLevel;
            splitDrcCharacteristicRight->nodeGain[i+1]  = DolbyNodesTableRight[dolbyIndex][i].nodeGain;
        }
    }
#endif
    else
    {
#ifdef AMD2_COR1
        fprintf(stderr, "WARNING: CICP characteristics > 11 are not supported\n");
#else
        fprintf(stderr, "WARNING: CICP characteristics > 6 not supported yet\n");
#endif
    }
    return(0);
}

int
compressorIO_sigmoid(SplitDrcCharacteristic* splitDrcCharacteristic,
                     const float inLevelDb,
                     float* outGainDb)
{
    float tmp;
    float ioRatio = splitDrcCharacteristic->ioRatio;
    float gainDbLimit = splitDrcCharacteristic->gain;
    float exp = splitDrcCharacteristic->exp;
    
    tmp = (DRC_INPUT_LOUDNESS_TARGET - inLevelDb) * ioRatio;
    if (exp < 1000.0f) {
        float x = tmp/gainDbLimit;
        if (x < 0.0f) {
            printf("ERROR in DRC characteristic parameters\n");
            return(UNEXPECTED_ERROR);
        }
        *outGainDb = tmp / pow(1.0f + pow(x, exp), 1.0f/exp);
    } else {
        *outGainDb = tmp;
    }
    if (splitDrcCharacteristic->flipSign == 1) {
        *outGainDb = - *outGainDb;
    }
    return (0);
}

/* TODO: test this function */
int
compressorIO_sigmoid_inverse(SplitDrcCharacteristic* splitDrcCharacteristic,
                             const float gainDb,
                             float* inLev)
{
    float ioRatio = splitDrcCharacteristic->ioRatio;
    float gainDbLimit = splitDrcCharacteristic->gain;
    float exp = splitDrcCharacteristic->exp;
    float tmp = gainDb;
    
    if (splitDrcCharacteristic->flipSign == 1) {
        tmp = - gainDb;
    }
    if (exp < 1000.0f) {
        float x = tmp/gainDbLimit;
        if (x < 0.0f) {
            printf("ERROR in DRC characteristic parameters\n");
            return(UNEXPECTED_ERROR);
        }
        tmp = tmp / pow(1.0f - pow(x, exp), 1.0f / exp);
    }
#ifdef AMD2_COR1
    if (ioRatio == 0.0f) {
        printf("ERROR: DRC characteristic cannot be inverted\n");
        return(UNEXPECTED_ERROR);
    }
#endif
    *inLev = DRC_INPUT_LOUDNESS_TARGET - tmp / ioRatio;
    
    return(0);
}

int
compressorIO_nodesLeft(SplitDrcCharacteristic* splitDrcCharacteristic,
                       const float inLevelDb,
                       float *outGainDb)
{
    int n;
    float w;
    float* nodeLevel = splitDrcCharacteristic->nodeLevel;
    float* nodeGain = splitDrcCharacteristic->nodeGain;
    
    if (inLevelDb > DRC_INPUT_LOUDNESS_TARGET) {
        printf("ERROR in compressorIO_nodesLeft\n");
        return (UNEXPECTED_ERROR);
    }
    for (n=1; n<=splitDrcCharacteristic->characteristicNodeCount; n++) {
        if ((inLevelDb <= nodeLevel[n-1]) && (inLevelDb > nodeLevel[n])) {
            w = (nodeLevel[n]- inLevelDb)/(nodeLevel[n]-nodeLevel[n-1]);
            *outGainDb = (w * nodeGain[n-1] + (1.0-w) * nodeGain[n]);
#ifdef AMD2_COR1
            return (0);
#endif
        }
    }
    *outGainDb = nodeGain[splitDrcCharacteristic->characteristicNodeCount];
    return (0);
}

int
compressorIO_nodesRight(SplitDrcCharacteristic* splitDrcCharacteristic,
                        const float inLevelDb,
                        float *outGainDb)
{
    int n;
    float w;
    float* nodeLevel = splitDrcCharacteristic->nodeLevel;
    float* nodeGain = splitDrcCharacteristic->nodeGain;
    
    if (inLevelDb < DRC_INPUT_LOUDNESS_TARGET) {
        printf("ERROR in compressorIO_nodesRight\n");
        return (UNEXPECTED_ERROR);
    }
    for (n=1; n<=splitDrcCharacteristic->characteristicNodeCount; n++) {
        if ((inLevelDb >= nodeLevel[n-1]) && (inLevelDb < nodeLevel[n])) {
            w = (nodeLevel[n]- inLevelDb)/(nodeLevel[n]-nodeLevel[n-1]);
            *outGainDb = (w * nodeGain[n-1] + (1.0-w) * nodeGain[n]);
#ifdef AMD2_COR1
            return (0);
#endif
        }
    }
    *outGainDb = (nodeGain[splitDrcCharacteristic->characteristicNodeCount]);
    return (0);
}

/* TODO: test this function */
int
compressorIO_nodes_inverse(SplitDrcCharacteristic* splitDrcCharacteristic,
                           const float gainDb,
                           float *inLev)
{
    int n;
#ifdef AMD2_COR1
    int k;
#endif
    float w;
#ifdef AMD2_COR1
    int gainIsNegative = 0;
#endif
    float* nodeLevel = splitDrcCharacteristic->nodeLevel;
    float* nodeGain = splitDrcCharacteristic->nodeGain;
    int nodeCount = splitDrcCharacteristic->characteristicNodeCount;
#ifdef AMD2_COR1
    for (k=1; k<=splitDrcCharacteristic->characteristicNodeCount; k++) {
        if (splitDrcCharacteristic->nodeGain[k] < 0.0f) {
            gainIsNegative = 1;
        }
    }
    if (gainIsNegative == 1) {
#else
    if (nodeGain[1] < 0.0f) {
#endif
        if (gainDb <= nodeGain[nodeCount]) {
            *inLev = nodeLevel[nodeCount];
        }
        else {
            if (gainDb >= 0.0f) {
                *inLev = DRC_INPUT_LOUDNESS_TARGET;
            }
            else {
                for (n=1; n<=nodeCount; n++) {
                    if ((gainDb <= nodeGain[n-1]) && (gainDb > nodeGain[n])) {
#ifdef AMD2_COR1
                        float gainDelta = nodeGain[n]-nodeGain[n-1];
                        if (gainDelta == 0.0f) {
#if AMD2_COR2
                            *inLev = nodeLevel[n-1];
#else
                            *inLev = nodeGain[n-1];
#endif
                            return(0);
                        }
                        w = (nodeGain[n]-gainDb)/gainDelta;
                        *inLev = (w * nodeLevel[n-1] + (1.0-w) * nodeLevel[n]);
                        return (0);
                    }
                }
                *inLev = nodeLevel[nodeCount];
#else
                        w = (nodeGain[n]-gainDb)/(nodeGain[n]-nodeGain[n-1]);
                        *inLev = (w * nodeLevel[n-1] + (1.0-w) * nodeLevel[n]);
                    }
                }
#endif
            }
        }
    }
    else {
        if (gainDb >= nodeGain[nodeCount]) {
            *inLev = nodeLevel[nodeCount];
        }
        else {
            if (gainDb <= 0.0f) {
                *inLev = DRC_INPUT_LOUDNESS_TARGET;
            }
            else {
                for (n=1; n<=nodeCount; n++) {
                    if ((gainDb >= nodeGain[n-1]) && (gainDb < nodeGain[n])) {
#ifdef AMD2_COR1
                        float gainDelta = nodeGain[n]-nodeGain[n-1];
                        if (gainDelta == 0.0f) {
#if AMD2_COR2
                            *inLev = nodeLevel[n-1];
#else
                            *inLev = nodeGain[n-1];
#endif
                            return(0);
                        }
                        w = (nodeGain[n]-gainDb)/(nodeGain[n]-nodeGain[n-1]);
                        *inLev = (w * nodeLevel[n-1] + (1.0-w) * nodeLevel[n]);
                        return (0);
                    }
                }
                *inLev = nodeLevel[nodeCount];
#else
                        w = (nodeGain[n]-gainDb)/(nodeGain[n]-nodeGain[n-1]);
                        *inLev = (w * nodeLevel[n-1] + (1.0-w) * nodeLevel[n]);
                    }
                }
#endif
            }
        }
    }
    return (0);
}

#endif /* MPEG_D_DRC_EXTENSION_V1 */

int
mapGain (
#if MPEG_D_DRC_EXTENSION_V1
         SplitDrcCharacteristic* splitDrcCharacteristicSource,
         SplitDrcCharacteristic* splitDrcCharacteristicTarget,
#endif /* MPEG_D_DRC_EXTENSION_V1 */
         const float gainInDb,
         float* gainOutDb)
{
#if MPEG_D_DRC_EXTENSION_V1
    float inLevel;
    int err = 0;
    
    switch (splitDrcCharacteristicSource->characteristicFormat) {
        case CHARACTERISTIC_SIGMOID:
            err = compressorIO_sigmoid_inverse(splitDrcCharacteristicSource, gainInDb, &inLevel);
            if (err) return (err);
            break;
        case CHARACTERISTIC_NODES:
            err = compressorIO_nodes_inverse(splitDrcCharacteristicSource, gainInDb, &inLevel);
            if (err) return (err);
            break;
        case CHARACTERISTIC_PASS_THRU:
            inLevel = gainInDb;
            break;
        default:
            return(UNEXPECTED_ERROR);
            break;
    }
    switch (splitDrcCharacteristicTarget->characteristicFormat) {
        case CHARACTERISTIC_SIGMOID:
            err = compressorIO_sigmoid(splitDrcCharacteristicTarget, inLevel, gainOutDb);
            if (err) return (err);
            break;
        case CHARACTERISTIC_NODES:
            if (inLevel < DRC_INPUT_LOUDNESS_TARGET) {
                err = compressorIO_nodesLeft(splitDrcCharacteristicTarget, inLevel, gainOutDb);
                if (err) return (err);
            }
            else {
                err = compressorIO_nodesRight(splitDrcCharacteristicTarget, inLevel, gainOutDb);
                if (err) return (err);
            }
            break;
        case CHARACTERISTIC_PASS_THRU:
            *gainOutDb = inLevel;
            break;
        default:
            break;
    }
#else
    *gainOutDb = gainInDb;
#endif /* MPEG_D_DRC_EXTENSION_V1 */
    return (0);
}

int
toLinear (InterpolationParams* interpolationParams,
          const int     drcBand,
          const float   gainDbIn,
          const float   slopeDb,
          float*        gainLin,
          float*        slopeLin)
{
#if MPEG_D_DRC_EXTENSION_V1
    int err = 0;
#endif
    float gainDb = gainDbIn;
    float gainRatio = 1.0;
    float gainDbMapped;
    GainModifiers*  gainModifiers = interpolationParams->gainModifiers;
    if (interpolationParams->drcEffectAllowsGainModification) {
#if MPEG_D_DRC_EXTENSION_V1
        SplitDrcCharacteristic* splitDrcCharacteristicSource;
        /* TODO: host target characteristic is not considered */
        /* TODO: move computation of slopeIsNegative to initialization. Check if the slope is invertible */
        int slopeIsNegative;
        
        if (interpolationParams->drcCharacteristicPresent) {
            if (interpolationParams->drcSourceCharacteristicFormatIsCICP)
            {
                /* we should not get here. CICP characteristics are converted to split characteristics */
#if DEBUG_WARNINGS
                fprintf(stderr, "WARNING: CICP characteristics must be converted\n");
#endif
            }
            else {
                slopeIsNegative = 0;
                splitDrcCharacteristicSource = interpolationParams->splitSourceCharacteristicLeft;
                /* assuming that slopes of left and right characteristics are identical */
                if (splitDrcCharacteristicSource->characteristicFormat == 0)
                {
#ifdef AMD2_COR1
                    slopeIsNegative = 1 - splitDrcCharacteristicSource->flipSign;
                }
                else
                {
                    int k;
                    for (k=1; k<=splitDrcCharacteristicSource->characteristicNodeCount; k++) {
                        if (splitDrcCharacteristicSource->nodeGain[k] > 0.0f) {
                            slopeIsNegative = 1;
                        }
                    }
                }
#else
                    slopeIsNegative = 1;
                }
                else
                {
                    if (splitDrcCharacteristicSource->nodeGain[1] > 0.0f) {
                        slopeIsNegative = 1;
                    }
                }
#endif
                if (gainDb == 0.0f) {
                    if (((gainModifiers->targetCharacteristicLeftPresent[drcBand] == 1) &&
                        (interpolationParams->splitTargetCharacteristicLeft->characteristicFormat == CHARACTERISTIC_PASS_THRU)) ||
                        ((gainModifiers->targetCharacteristicRightPresent[drcBand] == 1) &&
                         (interpolationParams->splitTargetCharacteristicRight->characteristicFormat == CHARACTERISTIC_PASS_THRU)))
                    {
                        gainDbMapped = DRC_INPUT_LOUDNESS_TARGET;
                        gainDb = DRC_INPUT_LOUDNESS_TARGET;  /* for loudness EQ */
                    }
                }
                else
                {
                    if (((gainDb > 0.0f) && (slopeIsNegative == 1)) || ((gainDb < 0.0f) && (slopeIsNegative == 0)))
                    {
                        /* left side */
                        if (gainModifiers->targetCharacteristicLeftPresent[drcBand] == 1)
                        {
                            err = mapGain(splitDrcCharacteristicSource, interpolationParams->splitTargetCharacteristicLeft, gainDb, &gainDbMapped);
                            if (err) return (err);
                            gainRatio = gainDbMapped / gainDb;   /* target characteristic in payload */
                        }
                        
                    }
#ifdef AMD2_COR1
                    else  /*  if (((gainDb < 0.0f) && (slopeIsNegative == 1)) || ((gainDb > 0.0f) && (slopeIsNegative == 0))) */
#else
                    else if (((gainDb < 0.0f) && (slopeIsNegative == 1)) || ((gainDb > 0.0f) && (slopeIsNegative == 0)))
#endif
                    {
                        /* right side */
                        if (gainModifiers->targetCharacteristicRightPresent[drcBand] == 1)
                        {
                            splitDrcCharacteristicSource = interpolationParams->splitSourceCharacteristicRight;
                            err = mapGain(splitDrcCharacteristicSource, interpolationParams->splitTargetCharacteristicRight, gainDb, &gainDbMapped);
                            if (err) return (err);
                            gainRatio = gainDbMapped / gainDb;   /* target characteristic in payload */
                        }
                    }
                }
            }
        }
#else
        if ((interpolationParams->characteristicIndex > 0) && (gainDb != 0.0f)){
            mapGain(gainDb, &gainDbMapped);
            gainRatio = gainDbMapped / gainDb;          /* target characteristic from host */
        }
#endif  /* MPEG_D_DRC_EXTENSION_V1 */
        
        if (gainDb < 0.0f) {
            gainRatio *= interpolationParams->compress;
        }
        else {
            gainRatio *= interpolationParams->boost;
        }
    }
    if (gainModifiers->gainScalingPresent[drcBand] == 1) {
        if (gainDb < 0.0) {
            gainRatio *= gainModifiers->attenuationScaling[drcBand];
        }
        else {
            gainRatio *= gainModifiers->amplificationScaling[drcBand];
        }
    }
    if ((interpolationParams->duckingModifiers->duckingScalingPresent == 1) && (interpolationParams->drcEffectIsDucking == TRUE)) {
        gainRatio *= interpolationParams->duckingModifiers->duckingScaling;
    }
    
#if MPEG_D_DRC_EXTENSION_V1
    if (interpolationParams->interpolationForLoudEq == 1)
    {
        *gainLin = gainRatio * gainDb + gainModifiers->gainOffset[drcBand];    /* gainLin represents level in dB */
        *slopeLin = 0.0f;                                                      /* not used (always linear interpolation) */
    }
    else
#endif  /* MPEG_D_DRC_EXTENSION_V1 */
    {
        *gainLin = (float)pow(2.0, (double)(gainRatio * gainDb / 6.0f));
        *slopeLin = SLOPE_FACTOR_DB_TO_LINEAR * gainRatio * *gainLin * slopeDb;
        
        if (gainModifiers->gainOffsetPresent[drcBand] == 1) {
            *gainLin *= (float)pow(2.0, (double)(gainModifiers->gainOffset[drcBand]/6.0f));
        }
        if ((interpolationParams->limiterPeakTargetPresent == 1)  && (interpolationParams->drcEffectIsClipping == TRUE)) {
            /* loudnessNormalizationGainModificationDb is included in loudnessNormalizationGainDb */
            *gainLin *= (float)pow(2.0, max(0.0, -interpolationParams->limiterPeakTarget - interpolationParams->loudnessNormalizationGainDb)/6.0);
            if (*gainLin >= 1.0) {
                *gainLin = 1.0;
                *slopeLin = 0.0;
            }
        }
    }
    return (0);
}

int
interpolateDrcGain(InterpolationParams* interpolationParams,
                   const int    drcBand,
                   const int    tGainStep,
                   const float  gain0,
                   const float  gain1,
                   const float  slope0,
                   const float  slope1,
                   float* result)
{
    int err = 0, n;
    float k1, k2, a, b, c, d;
    
    float slopeLeft;
    float slopeRight;
    float gainLeft;
    float gainRight;
    
    bool useCubicInterpolation = TRUE;
    int tConnect;
    float tConnectFloat;
    
    if (tGainStep <= 0)
    {
        fprintf(stderr, "ERROR: tGainStep must be positive: %d\n", tGainStep);
        return (UNEXPECTED_ERROR);
    }
    
    err = toLinear (interpolationParams, drcBand, gain0, slope0, &gainLeft, &slopeLeft);
    if (err) return (err);
    err = toLinear (interpolationParams, drcBand, gain1, slope1, &gainRight, &slopeRight);
    if (err) return (err);
    
    if (interpolationParams->gainInterpolationType == GAIN_INTERPOLATION_TYPE_SPLINE) {
        slopeLeft = slopeLeft/(float) interpolationParams->deltaTmin;
        slopeRight = slopeRight/(float) interpolationParams->deltaTmin;
        if ((float)fabs((double)slopeLeft) > (float)fabs((double)slopeRight)) {
            tConnectFloat = 2.0f * (gainRight - gainLeft - slopeRight * tGainStep) /
            (slopeLeft - slopeRight);
            tConnect = (int) (0.5f + tConnectFloat);
            if ((tConnect >= 0) && (tConnect < tGainStep)) {
                useCubicInterpolation = FALSE;
                
                result[0] = gainLeft;
                result[tGainStep] = gainRight;
                
                a = (slopeRight - slopeLeft) / (tConnectFloat + tConnectFloat);
                b = slopeLeft;
                c = gainLeft;
                
                for (n=1; n<tConnect; n++) {
                    float t = (float) n;
                    result[n] = (a * t + b ) * t + c;
                    result[n] = max(0.0f, result[n]);
                }
                a = slopeRight;
                b = gainRight;
                for (   ; n<tGainStep; n++) {
                    float t = (float) (n - tGainStep);
                    result[n] = a * t + b;
                }
            }
        }
        else if ((float)fabs((double)slopeLeft) < (float)fabs((double)slopeRight))
        {
            tConnectFloat = 2.0f * (gainLeft - gainRight + slopeLeft * tGainStep) /
            (slopeLeft - slopeRight);
            tConnectFloat = tGainStep - tConnectFloat;
            tConnect = (int) (0.5f + tConnectFloat);
            if ((tConnect >= 0) && (tConnect < tGainStep)) {
                useCubicInterpolation = FALSE;
                
                result[0] = gainLeft;
                result[tGainStep] = gainRight;
                
                a = slopeLeft;
                b = gainLeft;
                for (n=1; n<tConnect; n++) {
                    float t = (float) n;
                    result[n] = a * t + b;
                }
                a = (slopeRight - slopeLeft) / (2.0f * (tGainStep - tConnectFloat));
                b = - slopeRight;
                c = gainRight;
                
                for (   ; n<tGainStep; n++) {
                    float t = (float) (tGainStep-n);
                    result[n] = (a * t + b ) * t + c;
                    result[n] = max(0.0f, result[n]);
                }
            }
        }
        
        if (useCubicInterpolation == TRUE)
        {
            float tGainStepInv = 1.0f / (float)tGainStep;
            float tGainStepInv2 = tGainStepInv * tGainStepInv;
            
            k1 = (gainRight - gainLeft) * tGainStepInv2;
            k2 = slopeRight + slopeLeft;
            
            a = tGainStepInv * (tGainStepInv * k2 - 2.0f * k1);
            b = 3.0f * k1 - tGainStepInv * (k2 + slopeLeft);
            c = slopeLeft;
            d = gainLeft;
            
            result[0] = gainLeft;
            result[tGainStep] = gainRight;
            
            for (n=1; n<tGainStep; n++) {
                float t = (float) n;
                result[n] = (((a * t + b ) * t + c ) * t ) + d;
                result[n] = max(0.0f, result[n]);
            }
        }
    }
    else {  /* gainInterpolationType == GAIN_INTERPOLATION_TYPE_LINEAR */
        a = (gainRight - gainLeft) / (float)tGainStep;
        b = gainLeft;
        
        result[0] = gainLeft;
        result[tGainStep] = gainRight;
        for (n=1; n<tGainStep; n++) {
            float t = (float) n;
            result[n] = a * t + b;
        }
    }
    return (0);
}


int
advanceBuffer(const int drcFrameSize,
              GainBuffer* gainBuffer)
{
    int n;
    BufferForInterpolation* bufferForInterpolation;
#if DRC_GAIN_DEBUG_FILE
    /* interleave and output for debugging */
    int i;
    static int bCount = 0;
    bCount++;
    if (bCount > 3)
    {
        for (i=0; i<drcFrameSize; i++)
        {
            for (n=0; n<gainBuffer->bufferForInterpolationCount; n++)
            {
                bufferForInterpolation = &(gainBuffer->bufferForInterpolation[n]);
                float tmp = 20.0f / 24.0f * (float)log10(bufferForInterpolation->lpcmGains[i]);
                fwrite(&(tmp), 1, sizeof(float), gainBuffer->fGainDebug);
            }
        }
    }
#endif
    for (n=0; n<gainBuffer->bufferForInterpolationCount; n++)
    {
        bufferForInterpolation = &(gainBuffer->bufferForInterpolation[n]);
        bufferForInterpolation->nodePrevious = bufferForInterpolation->node;
        bufferForInterpolation->nodePrevious.time -= drcFrameSize;
        memmove(bufferForInterpolation->lpcmGains, bufferForInterpolation->lpcmGains + drcFrameSize, sizeof(float) * (drcFrameSize + MAX_SIGNAL_DELAY));
    }
    return(0);
}

int
concatenateSegments(const int drcFrameSize,
                    const int drcBand,
                    InterpolationParams* interpolationParams,
                    SplineNodes* splineNodes,
                    BufferForInterpolation* bufferForInterpolation)
{
    int timePrev, duration, n, err = 0;
    float gainDb = 0.0f, gainDbPrev, slope = 0.0f, slopePrev;
    
    timePrev = bufferForInterpolation->nodePrevious.time;
    gainDbPrev = bufferForInterpolation->nodePrevious.gainDb;
    slopePrev = bufferForInterpolation->nodePrevious.slope;
    for (n=0; n<splineNodes->nNodes; n++)
    {
        duration = splineNodes->node[n].time - timePrev;
        gainDb = splineNodes->node[n].gainDb;
        slope = splineNodes->node[n].slope;
#if DEBUG_NODES
        printf("gain: %8.4f %8.4f slope: %12.6f %12.6f time: %4d %4d duration %4d\n", gainDbPrev,
               gainDb,
               slopePrev,
               slope,
               timePrev,
               splineNodes->node[n].time,
               duration);
#endif
        err = interpolateDrcGain(interpolationParams,
                                 drcBand,
                                 duration,
                                 gainDbPrev,
                                 gainDb,
                                 slopePrev,
                                 slope,
                                 bufferForInterpolation->lpcmGains + MAX_SIGNAL_DELAY + drcFrameSize + timePrev);
        if (err) return (err);
        
        timePrev = splineNodes->node[n].time;
        gainDbPrev = gainDb;
        slopePrev = slope;
    }
    
    /* remember last node */
    bufferForInterpolation->node.gainDb = gainDb;
    bufferForInterpolation->node.slope = slope;
    bufferForInterpolation->node.time = timePrev;
    
#if DEBUG_NODES
    printf("==========================\n");
#endif
    return(0);
}


/* compute DRC gains of all gain sequences used */
int
getDrcGain (HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
            HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
            HANDLE_UNI_DRC_GAIN              hUniDrcGain,
            const float                      compress,
            const float                      boost,
            const int                        characteristicIndex,
            const float                      loudnessNormalizationGainDb,
            const int                        selDrcIndex,
            DrcGainBuffers*                  drcGainBuffers)
{
    DrcParams* drcParams = &(hUniDrcGainDecStructs->drcParams);
    int drcInstructionsIndex = drcParams->selDrc[selDrcIndex].drcInstructionsIndex;
    if (drcInstructionsIndex >= 0)
    {
        int b, g, gainElementIndex, err = 0;
#if AMD1_SYNTAX
        int parametricDrcInstanceIndex = 0;
#endif /* AMD1_SYNTAX */
        InterpolationParams interpolationParams = {0};
        
        DrcInstructionsUniDrc* drcInstructionsUniDrc = &(hUniDrcConfig->drcInstructionsUniDrc[drcInstructionsIndex]);
        int drcSetEffect = drcInstructionsUniDrc->drcSetEffect;
        int nDrcChannelGroups = drcInstructionsUniDrc->nDrcChannelGroups;
#if MPEG_D_DRC_EXTENSION_V1
        DrcCoefficientsUniDrc* drcCoefficientsUniDrc = NULL;
        int drcCoefficientsIndex = drcParams->selDrc[selDrcIndex].drcCoefficientsIndex;
        if (drcCoefficientsIndex >= 0)
        {
            drcCoefficientsUniDrc = &(hUniDrcConfig->drcCoefficientsUniDrc[drcCoefficientsIndex]);
            interpolationParams.interpolationForLoudEq = 0;
        }
        else
        {
            fprintf(stderr,"ERROR: index of DRC coefficients is %d\n", drcCoefficientsIndex);
            return (UNEXPECTED_ERROR);
        }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
        
        interpolationParams.loudnessNormalizationGainDb = loudnessNormalizationGainDb;
        interpolationParams.characteristicIndex = characteristicIndex; /* Note: mapping to target DRC characteristic is not supported */
        interpolationParams.compress = compress;
        interpolationParams.boost = boost;
        interpolationParams.limiterPeakTargetPresent = drcInstructionsUniDrc->limiterPeakTargetPresent;
        interpolationParams.limiterPeakTarget = drcInstructionsUniDrc->limiterPeakTarget;
        
        if ( ((drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF)) == 0) && (drcSetEffect != EFFECT_BIT_FADE) && (drcSetEffect != EFFECT_BIT_CLIPPING))
        {
            interpolationParams.drcEffectAllowsGainModification = TRUE;
        }
        else
        {
            interpolationParams.drcEffectAllowsGainModification = FALSE;
        }
        if (drcSetEffect & (EFFECT_BIT_DUCK_OTHER | EFFECT_BIT_DUCK_SELF))
        {
            interpolationParams.drcEffectIsDucking = TRUE;
        }
        else
        {
            interpolationParams.drcEffectIsDucking = FALSE;
        }
        if (drcSetEffect == EFFECT_BIT_CLIPPING) /* The only drcSetEffect is "clipping prevention" */
        {
            interpolationParams.drcEffectIsClipping = TRUE;
        }
        else
        {
            interpolationParams.drcEffectIsClipping = FALSE;
        }
        
        err = advanceBuffer(drcParams->drcFrameSize, &(drcGainBuffers->gainBuffer[selDrcIndex]));
        if (err) return (err);
        
        gainElementIndex = 0;
        for(g=0; g<nDrcChannelGroups; g++)
        {
            int gainSet = 0;
            int nDrcBands = 0;
            interpolationParams.gainInterpolationType = drcInstructionsUniDrc->gainInterpolationTypeForChannelGroup[g];
            interpolationParams.deltaTmin = drcInstructionsUniDrc->timeDeltaMinForChannelGroup[g];
            interpolationParams.duckingModifiers = &(drcInstructionsUniDrc->duckingModifiersForChannelGroup[g]);
            interpolationParams.gainModifiers = &(drcInstructionsUniDrc->gainModifiersForChannelGroup[g]);
#if AMD1_SYNTAX
            if (drcInstructionsUniDrc->channelGroupIsParametricDrc[g] == 0) {
#endif /* AMD1_SYNTAX */
                gainSet = drcInstructionsUniDrc->gainSetIndexForChannelGroup[g];
                nDrcBands = drcInstructionsUniDrc->bandCountForChannelGroup[g];
#if MPEG_D_DRC_EXTENSION_V1
                for(b=0; b<nDrcBands; b++)
                {
#ifdef AMD2_COR1
                    SplitDrcCharacteristic splitDrcCharacteristicLeft;
                    SplitDrcCharacteristic splitDrcCharacteristicRight;
                    GainParams* gainParams = &(drcCoefficientsUniDrc->gainSetParams[gainSet].gainParams[b]);
                    int seq                                                 = gainParams->gainSequenceIndex;
                    
                    interpolationParams.drcCharacteristicPresent            = gainParams->drcCharacteristicPresent;
                    if (gainParams->drcCharacteristicFormatIsCICP == 1) {
                        err = convertCicpToSplitCharacteristic(gainParams->drcCharacteristic, &splitDrcCharacteristicLeft, &splitDrcCharacteristicRight);
                        if (err) return err;
                        interpolationParams.splitSourceCharacteristicLeft  = &splitDrcCharacteristicLeft;
                        interpolationParams.splitSourceCharacteristicRight = &splitDrcCharacteristicRight;
                    }
                    else {
                        interpolationParams.splitSourceCharacteristicLeft  = &(drcCoefficientsUniDrc->splitCharacteristicLeft [gainParams->drcCharacteristicLeftIndex]);
                        interpolationParams.splitSourceCharacteristicRight = &(drcCoefficientsUniDrc->splitCharacteristicRight[gainParams->drcCharacteristicRightIndex]);
                    }
                    interpolationParams.drcSourceCharacteristicFormatIsCICP = 0;
                    interpolationParams.sourceDrcCharacteristic             = 0;
#else
                    GainParams* gainParams = &(drcCoefficientsUniDrc->gainSetParams[gainSet].gainParams[b]);
                    int seq                                                 = gainParams->gainSequenceIndex;
                    interpolationParams.drcCharacteristicPresent            = gainParams->drcCharacteristicPresent;
                    interpolationParams.drcSourceCharacteristicFormatIsCICP = gainParams->drcCharacteristicFormatIsCICP;
                    interpolationParams.sourceDrcCharacteristic             = gainParams->drcCharacteristic;
                    interpolationParams.splitSourceCharacteristicLeft       = &(drcCoefficientsUniDrc->splitCharacteristicLeft [gainParams->drcCharacteristicLeftIndex]);
                    interpolationParams.splitSourceCharacteristicRight      = &(drcCoefficientsUniDrc->splitCharacteristicRight[gainParams->drcCharacteristicRightIndex]);
#endif
                    interpolationParams.splitTargetCharacteristicLeft       = &(drcCoefficientsUniDrc->splitCharacteristicLeft [interpolationParams.gainModifiers->targetCharacteristicLeftIndex[b]]);
                    interpolationParams.splitTargetCharacteristicRight      = &(drcCoefficientsUniDrc->splitCharacteristicRight[interpolationParams.gainModifiers->targetCharacteristicRightIndex[b]]);
                    err = concatenateSegments (drcParams->drcFrameSize,
                                               b,
                                               &interpolationParams,
                                               &(hUniDrcGain->drcGainSequence[seq].splineNodes[0]),
                                               &(drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]));
                    if (err) return (err);
                    gainElementIndex++;
                }
#else
                for(b=0; b<nDrcBands; b++)
                {
                    err = concatenateSegments (drcParams->drcFrameSize,
                                               b,
                                               &interpolationParams,
                                               &(hUniDrcGain->drcGainSequence[gainSet].splineNodes[b]),
                                               &(drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]));
                    if (err) return (err);
                    gainElementIndex++;
                }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
#if AMD1_SYNTAX
            } else {
                if (drcParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_OFF && !hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].parametricDrcType == PARAM_DRC_TYPE_LIM) {
                    err = processParametricDrcInstance (hUniDrcGainDecStructs->audioIOBuffer.audioIOBuffer,
                                                        NULL,
                                                        NULL,
                                                        &hUniDrcGainDecStructs->parametricDrcParams,
                                                        &hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex]);
                    if (err) return (err);
                    
                    err = concatenateSegments(drcParams->drcFrameSize,
                                              0,
                                              &interpolationParams,
                                              &hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].splineNodes,
                                              &(drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]));
                    if (err) return (err);
#ifdef AMD1_PARAMETRIC_LIMITER
                } else if (drcParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_OFF && hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].parametricDrcType == PARAM_DRC_TYPE_LIM) {
#if AMD2_COR3
                    float* lpcmGains = (drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]).lpcmGains + MAX_SIGNAL_DELAY + drcParams->drcFrameSize;
#else
                    float* lpcmGains = (drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]).lpcmGains + MAX_SIGNAL_DELAY;
#endif
                    err = processParametricDrcTypeLim(hUniDrcGainDecStructs->audioIOBuffer.audioIOBuffer,
                                                      loudnessNormalizationGainDb,
                                                      &hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].parametricDrcTypeLimParams,
                                                      lpcmGains);
                    if (err) return (err);
#endif
                } else if (drcParams->subBandDomainMode != SUBBAND_DOMAIN_MODE_OFF && !hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].parametricDrcType == PARAM_DRC_TYPE_LIM) {
                    err = processParametricDrcInstance (NULL,
                                                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferReal,
                                                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferImag,
                                                        &hUniDrcGainDecStructs->parametricDrcParams,
                                                        &hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex]);
                    if (err) return (err);
                
                    err = concatenateSegments(drcParams->drcFrameSize,
                                              0,
                                              &interpolationParams,
                                              &hUniDrcGainDecStructs->parametricDrcParams.parametricDrcInstanceParams[parametricDrcInstanceIndex].splineNodes,
                                              &(drcGainBuffers->gainBuffer[selDrcIndex].bufferForInterpolation[gainElementIndex]));
                    if (err) return (err);
                    
                } else {
                    return (UNEXPECTED_ERROR); /* TODO: error handling */
                }
                gainElementIndex++;
                parametricDrcInstanceIndex++;
            }
#endif /* AMD1_SYNTAX */
        }
    }
    return(0);
}

#if MPEG_D_DRC_EXTENSION_V1
/* Compute the loudness values for loudness EQ. The values are currently not used (out of scope) */
int
getEqLoudness (HANDLE_LOUDNESS_EQ_STRUCT        hLoudnessEq,
               HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
               HANDLE_UNI_DRC_GAIN              hUniDrcGain)
{
    int b, i, err = 0;
    GainBuffer* gainBuffer = &(hLoudnessEq->loudEqGainBuffer);
    InterpolationParams interpolationParams;
    SplitDrcCharacteristic splitTargetCharacteristicPassThru, splitDrcCharacteristicLeft, splitDrcCharacteristicRight;
    DuckingModifiers duckingModifiers;
    GainModifiers gainModifiers;
    DrcCoefficientsUniDrc* drcCoefficientsUniDrc = &(hUniDrcConfig->drcCoefficientsUniDrc[0]); /* choose the coefficients that contain the sequence index */
    LoudEqInstructions* loudEqInstructions = &(hUniDrcConfig->uniDrcConfigExt.loudEqInstructions[hLoudnessEq->loudEqInstructionsSelectedIndex]);         /* select the one that matches the downmix etc. */
    
    interpolationParams.loudnessNormalizationGainDb = 0.0f;
    interpolationParams.compress = 1.0f;
    interpolationParams.boost = 1.0f;
    interpolationParams.limiterPeakTargetPresent = 0;
    interpolationParams.limiterPeakTarget = 0.0f;
    interpolationParams.drcEffectAllowsGainModification = TRUE;
    interpolationParams.drcEffectIsDucking = FALSE;
    interpolationParams.drcEffectIsClipping = FALSE;
    interpolationParams.duckingModifiers = &duckingModifiers;
    interpolationParams.duckingModifiers->duckingScalingPresent = 0;
    interpolationParams.gainModifiers = &gainModifiers;
    
    splitTargetCharacteristicPassThru.characteristicFormat = CHARACTERISTIC_PASS_THRU;
    interpolationParams.splitTargetCharacteristicLeft  = &splitTargetCharacteristicPassThru;
    interpolationParams.splitTargetCharacteristicRight = &splitTargetCharacteristicPassThru;
    interpolationParams.interpolationForLoudEq = 1;
    
    for (i=0; i<loudEqInstructions->loudEqGainSequenceCount; i++) {
        
        GainParams* gainParams = NULL;
        int seq = loudEqInstructions->gainSequenceIndex[i];
        int gainSet = drcCoefficientsUniDrc->gainSetParamsIndexForGainSequence[seq];
        GainSetParams* gainSetParams = &(drcCoefficientsUniDrc->gainSetParams[gainSet]);
        
        for(b=0; b<gainSetParams->bandCount; b++)
        {
            if (seq == gainSetParams->gainParams[b].gainSequenceIndex) {
                gainParams = &(gainSetParams->gainParams[b]);
                break;
            }
        }
        if (drcCoefficientsUniDrc->gainSetParams[seq].timeDeltaMinPresent)
        {
            interpolationParams.deltaTmin = drcCoefficientsUniDrc->gainSetParams[seq].timeDeltaMin;
        }
        else
        {
            interpolationParams.deltaTmin = hLoudnessEq->deltaTminDefault;
        }
        interpolationParams.gainInterpolationType = 1;  /* use linear interpolation (ignore the interpolation type of the sequence) */
        interpolationParams.gainModifiers->targetCharacteristicLeftPresent[b] = 1;
        interpolationParams.gainModifiers->targetCharacteristicRightPresent[b] = 1;
        if (loudEqInstructions->loudEqScaling[i] == 1.0f) {
            interpolationParams.gainModifiers->gainScalingPresent[b] = 0;
        }
        else
        {
            interpolationParams.gainModifiers->gainScalingPresent[b] = 1;
            interpolationParams.gainModifiers->attenuationScaling[b] = loudEqInstructions->loudEqScaling[i];
            interpolationParams.gainModifiers->amplificationScaling[b] = loudEqInstructions->loudEqScaling[i];
        }
        if (loudEqInstructions->loudEqOffset[i] == 0.0f) {
            interpolationParams.gainModifiers->gainOffsetPresent[b] = 0;
        }
        else
        {
            interpolationParams.gainModifiers->gainOffsetPresent[b] = 1;
            interpolationParams.gainModifiers->gainOffset[b] = loudEqInstructions->loudEqOffset[i];
        }
        interpolationParams.drcCharacteristicPresent            = 1;
        
        if (loudEqInstructions->drcCharacteristicFormatIsCICP[i] == 1) {
            err = convertCicpToSplitCharacteristic(loudEqInstructions->drcCharacteristic[i], &splitDrcCharacteristicLeft, &splitDrcCharacteristicRight);
            if (err) return err;
            interpolationParams.splitSourceCharacteristicLeft  = &splitDrcCharacteristicLeft;
            interpolationParams.splitSourceCharacteristicRight = &splitDrcCharacteristicRight;
        }
        else {
            interpolationParams.splitSourceCharacteristicLeft  = &(drcCoefficientsUniDrc->splitCharacteristicLeft [loudEqInstructions->drcCharacteristicLeftIndex[i]]);
            interpolationParams.splitSourceCharacteristicRight = &(drcCoefficientsUniDrc->splitCharacteristicRight[loudEqInstructions->drcCharacteristicRightIndex[i]]);
        }
        interpolationParams.drcSourceCharacteristicFormatIsCICP = 0;
        interpolationParams.sourceDrcCharacteristic             = 0;
        
        err = advanceBuffer(hLoudnessEq->drcFrameSize, &(gainBuffer[i]));
        if (err) return (err);
#if 0  /* For testing only */
        /* interleave and output for debugging */
        static FILE* fdebug=NULL;
        if (fdebug==NULL) fdebug=fopen("loudness.dat","wb");
        int n;
        {
                for (n=0; n<hLoudnessEq->drcFrameSize; n++)
                {
                    float tmp = (gainBuffer->bufferForInterpolation[i].lpcmGains[n]);
                    fwrite(&(tmp), 1, sizeof(float), fdebug);
                }
        }
#endif

        err = concatenateSegments (hLoudnessEq->drcFrameSize,
                                   b,
                                   &interpolationParams,
                                   &(hUniDrcGain->drcGainSequence[seq].splineNodes[0]),
                                   &(gainBuffer->bufferForInterpolation[i]));
        if (err) return (err);
    }
    return(0);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */




