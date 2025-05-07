/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial Author:
Bernhard Grill     (Fraunhofer Gesellschaft IIS)

and edited by
Markus Werner      (SEED / Software Development Karlsruhe)
Eugen Dodenhoeft   (Fraunhofer IIS)

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
$Id: dec_tf.c,v 1.7 2011-05-10 16:18:23 mul Exp $
*******************************************************************************/

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 ******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>

#include "allHandles.h"

#include "bitstream.h"
#include "common_m4a.h"
#include "dec_tf.h"
#include "mod_buf.h"
#include "tf_main.h"

/* Long term predictor */
#include "nok_lt_prediction.h"

/* ---  AAC --- */
#include "aac.h"
#include "all.h"

#ifdef I2R_LOSSLESS
#include "sls.h"
#endif

#include "reorderspec.h"
#include "resilience.h"
#include "concealment.h"
#include "buffers.h"
#include "statistics_aac.h"

#include "mp4_tf.h"
#include "sam_dec.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "coupling.h"

#ifdef DEBUGPLOT
#include "plotmtv.h"
#endif

extern int samplFreqIndex [] ;

static int framecnt = 0;

/*****************************************************************************************
 ***
 *** Function: DecTfInfo
 ***
 *** Purpose:  Provide Info about T/F-based decoder core
 ***
 ****************************************************************************************/

#define PROGVER "general audio decoder core FDAM1 16-Feb-2001"

char *DecTfInfo (FILE *helpStream)
{
  if (helpStream != NULL)
  {
    fprintf(helpStream,
      PROGVER "\n"
      "decoder parameter string format:\n"
      "possible options:\n"
      "     -aac_raw\n"
      "     -main\n"
      "     -lsf         length_of_scalefactor_data within bitstream\n"
      "     -rvlc        error resilient scalefactor coding\n"
      "     -scfCon      enables scalefactor concealment ( default is muting )\n"
      "     -scfBit      additional bit allowing enhanced concealment within bitstream\n"
      "     -lcw         length_of_longest_codeword within bitstream\n");
    fprintf(helpStream,
      "     -hcr <mode>  HCR within spectral data, length_of_longest_codeword and\n"
      "                                       length_of_spectral_data within bitstream\n"
      "                  modi: 0 - raw\n"
      "                        1 - codeword pre-sorting (standard)\n"
      "                        2 - consecutive HCR\n"
      "                        3 - codeword pre-sorting and consecutive HCR (standard)\n"
      "     -vcb11       virtual codebooks are used to encode codebook 11 within section data\n");

    ConcealmentHelp(helpStream);

    fprintf(helpStream,
      PROGVER "\n"
      "     -aac_sca\n"
      "     -aac_sys_bsac\n"
      "     -aac_ld : AAC Low Delay mode\n"
      "     -aac_ld_512 : block size 512 (default: 480)\n"
      "     -nttDecLyr <n> (scalable, n=0: base layer) (controlled by -maxl per default)\n"
#ifdef DRC
      "     -drc <cut> <boost> <ref_level> : parameters for drc, disabled per default\n"
#endif
#ifdef PARAMETRICSTEREO
      "     -ps (Enables parametric stereo decoding for SBR, the HE-AAC v2 Profile)\n"
#endif
      "\n");
  }

  return PROGVER;
}

/*****************************************************************************************
 ***
 *** Function: DecTfInit
 ***
 *** Purpose:  Initialize the T/F-part and the macro blocks of the T/F part of the VM
 ***
 ****************************************************************************************/
