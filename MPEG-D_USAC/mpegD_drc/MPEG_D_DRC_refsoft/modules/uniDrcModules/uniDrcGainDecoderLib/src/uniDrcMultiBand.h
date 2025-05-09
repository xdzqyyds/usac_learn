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

#ifndef _UNI_DRC_MULTI_BAND_H_
#define _UNI_DRC_MULTI_BAND_H_

#ifndef AMD2_COR1
#define DRC_SUBBAND_COUNT_WITH_AUDIO_CODEC_FILTERBANK_MAX   FILTER_BANK_PARAMETER_COUNT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    float overlapWeight[AUDIO_CODEC_SUBBAND_COUNT_MAX];
} OverlapParamsForBand;

typedef struct {
    OverlapParamsForBand overlapParamsForBand[BAND_COUNT_MAX];
} OverlapParamsForGroup;

typedef struct {
    OverlapParamsForGroup overlapParamsForGroup[CHANNEL_GROUP_COUNT_MAX];
} OverlapParams;



int
initFcenterNormSb(const int nSubbands,
#ifdef AMD2_COR1
                  const int subBandDomainMode,
#endif
                  float* fCenterNormSb);

int
generateSlope (const int nSubbands,
               const float* fCenterNormSb,
               const float fCrossNormLo,
               const float fCrossNormHi,
               float* response);

int
generateOverlapWeights (const int nDrcBands,
                        const int drcBandType,
                        const GainParams* gainParams,
                        const int audioDecoderSubbandCount,
                        OverlapParamsForGroup* overlapParamsForGroup);

int
initOverlapWeight (const DrcCoefficientsUniDrc* drcCoefficientsUniDrc,
                   const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                   const int subBandDomainMode,
                   OverlapParams* overlapParams);

int
removeOverlapWeights (OverlapParams* overlapParams);

#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_MULTI_BAND_H_ */
