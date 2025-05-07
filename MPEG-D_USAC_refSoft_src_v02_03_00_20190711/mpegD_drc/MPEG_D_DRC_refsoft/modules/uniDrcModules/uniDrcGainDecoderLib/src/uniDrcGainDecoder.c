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
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "uniDrcGainDecoder.h"
#include "uniDrcTables.h"
#include "uniDrcCommon.h"

/***********************************************************************************/

int drcDecOpen(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS *phUniDrcGainDecStructs,
               HANDLE_UNI_DRC_CONFIG *phUniDrcConfig,
               HANDLE_LOUDNESS_INFO_SET *phLoudnessInfoSet,
               HANDLE_UNI_DRC_GAIN *phUniDrcGain)
{
    int err = 0;
    UNI_DRC_GAIN_DEC_STRUCTS *uniDrcGainDecStructs = NULL;
    
    /* allocate structs */
    uniDrcGainDecStructs          = (UNI_DRC_GAIN_DEC_STRUCTS*) calloc(1, sizeof(struct T_UNI_DRC_GAIN_DEC_STRUCTS));
    
    /* return allocated structs */
    *phUniDrcGainDecStructs       = uniDrcGainDecStructs;
    
    
    if ( *phUniDrcConfig == NULL)
    {
        UniDrcConfig *hUniDrcConfig = NULL;
        hUniDrcConfig = (HANDLE_UNI_DRC_CONFIG)calloc(1,sizeof(struct T_UNI_DRC_CONFIG_STRUCT));
        
        *phUniDrcConfig = hUniDrcConfig;
    }
    
    if ( *phLoudnessInfoSet == NULL)
    {
        LoudnessInfoSet *hLoudnessInfoSet = NULL;
        hLoudnessInfoSet = (HANDLE_LOUDNESS_INFO_SET)calloc(1,sizeof(struct T_LOUDNESS_INFO_SET_STRUCT));
        
        *phLoudnessInfoSet = hLoudnessInfoSet;
    }
    if ( *phUniDrcGain == NULL)
    {
        UniDrcGain *hUniDrcGain = NULL;
        hUniDrcGain = (HANDLE_UNI_DRC_GAIN)calloc(1,sizeof(struct T_UNI_DRC_GAIN_STRUCT));
        
        *phUniDrcGain = hUniDrcGain;
    }

    return err;
}

/***********************************************************************************/

int drcDecInit(   const int                        audioFrameSize,
                  const int                        audioSampleRate,
                  const int                        gainDelaySamples,
                  const int                        delayMode,
                  const int                        subBandDomainMode,
                  HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs)
{
    int err = 0;

    err = initDrcParams(audioFrameSize,
                        audioSampleRate,
                        gainDelaySamples,
                        delayMode,
                        subBandDomainMode,
                        &hUniDrcGainDecStructs->drcParams);
    if (err) return (err);
    
#if AMD1_SYNTAX
    initParametricDrc(hUniDrcGainDecStructs->drcParams.drcFrameSize,
                      audioSampleRate,
                      subBandDomainMode,
                      &hUniDrcGainDecStructs->parametricDrcParams);
    if (err) return (err);
#endif /* AMD1_SYNTAX */
    
    return err;
}

/***********************************************************************************/

