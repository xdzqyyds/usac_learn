/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bernhard Grill (University of Erlangen)

and edited by

Olaf Kaehler (Fraunhofer-IIS)

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

Copyright (c) 1997.


 23-oct-97   HP   merged Nokia's predictor 971013 & 971020
                  tried to fix PRED_TYPE
 24-oct-97   Mikko Suonio   fixed PRED_TYPE 
 29-oct-03   OK   cleanups for encoder rewrite
*/


#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */
#include "ms.h"

#include "tf_main.h"
#include "bitstream.h"
#include "common_m4a.h"
#include "util.h"

#ifdef I2R_LOSSLESS
#include "lit_ll_en.h"
#include "mc_enc.h"
#include "psych.h"
#include "lit_ms.h"
#include "ll_bitmux.h"/*SAMSUNG updated 20061213*/
#endif

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "aac_qc.h"
#include "scal_enc_frame.h"
#include "bitmux.h"


#undef DEBUG_TNS_CORE


/********************************************************************************/

/********************************************************************************/


/********************************************************************************/

/********************************************************************************/

static int getNumDiffBands(int samplRate)
{
/* see ISO/IEC 14496-3 (2001) Table 4.112 */
  int numDiffBands = 0;
  switch( samplRate ) {
  case 96000 :
  case 88200 :
    numDiffBands = 16/*4*/; break;
  case 64000 :
    numDiffBands = 20/*5*/; break;
  case 48000 :
  case 44100 : 
    numDiffBands = 20/*5*/; break;
  case 32000 : 
    numDiffBands = 24/*6*/; break;
  case 22050 :
  case 24000 :
    numDiffBands = 32/*8*/; break;
  case 16000 :
    numDiffBands = 32/*8*/; break;
  case 12000 :
  case 11025 :
    numDiffBands = 40/*10*/; break;
  case  8000 :
  case  7350 :
    numDiffBands = 36/*9*/; break;
  default :
    CommonExit( -1, "scalable encoder doesn't support this sampling rate currently" );
    break;
  }
  return numDiffBands;
}

/* FSS: switch decision */  

