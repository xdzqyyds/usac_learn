/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, VoiceAge Corp.

and edited by
Yoshiaki Oikawa     (Sony Corporation),
Mitsuyuki Hatanaka  (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Markus Werner       (SEED / Software Development Karlsruhe)
Eugen Dodenhoeft    (Fraunhofer IIS)

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
$Id: decoder_usac.c,v 1.21.2.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "allHandles.h"
#include "tf_mainHandle.h"
#include "block.h"
#include "buffer.h"
#include "coupling.h"

#ifdef SONY_PVC
#include "../src_tf/sony_pvcprepro.h"
#endif  /* SONY_PVC */


#include "all.h"                 /* structs */
#include "obj_descr.h"           /* structs */
#include "usac_config.h"
#include "usac_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "usac_port.h"           /* channel mapping */

#include "allVariables.h"        /* variables */
#include "usac_allVariables.h"
#include "spatial_bitstreamreader.h"

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "usac.h"
#include "dec_tf.h"

#ifdef DRC
#include "drc.h"
#endif

#include "dolby_def.h"
#include "tf_main.h"
#include "common_m4a.h"
#include "usac_port.h"
#include "buffers.h"
#include "bitstream.h"

#include "resilience.h"
#include "concealment.h"

#include "allVariables.h"
#include "obj_descr.h"

#include "extension_type.h"

#include "usac_tw_tools.h"

#include "acelp_plus.h"
#include "sac_dec_interface.h"
#include "sac_polyphase.h"

#include "streamfile.h"

#define MAXNRSBRELEMENTS 6


/*
---     Global variables from usac-decoder      ---
 */

int debug [256] ;
static int* last_rstgrp_num = NULL;

/* global variables */
spatialDec* hSpatialDec[MAX_NUM_ELEMENTS] = {NULL};
extern int gUseFractionalDelayDecor;


/* -------------------------------------- */
/*       module-private functions         */
/* -------------------------------------- */

static void predinit (HANDLE_AACDECODER hDec)
{
  int i, ch;

  if (hDec->pred_type == MONOPRED)
  {
    for (ch = 0; ch < Chans; ch++)
    {
      for (i = 0; i < LN2; i++)
      {
        init_pred_stat(&hDec->sp_status[ch][i],
                       PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
      }
    }
  }
}

static int getdata (int*              tag,
                    int*              dt_cnt,
                    byte*             data_bytes,
                    HANDLE_RESILIENCE hResilience,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_BUFFER     hVm )
{
  int i, align_flag, cnt;

  if (debug['V']) fprintf(stderr,"# DSE detected\n");
  *tag = GetBits ( LEN_TAG,
                   ELEMENT_INSTANCE_TAG,
                   hResilience,
                   hEscInstanceData,
                   hVm );

  align_flag = GetBits ( LEN_D_ALIGN,
                         MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                         hResilience,
                         hEscInstanceData,
                         hVm );

  if ((cnt = GetBits ( LEN_D_CNT,
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience,
                       hEscInstanceData,
                       hVm)) == (1<<LEN_D_CNT)-1)

    cnt +=  GetBits ( LEN_D_ESC,
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
                      hVm);

  *dt_cnt = cnt;
  if (debug['x'])
    fprintf(stderr, "data element %d has %d bytes\n", *tag, cnt);
  if (align_flag)
    byte_align();

  for (i=0; i<cnt; i++){
    data_bytes[i] = (byte) GetBits ( LEN_BYTE,
                                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                     hResilience,
                                     hEscInstanceData,
                                     hVm);
    if (debug['X'])
      fprintf(stderr, "%6d %2x\n", i, data_bytes[i]);
  }
  return 0;
}

static int
getSbrExtensionData(
                       HANDLE_RESILIENCE        hResilience,
                       HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                       HANDLE_BUFFER            hVm
                       ,SBRBITSTREAM*           ct_sbrBitStr
                       ,int*                    payloadLength
)
{
  int i;
  int count = 0;
  int ReadBits = 0;
  int Extension_Type = 0;
  int unalignedBits = 0;

  /* mul: workaround, to not read too many bits */
  HANDLE_BSBITBUFFER bsBitBuffer = GetRemainingBufferBits(hVm);
  HANDLE_BSBITSTREAM bsBitStream = BsOpenBufferRead(bsBitBuffer);

  /* get Byte count of the payload data */
  count = (*payloadLength) >> 3; /* divide by 8 and floor */
  count = min(MAXSBRBYTES, count);
  if ( count > 0 ) {
    /* push sbr payload */
    if((count <= MAXSBRBYTES) && (ct_sbrBitStr->NrElements < MAXNRSBRELEMENTS)) {
      ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].ExtensionType = EXT_SBR_DATA;
      ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload       = count;

      for (i = 0 ; i < count ; i++) {
        BsGetBitChar(bsBitStream,
                     &ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i],
                     8);
        ReadBits+=8;
      }

      /* read unaligned bits */
      unalignedBits = (*payloadLength-ReadBits);
      if ( unalignedBits > 0 && unalignedBits < 8) {
        BsGetBitChar(bsBitStream, &ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[count], unalignedBits);

        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[count] =
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[count] << (8-unalignedBits);

        ReadBits += (unalignedBits);
        count++;
        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload = count; /* incr Payload cnt */
      }
      ct_sbrBitStr->NrElements +=1;
      ct_sbrBitStr->NrElementsCore += 1;
    }

    *payloadLength -= ReadBits;
  }

  BsClose(bsBitStream);
  BsFreeBuffer(bsBitBuffer);

  return ReadBits;
}

#define USAC_EXTENSION_PAYLOAD_INTEGRATION
#ifdef USAC_EXTENSION_PAYLOAD_INTEGRATION

