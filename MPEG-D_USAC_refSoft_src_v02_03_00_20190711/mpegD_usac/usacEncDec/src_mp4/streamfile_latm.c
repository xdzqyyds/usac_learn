/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer, LATM part

This software module was originally developed by

Toshiyuki Nomura (NEC Corporation)

and edited by

Heiko Purnhagen (University of Hannover)
Olaf Kaehler (Fraunhofer IIS-A)
Manuela Schinn (Fraunhofer IIS)

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

Copyright (c) 1999.
Source file: streamfile_latm.c (was latm_modules.c before)

 

Required modules:
common_m4a.o            common module
cmdline.o               commandline parser
bitstream.o             bit stream module
flex_mux.o		flexmux module
streamfile_helper.o     stream file helper module

**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj_descr.h"           /* structs */

#include "allHandles.h"
#include "common_m4a.h"          /* common module       */
#include "bitstream.h"           /* bit stream module   */
#include "flex_mux.h"            /* parse object descriptors */
#include "cmdline.h"             /* parse commandline options */

#include "mp4au.h"               /* frame work common declarations */


#include "streamfile.h"          /* HANDLE_STREAMFILE */
#include "streamfile_latm.h"     /* public functions */
#include "streamfile_helper.h"    /* internal data */

#include "streamfile_latm.tbl"
#include "lpc_common.h"
#include "phi_cons.h"
#include "nec_abs_const.h"


/* ---- latm specific structures ---- */
#define MAXFRAME 64
#define MAXCHUNK 16
#define MAXSTREAM 16
#define MAXBUFFER 128
#define ASS_SyncWord 0x2B7

#define MODULE_INFORMATION "StreamFile transport lib: LATM module"

/* string that is written at the beginning of encoded files to indicate the type of the file (.ass);
   this information is required by the decoder */
static const char* bsopen_info_latm = "\nASS";


typedef struct {
  unsigned long audioMuxVersion;
  unsigned long audioMuxVersionA;
  unsigned long taraBufferFullness;
  unsigned long allStreamSameTimeFraming;
  unsigned long numSubFrames;
  unsigned long otherDataPresent;
  unsigned long otherDataLenEsc;
  unsigned long otherDataLenTmp;
  unsigned long crcCheckPresent;
  unsigned long crcCheckSum;

  unsigned long progIndx[MAXSTREAM];
  unsigned long trakIndx[MAXSTREAM];
  unsigned long otherDataLenBits;
} STREAM_MUX_CONFIG ;

typedef struct {
  unsigned long numChunk;
  unsigned long streamIndx[MAXCHUNK];
  /* PayloadLengthInfo */
  unsigned long muxSlotLength[MAXCHUNK];
  unsigned long AUEndFlag[MAXCHUNK];
} PAYLOAD_LENGTH_INFO ;

typedef struct {
  unsigned long       useSameStreamMux;
  STREAM_MUX_CONFIG   streamMuxConfig;
  PAYLOAD_LENGTH_INFO payloadLengthInfo;
  /* otherData                    "otherDataLenBits" bit */
} AUDIO_MUX_ELEMENT ;


struct tagStreamSpecificInfo {
  HANDLE_BSBITSTREAM bitStream;
  AUDIO_MUX_ELEMENT  audioMuxElement;
  int first_frame;
  int same_timing_requested;
#ifdef CT_SBR
  int hierarchical_signalling;
#endif
};

struct tagProgSpecificInfo {
  /* MuxConfig */
  unsigned long useSameConfig[MAXTRACK];
  unsigned long frameLengthType[MAXTRACK];
  unsigned long frameLength[MAXTRACK]; /* also CELP/HVXC table index */
  unsigned long bufferFullness[MAXTRACK];
  unsigned long coreFrameOffset[MAXTRACK];
  /* Payload */
  FIFO_BUFFER   AUbuffer[MAXTRACK];
};


static void celpSpecificCnvrt( HANDLE_STREAMPROG prog, int trackNr,
			       unsigned long *index )
{
  CELP_SPECIFIC_CONFIG *celpConf;
#ifdef CORRIGENDUM1
  CELP_SPECIFIC_CONFIG *celpEnhConf;
#else
  CELP_ENH_SPECIFIC_CONFIG *celpEnhConf;
#endif

  celpConf = &(prog->decoderConfig[0].audioSpecificConfig.specConf.celpSpecificConfig);

  if ( celpConf->excitationMode.value == RegularPulseExc ) {
    *index = 58 + celpConf->RPE_Configuration.value;
  }

  if ( celpConf->excitationMode.value == MultiPulseExc ) {
    if ( celpConf->sampleRateMode.value == fs8kHz) {
      *index = celpConf->MPE_Configuration.value;
    } else {
      if (celpConf->MPE_Configuration.value < 7) {
	*index = 28 + celpConf->MPE_Configuration.value;
      }
      if (celpConf->MPE_Configuration.value >= 8 &&
	  celpConf->MPE_Configuration.value < 16) {
	*index = 35 + (celpConf->MPE_Configuration.value - 8);
      }
      if (celpConf->MPE_Configuration.value >= 16 &&
	  celpConf->MPE_Configuration.value < 23) {
	*index = 43 + (celpConf->MPE_Configuration.value - 16);
      }
      if (celpConf->MPE_Configuration.value >= 24 &&
	  celpConf->MPE_Configuration.value < 32) {
	*index = 50 + (celpConf->MPE_Configuration.value - 24);
      }
    }
  }

  if ( trackNr > 0 ) {
    if ( celpConf->sampleRateMode.value == fs8kHz) {
      if ( celpConf->MPE_Configuration.value < 3 ) *index = 0;
      if ( celpConf->MPE_Configuration.value >= 3 &&
	   celpConf->MPE_Configuration.value < 6 ) *index = 1;
      if ( celpConf->MPE_Configuration.value >= 6 &&
	   celpConf->MPE_Configuration.value < 22 ) *index = 2;
      if ( celpConf->MPE_Configuration.value >= 22 &&
	   celpConf->MPE_Configuration.value < 27 ) *index = 3;
      if ( celpConf->bandwidthScalabilityMode.value == ON ) {
	if ( trackNr == (int)(celpConf->numEnhLayers.value+1) ) {
#ifdef CORRIGENDUM1
	  celpEnhConf = &(prog->decoderConfig[trackNr].audioSpecificConfig.specConf.celpSpecificConfig);
#else
	  celpEnhConf = &(prog->decoderConfig[trackNr].audioSpecificConfig.specConf.celpEnhSpecificConfig);
#endif
	  if ( celpConf->MPE_Configuration.value < 3 )
	    *index = 4 + celpEnhConf->BWS_Configuration.value;
	  if ( celpConf->MPE_Configuration.value >= 3 &&
               celpConf->MPE_Configuration.value < 6 )
	    *index = 8 + celpEnhConf->BWS_Configuration.value;
	  if ( celpConf->MPE_Configuration.value >= 6 &&
               celpConf->MPE_Configuration.value < 22 )
	    *index = 12 + celpEnhConf->BWS_Configuration.value;
	  if ( celpConf->MPE_Configuration.value >= 22 &&
               celpConf->MPE_Configuration.value < 27 )
	    *index = 16 + celpEnhConf->BWS_Configuration.value;
	}
      }
    } else {
      if ( celpConf->MPE_Configuration.value < 16 ) *index = 0;
      if ( celpConf->MPE_Configuration.value >= 16 &&
	   celpConf->MPE_Configuration.value < 32 ) *index = 2;
    }
  }
}


