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
 *	Enhanced Excitation Gain Encoding Subroutines
 *
 *	Ver1.0	97.03.17	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_gain_4m4b1d.tbl"
#include "nec_exc_proto.h"

#define NEC_ENH_GAIN_BIT	4

void nec_enh_gain_enc(
		      long	vu_flag,	/* input */
		      long	pul_loc[],	/* input */
		      float	pul_amp[],	/* input */
		      long	num_pulse,	/* configuration input */
		      long	I_part,		/* input */
		      long	cand_gain,	/* configuration input */
		      float	*g_ac,		/* output */
		      float	*g_ec,		/* output */
		      float	qxnorm,		/* input */
		      float	z[],		/* input */
		      float	ac[],		/* input */
		      float	facx[],		/* input */
		      float	alpha[],	/* input */
		      float	g_den[],	/* input */
		      float	g_num[],	/* input */
		      long	lpc_order,	/* configuration input */
		      long	len_sf,		/* configuration input */
		      long	gainbit,	/* configuration input */
		      long	*ga_idx )	/* output */
{
   int		jg, i;
   int		cand_gain_ind;
   int		k, min_pulse;
   long		size_gc;
   float	Eg[1 << NEC_ENH_GAIN_BIT];
   float	*par, parpar0;
   float	renorm, vnorm1, vnorm2;
   float	ivnorm1, *ivnorm2;
   float	*fzsc1, *fsa1sc1, *fsc1sc1;
   float	nga, ngc;
   float	*fsc1, *exc1, *exc12;
   float	cand_er_gain;
   float	z_p;
   float	zsax, sasa;

   /* Cofiguration Parameter Check */
   if ( gainbit != NEC_ENH_GAIN_BIT ) {
      printf("\n Configuration error in nec_enc_gain \n");
      exit(1);
   }

   size_gc = 1 << gainbit;

   if((par = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((fsc1 = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((exc1 = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((exc12 = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((ivnorm2 = (float *)calloc (cand_gain, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((fzsc1 = (float *)calloc (cand_gain, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((fsa1sc1 = (float *)calloc (cand_gain, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }
   if((fsc1sc1 = (float *)calloc (cand_gain, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_enc_gain \n");
      exit(1);
   }

   /*--- <z,z> ---*/
   z_p = 0.0;
   for (i = 0; i < len_sf; i++) z_p += z[i] * z[i];
   /*--- <z,sa1> ---*/
   zsax = 0.0;
   for (i = 0; i < len_sf; i++) zsax += z[i] * facx[i];
   /*--- <sa1,sa1> ---*/
   sasa = 0.0;
   for (i = 0; i < len_sf; i++) sasa += facx[i] * facx[i];

   /*--- GAIN NORMARIZATION ---*/
   nec_lpc2par(alpha, par, lpc_order);
   parpar0 = 1.0;
   for (i = 0; i < lpc_order; i++){
      parpar0 *= (1 - par[i] * par[i]);
   }
   parpar0 = (float)((parpar0 > 0.0) ? sqrt(parpar0) : 0.0);
   renorm = qxnorm * parpar0;

   /*--- GAIN CODEVECTOR SEARCH ---*/
   cand_er_gain = (float)1.0e30;
   cand_gain_ind = -1;
   min_pulse = -1;
   *g_ac = 0.0;
   *g_ec = 0.0;
   for (k = 0; k < cand_gain; k++){
      for (i = 0; i < len_sf; i++) exc1[i] = 0.0;
      for (i = 0; i < num_pulse; i++)
	 exc1[pul_loc[k*num_pulse+i]] = pul_amp[k*num_pulse+i];

      nec_comb_filt(exc1, exc12, len_sf, I_part, vu_flag);

      /*--- syn_ec ---*/
      nec_zero_filt(exc12, fsc1, alpha, g_den, g_num, lpc_order, len_sf);
      /*--- <z,sc1> ---*/
      fzsc1[k] = 0.0;
      for (i = 0; i < len_sf; i++) fzsc1[k] += z[i] * fsc1[i];
      /*--- <sa1,sc1> ---*/
      fsa1sc1[k] = 0;
      for (i = 0; i < len_sf; i++) fsa1sc1[k] += facx[i] * fsc1[i];
      /*--- <sc1,sc1> ---*/
      fsc1sc1[k] = 0;
      for (i = 0; i < len_sf; i++) fsc1sc1[k] += fsc1[i] * fsc1[i];

      vnorm1 = 0.0;
      vnorm2 = 0.0;
      for (i = 0; i < len_sf; i++) {
	 vnorm1 += ac[i] * ac[i];
	 vnorm2 += exc12[i] * exc12[i];
      }
      ivnorm1 = (float)((vnorm1 == 0.0) ? 0.0 : 1.0 / sqrt(vnorm1));
      ivnorm2[k] = (float)((vnorm2 == 0.0) ? 0.0 : 1.0 / sqrt(vnorm2));

      for (jg = 0; jg < size_gc; jg++) {
	 nga = 1.0;
	 ngc = nec_egc[vu_flag][jg] * renorm * ivnorm2[k];
	
	 Eg[jg] = (float)(z_p
	    - 2.0 * nga * zsax
	       - 2.0 * ngc * fzsc1[k]
		  + 2.0 * nga * ngc * fsa1sc1[k]
		     + nga * nga * sasa
			+ ngc * ngc * fsc1sc1[k]);
	    
	 if (Eg[jg] < cand_er_gain && Eg[jg] >= 0.0 ) {
	    cand_er_gain = Eg[jg];
	    cand_gain_ind = jg;
	    min_pulse = k;
	    *g_ac = nga;
	    *g_ec  = ngc;
	 }
      }
   }

   if ( min_pulse == -1 || cand_gain_ind == -1 ) {
      *ga_idx = 0;
   } else {
      for(i = 0; i < num_pulse; i++){
	 pul_loc[i] = pul_loc[min_pulse*num_pulse+i];
	 pul_amp[i] = pul_amp[min_pulse*num_pulse+i];
      }
      *ga_idx = cand_gain_ind;
   }

   free( par );
   free( fsc1 );
   free( exc1 );
   free( exc12 );
   free( ivnorm2 );
   free( fzsc1 );
   free( fsa1sc1 );
   free( fsc1sc1 );

}
