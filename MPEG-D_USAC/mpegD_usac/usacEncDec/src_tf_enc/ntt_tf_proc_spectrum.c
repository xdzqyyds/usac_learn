/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
/*   Takehiro Moriya (NTT) on 2000-08-11,                                    */
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


/* 25-aug-1997  NI  added bandwidth control */
/* 01-oct-1997  HP  "iscount++" bug fix in line 627 & 631 */
/*                  (as reported by T. Moriya, NTT) */
/* 14-oct-1997  TM  mods by Takehiro Moriya (NTT) */


#include <stdio.h>
#include <math.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_encode.h"
#include "ntt_tools.h"
#include  "ntt_relsp.h"
#include "ntt_enc_para.h"

void ntt_tf_proc_spectrum(double spectrum[],       /* spectrum */
			  ntt_INDEX  *index,       /* code indices*/
			  /* Output */
			  double lpc_spectrum[],   /* LPC spectrum */
			  double bark_env[],       /* Bark-scale envelope*/
			  double pitch_sequence[],/*periodic peak components*/
			  double gain[])          /* gain factor */
{
  double tc[ntt_T_FR_MAX], pwt[ntt_T_FR_MAX];
  int    current_block = 0;
  int    i_ch, top;
  
  /*--- Parameters ---*/
  double band_lower, band_upper;
  int    nfr = 0;
  int    nsf = 0;
  int    n_ch = 0;
  int    n_crb, *bark_tbl;
  int    fw_ndiv, fw_length, fw_len_max, fw_cb_size;
  double fw_prcpt;
  double *fw_codev;
  int    n_pr;
  int    ifr, isf, subtop, ismp, block_size;
  double *lsp_code;
  double *lsp_fgcode;
  int    lsp_csize[2], lsp_cdim[2];
  int    flag_blim_l;
  int    p_w_type;
 /*--- Set parameters ---*/
  n_crb = 0;
  bark_tbl = index->nttDataBase->ntt_crb_tbl;
  fw_ndiv=fw_length=fw_len_max=fw_cb_size=0;
  fw_prcpt = 0.0;
  fw_codev = index->nttDataBase->ntt_fwcodev;
  
  switch(index->w_type){
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* subframe */
    nfr = index->block_size_samples;
    nsf = 1;
    n_ch = index->numChannel;
    /* Bark-scale table (Bark-scale envelope) */
    n_crb    = ntt_N_CRB;
    bark_tbl = index->nttDataBase->ntt_crb_tbl;
    /* quantization (Bark-scale envelope) */
    fw_ndiv = ntt_FW_N_DIV;
    fw_cb_size = ntt_FW_CB_SIZE;
    fw_length = ntt_FW_CB_LEN;
    fw_codev = index->nttDataBase->ntt_fwcodev;
    fw_len_max = ntt_FW_CB_LEN;
    fw_prcpt = 0.4;
    /* envelope calculation (Bark-scale envelope) */
    current_block = ntt_BLOCK_LONG;
    /* bandwidth control */
    flag_blim_l = 0;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* subframe */
    nfr = index->block_size_samples/8;
    nsf = ntt_N_SHRT;
    n_ch = index->numChannel;
    /* Bark-scale table (Bark-scale envelope) */
    fw_cb_size = 1;
    current_block = ntt_BLOCK_SHORT;
    /* bandwidth control */
    flag_blim_l = 1;
    break;
  default:
        fprintf( stderr,"Fatal error! %d: no such window type.\n", index->w_type );
        exit(1);
  }
  block_size = nfr*nsf;

  /* LPC spectrum */
  n_pr = ntt_N_PR;
  lsp_code   = index->nttDataBase->lsp_code_base;
  lsp_fgcode = index->nttDataBase->lsp_fgcode_base;
  lsp_csize[0] = ntt_NC0;
  lsp_csize[1] = ntt_NC1;
  lsp_cdim[0] = ntt_N_PR;
  lsp_cdim[1] = ntt_N_PR;

    p_w_type = index->nttDataBase->prev_w_type;
  
  /*--- Encoding tools ---*/
  /* LPC spectrum encoding */
  /** new version moriya informed **/
       ntt_scalebase_enc_lpc_spectrum(
                            nfr, nsf, 
			    index->block_size_samples,
			    n_ch, n_pr, lsp_code, lsp_fgcode,
                            lsp_csize, lsp_cdim, 
			    index->nttDataBase->prev_lsp_code,
			    /* prev_buf, */
			    index->nttDataBase->ma_np, 
                            spectrum, index->w_type, current_block,
                            index->lsp, lpc_spectrum,
                            index->nttDataBase->bandLower, 
			    index->nttDataBase->bandUpper,
			    index->nttDataBase->cos_TT);

 /* bandwidth control */
  flag_blim_l =0;
  for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * nsf * nfr;
    if ( index->bandlimit == 1){
      index->blim_h[i_ch] = ntt_BLIM_STEP_H/2;
      band_upper = 
	index->nttDataBase->bandUpper * 
	  (1. - (1.-ntt_CUT_M_H) *
	   (double)index->blim_h[i_ch]/(double)ntt_BLIM_STEP_H);
    }
    else{
      index->blim_h[i_ch] = 0;
      band_upper = index->nttDataBase->bandUpper;
    }
    if (( index->bandlimit == 1)) index->blim_l[i_ch] = 1;
    else                           index->blim_l[i_ch] = 0;
    band_lower = index->nttDataBase->bandLower 
	       + ntt_CUT_M_L * index->blim_l[i_ch];
    ntt_freq_domain_pre_process(nfr, nsf, band_lower, band_upper,
				spectrum+top, lpc_spectrum+top,
				spectrum+top, lpc_spectrum+top);
  }
  /* periodic peak components encoding */
  index->pitch_q =0;
  ntt_zerod(index->block_size_samples*index->numChannel, pitch_sequence);


if(index->tvq_ppc_enable){
    if((index->nttDataBase->ntt_VQTOOL_BITS > ntt_PIT_TBITperCH*index->numChannel*3)){
    ntt_enc_pitch(spectrum, lpc_spectrum, index->w_type,
		  index->numChannel,
		  index->block_size_samples,
		  index->isampf,
		  index->nttDataBase->bandUpper,
		  index->nttDataBase->ntt_codevp0,
		  index->nttDataBase->pleave0,
		  index->nttDataBase->pleave1,
		  index->pit, index->pls, index->pgain, pitch_sequence);
{ /*pitch_q*/
 double tmp[ntt_N_FR_MAX*MAX_TIME_CHANNELS];
 double dis1, dis2;
 for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * block_size;
    for(isf=0; isf<nsf; isf++){
      subtop = top + isf*nfr;
      /* make input data */
      for (ismp=0; ismp<nfr; ismp++){
       tmp[ismp+subtop] =
         spectrum[ismp+subtop] * lpc_spectrum[ismp+subtop]
           - pitch_sequence[ismp+subtop];
     }
    }
 }
      dis1 = 0.0;
      dis2 = 0.0;
 for (i_ch=0; i_ch<n_ch; i_ch++){
    top = i_ch * block_size;
    for(isf=0; isf<nsf; isf++){
      subtop = top + isf*nfr;
      for (ismp=0; ismp<nfr; ismp++){
       dis1 += tmp[ismp+subtop]*tmp[ismp+subtop];
       dis2 += pitch_sequence[ismp+subtop]*pitch_sequence[ismp+subtop];
      } 
    }
 }
 if(((dis1+dis2)/(index->nttDataBase->ntt_VQTOOL_BITS)
    <  dis2/ (ntt_PIT_TBITperCH*index->numChannel)) && 
    (index->nttDataBase->ntt_VQTOOL_BITS > ntt_PIT_TBITperCH*index->numChannel*3) ) index->pitch_q = 1;
 else {
    index->pitch_q =0;
    ntt_zerod(index->block_size_samples*index->numChannel, pitch_sequence);
 }


}  /* pitch_q*/ 
/**UUVV    index->pitch_q =0; */ 
/**UUVV    ntt_zerod(index->block_size_samples*index->numChannel, pitch_sequence);*/ 
  }
  else{
    ntt_zerod(index->block_size_samples*index->numChannel, pitch_sequence);
  }
} /* if flag_ppc */

/* Bark-scale envelope encoding */
 if(fw_cb_size==1){
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
 }
 else{
    ntt_enc_bark_env(nfr, nsf, n_ch, n_crb, bark_tbl,
                   index->nttDataBase->bandUpper, 
		   fw_ndiv, fw_cb_size, fw_length,
                   fw_codev, fw_len_max, fw_prcpt,
                   index->nttDataBase->prev_fw_code,
		   /*fw_env_memory, fw_pdot, */
		   &p_w_type,
                   spectrum, lpc_spectrum, pitch_sequence, current_block,
                   tc, pwt, index->fw, index->fw_alf, bark_env);
    if(index->w_type == EIGHT_SHORT_SEQUENCE)
	index->nttDataBase->prev_w_type = ntt_BLOCK_SHORT;
    else
	index->nttDataBase->prev_w_type = ntt_BLOCK_LONG;
  }
  /* gain encoding */
  ntt_enc_gain_t(nfr, nsf, n_ch, bark_env, tc, index->pow, gain);
}


