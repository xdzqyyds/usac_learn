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
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_gain_4m6b2d.tbl"
#include "nec_gain_wb_4m7b2d.tbl"
#include "nec_exc_proto.h"
#include "nec_abs_const.h"
#ifdef VERSION2_EC
#include "lpc_common.h"
#include "err_concealment.h"
#endif

#define NEC_GAIN_DIM	2

void nec_dec_gain(
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
		  float	*g_ec )		/* output */
{
   int		i;
   long		size_gc, n_cb;
   float	renorm;
   float	vnorm1, vnorm2, ivnorm1, ivnorm2;
   float	*par, parpar0;
   float	*gcb =NULL;
#ifdef VERSION2_EC	/* SC: 1, EC: GAIN, MPE */
   float	gwei[4] = {(float)4096/8192,
                           (float)8192/8192,
                           (float)12288/8192,
                           (float)16384/8192};
   unsigned long	signal_mode;
   static long	icount_err = 0;
   static float g_ec_p = 1.0, g_ac_p = 1.0;
#endif

   /* Cofiguration Parameter Check */
   n_cb = (len_sf == 40) ? 1 : 0;
   if ( gainbit == NEC_BIT_GAIN ) {
      gcb = nec_gc[4*n_cb+vu_flag];
   } else if ( gainbit == NEC_BIT_GAIN_WB ) {
      gcb = nec_wb_gc[4*n_cb+vu_flag];
   } else {
      printf("\n Configuration error in nec_enc_gain \n");
      exit(1);
   }
   size_gc = 1 << gainbit;

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
   vnorm2 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm1 += ac[i] * ac[i];
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

   *g_ac = gcb[NEC_GAIN_DIM*ga_idx+0] * renorm * ivnorm1;
   *g_ec = gcb[NEC_GAIN_DIM*ga_idx+1] * renorm * ivnorm2;

#ifdef VERSION2_EC	/* SC: 1, EC: GAIN, MPE */
   if( (getErrState() == 0) || (getErrState() > 4) )
      icount_err = 0;
   else
      icount_err++;

   if( getErrAction() & EA_GAIN_PREVIOUS ){
    if( errConGetScModePre() == 1 ){
#if FLG_BER_or_FER
      float	gcba, gcbe, wei_a, wei_e;

      errConLoadMode( &signal_mode );
      if( (signal_mode == 0) || (signal_mode == 1) ){
         gcba = gcb[NEC_GAIN_DIM*ga_idx+0];
	 gcbe = gcb[NEC_GAIN_DIM*ga_idx+1];
	 if( (gcbe != 0.0) && (gcba != 0.0) ){
            wei_a = gwei[signal_mode];
	    wei_e = 1.0 + (1.0 - wei_a * wei_a) * (gcba*gcba) / (gcbe*gcbe);
	    if( wei_e <0.0 ){
               wei_e = 0.0;
	       wei_a = sqrt( 1.0 + (gcbe*gcbe)/(gcba*gcba) );
            } else
               wei_e = sqrt( wei_e );
	    *g_ac *= wei_a;
	    *g_ec *= wei_e;
         }
      } else if( (signal_mode == 2) || (signal_mode == 3) ){
	 float dB1 = (float)-3276/8192;
	 float dB2 = (float)-9830/8192;

#ifdef	EC_SF_DEPEND
	 if( errConGetFsMode() == fs16kHz ){
	    if( len_sf <= EC_SF_DEPEND_SZ1 ){
               if( len_sf <= EC_SF_DEPEND_SZ2 ){
		  dB1 *= EC_SF_DEPEND_FAC2;
		  dB2 *= EC_SF_DEPEND_FAC2;
	       } else{
		  dB1 *= EC_SF_DEPEND_FAC1;
		  dB2 *= EC_SF_DEPEND_FAC1;
	       }
	    }
	 }
#endif
         *g_ac = (float)7782/8192;
         if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = ((float)409/8192) * sqrt( vnorm1/vnorm2 );
         if( icount_err * len_sf <= 320 ){
            *g_ac *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
	    *g_ec *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
	    *g_ec *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error (%d) in nec_dec_gain \n", (int)signal_mode );
         exit( 1 );
      }

#else /* FLG_BER_or_FER */

      errConLoadMode(&signal_mode);
      if( (signal_mode == 0) || (signal_mode == 1) ){
         if( *g_ac > (float)10312/8192 ){
            *g_ec *= ((float)10312/8192) /(*g_ac);
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
	 float dB1 = (float)-3276/8192;
	 float dB2 = (float)-9830/8192;

#ifdef	EC_SF_DEPEND
	 if( errConGetFsMode() == fs16kHz ){
	    if( len_sf <= EC_SF_DEPEND_SZ1 ){
               if( len_sf <= EC_SF_DEPEND_SZ2 ){
		  dB1 *= EC_SF_DEPEND_FAC2;
		  dB2 *= EC_SF_DEPEND_FAC2;
	       } else{
		  dB1 *= EC_SF_DEPEND_FAC1;
		  dB2 *= EC_SF_DEPEND_FAC1;
	       }
	    }
	 }
#endif
         *g_ac = (float)7782/8192;
	 if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = ((float)409/8192) * sqrt( vnorm1/vnorm2 );
         if( icount_err * len_sf <= 320 ){
            *g_ac *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
	    *g_ec *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
	    *g_ec *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error in nec_dec_gain \n");
         exit( 1 );
      }

#endif /* FLG_BER_or_FER */
    } else{ /* errConGetScModePre() != 1 */
      float	gcba, gcbe, wei_a, wei_e;

      errConLoadMode( &signal_mode );
      if( (signal_mode == 0) || (signal_mode == 1) ){
         gcba = gcb[NEC_GAIN_DIM*ga_idx+0];
	 gcbe = gcb[NEC_GAIN_DIM*ga_idx+1];
	 if( (gcbe != 0.0) && (gcba != 0.0) ){
            wei_a = gwei[signal_mode];
	    wei_e = 1.0 + (1.0 - wei_a * wei_a) * (gcba*gcba) / (gcbe*gcbe);
	    if( wei_e <0.0 ){
               wei_e = 0.0;
	       wei_a = sqrt( 1.0 + (gcbe*gcbe)/(gcba*gcba) );
            } else
               wei_e = sqrt( wei_e );
	    *g_ac *= wei_a;
	    *g_ec *= wei_e;
         }
      } else if( (signal_mode == 2) || (signal_mode == 3) ){
	 float dB1 = (float)-3276/8192;
	 float dB2 = (float)-9830/8192;

#ifdef	EC_SF_DEPEND
	 if( errConGetFsMode() == fs16kHz ){
	    if( len_sf <= EC_SF_DEPEND_SZ1 ){
               if( len_sf <= EC_SF_DEPEND_SZ2 ){
		  dB1 *= EC_SF_DEPEND_FAC2;
		  dB2 *= EC_SF_DEPEND_FAC2;
	       } else{
		  dB1 *= EC_SF_DEPEND_FAC1;
		  dB2 *= EC_SF_DEPEND_FAC1;
	       }
	    }
	 }
#endif
         *g_ac = (float)7782/8192;
         if( vnorm2 == 0.0 )
            *g_ec = 0.0;
         else
            *g_ec = ((float)409/8192) * sqrt( vnorm1/vnorm2 );
         if( icount_err * len_sf <= 320 ){
            *g_ac *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
	    *g_ec *= pow(10.0, dB1/20.0); /* -0.4dB/5ms */
         } else{
            *g_ac *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
	    *g_ec *= pow(10.0, dB2/20.0); /* -1.2dB/5ms */
         }
      } else{
         printf("\n Signal mode error (%d) in nec_dec_gain \n", (int)signal_mode );
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
            isub=0;
      }
      g_ac_p = (*g_ac);
      g_ec_p = (*g_ec);
   }
#endif
   free( par );
}

