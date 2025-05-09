/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-11-06

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

	SAMSUNG_2005-09-30 : EncTf_bsac_encode()
										   sam_tf_quantize_spectrum()
											

**********************************************************************/
#include <stdio.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"

#include "tns.h"
#include "tns3.h"                /* structs */

#include "common_m4a.h"
#include "sam_encode.h"


#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "util.h"

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "aac_qc.h"
#include "enc_tf.h"
#include "bitmux.h"
#include "flex_mux.h"

#define  ID_SCE  0x00
#define  ID_CPE  0x01
#define  MAX_PRED_SFB  40

static int num_of_channels;
static int bitrate;
static int sampling_frequency_index;
static int sampling_frequency;
static int frame_length_flag;
static int block_size_samples;
static int  fs_index[16]={
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025,  8000,     0,     0,     0,     0
};

#ifdef __cplusplus
extern "C" {
#endif

static int my_log2(int value);

#ifdef __cplusplus
}
#endif


#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"
#include "bitstream.h"
#include "util.h"

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "aac_qc.h"
#include "util.h"
#include "enc_tf.h"
#include "bitmux.h"
#include "bitstream.h"
#include "flex_mux.h"

/* --- BSAC --- */
#include "sam_encode.h"

static   HANDLE_BSBITBUFFER tmpBitBuf;
static   HANDLE_BSBITBUFFER dynpartBuf;

static   long  samplingRate;
static double pow_quant[1024]; /* SAMSUNG_2005-09-30 */


void sam_FlexMuxEncInit_bsac( 
  HANDLE_BSBITSTREAM headerStream,
  int         numChannel,
  long        samplRate,
  ENC_FRAME_DATA * frameData,
  int numES,
  int mainDebugLevel
)
{
  samplingRate = samplRate;
  /* write object descriptor in case of aac_sys */
/*OK: temporary disabled */

  /* allocate memory */
  tmpBitBuf = BsAllocBuffer(8000);
  dynpartBuf =BsAllocBuffer(6000);

  return;
}


/*****************************************************************************/
/* SAMSUNG_2005-09-30 */
static
int sort_for_grouping(int sfb_offset[],
		      const int sfb_width_table[],
		      double *p_spectrum,
		      int num_window_groups,
		      const int window_group_length[],
		      int nr_of_sfb,
		      double *allowed_dist,
		      const double *psy_allowed_dist,
                      int blockSizeSamples
		      )
{
  int i;
  int index = 0;
  int group_offset=0;
  int k=0;
  int shortBlockLines = blockSizeSamples/NSHORT;

  /* calc org sfb_offset just for shortblock */
  sfb_offset[k]=0;
  for (k=1 ; k <nr_of_sfb+1; k++) {
    sfb_offset[k] = sfb_offset[k-1] + sfb_width_table[k-1];
  }

  /* sort the input spectral coefficients */
#ifdef DEBUG_xx
  for (k=0; k<blockSizeSamples; k++){
    p_spectrum[k] = k;
  }
#endif


#ifdef DEBUG_xx
  ii=0;
  for (i=0;i<num_window_groups;i++){
    fprintf(stderr,"\ngroup%d: " ,i);
    for (k=0; k< shortBlockLines *window_group_length[i]; k++){
      fprintf(stderr," %4.0f" ,tmp[ii++]);
    }
  }
#endif  
 

  /* now calc the new sfb_offset table for the whole p_spectrum vector*/

  index = 0;
  sfb_offset[index++] = 0;	  
  for (i=0; i < num_window_groups; i++) {
    for (k=0 ; k <nr_of_sfb; k++) {
      allowed_dist[k+ i* nr_of_sfb]=psy_allowed_dist[k];
      sfb_offset[index] = sfb_offset[index-1] + sfb_width_table[k]*window_group_length[i] ;
      index++;
    }
  }

  /*  *nr_of_sfb = *nr_of_sfb * num_window_groups; */


  return 0;
}