typedef struct _usac_ext_element {
  UINT32  usacExtElementPresent;
  UINT32  usacExtElementUseDefaultLength;
  unsigned int  usacExtElementPayloadLength;
  UINT32  usacExtElementPayloadFrag;
  UINT32  usacExtElementStart;
  UINT32  usacExtElementStop;
  unsigned char usacExtElementSegmentData[6144*MAX_CHANNELS];
} USAC_EXT_ELEMENT;

static unsigned int __ReadUsacExtElement(USAC_EXT_CONFIG * pUsacExtConfig,
                                         USAC_EXT_ELEMENT * pUsacExtElement,
                                         HANDLE_BUFFER hVm){
  unsigned int nBitsRead = 0;

  HANDLE_BSBITBUFFER bsBitBuffer = GetRemainingBufferBits(hVm);
  HANDLE_BSBITSTREAM bsBitStream = BsOpenBufferRead(bsBitBuffer);

  if(pUsacExtConfig){
    unsigned int i = 0;

    nBitsRead += BsRWBitWrapper(bsBitStream, &pUsacExtElement->usacExtElementPresent, 1, 0);
    if(pUsacExtElement->usacExtElementPresent){
      nBitsRead += BsRWBitWrapper(bsBitStream, &pUsacExtElement->usacExtElementUseDefaultLength, 1, 0);
      if(pUsacExtElement->usacExtElementUseDefaultLength){
        pUsacExtElement->usacExtElementPayloadLength = pUsacExtConfig->usacExtElementDefaultLength;
      } else {


#ifdef USAC_EXTENSION_ELEMENT_NOFIX
        nBitsRead += UsacConfig_ReadEscapedValue(bsBitStream, &pUsacExtElement->usacExtElementPayloadLength, 8, 16, 0);
#else
        /* nBitsRead += UsacConfig_ReadEscapedValue(bsBitStream, &pUsacExtElement->usacExtElementPayloadLength, 8, 16, 0);
           OTE: which is correct?? atleast the above returns usacExtElementPayloadLength%8 == 0, i.e. even number of bytes*/

        nBitsRead += BsRWBitWrapper(bsBitStream, &pUsacExtElement->usacExtElementPayloadLength, 8, 0);
        if (pUsacExtElement->usacExtElementPayloadLength == 255){
          unsigned long valueAdd = 0;
          nBitsRead += BsRWBitWrapper(bsBitStream, &valueAdd, 16, 0);
          pUsacExtElement->usacExtElementPayloadLength += (valueAdd - 2);
        }
#endif
      }

      if(pUsacExtElement->usacExtElementPayloadLength > 0){
        if(pUsacExtConfig->usacExtElementPayloadFrag.value){
          nBitsRead += BsRWBitWrapper(bsBitStream, &pUsacExtElement->usacExtElementStart, 1, 0);
          nBitsRead += BsRWBitWrapper(bsBitStream, &pUsacExtElement->usacExtElementStop, 1, 0);
        } else {
          pUsacExtElement->usacExtElementStart = 1;
          pUsacExtElement->usacExtElementStop = 1;
        }

        for(i = 0; i < pUsacExtElement->usacExtElementPayloadLength; i++){
          nBitsRead += BsRWBitWrapper(bsBitStream, &(pUsacExtElement->usacExtElementSegmentData[i]), 8, 0);
        }

      }
    }
  }

  if(bsBitStream) BsClose(bsBitStream);
  if(bsBitBuffer) BsFreeBuffer(bsBitBuffer);
  SkipBits(hVm, nBitsRead);

  return nBitsRead;
}


#endif

#ifdef CT_SBR
static void
ResetSbrBitstream( SBRBITSTREAM *ct_sbrBitStr )
{

  ct_sbrBitStr->NrElements     = 0;
  ct_sbrBitStr->NrElementsCore = 0;

}


static void
sbrSetElementType( SBRBITSTREAM *ct_sbrBitStr, SBR_ELEMENT_ID Type )
{

  ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].ElementID = Type;

}


#endif /* CT_SBR */

static void fillSacDecUsacMps212Config(SAC_DEC_USAC_MPS212_CONFIG *pSacDecUsacMps212Config, USAC_MPS212_CONFIG *pUsacMps212Config){

  pSacDecUsacMps212Config->bsFreqRes              = pUsacMps212Config->bsFreqRes.value;
  pSacDecUsacMps212Config->bsFixedGainDMX         = pUsacMps212Config->bsFixedGainDMX.value;
  pSacDecUsacMps212Config->bsTempShapeConfig      = pUsacMps212Config->bsTempShapeConfig.value;
  pSacDecUsacMps212Config->bsDecorrConfig         = pUsacMps212Config->bsDecorrConfig.value;
  pSacDecUsacMps212Config->bsHighRateMode         = pUsacMps212Config->bsHighRateMode.value;
  pSacDecUsacMps212Config->bsPhaseCoding          = pUsacMps212Config->bsPhaseCoding.value;
  pSacDecUsacMps212Config->bsOttBandsPhasePresent = pUsacMps212Config->bsOttBandsPhasePresent.value;
  pSacDecUsacMps212Config->bsOttBandsPhase        = pUsacMps212Config->bsOttBandsPhase.value;
  pSacDecUsacMps212Config->bsResidualBands        = pUsacMps212Config->bsResidualBands.value;
  pSacDecUsacMps212Config->bsPseudoLr             = pUsacMps212Config->bsPseudoLr.value;
  pSacDecUsacMps212Config->bsEnvQuantMode         = pUsacMps212Config->bsEnvQuantMode.value;      
 
  return;
}


/* -------------------------------------- */
/*           Init USAC-Decoder             */
/* -------------------------------------- */

