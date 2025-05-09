/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by

 AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
  
 and edited by
 Yoshiaki Oikawa     (Sony Corporation),
 Mitsuyuki Hatanaka  (Sony Corporation),
 Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
 Markus Werner       (SEED / Software Development Karlsruhe) 
 Eugen Dodenhoeft    (Fraunhofer IIS)
 
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
 
 $Id: decoder_tf.c,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $
 AAC Decoder module
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

#include "extension_type.h"

/* son_AACpp */
#include "sony_local.h"

#ifdef DRC
#include "drc.h"
#endif

#define MAXNRSBRELEMENTS 6


/*
---     Global variables from aac-decoder      ---
*/

int debug [256] ;
static int* last_rstgrp_num = NULL;

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

#ifdef AAC_ELD

static int
getExtensionPayload_ER(
                       HANDLE_RESILIENCE        hResilience,
                       HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                       HANDLE_BUFFER            hVm
#ifdef CT_SBR
                       ,SBRBITSTREAM*           ct_sbrBitStr
#endif
#ifdef DRC
                       ,HANDLE_DRC              drc
#endif
                       ,int*                    payloadLength
                       )
{
  int i;
  
  int count = 0;
  int ReadBits = 0;
  
  int Extension_Type = 0;
  int unalignedBits = 0;
  
  Extension_Type = GetBits(4,
                           MAX_ELEMENTS,
                           hResilience,
                           hEscInstanceData,
                           hVm );
  ReadBits+=4;
  
  /* get Byte count of the payload data */
  count = (*payloadLength-ReadBits) >> 3; /* divide with 8 and floor */
  
  if ( count > 0 ) {
    switch (Extension_Type) {
      
#ifdef CT_SBR
    case EXT_SBR_DATA:
    case EXT_SBR_DATA_CRC:
      if((count < MAXSBRBYTES)
         && (ct_sbrBitStr->NrElements < MAXNRSBRELEMENTS)) {
          
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].ExtensionType = Extension_Type;
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload       = 1;
          
          /* 1st element contains only 4bits (right aligned) ! */
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[0] 
            =  GetBits(4,
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience,
                       hEscInstanceData,
                       hVm );
          ReadBits += 4;
          
          count = (*payloadLength-ReadBits)>>3;
          /*unalignedBits = (*payloadLength-ReadBits) % 8;*/
       
          if ( count > 1 ) {
            for (i = 1 ; i <= count ; i++) {
              ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] 
                =  GetBits(8,
                           MAX_ELEMENTS,
                           hResilience,
                           hEscInstanceData,
                           hVm );
              ReadBits+=8;
            }
            count += 1; /* set new Payload length (the 1st nibble + the following bytes) */
            ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload       = count;
          }
          ct_sbrBitStr->NrElements +=1;
          ct_sbrBitStr->NrElementsCore += 1;
      }
      break;
#endif
#ifdef DRC
      case EXT_DYNAMIC_RANGE:
        drc_parse(drc,
                  hResilience,
                  hEscInstanceData,
                  hVm);
      break;
#endif
      /* read unalignedbits */
      unalignedBits = (*payloadLength-ReadBits);
      if ( unalignedBits > 0 && unalignedBits < 8) {
        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements-1].Data[count] = 
          GetBits(unalignedBits,
                      MAX_ELEMENTS,
                      hResilience,
                      hEscInstanceData,
                      hVm );

        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements-1].Data[count] =
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements-1].Data[count] << (8-unalignedBits);

        ReadBits += (unalignedBits);
        count++;
        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements-1].Payload = count; /* incr Payload cnt */
      } else {
        /* should never happen! */
      }
    break;
    
    case EXT_FILL:
    case EXT_DATA_ELEMENT:
    case EXT_SAC_DATA:
    default:
        /* read FillBits */
        for (i = 0 ; i < count-1 ; i++) {
          GetBits(8,
                  MAX_ELEMENTS,
                  hResilience,
                  hEscInstanceData,
                  hVm );
          ReadBits+=8;
        }
        GetBits((*payloadLength-ReadBits), MAX_ELEMENTS, hResilience, hEscInstanceData, hVm);
        ReadBits+=(*payloadLength-ReadBits);
      break;
  } /* switch(Extenstion_Type) */
}

  *payloadLength -= ReadBits;
   
  return ReadBits;
}
#endif


static void
getExtensionPayload( HANDLE_RESILIENCE        hResilience,
                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                     HANDLE_BUFFER            hVm,
#ifdef CT_SBR
        SBRBITSTREAM *ct_sbrBitStr,
#endif
#ifdef DRC
        HANDLE_DRC drc,
#endif
        int count )
{
  int useScalSbr = 1;

  if ( useScalSbr ) {

    /* sbr scalable : payloadlength in byte */
    count = count >> 3;

  } 
  else {
    count = GetBits ( 4,
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
                      hVm );

    if ( count == 15 ) {
      int esc_count = GetBits ( 8,
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
                      hVm );

      count = esc_count + 14;
    }
  }
  
  if ( count > 0 ) {
    int i;
    int Extension_Type = GetBits ( 4,
                                   MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );

    switch (Extension_Type) {
#ifdef CT_SBR
    case EXT_SBR_DATA:
    case EXT_SBR_DATA_CRC:
      if( (count < MAXSBRBYTES) && (ct_sbrBitStr->NrElements < MAXNRELEMENTS)) {
        if (debug['V']) DebugPrintf(0, "# SBR detected\n");

        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].ExtensionType = Extension_Type;
        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload       = count;
        ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[0]       = GetBits ( 4,
                                                                                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                                                                     hResilience,
                                                                                     hEscInstanceData,
                                                                                     hVm );
        
        for (i = 1 ; i < count ; i++) {
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] =  GetBits ( 8,
                                                                                  MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                                                                  hResilience,
                                                                                  hEscInstanceData,
                                                                                  hVm );
        }
        ct_sbrBitStr->NrElements +=1;
        ct_sbrBitStr->NrElementsCore += 1;
      }
      break;
#endif
#ifdef DRC
    case EXT_DYNAMIC_RANGE:
      if (debug['V']) DebugPrintf(0, "# DRC detected\n");
      drc_parse(drc,
                hResilience,
                hEscInstanceData,
                hVm);
      break;
#endif
    case EXT_FILL_DATA:
      if ( GetBits ( 4,
                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                     hResilience,
                     hEscInstanceData,
                     hVm ) != 0 ) CommonWarning("getfill: fill_nibble not '0000'");
        
      for (i = 0 ; i < count-1 ; i++) {
        if ( GetBits ( 8,
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience,
                       hEscInstanceData,
                       hVm ) != 165 )  CommonWarning("getfill: fill_byte not '10100101'");
        }
      break;
    case EXT_FILL:
    case EXT_DATA_ELEMENT:
    case EXT_SAC_DATA:
    default:
      GetBits ( 4,
                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                hResilience,
                hEscInstanceData,
                hVm );
      
      for (i = 0 ; i < count-1 ; i++) {
        GetBits ( 8,
                  MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                  hResilience,
                  hEscInstanceData,
                  hVm );
      }
      break;
    }
  }
}
  