static void CalcFssControl(
  double       p_full[],
  double       p_diff[],
  WINDOW_SEQUENCE  blockType,
  enum DC_FLAG FssControl[],
  int          blockSizeSamples,
  int          sfb_width_table[],
  long         samplRate,
  enum DC_FLAG override
)
{
  int win, no_win, i, offset, sb, no_spec_coeff, *diffBandWidth;
  int numDiffBands = 0;
  int FssBwShort;
  double e_full[SFB_NUM_MAX];
  double e_diff[SFB_NUM_MAX];
#ifdef DEL_ADJ
  static double avg_gain[SFB_NUM_MAX];
  static int _cnt;
#endif

  if( blockType == EIGHT_SHORT_SEQUENCE ) {
    /* for now just always the difference signal */
    no_win        = 8;
    no_spec_coeff = blockSizeSamples/no_win;
    numDiffBands  = 1;
    diffBandWidth = &FssBwShort;


    switch( samplRate ) {
    case 48000:
    case 44100: 
      FssBwShort = 18; break;
    case 32000: 
      FssBwShort = 28; break;
    case 22050:
    case 24000:
      FssBwShort = 36; break;
    case 16000:
      FssBwShort = 54/*56*/; break;
    }

    for( i=0; i<8; i++ ) {  /* always diff for short for now */
      if (override==DC_SIMUL) {
        FssControl[i] = DC_SIMUL;
      } else {
        FssControl[i] = DC_DIFF;
      }
    }
  } else {
    no_win        = 1;
    no_spec_coeff = blockSizeSamples;
    diffBandWidth = sfb_width_table;

    numDiffBands = getNumDiffBands(samplRate);

    for( sb=0; sb<numDiffBands; sb++ ) {
      if (override==DC_SIMUL) {
        FssControl[sb] = DC_SIMUL;
      } else {
        FssControl[sb] = DC_DIFF;
      }
    }

    /* detailed check, whether difference is better than simulcast */
    if (override==DC_INVALID) {
      offset = 0;
      for( sb=0; sb<numDiffBands; sb++ ) {
        e_full[sb] = 0;
        e_diff[sb] = 0;

        for( i=0; i<sfb_width_table[sb]; i++ ) {
          e_full[sb] +=  p_full[offset] * p_full[offset];
          e_diff[sb] +=  p_diff[offset] * p_diff[offset];
          offset++;
        }
        if( e_diff[sb] >= e_full[sb] ) {
          FssControl[sb] = DC_SIMUL;
        }
#ifdef DEL_ADJ
        if( log10(e_full[sb] + FLT_MIN) > 0 ) {
          avg_gain[sb] += log10(e_full[sb] + FLT_MIN) - log10(e_diff[sb] + FLT_MIN);
        }
#endif
      }
    
#ifdef DEL_ADJ
      printf( "\nE_full :  " );
      for( sb=0; sb<numDiffBands; sb++ ) {
        printf( "%8.3f", 10*log10(e_full[sb] + FLT_MIN) );
      }
      printf( "\nE_diff :  " );
      for( sb=0; sb<numDiffBands; sb++ ) {
        printf( "%8.3f", 10*log10(e_diff[sb] + FLT_MIN) );
      }
      printf( "\nA_gain :  " );
      for( sb=0; sb<numDiffBands; sb++ ) {
        printf( "%8.3f", 10*avg_gain[sb]/_cnt );
      }
      _cnt++;
      printf( "\n" );
#endif
    }
  }
 
  for( win=0; win<no_win; win++ ) {
    int offset   = win*no_spec_coeff;
    int dcf_offs = win;
    int sum      = 0;
    for( sb=0; sb<numDiffBands; sb++ ) {
      if( FssControl[dcf_offs+sb] == DC_SIMUL ) {
        for( i=0; i<diffBandWidth[sb]; i++ ) {
          p_diff[offset+i] = p_full[offset+i];
        }
      }
      offset += diffBandWidth[sb];
      sum    += diffBandWidth[sb];
    }
    for( i=0; i<(no_spec_coeff-sum); i++ ) {
      p_diff[offset+i] = p_full[offset+i];
    }
  }
}

/********************************************************************************/

/********************************************************************************/

static int maxSfbFromBandwidth(int bandwidth,
  int sampling_rate,
  int blockSizeSamples,
  int *sfb_offset,
  WINDOW_SEQUENCE windowSequence)
{
  int no_subblocks = (windowSequence == EIGHT_SHORT_SEQUENCE) ? NSHORT : 1;
  int lines_per_subblock = blockSizeSamples/no_subblocks;
  int bw_lines = (int)(lines_per_subblock * bandwidth * 2.0 / sampling_rate);
  int i;
  if (bw_lines > lines_per_subblock) bw_lines = lines_per_subblock;

  for (i=0; sfb_offset[i]<bw_lines; i++);

  return i;
}

static void ltp_scal_reconstruct(WINDOW_SEQUENCE blockType,
                                 WINDOW_SHAPE windowShape,
                                 WINDOW_SHAPE windowShapePrev,
                                 int num_channels,
                                 double *p_reconstructed_spectrum_left, 
                                 double *p_reconstructed_spectrum_right,
                                 int blockSizeSamples,
                                 int short_win_in_long,
                                 int *sfb_offset,
                                 int nr_of_sfb,
                                 NOK_LT_PRED_STATUS *nok_lt_status,
                                 TNS_INFO **tnsInfo)
{
  int i, ch;
  double tmpbuffer[BLOCK_LEN_LONG];
  double *rec_spectrum[2];

  rec_spectrum[0] = p_reconstructed_spectrum_left;
  rec_spectrum[1] = p_reconstructed_spectrum_right;

  for(ch = 0; ch < num_channels; ch++) {
    /* Add the LTP contribution to the reconstructed spectrum. */
    nok_ltp_reconstruct(rec_spectrum[ch], blockType, sfb_offset, nr_of_sfb, 
			blockSizeSamples/short_win_in_long, &nok_lt_status[ch]);
	
    for(i = 0; i < blockSizeSamples; i++)
      tmpbuffer[i] = rec_spectrum[ch][i];
	
    /* TNS synthesis filtering. */
    if(tnsInfo[ch] != NULL)
      TnsEncode2(nr_of_sfb, nr_of_sfb, blockType, sfb_offset,
		 &(tmpbuffer[0]), tnsInfo[ch], 1);
	
    /* Update the time domain buffers of LTP. */
    nok_ltp_update(&(tmpbuffer[0]), nok_lt_status[ch].overlap_buffer,
		   blockType, windowShape, windowShapePrev, 
		   blockSizeSamples, 
		   blockSizeSamples/short_win_in_long, 
		   short_win_in_long,
		   &nok_lt_status[ch]);

  }
}


