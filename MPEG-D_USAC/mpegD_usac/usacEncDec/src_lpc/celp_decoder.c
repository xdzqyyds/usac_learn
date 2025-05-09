/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Rakesh Taori (Philips Research Laboratories, Eindhoven, The Netherlands) 
Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) 

and edited by
Torsten Mlasko		(Robert Bosch GmbH, Germany),
Markus Werner       (SEED / Software Development Karlsruhe) 

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 2000.

$Id: celp_decoder.c,v 1.1.1.1 2009-05-29 14:10:11 mul Exp $
CELP decoder
**********************************************************************/
 
/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common_m4a.h"

#include "bitstream.h"     /* bit stream module                         */
#include "phi_cons.h"           /* Philips Defined Constants            */
#include "phi_priv.h"       /* for PHI private data storage */
#include "bitstream.h"
#include "lpc_common.h"         /* Common LPC core Defined Constants    */
#include "phi_gxit.h"           /* Excitation Generation Prototypes     */
#include "phi_lpc.h"            /* Prototypes of LPC Subroutines        */
#include "phi_post.h"           /* Post Processing Module Prototype     */
#include "phi_fbit.h"           /* Frame bit allocation table           */
#include "nec_abs_proto.h"      /* Prototypes for NEC Routines          */
#include "pan_celp_const.h"     /* Constants for PAN Routines           */
#include "celp_proto_dec.h"     /* Prototypes for CELP Routines         */
#include "celp_decoder.h"       /* Prototypes for CELP Decoder Routines */
#include "celp_bitstream_demux.h"/* Prototypes for CELP Decoder Routines*/
#include "nec_abs_const.h"      /* Constants for NEC Routines           */

#ifdef VERSION2_EC
    #include "err_concealment.h"    /* Error Concealment Routines       */
    #include "fe_sub.h"     /* Frame Error Concealment related Routine */
#endif
#include "nec_vad.h"          /* Prototypes for Silence Compr. Routines */
unsigned long TX_flag_dec=1;

int CELPdecDebugLevel = 0;  /* HP 971120 */

/* ---------------------------------------------------------------------*/ 
/* Frame counter                                                        */
/* ---------------------------------------------------------------------*/
static unsigned long frame_ctr = 0;   /* Frame Counter                   */

static long postfilter = 0;           /* Postfilter switch               */
static float *prev_Qlsp_coefficients; /* previous quantized LSP coeff.   */

static long num_enhstages;
static long dec_enhstages;
static long dec_bwsmode;
static long num_indices;

/*======================================================================*/
/*    NarrowBand Data declaration                                       */
/*======================================================================*/
static float *buf_Qlsp_coefficients_bws; /* current quantized LSP coeff. */
static float *prev_Qlsp_coefficients_bws;/* previous quantized LSP coeff. */

static long frame_size_nb, frame_size_bws;
static long n_subframes_bws;
static long sbfrm_size_bws;
static long lpc_order_bws;
static long num_lpc_indices_bws;
static long mp_pos_bits, mp_sgn_bits;
static long enh_pos_bits, enh_sgn_bits;
static long bws_pos_bits, bws_sgn_bits;
static long *ofa;

/*======================================================================*/
/* Type definitions                                                     */
/*======================================================================*/

typedef struct
{
  PHI_PRIV_TYPE *PHI_Priv;
  /* add private data pointers here for other coding varieties */
} INST_CONTEXT_LPC_DEC_TYPE;