static int setupMuxConfig(HANDLE_STREAMFILE stream, unsigned long cur_frame)
{
  AUDIO_MUX_ELEMENT* audioMuxConf = &stream->spec->audioMuxElement;
  STREAM_MUX_CONFIG* muxConf = &audioMuxConf->streamMuxConfig;
  int i,j;

  if ( stream->spec->first_frame ) {
    stream->spec->first_frame = 0;
    audioMuxConf->useSameStreamMux = 0;
  } else {
    audioMuxConf->useSameStreamMux = 1;
  }
  if ( cur_frame != muxConf->numSubFrames+1 ) {
    audioMuxConf->useSameStreamMux = 0;
    muxConf->numSubFrames = cur_frame - 1;
  }

  if (audioMuxConf->useSameStreamMux == 0) { /* prepare new MuxConfig */
    int preTimeMS = 0, frameTimeMS;

    /* muxConf->audioMuxVersion = 0; This is now provided via a cmdline argument */

    muxConf->taraBufferFullness = 0xff; /* VBR */
    muxConf->allStreamSameTimeFraming = stream->spec->same_timing_requested;
    
    /** allStreamSameTimeFraming **/
    /* this is not properly supported if allStreamSameTimeFraming==0
       and there are really different time frames for the streams.
       This leads to problems in case of variable frame length, especially
       CELP/AAC combinations */
    if (muxConf->allStreamSameTimeFraming==0) CommonWarning("allStreamSameTimeFraming==0 might not be properly supported!\n");

    /** otherDataPresent, otherDataLenBits **/
    muxConf->otherDataPresent = 0;
    muxConf->otherDataLenEsc = 0;
    muxConf->otherDataLenTmp = 0;
    muxConf->otherDataLenBits = 0;

    /** crcCheckPresent, crcCheckSum **/
    muxConf->crcCheckPresent = 0;
    muxConf->crcCheckSum = 0;

    for ( i = 0; i < stream->progCount; i++ ){
      HANDLE_STREAMPROG prog = &stream->prog[i];
      for ( j = 0; j < (int)(prog->trackCount); j++ ) {
        DEC_CONF_DESCRIPTOR* decConf = &prog->decoderConfig[j];
        unsigned long *frameLengthType = &prog->programData->spec->frameLengthType[j];
        unsigned long *frameLength = &prog->programData->spec->frameLength[j];
        unsigned long *bufferFullness = &prog->programData->spec->bufferFullness[j];
        unsigned long *coreFrameOffset = &prog->programData->spec->coreFrameOffset[j];
        unsigned long *useSameConfig = &prog->programData->spec->useSameConfig[j];
        int frameTimeLength = prog->programData->sampleDuration[j];

        switch (decConf->audioSpecificConfig.audioDecoderType.value) {
        case TWIN_VQ:
          *useSameConfig = 0;
          *frameLengthType = 1; /* fixed length */
          *frameLength = ((decConf->avgBitrate.value * frameTimeLength / decConf->audioSpecificConfig.samplingFrequency.value) >> 3) - 20;
          /*bufferFullness irrelevant! coreFrameOffset ?*/
          break;
        case AAC_MAIN:
        case AAC_LC:
        case AAC_SSR:
        case AAC_LTP:
        case AAC_SCAL:
#ifdef CT_SBR
        case SBR:
#endif
        case ER_AAC_LC:
        case ER_AAC_LD:
#ifdef AAC_ELD
        case ER_AAC_ELD:
#endif
        case ER_AAC_LTP:
        case ER_AAC_SCAL:
        case ER_BSAC:
        case ER_TWIN_VQ:
#ifdef EXT2PAR
        case SSC:
#endif
#ifdef MPEG12
      case LAYER_1:
      case LAYER_2:
      case LAYER_3:
#endif
          *useSameConfig = 0;
          *frameLengthType = 0; /* variable length */
          /*frameLength irrelevant !*/
          *bufferFullness = 255; 
          *coreFrameOffset = 0; /* ??? */
          break;
        case ER_HILN: /* HP20001009 */
        case ER_PARA:
          *useSameConfig = 0;
          *frameLengthType = 0; /* variable */
          /*frameLength irrelevant !*/
          *bufferFullness = 255; /* ??? */
          *coreFrameOffset = 0; /* ??? */
          break;
        case ER_CELP:
        case CELP:
#ifndef CORRIGENDUM1
          if ( streamCnt == 0 ) *useSameConfig = 0;
          else if ( streamCnt == (int)(prog->decoderConfig[0].audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value + 1) ) {
            if ( prog->decoderConfig[0].audioSpecificConfig.specConf.celpSpecificConfig.bandwidthScalabilityMode.value == ON )
              *useSameConfig = 0;
          } else *useSameConfig = 1;
#else
          *useSameConfig = 0;
#endif
          if ( prog->decoderConfig[0].audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl.value == 1 ) 
            *frameLengthType = 3;
          else if ( prog->decoderConfig[0].audioSpecificConfig.specConf.celpSpecificConfig.silenceCompressionSW.value == 1 ) 
            *frameLengthType = 5;
          else
            *frameLengthType = 4;

          celpSpecificCnvrt( prog, j, frameLength /*celp table index*/);
          break;
        case HVXC :
        case ER_HVXC :
          /* bug fix of frameLengthType and HVXCframeLengthTableIndex for ER_HVXC object type(AI 991129) */
          if ( (j == 0) ) {
            *useSameConfig = 0;
            if ( decConf->audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value == 1 ) 
              *frameLengthType = 7;
            else
              *frameLengthType = 6;
            *frameLength /*TableIndex*/ = decConf->audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value;
          } else {
            *useSameConfig = 1;
            *frameLengthType = prog->programData->spec->frameLengthType[0];
            *frameLength = prog->programData->spec->frameLength[0];
          }
          break;
        default:
          CommonWarning("StreamFile:LATM: audioDecoderType not implemented");
          break;
        }
        /* Accuracy limit is 1 ms */
        frameTimeMS = (frameTimeLength*1000)/decConf->audioSpecificConfig.samplingFrequency.value;
        if ( i == 0 && j == 0 ) preTimeMS = frameTimeMS;
	if ( preTimeMS != frameTimeMS ) {
	  muxConf->allStreamSameTimeFraming = 0;
        }
      }
    }
  }

  return 0;
}


