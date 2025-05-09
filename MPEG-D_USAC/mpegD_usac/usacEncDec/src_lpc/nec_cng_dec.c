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

#include "nec_abs_const.h"
#include "nec_abs_proto.h"
#include "nec_exc_proto.h"
#include "nec_vad.h"
#ifdef VERSION2_EC
#include "err_concealment.h"
#endif

extern unsigned long TX_flag_dec;

/******************************************************************************/

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
)
{
    static float qxnorm[NEC_MAX_NSF];
    static long  c_subframe = 0;
    
    long     i;
    float    *fcb_cng, *excg;
    float    g_ec, g_gc;
    static short sm_off_flag = 0;
    static short seed = 21845;
    static float SID_rms = 0.0;
    static float prev_G = 0.0;
    
    /*------ Memory Allocation ----------*/
    if((fcb_cng = (float *)calloc (sbfrm_size, sizeof(float)))==NULL)
    {
        printf("\n Memory allocation error in nec_cng_dec \n");
        exit(1);
    }
    
    if((excg = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng_dec \n");
        exit(1);
    }
    
    c_subframe = c_subframe % n_subframes;
    
    /* RMS Decode */
    if(c_subframe==0) 
    {
        if(TX_flag_dec == 2 || TX_flag_dec == 3)
        {
            nec_dec_rms (qxnorm, 
                         n_subframes,
                         (float)NEC_RMS_VAD_MAX, 
                         (float)NEC_VAD_MU_LAW,
                         NEC_VAD_RMS_BIT, 
                         (long)rms_index
                         ,flag_rms 
                         ,pqxnorm
                         );
#ifdef VERSION2_EC	/* SC: 023, EC: RMS, MPE/RPE */
	    if( getErrAction() & EA_RMS_PREVIOUS ){
		    float last_rms;

		    errConLoadRms( &last_rms );
		    for( i = 0; i < n_subframes; i++ )
			    qxnorm[i] = last_rms;
	    }
#endif
            
            /* Smoothing ON/OFF decision */
            rms_smooth_decision (qxnorm[n_subframes-1], 
                                 SID_rms, 
                                 SampleRateMode,
                                 sbfrm_size*n_subframes, 
                                 prev_tx_flag,
                                 &sm_off_flag);
            
            SID_rms = qxnorm[n_subframes-1];
        }
        else if (TX_flag_dec == 0)
        {
            for ( i = 0; i < n_subframes; i++ ) 
            {
                qxnorm[i] = SID_rms;
            }

            sm_off_flag = 0;
        }
#ifdef VERSION2_EC	/* SC: 023, EC: RMS, MPE/RPE */
	errConSaveRms( qxnorm[n_subframes-1] );
#endif
    }
    
    qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);
    
    /* Seed Initialize */
    if( c_subframe == 0 && prev_tx_flag == 1 ) seed = 21845;
    
    /* Make Excitation for CNG */
    mk_cng (fcb_cng, 
            &g_ec, 
            excg, 
            &g_gc, 
            LpcCoef, 
            lpc_order,
            qxnorm[c_subframe]/sqrt(sbfrm_size), 
            sbfrm_size, 
            &seed,
            sm_off_flag, 
            &prev_G, 
            ExcitationMode,
            Configuration);
    
    for(i = 0; i < sbfrm_size; i++)
    {
        decoded_excitation[i] = g_ec * fcb_cng[i] + g_gc*excg[i];
    }

    
    c_subframe++;
    
    free( excg );
    free( fcb_cng );
}

/******************************************************************************/

