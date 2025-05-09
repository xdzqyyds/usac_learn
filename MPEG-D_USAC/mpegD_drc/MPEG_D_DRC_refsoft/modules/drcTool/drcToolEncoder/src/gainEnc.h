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

#ifndef DrcEnc_gainEnc_h
#define DrcEnc_gainEnc_h

#include "tables.h"

#define N_UNIDRC_GAIN_MAX NODE_COUNT_MAX
#define N_UNIDRC_GROUPS_MAX CHANNEL_GROUP_COUNT_MAX

#if defined __cplusplus
extern "C" {
#endif
    
    /* gain values of current frame to be applied in next frame */
    typedef struct {
        int   nGainValues;

        float* drcGainQuant;
        int*   gainCode;
        int*   gainCodeLength;
        float* slopeQuant;
        int*   slopeCodeIndex;
        int*   tsGainQuant;
        int*   timeDeltaQuant;

        float* drcGain;
        float* slope;
        int*   tsGain;

        float gainPrevNode;
        float drcGainQuantPrev;

    } DrcGroup;
    
    /* gain values in decoder to be applied in current frame */
    /* For interpolation, the previous and next gain value is also present */
    /* Each gain value has a time stamp relative to the beginning of the current frame */
    typedef struct {
        int   nGainValues;

#ifdef NODE_RESERVOIR
        int    nGainValuesReservoir;
        float *drcGainQuantReservoir;
        int   *slopeCodeIndexBufReservoir;
        int   *tsGainQuantReservoir;
        bool   frameEndFlag;
#endif

        float* drcGainQuant;
        int*   gainCode;
        int*   gainCodeLength;
        float* slopeQuant;
        int*   slopeCodeIndex;
        int*   tsGainQuant;
        int*   timeDeltaQuant;

        int*   timeDeltaCode;
        int*   timeDeltaCodeIndex;
        int*   timeDeltaCodeSize;
        int*   slopeCode;
        int*   slopeCodeSize;

        float drcGainQuantPrev;
        float drcGainQuantNext;
        int   timeQuantNext;
        int   timeQuantPrev;
        int   slopeCodeIndexNext;
        int   slopeCodeIndexPrev;

        int   codingMode;
    } DrcGroupForOutput;

    typedef struct {
        GainSetParams gainSetParams;
        DrcGroup drcGroup;
        DrcGroupForOutput drcGroupForOutput;
    } DrcGainSeqBuf;

    /* Major DRC encoder parameters */
    struct sDrcEncParams {
        int nSequences;
        int deltaTmin;
        int deltaTminDefault;
        int drcFrameSize;
        int sampleRate;
        int delayMode;
        int domain;
        int baseChannelCount;               /* audio channel count before downmix */
        UniDrcConfig uniDrcConfig;
        LoudnessInfoSet loudnessInfoSet;
        float** drcGainPerSampleWithPrevFrame;
        DrcGainSeqBuf* drcGainSeqBuf;
        DeltaTimeCodeTableEntry* deltaTimeCodeTable;
        int* deltaTimeQuantTable;
    };

    float
    limitDrcGain(const int gainCodingProfile, const float gain);
    
    void
    getQuantizedInitialDrcGain(const float initialGainValue,
                               float* initialGainValueQuant,
                               int* nBits,
                               int* code);

    void
    getQuantizedDeltaDrcGain(const int gainCodingProfile,
                             const float deltaGain,
                             float *deltaGainQuant,
                             int* nBits,
                             int* code);

    void
    quantizeAndEncodeDrcGain(DrcEncParams* drcParams,
                             const float* drcGainPerSample,
                             float* drcGainPerSampleWithPrevFrame,
                             DeltaTimeCodeTableEntry* deltaTimeCodeTable,
                             DrcGainSeqBuf* drcGainSeqBuf);

    void
    checkOvershoot(const int tGainStep,
                   const float gain0,
                   const float gain1,
                   const float slope0,
                   const float slope1,
                   const int timeDeltaMin,
                   bool *overshootLeft,
                   bool *overshootRight);
    void
    quantizeSlope (const float slope,
                   float* slopeQuant,
                   int* slopeCodeIndex);

    void
    getPreliminaryNodes(const DrcEncParams* drcParams,
                        const float* drcGainPerSample,
                        float* drcGainPerSampleWithPrevFrame,
                        DrcGroup* drcGroup,
                        const int fullFrame);
    
    void
    quantizeDrcFrame(const int drcFrameSize,
                     const int timeDeltaMin,
                     const int nGainValuesMax,
                     const float* drcGainPerSample,
                     const int* deltaTimeQuantTable,
                     const int gainCodingProfile,
                     DrcGroup* drcGroup,
                     DrcGroupForOutput* drcGroupForOutput);
    
    void
    advanceNodes(DrcEncParams* drcParams,
                 DrcGainSeqBuf* drcGainSeqBuf);
    
    void
    postProcessNodes (DrcEncParams* drcParams,
                      DeltaTimeCodeTableEntry* deltaTimeCodeTable,
                      DrcGainSeqBuf* drcGainSeqBuf);


#if defined __cplusplus
}
#endif /* #if defined __cplusplus */
#endif
