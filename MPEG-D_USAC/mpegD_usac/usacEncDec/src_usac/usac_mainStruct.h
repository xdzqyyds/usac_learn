/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp, University of Erlangen

Initial author:

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
$Id: usac_mainStruct.h,v 1.17.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/


#ifndef _USAC_MAIN_STRUCT_H_INCLUDED
#define _USAC_MAIN_STRUCT_H_INCLUDED

#include "allHandles.h"
#include "vmtypes.h"
#include "block.h"
#include "interface.h"
#include "usac_interface.h"
#include "tf_mainStruct.h" /* for Info */
#include "usac_arith_dec.h"    /* for arithQ*/
#include "all.h"
#include "tf_mainHandle.h"
#include "usac_tcx_mdct.h"

#include "cnst.h"
#include "nclass.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#include "ct_sbrconst.h"
#endif

#include "sac_polyphase.h"

#define NB_PITMEM              (NB_SUBFR*2-1) /* = 7 */

typedef struct tagTwMdct {
  int dummy;
} USAC_TW, *HANDLE_TW;

typedef struct tagUsacFdDecoder {
  int dummy;

} USAC_FD_DECODER, *HANDLE_USAC_FD_DECODER;

typedef struct
{
  int   mode;              /* LPD mode (0=ACELP frame)                */
  int   nbits;             /* number of bits used by ACELP or TCX     */

  /* signal memory */
  float Aq[2*(M+1)];       /* for TCX overlap-add synthesis (2 subfr) */
  float Ai[2*(M+1)];       /* for OA weighted synthesis (2 subfr)     */
  float syn[M+128];       /* synthesis memory                        */
  float wsyn[1+128];      /* weighted synthesis memory               */

  /* ACELP memory */
  float Aexc[2*L_DIV_1024];     /* ACELP exc memory (Aq)                   */

  /* FAC memory */
  int   RE8prm[LFAC_1024];      /* MDCT RE8 coefficients (encoded in ACELP)*/

  /* TCX memory */
  float Txn[128];        /* TCX target memory (2 subfr, use Aq)     */
  float Txnq[1+(2*128)];  /* Q target (overlap or ACELP+ZIR, use Aq) */
  float Txnq_fac;          /* Q target with fac (use Aq)              */

} LPD_state;

typedef struct tagUsacTdDecoder {

  int lFrame;   /* Input frame length (long TCX)         */
  int lDiv;     /* ACELP or short TCX frame length       */
  int nbSubfr;  /* Number of 5ms subframe per 20ms frame */

  /* dec_main.c */
  int last_mode;                         /* last mode in previous 80ms frame */

  /* dec_lf.c */
  float old_Aq[2*(M+1)];                 /* last tcx overlap synthesis       */
  float old_xnq[1+(2*LFAC_1024)];
  float old_xnq_synth[1+(2*LFAC_1024)];
  float old_synth[PIT_MAX_MAX+SYN_DELAY_1024];/* synthesis memory                 */
  float old_exc[PIT_MAX_MAX+L_INTERPOL];      /* old excitation vector (>20ms)    */

  /* bass_pf.c */
  float old_noise_pf[L_FILT+BPF_DELAY_1024];  /* bass post-filter: noise memory   */
  int old_T_pf[SYN_SFD_1024];                 /* bass post-filter: old pitch      */
  float old_gain_pf[SYN_SFD_1024];            /* bass post-filter: old pitch gain */

  /* FAC */
  float FAC_gain;
  float FAC_alfd[LFAC_1024/4];

  float lsfold[M];                     /* old isf (frequency domain) */
  float lspold[M];                     /* old isp (immittance spectral pairs) */
  float past_lsfq[M];                  /* past isf quantizer */
  float lsf_buf[L_MEANBUF*(M+1)];      /* old isf (for frame recovery) */
  int old_T0;                          /* old pitch value (for frame recovery) */
  int old_T0_frac;                     /* old pitch value (for frame recovery) */
  short seed_ace;                      /* seed memory (for random function) */
#ifdef UNIFIED_RANDOMSIGN
  unsigned int seed_tcx;               /* seed memory (for random function) */
#else
  short seed_tcx;                      /* seed memory (for random function) */
#endif
  float past_gpit;                     /* past gain of pitch (for frame recovery) */
  float past_gcode;                    /* past gain of code (for frame recovery) */
  float gc_threshold;
  float wsyn_rms;

  HANDLE_TCX_MDCT hTcxMdct;

  Tqi2 qbuf[N_MAX/2+4];

  int fscale;      /* scale factor (FSCALE_DENOM = 1.0)  */

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  float fdSynth_Buf[3*L_DIV_1024+1];
  float *fdSynth;
#else
  float fdSynth[2*L_DIV_1024+1];
#endif

#ifndef USAC_REFSOFT_COR1_NOFIX_09
  int prev_BassPostFilter_activ;
#endif

  float *pOlaBuffer;

} USAC_TD_DECODER, *HANDLE_USAC_TD_DECODER;

