/***********

This software module was originally developed by

Rongshan Yu (Institute for Infocomm Research)

and edited by

in the course of development of the MPEG-4 extension 3 
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
/ filename : lit_ll_en.c
/ project : MPEG-4 audio scalable lossless coding
/ author : RSY, Institute for Infocomm Research <rsyu@i2r.a-star.edu.sg>
/ date : Oct., 2003
/ contents/description : BPGC coding, error mapping
/    

*/

#include "all.h"
#include "drc.h"

#include "lit_ll_en.h"
#include "acod.h"
#include "cbac.h"
#include "int_compact_tables.h"

#define CBAC_LAZY 6
#define BPGC_LAZY 4

#define MDCT_SCALE 118
#define MDCT_SCALE_MS 116 
#define MDCT_SCALE_SHORT 112
#define MDCT_SCALE_SHORT_MS 110


#define LLE_ICS_HEADER 16

enum {
  ExplicitBand = 0,
  ImplicitBand ,
  ForcedExplicitBand,
  ErrBank
};

#define SQRT2 1.41421356

#define ROUND(x) ((x>=0)?(int)(x+0.5):(int)(x-0.5))
#define GarbageBits 14



static unsigned int bpc_mask[32] = 
  { 0x00000001,0x00000002,0x00000004,0x00000008,0x00000010,0x00000020,0x00000040,0x00000080,
    0x00000100,0x00000200,0x00000400,0x00000800,0x00001000,0x00002000,0x00004000,0x00008000,
    0x00010000,0x00020000,0x00040000,0x00080000,0x00100000,0x00200000,0x00400000,0x00800000,
    0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000
  };	

static int silence_freq[4][3] = {{12603,7447,6302},{9638,3344,745},{6554,1820,552},{3810,16384,16384}};
static int half_freq = 8192;

static int avoidImplicitSignaling = 1; /* set to 1 to test explicit signaling and scalable AAC */

static int num_osf_sfbs;


#ifdef I2R_LOSSLESS
int lit_ll_sfg(AACQuantInfo* quantInfo,        /* ptr to quantization information */
	       int sfb_width_table[],          /* Widths of single window */
	       int *p_spectrum,           /* Spectral values, noninterleaved */
	       int osf)
{
  int i,j,ii;
  int index = 0;
  int tmp[1024];
  int book=1;
  int group_offset=0;
  int k=0;
  int windowOffset = 0;


  /* set up local variables for used quantInfo elements */
  int* sfb_offset = quantInfo -> sfb_offset;
  int* window_group_length;
  int num_window_groups;
  int *total_sfb; 
  window_group_length = quantInfo -> window_group_length;
  num_window_groups = quantInfo -> num_window_groups;
 
  /* calc org sfb_offset just for shortblock */
  k = 0;
  sfb_offset[k]=0;

  for (k=1 ; k <*total_sfb+1; k++) {
    sfb_offset[k] = sfb_offset[k-1] + sfb_width_table[k-1];
  }


  /* sort the input spectral coefficients */

  index = 0;
  group_offset=0;
  for (i=0; i< num_window_groups; i++) 
    {
      for (k=0; k<*total_sfb; k++)
	{
	  for (j=0; j < window_group_length[i]; j++) 
	    {
	      for (ii=0;ii< sfb_width_table[k];ii++)
		tmp[index++] = p_spectrum[ii+ sfb_offset[k] + 128*j +group_offset];
	    }
	}
      group_offset +=  128*window_group_length[i];     
    }



  for (k=0; k<1024; k++){
    p_spectrum[k] = tmp[k];
  }

    

  /* now calc the new sfb_offset table for the whole p_spectrum vector*/

  index = 0;
  sfb_offset[index++] = 0;	  
  windowOffset = 0;
  for (i=0; i < num_window_groups; i++) {
    for (k=0 ; k <*total_sfb; k++) {
      sfb_offset[index] = sfb_offset[index-1] + sfb_width_table[k]*window_group_length[i] ;
      index++;
    }
    windowOffset += window_group_length[i];
  }


  *total_sfb *= num_window_groups;

  return 0;
}



