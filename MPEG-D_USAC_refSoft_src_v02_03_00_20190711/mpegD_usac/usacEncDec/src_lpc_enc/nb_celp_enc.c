/*
This software module was originally developed by

Ron Burns (Hughes Electronics)

and edited by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Toshiyuki Nomura (NEC Corporation)

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

/* MPEG-4 Audio Verification Model
 *   CELP core (Ver. 3.01)
 *                                                    
 *  Framework:
 *    Original: 06/18/96  R. Burns (Hughes Electronics)
 *	  Last Modified: 11/07/96  N.Tanaka (Panasonic)
 *                       01/13/97  N.Tanaka (Panasonic)
 *	       		 02/27/97  T.Nomura (NEC)
 *                       06/18/97  N.Tanaka (Panasonic)
 *                                                    
 *  Used Modules:
 *    abs_preprocessing          : Philips
 *    abs_classifier             : (Dummy)
 *    abs_lpc_analysis           : Philips
 *    abs_lpc_quantizer          : Panasonic
 *     (LPC-LSP conversion)      : AT&T
 *    abs_weighting_module       : Philips
 *    abs_excitation_analysis    : NEC
 *    abs_bitstream_mux          : NEC/Panasonic
 *
 *  abs_lpc_interpolation is included in abs_lpc_quantizer module.
 *
 *  History:
 *    Ver. 0.01 07/03/96	Born
 *    Ver. 1.10 07/25/96	Panasonic LPC quantizer
 *    Ver. 1.11 08/06/96	I/F revision
 *    Ver. 1.12 09/04/96    I/F revision
 *    Ver. 1.13 09/24/96	I/F revision
 *    Ver. 2.00 11/07/96    Module replacements & I/Fs revision 
 *    Ver. 2.11 01/13/97    Added 22bits ver. of Panasonic LSPVQ
 *    Ver. 3.00 02/27/97    Added NEC Rate Control Functionality
 *    Ver. 3.01 06/18/97    Panasonic LSPQ in source code
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* #include "libtsp.h" */ 	/* HP 971117 */

#include "lpc_common.h"
#include "celp_proto_enc.h"

/* for Panasonic modules */
#include "pan_celp_const.h"
#include "pan_celp_proto.h"

/* for AT&T modules */
#include	"att_proto.h"

/* for Philips modules */
/* Modified AG: 28-nov-96 */
#include	"phi_cons.h"
#include        "phi_lpc.h"

/* for NEC modules */
#include	"nec_abs_const.h"
#include	"nec_abs_proto.h"
#include        "nec_vad.h"

#define TRUE	1
#define FALSE	0

extern unsigned long TX_flag_enc;
/* -------------- */
/* Initialization */
/* -------------- */

