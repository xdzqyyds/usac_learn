/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works.
Copyright (c)1997.  

***************************************************************************/
/**************************************************************************
  nok_lt_prediction.c  -  Performs Long Term Prediction for the MPEG-4
                          T/F Decoder.
  Author(s): Mikko Suonio, Juha Ojanpera
             Nokia Research Center, Speech and Audio Systems, 1997.
  *************************************************************************/


/**************************************************************************
  External Objects Needed
  *************************************************************************/
#include <stdio.h>

#include "allHandles.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */
#include "tf_main.h"
#include "block.h"
#include "nok_ltp_common.h"
#include "buffers.h"
#include "port.h"

/* To open LTP loop just undefine following */
#define LTP_LOOP_CLOSED

/*
  Enum constant:MAX_SHORT_WINDOWS
  Purpose:      Gives the maximum number of subblocks (short windows).
  Explanation:  -  */

/*
  Variable:     debug
  Purpose:      Provided debug options.
  Explanation:  If element debug['p'] is true, we give debug information
                on long term prediction.  */

/*
  Function:     buffer2freq
  Call:         buffer2freq (p_in_data, p_out_mdct, p_overlap, windowSequence,
                             wfun_select, nlong, nshort, overlap_select,
                             num_short_win, save_window);
  Purpose:      Modified discrete cosine transform
  Input:        p_in_data[]     - input signal (length: 2*shift length)
                p_overlap[]     - buffer containing the previous frame
                                  in OVERLAPPED_MODE (length: 2*shift length)
                windowSequence  - selects the type of window to use
                wfun_select     - select window function
                nlong           - shift length for long windows
                nshort          - shift length for short windows
                overlap_select  - OVERLAPPED_MODE select
                num_short_win   - number of short windows to transform
                save_window     - if true, save the windowSequence
                                  for future use
  Output:       p_out_mdct[]    - transformed signal (length: shift length)
                p_overlap[]     - a copy of p_in_data in OVERLAPPED_MODE
                                  (length: shift length)
  Explanation:  -  */

/*
  Function:     freq2buffer
  Call:         freq2buffer (p_in_data, p_out_data, p_overlap, windowSequence,
                             nlong, nshort, wfun_select, fun_select_prev,
                             overlap_select, num_short_win);
  Purpose:      Inverse of modified discrete cosine transform
  Input:        p_in_data[]     - input signal (length: shift length)
                p_overlap[]     - the overlap buffer; does not have
                                  an effect in NON_OVERLAPPED_MODE,
                                  but have to be allocated
                                  (length: shift length)
                windowSequence  - selects the type of window to use
                nlong           - shift length for long windows
                nshort          - shift length for short windows
                wfun_select     - select window function
                overlap_select  - OVERLAPPED_MODE select
                num_short_win   - number of short windows to transform
                save_window     - if true, save windowSequence for future use
  Output:       p_out_data[]    - transformed signal (length: 2*shift length)
                p_overlap[]     - the overlap buffer; always modified
                                  (length: shift length)
  Explanation:  -  */

/*
  Function:     GetBits
  Purpose:      Reads specified number of bits from the open
                input bitstream.  */


/**************************************************************************
  External Objects Provided
  *************************************************************************/
#include "nok_lt_prediction.h"


/**************************************************************************
  Internal Objects
  *************************************************************************/
#include "nok_ltp_common_internal.h"


/**************************************************************************
  Object Definitions
  *************************************************************************/


/**************************************************************************
  Title:        nok_init_lt_pred

  Purpose:      Initialize the history buffer for long term prediction

  Usage:        nok_init_lt_pred (lt_status)


  Input:        lt_status
                        - buffer: history buffer

  Output:       lt_status
                        - buffer: filled with 0

  References:   -

  Explanation:  -

  Author(s):    Mikko Suonio
  *************************************************************************/

void
nok_init_lt_pred (NOK_LT_PRED_STATUS *lt_status)
{
  int i;

  for (i = 0; i < NOK_LT_BLEN; i++)
    lt_status->buffer[i] = 0;
}

extern int samplFreqIndex[];

