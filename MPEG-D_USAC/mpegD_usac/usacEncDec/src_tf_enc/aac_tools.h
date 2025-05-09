/**********************************************************************
MPEG-4 Audio VM 



This software module was originally developed by 
Olaf Kaehler (Fraunhofer IIS-A) 

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

**********************************************************************/

#ifndef _INCLUDED_AAC_TOOLS_H_
#define _INCLUDED_AAC_TOOLS_H_

#include "allHandles.h"
#include "tns3.h"
#include "ms.h"
#include "tf_mainStruct.h"       /* structs */
#include "tf_main.h"
#include "sony_local.h"
#include "bitmux.h"

typedef struct {
  /* energies of each scalefactor band */
  int  pns_sfb_nrg[SFB_NUM_MAX];
  /* PNS on/off flag for each scalefactor band */
  int  pns_sfb_flag[SFB_NUM_MAX];
} PnsInfo;


void bandwidth_limit_spectrum(
  const double *orig_spectrum,
  double *out_spectrum,
  WINDOW_SEQUENCE windowSequence,
  int    block_size_samples,
  int    sampling_rate,
  int    bw_limit);

void tf_apply_pns(
  double      *spectrum,
  double      energy[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],
  int         nr_of_sfb,
  int         pns_sfb_start,
  WINDOW_SEQUENCE windowSequence,
  PnsInfo     *pnsInfo);

int EncTf_aacplain_encode(
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         bits_available,
  int         bits_needed,
  HANDLE_AACBITMUX bitmux,
  int         ep_syntax,
  int         ld_mode,
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS],
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         nr_of_chan,
  long        samplRate,
  MSInfo      *msInfo,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  int         pns_sfb_start,
  GC_DATA_SWITCH gc_switch,
  int         max_band[MAX_TIME_CHANNELS],
  GAINC       **gainInfo[MAX_TIME_CHANNELS],
  int         num_window_groups,
  int         window_group_length[8],
  int         bw_limit,
  int         debugLevel
);

#endif
