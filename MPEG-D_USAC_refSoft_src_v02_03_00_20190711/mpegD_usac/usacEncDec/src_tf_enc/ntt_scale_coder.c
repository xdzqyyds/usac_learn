/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin      (NTT),                                                    */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/*   Kazuaki Chikira (NTT) on 2000-08-11,                                    */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-11                           */
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

#include <stdio.h>
#include <math.h>

#include "allHandles.h"
#include "tf_mainHandle.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_scale_enc_para.h"
#include "ntt_encode.h"
#include "ntt_scale_encode.h"
#include "ntt_tools.h"


void ntt_scale_enc_lpc_spectrum
  (/* Parameters */
   int    nfr,
   int    nsf,
   int    block_size_samples,
   int    n_ch,
   int    n_pr,
   double *lsp_code,
   double *lsp_fgcode,
   int    prev_lsp_code[],
   /*
   double prev_buf[MAX_TIME_CHANNELS][ntt_MA_NP][ntt_N_PR_MAX+1],
   */
   int    ma_np,
   /* Input */
   double spectrum[],
   int    w_type,
   int    current_block,
   /* Output */
   int    index_lsp[MAX_TIME_CHANNELS][ntt_LSP_NIDX_MAX],
   double lpc_spectrum[],
   /* Scalable layer ID */
   int iscl,
   double band_lower[],
   double band_upper[],
   double *cos_TT
   )
{
  /*--- Variables ---*/
  double resid, alf[MAX_TIME_CHANNELS][ntt_N_PR_MAX+1],
  lsp[ntt_N_PR_MAX+1], lspq[MAX_TIME_CHANNELS*(ntt_N_PR_MAX+1)];
  double spectrum_wide[ntt_T_FR_MAX],
         lpc_spectrum_re[ntt_T_FR_MAX];
  int    i_sup, top, subtop, w_type_t, isf, lsptop, ismp;
  int    block_size, nfr_l, nfr_u, nfr_lu;
  int lsp_csize[2];
  float ftmp ;
  int ftmp_i;

  block_size = nfr * nsf;

  lsp_csize[0] = ntt_NC0_SCL;
  lsp_csize[1] = ntt_NC1_SCL;
  /*--- encode LSP coefficients ---*/
  if (current_block == ntt_BLOCK_LONG) w_type_t = w_type;
  else                                 w_type_t = ONLY_LONG_SEQUENCE;
  for ( i_sup=0; i_sup<n_ch; i_sup++ ){
       ftmp = (float)band_upper[i_sup];
       ftmp *= (float)nfr;
       nfr_u  = (int)ftmp;
       ftmp = (float)band_lower[i_sup];
       ftmp *= (float)nfr;
       nfr_l  = (int)ftmp;
       ftmp = (float)band_upper[i_sup] - (float)band_lower[i_sup];
       ftmp *= (float)nfr;
       nfr_lu = (int)ftmp;
       top    = i_sup*block_size;
       ntt_zerod(block_size, spectrum_wide+top);
       ntt_setd(block_size,1.e5, lpc_spectrum+top);

       ftmp_i = (int)((band_upper[i_sup]-band_lower[i_sup])*16384.0);
       ftmp_i = (16384*16384)/ftmp_i;
       /*--- band extension for nallow band lpc (by A.Jin 1997.6.9) ---*/
       for(isf=0; isf<nsf; isf++){
	    int fftmp_i;
	    subtop = top + isf*nfr;
	    for(ismp=nfr_l; ismp<nfr_l+nfr_lu; ismp++){
	      fftmp_i = ftmp_i;
	      fftmp_i *= (ismp - nfr_l);
	      fftmp_i /= 16384;
	      spectrum_wide[fftmp_i+subtop] = spectrum[ismp + subtop];
	    }
       }

       /* calculate LSP coefficients */
       ntt_scale_fgetalf(nfr, nsf, block_size_samples,
			 spectrum_wide+top,alf[i_sup],&resid,
			 ntt_N_PR_SCL, ntt_BAND_EXPN_SCL, cos_TT);
       ntt_alflsp(ntt_N_PR_SCL, alf[i_sup], lsp);
       /* quantize LSP coefficients */
       ntt_lsp_encw(ntt_N_PR_SCL, (double (*)[ntt_N_PR_MAX])lsp_code,
		    (double (*)[ntt_MA_NP][ntt_N_PR_MAX])lsp_fgcode,
		    lsp_csize,
		    prev_lsp_code+i_sup*ntt_LSP_NIDX_MAX,
		    /* prev_buf[i_sup], */
		    ma_np, 
		    lsp, lspq+i_sup*ntt_N_PR_SCL,
		    index_lsp[i_sup], ntt_LSP_SPLIT_SCL);
  }

  /*--- reconstruct LPC spectrum ---*/
  for (i_sup=0; i_sup<n_ch; i_sup++){
       ftmp = (float)band_lower[i_sup];
       ftmp *= (float)nfr;
       nfr_l  =(int)ftmp;
       ftmp = (float)band_upper[i_sup]-(float)band_lower[i_sup];
       ftmp *= (float)nfr;
       nfr_lu = (int)ftmp;
       nfr_u  = nfr_l+nfr_lu;
       top = i_sup * block_size;
       lsptop = i_sup * n_pr;
       ntt_lsptowt(nfr, n_pr, block_size_samples,
		   lspq+lsptop, lpc_spectrum+top, cos_TT);

       /*--- band re-extension for nallow band lpc (by A.Jin 1997.6.9) ---*/
       ftmp_i = (int)((band_upper[i_sup]-band_lower[i_sup])*16384.0);
       ftmp_i = (16384*16384)/ftmp_i;
       for(ismp=0; ismp<nfr_lu; ismp++){
	 int fftmp_i;
	 fftmp_i = ftmp_i*ismp;
	 fftmp_i /= 16384;
	 lpc_spectrum_re[ismp+top+nfr_l] = lpc_spectrum[fftmp_i+top];
       }
       ntt_movdd(nfr_lu,lpc_spectrum_re+top+nfr_l, lpc_spectrum+top+nfr_l);
       ntt_setd(nfr_l, 999., lpc_spectrum+top);
       ntt_setd(nfr- nfr_u, 999., lpc_spectrum+top+nfr_u);

       for (isf=1; isf<nsf; isf++){
	    subtop = top + isf*nfr;
	    ntt_movdd(nfr, lpc_spectrum+top, lpc_spectrum+subtop);
       }
  }
}


#define ntt_GAMMA  0.95 

