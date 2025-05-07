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
*  MPEG-4 Audio Verification Model (LPC-ABS Core)
*  
*  LSP Parameters Encoding and Decoding Subroutines for CNG
*
*  Ver1.0  99.5.10 H.Ito(NEC)
*/

#include <stdio.h>
#include <stdlib.h>
#include "pan_celp_proto.h"
#include "pan_celp_const.h"
#include "att_proto.h"

#include "nec_vad.h"
#ifdef VERSION2_EC
#include "err_concealment.h"
#endif

/**** used in bws_lsp_quantizer ****/
#include "nec_lspnw20.tbl"
#include "nec_abs_proto.h"
#include "nec_abs_const.h"

#define NEC_LSPPRDCT_ORDER      4
#define NEC_NUM_LSPSPLIT1       2
#define NEC_NUM_LSPSPLIT2       4
#define NEC_QLSP_CAND           2
#define NEC_LSP_MINWIDTH_FRQ16  ((float)229/8192)
#define NEC_MAX_LSPVQ_ORDER     20
#define NEC_NUM_CNG_LSPVQ2      2

static void nec_psvq( float *, float *, float *,
                     long, long, float *, long *, long );
static void nec_lsp_sort( float *, long );

static float    nec_lsp_minwidth;


/******************************************************************************/

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
)
{
    float out_v[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_d[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float dist_d1,dist_d2;
    float dist_d_min1,dist_d_min2;
    long i, j;
    long d_idx[2*PAN_N_DC_LSP_MAX];
    long d_idx_tmp1,d_idx_tmp2;
    
    long idx_tmp[2*PAN_N_DC_LSP_MAX];
    long i_tmp[PAN_N_DC_LSP_MAX];
    
    if(lpc_order>PAN_DIM_MAX) {
        printf("\n LSP quantizer: LPC order exceeds the limit\n");
        exit(1);
    }
    if(num_dc>PAN_N_DC_LSP_MAX) {
        printf("\n LSP quantizer: number of delayed candidates exceeds the limit\n");
        exit(1);
    }
    for(i=0;i<PAN_N_DC_LSP_MAX;i++) i_tmp[i]=0;
    for(i=0;i<2*PAN_N_DC_LSP_MAX;i++) d_idx[i]=0;
    
    /* 1st stage vector quantization */
    pan_v_qtz_w_dd(&in[0], i_tmp, ncd_1[0], &tbl[0], dim_1[0], 
        &weight[0], num_dc);
    for(i=0;i<num_dc;i++) idx_tmp[2*i] = i_tmp[i];
    
    pan_v_qtz_w_dd(&in[dim_1[0]], i_tmp, ncd_1[1], 
        &tbl[dim_1[0]*ncd_1[0]], dim_1[1], &weight[dim_1[0]], num_dc);
    for(i=0;i<num_dc;i++) idx_tmp[2*i+1] = i_tmp[i];
    
    /* quantized value */
    for(j=0;j<num_dc;j++) {
        for(i=0;i<dim_1[0];i++) {
            out_v[lpc_order*j+i] = tbl[dim_1[0]*idx_tmp[2*j]+i];
        }
        
        for(i=0;i<dim_1[1];i++) {
            out_v[lpc_order*j+dim_1[0]+i] 
                = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx_tmp[2*j+1]+i];
        }
    }
    
    /* ------------------------------------------------------------------ */
    /* 2nd stage vector quantization */
    
    for(j=0;j<num_dc;j++) { 
        pan_d_qtz_w(&in[0], &out_v[lpc_order*j], &d_idx[2*j], ncd_2[0], 
            &d_tbl[0], dim_2[0], &weight[0]);
        
        /* quantized value */
        
        if(d_idx[2*j]<ncd_2[0]) {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] 
                    = out_v[lpc_order*j+i]+d_tbl[d_idx[2*j]*dim_2[0]+i];
            }
        }else {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] 
                    = out_v[lpc_order*j+i]-d_tbl[(d_idx[2*j]-ncd_2[0])*dim_2[0]+i];
            }
        }
        if(d_idx[2*j+1]<ncd_2[1]) {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i];
            }
        }else {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i];
            }
        }
        
    }
    /* ------------------------------------------------------------------ */
    /* Lower Part of DVQ */
    dist_d_min1 = 1.0e9;
    d_idx_tmp1 = 0;
    for(j=0;j<num_dc;j++) {
        dist_d1 = 0.;
        for(i=0;i<dim_2[0];i++)
            dist_d1 += (in[i]-qv_d[lpc_order*j+i])
            *(in[i]-qv_d[lpc_order*j+i])*weight[i];
        if(dist_d1<dist_d_min1) {
            dist_d_min1 = dist_d1;
            d_idx_tmp1 = j;
        }
    }
    
    /* Upper Part of DVQ */
    dist_d_min2 = 1.0e9;
    d_idx_tmp2 = 0;
    for(j=0;j<num_dc;j++) {
        dist_d2 = 0.;
        for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
            dist_d2 += (in[i]-qv_d[lpc_order*j+i])
            *(in[i]-qv_d[lpc_order*j+i])*weight[i];
        if(dist_d2<dist_d_min2) {
            dist_d_min2 = dist_d2;
            d_idx_tmp2 = j;
        }
    }
    
    for(i=0;i<dim_2[0];i++) out[i] = qv_d[lpc_order*d_idx_tmp1+i];
    for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++) {
        out[i] = qv_d[lpc_order*d_idx_tmp2+i];
    }
    idx[0] = idx_tmp[2*d_idx_tmp1];
    idx[1] = idx_tmp[2*d_idx_tmp2+1];
    idx[2] = d_idx[2*d_idx_tmp1];
    idx[3] = 0;
    idx[4] = 0;
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
    
}

