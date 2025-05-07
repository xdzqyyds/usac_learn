/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
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

/* 18-apr-97  NI  merged long, medium, & short procedure into single module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"

void ntt_tf_requantize_spectrum(/* Input */
				ntt_INDEX *indexp,
				/* Output */
				double flat_spectrum[])
{
  int    vq_bits=0, n_sf=0, cb_len_max=0;
  double *sp_cv0=NULL, *sp_cv1=NULL;
  int    nfr, isf, jsf, data_len; 
  float  ftmp;
  /*--- Parameter settings ---*/
  switch(indexp->w_type){
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* available bits */
    vq_bits = indexp->nttDataBase->ntt_VQTOOL_BITS;
    /* codebooks */
    sp_cv0 = indexp->nttDataBase->ntt_codev0; 
    sp_cv1 = indexp->nttDataBase->ntt_codev1;
    cb_len_max = ntt_CB_LEN_READ + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = indexp->numChannel;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* available bits */
    vq_bits = indexp->nttDataBase->ntt_VQTOOL_BITS_S;
    /* codebooks */
    sp_cv0 = indexp->nttDataBase->ntt_codev0s; 
    sp_cv1 = indexp->nttDataBase->ntt_codev1s;
    cb_len_max = ntt_CB_LEN_READ_S + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = indexp->numChannel * ntt_N_SHRT;
    break;
  default:
    CommonExit ( 1, "ntt_tf_requantize_spectrum(): %d: No such window type.\n",
	    indexp->w_type);
  }
  
  /*--- Reconstruction ---*/
  data_len =indexp->block_size_samples*indexp->numChannel/n_sf;
  ftmp = (float)(indexp->nttDataBase->bandUpper);
  ftmp *= (float)data_len;
  data_len = (int)ftmp;
  data_len *= n_sf;
  ntt_vex_pn(indexp->wvq,
	     sp_cv0, sp_cv1, cb_len_max,
	     n_sf, data_len, vq_bits, flat_spectrum);
    nfr = indexp->block_size_samples*indexp->numChannel/n_sf;
    ftmp = (float)(indexp->nttDataBase->bandUpper);
    ftmp *= (float)nfr;
    for(isf=n_sf-1; isf>0; isf--){
       for(jsf=(int)ftmp-1; jsf>=0; jsf--){
	  flat_spectrum[nfr*isf+jsf] = 
	  flat_spectrum[(int)ftmp*isf+jsf];
       }
    }
    for(isf=0; isf<n_sf; isf++){
       ntt_zerod(nfr-(int)ftmp, flat_spectrum+nfr*isf+(int)ftmp);
    }
}
