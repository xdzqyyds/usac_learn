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
 *        Last Modified: 10/28/96  N.Tanaka (Panasonic)
 *                       01/13/97  N.Tanaka (Panasonic)
 *                       02/27/97  T.Nomura (NEC)
 *                       06/18/97  N.Tanaka (Panasonic)
 *                                                    
 *  Used Modules:
 *    abs_bitstream_demux        : NEC/Panasonic
 *    abs_lpc_decode             : Panasonic
 *     (LPC-LSP conversion)      : AT&T
 *    abs_excitation_generation  : NEC
 *    abs_lpc_synthesis_filter   : Philips
 *    abs_postprocessing         : AT&T
 *
 *    >>abs_lpc_interpolation is included in abs_lpc_decode module.
 *
 *  History:
 *    Ver. 0.01  07/03/96       Born
 *    Ver. 1.10  07/25/96       Panasonic LPC quantizer
 *    Ver. 1.11  08/06/96       I/F revision
 *    Ver. 1.12  09/04/96   I/F revision
 *    Ver. 1.13  09/24/96   I/F revision
 *    Ver. 2.00  11/07/96   Module replacement & I/F revisions
 *    Ver. 2.11  01/13/96   Added 22bits ver. of Panasonic LSPVQ
 *    Ver. 3.00  02/27/97   Added NEC Rate Control Functionality
 *    Ver. 3.01  06/18/97   Panasonic LSPQ in source code 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common_m4a.h"

/* #include "libtsp.h" */       /* HP 971117 */

#include "bitstream.h"
#include "lpc_common.h"
#include "celp_proto_dec.h"

/* for Panasonic modules */
#include        "pan_celp_const.h"
#include        "pan_celp_proto.h"

/* for AT&T modules */
#include        "att_proto.h"

/* for Philips modules */
/* Modified AG: 28-nov-96 */
#include        "phi_cons.h"
#include        "phi_lpc.h"

/* for NEC modules */
#include        "nec_abs_const.h"
#include        "nec_abs_proto.h"
#ifdef VERSION2_EC
#include        "err_concealment.h"
#endif
#include        "nec_vad.h"

extern unsigned long TX_flag_dec;

/* -------------- */
/* Initialization */
/* -------------- */

