/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
/*   Kazuaki Chikira (NTT) on 2000-08-11,                                    */
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

/* 18-apr-97  NI   merged long, medium, & short procedure into single module */
/* 25-aug-97  NI   added bandwidth control */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"

void ntt_tf_proc_spectrum_d(/* Input */
			    ntt_INDEX  *indexp,
			    double flat_spectrum[],
			    /* Output */
			    double spectrum[])
{
  /*--- Variables ---*/
  double inv_lpc_spec[ntt_T_FR_MAX];
  double bark_env[ntt_T_FR_MAX];
  double gain[ntt_T_SHRT_MAX];
  double pit_seq[ntt_T_FR_MAX];
  double pgain[ntt_T_SHRT_MAX];
  int    i_ch, top;

  /*--- Parameters ---*/
  int    nfr = 0, nsf = 0;
  double *fw_codebook = NULL;
  int    fw_ndiv = 0, fw_cv_len = 0, fw_cv_len_max = 0;
  int    *fw_bark_tbl = NULL, fw_n_crb = 0;
  int    fw_nbit = 0;
  double fw_alf_step = 0 /*, *fw_penv_tmp = NULL */;
  double *lsp_code;
  double *lsp_fgcode;
  int    n_pr, lsp_csize[2], lsp_cdim[2];
  double band_lower, band_upper;
  int    ifr;
  int    flag_pitch = 0;
  double band_lower_base[2];

  /*--- Memories ---*/
  /* Bark-scale envelope quantization */
  
  /* LSP quantization */
  /* static double prev_buf[ntt_N_SUP_MAX][ntt_MA_NP][ntt_N_PR_MAX+1]; */

  
  /*--- Parameter settings ---*/
  switch (indexp->w_type){
  case ONLY_LONG_SEQUENCE:  /* long */
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* subframes */
    nfr = indexp->block_size_samples;
    nsf = 1;
    flag_pitch = indexp->pitch_q ;
    /* quantization (Bark-scale envelope) */
    fw_codebook = indexp->nttDataBase->ntt_fwcodev;
    fw_ndiv       = ntt_FW_N_DIV;
    fw_cv_len     = ntt_FW_CB_LEN;
    fw_cv_len_max = ntt_FW_CB_LEN;
    /* Bark-scale table (Bark-scale envelope) */
    fw_bark_tbl = indexp->nttDataBase->ntt_crb_tbl;
    fw_n_crb    = ntt_N_CRB;
    /* Reconstruction (Bark-scale envelope) */
    fw_alf_step = ntt_FW_ALF_STEP;
    fw_nbit = ntt_FW_N_BIT;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* subframes */
    nfr = indexp->block_size_samples/8;
    nsf = ntt_N_SHRT;
    flag_pitch = 0;
    /* quantization (Bark-scale envelope) */
    fw_nbit = 0;
    break;
  default:
    CommonExit(1,"window type not handled");
  }
  n_pr       = ntt_N_PR;                  /* prediction order */
  lsp_code   = indexp->nttDataBase->lsp_code_base;   /* code tables */
  lsp_fgcode = indexp->nttDataBase->lsp_fgcode_base;

  lsp_csize[0] = ntt_NC0;
  lsp_csize[1] = ntt_NC1;
  lsp_cdim[0] = ntt_N_PR;
  lsp_cdim[1] = ntt_N_PR;
  
  /*--- Decoding tools ---*/
  
  /* Periodic pitch components */
  if(flag_pitch){
    ntt_dec_pitch(indexp->pit, indexp->pls, indexp->pgain, 
		  indexp->numChannel,
		  indexp->block_size_samples,
		  indexp->isampf,
		  pit_seq, pgain, 
		  indexp->nttDataBase->bandUpper,
		  indexp->nttDataBase->ntt_codevp0, 
		  indexp->nttDataBase->pleave1);
  }
  else{
    ntt_zerod(indexp->block_size_samples*indexp->numChannel, pit_seq); 
    ntt_zerod(ntt_N_SHRT*indexp->numChannel, pgain);
  }
  /* Bark-scale envelope */
/*--- A.Jin 1997.10.19---*/
  band_lower_base[0]= band_lower_base[1]= 0.0;
  if(fw_nbit>0){
    ntt_dec_bark_env(nfr, nsf, indexp->numChannel,
                   fw_codebook, fw_ndiv, fw_cv_len, fw_cv_len_max,
                   fw_bark_tbl, fw_n_crb, fw_alf_step, 
		   indexp->nttDataBase->prev_fw_code,
                   indexp->fw, indexp->fw_alf, indexp->pf, bark_env
		   /* , band_lower_base */
		   );
        {
            int idiv;
	    for(idiv=0; idiv<ntt_FW_N_DIV*(indexp->numChannel); idiv++){
	      indexp->nttDataBase->prev_fw_code[idiv]
	      = indexp->fw[idiv];
	    }
	}

  
  }
  else{
    for(ifr=0; ifr<nfr*nsf*indexp->numChannel; ifr++ ){
      bark_env[ifr]=1.0;
    }
  }
  /* Gain */
/*--- A.Jin 1997.10.19--*/

  ntt_dec_gain(nsf, indexp->pow, indexp->numChannel, gain);

  /* LPC spectrum */
  ntt_dec_lpc_spectrum_inv(nfr, indexp->block_size_samples,
			   indexp->numChannel, ntt_N_PR, ntt_LSP_SPLIT,
                           lsp_code, lsp_fgcode,
                           lsp_csize, 
			   indexp->nttDataBase->prev_lsp_code,
			   /* prev_buf, */
			   indexp->nttDataBase->ma_np, 
                           indexp->lsp, inv_lpc_spec, 
			   indexp->nttDataBase->cos_TT);
   {
       int idiv, ich;
       indexp->nttDataBase->ma_np = 1;
       for(ich=0; ich<indexp->numChannel; ich++){
	   for(idiv=0; idiv<ntt_LSP_NIDX_MAX; idiv++){
	     indexp->nttDataBase->prev_lsp_code[idiv+ntt_LSP_NIDX_MAX*ich]
	      = indexp->lsp[ich][idiv];
           }
        }
    }

  {
       int nfr_l, nfr_lu, ismp;
       double inv_lpc_spec_nall[ntt_T_FR_MAX]; /* Tsushima add routine (This line)**/
       float ftmp;
       int ftmp_i;
	    ftmp =  (float)(indexp->nttDataBase->bandLower);
	    ftmp *= (float)nfr;
	    nfr_l  = (int)ftmp;
	    ftmp =  (float)(indexp->nttDataBase->bandUpper)
	         -  (float)(indexp->nttDataBase->bandLower);
	    ftmp *= (float)nfr;
	    nfr_lu = (int)ftmp;
	    ftmp_i =  (int)((indexp->nttDataBase->bandUpper
	         -  indexp->nttDataBase->bandLower)*16384.0);
	    ftmp_i = (16384*16384)/ftmp_i;
	    for( i_ch=0; i_ch<indexp->numChannel; i_ch++ ){
		top    = i_ch*nfr;
		ntt_zerod(nfr, inv_lpc_spec_nall+top);
		for(ismp=0; ismp<nfr_lu; ismp++){
		  int utmp_i;
		  utmp_i = ftmp_i*ismp;
		  utmp_i /= 16384;
		  inv_lpc_spec_nall[ismp+top+nfr_l] =
		  inv_lpc_spec[utmp_i+top];
		}
           } 
	    for( i_ch=0; i_ch<indexp->numChannel; i_ch++ ){
		top    = i_ch*nfr;
	        for(ismp=0; ismp<nfr; ismp++){
	            inv_lpc_spec[ismp+top] = inv_lpc_spec_nall[ismp+top];
	        }
            }
  }

  /* De-normalization */
  ntt_denormalizer_spectrum(nfr, nsf, indexp->numChannel,
			    flat_spectrum, gain, 
			    pit_seq, pgain, bark_env, inv_lpc_spec,
			    spectrum);
  /*--- Bandwidth control ---*/
  for (i_ch=0; i_ch<indexp->numChannel; i_ch++){
  float ftmp;
    top = i_ch * indexp->block_size_samples;
    if(indexp->bandlimit == 1){
      ftmp =  (float)(indexp->blim_h[i_ch])/(float)ntt_BLIM_STEP_H;
      ftmp *= (float)(1.-ntt_CUT_M_H);
      ftmp = 1.f- ftmp;
      ftmp *= (float)indexp->nttDataBase->bandUpper;
      band_upper = (double)ftmp; 
/*	  fprintf(stderr,"Bandwidth Cont active.\n");*/
    }
    else{
      band_upper = indexp->nttDataBase->bandUpper;
    }
    if(indexp->bandlimit == 1){
      ftmp = (float)(indexp->blim_l[i_ch]);
      ftmp *= (float)ntt_CUT_M_L;
      ftmp += (float)(indexp->nttDataBase->bandLower);
      band_lower = (double)ftmp;
    }
    else{
      band_lower = indexp->nttDataBase->bandLower;
    }
/*	if(indexp->bandlimit==1) */
		{
			ntt_post_process(nfr, nsf, band_lower, band_upper,
							 spectrum+top, spectrum+top);
/*			fprintf(stderr,"Postprocess active.\n");*/
		}
  }

}
