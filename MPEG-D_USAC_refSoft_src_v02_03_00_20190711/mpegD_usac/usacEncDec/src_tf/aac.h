/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by


and edited by

Takashi Koike (Sony Corporation)

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



Source file: 

 

**********************************************************************/
#ifndef _aac_h_
#define _aac_h_

/* ----------------------------------------------------------
   This part is connection between DecTfFrame in dec_tf.c 
   and the aac-decoder starting in decoder.c                           
  ----------------------------------------------------------- */
#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "block.h"

/* --- Init AAC-decoder --- */
HANDLE_AACDECODER AACDecodeInit (int  sampling_rate_decoded,
                                 char *aacDebugStr, 
                                 int  block_size_samples,
                                 Info *** sfbInfo, 
                                 int  predictor_type,
                                 int  profile,
                                 const char *decPara);

/* ----- Decode one frame with the AAC-decoder  ----- */
int AACDecodeFrame ( HANDLE_AACDECODER        hDec,
                     HANDLE_BSBITSTREAM       fixed_stream,
                     HANDLE_BSBITSTREAM       gc_stream[MAX_TIME_CHANNELS],
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
		     enum AAC_BIT_STREAM_TYPE bitStreamType, 
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     Info**                   sfbInfo, 
                     byte                     sfbCbMap[MAX_TIME_CHANNELS][MAXBANDS],
                     short*                   pFactors[MAX_TIME_CHANNELS],
                     HANDLE_DECODER_GENERAL   hFault,
                     QC_MOD_SELECT            qc_select,
                     NOK_LT_PRED_STATUS**     nok_lt_status,
                     FRAME_DATA*              fd ,
                     int                      layer,
                     int                      er_channel,
                     int                      aacExtensionLayerFlag
                     );

void AACDecodeFree (HANDLE_AACDECODER hAACDecoder);

void aacAUDecode ( int numChannels,
                   FRAME_DATA* frameData,
                   TF_DATA* tfData,
                   HANDLE_DECODER_GENERAL hFault,
                   enum AAC_BIT_STREAM_TYPE bitStreamType) ;


void decodeBlockType( int coded_types, int granules, int gran_block_type[], int act_gran );

TNS_frame_info GetTnsData( void *paacDec, int index );
int  GetTnsDataPresent( void *paacDec, int index );
void SetTnsDataPresent( void *paacDec, int index, int value );
int  GetTnsExtPresent(  void *paacDec, int index );
void SetTnsExtPresent(  void *paacDec, int index, int value );

int GetBlockSize( void *paacDec );
long UpdateBufferFullness( void* paacDec, Float consumedBits, int decoderBufferSize, int incomingBits);


#ifdef CT_SBR

SBRBITSTREAM *getSbrBitstream   ( void *paacDec );
void          setMaxSfb         ( void *paacDec, int maxSfb );
int           getMaxSfbFreqLine ( void *paacDec );

#endif


#endif
