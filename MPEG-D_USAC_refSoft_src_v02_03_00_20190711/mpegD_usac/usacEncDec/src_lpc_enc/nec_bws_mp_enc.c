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
 *	Multi-Pulse Excitation Encoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_abs_const.h"
#include "nec_abs_proto.h"
#include "nec_exc_proto.h"

#define NEC_NUM_PRUNING	32
#define NEC_NUM_CAND	4

void nec_bws_mp_enc(
		long	vu_flag,	/* input */
		float	z[],		/* input */
		float	syn_ac[],	/* input */
		float	og_ac,		/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float   *g_mp8,
		float	qxnorm,		/* input */
		long	I_part,		/* input */
		long	*pos_idx,	/* output */
		long	*sgn_idx,	/* output */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		float   mpexc_8[],
		float	alpha[],	/* input */
		float	g_den[],	/* input */
		float	g_num[],	/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	*ga_idx
	         )	
{
   static long	num_prune = NEC_NUM_PRUNING;
   static long	cand_gain = NEC_NUM_CAND;

   long		l, i, j, k;
   long		ptr_i, ptr_j;
   long		sidx = 0;
   long         np, np2;
   long		kn, kt, mn;
   long		now_cand, nxt_cand;
   long		*bit_pos, *num_pos, *chn_pos;
   long		*pul_loc;
   long		*loc[NEC_NUM_PRUNING], *loc2[NEC_NUM_PRUNING];
   float	max_bf2ps, ps0, bf0;
   float	ps[NEC_NUM_PRUNING];
   float	bf[NEC_NUM_PRUNING];
   float	*pst, *bft, *buf_bf2ps;
   float	*tmp_exc;
   float	*zmsa1, *dest, *hw_mp, *sgn, *fai;
   float	fai_sum;
   float	*pul_amp;

   float *syn_mp8;
   float zsax, sasa;
   float zsn, sasn, snsn;
   float oga, ogn, gn, g0;


   /*------- Set Multi-Pulse Configuration -------*/
   if((bit_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((num_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((chn_pos = (long *)calloc (num_pulse*len_sf, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }

   nec_mp_position(len_sf, num_pulse, bit_pos, chn_pos);
   for ( i = 0; i < num_pulse; i++ ) num_pos[i] = 1 << bit_pos[i];

   /*------- Computation of the target signal --------*/
   if((zmsa1 = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((dest = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((hw_mp = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((fai = (float *)calloc (len_sf*len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((sgn = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }

   if((syn_mp8 = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
    }

   nec_zero_filt(mpexc_8, syn_mp8, alpha, g_den, g_num, lpc_order, len_sf);

   /*--- <z,sa1> ---*/
   zsax = 0.0;
   for (i = 0; i < len_sf; i++) zsax += z[i] * syn_ac[i];
   /*--- <sa1,sa1> ---*/
   sasa = 0.0;
   for (i = 0; i < len_sf; i++) sasa += syn_ac[i] * syn_ac[i];
   /*--- <z,sn> ---*/
   zsn = 0.0;
   for (i = 0; i < len_sf; i++) zsn += z[i] * syn_mp8[i];
   /*--- <sa1,sn> ---*/
   sasn = 0.0;
   for (i = 0; i < len_sf; i++) sasn += syn_ac[i] * syn_mp8[i];
   /*--- <sn,sn> ---*/
   snsn = 0.0;
   for (i = 0; i < len_sf; i++) snsn += syn_mp8[i] * syn_mp8[i];

   oga = og_ac;
   gn  = (float)((sasa == 0.0) ? 0.0 : sasn/sasa);
   g0 = snsn - gn * sasn;
   ogn = (float)((g0 == 0.0) ? 0.0 : (zsn - gn * zsax)/g0);
   oga = oga - gn * ogn;

   for (i = 0; i < len_sf; i++) {
      zmsa1[i] = z[i] - oga * syn_ac[i] - ogn * syn_mp8[i];
   }
   nec_pw_imprs(hw_mp, alpha, lpc_order, g_den, g_num, len_sf);
   nec_comb_filt(hw_mp, hw_mp, len_sf, I_part, vu_flag);

   for (i = 0; i < len_sf; i++) {
      dest[i] = 0.0;
      for (j = i; j < len_sf; j++) {
	 dest[i] += zmsa1[j] * hw_mp[j - i];
      }
   }
   for (l = 0; l < len_sf; l++) {
      ptr_i = l;
      ptr_j = 0;
      fai_sum = 0.0;
      for (j = len_sf-1; j >= l; j--) {
	 i = j-l;
	 fai_sum += hw_mp[ptr_i++]*hw_mp[ptr_j++];
	 fai[i * len_sf + j] = fai[j * len_sf + i] = fai_sum;
      }
   }

   for (i = 0; i < len_sf; i++) {
      sgn[i] = 1.0;
      if (dest[i] < 0.0) sgn[i] = -1.0;
     dest[i] = sgn[i] * dest[i];
   }
   for (i = 0; i < len_sf; i++) {
      for (j = 0; j < len_sf; j++) {
	 if (i != j) fai[i*len_sf+j] = (float)(2.0 * sgn[i] * sgn[j] * fai[i*len_sf+j]);
      }
   }
   free( zmsa1 );
   free( hw_mp );

   /*---------- Multi-Pulse Postion Search ----------*/
   if((pst = (float *)calloc (num_prune*len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((bft = (float *)calloc (num_prune*len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((buf_bf2ps = (float *)calloc (num_prune*len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((pul_amp = (float *)calloc (cand_gain*num_pulse, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((pul_loc = (long *)calloc (cand_gain*num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   for ( i = 0; i < num_prune; i++ ) {
      if((loc[i] = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
	 printf("\n Memory allocation error in nec_enc_mp \n");
	 exit(1);
      }
      if((loc2[i] = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
	 printf("\n Memory allocation error in nec_enc_mp \n");
	 exit(1);
     }
   }

	/*=====init=====*/
   for (np = 0; np < num_pos[0]; np++) {
     kn = chn_pos[0*len_sf+np];
      loc[np][0] = kn;
      bf[np] = dest[kn];
      ps[np] = fai[kn * len_sf +kn];
   }
   for (np = num_pos[0]; np < num_prune; np++) {
      loc[np][0] = -1;
   }

   now_cand = num_pos[0];
   for (i = 1; i < num_pulse; i++) {
      for (np = 0; np < now_cand; np++) {
	 if (loc[np][i - 1] == -1)
	    for (mn = 0; mn < num_pos[i]; mn++) {
	       bft[np * num_pos[i] + mn] = 0.0;
	       pst[np * num_pos[i] + mn] = 0.0;
	       buf_bf2ps[np * num_pos[i] + mn] = 0.0;
	    }
	 else
	    for (mn = 0; mn < num_pos[i]; mn++) {
	       kn = chn_pos[i*len_sf+mn];
	       bf0 = bf[np] + dest[kn];
	       ps0 = ps[np] + fai[kn * len_sf + kn];

	       for (k = i - 1; k >= 0; k--) {
			  kt = loc[np][k];
			  ps0 += fai[kn * len_sf + kt];
	       }

	       bft[np * num_pos[i] + mn] = bf0;
	       pst[np * num_pos[i] + mn] = ps0;
	       buf_bf2ps[np * num_pos[i] + mn]
		  = (float)((ps0 == 0.0) ? 0.0 : bf0 * bf0 / ps0);
	    }
      }

      nxt_cand = now_cand * num_pos[i];
      now_cand = nxt_cand;
      if (nxt_cand > num_prune)
	 nxt_cand = num_prune;

      for (np = 0; np < nxt_cand; np++) {
	 max_bf2ps = (float)(-1.0e30);
	 for (k = 0; k < now_cand; k++) {
	    if (max_bf2ps < buf_bf2ps[k]) {
	       max_bf2ps = buf_bf2ps[k];
	       sidx = k;
	    }
	 }
	 buf_bf2ps[sidx] = (float)(-1.0e30);
	 bf[np] = bft[sidx];
	 ps[np] = pst[sidx];
	 mn = sidx % num_pos[i];
	 np2 = sidx / num_pos[i];
	 for (j = 0; j < i; j++)
	    loc2[np][j] = loc[np2][j];
	 loc2[np][i] = chn_pos[i*len_sf+mn];
      }
      for (np = 0; np < nxt_cand; np++) {
	 for (j = 0; j <= i; j++)
	    loc[np][j] = loc2[np][j];
      }
      for (np = nxt_cand; np < num_prune; np++) {
	 for (j = 0; j <= i; j++)
	    loc[np][j] = -1;
      }
      now_cand = nxt_cand;
   }

      for(l = 0; l < cand_gain; l++){
         for (i = 0; i < num_pulse; i++){
            pul_loc[l*num_pulse+i] = loc[l][i];
            pul_amp[l*num_pulse+i] = sgn[loc[l][i]];
	  }
       }


   free( pst );
   free( bft );
   free( buf_bf2ps );
   free( dest );
   free( fai );
   free( sgn );

   for ( i = 0; i < num_prune; i++ ) {
      free( loc[i] );
      free( loc2[i] );
   }

   /*------ Combination Search of Gain and Exc. -----*/
   nec_bws_gain_enc(vu_flag,pul_loc, pul_amp, num_pulse, I_part, cand_gain,
		    g_ac, g_ec, g_mp8,qxnorm, z, ac, syn_ac,
		    syn_mp8,alpha, g_den, g_num,
		    lpc_order, len_sf, gainbit, ga_idx );

   if((tmp_exc = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }

   for (i = 0; i < len_sf; i++) tmp_exc[i] = 0.0;
   for (i = 0; i < num_pulse; i++) tmp_exc[pul_loc[i]] = pul_amp[i];
   nec_comb_filt(tmp_exc, comb_exc, len_sf, I_part, vu_flag);
   free( tmp_exc );

   /*------- Set Indices --------*/
   *pos_idx = 0;
   *sgn_idx = 0;
   for (i = 0; i < num_pulse; i++) {
      *pos_idx = (*pos_idx) << bit_pos[i];
      for (j = 0; j < num_pos[i]; j++) {
	 if (chn_pos[i*len_sf+j] == pul_loc[i])
	    break;
      }
      *pos_idx += j;
      *sgn_idx = (*sgn_idx) << 1;
      if ( pul_amp[i] < 0.0 ) *sgn_idx += 1;
   }
   free( syn_mp8 );
   free( pul_amp );
   free( pul_loc );
   free( bit_pos );
   free( num_pos );
   free( chn_pos );

}

