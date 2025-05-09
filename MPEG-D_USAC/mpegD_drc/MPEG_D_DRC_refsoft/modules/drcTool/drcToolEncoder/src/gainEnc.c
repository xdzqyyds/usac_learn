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
#include <math.h>
#include "drcToolEncoder.h"
#include "gainEnc.h"
#include "tables.h"
#include "mux.h"

#define MAX_TIME_DEVIATION_FACTOR       0.25f
#define SLOPE_CHANGE_THR                3.0f
#define SLOPE_QUANT_THR                 8.0f

#define GAIN_QUANT_STEP_SIZE            0.125f
#define GAIN_QUANT_STEP_SIZE_INV        8.0f

#define MAX_DRC_GAIN_DELTA_BEFORE_QUANT (1.0f + 0.5f * GAIN_QUANT_STEP_SIZE)

#define SCALE_APPROXIMATE_DB            0.99657842f /* factor for converting dB to approximate dB: log2(10)*6/20 */

/*********************************************************************************************/

float
limitDrcGain(const int gainCodingProfile, const float gain)
{
    float drcGainLimited;
    switch (gainCodingProfile) {
        case GAIN_CODING_PROFILE_REGULAR:
            drcGainLimited = max (DRC_GAIN_CODING_PROFILE0_MIN, min (DRC_GAIN_CODING_PROFILE0_MAX, gain));
            break;
        case GAIN_CODING_PROFILE_FADING:
            drcGainLimited = max (DRC_GAIN_CODING_PROFILE1_MIN, min (DRC_GAIN_CODING_PROFILE1_MAX, gain));
            break;
        case GAIN_CODING_PROFILE_CLIPPING:
            drcGainLimited = max (DRC_GAIN_CODING_PROFILE2_MIN, min (DRC_GAIN_CODING_PROFILE2_MAX, gain));
            break;
        case GAIN_CODING_PROFILE_CONSTANT:
            drcGainLimited = gain;
            break;
        default:
            drcGainLimited = gain;
            break;
    }
    return (drcGainLimited);
}

void
getQuantizedDeltaDrcGain(const int gainCodingProfile,
                         const float deltaGain,
                         float *deltaGainQuant,
                         int* nBits,
                         int* code)
{
    int nEntries;
    int n;
    float diff;
    float posDiffMin = 1000.0f;
    float negDiffMin = -1000.0f;
    int posDiffMinIdx = 0;
    int negDiffMinIdx = 0;
    int optIndex;
    
    DeltaGainCodeEntry const* deltaGainCodeTable;
    getDeltaGainCodeTable(gainCodingProfile, &deltaGainCodeTable, &nEntries);
    
    for (n=0; n<nEntries; n++)
    {
        diff = deltaGain - deltaGainCodeTable[n].value;
        if (diff > 0.0f)
        {
            if (diff < posDiffMin)
            {
                posDiffMin = diff;
                posDiffMinIdx = n;
            }
        }
        else
        {
            if (diff > negDiffMin)
            {
                negDiffMin = diff;
                negDiffMinIdx = n;
            }
        }
    }
    if (posDiffMin < -negDiffMin)
    {
        optIndex = posDiffMinIdx;
    }
    else
    {
        optIndex = negDiffMinIdx;
    }
    *deltaGainQuant = deltaGainCodeTable[optIndex].value;
    *nBits = deltaGainCodeTable[optIndex].size;
    *code = deltaGainCodeTable[optIndex].code;
}


/*********************************************************************************************/
/* 
 check for overshoots:
 1) evaluating the second derivative at the two nodes. The curvature at one node must point into the direction towards the other node. 
 2) evaluate the minimum and maximum of the spline segment 
 */

void
checkOvershoot(const int tGainStep,
               const float gain0,
               const float gain1,
               const float slope0,
               const float slope1,
               const int timeDeltaMin,
               bool *overshootLeft,
               bool *overshootRight)
{

    float slopeNorm = 1.0f / (float)timeDeltaMin;

    float slope0norm;
    float slope1norm;
    float gainLeft, gainRight;
    int tConnect;
    
    float margin = 0.2f;  /* the maximum overshoot we tolerate*/
    float stepInv2 = 2.0f / tGainStep;
    float tmp;
    float curveLeft, curveRight;

    *overshootLeft = FALSE;
    *overshootRight = FALSE;

#if INTERPOLATE_IN_DB_DOMAIN
    gainLeft = gain0;
    gainRight = gain1;
    
    slope0norm = slope0 * slopeNorm;
    slope1norm = slope1 * slopeNorm;
#else
    /* interpolate in linear domain */
    
    gainLeft = (float)pow((double)10.0, (double)(0.05f * gain0));
    gainRight = (float)pow((double)10.0, (double)(0.05f * gain1));
    
    slope0norm = slope0 * slopeNorm * SLOPE_FACTOR_DB_TO_LINEAR * gainLeft;
    slope1norm = slope1 * slopeNorm * SLOPE_FACTOR_DB_TO_LINEAR * gainRight;
#endif

    if ((float)fabs((double)slope0norm) > (float)fabs((double)slope1norm))
    {
        /* Use quadratic interpolation on the left side and linear interpolation on the right side */
        tConnect = (int) (0.5f + 2.0f * (gainRight - gainLeft - slope1norm * tGainStep) / (slope0norm - slope1norm));

        if ((tConnect >= 0) && (tConnect < tGainStep))
        {
            return;
        }
    }
    else if ((float)fabs((double)slope0norm) < (float)fabs((double)slope1norm))
    {
        /* Use linear interpolation on the left side and quadratic interpolation on the right side */
        
        tConnect = (int) (0.5f + 2.0f * (gainLeft - gainRight + slope0norm * tGainStep) / (slope0norm - slope1norm));
        tConnect = tGainStep - tConnect;
        if ((tConnect >= 0) && (tConnect < tGainStep))
        {
            return;
        }
    }

    tmp = 1.5f * stepInv2 * (gainRight - gainLeft) - slope1norm - slope0norm;
    curveLeft =  stepInv2 * (tmp - slope0norm);
    curveRight = stepInv2 * (slope1norm - tmp);

    /* left side*/
    tmp = - slope0norm * tGainStep - gainLeft + gainRight;
    if (curveLeft < 0.0f)
    {
        if (tmp - margin > 0.0f) *overshootLeft = TRUE;
    }
    else
    {
        if (tmp + margin < 0.0f) *overshootLeft = TRUE;
    }
    tmp = slope1norm * tGainStep - gainRight + gainLeft;
    if (curveRight < 0.0f)
    {
        if (tmp - margin > 0.0f) *overshootRight = TRUE;
    }
    else
    {
        if (tmp + margin < 0.0f) *overshootRight = TRUE;        
    }
    
    
    if ((!*overshootLeft) && (!*overshootRight))
    {
        
        /* we can probably check a few more conditions before we have to execute this part */
        
        
        /* Check how far the gain curve exceeds the range between gainLeft and gainRight */
        /* 1) Compute the two points of the spline that have a steepness of zero (this is where the extreme values are) */
        /* 2) Compute the gain at these points and compare with the given range */
         
        float tGainStepInv = 1.0f / (float)tGainStep;
        float tGainStepInv2 = tGainStepInv * tGainStepInv;
        
        float k1 = (gainRight - gainLeft) * tGainStepInv2;
        float k2 = slope1norm + slope0norm;
        
        float a = tGainStepInv * (tGainStepInv * k2 - 2.0f * k1);
        float b = 3.0f * k1 - tGainStepInv * (k2 + slope0norm);
        float c = slope0norm;
        float d = gainLeft;
        
        tmp = b*b - 3.0f * a * c;
        if ((tmp < 0.0f) || (a == 0.0f))
        {
#if DEBUG_UNIDRC_ENC
            printf("Error in checkOvershoot() argument of sqrt must be non-negativ and denominator must be nonzero: %e  %e\n", tmp, a);
#endif
        }
        else
        {
            float maxVal = max(gainLeft, gainRight) + margin;
            float minVal = min(gainLeft, gainRight) - margin;
            float gExtreme;
            float tmp2;
            float tExtreme;
            
            tmp = (float)sqrt((double)tmp);
            tmp2 = (1.0f / (3.0f * a));
            tExtreme = tmp2 * (-b + tmp);
            
            if ((tExtreme > 0.0f) && (tExtreme < tGainStep))
            {
                gExtreme = (((a * tExtreme + b ) * tExtreme + c ) * tExtreme ) + d;
                
                if ((gExtreme > maxVal) || (gExtreme < minVal))
                {
                    *overshootLeft = TRUE;  /* doesn't matter if left or right*/
                }
            }
            
            tExtreme = tmp2 * (-b - tmp);

            if ((tExtreme > 0.0f) && (tExtreme < tGainStep))
            {
                gExtreme = (((a * tExtreme + b ) * tExtreme + c ) * tExtreme ) + d;

                if ((gExtreme > maxVal) || (gExtreme < minVal))
                {
                    *overshootLeft = TRUE;
                }
            }
        }
    }
}

