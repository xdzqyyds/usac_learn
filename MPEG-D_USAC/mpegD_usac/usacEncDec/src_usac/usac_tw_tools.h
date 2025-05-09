/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

Stefan Bayer               (Fraunhofer IIS)

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp.
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: usac_tw_tools.h,v 1.5 2011-02-24 10:03:24 frd Exp $
*******************************************************************************/


#ifndef USAC_TW_TOOLS_H_
#define USAC_TW_TOOLS_H_


int tw_resamp(const float *samplesIn,
              const int    startPos,
              int numSamplesIn,
              int numSamplesOut,
              const float *samplePos,
              float offsetPos,
              float *samplesOut);

int tw_calc_tw (int        tw_data_present,
                int        frameWasTd,
                int        *tw_ratio,
                WINDOW_SEQUENCE windowSequence,
                float      *warp_cont_mem,
                float      *sample_pos,
                float      *tw_trans_len,
                int        *tw_start_stop,
                float      *pitch_sum,
                int const mdctLen);

void tw_adjust_past ( WINDOW_SEQUENCE windowSequence,
                      WINDOW_SEQUENCE lastWindowSequence,
                      int frameWasTD,
                      int start_stop[],
                      float trans_len[],
                      int const mdctLen
                     );

void tw_init(void);

void tw_reset(const int  frame_len,
              float     *tw_cont_mem,
              float     *pitch_sum );

void tw_windowing_short(const float *wfIn,
                                 float *wfOut,
                                 int wStart,
                                 int wEnd,
                                 float warpedTransLenLeft,
                                 float warpedTransLenRight,
                                 const float *mdctWinTransLeft,
                                 const float *mdctWinTransRight,
                                 int const mdctLenShort);

void tw_windowing_long(const float *wfIn,
                                 float *wfOut,
                                 int wStart,
                                 int wEnd,
                                 int nlong,
                                 float warpedTransLenLeft,
                                 float warpedTransLenRight,
                                 const float *mdctWinTransLeft,
                                 const float *mdctWinTransRight);

void tw_windowing_past(const float *wfIn,
                       float *wfOut,
                       int wEnd,
                       int nLong,
                       float warpedTransLenRight,
                       const float *mdctWinTransRight);

#endif /* USAC_TW_TOOLS_H_ */
