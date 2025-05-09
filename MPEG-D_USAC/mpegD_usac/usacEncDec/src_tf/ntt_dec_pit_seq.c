/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
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

/* 25-aug-97   NI   bugfixes */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void ntt_dec_pit_seq(/* Input */
	      int    index_pit[],
              int    index_pls[],
	      int    numChannel,
	      int    block_size_samples,
	      int    isampf,
	      double bandUpper,
	      double *codevp0,
	      short *pleave1,
	      /* Output */
	      double pit_seq[])
{
  /*--- Variables ---*/
  int		i_div, i_smp;
  int    index0, index1;
  int   cb_len;
  int pol10, pol11;
  double *ptr0, *ptr1;
  double reconst[ntt_N_FR_P_MAX*ntt_N_SUP_MAX];
  int serial;	
  int bits_p[ntt_N_DIV_P_MAX], length_p[ntt_N_DIV_P_MAX];
  int ntt_N_DIV_P;

  ntt_N_DIV_P = ntt_N_DIV_PperCH*numChannel;

  ntt_vec_lenp(numChannel, bits_p, length_p);
  serial =0;
  for ( i_div=0; i_div<ntt_N_DIV_P; i_div++ ){
    index0 = (index_pls[i_div] ) & (ntt_PIT_CB_SIZE -1);
    index1 = (index_pls[i_div+ntt_N_DIV_P]) & (ntt_PIT_CB_SIZE -1);
    ptr0 = &(codevp0[(index0)*ntt_CB_LEN_P_MAX]);
    ptr1 = &(codevp0[(index1+ntt_PIT_CB_SIZE)*ntt_CB_LEN_P_MAX]);
    cb_len = length_p[i_div];
    pol10 = 1 - 2*((index_pls[i_div] >> ntt_MAXBIT_SHAPE_P) & 0x1);
    pol11 = 1 - 2*((index_pls[i_div+ntt_N_DIV_P]>> ntt_MAXBIT_SHAPE_P) & 0x1);
    for ( i_smp=0; i_smp<cb_len; i_smp++ ){
      reconst[pleave1[serial++]] = 
        (pol10*ptr0[i_smp] +pol11*ptr1[i_smp])*0.5;
    }
  }
  ntt_extend_pitch(index_pit, reconst, numChannel, block_size_samples,
		   isampf, bandUpper, pit_seq);
}