int sam_tf_quantize_spectrum( 	
  int quant[][NUM_COEFF],
	int scale_factor[][SFB_NUM_MAX],  
  /*QuantInfo   (*_qInfo)[MAX_TIME_CHANNELS],*/
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
	WINDOW_SHAPE windowShape,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],   /* out: grouped sfb offsets */
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
	MSInfo      *msInfo, 
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  int   window_group_length[],
  /*int         aacAllowScalefacs,*/
	int					i_ch
)
  /*QC_MOD_SELECT qc_select,*/
{
  int i=0;
  int j=0;
  int k;
  double max_dct_line;
  int common_scalefac;
  int common_scalefac_save;
  int scale_factor_diff[SFB_NUM_MAX];
  int scale_factor_save[SFB_NUM_MAX];
  double error_energy[SFB_NUM_MAX];
  double linediff;
  double pow_spectrum[NUM_COEFF];
  int sfb_amplify_check[SFB_NUM_MAX];
  double requant[NUM_COEFF];
  int largest_sb, sb;
  double largest_ratio;
  double ratio[SFB_NUM_MAX];
  int max_quant;
  int max_bits;
  int all_amplified;
  int maxRatio;
  int rateLoopCnt;
  double allowed_distortion[SFB_NUM_MAX];
  int  calc_sf[SFB_NUM_MAX];
  int  nr_of_sfs = 0;

  double quantFac;		/* Nokia 971128 / HP 980211 */
  double invQuantFac; 
  int start_com_sf = 40 ;

  /** create the sfb_offset tables **/
  nr_of_sfs = nr_of_sfb * num_window_groups;

  if (windowSequence == EIGHT_SHORT_SEQUENCE) {		
    sort_for_grouping( sfb_offset, sfb_width_table, spectrum,
			num_window_groups, window_group_length,
			nr_of_sfb,
			allowed_distortion, allowed_dist, blockSizeSamples
		       );
  } else if ((windowSequence == ONLY_LONG_SEQUENCE) ||  
             (windowSequence == LONG_START_SEQUENCE) || 
             (windowSequence == LONG_STOP_SEQUENCE)) {
    sfb_offset[0] = 0;
    k=0;
    for( i=0; i<nr_of_sfb; i++ ){
      sfb_offset[i] = k;
      k +=sfb_width_table[i];
      allowed_distortion[i] = allowed_dist[i];
    }
    sfb_offset[i] = k;
  } else {
    CommonExit(-1,"\nERROR : unsupported window type, window type = %d\n",windowSequence);
  } 

  /** find the maximum spectral coefficient **/
  max_dct_line = 0;
  for(i=0; i<sfb_offset[nr_of_sfs]; i++){
    pow_spectrum[i]=(pow(fabs(spectrum[i]), 0.75));
    if (fabs(spectrum[i]) > max_dct_line){
      max_dct_line = fabs(spectrum[i]);
    }
  }

  if (max_dct_line > 1)
    common_scalefac = start_com_sf + (int)ceil(16./3. * (log(pow(max_dct_line,0.75)/MAX_QUANT)/log(2.0)));
  else
    common_scalefac = 0;

  if ((common_scalefac>200) || (common_scalefac<0) )
    common_scalefac = 20;

  /* initialize the scale_factors */
  for(k=0; k<nr_of_sfs;k++){
    sfb_amplify_check[k] = 0;
    calc_sf[k]=1;
    scale_factor_diff[k] = 0;
    ratio[k] = 0.0;
  }

  maxRatio    = 1;
  largest_sb  = 0;
  rateLoopCnt = 0;
  do {  /* rate loop */
    do {  /* distortion loop */
      max_quant = 0;
      largest_ratio = 0;
      for (sb=0; sb<nr_of_sfs; sb++){
        sfb_amplify_check[sb] = 0;
        if (calc_sf[sb]){
          calc_sf[sb] = 0;
          quantFac = pow(2.0, 0.1875*(scale_factor_diff[sb] - common_scalefac));
          invQuantFac = pow(2.0, -0.25 * (scale_factor_diff[sb] - common_scalefac));
          error_energy[sb] = 0.0;

          for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){            
						quant[i_ch][i] = (int)(pow_spectrum[i] * quantFac  + MAGIC_NUMBER);            
						if (quant[i_ch][i]>MAX_QUANT)
              CommonExit(-1,"\nMaxquant exceeded");
            requant[i] =  pow_quant[quant[i_ch][i]] * invQuantFac; 
            quant[i_ch][i] = sgn(spectrum[i]) * quant[i_ch][i];  /* restore the original sign */
            /* requant[][] = pow(quant[ch][i], (1.333333333)) * invQuantFac ; */

            /* measure the distortion in each scalefactor band */
            /* error_energy[sb] += pow((spectrum[ch][i] - requant), 2.0); */
            linediff = (fabs(spectrum[i]) - requant[i]);
            error_energy[sb] += linediff*linediff;
          } /* --- for (i=sfb_offset[sb] --- */

          scale_factor_save[sb] = scale_factor_diff[sb];

          if ( ( allowed_distortion[sb] != 0) && (energy[sb] > allowed_distortion[sb]) ){
            ratio[sb] = error_energy[sb] / allowed_distortion[sb];
          } else {
            ratio[sb] = 1e-15;
            sfb_amplify_check[sb] = 1;
          }

          /* find the scalefactor band with the largest error ratio */
          /*if ((ratio[sb] > maxRatio) && (scale_factor_diff[sb]<60) && aacAllowScalefacs ) {*/
					if ((ratio[sb] > maxRatio) && (scale_factor_diff[sb]<60)) {
            scale_factor_diff[sb]++;
            sfb_amplify_check[sb] = 1;
            calc_sf[sb]=1;
          } else {
            calc_sf[sb]=0;
          }

          if ( (ratio[sb] > largest_ratio)&& (scale_factor_diff[sb]<60) ){
            largest_ratio = ratio[sb];
          }
        }
      } /* ---  for (sb=0; --- */

      /* amplify the scalefactor of the worst scalefactor band */
      /* check to see if all the sfb's have been amplified.*/
      /* if they have been all amplified, then all_amplified remains at 1 and we're done iterating */
      all_amplified = 1;
      for(j=0; j<nr_of_sfs-1;j++){
        if (sfb_amplify_check[j] == 0 )
          all_amplified = 0;
      }
    } while ((largest_ratio > maxRatio) && (all_amplified == 0)  ); 

    for (i=0; i<nr_of_sfs; i++){
      scale_factor_diff[i] = scale_factor_save[i];
    }

    /* offset the difference of common_scalefac and scalefactors by SF_OFFSET */
    for (i=0; i<nr_of_sfs; i++){
      scale_factor[i_ch][i] = common_scalefac - scale_factor_diff[i] + SF_OFFSET;

      /* scalefactors have to be greater than 0 and smaller than 256 */
      if (scale_factor[i_ch][i]>=(2*TEXP)) {
        /*fprintf(stderr,"scalefactor of %i truncated to %i\n",qInfo->scale_factor[i],(2*TEXP)-1);*/
        scale_factor[i_ch][i]=(2*TEXP)-1;
      }
      if (scale_factor[i_ch][i]<0) {
        /*fprintf(stderr,"scalefactor of %i truncated to 0\n",qInfo->scale_factor[i]);*/
        scale_factor[i_ch][i]=0;
      }
    }

/*OK: temporary disabled */

    /*if(obj_type == TF_BSAC)*/ 
		{

     for(k=0;k<nr_of_sfs;k++) {  
        calc_sf[k]=1;
      }

      /* calculate the amount of bits needed for the spectral values using BSAC */
/* 20070530 SBR */
	max_bits = sam_encode_frame(
		1,
		0/*ch_type*/,
		windowSequence,
		windowShape,
		sfb_offset,
		nr_of_sfs,
		num_window_groups,
		window_group_length,
		quant, scale_factor,
		msInfo,
		(HANDLE_BSBITSTREAM)NULL/*fixed_stream*/,
		i_ch,
		0,
		0/*mc_ext_size*/,
		0/*mc_enabled*/,
		0,/*sbrenable*/
		NULL,
		0,/*numEn*/
		0 /*i_el*/
	);
    } 

    /*     fprintf(stderr,"\n^^^ [bits to transmit = %d] global=%d spectral=%d sf=%d book=%d extra=%d", */
    /* 	   max_bits,common_scalefac,spectral_bits,\ */
    /* 	   sf_bits, book_bits, extra_bits); */
    common_scalefac_save = common_scalefac;
    if (all_amplified){ 
      maxRatio = maxRatio*2;
      common_scalefac += 1;      
    }
      
    common_scalefac += 1;
    if ( common_scalefac > 200 ) { 
      CommonExit(-1,"\nERROR in loops : common_scalefac is %d",common_scalefac );
    }

    rateLoopCnt++;
    /* --- TMP --- */
    /* byte align ! 
       printf("MAX bits %d to byte align=%d\n", max_bits, (8-(max_bits%8))%8); 
       max_bits += ((8-(max_bits%8))%8); 
    */
  } while (max_bits > available_bits);

  common_scalefac = common_scalefac_save;

