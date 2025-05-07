/*

  This software module was originally developed by 

  Martin Weishart (Fraunhofer IIS, Germany)

  and edited by 

  Ralph Sperschneider (Fraunhofer IIS, Germany)

  in the course of  development of the MPEG-2/MPEG-4 Conformance ISO/IEC
  13818-4, 14496-4. This software module is  an implementation of one or
  more of  the Audio Conformance  tools  as specified  by  MPEG-2/MPEG-4
  Conformance. ISO/IEC gives users     of the MPEG-2    AAC/MPEG-4 Audio
  (ISO/IEC  13818-3,7, 14496-3) free license to  this software module or
  modifications thereof for use in hardware  or software products. Those
  intending to use this software module in hardware or software products
  are advised that  its use may  infringe existing patents. The original
  developer of this software  module and his/her company, the subsequent
  editors and their companies, and ISO/IEC  have no liability for use of
  this      software   module    or   modifications      thereof   in an
  implementation. Philips retains full right to use the code for his/her
  own  purpose,  assign or   donate  the  code  to  a third  party. This
  copyright  notice  must  be included   in   all copies  or  derivative
  works. Copyright 2000, 2009.

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "libtsp.h"

#include "common.h"
#include "pns_common.h"
#include "tables.h"
#include "audiofile_io.h"
#include "pns.h"
#include "transfo.h"


#define         SFB_1               51
#define         SFB_2               15

#define         PNS_S_C1            0.4
#define         PNS_S_C2            0.8
#define         PNS_T_C1            91.0
#define         PNS_T_C2            99.0

enum {
  BLOCK_LENGTH_FL1024  =  64,
  BLOCK_LENGTH_FL0960  =  64,
  BLOCK_LENGTH_INVALID =   0
};

typedef enum {
  WINDOW_SIZE_LONG__FOR_AAC_FL1024  =  2048,
  WINDOW_SIZE_LONG__FOR_AAC_FL0960  =  1920,
  WINDOW_SIZE_LONG__FOR_AAC_FL0512  =  1024,
  WINDOW_SIZE_LONG__FOR_AAC_FL0480  =   960,

  WINDOW_SIZE_SHORT_FOR_AAC_FL1024  =   256,
  WINDOW_SIZE_SHORT_FOR_AAC_FL0960  =   240,

  WINDOW_SIZE_INVALID               =     0
} WINDOW_SIZE;


/*****************************************************************************/

/* critical_ratio for pns-test   */
typedef struct 
{
  Float                   m_meanRatio;
  Float                   m_standardDeviationRatio;
} critical_ratio;


/***************************************************************************/
struct pns_obj
{
  int                     m_pns_test;
  int                     m_pns_frameSize;

  /* spectral conformance */
  WINDOW_SIZE             m_pns_windowSize;

  /* temporal Conformance */
  const short int*        m_sfboff;
  unsigned int            m_sfbsize;
  int                     m_blockLength;
};


/*****************************************************************************/
/*                                                                           */
/* Conformance 1 and 2 : Spectral Criterion                                  */
/*                                                                           */
/*****************************************************************************/

