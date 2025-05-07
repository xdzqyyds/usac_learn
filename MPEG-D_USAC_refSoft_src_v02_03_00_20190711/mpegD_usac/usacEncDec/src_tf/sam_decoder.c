/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-29
  Doh-Hyung Kim (Samsung AIT) on 2003-07-02

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
#include <math.h>

#include "allHandles.h"

#include "port.h"
#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "sam_tns.h"             /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#include "common_m4a.h"
#include "bitstream.h"
#include "tf_main.h"
#include "sam_dec.h"

#define SF_OFFSET   100
#define TEXP    128
#define MAX_IQ_TBL  128

static int swb_offset_long[52];
static int swb_offset_short[16];

static int      long_block_size;
static int      short_block_size;
static int      long_sfb_top;
static int      short_sfb_top;
static int      fs_idx;

#ifdef  __cplusplus
extern "C" {
#endif

static double sam_calc_scale(int fac);
static double sam_iquant_exp(int q);
static void sam_gen_rand_vector(double *spec, int size, long *state);

static long sam_random2( long *seed );

#ifdef  __cplusplus
}
#endif

static double  sam_exptable[TEXP];
static double  sam_iq_exp_tbl[MAX_IQ_TBL];

void sam_decode_bsac_data ( int    target,
                            int    frameLength,
                            int    sba_mode,
                            int    top_layer,
                            int    base_snf_thr,
                            int    base_band,
                            int    maxSfb,
                            WINDOW_SEQUENCE  windowSequence,
                            int    num_window_groups,
                            int    window_group_length[8],

                            int    max_scalefactor[],
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
                            int    used_bits,
                            int    samples[][1024],
                            int    scalefactors[][MAX_SCFAC_BANDS],
                            int    nch
                            /* 20060107 */
                            ,int	   *core_bandwidth
                            )
{
  int     b, w, i;
  int     sfb;
  int     top_band = long_sfb_top;
  int     swb_offset[8][52];
  int     sample_buf[2][1024];

  target = (target - 16);

  if(windowSequence == EIGHT_SHORT_SEQUENCE) {
    b = 1;
    for(w = 0; w < num_window_groups; w++, b++) {
      swb_offset[w][0] = 0;
      for(sfb = 0; sfb < short_sfb_top; sfb++) {
        swb_offset[w][sfb+1] = swb_offset_short[sfb+1] * window_group_length[w];
      }
    }
    top_band = short_sfb_top;
  } else {
    swb_offset[0][0] = 0;
    for(sfb = 0; sfb < long_sfb_top+1; sfb++)
      swb_offset[0][sfb+1] = swb_offset_long[sfb+1];
    top_band = long_sfb_top;
  }

  sam_decode_bsac_stream ( target, 
                           frameLength,

                           sba_mode, 
                           top_layer, 
                           base_snf_thr, 
                           base_band, 

                           maxSfb, 
                           windowSequence,
                           num_window_groups, 
                           window_group_length, 

                           max_scalefactor,
                           cband_si_type,
                           scf_model0,
                           scf_model1,
                           max_sfb_si_len,

                           stereo_mode,
                           ms_mask, 
                           is_info,
                           pns_data_present,
                           pns_sfb_start,
                           pns_sfb_flag,
                           pns_sfb_mode,
                           swb_offset, 
                           top_band, 
                           used_bits,
                           sample_buf,
                           scalefactors, 
                           nch
                           /* 20060107 */
                           ,core_bandwidth
                           );


  if(windowSequence == EIGHT_SHORT_SEQUENCE) {
    int offset, b, s, k;
    s = 0;
    for(w = 0; w < num_window_groups; w++) {
      for(i = 0; i < 128; i+=4) {
        offset = (128 * s) + (i * window_group_length[w]);
        for(k = 0; k < 4; k++) for(b = 0; b < window_group_length[w]; b++) {
          samples[0][128*(b+s)+i+k] = sample_buf[0][offset+4*b+k];
          samples[1][128*(b+s)+i+k] = sample_buf[1][offset+4*b+k];
        }
      }
      s += window_group_length[w];
    }
  } else {
    for(i = 0; i < 1024; i++) {
      samples[0][i] = sample_buf[0][i];
      samples[1][i] = sample_buf[1][i];
    }
  }
}