int drcDecInitAfterConfig(    const int                        audioChannelCount,
                              const int*                       drcSetIdProcessed,
                              const int*                       downmixIdProcessed,
                              const int                        numSetsProcessed,
#if MPEG_D_DRC_EXTENSION_V1
                              const int                        eqSetIdProcessed,
#endif
#if MPEG_H_SYNTAX
                              const int                        channelOffset,
                              const int                        numChannelsProcess,
#endif
                              HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                              HANDLE_UNI_DRC_CONFIG            hUniDrcConfig
#if AMD1_SYNTAX
                            , HANDLE_LOUDNESS_INFO_SET         hLoudnessInfoSet
#endif
                        )
{
    int k, err = 0;

    /* values initialized from here on need information about selected DRC set and decoded DRC config */
    for (k = 0; k < numSetsProcessed; k++)
    {
        err = initSelectedDrcSet(hUniDrcConfig,
                                 &hUniDrcGainDecStructs->drcParams,
#if AMD1_SYNTAX
                                 &hUniDrcGainDecStructs->parametricDrcParams,
#endif /* AMD1_SYNTAX */                       
                                 audioChannelCount,
                                 drcSetIdProcessed[k],
                                 downmixIdProcessed[k],
                                 &hUniDrcGainDecStructs->filterBanks,
                                 &hUniDrcGainDecStructs->overlapParams
#if MPEG_D_DRC_EXTENSION_V1
                                 , hUniDrcGainDecStructs->shapeFilterBlock
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                                 );
        if (err) return (err);
    }
    
    hUniDrcGainDecStructs->audioChannelCount = audioChannelCount;
#if AMD1_SYNTAX
    hUniDrcGainDecStructs->drcParams.audioDelaySamples = hUniDrcGainDecStructs->drcParams.parametricDrcDelay;
    if (hUniDrcConfig->uniDrcConfigExt.parametricDrcPresent) {
        err = initParametricDrcAfterConfig(hUniDrcConfig,
                                           hLoudnessInfoSet,
                                           &hUniDrcGainDecStructs->parametricDrcParams);
        if (err) return (err);
    }
    hUniDrcGainDecStructs->audioIOBuffer.audioChannelCount        = audioChannelCount;
    hUniDrcGainDecStructs->audioIOBuffer.audioDelaySamples        = hUniDrcGainDecStructs->drcParams.audioDelaySamples;
    hUniDrcGainDecStructs->audioIOBuffer.audioFrameSize           = hUniDrcGainDecStructs->drcParams.drcFrameSize;
    switch (hUniDrcGainDecStructs->drcParams.subBandDomainMode) {
        case SUBBAND_DOMAIN_MODE_QMF64:
            hUniDrcGainDecStructs->audioIOBuffer.audioDelaySubBandSamples = hUniDrcGainDecStructs->drcParams.audioDelaySamples / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandFrameSize    = hUniDrcGainDecStructs->drcParams.drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandCount        = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
            break;
        case SUBBAND_DOMAIN_MODE_QMF71:
            hUniDrcGainDecStructs->audioIOBuffer.audioDelaySubBandSamples = hUniDrcGainDecStructs->drcParams.audioDelaySamples / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandFrameSize    = hUniDrcGainDecStructs->drcParams.drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandCount        = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
            break;
        case SUBBAND_DOMAIN_MODE_STFT256:
            hUniDrcGainDecStructs->audioIOBuffer.audioDelaySubBandSamples = hUniDrcGainDecStructs->drcParams.audioDelaySamples / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandFrameSize    = hUniDrcGainDecStructs->drcParams.drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandCount        = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
            break;
        case SUBBAND_DOMAIN_MODE_OFF:
        default:
            hUniDrcGainDecStructs->audioIOBuffer.audioDelaySubBandSamples = 0;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandFrameSize    = 0;
            hUniDrcGainDecStructs->audioIOBuffer.audioSubBandCount        = 0;
            break;
    }
    
    /* TODO: add parametric DRC support for MPEG-H */
#endif /* AMD1_SYNTAX */

    err = initDrcGainBuffers(hUniDrcConfig,
                             &hUniDrcGainDecStructs->drcParams,
                             &hUniDrcGainDecStructs->drcGainBuffers);
    if (err) return (err);
    
    err = initProcessAudio (hUniDrcConfig,
                            &hUniDrcGainDecStructs->drcParams,
#if !AMD1_SYNTAX
                            &hUniDrcGainDecStructs->audioBandBuffer);
#else /* AMD1_SYNTAX */
                            &hUniDrcGainDecStructs->audioBandBuffer,
                            &hUniDrcGainDecStructs->audioIOBuffer);
#endif
    if (err) return (err);
    
