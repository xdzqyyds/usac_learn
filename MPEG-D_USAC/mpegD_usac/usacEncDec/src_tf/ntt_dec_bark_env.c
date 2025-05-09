/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18                                       */
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

/* 18-apr-97  NI  merged long, medium, & short procedure into single module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#define ntt_PF_DENSITY 1.5

void ntt_dec_bark_env(/* Parameters */
		      int    nfr,
		      int    nsf,
		      int    n_ch,
		      double *codebook,
		      int    ndiv,
		      int    cv_len,
		      int    cv_len_max,
		      int    *bark_tbl,
		      int    n_crb,
		      double alf_step,
		      int    prev_fw_code[],
		      /* Input */
		      int    index_fw[],
		      int    index_fw_alf[],
		      int    pf_switch,
		      /* Output */
		      double bark_env[])
{
  /*--- Variables ---*/
  int    i_ch, top, subtop, idtop, block_size;
  int    ismp, isf;
  int    ib;
  double env[ntt_N_CRB_MAX], penv[ntt_N_CRB_MAX], alfq, dtmp;
  
  /*--- Initialization ---*/
  block_size = nfr * nsf;
  
  /*--- Decoding process ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * block_size;
    for (isf=0; isf<nsf; isf++){
      subtop = isf * nfr + top;
      idtop  = i_ch * nsf + isf;
      /* Initialization 
      penv = penv_tmp + i_ch*n_crb; */
      
      /* Vector excitation */
      ntt_fwex(prev_fw_code+idtop*ndiv,
	       ndiv, cv_len, codebook, cv_len_max, penv);
      ntt_fwex(index_fw+idtop*ndiv,
	       ndiv, cv_len, codebook, cv_len_max, env);
      for (ib=ndiv*cv_len; ib<n_crb; ib++) env[ib] = 0.;
      
      /* Reconstruction */
      alfq = (double)index_fw_alf[idtop] * alf_step;
      for (ib=0, ismp=0; ib<n_crb; ib++){
	dtmp = ntt_max(env[ib]+alfq*penv[ib]+1., ntt_FW_LLIM);
	/*
          printf("%5d FFF %8.4f ", ib, dtmp);
	*/
	if ((pf_switch==1) && (dtmp<1.)){
	  dtmp = dtmp*dtmp; /*pow(dtmp, ntt_PF_DENSITY)*/;
	}
	/*
          printf(" %8.4f \n", dtmp);
	*/
	for (; ismp<bark_tbl[ib]; ismp++){
	  bark_env[ismp+subtop] = dtmp;
	}
      }
      /* Prediction memory operation 
      ntt_movdd(n_crb, env, penv); */
      
    }
  }
}
