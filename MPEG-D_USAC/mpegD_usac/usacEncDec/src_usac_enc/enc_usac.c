/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:


and edited by: Jeremie Lecomte (Fraunhofer IIS)
               Markus Multrus (Fraunhofer IIS)

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

*******************************************************************************/
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "allHandles.h"
#include "encoder.h"

#include "block.h"
#include "ntt_win_sw.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */


#include "enc_usac.h"
#include "enc_tf.h"
#include "usac_mainStruct.h"   /* structs */
/*#include "usac_main.h"*/
#include "usac_mdct.h"
#include "tf_main.h"

#include "common_m4a.h"	/* common module */

/*sbr*/
#include "ct_sbr.h"

#include "sac_enc.h"
#include "sac_stream_enc.h"

/* aac */
#include "ms.h"
#include "psych.h"
#include "tns.h"
#include "tns3.h"
#include "scal_enc_frame.h"
#include "usac_fd_enc.h"
#include "usac_fd_qc.h"


#include "int_dec.h"

/*bitmux*/
#include "bitstream.h"
#include "usac_bitmux.h"
#include "flex_mux.h"

#include "plotmtv.h"
#include "usac_tw_enc.h"

#include "../src_usac/acelp_plus.h"
#include "usac_tw_tools.h"

#include "signal_classifier.h"

#ifdef SONY_PVC
#include "sony_pvcenc_prepro.h"
#endif /* SONY_PVC */

/* declarations */
static int EncUsac_data_free(HANDLE_ENCODER_DATA data);
static int EncUsac_tns_free(HANDLE_ENCODER_DATA data);


/* 20070530 SBR */
static SBR_CHAN sbrChan[MAX_TIME_CHANNELS];
extern int samplFreqIndex[];
extern long UsacSamplingFrequencyTable[32];

#ifdef __RESTRICT
#define restrict _Restrict
#else
#define restrict
#endif

int gUseFractionalDelayDecor = 0; 

/* encoded AU data */
struct usacFrame_data {
  unsigned int auLen;
  unsigned char au_data[USAC_MAX_EXTENSION_PAYLOAD_LEN];
};

/* encoded ASC data */
struct usacConfig_data {
  unsigned int configLen;
  unsigned char config_data[USAC_MAX_CONFIG_LEN];
};

/* audio pre-roll state */
typedef enum audioPreRoll_state {
  APR_IDLE,
  APR_PREPARE,
  APR_EMBED,
  APR_DELETE
} APR_STATE, *HANDLE_APR_STATE;

/* audio pre-roll bit distribution */
typedef struct audioPreRoll_bitbudget {
  int bits2Distribute;
  int bits2Save;
  int spreadFrames;
} APR_BITBUDGET, *HANDLE_APR_BITBUDGET;

/* audio pre-roll data */
typedef struct audioPreRollData {
  APR_STATE audioPreRollState;
  APR_BITBUDGET audioPreRollBitBudget;
  int audioPreRollExtID;
  int audioPreRollFrameID;
  USACCONFIG_DATA usacConfig;
  int applyCrossfade;
  int reserved;
  int numPreRollFrames;
  USACFRAME_DATA usacFrame[MPEG_USAC_CORE_MAX_AU];
} APR_DATA, *HANDLE_APRDATA;

/* Dynamic Range Control data */
typedef struct drcData {
  unsigned char *drcLoudnessInfoSet_ConfigExt;
  unsigned int  configExt_length;
  unsigned char *drcConfig_ExtElementConfig;
  unsigned int  extElementConfig_length;
  unsigned char *drcGains_ExtElement;
  int *drcGainBits_ExtElement;
} DRC_DATA, *HANDLE_DRCDATA;

/*** the main container structure ***/
struct tagEncoderSpecificData {
  int     debugLevel;

  unsigned int streamID_present;
  unsigned int streamID;
  int     bitrate;
  int     channels;
  int     bw_limit;  /* bandwidth limit of the spectrum */
  int     sampling_rate;      /* core coder sampling rate */
  int     sampling_rate_out;  /* output (i.e. SBR) sampling rate */
  int     block_size_samples; /* nr of samples per block in one audio channel */
  int     window_size_samples[MAX_TIME_CHANNELS];
  int     delay_encoder_frames;

  int     bUseFillElement;
  /*--- USAC independency flag ---*/
  int     usacIndependencyFlagInterval;
  int     usacIndependencyFlagCnt;

  /*--- TD/FD selection ---*/
  int frameCounter;
  int switchEveryNFrames;
  enum USAC_CODEC_MODE    codecMode;
  USAC_CORE_MODE   coreMode[MAX_TIME_CHANNELS];
  USAC_CORE_MODE   prev_coreMode[MAX_TIME_CHANNELS];
  USAC_CORE_MODE   next_coreMode[MAX_TIME_CHANNELS];

  /*---TD Data---*/
  float  tdBuffer[MAX_TIME_CHANNELS][L_FRAME_1024+L_NEXT_HIGH_RATE_1024];
  float  tdBuffer_past[MAX_TIME_CHANNELS][L_FRAME_1024+L_NEXT_HIGH_RATE_1024+L_LPC0_1024];
  HANDLE_USAC_TD_ENCODER tdenc[MAX_TIME_CHANNELS];
  int    total_nbbits[MAX_TIME_CHANNELS];   /* (o) : number of bits per superframe    */
  int    FD_nbbits_fac[MAX_TIME_CHANNELS];   /* (o) : number of bits used per FD FAC window    */
  int    TD_nbbits_fac[MAX_TIME_CHANNELS];   /* (o) : number of bits used per TD FAC window    */
  unsigned char TDfacData[MAX_TIME_CHANNELS][4*NBITS_MAX];
  int    td_bitrate[MAX_TIME_CHANNELS];
  int    acelp_core_mode[MAX_TIME_CHANNELS];

  /*---FD Data---*/
  int     flag_960;
  int     flag_768;
  int     ep_config;
  int     flag_twMdct;
  int     flag_noiseFilling;
  int     flag_harmonicBE;
  int     flag_winSwitchFD;
  int     layer_num;
  int     track_num;
  int     tracks_per_layer[MAX_TF_LAYER];
  int     max_bitreservoir_bits;
  int     available_bitreservoir_bits;
  TNS_COMPLEXITY        tns_select;
  int     ms_select;
  int     aacAllowScalefacs;
  int     max_sfb;
  float   warp_sum[MAX_TIME_CHANNELS][2];
  float  *warp_cont_mem[MAX_TIME_CHANNELS];

  MSInfo  msInfo;
  int     predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX];
  TNS_INFO *tnsInfo[MAX_TIME_CHANNELS];
  UsacICSinfo icsInfo[MAX_TIME_CHANNELS];
  UsacToolsInfo toolsInfo[MAX_TIME_CHANNELS];
  UsacQuantInfo quantInfo[MAX_TIME_CHANNELS];

  WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS];
  HANDLE_NTT_WINSW win_sw_module;
  HANDLE_TFDEC hTfDec;

  /*--- SBR Data ---*/
  int sbrRatioIndex;
  int sbrenable;
  struct channelElementData chElmData[MAX_CHANNEL_ELEMENT];
  int ch_elm_total;

  /*--- MPEGS 212 Data ---*/
  HANDLE_SPATIAL_ENC hSpatialEnc;
  int usac212enable;
  unsigned char SpatialData[MAX_MPEGS_BITS/8];
  unsigned long SpatialDataLength;
  int         flag_ipd;
  int         flag_mpsres;
  int         flag_cplxPred;
  char        tsdInputFileName[FILENAME_MAX];
  
  /*--- Data ---*/
  double *DTimeSigBuf[MAX_TIME_CHANNELS];
  double *DTimeSigLookAheadBuf[MAX_TIME_CHANNELS];
  double *spectral_line_vector[MAX_TIME_CHANNELS];
  double *twoFrame_DTimeSigBuf[MAX_TIME_CHANNELS]; /* temporary fix to the buffer size problem. */
  double *overlap_buffer[MAX_TIME_CHANNELS];
  short  tdOutStream[MAX_TIME_CHANNELS][NBITS_MAX];
  short  facOutStream[MAX_TIME_CHANNELS][NBITS_MAX];

  /*--- Synth ---*/
  double *reconstructed_spectrum[MAX_TIME_CHANNELS];

  int embedIPF;
  int numExtElements;
  USAC_ID_EXT_ELE  extType[USAC_MAX_EXTENSION_PAYLOADS];
  unsigned char    extElementConfigPayload[USAC_MAX_EXTENSION_PAYLOADS][USAC_MAX_EXTENSION_PAYLOAD_LEN];
  unsigned int     extElementConfigLength[USAC_MAX_EXTENSION_PAYLOADS];

  APR_DATA  aprData;
  int embedDRC;
  DRC_DATA  drcData;
};