int EncTf_aacscal_encode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      *baselayer_spectrum[MAX_TIME_CHANNELS],
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         *sfb_offset,
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_bits[MAX_TF_LAYER],
  int         reservoir_bits,
  int         needed_bits,
  HANDLE_AACBITMUX *bitmux,
#ifdef I2R_LOSSLESS
  HANDLE_AACBITMUX *ll_bitmux,
  int		  *quant_aac[MAX_TIME_CHANNELS],
  AACQuantInfo quantInfo[MAX_TIME_CHANNELS][MAX_TF_LAYER],
  LIT_LL_Info ll_info[MAX_TIME_CHANNELS],
  int		  *int_spectral_line_vector[MAX_TIME_CHANNELS],		
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS],
  int		  osf,
#endif
  int         nr_layer,
  int         core_max_sfb,
  int         isFirstTfLayer,
  int         hasMonoCore,
  MSInfo      *msInfo,
  int         tns_transmitted[],
  WINDOW_SEQUENCE  blockType[MAX_TIME_CHANNELS],
  WINDOW_SHAPE     windowShape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     windowShapePrev[MAX_TIME_CHANNELS],  /* needed for LTP - tool */
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         nr_of_chan[MAX_TF_LAYER],
  long        samplRate,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS* nok_lt_status,
  PRED_TYPE   pred_type,
  int         num_window_groups,
  int         window_group_length[8],
  int         layerBandwidth[MAX_TF_LAYER],
  enum DC_FLAG force_dc,
  int         debugLevel
  )