static int setLengthInfo(HANDLE_STREAMPROG prog, int track, unsigned long *slotLength, unsigned long lengthBits)
{
  int i;
  int type = prog->programData->spec->frameLengthType[track];
  unsigned long *table = &prog->programData->spec->frameLength[track];
  unsigned long tmp, len, align;
 
  lengthBits = ((int)((lengthBits+7)/8)) * 8; /* align bits */
  
  switch (type) {
  case 0:
    *slotLength = (lengthBits+7) >> 3;
    break;
  case 1:
    tmp = (lengthBits >> 3) - 20;
    if (prog->programData->spec->frameLength[track] != tmp) {
      CommonWarning("StreamFile:LATM: FrameLengthType %i, but frame length changes!",type);
      *slotLength = (unsigned)-1;
    }
    break;
  case 2:
    CommonWarning("StreamFile:LATM: reserved FrameLengthType 2 requested!");
    *slotLength = (unsigned)-1;
    break;
  case 3: /* CELP 1_of_2 */
    if ( track == 0 ) {
      *slotLength = (unsigned)-2;
      for ( i = 0; i < 2; i ++ ) {
	len = celptb1[*table][i*4+1];
	align = 8 - len%8;
	if (align == 8) align = 0;
	if ( (len+align) == lengthBits ) {
	  *slotLength = i;
	}
      }
    } else {
      *slotLength = (unsigned)-1;
      CommonWarning("StreamFile:LATM: conversion error in tblconv (FrameLengthType %i, layer %i",type,track);
    }
    break;
  case 4: /* CELP fixed */
    if ( track == 0 ) len = celptb1[*table][0];
    else              len = celptb2[*table][0];
    align = 8 - len%8;
    if (align == 8) align = 0;
    if ( (len+align) != lengthBits )
      *slotLength = (unsigned)-2;
    else
      *slotLength = 0;
    break;
  case 5: /* CELP 1_of_4 */
    *slotLength = (unsigned)-2;
    if ( track == 0 ) {
      for ( i = 0; i < 4; i ++ ) {
	len = celptb1[*table][i+1];
	align = 8 - len%8;
	if (align == 8) align = 0;
	if ( (len+align) == lengthBits ) {
          *slotLength = i;
	  break;
	}
      }
    } else { /* track > 0 */
      for ( i = 0; i < 4; i ++ ) {
	len = celptb2[*table][i];
	align = 8 - len%8;
	if (align == 8) align = 0;
	if ( (len+align) == lengthBits ) {
          *slotLength = i;
	}
      }
    }
    break;
  case 6: /* HVXC fixed */
    len = hvxctbl[*table][0];
    align = 8 - len%8;
    if (align == 8) align = 0;
    if ( (len+align) != lengthBits )
      *slotLength = (unsigned)-2;
    else
      *slotLength=0;
    break;
  case 7:/* HVXC 1_of_4 */
    *slotLength = (unsigned)-2;
    for ( i = 0; i < 4; i ++ ) {
      len = hvxctbl[*table][i];
      align = 8 - len%8;
      if (align == 8) align = 0;
      if ( (len+align) == lengthBits ) {
	*slotLength = i;
      }
    }
  }
  
  if ( *slotLength == (unsigned)-2 )
    CommonWarning("StreamFile:LATM: conversion error in tblconv (FrameLengthType %i, length %ibit)",type,lengthBits);
  return (*slotLength>=(unsigned)-2);
}

static int getLengthInfo(HANDLE_STREAMPROG prog, int track, unsigned long slotLength)
{
  int type = prog->programData->spec->frameLengthType[track];
  int table = prog->programData->spec->frameLength[track];   /* CELP/HVXC table */

  switch (type) {
  case 0:    /* variable length */
    return slotLength*8;
  case 1:    /* fixed length */
    return 8*(20+prog->programData->spec->frameLength[track]);
  case 2:    /* reserved */
    CommonWarning("StreamFile:LATM: reserved FrameLengthType 2 requested!");
  case 3:    /* CELP 1_of_2 */
    return celptb1[table][4*(slotLength)+1];
  case 4:    /* CELP fixed */
    if ( track == 0 ) return celptb1[table][0];
    else              return celptb2[table][0];
  case 5:    /* CELP 1_of_4 */
    if ( track == 0 ) return celptb1[table][slotLength+1];
    else              return celptb2[table][slotLength];
  case 6:    /* HVXC fixed */
    return hvxctbl[table][0];
  case 7:    /* HVXC 1_of_4 */
    return hvxctbl[table][slotLength];
  }
  return -1;
}

static int setupPayloadLengthInfo( HANDLE_STREAMFILE stream )
{
  int i, j, ret;
  int streamCnt = 0, chunkCnt = 0;
  AUDIO_MUX_ELEMENT   *audioMuxElement = &stream->spec->audioMuxElement;
  PAYLOAD_LENGTH_INFO *lenInfo = &audioMuxElement->payloadLengthInfo;

  for ( i = 0; i < (int)(stream->progCount); i++ ) {
    HANDLE_STREAMPROG prog = &stream->prog[i];
    for ( j = 0; j < (int)(prog->trackCount); j++ ) {
      unsigned long duration = 0;
      int AUinBuffer = 0;
      while (duration < prog->programData->timePerFrame) {
        HANDLE_STREAM_AU tmpAU = FIFObufferGet(prog->programData->spec->AUbuffer[j],AUinBuffer);

        if (tmpAU==NULL) {
          CommonWarning("StreamFile:LATM: Too few AUs buffered to write frame");
          return -1;
        }
        
        lenInfo->streamIndx[chunkCnt] = streamCnt;
        lenInfo->AUEndFlag[chunkCnt] = 1; /* ?? */

        ret = setLengthInfo(prog, j, &lenInfo->muxSlotLength[chunkCnt], tmpAU->numBits);
        if (ret) return ret;

        if (chunkCnt > MAXCHUNK)
          CommonWarning("StreamFile:LATM: Too many chunks requested! (%i)",chunkCnt);
        duration += prog->programData->timePerAU[j];
        AUinBuffer++;
        chunkCnt++;
      }
      streamCnt++;
    }
  }
  lenInfo->numChunk = chunkCnt - 1;
        
  return 0;
}

/* ---- LATM Get Value ---- */
static void LatmGetValue(HANDLE_BSBITSTREAM bitStream, unsigned long *latmValue, int WriteFlag)
{
  unsigned long bytesForValueM1, valueTmp;

  if ( WriteFlag == 0 ) {
    BsRWBitWrapper(bitStream, &bytesForValueM1, 2, WriteFlag);
    BsRWBitWrapper(bitStream, latmValue, 8*(bytesForValueM1+1), WriteFlag);
  }
  else {

  bytesForValueM1 = 0;
  for ( valueTmp = (*latmValue >> 8); valueTmp > 0; valueTmp >>= 8 ) {
    bytesForValueM1++;
  }

    BsRWBitWrapper(bitStream, &bytesForValueM1, 2, WriteFlag);
    BsRWBitWrapper(bitStream, latmValue, 8*(bytesForValueM1+1), WriteFlag);
  }
}