void ntt_scale_fgetalf(/* Parameters */
		 int    nfr,    /* subframe size */
		 int    nsf,    /* number of subframes */
		 int    block_size_samples,
		 /* Input */
		 double *spectrum,  /* MDCT spectrum */
		 /* Output */
		 double *alf,   /* LPC coefficients */
		 double *resid, /* residual power */
		 /* Input */
		 int n_pr,  
		 double band_exp,
		 double *ntt_cos_TT) 
{
    double wlag[ntt_N_PR_MAX + 1];
    double powspec[ntt_N_FR_MAX];
    int    unstable;
    int    ip;
    double ref[ntt_N_PR_MAX+1];         
    double cep[(ntt_N_PR_MAX+1)*2];        
    double refb[ntt_N_PR_MAX+1];      
    double alfb[ntt_N_PR_MAX+1];     
    double gamma; 			   
    double          
	cor[ntt_N_PR_MAX+1],
        pow_counter;
    double 
      xr[ntt_N_FR_MAX*2];
    int    ismp, isf;

    /*--- Initialization ---*/
    /*--- set lag window ---*/
    ntt_lagwdw( wlag, n_pr, band_exp );

    /*--- Calculate power spectrum ---*/
    pow_counter = 0.;
    for (ismp=0; ismp<nfr; ismp++) powspec[ismp] = 0.;
    for (isf=0; isf<nsf; isf++){
      for (ismp=0; ismp<nfr; ismp++){
	powspec[ismp] += spectrum[ismp+isf*nfr]*spectrum[ismp+isf*nfr];
	pow_counter   += powspec[ismp]*1.e-6;
      }
    }
    /*--- convert low power signals to zero(for certain LPC) ---*/
    if(pow_counter<1.e-2){
      ntt_zerod(nfr, powspec);
    }
    /*--- Calculate auto correlation coefficients ---*/
 {
 double tmp, theta;
 int jsmp, pointer, pointer0;
     for(jsmp=0; jsmp<n_pr+1; jsmp++){
       tmp=0.0;
       pointer =0; pointer0=0; theta=1.0;
       for(ismp=0; ismp<nfr; ismp++){
        tmp += powspec[ismp]*theta;
        pointer += jsmp*block_size_samples*4/nfr;
        if(pointer >= block_size_samples*8) pointer -= block_size_samples*8;
        if(0<= pointer && pointer <block_size_samples*2 ) {
          pointer0 = pointer;
	  theta = ntt_cos_TT[pointer0];
        }
        else if(block_size_samples*2 <= pointer && pointer <block_size_samples*4 ){
            pointer0 = block_size_samples*4-pointer;
            if(pointer0==block_size_samples*2) theta =0.0;
	    else  theta = -ntt_cos_TT[pointer0];
        }
        else if(block_size_samples*4<= pointer && pointer <block_size_samples*6 ){
            pointer0 = pointer-block_size_samples*4;
            theta = -ntt_cos_TT[pointer0];
        }
        else if(block_size_samples*6<= pointer && pointer <block_size_samples*8 ){
            pointer0 = block_size_samples*8-pointer;
            if(pointer0==block_size_samples*2) theta =0.0;
	    else  theta = ntt_cos_TT[pointer0];
        }
       }
        xr[jsmp]= tmp;
     }
 }
    ntt_movdd(n_pr+1, xr, cor);
    for (ip=1; ip<=n_pr; ip++){
      cor[ip] /= (cor[0] + 1.e-5)*1.02;
    }
    cor[0] = 1.0;

    /*--- LPC analysis --*/
    ntt_mulddd(n_pr + 1, cor, wlag, cor);
    ntt_corref(n_pr, cor, alfb ,refb, resid); alfb[0] = 1.;

    /*--- LPC parameters to cepstrum parameters ---*/
    ntt_alfcep(n_pr, alfb, cep, (n_pr)*2);
   
    /*--- Divide cepstrum parameters by 2 (sqrt in the spectrum domain) ---*/
    ntt_mulcdd((n_pr)*2, 0.5, cep+1, cep+1);

    /*--- Cepstrum parameters to LPC alpha parameters ---*/
    ntt_cep2alf((n_pr)*2, n_pr, cep, alf);

    /*--- Stability check ---*/
    alf[0]=1.0;
    do{
       ntt_alfref(n_pr, alf, ref, resid);
       unstable =0;
       for(ip=1; ip<=n_pr; ip++){
          if(ref[ip] > 0.999 || ref[ip] < -0.999) unstable =1;
          if(ref[ip] > 0.999 || ref[ip] < -0.999)
	       fprintf(stderr,
		       "ntt_scale_fgetalf(): unstable ref. pram. order:%5d, value: %7.4f\n",
		       ip, ref[ip]);
       }
       if(unstable) {
         gamma= ntt_GAMMA;
         for(ip=1; ip<=n_pr; ip++){
           alf[ip] *= gamma;
           gamma *= ntt_GAMMA;
         }
       }
    }while(unstable);

}

void ntt_scale_tf_perceptual_model(/* Input */
				   double lpc_spectrum[], /* LPC spectrum */
				   double bark_env[],     /* Bark-scale envelope */
				   double gain[],         /* gain factor */
				   int       w_type,      /* block type */
				   double spectrum[],     /* specturm */
				   double pitch_sequence[], /* pitch peak components */
				   int    numChannel,
				   int    block_size_samples,
				   /* Output */
				   double perceptual_weight[]) /* perceptual weight */
{
    /*--- Variables ---*/
    int    ismp, i_sup, top, isf, subtop;
    double ratio;

    switch(w_type){
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    for (ismp=0; ismp<block_size_samples; ismp++){
		perceptual_weight[ismp+top] = (1./lpc_spectrum[ismp+top]);
	    }
            ntt_prcptw(perceptual_weight+top, bark_env+top,
	    ntt_GAMMA_W2, ntt_GAMMA_W_MIC2, block_size_samples);
            ratio = pow((gain[i_sup]+0.0001), ntt_GAMMA_CH)/(gain[i_sup]+0.0001);
	    for (ismp=0; ismp<block_size_samples; ismp++){
		perceptual_weight[ismp+top] *=
		    lpc_spectrum[ismp+top]/bark_env[ismp+top]*ratio;
	    }

	}
	break;
    case EIGHT_SHORT_SEQUENCE:
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    for (isf=0; isf<ntt_N_SHRT; isf++){
		subtop = top + isf * block_size_samples/8;
		for (ismp=0; ismp<block_size_samples/8; ismp++){
		    perceptual_weight[ismp+subtop] =
		      (bark_env[ismp+subtop] / lpc_spectrum[ismp+subtop]);
		}
	    }
	}
        ntt_prcptw_s(perceptual_weight, gain, perceptual_weight,
	      ntt_GAMMA_W_S2, ntt_GAMMA_W_S_T2,
	      numChannel, block_size_samples);
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    for (isf=0; isf<ntt_N_SHRT; isf++){
		subtop = top + isf * block_size_samples/8;
		for (ismp=0; ismp<block_size_samples/8; ismp++){
		    perceptual_weight[ismp+subtop] *=
			lpc_spectrum[ismp+subtop] / bark_env[ismp+subtop];
		}
	    }
	}
	break;
    }
}


