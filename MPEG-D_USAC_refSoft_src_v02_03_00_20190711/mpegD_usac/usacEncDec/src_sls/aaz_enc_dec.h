#ifndef __AAZ_ENC_DEC_H_
#define __AAZ_ENC_DEC_H_

void determineBlockTypeLosslessOnlyCPE(short* block_type_lossless_only, /* r/w */
				       int* mdctTimeBuf_left,
				       int* mdctTimeBuf_right,
				       int* timeSigBuf_left,
				       int* timeSigBuf_right,
				       int* timeSigLookAheadBuf_left,
				       int* timeSigLookAheadBuf_right,
				       int osf);

void determineBlockTypeLosslessOnlySCE(short* block_type_lossless_only, /* r/w */
				       int* mdctTimeBuf,
				       int* timeSigBuf,
				       int* timeSigLookAheadBuf,
				       int osf);

/*static*/ float entropy(int* int_spectrum, int N);

#endif