/*======================================================================*/
/* Function definition: abs_coder                                       */
/*======================================================================*/
void celp_decoder (HANDLE_BSBITSTREAM bitStreamLay[],             /* In: Bitstream                     */
                   float       **OutputSignal,              /* Out: Multichannel Output          */
                   long        ExcitationMode,              /* In: Excitation Mode               */
                   long        SampleRateMode,              /* In: SampleRate Mode               */
                   long        QuantizationMode,            /* In: Type of Quantization          */
                   long        FineRateControl,             /* In: Fine Rate Control switch      */
                   long        RPE_configuration,           /* In: RPE_configuration             */
                   long        Wideband_VQ,                 /* In: Wideband VQ mode              */
                   long        BandwidthScalabilityMode,    /* In: bandwidth switch              */
                   long        MPE_Configuration,           /* In: Narrowband configuration      */
                   long        SilenceCompressionSW,        /* In: SilenceCompressionSW          */
                   int         dec_objectVerType,
        				   unsigned long epConfig,
                   long        frame_size,                  /* In:  Frame size                   */
                   long        n_subframes,                 /* In:  Number of subframes          */
                   long        sbfrm_size,                  /* In:  Subframe size                */
                   long        lpc_order,                   /* In:  Order of LPC                 */
                   long        num_lpc_indices,             /* In:  Number of LPC indices        */
                   long        num_shape_cbks,              /* In:  Number of Shape Codebooks    */
                   long        num_gain_cbks,               /* In:  Number of Gain Codebooks     */
                   long        *org_frame_bit_allocation,   /* In: bit num. for each index       */
                   void        *InstanceContext		        /* In: pointer to instance context   */
                   
)
{
    /*==================================================================*/
    /*      L O C A L   D A T A   D E F I N I T I O N S                 */
    /*==================================================================*/
    float           *int_ap;                           /*Interpolated ap */
    unsigned long   *shape_indices = 0;                /* Shape indices  */
    unsigned long   *gain_indices = 0;                 /* Gain indices   */
    unsigned long   *indices = 0;                      /* LPC codes*/
    long            sbfrm_ctr = 0;

    float           *int_ap_bws = 0;                   /*Interpolated ap */
    unsigned long   *shape_indices_bws = 0;            /* Shape indices  */
    unsigned long   *gain_indices_bws = 0;             /* Gain indices   */
    unsigned long   *indices_bws = 0;                  /* LPC codes      */
    long            *bws_nb_acb_index = 0;             /* ACB codes      */

    unsigned long   interpolation_flag = 0;            /* Interpolation Flag*/
    unsigned long   LPC_Present = 1;
    unsigned long   signal_mode = 0;
    unsigned long   rms_index = 0;
    
    PHI_PRIV_TYPE   *PHI_Priv;    
    int             k;
    int             stream_ctr;
    HANDLE_BSBITSTREAM bitStream;

    unsigned long   BS_TX_flag;
#ifdef VERSION2_EC  /* SC: 0123, EC: , MPE/RPE */
    static short    seed = 21845;
    long            crc = crc_dmy(); /* CRC check */
#endif   /*  VERSION2_EC   */

    /* -----------------------------------------------------------------*/
    /* Set up pointers to private data                                  */
    /* -----------------------------------------------------------------*/
    PHI_Priv = ((INST_CONTEXT_LPC_DEC_TYPE *)InstanceContext)->PHI_Priv;
    
    /* -----------------------------------------------------------------*/
    /* Create Arrays for frame processing                               */
    /* -----------------------------------------------------------------*/
    if (
        (( int_ap = (float *)malloc((unsigned int)(n_subframes * lpc_order) * sizeof(float))) == NULL )||
        (( shape_indices = (unsigned long *)malloc((unsigned int)((num_enhstages+1)*num_shape_cbks * n_subframes) * sizeof(unsigned long))) == NULL )||
        (( gain_indices = (unsigned long *)malloc((unsigned int)((num_enhstages+1)*num_gain_cbks * n_subframes) * sizeof(unsigned long))) == NULL )||
        (( indices = (unsigned long *)malloc((unsigned int)num_lpc_indices * sizeof(unsigned long))) == NULL )
        )
    {
        fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
        CommonExit(1,"celp error");
    }
    
    for (k = 0; k < num_lpc_indices; k++)
    {
        indices [k] = 0;
    }
    
    for (k = 0; k < ((num_enhstages+1)*num_shape_cbks * n_subframes); k++)
    {
        shape_indices [k] = 0;
    }
    
    for (k = 0; k < ((num_enhstages+1)*num_gain_cbks * n_subframes); k++)
    {
        gain_indices [k] = 0;
    }
    
    if (BandwidthScalabilityMode == ON) 
    {
        if (
            (( int_ap_bws = (float *)malloc((unsigned int)(n_subframes_bws * lpc_order_bws) * sizeof(float))) == NULL )||
            (( shape_indices_bws = (unsigned long *)malloc((unsigned int)(num_shape_cbks * n_subframes_bws) * sizeof(unsigned long))) == NULL )||
            (( gain_indices_bws = (unsigned long *)malloc((unsigned int)(num_gain_cbks * n_subframes_bws) * sizeof(unsigned long))) == NULL )||
            (( indices_bws = (unsigned long *)malloc((unsigned int)num_lpc_indices_bws * sizeof(unsigned long))) == NULL ) ||
            (( bws_nb_acb_index = (long *)malloc((unsigned int)n_subframes_bws * sizeof(long))) == NULL )
            )
        {
            fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
            CommonExit(1,"celp error");
;
        }
    }

    stream_ctr = 0;
    bitStream = bitStreamLay[stream_ctr];
    

    if (dec_objectVerType == CelpObjectV2)
    {
        if (SilenceCompressionSW == ON) 
        {
            BsGetBit(bitStream, &BS_TX_flag, 2);
        }
        else 
        {
            BS_TX_flag = 1;
        }
        
        if (CELPdecDebugLevel) 
        {
            printf("Frame: %ld SW: %ld, FILE: %ld, BS: %ld\n", frame_ctr, SilenceCompressionSW, TX_flag_dec, BS_TX_flag);
        }
        
        TX_flag_dec = BS_TX_flag;
        
#ifdef VERSION2_EC  /* SC: 0123, EC: , MPE/RPE */
        /* change EC state */
        if (crc != 0)
        {
            /* check new mode and previous mode */
            if (TX_flag_dec == 1 || errConGetScMode() == 1)
            {
                errConSaveScMode (TX_flag_dec);
                changeErrState (crc);
            } 
            else
            {
                /* EC use 0 TX_flag after all BsGetBit() call */
                errConSaveScMode ((unsigned long) 0);
                
                /* EC state not change*/
            }
        } 
        else
        {
            errConSaveScMode (TX_flag_dec);
            changeErrState (crc);
        }
#endif   /*  VERSION2_EC  */

    }

    
    
    if (dec_objectVerType == CelpObjectV1)
    {
        /*==================================================================*/
        /* CELP Lpc demux                                                   */
        /*==================================================================*/
        
        if (FineRateControl == ON)
        {
            /* ---------------------------------------------------------*/
            /* Step I: Read interpolation_flag and LPC_present flag     */
            /* ---------------------------------------------------------*/
            BsGetBit(bitStream, &interpolation_flag, 1);
            BsGetBit(bitStream, &LPC_Present, 1);
            
            /* ---------------------------------------------------------*/
            /* Step II: If LPC is present                               */
            /* ---------------------------------------------------------*/
            if (LPC_Present == YES)
            {
                if (SampleRateMode == fs8kHz)
                {
                    Read_NarrowBand_LSP(bitStream, indices);
                } 
                else
                {
                    Read_Wideband_LSP(bitStream, indices);
                }
            }
        }
        else
        {
            if (SampleRateMode == fs8kHz)
            {
                Read_NarrowBand_LSP(bitStream, indices);
            } 
            else
            {
                Read_Wideband_LSP(bitStream, indices);
            }
        }
        
        /*==================================================================*/
        /* CELP Excitation decoding                                         */
        /*==================================================================*/
        if ( ExcitationMode == RegularPulseExc )
        {  /* RPE_Frame() */
            long subframe;
            
            /*--------------------------------------------------------------*/
            /* Regular Pulse Excitation                                     */ 
            /*--------------------------------------------------------------*/
            for(subframe = 0; subframe < n_subframes; subframe++)
            {
                /* ---------------------------------------------------------*/
                /* Read the Adaptive Codebook Lag                           */
                /* ---------------------------------------------------------*/
                BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks], 8);
                
                /* ---------------------------------------------------------*/
                /*Read the Fixed Codebook Index (function of bit-rate)      */
                /* ---------------------------------------------------------*/
                
                switch (RPE_configuration)
                {
                    case     0   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
                                     break;
                    case     1   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
                                     break;
                    case     2   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
                                     break;
                    case     3   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
                                     break;
                }
                
                /* ---------------------------------------------------------*/
                /*Read the Adaptive Codebook Gain                           */
                /* ---------------------------------------------------------*/
                BsGetBit (bitStream, &gain_indices[subframe * num_gain_cbks], 6);
                
                /* ---------------------------------------------------------*/
                /*Read the Fixed Codebook Gain (function of subframe)       */
                /*Later subframes are encoded differentially w.r.t previous */
                /* ---------------------------------------------------------*/
                if (subframe == 0)
                {
                    BsGetBit(bitStream, &gain_indices[subframe * num_gain_cbks + 1], 5);
                }
                else
                {
                    BsGetBit(bitStream, &gain_indices[subframe * num_gain_cbks + 1], 3);
                }
            }
        }
        
        
        if (ExcitationMode == MultiPulseExc) 
        {
            /* MPE_frame() */
            /*--------------------------------------------------------------*/
            /* Multi-Pulse Excitation                                       */ 
            /*--------------------------------------------------------------*/
            long i;
            
            BsGetBit(bitStream, &signal_mode, NEC_BIT_MODE);
            BsGetBit(bitStream, &rms_index, NEC_BIT_RMS);
            if (SampleRateMode == fs8kHz) 
            {
                for ( i = 0; i < n_subframes; i++ ) 
                {
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+0], NEC_BIT_ACB);
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    BsGetBit(bitStream, &gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN);
                }
            }
            else 
            {
                for ( i = 0; i < n_subframes; i++ ) 
                {
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+0], NEC_ACB_BIT_WB);
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                    BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                    BsGetBit(bitStream, &gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN_WB);
                }
            }
        }
    }    /*  CelpObjectV1  */
    

    
    if (dec_objectVerType == CelpObjectV2)
    {

        if (TX_flag_dec == 1)
        {

            /**********************************************************************************/
            /* VERSION 2:  Error Resilient re-ordering !                                      */
            /**********************************************************************************/
            int i;
            unsigned long dummy;
            long subframe; 
            
            if (ExcitationMode == RegularPulseExc)
            {
                /**********************************************************************************/
                /* Read Class 0 Data                                                              */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (FineRateControl == ON)
                {
                    BsGetBit(bitStream, &interpolation_flag, 1);
                    BsGetBit(bitStream, &LPC_Present, 1);
                }
                
                if (LPC_Present == YES)
                {
                    Read_Wideband_LSP_V2(bitStream, indices, 0);
                }
                
                /********************* Excitation Data *********************/
                
                for (subframe = 0; subframe < n_subframes; subframe++)
                { 
                    /* Gain Ind 0: 3-5 */
                    BsGetBit (bitStream, &dummy, 3);
                    gain_indices[subframe * num_gain_cbks] |= (dummy<<3);
                    
                    /* gain index 1:sfr1: 3-4 else: 2*/
                    if (subframe == 0)
                    {
                        BsGetBit(bitStream, &dummy, 2);
                        gain_indices[subframe * num_gain_cbks + 1] |= (dummy<<3);
                    }
                    else
                    {
                        BsGetBit(bitStream, &dummy, 1);
                        gain_indices[subframe * num_gain_cbks + 1] |= (dummy<<2);
                    }
                }
                
                
                /**********************************************************************************/
                /* Read Class 1 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
		  stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];
        				
				if (LPC_Present == YES)
                {
                    Read_Wideband_LSP_V2(bitStream, indices, 1);
                }
                
                /********************* Excitation Data *********************/
                /* Shape Delay: 5-7 */
                for (subframe = 0; subframe < n_subframes; subframe++ ) 
                {
                    BsGetBit(bitStream, &dummy, 3);
                    shape_indices[subframe * num_shape_cbks] |= (dummy<<5);
                }
                
                
                /**********************************************************************************/
                /* Read Class 2 Data                                                              */
                /**********************************************************************************/

		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];


                /********************* LPC Data *********************/
                if (LPC_Present == YES)
                {
                    Read_Wideband_LSP_V2(bitStream, indices, 2);
                }
                
                /********************* Excitation Data *********************/
                for (subframe = 0; subframe < n_subframes; subframe++)
                {   
                    /* Shape Delay: 3-4 */
                    BsGetBit(bitStream, &dummy, 2);
                    shape_indices[subframe * num_shape_cbks] |= dummy<<3;
                    
                    /* Gain Ind 0: 2 */
                    BsGetBit(bitStream, &dummy, 1);
                    gain_indices[subframe * num_gain_cbks] |= (dummy<<2);
                    
                    /* gain index 1:sfr1: 2 else: 1*/
                    if (subframe == 0)
                    {
                        BsGetBit(bitStream, &dummy, 1);
                        gain_indices[subframe * num_gain_cbks + 1] |= dummy<<2;
                    }
                    else
                    {
                        BsGetBit(bitStream, &dummy, 1);
                        gain_indices[subframe * num_gain_cbks + 1] |= dummy<<1;
                    }
                }
                
                /**********************************************************************************/
                /* Read Class 3 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];

                /********************* LPC Data *********************/
                if (LPC_Present == YES)
                {
                    Read_Wideband_LSP_V2(bitStream, indices, 3);
                }
                
                /********************* Excitation Data *********************/
                /* Shape Delay: 1-2 */
                for ( subframe = 0; subframe < n_subframes; subframe++ ) 
                {
                    BsGetBit(bitStream, &dummy, 2);
                    shape_indices[subframe * num_shape_cbks] |= dummy<<1;
                }
                
                /**********************************************************************************/
                /* Read Class 4 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];


           
				/********************* LPC Data *********************/
                if (LPC_Present == YES)
                {
                    Read_Wideband_LSP_V2(bitStream, indices, 4);
                }
                
                /********************* Excitation Data *********************/
                for (subframe = 0; subframe < n_subframes; subframe++)
                {   
                    /* Shape Delay: 0 */
                    BsGetBit(bitStream, &dummy, 1);
                    shape_indices[subframe * num_shape_cbks] |= dummy;
                    
                    /* Shape Index: all */
                    switch (RPE_configuration)
                    {
                        case 0:   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
                                  break;
                        case 1:   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
                                  break;
                        case 2:   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
                                  break;
                        case 3:   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
                                  break;
                    }
                
                    /* Gain Ind 0: 0-1 */
                    BsGetBit(bitStream, &dummy, 2);
                    gain_indices[subframe * num_gain_cbks] |= dummy;
                    
                    /* gain index 1:sfr1: 0-1 else: 0*/
                    if (subframe == 0)
                    {
                        BsGetBit(bitStream, &dummy, 2);
                        gain_indices[subframe * num_gain_cbks + 1] |= dummy;
                    }
                    else
                    {
                        BsGetBit(bitStream, &dummy, 1);
                        gain_indices[subframe * num_gain_cbks + 1] |= dummy;
                    }
                }
            }
            else 
            {
                /**********************************************************************************/
                /* Re-Ordered MPE Data 8 & 16 kHz                                                 */
                /**********************************************************************************/
                
                /**********************************************************************************/
                /* Read Class 0 Data                                                              */
                /**********************************************************************************/
                /********************* LPC Data *********************/
                if (FineRateControl == ON)
                {
                    BsGetBit(bitStream, &interpolation_flag, 1);
                    BsGetBit(bitStream, &LPC_Present, 1);
                }
                
                if (SampleRateMode == fs8kHz) 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Narrowband_LSP_V2(bitStream, indices, 0);
                    }
                    
                    /********************* Excitation Data *********************/
                    /* RMS ind: 4-5 */
                    BsGetBit (bitStream, &dummy, 2);
                    rms_index |= (dummy<<4);
                    
                    /* Shape Delay: 7 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsGetBit(bitStream,&dummy, 1);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<7);
                    }
                } 
                else 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Wideband_LSP_V2(bitStream, indices, 0);
                    }
                    /********************* Excitation Data *********************/
                    /* RMS ind: 4-5 */
                    BsGetBit (bitStream, &dummy, 2);
                    rms_index |= (dummy<<4);
                }
                
                /**********************************************************************************/
                /* Read Class 1 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];

                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Narrowband_LSP_V2(bitStream, indices, 1);
                    }
                    /********************* Excitation Data *********************/
                    /* signal_mode  */
                    BsGetBit(bitStream, &signal_mode, 2);
                    
                    /* Shape Delay: 5-6 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsGetBit(bitStream,&dummy, 2);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<5);
                    }
                } 
                else 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Wideband_LSP_V2(bitStream, indices, 1);
                    }
                    
                    /********************* Excitation Data *********************/
                    /* signal_mode: 0-1*/
                    BsGetBit(bitStream, &signal_mode, 2);
                    
                    /* Shape Delay: 6-8 */
                    for ( i = 0; i < n_subframes; i++ ) 
                    {
                        BsGetBit(bitStream, &dummy, 3);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<6);
                    }
                }
            
                /**********************************************************************************/
                /* Read Class 2 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];

                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Narrowband_LSP_V2(bitStream, indices, 2);
                    }
                    /********************* Excitation Data *********************/
                    /* RMS ind: 3 */
                    BsGetBit(bitStream, &dummy, 1);
                    rms_index |= (dummy << 3);
                    
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 3-4 */
                        BsGetBit(bitStream,&dummy, 2);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<3);
                        
                        /* Gain Index: 0-1 */
                        BsGetBit(bitStream, &dummy, 2);
                        gain_indices[i*num_gain_cbks+0] |= dummy;
                    }
                } 
                else 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Wideband_LSP_V2(bitStream, indices, 2);
                    }
                    
                    /********************* Excitation Data *********************/
                    /* RMS ind: 3 */
                    BsGetBit(bitStream, &dummy, 1);
                    rms_index |= (dummy << 3);
                    
                    for ( i = 0; i < n_subframes; i++ )
                    {   
                        /* Shape Delay: 4-5 */
                        BsGetBit(bitStream,&dummy, 2);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<4);
                        
                        /* Gain Index: 0-1 */
                        BsGetBit(bitStream, &dummy, 2);   
                        gain_indices[i*num_gain_cbks+0] |= dummy;
                    }
                }
                
