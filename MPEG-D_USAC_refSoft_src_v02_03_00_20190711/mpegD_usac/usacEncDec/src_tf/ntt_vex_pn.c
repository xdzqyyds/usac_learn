/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 18-apr-97   NI   generalized the module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void ntt_vex_pn(int    *index,    /* Input : VQ indices */
		double *sp_cv0,    /* Input : shape codebook (conj. ch. 0) */
		double *sp_cv1,    /* Input : shape codebook (conj. ch. 1) */
		int    cv_len_max, /* Input : memory length of codevectors */
		int    n_sf,       /* Input : number of subframes in a frame */
		int    block_size, /* Input : total block size */
		int    available_bits,
		                   /* Input : available bits */
		double *sig)       /* Output: Reconstructed coefficients */
{
  /*--- Variables ---*/
  int    idiv, n_div, icv, ismp, itmp;
  int    pol0, pol1, index0, index1, mask;
  int    length, length0;
  
  /*--- Initialization ---*/
  mask = (0x1 << ntt_MAXBIT_SHAPE) -1;
  n_div = (int)((available_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  length0 = (int)((block_size + n_div - 1) / n_div);
  for ( idiv=0; idiv<n_div; idiv++ ){
    /*--- Index unpacking ---*/
    index0 = (index[idiv] ) & mask;
    index1 = (index[idiv+n_div]) & mask;
    pol0 = 1 - 2*((index[idiv] >> (ntt_MAXBIT_SHAPE)) & 0x1);
    pol1 = 1 - 2*((index[idiv+n_div]>>(ntt_MAXBIT_SHAPE)) & 0x1);
    /*--- Set length of the codevector ---*/
    length = (int)((block_size + n_div - 1 - idiv) / n_div);
    /*--- Reconstruction ---*/
    for (icv=0; icv<length; icv++){
      /* set interleave */
      if ((icv<length0-1) && 
	  ((n_div%n_sf==0 && n_sf>1) || ((n_div&0x1)==0 && (n_sf&0x1)==0)))
	itmp = ((idiv + icv) % n_div) + icv * n_div;
      else  itmp = idiv + icv * n_div;
      ismp = itmp / n_sf + ((itmp % n_sf) * (block_size / n_sf));
      /*
        printf("YYYYY %5d %5d %5d %5d %5d %12.2f \n", 
        length, ismp, index0*cv_len_max+icv,
        index1*cv_len_max+icv, cv_len_max, sp_cv0[index0*cv_len_max+icv]);
      */
      /*
        printf("YYYYY %5d %5d %5d %5d %12.2f \n", ismp, index0*cv_len_max+icv,
        index1*cv_len_max+icv, cv_len_max, sp_cv1[index1*cv_len_max+icv]);
      */
      /* reconstruction */
      sig[ismp] = 
	(pol0*sp_cv0[index0*cv_len_max+icv]
	 + pol1*sp_cv1[index1*cv_len_max+icv]) * 0.5;
    }
  }
}