/**************************************************************************
  Title:        nok_lt_predict

  Purpose:      Adds the LTP contribution to the reconstructed spectrum.

  Usage:        y = nok_ltp_predict(info, win_type, win_shape, win_shape_prev,
                                    sbk_prediction_used, sfb_prediction_used, 
                                    lt_status, weight, delay, current_frame,
                                    block_size_long, block_size_short,
                                    tns_frame_info, qc_select)

  Input:        info:
                 nsbk               - number of subblocks in a block
                 bins_per_sbk       - number of spectral coefficients
                                      in each subblock; currently we 
                                      assume that they are of equal size
                 sfb_per_bk         - total # scalefactor bands in a block
                 sfb_per_sbk        - # scalefactor bands in each subblock
                win_type            - window sequence (frame, block) type
                win_shape           - shape of the mdct window
                win_shape_prev      - shape from previous block
                sbk_prediction_used - first item toggles prediction on(1)/off(0) 
                                      for all subblocks, next items toggle it 
                                      on/off on each subblock separately
                sfb_prediction_used - first item is not used, but the following 
                                      items toggle prediction on(1)/off(0) 
                                      on each scalefactor-band of every subblock
                lt_status :
                 buffer             - history buffer for prediction
                weight              - a weight factor to apply to all subblocks
                delay               - array of delays to apply to each subblock
                current_frame       - the dequantized spectral coeffients and
                                      prediction errors where prediction is used
                block_size_long     - size of the long block
                block_size_short    - size of the short block
                tns_frame_info      - TNS parameters
                qc_select           - MPEG4/TF codec type

  Output:       y - reconstructed spectrum containing also LTP contribution

  References:   1.) buffer2freq
                2.) tns_encode_subblock in tns2.c
                3.) double_to_int
                4.) freq2buffer

  Explanation:  -

  Author(s):    Mikko Suonio, Juha Ojanpera
  *************************************************************************/

void nok_lt_predict ( Info*               info, 
                      WINDOW_SEQUENCE     win_type,
                      WINDOW_SHAPE        win_shape,
                      WINDOW_SHAPE        win_shape_prev,
                      int*                sbk_prediction_used,
                      int*                sfb_prediction_used, 
                      NOK_LT_PRED_STATUS* lt_status,
                      Float               weight, 
                      int*                delay, 
                      Float*              current_frame,
                      int                 block_size_long, 
                      int                 block_size_short,
                      int                 sampleRateIdx,
                      TNS_frame_info*     tns_frame_info,
                      QC_MOD_SELECT       qc_select)
{
  int i, j, subblock, last_band;
  int num_samples;
  double mdct_predicted[2 * NOK_MAX_BLOCK_LEN_LONG];
  double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];
  Float tmpBuffer[2 * NOK_MAX_BLOCK_LEN_LONG];
  
  switch(win_type)
    {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
      if (sbk_prediction_used[0])
        {
          /* Prediction for time domain signal */
          num_samples =  2 * block_size_long;
          j = NOK_LT_BLEN - 2 * block_size_long - delay[0];
          if(qc_select != AAC_LD)
            {
              j = NOK_LT_BLEN - 2 * block_size_long - (delay[0] - DELAY / 2);
              if(NOK_LT_BLEN - j <  2 * block_size_long)
                num_samples = NOK_LT_BLEN - j;
            }

          for(i = 0; i < num_samples; i++)
            predicted_samples[i] = weight * lt_status->buffer[i + j];
          for( ; i < 2 * block_size_long; i++)
            predicted_samples[i] = 0.0f;
        
          /* Transform prediction to frequency domain. */
          buffer2freq (predicted_samples, mdct_predicted, NULL, win_type,
                       win_shape, win_shape_prev, block_size_long, 
                       block_size_short, NON_OVERLAPPED_MODE,0,0, 1);

          /* 
           * TNS analysis filter the predicted spectrum. 
           */
          if(tns_frame_info != NULL)
            {
              for(i = 0; i < block_size_long; i++)
                tmpBuffer[i] = mdct_predicted[i];
          
              tns_encode_subblock(tmpBuffer, info->sfb_per_bk, 
                                  info->sbk_sfb_top[0], 1, 
                                  &(tns_frame_info->info[0]),
                                  qc_select, sampleRateIdx);
          
              for(i = 0; i < block_size_long; i++)
                mdct_predicted[i] = tmpBuffer[i];
            }
        
          /* Clean those sfb's where prediction is not used. */
          for (i = 0, j = 0; i < info->sfb_per_bk; i++)
            if (sfb_prediction_used[i + 1] == 0)
              for (; j < info->sbk_sfb_top[0][i]; j++)
                mdct_predicted[j] = 0.0;
            else
              j = info->sbk_sfb_top[0][i];

#ifdef LTP_LOOP_CLOSED
          /* Add the prediction to dequantized spectrum. */
          for (i = 0; i < block_size_long; i++)
            current_frame[i] = current_frame[i] + mdct_predicted[i];
#endif
        }
      break;
      
    case EIGHT_SHORT_SEQUENCE:
      last_band = (sfb_prediction_used[1] < NOK_MAX_LT_PRED_SHORT_SFB) ?
	sfb_prediction_used[1] : NOK_MAX_LT_PRED_SHORT_SFB;
      if(sbk_prediction_used[0] && last_band){  /**TM */ 
      for (subblock = 0; subblock < info->nsbk; subblock++)
      {
	if (sbk_prediction_used[0] && sbk_prediction_used[subblock + 1])
	{ 
	  /* 
	   * Prediction for time domain signal. The buffer shift
	   * causes an additional delay depending on the subblock
	   * number.
	   */
	  num_samples = 2 * block_size_short;
	  j = NOK_LT_BLEN - num_samples - delay[subblock];
	  if(NOK_LT_BLEN - j <  2 * block_size_short)
	    num_samples = NOK_LT_BLEN - j;
	  
	  for(i = 0; i < num_samples; i++)
	    predicted_samples[i] = weight * lt_status->buffer[i + j];
	  for( ; i < 2 * block_size_long /**TM block_size_short */; i++)
	    predicted_samples[i] = 0.0f;

	  /* Transform prediction to frequency domain.  */
	  buffer2freq (predicted_samples, mdct_predicted, NULL, win_type,
		       win_shape, win_shape_prev, block_size_long,
		       block_size_short, NON_OVERLAPPED_MODE,0,0, 1);
	  
	  /* 
	   * TNS analysis filter the predicted subblock spectrum.
	   */
	  if(tns_frame_info != NULL)
	  {
	    if(subblock < tns_frame_info->n_subblocks)
	    {
	      for(i = 0; i < block_size_short; i++)
		tmpBuffer[i] = mdct_predicted[i];
	      
	      tns_encode_subblock(tmpBuffer, last_band, 
				  info->sbk_sfb_top[subblock],
				  0, &(tns_frame_info->info[subblock]),
          qc_select, sampleRateIdx);
	      
	      for(i = 0; i < block_size_short; i++)
		mdct_predicted[i] = tmpBuffer[i];
	    }
	  }
	  
#ifdef LTP_LOOP_CLOSED
    /* Add the prediction to dequantized spectrum.  */
	  for (i = 0; i < info->sbk_sfb_top[subblock][last_band - 1]; i++)
	    current_frame[subblock * block_size_short + i] += mdct_predicted[i];
#endif
	  }
      }
      }
      break;
      
    default:
      break;
    }

}


