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
 
#ifndef drcToolEncoder_h
#define drcToolEncoder_h

#if defined __cplusplus
extern "C" {
#endif

#include "common.h"
#include "uniDrc.h"

struct sDrcEncParams;
typedef struct sDrcEncParams DrcEncParams, *HANDLE_DRC_ENCODER;
 
int
openDrcToolEncoder(HANDLE_DRC_ENCODER* phDrcEnc, 
                   UniDrcConfig uniDrcConfig,
                   LoudnessInfoSet loudnessInfoSet,
                   const int frameSize,
                   const int sampleRate,
                   const int delayMode,
                   const int domain);

int
closeDrcToolEncoder(HANDLE_DRC_ENCODER* phDrcEnc);

int
getNSequences(HANDLE_DRC_ENCODER hDrcEnc);

int
getDeltaTmin (const int sampleRate);

int
encodeUniDrcGain(HANDLE_DRC_ENCODER hDrcEnc,
                 float** gainBuffer);             /* 2D buffer [sequence][sample] containing the input gains in dB */

int
writeUniDrcConfig(HANDLE_DRC_ENCODER hDrcEnc,
                  unsigned char* bitstreamBuffer,
                  int* bitCount);

int
writeLoudnessInfoSet(HANDLE_DRC_ENCODER hDrcEnc,
                     unsigned char* bitstreamBuffer,
                     int* bitCount);
    
#ifdef DEBUG_OUTPUT_FORMAT
int
writeLoudnessInfoStruct(LoudnessInfo *loudnessInfo,
                           unsigned char* bitstreamBuffer,
                           int* bitCount);
#endif

int
writeUniDrcGain(HANDLE_DRC_ENCODER hDrcEnc,
                UniDrcGainExt uniDrcGainExtension,
                unsigned char* bitstreamBuffer,
                int* bitCount);

/* after calling encodeUniDrcGain,
   either call writeUniDrcConfig, writeLoudnessInfoSet and writeUniDrcGain separately, 
   or call writeUniDrc. */
int
writeUniDrc(HANDLE_DRC_ENCODER hDrcEnc,
            const int includeLoudnessInfoSet,
            const int includeUniDrcConfig,
            UniDrcGainExt uniDrcGainExtension,
            unsigned char* bitstreamBuffer,
            int* bitCount);

#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX

/* Writer for ISO base media file format DRC config */
int
writeIsobmffDrc(HANDLE_DRC_ENCODER hDrcEnc,
                unsigned char* bitstreamBuffer,
                int* bitCount);

/* Writer for ISO base media file format loudness */
int
writeIsobmffLoudness(HANDLE_DRC_ENCODER hDrcEnc,
                     unsigned char* bitstreamBuffer,
                     int* bitCount);
#endif
#endif
#endif

#if defined __cplusplus
}
#endif
#endif
