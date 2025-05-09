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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"

#include "tns3.h"                /* structs */

#include "sam_encode.h"

#ifdef __cplusplus
extern "C" {
#endif

static int my_log2(int value);

#ifdef __cplusplus
}
#endif

int sam_encode_data(
		int bitrate,
		WINDOW_SEQUENCE	windowSequence,
		int scaleFactors[][MAX_SCFAC_BANDS],
		int	num_window_groups,
		int window_group_length[],
		int quant[][1024],
		int maxSfb,
		int minSfb,
		int sfb_offset[],
		int nr_of_sfs,
		int	stereo_mode,
		int	stereo_info[],
		int pns_sfb_start,
		int pns_sfb_flag[][SFB_NUM_MAX],
		int pns_sfb_nrg[][SFB_NUM_MAX],
		int pns_sfb_mode[],
		int block_size_samples,
		int	ubits,
		int abits,
		int i_ch,
		int nch,
		int w_flag)
{
	int		i, j, k, ch;
	int		b, w, g, s;
	int		qband, cband, sfb;
	int		frame_length;
	int		nr_of_sfb;
	int		max;
	int		end_cband[8];
	int		start_i, end_i;
	int		ModelIndex[2][8][32];
	int		scf[2][8][MAX_SCFAC_BANDS];
	int		swb_offset[8][MAX_SCFAC_BANDS];
	int		sample_buf[2][1024];
	int 	*sample[2][8];

	for(ch = i_ch; ch < nch; ch++) {

		if(windowSequence == EIGHT_SHORT_SEQUENCE) {
			int short_block_size=block_size_samples/8;

			s = 0;
			nr_of_sfb = nr_of_sfs / num_window_groups;
			for(g = 0; g < num_window_groups; g++) { 
				sample[ch][g] = &(sample_buf[ch][short_block_size * s]);
				s += window_group_length[g];

				swb_offset[g][0] = 0;
				for(sfb = 0; sfb < nr_of_sfb; sfb++)
					swb_offset[g][sfb+1] = sfb_offset[sfb+1] * window_group_length[g];
				end_cband[g] = (swb_offset[g][maxSfb]+31)/32;
			}

			/* reshuffling */
       		for(g=0, b=0; g<num_window_groups; g++)  
       		for(w=0; w<window_group_length[g]; w++, b++)  
    		for (i=0; i<short_block_size; i+=4) 
			for (k=0; k<4; k++) 
				sample[ch][g][i*window_group_length[g]+4*w+k] = quant[ch][short_block_size*b+i+k];

		} else {

			nr_of_sfb = nr_of_sfs;
			num_window_groups = 1;
			swb_offset[0][0] = 0;
			for(sfb = 0; sfb < nr_of_sfb; sfb++)
				swb_offset[0][sfb+1] = sfb_offset[sfb+1];

			sample[ch][0] = &(quant[ch][0]);

			end_cband[0] = (swb_offset[0][maxSfb]+31)/32;
		}

		for (g = 0; g < num_window_groups; g++) 
		for (cband = 0; cband < 32; cband++) 
			ModelIndex[ch][g][cband] = 0;

       	for(g=0; g<num_window_groups; g++) { 
			int bal;

			for (cband = 0; cband < end_cband[g]; cband++) {
				start_i = cband * 32;
				end_i =   (cband+1) * 32;
				max = 0;
				for (i = start_i; i < end_i; i++) {
					if (abs(sample[ch][g][i])>max) 
						max = abs(sample[ch][g][i]);
				}
				if(max > 0) bal = my_log2(max);
				else		bal = 0;
				ModelIndex[ch][g][cband] = bal * 2  - 1;
				if (ModelIndex[ch][g][cband] < 0) 
					ModelIndex[ch][g][cband] = 0; 
			}
		}

		for (g = 0; g < num_window_groups; g++) {
			for (qband = 0; qband < maxSfb; qband++) {
				scf[ch][g][qband] = scaleFactors[ch][(g*nr_of_sfb)+qband];
			}
		}

	}


	/********** B I T S T R E A M   M A K I N G *********/
	frame_length = sam_encode_bsac(bitrate, windowSequence, sample,
			scf, maxSfb, minSfb, num_window_groups, window_group_length, swb_offset, ModelIndex,
			stereo_mode, stereo_info, pns_sfb_start, pns_sfb_flag, pns_sfb_nrg,
			pns_sfb_mode, block_size_samples, ubits, abits, i_ch, nch, w_flag);


	return frame_length;
}


static int my_log2(int value)
{
	int	i, step;

	if(value < 0) {
		fprintf(stderr, "my_log2:error : %d\n", value);
		return 0;
	}
	if(value == 0)	return 0;

	step = 2;
	for(i = 1; i < 24; i++) {
		if(value < step) return i;
		step *= 2;
	}
	return ( 1 + (int)(log10((double)value)/log10(2.0)));
}
