
#include <math.h>

#include "interface.h"
#include "block.h"
#include "int_mdct.h"
#include "int_mdct_defs.h"
#include "aaz_enc_dec.h"


/*static*/ float entropy(int* int_spectrum, int N)
{
  float tmp = 0;
  int i;

  for (i=0; i<N; i++) {
    tmp += log10(1.0+2.0*fabs((float)int_spectrum[i]));
  }
  tmp /= log10(2.0);

  return tmp;
}


/* block-switching for lossless-only by entropy calculation */
void determineBlockTypeLosslessOnlyCPE(short* block_type_lossless_only, /* r/w */
				       int* mdctTimeBuf_left,
				       int* mdctTimeBuf_right,
				       int* timeSigBuf_left,
				       int* timeSigBuf_right,
				       int* timeSigLookAheadBuf_left,
				       int* timeSigLookAheadBuf_right,
				       int osf)
{
  int try1;
  int try2;
  int i;
  int tmp_mdctTimeBuf[2][MAX_OSF*512];
  int tmp_int_spectral_line_vector[2][MAX_OSF*1024];
  float currentEntropy, minEntropy;
  short try_new_block_type[2]; 
  short try_look_ahead_block_type[2][2];
	  
  if ((*block_type_lossless_only == ONLY_LONG_WINDOW) ||
      (*block_type_lossless_only == LONG_STOP_WINDOW)) {     /*SHORT_LONG*/
    try_new_block_type[0] = ONLY_LONG_WINDOW;
    try_new_block_type[1] = LONG_START_WINDOW;              /*LONG_SHORT*/
  }
  if ((*block_type_lossless_only == LONG_START_WINDOW) ||
      (*block_type_lossless_only == EIGHT_SHORT_WINDOW)) {
    try_new_block_type[0] = LONG_STOP_WINDOW;
    try_new_block_type[1] = EIGHT_SHORT_WINDOW;
  }
  try_look_ahead_block_type[0][0] = ONLY_LONG_WINDOW;
  try_look_ahead_block_type[0][1] = LONG_START_WINDOW;
  try_look_ahead_block_type[1][0] = LONG_STOP_WINDOW;
  try_look_ahead_block_type[1][1] = EIGHT_SHORT_WINDOW;
  
  for (try1=0; try1<2; try1++) {
    for (try2=0; try2<2; try2++) {
      currentEntropy = 0;
      for (i=0; i<osf*512; i++) {
	tmp_mdctTimeBuf[0][i] = mdctTimeBuf_left[i];
	tmp_mdctTimeBuf[1][i] = mdctTimeBuf_right[i];
      }
      StereoIntMDCT(timeSigBuf_left,
		    timeSigBuf_right,
		    tmp_mdctTimeBuf[0],
		    tmp_mdctTimeBuf[1],
		    tmp_int_spectral_line_vector[0],
		    tmp_int_spectral_line_vector[1],
		    try_new_block_type[try1],
		    WS_FHG,
		    osf);
	      
      currentEntropy += entropy(tmp_int_spectral_line_vector[0], osf*1024);
      currentEntropy += entropy(tmp_int_spectral_line_vector[1], osf*1024);
      StereoIntMDCT(timeSigLookAheadBuf_left,
		    timeSigLookAheadBuf_right,
		    tmp_mdctTimeBuf[0],
		    tmp_mdctTimeBuf[1],
		    tmp_int_spectral_line_vector[0],
		    tmp_int_spectral_line_vector[1],
		    try_look_ahead_block_type[try1][try2],
		    WS_FHG,
		    osf);
	      
      currentEntropy += entropy(tmp_int_spectral_line_vector[0], osf*1024);
      currentEntropy += entropy(tmp_int_spectral_line_vector[1], osf*1024);
      
      if ((try1 == 0) && (try2 == 0)) {
	minEntropy = currentEntropy;
      }
      if (currentEntropy <= minEntropy) {
	minEntropy = currentEntropy;
	*block_type_lossless_only = try_new_block_type[try1];
      }
    }
  }
}

void determineBlockTypeLosslessOnlySCE(short* block_type_lossless_only, /* r/w */
				       int* mdctTimeBuf,
				       int* timeSigBuf,
				       int* timeSigLookAheadBuf,
				       int osf)
{
  int try1;
  int try2;
  int i;
  int tmp_mdctTimeBuf[MAX_OSF*512];
  int tmp_int_spectral_line_vector[MAX_OSF*1024];
  float currentEntropy, minEntropy;
  short try_new_block_type[2]; 
  short try_look_ahead_block_type[2][2];
	  
  if ((*block_type_lossless_only == ONLY_LONG_WINDOW) ||
      (*block_type_lossless_only == LONG_STOP_WINDOW)) {     /*SHORT_LONG*/
    try_new_block_type[0] = ONLY_LONG_WINDOW;
    try_new_block_type[1] = LONG_START_WINDOW;              /*LONG_SHORT*/
  }
  if ((*block_type_lossless_only == LONG_START_WINDOW) ||
      (*block_type_lossless_only == EIGHT_SHORT_WINDOW)) {
    try_new_block_type[0] = LONG_STOP_WINDOW;
    try_new_block_type[1] = EIGHT_SHORT_WINDOW;
  }
  try_look_ahead_block_type[0][0] = ONLY_LONG_WINDOW;
  try_look_ahead_block_type[0][1] = LONG_START_WINDOW;
  try_look_ahead_block_type[1][0] = LONG_STOP_WINDOW;
  try_look_ahead_block_type[1][1] = EIGHT_SHORT_WINDOW;
  
  for (try1=0; try1<2; try1++) {
    for (try2=0; try2<2; try2++) {
      currentEntropy = 0;
      for (i=0; i<osf*512; i++) {
	tmp_mdctTimeBuf[i] = mdctTimeBuf[i];
      }
      IntMDCT(timeSigBuf,
	      tmp_mdctTimeBuf,
	      tmp_int_spectral_line_vector,
	      try_new_block_type[try1],
	      WS_FHG,
	      osf);
	      
      currentEntropy += entropy(tmp_int_spectral_line_vector, osf*1024);
      IntMDCT(timeSigLookAheadBuf,
	      tmp_mdctTimeBuf,
	      tmp_int_spectral_line_vector,
	      try_look_ahead_block_type[try1][try2],
	      WS_FHG,
	      osf);
	      
      currentEntropy += entropy(tmp_int_spectral_line_vector, osf*1024);
      
      if ((try1 == 0) && (try2 == 0)) {
	minEntropy = currentEntropy;
      }
      if (currentEntropy <= minEntropy) {
	minEntropy = currentEntropy;
	*block_type_lossless_only = try_new_block_type[try1];
      }
    }
  }
}
