/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
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
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_encode.h"
#include "all.h"


void ntt_tf_quantize_spectrum(double spectrum[],          /* Input : spectrum*/
			      double lpc_spectrum[],      /* Input : LPC spectrum*/
			      double bark_env[],          /* Input : Bark-scale envelope*/
			      double pitch_sequence[],    /* Input : periodic peak components*/
			      double gain[],              /* Input : gain factor*/
			      double perceptual_weight[], /* Input : perceptual weight*/
			      ntt_INDEX *ntt_index,           /* In/Out : VQ code indices */
			      int tvq_debug_level)
{
  /*--- Variables ---*/
  int    ismp;
  int    nfr = 0;
  int    nsf = 0;
  int    n_can = 0;
  int    vq_bits = 0;
  double add_signal[ntt_T_FR_MAX];
  double *sp_cv0 = NULL;
  double *sp_cv1 = NULL;
  int    cb_len_max = 0;
  int    isf;
  int nfrlim;
  float ftmp;

  /*--- Parameter settings ---*/
  switch (ntt_index->w_type){
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* available bits */
    vq_bits = ntt_index->nttDataBase->ntt_VQTOOL_BITS;
    /* codebooks */
    sp_cv0 = ntt_index->nttDataBase->ntt_codev0;
    sp_cv1 = ntt_index->nttDataBase->ntt_codev1;
    cb_len_max = ntt_CB_LEN_READ + ntt_CB_LEN_MGN;
    /* number of pre-selection candidates */
    n_can = ntt_N_CAN;
    /* frame size */
    nfr = ntt_index->block_size_samples;
    nsf = ntt_index->numChannel;
    /* additional signal */
    for (ismp=0; ismp<ntt_index->block_size_samples*ntt_index->numChannel; ismp++){
      add_signal[ismp] = pitch_sequence[ismp] / lpc_spectrum[ismp];
    }
    break;
    case EIGHT_SHORT_SEQUENCE:
    /* available bits */
    vq_bits = ntt_index->nttDataBase->ntt_VQTOOL_BITS_S;
    /* codebooks */
    sp_cv0 = ntt_index->nttDataBase->ntt_codev0s; 
    sp_cv1 = ntt_index->nttDataBase->ntt_codev1s;
    cb_len_max = ntt_CB_LEN_READ_S + ntt_CB_LEN_MGN;
    /* number of pre-selection candidates */
    n_can = ntt_N_CAN_S;
    /* number of subframes in a frame */
    nfr = ntt_index->block_size_samples/8;
    nsf = ntt_index->numChannel * ntt_N_SHRT;
    /* additional signal */
    ntt_zerod(ntt_index->block_size_samples*ntt_index->numChannel, add_signal);
    break;
  default:
    fprintf(stderr, "ntt_sencode(): %d: Window mode error.\n", ntt_index->w_type);
    exit(1);
  }
      ftmp = (float)(ntt_index->nttDataBase->bandUpper);
      ftmp *= (float)nfr;
      nfrlim= (int)ftmp;
  for(isf=1; isf<nsf; isf++){
      ntt_movdd(nfrlim,spectrum+isf*nfr, spectrum+isf*nfrlim);
      ntt_movdd(nfrlim, lpc_spectrum+isf*nfr, lpc_spectrum+isf*nfrlim);
      ntt_movdd(nfrlim,bark_env+isf*nfr, bark_env+isf*nfrlim);
      ntt_movdd(nfrlim, perceptual_weight+isf*nfr,perceptual_weight+isf*nfrlim);
      ntt_movdd(nfrlim,add_signal+isf*nfr, add_signal+isf*nfrlim);
  }

  /*--- Vector quantization process ---*/
     if(tvq_debug_level>6)
       fprintf(stderr, "UUUUU %5d \n", vq_bits);
     ntt_vq_pn( nfrlim,
	    nsf, vq_bits, n_can,
	    sp_cv0, sp_cv1, cb_len_max,
	    spectrum, lpc_spectrum, bark_env, add_signal, gain,
	    perceptual_weight,
	    ntt_index->wvq, ntt_index->pow);
   {
    int top, data_len, i_ch, numChannel, iptop, gtop;
    double g_opt[16], tmp, nume[16], denom[16],sig_l[2048], norm;
    double g_gain, g_norm, pwr, nume0[2], denom0[2], glgain[2];
    
    if(nsf==2 || nsf==16) numChannel =2;
    else numChannel =1;
    data_len = nfrlim * nsf;
    if(nsf>3){
      double dmu_power, dec_power;
      for (i_ch=0; i_ch<numChannel; i_ch++){
       iptop = i_ch * (nsf/numChannel + 1);
           dmu_power = ntt_index->pow[iptop] * ntt_STEP +ntt_STEP/2.;
           dec_power = ntt_mulawinv(dmu_power, ntt_AMP_MAX, ntt_MU);
	   glgain[i_ch] = dec_power  / ntt_AMP_NM;
      }
    }
    ntt_vex_pn(ntt_index->wvq, sp_cv0, sp_cv1, cb_len_max,
	     nsf, data_len, vq_bits, sig_l);
    nume0[0]=denom0[0]=0.;
    if(numChannel == 2) nume0[1]=denom0[1]=0.;
    for(isf=0; isf<nsf; isf++){
       nume[isf]=denom[isf]=0.0;
       top = isf*nfrlim;
       for(ismp=0; ismp<nfrlim; ismp++){
	  tmp=sig_l[top+ismp]*perceptual_weight[top+ismp]*
	      bark_env[ismp+top] / lpc_spectrum[ismp+top];
	  nume[isf]+= tmp*
	  (spectrum[top+ismp] -add_signal[top+ismp])
	  *perceptual_weight[top+ismp];
	  denom[isf] +=  tmp*tmp;
	 
/*
	  if(nsf >=8){
fprintf(stderr, "PPP %2d %3d %7.2f %8.2f %5.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n", 
	    isf, ismp, spectrum[top+ismp], sig_l[top+ismp],
	    perceptual_weight[ismp+top], 
	    tmp, tmp*spectrum[top+ismp]*perceptual_weight[top+ismp],
	    tmp*tmp, nume0[isf/8], denom0[isf/8]);
	  }
*/
       }
      if(nsf >=8){
	if(tvq_debug_level>6  )
	fprintf(stderr, "%5d %12.3f %12.3f opt_gainZZ\n", 
	isf, nume[isf]/(denom[isf]+0.0001)*glgain[isf/8], gain[isf]);
      }
       g_opt[isf] = nume[isf]/(denom[isf]+0.0001); 
      if(tvq_debug_level>6  )
	fprintf(stderr, "%5d %12.3f %12.3f opt_gainZZ\n", 
	isf, g_opt[isf], gain[isf]);
    }
    for(i_ch=0; i_ch<numChannel; i_ch++){
    if (nsf < 3){ /* long*/
      /* global gain */
      if(tvq_debug_level>6 )
         fprintf(stderr, "%12.3f %5d ->",   gain[i_ch],
         ntt_index->pow[i_ch]);
      ntt_enc_gain(g_opt[i_ch]*1024, &(ntt_index->pow[i_ch]),
      &gain[i_ch],&g_norm);
      if(tvq_debug_level>6 )
         fprintf(stderr, "%5d %12.3f  %5d ZZZZZlong \n", i_ch,  gain[i_ch],
         ntt_index->pow[i_ch]);
    }
    else{
      /* global gain */
      iptop = (8+1)*i_ch;
      gtop  = 8*i_ch;
      nume0[i_ch]=0.;
      denom0[i_ch]=0.0;
      for(pwr=0.,isf=0; isf<nsf/numChannel; isf++){
        nume0[i_ch]+= gain[isf+gtop]/glgain[i_ch]*nume[gtop+isf];
        denom0[i_ch]+= gain[isf+gtop]/glgain[i_ch]*gain[isf+gtop]/glgain[i_ch]
			 *denom[gtop+isf];
      }
      pwr = nume0[i_ch]/(denom0[i_ch]+0.001)*1024.;
	  
	  if(tvq_debug_level>6  )
          fprintf(stderr, "%5d %12.3f %5d index_pow[i_ch]ZZZZZ %12.3f %12.3f\n", 
             i_ch,  g_gain, ntt_index->pow[iptop], pwr, 
	     1024.*nume0[i_ch]/denom0[i_ch]);
      ntt_enc_gain(pwr,&(ntt_index->pow[iptop]),&g_gain,&g_norm);

	  if(tvq_debug_level>6  )
          fprintf(stderr, "%5d %12.3f  %5d index_pow[i_ch]ZZZZZQ %12.3f\n", 
             i_ch,  g_gain, ntt_index->pow[iptop], glgain[i_ch]);

      /* subframe gain */
      for(isf=0; isf<nsf/numChannel; isf++){
          if(tvq_debug_level>6)
          fprintf(stderr, "%5d %12.3f %5d %12.4f %12.4f zzzzzzzsub \n", 
               i_ch,  gain[isf+gtop], ntt_index->pow[iptop+isf+1], g_norm, g_gain);
       ntt_enc_gaim(g_opt[gtop+isf]/g_gain*1024.,&(ntt_index->pow[iptop+isf+1]),
                    &gain[gtop+isf], &norm);
       gain[gtop+isf] *= g_gain;
          if(tvq_debug_level>6)
          fprintf(stderr, "%5d %12.3f %5d %12.4f %12.4f zzzzzzzsub \n", 
               i_ch,  gain[isf+gtop], ntt_index->pow[iptop+isf+1], g_norm, g_gain);
      }
    }
   }
  } 
}

