/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Naoki Iwakami     (NTT) 

and edited by
Naoki Iwakami     (NTT) 
Takehiro Moriya   (NTT)
Markus Werner     (SEED / Software Development Karlsruhe) 
Olaf Kaehler      (Fraunhofer IIS)

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
$Id: tvqAUDecode.c,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $
*******************************************************************************/


#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "dec_tf.h"

/* NTT VQ */ 
#include "ntt_conf.h"
#include "ntt_scale_conf.h"

/* Long term predictor */
#include "nok_lt_prediction.h"

#include "aac.h"
#include "allVariables.h"
#include "reorderspec.h"
#include "resilience.h"
#include "concealment.h"
#include "buffers.h"

#include "port.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"

#include "tf_main.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/*****************************************************************************************
 ***
 *** Function: tvqInitDecoder
 ***
 *** Purpose:  Initialize the TwinVQ decoder
 ***
 ****************************************************************************************/

void tvqInitDecoder (char *decPara, int layer,
                     FRAME_DATA *frameData,
                     TF_DATA *tfData)
                    
{
  int iscl,mat_total_bitrate;
  int tvqAacMode=0;
  int baseDecType, enhDecType;    

  float t_bit_rate_scl,tomo_ftmp;
  
  int ntt_NSclLay;
  int ntt_NSclLayDec;

  int ntt_IBPS = 0;
  int ntt_IBPS_SCL[8];
  
  AUDIO_SPECIFIC_CONFIG *layerConfig = &frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;
  
  int numChannels = layerConfig->channelConfiguration.value ;
  
  float ntt_BPS, ntt_BPS_SCL[8];
  char *p_ctmp;
  
  ntt_INDEX ntt_index;
  ntt_INDEX ntt_index_scl;

  int addRdFlag=0, TrueTracks=0;
  int iexit;

    
  baseDecType = frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  enhDecType = frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  
  if ( baseDecType == enhDecType ) 
  {
      tvqAacMode = 0; /* TVQ only */
  }
  else 
  {
      tvqAacMode = 1; /* TVQ base in AAC Scalable */
  }

  /* added by NTT 001206 */

  tfData->tvqDecoder = (TVQ_DECODER *)calloc(1,sizeof(TVQ_DECODER));

  iexit =0;
  ntt_NSclLay = -1;
  for (iscl=0; ((iscl<(int)frameData->od->streamCount.value) && (iexit ==0)); iscl++){ 
    if (frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==TWIN_VQ)
      ntt_NSclLay ++;
    else if(frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ){
      if(frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.epConfig.value&1)
        if(addRdFlag==0)
          addRdFlag=1;
        else{
          addRdFlag=0;
          ntt_NSclLay ++;
        }
      else
        ntt_NSclLay ++;

      if((ntt_NSclLay == (int)frameData->scalOutSelect) && (addRdFlag==0))
        TrueTracks = iscl+1;
    }
    else 
      iexit = 1000;
  }
  if(TrueTracks){
    frameData->od->streamCount.value = TrueTracks;
  }

  if(!tvqAacMode) {
    frameData->scalOutSelect = min((int)frameData->scalOutSelect, ntt_NSclLay);
    ntt_NSclLay = frameData->scalOutSelect;
  }


  /* kaehleof 20040303: calculate bitrate from length of first frame */
  {
    AUDIO_SPECIFIC_CONFIG *asc = &frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
    int iesc;
    float brate;
    float srate = (float)frameData->layer[layer].sampleRate;
    int samplesPerFrame = (asc->specConf.TFSpecificConfig.frameLength.value==1)?960:1024;
    int nESCs = (asc->epConfig.value&1)?2:1;

    for (iesc=0, brate=.0; iesc<nESCs; iesc++) {
      brate += frameData->layer[iesc].AULength[0]-frameData->layer[iesc].AUPaddingBits[0];
    }
    brate = brate*srate / samplesPerFrame;

    ntt_IBPS = (int)brate;
    ntt_BPS = brate;
    if ((unsigned)ntt_IBPS != frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value) {
      DebugPrintf(1,"TVQ: base layer bitrate: calculated %i, read from descriptor %i\n",ntt_IBPS, frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value);
    }

    for (iscl=0; iscl<ntt_NSclLay; iscl++) {
      for (iesc=0, brate=.0; iesc<nESCs; iesc++) brate += frameData->layer[(iscl+1)*nESCs+iesc].AULength[0]-frameData->layer[(iscl+1)*nESCs+iesc].AUPaddingBits[0];
      brate = brate*srate / samplesPerFrame;
      ntt_IBPS_SCL[iscl]= (int)brate;
      ntt_BPS_SCL[iscl] = brate;
      if ((unsigned)ntt_IBPS != frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value) {
        DebugPrintf(1,"TVQ: ext%i layer bitrate: calculated %i, read from descriptor %i\n",iscl,ntt_IBPS_SCL[iscl], frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value);
      }
    }
  }
  
  for(mat_total_bitrate=0,iscl=0;iscl<ntt_NSclLay;iscl++)
  {
    mat_total_bitrate += ntt_IBPS_SCL[iscl];
  }
  mat_total_bitrate += ntt_IBPS;
  
  /* bitrate control */
  t_bit_rate_scl = 0.;
  for (iscl=0; iscl<ntt_NSclLay; iscl++){
    t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
  }
  
  tomo_ftmp=(float)ntt_IBPS+t_bit_rate_scl;
  
  /* initalization of base decoder */
  ntt_base_init((float) frameData->layer[layer].sampleRate, (float)tomo_ftmp, t_bit_rate_scl, 
    numChannels, tfData->block_size_samples, &ntt_index);
    /*TM9902 */
  
  /* initialization of scalable coders */

  /* set number of scalable layers to be decoded */
  ntt_NSclLayDec = ntt_NSclLay;
  if( ( p_ctmp=strstr( decPara, "-nttDecLyr " ) ) ) {
    if(sscanf(p_ctmp+11, "%d", &ntt_NSclLayDec) == 0){
      CommonExit(1, "Encode: parameter of -nttDecLyr switch not found");
    }
  }
  
  if (ntt_NSclLayDec > ntt_NSclLay) 
    ntt_NSclLayDec = ntt_NSclLay;
  
  
  ntt_scale_init ( ntt_NSclLay, 
                   ntt_IBPS_SCL, 
                  (float) frameData->layer[layer].sampleRate, 
                  &ntt_index, 
                  &ntt_index_scl);

  
  if( strstr( decPara, "-nttSclMsg" ) ) {
    ntt_scale_message(iscl, ntt_IBPS, ntt_IBPS_SCL);
  }

  ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
  ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay; 

  tfData->tvqDecoder->nttDataBase = ntt_index.nttDataBase;
  tfData->tvqDecoder->nttDataScl = ntt_index_scl.nttDataScl;
  
  /* T.Ishikawa 980623 */
  t_bit_rate_scl=0;
  for(iscl=0;iscl<ntt_NSclLay;iscl++)
  {
    /* T.Ishikawa 980703 */
    t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl]*numChannels;
  }
  
  DebugPrintf (3, "BAND %f BPS %d\n",ntt_index.nttDataBase->bandUpper, ntt_IBPS);
 
  for(iscl=0;iscl<4;iscl++)
    DebugPrintf (3, "AC_TOP[%d] %f , AC_BTM[%d] %f\n",iscl,
    ntt_index_scl.nttDataScl->ac_top[0][0][iscl],iscl,
    ntt_index_scl.nttDataScl->ac_btm[0][0][iscl]);
 
  /* TwinVQ uses sfb tables from AAC decoder */

  {
    int i ;

    for (i=0; i<(1<<LEN_SAMP_IDX); i++) {
      if (samp_rate_info[i].samp_rate == frameData->layer[layer].sampleRate)
        break;
    }

    infoinit (&samp_rate_info[i], tfData->block_size_samples);

    winmap[0] = win_seq_info[ONLY_LONG_SEQUENCE];
    winmap[1] = win_seq_info[ONLY_LONG_SEQUENCE];
    winmap[2] = win_seq_info[EIGHT_SHORT_SEQUENCE];
    winmap[3] = win_seq_info[ONLY_LONG_SEQUENCE];

    tfData->sfbInfo = winmap;
  }

}  