#if MPEG_D_DRC_EXTENSION_V1
    if (eqSetIdProcessed > 0)
    {
        for(k=0; k<hUniDrcConfig->uniDrcConfigExt.eqInstructionsCount; k++)
        {
            if (hUniDrcConfig->uniDrcConfigExt.eqInstructions[k].eqSetId == eqSetIdProcessed) break;
        }
        if (k==hUniDrcConfig->uniDrcConfigExt.eqInstructionsCount) {
            fprintf(stderr, "EQ set with matching ID not found  %d\n", eqSetIdProcessed);
            return UNEXPECTED_ERROR;
        }

        err = createEqSet(&(hUniDrcGainDecStructs->eqSet));
        if (err) return (err);

        err = deriveEqSet (&hUniDrcConfig->uniDrcConfigExt.eqCoefficients,
                           &(hUniDrcConfig->uniDrcConfigExt.eqInstructions[k]),
                           hUniDrcGainDecStructs->drcParams.audioSampleRate,
                           hUniDrcGainDecStructs->drcParams.drcFrameSize,
                           hUniDrcGainDecStructs->drcParams.subBandDomainMode,
                           hUniDrcGainDecStructs->eqSet);
        if (err) return (err);
        
        err = getEqSetDelay (hUniDrcGainDecStructs->eqSet,
                             &hUniDrcGainDecStructs->drcParams.eqDelay);
        if (err) return (err);
    }
    else
    {
        err = removeEqSet(&(hUniDrcGainDecStructs->eqSet));
        if (err) return (err);
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 */
#if MPEG_H_SYNTAX
    if (channelOffset != -1 && numChannelsProcess != -1)
    {
        hUniDrcGainDecStructs->drcParams.channelOffset = channelOffset;
        hUniDrcGainDecStructs->drcParams.numChannelsProcess = numChannelsProcess;
    }
    else
    {
        hUniDrcGainDecStructs->drcParams.channelOffset = 0;
        hUniDrcGainDecStructs->drcParams.numChannelsProcess = -1;
    }
#endif /* MPEG_H_SYNTAX */

    return err;
}

/***********************************************************************************/

int drcDecClose( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS *phUniDrcGainDecStructs,
                 HANDLE_UNI_DRC_CONFIG *phUniDrcConfig,
                 HANDLE_LOUDNESS_INFO_SET *phLoudnessInfoSet,
                 HANDLE_UNI_DRC_GAIN *phUniDrcGain
#if MPEG_D_DRC_EXTENSION_V1
                , HANDLE_LOUDNESS_EQ_STRUCT* hLoudnessEq
#endif
                )
{
    int err = 0;

    err = removeTables();
    if (err) return (err);
#if !AMD1_SYNTAX
    err = removeProcessAudio(&(*phUniDrcGainDecStructs)->audioBandBuffer);
#else /* AMD1_SYNTAX */
    err = removeProcessAudio(&(*phUniDrcGainDecStructs)->audioBandBuffer, (*phUniDrcGainDecStructs)->drcParams.subBandDomainMode, &(*phUniDrcGainDecStructs)->audioIOBuffer);
#endif
    if (err) return (err);
    err = removeDrcGainBuffers(&(*phUniDrcGainDecStructs)->drcParams, &(*phUniDrcGainDecStructs)->drcGainBuffers);
    if (err) return (err);
    err = removeAllFilterBanks(&(*phUniDrcGainDecStructs)->filterBanks);
    if (err) return (err);
    err = removeOverlapWeights(&(*phUniDrcGainDecStructs)->overlapParams);
    if (err) return (err);
#if AMD1_SYNTAX
    err = removeParametricDrc(&(*phUniDrcGainDecStructs)->parametricDrcParams);
    if (err) return (err);
#endif

    if (*phUniDrcGainDecStructs != NULL)
    {
#if MPEG_D_DRC_EXTENSION_V1
        err = removeEqSet(&(*phUniDrcGainDecStructs)->eqSet);
        if (err) return (err);
#endif
        free(*phUniDrcGainDecStructs);
        *phUniDrcGainDecStructs = NULL;
    }
    if ( *phUniDrcConfig != NULL )
    {
        free( *phUniDrcConfig );
        *phUniDrcConfig = NULL;
    }
    if ( *phLoudnessInfoSet != NULL )
    {
        free( *phLoudnessInfoSet );
        *phLoudnessInfoSet = NULL;
    }
    if ( *phUniDrcGain != NULL )
    {
        free( *phUniDrcGain );
        *phUniDrcGain = NULL;
    }
#if MPEG_D_DRC_EXTENSION_V1
    err = removeLoudEq(hLoudnessEq);
    if (err) return (err);
#endif

    return err;
}

/***********************************************************************************/

