/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by
Olaf Kaehler (Fraunhofer IIS-A)

and edited by


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
#include "aac_qc.h"
#include "bitmux.h"
#include "tns3.h"
#include "ms.h"

static void fill_nonscalable_extradata(
  ToolsInfo *info,
  GC_DATA_SWITCH gc_switch,
  int max_band,
  GAINC **gainInfo,
  TNS_INFO *tnsInfo)
{
  if (info == NULL) return;
  info->pulse_data_present = 0;

  info->gc_data_present = (gc_switch==GC_PRESENT);
  info->max_band = max_band;
  info->gainInfo = gainInfo;

  info->tns_data_present = (tnsInfo!=NULL);
  info->tnsInfo = tnsInfo;
}

static void fill_ics_info(ICSinfo *info, 
                   int max_sfb,
                   WINDOW_SEQUENCE windowSequence, WINDOW_SHAPE window_shape,
                   int num_window_groups, int window_group_length[],
                   int ld_mode,
                   PRED_TYPE predictor_type,
                   NOK_LT_PRED_STATUS *nok_lt_statusLeft,
                   NOK_LT_PRED_STATUS *nok_lt_statusRight,
                   int stereo_flag)
{
  int i;
  if (info == NULL) return;
  info->max_sfb = max_sfb;
  info->windowSequence = windowSequence;
  info->window_shape = window_shape;
  info->windowSequence = windowSequence;
  info->num_window_groups = num_window_groups;
  for (i=0; i<num_window_groups; i++)
    info->window_group_length[i] = window_group_length[i];
  info->ld_mode = ld_mode;
  info->predictor_type = predictor_type;
  info->nok_lt_status[0] = nok_lt_statusLeft;
  info->nok_lt_status[1] = nok_lt_statusRight;
  info->stereo_flag = stereo_flag;
}



/* Bandwidth limiting: Do not code beyond cutoff bandwidth */
void bandwidth_limit_spectrum(const double *orig_spectrum,
                              double *out_spectrum,
                              WINDOW_SEQUENCE windowSequence,
                              int    block_size_samples,
                              int    sampling_rate,
                              int    bw_limit)
{
  int i, k;
  int no_subblocks;
  int lines_per_subblock;
  int no_lines;

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
}


void tf_apply_pns(
  double      *spectrum,
  double      energy[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],
  int         nr_of_sfb,
  int         pns_sfb_start,
  WINDOW_SEQUENCE windowSequence,
  PnsInfo     *pnsInfo)
{
  int sfb,i;

  for (sfb=0; sfb<SFB_NUM_MAX; sfb++) {
    pnsInfo->pns_sfb_flag[sfb] = 0;
    pnsInfo->pns_sfb_nrg[sfb] = 0;
  }

  /** Prepare Perceptual Noise Substitution **/
  if ((pns_sfb_start>=0)&&
      (windowSequence != EIGHT_SHORT_SEQUENCE)) {

    /* long blocks only */
    for(sfb=pns_sfb_start; sfb<nr_of_sfb; sfb++ ) {
      /* Calc. pseudo scalefactor */
      if (energy[sfb] < FLT_MIN) {
        pnsInfo->pns_sfb_flag[sfb] = 0;
        continue;
      }

      pnsInfo->pns_sfb_flag[sfb] = 1;
      pnsInfo->pns_sfb_nrg[sfb] = (int) (2.0 * log(energy[sfb]+1e-60) / log(2.0) + 0.5) + PNS_SF_OFFSET;

      /* Erase spectral lines */
      for( i=sfb_offset[sfb]; i<sfb_offset[sfb+1]; i++ ) {
        spectrum[i] = 0.0;
      }
    }
  }
}