/*********************************************************************************************/
void
quantizeSlope (const float slope,
               float* slopeQuant,
               int* slopeCodeIndex)
{
    int i=0;
    const SlopeCodeTableEntry* slopeCodeTable = getSlopeCodeTableByValue();

    
    while ((i<14) && (slope > slopeCodeTable[i].value)) i++;
    if (i>0)
    {
        if (slopeCodeTable[i].value - slope > slope - slopeCodeTable[i-1].value) i--;
    }
    *slopeQuant = slopeCodeTable[i].value;
    *slopeCodeIndex = slopeCodeTable[i].index;
    

}


/*********************************************************************************************/
void
getPreliminaryNodes(const DrcEncParams* drcParams,
                    const float* drcGainPerSample,
                    float* drcGainPerSampleWithPrevFrame,
                    DrcGroup* drcGroup,
                    const int fullFrame)
{
    
    int n, k, t;
    int drcFrameSize = drcParams->drcFrameSize;
    int timeDeltaMin = drcParams->deltaTmin;
    int nValues = drcFrameSize / timeDeltaMin;
    int offset = timeDeltaMin / 2;
    float slope[AUDIO_CODEC_FRAME_SIZE_MAX];
    float* gainAtNode = drcGroup->drcGain;
    float* slopeAtNode = drcGroup->slope;
    int* timeAtNode = drcGroup->tsGain;
    int nGainValues;
    
    int offs = drcFrameSize;
    float* gPtr = drcGainPerSampleWithPrevFrame + offs;
    
    float f0 = 0.9f;
    float f1 = 1.0f - f0;
    
    int step = timeDeltaMin;
    
    float quantErrorPrev = -1.0f;  /* Negative error because this node cannot be (easily) removed */
    float gainQuantPrev;
    float gain;
    float gainQuant;
    float quantError;
    
    int nLeft;
    int nRight;
    
    /* copy the gain vector into the extended buffer , the buffer holds three frames */
    memcpy(drcGainPerSampleWithPrevFrame, &(drcGainPerSampleWithPrevFrame[drcFrameSize]), drcFrameSize * sizeof(float));
    memcpy(&(drcGainPerSampleWithPrevFrame[drcFrameSize]), drcGainPerSample, drcFrameSize * sizeof(float));
    /* convert from dB to approximate dB */
    for (n=0; n<drcFrameSize; n++) {
        gPtr[n] *= SCALE_APPROXIMATE_DB;
    }

    for (n=0; n<drcFrameSize; n++)
    {
        gPtr[n] = f0 * gPtr[n-1] + f1 * gPtr[n];
    }

    /* quantize prev gain*/
    if(drcGroup->gainPrevNode >= 0.f)
        gainQuantPrev = GAIN_QUANT_STEP_SIZE * ((int) (0.5f + GAIN_QUANT_STEP_SIZE_INV * drcGroup->gainPrevNode));
    else
        gainQuantPrev = GAIN_QUANT_STEP_SIZE * ((int) (-0.5f + GAIN_QUANT_STEP_SIZE_INV * drcGroup->gainPrevNode));
    
    k=-1;
    for (n=1; n<nValues+1; n++)

    {
        float slopePrev;
        float slopeNext;
        gain = gPtr[(n) * timeDeltaMin -1];
        
        /* quantize current gain */
        if(gain >= 0.f)
            gainQuant = GAIN_QUANT_STEP_SIZE * ((int) (0.5f + GAIN_QUANT_STEP_SIZE_INV * gain));  /* quantize with the minimum step size */
        else
            gainQuant = GAIN_QUANT_STEP_SIZE * ((int) (-0.5f + GAIN_QUANT_STEP_SIZE_INV * gain));  /* quantize with the minimum step size */

        quantError = (float)fabs((double)(gain - gainQuant));

        slopePrev = (gain - gPtr[(n-1) * timeDeltaMin -1]);
        if (n == nValues)
          slopeNext = 0.2f; /* force shifting the node to the end of the frame */
        else
          slopeNext = (gPtr[(n+1) * timeDeltaMin -1] - gain);
        
        if (gainQuantPrev == gainQuant)
        {            
            /* place a node at the transition point from a steep slope to flat range */
            if ((float)fabs((double)slopeNext) > 0.1f)
            {
                if (k<0) {k=0;}
                timeAtNode[k] = n * timeDeltaMin -1;
            }
            else
            {
                /* if there is a series of identical quantized values, choose the one with the smallest quantization error */
                if (quantErrorPrev > quantError)
                {
                    if (k<0) {k=0;}
                    quantErrorPrev = quantError;
                    timeAtNode[k] = n * timeDeltaMin -1;
                }
            }
        }
        else
        {
            /* place (at least) one node whenever the quantized value changes */
            k++;
            quantErrorPrev = quantError;
            gainQuantPrev = gainQuant;
            timeAtNode[k] = n * timeDeltaMin -1;
            if ((float)fabs((double)slopePrev) > 0.1f)
            {
                /* place a node at the transition from a flat area to a steep slope */
                gainQuantPrev = 1000.0f;  /* pretend that this value is different so that it will not be changed */
            }
        }
    }
    
    /* insert a node at the last sample in fullFrame mode */
    if (fullFrame == 1)
    {
        if (timeAtNode[k] != drcFrameSize - 1)
        {
            k++;
            timeAtNode[k] = drcFrameSize - 1;
        }
    }
    
    nGainValues = k+1;
    
    if (nGainValues<=0)
    {
        int idx;
        /* put one node in the middle of the DRC frame */
        if (k<0) {k=0;}
        n = nValues / 2;
        idx = offset + n*timeDeltaMin -1;
        slope[n] = drcGainPerSample[idx + timeDeltaMin] - drcGainPerSample[idx];
        t = (n+1) * timeDeltaMin -1;
        gainAtNode[k] = drcGainPerSample[t];
        slopeAtNode[k] = slope[n];
        timeAtNode[k] = t;
        nGainValues++;
    }

    for (k=0; k<nGainValues; k++)
    {
        nLeft = max(0, timeAtNode[k] - step);
        nRight = nLeft + step;
        slopeAtNode[k] = gPtr[nRight] - gPtr[nLeft];
        gainAtNode[k] = gPtr[timeAtNode[k]];
    }
     
    drcGroup->nGainValues = nGainValues;

}