typedef struct tagUsacDecoder
{
  int blockSize;
  int twMdct[MAX_NUM_ELEMENTS];

  float *coef[Chans];
  float *data[Chans];
  float *state[Chans];

#ifndef USAC_REFSOFT_COR1_NOFIX_03
  float *coefSave[Chans]; /* save output for cplx prediction downmix */
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */

  WINDOW_SEQUENCE wnd[Winds];
  Wnd_Shape wnd_shape[Winds];

  byte *mask[Winds]; /* MS-Mask */
  byte *cb_map[Chans];
  short *factors[Chans];
  byte *group[Winds];

  TNS_frame_info *tns[Chans];
  int tns_data_present[Chans];
  int tns_ext_present[Chans];

  Info *info;

  int samplingRate;
  int fscale;       /* derived from sampling rate */

  /* data stream */

  int d_tag, d_cnt;
  byte d_bytes[Avjframe];

  MC_Info *mip;

  /*
   SSR Profile
   */
  int short_win_in_long;

  int max_band[Chans];

  HANDLE_USAC_TD_DECODER tddec[Chans];
  HANDLE_USAC_FD_DECODER fddec;

  SBRBITSTREAM ct_sbrBitStr[2];



  SAC_POLYPHASE_ANA_FILTERBANK *filterbank[6];

  /* TW-MDCT members */
  int tw_data_present[Chans];
  int *tw_ratio[Chans];

  /* Arith coding */
  int  arithPreviousSize[Chans];
  Tqi2 arithQ[Chans][1024/2+4];

  /*Noise Filling*/
  int bUseNoiseFilling[MAX_NUM_ELEMENTS];
  unsigned int nfSeed[Chans];

  /* FAC data*/
  int ApplyFAC[Chans];
  int facData[Chans][LFAC_1024+1];

} T_USAC_DECODER;

typedef struct drcOutputFile {
  char drc_file_name[FILENAME_MAX] ;
  FILE *save_drc_file;
} DRC_OUTPUT_FILE, *HANDLE_DRC_OUTPUT_FILE;

typedef struct drcInfo {
  int uniDrc_extEle_present;
  int loudnessInfo_configExt_present;
  int frameLength;
  int sbr_active;
} DRC_INFO, *HANDLE_DRC_INFO;

typedef struct {
  int tnsBevCore;
  int output_select;
  float* time_sample_vector[MAX_OSF * MAX_TIME_CHANNELS];
  float* spectral_line_vector[MAX_TIME_CHANNELS];
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS];
  WINDOW_SHAPE prev_windowShape[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS];

  USAC_CORE_MODE coreMode[MAX_TIME_CHANNELS];
  USAC_CORE_MODE prev_coreMode[MAX_TIME_CHANNELS];

#ifndef USAC_REFSOFT_COR1_NOFIX_06
  float* overlap_buffer_buf[MAX_OSF * MAX_TIME_CHANNELS];
#endif
  float* overlap_buffer[MAX_OSF * MAX_TIME_CHANNELS];

  Info** sfbInfo;
  int block_size_samples;
  float* sampleBuf[MAX_TIME_CHANNELS];
  HANDLE_MODULO_BUF_VM coreModuloBuffer;
  float* mdct_overlap_buffer;
  WINDOW_SEQUENCE windowSequenceLast[MAX_TIME_CHANNELS];
  int samplFreqFacCore;
  int decoded_bits;
  int mdctCoreOnly; /* debug switch */
  int twMdct[MAX_ELEMENTS];
  int bStereoSbr[MAX_ELEMENTS];
  int bPseudoLr[MAX_ELEMENTS];

  int FrameIsTD[MAX_TIME_CHANNELS];
  int FrameWasTD[MAX_TIME_CHANNELS];


  /* tw-mdct */
  float                warp_sum[MAX_TIME_CHANNELS][2];
  float               *warp_cont_mem[MAX_TIME_CHANNELS];
  float               *prev_sample_pos[MAX_TIME_CHANNELS];
  float               prev_tw_trans_len[MAX_TIME_CHANNELS][2];
  int               prev_tw_start_stop[MAX_TIME_CHANNELS][2];
  float               *prev_warped_time_sample_vector[MAX_TIME_CHANNELS];
  int osf; /* this defaults to 1 for non-sls */
  int sbrRatioIndex;

