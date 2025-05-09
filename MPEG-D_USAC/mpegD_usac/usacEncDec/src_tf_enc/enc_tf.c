/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER)
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
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- June-96  */

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 * Authors:
 * BG    Bernhard Grill, Fraunhofer Gesellshaft / University of Erlangen
 * HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover,de>
 * OS    Otto Schnurr, Motorola <schnurr@email.mot.com>
 * NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
 * TK    Takashi Koike, Sony Corporation <koike@av.crl.sony.co.jp>
 * SK    Sang-Wook Kim, Samsung, <swkim@dspsun.sait.samsung.co.kr>
 * MS    Mikko Suonio, Nokia, <Mikko.Suonio@research.nokia.com>
 * OK    Olaf K?ler, Fraunhofer IIS-A <kaehleof@iis.fhg.de>
 *
 *
 * Changes:
 * fixed bit stream handling: Heiko Purnhagen, Uni Hannover, 960705
 * 29-jul-96   OS   Modified source for 80 column width display.
 * 01-aug-96   OS   Integrated HQ source code module.
 *                  Integrated original TF mapping source code module.
 *                  Re-activated command line strings for run-time selection
 *                  of these modules.
 * 19-aug-96   BG   merge + inclusion of window switching / first short block support
 * 21-aug-96   HP   renamed to enc_tf.c, adapted to new enc.h
 * 26-aug-96   HP   updated to vm0_tf_02a
 * HP 960826   spectra_index2sf_index alloc bug fix (still repeated alloc !!!)
 * 28-aug-96   NI   Integrated NTT's VQ coder.
 *                  Provided automatic window switching.
 * 05-feb-97   TK   integrated AAC compliant pre processing module
 * 18-apr-97   NI   Integrated scalable coder.
 * 09-may-97   HP   merged contributions by NTT, Sony, FhG
 * 16-may-97   HP   included bug fix by TK (Sony)
 * 28-may-97   HP   -nttScl help
 * 17-Jul-97   NI   add include file for VQ encoder
 * 25-aug-97   NI   bug fixes
 * 29-aug-97   SK   added BSAC
 * 11-sep-97   HP   merged BSAC into current VM
 * 23-oct-97   HP   merged Nokia's predictor 971013 & 971020
 *                  tried to fix nok_PredCalcPrediction() call, PRED_TYPE
 * 26-nov-97   MS   adjusted AAC grouping, energy and allowed distortion
 *                  calculations
 * 08-apr-98   HP   removed LDFB
 * 20-oct-03   OK   clean up for rewrite encoder
 ******************************************************************************************/

#include <limits.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "encoder.h"

#include "block.h"
#include "ntt_win_sw.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "enc_tf.h"
#include "tf_main.h"
#include "psych.h"
#include "common_m4a.h"	/* common module */
#include "ms.h"

#include "ct_sbrdecoder.h"

/* son_AACpp */
#include "sony_local.h"

/* LTP */
#include "nok_ltp_enc.h"

/* aac */
#include "tns.h"
#include "tns3.h"
#include "scal_enc_frame.h"
#include "aac_tools.h"
#include "aac_qc.h"
#include "bitmux.h"

/* SAM BSAC */ /* YB : 970825 */
#include "sam_encode.h"

/* NTT VQ: */
#include "ntt_scale_conf.h" /* ntt_scale_init() */
#include "ntt_encode.h"
#include "plotmtv.h"

#ifdef I2R_LOSSLESS
#include "lit_ll_en.h"
#include "lit_ms.h"
#include "int_mdct.h"

#ifdef SONY_PVC
#include "sony_pvcenc_prepro.h"
#endif /* SONY_PVC */

#define ROUND(x) ((x>=0)?(int)(x+0.5):(int)(x-0.5))
#define SQRT2 1.41421356

double pcm_scaling[4] = {256.0, 1.0, 1.0/16, 1.0/256};    /* bug */

static WINDOW_TYPE last_block_type[MAX_TIME_CHANNELS];
static WINDOW_TYPE desired_block_type[MAX_TIME_CHANNELS];
static WINDOW_TYPE next_desired_block_type[MAX_TIME_CHANNELS];
static WINDOW_TYPE block_type_lossless_only;
static WINDOW_TYPE block_type_current;
#endif

/* 20070530 SBR */
static SBR_CHAN sbrChan[MAX_TIME_CHANNELS];
extern int samplFreqIndex[];

static const WINDOW_SHAPE windowShapeSeqToggle[] = { WS_FHG, WS_DOLBY, WS_DOLBY, WS_FHG };
static const WINDOW_SHAPE windowShapeSeqToggleLD[] = { WS_FHG, WS_ZPW, WS_ZPW, WS_FHG };
static const WINDOW_SEQUENCE windowSequenceNM[] = {
  ONLY_LONG_SEQUENCE, LONG_START_SEQUENCE, EIGHT_SHORT_SEQUENCE, LONG_STOP_SEQUENCE,
  ONLY_LONG_SEQUENCE, EIGHT_SHORT_SEQUENCE, EIGHT_SHORT_SEQUENCE, ONLY_LONG_SEQUENCE,
  LONG_STOP_SEQUENCE, LONG_START_SEQUENCE, LONG_STOP_SEQUENCE, LONG_STOP_SEQUENCE,
  EIGHT_SHORT_SEQUENCE, LONG_START_SEQUENCE, LONG_START_SEQUENCE, ONLY_LONG_SEQUENCE};
/* test all possible window combinations: 0 1 2 3  0 2 2 0  3 1 3 3  2 1 1 0 */


/* encoder modes: codec selection */
enum TF_CODEC_MODE {
  TF_AAC_PLAIN,
  TF_AAC_SCA,
  TF_TVQ,
  TF_BSAC,
  TF_AAC_LD,
#ifdef AAC_ELD
  TF_AAC_ELD,
#endif
  ___tf_codec_mode_end
};

/*** the main container structure ***/
struct tagEncoderSpecificData {
  int     debugLevel;
  enum TF_CODEC_MODE    codecMode;
  HANDLE_ENCODER        core;

  int     sampling_rate;
  int     block_size_samples; /* nr of samples per block in one audio channel */
  int     flag_960;
  int     ep_config;

  int     layer_num;
  int     track_num;
  int     tracks_per_layer[MAX_TF_LAYER];

#ifdef I2R_LOSSLESS
  int     ll_layer_num;
  int     ll_track_num;
#endif

  int     bitrates[MAX_TF_LAYER];
  int     bitrates_sum[MAX_TF_LAYER];
  int     bitrates_total;
  int     channels[MAX_TF_LAYER];
  int     channels_total;          /* no of of audio channels */
  int     bw_limit[MAX_TF_LAYER];  /* bandwidth limit of the spectrum */


  int     max_bitreservoir_bits;
  int     available_bitreservoir_bits;

  enum PP_MOD_SELECT    pp_select;
  GC_DATA_SWITCH        gc_switch;
  TNS_COMPLEXITY        tns_select;
  PRED_TYPE             pred_type;
  int     ms_select;
  int     pns_sfb_start;
  int     aacAllowScalefacs;
  enum DC_FLAG aacSimulcast;

  int     tvq_flag_ppc;
  int     tvq_flag_bandlimit;
  int     tvq_flag_postproc;
  int     tvq_flag_msmask1;
  enum NTT_PW_SELECT    tvq_pw_select; /* Perceptual weight mode for NTT_VQ */
  enum NTT_VARBIT       tvq_varbit;    /* Variable bit switch for NTT_VQ */

  int     max_sfb;
  MSInfo  msInfo;

/* SAMSUNG_2005-09-30 : added for BSAC Multichannel */
	struct channelElementData chElmData[MAX_CHANNEL_ELEMENT];
	int			ch_elm_total;
	int	lfePresent;
/* ~SAMSUNG_2005-09-30 */

/* 20070530 SBR */
   int sbrenable;
/* ---- DATA ---- */
  ntt_DATA nttData;
  TNS_INFO *tnsInfo[MAX_TIME_CHANNELS];

  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS];
  double *LTP_overlap_buffer[MAX_TIME_CHANNELS];

  double *DTimeSigBuf[MAX_TIME_CHANNELS];
  double *DTimeSigLookAheadBuf[MAX_TIME_CHANNELS];
  double *spectral_line_vector[MAX_TIME_CHANNELS];
  double *twoFrame_DTimeSigBuf[MAX_TIME_CHANNELS]; /* temporary fix to the buffer size problem. */
  double *overlap_buffer[MAX_TIME_CHANNELS];

  double *baselayer_spectral_line_vector[MAX_TIME_CHANNELS];
  double *reconstructed_spectrum[MAX_TIME_CHANNELS];
  int mat_shift[8][ntt_N_SUP_MAX];

#ifdef I2R_LOSSLESS
  /*static*/ int *rec_spectrum[MAX_TIME_CHANNELS];

  /*static*/ int int_DTimeSigBuf[2][MAX_OSF*1024];
  /*static*/ int int_DTimeSigLookAheadBuf[2][MAX_OSF*1024];
  /*static*/ int *mdctTimeBuf[MAX_TIME_CHANNELS];
  /*static*/ int *mdctFreqBuf[MAX_TIME_CHANNELS];

  /*static*/ double CoreScaling;

  /*static*/ LIT_LL_Info ll_info[MAX_TIME_CHANNELS];

  /*static*/ int *quant_aac[1024];
  AACQuantInfo quantInfo[MAX_TIME_CHANNELS][MAX_TF_LAYER];
  /*static*/ int *int_spectral_line_vector[MAX_TIME_CHANNELS];
#endif

  /* variables for the son_AACpp pre processing module */
  int     max_band[MAX_TIME_CHANNELS];
  double *spectral_line_vector_for_gc[MAX_TIME_CHANNELS];
  double *DTimeSigLookAheadBufForGC[MAX_TIME_CHANNELS];
  double *DTimeSigPQFDelayCompensationBuf[MAX_TIME_CHANNELS];
  double **DBandSigLookAheadBuf[MAX_TIME_CHANNELS];
  double **DBandSigBufForGCAnalysis[MAX_TIME_CHANNELS];
  double **DBandSigWithGCBuf[MAX_TIME_CHANNELS];
  GAINC  **gainInfo[MAX_TIME_CHANNELS];

  /* variables used by the T/F mapping */
  WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequencePrev[MAX_TIME_CHANNELS];

  enum WIN_SWITCH_MODE  win_switch_mode;
  enum WINDOW_SHAPE_ADAPT windowShapeAdapt;

  HANDLE_NTT_WINSW win_sw_module;

  int windowSequenceCounter;
  int windowShapeSeqCounter;

};


/* SAMSUNG_2005-09-30 : added for Multichannel setting */
enum {
SPEAKER_UNDEFINED						= 0x0,
SPEAKER_FRONT_LEFT_RIGHT    = 0x1,
SPEAKER_FRONT_CENTER				= 0x2,
SPEAKER_LOW_FREQUENCY       = 0x4,
SPEAKER_BACK_LEFT_RIGHT     = 0x8,
SPEAKER_BACK_CENTER         = 0x10,
SPEAKER_SIDE_LEFT_RIGHT     = 0x20,
SPEAKER_RESERVED            = 0x40
};

typedef struct {
int numElm;
int channelMask;
} channelMap;

channelMap chMapTbl[] =
{
	{0, SPEAKER_UNDEFINED},
	{1, SPEAKER_FRONT_CENTER},
	{1, SPEAKER_FRONT_LEFT_RIGHT},
	{2, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER},
	{3, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER},
	{3, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT_RIGHT},
	{4, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT_RIGHT},
	/*  7.1 channel */
	{4, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT_RIGHT|SPEAKER_SIDE_LEFT_RIGHT},
	{5, SPEAKER_FRONT_LEFT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT_RIGHT|SPEAKER_SIDE_LEFT_RIGHT}
};
/* ~SAMSUNG_2005-09-30 */

/********************************************************/
/* functions EncTfInfo() and EncTfFree()   HP 21-aug-96 */
/********************************************************/

#define PROGVER "general audio encoder core FDAM1 16-Feb-2001"

