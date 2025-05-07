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

int
initProcessAudio (UniDrcConfig* uniDrcConfig,
                  DrcParams* drcParams,
#if !AMD1_SYNTAX
                  AudioBandBuffer* audioBandBuffer)
#else /* AMD1_SYNTAX */
                  AudioBandBuffer* audioBandBuffer,
                  AudioIOBuffer* audioIOBuffer)
#endif
{
    int c, ix;
    int audioFrameSize = drcParams->drcFrameSize;
    int maxMultibandAudioSignalCount = 0;

    for (ix=0; ix<drcParams->drcSetCounter; ix++)
    {
        DrcInstructionsUniDrc* drcInstructionsUniDrc;
        drcInstructionsUniDrc = &(uniDrcConfig->drcInstructionsUniDrc[drcParams->selDrc[ix].drcInstructionsIndex]);
        maxMultibandAudioSignalCount = max(maxMultibandAudioSignalCount, drcInstructionsUniDrc->multibandAudioSignalCount);
    }

    audioBandBuffer->noninterleavedAudio = (float**) calloc (maxMultibandAudioSignalCount, sizeof(float*));
    for (c=0; c<maxMultibandAudioSignalCount; c++)
    {
        audioBandBuffer->noninterleavedAudio[c] = (float*) calloc (audioFrameSize, sizeof(float));
    }
    audioBandBuffer->multibandAudioSignalCount = maxMultibandAudioSignalCount;
    audioBandBuffer->audioFrameSize = audioFrameSize;
    
#if AMD1_SYNTAX
    if (drcParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_OFF && audioIOBuffer->audioDelaySamples) {
        audioIOBuffer->audioIOBufferDelayed = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        audioIOBuffer->audioIOBuffer        = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        for (c=0; c<audioIOBuffer->audioChannelCount; c++) {
            audioIOBuffer->audioIOBufferDelayed[c] = (float*) calloc(audioIOBuffer->audioFrameSize + audioIOBuffer->audioDelaySamples, sizeof(float));
            audioIOBuffer->audioIOBuffer[c] = &audioIOBuffer->audioIOBufferDelayed[c][audioIOBuffer->audioDelaySamples];
        }
    }
    if (drcParams->subBandDomainMode != SUBBAND_DOMAIN_MODE_OFF && audioIOBuffer->audioDelaySubBandSamples) {
        audioIOBuffer->audioIOBufferDelayedReal = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        audioIOBuffer->audioIOBufferDelayedImag = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        audioIOBuffer->audioIOBufferReal        = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        audioIOBuffer->audioIOBufferImag        = (float**) calloc(audioIOBuffer->audioChannelCount, sizeof(float*));
        for (c=0; c<audioIOBuffer->audioChannelCount; c++) {
            audioIOBuffer->audioIOBufferDelayedReal[c] = (float*) calloc((audioIOBuffer->audioSubBandFrameSize + audioIOBuffer->audioDelaySubBandSamples) * audioIOBuffer->audioSubBandCount, sizeof(float));
            audioIOBuffer->audioIOBufferDelayedImag[c] = (float*) calloc((audioIOBuffer->audioSubBandFrameSize + audioIOBuffer->audioDelaySubBandSamples) * audioIOBuffer->audioSubBandCount, sizeof(float));
            audioIOBuffer->audioIOBufferReal[c] = &audioIOBuffer->audioIOBufferDelayedReal[c][audioIOBuffer->audioDelaySubBandSamples * audioIOBuffer->audioSubBandCount];
            audioIOBuffer->audioIOBufferImag[c] = &audioIOBuffer->audioIOBufferDelayedImag[c][audioIOBuffer->audioDelaySubBandSamples * audioIOBuffer->audioSubBandCount];
        }
    }
#endif /* AMD1_SYNTAX */
    return (0);
}

int
#if !AMD1_SYNTAX
removeProcessAudio(AudioBandBuffer* audioBandBuffer)
#else /* AMD1_SYNTAX */
removeProcessAudio(AudioBandBuffer* audioBandBuffer,
                   const int        subBandDomainMode,
                   AudioIOBuffer*   audioIOBuffer)
