/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by
Olaf Kaehler (Fraunhofer IIS-A)

and edited by
Stefan Bayer/Guillaume Fuchs (Fraunhofer IIS)
Jeremie Lecomte (Fraunhofer IIS)


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

Copyright (c) 2003.

**********************************************************************/

#include <float.h>

#include "common_m4a.h"
#include "bitstream.h"
#include "allHandles.h"

#include "aac_tools.h"
#include "tf_mainStruct.h"       /* structs */
#include "tf_main.h"
#include "tns3.h"
#include "ms.h"

#include "usac_bitmux.h"
#include "usac_fd_qc.h"
#include "usac_fd_enc.h"
#include "proto_func.h"

#include "cplx_pred.h"

static void fill_toolsdata(UsacToolsInfo *info,
                           TNS_INFO *tnsInfo)
{
  if (info == NULL) return;
   info->noiseFilling = 0;
  info->noiseOffset = 0;
  info->noiseLevel=0;
  info->tw_data_present = 0;
}

static void usac_fill_ics_info(UsacICSinfo  *info,
                   int 						max_sfb,
                   WINDOW_SEQUENCE 			windowSequence,
                   WINDOW_SHAPE 			window_shape,
                   int 						num_window_groups,
                   int 						window_group_length[],
                   int 						stereo_flag,
                   int 						common_window)
{
  int i;
  if (info == NULL) return;
  info->max_sfb = max_sfb;
  info->windowSequence = windowSequence;
  info->window_shape = window_shape;
  info->num_window_groups = num_window_groups;
  for (i=0; i<num_window_groups; i++)
    info->window_group_length[i] = window_group_length[i];
  info->stereo_flag = stereo_flag;
  info->common_window = common_window;
}



static int calc_fd_static_bits(int 				common_window,
                               int 				common_tw,
                               int 				flag_twMdct,
                               int 				flag_noiseFilling,
                               WINDOW_SEQUENCE  windowSequence,
                               int             	max_sfb,
                               int             	num_window_groups,
                               int             	window_group_length[],
                               UsacToolsInfo  	*tools_info,
                               TNS_INFO       	*tns_info) {

  int bits_count = 0;

  bits_count += 8; /* global gain */

  if ( flag_noiseFilling ) {
    bits_count += 3;
    bits_count += 5;
  }

  if ( !common_window ) {
    bits_count += usac_write_ics_info(NULL,
                                      max_sfb,
                                      windowSequence,
                                      WS_FHG,
                                      num_window_groups,
                                      window_group_length
                                      );
  }

  if ( flag_twMdct ) {
    if ( !common_tw ) {
      bits_count += usac_write_tw_data(NULL,
                                       tools_info->tw_data_present,
                                       tools_info->tw_ratio);
    }
  }

  if ( tns_info != NULL ) {
    bits_count += write_tns_data(NULL,
                                 1,
                                 tns_info,
                                 windowSequence,
                                 NULL);
  }
  else {
    
  }

  return (bits_count);

}


/* Bandwidth limiting: Do not code beyond cutoff bandwidth */
void usac_bandwidth_limit_spectrum(double *orig_spectrum,
				   double 				  *out_spectrum,
				   WINDOW_SEQUENCE        windowSequence,
				   int    				  block_size_samples,
				   int    				  sampling_rate,
				   int  				  bw_limit,
				   int 					  sfb_offset[MAX_SCFAC_BANDS+1],
				   int  				  nr_of_sfb,
				   int  				  *max_sfb)
{
  int i, k, sfb;
  int no_subblocks;
  int lines_per_subblock;
  int no_lines;
  int highestSfb;

  no_subblocks = (windowSequence == EIGHT_SHORT_SEQUENCE) ? NSHORT : 1;
  lines_per_subblock = block_size_samples/no_subblocks;
  no_lines  = (int)(lines_per_subblock * bw_limit * 2.0 / sampling_rate);
  if (no_lines > lines_per_subblock) no_lines = lines_per_subblock;

  for (k=0; k<no_subblocks; k++) {
    for (i=0; i<no_lines; i++ ) {
      out_spectrum[k*lines_per_subblock+i] = orig_spectrum[k*lines_per_subblock+i];
    }
    for (i=no_lines; i<lines_per_subblock; i++ ) {
      out_spectrum[k*lines_per_subblock+i] = 0.0;
    }
  }

  /*Calculate max sfb*/
  highestSfb = 0;
  for (k = 0; k < no_subblocks; k++)
    {
      for (sfb = nr_of_sfb-1; sfb >= highestSfb; sfb--)
      {
        for (i = sfb_offset[sfb+1]-1; i >= sfb_offset[sfb]; i--)
        {
          if (out_spectrum[k*lines_per_subblock+i]) break;    /* this band is not completely zero */
        }
        if (i >= sfb_offset[sfb]) break;              /* this band was not completely zero */
      }
      highestSfb = highestSfb>sfb ? highestSfb : sfb;
    }
  highestSfb = highestSfb > 0 ? highestSfb : 0;
  *max_sfb = highestSfb+1;
}

