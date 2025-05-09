/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)
and edited by
Olaf Kaehler (Fraunhofer IIS-A)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1998.



 

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Bitmux.c  : Handling of bistream format for AAC Raw and AAC SCALABLE     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include "all.h"  /* MIN() */
#include "allHandles.h"
#include "block.h"
#include "interface.h"

#include "nok_ltp_common.h"      /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "nok_ltp_enc.h"
#include "tns3.h"

#include "aac_qc.h"
#include "bitmux.h"

#include "resilience.h"

#undef DEBUG_TNS_BITS
#undef DEBUG_FSS_BITS
#undef DEBUG_ESC

struct tagAACbitMux {
  HANDLE_BSBITSTREAM *streams;
  HANDLE_RESILIENCE hResilience;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData;
  int numStreams;
  int usedAssignmentScheme;
  int currentChannel;
};


/* ---- getting the correct stream for each element ---- */

void aacBitMux_free(HANDLE_AACBITMUX mux)
{
  if (mux==NULL) return;
  if (mux->hResilience) free(mux->hResilience);
  if (mux->hEscInstanceData) free(mux->hEscInstanceData);
  free(mux);
}

HANDLE_AACBITMUX aacBitMux_create(HANDLE_BSBITSTREAM *streams,
                                  int numStreams,
                                  int numChannels,
                                  int ep_config,
                                  int debugLevel)
{
  HANDLE_AACBITMUX ret = calloc(sizeof(struct tagAACbitMux),1);
  if (ret==NULL) return NULL;
  
  ret->streams = streams;
  ret->numStreams = numStreams;
  ret->usedAssignmentScheme = -1;
  ret->currentChannel = -1;

  ret->hResilience = CreateErrorResilience(NULL, /*decPara, not used*/
                                           ep_config>0,
                                           0, /*section data resilience*/
                                           0, /*scalefactor resilience*/
                                           0);/*spectral resilience*/
  if (ret->hResilience == NULL) { aacBitMux_free(ret); return NULL; }
  
  if (GetEPFlag(ret->hResilience)) {
    ret->hEscInstanceData = CreateEscInstanceData(NULL, /* info file ? */
                                                  debugLevel);
    if (ret->hEscInstanceData == NULL) { aacBitMux_free(ret); return NULL; }

    InitAacSpecificInstanceData(ret->hEscInstanceData,
                                ret->hResilience,
                                numChannels,
                                0); /* tns not used ?? */
  }

  return ret;
}

/*static */
HANDLE_BSBITSTREAM aacBitMux_getBitstream(HANDLE_AACBITMUX bitmux,
                                          CODE_TABLE code)
{
#define E(BITSTREAM_ELEMENT) #BITSTREAM_ELEMENT
  enum { maxbitstreamElementLen = 50 };
  char bitstreamElement[MAX_ELEMENTS+1][maxbitstreamElementLen] = {
#include "er_main_elements.h"
    "MAX_ELEMENTS"
  };
#undef E
  unsigned short esc;
  unsigned short instanceOfEsc;
  int i, j;
  int stream;
  HANDLE_BSBITSTREAM ret = NULL;

  if (bitmux == NULL) return NULL;

  if (GetEPFlag(bitmux->hResilience)) {

    if ((bitmux->usedAssignmentScheme == -1)||(bitmux->currentChannel == -1)) return NULL;

    esc = GetEscAssignment(bitmux->hEscInstanceData, code, bitmux->usedAssignmentScheme);
    instanceOfEsc = GetInstanceOfEsc_(bitmux->currentChannel+1, bitmux->usedAssignmentScheme);

#ifdef DEBUG_ESC
    fprintf(stderr, "element %s, channel %i assignment-scheme %i, (esc %i,instance %i)\n", bitstreamElement[code], bitmux->currentChannel, bitmux->usedAssignmentScheme, esc, instanceOfEsc);
#endif
    stream = 0;
    for (i=0; i<MAX_NR_OF_INSTANCE_PER_ESC; i++) {
      for (j=0; j<MAX_NR_OF_ESC; j++) {
        int esc_present = BsGetInstanceUsed(bitmux->hEscInstanceData, i, j);
        if ((esc_present)&&(i==instanceOfEsc)&&(j==esc)) goto endOfTheLoop;
        if (esc_present) {
          stream++;
        }
      }
    }
  endOfTheLoop:
    esc=esc; /* avoid deprecated use of label at end of compound statement */
  } else {
    stream = 0;
  }

  if (stream>=bitmux->numStreams) {
    CommonWarning("aacBitMux: element %s not expected\n",bitstreamElement[code]);
    ret = NULL;
  } else {
    ret = bitmux->streams[stream];
#ifdef DEBUG_ESC
    fprintf(stderr,"aacBitMux: element %s goes to stream %i (0x%08x)\n",bitstreamElement[code], stream, ret);
#endif
  }

  return ret;
}

void aacBitMux_setAssignmentScheme(HANDLE_AACBITMUX bitmux, int channels, int common_window)
{
  int scheme = -1;
  if (bitmux == NULL) return;

  if (channels==1) scheme = 0;
  if ((channels==2)&&(common_window==0)) scheme = 1;
  if ((channels==2)&&(common_window==1)) scheme = 2;

  bitmux->usedAssignmentScheme = scheme;
}

void aacBitMux_setCurrentChannel(HANDLE_AACBITMUX bitmux, int channel)
{
  if (bitmux == NULL) return;
  bitmux->currentChannel = channel;
}






/* ---- writing the individual elements ---- */


/* corresponds to ISO/IEC 14496-3 Table 4.46 section_data()
 * ...does not support sectionDataResilience though
 */