HANDLE_USAC_DECODER USACDecodeInit (int samplingRate,
                                   char *usacDebugStr,
                                   int  block_size_samples,
                                   Info *** sfbInfo,
                                   int  profile,
                                   AUDIO_SPECIFIC_CONFIG *streamConfig)
{
  int i;

  HANDLE_USAC_DECODER hDec = (HANDLE_USAC_DECODER) calloc (1,  sizeof(T_USAC_DECODER));
  int elemIdx = 0;

  USAC_CONFIG *pUsacConfig = &(streamConfig->specConf.usacConfig);
  USAC_DECODER_CONFIG * pUsacDecoderConfig = &(streamConfig->specConf.usacConfig.usacDecoderConfig);
  int const nElements = pUsacDecoderConfig->numElements;

#ifndef USAC_REFSOFT_COR1_NOFIX_04
  int outputFrameLength = UsacConfig_GetOutputFrameLength(streamConfig->specConf.usacConfig.coreSbrFrameLengthIndex.value);
  int const samplingRateOrig = samplingRate;
#endif


  if (!hDec) return hDec ;

  hDec->blockSize= block_size_samples;

  hDec->mip = &mc_info;
  hDec->short_win_in_long = 8;

#ifdef CT_SBR
  hDec->ct_sbrBitStr[0].NrElements     =  0;
  hDec->ct_sbrBitStr[0].NrElementsCore =  0;
  hDec->ct_sbrBitStr[1].NrElements     =  0;
  hDec->ct_sbrBitStr[1].NrElementsCore =  0;
#endif

  for(i=0; i<Chans; i++)
  {
    hDec->coef[i] = (float *)mal1((LN2 + LN2/8)*sizeof(*hDec->coef[0]));
    hDec->data[i] = (float *)mal1(LN2*sizeof(*hDec->data[0]));
    hDec->factors[i] = (short *)mal1(MAXBANDS*sizeof(*hDec->factors[0]));
    hDec->state[i] = (float *)mal1(LN*sizeof(*hDec->state[0]));	/* changed LN4 to LN 1/97 mfd */
    fltclrs(hDec->state[i], LN);
    hDec->cb_map[i] = (byte *)mal1(MAXBANDS*sizeof(*hDec->cb_map[0]));
    hDec->group[i] = (byte *)mal1(NSHORT*sizeof(*hDec->group[0]));

    hDec->tns[i] = (TNS_frame_info *)mal1(sizeof(*hDec->tns[0]));
    hDec->wnd_shape[i].prev_bk = WS_FHG;
    hDec->tw_ratio[i] = (int *) mal1(NUM_TW_NODES*sizeof(*hDec->tw_ratio[0]));


#ifndef USAC_REFSOFT_COR1_NOFIX_03
    hDec->coefSave[i]= (float*)mal1((LN2 + LN2/8)*sizeof(*hDec->coefSave[0]));
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */

#ifndef RANDOMSIGN_NO_BUGFIX
    hDec->nfSeed[i] = 0x0;
#else /* RANDOMSIGN_NO_BUGFIX */

#ifdef UNIFIED_RANDOMSIGN
    if (i%2 == 0) {
      hDec->nfSeed[i] = 0x3039;
    } else {
      hDec->nfSeed[i] = 0x10932;
    }
#else
    hDec->nfSeed[i] = 12345;
#endif

#endif /* RANDOMSIGN_NO_BUGFIX */

    hDec->ApplyFAC[i]=0;
  }

  for(i=0; i<Winds; i++)
  {
    hDec->mask[i] = (byte *)mal1(MAXBANDS*sizeof(hDec->mask[0]));
  }

  /* set defaults */

  current_program = -1;

  /* multi-channel info is invalid so far */

  mc_info.mcInfoCorrectFlag = 0;
  mc_info.profile = profile ;
  mc_info.sampling_rate_idx = Fs_48;

  /* reset all debug options */

  for (i=0;i<256;i++)
    debug[i]=0;

  if (usacDebugStr)
  {
    for (i=0;i<(int)strlen(usacDebugStr);i++){
      debug[(int)usacDebugStr[i]]=1;
      fprintf(stderr,"   !!! USAC debug option: %c recognized.\n", usacDebugStr[i]);
    }
  }

  /* Set sampling rate */
#ifndef USAC_REFSOFT_COR1_NOFIX_04
  samplingRate = (outputFrameLength == 768) ? ((samplingRateOrig * 4) / 3) : samplingRateOrig;
#endif

  for (i=0; i<(1<<LEN_SAMP_IDX); i++) {
    if (sampling_boundaries[i] <= samplingRate)
      break;
  }

  if (i == (1<<LEN_SAMP_IDX))
    CommonExit(1,"Unsupported sampling frequency %d", samplingRate);

  prog_config.sampling_rate_idx = mc_info.sampling_rate_idx = i;

  hDec->samplingRate = samplingRate;
  hDec->fscale = (int) ( (double) hDec->samplingRate * (double) FSCALE_DENOM / 12800.0f );

  for(i = 0; i < Chans; i++)
  {
    hDec->wnd_shape[i].prev_bk = WS_FHG;
    hDec->wnd_shape[i].this_bk = WS_FHG;
  }


  huffbookinit(block_size_samples, mc_info.sampling_rate_idx);
  usac_infoinit(&usac_samp_rate_info[mc_info.sampling_rate_idx], block_size_samples);
  usac_winmap[0] = usac_win_seq_info[ONLY_LONG_SEQUENCE];
  usac_winmap[1] = usac_win_seq_info[ONLY_LONG_SEQUENCE];
  usac_winmap[2] = usac_win_seq_info[EIGHT_SHORT_SEQUENCE];
  usac_winmap[3] = usac_win_seq_info[ONLY_LONG_SEQUENCE];
  usac_winmap[4] = usac_win_seq_info[ONLY_LONG_SEQUENCE]; /* STOP_START_SEQUENCE */

  *sfbInfo = usac_winmap;

  for ( i= 0 ; i < Chans ; i++ ) {
    hDec->tddec[i] = (USAC_TD_DECODER *)mal1(sizeof(*hDec->tddec[i]));
#ifndef USAC_REFSOFT_COR1_NOFIX_04
    if(outputFrameLength == 768){
      hDec->tddec[i]->fscale = samplingRateOrig;
    } else {
      hDec->tddec[i]->fscale = ((hDec->fscale)*block_size_samples)/L_FRAME_1024;
    }
#else
    hDec->tddec[i]->fscale = ((hDec->fscale)*block_size_samples)/L_FRAME_1024;
#endif
    hDec->tddec[i]->lFrame = block_size_samples;
    hDec->tddec[i]->lDiv    = block_size_samples/NB_DIV;
    hDec->tddec[i]->nbSubfr = (NB_SUBFR_1024*block_size_samples)/L_FRAME_1024;
    init_decoder_lf(hDec->tddec[i]);

#ifndef RANDOMSIGN_NO_BUGFIX
    hDec->tddec[i]->seed_tcx = 0x0;
#else /* RANDOMSIGN_NO_BUGFIX */
#ifdef UNIFIED_RANDOMSIGN
    if (i%2 == 0) {
      hDec->tddec[i]->seed_tcx = 0x3039;
    } else {
      hDec->tddec[i]->seed_tcx = 0x10932;
    }
#endif
#endif /* RANDOMSIGN_NO_BUGFIX */
  }




  for(elemIdx = 0; elemIdx < nElements; elemIdx++){
    USAC_CORE_CONFIG *pUsacCoreConfig = UsacConfig_GetUsacCoreConfig(pUsacConfig, elemIdx);
    USAC_SBR_CONFIG  *pUsacSbrConfig  = UsacConfig_GetUsacSbrConfig(pUsacConfig, elemIdx);
    USAC_ELEMENT_TYPE elemType = UsacConfig_GetUsacElementType(pUsacConfig, elemIdx);
    int stereoConfigIndex = UsacConfig_GetStereoConfigIndex(pUsacConfig, elemIdx);

    if(pUsacCoreConfig){
      hDec->twMdct[elemIdx] = pUsacCoreConfig->tw_mdct.value;
      if ( hDec->twMdct[elemIdx]) {
         tw_init();
      }

      /* Noise Filling */
      hDec->bUseNoiseFilling[elemIdx] = pUsacCoreConfig->noiseFilling.value;
    }

    SacOpenAnaFilterbank(&hDec->filterbank[elemIdx]);

    switch(elemType){
    case USAC_ELEMENT_TYPE_SCE:
      break;

    case USAC_ELEMENT_TYPE_CPE:
      /* MPS212 */
      if(stereoConfigIndex > 0){
        int nTimeSlots = 0;
        int bsFrameLength =  UsacConfig_GetMps212NumSlots(pUsacConfig->coreSbrFrameLengthIndex.value)-1;
        int bsResidualCoding = (stereoConfigIndex > 1)?1:0;
        int nBitsRead = 0;
        const int qmfBands = 64;
        SAC_DEC_USAC_MPS212_CONFIG sacDecUsacMps212Config;

        fillSacDecUsacMps212Config(&sacDecUsacMps212Config, &(pUsacConfig->usacDecoderConfig.usacElementConfig[elemIdx].usacCpeConfig.usacMps212Config));

        if(hSpatialDec[elemIdx]){
          SpatialDecClose(hSpatialDec[elemIdx]);
          hSpatialDec[elemIdx] = NULL;
        }

        hSpatialDec[elemIdx] = SpatialDecOpen(NULL,
                                     "",
                                     pUsacConfig->usacSamplingFrequency.value,
                                     1+((bsResidualCoding)?1:0), /* asc->channelConfiguration.value */
                                     &nTimeSlots,
                                     qmfBands,
                                     bsFrameLength,
                                     bsResidualCoding,
                                     gUseFractionalDelayDecor,
                                     0,
                                     0,
                                     1,
                                     68,
                                     &nBitsRead,
                                     &sacDecUsacMps212Config);
      }
      break;

    case USAC_ELEMENT_TYPE_LFE:
      break;
    case USAC_ELEMENT_TYPE_EXT:
      break;
    default:
      CommonExit(1, "Invalid usac element type: %d", elemType);
      break;
    }

    if(USAC_ELEMENT_TYPE_EXT != elemType){
#ifndef RANDOMSIGN_NO_BUGFIX
  {
    int ch1 = 0;
    int const tag = 0; 
    MC_Info tmp_mip;
    memset(&tmp_mip, 0, sizeof(tmp_mip));
    if ((ch1 = chn_config(elemType, tag, 0 /* common_window: dummy */, &tmp_mip)) < 0) {
      CommonWarning("Number of channels negativ (decode_chan_ele.c)");
    } else {
      hDec->tddec[ch1]->seed_tcx = 0x3039;
      hDec->nfSeed[ch1] = 0x3039;
      if(USAC_ELEMENT_TYPE_CPE == elemType){
        int ch2 = tmp_mip.ch_info[ch1].paired_ch;
        hDec->tddec[ch2]->seed_tcx = 0x10932;
        hDec->nfSeed[ch2] = 0x10932;
      }
    }
  }
#endif

    }



  }








  return hDec ;
}