/*  if ( qdebug > 0 ) {
    fprintf(stderr,"\n*** [bits transmitted = %d] largestRatio=%4.1f loopCnt=%d comm_sf=%d",
	    max_bits, 10*log10(largest_ratio+1e-15), rateLoopCnt, common_scalefac );
  }

  if ( qdebug > 0 ) {
    for (i=0; i<nr_of_sfs; i++){
      if ((error_energy[i]>energy[i]))
        CommonWarning("\nerror_energy greater than energy in sfb: %d ", i);
    }
  }*/

  /* write the reconstructed spectrum to the output for use with prediction */
  {
    int i;

    for (sb=0; sb<nr_of_sfs; sb++){
      /*if ( qdebug > 4 ) {
        fprintf(stderr,"\nrequantval band %d : ",sb);
      }*/
      for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
        p_reconstructed_spectrum[i] = sgn(quant[i_ch][i]) * requant[i]; 
        /*if ( qdebug > 4 ) {
          fprintf(stderr,"%7.0f",(float)p_reconstructed_spectrum[i]);
        }*/
      }
    }
  }

  return max_bits;
}

/* 20070530 SBR */
#include "ct_sbr.h"
int EncTf_bsac_encode
(
 HANDLE_CHANNEL_ELEMENT_DATA chData, 
 int		 numElm, 
 double      *spectral_line_vector[MAX_TIME_CHANNELS],
 double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
 double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
 double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
 int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
 int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
 int         nr_of_sfb[MAX_TIME_CHANNELS],
 int         bits_available,  
 int					frameNumBit,
 HANDLE_BSBITSTREAM *output_au,  
 WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
 WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
 WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS],
 /*int         aacAllowScalefacs,*/
 int         blockSizeSamples,
 int         nr_of_chan,
 long        samplRate,
 MSInfo      *msInfo,
 TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],  
 int         pns_sfb_start,    
 int         num_window_groups,
 int         window_group_length[8],
 int         bw_limit,
 int         debugLevel,	
