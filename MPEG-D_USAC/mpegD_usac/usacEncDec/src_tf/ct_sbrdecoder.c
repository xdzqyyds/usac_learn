/****************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3
standards for reference purposes and its performance may not have been
optimized. This software module is an implementation of one or more tools as
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications
thereof for use in products claiming conformance to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International
Standards. ISO/IEC gives users the same free license to this software module or
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its
use may infringe existing patents. ISO/IEC have no liability for use of this
software module or modifications thereof. Copyright is not released for
products that do not conform to audiovisual and image-coding related ITU
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its
own purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrdecoder.c,v 1.20.6.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  SBR decoder frontend $Revision: 1.20.6.1 $
*/


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> /* for debugging ndf 20080904 */
#include <assert.h>


#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_sbrdecoder.h"
#include "ct_envextr.h"
#include "ct_sbrdec.h"
#include "ct_envdec.h"
#include "ct_sbrcrc.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif
#include "tf_mainHandle.h"
#include "extension_type.h"

#ifdef AAC_ELD
#include "obj_descr.h"
#endif


typedef struct
{
  int outFrameSize;

  SBR_SYNC_STATE syncState ;

  SBR_FRAME_DATA frameData;

} SBR_CHANNEL;

typedef struct sbrDecoderInstance
{
  SBR_CHANNEL SbrChannel[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];
#ifdef PARAMETRICSTEREO
  HANDLE_PS_DEC hParametricStereoDec;
#endif
  int bHarmonicSBR[MAX_NUM_ELEMENTS];
  int bs_interTes[MAX_NUM_ELEMENTS];
  int bs_pvc[MAX_NUM_ELEMENTS];

#ifdef SONY_PVC_DEC
  HANDLE_PVC_DATA	pvcData;
#endif /* SONY_PVC_DEC */

  SBR_HEADER_DATA SbrDfltHeader[MAX_NUM_ELEMENTS];

} sbrDecoderInstance ;


#ifdef AAC_ELD
static void initFrameData(SBR_CHANNEL *SbrChannel);
#endif

/*******************************************************************************
 Functionname: openSBR
 *******************************************************************************

 Description:

 Arguments:

 Return:

*******************************************************************************/

SBRDECODER openSBR (int sampleRate,
                    int bDownSampledSbr,
                    int coreCodecFrameSize,
                    SBR_RATIO_INDEX sbrRatioIndex
#ifdef AAC_ELD
                    ,int ldsbr
                    ,UINT8 *sbrHeader

#endif
                    ,int * bUseHBE
                    ,int * bs_interTes
                    ,int * bs_pvc
                    ,int   bUseHQtransposer
#ifdef HUAWEI_TFPP_DEC
                    ,int actATFPP
#endif
                    )
{
  int i, el ;

  HANDLE_ERROR_INFO err = NOERROR;

  SBRDECODER self ;
  SBR_CHANNEL *SbrChannel ;
  int numAnaQmfBands, numSynQmfBands = bDownSampledSbr?32:64;
  float upsampleFac = 2.0f;


  self = (SBRDECODER) calloc (1, sizeof (sbrDecoderInstance));
  if (!self) return NULL ;

  for(el = 0; el < MAX_NUM_ELEMENTS; el++){
    self->bHarmonicSBR[el] = bUseHBE[el];
    self->bs_interTes[el] = bs_interTes[el];
    self->bs_pvc[el] = bs_pvc[el];
  }


  switch(sbrRatioIndex){
  case SBR_RATIO_INDEX_2_1:
    numAnaQmfBands = 32;
    break;
  case SBR_RATIO_INDEX_8_3:
    numAnaQmfBands = 24;
    break;
  case SBR_RATIO_INDEX_4_1:
    numAnaQmfBands = 16;
    upsampleFac = 4.0f;
    break;
  default:
    assert(0);
    break;
  }


  /* Allocate memory and initialise the filterbanks, and set up the buffers
      so that the SBR-Tool can be used for at least up-sampling only.*/
  initSbrDec (sampleRate,
              bDownSampledSbr,
              coreCodecFrameSize,
              sbrRatioIndex
#ifdef AAC_ELD
              ,ldsbr
#endif
              ,bUseHBE
              ,bUseHQtransposer
#ifdef HUAWEI_TFPP_DEC
              ,actATFPP
#endif
              );
#ifdef PARAMETRICSTEREO
  tfInitPsDec  (&self->hParametricStereoDec, 32 /* noCols */);
#endif


  for(el = 0; el < MAX_NUM_ELEMENTS; el++){
    for(i = 0; i < MAX_NUM_CHANNELS; i++){
      SbrChannel = &self->SbrChannel[el][i];
      memset (SbrChannel, 0, sizeof (SBR_CHANNEL));

      /* Set the sync-state to UPSAMPLING, since given the initialisation above, that's all
             we can do for now.
       */
      SbrChannel->syncState    = UPSAMPLING;
      SbrChannel->outFrameSize = (coreCodecFrameSize*numSynQmfBands)/numAnaQmfBands;

      if(sbrRatioIndex == SBR_RATIO_INDEX_4_1) {
        SbrChannel->frameData.rate  = 4;
      } else {
        SbrChannel->frameData.rate  = 2;
      }
    }
  }

#ifdef SONY_PVC_DEC
    self->pvcData = pvc_get_handle();
    pvc_init_decode(self->pvcData);
    self->pvcData->pvcParam.prev_sbr_mode = USE_UNKNOWN_SBR;
#endif /* SONY_PVC_DEC */

  return self ;
}


