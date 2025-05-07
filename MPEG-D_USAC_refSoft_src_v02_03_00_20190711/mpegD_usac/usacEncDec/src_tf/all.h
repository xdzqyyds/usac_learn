/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
Copyright(c)1996, 1997, 1998.
 *                                                                           *
 ****************************************************************************/

#ifndef _all_h_
#define _all_h_

#include <sys/types.h>
#include "vmtypes.h"

#include "allHandles.h"
#include "interface.h"
#include "block.h"
#include "tf_mainHandle.h"
#include "tf_mainStruct.h"
#include "tns.h"
#include "monopredStruct.h"
#include "sony_local.h"


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define nil                     ((void*)0)
#define Tnleaf                  0x8000

#define    FChans        15     /* front channels: left, center, right */
#define    FCenter       1      /* 1 if decoder has front center channel */
#define    SChans        18     /* side channels: */
#define    BChans        15     /* back channels: left surround, right surround */
#define    BCenter       1      /* 1 if decoder has back center channel */
#define    LChans        1      /* 1 * LFE channels */
#define    XChans        2      /* scratch space for parsing unused channels */  
#define    ICChans       3      /* 1 * independently switched coupling channels */
#define    DCChans       3      /* 2 * dependently switched coupling channels */
#define    XCChans       2      /* 1 * scratch space for parsing unused coupling channels */

#define    Chans         (FChans + SChans + BChans + LChans + XChans)
    
#define    CChans        (ICChans + DCChans + XCChans)

enum
{
  /* block switch windows for single channels or channel pairs */
  Winds       = Chans,
    
  /* average channel block length, bytes */
  Avjframe    = 341,  

  TEXP        = 128,          /* size of exp cache table */
  MAX_IQ_TBL  = 128,          /* size of inv quant table */
  MAXFFT      = LN4,

  XXXXX
};


typedef struct {
  int           samp_rate;
  int           nsfb1024;
  const short*  SFbands1024;
  int           nsfb128;
  const short*  SFbands128;
  int           nsfb960;
  const short*  SFbands960;
  int           nsfb120;
  const short*  SFbands120;
  int           nsfb768;
  const short*  SFbands768;
  int           nsfb96;
  const short*  SFbands96;
  int           shortFssWidth;
  int           longFssGroups;
  int           nsfb480;
  const short*  SFbands480;
  int           nsfb512;
  const short*  SFbands512;
} SR_Info;

typedef struct {
  	WINDOW_SHAPE    this_bk;
	WINDOW_SHAPE    prev_bk;
} Wnd_Shape;

typedef struct
{
  int           index;
  int           len;
  unsigned long cw;
} Huffman;

typedef struct
{
  int            n;
  int            dim;
  int            lav;
  int            lavInclEsc;
  int            mod;
  int            off;
  int            signed_cb;
  unsigned short maxCWLen;
  Huffman*       hcw;
} Hcb;

typedef struct
{
  int present;        /* channel present */
  int tag;            /* element tag */
  int cpe;            /* 0 if single channel, 1 if channel pair */
  int common_window;  /* 1 if common window for cpe */
  int ch_is_left;     /* 1 if left channel of cpe */
  int paired_ch;      /* index of paired channel in cpe */
  int widx;           /* window element index for this channel */
  int is_present;     /* intensity stereo is used */
  int ncch;           /* number of coupling channels for this ch */
#if (CChans > 0)
  int cch[CChans];    /* coupling channel idx */
  int cc_dom[CChans]; /* coupling channel domain */
  int cc_ind[CChans]; /* independently switched coupling channel flag */
  int cc_multi[CChans]; /*which gain coefficients if coupling channel has several different */
#endif
  char *fext;         /* filename extension */
} Ch_Info;

typedef struct tagMC_Info {
  int           nch;            /* total number of audio channels */
  int           nfsce;          /* number of front SCE's pror to first front CPE */
  int           nfch;           /* number of front channels */
  int           nsch;           /* number of side channels */
  int           nbch;           /* number of back channels */
  int           nlch;           /* number of lfe channels */
  int           ncch;           /* number of valid coupling channels */
  int           cch_tag[(1<<LEN_TAG)];  /* tags of valid CCE's */
  int		cc_ind[(1<<LEN_TAG)]; /* independently switched coupling channel tag */
  int           profile;
  int           sampling_rate_idx;
  Ch_Info       ch_info[Chans];
  unsigned char mcInfoCorrectFlag;
#ifdef I2R_LOSSLESS
  /* int osf; */
  int type_PCM;
#endif
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

typedef struct tagProgConfig {
  int tag;
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
  long    buffer_fullness;    /* put this transport level info here */
} ProgConfig;



typedef struct {
  char    adif_id[LEN_ADIF_ID+1];
  int     copy_id_present;
  char    copy_id[LEN_COPYRT_ID+1];
  int     original_copy;
  int     home;
  int     bitstream_type;
  long    bitrate;
  int     num_pce;
  int     prog_tags[(1<<LEN_TAG)];
} ADIF_Header;


typedef struct tagAACDecoder
{
  int blockSize;

  double *coef [Chans] ;
  double *data [Chans] ;
  double *state [Chans] ;

#ifdef I2R_LOSSLESS
  int     sls_quant_channel_temp[MAX_TIME_CHANNELS][1024];
#endif

  WINDOW_SEQUENCE wnd[Winds];
  Wnd_Shape wnd_shape[Winds];
  

  byte *mask [Winds] ;
  byte *cb_map [Chans];
  short *factors [Chans];
  byte *group [Winds] ;

  TNS_frame_info *tns [Chans];
  int tns_data_present[Chans];
  int tns_ext_present[Chans];

  Info *info;

#if (CChans > 0)
  double      *cc_coef[CChans];
  double      *cc_gain[CChans][Chans];
  WINDOW_SEQUENCE        cc_wnd[CChans];
  Wnd_Shape   cc_wnd_shape[CChans];
  Wnd_Shape	wnd_shape_SSR[Chans];
#endif
#if (ICChans > 0)
  double *cc_state[ICChans];
#endif

  /* data stream */

  int d_tag, d_cnt ;
  byte d_bytes[Avjframe] ;

  /* prediction */

  PRED_TYPE       pred_type;
  PRED_STATUS     *sp_status[Chans];
  
  int *lpflag[Chans];
  int *prstflag[Chans];

  MC_Info *mip ;

  /*
    SSR Profile
  */
  int short_win_in_long;

  int    max_band[Chans];
  double *spectral_line_vector_for_gc[Chans];
  double *imdctBufForGC[Chans];
  double **DBandSigBufForOverlapping[Chans];
  double **DBandSigBuf[Chans];
  GAINC  **gainInfo[Chans];

#ifdef CT_SBR
  int                  maxSfb;
#ifndef SBR_SCALABLE
  SBRBITSTREAM         ct_sbrBitStr;
#else
  SBRBITSTREAM         ct_sbrBitStr[2];
#endif
#endif

#ifdef DRC
  HANDLE_DRC drc;
#endif

} T_AACDECODER ;




#ifndef DOLBY_MDCT
#define fltmul(a,b)             ((a)*(b))
#define mkflt(f)                ((float)(f))
#endif

#endif  /* _all_h_ */