void ntt_div_vec(int nfr,                         /* Param. : block length */
		 int nsf,                         /* Param. : number of sub frames */
		 int cb_len,                      /* Param. : codebook length */
		 int cb_len0,                     /* Param.*/
		 int idiv,                        /* Param. */
		 int ndiv,                        /* Param. : number of interleave division */
		 double target[],                 /* Input */
		 double d_target[],               /* Output */
		 double weight[],                 /* Input */
		 double d_weight[],               /* Output */
		 double add_signal[],             /* Input */
		 double d_add_signal[],           /* Output */
		 double perceptual_weight[],      /* Input */
		 double d_perceptual_weight[])    /* Output */
{
  /*--- Variables ---*/
  int  icv, ismp, itmp;

  /*--- Parameter settings ---*/

  for (icv=0; icv<cb_len; icv++) {
    /*--- Set interleave ---*/
    if ((icv<cb_len0-1) &&
	((ndiv%nsf==0 && nsf>1) || ((ndiv&0x1)==0 && (nsf&0x1)==0)))
      itmp = ((idiv + icv) % ndiv) + icv * ndiv;
    else itmp = idiv + icv * ndiv;
    ismp = itmp / nsf + ((itmp % nsf) * nfr);
    /*--- Vector division ---*/
    d_target[icv] = target[ismp];
    d_weight[icv] = weight[ismp];
    d_add_signal[icv] = add_signal[ismp];
    d_perceptual_weight[icv] = perceptual_weight[ismp];
  }
}