int drcProcessTime( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                    HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                    HANDLE_UNI_DRC_GAIN              hUniDrcGain,
                    float*                           audioIOBuffer[],
                    const float                      loudnessNormalizationGainDb,
                    const float                      boost,
                    const float                      compress,
                    const int                        drcCharacteristicTarget)
{
    int selDrcIndex, err = 0;
    int passThru;
    DrcInstructionsUniDrc* drcInstructionsUniDrc = hUniDrcConfig->drcInstructionsUniDrc;
    
#if MPEG_D_DRC_EXTENSION_V1
    if (hUniDrcGainDecStructs->eqSet)
    {
        int i, c;
        float* audioChannel;
        for (c=0; c<hUniDrcGainDecStructs->eqSet->audioChannelCount; c++)
        {
            audioChannel = audioIOBuffer[c];
            for (i=0; i<hUniDrcGainDecStructs->drcParams.drcFrameSize; i++)
            {
                err = processEqSetTimeDomain(hUniDrcGainDecStructs->eqSet, c, audioChannel[i], &audioChannel[i]);
                if (err) return (err);
            }
        }
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 */

#if AMD1_SYNTAX
    err = storeAudioIOBufferTime(audioIOBuffer,
                                 &hUniDrcGainDecStructs->audioIOBuffer);
    if (err) return (err);
#endif /* AMD1_SYNTAX */
    
    if (hUniDrcConfig->applyDrc)
    {        
        for (selDrcIndex=0; selDrcIndex<hUniDrcGainDecStructs->drcParams.drcSetCounter; selDrcIndex++)
        {          
            /* interpolate gains of sequences used */
            err = getDrcGain (hUniDrcGainDecStructs,
                              hUniDrcConfig,
                              hUniDrcGain,
                              compress,
                              boost,
                              drcCharacteristicTarget,
                              loudnessNormalizationGainDb,
                              selDrcIndex,
                              &hUniDrcGainDecStructs->drcGainBuffers);
            if (err) return (err);
        }
        
#if AMD1_SYNTAX
        /* delay line for constant audio delay of all uniDrcGainDecoder instances */
        if (hUniDrcGainDecStructs->drcParams.drcSetCounter == 0)
        {
            err = retrieveAudioIOBufferTime(audioIOBuffer,
                                            &hUniDrcGainDecStructs->audioIOBuffer);
           if (err) return (err);
        } else
#endif /* AMD1_SYNTAX */
        {

            for (selDrcIndex=0; selDrcIndex<hUniDrcGainDecStructs->drcParams.drcSetCounter; selDrcIndex++)
            {
#if AMD1_SYNTAX
#if PARAM_DRC_DEBUG
                {
                    int i,j;
                    for (i=0; i<hUniDrcGainDecStructs->audioIOBuffer.audioChannelCount; i++) {
                        for (j=0; j<hUniDrcGainDecStructs->audioIOBuffer.audioFrameSize; j++) {
                            hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayed[i][j] = 1.f;
                        }
                    }
                }
#endif
#endif /* AMD1_SYNTAX */
#if MPEG_H_SYNTAX
                {
                    DrcInstructionsUniDrc* drcInstructionsUniDrcTmp = &(drcInstructionsUniDrc[hUniDrcGainDecStructs->drcParams.selDrc[selDrcIndex].drcInstructionsIndex]);
                    if (hUniDrcGainDecStructs->drcParams.numChannelsProcess == -1)
                        hUniDrcGainDecStructs->drcParams.numChannelsProcess = drcInstructionsUniDrcTmp->audioChannelCount;
                    
                    if ((hUniDrcGainDecStructs->drcParams.channelOffset + hUniDrcGainDecStructs->drcParams.numChannelsProcess) > (drcInstructionsUniDrcTmp->audioChannelCount))
                        return -1;
                }
#endif
                /* filter bank will only be applied if a DRC sequence is multi band */
                if (hUniDrcGainDecStructs->drcParams.multiBandSelDrcIndex == selDrcIndex)
                {
                    passThru = FALSE;
                }
                else
                {
                    passThru = TRUE;
                }
                err = applyFilterBanks(drcInstructionsUniDrc,
                                       hUniDrcGainDecStructs->drcParams.selDrc[selDrcIndex].drcInstructionsIndex,
                                       &hUniDrcGainDecStructs->drcParams,
#if !AMD1_SYNTAX
                                       audioIOBuffer,
#else /* AMD1_SYNTAX */
                                       hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayed,
#endif
                                       &hUniDrcGainDecStructs->audioBandBuffer,
                                       &hUniDrcGainDecStructs->filterBanks,
                                       passThru);
                if (err) return (err);
                
                /* Apply DRC */
                err = applyGains(drcInstructionsUniDrc,
                                 hUniDrcGainDecStructs->drcParams.selDrc[selDrcIndex].drcInstructionsIndex,
                                 &hUniDrcGainDecStructs->drcParams,
                                 &(hUniDrcGainDecStructs->drcGainBuffers.gainBuffer[selDrcIndex]),
#if MPEG_D_DRC_EXTENSION_V1
                                 hUniDrcGainDecStructs->shapeFilterBlock,
#endif /* MPEG_D_DRC_EXTENSION_V1 */
                                 hUniDrcGainDecStructs->audioBandBuffer.noninterleavedAudio,
                                 1);
                if (err) return (err);
                
                err = addDrcBandAudio(drcInstructionsUniDrc,
                                      hUniDrcGainDecStructs->drcParams.selDrc[selDrcIndex].drcInstructionsIndex,
                                      &hUniDrcGainDecStructs->drcParams,
                                      &hUniDrcGainDecStructs->audioBandBuffer,
                                      audioIOBuffer);
                if (err) return (err);
            }
        }
    }
    
#if AMD1_SYNTAX
    err = advanceAudioIOBufferTime(&hUniDrcGainDecStructs->audioIOBuffer);
    if (err) return (err);
#endif /* AMD1_SYNTAX */

    return err;
}


/***********************************************************************************/

int drcProcessFreq( HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                    HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                    HANDLE_UNI_DRC_GAIN              hUniDrcGain,
                    float*                           audioIOBufferReal[],
                    float*                           audioIOBufferImag[],
                    const float                      loudnessNormalizationGainDb,
                    const float                      boost,
                    const float                      compress,
                    const int                        drcCharacteristicTarget)
{
    int selDrcIndex, err = 0;
    DrcInstructionsUniDrc* drcInstructionsUniDrc = hUniDrcConfig->drcInstructionsUniDrc;

#if MPEG_D_DRC_EXTENSION_V1
    if (hUniDrcGainDecStructs->eqSet)
    {
        int c;
 
        for (c=0; c<hUniDrcGainDecStructs->eqSet->audioChannelCount; c++)
        {
            err = processEqSetSubbandDomain(hUniDrcGainDecStructs->eqSet,
                                            c,
                                            audioIOBufferReal[c],
                                            audioIOBufferImag[c]);
            if (err) return (err);
        }
    }
#endif /* MPEG_D_DRC_EXTENSION_V1 */

#if AMD1_SYNTAX
    err = storeAudioIOBufferFreq(audioIOBufferReal,
                                 audioIOBufferImag,
                                 &hUniDrcGainDecStructs->audioIOBuffer);
    if (err) return (err);
#endif /* AMD1_SYNTAX */
    
    if (hUniDrcConfig->applyDrc)
    {
        for (selDrcIndex=0; selDrcIndex<hUniDrcGainDecStructs->drcParams.drcSetCounter; selDrcIndex++)
        {
#if AMD1_SYNTAX
#if PARAM_DRC_DEBUG
            {
                int i,j;
                for (i=0; i<hUniDrcGainDecStructs->audioIOBuffer.audioChannelCount; i++) {
                    for (j=0; j<hUniDrcGainDecStructs->audioIOBuffer.audioSubBandFrameSize*hUniDrcGainDecStructs->audioIOBuffer.audioSubBandCount; j++) {
                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayedReal[i][j] = 1.f;
                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayedImag[i][j] = 1.f;
                    }
                }
            }
#endif
#endif /* AMD1_SYNTAX */
            /* interpolate gains of sequences used */
            err = getDrcGain (hUniDrcGainDecStructs,
                              hUniDrcConfig,
                              hUniDrcGain,
                              compress,
                              boost,
                              drcCharacteristicTarget,
                              loudnessNormalizationGainDb,
                              selDrcIndex,
                              &hUniDrcGainDecStructs->drcGainBuffers);
            if (err) return (err);
        }

#if AMD1_SYNTAX
        /* delay line for constant audio delay of all uniDrcGainDecoder instances */
       if (hUniDrcGainDecStructs->drcParams.drcSetCounter == 0)
       {
           err = retrieveAudioIOBufferFreq(audioIOBufferReal,
                                           audioIOBufferImag,
                                           &hUniDrcGainDecStructs->audioIOBuffer);
           if (err) return (err);
       } else
#endif /* AMD1_SYNTAX */
        {
            for (selDrcIndex=0; selDrcIndex<hUniDrcGainDecStructs->drcParams.drcSetCounter; selDrcIndex++)
            {
                /* Apply DRC */
                err = applyGainsSubband(drcInstructionsUniDrc,
                                        hUniDrcGainDecStructs->drcParams.selDrc[selDrcIndex].drcInstructionsIndex,
                                        &hUniDrcGainDecStructs->drcParams,
                                        &(hUniDrcGainDecStructs->drcGainBuffers.gainBuffer[selDrcIndex]),
                                        &hUniDrcGainDecStructs->overlapParams,
#if AMD1_SYNTAX
                                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayedReal,
                                        hUniDrcGainDecStructs->audioIOBuffer.audioIOBufferDelayedImag,
#endif /* AMD1_SYNTAX */
                                        audioIOBufferReal,
                                        audioIOBufferImag);
                if (err) return (err);
            }
        }
    }
    
#if AMD1_SYNTAX
    err = advanceAudioIOBufferFreq(&hUniDrcGainDecStructs->audioIOBuffer);
#endif /* AMD1_SYNTAX */

    return err;
}

/***********************************************************************************/

#if AMD1_SYNTAX
/* get parametric DRC delay */
int
getParametricDrcDelay(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
                      HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
                      int* parametricDrcDelay,
                      int* parametricDrcDelayMax)
{
    int err = 0;
       
    *parametricDrcDelay = hUniDrcGainDecStructs->drcParams.parametricDrcDelay;
    
    if (hUniDrcConfig->uniDrcConfigExt.parametricDrcPresent && hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMaxPresent) {
        *parametricDrcDelayMax = hUniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.parametricDrcDelayMax;
    } else if (hUniDrcConfig->uniDrcConfigExt.parametricDrcPresent == 0) {
        *parametricDrcDelayMax = 0;
    } else {
        *parametricDrcDelayMax = -1;
    }

    return err;
}

/* get EQ delay */
int
getEqDelay(HANDLE_UNI_DRC_GAIN_DEC_STRUCTS  hUniDrcGainDecStructs,
           HANDLE_UNI_DRC_CONFIG            hUniDrcConfig,
           int* eqDelay,
           int* eqDelayMax)
{
    int err = 0;
    
    *eqDelay = hUniDrcGainDecStructs->drcParams.eqDelay;
    
    if (hUniDrcConfig->uniDrcConfigExt.eqPresent && hUniDrcConfig->uniDrcConfigExt.eqCoefficients.eqDelayMaxPresent) {
        *eqDelayMax = hUniDrcConfig->uniDrcConfigExt.eqCoefficients.eqDelayMax;
    } else if (hUniDrcConfig->uniDrcConfigExt.eqPresent == 0) {
        *eqDelayMax = 0;
    } else {
        *eqDelayMax = -1;
    }
    
    return err;
}

#if AMD2_COR2
int
getFilterBanksComplexity (HANDLE_UNI_DRC_CONFIG hUniDrcConfig,
                          int                   drcInstructionsV1Index,
                          float*                complexity)
{
    FilterBanks filterBanks;
    
    initAllFilterBanks(&hUniDrcConfig->drcCoefficientsUniDrc[0],
                       &(hUniDrcConfig->drcInstructionsUniDrc[drcInstructionsV1Index]),
                       &filterBanks);
    
    *complexity = COMPLEXITY_W_IIR * filterBanks.complexity; /* Qi: time-domain filter bank for all channels */
    removeAllFilterBanks(&filterBanks);
    return 0;
}
#endif

#endif

/***********************************************************************************/