/******************************************************************************/

void nec_cng_lspdec_nb(float out[],          /* out: output LSPs */
                       float min_gap,        /* in: minimum gap between adjacent LSPs */
                       long lpc_order,       /* in: LPC order */
                       unsigned long idx[],  /* in: LSP indicies */
                       float tbl[],          /* in: VQ table */
                       float d_tbl[],        /* in: DVQ table */
                       long dim_1[],         /* in: dimensions of the 1st VQ vectors */
                       long ncd_1[],         /* in: numbers of the 1st VQ codes */
                       long dim_2[],         /* in: dimensions of the 2nd VQ vectors */
                       long ncd_2[],         /* in: numbers od the 2nd VQ codes */
                       long flagStab         /* in: 0 - stabilization OFF, 1 - stabilization ON */
)
{
    long    i;
    
    /* 1st stage quantized value */
    for(i=0;i<dim_1[0];i++) {
        out[i] = tbl[dim_1[0]*idx[0]+i];
    }
    
    for(i=0;i<dim_1[1];i++) {
        out[dim_1[0]+i] 
            = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx[1]+i];
    }
    
    /* 2nd stage quantized value */
    if((int)idx[2]<ncd_2[0]) {
        for(i=0;i<dim_2[0];i++) {
            out[i] = out[i]+d_tbl[idx[2]*dim_2[0]+i];
        }
    }else {
        for(i=0;i<dim_2[0];i++) {
            out[i] = out[i]-d_tbl[(idx[2]-ncd_2[0])*dim_2[0]+i];
        }
    }
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
}

/******************************************************************************/