/*********************************************************************************************/
void
advanceNodes(DrcEncParams* drcParams,
             DrcGainSeqBuf* drcGainSeqBuf)
{
    int k;
    DrcGroup* drcGroup = &(drcGainSeqBuf->drcGroup);
    DrcGroupForOutput* drcGroupForOutput = &(drcGainSeqBuf->drcGroupForOutput);
    
#ifndef NODE_RESERVOIR
    if (drcGroupForOutput->nGainValues > 0)
    {
        drcGroupForOutput->drcGainQuantPrev = drcGroupForOutput->drcGainQuant[drcGroupForOutput->nGainValues - 1];
        drcGroupForOutput->timeQuantPrev = drcGroupForOutput->tsGainQuant[drcGroupForOutput->nGainValues - 1] - drcParams->drcFrameSize;
        drcGroupForOutput->slopeCodeIndexPrev = drcGroupForOutput->slopeCodeIndex[drcGroupForOutput->nGainValues - 1];
    }
#endif
    for (k=0; k<drcGroup->nGainValues; k++)
    {
        drcGroupForOutput->drcGainQuant[k] = drcGroup->drcGainQuant[k];
        drcGroupForOutput->gainCode[k] = drcGroup->gainCode[k];
        drcGroupForOutput->gainCodeLength[k] = drcGroup->gainCodeLength[k];
        drcGroupForOutput->slopeQuant[k] = drcGroup->slopeQuant[k];
        drcGroupForOutput->slopeCodeIndex[k] = drcGroup->slopeCodeIndex[k];
        drcGroupForOutput->tsGainQuant[k] = drcGroup->tsGainQuant[k];
        drcGroupForOutput->timeDeltaQuant[k] = drcGroup->timeDeltaQuant[k];
    }
    drcGroupForOutput->nGainValues = drcGroup->nGainValues;
}



