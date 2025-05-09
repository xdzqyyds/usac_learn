/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by

 AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
  
 and edited by
 Yoshiaki Oikawa     (Sony Corporation),
 Mitsuyuki Hatanaka  (Sony Corporation),
 Markus Werner       (SEED / Software Development Karlsruhe) 
 
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
 
 $Id: huffdec1.c,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $
 AAC Decoder module
**********************************************************************/

#include <stdio.h>
#ifdef I2R_LOSSLESS
#include <string.h>
#endif

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "buffers.h"
#include "common_m4a.h"
#include "concealment.h"
#include "huffdec2.h"
#include "nok_lt_prediction.h"
#include "port.h"
#include "resilience.h"
#include "concealment.h"

#include "allVariables.h"        /* variables */
/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int huffdecode ( int                      id, 
                 MC_Info*                 mip,
                 HANDLE_AACDECODER        hDec,
                 WINDOW_SEQUENCE*         win, 
                 Wnd_Shape*               wshape, 
                 byte**                   cb_map, 
                 short**                  factors, 
                 byte**                   group, 
                 byte*                    hasmask, 
                 byte**                   mask, 
                 byte*                    max_sfb, 
                 PRED_TYPE                pred_type, 
                 int**                    lpflag, 
                 int**                    prstflag,
                 NOK_LT_PRED_STATUS**     nok_ltp_status, 
                 TNS_frame_info**         tns, 
                 HANDLE_BSBITSTREAM       gc_stream[], 
                 double**                  coef, 
                 enum AAC_BIT_STREAM_TYPE bitStreamType,
                 int                      common_window,
                 Info*                    sfbInfo,
                 QC_MOD_SELECT            qc_select,
                 HANDLE_DECODER_GENERAL   hFault,
                 int                      layer,
                 int                      er_channel,
                 int                      extensionLayerFlag 
                 )
{
  int i, tag, ch, widx, first=0, last=0, chCnt;
  int max_spec_coefficients=-1;

  short global_gain;  /* not used in this routine */

  HANDLE_BUFFER            hVm              = hFault[0].hVm;
  HANDLE_BUFFER            hHcrSpecData     = hFault[0].hHcrSpecData;
  HANDLE_RESILIENCE        hResilience      = hFault[0].hResilience;
  HANDLE_HCR               hHcrInfo         = hFault[0].hHcrInfo;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData = hFault[0].hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment     = hFault[0].hConcealment;
  
  int nPredictorDataPresent;

  BsSetChannel(1);
  
  if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER) && (bitStreamType != MULTICHANNEL_ELD)) {
    tag = GetBits ( LEN_TAG,
                    ELEMENT_INSTANCE_TAG, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );   
  }  
  else {
    tag=0;
  }
   


  switch(id) {
  case ID_LFE:
    max_spec_coefficients = 12;
  case ID_SCE:
    common_window = 0;
    break;
  case ID_CPE:
    if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) {
      if (bitStreamType == MULTICHANNEL_ELD)
        common_window=1;
      else
        common_window = GetBits ( LEN_COM_WIN,
                                COMMON_WINDOW, 
                                hResilience, 
                                hEscInstanceData,
                                hVm );
    }
    break;
  default:
    fprintf(stderr,"Unknown id %d\n", id);
    return(-1);
  }

  if (common_window)
    BsSetSynElemId (2) ; 
  if ((ch = chn_config(id, tag, common_window, mip)) < 0) {
    CommonWarning("Number of channels negativ (huffdec1.c)");
    return -1;
    }

  switch(id) {
  case ID_SCE:
  case ID_LFE:
    widx = mip->ch_info[ch].widx;
    first = ch;
    last = ch;
    hasmask[widx] = 0;
     break;
  case ID_CPE:
    first = ch;
    last = mip->ch_info[ch].paired_ch;
    if ( (common_window) && ( bitStreamType != SCALABLE ) && (bitStreamType != SCALABLE_ER)) 
      {
        widx = mip->ch_info[ch].widx;
        if (bitStreamType == MULTICHANNEL_ELD) 
            max_sfb[widx] = (unsigned char) GetBits ( LEN_MAX_SFBL,
                                                      MAX_SFB, 
                                                      hResilience, 
                                                      hEscInstanceData,
                                                      hVm ); 
        else 
        get_ics_info (&win[widx], 
                      &wshape[widx].this_bk, 
                      group[widx],
                      &max_sfb[widx], 
                      pred_type, 
                      lpflag[widx], 
                      prstflag[widx], 
                      bitStreamType,
                      qc_select,
                      hResilience,
                      hEscInstanceData,
                      nok_ltp_status[first-1],
                      nok_ltp_status[last-1],
                      common_window,
                      hVm,
                      &nPredictorDataPresent
                      );
      win[mip->ch_info[last].widx] = win[widx];

      hasmask[widx] = getmask ( winmap[win[widx]], 
                                group[widx], 
                                max_sfb[widx], 
                                mask[widx],
                                hResilience, 
                                hEscInstanceData,
                                hVm );
      
        
     if ( ( bitStreamType == MULTICHANNEL_ER ) && ( pred_type == NOK_LTP )  && ( nPredictorDataPresent ) ) {
        nok_lt_decode(*win, &max_sfb[widx], 
                      nok_ltp_status[first-1]->sbk_prediction_used,
                      nok_ltp_status[first-1]->sfb_prediction_used, 
                      &nok_ltp_status[first-1]->weight,
                      nok_ltp_status[first-1]->delay,
                      hResilience, 
                      hEscInstanceData, 
                      qc_select, 
                      NO_CORE, 
                      -1, 
                      hVm);
      }
    }
    else { 
      hasmask[mip->ch_info[first].widx] = 0;
      hasmask[mip->ch_info[last].widx] = 0;
    }
    break;
  }
  
  if(debug['v']) {
    fprintf(stderr,"tag %d, common window %d\n", tag, common_window);
    fprintf(stderr,"nch %d, channels %d %d, widx %d %d\n",
          last-first+1, first, last, 
          mip->ch_info[first].widx, mip->ch_info[last].widx);
  }

  chCnt = 0;

  if ((bitStreamType == SCALABLE_ER) && (id == ID_CPE)) {
    first = last = er_channel+1;
  }

  for (i=first; i<=last; i++) {

    BsSetChannel(i) ;
    if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER))
      widx = mip->ch_info[i].widx;
    else
      widx = 0;

    fltclr(coef[i], LN2);
    ConcealmentInitChannel(i, last-first+1, hConcealment);
    if ( ! getics ( hDec,
                    sfbInfo, 
                    common_window, 
                    &win[widx], 
                    &wshape[widx].this_bk,
                    group[widx], 
                    &max_sfb[widx], 
                    pred_type, 
                    lpflag[widx], 
                    prstflag[widx], 
                    cb_map[i], 
                    coef[i], 
                    max_spec_coefficients,
                    &global_gain, 
                    factors[i], 
                    nok_ltp_status[chCnt], 
                    (bitStreamType==SCALABLE_ER)?tns[er_channel]:tns[i],
                    gc_stream[i-first], 
                    bitStreamType,
                    hResilience,
                    hHcrSpecData,
                    hHcrInfo, 
                    hEscInstanceData,
                    hConcealment,
                    qc_select,
                    hVm,
                    extensionLayerFlag,
                    er_channel
#ifdef I2R_LOSSLESS
                    , hDec->sls_quant_channel_temp[i]
#endif

                    ) ) 
      {
        CommonWarning("getics returns error (huffdec1.c)");
        return -1;
      }
  
    if (common_window && id==ID_CPE && bitStreamType == MULTICHANNEL_ER) {
      if (nPredictorDataPresent && pred_type == NOK_LTP && i == first ) {

        BsSetChannel(i+1);
        nok_lt_decode(*win, &max_sfb[widx], 
                      nok_ltp_status[last-1]->sbk_prediction_used,
                      nok_ltp_status[last-1]->sfb_prediction_used, 
                      &nok_ltp_status[last-1]->weight,
                      nok_ltp_status[last-1]->delay,
                      hResilience, 
                      hEscInstanceData, 
                      qc_select, 
                      NO_CORE, 
                      -1, 
                      hVm);
      }
    }
    chCnt++;
  }
  return 0;
}

