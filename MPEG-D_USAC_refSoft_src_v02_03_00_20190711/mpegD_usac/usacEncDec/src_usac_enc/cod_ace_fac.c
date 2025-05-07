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

$Id: cod_ace_fac.c,v 1.5 2011-03-10 23:07:00 gournape2 Exp $
*******************************************************************************/

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../src_usac/acelp_plus.h"


void coder_acelp_fac(
  float A[],         /* input: coefficients 4xAz[M+1]   */
  float Aq[],        /* input: coefficients 4xAz_q[M+1] */
  float speech[],    /* input: speech[-M..lg]           */
  float wsig[],      /* input: wsig[-1..lg]             */
  float synth[],     /* out:   synth[-128..lg]          */
  float wsyn[],      /* out:   synth[-128..lg]          */
  short codec_mode,    /* input: AMR_WB+ mode (see cnst.h)*/
  LPD_state *LPDmem,
  int lDiv,          /* input: length of ACELP frame    */
  float norm_corr,
  float norm_corr2,
  int T_op,          /* input: open-loop LTP            */  
  int T_op2,         /* input: open-loop LTP            */  
  int	pit_adj,
  int *prm)          /* output: acelp parameters        */
{
  int i, i_subfr, select, nbits, t;
  int T0, T0_min, T0_max, index, pit_flag;
  long int T0_frac;
  float tmp, ener, max, mean_ener_code;
  float gain_pit, gain_code, voice_fac, gain1, gain2;
  float g_corr[5], g_corr2[2]; /*norm_corr, norm_corr2*/
  float *p_A, *p_Aq, Ap[M+1];
  float h1[L_SUBFR];
  float code[L_SUBFR];
  short code3GPP[L_SUBFR];
  float error[M+L_SUBFR+8];
  float cn[L_SUBFR];
  float xn[L_SUBFR];
  float xn2[L_SUBFR];
  float dn[L_SUBFR];        /* Correlation between xn and h1      */
  float y0[L_SUBFR];        /* ZIR in weighted domain             */
  float y1[L_SUBFR];        /* Filtered adaptive excitation       */
  float y2[L_SUBFR];        /* Filtered adaptive excitation       */
  int PIT_MIN;				/* Minimum pitch lag with resolution 1/4      */
  int PIT_FR2;				/* Minimum pitch lag with resolution 1/2      */
  int PIT_FR1;				/* Minimum pitch lag with resolution 1        */
  int PIT_MAX;				/* Maximum pitch lag                          */
  float exc_buf[(3*L_DIV_1024)+1], *exc;
  float mem_Txn, mem_Txnq;

  /* Variables dependent on the frame length */
  int lFAC;

  /* Set variables dependent on frame length */
  lFAC   = lDiv/2;

  if (LPDmem->mode > 0)
  {
    /* past frame is TCX, transmit FAC parameters */
    for (i=0; i<lFAC; i++)
    {
      prm[i] = LPDmem->RE8prm[i];
    }
    prm += lFAC;
  }

  exc = exc_buf+(2*lDiv);

  mvr2r(LPDmem->Aexc, exc_buf, 2*lDiv);
  mvr2r(&(LPDmem->syn[M]), synth-128, 128);
  mvr2r(&(LPDmem->wsyn[1]), wsyn-128, 128);

  if (lDiv==192)
  {
    nbits = ((NBITS_CORE_768[codec_mode]-NBITS_MODE)/4) - NBITS_LPC;
  }
  else
  {
    nbits = ((NBITS_CORE_1024[codec_mode]-NBITS_MODE)/4) - NBITS_LPC;
  }

  if(pit_adj ==0) {
    PIT_MIN = PIT_MIN_12k8;
    PIT_FR2 = PIT_FR2_12k8;			
    PIT_FR1 = PIT_FR1_12k8;			
    PIT_MAX = PIT_MAX_12k8;			
  }
  else {
    i = (((pit_adj*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8;
    PIT_MIN = PIT_MIN_12k8 + i;
    PIT_FR2 = PIT_FR2_12k8 - i;
    PIT_FR1 = PIT_FR1_12k8;
    PIT_MAX = PIT_MAX_12k8 + (6*i);
  }
  T_op *= OPL_DECIM;
  T_op2 *= OPL_DECIM;
  /* range for closed loop pitch search in 1st subframe */
  T0_min = T_op - 8;

  t = MIN(T_op, T_op2) - 4;
  if (T0_min < t) T0_min = t;

  if (T0_min < PIT_MIN) {
    T0_min = PIT_MIN;
  }
  T0_max = T0_min + 15;

  t = MAX(T_op, T_op2) + 4;
  if (T0_max > t) T0_max = t;

  if (T0_max > PIT_MAX) {
    T0_max = PIT_MAX;
    T0_min = T0_max - 15;
  }
 /*------------------------------------------------------------------------*
  * Find and quantize mean_ener_code for gain quantizer (q_gain2_live.c)   *
  * This absolute reference replace the gains prediction.                  *
  * This modification for AMR_WB+ have the following advantage:            *
  * - better quantization on attacks (onset, drum impulse, etc.)           *
  * - robust on packet lost.                                               *
  * - independent of previous mode (can be TCX with alg. 7 bits quantizer).*
  *------------------------------------------------------------------------*/
  max=0.0;
  mean_ener_code = 0.0;
  p_Aq = Aq;
  for (i_subfr=0; i_subfr<lDiv; i_subfr+=L_SUBFR) {
    E_UTIL_residu(p_Aq, &speech[i_subfr], &exc[i_subfr], L_SUBFR);
    ener = 0.01f;
    for (i=0; i<L_SUBFR; i++) {
      ener += exc[i+i_subfr]*exc[i+i_subfr];
    }
    ener = 10.0f*(float)log10(ener/((float)L_SUBFR));
    if (ener < 0.0) {
      ener = 0.0;          /* ener in log (0..90dB) */
    }
    if (ener > max) {
      max = ener;
    }
    mean_ener_code += 0.25f*ener;
    p_Aq += (M+1);
  } 
  /* reduce mean energy on voiced signal */
  mean_ener_code -= 5.0f*norm_corr;
  mean_ener_code -= 5.0f*norm_corr2;

  /* quantize mean energy with 2 bits : 18, 30, 42 or 54 dB */
  tmp = (mean_ener_code-18.0f) / 12.0f;
  index = (int)floor(tmp + 0.5);
  if (index < 0) {
    index = 0;
  }
  if (index > 3) {
    index = 3;
  }
  mean_ener_code = (((float)index) * 12.0f) + 18.0f;
  /* limit mean energy to be able to quantize attack (table limited to +24dB) */
  while ((mean_ener_code < (max-27.0)) && (index < 3)) {
    index++;
    mean_ener_code += 12.0;
  }
  *prm = index;      prm++;

 /*------------------------------------------------------------------------*
  *          Loop for every subframe in the analysis frame                 *
  *------------------------------------------------------------------------*
  *  To find the pitch and innovation parameters. The subframe size is     *
  *  L_SUBFR and the loop is repeated L_FRAME_PLUS/L_SUBFR times.          *
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
  p_Aq = Aq;
  for (i_subfr=0; i_subfr<lDiv; i_subfr+=L_SUBFR) 
  {
    pit_flag = i_subfr;
    if ((lDiv==256) && (i_subfr == (2*L_SUBFR))) {
      pit_flag = 0;
      /* range for closed loop pitch search in 3rd subframe */
      T0_min = T_op2 - 8;

      t = MIN(T_op, T_op2) - 4;
      if (T0_min < t) T0_min = t;

      if (T0_min < PIT_MIN) {
        T0_min = PIT_MIN;
      }
      T0_max = T0_min + 15;

      t = MAX(T_op, T_op2) + 4;
      if (T0_max > t) T0_max = t;

      if (T0_max > PIT_MAX) {
        T0_max = PIT_MAX;
        T0_min = T0_max - 15;
      }
    }
    /*-----------------------------------------------------------------------*
             Find the target vector for pitch search:
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                 |------|  res[n]
     speech[n]---| A(z) |--------
                 |------|       |   |-------------|
                       zero -- (-)--| 1/A(z/gamma)|----- target xn[]
                       exc          |-------------|
    Instead of subtracting the zero-input response of filters from
    the weighted input speech, the above configuration is used to
    compute the target vector.
    *-----------------------------------------------------------------------*/
    /* find WSP (normaly done for pitch ol) */
   
    mvr2r(&wsig[i_subfr], xn, L_SUBFR);

    /* find ZIR in weighted domain */
    mvr2r(&synth[i_subfr-M], error, M);
    set_zero(error+M, L_SUBFR);
    E_UTIL_synthesis(p_Aq, error+M, error+M, L_SUBFR, error, 0);
    E_LPC_a_weight(p_A, Ap, GAMMA1, M);
    E_UTIL_residu(Ap, error+M, xn2, L_SUBFR);

    tmp = wsyn[i_subfr-1];
    E_UTIL_deemph(xn2, TILT_FAC, L_SUBFR, &tmp);
    mvr2r(xn2, y0, L_SUBFR);

    /* target xn = wsp - ZIR in weighted domain */
    for (i=0; i<L_SUBFR; i++) {
      xn[i] -= xn2[i];
    }
    /* fill current exc with residu */
    E_UTIL_residu(p_Aq, &speech[i_subfr], &exc[i_subfr], L_SUBFR);
   /*--------------------------------------------------------------*
    * Find target in residual domain (cn[]) for innovation search. *
    *--------------------------------------------------------------*/
    /* first half: xn[] --> cn[] */
    set_zero(code, M);
    mvr2r(xn, code+M, L_SUBFR/2);
    tmp = 0.0;
    E_UTIL_f_preemph(code+M, TILT_FAC, L_SUBFR/2, &tmp);
    E_LPC_a_weight(p_A, Ap, GAMMA1, M);
    E_UTIL_synthesis(Ap, code+M, code+M, L_SUBFR/2, code, 0);
    E_UTIL_residu(p_Aq, code+M, cn, L_SUBFR/2);
    /* second half: res[] --> cn[] (approximated and faster) */
    mvr2r(&exc[i_subfr+(L_SUBFR/2)], cn+(L_SUBFR/2), L_SUBFR/2);
    /*---------------------------------------------------------------*
     * Compute impulse response, h1[], of weighted synthesis filter  *
     *---------------------------------------------------------------*/
    E_LPC_a_weight(p_A, Ap, GAMMA1, M);
    set_zero(h1, L_SUBFR);
    mvr2r(Ap, h1, M+1);
    E_UTIL_synthesis(p_Aq, h1, h1, L_SUBFR, &h1[M+1], 0);
    tmp = 0.0;
    E_UTIL_deemph(h1, TILT_FAC, L_SUBFR, &tmp);
    /*----------------------------------------------------------------------*
     *                 Closed-loop fractional pitch search                  *
     *----------------------------------------------------------------------*/
    /* find closed loop fractional pitch  lag */
    T0 = E_GAIN_closed_loop_search(&exc[i_subfr], xn, h1, T0_min, T0_max, &T0_frac,
                     pit_flag, PIT_FR2, PIT_FR1);
      /* encode pitch lag */
    if (pit_flag == 0) {  /* if 1st/3rd subframe */
       /*--------------------------------------------------------------*
        * The pitch range for the 1st/3rd subframe is encoded with     *
        * 9 bits and is divided as follows:                            *
        *   PIT_MIN to PIT_FR2-1  resolution 1/4 (frac = 0,1,2 or 3)   *
        *   PIT_FR2 to PIT_FR1-1  resolution 1/2 (frac = 0 or 2)       *
        *   PIT_FR1 to PIT_MAX    resolution 1   (frac = 0)            *
        *--------------------------------------------------------------*/
        if (T0 < PIT_FR2) {
          index = T0*4 + T0_frac - (PIT_MIN*4);
        } else if (T0 < PIT_FR1) {
          index = T0*2 + (T0_frac>>1) - (PIT_FR2*2) + ((PIT_FR2-PIT_MIN)*4);
        } else {
          index = T0 - PIT_FR1 + ((PIT_FR2-PIT_MIN)*4) + ((PIT_FR1-PIT_FR2)*2);
        }
        /* find T0_min and T0_max for subframe 2 and 4 */
        T0_min = T0 - 8;
        if (T0_min < PIT_MIN) {
          T0_min = PIT_MIN;
        }
        T0_max = T0_min + 15;
        if (T0_max > PIT_MAX) {
          T0_max = PIT_MAX;
          T0_min = T0_max - 15;
        }
      } else {     /* if subframe 2 or 4 */
       /*--------------------------------------------------------------*
        * The pitch range for subframe 2 or 4 is encoded with 6 bits:  *
        *   T0_min  to T0_max     resolution 1/4 (frac = 0,1,2 or 3)   *
        *--------------------------------------------------------------*/
        i = T0 - T0_min;
        index = i*4 + T0_frac;
      }
      *prm = index;      prm++;
   
   /*-----------------------------------------------------------------*
    * - find unity gain pitch excitation (adaptive codebook entry)    *
    *   with fractional interpolation.                                *
    * - find filtered pitch exc. y1[]=exc[] convolved with h1[])      *
    * - compute pitch gain1                                           *
    *-----------------------------------------------------------------*/
    /* find pitch exitation */
    pred_lt4(&exc[i_subfr], T0, T0_frac, L_SUBFR+1);
    E_UTIL_f_convolve(&exc[i_subfr], h1, y1);
    gain1 = E_ACELP_xy1_corr(xn, y1, g_corr);
    /* find energy of new target xn2[] */
    E_ACELP_codebook_target_update(xn, xn2, y1, gain1);
    ener = 0.0;
    for (i=0; i<L_SUBFR; i++) {
      ener += xn2[i]*xn2[i];
    }
   /*-----------------------------------------------------------------*
    * - find pitch excitation filtered by 1st order LP filter.        *
    * - find filtered pitch exc. y2[]=exc[] convolved with h1[])      *
    * - compute pitch gain2                                           *
    *-----------------------------------------------------------------*/
    /* find pitch excitation with lp filter */
    for (i=0; i<L_SUBFR; i++) {
      code[i] = (float)(0.18*exc[i-1+i_subfr] + 0.64*exc[i+i_subfr] + 0.18*exc[i+1+i_subfr]);
    }
    E_UTIL_f_convolve(code, h1, y2);
    gain2 = E_ACELP_xy1_corr(xn, y2, g_corr2);
    /* find energy of new target xn2[] */
    E_ACELP_codebook_target_update(xn, xn2, y2, gain2);
    tmp = 0.0;
    for (i=0; i<L_SUBFR; i++) {
      tmp += xn2[i]*xn2[i];
    }
   /*-----------------------------------------------------------------*
    * use the best prediction (minimise quadratic error).             *
    *-----------------------------------------------------------------*/
    if (tmp < ener) {
      /* use the lp filter for pitch excitation prediction */
      select = 0;
      mvr2r(code, &exc[i_subfr], L_SUBFR);
      mvr2r(y2, y1, L_SUBFR);
      gain_pit = gain2;
      g_corr[0] = g_corr2[0];
      g_corr[1] = g_corr2[1];
    } else {
      /* no filter used for pitch excitation prediction */
      select = 1;
      gain_pit = gain1;
    }
    *prm = select;      prm++;

   /*-----------------------------------------------------------------*
    * - update target vector for ACELP codebook search                *
    *-----------------------------------------------------------------*/
    E_ACELP_codebook_target_update(xn, xn2, y1, gain_pit);
    E_ACELP_codebook_target_update(cn, cn, &exc[i_subfr], gain_pit);
   /*-----------------------------------------------------------------*
    * - include fixed-gain pitch contribution into impulse resp. h1[] *
    *-----------------------------------------------------------------*/

    tmp = 0.0;
    E_UTIL_f_preemph(h1, TILT_CODE, L_SUBFR, &tmp);
    if (T0_frac > 2) {
      T0++;
    }
    E_GAIN_f_pitch_sharpening(h1, T0);
   /*-----------------------------------------------------------------*
    * - Correlation between target xn2[] and impulse response h1[]    *
    * - Innovative codebook search                                    *
    *-----------------------------------------------------------------*/
    E_ACELP_xh_corr(xn2, dn, h1);

    if (codec_mode == MODE_8k0) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 12, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_8k8){
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 16, 0, prm);
      prm += 4;      
    } else if (codec_mode == MODE_9k6) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 20, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_11k2) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 28, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_12k8) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 36, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_14k4) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 44, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_16k) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 52, 0, prm);
      prm += 4;
    } else if (codec_mode == MODE_18k4) {
      E_ACELP_4t(dn, cn, h1, code3GPP, y2, 64, 0, prm);
      prm += 8;
    } else {
      printf("invalid mode for acelp frame!\n");
      exit(0);
    }

    {
      int g;
      for(g=0;g<L_SUBFR;g++) {
        code[g] = (float)(code3GPP[g]/512);
      }
    }

   /*-------------------------------------------------------*
    * - Add the fixed-gain pitch contribution to code[].    *
    *-------------------------------------------------------*/
    tmp = 0.0;
    E_UTIL_f_preemph(code, TILT_CODE, L_SUBFR, &tmp);
    E_GAIN_f_pitch_sharpening(code, T0);

   /*----------------------------------------------------------*
    *  - Compute the fixed codebook gain                       *
    *  - quantize fixed codebook gain                          *
    *----------------------------------------------------------*/
    E_ACELP_xy2_corr(xn, y1, y2, g_corr);
    index = q_gain2_plus(code, L_SUBFR, &gain_pit, &gain_code, g_corr, mean_ener_code/*, &c_out[i_subfr/L_SUBFR]*/);
    *prm = index;      prm++;

   /*----------------------------------------------------------*
    * - voice factor (for pitch enhancement)                   *
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
   /*------------------------------------------------------*
    * - Update filter's memory "mem_w0" for finding the    *
    *   target vector in the next subframe.                *
    * - Find the total excitation                          *
    * - Find synthesis speech to update mem_syn[].         *
    *------------------------------------------------------*/

    for (i = 0; i < L_SUBFR;  i++) 
	{
      wsyn[i+i_subfr] = y0[i] + (gain_pit*y1[i]) + (gain_code*y2[i]);
	}

    for (i = 0; i < L_SUBFR;  i++) {
      exc[i+i_subfr] = gain_pit*exc[i+i_subfr] + gain_code*code[i];
    }

   /*----------------------------------------------------------*
    * - compute the synthesis speech                           *
    *----------------------------------------------------------*/
    E_UTIL_synthesis(p_Aq, &exc[i_subfr], &synth[i_subfr], L_SUBFR, &synth[i_subfr-M], 0);
    p_A += (M+1);
    p_Aq += (M+1);
  } /* end of subframe loop */

  /* update LPD memory */
  mvr2r(exc-lDiv, LPDmem->Aexc, 2*lDiv);
  mvr2r(synth+lDiv-(M+128), LPDmem->syn, M+128);
  mvr2r(wsyn+lDiv-(1+128), LPDmem->wsyn, 1+128);
  mvr2r(p_Aq-(2*(M+1)), LPDmem->Aq, 2*(M+1));
  mvr2r(p_A-(2*(M+1)), LPDmem->Ai, 2*(M+1));

  /* update Txn and Txnq for TCX frame (use Aq) */
  mem_Txn = LPDmem->Txn[128-1];
  mem_Txnq = LPDmem->Txnq_fac;
 
  /* process subframes for memory update */
  p_Aq = Aq;
  for (i_subfr=0; i_subfr<(lDiv-2*L_SUBFR); i_subfr+=L_SUBFR) 
  {
    E_LPC_a_weight(p_Aq, Ap, GAMMA1, M);

    mvr2r(&speech[i_subfr], error, L_SUBFR);
    E_UTIL_deemph(error, TILT_FAC, L_SUBFR, &mem_Txn);

    mvr2r(&synth[i_subfr], error, L_SUBFR);
    E_UTIL_deemph(error, TILT_FAC, L_SUBFR, &mem_Txnq);

    p_Aq += (M+1);
  }

  /* last 2 subfr required by next TCX frame */
  LPDmem->Txnq[0] = mem_Txnq;
  for (i_subfr=0; i_subfr<(2*L_SUBFR); i_subfr+=L_SUBFR) 
  {
    E_LPC_a_weight(p_Aq, Ap, GAMMA1, M);

    mvr2r(&speech[i_subfr+(lDiv-2*L_SUBFR)], &(LPDmem->Txn[i_subfr]), L_SUBFR);
    E_UTIL_deemph(&(LPDmem->Txn[i_subfr]), TILT_FAC, L_SUBFR, &mem_Txn);

    mvr2r(&synth[i_subfr+(lDiv-2*L_SUBFR)], &(LPDmem->Txnq[1+i_subfr]), L_SUBFR);
    E_UTIL_deemph(&(LPDmem->Txnq[1+i_subfr]), TILT_FAC, L_SUBFR, &mem_Txnq);

    p_Aq += (M+1);
  }
  LPDmem->Txnq_fac = mem_Txnq;

  /* wZIR for next TCX frame (Wz for 2nd subfr is extrapolated) */
  E_LPC_a_weight(p_Aq, Ap, GAMMA1, M);  

  mvr2r(&synth[lDiv-M], error, M);
  for (i_subfr=(2*L_SUBFR); i_subfr<(4*L_SUBFR); i_subfr+=L_SUBFR) 
  {
    set_zero(error+M, L_SUBFR);

    E_UTIL_synthesis(p_Aq, error+M, error+M, L_SUBFR, error, 0);
    mvr2r(error+M, &(LPDmem->Txnq[1+i_subfr]), L_SUBFR);
    E_UTIL_deemph(&(LPDmem->Txnq[1+i_subfr]), TILT_FAC, L_SUBFR, &mem_Txnq);
    mvr2r(error+L_SUBFR, error, M);
  }

  /* mode ACELP = 0, 1/2/3 for TCX20/40/80 */
  LPDmem->mode = 0;

  /* number of ACELP bits used */
  LPDmem->nbits = nbits;

  return ;
}