void ntt_scale_tf_proc_spectrum(/* In/Out */
                                double spectrum[],
                                /* In/Out */
                                ntt_INDEX  *index,
				int  mat_shift[ntt_N_SUP_MAX],
                                double   *cos_TT,
				/* Output */
                                double lpc_spectrum[],
                                double bark_env[],
                                double pitch_sequence[],
                                double gain[],
                                /* In/Out */
                                int iscl,
                                double lpc_spectrum_org[],
                                double lpc_spectrum_mod[])
{
  double tc[ntt_T_FR_MAX], pwt[ntt_T_FR_MAX];
  int    current_block = 0;
  int    i_ch;
  double band_l[MAX_TIME_CHANNELS], band_u[MAX_TIME_CHANNELS];

  /*--- Parameters ---*/
  int    nfr = 0;
  int    nsf = 0;
  int    n_ch = 0;
  int    n_crb, bark_tbl[128];
  int    fw_ndiv, fw_length, fw_len_max, fw_cb_size;
  double fw_prcpt;
  double *fw_codev;
  /* double *fw_env_memory, *fw_pdot; */
  int    n_pr;
  double *lsp_code;
  double *lsp_fgcode;
  int    lsp_csize[2], lsp_cdim[2];
  /*--- Memories ---*/
  /* Bark-scale envelope */
  int   p_w_type[ntt_NSclLay_MAX];
  /* LPC spectrum */
  int fw_nbit = 0;
  int ifr;
  int isf, top, subtop, ismp, block_size;

  /*--- Set parameters ---*/
  n_crb=fw_ndiv=fw_length=fw_len_max=fw_cb_size=0;
  fw_prcpt= 0.;
  fw_codev = index->nttDataScl->ntt_fwcodev_scl;
  switch(index->w_type){
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* subframe */
    nfr = index->block_size_samples;
    nsf = 1;
    n_ch = index->numChannel;
    /* Bark-scale table (Bark-scale envelope) */
    n_crb    = ntt_N_CRB_SCL;
    /* bark_tbl = ntt_crb_tbl_scl[iscl]; */
    
    mat_scale_set_shift_para2(iscl, bark_tbl, index, mat_shift);

    /* quantization (Bark-scale envelope) */
    fw_ndiv = ntt_FW_N_DIV_SCL;
    fw_cb_size = ntt_FW_CB_SIZE_SCL;
    fw_length  = ntt_FW_CB_LEN_SCL;
    fw_len_max = ntt_FW_CB_LEN_SCL;
    fw_codev = index->nttDataScl->ntt_fwcodev_scl;
    fw_prcpt = 0.4;
    /*fw_env_memory = fw_env_memory_long[iscl]; */
    /* envelope calculation (Bark-scale envelope) */
    current_block = ntt_BLOCK_LONG;
    fw_nbit = ntt_FW_N_BIT_SCL;
    break;
 case EIGHT_SHORT_SEQUENCE:
    /* subframe */
    nfr = index->block_size_samples/8;
    nsf = ntt_N_SHRT;
    n_ch = index->numChannel;
    /* Bark-scale table (Bark-scale envelope) */
    current_block = ntt_BLOCK_SHORT;
    fw_nbit = 0;
    break;
 default:
        fprintf( stderr,"Fatal error! %d: no such window type.\n", index->w_type );
        exit(1);
}
  /* LPC spectrum */
  n_pr = ntt_N_PR_SCL;
  lsp_code   = index->nttDataScl->lsp_code_scl;
  lsp_fgcode = index->nttDataScl->lsp_fgcode_scl;
  lsp_csize[0] = ntt_NC0_SCL;
  lsp_csize[1] = ntt_NC1_SCL;
  lsp_cdim[0]= ntt_N_PR_SCL;
  lsp_cdim[1]= ntt_N_PR_SCL;

  block_size = nfr*nsf ; /* tsushima add */

  p_w_type[iscl] = index->nttDataScl->prev_w_type;

  /*--- Encoding tools ---*/
  /* LPC spectrum encoding */
 { 
    for (i_ch=0;i_ch<n_ch;i_ch++){ 
      band_l[i_ch] = 
	 index->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]; 
      band_u[i_ch] = 
	 index->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]]; 
    }
    ntt_scale_enc_lpc_spectrum(nfr, nsf, index->block_size_samples,
			     n_ch, n_pr, lsp_code, lsp_fgcode,
			     index->nttDataScl->prev_lsp_code[iscl],
			     index->nttDataScl->ma_np,
                             spectrum, index->w_type, current_block,
                             index->lsp, lpc_spectrum, iscl,
                             band_l, band_u, cos_TT);
  }
  /* pre process (bandwidth setting) */
  for (i_ch=0;i_ch<n_ch;i_ch++){ /* corrected by TSUSHIMA */
    top=i_ch*index->block_size_samples;
    ntt_freq_domain_pre_process(nfr, nsf, band_l[i_ch], band_u[i_ch],
				spectrum+top, lpc_spectrum_org+top,
				spectrum+top, lpc_spectrum_mod+top); /* correct*/
    ntt_freq_domain_pre_process(nfr, nsf, band_l[i_ch], band_u[i_ch],
				spectrum+top, lpc_spectrum+top,
				spectrum+top, lpc_spectrum+top); /*correct*/
  }

  ntt_zerod(index->block_size_samples*index->numChannel, pitch_sequence);

  /* Bark-scale envelope encoding */

  if(fw_nbit == 0){
       for(ifr=0; ifr<nfr*nsf*n_ch; ifr++){
	    bark_env[ifr]=1.0;
       }
       for (i_ch=0; i_ch<n_ch; i_ch++){
	    top = i_ch * block_size;
	    for(isf=0; isf<nsf; isf++){
		 subtop = top + isf*nfr;
		 /* make input data */
		 for (ismp=0; ismp<nfr; ismp++){
		      tc[ismp+subtop] =
			   spectrum[ismp+subtop] * lpc_spectrum[ismp+subtop]
				- pitch_sequence[ismp+subtop];
		 }
	    }
       }
  }else{
     ntt_scale_enc_bark_env(nfr, nsf, n_ch, n_crb, bark_tbl,
		      fw_ndiv, fw_cb_size, fw_length,
		      fw_codev, fw_len_max, fw_prcpt,
		      index->nttDataScl->prev_fw_code[iscl],
		      &(p_w_type[iscl]),
		      spectrum, lpc_spectrum, pitch_sequence, current_block,
		      tc, pwt, index->fw, index->fw_alf, bark_env, band_l);
     if(index->w_type == EIGHT_SHORT_SEQUENCE)
	    index->nttDataScl->prev_w_type = ntt_BLOCK_SHORT;
     else   index->nttDataScl->prev_w_type = ntt_BLOCK_LONG;
  }
  /* gain encoding */
  ntt_scale_enc_gain_t(nfr, nsf, n_ch, iscl, bark_env, tc, index->pow, gain);

}