void nec_cng_lspenc_wb_low(float in[], /* in: input LSPs */
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
)
{
    float out_v[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_d[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float dist_d1, dist_d2;
    float dist_d_min1, dist_d_min2;
    long i, j;
    long d_idx[2*PAN_N_DC_LSP_MAX];
    long d_idx_tmp1,d_idx_tmp2;
    
    long idx_tmp[2*PAN_N_DC_LSP_MAX];
    long i_tmp[PAN_N_DC_LSP_MAX];
    
    if(lpc_order>PAN_DIM_MAX) {
        printf("\n LSP quantizer: LPC order exceeds the limit\n");
        exit(1);
    }
    if(num_dc>PAN_N_DC_LSP_MAX) {
        printf("\n LSP quantizer: number of delayed candidates exceeds the limit\n");
        exit(1);
    }
    for (i=0; i<PAN_N_DC_LSP_MAX;i++) i_tmp[i]=0;
    
    /* 1st stage vector quantization */
    pan_v_qtz_w_dd(&in[0], i_tmp, ncd_1[0], &tbl[0], dim_1[0], 
        &weight[0], num_dc);
    for(i=0;i<num_dc;i++) idx_tmp[2*i] = i_tmp[i];
    
    pan_v_qtz_w_dd(&in[dim_1[0]], i_tmp, ncd_1[1], 
        &tbl[dim_1[0]*ncd_1[0]], dim_1[1], &weight[dim_1[0]], num_dc);
    for(i=0;i<num_dc;i++) idx_tmp[2*i+1] = i_tmp[i];
    
    /* quantized value */
    for(j=0;j<num_dc;j++) {
        for(i=0;i<dim_1[0];i++) {
            out_v[lpc_order*j+i] = tbl[dim_1[0]*idx_tmp[2*j]+i];
        }
        
        for(i=0;i<dim_1[1];i++) {
            out_v[lpc_order*j+dim_1[0]+i] 
                = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx_tmp[2*j+1]+i];
        }
    }
    
    /* ------------------------------------------------------------------ */
    /* 2nd stage vector quantization */
    
    for(j=0;j<num_dc;j++) { 
        pan_d_qtz_w(&in[0], &out_v[lpc_order*j], &d_idx[2*j], ncd_2[0], 
            &d_tbl[0], dim_2[0], &weight[0]);
        
        pan_d_qtz_w(&in[dim_2[0]], &out_v[lpc_order*j+dim_2[0]], &d_idx[2*j+1], 
            ncd_2[1], &d_tbl[dim_2[0]*ncd_2[0]], 
            dim_2[1], &weight[dim_2[0]]);
        
        /* quantized value */
        
        if(d_idx[2*j]<ncd_2[0]) {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]+d_tbl[d_idx[2*j]*dim_2[0]+i];
            }
        }else {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]-d_tbl[(d_idx[2*j]-ncd_2[0])*dim_2[0]+i];
            }
        }
        if(d_idx[2*j+1]<ncd_2[1]) {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
                    + d_tbl[dim_2[0]*ncd_2[0]+d_idx[2*j+1]*dim_2[1]+i];
            }
        }else {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
                    - d_tbl[dim_2[0]*ncd_2[0]
                    + (d_idx[2*j+1]-ncd_2[1])*dim_2[1]+i];
            }
        }
        
    }
    /* ------------------------------------------------------------------ */
    /* Quantization mode selection */
    /* Lower Part of DVQ */
    dist_d_min1 = 1.0e9;
    d_idx_tmp1 = 0;
    for(j=0;j<num_dc;j++) {
        dist_d1 = 0.;
        for(i=0;i<dim_2[0];i++)
            dist_d1 += (in[i]-qv_d[lpc_order*j+i])
            *(in[i]-qv_d[lpc_order*j+i])*weight[i];
        if(dist_d1<dist_d_min1) {
            dist_d_min1 = dist_d1;
            d_idx_tmp1 = j;
        }
    }
    
    /* Upper Part of DVQ */
    dist_d_min2 = 1.0e9;
    d_idx_tmp2 = 0;
    for(j=0;j<num_dc;j++) {
        dist_d2 = 0.;
        for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
            dist_d2 += (in[i]-qv_d[lpc_order*j+i])
            *(in[i]-qv_d[lpc_order*j+i])*weight[i];
        if(dist_d2<dist_d_min2) {
            dist_d_min2 = dist_d2;
            d_idx_tmp2 = j;
        }
    }
    
    for(i=0;i<dim_2[0];i++) out[i] = qv_d[lpc_order*d_idx_tmp1+i];
    for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++){
        out[i] = qv_d[lpc_order*d_idx_tmp2+i];
    }
    idx[0] = idx_tmp[2*d_idx_tmp1];
    idx[1] = idx_tmp[2*d_idx_tmp2+1];
    idx[2] = d_idx[2*d_idx_tmp1];
    idx[3] = d_idx[2*d_idx_tmp2+1];
    idx[4] = 0;
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
    
}

/******************************************************************************/

void nec_cng_lspdec_wb_low(float out[], /* out: output LSPs */
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
)
{
    long    i;
    
    /* 1st stage quantized value */
    for(i=0;i<dim_1[0];i++) {
        out[i] = tbl[dim_1[0]*idx[0]+i];
    }
    for(i=0;i<dim_1[1];i++) {
        out[dim_1[0]+i] 
            = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx[1]+i];
    }
    
    /* 2nd stage quantized value */
    if(idx[2]<ncd_2[0]) {
        for(i=0;i<dim_2[0];i++) {
            out[i] = out[i]+d_tbl[idx[2]*dim_2[0]+i];
        }
    }else {
        for(i=0;i<dim_2[0];i++) {
            out[i] = out[i]-d_tbl[(idx[2]-ncd_2[0])*dim_2[0]+i];
        }
    }
    if(idx[3]<ncd_2[1]) {
        for(i=0;i<dim_2[1];i++) {
            out[dim_2[0]+i] = out[dim_2[0]+i]
                + d_tbl[dim_2[0]*ncd_2[0]+idx[3]*dim_2[1]+i];
        }
    }else {
        for(i=0;i<dim_2[1];i++) {
            out[dim_2[0]+i] = out[dim_2[0]+i]
                - d_tbl[dim_2[0]*ncd_2[0]
                + (idx[3]-ncd_2[1])*dim_2[1]+i];
        }
    }
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
}

/******************************************************************************/