#endif
{
    int c;

    if (audioBandBuffer->noninterleavedAudio)
    {
        for (c=0; c<audioBandBuffer->multibandAudioSignalCount; c++)
        {
            if (audioBandBuffer->noninterleavedAudio[c])
            {
                free(audioBandBuffer->noninterleavedAudio[c]);
                (audioBandBuffer->noninterleavedAudio[c]) = NULL;
            }
        }
        free(audioBandBuffer->noninterleavedAudio);
        audioBandBuffer->noninterleavedAudio = NULL;
        audioBandBuffer->multibandAudioSignalCount = 0;
        audioBandBuffer->audioFrameSize = 0;
    }
#if AMD1_SYNTAX
    if (subBandDomainMode == SUBBAND_DOMAIN_MODE_OFF && audioIOBuffer->audioDelaySamples) {
        for (c=0; c<audioIOBuffer->audioChannelCount; c++) {
            free(audioIOBuffer->audioIOBufferDelayed[c]); audioIOBuffer->audioIOBufferDelayed[c] = NULL;
        }
        free(audioIOBuffer->audioIOBufferDelayed); audioIOBuffer->audioIOBufferDelayed = NULL;
        free(audioIOBuffer->audioIOBuffer); audioIOBuffer->audioIOBuffer = NULL;
    }
    if (subBandDomainMode != SUBBAND_DOMAIN_MODE_OFF && audioIOBuffer->audioDelaySubBandSamples) {
        for (c=0; c<audioIOBuffer->audioChannelCount; c++) {
            free(audioIOBuffer->audioIOBufferDelayedReal[c]); audioIOBuffer->audioIOBufferDelayedReal[c] = NULL;
            free(audioIOBuffer->audioIOBufferDelayedImag[c]); audioIOBuffer->audioIOBufferDelayedImag[c] = NULL;
        }
        free(audioIOBuffer->audioIOBufferDelayedReal); audioIOBuffer->audioIOBufferDelayedReal = NULL;
        free(audioIOBuffer->audioIOBufferDelayedImag); audioIOBuffer->audioIOBufferDelayedImag = NULL;
        free(audioIOBuffer->audioIOBufferReal); audioIOBuffer->audioIOBufferReal = NULL;
        free(audioIOBuffer->audioIOBufferImag); audioIOBuffer->audioIOBufferImag = NULL;
    }
#endif /* AMD1_SYNTAX */
    return(0);
}

