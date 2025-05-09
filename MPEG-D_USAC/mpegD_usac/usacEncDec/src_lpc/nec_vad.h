/*
This software module was originally developed by
Hironori Ito (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	Constants and Function Prototype Definition
 *
 *	Ver1.0	99.5.10	H.Ito(NEC)
 */

/***** CNG parameters *****/
#define NEC_RMS_VAD_MAX		7932
#define NEC_VAD_MU_LAW		1024
#define NEC_VAD_RMS_BIT		6

#define RMS_ENC_AVE_LEN		8		/* 80msec */
#define LSP_AVE_LEN		8		/* 80msec */
#define LSP_AVE_LEN_WB		LSP_AVE_LEN*2	/* 80msec */

#define LSP_SM_COEFF		((float)7168/8192)
#define RMS_SM_COEFF		((float)7168/8192)

#define RMS_SMOOTH_OFF_LEN      4               /* 40msec */
#define RMS_CHANGE              6               /* 6 dB */

#define SID_LPC_ORDER_NB	10
#define SID_LPC_ORDER_WB	20
#define NUM_LSP_INDICES_NB	5
#define NUM_LSP_INDICES_WB	10

#define NEC_NSF8        8

#define CELP_MAX_SUBFRAMES 10
/********** prototype **********/
#ifndef _nec_vad_h_
#define _nec_vad_h_