void nec_cng_lspenc_wb_high(float in[],      /* in: input LSPs */
                            float out[],     /* out: output LSPs */
                            float weight[],  /* in: weighting factor */
                            float min_gap,   /* in: minimum gap between adjacent LSPs */
                            long lpc_order,  /* in: LPC order */
                            long num_dc,     /* in: number of delayed candidates for DD */
                            long idx[],      /* out: LSP indicies */
                            float tbl[],     /* in: VQ table */
                            long dim_1[],    /* in: dimensions of the 1st VQ vectors */
                            long ncd_1[],    /* in: numbers of the 1st VQ codes */
                            long flagStab    /* in: 0 - stabilization OFF, 1 - stabilization ON */
)
{
    long i;
    long idx_tmp[2*PAN_N_DC_LSP_MAX];
    long i_tmp[PAN_N_DC_LSP_MAX];
    
    if(lpc_order>PAN_DIM_MAX) {
        printf("\n LSP quantizer: LPC order exceeds the limit\n");
        exit(1);
    }
    
    /* 1st stage vector quantization */
    pan_v_qtz_w_dd(&in[0], i_tmp, ncd_1[0], &tbl[0], dim_1[0], &weight[0], num_dc);
    for(i=0;i<num_dc;i++) idx_tmp[2*i] = i_tmp[i];
    
    pan_v_qtz_w_dd(&in[dim_1[0]], i_tmp, ncd_1[1], &tbl[dim_1[0]*ncd_1[0]], dim_1[1], &weight[dim_1[0]], num_dc);
    
    for(i=0;i<num_dc;i++) idx_tmp[2*i+1] = i_tmp[i];
    
    /* quantized value */
    for(i=0;i<dim_1[0];i++) 
    {
        out[i] = tbl[dim_1[0]*idx_tmp[0]+i];
    }
    
    for(i=0;i<dim_1[1];i++) 
    {
        out[i+dim_1[0]] = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx_tmp[1]+i];
    }
    
    idx[0] = idx_tmp[0];
    idx[1] = idx_tmp[1];
    idx[2] = 0;
    idx[3] = 0;
    idx[4] = 0;
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
}

/******************************************************************************/

void nec_cng_lspdec_wb_high(float out[],     /* out: output LSPs */
                            float min_gap,   /* in: minimum gap between adjacent LSPs */
                            long lpc_order,  /* in: LPC order */
                            long idx[],      /* in: LSP indicies */
                            float tbl[],     /* in: VQ table */
                            long dim_1[],    /* in: dimensions of the 1st VQ vectors */
                            long ncd_1[],    /* in: numbers of the 1st VQ codes */
                            long flagStab    /* in: 0 - stabilization OFF, 1 - stabilization ON */
)
{
    long    i;
    
    /* 1st stage quantized value */
    for(i=0;i<dim_1[0];i++) 
    {
        out[i] = tbl[dim_1[0]*idx[0]+i];
    }
    
    for(i=0;i<dim_1[1];i++) 
    {
        out[dim_1[0]+i] = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx[1]+i];
    }
    
    if(flagStab) pan_stab(out, min_gap, lpc_order);
}

/******************************************************************************/

