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
#include <string.h>
#include "drcToolEncoder.h" 
#include "gainEnc.h"
#include "tables.h"

static int
checkDrcParams(const HANDLE_DRC_ENCODER hDrcEnc)
{
    if (hDrcEnc->drcFrameSize < 1)
    {
        fprintf(stderr, "Error: the DRC frame size must be positive.\n");
        return(UNEXPECTED_ERROR);
    }
    return (0);
}

int
openDrcToolEncoder(HANDLE_DRC_ENCODER* phDrcEnc,
                   UniDrcConfig uniDrcConfig,
                   LoudnessInfoSet loudnessInfoSet,
                   const int frameSize,
                   const int sampleRate,
                   const int delayMode,
                   const int domain) {
    int i, k, s, b, nGainValuesMax;
    DrcEncParams* hDrcEnc;
    DrcGroup* pDrcGroup;
    DrcGroupForOutput* pDrcGroupForOutput;
#if !AMD1_SYNTAX
    GainSetParams gainSetParams;
#endif
    *phDrcEnc = (HANDLE_DRC_ENCODER)calloc(1,sizeof(DrcEncParams));
    if (!*phDrcEnc) return -1;
    hDrcEnc = *phDrcEnc;
    
#if AMD1_SYNTAX
    /* get number of gain sequences from uniDrcConfig */
    if (uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0) {
        hDrcEnc->nSequences = uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSequenceCount;
    }
    else
#endif
    {
        int allBandGainCount = 0;
        int gainSetCount = uniDrcConfig.drcCoefficientsUniDrc[0].gainSetCount;
        for (i=0; i<gainSetCount; i++) {
            allBandGainCount += uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[i].bandCount;
        }
        hDrcEnc->nSequences = allBandGainCount;
    }
    
    /* override frameSize, if given in uniDrcConfig */
    if ((uniDrcConfig.drcCoefficientsUniDrcCount > 0) && (uniDrcConfig.drcCoefficientsUniDrc[0].drcFrameSizePresent)) {
        hDrcEnc->drcFrameSize = uniDrcConfig.drcCoefficientsUniDrc[0].drcFrameSize;
    }
#if AMD1_SYNTAX
    else if ((uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0) && (uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].drcFrameSizePresent)) {
        hDrcEnc->drcFrameSize = uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].drcFrameSize;
    }
#endif
    else
    {
        hDrcEnc->drcFrameSize = frameSize;
    }
    
    /* override sampleRate, if given in uniDrcConfig */
    if (uniDrcConfig.sampleRatePresent) {
        hDrcEnc->sampleRate = uniDrcConfig.sampleRate;
    }
    else {
        hDrcEnc->sampleRate = sampleRate;
    }
    
    hDrcEnc->delayMode = delayMode;
    hDrcEnc->domain = domain;
    hDrcEnc->deltaTminDefault = getDeltaTmin(hDrcEnc->sampleRate);
    
    /* We assume here that all sequences have the same deltaTmin info! This is not enforced by the standard. */
    /* TODO: support independent deltaTmin for each sequence */
#if AMD1_SYNTAX
    if ((uniDrcConfig.drcCoefficientsUniDrcCount > 0) && (uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[0].timeDeltaMinPresent == 1)) {
        hDrcEnc->deltaTmin = uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[0].deltaTmin;
    }
    else if ((uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0) && (uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[0].timeDeltaMinPresent == 1)) {
        hDrcEnc->deltaTmin = uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[0].deltaTmin;
    }
    else {
        hDrcEnc->deltaTmin = getDeltaTmin(hDrcEnc->sampleRate);
    }
#else /* !AMD1_SYNTAX */
    gainSetParams = uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[0];
    if (gainSetParams.timeDeltaMinPresent == 1) {
        hDrcEnc->deltaTmin = gainSetParams.deltaTmin;
    }
    else {
        hDrcEnc->deltaTmin = getDeltaTmin(hDrcEnc->sampleRate);
    }