/* 20070530 SBR */
 int							lfePresent,
 int							sbrenable,
 SBR_CHAN						*sbrChan
 )
{
	int i_ch, i;
	int used_bits = 0;
	int bits_written = 0;

	PnsInfo pnsInfo[MAX_TIME_CHANNELS];
	/*QuantInfo qInfo[MAX_TIME_CHANNELS];*/
	int scale_factor[MAX_TIME_CHANNELS][SFB_NUM_MAX];  
	int quant[MAX_TIME_CHANNELS][NUM_COEFF];	
	int grouped_sfb_offsets[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];
	
	int chNum = nr_of_chan;
	MSInfo      *pMsInfo = msInfo;
	int startChIdx = 0;
	int endChIdx = 1;
	int ch_bits_available;
	int ch_bits_available_bak; /*  to prevent too large frame size*/
	int lfeBits;
	int available_bitreservoir_bits = bits_available - frameNumBit;
	int ext_flag = 0;
	int mc_enabled = 0;
	int ch_type = 0;
	
	int i_elm;	
	float bitCont, inv_n_chan;
	int aval_bit;


	if(numElm>1) mc_enabled = 1;

	
	/*LEE*/
	if( (samplRate == 8000) && (nr_of_chan >= 8) ) {
		bitCont = 0.4f;
	}
	else if( (samplRate < 16000) && (nr_of_chan >= 8) ) {
		bitCont = 0.6f;
	}
	else if( (samplRate == 8000) && (nr_of_chan >= 5) ) {
		bitCont = 0.6f;
	}
	else {
		bitCont = 0.8f;
	}
	
	
	if( !lfePresent )
	{
		inv_n_chan = 1.f / nr_of_chan;
		ch_bits_available = (int)(frameNumBit * inv_n_chan);
		ch_bits_available += (int)(0.2 * available_bitreservoir_bits * inv_n_chan );
		
		/*Samsung.2006.10.*/
		if(ch_bits_available * nr_of_chan > 8.0 * 2048.0 * 0.98) { /*SAMSUNG updated 20061213*/
			ch_bits_available = (int)((frameNumBit * inv_n_chan) * bitCont);
		}

	}
	else
	{
		float lfeBitRatio;
		int reser;
		int temp1;


		/* Set LFE bits */
		inv_n_chan = 1.f / (nr_of_chan - 1);
		lfeBitRatio = 0.14f;													/* ratio of LFE bits to bits of one SCE */
		reser = (int)(0.2f * available_bitreservoir_bits * inv_n_chan);


		lfeBits = max( 200, (int)(frameNumBit * inv_n_chan * lfeBitRatio ) );	/* number of bits for LFE */
		temp1 = (int)(reser * lfeBitRatio );
		lfeBits += temp1;
		
		/* Set channel bits */
		ch_bits_available = (int)( (frameNumBit - lfeBits) * inv_n_chan );
		ch_bits_available += ( reser - temp1 );

		/*Samsung.2006.10.*/
		if( ch_bits_available*(nr_of_chan-1) + lfeBits > 8.0 * 2048 * 0.98 ) {
			ch_bits_available = (int)((frameNumBit / nr_of_chan) * bitCont);
		}

	}
	ch_bits_available_bak = ch_bits_available;

	for(i_elm = numElm-1; i_elm>=0; i_elm--)
	{
		
		HANDLE_CHANNEL_ELEMENT_DATA pChData = &chData[i_elm];
		ext_flag = pChData->extension;
		chNum = pChData->chNum;
		pMsInfo = &pChData->msInfo;
		startChIdx = pChData->startChIdx;
		endChIdx = pChData->endChIdx;
		ch_type =  pChData->ch_type;

		ch_bits_available = ch_bits_available_bak * chNum; 
		used_bits = 0;

/* 20070530 SBR */
		if(sbrenable) {
			used_bits += sbrChan[startChIdx].totalBits;
		}
  
		/* ---- process and quantize the spectrum ---- */

		if (chNum==2) {
			if (debugLevel>1)
				fprintf(stderr,"EncTfFrame: MS\n");

			MSApply(nr_of_sfb[MONO_CHAN],
							sfb_offset[MONO_CHAN],
							pMsInfo->ms_used,
							(windowSequence[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)?NSHORT:1,
							blockSizeSamples,
							spectral_line_vector[startChIdx],
							spectral_line_vector[endChIdx]);
		}


		for (i_ch=0; i_ch<chNum; i_ch++) {

			double temp_spectrum[NUM_COEFF];

			if (debugLevel>3)
				fprintf(stderr,"EncTfFrame: apply bandwidth limitation\n");
			if (debugLevel>5)
				fprintf(stderr,"            bandwidth limit %i Hz\n", bw_limit);

			/*Set upper spectral coefficients to zero for LFE*/
			if(ch_type == LFE)
			{
				int indStart;

				indStart = sfb_offset[MONO_CHAN][nr_of_sfb[MONO_CHAN]] * 500 / samplRate;
				for(i=indStart; i<sfb_offset[MONO_CHAN][nr_of_sfb[MONO_CHAN]]; i++) {
					spectral_line_vector[startChIdx+i_ch][i] = 0;
				}

			}


			bandwidth_limit_spectrum(spectral_line_vector[startChIdx+i_ch],
															 temp_spectrum,														 
															 windowSequence[startChIdx+i_ch],
															 blockSizeSamples,
															 samplRate, 
															 bw_limit);

			if ((debugLevel>3)&&(pns_sfb_start>=0))
				fprintf(stderr,"EncTfFrame: apply PNS (from sfb %i)\n", pns_sfb_start);

			tf_apply_pns(
				temp_spectrum,								 
				energy[startChIdx+i_ch],
				sfb_offset[startChIdx+i_ch],
				nr_of_sfb[startChIdx+i_ch],
				pns_sfb_start,
				windowSequence[startChIdx+i_ch],
				&pnsInfo[startChIdx+i_ch]
			);

			if (debugLevel>3)
				fprintf(stderr,"EncTfFrame: quantize\n");   


			/* Fixing 8channel LFE allocation bit*/
			if(ch_type == LFE) {
				aval_bit = (ch_bits_available-used_bits)/(chNum-i_ch)/2;
			}
			else {
				aval_bit = (ch_bits_available-used_bits)/(chNum-i_ch);
			}

			
			used_bits += sam_tf_quantize_spectrum(/*&qInfo,*/
				&quant[startChIdx],
				&scale_factor[startChIdx],
				temp_spectrum,
				reconstructed_spectrum[startChIdx+i_ch],
				energy[startChIdx+i_ch],
				allowed_dist[startChIdx+i_ch],
				windowSequence[startChIdx+i_ch],
				windowShape[startChIdx+i_ch],
				sfb_width_table[startChIdx+i_ch],
				grouped_sfb_offsets[startChIdx+i_ch],
				nr_of_sfb[startChIdx+i_ch],
				&pnsInfo[startChIdx+i_ch],
				pMsInfo, 
				aval_bit,
				blockSizeSamples,
				num_window_groups,
				window_group_length,
				i_ch
			);


		} /* End of "for (i_ch=0; i_ch<chNum; i_ch++) {" */

		
		bits_written += sam_encode_frame(
			chNum,
			ch_type,
			windowSequence[startChIdx],
			windowShape[startChIdx],
			sfb_offset[startChIdx],
			nr_of_sfb[startChIdx]* num_window_groups,
			num_window_groups,
			window_group_length,
			&quant[startChIdx],
			&scale_factor[startChIdx],
			pMsInfo,
			(HANDLE_BSBITSTREAM)output_au[0]/*fixed_stream*/,
			0,
			0,
			ext_flag ? 0 : bits_written/*mc_ext_size*/,
			mc_enabled,
			sbrenable,
			&sbrChan[startChIdx],
			numElm-1,
			i_elm
		);


	} /* End of "for(i_elm = numElm-1; i_elm>=0; i_elm--)" */



	return bits_written;


}

/* ~SAMSUNG_2005-09-30 */

/*****************************************************************************/

int sam_FlexMuxEncode_bsac(
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   WINDOW_SEQUENCE  windowSequence[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         available_bitreservoir_bits,
			   int         padding_limit,
			   HANDLE_BSBITSTREAM fixed_stream,
			   HANDLE_BSBITSTREAM var_stream,
			   HANDLE_BSBITBUFFER *gcBitBuf,	/* bit buffer for gain_control_data() */
			   int         nr_of_chan,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int         useShortWindows,
			   WINDOW_SHAPE     window_shape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
			   int aacAllowScalefacs,
               long        sampling_rate,
			   QC_MOD_SELECT qc_select,
			   PRED_TYPE predictor_type,
			   NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
               int blockSizeSamples,
               int num_window_groups,
               int window_group_length[],
               TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
               int bandwidth,
               int numES
) 
{

  int lay;

  HANDLE_BSBITSTREAM layerStream;
  int ubits=0;
  int frameBits = average_block_bits + available_bitreservoir_bits - 150 ;
  int flexmuxOverhead =  3*8*(1 + (frameBits/2040)) * numES;
  int read_bits, layer_bits;
  int total_bits = 0;


  frameBits -=   flexmuxOverhead;

  /* start of bitstream composition */
  layerStream = BsOpenBufferWrite(dynpartBuf);
/*OK:temporary disabled*/
  BsClose(layerStream);

  /* make bsac_lstep_stream() from dynpartBuf as many as numES and
     pack the bsac_lstep_stream(s) to flexMux packet(s) and
     write to bitstream */

  layerStream = BsOpenBufferRead(dynpartBuf);
  BsManipulateSetNumByte( ubits/8, layerStream);

  /* Base Layer ES */
  BsBufferManipulateSetNumBit(0,tmpBitBuf);
  layer_bits = (int)(blockSizeSamples * 16000. * nr_of_chan / samplingRate);
  layer_bits = (layer_bits + 7) / 8 * 8;

  ubits = ubits - 8 * numES; /* used bits - total minimum payload */

  read_bits = (ubits > layer_bits) ? layer_bits : ubits;
  total_bits = read_bits;
  
  read_bits += 8; /* minimum payload */
  BsGetBuffer(layerStream, tmpBitBuf, read_bits);
/*OK: disabled! 
  writeFlexMuxPDU(0,fixed_stream,tmpBitBuf);
*/

  /* Enhancement Layers ES */
  if (numES > 1) {
    layer_bits = (ubits-layer_bits)/(numES-1);
    layer_bits = (layer_bits + 7) / 8 * 8;
  }
  for(lay=1; lay<numES; lay++) {
    BsBufferManipulateSetNumBit(0,tmpBitBuf);
    if (total_bits >= ubits) {
        read_bits = 0;
    }
    else if (lay == (numES-1)) {
       read_bits = ubits - total_bits;
       total_bits = ubits;
    }
    else {
      read_bits = layer_bits;
      total_bits += layer_bits;
      if (total_bits > ubits) {
        read_bits = total_bits - ubits;
        total_bits = ubits;
      }
    } 

    read_bits += 8; /* minimum payload */
    BsGetBuffer(layerStream, tmpBitBuf, read_bits);
/*OK disabled
    writeFlexMuxPDU(lay,fixed_stream,tmpBitBuf);
*/
  }
  BsClose(layerStream);

  return(BsCurrentBit(fixed_stream));
}


void sam_init_encode_bsac(
    int numChannel,
    int sampling_rate,
    int bit_rate,
    int frame_size
)
{
  int  i;

  num_of_channels = numChannel;
  block_size_samples = frame_size;
  if(frame_size == 1024)
    frame_length_flag = 0;
  else
    frame_length_flag = 1;
  for(i = 0; i < 12; i++) if(sampling_rate == fs_index[i]) {
    sampling_frequency_index = i;
    break;
  }
  sampling_frequency = sampling_rate;

   /* bitrate error debugging */
  bitrate = bit_rate/num_of_channels;

  /* bitrate error debugging */
  if( (num_of_channels == 8) || (num_of_channels == 6)) {

	if( sampling_frequency > 16000 )
		bitrate = 64000;
	else
		bitrate = 40000;

  }
  else {
	bitrate = bit_rate/num_of_channels;
  }
 
  if(bitrate > 64000) {
    printf("BSAC: Maximum bitrate 64kbps/ch!\n");
    bitrate = 64000;
  }

  sam_scale_bits_init_enc(sampling_rate, block_size_samples);
/* SAMSUNG_2005-09-30 */
	for (i=0;i<1024;i++){ 
    pow_quant[i]=pow((double)i, ((double)4.0/(double)3.0)); 
  }

}

int startf=1;
int sam_encode_frame
(
 int num_of_chan,
 int ch_type, /* SAMSUNG_2005-09-30 */
 WINDOW_SEQUENCE windowSequence,
 WINDOW_SHAPE  window_shape,
 int sfb_offset[],
 int nr_of_sfs,
 int num_window_groups,
 int window_group_length[],
 int quant[][1024],
 int scfacs[][MAX_SCFAC_BANDS],
 MSInfo      *msInfo, /* SAMSUNG_2005-09-30 */
 HANDLE_BSBITSTREAM fixed_stream,
 int i_ch,
 /*int w_flag,*/
 int avr_bits,
 int mc_ext_size, /* SAMSUNG_2005-09-30 : extension parts length  */
 int mc_enabled, /* SAMSUNG_2005-09-30 */
 int sbrenable,
 SBR_CHAN *sbrChan,
 int numElm,
 int i_el
)
{
	int i=0, k, ch, w, g, s;
	int b = 0;
	int sfb, maxSfb;
	int frameSize;
	int abits;
	int nch;
	int cband;
	int nr_of_sfb = 0;
	int end_cband[8];
	int stereo_mode;
	/*int stereo_info[MAX_SCFAC_BANDS];*/
	int stereo_info[8][MAX_SCFAC_BANDS]; /* SAMSUNG_2005-09-30 */
	int ModelIndex[2][8][32];
	int scalefactors[2][MAX_SCFAC_BANDS];
	int scf[2][8][MAX_SCFAC_BANDS];
	int swb_offset[8][MAX_SCFAC_BANDS];
	int sample_buf[2][1024];
	int *sample[2][8];
	int w_flag = (fixed_stream)? 1 : 0; /* SAMSUNG_2005-09-30 */
	int bitRate;
	
	int bitCount;




	/*nch = i_ch + 1;*/
	nch = i_ch + num_of_chan; /* SAMSUNG_2005-09-30 */
	
	/* initialize */
	stereo_mode = msInfo->ms_mask;  /* SAMSUNG_2005-09-30 */
	
	
	/* SAMSUNG_2005-09-30 */
	for(sfb = 0; sfb < MAX_SCFAC_BANDS; sfb++) {
		for(ch = i_ch; ch < nch; ch++) {		
			scalefactors[ch][sfb] = scfacs[ch][sfb];
		}		
	}
	nr_of_sfb = nr_of_sfs / num_window_groups;
	
	for(g = 0; g < num_window_groups; g++) {
		for(sfb = 0; sfb < nr_of_sfb; sfb++) {
			stereo_info[g][sfb] = msInfo->ms_used[g][sfb];
		}
	}
	
	
	if(ch_type == LFE) {
		if( num_window_groups == 8 ) {
			maxSfb = 2;
		}
		else {
			maxSfb = 5;
		}
		bitRate = (int)(bitrate * 0.5);
	}
	else {
		maxSfb = nr_of_sfs / num_window_groups -3;/*SAMSUNG updated 20061213*/
		bitRate = bitrate;
	}
	
	
	for(ch = i_ch; ch < nch; ch++) {
		
		if(windowSequence == EIGHT_SHORT_SEQUENCE) {
			int short_block_size=block_size_samples/8;
			
			s = 0;
			/*nr_of_sfb = nr_of_sfs / num_window_groups;*/
			for(g = 0; g < num_window_groups; g++) { 
				sample[ch][g] = &(sample_buf[ch][short_block_size * s]);
				s += window_group_length[g];
				
				swb_offset[g][0] = 0;
				for(sfb = 0; sfb < nr_of_sfb; sfb++)
					swb_offset[g][sfb+1] = sfb_offset[sfb+1] * window_group_length[g];
				end_cband[g] = (swb_offset[g][maxSfb]+31)/32;
			}
			
			/* reshuffling */
			s = 0;
			for(g = 0; g < num_window_groups; g++) {
				for (i = 0; i < short_block_size; i+=4) {
					int offset = (128 * s) + (i * window_group_length[g]);          
					for(w=0; w<window_group_length[g]; w++, b++)  
						for (k = 0; k < 4; k++) 
							sample_buf[ch][offset+4*w+k] = quant[ch][short_block_size*(w+s)+i+k];
				}
				s += window_group_length[g];
			}
			
		} else {
			
			nr_of_sfb = nr_of_sfs;
			num_window_groups = 1;
			window_group_length[0] = 1;
			swb_offset[0][0] = 0;
			for(sfb = 0; sfb < nr_of_sfb; sfb++)
				swb_offset[0][sfb+1] = sfb_offset[sfb+1];
			
			sample[ch][0] = &(quant[ch][0]);
			
			end_cband[0] = (swb_offset[0][maxSfb]+31)/32;
		}
		
		for (g = 0; g < num_window_groups; g++) 
			for (cband = 0; cband < 32; cband++) 
				ModelIndex[ch][g][cband] = 0;
			
			for(g = 0; g < num_window_groups; g++) {
				int bal, max;
				
				for (cband = 0; cband < end_cband[g]; cband++) {
					max = 0;
					for (i = cband*32; i < (cband+1)*32; i++) {
						if (abs(sample[ch][g][i])>max) 
							max = abs(sample[ch][g][i]);
					}
					if(max > 0) bal = my_log2(max);
					else    bal = 0;
					if(bal >= 8)
						ModelIndex[ch][g][cband] = bal + 7;
					else
						ModelIndex[ch][g][cband] = bal * 2  - 1;
					if (ModelIndex[ch][g][cband] < 0) 
						ModelIndex[ch][g][cband] = 0; 
				}
			}
			
			for (g = 0; g < num_window_groups; g++) {
				for (sfb = 0; sfb < maxSfb; sfb++) {
					scf[ch][g][sfb] = scalefactors[ch][(g*nr_of_sfb)+sfb];
				}
			}
	}
	
	if(avr_bits == 0)
		abits = 10000;
	else
		abits = avr_bits - 8;
	
	/********** B I T S T R E A M   M A K I N G *********/
	if(w_flag) {

		frameSize = sam_encode_bsac(
			bitRate,
			ch_type,
			windowSequence,
			window_shape,
			sample,
			scf, maxSfb,
			num_window_groups,
			window_group_length,
			swb_offset,
			nr_of_sfb,
			ModelIndex,
			stereo_mode,
			stereo_info,
			abits+mc_ext_size,
			i_ch,
			nch,
			mc_enabled,
			sbrenable,
			sbrChan,
			numElm,
			i_el,
			&bitCount
		);
		
		abits = frameSize;
		
	}
	
	frameSize = sam_encode_bsac(
		bitRate,
		ch_type,
		windowSequence,
		window_shape,
		sample,
		scf, maxSfb,
		num_window_groups,
		window_group_length,
		swb_offset,
		nr_of_sfb,      
		ModelIndex,
		stereo_mode,
		stereo_info,
		abits+mc_ext_size,
		i_ch,
		nch,
		mc_enabled,
		sbrenable,
		sbrChan,
		numElm,
		i_el,
		&bitCount
	);
	
	/* 20060705 */
	if(w_flag) {

		if(mc_enabled)
		{

			/* Bitpack frame size */
			if( i_el == 0 )	{

				sam_frame_length_rewrite2bs( bitCount+mc_ext_size, 0, 11 );

			}
			else {

				sam_frame_length_rewrite2bs( frameSize, 0, 11 );
				
				/* Adding length of extention type bits to BSAC total length */
				frameSize += 8;

			}
			
		}
		else
		{
			if((frameSize % 8)) {
				frameSize += sam_putbits2bs(0x00, (8 - (frameSize % 8)));
			}
			sam_frame_length_rewrite2bs(frameSize, 0, 11);
		}


		if(!mc_enabled) {
			sam_bsflush(fixed_stream, frameSize);/*SAMSUNG updated 20061213*/
		}
		else {
			if( i_el == 0 )
				sam_bsflushAll(fixed_stream, frameSize+mc_ext_size);/* SAMSUNG_2005-09-30 : flush all of channel bistream */
		}
		
	}
	else {

		if(frameSize % 8) 
			frameSize += (8 - (frameSize % 8));

	}
	
	return bitCount;

}

static int my_log2(int value)
{
  int  i, step;

  if(value < 0) {
    fprintf(stderr, "BSC:my_log2:error : %d\n", value);
    return 0;
  }
  if(value == 0)  return 0;

  step = 2;
  for(i = 1; i < 24; i++) {
    if(value < step) return i;
    step *= 2;
  }
  return ( 1 + (int)(log10((double)value)/log10(2.0)));
}

