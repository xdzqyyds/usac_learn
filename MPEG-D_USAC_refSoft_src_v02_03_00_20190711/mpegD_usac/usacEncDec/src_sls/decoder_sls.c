/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by ...

 $Id: decoder_sls.c,v 1.1.1.1 2009-05-29 14:10:14 mul Exp $
 SLS Decoder module
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include "allHandles.h"
#include "tf_mainHandle.h"
#include "block.h"
#include "buffer.h"
#include "coupling.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */

#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "aac.h"
#include "dec_tf.h"
#include "sls_int.h"
#include "lit_ll_dec.h"


#include "dolby_def.h"
#include "tf_main.h"
#include "common_m4a.h"
#include "nok_lt_prediction.h"
#include "port.h"
#include "buffers.h"
#include "bitstream.h"

#include "resilience.h"
#include "concealment.h"

#include "allVariables.h"
#include "obj_descr.h"

/* son_AACpp */
#include "sony_local.h"

#ifdef DRC
#include "drc.h"
#endif


/*
---     Global variables from aac-decoder      ---
*/

int debug [256] ;
static int* last_rstgrp_num = NULL;

/* -------------------------------------- */
/*       module-private functions         */
/* -------------------------------------- */

/* -------------------------------------- */
/*           DECODE 1 SLS frame           */
/* -------------------------------------- */
 
int SLSDecodeFrame ( HANDLE_AACDECODER        hDec,
                     TF_DATA*                 tfData,
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     /* Info**                   sfbInfo,  */
                     int                      msMask[8][60],
                     FRAME_DATA*              fd ,
                     HANDLE_DECODER_GENERAL   hFault,
                     int                      layer
                     )
{
  MC_Info *mip = hDec->mip ;
  int           ch ;
  int           i;
  int           outCh;
  unsigned char nrOfSignificantElements = 0; /* SCE (mono) and CPE (stereo) only */
  HANDLE_BSBITSTREAM  layer_stream = NULL;

  HANDLE_RESILIENCE         hResilience      = hFault[layer].hResilience;
  HANDLE_ESC_INSTANCE_DATA  hEscInstanceData = hFault[layer].hEscInstanceData;
  HANDLE_BUFFER             hVm              = hFault[layer].hVm;


  static const unsigned long samplingRate [] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
  };
  
  double pcm_scaling[4] = {256.0, 1.0, 1.0/16, 1.0/256, }; /* 8,16,20,24 bit PCM */   /* standard */
  
  short block_type_lossless_only = ONLY_LONG_SEQUENCE;
  int msStereoIntMDCT = 1;
  int coredisable = 0;


  int samplFreqIdx = mip->sampling_rate_idx;
  /*(fd==NULL) ? -1 : (int)fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value*/
  

  if(debug['n']) {
    /*     fprintf(stderr, "\rblock %ld", bno); */
    fprintf(stderr,"\n-------\nBlock: %ld\n", bno);
  }

  
  if (fd->layer[layer].NoAUInBuffer<=0) 
    CommonExit(-1,"AAZ layer error: NoAUInBuffer<=0"); ;
  
  if ( layer_stream == NULL ) {
    layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 
    setHuffdec2BitBuffer ( layer_stream );
  }	

  reset_mc_info(mip);

  /***** LIT_ll_read() *****/
  {
    for (ch=0;ch<numChannels;ch++)
      {
        tfData->ll_info[ch]->stream_len = GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm );
        tfData->ll_info[ch]->stream_len += GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm )<<8;
        for (i = 0; i<tfData->ll_info[ch]->stream_len;i++)
          tfData->ll_info[ch]->stream[i] = GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm );
      }
  }

  /**************************/
  
  removeAU( layer_stream,
            BsCurrentBit((HANDLE_BSBITSTREAM)layer_stream),
            fd,
            layer);
  
  BsCloseRemove ( layer_stream,1 );
  layer_stream = NULL;

  /* xxxxxxxxx */
  
  check_mc_info ( mip, 
                  0,
                  hEscInstanceData,
                  hResilience
                  );

  /* xxxxxxxxx */


  /********* LLE Info extracted *******/

  for (ch=0;ch<numChannels;ch++)
    {  
      LIT_ll_Dec(tfData->ll_info[ch],
                 tfData->ll_quantInfo[ch],
                 msMask[0],                  /* ms mask of layer 0 ? */
                 1,
                 tfData->ll_coef[ch],
                 &tfData->aacDecoder->mip->ch_info[ch+1],
                 samplingRate[tfData->aacDecoder->mip->sampling_rate_idx],
                 tfData->/* aacDecoder->mip-> */osf, 
                 tfData->aacDecoder->mip->type_PCM,
                 &block_type_lossless_only,
                 &msStereoIntMDCT,
                 !coredisable,
                 ch,
                 layer -1);   
    }

  for (ch=0; ch<numChannels; ch++) {
    Ch_Info *cip = &tfData->aacDecoder->mip->ch_info[ch];
    int wn, left, right;
    if (!(cip->present)) continue;
    wn = cip->widx;  
    if (cip->cpe && cip->common_window && msStereoIntMDCT) {
      /* StereoIntMDCT */
      if (cip->ch_is_left) {
        left = ch - 1;
        right = cip->paired_ch - 1;
        
        if (block_type_lossless_only != 0) {
          InvStereoIntMDCT(tfData->ll_coef[left],
                           tfData->ll_coef[right],
                           tfData->mdct_buf[left],
                           tfData->mdct_buf[right],
                           tfData->ll_timesig[left],
                           tfData->ll_timesig[right],
                           block_type_lossless_only,
                           tfData->prev_windowShape[wn], /* wnd_shape[wn].this_bk, */
                           tfData/* ->aacDecoder->mip */->osf
                           );
        } else {
          
          InvStereoIntMDCT((int*)tfData->ll_coef[left],
                           (int*)tfData->ll_coef[right],
                           (int*)tfData->mdct_buf[left],
                           (int*)tfData->mdct_buf[right],
                           (int*)tfData->ll_timesig[left],
                           (int*)tfData->ll_timesig[right],
                           (byte)tfData->windowShape[wn], /* wnd[wn], */
                           (byte)tfData->prev_windowShape[wn], /* wnd_shape[wn].this_bk, */
                           tfData->/* aacDecoder->mip-> */osf 
                           );
        }
      }
    }
  }
  
  for (ch=0; ch<numChannels; ch++) {
    int ii;
    
    if (!(&tfData->aacDecoder->mip->ch_info[ch+1].present)) continue;
    
    for (ii = 0;ii<tfData->/* aacDecoder->mip-> */osf*1024;ii++)
      tfData->time_sample_vector[ch][ii] = tfData->ll_timesig[ch][ii] * pcm_scaling[tfData->aacDecoder->mip->type_PCM];
  }
  






  /* Copy the coeff and pass them back to VM */
  
  outCh=0;
    
  for (ch=0; ch<Chans; ch++)
    {
      int i;
      
      if ((mip->ch_info[ch].present))
        {
          if ((outCh+1)> numChannels){
            CommonExit(-1,"wrong number of channels in command line");
          }
          for (i=0;i<hDec->blockSize;i++)
          {
              spectral_line_vector[outCh][i] = hDec->coef[ch][i];
          }
#ifdef I2R_LOSSLESS
          if (outCh!=ch)
            memcpy(hDec->sls_quant_channel_temp[outCh], hDec->sls_quant_channel_temp[ch],1024*sizeof(int));
#endif

          outCh++;
        }
    }

  /* not necessary, we (=sls) should be the highest layer (??) */
  
  bno++;
  
  return GetReadBitCnt ( hVm );
  
}



