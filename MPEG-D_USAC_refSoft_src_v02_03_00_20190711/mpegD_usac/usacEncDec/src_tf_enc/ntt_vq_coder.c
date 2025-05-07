/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Takeshi Mori (NTT) on 2000-11-21,                                       */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-13                           */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 25-aug-1997  NI  bug fixes */

#include <stdio.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */
#include "ms.h"

#include "allVariables.h"        /* variables */

/*#include "tf_main.h"*/
#include "bitstream.h"
#include "nok_ltp_enc.h" 
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_encode.h"

#include "bitmux.h" /* write_tns_data */


static int tvqScalEnc(
  double *tvq_target_spectrum[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  HANDLE_BSBITSTREAM *output_au,
  int         mat_shift[ntt_N_SUP_MAX],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         iscl,
  int         *used_bits_scl,
  int         debug_level
);

static void ntt_maxsfb(
  ntt_INDEX   *index,
  int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int     nr_of_sfb[MAX_TIME_CHANNELS],
  int     iscl,
  int     debugLevel
);

void ntt_vq_coder(double      *spectral_line_vector[MAX_TIME_CHANNELS],
		  double      external_pw[],
		  int         pw_select,
		  WINDOW_SEQUENCE         windowSequence,
		  ntt_INDEX   *index,
		  ntt_PARAM   *param_ntt,
		  int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
                  int         nr_of_sfb[MAX_TIME_CHANNELS],
		  int         available_bits,
		  double      lpc_spectrum[],
		  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
		  int         debugLevel)
{
  static double bark_env[ntt_T_FR_MAX];
  static double spectrum[ntt_T_FR_MAX];
  static double pitch_sequence[ntt_T_FR_MAX];
  static double gain[ntt_T_SHRT_MAX];
  static double perceptual_weight[ntt_T_FR_MAX];
  int    sb, ismp, i_ch, top;
  double spectrum_gain = 0;
  Info   *sfbInfo[4], *p_sfbInfo;
  int nsbk = 0;
  int nfr = 0;

  /*--- Initialization ---*/
  
 {
  if(index->w_type == EIGHT_SHORT_SEQUENCE){
     p_sfbInfo = &eight_short_info;
     p_sfbInfo->nsbk = 8;
     p_sfbInfo->islong = 0;
     for(ismp=0; ismp<8; ismp++){
       p_sfbInfo->bins_per_sbk[ismp] = index->block_size_samples/8;
       p_sfbInfo->sfb_per_sbk[ismp]= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index->max_sfb[0] /*p_sfbInfo->sfb_per_sbk[ismp]*/; sb++){
	 top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = top;
       }
     }
  }
  else{
     p_sfbInfo = &only_long_info;
     p_sfbInfo->nsbk = 1;
     p_sfbInfo->islong = 1;
     p_sfbInfo->bins_per_bk = index->block_size_samples;
       p_sfbInfo->sfb_per_bk= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index->max_sfb[0] /*p_sfbInfo->sfb_per_sbk[0]*/; sb++){
         top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = top;
       }
  }
  sfbInfo[index->w_type] = p_sfbInfo;
  
}
   if(debugLevel >5)
   fprintf(stderr, "UUUUUUU %5d %5d %5d available_bits ntt_NMTOOL_BITS\n",
    available_bits, ntt_NMTOOL_BITSperCH*index->numChannel+3, 
    ntt_NMTOOL_BITS_SperCH*index->numChannel+1);

  /*--- Pre process ---*/
  if (param_ntt->ntt_param_set_flag == 0){
    param_ntt->fine_sw = 0;
    param_ntt->speech_sw = 0;
  }
  index->w_type = windowSequence;

  /*--- Bit number control ---*/
  switch(windowSequence){
    /* long frame */
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    index->nttDataBase->ntt_VQTOOL_BITS 
      = available_bits - (ntt_NMTOOL_BITSperCH*index->numChannel+3);
    spectrum_gain = 1./sqrt((double)index->block_size_samples*2.);
    nsbk=1;
    nfr = index->block_size_samples;
    break;
    /* short frame */
  case EIGHT_SHORT_SEQUENCE:
    index->nttDataBase->ntt_VQTOOL_BITS_S 
      = available_bits - (ntt_NMTOOL_BITS_SperCH*index->numChannel+1);
    spectrum_gain = 1./sqrt((double)index->block_size_samples/8*2.);
    nsbk=8;
    nfr = index->block_size_samples/8;
    break;
  default:
    fprintf(stderr, "ntt_vq_coder(): Error. %d: No such window type.\n",
	    windowSequence);
    exit(1);
    break;
  }

  /*--- Spectrum line normalization
    (for gain alignment of NTT & MPEG4 VM MDCT ---*/
  for (i_ch=0; i_ch<index->numChannel; i_ch++){
    top = i_ch * index->block_size_samples;
    for (ismp=0; ismp<index->block_size_samples; ismp++){
      spectrum[ismp+top] = spectral_line_vector[i_ch][ismp] * spectrum_gain;
    }
  }
  
  if(index->numChannel == 2){
  int sb, win;
  int sfbw;
  double buf[ntt_N_FR_MAX];
   /* if(index->ms_mask ==1 ) */
    {
     for( win=0; win<nsbk; win++ ) {
     int sboffs   = win*nfr;

     for( sb=0; sb<index->max_sfb[0] /*nr_of_sfb[MONO_CHAN]*/; sb++ ) {
           sfbw = sfb_width_table[MONO_CHAN][sb];

       if( index->msMask[win][sb] != 0  ) {
         for (ismp=sboffs; ismp<sboffs+sfbw; ismp++){
           buf[ismp] =  (spectrum[ismp] - spectrum[ismp+index->block_size_samples])/2.;
           spectrum[ismp] =  (spectrum[ismp]+spectrum[ismp+index->block_size_samples])/2.;
           spectrum[ismp+index->block_size_samples]  = buf[ismp];
         }
       }
       sboffs += sfbw;
      }
     }
    }
/*
    else if(index->ms_mask ==2){
        for (ismp=0; ismp<index->block_size_samples; ismp++){
	       buf[ismp] =  (spectrum[ismp] - spectrum[ismp+index->block_size_samples])/2.;
	       spectrum[ismp] =(spectrum[ismp]+spectrum[ismp+index->block_size_samples])/2.;
	       spectrum[ismp+index->block_size_samples]  = buf[ismp];
        }
    }
*/
  }

  /*--- Process spectrum ---*/
  ntt_tf_proc_spectrum(spectrum, index,
		       lpc_spectrum, bark_env,
		       pitch_sequence, gain);
  switch(windowSequence){
    /* pitch_q flag long frame */
  case ONLY_LONG_SEQUENCE:
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    if( index->pitch_q == 1) 
	  index->nttDataBase->ntt_VQTOOL_BITS -=  ntt_PIT_TBITperCH*index->numChannel;
    break;
  case EIGHT_SHORT_SEQUENCE: /* T.Ishikawa 981012*/
	break;
  case NUM_WIN_SEQ:
	break;
  default:
	  break;
  }
  if(index->bandlimit == 1){
	index->nttDataBase->ntt_VQTOOL_BITS   -= ntt_BWID_BITS*index->numChannel;
	index->nttDataBase->ntt_VQTOOL_BITS_S -= ntt_BWID_BITS*index->numChannel;
  }
      /*--- Perceptual model ---*/
  if (pw_select == NTT_PW_EXTERNAL &&
      (index->w_type == ONLY_LONG_SEQUENCE ||
       index->w_type == LONG_START_SEQUENCE ||
       index->w_type == LONG_STOP_SEQUENCE )){
    /* external perceptual weighting */
    ntt_movdd(index->block_size_samples*index->numChannel, external_pw, perceptual_weight);
  }
  else{
    /* internal perceptual weighting */
    ntt_tf_perceptual_model(lpc_spectrum, bark_env, gain, windowSequence,
			    spectrum, 
			    pitch_sequence, 
			    index->numChannel,
			    index->block_size_samples,
			    index->nttDataBase->bandUpper,
			    perceptual_weight);
  }

  /*--- Quantize spectrum ---*/
  ntt_tf_quantize_spectrum(spectrum, lpc_spectrum, bark_env,
			   pitch_sequence,
			   gain, perceptual_weight, index, debugLevel);
  ntt_vq_decoder(index,  reconstructed_spectrum, sfbInfo);
  { 
    int ii, jj;
    float dummy;
    for (ii=0; ii<index->numChannel; ii++){
      for(jj=0; jj<index->block_size_samples; jj++){
        dummy=(float)reconstructed_spectrum[ii][jj];
        reconstructed_spectrum[ii][jj]=dummy;
      }
    }
  }
}