void ntt_scale_enc_bark_env(/* Parameters */
                      int    nfr,
                      int    nsf,
                      int    n_ch,
                      int    n_crb,
                      int    *bark_tbl,
                      int    ndiv,
                      int    cb_size,
                      int    length,
                      double *codev,
                      int    len_max,
                      double prcpt,
                      int    prev_fw_code[],
		      /*
		      double *env_memory,
                      double *pdot,
                      */
		      int    *p_w_type,
                      /* Input */
                      double spectrum[],
                      double lpc_spectrum[],
                      double pitch_sequence[],
                      int    current_block,
                      /* Output */
                      double tc[ntt_T_FR_MAX],
                      double pwt[ntt_T_FR_MAX],
                      int    index_fw[],
                      int    ind_fw_alf[],
                      double bark_env[],
                      double band_lower[])
{
  /*--- Variables ---*/
  int    ismp, i_ch, top, isf, subtop, pred_sw, block_size;

  /*--- Initialization ---*/
  block_size = nfr * nsf;

  /*--- Encoding process ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * block_size;
    /* make weight */
    for (ismp=0; ismp<block_size; ismp++){
      /*pwt[ismp+top] = 1./lpc_spectrum[ismp+top];*/
      pwt[ismp+top] = 1. / pow(lpc_spectrum[ismp+top], 0.5);
  }

    /* set MA prediction switch */
    if (*p_w_type == current_block) pred_sw = 1;
    else                           pred_sw = 0;

    for(isf=0; isf<nsf; isf++){
      subtop = top + isf*nfr;
      /* make input data */
      for (ismp=0; ismp<nfr; ismp++){
        tc[ismp+subtop] =
          spectrum[ismp+subtop] * lpc_spectrum[ismp+subtop]
            - pitch_sequence[ismp+subtop];
    }
   /* envelope coding */
      ntt_scale_fwpred(nfr, n_crb, bark_tbl, ndiv,
                 cb_size, length, codev, len_max, prcpt,
                 prev_fw_code+(i_ch*nsf+isf)*ndiv,
		 /*env_memory, pdot, */
                 pwt+subtop, bark_env+subtop, tc+subtop,
                 &index_fw[(i_ch*nsf+isf)*ndiv],
                 &ind_fw_alf[isf+i_ch*nsf], i_ch, pred_sw, band_lower[i_ch]);
      pred_sw = 1;
  }
}
  *p_w_type = current_block;
}


void ntt_scale_fwpred(int    nfr,  /* Param:  block size */
                int    n_crb,       /* Param:  number of Bark-scale band */
                int    *bark_tbl, /* Param:  Bark-scale table */
                int    ndiv,        /* Param:  number of interleave division */
                int    cb_size,     /* Param:  codebook size */
                int    length,      /* Param:  codevector length */
                double *codev,      /* Param:  codebook */
                int    len_max,     /* Param:  codevector memory length */
                double prcpt,
                int    prev_fw_code[],
		/*double *penv_tmp,*//*Param: memory for Bark-scale envelope*/
                /*double *pdot_tmp,*//*Param: memory for env. calculation */
                double iwt[],       /* Input:  LPC envelope */
                double pred[],      /* In/Out: Prediction factor */
                double tc[],        /* Input:  Residual signal */
                int    index_fw[],  /* Output: Envelope VQ indices */
                int    *ind_fw_alf, /* Output: AR prediction coefficient index */
                int    i_sup,       /* Input:  Channel number */
                int    pred_sw,     /* Input:  prediction switch */
                double band_lower)
{
    /*--- Variables ---*/
    double        bwt[ntt_N_CRB_MAX], env[ntt_N_CRB_MAX];
    double        fwgain, alf, alfq;
    double        penv[ntt_N_CRB_MAX];
    register double acc, cdot, acc2, col0, col1, dtmp;
    int           top, top2, ismp, iblow, ibhigh, ib;
    int   n_crb_eff;
    float ftmp;

    /*--- Initialization ---*/
    top  = i_sup*nfr;
    top2 = i_sup*n_crb;

    ntt_fwex( prev_fw_code, ndiv, length, codev, len_max, penv );

    /*--- Calculate envelope ---*/
    ftmp = (float)band_lower;
    ftmp *= (float)nfr;
    iblow= (int)ftmp; /*iblow=0;*/ fwgain=0.; ib=0;
	  n_crb_eff=0;
    for (ib=0; ib<n_crb; ib++){
        ibhigh = bark_tbl[ib +i_sup*n_crb ];
	/* reset the accumulations */
        acc  = 0.0;
        acc2 = 0.0;
        /* bandle the input coefficient lines into the bark-scale band */
        for(ismp=iblow; ismp<ibhigh; ismp++){
            cdot =  tc[ismp]*tc[ismp];/* calc. power */
            acc +=  cdot; 
	    acc2 = acc2 + iwt[ismp];  /* accumulate the weighting factor */
        }
        env[ib] = sqrt(acc/(double)(ibhigh-iblow)) + 0.1; /* envelope in bark*/
        bwt[ib] = acc2;         /* weighting factor in bark scale */
        if(env[ib]> 0.1) {
	  n_crb_eff++;
	  fwgain += env[ib];      /* accumulate the envelope power */
        }
        iblow = ibhigh;
    }
/* gain to normalize the bark-scale envelope */
    fwgain = (double) n_crb_eff / fwgain;

    /*--- Normalize the envelope, and remove the offset ---*/
    for (ib=0; ib<n_crb_eff; ib++){
        env[ib] = env[ib]*fwgain - 1.0;
    }
    for (ib=n_crb_eff; ib<n_crb; ib++){
        env[ib] = 0.0;
    }

    /*--- Calculate the AR prediction coefficient and quantize it ---*/
    col0 = 0.1;
    col1 = 0.;
    if (pred_sw == 1){
      /* calculate auto correlation and cross correlation */
      for (ib=0; ib<n_crb; ib++){
        col0 += penv[ib]*penv[ib];
        col1 += penv[ib]*env[ib];
    }
      alf = col1/col0;   /* normalize the cross correlation coefficient */
      ntt_fwalfenc( alf, &alfq, ind_fw_alf ); /* quantize the cor. coefficient */
      /* AR prediction (cancellation using cor. coeff.) */
      for ( ib=0; ib<n_crb; ib++ ){
        env[ib] -= alfq*penv[ib];
    }
  }
    else{ /* in stop frame */
      alf = 0.;
      ntt_fwalfenc( alf, &alfq, ind_fw_alf );
  }

    /*--- Quantization of the envelope ---*/
    ntt_fwvq(ndiv, cb_size, length, codev, len_max, prcpt,
             env, bwt, index_fw);
    /*--- Local decoding ---*/
    ntt_fwex( index_fw, ndiv, length, codev, len_max, env );
    for(ib=ndiv*length; ib<n_crb; ib++) env[ib] = 0.;

    for(ismp=0;ismp<nfr*band_lower;ismp++){
      pred[ismp]=1.0;
  }
    for(ismp=bark_tbl[n_crb-1  +i_sup*n_crb];ismp<nfr;ismp++){
      pred[ismp]=1.0;
  }

    for(ib=0, ismp=(int)ftmp; ib<n_crb; ib++){
        ibhigh = bark_tbl[ib  +i_sup*n_crb ];
        dtmp = ntt_max(env[ib]+alfq*penv[ib]+1., ntt_FW_LLIM);
        while(ismp<ibhigh){
            pred[ismp] = dtmp;
            ismp++;
        }
    }
    /*--- Store the current envelope ---
    ntt_movdd(n_crb, env, penv); */
}