static void getfill (HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                     HANDLE_BUFFER hVm
#ifdef CT_SBR
                     ,SBRBITSTREAM *ct_sbrBitStr
#endif
#ifdef DRC
                     ,HANDLE_DRC drc
#endif
                     )
{
  int cnt;

  if ((cnt = GetBits (LEN_F_CNT,
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
                      hVm )) == (1<<LEN_F_CNT)-1)

    cnt +=  GetBits ( LEN_F_ESC,
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
                      hVm ) - 1;

  if (debug['x'])
    fprintf(stderr, "fill element has %d bytes\n", cnt);

  getExtensionPayload( hResilience,
                       hEscInstanceData,
                       hVm,
#ifdef CT_SBR
                       ct_sbrBitStr,
#endif
#ifdef DRC
                       drc,
#endif
                       cnt<<3 );
}



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


/* -------------------------------------- */
/*           Init AAC-Decoder             */
/* -------------------------------------- */

HANDLE_AACDECODER AACDecodeInit (int samplingRate,
                                 char *aacDebugStr, 
                                 int  block_size_samples,
                                 Info *** sfbInfo, 
                                 int  predictor_type,
                                 int  profile,
                                 const char *decPara)
{
  int i,band;

  
  HANDLE_AACDECODER hDec = (HANDLE_AACDECODER) calloc (1,  sizeof(T_AACDECODER));
  if (!hDec) return hDec ;

  hDec->blockSize= block_size_samples;
  hDec->pred_type = (PRED_TYPE)predictor_type;
  hDec->mip = &mc_info;
  hDec->short_win_in_long = 8;


#ifdef CT_SBR

#ifndef SBR_SCALABLE
  hDec->ct_sbrBitStr.NrElements     =  0;
  hDec->ct_sbrBitStr.NrElementsCore =  0;
#else
  hDec->ct_sbrBitStr[0].NrElements     =  0;
  hDec->ct_sbrBitStr[0].NrElementsCore =  0;
  hDec->ct_sbrBitStr[1].NrElements     =  0;
  hDec->ct_sbrBitStr[1].NrElementsCore =  0;
#endif
#endif



  for(i=0; i<Chans; i++)
  {
    hDec->coef[i] = (double *)mal1(LN2*sizeof(*hDec->coef[0]));
    hDec->data[i] = (double *)mal1(LN2*sizeof(*hDec->data[0]));
    hDec->factors[i] = (short *)mal1(MAXBANDS*sizeof(*hDec->factors[0]));
    hDec->state[i] = (double *)mal1(LN*sizeof(*hDec->state[0]));	/* changed LN4 to LN 1/97 mfd */
    fltclr(hDec->state[i], LN); 
    hDec->cb_map[i] = (byte *)mal1(MAXBANDS*sizeof(*hDec->cb_map[0]));
    hDec->group[i] = (byte *)mal1(NSHORT*sizeof(*hDec->group[0]));
    hDec->lpflag[i] = (int *)mal1(MAXBANDS*sizeof(*hDec->lpflag[0]));
    hDec->prstflag[i] = (int *)mal1((LEN_PRED_RSTGRP+1)*sizeof(*hDec->prstflag[0]));
    hDec->tns[i] = (TNS_frame_info *)mal1(sizeof(*hDec->tns[0]));
    hDec->wnd_shape[i].prev_bk = WS_FHG;
    hDec->wnd_shape_SSR[i].prev_bk= WS_FHG;

    if (predictor_type == MONOPRED)
    {
      hDec->sp_status[i] = (PRED_STATUS *)mal1(LN2*sizeof(*hDec->sp_status[0]));
    }
  }

  for(i=0; i<Winds; i++)
  {
    hDec->mask[i] = (byte *)mal1(MAXBANDS*sizeof(hDec->mask[0]));
  }

  /* coupling channels */

#if (CChans > 0)
  for(i=0; i<CChans; i++)
    {
      int j;
      hDec->cc_coef[i]= (double *)mal1(LN2*sizeof(*hDec->cc_coef[0]));
      for(j=0; j<Chans; j++)
        hDec->cc_gain[i][j] = (double *)mal1(MAXBANDS*sizeof(*hDec->cc_gain[0][0]));
#if (ICChans > 0)
      if (i < ICChans)
        {
          hDec->cc_state[i] = (double *)mal1(LN*sizeof(*hDec->cc_state[0]));
          fltclr(hDec->cc_state[i], LN);
          hDec->cc_wnd_shape[i].prev_bk = WS_FHG;
        }
#endif
    }
#endif
  
  /*
      SSR Profile
  */
  for (i=0; i<Chans; i++){ 
    hDec->spectral_line_vector_for_gc[i] = (double *)calloc(block_size_samples,sizeof(*hDec->spectral_line_vector_for_gc[0]));
    hDec->imdctBufForGC[i] = (double *)calloc(block_size_samples*2, sizeof(*hDec->imdctBufForGC[0]));
    hDec->DBandSigBufForOverlapping[i] = (double **)calloc(NBANDS, sizeof(double *));
    hDec->DBandSigBuf[i] = (double **)calloc(NBANDS, sizeof(double *));
    hDec->gainInfo[i] = (GAINC **)calloc(NBANDS, sizeof(GAINC *));
    for (band = 0; band < NBANDS; band++) {
      hDec->DBandSigBufForOverlapping[i][band] =
        (double *)calloc(block_size_samples/NBANDS*2, sizeof(*hDec->DBandSigBufForOverlapping[i][0]));
      hDec->DBandSigBuf[i][band] =
        (double *)calloc(block_size_samples/NBANDS, sizeof(*hDec->DBandSigBuf[i][0]));
      hDec->gainInfo[i][band] = (GAINC *)calloc(hDec->short_win_in_long, sizeof(GAINC));
    }
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

  if (aacDebugStr)
  {
    for (i=0;i<(int)strlen(aacDebugStr);i++){
      debug[(int)aacDebugStr[i]]=1;
      fprintf(stderr,"   !!! AAC debug option: %c recognized.\n", aacDebugStr[i]);
    }
  }
  
  /* Set sampling rate */

  for (i=0; i<(1<<LEN_SAMP_IDX); i++) {
    if (sampling_boundaries[i] <= samplingRate)
      break;
  }
  
  if (i == (1<<LEN_SAMP_IDX)) 
    CommonExit(1,"Unsupported sampling frequency %d", samplingRate);
  
  prog_config.sampling_rate_idx = mc_info.sampling_rate_idx = i;

#if (CChans > 0)
  init_cc(hDec->lpflag[CChans]); 
#endif
  
  for(i = 0; i < Chans; i++)
    {
      hDec->wnd_shape_SSR[i].prev_bk = 0;
      hDec->wnd_shape[i].prev_bk = 0;
      hDec->wnd_shape[i].this_bk = 0;
    }
  

  huffbookinit(block_size_samples, mc_info.sampling_rate_idx);
  predinit(hDec);

  winmap[0] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[1] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[2] = win_seq_info[EIGHT_SHORT_SEQUENCE];
  winmap[3] = win_seq_info[ONLY_LONG_SEQUENCE];

  *sfbInfo = winmap;

#ifdef DRC
  hDec->drc = drc_init();
  {
    double p1=0.0, p2=0.0;
    int p3=-1;
    const char *tmp = strstr(decPara, "drc ");
    if (tmp != NULL) {
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of "drc" */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if (*tmp!=0) p1 = atof(tmp);
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of first parameter */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if (*tmp!=0) p2 = atof(tmp);
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of second parameter */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if ((*tmp!=0)&&(*tmp!='-')) p3 = atoi(tmp);
    }
    drc_set_params(hDec->drc, p1, p2, p3);
  }
#endif

  return hDec ;
}

/* ------------------------------------------ */
/*  Release data allocated by AACDecoderInit  */
/* ------------------------------------------ */

void AACDecodeFree (HANDLE_AACDECODER hDec)
{
  int i ;

  for(i=0; i<Chans; i++)
  {
    int band;
    free (hDec->state[i]);
    free (hDec->coef[i]) ;
    free (hDec->data[i]) ;
    free (hDec->factors[i]) ;
    free (hDec->cb_map[i]) ;
    free (hDec->group[i]) ;
    free (hDec->lpflag[i]) ;
    free (hDec->prstflag[i]) ;
    free (hDec->tns[i]) ;

    for (band = 0; band < NBANDS; band++) {
      free(hDec->DBandSigBufForOverlapping[i][band]);
      free(hDec->DBandSigBuf[i][band]);
      free(hDec->gainInfo[i][band]);
    }

    free(hDec->spectral_line_vector_for_gc[i]);
    free(hDec->imdctBufForGC[i]);
    free(hDec->DBandSigBufForOverlapping[i]);
    free(hDec->DBandSigBuf[i]);
    free(hDec->gainInfo[i]);


    if (hDec->pred_type == MONOPRED)
    {
      free (hDec->sp_status[i]) ;
    }
  }

  if (NULL != last_rstgrp_num) {
    free (last_rstgrp_num);
  }
  for(i=0; i<Winds; i++)
    free (hDec->mask[i]) ;

#if (CChans > 0)
  for(i=0; i<CChans; i++)
  {
    int j;
    free (hDec->cc_coef[i]) ;
    for(j=0; j<Chans; j++)
      free (hDec->cc_gain[i][j]) ;
#if (ICChans > 0)
    if (i < ICChans)
    {
      free (hDec->cc_state[i]) ;
    }
#endif
  }
#endif

#ifdef DRC
  drc_free (hDec->drc);
#endif

  free (hDec) ;
}

/* -------------------------------------- */
/*           DECODE 1 frame               */
/* -------------------------------------- */
 
int AACDecodeFrame ( HANDLE_AACDECODER        hDec,
                     HANDLE_BSBITSTREAM       fixed_stream,
                     HANDLE_BSBITSTREAM       gc_stream[MAX_TIME_CHANNELS],
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     enum AAC_BIT_STREAM_TYPE bitStreamType, 
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     Info**                   sfbInfo, 
                     byte                     sfbCbMap[MAX_TIME_CHANNELS][MAXBANDS],
                     short*                   pFactors[MAX_TIME_CHANNELS],
                     HANDLE_DECODER_GENERAL   hFault,
                     QC_MOD_SELECT            qc_select,
                     NOK_LT_PRED_STATUS**     nok_lt_status,
                     FRAME_DATA*              fd ,
                     int                      layer,
                     int                      er_channel,
                     int                      aacExtensionLayerFlag
                     )
{
  MC_Info *mip = hDec->mip ;
  Ch_Info *cip ;

  byte hasmask [Winds] ;

  int           left, right, ele_id, wn, ch ;
#ifdef DRC
  int           i;
#endif
  int           chCnt;
  int           outCh;
  int           sfb;
  Info*         sfbInfoP;
  Info          sfbInfoNScale;
  unsigned char nrOfSignificantElements = 0; /* SCE (mono) and CPE (stereo) only */

  int samplFreqIdx = mip->sampling_rate_idx;
  /*(fd==NULL) ? -1 : (int)fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value*/
  
  HANDLE_BUFFER            hVm              = hFault[0].hVm;
  HANDLE_RESILIENCE        hResilience      = hFault[0].hResilience;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData = hFault[0].hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment     = hFault[0].hConcealment;

  if(debug['n']) {
    /*     fprintf(stderr, "\rblock %ld", bno); */
    fprintf(stderr,"\n-------\nBlock: %ld\n", bno);
  }
  
  /* Set the current bitStream for aac-decoder */
  setHuffdec2BitBuffer(fixed_stream);

  reset_mc_info(mip);

  if (( bitStreamType != SCALABLE ) && ( bitStreamType != SCALABLE_ER )) {

    if ( (bitStreamType == MULTICHANNEL_ER) || (bitStreamType == MULTICHANNEL_ELD)) {
      if ( numChannels == 2 ) {
        ele_id = ID_CPE;
      }
      else {
        ele_id = ID_SCE;
      }
    }
    else
    {
        ele_id = GetBits ( LEN_SE_ID,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience,
                           hEscInstanceData,
                           hVm );
      }
    if ( ele_id == ID_SCE || ele_id == ID_CPE ) {
      nrOfSignificantElements++;
    }
    sfbInfoP = &sfbInfoNScale;
  }
  else {
    sfbInfoP = sfbInfo[windowSequence[MONO_CHAN]];
    if (numChannels == 2) {
      ele_id = ID_CPE;
      if ( commonWindow ) {
        hDec->wnd[mip->ch_info[0].widx] = windowSequence[MONO_CHAN] ;
      } else {
        CommonExit(-1,"\n 2 channels but no common window: not supported");
      }
    }
    else { 
      ele_id=ID_SCE;
      hDec->wnd[mip->ch_info[0].widx]=windowSequence[MONO_CHAN];
    }
  }
  if(debug['v'])
    fprintf(stderr, "\nele_id %d\n", ele_id);

  BsSetSynElemId (ele_id) ;

#ifdef CT_SBR
#ifndef SBR_SCALABLE
  if ( layer == 0 ) {
    ResetSbrBitstream( &hDec->ct_sbrBitStr );
  }
#else
  if ( layer == 0 ) {
    ResetSbrBitstream( &hDec->ct_sbrBitStr[0] );
    ResetSbrBitstream( &hDec->ct_sbrBitStr[1] );
  }
#endif
#endif

#ifdef DRC
  drc_reset(hDec->drc);
#endif


  while ( ele_id != ID_END ) {
    /* get audio syntactic element */

    /* only SCE and FIL are currently allowed, if the bitstream might be disturbed */
    if ( ( GetEPFlag( hResilience ) ) && ( BsGetReadErrorForDataElementFlagEP ( COMMON_WINDOW , hEscInstanceData ) 
                                           || ! (( ele_id == ID_SCE ) 
                                           ||    ( ele_id == ID_CPE )
                                           ||    ( ele_id == ID_FIL ) ) 
                                           || nrOfSignificantElements > 1 ) ) {
      break;
    }
    
    switch (ele_id) {
    case ID_SCE:                /* single channel */
    case ID_CPE:                /* channel pair */
    case ID_LFE:                /* low freq effects channel */
     
      if ( huffdecode ( ele_id,  
                        mip, 
                        hDec,
                        hDec->wnd, 
                        hDec->wnd_shape,
                        hDec->cb_map,
                        hDec->factors, 
                        hDec->group, 
                        hasmask,
                        hDec->mask, 
                        max_sfb, 
                        hDec->pred_type, 
                        hDec->lpflag, 
                        hDec->prstflag, 
                        nok_lt_status, 
                        hDec->tns, 
                        gc_stream, 
                        hDec->coef,
                        bitStreamType,
                        commonWindow,
                        sfbInfoP,
                        qc_select,
                        hFault,
                        layer,                        /* AAC layer          */
                        er_channel,                   /* er_channel         */
                        aacExtensionLayerFlag         /* extensionLayerFlag */
                        ) < 0 )
        CommonExit(1,"huffdecode (decoder.c)");

        if ((bitStreamType == SCALABLE_ER) && (er_channel == 0) && (ele_id == ID_CPE)) 
          return GetReadBitCnt ( hVm );

#ifdef CT_SBR

#ifndef SBR_SCALABLE
  /* should be layer == firstAacLayer */
  if ( layer == 0 ) {
      if ( ele_id == ID_SCE )
        sbrSetElementType( &hDec->ct_sbrBitStr, SBR_ID_SCE );
      if ( ele_id == ID_CPE )
        sbrSetElementType( &hDec->ct_sbrBitStr, SBR_ID_CPE );
  }
#else
  /* should be layer == firstAacLayer */
  if ( layer == 0 ) {
    if (( bitStreamType == SCALABLE ) || ( bitStreamType == SCALABLE_ER )) {
      sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_CPE );
      sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_CPE );
    }
    else {
      if ( ele_id == ID_SCE ){
        sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_SCE );
        sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_SCE );
      }
      if ( ele_id == ID_CPE ){
        sbrSetElementType( &hDec->ct_sbrBitStr[0], SBR_ID_CPE );
        sbrSetElementType( &hDec->ct_sbrBitStr[1], SBR_ID_CPE );
      }
    }
  }