void ntt_enc_bark_env(/* Parameters */
		      int    nfr,               /* block length */
		      int    nsf,               /* number of sub frames */
		      int    n_ch,
		      int    n_crb,
		      int    *bark_tbl,
		      double bandUpper,
		      int    ndiv,              /* number of interleave division */
		      int    cb_size,           /* codebook size */
		      int    length,
		      double *codev,            /* code book */
		      int    len_max,           /* codevector memory length */
		      double prcpt,
		      int    prev_fw_code[],
		      /*
		      double *env_memory,
		      double *pdot,
		      */
		      int    *p_w_type,
		      /* Input */
		      double spectrum[],        /* spectrum */
		      double lpc_spectrum[],    /* LPC spectrum */
		      double pitch_sequence[],  /* periodic peak components */
		      int    current_block,     /* block length type */
		      /* Output */
		      double tc[ntt_T_FR_MAX],  /* flattened spectrum */
		      double pwt[ntt_T_FR_MAX], /* perceptual weight */
		      int    index_fw[],        /* indices for Bark-scale enveope coding */
		      int    ind_fw_alf[],
		      double bark_env[])        /* Bark-scale envelope */
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
      ntt_fwpred(nfr, n_crb, bark_tbl, bandUpper, ndiv,
		 cb_size, length, codev, len_max, prcpt,
		 prev_fw_code+(i_ch*nsf+isf)*ndiv,
		 /* env_memory, pdot, */
		 pwt+subtop, bark_env+subtop, tc+subtop,
		 &index_fw[(i_ch*nsf+isf)*ndiv],
		 &ind_fw_alf[isf+i_ch*nsf], i_ch, pred_sw);
      
      pred_sw = 1;
    }
  }
  
  *p_w_type = current_block;
}

void ntt_enc_gaim(/* Input */
		  double power,      /* Power to be encoded */
		  /* Output */
		  int    *index_pow, /* Power code index */
		  double *gain,      /* Gain */
		  double *norm)      /* Power normalize factor */
{
  double mu_power, dmu_power, dec_power;
  
  /* encoding */
  mu_power = ntt_mulaw(power, ntt_SUB_AMP_MAX, ntt_MU);  /* mu-Law power */
  *index_pow = (int)floor((mu_power)/ ntt_SUB_STEP); /* quantization */

  /* local decoding */
  dmu_power = *index_pow * ntt_SUB_STEP +ntt_SUB_STEP/2.;
  dec_power = ntt_mulawinv(dmu_power, ntt_SUB_AMP_MAX, ntt_MU);
  *gain= dec_power/ ntt_SUB_AMP_NM;                       /* calculate gain */
  *norm = ntt_SUB_AMP_NM/(dec_power+0.1);                 /* calculate norm. */
}

void ntt_enc_gain(/* Input */
		  double power,      /* Power to be encoded */
		  /* Output */
		  int    *index_pow, /* Power code index */
		  double *gain,      /* Gain */
		  double *norm)      /* Power normalize factor */
{
    /*--- Variables --*/
    double mu_power, dmu_power, dec_power;

    mu_power = ntt_mulaw(power, ntt_AMP_MAX, ntt_MU);           /* mu-Law power */

    *index_pow = (int)floor((mu_power)/ ntt_STEP);  /* quantization */

    dmu_power = *index_pow * ntt_STEP + ntt_STEP/2.;        /* decoded mu-Law power */
    dec_power = ntt_mulawinv(dmu_power, ntt_AMP_MAX, ntt_MU);   /* decoded power */

    *gain= dec_power * ntt_I_AMP_NM;                    /* calculate gain */
    *norm = ntt_AMP_NM/(dec_power+0.1);                 /* calculate norm. */
}

void ntt_enc_gain_t(/* Parameters */
		    int    nfr,          /* block length */
		    int    nsf,          /* number of sub frames */
		    int    n_ch,
		    /* Input */
		    double bark_env[],   /* Bark-scale envelope */
                    /* In/Out */
		    double tc[],         /* flattened spectrum */
		    /* Output */ 
		    int    index_pow[],  /* gain code indices */
		    double gain[])       /* gain factor */
{
  int    ismp, top, subtop, isf, iptop, gtop, i_ch, block_size;
  double norm, pwr;
  double sub_pwr_tmp, sub_pwr[ntt_N_SHRT_MAX];
  double g_gain, g_norm;

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
      ntt_enc_gain(pwr,&index_pow[i_ch],&gain[i_ch],&g_norm);
    }
    else{
      /* global gain */
      iptop = (nsf+1)*i_ch;
      gtop  = nsf*i_ch;
      ntt_enc_gain(pwr,&index_pow[iptop],&g_gain,&g_norm);
      gain[gtop] = g_gain;

      /* subband gain */
      for(isf=0; isf<nsf; isf++){
	subtop = top + isf*nfr;
	ntt_enc_gaim(sub_pwr[isf]*g_norm, &index_pow[iptop+isf+1],
		     &gain[gtop+isf], &norm);
	gain[gtop+isf] *= g_gain;
	norm *= g_norm;
      }
    }
  }
}

void ntt_enc_gair_p(int  *index_pgain, /* Output: Power code index */
	      double powG,        /* Input : power of the weighted target */
	      double gain2,       /* Input : squared gain of the target */
	      double *g_opt,      /* In/Out: optimum gain */                
	      double nume,/*Input: cross power or the orig. and quant. resid.*/
	      double denom)       /* Input : power of the orig. regid */
{
    double norm;
    double acc2_2, g_opt_2, acc2;
    int index_tmp;
    double pgain_step;
    
    pgain_step   = ntt_PGAIN_MAX / ntt_PGAIN_NSTEP;

    /*--- gain re-quantization ---*/
    ntt_enc_pgain(*g_opt, &index_tmp, g_opt, &norm);
    *index_pgain = index_tmp;    
    acc2= powG*gain2 -2.* *g_opt*nume + *g_opt* *g_opt*denom;
    if(index_tmp != 0){
	g_opt_2= ntt_mulawinv((index_tmp-1)*pgain_step+pgain_step/2., ntt_PGAIN_MAX, ntt_PGAIN_MU);
	acc2_2= powG*gain2 -2.*g_opt_2*nume+g_opt_2*g_opt_2*denom;
	if(acc2_2 < acc2){
	    acc2= acc2_2;
	    *index_pgain = ntt_max(index_tmp -1, 0);
	    *g_opt= g_opt_2;
	    return; 
	}
    }
}



void ntt_enc_pgain(/* Input */
		   double power,      /* Power to be encoded */
		   /* Output */
		   int    *index, /* Power code index */
		   double *pgain,
		   double *norm)      /* Gain */
{
    /*--- Variables --*/
    double mu_power;
    int itmp;
    double pgain_step;
    
    pgain_step   = ntt_PGAIN_MAX / ntt_PGAIN_NSTEP;

    mu_power = ntt_mulaw(power, ntt_PGAIN_MAX, ntt_PGAIN_MU);           /* mu-Law power */
    itmp = (int)floor((mu_power)/ pgain_step);  /* quantization */
    *index = itmp;  /* quantization */
    
    ntt_dec_pgain( *index, pgain);
    *norm = 1./(*pgain+1.e-3);

}


void ntt_enc_pitch(/* Input */
	       double spectrum[],         /* spectrum */
	       double lpc_spectrum[],     /* LPC spectrum */
	       int    w_type,             /* block type */
	       int    numChannel,
	       int     block_size_samples,
	       int     isampf,
	       double bandUpper,
	       double *pcode,
	       short *pleave0,
	       short *pleave1,
	     /* Output */
	       int    index_pit[],        /* indices for periodic peak components coding */
	       int    index_pls[],        
	       int    index_pgain[],
	       double pitch_sequence[])   /* periodic peak components */


