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
#include "usac_all.h"                 /* structs */
#include "usac_allVariables.h"        /* variables */
#include "huffdec3.h"
#include "buffers.h"
#include "usac_cplx_pred.h"

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
                            HANDLE_BUFFER hVm)
{
  const int sfb_per_pred_band = 2;
  Hcb*      hcb = &book[BOOKSCL];
  Huffman*  hcw = hcb->hcw;
  int cplx_pred_all;
  int delta_code_time;
  int g, sfb;
  int dpcm_alpha, last_alpha_q_re, last_alpha_q_im;

  cplx_pred_all = GetBits(LEN_CPLX_PRED_ALL,
                          CPLX_PRED_ALL,
                          hResilience,
                          hEscInstanceData,
                          hVm);

  if (cplx_pred_all == 0) {
    for (g = 0; g < num_window_groups; g++) {
      for (sfb = 0; sfb < max_sfb_ste; sfb += sfb_per_pred_band) {
        cplx_pred_used[g][sfb] = GetBits(LEN_CPLX_PRED_USED,
                                         CPLX_PRED_USED,
                                         hResilience,
                                         hEscInstanceData,
                                         hVm);

        if (sfb+1 < max_sfb_ste) {
          cplx_pred_used[g][sfb+1] = cplx_pred_used[g][sfb];
        }
      }
      for (sfb = max_sfb_ste; sfb < SFB_NUM_MAX; sfb++) cplx_pred_used[g][sfb] = 0;
    }
  }
  else {
    for (g = 0; g < num_window_groups; g++) {
      for (sfb = 0; sfb < max_sfb_ste; sfb++) {
        cplx_pred_used[g][sfb] = 1;
      }
      for (sfb = max_sfb_ste; sfb < SFB_NUM_MAX; sfb++) cplx_pred_used[g][sfb] = 0;
    }
  }

  *pred_dir = GetBits(LEN_PRED_DIR,
                      PRED_DIR,
                      hResilience,
                      hEscInstanceData,
                      hVm);

  *complex_coef = GetBits(LEN_COMPLEX_COEF,
                          COMPLEX_COEF,
                          hResilience,
                          hEscInstanceData,
                          hVm);

  if (*complex_coef) {
    if(bUsacIndependenceFlag){
      *use_prev_frame = 0;
    } else {
      *use_prev_frame = GetBits(LEN_USE_PREV_FRAME,
                                USE_PREV_FRAME,
                                hResilience,
                                hEscInstanceData,
                                hVm);
    }
  }

  if(bUsacIndependenceFlag){
    delta_code_time = 0;
  } else {
    delta_code_time = GetBits(LEN_DELTA_CODE_TIME,
                              DELTA_CODE_TIME,
                              hResilience,
                              hEscInstanceData,
                              hVm);
  }

  for (g = 0; g < num_window_groups; g++) {
    for (sfb = 0; sfb < max_sfb_ste; sfb += sfb_per_pred_band) {
      if (delta_code_time == 1) {
        last_alpha_q_re = alpha_q_re_prev[sfb];
        last_alpha_q_im = alpha_q_im_prev[sfb];
      }
      else {
        if (sfb > 0) {
          last_alpha_q_re = alpha_q_re[g][sfb-1];
          last_alpha_q_im = alpha_q_im[g][sfb-1];
        }
        else {
          last_alpha_q_re = last_alpha_q_im = 0;
        }
      }

      if (cplx_pred_used[g][sfb] == 1) {
        dpcm_alpha = -decode_huff_cw(hcw, 0, hResilience, hEscInstanceData, hVm) + 60;
        alpha_q_re[g][sfb] = dpcm_alpha + last_alpha_q_re;

        if (*complex_coef) {
          dpcm_alpha = -decode_huff_cw(hcw, 0, hResilience, hEscInstanceData, hVm) + 60;
          alpha_q_im[g][sfb] = dpcm_alpha + last_alpha_q_im;
        }
        else {
          alpha_q_im[g][sfb] = 0;
        }
      }
      else {
        alpha_q_re[g][sfb] = 0;
        alpha_q_im[g][sfb] = 0;
      }

      if ((sfb+1) < max_sfb_ste) {
        alpha_q_re[g][sfb+1] = alpha_q_re[g][sfb];
        alpha_q_im[g][sfb+1] = alpha_q_im[g][sfb];
      }

      alpha_q_re_prev[sfb] = alpha_q_re[g][sfb];
      alpha_q_im_prev[sfb] = alpha_q_im[g][sfb];
    }
    for (sfb = max_sfb_ste; sfb < SFB_NUM_MAX; sfb++) {
      alpha_q_re[g][sfb] = 0;
      alpha_q_im[g][sfb] = 0;
      alpha_q_re_prev[sfb] = 0;
      alpha_q_im_prev[sfb] = 0;
    }
  }

  return 1;
}