void nec_cng_bws_lsp_quantizer(float lsp[],                  /* input  */
                               float qlsp8[],                /* input  */
                               float qlsp[],                 /* output  */
                               float blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output  */
                               long indices[],               /* output  */
                               long frame_bit_allocation[],  /* configuration input */
                               long lpc_order,               /* configuration input */
                               long lpc_order_8)             /* configuration input */
{
    long     i, j, k;
    long     cb_size;
    long     cidx = 0;
    long     sp_order;
    long     cand1[NEC_NUM_LSPSPLIT1][NEC_QLSP_CAND];
    long     cand2[NEC_NUM_LSPSPLIT2][NEC_QLSP_CAND];
    float    *qqlsp, *tlsp;
    float    *error, *error2;
    float    *vec_hat, *weight;
    float    mindist, dist, sub;
    float    *cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
    
    /* Memory allocation */
    if ((qqlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    if ((tlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    if ((error = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    if ((error2 = (float *)calloc(lpc_order*NEC_QLSP_CAND, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    if ((weight = (float *)calloc(lpc_order+2, sizeof(float))) == NULL) {
        printf("\n Memory allocation error in nec_lsp_quantizer \n");
        exit(1);
    }
    
    if ( lpc_order == 20 && lpc_order_8 == 10 ) {
        cb[0] = nec_lspnw_p;
        cb[1] = nec_lspnw_1a;
        cb[2] = nec_lspnw_1b;
        cb[3] = nec_lspnw_2a;
        cb[4] = nec_lspnw_2b;
        nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
    } else {
        printf("Error in nec_cng_bws_lsp_quantizer\n");
        exit(1);
    }
    
    /*--- calc. weight ----*/
    weight[0] = 0.0;
    weight[lpc_order+1] = (float)NEC_PAI;
    for ( i = 0; i < lpc_order; i++ )
        weight[i+1] = lsp[i];
    for ( i = 0; i <= lpc_order; i++ )
        weight[i] = (float)(1.0/(weight[i+1]-weight[i]));
    for ( i = 0; i < lpc_order; i++ )
        weight[i] = (weight[i]+weight[i+1]);
    
    /*--- vector linear prediction ----*/
    for ( i = 0; i < lpc_order; i++)
        blsp[NEC_LSPPRDCT_ORDER-1][i] = 0.0;
    for ( i = 0; i < lpc_order_8; i++)
        blsp[NEC_LSPPRDCT_ORDER-1][i] = qlsp8[i];
    
    for ( i = 0; i < lpc_order; i++ ) {
        vec_hat[i] = 0.0;
        for ( k = 1; k < NEC_LSPPRDCT_ORDER; k++ ) {
            vec_hat[i] += (cb[0][k*lpc_order+i] * blsp[k][i]);
        }
    }
    for ( i = 0; i < lpc_order; i++) error[i] = lsp[i] - vec_hat[i];
    
    /*--- 1st VQ -----*/
    sp_order = lpc_order/NEC_NUM_LSPSPLIT1;
    for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
        cb_size = 1<<frame_bit_allocation[i];
        nec_psvq(error+i*sp_order,&cb[0][i*sp_order],cb[i+1],cb_size,sp_order,
            weight+i*sp_order,cand1[i],NEC_QLSP_CAND);
    }
    for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
        for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
            for ( j = 0; j < sp_order; j++)
                error2[k*lpc_order+i*sp_order+j] = 
                error[i*sp_order+j] - cb[0][i*sp_order+j]
                * cb[i+1][sp_order*cand1[i][k]+j];
        }
    }
    
    /*--- 2nd VQ -----*/
    sp_order = lpc_order/NEC_NUM_LSPSPLIT2;
    for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
        for ( i = 0; i < NEC_NUM_CNG_LSPVQ2; i++ ) {
            cb_size = 1<<frame_bit_allocation[i+NEC_NUM_LSPSPLIT1];
            nec_psvq(error2+k*lpc_order+i*sp_order,&cb[0][i*sp_order],
                cb[i+1+NEC_NUM_LSPSPLIT1], cb_size,sp_order,
                weight+i*sp_order,&cand2[i][k],1);
        }
    }
    
    mindist = (float)1.0e30;
    for ( k = 0; k < NEC_QLSP_CAND*NEC_QLSP_CAND; k++ ) {
        switch ( k ) {
        case 0:
            sp_order = 10;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
            sp_order = 5;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
            break;
        case 1:
            sp_order = 10;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
            sp_order = 5;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
            break;
        case 2:
            sp_order = 10;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
            sp_order = 5;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
            break;
        case 3:
            sp_order = 10;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
            sp_order = 5;
            for ( j = 0; j < sp_order; j++)
                tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
            for ( j = 0; j < sp_order; j++)
                tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
            break;
        }
        
        for ( i = 0; i < lpc_order; i++ ) qqlsp[i] = vec_hat[i]+cb[0][i]*tlsp[i];
        nec_lsp_sort( qqlsp, lpc_order );
        dist = 0.0;
        for ( i = 0; i < lpc_order; i++ ) {
            sub = lsp[i] - qqlsp[i];
            dist += weight[i] * sub * sub;
        }
        if ( dist < mindist || k == 0 ) {
            mindist = dist;
            cidx = k;
            for ( i = 0; i < lpc_order; i++ ) qlsp[i] = qqlsp[i];
            for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];
        }
    }
    
    /*--- store previous vector ----*/
    for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) {
        for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
    }
    
    /*---- set INDEX -----*/
    indices[0] = cand1[0][cidx/2];
    indices[1] = cand1[1][cidx%2];
    indices[2] = cand2[0][cidx/2];
    indices[3] = cand2[1][cidx/2];
    indices[4] = 0;
    indices[5] = 0;
    
    free( qqlsp );
    free( tlsp );
    free( error );
    free( error2 );
    free( vec_hat );
    free( weight );
    
}

/******************************************************************************/

void nec_cng_bws_lsp_decoder(unsigned long indices[],        /* input  */
                             float         qlsp8[],          /* input  */
                             float         qlsp[],           /* output  */
#ifdef VERSION2_EC
                             float qlsp_pre[],		     /* input  */
#endif
                             float blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
                             long          lpc_order,        /* configuration input */
                             long          lpc_order_8 )     /* configuration input */
{
    long     i, j, k, sp_order;
    float    *tlsp, *vec_hat;
    float    *cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
    
    /* Memory allocation */
    if ((tlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) 
    {
        printf("\n Memory allocation error in nec_lsp_decoder \n");
        exit(1);
    }
    
    if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) 
    {
        printf("\n Memory allocation error in nec_lsp_decoder \n");
        exit(1);
    }
    
    if ( lpc_order == 20 && lpc_order_8 == 10 ) 
    {
        cb[0] = nec_lspnw_p;
        cb[1] = nec_lspnw_1a;
        cb[2] = nec_lspnw_1b;
        cb[3] = nec_lspnw_2a;
        cb[4] = nec_lspnw_2b;
        nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
    }
    else 
    {
        printf("Error in nec_bws_lsp_decoder\n");
        exit(1);
    }
    
    /*--- vector linear prediction ----*/
    for ( i = 0; i < lpc_order; i++)   blsp[NEC_LSPPRDCT_ORDER-1][i] = 0.0;
    for ( i = 0; i < lpc_order_8; i++) blsp[NEC_LSPPRDCT_ORDER-1][i] = qlsp8[i];
    
    for ( i = 0; i < lpc_order; i++ ) 
    {
        vec_hat[i] = 0.0;
        for ( k = 1; k < NEC_LSPPRDCT_ORDER; k++ ) 
        {
            vec_hat[i] += (cb[0][k*lpc_order+i] * blsp[k][i]);
        }
    }
    
    sp_order = lpc_order/NEC_NUM_LSPSPLIT1;
    for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) 
    {
        for ( j = 0; j < sp_order; j++)
            tlsp[i*sp_order+j] = cb[i+1][sp_order*indices[i]+j];
    }
    
    sp_order = lpc_order/NEC_NUM_LSPSPLIT2;
    for ( i = 0; i < NEC_NUM_CNG_LSPVQ2; i++ ) 
    {
        for ( j = 0; j < sp_order; j++)
        {
            tlsp[i*sp_order+j] += cb[i+1+NEC_NUM_LSPSPLIT1][sp_order*indices[i+NEC_NUM_LSPSPLIT1]+j];
        }
    }
    
    for ( i = 0; i < lpc_order; i++ ) qlsp[i] = vec_hat[i]+cb[0][i]*tlsp[i];
    
    nec_lsp_sort( qlsp, lpc_order );
    
    for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];
#ifdef VERSION2_EC	/* SC: 1, EC: LSP, BWS */
    if( getErrAction() & EA_BLSP0_RECALC ){
	    for( i = 0; i < lpc_order; i++ )
		    blsp[0][i] = (qlsp_pre[i] - vec_hat[i]) / cb[0][i];
    }
#endif
    
    /*--- store previous vector ----*/
    for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) 
    {
        for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
    }
    
    free( tlsp );
    free( vec_hat );
    
}

