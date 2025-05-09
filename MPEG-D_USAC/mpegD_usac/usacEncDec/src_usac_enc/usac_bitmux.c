/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Fraunhofer IIS

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.

$Id: usac_bitmux.c,v 1.14 2011-04-29 12:10:29 mul Exp $
*******************************************************************************/
#include <stdio.h>

#include "usac_bitmux.h"
#include "usac_fd_qc.h"
#include "common_m4a.h"

#include "interface.h"
#include "usac_interface.h"
#include "usac_arith_dec.h"

static int find_grouping_bits(const int window_group_length[],
                              int num_window_groups)
{

  /* This function inputs the grouping information and outputs the seven bit
     'grouping_bits' field that the AAC decoder expects.  */


  int grouping_bits = 0;
  int tmp[8];
  int i,j;
  int index=0;

  for(i=0; i<num_window_groups; i++){
    for (j=0; j<window_group_length[i];j++){
      tmp[index++] = i;
      /*      printf("tmp[%d] = %d\n",index-1,tmp[index-1]);*/
    }
  }

  for(i=1; i<8; i++){
    grouping_bits = grouping_bits << 1;
    if(tmp[i] == tmp[i-1]) {
      grouping_bits++;
    }
  }

  /*  printf("grouping_bits = %d  [i=%d]\n",grouping_bits,i);*/
  return(grouping_bits);
}
int usac_write_ics_info (HANDLE_AACBITMUX bitmux,
			 int max_sfb,
			 WINDOW_SEQUENCE windowSequence,
			 WINDOW_SHAPE window_shape,
			 int num_window_groups,
			 const int window_group_length[])
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int tmpVar = 0;

  HANDLE_BSBITSTREAM bs_WINDOW_SEQ = aacBitMux_getBitstream(bitmux, WINDOW_SEQ);
  HANDLE_BSBITSTREAM bs_WINDOW_SHAPE_CODE = aacBitMux_getBitstream(bitmux, WINDOW_SHAPE_CODE);
  HANDLE_BSBITSTREAM bs_SCALE_FACTOR_GROUPING = aacBitMux_getBitstream(bitmux, SCALE_FACTOR_GROUPING);
  HANDLE_BSBITSTREAM bs_MAX_SFB = aacBitMux_getBitstream(bitmux, MAX_SFB);
  if (write_flag&&((bs_WINDOW_SEQ==NULL)||
                   (bs_WINDOW_SHAPE_CODE==NULL)||(bs_SCALE_FACTOR_GROUPING==NULL)||
                   (bs_MAX_SFB==NULL))) {
    CommonWarning("usacBitMux: error writing ics_info()");
    write_flag=0;
  }

  /* write out WINDOW_SEQUENCE */
  switch (windowSequence) {
  case EIGHT_SHORT_SEQUENCE:
    tmpVar=2;
    break;
  case ONLY_LONG_SEQUENCE:
    tmpVar=0;
    break;
  case LONG_START_SEQUENCE:
  case STOP_START_SEQUENCE:
    tmpVar=1;
    break;
  case LONG_STOP_SEQUENCE:
    tmpVar=3;
    break;
  default:
    CommonExit(-1,"\n unknown blocktype : %d",windowSequence);
  }
  if (write_flag) BsPutBit(bs_WINDOW_SEQ, tmpVar, 2);
  bit_count += 2;
   
  /* write out WINDOW SHAPE */
  switch( window_shape ) {
  case WS_FHG:
    tmpVar = 0;
    break;
  case WS_DOLBY:
    tmpVar = 1;
    break;
  default:
    CommonExit(-1,"\nunknow windowshape : %d", window_shape );
  }
  if (write_flag) BsPutBit(bs_WINDOW_SHAPE_CODE, tmpVar, 1);
  bit_count += 1;
   
   
  /* write out SCALE_FACTOR_GROUPING and MAX_SFB */
  if (windowSequence == EIGHT_SHORT_SEQUENCE) {
    if (write_flag) BsPutBit(bs_MAX_SFB, max_sfb, 4);
    bit_count += 4;
     
    tmpVar = find_grouping_bits(window_group_length, num_window_groups);
    if (write_flag) BsPutBit(bs_SCALE_FACTOR_GROUPING, tmpVar, 7);  /* the grouping bits */
    bit_count += 7;
     
  } else {    /* block type is either start, stop, or long */
    if (write_flag) BsPutBit(bs_MAX_SFB, max_sfb, 6);
    bit_count += 6;
  }
   
  return(bit_count);
}

