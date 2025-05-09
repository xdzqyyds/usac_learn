/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
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

/* 18-apr-97   NI   generalized the module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include  "ntt_relsp.h"

void ntt_redec(int n_pr,
	       int index[],
	       int csize1[],
	       int nsp,
	       double code[][ntt_N_PR_MAX],
	       double fg_sum[ntt_N_PR_MAX],
	       double pred_vec[ntt_N_PR_MAX],
	       double lspq[],
	       double out_vec[])
{
    int     i, j, off_code,  off;
    int     code1;
    int     idim;
    double  diff;
    int     ntt_isp[ntt_NSP_MAX+1];

    off_code = 0;
       off = csize1[0];
     ntt_set_isp(nsp, n_pr, ntt_isp);

     code1 = off_code + index[1];
     for (idim=0; idim<nsp; idim++){
	for(j=ntt_isp[idim]; j<ntt_isp[idim+1]; j++){
          lspq[j+1] = (code[code1][j]
		      +code[index[idim+2]+off_code+off][j]);
        }
     }
     if(lspq[1] < L_LIMIT) {
        diff = L_LIMIT - lspq[1];
        lspq[1] += diff*1.2;
     }
     if(lspq[n_pr] > M_LIMIT ){
        diff = lspq[n_pr]-M_LIMIT;
        lspq[n_pr] -= diff*1.2;
     }

     ntt_check_lsp(n_pr-1, lspq+2, MIN_GAP);
     
     ntt_movdd(n_pr, lspq+1, out_vec);
     for(j=0; j<n_pr; j++){
	  lspq[j+1] = lspq[j+1]*fg_sum[j] + pred_vec[j]; 
     }
     for(i=1; i<n_pr; i++){
          if(lspq[i] > lspq[i+1] - MIN_GAP){
            diff = lspq[i] - lspq[i+1];
            lspq[i]   -= (diff+MIN_GAP)/2.;
            lspq[i+1] += (diff+MIN_GAP)/2.;
          }
     }
     for(i=1; i<n_pr; i++){
          if(lspq[i] > lspq[i+1] - MIN_GAP*0.95){
            diff = lspq[i] - lspq[i+1];
            lspq[i]   -= (diff+MIN_GAP*0.95)/2.;
            lspq[i+1] += (diff+MIN_GAP*0.95)/2.;
          }
     }
     ntt_check_lsp_sort(n_pr-1, lspq+2);
 }