void ntt_scale_enc_gain_t(/* Parameters */
                    int    nfr,
                    int    nsf,
                    int    n_ch,
                    int    iscl,
                    /* Input */
                    double bark_env[],
                    double tc[],
                    /* Output */
                    int    index_pow[],
                    double gain[])
{
  int    ismp, top, subtop, isf, iptop, gtop, i_ch, block_size;
  double pwr;
  double sub_pwr_tmp, sub_pwr[ntt_N_SHRT_MAX];
  double g_gain;

  block_size = nfr * nsf;

  /*--- Encoding process ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * block_size;
    /*--- Gain calculation ---*/
    pwr = 0.1;
    for(isf=0; isf<nsf; isf++){
      subtop = top + isf*nfr;
      for ( ismp=0; ismp<nfr; ismp++ ){
        tc[ismp+subtop] /= bark_env[ismp+subtop];
    }
      sub_pwr_tmp
        =(ntt_dotdd(nfr, tc+subtop, tc+subtop)+0.1)/(double)nfr;
      pwr += sub_pwr_tmp;
      sub_pwr[isf] =sqrt( sub_pwr_tmp );
  }
    pwr = sqrt(pwr/(double)nsf);

    /*--- Quantization ---*/
    if (isf == 1){
      /* global gain */
      ntt_enc_sq_gain(pwr, ntt_AMP_MAX_SCL, ntt_MU_SCL,
                      ntt_STEP_SCL, ntt_AMP_NM,
                      &index_pow[i_ch],&gain[i_ch]);
  }
    else{
      /* global gain */
      iptop = (nsf+1)*i_ch;
      gtop  = nsf*i_ch;
      ntt_enc_sq_gain(pwr, ntt_AMP_MAX_SCL, ntt_MU_SCL,
                      ntt_STEP_SCL, ntt_AMP_NM,
                      &index_pow[iptop],&g_gain);

      /*
      gain[gtop] = g_gain;
      */
      /* subband gain */
      for(isf=0; isf<nsf; isf++){
        subtop = top + isf*nfr;
        ntt_enc_sq_gain(sub_pwr[isf]/(g_gain+0.0000001),
                      ntt_SUB_AMP_MAX_SCL, ntt_MU_SCL,
                      ntt_SUB_STEP_SCL, ntt_SUB_AMP_NM,
                     &index_pow[iptop+isf+1],&gain[gtop+isf]);
                gain[gtop+isf] *= g_gain;
    }
  }
}
}


void ntt_enc_sq_gain(
              double power,      /* Input  --- Power to be encoded */
              double  amp_max,
              double  mu,
              double  step,
              double  amp_nm,
              int    *index_pow, /* Output --- Power code index */
              double *gain)      /* Output --- Gain */
{
    /*--- Variables --*/
    double mu_power, dmu_power, dec_power;

    mu_power = ntt_mulaw(power, amp_max, mu);         /* mu-Law power */
    *index_pow = (int)floor(mu_power/ step);               /* quantization */
    dmu_power = *index_pow * step + step/2.;        /* decoded mu-Law power */
    dec_power = ntt_mulawinv(dmu_power, amp_max, mu); /* decoded power */
    *gain= dec_power / amp_nm;                        /* calculate gain */
}

/* T.Ishikawa 980813 */
#define mat_ERR_WEI_0 0.5
#define mat_ERR_WEI_1 1.2
#define mat_ERR_WEI_2 2.0
#define mat_ERR_WEI_3 2.5
#define mat_PERCEPT_POW 0.45

#define mat_SHORT_DELAY 16
#define mat_SPEECH 20