void DecTfInit (char*               decPara,      /* in: decoder parameter string */
                char*               aacDebugStr,
                int                 epDebugLevel,
                HANDLE_DECODER_GENERAL hFault,
                FRAME_DATA*         frameData,
                TF_DATA*            tfData,
                LPC_DATA*           lpcData,
                int                 layer,
                char*               infoFileName
                )
{
  AUDIO_SPECIFIC_CONFIG *streamConfig, *layerConfig ;
  int epFlag = 0;

  int streamChannels = 0 ;  /* number of audio channels in topmost layer */
  int layerChannels = 0 ;      /* number of audio channels in this layer */
  int profile = LC_Profile ;
  int i_ch;
  int i_layer;

  /*******************************************************************************************/
  /* General Initialization, Evaluate T/F-specific Commandline Switches                      */

  DebugPrintf(2, "  GA scaleable decoding init \n");

  tfData->aacDecoder = NULL ;
  tfData->tvqDecoder = NULL ; /* SAMSUNG_2005-09-30 */
  /* tfData->slsDecoder = NULL; */


  tfData->output_select = frameData->scalOutSelect ;

  tfData->prev_windowShape [0] = WS_FHG ;
  tfData->prev_windowShape [1] = WS_FHG ;

  tfData->tnsBevCore=0;
  if( strstr( decPara, "-tnsBevCor" ) )
    {
      DebugPrintf(1, "\n DON'T calc tns on core spectrum");

      tfData->tnsBevCore=1;
    }

  tfData->mdctCoreOnly=0;
  if( strstr( decPara, "-core_mdct"))
    {
      DebugPrintf (0, "\nIgnoring aac layers, output is core mdct only\n");
      tfData->mdctCoreOnly=1;
    }

#ifdef PARAMETRICSTEREO
  tfData->sbrEnablePS=0;
  if( strstr( decPara, "-ps"))
    {
      DebugPrintf (0, "\nParametric stereo decoding for SBR, the HE-AAC v2 Profile, is enabled\n");
      tfData->sbrEnablePS=1;
    }
#endif

  frameData->layer[layer].bitRate = frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value;

  streamConfig = &frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig ;
  layerConfig = &frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;

  switch(streamConfig->audioDecoderType.value){
  case ER_AAC_LC:
  case ER_AAC_LTP:
  case ER_AAC_LD:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
    tfData->output_select = frameData->od->streamCount.value-1;
    break;
  }

  switch(streamConfig->audioDecoderType.value){
#ifdef AAC_ELD
  case ER_AAC_ELD:
    tfData->block_size_samples = streamConfig->specConf.eldSpecificConfig.frameLengthFlag.value ? 480 : 512 ;
    break;
#endif
  case ER_AAC_LD:
    tfData->block_size_samples = streamConfig->specConf.TFSpecificConfig.frameLength.value ? 480 : 512 ;
    break;
  default:
    tfData->block_size_samples = streamConfig->specConf.TFSpecificConfig.frameLength.value ? 960 : 1024 ;
  }

  switch(streamConfig->audioDecoderType.value){
#ifdef I2R_LOSSLESS
  case SLS:
  case SLS_NCORE:
    switch (frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.slsSpecificConfig.SLSframeLength.value) {
    case 0: tfData->osf = 1; break;
    case 1: tfData->osf = 2; break;
    case 2: tfData->osf = 4; break;
    default:
      CommonExit(1,"invalid framelength for SLS\n");
      break;
    }
    break;
#endif
  default:
    tfData->osf = 1;
  }

  switch(streamConfig->audioDecoderType.value){
  case AAC_MAIN:
    tfData->pred_type = MONOPRED;
    break;
  case AAC_LC:
#ifdef CT_SBR
  case SBR:
#endif
#ifdef I2R_LOSSLESS
  case SLS:
  case SLS_NCORE:
#endif
    tfData->pred_type = PRED_NONE;
    break;
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
  default:
    tfData->pred_type = NOK_LTP;
    break;
  }


  streamChannels = streamConfig->channelConfiguration.value ;
  layerChannels = layerConfig->channelConfiguration.value ;

  if (layerConfig->samplingFreqencyIndex.value != 0xf)
    {
      frameData->layer[layer].sampleRate = samplFreqIndex [layerConfig->samplingFreqencyIndex.value] ;
    }
  else
    {
      frameData->layer[layer].sampleRate = layerConfig->samplingFrequency.value ;
      /*CommonExit(1,"unsupported sampleRateIndex") ;*/
    }

  /*******************************************************************************************/
  /* Initialize Error Resilience Tools                                                       */

  switch( layerConfig->audioDecoderType.value ) {
  case ER_AAC_LC:
  case ER_AAC_LTP:
  case ER_AAC_SCAL:
  case ER_AAC_LD:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
    epFlag = (layerConfig->epConfig.value & 1);
    break;
  default:
    epFlag = 0;
  }

  /* ER_SCAL */
  if (frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL) {

    /* EP Config 0,2,3 */
    if (frameData->od->ESDescriptor[tfData->output_select]->DecConfigDescr.audioSpecificConfig.epConfig.value == 0) {

      int tfLayer = 0; /* AAC & TVQ layer */

      for(i_layer=0; i_layer<=tfData->output_select; i_layer++) {

        if ( frameData->od->ESDescriptor[i_layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_CELP ) {
          tfLayer++;
        }
        else {

          AUDIO_SPECIFIC_CONFIG *layerConfigTmp = &frameData->od->ESDescriptor[i_layer]->DecConfigDescr.audioSpecificConfig;

          if ( frameData->od->ESDescriptor[i_layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL )
            epFlag = (layerConfig->epConfig.value & 1);

          /* create error resilience tools for all layer ! ( this is not needed for ER_CELP )*/
#ifdef AAC_ELD
          if ( frameData->od->ESDescriptor[i_layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD )
          hFault[tfLayer].hResilience  = CreateErrorResilience (decPara,
                                                                epFlag,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacSectionDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacSpectralDataResilienceFlag.value);
          else
#endif
          hFault[tfLayer].hResilience  = CreateErrorResilience (decPara,
                                                                epFlag,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacSectionDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacSpectralDataResilienceFlag.value);

          hFault[tfLayer].hVm          = CreateBuffer(VIRTUELL);
          hFault[tfLayer].hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);
          hFault[tfLayer].hHcrInfo     = CreateHcrInfo();

          tfLayer++;
        }
      }
    }
    /* EP Config 1 */
    else if ((frameData->od->ESDescriptor[tfData->output_select]->DecConfigDescr.audioSpecificConfig.epConfig.value == 1)||
             (frameData->od->ESDescriptor[tfData->output_select]->DecConfigDescr.audioSpecificConfig.epConfig.value == 2)||
             (frameData->od->ESDescriptor[tfData->output_select]->DecConfigDescr.audioSpecificConfig.epConfig.value == 3)) {

      unsigned int streamCnt  = 0; /* current stream */
      int tfLayer    = 0; /* AAC & TVQ layer */
      int twinVqCore = 0; /* signals TVQ layer */
      int celpCore   = 0; /* signals CELP layer */

      /* set stream to first stream of first aac layer */
      while ( frameData->od->ESDescriptor[streamCnt]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_CELP ) {
        streamCnt++;
        celpCore = 1;
      }

      /* get EP data for each er aac scalable layer */
      do {

        int numberOfChannel = 0;
        AUDIO_SPECIFIC_CONFIG *layerConfigTmp;

        /* get layer config of current stream */
        layerConfigTmp  = &frameData->od->ESDescriptor[streamCnt]->DecConfigDescr.audioSpecificConfig;
        numberOfChannel = layerConfigTmp->channelConfiguration.value;
#ifdef AAC_ELD
        if ( layerConfigTmp->audioDecoderType.value == ER_AAC_ELD )
          tfLayer         = 0;
        else
#endif
        tfLayer         = layerConfigTmp->specConf.TFSpecificConfig.layerNr.value;
        if ( layerConfigTmp->audioDecoderType.value == ER_TWIN_VQ )
          twinVqCore = 1;

        if ( layerConfigTmp->audioDecoderType.value == ER_AAC_SCAL ) {
          tfLayer += twinVqCore;
          tfLayer += celpCore;
          epFlag = 1;
        }

        /* has to be created one times per aac/tvq er layer */
        if ( hFault[tfLayer].hResilience == NULL ) {

          /* create error resilience tools for all tf layer */
          hFault[tfLayer].hVm          = CreateBuffer(VIRTUELL);
          hFault[tfLayer].hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);
          hFault[tfLayer].hHcrInfo     = CreateHcrInfo();
#ifdef AAC_ELD
          if ( layerConfigTmp->audioDecoderType.value == ER_AAC_ELD )
          hFault[tfLayer].hResilience  = CreateErrorResilience (decPara,
                                                                epFlag,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacSectionDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.eldSpecificConfig.aacSpectralDataResilienceFlag.value);
          else
#endif
          hFault[tfLayer].hResilience  = CreateErrorResilience (decPara,
                                                                epFlag,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacSectionDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                                layerConfigTmp->specConf.TFSpecificConfig.aacSpectralDataResilienceFlag.value);

          if( GetEPFlag ( hFault[tfLayer].hResilience ) ) {
            hFault[tfLayer].hEscInstanceData = CreateEscInstanceData ( infoFileName, epDebugLevel );

            InitAacSpecificInstanceData ( hFault[tfLayer].hEscInstanceData,
                                          hFault[tfLayer].hResilience,
                                          numberOfChannel,
                                          strstr(decPara, "tnsNotUsed") ? 1 : 0 );
          }
        }

        streamCnt++;

      } while ( streamCnt < frameData->od->streamCount.value ); /* for all streams */
    }
  }
  /* non SCAL ! */
  else {
    /* just hFault[0] needed */
    hFault[0].hVm          = CreateBuffer(VIRTUELL);
    hFault[0].hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);
    hFault[0].hHcrInfo     = CreateHcrInfo();
#ifdef AAC_ELD
    if ( layerConfig->audioDecoderType.value == ER_AAC_ELD )
    hFault[0].hResilience  = CreateErrorResilience (decPara,
                                                    epFlag,
                                                    layerConfig->specConf.eldSpecificConfig.aacSectionDataResilienceFlag.value,
                                                    layerConfig->specConf.eldSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                    layerConfig->specConf.eldSpecificConfig.aacSpectralDataResilienceFlag.value);
    else
#endif
    hFault[0].hResilience  = CreateErrorResilience (decPara,
                                                    epFlag,
                                                    layerConfig->specConf.TFSpecificConfig.aacSectionDataResilienceFlag.value,
                                                    layerConfig->specConf.TFSpecificConfig.aacScalefactorDataResilienceFlag.value,
                                                    layerConfig->specConf.TFSpecificConfig.aacSpectralDataResilienceFlag.value);

    if( GetEPFlag ( hFault[0].hResilience ) ) {
      hFault[0].hEscInstanceData = CreateEscInstanceData ( infoFileName, epDebugLevel );
      InitAacSpecificInstanceData ( hFault[0].hEscInstanceData, hFault[0].hResilience, layerChannels, strstr(decPara, "tnsNotUsed") ? 1 : 0 );
    }
  }

  ConcealmentInit ( &hFault->hConcealment, decPara,0);
  StatisticsAacInit ( hFault->hEscInstanceData, hFault[0].hResilience );


  /*******************************************************************************************/
  /* Initialize TwinVQ Base Layer                                                            */

  if ((layerConfig->audioDecoderType.value == TWIN_VQ) ||
      (layerConfig->audioDecoderType.value == ER_TWIN_VQ))
    {
      tvqInitDecoder (decPara, layer, frameData, tfData) ;
      tfData->pred_type = NOK_LTP ;

    }

  /*******************************************************************************************/
  /* Initialize Scalable AAC Layer(s)                                                        */

  switch (streamConfig->audioDecoderType.value) {

  case AAC_MAIN :
  case AAC_LTP :

    profile = Main_Profile ;

  case AAC_SCAL :
  case AAC_LC :
#ifdef CT_SBR
  case SBR :
#endif
  case ER_AAC_LC:
  case ER_AAC_LTP:
  case ER_AAC_SCAL:
  case ER_AAC_LD:
    /* 20060107 For sbr enabling */
  case ER_BSAC:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
#ifdef I2R_LOSSLESS
  case SLS:
  case SLS_NCORE:
#endif

      tfData->aacDecoder = AACDecodeInit (frameData->layer[layer].sampleRate,
                                          aacDebugStr,
                                          tfData->block_size_samples,
                                          &tfData->sfbInfo,
                                          tfData->pred_type,
                                          profile,
                                          decPara) ;
      break ;
    }


  /*******************************************************************************************/
  /* Initialize BSAC Tool                                                                    */

  /* Nice hyungk !!! (2003.1.20)
     For -maxl option supporting in ER_BSAC */
  if (streamConfig->audioDecoderType.value == ER_BSAC)
    {
      if ( frameData->bsacDecLayer == -1 )
        tfData->sam_DecBr = 0;  /* default decoded layer: full layer */
      else
        tfData->sam_DecBr = frameData->bsacDecLayer + 16;

      DebugPrintf (1, ">> BSAC Decoding BitRate: %d kbits/s/ch\n", tfData->sam_DecBr);

      sam_decode_init(frameData->layer[layer].sampleRate, tfData->block_size_samples);       /* HP 971105 */

      streamChannels = MAX_CHANNELS; /* SAMSUNG_2005-09-30 : for multichannel extension */
    }


  /*******************************************************************************************/
  /* Allocate Frame Scratch Memory, Initialize LTP                                           */

  for (i_ch = 0 ; i_ch < streamChannels ; i_ch++)
    {
      int x ;

      tfData->time_sample_vector[i_ch] = (double*)calloc(MAX_OSF*tfData->block_size_samples, sizeof(double));
      tfData->overlap_buffer[i_ch] = (double*)calloc(tfData->block_size_samples*4, sizeof(double));

#ifdef CT_SBR
      tfData->sampleBuf[i_ch] = (float*)malloc(MAX_OSF * 2 * tfData->block_size_samples*sizeof(float));
#ifdef PARAMETRICSTEREO
      if (streamChannels == 1)
        {
          tfData->sampleBuf[1] = (float*)malloc( 2 * tfData->block_size_samples*sizeof(float));
        }
#endif
#else
      tfData->sampleBuf[i_ch] = (float*)malloc(MAX_OSF* tfData->block_size_samples*sizeof(float));
#endif

      for (x = 0 ; x < MAX_TF_LAYER ; x++)
        {
          tfData->spectral_line_vector[x][i_ch] = (double*)calloc(tfData->block_size_samples, sizeof(double));
#ifdef I2R_LOSSLESS
          tfData->ll_quantInfo[i_ch][x] = (LIT_LL_quantInfo*)calloc(1, sizeof(LIT_LL_quantInfo));
#endif
        }

#ifdef I2R_LOSSLESS
      tfData->ll_info[i_ch] = (LIT_LL_decInfo*)calloc(1, sizeof(LIT_LL_decInfo));
      tfData->ll_coef[i_ch] = (int*)calloc(MAX_OSF*tfData->block_size_samples, sizeof(int));
      tfData->mdct_buf[i_ch] = (int*)calloc(MAX_OSF*2*tfData->block_size_samples, sizeof(int));
      tfData->ll_timesig[i_ch] = (int*)calloc(MAX_OSF*tfData->block_size_samples, sizeof(int));
      /********
               LIT_LL_quantInfo* ll_quantInfo[Chans];
               LIT_LL_decInfo * ll_info[Chans];
               LIT_LL_Info * enh_ll_info[Chans];

               int *ll_coef[Chans];
               int *mdct_buf[Chans] ;
               int *ll_timesig[Chans] ;
               int type_pcm;
      */
#endif

      if (tfData->pred_type == NOK_LTP)
        {
          tfData->nok_lt_status[i_ch] = (NOK_LT_PRED_STATUS *)malloc(sizeof(*tfData->nok_lt_status[0])) ;
          nok_init_lt_pred(tfData->nok_lt_status[i_ch]) ;
          tfData->nok_lt_status[i_ch]->overlap_buffer = (double*)calloc(tfData->block_size_samples, sizeof(double));

        }
    }


  /*******************************************************************************************/
  /* Allocate CELP Core Sample Delay Modulo Buffer                                           */

  if ((frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==CELP) ||
      (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_CELP))
    {

      int coreDelay     = 0;
      int samplIdxCore  = 0;
      int samplIdxTF    = 0;
      int firstAacLayer = 0;

      if (streamConfig->audioDecoderType.value == ER_AAC_SCAL) {
        unsigned int stream;
        for (stream=0; stream<frameData->od->streamCount.value; stream++) {
          if ( frameData->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL) {
            break;
          }
        }
        firstAacLayer = stream;
      }
      else {
        firstAacLayer = frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value + 1;
      }

      coreDelay= frameData->od->ESDescriptor[firstAacLayer]->DecConfigDescr\
        .audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay.value;
      samplIdxCore= frameData->od->ESDescriptor[0]\
        ->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;
      samplIdxTF= frameData->od->ESDescriptor[firstAacLayer]\
        ->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;

      tfData->samplFreqFacCore= samplFreqIndex[samplIdxTF]/samplFreqIndex[samplIdxCore];

      tfData->mdct_overlap_buffer = (double*)malloc(1024*sizeof(double));

      tfData->coreModuloBuffer = CreateFloatModuloBuffer(coreDelay
                                                         + (lpcData->frameNumSample
                                                            * tfData->samplFreqFacCore * 12));

      ZeroFloatModuloBuffer (tfData->coreModuloBuffer, coreDelay) ;
    }
}

