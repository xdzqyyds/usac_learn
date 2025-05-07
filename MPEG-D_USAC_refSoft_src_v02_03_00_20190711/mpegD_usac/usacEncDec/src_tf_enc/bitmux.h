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
/* Bitmux.h  : Handling of bistream format for AAC Raw and AAC SCALABLE     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#ifndef _bitmux_h_
#define _bitmux_h_

#include "block.h"
#include "tf_mainStruct.h"
#include "tf_main.h"
#include "tns3.h"
#include "sony_local.h"

typedef struct {
  int max_sfb;
  WINDOW_SEQUENCE windowSequence;
  WINDOW_SHAPE window_shape;
  int num_window_groups;
  int window_group_length[8 /*MAX_WINDOW_GROUPS*/];
  int ld_mode;
  PRED_TYPE predictor_type;
  NOK_LT_PRED_STATUS *nok_lt_status[2];
  int stereo_flag;
} ICSinfo;

typedef struct {
  int pulse_data_present;
  int gc_data_present;
  int max_band;
  GAINC **gainInfo;
  int tns_data_present;
  TNS_INFO *tnsInfo;
} ToolsInfo;


/* new bitmux struct by kaehleof */
typedef struct tagAACbitMux *HANDLE_AACBITMUX;
HANDLE_AACBITMUX aacBitMux_create(HANDLE_BSBITSTREAM *streams,
                                  int numStreams,
                                  int numChannels,
                                  int ep_config,
                                  int debugLevel);
void aacBitMux_free(HANDLE_AACBITMUX mux);
void aacBitMux_setCurrentChannel(HANDLE_AACBITMUX bitmux,
                                 int channel);
void aacBitMux_setAssignmentScheme(HANDLE_AACBITMUX bitmux,
                                   int numChannels,
                                   int common_window);
HANDLE_BSBITSTREAM aacBitMux_getBitstream(HANDLE_AACBITMUX bitmux,
                                          CODE_TABLE code);



/* corresponds to ISO/IEC 14496-3 Table 4.6 ics_info() */
int write_ics_info(
  HANDLE_AACBITMUX bitmux, 
  ICSinfo *info);

int
write_basic_ics_info (HANDLE_AACBITMUX bitmux, 
        int max_sfb, 
	WINDOW_SEQUENCE windowSequence, WINDOW_SHAPE window_shape,
	int num_window_groups, int window_group_length[],
		      int write_full_info);
int write_ms_data(HANDLE_AACBITMUX bitmux,
	int ms_mask,
	int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
	int num_window_groups,
	int nr_of_sfb_prev,
		  int nr_of_sfb);

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
				int qdebug);

/* corresponds to ISO/IEC 14496-3 Table 4.15 aac_scalable_main_header() */
int write_scalable_main_header(
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
  NOK_LT_PRED_STATUS *nok_lt_statusRight);

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
  TNS_INFO *tnsInfo[MAX_TIME_CHANNELS]);

/* corresponds to ISO/IEC 14496-3 Table 4.48 tns_data()
 * ...but this one will also write the tns_data_present bit
 * will preferably write to a stream from <bitmux>, but also try <stream>
 */
int write_tns_data(
  HANDLE_AACBITMUX   bitmux,
  int                bUsacSyntax,
  TNS_INFO*          tnsInfoPtr,
  WINDOW_SEQUENCE    windowSequence,
  HANDLE_BSBITSTREAM stream);

/* write single channel element header */
int write_aac_sce(
  HANDLE_AACBITMUX bitmux,
  int tag, int write_ele_id);

/* write channel pair element header */
int write_aac_cpe(
  HANDLE_AACBITMUX bitmux,
  int tag,
  int write_ele_id,
  int common_window,
  int ms_mask,
  int ms_used[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
  int num_window_groups,
  int nr_of_sfb,
  ICSinfo *ics_info);

/* write pulse_data, gain_control and tns_info */
int write_ics_nonscalable_extra(
  HANDLE_AACBITMUX bitmux,
  WINDOW_SEQUENCE windowSequence,
  ToolsInfo *info);


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
  int qdebug);

int write_fill_elements(
  HANDLE_AACBITMUX bitmux,
  int numFillBits);

int write_padding_bits(
  HANDLE_AACBITMUX bitmux,
  int padding_bits);

int write_aac_end_id(
  HANDLE_AACBITMUX bitmux);

#endif   /* define _bitmux_ */
