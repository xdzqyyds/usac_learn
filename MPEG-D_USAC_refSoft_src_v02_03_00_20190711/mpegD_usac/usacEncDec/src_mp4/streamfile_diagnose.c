/**********************************************************************
MPEG-4 Audio VM
consistency check routines for common stream file reader/writer

This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

and edited by

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

Copyright (c) 2004.
Source file: streamfile_diagnose.c



Required modules:
common_m4a.o            common module

**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flex_mux.h"            /* audio object types */
#include "common_m4a.h"          /* common module */

#include "lpc_common.h"
#include "phi_cons.h"
#include "nec_abs_const.h"       /* for CELP sample duration */

#include "streamfile.h"          /* public functions */
#include "streamfile_helper.h"
#include "streamfile_diagnose.h"

#define ReportError CommonWarning


/* struct for a buffer fullness simulation */
struct t_fullness_sim
{
  /* parameters */
  int   bufferSize;
  double avg_frame_length;
  /* variables */
  double bufferFullness;
  double min_bufferFullness;
  double max_bufferFullness;
  /* which tracks to include */
  int covered_tracks;
  int incoming_tracks;
  int incoming_bits;
};

static HANDLE_BUFFER_SIMULATION createBufferSimulation()
{
  HANDLE_BUFFER_SIMULATION ret = (HANDLE_BUFFER_SIMULATION)malloc(sizeof(struct t_fullness_sim));
  ret->bufferFullness = 0;
  ret->min_bufferFullness = 0;
  ret->max_bufferFullness = 0;
  ret->bufferSize = 0;
  ret->avg_frame_length = 0.0;
  ret->covered_tracks = 0;
  ret->incoming_tracks = 0;
  ret->incoming_bits = 0;
  return ret;
}

static void getDiagnosticValues(DEC_CONF_DESCRIPTOR *decConf,
                                DEC_CONF_DESCRIPTOR *decConfBaseLayer,
                                unsigned long *outSampleDuration,
                                unsigned long *outDecoderBufferSizeBit
                               ,const int sbrRatioIndex
                                );


/* --------------------------------------------------- */
/* ---- Find some known typical bugs              ---- */
/* --------------------------------------------------- */



/* ---- start checks for a single program or fix values ---- */

