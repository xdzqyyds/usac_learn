/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT)                                                          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/

#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"

void ntt_scale_vq_decoder(ntt_INDEX  *indexp,
			  ntt_INDEX  *index_base,
			  int    mat_shift[],
			  double *spectral_line_vector[MAX_TIME_CHANNELS],
			  int    iscl,
                          Info   **srbInfo)
{
  static double flat_spectrum[ntt_T_FR_MAX], spectrum[ntt_T_FR_MAX];
  int    ismp, i_sup, top;
  double spectrum_gain;
  Info *p_sfbInfo;

  /*--- Requantize flattened MDCT coefficients ---*/
  mat_scale_tf_requantize_spectrum(indexp, mat_shift, flat_spectrum, iscl);

  /*--- Decode side information */
  ntt_scale_tf_proc_spectrum_d(indexp, mat_shift, index_base->nttDataBase->cos_TT,
			       flat_spectrum, spectrum, iscl);

  /*--- Spectrum line normalization
    (for gain alignment of NTT & MPEG4 VM MDCT ---*/
  switch(indexp->w_type){
  case EIGHT_SHORT_SEQUENCE:
    spectrum_gain = sqrt((double)indexp->block_size_samples/8*2.);
    break;
  default: /* long */
    spectrum_gain = sqrt((double)indexp->block_size_samples*2.);
  }

  if(indexp->numChannel==2){
   int sb, win;
   int sfbw;
   double buf[ntt_N_FR_MAX];
 {
    p_sfbInfo = srbInfo[indexp->w_type];
    for( win=0; win<p_sfbInfo->nsbk; win++ ) {
     int sboffs   = win*p_sfbInfo->bins_per_sbk[win];

     for( sb=0; sb<indexp->max_sfb[iscl+1] /*p_sfbInfo->sfb_per_sbk[win]*/; sb++ ) {
       {
         if (sb==0)
           sfbw = p_sfbInfo->bk_sfb_top[sb];
         else
           sfbw = p_sfbInfo->bk_sfb_top[sb]-p_sfbInfo->bk_sfb_top[sb-1];
       }
       if( indexp->msMask[win][sb] != 0  ) {
         for (ismp=sboffs; ismp<sboffs+sfbw; ismp++){
           buf[ismp] =  spectrum[ismp] - spectrum[ismp+indexp->block_size_samples];
           spectrum[ismp] =  spectrum[ismp]+spectrum[ismp+indexp->block_size_samples];
           spectrum[ismp+indexp->block_size_samples]  = buf[ismp];
         }
       }

       sboffs += sfbw;
     }
    }
   }
  }
  for (i_sup=0; i_sup<indexp->numChannel; i_sup++){
    top = i_sup * indexp->block_size_samples;
    for (ismp=0; ismp<indexp->block_size_samples; ismp++){
      spectral_line_vector[i_sup][ismp] += spectrum[ismp+top]*spectrum_gain;
    }
  }

}
