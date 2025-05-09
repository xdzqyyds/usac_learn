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
/*   Kazuaki Chikira and Takehiro Moriya (NTT) on 2000-8-11,                 */
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
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"

/* For Enh-2 moving */
int mat_scale_set_shift_para2(int iscl, int *bark_table,
			  ntt_INDEX* ntt_index, int mat_shift[])
{
  int ii, i_ch;
  int band_l_i, band_h_i;
  for(i_ch=0; i_ch<ntt_index->numChannel; i_ch++){
    band_l_i = (int)(ntt_index->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]*16384.0);
    band_h_i = (int)(ntt_index->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]]*16384.0);
    if(iscl>=0) {
      int ave0_i,ave_i,dave_i,ave1_i,iitmp_i,iitmp2_i;
      /* Long */
      ave0_i = ntt_index->block_size_samples*(band_h_i-band_l_i);
      ave_i = ave0_i / ntt_N_CRB_SCL;
      dave_i = ave0_i / (ntt_N_CRB_SCL*ntt_N_CRB_SCL);
      ave1_i = band_l_i * ntt_index->block_size_samples;
      bark_table [i_ch*ntt_N_CRB_SCL+ntt_N_CRB_SCL-1] =
	ave1_i/16384 + ave0_i / 16384;
		for(ii=0;ii<ntt_N_CRB_SCL-1;ii++){
		   iitmp_i = ii + 1;
		   iitmp2_i = iitmp_i*iitmp_i;
		   iitmp_i *= ave_i;
		   iitmp_i /= 2;
		   iitmp2_i *= dave_i;
		   iitmp2_i /= 2;
		   iitmp2_i += 8192;
		   iitmp2_i += iitmp_i;
		   bark_table[i_ch*ntt_N_CRB_SCL+ii] =
		     ave1_i/16384 + iitmp2_i/ 16384;
		}
	}
       }
  return(0);
}
#ifdef __cplusplus
}
#endif
