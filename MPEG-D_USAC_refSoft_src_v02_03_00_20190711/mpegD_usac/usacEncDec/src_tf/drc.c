/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

#ifdef	DRC

#include <string.h>
#include <math.h>

#include "common_m4a.h"

#include "all.h"
#include "tf_mainStruct.h"
#include "allVariables.h"
#include "buffers.h"

#include "drc.h"
 

#ifdef CT_SBR

/*extern*/ float drcFactorVector[64/*Chans*/][64][64];
#endif


/*
 * Dynamic Range Control 
 */


#define DRC_REF_LEVEL	(27*4)	/* -27 dB below FS (typical for movies) */
#define MAX_DRC_THREADS	3	/* could be more! */
#define MAX_CHAN	(FChans + SChans + BChans + LChans + ICChans)
#define MAX_DRC_BANDS	(1<<LEN_DRC_BAND_INCR)

enum {
  /* DRC */
  LEN_DRC_PL		= 7, 
  LEN_DRC_PL_RESV	= 1, 
  LEN_DRC_PCE_RESV	= (8 - LEN_TAG),
  LEN_DRC_BAND_INCR	= 4, 

#ifdef CT_SBR
  LEN_DRC_BAND_INTERP	= 4, 
#else
  LEN_DRC_BAND_RESV	= 4, 
#endif

  LEN_DRC_BAND_TOP	= 8, 
  LEN_DRC_SGN		= 1, 
  LEN_DRC_MAG		= 7, 
  DRC_BAND_MULT	= 4,	/* band top is multiple of this no of bins */
  
  xXxXxXx
};

/* Chans: FChans + SChans + BChans ??? + LChans */


typedef struct {
  int excl_chn_present;
  int excl_chn_mask[MAX_CHAN];
  int num_bands;
  int band_top[MAX_DRC_BANDS];
  int prog_ref_level_present;
  int prog_ref_level;
  int drc_sgn[MAX_DRC_BANDS];
  int drc_mag[MAX_DRC_BANDS];
#ifdef CT_SBR
  int drc_interp_scheme;
#endif
} DRC_Bitstream;

typedef struct {
    int num_bands;
    int band_top[MAX_DRC_BANDS];
    int drc_sgn[MAX_DRC_BANDS];
    int drc_mag[MAX_DRC_BANDS];
#ifdef CT_SBR
    int drc_interp_scheme;
    int drc_interp_scheme_prev;
#endif
} DRC_Info;

struct tagDRC {
  int enable;
  double hi;
  double lo;
  int digital_norm;
  int target_ref_level;
  int prog_ref_level;
  int thread;
  DRC_Bitstream drc_bits[MAX_DRC_THREADS];
  DRC_Info drc_info[MAX_CHAN];
};

/* 
 * initialize DRC
 */

HANDLE_DRC drc_init(void)
{
  HANDLE_DRC drc = (HANDLE_DRC)malloc(sizeof(struct tagDRC));

  int i;
  DRC_Info *p;

  drc->enable = 0;
  drc->thread = 0;

  /* do program level normalization in digital domain */
  drc->digital_norm = 1;

  /* set desired program level */
  drc->target_ref_level = DRC_REF_LEVEL;
  drc->prog_ref_level = DRC_REF_LEVEL;

  /* set drc info structure to default values */
  for (i=0; i<MAX_CHAN; i++) {
    p = &(drc->drc_info[i]);
    p->num_bands = 1; 
    p->band_top[0] = (LN2 / DRC_BAND_MULT) - 1;
    p->drc_sgn[0] = 0;
    p->drc_mag[0] = 0;
#ifdef CT_SBR
    p->drc_interp_scheme = 0;
    p->drc_interp_scheme_prev = 0;
#endif
  }

  return drc;
}

void drc_free(HANDLE_DRC drc)
{
  if (drc!=NULL) free(drc);
}