static void set_mip_preset(MC_Info *mip, int channels)
{
  int ch;

  mip->nch = channels;
  mip->profile = 1;
  /* mono */
  if (mip->nch == 1) {
    mip->ch_info[0].present = 1;
  }
  /* stereo */
  if (mip->nch == 2) {
    for (ch=1;ch<=2;ch++){
      mip->ch_info[ch].present = 1;
      mip->ch_info[ch].cpe = 1;
      mip->ch_info[ch].common_window = 1;
      mip->ch_info[ch].widx = 1;
      if (ch == 1){
        mip->ch_info[ch].ch_is_left = 1;
        mip->ch_info[ch].paired_ch = 2;
      }
      else {
        mip->ch_info[ch].ch_is_left = 0;
        mip->ch_info[ch].paired_ch = 1;
      }
    }
  }
}


void slsAUDecode ( int numChannels,
                   FRAME_DATA* frameData,
                   TF_DATA* tfData,
                   HANDLE_DECODER_GENERAL hFault,
                   enum AAC_BIT_STREAM_TYPE bitStreamType)
{
  int i_ch, decoded_bits = 0;
  short* pFactors[MAX_TIME_CHANNELS]; 
  int i;
    
  HANDLE_BSBITSTREAM fixed_stream;
  HANDLE_AACDECODER  hDec = tfData->aacDecoder; 

  int short_win_in_long = 8;

  QC_MOD_SELECT qc_select;

  switch (frameData->scalOutObjectType) {
  default:
    qc_select = AAC_QC;
    break;
  }


  for (i=0; i<MAX_TIME_CHANNELS; i++) {
    pFactors[i] = (short *)mal1(MAXBANDS*sizeof(*pFactors[0]));
  }
  
  for ( i_ch = 0 ; i_ch < numChannels ; i_ch++ ) {
    if ( ! hFault->hConcealment || ! ConcealmentAvailable ( 0, hFault->hConcealment ) ) {
      tfData->windowSequence[i_ch] = ONLY_LONG_SEQUENCE;
    }
  }

  fixed_stream = BsOpenBufferRead (frameData->layer[0].bitBuf);


  /* buffers for SSR  */

  /* run dummy aac core to fill spectrum */

  /* lossless only: fill channel structure */
  set_mip_preset(hDec->mip, numChannels);
  
  removeAU(fixed_stream,decoded_bits,frameData,0);
  BsCloseRemove(fixed_stream,1);

}