static void filterAndAdd(const float *in, const int length, const float *filter, float *out,
                  const float factorEven, const float factorOdd)
{
  int i;
  float s;

  i = 0;
  s = filter[6]*in[2] + filter[5]*in[1] + filter[4]*in[0] + filter[3]*in[0] +
      filter[2]*in[1] + filter[1]*in[2] + filter[0]*in[3];
  out[i] += s*factorEven;
  i = 1;
  s = filter[6]*in[1] + filter[5]*in[0] + filter[4]*in[0] + filter[3]*in[1] +
      filter[2]*in[2] + filter[1]*in[3] + filter[0]*in[4];
  out[i] += s*factorOdd;
  i = 2;
  s = filter[6]*in[0] + filter[5]*in[0] + filter[4]*in[1] + filter[3]*in[2] +
      filter[2]*in[3] + filter[1]*in[4] + filter[0]*in[5];
  out[i] += s*factorEven;
  for (i = 3; i < length-4; i += 2) {
    s = filter[6]*in[i-3] + filter[5]*in[i-2] + filter[4]*in[i-1] + filter[3]*in[i] +
        filter[2]*in[i+1] + filter[1]*in[i+2] + filter[0]*in[i+3];
    out[i] += s*factorOdd;
    s = filter[6]*in[i-2] + filter[5]*in[i-1] + filter[4]*in[i] + filter[3]*in[i+1] +
        filter[2]*in[i+2] + filter[1]*in[i+3] + filter[0]*in[i+4];
    out[i+1] += s*factorEven;
  }
  i = length-3;
  s = filter[6]*in[i-3] + filter[5]*in[i-2] + filter[4]*in[i-1] + filter[3]*in[i] +
      filter[2]*in[i+1] + filter[1]*in[i+2] + filter[0]*in[i+2];
  out[i] += s*factorOdd;
  i = length-2;
  s = filter[6]*in[i-3] + filter[5]*in[i-2] + filter[4]*in[i-1] + filter[3]*in[i] +
      filter[2]*in[i+1] + filter[1]*in[i+1] + filter[0]*in[i];
  out[i] += s*factorEven;
  i = length-1;
  s = filter[6]*in[i-3] + filter[5]*in[i-2] + filter[4]*in[i-1] + filter[3]*in[i] +
      filter[2]*in[i]   + filter[1]*in[i-1] + filter[0]*in[i-2];
  out[i] += s*factorOdd;
}


