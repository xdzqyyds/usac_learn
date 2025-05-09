/*******************************************************************************
MPEG-4 Audio VM

This software module was originally developed by

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
*******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "allHandles.h"

#include "tf_mainHandle.h"
#include "sony_local.h"

#include "bitstream.h"

/*******************************************************************************
	arrange spectral_line_vector (for decoding)
*******************************************************************************/
void	son_gc_arrangeSpecDec(
	double	*freqSignalCh,
	int	block_size_samples,
	int	window_sequence,
	double	*freqSignalChForPP
	)
{
	int	i, j;
	int	band;
	double	tmp;

	for (band = 0; band < NBANDS; band++) {
		if (window_sequence == EIGHT_SHORT_SEQUENCE) {
			for (i = 0; i < 8; i++) {
				memcpy(
					(char *)&freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i],
					(char *)&freqSignalCh[band * block_size_samples/NBANDS/8 + i * block_size_samples/8],
					block_size_samples/NBANDS/8*sizeof(double)
				);
				/* reverse the order of the MDCT coefficients in each even PQF band */
				if (band % 2 == 1) {
					for (j = 0; j < block_size_samples/NBANDS/8/2; j++) {
						tmp = freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i + j];
						freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i + j]
							= freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * (i + 1) - 1 - j];
						freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * (i + 1) - 1 - j]
							= tmp;
					}
				}
			}
		}
		else {
			memcpy(
				(char *)&freqSignalChForPP[block_size_samples/NBANDS * band],
				(char *)&freqSignalCh[block_size_samples/NBANDS *band],
				block_size_samples/NBANDS * sizeof(double)
			);
			/* reverse the order of the MDCT coefficients in each even PQF band */
			if (band % 2 == 1) {
				for (i = 0; i < block_size_samples/NBANDS/2; i++) {
					tmp = freqSignalChForPP[block_size_samples/NBANDS * band + i];
					freqSignalChForPP[block_size_samples/NBANDS * band + i]
						= freqSignalChForPP[block_size_samples/NBANDS * (band + 1) - 1 - i];
					freqSignalChForPP[block_size_samples/NBANDS * (band + 1) - 1 - i]
						= tmp;
				}
			}
		}
	}
}
