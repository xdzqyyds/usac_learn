#ifndef DE_LOSSLESS_H
#define DE_LOSSLESS_H


/* #include "all.h" */
#include "interface.h"

#include "tf_mainHandle.h"


void LIT_ll_Dec(LIT_LL_decInfo *ll_Info,
                LIT_LL_quantInfo *quantInfo[MAX_TF_LAYER],
                /*byte*/ int mask[60],
                byte hasmask,
/*                byte ** cb_mask,*/
                int *rec_spectrum,
                Ch_Info *channelInfo,
                int sampling_rate,
                int osf,
                int type_PCM,
                short * block_type_lossless_only,
                int *msStereoIntMDCT,
                int coreenable,
                int ch,
                int num_aac_layers
                );

int ll_byte_align(void);

void LIT_ll_read(LIT_LL_decInfo *ll_Info);
void lit_ll_deinterleave(int inptr[], int outptr[], byte * group, LIT_LL_quantInfo *quantInfo);
void deinterleave_grp2_osf(int* int_spectral_line_vector, int osf);

#endif
