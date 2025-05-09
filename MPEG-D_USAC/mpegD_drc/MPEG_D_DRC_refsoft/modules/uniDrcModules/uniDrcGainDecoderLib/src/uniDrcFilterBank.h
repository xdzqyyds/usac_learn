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

#ifndef _UNI_DRC_FILTER_BANK_H_
#define _UNI_DRC_FILTER_BANK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define FILTER_BANK_PARAMETER_COUNT 16
#define CASCADE_ALLPASS_COUNT_MAX   9 /* for a maximum of 3 different 4-band LR filters in one DRC set */

typedef struct {
    float fCrossNorm;
    float gamma;
    float delta;
} FilterBankParams;

typedef struct {
    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
} FilterCoefficients;

typedef struct {
    float s00;
    float s01;
    float s10;
    float s11;
} LrFilterState;


typedef struct {
    float s0;
    float s1;
} AllPassFilterState;

typedef struct {
    FilterCoefficients lowPass;
    LrFilterState lowPassState[CHANNEL_COUNT_MAX];
    FilterCoefficients highPass;
    LrFilterState highPassState[CHANNEL_COUNT_MAX];
} TwoBandBank;

typedef struct {
    FilterCoefficients lowPassStage1;
    LrFilterState lowPassStage1State[CHANNEL_COUNT_MAX];
    FilterCoefficients highPassStage1;
    LrFilterState highPassStage1State[CHANNEL_COUNT_MAX];
    
    FilterCoefficients lowPassStage2;
    LrFilterState lowPassStage2State[CHANNEL_COUNT_MAX];
    FilterCoefficients highPassStage2;
    LrFilterState highPassStage2State[CHANNEL_COUNT_MAX];
    FilterCoefficients allPassStage2;
    AllPassFilterState allPassStage2State[CHANNEL_COUNT_MAX];
} ThreeBandBank;

typedef struct {
    FilterCoefficients lowPassStage1;
    LrFilterState lowPassStage1State[CHANNEL_COUNT_MAX];
    FilterCoefficients highPassStage1;
    LrFilterState highPassStage1State[CHANNEL_COUNT_MAX];
    
    FilterCoefficients allPassStage2High;
    AllPassFilterState allPassStage2HighState[CHANNEL_COUNT_MAX];
    FilterCoefficients allPassStage2Low;
    AllPassFilterState allPassStage2LowState[CHANNEL_COUNT_MAX];
    
    FilterCoefficients lowPassStage3High;
    LrFilterState lowPassStage3HighState[CHANNEL_COUNT_MAX];
    FilterCoefficients highPassStage3High;
    LrFilterState highPassStage3HighState[CHANNEL_COUNT_MAX];
    FilterCoefficients lowPassStage3Low;
    LrFilterState lowPassStage3LowState[CHANNEL_COUNT_MAX];
    FilterCoefficients highPassStage3Low;
    LrFilterState highPassStage3LowState[CHANNEL_COUNT_MAX];
} FourBandBank;

typedef struct {
    FilterCoefficients allPassStage;
    AllPassFilterState allPassStageState[CHANNEL_COUNT_MAX];
}   AllPassCascadeFilter;
    
typedef struct {
    AllPassCascadeFilter allPassCascadeFilter[CASCADE_ALLPASS_COUNT_MAX];
    int filterCount;
}   AllPassCascade;
    
typedef struct {
    int nBands;
    int complexity;
    TwoBandBank*   twoBandBank;
    ThreeBandBank* threeBandBank;
    FourBandBank*  fourBandBank;
    AllPassCascade* allPassCascade;  /* all-pass filters appended to filter bank for phase alignment */
} DrcFilterBank;

typedef struct {
    int nFilterBanks;
    int nPhaseAlignmentChannelGroups;
    int complexity;
    DrcFilterBank* drcFilterBank;
} FilterBanks;

int
initLrFilter(const int crossoverFreqIndex,
             FilterCoefficients* lowPassCoeff,
             FilterCoefficients* highPassCoeff);

int
initAllPassFilter(const int crossoverFreqIndex,
                  FilterCoefficients* allPassCoeff);

int
initFilterBank(const int nBands,
               const GainParams* gainParams,
               DrcFilterBank* drcFilterBank);

int
initAllPassCascade(const int filterCount,
                   const int* crossoverFreqIndex,
                   DrcFilterBank* drcFilterBank);

int
initAllFilterBanks(const DrcCoefficientsUniDrc* drcCoefficientsUniDrc,
                   const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                   FilterBanks* filterBanks);


int
removeAllFilterBanks(FilterBanks* filterBanks);

int
runLrFilter (const FilterCoefficients* filterCoefficients,
             LrFilterState* lrFilterState,
             const int size,
             const float* audioIn,
             float* audioOut);

int
runAllPassFilter (const FilterCoefficients* filterCoefficients,
                  AllPassFilterState* allPassFilterState,
                  const int size,
                  const float* audioIn,
                  float* audioOut);

int
runTwoBandBank(TwoBandBank* twoBandBank,
               const int c,
               const int size,
               const float* audioIn,
               float* audioOut[]);

int
runThreeBandBank(ThreeBandBank* threeBandBank,
                 const int c,
                 const int size,
                 const float* audioIn,
                 float* audioOut[]);

int
runFourBandBank(FourBandBank* fourBandBank,
                const int c,
                const int size,
                const float* audioIn,
                float* audioOut[]);

int
runAllPassCascade(AllPassCascade *allPassCascade,
                  const int c,
                  const int size,
                  float* audioIo);

#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_FILTER_BANK_H_ */