/* ------------------------------------------ */
/*  Release data allocated by AACDecoderInit  */
/* ------------------------------------------ */

void USACDecodeFree (HANDLE_USAC_DECODER hDec)
{
  int i ;

  for(i=0; i<Chans; i++)
  {
    free (hDec->state[i]);
    free (hDec->coef[i]) ;
    free (hDec->data[i]) ;
    free (hDec->factors[i]) ;
    free (hDec->cb_map[i]) ;
    free (hDec->group[i]) ;
    free (hDec->tns[i]) ;
    free (hDec->tw_ratio[i]);

#ifndef USAC_REFSOFT_COR1_NOFIX_03
    free (hDec->coefSave[i]);
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */

    close_decoder_lf(hDec->tddec[i]);
    if(hDec->tddec[i]) free (hDec->tddec[i]);
  }

  if (NULL != last_rstgrp_num) {
    free (last_rstgrp_num);
  }
  for(i=0; i<Winds; i++)
    free (hDec->mask[i]) ;

  for(i = 0; i < MAX_NUM_ELEMENTS; i++){
    if(hSpatialDec[i]){
      SpatialDecClose(hSpatialDec[i]);
      hSpatialDec[i] = NULL;

    }
  }
  for(i=0; i<sizeof(hDec->filterbank)/sizeof(hDec->filterbank[0]); i++){
    if(hDec->filterbank[i]){
      SacCloseAnaFilterbank(hDec->filterbank[i]);
    }
  }

  free (hDec) ;
}