#endif /* AMD1_SYNTAX */
    nGainValuesMax = hDrcEnc->drcFrameSize / hDrcEnc->deltaTmin;
    
    /* get baseChannelCount from uniDrcConfig */
    hDrcEnc->baseChannelCount = uniDrcConfig.channelLayout.baseChannelCount;
    
    /* keep copies of uniDrcConfig and loudnessInfoSet */
    hDrcEnc->uniDrcConfig = uniDrcConfig;
    hDrcEnc->loudnessInfoSet = loudnessInfoSet;
    
    /* allocate 2D input gain buffer [nSequences][3*hDrcEnc->drcFrameSize] */
    hDrcEnc->drcGainPerSampleWithPrevFrame = (float**)calloc(hDrcEnc->nSequences, sizeof(float*));
    if (!hDrcEnc->drcGainPerSampleWithPrevFrame) return -1;
    for (i=0; i<hDrcEnc->nSequences; i++) {
        hDrcEnc->drcGainPerSampleWithPrevFrame[i] = (float*)calloc(3*hDrcEnc->drcFrameSize, sizeof(float));
        if (!hDrcEnc->drcGainPerSampleWithPrevFrame[i]) return -1;
    }
    
    /* allocate and initialize drcGainSeqBuf */
    hDrcEnc->drcGainSeqBuf = (DrcGainSeqBuf*)calloc(hDrcEnc->nSequences, sizeof(DrcGainSeqBuf));
    if (!hDrcEnc->drcGainSeqBuf) return -1;
    
    for (i=0; i<hDrcEnc->nSequences; i++) {
        pDrcGroup = &(hDrcEnc->drcGainSeqBuf[i].drcGroup);
        pDrcGroup->drcGainQuant   = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroup->drcGainQuant)   return -1;
        pDrcGroup->gainCode       =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->gainCode)       return -1;
        pDrcGroup->gainCodeLength =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->gainCodeLength) return -1;
        pDrcGroup->slopeQuant     = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroup->slopeQuant)     return -1;
        pDrcGroup->slopeCodeIndex =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->slopeCodeIndex) return -1;
        pDrcGroup->tsGainQuant    =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->tsGainQuant)    return -1;
        pDrcGroup->timeDeltaQuant =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->timeDeltaQuant) return -1;
        
        pDrcGroup->drcGain        = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroup->drcGain)    return -1;
        pDrcGroup->slope          = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroup->slope)      return -1;
        pDrcGroup->tsGain         =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroup->tsGain)     return -1;
        
        pDrcGroupForOutput = &(hDrcEnc->drcGainSeqBuf[i].drcGroupForOutput);
#ifdef NODE_RESERVOIR
        pDrcGroupForOutput->drcGainQuant   = (float*)calloc(2*nGainValuesMax, sizeof(float));
#else
        pDrcGroupForOutput->drcGainQuant   = (float*)calloc(nGainValuesMax, sizeof(float));
#endif
        if (!pDrcGroupForOutput->drcGainQuant)      return -1;
        pDrcGroupForOutput->gainCode       =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->gainCode)          return -1;
        pDrcGroupForOutput->gainCodeLength =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->gainCodeLength)    return -1;
        pDrcGroupForOutput->slopeQuant     = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroupForOutput->slopeQuant)        return -1;
        pDrcGroupForOutput->slopeCodeIndex =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->slopeCodeIndex)    return -1;
#ifdef NODE_RESERVOIR
        pDrcGroupForOutput->tsGainQuant    =   (int*)calloc(2*nGainValuesMax, sizeof(int));
#else
        pDrcGroupForOutput->tsGainQuant    =   (int*)calloc(nGainValuesMax, sizeof(int));
#endif
        if (!pDrcGroupForOutput->tsGainQuant)       return -1;
        pDrcGroupForOutput->timeDeltaQuant =   (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->timeDeltaQuant)    return -1;
        
        pDrcGroupForOutput->timeDeltaCode      = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->timeDeltaCode)     return -1;
        pDrcGroupForOutput->timeDeltaCodeIndex = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->timeDeltaCodeIndex)return -1;
        pDrcGroupForOutput->timeDeltaCodeSize  = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->timeDeltaCodeSize) return -1;
        pDrcGroupForOutput->slopeCode          = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->slopeCode)         return -1;
        pDrcGroupForOutput->slopeCodeSize      = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->slopeCodeSize)     return -1;
#ifdef NODE_RESERVOIR
        pDrcGroupForOutput->drcGainQuantReservoir      = (float*)calloc(nGainValuesMax, sizeof(float));
        if (!pDrcGroupForOutput->drcGainQuantReservoir)     return -1;
        pDrcGroupForOutput->slopeCodeIndexBufReservoir = (int*)calloc((nGainValuesMax+2), sizeof(int));
        if (!pDrcGroupForOutput->slopeCodeIndexBufReservoir)return -1;
        pDrcGroupForOutput->tsGainQuantReservoir       = (int*)calloc(nGainValuesMax, sizeof(int));
        if (!pDrcGroupForOutput->tsGainQuantReservoir) return -1;
#endif
    }
    
    k=0;
#if AMD1_SYNTAX
    if (uniDrcConfig.drcCoefficientsUniDrcCount > 0) {
        for (s=0; s<uniDrcConfig.drcCoefficientsUniDrc[0].gainSetCount; s++)
        {
            for (b=0; b<uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[s].bandCount; b++)
            {
                hDrcEnc->drcGainSeqBuf[k].drcGroup.nGainValues = 1;
                hDrcEnc->drcGainSeqBuf[k].gainSetParams = uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[s];
                k++;
            }
        }
    }
    if (uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count > 0)
    {
        for (i=0; i<hDrcEnc->nSequences; i++)
        {
            int paramsFound = 0;
            
            for (s=0; s<uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetCount; s++)
            {
                for (b=0; b<uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[s].bandCount; b++)
                {
                    if (i == uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[s].gainParams[b].gainSequenceIndex)
                    {
                        hDrcEnc->drcGainSeqBuf[i].drcGroup.nGainValues = 1;
                        hDrcEnc->drcGainSeqBuf[i].gainSetParams = uniDrcConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[0].gainSetParams[s];
                        paramsFound = 1;
                    }
                    if (paramsFound == 1) break;
                }
                if (paramsFound == 1) break;
            }
        }
    }