int usac_write_tw_data(HANDLE_AACBITMUX bitmux,
                       int              tw_data_present,
                       int              tw_ratio[]) {

  int bit_count = 0;
  int write_flag = (bitmux!=NULL);
  HANDLE_BSBITSTREAM bs_TW_DATA_PRESENT = aacBitMux_getBitstream(bitmux, TW_DATA_PRESENT);
  if (write_flag) BsPutBit(bs_TW_DATA_PRESENT,tw_data_present,LEN_TW_PRES);
  bit_count += LEN_TW_PRES;

  if ( tw_data_present ) {
    HANDLE_BSBITSTREAM bs_TW_RATIO = aacBitMux_getBitstream(bitmux,TW_RATIO);
    int i=0;
    for ( i = 0 ; i < NUM_TW_NODES ; i++ ) {
      if (write_flag) BsPutBit(bs_TW_RATIO,tw_ratio[i],LEN_TW_RATIO);
      bit_count += LEN_TW_RATIO;
    }
  }

  return (bit_count);

}

/* write single channel element header */
int usac_write_sce(HANDLE_AACBITMUX bitmux, 
                   int core_mode,
                   int tns_data_present)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_CORE_MODE = aacBitMux_getBitstream(bitmux,CORE_MODE);
  HANDLE_BSBITSTREAM bs_TNS_DATA_PRESENT = aacBitMux_getBitstream(bitmux, TNS_DATA_PRESENT);

  if (write_flag&&(bs_CORE_MODE==NULL)) {
    CommonWarning("usacBitMux: error writing usac_write_sce()");
    write_flag=0;
  }

  if (write_flag) BsPutBit(bs_CORE_MODE, core_mode, LEN_CORE_MODE);
  bit_count += LEN_CORE_MODE;

  if (core_mode == 0) {
    if (write_flag) BsPutBit(bs_TNS_DATA_PRESENT, tns_data_present, LEN_TNS_PRES);
    bit_count += LEN_TNS_PRES;
  }

  return (bit_count);
}

static int write_cplx_pred_data(HANDLE_AACBITMUX bitmux,
	int num_window_groups,
	int nr_of_sfb,
  int predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  const int huff[13][1090][4],
  int const bUsacIndependenceFlag)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int i,j;

  HANDLE_BSBITSTREAM bs_CPLX_PRED_ALL = aacBitMux_getBitstream(bitmux, CPLX_PRED_ALL);
  HANDLE_BSBITSTREAM bs_PRED_DIR = aacBitMux_getBitstream(bitmux, PRED_DIR);
  HANDLE_BSBITSTREAM bs_COMPLEX_COEF = aacBitMux_getBitstream(bitmux, COMPLEX_COEF);
  HANDLE_BSBITSTREAM bs_DELTA_CODE_TIME = aacBitMux_getBitstream(bitmux, DELTA_CODE_TIME);
  HANDLE_BSBITSTREAM bs_DPRED_COEF = aacBitMux_getBitstream(bitmux, DPRED_COEF);

  const int cplx_pred_all = 1;
  const int pred_dir = 0;
  const int complex_coef = 0;
  const int delta_code_time = 0;

  if (write_flag) BsPutBit(bs_CPLX_PRED_ALL, cplx_pred_all, LEN_CPLX_PRED_ALL);
  bit_count += LEN_CPLX_PRED_ALL;
    
  if (write_flag) BsPutBit(bs_PRED_DIR, pred_dir, LEN_PRED_DIR);
  bit_count += LEN_PRED_DIR;
    
  if (write_flag) BsPutBit(bs_COMPLEX_COEF, complex_coef, LEN_COMPLEX_COEF);
  bit_count += LEN_COMPLEX_COEF;

  if(!bUsacIndependenceFlag){
    if (write_flag) BsPutBit(bs_DELTA_CODE_TIME, delta_code_time, LEN_DELTA_CODE_TIME);
    bit_count += LEN_DELTA_CODE_TIME;
  }
    
  for (i = 0; i < num_window_groups; i++) {
    int predCoefPrev = 0;
    int deltaPredCoef;
    const int sfb_per_predband = 2;

    for (j = 0; j < nr_of_sfb; j += 2) {
      int length;
      int codeword;

      deltaPredCoef = predCoef[i][j] - predCoefPrev;
      predCoefPrev = predCoef[i][j];
      
      length = huff[12][deltaPredCoef+60][2];    
	  codeword = huff[12][deltaPredCoef+60][3];
  
      if (write_flag) BsPutBit(bs_DPRED_COEF, codeword, length);
      bit_count += length;
    }
  }

  return bit_count;
}