void nb_abs_lpc_decode(
                       unsigned long lpc_indices[],     /* in: LPC code indices */
                       float int_Qlpc_coefficients[],   /* out: quantized & interpolated LPC*/ 
                       long lpc_order,                  /* in: order of LPC */
                       long n_subframes,                   /* in: number of subframes */
                       float *prev_Qlsp_coefficients
                       )
{

#include        "inc_lsp22.tbl"

  float *Qlsp_coefficients;
  float *int_Qlsp_coefficients;
  float *tmp_lpc_coefficients;
  long i, j;

  static float p_factor=PAN_LSP_AR_R_CELP;
  static float min_gap=PAN_MINGAP_CELP;

  float *lsp_tbl;
  float *d_tbl;
  float *pd_tbl;
  long *dim_1;
  long *dim_2;
  long *ncd_1;
  long *ncd_2;

  static int   SID_init_flag=1;
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
  if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    CommonExit(1,"celp error");
  }
  if((int_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    exit(2);
  }
  if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    exit(3);
  }

  /* LSP decode */
  if(TX_flag_dec==1){
    lsp_tbl = lsp_tbl22;
    d_tbl = d_tbl22;
    pd_tbl = pd_tbl22;
    dim_1 = dim22_1;
    dim_2 = dim22_2;
    ncd_1 = ncd22_1;
    ncd_2 = ncd22_2;
    pan_lspdec(prev_Qlsp_coefficients, Qlsp_coefficients, 
               p_factor, min_gap, lpc_order, lpc_indices, 
               lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 1, 1);
#ifdef VERSION2_EC      /* SC: 1, EC: LSP, NB */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients, prev_Qlsp_coefficients, lpc_order);
#endif
    lsp_sm_flag = 1;
  }else if(TX_flag_dec == 2){
    lsp_tbl = lsp_tbl22;
    d_tbl = d_tbl22;
    dim_1 = dim22_1;
    dim_2 = dim22_2;
    ncd_1 = ncd22_1;
    ncd_2 = ncd22_2;

    nec_cng_lspdec_nb(Qlsp_coefficients, min_gap, lpc_order, lpc_indices, 
                      lsp_tbl, d_tbl, dim_1, ncd_1, dim_2, ncd_2, 1);

#ifdef VERSION2_EC      /* SC: 2, EC: LSP, NB */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients, prev_Qlsp_coefficients, lpc_order);
#endif
    for(i=0; i<lpc_order; i++){
      SID_lsp_coeff[i] = Qlsp_coefficients[i];
    }
    if(lsp_sm_flag == 1){
      for(i=0; i<lpc_order; i++){
        Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
          +(1-LSP_SM_COEFF)*Qlsp_coefficients[i];
      }
    }else lsp_sm_flag = 1;
  }else if(TX_flag_dec == 0 || TX_flag_dec == 3){
    for(i=0; i<lpc_order; i++){
      Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
        +(1-LSP_SM_COEFF)*SID_lsp_coeff[i];
    }
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

    /* reverse the sign of input LPCs */
    /*   Philips:                       A(z) = 1+a1z^(-1)+ ... +apz^(-p) */
    /*   AT&T, Panasonic:       A(z) = 1-a1z^(-1)+ ... +apz^(-p) */

    for(j=0;j<lpc_order;j++) 
      int_Qlpc_coefficients[lpc_order*i+j] 
        = -tmp_lpc_coefficients[j+1];

  }

  pan_mv_fdata(prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);

  free(Qlsp_coefficients);
  free(int_Qlsp_coefficients);
  free(tmp_lpc_coefficients);
}

void bws_lpc_decoder(
                     unsigned long    lpc_indices_16[],                               
                     float   int_Qlpc_coefficients_16[],     
                     long    lpc_order_8,
                     long    lpc_order_16,
                     long    n_subframes_16,
                     float   buf_Qlsp_coefficients_8[],     
                     float   prev_Qlsp_coefficients_16[]
                     )
{
  float   *Qlsp_coefficients_16;
  float   *int_Qlsp_coefficients_16;
  float   *tmp_lpc_coefficients_16;
  long    i,j;
  static int   SID_init_flag=1;
  static float SID_lsp_coeff[SID_LPC_ORDER_WB];
  static float prev_lsp_coeff[SID_LPC_ORDER_WB];
  static short lsp_sm_flag = 0;
  static float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];
        
  if (SID_init_flag)
    {
      for (i=0; i<SID_LPC_ORDER_WB; i++)
        {
          SID_lsp_coeff[i] 
            = (float)(PAN_PI*(i+1)/(float)(SID_LPC_ORDER_WB+1));
          prev_lsp_coeff[i] 
            = (float)(PAN_PI*(i+1)/(float)(SID_LPC_ORDER_WB+1));
        }
      for( i = 0; i < NEC_LSPPRDCT_ORDER; i++ ){
	  for( j = 0; j < lpc_order_16; j++ )
	      if( j >= lpc_order_8 )
	          blsp[i][j] = (float)NEC_PAI/(float)(lpc_order_16+1)*(j+1);
	      else
		  blsp[i][j] = 0.0;
      }

      SID_init_flag=0;
    }
        
  /*Memory allocation*/
  if((Qlsp_coefficients_16 = (float *)calloc(lpc_order_16, sizeof(float))) == NULL){
    printf("\nMemory allocation err in lpc_decoder_16.\n");
    CommonExit(1,"celp error");
  }
  if((int_Qlsp_coefficients_16 = (float *)calloc(lpc_order_16, sizeof(float))) == NULL){
    printf("\nMemory allocation err in lpc_decoder_16.\n");
    CommonExit(1,"celp error");
  }
  if((tmp_lpc_coefficients_16 = (float *)calloc(lpc_order_16 + 1, sizeof(float))) == NULL){
    printf("\nMemory allocation err in lpc_quantizer_16.\n");
    CommonExit(1,"celp error");
  }
        
  /* LSP Decode*/
  if(TX_flag_dec == 1){
    nec_bws_lsp_decoder( lpc_indices_16, buf_Qlsp_coefficients_8,
                         Qlsp_coefficients_16,
#ifdef VERSION2_EC
			 prev_Qlsp_coefficients_16,
#endif
			 blsp,
                         lpc_order_16, lpc_order_8 );
                
#ifdef VERSION2_EC      /* SC: 1, EC: LSP, BWS */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients_16, prev_Qlsp_coefficients_16,
                   lpc_order_16);
