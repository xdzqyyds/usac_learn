
#ifndef _ll_bitmux_h_
#define _ll_bitmux_h_

#include "block.h"
#include "tf_mainStruct.h"
#include "tf_main.h"
#include "tns3.h"
#include "sony_local.h"

#ifdef I2R_LOSSLESS
#include "lit_ll_en.h"

void write_ll_sce(
  HANDLE_AACBITMUX bitmux,
  LIT_LL_Info *ll_Info 
  );
#endif

#endif