#define MAX_INT_DEC_LEN 4096
int usac_fd_encode(
		   double      *DTimeSigBuf[MAX_TIME_CHANNELS],
		   double      *spectral_line_vector[MAX_TIME_CHANNELS],
		   double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
		   HANDLE_USAC_TD_ENCODER tdenc[MAX_TIME_CHANNELS],
		   HANDLE_TFDEC hTfDec,
		   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
		   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
		   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
		   int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
		   int         max_sfb[MAX_TIME_CHANNELS],
		   int         nr_of_sfb[MAX_TIME_CHANNELS],
		   int         bits_available,
		   int         bits_needed,
		   HANDLE_AACBITMUX bitmux,
		   int         ep_syntax,
		   int         ld_mode,
		   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
		   WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
		   WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS],
		   int         aacAllowScalefacs,
		   int         blockSizeSamples[MAX_TIME_CHANNELS],
		   int         nr_of_chan,
		   long        samplRate,
		   MSInfo      *msInfo,
           int         predCoef[MAX_SHORT_WINDOWS][SFB_NUM_MAX],
		   TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
		   UsacICSinfo ics_info[MAX_TIME_CHANNELS],
		   UsacToolsInfo tool_info[MAX_TIME_CHANNELS],
		   UsacQuantInfo qInfo[MAX_TIME_CHANNELS],
		   int         num_window_groups[MAX_TIME_CHANNELS],
		   int         window_group_length[MAX_TIME_CHANNELS][8],
		   int         bw_limit,
		   int         flag_noiseFilling,
		   int         flag_twMdct,
		   int         commonWindow,
		   short       serialFac[MAX_TIME_CHANNELS][NBITS_MAX],
		   int        *nBitsFac,
		   USAC_CORE_MODE coreMode[MAX_TIME_CHANNELS],
		   USAC_CORE_MODE   prev_coreMode[MAX_TIME_CHANNELS],
		   USAC_CORE_MODE   next_coreMode[MAX_TIME_CHANNELS],
                   int         bUsacIndependencyFlag,
		   int         debugLevel){

  int i_ch;
  int used_bits = 0;
  int bits_written = 0;
  int common_tw;
  int grouped_sfb_offsets[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];
  double reconstructed_time_signal[MAX_TIME_CHANNELS][4096];

  /* ---- dummy writing to calculate number of bits used ---- */

  common_tw = tool_info[0].common_tw;

  for (i_ch=0; i_ch<nr_of_chan; i_ch++) {
    if (debugLevel>3)
      fprintf(stderr,"EncTfFrame: prepare ics_info\n");

    usac_fill_ics_info(&(ics_info[i_ch]),
                       max_sfb[i_ch],  /* should be maxSfb */
                       windowSequence[i_ch],
                       windowShape[i_ch],
                       num_window_groups[i_ch],
                       window_group_length[i_ch],
                       (nr_of_chan>1),
                       commonWindow);


  }

  /* ---- process and quantize the spectrum ---- */

  if (nr_of_chan==2&&
      coreMode[0] == CORE_MODE_FD && coreMode[1] == CORE_MODE_FD &&
      commonWindow == 1) {
    if (debugLevel>1)
      fprintf(stderr,"EncUsacFrame: MS\n");

    MSApply(nr_of_sfb[MONO_CHAN],
            sfb_offset[MONO_CHAN],
            msInfo->ms_used,
            (windowSequence[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)?NSHORT:1,
            blockSizeSamples[MONO_CHAN],
            spectral_line_vector[0],
            spectral_line_vector[1]);

    if (msInfo->ms_mask == 3) {
      ComplexPrediction(nr_of_sfb[MONO_CHAN],
                        sfb_offset[MONO_CHAN],
                        num_window_groups[MONO_CHAN],
                        spectral_line_vector[0],
                        spectral_line_vector[1],
                        predCoef);
    }
  }

  for (i_ch=0; i_ch<nr_of_chan; i_ch++) {

    if ( coreMode[i_ch] == CORE_MODE_FD ) {
      if (debugLevel>3)
        fprintf(stderr,"EncUsacFrame: quantize\n");

      used_bits += usac_quantize_spectrum(&qInfo[i_ch],
                                          spectral_line_vector[i_ch],
                                          reconstructed_spectrum[i_ch],
                                          energy[i_ch],
                                          allowed_dist[i_ch],
                                          windowSequence[i_ch],
                                          windowShape[i_ch],
                                          sfb_width_table[i_ch],
                                          grouped_sfb_offsets[i_ch],
                                          max_sfb[i_ch],
                                          nr_of_sfb[i_ch],
                                          (bits_available-used_bits)/(nr_of_chan-i_ch),
                                          blockSizeSamples[i_ch],
                                          num_window_groups[i_ch],
                                          window_group_length[i_ch],
                                          aacAllowScalefacs,
                                          &tool_info[i_ch],
                                          tnsInfo[i_ch],
                                          commonWindow,
                                          common_tw,
                                          flag_twMdct,
                                          flag_noiseFilling, 
                                          bUsacIndependencyFlag
                                          );
    }
  }


  /* FD-FAC */
  AdvanceIntDecUSAC(hTfDec,
                    reconstructed_spectrum,
                    windowSequence,
                    windowShape,
                    windowShapePrev,
                    nr_of_sfb,
                    max_sfb,
                    blockSizeSamples,
                    sfb_offset,
                    msInfo,
                    tnsInfo,
                    nr_of_chan,
                    commonWindow,
                    coreMode,
                    prev_coreMode,
                    next_coreMode,
                    reconstructed_time_signal);



  for (i_ch=0;i_ch < nr_of_chan;i_ch++) {

    if ( coreMode[i_ch] == CORE_MODE_FD ) {
      used_bits += FDFac(grouped_sfb_offsets[i_ch],
                         max_sfb[i_ch],
                         DTimeSigBuf[i_ch],
                         windowSequence[i_ch],
                         reconstructed_time_signal[i_ch],
                         tdenc[i_ch],
                         (acelpLastSubFrameWasAcelp(tdenc[i_ch]) && prev_coreMode[i_ch] == CORE_MODE_TD),
                         next_coreMode[i_ch] == CORE_MODE_TD,
                         serialFac[i_ch],
                         &nBitsFac[i_ch]);
    }
  }

  return used_bits;
}