int mat_scale_lay_shift2(double spectrum[],
			double spectrum_org[],
			int iscl,
			ntt_INDEX *index,
			int mat_shift[ntt_N_SUP_MAX])
{
	int ii, i_ch;
	double err_max,err[4];
	double tmp_spect[ntt_N_FR_MAX];

	/* T.Ishikawa 980804 */
	static int short_delay_flag,short_count,short_flag;

	double lpc_spect_tmp[ntt_T_FR_MAX],bark_env_tmp[ntt_T_FR_MAX];
	double lpc_spect_org[ntt_T_FR_MAX],bark_env_org[ntt_T_FR_MAX];
	double pitch_sequence_tmp[ntt_T_FR_MAX];
	double pitch_sequence_org[ntt_T_FR_MAX];
	double gain_tmp[ntt_T_SHRT_MAX];
	double gain_org[ntt_T_SHRT_MAX];
        double acc;
        double mat_wei[4];

	ntt_zerod(ntt_T_FR_MAX,pitch_sequence_tmp);
	ntt_zerod(ntt_T_FR_MAX,pitch_sequence_org);
	ntt_zerod(ntt_T_SHRT_MAX,gain_tmp);
	ntt_zerod(ntt_T_SHRT_MAX,gain_org);
	ntt_zerod(ntt_T_FR_MAX,lpc_spect_tmp);
	ntt_zerod(ntt_T_FR_MAX,lpc_spect_org);
	ntt_zerod(ntt_T_FR_MAX,bark_env_tmp);
	ntt_zerod(ntt_T_FR_MAX,bark_env_org);



	if((iscl==1)&&(index->w_type==EIGHT_SHORT_SEQUENCE))
		{
			short_flag=1;
			short_delay_flag=1;
			short_count=0;
		}
	

	if(iscl==1)
	short_count++;

	if(short_count < mat_SHORT_DELAY )
		{
			short_delay_flag=1;
		}
	else
		{
			short_delay_flag=0;
			short_flag=0;
		}
	
	
	   switch(iscl)
		   {
			   case 0:
			   mat_wei[0]=mat_ERR_WEI_0*1.1;
			   mat_wei[1]=mat_ERR_WEI_1*4.1;
			   mat_wei[2]=mat_ERR_WEI_2*0.1;
			   mat_wei[3]=mat_ERR_WEI_3*0.1;
			   break;
			   case 1:
			   mat_wei[0]=mat_ERR_WEI_0*0.1;
			   mat_wei[1]=mat_ERR_WEI_1*5.0;
			   mat_wei[2]=mat_ERR_WEI_2*0.1;
			   mat_wei[3]=mat_ERR_WEI_3*0.1;
			   break;
			   case 2:
			   mat_wei[0]=mat_ERR_WEI_0*0.1;
			   mat_wei[1]=mat_ERR_WEI_1*0.1;
			   mat_wei[2]=mat_ERR_WEI_2*8.0;
			   mat_wei[3]=mat_ERR_WEI_3*0.1;
			   break;
			   case 3:
			   mat_wei[0]=mat_ERR_WEI_0*0.1;
			   mat_wei[1]=mat_ERR_WEI_1*0.1;
			   mat_wei[2]=mat_ERR_WEI_2*0.1;
			   mat_wei[3]=mat_ERR_WEI_3*10.0;
			   break;
			   case 4:
			   mat_wei[0]=mat_ERR_WEI_0;
			   mat_wei[1]=mat_ERR_WEI_1;
			   mat_wei[2]=mat_ERR_WEI_2;
			   mat_wei[3]=mat_ERR_WEI_3;
			   break;
			   case 5:
			   mat_wei[0]=mat_ERR_WEI_0;
			   mat_wei[1]=mat_ERR_WEI_1;
			   mat_wei[2]=mat_ERR_WEI_2;
			   mat_wei[3]=mat_ERR_WEI_3;
			   break;
			   case 6:
			   mat_wei[0]=mat_ERR_WEI_0;
			   mat_wei[1]=mat_ERR_WEI_1;
			   mat_wei[2]=mat_ERR_WEI_2;
			   mat_wei[3]=mat_ERR_WEI_3;
			   break;
		   }
	{
               int ind, bias, qsample;
              static int prev[8][2] ={{0,0},{1,1},{2,2},
                                      {3,3}, {0,0}, {0,0},{0,0},{0,0}};
	      static double prev_pow[8][2] =
                                 {{0.0,0.0}, {0.0,0.0},{0.0,0.0},{0.0,0.0},
                                  {0.0,0.0}, {0.0,0.0},{0.0,0.0},{0.0,0.0}};
	      int smin[2], smax[2];
	switch(index->w_type){
	case ONLY_LONG_SEQUENCE:			
	case LONG_START_SEQUENCE:			
	case LONG_STOP_SEQUENCE:			
	for(i_ch=0; i_ch<index->numChannel; i_ch++){
	   {

	     for(ii=0;ii<index->block_size_samples-1;ii++){
	     tmp_spect[ii] = fabs(0.5*(fabs(spectrum[ii+i_ch*index->block_size_samples])+
				       fabs(spectrum[ii+i_ch*index->block_size_samples+1])))/
		 sqrt(sqrt(sqrt(0.5*(fabs(spectrum_org[ii+i_ch*index->block_size_samples])+
				     fabs(spectrum_org[ii+i_ch*index->block_size_samples+1])))+0.001));
	     }

	     tmp_spect[index->block_size_samples-1]=fabs(spectrum[index->block_size_samples-1+i_ch*index->block_size_samples])/
		 sqrt(sqrt(sqrt(fabs(spectrum_org[index->block_size_samples-1+i_ch*index->block_size_samples])+0.001)));
	  
	   {
	   
	   
	   for(ind=0; ind<4; ind++){
             float ftmp;
	     ftmp = (float)(index->nttDataScl->ac_top[iscl][i_ch][ind]) 
                  - (float)(index->nttDataScl->ac_btm[iscl][i_ch][ind]);
	     ftmp *=(float)(index->block_size_samples);
	     qsample=(int)ftmp;
	     ftmp = (float)(index->nttDataScl->ac_btm[iscl][i_ch][ind]);
	     ftmp *=(float)(index->block_size_samples);
             bias = (int)ftmp;
	  
	     for(acc=0.0, ii=bias;ii<bias+qsample;ii++){
		  acc+= (tmp_spect[ii]*tmp_spect[ii]);
	     }
	     if(ind ==0)      err[ind] = acc *mat_wei[0];
	     else if(ind ==1) err[ind] = acc *mat_wei[1];
	     else if(ind ==2) err[ind] = acc *mat_wei[2];
	     else if(ind ==3) err[ind] = acc *mat_wei[3];
	    }
	   }
	     err_max=0.0;
	    smin[i_ch]= smax[i_ch] = prev[iscl][i_ch];
	    if(prev_pow[iscl][i_ch]*10. < (err[0]+err[1]+err[2]+err[3])){
	     smin[i_ch] = prev[iscl][i_ch]-1; if(smin[i_ch]<0) smin[i_ch]=0;
	     smax[i_ch] = prev[iscl][i_ch]+1; if(smax[i_ch]>3) smax[i_ch]=3;
	    }
	    if((prev_pow[iscl][i_ch]*30. < (err[0]+err[1]+err[2]+err[3]))||
	       (prev_pow[iscl][i_ch] > (err[0]+err[1]+err[2]+err[3]))){
	     smin[i_ch] = 0;
	     smax[i_ch] = 3;
	    }
	    for(ii=smin[i_ch];ii<=smax[i_ch];ii++){
		  if(err_max<err[ii]){
		       err_max=err[ii];
		       mat_shift[i_ch]=ii;
		  }
	     }
	   prev[iscl][i_ch]= mat_shift[i_ch];
	   prev_pow[iscl][i_ch]= prev_pow[iscl][i_ch]*0.6
	   +(err[0]+err[1]+err[2]+err[3])*0.4;
	  }
	}
	     break;
	case EIGHT_SHORT_SEQUENCE:
	  for(i_ch=0; i_ch<index->numChannel; i_ch++)
	  {  int isf;  

	    for(ii=0;ii<index->block_size_samples;ii++){
	     tmp_spect[ii] = fabs(spectrum[ii+i_ch*index->block_size_samples])/
		 sqrt(sqrt(sqrt((fabs(spectrum_org[ii+i_ch*index->block_size_samples])+0.001))));
	     }

	   for(ind=0; ind<4; ind++){
             float ftmp;
	     ftmp = (float)(index->nttDataScl->ac_top[iscl][i_ch][ind]) 
                  - (float)(index->nttDataScl->ac_btm[iscl][i_ch][ind]);
	     ftmp *=(float)(index->block_size_samples/8);
	     qsample=(int)ftmp;
	     ftmp = (float)(index->nttDataScl->ac_btm[iscl][i_ch][ind]);
	     ftmp *=(float)(index->block_size_samples/8);
             bias = (int)ftmp;
             acc =0.0;
	     for(isf=0; isf<8; isf++){
	       for(ii=bias;ii<bias+qsample;ii++){
		  acc+= (tmp_spect[ii+isf*index->block_size_samples/8]*
			 tmp_spect[ii+isf*index->block_size_samples/8]);
	       }
             }
	     if(ind ==0)      err[ind] = acc *mat_wei[0];
	     else if(ind ==1) err[ind] = acc *mat_wei[1];
	     else if(ind ==2) err[ind] = acc *mat_wei[2];
	     else if(ind ==3) err[ind] = acc *mat_wei[3];
	   }
	    smin[i_ch]= smax[i_ch] = prev[iscl][i_ch];
	    if(prev_pow[iscl][i_ch]*10. < (err[0]+err[1]+err[2]+err[3])){
	     smin[i_ch] = prev[iscl][i_ch]-1; if(smin[i_ch]<0) smin[i_ch]=0;
	     smax[i_ch] = prev[iscl][i_ch]+1; if(smax[i_ch]>3) smax[i_ch]=3;
	    }
	    if((prev_pow[iscl][i_ch]*30. < (err[0]+err[1]+err[2]+err[3]))||
	       (prev_pow[iscl][i_ch] > (err[0]+err[1]+err[2]+err[3]))){
	     smin[i_ch] = 0;
	     smax[i_ch] = 3;
	    }
	    err_max= err[smin[i_ch]];
	    for(ii=smin[i_ch];ii<=smax[i_ch];ii++){
		  if(err_max<=err[ii]){
		       err_max=err[ii];
		       mat_shift[i_ch]=ii;
		  }
	     }
	   prev[iscl][i_ch]= mat_shift[i_ch];
	   prev_pow[iscl][i_ch]= prev_pow[iscl][i_ch]*0.6
	   +(err[0]+err[1]+err[2]+err[3])*0.4;
/*
fprintf(stderr, "SSSSS %5d %5d %10.3e %10.3e %10.3e %10.3e %10.3e\n",
iscl, mat_shift[iscl][i_ch], err_max, err[0], err[1], err[2], err[3]);
*/
	}
	     break;
	     break;
        default:
             fprintf( stderr,"Fatal error! %d: no such window type.\n", index->w_type );
             exit(1);
	}
	}
	return(0);

} /** end fix_or_flex **/

