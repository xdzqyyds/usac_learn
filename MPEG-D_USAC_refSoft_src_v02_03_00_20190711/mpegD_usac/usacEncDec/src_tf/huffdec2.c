/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by

 AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
  
 and edited by
 Yoshiaki Oikawa     (Sony Corporation),
 Mitsuyuki Hatanaka  (Sony Corporation),
 Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
 Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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
 
 $Id: huffdec2.c,v 1.5 2011-04-12 16:11:53 mul Exp $
 AAC Decoder module
**********************************************************************/


#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "huffdec2.h"
#include "huffdec3.h"
#include "port.h"
#include "sony_local.h" /* 971117 YT */
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

#include "dec_tf.h"
#include "aac.h"

#define MAX_NR_OF_SWB       120

TNS_frame_info GetTnsData( void *paacDec, int index );
int  GetTnsDataPresent( void *paacDec, int index );
void SetTnsDataPresent( void *paacDec, int index, int value );
int  GetTnsExtPresent(  void *paacDec, int index );
void SetTnsExtPresent(  void *paacDec, int index, int value );

typedef struct {
  unsigned short nrOfSwb;
  unsigned short swbReal;
} ENTRY_POINT;



static const struct {
    unsigned short min;
    unsigned short max;
  } preCodebookSorting[] = {{ 11, 11},
                            { 31, 31},
                            { 30, 30},
                            { 29, 29},
                            { 28, 28},
                            { 27, 27},
                            { 26, 26},
                            { 25, 25},
                            { 24, 24},
                            { 23, 23},
                            { 22, 22},
                            { 21, 21},
                            { 20, 20},
                            { 19, 19},
                            { 18, 18},
                            { 17, 17},
                            { 16, 16},
                            {  9, 10},
                            {  7,  8},
                            {  5,  6},
                            {  3,  4},
                            {  1,  2}};



static void PrintSpectralValues ( int *quant )
{
  unsigned short i;
  for ( i = 0; i < LN2; i++ )
    printf ( "bno %13li line %4i value %13i\n", bno, i, quant[i] );
}

unsigned char Vcb11Used( unsigned short codebook ) 
{
  if( (codebook == 11) || ((codebook >= 16) && (codebook <= 31)) ) {
    return ( 1 );
  }
  else {
    return ( 0 );
  }
}

void deinterleave ( void                    *inptr,  /* formerly pointer to type int */
                    void                    *outptr, /* formerly pointer to type int */
                    short                   length,  /* sizeof base type of inptr and outptr in chars */
                    const short             nsubgroups[], 
                    const int               ncells[], 
                    const short             cellsize[],
                    int                     nsbk,
                    int                     blockSize,
                    const HANDLE_RESILIENCE hResilience,
                    short                   ngroups )
{
  char *inptr1  = (char*)inptr;
  char *outptr1 = (char*)outptr;

  if ( GetReorderSpecFlag( hResilience ) )
    {
      unsigned short    unitNrInSpectralDirection;
      unsigned short    unitsInSpectralDirection;   /* number of units in spectral direction */
      unsigned short    windowNr;
      unsigned short    windows;
      unsigned short    lineNrPerUnit;
      unsigned short    linesPerUnit = 4;
      unsigned short    linesPerWindow;
      unsigned short    inLine = 0;

      
      windows = nsbk;
      linesPerWindow = blockSize / windows;
      unitsInSpectralDirection = linesPerWindow / linesPerUnit;

      for ( unitNrInSpectralDirection = 0; 
            unitNrInSpectralDirection < unitsInSpectralDirection;
            unitNrInSpectralDirection ++ )         /* for each unit of a window within the spectrum */
        for ( windowNr = 0; windowNr < windows; windowNr++ )         /* for each window within a spectrum part (4 lines) */
          for ( lineNrPerUnit = 0; lineNrPerUnit < linesPerUnit; lineNrPerUnit++ ) /* for each line within a unit */
            {
              memcpy( outptr1 + length*(unitNrInSpectralDirection * linesPerUnit + windowNr * linesPerUnit * unitsInSpectralDirection + lineNrPerUnit) ,
                      inptr1  + length*inLine,
                      length);
              inLine++;
            }
    }
  else
    {
      int i, j, k, l;
      char *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
      short cell_inc, subgroup_inc;

      start_subgroup_ptr = outptr1;

      for (i = 0; i < ngroups; i++)
        {
          cell_inc = 0;
          start_inptr = inptr1;

          /* Compute the increment size for the subgroup pointer */

          subgroup_inc = 0;
          for (j = 0; j < ncells[i]; j++) {
            subgroup_inc += cellsize[j];
          }

          /* Perform the deinterleaving across all subgroups in a group */

          for (j = 0; j < ncells[i]; j++) {
            subgroup_ptr = start_subgroup_ptr;
            for (k = 0; k < nsubgroups[i]; k++) {
              outptr1 = subgroup_ptr + length*cell_inc;
              for (l = 0; l < cellsize[j]; l++) {
                memcpy( outptr1, inptr1, length);
                outptr1 += length;
                inptr1  += length;
              }
              subgroup_ptr += length*subgroup_inc;
            }
            cell_inc += cellsize[j];
          }
          start_subgroup_ptr += (inptr1 - start_inptr);
        }
    }
}

static void calc_gsfb_table ( Info *info, 
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

void getgroup ( Info*             info, 
                byte*             group, 
                HANDLE_RESILIENCE hResilience,
                HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                HANDLE_BUFFER     hVm )
{
  int i, j, first_short;

  if( debug['g'] ) fprintf(stderr, "Grouping: 0");
  first_short = 1;
  for (i = 0; i < info->nsbk; i++) {
    if (info->bins_per_sbk[i] > SN2) {
      /* non-short windows are always their own group */
      *group++ = i+1;
    }
    else {
      /* only short-window sequences are grouped! */
      if (first_short) {
        /* first short window is always a new group */
        first_short=0;
      }
      else {
        if((j = GetBits ( 1,
                          SCALE_FACTOR_GROUPING, 
                          hResilience, 
                          hEscInstanceData,
                          hVm )) == 0) {
          *group++ = i;
        }
        if( debug['g'] ) fprintf(stderr, "%1d", j);
      }
    }
  }
  *group = i;
  if( debug['g'] ) fprintf(stderr, "\n");
}

/*
 * read a synthesis mask
 *  uses EXTENDED_MS_MASK
 *  and grouped mask 
 */
int getmask ( Info*             info, 
              byte*             group, 
              byte              max_sfb, 
              byte*             mask, 
              HANDLE_RESILIENCE hResilience, 
              HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
              HANDLE_BUFFER     hVm )
{
  int b, i, mp;

  mp = GetBits ( LEN_MASK_PRES,
                 MS_MASK_PRESENT, 
                 hResilience, 
                 hEscInstanceData,
                 hVm );
  if( debug['m'] )
    printf("\nExt. Mask Present: %d\n",mp);

  /* special EXTENDED_MS_MASK cases */
  if(mp == 0) { /* no ms at all */
    return 0;
  }
  if (debug['V']) fprintf(stderr,"# MS detected\n");
  if(mp == 2) {/* MS for whole spectrum on, mask bits set to 1 */
    for(b = 0; b < info->nsbk; b = *group++)
      for(i = 0; i < info->sfb_per_sbk[b]; i ++)
        *mask++ = 1;
    return 2;
  }

  if (mp == 3) {
    return 3;
  }

  /* otherwise get mask */
  for(b = 0; b < info->nsbk; b = *group++){
    if( debug['m'] ) printf(" gr%1i:",b);
    for(i = 0; i < max_sfb; i ++) {
      *mask = (unsigned char) GetBits ( LEN_MASK,
                                        MS_USED, 
                                        hResilience, 
                                        hEscInstanceData,
                                        hVm );
      if( debug['m'] )printf("%1i",*mask);
      mask++;
    }
    for( ; i < info->sfb_per_sbk[b]; i++){
      *mask = 0;
      if( debug['m'] ) printf("%1i",*mask);
      mask++;
    }
  }
  if( debug['m'] ) printf("\n");
  return 1;
}

void clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int s;

  tns_frame_info->n_subblocks = info->nsbk;
  for (s=0; s<tns_frame_info->n_subblocks; s++)
    tns_frame_info->info[s].n_filt = 0;
}

