/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works.
Copyright (c)1997.  

***************************************************************************/


#ifndef _nok_bwp_enc_h_
#define _nok_bwp_enc_h_

#include "allHandles.h"
#include "block.h"
#include "tf_mainHandle.h"

/*
  Type:		NOK_BWP_PRED_STATUS
  Purpose:	Type of the struct holding the status information of
  		the BWP prediction going to bitstream.  */
typedef struct
  {
    short global_pred_flag;
    int pred_sfb_flag[MAX_SCFAC_BANDS];
    short int reset_flag;
    int reset_group;
    int side_info;
  }
NOK_BW_PRED_STATUS;

void nok_InitPrediction (int debug);
void nok_PredCalcPrediction (double *act_spec, double *last_spec,
			     int btype, int nsfb, int *isfb_width,
			     short *pred_global_flag, int *pred_sfb_flag,
			     short *reset_flag, int *reset_group,
			     int *side_info);
void nok_bwp_encode (HANDLE_BSBITSTREAM bs, int num_of_sfb,
		     NOK_BW_PRED_STATUS *status);

#define NOK_CODESIZE 128+128+256


#endif /* _nok_bwp_enc_h_ */
