/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#ifndef _huffdec_intern_h_
#define _huffdec_intern_h_

#include "sac_resdefs.h"
#include "sac_reshuffinit.h"
#include "sac_restns.h"

#ifdef MPEG4V1
#include "nok_lt_prediction.h"
#endif

void	s_get_sign_bits(int *q, int n);

int	s_getescape(int q);
Float	s_esc_iquant(int q);
int	s_decode_huff_cw(Huffman *h);
void	s_unpack_idx(int *qp, int idx, Hcb *hcb);

void	s_getgroup(Info *info, unsigned char* group);

void	s_get_ics_info(BLOCKTYPE* win, 
                       byte* wshape, 
                       byte* group,
                       byte* max_sfb, 
                       Info *wininfo,
                       Info* winmap[4]);

int	s_getics(Info *info, 
                 int common_window, 
                 BLOCKTYPE* win, 
                 byte* wshape, 
                 byte* group, 
                 byte* max_sfb,
                 byte* cb_map, 
                 Float *coef, 
                 short *global_gain, 
                 short *factors,
                 TNS_frame_info *tns, int ch, int max_spectral_line,
                 Info* winmap[4] );

#endif