void drc_set_params(HANDLE_DRC drc, double hi, double lo, int ref_level)
{
  if (drc==NULL) return;
  
  /* hi and lo must be between 0.0 and 1.0, inclusive*/
  if (hi < 0.0) hi = 0;
  if (hi > 1.0) hi = 1.0;
  if (lo < 0.0)	lo = 0;
  if (hi > 1.0)	hi = 1.0;

  /* 0.0 indicates no drc, 1.0 indicates max drc */
  drc->hi = hi;    /* compress */
  drc->lo = lo;    /* boost */

  /* ref_level must be between 0 and 127, inclusive */
  if (ref_level > 127) ref_level = 127;
  if (ref_level < 0) {
    /* enable or disable drc */
    drc->enable = (hi > 0.0)||(lo > 0.0);
    drc->digital_norm = 0;
  } else {
    /* enable or disable drc */
    drc->enable = 1;

    drc->digital_norm = 1;
    drc->target_ref_level = ref_level;
    drc->prog_ref_level = DRC_REF_LEVEL;
  }

  if ((debug['v']||debug['D']) && drc->enable) {
    DebugPrintf(0, "drc: setting compression factors to %f %f", hi, lo);
    if (drc->digital_norm) DebugPrintf(0, "drc: digital norm ref_level %i\n", drc->target_ref_level);
    else DebugPrintf(0, "drc: digital norm is switched off\n");
  }
}



/* 
 * parse DRC information
 */

void drc_reset(HANDLE_DRC drc)
{
    /* set threads to zero */
    drc->thread = 0;
}

static int excluded_channels(DRC_Bitstream*           p,
                             HANDLE_RESILIENCE        hResilience, 
                             HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                             HANDLE_BUFFER            hVm)
{
  int i, j, n;
  int *mask = &p->excl_chn_mask[0];

  n=1;	    /* bytes */
  j=0;	    /* mask bits */
    
  for (i=0; i<7; i++)
    mask[j++] = GetBits(1,
                        MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                        hResilience,
                        hEscInstanceData,
                        hVm);
  
  /* additional_excluded_chns */
  while (GetBits(1,
                 MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                 hResilience,
                 hEscInstanceData,
                 hVm)) {
    for (i=0; i<7; i++)
      mask[j++] = GetBits(1,
                          MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                          hResilience,
                          hEscInstanceData,
                          hVm);
    n++;
  }
  return n;
}
 
int drc_parse(HANDLE_DRC               drc,
              HANDLE_RESILIENCE        hResilience, 
              HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
              HANDLE_BUFFER            hVm)
{
  int i, n, accept;
  DRC_Bitstream *p, tmp_drc_bits;

  n = 1;	/* byte count */

  /* pce_tag_present */
  if (GetBits(1,
              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
              hResilience,
              hEscInstanceData,
              hVm)) {
    accept = (current_program == GetBits(LEN_TAG,
                                         MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                         hResilience,
                                         hEscInstanceData,
                                         hVm)) ? 1 : 0;
    GetBits(LEN_DRC_PCE_RESV,
            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
            hResilience,
            hEscInstanceData,
            hVm);
    n++;
  } else {
    /* default is to apply drc to current program */
    accept = 1;
  }
  p = (accept) ? &drc->drc_bits[drc->thread++] : &tmp_drc_bits;

  /* excluded_chns_present */
  if ((p->excl_chn_present = GetBits(1,
                                     MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                     hResilience,
                                     hEscInstanceData,
                                     hVm))) {
    /* get excl_chn_mask */
    n += excluded_channels(p, hResilience, hEscInstanceData, hVm);
  }

  /* drc_bands_present */
  p->num_bands = 1;
  if (GetBits(1,
              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
              hResilience,
              hEscInstanceData,
              hVm)) {
    /* get band_incr */
    p->num_bands += GetBits(LEN_DRC_BAND_INCR,
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
                            hVm);
#ifdef CT_SBR
    /* interpolation_scheme */
    p->drc_interp_scheme = GetBits(LEN_DRC_BAND_INTERP,
                                   MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                   hResilience,
                                   hEscInstanceData,
                                   hVm);
#else
    /* reserved */
    GetBits(LEN_DRC_BAND_RESV, 
            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
            hResilience,
            hEscInstanceData,
            hVm);
#endif
    /* band_top */
    for (i=0; i<p->num_bands; i++) {
      p->band_top[i] = GetBits(LEN_DRC_BAND_TOP,
                               MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                               hResilience,
                               hEscInstanceData,
                               hVm);
    }
    n += 1 + p->num_bands;
  }

  /* prog_ref_level_present */
  if ((p->prog_ref_level_present = GetBits(1,
                                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                           hResilience,
                                           hEscInstanceData,
                                           hVm))) {
    /* prog_ref_level */
    p->prog_ref_level = GetBits(LEN_DRC_PL,
                                MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                hResilience,
                                hEscInstanceData,
                                hVm);
    GetBits(LEN_DRC_PL_RESV,
            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
            hResilience,
            hEscInstanceData,
            hVm);
    n++;
  }
  /* sgn and ctl */
  for (i=0; i<p->num_bands; i++) {
    p->drc_sgn[i] = GetBits(LEN_DRC_SGN,
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
                            hVm);
    p->drc_mag[i] = GetBits(LEN_DRC_MAG,
                            MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                            hResilience,
                            hEscInstanceData,
                            hVm);
  }
  n += p->num_bands;

  return n;
}