static ERRORCODE calculateEnergyOfFrame( double*      refBuffer,
                                         double*      estBuffer, 
                                         Float*       refSquaredAbsValue,
                                         Float*       estSquaredAbsValue,
                                         unsigned int windowSize,
                                         int          numberOfChannels,
                                         int          channel)
{

  Float* refBufferTmp = (Float*) calloc(2*windowSize, sizeof(Float)); 
  Float* estBufferTmp = (Float*) calloc(2*windowSize, sizeof(Float)); 

  Float* hannwin;
 
  unsigned int i;

  ERRORCODE status = OKAY;

  switch (windowSize) {
  case WINDOW_SIZE_LONG__FOR_AAC_FL1024:
    hannwin = &hannwin_2048[0];
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0960:
    hannwin = &hannwin_1920[0];
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0512:
    hannwin = &hannwin_1024[0];
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0480:
    hannwin = &hannwin_960[0];
    break;
  case WINDOW_SIZE_SHORT_FOR_AAC_FL1024:
    hannwin = &hannwin_256[0];
    break;
  case WINDOW_SIZE_SHORT_FOR_AAC_FL0960:
    hannwin = &hannwin_240[0];
    break;
  default:
    fprintf(stderr,"cannot handle windowSize\n\n");
    status = PNS_WINDOWSIZE_ERROR;
    goto bail;
  }
  
  /* multiply each time value with the window shape  */
  
  for (i = 0; i< windowSize; i++) {
#if ROUNDINGTO16BIT
    refBuffer[(i * numberOfChannels) + channel] = (int)refBuffer[(i * numberOfChannels) + channel]+1;
    estBuffer[(i * numberOfChannels) + channel] = (int)estBuffer[(i * numberOfChannels) + channel]+1;
#endif
    refBufferTmp[2*i]   = refBuffer[(i * numberOfChannels) + channel] * hannwin[i]; 
    estBufferTmp[2*i]   = estBuffer[(i * numberOfChannels) + channel] * hannwin[i];  
    refBufferTmp[2*i+1] = 0.0f;
    estBufferTmp[2*i+1] = 0.0f;
  }

  /* calculate fft of previously windowed values   */

  status = CompFFT((Float*)refBufferTmp, windowSize, 1);
  if (OKAY != status ) goto bail;
  status = CompFFT((Float*)estBufferTmp, windowSize, 1);
  if (OKAY != status ) goto bail;

  /* calculate energy of values, only half of the complex spectrum is of
     interest */

  for (i = 0; i< windowSize/2; i++){

    refSquaredAbsValue[i] 
      = refBufferTmp[2*i]   * refBufferTmp[2*i] 
      + refBufferTmp[2*i+1] * refBufferTmp[2*i+1];

    estSquaredAbsValue[i] 
      = estBufferTmp[2*i]   * estBufferTmp[2*i] 
      + estBufferTmp[2*i+1] * estBufferTmp[2*i+1];
  }

 bail:
  free (refBufferTmp);
  free (estBufferTmp);

  return status;
}  

/*****************************************************************************/

static ERRORCODE accumulateFrameEnergiesInSfb( double*      refBuffer,
                                               double*      estBuffer, 
                                               Float        est_sfb_sum[SFB_1], 
                                               Float        ref_sfb_sum[SFB_1],
                                               unsigned int windowSize,
                                               int          numberOfChannels,
                                               int          channel,
                                               pns_ptr      self)
{

  Float* refSquaredAbsValue = (Float*) calloc(windowSize/2,sizeof(Float));
  Float* estSquaredAbsValue = (Float*) calloc(windowSize/2,sizeof(Float));

  int          k;
  unsigned int i;
  ERRORCODE    status = OKAY;

  status = calculateEnergyOfFrame( refBuffer, 
                                   estBuffer, 
                                   refSquaredAbsValue, 
                                   estSquaredAbsValue, 
                                   windowSize,
                                   numberOfChannels,
                                   channel);

  /* for each sfb  */
  /*    accumulate sum of each of the fft values  */

  for (i = 0; i < self->m_sfbsize; i++) {
    ref_sfb_sum[i] = 0.0;
    est_sfb_sum[i] = 0.0;
    for (k = self->m_sfboff[i]; k < self->m_sfboff[i+1]; k++) {
      ref_sfb_sum[i] += refSquaredAbsValue[k];
      est_sfb_sum[i] += estSquaredAbsValue[k];
      /*       printf("sfb %2i: %4i - %4i\n",i,sfboff[i],sfboff[i+1]-1); */
    }
  }
  free (refSquaredAbsValue);
  free (estSquaredAbsValue);
 
  return status;
}

/*****************************************************************************/

static Float calculateStandardDeviation( Float          vector[],
                                         unsigned int   maxValue)
{
    
  Float mean = 0.0;
  unsigned int i;

  Float standardDeviation = 0.0;

  for ( i = 0; i < maxValue; i++) {
    mean += vector[i] / (maxValue);
  }
  for ( i = 0; i < maxValue; i++) {
    standardDeviation += (vector[i] - mean) * (vector[i] - mean) / (maxValue-1);
  }
  standardDeviation = sqrt(standardDeviation);
  
  return standardDeviation;
}

/*****************************************************************************/

static Float calculateMeanValue( Float          vector[],
                                 unsigned int   maxValue)
{

  Float sum = 0.0;
  unsigned int i;

  for ( i = 0; i < maxValue; i++) {
    sum += vector[i];
  }
  sum = sum / (maxValue);
  return sum;
}


/*****************************************************************************/

