/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_freqsca.c,v 1.2 2011-03-06 20:50:36 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  Frequency scale calculation $Revision: 1.2 $
*/
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ct_sbrconst.h"
#include "ct_sbrdecoder.h"
#include "ct_envextr.h"
#include "ct_freqsca.h"
#include "common_m4a.h"

#define MAX_OCTAVE        29
#define MAX_SECOND_REGION 50


static int getStartFreq(int fs, const int start_freq, float upsampleFac);
static int getStopFreq(int fs, const int stop_freq, float upsampleFac);
static int  numberOfBands(int b_p_o, int start, int stop, float warp_factor);
static void CalcBands(int * diff, int start , int stop , int num_bands);
static void modifyBands(int max_band, int * diff, int length);
static void cumSum(int start_value, int* diff, int length, int *start_adress);



/*******************************************************************************
 Functionname:  getStartFreq
 *******************************************************************************
 Description: 
  
 Arguments:   
 
 Return:      
 *******************************************************************************/
static int                      
getStartFreq(int fs, const int start_freq, float upsampleFac)
{
  int k0_min;
  int v_offset[]= {0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 28, 33};
  int fsMapped = 0;

  if (upsampleFac == 4) {
      fs = fs/2;
  }

  if(fs >= 0 && fs < 18783){
    fsMapped = 16000;
  } else if (fs >= 18783 && fs < 23004){
    fsMapped = 22050;
  } else if (fs >= 23004 && fs < 27713){
    fsMapped = 24000;
  } else if (fs >= 27713 && fs < 35777){
    fsMapped = 32000;
  } else if (fs >= 35777 && fs < 42000){
    fsMapped = 40000;
  } else if (fs >= 42000 && fs < 46009){
    fsMapped = 44100;
  } else if (fs >= 46009 && fs < 55426){
    fsMapped = 48000;
  } else if (fs >= 55426 && fs < 75132){
    fsMapped = 64000;
  } else if (fs >= 75132 && fs < 92017){
    fsMapped = 88200;
  } else if (fs >= 92017){
    fsMapped = 96000;
  } else {
    CommonExit(-1, "Invalid SBR frequency.");
  }
  
  if (upsampleFac == 4) {
    if(fsMapped < 32000) {
      k0_min=(int)(((float)(3000 * 2 * 32) / fsMapped)+0.5);
    } else {
      if (fsMapped < 64000) {
        k0_min=(int)(((float)(4000 * 2 * 32) / fsMapped)+0.5);
      } else {
        k0_min=(int)(((float)(5000 * 2 * 32) / fsMapped)+0.5);
      }
    }
  } else {
    if(fsMapped < 32000) {
      k0_min=(int)(((float)(3000 * 2 * 64) / fsMapped)+0.5);
    } else {
      if (fsMapped < 64000) {
        k0_min=(int)(((float)(4000 * 2 * 64) / fsMapped)+0.5);
      } else {
        k0_min=(int)(((float)(5000 * 2 * 64) / fsMapped)+0.5);
      }
    }
  }

  switch (fsMapped){
  case 16000:
    {
      int v_offset[]= {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 22050:
    {
      int v_offset[]= {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 24000:
    {
      int v_offset[]= {-5, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 32000:
    {
      int v_offset[]= {-6, -4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 40000:
    {
      int v_offset[]= {-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 11, 13, 15, 17, 19};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 44100:
  case 48000:
  case 64000:
    {
      int v_offset[]= {-4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20};
      return (k0_min + v_offset[start_freq]);
    }
    break;
  case 88200:
  case 96000:
    {
      int v_offset[]= {-2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24};
      return (k0_min + v_offset[start_freq]);
    }
    break;
    
  default:
    {
      int v_offset[]= {0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 28, 33};
      return (k0_min + v_offset[start_freq]);
    }
  }
}







/*******************************************************************************
 Functionname:  GetStopFreq
 *******************************************************************************
 Description: 
  
 Arguments:   
 
 Return:      
 *******************************************************************************/
static int
getStopFreq(int fs, const int stop_freq, float upsampleFac)
{
  int result,i;
  int v_stop_freq[14];
  int k1_min;
  int v_dstop[13];

  if (upsampleFac == 4) {
    fs = fs/2;
    if(fs < 32000) {
      k1_min = (int) ( ( (float) (6000 * 2 * 32) / fs ) + 0.5 );
    } else {
      if (fs < 64000) {
        k1_min = (int) ( ( (float) (8000 * 2 * 32) / fs ) + 0.5 );
      } else {
        k1_min = (int) ( ((float) (10000 * 2 * 32) / fs ) + 0.5);
      }
    }
  } else {
    if(fs < 32000) {
      k1_min = (int) ( ( (float) (6000 * 2 * 64) / fs ) + 0.5 );
    } else {
      if (fs < 64000) {
        k1_min = (int) ( ( (float) (8000 * 2 * 64) / fs ) + 0.5 );
      } else {
        k1_min = (int) ( ((float) (10000 * 2 * 64) / fs ) + 0.5);
      }
    }  
  }
  
  /*Calculate stop frequency vector*/
  for( i=0; i<=13; i++) {
    v_stop_freq[i]= (int)( k1_min * pow( 64.0 / k1_min, i / 13.0) + 0.5);
  }
  
  
  /*Ensure increasing bandwidth */
  for( i=0; i<=12; i++) {
    v_dstop[i] = v_stop_freq[i+1] - v_stop_freq[i];
  }

  shellsort(v_dstop, 13); /*Sort bandwidth changes */
    
  result=k1_min;
  for( i=0; i<stop_freq; i++) {
    result = result + v_dstop[i];
  }
  
  return(result);
  
}







void
sbrdecFindStartAndStopBand(const int samplingFreq,
                           const int startFreq,
                           const int stopFreq,
                           float upsampleFac,
                           int *lsbM,
                           int *usb)
{
  /* Update startFreq struct */
  *lsbM = getStartFreq(samplingFreq, startFreq, upsampleFac);

  
  /*Update stopFreq struct */
  if ( stopFreq < 14 ) { 
    *usb = getStopFreq(samplingFreq, stopFreq, upsampleFac);
  } else if( stopFreq == 14 ) {
    *usb = 2 * *lsbM;
  } else {
    *usb = 3 * *lsbM;
  }
  
  /* limit to Nyqvist */
  if (*usb > 64) {
    *usb = 64;
  }

  /* test for invalid lsb, usb combinations */
  if (upsampleFac == 4) {
    if ( (*usb - *lsbM) > 56 )
      /* XXX */
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */
  
    if ( (samplingFreq == 44100) && ( (*usb - *lsbM) > 56 ) )
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */

    if ( (samplingFreq >= 48000) && ( (*usb - *lsbM) > 56 ) )
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */
  } else {
    if ( (*usb - *lsbM) > 48 )
      /* XXX */
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */
  
    if ( (samplingFreq == 44100) && ( (*usb - *lsbM) > 35 ) )
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */

    if ( (samplingFreq >= 48000) && ( (*usb - *lsbM) > 32 ) )
      exit(0);
      /* myexit ("invalid SBR bitstream ?"); */
  }
  
}





/*******************************************************************************
 Functionname:  sbrdecUpdateFreqScale
 *******************************************************************************
 Description: 
  
 Arguments:   
 
 Return:      
 *******************************************************************************/
void
sbrdecUpdateFreqScale(int * v_k_master, int *h_num_bands,
                      const int lsbM, const int usb,
                      const int freqScale,
                      const int alterScale,
                      const int channelOffset,
                      float upsampleFac)
{
  int i, numBands = 0, numBands2;
    
  if(freqScale > 0){            /*Bark mode*/
    int reg,regions,b_p_o;
    int k[3];
    int d[MAX_SECOND_REGION];
    float w[2] = {1.0f, 1.0f};

    k[0] = lsbM;
    k[1] = usb;
    k[2] = usb;
    b_p_o = (freqScale==1)  ? 12 : 8;
    b_p_o = (freqScale==2)  ? 10 : b_p_o;
    w[1]  = (alterScale==0) ? 1.0f : 1.3f;

    if( (upsampleFac == 4) && (lsbM < b_p_o) ){
        b_p_o = ((int)(lsbM/2))*2;
    }

    if(((float)usb/lsbM) > 2.2449){
      regions = 2;
      k[1] = 2*lsbM;
    }
    else
      regions = 1;

    *h_num_bands = 0;
    for(reg = 0; reg < regions; reg++){
      int d2[MAX_SECOND_REGION];
      if (reg == 0){
        numBands = numberOfBands(b_p_o, k[reg], k[reg+1], w[reg]);
        CalcBands(d, k[reg], k[reg+1], numBands);                            /* CalcBands => d   */
        shellsort( d, numBands);                                             /* SortBands sort d */
        cumSum(k[reg] - channelOffset,d,numBands,v_k_master+*h_num_bands);   /* cumsum */
        *h_num_bands += numBands;                                            /* Output nr of bands */
      }
      else{
        numBands2 = numberOfBands(b_p_o, k[reg], k[reg+1], w[reg]);
        CalcBands(d2, k[reg], k[reg+1], numBands2);                           /* CalcBands => d   */
        shellsort(d2, numBands2);                                             /* SortBands sort d */
        if (d[numBands-1] > d2[0]){
          modifyBands(d[numBands-1],d2, numBands2);
        }
        cumSum(k[reg] - channelOffset,d2,numBands2,v_k_master+*h_num_bands);   /* cumsum */
        *h_num_bands += numBands2;                                           /* Output nr of bands */
      }
    }
  }
  else{                         /* Linear mode */
    int     dk,k2_achived,k2_diff;
    int     diff_tot[MAX_OCTAVE + MAX_SECOND_REGION];
    int     incr =0;
    
    dk = (alterScale==0) ? 1:2;
    numBands = 2 * (int) ( (float)(usb - lsbM)/(dk*2) + (-1+dk)/2.0f ); 
    
    k2_achived = lsbM + numBands*dk;
    k2_diff = usb - k2_achived;

    for(i=0;i<numBands;i++)
      diff_tot[i] = dk;

    if (k2_diff < 0){   /* If linear scale wasn't achived */
        incr = 1;       /* and we got too large SBR area */
        i = 0;
    }
      
    if (k2_diff > 0) {        /* If linear scale wasn't achived */
        incr = -1;            /* and we got too small SBR area */
        i = numBands-1;           
    }

    /* Adjust diff vector to get spec. SBR range */
    while (k2_diff != 0) {
      diff_tot[i] = diff_tot[i] - incr;
      i = i + incr;
      k2_diff = k2_diff + incr;
    }

    cumSum(lsbM, diff_tot, numBands, v_k_master); /* cumsum */
    *h_num_bands=numBands;                      /* Output nr of bands */
  }
#ifndef USAC_REFSOFT_COR1_NOFIX_05
  /*flx 2014-06-18*/
  /*requirement to avoid endless loop in HF-Generator*/
  if(v_k_master[*h_num_bands]-v_k_master[*h_num_bands-1]>lsbM-2) {
    exit(0);
  }
#endif
}






static int 
numberOfBands(int b_p_o, int start, int stop, float warp_factor)
{
  int result=0;

  result = 2* (int) ( b_p_o * log( (float)(stop)/start) / (2*log(2.0)*warp_factor) +0.5);
  
  return(result);
}  /* End numberOfBands */







static void
CalcBands(int * diff, int start , int stop , int num_bands)
{
  int i;
  int previous;
  int current;
  
  previous=start;
  for(i=1; i<= num_bands; i++)
    {
      /*              float temp=(start * pow( (float)stop/start, (float)i/num_bands)); */
      current=(int) ( (start * pow( (float)stop/start, (float)i/num_bands)) +0.5);
      diff[i-1]=current-previous;
      previous=current;
    }
  
}  /* End CalcBands */


static void 
cumSum(int start_value, int* diff, int length, int *start_adress)
{
  int i;
  start_adress[0]=start_value;
  for(i=1;i<=length;i++)
    start_adress[i]=start_adress[i-1]+diff[i-1];
}  /* End cumSum */


static void
modifyBands(int max_band_previous, int * diff, int length)
{
  int change=max_band_previous-diff[0];

  /* Limit the change so that the last band cannot get narrower than the first one */
  if ( change > (diff[length-1] - diff[0]) / 2 )
    change = (diff[length-1] - diff[0]) / 2;

  diff[0] += change;
  diff[length-1] -= change;
  shellsort(diff, length);
}  /* End modifyBands */


/*******************************************************************************
 Functionname:  UpdateHiRes
 *******************************************************************************
 Description: 
  
 Arguments:   
 
 Return:      
 *******************************************************************************/

void
sbrdecUpdateHiRes(int *h_hires, int *num_hires,int *v_k_master, int num_bands , int xover_band)
{
  int i;
  
  *num_hires = num_bands - xover_band;
  if(*num_hires < 0)
    CommonExit(1, "sbrdecUpdateHiRes: invalid band count");

  for(i = xover_band; i <= num_bands; i++) {
    
    h_hires[i-xover_band] = v_k_master[i];
  
  }
  
}  /* End UppdateHiRes */






/*******************************************************************************
 Functionname:  UppdateLoRes
 *******************************************************************************
 Description: 
  
 Arguments:   
 
 Return:      
 *******************************************************************************/
void                    
sbrdecUpdateLoRes(int *h_lores, int *num_lores, int *h_hires, int num_hires)
{
  int i;
  
  if(num_hires % 2 == 0) { /* if even number of hires bands */
  
    *num_lores = num_hires / 2;
    /* Use every second lores=hires[0,2,4...] */
    for(i = 0; i <= *num_lores; i++) {
      h_lores[i] = h_hires[2*i];
    }

  } else {            /* odd number of hires which means xover is odd */

    *num_lores = (num_hires + 1) / 2;
    /* Use lores=hires[0,1,3,5 ...] */
    h_lores[0] = h_hires[0];
    for(i = 1; i <= *num_lores; i++) {
      h_lores[i] = h_hires[2*i-1];
    }
    
  }

}  /* End UpdateLoRes */






void 
sbrdecDownSampleLoRes(int *v_result, int num_result, int *freqBandTableRef ,int num_Ref)
{
  int step;
  int i,j;
  int org_length,result_length;
  int v_index[MAX_FREQ_COEFFS/2];
  
  /* init */
  org_length=num_Ref;
  result_length=num_result;
  
  v_index[0]=0;   /* Always use left border */
  i=0;
  while(org_length > 0)   /* Create downsample vector */
    {
      i++;
      step=org_length/result_length; /* floor; */
      org_length=org_length - step;
      result_length--;
      v_index[i]=v_index[i-1]+step;
    }
  
  for(j=0;j<=i;j++)       /* Use downsample vector to index LoResolution vector. */
    {
      v_result[j]=freqBandTableRef[v_index[j]];
    }
  
}  /* End downSampleLoRes */




/* Sorting routine */
void shellsort(int *in, int n)
{

  int i, j, v;
  int inc = 1;

  do
    inc = 3 * inc + 1;
  while (inc <= n);

  do {
    inc = inc / 3;
    for (i = inc + 1; i <= n; i++) {
      v = in[i-1];
      j = i;
      while (in[j-inc-1] > v) {
        in[j-1] = in[j-inc-1];
        j -= inc;
        if (j <= inc)
          break;
      }
      in[j-1] = v;
    }
  } while (inc > 1);

}