void smulFLOAT(float a, const float * restrict X, float * restrict Z, int n)
{
  int i;
  if (n & 1) {
    Z[0] = (float) a *(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float) a * (X[i]), _b = (float) a * (X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

static int EncUsacAudioPreRoll_reset(HANDLE_APRDATA       aprData,
                                     APR_STATE            resetConfig,
                                     APR_STATE            resetAU
) {
  int err = 0;
  int i   = 0;

  aprData->audioPreRollState   = APR_IDLE;
  aprData->applyCrossfade      = 0;
  aprData->reserved            = 0;
  aprData->audioPreRollFrameID = 0;

  if (APR_PREPARE == resetConfig) {
    aprData->usacConfig.configLen = 0;
    memset(aprData->usacConfig.config_data, 0, USAC_MAX_CONFIG_LEN);
  }

  if (APR_PREPARE == resetAU) {
    for (i = 0; i < MPEG_USAC_CORE_MAX_AU; i++) {
      aprData->usacFrame[i].auLen = 0;
      memset(aprData->usacFrame[i].au_data, 0, USAC_MAX_EXTENSION_PAYLOAD_LEN);
    }
  }

  return err;
}
static int get_binary_data(char             * filename,
                           unsigned char   ** data,
                           unsigned int     * nBytes)
{
  FILE *file;
  unsigned int size;
  int err = 0;

  file = fopen(filename, "r");
  if(file != NULL){
    fseek(file, 0, SEEK_END);
    *nBytes = ftell(file);
    /*fclose(file);*/
    /* re-open file */
    /*file = fopen(filename, "r");*/
    fseek(file, 0, SEEK_SET);
    *data = (unsigned char *) malloc(*nBytes);
    size = fread(*data, sizeof(unsigned char), *nBytes, file);
    fclose(file);

    if( size != *nBytes ){
      err = 1;
    }

  } else {
    fprintf(stderr,"%s not found!\n",filename);
    err = 1;
  }

  return err;
}

static int get_ASCI_data(char    * filename,
                         int    ** data)
{
  FILE *file;
  long int size;
  int i = 0;
  int val;
  int err = 0;

  file = fopen(filename, "r");
  if(file != NULL){
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    /* re-open file */
    file = fopen(filename, "r");
    *data = (int*) malloc(size*sizeof(int));
    while(fscanf(file,"%d", &(*data)[i++]) == 1){ }
    fclose(file);
  } else {
    fprintf(stderr,"%s not found!\n",filename);
    err = 1;
  }

  return err;
}

static int EncUsacDrc_init(HANDLE_DRCDATA        drcData,
                           USAC_ID_EXT_ELE      *extType,
                           int                  *numExtElements
) {
  int err = 0;
  unsigned int tmp_length;

  /* Read in DRC info from drcToolEncoderCmdl output files */

  /* Read Loudness Info Set */
  err = get_binary_data( "tmpFileUsacEnc_drc_loudness_output.bit", &drcData->drcLoudnessInfoSet_ConfigExt, &drcData->configExt_length );

  /* Read DRC config */
  if( 0 == err ){
    err = get_binary_data( "tmpFileUsacEnc_drc_config_output.bit", &drcData->drcConfig_ExtElementConfig, &drcData->extElementConfig_length );
  }

  /* Read DRC gains */
  if( 0 == err ){
    err = get_binary_data( "tmpFileUsacEnc_drc_payload_output.bit", &drcData->drcGains_ExtElement, &tmp_length );
  }

  /* Read size of gains, in bits in ASCI format */
  if( 0 == err ){
    err = get_ASCI_data( "tmpFileUsacEnc_drc_payload_output.txt", &drcData->drcGainBits_ExtElement );
  }

  /* register DRC in the usacExtElementConfig */
  extType[*numExtElements]                = USAC_ID_EXT_ELE_UNI_DRC;
  (*numExtElements)++;

  return err;
}

static int EncUsacAudioPreRoll_init(HANDLE_APRDATA        aprData,
                                    USAC_ID_EXT_ELE      *extType,
                                    unsigned int         *extElementConfigLength,
                                    int                  *numExtElements,
                                    const int             requestedAPRframes

) {
  int err = 0;

  /* reset internal data */
  EncUsacAudioPreRoll_reset(aprData,
                            APR_PREPARE,
                            APR_PREPARE);

  aprData->audioPreRollBitBudget.bits2Distribute = 0;
  aprData->audioPreRollBitBudget.bits2Save       = 0;
  aprData->audioPreRollBitBudget.spreadFrames    = 20;    /* spread AudioPreRoll bit demand over 20 frames */
  aprData->audioPreRollExtID                     = *numExtElements;
  aprData->numPreRollFrames                      = requestedAPRframes;

  /* register AudioPreRoll in the usacExtElementConfig */
  extType[aprData->audioPreRollExtID]                = USAC_ID_EXT_ELE_AUDIOPREROLL;
  extElementConfigLength[aprData->audioPreRollExtID] = 0;
  (*numExtElements)++;

  return err;
}

static int EncUsacAudioPreRoll_prepare(HANDLE_APRDATA       aprData,
                                       const int            requestIPF,
                                       int                 *currentFrameIsIPF,
                                       int                 *bUsacIndependencyFlag,
                                       int                 *average_bits_total,
                                       USAC_ID_EXT_ELE     *extType,
                                       int                 *numExtElements
) {
  int err           = 0;
  int au_bits_total = *average_bits_total;

  /* don't write AudioPreRoll() extension any longer */
  if (APR_DELETE == aprData->audioPreRollState) {
    EncUsacAudioPreRoll_reset(aprData,
                              APR_IDLE,                 /* don't reset the config */
                              APR_PREPARE);

    /* idle APR chain */
    aprData->audioPreRollState = APR_IDLE;
  }

  /* add AudioPreRoll() to extension payload */
  if (APR_EMBED == aprData->audioPreRollState) {
    /* set the indep flag in the frame where the AudioPreRoll() is embedded */
    *bUsacIndependencyFlag = 1;
    *currentFrameIsIPF     = 1;
  } else {
    *currentFrameIsIPF     = 0;
  }

  /* this frame will be an AU for the AudioPreRoll in the next IPF frame */
  if (1 == requestIPF) {
    if (APR_IDLE == aprData->audioPreRollState) {
      EncUsacAudioPreRoll_reset(aprData,
                                APR_IDLE,                 /* don't reset the config */
                                APR_PREPARE);
    }

    /* set the indep flag in the first frame where the AU is buffered for the AudioPreRoll() */
    if (0 == aprData->audioPreRollFrameID) {
      *bUsacIndependencyFlag = 1;
    }

    /* (re-)start APR chain */
    aprData->audioPreRollState = APR_PREPARE;
  }

  /* estimate bitconsumtion of AUs to be stored in the IPF */
  if (APR_PREPARE == aprData->audioPreRollState || APR_EMBED == aprData->audioPreRollState) {
    au_bits_total       /= (aprData->numPreRollFrames + 1);
    *average_bits_total  = au_bits_total;
  }

  return err;
}

static int EncUsacAudioPreRoll_setConfig(HANDLE_ENCODER_DATA     data,
                                         HANDLE_BSBITBUFFER     *asc,
                                         DEC_CONF_DESCRIPTOR    *dec_conf
) {
  int err                       = 0;
  int i                         = 0;
  int bitCount                  = 0;
  HANDLE_BSBITSTREAM output_asc = NULL;
  USAC_CONFIG* pUsacCfg         = &(dec_conf[0].audioSpecificConfig.specConf.usacConfig);

  /* open asc bs bitstream */
  output_asc = BsOpenBufferWrite(asc[i]);
  if (output_asc == NULL) {
    CommonWarning("EncUsacAudioPreRoll_setConfig: error opening output space");
    return -1;
  }

  /* encode usac config -> usacConfig() */
  UsacConfig_Init(pUsacCfg, 1);
  bitCount = UsacConfig_Advance(output_asc, pUsacCfg, 1);

  if (0 == bitCount) {
    CommonWarning("EncUsacAudioPreRoll_setConfig: no usacConfig present");
    return -1;
  }

  /* transport config to AudioPreRoll */
  data->aprData.usacConfig.configLen = ((bitCount + 7) >> 3);

  if (data->aprData.usacConfig.configLen > sizeof(USACCONFIG_DATA)) {
    CommonWarning("EncUsacAudioPreRoll_setConfig: memory out of bounds");
    return -2;
  }
  memcpy(data->aprData.usacConfig.config_data, BsBufferGetDataBegin(asc[0]), sizeof(unsigned char) * data->aprData.usacConfig.configLen);

  /* close asc bs bitstream */
  BsClose(output_asc);

  return err;
}

static int EncUsacAudioPreRoll_setAU(HANDLE_APRDATA       aprData,
                                     const unsigned int   au_nBits,
#ifdef _DEBUG
                                     const long           au_Bits_debug,
#endif
                                     const unsigned char* au_data
) {
  int err = 0;

#ifdef _DEBUG
  /*assert(au_nBits == au_Bits_debug);*/
#endif

  if (APR_PREPARE == aprData->audioPreRollState) {
    aprData->usacFrame[aprData->audioPreRollFrameID].auLen = ((au_nBits + 7) >> 3);

    if (aprData->usacFrame[aprData->audioPreRollFrameID].auLen > USAC_MAX_EXTENSION_PAYLOAD_LEN) {
      CommonWarning("EncUsacAudioPreRoll_setAU: memory out of bounds");
      return -1;
    }

    memcpy(aprData->usacFrame[aprData->audioPreRollFrameID].au_data,
           au_data,
           sizeof(unsigned char) * aprData->usacFrame[aprData->audioPreRollFrameID].auLen);

    aprData->audioPreRollFrameID++;
    if (aprData->audioPreRollFrameID == aprData->numPreRollFrames) {
      aprData->audioPreRollState = APR_EMBED;
    }
  }

  return err;
}

static int EncUsac_writeEscapedValue(HANDLE_BSBITSTREAM   bitStream,
                                     unsigned int         value,
                                     unsigned int         nBits1,
                                     unsigned int         nBits2,
                                     unsigned int         nBits3,
                                     unsigned int         WriteFlag
) {
  int bitCount             = 0;
  unsigned long valueLeft  = value;
  unsigned long valueWrite = 0;
  unsigned long maxValue1  = (1 << nBits1) - 1;
  unsigned long maxValue2  = (1 << nBits2) - 1;
  unsigned long maxValue3  = (1 << nBits3) - 1;

  valueWrite = min(valueLeft, maxValue1);
  bitCount += BsRWBitWrapper(bitStream, &valueWrite, nBits1, WriteFlag);

  if(valueWrite == maxValue1){
    valueLeft = valueLeft - valueWrite;

    valueWrite = min(valueLeft, maxValue2);
    bitCount += BsRWBitWrapper(bitStream, &valueWrite, nBits2, WriteFlag);

    if(valueWrite == maxValue2){
      valueLeft = valueLeft - valueWrite;

      valueWrite = min(valueLeft, maxValue3);
      bitCount += BsRWBitWrapper(bitStream, &valueWrite, nBits3, WriteFlag);
#ifdef _DEBUG
      assert((valueLeft - valueWrite) == 0);
#endif
    }
  }

  return bitCount;
}

static int EncUsac_writeBits(HANDLE_BSBITSTREAM   bitStream,
                             unsigned int         data,
                             int                  numBit
) {
  BsPutBit(bitStream, data, numBit);

  return numBit;
}

static int EncUsacDrc_writeExtData(HANDLE_DRCDATA       drcData,
                                   unsigned char       *extensionData,
                                   int                 *extensionDataSize,
                                   int                 *extensionDataPresent
) {
  int err = 0;

  *extensionDataSize = ((*drcData->drcGainBits_ExtElement + 7) >> 3);

  /* extension data is present */
  if ((*extensionDataSize) > 0) {
    memcpy(extensionData, (drcData->drcGains_ExtElement), *extensionDataSize);
    *extensionDataPresent = 1;
  }

  /* shift pointer of drcGains_ExtElement */
  drcData->drcGains_ExtElement += *extensionDataSize; 

  ++drcData->drcGainBits_ExtElement;


  return err;
}

static int EncUsacAudioPreRoll_writeExtData(HANDLE_APRDATA       aprData,
                                            unsigned char       *extensionData,
                                            int                 *extensionDataSize,
                                            int                 *extensionDataPresent
) {
  int err                     = 0;
  unsigned int i              = 0;
  int frameIdx                = 0;
  int bitCount                = 0;
  const int maxNumBits        = ((USAC_MAX_EXTENSION_PAYLOAD_LEN + USAC_MAX_CONFIG_LEN) << 3) + 56;
  unsigned char *tmp          = NULL;
  HANDLE_BSBITSTREAM bs       = NULL;
  HANDLE_BSBITBUFFER bsBuffer = NULL;

  *extensionDataSize    = 0;
  *extensionDataPresent = 0;

  if (APR_EMBED == aprData->audioPreRollState) {
    if (0 != aprData->audioPreRollExtID) {
      CommonWarning("EncUsacAudioPreRoll_writeExtData: The first element of every frame shall be an extension element (mpegh3daExtElement) of type ID_EXT_ELE_AUDIOPREROLL.");
      return -1;
    }

    bsBuffer = BsAllocBuffer(maxNumBits);

    bs = BsOpenBufferWrite(bsBuffer);

    bitCount += EncUsac_writeEscapedValue(bs, aprData->usacConfig.configLen, 4, 4, 8, 1);

    for (i = 0; i < aprData->usacConfig.configLen; i++) {
      bitCount += EncUsac_writeBits(bs, aprData->usacConfig.config_data[i], 8);
    }

    bitCount += EncUsac_writeBits(bs, aprData->applyCrossfade, 1);
    bitCount += EncUsac_writeBits(bs, aprData->reserved, 1);

    bitCount += EncUsac_writeEscapedValue(bs, aprData->numPreRollFrames, 2, 4, 0, 1);

    for (frameIdx = 0; frameIdx < aprData->numPreRollFrames; frameIdx++) {
      bitCount += EncUsac_writeEscapedValue(bs, aprData->usacFrame[frameIdx].auLen, 16, 16, 0, 1);

      for (i = 0; i < aprData->usacFrame[frameIdx].auLen; i++) {
        bitCount += EncUsac_writeBits(bs, aprData->usacFrame[frameIdx].au_data[i], 8);
      }
    }

    if (bitCount % 8) {
      EncUsac_writeBits(bs, 0 , 8 - (bitCount % 8));
    }

    tmp = BsBufferGetDataBegin(bsBuffer);
    *extensionDataSize  = ((bitCount + 7) >> 3);

    /* extension data is present */
    if ((*extensionDataSize) > 0) {
      memcpy(extensionData, tmp, *extensionDataSize);
      *extensionDataPresent = 1;
    }

    BsClose(bs);
    BsFreeBuffer(bsBuffer);

    /* distribute AudioPreRoll() bits over the next frames */
    aprData->audioPreRollBitBudget.bits2Distribute += ((*extensionDataSize) << 3);
    aprData->audioPreRollBitBudget.bits2Save        = (aprData->audioPreRollBitBudget.bits2Distribute + aprData->audioPreRollBitBudget.spreadFrames - 1) / aprData->audioPreRollBitBudget.spreadFrames;

    /* clean-up APR chain */
    aprData->audioPreRollState = APR_DELETE;
  }

  return err;
}

static int EncUsacAudioPreRoll_bitDistribution(HANDLE_APRDATA       aprData,
                                               const int            extElementID
) {
  int savedBits = 0;

  if ((aprData->audioPreRollBitBudget.bits2Distribute > 0) && (aprData->audioPreRollExtID == extElementID)) {
    savedBits = aprData->audioPreRollBitBudget.bits2Save;

    if (aprData->audioPreRollBitBudget.bits2Distribute >= savedBits) {
      aprData->audioPreRollBitBudget.bits2Distribute -= savedBits;
    } else {
      savedBits = aprData->audioPreRollBitBudget.bits2Distribute;
      aprData->audioPreRollBitBudget.bits2Distribute = 0;
    }
  }

  return savedBits;
}


/********************************************************/
/* functions EncUsacInfo() and EncUsacFree()             */
/********************************************************/
#define PROGVER "Unified Speech And Audio Coding (USAC) Encoder 15-Sep-2008"

/* EncUsacInfo() */
/* Get info about usac encoder core.*/
char *EncUsacInfo (FILE *helpStream)
{
  if ( helpStream != NULL ) {
    fprintf(helpStream,
      PROGVER "\n"
      "encoder parameter string format:\n"
      "possible options:\n"
	    "\t-usac_switched   USAC Switched Coding\n"
            "\t-usac_fd  	USAC Frequency Domain Coding\n"
            "\t-usac_td  	USAC Temporal Domain Coding\n"
	    "\t-br <Br1>        bitrate(s) in kbit/s\n"
	    "\t-bw <Bw1>        bandwidths in Hz\n");
    fprintf(helpStream,
            "\t-tns, -tns_lc	for TNS or TNS with low complexity\n"
	    "\t-ms <num>	set msMask to 0 or 2, instead of autodetect (1)\n"
            "\t-wshape  	use window shape WS_DOLBY instead of WS_FHG\n");
    fprintf(helpStream,
	    "\t-aac_nosfacs	all AAC-scalefacs will be equal\n");
    fprintf(helpStream,
	    "\t-ep      	create bitstreams with epConfig 1 syntax\n");
    fprintf(helpStream,
	    "\nobscure/not working options:\n"
	    "\t-length_960  	frame length flag (never tested)\n"
	    "\t-noSbr	use sbr for BSAC object types\n"
	    "\t-wlp <blksize>\n");
    fprintf(helpStream,
	    "\t-length_768  	     frame length of 768 samples\n"
            "\t-sbrRatioIndex <int>  sbrRatioIndex\n"
            "\t-hSBR                 Harmonic SBR\n"
            "\t-nf                   Noise Filling\n"
            "\t-usac_tw              TW-MDCT\n"
            "\t-ipd                  IPD coding in MPEG Surround\n"
            "\t-mps_res              Allow MPEG Surround residual\n"
            "\t-cplx_pred            Complex prediction\n"
	  );
    fprintf(helpStream,
      "\t-fillElem    	     write a FillElement or not. Note: if no FillElement is written the buffer requirements may be violated (dflt: 1)\n"
	  );
  }

  return PROGVER;
}


/* EncUscaFree() */
/* Free memory allocated by usac encoder core.*/
void EncUsacFree (HANDLE_ENCODER enc)
{
  if(enc){

    if(enc->data){
      int i_ch;

      for ( i_ch = 0 ; i_ch <MAX_TIME_CHANNELS; i_ch++) {
        if(enc->data->tdenc[i_ch]){
          close_coder_lf(enc->data->tdenc[i_ch]);
        }
      }

      EncUsac_data_free(enc->data);

      EncUsac_tns_free(enc->data);

      for ( i_ch = 0 ; i_ch <MAX_TIME_CHANNELS; i_ch++) {
        if(enc->data->tdenc[i_ch]){
          free(enc->data->tdenc[i_ch]);
          enc->data->tdenc[i_ch] = NULL;
        }
      }

      if(enc->data->hSpatialEnc){
        SpatialEncClose(enc->data->hSpatialEnc);
        enc->data->hSpatialEnc = NULL;
      }

      if(enc->data->hTfDec){
        DeleteIntDec(&(enc->data->hTfDec));
      }

      free(enc->data);
      enc->data = NULL;
    }

  }
/* ... */
}


/**
 * Available as part of the HANDLE_ENCODER
 */
static int EncUsac_getNumChannels(HANDLE_ENCODER enc)
{ return enc->data->channels; }

static enum MP4Mode EncUsac_getEncoderMode(HANDLE_ENCODER enc)
{ enc=enc; return MODE_USAC; }

static int EncUsac_getSbrRatioIndex(HANDLE_ENCODER encoderStruct){
  return encoderStruct->data->sbrRatioIndex;
}

static int EncUsac_getBitrate(HANDLE_ENCODER encoderStruct){
  return encoderStruct->data->bitrate;
}

static int EncUsac_getSbrEnable(HANDLE_ENCODER encoderStruct, int *bitrate)
{
  int sbrenable;

  sbrenable = encoderStruct->data->sbrenable;
  *bitrate = encoderStruct->data->bitrate;

  return sbrenable;
}

static int EncUsac_getFlag768(HANDLE_ENCODER enc){
  return enc->data->flag_768;
}

/**
 * Helper functions
 */
static const char* getParam(const char* haystack, const char* needle)
{
  const char* ret = strstr(haystack, needle);
  if (ret) DebugPrintf(2, "EncUsacInit: accepted '%s'",needle);
  return ret;
}


/**
 * Initialise Encoder specific data structures:
 *   initialize window type and shape data
 */
static int EncUsac_window_init(HANDLE_ENCODER_DATA data, int wshape_flag)
{
  int i_ch;

  for (i_ch=0; i_ch<data->channels; i_ch++) {
    data->windowSequence[i_ch] = ONLY_LONG_SEQUENCE;
    if (wshape_flag == 1) {
      data->windowShapePrev[i_ch] = WS_DOLBY;
    } else {
      data->windowShapePrev[i_ch] = WS_FHG;
    }
  }

  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   allocate and initialize TNS
 */
static int EncUsac_tns_init(HANDLE_ENCODER_DATA data, TNS_COMPLEXITY tns_used)
{
  int i_ch;
  for (i_ch=0; i_ch<data->channels; i_ch++) {
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

static int EncUsac_tns_free(HANDLE_ENCODER_DATA data)
{

  if (data){
    int i_ch;

    for(i_ch = 0; i_ch < MAX_TIME_CHANNELS; i_ch++){

      if(data->tnsInfo[i_ch]){
        free(data->tnsInfo[i_ch]);
        data->tnsInfo[i_ch] = NULL;
      }
    }
  }

  return 0;
}




/**
 * Initialise Encoder specific data structures:
 *   calculate max. audio bandwidth
 */
static int EncUsac_bandwidth_init(HANDLE_ENCODER_DATA data)
{
  int i;

  if (data->bw_limit<=0) {
    /* no bandwidth is set at all, pick one from bitrate */
    /* table: audio_bandwidth( bit_rate ) */
    static const long bandwidth[8][2] =
      { {64000,20000},{56000,16000},{48000,14000},
        {40000,12000},{32000,9000},{24000,6000},
        {12000,3500},{-1,0}
      };

    i = 0;
    while( bandwidth[i][0] > (data->bitrate/data->channels)) {
      if( bandwidth[i][0] < 0 ) {
	i--;
	break;
      }
      i++;
    }
    data->bw_limit = bandwidth[i][1];
  }

  return 0;
}


/**
 * Initialise Encoder specific data structures:
 *   some global initializations and memory allocations
 */
static int EncUsac_data_init(HANDLE_ENCODER_DATA data)
{
  int i_ch;
  int frameLength = data->block_size_samples;

  for (i_ch=0; i_ch<data->channels; i_ch++) {
    data->DTimeSigBuf[i_ch] =
      (double*)calloc(2*MAX_OSF*(frameLength), sizeof(double));
    {
      int jj;
      for (jj=0; jj<2*MAX_OSF*(frameLength); jj++)
        data->DTimeSigBuf[i_ch][jj] = 0.0;
    }
    data->DTimeSigLookAheadBuf[i_ch] =
      (double*)calloc(MAX_OSF*frameLength, sizeof(double));


    data->spectral_line_vector[i_ch] =
      (double*)calloc(MAX_OSF*(frameLength+128), sizeof(double));

    data->reconstructed_spectrum[i_ch] =
        (double*)calloc(MAX_OSF*(frameLength+128), sizeof(double));

    data->twoFrame_DTimeSigBuf[i_ch] =
      (double*)calloc(MAX_OSF*3*frameLength, sizeof(double));
    {
      int ii;
      for(ii=0; ii<MAX_OSF*3*frameLength; ii++)
        data->twoFrame_DTimeSigBuf[i_ch][ii] = 0.0;
    }

    /* initialize t/f mapping */
    data->overlap_buffer[i_ch] =
      (double*)calloc(frameLength*4, sizeof(double));
  {
       int ii;
       for(ii=0; ii<4*frameLength; ii++)
         data->overlap_buffer[i_ch][ii] = 0.0;
     }
    /*initialize TD buffer */
    {
      int jj;
      for (jj=0; jj<L_FRAME_1024+L_NEXT_HIGH_RATE_1024; jj++)
        data->tdBuffer[i_ch][jj] = 0.f;

    }

    /* initialize TW buffers */
    if ( data->flag_twMdct == 1 ) {
      int ii;
      data->warp_cont_mem[i_ch] = (float*) calloc(2*frameLength,sizeof(float));
      for ( ii = 0 ; ii < 2*frameLength ; ii++ ) {
        data->warp_cont_mem[i_ch][ii] = 1.0f;
      }
      data->warp_sum[i_ch][0] = data->warp_sum[i_ch][1] = (float) frameLength;
    }

  }
  return 0;
}


static int EncUsac_data_free(HANDLE_ENCODER_DATA data)
{
  int i_ch;

  if(data){
    for (i_ch=0; i_ch<MAX_TIME_CHANNELS; i_ch++) {
      if(data->DTimeSigBuf[i_ch]){
        free(data->DTimeSigBuf[i_ch]);
        data->DTimeSigBuf[i_ch] = NULL;
      }

      if(data->DTimeSigLookAheadBuf[i_ch]){
        free(data->DTimeSigLookAheadBuf[i_ch]);
        data->DTimeSigLookAheadBuf[i_ch] = NULL;
      }

      if(data->spectral_line_vector[i_ch]){
        free(data->spectral_line_vector[i_ch]);
        data->spectral_line_vector[i_ch] = NULL;
      }

      if(data->reconstructed_spectrum[i_ch]){
        free(data->reconstructed_spectrum[i_ch]);
        data->reconstructed_spectrum[i_ch] = NULL;
      }

      if(data->twoFrame_DTimeSigBuf[i_ch]){
        free(data->twoFrame_DTimeSigBuf[i_ch]);
        data->twoFrame_DTimeSigBuf[i_ch] = NULL;
      }

      if(data->overlap_buffer[i_ch]){
        free(data->overlap_buffer[i_ch]);
        data->overlap_buffer[i_ch] = NULL;
      }


      if(data->warp_cont_mem[i_ch]){
        free(data->warp_cont_mem[i_ch]);
        data->warp_cont_mem[i_ch] = NULL;
      }

    }
  }

  return 0;
}


static int getCoreSbrFrameLengthIndex(int outFrameLength, int sbrRatioIndex){

  int index = -1;

  switch(outFrameLength){
  case 768:
    if(sbrRatioIndex == 0){
      index = 0;
    }
    break;
  case 1024:
    if(sbrRatioIndex == 0){
      index = 1;
    }
    break;
  case 2048:
    if(sbrRatioIndex == 2){
      index = 2;
    } else if (sbrRatioIndex == 3){
      index = 3;
    }
    break;
  case 4096:
    if(sbrRatioIndex == 1){
      index = 4;
    }
    break;
  default:
    break;
  }

  assert(index > -1);

  return index;
}


static void __fillUsacMps212Config(USAC_MPS212_CONFIG *pUsacMps212Config, SAC_ENC_USAC_MPS212_CONFIG *pSacEncUsacMps212Config){

  pUsacMps212Config->bsFreqRes.value              = pSacEncUsacMps212Config->bsFreqRes;
  pUsacMps212Config->bsFixedGainDMX.value         = pSacEncUsacMps212Config->bsFixedGainDMX;
  pUsacMps212Config->bsTempShapeConfig.value      = pSacEncUsacMps212Config->bsTempShapeConfig;
  pUsacMps212Config->bsDecorrConfig.value         = pSacEncUsacMps212Config->bsDecorrConfig;
  pUsacMps212Config->bsHighRateMode.value         = pSacEncUsacMps212Config->bsHighRateMode;
  pUsacMps212Config->bsPhaseCoding.value          = pSacEncUsacMps212Config->bsPhaseCoding;
  pUsacMps212Config->bsOttBandsPhasePresent.value = pSacEncUsacMps212Config->bsOttBandsPhasePresent;
  pUsacMps212Config->bsOttBandsPhase.value        = pSacEncUsacMps212Config->bsOttBandsPhase;
  pUsacMps212Config->bsResidualBands.value        = pSacEncUsacMps212Config->bsResidualBands;
  pUsacMps212Config->bsPseudoLr.value             = pSacEncUsacMps212Config->bsPseudoLr;
  pUsacMps212Config->bsEnvQuantMode.value         = pSacEncUsacMps212Config->bsEnvQuantMode;

}



/*****************************************************************************************
 ***
 *** Function: EncUsacInit
 ***
 *** Purpose:  Initialize USAC-part and the macro blocks of the USAC part of the VM
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

int EncUsacInit (
                int                 numChannel,             /* in: num audio channels */
                float               sampling_rate_f,        /* in: sampling frequancy [Hz] */
                HANDLE_ENCPARA      encPara,                /* in: encoder parameter struct */
                int                 *frameNumSample,        /* out: num samples per frame */
                int                 *frameMaxNumBit,        /* out: maximum num bits per frame */ /* SAMSUNG_2005-09-30 : added */
                DEC_CONF_DESCRIPTOR *dec_conf,              /* in: space to write decoder configurations */
                int                 *numTrack,              /* out: number of tracks */
                HANDLE_BSBITBUFFER  *asc,                   /* buffers to hold output Audio Specific Config */
                int                 *numAPRframes,          /* number of AudioPre-Roll frames in an IPF */
#ifdef I2R_LOSSLESS
                int                 type_PCM,
                int                 _osf,
#endif
                HANDLE_ENCODER      core,                   /* in:  core encoder or NULL */
                HANDLE_ENCODER      encoderStruct           /* in: space to put encoder data */
) {
  int err = 0;
  int wshape_flag = 0;
  int   i, i_dec_conf;
  int i_ch;
  int outputFrameLength;
  unsigned int elemIdx = 0;
  int samplingRateCore = (int)(sampling_rate_f+.5);
  unsigned int bs_pvc;
  unsigned int confExtIdx;
  HANDLE_ENCODER_DATA data = (HANDLE_ENCODER_DATA)calloc(1, sizeof(ENCODER_DATA));
  if (data == NULL) {
    CommonWarning("EncUsacInit: Allocation Error");
    return -1;
  }

  encoderStruct->data = data;
  encoderStruct->getNumChannels   = EncUsac_getNumChannels;
  encoderStruct->getEncoderMode   = EncUsac_getEncoderMode;
  encoderStruct->getSbrEnable     = EncUsac_getSbrEnable;
  encoderStruct->getFlag768       = EncUsac_getFlag768;
  encoderStruct->getSbrRatioIndex = EncUsac_getSbrRatioIndex;
  encoderStruct->getBitrate       = EncUsac_getBitrate;

  data->debugLevel=0;
  data->sampling_rate = (int)(sampling_rate_f+.5);
  data->sampling_rate_out = data->sampling_rate;
  data->tns_select = TNS_USAC;
  data->ms_select = 1;
  data->flag_960 = 0;
  data->flag_768 = 0;
  data->ep_config = -1;
  data->track_num = -1;
  data->layer_num = 1;
  data->max_sfb = 0;
  data->usacIndependencyFlagCnt = 0;
  data->usacIndependencyFlagInterval = 25; /* send usacIndependencyFlag every 25th frame */

  {
    int i,k;
    data->msInfo.ms_mask = 0;
    for (i=0; i<MAX_SHORT_WINDOWS; i++) {
      for (k=0; k<SFB_NUM_MAX; k++) {
        data->msInfo.ms_used[i][k] = 0;
      }
    }
  }

  /* Core coding selection*/
  data->frameCounter       = 0;
  data->switchEveryNFrames = 50;

  /* ---- transfer encoder options ---- */
  if( ! encPara ){
    CommonWarning("EncUsacInit: invalid encoder parameter struct");
    return -1;
  } else {

    data->debugLevel        = encPara->debugLevel;
    data->streamID_present  = encPara->streamID_present;
    data->streamID          = encPara->streamID;
    data->bitrate           = encPara->bitrate;
    data->bw_limit          = encPara->bw_limit;
    data->bUseFillElement   = encPara->bUseFillElement;
    data->embedIPF          = encPara->embedIPF;
    data->embedDRC          = encPara->enableDrcEnc;
    memcpy( data->prev_coreMode, encPara->prev_coreMode, sizeof(encPara->prev_coreMode) );
    memcpy( data->coreMode, encPara->coreMode, sizeof(encPara->coreMode) );
    data->codecMode         = encPara->codecMode;
    data->sbrenable         = encPara->sbrenable;
    data->sbrRatioIndex     = encPara->sbrRatioIndex;
    data->flag_960          = encPara->flag_960;
    data->flag_768          = encPara->flag_768;
    data->flag_twMdct       = encPara->flag_twMdct;
    data->tns_select        = encPara->tns_select;
    data->flag_harmonicBE   = encPara->flag_harmonicBE;
    data->flag_noiseFilling = encPara->flag_noiseFilling;
    data->ms_select         = encPara->ms_select;
    data->ep_config         = encPara->ep_config;
    data->aacAllowScalefacs = encPara->aacAllowScalefacs;
    memcpy( data->tsdInputFileName, encPara->tsdInputFileName, sizeof(encPara->tsdInputFileName) );
    data->flag_ipd          = encPara->flag_ipd;
    data->flag_mpsres       = encPara->flag_mpsres;
    data->flag_cplxPred     = encPara->flag_cplxPred;
    wshape_flag             = encPara->wshape_flag;
  }

  *numAPRframes = 0;
  if (0 != data->embedIPF) {
    /* determine the number of required APR frames */
    *numAPRframes = 1;

    if (1 == data->sbrenable) {
      (*numAPRframes)++;

      if (1 == data->flag_harmonicBE) {
        (*numAPRframes)++;
      }
    }
    /**numAPRframes = 1;*/

    /* initialize AudioPreRoll() */
    EncUsacAudioPreRoll_init(&data->aprData,
                             data->extType,
                             data->extElementConfigLength,
                             &data->numExtElements,
                             *numAPRframes);
  } else {
    EncUsacAudioPreRoll_reset(&data->aprData,
                              APR_PREPARE,
                              APR_PREPARE);
  }

  /* initialize DRC */
  if (1 == data->embedDRC) {
    if( EncUsacDrc_init(&data->drcData, data->extType, &data->numExtElements) != 0 ){
      CommonWarning("EncUsacInit: Error initializing DRC");
      return -1;
    }
  }

  if (numChannel == 2 && data->bitrate < 64000){
    data->channels = 1;
    data->usac212enable = 1;
    data->flag_mpsres = 0;
  }else if(numChannel == 2 && data->flag_mpsres && data->bitrate == 64000){
    data->channels = 2;
    data->usac212enable = 1;
  }else{
    data->channels = numChannel;
    data->usac212enable = 0;
    data->flag_mpsres = 0;
  }

  if (data->flag_cplxPred) {
    data->ms_select = 3;
  }

  /* init PVC dependent on number channels in SBR */
  bs_pvc = (data->channels == 1) ? 1 : 0; /* PVC not supported in CPEs */

  /* ---- Sanity check ---- */
  if (core != NULL) {
    CommonWarning("EncUsacInit: USAC doesn't support scalabity");
    return -1;
  }
  if (data->channels < 1) {
    CommonWarning("EncUsacInit: Channel number smaller than 1");
    return -1;
  }else if(data->channels > 2) {
    CommonWarning("EncUsacInit: Channel number bigger than 2");
    return -1;
  }
  if (data->layer_num!=1) {
    CommonWarning("EncUsacInit: Scalability not supported in USAC");
    return -1;
  }

  /* ======================= */
  /* Process Encoder Options */
  /* ======================= */
  if (data->flag_960) {
    data->block_size_samples = 960;
  } else if(data->flag_768){
    data->block_size_samples = 768;
  } else {
    data->block_size_samples = 1024;
  }


  /* ---- decide number of tracks for AAC ---- */
  {
    int tracks = 0;
    int t=0;
    if (data->ep_config==-1) {
      t=1;
    } else if (data->ep_config==1) {
      if (data->channels == 1) {
        t = 3;
      } else if (data->channels == 2) {
        t = 7;
      } else {
        CommonWarning("for USAC only mono or stereo configurations are supported");
        return -1;
      }
    } else {
      CommonWarning("only ep1 or non error resilient syntax are supported for AAC");
      return -1;
    }

    data->tracks_per_layer[0] = t;
    tracks += t;

    data->track_num = tracks;
  }
  if (data->track_num==-1) {
    CommonWarning("EncUsacInit: setup of track_count failed");
    return -1;
  }


  /*--- TW-MDCT ----*/
  if (data->flag_twMdct == 1 ) {
    tw_init_enc();
  }


  *frameNumSample = data->block_size_samples;
  *numTrack       = data->track_num;

  /* determine outputFrameLength */
  switch(data->sbrRatioIndex){
  case 0: /* no sbr */
    if(data->block_size_samples == 768){
      outputFrameLength = 768;
    } else {
      outputFrameLength = 1024;
    }
    break;
  case 1:
    outputFrameLength = 4096;
    break;
  case 2:
  case 3:
    outputFrameLength = 2048;
    break;
  default:
    assert(0);
    break;
  }


  /* ================================ */
  /* Write Header Information         */
  /* ================================ */
  for (i_dec_conf=0; i_dec_conf<data->track_num; i_dec_conf++) {
    int sx;
    int frameLenIdx = 0, sbrRatioIdx = 0;
    USAC_CONFIG *pUsacConfig = &(dec_conf[i_dec_conf].audioSpecificConfig.specConf.usacConfig);
    USAC_ELEMENT_TYPE   elemType = (numChannel == 1) ? USAC_ELEMENT_TYPE_SCE : USAC_ELEMENT_TYPE_CPE;
    USAC_CORE_CONFIG   *pUsacCoreConfig = NULL;
    USAC_SBR_CONFIG    *pUsacSbrConfig  = NULL;
    USAC_MPS212_CONFIG *pUsacMps212Config = NULL;

    /* ----- common part ----- */
    dec_conf[i_dec_conf].avgBitrate.value = 0;
    dec_conf[i_dec_conf].maxBitrate.value = 0;
    dec_conf[i_dec_conf].avgBitrate.value = data->bitrate;
    dec_conf[i_dec_conf].bufferSizeDB.value = 6144*data->channels /8;
    dec_conf[i_dec_conf].audioSpecificConfig.samplingFrequency.value = data->sampling_rate;
    for (sx=0;sx<0xf;sx++) {
      if (dec_conf[i_dec_conf].audioSpecificConfig.samplingFrequency.value == samplFreqIndex[sx]) break;
    }
    dec_conf[i_dec_conf].audioSpecificConfig.samplingFreqencyIndex.value = sx;
    dec_conf[i_dec_conf].audioSpecificConfig.channelConfiguration.value = data->channels;
    dec_conf[i_dec_conf].audioSpecificConfig.audioDecoderType.value = USAC;
    dec_conf[i_dec_conf].audioSpecificConfig.epConfig.value = 0;

    /* ----- USAC specific part ----- */
    pUsacConfig->usacSamplingFrequency.value = dec_conf[i_dec_conf].audioSpecificConfig.samplingFrequency.value;
    for(sx=0; sx<0x1f; sx++){
      if(pUsacConfig->usacSamplingFrequency.value == UsacSamplingFrequencyTable[sx]) break;
    }

    if (data->embedIPF == 1) {
      USAC_EXT_CONFIG * pFillElem = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacExtConfig);
      pUsacConfig->usacDecoderConfig.usacElementType[elemIdx] = USAC_ELEMENT_TYPE_EXT;

      pFillElem->usacExtElementType=USAC_ID_EXT_ELE_AUDIOPREROLL;
      pFillElem->usacExtElementConfigLength=0;
      pFillElem->usacExtElementDefaultLengthPresent.length=1;
      pFillElem->usacExtElementDefaultLengthPresent.value=0;
      pFillElem->usacExtElementPayloadFrag.length=1;
      pFillElem->usacExtElementPayloadFrag.value=0;

      pUsacConfig->usacDecoderConfig.numElements++;
      elemIdx++;
    }

    if (data->embedDRC == 1 ) {
      USAC_EXT_CONFIG * pDrcElem = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacExtConfig);
      pUsacConfig->usacDecoderConfig.usacElementType[elemIdx] = USAC_ELEMENT_TYPE_EXT;

      pDrcElem->usacExtElementType = USAC_ID_EXT_ELE_UNI_DRC;
      pDrcElem->usacExtElementConfigLength = data->drcData.extElementConfig_length; 
      memcpy(pDrcElem->usacExtElementConfigPayload, data->drcData.drcConfig_ExtElementConfig, data->drcData.extElementConfig_length);
      pDrcElem->usacExtElementDefaultLengthPresent.length = 1;
      pDrcElem->usacExtElementDefaultLengthPresent.value = 0;
      pDrcElem->usacExtElementPayloadFrag.length = 1;
      pDrcElem->usacExtElementPayloadFrag.value = 0;

      pUsacConfig->usacDecoderConfig.numElements++;
      elemIdx++;
    }

    pUsacConfig->usacSamplingFrequencyIndex.value           = sx;
    pUsacConfig->coreSbrFrameLengthIndex.value              = getCoreSbrFrameLengthIndex(outputFrameLength, data->sbrRatioIndex);
    pUsacConfig->channelConfigurationIndex.value            = numChannel;
    pUsacConfig->usacDecoderConfig.numElements             += 1;
    pUsacConfig->usacDecoderConfig.usacElementType[elemIdx] = elemType;

    pUsacCoreConfig = UsacConfig_GetUsacCoreConfig(pUsacConfig, elemIdx);
    pUsacSbrConfig  = UsacConfig_GetUsacSbrConfig(pUsacConfig, elemIdx);
    pUsacMps212Config = UsacConfig_GetUsacMps212Config(pUsacConfig, elemIdx);


    pUsacCoreConfig->tw_mdct.value      = data->flag_twMdct;
    pUsacCoreConfig->noiseFilling.value = data->flag_noiseFilling;
    pUsacSbrConfig->harmonicSBR.value   = data->flag_harmonicBE;
    pUsacSbrConfig->bs_interTes.value   = 1;
    pUsacSbrConfig->bs_pvc.value        = bs_pvc;

    assert(pUsacConfig->channelConfigurationIndex.value != 0); /* no support for arbitrary channel configs in encoder for now */

    /* Fill Element should be the last Element */
    if (data->bUseFillElement) {
      USAC_EXT_CONFIG * pFillElem = &(pUsacConfig->usacDecoderConfig.usacElementConfig[pUsacConfig->usacDecoderConfig.numElements].usacExtConfig);
      pUsacConfig->usacDecoderConfig.usacElementType[pUsacConfig->usacDecoderConfig.numElements] = USAC_ELEMENT_TYPE_EXT;

      pFillElem->usacExtElementType=USAC_ID_EXT_ELE_FILL;
      pFillElem->usacExtElementConfigLength=0;
      pFillElem->usacExtElementDefaultLengthPresent.length=1;
      pFillElem->usacExtElementDefaultLengthPresent.value=0;
      pFillElem->usacExtElementPayloadFrag.length=1;
      pFillElem->usacExtElementPayloadFrag.value=0;

      pUsacConfig->usacDecoderConfig.numElements++;
      /* do not increase elemIdx, this is used afterwards */
    }


    if (data->streamID_present == 1) {
      confExtIdx = pUsacConfig->usacConfigExtension.numConfigExtensions;
      pUsacConfig->usacConfigExtension.numConfigExtensions += 1;
      pUsacConfig->usacConfigExtension.usacConfigExtLength[confExtIdx] = 2;
      pUsacConfig->usacConfigExtension.usacConfigExtType[confExtIdx] = USAC_CONFIG_EXT_STREAM_ID;

      pUsacConfig->usacConfigExtension.usacConfigExt[confExtIdx][0] = (unsigned char) (data->streamID >> 8) & 0xFF;
      pUsacConfig->usacConfigExtension.usacConfigExt[confExtIdx][1] = (unsigned char) data->streamID & 0xFF;

      pUsacConfig->usacConfigExtensionPresent.value = 1;
    }

    if (data->embedDRC == 1) {
      confExtIdx = pUsacConfig->usacConfigExtension.numConfigExtensions;
      pUsacConfig->usacConfigExtension.numConfigExtensions += 1;
      pUsacConfig->usacConfigExtension.usacConfigExtLength[confExtIdx] = data->drcData.configExt_length;
      pUsacConfig->usacConfigExtension.usacConfigExtType[confExtIdx] = USAC_CONFIG_EXT_LOUDNESS_INFO;
      memcpy(pUsacConfig->usacConfigExtension.usacConfigExt[confExtIdx],data->drcData.drcLoudnessInfoSet_ConfigExt,data->drcData.configExt_length);

      pUsacConfig->usacConfigExtensionPresent.value = 1;
    }
  }



  /* ================================ */
  /* Init coding tools                */
  /* ================================ */

  /* Initialization Classification */
  if ((data->codecMode == USAC_SWITCHED)&&(data->bitrate < 64000)) {
    init_classification(&classfyData, data->bitrate, 1);
  }


  /* Initilization SBR */
  if (data->sbrRatioIndex > 0) {
    USAC_SBR_CONFIG * pUsacSbrConfig = UsacConfig_GetUsacSbrConfig(&dec_conf[0].audioSpecificConfig.specConf.usacConfig, elemIdx);
    USAC_SBR_HEADER * pUsacSbrDfltHeader = &pUsacSbrConfig->sbrDfltHeader;
    if(data->debugLevel>0){
      fprintf(stderr, "EncUsacInit: SBR enable\n");
    }
    
    switch(data->sbrRatioIndex){
      case 1:
        samplingRateCore = data->sampling_rate = data->sampling_rate/4;
        break;
      case 2:
        data->sampling_rate = 3*data->sampling_rate/8;
        
        samplingRateCore = (int)((4.f/3.f) * data->sampling_rate);
        break;
      case 3:
        samplingRateCore = data->sampling_rate = data->sampling_rate/2;
        break;
      default:
        assert(0);
        break;
    }

    /* Init SBR */
    data->ch_elm_total = data->channels;
    for(i=0; i<data->ch_elm_total; i++){
      struct channelElementData *chData  = &data->chElmData[i];
      sbrChan[chData->startChIdx].bs_pvc = bs_pvc;
      chData->startChIdx = 0;
      chData->endChIdx = 1;
      chData->chNum =  data->ch_elm_total;
      SbrInit(data->bitrate, data->sampling_rate_out, chData->chNum, (SBR_RATIO_INDEX)data->sbrRatioIndex, &sbrChan[chData->startChIdx]);
      chData->sbrChan[chData->startChIdx] = &sbrChan[chData->startChIdx];
      if(chData->chNum==2) {
        sbrChan[chData->endChIdx].bs_pvc = bs_pvc;
        SbrInit(data->bitrate, data->sampling_rate_out, chData->chNum, (SBR_RATIO_INDEX)data->sbrRatioIndex, &sbrChan[chData->endChIdx]);
        chData->sbrChan[chData->endChIdx] = &sbrChan[chData->endChIdx];
      }
      pUsacSbrDfltHeader->header_extra1.value = 0;
      pUsacSbrDfltHeader->header_extra2.value = 0;
      pUsacSbrDfltHeader->start_freq.value = sbrChan[chData->startChIdx].startFreq;
      pUsacSbrDfltHeader->stop_freq.value = sbrChan[chData->startChIdx].stopFreq;
    }
  }
  else {
    for(i=0; i<data->ch_elm_total; i++){
      struct channelElementData *chData  = &data->chElmData[i];
      chData->sbrChan[0] = chData->sbrChan[1] = NULL;
    }
  }


  /* Initilization MPEGS */
  if(data->usac212enable){
    int bufferSize = 0;
    int treeConfig;
    int timeSlots = (data->sbrRatioIndex == 1)?64:32;
    Stream bitstream;

    /* init variables for downsampling, upsampling and delay compensation*/
    int encDelaySamples    = 0;
    int encDelayFrames     = 0;
    int delayBufferSamples = 0;
    int downSampleRatio    = 1;
    int upSampleRatio      = 1;
    int NcoefRS            = 0;
    int NcoefUS            = 0;
    float lowPassFreq = 0.0f;
    float fSample     = (float)data->sampling_rate;
    float Fc          = 0.0f;
    float dF          = 0.0f;
    float alpha       = 0.0f;
    float D           = 0.0f;

    USAC_CONFIG *pUsacConfig = &(dec_conf[0].audioSpecificConfig.specConf.usacConfig); 
    USAC_CPE_CONFIG *pUsacCpeConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig);
    USAC_MPS212_CONFIG *pUsacMps212Config = UsacConfig_GetUsacMps212Config(pUsacConfig, elemIdx);
    SAC_ENC_USAC_MPS212_CONFIG sacEncUsacMps212Config;

    if(data->debugLevel>0){
      fprintf(stderr, "EncUsacInit: USAC212 enable\n");
    }
    InitStream(&bitstream, NULL, STREAM_WRITE);

    if(data->bitrate >= 16000 && data->bitrate < 24000){
      treeConfig = 2122;
    }else{
      treeConfig = 2121;
    }

    pUsacCpeConfig->stereoConfigIndex.value = (data->channels == 2)?3:1;

    /* open MPS212 encoder */
    data->hSpatialEnc = SpatialEncOpen(treeConfig, timeSlots, data->sampling_rate_out, &bufferSize, &bitstream, data->flag_ipd, data->flag_mpsres, data->tsdInputFileName);

    /* get configuration for UsacConfig() */
    SpatialEncGetUsacMps212Config(data->hSpatialEnc, &sacEncUsacMps212Config);

    /* copy to struct */
    __fillUsacMps212Config(pUsacMps212Config, &sacEncUsacMps212Config);

  /* init delay compensation */
  if(data->sbrenable && (!data->sbrRatioIndex) ){
    downSampleRatio = 2;
  } else {
    switch(data->sbrRatioIndex){
    case 0:
      downSampleRatio = 1;
      break;
    case 1:
      downSampleRatio = 4;
      break;
    case 2:
      downSampleRatio = 8;
      upSampleRatio   = 3;
      break;
    case 3:
      downSampleRatio = 2;
      break;
    default:
      assert(0);
      break;
    }
  }

  if (data->sbrenable || (data->sbrRatioIndex > 0)) {
    fSample = (upSampleRatio*fSample)/downSampleRatio;

    lowPassFreq = 5000.0f;
    if (numChannel ==1 ) {
      if (data->bitrate==24000 && fSample==22050)
        lowPassFreq = 5513.0f; /* startFreq 4 @ 22.05kHz */
      if (data->bitrate==24000 && fSample==24000)
        lowPassFreq = 5625.0f; /* startFreq 4 @ 24kHz */
    }else if (numChannel == 2) {
      if (data->bitrate == 48000 && fSample == 22050)
        lowPassFreq = 7924.0f; /* startFreq 9 @ 22.05kHz */
      if (data->bitrate == 48000 && fSample == 24000)
        lowPassFreq = 8250.0f; /* startFreq 9 @ 24kHz */
    }
    lowPassFreq = fSample / 2.0f;

    /* init resampler and buffers for downsampling by 2 */
    Fc = lowPassFreq / (fSample * downSampleRatio);
    dF = 0.1f * Fc; /* 10% transition bw */
    alpha = 7.865f; /* 80 dB attenuation */
    D = 5.015f; /* transition width ? */

    NcoefRS = (int)ceil(D/dF) + 1;

    Fc = 1.047198f; /* = pi/upsampleRatio = pi/3 */
    dF = 0.1f * Fc;
    NcoefUS = (int)ceil(D/dF) + 1;
    NcoefUS = ((NcoefUS + upSampleRatio - 1) / upSampleRatio) * upSampleRatio;
  }

  EncUsac_getUsacEncoderDelay(encoderStruct,
                              &encDelaySamples,
                              &encDelayFrames,
                              downSampleRatio,
                              upSampleRatio,
                             (upSampleRatio > 1)   ? NcoefUS : 1,
                             (downSampleRatio > 1) ? NcoefRS : 1,
                              data->usac212enable,
                              &delayBufferSamples);

  SpaceEncInitDelayCompensation(data->hSpatialEnc, encDelaySamples, *numAPRframes);

  } else {
    USAC_CONFIG *pUsacConfig = &(dec_conf[0].audioSpecificConfig.specConf.usacConfig); 
    USAC_CPE_CONFIG *pUsacCpeConfig = &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig);
    pUsacCpeConfig->stereoConfigIndex.value = 0;
  }

  /* Initilization core coding */
  if (EncUsac_window_init(data, wshape_flag) != 0) return -1;
  if (EncUsac_bandwidth_init(data) != 0) return -1;
  if (EncUsac_data_init(data) != 0) return -1;
  /* bit reservoir*/
  data->max_bitreservoir_bits = 6144*data->channels;
  data->available_bitreservoir_bits = data->max_bitreservoir_bits;
  {
    int average_bits_total = (data->bitrate*data->block_size_samples) / data->sampling_rate;
    data->available_bitreservoir_bits -= average_bits_total;
  }
  *frameMaxNumBit = data->max_bitreservoir_bits;

  /* Initialization Fd coding  */
  if((data->codecMode==USAC_SWITCHED) || (data->codecMode==USAC_ONLY_FD)){
    if(data->debugLevel>0){
      fprintf(stderr, "EncUsacInit: FD coding enable\n");
    }
  }

  /* ---- initialize TNS ---- */
  if (EncUsac_tns_init(data, data->tns_select) != 0) return -1;

  /* initialize psychoacoustic module (does nothing at the moment) */
  EncTf_psycho_acoustic_init();

  /* initialize FD Qc and noiseless coding*/
  for (i = 0; i < MAX_TIME_CHANNELS; i++)
    usac_init_quantize_spectrum(&(data->quantInfo[i]), data->debugLevel );

  /* initialize window switching  */
  data->flag_winSwitchFD = 0;
  if (data->flag_winSwitchFD) {
    data->win_sw_module = ntt_win_sw_init(data->channels, data->block_size_samples);
  }


  /* Initialization Td coding  */
  switch (data->codecMode) {
  case USAC_SWITCHED:
  case USAC_ONLY_TD:

    if(data->debugLevel>0){
      fprintf(stderr, "EncUsacInit: TD coding enable\n");
    }

    for (i_ch=0; i_ch<data->channels; i_ch++) {

      data->tdenc[i_ch] = (USAC_TD_ENCODER *)calloc(1, sizeof(*data->tdenc[i_ch]));

      if ( (samplingRateCore) < SR_MIN || (samplingRateCore) > SR_MAX ) {
        CommonWarning("EncUsacInit():Sampling frequency outside of range");
        return -1;
      } else {

        data->tdenc[i_ch]->fscale = (int) ( ((float)FSCALE_DENOM * (samplingRateCore) / 12800.0F) + 0.5F );
        if (data->tdenc[i_ch]->fscale > FAC_FSCALE_MAX){
          data->tdenc[i_ch]->fscale = FAC_FSCALE_MAX;
        }
        if (data->tdenc[i_ch]->fscale < FAC_FSCALE_MIN){
          data->tdenc[i_ch]->fscale = FAC_FSCALE_MIN;
        }

        init_coder_lf(data->tdenc[i_ch], data->block_size_samples, data->tdenc[i_ch]->fscale);
      }

      data->td_bitrate[i_ch] = data->bitrate;
      if(data->sbrenable)
        data->td_bitrate[i_ch] -= 1500;
      if(data->usac212enable)
        data->td_bitrate[i_ch] -= 1500;
      data->td_bitrate[i_ch] /= data->channels;
      config_coder_lf(data->tdenc[i_ch], samplingRateCore, data->td_bitrate[i_ch]);
      if((data->tdenc[i_ch])->mode<0){
        CommonWarning("EncUsacInit: no td coding mode found for the user defined parameters" );
        return -1;
      }
      data->acelp_core_mode[i_ch] = (data->tdenc[i_ch])->mode;
      if(data->debugLevel>0){
        fprintf(stderr, "EncUsacInit: acelp core mode=%d\n", (data->tdenc[i_ch])->mode);
      }
    }
    break;
  default:
    data->acelp_core_mode[0] = 0;
    break;
  }

  /* transport ASC to AudioPreRoll */
  err = EncUsacAudioPreRoll_setConfig(data,
                                      asc,
                                      dec_conf);
  if (0 != err) {
    return err;
  }

  /*initialise internal decoder module */
  data->hTfDec = NULL;
  CreateIntDec(&data->hTfDec, data->channels, data->sampling_rate, data->block_size_samples);

  return 0;
}

static int __getIndependencyFlag(HANDLE_ENCODER_DATA data, int offset) {

  int bUsacIndependencyFlag = 0;

  if(data){
    int tmpCnt = (data->usacIndependencyFlagCnt + offset)%data->usacIndependencyFlagInterval;
    if(tmpCnt == 0) bUsacIndependencyFlag = 1;
  }

  return bUsacIndependencyFlag;
}

int EncUsac_getIndependencyFlag(HANDLE_ENCODER enc, int offset){

  HANDLE_ENCODER_DATA data = enc->data;

  return __getIndependencyFlag(data, offset);
}

static int EncUsac_writeUsacExtElement(HANDLE_BSBITSTREAM     output_au,
                                       const int             *extensionDataPresent,
                                       const int             *extensionDataSize,
                                       const unsigned char    extensionData[USAC_MAX_EXTENSION_PAYLOADS][USAC_MAX_EXTENSION_PAYLOAD_LEN],
                                       const int              numExtElements
) {
  int bitCount = 0;
  int i        = 0;
  int j        = 0;

  for (i = 0; i < numExtElements; ++i) {
    int usacExtElementUseDefaultLength = 0;
    int usacExtElementPresent          = extensionDataPresent[i];
    int usacExtElementPayloadLength    = extensionDataSize[i];

    if (1 == usacExtElementPresent && 0 == usacExtElementPayloadLength) {
      CommonExit(-1, "EncUsac_writeUsacExtElement: invalid extension payload!");
    }

    BsPutBit(output_au, usacExtElementPresent, 1);
    bitCount += 1;

    if (1 == usacExtElementPresent) {
      BsPutBit(output_au, usacExtElementUseDefaultLength, 1);
      bitCount += 1;

      if (usacExtElementUseDefaultLength) {
        CommonExit(-1, "EncUsac_writeUsacExtElement: usacExtElementUseDefaultLength shall be 0!");
      } else {
        if (usacExtElementPayloadLength >= 255) {
          int valueAdd = usacExtElementPayloadLength - 255 + 2;
          BsPutBit(output_au, 255, 8);
          BsPutBit(output_au, valueAdd, 16);
          bitCount += 24;
        } else {
          BsPutBit(output_au, usacExtElementPayloadLength, 8);
          bitCount += 8;
        }
      }

      for (j = 0; j < usacExtElementPayloadLength; ++j) {
        BsPutBit(output_au, extensionData[i][j], 8);
        bitCount += 8;
      }
    }
  }

  return bitCount;
}


/*****************************************************************************************
 ***
 *** Function:    EncUsacFrame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on USAC encoding
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
int EncUsacFrame(const ENCODER_DATA_TYPE  input,
                 HANDLE_BSBITBUFFER      *au,             /* buffers to hold output AccessUnits */
                 const int                requestIPF,     /* indicate that an IPF is requested within the next frames */
                 USAC_SYNCFRAME_TYPE     *syncFrameType,  /* indicates the USAC sync frame state */
                 HANDLE_ENCODER           enc,
                 float                  **p_time_signal_orig
) {
  int err = 0;
  HANDLE_ENCODER_DATA data = enc->data;

  unsigned char extensionData[USAC_MAX_EXTENSION_PAYLOADS][USAC_MAX_EXTENSION_PAYLOAD_LEN] = {{ 0 }};
  int extensionDataSize[USAC_MAX_EXTENSION_PAYLOADS] = { 0 };
  int extensionDataPresent[USAC_MAX_EXTENSION_PAYLOADS] = { 0 };

  /*---FD---*/
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE next_windowSequence[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE newWindowSequence;

  const double *p_ratio[MAX_TIME_CHANNELS];
  double allowed_distortion[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  double p_energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    nr_of_sfb[MAX_TIME_CHANNELS];
  int    max_sfb[MAX_TIME_CHANNELS];
  int    sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];

  int i_ch, i, k;

  int core_channels = 0;
  int core_max_sfb = 0;
  int isFirstTfLayer = 1;
  int osf = 1;

  /* structures holding the output of the psychoacoustic model */
  CH_PSYCH_OUTPUT chpo_long[MAX_TIME_CHANNELS];
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS];

  /* AAC window grouping information */
  int num_window_groups[MAX_TIME_CHANNELS] = {0};
  int window_group_length[MAX_TIME_CHANNELS][8];

  HANDLE_BSBITSTREAM* output_au;
  HANDLE_AACBITMUX aacmux;

  int lfe_chIdx = -1; /* SAMSUNG_2005-09-30 : LFE channel index */

  /*---Bitmux---*/
  int average_bits_total;
  int used_bits = 0;
  int sbr_bits = 0;
  int num_bits_available;
  int min_bits_needed;
  int bitCount;
  int padding_bits;
  int common_win;
  int nb_bits_fac[MAX_TIME_CHANNELS];
  int bUsacIpfFlag          = 0;
  int bUsacIndependencyFlag = __getIndependencyFlag(data, -data->delay_encoder_frames);
  int AudioPreRollBits = 0;
  int DrcBits = 0;
  int mod[MAX_TIME_CHANNELS][4] = {{0}}; /* LPD modes per frame */
  int tns_data_present[2];
  int lFrame;
  int lLpc0;
  int lNextHighRate;
#ifdef SONY_PVC_ENC
  static int pvc_mode = 0;
#endif
  int bFacDataPresentFirst = 0;

  lFrame = data->block_size_samples;
  lLpc0 = (L_LPC0_1024 * lFrame)/L_FRAME_1024;
  lNextHighRate = (L_NEXT_HIGH_RATE_1024*lFrame)/L_FRAME_1024;

  /*-----------------------------------------------------------------------*/
  /* Initialization  */
  /*-----------------------------------------------------------------------*/
  if (data->debugLevel > 1) {
    fprintf(stderr,"EncUsacFrame entered\n");
  }

  /* pre-initialize the average number of bits available */
  average_bits_total = (data->bitrate * data->block_size_samples) / data->sampling_rate;

  /* prepare AudioPreRoll() */
  err = EncUsacAudioPreRoll_prepare(&data->aprData,
                                    requestIPF,
                                    &bUsacIpfFlag,
                                    &bUsacIndependencyFlag,
                                    &average_bits_total,
                                    data->extType,
                                    &data->numExtElements);
  if (0 != err) {
    return err;
  }

  /* Read extension data and adjust data rate */
  for (i = 0; i < data->numExtElements; ++i) {
    switch (data->extType[i]) {
      case USAC_ID_EXT_ELE_AUDIOPREROLL:
        err |= EncUsacAudioPreRoll_writeExtData(&data->aprData, extensionData[i], &extensionDataSize[i], &extensionDataPresent[i]);
        AudioPreRollBits = EncUsacAudioPreRoll_bitDistribution(&data->aprData, i);
        break;
      case USAC_ID_EXT_ELE_UNI_DRC:
        err |= EncUsacDrc_writeExtData(&data->drcData, extensionData[i], &extensionDataSize[i], &extensionDataPresent[i]);
        break;
      default:
        break;
    }

    if (1 == extensionDataPresent[i]) {
      if (USAC_ID_EXT_ELE_AUDIOPREROLL == data->extType[i]) {
        used_bits += AudioPreRollBits;
        average_bits_total -= AudioPreRollBits;
      } else {
        used_bits += extensionDataSize[i] * 8;
      }

      used_bits += 10;                        /* 1 (usacExtElementPresent) + 1 (usacExtElementUseDefaultLength) + 8 (usacExtElementPayloadLength) */

      if (extensionDataSize[i] >= 255) {
        used_bits += 16;                      /* 16 (valueAdd) */
      }
    } else {
      /* no extension data present for this frame, but registered in usacExtElementConfig() */
      used_bits += 1;                         /* 1 (usacExtElementPresent) */
    }
  }
  if (0 != err) {
    return err;
  }

  /* set the USAC sync frame state */
  if (1 == bUsacIpfFlag) {
    *syncFrameType = USAC_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME;
  } else if (1 == bUsacIndependencyFlag) {
    *syncFrameType = USAC_SYNCFRAME_INDEPENDENT_FRAME;
  } else {
    *syncFrameType = USAC_SYNCFRAME_NOSYNC;
  }

  output_au = (HANDLE_BSBITSTREAM*)calloc(1, data->track_num*sizeof(HANDLE_BSBITSTREAM));
  for (i = 0; i < data->track_num; i++) {
    output_au[i] = BsOpenBufferWrite(au[i]);
    if (output_au[i] == NULL) {
      CommonWarning("EncUsacFrame: error opening output space");
      return -1;
    }
  }

  for (i = 0; i < data->layer_num; i++) {
    int t  = 0;
    aacmux = NULL;
    aacmux = aacBitMux_create(&(output_au[t]),
                              data->tracks_per_layer[t],
                              data->channels,
                              data->ep_config,
                              data->debugLevel);
    t += data->tracks_per_layer[t];
  }

  /* bit budget adjustment */
  num_bits_available = (long)(average_bits_total + MAX(data->available_bitreservoir_bits-0.25*data->max_bitreservoir_bits,-0.1*average_bits_total));
  min_bits_needed    = (long)(data->available_bitreservoir_bits + 2*average_bits_total - data->max_bitreservoir_bits);
  /* 2*average_bits_total, because one full frame always has to stay in buffer and one other frame is processed each frame */
  if (min_bits_needed < 0) {
    min_bits_needed = 0;
  }

  /* Print out info*/
  if (data->debugLevel > 1) {
    fprintf(stderr, "EncUsacFrame: init done\n");
    fprintf(stderr, "UsacFrame [%d]\n", data->frameCounter);
  }

#ifdef SONY_PVC_ENC
  /* Selection of the core coding*/
  if (isFirstTfLayer) {
    if (data->debugLevel > 1) {
      fprintf(stderr,"EncUsacFrame: select core coding\n");
    }
    for ( i_ch = 0 ; i_ch < data->channels ; i_ch++ ) {
      switch( data->codecMode ) {
      case USAC_SWITCHED:
        if (classfyData.coding_mode == 2) {
          data->next_coreMode[i_ch]=CORE_MODE_FD;
        }
        else {
          data->next_coreMode[i_ch]=CORE_MODE_TD;
        }
        if (data->next_coreMode[i_ch] == CORE_MODE_TD) {
          pvc_mode = 2;
        } else {
          pvc_mode = 0;
        }

        /*           if((data->frameCounter%data->switchEveryNFrames)==0) */
        /*           { */
        /*              if(data->coreMode[i_ch]==CORE_MODE_FD) */
        /*                data->next_coreMode[i_ch] = CORE_MODE_TD; */
        /*             else */
        /*               data->next_coreMode[i_ch] = CORE_MODE_FD; */
        /*           } */
        /*           else */
        /*             data->next_coreMode[i_ch] = data->coreMode[i_ch]; */
        break;
      case USAC_ONLY_FD:
        data->next_coreMode[i_ch] = CORE_MODE_FD;
        pvc_mode = 0;
        break;
      case USAC_ONLY_TD:
        data->next_coreMode[i_ch] = CORE_MODE_TD;
        pvc_mode = 2;
        break;
      default:
        CommonWarning("EncUsacFrame: unsupported codec mode %d", data->codecMode);
        return(-1);
      }
    }
  }
#endif


  /***********************************************************************/
  /* SBR encoding */
  /***********************************************************************/
  if (p_time_signal_orig != NULL) {
    for (i_ch=0;i_ch<data->ch_elm_total;i_ch++) {
      if (data->chElmData[i_ch].chNum == 1) {
        /* single_channel_element */
        sbr_bits += SbrEncodeSingleChannel_BSAC(data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].startChIdx],
                                                data->flag_harmonicBE ? SBR_USAC_HARM : SBR_USAC,
                                                p_time_signal_orig[data->chElmData[i_ch].startChIdx],
#ifdef	PARAMETRICSTEREO
                                                NULL,  /* PS disable */
#endif
#ifdef SONY_PVC_ENC
                                                &pvc_mode,
#endif
                                                bUsacIndependencyFlag);
      } else {
        if (data->chElmData[i_ch].chNum == 2) {
          /* Write out cpe */
          sbr_bits +=
            SbrEncodeChannelPair_BSAC(data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].startChIdx],     /* Left */
                                      data->chElmData[i_ch].sbrChan[data->chElmData[i_ch].endChIdx], /* Right */
                                      data->flag_harmonicBE ? SBR_USAC_HARM : SBR_USAC,
                                      p_time_signal_orig[data->chElmData[i_ch].startChIdx],                       /* Left */
                                      p_time_signal_orig[data->chElmData[i_ch].endChIdx], /* Right */
                                      bUsacIndependencyFlag);
          i_ch++;
        }
      } /* if (chann...*/
    } /* for (chanNum...*/
  }


  /***********************************************************************/
  /* Copy input buffer                                                   */
  /***********************************************************************/
  /* convert float input to double, which is the internal format */
  /* store input data in look ahead buffer which may be necessary for the window switching decision and AAC pre processing */

  if (data->debugLevel>1)
    fprintf(stderr,"EncUsacFrame: copy time buffers\n");

  for (i_ch=0; i_ch<data->channels; i_ch++){
    if (data->flag_twMdct == 1) {
      /* temporary fix: a linear buffer for LTP containing the whole time frame */
      for( i=0; i<osf*data->block_size_samples/2; i++ ) {
        data->twoFrame_DTimeSigBuf[i_ch][i] =
          data->twoFrame_DTimeSigBuf[i_ch][osf*data->block_size_samples + i];
        data->twoFrame_DTimeSigBuf[i_ch][osf*data->block_size_samples/2+i] =
          data->DTimeSigBuf[i_ch][i];
        data->twoFrame_DTimeSigBuf[i_ch][osf*data->block_size_samples+i] =
          data->DTimeSigBuf[i_ch][osf*data->block_size_samples/2+i];
        data->twoFrame_DTimeSigBuf[i_ch][3*osf*data->block_size_samples/2+i] =
          data->DTimeSigLookAheadBuf[i_ch][osf*data->block_size_samples/2+i];
      }
      /* last frame input data are encoded now */
      for( i=0; i<osf*data->block_size_samples/2; i++ ) {
        data->DTimeSigBuf[i_ch][i]=data->DTimeSigLookAheadBuf[i_ch][osf*data->block_size_samples/2+i];
        data->DTimeSigBuf[i_ch][osf*data->block_size_samples/2+i] = (double) input[i_ch][i];
      }
      /* new data are stored here for use in window switching decision
           and AAC pre processing */
      for( i=0; i<osf*data->block_size_samples; i++ ) {
        data->DTimeSigLookAheadBuf[i_ch][i] = (double)input[i_ch][i];
      }

    }
    else {
      for( i=0; i<osf*data->block_size_samples; i++ ) {
        /* temporary fix: a linear buffer for LTP containing the whole time frame */
        data->twoFrame_DTimeSigBuf[i_ch][i] = data->DTimeSigBuf[i_ch][i];
        data->twoFrame_DTimeSigBuf[i_ch][data->block_size_samples + i] = data->DTimeSigLookAheadBuf[i_ch][i];
        /* last frame input data are encoded now */
        data->DTimeSigBuf[i_ch][i]          = data->DTimeSigLookAheadBuf[i_ch][i];
        /* new data are stored here for use in window switching decision
	 and AAC pre processing */
        data->DTimeSigLookAheadBuf[i_ch][i] = (double)input[i_ch][i];

      }
    }

    /* Copy TD coding input samples*/
    for (i=0; i<data->block_size_samples+lNextHighRate; i++){
      data->tdBuffer[i_ch][i] = (float)(data->twoFrame_DTimeSigBuf[i_ch][i + data->block_size_samples/2]);
    }
    for (i=0; i<data->block_size_samples+lNextHighRate+lLpc0; i++){
      data->tdBuffer_past[i_ch][i] = (float)(data->twoFrame_DTimeSigBuf[i_ch][i + (data->block_size_samples/2) - lLpc0]);
    }
  }

  /***********************************************************************/
  /* Pre-processing                                                      */
  /***********************************************************************/