static int sort_book_numbers(HANDLE_AACBITMUX bitmux,
                             int book_vector[],
                             int nr_of_sfb,
                             WINDOW_SEQUENCE windowSequence,
                             int num_win_groups,
                             int qdebug)
{

  /*
    This function inputs the vector, 'book_vector[]', which is of length SFB_NUM_MAX,
    and contains the optimal huffman tables of each sfb.  It returns the vector, 'output_book_vector[]', which
    has it's elements formatted for the encoded bit stream.  It's syntax is:
   
    {sect_cb[0], length_segment[0], ... ,sect_cb[num_of_sections], length_segment[num_of_sections]}

    The above syntax is true, unless there is an escape sequence.  An
    escape sequence occurs when a section is longer than 2 ^ (bit_len)
    long in units of scalefactor bands.  Also, the integer returned from
    this function is the number of bits written in the bitstream, 
    'bit_count'.  

    This function supports both long and short blocks.
  */

  int write_flag = (bitmux!=NULL);
  int i;
  int repeat_counter = 1;
  int bit_count = 0;
  int previous;
  int max, bit_len;
  int sfb_per_group = nr_of_sfb/num_win_groups;

  HANDLE_BSBITSTREAM bs_SECT_CB = aacBitMux_getBitstream(bitmux, SECT_CB);
  HANDLE_BSBITSTREAM bs_SECT_LEN_INCR = aacBitMux_getBitstream(bitmux, SECT_LEN_INCR);
  if (write_flag&&((bs_SECT_CB==NULL)||(bs_SECT_LEN_INCR==NULL))) {
    CommonWarning("aacBitMux: error writing section_data()");
    write_flag = 0;
  }

  if (windowSequence == EIGHT_SHORT_SEQUENCE){
    max = 7;
    bit_len = 3;
  }
  else {  /* the windowSequence is a long,start, or stop window */
    max = 31;
    bit_len = 5;
  }

  previous = book_vector[0];
  if (write_flag) {   
    BsPutBit(bs_SECT_CB, book_vector[0], 4);  
    if (qdebug > 1 )
      fprintf(stderr,"\nbookNrs:\n%d = ",book_vector[0]);
  }
  bit_count += 4;		/* bug fix TK */

  for (i=1;i<nr_of_sfb;i++) {
    if( (book_vector[i] != previous) || (i%sfb_per_group==0)) {
      if (write_flag) {
	BsPutBit(bs_SECT_LEN_INCR, repeat_counter, bit_len);  
	if (qdebug > 1 )
	  fprintf(stderr,"%d\n%d = ",i,book_vector[i]);
      }
      bit_count += bit_len;

      if (repeat_counter == max){  /* in case you need to terminate an escape sequence */
	if (write_flag) BsPutBit(bs_SECT_LEN_INCR, 0, bit_len);  
	bit_count += bit_len;
      }
	
      if (write_flag) BsPutBit(bs_SECT_CB, book_vector[i], 4);
      bit_count += 4;		/* bug fix TK */
      previous = book_vector[i];
      repeat_counter=1;

    }
    /* if the length of the section is longer than the amount of bits available in */
    /* the bitsream, "max", then start up an escape sequence */
    else if (((book_vector[i] == previous) && (repeat_counter == max)) ) { 
      if (write_flag) {
	BsPutBit(bs_SECT_LEN_INCR, repeat_counter, bit_len);
	if (qdebug > 1 )
	  fprintf(stderr,"(esc)");
      }
      bit_count += bit_len;
      repeat_counter = 1;
    }
    else {
      repeat_counter++;
    }
  }

  if (write_flag) {
    BsPutBit(bs_SECT_LEN_INCR, repeat_counter, bit_len);
    if (qdebug > 1 )
      fprintf(stderr,"%d\n",i);  
  }
  bit_count += bit_len;
  if (repeat_counter == max) {
    /* special case if the last section length is an escape sequence */
    if (write_flag) BsPutBit(bs_SECT_LEN_INCR, 0, bit_len);
    bit_count += bit_len;
  }
  return(bit_count);
}

/* corresponds to ISO/IEC 14496-3 Table 4.47 scale_factor_data()
 * ...does not support ScaleFactorResilience, though
 */
int write_scalefactor_bitstream(HANDLE_AACBITMUX bitmux,
                                int              bUsacSyntax,
				int nr_of_sfs,
				const int scale_factors[],
				const int book_vector[],
				int num_window_groups,
				int global_gain,
				WINDOW_SEQUENCE windowSequence,
				const int noise_nrg[],
				const int huff[13][1090][4],
				int qdebug)
{

  /* this function takes care of counting the number of bits necessary */
  /* to encode the scalefactors.  In addition, if the write_flag == 1, */
  /* then the scalefactors are written out the fixed_stream output bit */
  /* stream.  it returns k, the number of bits written to the bitstream*/

  int write_flag = (bitmux!=NULL);
  int i,j, bit_count=0;
  int diff,length,codeword;
  int index = 0;
  int sf_out = 0;
  int sf_not_out = 0;
  int nr_of_sfb_per_group=0;
  int pns_pcm_flag = 1;
  int previous_scale_factor = global_gain;
  int previous_noise_nrg = global_gain;

  HANDLE_BSBITSTREAM bs_DPCM_NOISE_NRG = aacBitMux_getBitstream(bitmux, DPCM_NOISE_NRG);
  HANDLE_BSBITSTREAM bs_HCOD_SF = aacBitMux_getBitstream(bitmux, HCOD_SF);
  if (write_flag&&((bs_DPCM_NOISE_NRG==NULL)||(bs_HCOD_SF==NULL))) {
    CommonWarning("aacBitMux: error writing scale_factor_data()");
    write_flag=0;
  }
  
  if (windowSequence == EIGHT_SHORT_SEQUENCE) { /* short windows */
    nr_of_sfb_per_group = nr_of_sfs/num_window_groups;
  }
  else {
    nr_of_sfb_per_group = nr_of_sfs;
  }

  if ( (qdebug > 2) && write_flag ) {
    fprintf(stderr,"\nscalefacs :");	
  }
    
  for(j=0; j<num_window_groups; j++){
    for(i=0;i<nr_of_sfb_per_group;i++) {  
      /* test to see if any codebooks in a group are zero */
      if ( (qdebug > 1) && write_flag ) {
	fprintf(stderr," %d:%d",index,scale_factors[index]);	
      }
      if (book_vector[index] == PNS_HCB) {
        /* send PNS pseudo scalefactors  */
        diff = noise_nrg[index] - previous_noise_nrg;
        if (pns_pcm_flag) {
          pns_pcm_flag = 0;
          if (diff+PNS_PCM_OFFSET<0  ||  diff+PNS_PCM_OFFSET>=(1<<PNS_PCM_BITS))
            fprintf(stderr,"%3d (PNS PCM Overflow0) *****\n", diff);
          length = PNS_PCM_BITS;
          codeword = diff + PNS_PCM_OFFSET;
        } else {
          if (diff<-60  ||  diff>60)
            printf("%3d (PNS Overflow0) *****\n", diff);
          length = huff[12][diff+60][2];    
	  codeword = huff[12][diff+60][3];
        }
	bit_count += length;
        previous_noise_nrg = noise_nrg[index];
	if (write_flag == 1) {   
	  /*	  printf("  scale_factor[%d] = %d\n",index,scale_factors[index]);*/
	  BsPutBit(bs_DPCM_NOISE_NRG, codeword, length); 
	  sf_out++;
	}
      } else if (book_vector[index]) { /* only send scalefactors if using non-zero codebooks */
        /* for USAC, do not transmit first scalefactor, since it's equal to global gain */
        if( !(bUsacSyntax && (i == 0) && (j == 0)) ){
          diff = scale_factors[index] - previous_scale_factor;
          length = huff[12][diff+60][2];    
          bit_count += length;
          previous_scale_factor = scale_factors[index];
          if (write_flag == 1 ) {   
            /*	  printf("  scale_factor[%d] = %d\n",index,scale_factors[index]);*/
            codeword = huff[12][diff+60][3];
            BsPutBit(bs_HCOD_SF, codeword, length); 
            sf_out++;
          }
        } 
      } else {
	/*if (write_flag) 
          printf("  zero_flag = 1, [j=%d index=%d i=%d]\n",j,index,i);*/
	sf_not_out++;
      }
      index++;
    }
  }
  /*  if (write_flag) printf("sf_out = %d  sf_not_out = %d\n",sf_out,sf_not_out);*/
  return(bit_count);
}



