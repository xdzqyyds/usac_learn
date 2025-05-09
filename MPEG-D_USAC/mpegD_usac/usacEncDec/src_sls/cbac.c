/*******************************************************************************
This software module was originally developed by

Institute for Infocomm Research and Fraunhofer IIS

and edited by

-

in the course of development of ISO/IEC 14496 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 14496. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 14496 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

#ifdef NOT_PUBLISHED

Assurance that the originally developed software module can be used (1) in
ISO/IEC 14496 once ISO/IEC 14496 has been adopted; and (2) to develop ISO/IEC
14496:
Institute for Infocomm Research and Fraunhofer IIS grant ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 14496 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 14496 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
14496. To the extent that Institute for Infocomm Research and Fraunhofer IIS 
own patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
14496 in a conforming product, Institute for Infocomm Research and Fraunhofer IIS 
will assure the ISO/IEC that they are willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 14496.

#endif

Institute for Infocomm Research and Fraunhofer IIS retain full right to
modify and use the code for their own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2005.
*******************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include "cbac.h"
#include "interface.h"



#define NUM_D2L 6
#define NUM_SS	13
#define NUM_FB	3

static FB_THR fb_thr[(1<<LEN_SAMP_IDX)+1] =
{
  { 8000  },
  { 11025 },
  { 12000 },
  { 16000 },
  { 22050 },
  { 24000 },
  { 32000 },
  { 44100, 186,512 },
  {48000,170,470},
  {96000,85,235},
  {192000,43,118},
  {0, 170, 470}
};

static int bpgc_freq[6] = {8192,8192,5461,3277,964,64};

#define HALF_FREQ 8192

static int cbac_table[NUM_FB][NUM_SS][NUM_D2L] = {
	{{7823, 7826, 6506, 4817, 2186, 1053}, 
	{8344, 7983, 6440, 4202, 1362, 64} ,
	{8399, 8382, 7016, 4202, 1234, 64} ,
	{8305, 7960, 6365, 3963, 1285, 64} ,
	{8335, 8146, 6655, 3746, 825, 64} ,
	{8473, 8244, 6726, 3929, 927, 64} ,
	{8398, 7919, 6098, 3581, 875, 64} ,
	{8359, 8028, 6382, 3459, 631, 64} ,
	{8192, 8192, 5461, 3277, 964, 64},
	{8333, 7481, 5288, 3076, 732, 64} ,
	{7658, 6898, 5145, 1424, 1636, 64} ,
	{5471, 5732, 6264, 4890, 1279, 93} ,
	{8180, 8136, 7897, 5715, 1553, 64} },	
	{{7242, 6876, 6083, 3604, 1214, 950} ,
	{7897, 7570, 6583, 3733, 1067, 900} ,
	{8071, 7928, 7069, 4294, 1406, 1200} ,
	{8197, 7952, 6906, 4050, 1457, 1101} ,
	{8278, 8039, 7094, 4160, 1381, 64} ,
	{8307, 8139, 7263, 4407, 1555, 64} ,
	{8339, 8124, 7065, 4074, 1636, 64} ,
	{8213, 7918, 6827, 3787, 1161, 64} ,
	{8286, 8067, 6902, 3855, 1387, 64} ,
	{8336, 8072, 6705, 3731, 1558, 64} ,
	{7636, 6962, 5036, 1985, 1037, 64} ,
	{5519, 5270, 5238, 4778, 1588, 219} ,
	{7884, 7528, 6743, 4848, 1970, 64} },
	{{6084, 6323, 5929, 3321, 900, 385} ,
	{7862, 7618, 6728, 4409, 1431, 1302}, 
	{8078, 7871, 7081, 5119, 2371, 1670} ,
	{8294, 8046, 7239, 5218, 2032, 967} ,
	{8378, 8119, 7351, 5413, 1947, 64} ,
	{8378, 8207, 7491, 5624, 2444, 64} ,
	{8484, 8302, 7626, 5514, 2021, 64} ,
	{8302, 8006, 7192, 4941, 1561, 64} ,
	{8464, 8246, 7510, 5217, 1780, 64} ,
	{8544, 8442, 7742, 4944, 2010, 64} ,
	{7556, 6771, 4859, 2638, 2155, 64} ,
	{5916, 4780, 4713, 4239, 1240, 182} ,
	{7658, 7095, 5986, 3886, 1394, 64} }
};


int cbac_model(int index,int frame_type, int sampling_rate,int layer, int *p_sig, int core_sig,int int_cx,int osf)
{
	int fb,d2l,ss;
	int i;
	int ss_tab[16] = {0,1,2,3,2,4,5,6,1,7,4,8,3,8,6,9};
	static int init = 0;
	static int samp;
	
	if (!init)
	{
		i = 0;
		while ((fb_thr[i].sampling_rate != sampling_rate)&& (fb_thr[i].sampling_rate !=0))
			i++;
		samp = i;
    init = 1;
	}

	if (int_cx == 2) return 0; /* the null context*/
	
	/*d2l*/
	d2l = layer+1;  /*6*/

	if (d2l < 0) return HALF_FREQ; /* the lazy context*/

	/* ft*/
	if (frame_type == 1) return bpgc_freq[d2l]; /* the bpgc context*/

	/*fb*/
	i = 0;
	if (index < fb_thr[samp].low_thr) fb = 0;
	else if (index < fb_thr[samp].high_thr) fb = 1;
	else fb = 2;

	/*ss*/
	if (p_sig[index] == 0) /*sig bit*/
	{
		if (index == 0)
			ss = p_sig[index+1]*2 + p_sig[index+2];
		else if (index == 1)
			ss = p_sig[index-1]*4 + p_sig[index+1]*2 + p_sig[index+2];
		else if (index == 1024*osf-1)
			ss = p_sig[index-2]*8 + p_sig[index-1]*4;
		else if (index == 1024*osf-2)
			ss = p_sig[index-2]*8 + p_sig[index-1]*4 + p_sig[index+1]*2;
		else
			ss = p_sig[index-2]*8 + p_sig[index-1]*4 + p_sig[index+1]*2 + p_sig[index+2];
		ss = ss_tab[ss]; /* consider the symmetric*/
	}
	else if (core_sig == 0)  /*sig bit, unsig core*/
	{
		ss = 10;
	}
	else /* sig bit, sig core, */
	{
		ss = 11 + int_cx;
	}

	return cbac_table[fb][ss][d2l];
}
