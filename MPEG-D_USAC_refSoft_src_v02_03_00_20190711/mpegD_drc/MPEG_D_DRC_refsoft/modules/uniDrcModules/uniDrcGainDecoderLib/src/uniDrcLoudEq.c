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

#include "uniDrcCommon.h"
#include "uniDrc.h"
#include "uniDrcGainDecoder.h"
#include "uniDrcGainDecoder_api.h"
#include "uniDrcGainDec.h"
#include "uniDrcLoudEq.h"

#if MPEG_D_DRC_EXTENSION_V1

int
initLoudEqGainBuffer (LoudEqInstructions* loudEqInstructions,
                      int drcFrameSize,
                      GainBuffer* loudEqGainBuffers)
{
    int err;
    
    err = initGainBuffer(0,
                         loudEqInstructions->loudEqGainSequenceCount,
                         drcFrameSize,
                         loudEqGainBuffers);
    if (err) return (err);

    return (0);
}

int
initLoudnessEqualizer(HANDLE_LOUDNESS_EQ_STRUCT loudnessEq,
                      HANDLE_UNI_DRC_GAIN_DEC_STRUCTS hDrcDec,
                      float mixingLevel)
{
    /* add equalizer initialization here */
    loudnessEq->mixingLevel = mixingLevel;
    loudnessEq->isActive = FALSE;
    loudnessEq->drcFrameSize = hDrcDec->drcParams.drcFrameSize;
    loudnessEq->deltaTminDefault = hDrcDec->drcParams.deltaTminDefault;
    
    return(0);
}

int
initLoudEq(HANDLE_LOUDNESS_EQ_STRUCT* loudnessEq,
           HANDLE_UNI_DRC_GAIN_DEC_STRUCTS hDrcDec,
           UniDrcConfigExt* uniDrcConfigExt,
           int loudEqInstructionsSelectedIndex,
           float mixingLevel)
{
    int err;

    *loudnessEq = (HANDLE_LOUDNESS_EQ_STRUCT) calloc(1, sizeof(struct T_LOUDNESS_EQ_STRUCT));
    
    err = initLoudnessEqualizer (*loudnessEq, hDrcDec, mixingLevel);
    if (err) return (err);
    err = initLoudEqGainBuffer (&uniDrcConfigExt->loudEqInstructions[loudEqInstructionsSelectedIndex], (*loudnessEq)->drcFrameSize, &(*loudnessEq)->loudEqGainBuffer);
    if (err) return (err);
    (*loudnessEq)->loudEqInstructionsSelectedIndex = loudEqInstructionsSelectedIndex;
    
    return (0);
}

int
removeLoudEq(HANDLE_LOUDNESS_EQ_STRUCT* loudnessEq)
{
    int err;
    if (*loudnessEq)
    {
        err = removeGainBuffer (&(*loudnessEq)->loudEqGainBuffer);
        if (err) return (err);
        
        free(*loudnessEq);
        *loudnessEq = NULL;
    }
    return (0);
}

int
updateLoudnessEqualizerParams(HANDLE_LOUDNESS_EQ_STRUCT hLoudnessEq,
                              HANDLE_UNI_DRC_INTERFACE  hUniDrcInterface)
{
    int i;
    
    hLoudnessEq->isActive = FALSE;
    if (hUniDrcInterface)
    {
        if (hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent)
        {
            LoudnessEqParameterInterface* loudnessEqParameters = &hLoudnessEq->loudnessEqParameters;
            *loudnessEqParameters = hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface;
            for (i=0; i<LOUD_EQ_BAND_COUNT_MAX; i++)
            {
                if (loudnessEqParameters->sensitivityPresent)
                {
                    hLoudnessEq->playbackLevel[i] = loudnessEqParameters->sensitivity;
                }
                if (loudnessEqParameters->playbackGainPresent)
                {
                    hLoudnessEq->playbackLevel[i] += loudnessEqParameters->playbackGain;
                }
            }
            if ((loudnessEqParameters->loudnessEqRequestPresent) && (loudnessEqParameters->sensitivityPresent))
            {
                if (loudnessEqParameters->loudnessEqRequest != LOUD_EQ_REQUEST_OFF)
                {
                    hLoudnessEq->isActive = TRUE;
                }
            }
        }
    }
    
/* For testing */
/*    hLoudnessEq->playbackLevel[0] = 80.0f;
      hLoudnessEq->playbackLevel[1] = 75.0f;
      hLoudnessEq->isActive = TRUE; */
    return(0);
}