/*****************************************************************************************
 ***
 *** Function:    DecTfFree
 ***
 *** Description: free memory allocated by T/F decoders
 ***
 ***
 *****************************************************************************************/

void DecTfFree (TF_DATA  *tfData,
                HANDLE_DECODER_GENERAL hFault)
{
  if ( hFault->hEscInstanceData && hFault->hResilience && GetReorderSpecFlag (hFault[0].hResilience ) ) {
    StatisticsAacPrint ( );
  }

  if (tfData)
    {
      unsigned int channel, layer ;

      for (channel = 0 ; channel < MAX_TIME_CHANNELS ; channel++)
        {
          if (tfData->time_sample_vector [channel]) {
            free (tfData->time_sample_vector [channel]);
          }
          if (tfData->overlap_buffer [channel]) {
            free (tfData->overlap_buffer [channel]);
          }
          if (tfData->nok_lt_status[channel]) {
            free (tfData->nok_lt_status[channel]);
          }

          if (tfData->sampleBuf[channel]) {
            free (tfData->sampleBuf[channel]);
          }

          for (layer=0;layer<MAX_TF_LAYER;layer++)
            {
              if (tfData->spectral_line_vector[layer][channel]) {
                free (tfData->spectral_line_vector[layer][channel]) ;
              }
#ifdef I2R_LOSSLESS
              if (tfData->ll_quantInfo[channel][layer]) {
                free (tfData->ll_quantInfo[channel][layer]);
              }
#endif
            }

#ifdef I2R_LOSSLESS
          if (tfData->ll_info[channel])    free (tfData->ll_info[channel]);
          if (tfData->ll_coef[channel])    free (tfData->ll_coef[channel]);
          if (tfData->mdct_buf[channel])   free (tfData->mdct_buf[channel]);
          if (tfData->ll_timesig[channel]) free (tfData->ll_timesig[channel]);
#endif

        }

      if (tfData->coreModuloBuffer) DeleteFloatModuloBuffer (tfData->coreModuloBuffer) ;
      if (tfData->mdct_overlap_buffer) { free (tfData->mdct_overlap_buffer) ; }
    }

  if (hFault)
    {
      unsigned int layer;

      for ( layer=0; layer<MAX_TF_LAYER; layer++ ) {

        if (hFault[layer].hVm)
          DeleteBuffer (hFault[layer].hVm);

        if (hFault[layer].hHcrSpecData)
          DeleteBuffer (hFault[layer].hHcrSpecData);

        if (hFault[layer].hHcrInfo)
          DeleteHcrInfo (hFault[layer].hHcrInfo);

        if (hFault[layer].hResilience)
          DeleteErrorResilience(hFault[layer].hResilience);
      }
    }

#ifdef CT_SBR
  closeSBR( tfData->ct_sbrDecoder );
#endif

  if(tfData->tvqDecoder)
    tvqFreeDecoder(tfData->tvqDecoder);

  if (tfData->aacDecoder)
    AACDecodeFree (tfData->aacDecoder) ;

#ifdef I2R_LOSSLESS
  /* if (tfData->slsDecoder) */
  /* SLSDecodeFree(tfData->slsDecoder); */
#endif
}