#endif
#endif

      if ((bitStreamType == SCALABLE) || (bitStreamType == SCALABLE_ER)){
        for (ch=0; ch<Chans; ch++ ) {
          if (!(mip->ch_info[ch].present)) 
            continue;
 /* copy codebooks of ALL sfbs and groups*/
          for (sfb=0; sfb<MAXBANDS; sfb++) {
            if ( numChannels == 2 ) {
              sfbCbMap[ch-1][sfb] = hDec->cb_map[ch][sfb];
            }
            if ( numChannels == 1 ) {
              sfbCbMap[0][sfb] = hDec->cb_map[ch][sfb];
            }
          }
        }
      }
      break;
#if (CChans > 0)
    case ID_CCE:                /* coupling channel */
      if ( getcc ( mip,
                   sfbInfoP,
                   hDec,
                   hDec->pred_type,
                   hDec->cc_wnd, 
                   hDec->cc_wnd_shape, 
                   hDec->cc_coef, 
                   hDec->cc_gain,
                   hDec->cb_map,
                   bitStreamType, /* AAC BIT_STREAM_TYPE */
                   qc_select, /* QC_MOD_SELECT */
                   gc_stream[0], /* gc_stream */ /*unsure*/
                   hVm, /* hVm */
                   hFault[0].hHcrSpecData, /* hHcrSpecData,*/ 
                   hFault[0].hHcrInfo, /* hHcrInfo,*/  
		   hEscInstanceData,
                   hResilience, /* HANDLE_RESILIENCE */
                   hConcealment /* HANDLE_CONCEALMENT */) < 0)
        CommonExit(1,"getcc");
      break;
