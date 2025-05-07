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
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>

#include	"nec_abs_proto.h"
#include	"nec_abs_const.h"
#include	"nec_exc_proto.h"

#define NEC_LAG_IDX_RNG	8

void nec_bws_excitation_analysis(
	float InputSignal[],		/* input */
	float LpcCoef[],		/* input */
	float WnumCoef[],		/* input */
	float WdenCoef[],		/* input */
	long  shape_indices[],		/* output */
	long  gain_indices[],		/* output */
	float decoded_excitation[],	/* output */
	long  lpc_order,		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	float bws_mp_exc[],		/* input */
	long  *acb_index_8,		/* input */
	long  rms_index_8,		/* in: RMS code index */
	long  signal_mode,		/* in: signal mode */
        float *mem_past_syn,            /* in/out */
        long  *flag_rms,                /* in/out */
        float *pqxnorm,                 /* in/out */
        short prev_vad                  /* in */
)
{
   static float mem_past_exc[NEC_PITCH_MAX_MAXIMUM + NEC_PITCH_IFTAP16+1];
   static float mem_past_wsyn[NEC_LPC_ORDER_MAXIMUM];
   static float mem_past_in[NEC_LPC_ORDER_MAXIMUM];
   static float mem_past_win[NEC_LPC_ORDER_MAXIMUM];
   static float qxnorm[NEC_MAX_NSF];
   static long  flag_mem = 0;
   static long	op_lag_idx[NEC_MAX_NSF], vu_flag;
   static long	c_subframe;

   float	*acbexc, *synacb;
   float	*target, *mpexc, *xr, *fk;
   float	*pmw;
   long		i;
   long		st_idx, ed_idx, integer_lag;
   long		lag_idx, mp_pos_idx, mp_sgn_idx, ga_idx;
   float	og_ac;
   float	g_ac, g_ec;
   long		lagbit, posbit, sgnbit, gainbit;
   long		num_pulse;
   long         pitch_min, pitch_max, pitch_lmt;
   float        *mpexc_8, g_mp8;
   long 	ip16, fp16;

/* set pitch minimum/maximum */
   pitch_min = NEC_PITCH_MIN_FRQ16;
   pitch_max = NEC_PITCH_MAX_FRQ16;
   pitch_lmt = NEC_PITCH_LIMIT_FRQ16;

   if (flag_mem == 0 || prev_vad != 1) {
      for ( i = 0; i < pitch_max +NEC_PITCH_IFTAP16+1; i++ )
	 mem_past_exc[i] = 0.0;
      for ( i = 0; i < lpc_order; i++ ) {
	 mem_past_wsyn[i] = 0.0;
	 mem_past_in[i] = 0.0;
	 mem_past_win[i] = 0.0;
      }
      c_subframe = 0;
      flag_mem = 1;
   }
   c_subframe = c_subframe % n_subframes;

   lagbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+0];
   posbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+1];
   sgnbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+2];
   gainbit=frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+3];

   /* Frame Operation */
   if(c_subframe==0) {
      /*=====acb_index_8 >>> op_lag_inx=====*/
      for(i = 0; i < n_subframes; i++){
	 if(acb_index_8[i] <= 161) {
	    ip16 = (17 + 2 * acb_index_8[i] / NEC_PITCH_RSLTN) * 2;
	    fp16 = (2 * acb_index_8[i]) % NEC_PITCH_RSLTN;
	 } else if(acb_index_8[i] <= 199) {
	    ip16 = (71 + 3 * (acb_index_8[i] - 162)/ NEC_PITCH_RSLTN) * 2;
	    fp16 = (3 * (acb_index_8[i] - 162)) % NEC_PITCH_RSLTN;
	 } else if(acb_index_8[i] <= 254) {
	    ip16 = (90 + (acb_index_8[i] - 200)) * 2;
	    fp16 = 0;
	 }else {
	    ip16 = 0;
	    fp16 = 0;
	 }
	 if ( fp16 != 0 ) ip16++;
	 
	 if(ip16==0)  op_lag_idx[i] = pitch_lmt;
	 else         op_lag_idx[i] = (ip16 - 32)*NEC_PITCH_RSLTN/2 + 2;
      }

      /*=====set vu_flag=====*/
      vu_flag = signal_mode;

      /* RMS */
      if ( vu_flag == 0 ) {
	 nec_bws_rms_dec(qxnorm, n_subframes,
			 (float)NEC_RMS_MAX_U, (float)NEC_MU_LAW_U,
                         (long)NEC_BIT_RMS, rms_index_8, flag_rms, pqxnorm);
      } else {
	 nec_bws_rms_dec(qxnorm, n_subframes,
			 (float)NEC_RMS_MAX_V, (float)NEC_MU_LAW_V,
                         (long)NEC_BIT_RMS, rms_index_8, flag_rms, pqxnorm);
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
   if((mpexc_8 = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
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
   if(op_lag_idx[c_subframe] == NEC_PITCH_LIMIT_FRQ16){
     st_idx = NEC_PITCH_LIMIT_FRQ16;
     ed_idx = NEC_PITCH_LIMIT_FRQ16;
   }
   else{
     st_idx = op_lag_idx[c_subframe] - NEC_LAG_IDX_RNG/2;
     ed_idx = op_lag_idx[c_subframe] + NEC_LAG_IDX_RNG/2 - 1;
     if(st_idx < 0) {
       st_idx = 0;
       ed_idx = st_idx + NEC_LAG_IDX_RNG -1;
     }
     if(ed_idx >= pitch_lmt) {
       ed_idx = pitch_lmt-1;
       st_idx = ed_idx - NEC_LAG_IDX_RNG +1;
     }
   }

   nec_bws_acb_enc(target, &og_ac, acbexc, synacb,
		   op_lag_idx[c_subframe],
		   st_idx, ed_idx, &lag_idx, lpc_order, sbfrm_size,
		   lagbit,
		   &LpcCoef[lpc_order*c_subframe],
		   &WdenCoef[lpc_order*c_subframe],
		   &WnumCoef[lpc_order*c_subframe], mem_past_exc, 
		   &integer_lag);

   /* Multi-Pulse Excitation Encoding */
   for ( i = 0; i < sbfrm_size/2; i++ ) {
      mpexc_8[2*i] = bws_mp_exc[i];
      mpexc_8[2*i+1] = 0.0;
   }

   num_pulse = sgnbit;
   nec_bws_mp_enc(vu_flag,target, synacb, og_ac, &g_ac, &g_ec, &g_mp8,
	      qxnorm[c_subframe], integer_lag,
	      &mp_pos_idx, &mp_sgn_idx, mpexc, acbexc, mpexc_8,
	      &LpcCoef[lpc_order*c_subframe],
	      &WdenCoef[lpc_order*c_subframe],
	      &WnumCoef[lpc_order*c_subframe],
	      lpc_order, sbfrm_size, num_pulse, gainbit, &ga_idx );


   /* set INDICES */
   if(op_lag_idx[c_subframe] ==  NEC_PITCH_LIMIT_FRQ16)
      shape_indices[c_subframe*num_shape_cbks+0] = 0;
   else
      shape_indices[c_subframe*num_shape_cbks+0] = lag_idx - st_idx;
   shape_indices[c_subframe*num_shape_cbks+1] = mp_pos_idx;
   shape_indices[c_subframe*num_shape_cbks+2] = mp_sgn_idx;
   gain_indices[c_subframe*num_gain_cbks+0] = ga_idx;

   for(i = 0; i < sbfrm_size; i++){
      decoded_excitation[i] = g_ac * acbexc[i] + g_ec * mpexc[i] + g_mp8 * mpexc_8[i];
   }

   for(i = 0; i < pitch_max + NEC_PITCH_IFTAP16+1 - sbfrm_size; i++){
      mem_past_exc[i] = mem_past_exc[i + sbfrm_size];
   }
   for(i = 0; i < sbfrm_size; i++){
      mem_past_exc[pitch_max + NEC_PITCH_IFTAP16+1 - sbfrm_size + i] =
	 decoded_excitation[i];
   }

   /*------ Memory Allocation ----------*/
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

   if(prev_vad == 1){
       for ( i = 0; i < lpc_order; i++) pmw[i] = mem_past_syn[i];
   }else{
       for ( i = 0; i < lpc_order; i++) pmw[i] = 0.0;
   }
   nec_syn_filt(decoded_excitation, &LpcCoef[lpc_order*c_subframe],
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
   free( mpexc_8);
   free( pmw );
}
