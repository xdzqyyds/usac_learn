/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Heiko Purnhagen     (University of Hannover / ACTS-MoMuSys)

and edited by

Markus Werner       (SEED / Software Development Karlsruhe) 
Olaf Kaehler        (Fraunhofer IIS-A)

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

Copyright (c) 2000.

$Id: mp4ifc.c,v 1.5 2012-03-13 11:33:37 frd Exp $
Decoder frame work
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "streamfile.h"
#include "ep_convert.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "hvxc_struct.h"	       /* structs */


#include "dec_tf.h"
#include "dec_lpc.h"
#include "dec_par.h"

#include "bitstream.h"
#include "common_m4a.h"
#include "mp4ifc.h"

#include "audio.h"      /* audio i/o module */
#include "mp4au.h"
#include "port.h"
#include "decifc.h"
#ifdef  DEBUGPLOT
#include "plotmtv.h"
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define MAX_AU 12
#define MAX_TRACKS_PER_LAYER 50

extern int bWriteIEEEFloat; /* defined in audio.c */

static int MP4Audio_SetupDecoders(DEC_DATA               *decData,
                                  DEC_DEBUG_DATA         *decDebugData,
                                  int                     streamCount,     /* in: number of streams decoded */
                                  char                   *decPara,         /* in: decoder parameter string */
                                  char                   *drc_payload_fileName,
                                  char                   *drc_config_fileName,
                                  char                   *loudness_config_fileName,
                                  int                    *streamID,
                                  HANDLE_DECODER_GENERAL  hFault,
                                  int                     mainDebugLevel,
                                  int                     audioDebugLevel,
                                  int                     epDebugLevel,
                                  char                   *aacDebugString
#ifdef CT_SBR
                                  ,int                    HEaacProfileLevel
                                  ,int                    bUseHQtransposer
#endif
#ifdef HUAWEI_TFPP_DEC
                                  ,int                    actATFPP
#endif
);

static int frameDataAddAccessUnit(DEC_DATA *decData,
                                  HANDLE_STREAM_AU au,
                                  int track);

static T_DECODERGENERAL faultData[MAX_TF_LAYER];
static HANDLE_DECODER_GENERAL hFault = &faultData[0];
static AudioFile *audioFile = NULL;
static int setAprInfo(AUDIOPREROLL_INFO        *aprInfo);
static int setElstInfo(ELST_INFO               *elstInfo,
                       const double             startOffset,
                       const double             durationTotal,
                       const long               numOutSamples,
                       const unsigned long      framesDone,
                       const float              fSampleOut,
                       const int                useEditlist
);

#ifdef DEBUGPLOT
extern  int framePlot;
#endif