static critical_ratio calculateCriticalRatio( Float*          est_sfb_sum, 
                                              Float*          ref_sfb_sum, 
                                              unsigned int    lastFrame,
                                              int             channel,
                                              int             sfbnum,
                                              unsigned int*   wrongRatioCount,
                                              int             verbose)
{
    
  Float est_sum = 0.0 , ref_sum = 0.0 ;
  critical_ratio cRatio = {0.0,0.0} ;

  /* criterion 1 */

  est_sum = calculateMeanValue(est_sfb_sum, lastFrame);
  ref_sum = calculateMeanValue(ref_sfb_sum, lastFrame);

  if ( ref_sum != 0 ) {
    cRatio.m_meanRatio = 10.0f*log10(est_sum / ref_sum) ;
  }

  if (verbose || ( cRatio.m_meanRatio< -PNS_S_C1 || cRatio.m_meanRatio > PNS_S_C1) ) {
    fprintf(stderr, "\nchan %d, sfb %2i, ",channel, sfbnum);  
    fprintf(stderr, "C1: %6.2fdB", cRatio.m_meanRatio);
    fprintf(stderr, ", range: [%3.1fdB;%3.1fdB]",-PNS_S_C1,PNS_S_C1);
  }

  if ( cRatio.m_meanRatio< -PNS_S_C1 || cRatio.m_meanRatio > PNS_S_C1) {
    (*wrongRatioCount)++ ;
    fprintf ( stderr, " -> range exceeded !");
  }

  /* criterion 2 */
  
  est_sum = calculateStandardDeviation(est_sfb_sum, lastFrame );
  ref_sum = calculateStandardDeviation(ref_sfb_sum, lastFrame );

  if ( ref_sum != 0 ) {
    cRatio.m_standardDeviationRatio =  10.0f*log10(est_sum / ref_sum) ;
  }

  if (verbose || ( cRatio.m_standardDeviationRatio< -PNS_S_C2 || cRatio.m_standardDeviationRatio > PNS_S_C2) ) {
    fprintf(stderr, "\nchan %d, sfb %2i, ",channel, sfbnum);  
    fprintf(stderr, "C2: %6.2fdB", cRatio.m_standardDeviationRatio);
    fprintf(stderr, ", range: [%3.1fdB;%3.1fdB]",-PNS_S_C2,PNS_S_C2);
  }

  if ( cRatio.m_standardDeviationRatio < -PNS_S_C2 || cRatio.m_standardDeviationRatio > PNS_S_C2 ) {
    (*wrongRatioCount)++ ;
    fprintf ( stderr, " -> range exceeded !");
    
  }
  return cRatio;
}

/*****************************************************************************/

static void calculateSpectralConformance( Float***       est_sfb_sum, 
                                          Float***       ref_sfb_sum,
                                          int            numberOfChannels,
                                          unsigned int   lastFrame,
                                          unsigned int*  wrongRatioCount,
                                          int            verbose,
                                          unsigned int   sfbsize)
{

  Float** est_sum = (Float**) calloc(numberOfChannels, sizeof(Float*)); 
  Float** ref_sum = (Float**) calloc(numberOfChannels, sizeof(Float*)); 
  unsigned int   i, k;
  critical_ratio cRatio;
  int            channel;

  for ( channel = 0; channel < numberOfChannels; channel++ ) {
    est_sum[channel] = (Float*) calloc(lastFrame, sizeof(Float)); 
    ref_sum[channel] = (Float*) calloc(lastFrame, sizeof(Float)); 
  }

  for ( channel = 0; channel < numberOfChannels; channel++ ) {
    for ( k = 0; k < sfbsize ; k++) {
      for ( i = 0; i < lastFrame; i++) {
        est_sum[channel][i] = est_sfb_sum[channel][i][k];
        ref_sum[channel][i] = ref_sfb_sum[channel][i][k];
      }
      cRatio = calculateCriticalRatio( est_sum[channel],
                                       ref_sum[channel],
                                       lastFrame,
                                       channel,
                                       k,
                                       &(wrongRatioCount[channel]),
                                       verbose );

      if (0) {
        fprintf ( stderr, "\nscale factor band %d has critical ratio:  %f", k, cRatio.m_meanRatio);
        fprintf ( stderr, "\nscale factor band has standard deviation critical ratio:  %f", cRatio.m_standardDeviationRatio);
      }
    }
  }

  for ( channel = 0; channel < numberOfChannels; channel++ ) {
    free(est_sum[channel]);
    free(ref_sum[channel]); 
  }
  free (est_sum);
  free (ref_sum);
}