static int checkProgram(HANDLE_STREAMPROG prog, int fixValues)
{
  unsigned int trackNr, aacLayer = 0;
  int warnings = 0;
  DEC_CONF_DESCRIPTOR *decConf_base = &prog->decoderConfig[0];

  if (!fixValues) prog->programData->buffer_sim = createBufferSimulation();

  for (trackNr=0; trackNr < prog->trackCount; trackNr++) {
    DEC_CONF_DESCRIPTOR *decConf = &prog->decoderConfig[trackNr];
    unsigned long duration, bufferSize;

    getDiagnosticValues(decConf,
                        decConf_base,
                        &duration,
                        &bufferSize
                       ,prog->sbrRatioIndex[0]
                        );

    /* +++ check decoder buffer size */
    if (((bufferSize+7)/8) != decConf->bufferSizeDB.value) {
      if (fixValues) decConf->bufferSizeDB.value = (bufferSize+7)/8;
      else ReportError("decoderBufferSize incorrect (got %i, expected %i) for track %i", decConf->bufferSizeDB.value, ((bufferSize+7)/8), trackNr);
      warnings++;
    }

    if ((decConf->audioSpecificConfig.audioDecoderType.value == AAC_SCAL)||
        (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL)) {
      /* +++ check AAC layerNr */
      {
        unsigned int foundLayer = decConf->audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value;

        /* can't do anything in case of ER-AAC-scal ep1 */
        if ((decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL)&&
            (decConf->audioSpecificConfig.epConfig.value == 1)) {
          aacLayer = foundLayer;
        }

        if (foundLayer!=aacLayer) {
          if (fixValues)
            decConf->audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value = aacLayer;
          else
            ReportError("layerNr-field incorrect (%i) for track %i", foundLayer, trackNr);
          warnings++;
        }
        aacLayer++;
      }
    }

    /* +++ check "dependsOnCoreCoder" (in GAspecificConfig only) */
    switch (decConf->audioSpecificConfig.audioDecoderType.value) {
    case AAC_MAIN:
    case AAC_LC:
    case AAC_SSR:
    case AAC_LTP:
    case AAC_SCAL:
    case TWIN_VQ:
    case ER_AAC_LC:
    case ER_AAC_LTP:
    case ER_AAC_SCAL:
    case ER_TWIN_VQ:
    case ER_BSAC:
    case ER_AAC_LD:
    {
      int expectDepends = 0;
      int foundDepends = decConf->audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value;

      if (trackNr > 0) {
        if (((decConf->audioSpecificConfig.audioDecoderType.value == AAC_SCAL)||
             (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL))&&
            ((decConf->audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value==0)&&
             (decConf->audioSpecificConfig.audioDecoderType.value != decConf_base->audioSpecificConfig.audioDecoderType.value))) {
          expectDepends = 1;
        }
      }

      if (foundDepends!=expectDepends) {
        if (fixValues) decConf->audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value = expectDepends;
        else ReportError("dependsOnCoreCoder-flag incorrect (%i) for track %i", foundDepends, trackNr);
        warnings++;
      }
    }
    break;
#ifdef AAC_ELD
    case ER_AAC_ELD:
      break;
#endif

    }

    /* +++ check "directMapping */
    if ((decConf->audioSpecificConfig.epConfig.value == 3)&&
        (decConf->audioSpecificConfig.directMapping.value == 0)) {
      if (fixValues) decConf->audioSpecificConfig.directMapping.value = 1;
      else ReportError("directMapping-flag incorrect (%i) for track %i", decConf->audioSpecificConfig.directMapping.value,trackNr);
      warnings++;
    }

    /* +++ buffer simulation in case of AAC */
    if (((decConf->audioSpecificConfig.audioDecoderType.value == AAC_MAIN)||
         (decConf->audioSpecificConfig.audioDecoderType.value == AAC_LC)||
         (decConf->audioSpecificConfig.audioDecoderType.value == AAC_SSR)||
         (decConf->audioSpecificConfig.audioDecoderType.value == AAC_LTP)||
         (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LC)||
         (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LTP)||
         (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LD)||
#ifdef AAC_ELD
         (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD)||
#endif
         (decConf->audioSpecificConfig.audioDecoderType.value == AAC_SCAL)||
         (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL))&&(!fixValues)) {
      /* always take last bufferSize we got */
      prog->programData->buffer_sim->bufferSize = bufferSize;
      /* store variable bitrate as negative avg_frame_length */
      if (decConf->avgBitrate.value == 0) prog->programData->buffer_sim->avg_frame_length = -1.0;
      if (prog->programData->buffer_sim->avg_frame_length>=.0) {
        /* else sum up the bitrate */
        prog->programData->buffer_sim->avg_frame_length +=
          (double)(decConf->avgBitrate.value * duration) / (double)(decConf->audioSpecificConfig.samplingFrequency.value);
      }
      /* cover one track additionally */
      prog->programData->buffer_sim->covered_tracks++;
    }

    
  }
  return warnings;
}


/* ---- check stream as a whole ---- */

int startStreamDiagnose(HANDLE_STREAMFILE stream)
{
  int progNr;
  int warnings = 0;
  for (progNr=0; progNr<stream->progCount; progNr++) {
    HANDLE_STREAMPROG prog = &stream->prog[progNr];
    warnings += checkProgram(prog, 0);
  }

  if (warnings) {
    ReportError("Diagnosis found a total of %i warning(s)\n",warnings);
  }
  return warnings;
}


/* ---- fix some header values in a single program ---- */

int StreamDiagnoseAndFixProgram(HANDLE_STREAMPROG prog)
{ return checkProgram(prog, 1); }


/* --------------------------------------------------- */
/* ---- Frame lengths and sample durations        ---- */
/* --------------------------------------------------- */



/* ---- least common multiplier ---- */