static void mdct2mdst(const float* mdctSpectrumMidCurr, const float* mdctSpectrumMidPrev,
               float* mdstSpectrumMid, const int len, WINDOW_SEQUENCE win,
               const int windowShape, const int prevWindowShape)
{

  /* filter coefficients */
  static const float h_long_sin_cur[]       = { -0.000000f, -0.000000f,  0.500000f,  0.000000f, -0.500000f,  0.000000f,  0.000000f };
  static const float h_long_kbd_cur[]       = {  0.091497f, -0.000000f,  0.581427f,  0.000000f, -0.581427f,  0.000000f, -0.091497f };
  static const float h_long_sin_kbd_cur[]   = {  0.045748f,  0.057238f,  0.540714f,  0.000000f, -0.540714f, -0.057238f, -0.045748f };
  static const float h_long_kbd_sin_cur[]   = {  0.045748f, -0.057238f,  0.540714f,  0.000000f, -0.540714f,  0.057238f, -0.045748f };
  static const float h_start_sin_cur[]      = {  0.102658f,  0.103791f,  0.567149f,  0.000000f, -0.567149f, -0.103791f, -0.102658f };
  static const float h_start_kbd_cur[]      = {  0.150512f,  0.047969f,  0.608574f,  0.000000f, -0.608574f, -0.047969f, -0.150512f };
  static const float h_start_sin_kbd_cur[]  = {  0.104763f,  0.105207f,  0.567861f,  0.000000f, -0.567861f, -0.105207f, -0.104763f };
  static const float h_start_kbd_sin_cur[]  = {  0.148406f,  0.046553f,  0.607863f,  0.000000f, -0.607863f, -0.046553f, -0.148406f };
  static const float h_stop_sin_cur[]       = {  0.102658f, -0.103791f,  0.567149f,  0.000000f, -0.567149f,  0.103791f, -0.102658f };
  static const float h_stop_kbd_cur[]       = {  0.150512f, -0.047969f,  0.608574f,  0.000000f, -0.608574f,  0.047969f, -0.150512f };
  static const float h_stop_sin_kbd_cur[]   = {  0.148406f, -0.046553f,  0.607863f,  0.000000f, -0.607863f,  0.046553f, -0.148406f };
  static const float h_stop_kbd_sin_cur[]   = {  0.104763f, -0.105207f,  0.567861f,  0.000000f, -0.567861f,  0.105207f, -0.104763f };
  static const float h_stopstart_sin_cur[]      = {  0.205316f, -0.000000f,  0.634298f,  0.000000f, -0.634298f,  0.000000f, -0.205316f };
  static const float h_stopstart_kbd_cur[]      = {  0.209526f, -0.000000f,  0.635722f,  0.000000f, -0.635722f,  0.000000f, -0.209526f };
  static const float h_stopstart_sin_kbd_cur[]  = {  0.207421f,  0.001416f,  0.635010f,  0.000000f, -0.635010f, -0.001416f, -0.207421f };
  static const float h_stopstart_kbd_sin_cur[]  = {  0.207421f, -0.001416f,  0.635010f,  0.000000f, -0.635010f,  0.001416f, -0.207421f };

  static const float h_long_sin_prv[]   = { -0.000000f,  0.106103f,  0.250000f,  0.318310f,  0.250000f,  0.106103f, -0.000000f };
  static const float h_long_kbd_prv[]   = {  0.059509f,  0.123714f,  0.186579f,  0.213077f,  0.186579f,  0.123714f,  0.059509f };
  static const float h_stop_sin_prv[]   = {  0.038498f,  0.039212f,  0.039645f,  0.039790f,  0.039645f,  0.039212f,  0.038498f };
  static const float h_stop_kbd_prv[]   = {  0.026142f,  0.026413f,  0.026577f,  0.026631f,  0.026577f,  0.026413f,  0.026142f };

  static const float h_long_sin_nxt[]   = { -0.000000f, -0.106103f,  0.250000f, -0.318310f,  0.250000f, -0.106103f, -0.000000f };
  static const float h_long_kbd_nxt[]   = {  0.059509f, -0.123714f,  0.186579f, -0.213077f,  0.186579f, -0.123714f,  0.059509f };
  static const float h_start_sin_nxt[]  = {  0.038498f, -0.039212f,  0.039645f, -0.039790f,  0.039645f, -0.039212f,  0.038498f };
  static const float h_start_kbd_nxt[]  = {  0.026142f, -0.026413f,  0.026577f, -0.026631f,  0.026577f, -0.026413f,  0.026142f };

  /* group window shape combinations (sin/sin, kbd/kbd, sin/kbd, kbd/sin) */
  static const float *h_long_cur[2][2] = { {h_long_sin_cur, h_long_sin_kbd_cur}, {h_long_kbd_sin_cur, h_long_kbd_cur} };
  static const float *h_start_cur[2][2] = { {h_start_sin_cur, h_start_sin_kbd_cur}, {h_start_kbd_sin_cur, h_start_kbd_cur} };
  static const float *h_stop_cur[2][2] = { {h_stop_sin_cur, h_stop_sin_kbd_cur}, {h_stop_kbd_sin_cur, h_stop_kbd_cur} };
  static const float *h_stopstart_cur[2][2] = { {h_stopstart_sin_cur, h_stopstart_sin_kbd_cur}, {h_stopstart_kbd_sin_cur, h_stopstart_kbd_cur} };

  static const float *h_long_prv[2] = {h_long_sin_prv, h_long_kbd_prv};
  static const float *h_stop_prv[2] = {h_stop_sin_prv, h_stop_kbd_prv};

  const float *h_curr = NULL;
  const float *h_prev = NULL;
  int i;

  /* determine filter coefficients */
  switch (win) {
  case ONLY_LONG_SEQUENCE:
  case EIGHT_SHORT_SEQUENCE:
    h_curr = h_long_cur[prevWindowShape][windowShape];
    h_prev = h_long_prv[prevWindowShape];
    break;
  case LONG_START_SEQUENCE:
    h_curr = h_start_cur[prevWindowShape][windowShape];
    h_prev = h_long_prv[prevWindowShape];
    break;
  case LONG_STOP_SEQUENCE:
    h_curr = h_stop_cur[prevWindowShape][windowShape];
    h_prev = h_stop_prv[prevWindowShape];
    break;
  case STOP_START_SEQUENCE:
    h_curr = h_stopstart_cur[prevWindowShape][windowShape];
    h_prev = h_stop_prv[prevWindowShape];
    break;
  default:
    /* invalid case; aborting */
    exit(-1);
  }

  /* estimate MDST by filtering MDCT */
  for (i = 0; i < len; i++) {
    mdstSpectrumMid[i] = 0.0f;
  }
  if (mdctSpectrumMidCurr) {
    filterAndAdd(mdctSpectrumMidCurr, len, h_curr, mdstSpectrumMid,  1.0f, 1.0f);
  }
  if (mdctSpectrumMidPrev) {
    filterAndAdd(mdctSpectrumMidPrev, len, h_prev, mdstSpectrumMid, -1.0f, 1.0f);
  }
}

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
                          WINDOW_SHAPE winShapePrev)
{
  float dmx_re[BLOCK_LEN_LONG], dmx_im[BLOCK_LEN_LONG];
  float alpha_re[BLOCK_LEN_LONG], alpha_im[BLOCK_LEN_LONG];
  int cp_used[BLOCK_LEN_LONG];

  int grp, sbk, sfb, bin, i=0;

  /* map group/sfb -> spec index, dequantize coefficients */
  for (grp = 0, sbk = 0; sbk < info->nsbk; sbk++) {
    if (sbk >= group[grp]) grp++;

    for (bin = 0, sfb = 0; bin < info->bins_per_sbk[sbk]; bin++) {
      if (bin >= info->sbk_sfb_top[sbk][sfb]) sfb++;

      cp_used[i] = cplx_pred_used[grp][sfb];
      alpha_re[i] = alpha_q_re[grp][sfb] * 0.1f;
      alpha_im[i] = complex_coef ? alpha_q_im[grp][sfb] * 0.1f : 0;

      i++;
    }
  }

  /* real part of dmx */
  for (i = 0; i < info->bins_per_bk; i++) {
    if (cp_used[i] == 1) {
      dmx_re[i] = specL[i];
    }
    else {
      if (pred_dir == 0) {
        dmx_re[i] = 0.5f * (specL[i] + specR[i]);
      }
      else {
        dmx_re[i] = 0.5f * (specL[i] - specR[i]);
      }
    }
  }

  /* imag part of dmx */
  if (complex_coef) {
    float *p_dmx_re = dmx_re;
    float *p_dmx_re_prev = use_prev_frame ? dmx_re_prev : NULL;
    float *p_dmx_im = dmx_im;

    for (i = 0; i < info->nsbk; i++) {
      mdct2mdst(p_dmx_re,
                p_dmx_re_prev,
                p_dmx_im,
                info->bins_per_sbk[i],
                win, winShape,
                winShapePrev);

      winShapePrev  = winShape;
      p_dmx_re_prev = p_dmx_re;
      p_dmx_re     += info->bins_per_sbk[i];
      p_dmx_im     += info->bins_per_sbk[i];
    }
  }
  else {
    for (i = 0; i < BLOCK_LEN_LONG; i++) {
      dmx_im[i] = 0;
    }
  }

  /* upmix */
  for (i = 0; i < info->bins_per_bk; i++) {
    if (cp_used[i]) {
      if (pred_dir == 0) {
        float side = specR[i] - alpha_re[i] * specL[i] - alpha_im[i] * dmx_im[i];
        specR[i] = specL[i] - side;
        specL[i] = specL[i] + side;
      }
      else {
        float mid = specR[i] - alpha_re[i] * specL[i] - alpha_im[i] * dmx_im[i];
        specR[i] = mid - specL[i];
        specL[i] = mid + specL[i];
      }
    }
  }

  return 1;
}

void usac_cplx_pred_prev_dmx(Info *info,
                             float *specL,
                             float *specR,
                             float *dmx_re_prev,
                             int pred_dir)
{
  int offs, i;

  /* save prev frame dmx (last subblock) */
  offs = info->bins_per_bk - info->bins_per_sbk[info->nsbk-1];
  for (i = 0; i < info->bins_per_sbk[info->nsbk-1]; i++) {
    if (pred_dir == 0) {
      dmx_re_prev[i] = 0.5f * (specL[i + offs] + specR[i + offs]);
    }
    else {
      dmx_re_prev[i] = 0.5f * (specL[i + offs] - specR[i + offs]);
    }
  }
}


#ifndef USAC_REFSOFT_COR1_NOFIX_03
void usac_cplx_save_prev(Info *info,
                         float *specL,
                         float *specR,
                         float *specLsave,
                         float *specRsave)
{
  int offs, i;

  /* save prev frame coefs (last subblock) */
  offs = info->bins_per_bk - info->bins_per_sbk[info->nsbk-1];
  for (i = 0; i < info->bins_per_sbk[info->nsbk-1]; i++) {

    specLsave[i + offs] = specL[i + offs];
    specRsave[i + offs] = specR[i + offs];
  }
}
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */
