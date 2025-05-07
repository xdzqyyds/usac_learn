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

#ifndef _INCLUDED_MS_H_
#define _INCLUDED_MS_H_

#include "tf_mainStruct.h"

typedef struct {
  int ms_mask;
  int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX];
} MSInfo;
  
void MSInverse(int num_sfb,
  int *sfb_offset,
  int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  int num_windows,
  int blockSizeSamples,
  double *left,
  double *right);

void MSApply(int num_sfb,
  int *sfb_offset,
  int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  int num_windows,
  int blockSizeSamples,
  double *left,
  double *right);

void select_ms(
  double *spectral_line_vector[MAX_TIME_CHANNELS],
  int         blockSizeSamples,
  int         num_window_groups,
  const int   window_group_length[MAX_SHORT_WINDOWS],
  const int   sfb_offset[MAX_SCFAC_BANDS+1],
  int         start_sfb,
  int         end_sfb,
  MSInfo      *msInfo,
  int         ms_select,
  int         debugLevel);

#endif