/*****************************************************************************/

static ERRORCODE CalculateSpectralConformance(WAVEFILES* wavefiles,
                                              pns_ptr    self,
                                              int        verbose)
{
  Float***      ref_sfb_sum = NULL;
  Float***      est_sfb_sum = NULL;
  unsigned int* wrongRatioCount;
  double*       estBuffer;
  double*       refBuffer;
  long int      j = 0, k;
  long int      block;
  long int      offset;
  int           ch;
  ERRORCODE     status = OKAY;

  unsigned int windowSize = self->m_pns_windowSize;
  long     int numBlocks  = 2 * wavefiles->m_samplesToCompare / windowSize; /* factor 2 due to 50% overlap */
  
  /* sanity check for unsuported sampling frequencies */
  k=-1;
  while ((samp_rate_info[++k].samp_rate != wavefiles->m_samplingFrequency)) {
    if (samp_rate_info[k].samp_rate == 0) {
      fprintf(stderr,"cannot handle sampling rate %f\n", wavefiles->m_samplingFrequency);
      return SAMPLING_RATE_ERROR;
    }
  }

  /* fprintf(stderr,"detected sampling rate: %5i Hz\n",samp_rate_info[k].samp_rate); */
  switch (windowSize) {
  case WINDOW_SIZE_LONG__FOR_AAC_FL1024:
    self->m_sfboff  = samp_rate_info[k].SFbands1024;
    self->m_sfbsize = samp_rate_info[k].nsfb1024;
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0960:
    self->m_sfboff  = samp_rate_info[k].SFbands960;
    self->m_sfbsize = samp_rate_info[k].nsfb960;
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0512:
    self->m_sfboff = samp_rate_info[k].SFbands512;
    self->m_sfbsize = samp_rate_info[k].nsfb512;
    break;
  case WINDOW_SIZE_LONG__FOR_AAC_FL0480:
    self->m_sfboff = samp_rate_info[k].SFbands480;
    self->m_sfbsize = samp_rate_info[k].nsfb480;
    break;
  case WINDOW_SIZE_SHORT_FOR_AAC_FL1024:
    self->m_sfboff  = samp_rate_info[k].SFbands128;
    self->m_sfbsize = samp_rate_info[k].nsfb128;
    break;
  case WINDOW_SIZE_SHORT_FOR_AAC_FL0960:
    self->m_sfboff  = samp_rate_info[k].SFbands120;
    self->m_sfbsize = samp_rate_info[k].nsfb120;
    break;
  default:
    fprintf(stderr,"cannot handle windowSize\n\n");
    return PNS_WINDOWSIZE_ERROR;
  }

  ref_sfb_sum     = (Float***)     calloc(wavefiles->m_numberOfChannels, sizeof(Float*));
  est_sfb_sum     = (Float***)     calloc(wavefiles->m_numberOfChannels, sizeof(Float*));
  wrongRatioCount = (unsigned int*)calloc(wavefiles->m_numberOfChannels, sizeof(unsigned int*));

  for ( ch = 0; ch < wavefiles->m_numberOfChannels; ch++ ) {
    ref_sfb_sum[ch] = (Float**)calloc(numBlocks, sizeof(Float*));
    est_sfb_sum[ch] = (Float**)calloc(numBlocks, sizeof(Float*));

    for ( block = 0; block < numBlocks; block++ ) {
      ref_sfb_sum[ch][block] = (Float*)calloc(SFB_1,sizeof(Float));
      est_sfb_sum[ch][block] = (Float*)calloc(SFB_1,sizeof(Float));
    }
  }

  refBuffer = (double*) calloc(windowSize * wavefiles->m_numberOfChannels, sizeof(double));
  estBuffer = (double*) calloc(windowSize * wavefiles->m_numberOfChannels, sizeof(double));

  for ( offset = 0; offset < (int)(wavefiles->m_samplesToCompare) - ((int)windowSize/2) ; offset += (int)(windowSize/2) ) {
    /*fprintf(stderr,"%d %d ",i, wavefiles->m_samplesToCompare);*/
    
    Waveform_GetSample(refBuffer,
                       wavefiles->m_waveformReference.m_stream,
                       wavefiles->m_numberOfChannels,
                       offset + wavefiles->m_waveformReference.m_offset,
                       windowSize);

    Waveform_GetSample(estBuffer,
                       wavefiles->m_waveformUnderTest.m_stream,
                       wavefiles->m_numberOfChannels,
                       offset + wavefiles->m_waveformUnderTest.m_offset,
                       windowSize);

    for ( ch = 0; ch < wavefiles->m_numberOfChannels; ch++ ) {
      status = accumulateFrameEnergiesInSfb(refBuffer,
                                            estBuffer,
                                            &est_sfb_sum[ch][j][0], 
                                            &ref_sfb_sum[ch][j][0], 
                                            windowSize, 
                                            wavefiles->m_numberOfChannels,
                                            ch,
                                            self);
      if (OKAY != status ) goto bail;
    }

    j++;
  }
  calculateSpectralConformance(est_sfb_sum,
                               ref_sfb_sum,
                               wavefiles->m_numberOfChannels,
                               numBlocks,
                               wrongRatioCount,
                               verbose, 
                               self->m_sfbsize);
  for ( ch = 0; ch < wavefiles->m_numberOfChannels; ch++ ) {
    fprintf(stderr,"\n number of sfb with wrong ratio in channel %u: %d\n", ch, wrongRatioCount[ch]);

    if ( wrongRatioCount[ch] == 0 ) {
      fprintf(stderr,"\n channel %u: result ok \n", ch);
    }
    else {
      fprintf(stderr,"\n channel %u: result wrong \n", ch);
      status = PNS_SPECTRALCONFORMANCE_ERROR;
    }
  }

 bail:
  for ( ch = 0; ch < wavefiles->m_numberOfChannels; ch++ ) {
    for ( block = 0; block < numBlocks; block++){
      free (ref_sfb_sum[ch][block]);
      free (est_sfb_sum[ch][block]);
    }
  }
  for ( ch = 0; ch < wavefiles->m_numberOfChannels; ch++ ) {
    free(ref_sfb_sum[ch]);
    free(est_sfb_sum[ch]);
  }
  free (wrongRatioCount);
  free (ref_sfb_sum);
  free (est_sfb_sum);
  free (refBuffer);
  free (estBuffer);

  /*  return self->m_wrongRatioCount; */
  return status;
}