#define IS_SCAL_CODEC(x) ((x) == TF_AAC_SCA || (x) == TF_TVQ)


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */
char *EncTfInfo (FILE *helpStream)
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "encoder parameter string format:\n"
	    "possible options:\n"
	    "\t-aac     	for AAC-Main, AAC-LC and AAC-LTP object types\n"
            "\t-aac_ld  	for AAC-Low-Delay object type\n"
	    "\t-scal    	for AAC-Scalable object type\n"
	    "\t-tvq     	for TwinVQ object types\n"
	    "\t-br <Br1> [<Br2> [<Br3> [<Br4> ...]]] bitrate(s) in kbit/s\n"
	    "\t-bw <Bw1> [<Bw2> [<Bw3> [<Bw4> ...]]] bandwidths in Hz\n");
    fprintf(helpStream,
            "\t-tns, -tns_lc	for TNS or TNS with low complexity\n"
	    "\t-pns <band>	start band for Perceptual Noise Subst (AAC only)\n"
	    "\t-ltp     	enable Long Term Prediction\n"
	    "\t-ms <num>	set msMask to 0 or 2, instead of autodetect (1)\n"
	    "\t-wsm <num>	override automatic window sequence detection\n"
            "\t-wsa <n>  	window shape adaptation mode n=0: static, n=1: toggle\n"
            "\t-wshape  	use window shape WS_DOLBY instead of WS_FHG\n");
    fprintf(helpStream,
            "\t-tvq_ppc 	use TVQ pre processing\n"
            "\t-tvq_bandlimit	use TVQ bandlimit\n"
            "\t-tvq_postproc	use TVQ post processing\n");
    fprintf(helpStream,
            "\t-scale_simulcast	force simulcast for AAC-scalable\n"
            "\t-scale_diff	force difference encoding for AAC-scalable\n"
	    "\t-monolay <num>	number of mono layers in AAC-scalable\n"
	    "\t-aac_nosfacs	all AAC-scalefacs will be equal\n");
    fprintf(helpStream,
	    "\t-ep      	create bitstreams with epConfig 1 syntax\n");
    fprintf(helpStream,
	    "obscure/not working options:\n"
	    "\t-pp_aac  	gain control... (probably broken)\n"
	    "\t-length_960  	frame length flag (never tested)\n"
            "-aac_bsac !\n"
            "-aac_sys_bsac\n"
			/* 20070530 SBR */
			"\t-bsac	for BSAC object types\n"
			"\t-sbr	use sbr for BSAC object types\n"
	    "-wlp <blksize>\n");
  }

  return PROGVER;
}


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */
void EncTfFree (HANDLE_ENCODER enc)
{
/* ... */
}


/**
 * Available as part of the HANDLE_ENCODER
 */

static int EncTf_getNumChannels(HANDLE_ENCODER enc)
{ return enc->data->channels_total; }

static enum MP4Mode EncTf_getEncoderMode(HANDLE_ENCODER enc)
{ enc=enc; return MODE_TF; }
/* 20070530 SBR */
static int EncTf_getSbrEnable(HANDLE_ENCODER encoderStruct, int *bitrate)
{
  int sbrenable;

  sbrenable = encoderStruct->data->sbrenable;
  *bitrate = encoderStruct->data->bitrates_total;

  return sbrenable;
}

/**
 * Helper functions
 */

static const char* getParam(const char* haystack, const char* needle)
{
  const char* ret = strstr(haystack, needle);
  if (ret) DebugPrintf(2, "EncTfInit: accepted '%s'",needle);
  return ret;
}


/**
 * Initialise Encoder specific data structures:
 *   initialize window type and shape data
 */