/*****************************************************************************************
 ***
 *** Function: tvqExitDecoder
 ***
 *** Purpose:  free the TwinVQ decoder allocated resources
 ***
 ****************************************************************************************/

void tvqFreeDecoder (HANDLE_TVQDECODER nttDecoder)
{
  ntt_base_free(nttDecoder->nttDataBase);
  ntt_scale_free(nttDecoder->nttDataScl);
  free(nttDecoder);

}



static void mergeTvqAccessUnits(FRAME_DATA* fd, int esc0, int esc1)
{
  HANDLE_BSBITSTREAM layerStream_tmp = BsOpenBufferRead(fd->layer[esc1].bitBuf);
  int tmp_bits0, tmp_bits1;

  tmp_bits0 = fd->layer[esc0].AULength[0]-fd->layer[esc0].AUPaddingBits[0];
  BsBufferManipulateSetNumBit(tmp_bits0, fd->layer[esc0].bitBuf);

  tmp_bits1 = fd->layer[esc1].AULength[0]-fd->layer[esc1].AUPaddingBits[0];
  BsGetBufferAppend(layerStream_tmp, fd->layer[esc0].bitBuf, 1, tmp_bits1);
  removeAU(layerStream_tmp, tmp_bits1, fd, esc1);

  tmp_bits0 += tmp_bits1;
  fd->layer[esc0].AULength[0] = ((tmp_bits0+7)/8)*8;
  fd->layer[esc0].AUPaddingBits[0] = fd->layer[esc0].AULength[0] - tmp_bits0;
  BsBufferManipulateSetNumBit(tmp_bits0, fd->layer[esc0].bitBuf);

  BsCloseRemove(layerStream_tmp, 1);
}