#ifdef VERSION2_EC  /* SC: 1, EC: MODE, MPE */
                /* error concealment of mode */
                
#if (!FLG_BER_or_FER)
                errConSaveMode_received (signal_mode);
                
#endif
                if( getErrAction() & EA_MODE_PREVIOUS )
                {
                    errConLoadMode( &signal_mode );
                }
                else
                {
                    errConSaveMode( signal_mode );
                }
#endif    /*  VERSION2_EC   */
                
                /**********************************************************************************/
                /* Read Class 3 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];

                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Narrowband_LSP_V2(bitStream, indices, 3);
                    }
                    
        /********************* Excitation Data *********************/
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 0-2 */
                        BsGetBit(bitStream,&dummy, 3);
                        shape_indices[i*num_shape_cbks+0] |= dummy;
                        
                        /* Shape Sign: all */
                        BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
                        
                        /* Gain Index: 2 */
                        BsGetBit(bitStream,&dummy, 1);
                        gain_indices[i*num_gain_cbks+0] |= (dummy<<2);
                    }
				}
                else 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Wideband_LSP_V2(bitStream, indices, 3);
                    }
                    
                    /********************* Excitation Data *********************/
                    for ( i = 0; i < n_subframes; i++ ) 
                    {  
                        /* Shape Delay: 2-3 */
                        BsGetBit(bitStream,&dummy, 2);
                        shape_indices[i*num_shape_cbks+0] |= (dummy<<2);

                        /* Shape Sign: all */
                        BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);                        

                        /* Gain Index: 2 */
                        BsGetBit(bitStream,&dummy, 1);
                        gain_indices[i*num_gain_cbks+0] |= (dummy<<2);
                    }
                }
                
                /**********************************************************************************/
                /* Read Class 4 Data                                                              */
                /**********************************************************************************/
		if (epConfig) 
      stream_ctr++;
		bitStream = bitStreamLay[stream_ctr];

                /********************* LPC Data *********************/
                if (SampleRateMode == fs8kHz) 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Narrowband_LSP_V2(bitStream, indices, 4);
                    }
                    /********************* Excitation Data *********************/
                    /* RMS ind: 0-2 */
                    BsGetBit(bitStream, &dummy, 3);
                    rms_index |= dummy;
                    
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Position: all */
                        BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                        
                        
                        /* Gain Index: 3-5 */
                        BsGetBit(bitStream,&dummy, 3);
                        gain_indices[i*num_gain_cbks+0] |= (dummy<<3);
                    }
                } 
                else 
                {
                    if (LPC_Present == YES)
                    {
                        Read_Wideband_LSP_V2(bitStream, indices, 4);
                    }
                    
                    /********************* Excitation Data *********************/
                    /* RMS ind: 0-2 */
                    BsGetBit(bitStream, &dummy, 3);
                    rms_index |= dummy;
                    
                    for ( i = 0; i < n_subframes; i++ ) 
                    {   
                        /* Shape Delay: 0-1 */
                        BsGetBit(bitStream,&dummy, 2);
                        shape_indices[i*num_shape_cbks+0] |= dummy;
                        
                        /* Shape Position: all */
                        BsGetBit(bitStream, &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
                        
                        
                        /* Gain Index: 3-6 */
                        BsGetBit(bitStream,&dummy, 4);
                        gain_indices[i*num_gain_cbks+0] |= (dummy<<3);
                    }
                }
            }
        }     /*  TX_flag_dec == 1  */
        

        if (TX_flag_dec == 2)
        {
            if (SampleRateMode == fs8kHz) 
            {
                BsGetBit(bitStream,&indices[0],4);
                BsGetBit(bitStream,&indices[1],4);
                BsGetBit(bitStream,&indices[2],7);
            }
            
            if (SampleRateMode == fs16kHz) 
            {
                BsGetBit(bitStream,&indices[0],5);
                BsGetBit(bitStream,&indices[1],5);
                BsGetBit(bitStream,&indices[2],7);
                BsGetBit(bitStream,&indices[3],7);
                BsGetBit(bitStream,&indices[5],4);
                BsGetBit(bitStream,&indices[6],4);
            }
            BsGetBit(bitStream, &rms_index, NEC_BIT_RMS);
            
#ifdef VERSION2_EC  /* SC: 2, EC: MODE, MPE/RPE */
            if( !(getErrAction() & EA_MODE_PREVIOUS) )
            {
                errConSaveMode( (unsigned long)0 );
            }
#endif    /*   VERSION2_EC   */
        }
        
        if (TX_flag_dec == 3) 
        {
            BsGetBit(bitStream, &rms_index, NEC_BIT_RMS);
#ifdef VERSION2_EC  /* SC: 3, EC: MODE, MPE/RPE */
            if (!(getErrAction() & EA_MODE_PREVIOUS))
            {
                errConSaveMode( (unsigned long)0 );
            }
#endif    /*  VERSION2_EC   */
        }
