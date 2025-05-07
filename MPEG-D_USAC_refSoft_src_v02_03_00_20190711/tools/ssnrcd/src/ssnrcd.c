
/*
  
  ====================================================================
  Copyright Header            
  ====================================================================
  
  This software module was originally developed by 

  Ralf Funken (Philips, Eindhoven, The Netherlands) 

  and edited by 

  Takehiro Moriya  (NTT Labs. Tokyo, Japan) 
  Ralph Sperschneider (Fraunhofer IIS, Germany)

  in the course of  development of the MPEG-2/MPEG-4 Conformance ISO/IEC
  13818-4, 14496-4. This software module  is an implementation of one or
  more of the Audio Conformance   tools  as specified by   MPEG-2/MPEG-4
  Conformance.   ISO/IEC gives  users  of  the  MPEG-2 AAC/MPEG-4  Audio
  (ISO/IEC 13818-3,7, 14496-3)  free license to  this software module or
  modifications thereof for use  in hardware or software products. Those
  intending to use this software module in hardware or software products
  are advised that  its use may infringe  existing patents. The original
  developer of this software  module and his/her company, the subsequent
  editors and their companies, and ISO/IEC  have no liability for use of
  this    software    module   or     modifications   thereof   in    an
  implementation. Philips retains full right to use the code for his/her
  own   purpose, assign  or  donate the   code to   a third party.  This
  copyright  notice   must  be included in     all  copies or derivative
  works. Copyright 2000, 2009.
  
*/



/*  Standard includes                           */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*  Includes for AFsp Library                   */

#include "libtsp.h"
#include "libtsp/AFpar.h"

/*  Private includes                            */

#include "common.h"
#include "cepstrum_analysis.h"
#include "audiofile_io.h"
#include "ssnrcd.h"


/*****************************************************************************/

/*     MPEG-4 CELP Conformance criteria   */

#define         ABSDIF_THRESHOLD(K)             ((1.0 / (double) (1 << ((K)-2))))                               /*  2^-(K-2)                            */
#define         RMS_THRESHOLD(K)                (20.0 * log10 (1.0 / ((double)(1 << ((K)-1)) * sqrt(12.0))))    /*  20*log10((2^(-(K-1))) / sqrt(12))   */

#define         DEFAULT_SEGMENTAL_SNR_THRESHOLD (30.0)        /*  in dB  */
#define         CEPSTRAL_DISTORTION_THRESHOLD   (1.0)         /*  in dB  */ 


/*     Defines for applying conformance criteria    */

#define         RMS_ABSMAXDIFSAMPLE_CRITERION   (0x01)
#define         SEGSNR_CD_CRITERION             (0x02)
#define         NR_BITS_ACCURACY_CRITERION      (0x04)
                                                
#define         PCM_SCALEFACTOR                 (32768.0)

#define         DEFAULT_SEGMENT_LENGTH          (320)   /* default segment length in samples                       */
#define         DEFAULT_CRITERIA                (0)     /* default criteria                                        */
#define         DEFAULT_RESOLUTION              (16)    /* default resolution in bits                              */
#define         MINIMUM_RESOLUTION              (1)
#define         MAXIMUM_RESOLUTION              (24)
#define         MINIMUM_SEGMENTAL_SNR_THRESHOLD (0.0)   /*  in dB  */
#define         MAXIMUM_SEGMENTAL_SNR_THRESHOLD (100.0) /*  in dB  */
                                                 
#define         INITIAL_RMS_dB                  (-999.9)
#define         THRESHOLD_RMS_dB                (-999.0)
#define         INITIAL_SNR_dB                  (-999.9)
#define         THRESHOLD_SNR_dB                (-999.0)
                                                 
#define         SEGSNR_MIN_NOISE                (1e-13 * (self->m_segment_length))
#define         SSNR_CD_LOWERBOUND              (-50.0)  /*  in dB  */
#define         SSNR_CD_UPPERBOUND              (-15.0)  /*  in dB  */


/****************************************************************************/