#else /* !AMD1_SYNTAX */
    for (s=0; s<uniDrcConfig.drcCoefficientsUniDrc[0].gainSetCount; s++)
    {
        for (b=0; b<uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[s].bandCount; b++)
        {
            hDrcEnc->drcGainSeqBuf[k].drcGroup.nGainValues = 1;
            hDrcEnc->drcGainSeqBuf[k].gainSetParams = uniDrcConfig.drcCoefficientsUniDrc[0].gainSetParams[s];
            k++;
        }
    }
#endif /* AMD1_SYNTAX */
    /* allocate and generate tables for coding of timeDelta */
    hDrcEnc->deltaTimeCodeTable = (DeltaTimeCodeTableEntry*)calloc(2*nGainValuesMax+1, sizeof(DeltaTimeCodeTableEntry));
    hDrcEnc->deltaTimeQuantTable = (int*)calloc(nGainValuesMax, sizeof(int));
    generateDeltaTimeCodeTable (nGainValuesMax,  hDrcEnc->deltaTimeCodeTable);
    for (i=0; i<nGainValuesMax; i++) {
        hDrcEnc->deltaTimeQuantTable[i] = hDrcEnc->deltaTmin * (i+1);
    }
    
    if (checkDrcParams(hDrcEnc)) return -1;
    
    return 0;
}

int closeDrcToolEncoder(HANDLE_DRC_ENCODER* phDrcEnc) {
    int i;
    DrcEncParams* hDrcEnc = *phDrcEnc;
    DrcGroup* pDrcGroup;
    DrcGroupForOutput* pDrcGroupForOutput;

    free(hDrcEnc->deltaTimeQuantTable);
    free(hDrcEnc->deltaTimeCodeTable);

    for (i=0; i<hDrcEnc->nSequences; i++) {
        free(hDrcEnc->drcGainPerSampleWithPrevFrame[i]);
    }
    free(hDrcEnc->drcGainPerSampleWithPrevFrame);

    for (i=0; i<hDrcEnc->nSequences; i++) {
        pDrcGroup = &(hDrcEnc->drcGainSeqBuf[i].drcGroup);
        free(pDrcGroup->drcGainQuant);
        free(pDrcGroup->gainCode);
        free(pDrcGroup->gainCodeLength);
        free(pDrcGroup->slopeQuant);
        free(pDrcGroup->slopeCodeIndex);
        free(pDrcGroup->tsGainQuant);
        free(pDrcGroup->timeDeltaQuant);

        free(pDrcGroup->drcGain);
        free(pDrcGroup->slope);
        free(pDrcGroup->tsGain);

        pDrcGroupForOutput = &(hDrcEnc->drcGainSeqBuf[i].drcGroupForOutput);
        free(pDrcGroupForOutput->drcGainQuant);
        free(pDrcGroupForOutput->gainCode);
        free(pDrcGroupForOutput->gainCodeLength);
        free(pDrcGroupForOutput->slopeQuant);
        free(pDrcGroupForOutput->slopeCodeIndex);
        free(pDrcGroupForOutput->tsGainQuant);
        free(pDrcGroupForOutput->timeDeltaQuant);

        free(pDrcGroupForOutput->timeDeltaCode);
        free(pDrcGroupForOutput->timeDeltaCodeIndex);
        free(pDrcGroupForOutput->timeDeltaCodeSize);
        free(pDrcGroupForOutput->slopeCode);
        free(pDrcGroupForOutput->slopeCodeSize);
#ifdef NODE_RESERVOIR
        free(pDrcGroupForOutput->drcGainQuantReservoir);
        free(pDrcGroupForOutput->slopeCodeIndexBufReservoir);
        free(pDrcGroupForOutput->tsGainQuantReservoir);
#endif
    }
    free(hDrcEnc->drcGainSeqBuf);


    free(*phDrcEnc);
    *phDrcEnc = NULL;
    return 0;
}

int
getNSequences(HANDLE_DRC_ENCODER hDrcEnc)
{
    return hDrcEnc->nSequences;
}
 
int
encodeUniDrcGain(HANDLE_DRC_ENCODER hDrcEnc,
                 float** gainBuffer)
{ 
    int k;

    for(k=0; k<hDrcEnc->nSequences; k++) {
        quantizeAndEncodeDrcGain(hDrcEnc, gainBuffer[k], hDrcEnc->drcGainPerSampleWithPrevFrame[k], hDrcEnc->deltaTimeCodeTable, &(hDrcEnc->drcGainSeqBuf[k]));
    }
    return 0;
}

int
getDeltaTmin (const int sampleRate)
{
    int lowerBound = (int) (0.5f + 0.0005f * sampleRate);
    int result = 1;
    if (sampleRate < 1000)
    {
        fprintf(stderr, "Error: audio sample rates below 1000 Hz are not supported: %d.\n", sampleRate);
        return(UNEXPECTED_ERROR);
    }
    while (result <= lowerBound) result = result << 1;
    return result;
}

