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
 *	Adaptive CB Encoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_abs_const.h"
#include "nec_exc_proto.h"

void nec_bws_acb_enc(
		 float	xw[],		/* input */
		 float	*og_ac,		/* output */
		 float	ac[],		/* output */
		 float	syn_ac[],	/* output */
		 long	op_idx,		/* configuration input */
		 long	st_idx,		/* configuration input */
		 long	ed_idx,		/* configuration input */
		 long	*ac_idx_opt,	/* output */
		 long	lpc_order,	/* configuration input */
		 long	len_sf,		/* configuration input */
		 long	lagbit,		/* configuration input */
		 float	alpha[],	/* input */
		 float	g_den[],	/* input */
		 float	g_num[],	/* input */
		 float	mem_past_exc[],	/* input */
		 long	*int_part	/* output */
)
{
   long		i, idx;
   float	*syn, *exc, *zero;
   float	cWc, cWx;
   float	cWx_opt, cWxcWx_opt, cWc_opt;
   float	cWxcWx;
   float	*mem_ac;
   long		I_part;
   long         acb_bit, pitch_min, pitch_max, pitch_limit;

   /* Cofiguration Parameter Check */
   acb_bit = NEC_ACB_BIT_FRQ16;
   pitch_min = NEC_PITCH_MIN_FRQ16;
   pitch_max = NEC_PITCH_MAX_FRQ16;
   pitch_limit = NEC_PITCH_LIMIT_FRQ16;

   if ( lagbit != acb_bit ) {
      printf("\n Configuration error in nec_bws_acb_enc %ld %ld\n",lagbit,acb_bit);
      exit(1);
   }

   /*------ Memory Allocation ----------*/
   if ((syn = (float *)calloc(len_sf, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_enc_acb \n");
      exit(1);
   }
   if ((exc = (float *)calloc(len_sf, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_enc_acb \n");
      exit(1);
   }
   if ((zero = (float *)calloc(len_sf, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_enc_acb \n");
      exit(1);
   }
   if ((mem_ac =(float *)calloc(pitch_max+NEC_PITCH_IFTAP16+1+len_sf, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_enc_acb \n");
      exit(1);
   }

   /*--- DELAY SEARCH ---*/
   for ( i = 0; i < pitch_max + NEC_PITCH_IFTAP16+1; i++)
      mem_ac[i] = mem_past_exc[i];
   for ( i = 0; i < len_sf; i++ ) zero[i] = 0.0;

   *ac_idx_opt = op_idx;
   *int_part = nec_acb_generation_16( *ac_idx_opt, len_sf, mem_ac,
				     zero, ac, 1.0, 0 );
   nec_zero_filt(ac, syn_ac, alpha, g_den, g_num, lpc_order, len_sf);
   cWc_opt = 0.0;
   cWx_opt = 0.0;
   for (i = 0; i < len_sf; i++) {
      cWc_opt += syn_ac[i] * syn_ac[i];
      cWx_opt += syn_ac[i] * xw[i];
   }
   cWxcWx_opt = cWx_opt * cWx_opt;

   for (idx = st_idx; idx <= ed_idx; idx++) {
      I_part = nec_acb_generation_16( idx, len_sf, mem_ac, zero, exc, 1.0, 0);
      nec_zero_filt(exc, syn, alpha, g_den, g_num, lpc_order, len_sf);

      cWc = 0.0;
      cWx = 0.0;
      for (i = 0; i < len_sf; i++) {
	 cWc += syn[i] * syn[i];
	 cWx += syn[i] * xw[i];
      }
      cWxcWx = cWx * cWx;

      if (cWxcWx * cWc_opt < cWxcWx_opt * cWc || cWx <= 0.0 ) continue;
      cWxcWx_opt = cWxcWx;
      cWx_opt = cWx;
      cWc_opt = cWc;
      *ac_idx_opt = idx;
      *int_part = I_part;
      for (i = 0; i < len_sf; i++) {
	 ac[i] = exc[i];
	 syn_ac[i] = syn[i];
      }
   }
   *og_ac = (float)(cWc_opt != 0.0 ? cWx_opt/cWc_opt : 0.0);
   if ( *og_ac < 0.0 ) *og_ac = (float)0.0;
   if ( *og_ac > 1.2 ) *og_ac = (float)1.2;
   
   free( syn );
   free( exc );
   free( zero );
   free( mem_ac );
}
