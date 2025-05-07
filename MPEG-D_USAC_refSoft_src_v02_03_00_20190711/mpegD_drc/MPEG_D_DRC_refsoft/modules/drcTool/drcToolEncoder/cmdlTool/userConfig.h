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

#ifndef DrcEnc_userConfig_h
#define DrcEnc_userConfig_h

#include "drcToolEncoder.h"

typedef struct {
    int frameSize;
    int sampleRate;
    int delayMode;
    int domain;
#if AMD1_SYNTAX
    int parametricDrcOnly;
    int frameCount;
#endif
    int gainSequencePresent;
} EncParams;

#if !MPEG_H_SYNTAX

int configureEnc1   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc1_ld(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc2   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc3   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc4   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc5   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc6   (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
#if AMD1_SYNTAX

int configureEnc1_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc2_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc3_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc4_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc5_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc6_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc7_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc8_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc9_Amd1 (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc10_Amd1(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc11_Amd1(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc12_Amd1(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc13_Amd1(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEnc14_Amd1(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);

#endif /* AMD1_SYNTAX */

#else /* MPEG_H_SYNTAX */

int configureEncMpegh1    (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEncMpegh2    (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEncMpegh3    (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEncMpegh4    (EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int configureEncMpeghDebug(EncParams* encParams, UniDrcConfig* encConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);

#endif /* MPEG_H_SYNTAX */
#endif
