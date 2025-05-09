/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

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
$Id: int3gpp.h,v 1.1.1.1 2009-05-29 14:10:20 mul Exp $
*******************************************************************************/


#ifndef int3gpp_h
#define int3gpp_h

/*#include "../lib_amr/typedef.h"*/
#include "typedef.h"

#define NB_QUA_GAIN7B 128

/* enc.rom.c */
extern const Float32 E_ROM_qua_gain7b[];

/* enc_util.c */
void   E_UTIL_residuPlus(Float32 * a, Word32 m, Float32 * x, Float32 * y, Word32 l);
void   E_UTIL_synthesisPlus(Float32 a[], Word32 m, Float32 x[], Float32 y[], Word32 l, Float32 mem[], Word32 update_m);
void   E_UTIL_autocorrPlus(float *x, float *r, int m, int n, float *fh);
void   E_UTIL_residu(Float32 * a, Float32 * x, Float32 * y, Word32 l);
void   E_UTIL_f_convolve(Float32 x[], Float32 h[], Float32 y[]);
void   E_UTIL_synthesis(Float32 a[], Float32 x[], Float32 y[], Word32 l, Float32 mem[], Word32 update_m);
Word16 E_UTIL_random(Word16 * seed);
void   E_UTIL_f_preemph(Float32 * signal, Float32 mu, Word32 L, Float32 * mem);
void   E_UTIL_deemph(Float32 * signal, Float32 mu, Word32 L, Float32 * mem);
void   E_UTIL_hp50_12k8(Float32 signal[], Word32 lg, Float32 mem[]);
Word16 E_UTIL_saturate(Word32 inp);

/* enc_lpc.c */
void E_LPC_lsf_reorderPlus(float *lsf, float min_dist, int n);
void E_LPC_a_lsp_conversion(Float32 * a, Float32 * lsp, Float32 * old_lsp);
void E_LPC_f_lsp_a_conversion(Float32 * lsp, Float32 * a);
float E_LPC_lev_dur(Float32 *a, Float32 *r, Word32 m);
void E_LPC_a_weight(Float32 * a, Float32 * ap, Float32 gamma, Word32 m);
void E_LPC_lsp_lsf_conversion(Float32 lsp[], Float32 lsf[], Word32 m);

/* enc_gain.c */
void   E_GAIN_f_pitch_sharpening(Float32 * x, Word32 pit_lag);
Word32 E_GAIN_open_loop_search(Float32 * wsp, Word32 L_min, Word32 L_max,
                               Word32 nFrame, Word32 L_0, Float32 * gain,
                               Float32 * hp_wsp_mem, Float32 hp_old_wsp[], UWord8 weight_flg);
Word32 E_GAIN_olag_median(Word32 prev_ol_lag, Word32 old_ol_lag[5]);
void   E_GAIN_clip_init(Float32 mem[]);
Word32 E_GAIN_clip_test(Float32 mem[]);
void   E_GAIN_clip_isf_test(Float32 isf[], Float32 mem[]);
Word32 E_GAIN_closed_loop_search(Float32 exc[], Float32 xn[], Float32 h[],
                                 Word32 t0_min, Word32 t0_max, Word32 * pit_frac, Word32 i_subfr, Word32 t0_fr2, Word32 t0_fr1);
void   E_GAIN_adaptive_codebook_excitation(Word16 exc[], Word16 T0, Word32 frac, Word16 L_subfr);
void   E_GAIN_lp_decim2(Float32 x[], Word32 l, Float32 * mem);


/* enc_acelp.c */
void    E_ACELP_xh_corr(Float32 * x, Float32 * y, Float32 * h);
void    E_ACELP_xy2_corr(Float32 xn[], Float32 y1[], Float32 y2[], Float32 g_corr[]);
Float32 E_ACELP_xy1_corr(Float32 xn[], Float32 y1[], Float32 g_corr[]);
void    E_ACELP_codebook_target_update(Float32 * x, Float32 * x2, Float32 * y, Float32 gain);
void    E_ACELP_2t(Float32 dn[], Float32 cn[], Float32 H[], Word16 code[], Float32 y[], Word32 * index);
void    E_ACELP_4t(Float32 dn[], Float32 cn[], Float32 H[], Word16 code[], Float32 y[], Word32 nbbits, Word16 mode, int _index[]);


/* dec_acelp.c */
void D_ACELP_decode_4t(Word16 index[], Word16 nbbits, Word16 code[]);


#endif