static void tvqCoreDec ( int                   numChannels,
                         FRAME_DATA*           fd, /* config data , obj descr. etc. */
                         TF_DATA*              tfData,
                         HANDLE_DECODER_GENERAL hFault,
                         ntt_INDEX*            index,
                         HANDLE_BSBITSTREAM    mat_layerStream,
                         TNS_frame_info        tns_info[MAX_TIME_CHANNELS],
                         QC_MOD_SELECT         qc_select,
                         int                   *nextLayer)

     /*--- base decoder ---*/
{
  Float current_frame[ntt_T_FR_MAX];
 
  int tomo_tmp, itmp, i, j, i_ch, decoded_bits;
  int ntt_available_bits;
  int samplFreqIdx = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
        samplingFreqencyIndex.value;

  /* bit unpacking */
  decoded_bits = tfData->decoded_bits;
  index->ms_mask = 0;
  index->group_code = 0x7F ; /*default */
  index->last_max_sfb[0] = 0;

  DebugPrintf (3, "____ windowSequence %d \n",tfData->windowSequence[MONO_CHAN]);
  DebugPrintf (3, "____ decoded_bit %d \n",decoded_bits);
  DebugPrintf (3, "tfData->pred_type %5d \n", tfData->pred_type); 

  ntt_headerdec(-1, mat_layerStream,  index, tfData->sfbInfo, 
                &decoded_bits, tns_info,
                tfData->nok_lt_status, 
                tfData->pred_type,
                hFault );
 
  tomo_tmp=decoded_bits;
  ntt_available_bits = index->nttDataBase->ntt_NBITS_FR - decoded_bits; /* T.Ishikawa980624 */

   DebugPrintf (3, "____ ntt_available_bits %d %d %d\n",ntt_available_bits, index->nttDataBase->ntt_NBITS_FR,decoded_bits);
   DebugPrintf (3, "____ ntt_available_bits %ld %d \n",BsBufferNumBit(fd->layer[0].bitBuf), decoded_bits);
  
 
   decoded_bits += ntt_BitUnPack ( mat_layerStream,
                                  BsBufferNumBit(fd->layer[0].bitBuf)
                                  - tomo_tmp,
								                  tomo_tmp,
                                  fd,
                                  tfData->windowSequence[MONO_CHAN],
                                  index);
  
  tfData->decoded_bits = decoded_bits;

  DebugPrintf (3, "<<<< ntt_BitUnPack %d %d >>>>\n", decoded_bits-tomo_tmp,decoded_bits);

  /* decoding tools*/
  ntt_vq_decoder(index,tfData->spectral_line_vector[0],
                 tfData->sfbInfo);

  DebugPrintf (3, "<<<< Base Layer Decoding Finished!!! >>>>\n");
	    
  /* nok 990113 ... */

  /*-- long term predictor --*/
  if(tfData->pred_type == NOK_LTP)
    {
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];

      for(i_ch = 0; i_ch < numChannels; i_ch++){
          double time_signal[BLOCK_LEN_LONG], tmpBuf[BLOCK_LEN_LONG];
        
          for (itmp = 0; itmp < info->bins_per_bk; itmp++)
            current_frame[itmp] = tfData->spectral_line_vector[0][i_ch][itmp];

            nok_lt_predict(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
                           tfData->windowSequence[MONO_CHAN],
                           tfData->windowShape[i_ch],
                           tfData->prev_windowShape[i_ch],
                           tfData->nok_lt_status[i_ch]->sbk_prediction_used,
                           tfData->nok_lt_status[i_ch]->sfb_prediction_used,
                           tfData->nok_lt_status[i_ch],
                           tfData->nok_lt_status[i_ch]->weight,
                           tfData->nok_lt_status[i_ch]->delay,
                           current_frame,
                           info->bins_per_bk,
                           info->bins_per_bk/8, 
                           samplFreqIdx,
                           /*(tvqTnsPresent) ?*/ &(tns_info[i_ch]) /*: NULL */,
                           qc_select);


          /* This is passed to upper layers. */
          for (i = 0; i < info->bins_per_bk; i++)
            tfData->spectral_line_vector[0][i_ch][i] = current_frame[i];

          {	
            /* TNS synthesis filtering. */  
            for(i = j = 0; i <  tns_info[i_ch].n_subblocks; i++)
              {
                tns_decode_subblock(current_frame + j,
                                    index->max_sfb[0],
                                    info->sbk_sfb_top[i],
                                    info->islong,
                                    &(tns_info[i_ch].info[i]), 
                                    qc_select,samplFreqIdx);
		  
                j += info->bins_per_sbk[i];
              }
          }	

          for (i = 0; i < info->bins_per_bk; i++)
	          tmpBuf[i] = current_frame[i];
	  
	        freq2buffer(tmpBuf, time_signal, 
		                  tfData->nok_lt_status[i_ch]->overlap_buffer,
		                  tfData->windowSequence[MONO_CHAN], 
		                  info->bins_per_bk,
		                  info->bins_per_bk/8, tfData->windowShape[MONO_CHAN], 
		                  tfData->prev_windowShape[i_ch], OVERLAPPED_MODE, 8);
	              
	        nok_lt_update(tfData->nok_lt_status[i_ch], time_signal, 
			                  tfData->nok_lt_status[i_ch]->overlap_buffer, 
			                  info->bins_per_bk);
        }
         
    }

  /* kaehleof: note the bits are removed from fd somewhere in tvqAUDecode() ! */

  if((index->er_tvq==1) && (fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value&1)){
    (*nextLayer)++;
  }  
  
  (*nextLayer)++;
}



