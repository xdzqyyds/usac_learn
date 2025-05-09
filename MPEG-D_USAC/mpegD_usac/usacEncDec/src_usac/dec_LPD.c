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

#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "proto_func.h"
#include "acelp_plus.h"
#include "buffers.h"

#define L_EXTRA  96

#define BUGFIX_BPF

extern const float lsf_init[16];

/* local functions */
static void bass_pf_1sf_delay(
  float *syn,       /* (i) : 12.8kHz synthesis to postfilter             */
  int *T_sf,        /* (i) : Pitch period for all subframes (T_sf[16])   */
  float *gainT_sf,  /* (i) : Pitch gain for all subframes (gainT_sf[16]) */
  float *synth_out, /* (o) : filtered synthesis (with delay of 1 subfr)  */
  int l_frame,      /* (i) : frame length (should be multiple of l_subfr)*/
  int l_subfr,      /* (i) : sub-frame length (60/64)                    */
  int l_next,       /* (i) : look ahead for symetric filtering           */
  float mem_bpf[]); /* i/o : memory state [L_FILT+L_SUBFR]               */


void init_decoder_lf(HANDLE_USAC_TD_DECODER st)
{
    TCX_MDCT_Open(&st->hTcxMdct);
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    st->fdSynth=&(st->fdSynth_Buf[L_DIV_1024]);
#endif
    reset_decoder_lf(st, NULL, 0, 0);
}

void reset_decoder_lf(HANDLE_USAC_TD_DECODER st, float* pOlaBuffer, int lastWasShort, int twMdct)
{
  int   i;
#ifndef USAC_REFSOFT_COR1_NOFIX_06
  int lfac;
#endif

  /* Static vectors to zero */
  if (lastWasShort == 1 ) {
    st->last_mode = -2;
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    lfac = (st->lFrame)/16;
#endif
  }
  else {
    st->last_mode = -1;      /* -1 indicate AAC frame (0=ACELP, 1,2,3=TCX) */
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    lfac = (st->lDiv)/2;
#endif
  }
  set_zero(st->old_Aq, 2*(M+1));
  set_zero(st->old_xnq, 1+(2*LFAC_1024));
  set_zero(st->old_exc, PIT_MAX_MAX+L_INTERPOL);   /* excitation */
  set_zero(st->old_synth, PIT_MAX_MAX+SYN_DELAY_1024);

  /* bass pf reset */
  set_zero(st->old_noise_pf, L_FILT+L_SUBFR);
  for (i=0; i<SYN_SFD_1024; i++)
  {
    st->old_T_pf[i] = 64;
    st->old_gain_pf[i] = 0.0f;
  }

  set_zero(st->past_lsfq, M);               /* past isf quantizer */
  
  /* Initialize the ISFs */
  mvr2r(lsf_init, st->lsfold, M);

  for (i = 0; i < L_MEANBUF; i++) 
  {
    mvr2r(lsf_init, &(st->lsf_buf[i*M]), M);
  }
  /* Initialize the ISPs */
  for (i=0; i<M; i++) 
  {
    st->lspold[i] = (float)cos(3.141592654*(float)(i+1)/(float)(M+1));
  }

  st->old_T0 = 64;
  st->old_T0_frac = 0;
  st->seed_ace = 0;
#ifndef UNIFIED_RANDOMSIGN
  st->seed_tcx = 21845;
#endif

  st->past_gpit = 0.0;
  st->past_gcode = 0.0;
  st->gc_threshold = 0.0f;
  st->wsyn_rms = 0.0f;
#ifndef USAC_REFSOFT_COR1_NOFIX_09
  st->prev_BassPostFilter_activ=0;
#endif

  if ( pOlaBuffer != NULL && !twMdct){
    const float *pWinCoeff;
#ifdef USAC_REFSOFT_COR1_NOFIX_06
    int lfac;
    if (lastWasShort ) {
      lfac = (st->lFrame)/16;
    }
    else {
      lfac = (st->lDiv)/2;
    }

#endif
    if  (lfac==48) {
      pWinCoeff = sineWindow96;
    }
    else if  (lfac==64) {
      pWinCoeff = sineWindow128;
    }
    else if (lfac==96) {
      pWinCoeff = sineWindow192;
    }
    else {
      pWinCoeff = sineWindow256;
    }

    for (i = 0; i < 2*lfac; i++){
      pOlaBuffer[(st->lFrame)/2-lfac+i] *= pWinCoeff[2*lfac-1-i];
    }
    for ( i = 0 ; i < (st->lFrame)/2-lfac; i++ ) {
      pOlaBuffer[(st->lFrame)/2+lfac+i] = 0.0f;
    }
  }

  st->pOlaBuffer = pOlaBuffer;

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  /*set old_xnq for calculation of the excitation buffer*/
  if ( pOlaBuffer != NULL) {
    for(i=0; i<(st->lDiv)/2-lfac; i++) {
      st->old_xnq[i]=0;
    }
    for(i=0; i<2*lfac+1; i++) {
      st->old_xnq[(st->lDiv)/2-lfac+i]=pOlaBuffer[i+st->lFrame/2-lfac-1];
    }
  } else {
    set_zero(st->old_xnq, 1+(2*LFAC_1024));
  }
#endif

  return;
}

