/***********

This software module was originally developed by

Rongshan Yu (Institute for Infocomm Research)

and edited by

in the course of development of the MPEG-4 extension 5 
audio scalable lossless (SLS) coding . This software module is an 
implementation of a part of one or more MPEG-4 Audio tools as 
specified by the MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. 

Copyright 2003.  

***********/
/******************************************************************** 
/
/ filename : lit_ll_dec.c
/ project : MPEG-4 audio scalable lossless coding
/ author : RSY, Institute for Infocomm Research <rsyu@i2r.a-star.edu.sg>
/ date : Oct., 2003
/ contents/description : AC/BPGC decoding, inverse error mapping
/    

*/


#include "all.h"
#include "lit_ll_dec.h"
#include "lit_ll_en.h"

#include "acod.h"
#include "cbac.h"
#include "int_compact_tables.h"

#include <string.h>   /* memset */

#define MDCT_SCALE 118
#define MDCT_SCALE_MS 116 
#define MDCT_SCALE_SHORT 112
#define MDCT_SCALE_SHORT_MS 110



#define BPGC_LAZY 4
#define CBAC_LAZY 6

enum {
	InsigBand = 0,
	SigBand ,
	ExplicitInsigBand
};

#define GARBAGE_BIT 14

#define MAX_QUANT 8192


static unsigned int bpc_mask[32]=
{ 0x00000001,0x00000002,0x00000004,0x00000008,0x00000010,0x00000020,0x00000040,0x00000080,
  0x00000100,0x00000200,0x00000400,0x00000800,0x00001000,0x00002000,0x00004000,0x00008000,
  0x00010000,0x00020000,0x00040000,0x00080000,0x00100000,0x00200000,0x00400000,0x00800000,
  0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000
};	


static int bpgc_reconst[5] = {0x007FFFFF,0x006AAAAB,0X001EEEEF,0x0016FEFF,0x000BFEFF}; /*24bit fixed-point bpgc reconstruction value*/

static int silence_freq[4][3] = {{12603,7447,6302},{9638,3344,745},{6554,1820,552},{3810,16384,16384}};
static int half_freq = 8192;

void lit_ll_deinterleave(int inptr[], int outptr[], byte * group, LIT_LL_quantInfo *quantInfo)
{
  int i, j, k, l;
  int *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
  short cell_inc, subgroup_inc;
  short ngroups,nsubgroups[8];
  int ncells;
  short cellsize[MAXBANDS];

/*  ncells = quantInfo->info->sfb_per_sbk[0];*/
  for (j=0; j<ncells; j++) {
/*    cellsize[j] = quantInfo->info->sfb_width_short[j]; //quantInfo->info->sfb_width_128[j];*/
  }
  for (j=ncells; j<ncells+quantInfo->num_osf_sfb; j++) {
    cellsize[j] = quantInfo->osf_sfb_width;
  }
  ncells += quantInfo->num_osf_sfb;
  
  j = 0;
  i = 0;
  do  {
    nsubgroups[i]=group[i]-j;
    j=group[i];
    i++;
  } while (j<8);
  ngroups=i;
	

  start_subgroup_ptr = outptr;

  for (i = 0; i < ngroups; i++)
    {
      cell_inc = 0;
      start_inptr = inptr;

      /* Compute the increment size for the subgroup pointer */

      subgroup_inc = 0;
      for (j = 0; j < ncells; j++) {
        subgroup_inc += cellsize[j];
      }

      /* Perform the deinterleaving across all subgroups in a group */

      for (j = 0; j < ncells; j++) {
        subgroup_ptr = start_subgroup_ptr;

        for (k = 0; k < nsubgroups[i]; k++) {
          outptr = subgroup_ptr + cell_inc;
          for (l = 0; l < cellsize[j]; l++) {
            *outptr++ = *inptr++;
          }
          subgroup_ptr += subgroup_inc;
        }
        cell_inc += cellsize[j];
      }
      start_subgroup_ptr += (inptr - start_inptr);
    }
}







/*
void LIT_ll_read(LIT_LL_decInfo *ll_Info)
{
	int i;

	ll_byte_align();
	ll_Info->stream_len = ll_getbits(8);
	ll_Info->stream_len += ll_getbits(8)<<8;
	for (i = 0; i<ll_Info->stream_len;i++)
		ll_Info->stream[i] = ll_getbits(8);
}
*/

