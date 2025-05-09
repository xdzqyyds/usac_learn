/*******************************************************************************
This software module was originally developed by

This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS, and VoiceAge Corp.

Initial author:
Yoshiaki Oikawa     (Sony Corporation),
Mitsuyuki Hatanaka  (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani  (Fraunhofer Gesellschaft IIS)
Markus Werner       (SEED / Software Development Karlsruhe)

and edited by:
Jeremie Lecomte     (Fraunhofer IIS)
Markus Multrus      (Fraunhofer IIS)
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
$Id: usac_fd_dec.c,v 1.16.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "usac_all.h"                 /* structs */
#include "usac_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "usac_allVariables.h"        /* variables */

#include "huffdec2.h"
#include "huffdec3.h"
#include "usac_port.h"
#include "common_m4a.h"
#include "resilience.h"
#include "concealment.h"
#include "reorderspec.h"
#include "bitfct.h"
#include "buffers.h"
#include "bitstream.h"
#include "interface.h"

#include "concealment.h"
#include "rvlcScfResil.h"
#include "huffScfResil.h"

#include "dec_usac.h"
#include "aac.h"

#include "usac_arith_dec.h"
#include "proto_func.h"


#define MAX_NR_OF_SWB       120

typedef struct {
  unsigned short nrOfSwb;
  unsigned short swbReal;
} ENTRY_POINT;


static void PrintSpectralValues ( int *quant )
{
  unsigned short i;
  for ( i = 0; i < LN2; i++ )
    printf ( "bno %13li line %4i value %13i\n", bno, i, quant[i] );
}

void calc_gsfb_table ( Info *info,
                       byte *group )
{
  int group_offset;
  int group_idx;
  int offset;
  short * group_offset_p;
  int sfb,len;
  /* first calc the group length*/
  if (info->islong){
    return;
  } else {
    group_offset = 0;
    group_idx = 0;
    do  {
      info->group_len[group_idx] = group[group_idx] - group_offset;
      group_offset=group[group_idx];
      group_idx++;
    } while (group_offset<8);
    info->num_groups = group_idx;
    group_offset_p = info->bk_sfb_top;
    offset=0;
    for ( group_idx = 0; group_idx < info->num_groups; group_idx++) {
      len = info->group_len[group_idx];
      for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++){
        offset += info->sfb_width_short[sfb] * len;
        *group_offset_p++ = offset;
      }
    }
  }
}


void usac_get_tns ( Info*             info,
                    TNS_frame_info*   tns_frame_info,
                    HANDLE_RESILIENCE hResilience,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_BUFFER     hVm )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static short              sgn_mask[] = {     0x2, 0x4, 0x8     };
  static short              neg_mask[] = {     (short) 0xfffc, (short)0xfff8, (short)0xfff0     };

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];

    if (!(tns_info->n_filt = GetBits ( short_flag ? 1 : 2,
                                       N_FILT,
                                       hResilience,
                                       hEscInstanceData,
                                       hVm )))
      continue;

    tns_info -> coef_res = res = GetBits ( 1,
                                           COEF_RES,
                                           hResilience,
                                           hEscInstanceData,
                                           hVm ) + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      top = tns_filt->start_band = top - GetBits ( short_flag ? 4 : 6,
                                                   LENGTH,
                                                   hResilience,
                                                   hEscInstanceData,
                                                   hVm );
      tns_filt->order = GetBits ( short_flag ? 3 : 4,
                                  ORDER,
                                  hResilience,
                                  hEscInstanceData,
                                  hVm );

      if (tns_filt->order)  {
        tns_filt->direction = GetBits ( 1,
                                        DIRECTION,
                                        hResilience,
                                        hEscInstanceData,
                                        hVm );
        compress = GetBits ( 1,
                             COEF_COMPRESS,
                             hResilience,
                             hEscInstanceData,
                             hVm );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = (short) GetBits ( res2,
                                  COEF,
                                  hResilience,
                                  hEscInstanceData,
                                  hVm );
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
}

static int scfcb ( byte*                    sect,
                   Info                     info,
                   int                      tot_sfb,
                   int                      sfb_per_sbk,
                   byte                     max_sfb,
                   unsigned char            numGroups)
{
  int            nsect;
  int            nrSfb;
  unsigned char  group = 0;
  unsigned char  currentMaxSfb;
  unsigned short vcb11Flag;
  unsigned short nrOfCodewords = 0;

  if (debug['s']) {
    fprintf(stderr, "total sfb %d\n", tot_sfb);
    fprintf(stderr, "   sect    top     cb\n");
  }
  nsect = 0;
  nrSfb = 0;

  while ( ( nrSfb < tot_sfb ) && ( nsect < tot_sfb ) ) {
    currentMaxSfb = sfb_per_sbk * group + max_sfb;
    vcb11Flag = 0; /* virtual codebooks flag is false */
    *sect = 11;
    sect++;
    *sect = nrSfb = currentMaxSfb;
    sect++;
    nsect++;
    /* insert a zero section for regions above max_sfb for each group */
    if ( sect[-1] == currentMaxSfb ) {           /* changed from ((sect[-1] % sfb_per_sbk) == max_sfb) */
      nrSfb = ( sfb_per_sbk * ( group + 1 ) );    /* changed from nrSfb += (sfb_per_sbk - max_sfb) */
      group++;
      if ( currentMaxSfb != sfb_per_sbk * group ) { /* this inserts a section with codebook 0 */
        *sect++ = 0;
        *sect++ = nrSfb;
        nsect++;

        if (debug['s'])
          fprintf(stderr, "(%6d %6d %6d)\n", nsect, sect[-1], sect[-2]);
      }
    }
  }
  return ( nsect );
}