/* -------------------------------------- */
/*           DECODE AUDIO PRE ROLL        */
/* Here we parse and then discard the Pre */
/* Roll AU's because we can't switch      */
/* streams. This code is mainly exemplary */
/* -------------------------------------- */
static int parseAudioPreRollAndDiscard(USAC_EXT_ELEMENT pUsacExtElement, USAC_CONFIG *pUsacConfig, HANDLE_STREAM_AU *prerollAU, unsigned int* numPrerollAU, unsigned int* applyCrossFade){

  USAC_CONFIG UsacConfigPreRoll;
  HANDLE_BSBITBUFFER bb = NULL;
  HANDLE_BSBITSTREAM bs;
  unsigned int i, nBitsToSkip, ascBitCounter = 0;
  int retVal = 0;
  unsigned int ascSize = 0;
  unsigned long useDefaultLengthFlag = 0;
  unsigned long dummy = 0;
  
  *applyCrossFade = 0;

  memcpy(&UsacConfigPreRoll, pUsacConfig, sizeof(USAC_CONFIG));

  bb = BsAllocPlainDirtyBuffer(pUsacExtElement.usacExtElementSegmentData,pUsacExtElement.usacExtElementPayloadLength*8);
  bs = BsOpenBufferRead(bb);

  /* read ASC size */
  UsacConfig_ReadEscapedValue(bs, &ascSize, 4, 4, 8);

  /* read ASC */
  if ( ascSize )
    ascBitCounter = UsacConfig_Advance(bs, &(UsacConfigPreRoll), 0);

  /* Skip remaining bits from ASC that were not parsed */
  nBitsToSkip  =  ascSize * 8 - ascBitCounter;
  while ( nBitsToSkip ) {
    int nMaxBitsToRead = min(nBitsToSkip, 32);
    BsGetBit(bs, &dummy, nMaxBitsToRead);
    nBitsToSkip -= nMaxBitsToRead;
  }

  BsGetBit(bs, (unsigned long*)applyCrossFade, 1);
  BsGetBit(bs, &dummy, 1);

  /* read num preroll AU's */
  UsacConfig_ReadEscapedValue(bs, numPrerollAU, 2, 4, 0);

  for ( i = 0; i < *numPrerollAU; ++i ) {
    unsigned int auLength = 0;
    unsigned int j = 0;

    /* For every AU get length and allocate memory  to hold the data */
    UsacConfig_ReadEscapedValue(bs, &auLength, 16, 16, 0);
    prerollAU[i] = StreamFileAllocateAU(auLength * 8);

    for ( j = 0; j < auLength; ++j ) {
      unsigned long auByte = 0;
      /* Read AU bytewise and copy into pre roll AU data buffer */
      BsGetBit(bs, &auByte, 8);
      memcpy(&(prerollAU[i]->data[j]), &auByte, 1 ); 
    }
  }

  return retVal;
}