void ntt_vq_main_select(/* Parameters */
			int    cb_len,       /* code book length */
			int    cb_len_max,   /* maximum code vector length */
			double *codev0,      /* code book0 */
			double *codev1,      /* code book1 */
			int    n_can0,
			int    n_can1,
			int    can_ind0[],
			int    can_ind1[],
			int    can_sign0[],
			int    can_sign1[],
                        /* Input */
			double d_target[],
			double d_weight[],
			double d_add_signal[],
			double d_perceptual_weight[],
			/* Output */
			int    *index_wvq0,
			int    *index_wvq1)
{
  /*--- Variables ---*/
  int    ismp, i_can, j_can;
  int    index0, index1, i_can0, i_can1;
  double dist_min, dist;
  double *codev0_p, *codev1_p;
  double pw2, sign0, sign1, reconst;

  index0=index1=i_can0=i_can1=0;
  /*--- Best codevector search ---*/
  dist_min = 1.e100;
  for (i_can=0; i_can<n_can0; i_can++){
    codev0_p = &(codev0[can_ind0[i_can]*cb_len_max]);
    sign0 = 1. - (2. * can_sign0[i_can]);
    for (j_can=0; j_can<n_can1; j_can++){
      codev1_p = &(codev1[can_ind1[j_can]*cb_len_max]);
      sign1 = 1. - (2. * can_sign1[j_can]);
      dist = 0.;
      for (ismp=0; ismp<cb_len; ismp++){
	pw2 = d_perceptual_weight[ismp] * d_perceptual_weight[ismp];

	reconst = d_add_signal[ismp]
	  + 0.5 * d_weight[ismp] *
	    (sign0 * codev0_p[ismp] + sign1 * codev1_p[ismp]);

	dist += pw2 * (d_target[ismp] - reconst) * (d_target[ismp] - reconst);
      }
      if (dist < dist_min){
	dist_min = dist;
	i_can0 = i_can;
	i_can1 = j_can;
	index0 = can_ind0[i_can0];
	index1 = can_ind1[i_can1];
      }
    }
  }

  /*--- Make output indexes ---*/
  *index_wvq0 = index0;
  *index_wvq1 = index1;
  if(can_sign0[i_can0] == 1) 
    *index_wvq0 += ((0x1)<<(ntt_MAXBIT_SHAPE));
  if(can_sign1[i_can1] == 1) 
    *index_wvq1 +=  ((0x1)<<(ntt_MAXBIT_SHAPE));
}