void deinterleave_grp2_osf(int* int_spectral_line_vector, int osf)
     
{
  int i;
  int tmp_spec[MAX_OSF*1024];
  int N = osf*1024;
  int Nshort = osf*128;
  for (i=0; i<N; i++) {
    tmp_spec[i] = int_spectral_line_vector[i];
  }
  for (i=0; i<Nshort; i++) {
    int_spectral_line_vector[i] = tmp_spec[2*i];
    int_spectral_line_vector[Nshort+i] = tmp_spec[2*i+1];
    int_spectral_line_vector[2*Nshort+i] = tmp_spec[2*Nshort+2*i];
    int_spectral_line_vector[3*Nshort+i] = tmp_spec[2*Nshort+2*i+1];
    int_spectral_line_vector[4*Nshort+i] = tmp_spec[4*Nshort+2*i];
    int_spectral_line_vector[5*Nshort+i] = tmp_spec[4*Nshort+2*i+1];
    int_spectral_line_vector[6*Nshort+i] = tmp_spec[6*Nshort+2*i];
    int_spectral_line_vector[7*Nshort+i] = tmp_spec[6*Nshort+2*i+1];
  }
}

void LIT_ll_Dec(LIT_LL_decInfo *ll_Info,
                LIT_LL_quantInfo *quantInfo_layer[MAX_TF_LAYER],
                /*byte*/ int mask[60],
                byte hasmask,
/*                byte ** cb_mask,*/
                int *rec_spectrum,
                Ch_Info *channelInfo,
                int sampling_rate,
                int osf,
                int type_PCM,
                short * block_type_lossless_only,
                int *msStereoIntMDCT,
                int coreenable,
                int ch,
                int num_aac_layers
                )
{
  int i,j,ii,grp,sfb;  
  int band_type[MAX_OSF*MAXBANDS];
  int sig_rec[MAX_OSF*1024];
  int ref[MAX_OSF*1024];
  int quant_aac[MAX_OSF*1024];
  int init,m0,m;
  int frame_type;
  ac_coder coder;
  int p_significant_error[MAX_OSF*1024],p_significant_error_bak[MAX_OSF*1024];
  int	*p_bpc_L = ll_Info->bpc_L; /* lazy layer for BPC */
  int	*p_bpc_maxbitplane = ll_Info->bpc_maxbitplane; /* maximum bit plane for BPC */
  int bpc_interval[MAX_OSF*1024];
  int significant_band[MAXBANDS];
  int band_type_signaling;
  int noise_floor_reached;
  long total_bits = ll_Info->stream_len*8;
  int isTrunc;
  int p_significant_core[MAX_OSF*1024];
  int amp_rec[MAX_OSF*1024];
  int is_lazy;
  
  /*******  quantInfo of max sfb layer  *****/
  int lay;
  int overall_scale_int,overall_scale_res;

  LIT_LL_quantInfo* quantInfo = NULL; /* &quantInfo_temp; */



  quantInfo = quantInfo_layer[num_aac_layers-1];

  /********************************************/


  if (channelInfo->ch_is_left) {
    *block_type_lossless_only = 0;
  }
    
  memset(ref,0,1024*osf*sizeof(int));
  memset(quant_aac,0,1024*osf*sizeof(int));
  
  memset(p_significant_error,0,1024*osf*sizeof(int));

  ac_decoder_init(&coder,ll_Info->stream,total_bits);
  
  if ((channelInfo->cpe && channelInfo->ch_is_left) || (!channelInfo->cpe))
  {
     read_bits(&coder, 4); /*element instance tag*/
  }
  
  read_bits(&coder,1); /* lle_reserved_bit */
  
  if (channelInfo->cpe && channelInfo->common_window &&channelInfo->ch_is_left) {  
     *msStereoIntMDCT = read_bits(&coder,1);
  }

  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {

      int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
	  band_type[sfb_sls] = ExplicitInsigBand;
    }
  }

  if (coreenable) {

    band_type_signaling = read_bits(&coder,2);

    if (band_type_signaling == 1) {
      
      for (grp=0; grp<quantInfo->num_window_groups; grp++) {
        for (sfb=0; sfb<quantInfo->max_sfb; sfb++) {
          int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      
          if (!read_bits(&coder,1)) {
            if (band_type[sfb_sls] == SigBand) {
              band_type[sfb_sls] = ExplicitInsigBand;
            }
          }
        }	
      }

    } else if (band_type_signaling == 2) {
	
      for (grp=0; grp<quantInfo->num_window_groups; grp++) {
        for (sfb=0; sfb<quantInfo->max_sfb; sfb++) {
          int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      
          if (band_type[sfb_sls] == SigBand) {
            band_type[sfb_sls] = ExplicitInsigBand;
          }
        }	
      }  
    }

  } else /* no core */ { 

    if ((channelInfo->cpe && channelInfo->ch_is_left) || (!channelInfo->cpe)) {
      *block_type_lossless_only = read_bits(&coder,2);
    }
    for (grp=0; grp<quantInfo->num_window_groups; grp++) {
      for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
        int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
        band_type[sfb_sls] = InsigBand;
      }
    }
  }

  for (i=0; i<1024; i++) {
    quant_aac[i] = quantInfo->quantCoef[i];
  }
  
  i=0;