#ifdef VERSION2_EC  /* SC: 0, EC: MODE, MPE/RPE */
        else
        {
            if (!(getErrAction() & EA_MODE_PREVIOUS))
            {
                errConSaveMode( (unsigned long)0 );
            }
        }
#endif    /*  VERSION2_EC   */

    }    /*   CelpObjectV2   */


    



    if (ExcitationMode == MultiPulseExc) 
    {
        long i, j;
        
        /* CelpBRSenhFrame() */
        if ( num_enhstages >= 1 ) 
        {
            if ((dec_objectVerType == CelpObjectV1) ||
                ((dec_objectVerType == CelpObjectV2) && (SilenceCompressionSW == OFF || TX_flag_dec == 1)))
            {
                for (j = 1; j <= num_enhstages; j++) 
                {
                   
                    stream_ctr++;
                    bitStream = bitStreamLay[stream_ctr];

                    for (i = 0; i < n_subframes; i++)
                    {
                        shape_indices[(j*n_subframes+i)*num_shape_cbks+0] = 0;
                        BsGetBit(bitStream, &shape_indices[(j*n_subframes+i)*num_shape_cbks+1], enh_pos_bits);
                        BsGetBit(bitStream, &shape_indices[(j*n_subframes+i)*num_shape_cbks+2], enh_sgn_bits);
                        BsGetBit(bitStream, &gain_indices[(j*n_subframes+i)*num_gain_cbks+0], NEC_BIT_ENH_GAIN);
                    }
                }
            } 
            else if (TX_flag_dec >= 2)
            {
          
		    if (epConfig) 
          stream_ctr += 4;
          stream_ctr += num_enhstages;
                
            }

        }
  else
	{
            if( dec_objectVerType == CelpObjectV2 &&
		SilenceCompressionSW == ON && TX_flag_dec >= 2 )
	    {
                if(epConfig )
		    stream_ctr += 4;
            }
	}

        /* CelpBWSenhFrame() */
        if ((SampleRateMode == fs8kHz) && 
            (FineRateControl == OFF) && (QuantizationMode == VectorQuantizer) &&
            (BandwidthScalabilityMode == ON)) 
        {
            if ((dec_objectVerType == CelpObjectV1) ||
                ((dec_objectVerType == CelpObjectV2) && (SilenceCompressionSW == OFF || TX_flag_dec == 1)))
            {
                stream_ctr++;
                bitStream = bitStreamLay[stream_ctr];
                
                Read_BandScalable_LSP(bitStream, indices_bws);
                
                for ( i = 0; i < n_subframes_bws; i++ ) 
                {
                    BsGetBit(bitStream, &shape_indices_bws[i*num_shape_cbks+0], NEC_BIT_ACB_FRQ16);
                    BsGetBit(bitStream, &shape_indices_bws[i*num_shape_cbks+1], bws_pos_bits);
                    BsGetBit(bitStream, &shape_indices_bws[i*num_shape_cbks+2], bws_sgn_bits);
                    BsGetBit(bitStream, &gain_indices_bws[i*num_gain_cbks+0], NEC_BIT_GAIN_FRQ16);
                }

#ifdef VERSION2_EC  /* SC: 1, EC: GAIN LAG PULSE, BWS */
                
                if (errConGetScModePre() == 1)
                {
                    int i;
#if FLG_BER_or_FER
                    /* FER mode */
                    
                    /* error concealment of gain */
                    if (getErrAction() & EA_GAIN_PREVIOUS)
                    {
                        unsigned long tmp;
                        
                        errConLoadGainBws( &tmp );
                        for (i = 0; i < n_subframes_bws; i++)
                        {
                            gain_indices_bws[i*num_gain_cbks+0] = tmp;
                        }
                    } 
                    else
                    {
                        errConSaveGainBws (gain_indices_bws[(n_subframes_bws-1)*num_gain_cbks+0]);
                    }
                    
                    /* error concealment of lag */
                    if( getErrAction() & EA_LAG_PREVIOUS )
                    {
                         unsigned long    last_lag;
                        
                        errConLoadLagBws( &last_lag );
                        for( i = 0; i < n_subframes_bws; i++ )
                        {
                            shape_indices_bws[i*num_shape_cbks+0] = last_lag;
                        }
                    } 
                    else
                    {
                        errConSaveLagBws( shape_indices_bws[(n_subframes_bws-1)*num_shape_cbks+0] );
                    }
                    
                    /* error concealment of multi-pulse info */
                    if (getErrAction() & EA_MPULSE_RANDOM)
                    {
                        for (i = 0; i < n_subframes_bws; i++)
                        {
                            setRandomBits( &shape_indices_bws[i*num_shape_cbks+1], (int)bws_pos_bits, &seed );
                            setRandomBits( &shape_indices_bws[i*num_shape_cbks+2], (int)bws_sgn_bits, &seed );
                        }
                    }
#else /* FLG_BER_or_FER */
                    /* BER mode */
                    
                    /* error concealment of gain */
                    /* nothing */
                    
                    /* error concealment of lag */
                    if ((getErrAction() & EA_LAG_PREVIOUS) && (signal_mode !=1) && (signal_mode !=0 ) )
                    {
                         unsigned long    last_lag;
                        
                        errConLoadLagBws( &last_lag );
                        for( i = 0; i < n_subframes_bws; i++ )
                        {
                            shape_indices_bws[i*num_shape_cbks+0] = last_lag;
                        }
                        
                    }
                    else if ((getErrAction() & EA_LAG_PREVIOUS) && (signal_mode == 1) )
                    {
                        unsigned long    last_lag;
                        short   con_flag;
                        
                        errConLoadLagBws( &last_lag );
                        con_flag = 0;
                        
                        for (i = 0; i < n_subframes_bws - 1; i++)
                        {
                            if (abs( shape_indices_bws[i*num_shape_cbks+0] - shape_indices_bws[(i+1)*num_shape_cbks+0] ) < 10 )
                            {
                                con_flag++;
                            }
                        }
                        
                        if (con_flag != n_subframes_bws - 1)
                        {
                            for( i = 0; i < n_subframes_bws; i++ )
                            {
                                if( abs( shape_indices_bws[i*num_shape_cbks+0] - last_lag ) >= 10 )
                                {
                                    shape_indices_bws[i*num_shape_cbks+0] = last_lag;
                                }
                                last_lag = shape_indices_bws[i*num_shape_cbks+0];
                            }
                            errConSaveLagBws( last_lag );
                        }
                        else
                        {
                            errConSaveLagBws( shape_indices_bws[(n_subframes_bws-1)*num_shape_cbks+0] );
                        }
                    } 
                    else
                    {
                        errConSaveLagBws( shape_indices_bws[(n_subframes_bws-1)*num_shape_cbks+0] );
                    }
#endif  /* FLG_BER_or_FER */
                } 
                else
                {  /* errConGetScModePre() != 1 */
                    int i;
                    
                    /* error concealment of gain */
#if FLG_BER_or_FER
                    if (getErrAction() & EA_GAIN_PREVIOUS )
                    {
                        /* no action */
                    }
                    else
                    {
                        errConSaveGainBws( gain_indices_bws[(n_subframes_bws-1)*num_gain_cbks+0] );
                    }
#endif    /*  FLG_BER_or_FER   */
                    
                    /* error concealment of lag */
                    if (getErrAction() & EA_LAG_PREVIOUS)
                    {
                                               
                        for (i = 0; i < n_subframes_bws; i++)
                        {
                            shape_indices_bws [i*num_shape_cbks+0] = 0;  /* no use */
                        }
                    } 
                    else
                    {
                        errConSaveLagBws (shape_indices_bws[(n_subframes_bws-1)*num_shape_cbks+0]);
                    }
                    
                    /* error concealment of multi-pulse info */
                    if (getErrAction() & EA_MPULSE_RANDOM )
                    {
                        for( i = 0; i < n_subframes_bws; i++ )
                        {
                            setRandomBits( &shape_indices_bws[i*num_shape_cbks+1], (int)bws_pos_bits, &seed );
                            setRandomBits( &shape_indices_bws[i*num_shape_cbks+2], (int)bws_sgn_bits, &seed );
                        }
                    }
                }
#endif /* VERSION2_EC */
            }
            else if (TX_flag_dec == 2)
            {
                stream_ctr++;
                bitStream = bitStreamLay[stream_ctr];
                
                BsGetBit(bitStream,&indices_bws[0],4);
                BsGetBit(bitStream,&indices_bws[1],7);
                BsGetBit(bitStream,&indices_bws[2],4);
                BsGetBit(bitStream,&indices_bws[3],6);
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

    if ((SilenceCompressionSW == ON) && (TX_flag_dec == 0))
    {
        stream_ctr = 0;
        bitStream = bitStreamLay[stream_ctr];
    }

#ifdef VERSION2_EC  /* SC: 1, EC: GAIN LAG PULSE, MPE */
    
    if (ExcitationMode == MultiPulseExc && (TX_flag_dec = errConGetScMode()) == 1)
    {
        /* TX_flag may change */
        int i, j;
        
        if( errConGetScModePre() == 1 )
        {
#if FLG_BER_or_FER
            /* FER mode */
            
            /* error concealment of gain */
            if( getErrAction() & EA_GAIN_PREVIOUS )
            {
                unsigned long tmp;
                
                for( j = 0; j <= num_enhstages; j++ )
                {
                    errConLoadGain( &tmp, (long)j );
                    for( i = 0; i < n_subframes; i++ )
                    {
                        gain_indices[(j*n_subframes+i)*num_gain_cbks+0] = tmp;
                    }
                }
            } 
            else
            {
                for( j = 0; j <= num_enhstages; j++ )
                {
                    errConSaveGain( gain_indices[(j*n_subframes+n_subframes-1) *num_gain_cbks+0], (long)j );
                }
            }
            
            /* error concealment of lag */
            if( getErrAction() & EA_LAG_PREVIOUS )
            {
                unsigned long    last_lag;
                
                errConLoadLag( &last_lag );
                for (i = 0; i < n_subframes; i++)
                    shape_indices[i*num_shape_cbks+0] = last_lag;
            } 
            else
            {
                errConSaveLag( shape_indices[(n_subframes-1)*num_shape_cbks+0] );
            }
            
            /* error concealment of multi-pulse info */
            if( getErrAction() & EA_MPULSE_RANDOM )
            {
                for( j = 0; j <= num_enhstages; j++ )
                {
                    long pos_bits, sgn_bits;
                    
                    if( j == 0 )
                    {
                        pos_bits = mp_pos_bits;
                        sgn_bits = mp_sgn_bits;
                    } 
                    else
                    {
                        pos_bits = enh_pos_bits;
                        sgn_bits = enh_sgn_bits;
                    }
                    
                    for( i = 0; i < n_subframes; i++ )
                    {
                        setRandomBits( &shape_indices[(j*n_subframes+i)*num_shape_cbks+1], (int)pos_bits, &seed );
                        setRandomBits( &shape_indices[(j*n_subframes+i)*num_shape_cbks+2], (int)sgn_bits, &seed );
                    }
                }
            }
#else /* FLG_BER_or_FER */
            /* BER mode */
            
            /* error concealment of gain */
            /* no action */
            
            /* error concealment of lag */
            if ((getErrAction() & EA_LAG_PREVIOUS) && (signal_mode != 1) && (signal_mode != 0))
            {
                unsigned long    last_lag;
                
                errConLoadLag( &last_lag );
                for ( i = 0; i < n_subframes; i++ )
                {
                    shape_indices[i*num_shape_cbks+0] = last_lag;
                }
            }
            else if ( (getErrAction() & EA_LAG_PREVIOUS) && (signal_mode == 1) )
            {
                unsigned long    last_lag;
                short   con_flag;
                
                errConLoadLag( &last_lag );
                con_flag = 0;
                for ( i = 0; i < n_subframes - 1; i++ )
                {
                    if( abs( shape_indices[i*num_shape_cbks+0] - shape_indices[(i+1)*num_shape_cbks+0] ) < 10 )
                        con_flag++;
                }
                
                if( con_flag != n_subframes - 1 )
                {
                    for( i = 0; i < n_subframes; i++ )
                    {
                        if( abs( shape_indices[i*num_shape_cbks+0] - last_lag ) >= 10 )
                        {
                            shape_indices[i*num_shape_cbks+0] = last_lag;
                        }
                        last_lag = shape_indices[i*num_shape_cbks+0];
                    }
                    errConSaveLag( last_lag );
                } 
                else
                {
                    errConSaveLag( shape_indices[(n_subframes-1)*num_shape_cbks+0] );
                }
            }
            else
            {
                errConSaveLag( shape_indices[(n_subframes-1)*num_shape_cbks+0] );
            }
#endif /* FLG_BER_or_FER */
        } 
        else
        { /* errConGetScModePre() != 1 */
            /* error concealment of gain */
#if FLG_BER_or_FER
            if ( getErrAction() & EA_GAIN_PREVIOUS )
            {
                /* no action */
            }
            else
            {
                for ( j = 0; j <= num_enhstages; j++ )
                {
                    errConSaveGain( gain_indices[(j*n_subframes+n_subframes-1) *num_gain_cbks+0], (long)j );
                }
            }
#endif
            
            /* error concealment of lag */
            if( getErrAction() & EA_LAG_PREVIOUS )
            {
                
                for( i = 0; i < n_subframes; i++ )
                {
                    shape_indices[i*num_shape_cbks+0] = 0;  /* no use */
                }
            } 
            else
            {
                errConSaveLag( shape_indices[(n_subframes-1)*num_shape_cbks+0] );
            }
            
            /* error concealment of multi-pulse info */
            if ( getErrAction() & EA_MPULSE_RANDOM )
            {
                for ( j = 0; j <= num_enhstages; j++ )
                {
                    long pos_bits, sgn_bits;
                    
                    if( j == 0 )
                    {
                        pos_bits = mp_pos_bits;
                        sgn_bits = mp_sgn_bits;
                    }
                    else
                    {
                        pos_bits = enh_pos_bits;
                        sgn_bits = enh_sgn_bits;
                    }
                    
                    for( i = 0; i < n_subframes; i++ )
                    {
                        setRandomBits( &shape_indices[(j*n_subframes+i)*num_shape_cbks+1], (int)pos_bits, &seed );
                        setRandomBits( &shape_indices[(j*n_subframes+i)*num_shape_cbks+2], (int)sgn_bits, &seed );
                    }
                }
            }
        }
    }
#endif /* VERSION2_EC */
    

    /* -----------------------------------------------------------------*/
    /* Decode LPC coefficients                                          */
    /* -----------------------------------------------------------------*/
  
    if (FineRateControl == ON)
    {
		if (TX_flag_dec != 1)
        {
           interpolation_flag = 0;
           PHI_Priv->PHI_dec_prev_interpolation_flag = 0;
        }

        VQ_celp_lpc_decode(indices, int_ap, lpc_order, n_subframes,
                           interpolation_flag, Wideband_VQ, PHI_Priv, TX_flag_dec);
    }   
    else
    {  /*  FineRate Control OFF  */
        if (SampleRateMode == fs8kHz)
        {
            long k;
            
            if (ExcitationMode == RegularPulseExc)
            {
                fprintf (stderr, "Combination of RPE + 8 kHz sampling rate not supported.\n");
                exit (1);
            }
            
            if (ExcitationMode == MultiPulseExc)
            {
                nb_abs_lpc_decode(indices, int_ap, lpc_order, n_subframes,
                    prev_Qlsp_coefficients);
            }
            
            if ( BandwidthScalabilityMode == ON)
            {
                for ( k = 0; k < lpc_order; k++ )
                {
                    buf_Qlsp_coefficients_bws[k] = PAN_PI * prev_Qlsp_coefficients[k];
                }
                bws_lpc_decoder (indices_bws, int_ap_bws, lpc_order, lpc_order_bws,
                                 n_subframes_bws, buf_Qlsp_coefficients_bws, 
                                 prev_Qlsp_coefficients_bws );
            }
        }
        
        if (SampleRateMode == fs16kHz)
        {
            if (ExcitationMode == RegularPulseExc)
            {
                VQ_celp_lpc_decode (indices, int_ap, lpc_order, n_subframes,
                                    interpolation_flag, Wideband_VQ, PHI_Priv, TX_flag_dec);
            }
            
            if (ExcitationMode == MultiPulseExc)
            {
                wb_celp_lsp_decode(indices, int_ap, lpc_order, n_subframes,
                    prev_Qlsp_coefficients);
            }       
        } 
    }
    
    if (ExcitationMode==RegularPulseExc)
    {
        static long  flag_rms = 0;
        static float  pqxnorm;
        static short prev_tx = 1;
        
        if (prev_tx != 1)
        {
            reset_cba_codebook_dec (PHI_Priv->PHI_adaptive_cbk, Lmax);
        }
        /* -----------------------------------------------------------------*/
        /* Subframe processing                                              */
        /* -----------------------------------------------------------------*/
        for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++)
        {
            long  m = sbfrm_ctr * lpc_order;
            float *cbk_sig;
            float *syn_speech = 0;
            
            /* -------------------------------------------------------------*/
            /* Create Arrays for subframe processing                        */
            /* -------------------------------------------------------------*/
            if
                (
                (( cbk_sig = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )||
                (( syn_speech = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )
                )
            {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
            
            if(TX_flag_dec == 1) 
            {
                
                /* ------------------------------------------------------------*/
                /* Excitation Generation                                       */
                /* ------------------------------------------------------------*/
                celp_excitation_generation (&(shape_indices[sbfrm_ctr*num_shape_cbks]),
                                            &(gain_indices[sbfrm_ctr*num_gain_cbks]), 
                                            num_shape_cbks,
                                            num_gain_cbks, 
                                            sbfrm_size,
                                            n_subframes, 
                                            cbk_sig, 
                                            PHI_Priv);
            }
            else
            {
                /* Confort Noise Generation in Non-active frame */
                mk_cng_excitation_dec (rms_index, 
                                       int_ap+lpc_order*sbfrm_ctr, 
                                       lpc_order, 
                                       sbfrm_size, 
                                       n_subframes, 
                                       cbk_sig, 
                                       SampleRateMode, 
                                       ExcitationMode,
                                       RPE_configuration, 
                                       &flag_rms, 
                                       &pqxnorm, 
                                       prev_tx);
            } 
            prev_tx = TX_flag_dec;
            
            /* ------------------------------------------------------------*/
            /* Speech Generation                                           */
            /* ------------------------------------------------------------*/
            celp_lpc_synthesis_filter (cbk_sig, syn_speech, &int_ap[m], lpc_order, sbfrm_size, PHI_Priv);
            
            if (postfilter)
            {
                /* --------------------------------------------------------*/
                /* Post Processing                                         */
                /* --------------------------------------------------------*/
                celp_postprocessing(syn_speech, &(OutputSignal[0][sbfrm_ctr*sbfrm_size]), &int_ap[m], lpc_order, sbfrm_size, PHI_Priv); 
                
            }
            else
            {
                long k;
                float *psyn_speech   =  syn_speech;
                float *pOutputSignal =  &(OutputSignal[0][sbfrm_ctr*sbfrm_size]);
                
                for(k=0; k < sbfrm_size; k++)
                {
                    *pOutputSignal++ = *psyn_speech++;    
                }
            }
            
            /* -------------------------------------------------------------*/
            /* Free   Arrays for subframe processing                        */
            /* -------------------------------------------------------------*/
            free ( syn_speech );
            free ( cbk_sig );
        }    
    } 
      
    if (ExcitationMode==MultiPulseExc)
    {
        float *dec_sig;
        float *bws_mp_sig = 0;
        long  *acb_delay;
        float *adaptive_gain = 0;
        float *dec_sig_bws = 0;
        long  *bws_delay = 0;
        float *bws_adpt_gain = 0;
        
        float *syn_sig;   /* HP 971112 */
        long  kk;
        static long  flag_rms = 0;
        static float  pqxnorm;
        static short prev_tx = 1;
        
        if (SampleRateMode == fs8kHz) 
        {
            if(
                (( dec_sig   = (float *)malloc((unsigned int)frame_size_nb * sizeof(float))) == NULL )||
                (( bws_mp_sig   = (float *)malloc((unsigned int)frame_size_nb * sizeof(float))) == NULL )
                ) 
            {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
        } 
        else 
        {
            if(
                (( dec_sig   = (float *)malloc((unsigned int)frame_size * sizeof(float))) == NULL )||
                (( bws_mp_sig   = (float *)malloc((unsigned int)frame_size * sizeof(float))) == NULL )
                ) 
            {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
        }
        
        if((( acb_delay   = (long *)malloc(n_subframes * sizeof(long))) == NULL )||
            (( adaptive_gain   = (float *)malloc(n_subframes * sizeof(float))) == NULL ) ) 
        {
            fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
            CommonExit(1,"celp error");
;
        }
        
#ifdef VERSION2_EC  /* SC: 0123, EC: RMS, MPE */
    errConLoadRms( &pqxnorm );
#endif
        for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++)
		{
            if(TX_flag_dec == 1)
			{
                /* Decoding Excitation in Active frame */
                nb_abs_excitation_generation (shape_indices, gain_indices, 
                                              num_shape_cbks, num_gain_cbks, rms_index,
                                              int_ap+lpc_order*sbfrm_ctr, lpc_order, 
                                              sbfrm_size, n_subframes, signal_mode, 
                                              org_frame_bit_allocation,
                                              dec_sig+sbfrm_size*sbfrm_ctr, 
                                              bws_mp_sig+sbfrm_size*sbfrm_ctr, 
                                              acb_delay+sbfrm_ctr,
                                              adaptive_gain+sbfrm_ctr,
                                              dec_enhstages,postfilter,
                                              SampleRateMode,
                                              &flag_rms, &pqxnorm, prev_tx);
                
            }else
			{
                /* Confort Noise Generation in Non-active frame */
                mk_cng_excitation_dec (rms_index, int_ap+lpc_order*sbfrm_ctr,
                                       lpc_order, sbfrm_size, n_subframes,
                                       dec_sig+sbfrm_size*sbfrm_ctr, 
                                       SampleRateMode, ExcitationMode,
                                       MPE_Configuration, &flag_rms, 
                                       &pqxnorm, prev_tx);
                acb_delay[sbfrm_ctr] = 0;
                adaptive_gain[sbfrm_ctr] = 0.0;
			}
            
        }
        prev_tx = TX_flag_dec;

        if (BandwidthScalabilityMode == ON) 
        {
            static long flag_rms = 0;
            static float pqxnorm;
            static short prev_tx=1;
            if(
                (( dec_sig_bws   = (float *)malloc((unsigned int)frame_size_bws * sizeof(float))) == NULL )
                ) {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
            if((( bws_delay   = (long *)malloc(n_subframes_bws * sizeof(long))) == NULL )||
                (( bws_adpt_gain   = (float *)malloc(n_subframes_bws * sizeof(float))) == NULL ) ) {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
            
            for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) {
                bws_nb_acb_index[sbfrm_ctr] = shape_indices[sbfrm_ctr*n_subframes/n_subframes_bws*num_shape_cbks];
            }
            
#ifdef VERSION2_EC      /* SC: 0123, EC: RMS, BWS */
        errConLoadRmsBws( &pqxnorm );
#endif
            for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) {
                if(TX_flag_dec == 1){
                    bws_excitation_generation (shape_indices_bws, gain_indices_bws, 
                                               num_shape_cbks, num_gain_cbks, rms_index,
                                               int_ap_bws+lpc_order_bws*sbfrm_ctr,
                                               lpc_order_bws, 
                                               sbfrm_size_bws, n_subframes_bws,
                                               signal_mode, 
                                               &org_frame_bit_allocation[num_indices-n_subframes_bws*(num_shape_cbks+num_gain_cbks)],
                                               dec_sig_bws+sbfrm_size_bws*sbfrm_ctr, 
                                               bws_mp_sig+sbfrm_size*n_subframes/n_subframes_bws*sbfrm_ctr,
                                               bws_nb_acb_index,
                                               bws_delay+sbfrm_ctr,bws_adpt_gain+sbfrm_ctr,postfilter, &flag_rms, &pqxnorm, prev_tx);
                }else{
                    /* Confort Noise Generation in Non-active frame */
                    mk_cng_excitation_bws_dec (rms_index,
                                               int_ap_bws+lpc_order_bws*sbfrm_ctr,
                                               lpc_order_bws, sbfrm_size_bws,
                                               n_subframes_bws, 
                                               dec_sig_bws+sbfrm_size_bws*sbfrm_ctr, 
                                               &flag_rms, &pqxnorm, prev_tx);
                    bws_delay[sbfrm_ctr] = 0;
                    bws_adpt_gain[sbfrm_ctr] = 0.0;
                }
                
            }
			prev_tx = TX_flag_dec;
        }

        if ( (BandwidthScalabilityMode==ON) && (dec_bwsmode) ) 
        {
            if(( syn_sig = (float *)malloc((unsigned int)sbfrm_size_bws * sizeof(float))) == NULL )
            {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
;
            }
            
            for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) 
            {
                celp_lpc_synthesis_filter (dec_sig_bws+sbfrm_size_bws*sbfrm_ctr,
                                           syn_sig,
                                           int_ap_bws+lpc_order_bws*sbfrm_ctr,
                                           lpc_order_bws, sbfrm_size_bws, PHI_Priv);
                if (postfilter) 
                {
                    nb_abs_postprocessing (syn_sig,
                                           &(OutputSignal[0][sbfrm_ctr*sbfrm_size_bws]),
                                           int_ap_bws+lpc_order_bws*sbfrm_ctr,
                                           lpc_order_bws, sbfrm_size_bws);
                }
                else 
                {
                    for(kk=0; kk < sbfrm_size_bws; kk++)
                        OutputSignal[0][sbfrm_ctr*sbfrm_size_bws+kk] = syn_sig[kk];
                }
            }
            free( syn_sig );
        } 
        else 
        {
            if(( syn_sig = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )
            {
                fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
                CommonExit(1,"celp error");
            }
            
            for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++) 
            {
                celp_lpc_synthesis_filter(dec_sig+sbfrm_size*sbfrm_ctr,
                                          syn_sig,
                                          int_ap+lpc_order*sbfrm_ctr,
                                          lpc_order, sbfrm_size, PHI_Priv);
                if (postfilter)
                {
                    nb_abs_postprocessing (syn_sig,
                                           &(OutputSignal[0][sbfrm_ctr*sbfrm_size]),
                                           int_ap+lpc_order*sbfrm_ctr,
                                           lpc_order, sbfrm_size);
                } 
                else 
                {
                    for(kk=0; kk < sbfrm_size; kk++)
                        OutputSignal[0][sbfrm_ctr*sbfrm_size+kk] = syn_sig[kk];
                }
            }
            free( syn_sig );
        }
        
        free ( dec_sig );
        free ( bws_mp_sig );
        free ( acb_delay );
        free ( adaptive_gain );
        
        if (BandwidthScalabilityMode == ON) 
        {
            free ( dec_sig_bws );
            free ( bws_delay );
            free ( bws_adpt_gain );
        }
    }
    
    /* -----------------------------------------------------------------*/
    /* Free   Arrays for Frame processing                               */
    /* -----------------------------------------------------------------*/
    free ( int_ap );
    free ( shape_indices );
    free ( gain_indices );
    free ( indices );
    
    if (SampleRateMode == fs8kHz) 
    {
        if ( BandwidthScalabilityMode == ON ) 
        {
            free ( int_ap_bws );
            free ( shape_indices_bws );
            free ( gain_indices_bws );
            free ( indices_bws );
            free ( bws_nb_acb_index );
        }
    }
    
    /* ----------------------------------------------------------------*/
    /* Report on the current frame count                               */
    /* ----------------------------------------------------------------*/
    frame_ctr++;
    if (frame_ctr % 10 == 0)
    {
        if (CELPdecDebugLevel) 
        {  /* HP 971120 */
            fprintf(stderr, "Frame Counter: %ld \r", frame_ctr);
        }
    }
    
}

/*======================================================================*/
/*   Function Definition:celp_initialisation_decoder                    */
/*======================================================================*/
void celp_initialisation_decoder (HANDLE_BSBITSTREAM hdrStream,                     /* In: Bitstream                                */
                                  long          DecEnhStage,                                                                    
                                  long          DecBwsMode,                                                                     
                                  long          PostFilterSW,                                                                   
                                  long          *frame_size,                    /* Out: frame size                              */
                                  long          *n_subframes,                   /* Out: number of  subframes                    */
                                  long          *sbfrm_size,                    /* Out: subframe size                           */ 
                                  long          *lpc_order,                     /* Out: LP analysis order                       */
                                  long          *num_lpc_indices,               /* Out: number of LPC indices                   */
                                  long          *num_shape_cbks,                /* Out: number of Shape Codeb.                  */    
                                  long          *num_gain_cbks,                 /* Out: number of Gain Codeb.                   */    
                                  long          **org_frame_bit_allocation,     /* Out: bit num. for each index                 */
                                  long          * ExcitationMode,               /* Out: Excitation Mode                         */
                                  long          * SampleRateMode,               /* Out: SampleRate Mode                         */
                                  long          * QuantizationMode,             /* Out: Type of Quantization                    */
                                  long          * FineRateControl,              /* Out: Fine Rate Control switch                */
                                  long          * LosslessCodingMode,           /* Out: Lossless Coding Mode                    */
                                  long          * RPE_configuration,            /* Out: Wideband configuration                  */
                                  long          * Wideband_VQ,                  /* Out: Wideband VQ mode                        */
                                  long          * MPE_Configuration,            /* Out: Narrowband configuration                */
                                  long          * NumEnhLayers,                 /* Out: Number of Enhancement Layers            */
                                  long          * BandwidthScalabilityMode,     /* Out: bandwidth switch                        */
                                  long          * BWS_configuration,            /* Out: BWS_configuration                       */
                                  long          * SilenceCompressionSW,         /* Out: SilenceCompressionSW                    */
                                  int           dec_objectVerType,
                                  void          **InstanceContext              /* Out: handle to initialised instance context  */
)
{
    
    INST_CONTEXT_LPC_DEC_TYPE    *InstCtxt;
    
    /* -----------------------------------------------------------------*/
    /* Create & initialise private storage for instance context         */
    /* -----------------------------------------------------------------*/
    if (( InstCtxt = (INST_CONTEXT_LPC_DEC_TYPE*)malloc(sizeof(INST_CONTEXT_LPC_DEC_TYPE))) == NULL )
    {
        fprintf(stderr, "MALLOC FAILURE in celp_initialisation_decoder  \n");
        CommonExit(1,"celp error");
;
    }
    
    if (( InstCtxt->PHI_Priv = (PHI_PRIV_TYPE*)malloc(sizeof(PHI_PRIV_TYPE))) == NULL )
    {
        fprintf(stderr, "MALLOC FAILURE in celp_initialisation_decoder  \n");
        CommonExit(1,"celp error");
;
    }
    
    PHI_Init_Private_Data(InstCtxt->PHI_Priv);
    
    *InstanceContext = InstCtxt;
    
    /* -----------------------------------------------------------------*/
    /*                                                                  */
    /* -----------------------------------------------------------------*/
    
    postfilter = PostFilterSW;
    
    /* -----------------------------------------------------------------*/
    /* Read bitstream header                                            */
    /* -----------------------------------------------------------------*/ 
    /* read from object descriptor */

    
    if (*ExcitationMode == RegularPulseExc)
    {
        if (*SampleRateMode == fs8kHz)
        {
            fprintf (stderr, "Combination of RPE + 8 kHz sampling rate not supported.\n");
            exit (1);
        }
        
        /* -----------------------------------------------------------------*/
        /*Check if a bit rate is a set of allowed bit rates                 */
        /* -----------------------------------------------------------------*/ 
        if (*RPE_configuration == 0)
        {
            *frame_size = FIFTEEN_MS;
            *n_subframes = 6;        
        }
        else if (*RPE_configuration == 1)
        {
            *frame_size  = TEN_MS;
            *n_subframes = 4;        
        }
        else if (*RPE_configuration == 2)
        {
            *frame_size  = FIFTEEN_MS;
            *n_subframes = 8;
        }
        else if (*RPE_configuration == 3)
        {
            *frame_size  = FIFTEEN_MS;
            *n_subframes = 10;
        }
        else
        {
            fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
            CommonExit(1,"celp error");
; 
        }
        
        *sbfrm_size          = (*frame_size)/(*n_subframes);
        
        *num_shape_cbks      = 2;     
        *num_gain_cbks       = 2;     
        
        *lpc_order       = ORDER_LPC_16;
        *num_lpc_indices = N_INDICES_VQ16;
        
        PHI_init_excitation_generation ( Lmax, *sbfrm_size, *RPE_configuration, InstCtxt->PHI_Priv );
        PHI_InitLpcAnalysisDecoder (ORDER_LPC_16, ORDER_LPC_8, InstCtxt->PHI_Priv);
        PHI_InitPostProcessor (*lpc_order, InstCtxt->PHI_Priv );
        
        *org_frame_bit_allocation = PHI_init_bit_allocation(*SampleRateMode, *RPE_configuration,
                                                            *FineRateControl, *num_lpc_indices,
                                                            *n_subframes, *num_shape_cbks, *num_gain_cbks); 
    }    
    
    if (*ExcitationMode == MultiPulseExc) 
    {
        if (*SampleRateMode == fs8kHz) 
        {
            int i, j;
            long ctr;
            
            num_enhstages = *NumEnhLayers;
            dec_enhstages = DecEnhStage;
            dec_bwsmode = DecBwsMode;
            
            if ( *MPE_Configuration >= 0 && *MPE_Configuration < 3 ) 
            {
                frame_size_nb = NEC_FRAME40MS;
                *n_subframes = NEC_NSF4;
            }
            
            if ( *MPE_Configuration >= 3 && *MPE_Configuration < 6 ) 
            {
                frame_size_nb = NEC_FRAME30MS;
                *n_subframes = NEC_NSF3;
            }
            
            if ( *MPE_Configuration >= 6 && *MPE_Configuration < 13 ) 
            {
                frame_size_nb = NEC_FRAME20MS;
                *n_subframes = NEC_NSF2;
            }
            
            if ( *MPE_Configuration >= 13 && *MPE_Configuration < 22 ) 
            {
                frame_size_nb = NEC_FRAME20MS;
                *n_subframes = NEC_NSF4;
            }
            
            if ( *MPE_Configuration >= 22 && *MPE_Configuration < 27 ) 
            {
                frame_size_nb = NEC_FRAME10MS;
                *n_subframes = NEC_NSF2;
            }
            
            if ( *MPE_Configuration == 27 ) 
            {
                frame_size_nb = NEC_FRAME30MS;
                *n_subframes = NEC_NSF4;
            }
            
            if ( *MPE_Configuration > 27 ) 
            {
                fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                CommonExit(1,"celp error");
; 
            }
            
            *sbfrm_size = frame_size_nb/(*n_subframes);
            *lpc_order = NEC_LPC_ORDER;
            *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
            *num_gain_cbks = NEC_NUM_GAIN_CBKS;
            
            if (*QuantizationMode == ScalarQuantizer) 
            {
                *num_lpc_indices = 4;
            }
            else 
            {
                *num_lpc_indices = PAN_NUM_LPC_INDICES;
            }
            
            num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES
                + (num_enhstages + 1) * (*n_subframes) *
                (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);

            switch ( *MPE_Configuration ) 
            {
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
            
            if ( *sbfrm_size == (NEC_FRAME20MS/NEC_NSF4) ) 
            {
                enh_pos_bits = NEC_BIT_ENH_POS40_2;
                enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
            }
            else 
            {
                enh_pos_bits = NEC_BIT_ENH_POS80_4;
                enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
            }
            
            if (*BandwidthScalabilityMode==ON) 
            {
                frame_size_bws = frame_size_nb * 2;
                n_subframes_bws = frame_size_bws/80;
                sbfrm_size_bws = frame_size_bws / n_subframes_bws;
                lpc_order_bws = NEC_LPC_ORDER_FRQ16;
                
                num_lpc_indices_bws = NEC_NUM_LPC_INDICES_FRQ16 ;
                num_indices += NEC_NUM_LPC_INDICES_FRQ16 
                    + n_subframes_bws * (NEC_NUM_SHAPE_CBKS
                    +NEC_NUM_GAIN_CBKS);

                switch ( *BWS_configuration ) 
                {
                    case 0:
                        bws_pos_bits = 22; bws_sgn_bits = 6; break;
                    case 1:
                        bws_pos_bits = 26; bws_sgn_bits = 8; break;
                    case 2:
                        bws_pos_bits = 30; bws_sgn_bits =10; break;
                    case 3:
                        bws_pos_bits = 32; bws_sgn_bits =12; break;
                }
            }
            
            if ( (*BandwidthScalabilityMode==ON) && (dec_bwsmode) ) 
            {
                *frame_size = frame_size_bws;
            }
            else 
            {
                *frame_size = frame_size_nb;
            }
            
            if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
                sizeof(long)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(3);
            }
            ofa=(*org_frame_bit_allocation);

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
            
            for ( i = 0; i < num_enhstages; i++ ) {
                for ( j = 0; j < *n_subframes; j++ ) {
                    *(*org_frame_bit_allocation+(ctr++)) =  0;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
                }
            }
            
            if (*BandwidthScalabilityMode==ON) {
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
            
            if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
                sizeof(float)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
                exit(5);
            }
            
            for(i=0;i<(*lpc_order);i++) 
                *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);
            
            if (*BandwidthScalabilityMode==ON) {
                if((buf_Qlsp_coefficients_bws=(float *)calloc(*lpc_order,
                    sizeof(float)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
                    exit(5);
                }
                
                if((prev_Qlsp_coefficients_bws=(float *)calloc(lpc_order_bws,
                    sizeof(float)))==NULL) {
                    fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
                    exit(5);
                }
                for(i=0;i<(lpc_order_bws);i++) 
                    *(prev_Qlsp_coefficients_bws+i) = PAN_PI * (i+1)
                    / (float)(lpc_order_bws+1);
            }
            
            /* submodules for initialization */
            if ((*BandwidthScalabilityMode==ON)&&(dec_bwsmode)) 
            {
                PHI_InitLpcAnalysisDecoder(lpc_order_bws, *lpc_order, InstCtxt->PHI_Priv);
            }
            else 
            {
                PHI_InitLpcAnalysisDecoder(*lpc_order, *lpc_order, InstCtxt->PHI_Priv);
            }
            
            if (*LosslessCodingMode == ON) 
            {
                *num_lpc_indices     = 10;   
            }
        }
        
        if (*SampleRateMode == fs16kHz) 
        {
            int i, j;
            long ctr;
            
            num_enhstages = *NumEnhLayers;
            dec_enhstages = DecEnhStage;
            
            if ((*MPE_Configuration>=0) && (*MPE_Configuration<7)) 
            {
                *frame_size = NEC_FRAME20MS_FRQ16;
                *n_subframes = NEC_NSF4;
            } 
            else if((*MPE_Configuration>=8)&&(*MPE_Configuration<16)) 
            {
                *frame_size = NEC_FRAME20MS_FRQ16;
                *n_subframes = NEC_NSF8;
            }
            else if((*MPE_Configuration>=16)&&(*MPE_Configuration<23)) 
            {
                *frame_size = NEC_FRAME10MS_FRQ16;
                *n_subframes = NEC_NSF2;
            }
            else if((*MPE_Configuration>=24)&&(*MPE_Configuration<32)) 
            {
                *frame_size = NEC_FRAME10MS_FRQ16;
                *n_subframes = NEC_NSF4;
            }
            else 
            {
                fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                CommonExit(1,"celp error");
; 
            }
            
            *sbfrm_size = *frame_size/(*n_subframes);
            *lpc_order = NEC_LPC_ORDER_FRQ16;
            *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
            *num_gain_cbks = NEC_NUM_GAIN_CBKS;
            *num_lpc_indices = PAN_NUM_LPC_INDICES_W;
            
            num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES_W
                + (num_enhstages + 1) * (*n_subframes) *
                (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);
            
            switch ( *MPE_Configuration ) 
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
            
            if ( *sbfrm_size == NEC_SBFRM_SIZE40 ) 
            {
                enh_pos_bits = NEC_BIT_ENH_POS40_2;
                enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
            }
            else 
            {
                enh_pos_bits = NEC_BIT_ENH_POS80_4;
                enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
            }
            
            if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
                sizeof(long)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
                exit(3);
            }
            ofa=(*org_frame_bit_allocation);
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
            
            for ( i = 0; i < num_enhstages; i++ ) {
                for ( j = 0; j < *n_subframes; j++ ) {
                    *(*org_frame_bit_allocation+(ctr++)) =  0;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
                    *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
                }
            }
            
            if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
                sizeof(float)))==NULL) {
                fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
                exit(5);
            }
            
            for(i=0;i<(*lpc_order);i++) 
                *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);
            
            /* submodules for initialization */
            PHI_InitLpcAnalysisDecoder(*lpc_order, *lpc_order, InstCtxt->PHI_Priv);
        }
   }
#ifdef VERSION2_EC
   /* Initialize Error Concealment module */
   errConInit( *SampleRateMode );
#endif
}

