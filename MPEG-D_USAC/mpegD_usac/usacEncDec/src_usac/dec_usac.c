/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Bernhard Grill     (Fraunhofer Gesellschaft IIS)

and edited by:
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
$Id: dec_usac.c,v 1.19.6.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/

/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by

 Bernhard Grill     (Fraunhofer Gesellschaft IIS)

 and edited by
 Markus Werner       (SEED / Software Development Karlsruhe)
 Eugen Dodenhoeft   (Fraunhofer IIS)

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

 $Id: dec_usac.c,v 1.19.6.1 2012-04-19 09:15:33 frd Exp $
 TF Decoder module
**********************************************************************/

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 ******************************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>

#include "allHandles.h"

#include "bitstream.h"
#include "common_m4a.h"
#include "dec_usac.h"
#include "mod_buf.h"
#include "tf_main.h"

/* Long term predictor */
#include "nok_lt_prediction.h"

/* ---  USAC --- */
#include "usac.h"
#include "usac_config.h"
#include "all.h"
#include "usac_tw_defines.h"

#include "reorderspec.h"
#include "resilience.h"
#include "concealment.h"
#include "buffers.h"
#include "statistics_aac.h"

#include "mp4_tf.h"
#include "sam_dec.h"

#include "usac_tw_tools.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "coupling.h"

#define MAX_SBR_UPSAMPLING_FAC 4

extern int samplFreqIndex [] ;

static int framecnt = 0;

/*****************************************************************************************
 ***
 *** Function: DecUsacInfo
 ***
 *** Purpose:  Provide Info about USAC decoder core
 ***
 ****************************************************************************************/

#define PROGVER "USAC RM0 decoder 2008-10-01"

char *DecUsacInfo (FILE *helpStream)
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
 *** Function: DecUsacInit
 ***
 *** Purpose:  Initialize the T/F-part and the macro blocks of the T/F part of the VM
 ***
 ****************************************************************************************/