static int lcm(int a, int b)
{
  int big,small,inc;
  if (a<b) {
    big = b; small = a; inc = b;
  } else {
    big = a; small = b; inc = a;
  }
  while (big%small) big+=inc;
  return big;
}


/* ---- get duration of one access unit ---- */


static void getDiagnosticValues(DEC_CONF_DESCRIPTOR *decConf,
                                DEC_CONF_DESCRIPTOR *decConfBaseLayer,
                                unsigned long *outSampleDuration,
                                unsigned long *outDecoderBufferSizeBit
                               ,const int sbrRatioIndex
) {
  long duration = 0;
  long bufferSize = 0;
  float bufferSizeFactor;
  int epToolAdditionalBuffer = 0;
  int downSampleRatio = 1;
  int upSampleRatio = 1;

  enum { UNKNOWN_BUFFER, CONST_RATE_BUFFER, AAC_BUFFER
#ifdef MPEG12
  ,MPEG12_BUFFER
#endif
#ifdef I2R_LOSSLESS
  ,SLS_BUFFER
#endif
  } bufferSizeType = UNKNOWN_BUFFER;
  enum { UNKNOWN_DURATION, TF_DURATION, TF_LD_DURATION, CELP_DURATION, PARA_DURATION, HVXC_DURATION, SSC_DURATION
#ifdef MPEG12
  ,MPEG12_DURATION
#endif
  } durationType = UNKNOWN_DURATION;
  switch(decConf->audioSpecificConfig.audioDecoderType.value) {
  case ER_AAC_LC:
  case ER_AAC_LTP:
  case ER_AAC_SCAL:
    epToolAdditionalBuffer = 1;
  case AAC_MAIN:
  case AAC_LC:
  case AAC_SSR:
  case AAC_LTP:
  case AAC_SCAL:
  case USAC:
    bufferSizeType = AAC_BUFFER;
    durationType = TF_DURATION;
    break;
  case ER_AAC_LD:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
    epToolAdditionalBuffer = 1;
    bufferSizeType = AAC_BUFFER;
    durationType = TF_LD_DURATION;
    break;
  case ER_BSAC:
    epToolAdditionalBuffer = 1;
    bufferSizeType = CONST_RATE_BUFFER; /* ??? */
    durationType = TF_DURATION;
    break;
  /* 20070326 BSAC Ext.*/
#ifdef CT_SBR
  case SBR:
	  if(decConf->audioSpecificConfig.extensionAudioDecoderType.value == ER_BSAC) {
		epToolAdditionalBuffer = 1;
		bufferSizeType = CONST_RATE_BUFFER; /* ??? */
		durationType = TF_DURATION;
	  }
	  break;
#endif
    break;
  case ER_TWIN_VQ:
    epToolAdditionalBuffer = 1;
  case TWIN_VQ:
    bufferSizeType = CONST_RATE_BUFFER;
    durationType = TF_DURATION;
    break;
  case ER_CELP:
    epToolAdditionalBuffer = 1;
  case CELP:
    bufferSizeType = CONST_RATE_BUFFER; /* ??? */
    durationType = CELP_DURATION;
    break;
  case ER_HILN:
  case ER_PARA:
    epToolAdditionalBuffer = 1;
    bufferSizeType = CONST_RATE_BUFFER; /* ??? */
    durationType = PARA_DURATION;
    break;
  case ER_HVXC:
    epToolAdditionalBuffer = 1;
  case HVXC:
    bufferSizeType = CONST_RATE_BUFFER; /* ??? */
    durationType = HVXC_DURATION;
    break;
#ifdef EXT2PAR
  case SSC:
    durationType = SSC_DURATION;
    break;
#endif
#ifdef MPEG12
  case LAYER_1:
  case LAYER_2:
  case LAYER_3:
    bufferSizeType = MPEG12_BUFFER;
    durationType = MPEG12_DURATION;
    break;
#endif
#ifdef I2R_LOSSLESS
  case SLS:
  case SLS_NCORE:
    bufferSizeType = SLS_BUFFER;
    durationType = TF_DURATION;
    break;
#endif
  default:
    CommonWarning("StreamFile:getDiagnosticValues: audioDecoderType %d not implemented",
                  decConf->audioSpecificConfig.audioDecoderType.value);
    break;
  }

  if ((epToolAdditionalBuffer)&&(decConf->audioSpecificConfig.epConfig.value>1)) {
    bufferSizeFactor = 1.2f; /* 20% additional buffer for FEC redundancy */
  } else {
    bufferSizeFactor = 1.0f;
  }


  /* +++ determine frame duration in samples +++ */
  switch (durationType) {
  case TF_DURATION:
    switch (decConf->audioSpecificConfig.audioDecoderType.value) {
    case USAC:
      if (decConf->audioSpecificConfig.specConf.usacConfig.frameLength.value == 0)
        duration = 1024;
      else
        duration = 960;
      break;

    default:
      if (decConf->audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value == 0)
        duration = 1024;
      else
        duration = 960;
    }

    break;
  case TF_LD_DURATION:
    switch (decConf->audioSpecificConfig.audioDecoderType.value) {
#ifdef AAC_ELD
      case ER_AAC_ELD:
       duration = (decConf->audioSpecificConfig.specConf.eldSpecificConfig.frameLengthFlag.value == 0) ? 512:480;
       break;
#endif
      default:
       duration = (decConf->audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value == 0) ? 512:480;
       break;
    }
    break;
  case CELP_DURATION:
    {
      CELP_SPECIFIC_CONFIG *celpBaseConf = &decConfBaseLayer->audioSpecificConfig.specConf.celpSpecificConfig;
      CELP_SPECIFIC_CONFIG *celpConf = &decConf->audioSpecificConfig.specConf.celpSpecificConfig;
      switch (celpBaseConf->excitationMode.value) {
      case RegularPulseExc:  /* RPE modes */
        switch (celpBaseConf->RPE_Configuration.value) {
        case 0:
        case 2:
        case 3:
          duration = FIFTEEN_MS;
          break;
        case 1:
          duration = TEN_MS;
          break;
        }
        break;
      case MultiPulseExc:  /* MPE modes */
        if ( celpBaseConf->sampleRateMode.value == fs8kHz) {   /* MPE modes 8 kHz */
          if (celpBaseConf->MPE_Configuration.value < 3) duration = NEC_FRAME40MS;
          else if (celpBaseConf->MPE_Configuration.value < 6) duration = NEC_FRAME30MS;
          else if (celpBaseConf->MPE_Configuration.value < 22) duration = NEC_FRAME20MS;
          else if (celpBaseConf->MPE_Configuration.value < 27) duration = NEC_FRAME10MS;
          else if (celpBaseConf->MPE_Configuration.value == 27) duration = NEC_FRAME30MS;
        } else {  /* MPE modes 16 kHz */
          if (celpBaseConf->MPE_Configuration.value < 7) duration = NEC_FRAME20MS_FRQ16;
          else if (celpBaseConf->MPE_Configuration.value < 16) duration = NEC_FRAME20MS_FRQ16;
          else if (celpBaseConf->MPE_Configuration.value < 23) duration = NEC_FRAME10MS_FRQ16;
          else if (celpBaseConf->MPE_Configuration.value < 32) duration = NEC_FRAME10MS_FRQ16;
        }
        break;
      }
      /* BWS mode */
      if ((celpBaseConf->sampleRateMode.value == fs8kHz)&&
          (celpBaseConf->bandwidthScalabilityMode.value == ON ) &&
          (celpConf->isBaseLayer.value == 0) &&
          (celpConf->isBWSLayer.value == 1)) duration *= 2;
    }
    break;
  case PARA_DURATION:
    switch (decConfBaseLayer->audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value) {
    case 0:
      duration = 160;
      break;
    case 1:
      duration = decConfBaseLayer->audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength.value;
      break;
    default: /* PARAmode 2 or 3 */
      duration = 320;
    }
    break;
  case HVXC_DURATION:
    duration = 160;	/* 20ms at fs=8000Hz */
    break;
  case SSC_DURATION:
    duration = 3072;
    break;
#ifdef MPEG12
  case MPEG12_DURATION:
    duration = 576;
    break;
#endif
  default:
    CommonWarning("StreamFile:getDiagnosticValues: could not determine frame duration");
  }

  /* +++ corect the duration by the SBR ratio index +++ */
  switch (sbrRatioIndex) {
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
      CommonWarning("StreamFile:getDiagnosticValues: could not determine SBR ratio index");
  }

  duration = duration * downSampleRatio / upSampleRatio;

  /* +++ determine decoder buffer size +++ */
  switch (bufferSizeType) {
  case AAC_BUFFER:
    {
    int indCCE, fc, sc, bc;
    switch (decConf->audioSpecificConfig.audioDecoderType.value) {
#ifdef AAC_ELD
      case ER_AAC_ELD:
       fc = decConf->audioSpecificConfig.channelConfiguration.value;
       indCCE = sc = bc = 0;
       break;
#endif
      case USAC:

        if (decConf->audioSpecificConfig.channelConfiguration.value > 0) {
          get_channel_number(decConf->audioSpecificConfig.channelConfiguration.value,
                           NULL,
                           &fc, NULL, &sc, &bc, NULL, NULL, &indCCE);
        } else {
          get_channel_number_USAC(&decConf->audioSpecificConfig.specConf.usacConfig,
                                  &fc, NULL, &sc, &bc, NULL, NULL, &indCCE);
        }
        break;
      default:
      get_channel_number(decConf->audioSpecificConfig.channelConfiguration.value,
                         decConf->audioSpecificConfig.specConf.TFSpecificConfig.progConfig,
                         &fc, NULL, &sc, &bc, NULL, NULL, &indCCE);
      break;
    }
    bufferSize = 6144*(indCCE+fc+sc+bc);
    }
    break;
  case CONST_RATE_BUFFER:
    {
      /* bufferSize is frame length as calculated from bitrate, rounding up */
      int br = decConf->avgBitrate.value;
      if (!br) br = decConf->maxBitrate.value;
      bufferSize = ((br * duration)+decConf->audioSpecificConfig.samplingFrequency.value-1) / decConf->audioSpecificConfig.samplingFrequency.value;
    }
    break;
#ifdef MPEG12
  case MPEG12_BUFFER:
    {
      bufferSize = 8192*8;      
    }
    break;
#endif
#ifdef I2R_LOSSLESS
  case SLS_BUFFER:
    {
      int indCCE, fc, sc, bc;
      get_channel_number(decConf->audioSpecificConfig.channelConfiguration.value,
                         decConf->audioSpecificConfig.specConf.TFSpecificConfig.progConfig,
                         &fc, NULL, &sc, &bc, NULL, NULL, &indCCE);
      bufferSize = 712856*(indCCE+fc+sc+bc);
    }
    break;
#endif
  default:
    CommonWarning("StreamFile:getDiagnosticValues: could not determine decoderBufferSize");
  }

  bufferSize = bufferSizeFactor * bufferSize;


  DebugPrintf(5,"StreamFile:getDiagnosticValues: duration = %ld, bufferSize = %ld bit\n",duration,bufferSize);
  *outSampleDuration = duration;
  *outDecoderBufferSizeBit = bufferSize;
  return;
}