void get_tw_data(int                     *tw_data_present,
                 int                     *tw_ratio,
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                 HANDLE_BUFFER            hVm
) {

  *tw_data_present = (int) GetBits ( LEN_TW_PRES,
                                     TW_DATA_PRESENT,
                                     hResilience,
                                     hEscInstanceData,
                                     hVm );

  if ( *tw_data_present ) {
    int i;
    for ( i = 0 ; i < NUM_TW_NODES ; i++ ) {
      tw_ratio[i] = (int) GetBits( LEN_TW_RATIO,
                                   TW_RATIO,
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );
    }

  }

}

WINDOW_SEQUENCE 
usacMapWindowSequences(WINDOW_SEQUENCE windowSequenceCurr, WINDOW_SEQUENCE windowSequenceLast){

  WINDOW_SEQUENCE windowSequence; 

  switch( windowSequenceCurr ) {
  case ONLY_LONG_SEQUENCE :
    windowSequence = ONLY_LONG_SEQUENCE;
    break;

  case LONG_START_SEQUENCE :
    if ( (windowSequenceLast == LONG_START_SEQUENCE)  || 
         (windowSequenceLast == EIGHT_SHORT_SEQUENCE) || 
         (windowSequenceLast == STOP_START_SEQUENCE) ){/* Stop Start Window*/
      windowSequence = STOP_START_SEQUENCE;
    } else { /*Start Window*/
      windowSequence = LONG_START_SEQUENCE;
    }
    break;
    
  case LONG_STOP_SEQUENCE :
    windowSequence = LONG_STOP_SEQUENCE;
    break;
    
  case EIGHT_SHORT_SEQUENCE :
    windowSequence = EIGHT_SHORT_SEQUENCE;
    break;
    
  default :
    CommonExit( 1, "Unknown window type" );
  }

  return windowSequence;
}





int usac_get_fdcs ( HANDLE_USAC_DECODER      hDec,
                    Info*                    info,
                    int                      common_window,
                    int                      common_tw,
                    int                      tns_data_present,
                    WINDOW_SEQUENCE*         win,
                    WINDOW_SHAPE*            wshape,
                    byte*                    group,
                    byte*                    max_sfb,
                    byte*                    cb_map,
                    float*                   coef,
                    int                      max_spec_coefficients,
                    short*                   global_gain,
                    short*                   factors,
                    int                     *arithPreviousSize,
                    Tqi2                     arithQ[],
                    TNS_frame_info*          tns,
                    int                     *tw_data_present,
                    int                     *tw_ratio,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_BUFFER            hHcrSpecData,
                    HANDLE_HCR               hHcrInfo,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_CONCEALMENT       hConcealment,
                    QC_MOD_SELECT            qc_select,
                    unsigned int             *nfSeed,
                    HANDLE_BUFFER            hVm,
                    WINDOW_SEQUENCE          windowSequenceLast,
                    int                      frameWasTD,
                    int                      bUsacIndependencyFlag,
                    int 		                 ch,
                    int                      bUseNoiseFilling

)

