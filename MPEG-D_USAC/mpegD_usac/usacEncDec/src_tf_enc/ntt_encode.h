/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/

#ifndef __ntt_encode_h__
#define __ntt_encode_h__

#include "block.h"
#include "tns3.h"
#include "ms.h"
/*#include "bitmux.h"
*/
#include "ntt_enc_para.h"

/*** MODULE FUNCTION PROTOTYPE(S) ***/
#ifdef __cplusplus
extern "C" {
#endif



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
			 int    len_max,         /* codevector memory length */
			 double prcpt,
			 int prev_fw_code[],
			 /*
			 double *env_memory,
			 double *pdot,
			 */
			 int    *p_w_type,
			 /* Input */
			 double spectrum[],        /* spectrum */
			 double lpc_spectrum[],    /* LPC spectrum */
			 double pitch_sequence[],/* periodic peak components */
			 int    current_block,     /* block length type */
			 /* Output */
			 double tc[ntt_T_FR_MAX],  /* flattened spectrum */
			 double pwt[ntt_T_FR_MAX], /* perceptual weight */
			 int    index_fw[], /* indices for Bark-scale envelope coding */
			 int    ind_fw_alf[], 
			 double bark_env[]);       /* Bark-scale envelope */
   
   void ntt_enc_gaim(/* Input */
		     double power,      /* power to be encoded */
		     /* Output */
		     int    *index_pow, /* power code index */
		     double *gain,      /* gain */
		     double *norm);     /* power normalize factor */
   
   void ntt_enc_gain(/* Input */
		     double power,      /* power to be encoded */
		     /* Output */
		     int    *index_pow, /* power code index */
		     double *gain,      /* gain */
		     double *norm);     /* power normalize factor */
   
   void ntt_enc_gain_t(/* Parameters */
		       int    nfr,         /* block length */
		       int    nsf,         /* number of sub frames */
		       int    n_ch,
		       /* Input */
		       double bark_env[],  /* Bark-scale envelope */
		       /* In/Out */
		       double tc[],        /* flattened spectrum */
		       /* Output */
		       int    index_pow[], /* gain code indices */
		       double gain[]);     /* gain factor */
   
   void ntt_enc_gair_p(int     *index_pgain,  /* Output: power code index */
		       double powG,        /* Input : power of weighted target */
		       double gain2,       /* Input : squared gain of target */
		       double *g_opt,      /* In/Out: optimum gain */
		       double nume, /*Input: cross power or the orig. & quant. */
		       double denom);      /* Input : power of the orig. regid */
   
   void ntt_enc_pgain(/* Input */
		      double power,      /* power to be encoded */
		      /* Output */
		      int    *index,     /* power code index */
		      double *pgain,
		      double *norm) ;    /* gain */
   
   void ntt_enc_pitch(/* Input */
		      double spectrum[],         /* spectrum */
		      double lpc_spectrum[],     /* LPC spectrum */
		      int    w_type,             /* block type */
		      int    numChannel,
		      int    block_size_samples,
		      int    isampf,
		      double bandUpper,
		      double *pcode,
		      short *pleave0,
		      short *pleave1,
		      /* Output */
		      int    index_pit[],        /* indices for periodic peak components coding */
		      int    index_pls[],
		      int    index_pgain[],
		      double pitch_component[]); /* periodic peak components */
   
   void ntt_extract_pitch(/* Input */
			  int     index_pit,
			  double  pit_seq[],
			  int     block_size_samples,
			  int     isampf,
			  double  bandUpper,
			  /* Output */
			  double  pit_pack[]);
   
   void ntt_freq_domain_pre_process(/* Parameters */
				    int    nfr,                /* block length */
				    int    nsf,                /* number of sub frames */
				    double band_lower,
				    double band_upper,
				    /* Input */
				    double spectrum[],         /* spectrum */
				    double lpc_spectrum[],     /* LPC spectrum */
				    /* Output */
				    double spectrum_out[],      /* spectrum */
				    double lpc_spectrum_out[]); /* LPC spectrum */
   
   void ntt_fw_div(int    ndiv,    /* Param  : number of interleave division */
		   int    length,  /* Param  : codebook length */
		   double prcpt,   /* Param  : perceptual weight */
		   double env[],   /* Input  : envelope vector */
		   double denv[],  /* Output : envelope subvector */
		   double wt[],    /* Input  : weighting factor vector */
		   double dwt[],   /* Output : weighting factor subvector */
		   int    idiv);   /* Param.  : division number */
   
   void ntt_fw_sear(/* Parameters */
		    int    cb_size, /* codebook size */
		    int    length,  /* codevector length */
		    double *codev,  /* codebook */
		    int    len_max, /* codevector memory length */
		    /* Input */
		    double denv[],  /* envelope subvector */
		    double dwt[],   /* weight subvector */
		    /* Output */
		    int    *index,  /* quantization index */
		    double *dmin);  /* quant. weighted distortion */
   
   void ntt_fwalfenc(/* Input */
		     double alf,    /* AR prediction coefficients */
		     /* Output */
		     double *alfq,  /* quantized AR pred. coefficients */
		     int    *ind);     /* the index */
   
   void ntt_fwpred(int    nfr,         /* Param:  block size */
		   int    n_crb,       /* Param:  number of Bark-scale band */
		   int    *bark_tbl,   /* Param:  Bark-scale table */
		   double bandUpper,
		   int    ndiv,        /* Param:  number of interleave division */
		   int    cb_size,     /* Param:  codebook size */
		   int    length,      /* Param:  codevector length */
		   double *codev,      /* Param:  codebook */
		   int    len_max,     /* Param:  codevector memory length */
		   double prcpt,       /* Param. */
		   int    prev_fw_code[],
		   /*double *penv_tmp,*//*Param:memory for Bark-scale envelope*/
		   /*double *pdot_tmp,*//*Param:memory for env. calculation */
		   double iwt[],       /* Input:  LPC envelope */
		   double pred[],      /* Output : Prediction factor */
		   double tc[],        /* Input:  Residual signal */
		   int    index_fw[],  /* Output: Envelope VQ indices */
		   int    *ind_fw_alf, /* Output: AR prediction coefficient index */
		   int    i_sup,       /* Input:  Channel number */
		   int    pred_sw);    /* Input:  prediction switch */
   
   
   void ntt_fwvq(/* Parameters */
		 int    ndiv,     /* number of interleave division */
		 int    cb_size,  /* codebook size */
		 int    length,   /* codevector length */
		 double *codev,   /* codebook */
		 int    len_max,  /* codevector memory length */
		 double prcpt,    /* perceptual control factor */
		 /* Input */
		 double env[],    /* the envelope */
		 double wt[],     /* the weighting factor */
		 /* Output */
		 int    index[]); /* indexes */
   
   void ntt_get_wegt(/* Parameters */
		     int    n_pr,
		     /* Input */
		     double flsp[],
		     /* Output */
		     double wegt[]);
   
   void ntt_getalf(/* Input */
		   double *in,          /* input sound data */
		   /* Output */
		   double *alf,         /* alpha parameter */
		   double *resid);      /* power of the residual */

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
                  int  nsp);
   
   void ntt_lsptowt(/* Parameters */
		    int    nfr,      /* block length */
		    int    n_pr,
		    int    block_size_samples,
		    /* Input */
		    double lsp[],
		    /* Output */
		    double wt[],
		    double *cos_TT);
   
   double ntt_mulaw(/* Input */
		    double x,
		    double xmax,
		    double mu);
   
   void ntt_opt_gain_p(int    i_div,    /* Input : division number */
		       int    cb_len,   /* Param. : codebook length */
		       int    numChannel,
		       double gt[],     /* Input : gain */
		       double d_targetv[], /* Input : target vector */
		       double sig_l[],  /* Input : decoded residual */
		       double d_wt[],   /* Input : weight */
		       double nume[],   /* Output: cross power (total)*/
		       double gain[],   /* Input : gain of the residual */
		       double denom[],  /* Output: resid. power without gain */
		       double nume0[],  /* Output: cross power (divided) */
		       double denom0[], /* Output: resid. power without gain */
                       short *pleave0,
		       short *pleave1);
   
   void ntt_pre_dot_p(int    cb_len,      /* Param. : codebook length */
		      int    i_div,       /* Input : division number */
		      double targetv[],   /* Input : target vector */
		      double d_targetv[], /* Output: target subvector (weighted) */
		      double wt[],        /* Input : weighting vector */
		      double mwt[],       /* Input : perceptual controlled weight */
		      double gt[],        /* Input : gain */
		      double powG[],      /* Output: power of the weighted target */
		      double d_wt[],     /* Output: weighting subvector */
                      int   numChannel,
		      short *pleave0,
		      short *pleave1);

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
                  int    nsp);

   
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
		      double can_code_targ[ntt_POLBITS][ntt_N_CAN_MAX],
		      double can_code_sign[ntt_POLBITS][ntt_N_CAN_MAX],
		      int    can_ind[]);
   
   void ntt_sear_pitch(/* Input */
		       double tc[],
		       double lpc_spectrum[],   /* LPC spectrum */
		       int  block_size_samples,
		       int  isampf,
		       double bandUpper,
		       /* Output */
		       int    *index_pit);
   
   void ntt_sear_x_p(/* Parameters */
		     int    i_div,          /* division number */
		     int    cb_len,         /* codebook length */
		     int    pol_bits0,      /* polarity bits (0 ch) */
		     int    pol_bits1,      /* polarity bits (1 ch) */
		     int    can_ind0[],     /* candidate indexes (0 ch) */
		     int    n_can0,         /* number of candidates (0 ch) */
		     int    can_ind1[],     /* candidate indexes (1 ch) */
		     int    n_can1,         /* number of candidates (1 ch) */
		     int    numChannel,
		     /* Output */
		     int    index[],        /* quantization index */
		     /* Input */
		     double can_code_targ0[ntt_POLBITS][ntt_N_CAN_MAX],
		     double can_code_targ1[ntt_POLBITS][ntt_N_CAN_MAX],
		     double can_code_sign0[ntt_POLBITS][ntt_N_CAN_MAX],
		     double can_code_sign1[ntt_POLBITS][ntt_N_CAN_MAX],
		     double d_wt[],          /* weight subvector */
		     double *pcode,
		     /* Output */
		     double sig_l[],         /* decoded residual */
		     double *dist_min);      /*  quantization distortion */
   
   void ntt_tf_proc_spectrum(double spectrum[],        /* In/Out : spectrum */
			     ntt_INDEX  *indexp,       /* In/Out : code indices*/
			     double lpc_spectrum[],    /* Output : LPC spectrum */
			     double bark_env[],        /* Output : Bark-scale envelope*/
			     double pitch_component[], /* Output : periodic peak components */
			     double gain[]);           /* Output : gain factor */

   
   void ntt_wvq_pitch(double targetv[],     /* Input : target vector */
		      double wt[],          /* Input : weighting vector */
		      double pwt[],         /* Input : perceptual controlled weight*/
		      double pgain[],       /* Input : gain */
		      double *pcode,
                      short *pleave0,
		      short *pleave1,
		      int    numChannel,
		      int    index_pls[],   /* Output : quantization indexes */
		      double pgainq[],      /* In/Out : gain */
		      int    index_pgain[]);/* Output : gain index */
   
   void ntt_prcptw(/* In/Out */
		   double pwt[],           /* perceptual weighting factor */
		   /* Input */          
		   double pred[],          /* interframe prediction factor */
                   double g1,
		   double g2,
		   int  block_size_samples);
   
   void ntt_prcptw_s(double wt[],    /* Input : LPC weighting factor */
		     double gain[],  /* Input : Gain parameter */
		     double pwt[],   /* Output : Perceptual weighting factor*/
                     double g1,
		     double g2,
		     int  numChannel,
		     int  block_size_samples);
   void ntt_tf_perceptual_model(double lpc_spectrum[],      /* Input : LPC spectrum*/
				double bark_env[],          /* Input : Bark-scale envelope*/
				double gain[],              /* Input : gain factor*/
				int       w_type,           /* Input : block type */
				double spectrum[],          /* Input : spectrum */
				double pitch_sequence[],    /* Input : periodic peak components */
                                int    numChannel,
				int    block_size_samples,
				double bandUpper,
				double perceptual_weight[]); /* Output : perceptual weight */

   void ntt_div_vec(int    nfr,                       /* Param. : block length */
		    int    nsf,                       /* Param. : number of sub frames */
		    int    cb_len,                    /* Param. : codebook length */
		    int    cb_len0,                   /* Param. */
		    int    idiv,                      /* Param. */
		    int    ndiv,                      /* Param. :number of interleave division */
		    double target[],               /* Input */
		    double d_target[],             /* Output */
		    double weight[],               /* Input */
		    double d_weight[],             /* Output */
		    double add_signal[],           /* Input */
		    double d_add_signal[],         /* Output */
		    double perceptual_weight[],    /* Input */
		    double d_perceptual_weight[]); /* Output */
   
   void ntt_tf_quantize_spectrum(double spectrum[],         /* Input : spectrum */
				 double lpc_spectrum[],     /* Input : LPC spectrum */
				 double bark_env[],         /* Input : Bark-scale envelope */
				 double pitch_component[],  /* Input : periodic peak components */
				 double gain[],             /* Input : gain factor*/
				 double perceptual_weight[],/* Input : perceptual weight */
				 ntt_INDEX  *indexp,         /* In/Out : VQ code indices */
				 int debug_level);

   
   void ntt_vq_main_select(/* Parameters */
			   int    cb_len,               /* code book length */
			   int    cb_len_max,           /* maximum code vector length */
			   double *codev0,              /* code book0 */
			   double *codev1,              /* code book1 */
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
			   int    *index_wvq1);
   
   void ntt_vq_pn(/* Parameters */
		  int    nfr,             /* block length */
		  int    nsf,             /* number of sub frames */
		  int    available_bits,  /* available bits for VQ */
		  int    n_can,           /* number of pre-selection candidates */
		  double *codev0,         /* code book0 */
		  double *codev1,         /* code book1 */
		  int    cb_len_max,      /* maximum code vector length */
		  /* Input */
		  double target[],
		  double lpc_spectrum[],  /* LPC spectrum */
		  double bark_env[],      /* Bark-scale envelope */
		  double add_signal[],    /* added signal */
		  double gain[],
		  double perceptual_weight[],
		  /* Output */
		  int    index_wvq[],
		  int    index_pow[]);
   
   void ntt_vq_pre_select(/* Parameters */
			  int    cb_len,                 /* code book length */
			  int    cb_len_max,             /* maximum code vector length */
			  int    pol_bits,
			  double *codev,                 /* code book */
			  /* Input */
			  double d_target[],
			  double d_weight[],
			  double d_add_signal[],
			  double d_perceptual_weight[],
			  int    n_can,
			  /* Output */
			  int    can_ind[],
			  int    can_sign[]);