#endif
    lsp_sm_flag = 1;
  }else if (TX_flag_dec == 2){
    nec_cng_bws_lsp_decoder(lpc_indices_16, buf_Qlsp_coefficients_8,
                            Qlsp_coefficients_16,
#ifdef VERSION2_EC
			    prev_Qlsp_coefficients_16,
#endif
			    blsp,
                            lpc_order_16, lpc_order_8 );
            
#ifdef VERSION2_EC      /* SC: 2, EC: LSP, BWS */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients_16, prev_Qlsp_coefficients_16,
                   lpc_order_16);
#endif
    for (i=0; i<lpc_order_16; i++){
      SID_lsp_coeff[i] = Qlsp_coefficients_16[i];
    }

    if (lsp_sm_flag == 1){
      for(i=0; i<lpc_order_16; i++){
        Qlsp_coefficients_16[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
          +(1-LSP_SM_COEFF)*Qlsp_coefficients_16[i];
      }
    }else{
      lsp_sm_flag = 1;
    }
  }else if(TX_flag_dec == 0 || TX_flag_dec == 3){
    for(i=0; i<lpc_order_16; i++){
      Qlsp_coefficients_16[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
        +(1-LSP_SM_COEFF)*SID_lsp_coeff[i];
    }
    nec_cng_bws_lspvec_renew( buf_Qlsp_coefficients_8,
			      Qlsp_coefficients_16,
			      blsp,
			      lpc_order_16, lpc_order_8 );
  }

  for (i=0; i<lpc_order_16; i++){
    prev_lsp_coeff[i] = Qlsp_coefficients_16[i];
  }
  /*Interpolation & LSP -> LPC conversion*/
  for( i = 0; i < n_subframes_16; i++){
    pan_lsp_interpolation(prev_Qlsp_coefficients_16,
                          Qlsp_coefficients_16, 
                          int_Qlsp_coefficients_16,
                          lpc_order_16, n_subframes_16, i);
    lsf2pc(tmp_lpc_coefficients_16, int_Qlsp_coefficients_16,
           lpc_order_16);
    for( j = 0; j < lpc_order_16; j++)
      int_Qlpc_coefficients_16[lpc_order_16 * i + j] = -tmp_lpc_coefficients_16[j+1];
  }
        
  pan_mv_fdata(prev_Qlsp_coefficients_16, Qlsp_coefficients_16, lpc_order_16);
        
  free(Qlsp_coefficients_16);
  free(int_Qlsp_coefficients_16);
  free(tmp_lpc_coefficients_16);
        
  return;
}       