static
void ntt_headerenc(int iscl,
		   HANDLE_BSBITSTREAM  stream, 
		   ntt_INDEX* index, 
		   WINDOW_SHAPE windowShape,
		   int *used_bits,
		   TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
		   NOK_LT_PRED_STATUS *nok_lt_status,
		   PRED_TYPE pred_type,
		   int debugLevel)
{
  int g,sfb,win;
  int gwin, num_groups;
  short group_len[8];

  if (iscl<0) {
    BsPutBit(stream, index->w_type, 2);
    *used_bits += 2;
    BsPutBit(stream,  (unsigned long)windowShape, 1);
    *used_bits += 1;
  }

  if(index->numChannel > 1){
    BsPutBit(stream, (unsigned int)index->ms_mask, 2);
    *used_bits += 2;
  }
  if( index->numChannel==2 ){
    if (index->ms_mask==1){
      if (index->w_type== EIGHT_SHORT_SEQUENCE ){
        if(iscl < 0){ /* only core */
          BsPutBit( stream,index->group_code,7);      /* max_sfb */
          *used_bits += 7;
        }
        num_groups =
              TVQ_decode_grouping ( index->group_code, group_len );
      } else {
        num_groups = 1;
        group_len[0]=1;
      }
      win=0;
      for (g=0;g<num_groups;g++){
        for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
          BsPutBit(stream,index->msMask[win][sfb],1);/*ms_mask*/
          *used_bits += 1;
        }
        for (gwin=1;gwin< group_len[g]; gwin++){
          for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
            index->msMask[win+gwin][sfb]=index->msMask[win][sfb];
            /* apply same ms mask to a ll subwindows of same group */
          }
        }
        /*win += (gwin -1);*/
        win += group_len[g];
      }
    }
  }
  if(iscl <0) {
    WINDOW_SEQUENCE windowSequence;
    int i_ch;

    windowSequence = index->w_type;
   
    for(i_ch=0; i_ch < index->numChannel; i_ch++) {
      if(pred_type == NOK_LTP) { /* LTP used. */
        *used_bits += nok_ltp_encode(NULL, stream, windowSequence, 
				     index->max_sfb[0],
				     nok_lt_status + i_ch, NTT_VQ, 
				     NTT_TVQ, 1 /**TM i_ch***/);
      } else {
        BsPutBit(stream, 0, 1); /* LTP not used. */
        *used_bits += 1;
      }

      /* TNS data */
      if(tnsInfo[MONO_CHAN]) {
	if (tnsInfo[MONO_CHAN]->tnsDataPresent) {     /* TNS active */
          int tns_bits,bits_tmp;
          bits_tmp=0;
          tns_bits= write_tns_data( NULL, 0, tnsInfo[MONO_CHAN], 
                                    windowSequence, NULL );
          if(debugLevel>5)
            fprintf(stderr, "AAAAA TNS %5d %5d\n", 
	      tnsInfo[MONO_CHAN]->tnsDataPresent, tns_bits);

          switch (index->w_type){
          case ONLY_LONG_SEQUENCE: 
          case LONG_START_SEQUENCE:
          case LONG_STOP_SEQUENCE:
            bits_tmp = index->nttDataBase->ntt_NBITS_FR
			   -(ntt_NMTOOL_BITSperCH*index->numChannel)-tns_bits-(*used_bits);
            break;
          case EIGHT_SHORT_SEQUENCE:
            bits_tmp = index->nttDataBase->ntt_NBITS_FR
		      -(ntt_NMTOOL_BITS_SperCH*index->numChannel)
		      -tns_bits-(*used_bits);
            break;
          default:
	    break;
          }

          if(bits_tmp<50) { /* TNS not active T.Ishikawa 981214 */
            BsPutBit(stream,0,1);
            *used_bits += 1;
            tnsInfo[MONO_CHAN]->tnsDataPresent=0;
            index->restore_flag_tns=1;
            if(debugLevel>2)
              fprintf(stderr,"tnsDataNotPresent %5d ,bits_tmp %5d\n",tns_bits,bits_tmp);
	  } else { /* TNS active T.Ishikawa 981214 */
            *used_bits+=write_tns_data( NULL, 0, tnsInfo[MONO_CHAN], 
                                        windowSequence, stream );
            index->restore_flag_tns=0;
            if(debugLevel>2)
              fprintf(stderr,"tnsDataPresent %5d bits_tmp %5d\n",tns_bits,bits_tmp);
          }
        } else {                    /* switch it off */
          BsPutBit(stream,0,1);
          *used_bits += 1;
        }
      } else {                    /* switch it off */
        BsPutBit(stream,0,1);
        *used_bits += 1;
      }

      if(debugLevel>2)
        fprintf(stderr, "TNS end %5d\n",  *used_bits); 
    }
  } /* only for core */
}