/*******************************************************************************
 Functionname: initFrameData
 *******************************************************************************

 Description:

 Arguments:

 Return:

*******************************************************************************/
static void
initFrameData(SBR_CHANNEL *SbrChannel){

#ifdef SBR_SCALABLE
  SbrChannel->frameData.bStereoEnhancementLayerDecodablePrevE = 1;
  SbrChannel->frameData.bStereoEnhancementLayerDecodablePrevQ = 1;
#endif

#ifdef SONY_PVC_DEC
  SbrChannel->frameData.sin_start_for_cur_top = 0;
  SbrChannel->frameData.sin_len_for_next_top = 0;
  SbrChannel->frameData.sin_start_for_next_top = 0;
  SbrChannel->frameData.sin_len_for_cur_top = 0;
#endif /* SONY_PVC_DEC */

  switch (SbrChannel->outFrameSize){
  case 4096:
  case 2048:
  case 1024:
    SbrChannel->frameData.numTimeSlots = 16;
    break;
  case 3840:
  case 1920:
  case 960:
    SbrChannel->frameData.numTimeSlots = 15;
    break;
  };
}



/*******************************************************************************
 Functionname:closeSBR
 *******************************************************************************

 Description: deletes SBR decoder

 Arguments:

 Return:      none

*******************************************************************************/

void closeSBR (SBRDECODER self)
{

  deleteSbrDec();

  if(self){
#ifdef SONY_PVC_DEC
    pvc_free_handle(self->pvcData);
#endif /* SONY_PVC_DEC */

    free (self);
  }
}




/*******************************************************************************
 Functionname:readSBR
 *******************************************************************************

 Description:

 Arguments:

 Return:      none

*******************************************************************************/
static SBR_ERROR readSBR (SBRDECODER self, SBRBITSTREAM *stream, int nChannels)
{
  HANDLE_ERROR_INFO err = NOERROR ;

  int element;
  int lr, sbrChannelIndex = 0;
  int CRCLen, SbrFrameOK = 1;
  int sbrCRCAlwaysOn = 0;
  int bs_header_flag = 0;

  SBR_HEADER_STATUS headerStatus = HEADER_OK;


  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0][0] ;  

  /* eval Bitstream */
  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements; element++)
  {

    BIT_BUFFER bitBuf ;

#ifdef SBR_SCALABLE
    BIT_BUFFER bitBufEnh, *pBitBufEnh;
    int bStereoLayer = 0;

    if(stream[1].NrElements>0) {
      initBitBuffer (&bitBufEnh, stream[1].sbrElement[element].Data,
                     stream[1].sbrElement[element].Payload * 8) ;
      pBitBufEnh = &bitBufEnh;
      bStereoLayer =1;
    }
    else{
      pBitBufEnh = &bitBuf;
      if(nChannels==2)
        bStereoLayer=1;
    }

#endif

    initBitBuffer (&bitBuf, stream->sbrElement[element].Data,
                   stream->sbrElement[element].Payload * 8) ;


    /* we have to skip a nibble because the first element of Data only
       contains a nibble of data ! */
#ifdef AAC_ELD
     if (SbrChannel->frameData.ldsbr != 1)
#endif
     BufGetBits (&bitBuf, LEN_NIBBLE);

#ifdef SBR_SCALABLE
     if(&bitBuf != pBitBufEnh) {
       BufGetBits (pBitBufEnh, LEN_NIBBLE);
     }
#endif


    /* read control data */

    if (stream->sbrElement[element].ExtensionType == EXT_SBR_DATA_CRC || sbrCRCAlwaysOn) {
      CRCLen = 8*(stream->sbrElement[element].Payload-1)+4 - SI_SBR_CRC_BITS;
      SbrFrameOK = SbrCrcCheck (&bitBuf, CRCLen);
    }

    if (SbrFrameOK){
      /*
      The sbr data seems ok, if the header flag is set we read the header and check
      if vital parameters have changed since the previous frame. If the syncState equals
      UPSAMPLING, the SBR Tool has not been initialised by SBR header data, and can only do
      upsampling.
      */

      bs_header_flag = BufGetBits (&bitBuf, 1); /* read Header flag */

      if (bs_header_flag) {

        /* If syncState == SBR_ACTIVE, it means that we've had a SBR header before, and we
        will compare with the previous header to see if a reset is required. If the syncState
        equals UPSAMPLING this means that the SBR-Tool so far is only initialised to do upsampling
        and hence we need to do a reset, and initialise the system according to the present header.*/

        headerStatus = sbrGetHeaderData (&(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                                         &bitBuf,
                                         SBR_ID_SCE,
                                         0,
                                         SbrChannel[sbrChannelIndex].syncState
										 );

      }

       switch (stream->sbrElement[element].ElementID){
          case SBR_ID_SCE :

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ){
              float upsampleFac = (float) SbrChannel[sbrChannelIndex].frameData.rate;
              resetSbrDec ( &(SbrChannel[sbrChannelIndex].frameData),
                            upsampleFac,
                            sbrChannelIndex,
                            element);

             /* At this point we have a header and the system has been reset,
                hence syncState from now on will be SBR_ACTIVE.*/
              SbrChannel[sbrChannelIndex].syncState = SBR_ACTIVE;
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
              sbrGetSingleChannelElement (&(SbrChannel[sbrChannelIndex].frameData),
                                          &bitBuf
#ifdef PARAMETRICSTEREO
                                          ,self->hParametricStereoDec
#endif
                                          );
            }
            break;

          case SBR_ID_CPE :

            if(bs_header_flag){
              memcpy (&(SbrChannel[sbrChannelIndex + 1].frameData.sbr_header),
                      &(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                      sizeof(SBR_HEADER_DATA) );
            }

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
              initFrameData(&SbrChannel[sbrChannelIndex + 1]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ) {
              for (lr = 0 ; lr < 2 ; lr++) {
                float upsampleFac = (float) SbrChannel[sbrChannelIndex+lr].frameData.rate;
                resetSbrDec ( &(SbrChannel[sbrChannelIndex+lr].frameData),
                              upsampleFac,
                              sbrChannelIndex+lr,
                              element);
                /* At this point we have a header and the system has been reset,
                   hence syncState from now on will be SBR_ACTIVE.*/
                SbrChannel[sbrChannelIndex+lr].syncState = SBR_ACTIVE;
              }
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
#ifdef SBR_SCALABLE
              sbrGetChannelPairElement (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf, pBitBufEnh,bStereoLayer);
#else
              sbrGetChannelPairElement (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf);
#endif
            }

            sbrChannelIndex++;
            break;

          default:
            return SBRDEC_ILLEGAL_PLUS_ELE_ID;
      }
    }

    sbrChannelIndex++;
  }

  return SBRDEC_OK ;
}