static int usac_core_parsePrerollData(HANDLE_STREAM_AU        au,
                                      HANDLE_STREAM_AU*       prerollAU,
                                      unsigned int*           numPrerollAU,
                                      AUDIO_SPECIFIC_CONFIG*  prerollASC,
                                      unsigned int*           prerollASCLength,
                                      unsigned int*           applyCrossFade
) {
  HANDLE_BSBITBUFFER bb = NULL;
  HANDLE_BSBITSTREAM bs;

  int retVal                          = 1;
  int nLFE                            = 0;
  int ascBitCounter                   = 0;  
  unsigned int i                      = 0;
  unsigned int j                      = 0;
  unsigned long indepFlag             = 0;
  unsigned long extPayloadPresentFlag = 0;
  unsigned long useDefaultLengthFlag  = 0;
  unsigned long dummy                 = 0;
  unsigned long auByte                = 0;
  unsigned int ascSize                = 0;
  unsigned int auLength               = 0;
  unsigned int totalPayloadLength     = 0;
  unsigned int nBitsToSkip            = 0; 

  bb = BsAllocPlainDirtyBuffer(au->data, au->numBits);
  bs = BsOpenBufferRead(bb);

  *applyCrossFade = 0;

  /* Indep flag must be one */
  BsGetBit(bs, &indepFlag, 1);
  if (!indepFlag) {
     goto freeMem;
  }
 
  /* Payload present flag must be one */
  BsGetBit(bs, &extPayloadPresentFlag, 1);
  if (!extPayloadPresentFlag) {
     goto freeMem;
  }

  /* Default length flag must be zero */
  BsGetBit(bs, &useDefaultLengthFlag, 1);
  if (useDefaultLengthFlag) {
     goto freeMem;
  }

  /* Seems to be a valid preroll extension */
  retVal = 0;

  /* read overall ext payload length */
  UsacConfig_ReadEscapedValue(bs, &totalPayloadLength, 8, 16, 0);

  /* read ASC size */
  UsacConfig_ReadEscapedValue(bs, &ascSize, 4, 4, 8);
  *prerollASCLength = ascSize;

  /* read ASC */
  if (ascSize) {
    ascBitCounter = UsacConfig_Advance(bs, &(prerollASC->specConf.usacConfig), 0);
    prerollASC->channelConfiguration.value = usac_get_channel_number(&prerollASC->specConf.usacConfig, &nLFE);

    /* Skip remaining bits from ASC that were not parsed */
    nBitsToSkip =  ascSize * 8 - ascBitCounter;

    while (nBitsToSkip) {
      int nMaxBitsToRead = min(nBitsToSkip, LONG_NUMBIT);
      BsGetBit(bs, &dummy, nMaxBitsToRead);
      nBitsToSkip -= nMaxBitsToRead;
    }
  }

  BsGetBit(bs, (unsigned long*)applyCrossFade, 1);
  BsGetBit(bs, &dummy, 1);

  /* read num preroll AU's */
  UsacConfig_ReadEscapedValue(bs, numPrerollAU, 2, 4, 0);
  assert(*numPrerollAU <= MPEG_USAC_CORE_MAX_AU);

  for (i = 0; i < *numPrerollAU; ++i) {
    /* For every AU get length and allocate memory  to hold the data */
    UsacConfig_ReadEscapedValue(bs, &auLength, 16, 16, 0);
    prerollAU[i] = StreamFileAllocateAU(auLength * 8);

    for (j = 0; j < auLength; ++j) {
      /* Read AU bytewise and copy into AU data buffer */
      BsGetBit(bs, &auByte, 8);
      memcpy(&(prerollAU[i]->data[j]), &auByte, 1); 
    }
  }

freeMem:
  BsCloseRemove(bs, 0);
  if (bb) {
    free(bb);
  }

  return retVal;
}

int MP4Audio_ProbeFile (char *inFile)
{
  HANDLE_STREAMFILE file = NULL;
  file = StreamFileOpenRead ( inFile, FILETYPE_AUTO ) ; 
  if (file != NULL) 
  { 
    StreamFileClose (file) ; 
    return 1 ;
  } 
  else 
    return 0 ;
}