/* time-domain DRC: in-place application of DRC gains to audio frame */
int
applyGains(DrcInstructionsUniDrc* drcInstructionsArray,
           const int drcInstructionsIndex,
           DrcParams* drcParams,
           GainBuffer* gainBuffer,
#if MPEG_D_DRC_EXTENSION_V1
           ShapeFilterBlock shapeFilterBlock[],
#endif /* MPEG_D_DRC_EXTENSION_V1 */
           float* deinterleavedAudio[],
           int applyGains)
{
    int c, b, g, i;
    int offset = 0, signalIndex = 0;
    int gainIndexForGroup[CHANNEL_GROUP_COUNT_MAX];
    int signalIndexForChannel[CHANNEL_COUNT_MAX];
    float* lpcmGains;
#if MPEG_D_DRC_EXTENSION_V1
    float drcGainLast, gainThr;
    int iEnd, iStart;
#endif
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    
    if (drcInstructionsIndex >= 0)
    {
        drcInstructionsUniDrc = &(drcInstructionsArray[drcInstructionsIndex]);
        {
            if (drcInstructionsUniDrc->drcSetId > 0)
            {
                if (drcParams->delayMode == DELAY_MODE_LOW_DELAY)
                {
                    offset = drcParams->drcFrameSize;
                }
                gainIndexForGroup[0] = 0;
                for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups-1; g++)
                {
                    gainIndexForGroup[g+1] = gainIndexForGroup[g] + drcInstructionsUniDrc->bandCountForChannelGroup[g]; /* index of first gain sequence in channel group */
                }
                signalIndexForChannel[0] = 0;
                for (c=0; c<drcInstructionsUniDrc->audioChannelCount-1; c++)
                {
                    if (drcInstructionsUniDrc->channelGroupForChannel[c] >= 0)
                    {
                        signalIndexForChannel[c+1] = signalIndexForChannel[c] + drcInstructionsUniDrc->bandCountForChannelGroup[drcInstructionsUniDrc->channelGroupForChannel[c]];
                    }
                    else
                    {
                        signalIndexForChannel[c+1] = signalIndexForChannel[c] + 1;
                    }
                }
                
                for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
                {
                    for (b=0; b<drcInstructionsUniDrc->bandCountForChannelGroup[g]; b++)
                    {
#if !AMD1_SYNTAX
                        lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples + offset;
#else /* AMD1_SYNTAX */
                        if (drcInstructionsUniDrc->channelGroupIsParametricDrc[g] == 0) {
                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples - drcParams->audioDelaySamples + offset;
                        } else {
#if AMD2_COR3
                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples + drcParams->drcFrameSize;
#else
                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples;
#endif
                        }
#endif
#if MPEG_D_DRC_EXTENSION_V1
                        iEnd = 0;
                        iStart = 0;
                        while (iEnd < drcParams->drcFrameSize)
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                        {
#if MPEG_D_DRC_EXTENSION_V1
                            if (shapeFilterBlock[g].shapeFilterBlockPresent) {
                                drcGainLast = shapeFilterBlock[g].drcGainLast;
                                gainThr = 0.0001f * drcGainLast;
                                while ((iEnd<drcParams->drcFrameSize) && (fabs(lpcmGains[iEnd] - drcGainLast) <= gainThr)) iEnd++;
                            }
                            else
                            {
                                iEnd = drcParams->drcFrameSize;
                            }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
#if MPEG_H_SYNTAX
                            for (c=drcParams->channelOffset; c<drcParams->channelOffset+drcParams->numChannelsProcess; c++)
#else
                            for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++)
#endif
                            {
                                if (g == drcInstructionsUniDrc->channelGroupForChannel[c])
                                {
                                    signalIndex = signalIndexForChannel[c] + b;
                                    
#if MPEG_D_DRC_EXTENSION_V1
                                    for (i=iStart; i<iEnd; i++)
                                    {
                                        if(applyGains == 1) {
                                            float audioOut;
                                            processShapeFilterBlock(&shapeFilterBlock[g],
                                                                    lpcmGains[i],
                                                                    signalIndex,
                                                                    deinterleavedAudio[signalIndex][i],
                                                                    &audioOut);
                                            deinterleavedAudio[signalIndex][i] = audioOut * lpcmGains[i];
                                        }
                                        else
                                            deinterleavedAudio[signalIndex][i] = lpcmGains[i];
                                    }
#else
                                    for (i=0; i<drcParams->drcFrameSize; i++)
                                    {
                                        if(applyGains == 1) {
                                            deinterleavedAudio[signalIndex][i] *= lpcmGains[i];
                                        }
                                        else
                                            deinterleavedAudio[signalIndex][i] = lpcmGains[i];
                                    }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                                }
                            }
#if MPEG_D_DRC_EXTENSION_V1
                            if ((iEnd < drcParams->drcFrameSize) && (shapeFilterBlock[g].shapeFilterBlockPresent))
                            {
                                adaptShapeFilterBlock(lpcmGains[iEnd], &shapeFilterBlock[g]);
                            }
                            iStart = iEnd;
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                        }
                    }
                }
            }
        }
    }
    return (0);
}

