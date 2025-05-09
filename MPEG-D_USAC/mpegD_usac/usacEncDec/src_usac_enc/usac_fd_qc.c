/************************************************************************************

This software module was originally developed by

Dolby Laboratories

and edited by

Takashi Koike (Sony Corporation)
Mikko Suonio (Nokia)
Olaf Kaehler (Fraunhofer IIS-A)
Guillaume Fuchs (Fraunhofer IIS)
Pierrick Philippe (Orange Labs)

in the course of development of the MPEG-2 AAC/MPEG-4
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools as
specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2AAC/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 AAC/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 AAC/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. Copyright 1996.

***********************************************************************************/


#include <stdio.h> 
#include <math.h>
#include <float.h>

#include "all.h"
#include "allHandles.h"
#include "block.h"

#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"
#include "bitstream.h"
#include "tf_main.h"
#include "bitmux.h"

#include "usac_fd_qc.h"
#include "usac_bitmux.h"

#include "usac_nf.h"

#ifdef USE_RUNTIME_CODEBOOK_FILE
static int table_choice = 5;      /* CHOOSE HERE BETWEEN TABLES 4 or 5 */
static int huffman_code_table[13][1090][4];
#else
#include "rm5hufbook.h"
#endif
static int qdebug;
static double pow_quant[9000];

static
int sort_for_grouping(int sfb_offset[],
                      const int sfb_with_table[],
                      double *p_spectrum,
                      int num_window_groups,
                      const int window_group_length[],
                      int nr_of_sfb,
                      double *allowed_dist,
                      const double *psy_allowed_dist,
                      int blockSizeSamples
);

static void degroup (const int grouped_sfb_offsets[MAX_SCFAC_BANDS+1],
		     int sfb_per_group,
		     double *rec_spectrum,
		     int num_window_groups,
		     const int window_group_length[],
		     int blockSizeSamples);
static void degroup_int(const int grouped_sfb_offsets[MAX_SCFAC_BANDS+1],
		     int sfb_per_group,
		     int *rec_spectrum,
		     int num_window_groups,
		     const int window_group_length[],
			int blockSizeSamples);


/* 
   quantizes each scalefactor band separately, in a very SLOW algorithm.  
   But, it does work, by incrementing (by one) the scalefactor in the corresponding
   scalefactor band with the largest ratio of error energy to allowed distortion.
   There are both rate and distortion loops currently implemented.
   Again, this algorithm has NOT been modified for speed.  With more time, the
   rate/distortion loops can be sped up.

   --SNL 11/15/96
   */

/*
  tf_encode_spectrum now correctly outputs the amount of bits written to the bitstream,
  along with accurately keeping a bit count in the quantizer loop.

  --SNL 11/8/96
*/

/*
   This code sucessfully implements grouping.  To test different versions of
   grouping, change the following variables near the top of tf_encode_spectrum_nbc():
   window_group_length[8] and num_window_groups.  An example of this grouping
   could be: (currently used)

      int window_group_length[8] = {3,1,2,1,1};
      num_window_groups = 5;

   This translates to there being five groups of short windows.  The first group
   contains three short windows, the second window contains one, etc.

November 1, 1996
*/


/*

   This quantizer / noiseless coder is a work in progress.  Currently,
   it supports only long or only short block modes.  The quantizer is
   a uniform quantizer that implements only the rate quantization
   loop, and not yet the distortion quantization loop.  Grouping is
   not yet supported, so during short blocks, the default
   implementation is each of the eight short blocks are individual
   groups.  The noiseless coding section is fully operational.  The
   output of this code is a AAC, CD compliant bitstream.

   This code also allows you to switch between two different huffman
   codebooks,  which I'll call RM4 and CD codebooks, received both
   from AT&T on different dates.  If table_choice == 4, then you
   choose to use the all signed, RM4 codebooks.  If table_choice == 5,
   then the CD codebooks will be loaded instead.  Both of these
   codebooks have been tested sucessfully with their respective
   decoders.


     September 24, 1996

*/