#endif                          
    case ID_DSE:                /* data element */
      if ( getdata ( &hDec->d_tag, 
                     &hDec->d_cnt, 
                     hDec->d_bytes,
                     hResilience, 
                     hEscInstanceData,
                     hVm ) < 0)
        CommonExit(1,"data channel");
      break;
    case ID_PCE:                /* program config element */
      get_prog_config ( &prog_config,
                        hDec->blockSize,
                        hResilience, 
                        hEscInstanceData,
                        hVm );
      break;
      
      
    case ID_FIL:                /* fill element */
      getfill ( hResilience,
                hEscInstanceData,
                hVm
#ifdef CT_SBR
#ifndef SBR_SCALABLE
                ,&hDec->ct_sbrBitStr
#else
                ,&hDec->ct_sbrBitStr[0]
#endif
#endif
#ifdef DRC
                ,hDec->drc
#endif
                );
      break;

    default:
      CommonExit(1, "Element not supported: %d\n", ele_id);

      break;
    }
    if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) {
      if ((bitStreamType == MULTICHANNEL_ER) || (bitStreamType == MULTICHANNEL_ELD)){
/* ER payload parser not only for AAC ELD necessary (snl:2007-06-19) */      
#ifdef AAC_ELD
        unsigned int au;
        int payloadLength = 0;
        int AuLength = 0;

        int AacPayloadLength = BsCurrentBit( (HANDLE_BSBITSTREAM)fixed_stream );
        for (au = 0; au<fd->layer[layer].NoAUInBuffer;au++) {
          AuLength += (fd->layer[layer].AULength[au] );
          if(AuLength>=AacPayloadLength)
            break;
        }
        
        payloadLength = AuLength - AacPayloadLength;
        
#ifdef AAC_ELD
	/* AAC-ELD SBR payload */
	if (fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD) {
	  if (fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.eldSpecificConfig.ldSbrPresentFlag.value == 1) {
	    unsigned int ReadBits = 0;
	    int count = (payloadLength >> 3);
	    int Extension_Type = fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.eldSpecificConfig.ldSbrCrcFlag.value ? EXT_SBR_DATA_CRC : EXT_SBR_DATA;
	    
	     /* push_low_delay_sbr_payload */	     
        if((count < MAXSBRBYTES) /*&& (ct_sbrBitStr->NrElements < MAXNRSBRELEMENTS)*/)
        {   	  
	  SBRBITSTREAM *ct_sbrBitStr = &hDec->ct_sbrBitStr;	               

          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].ExtensionType = Extension_Type;
          ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload       = count;
          for (i = 0 ; i < count ; i++) {
            ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] = GetBits(8, MAX_ELEMENTS, hResilience, hEscInstanceData, hVm );
            ReadBits+=8;
          }	  
	  {
	  /* read unalignedbits */
          unsigned int unalignedBits = (payloadLength-ReadBits);
          if ( unalignedBits > 0 && unalignedBits < 8) {
            ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] = 
              GetBits(unalignedBits, MAX_ELEMENTS, hResilience, hEscInstanceData, hVm );

            ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] =
             ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Data[i] << (8-unalignedBits);

            ReadBits += (unalignedBits);
            count++;
            ct_sbrBitStr->sbrElement[ct_sbrBitStr->NrElements].Payload = count; /* incr Payload cnt */
	    }
	  }
 
          ct_sbrBitStr->NrElements     += 1;
          ct_sbrBitStr->NrElementsCore += 1;        	  
	}
	payloadLength -= ReadBits;
       }       
      }
