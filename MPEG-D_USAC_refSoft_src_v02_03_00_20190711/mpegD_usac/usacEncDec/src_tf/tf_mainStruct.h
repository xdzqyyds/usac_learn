/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER)
 and edited by

 in the course of 
 development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1999.
 *                                                                           *
 ****************************************************************************/

#ifndef _TF_MAIN_STRUCT_H_INCLUDED
#define _TF_MAIN_STRUCT_H_INCLUDED

#include "allHandles.h"
#include "vmtypes.h"
#include "block.h"
#include "interface.h"
#include "nok_ltp_common.h"      /* defines, typedefs */ 
#include "ntt_tvq.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

typedef struct
{
  int       islong;                     /* true if long block */
  int       nsbk;                       /* sub-blocks (SB) per block */
  int       bins_per_bk;                /* coef's per block */
  int       sfb_per_bk;                 /* sfb per block */
  int       bins_per_sbk[MAX_SBK];      /* coef's per SB */
  int       sfb_per_sbk[MAX_SBK];       /* sfb per SB */
  int       sectbits[MAX_SBK];          /* 5 for long, 3 for short */
  const short* sbk_sfb_top[MAX_SBK];       /* top coef per sfb per SB */
  short*    sfb_width_short;            /* sfb width for short blocks */
  short     bk_sfb_top[200];            /* cum version of above */
  int       num_groups;
  short     group_len[8];
  short     group_offs[8];
  int       shortFssWidth;
  int       longFssGroups;
} Info;

#ifdef I2R_LOSSLESS
typedef struct LIT_LL_decInfo_t
{
	int type_PCM;

	int bpc_L[MAXBANDS];
	int bpc_maxbitplane[MAXBANDS];
	

	unsigned char stream[MAX_OSF*LN2*4];  /* lossless stream*/
	long	stream_len; /* lossless stream len*/
} LIT_LL_decInfo;

typedef struct LIT_LL_quantInfo_t
{
  Info *info; 
  /* NEW: use complete Info structure from AAC. 
     This might make several other values obsolete. Todo: check */

  int quantCoef[1024];
  int ll_present;
  int num_window_groups;
  int num_sfb;
  int num_osf_sfb;
  int total_sfb_per_group;
  int total_sfb;
  /*int*/ byte table[MAXBANDS];
  /*int*/ short scale_factor[MAXBANDS];

  /*added in enhancer*/
  byte max_sfb;/*[MAX_TIME_CHANNELS];*/
  int osf_sfb_width;
  int sfb_offset_lossless_only_short[MAXBANDS];  
  int aac_sfb_offset[MAXBANDS]; /* sfb_offset for AAC core layer */
  int sls_sfb_offset[MAXBANDS]; 
  int aac_sfb_width[MAXBANDS]; 
  int sls_sfb_width[MAXBANDS]; 
} LIT_LL_quantInfo;
#endif

typedef struct {
  int                  tnsBevCore;
  int                  output_select;
  double*              time_sample_vector[MAX_OSF*MAX_TIME_CHANNELS];
  double*              spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS];
  WINDOW_SHAPE         windowShape[MAX_TIME_CHANNELS];
  WINDOW_SHAPE         prev_windowShape[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE      windowSequence[MAX_TIME_CHANNELS];
  double*              overlap_buffer[MAX_OSF*MAX_TIME_CHANNELS];
  NOK_LT_PRED_STATUS*  nok_lt_status[MAX_TIME_CHANNELS];
  PRED_TYPE            pred_type;
  Info**               sfbInfo;
  int                  block_size_samples;
  float*               sampleBuf[MAX_TIME_CHANNELS];
  HANDLE_MODULO_BUF_VM coreModuloBuffer;
  double*              mdct_overlap_buffer;
  WINDOW_SEQUENCE      windowSequenceLast[MAX_TIME_CHANNELS];
  int                  samplFreqFacCore;
  int                  decoded_bits;
  int                  mdctCoreOnly; /* debug switch */     

#ifdef I2R_LOSSLESS
  LIT_LL_quantInfo*    ll_quantInfo[MAX_TIME_CHANNELS][MAX_TF_LAYER];
  LIT_LL_decInfo *     ll_info[MAX_TIME_CHANNELS];

  int *ll_coef[MAX_TIME_CHANNELS];
  int *mdct_buf[MAX_TIME_CHANNELS] ;
  int *ll_timesig[MAX_TIME_CHANNELS] ;
#endif
  int osf;             /* this defaults to 1 for non-sls */

  /* BSAC scalable decode bitrate */
  int sam_DecBr;  /* YB : 970825 */

#ifdef CT_SBR
  /* 20060107 : SBR bitstream for BSAC  */
  SBRBITSTREAM	sbrBitStr_BSAC[MAX_TIME_CHANNELS];
#endif

#ifdef CT_SBR
 int                  bDownSampleSbr;
 int                  sbrPresentFlag;
 int                  runSbr;
#ifdef PARAMETRICSTEREO
 int                  sbrEnablePS;
#endif
#endif

  /*
    aac Decoder data
  */
  HANDLE_AACDECODER aacDecoder ;
  
  /*
    Tvq Decoder data
  */
  HANDLE_TVQDECODER tvqDecoder;

  /*
    sbr Decoder 
  */

#ifdef CT_SBR
  SBRDECODER ct_sbrDecoder;
#endif

#ifdef I2R_LOSSLESS
  /* SLSDECODER slsDecoder; */
#endif

} TF_DATA;

#endif  /* #ifndef _TF_STRUCT_MAIN_H_INCLUDED */