void LIT_ll_perceptualEnhEnc(LIT_LL_Info * ll_Info,
			     AACQuantInfo* quantInfo,      /* AAC quantization information */ /* to be used */
			     long bits_to_use,
			     int sampling_rate,
			     int osf,
                             int total_sfb,
                             int *sfb_offset,
			     int short_nr_of_sfb,
			     int coreenable,
			     Ch_Info *channelInfo,
			     WINDOW_TYPE block_type_lossless_only,
			     int msStereoIntMDCT,
                             int num_aac_layers)
{
  int	*p_error_spectrum = ll_Info->error_spectrum; /* mapped error spectrum */
  int p_rec_spectrum[MAX_OSF*1024],p_significant_error_bak[MAX_OSF*1024];
  int p_significant_core[MAX_OSF*1024];
  int *p_significant_error = ll_Info->significant_error;
  int *p_significant_band = ll_Info->significant_band;
  int	*p_bpc_L = ll_Info->bpc_L; /* lazy layer for BPC */
  int	*p_bpc_maxbitplane = ll_Info->bpc_maxbitplane; /* maximum bit plane for BPC */
  int *bpc_interval = ll_Info->bpc_interval;
	
  int *bank_type = ll_Info->bank_type;
  
  int max_sfb = quantInfo->max_sfb;					  	  
  ac_coder coder;

  int frame_type;
  int len[4];

  int s,i,ii, lay;
  int noise_floor_reached;
  int rate_constraint_reached;
  int init,m0,m;
  int is_lazy;

  int *core_significance_signaling = &ll_Info->core_significance_signaling;
  int *core_significance = ll_Info->core_significance;


  if (block_type_lossless_only == EIGHT_SHORT_SEQUENCE/*ONLY_SHORT_WINDOW*/) {
    total_sfb = 4*short_nr_of_sfb;
    max_sfb = 4*short_nr_of_sfb;

  }

  ii = 0;
  noise_floor_reached = 0;
  rate_constraint_reached = 0;

  for (i=0;i<osf*1024;i++)
    {
      p_rec_spectrum[i] = 0;
      p_significant_core[i] = p_significant_error_bak[i] = p_significant_error[i];
    }
	
  ac_encoder_init(&coder,ll_Info->stream);


  if ((channelInfo->cpe && channelInfo->ch_is_left) || (!channelInfo->cpe))
    write_bits(&coder, channelInfo->tag, 4); /*element instance tag*/

  write_bits(&coder,0,1); /* lle_reserved_bit */

  
  /* additional side info for dynamic msStereoIntMDCT */
	if (channelInfo->cpe && channelInfo->ch_is_left&&channelInfo->common_window) {
    write_bits(&coder,msStereoIntMDCT,1);
  }
  
  if (coreenable)
    {
      write_bits(&coder,*core_significance_signaling,2);
      if (*core_significance_signaling == 1) 
	{ /* per sfb explicit core significance signaling */
	  for (s=0;s<max_sfb;s++)
	    write_bits(&coder,core_significance[s],1);
	}
    }
  else
    {
      
      /*write_bits(&coder,3,2);*/ /* use core_significance_signaling = 3 to signaling nocore at this moment */
    
      if ((channelInfo->cpe && channelInfo->ch_is_left) || (!channelInfo->cpe))
        write_bits(&coder,block_type_lossless_only,2);
    }

  init = 0;
  for (s = 0;s < total_sfb + num_osf_sfbs; s++)
    {
      if ((s>=total_sfb) || /*(quantInfo->book_vector[s] == 0) || (avoidImplicitSignaling)*/
		  (bank_type[s] != ImplicitBand))
	{
    /* reset differential coding of maxbitplane for each group */
	  if ((block_type_lossless_only == EIGHT_SHORT_SEQUENCE/*ONLY_SHORT_WINDOW*/) && 
	      (sfb_offset[s]%(256*osf) == 0)) {
	    init = 0;
	  }
	  if (!init)
	    {
	      write_bits(&coder,p_bpc_maxbitplane[s]+1,5);
	      init ++;
	    }
	  else
	    {
	      m = m0 - p_bpc_maxbitplane[s];
	      for (i=0;i<abs(m);i++)
		write_bits(&coder,0,1);
	      write_bits(&coder,1,1);
	      if (m>0)
		write_bits(&coder,0,1);
	      else if (m<0)
		write_bits(&coder,1,1);
	    }
	  m0 = p_bpc_maxbitplane[s];
	}
      if (p_bpc_maxbitplane[s]>=0)
	{
	  switch (p_bpc_L[s])
	    {
	    case 1:
	      write_bits(&coder,2,2);
	      break;
	    case 2:
	      write_bits(&coder,0,1);
	      break;
	    case 3:
	      write_bits(&coder,3,2);
	      break;
	    }
	} 
	
    }

  /* */



  frame_type = 1; /* 0: CB-BPAC, 1: BPGC */

	
  write_bits(&coder,frame_type,1);

  for (i=0;i<osf*1024;i++)
    {
      p_significant_error_bak[i] = p_significant_error[i] = p_significant_core[i];
      p_rec_spectrum[i] = 0;
    }

  ii = 0;
  noise_floor_reached = 0;
  rate_constraint_reached = 0;
  is_lazy=0;
  bits_to_use -= LLE_ICS_HEADER;

  while(!noise_floor_reached)
    {
      noise_floor_reached = 1;
      for (s=0;s<total_sfb + num_osf_sfbs;s++)
	{
	  int bit_plane = p_bpc_maxbitplane[s] - ii;
	  int lazy_plane = p_bpc_L[s]-ii+1;

	  if ((p_bpc_maxbitplane[s] >= ii) 
	      && (p_bpc_maxbitplane[s] - p_bpc_L[s] >0)
	      )
	    {
	      noise_floor_reached = 0;
				
	      for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)
		{
		  int sym = (abs(p_error_spectrum[i])&bpc_mask[bit_plane])?1:0;
		  int sgn = (p_error_spectrum[i] >=0)?0:1;
		  int freq;

		  int int_cx;

		  if ((bpc_interval[i] > p_rec_spectrum[i]+ bpc_mask[bit_plane]))
		    int_cx = (bpc_interval[i] >= p_rec_spectrum[i]+ 2*bpc_mask[bit_plane])?1:0 ;
		  else int_cx = 2;

		  freq = cbac_model(i,frame_type,sampling_rate,lazy_plane,p_significant_error_bak,p_significant_band[s],int_cx,osf);
						
		  if (freq) 
		    ac_encode_symbol(&coder,freq,sym);
		  p_rec_spectrum[i] += sym*bpc_mask[bit_plane];
				
		  if ((p_significant_error[i] == 0)&&sym)
		    {
		      ac_encode_symbol(&coder,half_freq,sgn);
		      p_significant_error[i] = 1;
		    }
		  if (ac_bits(&coder) >= (bits_to_use - 8)) /*??*/
		    {
		      rate_constraint_reached = 1;
		      goto Rate_Reached;
		    }
		}/* for (i)*/
	    }
	} /* s*/		
      for (i=0;i<osf*1024;i++)
	p_significant_error_bak[i] = p_significant_error[i];  /*update of significant states for next bp*/
      ii++; /* progress to next bit_plane*/
		
      /* LAZY_CODING */
     if ( ((ii==CBAC_LAZY) &&(frame_type==0)) || ((ii==BPGC_LAZY) &&(frame_type==1)) )
	{
	  is_lazy = 1;
	  goto LAZYMODE_CODING;
	}

    } /* while*/