#ifndef SONY_PVC_ENC
  /* Selection of the core coding*/
  if (isFirstTfLayer) {
    if (data->debugLevel>1)
      fprintf(stderr,"EncUsacFrame: select core coding\n");
    for ( i_ch = 0 ; i_ch < data->channels ; i_ch++ ) {
      switch( data->codecMode ) {
      case USAC_SWITCHED:
        if (classfyData.coding_mode == 2) {
          data->next_coreMode[i_ch]=CORE_MODE_FD;
        }
        else {
          data->next_coreMode[i_ch]=CORE_MODE_TD;
        }

        /*           if((data->frameCounter%data->switchEveryNFrames)==0) */
        /*           { */
        /*              if(data->coreMode[i_ch]==CORE_MODE_FD) */
        /*                data->next_coreMode[i_ch] = CORE_MODE_TD; */
        /*             else */
        /*               data->next_coreMode[i_ch] = CORE_MODE_FD; */
        /*           } */
        /*           else */
        /*             data->next_coreMode[i_ch] = data->coreMode[i_ch]; */
        break;
      case USAC_ONLY_FD:
        data->next_coreMode[i_ch] = CORE_MODE_FD;
        break;
      case USAC_ONLY_TD:
        data->next_coreMode[i_ch] = CORE_MODE_TD;
        break;
      default:
        CommonWarning("EncUsacFrame: unsupported codec mode %d", data->codecMode);
        return(-1);
      }
    }
  }