static void get_tns ( Info*             info, 
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
      tns_filt->order = GetBits ( short_flag ? 3 : 5,
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

/*
 * pulse noiseless coding
 */
struct Pulse_Info
{
  int pulse_data_present;
  int number_pulse;
  int pulse_start_sfb;
  int pulse_position[NUM_PULSE_LINES];
  int pulse_offset[NUM_PULSE_LINES];
  int pulse_amp[NUM_PULSE_LINES];
};
static struct Pulse_Info pulseInfo;

static void GetPulseNc ( struct Pulse_Info* pulseInfo,
                         HANDLE_RESILIENCE  hResilience,
                         HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                         HANDLE_BUFFER      hVm )
{
  int i;
  pulseInfo->number_pulse = GetBits ( LEN_PULSE_NPULSE,
                                      NUMBER_PULSE,
                                      hResilience, 
                                      hEscInstanceData,
                                      hVm );
  pulseInfo->pulse_start_sfb = GetBits ( LEN_PULSE_ST_SFB,
                                         PULSE_START_SFB, 
                                         hResilience, 
                                         hEscInstanceData,
                                         hVm );
  for(i=0; i<pulseInfo->number_pulse + 1; i++) {
    pulseInfo->pulse_offset[i] = GetBits ( LEN_PULSE_POFF,
                                           PULSE_OFFSET, 
                                           hResilience, 
                                           hEscInstanceData,
                                           hVm );
    pulseInfo->pulse_amp[i] = GetBits ( LEN_PULSE_PAMP,
                                        PULSE_AMP, 
                                        hResilience, 
                                        hEscInstanceData,
                                        hVm );
  }
}

static void pulse_nc ( int*               coef, 
                       struct Pulse_Info* pulse_info
                       ,HANDLE_RESILIENCE  hResilience,
                       HANDLE_ESC_INSTANCE_DATA     hEscInstanceData
 )
{
  int i, k;
  
  if (pulse_info->pulse_start_sfb>0){
    k = only_long_info.sbk_sfb_top[0][pulse_info->pulse_start_sfb-1];
  }
  else{
    k = 0;
  }
  for(i=0; i<=pulse_info->number_pulse; i++) {
    k += pulse_info->pulse_offset[i];
    if ( k < LN2) {
      if (coef[k]>0) {
        coef[k] += pulse_info->pulse_amp[i];
      }
      else {
        coef[k] -= pulse_info->pulse_amp[i];
      }
    }
    else {
      if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
        CommonExit( 2, "pulse_nc: pulse data cannot be processed on line %i (huffdec2.c)", k );
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("pulse_nc: pulse data cannot be processed on line %i\n", k );
        }
      }
    } 
  }
}

static long get_gcBuf ( WINDOW_SEQUENCE          window_sequence, 
                 HANDLE_BSBITSTREAM       gc_streamCh,
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                 HANDLE_BUFFER            hVm )
{
  unsigned int bd;
  int wd;
  unsigned int ad;
  unsigned long   max_band, natks, ltmp;
  int loc;

  loc = BsCurrentBit(gc_streamCh);
  max_band = GetBits ( NBANDSBITS,
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience, 
                       hEscInstanceData,
                       hVm );
  BsPutBit(gc_streamCh, max_band, NBANDSBITS);/*   0 < max_band <= 3 */

  switch (window_sequence) {
  case ONLY_LONG_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 1; wd++) {
        natks = GetBits ( NATKSBITS,
                          MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                          hResilience, 
                          hEscInstanceData,
                          hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits ( ATKLOCBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS);
        }
      }
    }
    break;
  case LONG_START_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits ( NATKSBITS,
                          MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                          hResilience, 
                          hEscInstanceData,
                          hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ATKLOCBITS_START_A,
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_A);
          }
          else {
            ltmp = GetBits ( ATKLOCBITS_START_B,
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_START_B);
          }
        }
      }
    }
    break;
  case EIGHT_SHORT_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 8; wd++) {
        natks = GetBits (  NATKSBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          ltmp = GetBits (  ATKLOCBITS_SHORT,
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience, 
                            hEscInstanceData,
                            hVm );
          BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_SHORT);
        }
      }
    }
    break;
  case LONG_STOP_SEQUENCE:
    for (bd = 1; bd <= max_band; bd++) {
      for (wd = 0; wd < 2; wd++) {
        natks = GetBits (  NATKSBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
        BsPutBit(gc_streamCh, natks, NATKSBITS);
        for (ad = 0; ad < natks; ad++) {
          ltmp = GetBits ( IDGAINBITS,
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience, 
                           hEscInstanceData,
                           hVm );
          BsPutBit(gc_streamCh, ltmp, IDGAINBITS);
          if (wd == 0) {
            ltmp = GetBits ( ATKLOCBITS_STOP_A,
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_A);
          }
          else {
            ltmp = GetBits ( ATKLOCBITS_STOP_B,
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEscInstanceData,
                             hVm );
            BsPutBit(gc_streamCh, ltmp, ATKLOCBITS_STOP_B);
          }
        }
      }
    }
    break;
  default:
    return  -1;
  }
  return BsCurrentBit(gc_streamCh) - loc;
}

/*
 * read the codebook and boundaries
 */