{
    /*--- Variables ---*/
    double tc[ntt_T_FR_MAX], pwt[ntt_T_FR_MAX];
    double pit_seq[ntt_T_FR_MAX];
    double pit_pack[ntt_N_FR_P_MAX*MAX_TIME_CHANNELS];
    double tc_pack[ntt_N_FR_P_MAX*MAX_TIME_CHANNELS];
    double wtbuf[ntt_T_FR_MAX];
    double pwtbuf[ntt_T_FR_MAX];
    double norm[MAX_TIME_CHANNELS], dtmp, dpow; 
    int    ismp, i_sup, top, ptop;
    double pgain[MAX_TIME_CHANNELS];
    double pgainq[MAX_TIME_CHANNELS];

    switch(w_type){
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
	/* LPC flattening */
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    for (ismp=0; ismp<block_size_samples; ismp++){
		tc[ismp+top] = spectrum[ismp+top] * lpc_spectrum[ismp+top];
	    }
	}
	/* pitch components extraction */
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    ptop = i_sup * ntt_N_FR_P;
            ntt_sear_pitch(tc+top, lpc_spectrum+top,
		    block_size_samples, isampf, bandUpper, &index_pit[i_sup]);
            ntt_extract_pitch(index_pit[i_sup], tc+top, block_size_samples,
		    isampf, bandUpper, tc_pack+ptop);
            ntt_extract_pitch(index_pit[i_sup], lpc_spectrum+top, 
		    block_size_samples, isampf, bandUpper, wtbuf+ptop);
	    dtmp = 1.e-3+ntt_dotdd(ntt_N_FR_P, tc_pack+ptop, tc_pack+ptop);
	    dpow = sqrt(dtmp/ntt_N_FR_P);
	    ntt_enc_pgain( dpow, &index_pgain[i_sup], &pgain[i_sup], &norm[i_sup]);
	    ntt_dec_pgain(index_pgain[i_sup], &pgainq[i_sup]);
	    for (ismp=0; ismp<block_size_samples; ismp++){
		pwt[ismp+top] = 1. / pow(lpc_spectrum[ismp+top], 0.5);
            }
	    ntt_extract_pitch(index_pit[i_sup], pwt+top, block_size_samples,
	         isampf, bandUpper, pwtbuf+ptop);
            ntt_extract_pitch(index_pit[i_sup],spectrum+top,block_size_samples, 
		 isampf, bandUpper, pit_pack+ptop);
	    dtmp = 1./(pgainq[i_sup]+1.e-5);
	    for (ismp=0; ismp<ntt_N_FR_P; ismp++){
	      pit_pack[ptop+ismp] *= dtmp;
	    }
            pgain[i_sup] = pow(pgain[i_sup] , ntt_GAMMA_CH);	
	}

	/*--- Quantization of the pitch value ---*/
	ntt_wvq_pitch(pit_pack, wtbuf, pwtbuf, pgain,
		  pcode, pleave0, pleave1, numChannel,
		  index_pls, pgainq, index_pgain);
	/*--- Making the pitch sequence ---*/
	ntt_dec_pit_seq(index_pit, index_pls, numChannel,
	       block_size_samples, isampf, bandUpper,
	       pcode, pleave1, pit_seq);
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
	    for(ismp=0; ismp<block_size_samples; ismp++){
		pitch_sequence[ismp+top] = pgainq[i_sup] * pit_seq[ismp+top];
	    }
	}
	break;
    case EIGHT_SHORT_SEQUENCE:
	ntt_zerod(block_size_samples*numChannel, pitch_sequence);
	break;
    }


}

void     ntt_extract_pitch(
	      /* Input */
	      int     index_pit, 
	      double  pit_seq[],
	      int     block_size_samples,
	      int     isampf,
	      double  bandUpper,
	      /* Output */
	      double  pit_pack[]) 
{
    /*--- Variables ---*/
    int	   i_smp;
    int npcount, iscount;
    int ii,jj;
    
    int bandUpper_i;
    int pitch_i;
    int tmpnp0_i, tmpnp1_i;
    int fcmin_i;
    int bandwidth_i;


    bandUpper_i =  (int)(bandUpper*16384.+0.1);
    fcmin_i=
     (int)( (float)block_size_samples/(float)isampf*0.2*1024.0 + 0.5);

    bandwidth_i = 8;
    if ( isampf == 8 ) bandwidth_i = 3;
    if ( isampf >= 11) bandwidth_i = 4;
    if ( isampf >= 22) bandwidth_i = 8;

        pitch_i = (int)(pow( 1.009792, (double)index_pit)*4096.0 + 0.5);
        pitch_i *= fcmin_i;
        pitch_i = pitch_i  / 256;
        if (bandwidth_i * bandUpper_i < 32768 ) {
            tmpnp0_i = pitch_i * 16384 / bandUpper_i;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i/ block_size_samples;
            npcount = tmpnp0_i / 16384;
        }
        else {
            tmpnp0_i = pitch_i * bandwidth_i;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i / block_size_samples;
            npcount = tmpnp0_i / 32768;
        }

        for (jj=0; jj<ntt_N_FR_P; jj++){
              pit_pack[jj] = 0.0;
        }
        iscount=0;

        for (jj=0; jj<npcount/2; jj++){
              pit_pack[iscount++] = pit_seq[jj];
        }
        
        for (ii=0; ii<(ntt_N_FR_P )&& (iscount<ntt_N_FR_P); ii++){
          tmpnp0_i = pitch_i*(ii+1);
          tmpnp0_i += 8192;
          i_smp = tmpnp0_i/16384;
          if(i_smp+(npcount-1)/2+1 < block_size_samples) {
             for (jj=-npcount/2; (jj<(npcount-1)/2+1) && (iscount < ntt_N_FR_P);
 jj++, iscount++){
                pit_pack[iscount] = pit_seq[jj+i_smp];
             }
           }
        }
}

void ntt_freq_domain_pre_process(/* Parameters */
				 int    nfr,              /* block length */
				 int    nsf,              /* number of sub frames */
				 double band_lower,
				 double band_upper,
				 /* Input */
				 double spectrum[],       /* spectrum */
				 double lpc_spectrum[],   /* LPC spectrum */
				 /* Output */
				 double spectrum_out[],   /* spectrum */
				 double lpc_spectrum_out[]) /* LPC spectrum */
{
  /*--- Variables ---*/
  int isf, subtop, start, stop, ismp, width;
  float ftmp;

  /*--- Band limit ---*/
  ftmp = (float)band_lower;
  ftmp *= (float)nfr;
  start =  (int)ftmp;
  ftmp = (float)band_upper-(float)band_lower;
  ftmp *= (float)nfr;
  width  = (int)ftmp;
  stop  = start+width;
  for (isf=0; isf<nsf; isf++){
    subtop = isf * nfr;
    /* lower cut */
    for (ismp=0; ismp<start; ismp++){
      spectrum_out[ismp+subtop] = 0.;
      lpc_spectrum_out[ismp+subtop] = 1.e5;
    }
    /* signal band */
    for (ismp=start; ismp<stop; ismp++){
      spectrum_out[ismp+subtop] = spectrum[ismp+subtop];
      lpc_spectrum_out[ismp+subtop] = lpc_spectrum[ismp+subtop];
    }
    /* upper cut */
    for (ismp=stop; ismp<nfr; ismp++){
      spectrum_out[ismp+subtop] = 0.;
      lpc_spectrum_out[ismp+subtop] = 1.e5;
    }
  }
}


void ntt_fw_div(int    ndiv,    /* Param. : number of interleave division */
		int    length,  /* Param. : codebook length */
		double prcpt,   /* Param. : perceptual weight */
		double env[],   /* Input  : envelope vector */
		double denv[],  /* Output : envelope subvector */
		double wt[],    /* Input  : weighting factor vector */
		double dwt[],   /* Output : weighting factor subvector */
		int    idiv)    /* Param. : division number */
{
  int ismp, ismp2;

  for ( ismp=0; ismp<length; ismp++ ){
    ismp2 = idiv + ismp * ndiv;
    denv[ismp] = env[ismp2];
    dwt[ismp] = pow(wt[ismp2], prcpt);
  }
}


void ntt_fw_sear(int    cb_size, /* Param  : codebook size */
		 int    length,  /* Param  : codevector length */
		 double *codev,  /* Param  : codebook */
		 int    len_max, /* Param  : codevector memory length */
		 double denv[],  /* Input  : envelope subvector */
		 double dwt[],   /* Input  : weight subvector */
		 int    *index,  /* Output : quantization index */
		 double *dmin)   /* Output : quant. weighted distortion */
{
  /*--- Variables ---*/
  double dtemp, ldmin;
  int    ismp, icb;
  double dist_buf;
  
  ldmin = 1.e50;
  for (icb=0; icb<cb_size; icb++){
    /*--- Calculate the weighted distortion measure ---*/
    dist_buf = 0.;
    for (ismp=0; ismp<length; ismp++){
      dtemp = denv[ismp] - codev[icb*len_max+ismp];
      dist_buf += dwt[ismp] * dtemp * dtemp;
    }
    
    /*--- Search for the optimum code vector ---*/
    if ( dist_buf < (ldmin) ){
      ldmin = dist_buf;
      *index = icb;
    }
  }
  *dmin = ldmin;
}


void ntt_fwalfenc(double alf,    /* Input  : AR prediction coefficients */
	      double *alfq,  /* Output : quantized AR pred. coefficients */
	      int *ind)      /* Output : the index */
{
    /*--- Variables ---*/
    double expand;
    
    /*--- Main operation ---*/
    expand = alf / (double) ntt_FW_ALF_STEP+0.5;
    *ind = ntt_min( (int)expand, ntt_FW_ARQ_NSTEP-1 );
    *ind = ntt_max(*ind, 0);
    *alfq = (double)*ind*ntt_FW_ALF_STEP;
}