int MP4Audio_DecodeFile(char*                 inFile,          /* in: bitstream input file name */
                        char*                 audioFileName,   /* in: audio file name */
                        char*                 audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                        int                   int24flag,
                        int                   monoFilesOut,
                        int                   maxLayer,        /* in  max layers */
                        char*                 decPara,         /* in: decoder parameter string */
                        int                   mainDebugLevel,
                        int                   audioDebugLevel,
                        int                   epDebugLevel,
                        int                   bitDebugLevel,
                        char*                 aacDebugString,
#ifdef CT_SBR
                        int                   HEaacProfileLevel,
                        int                   bUseHQtransposer,
#endif
#ifdef HUAWEI_TFPP_DEC
                        int                   actATFPP,
#endif
                        int                   programNr,
                        DRC_APPLY_INFO       *drcInfo,
                        AUDIOPREROLL_INFO    *aprInfo,
                        ELST_INFO            *elstInfo,
                        const ELST_MODE_LEVEL elstInfoModeLevel,
                        int                  *streamID,
                        const CORE_MODE_LEVEL decoderMode,
                        const int             verboseLevel
) {
  int err = 0;
  unsigned int i = 0;
  unsigned long framesDone;
  int frameCnt = 0;
  int nAudioPreRollSamplesWrittenPerChannel = 0;
  int AudioPreRollExisting                  = 0;
  int frameCntAudioPreRoll                  = 0;
  int discardAudioPreRoll                   = (CORE_STANDALONE_MODE == decoderMode) ? 1 : 0;  /* in stand-alone operation mode the AudioPreRoll frames will be discarded */
  int fileOpened = 0;
  int numFC = 0;
  int fCenter = 0;
  int numSC = 0; 
  int numBC = 0;
  int bCenter = 0;
  int numLFE = 0;
  int channelNum = 0, tmp;
  AUDIO_SPECIFIC_CONFIG* asc;
  DEC_DATA           *decData = NULL;
  DEC_DEBUG_DATA     *decDebugData = NULL;
  HANDLE_STREAMFILE stream = NULL;
  HANDLE_STREAMPROG prog = NULL;
  int  suitableTracks = 0;
  int  trackIdx;
  float **outSamples;
  long  numOutSamples;
  char buffer_empty;
  int numChannelOut;
  float fSampleOut = 0.0f;
  int firstDecdoeFrame = 1; /* SAMSUNG_2005-09-30 */
  int progStart, progStop, progCnt;
  int useEditlist[MAX_TRACKS_PER_LAYER] = {0};
  double startOffset[MAX_TRACKS_PER_LAYER] = {0.0};
  double durationTotal[MAX_TRACKS_PER_LAYER] = {0.0};
  long startOffsetInSamples[MAX_TRACKS_PER_LAYER] = {0};
  long playTimeInSamples[MAX_TRACKS_PER_LAYER] = {0};

  /* reduce internal processing chain to 24 bit resolution if requested*/
  if (int24flag == 1) {
    bWriteIEEEFloat = 0;
  }

  /* initialize AudioPreRoll Info struct */
  setAprInfo(aprInfo);

  /* initialize Elst Info struct */
  setElstInfo(elstInfo,
              0.0,
              0.0,
              0,
              0,
              0.0f,
              0);

  /* initialize hFault */
  memset(hFault, 0, MAX_TF_LAYER * sizeof(T_DECODERGENERAL));

  BsInit (0,bitDebugLevel,0);

  stream = StreamFileOpenRead( inFile, FILETYPE_AUTO ); if (stream==NULL) goto bail;
   
  if ( programNr != -1 ) {      /* specific program selected */
    progStart = programNr;
    progStop = programNr+1;
    DebugPrintf(1,"Selected program: %d",progStart);
  } else {                      /* no specific program selected -> loop over all programs */
    progStart = 0;
    progStop = StreamFileGetProgramCount( stream );
    if ( progStop > 1 ) {       /* more than one program in stream */
        DebugPrintf(1,"No specific program selected -> looping over all programs");
    }
    DebugPrintf(1,"\nNumber of programs: %d",progStop);
  }
  
  for ( progCnt = progStart; progCnt < progStop; progCnt++ ) {
    programNr = progCnt;
    DebugPrintf(1,"Decoding program: %d",programNr);
     
    prog = StreamFileGetProgram( stream, programNr ); if (prog==NULL) goto bail;
    suitableTracks = prog->trackCount;
  
  
    DebugPrintf(1,"\nFound MP4 file with  %d suitable Tracks  ",suitableTracks);

    if (!suitableTracks) {
      return -1 ;
    }

    asc = &(prog->decoderConfig[0].audioSpecificConfig);

    switch (asc->audioDecoderType.value) {
#ifdef AAC_ELD
      case ER_AAC_ELD:
        channelNum = asc->channelConfiguration.value;
        break;
#endif

      case USAC:
        channelNum = get_channel_number_USAC(&asc->specConf.usacConfig,
                                             &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
        break;
      default:
        if (asc->channelConfiguration.value == 0) {
          select_prog_config(asc->specConf.TFSpecificConfig.progConfig,
                             (asc->specConf.TFSpecificConfig.frameLength.value ? 960:1024),
                             hFault->hResilience, 
                             hFault->hEscInstanceData,
                             aacDebugString['v']);
        }
        channelNum = get_channel_number(asc->channelConfiguration.value,
                                        asc->specConf.TFSpecificConfig.progConfig,
                                        &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
        break;
    }

    if (channelNum > 2) {
      CommonWarning("No standards for putting %ld Channels into 1 File -> Using multiple files\n", channelNum);
      monoFilesOut = 1;
    }

    /*
      instantiate proper decoder for decoderconfig
    */

    decData = (DEC_DATA *)calloc(1,sizeof(DEC_DATA));
    decDebugData=(DEC_DEBUG_DATA *)calloc(1,sizeof(DEC_DEBUG_DATA));

    /* suitableTracks was initally total no of tracks
       this function stores total no of tracks in decData and
       returns no of tracks needed for decoding up to mayLayer */
    tmp = asc->channelConfiguration.value;
    asc->channelConfiguration.value = channelNum;

    suitableTracks=frameDataInit(prog->allTracks,
                                 suitableTracks,
                                 maxLayer,
                                 prog->decoderConfig,
                                 decData);
    asc->channelConfiguration.value = tmp;

    if (suitableTracks <= 0) {
      CommonWarning("Error during framework initialisation");
      goto bail;
    }

    /* get editlist info */
    for (trackIdx = 0; trackIdx < suitableTracks; trackIdx++) {
      double tmpStart, tmpDuration;

      useEditlist[trackIdx] = 0;
      if (0 == StreamFileGetEditlist(prog, trackIdx, &tmpStart, &tmpDuration)) {
        startOffset[trackIdx]   = tmpStart;
        durationTotal[trackIdx] = tmpDuration;
        if (tmpDuration > -1) {
          useEditlist[trackIdx] = 1;
        }
      }
    }

    if (verboseLevel > 0) {
      fprintf(stdout, "\nDecoding ...\n");
    }

    /* decoding loop */
    for (framesDone = 0, buffer_empty = 0; buffer_empty == 0; framesDone++) {
      int tracksForDecoder;
      AUDIO_SPECIFIC_CONFIG prerollASC;
      HANDLE_STREAM_AU inputAUs[MAX_TRACKS_PER_LAYER];
      HANDLE_STREAM_AU decoderAUs[MAX_TRACKS_PER_LAYER];
      HANDLE_STREAM_AU prerollAU[MPEG_USAC_CORE_MAX_AU];
      unsigned int prerollASCLength = 0;
      unsigned int applyCrossFade   = 0;
#ifdef DEBUGPLOT
      plotInit(framePlot,audioFileName,0);
#endif

      {
        int layer, track;
        int firstTrackInLayer, resultTracksInLayer;
        int err = 0;
        for (track=0; track<MAX_TRACKS_PER_LAYER; track++) {
          inputAUs[track]=StreamFileAllocateAU(0);
          decoderAUs[track]=StreamFileAllocateAU(0);
        }
        firstTrackInLayer = tracksForDecoder = 0;
        /* go through each decoded layer */
        for (layer=0; layer<(signed)decData->frameData->scalOutSelect+1; layer++) {
          /* go through all frames in the superframe */
          resultTracksInLayer = EPconvert_expectedOutputClasses(decData->frameData->ep_converter[layer]);
          while ((signed)decData->frameData->layer[tracksForDecoder].NoAUInBuffer<StreamAUsPerFrame(prog, firstTrackInLayer)) {
            int epconv_input_tracks;
            if (EPconvert_numFramesBuffered(decData->frameData->ep_converter[layer])) {
              epconv_input_tracks=0;
            } else {
              epconv_input_tracks=decData->frameData->tracksInLayer[layer];
            }

            /* go through all tracks in this layer */
            for (track=0; track<epconv_input_tracks; track++) {
              err = StreamGetAccessUnit( prog, firstTrackInLayer+track, inputAUs[track] );
              if ( err ) break;
            }
            if ( err ) break;

            /* decode with epTool if necessary */
            err = EPconvert_processAUs(decData->frameData->ep_converter[layer],
                                       inputAUs, epconv_input_tracks,
                                       decoderAUs, MAX_TRACKS_PER_LAYER);

            if (resultTracksInLayer!=err) {
              CommonWarning("Expected %i AUs after ep-conversion, but got %i AUs", resultTracksInLayer, err);
              /*resultTracksInLayer=err;*/
            }
            err=0;

            /* save the tracks */
            for (track=0; track<resultTracksInLayer; track++) {
              frameDataAddAccessUnit(decData, decoderAUs[track], tracksForDecoder+track);
            }
          }
          if (err) break;
          firstTrackInLayer += decData->frameData->tracksInLayer[layer];
          tracksForDecoder += resultTracksInLayer;
        }
        buffer_empty=1;
        for (track=0; track<(signed)decData->frameData->od->streamCount.value; track++) {
          if (decData->frameData->layer[track].NoAUInBuffer>0) buffer_empty=0;
        }
        if (buffer_empty) 
          break;
      }

      if (framesDone == 0) {
        int delay, track;

        delay = MP4Audio_SetupDecoders(decData,
                                       decDebugData,
                                       tracksForDecoder,
                                       decPara,
                                       drcInfo->uniDrc_payload_file,
                                       drcInfo->uniDrc_config_file,
                                       drcInfo->loudnessInfo_config_file,
                                       streamID,
                                       hFault,
                                       mainDebugLevel,
                                       audioDebugLevel,
                                       epDebugLevel,
                                       aacDebugString
#ifdef CT_SBR
                                      ,HEaacProfileLevel
                                      ,bUseHQtransposer
#endif
#ifdef HUAWEI_TFPP_DEC
                                      ,actATFPP
#endif
                                       );

        /* decoder initialisation will update streamCount in decData and
           print an error if decoder initialisation does not agree with track count
           determined earlier by frameDataInit() and stored in suitableTracks */

        fSampleOut = (float) decData->frameData->scalOutSamplingFrequency;
        numChannelOut = decData->frameData->scalOutNumChannels ;
      
        if (ELST_MODE_CORE_LEVEL == elstInfoModeLevel) {
          for (track = 0; track < (signed)decData->frameData->od->streamCount.value; track++) {
            if (useEditlist[track]) {
#ifndef ALIGN_PRECISION_NOFIX
              double tmpStartOffset = startOffset[track]   * fSampleOut + 0.5;
              double tmpPlayTime    = durationTotal[track] * fSampleOut + 0.5;

              startOffsetInSamples[track] = (long)(tmpStartOffset);
              playTimeInSamples[track]    = (long)(tmpPlayTime);
#else
              startOffsetInSamples[track] = (long)(startOffset[track]   * fSampleOut + 0.5);
              playTimeInSamples[track]    = (long)(durationTotal[track] * fSampleOut + 0.5);
#endif
            }
          }
        }

        /* open audio file */
        /* (sample format: 16 bit twos complement, uniform quantisation) */
        if (fileOpened == 0) {
      
        if (monoFilesOut==0) {
          audioFile = AudioOpenWrite(audioFileName,
                                     audioFileFormat,
                                     numChannelOut,
                                     fSampleOut,
                                     int24flag);
        } else {
          audioFile = AudioOpenWriteMC(audioFileName,
                                       audioFileFormat,
                                       fSampleOut,
                                       int24flag,
                                       numFC,
                                       fCenter,
                                       numSC,
                                       bCenter,
                                       numBC,
                                       numLFE);
        }
      
        if (audioFile==NULL)
          CommonExit(1,"Decode: error opening audio file %s "
                     "(maybe unknown format \"%s\")",
                     audioFileName,audioFileFormat);

        /* seek to beginning of first frame (with delay compensation) */
        AudioSeek(audioFile,-delay);

        fileOpened=1;
        }
      }

      /*
        send AU to decoder
      */
      if ((mainDebugLevel == 1) && (framesDone % 20 == 0)) {
        fprintf(stderr,"\rFrame [%ld] ",framesDone);
        fflush(stderr);
      }
      if (mainDebugLevel >= 2 && mainDebugLevel <= 3) {
        printf("\rFrame [%ld] ",framesDone);
        fflush(stdout);
      }
      if (mainDebugLevel > 3)
        printf("Frame [%ld]\n",framesDone);

      if (framesDone == 0) {
        int retVal                       = 0;
        int instance                     = 0;                                               /* AudioPreRoll() shall be the first element in the AU! */
        int track                        = decData->frameData->od->streamCount.value - 1;   /* USAC only supports one track! */
        AUDIO_SPECIFIC_CONFIG *pUsacASC  = &decData->frameData->od->ESDescriptor[track]->DecConfigDescr.audioSpecificConfig;
        USAC_DECODER_CONFIG *pUsacConfig = &pUsacASC->specConf.usacConfig.usacDecoderConfig;
        USAC_EXT_CONFIG *pUsacExtConfig  = &pUsacConfig->usacElementConfig[instance].usacExtConfig;
        
        if (USAC_ID_EXT_ELE_AUDIOPREROLL == pUsacExtConfig->usacExtElementType) {
          /* Prepare preroll-ASC layout - needed to have the correct number of bits for every entry */
          memcpy(&prerollASC, pUsacASC, sizeof(AUDIO_SPECIFIC_CONFIG));

          /* Parse preroll data if present */
          retVal = usac_core_parsePrerollData(decoderAUs[track],
                                              prerollAU,
                                              &aprInfo->numPrerollAU,
                                              &prerollASC,
                                              &prerollASCLength,
                                              &applyCrossFade);

          if (0 != retVal) {
            AudioPreRollExisting = 0;
          } else {
            AudioPreRollExisting = 1;

            /* decoder Pre Roll AU's  */
            for (i = 0; i < aprInfo->numPrerollAU; i++) {
              BsBufferSetData(&decData->frameData->layer[0],
                              prerollAU[i]->data,
                              prerollAU[i]->numBits);

              audioDecFrame(decData,
                            hFault,
                            &outSamples,
                            &numOutSamples,
                            &numChannelOut); 

              AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, 0);

              nAudioPreRollSamplesWrittenPerChannel += numOutSamples;
              frameCntAudioPreRoll++;
            }

            /* Put the IPF AU back into our decData instance and continue decoding as normal. */
            BsBufferSetData(&decData->frameData->layer[0],
                            decoderAUs[track]->data,
                            decoderAUs[track]->numBits);

            aprInfo->nSamplesCoreCoderPreRoll = nAudioPreRollSamplesWrittenPerChannel;
          }
        }

        startOffsetInSamples[0] += aprInfo->nSamplesCoreCoderPreRoll;
      }
      
      audioDecFrame(decData,
                    hFault,
                    &outSamples,
                    &numOutSamples,
                    &numChannelOut);

      /* SAMSUNG_2005-09-30 : In BSAC MC, final out channel number is determined after 1 frame decoding.  */
      if(numChannelOut != decData->frameData->scalOutNumChannels && firstDecdoeFrame)
        {
          AudioClose(audioFile);
          /*  fixing 7.1 channel error */
          if(decData->frameData->scalOutObjectType == ER_BSAC)
          {
            channelNum = get_channel_number_BSAC(numChannelOut, /* SAMSUNG_2005-09-30 : Warnning ! it may not be same with asc->channelConfiguration.value. */
                                                 asc->specConf.TFSpecificConfig.progConfig,
                                                 &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);

          }
          else
          {
            switch (asc->audioDecoderType.value) {
#ifdef AAC_ELD
              case ER_AAC_ELD:
              channelNum = asc->channelConfiguration.value;
              break;
#endif
              default:
              channelNum = get_channel_number(numChannelOut, /* SAMSUNG_2005-09-30 : Warnning ! it may not be same with asc->channelConfiguration.value. */
                                              asc->specConf.TFSpecificConfig.progConfig,
                                              &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
              break;
            }
          }

          if (channelNum > 2) {
            CommonWarning("No standards for putting %ld Channels into 1 File -> Using multiple files\n", channelNum);
            monoFilesOut = 1;
          }

          if (monoFilesOut==0) {
            audioFile = AudioOpenWrite(audioFileName,
                                       audioFileFormat,
                                       numChannelOut,
                                       fSampleOut,
                                       int24flag);
          } else {
            /* SAMSUNG 2005-10-18 */
            if(decData->frameData->scalOutObjectType == ER_BSAC)
              audioFile = AudioOpenWriteMC_BSAC(audioFileName,
                                                audioFileFormat,
                                                fSampleOut,
                                                int24flag,
                                                numFC,
                                                fCenter,
                                                numSC,
                                                bCenter,
                                                numBC,
                                                numLFE);
            else
              audioFile = AudioOpenWriteMC(audioFileName,
                                           audioFileFormat,
                                           fSampleOut,
                                           int24flag,
                                           numFC,
                                           fCenter,
                                           numSC,
                                           bCenter,
                                           numBC,
                                           numLFE);
          }
          firstDecdoeFrame = 0;
        }
  /* ~SAMSUNG_2005-09-30 */

#ifdef CT_SBR
      /* The implicit signalling....*/

      if (decData->tfData != NULL) {
        static int reInitAudioOutput = 1;

        if (decData->tfData->runSbr == 1 &&             /* If zero, we know that no implicit signalling was detected*/
            decData->tfData->sbrPresentFlag != 0 &&     /* If zero, we know no SBR is present.*/
            decData->tfData->sbrPresentFlag != 1 &&     /* If one , we know for sure SBR is present, and already set up the output correctly.*/
            reInitAudioOutput ) {

          /* check sampling frequency just for AAC_LC and AAC_SCAL ( other aot are not supported at the moment ) */
          /* 20070326 BSAC Ext.*/
          if ( (decData->frameData->scalOutObjectType == AAC_LC) || (decData->frameData->scalOutObjectType == AAC_SCAL) || (decData->frameData->scalOutObjectType == ER_BSAC) ) {
          /* 20070326 BSAC Ext.*/

            /* get current output sampling frequency */
            float outputSamplingFreq = AudioGetSamplingFreq( audioFile );

            /* calculate favoured output sbr frequency */ 
            float sbrSamplingFreq;

            if(decData->tfData->bDownSampleSbr){
              sbrSamplingFreq = (float) prog->decoderConfig[suitableTracks-1].audioSpecificConfig.samplingFrequency.value;
            }
            else{
              sbrSamplingFreq = (float) prog->decoderConfig[suitableTracks-1].audioSpecificConfig.samplingFrequency.value * 2;
            }


            if ( outputSamplingFreq != sbrSamplingFreq ) {

              reInitAudioOutput = 0;

              /* reinit audio writeout */
              AudioClose(audioFile);
              if (monoFilesOut==0) {
                audioFile = AudioOpenWrite(audioFileName,
                                           audioFileFormat,
                                           channelNum,
                                           sbrSamplingFreq,
                                           int24flag);
              } else if(decData->frameData->scalOutObjectType == ER_BSAC) {
                audioFile = AudioOpenWriteMC_BSAC(audioFileName,
                                                  audioFileFormat,
                                                  sbrSamplingFreq,
                                                  int24flag,
                                                  numFC,
                                                  fCenter,
                                                  numSC,
                                                  bCenter,
                                                  numBC,
                                                  numLFE);
              } else {
                audioFile = AudioOpenWriteMC(audioFileName,
                                             audioFileFormat,
                                             sbrSamplingFreq,
                                             int24flag,
                                             numFC,
                                             fCenter,
                                             numSC,
                                             bCenter,
                                             numBC,
                                             numLFE);
              }
            }
          }
        }
      }
#endif
    
      if (ELST_MODE_CORE_LEVEL == elstInfoModeLevel)
      {
        int writeoutOn = 0;

#ifdef I2R_LOSSLESS
        if ((decData->frameData->scalOutObjectType == SLS) || (decData->frameData->scalOutObjectType == SLS_NCORE)) {
          if (framesDone > 1) {
            writeoutOn = 1;      /* for SLS skip 1st 2 frames (to chk why?) */
          }
        } else
#endif
        {
          writeoutOn = 1;      /* normally write out all frames */
        }

        if (writeoutOn == 1) {
          long skipSamples = 0;

          if (useEditlist[0]) {
            if (numOutSamples < startOffsetInSamples[0]) {
              skipSamples = numOutSamples;
            } else {
              skipSamples = startOffsetInSamples[0];

              if (numOutSamples > playTimeInSamples[0]) {
                numOutSamples = playTimeInSamples[0];
              }
            }

            startOffsetInSamples[0] -= skipSamples;
            playTimeInSamples[0]    -= (numOutSamples - skipSamples);
          }

          if (decData->frameData->scalOutObjectType == USAC) {
            AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, skipSamples);
          } else {
            AudioWriteData(audioFile, outSamples, numOutSamples, skipSamples);
          }
        }
      }
      else {
        if (decData->frameData->scalOutObjectType == USAC) {
          AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, 0);
        } else {
          AudioWriteData(audioFile, outSamples, numOutSamples, 0);
        }
      }

#ifdef DEBUGPLOT
      framePlot++;
      plotDisplay(0);
#endif
      if (verboseLevel > 0) {
        fprintf(stdout, "Decoding frame %3d\r", frameCnt++);
      }
    }

    if (ELST_MODE_EXTERN_LEVEL == elstInfoModeLevel) {
      setElstInfo(elstInfo,
                  startOffset[0],
                  durationTotal[0],
                  numOutSamples,
                  framesDone,
                  fSampleOut,
                  useEditlist[0]);
    }

    if (verboseLevel > 0) {
      if ((frameCntAudioPreRoll > 0) && (0 == discardAudioPreRoll)) {
        fprintf(stdout, "Finished - (%d frames + %d Audio Pre-Roll frames)\n", frameCnt++, frameCntAudioPreRoll++);
      } else {
        fprintf(stdout, "Finished - (%d frames)\n\n", frameCnt++);
      }
    }

      /* transport MPEG-D DRC data */
      if (decData->usacData != NULL) {
        drcInfo->uniDrc_extEle_present          = decData->usacData->drcInfo.uniDrc_extEle_present;
        drcInfo->loudnessInfo_configExt_present = decData->usacData->drcInfo.loudnessInfo_configExt_present;
        drcInfo->frameLength                    = decData->usacData->drcInfo.frameLength;
        drcInfo->sbr_active                     = decData->usacData->drcInfo.sbr_active;
      }


    audioDecFree(decData,hFault);

    free(decData);
    free(decDebugData);

  }     /* program loop closed */
  
  bail:
 
  if (audioFile) {
    AudioClose(audioFile);
  }
  if (stream) {
    StreamFileClose(stream);
  }

  return err;
}


