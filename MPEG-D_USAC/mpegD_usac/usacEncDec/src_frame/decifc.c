/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Markus Werner (SEED / Software Development Karlsruhe)

and edited by

Markus Werner (SEED / Software Development Karlsruhe)
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

Copyright (c) 2000.

$Id: decifc.c,v 1.13.6.1 2012-04-19 09:15:33 frd Exp $
Decoder high level interface
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "decifc.h"
#include "bitstream.h"
#include "dec_tf.h"
#include "dec_usac.h"
#include "dec_lpc.h"
#include "dec_par.h"
#ifdef MPEG12
#include "dec_mpeg12.h"
#endif
#ifdef EXT2PAR
#include "SscDec.h"
#endif
#include "common_m4a.h"
#include "ep_convert.h"


#define MAX_AU 12

/* initialize decData, count and return streams to decode */
int frameDataInit(int                 allTracks,
                  int                 streamCount,            /* in: number of ESs */
                  int                 maxLayer,
                  DEC_CONF_DESCRIPTOR decConf [MAX_TRACKS],
                  DEC_DATA*           decData
                  )
{
  int         layer;
  int         track;
  int         decoderStreams;
  FRAME_DATA* fD;

  /* clear decData */
  /* causes compiler warning "warning: statement with no effect" (sps@2008-02-27) */

  memset(decData,sizeof(DEC_DATA),0);

  /*
    Alloc FrameData
  */
  if(!(decData->frameData = (FRAME_DATA *)calloc(1,sizeof(FRAME_DATA)))) {
    return -1;
  }

  fD = decData->frameData;

  /* save original layer number for ER_BSAC */
  fD->bsacDecLayer=maxLayer;

  if (maxLayer != -1) {
    allTracks = maxLayer+1;
  }

  if (streamCount < allTracks) {
    CommonWarning("audioObjectType of last %d Tracks not implemented, decoding only %ld Tracks", allTracks-streamCount, streamCount);
  }


  /* determine real number of layers and streams */
  if (maxLayer < 0) maxLayer = streamCount-1;
  if (decConf[0].audioSpecificConfig.audioDecoderType.value == ER_BSAC)
	  maxLayer = fD->scalOutSelect = streamCount-1;
  tracksPerLayer(decConf, &maxLayer, &streamCount, fD->tracksInLayer, NULL /* not interested in ext payload track count */);
  fD->scalOutSelect = maxLayer;


  /* prepare EP decoder */
  for (layer=0,track=0; layer<(signed)fD->scalOutSelect+1; layer++) {
    AUDIO_SPECIFIC_CONFIG *asc = &decConf[track].audioSpecificConfig;
    int convertToEp = asc->epConfig.value;
    if (asc->epConfig.value == 2) convertToEp=0;
    if (asc->epConfig.value == 3) convertToEp=1;
    fD->ep_converter[layer] = EPconvert_create(fD->tracksInLayer[layer],
                                        asc->epConfig.value,
                                        &decConf[track].audioSpecificConfig.epSpecificConfig,
                                        NULL, /* dont write predfile */
                                        0, /* dont remove extension payload classes */
                                        convertToEp,
                                        NULL, NULL /* no ep info needed for output */);
    if (fD->ep_converter[layer]==NULL) return -1; 
    track += fD->tracksInLayer[layer];
  }

  /* now allocate internal structures */
  if(!(fD->od = (OBJECT_DESCRIPTOR *)calloc(1,sizeof(OBJECT_DESCRIPTOR)))) return -1;


  initObjDescr(fD->od);
  presetObjDescr(fD->od);

  decoderStreams = track = 0;
  for ( layer = 0; layer < (signed)fD->scalOutSelect+1; layer++ ) {
    int j;
    DEC_CONF_DESCRIPTOR *dC = &decConf[track];

    for (j=0; j<EPconvert_expectedOutputClasses(fD->ep_converter[layer]); j++, decoderStreams++) {
      fD->layer[decoderStreams].AULength = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));
      fD->layer[decoderStreams].AUPaddingBits = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));
      fD->layer[decoderStreams].bitBuf = BsAllocBuffer(MAX_BITBUF);

      initESDescr(&(fD->od->ESDescriptor[decoderStreams]));
      fD->od->ESDescriptor[decoderStreams]->DecConfigDescr = *dC;


      fD->layer[decoderStreams].sampleRate = dC->audioSpecificConfig.samplingFrequency.value ;
      fD->layer[decoderStreams].bitRate = dC->avgBitrate.value;
      
    }
    track += fD->tracksInLayer[layer];
  }

  fD->od->streamCount.value = decoderStreams;

  return decoderStreams;
}


