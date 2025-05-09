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

 $Id: ct_hfgen.c,v 1.18 2012-04-24 07:09:34 frd Exp $


*******************************************************************************/
/*!
  \file
  \brief  HF Generator  $Revision: 1.18 $
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>


#include "ct_hfgen.h"

#include "HFgen_preFlat.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

struct PATCH Patch[MAX_NUM_CHANNELS];

static float  BwVector[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][MAX_NUM_PATCHES];
static float  BwVectorOld[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS][MAX_NUM_PATCHES];
static int lastMode[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS] = {{0}};

static float InvFiltFactors[7] = {     0,       /* OFF_LEVEL */
                                    0.6f,       /* TRANSITION_LEVEL */
                                   0.75f,       /* LOW_LEVEL */
                                    0.9f,       /* MID_LEVEL */
                                   0.98f };     /* HIGH_LEVEL */


static void
calcAutoCorr ( struct ACORR_COEFS *ac, float realBuf[][64], float imagBuf[][64], int bd, int len)
{
  int   j, jminus1, jminus2;
  const float rel = 1 / (1 + 1e-6f);

  memset(ac, 0, sizeof(struct ACORR_COEFS));

  for ( j = 0; j < len; j++ ) {

    jminus1 = j - 1;
    jminus2 = jminus1 - 1;

#ifdef LOW_POWER_SBR
    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd];

    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd];

    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd];

    ac->r12r += realBuf[jminus1][bd] * realBuf[jminus2][bd];

    ac->r22r += realBuf[jminus2][bd] * realBuf[jminus2][bd];
#else
    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
                imagBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
                realBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
                imagBuf[j][bd] * imagBuf[jminus2][bd];

    ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
                realBuf[j][bd] * imagBuf[jminus2][bd];

    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

    ac->r12r += realBuf[jminus1][bd] * realBuf[jminus2][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus2][bd];

    ac->r12i += imagBuf[jminus1][bd] * realBuf[jminus2][bd] -
                realBuf[jminus1][bd] * imagBuf[jminus2][bd];

    ac->r22r += realBuf[jminus2][bd] * realBuf[jminus2][bd] +
                imagBuf[jminus2][bd] * imagBuf[jminus2][bd];
#endif

  }



  ac->det = ac->r11r * ac->r22r - rel * (ac->r12r * ac->r12r + ac->r12i * ac->r12i);

}

static float
mapInvfMode (INVF_MODE mode,
             INVF_MODE prevMode)
{
  switch (mode) {

  case INVF_LOW_LEVEL:

    if(prevMode == INVF_OFF)
      return InvFiltFactors[1];
    else
      return InvFiltFactors[2];

  case INVF_MID_LEVEL:
    return InvFiltFactors[3];


  case INVF_HIGH_LEVEL:
    return InvFiltFactors[4];


  default:

    if(prevMode == INVF_LOW_LEVEL)
      return InvFiltFactors[1];
    else
      return InvFiltFactors[0];
  }
}

static void
inverseFilteringLevelEmphasis( INVF_MODE *invFiltMode,
                               INVF_MODE *prevInvFiltMode,
                               int nNfb,
                               int ch, int el)
{
  int i;

  for(i = 0; i < nNfb; i++){

    BwVector[el][ch][i] = mapInvfMode (invFiltMode[i],
                                   prevInvFiltMode[i]);

    if(BwVector[el][ch][i] < BwVectorOld[el][ch][i])
      BwVector[el][ch][i] = 0.75000f * BwVector[el][ch][i] + 0.25000f * BwVectorOld[el][ch][i];
    else
      BwVector[el][ch][i] = 0.90625f * BwVector[el][ch][i] + 0.09375f * BwVectorOld[el][ch][i];

    if(BwVector[el][ch][i] < 0.015625)
      BwVector[el][ch][i] = 0;

    if(BwVector[el][ch][i] >= 0.99609375f)
      BwVector[el][ch][i] = 0.99609375f;

  }
}

static int findClosestEntry(int goalSb, int *v_k_master, int numMaster, int direction)
{
  int index;

  if( goalSb <= v_k_master[0] )
    return v_k_master[0];

  if( goalSb >= v_k_master[numMaster] )
    return v_k_master[numMaster];

  if(direction) {
    index = 0;
    while( v_k_master[index] < goalSb ) {
      index++;
    }
  } else {
    index = numMaster;
    while( v_k_master[index] > goalSb ) {
      index--;
    }
  }

  return v_k_master[index];

}