/*****************************************************************************/
/*                                                                           */
/* Conformance 3: Temporal Criterium                                         */
/*                                                                           */
/*****************************************************************************/


static ERRORCODE calculateTemporalConformance( int           verbose,
                                               Float*        ConformanceEnergy3_ref,
                                               Float*        ConformanceEnergy3_est,
                                               unsigned int  numBlocks,
                                               int           channel)
{
  
  Float conf_crit;

  Float percent_5  = 0.0;
  Float percent_10 = 0.0;

  unsigned int block = 0; 

  unsigned int percent_count_5  = 0;
  unsigned int percent_count_10 = 0;

  for ( block = 0 ; block < numBlocks; block++) {
    conf_crit = 10.0 * log10(ConformanceEnergy3_est[block] / ConformanceEnergy3_ref[block]);
    
    if ( fabs(conf_crit ) >  5 )   percent_count_5++;
    if ( fabs(conf_crit ) > 10 )   percent_count_10++;
  }

  percent_5  = 100.0-((Float)percent_count_5  / (Float)numBlocks*100.0) ;
  percent_10 = 100.0-((Float)percent_count_10 / (Float)numBlocks*100.0) ;

  if ( (verbose) || ( percent_5 < PNS_T_C1 ) ) {
    fprintf(stderr,"\nchan %d, C1: %3.1f%%", channel, percent_5);
    fprintf(stderr,", threshold: >=%4.1f%%", PNS_T_C1);
  }
  if ( percent_5 < PNS_T_C1 ) {
    fprintf(stderr," -> threshold exceeded !");
  }
  if ( (verbose) || ( percent_10 < PNS_T_C2 ) ) {
    fprintf(stderr," \nchan %d, C2: %3.1f%%", channel, percent_10); 
    fprintf(stderr,", threshold: >=%4.1f%%", PNS_T_C2);
  }
  if ( percent_10 < PNS_T_C2 ) {
    fprintf(stderr," -> threshold exceeded !");
  }

  if ( ( percent_5 >= PNS_T_C1 )  &&  ( percent_10 >= PNS_T_C2 ) ) {
    fprintf(stderr,"\n\n result ok \n\n");
    return OKAY;
  }
  else {
    fprintf(stderr,"\n result wrong \n\n");
    return  PNS_TEMPORAL_CONFORMANCE_ERROR;
  }
}