LAZYMODE_CODING:
  if (is_lazy)
    {
      ac_encoder_flush(&coder); /*write terminating string*/
      while(!noise_floor_reached)
	{
	  noise_floor_reached = 1;
	  for (s=0;s<total_sfb + num_osf_sfbs;s++)
	    {
	      int bit_plane = p_bpc_maxbitplane[s] - ii;
	      int lazy_plane = p_bpc_L[s]-ii+1;
	      
	      if ((p_bpc_maxbitplane[s] >= ii) 
		  && (p_bpc_maxbitplane[s] - p_bpc_L[s] >0)
		  )
		{
		  noise_floor_reached = 0;
		  
		  for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)
		    {
		      if (bpc_interval[i] > p_rec_spectrum[i]+ bpc_mask[bit_plane]) /* int_cx=2*/
			{
			  int sym = (abs(p_error_spectrum[i])&bpc_mask[bit_plane])?1:0;
			  int sgn = (p_error_spectrum[i] >=0)?0:1;
if (ii == 10)
{
	int a;
	a=5;
}
			  write_bits(&coder,sym,1);
			  p_rec_spectrum[i] += sym*bpc_mask[bit_plane];
			  if ((p_significant_error[i] == 0)&&sym)
			    {
			      write_bits(&coder,sgn,1);
			      p_significant_error[i] = 1;
			    }
			  if (ac_bits(&coder) >= (bits_to_use - 8)) /*??*/
			    {
			      rate_constraint_reached = 1;
			      goto Rate_Reached;
			    }
			}
		    }/* for (i)*/
		}
	    } /* s*/		
	  ii++; /* progress to next bit_plane*/
	}
    }

