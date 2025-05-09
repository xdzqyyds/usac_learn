/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands)
and edited by Torsten Mlasko (Robert Bosch GmbH, Germany),
Ralf Funken (Philips Consumer Electronics) and Hironori Ito (NEC Corporation)
in the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      SOURCE_FILE:    CELP_ENCODER.C                                  */
/*                                                                      */
/*======================================================================*/
    
/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "common_m4a.h"

#include "lpc_common.h"   /* Common LPC core Defined Constants          */
#include "bitstream.h"     /* bit stream module                         */
#include "phi_cons.h"     /* Philips Defined Constants                  */
#include "phi_priv.h"     /* for PHI private data storage */
#include "phi_prep.h"     /* Prototypes for Preprocessing Modules       */
#include "phi_lpc.h"      /* Prototypes for LPC Modules                 */
#include "phi_axit.h"     /* Prototypes for Excitation Analysis Module  */
#include "celp_bitstream_mux.h"     /* Prototypes for Bitstream Mux     */
#include "phi_apre.h"     /* Prototypes for Excitation Analysis Module  */
#include "phi_fbit.h"     /* Frame bit allocation table                 */
#include "celp_encoder.h" /* Prototypes for Coder Routines              */
#include "pan_celp_const.h"/* Constants for PAN Routines                */
#include "celp_proto_enc.h"/* Prototypes for CELP Routines              */
#include "nec_abs_const.h" /* Constants for NEC Routines                */
#include "nec_abs_proto.h" /* Prototypes for NEC Routines               */
#include "phi_nec_lpc.h"   /* Prototypes for NEC Routines               */
#include "nec_vad.h"       /* Prototypes for Silence Compr. Routines    */

unsigned long TX_flag_enc;

int CELPencDebugLevel = 0;  /* HP 971120 */

/*======================================================================*/
/*    L O C A L     F U N C T I O N     D E C L A R A T I O N           */
/*======================================================================*/
void nec_lpf_down( float xin[], float xout[], int len );

/*======================================================================*/
/*    L O C A L     D A T A     D E C L A R A T I O N                   */
/*======================================================================*/
/*======================================================================*/
/*    WideBand Data Declaration                                         */
/*======================================================================*/
static float *PHI_sp_pp_buf;
static float PHI_af;
static long  PHI_BUFFER;

static float *VAD_pcm_buffer;
static short prev_tx = 1;

static float *Downsampled_signal;

static float **SquaredHammingWindow;       /* For Haming Window         */
static float **HammingWindow;              /* For Haming Window         */
static float *PHI_GammaArrayBE;            /* For Bandwidth Expansion   */
static float *PAN_GammaArrayBE;            /* For Bandwidth Expansion   */

static float PHI_prev_x = (float)0.0;      /* Previous x value          */
static float PHI_prev_y = (float)0.0;      /* previous y value          */

static float NEC_prev_x = (float)0.0;      /* Previous x value          */
static float NEC_prev_y = (float)0.0;      /* previous y value          */


/*======================================================================*/
/*    NarrowBand Data declaration                                       */
/*======================================================================*/
static float *bws_sp_buffer;          /* input signal buffer */
static float *prev_Qlsp_coefficients; /* previous quantized LSP coeff. */
static float *buf_Qlsp_coefficients_bws; /* current quantized LSP coeff. */
static float *prev_Qlsp_coefficients_bws;/* previous quantized LSP coeff. */

static float gamma_num;    /* fixed or variable? */
static float gamma_den;   /* fixed or variable? */

static long num_indices, buffer_size;
static long num_enhstages;

static long buffer_size_bws;
static long frame_size_nb, frame_size_bws;
static long n_subframes_bws;
static long sbfrm_size_bws;
static long lpc_order_bws;
static long n_lpc_analysis_bws;
static long num_lpc_indices_bws;
static long *window_sizes_bws;
static long *window_offsets_bws;

/* ---------------------------------------------------------------------*/ 
/*Frame Related Information                                             */
/* ---------------------------------------------------------------------*/
static unsigned long frame_ctr = 0;    /* Frame Counter                 */
static unsigned long lpc_sent_frames=0;/* Frames where LPc is sent      */

/*======================================================================*/
/* Type definitions                                                     */
/*======================================================================*/

typedef struct
{
  PHI_PRIV_TYPE *PHI_Priv;
  /* add private data pointers here for other coding varieties */
} INST_CONTEXT_LPC_ENC_TYPE;