void nb_abs_lpc_quantizer (
    float lpc_coefficients[],		/* in: LPC */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC */ 
    long lpc_indices[], 		/* out: LPC code indices */
    long lpc_order,			/* in: order of LPC */
    long n_lpc_analysis,     /* in: number of LP analysis per frame */
    long n_subframes,        /* in: number of subframes */
    float *prev_Qlsp_coefficients,
    float *cng_lsp_coeff
)
{

    #include	"inc_lsp22.tbl"

    float *lsp_coefficients;
    float *Qlsp_coefficients;
    float *int_Qlsp_coefficients;
    float *tmp_lpc_coefficients;
    long i, j;

    float *lsp_weight;
    float *d_lsp;
    static float p_factor=PAN_LSP_AR_R_CELP;
    static float min_gap=PAN_MINGAP_CELP;
    static long num_dc=PAN_N_DC_LSP_CELP;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;
    static short SID_init_flag=1;
    static float SID_lsp_coeff[SID_LPC_ORDER_NB];
    static float prev_lsp_coeff[SID_LPC_ORDER_NB];
    static short lsp_sm_flag = 0;

    if(SID_init_flag){
        for(i=0; i<SID_LPC_ORDER_NB; i++){
            SID_lsp_coeff[i]
                     = (float)((float)(i+1)/(float)(SID_LPC_ORDER_NB+1));
            prev_lsp_coeff[i]
                     = (float)((float)(i+1)/(float)(SID_LPC_ORDER_NB+1));
        }
        SID_init_flag=0;
    }

/* Memory allocation */
    if((lsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((int_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(3);
	}
    if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(4);
	}
    if((lsp_weight=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(5);
	}
    if((d_lsp=(float *)calloc((lpc_order+1), sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(6);
	}

/* reverse the sign of input LPCs */
/*   Philips: 			A(z) = 1+a1z^(-1)+ ... +apz^(-p) */
/*   AT&T, Panasonic:	A(z) = 1-a1z^(-1)- ... -apz^(-p) */
    tmp_lpc_coefficients[0] = 1.;
    for(i=0;i<lpc_order;i++) 

        tmp_lpc_coefficients[i+1] = 
			-lpc_coefficients[lpc_order*(n_lpc_analysis-1)+i];

/* LPC -> LSP */
    if(TX_flag_enc == 1){
	pc2lsf(lsp_coefficients, tmp_lpc_coefficients, lpc_order);
	for(i=0;i<lpc_order;i++) lsp_coefficients[i] /= (float)PAN_PI;
    }else{
        for(i=0;i<lpc_order;i++) lsp_coefficients[i] = cng_lsp_coeff[i];
    }

/* Weighting factor */
    d_lsp[0] = lsp_coefficients[0];
    for(i=1;i<lpc_order;i++) { 
        d_lsp[i] = lsp_coefficients[i]-lsp_coefficients[i-1];
    }
    d_lsp[lpc_order] = (float)(1.-lsp_coefficients[lpc_order-1]);
    for(i=0;i<=lpc_order;i++) {
        if(d_lsp[i]<min_gap) d_lsp[i] = min_gap;
    }
 
    for(i=0;i<=lpc_order;i++) d_lsp[i] = (float)(1./d_lsp[i]);
    for(i=0;i<lpc_order;i++) {
        lsp_weight[i] = d_lsp[i]+d_lsp[i+1];
    }

/* Not weighted 
    for(i=0;i<lpc_order;i++) lsp_weight[i] = 1.;
*/


    if(TX_flag_enc==1){
	lsp_sm_flag = 1;
/* Quantization */
    lsp_tbl = lsp_tbl22;
    d_tbl = d_tbl22;
    pd_tbl = pd_tbl22;
    dim_1 = dim22_1;
    dim_2 = dim22_2;
    ncd_1 = ncd22_1;
    ncd_2 = ncd22_2;
    pan_lspqtz2_dd(lsp_coefficients, 
        prev_Qlsp_coefficients, Qlsp_coefficients, 
        lsp_weight, p_factor, min_gap, lpc_order, num_dc,     
        lpc_indices, lsp_tbl, d_tbl, pd_tbl, dim_1,
        ncd_1, dim_2, ncd_2, 1);
    }else if(TX_flag_enc == 2){
       /* Quantization */
        lsp_tbl = lsp_tbl22;
        d_tbl = d_tbl22;
        dim_1 = dim22_1;
        dim_2 = dim22_2;
        ncd_1 = ncd22_1;
        ncd_2 = ncd22_2;

       nec_cng_lspenc_nb(lsp_coefficients, Qlsp_coefficients, 
                       lsp_weight, min_gap, lpc_order, num_dc,     
                       lpc_indices, lsp_tbl, d_tbl, dim_1,
                       ncd_1, dim_2, ncd_2, 1);
        for(i=0; i<lpc_order; i++){
            SID_lsp_coeff[i] = Qlsp_coefficients[i];
        }
        if(lsp_sm_flag == 1){
            for(i=0; i<lpc_order; i++){
                Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*Qlsp_coefficients[i];
            }
        }else lsp_sm_flag = 1;
    }else if(TX_flag_enc == 0 || TX_flag_enc == 3){
        for(i=0; i<lpc_order; i++){
            Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*SID_lsp_coeff[i];
        }
        for(i=0; i<NUM_LSP_INDICES_NB; i++) lpc_indices[i] = 0;
    }
    for(i=0; i<lpc_order; i++)prev_lsp_coeff[i] = Qlsp_coefficients[i];

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
/* Interpolation & LSP -> LPC conversion */
    for(i=0;i<n_subframes;i++) {
        pan_lsp_interpolation(prev_Qlsp_coefficients, Qlsp_coefficients, 
            int_Qlsp_coefficients, lpc_order, n_subframes, i);

        for(j=0;j<lpc_order;j++) int_Qlsp_coefficients[j] *= (float)PAN_PI;

        lsf2pc(tmp_lpc_coefficients, int_Qlsp_coefficients, lpc_order);

        for(j=0;j<lpc_order;j++) 
            int_Qlpc_coefficients[lpc_order*i+j] 
                    = -tmp_lpc_coefficients[j+1];

    }

    pan_mv_fdata(prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);

    free(lsp_coefficients);
    free(Qlsp_coefficients);
    free(int_Qlsp_coefficients);
    free(tmp_lpc_coefficients);
    free(lsp_weight);
    free(d_lsp);
}

void bws_lpc_quantizer(
        float   lpc_coefficients_16[],                  
        float   int_Qlpc_coefficients_16[],     
        long    lpc_indices_16[],                               
        long    lpc_order_8,
        long    lpc_order_16,
        long    num_lpc_indices_16,
        long    n_lpc_analysis_16,
        long    n_subframes_16,
        float   buf_Qlsp_coefficients_8[],                  
        float   prev_Qlsp_coefficients_16[],
        long    frame_bit_allocation[],
        float   *cng_lsp_coeff
)
{
        float   *lsp_coefficients_16;
        float   *Qlsp_coefficients_16;
        float   *int_Qlsp_coefficients_16;
        float   *tmp_lpc_coefficients_16;
        long    i,j;
        static short SID_init_flag=1;
        static float SID_lsp_coeff_bws[SID_LPC_ORDER_WB];
        static float prev_lsp_coeff[SID_LPC_ORDER_WB];
        static short lsp_sm_flag = 0;
        static float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];

        if(SID_init_flag){
            for(i=0; i<SID_LPC_ORDER_WB; i++){
                SID_lsp_coeff_bws[i]
                   = (float)(PAN_PI*(i+1)/(float)(SID_LPC_ORDER_WB+1));
                prev_lsp_coeff[i] 
                   = (float)(PAN_PI*(i+1)/(float)(SID_LPC_ORDER_WB+1));
            }
	    for ( i = 0; i < NEC_LSPPRDCT_ORDER; i++ ) {
		for ( j = 0; j < lpc_order_16; j++ ) {
		    if ( j >= lpc_order_8 )
			blsp[i][j]=(float)NEC_PAI/(float)(lpc_order_16+1)*(j+1);
		    else
			blsp[i][j]=0.0;
	        }
	    }
            SID_init_flag=0;
        }
        
        /*Memory allocation*/
        if((lsp_coefficients_16 = (float *)calloc(lpc_order_16, sizeof(float))) == NULL){
	   printf("\nMemory allocation err in lpc_quantizer_16.\n");
	   exit(1);
        }
        if((Qlsp_coefficients_16 = (float *)calloc(lpc_order_16, sizeof(float))) == NULL){
	   printf("\nMemory allocation err in lpc_quantizer_16.\n");
	   exit(1);
        }
        if((int_Qlsp_coefficients_16 = (float *)calloc(lpc_order_16, sizeof(float))) == NULL){
	   printf("\nMemory allocation err in lpc_quantizer_16.\n");
	   exit(1);
        }
        if((tmp_lpc_coefficients_16 = (float *)calloc(lpc_order_16 + 1, sizeof(float))) == NULL){
	   printf("\nMemory allocation err in lpc_quantizer_16.\n");
	   exit(1);
        }
        
        /*reverse the sign of input LPCs*/
        tmp_lpc_coefficients_16[0] = 1.;
        for( i = 0; i < lpc_order_16; i++)
	   tmp_lpc_coefficients_16[i+1] =  -lpc_coefficients_16[lpc_order_16 * (n_lpc_analysis_16 -1) + i];
                
        if(TX_flag_enc == 1){
	    /*LPC -> LSP*/
	    pc2lsf(lsp_coefficients_16, tmp_lpc_coefficients_16, lpc_order_16);
	}else{
            for(i=0; i<lpc_order_16; i++){
                lsp_coefficients_16[i] = (float)(cng_lsp_coeff[i]*PAN_PI);
            }
	}
        
        if(TX_flag_enc == 1){
	    lsp_sm_flag = 1;
        /*Quantizetion*/
        nec_bws_lsp_quantizer(lsp_coefficients_16, buf_Qlsp_coefficients_8,
			      Qlsp_coefficients_16, blsp, lpc_indices_16,
			      frame_bit_allocation,
			      lpc_order_16, lpc_order_8);
        }else if(TX_flag_enc == 2){
            nec_cng_bws_lsp_quantizer(lsp_coefficients_16,
                              buf_Qlsp_coefficients_8,
                              Qlsp_coefficients_16, blsp, lpc_indices_16,
                              frame_bit_allocation,
                              lpc_order_16, lpc_order_8);
            for(i=0; i<lpc_order_16; i++){
                SID_lsp_coeff_bws[i] = Qlsp_coefficients_16[i];
            }
            if(lsp_sm_flag == 1){
                for(i=0; i<lpc_order_16; i++){
                    Qlsp_coefficients_16[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*Qlsp_coefficients_16[i];
                }
            }else lsp_sm_flag = 1;
        }else if(TX_flag_enc == 0 || TX_flag_enc == 3){
            for(i=0; i<lpc_order_16; i++){
                Qlsp_coefficients_16[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*SID_lsp_coeff_bws[i];
            }
	    nec_cng_bws_lspvec_renew( buf_Qlsp_coefficients_8,
				      Qlsp_coefficients_16,
				      blsp,
				      lpc_order_16, lpc_order_8 );
            for(i=0; i<num_lpc_indices_16; i++){
                lpc_indices_16[i] = 0;
            }
        }
        for(i=0; i<lpc_order_16; i++){
            prev_lsp_coeff[i] = Qlsp_coefficients_16[i];
        }
                
        /*Interpolation & LSP -> LPC conversion*/
        for( i = 0; i < n_subframes_16; i++){
	   pan_lsp_interpolation(prev_Qlsp_coefficients_16,
				 Qlsp_coefficients_16, 
				 int_Qlsp_coefficients_16,
				 lpc_order_16, n_subframes_16, i);
	   lsf2pc(tmp_lpc_coefficients_16, int_Qlsp_coefficients_16, lpc_order_16);
	   for( j = 0; j < lpc_order_16; j++)
	      int_Qlpc_coefficients_16[lpc_order_16 * i + j] = -tmp_lpc_coefficients_16[j+1];
        }
        
        pan_mv_fdata(prev_Qlsp_coefficients_16, Qlsp_coefficients_16, lpc_order_16);

        free(lsp_coefficients_16);
        free(Qlsp_coefficients_16);
        free(int_Qlsp_coefficients_16);
        free(tmp_lpc_coefficients_16);
        
        return;
}       


void nb_abs_excitation_analysis (
	float PP_InputSignal[],        /* in: preprocessed input signal */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	float Wnum_coeff[],            /* in: weighting coeff.(numerator) */
	float Wden_coeff[],            /* in: weighting coeff.(denominator) */
	long frame_size,               /* in: frame size */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	long *signal_mode,             /* out: signal mode */
	long frame_bit_allocation[],   /* in: bit number for each index */
	long shape_indices[],          /* out: shape code indices */
	long gain_indices[],           /* out: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
	long num_gain_cbks,            /* in: number of gain codebooks */
	long *rms_index,               /* out: RMS code index */
	float decoded_excitation[],    /* out: decoded excitation */
	long num_enhstages,
	float bws_mp_exc[],
        long SampleRateMode,
        float *mem_past_syn,            /* in/out */
        long  *flag_rms,                /* in/out */
        float *pqxnorm,                 /* in/out */
        short prev_vad                  /* in */
)
{
/* local */
    float *RevIntQLPCoef;
    float *RevWnumCoef;
    float *RevWdenCoef;
    long i;
    static long c_subframe = 0;

    long num_lpc_indices;

    if(fs8kHz==SampleRateMode) {
        num_lpc_indices = PAN_NUM_LPC_INDICES;
    }else {
        num_lpc_indices = PAN_NUM_LPC_INDICES_W;
    }

    c_subframe = c_subframe % n_subframes;
/* memory allocation */
    if((RevIntQLPCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(1);
    }
    if((RevWnumCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(2);
    }
    if((RevWdenCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(3);
    }

/* reverse the sign of LP parameters */
    for(i=0;i<lpc_order*n_subframes;i++) *(RevIntQLPCoef+i) = -(*(int_Qlpc_coefficients+i));
    for(i=0;i<lpc_order*n_subframes;i++) *(RevWnumCoef+i) = -(*(Wnum_coeff+i));
    for(i=0;i<lpc_order*n_subframes;i++) *(RevWdenCoef+i) = -(*(Wden_coeff+i));

    nec_abs_excitation_analysis (
	PP_InputSignal,            /* in: preprocessed input signal */
	RevIntQLPCoef,             /* in: interpolated LPC */
	RevWnumCoef,               /* in: weighting coeff.(numerator) */
	RevWdenCoef,               /* in: weighting coeff.(denominator) */
	shape_indices,             /* out: shape code indices */
	gain_indices,              /* out: gain_code_indices */
	rms_index,                 /* out: RMS code index */
	signal_mode,               /* out: signal mode */
	decoded_excitation,        /* out: decoded excitation */
	lpc_order,                 /* in: order of LPC */
	frame_size,                /* in: frame size */
	sbfrm_size,                /* in: subframe size */
	n_subframes,               /* in: number of subframes */
	frame_bit_allocation+num_lpc_indices,
	num_shape_cbks,            /* in: number of shape codebooks */
	num_gain_cbks,              /* in: number of gain codebooks */
	num_enhstages,             /* in: number of enhancement stages */
        bws_mp_exc,
        SampleRateMode,
        mem_past_syn,              /* in/out */
        flag_rms,                  /* in/out */
        pqxnorm,                   /* in/out */
        prev_vad                   /* in */
				    );
    c_subframe++;
    free(RevIntQLPCoef);
    free(RevWnumCoef);
    free(RevWdenCoef);

}

void bws_excitation_analysis(
	float PP_InputSignal[],        /* in: preprocessed input signal */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	float Wnum_coeff[],            /* in: weighting coeff.(numerator) */
	float Wden_coeff[],            /* in: weighting coeff.(denominator) */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	long signal_mode,              /* in: signal mode */
	long frame_bit_allocation[],   /* in: bit number for each index */
	long shape_indices[],          /* out: shape code indices */
	long gain_indices[],           /* out: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
	long num_gain_cbks,            /* in: number of gain codebooks */
	float decoded_excitation[],    /* out: decoded excitation */
        float bws_mp_exc[],
        long  *acb_index_8,
	long  rms_index,		/* in: RMS code index */
        float *mem_past_syn,            /* in/out */
        long  *flag_rms,                /* in/out */
        float *pqxnorm,                 /* in/out */
        short prev_vad                  /* in */
)
{
/* local */
    float *RevIntQLPCoef;
    float *RevWnumCoef;
    float *RevWdenCoef;
    long i;
    static long c_subframe = 0;

    c_subframe = c_subframe % n_subframes;
/* memory allocation */
    if((RevIntQLPCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(1);
    }
    if((RevWnumCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(2);
    }
    if((RevWdenCoef=(float *)calloc(lpc_order*n_subframes, 
            sizeof(float)))==NULL) {
        printf("\n memory allocation error in excitation analysis\n");
        exit(3);
    }

/* reverse the sign of LP parameters */
    for(i=0;i<lpc_order*n_subframes;i++) *(RevIntQLPCoef+i) = -(*(int_Qlpc_coefficients+i));
    for(i=0;i<lpc_order*n_subframes;i++) *(RevWnumCoef+i) = -(*(Wnum_coeff+i));
    for(i=0;i<lpc_order*n_subframes;i++) *(RevWdenCoef+i) = -(*(Wden_coeff+i));

    nec_bws_excitation_analysis (
	PP_InputSignal,            /* in: preprocessed input signal */
	RevIntQLPCoef,             /* in: interpolated LPC */
	RevWnumCoef,               /* in: weighting coeff.(numerator) */
	RevWdenCoef,               /* in: weighting coeff.(denominator) */
	shape_indices,             /* out: shape code indices */
	gain_indices,              /* out: gain_code_indices */
	decoded_excitation,        /* out: decoded excitation */
	lpc_order,                 /* in: order of LPC */
	sbfrm_size,                /* in: subframe size */
	n_subframes,               /* in: number of subframes */
	frame_bit_allocation,
	num_shape_cbks,            /* in: number of shape codebooks */
	num_gain_cbks,              /* in: number of gain codebooks */
	bws_mp_exc,		/* input */
	acb_index_8,		/* input */
	rms_index,			/* in: RMS code index */
	signal_mode,		/* in: signal mode */
        mem_past_syn,           /* in/out */
        flag_rms,               /* in/out */
        pqxnorm,                /* in/out */
        prev_vad                /* in */
				    );
    c_subframe++;
    free(RevIntQLPCoef);
    free(RevWnumCoef);
    free(RevWdenCoef);

}

/* LSP VQ modules for wideband 980107 */
void wb_celp_lsp_quantizer (
    float lpc_coefficients[],		/* in: LPC */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC */ 
    long lpc_indices[], 		/* out: LPC code indices */
    long lpc_order,			/* in: order of LPC */
    long n_lpc_analysis,     /* in: number of LP analysis per frame */
    long n_subframes,        /* in: number of subframes */
    float *prev_Qlsp_coefficients,
    float *cng_lsp_coeff

)
{

    #include	"inc_lsp46w.tbl"

    float *lsp_coefficients;
    float *Qlsp_coefficients;
    float *int_Qlsp_coefficients;
    float *tmp_lpc_coefficients;
    long i, j;

    float *lsp_weight;
    float *d_lsp;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;

    long offset;
    long orderLsp;
    static short SID_init_flag=1;
    static float SID_lsp_coeff[SID_LPC_ORDER_WB];
    static float prev_lsp_coeff[SID_LPC_ORDER_WB];
    static short lsp_sm_flag = 0;

    if (SID_init_flag)
    {
        for(i=0; i<SID_LPC_ORDER_WB; i++)
        {
            SID_lsp_coeff[i]  = (float)((float)(i+1)/(float)(SID_LPC_ORDER_WB+1));
            prev_lsp_coeff[i] = (float)((float)(i+1)/(float)(SID_LPC_ORDER_WB+1));
        }
        SID_init_flag=0;
    }

/* Memory allocation */
    if((lsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((int_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(3);
	}
    if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(4);
	}
    if((lsp_weight=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(5);
	}
    if((d_lsp=(float *)calloc((lpc_order+1), sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(6);
	}

/* reverse the sign of input LPCs */
/*   Philips: 			A(z) = 1+a1z^(-1)+ ... +apz^(-p) */
/*   AT&T, Panasonic:	A(z) = 1-a1z^(-1)- ... -apz^(-p) */
    tmp_lpc_coefficients[0] = 1.;
    for(i=0;i<lpc_order;i++) 
        tmp_lpc_coefficients[i+1] = 
			-lpc_coefficients[lpc_order*(n_lpc_analysis-1)+i];

/* LPC -> LSP */
    if(TX_flag_enc == 1){
	pc2lsf(lsp_coefficients, tmp_lpc_coefficients, lpc_order);
	for(i=0;i<lpc_order;i++) lsp_coefficients[i] /= (float)PAN_PI;
    }else{
        for(i=0;i<lpc_order;i++){
            lsp_coefficients[i] = cng_lsp_coeff[i];
        }
    }

/* Weighting factor */
    d_lsp[0] = lsp_coefficients[0];
    for(i=1;i<lpc_order;i++) { 
        d_lsp[i] = lsp_coefficients[i]-lsp_coefficients[i-1];
    }
    d_lsp[lpc_order] = (float)(1.-lsp_coefficients[lpc_order-1]);
    for(i=0;i<=lpc_order;i++) {
        if(d_lsp[i]<PAN_MINGAP_CELP_W) d_lsp[i] = PAN_MINGAP_CELP_W;
    }
 
    for(i=0;i<=lpc_order;i++) d_lsp[i] = (float)(1./d_lsp[i]);
    for(i=0;i<lpc_order;i++) {
        lsp_weight[i] = d_lsp[i]+d_lsp[i+1];
    }

    if(TX_flag_enc==1){
	lsp_sm_flag = 1;

/* Quantization - lower part */
    orderLsp = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_L;
    d_tbl = d_tbl46w_L;
    pd_tbl = pd_tbl46w_L;
    dim_1 = dim46w_L1;
    dim_2 = dim46w_L2;
    ncd_1 = ncd46w_L1;
    ncd_2 = ncd46w_L2;
    pan_lspqtz2_dd(lsp_coefficients, 
        prev_Qlsp_coefficients, Qlsp_coefficients, 
        lsp_weight, PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, 
        orderLsp, PAN_N_DC_LSP_CELP_W,     
        lpc_indices, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 0);

/* Quantization - upper part */
    orderLsp = dim46w_U1[0]+dim46w_U1[1];
    offset = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_U;
    d_tbl = d_tbl46w_U;
    pd_tbl = pd_tbl46w_U;
    dim_1 = dim46w_U1;
    dim_2 = dim46w_U2;
    ncd_1 = ncd46w_U1;
    ncd_2 = ncd46w_U2;
    pan_lspqtz2_dd(lsp_coefficients+offset, 
        prev_Qlsp_coefficients+offset, Qlsp_coefficients+offset, 
        lsp_weight+offset, PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, 
        orderLsp, PAN_N_DC_LSP_CELP_W,     
        lpc_indices+5, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 0);
    }else if(TX_flag_enc == 2){
        /* Quantization - lower part */
        orderLsp = dim46w_L1[0]+dim46w_L1[1];
        lsp_tbl = lsp_tbl46w_L;
        d_tbl = d_tbl46w_L;
        dim_1 = dim46w_L1;
        dim_2 = dim46w_L2;
        ncd_1 = ncd46w_L1;
        ncd_2 = ncd46w_L2;
        nec_cng_lspenc_wb_low(lsp_coefficients, Qlsp_coefficients, 
                              lsp_weight, PAN_MINGAP_CELP_W, 
                              orderLsp, PAN_N_DC_LSP_CELP_W,     
                              lpc_indices, lsp_tbl, d_tbl,
                              dim_1, ncd_1, dim_2, ncd_2, 0);
        /* Quantization - upper part */
        orderLsp = dim46w_U1[0]+dim46w_U1[1];
        offset = dim46w_L1[0]+dim46w_L1[1];
        lsp_tbl = lsp_tbl46w_U;
        d_tbl = d_tbl46w_U;
        dim_1 = dim46w_U1;
        dim_2 = dim46w_U2;
        ncd_1 = ncd46w_U1;
        ncd_2 = ncd46w_U2;
        nec_cng_lspenc_wb_high(lsp_coefficients+offset,
                               Qlsp_coefficients+offset, 
                               lsp_weight+offset, PAN_MINGAP_CELP_W, 
                               orderLsp, 1, lpc_indices+5, lsp_tbl, 
                               dim_1, ncd_1, 0);
        for(i=0; i<lpc_order; i++){
            SID_lsp_coeff[i] = Qlsp_coefficients[i];
        }
        if(lsp_sm_flag == 1){
            for(i=0; i<lpc_order; i++){
                Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*Qlsp_coefficients[i];
            }
        }else lsp_sm_flag = 1;
    }else if(TX_flag_enc == 0 || TX_flag_enc == 3){
        for(i=0; i<lpc_order; i++){
            Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
                                    +(1-LSP_SM_COEFF)*SID_lsp_coeff[i];
        }
        for(i=0; i<NUM_LSP_INDICES_WB; i++){
            lpc_indices[i] = 0;
        }
    }
    for(i=0; i<lpc_order; i++) prev_lsp_coeff[i] = Qlsp_coefficients[i];

    pan_stab(SID_lsp_coeff, PAN_MINGAP_CELP_W, lpc_order);

    pan_stab(Qlsp_coefficients, PAN_MINGAP_CELP_W, lpc_order);

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
/* Interpolation & LSP -> LPC conversion */
    for(i=0;i<n_subframes;i++) {
        pan_lsp_interpolation(prev_Qlsp_coefficients, Qlsp_coefficients, 
            int_Qlsp_coefficients, lpc_order, n_subframes, i);

        for(j=0;j<lpc_order;j++) int_Qlsp_coefficients[j] *= (float)PAN_PI;

        lsf2pc(tmp_lpc_coefficients, int_Qlsp_coefficients, lpc_order);

        for(j=0;j<lpc_order;j++) 
            int_Qlpc_coefficients[lpc_order*i+j] 
                = -tmp_lpc_coefficients[j+1];
    }

    pan_mv_fdata(prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);

    free(lsp_coefficients);
    free(Qlsp_coefficients);
    free(int_Qlsp_coefficients);
    free(tmp_lpc_coefficients);
    free(lsp_weight);
    free(d_lsp);
}

