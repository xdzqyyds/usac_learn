/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-30

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

Copyright (c) 1999.

**********************************************************************/

/* HP 980211   adapted to new sam_scale.h by BG */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "tns3.h"                /* structs */

#include "sam_encode.h"
#include "sam_model.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_LAYER 100

static int encode_cband_si(
  int model_index[][8][32],
  int start_cband[],
  int end_cband[],
  int cband_si_type[],
  int g,
  int i_ch,
  int nch);

static int encode_scfband_si(
  int scf[][8][MAX_SCFAC_BANDS],
  int start_sfb[],
  int end_sfb[],
  int max_scf[],
  int scf_model[],
  int stereo_mode,
  /*int *stereo_info[8],*/
	int stereo_info[8][MAX_SCFAC_BANDS], /* SAMSUNG_2005-09-30 */
  int stereo_si_coded[8][MAX_SCFAC_BANDS],
  int g,
  int i_ch,
  int nch);

static int select_freq1 (
  int model_index,
  int bpl,
  int coded_samp_bit,
  int available_len);

static int select_freq0 (
  int model_index,
  int bpl,
  int enc_msb_vec,
  int samp_pos,
  int enc_csb_vec,
  int available_len);

static int encode_spectra(
  int *sample[][8],
  int s_reg,
  int e_reg,
  int s_freq[],
  int e_freq[],
  int min_bpl,
  int available_len,
  int model_index[][8][32],
  int *cur_bpl[][8],
  int *coded_samp_bit[][8],
  int *sign_coded[][8],
  int *enc_sign_vec[][8],
  int *enc_vec[][8],
  int sba_mode,
  int i_ch,
  int nch);

#ifdef	__cplusplus
}
#endif

#define MAX_CBAND0_SI_LEN 11
#define HEADER_LEN_OFFSET 7

