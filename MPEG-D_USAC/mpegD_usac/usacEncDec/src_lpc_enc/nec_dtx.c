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
*   MPEG-4 Audio Verification Model (LPC-ABS Core)
*   
*   Discontinuous Transmission(DTX) Subroutines
*
*   Ver1.0  99.5.10 H.Ito(NEC)
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_vad.h"
#include "nec_abs_const.h"

/* #define TX_INFO */                       /* Display TX information */

#define HANGOVER_LEN    8                   /* 80msec */
#define LSP_THR_NB      ((float)16/8192)    /* LSP range 0 to 1 */
#define LSP_THR_WB      ((float)12/8192)    /* LSP range 0 to 1 */
#define DTX_MIN_INTVL   2                   /* 20msec */
#define MIN_INTVL_OFF   4                   /* 40msec */
#define MIN_RMS         0.0                 /* 20*log10(1.0) [dB] */


/******************************************************************************/

unsigned long nec_dtx (int   vad_flag,           /* in: */
                       long  lpc_order,          /* in: */
                       float cng_xnorm,          /* in: */
                       float *cng_lsp_coeff,     /* in: */
                       long  SampleRateMode,     /* in: */
                       long  frame_size          /* in: */
)
{
    int i;
    float xnorm_in, LSP_dist;
    float lsp_threshold = 0;
    short rms_change_flag, lsp_change_flag;
    short n10msec = 0;
    short dtx_intvl, dtx_intvl_off_len;
    unsigned long tx_flag;
    
    static short prev_tx = 1;
    static short sid_min_rms_flag = 0;
    static short intvl_count = 0;
    static short min_intvl_count = 0;
    static float SID_rms_ref;
    static float SID_lsp_coeff_ref[SID_LPC_ORDER_WB];
    static short init_flag = 0;
    
    static float rms_th;
    static float rms_th_tab[5] = {  (float)96/16,
                                    (float)64/16,
                                    (float)48/16,
                                    (float)40/16,
                                    (float)32/16};
    
    if (init_flag == 0)
    {
        SID_rms_ref = 1.0;
        for(i=0; i<lpc_order; i++)
        {
            SID_lsp_coeff_ref[i] = (float)((float)(i+1)/(float)(lpc_order+1));
        }
        rms_th = rms_th_tab[4];
        init_flag = 1;
    }
    
    if (SampleRateMode == 0)
    { /* 8kHz sampling */
        n10msec = (short)((float)frame_size / 80.0);
        lsp_threshold = LSP_THR_NB;
    }
    else if (SampleRateMode == 1)
    { /* 16kHz sampling */
        n10msec = (short)((float)frame_size / 160.0);
        lsp_threshold = LSP_THR_WB;
    }   
    
    dtx_intvl = (short)((float)DTX_MIN_INTVL/(float)n10msec);
    dtx_intvl_off_len = (short)((float)MIN_INTVL_OFF/(float)n10msec);
    
    if(vad_flag == 1)
    {
        tx_flag = 1;
        intvl_count = 0;
        sid_min_rms_flag = 0;
        prev_tx = (short)tx_flag;
#ifdef TX_INFO
        for(i=0; i<n10msec; i++) printf("%d\n", tx_flag);
#endif
        return tx_flag;
    }
    
    /* RMS */
    if(cng_xnorm != 0.0)
    {
        xnorm_in = (float)(20*log10(cng_xnorm));
    }
    else 
        xnorm_in = (float)(-20.0); /* 20log10(0.1) */
    
    if(xnorm_in < MIN_RMS && sid_min_rms_flag)
    {
        sid_min_rms_flag = 1;
    }
    else sid_min_rms_flag = 0;
    
    if (fabs(SID_rms_ref-xnorm_in) > rms_th )
    {
        rms_change_flag = 1;
    }
    else 
        rms_change_flag = 0;
    
    /* LSP-Distance */
    LSP_dist = 0.0;
    
    for(i=0; i< lpc_order ; i++)
    {
        LSP_dist += (SID_lsp_coeff_ref[i]-cng_lsp_coeff[i]) *  (SID_lsp_coeff_ref[i]-cng_lsp_coeff[i]);
    }
    
    if(LSP_dist > lsp_threshold)
    {
        lsp_change_flag = 1;
    }
    else
    {
        lsp_change_flag = 0;
    }
    
    /* Minimum Interval OFF */
    if (prev_tx == 1 && dtx_intvl_off_len != 0)
    {
        intvl_count = dtx_intvl;
        min_intvl_count = 1;
    }
    else if (min_intvl_count < dtx_intvl_off_len)
    {
        intvl_count = dtx_intvl;
        min_intvl_count++;
    }
    else if (min_intvl_count == dtx_intvl_off_len)
    {
        intvl_count = 0;
        min_intvl_count = dtx_intvl_off_len+1;
    }   
    
    /* DTX decision */
    if ((intvl_count > dtx_intvl-1) && (sid_min_rms_flag == 0) )
    {
        if( lsp_change_flag == 1 )
        {
            tx_flag = 2;
        }
        else if (rms_change_flag == 1 )
        {
            tx_flag = 3;
        }
        else 
            tx_flag = 0;
    }
    else 
        tx_flag = 0;
    
    if (prev_tx == 1 && vad_flag == 0) tx_flag = 2;
    
    if (tx_flag == 2 || tx_flag == 3)
    {
        SID_rms_ref = xnorm_in;
        /* RMS Threshould Decision */
        if ( SID_rms_ref <= 1.0) 
        {
            rms_th = rms_th_tab[0];
        }

        if ( SID_rms_ref > 1.0 && SID_rms_ref <= (float)80/16 )
        {
            rms_th = rms_th_tab[1];
        }

        if ( SID_rms_ref > (float)80/16 && SID_rms_ref <= (float)144/16 )
        {
            rms_th = rms_th_tab[2];
        }

        if ( SID_rms_ref > (float)144/16 && SID_rms_ref <= (float)192/16 )
        {
            rms_th = rms_th_tab[3];
        }

        if ( SID_rms_ref > (float)192/16 )
        {
            rms_th = rms_th_tab[4];
        }
        
        if (SID_rms_ref < MIN_RMS)
        {
            sid_min_rms_flag = 1;
        }
        else 
        {
            sid_min_rms_flag = 0;
        }
        
        if (tx_flag == 2)
        {
            for(i=0; i< lpc_order ; i++)
            {
                SID_lsp_coeff_ref[i] = cng_lsp_coeff[i];
            }
        }
        
        intvl_count = 0;
    }
    else if (tx_flag == 0) intvl_count++;
    prev_tx = (short)tx_flag;
    
#ifdef TX_INFO
    for(i=0; i<n10msec; i++) printf("%d\n", tx_flag);
#endif
    
    return tx_flag;
}

/******************************************************************************/

void nec_hangover (long  SampleRateMode,          /* in:     */
                   long  frame_size,              /* in:     */
                   int   *vad_flag)               /* in/out: */
{
    short           n10msec = 0;
    short           hangover_len;
    static short    hangover_cnt = 0;
    static short    spfr_cnt = 0;
    
    if (SampleRateMode == 0)
    { /* 8kHz sampling */
        n10msec = (short)((float)frame_size / 80.0);
    }
    else if(SampleRateMode == 1)
    { /* 16kHz sampling */
        n10msec = (short)((float)frame_size / 160.0);
    }   
    
    hangover_len = (short)((float)HANGOVER_LEN/(float)n10msec);
    
    if(*vad_flag != 1)
    {
        if(spfr_cnt > hangover_len -1)
        {
            if(hangover_cnt < hangover_len)
            {
                *vad_flag = 1;
            }
            else 
            {
                spfr_cnt = 0;
            }
            
            hangover_cnt++;
        }
        else
        {
            spfr_cnt = 0;
        }
    }
    else
    {
        hangover_cnt = 0;
        spfr_cnt++;
    }
    
    /*  return *vad_flag;  */
}

/******************************************************************************/