/* LOW ENERGY MODE CODING */

  for (s=0;s<total_sfb + num_osf_sfbs;s++)
    { 
      int lazy_plane = p_bpc_maxbitplane[s] - p_bpc_L[s];
      if ((p_bpc_maxbitplane[s] >= 0) && (lazy_plane <= 0))
	{
	  for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)
	    {
	      int sym = abs(p_error_spectrum[i]);
	      int sgn = (p_error_spectrum[i] >=0)?0:1;
	      int bin = 0;
	      int dumb = sym;

	      while (dumb>0)
		{
		  ac_encode_symbol(&coder,silence_freq[-lazy_plane][bin],1);
		  bin++;
		  if (bin>2) bin = 2;
		  dumb -- ;
		}
	      if (sym!=(1<<(p_bpc_maxbitplane[s]+1))-1) 
		ac_encode_symbol(&coder,silence_freq[-lazy_plane][bin],0);
	      if ((p_significant_error[i] == 0)&&sym)
		{
		  ac_encode_symbol(&coder,half_freq,sgn);
		  p_significant_error[i] = 1;
		}
	      if (ac_bits(&coder) >= (bits_to_use - 8)) /*??*/
		{
		  rate_constraint_reached = 1;
		  goto Rate_Reached;
		}
	    }
	}
    } 


Rate_Reached:
  ac_encoder_done(&coder); /*output rest bits in register */
  if (rate_constraint_reached) /*byte align for lossy LLE*/
    ll_Info->streamLen = (bits_to_use>>3)<<3;
  else
    ll_Info->streamLen = ac_bits (&coder); /* total bits in bpc stream */
}