static int EncTf_window_init(struct tagEncoderSpecificData *data, int wshape_flag)
{
  int i_ch;

  for (i_ch=0; i_ch<data->channels_total; i_ch++) {
    data->windowSequencePrev[i_ch] = ONLY_LONG_SEQUENCE;
    if (wshape_flag == 1) {
      data->windowShapePrev[i_ch] = WS_DOLBY;
    } else {
      data->windowShapePrev[i_ch] = WS_FHG;
    }
  }

  data->windowSequenceCounter = 0;
  data->windowShapeSeqCounter = 0;

  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   allocate and initialize TNS
 */
static int EncTf_tns_init(struct tagEncoderSpecificData *data, TNS_COMPLEXITY tns_used)
{
  int i_ch;
  for (i_ch=0; i_ch<data->channels_total; i_ch++) {
    if (tns_used == NO_TNS) {
      data->tnsInfo[i_ch] = NULL;
    } else {
      data->tnsInfo[i_ch] = (TNS_INFO*)calloc(1, sizeof(TNS_INFO));
      if ( TnsInit(data->sampling_rate, data->tns_select, data->tnsInfo[i_ch]) )
        return -1;
    }
  }
  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   allocate and initialize TwinVQ encoder
 */
static int EncTf_tvq_init(struct tagEncoderSpecificData *data,
                          float sampling_rate_f)
{
  if (data->debugLevel>1) {
    fprintf(stderr,"#### TwinVQ Global Init");
    fprintf(stderr," \n encoding with TwinVQ mode at %8d bit/s/ch \n",
            data->bitrates_total/data->channels_total);
  }
  if (data->debugLevel > 5)
    fprintf(stderr, "JJJJJJJJJJJJ ntt_IBPS ntt_NSclLay %5d %5d \n",
            data->bitrates_total, data->layer_num);

  if ((data->tvq_pw_select==NTT_PW_EXTERNAL)&&(data->channels_total > 1)) {
    CommonWarning("EncTfInit: Error in NTT_VQ: No stereo support with NTT_PW_EXTERNAL.");
    return -1;
  }

  {
    ntt_INDEX    ntt_index_scl;
    ntt_INDEX    ntt_index;

    ntt_base_init(sampling_rate_f,
                  data->bitrates_total,
                  data->bitrates_total - data->bitrates[0],
                  data->channels_total, data->block_size_samples,
                  &ntt_index);

    ntt_scale_init(data->layer_num-1, &data->bitrates[1], sampling_rate_f,
                   &ntt_index, &ntt_index_scl);

    ntt_index.nttDataBase->epFlag=data->ep_config;
    ntt_index_scl.nttDataScl->epFlag=data->ep_config;
    ntt_index_scl.nttDataScl->ntt_NSclLay = data->layer_num - 1;
    data->nttData.nttDataBase = ntt_index.nttDataBase;
    data->nttData.nttDataScl = ntt_index_scl.nttDataScl;

  }
  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   calculate max. audio bandwidth
 */
static int EncTf_bandwidth_init(struct tagEncoderSpecificData *data)
{
  if (data->bw_limit[0]<=0) {
    /* bandwidth is not set for all layers */
    int i;
    float tmp;
    int maxBandwidth = data->bw_limit[data->layer_num-1];

    if (maxBandwidth<=0) {
      /* no bandwidth is set at all, pick one from bitrate */
      /* table: audio_bandwidth( bit_rate ) */
      static const long bandwidth[8][2] =
      { {64000,20000},{56000,16000},{48000,14000},
        {40000,12000},{32000,9000},{24000,6000},
        {16000,3500},{-1,0}
      };

      i = 0;
      while( bandwidth[i][0] > data->bitrates_total ) {
        if( bandwidth[i][0] < 0 ) {
          i--;
          break;
        }
        i++;
      }
      maxBandwidth = bandwidth[i][1];
    }
    tmp = 0;
    for (i=0; i<data->layer_num; i++) {
      data->bw_limit[i] = tmp += ((float)maxBandwidth)*((float)data->bitrates[i]/(float)data->bitrates_total);
    }
  }

  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   some global initializations and memory allocations
 */
static int EncTf_data_init(
						   struct tagEncoderSpecificData *data
						   )
{
  int i_ch;
  int frameLength = data->block_size_samples;
  for (i_ch=0; i_ch<data->channels_total; i_ch++) {
    data->DTimeSigBuf[i_ch] =
            (double*)calloc(MAX_OSF*frameLength, sizeof(double));
    {
      int jj;
      for (jj=0; jj<frameLength; jj++)
        data->DTimeSigBuf[i_ch][jj] = 0.0;
    }
    data->DTimeSigLookAheadBuf[i_ch] =
            (double*)calloc(MAX_OSF*frameLength, sizeof(double));


    data->spectral_line_vector[i_ch] =
            (double*)calloc(MAX_OSF*frameLength, sizeof(double));

    data->baselayer_spectral_line_vector[i_ch] =
            (double*)calloc(frameLength, sizeof(double));

    data->reconstructed_spectrum[i_ch] =
            (double*)calloc(frameLength, sizeof(double));

    data->twoFrame_DTimeSigBuf[i_ch] =
            (double*)calloc(MAX_OSF*2*frameLength, sizeof(double));
    {
      int ii;
      for(ii=0; ii<MAX_OSF*2*frameLength; ii++)
        data->twoFrame_DTimeSigBuf[i_ch][ii] = 0.0;
    }

    /* initialize t/f mapping */
    data->overlap_buffer[i_ch] =
            (double*)calloc(frameLength*4, sizeof(double));

#ifdef I2R_LOSSLESS
    data->mdctTimeBuf[i_ch] = (int *)calloc(MAX_OSF*frameLength*2,sizeof(int));

    data->mdctFreqBuf[i_ch] = (int *)calloc(MAX_OSF*frameLength*2,sizeof(int));

    data->rec_spectrum[i_ch] = (int *)calloc(MAX_OSF*frameLength,sizeof(int));

    data->int_spectral_line_vector[i_ch] = (int*)calloc(MAX_OSF*frameLength,sizeof(int) );

    data->quant_aac[i_ch] = (int *)calloc(MAX_OSF*frameLength,sizeof(int) );

    data->ll_info[i_ch].stream = (unsigned char *) calloc(MAX_OSF*1024*4,sizeof (unsigned char));

#endif
/*  for reference

    mdctTimeBuf[chanNum] = (int *)calloc(MAX_OSF*block_size_samples*2,sizeof(int));
	mdctFreqBuf[chanNum] = (int *)calloc(MAX_OSF*block_size_samples*2,sizeof(int));

	rec_spectrum[chanNum] = (int *)calloc(MAX_OSF*block_size_samples,sizeof(int));

    lossless_time_buffer[chanNum] = (int*)calloc(MAX_OSF*block_size_samples,sizeof(int) );
	lossless_time_error[chanNum] = (int*)calloc(MAX_OSF#*block_size_samples,sizeof(int) );
    lossless_overlap_buffer[chanNum] = (double*)calloc(MAX_OSF*block_size_samples,sizeof(double) );
	p_prev_time_signal[chanNum] = (double*)calloc(MAX_OSF*block_size_samples,sizeof(double) );
	p_prev2_time_signal[chanNum] = (double*)calloc(MAX_OSF*block_size_samples,sizeof(double) );
	int_spectral_line_vector[chanNum] = (int *)calloc(MAX_OSF*block_size_samples,sizeof(int) );
	quant_aac[chanNum] = (int *)calloc(MAX_OSF*block_size_samples,sizeof(int) );

	ll_info[chanNum].stream = (unsigned char *) calloc(MAX_OSF*1024*4,sizeof (unsigned char));
	ll_info[chanNum].type_PCM = type_PCM;
  */

  }
  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   initialise AAC-PP module
 */
static int EncTf_aac_pp_init(struct tagEncoderSpecificData *data)
{
  int frameLength = data->block_size_samples;
  int band;
  int i_ch;

  for (i_ch=0; i_ch<data->channels_total; i_ch++) {
    data->max_band[i_ch] = (NBANDS * data->bw_limit[data->layer_num-1] * 2 / data->sampling_rate);
             /* max_bands[] =0,1,2,3; is more like a parameter... */

    data->spectral_line_vector_for_gc[i_ch] =
            (double *)calloc(frameLength, sizeof(double));
    data->DTimeSigPQFDelayCompensationBuf[i_ch] =
            (double *)calloc(PQFDELAY, sizeof(double));
    data->DTimeSigLookAheadBufForGC[i_ch] =
            (double *)calloc(frameLength, sizeof(double));
    data->DBandSigLookAheadBuf[i_ch] =
            (double **)calloc(NBANDS, sizeof(double *));
    data->DBandSigBufForGCAnalysis[i_ch] =
            (double **)calloc(NBANDS, sizeof(double *));
    data->DBandSigWithGCBuf[i_ch] =
            (double **)calloc(NBANDS, sizeof(double *));
    data->gainInfo[i_ch] =
            (GAINC  **)calloc(NBANDS, sizeof(GAINC *));

    for (band = 0; band < NBANDS; band++) {
      data->DBandSigLookAheadBuf[i_ch][band] =
              (double *)calloc(frameLength/NBANDS, sizeof(double));
      data->DBandSigBufForGCAnalysis[i_ch][band] =
              (double *)calloc(frameLength/NBANDS*3, sizeof(double));
      data->DBandSigWithGCBuf[i_ch][band] =
              (double *)calloc(frameLength/NBANDS*2, sizeof(double));
      data->gainInfo[i_ch][band] =
              (GAINC  *)calloc(NSHORT,sizeof(GAINC));
    }
  }
  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   initialise LTP module
 */
static int EncTf_ltp_init(struct tagEncoderSpecificData *data)
{
  int ch;

  for(ch = 0; ch < data->channels_total; ch++) {
    nok_init_lt_pred(&(data->nok_lt_status[ch]));
    data->LTP_overlap_buffer[ch] =
            (double*)calloc(data->block_size_samples, sizeof(double));
    data->nok_lt_status[ch].overlap_buffer = data->LTP_overlap_buffer[ch];
  }

  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   Parse codec options
 */
static int EncTf_parse_options(struct tagEncoderSpecificData *data, char *encPara, int *wshape_flag)
{
  const char *p_ctmp;

  /* ===================== */
  /* Parse Encoder Options */
  /* ===================== */

  if ((p_ctmp=getParam(encPara, "-d ")) ) {
    if( sscanf(p_ctmp+3, "%d",&data->debugLevel ) == 0 ) {
      CommonWarning("EncTfInit: parameter of -d switch not found" );
      return -1;
    }
  }

/* 20070530 SBR */
  if (getParam(encPara, "-sbr")) {
	data->sbrenable = 1;
  }
  else {
	data->sbrenable = 0;
  }
  /* ---- get selected encoder mode: AAC, scalable, TwinVQ,... ---- */
  if (getParam(encPara, "-scal")) {
    data->codecMode = TF_AAC_SCA;
  } else if (getParam(encPara, "-tvq")) {
    data->codecMode = TF_TVQ;
/*#ifdef BSAC*/
  } else if (getParam(encPara, "-bsac")) {
    data->codecMode = TF_BSAC;
/*#endif*/
  } else if (getParam(encPara, "-aac_ld")) {
    data->codecMode = TF_AAC_LD;
  } else if (getParam(encPara, "-aac")) {
    data->codecMode = TF_AAC_PLAIN;
  } else {
    DebugPrintf(2, "EncTfInit: defaulting to '-aac'");
    data->codecMode = TF_AAC_PLAIN;
  }

  /* ---- get mode independent command line switches ---- */
  if ( (p_ctmp=getParam(encPara, "-br ")) ) {
    if ( (data->layer_num=sscanf(p_ctmp+4, "%d %d %d %d %d %d %d %d",
		   &data->bitrates[0],
	           &data->bitrates[1],
		   &data->bitrates[2],
		   &data->bitrates[3],
		   &data->bitrates[4],
		   &data->bitrates[5],
		   &data->bitrates[6],
		   &data->bitrates[7])) == 0) {
      CommonWarning("EncTfInit: parameter of -br switch not found");
      return -1;
    }
  } else {
    CommonWarning("EncTfInit: bitrate not specified using -br");
    return -1;
  }

  if ( (p_ctmp=getParam(encPara, "-bw ")) ) {
    int tmp;
    if ( (tmp=sscanf(p_ctmp+4, "%d %d %d %d %d %d %d %d",
                   &data->bw_limit[0],
                   &data->bw_limit[1],
                   &data->bw_limit[2],
                   &data->bw_limit[3],
		   &data->bw_limit[4],
		   &data->bw_limit[5],
		   &data->bw_limit[6],
		   &data->bw_limit[7])) == 0) {
      CommonWarning("EncTfInit: parameter of -bw switch not found");
      return -1;
    }
    if (tmp != data->layer_num) {
      if (tmp == 1) {
        data->bw_limit[data->layer_num-1] = data->bw_limit[0];
        data->bw_limit[0] = -1;
      } else {
        for (; tmp<data->layer_num; tmp++) {
          data->bw_limit[tmp] = data->bw_limit[tmp-1];
        }
      }
    }
  } else {
    int tmp;
    for (tmp=0; tmp<data->layer_num; tmp++) {
      data->bw_limit[tmp] = -1;
    }
  }

  if (getParam(encPara, "-tns")) {
    data->tns_select = TNS_LC;
  }
  if (getParam(encPara, "-length_960")) {
    data->flag_960 = 1;
  }
  if (getParam(encPara, "-ltp")) {
    data->pred_type = NOK_LTP;
  }
  if( (p_ctmp=getParam(encPara, "-ms ")) ) {
    if( sscanf( p_ctmp+4, "%d",&data->ms_select ) == 0 ) {
      CommonWarning("EncTfInit: parameter of -ms switch not found" );
      return -1;
    }
  }
  if (getParam(encPara, "-ep")) {
    data->ep_config = 1;
  }
  if (wshape_flag!=NULL) {
    if (getParam(encPara, "-wshape")) {
      *wshape_flag = 1;
    }
  }


  /* ---- read AAC common command line ---- */
  if ((data->codecMode == TF_AAC_PLAIN)||
      (data->codecMode == TF_AAC_LD)||
      (data->codecMode == TF_AAC_SCA)) {

    data->aacAllowScalefacs = 1;
    data->pns_sfb_start = -1;

    if (getParam(encPara, "-aac_nosfacs")) {
      data->aacAllowScalefacs = 0;
    }

    if( (p_ctmp=getParam(encPara, "-pns ")) ) {
      if( sscanf( p_ctmp+5, "%d",&data->pns_sfb_start ) == 0 ) {
        CommonWarning("EncTfInit: parameter of -pns switch not found" );
        return -1;
      }
    }
  }

  /* ---- read more specific AAC command line ---- */
  if (data->codecMode == TF_AAC_LD) {            /* AAC-LD */
  }

  if (data->codecMode == TF_AAC_SCA) {
    if (getParam(encPara, "-scale_simulcast")) {
      data->aacSimulcast = DC_SIMUL;
    } else if (getParam(encPara, "-scale_diff")) {
      data->aacSimulcast = DC_DIFF;
    } else {
      data->aacSimulcast = DC_INVALID;
    }

    if ( (p_ctmp=getParam(encPara, "-monolay ")) ) {
      int monolay, i;
      if ( sscanf(p_ctmp+9, "%d", &monolay) == 0) {
        CommonWarning("EncTfInit: parameter of -monolay switch not found");
        return -1;
      }
      if (monolay > data->layer_num) {
        CommonWarning("EncTfInit: parameter number of -monolay exceeds layer number");
        return -1;
      }
      for (i=0; i<monolay; i++) {
        data->channels[i] = 1;
      }
      for (; i<data->layer_num; i++) {
        data->channels[i] = data->channels_total;
      }
    }
  }

  if (data->codecMode==TF_AAC_PLAIN) {
    if (data->ep_config==-1) {
      if (data->tns_select!=NO_TNS) data->tns_select = TNS_MAIN;
      if (!getParam(encPara, "-tns_lc")) {
        data->tns_select = TNS_LC;
      }
    }

    if (getParam(encPara, "-pp_aac")) {
      data->pp_select = AAC_PP;
      if (getParam(encPara, "-no_gc")) {
        data->gc_switch = GC_NON_PRESENT;
      } else {
        data->gc_switch = GC_PRESENT;
      }
    }
  }

  /* ---- read BSAC specific command line ---- */
  if (data->codecMode == TF_BSAC) {
		data->pns_sfb_start = -1; /* SAMSUNG_2005-09-30 : disable PNS */

    /*if( (p_ctmp=getParam(encPara, "-pns ")) ) {
      if( sscanf( p_ctmp+5, "%d",&data->pns_sfb_start ) == 0 ) {
        CommonWarning("EncTfInit: parameter of -pns switch not found" );
        return -1;
      }
    }*/
  }

  if (data->codecMode == TF_TVQ) {
    data->tvq_flag_ppc = 0;
    data->tvq_flag_bandlimit = 0;
    data->tvq_flag_postproc = 0;
    data->tvq_flag_msmask1 = 0;

    data->tvq_pw_select = NTT_PW_INTERNAL;
    data->tvq_varbit = NTT_VARBIT_OFF;

    if (getParam(encPara, "-tvq_ppc")) data->tvq_flag_ppc =1;
    if (getParam(encPara, "-tvq_bandlimit")) data->tvq_flag_bandlimit =1;
    if (getParam(encPara, "-tvq_postproc")) data->tvq_flag_postproc =1;
    if (getParam(encPara, "-tvq_msmask1")) data->tvq_flag_msmask1 =1;

  }

#ifdef I2R_LOSSLESS
  data->ll_layer_num = 1;
#endif

  return 0;
}


/*****************************************************************************************
 ***
 *** Function: EncTfInit
 ***
 *** Purpose:  Initialize the T/F-part and the macro blocks of the T/F part of the VM
 ***
 *** Description:
 ***
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/

int EncTfInit (
                int                   numChannel,                 /* in: num audio channels */
                float                 sampling_rate_f,            /* in: sampling frequancy [Hz] */
                char                 *encPara,                    /* in: encoder parameter string */
                int                  *frameNumSample,             /* out: num samples per frame */
                int                  *frameMaxNumBit,             /* out: maximum num bits per frame */ /* SAMSUNG_2005-09-30 : added */
                DEC_CONF_DESCRIPTOR  *dec_conf,                   /* in: space to write decoder configurations */
                int                  *numTrack,                   /* out: number of tracks */
                HANDLE_BSBITBUFFER   *asc,                        /* buffers to hold output Audio Specific Config */
                int                  *numAPRframes,               /* number of AudioPre-Roll frames in an IPF */
#ifdef I2R_LOSSLESS
                int                   type_PCM,
                int                   _osf,
#endif
                HANDLE_ENCODER        core,                       /* in:  core encoder or NULL */
                HANDLE_ENCODER        encoderStruct               /* in: space to put encoder data */
) {
  int wshape_flag = 0;
  int i, i_dec_conf;
  int lfePresent = 0; /* SAMSUNG_2005-09-30 */

  HANDLE_BSBITBUFFER* asc_tmp = asc;
  int numAPRframes_tmp        = *numAPRframes;

  struct tagEncoderSpecificData *data = (struct tagEncoderSpecificData*)malloc(sizeof(struct tagEncoderSpecificData));

  if (data == NULL) {
    CommonWarning("EncTfInit: Allocation Error");
    return -1;
  }
  encoderStruct->data = data;
  encoderStruct->getNumChannels = EncTf_getNumChannels;
  encoderStruct->getEncoderMode = EncTf_getEncoderMode;
  encoderStruct->getSbrEnable = EncTf_getSbrEnable;

  data->debugLevel=0;
  data->channels_total = numChannel;
  for (i=0; i<MAX_TF_LAYER; i++) data->channels[i]=data->channels_total;
  data->sampling_rate = (int)(sampling_rate_f+.5);
  data->pp_select = NONE;
  data->gc_switch = GC_NON_PRESENT;
  data->pred_type = PRED_NONE;
  data->tns_select = NO_TNS;
  data->ms_select = 1;
  data->flag_960 = 0;
  data->ep_config = -1;
  data->track_num = -1;

#ifdef I2R_LOSSLESS
  data->ll_track_num = -1;
  {
    int ch, lay;
    for (ch = 0; ch < MAX_TIME_CHANNELS; ch++)
      for (lay = 0; lay < MAX_TF_LAYER; lay++)
        data->quantInfo[ch][lay].ll_present = 1;
  }

  encoderStruct->osf = _osf;
#endif

  data->core = core;
  data->win_switch_mode = STATIC_LONG ;
  data->windowShapeAdapt = STATIC;

  data->max_sfb = 0;
  {
    int i,k;
    data->msInfo.ms_mask = 0;
    for (i=0; i<MAX_SHORT_WINDOWS; i++) {
      for (k=0; k<SFB_NUM_MAX; k++) {
        data->msInfo.ms_used[i][k] = 0;
      }
    }
  }

  /* ---- parse encoder options ---- */
  if (EncTf_parse_options(data, encPara, &wshape_flag) != 0) {
    CommonWarning("EncTfInit: error parsing options");
    return -1;
  }

/* 20070530 SBR */
  if(data->sbrenable)
	data->sampling_rate = data->sampling_rate/2;
  /* ======================= */
  /* Process Encoder Options */
  /* ======================= */
  if ((data->codecMode == TF_AAC_PLAIN)||
      (data->codecMode == TF_AAC_SCA)) {         /* AAC */
    if (data->flag_960) {
      data->block_size_samples = 960;
    } else {
      data->block_size_samples = 1024;
    }
  }
/* SAMSUNG_2005-09-30 : channel element configuration */
	if(data->codecMode == TF_BSAC)
	{
		data->ch_elm_total = chMapTbl[numChannel].numElm;

		for(i=0; i<data->ch_elm_total; i++)
		{
			struct channelElementData *chData  = &data->chElmData[i];
			chData->tns_select = data->tns_select;
			chData->ms_select = data->ms_select;
			chData->win_switch_mode = data->win_switch_mode;
			chData->windowShapeAdapt = data->windowShapeAdapt;
			chData->max_sfb = data->max_sfb;
			{
				int j,k;
				chData->msInfo.ms_mask = data->msInfo.ms_mask;
				for (j=0; j<MAX_SHORT_WINDOWS; j++) {
					for (k=0; k<SFB_NUM_MAX; k++) {
						chData->msInfo.ms_used[j][k] = 0;
					}
				}
			}
		}
		{
			int chMask = chMapTbl[numChannel].channelMask;
			int tempMask = 1;
			int ext = 0;
			int start = 0;

			for(i=0; i<data->ch_elm_total; )
			{
				switch(tempMask & chMask)
				{
					case SPEAKER_FRONT_LEFT_RIGHT :
						data->chElmData[i].chNum = 2;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start+1;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = LR_FRONT;
						start += 2; i++; ext = 1;
						break;
					case SPEAKER_FRONT_CENTER :
						data->chElmData[i].chNum = 1;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = CENTER_FRONT;
						start += 1; i++; ext = 1;
						break;
					case SPEAKER_LOW_FREQUENCY :
						data->chElmData[i].chNum = 1;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = LFE;
						start += 1; i++; ext = 1;
						lfePresent = 1;
						break;
					case SPEAKER_BACK_LEFT_RIGHT :
						data->chElmData[i].chNum = 2;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start+1;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = LR_SUR;
						start += 2; i++; ext = 1;
						break;
					case SPEAKER_BACK_CENTER :
						data->chElmData[i].chNum = 1;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = REAR_SUR;
						start += 1; i++; ext = 1;
						break;
					case SPEAKER_SIDE_LEFT_RIGHT :
						data->chElmData[i].chNum = 2;
						data->chElmData[i].startChIdx = start;
						data->chElmData[i].endChIdx = start+1;
						data->chElmData[i].extension = ext;
						data->chElmData[i].ch_type = LR_OUTSIDE;
						start += 2; i++; ext = 1;
						break;
					default :
						break;
				}
				tempMask <<= 1;
			}
		}
	}
/* ~SAMSUNG_2005-09-30 */

  if (data->codecMode == TF_AAC_LD) {            /* AAC-LD */
    if (data->ep_config==-1) data->ep_config = 1;

    if (data->flag_960) {
      data->block_size_samples = 480;
      if (data->tns_select!=NO_TNS) data->tns_select = TNS_LD_480;
    } else {
      data->block_size_samples = 512;
      if (data->tns_select!=NO_TNS) data->tns_select = TNS_LD_512;
    }
  }

/* SAMSUNG_2005-09-30 */
	if (data->codecMode == TF_BSAC) {							/* BSAC */
		int tracks = 0;
    int t=1;

    data->ep_config = 0; /* SAMSUNG_2005-09-30 : only support epConfig 0 */

    /*if (data->ep_config == -1) t=1;*/
		if (data->ep_config == 0) t=1;
    else if (data->ep_config == 1) t=2;
    else { CommonWarning("only ep1 or non error resilient syntax are supported for BSAC"); return -1; }

    for (i=0; i<data->layer_num; i++) {
      data->tracks_per_layer[i] = t;
      tracks += t;
    }
    data->track_num=tracks;

		if (data->flag_960) {
      data->block_size_samples = 960;
    } else {
      data->block_size_samples = 1024;
    }
		data->lfePresent = lfePresent;
		if(lfePresent)
			data->win_switch_mode = STATIC_LONG;
	}
/* ~SAMSUNG_2005-09-30 */


  /* ---- decide number of tracks for AAC ---- */
  if ((data->codecMode == TF_AAC_PLAIN)||
      (data->codecMode == TF_AAC_LD)||
      (data->codecMode == TF_AAC_SCA)) {
    int tracks = 0;
    for (i=0; i<data->layer_num; i++) {
      int t=0;
      if (data->ep_config==-1) {
        t=1;
      } else if (data->ep_config==1) {
        if (data->channels[i] == 1) {
          t = 3;
        } else if (data->channels[i] == 2) {
          t = 7;
        } else {
          CommonWarning("for ER-AAC only mono or stereo configurations are supported");
          return -1;
        }
      } else {
        CommonWarning("only ep1 or non error resilient syntax are supported for AAC");
        return -1;
      }

      data->tracks_per_layer[i] = t;
      tracks += t;
    }

    data->track_num = tracks;
  }

#ifdef I2R_LOSSLESS
  data->ll_track_num = data->ll_layer_num;
#endif


  /* ---- decide number of tracks for TwinVQ ---- */
  if (data->codecMode == TF_TVQ) {
    int tracks = 0;
    int t;
    if (data->ep_config == -1) t=1;
    else if (data->ep_config == 1) t=2;
    else { CommonWarning("only ep1 or non error resilient syntax are supported for TwinVQ"); return -1; }

    for (i=0; i<data->layer_num; i++) {
      data->tracks_per_layer[i] = t;
      tracks += t;
    }
    data->track_num=tracks;


    if (data->flag_960) {
      data->block_size_samples = 960;
    } else {
      data->block_size_samples = 1024;
    }
  }

  /* -------------------------------- */

  {
    /* ---- override defaults ---- */
    const char *p_ctmp;

    if (data->codecMode == TF_AAC_LD) {            /* AAC-LD */
      data->win_switch_mode = STATIC_LONG;
    } else
    {
      /* Window Sequence switching parameters */
      if ( (p_ctmp=getParam(encPara, "-wsm ")) ) {
        if( sscanf( p_ctmp+5, "%d",(int*)&data->win_switch_mode ) == 0 ) {
          CommonWarning("EncTfInit: parameter of -wsm switch not found");
          return -1;
        }
      }
    }

    /* Window Shape Adaptation Mode */
    if( (p_ctmp=getParam(encPara, "-wsa ")) ) {
      if( sscanf( p_ctmp+5, "%u",(unsigned int*)(&data->windowShapeAdapt) ) == 0 ) {
        CommonWarning("Encode: parameter of -wsa not found");
        return -1;
      }
    }

    /* window length parameters... */
    if ( (p_ctmp=getParam(encPara, "-wlp ")) ) {
      if( sscanf( p_ctmp+5, "%i", &data->block_size_samples) != 1 ) {
        CommonWarning("EncTfInit: parameter of -wlp switch not found");
        return -1;
      }
    }
  }

  /* -------------------------------- */

  /* ---- process data where necessary ---- */
  data->bitrates_sum[0] = data->bitrates[0];
  for (i=1; i<data->layer_num; i++) {
    data->bitrates_sum[i] = data->bitrates_sum[i-1]+data->bitrates[i];
  }
  data->bitrates_total = data->bitrates_sum[i-1];

  for (i=1; i<data->layer_num; i++) {
    if (data->channels[i] < data->channels[i-1]) {
      CommonWarning("EncTfInit: Channel number is decreasing");
      return -1;
    }
  }
  if (data->channels[data->layer_num-1] > data->channels_total) {
    CommonWarning("EncTfInit: Channel number exceeds available chanels");
    return -1;
  }
  if (data->channels[0] < 1) {
    CommonWarning("EncTfInit: Channel number smaller than 1");
    return -1;
  }

  /* ================================ */
  /* Initialize Data Structures       */
  /* ================================ */

  if (EncTf_window_init(data, wshape_flag) != 0) return -1;
  if (EncTf_bandwidth_init(data) != 0) return -1;

  /* ---- initialize TNS ---- */
  if (EncTf_tns_init(data, data->tns_select) != 0) return -1;

  /* initialize psychoacoustic module (does nothing at the moment) */
  EncTf_psycho_acoustic_init();

  switch (data->codecMode) {
  case TF_AAC_PLAIN:
  case TF_AAC_SCA:
  case TF_AAC_LD:
    tf_init_encode_spectrum_aac( data->debugLevel );
    break;
/* #ifdef BSAC */
  case TF_BSAC:
    tf_init_encode_spectrum_aac( data->debugLevel );
/* SAMSUNG_2005-09-30 */

		for (i_dec_conf=0; i_dec_conf<data->track_num; i_dec_conf++)
			dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_BSAC;

		sam_init_encode_bsac(numChannel, data->sampling_rate, data->bitrates_total, data->block_size_samples);
/* ~SAMSUNG_2005-09-30 */


    break;
/* #endif */

  case TF_TVQ:
    if (EncTf_tvq_init(data, sampling_rate_f) != 0) return -1;
    break;

  default:
    CommonWarning("EncTfInit: case not handled in switch");
    return -1;
    break;
  }

  if (EncTf_data_init(data) != 0) return -1;

  /* initialize son_AACpp */
  if (data->pp_select == AAC_PP) {
    if (EncTf_aac_pp_init(data) != 0) return -1;
  }

  /* initialize Nokia's predictors */
  if (data->pred_type == NOK_LTP) {
    if (EncTf_ltp_init(data) != 0) return -1;
  }

  /* initialize window detection */
  if (data->win_switch_mode == FFT_PE_WINDOW_SWITCHING) {
    data->win_sw_module = ntt_win_sw_init(data->channels_total, data->block_size_samples);
  }

  /* ================================ */
  /* Write Header Information         */
  /* ================================ */

  /* ----- common part ----- */

  for (i_dec_conf=0; i_dec_conf<data->track_num; i_dec_conf++) {
    int sx;

    dec_conf[i_dec_conf].avgBitrate.value = 0;
    dec_conf[i_dec_conf].maxBitrate.value = 0;
    dec_conf[i_dec_conf].audioSpecificConfig.epConfig.value = 0;

    for (sx=0;sx<0xf;sx++) {
      if (data->sampling_rate == samplFreqIndex[sx]) break;
    }
    dec_conf[i_dec_conf].audioSpecificConfig.samplingFreqencyIndex.value = sx;
    dec_conf[i_dec_conf].audioSpecificConfig.samplingFrequency.value = data->sampling_rate;

    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = data->flag_960; /* 1: 960 0:1024*/
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value = 0;
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.extension.value = (data->ep_config!=-1);
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.progConfig = NULL;
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.aacSectionDataResilienceFlag.value = 0;
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.aacScalefactorDataResilienceFlag.value = 0;
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.aacSpectralDataResilienceFlag.value = 0;
    dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.extension3.value = 0;

    dec_conf[i_dec_conf].audioSpecificConfig.epConfig.value = data->ep_config;
  }

#ifdef I2R_LOSSLESS
#endif

  /* ----- AAC specific part ----- */

  if ((data->codecMode == TF_AAC_PLAIN)||
      (data->codecMode == TF_AAC_LD)) {
    if (data->layer_num!=1) {
      CommonWarning("EncTfInit: Scalability not supported in plain AAC");
      return -1;
    }
    for (i_dec_conf = 0; i_dec_conf < data->tracks_per_layer[0]; i_dec_conf++) {
      if (data->ep_config==-1)
        dec_conf[i_dec_conf].avgBitrate.value = data->bitrates[0];
      else
        dec_conf[i_dec_conf].avgBitrate.value = 0;
      dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels[0];

      dec_conf[i_dec_conf].bufferSizeDB.value = 6144*data->channels_total /8;
      if (data->codecMode == TF_AAC_PLAIN) {
        if (data->pred_type==NOK_LTP) {
          if ((data->tns_select!=NO_TNS)&&
              (data->tns_select!=TNS_LC)) {
            CommonWarning("EncTfInit: selected TNS complexity not supported with AAC-LTP");
            return -1;
          }
          if (data->ep_config == -1)
            dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = AAC_LTP;
          else
            dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_AAC_LTP;
        } else if (data->tns_select==TNS_MAIN) {
          if (data->ep_config != -1) {
            CommonWarning("EncTfInit: selected TNS complexity not supported with ER-AAC");
            return -1;
          }
          dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = AAC_MAIN;
        } else if ((data->tns_select==TNS_LC)||
                   (data->tns_select==NO_TNS)) {
          if (data->ep_config == -1) {
            dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = AAC_LC;
          } else {
            dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_AAC_LC;
          }
        } else {
          CommonWarning("EncTfInit: selected TNS complexity not supported with plain AAC");
          return -1;
        }
      } else {
        dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_AAC_LD;
      }
    }
  }

#ifdef AAC_ELD
  if (data->codecMode == TF_AAC_ELD) {
    if (data->layer_num!=1) {
      CommonWarning("EncTfInit: Scalability not supported in AAC-ELD");
      return -1;
    }
    for (i_dec_conf = 0; i_dec_conf < data->tracks_per_layer[0]; i_dec_conf++) {
      if (data->ep_config==-1)
        dec_conf[i_dec_conf].avgBitrate.value = data->bitrates[0];
      else
        dec_conf[i_dec_conf].avgBitrate.value = 0;
      dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels[0];

      dec_conf[i_dec_conf].bufferSizeDB.value = 6144*data->channels_total /8;
      dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_AAC_ELD;
    }
  }
#endif

  /* ----- AAC-Scalable specific part ----- */

  if (data->codecMode == TF_AAC_SCA){
    i_dec_conf = 0;
    for (i=0; i<data->layer_num; i++) {
      int j;
      for (j=0; j<data->tracks_per_layer[i]; j++) {
        if (data->ep_config == -1) {
          dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = AAC_SCAL;
          dec_conf[i_dec_conf].avgBitrate.value = data->bitrates[i];
        } else {
          dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = ER_AAC_SCAL;
          dec_conf[i_dec_conf].avgBitrate.value = 0;
        }

        dec_conf[i_dec_conf].bufferSizeDB.value = 6144*data->channels[i] /8;
        dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels[i];

        dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value = i;
        dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value = ((i==0)&&(core!=NULL));

        i_dec_conf++;
      }
    }

  }

  /* ----- TwinVQ specific parts ----- */

  if (data->codecMode == TF_TVQ){
    i_dec_conf = 0;
    for (i=0; i<data->layer_num; i++) {
      int j;
      for (j=0; j<data->tracks_per_layer[i]; j++) {
        dec_conf[i_dec_conf].bufferSizeDB.value = ((data->bitrates[i]*data->block_size_samples)/data->sampling_rate +7)/8;

        dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = (data->ep_config==-1)?TWIN_VQ:ER_TWIN_VQ;
        dec_conf[i_dec_conf].avgBitrate.value = data->bitrates[i];
        dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels[i];
        i_dec_conf++;
      }
    }
  }


#ifdef I2R_LOSSLESS
#endif

  /* SAMSUNG_2005-09-30 */
  /* ----- BSAC specific parts ----- */
  if (data->codecMode == TF_BSAC) {
    for (i_dec_conf = 0; i_dec_conf < data->track_num; i_dec_conf++) {
      if (data->ep_config==-1)
        dec_conf[i_dec_conf].avgBitrate.value = data->bitrates[0];
      else
        dec_conf[i_dec_conf].avgBitrate.value = 0;

      if(data->channels[0] > 2)
        dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = 2; /* SAMSUNG_2005-09-30 : for BC to stereo */
      else
        dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels[0];

      dec_conf[i_dec_conf].bufferSizeDB.value = 6144*data->channels_total /8;

      dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.numOfSubFrame.length = 5;
      dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.numOfSubFrame.value = 1;

      dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.layer_length.length = 11;

      dec_conf[i_dec_conf].audioSpecificConfig.specConf.TFSpecificConfig.layer_length.value = 0;	/*needs to be fixed.*/
    }
  }


  if(lfePresent) {
    data->max_bitreservoir_bits = 6144*(data->channels_total-1);
  }
  else {  /* ~SAMSUNG_2005-09-30 */
    data->max_bitreservoir_bits = 6144*data->channels_total;
  }
  data->available_bitreservoir_bits = data->max_bitreservoir_bits;

  {
    int average_bits_total = (data->bitrates_total*data->block_size_samples) / data->sampling_rate;
    data->available_bitreservoir_bits -= average_bits_total;
  }
	*frameMaxNumBit = data->max_bitreservoir_bits; /* SAMSUNG_2005-09-30 */

  /* initialize spectrum processing */



  if (data->track_num==-1) {
    CommonWarning("EncTfInit: setup of track_count failed");
    return -1;
  }

  *frameNumSample = data->block_size_samples;
  *numTrack       = data->track_num;

#ifdef I2R_LOSSLESS
#endif

/* 20070530 SBR */
  if (data->sbrenable) {
    if (data->sampling_rate != 22050 && data->sampling_rate != 24000) {
    }

    /* Init SBR */
    for(i=0; i<data->ch_elm_total; i++)
    {
      struct channelElementData *chData  = &data->chElmData[i];
      SbrInit(data->bitrates_total, 2*data->sampling_rate, chData->chNum, SBR_RATIO_INDEX_2_1, &sbrChan[chData->startChIdx]);
      chData->sbrChan[chData->startChIdx] = &sbrChan[chData->startChIdx];
      if(chData->chNum==2) {
        SbrInit(data->bitrates_total, 2*data->sampling_rate, chData->chNum, SBR_RATIO_INDEX_2_1, &sbrChan[chData->endChIdx]);
        chData->sbrChan[chData->endChIdx] = &sbrChan[chData->endChIdx];
      }
    }
  }
  else {
	for(i=0; i<data->ch_elm_total; i++)
	{
		struct channelElementData *chData  = &data->chElmData[i];
		chData->sbrChan[0] = chData->sbrChan[1] = NULL;
	}
  }
/* 20070530 SBR */
  return 0;
}


/*****************************************************************************************
 ***
 *** Function:    EncTfFrame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on T/F encoding
 ***
 *** Description:
 ***
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:  returns the number of used bits
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/

/*OK: must have time input data for -aac_pp or LTP */

int EncTfFrame (
                ENCODER_DATA_TYPE     input,
                HANDLE_BSBITBUFFER   *au,                         /* buffers to hold output AccessUnits */
                const int             requestIPF,                 /* indicate that an IPF is requested within the next frames */
                USAC_SYNCFRAME_TYPE     *syncFrameType,           /* indicates the USAC sync frame state */
#ifdef I2R_LOSSLESS
#endif
                HANDLE_ENCODER        enc,
/* 20070530 SBR */
                float               **p_time_signal_orig
) {
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS];

  const double *p_ratio[MAX_TIME_CHANNELS];
  double allowed_distortion[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  double p_energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    nr_of_sfb[MAX_TIME_CHANNELS];
  int    sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];

  int i_ch, i, k;

  ENCODER_DATA_TYPE core_timesig = NULL;
  int core_channels = 0;
  int core_max_sfb = 0;
  int isFirstTfLayer = 1;
  int core_transmitted_tns[MAX_TIME_CHANNELS] = {0,0};
  int osf = 1;

  /*-----------------------------------------------------------------------*/
  /* VQ: Variables for NTT_VQ coder */
  ntt_INDEX  ntt_index, ntt_index_scl;
  ntt_PARAM  param_ntt;
  /*-----------------------------------------------------------------------*/

  /* structures holding the output of the psychoacoustic model */
  CH_PSYCH_OUTPUT chpo_long[MAX_TIME_CHANNELS];
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS];

  /* AAC window grouping information */
  int num_window_groups = 0;
  int window_group_length[8];

  HANDLE_BSBITSTREAM* output_au;
#ifdef I2R_LOSSLESS
  HANDLE_BSBITSTREAM* ll_output_au;
  /* int osf = 1; */ 
  int msStereoIntMDCTMode = 2; 
  int msStereoIntMDCT = 0; 
  int coreenable = 1; 
#endif

  HANDLE_AACBITMUX aacmux[MAX_TF_LAYER];
#ifdef I2R_LOSSLESS
  HANDLE_AACBITMUX slsmux[MAX_TF_LAYER];
#endif

  struct tagEncoderSpecificData *data = enc->data;

  int lfe_chIdx = -1; /* SAMSUNG_2005-09-30 : LFE channel index */
  int frameNumBit = 0; /* SAMSUNG_2005-09-30 : number of bits per frame */

/* 20060107 */
  int sbr_bits = 0;

  int requestIPF_tmp = requestIPF;
  int usacSyncState = (int)(*syncFrameType);

  /*-----------------------------------------------------------------------*/
  /* Initialization  */
  /*-----------------------------------------------------------------------*/


  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame entered\n");

  output_au = (HANDLE_BSBITSTREAM*)malloc(data->track_num*sizeof(HANDLE_BSBITSTREAM));
  for (i=0; i<data->track_num; i++) {
    output_au[i] = BsOpenBufferWrite(au[i]);
    if (output_au[i]==NULL) {
      CommonWarning("EncTfFrame: error opening output space");
      return -1;
    }
  }

#ifdef I2R_LOSSLESS
  osf = enc->osf;
#endif


  for (i=0; i<data->layer_num; i++) aacmux[i] = NULL;
#ifdef I2R_LOSSLESS
  for (i=0; i<data->layer_num; i++) slsmux[i] = NULL;
#endif
  switch (data->codecMode) {
  case TF_BSAC:
    break;

  case TF_AAC_LD:
  case TF_AAC_PLAIN:
  case TF_AAC_SCA:
    {
      int t=0;
      for (i=0; i<data->layer_num; i++) {
        aacmux[i] = aacBitMux_create(&(output_au[t]),
                                     data->tracks_per_layer[i],
                                     data->channels[i],
                                     data->ep_config,
                                     data->debugLevel);
        t+=data->tracks_per_layer[i];
      }
#ifdef I2R_LOSSLESS
      t=0;
	  for (i=0; i<data->ll_layer_num; i++) {
        slsmux[i] = aacBitMux_create(&(ll_output_au[t]),
                                     data->tracks_per_layer[i],
                                     data->channels[i],
                                     data->ep_config,
                                     data->debugLevel);
        t+=1;
      }
#endif
    }
    break;

  case TF_TVQ:
    ntt_index.nttDataScl =  data->nttData.nttDataScl;
    ntt_index.nttDataBase =  data->nttData.nttDataBase;
    ntt_index_scl.nttDataScl =  ntt_index.nttDataScl;

    ntt_index.block_size_samples = data->block_size_samples;
    ntt_index.numChannel = data->channels_total;
    ntt_index.isampf = data->sampling_rate/1000;
    ntt_index_scl.block_size_samples = ntt_index.block_size_samples;
    ntt_index_scl.numChannel = ntt_index.numChannel;
    ntt_index_scl.isampf = ntt_index.isampf;

    ntt_index.tvq_tns_enable = 0;
    if (data->tns_select!=NO_TNS) ntt_index.tvq_tns_enable = 1;
    ntt_index.tvq_ppc_enable = data->tvq_flag_ppc;
    ntt_index.bandlimit = data->tvq_flag_bandlimit;
    ntt_index.pf = data->tvq_flag_postproc;
    ntt_index.ms_mask = data->tvq_flag_msmask1;
    ntt_index.er_tvq = (data->ep_config!=-1);

    ntt_index_scl.tvq_tns_enable = ntt_index.tvq_tns_enable;
    ntt_index_scl.tvq_ppc_enable = ntt_index.tvq_ppc_enable;
    ntt_index_scl.bandlimit = ntt_index.bandlimit;
    ntt_index_scl.pf = ntt_index.pf;
    ntt_index_scl.ms_mask = ntt_index.ms_mask;
    ntt_index_scl.er_tvq = ntt_index.er_tvq;
    break;

  default:
    CommonWarning("oops... mode not supported in enc_tf.c");
    return -1;
  }

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: init done\n");

/* 20070530 SBR */
  /***********************************************************************/
  /* SBR encoding */
  /***********************************************************************/
  if (p_time_signal_orig != NULL) {
    for (i_ch=0;i_ch<data->ch_elm_total;i_ch++) {
      if (data->chElmData[i_ch].chNum==1) {
        /* single_channel_element */
        if (1/*!channelInfo[chanNum].cpe*/) {
          if (1/*!channelInfo[chanNum].lfe*/) {
            sbr_bits = SbrEncodeSingleChannel_BSAC(data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].startChIdx],
                                                   SBR_TF,
                                              p_time_signal_orig[data->chElmData[i_ch].startChIdx]
#ifdef	PARAMETRICSTEREO
											  ,NULL  /* PS disable */
#endif
#ifdef SONY_PVC_ENC
								              ,0
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
												,0
#endif
#endif
                                              );
          }
        }
	  } else {
          if (data->chElmData[i_ch].chNum==2) {
            /* Write out cpe */
            sbr_bits =
				SbrEncodeChannelPair_BSAC(data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].startChIdx],     /* Left */
                                                          data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].endChIdx], /* Right */
                                                          SBR_TF,
                                                          p_time_signal_orig[data->chElmData[i_ch].startChIdx],                       /* Left */
                                                          p_time_signal_orig[data->chElmData[i_ch].endChIdx],  /* Right */
                                                          0
                                                          );
          }
        /*}   if (!channelInfo[chanNum].cpe)  else */
      } /* if (chann...*/
    } /* for (chanNum...*/
  }
  /*-----------------------------------------------------------------------*/


  /****
   * There is a core encoder:
   *   Take as much information from it as possible
   */
  if (data->core != NULL) {
    if (data->core->getNumChannels != NULL) {
      core_channels = data->core->getNumChannels(data->core);
    }
    if (core_channels != 0) {
      enum MP4Mode core_mode = MODE_UNDEF;
      DebugPrintf(2,"core with %i channels, ext with %i channels\n",core_channels, data->channels_total);
      if (data->core->getEncoderMode != NULL) {
        core_mode = data->core->getEncoderMode(data->core);
      }
      if (core_mode == MODE_TF) {
        struct tagEncoderSpecificData *core_data = data->core->data;
        /* copy spectrum */
        double **core_spectrum = core_data->reconstructed_spectrum;
        for (i_ch=0; i_ch<core_channels; i_ch++) {
          /* FIXME: will break if resampling is needed */
          for (i=0; i<data->block_size_samples; i++) {
            data->baselayer_spectral_line_vector[i_ch][i] = core_spectrum[i_ch][i];
          }
        }
        for (; i_ch<data->channels_total; i_ch++) {
          for (i=0; i<data->block_size_samples; i++) {
            data->baselayer_spectral_line_vector[i_ch][i] = 0;
          }
        }
        core_max_sfb = core_data->max_sfb;
        /* copy M/S mask */
        if (core_channels==2) {
          for (i=0; i<MAX_SHORT_WINDOWS; i++) {
            for (k=0; k<core_max_sfb; k++) {
              data->msInfo.ms_used[i][k] = core_data->msInfo.ms_used[i][k];
            }
          }
        }
        /* get TNS information */
        for (i_ch=0; i_ch<core_channels; i_ch++) {
          core_transmitted_tns[i_ch] = 0;
          if (core_data->tnsInfo[i_ch]) {
            if (core_data->tnsInfo[i_ch]->tnsDataPresent)
              core_transmitted_tns[i_ch] = 1;
          }
        }
        /* copy window information */
        for (i_ch=0; i_ch<core_channels; i_ch++) {
          windowSequence[i_ch] = core_data->windowSequencePrev[i_ch];
          windowShape[i_ch] = core_data->windowShapePrev[i_ch];
        }
        for (; i_ch<data->channels_total; i_ch++) {
          windowSequence[i_ch] = windowSequence[MONO_CHAN];
          windowShape[i_ch] = windowShape[MONO_CHAN];
        }
        isFirstTfLayer = 0;
      } else if (data->core->getReconstructedTimeSignal != NULL) {
        core_timesig = data->core->getReconstructedTimeSignal(data->core);
        
      }
    }
  }

  /****
   * Transform time domain to frequency domain
   */
  {
    /* convert float input to double, which is the internal format */
    /* store input data in look ahead buffer which may be necessary for the window switching decision and AAC pre processing */

    if (data->debugLevel>1)
      fprintf(stderr,"EncTfFrame: copy time buffers\n");

    for (i_ch=0; i_ch<data->channels_total; i_ch++){
      for( i=0; i<osf*data->block_size_samples; i++ ) {
	/* temporary fix: a linear buffer for LTP containing the whole time frame */
	data->twoFrame_DTimeSigBuf[i_ch][i] = data->DTimeSigBuf[i_ch][i];
	data->twoFrame_DTimeSigBuf[i_ch][data->block_size_samples + i] = data->DTimeSigLookAheadBuf[i_ch][i];
	/* last frame input data are encoded now */
	data->DTimeSigBuf[i_ch][i]          = data->DTimeSigLookAheadBuf[i_ch][i];
	/* new data are stored here for use in window switching decision
	   and AAC pre processing */
	if (data->pp_select == AAC_PP) {
	  data->DTimeSigLookAheadBuf[i_ch][i] = (i < PQFDELAY) ? data->DTimeSigPQFDelayCompensationBuf[i_ch][i] : (double)input[i_ch][i-PQFDELAY] ;
	  data->DTimeSigLookAheadBufForGC[i_ch][i] = (double)input[i_ch][i];
	}
	else { /* pp_select != AAC_PP */
	  data->DTimeSigLookAheadBuf[i_ch][i] = (double)input[i_ch][i];
	}
      }
    }
    if (data->pp_select == AAC_PP) {
      for (i_ch=0; i_ch<data->channels_total; i_ch++){
        for (i = 0; i < PQFDELAY; i++) {
          data->DTimeSigPQFDelayCompensationBuf[i_ch][i] = (double)input[MONO_CHAN][data->block_size_samples-PQFDELAY+i];
        }
      } /* for i_ch...*/
    }/* if pp_select...*/


  /*-----------------------------------------------------------------------*/
  /* VQ: Frame initialization for NTT_VQ coder*/

  param_ntt.ntt_param_set_flag = 0;
  /*-----------------------------------------------------------------------*/





  /******************************************************************************************************************************
  *
  * pre processing
  *
  ******************************************************************************************************************************/

    if (isFirstTfLayer) {
      if (data->debugLevel>1)
        fprintf(stderr,"EncTfFrame: select window sequence and shape\n");

      switch( data->win_switch_mode ) {
      case STATIC_LONG:
        if (data->debugLevel>2)
          printf("WSM: ONLY_LONG_SEQUENCE\n");
        windowSequence[MONO_CHAN] = ONLY_LONG_SEQUENCE;
        break;
      case STATIC_SHORT :
        if (data->debugLevel>2)
          printf("WSM:EIGHT_SHORT_SEQUENCE\n");
        windowSequence[MONO_CHAN] = EIGHT_SHORT_SEQUENCE;
        break;
      case LS_STARTSTOP_SEQUENCE :
        if (data->debugLevel>2)
          printf("WSM: LS_STARTSTOP_SEQUENCE\n");

        if (data->windowSequencePrev[MONO_CHAN]==ONLY_LONG_SEQUENCE)
          windowSequence[MONO_CHAN] = LONG_START_SEQUENCE;
        else if (data->windowSequencePrev[MONO_CHAN]==LONG_START_SEQUENCE)
          windowSequence[MONO_CHAN] = LONG_STOP_SEQUENCE;
        else if (data->windowSequencePrev[MONO_CHAN]==LONG_STOP_SEQUENCE)
          windowSequence[MONO_CHAN] = ONLY_LONG_SEQUENCE;

        break;
      case LONG_SHORT_SEQUENCE :
        if (data->debugLevel>2)
          printf("WSM: LONG_SHORT_SEQUENCE\n");

        if (data->windowSequencePrev[MONO_CHAN]==ONLY_LONG_SEQUENCE)
          windowSequence[MONO_CHAN] = LONG_START_SEQUENCE;
        else if (data->windowSequencePrev[MONO_CHAN]==LONG_START_SEQUENCE)
          windowSequence[MONO_CHAN] = EIGHT_SHORT_SEQUENCE;
        else if (data->windowSequencePrev[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)
          windowSequence[MONO_CHAN] = LONG_STOP_SEQUENCE;
        else if (data->windowSequencePrev[MONO_CHAN]==LONG_STOP_SEQUENCE)
          windowSequence[MONO_CHAN] = ONLY_LONG_SEQUENCE;

        break;

      case FFT_PE_WINDOW_SWITCHING :
        windowSequence[MONO_CHAN] =
          ntt_win_sw(data->twoFrame_DTimeSigBuf,
                     data->channels_total,
                     data->sampling_rate,
                     data->block_size_samples,
                     data->windowSequencePrev[MONO_CHAN],
                     data->win_sw_module);
        break;

      case NON_MEANINGFUL_TEST_SEQUENCE :
        if (data->windowSequenceCounter == sizeof(windowSequenceNM)/sizeof(windowSequenceNM[0]))
          data->windowSequenceCounter = 0;

        windowSequence[MONO_CHAN] = windowSequenceNM[data->windowSequenceCounter++];
        break;

      default:
        CommonWarning("Encode: unsupported window switching mode %d", data->win_switch_mode );
        return -1;
      }

      if (data->debugLevel>1)
        fprintf(stderr,"EncTfFrame: Blocktype   %d \n",windowSequence[MONO_CHAN] );

      /* Perform window shape adaptation depending on the selected mode */
      switch( data->windowShapeAdapt ) {
      case STATIC:
        windowShape[MONO_CHAN] = data->windowShapePrev[MONO_CHAN];
        break;

      case TOGGLE:
        if( data->windowShapeSeqCounter == 4 )
          data->windowShapeSeqCounter = 0;

        if (data->codecMode == AAC_LD)
          windowShape[MONO_CHAN] = windowShapeSeqToggleLD[data->windowShapeSeqCounter++];
        else
          windowShape[MONO_CHAN] = windowShapeSeqToggle[data->windowShapeSeqCounter++];
        break;

      default:
        CommonWarning("Encode: unsupported window shape adaptation mode (for Low Delay AAC)", data->windowShapeAdapt );
        return -1;
      }

      if (data->debugLevel>1)
        fprintf(stderr,"EncTfFrame: windowshape %d \n",windowShape[MONO_CHAN] );

      /* common window */
			for (i_ch=1; i_ch<data->channels_total;i_ch++) {
				windowShape[i_ch] = windowShape[MONO_CHAN];
				windowSequence[i_ch] = windowSequence[MONO_CHAN];
			}

			/* SAMSUNG_2005-09-30 */
			if(data->codecMode == TF_BSAC)
			{
				int stidx = 0;
				for(i=0; i<data->ch_elm_total; i++)
				{
					struct channelElementData *chData = &data->chElmData[i];
					if(chData->ch_type == LFE)
            windowSequence[chData->startChIdx] = ONLY_LONG_SEQUENCE;
				}
			}
			/* ~SAMSUNG_2005-09-30 */
    }

  /******************************************************************************************************************************
  *
  * son_AACpp
  *
  ******************************************************************************************************************************/

    if (data->pp_select == AAC_PP) {
      int band;

      if (data->debugLevel>1)
        fprintf(stderr, "EncTfFrame: AAC-PP\n");

      for(i_ch=0;i_ch<data->channels_total;i_ch++){ /* 971010 added YT*/

        /* polyphase quadrature filter */
        son_pqf_main(data->DTimeSigLookAheadBufForGC[i_ch],
                     data->block_size_samples,
                     i_ch, data->DBandSigLookAheadBuf[i_ch]);
        /* set analysis buffer for gain control */
        for (band = 0; band < NBANDS; band++) {
          memcpy((char *)&data->DBandSigBufForGCAnalysis[i_ch][band][data->block_size_samples/NBANDS*2],
                 (char *)data->DBandSigLookAheadBuf[i_ch][band],
                 data->block_size_samples/NBANDS*sizeof(double));
        }

        /* gain detector */
	if(data->gc_switch == GC_PRESENT) {
          son_gc_detect(data->DBandSigBufForGCAnalysis[i_ch],
                        data->block_size_samples,
                        windowSequence[i_ch], i_ch, data->gainInfo[i_ch]);
        } else {
          son_gc_detect_reset(data->DBandSigBufForGCAnalysis[i_ch],
                              data->block_size_samples,
                              windowSequence[i_ch], i_ch, data->gainInfo[i_ch]);
        }

        /* gain modifier */
        son_gc_modifier(data->DBandSigBufForGCAnalysis[i_ch],
                        data->gainInfo[i_ch],
                        data->block_size_samples,
                        windowSequence[i_ch], i_ch,
                        data->DBandSigWithGCBuf[i_ch]);
        /* shift analysis buffer */
        for (band = 0; band < NBANDS; band++) {
          memcpy((char *)data->DBandSigBufForGCAnalysis[i_ch][band],
                 (char *)&data->DBandSigBufForGCAnalysis[i_ch][band][data->block_size_samples/NBANDS],
                 data->block_size_samples/NBANDS*2*sizeof(double));
        }
      } /* for i_ch...*/
    } /* if (pp_select == AAC_PP).. */


  /******************************************************************************************************************************
  *
  * T/F mapping
  *
  ******************************************************************************************************************************/

    if (data->debugLevel>1)
      fprintf(stderr, "EncTfFrame: filterbank, t/f mapping\n");

    if (data->pp_select == AAC_PP) {
      int band;
      /* added 971010 YT*/
      for (i_ch=0; i_ch<data->channels_total; i_ch++) {

        for (band = 0; band < NBANDS; band++) {
          buffer2freq(data->DBandSigWithGCBuf[i_ch][band],
                      &data->spectral_line_vector_for_gc[i_ch][data->block_size_samples/NBANDS*band],
                      data->overlap_buffer[i_ch],	/* dummy */
                      windowSequence[i_ch],
                      windowShape[i_ch],
                      data->windowShapePrev[i_ch],
                      data->block_size_samples/NBANDS,
                      data->block_size_samples/NBANDS/NSHORT,
                      NON_OVERLAPPED_MODE,
                      0,0,
                      NSHORT);
        }
        son_gc_arrangeSpecEnc(data->spectral_line_vector_for_gc[i_ch],
                              data->block_size_samples,
                              windowSequence[i_ch],
                              data->spectral_line_vector[i_ch]);
      } /* for(i_ch...*/
    } else {
#ifdef DEBUGPLOT
      plotSend("l", "timeL",  MTV_DOUBLE,data->block_size_samples, data->DBandSigWithGCBuf[0], NULL);
      if (i_ch==2)
        plotSend("r", "timeR",  MTV_DOUBLE,data->block_size_samples,data->DBandSigWithGCBuf[1],  NULL);
#endif

      for (i_ch=0; i_ch<data->channels_total; i_ch++) {
#ifndef I2R_LOSSLESS
	buffer2freq(data->DTimeSigBuf[i_ch],
		    data->spectral_line_vector[i_ch],
		    data->overlap_buffer[i_ch],
		    windowSequence[i_ch],
                    windowShape[i_ch],
                    data->windowShapePrev[i_ch],
		    data->block_size_samples,
		    data->block_size_samples/NSHORT,
		    OVERLAPPED_MODE,
		    0,0,
		    NSHORT		/* HP 971023 */
		    );
      }
#else

      for (i=0;i<osf*1024;i++) {
        data->int_DTimeSigBuf[i_ch][i] = (int)( data->DTimeSigBuf[i_ch][i]/pcm_scaling[data->ll_info[i_ch].type_PCM]);
        data->int_DTimeSigLookAheadBuf[i_ch][i] = (int)( data->DTimeSigLookAheadBuf[i_ch][i]/pcm_scaling[data->ll_info[i_ch].type_PCM]);
      }

    }

    if (msStereoIntMDCTMode == 0) {
      msStereoIntMDCT = 0;
    } else {
      msStereoIntMDCT = 1;
    }

    data->ms_select = (msStereoIntMDCT==1)?2:0;  /* ms follow msStereoIntMDCT decision */

	for (i_ch=0;i_ch<data->channels_total;i_ch++) {
	  if (coreenable) {
	    block_type_current = windowSequence[i_ch]; /* block_type[i_ch]; */
	  } else {
	    block_type_current = block_type_lossless_only;
	  }
	  if (/*channelInfo[i_ch].cpe*/(data->channels_total == 2) && msStereoIntMDCT) {
	    /* apply StereoIntMDCT once per CPE */
	    if (i_ch == 0 /*channelInfo[i_ch].ch_is_left*/) {
	      int leftChan=i_ch;
	      int rightChan=i_ch+1; /* channelInfo[i_ch].paired_ch; */

	      StereoIntMDCT(data->int_DTimeSigBuf[leftChan],
			    data->int_DTimeSigBuf[rightChan],
			    data->mdctTimeBuf[leftChan],
			    data->mdctTimeBuf[rightChan],
			    data->int_spectral_line_vector[leftChan],
			    data->int_spectral_line_vector[rightChan],
			    block_type_current,
			    WS_FHG,
			    osf);
	    }
	  } else {
	    IntMDCT(data->int_DTimeSigBuf[i_ch],
		    data->mdctTimeBuf[i_ch],
		    data->int_spectral_line_vector[i_ch],
		    block_type_current,
		    WS_FHG,
		    osf);
	  }
	}

	if (!coreenable) {
	  if (block_type_lossless_only == EIGHT_SHORT_SEQUENCE/*ONLY_SHORT_WINDOW*/) {
	    for (i_ch=0;i_ch<data->channels_total;i_ch++) {
	      /* group spectrum as {2,2,2,2} */
	      int i;
	      int tmp_spec[MAX_OSF*1024];
	      int N = osf*1024;
	      int Nshort = osf*128;

	      for (i=0; i<N; i++) {
		tmp_spec[i] = data->int_spectral_line_vector[i_ch][i];
	      }
	      for (i=0; i<Nshort; i++) {
		data->int_spectral_line_vector[i_ch][2*i] = tmp_spec[i];
		data->int_spectral_line_vector[i_ch][2*i+1] = tmp_spec[Nshort+i];
		data->int_spectral_line_vector[i_ch][2*Nshort+2*i] = tmp_spec[2*Nshort+i];
		data->int_spectral_line_vector[i_ch][2*Nshort+2*i+1] = tmp_spec[3*Nshort+i];
		data->int_spectral_line_vector[i_ch][4*Nshort+2*i] = tmp_spec[4*Nshort+i];
		data->int_spectral_line_vector[i_ch][4*Nshort+2*i+1] = tmp_spec[5*Nshort+i];
		data->int_spectral_line_vector[i_ch][6*Nshort+2*i] = tmp_spec[6*Nshort+i];
		data->int_spectral_line_vector[i_ch][6*Nshort+2*i+1] = tmp_spec[7*Nshort+i];
	      }
	    }
	  }
       	}

	if (coreenable) {


          /* prepare for AAC_QC */

          for (i_ch=0; i_ch<data->channels_total; i_ch++)
            {
              int i;
              if ((data->channels_total == 2) && (i_ch==0))/*(channelInfo[i_ch].present) */
                {
                  int leftChan=i_ch;
                  int rightChan=i_ch + 1; /* channelInfo[i_ch].paired_ch; */

                  /*  channelInfo[leftChan].common_window = 1; */


                  if (1) /*channelInfo[leftChan].ms_info.is_present*/ /*msInfo->ms_mask == 2)*/ /* only "all on" supported now*/
                    {
                      data->ll_info[leftChan].CoreScaling =  \
                        (windowSequence[leftChan] == EIGHT_SHORT_SEQUENCE)?MDCT_SCALING_SHORT_MS:MDCT_SCALING_MS;
                      data->ll_info[rightChan].CoreScaling = \
                        (windowSequence[rightChan] == EIGHT_SHORT_SEQUENCE)?MDCT_SCALING_SHORT_MS:MDCT_SCALING_MS;
                    }
                  else
                    {
                      data->ll_info[leftChan].CoreScaling =  \
                        (windowSequence[leftChan] == EIGHT_SHORT_SEQUENCE)?MDCT_SCALING_SHORT:MDCT_SCALING;
                      data->ll_info[rightChan].CoreScaling = \
                        (windowSequence[rightChan] == EIGHT_SHORT_SEQUENCE)?MDCT_SCALING_SHORT:MDCT_SCALING;
                    }

                }
              else /*if (!channelInfo[i_ch].cpe)*/
                data->ll_info[i_ch].CoreScaling = (windowSequence[i_ch] == EIGHT_SHORT_SEQUENCE)?MDCT_SCALING_SHORT:MDCT_SCALING;
              data->ll_info[i_ch].CoreScaling *= pcm_scaling[data->ll_info[i_ch].type_PCM];

              for (i=0;i<1024*osf;i++)
                {
                  data->spectral_line_vector[i_ch][i] = (double)(data->int_spectral_line_vector[i_ch][i]/* +(double)rand()*6/RAND_MAX-3 */)  *\
                    data->ll_info[i_ch].CoreScaling;
                }
            }
	} /* if core_enable*/

#endif
#ifdef DEBUGPLOT
      plotSend("l", "specL",  MTV_DOUBLE_SQA, data->block_size_samples,data->baselayer_spectral_line_vector[0] , NULL);
      if (i_ch==2)
        plotSend("r", "specR",  MTV_DOUBLE_SQA,data->block_size_samples,data->baselayer_spectral_line_vector[1] ,  NULL);
#endif

    }

  } /* pre processing : Time -> Freq */



  /******************************************************************************************************************************
  *
  * psychoacoustic
  *
  ******************************************************************************************************************************/

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: psychoacoustic initialisation\n");

  EncTf_psycho_acoustic( (double)data->sampling_rate,
                         data->channels_total,
                         data->block_size_samples,
                         0,
                         chpo_long, chpo_short );

  if (data->debugLevel>2) fprintf(stderr,"NNNNN %d \n",chpo_long[0].no_of_cb);

  /******************************************************************************************************************************
  *
  * adapt ratios of psychoacoustic module to codec scale factor bands
  *
  ******************************************************************************************************************************/

  for (i_ch=0; i_ch<data->channels_total; i_ch++) {
    switch( windowSequence[i_ch] ) {
    case ONLY_LONG_SEQUENCE :

      memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_long[i_ch].cb_width, (NSFB_LONG+1)*sizeof(int) );
      nr_of_sfb[i_ch] = chpo_long[i_ch].no_of_cb;
      p_ratio[i_ch]   = chpo_long[i_ch].p_ratio;
      break;
    case LONG_START_SEQUENCE :
      memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_long[i_ch].cb_width, (NSFB_LONG+1)*sizeof(int) );
      nr_of_sfb[i_ch] = chpo_long[i_ch].no_of_cb;
      p_ratio[i_ch]   = chpo_long[i_ch].p_ratio;
      break;
    case EIGHT_SHORT_SEQUENCE :
      memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_short[i_ch][0].cb_width, (NSFB_SHORT+1)*sizeof(int) );
      nr_of_sfb[i_ch] = chpo_short[i_ch][0].no_of_cb;
      p_ratio[i_ch]   = chpo_short[i_ch][0].p_ratio;
      break;
    case LONG_STOP_SEQUENCE :
      memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_long[i_ch].cb_width, (NSFB_LONG+1)*sizeof(int) );
      nr_of_sfb[i_ch] = chpo_long[i_ch].no_of_cb;
      p_ratio[i_ch]   = chpo_long[i_ch].p_ratio;
      break;
    default:
      /* to prevent undefined ptr just use a meaningful value to allow for experiments with the meachanism above */
      memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_long[i_ch].cb_width, (NSFB_LONG+1)*sizeof(int) );
      nr_of_sfb[i_ch]  = chpo_long[i_ch].no_of_cb;
      p_ratio[i_ch] = chpo_long[i_ch].p_ratio;
      break;
    }
  }

  /* Set the AAC grouping information.  */
  if (windowSequence[MONO_CHAN] == EIGHT_SHORT_SEQUENCE) {
    num_window_groups=8;
    window_group_length[0] = 1;
    window_group_length[1] = 1;
    window_group_length[2] = 1;
    window_group_length[3] = 1;
    window_group_length[4] = 1;
    window_group_length[5] = 1;
    window_group_length[6] = 1;
    window_group_length[7] = 1;
  } else {
    num_window_groups = 1;
    window_group_length[0] = 1;
  }

  /* Calculate sfb-offset table. Needed by (at least) TNS and LTP. */
  for(i_ch = 0; i_ch < data->channels_total; i_ch++) {
    sfb_offset[i_ch][0] = 0;
    k=0;
    for(i = 0; i < nr_of_sfb[i_ch]; i++ ) {
      sfb_offset[i_ch][i] = k;
      k += sfb_width_table[i_ch][i];
    }
    sfb_offset[i_ch][i] = k;
  }

  /******************************************************************************************************************************
  *
  * carry out TNS processing
  *
  ******************************************************************************************************************************/
	/* SAMSUNG_2005-09-30 : find LFE channel index */
	if(data->codecMode == TF_BSAC)
	{
		for(i=0; i<data->ch_elm_total; i++)
		{
			if(data->chElmData[i].ch_type == LFE)
				lfe_chIdx = data->chElmData[i].startChIdx;
		}
	}
	/* ~SAMSUNG_2005-09-30 */

  if (data->tns_select!=NO_TNS) {
    if (data->debugLevel>1)
      fprintf(stderr,"EncTfFrame: TNS\n");

    for (i_ch=0; i_ch<data->channels_total; i_ch++) {
      if(i_ch != lfe_chIdx) /* SAMSUNG_2005-09-30 */
        TnsEncode(nr_of_sfb[i_ch],                /* Number of bands per window */
                  nr_of_sfb[i_ch],                /* max_sfb */
                  windowSequence[i_ch],           /* block type */
                  sfb_offset[i_ch],               /* Scalefactor band offset table */
                  data->spectral_line_vector[i_ch], /* Spectral data array */
                  data->tnsInfo[i_ch]);           /* TNS info */
    }
  }

  /******************************************************************************************************************************
  *
  * compute energy and allowed distortion from the original spectrum
  *
  ******************************************************************************************************************************/
  /* These have to be done before prediction (not, for example, inside
     the quantizer) since the signal going to the quantizer is changed
     by the prediction. Do not use p_ratio after this. Use
     allowed_distortion (and p_energy) instead.
     1997-11-23 Mikko Suonio  */

  if ((data->codecMode==TF_AAC_PLAIN)||
      (data->codecMode==TF_AAC_LD)||
      (data->codecMode==TF_BSAC)||
      (data->codecMode==TF_AAC_SCA)) {

    int group, index, j, sfb;
    double dtmp;

    if (data->debugLevel>1)
      fprintf(stderr,"EncTfFrame: compute allowed distorsion\n");

    data->max_sfb = nr_of_sfb[MONO_CHAN];
    for (i_ch = 0; i_ch < data->channels_total; i_ch++) {
      index = 0;
      /* calculate the scale factor band energy in window groups  */
      for (group = 0; group < num_window_groups; group++) {
        for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++)
          p_energy[i_ch][sfb + group * nr_of_sfb[i_ch]] = 0.0;
        for (i = 0; i < window_group_length[group]; i++) {
          for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++) {
            for (j = 0; j < sfb_width_table[i_ch][sfb]; j++) {
              dtmp = data->spectral_line_vector[i_ch][index++];
              p_energy[i_ch][sfb + group * nr_of_sfb[i_ch]] += dtmp * dtmp;

              if (data->debugLevel > 8 )
                fprintf(stderr,"enc_tf.c: spectral_line_vector index %d; %d %d %d %d %d\n", index, i_ch, group, i, sfb, j);
            }
          }
        }
      }
      /* calculate the allowed distortion */
      /* Currently the ratios from psychoacoustic model
         are common for all subwindows, that is, there is only
         num_of_sfb[i_ch] elements in p_ratio[i_ch]. However, the
         distortion has to be specified for scalefactor bands in all
         window groups. */
      for (group = 0; group < num_window_groups; group++) {
        for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++) {
          index = sfb + group * nr_of_sfb[i_ch];
          if ((10 * log10(p_energy[i_ch][index] + 1e-15)) > 70)
            allowed_distortion[i_ch][index] = p_energy[i_ch][index] * p_ratio[i_ch][sfb];
          else
            allowed_distortion[i_ch][index] = p_energy[i_ch][index] * 1.1;
        }
      }
    }
  }


  /******************************************************************************************************************************
  *
  * prediction
  *
  ******************************************************************************************************************************/

  if (data->pred_type == NOK_LTP) {
     

    if (data->debugLevel>1)
      fprintf(stderr,"EncTfFrame: LTP\n");

    for(i_ch = 0; i_ch < data->channels_total; i_ch++) {
      QC_MOD_SELECT qc_mode = (data->codecMode==TF_AAC_LD)?AAC_LD:AAC_QC;
      /* keep original spectrum if encoding extension layers */
      if (IS_SCAL_CODEC(data->codecMode)) {
        for(i = 0; i < data->block_size_samples; i++)
          data->baselayer_spectral_line_vector[i_ch][i] = data->spectral_line_vector[i_ch][i];
      }
      if (windowSequence[i_ch]!=EIGHT_SHORT_SEQUENCE) {
        nok_ltp_enc(data->spectral_line_vector[i_ch],
                    data->twoFrame_DTimeSigBuf[i_ch],
                    windowSequence[i_ch],
                    windowShape[i_ch], data->windowShapePrev[i_ch],
                    data->block_size_samples,
                    data->block_size_samples/NSHORT,
                    &sfb_offset[i_ch][0], nr_of_sfb[i_ch],
                    &data->nok_lt_status[i_ch],
                    data->tnsInfo[i_ch] ,
                    qc_mode);
      } else {
        
      }
    }
  }