void sam_dequantization(int target,
    int windowSequence,
    int scalefactors[MAX_SCFAC_BANDS],
    int num_window_groups,
    int window_group_length[8],
    int samples[],
    int maxSfb,
    int is_info[],
    double decSpectrum[],
    int ch)
{
  int     sfb, i, k;
  double   scale;

  target = (target - 16);

  for(i = 0; i < 1024; i++)
    decSpectrum[i] = -0.0;

  if(windowSequence != EIGHT_SHORT_SEQUENCE) {

    for(sfb = 0; sfb < maxSfb; sfb++) {
      if(ch == 1 && is_info[sfb]) continue;

      scale = sam_calc_scale(scalefactors[sfb]-SF_OFFSET);

      for(i = swb_offset_long[sfb]; i < swb_offset_long[sfb+1]; i++) {
          decSpectrum[i] = sam_iquant_exp(samples[i]) * scale;
      }
    }

  } else {
    int w, b, s, iquant;

    /* resuffling and scaling the decoded spectrum */
    s = 0;
    for(w = 0; w < num_window_groups; w++) {
      for(sfb = 0; sfb < maxSfb; sfb++) {
        if(ch == 1 && is_info[(w*maxSfb)+sfb]) continue;

        scale = sam_calc_scale(scalefactors[w*maxSfb+sfb]-SF_OFFSET);

        for (b = 0; b < window_group_length[w]; b++) {
          k = (s + b) * 128;
          for (i = swb_offset_short[sfb]; i < swb_offset_short[sfb+1]; i++) {
            iquant = samples[k+i];
            decSpectrum[k+i] = sam_iquant_exp(iquant) * scale;
          }
        }
      }
      s += window_group_length[w];
    }
  }
}

void sam_ms_stereo(
    int windowSequence,
    int num_window_groups,
    int window_group_length[8],
    double spectrum[][1024],
    int ms_mask[],
    int maxSfb)
{
  double   l, r, tmp;
  int     sfb, i;

  if(windowSequence != EIGHT_SHORT_SEQUENCE) {
    for(sfb = 0; sfb < maxSfb; sfb++) {
      if(ms_mask[sfb+1] == 0) continue;
      for(i = swb_offset_long[sfb]; i < swb_offset_long[sfb+1]; i++) {
        l = spectrum[0][i];
        r = spectrum[1][i];

        tmp = l + r;
        r = l - r;
        l = tmp;

        spectrum[0][i] = l;
        spectrum[1][i] = r;
      }
    }
  } else {
    int w, b, s;

    s = 0;
    for(w = 0; w < num_window_groups; w++) {
      for(b = s; b < s+window_group_length[w]; b++) {
        for(sfb = 0; sfb < maxSfb; sfb++) {
          if(ms_mask[w*maxSfb+sfb+1]==0)continue;
          for(i = swb_offset_short[sfb]; i < swb_offset_short[sfb+1]; i++) {
            l = spectrum[0][b*128+i];
            r = spectrum[1][b*128+i];

            tmp = l + r;
            r = l - r;
            l = tmp;

            spectrum[0][b*128+i] = l;
            spectrum[1][b*128+i] = r;
          }
        }
      }
      s += window_group_length[w];
    }
  }
}

/*
 *  for PNS
 */
#define MEAN_NRG 1.5625e+18 /* Theory: (2^31)^2 / 3 = 1.537228e+18 */
static long sam_random2( long *seed )
{
  *seed = (1664525L * *seed) + 1013904223L; /* Numerical recipes */
  return(long)(*seed);
}

static void sam_gen_rand_vector(double *spec, int size, long *state)
{
  int i;
  double s, norm, nrg=0.0;

  norm = 1.0 / sqrt(size * MEAN_NRG);

  for(i=0; i<size; i++) {
    spec[i] = (double)(sam_random2(state) * norm);
    nrg += spec[i] * spec[i];
  }
  s = 1.0 / sqrt( nrg );
  for(i=0; i<size; i++)
    spec[i] *= s;
}