void nb_abs_excitation_generation(
                                  unsigned long shape_indices[],          /* in: shape code indices */
                                  unsigned long gain_indices[],           /* in: gain code indices */
                                  long num_shape_cbks,           /* in: number of shape codebooks */
                                  long num_gain_cbks,            /* in: number of gain codebooks */
                                  unsigned long rms_index,                /* in: RMS code index */
                                  float int_Qlpc_coefficients[], /* in: interpolated LPC */
                                  long lpc_order,                /* in: order of LPC */
                                  long sbfrm_size,               /* in: subframe size */
                                  long n_subframes,              /* in: number of subframes */
                                  unsigned long signal_mode,              /* in: signal mode */
                                  long org_frame_bit_allocation[],   /* in: bit number for each index */
                                  float excitation[],            /* out: decoded excitation */
                                  float bws_mp_exc[],            /* out: decoded excitation */
                                  long *acb_delay,               /* out: adaptive code delay */
                                  float *adaptive_gain,          /* out: adaptive code gain */
                                  long dec_enhstages,
                                  long postfilter,
                                  long SampleRateMode,
                                  long *flag_rms,                 /* in/out */
                                  float *pqxnorm,                 /* in/out */
                                  short prev_vad                  /* in */
                                  )
{

  float *tmp_lpc_coefficients;
  long i;

  long num_lpc_indices;

  if(fs8kHz==SampleRateMode) {
    num_lpc_indices = PAN_NUM_LPC_INDICES;
  }else {
    num_lpc_indices = PAN_NUM_LPC_INDICES_W;
  }

  if((tmp_lpc_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_exc_generation\n");
    CommonExit(1,"celp error");
  }

  for(i=0;i<lpc_order;i++) 
    tmp_lpc_coefficients[i] = -int_Qlpc_coefficients[i];

  nec_abs_excitation_generation(
                                tmp_lpc_coefficients,  /* in: interpolated LPC */
                                shape_indices,         /* in: shape code indices */
                                gain_indices,          /* in: gain code indices */
                                rms_index,             /* in: RMS code index */
                                signal_mode,             /* in: signal mode */
                                excitation,            /* out: decoded excitation */
                                adaptive_gain,         /* out: adaptive code gain */
                                acb_delay,             /* out: adaptive code delay */
                                lpc_order,             /* in: order of LPC */
                                sbfrm_size,            /* in: subframe size */
                                n_subframes,           /* in: number of subframes */
                                org_frame_bit_allocation+num_lpc_indices,
                                num_shape_cbks,        /* in: number of shape codebooks */
                                num_gain_cbks,          /* in: number of gain codebooks */
                                dec_enhstages,             /* in: number of enhancement stages */
                                bws_mp_exc,
                                postfilter,
                                SampleRateMode,
                                flag_rms,                /* in/out */
                                pqxnorm,                 /* in/out */
                                prev_vad                 /* in */
                                );
  free(tmp_lpc_coefficients);

}

void bws_excitation_generation(
                               unsigned long shape_indices[],          /* in: shape code indices */
                               unsigned long gain_indices[],           /* in: gain code indices */
                               long num_shape_cbks,           /* in: number of shape codebooks */
                               long num_gain_cbks,            /* in: number of gain codebooks */
                               unsigned long rms_index,                /* in: RMS code index */
                               float int_Qlpc_coefficients[], /* in: interpolated LPC */
                               long lpc_order,                /* in: order of LPC */
                               long sbfrm_size,               /* in: subframe size */
                               long n_subframes,              /* in: number of subframes */
                               unsigned long signal_mode,              /* in: signal mode */
                               long org_frame_bit_allocation[],   /* in: bit number for each index */
                               float excitation[],            /* out: decoded excitation */
                               float bws_mp_exc[],            /* in: decoded mp excitation */
                               long acb_indx_8[],           /* in: acb_delay */
                               long *acb_delay,               /* out: adaptive code delay */
                               float *adaptive_gain,           /* out: adaptive code gain */
                               long postfilter,
                               long  *flag_rms,                /* in/out */
                               float *pqxnorm,                 /* in/out */
                               short prev_vad                  /* in */
                               )
{
  
  float *tmp_lpc_coefficients;
  long i;
  
  if((tmp_lpc_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_exc_generation\n");
    CommonExit(1,"celp error");
  }
  
  for(i=0;i<lpc_order;i++) 
    tmp_lpc_coefficients[i] = -int_Qlpc_coefficients[i];
  
  nec_bws_excitation_generation(
                                tmp_lpc_coefficients,  /* in: interpolated LPC */
                                shape_indices,         /* in: shape code indices */
                                gain_indices,          /* in: gain code indices */
                                rms_index,             /* in: RMS code index */
                                signal_mode,           /* in: signal_mode */
                                excitation,            /* out: decoded excitation */
                                adaptive_gain,         /* out: adaptive code gain */
                                acb_delay,             /* out: adaptive code delay */
                                lpc_order,             /* in: order of LPC */
                                sbfrm_size,            /* in: subframe size */
                                n_subframes,           /* in: number of subframes */
                                org_frame_bit_allocation,
                                num_shape_cbks,        /* in: number of shape codebooks */
                                num_gain_cbks,         /* in: number of gain codebooks */
                                bws_mp_exc,
                                acb_indx_8, postfilter,
                                flag_rms,
                                pqxnorm,
                                prev_vad
                                );
  free(tmp_lpc_coefficients);

}

void nb_abs_postprocessing(
                           float synth_signal[],          /* input */
                           float PP_synth_signal[],       /* output */
                           float int_Qlpc_coefficients[], /* input */
                           long lpc_order,                /* configuration input */
                           long sbfrm_sizes               /* configuration input */
                           )
{
  
  float *tmp_lpc_coefficients;
  long i;
  
  if((tmp_lpc_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_postprocessing\n");
    CommonExit(1,"celp error");
  }
  
  for(i=0;i<lpc_order;i++) 
    tmp_lpc_coefficients[i] = -int_Qlpc_coefficients[i];
  
  att_abs_postprocessing(
                         synth_signal,           /* input */
                         PP_synth_signal,        /* output */
                         tmp_lpc_coefficients,   /* input */
                         lpc_order,              /* configuration input */
                         sbfrm_sizes             /* configuration input */
                         );
  
  free(tmp_lpc_coefficients);
  
}

void wb_celp_lsp_decode(
                        unsigned long lpc_indices[],    /* in: LPC code indices */
                        float int_Qlpc_coefficients[],        /* out: quantized & interpolated LPC*/ 
                        long lpc_order,                               /* in: order of LPC */
                        long n_subframes,               /* in: number of subframes */
                        float *prev_Qlsp_coefficients
                        )
{

#include        "inc_lsp46w.tbl"

  float *Qlsp_coefficients;
  float *int_Qlsp_coefficients;
  float *tmp_lpc_coefficients;
  long i, j;

  float *lsp_tbl;
  float *d_tbl;
  float *pd_tbl;
  long *dim_1;
  long *dim_2;
  long *ncd_1;
  long *ncd_2;

  long offset;
  long orderLsp;
#ifdef VERSION2_EC      /* SC: 0123, EC: LSP, WB */
  static float raw_prev_Qlsp_coefficients[NEC_LPC_ORDER_FRQ16];/* not stabilized */
#endif
  static int   SID_init_flag=1;
  static float SID_lsp_coeff[SID_LPC_ORDER_WB];
  static float prev_lsp_coeff[SID_LPC_ORDER_WB];
  static short lsp_sm_flag = 0;

  if(SID_init_flag){
    for(i=0; i<SID_LPC_ORDER_WB; i++){
      SID_lsp_coeff[i] 
        = (float)((float)(i+1)/(float)(SID_LPC_ORDER_WB+1));
      prev_lsp_coeff[i]
        = (float)((float)(i+1)/(float)(SID_LPC_ORDER_WB+1));
    }
#ifdef VERSION2_EC	/* SC: 0123, EC: LSP, WB */
    pan_mv_fdata( raw_prev_Qlsp_coefficients, prev_Qlsp_coefficients, lpc_order);
#endif
    SID_init_flag=0;
  }

  /* Memory allocation */
  if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    CommonExit(1,"celp error");
  }
  if((int_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    CommonExit(2,"celp error");
  }
  if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in abs_lpc_quantizer\n");
    CommonExit(3,"celp error");
  }

  if(TX_flag_dec == 1){
    /* LSP decode - lower part */
    orderLsp = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_L;
    d_tbl = d_tbl46w_L;
    pd_tbl = pd_tbl46w_L;
    dim_1 = dim46w_L1;
    dim_2 = dim46w_L2;
    ncd_1 = ncd46w_L1;
    ncd_2 = ncd46w_L2;
    pan_lspdec(prev_Qlsp_coefficients, Qlsp_coefficients, 
               PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, orderLsp, lpc_indices, 
               lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 0, 1);

    /* LSP decode - upper part */
    offset = dim46w_L1[0]+dim46w_L1[1];
    orderLsp = dim46w_U1[0]+dim46w_U1[1];
    lsp_tbl = lsp_tbl46w_U;
    d_tbl = d_tbl46w_U;
    pd_tbl = pd_tbl46w_U;
    dim_1 = dim46w_U1;
    dim_2 = dim46w_U2;
    ncd_1 = ncd46w_U1;
    ncd_2 = ncd46w_U2;
    pan_lspdec(prev_Qlsp_coefficients+offset, Qlsp_coefficients+offset, 
               PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, orderLsp, lpc_indices+5, 
               lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 0, 1);

#ifdef VERSION2_EC      /* SC: 1, EC: LSP, WB */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients, raw_prev_Qlsp_coefficients, lpc_order);
    else
      pan_mv_fdata(raw_prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);
#endif
    lsp_sm_flag = 1;
  }else if(TX_flag_dec == 2){
    /* LSP decode - lower part */
    orderLsp = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_L;
    d_tbl = d_tbl46w_L;
    dim_1 = dim46w_L1;
    dim_2 = dim46w_L2;
    ncd_1 = ncd46w_L1;
    ncd_2 = ncd46w_L2;
    nec_cng_lspdec_wb_low(Qlsp_coefficients, PAN_MINGAP_CELP_W, orderLsp,
                          (long*) lpc_indices, lsp_tbl, d_tbl, dim_1, ncd_1, 
                          dim_2, ncd_2, 0);
        
    /* LSP decode - upper part */
    offset = dim46w_L1[0]+dim46w_L1[1];
    orderLsp = dim46w_U1[0]+dim46w_U1[1];
    lsp_tbl = lsp_tbl46w_U;
    d_tbl = d_tbl46w_U;
    dim_1 = dim46w_U1;
    dim_2 = dim46w_U2;
    ncd_1 = ncd46w_U1;
    ncd_2 = ncd46w_U2;
    nec_cng_lspdec_wb_high(Qlsp_coefficients+offset, PAN_MINGAP_CELP_W,
                           orderLsp, (long*) &(lpc_indices[5]), lsp_tbl, dim_1, ncd_1, 0);

#ifdef VERSION2_EC      /* SC: 2, EC: LSP, WB */
    /* error concealment of LSP */
    if( getErrAction() & EA_LSP_PREVIOUS )
      pan_mv_fdata(Qlsp_coefficients, raw_prev_Qlsp_coefficients, lpc_order);
    else
      pan_mv_fdata(raw_prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);
#endif
    for(i=0; i<lpc_order; i++){
      SID_lsp_coeff[i] = Qlsp_coefficients[i];
    }
    if(lsp_sm_flag == 1){
      for(i=0; i<lpc_order; i++){
        Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
          +(1-LSP_SM_COEFF)*Qlsp_coefficients[i];
      }
    }else lsp_sm_flag = 1;
  }else if(TX_flag_dec == 0 || TX_flag_dec == 3){
    for(i=0; i<lpc_order; i++){
      Qlsp_coefficients[i] = LSP_SM_COEFF*prev_lsp_coeff[i]
        +(1-LSP_SM_COEFF)*SID_lsp_coeff[i];
    }
#ifdef VERSION2_EC	/* SC: 03, EC: LSP, WB */
    pan_mv_fdata(raw_prev_Qlsp_coefficients, Qlsp_coefficients, lpc_order);
#endif
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

  free(Qlsp_coefficients);
  free(int_Qlsp_coefficients);
  free(tmp_lpc_coefficients);
}