/*****************/
/* corresponds to ISO/IEC 14496-3 Table 4.54 ms_data(),
 * ...but additionally writes ms_mask_present first
 */
int write_ms_data(HANDLE_AACBITMUX bitmux,
	int ms_mask,
	int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
	int num_window_groups,
	int nr_of_sfb_prev,
	int nr_of_sfb)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int i,j;
  int ms_mask_write = ms_mask;

  HANDLE_BSBITSTREAM bs_MS_MASK_PRESENT = aacBitMux_getBitstream(bitmux, MS_MASK_PRESENT);
  HANDLE_BSBITSTREAM bs_MS_USED = aacBitMux_getBitstream(bitmux, MS_USED);
  if (write_flag&&((bs_MS_MASK_PRESENT==NULL)||(bs_MS_USED==NULL))) {
    CommonWarning("aacBitMux: error writing ms_data()");	
    write_flag=0;
  }

  /* try to optimize the ms_mask (0=all off, 2=all on) */
  if (ms_mask_write == 1) {
    int first = -1;
    for (i=0; i<num_window_groups; i++) {
      for (j=nr_of_sfb_prev; j<nr_of_sfb; j++) {
        if (first == -1) {
          ms_mask_write = ms_used[i][j]?2:0;
          first = ms_used[i][j];
        }
        if (ms_used[i][j]!=first) {
          ms_mask_write = 1;
        }
      }
    }
  }

  /* write the ms_mask_present flags */
  if (write_flag) BsPutBit(bs_MS_MASK_PRESENT, ms_mask_write, LEN_MASK_PRES);
  bit_count += LEN_MASK_PRES;
  
  if (ms_mask_write == 1) {
    for (i=0; i<num_window_groups; i++) {
      for (j=nr_of_sfb_prev; j<nr_of_sfb; j++) {
        if (write_flag) BsPutBit(bs_MS_USED, ms_used[i][j], LEN_MASK);
        bit_count += LEN_MASK;
      }
    }
  }

  return bit_count;
}


static int find_grouping_bits(int window_group_length[],
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


int
write_basic_ics_info (HANDLE_AACBITMUX bitmux, 
        int max_sfb, 
	WINDOW_SEQUENCE windowSequence, WINDOW_SHAPE window_shape,
	int num_window_groups, int window_group_length[],
	int write_full_info)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int tmpVar = 0;

  HANDLE_BSBITSTREAM bs_ICS_RESERVED = aacBitMux_getBitstream(bitmux, ICS_RESERVED);
  HANDLE_BSBITSTREAM bs_WINDOW_SEQ = aacBitMux_getBitstream(bitmux, WINDOW_SEQ);
  HANDLE_BSBITSTREAM bs_WINDOW_SHAPE_CODE = aacBitMux_getBitstream(bitmux, WINDOW_SHAPE_CODE);
  HANDLE_BSBITSTREAM bs_SCALE_FACTOR_GROUPING = aacBitMux_getBitstream(bitmux, SCALE_FACTOR_GROUPING);
  HANDLE_BSBITSTREAM bs_MAX_SFB = aacBitMux_getBitstream(bitmux, MAX_SFB);
  if (write_flag&&((bs_ICS_RESERVED==NULL)||(bs_WINDOW_SEQ==NULL)||
                   (bs_WINDOW_SHAPE_CODE==NULL)||(bs_SCALE_FACTOR_GROUPING==NULL)||
                   (bs_MAX_SFB==NULL))) {
    CommonWarning("aacBitMux: error writing ics_info()");
    write_flag=0;
  }

  /* ics_reserved_bit */
  if (write_flag) BsPutBit(bs_ICS_RESERVED, 0, 1);
  bit_count +=1;

  if (write_full_info) {
    /* write out WINDOW_SEQUENCE */
    switch (windowSequence) {
    case EIGHT_SHORT_SEQUENCE:
      tmpVar=2;
      break;
    case ONLY_LONG_SEQUENCE:
      tmpVar=0;  
      break;
    case LONG_START_SEQUENCE:
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
#ifdef AAC_ELD
    case WS_FHG_LDFB:
#endif
      tmpVar = 0;
      break;
    case WS_DOLBY:
    case WS_ZPW:
      tmpVar = 1;
      break;
    default:
      CommonExit(-1,"\nunknow windowshape : %d", window_shape );
    }
    if (write_flag) BsPutBit(bs_WINDOW_SHAPE_CODE, tmpVar, 1);
    bit_count += 1;
  }

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

  return bit_count;
}


