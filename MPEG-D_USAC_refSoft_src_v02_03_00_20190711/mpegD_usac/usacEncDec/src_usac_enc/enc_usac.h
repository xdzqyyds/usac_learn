/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Fraunhofer IIS

and edited by:

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

#ifndef _enc_usac_h_
#define _enc_usac_h_

#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"
#include "encoder.h"
#include "ms.h"
#include "ntt_win_sw.h"
#include "sac_enc.h"

#define TD_BUFFER_OFFSET 448+64
#define MAX_STREAM_ID 65535

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagEncoderSpecificData ENCODER_DATA, *HANDLE_ENCODER_DATA;
typedef struct usacFrame_data USACFRAME_DATA, *HANDLE_USACFRAME_DATA;
typedef struct usacConfig_data USACCONFIG_DATA, *HANDLE_USACCONFIG_DATA;

char *EncUsacInfo (FILE *helpStream);

int EncUsacInit (
  int                 numChannel,
  float               fSample,
  HANDLE_ENCPARA      encPara,
  int                 *frameNumSample,
  int                 *frameMaxNumBit, /* SAMSUNG_2005-09-30 */
  DEC_CONF_DESCRIPTOR *dec_conf,
  int                 *numTrack,
  HANDLE_BSBITBUFFER  *asc,                   /* buffers to hold output Audio Specific Config */
  int                 *numAPRframes,          /* number of AudioPre-Roll frames in an IPF */
#ifdef I2R_LOSSLESS
  int                 type_PCM,
  int                 _osf,
#endif
  HANDLE_ENCODER      core,
  HANDLE_ENCODER      enc
);

  /*int Get_sbrenable(HANDLE_ENCODER encoderStruct, int *bitrate);*/

int EncUsacFrame (
  const ENCODER_DATA_TYPE input,
  HANDLE_BSBITBUFFER  *au,                    /* buffers to hold output AccessUnits */
  const int            requestIPF,            /* indicate that an IPF is requested within the next frames */
  USAC_SYNCFRAME_TYPE *syncFrameType,         /* indicates the USAC sync frame state */
  HANDLE_ENCODER       enc,
  float              **p_time_signal_orig
);

void EncUsacFree (HANDLE_ENCODER enc);


int EncUsac_getUsacDelay(HANDLE_ENCODER encoderStruct);
int EncUsac_getUsacEncoderDelay(
    HANDLE_ENCODER encoderStruct,
    int* encDelaySamples,
    int* encDelayFrames,
    const int downSampleRatio,
    const int upSampleRatio,
    const int nUpsampleTaps,
    const int nDownsampleTaps,
    const int usac212enable,
    int* delayBufferSamples);
int EncUsac_getusac212enable(HANDLE_ENCODER encoderStruct);
HANDLE_SPATIAL_ENC EncUsac_getSpatialEnc(HANDLE_ENCODER encoderStruct);
void EncUsac_setSpatialData(HANDLE_ENCODER encoderStruct, unsigned char *databuf, unsigned long size);
int EncUsac_getSpatialOutputBufferDelay(HANDLE_ENCODER encoderStruct);
int EncUsac_getIndependencyFlag(HANDLE_ENCODER encoderStruct, int offset);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _enc_usac_h_ */

/* end of enc_usac.h */