/*
   RM4 has tables with the following characteristics:  (they're all signed tables) --->
   book        signed/unsigned   range   values_in_codebook  dimension
    1            signed           +/- 1          81             4
    2            signed           +/- 1          81             4
    3            signed           +/- 2          625            4
    4            signed           +/- 2          625            4
    5            signed           +/- 4          81             2
    6            signed           +/- 4          81             2
    7            signed           +/- 7          225            2
    8            signed           +/- 7          225            2
    9            signed           +/- 12         625            2
    10           signed           +/- 12         625            2
    11           signed           +/- 16         1089           2


   RM5 has tables with the following characteristics:  (they're signed and unsigned) --->
   book        signed/unsigned   range   values_in_codebook  dimension
    1            signed           +/- 1          81             4
    2            signed           +/- 1          81             4
    3          unsigned           0,1,2          81             4
    4          unsigned           0,1,2          81             4
    5            signed           +/- 4          81             2
    6            signed           +/- 4          81             2
    7          unsigned           0...7          64             2
    8          unsigned           0...7          64             2
    9          unsigned           0...12         169            2
    10         unsigned           0...12         169            2
    11         unsigned           0...16         289            2
*/

void usac_init_quantize_spectrum(UsacQuantInfo *qInfo, int debugLevel)
{
  unsigned int   i,j,k;

#ifdef USE_RUNTIME_CODEBOOK_FILE
  FILE *fpout =NULL;
  short  tmpc[4];
  int *huff_ptr= &(huffman_code_table[0][0][0]);

  if(qc_select != AAC_BSAC && qc_select != AAC_SYS_BSAC) {	/* YB : 971106 */

  /*
    This function reads either one of the two set of huffman codebook tables into huff[][][].
  */

  if (table_choice == 5) {
    /* read the data in from file (used scan_huff_att3.c to read in rm5huf.txt from ascii)*/
    if ((fpout = fopen("../tables_enc/rm5hufbook","rb")) == NULL){
      if ((fpout = fopen("./tables_enc/rm5hufbook","rb")) == NULL){
        CommonExit(-1,"\ncannot open rm5hufbook");
      }
    }
  }

  if ((table_choice != 4) && (table_choice != 5)){
    CommonExit(-1,"\nInvalid value of table_choice=%d, please choose either 4 or 5\n",table_choice);
  }

  for (j=0;j<sizeof(huffman_code_table)/sizeof(huffman_code_table[0][0][0]) ;j++){
    tmpc[3] = getc(fpout);
    tmpc[2] = getc(fpout);
    tmpc[1] = getc(fpout);
    tmpc[0] = getc(fpout);
    *huff_ptr++ = (tmpc[0]<<0)+(tmpc[1]<<8)+(tmpc[2]<<16)+(tmpc[3]<<24);
  }
  /*   THIS IS NOT PROTABLE !!: */
  /*   i = fread(huff,sizeof(int),13*1090*4,fpout); */

  fprintf(stderr,"\nread %d huffman codebook values \n",j);
  }
#endif /*USE_RUNTIME_CODEBOOK_FILE*/

  qdebug = debugLevel;

  qInfo->reset=1;
  for(i=0;i<13;i++){
    for (j=0;j<1090;j++){
      for (k=0;k<4;k++){
	qInfo->huffTable[i][j][k] = huffman_code_table[i][j][k];
      }
    }
  }

  for (j=0;j<9000;j++){
    pow_quant[j]=pow((double)j, ((double)4.0/(double)3.0));
  }
}

static void debugPrint(
		   int nr_of_sfb,
		   double *ratio,
		   double *energy,
		   double *allowed_dist,
		   double *error_energy,
		   int *scale_factor,
		   int common_scalefac)
{
  int sb;
  fprintf(stderr,"\nratios    :");
  for (sb=0; sb<nr_of_sfb; sb++){
    fprintf(stderr,"%6.1f|",10*log10(ratio[sb]+1e-15));
  }
  fprintf(stderr,"\nenrgy     :");
  for (sb=0; sb<nr_of_sfb; sb++){
    fprintf(stderr,"%6.1f|",10*log10(energy[sb]+1e-15));
  }
  fprintf(stderr,"\nmask enrgy:");
  for (sb=0; sb<nr_of_sfb; sb++){
    fprintf(stderr,"%6.1f|",10*log10(allowed_dist[sb]+1e-15));
  }
  fprintf(stderr,"\nerro enrgy:");
  for (sb=0; sb<nr_of_sfb; sb++){
    fprintf(stderr,"%6.1f|",10*log10(error_energy[sb]+1e-15));
  }
  fprintf(stderr,"\nscalefacto:");
  for (sb=0; sb<nr_of_sfb; sb++){
    fprintf(stderr,"%6i|",(common_scalefac + SF_OFFSET -scale_factor[sb]));
  }

}