#endif				
        if ( payloadLength > 8 ) { /* if playloadLength < 8 we have align bits */
          /* parse ER-ExtensionPayload */
          
          getExtensionPayload_ER(
                            hResilience,
                            hEscInstanceData,
                            hVm
#ifdef CT_SBR
#ifndef SBR_SCALABLE
                            ,&hDec->ct_sbrBitStr
#else
                            ,&hDec->ct_sbrBitStr[0]
#endif
#endif
#ifdef DRC
                            ,hDec->drc
#endif
                            , &payloadLength
                            );
        }
#endif
        ele_id = ID_END;
      }
      else
      {
          ele_id = GetBits ( LEN_SE_ID,
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience,
                             hEscInstanceData,
                             hVm );
      }
      if ( ele_id == ID_SCE || ele_id == ID_CPE ) {
        nrOfSignificantElements++;
      }
    }
    else {

/* parse aac scalable extension payload */
      int ExtensionPayloadLength;
      int AuLength = 0;
      unsigned int au;
      int AacPayloadLength = BsCurrentBit( (HANDLE_BSBITSTREAM)fixed_stream );
#ifdef CT_SBR
      SBRBITSTREAM *hSbrBitstream;

#ifndef SBR_SCALABLE
      hSbrBitstream = &hDec->ct_sbrBitStr;
#else
      if(layer==0)
        hSbrBitstream = &hDec->ct_sbrBitStr[0];
      else
        hSbrBitstream = &hDec->ct_sbrBitStr[1];
#endif
#endif

      for (au = 0; au<fd->layer[layer].NoAUInBuffer;au++) {
        AuLength += (fd->layer[layer].AULength[au] );
        if(AuLength>=AacPayloadLength)
          break;
      }

      ExtensionPayloadLength = AuLength - AacPayloadLength;

      getExtensionPayload( hResilience,
              hEscInstanceData,
              hVm,
#ifdef CT_SBR
              hSbrBitstream,
#endif
#ifdef DRC
              hDec->drc,
#endif
              ExtensionPayloadLength );

      ele_id=ID_END;

    }
    if(debug['v']) {
      fprintf(stderr, "\nele_id %d\n", ele_id);
    }
  }
  check_mc_info ( mip, 
                  ( ! mc_info.mcInfoCorrectFlag && default_config ) || ( bitStreamType == SCALABLE ) || ( bitStreamType == SCALABLE_ER ),
                  hEscInstanceData,
                  hResilience
                  ); 