static int cband_si_tbl[24][24] = 
{ 
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

  { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  { 2, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
  { 2, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},

  { 5, 5, 5, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
  { 5, 5, 5, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},

  { 9, 9, 9, 9, 9,10,10,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 9, 9, 9, 9, 9,10,10,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12},

  {13,13,13,13,13,14,14,15,15,16,16,16,16,17,17,17,17,17,17,17,17,17,17,17},
  {13,13,13,13,13,14,14,15,15,16,16,16,16,17,17,17,17,17,17,17,17,17,17,17},

  {18,18,18,18,18,19,19,20,20,21,21,21,21,22,22,22,22,22,22,22,22,22,22,22},

  {23,23,23,23,23,23,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,24,24},

  {25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26},

  {27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27},

  {28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28},

  {29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29},

  {30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30},

  {31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31},
}; 
  
static int max_csi_acwlen_tbl[32] = 
{ 
  6, 5, 6, 5,  6,   6,  5, 6,  5,   6,  5,  6, 8,   6, 5, 6, 8, 9,
  6, 5, 6, 8, 10,   8, 10, 9, 10,  10, 12, 12, 12,  12
}; 

static int cband_si_cbook_tbl[32] = 
{ 
  0, 1,   0, 1, 2,   0, 1, 2, 3,   0, 1, 2, 4,   0, 1, 2, 4, 5,
    0, 1, 2, 4, 6,   4, 6,   5, 6,  6,  6,  6,  6,  6
}; 

static int min_freq[16] = { 
   16384,  8192,  4096,  2048,  1024,   512,   256,    128,
      64,    32,    16,     8,     4,     2,     1,      0
};

static int  sam_scale_bits_enc[MAX_LAYER];
static int  sampling_frequency;

void
sam_scale_bits_init_enc(int sampling_rate, int block_size_samples)
{
  int i, layer, layer_bit_rate;
  int average_layer_bits;
  static int init_pns_noise_nrg_model = 1; 

  /* calculate the avaerage amount of bits available
     for the scalability layer of one block per channel */

  for (layer=0; layer<=48; layer++) {
    layer_bit_rate = (layer+16) * 1000;
    average_layer_bits = (int)((double)layer_bit_rate*(double)block_size_samples/(double)sampling_rate);
    sam_scale_bits_enc[layer] = (average_layer_bits / 8) * 8;
  }
  sampling_frequency = sampling_rate;
/*
printf("FS: %d\n", sampling_frequency);
*/

  if (init_pns_noise_nrg_model) {
     AModelNoiseNrg[0] = 16384 - 32;
     for (i=1; i<512; i++) 
       AModelNoiseNrg[i] = AModelNoiseNrg[i-1] - 32;
     init_pns_noise_nrg_model = 0;
  }
}


int sam_encode_bsac
(
 int target_bitrate,
 int ch_type, /* SAMSUNG_2005-09-30 : channel speaker type */
 int windowSequence,
 int windowShape,
 int *sample[][8],
 int scalefactors[][8][MAX_SCFAC_BANDS],
 int maxSfb,
 int num_window_groups,
 int window_group_length[],
 int swb_offset[][MAX_SCFAC_BANDS],
 int top_band,
 int ModelIndex[][8][32],
 int stereo_mode,
 /*int stereo_info_buf[],*/
 int stereo_info[][MAX_SCFAC_BANDS], /* SAMSUNG_2005-09-30 */
 int abits,
 int i_ch,
 int nch,
 int mc_enabled, /* SAMSUNG_2005-09-30 : multichannel? */
/* 20070530 SBR */
 int sbrenable,
 SBR_CHAN *sbrChan,
 int n_Elm,
 int i_el,
 int *bitCount
 )
{
  int    i, ch, g, m, k;
  int    top_layer;
  int    sfb, qband, cband, reg;
  int    baseSfb;
  int    base_snf;
  int    base_band;
  int    sba_mode;
  int    layer_reg[MAX_LAYER];
  int    layer_max_freq[MAX_LAYER];
  int    layer_max_cband[MAX_LAYER];
  int    layer_max_qband[MAX_LAYER];
  int    last_freq[2][MAX_LAYER];
  int    layer_bit_flush[MAX_LAYER];
  int    layer_buf_offset[MAX_LAYER];
  int    layer_si_maxlen[MAX_LAYER];
  int    init_max_freq[8];
  int    layer_extra_len[MAX_LAYER];
  int    layer_cw_offset[MAX_LAYER];

  int    start_freq[8];
  int    start_qband[8];
  int    start_cband[8];
  int    end_freq[8];
  int    end_cband[8];
  int    end_qband[8];
  int    s_freq[8];
  int    e_freq[8];

  int    cband_si_type[2];
  int    maxScf;
  int    scf_model0[2];
  int    scf_model1[2];
  int    max_sfb_si_len[2];
  int    cw_len;
  int    max_model_index0;
  int    max_model_index1;
  int    min;
  int    layer;
  int    slayer_size;
  int    si_offset;
  int    est_total_len;
  int    available_len = 0;
  /*int    *stereo_info[8];*/
  int    used_bits;
  int    cur_snf_buf[2][1024];
  int    *cur_snf[2][8];
  int    band_snf[2][8][32];
  int    maxScalefactors[2];
  int    sign_is_coded_buf[2][1024];
  int    *sign_is_coded[2][8];
  int    *coded_samp_bit[2][8];
  int    coded_samp_bit_buf[2][1024];
  int    *enc_sign_vec[2][8];
  int    enc_sign_vec_buf[2][256];
  int    *enc_vec[2][8];
  int    enc_vec_buf[2][256];
  int    stereo_si_coded[8][MAX_SCFAC_BANDS];
  int    escale_bits[MAX_LAYER];
  int    estimated_bits;
  double minf;
  int    code_len;
  int    flushed;
  int    header_length;


  /* ***************************************************** */
  /*                  Initialize variables                 */
  /* ***************************************************** */
  for(ch = 0; ch < nch; ch++) {
    int s;

    s = 0;
    for(g = 0; g < num_window_groups; g++) { 
      sign_is_coded[ch][g] = &(sign_is_coded_buf[ch][1024*s/8]);
      coded_samp_bit[ch][g] = &(coded_samp_bit_buf[ch][1024*s/8]);
      enc_sign_vec[ch][g] = &(enc_sign_vec_buf[ch][(1024*s/8)/4]);
      enc_vec[ch][g] = &(enc_vec_buf[ch][(1024*s/8)/4]);
      cur_snf[ch][g] = &(cur_snf_buf[ch][1024*s/8]);
      /*stereo_info[g] = &(stereo_info_buf[g*maxSfb]);*/
      s += window_group_length[g];

      for (i=0; i<MAX_SCFAC_BANDS; i++)
        stereo_si_coded[g][i] = 0;
    }

    for (i=0; i<1024; i++) {
      sign_is_coded_buf[ch][i] = 0;
      coded_samp_bit_buf[ch][i] = 0;
      cur_snf_buf[ch][i] = 0;
    }

    for (i=0; i<256; i++) {
      enc_sign_vec_buf[ch][i] = 0;
      enc_vec_buf[ch][i] = 0;
    }
  }

  for(ch = i_ch; ch < nch; ch++) {
    for(g = 0; g < num_window_groups; g++) {
      for(cband = 0; cband < 32; cband++) {
        if(ModelIndex[ch][g][cband] >= 15)
          band_snf[ch][g][cband] = ModelIndex[ch][g][cband] - 7;
        else
          band_snf[ch][g][cband] = (ModelIndex[ch][g][cband]+1) / 2;
      }
    }
  }

  top_layer = (target_bitrate / 1000) - 16;

  for (i = 0; i <= top_layer; i++) {
    escale_bits[i] = sam_scale_bits_enc[i] * (nch - i_ch);
  }


	ch=0; /* SAMSUNG_2005-09-30 : bug fix */
  /* Initialize */
/* 20070530 SBR */
  if(sbrenable)
	base_band = 704/32; 
  else
	base_band = 320/32; /* SAMSUNG_2005-09-30 : temporary*/
  if(windowSequence == 2) {
      slayer_size = 0;
      for(g = 0; g < num_window_groups; g++) {

        init_max_freq[g] = base_band * window_group_length[g] * 4;
        last_freq[ch][g] = swb_offset[g][maxSfb];

        switch(sampling_frequency) {
           case 48000: case 44100:
             if ((init_max_freq[g]%32) >= 16)
             init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 20;
             else if ((init_max_freq[g]%32) >= 4)
             init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 8;
             break;
           case 32000: case 24000: case 22050:
             init_max_freq[g] = (init_max_freq[g] / 16) * 16;
             break;
           case 16000: case 12000: case 11025:
             init_max_freq[g] = (init_max_freq[g] / 32) * 32;
             break;
           default :
             init_max_freq[g] = (init_max_freq[g] / 64) * 64;
             break;
        }

        if (init_max_freq[g] > last_freq[ch][g])
          init_max_freq[g] = last_freq[ch][g];
        end_freq[g] = init_max_freq[g];
        end_cband[g] = (end_freq[g] + 31) / 32;
        slayer_size += (init_max_freq[g]+31)/32;
      }

      layer = 0;
      for(g = 0; g < num_window_groups; g++)
      for (cband = 1; cband <= end_cband[g]; layer++, cband++) {

        layer_reg[layer] = g;
        layer_max_freq[layer] = cband * 32;
        if (layer_max_freq[layer] > init_max_freq[g])
          layer_max_freq[layer] = init_max_freq[g];

        layer_max_cband[layer] = cband;
      }
  
      layer = slayer_size;
      for(g = 0; g < num_window_groups; g++) {
        for (i=0; i<window_group_length[g]; i++) { 
          layer_reg[layer] = g;
          layer++;
        }
      }
  

      for (layer=slayer_size+8; layer<MAX_LAYER; layer++) 
        layer_reg[layer] = layer_reg[layer-8];

      for (layer = slayer_size; layer <MAX_LAYER; layer++) {

        reg = layer_reg[layer];

        switch(sampling_frequency) {
        case 48000: case 44100:
          end_freq[reg] += 8;
          if(end_freq[reg]%32 != 0)
            end_freq[reg] += 4;
          break;

        case 32000: case 24000: case 22050:
          end_freq[reg] += 16;
          break;

        case 16000: case 12000: case 11025: 
          end_freq[reg] += 32;
          break;

        default:
          end_freq[reg] += 64;
          break;
        }

        if (end_freq[reg] > last_freq[ch][reg] )
          end_freq[reg] = last_freq[ch][reg];

        layer_max_freq[layer] = end_freq[reg];

        layer_max_cband[layer] = (layer_max_freq[layer]+31)/32;
      }


      for (layer = 0; layer < (top_layer+slayer_size-1); layer++) {
        if (layer_max_cband[layer] != layer_max_cband[layer+1] ||
          layer_reg[layer] != layer_reg[layer+1] )
          layer_bit_flush[layer] = 1;
        else
          layer_bit_flush[layer] = 0;
      }
      for ( ; layer < MAX_LAYER; layer++)
          layer_bit_flush[layer] = 1;

      for (layer = 0; layer < MAX_LAYER; layer++) {
        int  end_layer;


        end_layer = layer;
        while ( !layer_bit_flush[end_layer] ) 
          end_layer++;

        reg = layer_reg[layer];
        for (sfb=0; sfb<top_band; sfb++) {
          if (layer_max_freq[end_layer] <= swb_offset[reg][sfb+1]) {
            layer_max_qband[layer] = sfb+1;
            break;
          }
        }
        if (layer_max_qband[layer] > maxSfb)
          layer_max_qband[layer] = maxSfb;
      }
      for(g = 0; g < num_window_groups; g++)
        end_cband[g] = (swb_offset[g][maxSfb]+31)/32;

      for (layer = 0; layer < slayer_size; layer++) {
        g = layer_reg[layer];
        end_qband[g] = layer_max_qband[layer];
      }
  } else {
      g = 0;

      for(layer = 0; layer < MAX_LAYER; layer++)
        layer_reg[layer] = 0;

      last_freq[ch][g] = swb_offset[g][maxSfb];

      /************************************************/
      /* calculate layer_max_cband and layer_max_freq */
      /************************************************/
      init_max_freq[g] = base_band * 32;

      if (init_max_freq[g] > last_freq[ch][g])
        init_max_freq[g] = last_freq[ch][g];
      slayer_size = (init_max_freq[g]+31)/32;

      cband = 1;
      for (layer = 0; layer < slayer_size; layer++, cband++) {

        layer_max_freq[layer] = cband * 32;
        if (layer_max_freq[layer] > init_max_freq[g])
          layer_max_freq[layer] = init_max_freq[g];

        layer_max_cband[layer] = cband;
      }

      for (layer = slayer_size; layer < MAX_LAYER; layer++) {

        switch(sampling_frequency) {
        case 48000: case 44100:
          layer_max_freq[layer] = layer_max_freq[layer-1] + 8;
          if(layer_max_freq[layer]%32 != 0) layer_max_freq[layer] += 4;
          break;

        case 32000: case 24000: case 22050:
          layer_max_freq[layer] = layer_max_freq[layer-1] + 16;
          break;

        case 16000: case 12000: case 11025:
          layer_max_freq[layer] = layer_max_freq[layer-1] + 32;
          break;

        default:
          layer_max_freq[layer] = layer_max_freq[layer-1] + 64;
          break;
        }

        if (layer_max_freq[layer] > last_freq[ch][g])
          layer_max_freq[layer] = last_freq[ch][g];

        layer_max_cband[layer] = (layer_max_freq[layer]+31)/32;
      }

      /*****************************/
      /* calculate layer_bit_flush */
      /*****************************/
      for (layer = 0; layer < (top_layer+slayer_size-1); layer++) { 
        if (layer_max_cband[layer] != layer_max_cband[layer+1])
          layer_bit_flush[layer] = 1;
        else
          layer_bit_flush[layer] = 0;
      }
      for ( ; layer < MAX_LAYER; layer++)
          layer_bit_flush[layer] = 1;

      /*****************************/
      /* calculate layer_max_qband */
      /*****************************/
      for (layer = 0; layer < MAX_LAYER; layer++) {
        int  end_layer;

        end_layer = layer;
        while ( !layer_bit_flush[end_layer] ) 
          end_layer++;

        for (sfb=0; sfb < top_band; sfb++) {
          if (layer_max_freq[end_layer] <= swb_offset[0][sfb+1]) {
            layer_max_qband[layer] = sfb+1;
            break;
          }
        }
        if (layer_max_qband[layer] > maxSfb)
          layer_max_qband[layer] = maxSfb;
      }
      end_cband[g] = (swb_offset[0][maxSfb]+31)  / 32;
      end_qband[g] = layer_max_qband[slayer_size-1];
  }

  sam_init_bs();

  /***********************************/
  /* To find the maximum scalefactor */
  /***********************************/
  for(ch = 0; ch < nch; ch++) {
    maxScalefactors[ch] = 0;
    for(g = 0; g < num_window_groups; g++) 
    for(sfb = 0; sfb < maxSfb; sfb++) {
      if (maxScalefactors[ch] < scalefactors[ch][g][sfb]) 
        maxScalefactors[ch] = scalefactors[ch][g][sfb];
    }
  }

  /*********************************************/
  /* calculate the information for bsac_header */
  /*********************************************/
  for(ch = i_ch; ch < nch; ch++) {

    /***************************************************************/
    /* Arithmetic Model Table index for the coding band infomation */
    /***************************************************************/

    /* Arithmetic Model Table index for the coding band infomation */
    max_model_index0 = 0;
    for(g = 0; g < num_window_groups; g++) {
      if (max_model_index0 < ModelIndex[ch][g][0])
        max_model_index0 = ModelIndex[ch][g][0];
    }

    max_model_index1 = 0;
    for(g = 0; g < num_window_groups; g++) {
    for(cband = 1; cband < end_cband[g]; cband++) 
      if (max_model_index1 < ModelIndex[ch][g][cband])
        max_model_index1 = ModelIndex[ch][g][cband];
    }

    if (max_model_index0 < max_model_index1)
      max_model_index0 = max_model_index1;

    cband_si_type[ch] = cband_si_tbl[max_model_index0][max_model_index1]; 

    if (maxSfb<=0) continue;
    /****************************************************/
    /* scalefactor band side information for BASE layer */
    /****************************************************/
    maxScf = maxScalefactors[ch];
    baseSfb = layer_max_qband[slayer_size-1];

    min = 1000;
    for(g = 0; g < num_window_groups; g++)
    for(sfb = 0; sfb < end_qband[g]; sfb++) {
      if (scalefactors[ch][g][sfb] < 0) continue; 
      if (min > scalefactors[ch][g][sfb]) min = scalefactors[ch][g][sfb];
    }

    m = maxScf - min;

    if (m>31)       scf_model0[ch] = 7;
    else if (m>15)  scf_model0[ch] = 5;
    else if (m>7)   scf_model0[ch] = 3;
    else if (m>3)   scf_model0[ch] = 2;
    else if (m>0)   scf_model0[ch] = 1;
    else            scf_model0[ch] = 0;

    minf = 10000;
    for (i=scf_model0[ch]; i<8; i++) {
      double est_scf_len = 0;

      for(g = 0; g < num_window_groups; g++)
        for(sfb = 0; sfb < end_qband[g]; sfb++) {
          m = maxScf - scalefactors[ch][g][sfb];
          if (m==0)
            est_scf_len -= log(1.-AModelScf[i][m]/16384.)/log(2.);
          else
            est_scf_len -= log((AModelScf[i][m-1]-AModelScf[i][m])/16384.)/log(2.);
        }
      if (est_scf_len < minf) {
        scf_model0[ch] = i;
        minf = est_scf_len;
      }
    }


    /************************************************************/
    /* scalefactor band side information for ENHANCEMENT layer */
    /************************************************************/
    min = 1000;
    for(g = 0; g < num_window_groups; g++)
    for(sfb = end_qband[g]; sfb < maxSfb; sfb++) {
      if (scalefactors[ch][g][sfb] < 0) continue; 
      if (min > scalefactors[ch][g][sfb]) min = scalefactors[ch][g][sfb];
    }
    m = maxScf - min;

    if (m>31)     scf_model1[ch] = 7;
    else if (m>15)  scf_model1[ch] = 5;
    else if (m>7)   scf_model1[ch] = 3;
    else if (m>3)   scf_model1[ch] = 2;
    else if (m>0)   scf_model1[ch] = 1;
    else         scf_model1[ch] = 0;

    minf = 10000;
    for (i=scf_model1[ch]; i<8; i++) {
      double est_scf_len = 0;

      for(g = 0; g < num_window_groups; g++)
        for(sfb = end_qband[g]; sfb < maxSfb; sfb++) {
          m = maxScalefactors[ch] - scalefactors[ch][g][sfb];
          if (m==0)
            est_scf_len -= log(1.-AModelScf[i][m]/16384.)/log(2.);
          else
            est_scf_len -= log((AModelScf[i][m-1]-AModelScf[i][m])/16384.)/log(2.);
        }
      if (est_scf_len < minf) {
        scf_model1[ch] = i;
        minf = est_scf_len;
      }
    }

    /********************************************/
    /* scalefactor data length side information */
    /********************************************/
    max_sfb_si_len[ch] = 0;
    for(g = 0; g < num_window_groups; g++) {
      int	difscf;

      for(sfb = 0; sfb < end_qband[g]; sfb++) {
        m = maxScalefactors[ch] - scalefactors[ch][g][sfb];
        if (m==0) {
          cw_len = (int)(ceil(- log(1.-AModelScf[scf_model0[ch]][m]/16384.)/log(2.)));
        } else {
          difscf = AModelScf[scf_model0[ch]][m-1]-AModelScf[scf_model0[ch]][m];
          cw_len = (int)(ceil(- log(difscf/16384.)/log(2.)));
        }
        if (cw_len > max_sfb_si_len[ch])
          max_sfb_si_len[ch] = cw_len;
      }
      for(sfb = end_qband[g]; sfb < maxSfb; sfb++) {
        m = maxScalefactors[ch] - scalefactors[ch][g][sfb];
        if (m==0) {
          cw_len = (int)(ceil(- log(1.-AModelScf[scf_model1[ch]][m]/16384.)/log(2.)));
        } else {
          difscf = AModelScf[scf_model1[ch]][m-1]-AModelScf[scf_model1[ch]][m];
          cw_len = (int)(ceil(- log(difscf/16384.)/log(2.)));
        }
        if (cw_len > max_sfb_si_len[ch])
          max_sfb_si_len[ch] = cw_len;
        }
      }

      /*** PLEASE ADD M/S stereo & Intensity Stereo & PNS length info ***/
      if(ch == 0) {
        switch(stereo_mode) {
        case 1:
          max_sfb_si_len[ch]++;
          break;
        case 3:
          break;
      }
    }
    max_sfb_si_len[ch] -= 5;
    if(max_sfb_si_len[ch] < 0) max_sfb_si_len[ch] = 0;
    if(max_sfb_si_len[ch] > 15) max_sfb_si_len[ch] = 15;
  }


  /* frame length */
  used_bits = sam_putbits2bs(0, 11);

  /* ***************************************************** */
  /*                  bsac_header()                        */
  /* ***************************************************** */
	
	if(i_el)	
		used_bits += sam_putbits2bs(ch_type, 4); /* SAMSUNG_2005-09-30 : channel_configuration_index */

  /* header length */
  used_bits += sam_putbits2bs(0, 4);

  /* sba_mode flag */
  sba_mode = 0;
  used_bits += sam_putbits2bs(sba_mode, 1);

  /* top layer index */
  used_bits += sam_putbits2bs(top_layer, 6);

  base_snf = 1;
  /* base snf */
  used_bits += sam_putbits2bs(base_snf-1, 2);

  /* To find the maximum scalefactor */
  for(ch = i_ch; ch < nch; ch++) {
    used_bits += sam_putbits2bs(maxScalefactors[ch], 8);
  }

  /* base band */
  used_bits += sam_putbits2bs(base_band, 5);

  for(ch = i_ch; ch < nch; ch++) {
    /* Arithmetic Model Table index for the coding band infomation */
    used_bits += sam_putbits2bs(cband_si_type[ch], 5);

    /* scalefactor band side information for BASE layer */
    used_bits += sam_putbits2bs(scf_model0[ch], 3);

    /* scalefactor band side information for ENHANCEMENT layer */
    used_bits += sam_putbits2bs(scf_model1[ch], 3);

    /* scalefactor band side information maximum length */
    used_bits += sam_putbits2bs(max_sfb_si_len[ch], 4);
  }

  /* ***************************************************** */
  /*                  general_info()                       */
  /* ***************************************************** */
  used_bits += sam_putbits2bs(0x00, 1);

  used_bits += sam_putbits2bs((int)windowSequence, 2);

  used_bits += sam_putbits2bs((int)windowShape, 1);

  if(windowSequence == EIGHT_SHORT_SEQUENCE) {
    /*int groupInfo, b;*/
    used_bits += sam_putbits2bs(maxSfb, 4);

    /*for(b = 0; b < num_window_groups; b++) {
      groupInfo = 0;
      used_bits += sam_putbits2bs(groupInfo, 1);
      for(i = 1; i < window_group_length[b]; i++) {
        groupInfo = 1;
        used_bits +=  sam_putbits2bs(groupInfo, 1);
      }
    }*/
		/* ==> SAMSUNG_2005-09-30 : buf fix */
		for (i=1; i<window_group_length[0]; i++)
			used_bits += sam_putbits2bs(1, 1);
		for(g = 1; g < num_window_groups; g++) {
			used_bits += sam_putbits2bs(0, 1);
			for (i=1; i<window_group_length[g]; i++)
				used_bits += sam_putbits2bs(1, 1);
		}
		/*~SAMSUNG_2005-09-30 */ 

  } else {
    used_bits += sam_putbits2bs(maxSfb, 6);
  }

  /* pns_data_present */
  used_bits += sam_putbits2bs(0x00, 1);

  /* stereo mode */
  if(i_ch == 0 && nch == 2) {
    used_bits += sam_putbits2bs(stereo_mode, 2);
  }


  for(ch = i_ch; ch < nch; ch++) {
    used_bits += sam_putbits2bs(0, 1);  /* tns_data_present : 1 */
    used_bits += sam_putbits2bs(0, 1);  /* ltp_data_present : 1 */
  }

  /***** END of general_info () *******/


  /*********** byte align *********/
  if(used_bits % 8) {
    k = 8 - (used_bits % 8);
    used_bits += sam_putbits2bs(0, k);
  }

  header_length = used_bits - HEADER_LEN_OFFSET * 8;
  if ( (header_length/8) > 15 ) header_length = 15*8;    

  if(i_el) /* SAMSUNG_2005-09-30 : for extension part */
		sam_frame_length_rewrite2bs(header_length, 15, 4); 
	else
		sam_frame_length_rewrite2bs(header_length, 11, 4); 

  if(maxSfb == 0) {
    if(nch == 1) return used_bits;
    if(nch == 2 && maxSfb == 0) return used_bits;
  }

  /* ##################################################### */
  /*                   BSAC MAIN ROUTINE                    */
  /* ##################################################### */

  /* ***************************************************** */
  /*                  BSAC_layer_stream()                  */
  /* ***************************************************** */

  for (layer=0; layer<(top_layer+slayer_size); layer++) 
    layer_si_maxlen[layer] = 0;
  
  for(g = 0; g < num_window_groups; g++)
    end_cband[g] = end_qband[g] = 0;

  for (layer=0; layer<(top_layer+slayer_size); layer++) {

    g = layer_reg[layer];
    start_qband[g] = end_qband[g];
    start_cband[g] = end_cband[g];
    end_qband[g] = layer_max_qband[layer];
    end_cband[g] = layer_max_cband[layer];

    for(ch = i_ch; ch < nch; ch++) {
      /* coding band side infomation  */
      for (cband = start_cband[g]; cband < end_cband[g]; cband++) {
        if (cband <= 0) 
          layer_si_maxlen[layer] += MAX_CBAND0_SI_LEN;
        else 
          layer_si_maxlen[layer] += max_csi_acwlen_tbl[cband_si_type[ch]];  
        }
    
      /* side infomation : scalefactor */
      for(qband = start_qband[g]; qband < end_qband[g]; qband++) {
        layer_si_maxlen[layer] += max_sfb_si_len[ch] + 5;
      }
    }
  }


  if(abits < escale_bits[top_layer]) abits = escale_bits[top_layer];
  for (layer=top_layer+slayer_size; layer< MAX_LAYER; layer++)
    layer_buf_offset[layer] = abits;

  si_offset = abits - layer_si_maxlen[top_layer+slayer_size-1];
  for (layer=(top_layer+slayer_size-1); layer>=slayer_size; layer--) {
    if ( si_offset < escale_bits[layer-slayer_size] )
      layer_buf_offset[layer] = si_offset;
    else
      layer_buf_offset[layer] = escale_bits[layer-slayer_size];
    si_offset = layer_buf_offset[layer] - layer_si_maxlen[layer-1];
  }
  for (; layer>0; layer--) {
    layer_buf_offset[layer] = si_offset;
    si_offset = layer_buf_offset[layer] - layer_si_maxlen[layer-1];
  }
  layer_buf_offset[0] = used_bits;

  if (used_bits > si_offset) {
    si_offset = used_bits - si_offset;
    for (layer=(top_layer+slayer_size-1); layer>=slayer_size; layer--) {
      cw_len  = layer_buf_offset[layer+1] - layer_buf_offset[layer];
      cw_len -= layer_si_maxlen[layer];
      if (cw_len >= si_offset) { 
        cw_len = si_offset; 
        si_offset = 0;
      } else
        si_offset -= cw_len;

      for (i=1; i<=layer; i++)
        layer_buf_offset[i] += cw_len;

      if (si_offset == 0) break;
    }
  } else {
    si_offset = si_offset - used_bits;
    for (layer = 1; layer < slayer_size; layer++) {
      layer_buf_offset[layer] = layer_buf_offset[layer-1] + layer_si_maxlen[layer-1] + si_offset/slayer_size;
      if ( layer <= (si_offset%slayer_size) )
        layer_buf_offset[layer]++;
    }
  }

  estimated_bits = used_bits;

  for(i = 0; i < MAX_LAYER; i++) {
    layer_extra_len[i] = 0;
    layer_cw_offset[i] = 0;
  }

  est_total_len = used_bits;

  for(g = 0; g < num_window_groups; g++) {
    start_qband[g] = 0;
    start_cband[g] = 0;
    start_freq[g] = 0;
    end_qband[g] = 0;
    end_cband[g] = 0;
    end_freq[g] = 0;
  }

  for(g = 0; g < num_window_groups; g++)
    s_freq[g] = 0;

  if(windowSequence == 2) {
    for(layer = 0; layer < slayer_size; layer++) {
      g = layer_reg[layer];
      e_freq[g] = layer_max_freq[layer];
    }
  }

  code_len = 0;
  flushed = 0;
  if(!sba_mode) {
    sam_initArEncode(0);
    sam_setArEncode(0);
    available_len = 0;
  }

  for (layer = 0; layer < top_layer+slayer_size; layer++) {
    int min_snf;
    int *scf_model;

    g = layer_reg[layer];

    start_freq[g]  = end_freq[g];
    start_qband[g] = end_qband[g];
    start_cband[g] = end_cband[g];

    end_qband[g] = layer_max_qband[layer];
    end_cband[g] = layer_max_cband[layer];
    end_freq[g]  = layer_max_freq[layer];

    for(ch = i_ch; ch < nch; ch++) {
      for(i = start_freq[g]; i < end_freq[g]; i++)  
        cur_snf[ch][g][i] = band_snf[ch][g][i/32];
    }

    if(windowSequence == 2) {
      if(layer >= slayer_size) {
        g = layer_reg[layer];
        e_freq[g] = layer_max_freq[layer];
      }
    } else {
      if(layer >= slayer_size) {
        e_freq[0] = layer_max_freq[layer];
      } else
        e_freq[0] = layer_max_freq[slayer_size-1];
    }

    if(sba_mode) {
      if (layer==0 || layer_bit_flush[layer-1]) {
        available_len = layer_buf_offset[layer+1]-layer_buf_offset[layer];
        sam_initArEncode(layer);
        sam_setArEncode(layer);
        sam_setWBitBufPos(layer_buf_offset[layer]);
        est_total_len++;
        available_len--;
      } else  {
        available_len += layer_buf_offset[layer+1]-layer_buf_offset[layer];
      }
    } else {
      available_len += layer_buf_offset[layer+1]-layer_buf_offset[layer];
    }

    est_total_len += available_len;

    if (sba_mode && available_len <= 0 ) {
      if (layer_bit_flush[layer]) flushed += sam_done_encoding();
      continue;
    }


    /* coding band side infomation  */
    cw_len =  encode_cband_si(ModelIndex, start_cband, end_cband, cband_si_type, g, i_ch, nch);
    code_len += cw_len;
    g = layer_reg[layer];
    available_len -= cw_len;
    
    /* side infomation : scalefactor */
    if (layer < slayer_size) 
      scf_model = (int *)scf_model0;
    else
      scf_model = (int *)scf_model1;

    cw_len =  encode_scfband_si(scalefactors, start_qband, 
              end_qband, maxScalefactors, scf_model, 
            stereo_mode, stereo_info, stereo_si_coded, g, i_ch, nch);

    code_len += cw_len;

    available_len -= cw_len;

    min_snf = layer < slayer_size ? base_snf : 1;

    /* Bit-Sliced Spectral Data Coding */
    cw_len = encode_spectra(sample, g, g+1, start_freq, end_freq, 
        min_snf, available_len, ModelIndex, cur_snf, coded_samp_bit, 
        sign_is_coded, enc_sign_vec, enc_vec, sba_mode, i_ch, nch);

    code_len += cw_len;
    available_len -= cw_len;

    if (!sba_mode && available_len > 0) {
      cw_len = encode_spectra(sample, 0, num_window_groups, s_freq, e_freq, 
          1, available_len, ModelIndex, cur_snf, coded_samp_bit, 
          sign_is_coded, enc_sign_vec, enc_vec, sba_mode, i_ch, nch);

      code_len += cw_len;
      available_len -= cw_len;
    }

    if (sba_mode && layer_bit_flush[layer]) {
      if (available_len > 0) {
        layer_extra_len[layer] = available_len;
        layer_cw_offset[layer] = sam_getWBitBufPos();
        sam_storeArEncode(layer);
      } else {
        flushed += sam_done_encoding();
      }
    }
    if (sba_mode && available_len < 0) {
       printf("Buffer Error : layer = %d %d\n", layer, available_len);
    }
    est_total_len -= available_len;

  } /* for (layer = 0; layer <= top_layer; layer++) */

  /* ***************************************************** */
  /*         Extra Encoding Process  for each layer        */
  /* ***************************************************** */
  for(g = 0; g < num_window_groups; g++)
    start_freq[g] = 0;

  if (windowSequence == 2) {
    for (layer = 0; layer < slayer_size; layer++) {
      g = layer_reg[layer];
      e_freq[g]  = layer_max_freq[layer];
    }
  }

  if(!sba_mode) layer_extra_len[top_layer+slayer_size-1] = abits;

  if(sba_mode)
  for (layer = 0; layer < top_layer+slayer_size; layer++) {

    available_len = layer_extra_len[layer];

    if (windowSequence == 2) {
      if(layer >= slayer_size) {
        g = layer_reg[layer];
        e_freq[g]  = layer_max_freq[layer];
      }
    } else {
      if (layer >= slayer_size) 
        e_freq[0] = layer_max_freq[layer];
      else
        e_freq[0] = layer_max_freq[slayer_size-1];
    }
    if (available_len <= 0 )  continue;

    if(sba_mode) {
      sam_setArEncode(layer);
      sam_setWBitBufPos(layer_cw_offset[layer]);
      est_total_len += available_len;
    }

    /* Extra Bit-Sliced Spectral Data Coding */
    cw_len = encode_spectra(sample, 0, num_window_groups, start_freq, e_freq, 1,
           available_len, ModelIndex, cur_snf, coded_samp_bit, 
           sign_is_coded, enc_sign_vec, enc_vec, sba_mode, i_ch, nch);

    code_len += cw_len;
    available_len -= cw_len;

    if(windowSequence == 2) {
      for(g = 0; g < num_window_groups; g++)
        s_freq[g] = e_freq[g];
    }

    for (k = layer+1; k < top_layer+slayer_size; k++) {

      if (available_len <= 0 ) break; 

      g = layer_reg[k];
      s_freq[g] = e_freq[g];
      e_freq[g] = layer_max_freq[k];

      cw_len = encode_spectra(sample, 0, num_window_groups, s_freq, e_freq, 1,
            available_len, ModelIndex, cur_snf, coded_samp_bit, 
            sign_is_coded, enc_sign_vec, enc_vec, sba_mode, i_ch, nch);
      code_len += cw_len;
      available_len -= cw_len;
    }

    if(sba_mode) flushed += sam_done_encoding();

    if (sba_mode && available_len < 24 && available_len > 0) {
      sam_putbits2bs(0, available_len);
    } else {
      est_total_len -= available_len;
    }

    if (available_len < 0)
       printf("BSAC:Extra Buffer Error : layer = %d %d\n", layer, available_len);

  }
  if(!sba_mode) flushed += sam_done_encoding();

	/* SAMSUNG_2005-09-30 */
	used_bits = sam_sstell(); 


	/* Byte align */
	if((used_bits % 8)) {
		used_bits += sam_putbits2bs(0x00, (8 - (used_bits % 8)));
	}


	/* SAMSUNG_2006-10-11 */

	/* First part */
	if( i_el == 0 ) {

/* 20070530 SBR */
		if( mc_enabled || sbrenable ) {


			/* Zero stuffing at the end of stereo part */
			used_bits += sam_putbits2bs(0x00000000, 32);

			/* Syncword */
			used_bits += sam_putbits2bs(0x0F, 4);

			if( sbrenable ) {
				used_bits += sam_putbits2bs( 0x00, 4 );
				used_bits += WriteSBRinBSAC(sbrChan, nch);
			}
			*bitCount = used_bits;
			/* Extension type */
			if( mc_enabled && sbrenable ) {
				used_bits += sam_putbits2bs( 0x0E, 4 );
				*bitCount = used_bits - 4;
			}
			else if( mc_enabled && !sbrenable ) {
			used_bits += sam_putbits2bs( 0x0F, 4 );
				*bitCount = used_bits - 8;
			}

		}
		else {
			*bitCount = used_bits;
		}


	}
	/* Extended part */
	else {

		int reBit;
                /*
		if(ch_type == LFE)
			printf("");
                */ /*disblaed useless code sps@2008-01-14 */
		if( sbrenable && mc_enabled ) {
			*bitCount = used_bits + 4;
			*bitCount += WriteSBRinBSAC(sbrChan, nch);

			reBit = *bitCount % 8;
			if( reBit ) {
				sam_putbits2bs( 0x00, 8 - reBit );
				*bitCount += reBit;
			}
			
			if( i_el != n_Elm ) {
				sam_putbits2bs(0x0E, 4);
			}

		}
		else if( !sbrenable && mc_enabled ) {
			/* used_bits: only include the BSAC bits */
			if( i_el == 1 )
				*bitCount = used_bits + 8;
			else
				*bitCount = used_bits + 4; /* Adding length of extension type flag */

			reBit = *bitCount % 8;
			if( reBit ) {
				sam_putbits2bs( 0x00, 8 - reBit );
				*bitCount += reBit;
			}

			if( i_el != n_Elm ) {
				sam_putbits2bs( 0x0F, 4 );
			}
		}

	}
	/* SAMSUNG_2006-10-11 */



  return used_bits;
}

/*--------------------------------------------------------------*/
/***************   coding band side infomation   ****************/
/*--------------------------------------------------------------*/
static int encode_cband_si(
  int model_index[][8][32],
  int start_cband[],
  int end_cband[],
  int cband_si_type[],
  int g,
  int i_ch,
  int nch)
{
  int ch;
  int cband;
  int  m;
  int si_cw_len;
  int cband_model;
	
  si_cw_len = 0;
  for(ch = i_ch; ch < nch; ch++) {
    if(start_cband[g] == 0) {
      cband_model = 7;
    } else {
      cband_model = cband_si_cbook_tbl[cband_si_type[ch]];
    }
    for (cband = start_cband[g]; cband < end_cband[g]; cband++) {
      m = model_index[ch][g][cband];
      si_cw_len += sam_encode_symbol(m, AModelCBand[cband_model]);
    }
  }
  return si_cw_len;
}
  
/*--------------------------------------------------------------*/
/***********  scalefactor band side infomation   ****************/
/*--------------------------------------------------------------*/
static int encode_scfband_si(
  int scf[][8][MAX_SCFAC_BANDS],
  int start_sfb[],
  int end_sfb[],
  int max_scf[],
  int scf_model[],
  int stereo_mode,
  /*int *stereo_info[8],*/
	int stereo_info[8][MAX_SCFAC_BANDS], /* SAMSUNG_2005-09-30 */
  int stereo_si_coded[8][MAX_SCFAC_BANDS],
  int g,
  int i_ch,
  int nch)
{
  int ch;
  int nnch;
  int m;
  int sfb;
  int si_cw_len;
  int pns_data_present=0;
  int pns_start_sfb=100;
  int pns_sfb_mode=100;
  int pns_sfb_flag[2][8][60];
  int max_pns_energy[2];
  int pns_pcm_flag[2] = {1, 1}; 


  nnch = nch - i_ch;
  si_cw_len = 0;
  for(ch = i_ch; ch < nch; ch++) {
    for(sfb = start_sfb[g]; sfb < end_sfb[g]; sfb++) {
      if(nnch == 1) {
        if(pns_data_present && sfb >= pns_start_sfb) {
          m = pns_sfb_flag[0][g][sfb];
          si_cw_len += sam_encode_symbol(m, AModelNoiseFlag);
        }
      } else if(nnch == 2 && stereo_si_coded[g][sfb] == 0) {
        if(stereo_mode != 2) {
          m = stereo_info[g][sfb];
          if(stereo_mode == 1) {
            si_cw_len += sam_encode_symbol(m, AModelMsUsed);
          } else if(stereo_mode == 3) {
            si_cw_len += sam_encode_symbol(m, AModelStereoInfo);
          }
          if(pns_data_present && sfb >= pns_start_sfb) {
            m = pns_sfb_flag[0][g][sfb];
            si_cw_len += sam_encode_symbol(m, AModelNoiseFlag);
            m = pns_sfb_flag[1][g][sfb];
            si_cw_len += sam_encode_symbol(m, AModelNoiseFlag);
            if(stereo_mode == 3 && stereo_info[g][sfb] == 3)
              if(pns_sfb_flag[0][g][sfb] && pns_sfb_flag[1][g][sfb]) {
                m = pns_sfb_mode;
                si_cw_len += sam_encode_symbol(m, AModelNoiseMode);
              }
          }
        }
        stereo_si_coded[g][sfb] = 1;
      }

      pns_sfb_flag[ch][g][sfb] = 0;
      if (pns_sfb_flag[ch][g][sfb]) {
        if (pns_pcm_flag[ch]==1) {
		   m = max_pns_energy[ch];
           si_cw_len += sam_encode_symbol(m, AModelNoiseNrg);
		   pns_pcm_flag[ch] = 0;
		}
        m = max_pns_energy[ch] - scf[ch][g][sfb];
        si_cw_len += sam_encode_symbol(m, AModelScf[scf_model[ch]]);
     }
     else if ( stereo_info[g][sfb]>=2 && ch==1) {
	    /* is_position */
        if (scf[ch][g][sfb]>=0)
           m = scf[ch][g][sfb] * 2;
        else 
           m = -scf[ch][g][sfb] * 2 - 1;
        si_cw_len += sam_encode_symbol(m, AModelScf[scf_model[ch]]);
     }
     else { 
		if (scf_model[ch]) {
          m = max_scf[ch] - scf[ch][g][sfb];
          si_cw_len += sam_encode_symbol(m, AModelScf[scf_model[ch]]);
        }
      }
    }
  }


  return si_cw_len;
}
  

static int select_freq1 (
  int model_index,     /* model index for coding quantized spectral data */
  int bpl,             /* bit plane index */
  int coded_samp_bit,  /* vector of previously encoded MSB of sample */
  int available_len)
{
  int no_model;
  int dbpl;
  int  freq;

  if (model_index >= 15)
    dbpl =  model_index - 7  - bpl;
  else 
    dbpl =  (model_index+1)/2  - bpl;

  if (dbpl>=4) 
    no_model = model_offset_tbl[model_index][7];
  else 
    no_model = model_offset_tbl[model_index][dbpl+3];

  if (coded_samp_bit > 15)
    no_model += 15;
  else
    no_model += coded_samp_bit - 1;
  if(no_model > 1015) no_model = 1015;

  freq = AModelSpectrum[no_model];
  if ( available_len < 14) { 
    if ( AModelSpectrum[no_model] < min_freq[available_len] ) 
      freq = min_freq[available_len];
    else if ( AModelSpectrum[no_model] > (16384-min_freq[available_len]) ) 
      freq = 16384 - min_freq[available_len];
  }

  return freq;
}

static int select_freq0 (
  int model_index,    /* model index for coding quantized spectral data */
  int bpl,            /* bit plane index */
  int enc_msb_vec,    /* vector of the encoded bits of 4-samples 
                         until the (bpl+1)-th bit plane */ 
  int samp_pos,       /* current sample position among 4-samples */
  int enc_csb_vec,    /* vector of the previously encoded bits of 4-samples
                         at the current bit plane */
  int available_len)
{
  int no_model;
  int dbpl;
  int  freq;

  if (model_index >= 15)
    dbpl =  model_index - 7  - bpl;
  else 
    dbpl =  (model_index+1)/2  - bpl;
  if (dbpl>=3) 
    no_model = model_offset_tbl[model_index][3];
  else
    no_model = model_offset_tbl[model_index][dbpl];

  no_model += small_step_offset_tbl[enc_msb_vec][samp_pos][enc_csb_vec];
  if(no_model > 1015) no_model = 1015;

  freq = AModelSpectrum[no_model];
  if ( available_len < 14) { 
    if ( AModelSpectrum[no_model] < min_freq[available_len] ) 
      freq = min_freq[available_len];
    else if ( AModelSpectrum[no_model] > (16384-min_freq[available_len]) ) 
      freq = 16384 - min_freq[available_len];
  }

  return freq;
}

/*--------------------------------------------------------------*/
/***********     Bit-Sliced Spectral Data        ****************/
/*--------------------------------------------------------------*/
static int encode_spectra(
  int *sample[][8],
  int s_reg,
  int e_reg,
  int s_freq[8],
  int e_freq[8],
  int min_bpl,
  int available_len,
  int model_index[][8][32],
  int *cur_bpl[][8],
  int *coded_samp_bit[][8],
  int *sign_coded[][8],
  int *enc_sign_vec[][8],
  int *enc_vec[][8],
  int sba_mode,
  int i_ch,
  int nch)
{
  int  ch, i, m, k;
  int  maxbpl;
  int  bpl;
  int  cband;
  int  maxhalf;
  int  freq;
  int  cw_len;
  int  cw_len1;
  int  reg;

  maxbpl = 0;
  for(ch = i_ch; ch < nch; ch++)
  for(reg=s_reg; reg<e_reg; reg++)
  for(i = s_freq[reg]; i < e_freq[reg]; i++)
    if (maxbpl < cur_bpl[ch][reg][i]) 
			maxbpl = cur_bpl[ch][reg][i];

  cw_len = available_len;
  for (bpl = maxbpl; bpl >= min_bpl; bpl--) {

    if (cw_len <= 0 ) return (available_len-cw_len);

    maxhalf = 1<<(bpl-1);
      
    for(reg = s_reg; reg < e_reg; reg++)
    for (i = s_freq[reg]; i < e_freq[reg]; i++) {
    for(ch = i_ch; ch < nch; ch++) {

       if ( cur_bpl[ch][reg][i] < bpl ) continue; 

      if (coded_samp_bit[ch][reg][i]==0 || sign_coded[ch][reg][i]==1) {

        cband = i/32;
        if (i%4==0) {
          enc_sign_vec[ch][reg][i/4] |= enc_vec[ch][reg][i/4];
          enc_vec[ch][reg][i/4] = 0;
        }

        cw_len1 = cw_len;
        if(!sba_mode) cw_len1 = 100;
        if (coded_samp_bit[ch][reg][i]) {
          freq  = select_freq1 (model_index[ch][reg][cband], bpl, 
              coded_samp_bit[ch][reg][i], cw_len1);
        } else {

          freq  = select_freq0 (model_index[ch][reg][cband], bpl, 
          enc_sign_vec[ch][reg][i/4], i%4, enc_vec[ch][reg][i/4], cw_len1);
        }

        if( abs(sample[ch][reg][i]) & maxhalf)   m = 1;
        else                 m = 0;

        k = sam_encode_symbol2(m, freq);
        cw_len -= k;

        enc_vec[ch][reg][i/4] = (enc_vec[ch][reg][i/4]<<1) | m;
        coded_samp_bit[ch][reg][i] = (coded_samp_bit[ch][reg][i]<<1) | m;

      }

      if ( coded_samp_bit[ch][reg][i] && sign_coded[ch][reg][i]==0) {
        if (cw_len <= 0 ) return (available_len-cw_len); 

        if (sample[ch][reg][i] < 0) {
          m = 1;
        } else {
          m = 0;
        }
        k = sam_encode_symbol2(m, 8192);
        cw_len -= k;
        sign_coded[ch][reg][i] = 0x01;
      }
        cur_bpl[ch][reg][i]-=1;

      if (cw_len <= 0 ) return (available_len-cw_len);
    }
    } /* for (i=0; i<end_freq */
  }
  return (available_len-cw_len);
}