static SBR_ERROR readSBR_BSAC (SBRDECODER self, SBRBITSTREAM *stream, int nChannels)
{
  HANDLE_ERROR_INFO err = NOERROR ;

  int element;
  int lr, sbrChannelIndex = 0;
  int CRCLen, SbrFrameOK = 1;
  int sbrCRCAlwaysOn = 0;
  int bs_header_flag = 0;

  SBR_HEADER_STATUS headerStatus = HEADER_OK;


  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0][0] ; 

  /* eval Bitstream */
  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements; element++)
  {

    BIT_BUFFER bitBuf ;

#ifdef SBR_SCALABLE
    BIT_BUFFER bitBufEnh, *pBitBufEnh;
    int bStereoLayer = 0;

    if(stream[1].NrElements>0) {
      initBitBuffer (&bitBufEnh, stream[1].sbrElement[element].Data,
                     stream[1].sbrElement[element].Payload * 8) ;
      pBitBufEnh = &bitBufEnh;
      bStereoLayer =1;
    }
    else{
      pBitBufEnh = &bitBuf;
      if(nChannels==2)
        bStereoLayer=1;
    }

#endif

    initBitBuffer (&bitBuf, stream->sbrElement[element].Data,
                   stream->sbrElement[element].Payload * 8) ;

    /* we have to skip a nibble because the first element of Data only
       contains a nibble of data ! */
#ifdef SBR_SCALABLE
     if(&bitBuf != pBitBufEnh) {
       BufGetBits (pBitBufEnh, LEN_NIBBLE);
     }
#endif


    /* read control data */

    if (stream->sbrElement[element].ExtensionType == EXT_SBR_DATA_CRC || sbrCRCAlwaysOn) {
      CRCLen = 8*(stream->sbrElement[element].Payload-1)+4 - SI_SBR_CRC_BITS;
      SbrFrameOK = SbrCrcCheck (&bitBuf, CRCLen);
    }

    if (SbrFrameOK){
      /*
      The sbr data seems ok, if the header flag is set we read the header and check
      if vital parameters have changed since the previous frame. If the syncState equals
      UPSAMPLING, the SBR Tool has not been initialised by SBR header data, and can only do
      upsampling.
      */

      bs_header_flag = BufGetBits (&bitBuf, 1); /* read Header flag */

      if (bs_header_flag) {

        /* If syncState == SBR_ACTIVE, it means that we've had a SBR header before, and we
        will compare with the previous header to see if a reset is required. If the syncState
        equals UPSAMPLING this means that the SBR-Tool so far is only initialised to do upsampling
        and hence we need to do a reset, and initialise the system according to the present header.*/

        headerStatus = sbrGetHeaderData (&(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                                         &bitBuf,
                                         SBR_ID_SCE,
                                         0,
                                         SbrChannel[sbrChannelIndex].syncState
										 );

      }

       switch (stream->sbrElement[element].ElementID){
          case SBR_ID_SCE :

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ){
              float upsampleFac = (float) SbrChannel[sbrChannelIndex].frameData.rate;
              resetSbrDec ( &(SbrChannel[sbrChannelIndex].frameData),
                            upsampleFac,
                            sbrChannelIndex,
                            element);

             /* At this point we have a header and the system has been reset,
                hence syncState from now on will be SBR_ACTIVE.*/
              SbrChannel[sbrChannelIndex].syncState = SBR_ACTIVE;
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
              sbrGetSingleChannelElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                          &bitBuf);
            }
            break;

          case SBR_ID_CPE :

            if(bs_header_flag){
              memcpy (&(SbrChannel[sbrChannelIndex + 1].frameData.sbr_header),
                      &(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                      sizeof(SBR_HEADER_DATA) );
            }

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
              initFrameData(&SbrChannel[sbrChannelIndex + 1]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ) {
              for (lr = 0 ; lr < 2 ; lr++) {
                float upsampleFac = (float) SbrChannel[sbrChannelIndex+lr].frameData.rate;
                resetSbrDec ( &(SbrChannel[sbrChannelIndex+lr].frameData),
                              upsampleFac,
                              sbrChannelIndex+lr,
                              element);
                /* At this point we have a header and the system has been reset,
                   hence syncState from now on will be SBR_ACTIVE.*/
                SbrChannel[sbrChannelIndex+lr].syncState = SBR_ACTIVE;
              }
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
#ifdef SBR_SCALABLE
              sbrGetChannelPairElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf, pBitBufEnh,bStereoLayer);
#else
              sbrGetChannelPairElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf);
#endif
            }

            sbrChannelIndex++;
            break;

          default:
            return SBRDEC_ILLEGAL_PLUS_ELE_ID;
      }
    }

    sbrChannelIndex++;
  }

  return SBRDEC_OK ;
}