void sam_pns ( int     maxSfb,
               int windowSequence, /* SAMSUNG_2005-09-30 */
               int num_window_groups, /* SAMSUNG_2005-09-30 */
               int window_group_length[8], /* SAMSUNG_2005-09-30 */ 	
               int     pns_sfb_flag[][MAX_SCFAC_BANDS],
               int     pns_sfb_mode[MAX_SCFAC_BANDS],
               double   spectrums[][1024],
               int     factors[][MAX_SCFAC_BANDS],
               int     nch )
{
  int    i, size, ch, sfb;
	int w, b, s;
  int    corr_flag;
  long   *nsp;
  double  *fp, scale;
  static long cur_noise_state;
  static long noise_state_save[136];

	if(windowSequence != EIGHT_SHORT_SEQUENCE)
	{
		for(ch = 0; ch < nch; ch++) {
			nsp = noise_state_save;
			for(sfb = 0; sfb < maxSfb; sfb++) {
				if(pns_sfb_flag[ch][sfb] == 0)
					continue;
				corr_flag = pns_sfb_mode[sfb];
				fp = spectrums[ch]+swb_offset_long[sfb];
				size = swb_offset_long[sfb+1]-swb_offset_long[sfb];
				if(corr_flag) {
					sam_gen_rand_vector(fp, size, nsp+sfb);
				} else {
					nsp[sfb] = cur_noise_state;
					sam_gen_rand_vector(fp, size, &cur_noise_state);
				}
				scale = pow(2.0, 0.25*factors[ch][sfb]);
				for(i = swb_offset_long[sfb]; i < swb_offset_long[sfb+1]; i++)
					*fp++ *= scale;
			}
		}
	}
	else
	{
		for(ch = 0; ch < nch; ch++) {
			s = 0; nsp = noise_state_save;
			for(w = 0; w < num_window_groups; w++) {  /* SAMSUNG_2005-09-30 */			
			for(b = 0; b < window_group_length[w]; b++, s++) {  /* SAMSUNG_2005-09-30 */  				
				for(sfb = 0; sfb < maxSfb; sfb++) {
					if(pns_sfb_flag[ch][w*maxSfb+sfb] == 0)
						continue;
					corr_flag = pns_sfb_mode[w*maxSfb+sfb];
					fp = spectrums[ch]+(s*128)+swb_offset_short[sfb];
					size = swb_offset_short[sfb+1]-swb_offset_short[sfb];
					if(corr_flag) {
						sam_gen_rand_vector(fp, size, nsp+sfb);
					} else {
                                          nsp[w*maxSfb+sfb] = cur_noise_state;
                                          sam_gen_rand_vector(fp, size, &cur_noise_state);
					}
					scale = pow(2.0, 0.25*factors[ch][w*maxSfb+sfb]);
					for(i = swb_offset_short[sfb]; i < swb_offset_short[sfb+1]; i++)
                                          *fp++ *= scale;
				}
			}
			}
		}
                
	}
}

void sam_intensity ( int   windowSequence,
                     int   num_window_groups,
                     int   window_group_length[8],
                     double spectrum[][1024],
                     int   factors[][MAX_SCFAC_BANDS],
                     int   is_info[], 
                     int   maxSfb )
{
  int     i, sfb;
  int     sign_sfb;
  double   scale;
  if(windowSequence != EIGHT_SHORT_SEQUENCE) {
    for(sfb = 0; sfb < maxSfb; sfb++) {
      if(is_info[sfb] == 0) continue;

      sign_sfb = (is_info[sfb] == 1) ? 1 : -1;

      scale = sign_sfb * pow( 0.5, 0.25*factors[1][sfb]);

      for(i = swb_offset_long[sfb]; i < swb_offset_long[sfb+1]; i++) {
        spectrum[1][i] = spectrum[0][i] * scale;
      }
    }
  } else {
    int w, b, band, s;
    
    s = 0;
    for(w = 0; w < num_window_groups; w++) {
      for(b = 0; b < window_group_length[w]; b++, s++) {
        for(sfb = 0; sfb < maxSfb; sfb++) {
          band = (w*maxSfb)+sfb;
          if(is_info[band]==0)continue;
          
          sign_sfb = (is_info[band] == 1) ? 1 : -1;
          
          scale = sign_sfb * pow( 0.5, 0.25*factors[1][w*maxSfb+sfb]);
          
          for(i = swb_offset_short[sfb]; i < swb_offset_short[sfb+1]; i++)
            spectrum[1][s*128+i] = spectrum[0][s*128+i] * scale;
          /*spectrum[1][i] = spectrum[0][i] * scale;      */     /* Nice hyungk !!! (2003.07.02) */
        }
      }
    }
  }
}

void
sam_tns_data(int windowSequence, int maxSfb, double spect[1024], sam_TNS_frame_info *tns)
{
  int     i;
  int     blocks;
  int     *sfbs;

  if(windowSequence == 2) {
    sfbs = (int *)swb_offset_short;
    blocks = 8;
  } else {
    sfbs = (int *)swb_offset_long;
    blocks = 1;
  }

  for(i = 0; i < blocks; i++) {
    sam_tns_decode_subblock(windowSequence, spect+(i*short_block_size), maxSfb, sfbs, &(tns->info[i]));
  }
}