/* ---- Stream Mux Configuration ---- */
static int advanceStreamMuxConfig(HANDLE_STREAMFILE stream, int WriteFlag)
{
  int i, j, trackCount = 0, first;
  unsigned long tmp, progCount, ascLenBs = 0, ascLen;
  HANDLE_BSBITSTREAM bitStream = stream->spec->bitStream;
  STREAM_MUX_CONFIG* muxConf = &stream->spec->audioMuxElement.streamMuxConfig;

  BsRWBitWrapper(bitStream, &muxConf->audioMuxVersion, 1, WriteFlag);
  if ( muxConf->audioMuxVersion == 1 )
    BsRWBitWrapper(bitStream, &muxConf->audioMuxVersionA, 1, WriteFlag);
  else
    muxConf->audioMuxVersionA = 0;
  if ( muxConf->audioMuxVersionA == 0 ) {
    if ( muxConf->audioMuxVersion == 1 )
      LatmGetValue (bitStream, &muxConf->taraBufferFullness, WriteFlag);
    BsRWBitWrapper(bitStream, &muxConf->allStreamSameTimeFraming,1, WriteFlag);
    BsRWBitWrapper(bitStream, &muxConf->numSubFrames,6, WriteFlag);
    DebugPrintf(6, "StreamFile:LATM: number of subframes : %i",muxConf->numSubFrames+1);
    progCount = stream->progCount-1;
    BsRWBitWrapper(bitStream, &progCount, 4, WriteFlag); /* number of programs ! */
    stream->progCount = ++progCount;
    DebugPrintf(6, "StreamFile:LATM: number of programs  : %i",stream->progCount);

    for ( i = 0; i < stream->progCount; i++ ) {
      HANDLE_STREAMPROG prog;
      if (stream->prog[i].programData==NULL) {
        stream->progCount = i;
        prog = newStreamProg(stream);
        stream->progCount = progCount;
      } else
        prog = &stream->prog[i];

      tmp = prog->trackCount-1;
      BsRWBitWrapper(bitStream, &tmp,3, WriteFlag); /* number of tracks ! */
      prog->trackCount = tmp+1;
      DebugPrintf(6, "StreamFile:LATM: number of tracks(%1i) : %i",i,prog->trackCount);

      for ( j = 0; j < (int)(prog->trackCount); j++ ){
        if (prog->programData->spec->AUbuffer[j] == NULL) {
          prog->programData->spec->AUbuffer[j] = FIFObufferCreate(MAXBUFFER);
          if (prog->programData->spec->AUbuffer[j] == NULL)
            CommonWarning("StreamFile:LATM: could not create buffer");
        }
        muxConf->progIndx[trackCount] = i;
        muxConf->trakIndx[trackCount] = j;

        setupDecConfigDescr(&prog->decoderConfig[j]);
        if (WriteFlag == 0) {
          presetDecConfigDescr(&prog->decoderConfig[j]);
          if ( j==0 ) prog->dependencies[j] = -1;
          else prog->dependencies[j] = j-1;
        }
        /*prog->decoderConfig[j].maxBitrate.value=10;
          prog->decoderConfig[j].avgBitrate.value=10;*/
        if ( i == 0 && j == 0 )
          prog->programData->spec->useSameConfig[j] = 0;
        else
          BsRWBitWrapper(bitStream,&prog->programData->spec->useSameConfig[j],1, WriteFlag);
        if (prog->programData->spec->useSameConfig[j] == 0) {
#ifdef CT_SBR
          if (stream->spec->hierarchical_signalling == 1) {
            if (prog->decoderConfig[j].audioSpecificConfig.sbrPresentFlag.value == 1) {
#ifdef PARAMETRICSTEREO
              if (prog->decoderConfig[j].audioSpecificConfig.psPresentFlag.value == 1)
                prog->decoderConfig[j].audioSpecificConfig.audioDecoderType.value = PS;
              else
#endif
                prog->decoderConfig[j].audioSpecificConfig.audioDecoderType.value = SBR;
            }
            prog->decoderConfig[j].audioSpecificConfig.extensionAudioDecoderType.value = 0; 
          }
          else if ( (WriteFlag == 1) &&
                    (muxConf->audioMuxVersion == 0) &&
                    (prog->decoderConfig[j].audioSpecificConfig.sbrPresentFlag.value == 1) ) {
             /* backward compatible signaling of SBR and PS is not allowed for audioMuxVersion=0 */
             prog->decoderConfig[j].audioSpecificConfig.audioDecoderType.value = AAC_LC;
             prog->decoderConfig[j].audioSpecificConfig.extensionAudioDecoderType.value = 0;
          }
#endif
          if ( muxConf->audioMuxVersion == 1 ) {
            if (WriteFlag == 1) {
              ascLenBs = advanceAudioSpecificConfig(bitStream, &prog->decoderConfig[j].audioSpecificConfig, 2/*calc length*/, 0/*no systems-specific stuff*/
#ifndef CORRIGENDUM1
                                         ,j
#endif
                                         );
            }
            LatmGetValue (bitStream, &ascLenBs, WriteFlag);
          }
          ascLen = advanceAudioSpecificConfig(bitStream, &prog->decoderConfig[j].audioSpecificConfig, WriteFlag, 0/*no systems-specific stuff*/
#ifndef CORRIGENDUM1
                                     ,j
#endif
                                     );
          if ( muxConf->audioMuxVersion == 1 ) {
            if ( ascLenBs > ascLen ) {
              BsGetSkip (bitStream, ascLenBs - ascLen);
            }
          }
        } else {
          prog->decoderConfig[j].audioSpecificConfig = stream->prog[muxConf->progIndx[trackCount-1]].decoderConfig[muxConf->trakIndx[trackCount-1]].audioSpecificConfig;
        }
        trackCount++;

	BsRWBitWrapper(bitStream,&prog->programData->spec->frameLengthType[j],3, WriteFlag);
	if ( prog->programData->spec->frameLengthType[j] == 0 ) {
	  BsRWBitWrapper(bitStream,&prog->programData->spec->bufferFullness[j],8, WriteFlag);
	  if ( (!muxConf->allStreamSameTimeFraming)&&(j>0) ) {
	    if ((prog->decoderConfig[j].audioSpecificConfig.audioDecoderType.value == AAC_SCAL ||
		 prog->decoderConfig[j].audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL ) &&
		(prog->decoderConfig[j-1].audioSpecificConfig.audioDecoderType.value == CELP ||
		 prog->decoderConfig[j-1].audioSpecificConfig.audioDecoderType.value == ER_CELP )) {
	      BsRWBitWrapper(bitStream,&prog->programData->spec->coreFrameOffset[j],6, WriteFlag);
	    }
	  }
	} else if ( prog->programData->spec->frameLengthType[j] == 1 ) {
	  BsRWBitWrapper(bitStream,&prog->programData->spec->frameLength[j],9, WriteFlag);
	} else if ( prog->programData->spec->frameLengthType[j] == 4 ||
		    prog->programData->spec->frameLengthType[j] == 5 ||
		    prog->programData->spec->frameLengthType[j] == 3 ) { 
	  BsRWBitWrapper(bitStream,&prog->programData->spec->frameLength[j],6, WriteFlag); /*CELPframeLengthTableIndex*/
	} else if ( prog->programData->spec->frameLengthType[j] == 6 ||
		    prog->programData->spec->frameLengthType[j] == 7 ) { 
	  BsRWBitWrapper(bitStream,&prog->programData->spec->frameLength[j],1, WriteFlag);/*HVXCframeLengthTableIndex*/
	}
      }
    }

    BsRWBitWrapper(bitStream,&muxConf->otherDataPresent,1, WriteFlag);
    if ( muxConf->otherDataPresent ) {
      if ( muxConf->audioMuxVersion == 1 )
        LatmGetValue (bitStream, &muxConf->otherDataLenBits, WriteFlag);
      else if ( WriteFlag == 1 ) { /* write the length */
        tmp = muxConf->otherDataLenBits;
        first = 0;
        for ( i = 0; i < 4; i++ ) {
          muxConf->otherDataLenTmp = (tmp & 0xFF000000) >> 24;
          tmp = (tmp & 0xFFFFFF) << 8;

          if ( !first ) {
            if ( muxConf->otherDataLenTmp == 0 ) continue;
            first = 1;
          }
          if ( i == 3 ) muxConf->otherDataLenEsc = 0;
          else muxConf->otherDataLenEsc = 1;

          BsPutBit(bitStream,muxConf->otherDataLenEsc,1);
          BsPutBit(bitStream,muxConf->otherDataLenTmp,8);
        }
      } else if (WriteFlag == 0) { /* read the length */
        muxConf->otherDataLenBits = 0;
        do {
          muxConf->otherDataLenBits = muxConf->otherDataLenBits << 8;
          BsGetBit(bitStream,&(muxConf->otherDataLenEsc),1);
          BsGetBit(bitStream,&(muxConf->otherDataLenTmp),8);
          muxConf->otherDataLenBits += muxConf->otherDataLenTmp;
        } while ( muxConf->otherDataLenEsc );
      }
    }
    BsRWBitWrapper(bitStream,&muxConf->crcCheckPresent,1, WriteFlag);
    if ( muxConf->crcCheckPresent )
      BsRWBitWrapper(bitStream,&muxConf->crcCheckSum,8, WriteFlag);
  } else {
    CommonWarning("StreamFile:LATM: audioMuxVersionA 1 not defined/implemented");
    return -1;
  }
  return 0;
}