struct ssnrcd_obj
{ 
  unsigned long           no_diff_samples[MAX_PCM_CHANNELS];
  double                  maxabsdif_sample[MAX_PCM_CHANNELS];
  double                  rms_dif[MAX_PCM_CHANNELS];
  double                  ssnr[MAX_PCM_CHANNELS];
  double                  avg_CD[MAX_PCM_CHANNELS];   
  int                     max_rms_accuracy;
  int                     max_lsb_accuracy;
  unsigned long           distribution_list[(MAXIMUM_RESOLUTION + 1) * MAX_PCM_CHANNELS];
  int                     m_segment_length;
  unsigned int            m_apply_criteria;
  int                     m_bits_resolution;
  double                  m_seg_snr_threshold;
  int                     m_show_accuracy_distribution;
  int                     m_exponential_output;  
  int                     m_max_resolution;
};

/***************************************************************************/

ssnrcd_ptr Ssnrcd_Init ( void )
{
  ssnrcd_ptr self;
  int ch;
    
  /* allocate memory */
  if(! ( self = (ssnrcd_ptr) calloc(1,sizeof(struct ssnrcd_obj))) )
    return NULL;
    
  /* default values */ 
  for (ch = 0; ch < MAX_PCM_CHANNELS; ch++)
    {    
      self->rms_dif[ch]   =   INITIAL_RMS_dB;
    }
    
  self->max_lsb_accuracy              = MAXIMUM_RESOLUTION;
  self->max_rms_accuracy              = MAXIMUM_RESOLUTION;
  self->m_segment_length              = DEFAULT_SEGMENT_LENGTH;
  self->m_apply_criteria              = DEFAULT_CRITERIA;
  self->m_bits_resolution             = DEFAULT_RESOLUTION;
  self->m_seg_snr_threshold           = DEFAULT_SEGMENTAL_SNR_THRESHOLD;
  self->m_show_accuracy_distribution  = FALSE;
  self->m_max_resolution              = MAXIMUM_RESOLUTION;
  self->m_exponential_output          = FALSE; 
  return self;
}


/***************************************************************************/

void Ssnrcd_Free ( ssnrcd_ptr self )
{
  if (NULL != self) {
    free(self);
  }
}

/***************************************************************************/

