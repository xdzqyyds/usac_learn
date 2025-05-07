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

#ifndef _UNI_DRC_LOUD_EQ_H_
#define _UNI_DRC_LOUD_EQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "uniDrcGainDecoder_api.h"
#include "uniDrcGainDecoder.h"
#include "uniDrcInterface.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif
#if MPEG_D_DRC_EXTENSION_V1

#define LOUD_EQ_REF_LEVEL       100.0f
#define LOUD_EQ_BAND_COUNT_MAX  2
    
typedef struct T_LOUDNESS_EQ_STRUCT{
    /* Parameters */
    int isActive;
    float mixingLevel;
    float playbackLevel[LOUD_EQ_BAND_COUNT_MAX];
    int drcFrameSize;
    int deltaTminDefault;
    LoudnessEqParameterInterface loudnessEqParameters;
    GainBuffer loudEqGainBuffer;
    int loudEqInstructionsSelectedIndex;
    
    /* Add loudness EQ here */

} LoudnessEq;
    
#endif  /* MPEG_D_DRC_EXTENSION_V1  */
    
#ifdef __cplusplus
}
#endif
#endif /* _UNI_DRC_LOUD_EQ_H_ */