static
void ntt_tns_enc(double *spectral_line_vector[MAX_TIME_CHANNELS],
		 ntt_INDEX  *index, 
	         int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
	         WINDOW_SEQUENCE   windowSequence,
		 int         nr_of_sfb[MAX_TIME_CHANNELS],
		 TNS_INFO	 *tnsInfo[MAX_TIME_CHANNELS]
		)
{

  if(tnsInfo[MONO_CHAN] != NULL) {
    int i_ch;
    int sfb, k, sfb_offset[ 100 ];

    /* calc. sfb offset table */
    for (sfb=k=0; sfb<nr_of_sfb[MONO_CHAN]; sfb++) {
      sfb_offset[sfb] = k;
      k += sfb_width_table[MONO_CHAN][sfb];
    }
    sfb_offset[sfb] = k;

    for (i_ch=0; i_ch<index->numChannel; i_ch++) {
      TnsEncode(nr_of_sfb[MONO_CHAN],        /* Number of bands per w indow */
                nr_of_sfb[MONO_CHAN],        /* max_sfb */
                windowSequence,              /* block type */
                sfb_offset,                  /* Scalefactor band offs t table */
                spectral_line_vector[i_ch],  /* Spectral data array */
                tnsInfo[i_ch]);             
   } 
  }
}


