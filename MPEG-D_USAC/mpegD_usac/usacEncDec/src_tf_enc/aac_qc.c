/***********

This software module was originally developed by

Dolby Laboratories

and edited by

Takashi Koike (Sony Corporation) 
Mikko Suonio (Nokia)
Olaf Kaehler (Fraunhofer IIS-A)

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

***********/
/******************************************************************** 
/
/ filename : nbc_qc.c
/ project : MPEG-2 AAC
/ author : SNL, Dolby Laboratories, Inc
/ date : November 15, 1996
/ contents/description : slow rate/distortion quantizer,  
/    huffman coding, bit packing
/          
/
*******************************************************************/
 
#include <stdio.h>	/* moved here   HP 980211 */
#include <math.h>
#include <float.h>

#include "allHandles.h"
#include "block.h"

#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h" 
#include "bitstream.h"
#include "tf_main.h"
#include "aac_qc.h"
#include "bitmux.h"

#include "sam_encode.h"

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

static
int bit_search(const int quant[NUM_COEFF],
               int book_vector[],
               const int huff[13][1090][4],
               int sfb_offset[],
               int nr_of_sfb,
               int nr_sfb_per_win
               );

static
int noiseless_bit_count(
                        const int quant[NUM_COEFF],
                        const int huff[13][1090][4],
                        int hop,
                        int min_book_choice[MAX_SCFAC_BANDS][3],
                        const int sfb_offset[],
                        int nr_of_sfb,
                        int nr_sfb_per_win
);

static
int output_bits(
                const int huff[13][1090][4],
                int book,
                const int quant[NUM_COEFF],
                int offset,
                int length,
                int data[],
                int len[],
                int *counter,
                int write_flag
);


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

void tf_init_encode_spectrum_aac( /*int qc_select,*/ int debugLevel )
{
  unsigned int   j;

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


int tf_quantize_spectrum(
  QuantInfo   *qInfo,
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],   /* out: grouped sfb offsets */
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  const int   window_group_length[],
  int         aacAllowScalefacs)
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
              CommonExit(-1,"\nMaxquant exceeded");
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

