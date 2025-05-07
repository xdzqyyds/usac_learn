/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-07-17                                       */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-11                           */
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

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_scale_encode.h"

extern	Info		eight_short_info;
extern	Info		only_long_info;
void ntt_scale_vq_coder(double      *spectral_line_vector[MAX_TIME_CHANNELS],
			double      lpc_spectrum[],
			ntt_INDEX   *index,
			ntt_INDEX   *index_scl,
			ntt_PARAM   *param_ntt,
			int         mat_shift[ntt_N_SUP_MAX],
			int         sfb_width_table
					[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
		        int         nr_of_sfb[MAX_TIME_CHANNELS],
			int         available_bits,
			double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
			int         iscl)
{
  /*--- Variables ---*/
  int ismp, i_ch;
  long top;
  ntt_PARAM param_ntt2;
  double spectrum2[ntt_T_FR_MAX],
         mat_spectrum2[ntt_T_FR_MAX];
  double lpc_spectrum2[ntt_T_FR_MAX];
  double bark_env[ntt_T_FR_MAX];
  double pitch_sequence[ntt_T_FR_MAX];
  double gain[ntt_T_SHRT_MAX];
  double perceptual_weight[ntt_T_FR_MAX];
  double spectrum_gain = 0;
  double lpc_spectrum_out[ntt_T_FR_MAX];

  int sb;
  int nfr = 0;
  Info   *sfbInfo[4], *p_sfbInfo;

  /*--- set parameters ---*/
  index_scl->w_type = index->w_type;
  index_scl->pf = index->pf;
  switch(index_scl->w_type){
    /* long */
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl] = 
       available_bits - ntt_NMTOOL_BITS_SCLperCH*index->numChannel;
    nfr = index->block_size_samples;
    spectrum_gain = 1./sqrt((double)index->block_size_samples*2.); /* spec. norm. gain */
    break;
    /* short */
  case EIGHT_SHORT_SEQUENCE:
    index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl] = 
       available_bits - ntt_NMTOOL_BITS_S_SCLperCH*index->numChannel;
    nfr = index->block_size_samples/8;
    spectrum_gain = 1./sqrt((double)index->block_size_samples/8*2.); /* spec. norm. gain */
    break;
  default:
    fprintf(stderr, "ntt_scale_vq_coder(): Error. %d: No such window type.\n",index_scl->w_type);
    exit(1);
    break;
  }

/*if(index_scl->ms_mask ==1)*/
 {
  if(index_scl->w_type == EIGHT_SHORT_SEQUENCE){
     p_sfbInfo = &eight_short_info;
     p_sfbInfo->nsbk = 8;
     p_sfbInfo->islong = 0;
     for(ismp=0; ismp<8; ismp++){
       p_sfbInfo->bins_per_sbk[ismp] = index->block_size_samples/8;
       p_sfbInfo->sfb_per_sbk[ismp]= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index_scl->max_sfb[iscl+1] /*p_sfbInfo->sfb_per_sbk[ismp]*/; sb++){
         /*
	 p_sfbInfo->sfb_width_short[sb] = sfb_width_table[0][sb];
         */
	 top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = (short)top;
       }
     }
  
  }
  else{
     p_sfbInfo = &only_long_info;
     p_sfbInfo->nsbk = 1;
     p_sfbInfo->islong = 1;
     p_sfbInfo->bins_per_sbk[0] = index->block_size_samples;
       p_sfbInfo->sfb_per_sbk[0]= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index_scl->max_sfb[iscl+1] /*p_sfbInfo->sfb_per_sbk[0]*/; sb++){
         top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = (short)top;
       }
  }

  sfbInfo[index_scl->w_type]=p_sfbInfo;
 }


  /*--- Hand data to scalable coder ---*/
  param_ntt2.speech_sw = param_ntt->speech_sw;
  param_ntt2.fine_sw   = param_ntt->fine_sw;

  /*--- Scalable coder ---*/
  /* make input coefficients */
  for (i_ch =0 ; i_ch < index->numChannel ; i_ch++){
       top = i_ch * index->block_size_samples ;
       for (ismp=0; ismp<index->block_size_samples; ismp++){
	    spectrum2[ismp+top] =  spectrum_gain * 
		      (spectral_line_vector[i_ch][ismp] - reconstructed_spectrum[i_ch][ismp]);
	    mat_spectrum2[ismp+top] = spectrum2[ismp+top] ;
       }
  }

  if(index->numChannel==2){
  int sb, win;
  int sfbw;
  double buf[ntt_N_FR_MAX];
/*
if(index_scl->ms_mask ==1)*/
   {
     for( win=0; win<p_sfbInfo->nsbk; win++ ) {
     int sboffs   = win*nfr;

     for( sb=0; sb<index_scl->max_sfb[iscl+1] /*nr_of_sfb[MONO_CHAN]*/; sb++ ) {
           sfbw = sfb_width_table[MONO_CHAN][sb];
       if( index_scl->msMask[win][sb] != 0  ) {
         for (ismp=sboffs; ismp<sboffs+sfbw; ismp++){
           buf[ismp] =  (spectrum2[ismp] - spectrum2[ismp+index->block_size_samples])/2.;
           spectrum2[ismp] =  (spectrum2[ismp]+spectrum2[ismp+index->block_size_samples])/2.;
           spectrum2[ismp+index->block_size_samples]  = buf[ismp];
         }
       }
       sboffs += sfbw;
      }
     }
   }
/*
  else if(index_scl->ms_mask ==2){
        for (ismp=0; ismp<index->block_size_samples; ismp++){
              buf[ismp] =  (spectrum2[ismp] - spectrum2[ismp+index->block_size_samples])/2.;
              spectrum2[ismp] =(spectrum2[ismp]+spectrum2[ismp+index->block_size_samples])/2.;
              spectrum2[ismp+index->block_size_samples]  = buf[ismp];
        }
    }
*/
  }
  
  for (i_ch =0 ; i_ch < index->numChannel ; i_ch++){
       top = i_ch * index->block_size_samples ;
       for (ismp=0; ismp<index->block_size_samples; ismp++){
	    mat_spectrum2[ismp+top] = spectrum2[ismp+top] ;
       }
  }


 /* Base,Enh-1 : fix   Enh-2... : flex */ 
  mat_scale_lay_shift2(mat_spectrum2,&(*spectral_line_vector[0]),
			   iscl,index_scl, mat_shift);
       /*NN moved to ntt_scale_tf_proc_spectrum
       mat_scale_set_shift_para2(iscl);
  */
  /* spectrum normalization tool */
  ntt_scale_tf_proc_spectrum(spectrum2, index_scl,
                               mat_shift,
                               index->nttDataBase->cos_TT,
			       lpc_spectrum2, bark_env,
                               pitch_sequence, gain, iscl,
                               lpc_spectrum, lpc_spectrum_out);

  /* calculate perceptual weight */
  ntt_scale_tf_perceptual_model(lpc_spectrum2, bark_env, gain,
                                index_scl->w_type,
                                spectrum2, pitch_sequence,
                                index->numChannel,
				index->block_size_samples,
				perceptual_weight);
                  
  /* VQ tool */
  mat_scale_tf_quantize_spectrum(spectrum2, lpc_spectrum2, bark_env,
                                   pitch_sequence,
                                   gain, perceptual_weight, index_scl,
                                   mat_shift, iscl);
  /* local decoding */
  ntt_scale_vq_decoder(index_scl, index, mat_shift,
                       reconstructed_spectrum, iscl, sfbInfo);
}