/* corresponds to ISO/IEC 14496-3 Table 4.6 ics_info() */
int write_ics_info (HANDLE_AACBITMUX bitmux, 
                    ICSinfo *info)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_PREDICTOR_DATA_PRESENT = aacBitMux_getBitstream(bitmux, PREDICTOR_DATA_PRESENT);
  if (write_flag&&(bs_PREDICTOR_DATA_PRESENT==NULL)) {
    CommonWarning("aacBitMux: error writing ics_info()");
    write_flag=0;
  }

  bit_count += write_basic_ics_info(bitmux, info->max_sfb,
                           info->windowSequence, info->window_shape,
                           info->num_window_groups, info->window_group_length,
                           1 /* full info */);

  if (info->windowSequence!=EIGHT_SHORT_SEQUENCE) {
    switch(info->predictor_type) {    
    case MONOPRED:
      CommonExit(-1,"\nMonopred not implemented");
      break;
    case NOK_LTP:
      {
	int ltp_present = 0;
        QC_MOD_SELECT qc_mode = (info->ld_mode?AAC_LD:AAC_QC);

	if ((info->nok_lt_status[0])->global_pred_flag)
	  ltp_present = 1;
	if (info->stereo_flag)
	  if ((info->nok_lt_status[1])->global_pred_flag)
	    ltp_present = 1;

	if (write_flag) BsPutBit(bs_PREDICTOR_DATA_PRESENT, ltp_present, 1);
	bit_count += 1;

	if (ltp_present) {
	  /* predictor data for left channel. */
	  bit_count += 
            nok_ltp_encode(bitmux, NULL,
                           info->windowSequence, info->max_sfb, 
                           info->nok_lt_status[0], qc_mode,
                           NO_CORE, -1);
          
	  /* predictor data for right channel. */
	  if (info->stereo_flag) {
            
            if (write_flag) bitmux->currentChannel++;
	    bit_count += 
              nok_ltp_encode(bitmux, NULL,
                             info->windowSequence, info->max_sfb, 
                             info->nok_lt_status[1], qc_mode,
                             NO_CORE, -1);
            if (write_flag) bitmux->currentChannel--;
          }
	}
      }
      break;

    case PRED_NONE:
      if (write_flag) BsPutBit(bs_PREDICTOR_DATA_PRESENT, 0, 1);  /* no prediction_data_present */
      bit_count += 1; 
      break;
        
    default:
      CommonExit(-1,"\nUnknown preditcor type");
    }
  }
  return(bit_count);
}


/********************************************************************************/

/********************************************************************************/

#define FSS_BANDS 16
typedef struct {
  int code[FSS_BANDS];
  int length[FSS_BANDS];
}FSS_CODE;

static FSS_CODE fssCode={
  {0,20,21,22,23,24,25,8,9,26,27,28,29,30,31,1},
  {2, 5, 5, 5, 5, 5, 5,4,4, 5, 5, 5, 5, 5, 5, 2}
};

/* corresponds to ISO/IEC 14496-3 Table 4.17 diff_control_data() */
static int write_fss_data(
  HANDLE_AACBITMUX   bitmux,
  WINDOW_SEQUENCE    windowSequence,
  unsigned int       numDiffCtrlBands,
  enum DC_FLAG       FssControl[]
)
{
  int write_flag = (bitmux!=NULL);
  int i;
  int used_bits = 0;

  HANDLE_BSBITSTREAM bs_DIFF_CONTROL = aacBitMux_getBitstream(bitmux, DIFF_CONTROL);
  if (write_flag&&(bs_DIFF_CONTROL==NULL)) {
    CommonWarning("aacBitMux: error writing diff_control_data()");
    write_flag=0;
  }

#ifdef DEBUG_FSS_BITS
  fprintf(stderr,"write_fss_data...\n");
#endif
  if( windowSequence != EIGHT_SHORT_SEQUENCE ) {
    unsigned int fssGroup;
    unsigned int fssNoGroups = numDiffCtrlBands >> 2 ;
    for( fssGroup=0; fssGroup<fssNoGroups;  fssGroup++ ) {
      unsigned int group = 0;
      for( i=0; i<4; i++ ) {
        group <<= 1;
        if( FssControl[(fssGroup<<2)+i] == DC_SIMUL ) {
          group |= 0x1;
        }
      }
      if (write_flag) BsPutBit(bs_DIFF_CONTROL, fssCode.code[group], fssCode.length[group] ); 
      used_bits += fssCode.length[group];
    }
#ifdef DEBUG_FSS_BITS
    fprintf(stderr,"...huffman stuff: %i bits\n",used_bits);
#endif
  } else {
    for( i=0; i<NSHORT; i++ ) {
      if (write_flag) BsPutBit(bs_DIFF_CONTROL, FssControl[i], 1 ); 
      used_bits += 1;
    }
#ifdef DEBUG_FSS_BITS
    fprintf(stderr,"...eight short: %i bits\n",used_bits);
#endif
  }
  return used_bits;
}