/*********************************************************************************************/
void
postProcessNodes (DrcEncParams* drcParams,
                  DeltaTimeCodeTableEntry* deltaTimeCodeTable,
                  DrcGainSeqBuf* drcGainSeqBuf)
{
    int k, n;
    float gainBuf[N_UNIDRC_GAIN_MAX+2];
    int timeBuf[N_UNIDRC_GAIN_MAX+2];
    int slopeCodeIndexBuf[N_UNIDRC_GAIN_MAX+2];
    bool remove[N_UNIDRC_GAIN_MAX+2];
    
    int timeMandatoryNode;
    int timeDeltaMin = drcParams->deltaTmin;
    int cod_slope_zero = 0x7;

#ifdef NODE_RESERVOIR
    int thr=0, nGainValuesCur=0, nGainValuesReservoirPrev=0;
#endif

    if (drcGainSeqBuf->gainSetParams.fullFrame == 1)
    {
        timeMandatoryNode = drcParams->drcFrameSize-1;
    }
    else
    {
        timeMandatoryNode = 99999999;  /* means no mandatory node position */
    }
    
    {
        DrcGroupForOutput* drcGroupForOutput = &(drcGainSeqBuf->drcGroupForOutput);
        int nGainValues = drcGroupForOutput->nGainValues;
        bool slopeChanged = TRUE;
        bool repeatCheck = TRUE;
        bool overshootRight, overshootLeft;
        int timePrev = -1;
        float drcGainQuantPrev = drcGroupForOutput->drcGainQuantPrev;
        float deltaGainQuant;
        float gainValueQuant;
        
        gainBuf[0] = drcGroupForOutput->drcGainQuantPrev;
        timeBuf[0] = drcGroupForOutput->timeQuantPrev;
        for (k=0; k<nGainValues; k++)
        {
            gainBuf[k+1] = drcGroupForOutput->drcGainQuant[k];
            timeBuf[k+1] = drcGroupForOutput->tsGainQuant[k];
        }
        gainBuf[k+1] = drcGroupForOutput->drcGainQuantNext;
        timeBuf[k+1] = drcGroupForOutput->timeQuantNext;
        
        
        /* Remove the middle node if three subsequent nodes are identical */
        if (nGainValues > 1)
        {
            int nRemoved = 0;
            int idxLeft = 0;
            int idxRight = 2;
            
            for (k=0; k<=nGainValues+1; k++)
            {
                remove[k] = FALSE;
            }

            while (idxRight <= nGainValues+1)
            {
                if ((gainBuf[idxLeft] == gainBuf[idxRight-1]) && (gainBuf[idxRight-1] == gainBuf[idxRight]) && (nGainValues - nRemoved > 1) && (timeBuf[idxRight-1] != timeMandatoryNode))
                {
                    remove[idxRight-1] = TRUE;
                    nRemoved++;
                    idxRight++;
                }
                else
                {
                    idxLeft = idxRight-1;
                    idxRight++;
                }
            }

            n=1;
            for (k=1; k<=nGainValues+1; k++)
            {
                if (!remove[k])
                {
                    gainBuf[n] = gainBuf[k];
                    timeBuf[n] = timeBuf[k];
                    n++;
                }
            }
        
            n=0;
            for (k=0; k<nGainValues; k++)
            {
                if (!remove[k+1])
                {
                    drcGroupForOutput->tsGainQuant[n] = drcGroupForOutput->tsGainQuant[k];
                    drcGroupForOutput->slopeQuant[n] = drcGroupForOutput->slopeQuant[k];
                    drcGroupForOutput->slopeCodeIndex[n] = drcGroupForOutput->slopeCodeIndex[k];
                    drcGroupForOutput->drcGainQuant[n] = drcGroupForOutput->drcGainQuant[k];
                    drcGroupForOutput->gainCode[n] = drcGroupForOutput->gainCode[k];
                    drcGroupForOutput->gainCodeLength[n] = drcGroupForOutput->gainCodeLength[k];
                    drcGroupForOutput->timeDeltaQuant[n] = drcGroupForOutput->timeDeltaQuant[k];
                    n++;
                }
            }
            nGainValues = n;
        }
        
        /* Remove one middle node of four nodes if the two middle nodes are identical and the four nodes are on an ascending or descending slope*/
        if (nGainValues > 2)
        {
            int nRemoved = 0;
            
            int idx0 = 0;
            int idx1 = 1;
            int idx2 = 2;
            int idx3 = 3;
            
            bool moveOn = FALSE;
            
            for (k=0; k<=nGainValues+1; k++)
            {
                remove[k] = FALSE;
            }

            while (idx3 < nGainValues+1)
            {
                if (moveOn)
                {
                    idx0 = idx1;
                    idx1 = idx2;
                    idx2 = idx3;
                    idx3++;
                    moveOn = FALSE;
                }
                if (gainBuf[idx1] == gainBuf[idx2])
                {
                    float deltaLeft = gainBuf[idx1] - gainBuf[idx0];
                    float deltaRight = gainBuf[idx3] - gainBuf[idx2];
                    
                    if (((float)fabs((double)deltaLeft) < 0.26f) || ((float)fabs((double)deltaRight) < 0.26f))
                    {
                        if ((deltaLeft < 0.0f) && (deltaRight < 0.0f) && (timeBuf[idx2] != timeMandatoryNode))
                        {
                            /* We have a downward slope */
                            remove[idx2] = TRUE;
                            idx2=idx3;
                            idx3++;
                            nRemoved++;
                        }
                        else if ((deltaLeft > 0.0f) && (deltaRight > 0.0f) && (timeBuf[idx1] != timeMandatoryNode))
                        {
                            /* We have an upward slope */
                            remove[idx1] = TRUE;
                            drcGroupForOutput->gainCode[idx2-1] = drcGroupForOutput->gainCode[idx1-1];
                            drcGroupForOutput->gainCodeLength[idx2-1] = drcGroupForOutput->gainCodeLength[idx1-1];
                            idx1=idx2;
                            idx2=idx3;
                            idx3++;
                            nRemoved++;
                        }
                        else
                        {
                            moveOn = TRUE;
                        }
                    }
                    else
                    {
                        moveOn = TRUE;
                    }
                }
                else
                {
                    moveOn = TRUE;
                }
                
            }
            
            n=1;
            for (k=1; k<=nGainValues+1; k++)
            {
                if (!remove[k])
                {
                    gainBuf[n] = gainBuf[k];
                    timeBuf[n] = timeBuf[k];
                    n++;
                }
            }
            
            n=0;
            for (k=0; k<nGainValues; k++)
            {
                if (!remove[k+1])
                {
                    drcGroupForOutput->tsGainQuant[n] = drcGroupForOutput->tsGainQuant[k];
                    drcGroupForOutput->slopeQuant[n] = drcGroupForOutput->slopeQuant[k];
                    drcGroupForOutput->slopeCodeIndex[n] = drcGroupForOutput->slopeCodeIndex[k];
                    drcGroupForOutput->drcGainQuant[n] = drcGroupForOutput->drcGainQuant[k];
                    drcGroupForOutput->gainCode[n] = drcGroupForOutput->gainCode[k];
                    drcGroupForOutput->gainCodeLength[n] = drcGroupForOutput->gainCodeLength[k];
                    drcGroupForOutput->timeDeltaQuant[n] = drcGroupForOutput->timeDeltaQuant[k];
                    n++;
                }
            }
            nGainValues = n;
        }
        
        /* Force a zero slope for maxima and minima */
        for (k=1; k<=nGainValues; k++)
        {
            if ((gainBuf[k-1] < gainBuf[k]) && (gainBuf[k] > gainBuf[k+1]))
            {
                drcGroupForOutput->slopeQuant[k-1] = 0.0f;
                drcGroupForOutput->slopeCodeIndex[k-1] = cod_slope_zero;
            }
            if ((gainBuf[k-1] > gainBuf[k]) && (gainBuf[k] < gainBuf[k+1]))
            {
                drcGroupForOutput->slopeQuant[k-1] = 0.0f;
                drcGroupForOutput->slopeCodeIndex[k-1] = cod_slope_zero;
            }
        }
        
        /* Force zero slope steepness if neighboring quantized gain values are the same*/
        if (gainBuf[0] == gainBuf[1])
        {
            drcGroupForOutput->slopeQuant[0] = 0.0f;
            drcGroupForOutput->slopeCodeIndex[0] = cod_slope_zero;
        }
        for (k=0; k<nGainValues-1; k++)
        {
            if (gainBuf[k+1] == gainBuf[k+2])
            {
                drcGroupForOutput->slopeQuant[k] = 0.0f;
                drcGroupForOutput->slopeCodeIndex[k] = cod_slope_zero;
                drcGroupForOutput->slopeQuant[k+1] = 0.0f;
                drcGroupForOutput->slopeCodeIndex[k+1] = cod_slope_zero;
            }
        }
        if (gainBuf[k+1] == gainBuf[k+2])
        {
            drcGroupForOutput->slopeQuant[k] = 0.0f;
            drcGroupForOutput->slopeCodeIndex[k] = cod_slope_zero;
        }
    
        
        slopeCodeIndexBuf[0] = drcGroupForOutput->slopeCodeIndexPrev;
        for (k=0; k<nGainValues; k++)
        {
            slopeCodeIndexBuf[k+1] = drcGroupForOutput->slopeCodeIndex[k];
        }
        slopeCodeIndexBuf[k+1] = drcGroupForOutput->slopeCodeIndexNext;    
        
        /* Remove nodes on slopes if the slope is almost constant */
        
        for (k=0; k<=nGainValues+1; k++)
        {
            remove[k] = FALSE;
        }

        if (nGainValues > 1)
        {
            int nRemoved = 0;
            int left = 0;
            int mid = 1;
            int right = 2;
            
            float slopeOfNodesLeft;
            float slopeOfNodesRight;
            while ((right <= nGainValues + 1) && (nGainValues - nRemoved > 1))
            {
                if (((timeBuf[mid] - timeBuf[left]) > 0) &&  /* needed?*/
                    (float)fabs((double)(gainBuf[left] - gainBuf[right])) < MAX_DRC_GAIN_DELTA_BEFORE_QUANT)
                {
                    slopeOfNodesLeft =  (gainBuf[mid] - gainBuf[left]) / (timeBuf[mid] - timeBuf[left]);
                    slopeOfNodesRight = (gainBuf[right] - gainBuf[mid]) / (timeBuf[right] - timeBuf[mid]);
                    if (slopeOfNodesLeft < 0.0f)
                    {
                        if ((-slopeOfNodesLeft < -slopeOfNodesRight * SLOPE_CHANGE_THR) && (-slopeOfNodesLeft * SLOPE_CHANGE_THR > -slopeOfNodesRight))
                        {
                            float slopeAverage = - 0.5f * timeDeltaMin * (slopeOfNodesLeft + slopeOfNodesRight);
                            float thrLo = slopeAverage / SLOPE_QUANT_THR;
                            float thrHi = slopeAverage * SLOPE_QUANT_THR;
                            float slope0 = -decodeSlopeIndex(slopeCodeIndexBuf[left]);
                            float slope1 = -decodeSlopeIndex(slopeCodeIndexBuf[mid]);
                            float slope2 = -decodeSlopeIndex(slopeCodeIndexBuf[right]);
                            if (((slope0 < thrHi) && (slope0 > thrLo)) &&
                                ((slope1 < thrHi) && (slope1 > thrLo)) &&
                                ((slope2 < thrHi) && (slope2 > thrLo)) &&
                                (timeBuf[mid] != timeMandatoryNode))
                            {
                                remove[mid] = TRUE;
                                nRemoved++;
                                mid = right;
                                right++;
                            }
                            else
                            {
                                left = mid;
                                mid = right;
                                right++;
                            }
                        }
                        else
                        {
                            left = mid;
                            mid = right;
                            right++;
                        }
                        
                    }
                    else
                    {
                        
                        if ((slopeOfNodesLeft < slopeOfNodesRight * SLOPE_CHANGE_THR) && (slopeOfNodesLeft * SLOPE_CHANGE_THR > slopeOfNodesRight))
                        {
                            float slopeAverage = 0.5f * timeDeltaMin * (slopeOfNodesLeft + slopeOfNodesRight);
                            float thrLo = slopeAverage / SLOPE_QUANT_THR;
                            float thrHi = slopeAverage * SLOPE_QUANT_THR;
                            float slope0 = decodeSlopeIndex(slopeCodeIndexBuf[left]);
                            float slope1 = decodeSlopeIndex(slopeCodeIndexBuf[mid]);
                            float slope2 = decodeSlopeIndex(slopeCodeIndexBuf[right]);
                            if (((slope0 < thrHi) && (slope0 > thrLo)) &&
                                ((slope1 < thrHi) && (slope1 > thrLo)) &&
                                ((slope2 < thrHi) && (slope2 > thrLo)) &&
                                (timeBuf[mid] != timeMandatoryNode))
                            {
                                remove[mid] = TRUE;
                                nRemoved++;
                                mid = right;
                                right++;
                            }
                            else
                            {
                                left = mid;
                                mid = right;
                                right++;
                            }
                        }
                        else
                        {
                            left = mid;
                            mid = right;
                            right++;
                        }
                    }
                }
                else
                {
                    left = mid;
                    mid = right;
                    right++;
                }
            }
            
            
            n=1;
            for (k=1; k<=nGainValues+1; k++)
            {
                if (!remove[k])
                {
                    gainBuf[n] = gainBuf[k];
                    timeBuf[n] = timeBuf[k];
                    slopeCodeIndexBuf[n] = slopeCodeIndexBuf[k];
                    n++;
                }
            }
            
            n=0;
            for (k=0; k<nGainValues; k++)
            {
                if (!remove[k+1])
                {
                    drcGroupForOutput->tsGainQuant[n] = drcGroupForOutput->tsGainQuant[k];
                    drcGroupForOutput->slopeQuant[n] = drcGroupForOutput->slopeQuant[k];
                    drcGroupForOutput->slopeCodeIndex[n] = drcGroupForOutput->slopeCodeIndex[k];
                    drcGroupForOutput->drcGainQuant[n] = drcGroupForOutput->drcGainQuant[k];
                    drcGroupForOutput->gainCode[n] = drcGroupForOutput->gainCode[k];
                    drcGroupForOutput->gainCodeLength[n] = drcGroupForOutput->gainCodeLength[k];
                    drcGroupForOutput->timeDeltaQuant[n] = drcGroupForOutput->timeDeltaQuant[k];
                    n++;
                }
            }
            nGainValues = n;
        }
        
        drcGroupForOutput->nGainValues = nGainValues;
  
        
        /* Avoid overshoots: make steep slopes shallower if there is an overshoot */
        k = 0;
        while (repeatCheck)
        {
            repeatCheck = FALSE;
        
            while (k<nGainValues)
            {
                if (!slopeChanged)
                {
                    k++;
                }
                else
                {
                    slopeChanged = FALSE;
                }
                if ((slopeCodeIndexBuf[k] != cod_slope_zero) || (slopeCodeIndexBuf[k+1] != cod_slope_zero))
                {
                    checkOvershoot(timeBuf[k+1] - timeBuf[k],       /* tGainStep,*/
                                   gainBuf[k],                      /* gain0,*/
                                   gainBuf[k+1],                    /* gain1,*/
                                   decodeSlopeIndex(slopeCodeIndexBuf[k]),    /* slope0,*/
                                   decodeSlopeIndex(slopeCodeIndexBuf[k+1]),  /* slope1,*/
                                   timeDeltaMin,
                                   &overshootLeft,
                                   &overshootRight);
                    if (overshootRight || overshootLeft)
                    {
                        if ((k==0) || (decodeSlopeIndexMagnitude(slopeCodeIndexBuf[k]) <  decodeSlopeIndexMagnitude(slopeCodeIndexBuf[k+1])))
                        {
                            if (slopeCodeIndexBuf[k+1] > cod_slope_zero)
                            {
                                slopeCodeIndexBuf[k+1] = slopeCodeIndexBuf[k+1] - 1; /* Lower the slope one notch*/
                                slopeChanged = TRUE;
                            }
                            else if (slopeCodeIndexBuf[k+1] < cod_slope_zero)
                            {
                                slopeCodeIndexBuf[k+1] = slopeCodeIndexBuf[k+1] + 1; /* Lower the slope one notch*/
                                slopeChanged = TRUE;
                            }
                        }
                        else if ((k==nGainValues) || (decodeSlopeIndexMagnitude(slopeCodeIndexBuf[k]) >  decodeSlopeIndexMagnitude(slopeCodeIndexBuf[k+1])))
                        {
                            if (slopeCodeIndexBuf[k] > cod_slope_zero)
                            {
                                slopeCodeIndexBuf[k] = slopeCodeIndexBuf[k] - 1; /* Lower the slope one notch*/
                                slopeChanged = TRUE;
                                repeatCheck = TRUE;
                            }
                            else if (slopeCodeIndexBuf[k] < cod_slope_zero)
                            {
                                slopeCodeIndexBuf[k] = slopeCodeIndexBuf[k] + 1; /* Lower the slope one notch*/
                                slopeChanged = TRUE;
                                repeatCheck = TRUE;
                            }
                       }
                    }
                }
            }
        }
        for (k=0; k<nGainValues; k++)
        {
            drcGroupForOutput->slopeCodeIndex[k] = slopeCodeIndexBuf[k+1];
            drcGroupForOutput->slopeQuant[k] = decodeSlopeIndex(slopeCodeIndexBuf[k+1]);
        }
        

#ifdef NODE_RESERVOIR
        /* copy previous values to both, preroll and non-preroll struct */
        drcGroupForOutput->drcGainQuantPrev = drcGroupForOutput->drcGainQuant[drcGroupForOutput->nGainValues - 1];
        drcGroupForOutput->timeQuantPrev = drcGroupForOutput->tsGainQuant[drcGroupForOutput->nGainValues - 1] - drcParams->drcFrameSize;
        drcGroupForOutput->slopeCodeIndexPrev = drcGroupForOutput->slopeCodeIndex[drcGroupForOutput->nGainValues - 1];
        
        /* add node values from last frame (reservoir) to the current frame */
        nGainValuesCur = nGainValues;
        nGainValues    = nGainValues + drcGroupForOutput->nGainValuesReservoir;
        nGainValuesReservoirPrev = drcGroupForOutput->nGainValuesReservoir;
        memmove(drcGroupForOutput->drcGainQuant + drcGroupForOutput->nGainValuesReservoir, drcGroupForOutput->drcGainQuant, nGainValuesCur*sizeof(float));
        memcpy(drcGroupForOutput->drcGainQuant, drcGroupForOutput->drcGainQuantReservoir, drcGroupForOutput->nGainValuesReservoir*sizeof(float));
        memmove(slopeCodeIndexBuf + drcGroupForOutput->nGainValuesReservoir + 1, slopeCodeIndexBuf + 1, nGainValuesCur*sizeof(int)); /* first value is previous slope */
        memcpy(slopeCodeIndexBuf + 1, drcGroupForOutput->slopeCodeIndexBufReservoir, drcGroupForOutput->nGainValuesReservoir*sizeof(int));
        memcpy(drcGroupForOutput->tsGainQuant + nGainValuesCur, drcGroupForOutput->tsGainQuantReservoir, drcGroupForOutput->nGainValuesReservoir*sizeof(int));
        
        drcGroupForOutput->nGainValuesReservoir = 0;
        
        /* decision if node reservoir should be applied */
        thr = 6;
        if (nGainValues > thr && drcGainSeqBuf->gainSetParams.fullFrame != 1){
            drcGroupForOutput->nGainValuesReservoir = (int)ceil(((double)nGainValues-(double)thr)/2);
            if (drcGroupForOutput->nGainValuesReservoir > nGainValuesCur -1){
                drcGroupForOutput->nGainValuesReservoir = nGainValuesCur - 1;
            }
            nGainValues = nGainValues - drcGroupForOutput->nGainValuesReservoir;
            
            /* fill node reservoir buffers and remove nodes from current buffers */
            memcpy(drcGroupForOutput->tsGainQuantReservoir, drcGroupForOutput->tsGainQuant + nGainValuesCur - drcGroupForOutput->nGainValuesReservoir, drcGroupForOutput->nGainValuesReservoir*sizeof(int));
            memmove(drcGroupForOutput->tsGainQuant + nGainValuesCur-drcGroupForOutput->nGainValuesReservoir, drcGroupForOutput->tsGainQuant + nGainValuesCur, nGainValuesReservoirPrev*sizeof(int));
            memcpy(drcGroupForOutput->drcGainQuantReservoir, drcGroupForOutput->drcGainQuant + nGainValues, drcGroupForOutput->nGainValuesReservoir*sizeof(float));
            memcpy(drcGroupForOutput->slopeCodeIndexBufReservoir, slopeCodeIndexBuf + nGainValues + 1, drcGroupForOutput->nGainValuesReservoir*sizeof(int));
            for(k = 0; k < drcGroupForOutput->nGainValuesReservoir; k++){
                drcGroupForOutput->tsGainQuantReservoir[k] += drcParams->drcFrameSize;
            }
        }
        drcGroupForOutput->nGainValues = nGainValues;
#endif
        
        
        /* Final quantization and encoding */
#ifdef NODE_RESERVOIR
        drcGroupForOutput->frameEndFlag = 0;
        k = 0;
#endif
        for (n=0; n<nGainValues; n++)
        {
#ifdef NODE_RESERVOIR
            if ((drcGroupForOutput->tsGainQuant[n] == drcParams->drcFrameSize-1) && (nGainValues > 1)) /* if only one node, simple mode is preferred over frameEndFlag */
            {
                drcGroupForOutput->frameEndFlag = 1;
            }
            else
            {
                drcGroupForOutput->timeDeltaCodeIndex[k] = max(1, (drcGroupForOutput->tsGainQuant[n] - timePrev) / timeDeltaMin);
                
                timePrev += (drcGroupForOutput->timeDeltaCodeIndex[k]) * timeDeltaMin;
                k++;
            }
#else /* NODE_RESERVOIR */
            drcGroupForOutput->timeDeltaCodeIndex[n] = max(1, (drcGroupForOutput->tsGainQuant[n] - timePrev) / timeDeltaMin);
            
            timePrev += (drcGroupForOutput->timeDeltaCodeIndex[n]) * timeDeltaMin;
#endif /* NODE_RESERVOIR */
            if (n==0)
            {
                /* re-encode in case the initial gain was removed */

                encInitialGain(drcGainSeqBuf->gainSetParams.gainCodingProfile,
                               drcGroupForOutput->drcGainQuant[n],
                               &gainValueQuant,
                               &(drcGroupForOutput->gainCodeLength[n]),
                               &(drcGroupForOutput->gainCode[n]));
            }
            else
            {
                float deltaGain = drcGroupForOutput->drcGainQuant[n] - drcGainQuantPrev;
                getQuantizedDeltaDrcGain(drcGainSeqBuf->gainSetParams.gainCodingProfile,
                                         deltaGain,
                                         &deltaGainQuant,
                                         &(drcGroupForOutput->gainCodeLength[n]),
                                         &(drcGroupForOutput->gainCode[n]));
                gainValueQuant = deltaGainQuant + drcGainQuantPrev;
            }
            drcGainQuantPrev = gainValueQuant;
            drcGroupForOutput->drcGainQuant[n] = gainValueQuant;
        }
    
        drcGroupForOutput->codingMode = 1;  /* spline mode */
        if (nGainValues == 1)
        {
            if (drcGainSeqBuf->gainSetParams.gainInterpolationType == GAIN_INTERPOLATION_TYPE_SPLINE) {
                if (decodeSlopeIndexMagnitude(slopeCodeIndexBuf[1]) == 0.0f)
                {
                    if ((drcGroupForOutput->timeDeltaCodeIndex[0] == 0) || (drcGroupForOutput->timeDeltaCodeIndex[0] >  28))
                    {
                        drcGroupForOutput->codingMode = 0;  /* simple mode */
                    }
                    if ((decodeSlopeIndexMagnitude(slopeCodeIndexBuf[0]) == 0.0f) &&
                        (decodeSlopeIndexMagnitude(slopeCodeIndexBuf[2]) == 0.0f) &&
                        ((float)fabs((double)(gainBuf[1] - gainBuf[0])) < 0.126f) &&
                        ((float)fabs((double)(gainBuf[2] - gainBuf[1])) < 0.126f))
                    {
                        drcGroupForOutput->codingMode = 0;  /* simple mode */
                    }
                }
            }
            else { /* GAIN_INTERPOLATION_TYPE_LINEAR */
                if (drcGroupForOutput->timeDeltaCodeIndex[0] > (drcParams->drcFrameSize / drcParams->deltaTmin))
                {
                    drcGroupForOutput->codingMode = 0;  /* simple mode */
                }
            }
        }
        
        /* encode times and slopes */
        
        if (drcGroupForOutput->codingMode == 1)
        {
            const SlopeCodeTableEntry* slopeCodeTable = getSlopeCodeTableByValue();
            for (n=0; n<nGainValues; n++)
            {
                drcGroupForOutput->slopeCode[n] = slopeCodeTable[slopeCodeIndexBuf[n+1]].code;
                drcGroupForOutput->slopeCodeSize[n] = slopeCodeTable[slopeCodeIndexBuf[n+1]].size;
#ifdef NODE_RESERVOIR
                /* copy slope values after node reservoir */
                drcGroupForOutput->slopeCodeIndex[n] = slopeCodeIndexBuf[n+1];
                drcGroupForOutput->slopeQuant[n] = decodeSlopeIndex(slopeCodeIndexBuf[n+1]);
#endif
            }
            
            for (n=0; n<nGainValues; n++)
            {
                drcGroupForOutput->timeDeltaCode[n] = deltaTimeCodeTable[drcGroupForOutput->timeDeltaCodeIndex[n]].code;
                drcGroupForOutput->timeDeltaCodeSize[n] = deltaTimeCodeTable[drcGroupForOutput->timeDeltaCodeIndex[n]].size;
#ifdef NODE_RESERVOIR
                if (drcGroupForOutput->frameEndFlag == 1 && n == nGainValues-2)
                    break;
#endif
            }
        }
#if DEBUG_NODES
        for (n=0; n<nGainValues; n++)
        {
            printf("gain= %f slope= %f time= %d\n",
            drcGroupForOutput->drcGainQuant[n],
            drcGroupForOutput->slopeQuant[n],
            drcGroupForOutput->tsGainQuant[n]);
        }
        printf("=============================\n");
#endif
    }
}