/* write channel pair element header */
int usac_write_cpe(HANDLE_AACBITMUX bitmux,
                   USAC_CORE_MODE core_mode[MAX_TIME_CHANNELS],
                   int *tns_data_present,
                   int predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
                   const int huff[13][1090][4],
                   int common_window,
                   int common_tw,
                   int max_sfb,
                   WINDOW_SEQUENCE windowSequence,
                   WINDOW_SHAPE window_shape,
                   int num_window_groups,
                   int window_group_length[],
                   int ms_mask,
                   int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
                   int flag_twMdct,
                   UsacToolsInfo *toolsInfo,
                   int const bUsacIndependenceFlag)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_CORE_MODE = aacBitMux_getBitstream(bitmux,CORE_MODE);
  HANDLE_BSBITSTREAM bs_COM_WIN = aacBitMux_getBitstream(bitmux,COMMON_WINDOW);
  HANDLE_BSBITSTREAM bs_TNS_ACTIVE = aacBitMux_getBitstream(bitmux, TNS_ACTIVE);
  HANDLE_BSBITSTREAM bs_COMMON_MAX_SFB = aacBitMux_getBitstream(bitmux, COMMON_MAX_SFB);
  HANDLE_BSBITSTREAM bs_COMMON_TNS = aacBitMux_getBitstream(bitmux, COMMON_TNS);
  HANDLE_BSBITSTREAM bs_TNS_ON_LR = aacBitMux_getBitstream(bitmux, TNS_ON_LR);
  HANDLE_BSBITSTREAM bs_TNS_PRESENT_BOTH = aacBitMux_getBitstream(bitmux, TNS_PRESENT_BOTH);
  HANDLE_BSBITSTREAM bs_TNS_DATA_PRESENT1 = aacBitMux_getBitstream(bitmux, TNS_DATA_PRESENT1);
   
  int common_max_sfb = 1;

  if (write_flag&&(bs_CORE_MODE==NULL)) {
    CommonWarning("usacBitMux: error writing usac_write_sce()");
    write_flag=0;
  }

  if (write_flag) BsPutBit(bs_CORE_MODE, core_mode[0], LEN_CORE_MODE);
  bit_count += LEN_CORE_MODE;

  if (write_flag) BsPutBit(bs_CORE_MODE, core_mode[1], LEN_CORE_MODE);
  bit_count += LEN_CORE_MODE;

  if ( core_mode[0] == CORE_MODE_FD && core_mode[1] == CORE_MODE_FD) {
    int tns_active = tns_data_present[0] || tns_data_present[1];

    if (write_flag) BsPutBit(bs_TNS_ACTIVE, tns_active, LEN_TNS_ACTIVE);
    bit_count += LEN_TNS_ACTIVE;

    if (write_flag) BsPutBit(bs_COM_WIN, common_window, LEN_COM_WIN);
    bit_count += LEN_COM_WIN;

    if(common_window){
      bit_count += usac_write_ics_info(bitmux,max_sfb,windowSequence,window_shape,
                                       num_window_groups, window_group_length);

      if (write_flag) BsPutBit(bs_COMMON_MAX_SFB, common_max_sfb, LEN_COMMON_MAX_SFB);
      bit_count += LEN_COMMON_MAX_SFB;

      bit_count += write_ms_data(bitmux, ms_mask, ms_used,
                                 num_window_groups, 0, max_sfb);

      if (ms_mask == 3) {
        bit_count +=write_cplx_pred_data(bitmux, num_window_groups, max_sfb, predCoef, huff, bUsacIndependenceFlag);
      }
    }

    if ( flag_twMdct ) {
      HANDLE_BSBITSTREAM bs_COMMON_TW = aacBitMux_getBitstream(bitmux,COMMON_TIMEWARPING);
      if (write_flag) BsPutBit(bs_COMMON_TW,common_tw,LEN_COM_TW);
      bit_count += LEN_COM_TW;
      if ( common_tw ) {
        bit_count += usac_write_tw_data(bitmux,
                                        toolsInfo->tw_data_present,
                                        toolsInfo->tw_ratio);

      }
    }

    if (tns_active) {
      int common_tns = 0;
      int tns_on_lr = 1;
      int tns_present_both = tns_data_present[0] && tns_data_present[1];
      int tns_data_present1 = tns_data_present[1];

      if (common_window) {
        if (write_flag) BsPutBit(bs_COMMON_TNS, common_tns, LEN_COMMON_TNS);
        bit_count += LEN_COMMON_TNS;
      }

      if (write_flag) BsPutBit(bs_TNS_ON_LR, tns_on_lr, LEN_TNS_ON_LR);
      bit_count += LEN_TNS_ON_LR;

      if (write_flag) BsPutBit(bs_TNS_PRESENT_BOTH, tns_present_both, LEN_TNS_PRESENT_BOTH);
      bit_count += LEN_TNS_PRESENT_BOTH;

      if (!tns_present_both) {
        if (write_flag) BsPutBit(bs_TNS_DATA_PRESENT1, tns_data_present1, LEN_TNS_DATA_PRESENT1);
        bit_count += LEN_TNS_DATA_PRESENT1;
      }
    }
  }


  return (bit_count);
}


