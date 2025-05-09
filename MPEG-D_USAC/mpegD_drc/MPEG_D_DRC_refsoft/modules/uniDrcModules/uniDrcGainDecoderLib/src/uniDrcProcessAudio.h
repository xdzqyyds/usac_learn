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

#ifndef _UNI_DRC_PROCESS_AUDIO_H_
#define _UNI_DRC_PROCESS_AUDIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    int multibandAudioSignalCount;
    int audioFrameSize;
    float** noninterleavedAudio;
} AudioBandBuffer;

#if AMD1_SYNTAX
typedef struct {
    int audioChannelCount;
    int audioFrameSize;
    int audioSubBandCount;
    int audioSubBandFrameSize;
    int audioDelaySamples;
    int audioDelaySubBandSamples;
    float** audioIOBufferDelayed;
    float** audioIOBufferDelayedReal;
    float** audioIOBufferDelayedImag;
    float** audioIOBuffer;
    float** audioIOBufferReal;
    float** audioIOBufferImag;
} AudioIOBuffer;
#endif /* AMD1_SYNTAX */
    
int
initProcessAudio (UniDrcConfig* uniDrcConfig,
                  DrcParams* drcParams,
#if !AMD1_SYNTAX
                  AudioBandBuffer* audioBandBuffer);
#else /* AMD1_SYNTAX */
                  AudioBandBuffer* audioBandBuffer,
                  AudioIOBuffer* audioIOBuffer);
#endif
    
int
#if !AMD1_SYNTAX
removeProcessAudio(AudioBandBuffer* audioBandBuffer);
#else /* AMD1_SYNTAX */
removeProcessAudio(AudioBandBuffer* audioBandBuffer,
                   const int        subBandDomainMode,
                   AudioIOBuffer*   audioIOBuffer);
#endif

int
applyGains(DrcInstructionsUniDrc* drcInstructionsArray,
           const int drcInstructionsIndex,
           DrcParams* drcParams,
           GainBuffer* gainBuffer,
#if MPEG_D_DRC_EXTENSION_V1
           ShapeFilterBlock* shapeFilterBlock,
#endif /* MPEG_D_DRC_EXTENSION_V1 */
           float* deinterleavedAudio[],
           int applyGains);

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
           float* deinterleavedAudioImag[]);

int
addDrcBandAudio(DrcInstructionsUniDrc* drcInstructionsArray,
                const int drcInstructionsIndex,
                DrcParams* drcParams,
                AudioBandBuffer* audioBandBuffer,
                float* audioIoBuffer[]);

int
applyFilterBanks(DrcInstructionsUniDrc* drcInstructionsArray,
                 const int drcInstructionsIndex,
                 DrcParams* drcParams,
                 float* audioIoBuffer[],
                 AudioBandBuffer* audioBandBuffer,
                 FilterBanks* filterBanks,
                 const int passThru);

#if AMD1_SYNTAX
int
storeAudioIOBufferTime(float*         audioIOBuffer[],
                       AudioIOBuffer* audioIOBufferInternal);

int
storeAudioIOBufferFreq(float*         audioIOBufferReal[],
                       float*         audioIOBufferImag[],
                       AudioIOBuffer* audioIOBufferInternal);

int
retrieveAudioIOBufferTime(float*         audioIOBuffer[],
                          AudioIOBuffer* audioIOBufferInternal);
    
int
retrieveAudioIOBufferFreq(float*         audioIOBufferReal[],
                          float*         audioIOBufferImag[],
                          AudioIOBuffer* audioIOBufferInternal);
    
int
advanceAudioIOBufferTime(AudioIOBuffer* audioIOBufferInternal);

int
advanceAudioIOBufferFreq(AudioIOBuffer* audioIOBufferInternal);
#endif /* AMD1_SYNTAX */

#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_PROCESS_AUDIO_H_ */
