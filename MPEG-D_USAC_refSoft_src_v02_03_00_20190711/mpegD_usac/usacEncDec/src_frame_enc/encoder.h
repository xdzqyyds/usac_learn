/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

and edited by
Guillaume Fuchs (Fraunhofer IIS)

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

Copyright (c) 2003.

*/


#ifndef _ENCODER_H_INCLUDED
#define _ENCODER_H_INCLUDED


#define DELAY_ENC_FRAMES 2

#define USAC_MAX_EXTENSION_PAYLOADS MAX_TIME_CHANNELS
#define USAC_MAX_EXTENSION_PAYLOAD_LEN 6144/8*MAX_TIME_CHANNELS     /* bytes */

#define USAC_MAX_CONFIG_LEN 128                                     /* bytes */

#include "obj_descr.h"
#include "mp4au.h"
#include "usac_interface.h"
#include "tf_mainHandle.h"

typedef float** ENCODER_DATA_TYPE;

typedef struct tagEncoderData *HANDLE_ENCODER;
struct tagEncoderData {
  struct tagEncoderSpecificData *data;
  int (*getNumChannels)(HANDLE_ENCODER enc);
  ENCODER_DATA_TYPE (*getReconstructedTimeSignal)(HANDLE_ENCODER enc);
  enum MP4Mode (*getEncoderMode)(HANDLE_ENCODER enc);
  int (*getSbrEnable)(HANDLE_ENCODER enc, int *bitrate);
  int (*getFlag768)(HANDLE_ENCODER enc);
  int (*getSbrRatioIndex)(HANDLE_ENCODER enc);
  int (*getBitrate)(HANDLE_ENCODER enc);
#ifdef I2R_LOSSLESS
  int  osf;
#endif
  int ch_elm_tot; /* SAMSUNG_2005-09-30 : number of channel elements for BSAC Multichannel */
};

/* encoder modes: codec selection */
enum USAC_CODEC_MODE {
  USAC_SWITCHED,
  USAC_ONLY_FD,
  USAC_ONLY_TD,
  ___usac_codec_mode_end
};

/* encoder parameter data */
typedef struct encoderParameterData {
  int               debugLevel;
  int               wshape_flag;

  unsigned int      streamID_present;
  unsigned int      streamID;
  int               bitrate;
  int               bw_limit;  /* bandwidth limit of the spectrum */
  int               bUseFillElement;
  int               embedIPF;
  int               enableDrcEnc;

  /*--- TD/FD selection ---*/
  USAC_CORE_MODE    prev_coreMode[MAX_TIME_CHANNELS];
  USAC_CORE_MODE    coreMode[MAX_TIME_CHANNELS];
  enum USAC_CODEC_MODE    codecMode;

  /*--- SBR Data ---*/
  int               sbrenable;
  int               sbrRatioIndex;

  /*---FD Data---*/
  int               flag_960;
  int               flag_768;
  int               flag_twMdct;
  TNS_COMPLEXITY    tns_select;
  int               flag_harmonicBE;
  int               flag_noiseFilling;
  int               ms_select;
  int               ep_config;
  int               aacAllowScalefacs;

  /*--- MPEGS 212 Data ---*/
  int               flag_ipd;
  int               flag_mpsres;
  int               flag_cplxPred;
  char              tsdInputFileName[FILENAME_MAX];
} ENC_PARA, *HANDLE_ENCPARA;

typedef struct EncoderMode {
  int  mode;
  char *modename;
  char* (*EncInfo)(FILE*);
  int (*EncInit)(int                  numChannel,                 /* in: num audio channels */
                 float                fSample,                    /* in: sampling frequency [Hz] */
                 HANDLE_ENCPARA       encPara,                    /* in: encoder parameter struct */
                 int                 *frameNumSample,             /* out: num samples per frame */
                 int                 *frameMaxNumBit,             /* out: num samples per frame */
                 DEC_CONF_DESCRIPTOR *dec_conf,                   /* out: decoder config */
                                                                  /* descriptors for each track */
                 int *numTrack,                                   /* out: number of tracks */
                 HANDLE_BSBITBUFFER  *asc,                        /* buffers to hold output Audio Specific Config */
                 int                 *numAPRframes,               /* number of AudioPre-Roll frames in an IPF */
#ifdef I2R_LOSSLESS
                 int                  type_PCM,
                 int                  osf,
#endif
                 HANDLE_ENCODER       core,                       /* in:  core encoder or NULL */
                 HANDLE_ENCODER       enc                         /* out: internal id-datastructure */
  );

  int (*EncFrame)(ENCODER_DATA_TYPE     input,                    /*in: input signal */
                  HANDLE_BSBITBUFFER   *au,                       /*in: buffers to hold output AccessUnits */
                  const int             requestIPF,               /* indicate that an IPF is requested within the next frames */
                  USAC_SYNCFRAME_TYPE  *syncFrameType,            /* indicates the USAC sync frame state */
                  HANDLE_ENCODER        enc,
                  float               **p_time_signal_orig
  );

  void (*EncFree)(HANDLE_ENCODER enc
  );
} EncoderMode;

typedef struct {
  int                 encoder_count;
  int                 *trackCount;
  struct EncoderMode  *enc_mode;
  HANDLE_ENCODER      *enc_data;
  DEC_CONF_DESCRIPTOR dec_conf[MAX_TRACKS];
#ifdef I2R_LOSSLESS
  DEC_CONF_DESCRIPTOR ll_dec_conf[MAX_TRACKS];
  int				  ll_trackCount;
#endif
} ENC_FRAME_DATA ;

#endif