#ifdef __cplusplus
extern "C" {
#endif

void nec_vad_init( void );

int nec_vad_decision( void );

int nec_vad (float     PP_InputSignal[], 
             float     HamWin[],
             long      window_offset, 
             long      window_size,
             long      SampleRateMode,             
             long      ExcitationMode);

void nec_hangover(long SampleRateMode,      /* in: */
                  long frame_size,          /* in: */
                  int  *vad_flag);          /* in/out: */

unsigned long nec_dtx (int   vad_flag,           /* in: */
                       long  lpc_order,          /* in: */
                       float cng_xnorm,          /* in: */
                       float *cng_lsp_coeff,     /* in: */
                       long  SampleRateMode,     /* in: */
                       long  frame_size);        /* in: */

short Random(short *seed);

float Gauss(short *seed);

void setRandomBits(unsigned long *l, int n, short *seed);

float Randacbg(short *seed);

void rms_smooth_decision(float qxnorm,          /* in: */
                         float SID_rms,         /* in: */
                         long SampleRateMode,   /* in: */
                         long frame_size,       /* in: */
                         short prev_tx,         /* in: */
                         short *sm_off_flag);   /* out: */


void mk_cng (float *mpexc,          /* out: */
             float *g_ec,           /* out: */
             float *excg,           /* out: */
             float *g_gc,           /* out: */
             float *alpha,          /* in: LPC coeff */
             short lpc_order,       /* in: */
             float rms,             /* in */
             short sbfrm_size,      /* in */
             short *seed,           /* in/out: */
             short prev_tx_flag,    /* in */
             float *prev_G,         /* in/out */
             long  ExcitationMode,  /* in */
             long  Configuration);  /* in */

void nec_cng_lspenc_nb(float in[],       /* in: input LSPs */
                       float out[],      /* out: output LSPs */
                       float weight[],   /* in: weighting factor */
                       float min_gap,    /* in: minimum gap between adjacent LSPs */
                       long  lpc_order,  /* in: LPC order */
                       long  num_dc,     /* in: number of delayed candidates for DD */
                       long  idx[],      /* out: LSP indicies */
                       float tbl[],      /* in: VQ table */
                       float d_tbl[],    /* in: DVQ table */
                       long  dim_1[],    /* in: dimensions of the 1st VQ vectors */
                       long  ncd_1[],    /* in: numbers of the 1st VQ codes */
                       long  dim_2[],    /* in: dimensions of the 2nd VQ vectors */
                       long  ncd_2[],    /* in: numbers of the 2nd VQ codes */
                       long  flagStab    /* in: 0 - stabilization OFF, 1 - stabilization ON */
);

void nec_cng_lspdec_nb(
    float out[], /* out: output LSPs */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    unsigned long idx[], /* in: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[], /* in: numbers od the 2nd VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    );

void nec_cng_bws_lsp_quantizer(
		       float lsp[],		/* input  */
		       float qlsp8[],		/* input  */
		       float qlsp[],		/* output  */
#define	NEC_MAX_LSPVQ_ORDER	20
		       float blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
		       long indices[],		/* output  */
		       long frame_bit_allocation[], /* configuration input */
		       long lpc_order,		/* configuration input */
		       long lpc_order_8); 	/* configuration input */

void nec_cng_bws_lsp_decoder(
		     unsigned long indices[],	/* input  */
		     float qlsp8[],		/* input  */
		     float qlsp[],		/* output  */
#ifdef VERSION2_EC
		     float qlsp_pre[],		/* input  */
#endif
		     float blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
		     long lpc_order,		/* configuration input */
		     long lpc_order_8 );	/* configuration input */

void nec_cng_bws_lspvec_renew( float	qlsp8[],	/* input */
                               float 	qlsp_pre[],	/* input */
                               float 	blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
                               long	lpc_order,	/* configuration input */
                               long	lpc_order_8 );	/* configuration input */

void nec_cng_lspenc_wb_low(
    float in[], /* in: input LSPs */
    float out[], /* out: output LSPs */
    float weight[], /* in: weighting factor */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long num_dc, /* in: number of delayed candidates for DD */
    long idx[],  /* out: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[], /* in: numbers of the 2nd VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    );

void nec_cng_lspdec_wb_low(
    float out[], /* out: output LSPs */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long idx[], /* in: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[], /* in: numbers od the 2nd VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    );

void nec_cng_lspenc_wb_high(
    float in[], /* in: input LSPs */
    float out[], /* out: output LSPs */
    float weight[], /* in: weighting factor */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long num_dc, /* in: number of delayed candidates for DD */
    long idx[],  /* out: LSP indicies */
    float tbl[], /* in: VQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    );

void nec_cng_lspdec_wb_high(
    float out[], /* out: output LSPs */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long idx[], /* in: LSP indicies */
    float tbl[], /* in: VQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    );

void nec_mk_cng_excitation (float LpcCoef[],        /* in: */
                            long  *rms_index,       /* out: */
                            float decoded_excitation[], /* out: */
                            long  lpc_order,        /* in: */
                            long  frame_size,       /* in: */
                            long  sbfrm_size,       /* in: */
                            long  n_subframes,      /* in: */
                            long  SampleRateMode,   /* in: */
                            long  ExcitationMode,   /* in: */
                            long  Configuration,    /* in: */
                            float *mem_past_syn,    /* in/out: */
                            long  *flag_rms,        /* in/out: */
                            float *pqxnorm,         /* in/out: */
                            float *cng_xnorm,       /* in/out: */
                            short prev_tx_flag      /* in: */
                            );

void nec_mk_cng_excitation_dec (float          LpcCoef[],             /* in:     */
                                unsigned long  rms_index,             /* in:     */
                                float          decoded_excitation[],  /* out:    */
                                long           lpc_order,             /* in:     */
                                long           sbfrm_size,            /* in:     */
                                long           n_subframes,           /* in:     */
                                long           SampleRateMode,        /* in:     */
                                long           ExcitationMode,        /* in:     */
                                long           Configuration,         /* in:     */ 
                                long           *flag_rms,             /* in/out: */
                                float          *pqxnorm,              /* in/out: */
                                short          prev_tx_flag           /* in:     */
);


void nec_mk_cng_excitation_bws(
	float LpcCoef[],		/* in: */
	float decoded_excitation[],	/* out: */
	long  lpc_order,		/* in: */
	long  sbfrm_size, 		/* in: */
	long  n_subframes,		/* in: */
	long  rms_index_8,		/* in: RMS code index */
	float *mem_past_syn,		/* in/out: */
	long  *flag_rms,		/* in/out: */
	float *pqxnorm,  		/* in/out: */
	short prev_tx_flag
);

void nec_mk_cng_excitation_bws_dec(
        float LpcCoef[],                /* in: */
        unsigned long  rms_index,       /* in: */
        float decoded_excitation[],     /* out: */
        long  lpc_order,                /* in: */
        long  sbfrm_size,               /* in: */
        long  n_subframes,              /* in: */
        long *flag_rms,                 /* in/out: */
        float *pqxnorm,                 /* in/out: */
        short prev_tx_flag              /* in: */
);

void mk_cng_excitation(
    float int_Qlpc_coefficients[], /* in: interpolated LPC */
    long lpc_order,                /* in: order of LPC */
    long frame_size,               /* in: frame size */
    long sbfrm_size,               /* in: subframe size */
    long n_subframes,              /* in: number of subframes */
    long *rms_index,               /* out: RMS code index */
    float decoded_excitation[],    /* out: decoded excitation */
    long SampleRateMode,           /* in: */
    long ExcitationMode,           /* in: */
    long Configuration,            /* in: */
    float *mem_past_syn,           /* in/out: */
    long  *flag_rms,               /* in/out: */
    float *pqxnorm,                /* in/out: */
    float *cng_xnorm,              /* in/out: */
    short prev_tx_flag             /* in: */
);

void mk_cng_excitation_dec(unsigned long rms_index,       /* in: RMS code index */
                           float int_Qlpc_coefficients[], /* in: interpolated LPC */
                           long lpc_order,                /* in: order of LPC */
                           long sbfrm_size,               /* in: subframe size */
                           long n_subframes,              /* in: number of subframes */
                           float excitation[],            /* out: decoded excitation */
                           long SampleRateMode,           /* in: */
                           long ExcitationMode,           /* in:                     */
                           long Configuration,            /* in:     */ 
                           long *flag_rms,                /* in/out: */
                           float *pqxnorm,                /* in/out: */
                           short prev_tx_flag             /* in: */
);

void mk_cng_excitation_bws(
        float int_Qlpc_coefficients[], /* in: interpolated LPC */
        long lpc_order,                /* in: order of LPC */
        long sbfrm_size,               /* in: subframe size */
        long n_subframes,              /* in: number of subframes */
        float decoded_excitation[],    /* out: decoded excitation */
        long  rms_index,                /* in: RMS code index */
        float *mem_past_syn,            /* in/out: */
        long  *flag_rms,                /* in/out: */
        float *pqxnorm,                 /* in/out: */
        short prev_tx_flag              /* in: */
);

void mk_cng_excitation_bws_dec(
        unsigned long rms_index,       /* in: RMS code index */
        float int_Qlpc_coefficients[], /* in: interpolated LPC */
        long lpc_order,                /* in: order of LPC */
        long sbfrm_size,               /* in: subframe size */
        long n_subframes,              /* in: number of subframes */
        float excitation[],            /* out: decoded excitation */
        long  *flag_rms,                /* in/out: */
        float *pqxnorm,                 /* in/out: */
        short prev_tx_flag              /* in: */
);

void nec_rms_ave (float *InputSignal,     /* in:  */
                  float *cng_xnorm,       /* out: */
                  long  frame_size,       /* in:  */
                  long  sbfrm_size,       /* in:  */
                  long  n_subframes,      /* in:  */
                  long  SampleRateMode,   /* in:  */
                  int   vad_flag          /* in:  */
);

void nec_lsp_ave (float *lpc_coeff,          /* in:  */
                  float *lsp_coefficients,   /* out: */
                  long  lpc_order,           /* in:  */
                  long  n_lpc_analysis,      /* in:  */
                  long  ExcitationMode,      /* in:  */ 
		  int 	vad_flag	     /* in:  */
);

void nec_lsp_ave_bws(float *lpc_coeff,		/* in: */
		     float *lsp_coefficients,	/* out: */
		     long lpc_order,		/* in: */
		     long n_lpc_analysis,	/* in: */
		     int vad_flag		/* in: */
);

#ifdef __cplusplus
}
#endif

#endif
