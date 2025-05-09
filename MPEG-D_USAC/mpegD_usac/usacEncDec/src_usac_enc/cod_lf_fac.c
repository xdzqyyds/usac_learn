/*
This material is strictly confidential and shall remain as such.

Copyright  2009 VoiceAge Corporation. All Rights Reserved. No part of this
material may be reproduced, stored in a retrieval system, or transmitted, in any
form or by any means: photocopying, electronic, mechanical, recording, or
otherwise, without the prior written permission of the copyright holder.

This material is subject to continuous developments and improvements. All
warranties implied or expressed, including but not limited to implied warranties
of merchantability, or fitness for purpose, are excluded.

ACELP is registered trademark of VoiceAge Corporation in Canada and / or other
countries. Any unauthorized use is strictly prohibited.
 */

#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "acelp_plus.h"
#include "proto_func.h"

#define HP_ORDER        3

extern const float lsf_init[16];

/*-----------------------------------------------------------------*
 *   Funtion init_coder_wb 
 *   ~~~~~~~~~~~~~ 
 * ->Initialization of variables for the coder section.  
 * - initilize pointers to speech buffer 
 * - initialize static pointers 
 * - set static vectors to zero 
 * - compute lag window and LP analysis window
 *  
 *-----------------------------------------------------------------*/