/* ---- Payload Length Info ---- */
static int advancePayloadLengthInfo(HANDLE_STREAMFILE stream, int WriteFlag)
{
  int i, j=0;
  int chunkCnt = 0;
  unsigned long tmp;
  HANDLE_BSBITSTREAM  bitStream = stream->spec->bitStream;
  STREAM_MUX_CONFIG   *muxConf = &stream->spec->audioMuxElement.streamMuxConfig;
  PAYLOAD_LENGTH_INFO *lenInfo = &stream->spec->audioMuxElement.payloadLengthInfo;

  if ( muxConf->allStreamSameTimeFraming ) {
    for ( i = 0; i < stream->progCount; i++ ) {
      HANDLE_STREAMPROG prog = &stream->prog[i];
      for ( j = 0; j < (int)prog->trackCount; j++ ) {
	if ( prog->programData->spec->frameLengthType[j] == 0 ) {
          if ( WriteFlag == 1 ) { /* write */
            tmp = lenInfo->muxSlotLength[chunkCnt];
            while ( tmp >= 255 ) {
              BsPutBit(bitStream,255,8);
              tmp -= 255;
            }
            BsPutBit(bitStream,tmp,8);
          } else if ( WriteFlag == 0 ) { /*read*/
            lenInfo->muxSlotLength[chunkCnt] = 0;
            lenInfo->streamIndx[chunkCnt] = chunkCnt;
            do {
              BsGetBit(bitStream,&tmp,8);  /* tmp */
              lenInfo->muxSlotLength[chunkCnt] += tmp;
            } while ( tmp == 255 );
          }
	} else if ( prog->programData->spec->frameLengthType[j] == 5 ||
		    prog->programData->spec->frameLengthType[j] == 7 ||
		    prog->programData->spec->frameLengthType[j] == 3 ) {
	  BsRWBitWrapper(bitStream,&lenInfo->muxSlotLength[chunkCnt],2, WriteFlag);
	}
        chunkCnt++;
      }
    }
  } else {
    BsRWBitWrapper(bitStream,&lenInfo->numChunk,4, WriteFlag);
    for ( chunkCnt = 0; chunkCnt < (int)(lenInfo->numChunk+1); chunkCnt++ ){
      HANDLE_STREAMPROG prog;
      int trackIdx;

      BsRWBitWrapper(bitStream,&lenInfo->streamIndx[chunkCnt],4, WriteFlag);
      prog = &stream->prog[muxConf->progIndx[lenInfo->streamIndx[chunkCnt]]];
      trackIdx = muxConf->trakIndx[lenInfo->streamIndx[chunkCnt]];

      if ( prog->programData->spec->frameLengthType[trackIdx] == 0 ) {
        if (WriteFlag == 1) {
          tmp = (lenInfo->muxSlotLength[chunkCnt]);
          while ( tmp >= 255 ) {
            BsPutBit(bitStream,255,8);
            tmp -= 255;
          }
          BsPutBit(bitStream,tmp,8);
        } else if ( WriteFlag == 0 ) { /*read*/
          lenInfo->muxSlotLength[chunkCnt] = 0;
          do {
            BsGetBit(bitStream,&tmp,8);
            lenInfo->muxSlotLength[chunkCnt] += tmp;
          } while ( tmp == 255 );
        }
        BsRWBitWrapper(bitStream,&lenInfo->AUEndFlag[chunkCnt],1, WriteFlag);
      } else if ( prog->programData->spec->frameLengthType[trackIdx] == 5 ||
	 	  prog->programData->spec->frameLengthType[trackIdx] == 7 ||
		  prog->programData->spec->frameLengthType[trackIdx] == 3 ) {
        BsRWBitWrapper(bitStream,&lenInfo->muxSlotLength[chunkCnt],2,WriteFlag);
      }
    }
  }
  return 0;
}