static SBR_ERROR usac_readSBR (SBRDECODER self, SBRBITSTREAM *stream, int nChannels, int el, int *pnBitsRead
#ifdef SONY_PVC_DEC
                               ,int core_mode
                               ,int *sbr_mode
                               ,int *varlen
#endif /* SONY_PVC_DEC */
                               ,int const bUsacIndependencyFlag
                               )
{
  HANDLE_ERROR_INFO err = NOERROR ;

  int element;
  int lr, sbrChannelIndex = 0;
  int CRCLen, SbrFrameOK = 1;
  int sbrCRCAlwaysOn = 0;
  int bs_header_flag = 0;
  int nBitsRead = 0;

  SBR_HEADER_STATUS headerStatus = HEADER_OK;


/*  SBR_CHANNEL *SbrChannel = &self->SbrChannel [el][0] ;*/

  /* eval Bitstream */
  sbrChannelIndex = 0;

  for (element = 0 ; element < stream->NrElements; element++)
  {
    int tmpBitsRead;
    BIT_BUFFER bitBuf ;

    initBitBuffer (&bitBuf, stream->sbrElement[element].Data,
                   stream->sbrElement[element].Payload * 8) ;

    tmpBitsRead = GetNrBitsAvailable(&bitBuf);

    /* we have to skip a nibble because the first element of Data only
       contains a nibble of data ! */
#ifdef AAC_ELD
     if (SbrChannel->frameData.ldsbr != 1)
#endif
       /* ndf 20080902 what is meant? extension_type? not transmitted in USAC */
       /*BufGetBits (&bitBuf, LEN_NIBBLE);*/

    /* read control data */

    if (stream->sbrElement[element].ExtensionType == EXT_SBR_DATA_CRC || sbrCRCAlwaysOn) {
      CRCLen = 8*(stream->sbrElement[element].Payload-1)+4 - SI_SBR_CRC_BITS;
      SbrFrameOK = SbrCrcCheck (&bitBuf, CRCLen);
    }

    if (SbrFrameOK){
      /*
      The sbr data seems ok, if the header flag is set we read the header and check
      if vital parameters have changed since the previous frame. If the syncState equals
      UPSAMPLING, the SBR Tool has not been initialised by SBR header data, and can only do
      upsampling.
      */

      bs_header_flag = 1;

      if (bs_header_flag) {

        /* If syncState == SBR_ACTIVE, it means that we've had a SBR header before, and we
        will compare with the previous header to see if a reset is required. If the syncState
        equals UPSAMPLING this means that the SBR-Tool so far is only initialised to do upsampling
        and hence we need to do a reset, and initialise the system according to the present header.*/

        headerStatus = sbrGetHeaderDataUsac (&self->SbrChannel[el][sbrChannelIndex].frameData.sbr_header/*&(SbrChannel[sbrChannelIndex].frameData.sbr_header*/,
                                             &self->SbrDfltHeader[el],
                                             &bitBuf,
                                             bUsacIndependencyFlag,
                                             self->bs_pvc[el],
                                             self->SbrChannel[el][sbrChannelIndex].syncState/*SbrChannel[sbrChannelIndex].syncState*/
                                             );

#ifdef SONY_PVC_DEC
        if((self->pvcData->pvcParam.pvc_mode_prev == 0) && (self->SbrChannel[el][sbrChannelIndex].frameData.sbr_header.pvc_brmode != 0)){
          self->pvcData->pvcParam.pvcID_prev = 0;
        }
        self->pvcData->pvcParam.pvc_mode_prev = self->SbrChannel[el][sbrChannelIndex].frameData.sbr_header.pvc_brmode;
#endif /* SONY_PVC_DEC */

      }

#ifdef SONY_PVC_DEC
	  if(self->SbrChannel[el][sbrChannelIndex].frameData.sbr_header.pvc_brmode == 0){
            *sbr_mode = USE_ORIG_SBR;
	  }else{
            *sbr_mode = USE_PVC_SBR;
	  }
#endif /* SONY_PVC_DEC */

       switch (stream->sbrElement[element].ElementID){
          case SBR_ID_SCE :

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && self->SbrChannel[el][sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&self->SbrChannel[el][sbrChannelIndex]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ){
              float upsampleFac = (float) self->SbrChannel[el][sbrChannelIndex].frameData.rate;
              resetSbrDec ( &(self->SbrChannel[el][sbrChannelIndex].frameData),
                            upsampleFac,
                            sbrChannelIndex,
                            el); /* was element*/


             /* At this point we have a header and the system has been reset,
                hence syncState from now on will be SBR_ACTIVE.*/
              self->SbrChannel[el][sbrChannelIndex].syncState = SBR_ACTIVE;
            }

#ifdef SONY_PVC_DEC
            if (self->SbrChannel[el][sbrChannelIndex].syncState == SBR_ACTIVE){
              if(*sbr_mode == USE_ORIG_SBR){
                usac_sbrGetSingleChannelElement (&(self->SbrChannel[el][sbrChannelIndex].frameData), &bitBuf, self->bHarmonicSBR[el], self->bs_interTes[el], self->pvcData, bUsacIndependencyFlag);
                *varlen = 0;
              }else if(*sbr_mode == USE_PVC_SBR){
                /* Read rearranged SBR */
                usac_sbrGetSingleChannelElement_for_pvc (&(self->SbrChannel[el][sbrChannelIndex].frameData), &bitBuf, self->bHarmonicSBR[el], *sbr_mode,
                                                         self->pvcData, varlen, bUsacIndependencyFlag
                                                         );
              }
            }

#else /* SONY_PVC_DEC */
            if (self->SbrChannel[el][sbrChannelIndex].syncState == SBR_ACTIVE){
              usac_sbrGetSingleChannelElement (&(self->SbrChannel[el][sbrChannelIndex].frameData), &bitBuf, self->bHarmonicSBR[el], self->bs_interTes[el], bUsacIndependencyFlag);
            }
#endif /* SONY_PVC_DEC */
            break;

          case SBR_ID_CPE :

            if(bs_header_flag){
              memcpy (&(self->SbrChannel[el][sbrChannelIndex + 1].frameData.sbr_header),
                      &(self->SbrChannel[el][sbrChannelIndex].frameData.sbr_header),
                      sizeof(SBR_HEADER_DATA) );
            }

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && self->SbrChannel[el][sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&self->SbrChannel[el][sbrChannelIndex]);
              initFrameData(&self->SbrChannel[el][sbrChannelIndex + 1]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ) {
              for (lr = 0 ; lr < 2 ; lr++) {
                float upsampleFac = (float) self->SbrChannel[el][sbrChannelIndex + lr].frameData.rate;
                resetSbrDec ( &(self->SbrChannel[el][sbrChannelIndex + lr].frameData),
                              upsampleFac,
                              sbrChannelIndex+lr,
                              el); /* was element*/
                /* At this point we have a header and the system has been reset,
                   hence syncState from now on will be SBR_ACTIVE.*/
                self->SbrChannel[el][sbrChannelIndex + lr].syncState = SBR_ACTIVE;
              }
            }

            if (self->SbrChannel[el][sbrChannelIndex].syncState == SBR_ACTIVE){
            	usac_sbrGetChannelPairElement (&(self->SbrChannel[el][sbrChannelIndex].frameData),
                                               &(self->SbrChannel[el][sbrChannelIndex + 1].frameData),
                                               &bitBuf, 
                                               self->bHarmonicSBR[el],
                                               self->bs_interTes[el],
                                               bUsacIndependencyFlag);
#ifdef SONY_PVC_DEC
                *varlen = 0;
#endif
            }

            sbrChannelIndex++;
            break;

          default:
            return SBRDEC_ILLEGAL_PLUS_ELE_ID;
      }
    }
    tmpBitsRead -= GetNrBitsAvailable(&bitBuf);
    nBitsRead += tmpBitsRead;

    sbrChannelIndex++;

  }

  *pnBitsRead = nBitsRead;

  return SBRDEC_OK ;
}


