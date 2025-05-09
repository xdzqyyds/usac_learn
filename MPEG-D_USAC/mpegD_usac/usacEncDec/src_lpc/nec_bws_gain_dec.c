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
 *	Excitation Gain Decoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_gain_wb_4m4b1d7b2d_nl32.tbl"
#include "nec_exc_proto.h"
#ifdef VERSION2_EC
#include "err_concealment.h"
#endif

#define NEC_GAIN_A_BIT	4
#define NEC_GAIN_B_BIT	7
#define NEC_GAIN_DIM	2

void nec_bws_gain_dec(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	ac[],		/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec,		/* output */
		  float	*g_mp8 )		/* output */
{
   int		i;
   long		size_gc, bit1, bit2;
   long         qga_idx, qgc_idx;
   float	renorm;
   float	vnorm1, vnorm2, ivnorm1, ivnorm2;
   float	*par, parpar0;
#ifdef VERSION2_EC	/* SC: 1, EC: GAIN, BWS */
   float	gwei[4] = {(float)4096/8192,
                           (float)8192/8192,
                           (float)12288/8192,
                           (float)16384/8192};
   static long	icount_err = 0;
   static float g_ec_p = 1.0, g_ac_p = 1.0;
   unsigned long signal_mode;
#endif

   /* Cofiguration Parameter Check */
   if ( gainbit != NEC_GAIN_B_BIT + NEC_GAIN_A_BIT ) {
      printf("\n Configuration error in nec_dec_gain16 \n");
      exit(1);
   }

   bit1 = NEC_GAIN_A_BIT;
   bit2 = NEC_GAIN_B_BIT;
   size_gc = 1 << bit2;

   qga_idx = ga_idx >> bit2;
   qgc_idx = ga_idx - (qga_idx << bit2);

   if((par = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_dec_gain \n");
      exit(1);
   }

   nec_lpc2par(alpha, par, lpc_order);
   parpar0 = 1.0;
   for(i = 0; i < lpc_order; i++){
      parpar0 *= (1 - par[i] * par[i]);
   }
   parpar0 = (parpar0 > 0.0) ? sqrt(parpar0) : 0.0;
   renorm = qxnorm * parpar0;

   vnorm1 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm1 += ac[i] * ac[i];
   }
   vnorm2 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm2 += comb_exc[i] * comb_exc[i];
   }

   if(vnorm1 == 0.0) {
      ivnorm1 = 0.0;
   }
   else {
      ivnorm1 = 1.0 / sqrt(vnorm1);
   }
   if(vnorm2 == 0.0) {
      ivnorm2 = 0.0;
   }
   else {
      ivnorm2 = 1.0 / sqrt(vnorm2);
   }

   *g_ac = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+0] * renorm * ivnorm1;
   *g_mp8 = nec_gc_sq[vu_flag][qga_idx];
   *g_ec  = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+1] * renorm * ivnorm2;