/* -------------------------------------- */
/*           DECODE 1 frame               */
/* -------------------------------------- */
int USACDecodeFrame (USAC_DATA               *usacData,
                     HANDLE_BSBITSTREAM       fixed_stream,
                     float*                  spectral_line_vector[MAX_TIME_CHANNELS],
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int*                     numOutChannels,
                     Info**                   sfbInfo,
                     byte                     sfbCbMap[MAX_TIME_CHANNELS][MAXBANDS],
                     short*                   pFactors[MAX_TIME_CHANNELS],
                     HANDLE_DECODER_GENERAL   hFault,
                     QC_MOD_SELECT            qc_select,
                     FRAME_DATA*              fd

)
{
  HANDLE_USAC_DECODER hDec = usacData->usacDecoder;
  MC_Info *mip = hDec->mip ;
  Ch_Info *cip ;

  byte hasmask [Winds] ;

  int           left=0, right=0, ele_id=0, wn=0, ch=0 ;
  int 		i=0;
  int           chCnt=0;
  int           outCh=0;
  int           sfb=0;
  Info*         sfbInfoP;
  Info          sfbInfoNScale;
  unsigned char nrOfSignificantElements = 0; /* SCE (mono) and CPE (stereo) only */
  int           instance=0;


#ifdef SONY_PVC_DEC
  int	core_mode;
#endif /* SONY_PVC_DEC */

  USAC_CONFIG *pUsacConfig = &(fd->od->ESDescriptor [fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig);

  int samplFreqIdx = mip->sampling_rate_idx;

  HANDLE_BUFFER            hVm              = hFault[0].hVm;
  HANDLE_RESILIENCE        hResilience      = hFault[0].hResilience;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData = hFault[0].hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment     = hFault[0].hConcealment;
  float **sbrQmfBufferReal[2];
  float **sbrQmfBufferImag[2];

  USAC_DECODER_CONFIG * pUsacDecoderConfig = &(fd->od->ESDescriptor[fd->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.usacConfig.usacDecoderConfig);
  int nInstances = pUsacDecoderConfig->numElements;
  int elementTag[8] = {0};


  if(debug['n']) {
    /*     fprintf(stderr, "\rblock %ld", bno); */
    fprintf(stderr,"\n-------\nBlock: %ld\n", bno);
  }

  /* Set the current bitStream for usac-decoder */
  setHuffdec2BitBuffer(fixed_stream);

  reset_mc_info(mip);

  nrOfSignificantElements++;
  sfbInfoP = &sfbInfoNScale;

  if(debug['v'])
    fprintf(stderr, "\nele_id %d\n", ele_id);

  BsSetSynElemId (ele_id) ;


  /* read usac independency flag */
  usacData->usacIndependencyFlag = GetBits( LEN_USAC_INDEP_FLAG,
                                            USAC_INDEP_FLAG,
                                            hFault[0].hResilience,
                                            hFault[0].hEscInstanceData,
                                            hFault[0].hVm );

  for(instance = 0; instance < nInstances; instance++){
    int stereoConfigIndex = UsacConfig_GetStereoConfigIndex(pUsacConfig, instance);
    int numChannelsPerElement = 0;
    int channelOffset = 0;
    int nBitsReadSBR = 0;
    /* get audio syntactic element */

#ifdef CT_SBR
    ResetSbrBitstream( &hDec->ct_sbrBitStr[0] );
    ResetSbrBitstream( &hDec->ct_sbrBitStr[1] );
#endif

    switch(pUsacDecoderConfig->usacElementType[instance]){
    case USAC_ELEMENT_TYPE_SCE:
      ele_id = ID_SCE;
      numChannelsPerElement = 1;
      break;
    case USAC_ELEMENT_TYPE_CPE:
      ele_id = (stereoConfigIndex == 1) ? ID_SCE : ID_CPE;
      numChannelsPerElement = (stereoConfigIndex == 1) ? 1 : 2;
      break;
    case USAC_ELEMENT_TYPE_LFE:
      ele_id = ID_LFE;
      numChannelsPerElement = 1;
      break;
    case USAC_ELEMENT_TYPE_EXT:
      ele_id = ID_FIL;
      break;
    default:
      CommonExit(1,"Unknown usac element type (decoder_usac.c)");
      break;
    }

    switch (ele_id) {
    case ID_SCE:                /* single channel */
    case ID_CPE:                /* channel pair */
    case ID_LFE:                /* low frequency enhancement */

      if (0 > decodeChannelElement ( ele_id,
                                     mip,
                                     usacData,
                                     hDec->wnd,
                                     hDec->wnd_shape,
                                     hDec->cb_map,
                                     hDec->factors,
                                     hDec->group,
                                     hasmask,
                                     hDec->mask,
                                     max_sfb,
                                     hDec->tns,
                                     hDec->coef,
                                     sfbInfoP,
                                     qc_select,
                                     hFault,
                                     elementTag[ele_id],
                                     hDec->bUseNoiseFilling[instance],
                                     &channelOffset
#ifdef SONY_PVC_DEC
									,&core_mode
#endif /* SONY_PVC_DEC */
#ifndef CPLX_PRED_NOFIX
                                    ,stereoConfigIndex                  
#endif /* CPLX_PRED_NOFIX */
                                     ) ) {
        CommonExit(1,"decodeChannelElement (decoder.c)");
      }
      elementTag[ele_id]++;
#ifdef CT_SBR

      if ( ele_id == ID_SCE ) {
        sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_SCE );
        sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_SCE );
      }
      if ( ele_id == ID_CPE ) {
        if ( (stereoConfigIndex == 0) || (stereoConfigIndex == 3) ) {
          sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_CPE );
          sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_CPE );
        }
        else {
          sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_SCE );
          sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_SCE );
        }
      }
      if( ele_id == ID_LFE) {
        sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_SCE );
        sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_SCE );

        hDec->ct_sbrBitStr[0].NrElements++;
        hDec->ct_sbrBitStr[0].NrElementsCore++;
        hDec->ct_sbrBitStr[0].sbrElement[0].Payload = 0;
        /* hDec->ct_sbrBitStr[0]->sbrElement[0].ExtensionType = */
      }


      if (usacData->bPseudoLr[instance] && (stereoConfigIndex > 1) ) {
        /* Pseudo L/R to Dmx/Res rotation */
        float tmp;
        static const int l = 1;
        static const int r = 2;
        for (i = 0; i < usacData->block_size_samples; i++) {
          tmp = (usacData->time_sample_vector[l][i] + usacData->time_sample_vector[r][i])/sqrt(2.0f);
          usacData->time_sample_vector[r][i] =
              (usacData->time_sample_vector[l][i] - usacData->time_sample_vector[r][i])/sqrt(2.0f);
          usacData->time_sample_vector[l][i] = tmp;
        }
      }

      /* convert time data from double to float */
      {
        int osf=1;
        for(i = 0; i < osf * (int)usacData->block_size_samples; i++){
          usacData->sampleBuf[chCnt][i] = usacData->time_sample_vector[channelOffset][i];
          if(numChannelsPerElement == 2){
            usacData->sampleBuf[chCnt+1][i] = usacData->time_sample_vector[channelOffset+1][i];
          }
        }
      }

      if(ele_id != ID_LFE){
      /* sbr_extension_data() */
        if(usacData->sbrRatioIndex > 0){
          unsigned int au;
          int payloadLength = 0;
          int AuLength = 0;

          int AacPayloadLength = BsCurrentBit(fixed_stream);
          for (au = 0; au < fd->layer[0].NoAUInBuffer; au++) {
            AuLength += (fd->layer[0].AULength[au] );
            if(AuLength>=AacPayloadLength)
              break;
          }
          payloadLength = AuLength - AacPayloadLength;

          getSbrExtensionData(hResilience,
              hEscInstanceData,
              hVm,
              &hDec->ct_sbrBitStr[0],
              &payloadLength );

        }
      }


      /**********************************************************/
      /* Functions for SBR Decoder                              */
      /**********************************************************/

      {
        SBRBITSTREAM* hCt_sbrBitStr;

        usacData->runSbr = 0;
        if(usacData->sbrRatioIndex > 0) {
          hCt_sbrBitStr = &hDec->ct_sbrBitStr[0];
          usacData->runSbr = 1;

          if ( hDec != NULL ) {
            if (hCt_sbrBitStr[0].NrElements != 0 ) {

              {
#ifdef SBR_SCALABLE
                /* just useful for scalable SBR */
                int maxSfbFreqLine = getMaxSfbFreqLine( usacData->aacDecoder );
#endif

#ifdef SONY_PVC_DEC
                int sbr_modeTmp = usacData->sbr_mode;
#endif /* SONY_PVC_DEC */

                if ( usac_applySBR(usacData->ct_sbrDecoder,
                    hCt_sbrBitStr,
                    usacData->sampleBuf,
                    usacData->sampleBuf,
                    sbrQmfBufferReal,/* qmfBufferReal[64][128] */
                    sbrQmfBufferImag,/* qmfBufferImag[64][128] */
#ifdef SBR_SCALABLE
                    maxSfbFreqLine,
#endif
                    usacData->bDownSampleSbr,
#ifdef PARAMETRICSTEREO
                    usacData->sbrEnablePS,
#endif
                    stereoConfigIndex,
                    numChannelsPerElement, instance, chCnt
                    ,0 /* core_bandwidth */
                    ,0 /* bUsedBSAC */
                    ,&nBitsReadSBR
#ifdef SONY_PVC_DEC
                    ,core_mode
                    ,&sbr_modeTmp
#endif /* SONY_PVC_DEC */
                    ,usacData->usacIndependencyFlag
                ) != SBRDEC_OK ) {
                  CommonExit(1,"invalid sbr bitstream\n");
                }

#ifdef SONY_PVC_DEC
                usacData->sbr_mode = sbr_modeTmp;
#endif /* SONY_PVC_DEC */

#ifdef PARAMETRICSTEREO
                if ( usacData->sbrEnablePS == 1 ){
                  *numOutChannels = 2;
                }
#endif
                /*            framecnt++; */
              }
            }
            else{
              usacData->runSbr = 0;
            }
          }/* usacDecoder != NULL */
        } /* sbrPresentFlag */
      } /* SBR processing block */

      /* now skip bits really needed by SBR */
      SkipBits(hVm, nBitsReadSBR);

      /**********************************************************/
      /* MPEGS 212                                              */
      /**********************************************************/

      if (stereoConfigIndex > 0){

        float **inPointer[2*2];

        int numTimeSlots = 0;
        int ch,ts,i;
        int nBitsRead;
        int qmfBands;
        HANDLE_BSBITBUFFER bsBitBuffer = GetRemainingBufferBits(hVm);
        int nBitsInBuffer = BsBufferNumBit(bsBitBuffer);
        HANDLE_BSBITSTREAM bsBitStream;
        HANDLE_BITSTREAM_READER hBitstreamReader;
        HANDLE_BYTE_READER hByteReader;

        /* bit-reader inside MPEG-Surround reads byte-wise */
        BsBufferManipulateSetNumBit((((int)(nBitsInBuffer+7)/8)*8), bsBitBuffer);

        bsBitStream = BsOpenBufferRead(bsBitBuffer);
        hBitstreamReader = BitstreamReaderOpen(bsBitStream);
        hByteReader = BitstreamReaderGetByteReader(hBitstreamReader);


        *numOutChannels = SpatialDecGetNumOutputChannels(hSpatialDec[instance]);
        numTimeSlots    = SpatialDecGetNumTimeSlots(hSpatialDec[instance]);

        SpatialDecResetBitstream(hSpatialDec[instance],
            hByteReader);

        SpatialDecParseFrame(hSpatialDec[instance], &nBitsRead, usacData->usacIndependencyFlag);


        /*           for(ch=0; ch<numChannels; ch++){ */
        for(ch=0; ch<numChannelsPerElement; ch++){
          inPointer[2*ch]   = sbrQmfBufferReal[ch];
          inPointer[2*ch+1] = sbrQmfBufferImag[ch];
        }

        qmfBands = SpatialGetQmfBands(hSpatialDec[instance]);
        if ( qmfBands != 64 ){
          float *inSamplesDeinterleaved = (float*)malloc(qmfBands*sizeof(float));
          /* for (ch=0; ch<numChannels; ch++) { */
          for (ch=0; ch<numChannelsPerElement; ch++) {
            for (ts=0; ts<numTimeSlots; ts++) {
              for (i=0; i<qmfBands; i++) {
                inSamplesDeinterleaved[i] = usacData->sampleBuf[0][numChannelsPerElement* (ts*qmfBands+i)+ch]; 
              }

              SacCalculateAnaFilterbank( usacData->usacDecoder->filterbank[ch],
                  inSamplesDeinterleaved,
                  inPointer[2*ch][ts],
                  inPointer[2*ch+1][ts] );
            }
          }
        }

        SpatialDecApplyFrame(hSpatialDec[instance], numTimeSlots, inPointer, usacData->sampleBuf, 1.0f, instance);

        SkipBits(hVm, nBitsRead);

        BitstreamReaderClose(hBitstreamReader);
        if(bsBitStream) BsClose(bsBitStream);
        if(bsBitBuffer) BsFreeBuffer(bsBitBuffer);

      }

