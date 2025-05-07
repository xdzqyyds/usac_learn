/*
This material is strictly confidential and shall remain as such.

Copyright ? 2009 VoiceAge Corporation. All Rights Reserved. No part of this
material may be reproduced, stored in a retrieval system, or transmitted, in any
form or by any means: photocopying, electronic, mechanical, recording, or
otherwise, without the prior written permission of the copyright holder.

This material is subject to continuous developments and improvements. All
warranties implied or expressed, including but not limited to implied warranties
of merchantability, or fitness for purpose, are excluded.

ACELP is registered trademark of VoiceAge Corporation in Canada and / or other
countries. Any unauthorized use is strictly prohibited.
*/

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "acelp_plus.h"


void decoder_acelp_fac(
  td_frame_data *td,
  int             k, /* input: frame number (within superframe) */
  float A[],         /* input: coefficients NxAz[M+1]   */
  int codec_mode,    /* input: AMR-WB+ mode (see cnst.h)*/
  int bfi,           /* input: 1=bad frame              */
  float exc[],       /* i/o:   exc[-(PIT_MAX+L_INTERPOL)..L_DIV] */
  float synth[],     /* i/o:   synth[-M..L_DIV]            */
  int *pT,           /* out:   pitch for all subframe   */
  float *pgainT,     /* out:   pitch gain for all subfr */
  float stab_fac,    /* input: stability of isf         */
  HANDLE_USAC_TD_DECODER st) /* i/o :  coder memory state       */
{
  int i, i_subfr, select;
  int T0, T0_frac, index, pit_flag, T0_min, T0_max;
  float tmp, gain_pit, gain_code, voice_fac, ener, mean_ener_code;
  float code[L_SUBFR];
  float mem_syn[M], syn[L_DIV_1024+128];
  float fac, exc2[L_SUBFR];
  float *p_A, Ap[1+M];
  short code3GPP[L_SUBFR], prm3GPP[10];
  int PIT_MIN;				/* Minimum pitch lag with resolution 1/4      */
  int PIT_FR2;				/* Minimum pitch lag with resolution 1/2      */
  int PIT_FR1;				/* Minimum pitch lag with resolution 1        */
  int PIT_MAX;				/* Maximum pitch lag                          */
  int subfr_nb=0;

  float x[LFAC_1024], xn2[2*LFAC_1024];
  int TTT;
  /* Variables dependent on the frame length */
  int lDiv;
  int lFAC;

  /* Set variables dependent on frame length */
  lDiv = st->lDiv;
  lFAC = lDiv/2;

  /* update past synth and exc using FAC when previous frame is TCX */
  if (st->last_mode > 0)
  {
    /* apply gains */
    for(i=0; i<lFAC; i++)
	{
	  x[i] = st->FAC_gain * td->fac[k*LFAC_1024+i];
	}
    for(i=0; i<lFAC/4; i++)
	{
	  x[i] *= st->FAC_alfd[i];
	}

    /* Inverse DCT4 */
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(st->hTcxMdct, lFAC), x, xn2+lFAC);
    smulr2r((2.0f/(float)lFAC), xn2+lFAC, xn2+lFAC, lFAC);

    set_zero(xn2, lFAC);

    E_LPC_a_weight(st->old_Aq, Ap, GAMMA1, M);

    /* 1/Wz of decoded FAC */
    E_UTIL_synthesis(Ap, xn2+lFAC, xn2+lFAC, lFAC, xn2, 0);

    /* Merge FAC and TCX synthesis */
    for(i=0; i<2*lFAC; i++)
	{
      xn2[i] += synth[i-(2*lFAC)];
	}

    /* update past synthesis */
    mvr2r(xn2+lFAC, synth-lFAC, lFAC);

    /* update past excitation */
    tmp = 0.0;
    E_UTIL_f_preemph(xn2, PREEMPH_FAC, 2*lFAC, &tmp);

    p_A = st->old_Aq;
    TTT = lFAC%L_SUBFR;
    if (TTT!=0)
    {
      E_UTIL_residu(p_A, &xn2[lFAC], &exc[lFAC-(2*lFAC)], TTT);
      p_A += (M+1);
    }

    for (i_subfr=lFAC+TTT; i_subfr<2*lFAC; i_subfr+=L_SUBFR) 
	{
      E_UTIL_residu(p_A, &xn2[i_subfr], &exc[i_subfr-(2*lFAC)], L_SUBFR);

      p_A += (M+1);
	}
  }

  /* set ACELP synthesis memory */
  for(i=0; i<M; i++)
  {
    mem_syn[i] = synth[i-M] - (PREEMPH_FAC*synth[i-M-1]);
  }

  i = (((st->fscale*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8;
  PIT_MIN = PIT_MIN_12k8 + i;
  PIT_FR2 = PIT_FR2_12k8 - i;
  PIT_FR1 = PIT_FR1_12k8;
  PIT_MAX = PIT_MAX_12k8 + (6*i);

 /*------------------------------------------------------------------------*
  * - decode mean_ener_code for gain decoder (d_gain2.c)                   *
  *------------------------------------------------------------------------*/

  /* decode mean energy with 2 bits : 18, 30, 42 or 54 dB */
  if (!bfi) {
    mean_ener_code = (((float)td->mean_energy[k]) * 12.0f) + 18.0f;
  }

 /*------------------------------------------------------------------------*
  *          Loop for every subframe in the analysis frame                 *
  *------------------------------------------------------------------------*
  *  To find the pitch and innovation parameters. The subframe size is     *
  *  L_SUBFR and the loop is repeated L_ACELP/L_SUBFR times.               *
  *     - compute impulse response of weighted synthesis filter (h1[])     *
  *     - compute the target signal for pitch search                       *
  *     - find the closed-loop pitch parameters                            *
  *     - encode the pitch dealy                                           *
  *     - update the impulse response h1[] by including fixed-gain pitch   *
  *     - find target vector for codebook search                           *
  *     - correlation between target vector and impulse response           *
  *     - codebook search                                                  *
  *     - encode codebook address                                          *
  *     - VQ of pitch and codebook gains                                   *
  *     - find synthesis speech                                            *
  *     - update states of weighting filter                                *
  *------------------------------------------------------------------------*/
  p_A = A;
  for (i_subfr = 0; i_subfr < lDiv; i_subfr += L_SUBFR)
  {
    pit_flag = i_subfr;
    if ((lDiv==256) && (i_subfr == (2*L_SUBFR))) {
      pit_flag = 0;
    }
    index = td->acb_index[k*NB_SUBFR_1024+subfr_nb];

    /*-------------------------------------------------*
     * - Decode pitch lag                              *
     *-------------------------------------------------*/
    if (bfi)                     /* if frame erasure */
    {
      /* Lag indices received also in case of BFI, so that 
         the parameter pointer stays in sync. */
      st->old_T0_frac += 1;        /* use last delay incremented by 1/4 */
      if (st->old_T0_frac > 3)
      {
        st->old_T0_frac -= 4;
        (st->old_T0)++;
      }
      if (st->old_T0 >= PIT_MAX)
      {
        st->old_T0 = PIT_MAX-5;
      }
      T0 = st->old_T0;
      T0_frac = st->old_T0_frac;
    }
    else
    {
      if (pit_flag == 0)
      {
        if (index < (PIT_FR2-PIT_MIN)*4)
        {
          T0 = PIT_MIN + (index/4);
          T0_frac = index - (T0 - PIT_MIN)*4;
        }
        else if (index < ( (PIT_FR2-PIT_MIN)*4 + (PIT_FR1-PIT_FR2)*2) )  
        {
          index -=  (PIT_FR2-PIT_MIN)*4;
          T0 = PIT_FR2 + (index/2);
          T0_frac = index - (T0 - PIT_FR2)*2;
          T0_frac *= 2;
        }
        else {
          T0 = index + PIT_FR1 - ((PIT_FR2-PIT_MIN)*4) - ((PIT_FR1-PIT_FR2)*2);
          T0_frac = 0;
        }
        /* find T0_min and T0_max for subframe 2 or 4 */
        T0_min = T0 - 8;
        if (T0_min < PIT_MIN) {
          T0_min = PIT_MIN;
        }
        T0_max = T0_min + 15;
        if (T0_max > PIT_MAX)
        {
          T0_max = PIT_MAX;
          T0_min = T0_max - 15;
        }
      }
      else      /* if subframe 2 or 4 */
      {
        T0 = T0_min + index/4;
        T0_frac = index - (T0 - T0_min)*4;
      }
    }
    /*-------------------------------------------------*
     * - Find the pitch gain, the interpolation filter *
     *   and the adaptive codebook vector.             *
     *-------------------------------------------------*/

    pred_lt4(&exc[i_subfr], T0, T0_frac, L_SUBFR+1);
    select = td->ltp_filtering_flag[k*NB_SUBFR_1024+subfr_nb];

    if (bfi) {
      select = 1;
    }
    if (select == 0)
    {
      /* find pitch excitation with lp filter */
      for (i=0; i<L_SUBFR; i++) {
        code[i] = (float)(0.18*exc[i-1+i_subfr] + 0.64*exc[i+i_subfr] + 0.18*exc[i+1+i_subfr]);
      }
      mvr2r(code, &exc[i_subfr], L_SUBFR);
    }
   /*-------------------------------------------------------*
    * - Decode innovative codebook.                         *
    * - Add the fixed-gain pitch contribution to code[].    *
    *-------------------------------------------------------*/
    {
      int g;
      for(g=0;g<8;g++) {
        prm3GPP[g] = (short)td->icb_index[k*NB_SUBFR_1024+subfr_nb][g];
      }
    }

    if (codec_mode == MODE_8k0)     
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 12, code3GPP);
      }
    }

    else if (codec_mode == MODE_8k8)     
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 16, code3GPP);
      }
    }

    else if (codec_mode == MODE_9k6)     
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 20, code3GPP);
      }
    }
    else if (codec_mode == MODE_11k2)
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 28, code3GPP);
      }
    }
    else if (codec_mode == MODE_12k8)
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 36, code3GPP);
      }
    }
    else if (codec_mode == MODE_14k4)
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 44, code3GPP);
      }
    }
    else if (codec_mode == MODE_16k)
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 52, code3GPP);
      }
    }
    else if (codec_mode == MODE_18k4)   
    {
      if (!bfi) {
        D_ACELP_decode_4t(prm3GPP, 64, code3GPP);
      }
    }
    else
    {
      printf("invalid mode for acelp frame!\n");
      exit(0);
    }
    {
      int g;
      for(g=0;g<L_SUBFR;g++) {
        code[g] = (float) (code3GPP[g]/512);
      }
    }
    if (bfi)
    {
      /* the innovative code doesn't need to be scaled (see D_gain2) */ 
      for (i=0; i<L_SUBFR; i++)
      {
        code[i] = (float)E_UTIL_random(&(st->seed_ace));
      }  
    }

   /*-------------------------------------------------------*
    * - Add the fixed-gain pitch contribution to code[].    *
    *-------------------------------------------------------*/
    tmp = 0.0;
    E_UTIL_f_preemph(code, TILT_CODE, L_SUBFR, &tmp);
    i = T0;
    if (T0_frac > 2) {
      i++;
    }
    E_GAIN_f_pitch_sharpening(code, i);

   /*-------------------------------------------------*
    * - Decode codebooks gains.                       *
    *-------------------------------------------------*/
    index = td->gains[k*NB_SUBFR_1024+subfr_nb];
    
    d_gain2_plus(index, code, L_SUBFR, &gain_pit, &gain_code, bfi,
                 mean_ener_code, &(st->past_gpit), &(st->past_gcode));

   /*----------------------------------------------------------*
    * Update parameters for the next subframe.                 *
    * - tilt of code: 0.0 (unvoiced) to 0.5 (voiced)           *
    *----------------------------------------------------------*/
    /* energy of pitch excitation */
    ener = 0.0;
    for (i=0; i<L_SUBFR; i++) {
      ener += exc[i+i_subfr]*exc[i+i_subfr];
    }
    ener *= (gain_pit*gain_pit);
    /* energy of innovative code excitation */
    tmp = 0.0;
    for (i=0; i<L_SUBFR; i++) {
      tmp += code[i]*code[i];
    }
    tmp *= gain_code*gain_code;
    /* find voice factor (1=voiced, -1=unvoiced) */
    voice_fac = (float)((ener - tmp) / (ener + tmp + 0.01));
   /*-------------------------------------------------------*
    * - Find the total excitation.                          *
    *-------------------------------------------------------*/
    for (i = 0; i < L_SUBFR;  i++) {
      exc2[i] = gain_pit*exc[i+i_subfr];
    }
    for (i = 0; i < L_SUBFR;  i++) {
      exc[i+i_subfr] = gain_pit*exc[i+i_subfr] + gain_code*code[i];
    }
   /*-------------------------------------------------------*
    * - Output pitch parameters for bass post-filter        *
    *-------------------------------------------------------*/
    i = T0;
    if (T0_frac > 2) {
      i++;
    }
    if (i > PIT_MAX) {
      i = PIT_MAX;
    }
    *pT++ = i;
    *pgainT++ = gain_pit;

   /*------------------------------------------------------------*
    * noise enhancer                                             *
    * ~~~~~~~~~~~~~~                                             *
    * - Enhance excitation on noise. (modify gain of code)       *
    *   If signal is noisy and LPC filter is stable, move gain   *
    *   of code 1.5 dB toward gain of code threshold.            *
    *   This decrease by 3 dB noise energy variation.            *
    *------------------------------------------------------------*/
    tmp = (float)(0.5*(1.0-voice_fac));       /* 1=unvoiced, 0=voiced */
    fac = stab_fac*tmp;
    tmp = gain_code;
    if (tmp < st->gc_threshold) {
      tmp = (float)(tmp*1.19);
      if (tmp > st->gc_threshold) {
        tmp = st->gc_threshold;
      }
    }
    else {
      tmp = (float)(tmp/1.19);
      if (tmp < st->gc_threshold) {
        tmp = st->gc_threshold;
      }
    }
    st->gc_threshold = tmp;
    gain_code = (float)((fac*tmp) + ((1.0-fac)*gain_code));
    for (i=0; i<L_SUBFR; i++) {
      code[i] *= gain_code;
    }

   /*------------------------------------------------------------*
    * pitch enhancer                                             *
    * ~~~~~~~~~~~~~~                                             *
    * - Enhance excitation on voice. (HP filtering of code)      *
    *   On voiced signal, filtering of code by a smooth fir HP   *
    *   filter to decrease energy of code in low frequency.      *
    *------------------------------------------------------------*/
    tmp = (float)(0.125*(1.0+voice_fac));     /* 0.25=voiced, 0=unvoiced */
    exc2[0] += code[0] - (tmp*code[1]);
    for (i=1; i<L_SUBFR-1; i++) {
      exc2[i] += code[i] - (tmp*code[i-1]) - (tmp*code[i+1]);
    }
    exc2[L_SUBFR-1] += code[L_SUBFR-1] - (tmp*code[L_SUBFR-2]);

    E_UTIL_synthesis(p_A, exc2, &syn[i_subfr], L_SUBFR, mem_syn, 1);

    p_A += (M+1);
    subfr_nb++;
  } /* end of subframe loop */

  /* deemphasis of ACELP synthesis */
  mvr2r(syn, synth, lDiv);
  tmp = synth[-1];
  E_UTIL_deemph(synth, PREEMPH_FAC, lDiv, &tmp);

  /* ZIR at the end of the ACELP frame */
  set_zero(syn+lDiv, 128);
  E_UTIL_synthesis(p_A, syn+lDiv, syn+lDiv, 128, mem_syn, 0);

  /* update old_Aq */
  p_A -= (M+1);
  mvr2r(p_A, st->old_Aq, 2*(M+1));

  /* update old_xnq */
  mvr2r(synth+lDiv-(1+lFAC), st->old_xnq, 1+lFAC);

  /* find ZIR for next TCX frame */
  mvr2r(syn+lDiv, st->old_xnq+1+lFAC, lFAC);
  tmp = synth[lDiv-1];
  E_UTIL_deemph(st->old_xnq+1+lFAC, PREEMPH_FAC, lFAC, &tmp);

  return;
}