void close_decoder_lf(HANDLE_USAC_TD_DECODER st){
  if(st){
    if(st->hTcxMdct){
      TCX_MDCT_Close(&st->hTcxMdct);
    }
    st->hTcxMdct = NULL;
  }
}


void decoder_LPD(
  HANDLE_USAC_TD_DECODER    st,
  td_frame_data            *td,
  int                  pit_adj,
  float               fsynth[],
  int              bad_frame[],
  int               isAceStart,
  int           short_fac_flag,
  int         isBassPostFilter)
  {
  float synth_buf[PIT_MAX_MAX+SYN_DELAY_1024+L_FRAME_1024];
  float exc_buf[PIT_MAX_MAX+L_INTERPOL+L_FRAME_1024+1]; 
  float lsf[(NB_DIV+1)*M];
  float lspnew[M];     
  float lsfnew[M];     
  float Aq[(NB_SUBFR_SUPERFR_1024+1)*(M+1)]; /* A(z) for all subframes     */
  float *synth, *exc;
  int   pitch[NB_SUBFR_SUPERFR_1024+SYN_SFD_1024];
  float pit_gain[NB_SUBFR_SUPERFR_1024+SYN_SFD_1024];

  /* Scalars */
  int i, k, T, mode;
  int *mod;
  float tmp, gain, stab_fac;

  /* Variables dependent on the frame length */
  int lFrame;
  int lDiv;
  int nbSubfr;
  int nbSubfrSuperfr;
  int SynSfd;
  int SynDelay;

  /* Set variables dependent on frame length */
  lFrame   = st->lFrame;
  lDiv     = st->lDiv;
  nbSubfr  = st->nbSubfr;
  nbSubfrSuperfr = NB_DIV*nbSubfr;
  SynSfd   = (nbSubfrSuperfr/2)-BPF_SFD;
  SynDelay = SynSfd*L_SUBFR;

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  for ( i=0; i<PIT_MAX_MAX+SYN_DELAY_1024+L_FRAME_1024; i++) {
    synth_buf[i]=0.0f;
  }

  for(i = 0; i < sizeof(exc_buf) / sizeof(float); i++){
    exc_buf[i]=0.0f;
  }
#endif

  /* Initialize pointers */
  synth = synth_buf+PIT_MAX_MAX+SynDelay;
  mvr2r(st->old_synth, synth_buf, PIT_MAX_MAX+SynDelay);
  exc = exc_buf+PIT_MAX_MAX+L_INTERPOL;

  mvr2r(st->old_exc, exc_buf, PIT_MAX_MAX+L_INTERPOL);

  mod       = td->mod;

  /* for bass postfilter */
  for (i=0; i<SynSfd; i++) 
  {
    pitch[i] = st->old_T_pf[i];
    pit_gain[i] = st->old_gain_pf[i];
  }
  for (i=0; i<nbSubfrSuperfr; i++) 
  {
    pitch[i+SynSfd] = L_SUBFR;
    pit_gain[i+SynSfd] = 0.0f;
  }
  /* Decode LPC parameters */

  if (!isAceStart)
  {
     E_LPC_lsp_lsf_conversion(st->lspold, lsf, M);
  }

  dlpc_avq(td, isAceStart, &lsf[M], st->past_lsfq, td->mod, bad_frame[0]);

  if (isAceStart)
  {
     mvr2r(&lsf[0], st->lsfold, M);
     E_LPC_lsf_lsp_conversion(st->lsfold, st->lspold, M);
  }

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  if ( (isAceStart && mod[0] == 0) || (isAceStart && mod[1] == 0) || ((isAceStart && mod[2] == 0 && lDiv != L_DIV_1024))) {
#else
  if ( isAceStart && mod[0] == 0 ) {
#endif

    /* read and decode FD-FAC data */
    int facBits = 0;
    float mem=0.0f;
    float ANull[9*(M+1)];

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    float tmp_buf[3*L_DIV_1024+M];
    float tmpRes_buf[3*L_DIV_1024];
    float *tmp=&(tmp_buf[L_DIV_1024]);
    float *tmpRes=&(tmpRes_buf[L_DIV_1024]);
    int tmp_start;
#else
    float tmp[2*L_DIV_1024+M];
    float tmpRes[2*L_DIV_1024];
#endif

    int   nSamp = 0;
    float facTimeSig[2*L_DIV_1024];

    int_lpc_acelp(st->lspold, st->lspold, ANull, 8, M);

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    /*fill old_Aq for later generating an excitation signal on an overlapped signal between FD and TCX*/
    memcpy(st->old_Aq,ANull,(M+1)*sizeof(float));
    memcpy(st->old_Aq+M+1,ANull,(M+1)*sizeof(float));
#endif

    if (mod[0] == 0) {
      int lfac;
      if ( short_fac_flag ) {
        lfac = (lDiv*NB_DIV)/16;
      }
      else {
        lfac = lDiv/2;
      }
      decode_fdfac(td->fdFac, lDiv, lfac, ANull, NULL, facTimeSig);
      addr2r(facTimeSig,st->pOlaBuffer+(lFrame/2)-lfac,st->pOlaBuffer+(lFrame/2)-lfac,lfac);
      set_zero(st->pOlaBuffer+(lFrame/2),lfac);
    }

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    if (mod[0] == 0 && lDiv!=L_DIV_1024) {
      mvr2r(st->pOlaBuffer-lDiv, st->fdSynth-lDiv, 3*lDiv);
      nSamp = min(3*lDiv,PIT_MAX_MAX+SynDelay);
    } else {
      mvr2r(st->pOlaBuffer,st->fdSynth,2*lDiv);
      nSamp = min(2*lDiv,PIT_MAX_MAX+SynDelay);
    }
#else
    mvr2r(st->pOlaBuffer,st->fdSynth,2*lDiv);
    nSamp = min(2*lDiv,PIT_MAX_MAX+SynDelay);
#endif

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    mvr2r(st->fdSynth,synth-2*lDiv,2*lDiv);
#else
    mvr2r(st->fdSynth+2*lDiv-nSamp,synth-nSamp,nSamp);
#endif

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    if (mod[0] == 0 && lDiv!=L_DIV_1024) {
      E_UTIL_f_preemph(st->fdSynth-lDiv,PREEMPH_FAC,3*lDiv,&mem);
    } else {
      E_UTIL_f_preemph(st->fdSynth,PREEMPH_FAC,2*lDiv,&mem);
    }

    if (mod[0] == 0 && lDiv!=L_DIV_1024) {
      set_zero(tmp-lDiv, M);
      mvr2r(st->fdSynth-lDiv,tmp-lDiv+M,3*lDiv);
      tmp_start=-lDiv;
    } else {
      set_zero(tmp, M);
      mvr2r(st->fdSynth,tmp+M,2*lDiv);
      tmp_start=0;
    }
    set_zero(tmpRes-lDiv, 3*lDiv);
    for ( i = tmp_start ; i < 2*lDiv ; i+= L_SUBFR ) {
#else
    E_UTIL_f_preemph(st->fdSynth,PREEMPH_FAC,2*lDiv,&mem);

    set_zero(tmp, M);
    mvr2r(st->fdSynth,tmp+M,2*lDiv);
    set_zero(tmpRes, 2*lDiv);
    for ( i = 0 ; i < 2*lDiv ; i+= L_SUBFR ) {
#endif

      E_UTIL_residu(ANull,&tmp[M+i],&tmpRes[i],L_SUBFR);
    }

#ifndef USAC_REFSOFT_COR1_NOFIX_06
    if (mod[0] != 0 && (lDiv==L_DIV_1024 || mod[1]!=0) ) {
      nSamp = min(lDiv,PIT_MAX_MAX+L_INTERPOL);
    } else if (mod[0] == 0 && lDiv!=L_DIV_1024) {
      nSamp = min(3*lDiv,PIT_MAX_MAX+L_INTERPOL);
    } else {
      nSamp = min(2*lDiv,PIT_MAX_MAX+L_INTERPOL);
    }
#else
    nSamp = min(2*lDiv,PIT_MAX_MAX+L_INTERPOL);
#endif

    mvr2r(tmpRes+2*lDiv-nSamp,exc-nSamp,nSamp);
  }

  k = 0;
  while (k < NB_DIV)
  {
    mode = mod[k];

#ifndef USAC_REFSOFT_COR1_NOFIX_09
    if ((st->last_mode == 0) && (mode > 0) && (k!=0 || st->prev_BassPostFilter_activ==1)){
#else
    if ((st->last_mode == 0) && (mode > 0)){
#endif
      i = (k*nbSubfr)+SynSfd;
      pitch[i+1] = pitch[i] = pitch[i-1];
      pit_gain[i+1] = pit_gain[i] = pit_gain[i-1];
    }

    /* decode LSFs and convert LSFs to cosine domain */
    if ((mode==0) || (mode==1))
       mvr2r(&lsf[(k+1)*M], lsfnew, M);
    else if (mode==2)
       mvr2r(&lsf[(k+2)*M], lsfnew, M);
    else /*if (mode==3)*/
       mvr2r(&lsf[(k+4)*M], lsfnew, M);

    E_LPC_lsf_lsp_conversion(lsfnew, lspnew, M);

    /* Check stability on lsf : distance between old lsf and current lsf */
    tmp = 0.0f;
    for (i=0; i<M; i++) 
    {
      tmp += (lsfnew[i]-st->lsfold[i])*(lsfnew[i]-st->lsfold[i]);
    }
    stab_fac = (float)(1.25f - (tmp/400000.0f));
    if (stab_fac > 1.0f) {
      stab_fac = 1.0f;
    }
    if (stab_fac < 0.0f) {
      stab_fac = 0.0f;
    }

    /* - interpolate Ai in ISP domain (Aq) and save values for upper-band (Aq_lpc)
       - decode other parameters according to mode
       - set overlap size for next decoded frame (ovlp_size)
       - set mode for upper-band decoder (mod[]) */
    switch (mode) {
    case 0:
    case 1:
      if (mode == 0) 
      { 

        /* ACELP frame */
        int_lpc_acelp(st->lspold, lspnew, Aq, nbSubfr, M);

        decoder_acelp_fac(td, k, Aq, td->core_mode_index, bad_frame[k], &exc[k*lDiv], &synth[k*lDiv],
			&pitch[(k*nbSubfr)+SynSfd], &pit_gain[(k*nbSubfr)+SynSfd], stab_fac, st);

        if ( (st->last_mode != 0)  
#ifdef BUGFIX_BPF
             && isBassPostFilter 
#endif
             ) /* bass-postfilter also FD-ACELP FAC area */
        {
          i = (k*nbSubfr)+SynSfd;
          pitch[i-1] = pitch[i];
          pit_gain[i-1] = pit_gain[i];
#ifdef BUGFIX_BPF
          if(st->last_mode != -2){
#endif
            pitch[i-2]    = pitch[i];
            pit_gain[i-2] = pit_gain[i];
#ifdef BUGFIX_BPF
          }
#endif
        }
      } else { 
        /* short TCX frame */
        int_lpc_tcx(st->lspold, lspnew, Aq, nbSubfr, M);
        
        decoder_tcx_fac(td, k, Aq, lDiv, &exc[k*lDiv], &synth[k*lDiv], st);
        
      }
      k++;
      break;

    case 2: 
      /* medium TCX frame */
      int_lpc_tcx(st->lspold, lspnew, Aq, (nbSubfrSuperfr/2), M);

      decoder_tcx_fac(td, k, Aq, 2*lDiv, &exc[k*lDiv], &synth[k*lDiv], st);

      k+=2;
      break;
 
   case 3:
      /* long TCX frame */
      int_lpc_tcx(st->lspold, lspnew, Aq, nbSubfrSuperfr, M);

      decoder_tcx_fac(td, k, Aq, 4*lDiv, &exc[k*lDiv], &synth[k*lDiv], st);

      k+=4;
      break;

    default:
      printf("decoder error: mode > 3!\n");
      exit(0);
    }

    st->last_mode = mode;

    /* update lspold[] and lsfold[] for the next frame */
    mvr2r(lspnew, st->lspold, M);
    mvr2r(lsfnew, st->lsfold, M);
  }

  /*----- update signal for next superframe -----*/
  mvr2r(exc_buf+lFrame, st->old_exc, PIT_MAX_MAX+L_INTERPOL);

  mvr2r(synth_buf+lFrame, st->old_synth, PIT_MAX_MAX+SynDelay);

  /* check whether usage of bass postfilter was de-activated in the bitstream;
     if yes, set pitch gain to 0 */
  if (!isBassPostFilter) {
#ifndef USAC_REFSOFT_COR1_NOFIX_09
    if(mod[0]!=0 && st->prev_BassPostFilter_activ) {
      for (i=2; i<nbSubfrSuperfr; i++)
        pit_gain[SynSfd+i] = 0.0;
    } else {
      for (i=0; i<nbSubfrSuperfr; i++)
        pit_gain[SynSfd+i] = 0.0;
    }
#else
    for (i=0; i<nbSubfrSuperfr; i++)
      pit_gain[SynSfd+i] = 0.0;
#endif
  }

  /* for bass postfilter */
  for (i=0; i<SynSfd; i++){
    st->old_T_pf[i] = pitch[nbSubfrSuperfr+i];
    st->old_gain_pf[i] = pit_gain[nbSubfrSuperfr+i];
  }
#ifndef USAC_REFSOFT_COR1_NOFIX_09
  st->prev_BassPostFilter_activ=isBassPostFilter;
#endif

  /* bass post filter */
  synth = synth_buf+PIT_MAX_MAX;

  /* recalculate pitch gain to allow postfiltering on FAC area */
  for (i=0; i<nbSubfrSuperfr; i++){
    T = pitch[i];
    gain = pit_gain[i];
    if (gain > 0.0f){
      gain = get_gain(&synth[i*L_SUBFR], &synth[(i*L_SUBFR)-T], L_SUBFR);
      pit_gain[i] = gain;
    }
  }

#ifndef USAC_REFSOFT_COR1_NOFIX_08
  /* If the last frame was ACELP, the signal is correct computed up to the end of the superframe, 
   * only if the last frame is TCX, you shouldn't use the overlapp/FAC area */
  if(mod[3]==0) {
    bass_pf_1sf_delay(synth, pitch, pit_gain, fsynth,
                    lFrame, L_SUBFR, SynDelay, st->old_noise_pf);
  } else {
    bass_pf_1sf_delay(synth, pitch, pit_gain, fsynth,
                    lFrame, L_SUBFR, SynDelay-(lDiv/2), st->old_noise_pf);
  }
#else
  bass_pf_1sf_delay(synth, pitch, pit_gain, fsynth,
                    lFrame, L_SUBFR, SynDelay-(lDiv/2), st->old_noise_pf);
#endif

  return;
}


int  decoder_LPD_end(HANDLE_USAC_TD_DECODER tddec,
                     HANDLE_BUFFER     hVm,
                     USAC_DATA *usac_data,
                     int i_ch)
{
  int err = 0;
  int i;
  float dsynth[L_FRAME_1024];
  
  /* Decode the LPD data */
  decoder_Synth_end( dsynth, tddec);
  
  if ( usac_data->twMdct[0] == 1 ){    
    for (i = 0; i < usac_data->block_size_samples; i++){
      usac_data->overlap_buffer[i_ch][i + (usac_data->block_size_samples/2)] = dsynth[i];
    }
  } else {
    for (i = 0; i <  usac_data->block_size_samples; i++){
      usac_data->overlap_buffer[i_ch][i] = dsynth[i];
    }
  }

  usac_data->windowShape[i_ch]        = WS_FHG;
  usac_data->windowSequenceLast[i_ch] = EIGHT_SHORT_SEQUENCE;
  usac_data->FrameWasTD[i_ch]         = 1;

  /* get last LPC and ACELP ZIR */
  if ( tddec->last_mode == 0 ) {
    float *lastLPC = getLastLpc(tddec);
    float *acelpZIR = getAcelpZir(tddec);
    copyFLOAT(lastLPC, usac_data->lastLPC[i_ch], M+1);
    copyFLOAT(acelpZIR, usac_data->acelpZIR[i_ch], 1+(2*LFAC_1024));
  }

  return err;
}

void decoder_Synth_end(
  float signal_out[],/* output: signal with LPD delay (7 subfrs) */
  HANDLE_USAC_TD_DECODER st)       /* i/o:    decoder memory state pointer     */
{
  int i;
  float synth_buf[PIT_MAX_MAX+SYN_DELAY_1024+L_FRAME_1024];
  float *synth;
  int lFrame, LpdSfd, LpdDelay, SynSfd, SynDelay, lFAC;

  lFrame   = st->lFrame;
  LpdSfd   = (NB_DIV*st->nbSubfr)/2;
  LpdDelay = LpdSfd*L_SUBFR;
  SynSfd   = LpdSfd-BPF_SFD;
  SynDelay = SynSfd*L_SUBFR;
  lFAC     = (st->lDiv)/2;

  set_zero(synth_buf,PIT_MAX_MAX+SynDelay+lFrame);
  /* Initialize pointers for synthesis */
  synth = synth_buf+PIT_MAX_MAX+SynDelay;
  mvr2r(st->old_synth, synth_buf, PIT_MAX_MAX+SynDelay);

  /* copy last unwindowed TXC ovlp part to synth */
  for ( i = 0 ; i < 2*lFAC; i++ ){
    synth[i-lFAC] = st->old_xnq[i+1];
  }
  synth = synth_buf+PIT_MAX_MAX-(BPF_SFD*L_SUBFR);
  mvr2r(synth, signal_out, lFrame);
  set_zero(signal_out + LpdDelay + lFAC, lFrame - LpdDelay - lFAC);


  return;
}

void decoder_LPD_BPF_end(
  int   isShort,
  float out_buffer[],/* i/o: signal with LPD delay (7 subfrs) */
  HANDLE_USAC_TD_DECODER st)       /* i/o:    decoder memory state pointer     */
{
  int i, T;
  float synth_buf[PIT_MAX_MAX+SYN_DELAY_1024+L_FRAME_1024/*+LFAC*/];
  float signal_out[L_FRAME_1024];
  float *synth;
  int   pitch[LPD_SFD_1024+3];
  float gain, pit_gain[LPD_SFD_1024+3];
  int lFrame, LpdSfd, LpdDelay, SynSfd, SynDelay, lFAC;

  lFrame   = st->lFrame;
  LpdSfd   = (NB_DIV*st->nbSubfr)/2;
  LpdDelay = LpdSfd*L_SUBFR;
  SynSfd   = LpdSfd-BPF_SFD;
  SynDelay = SynSfd*L_SUBFR;
  lFAC     = (st->lDiv)/2;

  set_zero(synth_buf, PIT_MAX_MAX + SynDelay + lFrame);
  /* Initialize pointers for synthesis */
  synth = synth_buf + PIT_MAX_MAX + SynDelay;
  mvr2r(st->old_synth, synth_buf, PIT_MAX_MAX + SynDelay);
  mvr2r(out_buffer, synth_buf + PIT_MAX_MAX - (BPF_SFD*L_SUBFR), SynDelay + lFrame + (BPF_SFD*L_SUBFR));

  for (i=0; i<SynSfd; i++){
    pitch[i] = st->old_T_pf[i];
    pit_gain[i] = st->old_gain_pf[i];
  }
  for (i=SynSfd; i<LpdSfd + 3; i++){
    pitch[i] = L_SUBFR;
    pit_gain[i] = 0.0f;
  }
  if ( st->last_mode == 0 ) {
    pitch[SynSfd]    = pitch[SynSfd - 1];
    pit_gain[SynSfd] = pit_gain[SynSfd - 1];
    if ( !isShort ) {
      pitch[SynSfd + 1]    = pitch[SynSfd];
      pit_gain[SynSfd + 1] = pit_gain[SynSfd];
    }
  }

  /* bass post filter */
  synth = synth_buf+PIT_MAX_MAX;

  /* recalculate pitch gain to allow postfilering on FAC area */
  for (i=0; i<SynSfd + 2; i++){
    T = pitch[i];
    gain = pit_gain[i];
    if (gain > 0.0f){
      gain = get_gain(&synth[i*L_SUBFR], &synth[(i*L_SUBFR)-T], L_SUBFR);
      pit_gain[i] = gain;
    }
  }

#ifndef USAC_REFSOFT_COR1_NOFIX_08
  bass_pf_1sf_delay(synth, pitch, pit_gain, signal_out,
                    (LpdSfd+2)*L_SUBFR+(BPF_SFD*L_SUBFR), L_SUBFR, lFrame-(LpdSfd+4)*L_SUBFR, st->old_noise_pf);
#else
  bass_pf_1sf_delay(synth, pitch, pit_gain, signal_out,
                    (LpdSfd+2)*L_SUBFR+(BPF_SFD*L_SUBFR), L_SUBFR, lFrame-(LpdSfd+2)*L_SUBFR, st->old_noise_pf);
#endif

  mvr2r(signal_out,out_buffer,(LpdSfd+2)*L_SUBFR+(BPF_SFD*L_SUBFR));
  /*set_zero(signal_out+LPD_DELAY + LFAC, L_FRAME_PLUS-LPD_DELAY - LFAC);*/

  return;
}

static void bass_pf_1sf_delay(
  float *syn,       /* (i) : 12.8kHz synthesis to postfilter             */
  int *T_sf,        /* (i) : Pitch period for all subframes (T_sf[16])   */
  float *gainT_sf,  /* (i) : Pitch gain for all subframes (gainT_sf[16]) */
  float *synth_out, /* (o) : filtered synthesis (with delay of 1 subfr)  */
  int l_frame,      /* (i) : frame length (should be multiple of l_subfr)*/
  int l_subfr,      /* (i) : sub-frame length (60/64)                    */
  int l_next,       /* (i) : look ahead for symetric filtering           */
  float mem_bpf[])  /* i/o : memory state [L_FILT+L_SUBFR]               */
{
  int i, j, sf, i_subfr, T, T2, lg;
  float tmp, ener, corr, gain;
  float noise_buf[L_FILT+(2*L_SUBFR)], *noise, *noise_in, *x, *y;

  noise = noise_buf + L_FILT;
  noise_in = noise_buf + L_FILT + l_subfr;

  mvr2r(syn-l_subfr, synth_out, l_frame);

  /* Because the BPF does not always operate on multiples of 64 samples */
  if (l_frame%64)
  {
    set_zero(synth_out+l_frame,L_SUBFR-l_frame%64);
  }

  sf = 0;
  for (i_subfr=0; i_subfr<l_frame; i_subfr+=l_subfr, sf++){
    T = T_sf[sf];
    gain = gainT_sf[sf];

    if (gain > 1.0f) gain = 1.0f;
    if (gain < 0.0f) gain = 0.0f;

    /* pitch tracker: test pitch/2 to avoid continuous pitch doubling */
    /* Note: pitch is limited to PIT_MIN (34 = 376Hz) at the encoder  */
    T2 = T>>1;
    x = &syn[i_subfr-L_EXTRA];
    y = &syn[i_subfr-T2-L_EXTRA];

    ener = 0.01f;
    corr = 0.01f;
    tmp  = 0.01f;
    for (i=0; i<l_subfr+L_EXTRA; i++){
      ener += x[i]*x[i];
      corr += x[i]*y[i];
      tmp  += y[i]*y[i];
    }
    /* use T2 if normalized correlation > 0.95 */
    tmp = corr / (float)sqrt(ener*tmp);
    if (tmp > 0.95f) T = T2;

    lg = l_frame + l_next - T - i_subfr;
    if (lg < 0) lg = 0;
    if (lg > l_subfr) lg = l_subfr;

    if (gain > 0){
      /* limit gain to avoid problem on burst */
      if (lg > 0){
        tmp = 0.01f;
        for (i=0; i<lg; i++){
          tmp += syn[i+i_subfr] * syn[i+i_subfr];
        }
        ener = 0.01f;
        for (i=0; i<lg; i++){
          ener += syn[i+i_subfr+T] * syn[i+i_subfr+T];
        }
        tmp = (float)sqrt(tmp / ener);
        if (tmp < gain) gain = tmp;
      }

      /* calculate noise based on voiced pitch */
      tmp = gain*0.5f;
      for (i=0; i<lg; i++){
        noise_in[i] = tmp * (syn[i+i_subfr] - 0.5f*syn[i+i_subfr-T] - 0.5f*syn[i+i_subfr+T]);
      }
      for (i=lg; i<l_subfr; i++){
        noise_in[i] = tmp * (syn[i+i_subfr] - syn[i+i_subfr-T]);
      }
    } else {
      set_zero(noise_in, l_subfr);
    }

    mvr2r(mem_bpf, noise_buf, L_FILT+l_subfr);
    mvr2r(noise_buf+l_subfr, mem_bpf, L_FILT+l_subfr);

    /* substract from voiced speech low-pass filtered noise */
    for (i=0; i<l_subfr; i++){
      tmp = filt_lp[0] * noise[i];
      for(j=1; j<=L_FILT; j++){
        tmp += filt_lp[j] * (noise[i-j] + noise[i+j]);
      }
      synth_out[i+i_subfr] -= tmp;
    }
  }

  return;
}


float *getLastLpc(HANDLE_USAC_TD_DECODER st) {
#ifdef USAC_REFSOFT_COR1_NOFIX_01
  return st->old_Aq;
#else
  return &st->old_Aq[M+1];
#endif
}

float *getAcelpZir(HANDLE_USAC_TD_DECODER st) {
  return st->old_xnq;
}