int  EncTf_tvq_encode(
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *baselayer_spectral_line_vector[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  int         block_size_samples,
  HANDLE_BSBITSTREAM *output_stream,
  int         mat_shift[][ntt_N_SUP_MAX],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         *max_sfb,
  MSInfo      *msInfo,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
  int         debugLevel)
{
	int     output_stream_no = 0;
	int     ismp, tomo_tmp;
	int     used_bits, ntt_available_bits;
	double  lpc_spectrum[ntt_T_FR_MAX];
	double *tvq_target_spectrum[MAX_TIME_CHANNELS];
	double baselayer_rec_spectrum[BLOCK_LEN_LONG];
	int i_sup;

	HANDLE_BSBITSTREAM stream_for_esc[2];
	if ((index_scl->er_tvq==1)&&(index_scl->nttDataScl->epFlag==1)) {
	  /* ep_config == 1 */
	  stream_for_esc[0] = output_stream[0];
	  stream_for_esc[1] = output_stream[1];
	  output_stream_no = 2;
	} else {
	  stream_for_esc[0] = output_stream[0];
	  stream_for_esc[1] = output_stream[0];
	  output_stream_no = 1;
	}

	/*windowShape[MONO_CHAN]=WS_FHG;*/ /* <---*/ /* --->???*/
	index->w_type = windowSequence[MONO_CHAN];
	index->group_code=127;
	windowSequence[index->numChannel-1] = windowSequence[MONO_CHAN];

	if(debugLevel>5)
	  fprintf(stderr,"windowSequence %d\n",(int)windowSequence[MONO_CHAN]);
	  
	used_bits=0;

	index->group_code =0x7f;
	index->restore_flag_tns=0;
	index->last_max_sfb[0] =0;
	ntt_maxsfb( index, sfb_width_table, nr_of_sfb, -1, debugLevel);

	/*OK: was previously performed on spectrum before LTP, after TNS
	ntt_select_ms( spectral_line_vector,
		        index, sfb_width_table, nr_of_sfb, -1, debugLevel);
	*/
	if (index->numChannel==2) {
	  int iii,jjj;
	  for(iii=0; iii<8; iii++){
	    for(jjj=0; jjj<nr_of_sfb[MONO_CHAN]; jjj++){
	      index->msMask[iii][jjj]=msInfo->ms_used[iii][jjj];
	    }
	  }
	  index->ms_mask = msInfo->ms_mask;
	}

	tvq_target_spectrum[0] = spectral_line_vector[0];
	tvq_target_spectrum[1] = spectral_line_vector[1];
	if(debugLevel>1) 
	  fprintf(stderr,"ntt_tns_enc END pred_type = %2d %2d\n", pred_type, NOK_LTP);

	/***TM9902*/
	if (pred_type == NOK_LTP) {
	  for(i_sup = 0; i_sup < index->numChannel; i_sup++) {
	    tvq_target_spectrum[i_sup] = baselayer_spectral_line_vector[i_sup];
          }
	}
	if(debugLevel>5) 
	  fprintf(stderr, "MSMSMS %5d \n", index->ms_mask);
	ntt_headerenc(-1, stream_for_esc[0], index,
		      windowShape[MONO_CHAN],
		      &used_bits, tnsInfo, nok_lt_status, pred_type,
		      debugLevel);
	if(debugLevel>2)  fprintf(stderr,"headerenc %d\n",used_bits);
	ntt_available_bits = index->nttDataBase->ntt_NBITS_FR - used_bits;
	  
	ntt_vq_coder(spectral_line_vector,
                     lpc_spectrum /*ntt_external_pw[MONO_CHAN]*/,
                     NTT_PW_INTERNAL /*ntt_pw_select*/,
                     windowSequence[MONO_CHAN],
                     index,
                     param_ntt,
                     sfb_width_table,
	             nr_of_sfb,
	             ntt_available_bits,
                     lpc_spectrum,
                     reconstructed_spectrum,
		     debugLevel);

	/* Bit packing */

	if(debugLevel>5){
	   fprintf(stderr," before pack%5ld (esc0)\n",BsCurrentBit(stream_for_esc[0]));
	   fprintf(stderr," before pack%5ld (esc1)\n",BsCurrentBit(stream_for_esc[1]));
	}
	tomo_tmp=used_bits;
	used_bits += ntt_BitPack(index, stream_for_esc, NULL, -1);
	if (debugLevel>2) 
	  fprintf(stderr,"____ ntt_BaseLayer bit %d %d\n",used_bits-tomo_tmp,used_bits);

	if (debugLevel > 5) {
	  fprintf(stderr,"  after pack%5ld (esc0)\n",BsCurrentBit(stream_for_esc[0]));
	  fprintf(stderr,"  after pack%5ld (esc1)\n",BsCurrentBit(stream_for_esc[1]));
	}

	/* 
	 * Restore the spectrum of the base layer and update LTP buffer. 
	 */
	if(pred_type == NOK_LTP) {
	  for(i_sup = 0; i_sup < index->numChannel; i_sup++) {   
	    for(ismp = 0; ismp < block_size_samples; ismp++)
	      baselayer_rec_spectrum[ismp] = 
		 reconstructed_spectrum[i_sup][ismp];

	    /* Add the LTP contribution to the reconstructed spectrum. */
	    nok_ltp_reconstruct(baselayer_rec_spectrum, 
				windowSequence[i_sup], 
				&sfb_offset[i_sup][0], nr_of_sfb[i_sup],
				block_size_samples/8,
				&nok_lt_status[i_sup]);
	       
	    /* This is passed to the scaleable encoder. */
	    for(ismp = 0; ismp < block_size_samples; ismp++)
	      reconstructed_spectrum[i_sup][ismp] = 
		 baselayer_rec_spectrum[ismp];
	    /* TNS synthesis filtering. */
	    if(tnsInfo[i_sup] != NULL && (index->restore_flag_tns==0))
	      TnsEncode2(nr_of_sfb[i_sup], nr_of_sfb[i_sup], 
			 windowSequence[i_sup], &sfb_offset[i_sup][0], 
			 baselayer_rec_spectrum, tnsInfo[i_sup] , 1);
	    /* Update the time buffer of LTP. */
	    nok_ltp_update(baselayer_rec_spectrum, nok_lt_status[i_sup].overlap_buffer, 
			   windowSequence[i_sup], windowShape[MONO_CHAN], 
			   windowShape[MONO_CHAN], block_size_samples, 
			   block_size_samples/8, 8, &nok_lt_status[i_sup]);
	  }
	}
	  
	/* Scalable encoder */
        {
	  int iscl;
	  int  iii, jjj;
	  for(iii=0; iii<8; iii++){
	    for(jjj=0; jjj</*index->max_sfb[0]*/nr_of_sfb[MONO_CHAN]; jjj++){
	      index_scl->msMask[iii][jjj]=index->msMask[iii][jjj];
	    }
	  }
	  index_scl->max_sfb[0] = index->max_sfb[0];
          index_scl->w_type = index->w_type;
          index_scl->group_code = index->group_code;
	  index_scl->pf = index->pf;
          if(debugLevel >5)
	    fprintf(stderr, "LLL %5d ntt_NSclLay \n", index->nttDataScl->ntt_NSclLay); 
	  for (iscl=0; iscl<index->nttDataScl->ntt_NSclLay; iscl++){
            int used_bits_scl;
            output_stream_no +=
	      tvqScalEnc( tvq_target_spectrum,
			index,
			index_scl,
			param_ntt,
			&output_stream[output_stream_no],
			mat_shift[iscl],
			sfb_width_table,
			nr_of_sfb,
			reconstructed_spectrum,
			iscl,
                        &used_bits_scl,
			debugLevel );
            used_bits += used_bits_scl;
	  }
	}

	{
	  ntt_INDEX *lastIndex = index;
	  if (index->nttDataScl->ntt_NSclLay != 0) {
	    lastIndex = index_scl;
	  }
	  *max_sfb = lastIndex->max_sfb[index->nttDataScl->ntt_NSclLay];
	}
	return(used_bits);
}

static int tvqScalEnc(
  double      *tvq_target_spectrum[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  HANDLE_BSBITSTREAM *output_stream,
  int         mat_shift[ntt_N_SUP_MAX],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         iscl,
  int         *used_bits_scl,
  int         debugLevel
)

{
	double  lpc_spectrum[ntt_T_FR_MAX]; /*dummy*/
	int ntt_available_bits;
	int tomo_tmp, tomo_tmp2, used_bits;

	HANDLE_BSBITSTREAM stream_for_esc[2];
	int output_stream_no;

	if ((index_scl->er_tvq==1)&&(index_scl->nttDataScl->epFlag==1)) {
	  /* ep_config == 1 */
	  stream_for_esc[0] = output_stream[0];
	  stream_for_esc[1] = output_stream[1];
	  output_stream_no = 2;
	} else {
	  stream_for_esc[0] = output_stream[0];
	  stream_for_esc[1] = output_stream[0];
	  output_stream_no = 1;
	}

	index_scl->last_max_sfb[iscl+1] = index_scl->max_sfb[iscl];

	used_bits=0;
	ntt_maxsfb( index_scl, sfb_width_table, nr_of_sfb, iscl, debugLevel);

	ntt_headerenc( iscl, 
			stream_for_esc[0], index_scl, WS_FHG/*windowShape*/,
			&used_bits,
			NULL/*tnsInfo*/,
			NULL/*nok_lt_status*/, PRED_NONE, debugLevel);
	ntt_available_bits = 
	  index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]-used_bits;
	ntt_scale_vq_coder(tvq_target_spectrum, /*spectral_line_vector,*/
			   lpc_spectrum,
			   index,
			   index_scl,
			   param_ntt,
			   mat_shift,
			   sfb_width_table,
			   nr_of_sfb,
			   ntt_available_bits,
			   reconstructed_spectrum, iscl);

	/* Scalable bit packing */
	tomo_tmp=used_bits;
	tomo_tmp2=BsCurrentBit(stream_for_esc[0]);
        used_bits += ntt_BitPack(index_scl, stream_for_esc, mat_shift, iscl);
	if(debugLevel>2) {
	  fprintf(stderr,"____ ntt_SclLayer bit %d %d [%d]\n",used_bits-tomo_tmp,used_bits,iscl);
	  fprintf(stderr,"<<< mat_Stream[%d](esc0) %ld\n",iscl,BsCurrentBit(stream_for_esc[0])-tomo_tmp2);
	  fprintf(stderr,"<<< mat_Stream[%d](esc1) %ld\n",iscl,BsCurrentBit(stream_for_esc[1]));
	}

        if (used_bits_scl) *used_bits_scl = used_bits;

	return output_stream_no;

}


static
void ntt_maxsfb(    ntt_INDEX   *index,
		    int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
                    int     nr_of_sfb[MAX_TIME_CHANNELS],
                    int     iscl,
		    int     debugLevel
		    )
{
  int max_line, sb, sb_ind, k, i;
  int sfb_offset[MAX_SCFAC_BANDS];
  float ftmp;

  sb_ind=0;
  if (iscl <0) { /*base*/
    if (debugLevel>5)
      fprintf(stderr, "%12.2f %5d DDDD base \n",
		index->nttDataBase->bandUpper, iscl);
    ftmp = (float)(index->nttDataBase->bandUpper);
    if (index->w_type==EIGHT_SHORT_SEQUENCE){
      ftmp *= (float)(index->block_size_samples/8);
      max_line = (int)ftmp; 
    } else {
      ftmp *= (float)(index->block_size_samples);
      max_line = (int)ftmp;
    }
  } else {
    if (debugLevel>5)
      fprintf(stderr, "%12.2f %5d DDDD scl\n",
		index->nttDataScl->ac_top[iscl][0][3], iscl);
    ftmp = (float)(index->nttDataScl->ac_top[iscl][0][3]);
    if (index->w_type==EIGHT_SHORT_SEQUENCE) {
      ftmp *= (float)(index->block_size_samples/8);
      max_line = (int)ftmp; 
    } else {
      ftmp *= (float)(index->block_size_samples);
      max_line = (int)ftmp;
    }
  }   
  sfb_offset[0] = 0;
  k=0;
  for(i = 0; i < nr_of_sfb[MONO_CHAN]; i++ ) {
    sfb_offset[i] = k;
    k +=sfb_width_table[MONO_CHAN][i];
  }
  sfb_offset[i] = k;
  for(sb=0; (sfb_offset[sb]<max_line); sb++){
    sb_ind = sb+1;
  }
  index->max_sfb[iscl+1] = sb_ind;
}