/* ---- Payload Mux ---- */
static int writePayloadMux(HANDLE_STREAMFILE stream)
{
  HANDLE_BSBITSTREAM bitStream = stream->spec->bitStream;
  AUDIO_MUX_ELEMENT* audioMuxElement = &stream->spec->audioMuxElement;
  int i, j;
  int chunkCnt = 0;
  unsigned long k;
  STREAM_MUX_CONFIG   *muxConf;
  PAYLOAD_LENGTH_INFO *lenInfo;

  muxConf = &(audioMuxElement->streamMuxConfig);
  lenInfo = &(audioMuxElement->payloadLengthInfo);

  if ( muxConf->allStreamSameTimeFraming ) {
    for ( i = 0; i < (int)(stream->progCount); i++ ){
      HANDLE_STREAMPROG prog = &stream->prog[i];
      for ( j = 0; j < (int)(prog->trackCount); j++ ){
        HANDLE_STREAM_AU au = FIFObufferPop(prog->programData->spec->AUbuffer[j]);
        unsigned long writeBitCount = getLengthInfo(prog, j, lenInfo->muxSlotLength[chunkCnt++]);

        if (au==NULL) {
          CommonWarning("StreamFile:LATM: Too few AUs buffered to write frame");
          return -1;
        }

        if (((writeBitCount+7)>>3) != ((au->numBits+7)>>3))
          CommonWarning("StreamFile:LATM: implementation error in writePayloadMux (%i!=%i)",writeBitCount,au->numBits);
        DebugPrintf(5,"StreamFile:LATM: writing prog %i, track %i, length=%i(%i)",i,j,
                    writeBitCount, au->numBits);

        for (k=0; k<(writeBitCount)>>3; k++) {
          unsigned long tmp = au->data[k];
          BsPutBit(bitStream,tmp,8);
        }
        if (writeBitCount & 7)
          BsPutBit(bitStream, (au->data[k]>>(8-(writeBitCount&7))), writeBitCount&7);
        
        StreamFileFreeAU(au);
      }
    }
  } else {
    for ( chunkCnt = 0; chunkCnt < (int)(lenInfo->numChunk+1); chunkCnt++ ){
      HANDLE_STREAMPROG prog = &stream->prog[muxConf->progIndx[lenInfo->streamIndx[chunkCnt]]];
      int trackIdx = muxConf->trakIndx[lenInfo->streamIndx[chunkCnt]];
      HANDLE_STREAM_AU au = FIFObufferPop(prog->programData->spec->AUbuffer[trackIdx]);
      unsigned long writeBitCount = getLengthInfo(prog, trackIdx, lenInfo->muxSlotLength[chunkCnt]);

      if (au==NULL) {
        CommonWarning("StreamFile:LATM: Too few AUs buffered to write frame");
        return -1;
      }
      if (((writeBitCount+7)>>3) != ((au->numBits+7)>>3))
        CommonWarning("StreamFile:LATM: implementation error in writePayloadMux (%i!=%i)",writeBitCount,au->numBits);
      DebugPrintf(5,"StreamFile:LATM: writing chunk %i (prog:%i, track:%i), length=%i(%i)",
                  chunkCnt, muxConf->progIndx[lenInfo->streamIndx[chunkCnt]], trackIdx,
                  writeBitCount, au->numBits);

      for (k=0; k<(writeBitCount)>>3; k++) {
        unsigned long tmp = au->data[k];
        BsPutBit(bitStream,tmp,8);
      }
      if (writeBitCount & 7)
    	BsPutBit(bitStream, (au->data[k]>>(8-(writeBitCount&7))), writeBitCount&7);
      
      StreamFileFreeAU(au);
    }
  }
  return 0;
}


static int readPayloadMux(HANDLE_STREAMFILE stream)
{
  int i, j;
  int chunkCnt = 0;
  unsigned long k;
  STREAM_MUX_CONFIG   *muxConf = &stream->spec->audioMuxElement.streamMuxConfig;
  PAYLOAD_LENGTH_INFO *lenInfo = &stream->spec->audioMuxElement.payloadLengthInfo;
  HANDLE_BSBITSTREAM  bitStream = stream->spec->bitStream;

  if ( muxConf->allStreamSameTimeFraming ) {
    for ( i = 0; i < (int)(stream->progCount); i++ ){
      HANDLE_STREAMPROG prog = &stream->prog[i];
      for ( j = 0; j < (int)(prog->trackCount); j++ ){
        unsigned long numBits = getLengthInfo(prog, j, lenInfo->muxSlotLength[chunkCnt]);
        HANDLE_STREAM_AU au = StreamFileAllocateAU(numBits);

        DebugPrintf(5,"StreamFile:LATM: reading prog %i, track %i, length=%i (%i,%i)",i,j,numBits,lenInfo->muxSlotLength[chunkCnt],chunkCnt);

        for (k=0; k<(numBits)>>3; k++)
          BsGetBitChar(bitStream, &au->data[k], 8);
        if (numBits & 7) {
          BsGetBitChar(bitStream, &au->data[k], numBits&7);
          au->data[k]<<=8-(numBits&7);
        }
        if (FIFObufferPush(prog->programData->spec->AUbuffer[j],au))
          CommonWarning("StreamFile:LATM: Error saving AU to buffer");

        chunkCnt++;
      }
    }
  } else {
    for ( chunkCnt = 0; chunkCnt < (int)(lenInfo->numChunk+1); chunkCnt++ ){
      HANDLE_STREAMPROG prog = &stream->prog[muxConf->progIndx[lenInfo->streamIndx[chunkCnt]]];
      int trackIdx = muxConf->trakIndx[lenInfo->streamIndx[chunkCnt]];
      unsigned long numBits = getLengthInfo(prog, trackIdx, lenInfo->muxSlotLength[chunkCnt]);
      HANDLE_STREAM_AU au = StreamFileAllocateAU(numBits);

      DebugPrintf(5,"StreamFile:LATM: reading chunk %i (prog:%i, track:%i), length=%i",
                  chunkCnt,muxConf->progIndx[lenInfo->streamIndx[chunkCnt]],trackIdx,
                  numBits);

      StreamFile_AUresize(au, numBits);
      for (k=0; k<(numBits)>>3; k++)
        BsGetBitChar(bitStream, &au->data[k], 8);
      if (numBits & 7) {
        BsGetBitChar(bitStream, &au->data[k], numBits&7);
        au->data[k]<<=8-(numBits&7);
      }
      if (FIFObufferPush(prog->programData->spec->AUbuffer[trackIdx],au))
        CommonWarning("StreamFile:LATM: Error saving AU to buffer");
    }
  }
  return 0;
}

static int advancePayloadMux(HANDLE_STREAMFILE stream, int WriteFlag)
{
  if (WriteFlag == 0) { /*read*/
    return readPayloadMux(stream);
  } else if (WriteFlag == 1) { /* write*/
    return writePayloadMux(stream);
  }
  return -1;
}


/* ---- Complete Mux Element ---- */
static int advanceAudioMuxElementH(HANDLE_STREAMFILE stream, int muxConfigPresent, int WriteFlag )
{
  HANDLE_BSBITSTREAM bitStream = stream->spec->bitStream;
  AUDIO_MUX_ELEMENT *audioMuxElement = &stream->spec->audioMuxElement;

  if ( muxConfigPresent ) {
    BsRWBitWrapper(bitStream, &audioMuxElement->useSameStreamMux, 1, WriteFlag);
    if ( !(audioMuxElement->useSameStreamMux) ) {
      return advanceStreamMuxConfig(stream, WriteFlag);
    }
  }
  return 0;
}

