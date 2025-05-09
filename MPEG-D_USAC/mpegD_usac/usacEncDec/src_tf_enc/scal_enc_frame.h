/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Olaf Kaehler (Fraunhofer-IIS)

and edited by

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

Copyright (c) 2003.

*/


#ifndef __SCAL_ENC_FRAME_H_INCLUDED_
#define __SCAL_ENC_FRAME_H_INCLUDED_

#include "bitmux.h"
#include "ms.h"
#ifdef I2R_LOSSLESS
#include "aac_qc.h"
#include "lit_ll_en.h"
#endif

int EncTf_aacscal_encode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      *baselayer_spectrum[MAX_TIME_CHANNELS],
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         *sfb_offset,
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_bits[MAX_TF_LAYER],
  int         reservoir_bits,
  int         needed_bits,
  HANDLE_AACBITMUX *bitmux_per_layer,
#ifdef I2R_LOSSLESS
  HANDLE_AACBITMUX *ll_bitmux_per_layer,
  int		  *quant_aac[MAX_TIME_CHANNELS],
  AACQuantInfo quantInfo[MAX_TIME_CHANNELS][MAX_TF_LAYER],
  LIT_LL_Info ll_info[MAX_TIME_CHANNELS], 
  int		  *int_spectral_line_vector[MAX_TIME_CHANNELS],	
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS],  
  int		  osf,
#endif
  int         nr_layer,
  int         core_max_sfb, /* <=0 for no core */
  int         isFirstTfLayer,
  int         hasMonoCore,
  MSInfo      *msInfo,
  int         core_transmitted_tns[],
  WINDOW_SEQUENCE  blockType[MAX_TIME_CHANNELS],
  WINDOW_SHAPE     windowShape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     windowShapePrev[MAX_TIME_CHANNELS],  /* needed for LTP - tool */
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         nr_of_chan[MAX_TF_LAYER],
  long        samplRate,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS* nok_lt_status,
  PRED_TYPE   pred_type,
  int         num_window_groups,
  int         window_group_length[8],
  int         bw_limit[MAX_TF_LAYER],
  enum DC_FLAG force_dc,
  int         debugLevel
  );


#endif