/*******************************************************************************
 Functionname:applySbr
 *******************************************************************************

 Description: sbr decoder processing,
              set up SBR decoder phase 2 in case of different cotrol data

 Arguments:

 Return:      errorCode, noError if successful

*******************************************************************************/
SBR_ERROR
applySBR (SBRDECODER self,
          SBRBITSTREAM * stream,
          float** timeData,
          float** timeDataOut,
          float **qmfBufferReal, /* qmfBufferReal[64][128] */
          float **qmfBufferImag, /* qmfBufferImag[64][128] */
#ifdef SBR_SCALABLE
          int maxSfbFreqLine,
#endif
          int bDownSampledSbr,
#ifdef PARAMETRICSTEREO
          int sbrEnablePS,
#endif
          int numChannels
          /* 20060107 */
          ,int	core_bandwidth
          ,int bUsedBSAC
          )
{
  SBR_ERROR err = SBRDEC_OK ;

  int element, eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex;
  int processedChannels[MAX_TIME_CHANNELS];
  int i;

  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0][0] ;  

  memset(&processedChannels,0,MAX_TIME_CHANNELS*sizeof(int));

  /* We have already done a check (outside this function) to make sure that SBR data exists.
     This is since this implementation of the implicit signalling does not assume that SBR
     exists, but rather checks for its existence, and adjusts the output sample rate if
     SBR data is found.

     However, if we had assumed that SBR data did exist, we would have to check now if it does
     i.e. stream->NrElements != 0, and if it doesn't we would have to use the SBR-Tool for upsampling,
     since we've already set up the output device at twice the sampling rate.
     */
  if (stream->NrElements)
  {
	  /* 20060107 */
	  if (bUsedBSAC==1)
			err = readSBR_BSAC (self, stream, numChannels) ;
	  else
			err = readSBR (self, stream, numChannels) ;
    if (err != SBRDEC_OK) return err ;
  }


  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements ; element++)
  {
    if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      eleChannels = (stream->sbrElement [element].ElementID == SBR_ID_CPE) ? 2 : 1 ;
      channelIndex = sbrChannelIndex;

      for(i=0; i<eleChannels; i++){
        decodeSbrData(SbrChannel[sbrChannelIndex + i].frameData.iEnvelope,
                      SbrChannel[sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec1,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec2,
                      SbrChannel[sbrChannelIndex + i].frameData.nSfb,
                      SbrChannel[sbrChannelIndex + i].frameData.frameInfo,
                      SbrChannel[sbrChannelIndex + i].frameData.nNfb,
                      SbrChannel[sbrChannelIndex + i].frameData.ampRes,
                      SbrChannel[sbrChannelIndex + i].frameData.nScaleFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.nNoiseFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.offset,
                      SbrChannel[sbrChannelIndex + i].frameData.coupling,
                      sbrChannelIndex + i,
                      element);
      }

      if(eleChannels == 2 && SbrChannel[sbrChannelIndex].frameData.coupling){
          sbr_envelope_unmapping (SbrChannel[sbrChannelIndex].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex+1].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex+1].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex].frameData.nScaleFactors,
                                  SbrChannel[sbrChannelIndex].frameData.nNoiseFactors,
                                  SbrChannel[sbrChannelIndex+1].frameData.ampRes);

      }