int EncTf_aacplain_encode(
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
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
  int         blockSizeSamples,
  int         nr_of_chan,
  long        samplRate,
  MSInfo      *msInfo,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  int         pns_sfb_start,
  GC_DATA_SWITCH gc_switch,
  int         max_band[MAX_TIME_CHANNELS],
  GAINC       **gainInfo[MAX_TIME_CHANNELS],
  int         num_window_groups,
  int         window_group_length[8],
  int         bw_limit,
  int         debugLevel
)
{
  int i_ch;
  int used_bits = 0;
  int bits_written = 0;
  int commonWindow;
  ICSinfo ics_info[MAX_TIME_CHANNELS];
  ToolsInfo tool_info[MAX_TIME_CHANNELS];
  PnsInfo pnsInfo[MAX_TIME_CHANNELS];
  QuantInfo qInfo[MAX_TIME_CHANNELS];
  int grouped_sfb_offsets[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];


  /* ---- dummy writing to calculate number of bits used ---- */

  for (i_ch=0; i_ch<nr_of_chan; i_ch++) {
    if (i_ch==0) { /* common_window is always 1 currently */
      if (debugLevel>3)
        fprintf(stderr,"EncTfFrame: prepare ics_info\n");

      fill_ics_info(&(ics_info[i_ch]),
                    nr_of_sfb[i_ch],
                    windowSequence[i_ch],
                    windowShape[i_ch],
                    num_window_groups,
                    window_group_length,
                    ld_mode,
                    pred_type,
                    &(nok_lt_status[i_ch]),
                    &(nok_lt_status[i_ch+1]),
                    (nr_of_chan>1)); /* stereo flag */
    }
    if (debugLevel>3)
      fprintf(stderr,"EncTfFrame: prepare pulse_data, tns_data and gc_info\n");

    fill_nonscalable_extradata(&(tool_info[i_ch]),
                               gc_switch,
                               max_band[i_ch],
                               gainInfo[i_ch],
                               tnsInfo[i_ch]);
    used_bits += write_ics_nonscalable_extra(NULL,
                                             windowSequence[i_ch],
                                             &(tool_info[i_ch]));
  }

  switch (nr_of_chan) {
  case 1:
    commonWindow = 0;
    used_bits += write_aac_sce(NULL, 0/*tag*/, !ep_syntax);
    
    used_bits += write_ics_info(NULL, &(ics_info[0]));
    break;
  case 2:
    commonWindow = 1;
    used_bits += write_aac_cpe(NULL, 0/*tag*/, !ep_syntax,
                               commonWindow,
                               msInfo->ms_mask, msInfo->ms_used,
                               num_window_groups, nr_of_sfb[0],
                               ics_info);
    break;
  default:
    CommonWarning("Only mono or stereo supported at the moment");
    return -1;
  }
  if (!ep_syntax) used_bits += write_aac_end_id(NULL);

  
  /* ---- process and quantize the spectrum ---- */

  if (nr_of_chan==2) {
    if (debugLevel>1)
      fprintf(stderr,"EncTfFrame: MS\n");

    MSApply(nr_of_sfb[MONO_CHAN],
            sfb_offset[MONO_CHAN],
            msInfo->ms_used,
            (windowSequence[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)?NSHORT:1,
            blockSizeSamples,
            spectral_line_vector[0],
            spectral_line_vector[1]);
  }

  for (i_ch=0; i_ch<nr_of_chan; i_ch++) {
    double temp_spectrum[NUM_COEFF];

    if (debugLevel>3)
      fprintf(stderr,"EncTfFrame: apply bandwidth limitation\n");
    if (debugLevel>5)
      fprintf(stderr,"            bandwidth limit %i Hz\n", bw_limit);


    bandwidth_limit_spectrum(spectral_line_vector[i_ch],
                             temp_spectrum,
                             windowSequence[i_ch],
                             blockSizeSamples,
                             samplRate, 
                             bw_limit);

    if ((debugLevel>3)&&(pns_sfb_start>=0))
      fprintf(stderr,"EncTfFrame: apply PNS (from sfb %i)\n", pns_sfb_start);

    tf_apply_pns(temp_spectrum,
                 energy[i_ch],
                 sfb_offset[i_ch],
                 nr_of_sfb[i_ch],
                 pns_sfb_start,
                 windowSequence[i_ch],
                 &pnsInfo[i_ch]);

    if (debugLevel>3)
      fprintf(stderr,"EncTfFrame: quantize\n");

    used_bits += tf_quantize_spectrum(&qInfo[i_ch],
			temp_spectrum,
			reconstructed_spectrum[i_ch],
			energy[i_ch],
			allowed_dist[i_ch],
			windowSequence[i_ch],
			sfb_width_table[i_ch],
			grouped_sfb_offsets[i_ch],
			nr_of_sfb[i_ch],
			&pnsInfo[i_ch],
			(bits_available-used_bits)/(nr_of_chan-i_ch),
			blockSizeSamples,
			num_window_groups,
			window_group_length,
			aacAllowScalefacs);
  }



  /* ---- now write out the results ---- */

  aacBitMux_setAssignmentScheme(bitmux, nr_of_chan, commonWindow);
  aacBitMux_setCurrentChannel(bitmux, 0);
  

  switch (nr_of_chan) {
  case 1:
    commonWindow = 0;
    bits_written += write_aac_sce(bitmux, 0/*tag*/, !ep_syntax);
    break;
  case 2:
    commonWindow = 1;
    bits_written += write_aac_cpe(bitmux, 0/*tag*/, !ep_syntax,
                                  commonWindow,
                                  msInfo->ms_mask, msInfo->ms_used,
                                  num_window_groups, nr_of_sfb[0],
                                  ics_info);
    break;
  default:
    CommonWarning("Only mono or stereo supported at the moment");
    return -1;
  }

  for (i_ch=0; i_ch<nr_of_chan; i_ch++) {
    aacBitMux_setCurrentChannel(bitmux, i_ch);
    bits_written += bitpack_ind_channel_stream(bitmux,
                        &(ics_info[i_ch]),
			&(tool_info[i_ch]),
			&qInfo[i_ch],
			commonWindow,
			windowSequence[i_ch],
			num_window_groups,
			grouped_sfb_offsets[i_ch],
			nr_of_sfb[i_ch],
			&pnsInfo[i_ch],
			0/*scaleFlag*/,debugLevel);
  }

  /* fill up */
  if (!ep_syntax) {
    int fill_needed;

    bits_written += 3; /* length of ID_END element */
    fill_needed = bits_needed-bits_written;
    if (fill_needed > 0) {
      fill_needed = fill_needed+7; /* fill elements come in bytes => round up */
      bits_written += write_fill_elements(bitmux, fill_needed);
    }

    write_aac_end_id(bitmux);
  } else {
    int padding_bits = bits_needed - bits_written;
    if (padding_bits>0) {
      bits_written += write_padding_bits(bitmux, padding_bits);
    }
  }


  /* ---- finally update internal buffers ---- */

  if (debugLevel>3)
    fprintf(stderr,"EncTfFrame: prepare prediction for next frame\n");

  /* Nokia LTP buffer updates */
  if (pred_type == NOK_LTP) {
    for(i_ch = 0; i_ch < nr_of_chan; i_ch++) {  
      /* Add the LTP contribution to the reconstructed spectrum. */
      nok_ltp_reconstruct(reconstructed_spectrum[i_ch], 
			  windowSequence[i_ch], 
			  &sfb_offset[i_ch][0], nr_of_sfb[i_ch],
			  blockSizeSamples/NSHORT,
			  &nok_lt_status[i_ch]);

      /* TNS synthesis filtering. */
      if (tnsInfo[i_ch]!=NULL)
        TnsEncode2(nr_of_sfb[i_ch], nr_of_sfb[i_ch], windowSequence[i_ch], 
		   &sfb_offset[i_ch][0], reconstructed_spectrum[i_ch], 
		   tnsInfo[i_ch], 1);

      /* Update the time buffer of LTP. */
      nok_ltp_update(reconstructed_spectrum[i_ch],
		     nok_lt_status[i_ch].overlap_buffer,
		     windowSequence[i_ch],
		     windowShape[i_ch], windowShapePrev[i_ch], 
		     blockSizeSamples, 
		     blockSizeSamples/NSHORT,
		     NSHORT,
		     &nok_lt_status[i_ch]);
    }
  }
  return bits_written;
}