#endif


  for ( i_ch = 0 ; i_ch < data->channels ; i_ch++ ) {
    data->window_size_samples[i_ch]=data->block_size_samples;
    windowSequence[i_ch] =  data->windowSequence[i_ch];
  }


  /* Selection of the window sequence*/
  if (data->debugLevel>1)
    fprintf(stderr,"EncUsacFrame: select window sequence and shape\n");

  if(data->flag_winSwitchFD){
    newWindowSequence =
        ntt_win_sw(data->DTimeSigLookAheadBuf,
                   data->channels,
                   data->sampling_rate,
                   data->block_size_samples,
                   data->windowSequence[0],
                   data->win_sw_module);
  }
  else{
    newWindowSequence =  ONLY_LONG_SEQUENCE;
  }

  for ( i_ch = 0 ; i_ch < data->channels ; i_ch++ ) {
    next_windowSequence[i_ch] = newWindowSequence;

    if (data->next_coreMode[i_ch]==CORE_MODE_TD ) {
      next_windowSequence[i_ch] = EIGHT_SHORT_SEQUENCE;
    }

    if (data->coreMode[i_ch]==CORE_MODE_TD && data->next_coreMode[i_ch] != CORE_MODE_TD) {
      next_windowSequence[i_ch] = LONG_STOP_SEQUENCE;
    }

    /* Adjust to allowed Window Sequence */
    if(next_windowSequence[i_ch] == EIGHT_SHORT_SEQUENCE) {
      if(windowSequence[i_ch] == ONLY_LONG_SEQUENCE){
        windowSequence[i_ch] = LONG_START_SEQUENCE;
      }
      if(windowSequence[i_ch] == LONG_STOP_SEQUENCE) {
        windowSequence[i_ch] = STOP_START_SEQUENCE;
      }
    }

    if(next_windowSequence[i_ch] == ONLY_LONG_SEQUENCE) {
      if(windowSequence[i_ch] == EIGHT_SHORT_SEQUENCE){
        next_windowSequence[i_ch] = LONG_STOP_SEQUENCE;
      }
    }

    if (data->debugLevel>1)
      fprintf(stderr,"EncUsacFrame: Blocktype   %d \n",windowSequence[0] );

    /* Perform window shape adaptation depending on the selected mode */
    windowShape[i_ch] = data->windowShapePrev[i_ch];

    if (data->debugLevel>1)
      fprintf(stderr,"EncUsacFrame: windowshape %d \n",windowShape[i_ch] );
  }

  used_bits += 1; /* usac independency flag */
  used_bits += sbr_bits;
  if(data->usac212enable){
    used_bits += data->SpatialDataLength;
  }

  for ( i_ch = 0 ; i_ch < data->channels ; i_ch++ ) {
    /***********************************************************************/
    /* Core-coding                                                         */
    /***********************************************************************/
    if(data->coreMode[i_ch]==CORE_MODE_TD){
      if (data->debugLevel>1)
        fprintf(stderr,"EncUsacFrame: TD coding\n");

      if(data->prev_coreMode[i_ch]==CORE_MODE_FD){
        reset_coder_lf(data->tdenc[i_ch]);

        coder_amrwb_plus_first(&data->tdBuffer_past[i_ch][0],
                               lNextHighRate+lLpc0,
                               data->tdenc[i_ch]
        );

      }

     coder_amrwb_plus_mono(&data->tdBuffer[i_ch][lNextHighRate],
                           data->tdOutStream[i_ch],
                           data->facOutStream[i_ch],
                           &data->TD_nbbits_fac[i_ch],
                           data->tdenc[i_ch],
                           data->tdenc[i_ch]->fscale,
                           (data->prev_coreMode[i_ch]==CORE_MODE_FD),
                           &data->total_nbbits[i_ch],
                           mod[i_ch],
                           bUsacIndependencyFlag
                           );

     /* count bits */
     used_bits += data->total_nbbits[i_ch];

     /* write FAC data */
     if( (data->prev_coreMode[i_ch]==CORE_MODE_FD) && (mod[i_ch][0] == 0) ){
       for(i=0; i<(data->TD_nbbits_fac[i_ch]+7)/8; i++){
         data->TDfacData[i_ch][i] = (unsigned char) (
                                                     (data->facOutStream[i_ch][8*i+0] & 0x1) << 7 |
                                                     (data->facOutStream[i_ch][8*i+1] & 0x1) << 6 |
                                                     (data->facOutStream[i_ch][8*i+2] & 0x1) << 5 |
                                                     (data->facOutStream[i_ch][8*i+3] & 0x1) << 4 |
                                                     (data->facOutStream[i_ch][8*i+4] & 0x1) << 3 |
                                                     (data->facOutStream[i_ch][8*i+5] & 0x1) << 2 |
                                                     (data->facOutStream[i_ch][8*i+6] & 0x1) << 1 |
                                                     (data->facOutStream[i_ch][8*i+7] & 0x1) << 0);
       }
       used_bits += data->TD_nbbits_fac[i_ch];
     }

     used_bits += LEN_ACELP_CORE_MODE_IDX;
     if ( data->flag_twMdct == 1 ) {
       for(i=0;i<osf*data->block_size_samples;i++){
         data->overlap_buffer[i_ch][i]=data->overlap_buffer[i_ch][osf*data->block_size_samples+i];
       }
       for(i=0;i<osf*data->block_size_samples;i++){
         data->overlap_buffer[i_ch][osf*data->block_size_samples+i]=data->DTimeSigBuf[i_ch][i];
       }
     }
     else {
       for(i=0;i<osf*data->block_size_samples;i++){
         data->overlap_buffer[i_ch][i]=data->DTimeSigBuf[i_ch][i];
       }
     }
    }
    else{
      float sample_pos[MAX_TIME_CHANNELS][1024*3];
      float tw_trans_len[MAX_TIME_CHANNELS][2];
      int   tw_start_stop[MAX_TIME_CHANNELS][2];

      if (data->debugLevel>1)
        fprintf(stderr,"EncUsacFrame: FD coding\n");


      /*--- time - warping ---*/

      if ( data->flag_twMdct ) {

        if (data->prev_coreMode[i_ch]==CORE_MODE_TD) {
          tw_reset(data->block_size_samples,
                   data->warp_cont_mem[i_ch],
                   data->warp_sum[i_ch]);
        }

        EncUsac_TwEncode (data->DTimeSigBuf[i_ch],
                          (data->prev_coreMode[i_ch]==CORE_MODE_TD),
                          windowSequence[i_ch],
                          &data->toolsInfo[i_ch],
                          data->warp_cont_mem[i_ch],
                          data->warp_sum[i_ch],
                          sample_pos[i_ch],
                          tw_trans_len[i_ch],
                          tw_start_stop[i_ch],
                          data->block_size_samples);
      }

      buffer2fd(data->DTimeSigBuf[i_ch],
                data->spectral_line_vector[i_ch],
                data->overlap_buffer[i_ch],
                windowSequence[i_ch],
                windowShape[i_ch],
                data->windowShapePrev[i_ch],
                data->block_size_samples,
                data->block_size_samples/NSHORT,
                OVERLAPPED_MODE,
                (data->prev_coreMode[i_ch]==CORE_MODE_TD),
                (data->next_coreMode[i_ch]==CORE_MODE_TD),
                NSHORT,
                data->flag_twMdct,
                sample_pos[i_ch],
                tw_trans_len[i_ch],
                tw_start_stop[i_ch]);


      /*---  psychoacoustic ---*/
      if (data->debugLevel>1)
        fprintf(stderr,"EncUsacFrame: psychoacoustic initialisation\n");

      EncTf_psycho_acoustic( ((data->block_size_samples == 768) ? ((double)data->sampling_rate * 4 / 3) : (double)data->sampling_rate),
                             data->channels,
                             data->block_size_samples,
                             data->window_size_samples[i_ch],
                             chpo_long, chpo_short );


      switch( windowSequence[i_ch] ) {
        case ONLY_LONG_SEQUENCE :
          memcpy( (char*)sfb_width_table[i_ch], (char*)chpo_long[i_ch].cb_width, (NSFB_LONG+1)*sizeof(int) );
          nr_of_sfb[i_ch] = chpo_long[i_ch].no_of_cb;
          p_ratio[i_ch]   = chpo_long[i_ch].p_ratio;
          break;
        case LONG_START_SEQUENCE :
        case STOP_START_SEQUENCE :
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


      /* Set the AAC grouping information.  */
      if (windowSequence[i_ch] == EIGHT_SHORT_SEQUENCE) {
        num_window_groups[i_ch]=8;
        window_group_length[i_ch][0] = 1;
        window_group_length[i_ch][1] = 1;
        window_group_length[i_ch][2] = 1;
        window_group_length[i_ch][3] = 1;
        window_group_length[i_ch][4] = 1;
        window_group_length[i_ch][5] = 1;
        window_group_length[i_ch][6] = 1;
        window_group_length[i_ch][7] = 1;
      } else {
        num_window_groups[i_ch] = 1;
        window_group_length[i_ch][0] = 1;
      }

      /* Calculate sfb-offset table. Needed by (at least) TNS */
      sfb_offset[i_ch][0] = 0;
      k=0;
      for(i = 0; i < nr_of_sfb[i_ch]; i++ ) {
        sfb_offset[i_ch][i] = k;
        k += sfb_width_table[i_ch][i];
      }
      sfb_offset[i_ch][i] = k;

      /* Limit the bandwidth and calculate the max sfb*/
      if (data->debugLevel>3)
        fprintf(stderr,"EncUsacFrame: apply bandwidth limitation\n");
      if (data->debugLevel>5)
        fprintf(stderr,"            bandwidth limit %i Hz\n", data->bw_limit);

      usac_bandwidth_limit_spectrum(data->spectral_line_vector[i_ch],
                                    data->spectral_line_vector[i_ch],
                                    windowSequence[i_ch],
                                    data->window_size_samples[i_ch],
                                    data->sampling_rate,
                                    data->bw_limit,
                                    sfb_offset[i_ch],
                                    nr_of_sfb[i_ch],
                                    &(max_sfb[i_ch]));

      /*--- TNS ---*/
      if (data->tns_select!=NO_TNS) {
        if (data->debugLevel>1)
          fprintf(stderr,"EncUsacFrame: TNS\n");


        if(i_ch != lfe_chIdx) /* SAMSUNG_2005-09-30 */
          TnsEncode(nr_of_sfb[i_ch],                /* Number of bands per window */
                    max_sfb[i_ch],                /* max_sfb */
                    windowSequence[i_ch],           /* block type */
                    sfb_offset[i_ch],               /* Scalefactor band offset table */
                    data->spectral_line_vector[i_ch], /* Spectral data array */
                    data->tnsInfo[i_ch]);           /* TNS info */

      }

      {

        int group, index, j, sfb;
        double dtmp;

        if (data->debugLevel>1)
          fprintf(stderr,"EncUsacFrame: compute allowed distorsion\n");

        data->max_sfb = nr_of_sfb[MONO_CHAN];

        index = 0;
        /* calculate the scale factor band energy in window groups  */
        for (group = 0; group < num_window_groups[i_ch]; group++) {
          for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++)
            p_energy[i_ch][sfb + group * nr_of_sfb[i_ch]] = 0.0;
          for (i = 0; i < window_group_length[i_ch][group]; i++) {
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
        for (group = 0; group < num_window_groups[i_ch]; group++) {
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
  } /* end of first channel loop */

  /* common window */
  if ( (data->channels == 2) &&
       (windowShape[0] == windowShape[1]) &&
       (windowSequence[0] == windowSequence[1]) &&
       (data->coreMode[0] == data->coreMode[1]) &&
       (max_sfb[0] == max_sfb[1]) ) {
    common_win = 1;
  }
  else {
    common_win = 0;
  }


  /*--- M/S selection ---*/
  if (data->channels == 2 &&
      data->coreMode[0] == CORE_MODE_FD && data->coreMode[1] == CORE_MODE_FD &&
      common_win == 1) {
    select_ms(data->spectral_line_vector,
              data->block_size_samples,
              num_window_groups[0],
              window_group_length[0],
              sfb_offset[0],
              (core_channels==2)?core_max_sfb:0,
              nr_of_sfb[0],
              &data->msInfo,
              data->ms_select,
              data->debugLevel);
  }



  used_bits += usac_fd_encode(data->twoFrame_DTimeSigBuf,
                              data->spectral_line_vector,
                              data->reconstructed_spectrum,
                              data->tdenc,
                              data->hTfDec,
                              p_energy,
                              allowed_distortion,
                              sfb_width_table,
                              sfb_offset,
                              max_sfb,
                              nr_of_sfb,
                              num_bits_available-used_bits,
                              min_bits_needed-used_bits,
                              aacmux,
                              (data->ep_config!=-1),
                              0/*ld_mode*/,
                              windowSequence,
                              windowShape,
                              data->windowShapePrev,
                              data->aacAllowScalefacs,
                              data->window_size_samples,
                              data->channels,
                              data->sampling_rate,
                              &data->msInfo,
                              data->predCoef,
                              data->tnsInfo,
                              data->icsInfo,
                              data->toolsInfo,
                              data->quantInfo,
                              num_window_groups,
                              window_group_length,
                              data->bw_limit,
                              data->flag_noiseFilling,
                              data->flag_twMdct,
                              common_win,
                              data->facOutStream,
                              nb_bits_fac,
                              data->coreMode,
                              data->prev_coreMode,
                              data->next_coreMode,
                              bUsacIndependencyFlag,
                              data->debugLevel);

  for (i = 0; i < data->channels; i++) {
    tns_data_present[i] = data->tnsInfo[i] != NULL;

    if (tns_data_present[i]) {
      tns_data_present[i] = data->tnsInfo[i]->tnsDataPresent;
    }
  }

  /* count channel element bits */
  switch (data->channels) {
    case 1:
      used_bits += usac_write_sce(NULL, data->coreMode[0],
                                  tns_data_present[0]);
      break;
    case 2:
      used_bits += usac_write_cpe(NULL,
                                  data->coreMode,
                                  tns_data_present,
                                  data->predCoef,
                                  data->quantInfo[0].huffTable,
                                  common_win,
                                  data->toolsInfo[0].common_tw,
                                  data->icsInfo[0].max_sfb,
                                  data->icsInfo[0].windowSequence,
                                  data->icsInfo[0].window_shape,
                                  num_window_groups[0],
                                  window_group_length[0],
                                  data->msInfo.ms_mask,
                                  data->msInfo.ms_used,
                                  data->flag_twMdct,
                                  &(data->toolsInfo[0]),
                                  bUsacIndependencyFlag);

      break;
    default:
      CommonWarning("Only mono or stereo supported at the moment");
      return -1;
  }




  /***********************************************************************/
  /* Write AUs                                                           */
  /***********************************************************************/
  
  bitCount = 0;

  /* usac independency flag */
  BsPutBit(output_au[0], bUsacIndependencyFlag, 1);
  bitCount++;

  /* Extension data - usacExtElement */
  bitCount += EncUsac_writeUsacExtElement(output_au[0],
                                          extensionDataPresent,
                                          extensionDataSize,
                                          extensionData,
                                          data->numExtElements);

  /*Header*/
  switch (data->channels) {
    case 1:
      aacBitMux_setAssignmentScheme(aacmux, data->channels, 0);
      aacBitMux_setCurrentChannel(aacmux, 0);
      bitCount+=usac_write_sce(aacmux, data->coreMode[0],
                               tns_data_present[0]);
      break;
    case 2:
      aacBitMux_setAssignmentScheme(aacmux, data->channels, 1);
      aacBitMux_setCurrentChannel(aacmux, 0);
      bitCount+=usac_write_cpe(aacmux,
                               data->coreMode,
                               tns_data_present,
                               data->predCoef,
                               data->quantInfo[0].huffTable,
                               common_win,
                               data->toolsInfo[0].common_tw,
                               max_sfb[0],
                               windowSequence[0],
                               windowShape[0],
                               num_window_groups[0],
                               window_group_length[0],
                               data->msInfo.ms_mask,
                               data->msInfo.ms_used,
                               data->flag_twMdct,
                               &data->toolsInfo[0],
                               bUsacIndependencyFlag);
      break;
    default:
      CommonWarning("Only mono or stereo supported at the moment");
      return -1;
  }

  /*Channel stream*/
  for (i_ch=0; i_ch<data->channels; i_ch++){
    if(data->coreMode[i_ch]==CORE_MODE_TD){
      HANDLE_BSBITSTREAM bs_ACELP_CORE_MODE_INDEX;
      aacBitMux_setCurrentChannel(aacmux, 0);
      bs_ACELP_CORE_MODE_INDEX = aacBitMux_getBitstream(aacmux, TD_CORE_MODE_IDX);

      if((data->tdenc[i_ch])->mode > 7) CommonExit(-1,"\n invalid acelp_core_mode : %d", (data->tdenc[i_ch])->mode);
      BsPutBit(bs_ACELP_CORE_MODE_INDEX,(data->tdenc[i_ch])->mode,LEN_ACELP_CORE_MODE_IDX);
      bitCount += LEN_ACELP_CORE_MODE_IDX;

      for(i=0; i<data->total_nbbits[i_ch]; i++){
        BsPutBit(output_au[0], data->tdOutStream[i_ch][i], 1);
        bitCount++;
      }
      if( (data->prev_coreMode[i_ch] == CORE_MODE_FD) && (mod[i_ch][0] == 0) ){
        for(i=0; i<data->TD_nbbits_fac[i_ch]; i++){
          BsPutBit(output_au[0], data->facOutStream[i_ch][i], 1);
          bitCount++;
        }
      }
    }
    else{
      int used_bits_fd_cs = 0;
      aacBitMux_setCurrentChannel(aacmux, i_ch);
      used_bits_fd_cs += usac_fd_cs(aacmux,
                                    windowSequence[i_ch],
                                    windowShape[i_ch],
                                    data->quantInfo[i_ch].scale_factor[0],
                                    data->quantInfo[i_ch].huffTable,
                                    max_sfb[i_ch],
                                    nr_of_sfb[i_ch],
                                    num_window_groups[i_ch],
                                    window_group_length[i_ch],
                                    NULL, /*No PNS*/
                                    &(data->icsInfo[i_ch]),
                                    &(data->toolsInfo[i_ch]),
                                    data->tnsInfo[i_ch],
                                    &(data->quantInfo[i_ch]),
                                    data->icsInfo[i_ch].common_window,
                                    data->toolsInfo[i_ch].common_tw,
                                    data->flag_twMdct,
                                    data->flag_noiseFilling,
                                    data->facOutStream[i_ch],
                                    nb_bits_fac[i_ch],
                                    bUsacIndependencyFlag,
                                    data->debugLevel);
      bitCount += used_bits_fd_cs;
    }
  }


  /* write SBR and USAC212 data */
  if(data->sbrenable)
  {
    int used_bits_sbr=0;
    if (data->channels == 1) {
      used_bits_sbr = WriteSbrSCE(data->chElmData[0].sbrChan[data->chElmData[0].startChIdx],
                                  data->flag_harmonicBE ? SBR_USAC_HARM : SBR_USAC,
                                  output_au[0],
#ifdef SONY_PVC_ENC
                                  pvc_mode,
#endif
                                  bUsacIndependencyFlag);
    }
    else if (data->channels == 2) {
      used_bits_sbr = WriteSbrCPE(data->chElmData[0].sbrChan[data->chElmData[0].startChIdx],
                                  data->chElmData[0].sbrChan[data->chElmData[0].endChIdx],
                                  data->flag_harmonicBE ? SBR_USAC_HARM : SBR_USAC,
                                  output_au[0],
                                  bUsacIndependencyFlag);

    }

    bitCount += used_bits_sbr;
  }

  if(data->usac212enable){
    int used_bits_usac212=0;

    for(i=0;i<(int)(data->SpatialDataLength)-7;i+=8){
      BsPutBit(output_au[0], data->SpatialData[i/8], 8);
      used_bits_usac212+=8;
    }

    /* take care of remaining bits... */
    BsPutBit(output_au[0], data->SpatialData[i/8]>>(8-(data->SpatialDataLength%8)), data->SpatialDataLength%8);
    used_bits_usac212+=data->SpatialDataLength%8;

    bitCount += used_bits_usac212;
  }

  if (data->bUseFillElement) {
    int used_bits_fillElem = 0;
    padding_bits           = min_bits_needed - bitCount;
    used_bits_fillElem     = usac_write_fillElem(output_au[0], padding_bits);
    bitCount              += used_bits_fillElem;
    used_bits             += used_bits_fillElem;
  }

  /* store current AU as AudioPreRoll-AU for the next frame */
  err = EncUsacAudioPreRoll_setAU(&data->aprData,
                                  bitCount,
#ifdef _DEBUG
                                  BsBufferNumBit(au[0]),
#endif
                                  BsBufferGetDataBegin(au[0]));
  if (0 != err) {
    return err;
  }

  /* correct the number of calculated bits if an AudioPreRoll is written in the current frame */
  for (i = 0; i < data->numExtElements; ++i) {
    if (1 == extensionDataPresent[i]) {
      if (USAC_ID_EXT_ELE_AUDIOPREROLL == data->extType[i]) {
        used_bits = used_bits - AudioPreRollBits + extensionDataSize[i] * 8;
      }
    }
  }

  /*Update bit reservoir*/
  if (data->debugLevel>2) {
    fprintf(stderr,"\nEncUsacFrame: bitbuffer: having %i bit reservoir, produced: %i bits, consumed %i bits\n",
            data->available_bitreservoir_bits,
            bitCount,
            average_bits_total);
  }
  data->available_bitreservoir_bits -= bitCount;

  if (bitCount % 8) {
    data->available_bitreservoir_bits -= 8 - (bitCount % 8);
  }
  data->available_bitreservoir_bits += average_bits_total;

  if (used_bits != bitCount) {
    fprintf(stderr, "Warning: the number of calculated and written bits are not equal!\n");
  }

  if (data->available_bitreservoir_bits > data->max_bitreservoir_bits) {
    fprintf(stderr,"WARNING: bit reservoir got too few bits\n");
    data->available_bitreservoir_bits = data->max_bitreservoir_bits;
  }

  if (data->available_bitreservoir_bits < 0) {
    fprintf(stderr,"WARNING: no bits in bit reservoir remaining: %d\n", data->available_bitreservoir_bits);
  }

  if (data->debugLevel>2) {
    fprintf(stderr,"EncUsacFrame: bitbuffer: => now %i bit reservoir\n", data->available_bitreservoir_bits);
  }

  if (data->debugLevel>2)
    fprintf(stdout,"EncUsacFrame: %d bits written\n", bitCount);

  /***********************************************************************/
  /* End of frame                                                        */
  /***********************************************************************/
  if (data->debugLevel>1)
    fprintf(stderr,"EncUsacFrame: prepare next frame\n");

  /* save the actual window shape for next frame */
  for(i_ch=0; i_ch<data->channels; i_ch++) {
    data->windowShapePrev[i_ch] = windowShape[i_ch];
    data->windowSequence[i_ch] = next_windowSequence[i_ch];
  }

  /* save the core coding mode */
  for(i_ch=0; i_ch<data->channels; i_ch++) {
    data->prev_coreMode[i_ch] = data->coreMode[i_ch];
    data->coreMode[i_ch] = data->next_coreMode[i_ch];
  }

  /* update usac independency flag */
  data->usacIndependencyFlagCnt = (data->usacIndependencyFlagCnt + 1) % data->usacIndependencyFlagInterval;

  data->frameCounter++;
  for (i=0; i<data->track_num; i++) {
    BsClose(output_au[i]);
  }
  free(output_au);

  if (aacmux!=NULL)
    aacBitMux_free(aacmux);


  if (data->debugLevel>1)
    fprintf(stderr,"EncUsacFrame: successful return\n");

  return 0;
}

int EncUsac_getUsacDelay(HANDLE_ENCODER encoderStruct) {
  int addlDelay = 0;

  if (encoderStruct->data->flag_twMdct) {
    addlDelay = 1024;
  }

  return (addlDelay);
}

/* 
   We require the first sample of the decoded output to correspond to the first sample of our input signal.
   This will ensure the original file length is maintained.
   We need to pad zeros to our input signal in order to align the signal on the decoder side after we cut away the pre roll AU's. 
   We do this by shifting the input signal so the first sample of the input signal is the first sample of the IPF AU.
   TO do this we need to know the total codec delay and then we can increase it to an integer multiple of the output frame size.   
*/
int EncUsac_getUsacEncoderDelay(HANDLE_ENCODER  encoderStruct,
                                int            *encDelaySamples,
                                int            *encDelayFrames,
                                const int       downSampleRatio,
                                const int       upSampleRatio,
                                const int       nUpsampleTaps, 
                                const int       nDownsampleTaps,
                                const int       usac212enable,
                                int            *delayBufferSamples
) {
  int error                  = 0;
  const int sbr41            = (encoderStruct->data->sbrRatioIndex == 1) ? 1 : 0;       /* use SBR 4:1 */
  const int QmfEncDelay      = (sbr41 == 1)         ? 580  : 578;                       /* QMF encoder delay */
  const int SbrDecDelay      = (sbr41 == 1)         ? 768  : 384;                       /* SBR decoder delay */
  const int outputFrameSize  = (sbr41 == 1)         ? 4096 : 2048;                      /* number of pcm samples returned for each AU in the decoder = ccfl * resampleFactor */
  const int MpsEncDelay      = (usac212enable == 1) ? 961  : 0;                         /* MPS212 QMF delay on the encoder side */
  const int hSBR             = (encoderStruct->data->flag_harmonicBE == 1) ? 1 : 0;     /* HBE adds one output frame of delay (on the decoder) */
  const int ccfl             = encoderStruct->data->block_size_samples;                 /* core coder frame length */
  const float resampleFactor = (float)downSampleRatio / (float)upSampleRatio;           /* SBR resampling factor */

  /* preset return values */
  *encDelaySamples    = 0;
  *delayBufferSamples = 0;
  *encDelayFrames     = 0;

  /* calculate codec delay */
  *encDelaySamples = (int)((DELAY_ENC_FRAMES + hSBR) * ccfl * resampleFactor) + (int)(((nUpsampleTaps - 1) / 2.0f + (nDownsampleTaps - 1) / 2.0f) / upSampleRatio) + QmfEncDelay + SbrDecDelay + MpsEncDelay;

  /* set encoder delay in frames */
  *encDelayFrames = (int)ceil((float)(*encDelaySamples) / (float)outputFrameSize);
   encoderStruct->data->delay_encoder_frames = *encDelayFrames;

  /* How many sample do we need to add to the input buffer to align the first sample of our signal with the first sample returned by the decoder. */
  *delayBufferSamples  = (int)((*encDelayFrames - ((float)(*encDelaySamples) / (float)outputFrameSize)) * outputFrameSize);

  return error;
}

int EncUsac_getusac212enable(HANDLE_ENCODER encoderStruct)
{
  return(encoderStruct->data->usac212enable);
}

HANDLE_SPATIAL_ENC EncUsac_getSpatialEnc(HANDLE_ENCODER encoderStruct)
{
  return(encoderStruct->data->hSpatialEnc);
}

void EncUsac_setSpatialData(HANDLE_ENCODER encoderStruct, unsigned char *databuf, unsigned long size)
{
  unsigned long int i;
  unsigned int j, currentSize;
  HANDLE_SPATIAL_ENC enc = encoderStruct->data->hSpatialEnc;

  enc->pnOutputBits[enc->nBitstreamBufferWrite] = size;
  for (i=0; i<(size + 7)/8; i++) {
    enc->ppBitstreamDelayBuffer[enc->nBitstreamBufferWrite][i] = databuf[i];
  }

  encoderStruct->data->SpatialDataLength = enc->pnOutputBits[enc->nBitstreamBufferRead];
  currentSize = encoderStruct->data->SpatialDataLength;
  for (j=0; j<(currentSize + 7)/8; j++) {
    encoderStruct->data->SpatialData[j] = enc->ppBitstreamDelayBuffer[enc->nBitstreamBufferRead][j];
  }

  enc->nBitstreamBufferRead  = (enc->nBitstreamBufferRead  + 1)%enc->nBitstreamDelayBuffer;
  enc->nBitstreamBufferWrite = (enc->nBitstreamBufferWrite + 1)%enc->nBitstreamDelayBuffer;
}

int EncUsac_getSpatialOutputBufferDelay(HANDLE_ENCODER encoderStruct)
{
  return(encoderStruct->data->hSpatialEnc->nOutputBufferDelay);
}


float * acelpGetZir(HANDLE_USAC_TD_ENCODER hAcelp) {
  return hAcelp->LPDmem.Txnq;
}
float * acelpGetLastLPC(HANDLE_USAC_TD_ENCODER hAcelp) {
  return hAcelp->LPDmem.Aq;
}

void acelpUpdatePastFDSynthesis(HANDLE_USAC_TD_ENCODER hAcelp, double* pPastFDSynthesis, double* pPastFDOrig, int lowpassLine) {

  int i;
  if (hAcelp != NULL) {
    for (i = 0; i < 1024 / 2 + 1 + M; i++){
      hAcelp->fdSynth[i] = (float)pPastFDSynthesis[i - M];
    }

    for (i = 0; i < 1024 / 2 + 1 + M; i++){
    	hAcelp->fdOrig[i] = (float)pPastFDOrig[-M + i];
    }

    hAcelp->lowpassLine = lowpassLine;
  }

  return;
}


int acelpLastSubFrameWasAcelp(HANDLE_USAC_TD_ENCODER hAcelp) {

  int result = 0;
  if ( hAcelp != NULL ) {
    result = (hAcelp->prev_mod== 0);

  }
  return result;
}