/**************************************************************************
  Title:        nok_lt_update

  Purpose:      Updates the time domain buffer of LTP.

  Usage:        nok_lt_update(lt_status, time_signal, overlap_signal, 
                              block_size_long)
                                   
  Input:        time_signal     - reconstructed time signal
                overlap_signal  - alias buffer
                block_size_long - size of the long block

  Output:       lt_status :
                 buffer         - history buffer for prediction

  References:   1.) double_to_int

  Author(s):    Mikko Suonio, Juha Ojanpera
  *************************************************************************/

void 
nok_lt_update(NOK_LT_PRED_STATUS *lt_status, double *time_signal, 
              double *overlap_signal, int block_size_long)
{
  int i;
  
  for(i = 0; i < NOK_LT_BLEN - 2 * block_size_long; i++)
    lt_status->buffer[i] = lt_status->buffer[i + block_size_long];
  
  for(i = 0; i < block_size_long; i++) 
    {
      lt_status->buffer[NOK_LT_BLEN - 2 * block_size_long + i] = 
        time_signal[i];
    
      lt_status->buffer[NOK_LT_BLEN - block_size_long + i] = 
        overlap_signal[i];
    }
  
}



/**************************************************************************
  Title:        nok_lt_decode

  Purpose:      Decode the bitstream elements for long term prediction

  Usage:        y = nok_lt_decode(win_type, max_sfb, sbk_prediction_used,
                                  sfb_prediction_used, weight, delay,
                                  hResilience, hVm, hEscInstanceData, qc_select,
                                  coreCodecIdx, max_sfb_tvq_set)


  Input:        win_type        - window sequence (frame, block) type
                max_sfb         - # scalefactor bands used in the current frame
                hResilience     - error resilience stuff
                hVm             - same as above
                hEscInstanceData         - same as above
                qc_select       - MPEG4/TF codec type
                coreCodecIdx    - core codec (used only in scaleable mode)
                max_sfb_tvq_set - 1 if max_sfb need not to be read from the bit 
                                  stream, 0 otherwise (used only in scaleable mode)

  Output:       y               - # bits read from the stream
                sbk_prediction_used - first item toggles prediction 
                                      on(1)/off(0) for all subblocks, 
                                      next items toggle it on/off on
                                      each subblock separately
                sfb_prediction_used - first item is not used, but the 
                                      following items toggle prediction 
                                      on(1)/off(0) on each scalefactor-band 
                                      of every subblock
                weight          - a weight factor to apply to all subblocks
                delay           - array of delays to apply to each subblock

  References:   1.) GetBits

  Explanation:  -

  Author(s):    Mikko Suonio, Juha Ojanpera
  *************************************************************************/

