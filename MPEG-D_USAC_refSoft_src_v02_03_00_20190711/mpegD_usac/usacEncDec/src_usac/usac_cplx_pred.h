/*******************************************************************************
This software module was originally developed by

Dolby Laboratories, Fraunhofer IIS

and edited by


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
Fraunhofer IIS, Dolby Laboratories grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, Dolby Laboratories
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, Dolby Laboratories
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, Dolby Laboratories retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: $
*******************************************************************************/
#ifndef _usac_cplx_pred_h_
#define _usac_cplx_pred_h_

int usac_get_cplx_pred_data(int **alpha_q_re,
                            int **alpha_q_im,
                            int *alpha_q_re_prev,
                            int *alpha_q_im_prev,
                            int **cplx_pred_used,
                            int *pred_dir,
                            int *complex_coef,
                            int *use_prev_frame,
                            int num_window_groups,
                            int max_sfb_ste,
                            int const bUsacIndependenceFlag,
                            HANDLE_RESILIENCE hResilience,
                            HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                            HANDLE_BUFFER hVm);

int usac_decode_cplx_pred(Info *info,
                          byte *group,
                          float *specL,
                          float *specR,
                          float *dmx_re_prev,
                          const int **alpha_q_re,
                          const int **alpha_q_im,
                          const int **cplx_pred_used,
                          int pred_dir,
                          int complex_coef,
                          int use_prev_frame,
                          WINDOW_SEQUENCE win,
                          WINDOW_SHAPE winShape,
                          WINDOW_SHAPE winShapePrev);

void usac_cplx_pred_prev_dmx(Info *info,
                             float *specL,
                             float *specR,
                             float *dmx_re_prev,
                             int pred_dir);


#ifndef USAC_REFSOFT_COR1_NOFIX_03
void usac_cplx_save_prev(Info *info,
                         float *specL,
                         float *specR,
                         float *specLsave,
                         float *specRsave);
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */

#endif
