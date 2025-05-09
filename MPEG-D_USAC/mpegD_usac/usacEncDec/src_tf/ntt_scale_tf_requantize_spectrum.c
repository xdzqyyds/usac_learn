/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT)                                                          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
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
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"


void ntt_scale_tf_requantize_spectrum(/* Input */
				      ntt_INDEX *ntt_index_scl,
				      /* Output */
				      double flat_spectrum[],
				      /* scalable layer ID */
				      int iscl)
{
  int    vq_bits = 0, n_sf = 0, cb_len_max = 0;
  double *sp_cv0 = NULL, *sp_cv1 = NULL;
  
  /*--- Parameter settings ---*/
  switch(ntt_index_scl->w_type){
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* available bits */
    vq_bits = ntt_index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
    /* codebooks */
    sp_cv0 = (double *)ntt_codev0_scl[iscl];
    sp_cv1 = (double *)ntt_codev1_scl[iscl];
    cb_len_max = ntt_CB_LEN_READ_SCL[iscl] + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = ntt_index_scl->numChannel;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* available bits */
    vq_bits = ntt_index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
    /* codebooks */
    sp_cv0 = (double *)ntt_codev0s_scl[iscl];
    sp_cv1 = (double *)ntt_codev1s_scl[iscl];
    cb_len_max = ntt_CB_LEN_READ_S_SCL[iscl] + ntt_CB_LEN_MGN;
    /* number of subframes in a frame */
    n_sf = ntt_index_scl->numChannel * ntt_N_SHRT;
    break;
  default:
    CommonExit ( 1, "ntt_scale_tf_requantize_spectrum(): %d: No such window type.\n",
	    ntt_index_scl->w_type);
  }
  
  /*--- Reconstruction ---*/
  ntt_vex_pn(ntt_index_scl->wvq,
	     sp_cv0, sp_cv1, cb_len_max,
	     n_sf, 
	     (ntt_index_scl->block_size_samples)*ntt_index_scl->numChannel,
	     vq_bits,
	     flat_spectrum);
}