/**********************EDIT SCA_AAC HERE!!!   **********/
  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb; sfb++) {
      int sfb_aac = grp * quantInfo->num_sfb + sfb;
      int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      int dummy=0;
      int overall_scale, mdct_scale;

/*     if ((quantInfo->info->islong)) to do add short block*/
      if (1) 
      {
        if (hasmask && (mask != NULL)) /* M/S L/R compensation */
          mdct_scale = (mask[sfb_aac])? MDCT_SCALE_MS:MDCT_SCALE;
        else
          mdct_scale = MDCT_SCALE;
      }	else {
        if (hasmask && (mask != NULL)) /* M/S L/R compensation */
          mdct_scale = (mask[sfb_aac])? MDCT_SCALE_SHORT_MS:MDCT_SCALE_SHORT;
        else
          mdct_scale = MDCT_SCALE_SHORT;
      }
      
      for (i=0; i<quantInfo->sls_sfb_width[sfb_sls]; i++) {		
        int i_aac = quantInfo->aac_sfb_offset[sfb_aac]+i;
        int i_sls = quantInfo->sls_sfb_offset[sfb_sls]+i;
      
		/* Assume all quant_aac[i] != 0 */ 
		{

		  switch (band_type[sfb_sls])
		  {
		  case ExplicitInsigBand:
			  {
				  int invQuant_sum = 0;

				  for (lay = 0; lay < num_aac_layers; lay++)
					  {
						int quant;

						if (quantInfo_layer[lay]->ll_present){

							overall_scale = quantInfo_layer[lay]->scale_factor[sfb] - SF_OFFSET - mdct_scale + scale_pcm[type_PCM]*4;
							overall_scale_int = (int)floor((double)overall_scale/4);
							overall_scale_res = overall_scale - overall_scale_int*4;

							quant = abs(quantInfo_layer[lay]->quantCoef[i_aac]);

							invQuant_sum += ((quantInfo_layer[lay]->quantCoef[i_aac]>0)?1:-1)*invQuantFunction(quant, overall_scale);	
						}
				  }

				ref[i_sls] = invQuant_sum;

				bpc_interval[i_sls] = 0x7fffffff;
				p_significant_error[i_sls] = 0;
			  }
			  break;

		  default:
			  return /* -1 */;   /* avoid compiler warning */

		  } /* switch */
        }

        if (band_type[sfb_sls] == SigBand) {
          if (dummy<j) dummy = j;
        }  
      } /* for (; i<sfb_offset[s]; i++) */
    
      if (band_type[sfb_sls] == SigBand) {
        p_bpc_maxbitplane[sfb_sls] = dummy;
        significant_band[sfb_sls] = 1;
      } else  /* INSIG band */ {
        significant_band[sfb_sls] = 0;
      }

    } 
  } /* for (s = 0; s < total_sfb+num_osf_sfbs; s++) */