void ntt_fwpred(int    nfr,  /* Param:  block size */
		int    n_crb,       /* Param:  number of Bark-scale band */
		int    *bark_tbl,   /* Param:  Bark-scale table */
		double  bandUpper,
		int    ndiv,        /* Param:  number of interleave division */
		int    cb_size,     /* Param:  codebook size */
		int    length,      /* Param:  codevector length */
		double *codev,      /* Param:  codebook */
		int    len_max,     /* Param:  codevector memory length */
		double prcpt,
		int     prev_fw_code[],
		/*double *penv_tmp, *//*Param:  memory for Bark-scale envelope*/
		/*double *pdot_tmp, *//*Param:  memory for env. calculation */
		double iwt[],       /* Input:  LPC envelope */
		double pred[],      /* Output : Prediction factor */
		double tc[],        /* Input:  Residual signal */
		int    index_fw[],  /* Output: Envelope VQ indices */
		int    *ind_fw_alf, /* Output: AR prediction coefficient index */
		int    i_sup,       /* Input:  Channel number */
		int    pred_sw)     /* Input:  prediction switch */
{
    /*--- Variables ---*/
    double        bwt[ntt_N_CRB_MAX], env[ntt_N_CRB_MAX];
    double        fwgain, alf, alfq;
    double        penv[ntt_N_CRB_MAX];
    double        envsave[ntt_N_CRB_MAX];
    register double acc, cdot, acc2, col0, col1, dtmp;
    int           top, top2, ismp, iblow, ibhigh, ib;
    int  n_crb_eff;
    float ftmp;

    /*--- Initialization ---*/
    top  = i_sup*nfr;
    top2 = i_sup*n_crb;
    ntt_fwex( prev_fw_code, ndiv, length, codev, len_max, penv );
    /*--- Calculate envelope ---*/
    iblow=0; fwgain=0.; ib=0;
     n_crb_eff=0;
     for (ib=0; ib<n_crb; ib++){
	ibhigh = bark_tbl[ib];
	/* reset the accumulations */
	acc  = 0.0;
	acc2 = 0.0;
	/* bandle the input coefficient lines into the bark-scale band */
	ftmp = (float)bandUpper;
	ftmp *= (float)nfr;
	for(ismp=iblow; (ismp<ibhigh) && (ismp<(int)ftmp); ismp++){
	    cdot =  tc[ismp]*tc[ismp];/* calc. power */
	    acc +=  cdot; /* acc. the current and previous power */
	    acc2 = acc2 + iwt[ismp];  /* accumulate the weighting factor */
	}
	env[ib] = sqrt(acc/((double)(ismp-iblow)+0.001)) + 0.1; /* envelope in bark*/
	bwt[ib] = acc2/((double)(ismp-iblow)+0.001); 	/* weighting factor in bark scale */
	if(env[ib]>0.1){
	  n_crb_eff++;
	  fwgain += env[ib];	/* accumulate the envelope power */
	}
	iblow = ibhigh;
    }
    /* gain to normalize the bark-scale envelope */
    if(n_crb_eff != 0){
	   fwgain = (double) n_crb_eff / fwgain;
    }
    else{
	  fwgain = 0.;
    }

    /*--- Normalize the envelope, and remove the offset ---*/
    for (ib=0; ib<n_crb_eff; ib++){
	env[ib] = env[ib]*fwgain - 1.0;
	envsave[ib]= env[ib];
    }
    for (ib=n_crb_eff; ib<n_crb; ib++){
	env[ib] = 0.0;
	envsave[ib]= env[ib];
    }

    /*--- Calculate the AR prediction coefficient and quantize it ---*/
    col0 = 0.1;
    col1 = 0.;
    if (pred_sw == 1){
      /* calculate auto correlation and cross correlation */
      for (ib=0; ib<n_crb; ib++){
	col0 += penv[ib]*penv[ib]*bwt[ib];
	col1 += penv[ib]*env[ib]*bwt[ib];
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
    for (ib=ndiv*length; ib<n_crb; ib++) env[ib] = 0.;
    for (ib=0, ismp=0; ib<n_crb; ib++){
	ibhigh = bark_tbl[ib];
	dtmp = ntt_max(env[ib]+alfq*penv[ib]+1., ntt_FW_LLIM);
	while(ismp<ibhigh){
	    pred[ismp] = dtmp;
	    ismp++;
	}
    }
    /*--- Store the current envelope ---
    ntt_movdd(n_crb, env, penv); */
}


void ntt_fwvq(int    ndiv,     /* Param  : number of interleave division */
	      int    cb_size,  /* Param  : codebook size */
	      int    length,   /* Param  : codevector length */
	      double *codev,   /* Param  : codebook */
	      int    len_max,  /* Param  : codevector memory length */
	      double prcpt,    /* Param  : perceptual control factor */
	      double env[],    /* Input  : the envelope */
	      double wt[],     /* Input  : the weighting factor */
	      int    index[])  /* Output : indexes */
{
    /*--- Variables ---*/
    int    idiv;
    double denv[ntt_FW_CB_LEN_MX], dwt[ntt_FW_CB_LEN_MX], dmin;

    /*--- Initialization ---*/

    /*--- Main operation ---*/
    for ( idiv=0; idiv<ndiv; idiv++ ){
	/*--- Divide the input frame into subframe ---*/
	ntt_fw_div(ndiv, length, prcpt,
		   env, denv, wt, dwt, idiv);
	/*--- search ---*/
	ntt_fw_sear(cb_size, length,
		    codev, len_max,
		    denv, dwt, &index[idiv], &dmin );

    }
}

void ntt_get_wegt(/* Parameters */
		  int n_pr,
                  /* Input */
		  double flsp[],
		  /* Output */
		  double wegt[])
{
    int    i;
    double tmp;
    double llimit, ulimit;
    
    llimit = 0.4/(double)n_pr;
    ulimit = 1.-0.4/(double)n_pr;

    tmp     = ( ( flsp[2]-PI*llimit )-1.0);
    wegt[1] = tmp * tmp * 10.+1.0;
    if (tmp > 0.0 ) wegt[1] = 1.0;

    for ( i=2; i<=n_pr-1; i++ ) {
	tmp     = -flsp[i-1]+flsp[i+1]- 1.0;
        wegt[i] = tmp * tmp * 10.+1.0;
        if(tmp > 0.0 ) wegt[i] = 1.0;
    }
    tmp = -flsp[n_pr-1]+PI*ulimit- 1.0;
    wegt[n_pr] = tmp* tmp * 10.+1.0;
    if (tmp > 0.0 ) wegt[n_pr] = 1.0;
}



void ntt_lsp_encw(int    n_pr,
                  double code[][ntt_N_PR_MAX],
                  double fgcode[][ntt_MA_NP][ntt_N_PR_MAX],
                  int    *csize,
                  int    prev_lsp_code[],
		  /*
		  double buf_prev[ntt_MA_NP][ntt_N_PR_MAX+1],
                  */
                  int    ma_np,
		  double freq[],     /* Input  : LSP parameters */
                  double freqout[],  /* Output : quantized LSP parameters */
                  int index[],       /* Output : the optimum LSP code index */
                  int  nsp)
{
  /*--- Variables ---*/
  int    j, vqword;
  double wegt[ntt_N_PR_MAX+1];
  double flsp[ntt_N_PR_MAX+1];
  double target_buf[ntt_N_MODE_MAX][ntt_N_PR_MAX];
  double fg_sum[ntt_N_MODE_MAX][ntt_N_PR_MAX];
  double out_vec[ntt_N_PR_MAX];
  int    i_mode;
  int ntt_isp[ntt_NSP_MAX+1];
  double  pred_vec[ntt_N_PR_MAX], buf_prev[ntt_N_PR_MAX+1];
  double  lspq[ntt_N_PR_MAX+1];

/*--- Initialization ---*/
  /* set the band-split table */
  ntt_set_isp(nsp, n_pr, ntt_isp);

  /*--- Make the codebook with the prediction ---*/
  for(i_mode=0; i_mode<ntt_N_MODE; i_mode++){
    for(j=0; j<n_pr; j++){
      fg_sum[i_mode][j] =1.0;
      if(ma_np==1) fg_sum[i_mode][j] -= fgcode[i_mode][0][j];
    }
  }
  for(j=0; j<n_pr; j++){ pred_vec[j] = 0.0; }

  ntt_redec(n_pr, prev_lsp_code, csize,  nsp, code,
		      fg_sum[0], pred_vec, lspq, buf_prev );

  ntt_movdd( n_pr, freq+1, flsp+1 );
  ntt_get_wegt(n_pr, flsp, wegt );
  for(j=1; j<nsp; j++) {
    wegt[ntt_isp[j]] *= 1.2;
    wegt[ntt_isp[j]+1] *= 1.2;
}

  for (i_mode=0; i_mode< ntt_N_MODE; i_mode++){
    for(j=0; j<n_pr; j++) target_buf[i_mode][j] = flsp[j+1];
    if(ma_np==1){
      for(j=0; j<n_pr; j++){
        target_buf[i_mode][j] -= fgcode[i_mode][0][j]*buf_prev[j];
      }
    }
    for(j=0; j<n_pr; j++)
      target_buf[i_mode][j] /= fg_sum[i_mode][j];
  }
  ntt_relspwed(n_pr, flsp, wegt, freqout, ntt_LSP_NSTAGE,
               csize, code, fg_sum,
               target_buf, out_vec, &vqword , index, nsp );
}


void ntt_lsptowt(/* Parameters */
		 int nfr,        /* block length */
		 int n_pr,
		 int block_size_samples,
		 /* Input */
		 double lsp[],
		 /* Output */
		 double wt[],
		 double *cos_TT)
{
    register double  a,b,c,d,cosz2;
    double sa, sb,sc,sd;
    double coslsp[ntt_N_PR_MAX];
    int i, mag;
    double *lspp, *wp1, *wp2, *cp;

    mag =2*block_size_samples/nfr;
    for(i=0; i<n_pr; i++) coslsp[i] =  2.* cos(lsp[i+1]);

    wp1= wt;
    wp2= wt+nfr-1;
    cp = cos_TT+mag;
    switch(n_pr){
    case 20 :
	 for(i=0;i<nfr/2;i++) { 
           lspp = coslsp;
	   cosz2=2. * *cp;
	   a = (cosz2 - *lspp);
	   c = (cosz2 + *(lspp++));
	   b = (cosz2 - *lspp);
	   d = (cosz2 + *(lspp++));
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 sa=a*a; sb=b*b; sc=c*c; sd=d*d;
	 *(wp1++) = sa+sb +  (sa-sb)* *cp;
	 *(wp2--) = sc+sd + (-sc+sd)* *cp;
	 cp += mag*2;
      }
    break;
    case 16 :
	 for(i=0;i<nfr/2;i++) { 
           lspp = coslsp;
	   cosz2=2. * *cp;
	   a = (cosz2 - *lspp);
	   c = (cosz2 + *(lspp++));
	   b = (cosz2 - *lspp);
	   d = (cosz2 + *(lspp++));
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 { a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++)); }
	 sa=a*a; sb=b*b; sc=c*c; sd=d*d;
	 *(wp1++) = sa+sb +  (sa-sb)* *cp;
	 *(wp2--) = sc+sd + (-sc+sd)* *cp;
	 cp += mag*2;
      }
    break;
    default:
	 for(i=0;i<nfr/2;i++) { 
           lspp = coslsp;
	   cosz2=2. * *cp;
	   a = (cosz2 - *lspp);
	   c = (cosz2 + *(lspp++));
	   b = (cosz2 - *lspp);
	   d = (cosz2 + *(lspp++));
         while(lspp<coslsp+n_pr)     /* n_pr should be even */ 
	 {
	   a *= (cosz2 - *lspp);
	   c *= (cosz2 + *(lspp++));
	   b *= (cosz2 - *lspp);
	   d *= (cosz2 + *(lspp++));
	 }
	 sa=a*a; sb=b*b; sc=c*c; sd=d*d;
	 *(wp1++) = sa+sb +  (sa-sb)* *cp;
	 *(wp2--) = sc+sd + (-sc+sd)* *cp;
	 cp += mag*2;
     }
   }
}



