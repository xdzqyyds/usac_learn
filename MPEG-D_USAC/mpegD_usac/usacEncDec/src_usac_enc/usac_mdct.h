/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Stefan Bayer    (Fraunhofer IIS)

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
$Id: usac_mdct.h,v 1.2 2010-03-19 19:37:24 mul Exp $
*******************************************************************************/


#ifndef USAC_MDCT_H_
#define USAC_MDCT_H_

void calc_window_db( double window[], int len, WINDOW_SHAPE wfun_select);

void buffer2fd(
  double           p_in_data[],
  double           p_out_mdct[],
  double           p_overlap[],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     wfun_select_prev,
  int              nlong,            /* shift length for long windows   */
  int              nshort,           /* shift length for short windows  */
  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                                     /* switch (overlap_select) {       */
                                     /* case OVERLAPPED_MODE:                */
                                     /*   p_in_data[]                   */
                                     /*   = overlapped signal           */
                                     /*         (bufferlength: nlong)   */
                                     /* case NON_OVERLAPPED_MODE:            */
                                     /*   p_in_data[]                   */
                                     /*   = non overlapped signal       */
                                     /*         (bufferlength: 2*nlong) */
  int              previousMode,
  int              nextMode,
  int              num_short_win,     /* number of short windows per frame */
  int              twMdct,            /* TW-MDCT flag */
  float            sample_pos[],      /* TW sample positions */
  float            tw_trans_len[],    /* TW transition lengths */
  int              tw_start_stop[]    /* TW window borders */
);
#endif /* USAC_MDCT_H_ */
