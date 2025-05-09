/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, and VoiceAge Corp.


Initial author:

and edited by:
Yoshiaki Oikawa     (Sony Corporation),
Mitsuyuki Hatanaka  (Sony Corporation),

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
$Id: usac_port.h,v 1.17.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/

#ifndef _usac_port_h_
#define _usac_port_h_

#include "usac_all.h"
#include "allHandles.h"
#include "block.h"
#include "tf_mainHandle.h"
#include "tns.h"
#include "usac_mainStruct.h"
#include "port.h"

#include "td_frame.h"


void usac_get_ics_info ( WINDOW_SEQUENCE*         win,
                         WINDOW_SHAPE*            wshape,
                         byte*                    group,
                         byte*                    max_sfb,
                         QC_MOD_SELECT            qc_select,
                         HANDLE_RESILIENCE        hResilience,
                         HANDLE_ESC_INSTANCE_DATA EPInfo,
                         HANDLE_BUFFER            hVm);

void calc_gsfb_table ( Info *info,
                       byte *group );

void usac_get_tns ( Info*             info,
                    TNS_frame_info*   tns_frame_info,
                    HANDLE_RESILIENCE hResilience,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_BUFFER     hVm );


void get_tw_data(int                     *tw_data_present,
                 int                     *tw_ratio,
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                 HANDLE_BUFFER            hVm
   );



int decodeChannelElement ( int                      id,
                           MC_Info*                 mip,
                           USAC_DATA               *usacData,
                           WINDOW_SEQUENCE*         win,
                           Wnd_Shape* 	            wshape,
                           byte**                   cb_map,
                           short**                  factors,
                           byte**                   group,
                           byte*                    hasmask,
                           byte**                   mask,
                           byte*                    max_sfb,
                           TNS_frame_info**         tns,
                           float**                  coef,
                           Info*                    sfbInfo,
                           QC_MOD_SELECT            qc_select,
                           HANDLE_DECODER_GENERAL   hFault,
                           int                      elementTag,
                           int                      bNoiseFilling,
                           int                     *channelOffset

#ifdef SONY_PVC_DEC
							,int *cm
#endif /* SONY_PVC_DEC */
#ifndef CPLX_PRED_NOFIX
                          ,int stereoConfigIndex                  
#endif /* CPLX_PRED_NOFIX */
                             );


void usac_past_tw( float 	   p_overlap[],
                   int             nlong,            /* shift length for long windows   */
                   int             nshort,           /* shift length for short windows  */
                   WINDOW_SHAPE    wfun_select,
                   WINDOW_SHAPE    prev_wfun_select,
                   WINDOW_SEQUENCE windowSequenceLast,
                   int             mod0,
                   int             frameWasTd,
                   int             twMdct,
                   float           past_sample_pos[],
                   float           past_tw_trans_len[],
                   int             past_tw_start_stop[],
                   float           past_warped_time_sample_vector[]);


void usac_td2buffer(float p_in_data[],
		    float p_out_data[],
		    float p_overlap[],
		    int              nlong,            /* shift length for long windows   */
		    int              nshort,           /* shift length for short windows  */
		    WINDOW_SEQUENCE windowSequenceLast,
		    int mod0,
		    int frameWasTd,
		    int twMdct,
                    float           prev_sample_pos[],
                    float           prev_tw_trans_len[],
                    int             prev_tw_start_stop[],
                    float           prev_warped_time_sample_vector[]);


int usac_get_tdcs ( HANDLE_USAC_TD_DECODER   hDec,
                    int                      *arithPreviousSize,
                    Tqi2                     arithQ[],
                    td_frame_data            *td,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_BUFFER            hVm,
                    int                      *isAceStart,          /* (o): indicator of first TD frame */
                    int                      *short_fac_flag,      /* (o): short fac flag */
                    int                      *isBassPostFilter,    /* (o): bassfilter on/off */
                    int const                bUsacIndependencyFlag);

void acelp_decoding(int                      codec_mode,
                    int                      k,
                    int                      nb_subfr,
                    td_frame_data            *td,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_BUFFER            hVm );

void tcx_decoding( HANDLE_USAC_TD_DECODER   hDec,
                   int 			    mode,
                   int*                     quant,
                   int 			    k,
                   int 			    last_mode,
                   int 			    first_tcx_flag,
                   int                      *arithPreviousSize,
                   Tqi2                     arithQ[],
                   td_frame_data            *td,
                   HANDLE_RESILIENCE        hResilience,
                   HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                   HANDLE_BUFFER            hVm,
                   int const                bUsacIndependencyFlag
);


WINDOW_SEQUENCE 
usacMapWindowSequences(WINDOW_SEQUENCE windowSequenceCurr, 
                       WINDOW_SEQUENCE windowSequenceLast);

int usac_get_fdcs ( HANDLE_USAC_DECODER      hDec,
                    Info*                    info,
                    int                      common_window,
                    int                      common_tw,
                    int                      tns_data_present,
                    WINDOW_SEQUENCE*         win,
                    WINDOW_SHAPE*            wshape,
                    byte*                    group,
                    byte*                    max_sfb,
                    byte*                    cb_map,
                    float*                   coef,
                    int                      max_spec_coefficients,
                    short*                   global_gain,
                    short*                   factors,
                    int                     *arithPreviousSize,
                    Tqi2                     arithQ[],
                    TNS_frame_info*          tns,
                    int*                     tw_data_present,
                    int*                     tw_ratio,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_BUFFER            hHcrSpecData,
                    HANDLE_HCR               hHcrInfo,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_CONCEALMENT       hConcealment,
                    QC_MOD_SELECT            qc_select,
                    unsigned int             *nfSeed,
                    HANDLE_BUFFER            hVm,
                    WINDOW_SEQUENCE  windowSequenceLast,
                    int              frameWasTD,
                    int              bUsacIndependencyFlag,
                    int ch,
                    int               bUseNoiseFilling
                    );

int ReadFacData( int         lfac,
                 int        *FacData,
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                 HANDLE_BUFFER            hVm
                 
);

unsigned long ReadCntBit ( HANDLE_BUFFER stream);

void usac_infoinit (USAC_SR_Info * sip, int block_size_samples);

#endif  /* _usac_port_h_ */