static void tvqScalDec(
                       FRAME_DATA*  fd, /* config data , obj descr. etc. */
                       TF_DATA*     tfData,
                       HANDLE_DECODER_GENERAL hFault,
                       ntt_INDEX*   index,
                       ntt_INDEX*   index_scl,
                       
                       TNS_frame_info tns_info[MAX_TIME_CHANNELS],
                       int *nextLayer)
     /*--- scalable decoders ---*/
{
  int mat_shift[8][ntt_N_SUP_MAX];
  HANDLE_BSBITSTREAM  mat_layerStream ;
  /*
    TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  */
  int decoded_bits, iscl,tomo_tmp,tomo_tmp2;
  int   ntt_available_bits;
  int  iii, jjj, ntt_NSclLay, ntt_NSclLayDec;

  /*ntt_NSclLay = fd->od->streamCount.value-1; */
  ntt_NSclLay = index_scl->nttDataScl->ntt_NSclLay;

  if (( fd->od->streamCount.value > ( unsigned int )*nextLayer ) 
      && (((fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==TWIN_VQ))
        ||
         ((fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ))
)) {
    ntt_NSclLayDec = tfData->output_select;
  }  else {
    ntt_NSclLayDec=0;
  }
  if(ntt_NSclLayDec > ntt_NSclLay) ntt_NSclLayDec = ntt_NSclLay;

  DebugPrintf (5, "WWWWWW %5d %5d NSclLay\n", ntt_NSclLay, ntt_NSclLayDec);

  for(iii=0; iii<8; iii++){
    for(jjj=0; jjj<index->max_sfb[0]; jjj++){
      index_scl->msMask[iii][jjj] = 
        index->msMask[iii][jjj];
    }
  }
  index_scl->w_type = tfData->windowSequence[MONO_CHAN];
  index_scl->max_sfb[0] = index->max_sfb[0];
  index_scl->group_code = index->group_code ;
  index_scl->pf = index->pf ;
  for (iscl=0; iscl<ntt_NSclLayDec; iscl++){
    index_scl->last_max_sfb[iscl+1] = 
      index_scl->max_sfb[iscl];
    /* added by NTT 001205 */
    index_scl->er_tvq=0;
    if(fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ)
      index_scl->er_tvq=1;
    if((index_scl->er_tvq==1) && (fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.epConfig.value&1)){
      mergeTvqAccessUnits(fd, *nextLayer, (*nextLayer)+1);
    }

    mat_layerStream = BsOpenBufferRead((fd->layer[*nextLayer].bitBuf));
    /* fixed by NTT 001206 */
    index_scl->ms_mask = 0;

    decoded_bits=0;
    ntt_headerdec(iscl, mat_layerStream,  
                  index_scl, tfData->sfbInfo, 
                  &decoded_bits, tns_info,
                  tfData->nok_lt_status, 
                  tfData->pred_type,
                  hFault );
    ntt_available_bits =  
      index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]-decoded_bits;

    DebugPrintf (3, "TTTTTTT available %5d %5d[%d]\n",
             ntt_available_bits,
             index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl],iscl);

    /* bit unpacking */
    tomo_tmp=decoded_bits;
    tomo_tmp2=BsCurrentBit(mat_layerStream);
    
    decoded_bits += ntt_SclBitUnPack ( mat_layerStream,
                                       index_scl,
				       mat_shift[iscl],
                                       tomo_tmp,
                                       fd,
                                       nextLayer,
                                       BsBufferNumBit(fd->layer[iscl+1].bitBuf)
                                       - tomo_tmp,
                                       iscl);
    
    tfData->decoded_bits += decoded_bits;

    DebugPrintf (3, "<<<< ntt_SclBitUnPack[%d] %d %d >>>>\n", iscl,decoded_bits-tomo_tmp,decoded_bits);
    DebugPrintf (3, "<<<< mat_Stream[%d] %ld >>>>\n", iscl,BsCurrentBit(mat_layerStream)-tomo_tmp2);

    /* decoding tools */
    if (iscl < tfData->output_select){
      /*NN
        mat_scale_set_shift_para2(iscl);
      */
      ntt_scale_vq_decoder(index_scl, 
                           index,
			   mat_shift[iscl],
                           tfData->spectral_line_vector[0], iscl, 
                           tfData->sfbInfo);

      DebugPrintf (3, "<<<< Scale Layer[%d] Decoding Finished!! ! >>>>\n",iscl);
    }
    
    /* kaehleof: note the bits of esc1 are removed from fd when merging
       the two escs... however we still have to remove esc0, or in case
       of epConfig != 1 we remove all bits at once */
    removeAU(mat_layerStream, decoded_bits, fd, *nextLayer);
    BsCloseRemove(mat_layerStream,1);

    if((index_scl->er_tvq==1) && (fd->od->ESDescriptor[iscl+1]->DecConfigDescr.audioSpecificConfig.epConfig.value&1)){
      (*nextLayer)++;
    }

    (*nextLayer)++;
  }
}

