/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
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
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 18-apr-97  NI  merged long, medium, & short procedure into single module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void ntt_dec_lpc_spectrum_inv
  (
   /* Parameters */
   int    nfr,
   int    block_size_samples,
   int    n_ch,
   int    n_pr,
   int    nsp,
   double *lsp_code,
   double *lsp_fgcode,
   int    *csize,
   int    prev_lsp_index[ntt_N_SUP_MAX*ntt_LSP_NIDX_MAX],
   /*
   double prev_buf[ntt_N_SUP_MAX][ntt_MA_NP][ntt_N_PR_MAX+1],
   */
   int    ma_np,
   /* Input */
   int    index_lsp[ntt_N_SUP_MAX][ntt_LSP_NIDX_MAX],
   /* Output */
   double inv_lpc_spec[],
   double *cos_TT
   )
{
  /*--- Variables ---*/
  double        lspq[ntt_N_SUP_MAX*(ntt_N_PR_MAX+1)];
  int           i_ch, top, lsptop;
  
  /*--- LSP decoding ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    lsptop = i_ch * (n_pr + 1);
    ntt_lsp_decw(n_pr, nsp, (double (*)[ntt_N_PR_MAX])lsp_code,
		 (double (*)[ntt_MA_NP][ntt_N_PR_MAX])lsp_fgcode,
		 csize, 
		 prev_lsp_index +i_ch*ntt_LSP_NIDX_MAX,
		 /*prev_buf[i_ch], */
		 ma_np, 
		 index_lsp[i_ch], lspq+lsptop);
  }
  
  /*--- Transforming LSP coefficients to the LPC spectrum ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    top    = i_ch * nfr;
    lsptop = i_ch * (n_pr + 1);
    ntt_lsptowt_int(nfr, block_size_samples,
		    n_pr, lspq+lsptop, inv_lpc_spec+top, cos_TT);
  }
}