void DecUsacInit (char*               decPara,      /* in: decoder parameter string */
                  char*               usacDebugStr,
                  int                 epDebugLevel,
                  HANDLE_DECODER_GENERAL hFault,
                  FRAME_DATA*         frameData,
                  USAC_DATA*          usacData,
                  char*               infoFileName
)
{
  AUDIO_SPECIFIC_CONFIG *streamConfig, *layerConfig ;
  int epFlag = 0;

  int numOutChannels = 0 ;  /* number of audio channels in topmost layer */
  int profile = LC_Profile ;
  int i_ch;
  int outputFrameLength, sbrRatioIndex;
  const int elemIdx = 0; /* hardcode to 0 for now */
  USAC_CONFIG *pUsacConfig = &(frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig);


  /*******************************************************************************************/
  /* General Initialization, Evaluate T/F-specific Commandline Switches                      */

  DebugPrintf(2, "  USAC decoding init \n");

  usacData->usacDecoder = NULL ;

  usacData->output_select = frameData->scalOutSelect ; /* bay : might be obsolete */

  usacData->prev_windowShape [0] = WS_FHG ;
  usacData->prev_windowShape [1] = WS_FHG ;

  usacData->tnsBevCore=0; /* bay: might be obsolete */
  usacData->mdctCoreOnly=0;

  /* for debug purposes only, remove before MPEG upload! */
  usacData->frameNo = 0;

#ifdef PARAMETRICSTEREO
  usacData->sbrEnablePS=0;
  if( strstr( decPara, "-ps"))
  {
    DebugPrintf (0, "\nParametric stereo decoding for SBR, the HE-AAC v2 Profile, is enabled\n");
    usacData->sbrEnablePS=1;
  }
#endif

  frameData->layer[frameData->od->streamCount.value-1].bitRate = frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.avgBitrate.value;

  streamConfig = &frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig ;
  layerConfig = &frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig;

  usacData->output_select = frameData->od->streamCount.value-1;

  outputFrameLength = UsacConfig_GetOutputFrameLength(pUsacConfig->coreSbrFrameLengthIndex.value);
  sbrRatioIndex     = UsacConfig_GetSbrRatioIndex(pUsacConfig->coreSbrFrameLengthIndex.value);

  switch(outputFrameLength){
  case 768:
    usacData->block_size_samples = 768;
    break;
  case 1024:
    usacData->block_size_samples = 1024;
    break;
  case 2048:
    usacData->block_size_samples = ((sbrRatioIndex == 2) ? 768 : 1024);
    break;
  case 4096:
    usacData->block_size_samples = 1024;
    break;
  default:
    assert(0);
    break;
  }

  {
    int const nElements = pUsacConfig->usacDecoderConfig.numElements;
    int el = 0;
    for(el = 0; el < nElements; el++){
      USAC_CORE_CONFIG *pUsacCoreConfig = UsacConfig_GetUsacCoreConfig(pUsacConfig, elemIdx);
      USAC_MPS212_CONFIG *pUsacMps212Config = UsacConfig_GetUsacMps212Config(pUsacConfig, elemIdx);
      int stereoConfigIndex = UsacConfig_GetStereoConfigIndex(pUsacConfig, elemIdx);

      if(pUsacCoreConfig){
        usacData->twMdct[el]     = pUsacCoreConfig->tw_mdct.value;
      }

      usacData->bStereoSbr[el] = (stereoConfigIndex == 3)?1:0;
      usacData->bPseudoLr[el]  = (stereoConfigIndex > 1)?pUsacMps212Config->bsPseudoLr.value:0;
    }
  }
  usacData->sbrRatioIndex = sbrRatioIndex;
  usacData->osf = 1;

  numOutChannels       = pUsacConfig->usacChannelConfig.numOutChannels; 
  
  /*
    assert(streamConfig->specConf.usacConfig.channelConfigurationIndex.value <= 0x7);
    streamChannels = streamConfig->specConf.usacConfig.channelConfigurationIndex.value;
  */

  if (layerConfig->samplingFreqencyIndex.value != 0xf)
  {
    frameData->layer[frameData->od->streamCount.value-1].sampleRate = samplFreqIndex [layerConfig->samplingFreqencyIndex.value] ;
  }
  else
  {
    frameData->layer[frameData->od->streamCount.value-1].sampleRate = layerConfig->samplingFrequency.value ;
    /*CommonExit(1,"unsupported sampleRateIndex") ;*/
  }


#ifdef USAC_REFSOFT_COR1_NOFIX_04
  if(outputFrameLength == 768){
    frameData->layer[frameData->od->streamCount.value-1].sampleRate = 4*frameData->layer[frameData->od->streamCount.value-1].sampleRate/3;
  }
#endif

  epFlag = 0;

  hFault[0].hVm          = CreateBuffer(VIRTUELL);
  hFault[0].hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);
  hFault[0].hHcrInfo     = CreateHcrInfo();
  hFault[0].hResilience  = CreateErrorResilience (decPara,
                                                  epFlag,
                                                  0,0,0);


  ConcealmentInit ( &hFault->hConcealment, decPara,0);
  StatisticsAacInit ( hFault->hEscInstanceData, hFault[0].hResilience );

  /* initialize DRC parameters */
  memset(&usacData->drcInfo, 0, sizeof(DRC_INFO));

  /*******************************************************************************************/
  /* Initialize USAC                                                                         */
  usacData->usacDecoder = USACDecodeInit(frameData->layer[frameData->od->streamCount.value-1].sampleRate,
                                          usacDebugStr,
                                          usacData->block_size_samples,
                                          &usacData->sfbInfo,
                                          profile,
                                          streamConfig);

  /*******************************************************************************************/
  /* Allocate Frame Scratch Memory                                                           */       
  /* :HACK: bay 20080902 allocate more channels (mip channel distribution is somewhat strange,
   * eg. CPEs get three channels assigned)
   */
  for (i_ch = 0 ; i_ch < MAX_TIME_CHANNELS /* min(numOutChannels*2, MAX_TIME_CHANNELS) */ ; i_ch++)
  {
    usacData->time_sample_vector[i_ch] = (float*)calloc(MAX_OSF * outputFrameLength, sizeof(float));
#ifndef USAC_REFSOFT_COR1_NOFIX_06
    usacData->overlap_buffer_buf[i_ch] = (float*)calloc(MAX_OSF * outputFrameLength+L_DIV_1024, sizeof(float));
    usacData->overlap_buffer[i_ch] = &(usacData->overlap_buffer_buf[i_ch][L_DIV_1024]);
#else
    usacData->overlap_buffer[i_ch] = (float*)calloc(MAX_OSF * outputFrameLength, sizeof(float));
#endif
    usacData->sampleBuf[i_ch] = (float*)malloc(MAX_OSF * outputFrameLength * sizeof(float));    
    usacData->spectral_line_vector[i_ch] = (float*)calloc(usacData->block_size_samples, sizeof(double));

    if ( usacData->twMdct[0] == 1) {      
      int i;
      usacData->warp_cont_mem[i_ch] = (float*) calloc (usacData->block_size_samples*2, sizeof(float));
      for ( i = 0 ; i < 2*usacData->block_size_samples ; i++ ) {
        usacData->warp_cont_mem[i_ch][i] = 1.0;
      }
      usacData->warp_sum[i_ch][0] = usacData->warp_sum[i_ch][1] = (float) usacData->block_size_samples;
      usacData->prev_sample_pos[i_ch] = (float*) calloc(3*usacData->block_size_samples,sizeof(float));
      usacData->prev_warped_time_sample_vector[i_ch] = (float*) calloc(3*usacData->block_size_samples,sizeof(float));
    }
  }

  {
    int i;

    usacData->alpha_q_re = (int**)calloc(MAX_SHORT_WINDOWS, sizeof(int*));
    usacData->alpha_q_im = (int**)calloc(MAX_SHORT_WINDOWS, sizeof(int*));
    usacData->alpha_q_re_prev = (int*)calloc(SFB_NUM_MAX, sizeof(int));
    usacData->alpha_q_im_prev = (int*)calloc(SFB_NUM_MAX, sizeof(int));
    usacData->cplx_pred_used = (int**)calloc(MAX_SHORT_WINDOWS, sizeof(int*));
    usacData->dmx_re_prev = (float*)calloc(BLOCK_LEN_LONG, sizeof(float));

    for (i = 0; i < MAX_SHORT_WINDOWS; i++) {
      usacData->alpha_q_re[i] = (int*)calloc(SFB_NUM_MAX, sizeof(int));
      usacData->alpha_q_im[i] = (int*)calloc(SFB_NUM_MAX, sizeof(int));
      usacData->cplx_pred_used[i] = (int*)calloc(SFB_NUM_MAX, sizeof(int));
    }
  }
}