/*======================================================================*/
/* Function definition: celp_coder                                      */
/*======================================================================*/
void celp_coder (float       **InputSignal,                 /* In:  Multichannel Speech             */
                 HANDLE_BSBITSTREAM bitStreamLay[],               /* Out: Bitstream                       */
                 long        ExcitationMode,                /* In:  Excitation Mode                 */
                 long        SampleRateMode,                                                        
                 long        QuantizationMode,              /* In:  Type of Quantization            */
                 long        FineRateControl,               /* In:  Fine Rate Control switch        */
                 long        RPE_configuration,             /* In:  Wideband configuration          */
                 long        Wideband_VQ,                   /* In:  Wideband VQ mode                */
                 long        MPE_Configuration,             /* In:  Narrowband configuration        */
                 long        BandwidthScalabilityMode,      /* In:  bandwidth switch                */
                 long        PreProcessingSW,               /* In:  PreProcessingSW                 */
                 long        SilenceCompressionSW,          /* In:  SilenceCompressionSW            */
                 int         enc_objectVerType,             /* In:  object type: CELP or ER_CELP    */
                 long        frame_size,                    /* In:  Frame size                      */
                 long        n_subframes,                   /* In:  Number of subframes             */
                 long        sbfrm_size,                    /* In:  Subframe size                   */
                 long        lpc_order,                     /* In:  Order of LPc                    */
                 long        num_lpc_indices,               /* In:  Number of LPC indices           */
                 long        num_shape_cbks,                /* In:  Number of Shape Codebooks       */
                 long        num_gain_cbks,                 /* In:  Number of Gain Codebooks        */
                 long        n_lpc_analysis,                /* In:  Number of LPCs per frame        */
                 long        window_offsets[],              /* In:  Offset for LPC-frame v.window   */
                 long        window_sizes[],                /* In:  LPC Analysis Window size        */
                 long        max_n_lag_candidates,          /* In:  Maximum search candidates       */
                 long        org_frame_bit_allocation[],    /* In:  Frame Bit allocation            */
                 void        *InstanceContext,              /* In/Out: instance context             */
                 int         mp4ffFlag
)
{    
    /*==================================================================*/
    /*      L O C A L   D A T A   D E F I N I T I O N S                 */
    /*==================================================================*/
    float            af_next =(float)0.0;             /* First-order fit lpc parameter      */
    float            *sp_buf_ptr = InputSignal[0];    /* Speech Buffer                      */
    float            *int_ap;                         /* Interpolated LPC coeffs            */
    float            *ap = NULL;                      /* LPC Parameters                     */
    float            *ds_ap = NULL;
    long             *ds_window_offsets = NULL;
    long             *ds_window_sizes = NULL;
    long             *shape_indices = NULL;           /* Lags for Codebooks                 */
    long             *gain_indices = NULL;            /* Gains for Codebooks                */
    long             *codes = NULL;                   /* LPC codes                          */
    long             interpolation_flag  = 0;         /* Interpolation flag                 */
    long             LPC_Present = 1;                                                       
    long             signal_mode  = 0;                /* Signal mode                        */
    long             sbfrm_ctr = 0;                   /* Subframe counter                   */
    int              k;                                                                     
    float            *Downsampled_frame;                                                    
    long             rms_index;                                                             
                                                                                            
    float            *int_ap_bws = NULL;              /* Interpolated LPC coeffs            */
    float            *ap_bws = NULL;                  /* LPC Parameters                     */
    long             *shape_indices_bws = NULL;       /* Lags for Codebooks                 */
    long             *gain_indices_bws = NULL;        /* Gains for Codebooks                */
    long             *codes_bws = NULL;               /* LPC codes                          */
    long             *bws_nb_acb_index = NULL;        /* ACB codes                          */

    PHI_PRIV_TYPE    *PHI_Priv;
    int              stream_ctr;
    HANDLE_BSBITSTREAM bitStream;

    int              VAD_flag;
    float            cng_xnorm[CELP_MAX_SUBFRAMES];
    float            cng_lsp_coeff[SID_LPC_ORDER_WB];
    float            cng_lsp_coeff_bws[SID_LPC_ORDER_WB];
    unsigned long    dummy_pad = 0;
    int              force_lpc = 0;

    long             vad_analysis_window_size   = (NEC_FRAME20MS_FRQ16+NEC_SBFRM_SIZE80);
    long             vad_analysis_window_offset = (sbfrm_size / 2) + (frame_size / 2) - (vad_analysis_window_size / 2);


    /* -----------------------------------------------------------------*/
    /* Set up pointers to private data                                  */
    /* -----------------------------------------------------------------*/
    PHI_Priv = ((INST_CONTEXT_LPC_ENC_TYPE *)InstanceContext)->PHI_Priv;

    stream_ctr = 0;
    bitStream = bitStreamLay[stream_ctr];


    if (enc_objectVerType == CelpObjectV1)
    {
        SilenceCompressionSW = OFF;
    }



    if ( BandwidthScalabilityMode == OFF) 
    {
        /* -----------------------------------------------------------------*/
        /* Fill PCM buffer for VAD                                          */
        /* -----------------------------------------------------------------*/
        if (ExcitationMode == RegularPulseExc)
        {
            for ( k = 0; k < vad_analysis_window_size; k++)
            {
                VAD_pcm_buffer [k] = PHI_sp_pp_buf [k + vad_analysis_window_offset + frame_size];
            }
        }
        
        /* -----------------------------------------------------------------*/
        /* Segmentation for wideband speech                                 */
        /* -----------------------------------------------------------------*/
        for ( k = 0; k < (int)(PHI_BUFFER-frame_size); k++)
        {
            PHI_sp_pp_buf[k] = PHI_sp_pp_buf[k + (int)(frame_size)];
        }
        /* -----------------------------------------------------------------*/
        /* Preprocessing (In our case it is just a DC notch)                */
        /* -----------------------------------------------------------------*/
        if (PreProcessingSW == ON) 
        {
            celp_preprocessing(sp_buf_ptr, &PHI_sp_pp_buf[PHI_BUFFER-frame_size], &PHI_prev_x, &PHI_prev_y, frame_size);
        }
        else 
        {
            for ( k = 0; k < frame_size; k++)
            {
                PHI_sp_pp_buf[PHI_BUFFER-frame_size+k] = sp_buf_ptr[k];
            }
        }
    } 
    else 
    {
        for ( k = 0; k < (int)(PHI_BUFFER-frame_size_nb); k++) 
        {
            PHI_sp_pp_buf[k] = PHI_sp_pp_buf[k + (int)(frame_size_nb)];
        }
        
        for ( k = 0; k < (int)(buffer_size_bws-frame_size_bws); k++) 
        {
            bws_sp_buffer[k] = bws_sp_buffer[k + (int)(frame_size_bws)];
        }
        
        if (PreProcessingSW == ON) 
        {
            celp_preprocessing(sp_buf_ptr, &bws_sp_buffer[buffer_size_bws-frame_size_bws], &PHI_prev_x, &PHI_prev_y, frame_size_bws);
        }
        else 
        {
            
            for ( k = 0; k < frame_size_bws; k++)
            {
                bws_sp_buffer[buffer_size_bws-frame_size_bws+k] = sp_buf_ptr[k];
            }
        }
        
        nec_lpf_down( &bws_sp_buffer[buffer_size_bws-frame_size_bws],
            &PHI_sp_pp_buf[buffer_size-frame_size_nb], frame_size_bws );
    }

    /* -----------------------------------------------------------------*/
    /* Segmentation for downsampled speech                              */
    /* -----------------------------------------------------------------*/
    if ((ExcitationMode == RegularPulseExc) && (SampleRateMode == fs16kHz) && (QuantizationMode == VectorQuantizer) && (Wideband_VQ == Scalable_VQ))
    {
        for ( k = 0; k < (int)(PHI_BUFFER-frame_size)/2; k++)
        {
             Downsampled_signal[k] = Downsampled_signal[k + (int)(frame_size/2)];
        }
        /* -----------------------------------------------------------------*/
        /*  Allocate memory for downsampling frame                          */
        /* -----------------------------------------------------------------*/
        if
        (
        (( Downsampled_frame = (float *)malloc((unsigned int)(frame_size/2) * sizeof(float))) == NULL )
        )
        {
            fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
            CommonExit(1,"celp error");
        }

        /* -----------------------------------------------------------------*/
        /* Segmentation for downsampled speech                              */
        /* -----------------------------------------------------------------*/
        nec_lpf_down(sp_buf_ptr, Downsampled_frame, (int)frame_size);

        /* -----------------------------------------------------------------*/
        /* Preprocessing (In our case it is just a DC notch)                */
        /* -----------------------------------------------------------------*/
        celp_preprocessing(Downsampled_frame, &Downsampled_signal[(PHI_BUFFER-frame_size)/2], &NEC_prev_x, &NEC_prev_y, frame_size/2);

        /* -----------------------------------------------------------------*/
        /* Free allocated downsampled frame                                 */
        /* -----------------------------------------------------------------*/
        free(Downsampled_frame);
    }
    
    if (ExcitationMode == RegularPulseExc)
    {
        /* -----------------------------------------------------------------*/
        /* To synchronise the decoded output with the original signal       */
        /* This helps to make measurements such as SNR                      */
        /* -----------------------------------------------------------------*/
        if (frame_ctr < 2)
        {
            frame_ctr++;
            return;
        }
    }

    if (ExcitationMode == MultiPulseExc)
    {
        if (bitStream == NULL) return;
    }
    
    
    /* -----------------------------------------------------------------*/
    /* Create Arrays for frame processing                               */
    /* -----------------------------------------------------------------*/
    if
        (
        (( int_ap = (float *)malloc((unsigned int)(n_subframes * lpc_order) * sizeof(float))) == NULL )||
        (( ap     = (float *)malloc((unsigned int)(n_lpc_analysis * lpc_order) * sizeof(float))) == NULL )||
        (( shape_indices = (long *)malloc((unsigned int)((num_enhstages+1)*num_shape_cbks * n_subframes) * sizeof(long))) == NULL )||
        (( gain_indices = (long *)malloc((unsigned int)((num_enhstages+1)*num_gain_cbks * n_subframes) * sizeof(long))) == NULL )||
        (( ds_ap = (float *)malloc((unsigned int)(n_lpc_analysis * lpc_order) * sizeof(float))) == NULL )||
        (( ds_window_offsets = (long *)malloc((unsigned int)(n_lpc_analysis) * sizeof(long))) == NULL )||
        (( ds_window_sizes = (long *)malloc((unsigned int)(n_lpc_analysis) * sizeof(long))) == NULL )||
        (( codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL )
        )
    {
        fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
        CommonExit(1,"celp error");
    }

    if ( BandwidthScalabilityMode == ON) {
        if
            (
            (( int_ap_bws = (float *)malloc((unsigned int)(n_subframes_bws * lpc_order_bws) * sizeof(float))) == NULL )||
            (( ap_bws     = (float *)malloc((unsigned int)(n_lpc_analysis_bws * lpc_order_bws) * sizeof(float))) == NULL )||
            (( shape_indices_bws = (long *)malloc((unsigned int)(num_shape_cbks * n_subframes_bws) * sizeof(long))) == NULL )||
            (( gain_indices_bws = (long *)malloc((unsigned int)(num_gain_cbks * n_subframes_bws) * sizeof(long))) == NULL )||
            (( codes_bws = (long *)malloc((unsigned int)num_lpc_indices_bws * sizeof(long))) == NULL ) ||
            (( bws_nb_acb_index = (long *)malloc((unsigned int)n_subframes_bws * sizeof(long))) == NULL )
            )
        {
            fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
            CommonExit(1,"celp error");
        }
    }    

    /* -----------------------------------------------------------------*/
    /* LPC Analysis - I  (down sampled parameter computation)           */
    /* -----------------------------------------------------------------*/
    if ((ExcitationMode==RegularPulseExc) && (SampleRateMode == fs16kHz) && (QuantizationMode == VectorQuantizer) && (Wideband_VQ == Scalable_VQ))
    {
        long kk;
        float dmy_af;
        
        for (kk=0; kk < n_lpc_analysis; kk++)
        {
            ds_window_offsets[kk] = window_offsets[kk]/2;
            ds_window_sizes[kk]   = window_sizes[kk]/2;
        }
        
        celp_lpc_analysis (Downsampled_signal, 
                           ds_ap, 
                           &dmy_af, 
                           ds_window_offsets,
                           ds_window_sizes, 
                           HammingWindow, 
                           SampleRateMode, 
                           VAD_pcm_buffer, 
                           vad_analysis_window_size, 
                           &VAD_flag,
                           0,
                           PHI_GammaArrayBE, 
                           lpc_order/2, 
                           n_lpc_analysis);

    }

    /* -----------------------------------------------------------------*/
    /* LPC Analysis - II  (parameter computation)                       */
    /* -----------------------------------------------------------------*/

    
    if (ExcitationMode==RegularPulseExc)
    {

        celp_lpc_analysis (PHI_sp_pp_buf, 
                           ap, 
                           &af_next, 
                           window_offsets,
                           window_sizes, 
                           SquaredHammingWindow, 
                           SampleRateMode, 
                           VAD_pcm_buffer, 
                           vad_analysis_window_size, 
                           &VAD_flag,
                           0,
                           PHI_GammaArrayBE, 
                           lpc_order, 
                           n_lpc_analysis);
    }


    if (ExcitationMode==MultiPulseExc)
    {
        if (SampleRateMode == fs8kHz)
        {
            celp_lpc_analysis_lag (PHI_sp_pp_buf, ap, &af_next,
                                   window_offsets, window_sizes, HammingWindow,
                                   lpc_order, n_lpc_analysis,
                                   SampleRateMode, &VAD_flag
                   );
            if ( BandwidthScalabilityMode == ON)
            {
                float dmy_af_bws;
                celp_lpc_analysis_bws(bws_sp_buffer, ap_bws, &dmy_af_bws,
                    window_offsets_bws,
                    window_sizes_bws, lpc_order_bws,
                    n_lpc_analysis_bws); 
            }
        } 
        else {
            celp_lpc_analysis_lag(PHI_sp_pp_buf, ap, &af_next,
                  window_offsets, window_sizes, HammingWindow,
                  lpc_order, n_lpc_analysis,
                  SampleRateMode, &VAD_flag
                  );
        }
    }

    /* Calculate RMS and LSP for DTX/CNG */
    if (ExcitationMode==RegularPulseExc)
    {
        nec_rms_ave (&PHI_sp_pp_buf[(sbfrm_size / 2) + (sbfrm_ctr * sbfrm_size)],
                     cng_xnorm, 
                     frame_size, 
                     sbfrm_size, 
                     n_subframes, 
                     SampleRateMode,
                     VAD_flag);
        
        nec_lsp_ave (PHI_Priv->previous_uq_lsf_16, 
                     cng_lsp_coeff, 
                     lpc_order, 
                     n_lpc_analysis, 
                     RegularPulseExc,
                     VAD_flag);
    }
    
    if (ExcitationMode==MultiPulseExc)
    {
        if(SampleRateMode == fs8kHz )
        {
            nec_rms_ave (&PHI_sp_pp_buf[NEC_PAN_NLB + (sbfrm_ctr * sbfrm_size)],
                         cng_xnorm, 
                         frame_size_nb,
                         sbfrm_size,
                         n_subframes, 
                         SampleRateMode,
                         VAD_flag);
            
            nec_lsp_ave (&ap[lpc_order*(n_lpc_analysis-1)], 
                         cng_lsp_coeff, 
                         lpc_order, 
                         n_lpc_analysis, 
                         MultiPulseExc,
                         VAD_flag);
            
            if (BandwidthScalabilityMode == ON)
            {
                nec_lsp_ave_bws (&ap_bws[lpc_order_bws*(n_lpc_analysis_bws-1)],
                                 cng_lsp_coeff_bws, 
                                 lpc_order_bws, 
                                 n_lpc_analysis_bws,
                                 VAD_flag);
            }
        }
        else
        {
            nec_rms_ave (&PHI_sp_pp_buf[NEC_FRQ16_NLB+(sbfrm_ctr * sbfrm_size)],
                         cng_xnorm, 
                         frame_size, 
                         sbfrm_size, 
                         n_subframes, 
                         SampleRateMode,
                         VAD_flag);
            
            nec_lsp_ave (&ap[lpc_order*(n_lpc_analysis-1)], 
                         cng_lsp_coeff, 
                         lpc_order, 
                         n_lpc_analysis, 
                         MultiPulseExc,
                         VAD_flag);
        }
    }

    /* Hangover for VAD */
    if (BandwidthScalabilityMode == ON)
    {
        nec_hangover (SampleRateMode, frame_size_nb, &VAD_flag);
    }
    else
    {
        nec_hangover (SampleRateMode, frame_size, &VAD_flag);
    }

    /* DTX */
    if(BandwidthScalabilityMode == ON)
    {
        TX_flag_enc = nec_dtx (VAD_flag, lpc_order, cng_xnorm[n_subframes-1],
                               cng_lsp_coeff, SampleRateMode, frame_size_nb);
    }
    else
    {
        TX_flag_enc = nec_dtx (VAD_flag, lpc_order, cng_xnorm[n_subframes-1],
                               cng_lsp_coeff, SampleRateMode, frame_size);
    }
    
    /*   When Silence Compression is used in combination with FRC: first active 
         frame following a non-active frame must carry LPC data          RF 991123   */
    
    if ((TX_flag_enc == 1) && (prev_tx != 1))
    {
        force_lpc = 1;
    }
    else
    {
        force_lpc = 0;
    }
    
    if (SilenceCompressionSW == OFF)
    {
        TX_flag_enc = 1;
    }
    /* -----------------------------------------------------------------*/
    /* LPC Analysis - III (parameter quantization)                      */
    /* -----------------------------------------------------------------*/
    
    if (ExcitationMode==RegularPulseExc)
    {
        VQ_celp_lpc_quantizer  (ap, ds_ap, int_ap, codes, lpc_order, num_lpc_indices,
                                n_subframes, &interpolation_flag, 
                                cng_lsp_coeff, TX_flag_enc, force_lpc,
                                &LPC_Present, Wideband_VQ, PHI_Priv);
    }
    
    if (ExcitationMode==MultiPulseExc) 
    {
        if (FineRateControl == ON)
        {
            VQ_celp_lpc_quantizer  (ap, ds_ap, int_ap, codes, lpc_order, num_lpc_indices,
                                   n_subframes, &interpolation_flag, 
                                    cng_lsp_coeff, TX_flag_enc, force_lpc,
                                    &LPC_Present, Wideband_VQ, PHI_Priv);
        }
        else
        {   /*   FineRate Control off  */
            if (SampleRateMode == fs8kHz)
            {
                /* LPC quantization & interpolation */
                nb_abs_lpc_quantizer(ap, int_ap, codes, lpc_order,
                     n_lpc_analysis, 
                     n_subframes,
                     prev_Qlsp_coefficients, cng_lsp_coeff
                     );
                if ( BandwidthScalabilityMode == ON)
                {
                    for ( k = 0; k < lpc_order; k++ )
                    {
                        buf_Qlsp_coefficients_bws[k] = (float)(PAN_PI * prev_Qlsp_coefficients[k]);
                    }
                    
                    bws_lpc_quantizer(ap_bws, int_ap_bws, codes_bws,
                      lpc_order, lpc_order_bws,
                      num_lpc_indices_bws,
		      n_lpc_analysis_bws, 
                      n_subframes_bws,buf_Qlsp_coefficients_bws,
                      prev_Qlsp_coefficients_bws,
                      &org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+(num_enhstages+1)*n_subframes*(num_shape_cbks+num_gain_cbks)], cng_lsp_coeff_bws
                      );
                }
            }
            
            if (SampleRateMode == fs16kHz)
            {
                /*   16 khz Optimized VQ  */ 
                wb_celp_lsp_quantizer(ap, int_ap, codes, lpc_order, 
                      n_lpc_analysis,
                      n_subframes,
                      prev_Qlsp_coefficients,
                      cng_lsp_coeff
                      );
            }
        }
    }
    
    /* -----------------------------------------------------------------*/
    /* Also weight the first-order-fit lpc parameter                    */
    /* -----------------------------------------------------------------*/
    PHI_af *= (float)GAMMA_WF;


    if (ExcitationMode == RegularPulseExc)
    {
        static float mem_past_syn[NEC_LPC_ORDER_FRQ16];
        static long  flag_mem = 0;
        static long  flag_rms = 0;
        static float pqxnorm;
        int i;
        
        if (flag_mem == 0)
        {
            for ( i = 0; i < lpc_order; i++ ) 
            {
                mem_past_syn[i] = 0.0;
            }
            
            flag_mem = 1;
        }
        
        if (prev_tx != 1)
        {
            reset_cba_codebook (Lmax);
        }
        
        /* -----------------------------------------------------------------*/
        /* Begin Processing on a subframe basis                             */
        /* -----------------------------------------------------------------*/
        for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++)
        {
            long  m = sbfrm_ctr * lpc_order;
            float *sptr;
            long  *lag_cands;
            float *cbk_sig = NULL;
            float *a_parw = NULL;
            float *residue = NULL;
            float *dummy1 = NULL;
            float *Wnum_coeff = NULL;
            float *Wden_coeff = NULL;
            
            /* -------------------------------------------------------------*/
            /* Create Arrays for subframe processing                        */
            /* -------------------------------------------------------------*/
            if
                (
                (( lag_cands = (long  *)malloc((unsigned int)max_n_lag_candidates * sizeof(long))) == NULL )||
                (( residue   = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )||
                (( cbk_sig   = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )||
                (( a_parw    = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )||
                (( Wden_coeff=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL)    ||
                (( Wnum_coeff=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL)    ||
                (( dummy1    = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
                )
            {
                fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
                CommonExit(1,"celp error");
            }
            
            if (TX_flag_enc == 1)
            {
                /* -------------------------------------------------------------*/
                /* LPC inverse-filtering                                        */
                /* -------------------------------------------------------------*/
                
                sptr = PHI_sp_pp_buf + PHI_BUFFER - (3 * frame_size) + (sbfrm_ctr * sbfrm_size);
                
                celp_lpc_analysis_filter(sptr, residue, &int_ap[m], lpc_order, sbfrm_size, PHI_Priv);
                
                /* -------------------------------------------------------------*/
                /* Weighting filter parameter generation                        */
                /* -------------------------------------------------------------*/
                celp_weighting_module(&int_ap[m], lpc_order, dummy1, a_parw, (float)GAMMA_WF, (float)GAMMA_WF);
                
                /* -------------------------------------------------------------*/
                /* Excitation Analysis                                          */
                /* -------------------------------------------------------------*/
                celp_excitation_analysis ( residue,  a_parw, lpc_order,
                                          PHI_af,  lag_cands, 
                                          max_n_lag_candidates, sbfrm_size,
                                          n_subframes,
                                          &(shape_indices[sbfrm_ctr * num_shape_cbks]), 
                                          &(gain_indices[sbfrm_ctr * num_gain_cbks]),
                                          num_shape_cbks, num_gain_cbks, cbk_sig);
                
            }
            else
            {
                /* Confort Noise Generation in Non-active frame */
                mk_cng_excitation (int_ap, lpc_order, frame_size, sbfrm_size, 
                                   n_subframes, &rms_index, cbk_sig, SampleRateMode, 
                                   ExcitationMode, RPE_configuration, mem_past_syn, 
                                   &flag_rms, &pqxnorm, cng_xnorm, prev_tx);        
            }
            prev_tx = (short)TX_flag_enc;
            /* -------------------------------------------------------------*/
            /* Free   Arrays for subframe processing                        */
            /* -------------------------------------------------------------*/
            free ( lag_cands );
            free ( residue );
            free ( cbk_sig );
            free ( a_parw );
            free ( dummy1 );
            free(Wnum_coeff);
            free(Wden_coeff);
        }
    }
        
    if (ExcitationMode==MultiPulseExc) 
    {
        long NQlpc_cnt;
        float *sptr;
        float *wn_ap = NULL;
        float *wd_ap = NULL;
        float *dec_sig;
        float *bws_mp_sig;
        
        if ((( dec_sig   = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )||
            (( wn_ap=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL)    ||
            (( wd_ap=(float *)calloc(lpc_order*n_subframes, sizeof(float)))==NULL)
            ) 
        {
            fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
            CommonExit(1,"celp error");
        }
        
        if (SampleRateMode == fs8kHz) 
        {
            if ((bws_mp_sig = (float *)malloc((unsigned int)frame_size_nb * sizeof(float))) == NULL) 
            {
                fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
                CommonExit(1,"celp error");
            }
        }
        else 
        {
            if ((bws_mp_sig = (float *)malloc((unsigned int)frame_size * sizeof(float))) == NULL) 
            {
                fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
                CommonExit(1,"celp error");
            }
        }
        
        for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++) 
        {
            NQlpc_cnt = n_lpc_analysis*sbfrm_ctr/n_subframes;

            celp_weighting_module (ap+lpc_order*NQlpc_cnt,
                                   lpc_order, &wn_ap[lpc_order*sbfrm_ctr],
                                   &wd_ap[lpc_order*sbfrm_ctr],
                                   gamma_num, gamma_den);
        }
        
        for ( sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++ ) 
        {
            static float mem_past_syn[NEC_LPC_ORDER_FRQ16];
            static long  flag_mem = 0;
            static long  flag_rms = 0;
            static float pqxnorm;
            int i;
            
            if (flag_mem == 0)
            {
                for ( i = 0; i < lpc_order; i++ ) mem_past_syn[i] = 0.0;
                flag_mem = 1;
            }
            if ((FineRateControl == ON) || (QuantizationMode == ScalarQuantizer))
            {   
                sptr = PHI_sp_pp_buf + (sbfrm_ctr * sbfrm_size);
            }
            else
            {
                if (SampleRateMode == fs8kHz) {
                    sptr = PHI_sp_pp_buf + NEC_PAN_NLB + (sbfrm_ctr * sbfrm_size);
                } else {
                    sptr = PHI_sp_pp_buf + NEC_FRQ16_NLB + (sbfrm_ctr * sbfrm_size);
                }
            }
            
            if (SampleRateMode == fs8kHz)
            {
                if (TX_flag_enc == 1)
                {
                    nb_abs_excitation_analysis (sptr, int_ap, lpc_order, 
                                                wn_ap, wd_ap,
                                                frame_size_nb, sbfrm_size, 
                                                n_subframes, &signal_mode,
                                                org_frame_bit_allocation, 
                                                shape_indices, gain_indices, 
                                                num_shape_cbks, num_gain_cbks, 
                                                &rms_index, dec_sig, num_enhstages,
                                                bws_mp_sig+sbfrm_size*sbfrm_ctr,
                        SampleRateMode, mem_past_syn,
                        &flag_rms, &pqxnorm, prev_tx
          );
                }
                else
                {
                    /* Confort Noise Generation in Non-active frame */
                    mk_cng_excitation (int_ap, lpc_order, frame_size_nb, sbfrm_size, 
                                       n_subframes, &rms_index, dec_sig,
                                       SampleRateMode, ExcitationMode,
                                       MPE_Configuration, mem_past_syn,
                                       &flag_rms, &pqxnorm, cng_xnorm, prev_tx);
                }
                prev_tx = (short)TX_flag_enc;
            } 
            else 
            {
                if (TX_flag_enc == 1)
                {
                    nb_abs_excitation_analysis  (sptr, int_ap, lpc_order, 
                                                 wn_ap, wd_ap,
                                                 frame_size, sbfrm_size, 
                                                 n_subframes, &signal_mode,
                                                 org_frame_bit_allocation, 
                                                 shape_indices, gain_indices, 
                                                 num_shape_cbks, num_gain_cbks, 
                                                 &rms_index, dec_sig, num_enhstages,
                                                 bws_mp_sig+sbfrm_size*sbfrm_ctr,
                         SampleRateMode, mem_past_syn,
                         &flag_rms, &pqxnorm, prev_tx
      );
        }else{
          /* Confort Noise Generation in Non-active frame */
          mk_cng_excitation(int_ap, lpc_order, frame_size, sbfrm_size, 
                n_subframes, &rms_index, dec_sig,
                SampleRateMode, ExcitationMode, MPE_Configuration, mem_past_syn,
                &flag_rms, &pqxnorm, cng_xnorm, prev_tx);
         }
     prev_tx = (short)TX_flag_enc;
            }
        }
        
        if ( BandwidthScalabilityMode == ON) 
        {
            float *wn_ap_bws = NULL;
            float *wd_ap_bws = NULL;
            float *dec_sig_bws;
            
            static long  flag_rms = 0;
            static float pqxnorm;
            static float mem_past_syn[NEC_LPC_ORDER_MAXIMUM];
            static long  flag_mem = 0;
            static short prev_tx = 1;
            int i;
            
            if(flag_mem == 0)
            {
                for ( i = 0; i < lpc_order_bws; i++ ) mem_past_syn[i] = 0.0;
                flag_mem = 1;
            }
            if(
                (( dec_sig_bws   = (float *)malloc((unsigned int)sbfrm_size_bws * sizeof(float))) == NULL )||
                (( wn_ap_bws=(float *)calloc(lpc_order_bws*n_subframes_bws, sizeof(float)))==NULL)    ||
                (( wd_ap_bws=(float *)calloc(lpc_order_bws*n_subframes_bws, sizeof(float)))==NULL)
                )
            {
                fprintf(stderr, "MALLOC FAILURE in abs_coder  \n");
                CommonExit(1,"celp error");
            }
            
            for ( sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++ ) 
            {
                NQlpc_cnt = n_lpc_analysis_bws*sbfrm_ctr/n_subframes_bws;
                celp_weighting_module  (ap_bws+lpc_order_bws*NQlpc_cnt,
                                        lpc_order_bws,
                                        &wn_ap_bws[lpc_order_bws*sbfrm_ctr],
                                        &wd_ap_bws[lpc_order_bws*sbfrm_ctr],
                                        (float)NEC_GAM_MA, (float)NEC_GAM_AR);
            }
         if (TX_flag_enc == 1){
            for ( sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++ ) 
            {
                bws_nb_acb_index[sbfrm_ctr] = shape_indices[sbfrm_ctr*n_subframes/n_subframes_bws*num_shape_cbks];
            }
		 }
            
            for ( sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++ ) 
            {
                sptr = bws_sp_buffer + NEC_FRQ16_NLB + (sbfrm_ctr * sbfrm_size_bws);
                
                if (TX_flag_enc == 1)
                {
                    bws_excitation_analysis  (sptr, int_ap_bws, lpc_order_bws, 
                                              wn_ap_bws, wd_ap_bws,
                                              sbfrm_size_bws, 
                                              n_subframes_bws, signal_mode,
                                              &org_frame_bit_allocation[num_indices-n_subframes_bws*(num_shape_cbks+num_gain_cbks)],
                                              shape_indices_bws, gain_indices_bws, 
                                              num_shape_cbks, num_gain_cbks, 
                                              dec_sig_bws,
                                              bws_mp_sig+sbfrm_size*n_subframes/n_subframes_bws*sbfrm_ctr,
                                              bws_nb_acb_index, rms_index, mem_past_syn,
                                              &flag_rms, &pqxnorm, prev_tx
                          );
                }
                else
                {
                    /* Confort Noise Generation in Non-active frame */
                    mk_cng_excitation_bws  (int_ap_bws, lpc_order_bws,
                                            sbfrm_size_bws, n_subframes_bws,
                                            dec_sig_bws, rms_index, mem_past_syn,
                                            &flag_rms, &pqxnorm, prev_tx);
                }
                prev_tx = (short)TX_flag_enc;
            }
            
            free ( wn_ap_bws );
            free ( wd_ap_bws );
            free ( dec_sig_bws );
        }
        
        free ( wn_ap );
        free ( wd_ap );
        free ( dec_sig );
        free ( bws_mp_sig );
    }


    /*==================================================================*/
    /* CELP / ER CELP BITSTREAM MULTIPLEXING                            */
    /*==================================================================*/

    if (enc_objectVerType == CelpObjectV1)
    {
        /* CelpBaseFrame() */
        /* Celp_LPC() */
        /*==================================================================*/
        /* CELP Lpc Mux                                                     */
        /*==================================================================*/
        /* -------------------------------------------------------------*/
        /* Vector Quantization Mode                                     */
        /* -------------------------------------------------------------*/
        if (FineRateControl == ON) 
        {
            /* ---------------------------------------------------------*/
            /* Step I: Read interpolation_flag and LPC_present flag     */
            /* ---------------------------------------------------------*/
            BsPutBit(bitStream, interpolation_flag, 1);
            BsPutBit(bitStream, LPC_Present, 1);
            
            /* ---------------------------------------------------------*/
            /* Step II: If LPC is present                               */
            /* ---------------------------------------------------------*/
            if (LPC_Present == YES) 
            {
                /* LSP_VQ() */
                if (SampleRateMode == fs8kHz) 
                {
                    Write_NarrowBand_LSP(bitStream, codes);
                }
                
                if (SampleRateMode == fs16kHz) 
                {
                    Write_WideBand_LSP(bitStream, codes);
                }
                /* end of LSP_VQ() */
            }
        } 
        else 
        {
            /* LSP_VQ() */
            if (SampleRateMode == fs8kHz) 
            {
                Write_NarrowBand_LSP(bitStream, codes);
            }
            
            if (SampleRateMode == fs16kHz) 
            {
                Write_WideBand_LSP(bitStream, codes);
            }
            /* end of LSP_VQ() */
        }
        
        /*==================================================================*/
        /* CELP Excitation encoding                                         */
        /*==================================================================*/
        if ( ExcitationMode == RegularPulseExc )   /* RPE_Frame() */
        {
            long subframe;
            
            /*--------------------------------------------------------------*/
            /* Regular Pulse Excitation                                     */ 
            /*--------------------------------------------------------------*/
            for (subframe = 0; subframe < n_subframes; subframe++)
            {
                
                /* ---------------------------------------------------------*/
                /*Read the Adaptive Codebook Lag                            */
                /* ---------------------------------------------------------*/
                BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks], 8);
                
                /* ---------------------------------------------------------*/
                /*Read the Fixed Codebook Index (function of bit-rate)      */
                /* ---------------------------------------------------------*/
                switch (RPE_configuration)
                {
                    case     0   :   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 11);
                                     break;
                    case     1   :   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 11);
                                     break;
                    case     2   :   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 12);
                                     break;
                    case     3   :   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 12);
                                     break;
                }

                /* ---------------------------------------------------------*/
                /*Read the Adaptive Codebook Gain                           */
                /* ---------------------------------------------------------*/
                BsPutBit(bitStream, gain_indices[subframe * num_gain_cbks], 6);
                
                /* ---------------------------------------------------------*/
                /*Read the Fixed Codebook Gain (function of subframe)       */
                /*Later subframes are encoded differentially w.r.t previous */
                /* ---------------------------------------------------------*/
                
                if (subframe == 0)
                {
                    BsPutBit(bitStream, gain_indices[subframe * num_gain_cbks + 1], 5);
                }
                else
                {
                    BsPutBit(bitStream, gain_indices[subframe * num_gain_cbks + 1], 3);
                }
            }
        }
    
        if (ExcitationMode == MultiPulseExc)  /* MPE_frame() */
        {  
            /*--------------------------------------------------------------*/
            /* Multi-Pulse Excitation                                       */ 
            /*--------------------------------------------------------------*/
            long i;
            long mp_pos_bits, mp_sgn_bits;
            
            BsPutBit(bitStream, signal_mode, NEC_BIT_MODE);
            BsPutBit(bitStream, rms_index, NEC_BIT_RMS);
            
            if (SampleRateMode == fs8kHz) 
            {
                mp_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+1];
                mp_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+2];
                
                for ( i = 0; i < n_subframes; i++ ) 
                {
                    BsPutBit(bitStream,shape_indices[i*num_shape_cbks+0], NEC_BIT_ACB);
                    BsPutBit(bitStream,shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    BsPutBit(bitStream,shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    BsPutBit(bitStream,gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN);
                }
            }
            else 
            {
                mp_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+1];
                mp_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+2];
                
                for ( i = 0; i < n_subframes; i++ ) 
                {
                    BsPutBit(bitStream, shape_indices[i*num_shape_cbks+0], NEC_ACB_BIT_WB);
                    BsPutBit(bitStream, shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    BsPutBit(bitStream, shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    BsPutBit(bitStream, gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN_WB);
                }
            }
        }
    }


    if (enc_objectVerType == CelpObjectV2)
    {   
        long mp_pos_bits, mp_sgn_bits;
        int i;
        int send_lpc;

        if (SilenceCompressionSW == ON)
        {
            BsPutBit (bitStream, TX_flag_enc, 2);
        }
        
        /**********************************************************************************/
        /* Error Resilient re-ordering !                                                  */
        /**********************************************************************************/
        
        if ((FineRateControl == ON) && (LPC_Present == 0))
        {
            send_lpc = 0;
        }
        else
        {
            send_lpc = 1;
        }

        if ((SilenceCompressionSW == OFF) || (TX_flag_enc == 1))
        {
            /*  Active frames    */

            if (ExcitationMode == RegularPulseExc)
            {
                /* RPE_Frame() */
                long subframe;
            
                /**********************************************************************************/
                /* Write Class 0 Data                                                             */
                /**********************************************************************************/
                if (FineRateControl == ON) 
                {
                    BsPutBit(bitStream, interpolation_flag, 1);
                    BsPutBit(bitStream, LPC_Present, 1);
                }
            
                if (send_lpc)
                {
                    Write_WideBand_LSP_V2(bitStream, codes, 0);
                }
            
                /********************* Excitation Data *********************/
            
                for ( subframe = 0; subframe < n_subframes; subframe++ )
                {   
                    /* gain index 0:3-5 */
                    BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks]>>3)&7), 3);
                
                    /* gain index 1:sfr1: 3-4 else: 2*/
                    if (subframe == 0)
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1]>>3)&3), 2);
                    }
                    else
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1]>>2)&1), 1);
                    }
                }
            
                /********************* Excitation Data *********************/
            
                /**********************************************************************************/
                /* Write Class 1 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
            
                if (send_lpc)
                {
                    Write_WideBand_LSP_V2(bitStream, codes, 1);
                }
            
                /********************* Excitation Data *********************/
                /* Shape Delay: 5-7 */
                for ( subframe = 0; subframe < n_subframes; subframe++ ) 
                {
                    BsPutBit(bitStream, ((shape_indices[subframe * num_shape_cbks]>>5)&7), 3);
                }
            
                /**********************************************************************************/
                /* Write Class 2 Data                                                             */
                /**********************************************************************************/
            
                if (send_lpc)
                {
                    Write_WideBand_LSP_V2(bitStream, codes, 2);
                }
            
                /********************* Excitation Data *********************/
                for ( subframe = 0; subframe < n_subframes; subframe++ ) 
                {   
                    /* Shape Delay: 3-4 */
                    BsPutBit(bitStream, ((shape_indices[subframe * num_shape_cbks]>>3)&3), 2);
                
                    /* gain index 0:2 */
                    BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks]>>2)&1), 1);
                
                    /* gain index 1:sfr1: 2 else: 1*/
                    if (subframe == 0)
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1]>>2)&1), 1);
                    }
                    else
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1]>>1)&1), 1);
                    }
                }
            
                /**********************************************************************************/
                /* Write Class 3 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
            
                if (send_lpc)
                {
                    Write_WideBand_LSP_V2(bitStream, codes, 3);
                }
            
                /********************* Excitation Data *********************/
                /* Shape Delay: 1-2 */
                for ( subframe = 0; subframe < n_subframes; subframe++ ) 
                {
                    BsPutBit(bitStream, ((shape_indices[subframe * num_shape_cbks]>>1)&3), 2);
                }
            
                /**********************************************************************************/
                /* Write Class 4 Data                                                             */
                /**********************************************************************************/
            
                if (send_lpc)
                {
                    Write_WideBand_LSP_V2(bitStream, codes, 4);
                }
       
                for ( subframe = 0; subframe < n_subframes; subframe++ ) 
                {
                    /* Shape Delay: 0 */
                    BsPutBit(bitStream, (shape_indices[subframe * num_shape_cbks]&1), 1);
                
                    /* shape index: all */
                    switch (RPE_configuration)
                    {
                       case 0:   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 11);
                                 break;
                       case 1:   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 11);
                                 break;
                       case 2:   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 12);
                                 break;
                       case 3:   BsPutBit(bitStream, shape_indices[subframe * num_shape_cbks + 1], 12);
                                 break;
                    }
            
                    /* gain index 0:0-1 */
                    BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks])&3), 2);
                
                    /* gain index 1:sfr1: 0-1 else: 0*/
                    if (subframe == 0)
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1])&3), 2);
                    }
                    else
                    {
                        BsPutBit(bitStream, ((gain_indices[subframe * num_gain_cbks + 1])&1), 1);
                    }
                }
            }
            else
            {
                /**********************************************************************************/
                /* Re-orderes MPE Excitation 8&16 kHz                                             */
                /**********************************************************************************/
            
                if (SampleRateMode == fs8kHz) 
                {
                    mp_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+1];
                    mp_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+2];
                }
                else
                {
                    mp_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+1];
                    mp_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+2];
                }
            
                /**********************************************************************************/
                /* Write Class 0 Data                                                             */
                /**********************************************************************************/
                if (FineRateControl == ON) 
                {
                    BsPutBit(bitStream, interpolation_flag, 1);
                    BsPutBit(bitStream, LPC_Present, 1);
                }
            
                if (SampleRateMode == fs8kHz) 
                {
                    /********************* LPC Data *********************/
                    if (send_lpc) 
                    {
                        Write_NarrowBand_LSP_V2(bitStream, codes, 0);
                    }
                
                    /********************* Excitation Data *********************/
                
                    /* RMS ind: 4-5 */
                    BsPutBit(bitStream, ((rms_index>>4)&3), 2);
                
                    /* Shape Delay: 7 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>7)&1), 1);
                    }
                }
                else 
                {
                    /**********************************************************************/
                    /* 16 kHz                                                             */
                    /**********************************************************************/
                    if (send_lpc) 
                    {
                        Write_WideBand_LSP_V2(bitStream, codes, 0);
                    }
                    /********************* Excitation Data *********************/
                    /* RMS ind: 4-5 */
                    BsPutBit(bitStream, ((rms_index>>4)&3), 2);
                }
            
                /**********************************************************************************/
                /* Write Class 1 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (send_lpc) 
                    {
                        Write_NarrowBand_LSP_V2(bitStream, codes, 1);
                    }
                    /********************* Excitation Data *********************/
                
                    /* signal_mode: 1*/
                    BsPutBit (bitStream, signal_mode, 2);
                
                    /* Shape Delay: 5-6 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>5)&3), 2);
                    }
                
                } 
                else 
                {
                    /**********************************************************************/
                    /* 16 kHz                                                             */
                    /**********************************************************************/
                    if (send_lpc) 
                    {
                        Write_WideBand_LSP_V2(bitStream, codes, 1);
                    }
                
                    /********************* Excitation Data *********************/
                    /* signal_mode: 0-1*/
                    BsPutBit (bitStream, signal_mode, 2);
                
                    /* Shape Delay: 6-8 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>6)&7), 3);
                    }
                }
            
            
                /**********************************************************************************/
                /* Write Class 2 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (send_lpc) 
                    {
                        Write_NarrowBand_LSP_V2(bitStream, codes, 2);
                    }
                
                    /********************* Excitation Data *********************/
                    /* RMS ind: 3 */
                    BsPutBit(bitStream, ((rms_index>>3)&1), 1);
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 3-4 */
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>3)&3), 2);
                    
                        /* Gain Index: 0-1 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>0)&3), 2);
                    }
                } 
                else 
                {
                    /**********************************************************************/
                    /* 16 kHz                                                             */
                    /**********************************************************************/
                    if (send_lpc) 
                    {
                        Write_WideBand_LSP_V2(bitStream, codes, 2);
                    }
                
                    /********************* Excitation Data *********************/
                    /* RMS ind: 3 */
                    BsPutBit (bitStream, ((rms_index>>3)&1), 1);
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {    
                        /* Shape Delay: 4-5 */
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>4)&3), 2);
                    
                        /* Gain Index: 0-1 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>0)&3), 2);
                    }
                }
            
            
                /**********************************************************************************/
                /* Write Class 3 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (send_lpc) 
                    {
                        Write_NarrowBand_LSP_V2(bitStream, codes, 3);
                    }
                
                    /********************* Excitation Data *********************/
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 0-2 */
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0])&7), 3);

                        /* Shape Sign: all */
                        BsPutBit(bitStream,shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    
                        /* Gain Index: 2 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>2)&1), 1);
                    }
                }
                else 
                {
                    /**********************************************************************/
                    /* 16 kHz                                                             */
                    /**********************************************************************/
                    if (send_lpc) 
                    {
                        Write_WideBand_LSP_V2(bitStream, codes, 3);
                    }
                
                    /********************* Excitation Data *********************/
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        /* Shape Delay: 2-3 */
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>2)&3), 2);
                    
                        /* Shape Sign: all */
                        BsPutBit(bitStream,shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    
                        /* Gain Index: 2 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>2)&1), 1);
                    }
                }
            
            
                /**********************************************************************************/
                /* Write Class 4 Data                                                             */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (send_lpc) 
                    {
                        Write_NarrowBand_LSP_V2(bitStream, codes, 4);
                    }
                
                    /********************* Excitation Data *********************/
                
                    /* RMS ind: 0-2 */
                    BsPutBit(bitStream, ((rms_index)&7), 3);
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Position: all */
                        BsPutBit(bitStream,shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    
                        /* Gain Index: 3-5 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>3)&7), 3);
                    }
                }
                else 
                {
                    /**********************************************************************/
                    /* 16 kHz                                                             */
                    /**********************************************************************/
                    if (send_lpc) 
                    {
                        Write_WideBand_LSP_V2(bitStream, codes, 4);
                    }
                
                    /********************* Excitation Data *********************/
                    /* RMS ind: 0-2 */
                    BsPutBit(bitStream, ((rms_index>>0)&7), 3);
                
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 0-1 */
                        BsPutBit(bitStream,((shape_indices[i*num_shape_cbks+0]>>0)&3), 2);
                    
                        /* Shape Position: all */
                        BsPutBit(bitStream,shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    
                        /* Gain Index: 3-6 */
                        BsPutBit(bitStream,((gain_indices[i*num_gain_cbks+0]>>3)&15), 4);
                    }
                }
            }
        }
    }


    if (SilenceCompressionSW == ON)
    {
        /*  Non-active frames   */

        if (TX_flag_enc == 2)
        {
            if (SampleRateMode == fs8kHz)
            {
                BsPutBit (bitStream, codes[0], 4);
                BsPutBit (bitStream, codes[1], 4);
                BsPutBit (bitStream, codes[2], 7);
            }
            
            if (SampleRateMode == fs16kHz)
            {
                BsPutBit (bitStream, codes[0], 5);
                BsPutBit (bitStream, codes[1], 5);
                BsPutBit (bitStream, codes[2], 7);
                BsPutBit (bitStream, codes[3], 7);
                BsPutBit (bitStream, codes[5], 4);
                BsPutBit (bitStream, codes[6], 4);
            }
            
            BsPutBit (bitStream, rms_index, NEC_BIT_RMS);
        }
        
        if (TX_flag_enc == 3)
        {
            BsPutBit (bitStream, rms_index, NEC_BIT_RMS);
        }
    }


   
   if (ExcitationMode == MultiPulseExc) 
   {
       long i, j;
       long enh_pos_bits, enh_sgn_bits;
       long bws_pos_bits, bws_sgn_bits;
       
       /* CelpBRSenhFrame() */

       if (num_enhstages >= 1) 
       {
           if (SampleRateMode == fs8kHz) 
           {
               enh_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+n_subframes*(num_shape_cbks+num_gain_cbks)+1];
               enh_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+n_subframes*(num_shape_cbks+num_gain_cbks)+2];
           } 
           else 
           {
               enh_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+n_subframes*(num_shape_cbks+num_gain_cbks)+1];
               enh_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES_W+NEC_NUM_OTHER_INDICES+n_subframes*(num_shape_cbks+num_gain_cbks)+2];
           }
           
           if ((SilenceCompressionSW == OFF) || (TX_flag_enc == 1))
           {
               for ( j = 1; j <= num_enhstages; j++ ) 
               {
                   if (mp4ffFlag) 
                   {
                       stream_ctr++;
                   }
                   bitStream = bitStreamLay[stream_ctr];
                   
                   for ( i = 0; i < n_subframes; i++ ) 
                   {
                       BsPutBit(bitStream, shape_indices[(j*n_subframes+i)*num_shape_cbks+1], enh_pos_bits);
                       BsPutBit(bitStream, shape_indices[(j*n_subframes+i)*num_shape_cbks+2], enh_sgn_bits);
                       BsPutBit(bitStream, gain_indices[(j*n_subframes+i)*num_gain_cbks+0], NEC_BIT_ENH_GAIN);
                   }
               }
           } 
           else if (TX_flag_enc >= 2) 
           {
               if (mp4ffFlag) 
               {
                   stream_ctr += num_enhstages;
               }
               bitStream = bitStreamLay[stream_ctr];
               
           } 
       }
       
       /* CelpBWSenhFrame() */
       if ((SampleRateMode == fs8kHz) &&
           (FineRateControl == OFF) && 
           (QuantizationMode == VectorQuantizer) && 
           (BandwidthScalabilityMode == ON))
       {
           if ((SilenceCompressionSW == OFF) || (TX_flag_enc == 1))
           {
               if (mp4ffFlag) 
               {
                   stream_ctr++;
               }
               bitStream = bitStreamLay[stream_ctr];
               
               Write_BandScalable_LSP(bitStream, codes_bws);
               
               bws_pos_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+(num_enhstages+1)*n_subframes*(num_shape_cbks+num_gain_cbks)+NEC_NUM_LPC_INDICES_FRQ16+1];
               bws_sgn_bits = org_frame_bit_allocation[PAN_NUM_LPC_INDICES+NEC_NUM_OTHER_INDICES+(num_enhstages+1)*n_subframes*(num_shape_cbks+num_gain_cbks)+NEC_NUM_LPC_INDICES_FRQ16+2];
               
               for ( i = 0; i < n_subframes_bws; i++ ) 
               {
                   BsPutBit(bitStream, shape_indices_bws[i*num_shape_cbks+0], NEC_BIT_ACB_FRQ16);
                   BsPutBit(bitStream, shape_indices_bws[i*num_shape_cbks+1], bws_pos_bits);
                   BsPutBit(bitStream, shape_indices_bws[i*num_shape_cbks+2], bws_sgn_bits);
                   BsPutBit(bitStream, gain_indices_bws[i*num_gain_cbks+0], NEC_BIT_GAIN_FRQ16);
               }
           }
           else if( TX_flag_enc == 2 )
           {
               if (mp4ffFlag) 
               {
                   stream_ctr++;
               }
               bitStream = bitStreamLay[stream_ctr];
               
               BsPutBit(bitStream,codes_bws[0],4);
               BsPutBit(bitStream,codes_bws[1],7);
               BsPutBit(bitStream,codes_bws[2],4);
               BsPutBit(bitStream,codes_bws[3],6);
           }
       }
   }
   
   

   /*
        These stuffing bits are required ONLY in the current implementation
        of the MPEG-4 Audio Reference Software. They are NOT part of the
        MPEG-4 Version 2 CELP standard.

        The MPEG-4 Audio Reference Software currently requires coded
        frames to be 8 bits or longer, therefore inactive frames in V2 CELP
        (TX_flag == 0) need to be stuffed with 6 dummy bits.  They will be 
        removed from the final reference software when the MP4FF (MPEG-4 File Format) 
        and LATM (Low-overhead Audio Transport Multiplex) have been implemented.
     
        RF 991124

    */

    if ((SilenceCompressionSW == ON) && (TX_flag_enc == 0))
    {
        stream_ctr = 0;
        bitStream = bitStreamLay[stream_ctr];
        
        BsPutBit (bitStream, dummy_pad, 6);
    }

   /* -----------------------------------------------------------------*/
   /* Next becomes currrent af                                         */
   /* -----------------------------------------------------------------*/
   PHI_af  = af_next;
   
   /* -----------------------------------------------------------------*/
   /* Free Arrays that were malloced for Frame processing              */
   /* -----------------------------------------------------------------*/
   free ( int_ap );
   free ( ap );
   free ( shape_indices );
   free ( gain_indices );
   free ( codes );
   free ( ds_ap  );
   free ( ds_window_offsets );
   free ( ds_window_sizes );
   
   if (SampleRateMode == fs8kHz) 
   {
       if ( BandwidthScalabilityMode == ON) 
       {
           free ( int_ap_bws );
           free ( ap_bws );
           free ( shape_indices_bws );
           free ( gain_indices_bws );
           free ( codes_bws );
           free ( bws_nb_acb_index );
       }
   }
   
   /* -----------------------------------------------------------------*/
   /* Frame Counter                                                    */
   /* -----------------------------------------------------------------*/
   if (!interpolation_flag)
   {
       lpc_sent_frames++;
   }
   
   frame_ctr++;
   if ((frame_ctr % 10) == 0)
   {
       if (CELPencDebugLevel) 
       { /* HP 971120 */
           fprintf(stderr, "Frame Counter: %ld \r", frame_ctr);
       }
   }
}
/*======================================================================*/
/*   Function Definition:celp_initialisation_encoder                    */
/*======================================================================*/
void celp_initialisation_encoder (HANDLE_BSBITSTREAM hdrStream,               /* Out: Bitstream                               */
                                  long     bit_rate,                    /* In: bit rate                                 */
                                  long     sampling_frequency,          /* In: sampling frequency                       */
                                  long     ExcitationMode,              /* In: Excitation Mode                          */
                                  long     SampleRateMode,              /* In: SampleRate Mode                          */
                                  long     QuantizationMode,            /* In: Type of Quantization                     */
                                  long     FineRateControl,             /* In: Fine Rate Control switch                 */
                                  long     LosslessCodingMode,          /* In: Lossless Coding Mode                     */
                                  long     *RPE_configuration,          /* Out: RPE_configuration                       */
                                  long     *MPE_Configuration,          /* Out: Multi-Pulse Exc. configuration          */
                                  long     NumEnhLayers,                /* In: Number of Enhancement Layers for MPE     */
                                  long     BandwidthScalabilityMode,    /* In: bandwidth switch                         */
                                  long     *BWS_Configuration,          /* Out: BWS_configuration                       */
                                  long     BWS_nb_bitrate,              /* In: narrowband bitrate for BWS               */
                                  long     SilenceCompressionSW,        /* In: SilenceCompressionSW                     */
                                  int      enc_objectVerType,           /* In: object type: CELP or ER CELP             */ 
                                  long     InputConfiguration,          /* In:  desired RPE/MPE Configuration for FRC   */
                                  long     *frame_size,                 /* Out: frame size                              */
                                  long     *n_subframes,                /* Out: number of  subframes                    */
                                  long     *sbfrm_size,                 /* Out: subframe size                           */ 
                                  long     *lpc_order,                  /* Out: LP analysis order                       */
                                  long     *num_lpc_indices,            /* Out: number of LPC indices                   */
                                  long     *num_shape_cbks,             /* Out: number of Shape Codebooks               */
                                  long     *num_gain_cbks,              /* Out: number of Gain Codebooks                */ 
                                  long     *n_lpc_analysis,             /* Out: number of LP analysis per frame         */
                                  long     **window_offsets,            /* Out: window offset for each LP ana           */
                                  long     **window_sizes,              /* Out: window size for each LP ana             */
                                  long     *n_lag_candidates,           /* Out: number of pitch candidates              */
                                  float    *min_pitch_frequency,        /* Out: minimum pitch frequency                 */
                                  float    *max_pitch_frequency,        /* Out: maximum pitch frequency                 */
                                  long     **org_frame_bit_allocation,  /* Out: bit num. for each index                 */
                                  void     **InstanceContext,           /* Out: handle to initialised instance context  */
                                  int      sysFlag                      /* In: system interface(flexmux) flag           */
)
{
    
    INST_CONTEXT_LPC_ENC_TYPE   *InstCtxt;
    
    /* -----------------------------------------------------------------*/
    /* Create & initialise private storage for instance context         */
    /* -----------------------------------------------------------------*/
    if (( InstCtxt = (INST_CONTEXT_LPC_ENC_TYPE *)malloc(sizeof(INST_CONTEXT_LPC_ENC_TYPE))) == NULL )
    {
        fprintf(stderr, "MALLOC FAILURE in celp_initialisation_encoder  \n");
        CommonExit(1,"celp error");
    }
    
    if (( InstCtxt->PHI_Priv = (PHI_PRIV_TYPE *)malloc(sizeof(PHI_PRIV_TYPE))) == NULL )
    {
        fprintf(stderr, "MALLOC FAILURE in celp_initialisation_encoder  \n");
        CommonExit(1,"celp error");
    }
    PHI_Init_Private_Data(InstCtxt->PHI_Priv);
    
    *InstanceContext = InstCtxt;
    
    /* -----------------------------------------------------------------*/
    /*                                                                  */
    /* -----------------------------------------------------------------*/
    
    if (ExcitationMode == RegularPulseExc)
    {
        int k, x;
        long BIT_RATE_1 = 0;
        long BIT_RATE_2 = 0;
        long BIT_RATE_3 = 0;
        long BIT_RATE_4 = 0;
        long BIT_RATE_5 = 0;
        long BIT_RATE_6 = 0;
        long BIT_RATE_7 = 0;
        long BIT_RATE_8 = 0;  
        
        if (SilenceCompressionSW == 0)
        {
            if (SampleRateMode == fs16kHz)
            {
                if (FineRateControl == OFF)
                {   /*  VQ, FRC off  */
                    BIT_RATE_1 = 14400;
                    BIT_RATE_2 = 16000;
                    BIT_RATE_3 = 18667;
                    BIT_RATE_4 = 22533;
                    BIT_RATE_5 = 0;
                    BIT_RATE_6 = 0;
                    BIT_RATE_7 = 0;
                    BIT_RATE_8 = 0;
                }
                else
                {   /*  VQ, FRC on  */
                    BIT_RATE_1 = 13000;
                    BIT_RATE_2 = 14533;
                    BIT_RATE_3 = 13900;
                    BIT_RATE_4 = 16200;
                    BIT_RATE_5 = 17267;
                    BIT_RATE_6 = 18800;
                    BIT_RATE_7 = 21133;
                    BIT_RATE_8 = 22667;
                }
            }
        }
        else
        {
            /*  Silence compression ON   */
            if (FineRateControl == OFF)
            {   /*  VQ, FRC off  */
                BIT_RATE_1 = 14533;
                BIT_RATE_2 = 16200;
                BIT_RATE_3 = 18800;
                BIT_RATE_4 = 22667;
                BIT_RATE_5 = 0;
                BIT_RATE_6 = 0;
                BIT_RATE_7 = 0;
                BIT_RATE_8 = 0;
            }
            else
            {   /*  VQ, FRC on  */
                BIT_RATE_1 = 13133;
                BIT_RATE_2 = 14667;
                BIT_RATE_3 = 14100;
                BIT_RATE_4 = 16400;
                BIT_RATE_5 = 17400;
                BIT_RATE_6 = 18933;
                BIT_RATE_7 = 21267;
                BIT_RATE_8 = 22800;
            }
        }
        
        /* -----------------------------------------------------------------*/
        /*Check if the bitrate is set correctly                             */
        /* -----------------------------------------------------------------*/ 
        
        if (FineRateControl == OFF)
        {
            /* -----------------------------------------------------------------*/
            /* Check if a bit rate is one of the allowed bit rates              */
            /* -----------------------------------------------------------------*/ 
            if (bit_rate == BIT_RATE_1)
            {
                *RPE_configuration = 0;
                *frame_size = FIFTEEN_MS;
                *n_subframes = 6;        
            }
            else if (bit_rate == BIT_RATE_2)
            {
                *RPE_configuration = 1;
                *frame_size  = TEN_MS;
                *n_subframes = 4;        
            }
            else if (bit_rate == BIT_RATE_3)
            {
                *RPE_configuration = 2;
                *frame_size  = FIFTEEN_MS;
                *n_subframes = 8;
            }
            else if (bit_rate == BIT_RATE_4)
            {
                *RPE_configuration = 3;
                *frame_size  = FIFTEEN_MS;
                *n_subframes = 10;
            }
            else
            {
                fprintf (stderr, "ERROR: Bit Rate not in the set of supported bit rates \n");
                fprintf (stderr, "Supported bit rates when FineRate Control disabled: \n");
                fprintf (stderr, "                     RPE_Configuration 0: %ld bps\n", BIT_RATE_1);
                fprintf (stderr, "                     RPE_Configuration 1: %ld bps\n", BIT_RATE_2);
                fprintf (stderr, "                     RPE_Configuration 2: %ld bps\n", BIT_RATE_3);
                fprintf (stderr, "                     RPE_Configuration 3: %ld bps\n", BIT_RATE_4);
                exit (1);
            }
        }
        else
        {   /*  FineRate Control on  */
            
            /* -----------------------------------------------------------------*/
            /*  Determine RPE_Configuration                                     */
            /* -----------------------------------------------------------------*/ 
            
            if (InputConfiguration == -1)
            {
                /*   RPE_Configuration not specified on command line:
                     determine configuration from bitrate                    */
                
                if ((bit_rate >= BIT_RATE_1) && (bit_rate <= BIT_RATE_3))
                {
                    *RPE_configuration = 0;
                }
                else if ((bit_rate > BIT_RATE_3) && (bit_rate <= BIT_RATE_4))
                {
                    *RPE_configuration = 1;
                }
                else if ((bit_rate >= BIT_RATE_5) && (bit_rate <= BIT_RATE_6))
                {
                    *RPE_configuration = 2;
                }
                else if ((bit_rate >= BIT_RATE_7) && (bit_rate <= BIT_RATE_8))
                {
                    *RPE_configuration = 3;
                }
                else
                {
                    fprintf (stderr, "ERROR: Bit Rate not in the set of supported bit rates \n");
                    fprintf (stderr, "Supported bit rates when FineRate Control enabled: \n");
                    fprintf (stderr, "                     RPE_Configuration 0: %ld-%ld bps\n", BIT_RATE_1, BIT_RATE_2);          
                    fprintf (stderr, "                     RPE_Configuration 1: %ld-%ld bps\n", BIT_RATE_3, BIT_RATE_4);
                    fprintf (stderr, "                     RPE_Configuration 2: %ld-%ld bps\n", BIT_RATE_5, BIT_RATE_6);
                    fprintf (stderr, "                     RPE_Configuration 3: %ld-%ld bps\n", BIT_RATE_7, BIT_RATE_8);
                    exit (1);
                }
            }
            else
            {
                /*   RPE_Configuration specified on command line: check consistency
                     with the specified bitrate                                      */
                
                if (((InputConfiguration == 0) && ((bit_rate < BIT_RATE_1) || (bit_rate > BIT_RATE_2))) ||
                    ((InputConfiguration == 1) && ((bit_rate < BIT_RATE_3) || (bit_rate > BIT_RATE_4))) ||
                    ((InputConfiguration == 2) && ((bit_rate < BIT_RATE_5) || (bit_rate > BIT_RATE_6))) ||
                    ((InputConfiguration == 3) && ((bit_rate < BIT_RATE_7) || (bit_rate > BIT_RATE_8))))
                {
                    fprintf (stderr, "ERROR: Bit Rate not in the set of supported bit rates \n");
                    fprintf (stderr, "Supported bit rates when FineRate Control enabled: \n");
                    fprintf (stderr, "                     RPE_Configuration 0: %ld-%ld bps\n", BIT_RATE_1, BIT_RATE_2);          
                    fprintf (stderr, "                     RPE_Configuration 1: %ld-%ld bps\n", BIT_RATE_3, BIT_RATE_4);
                    fprintf (stderr, "                     RPE_Configuration 2: %ld-%ld bps\n", BIT_RATE_5, BIT_RATE_6);
                    fprintf (stderr, "                     RPE_Configuration 3: %ld-%ld bps\n", BIT_RATE_7, BIT_RATE_8);
                    exit (1);
                }
                else
                {
                    *RPE_configuration = InputConfiguration;
                }
            }
            
            /* -----------------------------------------------------------------*/
            /* Set CELP coding parameters according to RPE_Confguration         */
            /* -----------------------------------------------------------------*/ 
            
            switch (*RPE_configuration)
            {
                case     0   :  *frame_size  = FIFTEEN_MS;
                                *n_subframes = 6;
                                break;
                case     1   :  *frame_size  = TEN_MS;
                                *n_subframes = 4;
                                break;
                case     2   :  *frame_size  = FIFTEEN_MS;
                                *n_subframes = 8;
                                break;
                case     3   :  *frame_size  = FIFTEEN_MS;
                                *n_subframes = 10;
                                break;
                default      :  fprintf (stderr, "ERROR: Invalid RPE_Configuration \n");
                                exit (1);
                                break;          
            }
        }
        
        
        if (SampleRateMode == fs16kHz)
        {
            *lpc_order = ORDER_LPC_16;
        }
        
        *sbfrm_size = (*frame_size) / (*n_subframes);
        
        if ((prev_Qlsp_coefficients=(float *)calloc(*lpc_order, sizeof(float)))==NULL)
        {
            fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
            exit(5);
        }
        
        for (x = 0;x < (*lpc_order); x++)
        {
            *(prev_Qlsp_coefficients + x) = (x + 1) / (float)((*lpc_order) + 1);
        }
        
        *num_lpc_indices = N_INDICES_VQ16;
        
        *n_lpc_analysis      = 1;
        *n_lag_candidates    = MAX_N_LAG_CANDIDATES;
        *min_pitch_frequency = (float)1.0/(float)Lmax;
        *max_pitch_frequency = (float)1.0/(float)Lmin;
        PHI_BUFFER           = ((3 * (*frame_size)) + (*sbfrm_size)/2);  /*  RPE only */
        *num_shape_cbks      = 2;
        *num_gain_cbks       = 2;
        
        /* -----------------------------------------------------------------*/
        /* Malloc  parameters  and set them                                 */
        /* -----------------------------------------------------------------*/
        if
            (
            (( *window_offsets = (long *)malloc((unsigned int)(*n_lpc_analysis) * sizeof(long))) == NULL )||
            (( PHI_sp_pp_buf = (float *)malloc((unsigned int)PHI_BUFFER *  sizeof(float))) == NULL )||
            (( VAD_pcm_buffer = (float *)malloc((unsigned int)(NEC_FRAME20MS_FRQ16+NEC_SBFRM_SIZE80) *  sizeof(float))) == NULL )||
            (( Downsampled_signal = (float *)malloc((unsigned int)(PHI_BUFFER/2) *  sizeof(float))) == NULL )||
            (( *window_sizes   = (long *)malloc((unsigned int)(*n_lpc_analysis) * sizeof(long))) == NULL )
            )
        {
            fprintf(stderr,"MALLOC FAILURE in Routine abs_initialisation_encoder \n");
            CommonExit(1,"celp error");
        } 
        
        if (*RPE_configuration == 1)
        {
            (*window_sizes)[0]   = TWENTY_MS;     
        }
        else
        {
            (*window_sizes)[0]   = TWENTY_MS + FIVE_MS;     
        }
        
        (*window_offsets)[0] = 2 * (*frame_size) - (*window_sizes)[0]/2;
        
        /* -----------------------------------------------------------------*/
        /* Initilialising data                                              */
        /* -----------------------------------------------------------------*/ 
        for(k = 0; k < (int)PHI_BUFFER; k++)
        {
            PHI_sp_pp_buf[k] = (float)0.0;
        }
        
        /* -----------------------------------------------------------------*/
        /* Call various init routines to allocate memory                    */
        /* -----------------------------------------------------------------*/ 
        PHI_allocate_energy_table(*sbfrm_size, ACBK_SIZE, Sa);
        
        PHI_init_excitation_analysis(Lmax, *lpc_order, *sbfrm_size, *RPE_configuration);
        
        *org_frame_bit_allocation = PHI_init_bit_allocation(SampleRateMode, *RPE_configuration,
            FineRateControl, *num_lpc_indices,
            *n_subframes, *num_shape_cbks,
            *num_gain_cbks); 
        
        PHI_InitLpcAnalysisEncoder (ORDER_LPC_16, ORDER_LPC_8,
                                    bit_rate, sampling_frequency, 
                                    *frame_size, *num_lpc_indices, *n_subframes, *num_shape_cbks, 
                                    *num_gain_cbks, *org_frame_bit_allocation,
                                    SilenceCompressionSW,
                                    InstCtxt->PHI_Priv);
        
        /* -----------------------------------------------------------------*/
        /* Initialise windows for wideband LPC analysis                     */
        /* -----------------------------------------------------------------*/ 
        
        {                  
            int     x;
            float   *pin;
            float   *pout;
            
            long i;
            
            if ((SquaredHammingWindow=(float **)calloc((unsigned int)*n_lpc_analysis, sizeof(float *)))==NULL)
            {
                fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                CommonExit(1,"celp error");
            }
            
            for (i=0;i<*n_lpc_analysis;i++) 
            {
                if((SquaredHammingWindow[i]=(float *)calloc((unsigned int)(*window_sizes)[i], sizeof(float)))==NULL)
                {
                    fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                    CommonExit(1,"celp error");
                }
                
                /* -------------------------------------------------------------*/       
                /* Generate Hamming Window For Lpc                              */
                /* -------------------------------------------------------------*/ 
                pout = SquaredHammingWindow[i];     
                for(x=0; x < (int)(*window_sizes)[i];x++)
                {
                    *pout++ = (float)(0.540000 - 0.460000 * cos(6.283185307 * ((double)x-(double)0.00000)/(double)((*window_sizes)[i])));
                }
                
                for(x = 0; x < (int)(*window_sizes)[i]; x++)
                {
                    SquaredHammingWindow[i][x] *= SquaredHammingWindow[i][x];
                }
            }
            
            /* -----------------------------------------------------------------*/       
            /* Generate Gamma Array for Bandwidth Expansion                     */
            /* -----------------------------------------------------------------*/       
            if
                (
                (( PHI_GammaArrayBE = (float *)malloc((unsigned int)(*lpc_order) * sizeof(float))) == NULL )
                )
            {
                fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                CommonExit(1,"celp error");
            }
            
            pin     = PHI_GammaArrayBE;
            pout    = PHI_GammaArrayBE;
            *pout++ = (float)GAMMA_BE;
            for (x=1; x < (int)*lpc_order; x++)
            {
                *pout++ = (float)GAMMA_BE * *pin++;
            }
        }
        
        gamma_num=(float)PAN_GAM_MA;
        gamma_den=(float)PAN_GAM_AR;
        
        /* -----------------------------------------------------------------*/
        /* Initialise windows for LPC analysis                              */
        /* -----------------------------------------------------------------*/ 
        {                  
            int     x;
            float   *pin;
            float   *pout;
            long    i;
            
            if ((HammingWindow=(float **)calloc((unsigned int)*n_lpc_analysis, sizeof(float *)))==NULL)
            {
                fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                CommonExit(1,"celp error");
            }
            
            for (i=0;i<*n_lpc_analysis;i++) 
            {
                if ((HammingWindow[i]=(float *)calloc((unsigned int)(((*window_sizes)[i])/2), sizeof(float)))==NULL)
                {
                    fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                    CommonExit(1,"celp error");
                }
                
                /* -------------------------------------------------------------*/       
                /* Generate Hamming Window For Lpc                              */
                /* -------------------------------------------------------------*/ 
                pout = HammingWindow[i];    
                for (x=0; x < (int)(((*window_sizes)[i])/2);x++)
                {
                    *pout++ = (float)(0.540000 - 0.460000 * cos(6.283185307 * ((double)x-(double)0.00000)/(double)((((*window_sizes)[i])/2))));
                }
            }
            
            /* -----------------------------------------------------------------*/       
            /* Generate Gamma Array for Bandwidth Expansion                     */
            /* -----------------------------------------------------------------*/       
            if
                (
                (( PAN_GammaArrayBE = (float *)malloc((unsigned int)(*lpc_order/2) * sizeof(float))) == NULL )
                )
            {
                fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                CommonExit(1,"celp error");
            }
            pin     = PAN_GammaArrayBE;
            pout    = PAN_GammaArrayBE;
            *pout++ = (float)PAN_GAMMA_BE;
            
            for(x=1; x < (int)*lpc_order/2; x++)
            {
                *pout++ = (float)PAN_GAMMA_BE * *pin++;
            }
        }
    }   
    

    if (ExcitationMode == MultiPulseExc) 
    {
        if (SampleRateMode == fs8kHz) {
            int i, j;
            long    target_mp_bits, mp_pos_bits, mp_sgn_bits;
            long    enh_pos_bits, enh_sgn_bits;
            long    bws_pos_bits, bws_sgn_bits;
            long    ctr, calc_bit_rate;
            long    CoreBitRate;
            long    num_bits_lpc_int;
            long    BITRATE1 = 0;
            long    BITRATE2 = 0;
            long    BITRATE3 = 0;
            long    BITRATE4 = 0;
            long    BITRATE5 = 0;
            long    BITRATE6 = 0;
            long    BITRATE7 = 0;
            long    len_lpc_analysis = 0;
        long    min_bit_rate, max_bit_rate;
            
            if (SilenceCompressionSW == OFF) {
                if (QuantizationMode == VectorQuantizer) {
                    BITRATE1=3850;
                    BITRATE2=4900;
                    BITRATE3=5700;
                    BITRATE4=7700;
                    BITRATE5=11000;
                    BITRATE6=12200;
                    BITRATE7=6200;
                }
            }else{
                BITRATE1=3850+50;
                BITRATE2=4900+66;
                BITRATE3=5700+100;
                BITRATE4=7700+100;
                BITRATE5=11000+200;
                BITRATE6=12200+200;
                BITRATE7=6200+66;
            }
            
            num_enhstages = NumEnhLayers;
            if (BandwidthScalabilityMode==ON) {
                CoreBitRate = BWS_nb_bitrate - NEC_ENH_BITRATE * num_enhstages;
            } else {
                CoreBitRate = bit_rate - NEC_ENH_BITRATE * num_enhstages;
            }
            if ( CoreBitRate == BITRATE7 && num_enhstages > 0 ) {
                fprintf(stderr,"Error: BitRateScalable mode is not suported at the Specified BitRate.\n");
                CommonExit(1,"celp error");
            }
            
            *lpc_order = NEC_LPC_ORDER;
            *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
            *num_gain_cbks = NEC_NUM_GAIN_CBKS;
            *num_lpc_indices = PAN_NUM_LPC_INDICES;
            *n_lag_candidates = PAN_N_PCAN_INT;
            *min_pitch_frequency = sampling_frequency/(float)NEC_PITCH_MAX;
            *max_pitch_frequency = sampling_frequency/(float)NEC_PITCH_MIN;
            
            if (FineRateControl == ON) {
                num_bits_lpc_int = 2+PAN_BIT_LSP22_0
                    +PAN_BIT_LSP22_1+PAN_BIT_LSP22_2
                    +PAN_BIT_LSP22_3+PAN_BIT_LSP22_4;
            } else {
                num_bits_lpc_int = PAN_BIT_LSP22_0+PAN_BIT_LSP22_1+PAN_BIT_LSP22_2
                    +PAN_BIT_LSP22_3+PAN_BIT_LSP22_4;
            }
            if (SilenceCompressionSW == ON) num_bits_lpc_int += 2;
            
        if ( FineRateControl == OFF ) {
          if ( CoreBitRate == BITRATE7 ) {
        frame_size_nb = NEC_FRAME30MS;
        *n_subframes = NEC_NSF4;
        len_lpc_analysis = NEC_FRAME30MS/NEC_NSF4;
          } else {
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
        if ( CoreBitRate >= BITRATE1 && CoreBitRate < BITRATE2 ) {
          frame_size_nb = NEC_FRAME40MS;
          *n_subframes = NEC_NSF4;
        }
        if ( CoreBitRate >= BITRATE2 && CoreBitRate < BITRATE3 ) {
          frame_size_nb = NEC_FRAME30MS;
          *n_subframes = NEC_NSF3;
        }
        if ( CoreBitRate >= BITRATE3 && CoreBitRate < BITRATE4 ) {
          frame_size_nb = NEC_FRAME20MS;
          *n_subframes = NEC_NSF2;
        }
        if ( CoreBitRate >= BITRATE4 && CoreBitRate < BITRATE5 ) {
          frame_size_nb = NEC_FRAME20MS;
          *n_subframes = NEC_NSF4;
        }
        if ( CoreBitRate >= BITRATE5 && CoreBitRate <= BITRATE6 ) {
          frame_size_nb = NEC_FRAME10MS;
          *n_subframes = NEC_NSF2;
        }
        if ( CoreBitRate < BITRATE1 || CoreBitRate > BITRATE6 ) {
          fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n",
              CoreBitRate);
          CommonExit(1,"celp error"); 
        }
          }
        } else {
          *MPE_Configuration = InputConfiguration;
          if ( *MPE_Configuration >= 0 && *MPE_Configuration < 3 ) {
                frame_size_nb = NEC_FRAME40MS;
                *n_subframes = NEC_NSF4;
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
          }
          if ( *MPE_Configuration >= 3 && *MPE_Configuration < 6 ) {
                frame_size_nb = NEC_FRAME30MS;
                *n_subframes = NEC_NSF3;
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
          }
          if ( *MPE_Configuration >= 6 && *MPE_Configuration < 13 ) {
                frame_size_nb = NEC_FRAME20MS;
                *n_subframes = NEC_NSF2;
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
          }
          if ( *MPE_Configuration >= 13 && *MPE_Configuration < 22 ) {
                frame_size_nb = NEC_FRAME20MS;
                *n_subframes = NEC_NSF4;
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
          }
          if ( *MPE_Configuration >= 22 && *MPE_Configuration < 27 ) {
                frame_size_nb = NEC_FRAME10MS;
                *n_subframes = NEC_NSF2;
        len_lpc_analysis = NEC_LEN_LPC_ANALYSIS;
          }
          if ( *MPE_Configuration == 27 ) {
                frame_size_nb = NEC_FRAME30MS;
                *n_subframes = NEC_NSF4;
        len_lpc_analysis = NEC_FRAME30MS/NEC_NSF4;
          }
          if ( *MPE_Configuration > 27 ) {
                fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                CommonExit(1,"celp error"); 
          }
        }
                
        *sbfrm_size = (frame_size_nb)/(*n_subframes);
        *n_lpc_analysis = (frame_size_nb)/len_lpc_analysis;
                
        num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES
                    + (num_enhstages + 1) * (*n_subframes) *
                    (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);

        if ( FineRateControl == OFF ) {
          target_mp_bits=(long)(((float)(frame_size_nb)*CoreBitRate/8000
                    - (num_bits_lpc_int+NEC_BIT_MODE
                    +NEC_BIT_RMS))
                    /(*n_subframes)
                    - (NEC_BIT_ACB + NEC_BIT_GAIN));
                
          nec_mp_config((*sbfrm_size),target_mp_bits,
                    &mp_pos_bits,&mp_sgn_bits);
                
          calc_bit_rate=(long)(((num_bits_lpc_int+NEC_BIT_MODE+NEC_BIT_RMS)
                    +(NEC_BIT_ACB+mp_pos_bits+mp_sgn_bits
                    +NEC_BIT_GAIN)*(*n_subframes))
                    *8000/(frame_size_nb));
                
          if ( CoreBitRate != calc_bit_rate ) {
        fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n", bit_rate);
        fprintf(stderr,"\t Please set BitRate at %ld\n",
            calc_bit_rate+num_enhstages*NEC_ENH_BITRATE);
        CommonExit(1,"celp error"); 
          }

          if ( CoreBitRate == BITRATE7 ) {
        *MPE_Configuration = 27;
          } else {
        if ( CoreBitRate >= BITRATE1 && CoreBitRate < BITRATE2 ) {
          *MPE_Configuration = mp_sgn_bits-3;
        }
        if ( CoreBitRate >= BITRATE2 && CoreBitRate < BITRATE3 ) {
          *MPE_Configuration = (mp_sgn_bits-5)+3;
        }
        if ( CoreBitRate >= BITRATE3 && CoreBitRate < BITRATE4 ) {
          *MPE_Configuration = (mp_sgn_bits-6)+6;
        }
        if ( CoreBitRate >= BITRATE4 && CoreBitRate < BITRATE5 ) {
          *MPE_Configuration = (mp_sgn_bits-4)+13;
        }
        if ( CoreBitRate >= BITRATE5 && CoreBitRate <= BITRATE6 ) {
          *MPE_Configuration = (mp_sgn_bits-8)+22;
        }
          }
        } else {
          switch ( *MPE_Configuration ) {
          case 0:
                 mp_pos_bits = 14; mp_sgn_bits =  3; break;
          case 1:
                 mp_pos_bits = 17; mp_sgn_bits =  4; break;
          case 2:
                 mp_pos_bits = 20; mp_sgn_bits =  5; break;
          case 3:
                 mp_pos_bits = 20; mp_sgn_bits =  5; break;
          case 4:
                 mp_pos_bits = 22; mp_sgn_bits =  6; break;
          case 5:
                 mp_pos_bits = 24; mp_sgn_bits =  7; break;
          case 6:
                 mp_pos_bits = 22; mp_sgn_bits =  6; break;
          case 7:
                 mp_pos_bits = 24; mp_sgn_bits =  7; break;
          case 8:
                 mp_pos_bits = 26; mp_sgn_bits =  8; break;
          case 9:
                 mp_pos_bits = 28; mp_sgn_bits =  9; break;
          case 10:
                 mp_pos_bits = 30; mp_sgn_bits = 10; break;
          case 11:
                 mp_pos_bits = 31; mp_sgn_bits = 11; break;
          case 12:
                 mp_pos_bits = 32; mp_sgn_bits = 12; break;
          case 13:
                 mp_pos_bits = 13; mp_sgn_bits =  4; break;
          case 14:
                 mp_pos_bits = 15; mp_sgn_bits =  5; break;
          case 15:
                 mp_pos_bits = 16; mp_sgn_bits =  6; break;
          case 16:
                 mp_pos_bits = 17; mp_sgn_bits =  7; break;
          case 17:
                 mp_pos_bits = 18; mp_sgn_bits =  8; break;
          case 18:
                 mp_pos_bits = 19; mp_sgn_bits =  9; break;
          case 19:
                 mp_pos_bits = 20; mp_sgn_bits =  10; break;
          case 20:
                 mp_pos_bits = 20; mp_sgn_bits =  11; break;
          case 21:
                 mp_pos_bits = 20; mp_sgn_bits =  12; break;
          case 22:
                 mp_pos_bits = 18; mp_sgn_bits =  8; break;
          case 23:
                 mp_pos_bits = 19; mp_sgn_bits =  9; break;
          case 24:
                 mp_pos_bits = 20; mp_sgn_bits =  10; break;
          case 25:
                 mp_pos_bits = 20; mp_sgn_bits =  11; break;
          case 26:
                 mp_pos_bits = 20; mp_sgn_bits =  12; break;
          case 27:
                 mp_pos_bits = 19; mp_sgn_bits =  6; break;
          }
          max_bit_rate=(long)((float)((num_bits_lpc_int+NEC_BIT_MODE
                       +NEC_BIT_RMS)
                      +(NEC_BIT_ACB+mp_pos_bits+mp_sgn_bits
                        +NEC_BIT_GAIN)*(*n_subframes))
                  *8000/(frame_size_nb)+0.5);
          min_bit_rate=(long)((float)((num_bits_lpc_int
                       -PAN_BIT_LSP22_0-PAN_BIT_LSP22_1
                       -PAN_BIT_LSP22_2-PAN_BIT_LSP22_3
                       -PAN_BIT_LSP22_4
                       +NEC_BIT_MODE
                       +NEC_BIT_RMS)
                      +(NEC_BIT_ACB+mp_pos_bits+mp_sgn_bits
                        +NEC_BIT_GAIN)*(*n_subframes))
                  *8000/(frame_size_nb)+0.5);

              if ( CoreBitRate < min_bit_rate || CoreBitRate > max_bit_rate ) {
        fprintf(stderr,"Error: Illegal combination of BitRate %ld and InputConfiguration %ld.\n", bit_rate, InputConfiguration);
        fprintf(stderr,"\t Please set BitRate in the range from %ld to %ld\n",
            min_bit_rate+num_enhstages*NEC_ENH_BITRATE,
            max_bit_rate+num_enhstages*NEC_ENH_BITRATE);
        CommonExit(1,"celp error"); 
              }
        }
            
            if (BandwidthScalabilityMode==ON) {
                frame_size_bws = frame_size_nb * 2;
                n_subframes_bws = frame_size_bws/80;
                sbfrm_size_bws = frame_size_bws / n_subframes_bws;
                lpc_order_bws = NEC_LPC_ORDER_FRQ16;
                n_lpc_analysis_bws = frame_size_bws / NEC_LEN_LPC_ANALYSIS;
                
                target_mp_bits=(long)(((float)(frame_size_bws)*
                    (bit_rate-BWS_nb_bitrate)
                    /16000 - NEC_BIT_LSP1620)
                    /n_subframes_bws - (NEC_BIT_ACB_FRQ16
                    + NEC_BIT_GAIN_FRQ16));
                nec_mp_config(sbfrm_size_bws,target_mp_bits,
                    &bws_pos_bits,&bws_sgn_bits);
                
                calc_bit_rate=(long)((float)(NEC_BIT_LSP1620
                    +(NEC_BIT_ACB_FRQ16 + bws_pos_bits
                    +bws_sgn_bits + NEC_BIT_GAIN_FRQ16)
                    *n_subframes_bws)*16000/(frame_size_bws)+0.5);
                
                if ( (bit_rate-BWS_nb_bitrate) != calc_bit_rate ) {
                    fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n",bit_rate);
                    CommonExit(1,"celp error"); 
                }
                if ( bws_sgn_bits < 6 || (bws_sgn_bits%2) != 0 ) {
                    fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n",bit_rate);
                    CommonExit(1,"celp error");
                } else {
                    *BWS_Configuration = (bws_sgn_bits-6)/2;
                }
                
                *frame_size = frame_size_bws;
                num_lpc_indices_bws = NEC_NUM_LPC_INDICES_FRQ16 ;
                num_indices += NEC_NUM_LPC_INDICES_FRQ16 
                    + n_subframes_bws * (NEC_NUM_SHAPE_CBKS
                    +NEC_NUM_GAIN_CBKS);
            } else {
                *frame_size = frame_size_nb;
            }
            
            if((*window_sizes=(long *)calloc(*n_lpc_analysis, 
                sizeof(long)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                CommonExit(1,"celp error");
            }
            if((*window_offsets=(long *)calloc(*n_lpc_analysis, 
                sizeof(long)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(2);
            }
            
            if ((FineRateControl == ON) || (QuantizationMode == ScalarQuantizer)) {
                for(i=0;i<(*n_lpc_analysis);i++) {
                    *(*window_sizes+i) = NEC_PAN_NLB+NEC_LEN_LPC_ANALYSIS+NEC_PAN_NLA;
                    *(*window_offsets+i) = i*NEC_LEN_LPC_ANALYSIS + ((*frame_size) - NEC_PAN_NLB);
                }
            } else {
                for(i=0;i<(*n_lpc_analysis);i++) {
                    *(*window_sizes+i) = NEC_PAN_NLB+len_lpc_analysis+NEC_PAN_NLA;
                    *(*window_offsets+i) = i*len_lpc_analysis;
                }
            }
            
            if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
                sizeof(long)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(3);
            }
            
            ctr = 0;
            *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_0;
            *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_1;
            *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_2;
            *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_3;
            *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_4;
            *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_MODE;
            *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_RMS;
            for ( i = 0; i < *n_subframes; i++ ) {
                *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ACB;
                *(*org_frame_bit_allocation+(ctr++)) =  mp_pos_bits;
                *(*org_frame_bit_allocation+(ctr++)) =  mp_sgn_bits;
                *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN;
            }
            
            if ( *sbfrm_size == (NEC_FRAME20MS/NEC_NSF4) ) {
                enh_pos_bits = NEC_BIT_ENH_POS40_2;
                enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
            } else {
                enh_pos_bits = NEC_BIT_ENH_POS80_4;
                enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
            }
            for ( i = 0; i < num_enhstages; i++ ) {
                for ( j = 0; j < *n_subframes; j++ ) {
                    *(*org_frame_bit_allocation+(ctr++)) =  0;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
                }
            }
            
            if (BandwidthScalabilityMode==ON) {
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_0;
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_1;
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_2;
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_3; 
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_4; 
                *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_5;
                for ( i = 0; i < n_subframes_bws; i++ ) {
                    *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ACB_FRQ16;
                    *(*org_frame_bit_allocation+(ctr++)) =  bws_pos_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  bws_sgn_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN_FRQ16;
                }
            }
            
            if (FineRateControl == ON) {
                PHI_BUFFER = buffer_size = NEC_PAN_NLB+frame_size_nb+NEC_PAN_NLA + (frame_size_nb-NEC_PAN_NLB); 
                /* AG: one frame delay for interpolation */ 
            } else {
                PHI_BUFFER = buffer_size = NEC_PAN_NLB+frame_size_nb+NEC_PAN_NLA;
            }
            
            /* momory allocation to static parameters */
            if((PHI_sp_pp_buf=(float *)calloc(buffer_size, sizeof(float)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(4);
            }
            for(i=0;i<buffer_size;i++) *(PHI_sp_pp_buf+i) = 0.;
            
            if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
                sizeof(float)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(5);
            }
            
            for(i=0;i<(*lpc_order);i++) 
                *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);
            
            gamma_num=(float)PAN_GAM_MA;
            gamma_den=(float)PAN_GAM_AR;
            
            /* momory allocation to static parameters */
            if (BandwidthScalabilityMode==ON) {
                if((window_sizes_bws=(long *)calloc(n_lpc_analysis_bws,
                    sizeof(long)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                    CommonExit(1,"celp error");
                }
                if((window_offsets_bws=(long *)calloc(n_lpc_analysis_bws,
                    sizeof(long)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                    exit(2);
                }
                
                for(i=0;i<n_lpc_analysis_bws;i++) {
                    *(window_sizes_bws+i) = NEC_FRQ16_NLB+NEC_LEN_LPC_ANALYSIS
                        + NEC_FRQ16_NLA;
                    *(window_offsets_bws+i) = i*NEC_LEN_LPC_ANALYSIS;
                }
                
                buffer_size_bws = NEC_FRQ16_NLB+(frame_size_bws)
                    + NEC_FRQ16_NLA+NEC_LPF_DELAY;
                if((bws_sp_buffer=(float *)calloc(buffer_size_bws,
                    sizeof(float)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                    exit(4);
                }
                for(i=0;i<buffer_size_bws;i++) *(bws_sp_buffer+i) = 0.;
                
                if((buf_Qlsp_coefficients_bws=(float *)calloc(*lpc_order,
                    sizeof(float)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                    exit(5);
                }
                
                if((prev_Qlsp_coefficients_bws=(float *)calloc(lpc_order_bws,
                    sizeof(float)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                    exit(5);
                }
                for(i=0;i<(lpc_order_bws);i++) 
                    *(prev_Qlsp_coefficients_bws+i) = (float)PAN_PI * (i+1)
                    / (float)(lpc_order_bws+1);
            }
            
            /* submodules for initialization */
            if ((QuantizationMode == VectorQuantizer) && (FineRateControl == OFF)) 
            {
                PAN_InitLpcAnalysisEncoder(*lpc_order, InstCtxt->PHI_Priv);
                if (BandwidthScalabilityMode == ON) 
                {
                    NEC_InitLpcAnalysisEncoder(window_sizes_bws,n_lpc_analysis_bws,
                        lpc_order_bws, (float)NEC_GAMMA_BE);
                }
            } 
            else 
            {
                PHI_InitLpcAnalysisEncoder (*lpc_order, *lpc_order, 
                                            bit_rate,  sampling_frequency,
                                            *frame_size, *num_lpc_indices, *n_subframes, *num_shape_cbks, 
                                            *num_gain_cbks,  *org_frame_bit_allocation,
                                            SilenceCompressionSW,
                                            InstCtxt->PHI_Priv);
            }
            /* -----------------------------------------------------------------*/
            /* Initialise windows for narrowband LPC analysis                   */
            /* -----------------------------------------------------------------*/ 
            {                  
                int     x;
                float   *pin;
                float   *pout;
                
                long i;
                
                if((HammingWindow=(float **)calloc((unsigned int)*n_lpc_analysis, sizeof(float *)))==NULL) {
                    fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                    CommonExit(1,"celp error");
                }
                
                for(i=0;i<*n_lpc_analysis;i++) {
                    if((HammingWindow[i]=(float *)calloc((unsigned int)(((*window_sizes)[i])), sizeof(float)))==NULL) {
                        fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                        CommonExit(1,"celp error");
                    }
                    
                    /* -------------------------------------------------------------*/
                    /* Generate Hamming Window For Lpc                              */
                    /* -------------------------------------------------------------*/ 
                    /* using asymmetric windows */
                    
                    pout = HammingWindow[i];    
                    for(x=0;x<NEC_PAN_NLB+len_lpc_analysis/2;x++) {
                        *pout++ = (float)(.54-.46*cos(NEC_PI*2.*((double)x)
                            /(double)((NEC_PAN_NLB+len_lpc_analysis/2)*2-1)));
                    }
                    for(x=0;x<NEC_PAN_NLA+len_lpc_analysis/2;x++) {
                        *pout++ = (float)(cos(NEC_PI*2.*((double)x)
                            /(double)((NEC_PAN_NLA+len_lpc_analysis/2)*4-1)));
                    }
                }
                
                /* -------------------------------------------------------------*/
                /* Generate Gamma Array for Bandwidth Expansion                 */
                /* -------------------------------------------------------------*/
                if(
                    (( PAN_GammaArrayBE = (float *)malloc((unsigned int)(*lpc_order) * sizeof(float))) == NULL )
                    ) {
                    fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                    CommonExit(1,"celp error");
                }
                pin     = PAN_GammaArrayBE;
                pout    = PAN_GammaArrayBE;
                *pout++ = (float)PAN_GAMMA_BE;
                for(x=1; x < (int)*lpc_order; x++) {
                    *pout++ = (float)PAN_GAMMA_BE * *pin++;
                }
            }   
            
            if (LosslessCodingMode == ON) {
                *num_lpc_indices     = N_INDICES_SQ8LL;   
            }
      } /* end of NB-MPE */
      
      if (SampleRateMode == fs16kHz) {
          int i, j;
          long    target_mp_bits, mp_pos_bits, mp_sgn_bits;
          long    enh_pos_bits, enh_sgn_bits;
          long    ctr, calc_bit_rate;
          long    CoreBitRate;
          long    num_bits_lpc_int;
          long    min_bit_rate, max_bit_rate;
          
          num_enhstages = NumEnhLayers;
          CoreBitRate = bit_rate - NEC_ENH_BITRATE_FRQ16 * num_enhstages;
          if ( SilenceCompressionSW == ON ){
              if ( InputConfiguration >= 16 && InputConfiguration < 32 ){
                  CoreBitRate -= 200;
              } 
              
              if ( InputConfiguration >= 0 && InputConfiguration < 16 ){
                  CoreBitRate -= 100;
              }
          }
          
          *lpc_order = NEC_LPC_ORDER_FRQ16;
          *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
          *num_gain_cbks = NEC_NUM_GAIN_CBKS;
          *num_lpc_indices = PAN_NUM_LPC_INDICES_W;
          *n_lag_candidates = PAN_N_PCAN_INT;
          *min_pitch_frequency = sampling_frequency/(float)NEC_PITCH_MAX_FRQ16;
          *max_pitch_frequency = sampling_frequency/(float)NEC_PITCH_MIN_FRQ16;
          
          num_bits_lpc_int = PAN_NUM_LSP_BITS_W;
          
          if ( FineRateControl == OFF ) {
              switch ( CoreBitRate ) {
              case 10900:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 11500:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 12100:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 12700:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 13300:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 13900:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 14300:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF4;  break;
                  
              case 14700:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 15900:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 17100:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 17900:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 18700:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 19500:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 20300:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
              case 21100:
                  *frame_size = NEC_FRAME20MS_FRQ16; *n_subframes = NEC_NSF8;  break;
                  
              case 13600:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 14200:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 14800:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 15400:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 16000:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 16600:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
              case 17000:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF2;  break;
                  
              case 17400:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 18600:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 19800:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 20600:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 21400:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 22200:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 23000:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
              case 23800:
                  *frame_size = NEC_FRAME10MS_FRQ16; *n_subframes = NEC_NSF4;  break;
                  
              default:
                  fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n",
                      CoreBitRate);
                  CommonExit(1,"celp error"); 
              }
          } else {
              if ((InputConfiguration>=0) && (InputConfiguration<7)) {
                  *frame_size = NEC_FRAME20MS_FRQ16;
                  *n_subframes = NEC_NSF4;
              } else if((InputConfiguration>=8)&&(InputConfiguration<16)) {
                  *frame_size = NEC_FRAME20MS_FRQ16;
                  *n_subframes = NEC_NSF8;
              } else if((InputConfiguration>=16)&&(InputConfiguration<23)) {
                  *frame_size = NEC_FRAME10MS_FRQ16;
                  *n_subframes = NEC_NSF2;
              } else if((InputConfiguration>=24)&&(InputConfiguration<32)) {
                  *frame_size = NEC_FRAME10MS_FRQ16;
                  *n_subframes = NEC_NSF4;
              } else {
                  fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                  CommonExit(1,"celp error"); 
              }
              switch ( InputConfiguration ) 
              {
                  case 0:
                      mp_pos_bits = 20; mp_sgn_bits =  5; break;
                  case 1:
                      mp_pos_bits = 22; mp_sgn_bits =  6; break;
                  case 2:
                      mp_pos_bits = 24; mp_sgn_bits =  7; break;
                  case 3:
                      mp_pos_bits = 26; mp_sgn_bits =  8; break;
                  case 4:
                      mp_pos_bits = 28; mp_sgn_bits =  9; break;
                  case 5:
                      mp_pos_bits = 30; mp_sgn_bits =  10; break;
                  case 6:
                      mp_pos_bits = 31; mp_sgn_bits =  11; break;
                  case 7:
                      break;
                  case 8:
                      mp_pos_bits = 11; mp_sgn_bits =  3; break;
                  case 9:
                      mp_pos_bits = 13; mp_sgn_bits =  4; break;
                  case 10:
                      mp_pos_bits = 15; mp_sgn_bits =  5; break;
                  case 11:
                      mp_pos_bits = 16; mp_sgn_bits =  6; break;
                  case 12:
                      mp_pos_bits = 17; mp_sgn_bits =  7; break;
                  case 13:
                      mp_pos_bits = 18; mp_sgn_bits =  8; break;
                  case 14:
                      mp_pos_bits = 19; mp_sgn_bits =  9; break;
                  case 15:
                      mp_pos_bits = 20; mp_sgn_bits =  10; break;
                  case 16:
                      mp_pos_bits = 20; mp_sgn_bits =  5; break;
                  case 17:
                      mp_pos_bits = 22; mp_sgn_bits =  6; break;
                  case 18:
                      mp_pos_bits = 24; mp_sgn_bits =  7; break;
                  case 19:
                      mp_pos_bits = 26; mp_sgn_bits =  8; break;
                  case 20:
                      mp_pos_bits = 28; mp_sgn_bits =  9; break;
                  case 21:
                      mp_pos_bits = 30; mp_sgn_bits =  10; break;
                  case 22:
                      mp_pos_bits = 31; mp_sgn_bits =  11; break;
                  case 23:
                      break;
                  case 24:
                      mp_pos_bits = 11; mp_sgn_bits =  3; break;
                  case 25:
                      mp_pos_bits = 13; mp_sgn_bits =  4; break;
                  case 26:
                      mp_pos_bits = 15; mp_sgn_bits =  5; break;
                  case 27:
                      mp_pos_bits = 16; mp_sgn_bits =  6; break;
                  case 28:
                      mp_pos_bits = 17; mp_sgn_bits =  7; break;
                  case 29:
                      mp_pos_bits = 18; mp_sgn_bits =  8; break;
                  case 30:
                      mp_pos_bits = 19; mp_sgn_bits =  9; break;
                  case 31:
                      mp_pos_bits = 20; mp_sgn_bits =  10; break;
              }
          }
          
          *sbfrm_size = (*frame_size)/(*n_subframes);
          *n_lpc_analysis = (*frame_size)/NEC_LEN_LPC_ANALYSIS;
          
          num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES_W
              + (num_enhstages + 1) * (*n_subframes) *
              (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);
          
          if ( FineRateControl == OFF ) {
              target_mp_bits=(long)(((float)(*frame_size)*CoreBitRate/16000
                  - (num_bits_lpc_int+NEC_BIT_MODE
                  +NEC_BIT_RMS))
                  /(*n_subframes)
                  - (NEC_ACB_BIT_WB + NEC_BIT_GAIN_WB));  
              
              nec_mp_config((*sbfrm_size),target_mp_bits,
                  &mp_pos_bits,&mp_sgn_bits);
              
              calc_bit_rate=(long)(((num_bits_lpc_int+NEC_BIT_MODE
                  +NEC_BIT_RMS)
                  +(NEC_ACB_BIT_WB+mp_pos_bits+mp_sgn_bits 
                  +NEC_BIT_GAIN_WB)*(*n_subframes))
                  *16000/(*frame_size));
              if ( CoreBitRate != calc_bit_rate ) {
                  fprintf(stderr,"Error: Specified BitRate %ld is not supported.\n",
                      bit_rate);
                  fprintf(stderr,"\t Please set BitRate at %ld\n",
                      calc_bit_rate+num_enhstages*NEC_ENH_BITRATE_FRQ16);
                  CommonExit(1,"celp error"); 
              }
              
              if ( *frame_size == NEC_FRAME20MS_FRQ16 ) {
                  if ( CoreBitRate >= 10900 && CoreBitRate <= 14300 ) {
                      *MPE_Configuration = mp_sgn_bits-5;
                  } 
                  if ( CoreBitRate >= 14700 && CoreBitRate <= 21100 ) {
                      *MPE_Configuration = (mp_sgn_bits-3)+8;
                  }
              } else {
                  if ( CoreBitRate >= 13600 && CoreBitRate <= 17000 ) {
                      *MPE_Configuration = (mp_sgn_bits-5)+16;
                  } 
                  if ( CoreBitRate >= 17400 && CoreBitRate <= 23800 ) {
                      *MPE_Configuration = (mp_sgn_bits-3)+24;
                  }
              }
          } else {
              max_bit_rate=(long)((float)((2+num_bits_lpc_int+NEC_BIT_MODE
                  +NEC_BIT_RMS)
                  +(NEC_ACB_BIT_WB+mp_pos_bits+mp_sgn_bits 
                  +NEC_BIT_GAIN_WB)*(*n_subframes))
                  *16000/(*frame_size)+0.5);
              min_bit_rate=(long)((float)((2+NEC_BIT_MODE+NEC_BIT_RMS)
                  +(NEC_ACB_BIT_WB+mp_pos_bits+mp_sgn_bits 
                  +NEC_BIT_GAIN_WB)*(*n_subframes))
                  *16000/(*frame_size)+0.5);
              min_bit_rate = (long)(0.5*(min_bit_rate+max_bit_rate)+0.5);
              
              /*   RF 991124   */
              if (SilenceCompressionSW == ON)
              {    
                  if ((InputConfiguration >= 0) && (InputConfiguration < 16))
                  {
                      min_bit_rate += 100;
                      max_bit_rate += 100;
                      CoreBitRate  += 100;
                  }

                  if ((InputConfiguration >= 16) && (InputConfiguration < 32))
                  {
                      min_bit_rate += 200;
                      max_bit_rate += 200;
                      CoreBitRate  += 200;
                  } 
              }


              if ( CoreBitRate < min_bit_rate || CoreBitRate > max_bit_rate ) {
                  fprintf(stderr,"Error: Illegal combination of BitRate %ld and InputConfiguration %ld.\n", bit_rate, InputConfiguration);
                  fprintf(stderr,"\t Please set BitRate in the range from %ld to %ld\n",
                      min_bit_rate+num_enhstages*NEC_ENH_BITRATE_FRQ16,
                      max_bit_rate+num_enhstages*NEC_ENH_BITRATE_FRQ16);
                  CommonExit(1,"celp error"); 
              }
              *MPE_Configuration = InputConfiguration;
          }
          
          if((*window_sizes=(long *)calloc(*n_lpc_analysis, 
              sizeof(long)))==NULL) {
              fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
              CommonExit(1,"celp error");
          }
          
          if((*window_offsets=(long *)calloc(*n_lpc_analysis, 
              sizeof(long)))==NULL) {
              fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
              exit(2);
          }
          
          if ( FineRateControl == OFF ) {
              for(i=0;i<(*n_lpc_analysis);i++) {
                  *(*window_sizes+i) = NEC_FRQ16_NLB+NEC_LEN_LPC_ANALYSIS+NEC_FRQ16_NLA;
                  *(*window_offsets+i) = i*NEC_LEN_LPC_ANALYSIS;
              }
          } else {
              for(i=0;i<(*n_lpc_analysis);i++) {
                  *(*window_sizes+i) = NEC_FRQ16_NLB+NEC_LEN_LPC_ANALYSIS+NEC_FRQ16_NLA;
                  *(*window_offsets+i) = i*NEC_LEN_LPC_ANALYSIS+((*frame_size)-NEC_FRQ16_NLB);
              }
          }
          
          if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
              sizeof(long)))==NULL) {
              fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
              exit(3);
          }
          ctr = 0;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_0;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_1;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_2;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_3;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_4;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_0;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_1;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_2;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_3;
          *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_4;
          *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_MODE;
          *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_RMS;
          for ( i = 0; i < *n_subframes; i++ ) {
              *(*org_frame_bit_allocation+(ctr++)) =  NEC_ACB_BIT_WB;
              *(*org_frame_bit_allocation+(ctr++)) =  mp_pos_bits;
              *(*org_frame_bit_allocation+(ctr++)) =  mp_sgn_bits;
              *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN_WB;
          }
          
          if ( *sbfrm_size == (NEC_SBFRM_SIZE40) ) {
              enh_pos_bits = NEC_BIT_ENH_POS40_2;
              enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
          } else {
              enh_pos_bits = NEC_BIT_ENH_POS80_4;
              enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
          }
          for ( i = 0; i < num_enhstages; i++ ) {
              for ( j = 0; j < *n_subframes; j++ ) {
                  *(*org_frame_bit_allocation+(ctr++)) =  0;
                  *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
                  *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
                  *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
              }
          }
          
          if ( FineRateControl == OFF ) {
              PHI_BUFFER = buffer_size = NEC_FRQ16_NLB+(*frame_size)+NEC_FRQ16_NLA;
          } else {
              PHI_BUFFER = buffer_size = NEC_FRQ16_NLB+(*frame_size)+NEC_FRQ16_NLA+((*frame_size)-NEC_FRQ16_NLB);
          }
          
          /* momory allocation to static parameters */
          if((PHI_sp_pp_buf=(float *)calloc(buffer_size, sizeof(float)))==NULL) {
              fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
              exit(4);
          }
          for(i=0;i<buffer_size;i++) *(PHI_sp_pp_buf+i) = 0.;
          
          if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
              sizeof(float)))==NULL) {
              fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
              exit(5);
          }
          
          for(i=0;i<(*lpc_order);i++) 
              *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);
          
          gamma_num=(float)PAN_GAM_MA;
          gamma_den=(float)PAN_GAM_AR;
          
          /* submodules for initialization */
          if ( FineRateControl == OFF ) 
          {
              PAN_InitLpcAnalysisEncoder (*lpc_order,InstCtxt->PHI_Priv);
          }
          else 
          {
              PHI_InitLpcAnalysisEncoder (*lpc_order, *lpc_order,
                                          bit_rate, sampling_frequency,
                                          *frame_size, *num_lpc_indices, *n_subframes, *num_shape_cbks,
                                          *num_gain_cbks, *org_frame_bit_allocation,
                                          SilenceCompressionSW,
                                          InstCtxt->PHI_Priv);
          }
          
          /* -----------------------------------------------------------------*/
          /* Initialise windows for narrowband LPC analysis                   */
          /* -----------------------------------------------------------------*/ 
          {                  
              int     x;
              float   *pin;
              float   *pout;
              
              long i;
              
              if((HammingWindow=(float **)calloc((unsigned int)*n_lpc_analysis, sizeof(float *)))==NULL) {
                  fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                  CommonExit(1,"celp error");
              }
              
              for(i=0;i<*n_lpc_analysis;i++){
                  if((HammingWindow[i]=(float *)calloc((unsigned int)(((*window_sizes)[i])), sizeof(float)))==NULL) {
                      fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                      CommonExit(1,"celp error");
                  }
                  
                  /* -------------------------------------------------------------*/
                  /* Generate Hamming Window For Lpc                              */
                  /* -------------------------------------------------------------*/ 
                  /* using asymmetric windows */
                  
                  pout = HammingWindow[i];    
                  for(x=0;x<NEC_FRQ16_NLB+NEC_LEN_LPC_ANALYSIS/2;x++) {
                      *pout++ = (float)(.54-.46*cos(NEC_PI*2.*((double)x)
                          /(double)((NEC_FRQ16_NLB+NEC_LEN_LPC_ANALYSIS/2)*2-1)));
                  }
                  for(x=0;x<NEC_FRQ16_NLA+NEC_LEN_LPC_ANALYSIS/2;x++) {
                      *pout++ = (float)(cos(NEC_PI*2.*((double)x)
                          /(double)((NEC_FRQ16_NLA+NEC_LEN_LPC_ANALYSIS/2)*4-1)));
                  }
              }
              
              /* -------------------------------------------------------------*/
              /* Generate Gamma Array for Bandwidth Expansion                 */
              /* -------------------------------------------------------------*/
              if((( PAN_GammaArrayBE = (float *)malloc((unsigned int)(*lpc_order) * sizeof(float))) == NULL ))
              {
                  fprintf(stderr,"MALLOC FAILURE in Routine celp_initialisation_encoder \n");
                  CommonExit(1,"celp error");
              }
              pin     = PAN_GammaArrayBE;
              pout    = PAN_GammaArrayBE;
              *pout++ = (float)PAN_GAMMA_BE;
              for (x = 1; x < (int)*lpc_order; x++)
              {
                  *pout++ = (float)PAN_GAMMA_BE * *pin++;
              }
          }   
      } /* end of WB-MPE */
    } /* end of MultiPulseExcitation */
    
    /* -----------------------------------------------------------------*/
    /* Write bitstream header                                           */
    /* -----------------------------------------------------------------*/
    if (!sysFlag) 
    {
        write_celp_bitstream_header (hdrStream,
                                     ExcitationMode, 
                                     SampleRateMode,
                                     FineRateControl, 
                                     *RPE_configuration, 
                                     *MPE_Configuration, 
                                     NumEnhLayers, 
                                     BandwidthScalabilityMode, 
                                     SilenceCompressionSW,
                                     enc_objectVerType,
                                     *BWS_Configuration);
    }
    
} 

/*======================================================================*/
/*   Function Definition: celp_close_encoder                            */
/*======================================================================*/

void celp_close_encoder (long ExcitationMode,
                         long BandwidthScalabilityMode,
                         long sbfrm_size,               /* In: subframe size                  */
                         long frame_bit_allocation[],   /* In: bit num. for each index        */
                         long window_offsets[],         /* In: window offset for each LP ana  */
                         long window_sizes[],           /* In: window size for each LP ana    */
                         long n_lpc_analysis,           /* In: number of LP analysis/frame    */
                         void **InstanceContext         /* In/Out: handle to instance context */
)
{
    long i;
    
    INST_CONTEXT_LPC_ENC_TYPE *InstCtxt;
    PHI_PRIV_TYPE *PHI_Priv;
    
    /* -----------------------------------------------------------------*/
    /* Set up pointers to private data                                  */
    /* -----------------------------------------------------------------*/
    PHI_Priv = ((INST_CONTEXT_LPC_ENC_TYPE *) *InstanceContext)->PHI_Priv;
    
    /* -----------------------------------------------------------------*/
    /*                                                                  */
    /* -----------------------------------------------------------------*/
    
    if (ExcitationMode == RegularPulseExc)
    {
        PHI_FreeLpcAnalysisEncoder(PHI_Priv);
        PHI_close_excitation_analysis();
        PHI_free_energy_table(sbfrm_size, ACBK_SIZE);
        PHI_free_bit_allocation(frame_bit_allocation);
        
        free(window_offsets);
        free(window_sizes);
        
        free(PHI_sp_pp_buf);
        free (VAD_pcm_buffer);
        free(Downsampled_signal);
        
        for(i=0;i<n_lpc_analysis;i++) 
        {
            free(SquaredHammingWindow[i]);
            free(HammingWindow[i]);
        }
        free(SquaredHammingWindow);
        free(HammingWindow);
        free(prev_Qlsp_coefficients);
        
        free(PHI_GammaArrayBE);
        free(PAN_GammaArrayBE);
        /* -----------------------------------------------------------------*/
        /* Print Total Frames Processed                                     */
        /* -----------------------------------------------------------------*/
        frame_ctr -= 2; 
        if (CELPencDebugLevel)
        {   /* HP 971120 */
            fprintf(stderr,"\n");
            fprintf(stderr,"Total Frames:  %ld \n", frame_ctr);
            fprintf(stderr,"LPC Frames:    %ld \n", lpc_sent_frames);
            fprintf(stderr,"LPC sent:      %5.2f %%\n", (float)lpc_sent_frames/(float)frame_ctr * (float)100.0);
        }
    }
    
    if (ExcitationMode == MultiPulseExc)
    {
        if (CELPencDebugLevel)
        {   /* HP 971120 */
            fprintf(stderr,"Total Frames:  %ld \n", frame_ctr);
            fprintf(stderr,"LPC Frames:    %ld \n", lpc_sent_frames);
            fprintf(stderr,"LPC sent:      %5.2f %%\n", (float)lpc_sent_frames/(float)frame_ctr * (float)100.0);
        }
        PAN_FreeLpcAnalysisEncoder(PHI_Priv);
        
        free(window_offsets);
        free(window_sizes);
        
        for(i=0;i<n_lpc_analysis;i++) 
        {
            free(HammingWindow[i]);
        }
        free(HammingWindow);
        free(PAN_GammaArrayBE);
        free(frame_bit_allocation);
        free(PHI_sp_pp_buf);
        free(prev_Qlsp_coefficients);

        if (BandwidthScalabilityMode == ON)
        {
            free(window_offsets_bws);
            free(window_sizes_bws);
            free(bws_sp_buffer);
            free(buf_Qlsp_coefficients_bws);
            free(prev_Qlsp_coefficients_bws);
            NEC_FreeLpcAnalysisEncoder(n_lpc_analysis_bws);
        }
    }
    
    /* -----------------------------------------------------------------*/
    /* Dispose of private storage for instance context                  */
    /* -----------------------------------------------------------------*/
    InstCtxt = (INST_CONTEXT_LPC_ENC_TYPE *)*InstanceContext;
    free(InstCtxt->PHI_Priv);
    free(InstCtxt);
    *InstanceContext = NULL;
    
 }

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 27-08-96 R. Taori  Initial Version                                   */
/* 18-09-96 R. Taori  Brought in line with MPEG-4 Interface             */
/* 26-09-96 R. Taori  & A. Gerrits                                      */
/* 09-09-97 A. Gerrits  Introduction of NEC VQ                          */
/* 21-05-99 T. Mlasko               Version 2 stuff added               */