/*OK: temporary disabled */
      {

        /* find a good method to section the scalefactor bands into huffman codebook sections */
        bit_search( qInfo->quant, qInfo->book_vector, huffman_code_table, sfb_offset, nr_of_sfs, nr_of_sfb );

        /* Set special codebook for bands coded via PNS
         * long blocks only */
        if (windowSequence != EIGHT_SHORT_SEQUENCE) {
          for(k=0;k<nr_of_sfs;k++) {
            if (pnsInfo->pns_sfb_flag[k]) {
              if (qInfo->book_vector[k])
                fprintf(stderr,"\n PNS: Subst. over non-zero book (%d), sfb=%d !", qInfo->book_vector[k], k);
              qInfo->book_vector[k] = PNS_HCB;
            }
          }
        }

        for(k=0;k<nr_of_sfs;k++) {  
          calc_sf[k]=1;
        }

        max_bits = bitpack_ind_channel_stream(NULL, /* output_stream */
                                   NULL, /* ics_buffer containing ics_info */
                                   NULL, /* var_buffer containing tns & co */
                                   qInfo,
                                   0, /* commonWindow, does not matter if ics_buffer==NULL*/
                                   windowSequence,
                                   num_window_groups,
                                   sfb_offset,
                                   nr_of_sfb,
                                   pnsInfo,
                                   0, /*scale_flag, does not matter if var_buffer==NULL*/
                                   qdebug);


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

  if ( qdebug > 0 ) {
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








int bitpack_ind_channel_stream(
  HANDLE_AACBITMUX bitmux,
  ICSinfo *ics_info,
  ToolsInfo *tools_info,
  QuantInfo   *qInfo,
  int         commonWindow,
  WINDOW_SEQUENCE windowSequence,
  int         num_window_groups,
  int         sfb_offset[MAX_SCFAC_BANDS+1],
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
  int         scale_flag,
  int         qdebug
)
{
  int data[5*NUM_COEFF];  /* for each pair of spec values 5 bitstream elemets are requiered: 1*(esc)+2*(sign)+2*(esc value)=5 */
  int len[5*NUM_COEFF];
  int counter =0;
  int nr_of_sfs;
  int bits_written = 0;
  int k;
  int WriteFlag = 1;
  if (bitmux==NULL) WriteFlag = 0;

  nr_of_sfs = nr_of_sfb * num_window_groups;

  /* place the codewords and their respective lengths in arrays data[] and len[] respectively */
  /* there are 'counter' elements in each array, and these are variable length arrays depending on the input */
  counter = 0;
  for(k=0;k<nr_of_sfs;k++) {  
    if (( qdebug>3 )&& WriteFlag) fprintf(stderr,"\n{band %d}  ",k);
    bits_written += output_bits(huffman_code_table,
                                qInfo->book_vector[k],
                                qInfo->quant,
                                sfb_offset[k],
                                sfb_offset[k+1]-sfb_offset[k],
                                WriteFlag?data:NULL,
                                WriteFlag?len:NULL,
                                WriteFlag?&counter:NULL,
                                WriteFlag);
  }
  if (WriteFlag) bits_written=0; /* the bits will be counted again in write_ind_cha_stream() */

  /* create the extra header information, and send out the raw data into the bitstream */
  bits_written+= write_ind_cha_stream(WriteFlag?bitmux:NULL,
                                      windowSequence,
                                      qInfo->scale_factor[0],
                                      qInfo->scale_factor,
                                      qInfo->book_vector,
                                      WriteFlag?data:NULL,
                                      WriteFlag?len:NULL,
                                      huffman_code_table,
                                      counter,
                                      nr_of_sfs,
                                      num_window_groups,
                                      pnsInfo->pns_sfb_nrg,
                                      ics_info,
                                      tools_info,
                                      commonWindow,
                                      scale_flag,
                                      qdebug);


  return( bits_written );
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


static
int bit_search(const int quant[NUM_COEFF],
               int book_vector[],
               const int huffman_code_table[13][1090][4],
               int sfb_offset[],
	       int nr_of_sfb,
               int nr_sfb_per_win
	       )
     /*
  This function inputs a vector of quantized spectral data, quant[][], and returns a vector,
  'book_vector[]' that describes how to group together the scalefactor bands into a smaller
  number of sections.  There are SFB_NUM_MAX elements in book_vector (equal to 49 in the 
  case of long blocks and MAX_SCFAC_BANDS for short blocks), and each element has a huffman codebook 
  number assigned to it.

  For a quick and simple algorithm, this function performs a binary
  search across the sfb's (scale factor bands).  On the first approach, it calculates the 
  needed amount of bits if every sfb were its own section and transmitted its own huffman 
  codebook value side information (equal to 9 bits for a long block, 7 for a short).  The 
  next iteration combines adjacent sfb's, and calculates the bit rate for length two sfb 
  sections.  If any wider two-sfb section requires fewer bits than the sum of the two 
  single-sfb sections (below it in the binary tree), then the wider section will be chosen.
  This process occurs until the sections are split into three uniform parts, each with an
  equal amount of sfb's contained.  

  The binary tree is stored as a two-dimensional array.  Since this tree is not full, (there
  are only 49 nodes, not 2^6 = 64), the numbering is a little complicated.  If the tree were
  full, the top node would be 1.  It's children would be 2 and 3.  But, since this tree
  is not full, the top row of three nodes are numbered {4,5,6}.  The row below it is
  {8,9,10,11,12,13}, and so on.  

  The binary tree is called bit_stats[MAX_SCFAC_BANDS][3].  There are MAX_SCFAC_BANDS total nodes (some are not
  used since it's not full).  bit_stats[x][0] holds the bit totals needed for the sfb sectioning
  strategy represented by the node x in the tree.  bit_stats[x][1] holds the optimal huffman
  codebook table that minimizes the bit rate, given the sectioning boundaries dictated by node x.
*/

{
  int i,j,m,n;
  int hop;
  int min_book_choice[128][3]={{0x0}};
  int bit_stats[300][3]={{0x0}};
  int total_bits;
  int total_bit_count;
  int levels;
  double fraction;
  int startIdx,endIdx,startWin,endWin;
  levels = (int) ((log((double)nr_of_sfb)/log((double)2.0))+1); 
  fraction = (pow(2,levels)+nr_of_sfb)/(double)(pow(2,levels)); 
  for(i=0;i<5;i++){
    hop = (int)(pow(2,i));
    if (hop>nr_sfb_per_win){
      hop=nr_sfb_per_win;
    }
    total_bits = noiseless_bit_count(quant,huffman_code_table,hop,min_book_choice,sfb_offset,nr_of_sfb,nr_sfb_per_win); 

    /* load up the (not-full) binary search tree with the min_book_choice values */
    startIdx=0;
    m=0;
    total_bit_count = 0;

    for (j=(int)(pow(2,levels-i)); j<(int)(fraction*pow(2,levels-i)); j++){
      if ((j>=300) || (startIdx >= 128)) {
        fprintf(stderr,"\n j %d startIdx%d,levels %d, fraction %f ",j,startIdx,levels,(float)fraction);
        CommonWarning("\n error in bitcount");
      }

      startWin=startIdx/nr_sfb_per_win;
      
      if ((startIdx+hop) > nr_of_sfb)
        endIdx = nr_of_sfb;
      else
        endIdx = startIdx+hop;
      
      endWin=endIdx/nr_sfb_per_win;
      
      if (startWin!=endWin){
        endIdx=(startWin+1)*nr_sfb_per_win;
      }

                                                                            
      bit_stats[j][0] = min_book_choice[startIdx][0]; /* the minimum bit cost for this section */
      bit_stats[j][1] = min_book_choice[startIdx][1]; /* used with this huffman book number */

      if ( i>0  ){  /* not on the lowest level, grouping more than one signle scalefactor band per section*/
	if  ( (bit_stats[j][0] < bit_stats[2*j][0] + bit_stats[2*j+1][0])  &&  (hop<nr_sfb_per_win) ){
	  /* it is cheaper to combine surrounding sfb secionts into one larger huffman book section */
	  for(n=startIdx;n<endIdx;n++) { /* write the optimal huffman book value for the new larger section */
	    book_vector[n] = bit_stats[j][1];
	  }
        }

	else {  /* it was cheaper to transmit the smaller huffman table sections */
          bit_stats[j][0] = bit_stats[2*j][0] + bit_stats[2*j+1][0];
	}

      } else {  /* during the first stage of the iteration, all sfb's are individual sections */
        book_vector[startIdx] = bit_stats[j][1];  /* initially, set all sfb's to their own optimal section table values */
      }
      total_bit_count = total_bit_count +  bit_stats[j][0];
      startIdx=endIdx;

      m++;
    }
  }
  /*   book_vector[k] = book_vector[k-1]; */
  return(total_bit_count);
}



static
int noiseless_bit_count(
			const int quant_data[NUM_COEFF],
			const int huffman_code_table[13][1090][4],
			int hop,
			int min_book_choice[128][3],
                        const int sfb_offset[],
			int nr_of_sfb,
                        int nr_sfb_per_win
			)
{
  int startIdx,j,k;

  /* 
     This function inputs:
     - the quantized spectral data, 'quant_data[]';
     - all of the huffman codebooks, 'huff[][]';
     - the size of the sections, in scalefactor bands (SFB's), 'hop';
     - an empty matrix, min_book_choice[][] passed to it; 

     This function outputs:
     - the matrix, min_book_choice.  It is a two dimensional matrix, with its
     rows corresponding to spectral sections.  The 0th column corresponds to 
     the bits needed to code a section with 'hop' scalefactors bands wide, all using 
     the same huffman codebook.  The 1st column contains the huffman codebook number 
     that allows the minimum number of bits to be used.   

     Other notes:
     - Initally, the dynamic range is calculated for each spectral section.  The section
     can only be entropy coded with books that have an equal or greater dynamic range
     than the section's spectral data.  The exception to this is for the 11th ESC codebook.
     If the dynamic range is larger than 16, then an escape code is appended after the
     table 11 codeword which encodes the larger value explicity in a pseudo-non-uniform
     quantization method.
     
  */

  int max_sb_coeff;
  int book_choice[12*2][2];
  int total_bits_cost = 0;
  int data[NUM_COEFF*5];
  int len[NUM_COEFF*5];
  int counter = 0;
  int offset, length, end;
  int endIdx;
  int write_flag = 0;
  int startWin,endWin;
  /* each section is 'hop' scalefactor bands wide */
  if (hop>nr_sfb_per_win){
    CommonWarning("\nhop>nr_sfb_per_win");    
  }  
  startIdx=0;
  while(startIdx<nr_of_sfb) {

    startWin=startIdx/nr_sfb_per_win;
   
    if ((startIdx+hop) > nr_of_sfb)
      endIdx = nr_of_sfb;
    else
      endIdx = startIdx+hop;

    endWin=endIdx/nr_sfb_per_win;

    if (startWin!=endWin){
      endIdx=(startWin+1)*nr_sfb_per_win;
    }
    
    /* find the maximum absolute value in the current spectral section, to see what tables are available to use */
    max_sb_coeff = 0;
    for (j=sfb_offset[startIdx]; j<sfb_offset[endIdx]; j++){  /* snl */
      if (abs(quant_data[j]) > max_sb_coeff)
	max_sb_coeff = abs(quant_data[j]);
    }
    
    j = 0;
    offset = sfb_offset[startIdx];
    if ((startIdx+hop) > nr_of_sfb){
      end = sfb_offset[nr_of_sfb];
    }
    else
      end = sfb_offset[startIdx+hop];
    length = end - offset;

    /* all spectral coefficients in this section are zero */
    if (max_sb_coeff == 0) { 
      book_choice[j][0] = output_bits(huffman_code_table,0,quant_data,offset,length,data,len,&counter,write_flag);
      book_choice[j][1] = 0; 
      j++;
    }

    else {  /* if the section does have non-zero coefficients */
      if(max_sb_coeff < 2){
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,1,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 1;
	j++;
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,2,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 2;
	j++;
      }
      if (max_sb_coeff < 3){
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,3,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 3;
	j++;
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,4,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 4;
	j++;
      }
      if (max_sb_coeff < 5){
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,5,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 5;
	j++;
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,6,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 6;
	j++;
      }
      if (max_sb_coeff < 8){
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,7,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 7;
	j++;
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,8,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 8;
	j++;
      }
      if (max_sb_coeff < 13){
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,9,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 9;
	j++;
	counter = 0; /* just for debugging : using data and len vectors */
	book_choice[j][0] = output_bits(huffman_code_table,10,quant_data,offset,length,data,len,&counter,write_flag);
	book_choice[j][1] = 10;
	j++;
      }
      /* (max_sb_coeff >= 13), choose table 11 */
      counter = 0; /* just for debugging : using data and len vectors */
      book_choice[j][0] = output_bits(huffman_code_table,11,quant_data,offset,length,data,len,&counter,write_flag);
      book_choice[j][1] = 11;
      j++;
    }

    /* find the minimum bit cost and table number for huffman coding this scalefactor section */
    min_book_choice[startIdx][0] = 100000;  
    for(k=0;k<j;k++){
      if (book_choice[k][0] < min_book_choice[startIdx][0]){
	min_book_choice[startIdx][1] = book_choice[k][1];
	min_book_choice[startIdx][0] = book_choice[k][0];
      }
    }
    total_bits_cost += min_book_choice[startIdx][0];
    startIdx=endIdx;  /*         startIdx+hop; */
  }
  return(total_bits_cost);
}



static int calculate_esc_sequence(
                                  int input,
                                  int *len_esc_sequence
                                  )
     /* 
   This function takes an element that is larger than 16 and generates the base10 value of the 
   equivalent escape sequence.  It returns the escape sequence in the variable, 'output'.  It
   also passed the length of the escape sequence through the parameter, 'len_esc_sequence'.
*/

{
  float x,y;
  int output;
  int N;

  N = -1;
  y = (float)abs(input);
  x = y / 16;

  while (x >= 1) {
    N++;
    x = x/2;
  }

  *len_esc_sequence = 2*N + 5;  /* the length of the escape sequence in bits */

  output = (int)((pow(2,N) - 1)*pow(2,N+5) + y - pow(2,N+4));
  return(output);
}


static
int output_bits(
		const int huffman_code_table[13][1090][4],
		int book,	   
		const int quant_data[NUM_COEFF],		
		int offset,
		int length,
                int data[],
                int len[],
                int *outside_counter,
                int write_flag
		)
{
  /* 
     This function inputs 
     - all the huffman codebooks, 'huff[]' 
     - a specific codebook number, 'book'
     - the quantized spectral data, 'quant_data[]'
     - the offset into the spectral data to begin scanning, 'offset'
     - the 'length' of the segment to huffman code
     -> therefore, the segment quant_data[offset] to quant_data[offset+length-1]
     is huffman coded.
     - a flag, 'write_flag' to determine whether the codebooks and lengths need to be written
     to file.  If write_flag=0, then this function is being used only in the quantization
     rate loop, and does not need to spend time writing the codebooks and lengths to file.
     If write_flag=1, then it is being called by the function output_bits(), which is 
     sending the bitsteam out of the encoder.  

     This function outputs 
     - the number of bits required, 'bits'  using the prescribed codebook, book applied to 
     the given segment of spectral data.

     There are three parameters that are passed back and forth into this function.  data[]
     and len[] are one-dimensional arrays that store the codebook values and their respective
     bit lengths.  These are used when packing the data for the bitstream in output_bits().  The
     index into these arrays is 'outside_counter'.  It gets incremented internally in this
     function as counter, then passed to the outside through outside_counter.  The next time
     output_bits() is called, counter starts at the value it left off from the previous call.

   */
     
  int esc_sequence;     
  int len_esc;
  int index;
  int bits=0;
  int tmp = 0;
  int codebook,i,j;
  int counter = 0;

  if (write_flag) counter = *outside_counter;

  if (book == 0) {  /* if using the zero codebook, data of zero length is sent */
   
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d]  ",book);
    if (write_flag) {
      data[counter] = 0;
      len[counter++] = 0;
    }
  }

  if ((book == 1) || (book == 2)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+4){
      index = 27*quant_data[i] + 9*quant_data[i+1] + 3*quant_data[i+2] + quant_data[i+3] + 40;
      codebook = huffman_code_table[book][index][3];
      tmp = huffman_code_table[book][index][2];
      bits = bits + tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d %d %d ",quant_data[i],quant_data[i+1],quant_data[i+2],quant_data[i+3]);
	data[counter] = codebook;
        len[counter++] = tmp;
      }
    }
  }

  if ((book == 3) || (book == 4)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+4){
      index = 27*abs(quant_data[i]) + 9*abs(quant_data[i+1]) + 3*abs(quant_data[i+2]) + abs(quant_data[i+3]);
      codebook = huffman_code_table[book][index][3];
      tmp = huffman_code_table[book][index][2];
      bits = bits + tmp;
      for(j=0;j<4;j++){
	if(abs(quant_data[i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d %d %d ",quant_data[i],quant_data[i+1],quant_data[i+2],quant_data[i+3]);
        data[counter] = codebook;
        len[counter++] = tmp;
	for(j=0;j<4;j++){
	  if(quant_data[i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant_data[i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 5) || (book == 6)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 9*(quant_data[i]) + (quant_data[i+1]) + 40;
      codebook = huffman_code_table[book][index][3];
      tmp = huffman_code_table[book][index][2];
      bits = bits + tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant_data[i],quant_data[i+1]);
        data[counter] = codebook;
        len[counter++] = tmp;
      }
    }
  }

  if ((book == 7) || (book == 8)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 8*abs(quant_data[i]) + abs(quant_data[i+1]);
      codebook = huffman_code_table[book][index][3];
      tmp = huffman_code_table[book][index][2];
      bits = bits + tmp;
      for(j=0;j<2;j++){
	if(abs(quant_data[i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant_data[i],quant_data[i+1]);
	data[counter] = codebook;
	len[counter++] = tmp;
	for(j=0;j<2;j++){
	  if(quant_data[i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant_data[i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 9) || (book == 10)) {
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    for(i=offset;i<offset+length;i=i+2){
      index = 13*abs(quant_data[i]) + abs(quant_data[i+1]);
      codebook = huffman_code_table[book][index][3];
      tmp = huffman_code_table[book][index][2];
      bits = bits + tmp;
      for(j=0;j<2;j++){
	if(abs(quant_data[i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant_data[i],quant_data[i+1]);
        
	data[counter] = codebook;
        len[counter++] = tmp;

	for(j=0;j<2;j++){
	  if(quant_data[i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant_data[i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }
    }
  }

  if ((book == 11)){
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    /* First, calculate the indecies into the huffman tables */

    for(i=offset;i<offset+length;i=i+2){
      if ((abs(quant_data[i]) >= 16) && (abs(quant_data[i+1]) >= 16)) {  /* both codewords were above 16 */
        /* first, code the orignal pair, with the larger value saturated to +/- 16 */
	index = 17*16 + 16;
      }
      else if (abs(quant_data[i]) >= 16) {  /* the first codeword was above 16, not the second one */
        /* first, code the orignal pair, with the larger value saturated to +/- 16 */
	index = 17*16 + abs(quant_data[i+1]);
      }
      else if (abs(quant_data[i+1]) >= 16) { /* the second codeword was above 16, not the first one */
	index = 17*abs(quant_data[i]) + 16;
      }
      else {  /* there were no values above 16, so no escape sequences */
	index = 17*abs(quant_data[i]) + abs(quant_data[i+1]);
      }

      /* write out the codewords */

      tmp = huffman_code_table[book][index][2];
      codebook = huffman_code_table[book][index][3];       
      bits += tmp;
      if (write_flag) {
	if (qdebug>3)
	  fprintf(stderr," %d %d ",quant_data[i],quant_data[i+1]);
	/*	printf("[book %d] {%d %d} \n",book,quant_data[i],quant_data[i+1]);*/
	data[counter] = codebook;
	len[counter++] = tmp;
      }
   
      /* Take care of the sign bits */

      for(j=0;j<2;j++){
	if(abs(quant_data[i+j]) > 0) bits += 1; /* only for non-zero spectral coefficients */
      }
      if (write_flag) {
	for(j=0;j<2;j++){
	  if(quant_data[i+j] > 0) {  /* send out '0' if a positive value */
	    data[counter] = 0;
	    len[counter++] = 1;
	  }
	  if(quant_data[i+j] < 0) {  /* send out '1' if a negative value */
	    data[counter] = 1;
	    len[counter++] = 1;
	  }
	}
      }

      /* write out the escape sequences */

      if ((abs(quant_data[i]) >= 16) && (abs(quant_data[i+1]) >= 16)) {  /* both codewords were above 16 */
        /* code and transmit the first escape_sequence */
        esc_sequence = calculate_esc_sequence(quant_data[i],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }

        /* then code and transmit the second escape_sequence */
        esc_sequence = calculate_esc_sequence(quant_data[i+1],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      }

      else if (abs(quant_data[i]) >= 16) {  /* the first codeword was above 16, not the second one */
        /* code and transmit the escape_sequence */
        esc_sequence = calculate_esc_sequence(quant_data[i],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      }

      else if (abs(quant_data[i+1]) >= 16) { /* the second codeword was above 16, not the first one */
        /* code and transmit the escape_sequence */
        esc_sequence = calculate_esc_sequence(quant_data[i+1],&len_esc); 
        bits += len_esc;
        if (write_flag) {
          data[counter] = esc_sequence;
          len[counter++] = len_esc;
        }
      } 
    }
  }

  if (book == PNS_HCB) {  /* if using the PNS codebook, data of zero length is sent */
   
    if (( qdebug>3) && write_flag)
      fprintf(stderr,"\n [book %d] start%d  ",book,offset);
    if (write_flag) {
      data[counter] = 0;
      len[counter++] = 0;
    }
  }

  /* send the current count back to the outside world */
  if (write_flag) *outside_counter = counter;

  return(bits);
}
