/**********************************************************************
MPEG-4 Audio VM
Encoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

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

Copyright (c) 1996.



Header file: enc_tf.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
SE    Sebastien Etienne, Jean Bernard Rault CCETT <jbrault@ccett.fr>
OK    Olaf Kaehler, Fraunhofer IIS-A <kaehleof@iis.fhg.de>

Changes:
14-jun-96   HP    first version
18-jun-96   HP    added bit reservoir handling
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
09-aug-96   HP    added EncXxxInfo(), EncXxxFree()
15-aug-96   HP    changed EncXxxInit(), EncXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
19-feb-97   HP    added include <stdio.h>
08-apr-97   BT    added EncG729Init() EncG729Frame()
22-apr-99   HP    created from enc.h
19-oct-03   OK    cleanup for rewrite encoder
**********************************************************************/


#ifndef _enc_tf_h_
#define _enc_tf_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"
#include "encoder.h" 
#include "ms.h"
#include "ntt_win_sw.h"

/* 20070530 SBR */
/* SBR */
#include "ct_sbr.h"

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */

/* encoder modes: window sequence and shape behavior */
enum WIN_SWITCH_MODE {
  STATIC_LONG,
  STATIC_SHORT,
  LS_STARTSTOP_SEQUENCE,
  LONG_SHORT_SEQUENCE,
  FFT_PE_WINDOW_SWITCHING,
  NON_MEANINGFUL_TEST_SEQUENCE
};

enum  WINDOW_SHAPE_ADAPT{ 
  STATIC, 
  TOGGLE, 
  DYNAMIC
};


/* SAMSUNG_2005-09-30 : channel element for BSAC Multichannel */
typedef enum {
        CENTER_FRONT,           /* 1 */
        LR_FRONT,                               /* 2 */
        REAR_SUR,                               /* 1 */
        LR_SUR,                                 /* 2 */
        LFE,                                            /* 1 */
        LR_OUTSIDE,                     /* 2 */ 
        RESERVED        
}CH_CFG ;

struct channelElementData {
  TNS_COMPLEXITY        tns_select;
  int     ms_select;
  int     pns_sfb_start;
  int     aacAllowScalefacs;
  enum DC_FLAG aacSimulcast;
  enum WIN_SWITCH_MODE  win_switch_mode;
  enum WINDOW_SHAPE_ADAPT windowShapeAdapt;


  int     max_sfb;
  MSInfo  msInfo;
        
        int startChIdx;
        int endChIdx;
        int chNum;
        int extension;
        CH_CFG ch_type;

  /* variables used by the T/F mapping */
  WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS];
  WINDOW_SEQUENCE windowSequencePrev[MAX_TIME_CHANNELS];


  HANDLE_NTT_WINSW win_sw_module;

  int windowSequenceCounter;
  int windowShapeSeqCounter;
/* 20070530 SBR */
  SBR_CHAN *sbrChan[2];
};

typedef struct channelElementData * HANDLE_CHANNEL_ELEMENT_DATA;
/* ~SAMSUNG_2005-09-30 */


char *EncTfInfo (FILE *helpStream);

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
);


int EncTfFrame (
                ENCODER_DATA_TYPE     input,
                HANDLE_BSBITBUFFER   *au,                         /* buffers to hold output AccessUnits */
                const int             requestIPF,                 /* indicate that an IPF is requested within the next frames */
                USAC_SYNCFRAME_TYPE  *syncFrameType,              /* indicates the USAC sync frame state */
#ifdef I2R_LOSSLESS
#endif
                HANDLE_ENCODER        enc,
/* 20070530 SBR */
                float               **p_time_signal_orig
);

void EncTfFree (HANDLE_ENCODER enc);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _enc_tf_h_ */

/* end of enc_tf.h */