#endif
      chCnt += numChannelsPerElement;
      break;

    case ID_FIL:
      /* usacExtElement extension payload */
    {
      USAC_EXT_ELEMENT usacExtElement;
      USAC_EXT_CONFIG * pUsacExtConfig = &pUsacDecoderConfig->usacElementConfig[instance].usacExtConfig;

      memset(&usacExtElement, 0, sizeof(USAC_EXT_ELEMENT));
      __ReadUsacExtElement(pUsacExtConfig, &usacExtElement, hVm);

      if (pUsacExtConfig->usacExtElementType == USAC_ID_EXT_ELE_UNI_DRC) { 
        /* write to tmpFileUSACdec_drc_payload.bit */
        fwrite(usacExtElement.usacExtElementSegmentData, sizeof(unsigned char), usacExtElement.usacExtElementPayloadLength, usacData->drc_payload_file.save_drc_file); 
      }
      if (pUsacExtConfig->usacExtElementType == USAC_ID_EXT_ELE_AUDIOPREROLL && usacExtElement.usacExtElementPresent == 1 && usacExtElement.usacExtElementUseDefaultLength == 0 && usacData->usacIndependencyFlag) { 
        unsigned int numPrerollAU, applyCrossFade, i;
        HANDLE_STREAM_AU prerollAU[MPEG_USAC_CORE_MAX_AU]; 
       
        for (i = 0; i < MPEG_USAC_CORE_MAX_AU; i++) {
          prerollAU[i] = (HANDLE_STREAM_AU)calloc(1, sizeof(struct StreamAccessUnit));
        }

        if (parseAudioPreRollAndDiscard(usacExtElement, pUsacConfig, prerollAU, &numPrerollAU, &applyCrossFade)) {
          fprintf(stderr, "Error in parseAudioPreRoll()");
        }

        for (i = 0; i < MPEG_USAC_CORE_MAX_AU; i++) {
          free(prerollAU[i]);
        }
      }
    }
      break;

    default:
      CommonExit(1, "Element not supported: %d\n", ele_id);

      break;
    }

    if ( ele_id == ID_SCE || ele_id == ID_CPE ) {
      nrOfSignificantElements++;
    }


    if(debug['v']) {
      fprintf(stderr, "\nele_id %d\n", ele_id);
    }
  }


  check_mc_info ( mip,
                  ( ! mc_info.mcInfoCorrectFlag && default_config ) ,
                  hEscInstanceData,
                  hResilience
  );