/* initialize audio decoders */
int audioDecInit(HANDLE_DECODER_GENERAL   hFault,
                 DEC_DATA                *decData,
                 DEC_DEBUG_DATA          *decDebugData,
                 char                    *drc_payload_fileName,
                 char                    *drc_config_fileName,
                 char                    *loudness_config_fileName,
                 int                     *streamID,
#ifdef CT_SBR
                 int                      HEaacProfileLevel,
                 int                      bUseHQtransposer,
#endif
                 int                      tracksForDecoder
) {
  int stream         = 0;
  int epFlag         = 0;
  int delayNumSample = 0;
#ifdef AAC_ELD
  int ldsbr          = 0;
#endif
  FRAME_DATA *fD     = decData->frameData;
  int streamCount    = fD->od->streamCount.value;


  fD->od->streamCount.value = tracksForDecoder;

  for ( stream = 0; stream < streamCount; stream++ )  {
    unsigned long aot = fD->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
    /* goes through all ESs, which outnumbers layers for epConfig==1 */

    switch ( aot )
      {
        /* Nice hyungk !!! (2003.1.20)
           For -maxl option supporting in ER_BSAC */
      case ER_BSAC:
      case AAC_MAIN :
      case AAC_LC :
      case AAC_SCAL :
      case AAC_LTP:
      case TWIN_VQ :
      case ER_TWIN_VQ :
      case ER_AAC_LC:
      case ER_AAC_LTP:
      case ER_AAC_SCAL:
      case ER_AAC_LD:
#ifdef AAC_ELD
      case ER_AAC_ELD:
#endif
#ifdef I2R_LOSSLESS
      case SLS:
      case SLS_NCORE:
#endif

        if(decData->tfData == NULL){
          /* init only once for all tf layers */
          if(!(decData->tfData = (TF_DATA *)calloc(1,sizeof(TF_DATA))))
            return(1);
          DecTfInit(decDebugData->decPara,
                    decDebugData->aacDebugString,
                    decDebugData->epDebugLevel,
                    hFault,
                    fD,
                    decData->tfData,
                    decData->lpcData,
                    stream,
                    decDebugData->infoFileName
                    );
        }
        break;
      case CELP :
      case ER_CELP :
        if(decData->lpcData == NULL) {
          /* call it only once for all celp layers */
          if(!(decData->lpcData = (LPC_DATA *)calloc(1,sizeof(LPC_DATA))))
            return(1);
          DecLpcInit(decDebugData->decPara,
                     fD,
                     decData->lpcData);
        }
        break;
      case HVXC:
      case ER_HVXC:

        if (decData->hvxcData == NULL) {
          /* call it only once for all hvxc layers */

          decData->hvxcData= DecHvxcInit(decDebugData->decPara,
                                         fD,
                                         stream,
                                         epFlag,
                                         decDebugData->epDebugLevel,
                                         decDebugData->infoFileName,
                                         &hFault->hEscInstanceData);
          delayNumSample = decData->hvxcData->delayNumSample;
        }
        break;
      case ER_HILN:
      case ER_PARA:
        if (decData->paraData == NULL ){ /* call it only once for all hiln layers */
          decData->paraData = DecParInit(decDebugData->decPara,
                                         fD,
                                         stream,
                                         epFlag,
                                         decDebugData->epDebugLevel,
                                         decDebugData->infoFileName,
                                         &hFault->hEscInstanceData);

          delayNumSample = decData->paraData->delayNumSample;
        }
        break;

#ifdef EXT2PAR
    case SSC:
        if (decData->sscData == NULL)
      {
        DEC_CONF_DESCRIPTOR* decConf = &fD->od->ESDescriptor[0]->DecConfigDescr;
        int DecoderLevel;
        int UpdateRate;
        int SynthesisMethod;
        int ModeExt;
        int PsReserved;
        int LocalError;
        int Mode;
        int SamplingFrequency;
        int DecodeBaseLayer;
        int VarSfSize;

              /* Assign all the general parameters obtained from MP4 FF */

        DecoderLevel = decConf[0].audioSpecificConfig.specConf.sscSpecificConfig.DecoderLevel.value;
        UpdateRate = decConf[0].audioSpecificConfig.specConf.sscSpecificConfig.UpdateRate.value;
        SynthesisMethod = decConf[0].audioSpecificConfig.specConf.sscSpecificConfig.SynthesisMethod.value;
        ModeExt = decConf[0].audioSpecificConfig.specConf.sscSpecificConfig.ModeExt.value;
        PsReserved = decConf[0].audioSpecificConfig.specConf.sscSpecificConfig.PsReserved.value;
        Mode = decConf[0].audioSpecificConfig.channelConfiguration.value;
        SamplingFrequency = decConf[0].audioSpecificConfig.samplingFrequency.value;

        /* Get baselayer only decoding switch */

        DecSSCParseString(decDebugData->decPara, &DecodeBaseLayer, &VarSfSize);

        if ((VarSfSize < 22) ||(VarSfSize > 512))
        {
          CommonExit(-1,"\nTime scale/Speed limits exceeded (22..512)\n");
        }

        /* Create decoder instance */

        decData->sscData = SSCDEC_CreateInstance(Mode, DecodeBaseLayer, VarSfSize);



        /* Init */

        DecSSCInit(decData->sscData);

        /* Fill up the "SSC FF" structure */

        LocalError = SSCDEC_SetOriginalFileFormat(
                 DecoderLevel,
                 UpdateRate,
                 SynthesisMethod,
                 ModeExt,
                 PsReserved,
                 decData->sscData,
                 Mode,
                 SamplingFrequency,
                 VarSfSize
                 );
      }
      break;
#endif
#ifdef MPEG12
      case LAYER_1:
      case LAYER_2:
      case LAYER_3:

        if(decData->mpeg12Data == NULL) {
          /* init only once for all layers */
          if(!(decData->mpeg12Data = (MPEG12_DATA *)calloc(1,sizeof(MPEG12_DATA))))
            return(1);
          DecMPEG12Init(decDebugData->decPara,
                        decData->mpeg12Data,
                        stream,
                        fD
                        );
        }
        break;
#endif
      case USAC:
        if(decData->usacData == NULL){
          unsigned int nElement = 0;
          USAC_DECODER_CONFIG * pUsacDecoderConfig = &decData->frameData->od->ESDescriptor[decData->frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig.usacDecoderConfig;
          /* init only once for all usac layers */
          if(!(decData->usacData = (USAC_DATA *)calloc(1,sizeof(USAC_DATA)))) {
            return(1);
          }

          DecUsacInit(decDebugData->decPara,
                      decDebugData->aacDebugString,
                      decDebugData->epDebugLevel,
                      hFault,
                      fD,
                      decData->usacData,
                      decDebugData->infoFileName);

          /* Init DRC output files */
          for (nElement = 0; nElement < pUsacDecoderConfig->numElements; nElement++) {
            if (USAC_ELEMENT_TYPE_EXT == pUsacDecoderConfig->usacElementType[nElement]) {
              if (USAC_ID_EXT_ELE_UNI_DRC == pUsacDecoderConfig->usacElementConfig[nElement].usacExtConfig.usacExtElementType) {
                decData->usacData->drcInfo.uniDrc_extEle_present = 1;

                strncpy(decData->usacData->drc_config_file.drc_file_name, drc_config_fileName, FILENAME_MAX);
                strncpy(decData->usacData->drc_payload_file.drc_file_name, drc_payload_fileName, FILENAME_MAX);

                decData->usacData->drc_config_file.save_drc_file  = fopen(decData->usacData->drc_config_file.drc_file_name, "wb");
                decData->usacData->drc_payload_file.save_drc_file = fopen(decData->usacData->drc_payload_file.drc_file_name, "wb");

                /* write uniDrc config to file */
                fwrite(pUsacDecoderConfig->usacElementConfig[nElement].usacExtConfig.usacExtElementConfigPayload, sizeof(unsigned char), pUsacDecoderConfig->usacElementConfig[nElement].usacExtConfig.usacExtElementConfigLength, decData->usacData->drc_config_file.save_drc_file);
              }
            }
          }

          if (decData->frameData->od->ESDescriptor[decData->frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig.usacConfigExtensionPresent.value == 1) {
            USAC_CONFIG_EXTENSION *pUsacConfigExtension=&(decData->frameData->od->ESDescriptor[decData->frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig.usacConfigExtension);

            for (nElement = 0; nElement < pUsacConfigExtension->numConfigExtensions; nElement++) {
              if (USAC_CONFIG_EXT_LOUDNESS_INFO == pUsacConfigExtension->usacConfigExtType[nElement]) {
                decData->usacData->drcInfo.loudnessInfo_configExt_present = 1;

                strncpy(decData->usacData->drc_loudness_file.drc_file_name, loudness_config_fileName, FILENAME_MAX);

                decData->usacData->drc_loudness_file.save_drc_file = fopen(decData->usacData->drc_loudness_file.drc_file_name, "wb");

                 /* write LoudnessInfoSet to file */
                fwrite(pUsacConfigExtension->usacConfigExt[nElement], sizeof(unsigned char), pUsacConfigExtension->usacConfigExtLength[nElement], decData->usacData->drc_loudness_file.save_drc_file);
              }
              if (USAC_CONFIG_EXT_STREAM_ID == pUsacConfigExtension->usacConfigExtType[nElement]) {
                /* Put StreamID into usacConfig so it can be checked when checking for a config change. */
                AUDIO_SPECIFIC_CONFIG *asc = &decData->frameData->od->ESDescriptor[decData->frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig;
                asc->specConf.usacConfig.usacStreamId.streamID         = pUsacConfigExtension->usacConfigExt[nElement][1] | ((short)pUsacConfigExtension->usacConfigExt[nElement][0] << 8);
                asc->specConf.usacConfig.usacStreamId.streamID_present = 1;
              }
            }
          }

          if (decData->usacData->drcInfo.uniDrc_extEle_present == 1 || decData->usacData->drcInfo.loudnessInfo_configExt_present == 1) {
            AUDIO_SPECIFIC_CONFIG *asc = &decData->frameData->od->ESDescriptor[decData->frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig;

            /* transport core coder frame length (ccfl) to DRC */
            switch (asc->specConf.usacConfig.coreSbrFrameLengthIndex.value) {
              case 0:
              case 1:
                if (asc->specConf.usacConfig.frameLength.value == 0) {
                  decData->usacData->drcInfo.frameLength = 1024;
                } else {
                  decData->usacData->drcInfo.frameLength = 768;
                }
                break;
              case 2:
              case 3:
                decData->usacData->drcInfo.frameLength = 2048;
                break;
              case 4:
                decData->usacData->drcInfo.frameLength = 4096;
                break;
              default:
                DebugPrintf(1, "wrong coreSbrFrameLengthIndex");
                break;
            }

            /* transport SBR activation to DRC */
            if (decData->usacData->sbrRatioIndex > USAC_SBR_RATIO_INDEX_NO_SBR) {
              decData->usacData->drcInfo.sbr_active = 1;
            }
          }

          if (decData->usacData->drcInfo.uniDrc_extEle_present == 1 && decData->usacData->drcInfo.loudnessInfo_configExt_present == 0) {
            DebugPrintf(1, "Can't have a DRC without Loudness, an error may occur");
          }
        }
        break;

      default :
        DebugPrintf(1, "unsupported audio object type %ld", aot);
        break;
      } /* AOT switch */
  } /* loop over streams */

  fD->scalOutObjectType = fD->od->ESDescriptor[streamCount-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  fD->scalOutNumChannels = fD->od->ESDescriptor[streamCount-1]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
  fD->scalOutSamplingFrequency = fD->od->ESDescriptor[streamCount-1]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;

#ifdef CT_SBR
  /************************************/
  /* SBR for USAC */

  if (decData->usacData != NULL) {
    AUDIO_SPECIFIC_CONFIG * hAsc      = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
    USAC_CONFIG      *pUsacConfig     = &(fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig);
    int bs_interTes[MAX_NUM_ELEMENTS] = {0};
    int bs_pvc[MAX_NUM_ELEMENTS]      = {0};
    int harmonicSBR[MAX_NUM_ELEMENTS] = {0};

    USAC_DECODER_CONFIG * pUsacDecoderConfig = &pUsacConfig->usacDecoderConfig;
    int const nElements = pUsacDecoderConfig->numElements;
    int elemIdx = 0;

    if (pUsacConfig->usacStreamId.streamID_present == 1) {
      *streamID = pUsacConfig->usacStreamId.streamID;
    } else {
      *streamID = -1;
    }

    for(elemIdx = 0; elemIdx < nElements; elemIdx++){
      USAC_SBR_CONFIG  *pUsacSbrConfig  = UsacConfig_GetUsacSbrConfig(pUsacConfig, elemIdx);
      bs_interTes[elemIdx] = (pUsacSbrConfig != NULL)?pUsacSbrConfig->bs_interTes.value:0;
      bs_pvc[elemIdx] = (pUsacSbrConfig != NULL)?pUsacSbrConfig->bs_pvc.value:0;
      harmonicSBR[elemIdx] = (pUsacSbrConfig != NULL) ? pUsacSbrConfig->harmonicSBR.value:0;
    }

    /* channel configuration from usacConfig if ASC->channelConfiguration == 0 */
    if (fD->scalOutNumChannels == 0) {
      fD->scalOutNumChannels = pUsacConfig->usacChannelConfig.numOutChannels;
    }

    /* decData->usacData->sbrRatioIndex = fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.usacSpecificConfig.sbrRatioIndex.value; */
    decData->usacData->bDownSampleSbr = 0;

    if (HEaacProfileLevel < 5 && hAsc->samplingFrequency.value > 24000) {
      decData->usacData->bDownSampleSbr = 1;
    }

    if (decData->usacData->sbrRatioIndex > 0) { 
      if (hAsc->extensionSamplingFrequency.value == hAsc->samplingFrequency.value) {
        decData->usacData->bDownSampleSbr = 1;
      }
      if (decData->usacData->bDownSampleSbr == 0) {
        if (decData->usacData->sbrRatioIndex == 1) {
          fD->scalOutSamplingFrequency = 4 * fD->scalOutSamplingFrequency;
        } else {
          fD->scalOutSamplingFrequency = 2 * fD->scalOutSamplingFrequency;
        }
      }

      /* Initiate the SBR decoder. */
      decData->usacData->ct_sbrDecoder = openSBR(fD->layer->sampleRate,
                                                 decData->usacData->bDownSampleSbr,
                                                 decData->usacData->block_size_samples,
                                                 (SBR_RATIO_INDEX)decData->usacData->sbrRatioIndex
#ifdef AAC_ELD
                                                ,0
                                                ,NULL
#endif
                                                ,harmonicSBR,
                                                 bs_interTes,
                                                 bs_pvc,
                                                 bUseHQtransposer);
      if ( decData->usacData->ct_sbrDecoder == NULL ) {
        CommonExit(1,"can't open SBR decoder\n") ;
      }

      for(elemIdx = 0; elemIdx < nElements; elemIdx++)
      {
        USAC_ELEMENT_TYPE usacElType = pUsacConfig->usacDecoderConfig.usacElementType[elemIdx];
        
        if((usacElType != USAC_ELEMENT_TYPE_LFE) && (usacElType != USAC_ELEMENT_TYPE_EXT)){

          USAC_SBR_CONFIG  *pUsacSbrConfig  = UsacConfig_GetUsacSbrConfig(pUsacConfig, elemIdx);
          SBRDEC_USAC_SBR_HEADER usacDfltHeader;
          
          USAC_SBR_HEADER *pConfigDfltHeader = &(pUsacSbrConfig->sbrDfltHeader);

          memset(&usacDfltHeader, 0, sizeof(SBRDEC_USAC_SBR_HEADER));

          usacDfltHeader.start_freq     = pConfigDfltHeader->start_freq.value;
          usacDfltHeader.stop_freq      = pConfigDfltHeader->stop_freq.value;
          usacDfltHeader.header_extra1  = pConfigDfltHeader->header_extra1.value;
          usacDfltHeader.header_extra2  = pConfigDfltHeader->header_extra2.value;
          usacDfltHeader.freq_scale     = pConfigDfltHeader->freq_scale.value;
          usacDfltHeader.alter_scale    = pConfigDfltHeader->alter_scale.value;
          usacDfltHeader.noise_bands    = pConfigDfltHeader->noise_bands.value;
          usacDfltHeader.limiter_bands  = pConfigDfltHeader->limiter_bands.value;
          usacDfltHeader.limiter_gains  = pConfigDfltHeader->limiter_gains.value;
          usacDfltHeader.interpol_freq  = pConfigDfltHeader->interpol_freq.value;
          usacDfltHeader.smoothing_mode = pConfigDfltHeader->smoothing_mode.value;
          
          initUsacDfltHeader(decData->usacData->ct_sbrDecoder, &usacDfltHeader, elemIdx);
        }
      }

      { /* clear qmf interface */
        int s,c;
        for (s=0;s<TIMESLOT_BUFFER_SIZE;++s) {
          for (c=0;c<QMF_BUFFER_SIZE; ++c) {
            decData->usacData->sbrQmfBufferReal[s][c] = .0;
            decData->usacData->sbrQmfBufferImag[s][c] = .0;
          }
        }
      }
    }
  }


  /************************************/
  /* SBR for TF */

  if (decData->tfData != NULL) {
    DEC_CONF_DESCRIPTOR* decConf = &fD->od->ESDescriptor[0]->DecConfigDescr;
    decData->tfData->sbrPresentFlag = decConf->audioSpecificConfig.sbrPresentFlag.value;
    decData->tfData->bDownSampleSbr = 0;

    if(HEaacProfileLevel < 5 && decConf->audioSpecificConfig.samplingFrequency.value > 24000){
      decData->tfData->bDownSampleSbr = 1;
    }

    /* If the sbrPresentFlag is one, we know we have SBR, and can set up the output device
       with the correct sampling frequency.
    */
    if(decConf->audioSpecificConfig.sbrPresentFlag.value == 1){
#ifdef AAC_ELD
      unsigned char *sbrHeader;
#endif
      /* If the sampling FrequencyIndex is the same as the extensionSamplingFrequencyIndex, or the
	 extensionSamplingFrequency is the same as the samplingFrequency, we should use down-sampled SBR.
	 This means the the bDownSampledSbr = 1.
	 The upsamplingFactor is always two, since we always run a dual rate system. */
      if(decConf->audioSpecificConfig.extensionSamplingFrequency.value == decConf->audioSpecificConfig.samplingFrequency.value){
        decData->tfData->bDownSampleSbr = 1;
      }

      if(decData->tfData->bDownSampleSbr == 0){
        fD->scalOutSamplingFrequency = 2*fD->scalOutSamplingFrequency;
      }

      /* Initiate the SBR decoder. */
#ifdef AAC_ELD
      if( decConf->audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD ){
        ldsbr = 1;
        /* snl: implemntation of multiple sbr headers for multichannel is not done */
        sbrHeader =  decConf->audioSpecificConfig.specConf.eldSpecificConfig.sbrHeaderData[0];
      }
      else sbrHeader = NULL;
#endif
      decData->tfData->ct_sbrDecoder = openSBR(fD->layer->sampleRate,
                                               decData->tfData->bDownSampleSbr,
                                               decData->tfData->block_size_samples,
                                               SBR_RATIO_INDEX_2_1
#ifdef AAC_ELD
                                               ,ldsbr
                                               ,sbrHeader
#endif
                                               ,0
                                               ,0
                                               ,0
                                               ,0
                                               );
      if (decData->tfData->ct_sbrDecoder == NULL) {
        CommonExit(1,"can't open SBR decoder\n") ;
      }
    } else {
      if(decConf->audioSpecificConfig.sbrPresentFlag.value != 0) {
	/* The sbrPresentFlag is neither zero or one, hence implicit signalling may occur.
	   We are free to at this point set up the output device at twice the sampling rate in anticipation
	   of SBR data (if none is found we just use the SBR tool for upsampling only), or assume for now that
	   no SBR data is available and change the setup if we find any. This implementation does the latter,
	*/
      }
    }
  }
#endif

  return(delayNumSample);
}




int audioDecFrame(DEC_DATA   *decData,      /* in  : Decoder Status(Handles) */
                  HANDLE_DECODER_GENERAL hFault,
                  float      ***outSamples, /* out : composition Unit (outsamples) */
                  long        *numOutSamples, /* out : number of oputput samples/ch  */
                  int				*numOutChannels) /* out : number of oputput channels */ /* SAMSUNG_2005-09-30 */
{

  int numBits;
#ifdef EXT2PAR
  /* SSC variables */
  SSCDEC_FRAME_PARAM FrameParam;
  SSC_ERROR          ErrorCode;
  UInt               FrameLength;
  SInt               ErrorPos;
  HANDLE_BSBITBUFFER tmpBitBuf;
#endif

  switch (decData->frameData->scalOutObjectType)
    {
    case AAC_LC :
#ifdef CT_SBR
    case SBR :
#endif
    case AAC_MAIN :
    case AAC_LTP:
    case AAC_SCAL :
    case TWIN_VQ :
    case ER_TWIN_VQ :
    case ER_BSAC:
    case ER_AAC_LC:
    case ER_AAC_LTP:
    case ER_AAC_SCAL:
    case ER_AAC_LD:
#ifdef AAC_ELD
    case ER_AAC_ELD:
#endif
#ifdef I2R_LOSSLESS
    case SLS:
    case SLS_NCORE:
#endif
      DecTfFrame(decData->frameData->scalOutNumChannels,
                 decData->frameData,
                 decData->tfData,
                 decData->lpcData,
                 hFault,
                 numOutChannels) ; /* SAMSUNG_2005-09-30 */

      *outSamples    = decData->tfData->sampleBuf;
      {
        int osf = 1;
#ifdef I2R_LOSSLESS
        /* osf = decData->tfData->aacDecoder->mip->osf; */
        osf = decData->tfData->osf;
#endif

#ifdef CT_SBR
        if(decData->tfData->bDownSampleSbr || decData->tfData->runSbr == 0){
          *numOutSamples = osf*decData->tfData->block_size_samples;
        }
        else{
          *numOutSamples = osf*decData->tfData->block_size_samples * 2;
        }

#else
        *numOutSamples = osf*decData->tfData->block_size_samples;
#endif
      }
      break;
    case CELP :
    case ER_CELP :
      if (decData->frameData->layer[0].NoAUInBuffer>0) {
        DecLpcFrame(decData->frameData,
                    decData->lpcData,
                    &numBits);

      }
      else {
        CommonExit(-1,"\nNO more Access Units for Celp\n");
      }
      *outSamples    = decData->lpcData->sampleBuf;
      *numOutSamples = decData->lpcData->frameNumSample;
      break;

    case HVXC:
    case ER_HVXC:
        if (decData->frameData->layer[0].NoAUInBuffer>0) {
          DecHvxcFrame(decData->frameData,
                       decData->hvxcData,
                       &numBits,
                       0,/*epFlag,*/
                       hFault->hEscInstanceData);
        }
        else{
          CommonExit(-1,"\nNO more Access Units for Hvxc\n");
        }

        *outSamples    = decData->hvxcData->sampleBuf;
        *numOutSamples = decData->hvxcData->frameNumSample;
      break;
      case ER_HILN:
      case ER_PARA:
        if (decData->frameData->layer[0].NoAUInBuffer>0) {
           DecParFrame(decData->frameData,
                       decData->paraData,
                       &numBits,
                       0,/*epFlag*/
                       hFault->hEscInstanceData);

        }
        else {
          CommonExit(-1,"\nNO more Access Units for Para\n");
        }

       *outSamples    = decData->paraData->sampleBuf;
       *numOutSamples = decData->paraData->frameNumSample;
      break;

    case USAC:
        DecUsacFrame(decData->frameData->scalOutNumChannels,
                     decData->frameData,
                     decData->usacData,
                     hFault,
                     numOutChannels);
        *outSamples    = decData->usacData->sampleBuf;
        {
          int osf = 1;
#ifdef CT_SBR
          switch(decData->usacData->sbrRatioIndex){
          case 0:
            *numOutSamples = osf*decData->usacData->block_size_samples;
            break;
          case 1:
            *numOutSamples = osf*decData->usacData->block_size_samples*4;
            break;
          case 2:
            *numOutSamples = (osf*decData->usacData->block_size_samples*8)/3;
            break;
          case 3:
            *numOutSamples = osf*decData->usacData->block_size_samples*2;
            break;
          default:
            assert(0);
            break;
          }
#else
          *numOutSamples = osf*decData->usacData->block_size_samples;
#endif
        }
        break;


#ifdef EXT2PAR
    case SSC:
      {
        int NrOfOutSamples;

        tmpBitBuf = decData->frameData->layer[0].bitBuf;

        ErrorCode = SSCDEC_ParseFrameWrapper(decData->sscData,
                  BsBufferGetDataBegin(tmpBitBuf),
                  8192,
                  &FrameLength,
                  &ErrorPos);
        if (ErrorCode != SSC_ERROR_OK)
        {
          CommonExit(-1,"\nError parsing SSC frame\n");
        }

        ErrorCode = SSCDEC_SynthesiseFrameWrapper
          (decData->sscData, outSamples, &NrOfOutSamples);

        if (ErrorCode != SSC_ERROR_OK)
        {
          CommonExit(-1,"\nError synthesizing SSC frame\n");
        }

        *numOutSamples = (long)NrOfOutSamples;

        /* For the first frame the length is set to zero to */
        /* ommit writing the first frame            */
            SSCDEC_GetFrameParameters(decData->sscData, &FrameParam);
        *numOutSamples = FrameParam.FrameLength;

        /* Reset the bitpointer */
        BsClearBuffer (decData->frameData->layer[0].bitBuf);

        /* Indicate that acces unit is processed */
        decData->frameData->layer[0].NoAUInBuffer = 0;
      }
  break;
#endif
#ifdef MPEG12
    case LAYER_1:
    case LAYER_2:
    case LAYER_3:
        if (decData->frameData->layer[0].NoAUInBuffer>0) {
            DecMPEG12Frame(decData->frameData,
                           decData->mpeg12Data
                           );
        }
        else {
            CommonExit(-1,"\nNO more Access Units for MPEG12\n");
        }

        *outSamples     = decData->mpeg12Data->sampleBuf;
        *numOutSamples  = decData->mpeg12Data->frameNumSample;
    break;
#endif
    default :
    DebugPrintf(1,"unsupported audio object type %ld " ,decData->frameData->scalOutObjectType);
    break;
  }

  return(0);
}


int audioDecFree(DEC_DATA   *decData,
                 HANDLE_DECODER_GENERAL hFault)       /* in  : Decoder Status(Handles) */
{

  if(decData->hvxcData)
    DecHvxcFree (decData->hvxcData);

  if(decData->lpcData){
    DecLpcFree(decData->lpcData);
    free(decData->lpcData);
  }
  if(decData->tfData){
    DecTfFree(decData->tfData,hFault);
    free(decData->tfData);
  }
  if(decData->usacData){
	  DecUsacFree(decData->usacData,hFault);
	  free(decData->usacData);
  }
#ifdef EXT2PAR
  if (decData->sscData)
  {
    DecSSCFree(decData->sscData);
    SSCDEC_DestroyInstance(decData->sscData);
  }
#endif
#ifdef MPEG12
  if(decData->mpeg12Data){
    DecMPEG12Free(decData->mpeg12Data);
    free(decData->mpeg12Data);
  }
#endif
  free(decData->frameData->od);
  free(decData->frameData);

  return(0);

}