int 
nok_lt_decode ( WINDOW_SEQUENCE   win_type, 
                byte*             max_sfb, 
                int*              sbk_prediction_used,
                int*              sfb_prediction_used, 
                Float*            weight, 
                int*              delay,
                HANDLE_RESILIENCE hResilience,
                HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                QC_MOD_SELECT     qc_select,
                enum CORE_CODEC   coreCodecIdx,
                int               max_sfb_tvq_set,
                HANDLE_BUFFER     hVm)
{
  int i;
  int last_band = 0;
  int read_bits;
  int ltp_lag_update;


  *weight = 0.0;
  read_bits = LEN_LTP_DATA_PRESENT;
  sbk_prediction_used[0] = GetBits ( LEN_LTP_DATA_PRESENT,
                                     LTP_DATA_PRESENT,
                                     hResilience, 
                                     hEscInstanceData,
                                     hVm );
  if ((sbk_prediction_used[0]))
    {
      if(coreCodecIdx == NTT_TVQ && !max_sfb_tvq_set)
        {
          *max_sfb = (unsigned char ) GetBits(6, 
                                              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                              hResilience, 
                                              hEscInstanceData, 
                                              hVm);
          read_bits += 6;
        }
        
      if (qc_select == AAC_LD) {
        read_bits += LEN_LTP_LAG_UPDATE;
        
        ltp_lag_update = GetBits ( LEN_LTP_LAG_UPDATE,
                                   LTP_LAG_UPDATE,
                                   hResilience, 
                                   hEscInstanceData,
                                   hVm ); 
        if (ltp_lag_update) 
          {
            read_bits += LEN_LTP_LAG_LD;
          
            delay[0] = GetBits ( LEN_LTP_LAG_LD,
                                 LTP_LAG,
                                 hResilience, 
                                 hEscInstanceData,
                                 hVm );
          }
      } 
      else
        {
          read_bits += LEN_LTP_LAG;
          delay[0] = GetBits ( LEN_LTP_LAG,
                               LTP_LAG,
                               hResilience, 
                               hEscInstanceData,
                               hVm );
        }
        
      *weight = codebook[GetBits ( LEN_LTP_COEF,
                                   LTP_COEF,
                                   hResilience, 
                                   hEscInstanceData,
                                   hVm )];

      read_bits += LEN_LTP_COEF;
    
      if (win_type != EIGHT_SHORT_SEQUENCE)
        {
          last_band = (*max_sfb < NOK_MAX_LT_PRED_LONG_SFB
                       ? *max_sfb : NOK_MAX_LT_PRED_LONG_SFB) + 1;

          read_bits += last_band - 1;
      
          sfb_prediction_used[0] = sbk_prediction_used[0];
          for (i = 1; i < last_band; i++)
            sfb_prediction_used[i] = GetBits ( LEN_LTP_LONG_USED,
                                               LTP_LONG_USED,
                                               hResilience, 
                                               hEscInstanceData,
                                               hVm );
          for (; i < MAX_SCFAC_BANDS; i++)
            sfb_prediction_used[i] = 0;
        }
      else
	{
	  int ltp_lag;
	  
	  ltp_lag = delay[0];
          read_bits += NSHORT;
      
          sfb_prediction_used[0] = sbk_prediction_used[0];
	  sfb_prediction_used[1] = *max_sfb;
          for (i = 0; i < NSHORT; i++)
            {
              delay[i] = 0;
              if ((sbk_prediction_used[i+1] = GetBits ( LEN_LTP_SHORT_USED,
                                                        LTP_SHORT_USED,
                                                        hResilience, 
                                                        hEscInstanceData,
                                                        hVm )))
                {
		  read_bits++;
		  
		  delay[i] = ltp_lag;
		  if (GetBits (  LEN_LTP_SHORT_LAG_PRESENT,
				 LTP_SHORT_LAG_PRESENT,
				 hResilience, 
				 hEscInstanceData,
				 hVm ))
		  {
		    int tmp;
		    
		    read_bits += LEN_LTP_SHORT_LAG;
		    
		    tmp = GetBits ( LEN_LTP_SHORT_LAG,
				    LTP_SHORT_LAG,
				    hResilience, 
				    hEscInstanceData,
				    hVm );

		    tmp -= (NOK_LTP_LAG_OFFSET >> 1);
		    delay[i] = tmp + ltp_lag;
		  }
                }
            }
        }
    }
  
  if (debug['p'])
    {
      if (sbk_prediction_used[0])
        {
          fprintf(stderr, "long term prediction enabled (%2d):  ", last_band);
          for (i = 1; i < *max_sfb + 1; i++)
            fprintf(stderr, " %d", sfb_prediction_used[i]);
          fprintf(stderr, "\n");
          for (i = 1; i < MAX_SHORT_WINDOWS + 1; i++)
            fprintf(stderr, " %d", sbk_prediction_used[i]);
          fprintf(stderr, "\n");
        }
    }

  return (read_bits);
}
