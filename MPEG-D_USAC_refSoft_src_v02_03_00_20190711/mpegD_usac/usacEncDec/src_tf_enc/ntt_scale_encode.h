/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
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



/*** MODULE FUNCTION PROTOTYPE(S) ***/
#ifdef __cplusplus
extern "C" {
#endif
   
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
   );
   void ntt_scale_fgetalf(int    nfr,        /* Param. : subframe size */
			  int    nsf,        /* Param. : number of subframes */
			  int    block_size_samples,
			  double *spectrum,  /* Input : MDCT spectrum */
			  double *alf,       /* Output : LPC coefficients */
			  double *resid,     /* Output : residual power */
			  int    n_pr,          /* Input */
			  double band_exp,   /* Input */
                          double *cos_TT);
			   
   void ntt_scale_prcptw(/* In/Out */
			 double pwt[],            /* perceptual weighting factor */
			 /* Input */
			 double pred[],           /* interframe prediction factor */
			 ntt_PARAM *param_ntt,    /* encoding parameters */
			 double gamma_w_scl,     
			 double gamma_w_mic_scli);
   
   void ntt_scale_prcptw_s(double wt[],            /* Input : LPC weighting factor */
			   double gain[],                /* Input : Gain parameter */
			   double pwt[],                 /* Output : Perceptual weighting factor */
			   ntt_PARAM *param_ntt,         /* Input : encoding parameters */
			   double gamma_w_s_scl,   /* Input */
			   double gamma_w_s_t_scl, /* Input */
			   int    prcptw_s_t_scl); /* Input */
   
   void ntt_scale_prcptw_m(double wt[],       /* Input : LPC weighting factor */
			   double gain[],           /* Input : Gain parameter */
			   double pwt[],            /* Output : Perceptual weighting factor */
			   double gamma_w_m_scl,    /* Input */
			   double gamma_w_m_t_scl,  /* Input */
			   int    prcptw_m_t_scl);  /* Input */
   
   void ntt_scale_tf_perceptual_model(/* Input */
				      double lpc_spectrum[], /* LPC spectrum */
				      double bark_env[],     /* Bark-scale envelope */
				      double gain[],         /* gain factor */
				      int    w_type,            /* block type */
				      double spectrum[],     /* specturm */
				      double pitch_sequence[], /* pitch peak components */
				      int  numChannel,
				      int  block_size_samples,
				      /* Output */
				      double perceptual_weight[]); /* perceptual weight */

void ntt_scale_tf_proc_spectrum(/* In/Out */
                                double spectrum[],
                                /* In/Out */
                                ntt_INDEX  *index,
				int mat_shift[ntt_N_SUP_MAX],
                                double *cos_TT,
				/* Output */
                                double lpc_spectrum[],
                                double bark_env[],
                                double pitch_sequence[],
                                double gain[],
                                int iscl,
                                double lpc_spectrum_org[],
                                double lpc_spectrum_mod[]);
   
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
                      double band_lower[]);
   
void ntt_scale_fwpred(int    nfr,  /* Param:  block size */
                int    n_crb,       /* Param:  number of Bark-scale band */
                int    *bark_tbl,   /* Param:  Bark-scale table */
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
                double band_lower);

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
                    double gain[]);

void ntt_enc_sq_gain(
                     double power,      /* Input  --- Power to be encoded */
                     double  amp_max,
                     double  mu,
                     double  step,
                     double  amp_nm,
                     int    *index_pow, /* Output --- Power code index */
                     double *gain);     /* Output --- Gain */




/* Matsushita Electric Industrial ( Tomokazu Ishikawa ) */


int mat_scale_lay_shift2(double *spect,
                         double *spect2,
                         int iscl,
                         ntt_INDEX *index,
			 int mat_shift[ntt_N_SUP_MAX]);

void mat_scale_tf_quantize_spectrum(double spectrum[],
                                    double lpc_spectrum[],
                                    double bark_env[],
                                    double pitch_sequence[],
                                    double gain[],
                                    double perceptual_weight[],
                                    ntt_INDEX *index,
				    int    mat_shift[ntt_N_SUP_MAX],
                                    int       iscl);

   
#ifdef __cplusplus
}
#endif

