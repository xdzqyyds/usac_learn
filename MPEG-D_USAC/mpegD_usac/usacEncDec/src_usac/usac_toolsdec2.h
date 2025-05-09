/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Ralph Sperschneider (Fraunhofer IIS)

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
$Id: usac_toolsdec2.h,v 1.1.1.1 2009-05-29 14:10:21 mul Exp $
*******************************************************************************/


#ifndef _HUFFDEC2_H_
#define _HUFFDEC2_H_

#include "allHandles.h"
#include "all.h"
#include "block.h"
#include "tns.h"

#include "tf_mainStruct.h"       /* typedefs */

void deinterleave ( void                    *inptr,  /* formerly pointer to type int */
                    void                    *outptr, /* formerly pointer to type int */
                    short                   length,  /* sizeof base type of inptr and outptr in chars */
                    const short             nsubgroups[],
                    const int               ncells[],
                    const short             cellsize[],
                    int                     nsbk,
                    int                     blockSize,
                    const HANDLE_RESILIENCE hResilience,
                    short                   ngroups );

void          clr_tns ( Info*           info,
                        TNS_frame_info* tns_frame_info );

void          getgroup ( Info*             info,
                         unsigned char*    group,
                         HANDLE_RESILIENCE hResilience,
                         HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                         HANDLE_BUFFER     hVm  );

int usac_getics ( HANDLE_AACDECODER        hDec,
             Info*                    info,
             int                      common_window,
             WINDOW_SEQUENCE*         win,
             WINDOW_SHAPE* 	      wshape,
             unsigned char*           group,
             unsigned char*           max_sfb,
             PRED_TYPE                pred_type,
             int*                     lpflag,
             int*                     prstflag,
             byte*                    cb_map,
             double*                   coef,
             int                      max_spec_coefficients,
             short*                   global_gain,
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp,
             TNS_frame_info*          tns,
             HANDLE_BSBITSTREAM       gc_streamCh,
             enum AAC_BIT_STREAM_TYPE bitStreamType,
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_ESC_INSTANCE_DATA           hEPInfo,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT            qc_select,
             HANDLE_BUFFER            hVm,
             /* needful for ER_AAC_Scalable */
             int                      extensionLayerFlag,   /* signaled an extension layer */
             int                      er_channel            /* 0:left 1:right channel      */
#ifdef I2R_LOSSLESS
             , int*                   sls_quant_mono_temp
#endif
             );




unsigned short HuffSpecKernelPure ( int*                     qp,
                                    Hcb*                     hcb,
                                    Huffman*                 hcw,
                                    int                      step,
                                    HANDLE_HCR               hHcrInfo,
                                    HANDLE_RESILIENCE        hResilience,
                                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                    HANDLE_CONCEALMENT       hConcealment,
                                    HANDLE_BUFFER            hSpecData );

unsigned char Vcb11Used ( unsigned short );

#endif  /* #ifndef _HUFFDEC2_H_ */