int usac_fd_cs(
  HANDLE_AACBITMUX bitmux,
  WINDOW_SEQUENCE windowSequence,
  WINDOW_SHAPE windowShape,
  int global_gain,
  const int huff[13][1090][4],
  int max_sfb,
  int nr_of_sfb,
  int num_window_groups,
  const int window_group_length[],
  const int noise_nrg[],
  UsacICSinfo *ics_info,
  UsacToolsInfo *tool_data,
  TNS_INFO      *tnsInfo,
  UsacQuantInfo *qInfo,
  int common_window,
  int common_tw,
  int flag_twMdct,
  int flag_noiseFilling,
  short *facData,
  int   nb_bits_fac,
  int   bUsacIndependencyFlag,
  int qdebug)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int book_vector[SFB_NUM_MAX];
  int fac_data_present = (nb_bits_fac > 0)?1:0;

  HANDLE_BSBITSTREAM bs_GLOBAL_GAIN = aacBitMux_getBitstream(bitmux, GLOBAL_GAIN);
  if (write_flag&&(bs_GLOBAL_GAIN==NULL)) {
    CommonWarning("usacBitMux: error writing tf_channel_stream()");
    write_flag=0;
  }

  /* the 8-bit global_gain is the first scalefactor */
  if (write_flag) {
    BsPutBit(bs_GLOBAL_GAIN, global_gain, LEN_SCL_PCM);
    if ( (qdebug > 2)  ) {
      fprintf(stderr,"\nglobal_gain %d ",global_gain);
    }
  }
  bit_count += LEN_SCL_PCM;

  /* Noise filling*/
  if(flag_noiseFilling){
      HANDLE_BSBITSTREAM bs_NOISE_OFFSET = aacBitMux_getBitstream(bitmux,FD_NOISE_OFFSET);
      HANDLE_BSBITSTREAM bs_NOISE_LEVEL = aacBitMux_getBitstream(bitmux,FD_NOISE_LEVEL);
      if (write_flag&&(bs_NOISE_OFFSET==NULL || bs_NOISE_LEVEL == NULL)) {
        CommonWarning("usacBitMux: error writing tf_channel_stream()");
        write_flag=0;
      }

      /* the 8-bit global_gain is the first scalefactor */
      if (write_flag) {

        tool_data->noiseOffset =  0;
        tool_data->noiseLevel  =  7;

        BsPutBit(bs_NOISE_LEVEL,tool_data->noiseLevel,LEN_NOISE_LEV);

        BsPutBit(bs_NOISE_OFFSET,tool_data->noiseOffset,LEN_NOISE_OFF);
      }
      bit_count += LEN_NOISE_FAC+LEN_NOISE_LEV;
    }

  if(!common_window){
    bit_count += usac_write_ics_info (bitmux,
				      max_sfb,
				      windowSequence,
				      windowShape,
				      num_window_groups,
				      window_group_length);
  }

  /* TW-mdct*/
  if(flag_twMdct){
    if(!common_tw)
      bit_count += usac_write_tw_data(bitmux,
                                      tool_data->tw_data_present,
                                      tool_data->tw_ratio);
  }


  /* No section data*/
  /* Generate a reasonnable book_vector*/
  {
    int i,j,index=0;

    for(j=0; j<num_window_groups; j++){
      for(i=0;i<max_sfb;i++) {
	book_vector[index++]=11;
      }
      for(;i<nr_of_sfb;i++) {
	book_vector[index++]=0;
      }
    }
  }


  /* scale_factor_data() information */
  bit_count += write_scalefactor_bitstream(bitmux,
                                           1,
                                           nr_of_sfb,
					   qInfo->scale_factor,
					   book_vector,
					   num_window_groups,
					   global_gain,
					   windowSequence, noise_nrg,
					   huff,
                                           qdebug);

  /*TNS*/
  bit_count += write_tns_data(bitmux,
                              1,
			      tnsInfo,
			      windowSequence,
			      NULL);

  {
    HANDLE_BSBITSTREAM bs_reset = aacBitMux_getBitstream(bitmux,
							 RESET_ARIT_DEC);
    if (write_flag&&(bs_reset==NULL)) {
      CommonWarning("aencSpecFrame error writing arithmetic reset flag()");
      write_flag=0;
    }

    if(!bUsacIndependencyFlag){
      /* write reset-flag */
      if(write_flag) BsPutBit(bs_reset, qInfo->reset, LEN_RESET_ARIT_DEC);
      bit_count += LEN_RESET_ARIT_DEC;
    }
  }


  {
    HANDLE_BSBITSTREAM bs_data = aacBitMux_getBitstream(bitmux,
							FD_ARIT_DATA);

    if (write_flag&&(bs_data==NULL)) {
      CommonWarning("aencSpecFrame: error writing spectral_data()");
      write_flag=0;
    }

    /* write out the spectral data */
    bit_count += aencSpecFrame(bs_data,
                              windowSequence,
                              1024,
                              qInfo->quantDegroup,
                              qInfo->max_spec_coeffs,
                              qInfo->arithQ,
                              &(qInfo->arithPreviousSize),
                              bUsacIndependencyFlag || qInfo->reset);
  }

  /* FAC data */
  {
    HANDLE_BSBITSTREAM bs_fac = aacBitMux_getBitstream(bitmux, FD_FAC_DATA);
    
    if ( write_flag && (bs_fac == NULL) ) {
      CommonWarning("aencSpecFrame: error writing fac_data()");
      write_flag=0;
    }
    
    /* write fac_data_present */
    if(write_flag) BsPutBit(bs_fac, fac_data_present, LEN_FAC_DATA_PRESENT);
    bit_count += LEN_FAC_DATA_PRESENT;

    if(fac_data_present){
      int i;
      if(write_flag){
        for(i=0; i<nb_bits_fac; i+= 8){
          int bitsToWrite = min(8, nb_bits_fac - i);
          BsPutBit(bs_fac, facData[i/8] >> (8 - bitsToWrite), bitsToWrite);
        }
      }
      bit_count += nb_bits_fac;
    }
  }

  return(bit_count);
}