/* subband-domain DRC: in-place application of DRC gains to audio frame */
int
applyGainsSubband(DrcInstructionsUniDrc* drcInstructionsArray,
           const int drcInstructionsIndex,
           DrcParams* drcParams,
           GainBuffer* gainBuffer,
           OverlapParams* overlapParams,
#if AMD1_SYNTAX
           float* deinterleavedAudioDelayedReal[],
           float* deinterleavedAudioDelayedImag[],
#endif /* AMD1_SYNTAX */
           float* deinterleavedAudioReal[],
           float* deinterleavedAudioImag[])
{
    int c, b, g, m, s;
    int gainIndexForGroup[CHANNEL_GROUP_COUNT_MAX];
    float* lpcmGains;
    float gainSb, gainLr;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    int offset = 0, signalIndex = 0;
    int drcFrameSizeSb = 0;
    int nDecoderSubbands = 0;
    int L = 0; /* L: downsampling factor */
    int analysisDelay = 0;
    switch (drcParams->subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
            analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
            analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
            L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
            analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_STFT256;
            break;
        default:
            return -1;
            break;
    }
    drcFrameSizeSb = drcParams->drcFrameSize/L;
    
    if (drcInstructionsIndex >= 0)
    {
        drcInstructionsUniDrc = &(drcInstructionsArray[drcInstructionsIndex]);
        {
            if (drcInstructionsUniDrc->drcSetId > 0)
            {
                if (drcParams->delayMode == DELAY_MODE_LOW_DELAY)
                {
                    offset = drcParams->drcFrameSize;
                }
                gainIndexForGroup[0] = 0;
                for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups-1; g++)
                {
                    gainIndexForGroup[g+1] = gainIndexForGroup[g] + drcInstructionsUniDrc->bandCountForChannelGroup[g]; /* index of first gain sequence in channel group */
                }
                
#if MPEG_H_SYNTAX
                if (drcParams->numChannelsProcess == -1)
                    drcParams->numChannelsProcess = drcInstructionsUniDrc->audioChannelCount;

                if ((drcParams->channelOffset + drcParams->numChannelsProcess) > (drcInstructionsUniDrc->audioChannelCount))
                    return -1;

                for (c=drcParams->channelOffset; c<drcParams->channelOffset+drcParams->numChannelsProcess; c++)
#else
                for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++)
#endif
                {
                    g = drcInstructionsUniDrc->channelGroupForChannel[c];
                    if (g>=0)
                    {
                        for (m=0; m<drcFrameSizeSb; m++)
                        {   
                            if (drcInstructionsUniDrc->bandCountForChannelGroup[g] > 1)
                            {   /* multiband DRC */
                                for (s=0; s<nDecoderSubbands; s++)
                                {
                                    gainSb = 0.0f;
                                    for (b=0; b<drcInstructionsUniDrc->bandCountForChannelGroup[g]; b++)
                                    {
#if !AMD1_SYNTAX
                                        lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples + offset;
#else /* AMD1_SYNTAX */
                                        if (drcInstructionsUniDrc->channelGroupIsParametricDrc[g] == 0) {
                                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples - drcParams->audioDelaySamples + offset;
                                        } else {
#if AMD2_COR3
                                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples + analysisDelay + drcParams->drcFrameSize;
#else
                                            lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]+b].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples + analysisDelay;
#endif
                                        }
#endif
                                        /* get gain for this timeslot by downsampling */
                                        gainLr = lpcmGains[(m*L+(L-1)/2)];
                                        gainSb += overlapParams->overlapParamsForGroup[g].overlapParamsForBand[b].overlapWeight[s] * gainLr;
                                    }
#if !AMD1_SYNTAX
                                    deinterleavedAudioReal[signalIndex][m*nDecoderSubbands+s] *= gainSb;
                                    if (drcParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_STFT256) { /* For STFT filterbank, the real value of the nyquist band is stored at the imag value of the first band */
                                        if (s != 0)
                                            deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] *= gainSb;
                                        if (s == (nDecoderSubbands-1))
                                            deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+0] *= gainSb;
                                    } else {
                                        deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] *= gainSb;
                                    }
#else /* AMD1_SYNTAX */
                                    deinterleavedAudioReal[signalIndex][m*nDecoderSubbands+s] = gainSb * deinterleavedAudioDelayedReal[signalIndex][m*nDecoderSubbands+s];
                                    if (drcParams->subBandDomainMode == SUBBAND_DOMAIN_MODE_STFT256) { /* For STFT filterbank, the real value of the nyquist band is stored at the imag value of the first band */
                                        if (s != 0)
                                            deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] = gainSb * deinterleavedAudioDelayedImag[signalIndex][m*nDecoderSubbands+s];
                                        if (s == (nDecoderSubbands-1))
                                            deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+0] = gainSb * deinterleavedAudioDelayedImag[signalIndex][m*nDecoderSubbands+0];
                                    } else {
                                        deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] = gainSb * deinterleavedAudioDelayedImag[signalIndex][m*nDecoderSubbands+s];
                                    }
