
/*

This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
This software module is an implementation of a part of one or more
MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
standard (ISO/IEC 14496-3).
ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
free license to this software module or modifications thereof for use
in hardware or software products claiming conformance to the MPEG-4
Audio standards (ISO/IEC 14496-3).
Those intending to use this software module in hardware or software
products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company,
the subsequent editors and their companies, and ISO/IEC have no
liability for use of this software module or modifications thereof in
an implementation.
Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
conforming products. The original developer retains full right to use
the code for his/her own purpose, assign or donate the code to a third
party and to inhibit third party from using the code for non MPEG-4
Audio (ISO/IEC 14496-3) conforming products.
This copyright notice must be included in all copies or derivative works.

Copyright (c)1997.

*/

/**********************************************************************
Dummy version of t/f scalable celp functions

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
20-nov-97   HP    born, based on celp_encoder.h and celp_decoder.h
**********************************************************************/

#include <stdio.h>

#include "common_m4a.h"


#ifdef __cplusplus
extern "C" {
#endif


#include "lpc_common.h"
     
/*======================================================================*/
/* Function prototype: celp_coder                                       */
/*======================================================================*/
void celp_coder
(
float     **InputSignal,           /* In: Multichannel Speech           */
HANDLE_BSBITSTREAM  p_bitstream,           /* Out: Bitstream                    */
long        sampling_frequency,    /* In:  Sampling Frequency           */
long        bit_rate,              /* In:  Bit rate                     */
long        SampleRateMode,
long        QuantizationMode,      /* In: Type of Quantization  	    */
long        FineRateControl,	   /* In: Fine Rate Control switch      */
long        LosslessCodingMode,    /* In: Lossless Coding Mode  	    */   
long        RPE_configuration,      /* In: RPE_configuration 	    */
long        Wideband_VQ,		   /* In: Wideband VQ mode			    */
long        NB_Configuration,      /* In: Narrowband configuration      */
long        NumEnhLayers,  	       /* In: Number of Enhancement Layers  */
long        BandwidthScalabilityMode, /* In: bandwidth switch           */
long        BWS_configuration,     /* In: BWS_configuration 		    */
long        PreProcessingSW,       /* In: PreProcessingSW	    */
long        frame_size,            /* In:  Frame size                   */
long        n_subframes,           /* In:  Number of subframes          */
long        sbfrm_size,            /* In:  Subframe size                */
long        lpc_order,             /* In:  Order of LPc                 */
long        num_lpc_indices,       /* In:  Number of LPC indices        */
long        num_shape_cbks,        /* In:  Number of Shape Codebooks    */
long        num_gain_cbks,         /* In:  Number of Gain Codebooks     */
long        n_lpc_analysis,        /* In:  Number of LPCs per frame     */
long        window_offsets[],      /* In:  Offset for LPC-frame v.window*/
long        window_sizes[],        /* In:  LPC Analysis Window size     */
long        max_n_lag_candidates,  /* In:  Maximum search candidates    */
float       min_pitch_frequency,   /* IN:  Min Pitch Frequency          */
float       max_pitch_frequency,   /* IN:  Max Pitch Frequency          */
long        org_frame_bit_allocation[]/* In: Frame BIt alolocation      */
)
{
  CommonExit(1,"celp_coder: dummy");
}

/*======================================================================*/
/*   Function Prototype:celp_initialisation_encoder                     */
/*======================================================================*/
void celp_initialisation_encoder
(
HANDLE_BSBITSTREAM  const p_bitstream,  /* In/Out: Bitstream                   */
long	 bit_rate,  	         /* In: bit rate                        */
long	 sampling_frequency,     /* In: sampling frequency              */
long     SampleRateMode,	     /* In: SampleRate Mode 		*/
long     QuantizationMode,       /* In: Type of Quantization		*/
long     FineRateControl,	     /* In: Fine Rate Control switch	 */
long     LosslessCodingMode,     /* In: Lossless Coding Mode	*/
long     *RPE_configuration,      /* In: RPE_configuration 	*/
long     Wideband_VQ,		     /* Out: Wideband VQ mode			*/
long     *NB_Configuration,      /* Out: Narrowband configuration   */
long     NumEnhLayers,  	     /* In: Number of Enhancement Layers*/
long     BandwidthScalabilityMode, /* In: bandwidth switch   */
long     *BWS_configuration,     /* Out: BWS_configuration 		*/
long     BWS_nb_bitrate,     /* In: NB bitrate in BWS_mode 		*/
long	 *frame_size,	         /* Out: frame size                     */
long	 *n_subframes,           /* Out: number of  subframes           */
long	 *sbfrm_size,	         /* Out: subframe size                  */ 
long	 *lpc_order,	         /* Out: LP analysis order              */
long	 *num_lpc_indices,       /* Out: number of LPC indices          */
long	 *num_shape_cbks,	     /* Out: number of Shape Codebooks      */
long	 *num_gain_cbks,	     /* Out: number of Gain Codebooks       */ 
long	 *n_lpc_analysis,	     /* Out: number of LP analysis per frame*/
long	 **window_offsets,       /* Out: window offset for each LP ana  */
long	 **window_sizes,         /* Out: window size for each LP ana    */
long	 *n_lag_candidates,      /* Out: number of pitch candidates     */
float	 *min_pitch_frequency,   /* Out: minimum pitch frequency        */
float	 *max_pitch_frequency,   /* Out: maximum pitch frequency        */
long	 **org_frame_bit_allocation/* Out: bit num. for each index      */
)
{
  CommonExit(1,"celp_initialisation_encoder: dummy");
}

/*======================================================================*/
/*   Function  Prototype: celp_close_encoder                            */
/*======================================================================*/
void celp_close_encoder
(
	long SampleRateMode,
	long BandwidthScalabilityMode,
    long sbfrm_size,              /* In: subframe size                  */
    long frame_bit_allocation[],  /* In: bit num. for each index        */
    long window_offsets[],        /* In: window offset for each LP ana  */
    long window_sizes[],          /* In: window size for each LP ana    */
    long n_lpc_analysis           /* In: number of LP analysis/frame    */
)
{
  CommonExit(1,"celp_close_encoder: dummy");
}

/*======================================================================*/
/* Function Prototype: celp_decoder                                     */
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
                   long        frame_size,                  /* In:  Frame size                   */
                   long        n_subframes,                 /* In:  Number of subframes          */
                   long        sbfrm_size,                  /* In:  Subframe size                */
                   long        lpc_order,                   /* In:  Order of LPC                 */
                   long        num_lpc_indices,             /* In:  Number of LPC indices        */
                   long        num_shape_cbks,              /* In:  Number of Shape Codebooks    */
                   long        num_gain_cbks,               /* In:  Number of Gain Codebooks     */
                   long        *org_frame_bit_allocation,   /* In: bit num. for each index       */
                   void        *InstanceContext,            /* In: pointer to instance context   */
                   int         mp4ffFlag
)
{
  CommonExit(1,"celp_decoder: dummy");
}