void nec_cng_bws_lspvec_renew( float	qlsp8[],	/* input */
                               float 	qlsp[],		/* input */
                               float 	blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
                               long	lpc_order,	/* configuration input */
                               long	lpc_order_8 )	/* configuration input */
{
    long     i, k;
    float    *vec_hat;
    float    *cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
    
    /* Memory allocation */
    if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) 
    {
        printf("\n Memory allocation error in nec_lsp_decoder \n");
        exit(1);
    }
    
    if ( lpc_order == 20 && lpc_order_8 == 10 ) 
    {
        cb[0] = nec_lspnw_p;
        cb[1] = nec_lspnw_1a;
        cb[2] = nec_lspnw_1b;
        cb[3] = nec_lspnw_2a;
        cb[4] = nec_lspnw_2b;
        nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
    }
    else 
    {
        printf("Error in nec_bws_lsp_decoder\n");
        exit(1);
    }
    
    /*--- vector linear prediction ----*/
    for ( i = 0; i < lpc_order; i++)   blsp[NEC_LSPPRDCT_ORDER-1][i] = 0.0;
    for ( i = 0; i < lpc_order_8; i++) blsp[NEC_LSPPRDCT_ORDER-1][i] = qlsp8[i];
    
    for ( i = 0; i < lpc_order; i++ ) 
    {
        vec_hat[i] = 0.0;
        for ( k = 1; k < NEC_LSPPRDCT_ORDER; k++ ) 
        {
            vec_hat[i] += (cb[0][k*lpc_order+i] * blsp[k][i]);
        }
    }
    
    for( i = 0; i < lpc_order; i++ )
	    blsp[0][i] = (qlsp[i] - vec_hat[i]) / cb[0][i];

    /*--- store previous vector ----*/
    for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) 
    {
        for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
    }
    
    free( vec_hat );
    
}