/*======================================================================*/
/*   Function Definition: celp_close_decoder                            */
/*======================================================================*/
void celp_close_decoder (long ExcitationMode,
                         long BandwidthScalabilityMode,
                         long frame_bit_allocation[],         /* In: bit num. for each index */
                         void **InstanceContext               /* In/Out: handle to instance context */
)
{
    INST_CONTEXT_LPC_DEC_TYPE *InstCtxt;
    PHI_PRIV_TYPE *PHI_Priv;
    
    /* -----------------------------------------------------------------*/
    /* Set up pointers to private data                                  */
    /* -----------------------------------------------------------------*/
    PHI_Priv = ((INST_CONTEXT_LPC_DEC_TYPE *) *InstanceContext)->PHI_Priv;
    
    /* -----------------------------------------------------------------*/
    /*                                                                  */
    /* -----------------------------------------------------------------*/
    
    
    if (ExcitationMode == RegularPulseExc)
    {
        PHI_ClosePostProcessor(PHI_Priv);
        PHI_close_excitation_generation(PHI_Priv);
        PHI_FreeLpcAnalysisDecoder(PHI_Priv);
        PHI_free_bit_allocation(frame_bit_allocation);
    }
    
    if (ExcitationMode == MultiPulseExc)
    {
        free(prev_Qlsp_coefficients);
		free(ofa);
        
        PHI_FreeLpcAnalysisDecoder(PHI_Priv);
        if (BandwidthScalabilityMode == ON)
        {
            free(buf_Qlsp_coefficients_bws);
            free(prev_Qlsp_coefficients_bws);
        }
    }
    
    /* -----------------------------------------------------------------*/
    /* Print Total Frames Processed                                     */
    /* -----------------------------------------------------------------*/ 
    if (CELPdecDebugLevel) {    /* HP 971120 */
        fprintf(stderr,"\n");
        fprintf(stderr,"Total Frames:  %ld \n", frame_ctr);
    }
    
    /* -----------------------------------------------------------------*/
    /* Dispose of private storage for instance context                  */
    /* -----------------------------------------------------------------*/
    InstCtxt = (INST_CONTEXT_LPC_DEC_TYPE *)*InstanceContext;
    free(InstCtxt->PHI_Priv);
    free(InstCtxt);
    *InstanceContext = NULL;
    
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 01-09-96 R. Taori  Initial Version                                   */
/* 18-09-96 R. Taori  Brought in line with MPEG-4 Interface             */
/* 05-05-98 R. Funken Brought in line with MPEG-4 FCD: 3 complexity levels */
/* 21-05-99 T. Mlasko               Version 2 stuff added               */