/***************************************/
/* quantization in ntt_mulaw-scale         */
/* coded by K. Mano    28/Jul/1988     */
/***************************************/

#define DISP_OVF 0

double ntt_mulaw(double x, double xmax,double mu) /* Input */
{
   double  y;
   if (x>xmax){
#if (DISP_OVF)
       fprintf(stderr, "ntt_mulaw(): %lf, %lf: Input overflow.\n", x, xmax);
#endif
       x=xmax;
   }
   else if ( x <=0.0){
       x = 0.0; 
   }
   y = xmax*log10(1.+mu*fabs(x)/xmax)/log10(1.+mu); 
   if (x<0.) y= -y;
   return(y);
}


void ntt_opt_gain_p(
	      int    i_div,    /* Input : division number */
	      int    cb_len,   /* Param. : codebook length */
	      int    numChannel,
	      double gt[],     /* Input : gain */
	      double d_targetv[], /* Input : target vector */
	      double sig_l[],  /* Input : decoded residual */
	      double d_wt[],   /* Input : weight */
	      double nume[],   /* Output: cross power (total)*/
	      double gain[],   /* Input : gain of the residual */
	      double denom[],  /* Output: decoded resid. power without gain */
	      double nume0[],  /* Output: cross power (divided) */
	      double denom0[], /* Output: decoded resid. power without gain */
              short *pleave0, /*ntt_SMP_ACCp*/
              short *pleave1) /*ntt_BIT_REVp*/

{
    double fact[MAX_TIME_CHANNELS], acc;
    int i_smp, point, points, serial;
     
    /*serial = ntt_SMP_ACCp[i_div];  */
      serial = *(pleave0+i_div); 
   if( numChannel ==1){
	points = 0;
	for(i_smp=0; i_smp<cb_len; i_smp++){
	    acc =  gain[points]*d_targetv[i_smp]*sig_l[i_smp] ;
	    nume[points] +=  acc;
	    nume0[i_div] +=  acc;
	    acc = sig_l[i_smp]*sig_l[i_smp]*d_wt[i_smp];
	    denom[points] += acc;
	    denom0[i_div] += acc;
	}
    }
    else{
	for(points=0; points<numChannel; points++)
	    fact[points] = 1./(gt[points])/(gt[points]);
	for(i_smp=0; i_smp<cb_len; i_smp++){
	    /* point = ntt_BIT_REVp[serial++]; */
	    point = pleave1[serial++];
	    points = (point)/ntt_N_FR_P;
	    acc =  gain[points]*d_targetv[i_smp]*sig_l[i_smp]*fact[points] ;
	    nume[points] +=  acc;
	    nume0[i_div] +=  acc;
	    acc = sig_l[i_smp]*sig_l[i_smp]*d_wt[i_smp]*fact[points];
	    denom[points] += acc;
	    denom0[i_div] += acc;
	}
    }
}


void ntt_pre_dot_p(int    cb_len,      /* Param. : codebook length */
	       int    i_div,       /* Input : division number */
	       double targetv[],   /* Input : target vector */
	       double d_targetv[], /* Output: target subvector (weighted) */
	       double wt[],        /* Input : weighting vector */
	       double mwt[],       /* Input : perceptual controlled weight */
	       double gt[],        /* Input : gain */
	       double powG[],      /* Output: power of the weighted target */
	       double d_wt[],      /* Output: weighting subvector */
               int   numChannel,
	       short  *pleave0,
               short  *pleave1)

{
    int   i_smp, points, point, serial;
    register double acc, accb;
    
    /* Make divided target and weighting vector */
      serial = *(pleave0+i_div); 
    if(numChannel==1){
	points = 0;
	for ( i_smp=0; i_smp<cb_len; i_smp++ ) {
	    point = pleave1[serial++]; 
	    accb = mwt[point];
	    acc = targetv[point]* (wt[point] * accb);
	    accb *= gt[points];
	    d_targetv[i_smp] = acc *gt[points]*accb;
	    d_wt[i_smp] = accb*accb;
	    powG[points] += acc* acc;
	}
    }
    else{
	for ( i_smp=0; i_smp<cb_len; i_smp++ ) {
	    /*point = ntt_BIT_REVp[serial++]; */
	    point = pleave1[serial++]; 
	    points = (point) / ntt_N_FR_P;
	    accb = mwt[point];
	    acc = targetv[point]* (wt[point] * accb);
	    accb *= gt[points];
	    d_targetv[i_smp] = acc *gt[points]*accb;
	    d_wt[i_smp] = accb*accb;
	    powG[points] += acc* acc;
	}
    }
}