/*****************************************************************************************
 ***
 *** Function:    DecUsacFree
 ***
 *** Description: free memory allocated by T/F decoders
 ***
 ***
 *****************************************************************************************/

void DecUsacFree(USAC_DATA  *usacData,
                  HANDLE_DECODER_GENERAL hFault
) {
  int i                = 0;
  unsigned int channel = 0;
  unsigned int layer   = 0;

  if (hFault->hEscInstanceData && hFault->hResilience && GetReorderSpecFlag (hFault[0].hResilience)) {
    StatisticsAacPrint();
  }

  if (usacData) {
    for (channel = 0; channel < MAX_TIME_CHANNELS; channel++) {
      if (usacData->time_sample_vector [channel]) {
        free (usacData->time_sample_vector [channel]);
      }
#ifndef USAC_REFSOFT_COR1_NOFIX_06
      if (usacData->overlap_buffer_buf [channel]) {
        free (usacData->overlap_buffer_buf [channel]);
      }
      usacData->overlap_buffer [channel] = NULL;
#else
      if (usacData->overlap_buffer [channel]) {
        free (usacData->overlap_buffer [channel]);
      }
#endif
      if (usacData->sampleBuf[channel]) {
        free (usacData->sampleBuf[channel]);
      }
      if (usacData->spectral_line_vector[channel]) {
        free(usacData->spectral_line_vector[channel]);
      }
      if (usacData->warp_cont_mem[channel]) {
        free(usacData->warp_cont_mem[channel]);
      }
      if (usacData->prev_sample_pos[channel]) {
        free(usacData->prev_sample_pos[channel]);
      }
      if (usacData->prev_warped_time_sample_vector[channel]) {
        free(usacData->prev_warped_time_sample_vector[channel]);
      }
    }

    if (usacData->drc_config_file.save_drc_file != NULL) {
      fclose(usacData->drc_config_file.save_drc_file);
    }
    if (usacData->drc_loudness_file.save_drc_file != NULL) {
      fclose(usacData->drc_loudness_file.save_drc_file);
    }
    if (usacData->drc_payload_file.save_drc_file != NULL) {
      fclose(usacData->drc_payload_file.save_drc_file);
    }
    if (usacData->coreModuloBuffer) {
      DeleteFloatModuloBuffer(usacData->coreModuloBuffer);
    }
    if (usacData->mdct_overlap_buffer) {
      free (usacData->mdct_overlap_buffer);
    }
  }

  if (hFault) {
    for (layer = 0; layer < MAX_TF_LAYER; layer++) {
      if (hFault[layer].hVm) {
        DeleteBuffer (hFault[layer].hVm);
      }
      if (hFault[layer].hHcrSpecData) {
        DeleteBuffer (hFault[layer].hHcrSpecData);
      }
      if (hFault[layer].hHcrInfo) {
        DeleteHcrInfo (hFault[layer].hHcrInfo);
      }
      if (hFault[layer].hResilience) {
        DeleteErrorResilience(hFault[layer].hResilience);
      }
    }
  }

#ifdef CT_SBR
  closeSBR(usacData->ct_sbrDecoder);
#endif

  if (usacData->usacDecoder) {
    USACDecodeFree(usacData->usacDecoder);
  }



  for (i = 0; i < MAX_SHORT_WINDOWS; i++) {
    free(usacData->alpha_q_re[i]);
    free(usacData->alpha_q_im[i]);
    free(usacData->cplx_pred_used[i]);
  }

  free(usacData->alpha_q_re);
  free(usacData->alpha_q_im);
  free(usacData->alpha_q_re_prev);
  free(usacData->alpha_q_im_prev);
  free(usacData->cplx_pred_used);
  free(usacData->dmx_re_prev);
}

/*****************************************************************************************
 ***
 *** Function:    DecUsacFrame
 ***
 *** Description: processes a block of time signal input samples into a bitstream
 ***              based on T/F encoding
 ***
 ***
 *****************************************************************************************/

void DecUsacFrame (int                    numChannels,
                   FRAME_DATA*            fd, /* config data ,one bitstream buffer for each layer, obj descr. etc. */
                   USAC_DATA*             usacData,
                   HANDLE_DECODER_GENERAL hFault,
                   int*                   numOutChannels
) {
  usacData->frameNo++;

  ResetReadBitCnt ( hFault[0].hVm );

  usacAUDecode(numChannels, fd, usacData, hFault, numOutChannels);

}