/*****************************************************************************************
 ***
 *** Function:    DecTfFrame
 ***
 *** Description: processes a block of time signal input samples into a bitstream
 ***              based on T/F encoding
 ***
 ***
 *****************************************************************************************/

void DecTfFrame (int          numChannels,
                 FRAME_DATA*  fd, /* config data ,one bitstream buffer for each layer, obj descr. etc. */
                 TF_DATA*     tfData,
                 LPC_DATA*    lpcData,
                 HANDLE_DECODER_GENERAL hFault,
                 int *numOutChannels
                 ) /* SAMSUNG_2005-09-30 */
{
  int i_ch;
  int ch,i;
  int tl_channel; /* indicates which channel should be used at top level*/
  /* 20060107 */
  unsigned long aot;
  int core_bandwidth = 0;
  int	flagBSAC_SBR = 0;
#ifdef AAC_ELD
  int ldsbr = 0;
#endif


  aot = fd->od->ESDescriptor [fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;

  /* 20060107 */
  if (aot==ER_BSAC)
    {
      int	i;
      for(i=0;i<MAX_TIME_CHANNELS;++i)
        {
          tfData->sbrBitStr_BSAC[i].NrElements     =  0;
          tfData->sbrBitStr_BSAC[i].NrElementsCore =  0;
        }
    }


  ResetReadBitCnt ( hFault[0].hVm );

  switch (fd->od->ESDescriptor [fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value)   /* was -2 for I2R_LOSSLESS */
    {
    case ER_BSAC :

      bsacAUDecode (numChannels,
                    fd, /* static configuration data, bitstream buffer, obj descr. etc. */
                    tfData, /* actual frame input/output data */
                    tfData->sam_DecBr,
                    numOutChannels
                    /* 20060107 */
                    , &flagBSAC_SBR
                    , &core_bandwidth
                    ); /* SAMSUNG_2005-09-30 */

      numChannels = *numOutChannels; /* SAMSUNG_2005-09-30 */

      break ;

    case AAC_SCAL :
    case TWIN_VQ :
    case ER_TWIN_VQ :
#ifdef I2R_LOSSLESS
    case SLS:
#endif
      tfScaleableAUDecode (numChannels,
                           fd, /* static configuration data, bitstream buffer, obj descr. etc. */
                           tfData, /* actual frame input/output data */
                           lpcData,
                           hFault , /* additional data for error protection */
                           SCALABLE);

      break ;
    case ER_AAC_SCAL:

      tfScaleableAUDecode (numChannels,
                           fd, /* static configuration data, bitstream buffer, obj descr. etc. */
                           tfData, /* actual frame input/output data */
                           lpcData,
                           hFault , /* additional data for error protection */
                           SCALABLE_ER);
      break;

    case AAC_LC :
#ifdef CT_SBR
    case SBR :
#endif
    case AAC_MAIN :
    case AAC_LTP :
      aacAUDecode (numChannels, fd, tfData, hFault, MULTICHANNEL) ;

      break;

#ifdef AAC_ELD
    case ER_AAC_ELD:
      ldsbr = 1;
      aacAUDecode (numChannels, fd, tfData, hFault,MULTICHANNEL_ELD) ;
      break;
#endif
    case ER_AAC_LC:
    case ER_AAC_LTP:
    case ER_AAC_LD:
      aacAUDecode (numChannels, fd, tfData, hFault,MULTICHANNEL_ER) ;

      break ;

#ifdef I2R_LOSSLESS
    case SLS_NCORE:

      break;
#endif
    }

#ifdef I2R_LOSSLESS
  if ( (fd->scalOutObjectType != SLS) && (fd->scalOutObjectType != SLS_NCORE) ) {
#endif
    for (i_ch=0; i_ch<numChannels; i_ch++)
      {
#ifdef REDUCE_SIGNAL_TO_16
        int i;
#endif

#ifdef AAC_ELD
           /* set window shape for ER_AAC_ELD to low delay windows */
           if (fd->scalOutObjectType == ER_AAC_ELD) {
             tfData->prev_windowShape[i_ch] = WS_FHG_LDFB;
             tfData->windowShape[i_ch] = WS_FHG_LDFB;
           }
#endif

        freq2buffer (tfData->spectral_line_vector[0][i_ch],
                     tfData->time_sample_vector[i_ch],
                     tfData->overlap_buffer[i_ch],
                     tfData->windowSequence[i_ch],
                     tfData->block_size_samples,
                     tfData->block_size_samples/8,
                     tfData->windowShape[i_ch],
                     tfData->prev_windowShape[i_ch],
                     OVERLAPPED_MODE,
                     8);

        tfData->prev_windowShape[i_ch] = tfData->windowShape[i_ch];
#define REDUCE_SIGNAL_TO_16_
#ifdef REDUCE_SIGNAL_TO_16
        for(i = 0; i < tfData->block_size_samples; i++){
          /* Additional noise for demonstration */
#define AAA (0.5)
          tfData->time_sample_vector[i_ch][i]+=(AAA/RAND_MAX)*(float)rand()-AAA/2;
          tfData->overlap_buffer[i_ch][i]+=    (AAA/RAND_MAX)*(float)rand()-AAA/2;
        }
#endif

        if(fd->od->ESDescriptor[fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == AAC_LTP
           || fd->od->ESDescriptor[fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_LTP
           || fd->od->ESDescriptor[fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_LD
#ifdef AAC_ELD
           || fd->od->ESDescriptor[fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD
#endif
           )
          nok_lt_update(tfData->nok_lt_status[i_ch], tfData->time_sample_vector[i_ch],
                        tfData->overlap_buffer[i_ch], tfData->block_size_samples);

#ifdef DEBUGPLOT
        if (i_ch==0){
          plotSend("L", "timeOut",  MTV_DOUBLE,tfData->block_size_samples,  tfData->time_sample_vector[i_ch] , NULL);
        } else {
          plotSend("R", "timeOutR",  MTV_DOUBLE,tfData->block_size_samples,  tfData->time_sample_vector[i_ch] , NULL);
        }
#endif

      }
#ifdef I2R_LOSSLESS
  }
#endif

  /* Independently switched coupling */

  tl_channel=0;
  for (ch=0; ch<Chans; ch++)
    {
      if (tfData->aacDecoder != NULL)
        {
	  if (!(tfData->aacDecoder->mip->ch_info[ch].present)) continue;
          coupling(tfData->aacDecoder->info,
                   tfData->aacDecoder->mip,
                   tfData->time_sample_vector,
                   tfData->aacDecoder->cc_coef,
                   tfData->aacDecoder->cc_gain,
                   ch, /* i_ch */
                   CC_DOM,
                   tfData->aacDecoder->cc_wnd,
                   tfData->aacDecoder->cb_map,
                   CC_IND,
                   tl_channel);
          tl_channel++;

        }


      /*fprintf(stderr,"channel: %d present: %d.\n",ch,tfData->aacDecoder->mip->ch_info[ch].present);*/

    } /*end coupling*/

  /* convert time data from double to float */
  for (i_ch=0; i_ch<numChannels; i_ch++)
    {
      int osf=1;
#ifdef I2R_LOSSLESS
      /* osf=tfData->aacDecoder->mip->osf; */
      osf=tfData->osf;
#endif
      for( i=0; i<osf*(int)tfData->block_size_samples; i++ )
        {
          tfData->sampleBuf[i_ch][i] = /*(float)*/tfData->time_sample_vector[i_ch][i];
        }
    }

#ifdef CT_SBR
  /* 20060107 */
  if (aot==ER_BSAC)
    {
      if (flagBSAC_SBR)
        {
          tfData->sbrPresentFlag = 2;
        }
    }

  /* Module Functions for SBR Decoder */
  {
    SBRBITSTREAM* hCt_sbrBitStr;

    tfData->runSbr = 0;

    if(tfData->sbrPresentFlag != 0){
      /* 20060107 */
      if (aot == ER_BSAC)
        hCt_sbrBitStr = tfData->sbrBitStr_BSAC;
      else
        {
          hCt_sbrBitStr = getSbrBitstream( tfData->aacDecoder );
          core_bandwidth = 0;
        }

      tfData->runSbr = 1;

      if ( tfData->aacDecoder != NULL ) {
        if ( hCt_sbrBitStr[0].NrElements != 0 ) {

          if (tfData->sbrPresentFlag != 1 && framecnt == 0 ) {
				/* It seems that implicit signalling occurs, since there are SBR
				   bitsream elements available. Hence, we need to initialise the
				   SBR decoder. */

            DebugPrintf(3,"   Implicit signalling of SBR detected!");

            tfData->ct_sbrDecoder = openSBR(fd->layer->sampleRate,
                                            tfData->bDownSampleSbr,
                                            tfData->block_size_samples,
                                            SBR_RATIO_INDEX_2_1
#ifdef AAC_ELD
                                            ,ldsbr
                                            ,NULL /* no sbr header in ASC */
#endif
                                            ,0
                                            ,0
                                            ,0
                                            ,0
                                            );

            if ( tfData->ct_sbrDecoder == NULL ) {
              CommonExit(1,"can't open SBR decoder\n") ;
            }

          }

          {

#ifdef SBR_SCALABLE
            /* just usefull for scalable sbr */
            int maxSfbFreqLine = getMaxSfbFreqLine( tfData->aacDecoder );
#endif
#ifdef PARAMETRICSTEREO
            if ( tfData->sbrEnablePS == 1 )
              *numOutChannels = 2;
#endif
            framecnt++;
          }
        }
        else{
          tfData->runSbr = 0;
        }
      }
    }
  }
#endif
}