void Ssnrcd_Usage ( void )
{
  fprintf (stderr, "SSNRCD/RMS related options:\n");
  fprintf (stderr, "                -a           Show distribution of bit accuracy over all samples\n");

  fprintf (stderr, "                -e [<n>]     Exponential output of values scaled to 2^n. If no\n");
  fprintf (stderr, "                             n is given, bit resolution of file under test will\n");
  fprintf (stderr, "                             be used.\n");
  
  fprintf (stderr, "                -k <num>     Resolution for calculation of RMS and max. abs.\n");
  fprintf (stderr, "                             difference sample, expressed in bits.\n");
  fprintf (stderr, "                             Valid range:\n");
  fprintf (stderr, "                               [%d...%d]\n", MINIMUM_RESOLUTION, MAXIMUM_RESOLUTION);
  fprintf (stderr, "                             Default: %d bits.\n", DEFAULT_RESOLUTION);

  fprintf (stderr, "                -s <num>     Threshold for the Segmental SNR criterion in dB.\n");
  fprintf (stderr, "                             <num> shall be an integer value the range\n");
  fprintf (stderr, "                               [%d...%d].\n", (int)MINIMUM_SEGMENTAL_SNR_THRESHOLD, (int)MAXIMUM_SEGMENTAL_SNR_THRESHOLD);
  fprintf (stderr, "                             Default: %d dB.\n", (int)DEFAULT_SEGMENTAL_SNR_THRESHOLD);

  fprintf (stderr, "                -l <num>     Analysis segment length in samples.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_SEGMENT_LENGTH);
  
  fprintf (stderr, "                -t <num>     Conformance criteria to be applied.\n");
  fprintf (stderr, "                               0 : No criterion (default)\n");
  fprintf (stderr, "                               1 : RMS + Max. absolute difference sample\n");
  fprintf (stderr, "                                   using K bit resolution (see -k option)\n");
  fprintf (stderr, "                               2 : Segmental SNR + avg. Cepstral Distortion\n");
  fprintf (stderr, "                               4 : Number of bits accuracy reached for RMS\n");
  fprintf (stderr, "                                   criterion\n");
  fprintf (stderr, "                               7 : enable all test critaria\n");

  fprintf (stderr, "\n");

  fprintf (stderr, "Note: Segmental SNR and cepstral distortion are calculated only for\n");
  fprintf (stderr, "      segments with reference signal power in the range [%.0f...%.0f] dB.\n", SSNR_CD_LOWERBOUND, SSNR_CD_UPPERBOUND);
  fprintf (stderr, "      Any partial segments at the end of the input signal are padded with\n");
  fprintf (stderr, "      zeroes when calculating segmental SNR and Cepstral Distortion.\n");
  fprintf (stderr, "\n");
}

/***************************************************************************/

void Ssnrcd_SetSegmentLength(ssnrcd_ptr self,
                             int        segmentLength)
{
  self->m_segment_length = segmentLength;
}

/***************************************************************************/

void Ssnrcd_SetApplyCriteria(ssnrcd_ptr   self,
                             unsigned int applyCriteria)
{
  self->m_apply_criteria = applyCriteria;
}

/***************************************************************************/

void Ssnrcd_SetShowAccuracyDistribution(ssnrcd_ptr self,
                                        int        showAccuracyDistribution)
{
  self->m_show_accuracy_distribution = showAccuracyDistribution;
}

/***************************************************************************/

void Ssnrcd_SetExponentialOutput(ssnrcd_ptr self,
                                 int        exponentialOutput)
{
  self->m_exponential_output = exponentialOutput;
}

/***************************************************************************/

void Ssnrcd_SetBitsResolution(ssnrcd_ptr self,
                              int         bitsResolution)
{
  if ((bitsResolution >= MINIMUM_RESOLUTION) && (bitsResolution <= MAXIMUM_RESOLUTION)) {
    self->m_bits_resolution = bitsResolution;
  }
  else {
    fprintf(stderr,
            "resolution %d out of range, using default %d\n",
            bitsResolution,
            DEFAULT_RESOLUTION);
  }
}

/***************************************************************************/

void Ssnrcd_SetSegSnrThreshold(ssnrcd_ptr self,
                               int         segSnrThreshold)
{
  double threshold = (double)segSnrThreshold;

  if ((threshold >= MINIMUM_SEGMENTAL_SNR_THRESHOLD) && (threshold <= MAXIMUM_SEGMENTAL_SNR_THRESHOLD)) {
    self->m_seg_snr_threshold = threshold;
  }
  else {
    fprintf(stderr,
            "threshold %f out of range, using default %f\n",
            threshold,
            DEFAULT_SEGMENTAL_SNR_THRESHOLD);
  }
}

/***************************************************************************/

ERRORCODE Ssnrcd_CheckSettings(ssnrcd_ptr   self,
                               WAVEFILES*   wavefiles)
{
  if ((self->m_apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION) {
    if (self->m_bits_resolution >= wavefiles->m_waveformReference.m_bits_per_sample) {    
      printf ("WARNING: Resolution of reference waveform too low for requested %d bit conformance testing.\n", 
              self->m_bits_resolution);
    }

    if (self->m_bits_resolution >= wavefiles->m_waveformUnderTest.m_bits_per_sample) {    
      printf ("WARNING: Resolution of waveform under test too low for requested %d bit conformance testing.\n", 
              self->m_bits_resolution);
    }
  }
    
  if (self->m_exponential_output) {
    if (self->m_exponential_output == 1) {
      self->m_exponential_output = wavefiles->m_waveformUnderTest.m_bits_per_sample;
    }
        
    if (self->m_bits_resolution > self->m_exponential_output) {
      printf ("ERROR: Resolution for calculation of RMS and max. abs. (%d bit) is greather than output resolution (%d bit).\n", 
              self->m_bits_resolution, 
              self->m_exponential_output);
      return RMS_CALCULATION_ERROR;
    }
        
    self->m_max_resolution = self->m_exponential_output;
  }
  return OKAY;
}

/***************************************************************************/

ERRORCODE Ssnrcd_CompareAudiodata (WAVEFILES*          wavefiles,
                                   int                 verbose,
                                   ssnrcd_ptr          self)
{
  int                 sample_counter, abs_sample_no, n, ch, sample_accuracy;
  int                 segment_accuracy [MAX_PCM_CHANNELS];
  double              sample_ref, sample_test;
  double              abs_difference, abs_difference_scaled;
  double              maxabsdif_segment [MAX_PCM_CHANNELS];
  double              overall_cum_difference [MAX_PCM_CHANNELS];
  int                 nr_segments_total, segment_counter;
  int                 samples_read1, samples_read2;
  int                 nr_segments_snrcd [MAX_PCM_CHANNELS];
  double              seg_cum_signal1 [MAX_PCM_CHANNELS];
  double              seg_cum_signal2 [MAX_PCM_CHANNELS];
  double              ss_cum [MAX_PCM_CHANNELS];
  double*             pcm_buffer1;
  double*             pcm_buffer2;
  double*             temp_pcm_buffer;
  double              lpc_coef [N_PR + 1];
  double              cepstrum1 [2*N_PR + 1], cepstrum2 [2*N_PR + 1]; 
  double              cum_sqrt_D [MAX_PCM_CHANNELS];
  double              D;
  double              refsig_power_dB;
  float*              ham_window;
  float               wlag [N_PR + 1];
  const int           scale_up = 1 << (self->m_exponential_output - 1);
  ERRORCODE           status = OKAY;
    
  /****************************************************************************/
  /*   Memory allocation                                                      */
  /****************************************************************************/
     
  pcm_buffer1     = (double*) malloc (sizeof (double) * wavefiles->m_numberOfChannels * self->m_segment_length);
  pcm_buffer2     = (double*) malloc (sizeof (double) * wavefiles->m_numberOfChannels * self->m_segment_length);
  temp_pcm_buffer = (double*) malloc (sizeof (double) * self->m_segment_length);
  ham_window      =  (float*) malloc (sizeof  (float) * self->m_segment_length);
   
  if ((pcm_buffer1 == NULL) || 
      (pcm_buffer2 == NULL) || 
      (temp_pcm_buffer == NULL) || 
      (ham_window == NULL)) {
    printf ("ERROR: Memory allocation failed.\n");
    status = MEMORYALOCATION_ERROR;
    goto bail;
  }
   

  /****************************************************************************/
  /*   Variable initialization                                                */
  /****************************************************************************/

  for (ch = 0; ch < MAX_PCM_CHANNELS; ch++) {
    nr_segments_snrcd [ch]       = 0;
    overall_cum_difference [ch]  = 0.0;
    seg_cum_signal1 [ch]         = 0.0;
    seg_cum_signal2 [ch]         = 0.0;
    ss_cum [ch]                  = 0.0; 
    cum_sqrt_D [ch]              = 0.0;
  }  

    
  nr_segments_total = 1 + ((wavefiles->m_samplesToCompare - 1) / self->m_segment_length);
  abs_sample_no = 0;
    
  for (n = 0; n <= N_PR; n++) {
    lpc_coef [n] = 0.0;
  }

  /****************************************************************************/
  /*   Calculate windows for cepstral analysis                                */
  /****************************************************************************/

  hamwdw (ham_window, self->m_segment_length);
  lagwdw (wlag, N_PR, BW);
   

  /****************************************************************************/
  /*   Verbose output                                                         */
  /****************************************************************************/
    
  if (verbose == 2) {
    printf ("\n%s  Segments which didn't reach the LSB criterion:\n", GREP_STRING);
  }
  else if (verbose == 3) {
    printf ("\n%s  Samples which didn't reach the LSB criterion :\n", GREP_STRING);
  }
  else if (verbose == 4) {
    printf ("\n%s  Difference for all samples                   :\n", GREP_STRING);
  }
    
    
  /****************************************************************************/
  /*   Process audio input files                                              */
  /****************************************************************************/
  
  for (segment_counter = 0; segment_counter < nr_segments_total; segment_counter++) {
    /****************************************************************************/
    /*   Reset variables for segment loop                                       */
    /****************************************************************************/
    
    for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
      segment_accuracy  [ch] = self->m_max_resolution;
      maxabsdif_segment [ch] = 0.0;
    }
    
    /****************************************************************************/
    /*   Read one segment of PCM data from both input files                     */
    /****************************************************************************/
    
    if (verbose == 1) {
      fprintf (stderr, "   %d %% \r", (100 * segment_counter) / nr_segments_total);
    }
    
    samples_read1 = Waveform_GetSample (pcm_buffer1, 
                                        wavefiles->m_waveformReference.m_stream, 
                                        wavefiles->m_waveformReference.m_channels, 
                                        segment_counter * self->m_segment_length + wavefiles->m_waveformReference.m_offset, 
                                        self->m_segment_length);

    samples_read2 = Waveform_GetSample (pcm_buffer2, 
                                        wavefiles->m_waveformUnderTest.m_stream,
                                        wavefiles->m_waveformUnderTest.m_channels, 
                                        segment_counter * self->m_segment_length + wavefiles->m_waveformUnderTest.m_offset,
                                        self->m_segment_length); 
                                         
    Waveform_ApplyZeropadding (pcm_buffer1, 
                               wavefiles->m_numberOfChannels, 
                               MIN (samples_read1, samples_read2),
                               self->m_segment_length);

    Waveform_ApplyZeropadding (pcm_buffer2, 
                               wavefiles->m_numberOfChannels, 
                               MIN (samples_read1, samples_read2),
                               self->m_segment_length);
        
    /****************************************************************************/
    /*   Calculate max. abs. difference, SNR, RMS, etc. in current segment      */
    /****************************************************************************/

    for (sample_counter = 0; sample_counter < MIN(samples_read1, samples_read2); sample_counter++) {
      abs_sample_no++;
            
      for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
        /*   read reference and test sample from buffer */

        sample_ref  = pcm_buffer1[(sample_counter * wavefiles->m_numberOfChannels) + ch];
        sample_test = pcm_buffer2[(sample_counter * wavefiles->m_numberOfChannels) + ch];
                
        /*   count number of different samples and 
             calculate max. absolute value in difference signal  */
                
        if (sample_ref == sample_test) {
          abs_difference = 0.0;
          abs_difference_scaled = 0.0;
        }
        else {
          (self->no_diff_samples[ch]) ++;
          abs_difference = fabs (sample_ref - sample_test);
          abs_difference_scaled = abs_difference / PCM_SCALEFACTOR;
        }
                
        if (abs_difference_scaled > self->maxabsdif_sample[ch]) {
          self->maxabsdif_sample[ch] = abs_difference_scaled;
        }
                
        if (abs_difference_scaled > maxabsdif_segment [ch]) {
          maxabsdif_segment [ch] = abs_difference_scaled;
        }

        /*   calculate accuracy of sample from its absolute difference */
                
        if  (abs_difference_scaled == 0.0) {
          sample_accuracy = self->m_max_resolution + 2;
        }
        else {
          for (sample_accuracy = self->m_max_resolution + 1; (abs_difference_scaled > ABSDIF_THRESHOLD(sample_accuracy)) && (sample_accuracy > 2); sample_accuracy--);
        }
                
        /*   get maximal LSB accuracy */
                
        if (sample_accuracy < self->max_lsb_accuracy) {
          self->max_lsb_accuracy = sample_accuracy;
        }
                
        if (sample_accuracy < segment_accuracy [ch]) {
          segment_accuracy [ch] = sample_accuracy;
        }
            
        /*   count number of samples with their specific absolute difference */
                
        (self->distribution_list[((MAXIMUM_RESOLUTION + 1) * ch) + (sample_accuracy - 2)]) ++;
                
        /*  output accuracy for sample (option -v) */
                
        if ( ((verbose==3) && (sample_accuracy < self->m_bits_resolution)) || verbose==4) {
          printf ("channel: %2d, segment: %8d,  sample: %8d,  absolute sample: %10d,  ", ch, segment_counter, sample_counter, abs_sample_no);
          if (self->m_exponential_output) {
            printf ("difference: 2 ^ %6.3e = ", LD(abs_difference_scaled * scale_up));
            if (LD(abs_difference_scaled * scale_up) < 0) {
              printf ("%.3e\n", abs_difference_scaled * scale_up);
            }
            else {
              printf ("%.0f\n", abs_difference_scaled * scale_up);
            }
          }
          else {
            printf ("difference: %.10e\n", abs_difference_scaled);
          }
        }
                
        /*   collect statistics for overall RMS      */
                
        overall_cum_difference [ch] += (abs_difference_scaled * abs_difference_scaled);
                
        /*   collect statistics for segmental SNR    */
                
        seg_cum_signal1 [ch] += (pcm_buffer1[(sample_counter * wavefiles->m_numberOfChannels) + ch] / PCM_SCALEFACTOR) * (pcm_buffer1[(sample_counter * wavefiles->m_numberOfChannels) + ch] / PCM_SCALEFACTOR);
        seg_cum_signal2 [ch] += (abs_difference_scaled * abs_difference_scaled);
               
      }   /*  for  ch  */
    }
         
    /****************************************************************************/
    /*   Process completed segment                                              */
    /****************************************************************************/
        
    for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
      if (seg_cum_signal1 [ch] > 0.0) {
        refsig_power_dB = 10 * log10 (seg_cum_signal1 [ch] / self->m_segment_length);
                
        if ((refsig_power_dB >= SSNR_CD_LOWERBOUND) && (refsig_power_dB <= SSNR_CD_UPPERBOUND)) {   
          nr_segments_snrcd[ch]++;

          ss_cum [ch] += log10 (1.0 + (seg_cum_signal1 [ch] / (SEGSNR_MIN_NOISE + seg_cum_signal2 [ch])));
                    
          /****************************************************************************/
          /*   Cepstral analysis on current segment                                   */
          /****************************************************************************/
                    
          Waveform_CopySample(temp_pcm_buffer, pcm_buffer1, ch, wavefiles->m_numberOfChannels, self->m_segment_length);
          calculate_lpc  (temp_pcm_buffer, self->m_segment_length, lpc_coef, ham_window, wlag);
          lpc2cepstrum (N_PR, lpc_coef, 2 * N_PR, cepstrum1);
                    
          Waveform_CopySample(temp_pcm_buffer, pcm_buffer2, ch, wavefiles->m_numberOfChannels, self->m_segment_length);
          calculate_lpc  (temp_pcm_buffer, self->m_segment_length, lpc_coef, ham_window, wlag);
          lpc2cepstrum (N_PR, lpc_coef, 2 * N_PR, cepstrum2);
                    
          for (n = 1, D = 0.0; n <= 2 * N_PR; n++) {
            D += ((cepstrum1[n] - cepstrum2[n]) * (cepstrum1[n] - cepstrum2[n]));
          }
                    
          cum_sqrt_D [ch] += sqrt (D);
        } 
      }
            
      seg_cum_signal1 [ch] = 0.0;
      seg_cum_signal2 [ch] = 0.0;

        
      /*  output accuracy for segment (option -v) */
            
      if ((verbose==2) && (segment_accuracy [ch] < self->m_bits_resolution)) {
        printf ("channel: %2d, segment: %8d,  ", ch, segment_counter);
        if (self->m_exponential_output) {
          printf ("maximum difference: 2 ^ %6.3e = ", LD(maxabsdif_segment [ch] * scale_up));
          if (LD(maxabsdif_segment [ch] * scale_up) < 0) {
            printf ("%.3e\n", maxabsdif_segment [ch] * scale_up);
          }
          else {
            printf ("%.0f\n", maxabsdif_segment [ch] * scale_up);
          }
        }
        else {
          printf ("maximum difference: %.10e\n", maxabsdif_segment [ch]);
        }
      }

    }   /*  for  ch  */
        
  }   /*  for segment_counter   */
    
  /****************************************************************************/
  /*   Calculate overall statistics                                           */
  /****************************************************************************/

  /*   calculate overall RMS value of difference signal (in dB)  */

  for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
    if (overall_cum_difference [ch] > 0.0) {
      self->rms_dif[ch] = 20 * log10 (sqrt (overall_cum_difference [ch] / wavefiles->m_samplesToCompare));
    }
  }

    
  /*   calculate average segmental SNR value of difference signal (in dB)  */

  for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
    if ((ss_cum [ch] > 0.0) && (nr_segments_snrcd [ch] > 0)) {
      self->ssnr[ch] = 10 * log10 (pow (10.0, (ss_cum [ch] / nr_segments_snrcd [ch])) - 1.0);
    }
    else {
      self->ssnr[ch] = INITIAL_SNR_dB;
    }
  }


  /*   calculate average cepstral distortion (in dB)   */
    
  for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
    if (nr_segments_snrcd [ch] > 0) {
      self->avg_CD[ch] = ((10.0 * sqrt (2.0)) / (log(10.0) *  nr_segments_snrcd[ch])) * cum_sqrt_D [ch];
    }
    else
      {
        self->avg_CD[ch] = 0.0; 
      }
  }


  /****************************************************************************/
  /*   Calculate maximal RMS accuracy                                         */
  /****************************************************************************/
    
  for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) { 
    while ((self->rms_dif[ch] > RMS_THRESHOLD(self->max_rms_accuracy)) && 
           (self->max_rms_accuracy > 0)) {
      (self->max_rms_accuracy)--;
    }        
  }
       
    
  /****************************************************************************/
  /*   Verbose output                                                         */
  /****************************************************************************/
    
  if (verbose > 1) {
    printf ("%s  end of list.\n\n", GREP_STRING);
  }
    
  /****************************************************************************/
  /*   Free all allocated memory                                              */
  /****************************************************************************/

 bail:
  free (ham_window);
  free (temp_pcm_buffer);
  free (pcm_buffer2);
  free (pcm_buffer1);

  return status;
}

