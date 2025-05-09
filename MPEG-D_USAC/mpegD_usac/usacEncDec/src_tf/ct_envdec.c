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

 $Id: ct_envdec.c,v 1.2.12.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  envelope decoding $Revision: 1.2.12.1 $
*/
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifdef SONY_PVC
#include "sony_pvcprepro.h"
#endif  /* SONY_PVC */

#include "ct_envdec.h"


static  float sfb_nrg_prev[MAX_NUM_ELEMENTS][8][MAX_FREQ_COEFFS];
static  float prevNoiseLevel[MAX_NUM_ELEMENTS][8][MAX_NUM_NOISE_VALUES];

#ifdef SONY_PVC_DEC
float LastEnvNoiseLevel[8][MAX_NUM_NOISE_VALUES];
int   nNfb_global;
#endif /* SONY_PVC_DEC */

static void
requantizeEnvelopeData (float* iEnvelope,
                        int nScaleFactors,
                        int nNoiseFactors,
                        int ampRes,
                        int coupling,
                        float* sbrNoiseFloorLevel);   

#ifdef SONY_PVC_DEC
static void
requantizeEnvelopeData_for_pvc (
                        int nNoiseFactors,
                        float* sbrNoiseFloorLevel
							,int nNfb
                        );  
#endif /* SONY_PVC_DEC */

static void
decodeEnvelope(float* iEnvelope,
               int offset,
               int* nSfb,
               int* domain_vec,
               int* frameInfo,
               int ch, int el);
static void
decodeNoiseFloorlevels (float* sbrNoiseFloorLevel,
                        int* frameInfo,
                        int nNfb,
                        int* domain_vec2,
                        int ch, int el);





/*******************************************************************************
 \function  indexLow2High
 *******************************************************************************
 \author    CTS AB
 \return    mapped index
*******************************************************************************/
static int indexLow2High(int offset,int index,int res)   
{
  if(res == LO){
    if (offset >= 0){
        if (index < offset)
          return(index);
        else
          return(2*index - offset);
    }
    else{
      offset = -offset;
      if (index < offset)
        return(2*index+index);
      else
        return(2*index + offset);
    }
  }
  else
    return(index);
}



/*******************************************************************************
 \function  mapLowResEnergyVal
 *******************************************************************************
 \author    CTS AB
*******************************************************************************/
static void mapLowResEnergyVal(float currVal,float* prevData,int offset,int index,      
                               int res)        
{
  if(res == LO){
    if (offset >= 0){
      if(index < offset)
          prevData[index] = currVal;
      else{
          prevData[2*index - offset] = currVal;
          prevData[2*index+1 - offset] = currVal;
      }
    }
    else
    {
      offset = -offset;
      if (index < offset){
          prevData[3*index] = currVal;
          prevData[3*index+1] = currVal;
          prevData[3*index+2] = currVal;
      }
      else{
          prevData[2*index + offset] = currVal;
          prevData[2*index + 1 + offset] = currVal;
      }
    }    
  }
  else
    prevData[index] = currVal;
}




/*******************************************************************************
 \function  decodeSbrData
 *******************************************************************************
  \brief    Convert raw envelope and noisefloor data to energy levels

  \author   CTS AB

*******************************************************************************/
void
decodeSbrData(float* iEnvelope,float* noiseLevel,int* domain_vec1,int* domain_vec2,
              int* nSfb,int* frameInfo,int nNfb,int ampRes,int nScaleFactors,
              int nNoiseFactors,int offset,int coupling,int ch, int el)
{
  decodeEnvelope(iEnvelope,offset,nSfb,domain_vec1,frameInfo,ch, el);
  decodeNoiseFloorlevels (noiseLevel,frameInfo,nNfb,domain_vec2,ch, el);
  if(!coupling){
    requantizeEnvelopeData( iEnvelope,nScaleFactors,nNoiseFactors,ampRes,coupling,noiseLevel);
  }
}


#ifdef SONY_PVC_DEC
void
decodeSbrData_for_pvc(float* noiseLevel,int* domain_vec2,
              int* frameInfo,int nNfb, int nNoiseFactors,int coupling, int ch, int el)
{

	decodeNoiseFloorlevels (noiseLevel,frameInfo,nNfb,domain_vec2,ch, el);
	if(!coupling){
		requantizeEnvelopeData_for_pvc( nNoiseFactors, noiseLevel
			,nNfb
			);
	}
}
#endif /* SONY_PVC_DEC */