static int MP4Audio_SetupDecoders(DEC_DATA               *decData,
                                  DEC_DEBUG_DATA         *decDebugData,
                                  int                     streamCount,     /* in: number of streams decoded */
                                  char                   *decPara,         /* in: decoder parameter string */
                                  char                   *drc_payload_fileName,
                                  char                   *drc_config_fileName,
                                  char                   *loudness_config_fileName,
                                  int                    *streamID,
                                  HANDLE_DECODER_GENERAL  hFault,
                                  int                     mainDebugLevel,
                                  int                     audioDebugLevel,
                                  int                     epDebugLevel,
                                  char                   *aacDebugString
#ifdef CT_SBR
                                  ,int                    HEaacProfileLevel
                                  ,int                    bUseHQtransposer
#endif
#ifdef HUAWEI_TFPP_DEC
                                  ,int                    actATFPP
#endif
) {
  int delayNumSample = 0;

  AudioInit(NULL, audioDebugLevel);

  /*
    init decoders
  */

  decDebugData->aacDebugString = aacDebugString;
  decDebugData->decPara = decPara;
  decDebugData->infoFileName=NULL;
  decDebugData->mainDebugLevel=mainDebugLevel;
  decDebugData->epDebugLevel=epDebugLevel;
  
  delayNumSample = audioDecInit(hFault,
                                decData,
                                decDebugData,
                                drc_payload_fileName,
                                drc_config_fileName,
                                loudness_config_fileName,
                                streamID,
#ifdef CT_SBR
                                HEaacProfileLevel,
                                bUseHQtransposer,
#endif
                                streamCount
#ifdef HUAWEI_TFPP_DEC
                               ,actATFPP
#endif
                                );

  return delayNumSample;
}