int usac_write_fillElem(HANDLE_BSBITSTREAM bs_padding,
                        int fillBits
) {
  int write_flag = (bs_padding != NULL);
  int bit_count  = 0;

  if (fillBits <= 8) {
    if (write_flag) {
      BsPutBit(bs_padding, 0, 1);
    }
    bit_count++;
    fillBits--;
  } else {
    if (write_flag) {
      BsPutBit(bs_padding, 1, 1);
    }
    bit_count++;
    fillBits--;

    if (fillBits <= 8) {
      /* defaultLength is 0 */
      if (write_flag) {
        BsPutBit(bs_padding, 1, 1);
      }
      bit_count++;
      fillBits--;
    } else {
      /* round down */
      int writeBytes = 0;
      if (write_flag) {
        BsPutBit(bs_padding, 0, 1);
      }
      bit_count++;
      fillBits--;
      writeBytes = fillBits/8;

      if (writeBytes > 255) {
        writeBytes -= 3;
        if (write_flag) {
          BsPutBit(bs_padding, 255, 8);
        }
        if (write_flag) {
          BsPutBit(bs_padding, writeBytes-253, 16);
        }
        bit_count += 24;
        fillBits -= 24;
      } else {
        writeBytes--;
        if (write_flag) {
          BsPutBit(bs_padding, writeBytes, 8);
        }
        bit_count += 8;
        fillBits  -= 8;
      }
      
      while (writeBytes > 0) {
        if (write_flag) {
          BsPutBit(bs_padding, 0xA9, 8);
        }
        bit_count += 8;
        fillBits  -= 8;
        writeBytes--;
      }
    }
  }

  if ((bit_count > 1 && fillBits < 0) || fillBits >= 8 || bit_count < 1 ) {
    CommonExit(-1, "ERROR: Fill element is not implemented correctly");
  }

  return bit_count;
}