/*********************************************************************************************/
void
quantizeDrcFrame(const int drcFrameSize,
                 const int timeDeltaMin,
                 const int nGainValuesMax,
                 const float* drcGainPerSampleWithPrevFrame,
                 const int* deltaTimeQuantTable,
                 const int gainCodingProfile,
                 DrcGroup* drcGroup,
                 DrcGroupForOutput* drcGroupForOutput)
{
    int n, i;
    float gainValueQuant;
    float deltaGainQuant;
    int nBits, code;
    
    float* gainAtNode = drcGroup->drcGain;
    float* slopeAtNode = drcGroup->slope;
    int* timeAtNode = drcGroup->tsGain;
    int* tsGainQuant = drcGroup->tsGainQuant;
    float* slopeQuant = drcGroup->slopeQuant;
    int* slopeCodeIndex = drcGroup->slopeCodeIndex;
    float maxTimeDeviation;
    int timeDeltaLeft;
    int timeDeltaRight;
    int tLeft, tRight;
    int k=0;
    
    const float* drcGainPerSample = drcGainPerSampleWithPrevFrame + drcFrameSize;
    float* drcGainQuantPrev = &(drcGroup->drcGainQuantPrev);
    int nDrcGainValues = drcGroup->nGainValues;
    
    /* Quantize the time coordinates of each node, insert new nodes if necessary */
    
    int restart = TRUE;
    while (restart)
    {
        restart = FALSE;
        n=0;
        while ((restart == FALSE) && (n<nDrcGainValues))
        {
            if (n==0)
            {
                timeDeltaLeft = timeAtNode[n];
                timeDeltaRight = timeAtNode[n+1] - timeAtNode[n];
            }
            else if (n<nDrcGainValues-1)
            {
                timeDeltaLeft = timeAtNode[n] - timeAtNode[n-1];
                timeDeltaRight = timeAtNode[n+1] - timeAtNode[n];
            }
            else
            {
                timeDeltaLeft = timeAtNode[n] - timeAtNode[n-1];
                timeDeltaRight = drcFrameSize - timeAtNode[n];
            }
            maxTimeDeviation = MAX_TIME_DEVIATION_FACTOR * min(timeDeltaLeft, timeDeltaRight);
            maxTimeDeviation = max(timeDeltaMin, maxTimeDeviation);
            
            i=0;
            while ((i<nGainValuesMax-2) && (deltaTimeQuantTable[i] < timeDeltaLeft))
            {
                i++;
            }
            if (i>0)
            {
                if (deltaTimeQuantTable[i] - timeDeltaLeft > timeDeltaLeft - deltaTimeQuantTable[i-1]) i--;
                if (deltaTimeQuantTable[i] >= drcFrameSize) i--;
            }
            if (abs(deltaTimeQuantTable[i] - timeDeltaLeft) > maxTimeDeviation)
            {
                int t;
                /* need to insert a new node */
                if (deltaTimeQuantTable[i] > timeDeltaLeft) i--;
                for (k=nDrcGainValues; k>n; k--)
                {
                    gainAtNode[k] = gainAtNode[k-1];
                    slopeAtNode[k] = slopeAtNode[k-1];
                    timeAtNode[k] = timeAtNode[k-1];
                }
                if (n>0)
                {
                    timeAtNode[n] = timeAtNode[n-1] + deltaTimeQuantTable[i];
                }
                else
                {
                    timeAtNode[n] = deltaTimeQuantTable[i];
                }
                nDrcGainValues++;
                t = timeAtNode[n];
                gainAtNode[n] = drcGainPerSample[t];
                tLeft = max(0, t - timeDeltaMin/2);
                tRight = min (drcFrameSize, tLeft + timeDeltaMin/2);
                slopeAtNode[n] = drcGainPerSample[tRight] - drcGainPerSample[tLeft];
                restart = TRUE;
            }
            n++;
        }
    }

    tsGainQuant[0] = (int)(timeDeltaMin * (timeAtNode[0] + 0.5f) / (float)timeDeltaMin);
    k = 1;
    for (n=1; n<nDrcGainValues; n++)
    {
        int tmp = (int)(timeDeltaMin * (timeAtNode[n] + 0.5f) / (float)timeDeltaMin);
        if (tmp > tsGainQuant[k-1])
        {
            tsGainQuant[k] = tmp;
            k++;
        }
    }
    
    nDrcGainValues = k;
    drcGroup->nGainValues = nDrcGainValues;
    
    /* Re-compute the gains and slopes for the quantized time stamps */
    for (n=0; n<nDrcGainValues; n++)
    {
        float drcGainPerSample_Limited;
        float slope;
        gainAtNode[n] = drcGainPerSample[tsGainQuant[n]];
        drcGainPerSample_Limited = limitDrcGain(gainCodingProfile, gainAtNode[n]);
        
        if (n==0)
        {
            encInitialGain(gainCodingProfile,
                           drcGainPerSample_Limited,
                           &gainValueQuant,
                           &nBits,
                           &code);
        }
        else
        {
            float deltaGain = drcGainPerSample_Limited - *drcGainQuantPrev;
            getQuantizedDeltaDrcGain(gainCodingProfile,
                                     deltaGain,
                                     &deltaGainQuant,
                                     &nBits,
                                     &code);
            gainValueQuant = deltaGainQuant + *drcGainQuantPrev;
        }
        drcGroup->drcGainQuant[n] = gainValueQuant;
        drcGroup->gainCode[n] = code;
        drcGroup->gainCodeLength[n] = nBits;
        *drcGainQuantPrev = gainValueQuant;
        
        tRight = min(drcFrameSize - 1, tsGainQuant[n] + timeDeltaMin / 2);
        tLeft = tRight - timeDeltaMin;
        slope = drcGainPerSample[tRight] - drcGainPerSample[tLeft];
        slopeAtNode[n] = slope;
        quantizeSlope (slope, &(slopeQuant[n]), &(slopeCodeIndex[n]));

    }
    drcGroup->nGainValues = nDrcGainValues;
    drcGroup->gainPrevNode = gainAtNode[nDrcGainValues-1];
    drcGroupForOutput->drcGainQuantNext = drcGroup->drcGainQuant[0];
    drcGroupForOutput->slopeCodeIndexNext = drcGroup->slopeCodeIndex[0];
    drcGroupForOutput->timeQuantNext = drcGroup->tsGainQuant[0] + drcFrameSize;
    
#if DEBUG_UNIDRC_ENC
    for (n=0; n<nDrcGainValues; n++)
    {
        printf("Gain[%d]=%6.3f slope=%8.4f time=%4d\n", n, drcGroup->drcGainQuant[n], slopeQuant[n], tsGainQuant[n]);
    }
#endif
}
/*********************************************************************************************/
void
quantizeAndEncodeDrcGain(DrcEncParams* drcParams,
                         const float* drcGainPerSample,
                         float* drcGainPerSampleWithPrevFrame,
                         DeltaTimeCodeTableEntry* deltaTimeCodeTable,
                         DrcGainSeqBuf* drcGainSeqBuf)
{
    int drcFrameSize = drcParams->drcFrameSize;
    
    const int* deltaTimeQuantTable = drcParams->deltaTimeQuantTable;
    DrcGroup* drcGroup;
    DrcGroupForOutput* drcGroupForOutput;

    advanceNodes(drcParams, drcGainSeqBuf);

    drcGroup = &(drcGainSeqBuf->drcGroup);
    drcGroupForOutput = &(drcGainSeqBuf->drcGroupForOutput);

    getPreliminaryNodes(drcParams,
                        drcGainPerSample,
                        drcGainPerSampleWithPrevFrame,
                        drcGroup,
                        drcGainSeqBuf->gainSetParams.fullFrame);
    
    quantizeDrcFrame(drcFrameSize,
                     drcParams->deltaTmin,
                     drcParams->drcFrameSize / drcParams->deltaTmin,
                     drcGainPerSampleWithPrevFrame,
                     deltaTimeQuantTable,
                     drcGainSeqBuf->gainSetParams.gainCodingProfile,
                     drcGroup,
                     drcGroupForOutput);

    postProcessNodes(drcParams, deltaTimeCodeTable, drcGainSeqBuf);
}