int usac_quantize_spectrum(
  UsacQuantInfo   *qInfo,
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE windowShape,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],   /* out: grouped sfb offsets */
  int         max_sfb,
  int         nr_of_sfb,
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  const int   window_group_length[],
  int         aacAllowScalefacs,
  UsacToolsInfo *tool_data,
  TNS_INFO      *tnsInfo,
  int common_window,
  int common_tw,
  int flag_twMdct,
  int flag_noiseFilling, 
  int bUsacIndependencyFlag
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
  } else if ((windowSequence == ONLY_LONG_SEQUENCE)  ||
             (windowSequence == LONG_START_SEQUENCE) ||
             (windowSequence == LONG_STOP_SEQUENCE)  ||
             (windowSequence == STOP_START_SEQUENCE) ) {

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

  if ( qdebug > 5 )
    fprintf(stderr,"aac_qc.c: start common scalefactor %d, max_dct_line %f\n", common_scalefac, max_dct_line);
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
            qInfo->quant[i] = (int)(pow_spectrum[i] * quantFac  + MAGIC_NUMBER);
            if (qInfo->quant[i]>MAX_QUANT)
              CommonExit(-1,"\nMaxquant exceeded %d at %d line %e\n", qInfo->quant[i], i, quantFac);
            requant[i] =  pow_quant[qInfo->quant[i]] * invQuantFac;
            qInfo->quant[i] = sgn(spectrum[i]) * qInfo->quant[i];  /* restore the original sign */
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

	  /* if(error_energy[sb]==0)
	    sfb_amplify_check[sb] = 1;*/

          /* find the scalefactor band with the largest error ratio */
          if ((ratio[sb] > maxRatio) && (scale_factor_diff[sb]<60) && aacAllowScalefacs ) {
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
      qInfo->scale_factor[i] = common_scalefac - scale_factor_diff[i] + SF_OFFSET;

      /* scalefactors have to be greater than 0 and smaller than 256 */
      if (qInfo->scale_factor[i]>=(2*TEXP)) {
        fprintf(stderr,"scalefactor of %i truncated to %i\n",qInfo->scale_factor[i],(2*TEXP)-1);
        qInfo->scale_factor[i]=(2*TEXP)-1;
      }
      if (qInfo->scale_factor[i]<0) {
        fprintf(stderr,"scalefactor of %i truncated to 0\n",qInfo->scale_factor[i]);
        qInfo->scale_factor[i]=0;
      }
    }

     
    for(k=0;k<nr_of_sfs;k++) {  
      calc_sf[k]=1;
    }
    
    /*Degroup quantized vector*/
    for (k=0; k<nr_of_sfs; k++){
      for (i=sfb_offset[k]; i<sfb_offset[k+1]; i++){
	qInfo->quantDegroup[i] = qInfo->quant[i]; 
      }
    }
    if (windowSequence == EIGHT_SHORT_SEQUENCE) {
      degroup_int(sfb_offset, nr_of_sfb, qInfo->quantDegroup, 
		  num_window_groups,
		  window_group_length,
		  blockSizeSamples);
    }
    

    qInfo->max_spec_coeffs = 0;
    for (k=0 ; k <max_sfb; k++) {
      qInfo->max_spec_coeffs +=  sfb_width_table[k];
    }
    
    max_bits = usac_fd_cs(NULL,
			  windowSequence,
			  windowShape,
			  qInfo->scale_factor[0],
			  huffman_code_table,
			  max_sfb,
			  nr_of_sfb,
			  num_window_groups,
			  window_group_length,
			  NULL,
			  NULL,   /*No PNS*/
			  tool_data,
			  tnsInfo,
			  qInfo,
			  common_window,
			  common_tw,
			  flag_twMdct,
			  flag_noiseFilling,
			  NULL,
			  0,
                          bUsacIndependencyFlag,
			  qdebug);
    
   
    
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
    if ( qdebug > 5 )
      fprintf(stderr, "usac_fd_qc():outer loop %d %d\n", max_bits, available_bits);
  } while (max_bits > available_bits);

  common_scalefac = common_scalefac_save;

  /* PP Dec 2009 */
  /* compute noise filling parameters */
  if( flag_noiseFilling && (max_dct_line > 1) )
  {
	  
	  if (windowSequence != EIGHT_SHORT_SEQUENCE)
	  {
		  usac_noise_filling(tool_data,spectrum,qInfo,sfb_offset,max_sfb);
	  }
  }

  if ( qdebug > 3 ) {
    fprintf(stderr,"\n*** [bits transmitted = %d] largestRatio=%4.1f loopCnt=%d comm_sf=%d",
	    max_bits, 10*log10(largest_ratio+1e-15), rateLoopCnt, common_scalefac );
  }

  if ( qdebug > 0 ) {
    for (i=0; i<nr_of_sfs; i++){
      if ((error_energy[i]>energy[i]))
        CommonWarning("\nerror_energy greater than energy in sfb: %d ", i);
    }
  }


  /* write the reconstructed spectrum to the output for use with prediction */
  {
    int i;

    for (sb=0; sb<nr_of_sfs; sb++){
      if ( qdebug > 4 ) {
        fprintf(stderr,"\nrequantval band %d : ",sb);
      }
      for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++){
        p_reconstructed_spectrum[i] = sgn(qInfo->quant[i]) * requant[i];
        if ( qdebug > 4 ) {
          fprintf(stderr,"%7.0f",(float)p_reconstructed_spectrum[i]);
        }
      }
    }
    if (windowSequence == EIGHT_SHORT_SEQUENCE) {
      degroup (sfb_offset, nr_of_sfb, p_reconstructed_spectrum,
		     num_window_groups,
		     window_group_length,
		     blockSizeSamples);
    }
  }

  return max_bits;
}


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
  int i,j,ii;
  int index = 0;
  double tmp[NUM_COEFF];
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
  index = 0;
  group_offset=0;
  for (i=0; i< num_window_groups; i++) {
    for (k=0; k<nr_of_sfb; k++) {
      for (j=0; j < window_group_length[i]; j++) {
	for (ii=0;ii< sfb_width_table[k];ii++) {
	  tmp[index++] = p_spectrum[ii+ sfb_offset[k] + shortBlockLines*j +group_offset];
        }
      }
    }
    group_offset +=  shortBlockLines*window_group_length[i];
  }