/******************************************************************************************************************************/
/*
 * quantization and coding
 *
 ******************************************************************************************************************************/

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: select MS\n");
/* SAMSUNG_2005-09-30 */
	if(data->codecMode == TF_BSAC)
	{
		for(i=0; i<data->ch_elm_total; i++)
		{
			struct channelElementData *chData = &data->chElmData[i];
			if(chData->chNum == 2)
			{
				select_ms(&data->spectral_line_vector[chData->startChIdx],
				data->block_size_samples,
				num_window_groups,
				window_group_length,
				sfb_offset[MONO_CHAN],
				(core_channels==2)?core_max_sfb:0,
				nr_of_sfb[MONO_CHAN],
				&chData->msInfo,
				chData->ms_select,
				data->debugLevel);
			}
		}
	}
	else
/* ~SAMSUNG_2005-09-30 */
	{

		/* does not apply the selected M/S mask! */
		if (data->channels_total == 2) {
			select_ms(data->spectral_line_vector,
			data->block_size_samples,
			num_window_groups,
			window_group_length,
			sfb_offset[MONO_CHAN],
			(core_channels==2)?core_max_sfb:0,
			nr_of_sfb[MONO_CHAN],
			&data->msInfo,
			data->ms_select,
			data->debugLevel);
		}
	}

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: quantization\n");

  {
    int average_bits[MAX_TF_LAYER];
    int average_bits_total;
    int used_bits = 0;
    int num_bits_available;
    int min_bits_needed;

    for (i=0; i<data->layer_num; i++) {
      average_bits[i] = (data->bitrates[i]*data->block_size_samples) / data->sampling_rate;
    }
    average_bits_total = (data->bitrates_total*data->block_size_samples) / data->sampling_rate;
		frameNumBit = average_bits_total; /* SAMSUNG_2005-09-30 */

    /* bit budget */
    num_bits_available = (long)(average_bits_total + data->available_bitreservoir_bits);
    min_bits_needed = (long)(data->available_bitreservoir_bits + 2*average_bits_total - data->max_bitreservoir_bits); /* 2*average_bits_total, because one full frame always has to stay in buffer and one other frame is processed each frame */
    if (min_bits_needed < 0) min_bits_needed = 0;

    /* QC selection is currently assumed to be mutually exclusive. */
    switch( data->codecMode ) {

    case TF_AAC_PLAIN:
    /*case TF_BSAC: *//* YB : 971106 */ /*?*/
    case TF_AAC_LD:
	used_bits += EncTf_aacplain_encode(
			data->spectral_line_vector,
			data->reconstructed_spectrum,
			p_energy,
			allowed_distortion,
			sfb_width_table,
			sfb_offset,
			nr_of_sfb,
			num_bits_available,
			min_bits_needed,
			aacmux[0],
      (data->ep_config!=-1),
                        (data->codecMode == TF_AAC_LD),
			windowSequence,
			windowShape,
			data->windowShapePrev,
			data->aacAllowScalefacs,
			data->block_size_samples,
			data->channels_total,
			data->sampling_rate,
			&data->msInfo,
			data->tnsInfo,
			data->nok_lt_status,
			data->pred_type,
			data->pns_sfb_start,
			data->gc_switch,
			data->max_band,
			data->gainInfo,
			num_window_groups,
			window_group_length,
			data->bw_limit[0],
			data->debugLevel);
	break;
/* SAMSUNG_2005-09-30 */
		case TF_BSAC:
                  used_bits += EncTf_bsac_encode(
				data->chElmData,
				data->ch_elm_total,
				data->spectral_line_vector,
				data->reconstructed_spectrum,
				p_energy,
				allowed_distortion,
				sfb_width_table,
				sfb_offset,
				nr_of_sfb,
				/*num_bits_available,*/
				data->available_bitreservoir_bits,
				frameNumBit,
				output_au,
				windowSequence,
				windowShape,
				data->windowShapePrev,
				/*data->aacAllowScalefacs,*/
				data->block_size_samples,
				data->channels_total,
				data->sampling_rate,
				&data->msInfo,
				data->tnsInfo,
				data->pns_sfb_start,
				num_window_groups,
				window_group_length,
				data->bw_limit[0],
				data->debugLevel,
				data->lfePresent,
/* 20070530 SBR */
				p_time_signal_orig != NULL,
				sbrChan
				);
		break;

/* ~SAMSUNG_2005-09-30 */

    case TF_AAC_SCA:
      used_bits += EncTf_aacscal_encode(
			data->spectral_line_vector,
			data->baselayer_spectral_line_vector,
			data->reconstructed_spectrum,
			p_energy,
			allowed_distortion,
			sfb_width_table,
			&sfb_offset[MONO_CHAN][0],
			nr_of_sfb,
			average_bits,
			data->available_bitreservoir_bits,
			min_bits_needed,
			aacmux,
#ifdef I2R_LOSSLESS
			slsmux,
			data->quant_aac,
			&data->quantInfo[MONO_CHAN],
			&data->ll_info[MONO_CHAN],
			data->int_spectral_line_vector,
			&chpo_short[MONO_CHAN],
			osf,
#endif
			data->layer_num,
			core_max_sfb,
			isFirstTfLayer,
			(core_channels==1), /*hasMonoCore*/
			&data->msInfo,
			core_transmitted_tns,
			windowSequence,
			windowShape,
			data->windowShapePrev,
			data->aacAllowScalefacs,
			data->block_size_samples,
			data->channels,  /* nr of audio channels */
			data->sampling_rate,
			data->tnsInfo,
			data->nok_lt_status,
			data->pred_type,
			num_window_groups,
			window_group_length,
			data->bw_limit,
                        data->aacSimulcast,
			data->debugLevel);

#ifdef DEBUGPLOT
        plotSend("l", "recspecL",  MTV_DOUBLE_SQA,data->block_size_samples,data->reconstructed_spectrum[0] , NULL);
        if (core_channels==2)
          plotSend("r", "recspecR",  MTV_DOUBLE_SQA,data->block_size_samples,data->reconstructed_spectrum[1] ,  NULL);
#endif

      break;

#ifdef BSAC
/*OK: temporary disabled*/
#endif



      case TF_TVQ:
	used_bits += EncTf_tvq_encode(
                    data->spectral_line_vector,
                    data->baselayer_spectral_line_vector,
                    data->reconstructed_spectrum,
                    windowSequence,
                    windowShape,
                    &ntt_index,
                    &ntt_index_scl,
                    &param_ntt,
                    data->block_size_samples,
                    output_au,
                    data->mat_shift,
                    sfb_width_table,
                    sfb_offset,
                    nr_of_sfb,
                    &data->max_sfb,
                    &data->msInfo,
                    data->tnsInfo,
                    data->pred_type,
                    data->nok_lt_status,
                    data->debugLevel);

	break;

      default:
        CommonExit(-1,"\ncase not in switch");
        break;

      } /* switch qc_select */

    if (data->debugLevel>2) {
      fprintf(stderr,"EncTfFrame: bitbuffer: having %i bit reservoir, produced: %i bits, consumed %i bits\n",
              data->available_bitreservoir_bits,
              used_bits, average_bits_total);
    }
    data->available_bitreservoir_bits -= used_bits;
    data->available_bitreservoir_bits += average_bits_total;
    if (data->available_bitreservoir_bits > data->max_bitreservoir_bits) {
      fprintf(stderr,"WARNING: bit reservoir got too few bits\n");
      data->available_bitreservoir_bits = data->max_bitreservoir_bits;
    }
    if (data->available_bitreservoir_bits < 0) {
      fprintf(stderr,"WARNING: no bits in bit reservoir remaining\n");
    }

    if (data->debugLevel>2) {
      fprintf(stderr,"EncTfFrame: bitbuffer: => now %i bit reservoir\n", data->available_bitreservoir_bits);
    }

  }   /* Quantization and coding block */

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: prepare next frame\n");

  /* save the actual window shape for next frame */
  for(i_ch=0; i_ch<data->channels_total; i_ch++) {
    data->windowShapePrev[i_ch] = windowShape[i_ch];
    data->windowSequencePrev[i_ch] = windowSequence[i_ch];
  }

  for (i=0; i<data->track_num; i++) {
    BsClose(output_au[i]);
  }
  free(output_au);

  for (i=0; i<data->layer_num; i++) if (aacmux[i]!=NULL) aacBitMux_free(aacmux[i]);

/* #ifdef I2R_LOSSLESS */

  if (data->debugLevel>1)
    fprintf(stderr,"EncTfFrame: successful return\n");
  return 0;
}