static double sam_calc_scale(int fac)
{
  double   scale;

  if(fac >= 0 && fac < TEXP) {
    scale = sam_exptable[fac];
  } else {
    if(fac == -SF_OFFSET)
      scale = 0.;
    else
      scale = pow((double)2.0, (double)(0.25*fac));
  }

  return scale;
}

static double sam_iquant_exp(int q)
{
  double return_value;

  if (q > 0) {
    if (q < MAX_IQ_TBL) {
      return((double)sam_iq_exp_tbl[q]);
    } else {
      return_value = (double)(pow((double)q, (double)(4./3.)));
    }
  } else {
    q = -q;
    if (q < MAX_IQ_TBL) {
      return((double)(-sam_iq_exp_tbl[q]));
    } else {
      return_value = (double)(-pow((double)q, (double)(4./3.)));
    }
  }

  return return_value;
}

void sam_decode_init(int sampling_rate_decoded, int block_size_samples)
{
  int     i;

  for(i = 0; i < TEXP; i++)
    sam_exptable[i] = (double)pow((double)2.0, (double)(0.25*i));

  for(i = 0; i < MAX_IQ_TBL; i++)
    sam_iq_exp_tbl[i] = (double)pow((double)i, (double)4./3.);

  sam_scale_bits_init(sampling_rate_decoded, block_size_samples);

  long_block_size = block_size_samples;
  short_block_size = long_block_size / 8;

  fs_idx = Fs_48;
  for(i = 0; i < (1<<LEN_SAMP_IDX); i++) {
    if(sampling_rate_decoded == samp_rate_info[i].samp_rate) {
      fs_idx = i;
      break;
    }
  }

  if(block_size_samples == 1024) {
    long_sfb_top = samp_rate_info[fs_idx].nsfb1024;
    short_sfb_top = samp_rate_info[fs_idx].nsfb128;
    swb_offset_long[0] = 0;
    for(i = 0; i < long_sfb_top; i++)
      swb_offset_long[i+1] = samp_rate_info[fs_idx].SFbands1024[i];
    swb_offset_short[0] = 0;
    for(i = 0; i < short_sfb_top; i++)
      swb_offset_short[i+1] = samp_rate_info[fs_idx].SFbands128[i];
  } else if(block_size_samples == 960) {
    long_sfb_top = samp_rate_info[fs_idx].nsfb960;
    short_sfb_top = samp_rate_info[fs_idx].nsfb120;
    swb_offset_long[0] = 0;
    for(i = 0; i < long_sfb_top; i++)
      swb_offset_long[i+1] = samp_rate_info[fs_idx].SFbands960[i];
    swb_offset_short[0] = 0;
    for(i = 0; i < short_sfb_top; i++)
      swb_offset_short[i+1] = samp_rate_info[fs_idx].SFbands120[i];
  }
}

int sam_tns_top_band(int windowSequence)
{
  if(windowSequence == 2) return short_sfb_top;
  else return long_sfb_top;
}

int sam_tns_max_bands(int windowSequence)
{
  return tns_max_bands((windowSequence != 2), TNS_BSAC, fs_idx);
}

int sam_tns_max_order(int windowSequence)
{
  return tns_max_order((windowSequence != 2), TNS_BSAC, fs_idx);
}

/*
 *  for BISTREAM
 */

static HANDLE_BSBITSTREAM vmBitBuffer;
static int          sam_numUsedBits;

void sam_setBsacdec2BitBuffer(HANDLE_BSBITSTREAM fixed_stream)
{
  vmBitBuffer = fixed_stream;
  sam_numUsedBits = 0;
  
}

long sam_BsNumByte( void )
{
   return(BsNumByte(vmBitBuffer));
}

long sam_GetBits(int n)
{
  unsigned long value;
  if (n!=0){
    sam_numUsedBits+=n;
    BsGetBit(vmBitBuffer,&value,n);
  }
  return value;
}

int sam_getUsedBits(void)
{
  return sam_numUsedBits;
}

/* SAMSUNG_2005-09-30 : set buffer current position to n */
void sam_SetBitstreamBufPos(int n) 
{
	BsSetBufRPos(vmBitBuffer, n);
}