void mat_scale_tf_quantize_spectrum(double spectrum[],
                                    double lpc_spectrum[],
                                    double bark_env[],
                                    double pitch_sequence[],
                                    double gain[],
                                    double perceptual_weight[],
                                    ntt_INDEX *index_scl,
				    int    mat_shift[ntt_N_SUP_MAX],
                                    int       iscl)
{
  /*--- Variables ---*/
  int    ismp;
  int    nfr = 0;
  int    nsf = 0;
  int    n_can = 0;
  int    vq_bits = 0;
  int    cb_len_max= 0;
  int    subtop, isf, i_ch, subtopq;
  double add_signal[ntt_T_FR_MAX];
  double *sp_cv0 = NULL;
  double *sp_cv1 = NULL;
  double spec_buf[ntt_T_FR_MAX],lpc_buf[ntt_T_FR_MAX],bark_buf[ntt_T_FR_MAX],
         wei_buf[ntt_T_FR_MAX];
  int qsample[MAX_TIME_CHANNELS], bias[MAX_TIME_CHANNELS];
  float ftmp;

  /*--- Parameter settings ---*/
  switch (index_scl->w_type){
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* available bits */
    vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
    /* codebooks */
    sp_cv0 = index_scl->nttDataScl->ntt_codev0_scl;
    sp_cv1 = index_scl->nttDataScl->ntt_codev1_scl;
    cb_len_max = ntt_CB_LEN_READ_SCL + ntt_CB_LEN_MGN;
    /* number of pre-selection candidates */
    n_can = ntt_N_CAN_SCL;
    /* frame size */
    nfr = index_scl->block_size_samples;
    nsf = index_scl->numChannel;
    /* additional signal */
    for (ismp=0; ismp<index_scl->block_size_samples*index_scl->numChannel; ismp++){
      add_signal[ismp] = pitch_sequence[ismp] / lpc_spectrum[ismp];
  }
    break;
 case EIGHT_SHORT_SEQUENCE:
    /* available bits */
    vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
    /* codebooks */
    sp_cv0 = index_scl->nttDataScl->ntt_codev0s_scl;
    sp_cv1 = index_scl->nttDataScl->ntt_codev1s_scl;
    cb_len_max = ntt_CB_LEN_READ_S_SCL + ntt_CB_LEN_MGN;
    /* number of pre-selection candidates */
    n_can = ntt_N_CAN_S_SCL;
    /* number of subframes in a frame */
    nfr = index_scl->block_size_samples/8;
    nsf = index_scl->numChannel * ntt_N_SHRT;
    /* additional signal */
    ntt_zerod(index_scl->block_size_samples*index_scl->numChannel, add_signal);
    break;
 default:
    fprintf(stderr, "ntt_sencode(): %d: Window mode error.\n", index_scl->w_type);
    exit(1);
}

  /*--- Vector quantization process ---*/
/* Base,Enh-1 : fix ,,,, Enh-2... : flex  980302 Tomokazu Ishikawa */
    ntt_zerod(index_scl->block_size_samples*index_scl->numChannel,spec_buf);
    ntt_setd(index_scl->block_size_samples*index_scl->numChannel,1.e5,lpc_buf);
    ntt_setd(index_scl->block_size_samples*index_scl->numChannel,1.e-1,bark_buf);
    ntt_setd(index_scl->block_size_samples*index_scl->numChannel,2.e3,wei_buf);
    for(i_ch = 0; i_ch<index_scl->numChannel; i_ch++){
      ftmp = (float)
	 (index_scl->nttDataScl->ac_top[iscl][i_ch][mat_shift[i_ch]])
	 -(float)
         (index_scl->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]);
      ftmp *= nfr;
      qsample[i_ch]=(int)ftmp;
      ftmp = (float)
         (index_scl->nttDataScl->ac_btm[iscl][i_ch][mat_shift[i_ch]]);
      ftmp *= nfr;
      bias[i_ch] = (int)ftmp;
    }
    for(isf=0;isf<nsf;isf++){
	subtop= isf*nfr;
        i_ch = subtop/index_scl->block_size_samples;
	subtopq= qsample[i_ch]*isf;
        ntt_movdd(qsample[i_ch],spectrum+subtop+bias[i_ch],spec_buf+subtopq);
        ntt_movdd(qsample[i_ch],lpc_spectrum+subtop+bias[i_ch],lpc_buf+subtopq);
        ntt_movdd(qsample[i_ch],bark_env+subtop+bias[i_ch],bark_buf+subtopq);
        ntt_movdd(qsample[i_ch],perceptual_weight+subtop+bias[i_ch],wei_buf+subtopq);
    }
        ntt_movdd(index_scl->block_size_samples*index_scl->numChannel,spec_buf,spectrum);
        ntt_movdd(index_scl->block_size_samples*index_scl->numChannel,lpc_buf,lpc_spectrum);
        ntt_movdd(index_scl->block_size_samples*index_scl->numChannel,bark_buf,bark_env);
        ntt_movdd(index_scl->block_size_samples*index_scl->numChannel,wei_buf,perceptual_weight);
                                