void ntt_relspwed(int    n_pr,
                  double lsp[ntt_N_PR_MAX+1],
                  double wegt[ntt_N_PR_MAX+1],
                  double lspq[ntt_N_PR_MAX+1],
                  int    nstage,
                  int    csize[],
                  double code[][ntt_N_PR_MAX],
                  double fg_sum[ntt_N_MODE_MAX][ntt_N_PR_MAX],
                  double target_buf[ntt_N_MODE_MAX][ntt_N_PR_MAX],
                  double out_vec[ntt_N_PR_MAX],
                  int    *vqword,
                  int    index_lsp[],
                  int    nsp)
{
  int      i, j, k, off_code, off_dim, off, code1, code2;
  double   dist, dmin, diff;
  int   kst, kend, index[ntt_N_MODE_MAX][NCD], index0;
  int   nbit0, nbit, k1;
  double dmin1, dmin2, tdist[NCD*ntt_N_MODE_MAX];
  int  tindex[NCD*ntt_N_MODE_MAX], mode_index, mode;
  double bufsave, rbuf[ntt_N_MODE_MAX][ntt_N_PR_MAX];
  double buf[ntt_N_PR_MAX], d[ntt_NC0_MAX];
  double lspbuf[ntt_N_PR_MAX+1];
  int i_mode, idim;
  int index_sp[ntt_NSP_MAX];
  register double acc, acc1;
  double *p_code, *p_rbuf;
/*
  int ntt_isp[ntt_NSP_MAX];
*/
  int ntt_isp[ntt_NSP_MAX+1];

  ntt_set_isp(nsp, n_pr, ntt_isp);
  for(i_mode=0; i_mode<ntt_N_MODE; i_mode++)
    ntt_movdd(n_pr, target_buf[i_mode], rbuf[i_mode]);
  off_code = 0;
  kst = 0;
  kend = nstage;
  off_dim = 0;
  off = csize[kst];
  nbit0 = (int)floor(log((double)csize[kst])/log(2.0));
  nbit  = (int)floor(log((double)csize[kend-1])/log(2.0) + 0.00001);
  for(mode = 0; mode<ntt_N_MODE; mode++){
    for(i=off_code; i<csize[kst]+off_code; i++){
      acc =0.0;
      acc1 =0.0;
      p_code = code[i];
      p_rbuf = rbuf[mode];
      for(j=off_dim; j<n_pr;j++){
        acc += *(p_rbuf++) * *(p_code);
        acc1+= *(p_code) * *(p_code);
        p_code++; 
      }
      d[i-off_code] =  -acc*2.0 + acc1;

  }
    for(j=0; j<NCD; j++){
      for(i=1, index[mode][j]=0, dmin= d[0]; i<csize[kst]; i++){
        if(d[i]<dmin){
          dmin=d[i];
          index[mode][j]=i;
      }
    }
      d[index[mode][j]]=1.e38;
  }
    for(k=0; k<NCD; k++){
      code1=off_code+index[mode][k];
      for(dmin1=1.e38, k1 = 0; k1<csize[kend-1]; k1++){
        code2=off_code+k1+off;
        for(j=ntt_isp[0]; j<ntt_isp[1]; j++){
          buf[j]=(code[code1][j]+code[code2][j]);
      }
        ntt_dist_lsp(ntt_isp[1], rbuf[mode], buf, wegt, &dist);
        if(dist<dmin1) { dmin1 = dist; index_sp[0] = k1; }
        for(j=ntt_isp[0]; j<ntt_isp[1]; j++){
          buf[j]=(code[code1][j]+code[off_code+index_sp[0]+off][j]);
      }
    }
      bufsave = buf[ntt_isp[1]-1];
      for(idim=1; idim<nsp-1; idim++){

        for(dmin2=1.e38, k1 = 0; k1<csize[kend-1]; k1++){
          code2=off_code+k1+off;
          buf[ntt_isp[idim]-1] =bufsave;

          for(j=ntt_isp[idim]; j<ntt_isp[idim+1]; j++){
            buf[j]=(code[code1][j]+code[code2][j]);
	}
          ntt_dist_lsp(ntt_isp[idim+1]-ntt_isp[idim],rbuf[mode]+ntt_isp[idim],
                       buf+ntt_isp[idim],wegt+ntt_isp[idim],&dist);
          if(dist<dmin2) { dmin2 = dist; index_sp[idim] = k1; }
      }
        for(j=ntt_isp[idim]; j<ntt_isp[idim+1]; j++){
          buf[j]=(code[code1][j]+code[off_code+index_sp[idim]+off][j]);
      }
        bufsave = buf[ntt_isp[idim+1]-1];
    }

      for(dmin2=1.e38, k1 = 0; k1<csize[kend-1]; k1++){
        code2=off_code+k1+off;
        buf[ntt_isp[nsp-1]-1] =bufsave;
        for(j=ntt_isp[nsp-1]; j<ntt_isp[nsp]; j++){
          buf[j]=(code[code1][j]+code[code2][j]);
      }
        ntt_dist_lsp(ntt_isp[nsp]-ntt_isp[nsp-1],rbuf[mode]+ntt_isp[nsp-1],
                     buf+ntt_isp[nsp-1],wegt+ntt_isp[nsp-1],&dist);
        if(dist<dmin2) { dmin2 = dist; index_sp[nsp-1] = k1; }
    }
      for(j=ntt_isp[nsp-1]; j<ntt_isp[nsp]; j++){
        buf[j]=(code[code1][j]+code[off_code+index_sp[nsp-1]+off][j]);
    }
      if(buf[0] < L_LIMIT) {
        diff = L_LIMIT - buf[0];
        buf[0] += diff*1.2;
    }
      if(buf[n_pr-1] > M_LIMIT ){
        diff = buf[n_pr-1]-M_LIMIT;
        buf[n_pr-1] -= diff*1.2;
    }

      ntt_check_lsp(n_pr-1, buf+1, MIN_GAP);

      lspbuf[0] = 0.0;
      for(j=0; j<n_pr; j++){
        lspbuf[j+1] = lsp[j+1] +
          (buf[j] - rbuf[mode][j] ) * fg_sum[mode][j] ;
    }
      if(lspbuf[1] < L_LIMIT) {
        diff = L_LIMIT - lspbuf[1];
        lspbuf[1] += diff*1.2;
    }
      if(lspbuf[n_pr] > M_LIMIT ){
        diff = lspbuf[n_pr]-M_LIMIT;
        lspbuf[n_pr] -= diff*1.2;
    }
      ntt_check_lsp(n_pr-1, lspbuf+2, MIN_GAP);
      ntt_check_lsp(n_pr-1, lspbuf+2, MIN_GAP*0.95);
      ntt_check_lsp_sort(n_pr-1, lspbuf+2);
      ntt_dist_lsp(n_pr, lsp+1, lspbuf+1, wegt, &dist);

      tdist[k+mode*NCD] = dist;
      tindex[k+mode*NCD] =  0;
      for(idim=0; idim<nsp; idim++){
        tindex[k+mode*NCD] +=(index_sp[idim]<<(nbit*(nsp-idim-1)));
    }
  }
}
  for(index0=0, mode_index=0, dmin= 1.e38, mode=0; mode<ntt_N_MODE ; mode ++){
    for(k=0; k<NCD; k++){
      if(tdist[k+mode*NCD]<dmin) {
        dmin = tdist[k+mode*NCD];
        index0 = k;
        mode_index = mode;
    }
  }
}
  *vqword = (mode_index<<(nbit*nsp+nbit0))
    | (index[mode_index][index0]<<(nbit*nsp))
      | tindex[index0+mode_index*NCD];
  index_lsp[0] = mode_index;
  index_lsp[1] = index[mode_index][index0];
  for(idim=0; idim<nsp; idim++){
    index_lsp[idim+2] =
      (tindex[index0+mode_index*NCD]>>(nbit*(nsp-idim-1)))&(csize[kend-1]-1) ;
}

  code1=off_code+index[mode_index][index0];
  for(idim=0; idim<nsp; idim++){
    for(j=ntt_isp[idim]; j<ntt_isp[idim+1]; j++){
      lspq[j+1] =
        (code[code1][j] +code[off_code+off+index_lsp[2+idim]][j]);
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

  for(j=off_dim; j<n_pr; j++) buf[j] = lspq[j+1];
  for(j=off_dim; j<n_pr; j++){
    lspq[j+1] = lsp[j+1] +
      (lspq[j+1] - rbuf[mode_index][j] ) *
        fg_sum[mode_index][j] ;
    out_vec[j] = buf[j];
}
  if(lspq[1] < L_LIMIT) {
    diff = L_LIMIT - lspq[1];
    lspq[1] += diff*1.2;
}
  if(lspq[n_pr] > M_LIMIT ){
    diff = lspq[n_pr]-M_LIMIT;
    lspq[n_pr] -= diff*1.2;
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
      /**/        fprintf(stderr,"%12.5f %5d %12.5f PERMUTATION \n",
                          diff, i, lsp[i+1]-lsp[i]);
      /**/
      lspq[i]   -= (diff+MIN_GAP*0.95)/2.;
      lspq[i+1] += (diff+MIN_GAP*0.95)/2.;
  }
}
  ntt_check_lsp_sort(n_pr-1, lspq+2);
}






void ntt_sear_ch_p(/* Parameters */
             int    cb_len,      /* codebook length */
	     int    pol_bits0,   /* number of polarity bits */
	     int    cb_size0,    /* codebook size */
	     int    n_can,       /* number of candidates */
             /* Input */
	     double d_targetv[], /* target subvector (weighted) */
	     double *codevv, /* codebook */
	     double d_wt[],      /* divided weight */
	     /* Output */
	     double can_code_targ[ntt_POLBITS_P][ntt_N_CAN_MAX],
	     double can_code_sign[ntt_POLBITS_P][ntt_N_CAN_MAX],
	     int    can_ind[])
{
    /*--- Variables ---*/
    int i_cb, i_pol, i_can, j_can, ismp, tb_top, spl_pnt, t_ind;
    double cros_max_t[ntt_N_CAN_MAX], dtmp;
    double code_targ[ntt_POLBITS][ntt_PIT_CB_SIZE_MAX], 
    code_targ_tmp[ntt_PIT_CB_SIZE_MAX];
    register double accb, accc;



    for (i_cb=0; i_cb<cb_size0; i_cb++){
       accc = 0.;
       accb = 0.;
      {
       double *p_code, *p_targ, *p_wt;
       p_code= codevv+i_cb*ntt_CB_LEN_P_MAX;
       p_targ=d_targetv;
       p_wt = d_wt;
       for (ismp=0; ismp<cb_len; ismp++){
           accb += *(p_targ++) * (*p_code);
           accc += *(p_wt++)  * *(p_code) * *(p_code);
           p_code++;
       }
      }
      code_targ[0][i_cb] = accb;
      if((pol_bits0 == 1)&&(accb<=0.0)){
           code_targ_tmp[i_cb] = -4.0*accb-accc;
      }
      else{
           code_targ_tmp[i_cb] =  4.0*accb-accc;
      }
    }

    /*--- pre-selection search ---*/
    if (n_can == ntt_PIT_CB_SIZE){ /* full search */
	i_pol=0;
	for( i_can=0; i_can<n_can; i_can++){
	    can_ind[i_can] = i_can;
	    can_code_targ[i_pol][i_can] = code_targ_tmp[i_can];
	    can_code_sign[i_pol][i_can] = 
		code_targ[i_pol][i_can] > 0. ? 1.0 : -1.0;
	}/* end 1st-ch preselection */
    }
    else{
	cros_max_t[0] = -1.e30;
	can_ind[0] = 0;
	tb_top=0;
	/*--- search for the insertion point ---*/
	for (i_cb=0; i_cb<cb_size0; i_cb++){
	    dtmp = code_targ_tmp[i_cb];
	    i_can=tb_top; j_can=0;
	    if (dtmp>cros_max_t[tb_top]){
		i_can=tb_top; j_can=0;
		while(i_can>j_can){
		    spl_pnt = (i_can-j_can)/2 + j_can;
		    if (cros_max_t[spl_pnt]>dtmp){
			j_can = spl_pnt+1;
		    }
		    else{
			i_can = spl_pnt;
		    }
		}
		tb_top = ntt_min(tb_top+1, n_can-1);
		for (j_can=tb_top; j_can>i_can; j_can--){
		    cros_max_t[j_can] = cros_max_t[j_can-1];
		    can_ind[j_can] = can_ind[j_can-1];
		}
		cros_max_t[i_can] = dtmp;
		can_ind[i_can] = i_cb;
	    }
	} /* end 1st-ch preselection */

	
	/*--- Make output ---*/
	i_pol = 0;
	for (i_can=0; i_can<n_can; i_can++){
	    t_ind = can_ind[i_can];
	    can_code_targ[i_pol][i_can] = code_targ_tmp[t_ind];
	    can_code_sign[i_pol][i_can] = 
		code_targ[i_pol][t_ind] > 0. ? 1.0 : -1.0;
	}
    }
}


#define ntt_GAMMA_P  0.05 

void ntt_sear_pitch(
		    /* Input */
		    double tc[],
		    double lpc_spectrum[],  /* LPC spectrum */
		    int   block_size_samples,
		    int   isampf,
		    double bandUpper,
		    /* Output */
		    int    *index_pit)
{
  /*--- Variables ---*/
  int ismp, ii, jj;
  int npcount, ifc;
  int  iscount;
  float pmax;
  double ddtmp;
  double tcbuf[ntt_N_FR_MAX];

  /*--- Calculate the cepstrum ---*/
    int fcmin_i, bandwidth_i;
    int tmpnp0_i, tmpnp1_i;
    int bandUpper_i;
    int pitch_i;
    int dtmp_i;

    bandUpper_i =  (int)(bandUpper*16384.+0.1);

    bandwidth_i = 8;
    if ( isampf == 8 ) bandwidth_i = 3;
    if ( isampf >= 11) bandwidth_i = 4;
    if ( isampf >= 22) bandwidth_i = 8;

    fcmin_i=
     (int)( (float)block_size_samples/(float)isampf*0.2*1024.0 + 0.5);
    for(ii=0; ii<block_size_samples; ii++)
     tcbuf[ii] = tc[ii]*tc[ii]*pow((lpc_spectrum[ii]+0.001), -ntt_GAMMA_P*2.0);

    pmax= (float)(-1.0e30);
    for (ifc=0; ifc<(1<<ntt_BASF_BIT); ifc++){

        pitch_i = (int)(pow( 1.009792, (double)ifc )*4096.0 + 0.5);
        pitch_i *= fcmin_i;
        pitch_i = pitch_i  / 256;
        if (bandwidth_i * bandUpper_i < 32768 ) {
            tmpnp0_i = pitch_i * 16384 / bandUpper_i;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i/ block_size_samples;
            npcount = tmpnp0_i / 16384;
        }
        else {
            tmpnp0_i = pitch_i * bandwidth_i;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i / block_size_samples;
            npcount = tmpnp0_i / 32768;
        }

        ddtmp = 0.0;
        iscount=0;
        for (jj=0; jj<npcount/2; jj++){
           iscount ++;
           ddtmp += tcbuf[jj];
        }
        for (ii=0; ii<(ntt_N_FR_P )&& (iscount<ntt_N_FR_P); ii++){
          dtmp_i = pitch_i*(ii+1);
          dtmp_i += 8192;
          ismp = dtmp_i/16384;
          if(ismp+(npcount-1)/2+1<block_size_samples){
	     for (jj=-npcount/2; (jj<(npcount-1)/2+1) && (iscount<ntt_N_FR_P);
	     jj++, iscount++){
	       ddtmp += tcbuf[ismp+jj];
	     }
          }
         }
         if(pmax<ddtmp){
            pmax=(float)ddtmp;
            *index_pit = ifc;
         }
 }

}


void ntt_sear_x_p(/* Parameters */
	    int    i_div,      /* division number */
	    int    cb_len,     /* codebook length */
	    int    pol_bits0,  /* polarity bits (0 ch) */
	    int    pol_bits1,  /* polarity bits (1 ch) */
	    int    can_ind0[], /* candidate indexes (0 ch) */
	    int    n_can0,     /* number of candidates (0 ch) */
	    int    can_ind1[], /* candidate indexes (1 ch) */
	    int    n_can1,     /* number of candidates (1 ch) */
            int    numChannel,
	    /* Output */
	    int    index[],    /* quantization index */
            /* Input */
	    double can_code_targ0[ntt_POLBITS_P][ntt_N_CAN_MAX], 
	    double can_code_targ1[ntt_POLBITS_P][ntt_N_CAN_MAX], 
	    double can_code_sign0[ntt_POLBITS_P][ntt_N_CAN_MAX], 
	    double can_code_sign1[ntt_POLBITS_P][ntt_N_CAN_MAX], 
	    double d_wt[],     /* weight subvector */
            double *pcode,
	    /* Output */
	    double sig_l[],    /* decoded residual */
	    double *dist_min)  /* quantization distortion */
{    
    /*--- Variables ---*/
    int i_pol, i_can, j_can, i_smp, index0, index1, i_can0, i_can1;
    double pol0[ntt_POLBITS_P], pol1[ntt_POLBITS_P];
    double cross, dist;
    double *pt_i, *pt_j;
    double weightbuf[ntt_CB_LEN_P_MAX];
    int n_can0_act, n_can1_act;
    double ttmp;
    
	i_can0= i_can1=0;
 	i_pol = 0;
    if (pol_bits1 == 1){
        *dist_min = 1.e50;
        n_can0_act = n_can0;
        n_can1_act = n_can1;
	for(i_can=0; i_can<n_can0_act; i_can++){
            ttmp = -can_code_targ0[i_pol][i_can];
	    pt_i = pcode + can_ind0[i_can]*ntt_CB_LEN_P_MAX;
	    for(i_smp=0; i_smp<cb_len; i_smp++){
	        weightbuf[i_smp] = 2. * *(pt_i++) * d_wt[i_smp]
		                 * can_code_sign0[i_pol][i_can];
	    }
	    for(j_can=0; j_can<n_can1_act; j_can++){
		/*--- Calculate the distortion measure ---*/
	        pt_j = pcode +(can_ind1[j_can]+ntt_PIT_CB_SIZE)
			       *ntt_CB_LEN_P_MAX;
		cross =0.0;
	        for(i_smp=0; i_smp<cb_len; i_smp++){
	            cross += weightbuf[i_smp]* *(pt_j++);
	        }
		cross *= can_code_sign1[i_pol][j_can];

        	dist = cross + ttmp - can_code_targ1[i_pol][j_can];

		/*--- Compare the distortion with the minimum ---*/
		if ( dist < *dist_min ){
		    i_can0 = i_can;
		    i_can1 = j_can;
		    *dist_min = dist;
		}
	    } /* j_can */
	}  /* i_can */
	
	/*--- Make output indexes ---*/
	index[i_div] = index0 = can_ind0[i_can0];
	index[i_div+ntt_N_DIV_PperCH*numChannel] = index1 = can_ind1[i_can1];
	if(can_code_sign0[0][i_can0] <= 0) 
	    index[i_div] += ((0x1)<<(ntt_MAXBIT_SHAPE_P));
	if(can_code_sign1[0][i_can1] <= 0) 
	    index[i_div+ntt_N_DIV_PperCH*numChannel] +=  
	     ((0x1)<<(ntt_MAXBIT_SHAPE_P));
	
    }
    /**/
    else {
	*dist_min = 1.e50;
        n_can0_act = n_can0;
        n_can1_act = n_can1;
        if(n_can0> 8){
	  i_can=0;
          while( (can_code_targ0[0][n_can0-1] >
                  can_code_targ1[0][n_can1-1-i_can]) &&(n_can1_act>8) ){
            i_can++;
            n_can1_act --;
          }
          while( (can_code_targ0[0][n_can0-1-i_can] <
                  can_code_targ1[0][n_can1-1]) && (n_can0_act>8) ){
            i_can++;
            n_can0_act --;
          }
        }
        for(i_can=0; i_can<n_can0_act; i_can++){
            ttmp = -can_code_targ0[i_pol][i_can];
	    pt_i = pcode +can_ind0[i_can]*ntt_CB_LEN_P_MAX;
	    for(i_smp=0; i_smp<cb_len; i_smp++){
	        weightbuf[i_smp] = 2. * *(pt_i++) * d_wt[i_smp]
		                 * can_code_sign0[i_pol][i_can];
	    }
	    for(j_can=0; j_can<n_can1_act; j_can++){
		
		/*--- Calculate the distortion measure ---*/
	        pt_j = pcode + (can_ind1[j_can]+ntt_PIT_CB_SIZE)
			       *ntt_CB_LEN_P_MAX;
		cross =0.0;
	        for(i_smp=0; i_smp<cb_len; i_smp++){
	            cross += weightbuf[i_smp]* *(pt_j++);
	        }
        	dist = cross + ttmp - can_code_targ1[i_pol][j_can];

		/*--- Compare the distortion with the minimum ---*/
		if ( dist < *dist_min ){
		    i_can0 = i_can;
		    i_can1 = j_can;
		    *dist_min = dist;
		}
	    } /* j_can */
	}  /* i_can */
	/*--- Make output indexes ---*/
	index[i_div] = index0 = can_ind0[i_can0];
	index[i_div+ntt_N_DIV_PperCH*numChannel] = index1 = can_ind1[i_can1];
	if(pol_bits0 ==1){
	    if(can_code_sign0[0][i_can0] <= 0) 
	    index[i_div] += ((0x1)<<(ntt_MAXBIT_SHAPE_P));
        }
    }
    
    
    /*--- Make the polarity indexes ---*/
    for(i_pol=0; i_pol<ntt_POLBITS_P; i_pol++){
	pol0[i_pol] =1- 2*((index[i_div] >> (i_pol+ntt_MAXBIT_SHAPE_P)) & 0x1);
	pol1[i_pol] =1- 2*((index[i_div+ntt_N_DIV_PperCH*numChannel]>>(i_pol+ntt_MAXBIT_SHAPE_P)) & 0x1);
    }

    /*--- Local decoding ---*/
    for(i_pol=0; i_pol<ntt_POLBITS_P; i_pol++){
	for ( i_smp=i_pol; i_smp<cb_len; i_smp +=ntt_POLBITS_P ){
	    sig_l[i_smp] = 
		(pol0[i_pol]*pcode[index0*ntt_CB_LEN_P_MAX+i_smp]
		 +pol1[i_pol]*
		  pcode[(index1+ntt_PIT_CB_SIZE)*ntt_CB_LEN_P_MAX+i_smp])*0.5;
	}
    }
}


#define EPS  1.e-10

void ntt_wvq_pitch(
	   double targetv[],     /* Input : target vector */
	   double wt[],          /* Input : weighting vector */
	   double pwt[],         /* Input : perceptual controlled weight */
	   double pgain[],       /* Input : gain */
	   double *pcode,
	   short *pleave0,
	   short *pleave1,
	   int    numChannel,
	   int    index_pls[],   /* Output: quantization indexes */
	   double pgainq[],      /* In/Out : gain */
	   int    index_pgain[]) /* Output: gain index */
{
    /*--- Variables ---*/
    int		i_div;
    double	d_targetv[ntt_CB_LEN_P_MAX], d_wt[ntt_CB_LEN_P_MAX];
    double	dist_min;
    int         can_ind0[ntt_N_CAN_MAX], can_ind1[ntt_N_CAN_MAX];
    double      can_code_targ0[ntt_POLBITS_P][ntt_N_CAN_MAX], 
		can_code_targ1[ntt_POLBITS_P][ntt_N_CAN_MAX];
    double      can_code_sign0[ntt_POLBITS_P][ntt_N_CAN_MAX];
    double      can_code_sign1[ntt_POLBITS_P][ntt_N_CAN_MAX];
    static double      gain2[MAX_TIME_CHANNELS];
    static int bits_p[ntt_N_DIV_P_MAX], length_p[ntt_N_DIV_P_MAX];
    int         cb_len, cb_size0, cb_size1;
    double   sig_l[ntt_CB_LEN_P_MAX];
    double   nume[MAX_TIME_CHANNELS], denom[MAX_TIME_CHANNELS];
    double   g_opt[MAX_TIME_CHANNELS];
    int      pol_bits0, pol_bits1;
    double   nume0[ntt_N_DIV_P_MAX], denom0[ntt_N_DIV_P_MAX];
    double   powG[MAX_TIME_CHANNELS];
    int      i_sup;
    double   acc_tmp;
    static double ntt_codevp1[ntt_PIT_CB_SIZE_MAX][ntt_CB_LEN_P_MAX];
    int index_pgain0;

    ntt_vec_lenp(numChannel, bits_p, length_p);
    for(i_sup=0; i_sup<numChannel; i_sup++){ 
	nume[i_sup] = denom[i_sup] = 0.;
	gain2[i_sup] = pgain[i_sup]*pgain[i_sup];
	powG[i_sup] = 0.0;
    }

    /*--- Main operation ---*/
    acc_tmp=0.0;
    for ( i_div=0; i_div<ntt_N_DIV_PperCH*numChannel; i_div++ ){
	nume0[i_div]= denom0[i_div] =0.0;
	/*--- set codebook lengths and sizes ---*/
	cb_len = length_p[i_div];
	cb_size0 = 0x1 << ntt_MAXBIT_SHAPE_P;
	cb_size1 = 0x1 << ntt_MAXBIT_SHAPE_P;
	pol_bits0 = ((bits_p[i_div]+1)/2)-ntt_MAXBIT_SHAPE_P;
	pol_bits1 = ((bits_p[i_div])/2)-ntt_MAXBIT_SHAPE_P;

	/*--- divide vectors into subvectors ---*/
	ntt_pre_dot_p(cb_len, i_div, targetv, d_targetv,
		wt, pwt, pgain, powG, d_wt, 
		numChannel, pleave0, pleave1);

	/*--- Best codebook search ---*/
	/* pre-selection 0 */
        ntt_sear_ch_p(cb_len,pol_bits0, cb_size0, ntt_N_CAN_P, d_targetv, 
	       pcode, d_wt,
	       can_code_targ0, can_code_sign0,
	       can_ind0);
	/* pre-selection 1 */
        ntt_sear_ch_p(cb_len,pol_bits1, cb_size1, ntt_N_CAN_P, d_targetv, 
	       pcode+ntt_PIT_CB_SIZE*ntt_CB_LEN_P_MAX, d_wt,
	       can_code_targ1, can_code_sign1,
	       can_ind1);
	/* main selection */
	ntt_sear_x_p(i_div, cb_len, pol_bits0, pol_bits1,
	       can_ind0, ntt_N_CAN_P, can_ind1, ntt_N_CAN_P, 
	       numChannel, index_pls,
	       can_code_targ0, can_code_targ1,
	       can_code_sign0, can_code_sign1, 
	       d_wt, pcode, sig_l, &dist_min);
	acc_tmp += dist_min;
	/* Prepair for calculating the optimum gain */
        ntt_opt_gain_p(i_div,cb_len, numChannel, 
		 pgain,d_targetv,sig_l,d_wt,nume,
                 pgainq, denom, nume0, denom0, pleave0, pleave1);
    }

    /*--- gain re-quantization ---*/
    for(i_sup=0; i_sup<numChannel; i_sup++){
	g_opt[i_sup] = nume[i_sup]/(denom[i_sup]+EPS); /* calc. opt. gain */
	/* re-quantization */
	ntt_enc_gair_p(&index_pgain0,
		 powG[i_sup], gain2[i_sup], &g_opt[i_sup],
		 nume[i_sup], denom[i_sup]);
	index_pgain[i_sup] = index_pgain0;
        ntt_dec_pgain(index_pgain[i_sup], &pgainq[i_sup]); 
    } /* end i_sup */
}