#ifdef CT_SBR
#ifndef SBR_SCALABLE
  if ( hDec->ct_sbrBitStr.NrElements ) {
    if ( hDec->ct_sbrBitStr.NrElements != hDec->ct_sbrBitStr.NrElementsCore ) {
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


#ifdef DRC
  drc_map_channels(hDec->drc, mip);
#endif

#if (ICChans > 0)
  /* process independently switched coupling channels */
#ifdef 	DRC
  /* apply dynamic range control */ 
  i=0;
  for (ch=0; ch<mip->ncch; ch++) {
#ifdef CT_SBR
    wn = mip->ch_info[ch].widx;
#endif
     if (mip->cc_ind[ch]) 
     {
       	      drc_apply(hDec->drc, 
		 hDec->cc_coef[ch], 
		 Chans+i, 
#ifdef CT_SBR 
#ifndef SBR_SCALABLE
                 hDec->ct_sbrBitStr.NrElements, 
#else
                 hDec->ct_sbrBitStr[0].NrElements, 
#endif
                 hDec->wnd [wn] 
#endif 
                 ); 
       i++;	 
     } 
  }
#endif
  
  if(!bno)
    for (ch=0; ch<Chans; ch++)
      hDec->wnd_shape[ch].prev_bk = hDec->wnd_shape[ch].this_bk;
	   
  ind_coupling(mip, 
               hDec->wnd, 
               hDec->wnd_shape, 
               hDec->cc_wnd, 
               hDec->cc_wnd_shape, 
               hDec->cc_coef ,
               hDec->cc_state,
               hDec->blockSize
	       );
  
#endif  

  ConcealmentCheckClassBufferFullnessEP(GetReadBitCnt(hVm), 
                                         hResilience, 
                                         hEscInstanceData, 
                                         hConcealment); 
  
   if (ConcealmentGetEPprematurely(hConcealment)) 
     hEscInstanceData = ConcealmentGetEPprematurely(hConcealment); 
  
   if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) { 
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
    
    /* m/s stereo */

     for (ch=0; ch<Chans; ch++) { 
       cip = &mip->ch_info[ch]; 
       if ((cip->present) && (cip->cpe) && (cip->ch_is_left)) {   
         wn = cip->widx; 
         if(hasmask[wn]) { 
           left = ch; 
           right = cip->paired_ch; 
           hDec->info = winmap[hDec->wnd[wn]]; 
           map_mask(hDec->info, hDec->group[wn], hDec->mask[wn], hDec->cb_map[right], hasmask[wn]); 
        
           ConcealmentMiddleSideStereo(hDec->info, 
                                       hDec->group[wn], 
                                       hDec->mask[wn], 
                                       hDec->coef, 
                                       left, 
                                       right, 
                                       hConcealment); 
          
          synt(hDec->info, hDec->group[wn], hDec->mask[wn], hDec->coef[right], hDec->coef[left]); 
         } 
       } 
     } 
   } 
  
  /* Common part for aac_raw and aac_scaleable */
   chCnt = 0; 
  
   for (ch=0; ch<Chans; ch++)  
     { 
        
       BsSetChannel (ch); 
       
       
       if (! ( hEscInstanceData && ( BsGetErrorForDataElementFlagEP ( SECT_CB, hEscInstanceData ) ) ) )  
         {
           if (!(mip->ch_info[ch].present)) continue; 
           if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) 
             { 
               wn = mip->ch_info[ch].widx; 
             } 
           else 
             { 
               wn = 0; 
             } 
          
           hDec->info = winmap[hDec->wnd[wn]]; 
          
#ifdef PNS
          
          /* pns for scaleable object type is decoded in tfScaleableAUDecode() */
           if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER))  
             { 
              /*calc PNS spectrum */ 
               pns( mip,   
                   hDec->info,   
                    ch,   
                    hDec->group[wn],   
                    hDec->cb_map[ch],   
                    hDec->factors[ch],  
                    hDec->lpflag[wn],  
                    hDec->coef );  
             } 
#endif /*PNS*/
           chCnt++; 
         } 
       else  
         { 
           if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) { 
             printf( "AacDecodeFrame: SI disturbed\n" ); 
             printf( "AacDecodeFrame: --> pns disabled (channel %d)\n", ch ); 
           } 
         } 
     } 
  
  if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)) {  
    /* intensity stereo and prediction */
    chCnt = 0;
      for (ch=0; ch<Chans; ch++) { 
    
           ConcealmentSetChannel(ch, hConcealment); 
    
           if (!(mip->ch_info[ch].present)) continue; 
           wn = mip->ch_info[ch].widx; 
           hDec->info = winmap[hDec->wnd[wn]]; 
    
           if (hDec->pred_type == MONOPRED && mip->profile == Main_Profile) { 
              predict(hDec->info,  
                     hDec->lpflag[wn],  
                     hDec->sp_status[ch],  
                     hConcealment, 
                     hDec->coef[ch]); 
           } 
           intensity(mip, hDec->info, ch,  
                     hDec->group[wn], hDec->cb_map[ch], hDec->factors[ch],  
                     hDec->lpflag[wn], hDec->coef); 
    
           if (hDec->pred_type == MONOPRED && ! mip->profile == Main_Profile) { 
             if (*hDec->lpflag[wn] != 0) { 
               if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) { 
                 CommonExit(1,"AacDecodeFrame: prediction not allowed in this profile!"); 
               } 
               else { 
                 if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) { 
                   printf("AacDecodeFrame: prediction not allowed in this profile\n"); 
                 } 
               } 
             } 
           } 
           chCnt++; 
         } 
    
    chCnt = 0;
    for (ch=0; ch<Chans; ch++) 
      {
     
      if (NULL == last_rstgrp_num) {
	unsigned int maxChannel = 0;
	unsigned int i = 0;
	for (; i < Chans; i ++) {
	  if (mip->ch_info[ch].present == 1) {
	   maxChannel = i;
	  }
	}
	/* allocate memory for the predictor reset number array*/
	last_rstgrp_num = (int*) malloc (sizeof (int) * maxChannel);
      }
		   
	      
      BsSetChannel(ch);
                  
      if (!(mip->ch_info[ch].present)) continue;
      wn = mip->ch_info[ch].widx;
      hDec->info = winmap[hDec->wnd[wn]];
      
      /* predictor reset */
      left = ch;
      right = left;
      if ((mip->ch_info[ch].cpe) && (mip->ch_info[ch].common_window))
        /* prstflag's shared by channel pair */
        right = mip->ch_info[ch].paired_ch;
      if (hDec->pred_type == MONOPRED)
        predict_reset(hDec->info, hDec->prstflag[wn], hDec->sp_status, 
                      left, right, last_rstgrp_num);
      else if (hDec->pred_type == NOK_LTP) /* Long term prediction.  */
        nok_lt_predict(hDec->info, hDec->wnd[wn], 
                       hDec->wnd_shape[wn].this_bk,
                       hDec->wnd_shape[ch].prev_bk,
                       nok_lt_status[chCnt]->sbk_prediction_used,
                       nok_lt_status[chCnt]->sfb_prediction_used,
                       nok_lt_status[chCnt], nok_lt_status[chCnt]->weight, 
                       nok_lt_status[chCnt]->delay, hDec->coef[ch],
                       (qc_select == AAC_LD) ? hDec->blockSize : BLOCK_LEN_LONG,
                       (qc_select == AAC_LD) ? 
                       hDec->blockSize * BLOCK_LEN_SHORT / BLOCK_LEN_LONG : 
                       BLOCK_LEN_SHORT,
                       samplFreqIdx,
                       hDec->tns[ch], qc_select);

      chCnt++;
      
      /* Save window shape for next frame (used by LTP tool)*/
      hDec->wnd_shape[ch].prev_bk = hDec->wnd_shape[wn].this_bk;

#ifdef PNS
      /* PNS predictor reset, works only for MONOPRED ! */
      if(hDec->pred_type==MONOPRED)
        predict_pns_reset(hDec->info, hDec->sp_status[ch], hDec->cb_map[ch]);
#endif /*PNS*/
      
#if (CChans > 0)

       /* if cc_domain indicates before TNS */
       coupling(hDec->info, 
                mip, 
                hDec->coef,  
                hDec->cc_coef,  
                hDec->cc_gain,  
                ch, 
                CC_DOM,  
                hDec->cc_wnd, 
                hDec->cb_map, 
                !CC_IND,
		-1); 
#endif
    
      /* tns */
      
       if (! ( hEscInstanceData && ( BsGetErrorForDataElementFlagEP ( N_FILT       , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( COEF_RES     , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( LENGTH       , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( ORDER        , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( DIRECTION    , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( COEF_COMPRESS, hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( COEF         , hEscInstanceData ) || 
                                     BsGetErrorForDataElementFlagEP ( GLOBAL_GAIN  , hEscInstanceData ) ) ) )  
         { 
           if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER))  
             { 
               int i,j ; 
              
               for (i=j=0; i<hDec->tns[ch]->n_subblocks; i++)  
                 { 
                   if (debug['T']) { 
                     fprintf(stderr, "%ld %d %d\n", bno, ch, i); 
                     print_tns( &(hDec->tns[ch]->info[i])); 
                   } 
                  
                   if (hDec->tns[ch]->info[i].n_filt) 
                     ConcealmentTemporalNoiseShaping(hDec->coef, 
                                                     hDec->info, 
                                                     (short) i, 
                                                     (short) ch, 
                                                     hConcealment); 
                  
                   tns_decode_subblock(&hDec->coef[ch][j], 
                                       max_sfb[wn], 
                                       hDec->info->sbk_sfb_top[i], 
                                       hDec->info->islong, 
                                       &(hDec->tns[ch]->info[i]), 
                                       qc_select,samplFreqIdx);       
                  
                   j += hDec->info->bins_per_sbk[i]; 
                 } 
             } 
         } 
       else  
         { 
           if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) { 
             printf( "AacDecodeFrame: tns data disturbed\n" ); 
             printf( "AacDecodeFrame: --> tns disabled (channel %d)\n", ch ); 
           } 
         } 
      
#if (CChans > 0)
      /* if cc_domain indicated after TNS */
      coupling(hDec->info, 
                mip,  
                hDec->coef,  
                hDec->cc_coef,  
                hDec->cc_gain,  
                ch,  
                !CC_DOM,  
                hDec->cc_wnd, 
               hDec->cb_map, 
               !CC_IND,
	       -1); 
#endif
      
#ifdef	DRC
      {
#ifdef CT_SBR
        SBRBITSTREAM *hSbrBitstream;
        
#ifndef SBR_SCALABLE
        hSbrBitstream = &hDec->ct_sbrBitStr;
#else
        if(layer==0)
          hSbrBitstream = &hDec->ct_sbrBitStr[0];
        else
          hSbrBitstream = &hDec->ct_sbrBitStr[1];
#endif
#endif
        /* apply dynamic range control to each audio channel */
        drc_apply(hDec->drc, hDec->coef[ch], ch
#ifdef CT_SBR
                  ,(hSbrBitstream->NrElements != 0/* || mip->sbrPresentFlag != 0*/)
                  ,hDec->wnd[wn]
#endif
                  );        
      }
#endif
    }
  }
  
  /* Concealment save and restore coefficient functionality */
  ConcealmentMainEntry(mip,
                       hDec->wnd,
                       hDec->wnd_shape,
                       windowSequence,
                       window_shape,
                       hDec->lpflag,
                       hDec->group,
                       hasmask,
                       hDec->mask,
                       hDec->coef,
                       hResilience,
                       hConcealment);


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
          if (outCh!=ch) {
            memcpy(hDec->sls_quant_channel_temp[outCh], hDec->sls_quant_channel_temp[ch],1024*sizeof(int));
          }
#endif

          outCh++;
        }
    }

  /* pass scalefactors to calling function */
  if ((bitStreamType == SCALABLE ) || ( bitStreamType == SCALABLE_ER )) {
    if( pFactors!=NULL ) {
      if( numChannels == 1 ) {
        	      memcpy( pFactors[0], hDec->factors[0], sizeof(*hDec->factors[0]) * MAXBANDS);
        }
      else {
	      memcpy( pFactors[0], hDec->factors[1], sizeof(*hDec->factors[1]) * MAXBANDS);
	      memcpy( pFactors[1], hDec->factors[2], sizeof(*hDec->factors[2]) * MAXBANDS);
       }
    }
  }
  
  bno++;
  
  return GetReadBitCnt ( hVm );
  
}