static int huffcb ( byte*                    sect, 
                    Info                     info,
                    int*                     sectbits, 
                    int                      tot_sfb, 
                    int                      sfb_per_sbk, 
                    byte                     max_sfb,
                    unsigned char            numGroups,
                    HANDLE_HCR               hHcrInfo,
                    HANDLE_RESILIENCE        hResilience,
                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                    HANDLE_CONCEALMENT       hConcealment,
                    HANDLE_BUFFER            hVm)
{
  int            nsect;
  int            n;
  int            nrSfb;
  int            bits;
  int            len;
  unsigned char  group = 0;
  unsigned char  currentMaxSfb;
  unsigned short vcb11Flag;
  unsigned short nrOfCodewords = 0;
  unsigned short dim;

  if (debug['s']) {
    fprintf(stderr, "total sfb %d\n", tot_sfb);
    fprintf(stderr, "   sect    top     cb\n");
  }
  bits = sectbits[0];
  len = (1 << bits) - 1;
  nsect = 0;
  nrSfb = 0;
  
  while ( ( nrSfb < tot_sfb ) && ( nsect < tot_sfb ) ) {
    currentMaxSfb = sfb_per_sbk * group + max_sfb;
    vcb11Flag = 0; /* virtual codebooks flag is false */
    if(GetVcb11Flag( hResilience )) { /* virtual codebooks used */
      *sect = (unsigned char) GetBits ( LEN_CB_VCB11, 
                                        SECT_CB, 
                                        hResilience,  
                                        hEscInstanceData, 
                                        hVm ); 
    }
    else { /* section data transcoding off */
      *sect = (unsigned char) GetBits ( LEN_CB,
                                        SECT_CB, 
                                        hResilience, 
                                        hEscInstanceData,
                                        hVm );
      
    }
    ConcealmentDetectErrorCodebook(sect, hConcealment);
    
    if ( GetVcb11Flag ( hResilience ) ) {         /* virtual codebooks used */
      if( Vcb11Used( *sect ) ) {
        vcb11Flag = 1;                            /* virtual codebooks flag is set */
      }
      
    }
    if( vcb11Flag ) {
      n = 1;                                     /* virtual codebooks are not combined */
    }
    else {                                       /* for non virtual codebooks read the number of occurrences */
      n = GetBits ( bits,
                    SECT_LEN_INCR, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );
    }
    if ( n == 0 ) {
      ConcealmentDetectErrorSectionData(SECTIONLENGTH__EQ__ZERO, hConcealment);
      if ( ! hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("huffcb: a section length of 0 (huffdec2.c)\n");
      }
    }
    while ( n == len && nrSfb < currentMaxSfb ) { /* changed from < tot_sfb */
      nrSfb += len;
      n = GetBits ( bits,
                    SECT_LEN_INCR, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );
    }
    nrSfb += n;
    sect++;
    *sect = nrSfb;
    sect++;
    /**/
    if ( sect[-1] > currentMaxSfb ) {
      if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
        CommonExit( 2, "huffcb: group %i: nrOfScfBands (%2i) > currentMaxSfb (%2i) (huffdec2.c)",
                    group, sect[-1], currentMaxSfb );
      }
      else {
        ConcealmentDetectErrorSectionData(NR_OF_SCF_BANDS__GT__CURRENT_MAX_SFB, hConcealment);
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("huffcb: group %i: nrOfScfBands (%2i) > currentMaxSfb (%2i)\n", group, sect[-1], currentMaxSfb );
          printf("huffcb: --> nrOfScfBands has been clipped at %2i\n", currentMaxSfb );
        }
        sect[-1] = currentMaxSfb;
      }
    }
    /* derivation of nrOfCodewords for HCR_SQUARE */
    if ( ! ( ( sect[-2] == ZERO_HCB ) ||
#ifdef PNS
             ( sect[-2] == NOISE_HCB ) ||
#endif
             ( sect[-2] == INTENSITY_HCB ) ||
             ( sect[-2] == INTENSITY_HCB2 ) ||
             ( sect[-2] == BOOKSCL ) ) ) {
      if( sect[-2] < BY4BOOKS + 1 ) {
        dim = 4;
      }
      else {
        dim = 2;
      }
      if ( sect[-2] ) {
        if ( ! nsect ) {
          nrOfCodewords = (info.bk_sfb_top[sect[-1]-1] ) / dim;
        }
        else {
          nrOfCodewords += ( info.bk_sfb_top[sect[-1]-1] - info.bk_sfb_top[sect[-3]-1] ) / dim;
        }
      }
    }
    /**/
    nsect++;
    if (debug['s'] ) {
      fprintf(stderr, " %6d %6d %6d %6d %6d\n", 
             nsect, 
             sect[-1], 
             sect[-2], 
             info.bk_sfb_top[sect[-1]-1], 
             nrOfCodewords );
    }
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
  if ( nrSfb != tot_sfb ) {
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "huffcb: nrSfb (%3hu) != totSfb (%3hu)", nrSfb, tot_sfb );
    }
    else {
      ConcealmentDetectErrorSectionData(NR_SFB__NE__TOT_SFB, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "huffcb: nrSfb (%3hu) != totSfb (%3hu)\n", nrSfb, tot_sfb );
        printf( "huffcb: --> something should be done here\n");
      }
    }
    return 0;
  }
  if ( nsect > tot_sfb ) {
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "huffcb: nsect (%3hu) > totSfb (%3hu)", nsect, tot_sfb );
    }
    else {
      ConcealmentDetectErrorSectionData(NSECT__GT__TOTSFB, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("huffcb: nsect (%3hu) > totSfb (%3hu)\n", nsect, tot_sfb );
        printf( "huffcb: --> nsect has been clipped at %3hu\n", tot_sfb );
      }
      nsect = tot_sfb;
    }
    return 0;
  }
  if ( max_sfb && ( group != numGroups ) ) {
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "huffcb: group (%hu) != numGroups (%hu) (huffdec2.c)", group, numGroups );
    }
    else {
      ConcealmentDetectErrorSectionData(GROUP__NE__NUMGROUPS, hConcealment);
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "huffcb: group (%hu) != numGroups (%hu)\n", group, numGroups );
        printf ( "huffcb: --> something should be done here\n");
      }
    }
  }
  SetNrOfCodewords ( nrOfCodewords,
                     hHcrInfo );
  return ( nsect );
}
static int getescape ( int                q,
                       HANDLE_HCR         hHcrInfo, 
                       HANDLE_RESILIENCE  hResilience,
                       HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                       HANDLE_BUFFER      hVm )
{
  int           i;
  int           off;
  int           neg;
  unsigned long tmp; /*error robustness, sfr */

  if ( q < 0 ) {
    if ( q != -16 ) {
      return ( q );
    }
    neg = 1;
  } 
  else {
    if ( q != +16 ) {
      return ( q );
    }
    neg = 0;
  }

  for ( i = 4; ; i++ ) {
    if ( GetReorderSpecFlag ( hResilience )) {
      tmp = GetBits ( 1, 
                      MAX_ELEMENTS, 
                      hResilience, 
                      hEscInstanceData, 
                      hVm );
    }
    else
      {
        tmp = GetBits ( 1, 
                        HCOD_ESC, 
                        hResilience, 
                        hEscInstanceData, 
                        hVm );
      }
    if ( tmp == 0 ) { /* truncation condition */
      break;
    }

    if (GetReadBitCnt(hVm) <= GetWriteBitCnt(hVm) && (i >= 12)) {

      /* if i == 12 and no escape separator has been read -> break,
         because the escape word has a maximum length of Nmax + 4, with Nmax = 8 --> imax = 12 */
      if ( GetEPFlag ( hResilience ) 
           || ( GetConsecutiveReorderSpecFlag ( hResilience ) && GetReorderStatusFlag (hHcrInfo ) ) ) {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("getescape: escape_prefix longer than 8 bit\n");
          printf("getescape: --> length of escape_prefix is set to 8\n");
        }
      }
      else {
        CommonExit(2, "getescape: escape_prefix longer than 8 bit (huffdec2.c)");
      }
      CommonExit(2,"getescape: escape_prefix longer than 8 bit (huffdec2.c)");
      break; 
    }
  }
  if ( GetReorderSpecFlag ( hResilience )) {
    off = GetBits ( i,
                    MAX_ELEMENTS, 
                    hResilience, 
                    hEscInstanceData,
                    hVm );
  }
  else 
    {
      off = GetBits ( i,
                      HCOD_ESC, 
                      hResilience, 
                      hEscInstanceData,
                      hVm );
    }
  i = off + (1<<i);
  if(neg)
    i = -i;
  return i;
}