/*******************************************************************************
 Functionname:  generateHF
 *******************************************************************************

 Description:   HF generator with built-in QMF bank inverse filtering function.

 Arguments:

 Return:        none

*******************************************************************************/

void
generateHF (float sourceBufferReal[][64],
            float sourceBufferImag[][64],
            float sourceBufferPVReal[][64],
            float sourceBufferPVImag[][64],
            int      sbrPatchingMode,
            int      bUseHBE,
            float targetBufferReal[][64],
            float targetBufferImag[][64],
            INVF_MODE *invFiltMode,
            INVF_MODE *prevInvFiltMode,
            int *invFiltBandTable,
            int noInvFiltBands,
            int highBandStartSb,
            int *v_k_master,
            int numMaster,
            int fs,
            int* frameInfo,
#ifdef LOW_POWER_SBR
            float *degreeAlias,
#endif
            int ch, int el
#ifdef AAC_ELD
            ,int numTimeSlots
            ,int ldsbr
#endif
            ,int bPreFlattening
            ,int bSbr41
            )
{
  int    bwIndex, i, loBand, hiBand, patch, j;
  int    autoCorrStart, autoCorrLength;
  int    startSample, stopSample, goalSb;
  int    targetStopBand, sourceStartBand, patchDistance, numBandsInPatch;

  float  a0r, a0i, a1r, a1i;

  struct ACORR_COEFS ac;

  int    lsb = v_k_master[0];                           /* Lowest subband related to the synthesis filterbank */
  int    usb = v_k_master[numMaster];                   /* Stop subband related to the synthesis filterbank */
  int    xoverOffset = highBandStartSb - v_k_master[0]; /* Calculate distance in subbands between k0 and kx */

#ifdef LOW_POWER_SBR
  float k1[64];
#endif

  float  bw = 0.0f;
  float  fac = 0.0f;

  float gain;
  float GainVec[64];

  int    slopeLength = 0;
  int    buffOffset  = 0;
  int    firstSlotOffs = frameInfo[1];
  int    lastSlotOffs  = 0;

  static int masterInit = 0;
  static int init[MAX_NUM_ELEMENTS][MAX_NUM_CHANNELS];

#ifdef AAC_ELD
  if (ldsbr) {
    lastSlotOffs = frameInfo[frameInfo[0]+1] - numTimeSlots;
  } else {
#endif
    lastSlotOffs = frameInfo[frameInfo[0]+1] - 16;
#ifdef AAC_ELD
  }
#endif

  if(masterInit == 0){
    for(j = 0; j < MAX_NUM_ELEMENTS; j++){
      for(i=0;i<MAX_NUM_CHANNELS;i++){
        init[j][i] = 0;
      }
    }
    masterInit = 1;
  }

  if(init[el][ch] == 0){
    init[el][ch] = 1;

    memset(BwVectorOld[el][ch],0,sizeof(int)*MAX_NUM_PATCHES);
    memset(prevInvFiltMode,0,sizeof(INVF_MODE)*MAX_NUM_NOISE_VALUES);
  }

 if(bSbr41) {
    startSample = buffOffset + firstSlotOffs * 4;
  } else {
    startSample = buffOffset + firstSlotOffs * 2;
  }
#ifdef AAC_ELD
  if ( ldsbr ) {
    stopSample  = buffOffset + numTimeSlots;
    startSample = buffOffset + 0;
  } else {
#endif
  if(bSbr41) {
    stopSample  = buffOffset + 64 + lastSlotOffs * 4;
  } else {
    stopSample  = buffOffset + 32 + lastSlotOffs * 2;
  }
#ifdef AAC_ELD
  }
#endif

  if (bPreFlattening) {
    calculateGainVec(sourceBufferReal,
                     sourceBufferImag,
                     GainVec, 
                     v_k_master[0],
                     startSample, 
                     stopSample);
  }

  inverseFilteringLevelEmphasis( invFiltMode,
                                 prevInvFiltMode,
                                 noInvFiltBands,
                                 ch, el);

  /* Set subbands to zero */

#ifndef RESET_HF_NOFIX
  for ( i = startSample; i < stopSample; i++ ) {
    memset (targetBufferReal[i]+usb, 0, (64-usb) * sizeof (float));
    memset (targetBufferImag[i]+usb, 0, (64-usb) * sizeof (float));
  } 
#else
  for ( i = startSample; i < stopSample; i++ ) {
    memset (targetBufferReal[i], 0, 64 * sizeof (float));
    memset (targetBufferImag[i], 0, 64 * sizeof (float));
  }
#endif

  autoCorrStart  = buffOffset;
#ifdef AAC_ELD
  if(ldsbr) {
    autoCorrStart = 0;
    autoCorrLength = numTimeSlots;
  } else {
#endif
  if(bSbr41) {
    autoCorrLength = 76;
  } else {
    autoCorrLength = 38;
  }
#ifdef AAC_ELD
  }
#endif

  if (sbrPatchingMode || !bUseHBE) {
    float  alphar[64][2], alphai[64][2];
          

    for ( loBand = 1; loBand < v_k_master[0]; loBand++ ) {

      calcAutoCorr (&ac,
                    &sourceBufferReal[autoCorrStart],
                    &sourceBufferImag[autoCorrStart],
                    loBand,
                    autoCorrLength);

      if ( ac.det == 0.0f ) {
        alphar[loBand][1] = alphai[loBand][1] = 0;
      } else {
        fac = 1.0f / ac.det;
        alphar[loBand][1] = ( ac.r01r * ac.r12r - ac.r01i * ac.r12i - ac.r02r * ac.r11r ) * fac;
        alphai[loBand][1] = ( ac.r01i * ac.r12r + ac.r01r * ac.r12i - ac.r02i * ac.r11r ) * fac;
      }
      
      if ( ac.r11r == 0 ) {
        alphar[loBand][0] =  alphai[loBand][0] = 0;
      } else {
        fac = 1.0f / ac.r11r;
        alphar[loBand][0] = - ( ac.r01r + alphar[loBand][1] * ac.r12r + alphai[loBand][1] * ac.r12i ) * fac;
        alphai[loBand][0] = - ( ac.r01i + alphai[loBand][1] * ac.r12r - alphar[loBand][1] * ac.r12i ) * fac;
      }
      
      /* prevent for shootovers */
      if ( (alphar[loBand][0]*alphar[loBand][0] + alphai[loBand][0]*alphai[loBand][0] >= 16.0f) ||
           (alphar[loBand][1]*alphar[loBand][1] + alphai[loBand][1]*alphai[loBand][1] >= 16.0f) )
        {
          alphar[loBand][0] = 0.0f;
          alphai[loBand][0] = 0.0f;
          alphar[loBand][1] = 0.0f;
          alphai[loBand][1] = 0.0f;
        }
      
#ifdef LOW_POWER_SBR
      if(ac.r11r==0.0) {
        k1[loBand] = 0.0f;
      } else {
        k1[loBand] = -(ac.r01r/ac.r11r);
        if(k1[loBand]>1.0)  k1[loBand] = 1.0f;
        if(k1[loBand]<-1.0) k1[loBand] = -1.0f;
      }
#endif
    }


#ifdef LOW_POWER_SBR
    k1[0] = 0.0;
    degreeAlias[1] = 0.0;
    for ( loBand = 2; loBand < v_k_master[0]; loBand++ ){
      degreeAlias[loBand] = 0.0;
      if ((loBand % 2 == 0) && (k1[loBand] < 0.0)){
        if (k1[loBand-1] < 0.0) { /* 2-CH Aliasing Detection */
          degreeAlias[loBand]   = 1.0;
          if ( k1[loBand-2] > 0.0 ) { /* 3-CH Aliasing Detection */
            degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
          }
        }
        else if ( k1[loBand-2] > 0.0 ) { /* 3-CH Aliasing Detection */
          degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
      if ((loBand % 2 == 1) && (k1[loBand] > 0.0)){
        if (k1[loBand-1] > 0.0) { /* 2-CH Aliasing Detection */
          degreeAlias[loBand]   = 1.0f;
          if ( k1[loBand-2] < 0.0 ) { /* 3-CH Aliasing Detection */
            degreeAlias[loBand-1] = 1.0f - k1[loBand-1] * k1[loBand-1];
          }
        }
        else if ( k1[loBand-2] < 0.0 ) { /* 3-CH Aliasing Detection */
          degreeAlias[loBand]   = 1.0f - k1[loBand-1] * k1[loBand-1];
        }
      }
    }
#endif

    /*
     * Initialize the patching parameter
     */
    goalSb = (int)( 2.048e6f / fs + 0.5f );                      /* 16 kHz band */
    goalSb = findClosestEntry(goalSb, v_k_master, numMaster, 1); /* Adapt region to master-table */

    /* First patch */
    sourceStartBand = xoverOffset + 1;
    targetStopBand = lsb + xoverOffset;

    /* even (odd) numbered channel must be patched to even (odd) numbered channel */
    patch = 0;
    while(targetStopBand < usb) {

      Patch[ch].targetStartBand[patch] = targetStopBand;

      numBandsInPatch = goalSb - targetStopBand;                   /* get the desired range of the patch */

      if ( numBandsInPatch >= lsb - sourceStartBand ) {
        /* desired number bands are not available -> patch whole source range */
        patchDistance   = targetStopBand - sourceStartBand;        /* get the targetOffset */
        patchDistance   = patchDistance & ~1;                      /* rounding off odd numbers and make all even */
        numBandsInPatch = lsb - (targetStopBand - patchDistance);
        numBandsInPatch = findClosestEntry(targetStopBand + numBandsInPatch, v_k_master, numMaster, 0) -
          targetStopBand;  /* Adapt region to master-table */
      }

      /* desired number bands are available -> get the minimal even patching distance */
      patchDistance   = numBandsInPatch + targetStopBand - lsb;  /* get minimal distance */
      patchDistance   = (patchDistance + 1) & ~1;                /* rounding up odd numbers and make all even */

      /* All patches but first */
      sourceStartBand = 1;


      /* Check if we are close to goalSb */
      /* if( abs(targetStopBand + numBandsInPatch - goalSb) < 3) { */
      if( goalSb - (targetStopBand + numBandsInPatch) < 3) { /* MPEG doc */
        goalSb = usb;
      }

#ifndef HF_GENERATOR_NOFIX
      if ((numBandsInPatch < 3) && (patch > 0) && (targetStopBand + numBandsInPatch == usb)) {
#else
      if ( (numBandsInPatch < 3) && (patch > 0) ) {
#endif

#ifdef LOW_POWER_SBR
        for (hiBand = targetStopBand; hiBand < targetStopBand + numBandsInPatch; hiBand++ ) {
          degreeAlias[hiBand] = 0;
        }
#endif
#ifndef SONY_RESET_HF_NOFIX
        for(i = startSample + slopeLength; i < stopSample + slopeLength; i++) {
          for (hiBand = targetStopBand; hiBand < targetStopBand + numBandsInPatch; hiBand++ ){
            targetBufferReal[i][hiBand] = 0.0f;
            targetBufferImag[i][hiBand] = 0.0f;
          }
        }
#endif

        break;
      }

      if ( numBandsInPatch <= 0 ) {
        continue;
      }

      for (hiBand = targetStopBand; hiBand < targetStopBand + numBandsInPatch; hiBand++ ) {

        loBand = hiBand - patchDistance;

#ifdef LOW_POWER_SBR
        if(hiBand != targetStopBand)
          degreeAlias[hiBand] = degreeAlias[loBand];
        else
          degreeAlias[hiBand] = 0;
#endif

        bwIndex = 0;
        while (hiBand >= invFiltBandTable[bwIndex])	bwIndex++;

        bw = BwVector[el][ch][bwIndex];

        a0r = bw * alphar[loBand][0]; /* Apply current bandwidth expansion factor */
        a0i = bw * alphai[loBand][0];
        bw *= bw;
        a1r = bw * alphar[loBand][1];
        a1i = bw * alphai[loBand][1];

        if (bPreFlattening) {
          gain = GainVec[loBand];
        }
        else {
          gain = 1.0f;
        }

	for(i = startSample + slopeLength; i < stopSample + slopeLength; i++ ) {
	  targetBufferReal[i][hiBand] = sourceBufferReal[i][loBand]*gain;

#ifdef LOW_POWER_SBR
          targetBufferImag[i][hiBand] = 0.0f;
#else
	  targetBufferImag[i][hiBand] = sourceBufferImag[i][loBand]*gain;
#endif

	  if ( bw > 0.0f ) {
	    targetBufferReal[i][hiBand] += ( a0r * sourceBufferReal[i-1][loBand] -
					     a0i * sourceBufferImag[i-1][loBand] +
					     a1r * sourceBufferReal[i-2][loBand] -
					     a1i * sourceBufferImag[i-2][loBand] ) * gain;
#ifndef LOW_POWER_SBR
	    targetBufferImag[i][hiBand] += ( a0i * sourceBufferReal[i-1][loBand] +
					     a0r * sourceBufferImag[i-1][loBand] +
					     a1i * sourceBufferReal[i-2][loBand] +
					     a1r * sourceBufferImag[i-2][loBand] ) * gain;
#endif
          }
        }
      }  /* hiBand */

      targetStopBand += numBandsInPatch;

      patch++;

    }  /* targetStopBand */

  } /* sbrPatchingMode */

  if(bUseHBE && !sbrPatchingMode){
    float  alphar[2], alphai[2];
 
    bwIndex = 0, patch = 1;

    for ( hiBand = highBandStartSb; hiBand < v_k_master[numMaster]; hiBand++ ) {

      calcAutoCorr (&ac,
                    &sourceBufferPVReal[autoCorrStart],
                    &sourceBufferPVImag[autoCorrStart],
                    hiBand,
                    autoCorrLength);

      if ( ac.det == 0.0f ) {
        alphar[1] = alphai[1] = 0;
      } else {
        fac = 1.0f / ac.det;
        alphar[1] = ( ac.r01r * ac.r12r - ac.r01i * ac.r12i - ac.r02r * ac.r11r ) * fac;
        alphai[1] = ( ac.r01i * ac.r12r + ac.r01r * ac.r12i - ac.r02i * ac.r11r ) * fac;
      }

      if ( ac.r11r == 0 ) {
        alphar[0] =  alphai[0] = 0;
      } else {
        fac = 1.0f / ac.r11r;
        alphar[0] = - ( ac.r01r + alphar[1] * ac.r12r + alphai[1] * ac.r12i ) * fac;
        alphai[0] = - ( ac.r01i + alphai[1] * ac.r12r - alphar[1] * ac.r12i ) * fac;
      }

      /* prevent for shootovers */
      if ( alphar[0]*alphar[0] + alphai[0]*alphai[0] >= 16.0f ||
           alphar[1]*alphar[1] + alphai[1]*alphai[1] >= 16.0f )
        {
          alphar[0] = 0.0f;
          alphai[0] = 0.0f;
          alphar[1] = 0.0f;
          alphai[1] = 0.0f;
    	}

      while (hiBand >= invFiltBandTable[bwIndex]) bwIndex++;

      bw = BwVector[el][ch][bwIndex];

      a0r = bw * alphar[0]; /* Apply current bandwidth expansion factor */
      a0i = bw * alphai[0];
      bw *= bw;
      a1r = bw * alphar[1];
      a1i = bw * alphai[1];

      if ( bw > 0.0f ) {
        for(i = startSample; i < stopSample; i++ ) {
          float real1, imag1, real2, imag2, realTarget, imagTarget;          
        
          realTarget = sourceBufferPVReal[i]  [hiBand];
          imagTarget = sourceBufferPVImag[i]  [hiBand];
          real1      = sourceBufferPVReal[i-1][hiBand];
          imag1      = sourceBufferPVImag[i-1][hiBand];
          real2      = sourceBufferPVReal[i-2][hiBand];
          imag2      = sourceBufferPVImag[i-2][hiBand];
          realTarget += ( (a0r * real1 -
                           a0i * imag1) +
                          (a1r * real2 -
                           a1i * imag2) );
          imagTarget += ( (a0i * real1 +
                           a0r * imag1) +
                          (a1i * real2 +
                           a1r * imag2) );
            
          targetBufferReal[i][hiBand] = realTarget;
          targetBufferImag[i][hiBand] = imagTarget;
        }
      } else {
       	for(i = startSample; i < stopSample; i++ ) {
          targetBufferReal[i][hiBand] = sourceBufferPVReal[i][hiBand];
          targetBufferImag[i][hiBand] = sourceBufferPVImag[i][hiBand];
        }  
      }
    }  /* hiBand */
  } /* bUseHBE && !sbrPatchingMode */
  

  Patch[ch].noOfPatches = patch;

  for (i = 0; i < noInvFiltBands; i++ ) {
    BwVectorOld[el][ch][i] = BwVector[el][ch][i];
  }
}