static int frameDataAddAccessUnit(DEC_DATA *decData,
                                  HANDLE_STREAM_AU au,
                                  int track)
{
  unsigned long unitSize, unitPad;
  HANDLE_BSBITBUFFER bb;
  HANDLE_BSBITSTREAM bs;
  HANDLE_BSBITBUFFER AUBuffer  = NULL;
  int idx = track;

  unitSize = (au->numBits+7)/8;
  unitPad = (unitSize*8) - au->numBits;

  bb = BsAllocPlainDirtyBuffer(au->data, /*au->numBits*/ unitSize<<3);
  bs = BsOpenBufferRead(bb);

  AUBuffer = decData->frameData->layer[idx].bitBuf ;
  if (AUBuffer!=0 ) {
    if ((BsBufferFreeBits(AUBuffer)) > (long)unitSize*8 ) {
      int auNumber = decData->frameData->layer[idx].NoAUInBuffer;
      BsGetBufferAppend(bs, AUBuffer, 1, unitSize*8);
      decData->frameData->layer[idx].AULength[auNumber]= unitSize*8;
      decData->frameData->layer[idx].AUPaddingBits[auNumber]= unitPad;
      decData->frameData->layer[idx].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */       
            /* each decoder must remove the first element in the list  by shifting the elements down */
      DebugPrintf(6,"AUbuffer %2i: %i AUs, last size %i, lastPad %i\n",idx, decData->frameData->layer[idx].NoAUInBuffer, decData->frameData->layer[idx].AULength[auNumber], decData->frameData->layer[idx].AUPaddingBits[auNumber]);
    } else {
      BsGetSkip(bs,unitSize*8);
      CommonWarning (" input buffer overflow for layer %d ; skipping next AU",idx);
    }
  } else {
    BsGetSkip(bs,unitSize*8);
  }
  BsCloseRemove (bs, 0);
  free(bb);
  return 0;
}