/***************************************************************************/

ERRORCODE  Ssnrcd_GenerateReport (WAVEFILES*  wavefiles,
                                  ssnrcd_ptr  self)
{

  int       ch, accuracy;
  char      full_acc_fail = FALSE;
  char      lim_acc_fail  = FALSE;
  ERRORCODE status        = OKAY;
  const int scale_up      = 1 << (self->m_exponential_output - 1);

    
  /*  print results per channel   */
    
  for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++)
    {
      printf ("channel: %d \n", ch);

      if (self->no_diff_samples[ch] == 0)
        {
          printf ("    input channels match exactly\n");
        }
      else
        {
          printf ("    samples compared                              : %ld\n", wavefiles->m_samplesToCompare);
          printf ("    number of different samples                   : %lu\n", self->no_diff_samples[ch]);
            
          /*  Max. absolute difference sample             */
            
          if (self->m_exponential_output)
            {
              printf ("    max. abs. difference sample                   : 2 ^ %.3e = ", LD(self->maxabsdif_sample[ch] * scale_up));
              if (LD(self->maxabsdif_sample[ch] * scale_up) < 0)
                {
                  printf ("%.3e", self->maxabsdif_sample[ch] * scale_up);
                }
              else
                {
                  printf ("%.0f", self->maxabsdif_sample[ch] * scale_up);
                }
              printf (" (threshold: 2 ^ %.0f = %.0f, %d bits res.)", LD(ABSDIF_THRESHOLD(self->m_bits_resolution) * scale_up), ABSDIF_THRESHOLD(self->m_bits_resolution) * scale_up, self->m_bits_resolution);
            }
          else
            {
              printf ("    max. abs. difference sample                   : %.2e (threshold: %.2e, %d bits res.)", self->maxabsdif_sample[ch], ABSDIF_THRESHOLD(self->m_bits_resolution), self->m_bits_resolution);
            }

          if ((self->m_apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
            {
              if (self->maxabsdif_sample[ch] <= ABSDIF_THRESHOLD(self->m_bits_resolution))
                {
                  printf (" -> OK");
                }
              else
                {
                  printf (" -> FAIL");
                  full_acc_fail = TRUE;
                }
                
            }
           
          printf ("\n");

          /*  Overall RMS value of difference signal      */

          printf ("    overall RMS value of difference               : ");
          if (self->rms_dif[ch] > THRESHOLD_RMS_dB)
            {
              printf ("%.2f dB (threshold: %.2f dB, %d bits res.)", self->rms_dif[ch], RMS_THRESHOLD(self->m_bits_resolution), self->m_bits_resolution);
                
              if ((self->m_apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
                {
                  if (self->rms_dif[ch] < RMS_THRESHOLD(self->m_bits_resolution))
                    {
                      printf (" -> OK");
                    }
                  else
                    {
                      printf (" -> FAIL");
                      full_acc_fail = TRUE;
                    }
                }
            }
          else
            {
              printf ("--");
            }
          printf ("\n");
            
          /*  Average segmental SNR of difference signal  */
            
          printf ("    average segmental SNR value of difference     : ");
          if (self->ssnr[ch] > THRESHOLD_SNR_dB)
            {
              printf ("%.2f dB (%d samples per segment)", self->ssnr[ch], self->m_segment_length);
                
              if ((self->m_apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
                {
                  if (self->ssnr[ch] > self->m_seg_snr_threshold)
                    {
                      printf (" -> OK");
                    }
                  else
                    {
                      printf (" -> FAIL");
                      lim_acc_fail = TRUE;
                    }
                }
            }
          else
            {
              printf ("--");
            }
          printf ("\n");

          /*  Average cepstral distortion                 */

          printf ("    cepstral distortion                           : ");
          printf ("%.2f dB (%d samples per segment)", self->avg_CD[ch], self->m_segment_length);
            
          if ((self->m_apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
            {
              if (self->avg_CD[ch] < CEPSTRAL_DISTORTION_THRESHOLD)
                {
                  printf (" -> OK");
                }
              else
                {
                  printf (" -> FAIL");
                  lim_acc_fail = TRUE;
                }
            }    
          printf ("\n");
        }
    }


  /*   print overall result   */

  if ((self->m_apply_criteria & RMS_ABSMAXDIFSAMPLE_CRITERION) == RMS_ABSMAXDIFSAMPLE_CRITERION)
    {
      printf ("%s  Test on RMS and abs. max. difference sample  : %s\n", GREP_STRING, full_acc_fail ? "FAIL" : "OK");
    }

  if ((self->m_apply_criteria & SEGSNR_CD_CRITERION) == SEGSNR_CD_CRITERION)
    {
      printf ("%s  Test on seg. SNR and cepstral distortion     : %s\n", GREP_STRING, lim_acc_fail ? "FAIL" : "OK");
    }

  if ((self->m_apply_criteria & NR_BITS_ACCURACY_CRITERION) == NR_BITS_ACCURACY_CRITERION)
    {
      printf ("%s  Reached RMS criterion    for a maximum of    : %i bit\n", GREP_STRING, self->max_rms_accuracy);
      printf ("%s  Reached LSB criterion    for a maximum of    : %i bit\n", GREP_STRING, self->max_lsb_accuracy);
      printf ("%s  Reached RMS/LSB criteria for a maximum of    : %i bit\n", GREP_STRING, MIN(self->max_rms_accuracy, self->max_lsb_accuracy));
    }
    
  if (self->m_show_accuracy_distribution)
    {
      printf ("%s  Distribution of acuracy over all samples     :\n\n", GREP_STRING); 
      printf (" channel    :       ");
      for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++)
        {
          printf("%8i", ch); 
        }
      printf ("\n");
      printf ("--------------------");
      for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++)
        {
          printf("--------"); 
        }
      printf ("------------\n");
      printf (" difference :\n");
        
      for (accuracy = self->m_max_resolution + 2; accuracy >= 2; accuracy--) {
        if (accuracy < self->m_max_resolution + 2) {
          if (self->m_exponential_output) {
            printf ("  <= 2 ^ %7.0f :  ", LD(ABSDIF_THRESHOLD(accuracy) * scale_up));
          }
          else {
            printf ("  <= %.5e :  ", ABSDIF_THRESHOLD(accuracy));
          }
        }
        else {
          /* print no difference samples */
          if (self->m_exponential_output) {
            printf ("   = 0           :  ");
          }
          else {
            printf ("   = %.5e :  ", 0.0);
          }
        }
            
        for (ch = 0; ch < wavefiles->m_numberOfChannels; ch++) {
          printf("%8lu", self->distribution_list[((MAXIMUM_RESOLUTION + 1) * ch) + (accuracy - 2)]); 
        }
                        
        printf ("  samples\n");
        
        /* print threshold line */
        if (accuracy == self->m_bits_resolution) {
          printf("***\n");
        }
      }
    
    } /* self->m_show_accuracy_distribution */

  printf ("\n\n");

  if ( full_acc_fail ) status = SSNRCD_FULL_ACCURACY_ERROR;
  if ( lim_acc_fail )  status = SSNRCD_LIMITED_ACCURACY_ERROR;

  return status;
}

/***************************************************************************/
