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


$Id: cod_main.c,v 1.8 2011-05-02 09:11:02 mul Exp $

*******************************************************************************/
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "int3gpp.h"
#include "acelp_plus.h"


/*-----------------------------------------------------------------*
 * Funtion  coder_amrwb_plus_mono                                  *
 *          ~~~~~~~~~~~~~~~~                                       *
 *   - Main mono coder routine.                                    *
 *                                                                 *
 *-----------------------------------------------------------------*/
int coder_amrwb_plus_mono(  /* output: number of sample processed */
    float channel_right[], /* input: used on mono and stereo       */
    short serial[],    /* output: serial parameters                */
    short serialFac[],
    int   *nBitsFac,
    HANDLE_USAC_TD_ENCODER st,    /* i/o : coder memory state                 */
    int fscale,
    int isAceStart,
    int *total_nbbits,
    int  mod_out[4], /* output: mode index */
    int const bUsacIndependencyFlag
)
{
  /* LPC coefficients of lower frequency */
  int param[NB_DIV*NPRM_DIV];
  int param_lpc[NPRM_LPC_NEW];
  /* vector working at fs=12.8kHz */
  float old_speech[L_TOTAL_HIGH_RATE+L_LPC0_1024];
  float old_synth[M+L_FRAME_1024];
  float *speech, *new_speech;
  float *synth;
  /* Scalars */
  int i;
  int mod_buf[1+NB_DIV], *mod;
  /* LTP parameters for high band */
  float ol_gain[NB_DIV];  
  /*ClassB parameters*/
  short excType[4];  
  int n_param_tcx[NB_DIV];
  int isBassPostFilter;

  int lDiv;

  lDiv = st->lDiv;

  /* reset prev_mod, in case this is the first LPD frame */
  if(isAceStart){
    st->prev_mod = -1;
  }

  mod = mod_buf+1;
  mod[-1] = st->prev_mod;    /* previous mode */

  /* Update fscale to take into account 3/4 resampling */
  fscale = (fscale * lDiv) / L_DIV_1024;

  /*---------------------------------------------------------------------*
   * Initialize pointers to speech vector.                               *
   *                                                                     *
   *                     20ms     20ms     20ms     20ms    >=20ms       *
   *             |----|--------|--------|--------|--------|--------|     *
   *           past sp   div1     div2     div3     div4    L_NEXT       *
   *             <--------  Total speech buffer (L_TOTAL_PLUS)  -------->     *
   *        old_speech                                                   *
   *                  <----- present frame (L_FRAME_PLUS) ----->              *
   *                  |        <------ new speech (L_FRAME_PLUS) ------->     *
   *                  |        |                                         *
   *                speech     |                                         *
   *                         new_speech                                  *
   *---------------------------------------------------------------------*/
  /* if(isAceStart){
    memset(old_speech,0,L_TOTAL_ST*sizeof(*old_speech));
  }*/
  new_speech = old_speech + M + (L_NEXT_HIGH_RATE_1024 * lDiv) / L_DIV_1024;
  speech     = old_speech + M;
  synth      = old_synth + M;
  if (isAceStart)
  {
    new_speech += (L_LPC0_1024 * lDiv) / L_DIV_1024;
    speech     += (L_LPC0_1024 * lDiv) / L_DIV_1024;
  }
  /*-----------------------------------------------------------------*
   * MONO/STEREO signal downsampling (codec working at 6.4kHz)       *
   * - decimate signal to fs=12.8kHz                                 *
   * - Perform 50Hz HP filtering of signal at fs=12.8kHz.            *
   * - Perform fixed preemphasis through 1 - g z^-1                  *
   * - Mix left and right channels into sum and difference signals   *
   * - perform parametric stereo encoding                            *
   *-----------------------------------------------------------------*/

  mvr2r(channel_right,new_speech,st->lFrame);

  /* high pass filter with a 2nd order IIR filter */
  hp50_12k8(new_speech, st->lFrame, st->mem_sig_in, fscale);

  /*------------------------------------------------------------*
   * Encode MONO low frequency band using ACELP/TCX model       *
   *------------------------------------------------------------*/
  /* Apply preemphasis (for core codec only */
  E_UTIL_f_preemph(new_speech, PREEMPH_FAC, st->lFrame, &(st->mem_preemph));

  /* copy memory into working space */
  if (isAceStart)
  {
    mvr2r(st->old_speech_pe, old_speech, M+(((L_NEXT_HIGH_RATE_1024+L_LPC0_1024)*lDiv)/L_DIV_1024)); 
    {
      float tmp;
      float tmp_buf[L_DIV_1024+1];

      for (i=0; i<(lDiv+1); i++)
      {
        tmp_buf[i] = speech[-lDiv-1+i];
      }
      tmp = tmp_buf[0];
      E_UTIL_deemph(tmp_buf, PREEMPH_FAC, lDiv+1, &tmp);
      mvr2r(&tmp_buf[lDiv-128+1], st->LPDmem.Txn, 128);
    }

  }
  else
  {
    mvr2r(st->old_speech_pe, old_speech, M+((L_NEXT_HIGH_RATE_1024*lDiv)/L_DIV_1024));
  }

  mvr2r(st->old_synth, old_synth, M);

  for (i=0;i<4;i++) {
    excType[i] = 0;
  }
  /* encode mono lower band */
  coder_lf_amrwb_plus(&st->mode,
                      speech,
                      synth,
                      mod,
                      n_param_tcx,
                      st->window,
                      param_lpc,
                      param,
                      serialFac,
                      nBitsFac,
                      ol_gain,
                      fscale,
                      st,
                      isAceStart
                      );

  /* update lower band memory for next frame */
  if (isAceStart)
  {
    mvr2r(&old_speech[(st->lFrame)+(L_LPC0_1024*lDiv)/L_DIV_1024], st->old_speech_pe, M+((L_NEXT_HIGH_RATE_1024*lDiv)/L_DIV_1024));
  }
  else
  {
    mvr2r(&old_speech[(st->lFrame)], st->old_speech_pe, M+((L_NEXT_HIGH_RATE_1024*lDiv)/L_DIV_1024));
  }
  mvr2r(&old_synth[st->lFrame], st->old_synth, M);

  isBassPostFilter = 1;

  /*--------------------------------------------------*
   * encode bits for serial stream                    *
   *--------------------------------------------------*/
  /* mode (0=ACELP 20ms, 1=TCX 20ms, 2=TCX 40ms, 3=TCX 80ms) */
  /* for 20-ms packetization, divide by 4 the 80-ms bitstream */
  /*   nbits_pack = (NBITS_CORE[codec_mode] + NBITS_BWE)/4; */
  enc_prm_fac(mod, n_param_tcx, st->mode, param, param_lpc, isAceStart, isBassPostFilter, serial, st, total_nbbits, bUsacIndependencyFlag);

  /* update modes */
  st->prev_mod = mod[3];
  memcpy(mod_out, mod, 4*sizeof(int));

  return(st->lFrame);
}


/* ndf 20071128 copied from the original cod_main.c. Modified */
/*-----------------------------------------------------------------*
 * Funtion  coder_amrwb_plus_first                                 *
 *          ~~~~~~~~~~~~~~~~                                       *
 *   - Fill lookahead buffers                                      *
 *                                                                 *
 *-----------------------------------------------------------------*/
int coder_amrwb_plus_first(   /* output: number of sample processed */
    float channel_right[], /* input: used on mono and stereo       */
    int L_next,        /* input: lookahead                         */
    HANDLE_USAC_TD_ENCODER st   /* i/o : coder memory state                 */
)
{
  float old_speech[L_NEXT_HIGH_RATE_1024+L_LPC0_1024];
  float *new_speech=old_speech;

  mvr2r(channel_right,new_speech,L_next);

  hp50_12k8(new_speech, L_next, st->mem_sig_in, st->fscale);

  /* copy working space into memory */
  mvr2r(new_speech, st->old_speech+M, L_next);

  /* Apply preemphasis (for core codec only */
  E_UTIL_f_preemph(new_speech, PREEMPH_FAC, L_next, &(st->mem_preemph));

  /* update lower band memory for next frame */
  mvr2r(new_speech, st->old_speech_pe+M, L_next);

  return(L_next);

}