void ntt_scale_reenc_gain(
                    int nfr,
                    int nsf,
                    int nch,
                    int iscl,
                    int available_bits,
                    double *codev0,
                    double *codev1,
                    int cb_len_max,
                    double  target[],
                    double lpc_spectrum[],
                    double bark_env[],
                    double add_signal[],
                    double gain[],
                    double perceptual_weight[],
                    int index_wvq[],
                    int index_pow[]);

void ntt_reenc_gain(
               int nfr,
               int nsf,
               int nch,
               int vq_bits,
               double *covev0,
               double *codev1,
               int cb_len_max,
               double  spectrum[],
               double lpc_spectrum[],
               double bark_env[],
               double add_signal[],
               double gain[],
               double perceptual_weight[],
               int    index_wvq[],
               int    index_pow[]);


/* moriya informed to tsushima BUGFIX */
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
        );
/** end tsushima **/

/*
void ntt_headerenc( int iscl,
		    HANDLE_BSBITSTREAM  stream, 
                    ntt_INDEX* ntt_index, 
		    int *used_bits,
		    int *nr_of_sfb,
		    TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
		    NOK_LT_PRED_STATUS *nok_lt_status,
		    PRED_TYPE pred_type);


void ntt_select_ms( 
           double      *spectral_line_vector[MAX_TIME_CHANNELS] ,
           ntt_INDEX   *index,
           int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
           int     nr_of_sfb[MAX_TIME_CHANNELS],
	   int      iscl);



void ntt_tns_enc(double *spectral_line_vector[MAX_TIME_CHANNELS],
		 ntt_INDEX  *index, 
	         int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
	         WINDOW_SEQUENCE   windowSequence,
		 int         nr_of_sfb[MAX_TIME_CHANNELS],
		 TNS_INFO	 *tnsInfo[MAX_TIME_CHANNELS]
		);   


void ntt_maxsfb( ntt_INDEX   *index,
		 int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
	         int     nr_of_sfb[MAX_TIME_CHANNELS],
	         int     iscl
		 );
*/

int  EncTf_tvq_encode(
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *baselayer_spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  int         block_size_samples,
  HANDLE_BSBITSTREAM *output_au,
  int         mat_shift[][ntt_N_SUP_MAX],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         *max_sfb,
  MSInfo      *msInfo,
  TNS_INFO    *tvqTnsInfo[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
  int         debug_level
);

#ifdef __cplusplus
}
#endif

#endif /* __ntt_encode_h__ */