unsigned short  HuffSpecKernelPure ( int*                     qp, 
                                     Hcb*                     hcb, 
                                     Huffman*                 hcw, 
                                     int                      step,
                                     HANDLE_HCR               hHcrInfo,
                                     HANDLE_RESILIENCE        hResilience,
                                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                     HANDLE_CONCEALMENT       hConcealment,
                                     HANDLE_BUFFER            hSpecData )
                   
{
  int            temp;
  unsigned long  codewordBegin;
  unsigned short codewordLength;

  ConcealmentTouchLine ( step,
                         hConcealment );
  codewordBegin = GetReadBitCnt ( hSpecData );
  temp = decode_huff_cw ( hcw,
                          1, 
                          hResilience, 
                          hEscInstanceData,
                          hSpecData );
  unpack_idx(qp, temp, hcb);
  
  if (!hcb->signed_cb)
    get_sign_bits ( qp, 
                    step,
                    hResilience, 
                    hEscInstanceData,
                    hSpecData );
  if ( hcw == &book11[0] ) {
    qp[0] = getescape ( qp[0],
                        hHcrInfo, 
                        hResilience,
                        hEscInstanceData,
                        hSpecData);
    qp[1] = getescape ( qp[1],
                        hHcrInfo, 
                        hResilience,
                        hEscInstanceData,
                        hSpecData);
  }
  /* section data coding of codebook 11: checks max/min values within a SFB, if codebook 11 is used */

  codewordLength = (unsigned short) (GetReadBitCnt ( hSpecData ) - codewordBegin);

  if ( GetLenOfLongestCwFlag( hResilience ) && 
       ( codewordLength > GetLenOfLongestCw ( hHcrInfo ) ) ) {
    if ( ! GetConsecutiveReorderSpecFlag ( hResilience ) ||       /* errors are not allowed, if HCR_SQUARE is disabled */
         ! GetReorderStatusFlag ( hHcrInfo ) ) {                  /* errors are not allowed in PCWs */
      if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
        CommonExit ( 2, "HuffSpecKernelPure: codewordLength (%hu) > lengthOfLongestCodeword (%hu))", codewordLength, GetLenOfLongestCw ( hHcrInfo ) );
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf ("HuffSpecKernelPure: codewordLength (%hu) > lengthOfLongestCodeword (%hu))\n", codewordLength, GetLenOfLongestCw ( hHcrInfo ) );
        }
      }
    }
  }

  return ( codewordLength );
}

static unsigned short HuffSpecKernelNoReordering ( int*               qp, 
                                                   Hcb*               hcb,
                                                   Huffman*           hcw,
                                                   int                step,
                                                   unsigned short     codebook,
                                                   HANDLE_RESILIENCE  hResilience,
                                                   HANDLE_HCR         hHcrInfo,
                                                   HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                                   HANDLE_CONCEALMENT hConcealment,
                                                   HANDLE_BUFFER      hSpecData )
{
  unsigned short codewordLength;
  codewordLength = HuffSpecKernelPure ( qp, 
                                        hcb, 
                                        hcw, 
                                        step, 
                                        hHcrInfo,
                                        hResilience, 
                                        hEscInstanceData,
                                        hConcealment,
                                        hSpecData );
    ConcealmentDetectErrorCodeword ( step,
                                     codewordLength, 
                                     hcb->maxCWLen,
                                     hcb->lavInclEsc,
                                     codebook,
                                     qp,
                                     CODEWORD_NON_PCW,
                                     hResilience,
                                     hHcrInfo,
                                     hConcealment );
  Vcb11ConcealmentPatch ( "HuffSpecKernelNoReordering",
                          codebook,
                          hcb->lavInclEsc,
                          qp,
                          step,
                          hResilience );
  return ( codewordLength );
}

static unsigned short HuffSpecKernelInclReordering ( int*               qp, 
                                                     Hcb*               hcb, 
                                                     Huffman*           hcw, 
                                                     unsigned short     step, 
                                                     unsigned short     codebook,
                                                     unsigned short*    codewordsInSet,
                                                     HANDLE_BUFFER      hSpecData,
                                                     HANDLE_RESILIENCE  hResilience,
                                                     HANDLE_HCR         hHcrInfo,
                                                     HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                                     HANDLE_CONCEALMENT hConcealment)
{
  unsigned short codewordLength = 0;
  
  StoreBufferPointer ( hSpecData );
  if ( ! GetReorderStatusFlag ( hHcrInfo ) ) {
    ReorderSpecDecPCWFinishedCheck ( hcb->maxCWLen,
                                     hSpecData,
                                     hResilience,
                                     hHcrInfo,
                                     hEscInstanceData );
  }
  if ( ! GetReorderStatusFlag ( hHcrInfo ) ) {
    ReadPcws ( &codewordLength,
               qp,
               hcb,
               hcw,
               codebook,
               hcb->maxCWLen,
               step, 
               hResilience,
               hSpecData,
               hHcrInfo, 
               hEscInstanceData,
               hConcealment );
  }
  else {
    if ( GetConsecutiveReorderSpecFlag ( hResilience ) ) {
      ReadNonPcwsNew ( &codewordLength,
                       qp, 
                       hcb, 
                       hcw, 
                       step, 
                       codebook,
                       codewordsInSet,
                       hResilience, 
                       hHcrInfo,
                       hEscInstanceData,
                       hConcealment );
    }
    else {
      ReadNonPcws ( &codewordLength,
                    qp, 
                    hcb, 
                    hcw, 
                    step, 
                    codebook,
                    hResilience, 
                    hHcrInfo,
                    hEscInstanceData,
                    hConcealment );
    }
  }
  return ( codewordLength );
}

