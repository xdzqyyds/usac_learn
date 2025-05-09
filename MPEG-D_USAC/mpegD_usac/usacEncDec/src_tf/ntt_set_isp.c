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

#include "common_m4a.h"
#include "ntt_conf.h"
#include "ntt_relsp.h"
void ntt_set_isp(int nsp, int n_pr, int *ntt_isp)
{
    switch( nsp ){
    case 5:
	ntt_isp[0] = 0;
	ntt_isp[1] = n_pr/5-1;
	ntt_isp[2] = n_pr*2/5-1;
	ntt_isp[3] = n_pr*3/5-1;
	ntt_isp[4] = n_pr*4/5-1;
	ntt_isp[5] = n_pr;
	break;
    case 4:
	ntt_isp[0] = 0;
	ntt_isp[1] = n_pr/4-1;
	ntt_isp[2] = n_pr/2-1;
	ntt_isp[3] = n_pr/2+n_pr/4-1;
	ntt_isp[4] = n_pr;
	break;
    case 3:
	ntt_isp[0] = 0;
	ntt_isp[1] = n_pr/3-1;
	ntt_isp[2] = n_pr-n_pr/3;
	ntt_isp[3] = n_pr;
	break;
    case 2:
	ntt_isp[0] = 0;
	ntt_isp[1] = n_pr/2-1;
	ntt_isp[2] = n_pr;
	break;
    default:
	CommonExit ( 1, "LSP: Number of split: %d: Not supoorted.\n",
		nsp );
    }
}