int
processLoudnessEqualizer(HANDLE_LOUDNESS_EQ_STRUCT loudnessEq,
                         int frequencyRangeCount,
                         int* frequencyRangeIndex,
                         float* levelDifference,    /* long-term acoustic level difference: playback level - mixing level for each band */
                         float* dynMixLevel         /* momentary acoustic level at mixing location for each band */
                         /* float* audioIn,
                         float* audioOut */)
{
    /* Add DRC gain and/or EQ gain to playbackLevel here if neccessary */
    /* add equalizer here */

#if 0  /* For testing only */
    static FILE* fdebug=NULL;
    if (fdebug==NULL) fdebug=fopen("level.dat","wb");
    fwrite((dynMixLevel), 1, sizeof(float), fdebug);
#endif
    return(0);
}

/* The following function is not normative. It is only provided as an example for illustrative purposes */
int
processLoudnessEq(HANDLE_LOUDNESS_EQ_STRUCT hLoudnessEq,
                  HANDLE_UNI_DRC_CONFIG     hUniDrcConfig)
{
    int i, n;
    GainBuffer* gainBuffer = &(hLoudnessEq->loudEqGainBuffer);
    LoudEqInstructions* loudEqInstructions = &(hUniDrcConfig->uniDrcConfigExt.loudEqInstructions[hLoudnessEq->loudEqInstructionsSelectedIndex]);         /* select the one that matches the downmix etc. */
    int* frequencyRangeIndex = loudEqInstructions->frequencyRangeIndex;
    float dynMixLevel[LOUD_EQ_BAND_COUNT_MAX];
    float levelDifference[LOUD_EQ_BAND_COUNT_MAX];
    for (n=0; n<hLoudnessEq->drcFrameSize; n++)
    {
        for (i=0; i<loudEqInstructions->loudEqGainSequenceCount; i++)
        {
            /* Add DRC gain and/or EQ gain to playbackLevel here if neccessary */
            levelDifference[i] = hLoudnessEq->playbackLevel[i] - hLoudnessEq->mixingLevel;
            dynMixLevel[i] = LOUD_EQ_REF_LEVEL + gainBuffer->bufferForInterpolation[i].lpcmGains[n];
        }
        processLoudnessEqualizer(hLoudnessEq,
                                 loudEqInstructions->loudEqGainSequenceCount,
                                 frequencyRangeIndex,
                                 levelDifference,
                                 dynMixLevel
                                 /* audioIn, audioOut */);  /* audio still missing here */
    }
    return (0);
}

int
processLoudEq(HANDLE_LOUDNESS_EQ_STRUCT hLoudnessEq,
              HANDLE_UNI_DRC_CONFIG     hUniDrcConfig,
              HANDLE_UNI_DRC_GAIN       hUniDrcGain)

{
    int err;
    
    if (hLoudnessEq->isActive)
    {
        /* interpolate gains of sequences used */
        err = getEqLoudness (hLoudnessEq,
                             hUniDrcConfig,
                             hUniDrcGain);
        if (err) return (err);
        
        err = processLoudnessEq(hLoudnessEq,
                                hUniDrcConfig);
        if (err) return (err);
    }
    return (0);
}

#endif /* MPEG_D_DRC_EXTENSION_V1 */