{
  int pns_sfb_start[MAX_TF_LAYER] = {-1, -1, -1, -1, -1, -1, -1, -1}; /*no PNS*/
  int total_bits = 0;
  int i, lay;
  int i_ch;
  enum DC_FLAG FssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  HANDLE_AACBITMUX layerMux = NULL;
#ifdef I2R_LOSSLESS
  HANDLE_AACBITMUX ll_layerMux = NULL;
  WINDOW_TYPE block_type_lossless_only = ONLY_LONG_SEQUENCE;
  double pcm_scaling[4] = {1.0, 1.0/16, 1.0/256, 1.0/65536};   
#endif
  int isFirstLayer = (core_max_sfb<=0);
  int isFirstStereoLayer;
  int hasMonoLayer = (hasMonoCore)||(nr_of_chan[0]==1);
  int max_sfb_now;
  int prev_max_sfb = (core_max_sfb<0)?0:core_max_sfb;

  double diffSpectrum[/*MAX_TIME_CHANNELS*/2][BLOCK_LEN_LONG];
  double **fullSpectrum = p_spectrum;

  int channels_total = nr_of_chan[nr_layer-1];
  int tns_channel_mono_layer = 0;

#ifdef I2R_LOSSLESS

  /* Channel information */
  Ch_Info channelInfo[MAX_TIME_CHANNELS];

  /***********************************************************************/
  /* Determine channel elements      */
  /***********************************************************************/
  DetermineChInfo(channelInfo, channels_total, 0/*cplenabl*/, 0/*lfePresent*/);

#endif

  if (channels_total == 2) {  
	  
#ifndef I2R_LOSSLESS  /* done in stereoIntMDCT*/
    /* apply MS */
    MSApply(nr_of_sfb[MONO_CHAN], sfb_offset,
            msInfo->ms_used,
            (blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE)?NSHORT:1,
            blockSizeSamples,
            p_spectrum[0], p_spectrum[1]);
#endif


  }
  if ((isFirstLayer&&(channels_total==2))||(!isFirstLayer&&!hasMonoCore)) {
    MSApply(nr_of_sfb[MONO_CHAN], sfb_offset,
            msInfo->ms_used,
            (blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE)?NSHORT:1,
            blockSizeSamples,
            baselayer_spectrum[0], baselayer_spectrum[1]);
  }

  /* initialize msFssControl...
   *  not meaningful at the moment, as MS always active
   *  will break for core and/or without MS */
  for (i_ch=0; i_ch<channels_total; i_ch++) {
    for (i=0; i<SFB_NUM_MAX; i++) {
      msFssControl[i_ch][i] = 0;
    }
  }

  if (!isFirstLayer) {
    /* There is a core coder */
    if (hasMonoCore&&(nr_of_chan[0]==2)) {
      /* apply one TNS to mono channel */
      i_ch = tns_channel_mono_layer;
      if (tnsInfo[i_ch]&&!tns_transmitted[MONO_CHAN]) {
#ifdef DEBUG_TNS_CORE
        fprintf(stderr,"apply one TNS to mono channel\n");
#endif
        TnsEncode2(nr_of_sfb[MONO_CHAN],    /* Number of bands per window */
		nr_of_sfb[MONO_CHAN],       /* max_sfb */ 
		blockType[MONO_CHAN],       /* block type */
		sfb_offset,                 /* Scalefactor band offset table */
		baselayer_spectrum[MONO_CHAN], /* Spectral data array */
		tnsInfo[i_ch],              /* TNS info */
		0);                         /* Analysis filter. */
      }
    } else {
      /* apply TNS to all channels without TNS yet */
#ifdef DEBUG_TNS_CORE
      fprintf(stderr,"apply TNS to all channels without TNS yet\n");
#endif
      for (i_ch=0; i_ch<(hasMonoCore?1:2); i_ch++) {
        if (!tns_transmitted[i_ch]&&tnsInfo[i_ch]) {
          TnsEncode2(nr_of_sfb[MONO_CHAN],    /* Number of bands per window */
		nr_of_sfb[MONO_CHAN],       /* max_sfb */ 
		blockType[MONO_CHAN],       /* block type */
		sfb_offset,                 /* Scalefactor band offset table */
		baselayer_spectrum[i_ch],   /* Spectral data array */
		tnsInfo[i_ch],              /* TNS info */
		0);                         /* Analysis filter. */
        }
      }
    }

    for (i_ch=0; i_ch<(hasMonoCore?1:2); i_ch++ ) {
      for (i=0; i<blockSizeSamples; i++ ) {
        diffSpectrum[i_ch][i] = fullSpectrum[i_ch][i] - baselayer_spectrum[i_ch][i];
      }
    }
    for (; i_ch<channels_total; i_ch++ ) {
      for (i=0; i<blockSizeSamples; i++ ) {
        diffSpectrum[i_ch][i] = fullSpectrum[i_ch][i];
      }
    }

  } else {
    /* no core coder */
    for (i_ch=0; i_ch<channels_total; i_ch++ ) {
      for (i=0; i<blockSizeSamples; i++ ) {
        if (pred_type == NOK_LTP)
          fullSpectrum[i_ch][i] = baselayer_spectrum[i_ch][i];
      }
    }
  }



  {
    int total_avg_bits = 0;
    for (lay=0; lay<nr_layer; lay++) {
      total_avg_bits += average_bits[lay];
    }
  }

  for (lay=0; lay<nr_layer; lay++) {
    int avail_bits = average_bits[lay] + reservoir_bits/(nr_layer-lay);
    int used_bits = 0;

    /* bitmux for this layer */
    layerMux = bitmux[lay];
    aacBitMux_setAssignmentScheme(layerMux, nr_of_chan[lay], 1/*commonWindow*/);
    aacBitMux_setCurrentChannel(layerMux, 0);

    /* bandwidth and max_sfb */
    max_sfb_now = maxSfbFromBandwidth(layerBandwidth[lay], samplRate,
                                      blockSizeSamples, sfb_offset,
                                      blockType[MONO_CHAN]);
    if (max_sfb_now < prev_max_sfb) max_sfb_now = prev_max_sfb;
    if (debugLevel>4)
      fprintf(stderr,"ScalEncFrame: maxsfb prev: %i, now: %i\n",prev_max_sfb, max_sfb_now);

    /* mono stereo scalability and FSS control */
    if (lay==0)
      isFirstStereoLayer = (hasMonoCore)&&(nr_of_chan[lay]>1);
    else
      isFirstStereoLayer = (nr_of_chan[lay-1]==1)&&(nr_of_chan[lay]>1);

    for( i_ch=0; i_ch<nr_of_chan[lay]; i_ch++ ) {
      enum DC_FLAG override = force_dc;
      if (((lay==0)&&(isFirstLayer))||
          ((i_ch==1)&&(isFirstStereoLayer))) { /*use simulcast if no layers below*/
        override = DC_SIMUL;
      }
      CalcFssControl(fullSpectrum[i_ch],
		diffSpectrum[i_ch],
		blockType[i_ch],
		FssControl[i_ch],
		blockSizeSamples,
		sfb_width_table[i_ch],
		samplRate,
                override);
    }

    /* write header for this AAC-Layer */
    if (lay==0) {
      int fss_bands;

      if (prev_max_sfb<=0) fss_bands = getNumDiffBands(samplRate);
      else fss_bands = ((prev_max_sfb+3)/4)*4;

      used_bits += write_scalable_main_header (
			layerMux,
			max_sfb_now, prev_max_sfb,
			blockType[MONO_CHAN], windowShape[MONO_CHAN],
			num_window_groups, window_group_length,
			isFirstTfLayer,
			(nr_of_chan[lay]==2), isFirstStereoLayer,
			fss_bands, FssControl, msFssControl,
			msInfo->ms_mask, msInfo->ms_used,
			tns_transmitted, tnsInfo,
			pred_type,
			&nok_lt_status[MONO_CHAN], &nok_lt_status[MONO_CHAN+1]);
    } else {
      used_bits += write_scalable_ext_header(layerMux,
			blockType[MONO_CHAN],
			num_window_groups,
			/*nr_of_sfb[MONO_CHAN]*/max_sfb_now,
			prev_max_sfb,
			(nr_of_chan[lay]>1),
			isFirstStereoLayer,
			hasMonoLayer,
			msFssControl,
			msInfo->ms_mask,
			msInfo->ms_used,
			tnsInfo);
    }

    for (i_ch=0; i_ch<nr_of_chan[lay]; i_ch++) {
      double temp_spectrum[NUM_COEFF];
      int grouped_sfb_offsets[MAX_SCFAC_BANDS+1];
      QuantInfo qInfo;
      PnsInfo pnsInfo;
      int bits;

      aacBitMux_setCurrentChannel(layerMux, i_ch);

      /* quantize spectrum */
      if (debugLevel>3)
        fprintf(stderr,"ScalEncFrame: apply bandwidth limitation (ch=%i,bw=%i)\n",i_ch,layerBandwidth[lay]);

      bandwidth_limit_spectrum(diffSpectrum[i_ch],
			temp_spectrum,
			blockType[MONO_CHAN],
			blockSizeSamples,
			samplRate,
			layerBandwidth[lay]);

      if (debugLevel>3)
        fprintf(stderr,"ScalEncFrame: apply PNS (ch=%i,sfb=%i)\n",i_ch,pns_sfb_start[lay]);

      tf_apply_pns(temp_spectrum,
		energy[i_ch],
		sfb_offset,
		/*nr_of_sfb[i_ch]*/max_sfb_now,
		pns_sfb_start[lay],
		blockType[i_ch],
		&pnsInfo);

      bits = (avail_bits-used_bits)/(nr_of_chan[lay]-i_ch);
      if (debugLevel>3)
        fprintf(stderr,"ScalEncFrame: quantize (ch=%i,bits=%i,%i)\n",i_ch,bits,average_bits[lay]);

#ifdef I2R_LOSSLESS
      memset(p_reconstructed_spectrum[i_ch],0,1024*sizeof(double));  /*??*/
#endif

      tf_quantize_spectrum(&qInfo,
                           temp_spectrum,
                           p_reconstructed_spectrum[i_ch],
                           energy[i_ch],
                           allowed_dist[i_ch],
                           blockType[i_ch],
                           sfb_width_table[i_ch],
                           grouped_sfb_offsets,
                           /*nr_of_sfb[i_ch]*/max_sfb_now,
                           &pnsInfo,
                           bits,
                           blockSizeSamples,
                           num_window_groups,
                           window_group_length,
                           aacAllowScalefacs);

#ifdef I2R_LOSSLESS
      memset(quant_aac[i_ch], 0, 1024*sizeof(int));
      memcpy(quant_aac[i_ch], qInfo.quant, /*grouped_sfb_offsets[max_sfb_now]*/1024*sizeof(int));
      
      
      quantInfo[i_ch][lay].ll_present = 1;
      quantInfo[i_ch][lay].max_sfb = quantInfo[i_ch][lay].total_sfb = max_sfb_now;
      
      quantInfo[i_ch][lay].windowSequence = quantInfo[i_ch][lay].block_type = blockType[i_ch];
      quantInfo[i_ch][lay].num_window_groups = num_window_groups;
      
      memset(quantInfo[i_ch][lay].book_vector, 0, SFB_NUM_MAX*sizeof(int));
      memset(quantInfo[i_ch][lay].scale_factor, 0, SFB_NUM_MAX*sizeof(int));
      memset(quantInfo[i_ch][lay].quant_aac, 0, MAX_OSF*NUM_COEFF*sizeof(int));	
      
      memcpy(quantInfo[i_ch][lay].book_vector, qInfo.book_vector, max_sfb_now*sizeof(int));
      memcpy(quantInfo[i_ch][lay].scale_factor, qInfo.scale_factor, max_sfb_now*sizeof(int));
      memcpy(quantInfo[i_ch][lay].quant_aac, qInfo.quant, grouped_sfb_offsets[max_sfb_now]*sizeof(int));
      

      if (lay == MONO_CHAN)
        {
/*
  if (blockType[i_ch] == EIGHT_SHORT_SEQUENCE)
  {
  lit_ll_sfg(&quantInfo[i_ch],
  sfb_width_table[i_ch],
  int_spectral_line_vector[i_ch],
  osf
  );
  }
*/
        }
#endif

      /* packing into bitstream */
      if (debugLevel>3)
        fprintf(stderr,"ScalEncFrame: bitpack (ch=%i)\n",i_ch);
      
      used_bits += bitpack_ind_channel_stream(layerMux,
                                              NULL, /*ics_buffer for ics_info*/
                                              NULL, /*var_buffer for non-scalable-stuff*/
                                              &qInfo,
                                              1, /*common_window*/
                                              blockType[i_ch],
                                              num_window_groups,
                                              grouped_sfb_offsets,
                                              /*nr_of_sfb[i_ch]*/max_sfb_now,
                                              &pnsInfo,
                                              1/*scaleFlag*/, debugLevel);
    }
    
#ifdef I2R_LOSSLESS
    for (; i_ch<channels_total; i_ch++) {
      quantInfo[i_ch][lay].ll_present = 0; /* switch off error mapping for right channel of mono layer*/
    }
#endif

    /* Update the LTP buffers of the lowest layer. */
    if ((lay==0) && isFirstLayer && (pred_type == NOK_LTP)) {
      if (debugLevel>3)
        fprintf(stderr,"ScalEncFrame: update LTP\n");

      /* Inverse MS matrix */
      if (channels_total == 2)
        MSInverse(nr_of_sfb[MONO_CHAN], sfb_offset,
		msInfo->ms_used, 
		(blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE)?NSHORT:1,
		blockSizeSamples,
		p_reconstructed_spectrum[0], p_reconstructed_spectrum[1]);

      ltp_scal_reconstruct(blockType[MONO_CHAN],
			windowShape[MONO_CHAN], windowShapePrev[MONO_CHAN],
			channels_total,
			p_reconstructed_spectrum[0], p_reconstructed_spectrum[1],
			blockSizeSamples, NSHORT, sfb_offset,
			nr_of_sfb[MONO_CHAN], nok_lt_status, tnsInfo);

      /* re-apply MS for next layers */
      if (channels_total == 2)
        MSApply(nr_of_sfb[MONO_CHAN], sfb_offset,
		msInfo->ms_used,
		(blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE)?NSHORT:1,
		blockSizeSamples,
		p_reconstructed_spectrum[0], p_reconstructed_spectrum[1]);

    } else if (lay==nr_layer-1) {
      /* last layer: reconstruct MS */
      if (channels_total == 2)
        MSInverse(nr_of_sfb[MONO_CHAN], sfb_offset,
		msInfo->ms_used,
		(blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE)?NSHORT:1,
		blockSizeSamples,
		p_reconstructed_spectrum[0], p_reconstructed_spectrum[1]);
    }

    /* prepare next layer */
    total_bits += used_bits;
    reservoir_bits -= used_bits - average_bits[lay];

    prev_max_sfb = /*nr_of_sfb[MONO_CHAN]*/max_sfb_now;
    for (i_ch=0; i_ch<nr_of_chan[lay]; i_ch++ ) {
      for( i=0; i<blockSizeSamples; i++ ) {
        diffSpectrum[i_ch][i] -= p_reconstructed_spectrum[i_ch][i];
#ifdef I2R_LOSSLESS
        fullSpectrum[i_ch][i]/*I2R_LOSSLESS?*/ = diffSpectrum[i_ch][i];
#endif
      }
    }
  }

  if ((needed_bits - total_bits)>0) {
    int padding_bits = needed_bits - total_bits;
    total_bits += write_padding_bits(layerMux, padding_bits);
  }

/***************** Perform ERROR MAPPING AND BPGC HERE ************/

#ifdef I2R_LOSSLESS
/*
	for (i_ch=0; i_ch<nr_of_chan[MONO_CHAN]; i_ch++) {
		if (blockType[i_ch] == EIGHT_SHORT_SEQUENCE)
		 {
		   lit_ll_sfg(&quantInfo[i_ch],
			      sfb_width_table[i_ch],
			      int_spectral_line_vector[i_ch],
			      osf
			      );
		 }
    }
*/

  lit_MSEncode(int_spectral_line_vector,
               channelInfo,
               msInfo,
               sfb_offset,
               blockType,
               &quantInfo[0],
               channels_total,
               1 /* msenable */
               );

  for (i_ch=0; i_ch<channels_total; i_ch++) {

 		
    LIT_ll_errorMap(
                    int_spectral_line_vector[i_ch],  /* MDCT spectrum */
                    &quantInfo[i_ch][0],      /* AAC quantization information */ 
                    &ll_info[i_ch],
                    msInfo,
                    1,   /* msenable, */
                    osf,
                    block_type_lossless_only,
                    nr_of_sfb[i_ch],
                    sfb_offset,
                    chpo_short[0][0].cb_width,
                    chpo_short[0][0].no_of_cb,
                    1,  /* coreenable */
                    nr_layer,
                    samplRate
                    );	
  }

  for (i_ch=0;i_ch<channels_total;i_ch++) {
    LIT_ll_perceptualEnhEnc(
                            &ll_info[i_ch],
                            &quantInfo[i_ch][0],
                            712856 /*ll_bits_to_used*/ /nr_of_chan[MONO_CHAN] /*nchans*/,
                            samplRate,
                            osf,
                            nr_of_sfb[i_ch],
                            sfb_offset,
                            
                            chpo_short[0][0].no_of_cb,
                            1,  /* coreenable, */
                            &channelInfo[i_ch],
                            block_type_lossless_only,
                            1,  /* msStereoIntMDCT */
                            nr_layer
                            );
  }
  
  /* bitmux for this SLS layer */
  ll_layerMux = ll_bitmux[0];
  aacBitMux_setAssignmentScheme(ll_layerMux, nr_of_chan[0], 1/*commonWindow*/);
  aacBitMux_setCurrentChannel(ll_layerMux, 0);
  
  write_ll_sce(ll_layerMux, &ll_info[0]);
  write_ll_sce(ll_layerMux, &ll_info[1]);
  
#endif

/***********************************************************************/

  return total_bits;
}