/*****************************************************************************/

static ERRORCODE CalculateTemporalConformance(WAVEFILES* wavefiles,
                                              pns_ptr    self,
                                              int        verbose)
{
  ERRORCODE     status = OKAY;

  double*       refBuffer;
  double*       estBuffer;

  Float**       ConformanceEnergy3_ref = NULL;
  Float**       ConformanceEnergy3_est = NULL;

  long int      numBlocks;
  int           sample;
  int           channel;
  int           block;
  
  numBlocks = wavefiles->m_samplesToCompare / self->m_blockLength;

  refBuffer = (double*) calloc(self->m_blockLength * wavefiles->m_numberOfChannels, sizeof(double));
  estBuffer = (double*) calloc(self->m_blockLength * wavefiles->m_numberOfChannels, sizeof(double));
  
  ConformanceEnergy3_ref = (Float**) calloc(wavefiles->m_numberOfChannels,sizeof(Float*));
  ConformanceEnergy3_est = (Float**) calloc(wavefiles->m_numberOfChannels,sizeof(Float*));

  for (channel = 0; channel < wavefiles->m_numberOfChannels; channel++) {
    ConformanceEnergy3_ref[channel] = (Float*) calloc(numBlocks,sizeof(Float));
    ConformanceEnergy3_est[channel] = (Float*) calloc(numBlocks,sizeof(Float));
  }

  for ( block = 0; block < numBlocks; block++ ) {
      
    Waveform_GetSample(refBuffer,
                       wavefiles->m_waveformReference.m_stream,
                       wavefiles->m_numberOfChannels,
                       block * self->m_blockLength - (wavefiles->m_delay<0?wavefiles->m_delay:0),
                       self->m_blockLength);

    Waveform_GetSample(estBuffer,
                       wavefiles->m_waveformUnderTest.m_stream,
                       wavefiles->m_numberOfChannels,
                       block * self->m_blockLength + (wavefiles->m_delay>0?wavefiles->m_delay:0),
                       self->m_blockLength);

    for (channel = 0; channel < wavefiles->m_numberOfChannels; channel++) {

#if ROUNDINGTO16BIT
      for ( sample = 0; sample < self->blockLength; sample++ ) {
        refBuffer[(sample * wavefiles->m_numberOfChannels) + channel] 
          = (int)refBuffer[(sample * wavefiles->m_numberOfChannels) + channel]+1;

        estBuffer[(sample * wavefiles->m_numberOfChannels) + channel] 
          = (int)estBuffer[(sample * wavefiles->m_numberOfChannels) + channel]+1;
      }
#endif
      
      for ( sample = 0; sample < self->m_blockLength; sample++) {
        ConformanceEnergy3_ref[channel][block]
          += refBuffer[(sample * wavefiles->m_numberOfChannels) + channel] 
          *  refBuffer[(sample * wavefiles->m_numberOfChannels) + channel];
        
        ConformanceEnergy3_est[channel][block] 
          += estBuffer[(sample * wavefiles->m_numberOfChannels) + channel]
          *  estBuffer[(sample * wavefiles->m_numberOfChannels) + channel];
      }
    }
  }

  for (channel = 0; channel < wavefiles->m_numberOfChannels; channel++) {
    status= calculateTemporalConformance(verbose,
                                         ConformanceEnergy3_ref[channel], 
                                         ConformanceEnergy3_est[channel],
                                         numBlocks,
                                         channel);
  }
  
  free(refBuffer);
  free(estBuffer);

  for (channel = 0; channel < wavefiles->m_numberOfChannels; channel++) {
    free(ConformanceEnergy3_ref[channel]);
    free(ConformanceEnergy3_est[channel]);
  }

  free(ConformanceEnergy3_ref);
  free(ConformanceEnergy3_est);

  return status;
}


/*****************************************************************************/
/*                                                                           */
/* public functions                                                          */
/*                                                                           */
/*****************************************************************************/

pns_ptr Pns_Init (void)
{
  pns_ptr self;
  
  /* allocate memory */
  if(! ( self = (pns_ptr) calloc(1,sizeof(struct pns_obj))) )
    return NULL;
  
    
  /* default values */
  self->m_pns_test        =   -1;
  self->m_pns_windowSize  =   WINDOW_SIZE_INVALID;
  
  return self;
}


