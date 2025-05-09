/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp, University of Erlangen

Initial author:
Bernhard Grill (Fraunhofer IIS)

and edited by
Takashi Koike  (Sony Corporation)
Olaf Kaehler   (Fraunhofer IIS-A)

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
$Id: usac_main.h,v 1.5 2011-04-27 20:54:08 mul Exp $
*******************************************************************************/

/* 28-Aug-1996  NI: added "NO_SYNCWORD" to enum TRANSPORT_STREAM */

#ifndef _USAC_H_INCLUDED
#define _USAC_H_INCLUDED


#include "block.h"
#include "ntt_conf.h"
#include "usac_mainStruct.h"       /* structs */
#include "td_frame.h"

void usacBuffer2freq( double           p_in_data[],      /* Input: Time signal              */
                  double           p_out_mdct[],     /* Output: MDCT cofficients        */
                  double           p_overlap[],
                  WINDOW_SEQUENCE  windowSequence,
                  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                  WINDOW_SHAPE     wfun_select_prev,
                  int              nlong,            /* shift length for long windows   */
                  int              nshort,           /* shift length for short windows  */
                  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                  int              num_short_win);   /* number of short windows to
                                                        transform                       */


void usac_imdct_float(float in_data[], float out_data[], int len);
void usac_tw_imdct(float in_data[], float out_data[], int len);
void calc_window( float window[], int len, WINDOW_SHAPE wfun_select);
void calc_window_ratio( float window[], int len, int prev_len, WINDOW_SHAPE wfun_select, WINDOW_SHAPE prev_wfun_select);
void calc_tw_window( float window[], int len, WINDOW_SHAPE wfun_select);

void fd2buffer( float           p_in_data[],      /* Input: MDCT coefficients                */
                float           p_out_data[],     /* Output:time domain reconstructed signal */
                float           p_overlap[],
                WINDOW_SEQUENCE  windowSequence,
                WINDOW_SEQUENCE  windowSequenceLast,
                int              nlong,            /* shift length for long windows   */
                int              nshort,           /* shift length for short windows  */
                WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                WINDOW_SHAPE     wfun_select_prev, /* YB : 971113 */
                Imdct_out        overlap_select,   /* select imdct output *TK*        */
                int              num_short_win,    /* number of short windows to  transform */
                int              FrameWasTd ,
                int             twMdct,
                float            sample_pos[],
                float            tw_trans_len[],
                int              tw_start_stop[],
                float           prev_sample_pos[],
                float           prev_tw_trans_len[],
                int             prev_tw_start_stop[],
                float           prev_warped_time_sample_vector[],
                int ApplyFac,
                int facPrm[],
                float lastLPC[],
                float acelpZIR[],
                HANDLE_USAC_TD_DECODER    st,
                HANDLE_RESILIENCE hResilience,
                HANDLE_ESC_INSTANCE_DATA hEscInstanceData);

void  TDAliasing( float                 inputData[],
                  float                 Output[],
                  int              		nlong,
                  int              		nshort,
                  int                   pos);

void
usac_tns_decode_subblock(float *spec, int nbands, const short *sfb_top, int islong,
  TNSinfo *tns_info, QC_MOD_SELECT qc_select,int samplFreqIdx);


/* functions from tvqAUDecode.e */


#endif  /* #ifndef _TF_MAIN_H_INCLUDED */