int LIT_ll_errorMap(
		    int		*p_spectrum,  /* MDCT spectrum */
		    AACQuantInfo quantInfo[MAX_TF_LAYER],      /* AAC quantization information */ 
		    LIT_LL_Info *ll_Info,
                    MSInfo *msInfo,
		    short msenable,
		    int osf,
		    WINDOW_TYPE block_type_lossless_only,
                    int total_sfb,
                    int *sfb_offset,
		    int* short_sfb_width,
		    int short_nr_of_sfb,
                    int core_enable,
                    int num_aac_layers,
                    long samplRate
		    )				
{

  int quant;
  int i,j,s,lay;

  int *bank_type = ll_Info->bank_type;

  int Layer;

  double scaling;  	
  int	*p_error_spectrum = ll_Info->error_spectrum; /* mapped error spectrum */
  int *p_significant_error = ll_Info->significant_error;
  int	*p_bpc_L = ll_Info->bpc_L; /* lazy layer for BPC */
  int	*p_bpc_maxbitplane = ll_Info->bpc_maxbitplane; /* maximum bit plane for BPC */
  int *bpc_interval = ll_Info->bpc_interval;
  int *p_significant_band = ll_Info->significant_band;

  int group;
  int sfb_per_group;
  int osfw = 64; /* width of osf sfbs */

  int *core_significance_signaling = &ll_Info->core_significance_signaling;
  int *core_significance = ll_Info->core_significance;

  int mdct_scaling;
  int overall_scale,overall_scale_int,overall_scale_res,scale;


  *core_significance_signaling = 0;  /* assume everything good*/
  for (s=0;s<total_sfb;s++)
		  core_significance[s] = 1;

  if (avoidImplicitSignaling)
  {
	  *core_significance_signaling = 2;
	  for (s=0;s<total_sfb;s++)
		  core_significance[s] = 0;
  }

  num_osf_sfbs = (osf-1)*1024/osfw;

  if (!(block_type_lossless_only == EIGHT_SHORT_SEQUENCE/*ONLY_SHORT_WINDOW*/)) {
    for (i=1;i<=num_osf_sfbs;i++) {
      sfb_offset[total_sfb+i] = 1024+i*osfw;
    }
  } else {
    total_sfb = 4*short_nr_of_sfb;

    sfb_offset[0] = 0;
    sfb_per_group = short_nr_of_sfb+num_osf_sfbs/4;
    for (group=0; group<4; group++) {
      for (i=1;i<=short_nr_of_sfb; i++) {
	sfb_offset[i+group*sfb_per_group] = sfb_offset[i-1+group*sfb_per_group]+2*short_sfb_width[i-1]; 
      }
      for (i=short_nr_of_sfb+1;i<=sfb_per_group; i++) {
	sfb_offset[i+group*sfb_per_group] = sfb_offset[i-1+group*sfb_per_group]+osfw;
      }
    }

  }

  scaling = ll_Info->CoreScaling;


  {
    INT64 A,N;
		
    for (i=0;i<osf*1024;i++) {
      bpc_interval[i] = 0x7fffffff;
    }

    for (s = 0; s < total_sfb + num_osf_sfbs; s++)
      {  

	bank_type[s] = ForcedExplicitBand;

			N=sfb_offset[s+1]-sfb_offset[s];
			A=0;

			if (quantInfo->block_type != EIGHT_SHORT_SEQUENCE/*ONLY_SHORT_WINDOW*/)
			{
                          /* MSInfo *msInfo = &channelInfo->ms_info; */
                          if ((msInfo->ms_mask)&&((msInfo->ms_mask == 2) || msInfo->ms_used[s]))  
                            mdct_scaling = MDCT_SCALE_MS;
                          else
                            mdct_scaling = MDCT_SCALE;
			}
			else
			{

				if ((msInfo->ms_mask)&&((msInfo->ms_mask == 2) || msInfo->ms_used[s]))
					mdct_scaling = MDCT_SCALE_SHORT_MS;
				else
					mdct_scaling = MDCT_SCALE_SHORT;

			}
                        /* overall_scale = scale_factor[s] - SF_OFFSET - mdct_scaling + scale_pcm[ll_Info->type_PCM]*4; */
/*
			overall_scale = 0;
			for (lay = 0; lay < num_aac_layers; lay++)
			{
				overall_scale += quantInfo[lay].scale_factor[s] - SF_OFFSET - mdct_scaling + scale_pcm[ll_Info->type_PCM]*4;
			}

			overall_scale_int = (int)floor((double)overall_scale/4);
			overall_scale_res = overall_scale - overall_scale_int*4;
*/
/*
void get_overall_scale(int *overall_scale[s], int total_layer)
{
	for (total_layer)
	{
	}
}
*/
			


			switch (bank_type[s])
			{
			case ForcedExplicitBand:
					{
					int dummy= 0;
                                        /* quantFac = pow(2.0, 0.1875*(SF_OFFSET - scale_factor[s] )); */
					p_significant_band[s] = 0;

					for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++)
					{
                                          /* int thr1,thr; */
					  int invQuant_sum = 0;	
					  
					  for (lay = 0; lay < num_aac_layers; lay++)
					  {
						if (quantInfo[lay].ll_present){
							overall_scale = quantInfo[lay].scale_factor[s] - SF_OFFSET - mdct_scaling + scale_pcm[ll_Info->type_PCM]*4;
							overall_scale_int = (int)floor((double)overall_scale/4);
							overall_scale_res = overall_scale - overall_scale_int*4;

							quant = abs(quantInfo[lay].quant_aac[i]);
						
							p_significant_error[i] = 0;

							invQuant_sum += ((quantInfo[lay].quant_aac[i]>0)?1:-1)*invQuantFunction(quant, overall_scale);	


						}

					  } /* for (num_aac_layers) loop */

					  {
						p_error_spectrum[i] = p_spectrum[i] - invQuant_sum;
					  }

					  A += abs(p_error_spectrum[i]);
					}

				   for (i=sfb_offset[s]; i<sfb_offset[s+1]; i++) {
					  if (dummy<(abs(p_error_spectrum[i]))) 
					  dummy = abs(p_error_spectrum[i]);
					  }

				for (i=0;(1<<(i+1))<=(dummy);i++);
				 p_bpc_maxbitplane[s] = i;  /* should be sent as side info in insig bank*/
					
			}
			break;

			case ErrBank:
			default:
				return -1; /* error */
				break;
			}/* switch */	
		       

		   if (N<A) 
			for (Layer=0;(N<<(Layer+1))<=A;Layer++);
		   else
			for (Layer=-1;(N>>(-Layer))>A;Layer--);

		   p_bpc_L[s] = p_bpc_maxbitplane[s] - Layer;


			if (p_bpc_L[s] < 1) p_bpc_L[s] = 1;
			else if (p_bpc_L[s] > 3) p_bpc_L[s] = 3;
		} /* for (s...*/
	}
	return 0;
}

#endif