static void tvqHeaderDec (
                          int          numChannels,
                          TF_DATA*     tfData,
                          ntt_INDEX*   index,
                          HANDLE_BSBITSTREAM  mat_layerStream )
{
  int decoded_bits, codedBlockType;
  unsigned long  ultmp;

  decoded_bits = 0;
  BsGetBit( mat_layerStream, &ultmp,2); /* window_sequence */
  codedBlockType = (int)ultmp;
  decoded_bits += 2;
  switch (   codedBlockType )
    {
    case 0:
      tfData->windowSequence[MONO_CHAN] =  ONLY_LONG_SEQUENCE;
      break;
    case 1:
      tfData->windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE;
      break;
    case 2:
      tfData->windowSequence[MONO_CHAN] =  EIGHT_SHORT_SEQUENCE;
      break;
    case 3:
      tfData->windowSequence[MONO_CHAN] =  LONG_STOP_SEQUENCE;
      break;
    default:
      CommonExit(-1,"wrong blocktype %d",   codedBlockType);
    }
  BsGetBit( mat_layerStream, &ultmp,1);
  tfData->windowShape[0] = (WINDOW_SHAPE)ultmp; /* window shape */
  decoded_bits += 1;
  tfData->decoded_bits = decoded_bits;
 

  if (numChannels==2){
    tfData->windowSequence[1]=tfData->windowSequence[MONO_CHAN];
    tfData->windowShape[1] =   tfData->windowShape[MONO_CHAN];
  }

  index->w_type=tfData->windowSequence[MONO_CHAN];
}