/**********************EDIT SCA_AAC HERE!!!   **********/

  /* oversampling range */
  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=quantInfo->num_sfb; 
         sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
      int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      band_type[sfb_sls] = InsigBand;
      significant_band[sfb_sls] = 0;
      for (i=quantInfo->sls_sfb_offset[sfb_sls]; 
           i<quantInfo->sls_sfb_offset[sfb_sls+1]; i++) {		
        bpc_interval[i] = 0x7fffffff;
        ref[i] = 0;
        p_significant_error[i] = 0;
      }

    }
  }

  
  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    init = 0;
    for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
      int s = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      if (band_type[s] != SigBand) {
        if (!init) {
          p_bpc_maxbitplane[s] = read_bits(&coder,5) - 1;
          init ++;
        } else {
          m = 0;
          while (read_bits(&coder,1) == 0)
            m++;
          if (m) {
            if (read_bits(&coder,1))
              m = -m;
          }
          p_bpc_maxbitplane[s] = m0 - m;
        }
        m0 = p_bpc_maxbitplane[s];
      }
      if (p_bpc_maxbitplane[s]>=0) {
        if (read_bits(&coder,1)==0) {
          p_bpc_L[s] = 2;
        } else {
          if (read_bits(&coder,1)==0)
            p_bpc_L[s] = 1;
          else
            p_bpc_L[s] = 3;
        }
      }
    }
  }

  isTrunc = 0;
      
  frame_type = read_bits(&coder,1);
  
  for (i=0;i<osf*1024;i++) {
    p_significant_error_bak[i] = p_significant_core[i] = p_significant_error[i];
    amp_rec[i] = 0;
    sig_rec[i] = 1;
  }
  
  ii = 0;
  noise_floor_reached = 0;
  is_lazy = 0;
  while(!noise_floor_reached) {
    noise_floor_reached = 1;
    for (grp=0; grp<quantInfo->num_window_groups; grp++)
      for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
        int s = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
        if ( (p_bpc_maxbitplane[s] >= ii) && (p_bpc_maxbitplane[s] - p_bpc_L[s] > 0) ) {
          int bit_plane = p_bpc_maxbitplane[s] - ii;
          int lazy_plane = p_bpc_L[s] - ii + 1;
          
          noise_floor_reached = 0;
          for (i=quantInfo->sls_sfb_offset[s]; i<quantInfo->sls_sfb_offset[s+1]; i++) {
            int sym;
            int freq;
            int int_cx;
	  
            if (bpc_interval[i] > amp_rec[i]+ bpc_mask[bit_plane])  /*null context int_cx = 2*/ {
              int_cx = (bpc_interval[i] >= amp_rec[i]+ 2*bpc_mask[bit_plane]) ? 1:0 ;
              freq = cbac_model(i,
                                frame_type,
                                sampling_rate,
                                lazy_plane,
                                p_significant_error_bak,
                                significant_band[s],
                                int_cx,
                                osf);
              sym = ac_decode_symbol (&coder, freq);
              amp_rec[i] += sym*bpc_mask[bit_plane];
              if ((p_significant_error[i]==0) && sym) {
                sym = ac_decode_symbol (&coder, half_freq);
                sig_rec[i] = (sym)? -1:1;
                p_significant_error[i] = 1;
              }
              if (ac_bits(&coder)  > total_bits+GARBAGE_BIT) 
                {
                  isTrunc = 1; /*?*/
                  goto FILL_ELEMENT;
                }
              
            } 
          } /*for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)*/
        } /*if (p_bpc_maxbitplane[s] >= ii)*/
      }	/*	for (s=0;s<total_sfb;s++)*/
    memcpy(p_significant_error_bak, 
           p_significant_error,
           1024*osf*sizeof(int));
    
    ii++;
    /* LAZY_CODING */
    if ( ( (ii==CBAC_LAZY) && (frame_type==0) ) || ( (ii==BPGC_LAZY) && (frame_type==1) ) ) {
      is_lazy = 1;
      goto LAZYMODE_CODING;
    }

  } /* while*/
  
