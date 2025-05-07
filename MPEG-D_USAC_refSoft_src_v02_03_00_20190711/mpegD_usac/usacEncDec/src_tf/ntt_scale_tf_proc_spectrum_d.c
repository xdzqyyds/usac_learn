/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT)                                                          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
/*   Kazuaki Chikira (NTT) on 2000-08-11,                                    */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-23                           */
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

void ntt_scale_tf_proc_spectrum_d(/* Input */
				  ntt_INDEX  *ntt_index_scl,
				  int mat_shift[],
				  double *cos_TT,
				  double flat_spectrum[],
				  /* Output */
				  double spectrum[],
				  /* scalable layer ID */
				  int iscl)
{
  /*--- Variables ---*/
  double inv_lpc_spec[ntt_T_FR_MAX],inv_lpc_spec_nall[ntt_T_FR_MAX];
  double bark_env[ntt_T_FR_MAX];
  double gain[ntt_T_SHRT_MAX];
  double pit_seq[ntt_T_FR_MAX];
  double pgain[ntt_T_SHRT_MAX];
  int    i_ch;
  int    ifr, fw_nbit = 0;
  /*--- Parameters ---*/
  int    nfr = 0, nsf = 0;
  double *fw_codebook = NULL;
  int    fw_ndiv = 0, fw_cv_len = 0, fw_cv_len_max = 0;
  int    fw_bark_tbl[128], fw_n_crb = 0;
  double fw_alf_step = 0  /*, *fw_penv_tmp = NULL */;
  double *lsp_code;
  double *lsp_fgcode;
  int    n_pr, lsp_csize[2], lsp_cdim[2], isf, ismp, top, nfr_l, nfr_lu;
  double band_lower, band_upper;
  int band_lower_i,band_upper_i;
  /*--- Memories ---*/
  /* Bark-scale envelope quantization */
  /* static double
     env_memory_long[ntt_NSclLay_MAX][ntt_N_CRB_MAX*ntt_N_SUP_MAX]; */
  /* LSP quantization */
  /* static double
     prev_buf[ntt_NSclLay_MAX][ntt_N_SUP_MAX][ntt_MA_NP][ntt_N_PR_MAX+1]; */
  int i_sup, iptop;
  double g_gain;

  /*--- Parameter settings ---*/
  switch (ntt_index_scl->w_type){
  case ONLY_LONG_SEQUENCE:  /* long */
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* subframes */
    nfr = ntt_index_scl->block_size_samples;
    nsf = 1;
    /* quantization (Bark-scale envelope) */
    fw_codebook = ntt_index_scl->nttDataScl->ntt_fwcodev_scl;
    fw_ndiv       = ntt_FW_N_DIV_SCL;
    fw_cv_len     = ntt_FW_CB_LEN_SCL;
    fw_cv_len_max = ntt_FW_CB_LEN_SCL;
    /* Bark-scale table (Bark-scale envelope) */
    /* fw_bark_tbl = ntt_crb_tbl_scl[iscl]; */
    mat_scale_set_shift_para2(iscl, fw_bark_tbl, ntt_index_scl, mat_shift);
    fw_n_crb    = ntt_N_CRB_SCL;
    /* Reconstruction (Bark-scale envelope) */
    fw_alf_step = ntt_FW_ALF_STEP;
    /* fw_penv_tmp = env_memory_long[iscl]; */
    fw_nbit = ntt_FW_N_BIT_SCL;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* subframes */
    nfr = ntt_index_scl->block_size_samples/8;
    nsf = ntt_N_SHRT;
    /* quantization (Bark-scale envelope) */
    fw_nbit = 0;
    break;
  default:
    CommonExit(1,"window type not handled");
  }
  /* LSP quantization */
  n_pr       = ntt_N_PR_SCL;                  /* prediction order */
  lsp_code   = ntt_index_scl->nttDataScl->lsp_code_scl;   /* code tables */
  lsp_fgcode = ntt_index_scl->nttDataScl->lsp_fgcode_scl;
  lsp_csize[0] = ntt_NC0_SCL;
  lsp_csize[1] = ntt_NC1_SCL;
  lsp_cdim[0]= ntt_N_PR_SCL;
  lsp_cdim[1]= ntt_N_PR_SCL;
  
  /*--- Decoding tools ---*/
  
  ntt_zerod(ntt_index_scl->block_size_samples*ntt_index_scl->numChannel, pit_seq); 
  ntt_zerod(ntt_N_SHRT*ntt_index_scl->numChannel, pgain);
  /* Bark-scale envelope */
  for(ifr=0; ifr<nsf*nfr*ntt_index_scl->numChannel; ifr++){
     bark_env[ifr] = 1.0;
  }
  if ( fw_nbit > 0 ) {
    double band_l[ntt_N_SUP_MAX];
    int    idiv;

    for(i_ch=0; i_ch<ntt_index_scl->numChannel; i_ch++){
      band_l[i_ch] = 
        ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]];
    }
    ntt_scale_dec_bark_env( nfr, 
                            nsf, 
                            ntt_index_scl->numChannel,
                            fw_codebook, 
                            fw_ndiv, 
                            fw_cv_len, 
                            fw_cv_len_max,
                            fw_bark_tbl, 
                            fw_n_crb, 
                            fw_alf_step, 
                            ntt_index_scl->nttDataScl->prev_fw_code[iscl],
                            ntt_index_scl->fw, 
                            ntt_index_scl->fw_alf, 
                            bark_env, 
                            band_l);
    for(idiv=0; idiv<ntt_FW_N_DIV*(ntt_index_scl->numChannel); idiv++){
      ntt_index_scl->nttDataScl->prev_fw_code[iscl][idiv] 
        = ntt_index_scl->fw[idiv];
    }
  }
  else { 
    for(ifr=0; ifr<nsf*nfr*ntt_index_scl->numChannel; ifr++){
      bark_env[ifr] = 1.0;
    }
  }
  /* Gain */
  if(nsf==1) {
     for(i_sup=0; i_sup<ntt_index_scl->numChannel; i_sup++){
        ntt_dec_sq_gain(ntt_index_scl->pow[i_sup],
                ntt_AMP_MAX_SCL, ntt_MU_SCL, 
		ntt_STEP_SCL,
		ntt_AMP_NM, &gain[i_sup]);
     g_gain= gain[i_sup];
     }
  }
  else{
    for(i_sup=0; i_sup<ntt_index_scl->numChannel; i_sup++){
         iptop = i_sup * (nsf + 1);
        ntt_dec_sq_gain(ntt_index_scl->pow[iptop],
                ntt_AMP_MAX_SCL, ntt_MU_SCL, 
		ntt_STEP_SCL, ntt_AMP_NM, &g_gain);
         for(isf=0; isf<nsf; isf++){
            ntt_dec_sq_gain(ntt_index_scl->pow[iptop+1+isf],
                  ntt_SUB_AMP_MAX_SCL, ntt_MU_SCL, 
		  ntt_SUB_STEP_SCL,
                  ntt_SUB_AMP_NM, &gain[isf+i_sup*nsf]);
            gain[isf+i_sup*nsf] *=g_gain;
        }
    }
  }
  /* LPC spectrum */
  ntt_dec_lpc_spectrum_inv(nfr, ntt_index_scl->block_size_samples,
			   ntt_index_scl->numChannel, ntt_N_PR_SCL,
			   ntt_LSP_SPLIT_SCL,
			   lsp_code, lsp_fgcode,
			   lsp_csize, 
			   ntt_index_scl->nttDataScl->prev_lsp_code[iscl],
			   /*prev_buf[iscl]*/
			   ntt_index_scl->nttDataScl->ma_np, 
			   ntt_index_scl->lsp, inv_lpc_spec,
			   cos_TT);
    {
       int idiv, ich;
       ntt_index_scl->nttDataScl->ma_np = 1;
       for(ich=0; ich<ntt_index_scl->numChannel; ich++){
	 for(idiv=0; idiv<ntt_LSP_NIDX_MAX; idiv++){
          ntt_index_scl->nttDataScl->
	     prev_lsp_code[iscl][idiv+ntt_LSP_NIDX_MAX*ich] 
        = ntt_index_scl->lsp[ich][idiv];
         }
       }
    }

  /*--- band re-extension for nallow band lpc (by A.Jin 1997.6.9) ---*/
  for( i_ch=0; i_ch<ntt_index_scl->numChannel; i_ch++ ){
   int ftmp_i;
   band_upper_i = (int)(ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]] * 16384.0);
   band_lower_i = (int)(ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]] * 16384.0);

   nfr_l = (nfr * band_lower_i) / 16384;
   nfr_lu = (nfr * (band_upper_i - band_lower_i)) / 16384;

    top = i_ch * nfr;
    ntt_zerod(nfr, inv_lpc_spec_nall+top);
        for(ismp=0; ismp<nfr_lu; ismp++){
          ftmp_i = (16384*16384) / (band_upper_i - band_lower_i);
          ftmp_i = (ismp * ftmp_i)/16384;
	  inv_lpc_spec_nall[ismp+top+nfr_l] =
          inv_lpc_spec[ftmp_i+top];
        }
  }
  /* De-normalization */
  ntt_denormalizer_spectrum(nfr, nsf, ntt_index_scl->numChannel,
			    flat_spectrum, gain, 
			    pit_seq, pgain, bark_env, inv_lpc_spec_nall,
			    spectrum);

  /*--- Post processing ---*/
  for (i_ch=0;i_ch<ntt_index_scl->numChannel;i_ch++){ /* Tsushima correct */
   band_upper=ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]];
   band_lower=ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]];
    top=i_ch*ntt_index_scl->block_size_samples;
    ntt_post_process(nfr, nsf, band_lower, band_upper,
		     spectrum+top, spectrum+top);
  }

}