static unsigned short HuffSpecDecDefault ( Info*              info, 
                                           byte*              sect, 
                                           int                nsect, 
                                           int                quant[],
                                           HANDLE_RESILIENCE  hResilience,
                                           HANDLE_HCR         hHcrInfo,
                                           HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                           HANDLE_CONCEALMENT hConcealment,
                                           HANDLE_BUFFER      hSpecData )
{
  short*         bandp;
  short*         bands;
  unsigned short bottom;
  unsigned short lineCnt; 
  unsigned short lineStart;
  unsigned short sectionCnt;
  unsigned short sfbCnt;
  unsigned short step;
  unsigned short stop;
  unsigned short codebook;
  unsigned short top;
  int*           qp;      /* probably could be short */
  Hcb*           hcb;
  Huffman*       hcw;
  unsigned short codewordLength;
  unsigned short lengthOfLongestCodeword = 0;

  bands = info->bk_sfb_top;
  bottom = 0;
  lineCnt = 0;
  bandp = bands;
  for( sectionCnt = nsect; sectionCnt; sectionCnt-- ) {
    codebook = sect[0];  /* codebook for this section */
    top   = sect[1];
    sect += 2;
    if ( codebook == BOOKSCL ) {
      if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
        CommonExit ( 2, "HuffspecDecDefault: book %d: invalid book (huffdec2.c, HuffSpecDecDefault())", BOOKSCL );
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf ("HuffspecDecDefault: book %d: invalid book (huffdec2.c, HuffSpecDecDefault())\n", BOOKSCL );
          printf( "HuffspecDecDefault: --> book has been set to %d\n", ZERO_HCB );
        }
        codebook = ZERO_HCB;
      }
    }
    if( ( codebook == ZERO_HCB ) ||
#ifdef PNS
        ( codebook == NOISE_HCB ) ||
#endif
        ( codebook == INTENSITY_HCB ) ||
        ( codebook == INTENSITY_HCB2 ) ) {
      bandp = bands+top;
      lineCnt = bandp[-1];
      bottom = top;
      continue;
    }
    if(codebook < BY4BOOKS+1)
      step = 4;
    else 
      step = 2;
    hcb       = &book[codebook];
    hcw       = hcb->hcw;
    qp        = quant       + lineCnt;
    ConcealmentSetLine(lineCnt, hConcealment);
    lineStart = lineCnt;
    for ( sfbCnt = bottom; sfbCnt < top; sfbCnt++ ) {
      stop = *bandp++;
      while ( lineCnt < stop ) {
        codewordLength = HuffSpecKernelNoReordering ( qp, 
                                                      hcb, 
                                                      hcw, 
                                                      step, 
                                                      codebook,
                                                      hResilience, 
                                                      hHcrInfo,
                                                      hEscInstanceData,
                                                      hConcealment,
                                                      hSpecData );
        lengthOfLongestCodeword = MAX ( codewordLength, lengthOfLongestCodeword );
        qp       += step;
#ifdef TESTSECTIONSWITCHING
        if (((bno % 2 == 0) && (sectionCnt % 2 == 0)) ||
            ((bno % 2 == 1) && (sectionCnt % 2 == 1)))
          ConcealmentSetErrorCodeword(step, hConcealment);
#endif
        ConcealmentIncrementLine(step, hConcealment);
        lineCnt  += step;
      } 
      if(debug['q']){
        unsigned short idx;
        printf("\nsect %d %d\n", codebook, lineStart);
        for (idx = lineStart; idx < lineCnt; idx += 2 )
          printf("%d %d  ",quant[idx],quant[idx+1]);
        printf("\n");
      }
    }
    bottom = top;
  }
  return ( lengthOfLongestCodeword ); 
}

static unsigned short HuffSpecDecInterleaved ( HANDLE_AACDECODER  hDec,
                                               Info*              info, 
                                               byte*              sect, 
                                               int                nsect, 
                                               int                quant[], 
                                               HANDLE_BUFFER      hHcrSpecData,
                                               HANDLE_RESILIENCE  hResilience,
                                               HANDLE_HCR         hHcrInfo,
                                               HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                               HANDLE_CONCEALMENT hConcealment)
{
  Hcb*                  hcb = NULL;
  Huffman*              hcw = NULL;
  int*                  qp;
  unsigned short        step;
  volatile int          i; /* volatile is necessary for Optimization only, probably due to compiler bug
                              gcc version 2.95.2 19991024 (release) */
  int                   j;
  unsigned short        codeNr                 = 0;  /* code counter */
  unsigned short        unitsInSpectralDirection;    /* number of units in spectral direction */
  unsigned short        unitNrInSpectralDirection;   /* unit counter in spectral direction */
  unsigned short        sfb            [NSHORT];     /* number of current sfb           for each group */
  unsigned short        spectralLine   [NSHORT];     /* number of current spectral line for each group */
  unsigned short        codebookArray  [NSHORT];     /* number of current codebook      for each group */
  unsigned short        section        [NSHORT];     /* number of current section       for each group */
  unsigned short        groupNr;                     /* number of current group */
  unsigned short        groupNrPast;                 /* number of past groups */
  unsigned short        codewordLength;
  unsigned short        lengthOfLongestCodeword = 0;
  unsigned short        preSortingIndex;
  unsigned short        firstLineForConcealment = 0;
  unsigned short        codewordsInSet          = 0;


 
  if ( ! nsect )
    return ( 0 );
  if (debug['i']) {
    if ( info->islong ) {
      printf("long block interleaving:\n\n");
    }
    else {
      printf("short block interleaving:\n\n");
    }
  }
  unitsInSpectralDirection = ( GetBlockSize(hDec) / 4 ) / info->nsbk;
  for ( preSortingIndex = 0; preSortingIndex < sizeof(preCodebookSorting)/ ( sizeof(short) * 2 ); preSortingIndex++) {
    qp       = quant;
    ConcealmentSetLine(0, hConcealment);
    firstLineForConcealment = 0;
    /* initialization */
    shortclr ( (short*) spectralLine, NSHORT );
    shortclr ( (short*) codebookArray,   NSHORT );
    shortclr ( (short*) section,      NSHORT );
    for ( groupNr = 0; groupNr < info->num_groups ; groupNr++ ) {               
      sfb[groupNr] = groupNr * info->sfb_per_sbk[0];                 /* initialization of sfb[]
                                                                        with number of first sfb within each group */
      for ( groupNrPast = 0; groupNrPast < groupNr; groupNrPast++ ) {/* initialization of spectralLine[] */
        spectralLine[groupNr] += (info->bins_per_bk/8) * info->group_len[groupNrPast]; /* with number of first spectral line within each group */
      }
    }                                                               
    /* loops */
    for ( unitNrInSpectralDirection = 0; 
          unitNrInSpectralDirection < unitsInSpectralDirection; 
          unitNrInSpectralDirection ++ ) {  
      for ( groupNr = 0; groupNr < info->num_groups; groupNr++ ) {         /* for each group in temporal direction */
        while (info->bk_sfb_top[sfb[groupNr]] <= spectralLine[groupNr]) {/* trace sfb of current group */
          sfb[groupNr]++;
        }
        while ( sect[2 * section[groupNr] + 1] <= sfb[groupNr] ) {       /* trace section/codebook of current group */
          section[groupNr]++;
        }
        codebookArray[groupNr] = sect[2 * section[groupNr]];
        /* correct the virtual codebooks series */
       
        if ( codebookArray[groupNr] == BOOKSCL ) {
          if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
            CommonExit ( 2, "HuffSpecDecInterleaved: book %d: invalid book (huffdec2.c, HuffSpecDecInterleaved())", BOOKSCL );
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "HuffSpecDecInterleaved: book %d: invalid book (huffdec2.c, HuffSpecDecInterleaved())\n", BOOKSCL );
              printf( "HuffSpecDecInterleaved: --> book has been set to %d\n", ZERO_HCB );
            }
            codebookArray[groupNr] = ZERO_HCB;
          }
        }
        if( ! ( ( codebookArray[groupNr] == ZERO_HCB ) ||
#ifdef PNS
                ( codebookArray[groupNr] == NOISE_HCB ) ||
#endif
                ( codebookArray[groupNr] == INTENSITY_HCB ) ||
                ( codebookArray[groupNr] == INTENSITY_HCB2 ) ) ) {
          hcb = &book[codebookArray[groupNr]];      /* init hcb (pointer) with huffman codebook according codebook */
          hcw = hcb->hcw;                        /* init hcw (pointer) with array of codewords of current codebook */
          if ( codebookArray[groupNr] < BY4BOOKS + 1 )                       /* decide between 2 and 4 dimensional codewords */
            step = 4;
          else 
            step = 2;
        }
        else {
          step = 4;
        }
        for ( j = 0; j < info->group_len[groupNr]; j++ ) {             /* for each unit of the current group in temporal direction */
          for ( i = 0; i < 4; i += step ) {                            /* for each codeword of the current unit (4 spectral lines) */
            if( ! ( ( codebookArray[groupNr] == ZERO_HCB ) ||
#ifdef PNS
                    ( codebookArray[groupNr] == NOISE_HCB ) ||
#endif
                    ( codebookArray[groupNr] == INTENSITY_HCB ) ||
                    ( codebookArray[groupNr] == INTENSITY_HCB2 ) ) ) {
              if ( ( ! GetReorderSpecPreSortFlag ( hResilience ) ) ||
                   ( codebookArray[groupNr] >= preCodebookSorting[preSortingIndex].min &&
                     codebookArray[groupNr] <= preCodebookSorting[preSortingIndex].max ) ) {
                if (debug['i']) {
                  printf("code %3hu, group %1hu, section %2hu, sfb %2hu (%2hu), codebook %2i", 
                        codeNr,                                       /* comparison with wrapper */
                        groupNr,                                      /* comparison with wrapper */
                        section       [groupNr],                      /* comparison with wrapper */
                        sfb           [groupNr],                      /* comparison with wrapper */
                        sfb           [groupNr]%info->sfb_per_sbk[0], /* comparison with wrapper */
                        codebookArray [groupNr]                       /* comparison with wrapper */
                        );
                  printf("\n");
                }
                codewordLength = HuffSpecKernelInclReordering ( qp, 
                                                                hcb, 
                                                                hcw, 
                                                                step, 
                                                                codebookArray[groupNr],
                                                                &codewordsInSet,
                                                                hHcrSpecData,
                                                                hResilience,
                                                                hHcrInfo, 
                                                                hEscInstanceData,
                                                                hConcealment );
                lengthOfLongestCodeword = MAX ( codewordLength, lengthOfLongestCodeword ) ;
              }
              codeNr++;
            }
            qp += step;
            firstLineForConcealment += step;
            ConcealmentIncrementLine(step, hConcealment);  
            /* alternative possibility:
               ConcealmentSetLine ( firstLineForConcealment, hConcealment );
            */
            /* this check works only, if ERRORCONCEALMENT = 1 */
/*             if ( firstLineForConcealment != ConcealmentGetLine (hConcealment)) { */
/*               CommonExit (1, "IMPLEMENTATION ERROR: \n%4d != %4d", firstLineForConcealment, ConcealmentGetLine (hConcealment) ); */
/*             } */
            spectralLine[groupNr] += step;
          }
        }
      }
    }
    if ( ! GetReorderSpecPreSortFlag ( hResilience ) ) {
      break;
    }
  }
  CheckDecodingProgress( hHcrSpecData,
                         hEscInstanceData, 
                         hHcrInfo, 
                         hResilience );

  return ( lengthOfLongestCodeword );
}