#ifdef VERSION2_EC	/* SC: 1, EC: GAIN, BWS */
   /* applied by NEC 990507 */
      if( (getErrState() == 0) || (getErrState() > 4) )
         icount_err = 0;
      else
         icount_err++;

   if( getErrAction() & EA_GAIN_PREVIOUS ){
    if( errConGetScModePre() == 1 ){
#if FLG_BER_or_FER
      /* FER mode */
      float	gcba, gcbe, wei_a, wei_e;

      errConLoadMode( &signal_mode );
      if( (signal_mode == 0) || (signal_mode == 1) ){
         gcba = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+0];
	 gcbe = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+1];
	 if( (gcbe != 0.0) && (gcba != 0.0) ){
            wei_a = gwei[signal_mode];
	    wei_e = 1.0 + (1.0-wei_a*wei_a)*(gcba*gcba)/(gcbe*gcbe) ;
	    if( wei_e  < 0.0 ){
               wei_e = 0.0;
	       wei_a = sqrt( 1.0 + (gcbe*gcbe)/(gcba*gcba) );
            } else
               wei_e = sqrt( wei_e );
            *g_ac *= wei_a;
	    *g_ec *= wei_e;
         }
      } else if( (signal_mode == 2) || (signal_mode == 3) ){
         *g_ac = (float)7782/8192;
	 if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = ((float)409/8192) * sqrt( vnorm1/vnorm2 );
         if( icount_err*len_sf <= 320 ){
            *g_ac *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
            *g_ec *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, ((float)-9830/819)/20.0); /* -1.2dB/5ms */
            *g_ec *= pow(10.0, ((float)-9830/819)/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error in nec_bws_gain_dec \n");
         exit( 1 );
      }

#else /* FLG_BER_or_FER */

      errConLoadMode( &signal_mode );
      if( (signal_mode == 0) || (signal_mode == 1) ){
         if( *g_ac > (float)10312/8192 ){
            *g_ec *= ((float)10312/8192) / (*g_ac);
	    *g_ac = (float)10312/8192;       	/* < +2.0dB */
         }
	 if( *g_ec > ((float)10312/8192) * g_ec_p ){
            *g_ac *= ((float)10312/8192) * g_ec_p / (*g_ec);
	    *g_ec = ((float)10312/8192) * g_ec_p;	/* < +2.0dB */
         }
         if( (signal_mode == 1) && (g_ac_p < (float)10312/8192)
				&& (*g_ac < ((float)6506/8192) * g_ac_p) ){
            *g_ac = ((float)6506/8192) * g_ac_p;
            *g_ec = ((float)6506/8192) * g_ec_p;
         }
      } else if( (signal_mode == 2) || (signal_mode == 3) ){
         *g_ac = (float)7782/8192;
         if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = 0.05 * sqrt( vnorm1/vnorm2 );
         if( icount_err * len_sf <= 320 ){
            *g_ac *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
	    *g_ec *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, ((float)-9830/8192)/20.0); /* -1.2dB/5ms */
	    *g_ec *= pow(10.0, ((float)-9830/8192)/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error in nec_bws_gain_dec \n");
         exit( 1 );
      }

#endif /* FLG_BER_or_FER */
    } else{ /* errConGetScModePre() != 1 */
      float	gcba, gcbe, wei_a, wei_e;

      errConLoadMode( &signal_mode );
      if( (signal_mode == 0) || (signal_mode == 1) ){
         gcba = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+0];
	 gcbe = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+1];
	 if( (gcbe != 0.0) && (gcba != 0.0) ){
            wei_a = gwei[signal_mode];
	    wei_e = 1.0 + (1.0-wei_a*wei_a)*(gcba*gcba)/(gcbe*gcbe) ;
	    if( wei_e  < 0.0 ){
               wei_e = 0.0;
	       wei_a = sqrt( 1.0 + (gcbe*gcbe)/(gcba*gcba) );
            } else
               wei_e = sqrt( wei_e );
            *g_ac *= wei_a;
	    *g_ec *= wei_e;
         }
      } else if( (signal_mode == 2) || (signal_mode == 3) ){
         *g_ac = (float)7782/8192;
	 if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = ((float)409/8192) * sqrt( vnorm1/vnorm2 );
         if( icount_err*len_sf <= 320 ){
            *g_ac *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
            *g_ec *= pow(10.0, ((float)-3276/8192)/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, ((float)-9830/8192)/20.0); /* -1.2dB/5ms */
            *g_ec *= pow(10.0, ((float)-9830/8192)/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error in nec_bws_gain_dec \n");
         exit( 1 );
      }
      *g_ac = 0.0;
    }
   }

   {
      static long isub = 0;

      if( 1 <= getErrState() && getErrState() <= 4 )
         isub = 0;
      else{
         if( getErrState() == 5 ){
            if( isub == 0 )
               isub = 1;
	 }
         if( (0 < isub * len_sf) && (isub * len_sf <= 80) ){
            if( *g_ac > 1.0 ){
               *g_ec *= 1.0 / (*g_ac);
	       *g_ac = 1.0;
            }
         } else if( (80 < isub * len_sf) && (isub * len_sf <= 160) ){
            if( *g_ac > 1.0 ){
               *g_ec *= ((*g_ac + 1.0) / 2.0) / (*g_ac);
	       *g_ac = (*g_ac + 1.0) / 2.0;
            }
         }
         if( isub > 0 )
            isub++;
         if( isub > 4 )
            isub = 0;
      }
      g_ac_p = (*g_ac);
      g_ec_p = (*g_ec);
   }
#endif

   free( par );
}