#endif
                                }
                            }
                            else
                            {   /* single-band DRC */
#if !AMD1_SYNTAX
                                lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples + offset;
#else /* AMD1_SYNTAX */
                                if (drcInstructionsUniDrc->channelGroupIsParametricDrc[g] == 0) {
                                    lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]].lpcmGains + MAX_SIGNAL_DELAY - drcParams->gainDelaySamples - drcParams->audioDelaySamples + offset;
                                } else {
#if AMD2_COR3
                                    lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples + analysisDelay + drcParams->drcFrameSize;
#else
                                    lpcmGains = gainBuffer->bufferForInterpolation[gainIndexForGroup[g]].lpcmGains + MAX_SIGNAL_DELAY + drcInstructionsUniDrc->parametricDrcLookAheadSamples[g] - drcParams->audioDelaySamples + analysisDelay;
#endif
                                }
#endif
                                /* get gain for this timeslot by downsampling */
                                gainSb = lpcmGains[(m*L+(L-1)/2)];
                                for (s=0; s<nDecoderSubbands; s++)
                                {
#if !AMD1_SYNTAX
                                    deinterleavedAudioReal[signalIndex][m*nDecoderSubbands+s] *= gainSb;
                                    deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] *= gainSb;
#else /* AMD1_SYNTAX */
                                    deinterleavedAudioReal[signalIndex][m*nDecoderSubbands+s] = gainSb * deinterleavedAudioDelayedReal[signalIndex][m*nDecoderSubbands+s];
                                    deinterleavedAudioImag[signalIndex][m*nDecoderSubbands+s] = gainSb * deinterleavedAudioDelayedImag[signalIndex][m*nDecoderSubbands+s];
#endif
                                }
                            }
                        }
                    }
                    signalIndex++;
                }
            }
        }
    }
    return (0);
}

int
addDrcBandAudio(DrcInstructionsUniDrc* drcInstructionsArray,
                const int drcInstructionsIndex,
                DrcParams* drcParams,
                AudioBandBuffer* audioBandBuffer,
                float* audioIoBuffer[])
{
    int g, b, i, c;
    float sum;
    int signalIndex = 0;
    float** drcBandAudio;
    float** channelAudio;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    
    drcBandAudio = audioBandBuffer->noninterleavedAudio;
    channelAudio = audioIoBuffer;
    
    if (drcInstructionsIndex >= 0) {
        drcInstructionsUniDrc = &(drcInstructionsArray[drcInstructionsIndex]);
    } else {
        return -1;
    }

    if (drcInstructionsUniDrc->drcSetId > 0)
    {
#if MPEG_H_SYNTAX
        for (c=drcParams->channelOffset; c<drcParams->channelOffset+drcParams->numChannelsProcess; c++)
#else
        for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++)
#endif
        {
            g = drcInstructionsUniDrc->channelGroupForChannel[c];
            if (g>=0)
            {
                for (i=0; i<drcParams->drcFrameSize; i++)
                {
                    sum = 0.0f;
                    for (b=0; b<drcInstructionsUniDrc->bandCountForChannelGroup[g]; b++)
                    {
                        sum += drcBandAudio[signalIndex+b][i];
                    }
#if MPEG_H_SYNTAX
                    channelAudio[c-drcParams->channelOffset][i] = sum;
#else
                    channelAudio[c][i] = sum;
#endif
                }
                signalIndex += drcInstructionsUniDrc->bandCountForChannelGroup[g];
            }
            else
            {
#if AMD1_SYNTAX
                for (i=0; i<drcParams->drcFrameSize; i++)
                {
#if MPEG_H_SYNTAX
                    channelAudio[c-drcParams->channelOffset][i] = drcBandAudio[signalIndex][i];
#else
                    channelAudio[c][i] = drcBandAudio[signalIndex][i];
#endif
                }
#endif
                signalIndex++;
            }
        }
    }
    else
    {
        /* pass through */
#if MPEG_H_SYNTAX
        for (c=0; c<drcParams->numChannelsProcess; c++)
#else
        for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++)
#endif
        {
            for (i=0; i<drcParams->drcFrameSize; i++)
            {
                channelAudio[c][i] = drcBandAudio[c][i];
            }
        }
    }
    return (0);
}

