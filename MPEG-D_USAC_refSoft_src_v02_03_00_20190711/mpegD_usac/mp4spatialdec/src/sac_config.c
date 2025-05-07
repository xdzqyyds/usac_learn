/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#include "sac_config.h"
#include "sac_resdefs.h"
#include "sac_reshuffinit.h"
#include "sac_bitinput.h"
#include "sac_restns.h"



static int
tns_max_bands_tbl[(1<<LEN_SAMP_IDX)][4] = {
    
    {31,  9, 28,  7},	    
    {31,  9, 28,  7},	    
    {34, 10, 27,  7},	    
    {40, 14, 26,  6},	    
    {42, 14, 26,  6},	    
    {51, 14, 26,  6},	    
    {46, 14, 29,  7},	    
    {46, 14, 29,  7},	    
    {42, 14, 23,  8},	    
    {42, 14, 23,  8},	    
    {42, 14, 23,  8},	    
    {39, 14, 19,  7},	    
    {39, 14, 19,  7},       
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};

static int sampling_boundarys[(1<<LEN_SAMP_IDX)] = {
  92017, 75132, 55426, 46009, 37566, 27713, 23004, 18783,
  13856, 11502, 9391, 0, 0, 0, 0, 0
};



MC_Info		s_mc_info;

int s_sampling_rate[(1<<LEN_SAMP_IDX)] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};




int
s_getNextSampFreqIndex(int sampRate)
{
  int i = 0;
  if (sampRate < sampling_boundarys[(1<<LEN_SAMP_IDX)-1]) return -1;
  while (sampRate < sampling_boundarys[i]) { i++; }
  return i;
}


int s_tns_max_bands(int islong)
{
    int i;

    
    i = islong ? 0 : 1;
    i += (s_mc_info.profile ==  SSR_Profile) ? 2 : 0;
    
    return tns_max_bands_tbl[s_mc_info.sampling_rate_idx][i];
}

int
s_tns_max_order(int islong)
{
    if (islong) {
	switch (s_mc_info.profile) {
	case Main_Profile:
#ifdef MPEG4V1
	case LTP_Profile:
#endif
	    return 20;
	case LC_Profile:
	case SSR_Profile:
	    return 12;
	}
    }
    else {
	
	return 7;
    }
    return 0;
}
