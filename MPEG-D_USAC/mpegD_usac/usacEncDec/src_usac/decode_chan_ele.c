/*******************************************************************************
This software module was originally developed by


AT&T, Dolby Laboratories, Fraunhofer IIS, VoiceAge Corp.

and edited by
Yoshiaki Oikawa     (Sony Corporation),
Mitsuyuki Hatanaka  (Sony Corporation),
Markus Werner       (SEED / Software Development Karlsruhe)


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
$Id: decode_chan_ele.c,v 1.28.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/

/**********************************************************************
 MPEG-4 Audio VM



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

 $Id: decode_chan_ele.c,v 1.28.4.1 2012-04-19 09:15:33 frd Exp $
 USAC tf Decoder module
**********************************************************************/

#include <stdio.h>
#include <string.h>

#ifdef SONY_PVC
#include "../src_tf/sony_pvcprepro.h"
#endif  /* SONY_PVC */

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "usac_mainStruct.h"     /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "buffers.h"
#include "common_m4a.h"
#include "concealment.h"
#include "huffdec2.h"
#include "usac_port.h"
#include "resilience.h"
#include "concealment.h"

#include "usac_allVariables.h"        /* variables */
#include "usac_interface.h"
#include "usac_tw_tools.h"

#include "acelp_plus.h"
#include "usac_main.h"

#include "proto_func.h"

#include "usac_cplx_pred.h"

#ifdef DEBUGPLOT
#include "plotmtv.h"
#endif

/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 *
 * This function corresponds to the bitstream element single_channel_element() and/or channel_pair_element().
 *
 */