#ifdef DEBUG_xx
  ii=0;
  for (i=0;i<num_window_groups;i++){
    fprintf(stderr,"\ngroup%d: " ,i);
    for (k=0; k< shortBlockLines *window_group_length[i]; k++){
      fprintf(stderr," %4.0f" ,tmp[ii++]);
    }
  }
#endif
  for (k=0; k<blockSizeSamples; k++){
    p_spectrum[k] = tmp[k];
  }

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

static void degroup (const int grouped_sfb_offsets[MAX_SCFAC_BANDS+1],
		     int sfb_per_group,
		     double *rec_spectrum,
		     int num_window_groups,
		     const int window_group_length[],
		     int blockSizeSamples)
{
  int i, j, k, ii;
  int index, group_offset;
  double tmp[NUM_COEFF];
  int shortBlockLines = blockSizeSamples/NSHORT;

  index = 0;
  group_offset = 0;
  for (i = 0; i < blockSizeSamples; i++)
    tmp[i] = 0;

  for (i = 0; i < num_window_groups; i++) {
    /*for (j = 0; j < shortBlockLines; j++) {*/
    for (j = 0; j < sfb_per_group; j++) {
      for (k = 0; k < window_group_length[i]; k++) {
        for (ii = 0; ii < grouped_sfb_offsets[j+1]-grouped_sfb_offsets[j]; ii++) {
          tmp[ii + grouped_sfb_offsets[j] + shortBlockLines * k + group_offset] = rec_spectrum[index++];
        }
      }
    }
    group_offset += BLOCK_LEN_SHORT * window_group_length[i];
  }

  for (i = 0; i < blockSizeSamples; i++)
    rec_spectrum[i] = tmp[i];
}

static void degroup_int(const int grouped_sfb_offsets[MAX_SCFAC_BANDS+1],
		     int sfb_per_group,
		     int *rec_spectrum,
		     int num_window_groups,
		     const int window_group_length[],
		     int blockSizeSamples)

{
  int i, j, k, ii;
  int index, group_offset;
  int tmp[NUM_COEFF];
  int shortBlockLines = blockSizeSamples/NSHORT;

  index = 0;
  group_offset = 0;
  for (i = 0; i < blockSizeSamples; i++)
    tmp[i] = 0;

  for (i = 0; i < num_window_groups; i++) {
    /*for (j = 0; j < shortBlockLines; j++) {*/
    for (j = 0; j < sfb_per_group; j++) {
      for (k = 0; k < window_group_length[i]; k++) {
        for (ii = 0; ii < grouped_sfb_offsets[j+1]-grouped_sfb_offsets[j]; ii++) {
          tmp[ii + grouped_sfb_offsets[j] + shortBlockLines * k + group_offset] = rec_spectrum[index++];
        }
      }
    }
    group_offset += BLOCK_LEN_SHORT * window_group_length[i];
  }

  for (i = 0; i < blockSizeSamples; i++)
    rec_spectrum[i] = tmp[i];
}
