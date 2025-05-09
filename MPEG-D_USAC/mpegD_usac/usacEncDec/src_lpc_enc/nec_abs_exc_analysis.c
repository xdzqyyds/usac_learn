/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	Excitation Analysis Subroutines
 *
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 *	Ver2.0	97.03.17	T.Nomura(NEC)
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>

#include "lpc_common.h"
#include "nec_abs_proto.h"
#include "nec_abs_const.h"
#include "nec_exc_proto.h"

#define NEC_LAG_IDX_RNG	16

void nec_abs_excitation_analysis (
	float InputSignal[],		/* input */
	float LpcCoef[],		/* input */
	float WnumCoef[],		/* input */
	float WdenCoef[],		/* input */
	long  shape_indices[],		/* output */
	long  gain_indices[],		/* output */
	long  *rms_index,		/* output */
	long  *signal_mode,		/* output */
	float decoded_excitation[],	/* output */
	long  lpc_order,		/* configuration input */
	long  frame_size, 		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	long  n_enhstages,		/* configuration input */
	float bws_mp_exc[],
    long SampleRateMode,
        float *mem_past_syn,            /* in/out */
        long  *flag_rms,                /* in/out */
        float *pqxnorm,                 /* in/out */
        short prev_vad                  /* in */
)
{
   static float mem_past_exc[NEC_PITCH_MAX_FRQ16 + NEC_PITCH_IFTAP16+1];
   static float mem_past_wsyn[NEC_LPC_ORDER_FRQ16];
   static float mem_past_in[NEC_LPC_ORDER_FRQ16];
   static float mem_past_win[NEC_LPC_ORDER_FRQ16];
   static float qxnorm[NEC_MAX_NSF];
   static long  flag_mem = 0;
   static long	op_lag_idx[NEC_MAX_NSF], vu_flag;
   static long  c_subframe = 0;
   static long  idx_max, pitch_max, pitch_iftap;

   float	*CoreExcitation, *PreStageExcitation;
   float	*synp, og_pc;
   float	*acbexc, *synacb;
   float	*target, *mpexc, *xr, *fk;
   float	*pmw;
   long		i, j;
   float	xnorm[NEC_MAX_NSF];
   long		st_idx, ed_idx, integer_lag;
   long		lag_idx, mp_pos_idx, mp_sgn_idx, ga_idx;
   float	og_ac;
   float	g_ac, g_ec, g_pc;
   long		rmsbit, lagbit, posbit, sgnbit, gainbit;
   long		c_enh, idx_ctr, *num_pulse, *pre_indices;

   if (flag_mem == 0) {
      if(fs8kHz==SampleRateMode) {
        idx_max = 255;
        pitch_max = NEC_PITCH_MAX;
        pitch_iftap = NEC_PITCH_IFTAP;
      }else {
        idx_max = 511;
        pitch_max = NEC_PITCH_MAX_FRQ16;
        pitch_iftap = NEC_PITCH_IFTAP16;
      }
   }

   if (flag_mem == 0 || prev_vad != 1) {

      for ( i = 0; i < pitch_max+pitch_iftap+1; i++ ) {
	 mem_past_exc[i] = 0.0;
      }
      for ( i = 0; i < lpc_order; i++ ) {
	 mem_past_wsyn[i] = 0.0;
	 mem_past_in[i] = 0.0;
	 mem_past_win[i] = 0.0;
      }
      flag_mem = 1;
   }
   c_subframe = c_subframe % n_subframes;

   rmsbit =frame_bit_allocation[1];
   lagbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+0];
   posbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+1];
   sgnbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+2];
   gainbit=frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+3];

   /* Frame Operation */
   if(c_subframe==0) {
      /* Mode Decision */
      nec_mode_decision(InputSignal,
			WnumCoef, WdenCoef,
			lpc_order, frame_size, sbfrm_size,
			op_lag_idx, &vu_flag, SampleRateMode );
      *signal_mode = vu_flag;

      /* RMS */
      for ( i = 0; i < n_subframes; i++ ) {
	 xnorm[i] = 0.0;
	 for ( j = 0; j < sbfrm_size; j++ ) {
	    xnorm[i] += (InputSignal[i*sbfrm_size+j]
			 * InputSignal[i*sbfrm_size+j]);
	 }
	 xnorm[i] = (float)sqrt(xnorm[i] / (float)sbfrm_size);
      }
      if ( vu_flag == 0 ) {
	 nec_enc_rms(xnorm, qxnorm, n_subframes,
		     (float)NEC_RMS_MAX_U, (float)NEC_MU_LAW_U,
                     rmsbit, rms_index, flag_rms, pqxnorm);
      } else {
	 nec_enc_rms(xnorm, qxnorm, n_subframes,
		     (float)NEC_RMS_MAX_V, (float)NEC_MU_LAW_V,
                     rmsbit, rms_index, flag_rms, pqxnorm);
      }
   }
   qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);

   /*------ Memory Allocation ----------*/
   if((target = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((acbexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((synacb = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((mpexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }

   /* Make Target Signal */
   nec_mk_target( InputSignal, target, sbfrm_size, lpc_order,
		     &LpcCoef[lpc_order*c_subframe],
		     &WdenCoef[lpc_order*c_subframe],
		     &WnumCoef[lpc_order*c_subframe],
		     mem_past_in, mem_past_win,
		     mem_past_syn,
                     mem_past_wsyn, prev_vad);

   /* Adaptive Code Book Search */
   st_idx = op_lag_idx[c_subframe] - NEC_LAG_IDX_RNG/2;
   ed_idx = op_lag_idx[c_subframe] + NEC_LAG_IDX_RNG/2;
   if(st_idx < 0) {
      st_idx = 0;
      ed_idx = st_idx + NEC_LAG_IDX_RNG;
   }
   if(ed_idx > idx_max-1) {
      ed_idx = idx_max-1;
      st_idx = ed_idx - NEC_LAG_IDX_RNG;
   }
   nec_enc_acb(target, &og_ac, acbexc, synacb,
	       op_lag_idx[c_subframe],
	       st_idx, ed_idx, &lag_idx, lpc_order, sbfrm_size,
	       lagbit,
	       &LpcCoef[lpc_order*c_subframe],
	       &WdenCoef[lpc_order*c_subframe],
	       &WnumCoef[lpc_order*c_subframe], mem_past_exc, &integer_lag,
	       SampleRateMode);

   /* Multi-Pulse Excitation Encoding */
   nec_enc_mp(vu_flag,target, synacb, og_ac, &g_ac, &g_ec,
	      qxnorm[c_subframe], integer_lag,
	      &mp_pos_idx, &mp_sgn_idx, mpexc, acbexc,
	      &LpcCoef[lpc_order*c_subframe],
	      &WdenCoef[lpc_order*c_subframe],
	      &WnumCoef[lpc_order*c_subframe],
	      lpc_order, sbfrm_size, sgnbit, gainbit, &ga_idx);

   /* set INDICES */
   shape_indices[c_subframe*num_shape_cbks+0] = lag_idx;
   shape_indices[c_subframe*num_shape_cbks+1] = mp_pos_idx;
   shape_indices[c_subframe*num_shape_cbks+2] = mp_sgn_idx;
   gain_indices[c_subframe*num_gain_cbks+0] = ga_idx;

   /* Enhanced Multi-Pulse Excitation Encoding */
   if((CoreExcitation = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((PreStageExcitation=(float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((synp = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((num_pulse = (long *)calloc (n_enhstages+1, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((pre_indices = (long *)calloc (n_enhstages, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }

   for(i = 0; i < sbfrm_size; i++){
      CoreExcitation[i] = g_ac * acbexc[i] + g_ec * mpexc[i];
      bws_mp_exc[i] = g_ec * mpexc[i];
      PreStageExcitation[i] = CoreExcitation[i];
   }

   num_pulse[0] = sgnbit;
   for ( c_enh = 0; c_enh < n_enhstages; c_enh++ ) {
      nec_zero_filt(PreStageExcitation, synp,
		    &LpcCoef[lpc_order*c_subframe],
		    &WdenCoef[lpc_order*c_subframe],
		    &WnumCoef[lpc_order*c_subframe],
		    lpc_order, sbfrm_size );
      og_pc = 1.0;

      idx_ctr = (c_enh+1)*n_subframes + c_subframe;
      num_pulse[c_enh+1] = frame_bit_allocation[2+idx_ctr*(num_shape_cbks+num_gain_cbks)+2];
      gainbit = frame_bit_allocation[2+idx_ctr*(num_shape_cbks+num_gain_cbks)+3];
      pre_indices[c_enh] = mp_pos_idx;

      nec_enh_mp_enc(vu_flag,target, synp, og_pc, &g_pc, &g_ec,
		     qxnorm[c_subframe], integer_lag,
		     &mp_pos_idx, &mp_sgn_idx, mpexc, PreStageExcitation,
		     &LpcCoef[lpc_order*c_subframe],
		     &WdenCoef[lpc_order*c_subframe],
		     &WnumCoef[lpc_order*c_subframe],
		     lpc_order, sbfrm_size, num_pulse, pre_indices,
		     c_enh+1, gainbit, &ga_idx);

      shape_indices[idx_ctr*num_shape_cbks+0] = 0;
      shape_indices[idx_ctr*num_shape_cbks+1] = mp_pos_idx;
      shape_indices[idx_ctr*num_shape_cbks+2] = mp_sgn_idx;
      gain_indices[idx_ctr*num_gain_cbks+0] = ga_idx;

      for(i = 0; i < sbfrm_size; i++){
	 PreStageExcitation[i] = g_pc*PreStageExcitation[i] + g_ec*mpexc[i];
	 bws_mp_exc[i] += g_ec * mpexc[i];
      }
   }

   for(i = 0; i < sbfrm_size; i++){
      decoded_excitation[i] = PreStageExcitation[i];
   }

   free( PreStageExcitation );
   free( synp );
   free( num_pulse );
   free( pre_indices );

   /* Memory Update */
   if((xr = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((fk = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((pmw = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }

   for(i = 0; i < pitch_max + pitch_iftap+1 - sbfrm_size; i++){
      mem_past_exc[i] = mem_past_exc[i + sbfrm_size];
   }
   for(i = 0; i < sbfrm_size; i++){
      mem_past_exc[pitch_max + pitch_iftap+1 - sbfrm_size + i] =
	 CoreExcitation[i];
   }

   if(prev_vad == 1){
       for ( i = 0; i < lpc_order; i++) pmw[i] = mem_past_syn[i];
   }else{
       for ( i = 0; i < lpc_order; i++) pmw[i] = 0.0;
   }
   nec_syn_filt(CoreExcitation, &LpcCoef[lpc_order*c_subframe],
		mem_past_syn, xr, lpc_order, sbfrm_size);
   nec_pw_filt(fk, xr, lpc_order,
	       &WdenCoef[lpc_order*c_subframe],
	       &WnumCoef[lpc_order*c_subframe], pmw, mem_past_wsyn,
	       sbfrm_size);

   c_subframe++;

   free( acbexc );
   free( synacb );
   free( target );
   free( xr );
   free( fk );
   free( mpexc );
   free( pmw );

   free( CoreExcitation );

}
