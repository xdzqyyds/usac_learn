/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-29

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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "block.h"               /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "sam_tns.h"             /* structs */

#include "common_m4a.h"

#include "sam_dec.h"
#include "sam_model.h"


#define	TOP_LAYER		48
#define	PNS_SF_OFFSET	90
#define	MAX_CBAND0_SI_LEN	11
#define MAX_LAYER 100

static int max_cband_si_len_tbl[32] = 
{ 
  6, 5,   6, 5, 6,   6, 5, 6, 5,   6, 5, 6, 8,   6, 5, 6, 8, 9,
  6, 5, 6, 8, 10,   8, 10,   9, 10,  10,  12,  12,  12,  12
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

/*
static double  fs_tbl[9]={48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};
*/
static int sam_scale_bits_dec[MAX_LAYER];
static int sampling_rate;

#ifdef  __cplusplus
extern "C" {
#endif

  static int decode_cband_si(
                             int model_index[][8][32],
                             int cband_si_type[],
                             int start_cband[][8],
                             int end_cband[][8],
                             int g,
                             int nch );

  static int decode_scfband_si(
                               int	scf[][MAX_SCFAC_BANDS],
                               int	start_sfb[][8],
                               int end_sfb[][8],
                               int	max_scf[],
                               int	scf_model[],
                               int	stereo_mode,
                               int	ms_mask[],
                               int	is_info[],
                               int maxSfb,
                               int stereo_si_coded[],
                               int pns_data_present,
                               int pns_sfb_start,
                               int pns_sfb_flag[][MAX_SCFAC_BANDS],
                               int pns_sfb_mode[MAX_SCFAC_BANDS],
                               int pns_pcm_flag[],
                               int max_pns_energy[],
                               int g,
                               int nch);

  static int select_freq1 (
                           int model_index,
                           int bpl,
                           int coded_samp_bit,
                           int available_len );

  static int select_freq0 (
                           int model_index,
                           int bpl,
                           int enc_msb_vec,
                           int samp_pos,
                           int enc_csb_vec,
                           int available_len );

  static int decode_spectra(
                            int *sample[][8],
                            int s_reg,
                            int e_reg,
                            int s_freq[][8],
                            int e_freq[][8],
                            int min_bpl,
                            int available_len,
                            int model_index[][8][32],
                            int *cur_bpl[][8],
                            int *coded_samp_bit[][8],
                            int *sign_coded[][8],
                            int *enc_sign_vec[][8],
                            int *enc_vec[][8],
                            int sba_mode,
                            int nch );

  static int initialize_layer_data(
                                   WINDOW_SEQUENCE windowSequence,
                                   int num_window_groups, 
                                   int window_group_length[],
                                   int base_band,
                                   int enc_top_layer,
                                   int maxSfb,
                                   int swb_offset[][52],
                                   int top_band,
                                   int layer_max_freq[],
                                   int layer_max_cband[], 
                                   int layer_max_qband[],
                                   int layer_bit_flush[],
                                   int layer_reg[]
                                   );

#ifdef  __cplusplus
}
#endif

void sam_scale_bits_init(int sampling_rate_decoded, int block_size_samples)
{
  int layer, layer_bit_rate;
  int average_layer_bits;
  static int init_pns_noise_nrg_model = 1; 

  /* calculate the avaerage amount of bits available
     for the scalability layer of one block per channel */
  for (layer=0; layer<MAX_LAYER; layer++) {
    layer_bit_rate = (layer+16) * 1000;
    average_layer_bits = (int)((double)layer_bit_rate*(double)block_size_samples/(double)sampling_rate_decoded);
    sam_scale_bits_dec[layer] = (average_layer_bits / 8) * 8;
  }
  sampling_rate = sampling_rate_decoded;
  if (init_pns_noise_nrg_model) {
    int i;

    AModelNoiseNrg[0] = 16384 - 32;
    for (i=1; i<512; i++) 
      AModelNoiseNrg[i] = AModelNoiseNrg[i-1] - 32;
    init_pns_noise_nrg_model = 0;
  }
}


void sam_decode_bsac_stream ( int    target,
                              int    frameLength,
                              int    sba_mode,
                              int    enc_top_layer,
                              int    base_snf,
                              int    base_band,
                              int    maxSfb,
                              WINDOW_SEQUENCE  windowSequence,
                              int    num_window_groups,
                              int    window_group_length[8],

                              int    maxScalefactors[],
                              int    cband_si_type[],
                              int    scf_model0[],
                              int    scf_model1[],
                              int    max_sfb_si_len[],

                              int    stereo_mode,
                              int    ms_mask[],
                              int    is_info[],
                              int    pns_data_present,
                              int    pns_sfb_start,
                              int    pns_sfb_flag[][MAX_SCFAC_BANDS],
                              int    pns_sfb_mode[MAX_SCFAC_BANDS],
                              int    swb_offset[][52],
                              int    top_band,
                              int    used_bits,
                              int    sample_buf[][1024],
                              int    scalefactors[][MAX_SCFAC_BANDS],
                              int    nch
                              /* 20060107 */
                              ,int	*core_bandwidth
                              )
{

  int    i, ch, g, k;
  int    qband, cband;
  int    layer_sfb[MAX_LAYER];
  int    layer_cband[8][MAX_LAYER];
  int    layer_index[8][MAX_LAYER];
  int    layer_reg[MAX_LAYER];
  int    layer_max_freq[MAX_LAYER];
  int    layer_max_cband[MAX_LAYER];
  int    layer_max_qband[MAX_LAYER];
  int    layer_bit_flush[MAX_LAYER];
  int    layer_buf_offset[MAX_LAYER];
  int    layer_si_maxlen[MAX_LAYER];
  int    layer_extra_len[MAX_LAYER];
  int    layer_cw_offset[MAX_LAYER];

  int    start_freq[2][8];
  int    start_qband[2][8];
  int    start_cband[2][8];
  int    end_freq[2][8];
  int    end_cband[2][8];
  int    end_qband[2][8];
  int    s_freq[2][8];
  int    e_freq[2][8];

  int    cw_len;
  int    layer;
  int    slayer_size;
  int    si_offset;
  int    available_len = 0;
  int    cur_snf_buf[2][1024];
  int    *cur_snf[2][8];
  int    band_snf[2][8][32];
  int    ModelIndex[2][8][32];
  int    *sample[2][8];
  int    sign_is_coded_buf[2][1024];
  int    *sign_is_coded[2][8];
  int    *coded_samp_bit[2][8];
  int    coded_samp_bit_buf[2][1024];
  int    *enc_sign_vec[2][8];
  int    enc_sign_vec_buf[2][256];
  int    *enc_vec[2][8];
  int    enc_vec_buf[2][256];
  int     stereo_si_coded[MAX_SCFAC_BANDS];
  int    dscale_bits[MAX_LAYER];
  int    flen;
  int    max_pns_energy[2];
  int    pns_pcm_flag[2] = {1, 1};
  long   avail_bit_len;

  /* 20060107 */
  long	UsedBit;


  if (target > enc_top_layer) target = enc_top_layer; 
  /* ***************************************************** */
  /*                  Initialize variables                 */
  /* ***************************************************** */
  for(ch = 0; ch < nch; ch++) {
    int s;

    s = 0;
    for(g = 0; g < num_window_groups; g++) { 
      sample[ch][g] = &(sample_buf[ch][1024*s/8]);
      sign_is_coded[ch][g] = &(sign_is_coded_buf[ch][1024*s/8]);
      coded_samp_bit[ch][g] = &(coded_samp_bit_buf[ch][1024*s/8]);
      enc_sign_vec[ch][g] = &(enc_sign_vec_buf[ch][(1024*s/8)/4]);
      enc_vec[ch][g] = &(enc_vec_buf[ch][(1024*s/8)/4]);
      cur_snf[ch][g] = &(cur_snf_buf[ch][1024*s/8]);
      s += window_group_length[g];
      for (i=0; i<maxSfb; i++)
        scalefactors[ch][g*maxSfb+i] = 0;
      for (i=0; i<32; i++) {
        band_snf[ch][g][i] = 0;
        ModelIndex[ch][g][i] = 0;
      }
    }

    for (i=0; i<1024; i++) {
      sign_is_coded_buf[ch][i] = 0;
      coded_samp_bit_buf[ch][i] = 0;
      cur_snf_buf[ch][i] = 0;
      sample_buf[ch][i] = 0;
    }
    for (i=0; i<256; i++) {
      enc_sign_vec_buf[ch][i] = 0;
      enc_vec_buf[ch][i] = 0;
    }
  }

  for (i=0; i<MAX_SCFAC_BANDS; i++)
    stereo_si_coded[i] = 0;

  layer_sfb[0] = 0;
  for(g = 0; g < num_window_groups; g++) {
    layer_cband[g][0] = 0;
    layer_index[g][0] = 0;
  }

  if(maxSfb == 0) {
    if(nch == 1) return;
    if(nch == 2 && maxSfb == 0) return;
  }

  if(used_bits % 8) {
    int  dummy;
    k = 8 - (used_bits % 8);
    dummy = sam_GetBits(k);
    used_bits += k;
  }

  /* Initialize
     layer_max_freq
     layer_max_cband
     layer_max_qband
     layer_bit_flush
  */
  slayer_size = initialize_layer_data(windowSequence, num_window_groups, 
                                      window_group_length, base_band, enc_top_layer, maxSfb, swb_offset, top_band,
                                      layer_max_freq, layer_max_cband, layer_max_qband,
                                      layer_bit_flush, layer_reg);

  for (i = 0; i <= enc_top_layer; i++)
    dscale_bits[i] = sam_scale_bits_dec[i] * nch;

  /* ##################################################### */
  /*                   BSAC MAIN ROUTINE                    */
  /* ##################################################### */

  /* ***************************************************** */
  /*                  BSAC_layer_stream()                  */
  /* ***************************************************** */

  /* calculate layer buffer position */
  for (layer=0; layer<(enc_top_layer+slayer_size); layer++) 
    layer_si_maxlen[layer] = 0;
  
  for(ch = 0; ch < nch; ch++)
    for(g = 0; g < num_window_groups; g++)
      end_cband[ch][g] = end_qband[ch][g] = 0;

  for (layer=0; layer<(enc_top_layer+slayer_size); layer++) {

    for(ch = 0; ch < nch; ch++) {
      g = layer_reg[layer];
      start_qband[ch][g] = end_qband[ch][g];
      start_cband[ch][g] = end_cband[ch][g];
      end_qband[ch][g] = layer_max_qband[layer];
      end_cband[ch][g] = layer_max_cband[layer];
    }

    for(ch = 0; ch < nch; ch++) {
      /* coding band side infomation  */
      for (cband = start_cband[ch][g]; cband < end_cband[ch][g]; cband++) {
        if (cband <= 0) 
          layer_si_maxlen[layer] += MAX_CBAND0_SI_LEN;
        else 
          layer_si_maxlen[layer] += max_cband_si_len_tbl[cband_si_type[ch]];  
      }
    
      /* side infomation : scalefactor */
      for(qband = start_qband[ch][g]; qband < end_qband[ch][g]; qband++) {
        layer_si_maxlen[layer] += max_sfb_si_len[ch] + 5;
      }
    }
  }

  flen = frameLength;
  if(flen < dscale_bits[enc_top_layer]) flen = dscale_bits[enc_top_layer];

  for (layer=enc_top_layer+slayer_size; layer< MAX_LAYER; layer++)
    layer_buf_offset[layer] = flen;

  si_offset = flen - layer_si_maxlen[enc_top_layer+slayer_size-1];
  /* 2000. 08. 22. enc_top_layer -> enc_top_layer+slayer_size-1. SHPARK */

  for (layer=(enc_top_layer+slayer_size-1); layer>=slayer_size; layer--) {
    if ( si_offset < dscale_bits[layer-slayer_size] )
      layer_buf_offset[layer] = si_offset;
    else
      layer_buf_offset[layer] = dscale_bits[layer-slayer_size];
    si_offset = layer_buf_offset[layer] - layer_si_maxlen[layer-1];
  }
  for (; layer>0; layer--) {
    layer_buf_offset[layer] = si_offset;
    si_offset = layer_buf_offset[layer] - layer_si_maxlen[layer-1];
  }
  layer_buf_offset[0] = used_bits;

  if (used_bits > si_offset) {
    si_offset = used_bits - si_offset;
    for (layer=(enc_top_layer+slayer_size-1); layer>=slayer_size; layer--) {
      cw_len  = layer_buf_offset[layer+1] - layer_buf_offset[layer];
      cw_len -= layer_si_maxlen[layer];
      if (cw_len >= si_offset) { 
        cw_len = si_offset; 
        si_offset = 0;
      } else
        si_offset -= cw_len;

      for (i=1; i<=layer; i++)
        layer_buf_offset[i] += cw_len;
 
      if (si_offset==0) break;
    }
  } else {
    si_offset = si_offset - used_bits;
    for (layer = 1; layer < slayer_size; layer++) {
      layer_buf_offset[layer] = layer_buf_offset[layer-1] + layer_si_maxlen[layer-1] + si_offset/slayer_size;
      if ( layer <= (si_offset%slayer_size) )
        layer_buf_offset[layer]++;
    }
  }
 
  /* added by shpark. 2001. 02. 15 */
  for (layer=enc_top_layer+slayer_size-1; layer>=0; layer--) {
    if (layer_buf_offset[layer] > frameLength) 
      layer_buf_offset[layer] = frameLength;
    else 
      break;
  }

  /* Nice hyungk !!! (2003.1.22)
     avail_bit_len = sam_BsNumByte() * 8;*/
  avail_bit_len = frameLength;
 
  for (layer=target+slayer_size-1; layer>=0; layer--) {
    if (layer_buf_offset[layer] > avail_bit_len) target--;
    else break;
  }

  /* ********************************************************* */
  /*       Split Input Bitstream into Layer Bitstreams        */
  /* ********************************************************* */
  sam_init_layer_buf();

  sam_setRBitBufPos(used_bits);

  if(sba_mode) {
    int  total_layer;

    total_layer = enc_top_layer + slayer_size;
    total_layer = target + slayer_size;
    for(layer = 0; layer < total_layer; layer++) {
      unsigned data;
      layer_cw_offset[layer] = sam_getRBitBufPos();
      cw_len = layer_buf_offset[layer+1] - layer_buf_offset[layer];

      while(cw_len > 0) {
        int len;

        if (sam_getUsedBits()>=frameLength) break;
        len = (cw_len > 8) ? 8 : cw_len;
        data = sam_GetBits(len);
        sam_putbits2buf(data, len);
        cw_len -= len;
      }
      while(cw_len > 0) {
        sam_putbits2buf(0, 8);
        cw_len -= 8;
      } 
      if(layer_bit_flush[layer])
        sam_putbits2buf(0x00, 32);
    }
  } else {
    unsigned data;

    cw_len = avail_bit_len - used_bits;
    while(cw_len > 0) {
      int len;

      if (sam_getUsedBits()>=avail_bit_len) break;
      len = (cw_len > 8) ? 8 : cw_len;
      data = sam_GetBits(len);
      sam_putbits2buf(data, len);
      cw_len -= len;
    }
    while(cw_len > 0) {
      sam_putbits2buf(0, 8);
      cw_len -= 8;
    } 
    sam_putbits2buf(0x00, 32);
  }

  for(i = 0; i < MAX_LAYER; i++) {
    layer_extra_len[i] = 0;
  }

  for(ch = 0; ch < nch; ch++)
    for(g = 0; g < num_window_groups; g++) {
      start_qband[ch][g] = 0;
      start_cband[ch][g] = 0;
      start_freq[ch][g] = 0;
      end_qband[ch][g] = 0;
      end_cband[ch][g] = 0;
      end_freq[ch][g] = 0;
    }

  if(!sba_mode) {
    sam_initArDecode(0);
    sam_setArDecode(0);
    sam_setRBitBufPos(used_bits);
    available_len = 0;
  }

  for(ch = 0; ch < nch; ch++)
    for(g = 0; g < num_window_groups; g++)
      s_freq[ch][g] = 0;

  if (windowSequence == 2) {
    for (layer = 0; layer < slayer_size; layer++) {
      for(ch = 0; ch < nch; ch++) {
        g = layer_reg[layer];
        e_freq[ch][g]  = layer_max_freq[layer];
      }
    }
  }

  /* 20060107 */
  /*return core bandwidth to applay sbr beyond it.*/
  if(windowSequence == 2)
    {
      g = layer_reg[target+slayer_size-1];
      *core_bandwidth = layer_max_freq[target+slayer_size-1]/window_group_length[g]*sampling_rate/2./128.;
    }
  else
    {
      *core_bandwidth = layer_max_freq[target+slayer_size-1]*sampling_rate/2./1024.;
    }

  for (layer = 0; layer < target+slayer_size; layer++) {
    int min_snf;
    int *scf_model;

    for(ch = 0; ch < nch; ch++) {
      g = layer_reg[layer];

      start_freq[ch][g]  = end_freq[ch][g];
      start_qband[ch][g] = end_qband[ch][g];
      start_cband[ch][g] = end_cband[ch][g];

      end_qband[ch][g] = layer_max_qband[layer];
      end_cband[ch][g] = layer_max_cband[layer];
      end_freq[ch][g]  = layer_max_freq[layer];
    }

    if (windowSequence == 2) {
      if(layer >= slayer_size) {
        for(ch = 0; ch < nch; ch++) {
          g = layer_reg[layer];
          e_freq[ch][g]  = layer_max_freq[layer];
        }
      }
    } else {
      for(ch = 0; ch < nch; ch++) {
        if (layer >= slayer_size) 
          e_freq[ch][0] = layer_max_freq[layer];
        else
          e_freq[ch][0] = layer_max_freq[slayer_size-1];
      }
    }

    if(sba_mode) {
      if (layer==0 || layer_bit_flush[layer-1]) {
        sam_initArDecode(layer);
        sam_setArDecode(layer);
        sam_setRBitBufPos(layer_cw_offset[layer]);

        available_len = layer_buf_offset[layer+1]-layer_buf_offset[layer];
        available_len--;
      } else  {
        available_len += layer_buf_offset[layer+1]-layer_buf_offset[layer];
      }
    } else {
      available_len += layer_buf_offset[layer+1]-layer_buf_offset[layer];
    }

    /*if (available_len <= 0 )  {*/
    if (sba_mode && available_len <= 0 ) { /* SAMSUNG_2005-09-30 */
      for(ch = 0; ch < nch; ch++) {
        g = layer_reg[layer];
        for(i = start_freq[ch][g]; i < end_freq[ch][g]; i++)  
          cur_snf[ch][g][i] = band_snf[ch][g][i/32];
      }
      continue;
    }

    /* coding band side infomation  */
    cw_len =  decode_cband_si(ModelIndex, cband_si_type, start_cband, end_cband,g, nch);
    for(ch = 0; ch < nch; ch++) {
      g = layer_reg[layer];
      for(cband = start_cband[ch][g]; cband < end_cband[ch][g]; cband++) {
        if(ModelIndex[ch][g][cband] >= 15)
          band_snf[ch][g][cband] = ModelIndex[ch][g][cband] - 7;
        else
          band_snf[ch][g][cband] = (ModelIndex[ch][g][cband]+1) / 2;
      }
      
      for(i = start_freq[ch][g]; i < end_freq[ch][g]; i++)  
        cur_snf[ch][g][i] = band_snf[ch][g][i/32];
    }

    g = layer_reg[layer];
    available_len -= cw_len;
    
    /* side infomation : scalefactor */
    if (layer < slayer_size) 
      scf_model = (int *)scf_model0;
    else
      scf_model = (int *)scf_model1;
    cw_len =  decode_scfband_si(scalefactors, start_qband, end_qband, 
                                maxScalefactors, scf_model, stereo_mode, ms_mask, is_info, 
                                maxSfb, stereo_si_coded, pns_data_present, pns_sfb_start, 
                                pns_sfb_flag, pns_sfb_mode, pns_pcm_flag, max_pns_energy, g, nch);

    available_len -= cw_len;

    min_snf = layer < slayer_size ? base_snf : 1;

    /* Bit-Sliced Spectral Data Coding */
    cw_len = decode_spectra(sample, g, g+1, start_freq, end_freq, 
                            min_snf, available_len, ModelIndex, cur_snf, coded_samp_bit, 
                            sign_is_coded, enc_sign_vec, enc_vec, sba_mode, nch);

    available_len -= cw_len;

    if (sba_mode && layer_bit_flush[layer] && available_len > 0) {
      layer_extra_len[layer] = available_len;
      sam_storeArDecode(layer);
      layer_cw_offset[layer] = sam_getRBitBufPos();
    }
    if(!sba_mode && available_len > 0) {
      cw_len = decode_spectra(sample, 0, num_window_groups, s_freq, e_freq, 1,
                              available_len, ModelIndex, cur_snf, coded_samp_bit, 
                              sign_is_coded, enc_sign_vec, enc_vec, sba_mode, nch);
      available_len -= cw_len;
    }

  } /* for (layer = 0; layer <= top_layer; layer++) */
  if(!sba_mode) layer_extra_len[enc_top_layer+slayer_size-1] = frameLength;


  /* ***************************************************** */
  /*         Extra Encoding Process  for each layer        */
  /* ***************************************************** */
  for(ch = 0; ch < nch; ch++)
    for(g = 0; g < num_window_groups; g++)
      start_freq[ch][g] = 0;

  if (windowSequence == 2) {
    for (layer = 0; layer < slayer_size; layer++) {
      for(ch = 0; ch < nch; ch++) {
        g = layer_reg[layer];
        e_freq[ch][g]  = layer_max_freq[layer];
      }
    }
  }

  if(sba_mode)
    for (layer = 0; layer < target+slayer_size; layer++) {

      available_len = layer_extra_len[layer];

      if (windowSequence == 2) {
        if(layer >= slayer_size) {
          for(ch = 0; ch < nch; ch++) {
            g = layer_reg[layer];
            e_freq[ch][g]  = layer_max_freq[layer];
          }
        }
      } else {
        for(ch = 0; ch < nch; ch++) {
          if (layer >= slayer_size) 
            e_freq[ch][0] = layer_max_freq[layer];
          else
            e_freq[ch][0] = layer_max_freq[slayer_size-1];
        }
      }
      if (available_len <= 0 )  continue;

      if(sba_mode) {
        sam_setArDecode(layer);
        sam_setRBitBufPos(layer_cw_offset[layer]);
      }

      /* Extra Bit-Sliced Spectral Data Coding */
      cw_len = decode_spectra(sample, 0, num_window_groups, start_freq, e_freq, 1,
                              available_len, ModelIndex, cur_snf, coded_samp_bit, 
                              sign_is_coded, enc_sign_vec, enc_vec, sba_mode, nch);

      available_len -= cw_len;

      if(windowSequence == 2) {
        for(ch = 0; ch < nch; ch++)
          for(g = 0; g < num_window_groups; g++)
            s_freq[ch][g] = e_freq[ch][g];
      }

      for (k = layer+1; k < target+slayer_size; k++) {

        if (available_len <= 0 ) break; 

        for(ch = 0; ch < nch; ch++) {
          g = layer_reg[k];
          s_freq[ch][g] = e_freq[ch][g];
          e_freq[ch][g] = layer_max_freq[k];
        }

        cw_len = decode_spectra(sample, 0, num_window_groups, s_freq, e_freq, 1,
                                available_len, ModelIndex, cur_snf, coded_samp_bit, 
                                sign_is_coded, enc_sign_vec, enc_vec, sba_mode, nch);
        available_len -= cw_len;
      }
    }
  
  /* 20060107 */
  UsedBit = sam_getRBitBufPos();
  
  /*  byte_algin */
  if (UsedBit %8 != 0) {
    sam_getbitsfrombuf(8-(UsedBit%8));
  }
  
}

/*--------------------------------------------------------------*/
/***************   coding band side infomation   ****************/
/*--------------------------------------------------------------*/
static int decode_cband_si(
                           int model_index[][8][32],
                           int cband_si_type[],
                           int start_cband[][8],
                           int end_cband[][8],
                           int g,
                           int nch)
{
  int	ch;
  int	m;
  int cband;
  int cband_model;
  int si_cw_len;
  int max_model_index[2];
  static int max_model0index_tbl[32] = 
  { 
    6, 6,   8, 8, 8,   10, 10, 10, 10,   12, 12, 12, 12,   14, 14, 14, 14, 14,
    15, 15, 15, 15, 15,   16, 16,  17, 17,  18,  19,  20,  21,  22
  };
	
  static int max_modelindex_tbl[32] = 
  { 
    4, 6,   4, 6, 8,   4, 6, 8, 10,   4, 6, 8, 12,   4, 6, 8, 12, 14,
    4, 6, 8, 12, 15,   12, 16,   14, 17,  18,  19,  20,  21,  22
  };
	

  si_cw_len = 0;
  for(ch = 0; ch < nch; ch++) {
    if(start_cband[ch][g] == 0) {
      cband_model = 7;
      max_model_index[ch] = max_model0index_tbl[cband_si_type[ch]];
    } else {
      cband_model = cband_si_cbook_tbl[cband_si_type[ch]];
      max_model_index[ch] = max_modelindex_tbl[cband_si_type[ch]];
    }
    for (cband = start_cband[ch][g]; cband < end_cband[ch][g]; cband++) {
      si_cw_len += sam_decode_symbol(AModelCBand[cband_model], &m);
      model_index[ch][g][cband] = m;

      /* F O R  E R R O R */
      if (model_index[ch][g][cband] > max_model_index[ch])
        model_index[ch][g][cband] = max_model_index[ch];

    }
  }
  return si_cw_len;
}


/*--------------------------------------------------------------*/
/***********  scalefactor band side infomation   ****************/
/*--------------------------------------------------------------*/
static int decode_scfband_si(
                             int	scf[][MAX_SCFAC_BANDS],
                             int	start_sfb[][8],
                             int end_sfb[][8],
                             int	max_scf[],
                             int	scf_model[],
                             int	stereo_mode,
                             int	ms_mask[],
                             int	is_info[],
                             int maxSfb,
                             int stereo_si_coded[],
                             int pns_data_present,
                             int pns_sfb_start,
                             int pns_sfb_flag[][MAX_SCFAC_BANDS],
                             int pns_sfb_mode[MAX_SCFAC_BANDS],
                             int pns_pcm_flag[],
                             int max_pns_energy[],
                             int g,
                             int nch)
{
  int ch;
  int	m;
  int sfb;
  int si_cw_len;

  si_cw_len = 0;
  for(ch = 0; ch < nch; ch++) {
    for(sfb = start_sfb[ch][g]; sfb < end_sfb[ch][g]; sfb++) {
      if(nch == 1) {
        if(pns_data_present && sfb >= pns_sfb_start) {
          si_cw_len += sam_decode_symbol(AModelNoiseFlag, &m);
          pns_sfb_flag[0][g*maxSfb+sfb] = m;
        }
      } else if(stereo_si_coded[maxSfb*g+sfb] == 0) {
        if(stereo_mode != 2) {
          m = 0;
          if(stereo_mode == 1)
            si_cw_len += sam_decode_symbol(AModelMsUsed, &m);
          else if(stereo_mode == 3)
            si_cw_len += sam_decode_symbol(AModelStereoInfo, &m);

          switch(m) {
          case 1:
            ms_mask[g*maxSfb+sfb+1] = 1;
            break;
          case 2:
            is_info[g*maxSfb+sfb] = 1;
            break;
          case 3:
            is_info[g*maxSfb+sfb] = 2;
            break;
          }
          if(pns_data_present && sfb >= pns_sfb_start) {
            si_cw_len += sam_decode_symbol(AModelNoiseFlag, &m);
            pns_sfb_flag[0][g*maxSfb+sfb] = m;
            si_cw_len += sam_decode_symbol(AModelNoiseFlag, &m);
            pns_sfb_flag[1][g*maxSfb+sfb] = m;
            if(stereo_mode == 3 && is_info[g*maxSfb+sfb] == 2) {
              if(pns_sfb_flag[0][g*maxSfb+sfb] && pns_sfb_flag[1][g*maxSfb+sfb]) {
                si_cw_len += sam_decode_symbol(AModelNoiseMode, &m);
                pns_sfb_mode[g*maxSfb+sfb] = m;
              }
            }
          }
        }
        stereo_si_coded[maxSfb*g+sfb] = 1;
      }
      if (pns_sfb_flag[ch][g*maxSfb+sfb]) {
        if (pns_pcm_flag[ch]==1) {
          si_cw_len+=sam_decode_symbol(AModelNoiseNrg,&m);
          max_pns_energy[ch] = m;
          pns_pcm_flag[ch] = 0;
        }
        /* Latest change by Sang-Wook Kim on 14th March 2005. */
        if( scf_model[ch])   {  
          si_cw_len+=sam_decode_symbol(AModelScf[scf_model[ch]],&m);                
          scf[ch][g*maxSfb+sfb] = max_pns_energy[ch] - m;
        }
        else {
          scf[ch][g*maxSfb+sfb] = max_pns_energy[ch];
        }
      }
      else if ( is_info[g*maxSfb+sfb] && ch==1) {
        if (scf_model[ch]==0) {
          scf[ch][g*maxSfb+sfb] = 0;
        }
        else {
          /* is_position */
          si_cw_len += sam_decode_symbol(AModelScf[scf_model[ch]],&m);
          if (m%2) 
            scf[ch][g*maxSfb+sfb] = -(int)((m+1)/2);
          else
            scf[ch][g*maxSfb+sfb] = (int)(m/2);
        }
      }
      else { 
        if (scf_model[ch]==0) {
          scf[ch][g*maxSfb+sfb] = max_scf[ch];
        } else {
          si_cw_len += sam_decode_symbol(AModelScf[scf_model[ch]],&m);
          scf[ch][g*maxSfb+sfb] = max_scf[ch] - m;
        }
      }
    }
  }

  return si_cw_len;
}

static int select_freq1 (
                         int model_index,     /* model index for coding quantized spectral data */
                         int  bpl,            /* bit position */
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

  if(no_model >= 1016) no_model = 1015;
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
                         int bpl,           /* bit position */
                         int enc_msb_vec,   /* vector of the encoded bits of 4-samples 
                                               until the 'bpl+1'-th bit position */ 
                         int samp_pos,       /* current sample position among 4-samples */
                         int enc_csb_vec,    /* vector of the previously encoded bits of 4-samples
                                                at the current bit plane  */
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

  if(no_model >= 1016) no_model = 1015;
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
static int decode_spectra(
                          int *sample[][8],
                          int s_reg,
                          int e_reg,
                          int s_freq[][8],
                          int e_freq[][8],
                          int min_bpl,
                          int available_len,
                          int model_index[][8][32],
                          int *cur_bpl[][8],
                          int *coded_samp_bit[][8],
                          int *sign_coded[][8],
                          int *enc_sign_vec[][8],
                          int *enc_vec[][8],
                          int sba_mode,
                          int nch)
{
  int ch, i, m, k;
  int maxbpl;
  int  bpl;
  int  cband;
  int  maxhalf;
  int  freq;
  int  cw_len;
  int  cw_len1;
  int  reg;

  maxbpl = 0;
  
  for(ch = 0; ch < nch; ch++)
    for (reg = s_reg; reg < e_reg; reg++)
      for(i = s_freq[ch][reg]; i< e_freq[ch][reg]; i++)  
        if (maxbpl < cur_bpl[ch][reg][i]) maxbpl = cur_bpl[ch][reg][i];

  cw_len = available_len;
  for (bpl=maxbpl; bpl>=min_bpl; bpl--) {

    if (cw_len <= 0 ) return (available_len-cw_len);

    maxhalf = 1<<(bpl-1);
      
    for (reg = s_reg; reg < e_reg; reg++)
      for (i=s_freq[0][reg]; i<e_freq[0][reg]; i++)
        for(ch = 0; ch < nch; ch++) {

          if ( cur_bpl[ch][reg][i] < bpl ) continue; 

          if (coded_samp_bit[ch][reg][i]==0 || sign_coded[ch][reg][i]==1) { 

            cband = i/32;
            if (i%4==0) {
              enc_sign_vec[ch][reg][i/4] |= enc_vec[ch][reg][i/4];
              enc_vec[ch][reg][i/4] = 0;
            }

            cw_len1 = cw_len;
            if(!sba_mode) cw_len1 = 100;
            if (coded_samp_bit[ch][reg][i]) 
              freq  = select_freq1 (model_index[ch][reg][cband], bpl, 
                                    coded_samp_bit[ch][reg][i], cw_len1);
            else

              freq  = select_freq0 (model_index[ch][reg][cband], bpl, 
                                    enc_sign_vec[ch][reg][i/4], i%4, enc_vec[ch][reg][i/4], cw_len1);

            k = sam_decode_symbol2(freq, &m);
            cw_len -= k;

            /* disassemble the decoded vector and
               reconstruct the decoded sample */
            if (m) {
              if (sample[ch][reg][i]>=0)   
                sample[ch][reg][i] += maxhalf;
              else           
                sample[ch][reg][i] -= maxhalf;
            }

            enc_vec[ch][reg][i/4] = (enc_vec[ch][reg][i/4]<<1) | m;
            coded_samp_bit[ch][reg][i] = (coded_samp_bit[ch][reg][i]<<1) | m;
          }

          if ( coded_samp_bit[ch][reg][i] && sign_coded[ch][reg][i]==0) {
            if (cw_len <= 0 ) return (available_len-cw_len); 

            cw_len -= sam_decode_symbol2(8192, &m);
    
            if (m) sample[ch][reg][i] *= -1;
            sign_coded[ch][reg][i] = 0x01;
          }
          cur_bpl[ch][reg][i]-=1;

          if (cw_len <= 0 ) return (available_len-cw_len);
        } 
  }
  return (available_len-cw_len);
}

static int initialize_layer_data(
                                 WINDOW_SEQUENCE windowSequence,
                                 int num_window_groups, 
                                 int window_group_length[],
                                 int base_band,
                                 int enc_top_layer,
                                 int maxSfb,
                                 int swb_offset[][52],
                                 int top_band,
                                 int layer_max_freq[],
                                 int layer_max_cband[], 
                                 int layer_max_qband[],
                                 int layer_bit_flush[],
                                 int layer_reg[])
{
  int		i, g;
  int		sfb, cband, reg;
  int		last_freq[MAX_LAYER];
  int		init_max_freq[8];
  int		end_freq[MAX_LAYER];
  int		end_cband[MAX_LAYER];
  int		layer;
  int		slayer_size = 0;

  if(windowSequence == 2) {
    slayer_size = 0;
    for(g = 0; g < num_window_groups; g++) {

      init_max_freq[g] = base_band * window_group_length[g] * 4;
      last_freq[g] = swb_offset[g][maxSfb];

      switch(sampling_rate) {
      case 48000:
      case 44100:
        if ((init_max_freq[g]%32) >= 16)
          init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 20;
        else if ((init_max_freq[g]%32) >= 4)
          init_max_freq[g] = (init_max_freq[g] / 32) * 32 + 8;
        break;
      case 32000:
      case 24000:
      case 22050:
        init_max_freq[g] = (init_max_freq[g] / 16) * 16;
        break;
      case 16000:
      case 12000:
      case 11025:
        init_max_freq[g] = (init_max_freq[g] / 32) * 32;
        break;
      case  8000:
        init_max_freq[g] = (init_max_freq[g] / 64) * 64;
        break;
      }

      if (init_max_freq[g] > last_freq[g])
        init_max_freq[g] = last_freq[g]; 
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
      switch(sampling_rate) {
      case 48000:
      case 44100:
        end_freq[reg] += 8;
        if ((end_freq[reg]%32) != 0)
          end_freq[reg] += 4;
        break;
      case 32000:
      case 24000:
      case 22050:
        end_freq[reg] += 16;
        break;
      case 16000:
      case 12000:
      case 11025:
        end_freq[reg] += 32;
        break;
      default:
        end_freq[reg] += 64;
        break;
      }
      if (end_freq[reg] > last_freq[reg] )
        end_freq[reg] = last_freq[reg];

      layer_max_freq[layer] = end_freq[reg];
      layer_max_cband[layer] = (layer_max_freq[layer]+31)/32;
    }


    for (layer = 0; layer < (enc_top_layer+slayer_size-1); layer++) {
      if (layer_max_cband[layer] != layer_max_cband[layer+1] ||
          layer_reg[layer] != layer_reg[layer+1] )
        layer_bit_flush[layer] = 1;
      else
        layer_bit_flush[layer] = 0;
    }
    for ( ; layer < MAX_LAYER; layer++)
      layer_bit_flush[layer] = 1;

    for (layer = 0; layer < MAX_LAYER; layer++) {
      int	end_layer;

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
  } else {
    g = 0;

    for(layer = 0; layer < MAX_LAYER; layer++)
      layer_reg[layer] = 0;

    last_freq[g] = swb_offset[g][maxSfb];

    /************************************************/
    /* calculate layer_max_cband and layer_max_freq */
    /************************************************/
    init_max_freq[g] = base_band * 32;

    if (init_max_freq[g] > last_freq[g])
      init_max_freq[g] = last_freq[g];
    slayer_size = (init_max_freq[g]+31)/32;

    cband = 1;
    for (layer = 0; layer < slayer_size; layer++, cband++) {

      layer_max_freq[layer] = cband * 32;
      if (layer_max_freq[layer] > init_max_freq[g])
        layer_max_freq[layer] = init_max_freq[g];

      layer_max_cband[layer] = cband;
    }

    for (layer = slayer_size; layer < MAX_LAYER; layer++) {

      switch(sampling_rate) {
      case 48000:
      case 44100:
        layer_max_freq[layer] = layer_max_freq[layer-1] + 8;

        if ((layer_max_freq[layer]%32) != 0)
          layer_max_freq[layer] += 4;
        break;
      case 32000:
      case 24000:
      case 22050:
        layer_max_freq[layer] = layer_max_freq[layer-1] + 16;
        break;
      case 16000:
      case 12000:
      case 11025:
        layer_max_freq[layer] = layer_max_freq[layer-1] + 32;
        break;
      default:
        layer_max_freq[layer] = layer_max_freq[layer-1] + 64;
        break;
      }
      if (layer_max_freq[layer] > last_freq[g])
        layer_max_freq[layer] = last_freq[g];

      layer_max_cband[layer] = (layer_max_freq[layer]+31)/32;
    }


    /*****************************/
    /* calculate layer_bit_flush */
    /*****************************/
    for (layer = 0; layer < (enc_top_layer+slayer_size-1); layer++) { 
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
      int	end_layer;

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
  }

  return slayer_size;
}

