/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-11-06

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

Copyright (c) 1997.

**********************************************************************/

#ifndef	_sam_enc_h_
#define	_sam_enc_h_

#include "allHandles.h"
#include "block.h"
#include "tf_mainHandle.h"
#include "util.h"
#include "common_m4a.h"
#include "flex_mux.h"

#include "nok_ltp_enc.h"
#include "ms.h"
#include "enc_tf.h" 
#include "aac_qc.h"

#ifdef __cplusplus
extern "C" {
#endif


void sam_init_encode_bsac(
		int numChannel,
		int sampling_rate,
		int bit_rate,
		int block_size_samples);

int sam_encode_frame(
		int num_of_chan,
		int ch_type, /* SAMSUNG_2005-09-30 */
		WINDOW_SEQUENCE windowSequence,
		WINDOW_SHAPE	window_shape,
		int sfb_offset[],
		int nr_of_sfs,
		int num_window_groups,
		int window_group_length[],
		int	quant[][1024],
		int	scfacs[][MAX_SCFAC_BANDS],
		MSInfo      *msInfo, /* SAMSUNG_2005-09-30 */
		HANDLE_BSBITSTREAM fixed_stream,
		int i_ch,
		/*int w_flag,*/
		int avr_bits,
		int mc_ext_size, /* SAMSUNG_2005-09-30 */
		int mc_enabled, /* SAMSUNG_2005-09-30 */
/* 20070530 SBR */
		int sbrenable,
		SBR_CHAN *sbrChan,
		int numEn,
		 int i_el
		); /* SAMSUNG_2005-09-30 */


void sam_scale_bits_init_enc(
		int sampling_rate,
		int block_size);



/* SAMSUNG_2005-09-30 */
int EncTf_bsac_encode(
	HANDLE_CHANNEL_ELEMENT_DATA chData,
	int					numElm,
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         bits_available,  
	int					frameNumBit,
	HANDLE_BSBITSTREAM *output_au,  
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS],
  /*int         aacAllowScalefacs,*/
  int         blockSizeSamples,
  int         nr_of_chan,
  long        samplRate,
  MSInfo      *msInfo,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],  
  int         pns_sfb_start,    
  int         num_window_groups,
  int         window_group_length[8],
  int         bw_limit,
  int         debugLevel,
  int         lfePresent,
/* 20070530 SBR */
  int      sbrenable,
  SBR_CHAN *sbrChan
);
/* ~SAMSUNG_2005-09-30 */


int sam_FlexMuxEncode_bsac(
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   WINDOW_SEQUENCE  windowSequence[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         available_bitreservoir_bits,
			   int         padding_limit,
			   HANDLE_BSBITSTREAM fixed_stream,
			   HANDLE_BSBITSTREAM var_stream,
			   HANDLE_BSBITBUFFER *gcBitBuf,
			   int         nr_of_chan,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int         useShortWindows,
			   WINDOW_SHAPE     window_shape[MAX_TIME_CHANNELS],  
			   int aacAllowScalefacs,
               long        sampling_rate,
			   QC_MOD_SELECT qc_select,
			   PRED_TYPE predictor_type,
			   NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
               int blockSizeSamples,
               int num_window_groups,
               int window_group_length[],
               TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
               int bandwidth,
               int numES
); 

int sam_encode_bsac(
        int target_bitrate,
				int ch_type, /* SAMSUNG_2005-09-30 : channel type */
        int windowSequence,
        int windowShape,
        int *sample[][8],
        int scalefactors[][8][MAX_SCFAC_BANDS],
        int maxSfb,
        int num_window_groups,
        int window_group_length[],
        int swb_offset[][MAX_SCFAC_BANDS],
        int top_band,
        int ModelIndex[][8][32],
        int stereo_mode,
        /*int stereo_info_buf[],*/
				int stereo_info[][MAX_SCFAC_BANDS],
        int abits,
        int i_ch,
        int nch,
				int mc_enabled, /* SAMSUNG_2005-09-30 */
/* 20070530 SBR */
					int sbrenable,
					SBR_CHAN *sbrChan,
					int			n_Elm,
					int			i_el,
					int *bitCount
				); /* SAMSUNG_2005-09-30 */

/* 20070530 SBR */
int WriteSBRinBSAC(SBR_CHAN *sbrChan, int nch);

void sam_initArEncode(int arEnc_i);

void sam_storeArEncode(int arEnc_i);

void sam_setArEncode(int arEnc_i);

int sam_encode_symbol( int symbol, int armodel[]);
int sam_encode_symbol2( int symbol, int freq);

int sam_done_encoding(void);

void sam_init_bs(void);
int  sam_putbits2bs(int val, int N);
int  sam_sstell(void);
int  sam_getWBitBufPos(void);
void sam_setWBitBufPos(int);
void sam_frame_length_rewrite2bs(int framesize, int pos, int len);
void sam_bsflush(HANDLE_BSBITSTREAM fixed_steam, int len);
/* 20070530 SBR */
void sam_bsflushAll(HANDLE_BSBITSTREAM fixed_stream, int frameSize);

void sam_FlexMuxEncInit_bsac( 
  HANDLE_BSBITSTREAM headerStream,
  int         numChannel,
  long        samplRate,
  ENC_FRAME_DATA * frameData,
  int numES,
  int mainDebugLevel
);

int sam_tf_quantize_spectrum( 	
  int quant[][NUM_COEFF],
  int scale_factor[][SFB_NUM_MAX],  
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE windowShape,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],   /* out: grouped sfb offsets */
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
  MSInfo      *msInfo, 
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  int   window_group_length[],
  int	i_ch
);

int sam_putbits2bs_ow(int val, int N);

/* 20070530 SBR */
int WriteSbrSCEinBSAC(SBR_CHAN *sbrChan);
int WriteSbrCPEinBSAC(SBR_CHAN *sbrChan);
#ifdef __cplusplus
}
#endif

#endif /* #ifndef	_sam_enc_h_ */