void decodeBlockType ( int coded_types, 
                       int granules, 
                       int gran_block_type[], 
                       int act_gran )
{
  static int ty_seq[4][2]={{0,1},{2,3},{2,3},{0,1}};
  register int tmp;
  register int i;
  
  if( act_gran < 0 ) {
    gran_block_type[0] = (coded_types >> (granules - 1)) & 0x03;
  } else if( act_gran == 0 ) {
    gran_block_type[0] = coded_types & 0x03;
  }
  for( i=1; i<granules; i++ ) {
    if( act_gran < 0 ) {
      tmp = ((coded_types >> (granules - 1 - i)) & 0x01);
      gran_block_type[i] = ty_seq[gran_block_type[i-1]][tmp];
    } else if( act_gran == i ) {
      tmp = coded_types  & 0x01;
      gran_block_type[i] = ty_seq[gran_block_type[i-1]][tmp];
    }
  }
}


void aacAUDecode ( int numChannels,
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
  /* son_AACpp */
  HANDLE_BSBITSTREAM  gc_stream[MAX_TIME_CHANNELS];
  HANDLE_BSBITSTREAM  gc_WRstream[MAX_TIME_CHANNELS];
  HANDLE_BSBITBUFFER  gcBitBuf[MAX_TIME_CHANNELS];     /* bit buffer for gain_control_data() */
  
  int short_win_in_long = 8;
  
  byte max_sfb[Winds];
  byte dummy[MAX_TIME_CHANNELS][MAXBANDS];
  QC_MOD_SELECT qc_select;

  switch (frameData->scalOutObjectType) {
      case ER_AAC_LD:
#ifdef AAC_ELD
      case ER_AAC_ELD:
#endif
          qc_select = AAC_LD;
          break;
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

  switch( (AUDIO_OBJECT_TYPE_ID)frameData->scalOutObjectType ) {
  case ER_AAC_LC:
  case ER_AAC_LTP:
  case ER_AAC_LD:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
    if ( GetEPFlag( hFault[0].hResilience ) ) {
      BsPrepareEp1Stream( hFault->hEscInstanceData,
                          frameData->layer,
                          tfData->output_select + 1 );
    }
    break;
  default:
    break;
  }

  fixed_stream = BsOpenBufferRead (frameData->layer[0].bitBuf);


  /* buffers for SSR  */

  for (i_ch = 0 ; i_ch < numChannels ; i_ch++)
  {
    gcBitBuf[i_ch] = BsAllocBuffer(4096);
    gc_WRstream[i_ch] = BsOpenBufferWrite(gcBitBuf[i_ch]);
    gc_stream[i_ch] = BsOpenBufferRead(gcBitBuf[i_ch]);
  } /* for i_ch...*/
 
  /* inverse Q&C */

  decoded_bits += AACDecodeFrame ( hDec,
                                   fixed_stream,
                                   gc_WRstream,
                                   tfData->spectral_line_vector[0], 
                                   tfData->windowSequence, 
                                   tfData->windowShape,
                                   bitStreamType,
                                   max_sfb,
                                   numChannels,
                                   0,/*commonWindow*/
                                   tfData->sfbInfo,   
                                   dummy,/*sfbCbMap*/
                                   pFactors, /*pFactors */
                                   hFault,
                                   qc_select,
                                   tfData->nok_lt_status,
                                   frameData, 
                                   0,  
                                   0,  
                                   0
                                   );


  if (mc_info.profile  == SSR_Profile) {

    /*  
        check if SSR Profile is allowed
    */
    
    if(!( hFault->hResilience  && GetEPFlag ( hFault[0].hResilience )) ||
       !( hFault->hEscInstanceData || BsGetEpDebugLevel ( hFault->hEscInstanceData ) >= 2 ))
      CommonExit ( 2, "aacAUDecode: ssr not allowed in this profile");
    
    
    /* unpack gain_control_data() */
    for(i_ch=0;i_ch<numChannels;i_ch++){
      if (BsBufferNumBit(gcBitBuf[i_ch]) > 0) {
        /* decoded_bits = */
        son_gc_unpack(gc_stream[i_ch], tfData->windowSequence[i_ch],
                      &hDec->max_band[i_ch], hDec->gainInfo[i_ch]);
      }
    }
  }
  
  
  if (mc_info.profile  == SSR_Profile) {
    int band;
    int i_ch; 
    for(i_ch=0;i_ch<numChannels;i_ch++){
      
      son_gc_arrangeSpecDec( tfData->spectral_line_vector[0][i_ch],
                             tfData->block_size_samples,
                             tfData->windowSequence[i_ch],
                             hDec->spectral_line_vector_for_gc[i_ch] );
      
      for (band = 0; band < NBANDS; band++) {
        freq2buffer(  &hDec->spectral_line_vector_for_gc[i_ch][tfData->block_size_samples/NBANDS*band],
                      &hDec->imdctBufForGC[i_ch][tfData->block_size_samples/NBANDS*2*band],
                      &tfData->overlap_buffer[i_ch][tfData->block_size_samples/NBANDS*band],
                      tfData->windowSequence[i_ch],
                      tfData->block_size_samples/NBANDS,
                      tfData->block_size_samples/NBANDS/short_win_in_long, 
                      tfData->windowShape[i_ch],
                      tfData->prev_windowShape[i_ch], /* YB : 971113 */
                      NON_OVERLAPPED_MODE,
                      short_win_in_long );
      }
      /* gain compensator */
      son_gc_compensate( hDec->imdctBufForGC[i_ch],
                         hDec->gainInfo[i_ch],
                         tfData->block_size_samples,
                         tfData->windowSequence[i_ch],
                         i_ch,
                         hDec->DBandSigBufForOverlapping[i_ch],
                         hDec->DBandSigBuf[i_ch],
                         NBANDS );
      /* ipqf */
      son_ipqf_main( hDec->DBandSigBuf[i_ch],
                     tfData->block_size_samples,
                     i_ch,
                     tfData->time_sample_vector[i_ch] );
      tfData->prev_windowShape[i_ch] = tfData->windowShape[i_ch];
    } /* for i_ch ...*/
  }
  
  removeAU(fixed_stream,decoded_bits,frameData,0);
  BsCloseRemove(fixed_stream,1);
  
  for (i_ch=0; i_ch<numChannels; i_ch++){
    BsCloseRemove(gc_stream[i_ch],1);
    BsCloseRemove(gc_WRstream[i_ch],1);
    BsFreeBuffer(gcBitBuf[i_ch]);
  }/* for(i_ch...*/

}


/* Module Functions for TNSDATA */


TNS_frame_info GetTnsData( void *paacDec, int index )
{
  return *(( (T_AACDECODER *) paacDec )->tns)[index];
}

int GetTnsDataPresent( void *paacDec, int index )
{
  return ( (T_AACDECODER *) paacDec )->tns_data_present[index];
}

void SetTnsDataPresent( void *paacDec, int index, int value )
{
  ( (T_AACDECODER *) paacDec )->tns_data_present[index] = value;
}

int GetTnsExtPresent( void *paacDec, int index )
{
  return ( (T_AACDECODER *) paacDec )->tns_ext_present[index];
}

void SetTnsExtPresent( void *paacDec, int index, int value )
{
 ( (T_AACDECODER *) paacDec )-> tns_ext_present[index] = value;
}


/* Module Functions for BLOCKSIZE */


int GetBlockSize( void *paacDec )
{
  return ( (T_AACDECODER *) paacDec )->blockSize;
}


#ifdef CT_SBR

/* Module Functions for SBR Decoder */

SBRBITSTREAM *getSbrBitstream( void *paacDec ) {

#ifndef SBR_SCALABLE
  return &( (T_AACDECODER *) paacDec )->ct_sbrBitStr;
#else
  return ( (T_AACDECODER *) paacDec )->ct_sbrBitStr;
#endif
}

void setMaxSfb ( void *paacDec, int maxSfb )
{
  if ( paacDec != NULL ) {
    ( (T_AACDECODER *) paacDec )->maxSfb = maxSfb;
  }
}

int getMaxSfbFreqLine( void *paacDec ) 
{

  int maxSfb = (((T_AACDECODER *)paacDec)->maxSfb -1);

  if ( maxSfb > 0 ) {

    /* get longblock flag */
    int isLongBlock = ((T_AACDECODER *)paacDec)->info->islong;

    if ( isLongBlock ) { 
      /* longblock */
      return ((T_AACDECODER *)paacDec )->info->bk_sfb_top[maxSfb-1];
    }
    else {
      /* shortblock */
      return ((T_AACDECODER *)paacDec )->info->bk_sfb_top[maxSfb-1] <<3;
    }
  }
  else 
    /* if maxSfb=0 no lines are in the freq. spectrum */
    return 0;
}

#endif