/******************************************************************************/

void nec_psvq (float   vector[], 
               float   p[], 
               float   cb[],
               long    size, 
               long    order,
               float   weight[], 
               long    code[], 
               long    num)
{
    long     i, j, k;
    float    mindist, sub, *dist;
    
    if ((dist = (float *)calloc(size, sizeof(float))) == NULL)
    {
        printf("\n Memory allocation error in nec_svq \n");
        exit(1);
    }
    
    for ( i = 0; i < size; i++ ) 
    {
        dist[i] = 0.0;
        for ( j = 0; j < order; j++ ) 
        {
            sub = vector[j] - p[j] * cb[i*order+j];
            dist[i] += weight[j] * sub * sub;
        }
    }
    
    for ( k = 0; k < num; k++ ) 
    {
        code[k] = 0;
        mindist = (float)1.0e30;
        for ( i = 0; i < size; i++ ) 
        {
            if ( dist[i] < mindist ) 
            {
                mindist = dist[i];
                code[k] = i;
            }
        }
        dist[code[k]] = (float)1.0e30;
    }
    free( dist );
    
}

/******************************************************************************/

void nec_lsp_sort( float x[], long order )
{
    long     i, j;
    float    tmp;
    
    for ( i = 0; i < order; i++ ) 
    {
        if ( x[i] < 0.0 || x[i] > (float)NEC_PAI ) 
        {
            x[i] = ((float)409/8192) + (float)NEC_PAI * (float)i / (float)order;
        }
    }
    
    for ( i = (order-1); i > 0; i-- ) 
    {
        for ( j = 0; j < i; j++ ) 
        {
            if ( x[j] + nec_lsp_minwidth > x[j+1] ) 
            {
                tmp = ((float)4096/8192) * (x[j] + x[j+1]);
                x[j] = tmp - ((float)4177/8192) * nec_lsp_minwidth;
                x[j+1] = tmp + ((float)4177/8192) * nec_lsp_minwidth;
            }
        }
    }
}

/******************************************************************************/

void nec_lsp_ave (float *lpc_coeff,          /* in:  */
                  float *lsp_coefficients,   /* out: */
                  long  lpc_order,           /* in:  */
                  long  n_lpc_analysis,      /* in:  */
                  long  ExcitationMode,      /* in:  */ 
		  int   vad_flag	     /* in:  */
)
{
    static float Buf_lsp_coefficients[LSP_AVE_LEN_WB+1][SID_LPC_ORDER_WB];
    static short lsp_ave_len;
    static short lsp_init_flag = 0;
    float        *org_lsp_coeff;
    float        *tmp_lpc_coeff=NULL;
    int          i, j;
    
    if(lsp_init_flag == 0)
    {
        if(lpc_order == 10 )
        {
            lsp_ave_len = (short)((float)LSP_AVE_LEN/(float)n_lpc_analysis);
            
            for(i=0; i<LSP_AVE_LEN; i++)
            {
                for(j=0; j<lpc_order; j++)
                {
                    Buf_lsp_coefficients[i][j] = (float)((float)(j+1)/(float)(lpc_order+1));
                }
            }
        }
        else if(lpc_order == 20)
        {
            lsp_ave_len = (short)((float)LSP_AVE_LEN_WB/(float)n_lpc_analysis);
            
            for(i=0; i<LSP_AVE_LEN; i++)
            {
                for(j=0; j<lpc_order; j++)
                {
                    Buf_lsp_coefficients[i][j] = (float)((float)(j+1)/(float)(lpc_order+1));
                }
            }
        }
        lsp_init_flag = 1;
    }
    
    if((org_lsp_coeff=(float *)calloc(lpc_order, sizeof(float)))==NULL)
    {
        printf("\n Memory allocation error in nec_lsp_ave\n");
        exit(0);
    }
    
    
    if (ExcitationMode == RegularPulseExc)
    {
        /*   RPE   */   

        for(i = 0; i < lpc_order; i++)
        {
            lsp_coefficients[i] = lpc_coeff[i];
        }
    }
    else
    {
        /*   MPE   */

        if((tmp_lpc_coeff=(float *)calloc(lpc_order+1, sizeof(float)))==NULL)
        {
            printf("\n Memory allocation error in nec_lsp_ave\n");
            exit(0);
        }

        /* LPC format conversion */
        tmp_lpc_coeff[0] = 1.0;
        
        for(i=0;i<lpc_order;i++)
        {
            tmp_lpc_coeff[i+1] = -lpc_coeff[i];
        }
        
        /* LPC -> LSP */
        pc2lsf(lsp_coefficients, tmp_lpc_coeff, lpc_order);
    }

    
    for(i=0;i<lpc_order;i++) lsp_coefficients[i] /= (float)PAN_PI;
    
    /* Calculate Average LSP */    
    for(i=0;i<lpc_order;i++) org_lsp_coeff[i] = lsp_coefficients[i];
    
    if(vad_flag !=1 && lsp_ave_len != 0)
    {
        for(i=0;i<lpc_order;i++)
        {
            lsp_coefficients[i] = lsp_coefficients[i]/lsp_ave_len;
        }
        
        for(i=0;i<lsp_ave_len-1;i++)
        {
            for(j=0;j<lpc_order;j++)
            {
                lsp_coefficients[j] += Buf_lsp_coefficients[i][j]/lsp_ave_len;
            }
        }
    }

    /* Buffer Update */
    if(vad_flag != 1)
    {
	for(i=lsp_ave_len-3;i>=0;i--)
	{
	    for(j=0;j<lpc_order;j++)
	    {
		Buf_lsp_coefficients[i+1][j] = Buf_lsp_coefficients[i][j];
	    }
	}
	for(j=0;j<lpc_order;j++)
	{
	    Buf_lsp_coefficients[0][j] = org_lsp_coeff[j];
	}
    }
    free(org_lsp_coeff);
    if (ExcitationMode == MultiPulseExc) free(tmp_lpc_coeff);
}

