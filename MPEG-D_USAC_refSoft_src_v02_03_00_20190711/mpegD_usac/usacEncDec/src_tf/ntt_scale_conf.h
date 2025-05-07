/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-24                           */
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
#ifndef ntt_scale_conf_h
#define ntt_scale_conf_h


/*---------------------------------------------------------------------------*/
/* twinvq.h                                                                  */
/* Header file of TwinVQ audio coding program                                */
/*---------------------------------------------------------------------------*/

#define ntt_NSclLay_MAX  8   /* Maximum number of scale layers */
#define ntt_NSclLay_BITS 4  /* Number of bits for layers information */
#define mat_SHIFT_BIT 2 /* Matsushita Electric Industrial(Tomokazu Ishikawa) */

#define ntt_BLIM_BITS 4     /* Number of bits for bandwidth information */
#define ntt_IBPS_BITS_SCL 16 /* bitrate for scalable coder */

/*---------------------------------------------------------------------------*/
/* POST FILTER                                                               */
/*---------------------------------------------------------------------------*/
#define ntt_POSTFILT_SCL   YES

#define   ntt_CB_LEN_READ_SCL     28
#define   ntt_CB_LEN_READ_S_SCL   24
#define   ntt_N_MODE_SCL   (1<<ntt_LSP_BIT0)
#define   ntt_NC0_SCL      (1<<ntt_LSP_BIT1)
#define   ntt_NC1_SCL      (1<<ntt_LSP_BIT2)


#define    ntt_GAIN_BITS_SCL     8
#define    ntt_AMP_MAX_SCL       8000.
#define    ntt_SUB_GAIN_BITS_SCL 4
#define    ntt_SUB_AMP_MAX_SCL   6000.
#define    ntt_MU_SCL            100.
#define    ntt_N_PR_SCL          20
#define    ntt_LSPCODEBOOK_SCL   "./tables/tf_vq_tbls/20b19s48sc"
#define    ntt_LSP_BIT0_SCL      1
#define    ntt_LSP_BIT1_SCL      6
#define    ntt_LSP_BIT2_SCL      4
#define    ntt_LSP_SPLIT_SCL     3
#define    ntt_FW_CB_NAME_SCL    "./tables/tf_vq_tbls/fcdl"
#define    ntt_FW_N_DIV_SCL      7
#define    ntt_FW_N_BIT_SCL      6
#define    ntt_CB_NAME_SCL0      "./tables/tf_vq_tbls/scmdct_0"
#define    ntt_CB_NAME_SCL1      "./tables/tf_vq_tbls/scmdct_1"
#define    ntt_CB_NAME_SCL0s     "./tables/tf_vq_tbls/scmdct_2"
#define    ntt_CB_NAME_SCL1s     "./tables/tf_vq_tbls/scmdct_3"

#define    ntt_NUM_STEP_SCL     (1<<ntt_GAIN_BITS_SCL)
#define    ntt_STEP_SCL         (ntt_AMP_MAX_SCL / (ntt_NUM_STEP_SCL - 1))
#define    ntt_SUB_NUM_STEP_SCL (1<<ntt_SUB_GAIN_BITS_SCL)
#define    ntt_SUB_STEP_SCL     (ntt_SUB_AMP_MAX_SCL/(ntt_SUB_NUM_STEP_SCL-1))


#define    ntt_FW_CB_SIZE_SCL  (1<<ntt_FW_N_BIT_SCL)
#define    ntt_N_CRB_SCL       42
#define    ntt_N_CRB_S_SCL     12
#define    ntt_FW_CB_LEN_SCL   (ntt_N_CRB_SCL/ntt_FW_N_DIV_SCL)
#define    ntt_FW_CB_SIZE_S_SCL 0
#define    ntt_FW_CB_LEN_S_SCL  0
 
#define    GAIN_TBIT_SCLperCH  ntt_GAIN_BITS_SCL
#define    GAIN_TBIT_S_SCLperCH \
	     (ntt_GAIN_BITS_SCL+ntt_SUB_GAIN_BITS_SCL*ntt_N_SHRT)
#define    ntt_NMTOOL_BITS_SCLperCH \
	     (LSP_TBITperCH+GAIN_TBIT_SCLperCH+FW_TBITperCH+mat_SHIFT_BIT)
#define    ntt_NMTOOL_BITS_S_SCLperCH \
	     (LSP_TBITperCH+GAIN_TBIT_S_SCLperCH+mat_SHIFT_BIT)

/****************************  FUNCTIONS  ************************************/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
 
#ifdef __cplusplus
extern "C" {
#endif

void ntt_scale_message(int iscl, int IBPS, int IBPS_SCL[]);

void ntt_scale_tf_proc_spectrum_d(/* Input */
				  ntt_INDEX  *indexp,
				  int mat_shift[],
				  double *cos_TT,
				  double flat_spectrum[],
				  /* Output */
				  double spectrum[],
				  int iscl);

void ntt_scale_init ( int        ntt_NSclLay, 
                      int        *ibps, 
                      float      sampling_rate,
                      ntt_INDEX* ntt_index, 
                      ntt_INDEX* ntt_index_scl);

void ntt_scale_free(ntt_DATA_scl *nttDatScl);


void ntt_scale_dec_bark_env(/* Parameters */
                      int    nfr,
                      int    nsf,
                      int    n_ch,
                      double *codebook,
                      int    ndiv,
                      int    cv_len,
                      int    cv_len_max,
                      int    *bark_tbl,
                      int    n_crb,
                      double alf_step,
                      int    prev_fw_code[],
                      /* Input */
                      int    index_fw[],
                      int    index_fw_alf[],
                      /* Output */
                      double bark_env[],
                      double band_lower[]); /*In*/

void ntt_dec_sq_gain( /* Input */
                  int     index_pow,
                  double  amp_max,
                  double  mu,
                  double  step,
                  double  amp_nm,
                     /* Output */
                  double  *gain);

int mat_scale_set_shift_para2(int a, int *bark_table,
		 ntt_INDEX* ntt_index, int mat_shift[]);

void mat_scale_tf_requantize_spectrum(/* Input */
                                      ntt_INDEX *indexp,
				      int mat_shift[],
                                      /* Output */
                                      double flat_spectrum[],
                                      /* scalable layer ID */
                                      int iscl);

#ifdef __cplusplus
}
#endif

#endif