int
applyFilterBanks(DrcInstructionsUniDrc* drcInstructionsArray,
                 const int drcInstructionsIndex,
                 DrcParams* drcParams,
                 float* audioIoBuffer[],
                 AudioBandBuffer* audioBandBuffer,
                 FilterBanks* filterBanks,
                 const int passThru)
{
    int c, g, e, i, nBands, err = 0;
    float* audioIn;
    float** audioOut;
    DrcFilterBank* drcFilterBank;
    DrcInstructionsUniDrc* drcInstructionsUniDrc;
    int drcFrameSize = drcParams->drcFrameSize;

    if (drcInstructionsIndex >= 0) {
        drcInstructionsUniDrc = &(drcInstructionsArray[drcInstructionsIndex]);
    }  else {
        return -1;
    }

    e=0;
#if MPEG_H_SYNTAX
    for (c=drcParams->channelOffset; c<drcParams->channelOffset+drcParams->numChannelsProcess; c++)
#else
    for (c=0; c<drcInstructionsUniDrc->audioChannelCount; c++)
#endif
    {
        drcFilterBank = NULL;
#if MPEG_H_SYNTAX
        audioIn = audioIoBuffer[c-drcParams->channelOffset];
#else
        audioIn = audioIoBuffer[c];
#endif
        audioOut = &(audioBandBuffer->noninterleavedAudio[e]);
        if ((passThru == FALSE) && (drcInstructionsIndex >= 0))
        {
            if (drcInstructionsUniDrc->drcSetId < 0)
            {
                nBands = 1;  /* pass through*/
            }
            else
            {
                g = drcInstructionsUniDrc->channelGroupForChannel[c];
                if (g== -1)
                {
                    nBands = 1;
                    if (filterBanks->drcFilterBank != NULL)
                    {
                        drcFilterBank = &(filterBanks->drcFilterBank[drcInstructionsUniDrc->nDrcChannelGroups]);
                    }
                }
                else
                {
                    nBands = drcInstructionsUniDrc->bandCountForChannelGroup[g];
                    if (filterBanks->drcFilterBank != NULL)
                    {
                        drcFilterBank = &(filterBanks->drcFilterBank[g]);
                    }
                }
                if (filterBanks->drcFilterBank != NULL)
                {
                    if (drcFilterBank->allPassCascade != NULL)
                    {
                        err = runAllPassCascade(drcFilterBank->allPassCascade, c, drcFrameSize, audioIn); /* result in place */
                        if (err) return (err);
                    }
                }
            }
        }
        else
        {
            nBands = 1;
        }
        switch (nBands) {
            case 1:
                for (i=0; i<drcFrameSize; i++)
                {
                    audioOut[0][i] = audioIn[i];
                }
                e++;
                break;
            case 2:
                err = runTwoBandBank(drcFilterBank->twoBandBank,
                                     c,
                                     drcFrameSize,
                                     audioIn,
                                     audioOut);
                if (err) return (err);
                e+=2;
                break;
            case 3:
                err = runThreeBandBank(drcFilterBank->threeBandBank,
                                       c,
                                       drcFrameSize,
                                       audioIn,
                                       audioOut);
                if (err) return (err);
                e+=3;
                break;
            case 4:
                err = runFourBandBank(drcFilterBank->fourBandBank,
                                      c,
                                      drcFrameSize,
                                      audioIn,
                                      audioOut);
                if (err) return (err);
                e+=4;
                break;
            default:
                fprintf(stderr, "ERROR: unsupported number of DRC bands: %d\n", nBands);
                return (PARAM_ERROR);
                break;
        }
    }
    
    return (0);
}

#if AMD1_SYNTAX
int
storeAudioIOBufferTime(float*         audioIOBuffer[],
                       AudioIOBuffer* audioIOBufferInternal)
{
    int i,j;
    
    if (audioIOBufferInternal->audioDelaySamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            for (j=0; j<audioIOBufferInternal->audioFrameSize; j++) {
                audioIOBufferInternal->audioIOBufferDelayed[i][audioIOBufferInternal->audioDelaySamples+j] = audioIOBuffer[i][j];
            }
        }
    } else {
        audioIOBufferInternal->audioIOBufferDelayed = audioIOBuffer;
        audioIOBufferInternal->audioIOBuffer        = audioIOBuffer;
    }
    
    return 0;
}

