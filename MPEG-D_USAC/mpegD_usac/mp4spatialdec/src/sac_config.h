/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/





#ifndef _config_h_
#define _config_h_

#include "sac_chandefs.h"
#include "sac_interface.h"

typedef struct
{
    int present;	
    int tag;		
    int cpe;		
    int	common_window;	
    int	ch_is_left;	
    int	paired_ch;	
    int widx;		
    int is_present;	
    int ncch;		
#if (CChans > 0)
    int cch[CChans];	
    int cc_dom[CChans];	
    int cc_ind[CChans];	
    int cc_multi[CChans]; 
#endif
    char *fext;		
} Ch_Info;

typedef struct {
  int nch;		
  int nfsce;		
  int nfch;		
  int nsch;		
  int nbch;		
  int nlch;		
  int ncch;		
  int cc_tag[(1<<LEN_TAG)];	
  int cc_ind[(1<<LEN_TAG)];	
  int profile;
  int sampling_rate_idx; 
  int sampling_rate;     
#ifdef CT_SBR
  float upsamplingFactor;
  int bDownSampledSbr;
  int HE_AAC_level;
  int sbrPresentFlag;
#ifdef PARAMETRICSTEREO
  int sbrEnablePS;
#endif
#endif
  Ch_Info ch_info[Chans];
} MC_Info;

typedef struct {
    int num_ele;
    int ele_is_cpe[(1<<LEN_TAG)];
    int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
    int present;
    int ele_tag;
    int pseudo_enab;
} MIXdown;

typedef struct {
    int profile;
    int sampling_rate_idx;
    EleList front;
    EleList side;
    EleList back;
    EleList lfe;
    EleList data;
    EleList coupling;
    MIXdown mono_mix;
    MIXdown stereo_mix;
    MIXdown matrix_mix;
    char comments[(1<<LEN_PC_COMM)+1];
} ProgConfig;

typedef struct {
    char    adif_id[LEN_ADIF_ID+1];
    int	    copy_id_present;
    char    copy_id[LEN_COPYRT_ID+1];
    int	    original_copy;
    int	    home;
    int	    bitstream_type;
    long    bitrate;
    int	    num_pce;
    int	    prog_tags[(1<<LEN_TAG)];
    long    buffer_fullness;	
} ADIF_Header;

typedef struct {
  int	syncword;
  int	id;
  int	layer;
  int	protection_abs;
  int 	profile;
  int	sampling_freq_idx;
  int	private_bit;
  int	channel_config;
  int	original_copy;
  int	home;
  int	copyright_id_bit;
  int	copyright_id_start;
  int	frame_length;
  int	adts_buffer_fullness;
  int	num_of_rdb;
  int	crc_check;
#if ((defined NEW_ADTS)||(defined WRITE_NEW_ADTS))
  int rdb_position[MAX_RDB_PER_FRAME-1];
  int crc_check_rdb;
#endif
} ADTS_Header;

#define ADTS_HEADER_SIZE(p)	(7+((p).protection_abs?0:(((p).num_of_rdb+1)*2)))


extern	ProgConfig	prog_config;
extern	MC_Info		mc_info;

extern	int		current_program;
extern	int		default_config;
extern	int		channel_num;
extern	int		sampling_rate[];


int		ch_index(MC_Info *mip, int cpe, int tag);
void		check_mc_info(MC_Info *mip, int new_config);
int		chn_config(int id, int tag, int common_window, MC_Info *mip, int* first_cpe);
int		enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);
int		get_prog_config(ProgConfig *p);
int		s_getNextSampFreqIndex(int sampRate);
int		s_tns_max_bands(int win);
int		s_tns_max_order(int islong);
int		pred_max_bands(void);
void		print_mc_info(MC_Info *mip);
void		print_adif_header(ADIF_Header *p);
void		print_adts_header(ADTS_Header *p);
void		print_PCE(ProgConfig *p);
void		reset_mc_info(MC_Info *mip);


#endif
