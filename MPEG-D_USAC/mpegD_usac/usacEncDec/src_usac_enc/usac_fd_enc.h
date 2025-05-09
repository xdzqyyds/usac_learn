/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Fraunhofer IIS

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.

*******************************************************************************/

#ifndef _INCLUDED_USAC_FD_ENC_H_
#define _INCLUDED_USAC_FD_ENC_H_

#include "allHandles.h"
#include "tns3.h"
#include "ms.h"
#include "tf_mainStruct.h"       /* structs */
#include "tf_main.h"
#include "sony_local.h"
#include "bitmux.h"
#include "usac_bitmux.h"
#include "usac_fd_qc.h"
#include "usac_mainStruct.h"
#include "int_dec.h"

void usac_bandwidth_limit_spectrum(double *orig_spectrum,
				   double *out_spectrum,
				   WINDOW_SEQUENCE windowSequence,
				   int    block_size_samples,
				   int    sampling_rate,
				   int    bw_limit,
				   int    sfb_offset[MAX_SCFAC_BANDS+1],
				   int    nr_of_sfb,
				   int    *max_sfb);

int usac_fd_encode(
  double       *DTimeSigBuf[MAX_TIME_CHANNELS],
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  HANDLE_USAC_TD_ENCODER tdenc[MAX_TIME_CHANNELS],
  HANDLE_TFDEC hTfDEC,
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  int         max_sfb[MAX_TIME_CHANNELS],
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
  int         blockSizeSamples[MAX_TIME_CHANNELS],
  int         nr_of_chan,
  long        samplRate,
  MSInfo      *msInfo,
  int         predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  UsacICSinfo ics_info[MAX_TIME_CHANNELS],
  UsacToolsInfo tool_info[MAX_TIME_CHANNELS],
  UsacQuantInfo qInfo[MAX_TIME_CHANNELS],
  int         num_window_groups[MAX_TIME_CHANNELS],
  int         window_group_length[MAX_TIME_CHANNELS][8],
  int         bw_limit,
  int         flag_noiseFilling,
  int         flag_twMdct,
  int         commonWindow,
  short        serialFac[MAX_TIME_CHANNELS][NBITS_MAX],
  int   *nBitsFac,
  USAC_CORE_MODE coreMode[MAX_TIME_CHANNELS],
  USAC_CORE_MODE   prev_coreMode[MAX_TIME_CHANNELS],
  USAC_CORE_MODE   next_coreMode[MAX_TIME_CHANNELS],
  int         bUsacIndependencyFlag,
  int         debugLevel
		   );


#endif