void init_coder_lf(HANDLE_USAC_TD_ENCODER st, int L_frame, int fscale)
{
  int i;
  int lWindow;

  /* Initialize variables depending on input frame length */
  assert(L_frame == 1024 || L_frame == 768);

  st->lFrame = L_frame;
  st->lDiv   = L_frame/NB_DIV;

  st->nbSubfr = (NB_SUBFR_1024*L_frame)/L_FRAME_1024;

  /* Static vectors to zero */
  set_zero(st->old_speech, L_OLD_SPEECH_HIGH_RATE);
  set_zero(st->old_synth, M);

  set_zero(st->old_exc, PIT_MAX_MAX+L_INTERPOL);
  set_zero(st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
  set_zero(st->mem_lp_decim2, 3);
  set_zero(st->wsig, 128);

 /* Init mem with AAC state (LPDmem.mode = -1)
    no initialization needed for RE8prm[128] */
  st->LPDmem.mode = -1;      
  st->LPDmem.nbits = 0;

  set_zero(st->LPDmem.Aq, 2*(M+1));
  set_zero(st->LPDmem.Ai, 2*(M+1));
  set_zero(st->LPDmem.syn, M+128);
  set_zero(st->LPDmem.wsyn, 1+128);

  /* ACELP memory */
  set_zero(st->LPDmem.Aexc, 2*L_DIV_1024);

  /* TCX memory */
  set_zero(st->LPDmem.Txn, 128);
  set_zero(st->LPDmem.Txnq, 1+256);
  st->LPDmem.Txnq_fac = 0.0f;

  set_zero(st->hp_old_wsp, L_FRAME_1024/OPL_DECIM+(PIT_MAX_MAX/OPL_DECIM));
  set_zero(st->hp_ol_ltp_mem, 3*2+1);
  for (i=0;i<5;i++) st->old_ol_lag[i] = 40;
  st->old_mem_wsyn = 0.0;
  st->old_mem_w0   = 0.0;
  st->old_mem_xnq  = 0.0;
  st->mem_wsp      = 0.0;
  st->old_ovlp_size = 0;
  st->old_T0_med = 40;
  st->ol_wght_flg = 0;
  st->ada_w = 0.0;

  /* isf and isp initialization */
  mvr2r(lsf_init, st->isfold, M);
  for (i=0; i<M; i++)
    st->ispold[i] = (float)cos(3.141592654*(float)(i+1)/(float)(M+1));
  mvr2r(st->ispold, st->ispold_q, M);

  st->prev_mod = -1;
  st->arithReset=1;
  st->codingModeSelection = 0;

  /* Initialize the LP analysis window */
  if(fscale <= FSCALE_DENOM) {
    lWindow = (L_WINDOW_1024*L_frame)/L_FRAME_1024;
  }
  else {
    lWindow = (L_WINDOW_HIGH_RATE_1024*L_frame)/L_FRAME_1024;
  }
  cos_window(st->window, lWindow/2, lWindow/2);

  if(TCX_MDCT_NO_ERROR != TCX_MDCT_Open(&(st->hTcxMdct))){
    fprintf(stderr, "init_coder_lf(): Unable to initialize TCX MDCT\n");
    exit(1);
  }

  return;
}


void config_coder_lf(HANDLE_USAC_TD_ENCODER st, int sampling_rate, int bitrate)
{
  int max_bits, coder_bits;
 

  max_bits = (int) ((float)(bitrate*L_FRAME_1024)/(float)sampling_rate);

  for(st->mode=5; st->mode>=0; st->mode--){
    coder_bits = (st->lFrame == 768) ? NBITS_CORE_768[st->mode] : NBITS_CORE_1024[st->mode];
    if(coder_bits<max_bits)
      return;
  }


  for(st->mode=7; st->mode>=6; st->mode--){
    coder_bits = (st->lFrame == 768) ? NBITS_CORE_768[st->mode] : NBITS_CORE_1024[st->mode];
    if(coder_bits<max_bits)
      return;
  }

  st->mode=-1;

  return;
}

void close_coder_lf(HANDLE_USAC_TD_ENCODER st) {

  TCX_MDCT_Close(&(st->hTcxMdct));
}

void reset_coder_lf(HANDLE_USAC_TD_ENCODER st)
{
  int   i;

  set_zero(st->old_speech, M+L_NEXT_HIGH_RATE_1024);
  set_zero(st->old_speech_pe, M+L_NEXT_HIGH_RATE_1024);
  set_zero(st->old_synth, M);

  /* Static vectors to zero */
  set_zero(st->old_exc, PIT_MAX_MAX+L_INTERPOL);              /* excitation */
  set_zero(st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);             /* weighted speech */
  set_zero(st->mem_lp_decim2, 3);
  set_zero(st->wsig, 128);

  /* Init mem with AAC state (LPDmem.mode = -1)
     no initialization needed for RE8prm[128] */
  st->LPDmem.mode = -1;      
  st->LPDmem.nbits = 0;
  set_zero(st->LPDmem.Aq, 2*(M+1));
  set_zero(st->LPDmem.Ai, 2*(M+1));
  set_zero(st->LPDmem.syn, M+128);
  set_zero(st->LPDmem.wsyn, 1+128);

  /* ACELP memory */
  set_zero(st->LPDmem.Aexc, 2*L_DIV_1024);

  /* TCX memory */
  set_zero(st->LPDmem.Txn, 128);
  set_zero(st->LPDmem.Txnq, 1+256);
  st->LPDmem.Txnq_fac = 0.0f;

  set_zero(st->hp_old_wsp, L_FRAME_1024/OPL_DECIM+(PIT_MAX_MAX/OPL_DECIM));

  set_zero(st->hp_ol_ltp_mem, 3*2+1); /* open loop pitch prediction buffers */
  for (i=0;i<5;i++)
    st->old_ol_lag[i] = 40;
  st->old_mem_wsyn = 0.0;
  st->old_mem_w0   = 0.0;
  st->old_mem_xnq  = 0.0;
  st->mem_wsp      = 0.0;
  st->old_ovlp_size = 0;
  st->old_T0_med = 40;
  st->ol_wght_flg = 0;
  st->ada_w = 0.0;

  /* isf and isp initialization */
  mvr2r(lsf_init, st->isfold, M);
  for (i=0; i<M; i++)
    st->ispold[i] = (float)cos(3.141592654*(float)(i+1)/(float)(M+1));
  mvr2r(st->ispold, st->ispold_q, M);

  /*filter state*/
  st->mem_preemph=0.0;
  set_zero(st->mem_sig_in,4);

  /*For the mdct CELP->TCX80*/
  set_zero(st->xn_buffer, 128);

  return;
}


/*-----------------------------------------------------------------*
 *   Funtion  coder_wb                                             *
 *            ~~~~~~~~                                             *
 *   ->Main coder routine.                                         *
 *                                                                 *
 *-----------------------------------------------------------------*/

void coder_lf_amrwb_plus(
    short *codec_mode,        /* (i) : AMR-WB+ mode (see cnst.h)             */
    float speech[],         /* (i) : speech vector [-M..L_FRAME_PLUS+L_NEXT]   */
    float synth_enc[],      /* (o) : synthesis vector [-M..L_FRAME_PLUS]       */
    int mod[],              /* (o) : mode for each 20ms frame (mode[4]     */
    int n_param_tcx[],      /* (o) : numer of tcx parameters per 20ms subframe */
    float window[],         /* (i) : window for LPC analysis               */
    int param_lpc[],        /* (o) : parameters for LPC filters            */
    int param[],            /* (o) : parameters (NB_DIV*NPRM_DIV)          */
    short serialFac[],
    int *nBitsFac,
    float ol_gain[],        /* (o) : open-loop LTP gain                    */
    int pit_adj,            /* (i) : indicate pitch adjustment             */
    HANDLE_USAC_TD_ENCODER st,   /* i/o : coder memory state                    */
    int isAceStart
)
{ 
  /* LPC coefficients */
  float r[M+1];                   /* Autocorrelations of windowed speech  */
  float A[(NB_SUBFR_SUPERFR_1024+1)*(M+1)];
  float Aq[(NB_SUBFR_SUPERFR_1024+1)*(M+1)];
  float ispnew[M];                /* LSPs at 4nd subframe                 */
  float isp[(NB_DIV+1)*M];
  float isp_q[(NB_DIV+1)*M];
  float isf[(NB_DIV+1)*M];
  float isf_q[(NB_DIV+1)*M];
  int nb_indices, nb_bits;
  float Ap[M+1];
  int prm_tcx[NPRM_TCX80];
  float *synth_tcx, synth_tcx_buf[128+L_FRAME_1024];
  float *synth, synth_buf[128+L_FRAME_1024];
  float *wsig, wsig_buf[128+L_FRAME_1024];
  float *wsyn, wsyn_buf[128+L_FRAME_1024];
  float *wsyn_tcx, wsyn_tcx_buf[128+L_FRAME_1024];

  float old_d_wsp[(PIT_MAX_MAX/OPL_DECIM)+L_DIV_1024];     /* Weighted speech vector */
  float *d_wsp; 
  int *prm_lpc;

  /* Scalars */
  int i, j, k, *prm;
  float ener, cor_max, t0;
  float *p, *p1;
  int Top[2*NB_DIV];
  float Tnc[2*NB_DIV];

  int n_param;
  int PIT_MIN;				/* Minimum pitch lag with resolution 1/4      */
  int PIT_MAX;				/* Maximum pitch lag                          */
  LPD_state LPDmem[5], LPDmem_tmp;

  int nbits_acelp, nbits_tcx20;

  int lfacNext, lfacPrev;
  int l_pit_search;

  /* Variables dependent on frame length */
  int lDiv;
  int nbSubfr;
  int nbSubfrSuperfr;
  int lWindow;

  /* Set variables dependent on frame length */
  lDiv    = st->lDiv;
  nbSubfr = st->nbSubfr;
  nbSubfrSuperfr = NB_DIV*nbSubfr;
  if (pit_adj<=FSCALE_DENOM) 
  {
    lWindow = (L_WINDOW_1024*lDiv)/L_DIV_1024;
  }
  else
  {
    lWindow = (L_WINDOW_HIGH_RATE_1024*lDiv)/L_DIV_1024;
  }

  /* set pointers and memory */
  synth_tcx = synth_tcx_buf+128;
  synth = synth_buf+128;
  wsig = wsig_buf+128;
  wsyn = wsyn_buf+128;
  wsyn_tcx = wsyn_tcx_buf+128;

  mvr2r(st->wsig, wsig_buf, 128);

  if ((st->lDiv)==((3*L_DIV_1024)/4))
  {
    nbits_acelp = (NBITS_CORE_768[*codec_mode] - NBITS_MODE)>>2;
  }
  else
  {
    nbits_acelp = (NBITS_CORE_1024[*codec_mode] - NBITS_MODE)>>2;
  }
  nbits_tcx20 = (int)(0.85f*nbits_acelp-NBITS_LPC); /* Safety margin for possible FAC windows */

  if(pit_adj ==0) {
    PIT_MIN = PIT_MIN_12k8;
    PIT_MAX = PIT_MAX_12k8;
  }
  else {
    i = (((pit_adj*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8;
    PIT_MIN = PIT_MIN_12k8 + i;
    PIT_MAX = PIT_MAX_12k8 + (6*i);
  }

  /* Initialize pointers */
  d_wsp = old_d_wsp + PIT_MAX_MAX/OPL_DECIM;
  /* copy coder memory state into working space (dynamic memory) */
  mvr2r(st->old_d_wsp, old_d_wsp, PIT_MAX_MAX/OPL_DECIM);

  prm_lpc=param_lpc;

  /*---------------------------------------------------------------*
   *  Perform LPC0 analysis and corresponding updates              *
   *---------------------------------------------------------------*/
  if (isAceStart) 
  {
    for (i=0; i<M; i++) {
      st->ispold[i] = (float)cos(3.141592654*(float)(i+1)/(float)(M+1));
    }

    /**** LPC0 analysis ****/
    E_UTIL_autocorrPlus(&speech[-(lWindow/2)], r, M, lWindow, window);
    lag_wind(r, M);                            /* Lag windowing    */
    E_LPC_lev_dur(Ap, r, M);                         /* Levinson Durbin  */
    E_LPC_a_lsp_conversion(Ap, ispnew, st->ispold);          /* From A(z) to ISP */
    mvr2r(ispnew, st->ispold, M);

    /* Convert isps to frequency domain 0..6400 */
    E_LPC_lsp_lsf_conversion(ispnew, isf, M);
    mvr2r(isf, st->isfold, M);
  }

  /*---------------------------------------------------------------*
   *  Perform LP analysis four times (every 20 ms)                 *
   *  - autocorrelation + lag windowing                            *
   *  - Levinson-Durbin algorithm to find a[]                      *
   *  - convert a[] to isp[]                                       *
   *  - interpol isp[]                                             *
   *---------------------------------------------------------------*/

  /* read old isp for LPC interpolation */
  mvr2r(st->ispold, isp, M);

  for (i = 0; i < NB_DIV; i++)
  {
    E_UTIL_autocorrPlus(&speech[((i+1)*lDiv)-(lWindow/2)], r, M, lWindow, window);
    lag_wind(r, M);                            /* Lag windowing    */
    E_LPC_lev_dur(Ap, r, M);                   /* Levinson Durbin  */
    E_LPC_a_lsp_conversion(Ap, ispnew, st->ispold);          /* From A(z) to ISP */
    mvr2r(ispnew, &isp[(i+1)*M], M);

    /* A(z) interpolation every 20 ms (used for weighted speech) */
    int_lpc_acelp(st->ispold, ispnew, &A[i*nbSubfr*(M+1)], nbSubfr, M);

    /* Convert isps to frequency domain 0..6400 */
    E_LPC_lsp_lsf_conversion(&isp[(i+1)*M], &isf[(i+1)*M], M);

    /* update ispold[] for the next LPC analysis */
    mvr2r(ispnew, st->ispold, M);
  } /* end of of LP analysis */

  /* load isfold[] & update for the next frame */
  mvr2r(st->isfold, isf, M);
  mvr2r(&isf[NB_DIV*M], st->isfold, M);


  /*---------------------------------------------------------------*
   *  Perform variable-bit-rate LP quantization                    *
   *---------------------------------------------------------------*/

  if (!isAceStart)
  {
    E_LPC_lsp_lsf_conversion(st->ispold_q, isf_q, M);        /* for LPC1 mid0 */
  }

  qlpc_avq(&isf[M], &isf_q[M], isAceStart, &param_lpc[0], &nb_indices, &nb_bits);

  for (i=0; i<NB_DIV; i++)
  {
    E_LPC_lsf_lsp_conversion(&isf_q[(i+1)*M], &isp_q[(i+1)*M], M);
  }

  if (isAceStart)
  {
    /* Convert isfs to the cosine domain */
    E_LPC_lsf_lsp_conversion(isf_q, isp_q, M);
    mvr2r(isp_q, st->ispold_q, M);
  }

  *nBitsFac = 0;
  if ( isAceStart ) {
    /* reset ACB memory using past AAC synthesis and LPC0 here */
    Float32 tmp[2*L_DIV_1024+M];
    Float32 tmpRes[2*L_DIV_1024];
    Float32 ANull[9*(M+1)];
    float memHP[4];
    Float32 mem = 0.0f;
    int lfac;
    int nbitsFac = (int)((float)nbits_tcx20 / 2.f); /* spend 10% of the TXC rate to fac */
    float tmp_buf[M+L_DIV_1024];

    if (st->lastWasShort)
    {
       lfac = (st->lFrame)/16; /*LFAC_SHORT*/
    }
    else
    {
       lfac = lDiv/2;
    }

    int_lpc_np1(st->ispold_q, st->ispold_q, ANull, (2*lDiv)/L_SUBFR, M);

    set_zero(tmp,M+2*lDiv);
    set_zero(tmpRes,2*lDiv);

    /* FD->ACELP FAC encoding */
    coder_fdfac(&st->fdOrig[1+M], lDiv, lfac, st->lowpassLine,nbitsFac,  &st->fdSynth[1+M], ANull, serialFac, nBitsFac);
    set_zero(memHP,4);
    hp50_12k8(st->fdOrig,2*lDiv+1+M,memHP,pit_adj);
    mem = 0.0f;
    E_UTIL_f_preemph(st->fdOrig,PREEMPH_FAC,2*lDiv+1+M,&mem);

    mvr2r(st->fdOrig+lDiv+1, tmp_buf, lDiv+M);
    mem = tmp_buf[0];
    E_UTIL_deemph(tmp_buf, PREEMPH_FAC, lDiv+M, &mem);
    mvr2r(&tmp_buf[lDiv+M-128], st->LPDmem.Txn, 128);

    /* Compute past preemphasized synthesis signal */
    mem = 0.0f;
    E_UTIL_f_preemph(st->fdSynth,PREEMPH_FAC,2*lDiv+1+M,&mem);
    mvr2r(st->fdSynth + 2*lDiv-M-128+1+M,st->LPDmem.syn,M+128);
    mvr2r(st->fdSynth+1+M,tmp+M,2*lDiv);

    /* Compute past weigthed synthesis signal */
    mem = 0.0f;
    find_wsp(ANull,tmp+M,tmpRes,&mem,2*lDiv);
    st->old_mem_wsyn = mem;
    mvr2r(tmpRes+2*lDiv-M-128,st->LPDmem.wsyn,M+128);
    mvr2r(st->fdSynth+1+M,tmp+M,2*lDiv);
    set_zero(tmpRes, 2*lDiv);
    for ( i = 0 ; i < 2*lDiv ; i+= L_SUBFR ) {
      E_UTIL_residu(ANull,&tmp[M+i],&tmpRes[i],L_SUBFR);
    }
    mvr2r(tmpRes,st->LPDmem.Aexc,2*lDiv);

    /* excitation */

    /* weighted speech */
    mem = 0.0f;

    find_wsp(A,st->fdOrig+1+M,tmp+M,&(st->mem_wsp),2*lDiv);
    mvr2r(tmp+M+2*lDiv-128,st->wsig,128);
    mvr2r(st->wsig,wsig_buf,128);
    for ( i = 0 ; i < 2*lDiv ; i+= lDiv ) {
      E_GAIN_lp_decim2(&tmp[i+M],lDiv,st->mem_lp_decim2);
      mvr2r(tmp+M+i,tmp+M+i/OPL_DECIM,lDiv/OPL_DECIM);
    }
    mvr2r(tmp+M+2*lDiv/OPL_DECIM - PIT_MAX_MAX/OPL_DECIM, old_d_wsp,PIT_MAX_MAX/OPL_DECIM);

    /* hp_old_wsp */
    {
      int L_max = PIT_MAX/OPL_DECIM;
      int nFrame = (2*L_SUBFR)/OPL_DECIM;
      Float32 *hp_wsp_mem = st->hp_ol_ltp_mem;
      Float32 *hp_old_wsp = st->hp_old_wsp;
      for ( i = 0 ; i < 2*lDiv/OPL_DECIM; i+= nFrame) {
        Float32 *data_a, *data_b, *hp_wsp, o;
        Float32 *wsp = tmp+M+i;
        data_a = hp_wsp_mem;
        data_b = hp_wsp_mem + HP_ORDER;
        hp_wsp = hp_old_wsp + L_max;
        for (k = 0; k < nFrame; k++)
        {
          data_b[0] = data_b[1];
          data_b[1] = data_b[2];
          data_b[2] = data_b[3];
          data_b[HP_ORDER] = wsp[k];
          o = data_b[0] * 0.83787057505665F;
          o += data_b[1] * -2.50975570071058F;
          o += data_b[2] * 2.50975570071058F;
          o += data_b[3] * -0.83787057505665F;
          o -= data_a[0] * -2.64436711600664F;
          o -= data_a[1] * 2.35087386625360F;
          o -= data_a[2] * -0.70001156927424F;
          data_a[2] = data_a[1];
          data_a[1] = data_a[0];
          data_a[0] = o;
          hp_wsp[k] = o;
        }
        mvr2r(&hp_old_wsp[nFrame], hp_old_wsp, L_max);
      }
    }

  }

  /* load ispold_q[] & update for the next frame */
  mvr2r(st->ispold_q, isp_q, M);
  mvr2r(&isp_q[NB_DIV*M], st->ispold_q, M);


  /*---------------------------------------------------------------*
   * Calculate open-loop LTP parameters                            *
   *---------------------------------------------------------------*/

  for (i = 0; i < NB_DIV; i++) {
    /* weighted speech for SNR */
    find_wsp(&A[i*(nbSubfrSuperfr/4)*(M+1)], &speech[i*lDiv], &wsig[i*lDiv], &(st->mem_wsp), lDiv);
    mvr2r(&wsig[i*lDiv], d_wsp, lDiv);

    E_GAIN_lp_decim2(d_wsp, lDiv, st->mem_lp_decim2);
    /* Find open loop pitch lag for first 1/2 frame */
    l_pit_search = 2*L_SUBFR;
    if (nbSubfr<4) l_pit_search = 3*L_SUBFR;
    Top[i*2] = E_GAIN_open_loop_search(d_wsp, (PIT_MIN/OPL_DECIM)+1, PIT_MAX/OPL_DECIM,
                                       l_pit_search/OPL_DECIM, st->old_T0_med, &(st->ol_gain),
                                       st->hp_ol_ltp_mem, st->hp_old_wsp, (unsigned char)st->ol_wght_flg);
    if (st->ol_gain > 0.6) {
      st->old_T0_med = E_GAIN_olag_median(Top[i*2], st->old_ol_lag);
      st->ada_w = 1.0;
    }     
    else {
      st->ada_w = st->ada_w * 0.9f;
    }
    if (st->ada_w < 0.8) {
      st->ol_wght_flg = 0;
    }
    else {
      st->ol_wght_flg = 1;
    }
    /* compute max */
    cor_max = 0.0f;
    p = &d_wsp[0];
    p1 = d_wsp - Top[i*2];
    for(j=0; j<l_pit_search/OPL_DECIM; j++) {
      cor_max += *p++ * *p1++;
    }
    /* compute energy */
    t0 = 0.01f;
    p = d_wsp - Top[i*2];
    for(j=0; j<l_pit_search/OPL_DECIM; j++, p++) {
      t0 += *p * *p;
    }
    t0 = (float)(1.0 / sqrt(t0));		/* 1/sqrt(energy)    */
    Tnc[i*2] = cor_max * t0;        /* max/sqrt(energy)  */

    /* normalized corr (0..1) */
    ener = 0.01f;
    for (j=0; j<l_pit_search/OPL_DECIM; j++) {
      ener += d_wsp[j]*d_wsp[j];
    }
    ener = (float)(1.0 / sqrt(ener));		/* 1/sqrt(energy)    */
    Tnc[i*2] *= ener;

    if (nbSubfr<4)
    {
       Top[(i*2)+1] = Top[i*2];
       Tnc[(i*2)+1] = Tnc[i*2];
    }
    else
    {
    /* Find open loop pitch lag for second 1/2 frame */
    Top[(i*2)+1] = E_GAIN_open_loop_search(d_wsp + ((2*L_SUBFR)/OPL_DECIM), (PIT_MIN/OPL_DECIM)+1, PIT_MAX/OPL_DECIM,
                                           (2*L_SUBFR)/OPL_DECIM, st->old_T0_med, &st->ol_gain,
                                           st->hp_ol_ltp_mem,st->hp_old_wsp, (unsigned char)st->ol_wght_flg);
    if (st->ol_gain > 0.6) {
      st->old_T0_med = E_GAIN_olag_median(Top[(i*2)+1], st->old_ol_lag);
      st->ada_w = 1.0;
    }     
    else {
      st->ada_w = st->ada_w * 0.9f;
    }
    if( st->ada_w < 0.8) {
      st->ol_wght_flg = 0;
    }
    else {
      st->ol_wght_flg = 1;
    }
    /* compute max */
    cor_max = 0.0f;
    p = d_wsp + (2*L_SUBFR)/OPL_DECIM;
    p1 = d_wsp + ((2*L_SUBFR)/OPL_DECIM) - Top[(i*2)+1];
    for(j=0; j<(2*L_SUBFR)/OPL_DECIM; j++) {
      cor_max += *p++ * *p1++;
    }
    /* compute energy */
    t0 = 0.01f;
    p = d_wsp + ((2*L_SUBFR)/OPL_DECIM) - Top[(i*2)+1];
    for(j=0; j<(2*L_SUBFR)/OPL_DECIM; j++, p++) {
      t0 += *p * *p;
    }
    t0 = (float)(1.0 / sqrt(t0));		/* 1/sqrt(energy)    */
    Tnc[(i*2)+1] = cor_max * t0;        /* max/sqrt(energy)  */

    /* normalized corr (0..1) */
    ener = 0.01f;
    for (j=0; j<(2*L_SUBFR)/OPL_DECIM; j++) {
      ener += d_wsp[((2*L_SUBFR)/OPL_DECIM)+j]*d_wsp[((2*L_SUBFR)/OPL_DECIM)+j];
    }
    ener = (float)(1.0 / sqrt(ener));		/* 1/sqrt(energy)    */
    Tnc[(i*2)+1] *= ener;
    }

    ol_gain[i] = st->ol_gain;
    mvr2r(&old_d_wsp[lDiv/OPL_DECIM], old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
  }

  /*---------------------------------------------------------------*
   *  Call ACELP and TCX codec                                     *
   *---------------------------------------------------------------*/

  /* load LPD state */
  LPDmem[0] = st->LPDmem;

  for(k=0;k<4;)
  {
    /* set pointer to parameters */
    prm = param + (k*NPRM_DIV);

    switch(st->codingModeSelection%4){
    case 0:
      /* interpol quantized lpc */
      int_lpc_acelp(&isp_q[k*M], &isp_q[(k+1)*M], Aq, nbSubfr, M);

      /*---------------------------------------------------------------*
       *  Call ACELP nbSubfr x 5ms = 20 ms frame                       *
       *---------------------------------------------------------------*/

      /* copy LPD state */
      LPDmem[k+1] = LPDmem[k];

      coder_acelp_fac(
                      &A[k*(nbSubfrSuperfr/4)*(M+1)],
                      Aq,
                      &speech[k*lDiv],
                      &wsig[k*lDiv],
                      &synth[k*lDiv],
                      &wsyn[k*lDiv],
                      *codec_mode,
                      &LPDmem[k+1],
                      lDiv,
                      Tnc[k*2],
                      Tnc[(k*2)+1],
                      Top[k*2],
                      Top[(k*2)+1],
                      pit_adj,
                      prm);

      mod[k] = 0;
      n_param_tcx[k] = 0;
      k+=1;
      break;

    case 1:
      /*--------------------------------------------------*
       * Call short TCX coder                             *
       *--------------------------------------------------*/
        
      int_lpc_tcx(&isp_q[k*M], &isp_q[(k+1)*M], Aq, nbSubfr, M);
        
      /* copy LPD state */
      LPDmem_tmp = LPDmem[k];
      if ( k == 0 && st->lastWasShort ) {
        lfacPrev = (st->lFrame)/16;
      }
      else {
        lfacPrev = lDiv/2;
      }
      if ( k == 3 && st->nextIsShort ) {
        lfacNext = (st->lFrame)/16;
      }
      else {
        lfacNext = lDiv/2;
      }
        
      coder_tcx_fac(
                    &A[k*(nbSubfrSuperfr/4)*(M+1)],
                    Aq,
                    &speech[k*lDiv],
                    &wsig[k*lDiv],
                    synth_tcx,
                    wsyn_tcx,
                    lDiv,
                    lDiv,
                    nbits_tcx20,
                    &LPDmem_tmp,
                    prm_tcx,
                    &n_param,
                    lfacPrev,
                    lfacNext,
                    st->hTcxMdct
                    );
        
      /*--------------------------------------------------------*
       * Save tcx parameters                                    *
       *--------------------------------------------------------*/
        
      mod[k] = 1;
      n_param_tcx[k] = n_param;
        
      LPDmem[k+1] = LPDmem_tmp;    /* update LPD state */
        
      mvr2r(synth_tcx-128, &synth[(k*lDiv)-128], lDiv+128);
      mvr2r(wsyn_tcx-128, &wsyn[(k*lDiv)-128], lDiv+128);
      mvi2i(prm_tcx, prm, NPRM_TCX20);
      k+=1;
      break;
        
    case 2:
      /* interpol quantized lpc */
      int_lpc_tcx(&isp_q[k*M], &isp_q[(k+2)*M], Aq, (nbSubfrSuperfr/2), M);
        
      /*--------------------------------------------------*
       * Call medium TCX coder                            *
       *--------------------------------------------------*/
        
      /* copy LPD state */
      LPDmem_tmp = LPDmem[k];
      if ( k == 0 && st->lastWasShort ) {
        lfacPrev = (st->lFrame)/16;
      }
      else {
        lfacPrev = lDiv/2;
      }
      if ( k == 2 && st->nextIsShort ) {
        lfacNext = (st->lFrame)/16;
      }
      else {
        lfacNext = lDiv/2;
      }
        
      coder_tcx_fac(
                    &A[k*(nbSubfrSuperfr/4)*(M+1)],
                    Aq,
                    &speech[k*lDiv],
                    &wsig[k*lDiv],
                    synth_tcx,
                    wsyn_tcx,
                    2*lDiv,
                    lDiv,
                    2*nbits_tcx20,
                    &LPDmem_tmp,
                    prm_tcx,
                    &n_param,
                    lfacPrev,
                    lfacNext,
                    st->hTcxMdct
                    );
        
      /*--------------------------------------------------------*
       * Save tcx parameters                                    *
       *--------------------------------------------------------*/
        
      for (i=0; i<2; i++) {
        mod[k+i] = 2;
        n_param_tcx[k+i] = n_param;
      }
        
      LPDmem[k+2] = LPDmem_tmp;    /* update LPD state */
        
      mvr2r(synth_tcx-128, &synth[(k*lDiv)-128], (2*lDiv)+128);
      mvr2r(wsyn_tcx-128, &wsyn[(k*lDiv)-128], (2*lDiv)+128);
      mvi2i(prm_tcx, prm, NPRM_TCX40);
      k+=2;
      break;
        
    case 3:
      /* interpol quantized lpc */
      int_lpc_tcx(&isp_q[k*M], &isp_q[(k+4)*M], Aq, nbSubfrSuperfr, M);
        
      /*--------------------------------------------------*
       * Call long TCX coder                              *
       *--------------------------------------------------*/
        
      /* copy LPD state */
      LPDmem_tmp = LPDmem[k];
      if ( st->lastWasShort ) {
        lfacPrev = (st->lFrame)/16;
      }
      else {
        lfacPrev = lDiv/2;
      }
      if ( st->nextIsShort ) {
        lfacNext = (st->lFrame)/16;
      }
      else {
        lfacNext = lDiv/2;
      }
        
      coder_tcx_fac(
                    &A[k*(nbSubfrSuperfr/4)*(M+1)],
                    Aq,
                    &speech[k*lDiv],
                    &wsig[k*lDiv],
                    synth_tcx,
                    wsyn_tcx,
                    4*lDiv,
                    lDiv,
                    4*nbits_tcx20,
                    &LPDmem_tmp,
                    prm_tcx,
                    &n_param,
                    lfacPrev,
                    lfacNext,
                    st->hTcxMdct
                    );
        
      /*--------------------------------------------------------*
       * Save tcx parameters                                    *
       *--------------------------------------------------------*/
        
      for (i=0; i<4; i++) {
        mod[k+i] = 3;
        n_param_tcx[k+i] = n_param;
      }
        
      LPDmem[k+4] = LPDmem_tmp;    /* update LPD state */
        
      mvr2r(synth_tcx-128, &synth[(k*lDiv)-128], (4*lDiv)+128);
      mvr2r(wsyn_tcx-128, &wsyn[(k*lDiv)-128], (4*lDiv)+128);
      mvi2i(prm_tcx,     prm,     NPRM_TCX80);
      k+=4;
      break;
        
    default:
      fprintf(stderr, "cod_lf_amrwb_plus(): not existing coding mode\n");
      exit(1);
    }
  }
  
  st->codingModeSelection++;
  
  /* update LPD state */
  st->LPDmem = LPDmem[4];

  /* update memories */
  mvr2r(wsig_buf+(st->lFrame), st->wsig, 128);
  mvr2r(old_d_wsp, st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);   /* d_wsp already shifted */

  mvr2r(synth-M, synth_enc-M, M+(4*lDiv));

  return;
}


int* getRe8Prm(HANDLE_USAC_TD_ENCODER st){

  return st->LPDmem.RE8prm;
}

/*--------------------------------------------------*
 * Find weighted speech (formula from AMR-WB)       *
 *--------------------------------------------------*/
void find_wsp(float A[],
              float speech[],        /* speech[-M..lg]   */
              float wsp[],           /* wsp[0..lg]       */
              float *mem_wsp,        /* memory           */
              int lg)
{
  int i_subfr;
  float *p_A, Ap[M+1];
  p_A = A;
  for (i_subfr=0; i_subfr<lg; i_subfr+=L_SUBFR) {
    E_LPC_a_weight(p_A, Ap, GAMMA1, M);
    E_UTIL_residu(Ap, &speech[i_subfr], &wsp[i_subfr], L_SUBFR);
    p_A += (M+1);
  }
  E_UTIL_deemph(wsp, TILT_FAC, lg, mem_wsp);
  return;
}