static double esc_iquant(int q)
{
  if (q > 0) {
    if (q < MAX_IQ_TBL) {
      return((double)iq_exp_tbl[q]);
    }
    else {
      return(pow(q, 4./3.));
    }
  }
  else {
    q = -q;
    if (q < MAX_IQ_TBL) {
      return((double)(-iq_exp_tbl[q]));
    }
    else {
      return(-pow(q, 4./3.));
    }
  }
}
 
static int HuffSpecFrame ( HANDLE_AACDECODER        hDec,
                           Info*              info, 
                           int                nsect, 
                           byte*              sect, 
                           short*             factors, 
                           double*             coef,
                           int                max_spec_coefficients,
                           HANDLE_RESILIENCE  hResilience,
                           HANDLE_BUFFER      hHcrSpecData,
                           HANDLE_CONCEALMENT hConcealment,
                           HANDLE_HCR         hHcrInfo,
                           HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                           HANDLE_BUFFER      hVm
#ifdef I2R_LOSSLESS
                           ,int * sls_quant_mono_temp
#endif
                           )
{
  int            quant[LN2]; 
  unsigned short lengthOfLongestCodeword;
  
  intclr  ( quant, LN2);

  if ( GetReorderSpecFlag ( hResilience ) ) {
    ResetWriteBitCnt ( hHcrSpecData );
    PrepareWriting ( hHcrSpecData );
    TransferBits ( GetLenOfSpecData ( hHcrInfo ), 
                   REORDERED_SPECTRAL_DATA,
                   hVm,
                   hHcrSpecData,
                   hEscInstanceData, 
                   hResilience );
    InitHcr ( hHcrSpecData, hHcrInfo );
    lengthOfLongestCodeword = HuffSpecDecInterleaved ( hDec,
                                                       info, 
                                                       sect, 
                                                       nsect, 
                                                       quant, 
                                                       hHcrSpecData,
                                                       hResilience,
                                                       hHcrInfo, 
                                                       hEscInstanceData,
                                                       hConcealment );
  }
  else
    lengthOfLongestCodeword = HuffSpecDecDefault ( info, 
                                                   sect, 
                                                   nsect, 
                                                   quant, 
                                                   hResilience, 
                                                   hHcrInfo,
                                                   hEscInstanceData,
                                                   hConcealment, 
                                                   hVm );
#ifdef I2R_LOSSLESS
  if (sls_quant_mono_temp) {  /* for coupling etc this is zero */
    memcpy(sls_quant_mono_temp, quant, 1024*sizeof(int));
  }
#endif

  if ( hHcrInfo && hEscInstanceData && ! ( BsGetCrcResultInFrameFlagEP ( hEscInstanceData ) || BsGetReadErrorInFrameFlagEP ( hEscInstanceData ) ) &&
       ( hResilience && GetLenOfLongestCwFlag ( hResilience ) ) &&
       ( lengthOfLongestCodeword != GetLenOfLongestCw ( hHcrInfo ) ) ) {
    if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
      printf("HuffSpecFrame: lengthOfLongestCodeword (%hu) != transmittedLengthOfLongestCodeword (%hu) (HuffSpecDecDefault()\n",
             lengthOfLongestCodeword,
             GetLenOfLongestCw ( hHcrInfo ) );
    }
  }

  if ( GetReorderSpecFlag ( hResilience ) ) {
    ConcealmentCheckLenOfHcrSpecData ( hHcrInfo,
                                       hConcealment );
  }

  /* check number of coefficients (only 12 allowed for LFE) */
  if (max_spec_coefficients>0) {
    int x;
    for (x=max_spec_coefficients; x<LN2; x++) {
      if (quant[x] != 0) max_spec_coefficients=-1;
    }
    if (max_spec_coefficients==-1) printf("WARNING: more non-zero spectral lines than expected!\n");
  }

  /* pulse noisless coding reconstruction */
  if ( (info->islong) && (pulseInfo.pulse_data_present) )
    pulse_nc(quant, &pulseInfo
             ,hResilience, hEscInstanceData
             );

  if (!info->islong) {
    int tmp_spec[LN2];
    deinterleave ( quant,
                   tmp_spec,
                   sizeof(int),
                   info->group_len,
                   info->sfb_per_sbk,
                   info->sfb_width_short,
                   info->nsbk,
                   GetBlockSize(hDec),
                   hResilience,
                   info->num_groups );
    memcpy(quant,tmp_spec,sizeof(tmp_spec));
  }
  ConcealmentDeinterleave (info, hResilience, hConcealment);

  if ( debug['Q'] ) {
    PrintSpectralValues ( quant );
  }