/* corresponds to ISO/IEC 14496-3 Table 4.18 diff_control_data_lr() */
static int write_fss_data_lr(
  HANDLE_AACBITMUX   bitmux,
  WINDOW_SEQUENCE    windowSequence,
  enum DC_FLAG       FssControl[],
  int                ms_mask[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  int                last_max_sfb_ms,
  int                max_sfb_used
)
{
  int write_flag = (bitmux!=NULL);
  int i;
  int used_bits = 0;
 
  HANDLE_BSBITSTREAM bs_DIFF_CONTROL_LR = aacBitMux_getBitstream(bitmux, DIFF_CONTROL_LR);
  if (write_flag&&(bs_DIFF_CONTROL_LR==NULL)) {
    CommonWarning("aacBitMux: error writing diff_control_data_lr()");
    write_flag=0;
  }

#ifdef DEBUG_FSS_BITS
  fprintf(stderr,"write_fss_data_LR !!!\n");
#endif
  if( windowSequence != EIGHT_SHORT_SEQUENCE ) {
    for( i=last_max_sfb_ms; i < max_sfb_used; i++ ) {
      if( ms_mask[0][i] == JS_MASK_OFF ) {
        if (write_flag) BsPutBit(bs_DIFF_CONTROL_LR, FssControl[i], 1);
        used_bits += 1;
      }
    }
#ifdef DEBUG_FSS_BITS
    fprintf(stderr,"...from %i to %i used %i bits\n",last_max_sfb_ms, max_sfb_used, used_bits);
#endif
  } else {
    if (last_max_sfb_ms == 0) {
      for( i=0; i<NSHORT; i++ ) {
        if (write_flag) BsPutBit(bs_DIFF_CONTROL_LR, FssControl[i], 1);
        used_bits += 1;
      }
    }
#ifdef DEBUG_FSS_BITS
    fprintf(stderr,"...eight short from %i used %i bits\n",last_max_sfb_ms, used_bits);
#endif
  }
  return used_bits;
}


/* corresponds to ISO/IEC 14496-3 Table 4.15 aac_scalable_main_header() */
int write_scalable_main_header (
	HANDLE_AACBITMUX bitmux, 
	int max_sfb,
	int max_sfb_prev,
	WINDOW_SEQUENCE windowSequence, WINDOW_SHAPE window_shape,
	int num_window_groups, int window_group_length[],
	int isFirstTfLayer,
	int isStereoLayer,
	int isFirstStereoLayer,
	int fss_bands,
	enum DC_FLAG FssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX],
	enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX],
	int ms_mask,
	int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
	int tns_transmitted[2],
	TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
	PRED_TYPE pred_type, 
	NOK_LT_PRED_STATUS *nok_lt_statusLeft,
	NOK_LT_PRED_STATUS *nok_lt_statusRight)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int isFirstLayer = max_sfb_prev<=0;
  int last_max_sfb_ms = isFirstStereoLayer ? 0 : max_sfb_prev;
  int ch;
  NOK_LT_PRED_STATUS *nok_lt_status[2];
  
  HANDLE_BSBITSTREAM bs_TNS_CHANNEL_MONO = NULL;

  if ( isStereoLayer ) {
    bs_TNS_CHANNEL_MONO = aacBitMux_getBitstream(bitmux, TNS_CHANNEL_MONO);
    if (write_flag&&(bs_TNS_CHANNEL_MONO==NULL)) {
      CommonWarning("aacBitMux: error writing aac_scalable_main_header()\n");
      write_flag=0;
    }
  }

  nok_lt_status[0] = nok_lt_statusLeft;
  nok_lt_status[1] = nok_lt_statusRight;

  bit_count += write_basic_ics_info(bitmux, max_sfb,
		windowSequence, window_shape,
		num_window_groups, window_group_length,
		isFirstTfLayer /* full info */);

  /* MS mask */
  if ( isStereoLayer ) {
    /* write the ms_mask flags */
    bit_count += write_ms_data(bitmux, ms_mask, ms_used,
			num_window_groups, last_max_sfb_ms, max_sfb);
  }

  /* tns_channel_mono_layer data */
  if (isFirstStereoLayer && !tns_transmitted[MONO_CHAN]) {
#ifdef DEBUG_TNS_BITS
    fprintf(stderr,"write tns_channel_mono_layer bit\n");
#endif
    if (write_flag) BsPutBit(bs_TNS_CHANNEL_MONO, 0, 1);
    bit_count += 1;
  }

  for( ch=0; ch<(isStereoLayer?2:1); ch++ ) {
    
    if (write_flag) bitmux->currentChannel+=ch;
    
    /* TNS data */
    if (isFirstTfLayer || (tns_transmitted[ch]==0) || isFirstStereoLayer) {
#ifdef DEBUG_TNS_BITS
      fprintf(stderr,"write tns flag %i\n",ch);
#endif
      bit_count += write_tns_data(bitmux, 0, tnsInfo[ch], windowSequence, NULL);
    }

    /* FSS data */
    if (!isFirstLayer) {
      if ((ch==0)||((ch==1)&&(!isFirstStereoLayer)))
        bit_count += write_fss_data(bitmux, windowSequence, fss_bands, FssControl[ch] );
      if (isFirstStereoLayer)
        /* will break if sfb is decreasing, as max_sfb_prev should be
           min(max_sfb, higest_mono_max_sfb)
           ...but then again, decrease of max_sfb is prohibited
           kaehleof 20040909
        */
        write_fss_data_lr(bitmux, windowSequence,
                          msFssControl[ch],
                          ms_used, last_max_sfb_ms, max_sfb_prev);
    } else {
      NOK_LT_PRED_STATUS *lt_status = NULL;
      if (pred_type == NOK_LTP) lt_status = nok_lt_status[ch];
        
      /* predictor data for this channel. */
      bit_count += 
        nok_ltp_encode(bitmux, NULL, windowSequence, max_sfb, 
                       lt_status, AAC_SYS,
                       NO_CORE, -1);
    }
    if (write_flag) bitmux->currentChannel-=ch;
  }
  return(bit_count);
}


/********************************************************************************/

/********************************************************************************/