#ifdef PARAMETRICSTEREO
      if(sbrEnablePS /*eleChannels == 1 && SbrChannel[sbrChannelIndex].frameData.enablePS*/)
        tfDecodePs(self->hParametricStereoDec);
#endif

#ifdef SBR_SCALABLE
      eleChannels = numChannels;
#endif

      for(i=0; i<eleChannels; i++){

      	sbr_dec (timeData[channelIndex + i],
                 timeDataOut[channelIndex + i],
                 &(SbrChannel[sbrChannelIndex + i].frameData),
                 (SbrChannel[sbrChannelIndex + i].syncState==SBR_ACTIVE),
                 sbrChannelIndex + i,
                 element,
#ifdef SBR_SCALABLE
                 maxSfbFreqLine,
#endif
                 bDownSampledSbr
#ifdef PARAMETRICSTEREO
                 ,sbrEnablePS,
                 sbrEnablePS?timeDataOut[channelIndex + i + 1]:NULL,
                 self->hParametricStereoDec
#endif
                ,0
                 /* 20060107 */
                 ,core_bandwidth
                 ,bUsedBSAC
#ifdef SONY_PVC_DEC
                 ,NULL
                 ,USE_ORIG_SBR
                 ,0
#endif /* SONY_PVC_DEC */
                 ) ;

        processedChannels[channelIndex + i] = 1;
      }

      sbrChannelIndex += eleChannels;
    } /* if SBR_ACTIVE */
  } /* loop over elements */



  /*
  Do the LFE channel, if available, or channels were syncState != ACTIVE for which we do up-sampling only.
  */
  for (channelIndex = 0 ; channelIndex < numChannels ; channelIndex++)
  {
    if(!processedChannels[channelIndex]){
      sbr_dec (timeData[channelIndex],
               timeDataOut[channelIndex],
               NULL,  /* no SBR...*/
               0,
               sbrChannelIndex,
               element,
#ifdef SBR_SCALABLE
               maxSfbFreqLine,
#endif
               bDownSampledSbr
#ifdef PARAMETRICSTEREO
               ,sbrEnablePS,
               sbrEnablePS?timeDataOut[channelIndex + 1]:NULL,
               NULL
#endif
               ,0
               /* 20060107 */
               ,core_bandwidth
               ,bUsedBSAC
#ifdef SONY_PVC_DEC
               ,NULL
               ,USE_ORIG_SBR
               ,0
#endif /* SONY_PVC_DEC */
               );

      sbrChannelIndex++;
      processedChannels[channelIndex] = 1;
    }
  }

  return err ;
}


