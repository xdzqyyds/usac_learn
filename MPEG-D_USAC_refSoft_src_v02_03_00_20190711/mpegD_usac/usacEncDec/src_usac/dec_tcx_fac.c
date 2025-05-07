/*
This material is strictly confidential and shall remain as such.

Copyright (c) 2009 VoiceAge Corporation. All Rights Reserved. No part of this
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
#include "re8.h"


#ifdef UNIFIED_RANDOMSIGN
static float randomSign(unsigned int *seed);
#else
static void rnd_sgn16(short *seed, float *xri, int lg);
#endif

void decoder_tcx_fac(td_frame_data *td,
                     int frame_index,   /* input: index of the presemt frame to decode*/
                     float A[],         /* input: coefficients NxAz[M+1]  */
                     int L_frame,       /* input: frame length            */
                     float exc[],       /* output: exc[-lg..lg]            */
                     float synth[],     /* in/out: synth[-M..lg]           */
                     HANDLE_USAC_TD_DECODER st) /* i/o :  coder memory state       */
{
  int i, k, i_subfr, index, lg, lext, mode;
  int *ptr_quant;
  float tmp, gain, gain_tcx, noise_level, ener;
  float *p_A, Ap[M+1];
  float mem_xnq;
  const float *sinewindowPrev, *sinewindow;
  int lfacPrev;
  float alfd_gains[N_MAX/(4*8)];

#ifndef UNIFIED_RANDOMSIGN
  float ns[N_MAX];
#endif
  float x[N_MAX], buf[M+L_FRAME_1024];
  float gainlpc[N_MAX], gainlpc2[N_MAX];

  float xn_buf[L_FRAME_1024+(2*LFAC_1024)];
  float *xn;
  float facelp[LFAC_1024];
  float xn1[2*LFAC_1024], facwindow[2*LFAC_1024];
  int TTT;

  /* Variables dependent on the frame length */
  int lFAC;

  /* Set variables dependent on frame length */
  lFAC   = (st->lDiv)/2;

  /* mode TCX: 1/2/3 = TCX20/40/80 */
  mode = L_frame/(st->lDiv);
  if (mode > 2 ) mode = 3;

  if (st->last_mode == -2 ) {
    lfacPrev = (st->lFrame)/16;
  }
  else {
    lfacPrev = lFAC;
  }

  if (lFAC==96)
  {
    sinewindow = sineWindow192; /*2*LFAC*/
  }
  else
  {
    sinewindow = sineWindow256;
  }

  if (lfacPrev == 48) {
    sinewindowPrev = sineWindow96;
  }
  else if (lfacPrev == 64) {
    sinewindowPrev = sineWindow128;
  }
  else if (lfacPrev == 96) {
    sinewindowPrev = sineWindow192;
  }
  else {
    sinewindowPrev = sineWindow256;
  }

  lg = L_frame;
  lext = lFAC;
  /* set target pointer (pointer to current frame) */
  xn = xn_buf + lFAC;

  /* window past overlap */
  if (st->last_mode != 0 ) {
#ifndef USAC_REFSOFT_COR1_NOFIX_06
      /*st->old_xnq is already weighted if previous frame was FD*/
    if(st->last_mode>0) {
#endif
      for (i=0; i<(2*lfacPrev); i++)
      {
        st->old_xnq[i+lFAC-lfacPrev+1] *= sinewindowPrev[(2*lfacPrev)-1-i];
      }
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    }
#endif
    for ( i = 0 ; i < lFAC-lfacPrev; i++ ) {
      st->old_xnq[i+lFAC+lfacPrev+1] = 0.0f;
    }
  }

  /* decode noise level (noise_level) (stored in 2nd packet on TCX80) */
  index = td->tcx_noise_factor[frame_index];
  noise_level = 0.0625f*(8.0f - ((float)index));   /* between 0.5 and 0.0625 */

  /* read index of global TCX gain */
  index =  td->tcx_global_gain[frame_index];

  /* decoded MDCT coefficients */
  ptr_quant = td->tcx_quant;
  for(i=0; i<frame_index; i++)    
    ptr_quant += td->tcx_lg[i];
  if(lg!=td->tcx_lg[i])
    exit(1);
  for(i=0; i<lg; i++)
    x[i] = (float) ptr_quant[i];

#ifndef UNIFIED_RANDOMSIGN
  /* generate random excitation buffer */
  set_zero(ns, lg);
  rnd_sgn16(&(st->seed_tcx), &ns[lg/6], lg-(lg/6)); /*random Sign for MDCT (instead of phase)*/
#endif

  /*----------------------------------------------*
   * noise fill-in on unquantized subvector       *
   * injected only from 1066Hz to 6400Hz.         *
   *----------------------------------------------*/
  for(k=lg/6; k<lg;) 
  {
    int maxK;

	tmp = 0.0f;
	maxK = MIN(lg,k+8);

    for(i=k; i<maxK; i++) 
	{
      tmp += x[i]*x[i];
    }
    if (tmp == 0.0) 
	{
      for(i=k; i<maxK; i++) 
	  {
#ifdef UNIFIED_RANDOMSIGN
        x[i] = noise_level * randomSign(&(st->seed_tcx));
#else
        x[i] = noise_level*ns[i];
#endif
      }
    }
	k=maxK;
  }

  AdaptLowFreqDeemph(x, lg, alfd_gains);

 /*-----------------------------------------------------------*
  * decode TCX global gain.                                   *
  *-----------------------------------------------------------*/

  ener = 0.01f;
  for(i=0; i<lg; i++) ener += x[i]*x[i];

  tmp = 2.0f*(float)sqrt(ener)/lg;
  
  gain_tcx = (float)pow(10.0f, ((float)index)/28.0f) / tmp;

 /*-----------------------------------------------------------*
  * Noise shaping in frequency domain (1/Wz)                  *
  *-----------------------------------------------------------*/
  /* old NS spectral gain (could be computed every LPC frame) */
  E_LPC_a_weight(A+(M+1), Ap, GAMMA1, M);
  lpc2mdct(Ap, M, gainlpc, (FDNS_NPTS_1024*(st->lDiv))/L_DIV_1024);

  /* new NS spectral gain (could be computed every LPC frame) */
  E_LPC_a_weight(A+(2*(M+1)), Ap, GAMMA1, M);
  lpc2mdct(Ap, M, gainlpc2, (FDNS_NPTS_1024*(st->lDiv))/L_DIV_1024);

  mdct_IntNoiseShaping(x, lg, (FDNS_NPTS_1024*(st->lDiv))/L_DIV_1024, gainlpc, gainlpc2);

 /*-----------------------------------------------------------*
  * Compute inverse MDCT of x[].                              *
  *-----------------------------------------------------------*/

  TCX_MDCT_ApplyInverse(st->hTcxMdct, x, xn_buf, (2*lFAC), L_frame-(2*lFAC), (2*lFAC));
  smulr2r((2.0f/lg), xn_buf, xn_buf, L_frame+(2*lFAC));

  gain = gain_tcx*0.5f*(float)sqrt(((float)lFAC)/(float)L_frame);
  st->FAC_gain = gain;

  for(i=0; i<lFAC/4; i++)
  {
    k = i*lg/(8*lFAC);
	st->FAC_alfd[i] = alfd_gains[k];
  }

  if (st->last_mode == 0)    
  {
   /*-----------------------------------------------------------*
    * Decode FAC in case where previous frame is ACELP.         *
    *-----------------------------------------------------------*/

    /* build FAC window (should be in ROM) */
    for (i=0; i<lfacPrev; i++)
    {
      facwindow[i] = sinewindowPrev[i]*sinewindowPrev[(2*lfacPrev)-1-i];
      facwindow[lfacPrev+i] = 1.0f - (sinewindowPrev[lfacPrev+i]*sinewindowPrev[lfacPrev+i]);
    }

    /* windowing and folding of ACELP part including the ZIR */
    for (i=0; i<lFAC; i++)
    {
      facelp[i] = st->old_xnq[1+lFAC+i]*facwindow[lFAC+i]
	         + st->old_xnq[lFAC-i]*facwindow[lFAC-1-i];
    }

    /* retrieve MDCT coefficients */
    for(i=0; i<lFAC; i++) x[i] = (float)td->fac[frame_index*LFAC_1024+i];

    /* apply gains */
    for(i=0; i<lFAC; i++)
    {
      x[i] *= gain;
    }
    for(i=0; i<lFAC/4; i++)
    {
      x[i] *= st->FAC_alfd[i];
    }

    /* Inverse DCT4 */
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(st->hTcxMdct, lFAC), x, xn1);
    smulr2r((2.0f/(float)lFAC), xn1, xn1, lFAC);

    set_zero(xn1+lFAC, lFAC);

    E_LPC_a_weight(A+(M+1), Ap, GAMMA1, M);       /* Wz of old LPC */
    E_UTIL_synthesis(Ap, xn1, xn1, 2*lFAC, xn1+lFAC, 0);

    /* add folded ACELP + ZIR */
    for (i=0; i<lFAC; i++) 
    {
      xn1[i] += facelp[i];
    }

  }

 /*-----------------------------------------------------------*
  * TCX gain, windowing, overlap and add.                     *
  *-----------------------------------------------------------*/

  for (i=0; i<L_frame+(2*lFAC); i++)
  {
    xn_buf[i] *= gain_tcx;
  }
  for (i=0; i<(2*lfacPrev); i++)
  {
    xn_buf[i+lFAC-lfacPrev] *= sinewindowPrev[i];
  }
  for ( i = 0 ; i < lFAC-lfacPrev; i++ ) {
    xn_buf[i] = 0.0f;
  }

  if (st->last_mode != 0)    /* if previous frame is not ACELP */
  {
    /* overlap-add with previous TCX frame */
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    for (i=lFAC-lfacPrev; i<(lFAC+lfacPrev); i++) xn_buf[i] += st->old_xnq[1+i];
#else
    for (i=0; i<(2*lFAC); i++) xn_buf[i] += st->old_xnq[1+i];
#endif

    mem_xnq = st->old_xnq[0];
  }
  else
  {
    /* aliasing cancellation with FAC */

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  for (i=lFAC-lfacPrev; i<(lFAC+lfacPrev); i++) {
#else
  for (i=0; i<(2*lFAC); i++) {
#endif
      xn_buf[i+lFAC] += xn1[i];
	}
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    mem_xnq = st->old_xnq[lfacPrev];
#else
    mem_xnq = st->old_xnq[lFAC];
#endif
  }

  /* update old_xnq for next frame */
  mvr2r(xn_buf+L_frame-1, st->old_xnq, 1+(2*lFAC));

  for (i=0; i<(2*lFAC); i++)
  {
    xn_buf[i+L_frame] *= sinewindow[(2*lFAC)-1-i];
  }

 /*-----------------------------------------------------------*
  * Resynthesis of past overlap (FAC method only)             *
  *-----------------------------------------------------------*/

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    if (st->last_mode != 0)    /* if previous frame is TCX or FD */
#else
    if (st->last_mode > 0)    /* if previous frame is TCX */
#endif
    {

#ifndef USAC_REFSOFT_COR1_NOFIX_06
      mvr2r(xn_buf+lFAC-lfacPrev, synth-lfacPrev, lfacPrev);
#else
      mvr2r(xn_buf, synth-lFAC, lFAC);
#endif

      for(i=0; i<M+lFAC; i++)
      {
        buf[i] = synth[i-M-lFAC] - (PREEMPH_FAC*synth[i-M-lFAC-1]);
      }

      p_A = st->old_Aq;
      TTT = lFAC%L_SUBFR;
      if (TTT!=0)
      {
        E_UTIL_residu(p_A, &buf[M], &exc[-lFAC], TTT);
        p_A += (M+1);
      }

      for (i_subfr=TTT; i_subfr<lFAC; i_subfr+=L_SUBFR)
      {
        E_UTIL_residu(p_A, &buf[M+i_subfr], &exc[i_subfr-lFAC], L_SUBFR);
        p_A += (M+1);
      }
    }

#ifdef USAC_REFSOFT_COR1_NOFIX_06
    if (st->last_mode < 0 ) {
      mvr2r(xn_buf, synth-lFAC, lFAC);
    }
#endif

 /*-----------------------------------------------------------*
  * find synthesis and excitation for ACELP frame             *
  *-----------------------------------------------------------*/

  mvr2r(xn, synth, L_frame);

  mvr2r(synth-M-1, xn-M-1, M+1);
  tmp = xn[-M-1];
  E_UTIL_f_preemph(xn-M, PREEMPH_FAC, M+L_frame, &tmp);

  p_A = A+(2*(M+1));

  E_UTIL_residu(p_A, xn, exc, L_frame);

  /* update old_Aq */
  mvr2r(p_A, st->old_Aq, M+1);
  mvr2r(p_A, st->old_Aq+M+1, M+1);

  return;
}



#ifdef UNIFIED_RANDOMSIGN

static float randomSign(unsigned int *seed)
{
  float sign = 0.f;
  *seed = ((*seed) * 69069) + 5;
  if ( ((*seed) & 0x10000) > 0) {
    sign = -1.f;
  } else {
    sign = +1.f;
  }
  return sign;
}

#else

static void rnd_sgn16(short *seed, float *xri, int lg)
{
  int i;

  for (i=0; i<lg; i++)
  {
    /* random phase from 0 to 15 */
    if(E_UTIL_random(seed)>=0)
    {
      xri[i] = 1.f;
    }
    else
    {
      xri[i] = -1.f;
    }
  }
  return;
}

#endif