/* Fix or Flex 980302 Tomokazu Ishikawa*/
    ntt_vq_pn(qsample[0], nsf, vq_bits, n_can,
            sp_cv0, sp_cv1, cb_len_max,
            spectrum, lpc_spectrum, bark_env, add_signal, gain,
            perceptual_weight,
            index_scl->wvq, index_scl->pow);
}



/** moriya informed and add **/
void ntt_scalebase_enc_lpc_spectrum  /* Takehiro Moriya*/
       (/* Parameters */
       int    nfr,
       int    nsf,
       int    block_size_samples,
       int    n_ch,
       int    n_pr,
       double *lsp_code,
       double *lsp_fgcode,
        int    *lsp_csize,
        int    *lsp_cdim,
        int    prev_lsp_code[],
	/*
	double prev_buf[MAX_TIME_CHANNELS][ntt_MA_NP][ntt_N_PR_MAX+1],
        */
        int    ma_np,
	/* Input */
        double spectrum[],
        int    w_type,
        int    current_block,
        /* Output */
        int    index_lsp[MAX_TIME_CHANNELS][ntt_LSP_NIDX_MAX],
        double lpc_spectrum[],
        double band_lower,
        double band_upper,
	double *cos_TT
        )
{
       /*--- Variables ---*/
       double resid, alf[MAX_TIME_CHANNELS][ntt_N_PR_MAX+1],
       lsp[ntt_N_PR_MAX+1], lspq[MAX_TIME_CHANNELS*(ntt_N_PR_MAX+1)];
       double spectrum_wide[ntt_T_FR_MAX],
              lpc_spectrum_re[ntt_T_FR_MAX];
       int    i_sup, top, subtop, w_type_t, isf, lsptop, ismp;
       int    block_size, nfr_l, nfr_u, nfr_lu;
       float ftmp;
       int ftmp_i;

       block_size = nfr * nsf;
       ftmp = (float)band_lower;
       ftmp *= (float)nfr;
       nfr_l = (int)ftmp;
       ftmp = (float)band_upper-(float)band_lower;
       ftmp *= (float)nfr;
       nfr_lu = (int)ftmp;
       nfr_u = nfr_l+nfr_lu;
       /*--- encode LSP coefficients ---*/
       if (current_block == ntt_BLOCK_LONG) w_type_t = w_type;
       else                                 w_type_t = ONLY_LONG_SEQUENCE;

       ftmp_i = (int)((band_upper-band_lower)*16384.0);
       ftmp_i = (16384*16384)/ftmp_i;
       for ( i_sup=0; i_sup<n_ch; i_sup++ ){
	 int fftmp_i;
	 top    = i_sup*block_size;
         ntt_zerod(block_size, spectrum_wide+top);
         ntt_setd(block_size,1.e5, lpc_spectrum+top);
  
         /*--- band extension for nallow band lpc (by A.Jin 1997.6.9) ---*/
         for(isf=0; isf<nsf; isf++){
           subtop = top + isf*nfr;
	   for(ismp=nfr_l; ismp<nfr_l+nfr_lu; ismp++){
            fftmp_i = ftmp_i * (ismp-nfr_l);
	    fftmp_i /= 16384;
	    spectrum_wide[fftmp_i+subtop]=spectrum[ismp+subtop];
           }
         }
   
         /* calculate LSP coefficients */
         ntt_scale_fgetalf(nfr, nsf, block_size_samples,
			   spectrum_wide+top,alf[i_sup],&resid,
                           n_pr, ntt_BAND_EXPN, cos_TT);
	 ntt_alflsp(n_pr, alf[i_sup], lsp);
         /* quantize LSP coefficients */
	 ntt_lsp_encw(n_pr, (double (*)[ntt_N_PR_MAX])lsp_code,
                      (double (*)[ntt_MA_NP][ntt_N_PR_MAX])lsp_fgcode,
                      lsp_csize,
                      prev_lsp_code+i_sup*ntt_LSP_NIDX_MAX,
		      /*prev_buf[i_sup], */
		      ma_np, 
                      lsp, lspq+i_sup*n_pr,
                      index_lsp[i_sup], ntt_LSP_SPLIT);
    }
   
       /*--- reconstruct LPC spectrum ---*/
       for (i_sup=0; i_sup<n_ch; i_sup++){
         top = i_sup * block_size;
         lsptop = i_sup * n_pr;
         ntt_lsptowt(nfr, n_pr, block_size_samples,
		     lspq+lsptop, lpc_spectrum+top, cos_TT);
       /*--- band re-extension for nallow band lpc (by A.Jin 1997.6.9) ---*/
       ftmp_i = (int)((band_upper-band_lower)*16384.0);
       ftmp_i = (16384*16384)/ftmp_i;
       for(ismp=0; ismp<nfr_lu; ismp++){
            int fftmp_i;
	    fftmp_i = ftmp_i*ismp; 
	    fftmp_i /= 16384;
	    lpc_spectrum_re[ismp+top+nfr_l] = lpc_spectrum[fftmp_i+top];
       }
         ntt_movdd(nfr_lu,lpc_spectrum_re+top+nfr_l, lpc_spectrum+top+nfr_l);
         ntt_setd(nfr_l, 999., lpc_spectrum+top);
         ntt_setd(nfr- nfr_u, 999., lpc_spectrum+top+nfr_u);
   
         for (isf=1; isf<nsf; isf++){
           subtop = top + isf*nfr;
           ntt_movdd(nfr, lpc_spectrum+top, lpc_spectrum+subtop);
      }
    }
}