static int setAprInfo(AUDIOPREROLL_INFO        *aprInfo
) {
  int retVal = 0;

  if (NULL == aprInfo) {
    retVal = 1;
  } else {
    aprInfo->numPrerollAU             = 0;
    aprInfo->nSamplesCoreCoderPreRoll = 0;
  }

  return retVal;
}

static int setElstInfo(ELST_INFO               *elstInfo,
                       const double             startOffset,
                       const double             durationTotal,
                       const long               numOutSamples,
                       const unsigned long      framesDone,
                       const float              fSampleOut,
                       const int                useEditlist
) {
  int retVal = 0;

  if (NULL == elstInfo) {
    retVal = 1;
  } else {
    if (useEditlist == 1) {
      unsigned long tmpFileLength = numOutSamples * framesDone;
      double tmpStartOffset       = startOffset   * fSampleOut + 0.5;
      double tmpPlayTime          = durationTotal * fSampleOut + 0.5;

      elstInfo->truncLeft        = (long)(tmpStartOffset);
      elstInfo->totalSamplesElst = (long)(tmpPlayTime);
      elstInfo->totalSamplesFile = tmpFileLength;

      if (elstInfo->totalSamplesElst <= elstInfo->totalSamplesFile) {
        elstInfo->truncRight     = ((elstInfo->totalSamplesFile-elstInfo->truncLeft) - elstInfo->totalSamplesElst);
      } else {
        /* warning: edit list ist longer than actual file length */
        elstInfo->truncRight     = 0;
      }
    } else {
      elstInfo->truncLeft        = 0;
      elstInfo->truncRight       = 0;
      elstInfo->totalSamplesElst = 0;
      elstInfo->totalSamplesFile = 0;
    }
  }

  return retVal;
}