/* corresponds to ISO/IEC 14496-3 Table 4.16 aac_scalable_extension_header() */
int write_scalable_ext_header(
	HANDLE_AACBITMUX bitmux,
	WINDOW_SEQUENCE windowSequence,
	int num_window_groups,
	int max_sfb,
	int max_sfb_prev,
	int isStereoLayer,
	int isFirstStereoLayer,
        int hasMonoLayer,
	enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX],
	int ms_mask,
	int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
	TNS_INFO *tnsInfo[MAX_TIME_CHANNELS])
{
  int write_flag = (bitmux!=NULL);
  int used_bits = 0;
  int ch;
  int max_sfb_prev_ms = isFirstStereoLayer?0:max_sfb_prev;

  HANDLE_BSBITSTREAM bs_MAX_SFB = aacBitMux_getBitstream(bitmux, MAX_SFB);
  if (write_flag&&(bs_MAX_SFB==NULL)) {
    CommonWarning("aacBitMux: error writing aac_scalable_extension_header()\n");
    write_flag=0;
  }

  if( windowSequence == EIGHT_SHORT_SEQUENCE ){
    if (write_flag) BsPutBit(bs_MAX_SFB, max_sfb, 4); /* max_sfb */
    used_bits += 4;
  } else {
    if (write_flag) BsPutBit(bs_MAX_SFB, max_sfb, 6); /* max_sfb */
    used_bits += 6;
  }

  /* MS mask */
  if ( isStereoLayer ) {
    /* write the ms_mask flags */
    used_bits += write_ms_data(bitmux, ms_mask, ms_used,
			num_window_groups, max_sfb_prev_ms, max_sfb);
  }

  /* TNS data */
  if ( isFirstStereoLayer ) {
    for( ch=0; ch<2; ch++ ) {
      used_bits += write_tns_data(bitmux, 0, tnsInfo[ch], windowSequence, NULL);
    }
  }

  /* FSS data */
  if ( hasMonoLayer && isStereoLayer ) {
    int ch;
    for( ch=0; ch<2; ch++ ) {
      /* will break if sfb is decreasing, as max_sfb_prev should be
         min(max_sfb, higest_mono_max_sfb)
         ...but then again, decrease of max_sfb is prohibited
         kaehleof 20040909
      */
      used_bits += write_fss_data_lr(bitmux, windowSequence, msFssControl[ch], ms_used, max_sfb_prev_ms, isFirstStereoLayer?max_sfb_prev:0 );
    }
  }

  return used_bits;
}




/*****************************************************************************/
/* write_tns_data(...), write TNS data.                            */
/*****************************************************************************/
/* corresponds to ISO/IEC 14496-3 Table 4.48 tns_data()
 * ...but this one will also write the tns_data_present bit
 */
int write_tns_data(HANDLE_AACBITMUX   bitmux,
                   int                bUsacSyntax,
                   TNS_INFO*          tnsInfoPtr,       /* TnsInfo structure */
                   WINDOW_SEQUENCE    windowSequence,     /* block type */
                   HANDLE_BSBITSTREAM stream)
{
  int write_flag = (bitmux!=NULL)||(stream!=NULL);
  int bit_count = 0;
  int numWindows = 1;
  int len_tns_nfilt;
  int len_tns_length;
  int len_tns_order;
  int filtNumber;
  int resInBits;
  unsigned long unsignedIndex;
  int w;

  HANDLE_BSBITSTREAM bs_TNS_DATA_PRESENT = aacBitMux_getBitstream(bitmux, TNS_DATA_PRESENT);
  HANDLE_BSBITSTREAM bs_COEF = aacBitMux_getBitstream(bitmux, COEF);
  HANDLE_BSBITSTREAM bs_COEF_COMPRESS = aacBitMux_getBitstream(bitmux, COEF_COMPRESS);
  HANDLE_BSBITSTREAM bs_COEF_RES = aacBitMux_getBitstream(bitmux, COEF_RES);
  HANDLE_BSBITSTREAM bs_DIRECTION = aacBitMux_getBitstream(bitmux, DIRECTION);
  HANDLE_BSBITSTREAM bs_LENGTH = aacBitMux_getBitstream(bitmux, LENGTH);
  HANDLE_BSBITSTREAM bs_N_FILT = aacBitMux_getBitstream(bitmux, N_FILT);
  HANDLE_BSBITSTREAM bs_ORDER = aacBitMux_getBitstream(bitmux, ORDER);
  if (bitmux==NULL) {
    bs_TNS_DATA_PRESENT = bs_COEF = bs_COEF_COMPRESS = bs_DIRECTION =
      bs_LENGTH = bs_N_FILT = bs_ORDER = stream;
  }
  if (write_flag&&((bs_TNS_DATA_PRESENT==NULL)||(bs_COEF==NULL)||
                   (bs_COEF_COMPRESS==NULL)||(bs_COEF_RES==NULL)||
                   (bs_DIRECTION==NULL)||(bs_LENGTH==NULL)||
                   (bs_N_FILT==NULL)||(bs_ORDER==NULL))) {
    CommonWarning("aacBitMux: error writing tns_data()");
    write_flag=0;
  }

  w = (tnsInfoPtr!=NULL);
  if (w) w=tnsInfoPtr->tnsDataPresent;

  /* If TNS is not present, bail */
  if (!w) {
    return bit_count;
  }

  /* Set window-dependent TNS parameters */
  if (windowSequence == EIGHT_SHORT_SEQUENCE) {
    numWindows = MAX_SHORT_IN_LONG_BLOCK;
    len_tns_nfilt = LEN_TNS_NFILTS;
    len_tns_length = LEN_TNS_LENGTHS;
    len_tns_order = LEN_TNS_ORDERS;
  } else {
    numWindows = 1;
    len_tns_nfilt = LEN_TNS_NFILTL;
    len_tns_length = LEN_TNS_LENGTHL;
    len_tns_order = bUsacSyntax?LEN_TNS_ORDERL_USAC:LEN_TNS_ORDERL;
  }

  /*fprintf(stderr,"numWindows %5d len_tns_nfilt %5d\n",numWindows,len_tns_nfilt);*/

  /* Write TNS data */
  /*  fprintf(stderr,"TNS count (1) %5d\n",bit_count);*/
  for (w=0;w<numWindows;w++) {
    TNS_WINDOW_DATA* windowDataPtr = &tnsInfoPtr->windowData[w];
    int numFilters = windowDataPtr->numFilters;
    /*	fprintf(stderr,"numFilters %5d\n",numFilters);*/
    if (write_flag) {
      BsPutBit(bs_N_FILT, numFilters, len_tns_nfilt); /* n_filt[] = 0 */
    }
    bit_count += len_tns_nfilt;
    if (numFilters) {
      /*  fprintf(stderr,"TNS count (2) %5d\n",bit_count);*/
      resInBits = windowDataPtr->coefResolution;
      if (write_flag) {
	BsPutBit(bs_COEF_RES,resInBits-DEF_TNS_RES_OFFSET,LEN_TNS_COEFF_RES);
      }
      bit_count += LEN_TNS_COEFF_RES;
      /*  fprintf(stderr,"TNS count (3) %5d\n",bit_count);*/
      for (filtNumber=0;filtNumber<numFilters;filtNumber++) {
	TNS_FILTER_DATA* tnsFilterPtr=&windowDataPtr->tnsFilter[filtNumber];
	int order = tnsFilterPtr->order;
	if (write_flag) {
          /*  fprintf(stderr,"&& %5d\n",tnsFilterPtr->length);*/
	  BsPutBit(bs_LENGTH, tnsFilterPtr->length, len_tns_length);
	  BsPutBit(bs_ORDER, order, len_tns_order);
	}
        bit_count += (len_tns_length+len_tns_order);
	if (order) {
	  int i;
          /*   fprintf(stderr,"TNS count (4) %5d\n",bit_count);*/
	  if (write_flag) {
	    BsPutBit(bs_DIRECTION, tnsFilterPtr->direction, LEN_TNS_DIRECTION);
	    BsPutBit(bs_COEF_COMPRESS, tnsFilterPtr->coefCompress, LEN_TNS_COMPRESS);
	  }
	  bit_count += (LEN_TNS_DIRECTION + LEN_TNS_COMPRESS);
          /*   fprintf(stderr,"TNS count (5) %5d\n",bit_count);*/
	  for (i=1;i<=order;i++) {
	    if (write_flag) {
	      unsignedIndex = (unsigned long) (tnsFilterPtr->index[i])&(~(~0<<resInBits));
	      BsPutBit(bs_COEF, unsignedIndex, resInBits);
            }
	    bit_count += resInBits;
	  }
	}
      }
    }
  }
  return bit_count;
}


