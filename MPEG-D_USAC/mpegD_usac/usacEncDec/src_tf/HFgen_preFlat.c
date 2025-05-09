/*******************************************************************************
This software module was originally developed by

  Dolby Laborities Inc.


Initial author: Kristofer Kjörling, Leif Sehlström

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003: Dolby Laboratories grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Dolby Laboratories
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Dolby Laboratories
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Dolby Laboratories retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: HFgen_preFlat.c,v 1.5 2011-02-01 09:42:30 mul Exp $
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "HFgen_preFlat.h"

#define MAXDEG 3

static void polyfit(int degeree, int n, float x[], float y[], float p[]);
static void  gaussSolve(int n, float A[][MAXDEG+1], float b[], float y[]);


void calculateGainVec(float sourceBufferReal[][64],
                      float sourceBufferImag[][64],
                      float GainVec[], 
                      int numBands, 
                      int startSample, 
                      int stopSample)
{

  int loBand;
  int k,i;
  float p[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  int polyOrderWhite = 3;
  float x_lowBand[64];
  float meanNrg;
  float lowBandEnvSlope[64];
  float LowEnv[64];

  for (i=0; i<64; i++){
    x_lowBand[i] = (float) i;
    lowBandEnvSlope[i] = 0;
  }


  /* Calculate the spectral envelope in dB over the current copy-up frame. */
  for ( loBand = 0; loBand < numBands; loBand++ ) {
    LowEnv[loBand] = 0;
    for ( i = startSample; i < stopSample; i++ ) {
      LowEnv[loBand] += sourceBufferReal[i][loBand]*sourceBufferReal[i][loBand] + sourceBufferImag[i][loBand]*sourceBufferImag[i][loBand];
     }
    LowEnv[loBand] /= (stopSample - startSample);
  }
  
  for ( loBand = 0; loBand < numBands; loBand++ ) {
    LowEnv[loBand] = (float) (10*log10(LowEnv[loBand] + 1)); /* 1 equals epsilon in this environment. */
  }


  polyfit(polyOrderWhite, numBands, x_lowBand, LowEnv, p);
   
  for (k=polyOrderWhite; k>=0; k--){
    for(i=0; i < numBands; i++){
      lowBandEnvSlope[i] = lowBandEnvSlope[i] + ((float) (pow(x_lowBand[i],k)*p[polyOrderWhite - k]));
    }
  }

  meanNrg = 0;
  for(i=0; i < numBands; i++){
    meanNrg += LowEnv[i];
  }
  meanNrg /= numBands;

  for(i=0; i < numBands; i++){
    GainVec[i] = (float) pow(10,(meanNrg - lowBandEnvSlope[i])/20.0f);
  }
}








/* Matlab "polyfit" ported to C by Leif Sehlström */

void polyfit(int deg, int n, float x[], float y[], float p[]) {
  int i, j, k;
  float A[MAXDEG+1][MAXDEG+1];
  float b[MAXDEG+1];
  float v[2*MAXDEG+1];

  for (i = 0; i <= deg; i++) {
    b[i] = 0.0f;
    for (j = 0; j <= deg; j++) {
       A[i][j] = 0.0f;
    }
  }

  for (k = 0; k < n; k++) {
    v[0] = 1.0;
    for (i = 1; i <= 2*deg; i++) {
      v[i] = x[k]*v[i-1];
    }

    for (i = 0; i <= deg; i++) {
      b[i] += v[deg-i]*y[k];
      for (j = 0; j <= deg; j++) {
         A[i][j] += v[2*deg - i - j];
      }
    }
  }

  gaussSolve(deg + 1, A, b, p);
}


static void gaussSolve(int n, float A[][MAXDEG+1], float b[], float y[]) {
  int i, j, k, imax;
  float v;
  float eps = 1.0e-9f;

  for (i = 0; i < n; i++) {
    imax = i;
    for (k = i + 1; k < n; k++) {  /* find pivot */
      if (fabs(A[k][i]) > fabs(A[imax][i])) {
        imax = k;
      }
    }

    if (imax != i) {  /* swap rows */
      v = b[imax];
      b[imax] = b[i];
      b[i] = v;
      for (j = i; j < n; j++) {
        v = A[imax][j];
        A[imax][j] = A[i][j];
        A[i][j] = v;
      }
    }

    v = A[i][i];  /* normailize row */

    if (fabs(v) > eps) { /* should always be true */
      b[i] /= v;
      for (j = i; j < n; j++) {
        A[i][j] /= v;
      }
    }

    for (k = i + 1; k < n; k++) { /* subtract row i for row > i */
      v = A[k][i];
      b[k] -= v*b[i];
      for (j = i + 1; j < n; j++) {
        A[k][j] -= v*A[i][j];
      }
    }
  }

  for (i = n - 1; i >= 0; i--) {   /* back substitution */
    y[i] = b[i];
    for (j = i + 1; j < n; j++) {
      y[i] -= A[i][j]*y[j];
    }
  }
}
