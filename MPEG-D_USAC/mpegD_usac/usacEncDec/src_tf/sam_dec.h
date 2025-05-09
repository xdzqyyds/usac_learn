/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-29

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

Copyright (c) 1999.

**********************************************************************/

#ifndef _sam_dec_h_
#define _sam_dec_h_

#include "allHandles.h"
#include "all.h"/*20070107-2*/
#include "block.h"

#include "obj_descr.h"           /* struct */
#include "tf_mainStruct.h"       /* struct */
#include "sam_tns.h"             /* struct */


/* 20060107 */
#include "ct_sbrdecoder.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {	/* SAMSUNG_2005-09-30 : channel speaker type */
	CENTER_FRONT,		/* 1 */
	LR_FRONT,				/* 2 */
	REAR_SUR,				/* 1 */
	LR_SUR,					/* 2 */
	LFE,						/* 1 */
	LR_OUTSIDE,			/* 2 */	
	RESERVED 	
}CH_TYPE ;


  void bsacAUDecode ( int          numChannels,
                      FRAME_DATA*  fd,
                      TF_DATA*     tfData,
                      int          target,
                      int						*numOutChannels
                      /* 20060107 */
                      ,int	*flagBSAC_SBR
                      ,int	*core_bandwidth
                      ); /* SAMSUNG_2005-09-30 */

 void sam_FlexMuxDecode_bsac( HANDLE_BSBITSTREAM fixed_stream,
                             WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
                             int*               flexMuxBits,
                             double*            spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                             int                numChannels,
                             WINDOW_SHAPE*      windowShape,
                             int                bsacDecBr,
                             FRAME_DATA*        fd
);
  /* --- Init BSAC-decoder --- */
  void sam_decode_init(int sampling_rate_decoded, int block_size_samples);
  void sam_scale_bits_init(int sampling_rate_decoded, int block_size_samples);

  /* ----- Decode one frame with the BSAC-decoder  ----- */
  int     sam_decode_frame(HANDLE_BSBITSTREAM fixed_stream,
                           int     target_br,
                           double  **coef,
                           WINDOW_SEQUENCE     *windowSequence,
                           WINDOW_SHAPE *window_Shape,
                           int     nch
                           /* 20060107 */
                           ,int	*flagBSAC_SBR
                           ,SBRBITSTREAM *ct_sbrBitStr
                           ,int	*core_bandwidth
                           );

  void sam_decode_bsac_data ( int    target,
                              int    frameLength,
                              int    sba_mode,
                              int    top_layer,
                              int    base_snf_thr,
                              int    base_band,
                              int    maxSfb,
                              WINDOW_SEQUENCE  windowSequence,
                              int    num_window_groups,
                              int    window_group_length[8],

                              int    max_scalefactor[],
                              int    cband_si_type[],
                              int    scf_model0[],
                              int    scf_model1[],
                              int    max_sfb_si_len[],

                              int    stereo_mode,
                              int    ms_mask[],
                              int    is_info[],
                              int    pns_data_present,
                              int    pns_sfb_start,
                              int    pns_sfb_flag[][MAX_SCFAC_BANDS],
                              int    pns_sfb_mode[MAX_SCFAC_BANDS],
                              int    used_bits,
                              int    samples[][1024],
                              int    scalefactors[][MAX_SCFAC_BANDS],
                              int    nch
                              /* 20060107 */
                              ,int	*core_bandwidth
                              );

  void sam_decode_bsac_stream(int    target,
                              int    frameLength,
                              int    sba_mode,
                              int    enc_top_layer,
                              int    base_snf,
                              int    base_band,
                              int    maxSfb,
                              WINDOW_SEQUENCE  windowSequence,
                              int    num_window_groups,
                              int    window_group_length[8],

                              int    max_scalefactor[],
                              int    cband_si_type[],
                              int    scf_model0[],
                              int    scf_model1[],
                              int    max_sfb_si_len[],

                              int    stereo_mode,
                              int    ms_mask[],
                              int    is_info[],
                              int    pns_data_present,
                              int    pns_sfb_start,
                              int    pns_sfb_flag[][MAX_SCFAC_BANDS],
                              int    pns_sfb_mode[MAX_SCFAC_BANDS],
                              int    swb_offset[][52],
                              int    top_band,
                              int    used_bits,
                              int    samples[][1024],
                              int    scalefactors[][MAX_SCFAC_BANDS],
                              int    nch
                              /* 20060107 */
                              ,int	*core_bandwidth
                              );

  void sam_dequantization(int target_br,
                          int windowSequence,
                          int scalefactors[MAX_SCFAC_BANDS],
                          int num_window_groups,
                          int window_group_length[8],
                          int samples[],
                          int maxSfb,
                          int is_info[],
                          double spectrums[],
                          int ch);

  void sam_intensity(int   windowSequence,
                     int   num_window_groups,
                     int   window_group_length[8],
                     double spectrum[][1024],
                     int   factors[][MAX_SCFAC_BANDS],
                     int   is_info[], 
                     int    maxSfb);

  void sam_ms_stereo(int windowSequence,
                     int num_window_groups,
                     int window_group_length[8],
                     double spectrum[][1024],
                     int ms_mask[],
                     int maxSfb);

  void sam_pns ( int maxSfb,
								 int windowSequence, /* SAMSUNG_2005-09-30 */
                 int num_window_groups, /* SAMSUNG_2005-09-30 */
                 int window_group_length[8], /* SAMSUNG_2005-09-30 */
                 int pns_sfb_flag[][MAX_SCFAC_BANDS],
                 int pns_sfb_mode[MAX_SCFAC_BANDS],
                 double spectrum[][1024],
                 int scalefactors[][MAX_SCFAC_BANDS],
                 int nch);

  void sam_tns_data(int windowSequence, int maxSfb, double spect[1024], sam_TNS_frame_info *tns);
  int sam_tns_top_band(int windowSequence);
  int sam_tns_max_bands(int windowSequence);
  int sam_tns_max_order(int windowSequence);
  void sam_tns_decode_subblock(int windowSequence, double *spec, int maxSfb, int *sfbs, sam_TNSinfo *tns_info);

  void sam_initArDecode(int);
  void sam_setArDecode(int);
  void sam_storeArDecode(int);

  void sam_start_decoding(int , int);
  int sam_decode_symbol(int *, int *);
  int sam_decode_symbol2(int, int *);

  void sam_init_layer_buf(void);
  void sam_setRBitBufPos(int);
  int sam_getRBitBufPos(void);
  int sam_putbits2buf(unsigned, int);
  unsigned sam_getbitsfrombuf(int);

  void sam_setBsacdec2BitBuffer(HANDLE_BSBITSTREAM fixed_stream);
  long sam_BsNumByte( void );
  long sam_GetBits(int n);
  int sam_getUsedBits(void);
  void sam_SetBitstreamBufPos(int n);

static void sam_get_tns( int windowSequence, sam_TNS_frame_info *tns_frame_info );

  int sam_decode_element(HANDLE_BSBITSTREAM   fixed_stream,
                         int           target_br,
                         double**      coef,
                         WINDOW_SEQUENCE* window_Sequence,
                         WINDOW_SHAPE* window_Shape,
                         int *numChannels,
                         int ext_flag, 
                         int remain_tot_bits
                         /* 20060107 */
                         ,int	*core_bandwidth,
                         int *frameLength
                         );
  
  void sam_SetBitstreamBufPos(int n);													

#ifdef __cplusplus
           }
#endif

#endif /* #ifndef       _sam_dec_h_ */


/* 20060107 */
int
getExtensionPayload_BSAC( 
#ifdef CT_SBR
        SBRBITSTREAM *ct_sbrBitStr,
#endif
#ifdef DRC
        HANDLE_DRC drc,
#endif
		/* 20070326 BSAC Ext.*/
		int ext_type,
        int *data_available);