/*******************************************************************************
 \function   sbr_envelope_unmapping
 *******************************************************************************
 \author     CTS AB
*******************************************************************************/
void
sbr_envelope_unmapping (float* iEnvelopeLeft,float* iEnvelopeRight,
                        float* noiseFloorLeft,float* noiseFloorRight,
                        int nScaleFactors,int nNoiseFactors, int ampRes)
{
  int i;
  float tempLeft, tempRight;
  float panOffset[2] = {24.0f, 12.0f};
  float a = 2.0f - ampRes;

  for (i = 0; i < nScaleFactors; i++) {
    tempLeft  = iEnvelopeLeft[i];
    tempRight = iEnvelopeRight[i];

    iEnvelopeLeft[i]  = (float) (64* (pow(2,tempLeft/a + 1 ) / (1 + pow(2,(panOffset[ampRes] - tempRight) / a))));
    iEnvelopeRight[i] = (float) (64* (pow(2,tempLeft/a + 1 ) / (1 + pow(2,(tempRight - panOffset[ampRes]) / a))));
  }

  for (i = 0; i < nNoiseFactors; i++) {
    tempLeft  = noiseFloorLeft[i];
    tempRight = noiseFloorRight[i];

    noiseFloorLeft[i]  = (float) (pow(2,NOISE_FLOOR_OFFSET - tempLeft + 1 ) / (1 + pow(2,panOffset[1] - tempRight)));
    noiseFloorRight[i] = (float) (pow(2,NOISE_FLOOR_OFFSET - tempLeft + 1 ) / (1 + pow(2,tempRight - panOffset[1])));
  }
}


#ifdef SONY_PVC_DEC
void
sbr_envelope_unmapping_for_pvc (float* iEnvelopeLeft,float* iEnvelopeRight,
                        float* noiseFloorLeft,float* noiseFloorRight,
                        int nScaleFactors,int nNoiseFactors, int ampRes)
{
  int i;
  float tempLeft, tempRight;
  float panOffset[2] = {24.0f, 12.0f};
  float a = 2.0f - ampRes;

  for (i = 0; i < nScaleFactors; i++) {
    tempLeft  = iEnvelopeLeft[i];
    tempRight = iEnvelopeRight[i];

    iEnvelopeLeft[i]  = (float) (64* (pow(2,tempLeft/a + 1 ) / (1 + pow(2,(panOffset[ampRes] - tempRight) / a))));
    iEnvelopeRight[i] = (float) (64* (pow(2,tempLeft/a + 1 ) / (1 + pow(2,(tempRight - panOffset[ampRes]) / a))));
  }

  for (i = 0; i < nNoiseFactors; i++) {
    tempLeft  = noiseFloorLeft[i];
    tempRight = noiseFloorRight[i];

    noiseFloorLeft[i]  = (float) (pow(2,NOISE_FLOOR_OFFSET - tempLeft + 1 ) / (1 + pow(2,panOffset[1] - tempRight)));
    noiseFloorRight[i] = (float) (pow(2,NOISE_FLOOR_OFFSET - tempLeft + 1 ) / (1 + pow(2,tempRight - panOffset[1])));
  }
}
#endif /* SONY_PVC_DEC */


/*******************************************************************************
 \function    requantizeEnvelopeData
 *******************************************************************************
 \author      CTS AB
*******************************************************************************/
static void
requantizeEnvelopeData (float* iEnvelope,
                        int nScaleFactors,
                        int nNoiseFactors,
                        int ampRes,
                        int coupling,
                        float* sbrNoiseFloorLevel)   
{
  int i;
  float ca = 1.0F / (2.0F - ampRes);

  for (i = 0; i < nScaleFactors; i++){
    iEnvelope[i] = (float) (pow(2,iEnvelope[i] * ca) * 64);
  }

  for (i = 0; i < nNoiseFactors; i++) {
    sbrNoiseFloorLevel[i] =
       NOISE_FLOOR_OFFSET - sbrNoiseFloorLevel[i];

      sbrNoiseFloorLevel[i] =
        (float) pow (2.0f, sbrNoiseFloorLevel[i] );
  }
}