{
  int            nsect;
  int            i;
  int            cb;
  int            top;
  int            bot;
  int            tot_sfb;
  int            read_FAC_bits=0;
  byte           sect[ 2*(MAXBANDS+1) ];
  int            noise_level=0;
  int                 reset;
  WINDOW_SEQUENCE   windowSequence;
  int            arithSize;

  windowSequence=ONLY_LONG_SEQUENCE;
  /*
   * global gain
   */
  *global_gain = (short) GetBits ( 8,
                                   GLOBAL_GAIN,
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );

  if (debug['f'])
    printf("global gain: %3d\n", *global_gain);

  /*
   * Noise Filling
   */
  if(bUseNoiseFilling){
    noise_level = (short) GetBits ( 8,
				    NOISE_LEVEL,
				    hResilience,
				    hEscInstanceData,
				    hVm );
  }
  else{
    noise_level = 0;
  }

  memcpy(info, usac_winmap[*win], sizeof(Info));

  if ( !common_window ) {
    usac_get_ics_info ( win,
                        wshape,
                        group,
                        max_sfb,
                        qc_select,
                        hResilience,
                        hEscInstanceData,
                        hVm );

    *win = usacMapWindowSequences(*win, windowSequenceLast);

    memcpy(info, usac_winmap[*win], sizeof(Info));
  }

  if ( !common_tw && hDec->twMdct[0] == 1) {    
    get_tw_data(tw_data_present,
                tw_ratio,
                hResilience,
                hEscInstanceData,
                hVm );
  }

  /* calculate total number of sfb for this grouping */
  if (*max_sfb == 0) {
    tot_sfb = 0;
  }
  else {
    i=0;
    tot_sfb = info->sfb_per_sbk[0];
    if (debug['f'])printf("tot sfb %d %d\n", i, tot_sfb);
    while (group[i++] < info->nsbk) {
      tot_sfb += info->sfb_per_sbk[0];
      if (debug['f'])printf("tot sfb %d %d\n", i, tot_sfb);
    }
  }

  /* calculate band offsets
   * (because of grouping and interleaving this cannot be
   * a constant: store it in info.bk_sfb_top)
   */
  calc_gsfb_table(info, group);

  /* set correct sections */

  nsect = scfcb(sect,
                *info,
                tot_sfb,
                info->sfb_per_sbk[0],
                *max_sfb,
                info->num_groups);

  /* generate "linear" description from section info
   * stored as codebook for each scalefactor band and group
   */
  if (nsect) {
    bot = 0;
    for (i=0; i<nsect; i++) {
      cb = sect[2*i];
      top = sect[2*i + 1];
      for (; bot<top; bot++)
        *cb_map++ = cb;
      bot = top;
    }
  }
  else {
    for (i=0; i<MAXBANDS; i++)
      cb_map[i] = 0;
  }

  /*
   * scale factor data
   */
  /* normal AAC Mode */
  if(!hufffac( info,
               group,
               nsect,
               sect,
               *global_gain,
               1,
               factors,
               hResilience,
               hEscInstanceData,
               hVm ) ) {
    if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
      printf("getics: hufffac returns zero (huffdec2.c, getics())\n");
    }
    return 0;
  }

  /*
   * tns data
   */

  if(tns_data_present == 0)
    clr_tns ( info, tns );


  if(tns_data_present == 1)
    usac_get_tns ( info, tns, hResilience, hEscInstanceData,hVm );
  if (tns_data_present&&debug['V']) fprintf(stderr,"# TNS detected\n");

  if(*max_sfb > 0){
    max_spec_coefficients = info->bk_sfb_top[*max_sfb-1]/info->group_len[0];
  } else {
    max_spec_coefficients = 0;
  }

  if(!bUsacIndependencyFlag){
    reset = GetBits (LEN_RESET_ARIT_DEC,
                     RESET_ARIT_DEC,
                     hResilience,
                     hEscInstanceData,
                     hVm );
  } else {
    reset = 1;
  }
  

  switch(*win){
  case EIGHT_SHORT_SEQUENCE:
    arithSize = hDec->blockSize/8;
    break;
  default:
    arithSize = hDec->blockSize;
    break;
  }
  
  acSpecFrame(hDec,
              info,
              coef,
              max_spec_coefficients,
              hDec->blockSize,
              noise_level,
              factors,
              arithSize,
              arithPreviousSize,
              arithQ,
              hVm,
              *max_sfb,
              reset,
              nfSeed,
              bUseNoiseFilling);

  /* read fac_data_present */
  hDec->ApplyFAC[ch] = (short) GetBits ( LEN_FAC_DATA_PRESENT,
                                         NOISE_LEVEL,
                                         hResilience,
                                         hEscInstanceData,
                                         hVm );
  
  if ( hDec->ApplyFAC[ch] ) {
    int lFac;
    if ( (*win)  == EIGHT_SHORT_SEQUENCE ) {
      lFac = (hDec->blockSize)/16;
    }
    else {
      lFac = (hDec->blockSize)/8;
    }
    read_FAC_bits=ReadFacData(lFac, hDec->facData[ch],hResilience,hEscInstanceData,hVm);
  }
  
  return 1 ;
}

int ReadFacData(int lfac,
                int                      *facData,
                HANDLE_RESILIENCE         hResilience,
                HANDLE_ESC_INSTANCE_DATA  hEscInstanceData,
                HANDLE_BUFFER             hVm
) {
  int ReadBits = 0;

  /* read gain */
  facData[0] = GetBits(7,
                       FAC_GAIN,
                       hResilience,
                       hEscInstanceData,
                       hVm);

  ReadBits+=7;

  /* read FAC data */
  ReadBits+=fac_decoding(lfac,
                         0,
                         &facData[1],
                         hResilience,
                         hEscInstanceData,
                         hVm);


  return ReadBits;
}