#ifdef CT_SBR
  int bDownSampleSbr;
  int runSbr;
#ifdef PARAMETRICSTEREO
  int sbrEnablePS;
#endif
#endif

  /*
   USAC Decoder data
   */
  HANDLE_USAC_DECODER usacDecoder;

  /*
   sbr Decoder
   */

#ifdef CT_SBR
  SBRDECODER ct_sbrDecoder;
  float sbrQmfBufferReal[TIMESLOT_BUFFER_SIZE][QMF_BUFFER_SIZE];
  float sbrQmfBufferImag[TIMESLOT_BUFFER_SIZE][QMF_BUFFER_SIZE];

#endif

/* FAC transition*/
  float  lastLPC[MAX_TIME_CHANNELS][M+1];
  float  acelpZIR[MAX_TIME_CHANNELS][1+(2*LFAC_1024)];

  int **alpha_q_re, **alpha_q_im;
  int *alpha_q_re_prev, *alpha_q_im_prev;
  int **cplx_pred_used;
  float *dmx_re_prev;

  /* usac independency flag */
  int usacIndependencyFlag;

  /* for debug purposes only, remove before MPEG upload! */
  int frameNo;

#ifdef SONY_PVC_DEC
  int	debug_array[64];
  int	sbr_mode;
#endif /* SONY_PVC_DEC */

  DRC_OUTPUT_FILE drc_payload_file;
  DRC_OUTPUT_FILE drc_config_file;
  DRC_OUTPUT_FILE drc_loudness_file;
  DRC_INFO drcInfo;
} USAC_DATA;



typedef struct tagUsacTdEncoder {

  int lFrame;   /* Input frame length (long TCX)         */
  int lDiv;     /* ACELP or short TCX frame length       */
  int nbSubfr;  /* Number of 5ms subframe per 20ms frame */

  short mode;                  /* ACELP core mode: 0..7 */
  int fscale; 

  float mem_lp_decim2[3];              /* wsp decimation filter memory */

  /* cod_main.c */
  int decim_frac;
  float mem_sig_in[4];                 /* hp50 filter memory */
  float mem_preemph;                   /* speech preemph filter memory */

  float old_speech_pe[L_OLD_SPEECH_HIGH_RATE+L_LPC0_1024];
  float old_speech[L_OLD_SPEECH_HIGH_RATE+L_LPC0_1024];   /* old speech vector at 12.8kHz */
  float old_synth[M];                  /* synthesis memory */
  
  /* cod_lf.c */
  float wsig[128];      
  LPD_state LPDmem;
  float old_d_wsp[PIT_MAX_MAX/OPL_DECIM];  /* Weighted speech vector */
  float old_exc[PIT_MAX_MAX+L_INTERPOL];   /* old excitation vector */

  float old_mem_wsyn;                  /* weighted synthesis memory */
  float old_mem_w0;                    /* weighted speech memory */
  float old_mem_xnq;                   /* quantized target memory */
  int old_ovlp_size;                   /* last tcx overlap size */

  float isfold[M];                     /* old isf (frequency domain) */
  float ispold[M];                     /* old isp (immittance spectral pairs) */
  float ispold_q[M];                   /* quantized old isp */
  float mem_wsp;                       /* wsp vector memory */

  /* memory of open-loop LTP */
  float ada_w;
  float ol_gain;
  short ol_wght_flg;
  
  long int old_ol_lag[5];
  int old_T0_med;
  float hp_old_wsp[L_FRAME_1024/OPL_DECIM+(PIT_MAX_MAX/OPL_DECIM)];
  float hp_ol_ltp_mem[/* HP_ORDER*2 */ 3*2+1];

  /* LP analysis window */
  float window[L_WINDOW_HIGH_RATE_1024];
  
  /* mdct-based tcx*/
  HANDLE_TCX_MDCT hTcxMdct;
  float xn_buffer[128];
  Tqi2  arithQ[1024/2+4];
  int   arithReset;

  short prev_mod;
  short codingModeSelection;

  int  nb_bits; /*number of bits used for a superframe*/

  /* ACELP startup, past FD synthesis including FD-FAC */
  float fdSynth[2*L_DIV_1024+1+M];
  float fdOrig[2*L_DIV_1024+1+M];
  int lowpassLine;

  /* left and right short overlap if short FD -> LPD */
  int lastWasShort;
  int nextIsShort;

} USAC_TD_ENCODER, *HANDLE_USAC_TD_ENCODER;


#endif  /* #ifndef _USAC_STRUCT_MAIN_H_INCLUDED */
