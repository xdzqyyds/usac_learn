/*
  todo: add disclaimer

 */

#ifndef __LIT_LL_COMMON_H__ 
#define __LIT_LL_COMMON_H__ 

#include "tf_mainStruct.h"


extern short scale_pcm[4];

void calc_sfb_offset_table(
                           LIT_LL_quantInfo *quantInfo,
                           /* Info * info, */
                           int osf
                           );


#endif


