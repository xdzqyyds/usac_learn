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
*  Confort Noise Generator(CNG) Subroutines
*
*  Ver1.0  99.5.10 H.Ito(NEC)
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include    "nec_abs_const.h"
#include    "nec_abs_proto.h"
#include    "nec_exc_proto.h"
#include    "nec_vad.h"

extern unsigned long TX_flag_enc;

/******************************************************************************/

void nec_mk_cng_excitation (float LpcCoef[],             /* in: */
                            long  *rms_index,            /* out: */
                            float decoded_excitation[],  /* out: */
                            long  lpc_order,             /* in: */
                            long  frame_size,            /* in: */
                            long  sbfrm_size,            /* in: */
                            long  n_subframes,           /* in: */
                            long  SampleRateMode,        /* in: */
                            long  ExcitationMode,        /* in: */
                            long  Configuration,         /* in: */
                            float *mem_past_syn,         /* in/out: */
                            long  *flag_rms,             /* in/out: */
                            float *pqxnorm,              /* in/out: */
                            float *cng_xnorm,            /* in: */
                            short prev_tx_flag           /* in: */
)
{
    static float qxnorm[NEC_MAX_NSF];
    static long  c_subframe = 0;
    
    float    *CoreExcitation;
    float    *excg, *mpexc, *xr, *fk;
    float    *pmw;
    long     i;
    float    g_ec, g_gc;
    long     rmsbit;
    static short sm_off_flag = 0;
    static short seed = 21845;
    static float SID_rms =0.0;
    static float prev_G = 0.0;
    
    /* Memory Allocation */
    if((xr = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((fk = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((pmw = (float *)calloc (lpc_order, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((excg = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((mpexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    
    c_subframe = c_subframe % n_subframes;
    
    rmsbit = NEC_VAD_RMS_BIT;
    
    /* RMS Encode */
    if(c_subframe==0) 
    {
        if(TX_flag_enc == 2 || TX_flag_enc == 3)
        {
            nec_enc_rms(cng_xnorm, qxnorm, n_subframes,
                (float)NEC_RMS_VAD_MAX, (float)NEC_VAD_MU_LAW,
                rmsbit, rms_index
                        , flag_rms, pqxnorm
                        );
            
            /* Smoothing ON/OFF decision */
            rms_smooth_decision(qxnorm[n_subframes-1], SID_rms, SampleRateMode,
                frame_size, prev_tx_flag, &sm_off_flag);
            
            SID_rms = qxnorm[n_subframes-1];
        }
        else if(TX_flag_enc == 0)
        {
            for ( i = 0; i < n_subframes; i++ ) qxnorm[i] = SID_rms;
            sm_off_flag = 0;
        }
    }
    qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);
    
    /* Seed Initialize */
    if(prev_tx_flag == 1) seed = 21845;
    
    /* Make Excitation for CNG */
    mk_cng (mpexc, &g_ec, excg, &g_gc, &LpcCoef[lpc_order*c_subframe], 
            (short)lpc_order, (float)(qxnorm[c_subframe]/sqrt(sbfrm_size)), (short)sbfrm_size, 
            &seed, sm_off_flag, &prev_G, 
            ExcitationMode, Configuration);
    
    if((CoreExcitation = (float *)calloc (sbfrm_size, sizeof(float)))==NULL)
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    for(i = 0; i < sbfrm_size; i++)
    {
        CoreExcitation[i] = g_ec * mpexc[i] + g_gc * excg[i];
    }
    
    for(i = 0; i < sbfrm_size; i++)
    {
        decoded_excitation[i] = 0.0;
    }
    
    /* LP Synthesis */

    nec_syn_filt (CoreExcitation, &LpcCoef[lpc_order*c_subframe],
                  mem_past_syn, xr, lpc_order, sbfrm_size);

    
    c_subframe++;
    
    free( xr );
    free( fk );
    free( mpexc );
    free( excg );
    free( pmw );
    
    free( CoreExcitation );

}
                            
/******************************************************************************/
                            
void nec_mk_cng_excitation_bws(
    float LpcCoef[],             /* in: */
    float decoded_excitation[], /* out: */
    long  lpc_order,        /* in: */
    long  sbfrm_size,       /* in: */
    long  n_subframes,      /* in: */
    long  rms_index_8,      /* in: RMS code index */
    float *mem_past_syn,        /* in/out: */
    long  *flag_rms,        /* in/out: */
    float *pqxnorm,         /* in/out: */
    short prev_tx_flag
)
{
    static float qxnorm[NEC_MAX_NSF];
    static long  c_subframe = 0;
    
    float    *xr, *fk, *pmw;
    float        *excg, *mpexc;
    long     i;
    float    g_ec, g_gc;
    static short sm_off_flag = 0;
    static short seed = 21845;
    static float SID_rms = 0.0;
    static float prev_G = 0.0;
    
    /*------ Memory Allocation ----------*/
    if((mpexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((excg = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((xr = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((fk = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    if((pmw = (float *)calloc (lpc_order, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng \n");
        exit(1);
    }
    
    c_subframe = c_subframe % n_subframes;
    
    /* RMS decode */
    if(c_subframe==0)
    {
        if(TX_flag_enc == 2 || TX_flag_enc == 3)
        {
            /* RMS */
            nec_bws_rms_dec(qxnorm, n_subframes,
                (float)NEC_RMS_VAD_MAX, (float)NEC_VAD_MU_LAW,
                (long)NEC_BIT_RMS, rms_index_8
                            , flag_rms, pqxnorm
                            );
            
            /* Smoothing ON/OFF decision */
            rms_smooth_decision(qxnorm[n_subframes-1], SID_rms, 1,
                sbfrm_size*n_subframes, prev_tx_flag,
                &sm_off_flag);
            
            SID_rms = qxnorm[n_subframes-1];
        }
        else if(TX_flag_enc ==0)
        {
            for ( i = 0; i < n_subframes; i++ ) qxnorm[i] = SID_rms;
            sm_off_flag = 0;
        }
    }
    qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);
    
    /* Seed Initialize */
    if(prev_tx_flag == 1) seed = 21845;
    
    /* Make Excitation for CNG */
    mk_cng(mpexc, &g_ec, excg, &g_gc, 
        &LpcCoef[lpc_order*c_subframe], (short)lpc_order,
        (float)(qxnorm[c_subframe]/sqrt(sbfrm_size)), (short)sbfrm_size, &seed,
        sm_off_flag, &prev_G, MultiPulseExc, 0);
    
    for(i = 0; i < sbfrm_size; i++)
    {
        decoded_excitation[i] = g_ec * mpexc[i] + g_gc * excg[i];
    }
    
    /* LP Synthesis */
    for ( i = 0; i < lpc_order; i++) 
    {
        pmw[i] = mem_past_syn[i];
    }

    nec_syn_filt(decoded_excitation, &LpcCoef[lpc_order*c_subframe],
        mem_past_syn, xr, lpc_order, sbfrm_size);
    
    c_subframe++;
    
    free( xr );
    free( fk );
    free( mpexc );
    free( excg );
    free( pmw );
}
                            
/******************************************************************************/

void mk_cng_excitation(float int_Qlpc_coefficients[], /* in: interpolated LPC */
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
                       float *xnorm,                  /* in: */
                       short prev_tx_flag             /* in: */
)
{
    /* local */
    float *RevIntQLPCoef;
    long i;
    static long c_subframe = 0;
    
    long num_lpc_indices;
    
    if(fs8kHz==SampleRateMode)
    {
        num_lpc_indices = NUM_LSP_INDICES_NB;
    }
    else 
    {
        num_lpc_indices = NUM_LSP_INDICES_WB;
    }
    
    c_subframe = c_subframe % n_subframes;
    /* memory allocation */
    if((RevIntQLPCoef=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL) 
    {
        printf("\n memory allocation error in excitation analysis\n");
        exit(1);
    }
    
    /* reverse the sign of LP parameters */
    for(i=0;i<lpc_order*n_subframes;i++) *(RevIntQLPCoef+i) = -(*(int_Qlpc_coefficients+i));
    
    nec_mk_cng_excitation (RevIntQLPCoef,             /* in: interpolated LPC */
                           rms_index,                 /* out: RMS code index */
                           decoded_excitation,        /* out: decoded excitation */
                           lpc_order,                 /* in: order of LPC */
                           frame_size,                /* in: frame size */
                           sbfrm_size,                /* in: subframe size */
                           n_subframes,               /* in: number of subframes */
                           SampleRateMode,            /* in: */
                           ExcitationMode,            /* in: */
                           Configuration,             /* in: */
                           mem_past_syn,              /* in/out: */
                           flag_rms,                  /* in/out: */
                           pqxnorm,                   /* in/out: */
                           xnorm,                     /* in: */
                           prev_tx_flag);             /* in: */
        
    c_subframe++;
    free(RevIntQLPCoef);
    
}

/******************************************************************************/

void mk_cng_excitation_bws(float int_Qlpc_coefficients[], /* in: interpolated LPC */
                           long lpc_order,                /* in: order of LPC */
                           long sbfrm_size,               /* in: subframe size */
                           long n_subframes,              /* in: number of subframes */
                           float decoded_excitation[],    /* out: decoded excitation */
                           long  rms_index,               /* in: RMS code index */
                           float *mem_past_syn,           /* in/out: */
                           long  *flag_rms,               /* in/out: */
                           float *pqxnorm,                /* in/out: */
                           short prev_tx_flag             /* in */
)
{
    /* local */
    float *RevIntQLPCoef;
    long i;
    static long c_subframe = 0;
    
    c_subframe = c_subframe % n_subframes;
    /* memory allocation */
    if((RevIntQLPCoef=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL)
    {
        printf("\n memory allocation error in excitation analysis\n");
        exit(1);
    }
    
    /* reverse the sign of LP parameters */
    for(i=0;i<lpc_order*n_subframes;i++) *(RevIntQLPCoef+i) = -(*(int_Qlpc_coefficients+i));
    
    nec_mk_cng_excitation_bws (RevIntQLPCoef,             /* in: interpolated LPC */
                               decoded_excitation,        /* out: decoded excitation */
                               lpc_order,                 /* in: order of LPC */
                               sbfrm_size,                /* in: subframe size */
                               n_subframes,               /* in: number of subframes */
                               rms_index,                 /* in: RMS code index */
                               mem_past_syn,              /* in/out: */
                               flag_rms,                  /* in/out: */
                               pqxnorm,                   /* in/out: */
                               prev_tx_flag);             /* in */
                                
    c_subframe++;
    free(RevIntQLPCoef);
    
}

/******************************************************************************/

void nec_rms_ave (float *InputSignal,     /* in:  */
                  float *cng_xnorm,       /* out: */
                  long  frame_size,       /* in:  */
                  long  sbfrm_size,       /* in:  */
                  long  n_subframes,      /* in:  */
                  long  SampleRateMode,   /* in:  */
                  int   vad_flag)         /* in:  */
{
    int             i, j;
    float           xnorm[CELP_MAX_SUBFRAMES];
    short           n10msec = 0;
    short           rms_ave_len;
    static float    buf_xnorm[RMS_ENC_AVE_LEN*CELP_MAX_SUBFRAMES];
    
    /* Calculate xnorm for each subframe */
    
    for ( i = 0; i < n_subframes; i++ )
    {              
        xnorm[i] = 0.0;
        
        for ( j = 0; j < sbfrm_size; j++ ) 
        {
            xnorm[i] += (InputSignal[i*sbfrm_size+j] * InputSignal[i*sbfrm_size+j]);
        }
        
        xnorm[i] = (float)sqrt(xnorm[i] / (float)sbfrm_size);
    }
    
    /* Set Averaging Length */
    if(SampleRateMode == 0)
    {
        n10msec = (short)((float)frame_size/80.0);
    }
    else if(SampleRateMode == 1)
    {
        n10msec = (short)((float)frame_size/160.0);
    }
    rms_ave_len = (short)((float)RMS_ENC_AVE_LEN/(float)n10msec);
    
    /* Averaging */
    if(vad_flag != 1 && rms_ave_len != 0)
    {
        for(i=n_subframes-1; i>=0; i--)
        {
            cng_xnorm[i] = 0.0;
            for(j=0; j<=i; j++)
            {
                cng_xnorm[i] += xnorm[j];
            }
            for(j=i+1; j<rms_ave_len*n_subframes; j++)
            {
                cng_xnorm[i] += buf_xnorm[j];
            }
            cng_xnorm[i] /= rms_ave_len*n_subframes;
        }
    }
    else
    {
        for(i=0; i<n_subframes; i++) cng_xnorm[i] = xnorm[i];
    }
    
    /* Buffer Update */
    if (vad_flag != 1 && rms_ave_len != 0)
    {
        for(i=n_subframes; i<rms_ave_len*n_subframes; i++)
        {
            buf_xnorm[i-n_subframes] = buf_xnorm[i];
        }
        for(i=0; i<n_subframes; i++)
        {
            buf_xnorm[(rms_ave_len-1)*n_subframes+i] = xnorm[i];
        }
    }
}

/******************************************************************************/