SBR_ERROR
usac_applySBR (SBRDECODER self,
               SBRBITSTREAM * stream,
               float** timeData,
               float** timeDataOut,
               float ***qmfBufferReal, /* qmfBufferReal[2][MAX_TIME_SLOTS][64] */
               float ***qmfBufferImag, /* qmfBufferImag[2][MAX_TIME_SLOTS][64] */
#ifdef SBR_SCALABLE
               int maxSfbFreqLine,
#endif
               int bDownSampledSbr,
#ifdef PARAMETRICSTEREO
               int sbrEnablePS,
#endif
               int stereoConfigIndex,
               int numChannels, int el,
               int channelOffset
               /* 20060107 */
               ,int  core_bandwidth
               ,int bUsedBSAC
               ,int *pnBitsRead
#ifdef SONY_PVC_DEC
               ,int core_mode
               ,int *sbr_mode2
#endif /* SONY_PVC_DEC */
               ,int const bUsacIndependencyFlag
               )
{
  SBR_ERROR err = SBRDEC_OK ;

  int element = el;
  int eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex = 0;
  int processedChannels[MAX_TIME_CHANNELS];
  int i;

#ifdef SONY_PVC_DEC
  int sbr_mode = *sbr_mode2;
  int varlen;
#endif /* SONY_PVC_DEC */


  memset(&processedChannels,0,MAX_TIME_CHANNELS*sizeof(int));

  /* We have already done a check (outside this function) to make sure that SBR data exists.
     This is since this implementation of the implicit signalling does not assume that SBR
     exists, but rather checks for its existence, and adjusts the output sample rate if
     SBR data is found.

     However, if we had assumed that SBR data did exist, we would have to check now if it does
     i.e. stream->NrElements != 0, and if it doesn't we would have to use the SBR-Tool for upsampling,
     since we've already set up the output device at twice the sampling rate.
     */
  if (stream->NrElements && (stream->sbrElement[0].Payload > 0))
  {
      err = usac_readSBR (self, stream, numChannels, element, pnBitsRead
#ifdef SONY_PVC_DEC
                          ,core_mode
                          ,&sbr_mode
                          ,&varlen
#endif /* SONY_PVC_DEC */
                          ,bUsacIndependencyFlag
		  ) ;
  }

  sbrChannelIndex = 0;
    if (self->SbrChannel[el][sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      eleChannels = (stream->sbrElement [0].ElementID == SBR_ID_CPE) ? 2 : 1 ;


#ifdef SONY_PVC_DEC
      if(sbr_mode == USE_PVC_SBR){
        for(i=0; i<eleChannels; i++){
          decodeSbrData_for_pvc(
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.domain_vec2,
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.frameInfo,
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.nNfb,
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.nNoiseFactors,
                                self->SbrChannel[el][sbrChannelIndex + i].frameData.coupling,
                                sbrChannelIndex + i,
                                el);
        }
      }else if(sbr_mode == USE_ORIG_SBR){
        for(i=0; i<eleChannels; i++){
          decodeSbrData(self->SbrChannel[el][sbrChannelIndex + i].frameData.iEnvelope,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.domain_vec1,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.domain_vec2,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.nSfb,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.frameInfo,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.nNfb,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.ampRes,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.nScaleFactors,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.nNoiseFactors,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.offset,
                        self->SbrChannel[el][sbrChannelIndex + i].frameData.coupling,
                        sbrChannelIndex + i,
                        el);
        }
        
        if(eleChannels == 2 && self->SbrChannel[el][sbrChannelIndex].frameData.coupling){
          sbr_envelope_unmapping (self->SbrChannel[el][sbrChannelIndex].frameData.iEnvelope,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.iEnvelope,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.sbrNoiseFloorLevel,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.sbrNoiseFloorLevel,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.nScaleFactors,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.nNoiseFactors,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.ampRes);
        }
      }
      
#else /* SONY_PVC_DEC */
      
      
      for(i=0; i<eleChannels; i++){
        decodeSbrData(self->SbrChannel[el][sbrChannelIndex + i].frameData.iEnvelope,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.domain_vec1,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.domain_vec2,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.nSfb,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.frameInfo,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.nNfb,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.ampRes,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.nScaleFactors,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.nNoiseFactors,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.offset,
                      self->SbrChannel[el][sbrChannelIndex + i].frameData.coupling,
                      sbrChannelIndex + i);
      }

      if(eleChannels == 2 && self->SbrChannel[el][sbrChannelIndex].frameData.coupling){
          sbr_envelope_unmapping (self->SbrChannel[el][sbrChannelIndex].frameData.iEnvelope,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.iEnvelope,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.sbrNoiseFloorLevel,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.sbrNoiseFloorLevel,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.nScaleFactors,
                                  self->SbrChannel[el][sbrChannelIndex].frameData.nNoiseFactors,
                                  self->SbrChannel[el][sbrChannelIndex+1].frameData.ampRes);

      }

#endif /* SONY_PVC_DEC */


#ifdef PARAMETRICSTEREO
      if(sbrEnablePS /*eleChannels == 1 && self->SbrChannel[el][sbrChannelIndex].frameData.enablePS*/)
        tfDecodePs(self->hParametricStereoDec);
#endif

#ifdef SBR_SCALABLE
      eleChannels = numChannels;
#endif


      for(i=0; i<eleChannels; i++){
        sbr_dec (timeData[channelOffset + i], /* was channelIndex */
                 timeDataOut[channelOffset + i], /* was channelIndex */
                 &(self->SbrChannel[el][sbrChannelIndex + i].frameData),
                 (self->SbrChannel[el][sbrChannelIndex + i].syncState==SBR_ACTIVE),
                 sbrChannelIndex + i,
                 el, /* was: element */
#ifdef SBR_SCALABLE
                 maxSfbFreqLine,
#endif
                 bDownSampledSbr
#ifdef PARAMETRICSTEREO
                 ,sbrEnablePS,
                 sbrEnablePS ? timeDataOut[channelIndex + i + 1] : NULL,
                 self->hParametricStereoDec
#endif
                ,stereoConfigIndex
                 /* 20060107 */
                 ,core_bandwidth
                 ,bUsedBSAC
#ifdef SONY_PVC_DEC
				,self->pvcData
				,sbr_mode
				,varlen
#endif /* SONY_PVC_DEC */
                 ) ;


        processedChannels[channelIndex + i] = 1;
      }

      sbrChannelIndex += eleChannels;
    } /* if SBR_ACTIVE */

  /*
  Do the LFE channel, if available, or channels were syncState != ACTIVE for which we do up-sampling only.
  */
  for (channelIndex = 0 ; channelIndex < numChannels ; channelIndex++)
  {
    if(!processedChannels[channelIndex]){
      sbr_dec (
#ifndef UNI_STE_MONO_SBR_NOFIX
               timeData[channelOffset+channelIndex],
               timeDataOut[channelOffset+channelIndex],
#else
               timeData[channelOffset],
               timeDataOut[channelOffset],
#endif
               NULL,  /* no SBR...*/
               0,
               sbrChannelIndex,
               el /* was element */,
#ifdef SBR_SCALABLE
               maxSfbFreqLine,
#endif
               bDownSampledSbr
#ifdef PARAMETRICSTEREO
               ,sbrEnablePS,
               sbrEnablePS?timeDataOut[channelIndex + 1]:NULL,
               NULL
#endif
              ,stereoConfigIndex
               /* 20060107 */
               ,core_bandwidth
               ,bUsedBSAC
#ifdef SONY_PVC_DEC
               ,NULL
               ,USE_ORIG_SBR
               ,0
#endif /* SONY_PVC_DEC */
               );

      sbrChannelIndex++;
      processedChannels[channelIndex] = 1;
    }
  }


  sbrDecGetQmfSamples(numChannels, el,
                      qmfBufferReal,
                      qmfBufferImag);


#ifdef SONY_PVC_DEC
  *sbr_mode2 = sbr_mode;
#endif /* SONY_PVC_DEC */


  return err ;
}


void initUsacDfltHeader(SBRDECODER self, 
                        SBRDEC_USAC_SBR_HEADER *pUsacDfltHeader,
                        int elIdx){

  if(self && pUsacDfltHeader){
    SBR_HEADER_DATA *pSbrDfltHeader = &self->SbrDfltHeader[elIdx];

    pSbrDfltHeader->startFreq     = pUsacDfltHeader->start_freq;
    pSbrDfltHeader->stopFreq      = pUsacDfltHeader->stop_freq;

    if(pUsacDfltHeader->header_extra1){
      pSbrDfltHeader->freqScale   = pUsacDfltHeader->freq_scale;
      pSbrDfltHeader->alterScale  = pUsacDfltHeader->alter_scale;
      pSbrDfltHeader->noise_bands = pUsacDfltHeader->noise_bands; 
    } else {
      pSbrDfltHeader->freqScale   = SBR_FREQ_SCALE_DEFAULT;
      pSbrDfltHeader->alterScale  = SBR_ALTER_SCALE_DEFAULT;
      pSbrDfltHeader->noise_bands = SBR_NOISE_BANDS_DEFAULT; 
    }
    
    if(pUsacDfltHeader->header_extra2){        
      pSbrDfltHeader->limiterBands    = pUsacDfltHeader->limiter_bands;
      pSbrDfltHeader->limiterGains    = pUsacDfltHeader->limiter_gains;
      pSbrDfltHeader->interpolFreq    = pUsacDfltHeader->interpol_freq;
      pSbrDfltHeader->smoothingLength = pUsacDfltHeader->smoothing_mode;
    } else {
      pSbrDfltHeader->limiterBands    = SBR_LIMITER_BANDS_DEFAULT;
      pSbrDfltHeader->limiterGains    = SBR_LIMITER_GAINS_DEFAULT;
      pSbrDfltHeader->interpolFreq    = SBR_INTERPOL_FREQ_DEFAULT;
      pSbrDfltHeader->smoothingLength = SBR_SMOOTHING_LENGTH_DEFAULT;
    }

  }

  return;
}