static int advanceAudioMuxElementP(HANDLE_STREAMFILE stream, int WriteFlag )
{
  AUDIO_MUX_ELEMENT *audioMuxElement = &stream->spec->audioMuxElement;
  int i,ret = 0;

  if ( audioMuxElement->streamMuxConfig.audioMuxVersionA == 0 ) {
    for ( i = 0; i < (int)(audioMuxElement->streamMuxConfig.numSubFrames+1); i++ ) {
      if (WriteFlag == 1) {
        ret = setupPayloadLengthInfo(stream); if (ret) return ret;
      }
      ret = advancePayloadLengthInfo(stream, WriteFlag); if (ret) return ret;
      
      ret = advancePayloadMux(stream, WriteFlag); if (ret) return ret;
    }
    if ( audioMuxElement->streamMuxConfig.otherDataPresent ) {
      /* read/write other data */
    }
  } else {
    CommonWarning("StreamFile:LATM: audioMuxVersionA 1 not defined/implemented");
    ret = -1;
  }

  return ret;
}

static int advanceAudioMuxElement(HANDLE_STREAMFILE stream, int muxConfigPresent, int WriteFlag )
{
  int ret;
  ret = advanceAudioMuxElementH(stream, muxConfigPresent, WriteFlag );
  if (ret) return ret;
  ret = advanceAudioMuxElementP(stream, WriteFlag );
  return ret;
}


static int read_latm_frame(HANDLE_STREAMFILE stream)
     /*AudioSyncStream() from ISO/IEC 14496-3 table 1.16 */
{
  HANDLE_BSBITSTREAM bitStream = stream->spec->bitStream;
  unsigned long latm_sync_word;
  unsigned long ed, st, ltmp, align, audioMuxLengthBytes;
  int err;

  if ( BsEof ( bitStream, 8) ) return -2;

  /* read audioSyncStream (streamMuxConfig) */
  BsGetBit(bitStream,&latm_sync_word,11);
  if ( latm_sync_word != ASS_SyncWord ) {
    CommonExit(1,"error read LATM sync word");
  }
  BsGetBit(bitStream,&audioMuxLengthBytes,13);

  st = BsCurrentBit(bitStream);
  err = advanceAudioMuxElementH(stream, 1, 0 );
  if (err) return err;

  setStreamStatus(stream,STATUS_READING);

  /* read audioSyncStream (PayloadMux) */
  err = advanceAudioMuxElementP(stream, 0 );
  if (err) return err;

  /* byte_align */
  ed = BsCurrentBit(bitStream);
  ltmp = ed - st;
  align = 8 -  ltmp % 8;
  if (align == 8) align = 0;

  if ((ltmp+align)/8 != audioMuxLengthBytes) {
    CommonWarning("StreamFile:LATM: audioMuxLengthBytes indicated %i bytes but got %i bytes",
                  audioMuxLengthBytes, (ltmp+align)/8);
  }

  BsGetBit(bitStream,&ltmp,align) ;

  return 0;
}


static int write_latm_frame(HANDLE_STREAMFILE stream)
     /*AudioSyncStream() from ISO/IEC 14496-3 table 1.16 */
{
  unsigned long ed, st, audioMuxLengthBytes, align;
  int err;
  HANDLE_BSBITSTREAM outBitStream = stream->spec->bitStream;
  HANDLE_BSBITBUFFER audioMuxBuffer = BsAllocBuffer((1<<13)<<3);
  HANDLE_BSBITSTREAM audioMuxBufferStream;

  /* prepare buffer to hold the audioMuxElement */
  if (audioMuxBuffer == NULL) {
    CommonWarning("StreamFile:LATM: Could not allocate a buffer");
    return -1;
  }
  audioMuxBufferStream = BsOpenBufferWrite(audioMuxBuffer);
  if (audioMuxBufferStream == NULL) {
    CommonWarning("StreamFile:LATM:Could not write into buffer");
    return -1;
  }

  /* write audioMuxElement into buffer */
  {
    int cur_frames = FIFObufferLength(stream->prog[0].programData->spec->AUbuffer[0])/StreamAUsPerFrame(&stream->prog[0],0);
    err = setupMuxConfig(stream, cur_frames);
    if (err) return err;
  }

  st = BsCurrentBit(audioMuxBufferStream);
  stream->spec->bitStream = audioMuxBufferStream;
  err = advanceAudioMuxElement(stream, 1/*muxConfigPresent*/, 1/*write*/);
  stream->spec->bitStream = outBitStream;
  if (err) return err;
  ed = BsCurrentBit(audioMuxBufferStream);
  BsClose(audioMuxBufferStream);

  /* byte alignment */
  audioMuxLengthBytes = ed - st;
  align = 8 - audioMuxLengthBytes % 8;
  if (align == 8) align = 0;
  audioMuxLengthBytes /= 8;
  if (align != 0) audioMuxLengthBytes += 1;

  /* write the data */
  BsPutBit(outBitStream,ASS_SyncWord,11);
  BsPutBit(outBitStream,audioMuxLengthBytes,13);
  BsPutBuffer(outBitStream, audioMuxBuffer);
  BsPutBit(outBitStream,0,align);

  /* clean up */
  BsFreeBuffer(audioMuxBuffer);

  return 0;
}

static int checkWriteCompleteFrame(HANDLE_STREAMFILE stream, int force)
{
  int i, j, write=1;
  int num, err = 0;
  if (force) 
    /* num = current frames in prog 0 track 0 */
    num = FIFObufferLength(stream->prog[0].programData->spec->AUbuffer[0])/StreamAUsPerFrame(&stream->prog[0], 0);
  else
    /* num = selected number of subframes */
    num = (int) stream->spec->audioMuxElement.streamMuxConfig.numSubFrames+1;
  if (num==0) write=0;
  for (i=0; i<stream->progCount; i++) {
    HANDLE_STREAMPROG prog = &stream->prog[i];
    for (j=0;j<(int)(prog->trackCount);j++) {
      if (FIFObufferLength(prog->programData->spec->AUbuffer[j]) <
          (StreamAUsPerFrame(prog, j) * num)) write = 0;
    }
  }
  if (write) err = write_latm_frame(stream);
  return err;
}