#ifdef SENSITIVITY
  if ( hEscInstanceData && hConcealment ) {
    ResilienceDealWithSpectralValues ( quant,
                             hConcealment,
                             bno );
  }
#endif /*SENSITIVITY*/

  /* inverse quantization */
  {
   unsigned short i;
    for (i=0; i<info->bins_per_bk; i++) {
      coef[i] = esc_iquant(quant[i]);
    }   
  }
  /* rescaling */
  {
    unsigned short i;
    unsigned short k;
    int sbk, nsbk, sfb, nsfb, fac, top;
    double *fp, scale;

    i = 0;
    fp = coef;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) {
      nsfb = info->sfb_per_sbk[sbk];
      k=0;
      for (sfb=0; sfb<nsfb; sfb++) {
        top = info->sbk_sfb_top[sbk][sfb];
        fac = factors[i++]-SF_OFFSET;

        if (fac >= 0 && fac < TEXP) {
          scale = exptable[fac];
        }
        else {
          if (fac == -SF_OFFSET) {
            scale = 0;
          }
          else {
            scale = pow( 2.0,  0.25*fac );
          }
        }
        for ( ; k<top; k++) {
          *fp++ *= scale;
          }
      }
      }
    }
  return 1;
}

int getics ( HANDLE_AACDECODER        hDec,
             Info*                    info, 
             int                      common_window, 
             WINDOW_SEQUENCE*         win, 
             WINDOW_SHAPE*            wshape, 
             byte*                    group, 
             byte*                    max_sfb, 
             PRED_TYPE                pred_type, 
             int*                     lpflag, 
             int*                     prstflag, 
             byte*                    cb_map, 
             double*                   coef, 
             int                      max_spec_coefficients,
             short*                   global_gain, 
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp, 
             TNS_frame_info*          tns,
             HANDLE_BSBITSTREAM       gc_streamCh, 
             enum AAC_BIT_STREAM_TYPE bitStreamType,
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT            qc_select,
             HANDLE_BUFFER            hVm,
             int                      extensionLayerFlag,
             int                      er_channel
#ifdef I2R_LOSSLESS
             , int*                   sls_quant_mono_temp
#endif
             )

{
  int            nsect;
  int            i;
  int            cb;
  int            top;
  int            bot;
  int            tot_sfb;
  byte           sect[ 2*(MAXBANDS+1) ];
  int            tns_data_present=0;
  RVLC_ESC1_DATA rvlcESC1data;

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
  if ((bitStreamType != SCALABLE) && (bitStreamType != SCALABLE_ER)){ 
    memcpy(info, winmap[*win], sizeof(Info));
  }
  if (( !common_window ) && ( bitStreamType != SCALABLE ) && (bitStreamType != SCALABLE_ER) && (bitStreamType != MULTICHANNEL_ELD)) {
    int nPredictorDataPresent;

    get_ics_info ( win, 
                   wshape, 
                   group, 
                   max_sfb, 
                   pred_type, 
                   lpflag, 
                   prstflag, 
                   bitStreamType,
                   qc_select,
                   hResilience,
                   hEscInstanceData,
                   nok_ltp,
                   NULL,
                   common_window,
                   hVm, 
                   &nPredictorDataPresent );
    
    memcpy(info, winmap[*win], sizeof(Info));
  } else if (!common_window && (bitStreamType == MULTICHANNEL_ELD)) {
      *max_sfb = (unsigned char) GetBits ( LEN_MAX_SFBL,
                                           MAX_SFB, 
                                           hResilience, 
                                           hEscInstanceData,
                                           hVm ); 
      group[0] = 1;
    
  } else {
    if( info->nsbk == 1 ) {
      group[0] = 1;
    } else {
      int sum = 0;
      for( i=0; i<info->num_groups; i++ ) {
        sum  += info->group_len[i];
        group[i]  = sum;
      }
    }
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

  /* 
   * section data
   */
  nsect = huffcb ( sect, 
                   *info,
                   info->sectbits, 
                   tot_sfb, 
                   info->sfb_per_sbk[0], 
                   *max_sfb,
                   info->num_groups,
                   hHcrInfo,
                   hResilience, 
                   hEscInstanceData,
                   hConcealment,
                   hVm);
  if( ( nsect == 0 ) && ( *max_sfb > 0 ) ) {
    if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "getics: nsect = 0 but maxSfb = %hu", *max_sfb);
    }
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf ( "getics: nsect = 0 but maxSfb = %hu\n", *max_sfb);
        printf ( "getics: --> maxSfb has been set to 0\n" );
      }
      *max_sfb = 0;
    }
  }

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
  if ( GetLenOfScfDataFlag(hResilience) && GetRvlcScfFlag(hResilience) ) {
    CommonExit (1,"getics: -lsf *and* -rvlc makes no sense!" ); 
  }

  if ( !GetLenOfScfDataFlag(hResilience) && !GetRvlcScfFlag(hResilience) ) {
    /* normal AAC Mode */
    if(!hufffac( info, 
                 group, 
                 nsect, 
                 sect, 
                 *global_gain, 
                 0,
                 factors,
                 hResilience, 
                 hEscInstanceData,
                 hVm ) ) {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
        printf("getics: hufffac returns zero (huffdec2.c, getics())\n");
      }
      return 0;
    }
  } else {
    /* length_of_sf_data within bitstream */
    if ( GetLenOfScfDataFlag(hResilience) ) {
      if(!HuffScfResil( info, 
                        group, 
                        nsect, 
                        sect, 
                        *global_gain, 
                        factors,
                        hVm,
                        hResilience, 
                        hEscInstanceData ) ) {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
          printf("getics: HuffScfResil returns zero (huffdec2.c, getics())\n");
        }
        return 0;
      }
    }
    /* RVL coded scalefactors */
    if ( GetRvlcScfFlag(hResilience) ) {
      RVLCScfDecodingESC1( info, 
                       group, 
                       nsect, 
                       sect, 
                       hVm,
                       hResilience, 
                       hEscInstanceData,
                       &rvlcESC1data);      
    }
  }

  if ((bitStreamType  !=  SCALABLE) && (bitStreamType != SCALABLE_ER)){ 
    /*
     * pulse noiseless coding
     */
   if (bitStreamType  !=  MULTICHANNEL_ELD) {
    if ((pulseInfo.pulse_data_present = GetBits (LEN_PULSE_PRES,
                                                 PULSE_DATA_PRESENT, 
                                                 hResilience, 
                                                 hEscInstanceData,
                                                 hVm ))) {
      if (debug['V']) fprintf(stderr,"# PulseData detected\n");
      if (info->islong) {
        GetPulseNc ( &pulseInfo,
                     hResilience, 
                     hEscInstanceData,
                     hVm );
      }
      else {
        if ( ! ( hResilience  && GetEPFlag ( hResilience ) ) ) {
          CommonExit ( 2, "getics: pulse data not allowed for short blocks");
        }
        else {
          if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
            printf ( "getics: pulse data not allowed for short blocks\n");    
            printf ( "getics: -->something should be done here\n");
          }
        }
      }
     }
    }


    /*
     * tns data
     */

    tns_data_present = GetBits (LEN_TNS_PRES,
                                TNS_DATA_PRESENT, 
                                hResilience, 
                                hEscInstanceData,
                                hVm );

    if(!tns_data_present)
      clr_tns ( info, tns );

    
    if(tns_data_present && ((bitStreamType != MULTICHANNEL_ER)&&(bitStreamType != MULTICHANNEL_ELD)))
      get_tns ( info, tns, hResilience, hEscInstanceData,hVm );
    if (tns_data_present&&debug['V']) fprintf(stderr,"# TNS detected\n");
    
    /*
     * Sony gain control
     */
    if (bitStreamType  !=  MULTICHANNEL_ELD) {
      if (GetBits ( LEN_GAIN_PRES,
                    GAIN_CONTROL_DATA_PRESENT, 
                    hResilience, 
                    hEscInstanceData,
                    hVm )) {
        get_gcBuf( *win,
                   gc_streamCh,
                   hResilience, 
                   hEscInstanceData,
                   hVm);
      }
    }
  }
  if ( GetReorderSpecFlag ( hResilience ) ) {
    ReadLenOfSpecData ( hHcrInfo, 
                        hResilience,
                        hVm, 
                        hEscInstanceData );
  }
  if ( GetLenOfLongestCwFlag ( hResilience ) ) {
    ReadLenOfLongestCw ( hHcrInfo, 
                         hResilience, 
                         hVm, 
                         hEscInstanceData);
  }

  /* RVL coded scalefactors */
  if ( GetRvlcScfFlag(hResilience) && 
       ((bitStreamType == MULTICHANNEL_ER) || (bitStreamType == MULTICHANNEL_ELD) || (bitStreamType == SCALABLE_ER)) ) 
  {
    RVLCScfDecodingESC2( info, 
                         group, 
                         nsect, 
                         sect, 
                         *global_gain, 
                         factors,
                         hVm,
                         hResilience, 
                         hEscInstanceData,
                         &rvlcESC1data);
  }

  if ( extensionLayerFlag ) {
    if((tns_data_present && bitStreamType == MULTICHANNEL_ER) || 
       (tns_data_present && bitStreamType == MULTICHANNEL_ELD) || 
       (bitStreamType == SCALABLE_ER && GetTnsExtPresent(hDec, er_channel))) {
      get_tns ( info, tns, hResilience, hEscInstanceData,hVm );
    }
  }
  else {
    if((tns_data_present && bitStreamType == MULTICHANNEL_ER) || 
        (tns_data_present && bitStreamType == MULTICHANNEL_ELD) || 
        (bitStreamType == SCALABLE_ER && GetTnsDataPresent(hDec, er_channel ))) {
      get_tns ( info, tns, hResilience, hEscInstanceData,hVm );
    }
  }

  return HuffSpecFrame ( hDec,
                         info, 
                         nsect, 
                         sect, 
                         factors,
                         coef,
                         max_spec_coefficients,
                         hResilience,
                         hHcrSpecData,
                         hConcealment, 
                         hHcrInfo, 
                         hEscInstanceData,
                         hVm
#ifdef I2R_LOSSLESS
                         , sls_quant_mono_temp
#endif
                         );
}

