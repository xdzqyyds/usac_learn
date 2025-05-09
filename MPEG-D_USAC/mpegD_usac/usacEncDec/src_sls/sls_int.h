/**********************************************************************

todo: add disclaimer

**********************************************************************/
#ifndef _sls_int_h_
#define _sls_int_h_

#include "sls.h"

/* ----------------------------------------------------------
   sls internal definitions
  ----------------------------------------------------------- */

int sineDataFunction(int index);
int sineDataFunction_cs(int index);

int thrFunction(int abs_quant_aac, int overall_scale);
int invQuantFunction(int abs_quant_aac, int overall_scale);

void InvStereoIntMDCT(int* p_in_0,
		      int* p_in_1,
		      int* p_overlap_0,
		      int* p_overlap_1,
		      int* p_out_0,
		      int* p_out_1,
		      byte blockType,
		      byte windowShape,
		      int osf);


#endif