void nec_mk_cng_excitation_bws_dec(float LpcCoef[],            /* in: */
                                   unsigned long  rms_index,   /* in: */
                                   float decoded_excitation[], /* out: */
                                   long  lpc_order,            /* in: */
                                   long  sbfrm_size,           /* in: */
                                   long  n_subframes,          /* in: */
                                   long *flag_rms,             /* in/out: */
                                   float *pqxnorm,             /* in/out: */
                                   short prev_tx_flag          /* in: */
)
{
    static float qxnorm[NEC_MAX_NSF];
    static long  c_subframe = 0;
    static short seed = 21845;
    static float SID_rms = 0.0;
    static float prev_G = 0.0;
    static short sm_off_flag = 0;
    
    long     i;
    float    *mpexc, *excg;
    float    g_ec, g_gc;
    
    /*------ Memory Allocation ----------*/
    if((mpexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
        printf("\n Memory allocation error in nec_cng_dec \n");
        exit(1);
    }
    if((excg = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
        printf("\n Memory allocation error in nec_cng_dec \n");
        exit(1);
    }
    
    c_subframe = c_subframe % n_subframes;
    
    /* RMS Decode */
    if(c_subframe==0) {
        if(TX_flag_dec == 2 || TX_flag_dec == 3){
            nec_bws_rms_dec(qxnorm, n_subframes,
                            (float)NEC_RMS_VAD_MAX, (float)NEC_VAD_MU_LAW,
                            (long)(NEC_BIT_RMS), (long)rms_index
                            
                            ,flag_rms, pqxnorm
                            );
#ifdef VERSION2_EC	/* SC: 023, EC: RMS, BWS */
	    if( getErrAction() & EA_RMS_PREVIOUS ){
		    float last_rms;

		    errConLoadRmsBws( &last_rms );
		    for( i = 0; i < n_subframes; i++ )
			    qxnorm[i] = last_rms;
	    }
#endif
            
            /* Smoothing ON/OFF decision */
            rms_smooth_decision(qxnorm[n_subframes-1], SID_rms, 1,
                sbfrm_size*n_subframes, prev_tx_flag,
                &sm_off_flag);
            
            SID_rms = qxnorm[n_subframes-1];
        }else if(TX_flag_dec == 0){
            for ( i = 0; i < n_subframes; i++ ) qxnorm[i] = SID_rms;
            sm_off_flag = 0;
        }
#ifdef VERSION2_EC	/* SC: 023, EC: RMS, BWS */
	errConSaveRmsBws( qxnorm[n_subframes-1] );
#endif
    }
    qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);
    
    /* Seed Initialize */
    if( c_subframe == 0 && prev_tx_flag == 1 ) seed = 21845;
    
    /* Make Excitation for CNG */
    mk_cng(mpexc, &g_ec, excg, &g_gc, 
        LpcCoef, lpc_order, qxnorm[c_subframe]/sqrt(sbfrm_size),
        sbfrm_size, &seed, sm_off_flag, &prev_G, MultiPulseExc, 0);
    
    for(i = 0; i < sbfrm_size; i++){
        decoded_excitation[i] = g_ec * mpexc[i] + g_gc * excg[i];
    }
    
    c_subframe++;
    
    free( mpexc );
    free( excg );
    
}

/******************************************************************************/

void mk_cng_excitation_dec (unsigned long  rms_index,               /* in: RMS code index      */
                            float          int_Qlpc_coefficients[], /* in: interpolated LPC    */
                            long           lpc_order,               /* in: order of LPC        */
                            long           sbfrm_size,              /* in: subframe size       */
                            long           n_subframes,             /* in: number of subframes */
                            float          excitation[],            /* out: decoded excitation */
                            long           SampleRateMode,          /* in:                     */
                            long           ExcitationMode,          /* in:                     */
                            long           Configuration,           /* in:                     */
                            long           *flag_rms,               /* in/out:                 */
                            float          *pqxnorm,                /* in/out:                 */
                            short          prev_tx_flag             /* in/out:                 */
)
{
    
    float *tmp_lpc_coefficients;
    long i;
    
    long num_lpc_indices;
    
    if(fs8kHz==SampleRateMode)
    {
        num_lpc_indices = NUM_LSP_INDICES_NB;
    }
    else 
    {
        num_lpc_indices = NUM_LSP_INDICES_WB;
    }
    
    if((tmp_lpc_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in abs_exc_generation\n");
        exit(1);
    }
    
    for(i=0;i<lpc_order;i++) 
        tmp_lpc_coefficients[i] = -int_Qlpc_coefficients[i];
    
    nec_mk_cng_excitation_dec ( tmp_lpc_coefficients,  /* in: interpolated LPC */
                                rms_index,             /* in: RMS code index */
                                excitation,            /* out: decoded excitation */
                                lpc_order,             /* in: order of LPC */
                                sbfrm_size,            /* in: subframe size */
                                n_subframes,           /* in: number of subframes */
                                SampleRateMode,        /* in: */
                                ExcitationMode,        /* in: */
                                Configuration,         /* in:     */ 
                                flag_rms,              /* in/out: */
                                pqxnorm,               /* in/out: */
                                prev_tx_flag);         /* in/out: */
        
    
    free(tmp_lpc_coefficients);
    
}

/******************************************************************************/

void mk_cng_excitation_bws_dec(unsigned long rms_index,       /* in: RMS code index */
                               float int_Qlpc_coefficients[], /* in: interpolated LPC */
                               long lpc_order,                /* in: order of LPC */
                               long sbfrm_size,               /* in: subframe size */
                               long n_subframes,              /* in: number of subframes */
                               float excitation[],            /* out: decoded excitation */
                               long  *flag_rms,               /* in/out: */
                               float *pqxnorm,                /* in/out: */
                               short prev_tx_flag             /* in/out: */
)
{
    
    float *tmp_lpc_coefficients;
    long i;
    
    if((tmp_lpc_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
        printf("\n Memory allocation error in abs_exc_generation\n");
        exit(1);
    }
    
    for(i=0;i<lpc_order;i++) 
        tmp_lpc_coefficients[i] = -int_Qlpc_coefficients[i];
    
    nec_mk_cng_excitation_bws_dec(
        tmp_lpc_coefficients,  /* in: interpolated LPC */
        rms_index,             /* in: RMS code index */
        excitation,            /* out: decoded excitation */
        lpc_order,             /* in: order of LPC */
        sbfrm_size,            /* in: subframe size */
        n_subframes,           /* in: number of subframes */
        flag_rms,       /* in/out: */
        pqxnorm,        /* in/out: */
        prev_tx_flag        /* in: */
        );
    
    free(tmp_lpc_coefficients);
    
}

/******************************************************************************/