void ntt_vq_pn(/* Parameters */
	       int    nfr,            /* block length */
	       int    nsf,            /* number of sub frames */
	       int    available_bits, /* available bits for VQ */
	       int    n_can,          /* number of pre-selection candidates */
	       double *codev0,        /* code book0 */
	       double *codev1,        /* code book1 */
	       int    cb_len_max,     /* maximum code vector length */
               /* Input */
	       double target[],
	       double lpc_spectrum[], /* LPC spectrum */
	       double bark_env[],     /* Bark-scale envelope */
	       double add_signal[],   /* added signal */
	       double gain[],
	       double perceptual_weight[],
	       /* Output */
	       int    index_wvq[],
	       int    index_pow[])
{
  /*--- Variables ---*/
  int    ismp, isf, idiv, ndiv, top, block_size;
  int    bits, pol_bits0, pol_bits1, cb_len, cb_len0;
  int    can_ind0[ntt_N_CAN_MAX], can_ind1[ntt_N_CAN_MAX];
  int    can_sign0[ntt_N_CAN_MAX], can_sign1[ntt_N_CAN_MAX];
  double weight[ntt_T_FR_MAX], d_weight[ntt_CB_LEN_MAX];
  double d_add_signal[ntt_CB_LEN_MAX];
  double d_target[ntt_CB_LEN_MAX];
  double d_perceptual_weight[ntt_CB_LEN_MAX];
  
  /*--- Parameter settings ---*/
  block_size = nfr * nsf;
  ndiv    = (int)((available_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  cb_len0 =  (int)((block_size + ndiv - 1) / ndiv);
  /*--- Make weighting factor ---*/
  for (isf=0; isf<nsf; isf++){
    top = isf * nfr;
    for (ismp=0; ismp<nfr; ismp++){
      weight[ismp+top]
	= gain[isf] * bark_env[ismp+top] / lpc_spectrum[ismp+top];
    }
  }
  /*--- Sub-vector division loop ---*/
  for (idiv=0; idiv<ndiv; idiv++){
    /*--- set codebook lengths and sizes ---*/
    cb_len = (int)((block_size + ndiv - 1 - idiv) / ndiv);
    bits = (available_bits+ndiv-1-idiv)/ndiv;
    pol_bits0 = ((bits+1)/2)-ntt_MAXBIT_SHAPE;
    pol_bits1 = (bits/2)-ntt_MAXBIT_SHAPE;
    
    /*--- vector division ---*/
    ntt_div_vec(nfr, nsf, cb_len, cb_len0, idiv, ndiv,
		target, d_target,
		weight, d_weight,
		add_signal, d_add_signal,
		perceptual_weight, d_perceptual_weight);
     /*--- pre selection ---*/
    ntt_vq_pre_select(cb_len, cb_len_max, pol_bits0, codev0,
		      d_target, d_weight, d_add_signal, d_perceptual_weight,
		      n_can, can_ind0, can_sign0);
    ntt_vq_pre_select(cb_len, cb_len_max, pol_bits1, codev1,
		      d_target, d_weight, d_add_signal, d_perceptual_weight,
		      n_can, can_ind1, can_sign1);
    
    /*--- main selection ---*/
    ntt_vq_main_select(cb_len, cb_len_max,
		       codev0, codev1,
		       n_can, n_can, can_ind0, can_ind1,
		       can_sign0, can_sign1,
		       d_target, d_weight, d_add_signal, d_perceptual_weight,
		       &index_wvq[idiv], &index_wvq[idiv+ndiv]);
  
  }

  /*--- gain re-quantization ---*/
}

#define OFFSET_GAIN 1.3

void ntt_vq_pre_select(/* Parameters */
		       int    cb_len,         /* code book length */
		       int    cb_len_max,     /* maximum code vector length */
		       int    pol_bits,
		       double *codev,         /* code book */
                       /* Input */
		       double d_target[],
		       double d_weight[],
		       double d_add_signal[],
		       double d_perceptual_weight[],
		       int    n_can,
		       /* Output */
		       int    can_ind[],
		       int    can_sign[])
{
  /*--- Variables ---*/
  double xxp, xyp, xxn, xyn;
  double recp, recn;
  double pw2;
  double *p_code;
  int    icb, ismp;
  double max_t[ntt_N_CAN_MAX];
  double code_targ_tmp_p, code_targ_tmp_n, dtmp;
  double code_targ[ntt_CB_SIZE];
  int    code_sign[ntt_CB_SIZE];
  int    i_can, j_can, tb_top, spl_pnt;

  /*--- Distance calculation ---*/
  for (icb=0; icb<ntt_CB_SIZE; icb++){
    xxp=xyp=xxn=xyn = 0.0;
    code_targ_tmp_p = code_targ_tmp_n = 0.;
    p_code = &(codev[icb*cb_len_max]);
    for (ismp=0; ismp<cb_len; ismp++){
      pw2 = d_perceptual_weight[ismp] * d_perceptual_weight[ismp];

      recp =(d_add_signal[ismp] + p_code[ismp] * d_weight[ismp]) * OFFSET_GAIN;
      xxp = recp * recp;
      xyp = recp * d_target[ismp];
      code_targ_tmp_p += pw2 * (4.0 * xyp - xxp);

      recn =(d_add_signal[ismp] - p_code[ismp] * d_weight[ismp]) * OFFSET_GAIN;
      xxn = recn * recn;
      xyn = recn * d_target[ismp];
      code_targ_tmp_n += pw2 * (4.0 * xyn - xxn);
    }
    if ((pol_bits==1) && (code_targ_tmp_n>code_targ_tmp_p)){
      code_targ[icb] = code_targ_tmp_n;
      code_sign[icb] = 1;
    }
    else{
      code_targ[icb] = code_targ_tmp_p;
      code_sign[icb] = 0;
    }
  }

  /*--- Pre-selection search ---*/
  if (n_can < ntt_CB_SIZE){
    max_t[0] = -1.e30;
    can_ind[0] = 0;
    tb_top = 0;
    for (icb=0; icb<ntt_CB_SIZE; icb++){
      dtmp = code_targ[icb];
      if (dtmp>max_t[tb_top]){
	i_can = tb_top; j_can = 0;
	while(i_can>j_can){
	  spl_pnt = (i_can-j_can)/2 + j_can;
	  if (max_t[spl_pnt]>dtmp){
	    j_can = spl_pnt + 1;
	  }
	  else{
	    i_can = spl_pnt;
	  }
	}
	tb_top = ntt_min(tb_top+1, n_can-1);
	for (j_can=tb_top; j_can>i_can; j_can--){
	  max_t[j_can] = max_t[j_can-1];
	  can_ind[j_can] = can_ind[j_can-1];
	}
	max_t[i_can] = dtmp;
	can_ind[i_can] = icb;
      }
    }

    /*--- Make output ---*/
    for (i_can=0; i_can<n_can; i_can++){
      can_sign[i_can] = code_sign[can_ind[i_can]];
    }
  }
  else{
    for (i_can=0; i_can<n_can; i_can++){
      can_ind[i_can] = i_can;
      can_sign[i_can] = code_sign[i_can];
    }
  }
}