/* 
 * apply DRC
 */

static void
cp_drc(DRC_Info *p, DRC_Bitstream *b)
{
  int i;
  if ((p->num_bands = b->num_bands) == 1) {
    p->band_top[0] = (LN2 / DRC_BAND_MULT) - 1;
    p->drc_sgn[0] = b->drc_sgn[0];
    p->drc_mag[0] = b->drc_mag[0];
    /*
    if(0) {i=0; DebugPrintf(0,"%d %d %4d %d %3d\n",
                            p-&drc->drc_info[0], i, p->band_top[i], p->drc_sgn[i], p->drc_mag[i]);}
    */
  }
  else {
    for (i=0; i<p->num_bands; i++) {
      p->band_top[i] = b->band_top[i];
      p->drc_sgn[i] = b->drc_sgn[i];
      p->drc_mag[i] = b->drc_mag[i];
      /*
      if(0) DebugPrintf(0,"%d %d %4d %d %3d\n", 
                  p-&drc->drc_info[0], i, p->band_top[i], p->drc_sgn[i], p->drc_mag[i]);
      */
    }
  }

#ifdef CT_SBR
  p->drc_interp_scheme = b->drc_interp_scheme;
#endif
}

void drc_map_channels(HANDLE_DRC drc, MC_Info *mip)
{
  int i, j, k, valid_chn, thread, *mask;
  int valid_thread[MAX_DRC_THREADS], valid_threads;
  int num_excl_chn[MAX_DRC_THREADS]; 
  DRC_Bitstream *b;
  
  /* calculate number of valid bits in excl_chn_mask */
  valid_chn = 0;
  for (i=0; i<Chans; i++)
    if (mip->ch_info[i].present)
      valid_chn++;

#if (CChans > 0)  
  for (i=0; i<mip->ncch; i++)
    if (mip->cc_ind[i]/*mip->ch_info[i].cc_ind[i]*/)
      valid_chn++;
#endif
  
  /* check for valid threads */
  valid_threads = 0;
  for (i=0; i<drc->thread; i++) {
    b = &drc->drc_bits[i];
    /* calculate number of excluded channels */
    num_excl_chn[i] = 0;
    if (b->excl_chn_present) {
      mask = &b->excl_chn_mask[0];
      for (j=0; j<valid_chn; j++)
        if (mask[j])
          num_excl_chn[i]++;
    }
    if (num_excl_chn[i] == valid_chn) {
      /* all channels are excluded! */
      valid_thread[i] = 0;
    }
    else {
      valid_thread[i] = 1;
      valid_threads++; 
    }
  }
  
  if (valid_threads == 0) {
    /* nothing to do */
    return;
  }
  
  if (valid_threads == 1) {
    /* point to valid thread */
    for (i=0; i<drc->thread; i++)
      if (valid_thread[i])
        break;

    if (num_excl_chn[i] == 0) {
      /* apply this to all channels */
      b = &drc->drc_bits[i];
	      
      if (b->prog_ref_level_present)
        drc->prog_ref_level = b->prog_ref_level;
      
      /* SCE, CPE and LFE */
      for (i=0; i<Chans; i++) {
        if (!mip->ch_info[i].present)
          continue;   /* chn not present */
        cp_drc(&drc->drc_info[i], b);
      }

#if (CChans > 0)
      /* CCE */
      for (j=0; j<mip->ncch; j++) {
        if (!mip->cc_ind[j]/*!mip->ch_info[i].cc_ind[j]*/)
          continue;   /* chn not present */
        cp_drc(&drc->drc_info[i+j], b);
      }
#endif

      return;
    }
  }
    
  if (valid_threads > 1) {
    /* check consistency of excl_chn_mask amongst valid drc threads */
    for (i=0; i<valid_chn; i++) {
      int present = 0;
      if (debug['D'] && drc->enable) DebugPrintf(0,"%d: ", i);

      for (thread=0; thread<drc->thread; thread++) {
        if (!valid_thread[thread])
          continue;

        b = &drc->drc_bits[thread];
        if (debug['D'] && drc->enable) DebugPrintf(0,"%d",b->excl_chn_mask[i]);

        /* thread applies to this channel */
        if ( (num_excl_chn[thread] == 0) || (!b->excl_chn_mask[i]) ) 
          present++;
      }

      if (debug['D'] && drc->enable) DebugPrintf(0," %d\n", present);

      if (present > 1) {
        /* a channel can appear in only one drc thread! */
        CommonWarning("Error in DRC: channel present in more than one DRC thread\n");
        return;
      }
    }
  }
    
  /* map DRC bitstream information onto DRC channel information */
  for (thread=0; thread<drc->thread; thread++) {
    if (!valid_thread[thread])
      continue;
    
    b = &drc->drc_bits[thread];
    mask = b->excl_chn_mask;

    /* last prog_ref_level transmitted is the one that is used
     * (but it should really only be transmitted once per block!)
     */
    if (b->prog_ref_level_present)
      drc->prog_ref_level = b->prog_ref_level;

    /* SCE, CPE and LFE */
    k = 0;  /* mask bit index */
    for (i=0; i<Chans; i++) {
      if (!mip->ch_info[i].present)
        continue;   /* chn not present */
      if (mask[k++])
        continue;   /* chn excluded from this thread */

      cp_drc(&drc->drc_info[i], b);
    }
#if (CChans > 0)
    /* CCE */
    for (j=0; j<mip->ncch; j++) {
      if (!mip->cc_ind[j]/*!mip->ch_info[i].cc_ind[j]*/)
        continue;   /* chn not present */
      if (mask[k++])
        continue;   /* chn excluded from this thread */

      cp_drc(&drc->drc_info[i+j], b);
    }
#endif
  }
  return;
}