/******************************************************************************/

void nec_lsp_ave_bws(float *lpc_coeff,         /* in: */
                     float *lsp_coefficients,  /* out: */
                     long  lpc_order,          /* in: */
                     long  n_lpc_analysis,     /* in: */
		     int   vad_flag	       /* in: */
)
{
    static float Buf_lsp_coefficients[LSP_AVE_LEN_WB+1][SID_LPC_ORDER_WB];
    static short lsp_ave_len;
    static short lsp_init_flag = 0;
    float *org_lsp_coeff, *tmp_lpc_coeff;
    int i, j;
    
    if(lsp_init_flag == 0)
    {
        lsp_ave_len = (short)((float)LSP_AVE_LEN_WB/(float)n_lpc_analysis);
        for(i=0; i<LSP_AVE_LEN; i++)
        {
            for(j=0; j<lpc_order; j++)
            {
                Buf_lsp_coefficients[i][j] = (float)((float)(j+1)/(float)(lpc_order+1));
            }
        }
        lsp_init_flag = 1;
    }
    
    if((org_lsp_coeff=(float *)calloc(lpc_order, sizeof(float)))==NULL)
    {
        printf("\n Memory allocation error in abs_lpc_quantizer\n");
        exit(0);
    }
    
    if((tmp_lpc_coeff=(float *)calloc(lpc_order+1, sizeof(float)))==NULL)
    {
        printf("\n Memory allocation error in abs_lpc_quantizer\n");
        exit(0);
    }
    

    /* LPC format conversion */
    tmp_lpc_coeff[0] = 1.0;
    
    for(i=0;i<lpc_order;i++)
    {
        tmp_lpc_coeff[i+1] = -lpc_coeff[i];
    }
    
    /* LPC -> LSP */
    pc2lsf(lsp_coefficients, tmp_lpc_coeff, lpc_order);
    
    for(i=0;i<lpc_order;i++) lsp_coefficients[i] /= (float)PAN_PI;
    
    /* Calculate Average LSP */    
    for(i=0;i<lpc_order;i++) org_lsp_coeff[i] = lsp_coefficients[i];
    
    if(vad_flag != 1 && lsp_ave_len != 0)
    {
        for(i=0;i<lpc_order;i++)
        {
            lsp_coefficients[i] = lsp_coefficients[i]/lsp_ave_len;
        }
        
        for(i=0;i<lsp_ave_len-1;i++)
        {
            for(j=0;j<lpc_order;j++)
            {
                lsp_coefficients[j] += Buf_lsp_coefficients[i][j]/lsp_ave_len;
            }
        }
    }
    
    /* Buffer Update */
    if(vad_flag != 1)
    {
	for(i=lsp_ave_len-3;i>=0;i--)
	{
	    for(j=0;j<lpc_order;j++)
	    {
		Buf_lsp_coefficients[i+1][j] = Buf_lsp_coefficients[i][j];
	    }
	}
	
	for(j=0;j<lpc_order;j++)
	{
	    Buf_lsp_coefficients[0][j] = org_lsp_coeff[j];
	}
    }
    free(tmp_lpc_coeff);
    free(org_lsp_coeff);
}
/******************************************************************************/