/* ---- get sample durations for a whole program ---- */


static void getSampleDurations(HANDLE_STREAMPROG prog)
{
  unsigned long x;
  unsigned long duration, bufferSize;

  int maxSamplRate = prog->decoderConfig[prog->trackCount-1].audioSpecificConfig.samplingFrequency.value;

  int timePerFrame = -1;
  for (x=0; x < prog->trackCount; x++) {
    getDiagnosticValues(&prog->decoderConfig[x],
                        &prog->decoderConfig[0],
                        &duration,
                        &bufferSize
                       ,prog->sbrRatioIndex[x]
                        );

    prog->programData->sampleDuration[x] = duration;
    prog->programData->timePerAU[x] = duration * maxSamplRate / prog->decoderConfig[x].audioSpecificConfig.samplingFrequency.value;

    if (timePerFrame == -1) timePerFrame = prog->programData->timePerAU[x];
    else timePerFrame = lcm(prog->programData->timePerAU[x], timePerFrame);

    DebugPrintf(5,"sample duration/time in track %i: %i, %i",x,prog->programData->sampleDuration[x],prog->programData->timePerAU[x]);
  }

  prog->programData->timePerFrame = timePerFrame;
  DebugPrintf(5,"total time per frame: %i",timePerFrame);
}


void getAllSampleDurations(HANDLE_STREAMFILE stream)
{
  int progIdx;
  for (progIdx=0; progIdx<stream->progCount; progIdx++)
    getSampleDurations(&stream->prog[progIdx]);
}