/* 
 * get scale factors
 */
int hufffac ( Info*              info, 
              byte*              group, 
              int                nsect, 
              byte*              sect,
              short              global_gain, 
              int                bUsacSyntax,
              short*             factors,
              HANDLE_RESILIENCE  hResilience,
              HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
              HANDLE_BUFFER      hVm )
 
{
  Hcb*     hcb;
  Huffman* hcw;
  int      i;
  int      b;
  int      bb;
  int      t;
  int      n;
  int      sfb;
  int      top;
  int      fac;
  int      is_pos;
  int      factor_transmitted[MAXBANDS];
  int*     fac_trans;
#ifdef PNS
  int      noise_pcm_flag = 1;
#endif /*PNS*/
  int      noise_nrg;
  int      is_first_group = 1;

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ ) {
    t   = sect[0];              /* codebook for this section */
    top = sect[1];              /* top of section in sfb */
    sect += 2;
    for ( ; sfb < top; sfb++ ) {
      fac_trans[sfb] = t;
    }
  }

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  noise_nrg = global_gain - NOISE_OFFSET;

  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    printf("scale factors\n");
  for(b = 0; b < info->nsbk; ){
    n = info->sfb_per_sbk[b];
    b = *group++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:        /* zero book */
        break;
      default:              /* spectral books */
        /* decode scale factor */
        if( !(bUsacSyntax && is_first_group && (i == 0)) ){
          /* for USAC, skip first scale factor */
          t = decode_huff_cw ( hcw,
                               0, 
                               hResilience, 
                               hEscInstanceData,
                               hVm );
          fac += t - MIDFAC;    /* 1.5 dB */
        }

        if (debug['f'])
          printf("%3d:%3d", i, fac);
        if( ( fac >= ( 2 * TEXP ) ) ) {
          if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
            CommonExit(2, "hufffac: Scale factor too large: %d", fac);
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "hufffac: scale factor too large: %d\n", fac);
              printf( "hufffac: --> scale factor has been set to 255\n");
            }
            fac = 255;
          }
        }
        if( fac < 0 ) {
          if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
            CommonExit( 2, "hufffac: scale factor too small: %d", fac);
          }
          else {
            if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
              printf( "hufffac: scale factor too small: %d\n", fac);
              printf( "hufffac: --> scale factor has been set to 0\n");
            }
            fac = 0;
          }
        }
        factors[i] = fac;
        break;
      case BOOKSCL:         /* invalid books */
        if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
          CommonExit ( 2, "hufffac: book %d: invalid book", BOOKSCL );
        }
        else {
          if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 3 ) {
            printf( "hufffac: book %d: invalid book\n", BOOKSCL );
          }
        }
        break;
      case INTENSITY_HCB:           /* intensity books */
      case INTENSITY_HCB2:
        /* decode intensity position */
        if (debug['V']) fprintf(stderr,"# IS detected\n");
        t = decode_huff_cw ( hcw,
                             0, 
                             hResilience, 
                             hEscInstanceData,
                             hVm );
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
#ifdef PNS
      case NOISE_HCB:       /* noise books */
        /* decode noise energy */
        if (debug['V']) fprintf(stderr,"# PNS detected\n");
        if (noise_pcm_flag) {
          noise_pcm_flag = 0;
          t = GetBits ( NOISE_PCM_BITS,
                        DPCM_NOISE_NRG, 
                        hResilience, 
                        hEscInstanceData,
                        hVm ) - NOISE_PCM_OFFSET;
        }
        else
          t = decode_huff_cw ( hcw,
                               0, 
                               hResilience, 
                               hEscInstanceData,
                               hVm ) - MIDFAC;
        noise_nrg += t;
        if (debug['f'])
          printf("\n%3d %3d (noise, code %3d)", i, noise_nrg, t);
        factors[i] = noise_nrg;
        break;
#endif /*PNS*/
      }
      if (debug['f'])
        printf("%3d: %3d %3d\n", i, fac_trans[i], factors[i]);
    }
    if (debug['f'])
      printf("\n");

    /* expand short block grouping */
    if (!(info->islong)) {
      for(bb++; bb < b; bb++) {
        for (i=0; i<n; i++) {
          factors[i+n] = factors[i];
        }
        factors += n;
      }
    }
    fac_trans += n;
    factors += n;
    is_first_group = 0;
  }
  return 1;
}