/* corresponds to ISO/IEC 14496-3 Table 4.49 spectral_data() */
static int write_spectral_data(HANDLE_AACBITMUX bitmux,
                               int counter, int len[], int data[])
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;
  int i;

  /* FIXME: this is not very nice, as there is no difference in
     HCOD, HCOD_ESC, SIGN_BITS
     anyway, all these end up in the same esc according to current standard
  */
  HANDLE_BSBITSTREAM bs_spectral_data = aacBitMux_getBitstream(bitmux, HCOD);
  if (write_flag&&(bs_spectral_data==NULL)) {
    CommonWarning("aacBitMux: error writing spectral_data()");
    write_flag=0;
  }

  for(i=0; i<counter; i++) {
    if (len[i] > 0) {  /* only send out non-zero codebook data */
      if (write_flag) BsPutBit(bs_spectral_data, data[i], len[i]);
      bit_count += len[i];
    }
  }
  return (bit_count);
}


/* write single channel element header */
int write_aac_sce(HANDLE_AACBITMUX bitmux, int tag, int write_ele_id)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_ELEMENT_INSTANCE_TAG = aacBitMux_getBitstream(bitmux, ELEMENT_INSTANCE_TAG);
  if (write_flag&&(bs_ELEMENT_INSTANCE_TAG==NULL)) {
    CommonWarning("aacBitMux: error writing aac_sce()");
    write_flag=0;
  }

  if (write_ele_id) {
    /* write ID_SCE, single_element_channel() identifier... */
    if (write_flag) BsPutBit(bs_ELEMENT_INSTANCE_TAG, ID_SCE, LEN_SE_ID);  
    bit_count += 3;
  }

  /* write the element_identifier_tag */
  if (write_flag) BsPutBit(bs_ELEMENT_INSTANCE_TAG, tag, LEN_TAG);
  bit_count += 4;

  return (bit_count);
}

int write_aac_cpe(HANDLE_AACBITMUX bitmux,
                  int tag,
                  int write_ele_id,
                  int common_window,
                  int ms_mask,
                  int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
                  int num_window_groups,
                  int nr_of_sfb,
                  ICSinfo *ics_info)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_ELEMENT_INSTANCE_TAG = aacBitMux_getBitstream(bitmux, ELEMENT_INSTANCE_TAG);
  HANDLE_BSBITSTREAM bs_COMMON_WINDOW = aacBitMux_getBitstream(bitmux, COMMON_WINDOW);
  if (write_flag&&((bs_ELEMENT_INSTANCE_TAG==NULL)||(bs_COMMON_WINDOW==NULL))) {
    CommonWarning("aacBitMux: error writing aac_cpe()");
    write_flag=0;
  }

  if (write_ele_id) {
    /* write ID_CPE, channel_pair_element identifier */
    if (write_flag) BsPutBit(bs_ELEMENT_INSTANCE_TAG, ID_CPE, LEN_SE_ID);
    bit_count += LEN_SE_ID;
  }

  /* write the element_instance_tag */
  if (write_flag) BsPutBit(bs_ELEMENT_INSTANCE_TAG, tag, LEN_TAG);
  bit_count += LEN_TAG;

  /* write the common_window flag */
  if (write_flag) BsPutBit(bs_COMMON_WINDOW, common_window, LEN_COM_WIN);
  bit_count += LEN_COM_WIN;

  if (common_window) {
    if (ics_info!=NULL) {
      bit_count += write_ics_info(bitmux, ics_info);
    }

    /* write the ms_mask flags */
    bit_count += write_ms_data(bitmux, ms_mask, ms_used,
                               num_window_groups, 0, nr_of_sfb);
  }
  return (bit_count);
}