#ifdef CT_SBR
#ifndef SBR_SCALABLE
  if ( hDec->ct_sbrBitStr->NrElements ) {
    if ( hDec->ct_sbrBitStr->NrElements != hDec->ct_sbrBitStr->NrElementsCore ) {
      CommonExit(1,"number of SBR elements does not match number of SCE/CPEs");
    }
  }
#else
  if ( hDec->ct_sbrBitStr[0].NrElements ) {
    if ( hDec->ct_sbrBitStr[0].NrElements != hDec->ct_sbrBitStr[0].NrElementsCore ) {
      CommonExit(1,"number of SBR elements does not match number of SCE/CPEs");
    }
  }
#endif
#endif


  ConcealmentCheckClassBufferFullnessEP(GetReadBitCnt(hVm),
                                        hResilience,
                                        hEscInstanceData,
                                        hConcealment);

  if (ConcealmentGetEPprematurely(hConcealment))
    hEscInstanceData = ConcealmentGetEPprematurely(hConcealment);

  {
    int chanCnt = 0;
    for (ch=0; ch<Chans; ch++) {
      cip = &mip->ch_info[ch];
      if (cip->present) {
        windowSequence[chanCnt] = hDec->wnd[cip->widx];
        window_shape[chanCnt]   = hDec->wnd_shape[cip->widx].this_bk;
        chanCnt++;
      }
      if (chanCnt >= MAX_TIME_CHANNELS) {
        break;
      }
    }
  }

  /* moved to decodeChannelElement():
     m/s stereo, intensity, prediction, tns
  */





  bno++;

  return GetReadBitCnt ( hVm );
}




void usacAUDecode ( int numChannels,
                    FRAME_DATA* frameData,
                    USAC_DATA* usacData,
                    HANDLE_DECODER_GENERAL hFault,
                    int *numOutChannels
) {
  int i_ch, decoded_bits = 0;
  short* pFactors[MAX_TIME_CHANNELS] = {NULL};
  int i;
  int unreadBits = 0;

  HANDLE_BSBITSTREAM fixed_stream;
  HANDLE_USAC_DECODER  hDec = usacData->usacDecoder;


  int short_win_in_long = 8;

  byte max_sfb[Winds];
  byte dummy[MAX_TIME_CHANNELS][MAXBANDS];
  QC_MOD_SELECT qc_select;

  qc_select = USAC_AC;

  for (i=0; i<MAX_TIME_CHANNELS; i++) {
    pFactors[i] = (short *)mal1(MAXBANDS*sizeof(*pFactors[0]));
  }

  for ( i_ch = 0 ; i_ch < numChannels ; i_ch++ ) {
    if ( ! hFault->hConcealment || ! ConcealmentAvailable ( 0, hFault->hConcealment ) ) {
      usacData->windowSequence[i_ch] = ONLY_LONG_SEQUENCE;
    }
  }

  fixed_stream = BsOpenBufferRead (frameData->layer[0].bitBuf);
  /* inverse Q&C */

  decoded_bits += USACDecodeFrame ( usacData,
                                    fixed_stream,
                                    usacData->spectral_line_vector,
                                    usacData->windowSequence,
                                    usacData->windowShape,
                                    max_sfb,
                                    numChannels,
                                    numOutChannels,
                                    usacData->sfbInfo,
                                    dummy,/*sfbCbMap*/
                                    pFactors, /*pFactors */
                                    hFault,
                                    qc_select,
                                    frameData
  );

  unreadBits = (int) (*((frameData->layer)[0]).AULength) - decoded_bits;
  if (unreadBits < 0) {
    CommonExit(1, "Read too many bits (bitsleft %d)\n", unreadBits);
  }
  if (unreadBits > 7) {
    CommonExit(1, "Too many bits left after parsing (bitsleft %d)\n", unreadBits);
  }

  removeAU(fixed_stream,decoded_bits,frameData,0);
  BsCloseRemove(fixed_stream,1);

  for (i=0; i<MAX_TIME_CHANNELS; i++) {
    if(pFactors[i]) free (pFactors[i]);
  }
}


