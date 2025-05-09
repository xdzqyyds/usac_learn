/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/* and edited by                                                             */
/*   Mineo Tsushima,(Matsushita) on 1998-02-20,                              */
/*   Takehito Moriya,(NTT)       on 1998-05-01,                              */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-23                           */
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


#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"

void mat_scale_tf_requantize_spectrum(/* Input */
				      ntt_INDEX *indexp,
				      int mat_shift[],
				      /* Output */
				      double flat_spectrum[],
				      /* scalable layer ID */
				      int iscl)
{
  int    vq_bits = 0, n_sf = 0, cb_len_max = 0, nfr = 0, isf, subtop, subtopq;
  double *sp_cv0 = NULL, *sp_cv1 = NULL, spec_buf[ntt_T_FR_MAX];
  int qsample[ntt_N_SUP_MAX], bias[ntt_N_SUP_MAX];
  int i_ch;

  /*--- Parameter settings ---*/
  switch(indexp->w_type){
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* available bits */
    vq_bits = indexp->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
    /* codebooks */
    sp_cv0 = indexp->nttDataScl->ntt_codev0_scl;
    sp_cv1 = indexp->nttDataScl->ntt_codev1_scl;
    cb_len_max = ntt_CB_LEN_READ_SCL + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = indexp->numChannel;
    nfr = indexp->block_size_samples;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* available bits */
    vq_bits = indexp->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
    /* codebooks */
    sp_cv0 = indexp->nttDataScl->ntt_codev0s_scl;
    sp_cv1 = indexp->nttDataScl->ntt_codev1s_scl;
    cb_len_max = ntt_CB_LEN_READ_S_SCL + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = indexp->numChannel * ntt_N_SHRT;
    nfr = indexp->block_size_samples/8;
    break;
  default:
    CommonExit (1, "ntt_scale_tf_requantize_spectrum(): %d: No such window type.",
                indexp->w_type);
  }

  /*--- Reconstruction ---*/
     ntt_zerod(indexp->block_size_samples*indexp->numChannel,spec_buf);  /* Tsushima */
     for(i_ch = 0; i_ch<indexp->numChannel; i_ch++){
	float tmp;
	tmp =
	 (float)(indexp->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]])
	-(float)(indexp->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]);
	tmp *= (float)nfr;
	qsample[i_ch]=(int)tmp;
	tmp = 
	(float)(indexp->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]);
        tmp *= (float)nfr;
	bias[i_ch] =  (int)tmp;
     }
     ntt_vex_pn(indexp->wvq,
	     sp_cv0, sp_cv1, cb_len_max,
	     n_sf, 
	     qsample[0]*n_sf,
	     vq_bits,
	     flat_spectrum);
      for(isf=0;isf<n_sf;isf++){
        subtop= isf*nfr;
	i_ch = subtop/indexp->block_size_samples;
        subtopq= isf*qsample[i_ch];
        ntt_movdd(qsample[i_ch],flat_spectrum+subtopq,
				spec_buf+subtop+bias[i_ch]);
      }
      ntt_movdd(indexp->block_size_samples*indexp->numChannel,spec_buf,flat_spectrum); /* Tsushima */
}