int write_ics_nonscalable_extra(
  HANDLE_AACBITMUX bitmux,
  WINDOW_SEQUENCE windowSequence,
  ToolsInfo *info)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_PULSE_DATA_PRESENT = aacBitMux_getBitstream(bitmux, PULSE_DATA_PRESENT);
  HANDLE_BSBITSTREAM bs_GAIN_CONTROL_DATA_PRESENT = aacBitMux_getBitstream(bitmux, GAIN_CONTROL_DATA_PRESENT);
  if (write_flag&&((bs_PULSE_DATA_PRESENT==NULL)||(bs_GAIN_CONTROL_DATA_PRESENT==NULL))) {
    CommonWarning("aacBitMux: error writing individual_channel_stream()");
    write_flag=0;
  }

  /* write pulse data, currently not supported */
  if (write_flag) BsPutBit(bs_PULSE_DATA_PRESENT, 0/*info->pulse_data_present*/, LEN_PULSE_PRES);
  bit_count += LEN_PULSE_PRES;
  if (info->pulse_data_present) {
    CommonWarning("aacBitMux: pulse_data() not implemented!!!");
  }

  /* write TNS data */
  bit_count += write_tns_data(bitmux, 0, info->tnsInfo, windowSequence, NULL);

  /* write gain control data */
  if (write_flag) BsPutBit(bs_GAIN_CONTROL_DATA_PRESENT, info->gc_data_present, LEN_GAIN_PRES);
  bit_count += LEN_GAIN_PRES;
  if (info->gc_data_present) {
    /* FIXME: luckily gain control is not supported in case of ep syntax */
    bit_count += son_gc_pack(bs_GAIN_CONTROL_DATA_PRESENT, windowSequence, info->max_band, info->gainInfo);
  }

  return bit_count;
}


/* corresponds to ISO/IEC 14496-3 Table 4.44 individual_channel_stream() */
int write_ind_cha_stream(
  HANDLE_AACBITMUX bitmux,
  WINDOW_SEQUENCE windowSequence,
  int global_gain,
  int scale_factors[],
  int book_vector[],
  int data[],
  int len[],
  const int huff[13][1090][4],
  int counter,
  int nr_of_sfb,
  int num_window_groups,
  const int noise_nrg[],
  ICSinfo *ics_info,
  ToolsInfo *tool_data,
  int common_window,
  int scale_flag,
  int qdebug)
{
  int write_flag = (bitmux!=NULL);
  int bit_count = 0;

  HANDLE_BSBITSTREAM bs_GLOBAL_GAIN = aacBitMux_getBitstream(bitmux, GLOBAL_GAIN);
  if (write_flag&&(bs_GLOBAL_GAIN==NULL)) {
    CommonWarning("aacBitMux: error writing individual_channel_stream()");
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

  if ((!common_window) && (!scale_flag) && (ics_info!=NULL)) {
    bit_count += write_ics_info(bitmux, ics_info);
  }


  /* write out section_data() information to the bitstream */
  bit_count += sort_book_numbers(bitmux,
                                 book_vector,
				 nr_of_sfb,
				 windowSequence,
				 num_window_groups,
				 qdebug);


  /* scale_factor_data() information */
  bit_count += write_scalefactor_bitstream(bitmux,
                                           0,
                                           nr_of_sfb, scale_factors,
					   book_vector,
					   num_window_groups,
					   global_gain,
					   windowSequence, noise_nrg,
					   huff,
                                           qdebug);


  if ((!scale_flag)&&(tool_data!=NULL)) {
    /* write pulse_data, tns_data, gain_control_data */
    bit_count += write_ics_nonscalable_extra(bitmux, windowSequence, tool_data);
  }

  /* write out the spectral_data() */
  if ((len!=NULL)&&(data!=NULL)) {
    bit_count += write_spectral_data(bitmux, counter, len, data);
  }

  return(bit_count);
}


int write_fill_elements(HANDLE_AACBITMUX bitmux, int numFillBits)
{
  int write_flag = (bitmux!=NULL);
  int i;
  int cnt;
  int bitCount = numFillBits;

  HANDLE_BSBITSTREAM bs_fill_elements = aacBitMux_getBitstream(bitmux, ELEMENT_INSTANCE_TAG);
  if (write_flag&&(bs_fill_elements==NULL)) {
    CommonWarning("aacBitMux: error writing fill_element()");
    write_flag=0;
  }

  /* write fill elements */    
  while(bitCount>7) {
    /* Fill elements ID */
    if (write_flag) BsPutBit(bs_fill_elements, ID_FIL, LEN_SE_ID);
    bitCount -= LEN_SE_ID;
    
    /* check for escape values */
    cnt = (bitCount-4)>>3;
    if(cnt>=15) {

      if (write_flag) BsPutBit(bs_fill_elements, 15, 4);
      bitCount -= 4;

      if (write_flag) BsPutBit(bs_fill_elements, ((cnt-15)>255)?255:cnt-15, 8);  
      bitCount -= 8;
      cnt = ((cnt>270)?270:cnt)-1;
 
    } else {
      if (write_flag) BsPutBit(bs_fill_elements, cnt, 4);
      bitCount -= 4;
    }

    for(i=0; i<cnt; i++) {
      if (write_flag) BsPutBit(bs_fill_elements, 0, 8);
      bitCount -= 8;
    }

  } /* while(bitCount>7) */

  return (numFillBits-bitCount);
}

int write_padding_bits(HANDLE_AACBITMUX bitmux, int padding_bits)
{
  int write_flag = (bitmux!=NULL);
  int bitCount = 0;
  HANDLE_BSBITSTREAM bs_padding=NULL;

  if (write_flag) bs_padding = bitmux->streams[bitmux->numStreams-1];

  while (padding_bits>0) {
    int tmp = MIN(8,padding_bits);
    if (write_flag) BsPutBit(bs_padding, 0, tmp);
    bitCount += tmp;
    padding_bits -= tmp;
  }

  return bitCount;
}

int write_aac_end_id(HANDLE_AACBITMUX bitmux)
{
  int write_flag = (bitmux!=NULL);

  HANDLE_BSBITSTREAM bs_ELEMENT_INSTANCE_TAG = aacBitMux_getBitstream(bitmux, ELEMENT_INSTANCE_TAG);
  if (write_flag&&(bs_ELEMENT_INSTANCE_TAG==NULL)) {
    CommonWarning("aacBitMux: error writing id_end");
    write_flag=0;
  }

  if (write_flag) BsPutBit(bs_ELEMENT_INSTANCE_TAG, ID_END, LEN_SE_ID);

  return (LEN_SE_ID);
}


