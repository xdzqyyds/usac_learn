#ifndef EN_LOSSLESS_H
#define EN_LOSSLESS_H

#include "aac_qc.h"
#include "mc_enc.h"
#include "int_mdct_defs.h"

#include "lit_ll_common.h"


#define MDCT_SCALING 45.254834
#define MDCT_SCALING_MS 32 /* compensate MS gain in MPEG decoder*/
#define MDCT_SCALING_SHORT 16
#define MDCT_SCALING_SHORT_MS 11.3137085


#ifdef I2R_LOSSLESS
typedef struct LIT_LL_Info_t
{
  int type_PCM; /* PCM types, 0/1/2/3 = 8/16/20/24 bits per sample*/

  int bpc_L[MAX_OSF*MAXBANDS];
  int bpc_maxbitplane[MAX_OSF*MAXBANDS];
  int bank_type[MAX_OSF*MAXBANDS];
  double CoreScaling;
  
  int core_significance_signaling;
  int core_significance[MAXBANDS];
  
  
  int bpc_sig_maxbitplane[MAX_OSF*BLOCK_LEN_LONG];
  int bpc_interval[MAX_OSF*BLOCK_LEN_LONG];
  int significant_error[MAX_OSF*BLOCK_LEN_LONG];
  int error_spectrum[MAX_OSF*BLOCK_LEN_LONG];
  int significant_band[MAXBANDS];
  
  unsigned char *stream;
  long streamLen;
  
  /*added in enhancer*/
  /* byte max_sfb; *//*[MAX_TIME_CHANNELS];*/
  short bk_sfb_top[200];
  int num_window_groups;
  int num_sfb;
  int num_osf_sfb;
  int total_sfb_per_group;
  int osf_sfb_width;
  int sfb_offset_lossless_only_short[MAX_SCFAC_BANDS];  
  int aac_sfb_offset[MAX_SCFAC_BANDS]; /* sfb_offset for AAC core layer */
  int sls_sfb_offset[MAX_SCFAC_BANDS]; 
  int aac_sfb_width[MAX_SCFAC_BANDS]; 
  int sls_sfb_width[MAX_SCFAC_BANDS]; 

} LIT_LL_Info;


int lit_ll_sfg(AACQuantInfo* quantInfo,        /* ptr to quantization information */
               int sfb_width_table[],          /* Widths of single window */
               int *p_spectrum,           /* Spectral values, noninterleaved */
	       int osf);

void LIT_ll_perceptualEnhEnc(LIT_LL_Info * ll_Info,
			     AACQuantInfo quantInfo[MAX_TF_LAYER] ,     /* AAC quantization information */ 
			     long bit_to_use,
			     int sampling_rate,
			     int osf,
                             int total_sfb,
                             int *sfb_offset,
			     int short_nr_of_sfb,
			     int coreenable,
			     Ch_Info *channelInfo,
			     WINDOW_TYPE block_type_lossless_only,
			     int msStereoIntMDCT,
                             int num_aac_layers
			     );


int LIT_ll_errorMap(\
		    int		*p_spectrum,  /* MDCT spectrum */\
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
		    );	


#endif /* #ifdef I2R_LOSSLESS */

#endif 






