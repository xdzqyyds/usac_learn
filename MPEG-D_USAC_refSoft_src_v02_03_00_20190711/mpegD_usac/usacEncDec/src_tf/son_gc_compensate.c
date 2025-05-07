/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by

Takashi Koike (Sony Corporation)

in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
software module is an implementation of a part of one or more MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4
System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
notice must be included in all copies or derivative works.

Copyright (C) 1996.
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "allHandles.h"
#include "tf_mainHandle.h"

#include "common_m4a.h"
#include "sony_local.h"

static void son_gc_compensate_sub(
	double	input[],
	GAINC	*gain_data[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	int	band,
	double	*gcOverlapBufferCh[],
	double	*out[]
	)
{
	int	i,j,k;

	/* These parameter will be removed outside */
	static 	GAINC	gain_data_prev[MAX_TIME_CHANNELS][NBANDS];
	static	int init_flag = 0;

	double	*a_gcwind;
	a_gcwind = (double *)calloc(block_size_samples/NBANDS*2,
					sizeof(double));
	if(init_flag == 0){
		for(i = 0; i < NBANDS ; i++){
			gain_data_prev[ch][i].natks = 0;
		}
		init_flag = 1;
	}
	
	switch(window_sequence){
	case ONLY_LONG_SEQUENCE:
		/* Gain function making routine */
		son_gainc_window(block_size_samples/NBANDS*2,
				gain_data_prev[ch][band],
				gain_data[band][0], gain_data[band][0],
				a_gcwind, ONLY_LONG_SEQUENCE);
		/* Gain compensating routine */
		for( j = 0; j < block_size_samples/NBANDS*2; j++){
			input[band*block_size_samples/NBANDS*2+j] *=a_gcwind[j];
		}
		/* Overlapping routine */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBufferCh[band][j]+input[band*block_size_samples/NBANDS*2+j];
		}
		/* Shift Previous data */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBufferCh[band][j] =
					input[band*block_size_samples/NBANDS*2+block_size_samples/NBANDS+j];
		}
		gain_data_prev[ch][band] = gain_data[band][0];
		break;
	case LONG_START_SEQUENCE:
		/* Gain function making routine */
		son_gainc_window(block_size_samples/NBANDS*2,
				gain_data_prev[ch][band],
				gain_data[band][0], gain_data[band][1],
				a_gcwind, LONG_START_SEQUENCE);
		/* Gain compensating routine */
		for( j = 0; j < block_size_samples/NBANDS*2; j++){
			input[band*block_size_samples/NBANDS*2+j] *=a_gcwind[j];
		}
		/* Overlapping routine */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBufferCh[band][j] +
					input[band*block_size_samples/NBANDS*2+j];
		}
		/* Shift Previous data */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBufferCh[band][j] =
				input[band*block_size_samples/NBANDS*2+block_size_samples/NBANDS+j];
		}
		gain_data_prev[ch][band] = gain_data[band][1];
		break;
	case LONG_STOP_SEQUENCE:
		/* Gain function making routine */
		son_gainc_window(block_size_samples/NBANDS*2,
				gain_data_prev[ch][band],
				gain_data[band][0], gain_data[band][1],
				a_gcwind, LONG_STOP_SEQUENCE);
		/* Gain compensating routine */
		for( j = 0; j < block_size_samples/NBANDS*2; j++){
			input[band*block_size_samples/NBANDS*2+j] *= a_gcwind[j];
		}
		/* Overlapping rouitne */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBufferCh[band][j] +
					input[band*block_size_samples/NBANDS*2+j];
		}
		/* Shift Previous data */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBufferCh[band][j] =
					input[band*block_size_samples/NBANDS*2+block_size_samples/NBANDS+j];
		}
		gain_data_prev[ch][band] = gain_data[band][1];
		break;
	case EIGHT_SHORT_SEQUENCE:
		for(k = 0; k < SHORT_WIN_IN_LONG; k++){
			/* Gain function making routine */
			son_gainc_window(block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2,
					gain_data_prev[ch][band],
					gain_data[band][k], gain_data[band][k],
					a_gcwind, EIGHT_SHORT_SEQUENCE);
			/* Gain compensating routine */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2; j++){
				input[band*block_size_samples/NBANDS*2+k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+j] *= a_gcwind[j];
			}
			/* Overlapping rouitne */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS; j++){
				gcOverlapBufferCh[band][j+block_size_samples/NBANDS*7/16+block_size_samples/SHORT_WIN_IN_LONG/NBANDS*k] += input[band*block_size_samples/NBANDS*2+k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+j];
			}
			/* Shift Previous data */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS; j++){
				gcOverlapBufferCh[band][j+block_size_samples/NBANDS*7/16+block_size_samples/SHORT_WIN_IN_LONG/NBANDS*(k+1)] = input[band*block_size_samples/NBANDS*2+k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+block_size_samples/SHORT_WIN_IN_LONG/NBANDS+j];
			}
			gain_data_prev[ch][band] = gain_data[band][k];
		}
		for(j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBufferCh[band][j];
		}
		for(j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBufferCh[band][j] =
					gcOverlapBufferCh[band][j+block_size_samples/NBANDS];
		}
		break;
	default:
		CommonExit(1, "gc_compensate: ERROR! Illegal Window Type \n");
		break;
	}
	free(a_gcwind);
}

void	son_gc_compensate(
	double	timeSigChWithGC[],
	GAINC	*gainInfoCh[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	double	*gcOverlapBufferCh[],
	double	*bandSigCh[],
	int ssr_decoder_band
	)
{
  int i;
	for ( i = 0; i < ssr_decoder_band; i++) { 
		son_gc_compensate_sub(timeSigChWithGC,
				gainInfoCh, block_size_samples,
				window_sequence, ch, i,
				gcOverlapBufferCh,
				bandSigCh);
	}
}