static int commandline(HANDLE_STREAMFILE stream, int argc, char**argv)
{
  int numSubFrames;
  int timing, timingUsed;
  int audioMuxVersion;
#ifdef CT_SBR
  int hierarchicalSignalling;
#endif

  CmdLineSwitch switchList[] = {
    {"n",NULL,"%i","1",NULL,"num sub-frames (1<=n<=MAXFRAME)"},
    {"t",NULL,"%i","1",NULL,"allStreamSameTiming (0/1)"},
#ifdef CT_SBR
    {"m",NULL,"%i","1",NULL,"audioMuxVersion (0/1)"},
    {"h",NULL,"%i","0",NULL,"SBR/PS hierarchical signalling (0/1)"},
#else
    {"m",NULL,"%i","0",NULL,"audioMuxVersion (0/1)"},
#endif
    {NULL,NULL,NULL,NULL,NULL,NULL}
  };
  CmdLinePara paraList[] = {
    {NULL,NULL,NULL}
  };

  switchList[0].argument = &numSubFrames;
  switchList[1].argument = &timing;
  switchList[1].usedFlag = &timingUsed;
  switchList[2].argument = &audioMuxVersion;
#ifdef CT_SBR
  switchList[3].argument = &hierarchicalSignalling;
#endif

  if (stream!=NULL) {
    if (CmdLineEval(argc,argv,paraList,switchList,1,NULL)) return -10;

    if (numSubFrames<1 || numSubFrames>MAXFRAME) return -10;
    stream->spec->audioMuxElement.streamMuxConfig.numSubFrames = numSubFrames-1;
    stream->spec->same_timing_requested = timing;
    stream->spec->audioMuxElement.streamMuxConfig.audioMuxVersion = audioMuxVersion;
#ifdef CT_SBR
    stream->spec->hierarchical_signalling = hierarchicalSignalling;
#endif
  } else {
    CmdLineHelp(NULL,paraList,switchList,stdout);
  }

  return 0;
}


/* --------------------------------------------------- */
/* ---- LATM                                      ---- */
/* --------------------------------------------------- */


static int LATMinitProgram(HANDLE_STREAMPROG prog)
{
  if ((prog->programData->spec = (struct tagProgSpecificInfo*)malloc(sizeof(struct tagProgSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initProgram: error in malloc");
    return -1;
  }
  memset(prog->programData->spec, 0, sizeof(struct tagProgSpecificInfo));
  return 0;
}


static int LATMopenRead(HANDLE_STREAMFILE stream)
{
  char *info;
  stream->spec->bitStream = BsOpenFileRead(stream->fileName,MP4_MAGIC,&info);
  if (stream->spec->bitStream==NULL) {
    DebugPrintf(1,"StreamFile:openRead(LATM): error opening bit stream for reading");
    return -1;
  }
  if (strcmp(info,bsopen_info_latm)) {
    DebugPrintf(4,"StreamFile:openRead(LATM): bitstream info does not indicate LATM");
    return -1;
  }
  stream->spec->first_frame = 1;

  read_latm_frame(stream);

  return 0;
}

static int LATMgetAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  HANDLE_STREAMFILE stream = prog->fileData;
  HANDLE_STREAM_AU tmpAU;
  int err;
  int i,j;

  if (FIFObufferLength(prog->programData->spec->AUbuffer[trackNr]) == 0) {
    /* no more AUs buffered in this stream... and in the other streams? */
    for (i=0; i<stream->progCount; i++) {
      HANDLE_STREAMPROG prg = &stream->prog[i];
      for (j=0;j<(int)(prg->trackCount);j++) {
        int AUsLeft = FIFObufferLength(prg->programData->spec->AUbuffer[trackNr]);
        if (AUsLeft!=0) {
          CommonWarning("StreamFile:getAU(LATM): did not read %i AccessUnits in prog %i, track %i. discarding due to read of new payload", AUsLeft, i, j);
        }
        while (AUsLeft--) {
          tmpAU = FIFObufferPop(prg->programData->spec->AUbuffer[trackNr]);
          StreamFileFreeAU(tmpAU);
        }
      }
    }
    /* read and buffer new AUs */
    err = read_latm_frame(stream);
    if (err) return err;
  }

  tmpAU = FIFObufferPop(prog->programData->spec->AUbuffer[trackNr]);
  if (tmpAU==NULL) {
    CommonWarning("StreamFile:getAU(LATM): empty AU buffered???");
    return -1;
  }
  
  err = StreamFile_AUcopyResize(au, tmpAU->data, tmpAU->numBits);
  StreamFileFreeAU(tmpAU);
  
  return err;
}


static int LATMopenWrite(HANDLE_STREAMFILE stream, int argc, char** argv)
{
  int result;
  char *info;

  /* - options parsing */

  result = commandline(stream, argc, argv);
  if (result) return result;

  /* - open bit stream file */

  info = (char*)bsopen_info_latm;

  stream->spec->bitStream = BsOpenFileWrite(stream->fileName,MP4_MAGIC,info);
  if (stream->spec->bitStream==NULL) {
    DebugPrintf(1,"StreamFile:openWrite(LATM): error opening bit stream for writing");
    return -1;
  }
  stream->spec->first_frame = 1;

  return 0;
}

static int LATMheaderWrite(HANDLE_STREAMFILE stream)
{
  int i,j;
  for (i=0; i<stream->progCount; i++) {
    HANDLE_STREAMPROG prog = &stream->prog[i];
    for (j=0; j<(int)(prog->trackCount); j++) {
      prog->programData->spec->AUbuffer[j] = FIFObufferCreate(MAXBUFFER);
    }
  }

  return 0;
}

static int LATMputAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  HANDLE_STREAM_AU destAU;
  int err = 0;

  destAU = StreamFileAllocateAU(au->numBits);
  err = StreamFile_AUcopyResize(destAU,au->data, au->numBits);

  if (err == 0) {
    if (FIFObufferPush(prog->programData->spec->AUbuffer[trackNr],destAU))
      CommonWarning("StreamFile:LATM: Error saving AU to buffer");
    err = checkWriteCompleteFrame(prog->fileData, 0);
  }

  return err;
}

static int LATMclose(HANDLE_STREAMFILE stream)
{
  int progIdx;
  unsigned long x;

  if (stream->status == STATUS_WRITING) {
    checkWriteCompleteFrame(stream, 1);
  }

  for (progIdx=0; progIdx<stream->progCount; progIdx++) {
    HANDLE_STREAMPROG prog = &stream->prog[progIdx];
    for (x=0;x<prog->trackCount;x++) {
      HANDLE_STREAM_AU tmpAU;
      while ((tmpAU = FIFObufferPop(prog->programData->spec->AUbuffer[x]))) {
        StreamFile_AUfree(tmpAU);
      }
      FIFObufferFree(prog->programData->spec->AUbuffer[x]);
    }
  }

  return BsClose(stream->spec->bitStream);
}


/* --------------------------------------------------- */
/* ---- Constructor                               ---- */
/* --------------------------------------------------- */

int LATMinitStream(HANDLE_STREAMFILE stream)
{
  stream->initProgram=LATMinitProgram;
  stream->openRead=LATMopenRead;
  stream->openWrite=LATMopenWrite;
  stream->headerWrite=LATMheaderWrite;
  stream->close=LATMclose;
  stream->getAU=LATMgetAccessUnit;
  stream->putAU=LATMputAccessUnit;

  if ((stream->spec = (struct tagStreamSpecificInfo*)malloc(sizeof(struct tagStreamSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initStream: error in malloc");
    return -1;
  }
  memset(stream->spec, 0, sizeof(struct tagStreamSpecificInfo));
  return 0;
}


void LATMshowHelp( void )
{
  printf(MODULE_INFORMATION);
  commandline(NULL,0,NULL);
}
