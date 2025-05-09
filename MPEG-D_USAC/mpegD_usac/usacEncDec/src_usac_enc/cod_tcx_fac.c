/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Guillaume Fuchs (Fraunhofer IIS)

and edited by:  Philippe Gournay
				Jeremie Lecomte (Fraunhofer IIS)

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

*******************************************************************************/

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "acelp_plus.h"
#include "re8.h"

#include "proto_func.h"

#include "usac_tcx_mdct.h"

static int ibark2[11] = {0, 8, 16, 24, 36, 50, 68, 92, 126, 176, 256};


void coder_tcx_fac(
  float Ai[],        /* input: coefficients NxAz[M+1]   */
  float A[],         /* input: coefficients NxAz_q[M+1] */
  float speech[],    /* input: speech[-M..lg]           */
  float wsig[],      /* input: wsig[-1..lg]             */
  float synth[],     /* out:   synth[-128..lg]          */
  float wsyn[],      /* out:   synth[-128..lg]          */
  int L_frame,       /* input: frame length             */
  int lDiv,          /* input: length of ACELP frame    */
  int nb_bits,       /* input: number of bits allowed   */
  LPD_state *LPDmem,
  int prm[],         /* output: tcx parameters          */
  int *n_param,
  int lfacPrev,
  int lfacNext,
  HANDLE_TCX_MDCT hTcxMdct/* in/out: handle for MDCT transform */
  )
{
  int i, k, n, mode, i_subfr, lg, lext, index, sqTargetBits;
  float tmp, gain, fac_ns, ener, gain_tcx, nsfill_en_thres;
  float *p_A, Ap[M+1];
  const float *sinewindowPrev, *sinewindowNext;
  float mem_xnq;
  float *xn;
  float xn1[2*LFAC_1024],xn_buf[128+L_FRAME_1024+128];
  float x[N_MAX], x_tmp[N_MAX], en[N_MAX];
  float sqGain;
  float gainPrev, gainNext;
  float alfd_gains[N_MAX/(4*8)];
  float sqEnc[N_MAX];
  int sqQ[N_MAX];
  float sqErrorNrg;
  int maxK;
  float gainlpc[N_MAX], gainlpc2[N_MAX];

  float facelp[LFAC_1024];
  float xn2[2*LFAC_1024], facwindow[2*LFAC_1024];
  float x1[LFAC_1024], x2[LFAC_1024];
  int y[LFAC_1024];
  int TTT;

  /* Variables dependent on the frame length */
  int lFAC;

  /* Set variables dependent on frame length */
  lFAC   = lDiv/2;

  set_zero(xn_buf,128+L_frame+128);

  /* mode TCX: 1/2/3 = TCX20/40/80 */
  mode = L_frame/lDiv;
  if (mode > 2 ) mode = 3;
  
  /* if past frame is ACELP, keep space for FAC parameters */
  if (LPDmem->mode == 0)
  {
    prm += lfacPrev;
  }
  if (lfacPrev == 64) {
    sinewindowPrev = sineWindow128;
  }
  else if (lfacPrev == 48) {
    sinewindowPrev = sineWindow96;
  }
  else if (lfacPrev == 96) {
    sinewindowPrev = sineWindow192;
  }
  else {
    sinewindowPrev = sineWindow256;
  }
  if (lfacNext == 64) {
    sinewindowNext = sineWindow128;
  }
  else if (lfacNext == 48) {
    sinewindowNext = sineWindow96;
  }
  else if (lfacNext == 96) {
    sinewindowNext = sineWindow192;
  }
  else {
    sinewindowNext = sineWindow256;
  }

  lg = L_frame;
  lext = lFAC;
  /* set target pointer (pointer to current frame) */
  xn = xn_buf + lFAC;

  *n_param = lg;               /* number of TCX parameters */

  sqTargetBits = nb_bits-3-7;      /* target for SQ */

  /*-----------------------------------------------------------*
   * Find target xn[] (weighted speech when prev mode is TCX)  *
   * Note that memory isn't updated in the overlap area.       *
   *-----------------------------------------------------------*/

  for(i=0; i<lFAC; i++)
  {
    xn_buf[i] = LPDmem->Txn[i+128-lFAC];
  }

  /* target for L_frame */
  mvr2r(speech, xn, L_frame+lFAC);

  tmp = xn[-1];

  E_UTIL_deemph(xn, TILT_FAC, L_frame, &tmp);

  /* update LPD.memory for next frame */
  mvr2r(&xn[L_frame-128], LPDmem->Txn, 128);

  /* target for lext */
  mvr2r(&speech[L_frame], &xn[L_frame], lext);
  E_UTIL_deemph(&xn[L_frame], TILT_FAC, lext, &tmp);

  /* save signal of overlap area */
  for (i=0; i<M+lfacPrev; i++)
  {
    xn1[i] = xn_buf[lFAC-M+i];
  }
  for ( i = 0 ; i < M + lfacNext ; i++) {
    xn2[i] = xn_buf[L_frame-M+i];
  }

  /* xn[] windowing for TCX overlap */
  if (LPDmem->mode >= -1)    /* windowing also when from AAC */
  {

    for ( i=0; i < lFAC-lfacPrev ; i++ ) {
      xn_buf[i] = 0.0f;
    }
    for (i= lFAC-lfacPrev ; i<(lFAC+lfacPrev); i++) {
      xn_buf[i] *= sinewindowPrev[i-lFAC+lfacPrev];
    }
    for (i=0; i<(2*lfacNext); i++) {
      xn_buf[L_frame+lFAC-lfacNext+i] *= sinewindowNext[(2*lfacNext)-1-i];
    }
    for (i=0; i < lFAC-lfacNext ; i++ ) {
      xn_buf[L_frame+lFAC+lfacNext + i] = 0.0f;
    }

  }

 /*-----------------------------------------------------------*
  * Compute MDCT for xn_buf[].                                *
  *-----------------------------------------------------------*/
  TCX_MDCT_Apply(hTcxMdct, xn_buf, x, (2*lFAC), L_frame-(2*lFAC), (2*lFAC));
  smulr2r(1., x, x, lg);

 /*-----------------------------------------------------------*
  * Pre-shaping in frequency domain using weighted LPC (Wz)   *
  *-----------------------------------------------------------*/
  /* old NS spectral gain (could be computed every LPC frame) */
  E_LPC_a_weight(A+(M+1), Ap, GAMMA1, M);
  lpc2mdct(Ap, M, gainlpc, (FDNS_NPTS_1024*lDiv)/L_DIV_1024);

  /* new NS spectral gain (could be computed every LPC frame) */
  E_LPC_a_weight(A+(2*(M+1)), Ap, GAMMA1, M);
  lpc2mdct(Ap, M, gainlpc2, (FDNS_NPTS_1024*lDiv)/L_DIV_1024);

  mdct_IntPreShaping(x, lg, (FDNS_NPTS_1024*lDiv)/L_DIV_1024, gainlpc, gainlpc2);

  /*Save orginal spectrum*/
  for (i=0; i<lg; i++) 
  {
    x_tmp[i] = x[i];
  }

 /*-----------------------------------------------------------*
  * Spectral algebraic quantization                           *
  * with adaptive low frequency emphasis/deemphasis.          *
  *-----------------------------------------------------------*/

  AdaptLowFreqEmph(x, lg);

  sqGain = SQ_gain(x, sqTargetBits, lg);

  for(i=0; i<lg; i++) 
  {
    sqEnc[i] = x[i]/sqGain;

    if(sqEnc[i]>0.f)
      sqQ[i] = ((int) (0.5f + sqEnc[i]));
    else
      sqQ[i] = ((int) (-0.5f + sqEnc[i]));
  }

  /* Save quantized Values */
  for(i=0; i<lg; i++) 
  {
    prm[i+2] = sqQ[i];
    x[i] = (float)sqQ[i];
  }

 /*-----------------------------------------------------------*
  * Find noise fill factor.                                   *
  *  - use amplitude envelop (1dB/25Hz) to detect weak zone.  *
  *  - set noise fill-in factor to the RMS of the weak zone.  *
  *-----------------------------------------------------------*/

  for(i=0; i<lg; i++) en[i] = x[i]*x[i];

  if (mode == 3) tmp = 0.9441f;
  else if (mode == 2) tmp = 0.8913f;
  else tmp = 0.7943f;

  ener = 0.0f;
  for(i=0; i<lg; i++)
  {
    if (en[i] > ener) ener = en[i];
    en[i] = ener;
    ener *= tmp;
  }
  ener = 0.0f;
  for(i=lg-1; i>=0; i--)
  {
    if (en[i] > ener) ener = en[i];
    en[i] = ener;
    ener *= tmp;
  }

  nsfill_en_thres = 0.707f;

  tmp = 0.0625f;
  k = 1;
  for(i=0; i<lg; i++)
  {
    if (en[i] <= nsfill_en_thres)
    {
      tmp += sqEnc[i]*sqEnc[i];
      k++;
    }
  }
  fac_ns = (float)sqrt(tmp/(float)k);

 /*-----------------------------------------------------------*
  * Adaptive low frequency deemphasis.                        *
  * compute TCX gain for SNR computation and quantization.    *
  *-----------------------------------------------------------*/

  AdaptLowFreqDeemph(x, lg, alfd_gains);

  gain_tcx = get_gain(x_tmp, x, lg);
  if (gain_tcx == 0.0f) gain_tcx = sqGain;

 /*-----------------------------------------------------------*
  * TCX SNR (can be replaced by noise level)                  *
  *-----------------------------------------------------------*/

  ener = 0.0001f;
  for(i=0; i<lg; i++)
  {
    tmp = x_tmp[i] - gain_tcx*x[i];
    ener += tmp*tmp;
  }

  tmp = (float)sqrt((ener*(2.0f/(float)lg))/(float)lg);

  for(i=0; i<L_frame; i++) wsyn[i] = wsig[i] + tmp;

 /*-----------------------------------------------------------*
  * Quantize TCX gain                                         *
  *-----------------------------------------------------------*/

  ener = 0.01f;
  for(i=0; i<lg; i++) ener += x[i]*x[i];

  tmp = 2.0f * (float)sqrt(ener) / (float)lg;
  gain = gain_tcx * tmp;

  /* quantize gain with step of 0.714 dB */
  index = (int)floor(0.5f + (28.0f * (float)log10(gain)));
  if (index < 0) index = 0;
  if (index > 127) index = 127;
  prm[1] = index;
  
  gain_tcx = (float)pow(10.0f, ((float)index)/28.0f) / tmp;

  /*Adjust noise filling level*/
  sqErrorNrg = 0.f;
  n=0;
  for(k=lg/2; k<lg;) 
  {
    tmp=0.f;
    
    maxK=MIN(lg,k+8);
    for(i=k; i<maxK; i++) 
    {
      tmp += sqQ[i]*sqQ[i];
    }
    if(tmp==0.f)
    {
      tmp = 0.f;
      for(i=k; i<maxK; i++) 
      {
        tmp += sqEnc[i]*sqEnc[i];
      }
      
      sqErrorNrg += (float)log10((tmp/(double)8)+0.000000001);
      n += 1;
    }
    k = maxK;
  }
  if (n>0) fac_ns = (float) pow(10., sqErrorNrg/(double)(2*n));
  else  fac_ns = 0.f;

  /* quantize noise factor (noise factor = 0.065 to 0.5) */
  tmp = 8.0f - (16.0f*fac_ns);

  index = (int)floor(tmp + 0.5);
  if (index < 0) {
    index = 0;
  }
  if (index > 7) {
    index = 7;
  }

  prm[0] = index;        /* fac_ns : 3 bits */

  fac_ns = 0.0625f*(8.0f - ((float)index));

 /*-----------------------------------------------------------*
  * Noise shaping in frequency domain (1/Wz)                  *
  *-----------------------------------------------------------*/
  mdct_IntNoiseShaping(x, lg, (FDNS_NPTS_1024*lDiv)/L_DIV_1024, gainlpc, gainlpc2);

 /*-----------------------------------------------------------*
  * Compute inverse MDCT of x[].                              *
  *-----------------------------------------------------------*/

  TCX_MDCT_ApplyInverse(hTcxMdct, x, xn_buf, (2*lFAC), L_frame-(2*lFAC), (2*lFAC));
  smulr2r((2.0f/lg), xn_buf, xn_buf, L_frame+(2*lFAC));

 /*-----------------------------------------------------------*
  * Find & quantize FAC frame in case of:                     *
  * - ACELP to TCX (xn1) using ACELP+ZIR data.                *
  * - TCX to ACELP (xn2)                                      *
  * using the AVQ and TCX scale factors.                      *
  *-----------------------------------------------------------*/

  /* build FAC window (should be in ROM) */
  for (i=0; i< lfacPrev ; i++)
  {
    facwindow[i] = sinewindowPrev[i]*sinewindowPrev[(2*lfacPrev)-1-i];
    facwindow[lfacPrev+i] = 1.0f - (sinewindowPrev[lfacPrev+i]*sinewindowPrev[lfacPrev+i]);
  }

  /* error of windowed TCX (FAC area) */
  for (i=0; i<lfacPrev; i++)
  {
    xn1[M+i] -= sqGain*xn_buf[lFAC+i]*sinewindowPrev[lfacPrev+i];
  }
  for ( i = 0 ; i < lfacNext ; i++ ) {
    xn2[M+i] -= sqGain*xn_buf[i+L_frame]*sinewindowNext[(2*lfacNext)-1-i];
  }

  /* past error */
  for (i=0; i<M; i++)
  {
    xn1[i] -= LPDmem->Txnq[1+128-M+i];     /* ACELP error */
    xn2[i] -= sqGain*xn_buf[L_frame-M+i];  /* TCX error */
  }

  /* windowing and folding of ACELP part including the ZIR */
  for (i=0; i<lfacPrev; i++)
  {
    facelp[i] = LPDmem->Txnq[1+128+i]*facwindow[lfacPrev+i]
      + LPDmem->Txnq[1+128-1-i]*facwindow[lfacPrev-1-i];
  }

  /* gain limitation to avoid spending bits on artefact */
  ener = 0.0f;
  for (i=0; i<lfacPrev; i++) ener += xn1[M+i]*xn1[M+i];
  ener *= 2.0f;
  tmp = 0.0f;
  for (i=0; i<lfacPrev; i++) tmp += facelp[i]*facelp[i];
  if (tmp > ener) gain = (float)sqrt(ener/tmp);
  else gain = 1.0f;

  /* remove folded ACELP part */
  for (i=0; i<lfacPrev; i++)
  {
    xn1[M+i] -= gain*facelp[i];
  }

  /* find target of left part (xn1) */
  E_LPC_a_weight(A+(M+1), Ap, GAMMA1, M);       /* Wz of old LPC */
  /* Wz of ACELP+TCX error */
  E_UTIL_residu(Ap, xn1+M, x1, lfacPrev);

  /* find target of right part (xn2) */
  E_LPC_a_weight(A+(2*(M+1)), Ap, GAMMA1, M);   /* Wz of new LPC */
  /* Wz of TCX error */
  E_UTIL_residu(Ap, xn2+M, x2, lfacNext);

  if ( ((lDiv==192) && (((lfacPrev!=96)  && (lfacPrev!=48)) || ((lfacNext!=96)  && (lfacNext!=48)))) ||
       ((lDiv==256) && (((lfacPrev!=128) && (lfacPrev!=64)) || ((lfacNext!=128) && (lfacNext!=64)))) )
  {
    printf("\nInvalid value for lfacPrev or lfacNext\n");
    exit(0);
  }
  /* DCT4 of input vector */
  if (lfacPrev==128)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 128), x1, x1);
  }
  else if (lfacPrev==96)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 96), x1, x1);
  }
  else if (lfacPrev==48)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 48), x1, x1);
  }
  else
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 64), x1, x1);
  }
  if (lfacNext==128)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 128), x2, x2);
  }
  else if (lfacNext==96)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 96), x2, x2);
  }
  else if (lfacNext==48)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 48), x2, x2);
  }
  else
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 64), x2, x2);
  }

  /* AVQ gain = gainSQ/2, sqrt()= fac MDCT */
  gainPrev = sqGain * 0.5f * (float)sqrt(((float)lfacPrev)/(float)L_frame);
  gainNext = sqGain * 0.5f * (float)sqrt(((float)lfacNext)/(float)L_frame);
  /* apply gains */
  for(i=0; i<lfacPrev; i++)
  {
    x1[i] /= gainPrev;
  }
  for ( i = 0 ; i < lfacNext ; i++ ) {
    x2[i] /= gainNext;
  }
  for(i=0; i<lfacPrev/4; i++)
  {
    k = i*lg/(8*lfacPrev);
    x1[i] /= alfd_gains[k];
  }
  for(i=0; i<lfacNext/4; i++)
  {
    k = i*lg/(8*lfacNext);
    x2[i] /= alfd_gains[k];
  }

  /* quantization of x2 with AVQ */
  for (i=0; i<lfacNext; i+=8) {
    RE8_PPV(&x2[i], &y[i]);
  }
  for(i=0; i<lfacNext; i++) LPDmem->RE8prm[i] = y[i];
  for(i=0; i<lfacNext; i++) x2[i] = (float)y[i];

  /* quantization of x1 with AVQ */
  for (i=0; i<lfacPrev; i+=8) {
    RE8_PPV(&x1[i], &y[i]);
  }

  for(i=0; i<lfacPrev; i++) x1[i] = (float)y[i];

  /* AVQ gain = gain_tcx/2, sqrt()= fac MDCT */
  gainPrev = gain_tcx * 0.5f * (float)sqrt(((float)lfacPrev)/(float)L_frame);
  gainNext = gain_tcx * 0.5f * (float)sqrt(((float)lfacNext)/(float)L_frame);

  /* apply gains */
  for(i=0; i<lfacPrev; i++)
  {
    x1[i] *= gainPrev;
  }
  for ( i = 0 ; i < lfacNext ; i++ ) {
    x2[i] *= gainNext;
  }
  for(i=0; i<lfacPrev/4; i++)
  {
    k = i*lg/(8*lfacPrev);
    x1[i] *= alfd_gains[k];
  }
  for(i=0; i<lfacNext/4; i++)
  {
    k = i*lg/(8*lfacNext);
    x2[i] *= alfd_gains[k];
  }

  /* inverse DCT4 */
  if (lfacPrev==128)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 128), x1, xn1);
  }
  else if (lfacPrev==96)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 96), x1, xn1);
  }
  else if (lfacPrev==48)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 48), x1, xn1);
  }
  else
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 64), x1, xn1);
  }
  if (lfacNext==128)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 128), x2, xn2);
  }
  else if (lfacNext==96)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 96), x2, xn2);
  }
  else if (lfacNext==48)
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 48), x2, xn2);
  }
  else
  {
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, 64), x2, xn2);
  }

  smulFLOAT((2.0f/(float)lfacPrev), xn1, xn1, lfacPrev);
  smulFLOAT((2.0f/(float)lfacNext), xn2, xn2, lfacNext);

  set_zero(xn1+lfacPrev, lfacPrev);
  set_zero(xn2+lfacNext, lfacNext);

  /* 1/Wz of xn1 */
  E_LPC_a_weight(A+(M+1), Ap, GAMMA1, M);       /* Wz of old LPC */
  E_UTIL_synthesis(Ap, xn1, xn1, 2*lfacPrev, xn1+lfacPrev, 0);

  /* 1/Wz of xn2 */
  E_LPC_a_weight(A+(2*(M+1)), Ap, GAMMA1, M);   /* Wz of new LPC */
  E_UTIL_synthesis(Ap, xn2, xn2, lfacNext, xn2+lfacNext, 0);

  /* add folded ACELP + ZIR */
  for (i=0; i<lfacPrev; i++)
  {
    xn1[i] += facelp[i];
  }

 /*-----------------------------------------------------------*
  * TCX gain, windowing, overlap and add.                     *
  *-----------------------------------------------------------*/

  for (i=0; i<L_frame+(2*lFAC); i++)
  {
	xn_buf[i] *= gain_tcx;
  }

  if (LPDmem->mode >= -1)    /* windowing also when from AAC */
  {
    for (i=0; i<(2*lfacPrev); i++)
    {
      xn_buf[i+lFAC-lfacPrev] *= sinewindowPrev[i];
    }
    for ( i = 0 ; i < lFAC-lfacPrev ; i++ ) {
      xn_buf[i] = 0.0f;
    }
  }
  for (i=0; i<(2*lfacNext); i++)
  {
    xn_buf[i+L_frame+lFAC-lfacNext] *= sinewindowNext[(2*lfacNext)-1-i];
  }
  for ( i = 0 ; i < lFAC-lfacNext; i++ ) {
    xn_buf[i+L_frame+lFAC+lfacNext] = 0.0f;
  }

  if (LPDmem->mode != 0)    /* if previous frame is not ACELP */
  {
    /* overlap-add with previous TCX frame */
    for (i=0; i<(2*lFAC); i++) {
      xn_buf[i] += LPDmem->Txnq[1+128-lFAC+i];
    }

    mem_xnq = LPDmem->Txnq[128-lFAC];
  }
  else
  {
    /* previous frame is ACELP, Forward the AC */
    for (i=0; i<lfacPrev; i++)
    {
      prm[i-lfacPrev] = y[i];
    }

    /* aliasing cancellation with FAC */
    for (i=0; i<(2*lfacPrev); i++)
    {
      xn_buf[i+lFAC] += xn1[i];
    }
    mem_xnq = LPDmem->Txnq[128];
  }

  /* update Txnq for next frame */
  mvr2r(xn_buf+L_frame+lFAC-128-1, LPDmem->Txnq, 1+256);

  /* aliasing cancellation for next ACELP frame */
  for (i=0; i<lfacNext; i++)
  {
    xn_buf[i+L_frame + (lFAC-lfacNext)] += xn2[i];
  }

 /*-----------------------------------------------------------*
  * Resynthesis of past overlap (FAC method only)             *
  *-----------------------------------------------------------*/

  if (LPDmem->mode > 0 /*|| LPDmem->mode == -1*/ )    /* if previous frame is TCX or FD */
  {

    /* resynthesis of past (overlap area) */
    E_UTIL_f_preemph(xn_buf, TILT_FAC, lFAC, &mem_xnq);

    p_A = LPDmem->Aq;

    /* Because LFAC is not necessarily a multiple of L_SUBFR */
    TTT = lFAC%L_SUBFR;
    if (TTT!=0)
    {
       mvr2r(&xn_buf[0], &(LPDmem->syn[M+128-lFAC]), TTT);
       E_UTIL_residu(p_A, &(LPDmem->syn[M+128-lFAC]), &(LPDmem->Aexc[(2*lDiv)-lFAC]), TTT);

       p_A += (M+1);
    }

    for (i_subfr=TTT; i_subfr<lFAC; i_subfr+=L_SUBFR) 
    {
      mvr2r(&xn_buf[i_subfr], &(LPDmem->syn[M+128-lFAC+i_subfr]), L_SUBFR);
      E_UTIL_residu(p_A, &(LPDmem->syn[M+128-lFAC+i_subfr]), &(LPDmem->Aexc[(2*lDiv)-lFAC+i_subfr]), L_SUBFR);
      p_A += (M+1);
    }

    /* weighting of past synthesis using uQ A (overlap area) */
    p_A = LPDmem->Ai;
    for (i_subfr=0; i_subfr<lFAC; i_subfr+=L_SUBFR) 
    {
      E_LPC_a_weight(p_A, Ap, GAMMA1, M);
      E_UTIL_residu(Ap, &(LPDmem->syn[M+128-lFAC+i_subfr]), &(LPDmem->wsyn[1+128-lFAC+i_subfr]), L_SUBFR);
      p_A += (M+1);
    }
    tmp = LPDmem->wsyn[0+128-lFAC];
    E_UTIL_deemph(&(LPDmem->wsyn[1+128-lFAC]), TILT_FAC, lFAC, &tmp);
  }

 /*-----------------------------------------------------------*
  * find synthesis and excitation for ACELP frame             *
  *-----------------------------------------------------------*/

  /* update Ai and Aq for next frame */
  k = ((L_frame/L_SUBFR)-2)*(M+1);
  mvr2r(Ai+k, LPDmem->Ai, 2*(M+1));

  mvr2r(A+(2*(M+1)), LPDmem->Aq, M+1);
  mvr2r(LPDmem->Aq, LPDmem->Aq+(M+1), M+1);
  
  /* find synthesis and excitation */
  mvr2r(&(LPDmem->syn[M]), synth-128, 128);
  LPDmem->Txnq_fac = xn[L_frame-1];

  E_UTIL_f_preemph(xn, TILT_FAC, L_frame, &mem_xnq);
  p_A = A;
  for (i_subfr=0; i_subfr<L_frame; i_subfr+=L_SUBFR) {
    mvr2r(&xn[i_subfr], &synth[i_subfr], L_SUBFR);
    E_UTIL_residu(A+(2*(M+1)), &synth[i_subfr], &xn[i_subfr], L_SUBFR);
  }
  mvr2r(synth+L_frame-(M+128), LPDmem->syn, (M+128));

  /* update excitation for ACELP */
  if (L_frame == lDiv)
  {
    mvr2r(LPDmem->Aexc+lDiv, x, lDiv);
    mvr2r(x, LPDmem->Aexc, lDiv);
    mvr2r(xn, LPDmem->Aexc+lDiv, lDiv);
  }
  else {
    mvr2r(xn+L_frame-(2*lDiv), LPDmem->Aexc, 2*lDiv);
  }

 /*-----------------------------------------------------------*
  * find weighted synthesis                                   *
  *-----------------------------------------------------------*/

  /* find weighted synthesis (use uQ A) */
  mvr2r(&(LPDmem->wsyn[1]), wsyn-128, 128);

  p_A = Ai;
  for (i_subfr=0; i_subfr<L_frame; i_subfr+=L_SUBFR) {
    E_LPC_a_weight(p_A, Ap, GAMMA1, M);
    E_UTIL_residu(Ap, &synth[i_subfr], &wsyn[i_subfr], L_SUBFR);
    p_A += (M+1);
  }
  tmp = wsyn[-1];
  E_UTIL_deemph(wsyn, TILT_FAC, L_frame, &tmp);

  mvr2r(wsyn+L_frame-(1+128), LPDmem->wsyn, (1+128));

  /* mode TCX: 1/2/3 for TCX20/40/80 */
  LPDmem->mode = mode;

  /* number of TCX bits Targeted */
  LPDmem->nbits = 3+7+sqTargetBits;

  return ;
}
