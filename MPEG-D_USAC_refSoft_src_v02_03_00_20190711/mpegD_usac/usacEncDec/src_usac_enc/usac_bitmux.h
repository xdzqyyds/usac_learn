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

$Id: usac_bitmux.h,v 1.7 2011-03-18 21:35:15 mul Exp $
*******************************************************************************/

#ifndef _usac_bitmux_h_
#define _usac_bitmux_h_

#include "block.h"
#include "tf_mainStruct.h"
#include "bitmux.h"
#include "usac_interface.h"

typedef struct USAC_QUANT_INFO UsacQuantInfo;

typedef struct {
  int max_sfb;
  WINDOW_SEQUENCE windowSequence;
  WINDOW_SHAPE window_shape;
  int num_window_groups;
  int window_group_length[8 /*MAX_WINDOW_GROUPS*/];
  int stereo_flag;
  int common_window;
} UsacICSinfo;

typedef struct {
  int common_tw;
  int tw_data_present;
  int tw_ratio[NUM_TW_NODES];
  int tns_data_present;
  int noiseFilling;
  int noiseOffset;
  int noiseLevel;
} UsacToolsInfo;

int usac_write_ics_info (HANDLE_AACBITMUX bitmux,
			 int max_sfb,
			 WINDOW_SEQUENCE windowSequence,
			 WINDOW_SHAPE window_shape,
			 int num_window_groups,
			 const int window_group_length[]);
int usac_write_tw_data(HANDLE_AACBITMUX bitmux,
                       int              tw_data_present,
                       int              tw_ratio[]);
int usac_write_sce(HANDLE_AACBITMUX bitmux,
                   int core_mode,
                   int tns_data_present);
int usac_write_cpe(HANDLE_AACBITMUX bitmux,
		   USAC_CORE_MODE core_mode[MAX_TIME_CHANNELS],
           int *tns_data_present,
           int predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
           const int huff[13][1090][4],
		   int common_window,
		   int common_tw,
		   int max_sfb,
		   WINDOW_SEQUENCE windowSequence,
		   WINDOW_SHAPE window_shape,
		   int num_window_groups,
		   int window_group_length[],
		   int ms_mask,
		   int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
		   int flag_twMdct,
		   UsacToolsInfo *toolsInfo,
		   int const bUsacIndependenceFlag);
int usac_fd_cs(
  HANDLE_AACBITMUX bitmux,
  WINDOW_SEQUENCE windowSequence,
  WINDOW_SHAPE windowShape,
  int global_gain,
  const int huff[13][1090][4],
  int max_sfb,
  int nr_of_sfb,
  int num_window_groups,
  const int window_group_length[],
  const int noise_nrg[],
  UsacICSinfo *ics_info,
  UsacToolsInfo *tool_data,
  TNS_INFO      *tnsInfo,
  UsacQuantInfo *qInfo,
  int common_window,
  int common_tw,
  int flag_twMdct,
  int flag_noiseFilling,
  short *facData,
  int   nb_bits_fac,
  int bUsacIndependencyFlag,
  int qdebug);

int usac_write_fillElem(HANDLE_BSBITSTREAM bs_padding,
                        int fillBits);

#endif   /* define _usac_bitmux_ */