/*****************************************************************************/

void Pns_Free (pns_ptr self)
{
  if(NULL != self ) {
    free(self);
  }
}

/*****************************************************************************/

void Pns_SetTest(pns_ptr self, 
                 int test) {
    
  self->m_pns_test = test;
}

/*****************************************************************************/

ERRORCODE Pns_EvaluateFrameSize(pns_ptr self, 
                                int     frameSize) 
{
  self->m_pns_frameSize = frameSize;

  switch (self->m_pns_test) {
  case 1:
    switch(frameSize) {
    case 1024:
      self->m_pns_windowSize = WINDOW_SIZE_LONG__FOR_AAC_FL1024;
      break;
    case 960:
      self->m_pns_windowSize = WINDOW_SIZE_LONG__FOR_AAC_FL0960;
      break;
    case 512:
      self->m_pns_windowSize = WINDOW_SIZE_LONG__FOR_AAC_FL0512;
      break;
    case 480:
      self->m_pns_windowSize = WINDOW_SIZE_LONG__FOR_AAC_FL0480;
      break;
    default:
      self->m_pns_windowSize = WINDOW_SIZE_INVALID;
      fprintf(stderr,"ERROR: cannot handle AAC frameSize: %i (possible frame sizes for test PNS-1: 480, 512, 960, 1024)\n", 
              frameSize);
      return PNS_SETWINDOWSIZE_ERROR;
    }
    break;

  case 2: 
    switch(frameSize) {
    case 1024:
      self->m_pns_windowSize = WINDOW_SIZE_SHORT_FOR_AAC_FL1024;
      break;
    case 960:
      self->m_pns_windowSize = WINDOW_SIZE_SHORT_FOR_AAC_FL0960;
      break;
    default:
      self->m_pns_windowSize = WINDOW_SIZE_INVALID;
      fprintf(stderr,"ERROR: cannot handle AAC frameSize: %i (possible frame sizes for test PNS-2/3: 1024, 960)\n", 
              frameSize);
      return PNS_SETWINDOWSIZE_ERROR;
    }
    break;

  case 3: 
    switch(frameSize) {
    case 1024:
      self->m_blockLength = BLOCK_LENGTH_FL1024;
      break;
    case 960:
      self->m_blockLength = BLOCK_LENGTH_FL0960;
      break;
    default:
      self->m_blockLength = BLOCK_LENGTH_INVALID;
      fprintf(stderr,"ERROR: cannot handle AAC frameSize: %i (possible frame sizes for test PNS-2/3: 1024, 960)\n", 
              frameSize);
      return PNS_SETWINDOWSIZE_ERROR;
    }
    break;
 
 default:
    fprintf(stderr,"ERROR: cannot handle PNS testmode %d\n",self->m_pns_test);
    return PNS_SETWINDOWSIZE_ERROR;
  }
  return OKAY;
}

/*****************************************************************************/

ERRORCODE Pns_ApplyConfTest( WAVEFILES*  wavefiles,
                             pns_ptr     self,
                             int         verbose)
{
  ERRORCODE status = OKAY;
  
  printf("running Test PNS-%d", 
         self->m_pns_test);
  
  switch (self->m_pns_test) {
  case 1:
  case 2: 
    /* Spectral Conformance */
    printf(" (analysis window size is %d)\n", 
           self->m_pns_windowSize);
  
    status = CalculateSpectralConformance(wavefiles,
                                          self,
                                          verbose);
    
    printf("%s  spectral PNS conformance with analysis window size %d  : %s\n\n", 
           GREP_STRING, 
           self->m_pns_windowSize, 
           status ? "FAIL" : "OK");
    break;
  case 3:
    /* Temporal Conformance */      
    printf("\n");
    status = CalculateTemporalConformance(wavefiles,
                                          self,
                                          verbose);
    
    printf("%s  temporal PNS conformance with analysis block length %d  : %s\n\n", 
           GREP_STRING, 
           self->m_blockLength, 
           status ? "FAIL" : "OK");
    break;
  default:
    printf ("ERROR: cannot handle PNS testmode: %d.\n", self->m_pns_test);
    status = PNS_TESTMODE_ERROR;
    break;
  }
  return status;
}

/*****************************************************************************/