void get_ics_info ( WINDOW_SEQUENCE*          win, 
                    WINDOW_SHAPE*             wshape, 
                    byte*                     group,
                    byte*                     max_sfb, 
                    PRED_TYPE                 pred_type, 
                    int*                      lpflag, 
                    int*                      prstflag, 
                    enum  AAC_BIT_STREAM_TYPE bitStreamType,
                    QC_MOD_SELECT             qc_select,
                    HANDLE_RESILIENCE         hResilience,
                    HANDLE_ESC_INSTANCE_DATA  hEscInstanceData,
                    NOK_LT_PRED_STATUS*       nok_ltp_left,
                    NOK_LT_PRED_STATUS*       nok_ltp_right,
                    int                       stereoFlag,
                    HANDLE_BUFFER             hVm,
                    int* pPredictorDataPresent )

{
  int i, j;
  int max_pred_sfb = pred_max_bands(mc_info.sampling_rate_idx);
  Info *info;
  *pPredictorDataPresent=0;

  if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) {
    int tmp;

    tmp = GetBits ( LEN_ICS_RESERV,
              ICS_RESERVED, 
              hResilience, 
              hEscInstanceData,
              hVm );	    /* reserved bit */
    if (tmp != 0) fprintf(stderr, "WARNING: ICS - reserved bit is not set to 0\n");

    tmp = (WINDOW_SEQUENCE)GetBits ( LEN_WIN_SEQ,
                                     WINDOW_SEQ, 
                                     hResilience, 
                                     hEscInstanceData,
                                     hVm );
    if (bno&&debug['V']) {
      if ((((tmp==ONLY_LONG_SEQUENCE)||(tmp==LONG_START_SEQUENCE)) &&((*win==EIGHT_SHORT_SEQUENCE)||(*win==LONG_START_SEQUENCE)))||
          (((tmp==LONG_STOP_SEQUENCE)||(tmp==EIGHT_SHORT_SEQUENCE))&&((*win==ONLY_LONG_SEQUENCE)  ||(*win==LONG_STOP_SEQUENCE))))
        fprintf(stderr,"Non meaningful window sequence transistion %i %i %i\n",tmp, *win, LONG_STOP_SEQUENCE);
    }
    *win = tmp;

    tmp = GetBits ( LEN_WIN_SH,
                    WINDOW_SHAPE_CODE, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );
    if (bno&debug['V']) {
      if (tmp-*wshape) fprintf(stderr,"# WndShapeSW detected\n");
    }
    if( qc_select == AAC_LD && tmp == 1 )
      *wshape = WS_ZPW;  /* For Low delay set the shape to ZPW */
    else
      *wshape = (WINDOW_SHAPE)tmp;
  }
  if ( ( info = winmap[*win] ) == NULL ) {
    CommonExit(1,"bad window code");
  }
  /*
   * max scale factor, scale factor grouping and prediction flags
   */
  prstflag[0] = 0;
  if (info->islong) {
    *max_sfb = (unsigned char) GetBits ( LEN_MAX_SFBL,
                                         MAX_SFB, 
                                         hResilience, 
                                         hEscInstanceData,
                                         hVm );
    group[0] = 1;

    switch(pred_type) {
    case PRED_NONE:
    case MONOPRED:

      if ((lpflag[0] = GetBits ( LEN_PRED_PRES,
                                 PREDICTOR_DATA_PRESENT, 
                                 hResilience, 
                                 hEscInstanceData,
                                 hVm ))) {
        if (debug['V']) fprintf(stderr,"# Pred detected\n");
        if ((prstflag[0] = GetBits ( LEN_PRED_RST,
                                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                     hResilience, 
                                     hEscInstanceData,
                                     hVm ))) {
          for(i=1; i<LEN_PRED_RSTGRP+1; i++)
            prstflag[i] = GetBits ( LEN_PRED_RST,
                                    MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                    hResilience, 
                                    hEscInstanceData,
                                    hVm );
        }
        j = ( (*max_sfb < max_pred_sfb) ? 
              *max_sfb : max_pred_sfb ) + 1;
        
        for (i = 1; i < j; i++)
	  lpflag[i] = GetBits ( LEN_PRED_ENAB,
                                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                hResilience, 
                                hEscInstanceData,
                                hVm );  
        for ( ; i < max_pred_sfb+1; i++)
          lpflag[i] = 0;

      }       
      break;

    case NOK_LTP:
      if((*pPredictorDataPresent=GetBits(1, 
                                        PREDICTOR_DATA_PRESENT, 
                                         hResilience, 
                                         hEscInstanceData, 
                                         hVm))) {
        if (debug['V']) fprintf(stderr,"# LTP detected\n");
        if(bitStreamType != MULTICHANNEL_ER) {
          nok_lt_decode(*win, max_sfb, 
                        nok_ltp_left->sbk_prediction_used,
                        nok_ltp_left->sfb_prediction_used, 
                        &nok_ltp_left->weight,
                        nok_ltp_left->delay,
                        hResilience, 
                        hEscInstanceData, 
                        qc_select, 
                        NO_CORE, 
                        -1, 
                        hVm);

          if(stereoFlag)
            nok_lt_decode(*win, max_sfb, 
                          nok_ltp_right->sbk_prediction_used,
                          nok_ltp_right->sfb_prediction_used, 
                          &nok_ltp_right->weight,
                          nok_ltp_right->delay,
                          hResilience, 
                          hEscInstanceData, 
                          qc_select, 
                          NO_CORE, 
                          -1, 
                          hVm);
        }

        if(!stereoFlag && bitStreamType == MULTICHANNEL_ER) {
          nok_lt_decode(*win, max_sfb, 
                        nok_ltp_left->sbk_prediction_used,
                        nok_ltp_left->sfb_prediction_used, 
                        &nok_ltp_left->weight,
                        nok_ltp_left->delay,
                        hResilience, 
                        hEscInstanceData, 
                        qc_select, 
                        NO_CORE, 
                        -1, 
                        hVm);
        }

      }
      else
        {
          nok_ltp_left->sbk_prediction_used[0] = 0;
	if(stereoFlag)
	  nok_ltp_right->sbk_prediction_used[0] = 0;
      }
      break;
      
    default:
      CommonExit(-1,"\nUnknown predictor type");
      break;
    }
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
    lpflag[0] = 0;

    /* Reset LTP flags. */
    if(pred_type == NOK_LTP)
    {
      nok_ltp_left->sbk_prediction_used[0] = 0;
      if(stereoFlag)
	nok_ltp_right->sbk_prediction_used[0] = 0;
    }
  }

  if ( *max_sfb > info->sfb_per_sbk[0] ) {
    if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "get_ics_info: max_sfb (%2d) > sfb_per_sbk (%2d) (huffdec1.c)", *max_sfb, info->sfb_per_sbk[0]);
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
  
  if (debug['p']) {
    if (lpflag[0]) {
      fprintf(stderr,"prediction enabled (%2d):  ", *max_sfb);
      for (i=1; i<max_pred_sfb+1; i++)
        fprintf(stderr," %d", lpflag[i]);
      fprintf(stderr,"\n");
    }
  }
}
