/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT)      on 1997-10-23,                                      */
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
#include  "ntt_relsp.h"


void ntt_lsp_decw(/* Parameters */
		  int    n_pr,
		  int    nsp,
		  double code[][ntt_N_PR_MAX],
		  double fgcode[][ntt_MA_NP][ntt_N_PR_MAX],
		  int    *csize,
		  int    prev_lsp_code[],
		  /*
		  double buf_prev[ntt_MA_NP][ntt_N_PR_MAX+1],
		  */
		  int    ma_np,
		  /* Input */
		  int    index[],
		  /* Output */
		  double freq[])
{
    int            j;
    double         lspq[ntt_N_PR_MAX+1];
    double         out_vec[ntt_N_PR_MAX], fg_sum[ntt_N_PR_MAX];
    double         pred_vec[ntt_N_PR_MAX];
    double         buf_prev[ntt_N_PR_MAX];
    int mode, i_ma;

    mode = index[0];
    for(j=0; j<n_pr; j++){
	 fg_sum[j] =1.0;
	 for(i_ma=0; i_ma<ma_np; i_ma++){
	   fg_sum[j] -= fgcode[mode][i_ma][j];
	 }
    }
    
    for(j=0; j<n_pr; j++){ pred_vec[j] = 0.0; }

    ntt_redec(n_pr, prev_lsp_code, csize,  nsp, code, 
	      fg_sum, pred_vec, lspq, buf_prev );
    
    if(ma_np == 1){
      for(j=0; j<n_pr; j++){
       pred_vec[j] = fgcode[mode][0][j]*buf_prev[j];
      }
    }
    ntt_redec(n_pr, index, csize,  nsp, code, 
	      fg_sum, pred_vec, lspq, out_vec );
    ntt_movdd( n_pr, lspq+1, freq+1 );
}