int decodeChannelElement (  int                      id,
                            MC_Info*                 mip,
                            USAC_DATA               *usacData,
                            WINDOW_SEQUENCE*         win,
                            Wnd_Shape*               wshape,
                            byte**                   cb_map,
                            short**                  factors,
                            byte**                   group,
                            byte*                    hasmask,
                            byte**                   mask,
                            byte*                    max_sfb,
                            TNS_frame_info**         tns,
                            float**                 coef,
                            Info*                    sfbInfo,
                            QC_MOD_SELECT            qc_select,
                            HANDLE_DECODER_GENERAL   hFault,
                            int                      elementTag,
                            int                      bNoiseFilling,
                            int                     *channelOffset
#ifdef SONY_PVC_DEC
							,int *cm
#endif /* SONY_PVC_DEC */
#ifndef CPLX_PRED_NOFIX
                           ,int stereoConfigIndex                  
#endif /* CPLX_PRED_NOFIX */
                            )
{
  int i=0, k=0,i_ch=0, tag=0, ch=0, widx=0, first=0, last=0, chCnt=0;
  int max_spec_coefficients=-1;
  int core_mode[2] = {0, 0};
  int common_tw = 0;
  int common_window = 0;
  int wn = 0;
  int outCh=0;
  int tns_data_present[2] = {0, 0};
  int tns_active, common_tns, tns_on_lr=1, tns_present_both;
  unsigned char common_max_sfb, max_sfb1, max_sfb_ste;
  int pred_dir=0, complex_coef=0, use_prev_frame=0;
  Wnd_Shape tmpWshape;

  Ch_Info *cip ;
  HANDLE_USAC_DECODER hDec = usacData->usacDecoder;

  HANDLE_BUFFER            hVm              = hFault[0].hVm;
  HANDLE_BUFFER            hHcrSpecData     = hFault[0].hHcrSpecData;
  HANDLE_RESILIENCE        hResilience      = hFault[0].hResilience;
  HANDLE_HCR               hHcrInfo         = hFault[0].hHcrInfo;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData = hFault[0].hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment     = hFault[0].hConcealment;

  int bad_frame[4]={0, 0, 0, 0};
  float synth_buf[128+L_FRAME_1024];
  float *synth = NULL;
  td_frame_data td_frame;
  int   isBassPostFilter;
  int   isAceStart = 0;
  int   short_fac_flag = 0;
  int   nChannelsCoreCoder = (id == ID_CPE)?2:1;

  memset(&td_frame, 0, sizeof(td_frame));

  synth = synth_buf + 128;

  BsSetChannel(1);

  tag=elementTag;

  usacData->FrameWasTD[0] = usacData->FrameIsTD[0];
  usacData->FrameWasTD[1] = usacData->FrameIsTD[1];

  if((id == ID_SCE) || (id == ID_CPE)){
    for(i=0; i<nChannelsCoreCoder; i++){
      core_mode[i] = GetBits( LEN_CORE_MODE,
                              CORE_MODE,
                              hResilience,
                              hEscInstanceData,
                              hVm );
    }
  }

#ifdef SONY_PVC_DEC
  *cm = core_mode[0];
#endif /* SONY_PVC_DEC */

  if ((id == ID_SCE) && (core_mode[0] == CORE_MODE_FD)) {
    tns_data_present[0] = GetBits( LEN_TNS_ACTIVE,
                                   TNS_ACTIVE,
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );
  } 

  if( (id == ID_CPE) && (core_mode[0] == CORE_MODE_FD) && (core_mode[1] == CORE_MODE_FD) ){ /* parse StereoCoreToolInfo() */

    tns_active = GetBits( LEN_TNS_ACTIVE,
                          TNS_ACTIVE,
                          hResilience,
                          hEscInstanceData,
                          hVm );

    common_window = GetBits ( LEN_COM_WIN,
                              COMMON_WINDOW,
                              hResilience,
                              hEscInstanceData,
                              hVm );

    if(common_window){
      BsSetSynElemId(2);
      if ((ch = chn_config(id, tag, common_window, mip)) < 0) {
        CommonWarning("Number of channels negativ (decode_chan_ele.c)");
        return -1;
      }
      *channelOffset = ch;
      first = ch;
      last = mip->ch_info[ch].paired_ch;
      usacData->FrameWasTD[first] = usacData->FrameIsTD[first];
      usacData->FrameWasTD[last] = usacData->FrameIsTD[last];

      usacData->FrameIsTD[first] = usacData->coreMode[first] = (USAC_CORE_MODE) core_mode[0];
      usacData->FrameIsTD[last] = usacData->coreMode[last] = (USAC_CORE_MODE) core_mode[1];


      widx = mip->ch_info[ch].widx;

      usac_get_ics_info (&win[widx],
                         &wshape[widx].this_bk,
                         group[widx],
                         &max_sfb[widx],
                         qc_select,
                         hResilience,
                         hEscInstanceData,
                         hVm);

      *sfbInfo = *usac_winmap[win[widx]];
      calc_gsfb_table(sfbInfo, group[widx]);

      common_max_sfb = GetBits( LEN_COMMON_MAX_SFB,
                                COMMON_MAX_SFB,
                                hResilience,
                                hEscInstanceData,
                                hVm );

      if (common_max_sfb == 0) {
        if (win[widx] == EIGHT_SHORT_SEQUENCE) {
          max_sfb1 = GetBits( LEN_MAX_SFBS,
                              MAX_SFB,
                              hResilience,
                              hEscInstanceData,
                              hVm );
        }
        else {
          max_sfb1 = GetBits( LEN_MAX_SFBL,
                              MAX_SFB,
                              hResilience,
                              hEscInstanceData,
                              hVm );
        }
      }
      else {
        max_sfb1 = max_sfb[widx];
      }
      max_sfb_ste = max(max_sfb[widx], max_sfb1);

      win[widx] = usacMapWindowSequences(win[widx], usacData->windowSequenceLast[first]);

      win[mip->ch_info[last].widx] = win[widx];

      hasmask[widx] = getmask ( usac_winmap[win[widx]],
                                group[widx],
                                max_sfb_ste,
                                mask[widx],
                                hResilience,
                                hEscInstanceData,
                                hVm );

#ifndef CPLX_PRED_NOFIX
      if ((hasmask[widx] == 3) && (stereoConfigIndex == 0))                 
#else
      if (hasmask[widx] == 3) 
#endif /* CPLX_PRED_NOFIX */
      {
        usac_get_cplx_pred_data(usacData->alpha_q_re,
                                usacData->alpha_q_im,
                                usacData->alpha_q_re_prev,
                                usacData->alpha_q_im_prev,
                                usacData->cplx_pred_used,
                                &pred_dir,
                                &complex_coef,
                                &use_prev_frame,
                                sfbInfo->num_groups,
                                max_sfb_ste,
                                usacData->usacIndependencyFlag,
                                hResilience,
                                hEscInstanceData,
                                hVm);
      }
    } /* common_window */

    else{
      if ((ch = chn_config(id, tag, common_window, mip)) < 0) {
        CommonWarning("Number of channels negativ (decode_chan_ele.c)");
        return -1;
      }
      *channelOffset = ch;
      BsSetSynElemId(1);
      first = ch;
      last = mip->ch_info[ch].paired_ch;

      usacData->FrameWasTD[first] = usacData->FrameIsTD[first];
      usacData->FrameWasTD[last] = usacData->FrameIsTD[last];

      usacData->FrameIsTD[first] = usacData->coreMode[first] = (USAC_CORE_MODE) core_mode[0];
      usacData->FrameIsTD[last] = usacData->coreMode[last] = (USAC_CORE_MODE) core_mode[1];

      hasmask[mip->ch_info[first].widx] = 0;
      hasmask[mip->ch_info[last].widx] = 0;
    }
    if ( hDec->twMdct[0] == 1 ) {           
      common_tw = GetBits ( LEN_COM_TW,
                            COMMON_TIMEWARPING,
                            hResilience,
                            hEscInstanceData,
                            hVm );
      if ( common_tw == 1) {
        get_tw_data(&hDec->tw_data_present[first],
                    hDec->tw_ratio[first],
                    hResilience,
                    hEscInstanceData,
                    hVm );
        hDec->tw_data_present[last]=hDec->tw_data_present[first];
        for ( i = 0 ; i < NUM_TW_NODES ; i++ ) {
          hDec->tw_ratio[last][i] = hDec->tw_ratio[first][i];
        }
      }
    }

    if (tns_active) {
      if (common_window) {
        common_tns = GetBits( LEN_COMMON_TNS,
                              COMMON_TNS,
                              hResilience,
                              hEscInstanceData,
                              hVm );
      }
      else {
        common_tns = 0;
      }
      
      tns_on_lr = GetBits( LEN_TNS_ON_LR,
                           TNS_ON_LR,
                           hResilience,
                           hEscInstanceData,
                           hVm );

      if (common_tns) {
        usac_get_tns(sfbInfo, tns[first], hResilience, hEscInstanceData, hVm);
        *tns[last] = *tns[first];
        tns_data_present[0] = -1;	/* -1 signals that TNS data should */
        tns_data_present[1] = -1;	/* neither be read nor cleared */
      }
      else {
        tns_present_both = GetBits( LEN_TNS_PRESENT_BOTH,
                                    TNS_PRESENT_BOTH,
                                    hResilience,
                                    hEscInstanceData,
                                    hVm );

        if (tns_present_both) {
          tns_data_present[0] = 1;
          tns_data_present[1] = 1;
        }
        else {
          tns_data_present[1] = GetBits( LEN_TNS_DATA_PRESENT1,
                                         TNS_DATA_PRESENT,
                                         hResilience,
                                         hEscInstanceData,
                                         hVm );

          tns_data_present[0] = 1 - tns_data_present[1];
        }
      }
    }
    else {
      common_tns = 0;
      tns_data_present[0] = 0;
      tns_data_present[1] = 0;
    }
  } /* both channels fd */
  else{
    common_window = 0;
    common_tw = 0;

    if ((ch = chn_config(id, tag, common_window, mip)) < 0) {
      CommonWarning("Number of channels negativ (decode_chan_ele.c)");
      return -1;
    }
    *channelOffset = ch;

    switch(id){
    case ID_CPE:
      BsSetSynElemId(1);
      first = ch;
      last = mip->ch_info[ch].paired_ch;
      usacData->FrameWasTD[last] = usacData->FrameIsTD[last];
      usacData->FrameIsTD[last] = usacData->coreMode[last] = (USAC_CORE_MODE) core_mode[1];
      break;

    case ID_SCE:
    case ID_LFE:
      BsSetSynElemId(0);
      widx = mip->ch_info[ch].widx;
      hasmask[widx] = 0;
      first = ch;
      last = ch;
      break;

    default:
      CommonWarning("Invalid channel element (decode_chan_ele.c)");
      return -1;
      break;
    }
    usacData->FrameWasTD[first] = usacData->FrameIsTD[first];
    usacData->FrameIsTD[first] = usacData->coreMode[first] = (USAC_CORE_MODE) core_mode[0];

  }

  for (i = first; i <= last; i++) {

    if (usacData->coreMode[i] == CORE_MODE_FD) {
      BsSetChannel(i) ;
      widx = mip->ch_info[i].widx;

      fltclrs(coef[i], LN2);
      ConcealmentInitChannel(i, last-first+1, hConcealment);

      if (hDec->tddec[i] && usacData->FrameWasTD[i]) {
        decoder_LPD_end(hDec->tddec[i] ,hVm, usacData,i);
      }

      if ((id == ID_CPE) && (usacData->coreMode[first] != usacData->coreMode[last]) ){
        tns_data_present[i-first] = GetBits( LEN_TNS_ACTIVE,
                                             TNS_ACTIVE,
                                             hResilience,
                                             hEscInstanceData,
                                             hVm );
      } 

      {
        float tmpCoef[2048];
        short global_gain;

        if ( ! usac_get_fdcs ( hDec,
                               sfbInfo,
                               common_window,
                               common_tw,
                               tns_data_present[i-first],
                               &win[widx],
                               &wshape[widx].this_bk,
                               group[widx],
                               (i > first) ? &max_sfb1 : &max_sfb[widx],
                               cb_map[i],
                               tmpCoef,
                               max_spec_coefficients,
                               &global_gain,
                               factors[i],
                               &hDec->arithPreviousSize[i],
                               hDec->arithQ[i],
                               tns[i],
                               &hDec->tw_data_present[i],
                               hDec->tw_ratio[i],
                               hResilience,
                               hHcrSpecData,
                               hHcrInfo,
                               hEscInstanceData,
                               hConcealment,
                               qc_select,
                               &(hDec->nfSeed[i]),
                               hVm,
                               usacData->windowSequenceLast[i],
                               usacData->FrameWasTD[i],
                               usacData->usacIndependencyFlag,
                               i,
                               bNoiseFilling
                               ) )
          {
            CommonWarning("getics returns error (huffdec1.c)");
            return -1;
          }
        for(k=0;k<hDec->blockSize + hDec->blockSize/8; k++){
          hDec->coef[i][k] = tmpCoef[k];
        }
      }

#ifdef UNIFIED_RANDOMSIGN
      /* sync random noise seeds */
      hDec->tddec[i]->seed_tcx = hDec->nfSeed[i];
#endif

    } /* CORE_MODE_FD */

    else {
      BsSetChannel(i) ;

      usac_past_tw(  usacData->overlap_buffer[i],
                     usacData->block_size_samples,
                     usacData->block_size_samples/8,
                     usacData->windowShape[i],
                     usacData->prev_windowShape[i],
                     usacData->windowSequenceLast[i],
                     td_frame.mod[0],
                     usacData->FrameWasTD[i],
                     usacData->twMdct[0],                   
                     usacData->prev_sample_pos[i],
                     usacData->prev_tw_trans_len[i],
                     usacData->prev_tw_start_stop[i],
                     usacData->prev_warped_time_sample_vector[i]);


      if (!usacData->FrameWasTD[i]) {
        if ( usacData->twMdct[0] ) {            
          reset_decoder_lf(hDec->tddec[i],&usacData->overlap_buffer[i][512], (usacData->windowSequenceLast[i] == EIGHT_SHORT_SEQUENCE), 1); /*JEREMIE: -1 for lastWasShort, put correct value here*/
        }
        else {
          reset_decoder_lf(hDec->tddec[i],usacData->overlap_buffer[i], (usacData->windowSequenceLast[i] == EIGHT_SHORT_SEQUENCE), 0); /*JEREMIE: -1 for lastWasShort, put correct value here*/
        }
      }

      usac_get_tdcs( hDec->tddec[i],
                     &hDec->arithPreviousSize[i],
                     hDec->arithQ[i],
                     &td_frame,
                     hResilience,
                     hEscInstanceData,
                     hVm,
                     &isAceStart,
                     &short_fac_flag,
                     &isBassPostFilter,
                     usacData->usacIndependencyFlag
                     );

      decoder_LPD(hDec->tddec[i], 
                  &td_frame, 
                  hDec->fscale, 
                  synth, 
                  bad_frame, 
                  isAceStart,
                  short_fac_flag,
                  isBassPostFilter);

      widx = mip->ch_info[i].widx;
      usacData->windowShape[i] = wshape[widx].this_bk = WS_FHG;

      usac_td2buffer(synth,
                     usacData->time_sample_vector[i],
                     usacData->overlap_buffer[i],
                     usacData->block_size_samples,
                     usacData->block_size_samples/8,
                     usacData->windowSequenceLast[i],
                     td_frame.mod[0],
                     usacData->FrameWasTD[i],
                     usacData->twMdct[0],                       
                     usacData->prev_sample_pos[i],
                     usacData->prev_tw_trans_len[i],
                     usacData->prev_tw_start_stop[i],
                     usacData->prev_warped_time_sample_vector[i]);

      usacData->prev_windowShape[i] = usacData->windowShape[i];
      usacData->windowSequenceLast[i] = EIGHT_SHORT_SEQUENCE;

    }

#ifdef UNIFIED_RANDOMSIGN
      /* sync random noise seeds */
    hDec->nfSeed[i] = hDec->tddec[i]->seed_tcx;
#endif

  } /* end channel loop */

  tmpWshape = wshape[mip->ch_info[first].widx];

#ifndef USAC_REFSOFT_COR1_NOFIX_03
  /* downmix has to be calculated from last frame's ouput */
  /* it depends on this frame's prediction direction      */

  if(core_mode[0] == CORE_MODE_FD && core_mode[1] == CORE_MODE_FD && id == ID_CPE){
    int left = first;
    int right = mip->ch_info[first].paired_ch;

    usac_cplx_pred_prev_dmx(sfbInfo,
                            hDec->coefSave[left],
                            hDec->coefSave[right],
                            usacData->dmx_re_prev,
                            pred_dir);
  }
#endif /* USAC_REFSOFT_COR1_NOFIX_03 */

  for (k = 0; k < 2; k++) { /* order of TNS and stereo processing depends on tns_on_lr */
    if (k != tns_on_lr) {   /* if tns_on_lr apply stereo processing first */
      if(core_mode[0] == CORE_MODE_FD && core_mode[1] == CORE_MODE_FD && id == ID_CPE){
        /* stereo processing */
        ch = first;
        cip = &mip->ch_info[ch];
        if ((cip->present) && (cip->cpe) && (cip->ch_is_left)) {
          int left = ch;
          int right = cip->paired_ch;
          wn = cip->widx;
          if (hasmask[wn] != 3) {
            int sfb;

            for (sfb = 0; sfb < SFB_NUM_MAX; sfb++) {
              usacData->alpha_q_re_prev[sfb] = 0;
              usacData->alpha_q_im_prev[sfb] = 0;
            }
          }

#ifndef CPLX_PRED_NOFIX
          if ((hasmask[wn] == 3) && (stereoConfigIndex == 0))                 
#else
          if (hasmask[wn] == 3) 
#endif /* CPLX_PRED_NOFIX */
          {
            usac_decode_cplx_pred(sfbInfo,
                                  group[wn],
                                  hDec->coef[left],
                                  hDec->coef[right],
                                  usacData->dmx_re_prev,
                                  usacData->alpha_q_re,
                                  usacData->alpha_q_im,
                                  usacData->cplx_pred_used,
                                  pred_dir,
                                  complex_coef,
                                  use_prev_frame,
                                  win[wn],
                                  tmpWshape.this_bk,
                                  tmpWshape.prev_bk);
          }
          else 
          if (hasmask[wn]) {
            hDec->info = usac_winmap[hDec->wnd[wn]];
            map_mask(hDec->info, hDec->group[wn], hDec->mask[wn], hDec->cb_map[right], hasmask[wn]);
            usac_synt(hDec->info, hDec->group[wn], hDec->mask[wn], hDec->coef[right], hDec->coef[left]);
          }
        }
      }
    }
    else {  /* if not tns_on_lr apply TNS first */
		for(ch = first; ch <= last; ch++){

        if(usacData->coreMode[ch] == CORE_MODE_FD){
          BsSetChannel(ch);
          if (!(mip->ch_info[ch].present)) continue;
          wn = mip->ch_info[ch].widx;
          hDec->info = usac_winmap[hDec->wnd[wn]];
          hDec->wnd_shape[ch].prev_bk = hDec->wnd_shape[wn].this_bk;
          if (! ( hEscInstanceData && ( BsGetErrorForDataElementFlagEP ( N_FILT       , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( COEF_RES     , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( LENGTH       , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( ORDER        , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( DIRECTION    , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( COEF_COMPRESS, hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( COEF         , hEscInstanceData ) ||
                                        BsGetErrorForDataElementFlagEP ( GLOBAL_GAIN  , hEscInstanceData ) ) ) )
            {

              int i,j ;

              for (i=j=0; i<hDec->tns[ch]->n_subblocks; i++)
                {
                  if (debug['T']) {
                    if ( win[wn] == EIGHT_SHORT_SEQUENCE ) {
                      fprintf(stderr, "%ld %d %d\n", bno, ch, i);
                      print_tns( &(hDec->tns[ch]->info[i]));
                    }
                  }

                  if (hDec->tns[ch]->info[i].n_filt)

                  usac_tns_decode_subblock(&hDec->coef[ch][j],
                                           (ch > first) ? max_sfb1 : max_sfb[wn],
                                           hDec->info->sbk_sfb_top[i],
                                           hDec->info->islong,
                                           &(hDec->tns[ch]->info[i]),
                                           qc_select,
                                           mip->sampling_rate_idx);

                  j += hDec->info->bins_per_sbk[i];
                }
            }
          else
            {
              if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
                printf( "AacDecodeFrame: tns data disturbed\n" );
                printf( "AacDecodeFrame: --> tns disabled (channel %d)\n", ch );
              }
            }

        }
      } /* end channel loop */
    }
  }
  if(core_mode[0] == CORE_MODE_FD && core_mode[1] == CORE_MODE_FD && id == ID_CPE){
    int left = first;
    int right = cip->paired_ch;

#ifndef USAC_REFSOFT_COR1_NOFIX_03

    /* downmix has to be calculated in the next frame because */
    /* it depends on that frame's prediction direction        */

    usac_cplx_save_prev(sfbInfo,
                        hDec->coef[left],
                        hDec->coef[right],
                        hDec->coefSave[left],
                        hDec->coefSave[right]);

#else

    usac_cplx_pred_prev_dmx(sfbInfo,
                            hDec->coef[left],
                            hDec->coef[right],
                            usacData->dmx_re_prev,
                            pred_dir);

#endif /* USAC_REFSOFT_COR1_NOFIX_03 */
  }

  for (ch=first; ch<=last; ch++){
    if(usacData->coreMode[ch] == CORE_MODE_FD){
      for (i=0;i<(hDec->blockSize); i++){
        usacData->spectral_line_vector[ch][i] = hDec->coef[ch][i];
      }
    }
  }


  for (i_ch=first; i_ch<=last; i_ch++){
    if(usacData->coreMode[i_ch] == CORE_MODE_FD){
      widx = mip->ch_info[i_ch].widx;
      usacData->windowShape[i_ch] = wshape[widx].this_bk;
      usacData->windowSequence[i_ch] = win[widx];
    }
  }
  for (i_ch=first; i_ch<=last; i_ch++) {
    if (usacData->coreMode[i_ch] == CORE_MODE_FD) {
      float sample_pos[1024*3];
      float tw_trans_len[2];
      int   tw_start_stop[2];

      if ( usacData->twMdct[0] ) {            
        WINDOW_SEQUENCE winSeq = usacData->windowSequence[i_ch];
        WINDOW_SEQUENCE winSeqLast = usacData->windowSequenceLast[i_ch];

        if ( usacData->FrameWasTD[i_ch] ) {
          tw_reset(usacData->block_size_samples,
                   usacData->warp_cont_mem[i_ch],
                   usacData->warp_sum[i_ch]);
        }

        tw_calc_tw(hDec->tw_data_present[i_ch],
                   usacData->FrameWasTD[i_ch],
                   hDec->tw_ratio[i_ch],
                   winSeq,
                   usacData->warp_cont_mem[i_ch],
                   sample_pos,
                   tw_trans_len,
                   tw_start_stop,
                   usacData->warp_sum[i_ch],
                   usacData->block_size_samples
                   );


        tw_adjust_past(winSeq,
                       winSeqLast,
                       usacData->FrameWasTD[i_ch],
                       usacData->prev_tw_start_stop[i_ch],
                       usacData->prev_tw_trans_len[i_ch],
                       usacData->block_size_samples);

      }

      fd2buffer (usacData->spectral_line_vector[i_ch],
                 usacData->time_sample_vector[i_ch],
                 usacData->overlap_buffer[i_ch],
                 usacData->windowSequence[i_ch],
                 usacData->windowSequenceLast[i_ch],
                 usacData->block_size_samples,
                 usacData->block_size_samples/8,
                 usacData->windowShape[i_ch],
                 usacData->prev_windowShape[i_ch],
                 OVERLAPPED_MODE,
                 8,
                 usacData->FrameWasTD[i_ch],
                 usacData->twMdct[0],           
                 sample_pos,
                 tw_trans_len,
                 tw_start_stop,
                 usacData->prev_sample_pos[i_ch],
                 usacData->prev_tw_trans_len[i_ch],
                 usacData->prev_tw_start_stop[i_ch],
                 usacData->prev_warped_time_sample_vector[i_ch],
                 hDec->ApplyFAC[i_ch],
                 hDec->facData[i_ch],
                 usacData->lastLPC[i_ch],
                 usacData->acelpZIR[i_ch],
                 hDec->tddec[i_ch],
                 hResilience,
                 hEscInstanceData);

      usacData->prev_windowShape[i_ch] = usacData->windowShape[i_ch];
      usacData->windowSequenceLast[i_ch] =  usacData->windowSequence[i_ch];

#define REDUCE_SIGNAL_TO_16_
#ifdef REDUCE_SIGNAL_TO_16
      for(i = 0; i < usacData->block_size_samples; i++){
        /* Additional noise for demonstration */
#define AAA (0.5)
        usacData->time_sample_vector[i_ch][i]+=(AAA/RAND_MAX)*(float)rand()-AAA/2;
        usacData->overlap_buffer[i_ch][i]+=    (AAA/RAND_MAX)*(float)rand()-AAA/2;
      }
#endif


#ifdef DEBUGPLOT
      if (i_ch==0){
        plotSend("L", "timeOut",  MTV_DOUBLE,usacData->block_size_samples,  usacData->time_sample_vector[i_ch] , NULL);
      } else {
        plotSend("R", "timeOutR",  MTV_DOUBLE,usacData->block_size_samples,  usacData->time_sample_vector[i_ch] , NULL);
      }
#endif

    }
  }
  for (i_ch=first; i_ch<=last; i_ch++){
    for( i=0; i<(int)usacData->block_size_samples; i++ ){
      usacData->sampleBuf[i_ch][i] = (float) usacData->time_sample_vector[i_ch][i];
    }
    usacData->prev_coreMode[i_ch] = usacData->coreMode[i_ch];
    usacData->prev_coreMode[i_ch] = usacData->coreMode[i_ch];
  }

  return 0;
}


void usac_get_ics_info ( WINDOW_SEQUENCE*          win,
                         WINDOW_SHAPE*             wshape,
                         byte*                     group,
                         byte*                     max_sfb,
                         QC_MOD_SELECT             qc_select,
                         HANDLE_RESILIENCE         hResilience,
                         HANDLE_ESC_INSTANCE_DATA  hEscInstanceData,
                         HANDLE_BUFFER             hVm)

{
  Info *info;
  int tmp;

  tmp = (WINDOW_SEQUENCE)GetBits ( LEN_WIN_SEQ,
                                   WINDOW_SEQ,
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );
  if (bno&&debug['V']) {
    if ((((tmp==ONLY_LONG_SEQUENCE)||(tmp==LONG_START_SEQUENCE)) &&((*win==EIGHT_SHORT_SEQUENCE)||(*win==LONG_START_SEQUENCE)))||
        (((tmp==LONG_STOP_SEQUENCE)||(tmp==EIGHT_SHORT_SEQUENCE))&&((*win==ONLY_LONG_SEQUENCE)  ||(*win==LONG_STOP_SEQUENCE))))
      fprintf(stderr,"Non meaningful window sequence transition %i %i %i\n",tmp, *win, LONG_STOP_SEQUENCE);
  }
  *win = (WINDOW_SEQUENCE)tmp;

  tmp = GetBits ( LEN_WIN_SH,
                  WINDOW_SHAPE_CODE,
                  hResilience,
                  hEscInstanceData,
                  hVm );
  if (bno&debug['V']) {
    if (tmp-*wshape) fprintf(stderr,"# WndShapeSW detected\n");
  }
  *wshape = (WINDOW_SHAPE)tmp;

  if ( ( info = usac_winmap[*win] ) == NULL ) {
    CommonExit(1,"bad window code");
  }
  /*
   * max scale factor, scale factor grouping and prediction flags
   */
  if (info->islong) {
    *max_sfb = (unsigned char) GetBits ( LEN_MAX_SFBL,
                                         MAX_SFB,
                                         hResilience,
                                         hEscInstanceData,
                                         hVm );
    group[0] = 1;


  }
  else {  /* EIGHT_SHORT_SEQUENCE */
    *max_sfb = (unsigned char) GetBits (LEN_MAX_SFBS,
                                        MAX_SFB,
                                        hResilience,
                                        hEscInstanceData,
                                        hVm );
    getgroup ( info,
               group,
               hResilience,
               hEscInstanceData,
               hVm );
  }

  if ( *max_sfb > info->sfb_per_sbk[0] ) {
    if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "get_ics_info: max_sfb (%2d) > sfb_per_sbk (%2d) (decode_chan_ele.c)", *max_sfb, info->sfb_per_sbk[0]);
    }
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
        printf ( "get_ics_info: max_sfb (%2d) > sfb_per_sbk (%2d)\n", *max_sfb, info->sfb_per_sbk[0]);
        printf ( "get_ics_info: --> max_sfb is set to %2d\n", info->sfb_per_sbk[0]);
      }
      *max_sfb = info->sfb_per_sbk[0];
    }
  }

  if(debug['v']) {
    fprintf(stderr,"win %d, wsh %d\n", *win, *wshape);
    fprintf(stderr,"max_sf %d\n", *max_sfb);
  }
}