#ifdef CT_SBR

#define FRAME_SIZE 1024
#define NUM_QMF_SUBSAMPLES FRAME_SIZE/32
#define NUM_QMF_SUBSAMPLES_2 FRAME_SIZE/64


static const float offset[8] = {0, 4, 8, 12, 16, 20, 24, 28};

#endif

void
drc_apply(HANDLE_DRC drc, double *coef, int ch
#ifdef CT_SBR
          ,int bSbrPresent
          ,int bT /* window type*/
#endif
          )
{
  int band, bin, bottom, top;
  double norm, factor;
  DRC_Info *p = &drc->drc_info[ch];


#ifdef CT_SBR
  int startSample,stopSample, bottomQMF,i,j;
  float previousFactors[64];
   
  memcpy(previousFactors,drcFactorVector[ch][2*NUM_QMF_SUBSAMPLES - 1],64*sizeof(float));
#endif  

  if (!drc->enable) return;

  /*  bSbrPresent = 0;  */
  

  /* If program reference normalization is done in the digital domain,
  modify factor to perform normalization.  prog_ref_level can
  alternatively be passed to the system for modification of the level in
  the analog domain.  Analog level modification avoids problems with
  reduced DAC SNR (if signal is attenuated) or clipping (if signal is
  boosted) */
  if (drc->digital_norm) {
    norm = pow(0.5, ((drc->target_ref_level - drc->prog_ref_level) / 24.0));
  } else {
    norm = 1.0;
  }
  
  bottom = 0;
  for (band=0; band<p->num_bands; band++) {
    /* apply the scaled dynamic range control words to factor.
     * if scaling drc_hi (or drc_lo), or control word drc_mag is 0.0
     * then there is no dynamic range compression
     *
     * if sgn is 
     *	1 then gain is < 1
     *	0 then gain is > 1
     */
    if (p->drc_sgn[band]) {
      /* compress */
      factor = norm * pow(2.0, ((-drc->hi * p->drc_mag[band]) / 24.0));
    } else {
      /* boost */
      factor = norm * pow(2.0, ((drc->lo * p->drc_mag[band]) / 24.0));
    }
    
    if (debug['D'] && drc->enable) {
      DebugPrintf(0, "Chn %d Bnd %d (%4d-%4d) has drc %f %f %f\n",
                  ch, band, bottom, (p->band_top[band]+1)*DRC_BAND_MULT - 1, norm, factor/norm, factor);
    }

    /* apply factor to spectral lines
     *  short blocks must take care that bands fall on 
     *  block boundaries!
     */
    top = (p->band_top[band]+1)*DRC_BAND_MULT;

#ifdef CT_SBR    
    if(bT != EIGHT_SHORT_SEQUENCE){
      bottom = 32*(int)(bottom/32);
      top    = 32*(int)(top/32);
    } else {
      bottom = 4*(int)(bottom/4);
      top    = 4*(int)(top/4);
    }
#endif

#ifndef QMF_BASED_DRC
    for (bin=bottom; bin<top; bin++) {
      coef[bin] *= factor;
    }
#endif

#ifdef CT_SBR
    if(bSbrPresent){
      /* LONG_WINDOW is a variable indicating if long blocks are used 
         for the AAC. If short blocks are used LONG_BLOCKS is zero.*/
      
      if (bT != EIGHT_SHORT_SEQUENCE) {

        bottomQMF = bottom*32/FRAME_SIZE;
        
        for (j = -NUM_QMF_SUBSAMPLES_2; j < NUM_QMF_SUBSAMPLES; j++){	

          float alphaValue = 0;

          if(j+NUM_QMF_SUBSAMPLES_2 < NUM_QMF_SUBSAMPLES){
            if(p->drc_interp_scheme == 0){
              float k = 1.0f/ ((float) NUM_QMF_SUBSAMPLES);
              alphaValue = (j+NUM_QMF_SUBSAMPLES_2)*k;
            } else {
              if (j+NUM_QMF_SUBSAMPLES_2 >= offset[p->drc_interp_scheme - 1])
                alphaValue = 1;
            }
          } else
            alphaValue = 1;

          
          for (i = bottomQMF ; i < 64 ; i++) {
            drcFactorVector[ch][NUM_QMF_SUBSAMPLES + j ][i] = alphaValue*factor + (1 - alphaValue)*previousFactors[i];
          }
        }
      } else {
        /* StartSample and stopSample indicate the borders in QMF subsamples for the
           current DRC data. They are derived by first removing all integer multiples 			
           of the short window length. Since there are always 32 QMF analysis 			
           subbands for FRAME_SIZE input frame size, the number of QMF subsamples 			
           is FRAME_SIZE/32 (NUM_QMF_SUBSAMPLES) for every frame. Hence the number 			
           of QMF subsamples for a short window is (NUM_QMF_SUBSAMPLES)/8.			
           bottomQMF is calculated from the remainder when the integer multiples of  			
           the short window length has been removed. This index to an MDCT line is 			
           mapped to the corresponding subband in the QMF by multiplying by  			
           32/(FRAME_SIZE/8).					
           If the the borders between two short windows are crossed, i.e. startSample 			
           is located in short window n-1 and stopSample is located in short window 			
           n, the bottomQMF needs to be set to zero when the border between the 			
           frames is crossed.
        */
		    
        /* startSample is truncated to the nearest corresponding start subsample in 			
           the QMF of the short window bottom is present in:*/
        startSample = floor((float) bottom/(FRAME_SIZE/8.0f))*(NUM_QMF_SUBSAMPLES)/8;
		    
        /* stopSample is rounded upwards to the nearest corresponding stop subsample 			
           in the QMF of the short window top is present in.*/
        stopSample = ceil ((float) top/(FRAME_SIZE/8.0f))*(NUM_QMF_SUBSAMPLES)/8;

        bottomQMF  = (bottom%(FRAME_SIZE/8))*32/(FRAME_SIZE/8);
        for (j = startSample; j < stopSample; j++){	
          if(j > startSample && j%4 == 0){
            bottomQMF = 0;
          }
          for (i = bottomQMF ; i < 64 ; i++) {
            drcFactorVector[ch][NUM_QMF_SUBSAMPLES + j ][i] = factor;
          }
        }
      }
    }
#endif    

    bottom = top;
  }

#ifdef CT_SBR
  if(bT != EIGHT_SHORT_SEQUENCE){                   							
    p->drc_interp_scheme_prev = p->drc_interp_scheme;
  }
  else{
    p->drc_interp_scheme_prev = 8;
  }
#endif
}
#endif	/* define DRC */