void tvqAUDecode( 
                  FRAME_DATA*  fd, /* config data , obj descr. etc. */
                  TF_DATA*     tfData,
                  HANDLE_DECODER_GENERAL hFault,
                  QC_MOD_SELECT      qc_select,
                  ntt_INDEX*   index,
                  ntt_INDEX*   index_scl,
                  int *nextLayer)
     
{
  int tmpEpConfig = 0;
  int mat_usedNumBit;
  int samplFreqIdx = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
    samplingFreqencyIndex.value;
  HANDLE_BSBITSTREAM  mat_layerStream;
  int ntt_output_select;
  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  index->er_tvq=0;

  if( fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ)
   index->er_tvq=1;

  if(fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value==2) {
    tmpEpConfig = 2;
    fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value = 0;
  }

  if((index->er_tvq) && (fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value)){
    mergeTvqAccessUnits(fd, 0, 1);
  }
  mat_layerStream = BsOpenBufferRead( fd->layer[0].bitBuf);

  ntt_output_select = tfData->output_select;
  tvqHeaderDec( index->numChannel,
                tfData,
                index, mat_layerStream);
     
  mat_usedNumBit =BsCurrentBit(mat_layerStream);
  tvqCoreDec( index->numChannel,
              fd, /* config data , obj descr. etc. */
              tfData,
              hFault, index, mat_layerStream, tns_info,	qc_select,nextLayer);
  mat_usedNumBit =BsCurrentBit(mat_layerStream);

  /* kaehleof: note the bits of esc1 are removed from fd when merging
     the two escs... however we still have to remove esc0, or in case
     of epConfig != 1 we remove all bits at once */
  removeAU(mat_layerStream, mat_usedNumBit, fd, 0);
  BsCloseRemove(mat_layerStream,1);     
  
  tvqScalDec( fd,/* config data , obj descr. etc. */
              tfData,
              hFault, index, index_scl, tns_info,nextLayer);
  
  if(ntt_output_select <= index->nttDataScl->ntt_NSclLay)
  {
    int ch;
    for (ch=0;ch<index->numChannel;ch++){
      int i, k, j, max_sfb[8];
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      if(ntt_output_select==0) max_sfb[0] = index->max_sfb[0];
      else max_sfb[ntt_output_select] = index_scl->max_sfb[ntt_output_select];

      for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
        Float tmp_spec[1024];
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
        }
        tns_decode_subblock( tmp_spec,
                             max_sfb[ntt_output_select],
                             info->sbk_sfb_top[i],
                             info->islong,
                             &(tns_info[ch].info[i]) ,
                             qc_select,samplFreqIdx);

        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
        }

        j += info->bins_per_sbk[i];
      }
    }
  }
  tfData->decoded_bits = mat_usedNumBit;

  if( tmpEpConfig )
    fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value = tmpEpConfig;

}