/* --------------------------------------------------- */
/* ---- Tests done for each frame                 ---- */
/* --------------------------------------------------- */



static int UpdateBufferFullness(HANDLE_BUFFER_SIMULATION handle, int frame_length)
{
  int err = 0;
  int incomingBits;
  if ( handle == NULL ) return -1;

  handle->incoming_bits += frame_length;
  handle->incoming_tracks++;

  if (handle->incoming_tracks<handle->covered_tracks) return 0;

  incomingBits = handle->incoming_bits;
  handle->incoming_bits = 0;
  handle->incoming_tracks = 0;

  /* +++ variable bitrate check */
  if (incomingBits > handle->bufferSize) {
    CommonWarning("bitBuffer-simulation: framelength of %i too large for buffer size %i\n",
                  incomingBits, handle->bufferSize);
    err = 1;
  }
  /* only do variable bitrate check ? */
  if ( handle->avg_frame_length < .0) return err;


  /* +++ buffer fullness simulation for constant bitrate */
  handle->bufferFullness += handle->avg_frame_length - incomingBits;

  DebugPrintf(8,"bitBuffer-simulation: having %ld free bit(processed: %ld, got: %i)\n",
              (long)handle->bufferFullness, (long)handle->avg_frame_length, incomingBits);

  /* update and check the limits */
  if (handle->bufferFullness > handle->max_bufferFullness)
    handle->max_bufferFullness = handle->bufferFullness;
  if (handle->bufferFullness < handle->min_bufferFullness)
    handle->min_bufferFullness = handle->bufferFullness;

  if (handle->max_bufferFullness - handle->min_bufferFullness > handle->bufferSize - handle->avg_frame_length) {
    CommonWarning("bitBuffer-simulation: ERROR: used bit reservoir %.2f, available %.2f => buffer-limits exceeded\n",
                  handle->max_bufferFullness-handle->min_bufferFullness, handle->bufferSize - handle->avg_frame_length);
    DebugPrintf(8,"bitBuffer-simulation:        min free size: %.2f, max free size %.2f\n",
                handle->min_bufferFullness, handle->max_bufferFullness);
    err = 1;
  }

  return err;
}


int StreamDiagnoseAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  DEC_CONF_DESCRIPTOR *decConf;
  int warnings = 0;
  if ((prog==NULL)||(au==NULL)) return -1;

  decConf = &prog->decoderConfig[trackNr];

  /* +++ buffer simulation in case of AAC */
  if ((decConf->audioSpecificConfig.audioDecoderType.value == AAC_MAIN)||
      (decConf->audioSpecificConfig.audioDecoderType.value == AAC_LC)||
      (decConf->audioSpecificConfig.audioDecoderType.value == AAC_SSR)||
      (decConf->audioSpecificConfig.audioDecoderType.value == AAC_LTP)||
      (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LC)||
      (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LTP)||
      (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_LD)||
#ifdef AAC_ELD
      (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD)||
#endif
      (decConf->audioSpecificConfig.audioDecoderType.value == AAC_SCAL)||
      (decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL)) {
    if (UpdateBufferFullness(prog->programData->buffer_sim, au->numBits)) {
      warnings++;
    }
  }

  /* TODO: check whether AAC access units have attached the inherent byte-alignment even in the case of epConfig=1
   */

  return warnings;
}