/*======================================================================*/
/*   Function Prototype: celp_initialisation_decoder                    */
/*======================================================================*/
void celp_initialisation_decoder
(
HANDLE_BSBITSTREAM  p_bitstream,                 /* In: Bitstream               */
long     bit_rate,		                 /* in: bit rate */
long     complexity_level,               /* In: complexity level decoder*/
long     reduced_order,                  /* In: reduced order decoder   */
long     DecEnhStage,
long     DecBwsMode,
long     PostFilterSW,
long     *frame_size,                    /* Out: frame size             */
long     *n_subframes,                   /* Out: number of  subframes   */
long     *sbfrm_size,                    /* Out: subframe size          */ 
long     *lpc_order,                     /* Out: LP analysis order      */
long     *num_lpc_indices,               /* Out: number of LPC indices  */
long     *num_shape_cbks,                /* Out: number of Shape Codeb. */    
long     *num_gain_cbks,                 /* Out: number of Gain Codeb.  */    
long     **org_frame_bit_allocation,     /* Out: bit num. for each index*/
long     * SampleRateMode,               /* Out: SampleRate Mode	    */
long     * QuantizationMode,             /* Out: Type of Quantization	*/
long     * FineRateControl,              /* Out: Fine Rate Control switch*/
long     * LosslessCodingMode,           /* Out: Lossless Coding Mode  	*/
long     * RPE_configuration,             /* Out: RPE_configuration */
long     * Wideband_VQ, 	             /* Out: Wideband VQ mode		*/
long     * NB_Configuration,             /* Out: Narrowband configuration*/
long     * NumEnhLayers,	             /* Out: Number of Enhancement Layers*/
long     * BandwidthScalabilityMode,     /* Out: bandwidth switch	    */
long     * BWS_configuration             /* Out: BWS_configuration		*/
)
{
  CommonExit(1,"celp_initialisation_decoder: dummy");
}

/*======================================================================*/
/*   Function Prototype: celp_close_decoder                             */
/*======================================================================*/
void celp_close_decoder
(
   long ExcitationMode,
   long BandwidthScalabilityMode,
   long frame_bit_allocation[],         /* In: bit num. for each index */
   void **InstanceContext               /* In/Out: handle to instance context */
)
{
  CommonExit(1,"celp_close_decoder: dummy");
}

#ifdef __cplusplus
}
#endif

/* end of tf_celp_dmy.c */