int
storeAudioIOBufferFreq(float*         audioIOBufferReal[],
                       float*         audioIOBufferImag[],
                       AudioIOBuffer* audioIOBufferInternal)
{
    int i,j;
    
    if (audioIOBufferInternal->audioDelaySubBandSamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            for (j=0; j<audioIOBufferInternal->audioSubBandFrameSize*audioIOBufferInternal->audioSubBandCount; j++) {
                audioIOBufferInternal->audioIOBufferDelayedReal[i][audioIOBufferInternal->audioDelaySubBandSamples*audioIOBufferInternal->audioSubBandCount+j] = audioIOBufferReal[i][j];
                audioIOBufferInternal->audioIOBufferDelayedImag[i][audioIOBufferInternal->audioDelaySubBandSamples*audioIOBufferInternal->audioSubBandCount+j] = audioIOBufferImag[i][j];
            }
        }
    } else {
        audioIOBufferInternal->audioIOBufferDelayedReal = audioIOBufferReal;
        audioIOBufferInternal->audioIOBufferDelayedImag = audioIOBufferImag;
        audioIOBufferInternal->audioIOBufferReal        = audioIOBufferReal;
        audioIOBufferInternal->audioIOBufferImag        = audioIOBufferImag;
    }
    
    return 0;
}

int
retrieveAudioIOBufferTime(float*         audioIOBuffer[],
                          AudioIOBuffer* audioIOBufferInternal)
{
    int i,j;
    
    if (audioIOBufferInternal->audioDelaySamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            for (j=0; j<audioIOBufferInternal->audioFrameSize; j++) {
                audioIOBuffer[i][j] = audioIOBufferInternal->audioIOBufferDelayed[i][j];
            }
        }
    }
    
    return 0;
}

int
retrieveAudioIOBufferFreq(float*         audioIOBufferReal[],
                          float*         audioIOBufferImag[],
                          AudioIOBuffer* audioIOBufferInternal)
{
    int i,j;
    
    if (audioIOBufferInternal->audioDelaySubBandSamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            for (j=0; j<audioIOBufferInternal->audioSubBandFrameSize*audioIOBufferInternal->audioSubBandCount; j++) {
                audioIOBufferReal[i][j] = audioIOBufferInternal->audioIOBufferDelayedReal[i][audioIOBufferInternal->audioSubBandCount+j];
                audioIOBufferImag[i][j] = audioIOBufferInternal->audioIOBufferDelayedImag[i][audioIOBufferInternal->audioSubBandCount+j];
            }
        }
    }
    
    return 0;
}

int
advanceAudioIOBufferTime(AudioIOBuffer* audioIOBufferInternal)
{
    int i;
    if (audioIOBufferInternal->audioDelaySamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            memmove(audioIOBufferInternal->audioIOBufferDelayed[i], &audioIOBufferInternal->audioIOBufferDelayed[i][audioIOBufferInternal->audioFrameSize],sizeof(float) * audioIOBufferInternal->audioDelaySamples);
        }
    }

    return 0;
}

int
advanceAudioIOBufferFreq(AudioIOBuffer* audioIOBufferInternal)
{
    int i;
    if (audioIOBufferInternal->audioDelaySubBandSamples) {
        for (i=0; i<audioIOBufferInternal->audioChannelCount; i++) {
            memmove(audioIOBufferInternal->audioIOBufferDelayedReal[i], &audioIOBufferInternal->audioIOBufferDelayedReal[i][audioIOBufferInternal->audioSubBandFrameSize * audioIOBufferInternal->audioSubBandCount],sizeof(float) * audioIOBufferInternal->audioDelaySubBandSamples * audioIOBufferInternal->audioSubBandCount);
            memmove(audioIOBufferInternal->audioIOBufferDelayedImag[i], &audioIOBufferInternal->audioIOBufferDelayedImag[i][audioIOBufferInternal->audioSubBandFrameSize * audioIOBufferInternal->audioSubBandCount],sizeof(float) * audioIOBufferInternal->audioDelaySubBandSamples * audioIOBufferInternal->audioSubBandCount);
        }
    }
    return 0;
}
#endif /* AMD1_SYNTAX */