LAZYMODE_CODING:

  if (is_lazy) {
    read_bits(&coder,2); /*skip terminationg string*/
    while(!noise_floor_reached) {
      noise_floor_reached = 1;
      for (grp=0; grp<quantInfo->num_window_groups; grp++)
        for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
          int s = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
          if ( (p_bpc_maxbitplane[s] >= ii) && (p_bpc_maxbitplane[s] - p_bpc_L[s] > 0) ) {
            int bit_plane = p_bpc_maxbitplane[s] - ii;
/*             int lazy_plane = p_bpc_L[s] - ii + 1; */	
            noise_floor_reached = 0;
            for (i=quantInfo->sls_sfb_offset[s]; i<quantInfo->sls_sfb_offset[s+1]; i++) {
              int sym;
              
              if  (bpc_interval[i] > amp_rec[i]+ bpc_mask[bit_plane]) /*null context int_cx = 2*/ {
                sym = read_bits(&coder,1);
                amp_rec[i] += sym*bpc_mask[bit_plane];
                if ((p_significant_error[i]==0) && sym) {
                  sym = read_bits(&coder,1);
                  sig_rec[i] = (sym)? -1 : 1;
                  p_significant_error[i] = 1;
                }
                if (ac_bits(&coder)  > total_bits+GARBAGE_BIT) {
                  isTrunc = 1; /*?*/
                  goto FILL_ELEMENT;
                }
              } 
            } /*for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)*/
          } /*if (p_bpc_maxbitplane[s] >= ii)*/
        }		/*	for (s=0;s<total_sfb;s++)*/
    
      ii++;
    } /* while*/
  }

FILL_ELEMENT:

  
  if (isTrunc) {
    for (grp=0; grp<quantInfo->num_window_groups; grp++)
      for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
        int s = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
        if ( (p_bpc_maxbitplane[s] >= ii) && (p_bpc_maxbitplane[s] - p_bpc_L[s] > 0) ) {
          int bit_plane = p_bpc_maxbitplane[s] - ii;
          int lazy_plane = p_bpc_L[s] - ii + 1;
		  
          if (lazy_plane <0) lazy_plane = 0;
	  
          for (i=quantInfo->sls_sfb_offset[s]; i<quantInfo->sls_sfb_offset[s+1]; i++) {

            if ((p_significant_error[i]) /*(s>0) && (i != sfb_offset[s-1])*/) {
              if (amp_rec[i] > 0){
                amp_rec[i] += (bpgc_reconst[lazy_plane] >> (23-bit_plane)) ;
              }
            }
          } /*for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)*/
        } /*if (p_bpc_maxbitplane[s] >= ii)*/
      }		/*	for (s=0;s<total_sfb;s++)*/  
  }

  /* LOW ENERGY MODE */

  if (!isTrunc)
  {
    for (grp=0; grp<quantInfo->num_window_groups; grp++)
      for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
        int s = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
        int lazy_plane = p_bpc_maxbitplane[s] - p_bpc_L[s];
        if ((p_bpc_maxbitplane[s] >= 0) && (lazy_plane <= 0)) {
          for (i=quantInfo->sls_sfb_offset[s]; i<quantInfo->sls_sfb_offset[s+1]; i++) {
            int bin = 0;
            int sym=0;
	  
            while (ac_decode_symbol(&coder,silence_freq[-lazy_plane][bin])==1) {
              sym++;
              if (sym==(1<<(p_bpc_maxbitplane[s]+1))-1) break;
              bin++;
              if (bin >2) bin = 2;
            }
            amp_rec[i] += sym;
            if ((p_significant_error[i] == 0)&&sym) {
              sig_rec[i] = (ac_decode_symbol(&coder,half_freq))? -1:1;
              p_significant_error[i] = 1;
            }
            if (ac_bits(&coder)  > total_bits+GARBAGE_BIT) {
              isTrunc = 1; /*?*/
              goto INVERSE_ERROR_MAPPING;
            }
          }
        }
      }
  }

INVERSE_ERROR_MAPPING:

   for (grp=0; grp<quantInfo->num_window_groups; grp++) {
     for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
       int sfb_aac = grp * quantInfo->num_sfb + sfb;
       int sfb_sls = grp * quantInfo->num_sfb + grp * 
quantInfo->num_osf_sfb + sfb;

       if (band_type[sfb_sls] == InsigBand) {
         for (i=quantInfo->sls_sfb_offset[sfb_sls];
              i<quantInfo->sls_sfb_offset[sfb_sls+1]; i++) {
           rec_spectrum[i] = amp_rec[i]*sig_rec[i];
         }
       } else {
         for (i=0; i<quantInfo->sls_sfb_width[sfb_sls]; i++) {         
           int i_aac = quantInfo->aac_sfb_offset[sfb_aac]+i;
           int i_sls = quantInfo->sls_sfb_offset[sfb_sls]+i;
           rec_spectrum[i_sls] = ref[i_sls] + amp_rec[i_sls]*sig_rec[i_sls];

         }
       }
     }
   }


  
}