#ifdef SONY_PVC_DEC
static void
requantizeEnvelopeData_for_pvc (
							int nNoiseFactors,
							float* sbrNoiseFloorLevel
							,int nNfb
							)   
{
  int i;

  for (i = 0; i < nNoiseFactors; i++) {
    sbrNoiseFloorLevel[i] =
       NOISE_FLOOR_OFFSET - sbrNoiseFloorLevel[i];

      sbrNoiseFloorLevel[i] =
        (float) pow (2.0f, sbrNoiseFloorLevel[i] );
  }

  for (i = 0; i < nNfb; i++) {
    LastEnvNoiseLevel[0][i] = NOISE_FLOOR_OFFSET - LastEnvNoiseLevel[0][i];
    LastEnvNoiseLevel[0][i] = (float) pow (2.0f, LastEnvNoiseLevel[0][i] );
  }
  nNfb_global = nNfb;
}
#endif /* SONY_PVC_DEC */

/*******************************************************************************
 \function   decodeEnvelope
 *******************************************************************************
 \author     CTS AB
*******************************************************************************/
static void
decodeEnvelope(float* iEnvelope,
               int offset,
               int* nSfb,
               int* domain_vec,
               int* frameInfo,
               int ch, int el)
{
  int i, no_of_bands, band, freqRes;

  float *ptr_nrg = iEnvelope;

  for (i = 0; i < frameInfo[0]; i++) {
    freqRes = frameInfo[frameInfo[0] + i + 2];
    no_of_bands = nSfb[freqRes];
    
    if (domain_vec[i] == 0){
      mapLowResEnergyVal(*ptr_nrg, sfb_nrg_prev[el][ch], offset, 0,freqRes);
      ptr_nrg++;

      for (band = 1; band < no_of_bands; band++) {
        *ptr_nrg = *ptr_nrg + *(ptr_nrg-1);
        mapLowResEnergyVal(*ptr_nrg, sfb_nrg_prev[el][ch], offset, band, freqRes);
        ptr_nrg++;
      }
    }
    else {
      for (band = 0; band < no_of_bands; band++) {
        *ptr_nrg = *ptr_nrg + sfb_nrg_prev[el][ch][indexLow2High(offset, band, freqRes)];
        mapLowResEnergyVal(*ptr_nrg, sfb_nrg_prev[el][ch], offset, band, freqRes);
        ptr_nrg++;
      }
    }
  }
}



/*******************************************************************************
 \function   decodeNoiseFloorlevels
 *******************************************************************************
 \author     CTS AB
*******************************************************************************/
static void
decodeNoiseFloorlevels (float* sbrNoiseFloorLevel,
                        int* frameInfo,
                        int nNfb,
                        int* domain_vec,
                        int ch, int el)
{
  int i,env;
  int nEnv = frameInfo[frameInfo[0]*2+3];
  float *ptr_nrg      = sbrNoiseFloorLevel;

  for(env=0; env < nEnv; env++){
    if (domain_vec[env] == 0) {
      prevNoiseLevel[el][ch][0] = *ptr_nrg;
#ifdef SONY_PVC_DEC
      if(env == nEnv -1){
		LastEnvNoiseLevel[ch][0] = *ptr_nrg;
	  }
#endif /* SONY_PVC_DEC */
      ptr_nrg++;
      for (i = 1; i < nNfb; i++){
        *ptr_nrg += *(ptr_nrg-1);
        prevNoiseLevel[el][ch][i] = *ptr_nrg;
#ifdef SONY_PVC_DEC
		if(env == nEnv -1){
			LastEnvNoiseLevel[ch][i] = *ptr_nrg;
		}
#endif /* SONY_PVC_DEC */
        ptr_nrg++;
      }
    }
    else {
      for (i = 0; i < nNfb; i++){
        *ptr_nrg += prevNoiseLevel[el][ch][i];
        prevNoiseLevel[el][ch][i] = *ptr_nrg;
#ifdef SONY_PVC_DEC
		if(env == nEnv -1){
			LastEnvNoiseLevel[ch][i] = *ptr_nrg;
		}
#endif /* SONY_PVC_DEC */
        ptr_nrg++;
      }
    }
  }
}
