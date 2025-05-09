/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp, Dolby Laboratories.


Initial author:
Nikolaus Rettelbach     (Fraunhofer IIS)
Frederik Nagel          (Fraunhofer IIS)

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
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: harmsbrWingen.c,v 1.4 2011-02-10 11:52:52 mul Exp $
*******************************************************************************/
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "harmsbrWingen.h"

#define V2_2_WARNINGS_ERRORS_FIX

void sineWindow(float *win, int size)
{
  int n;
  float pi = (float) acos(-1.0);

  for ( n=0; n<size; n++) {
    win[n] = (float) (sin( ( pi * n ) / (size) ));
  }
}


void designFlatTopWin(float *win, int size, int sideLobeSize,int exponent) {
  int n;
  double pi = acos(-1.0);

  for ( n=0; n<size; n++) win[n] = 1;
  for ( n=0; n<sideLobeSize; n++)  {
    win[n] = (float) (pow(sin(n*pi/(2*sideLobeSize)),exponent));
    win[size-n-1] = win[n];
  }
  win[0]= 0;
  win[size-1]= 0;
}

void designBandPass(float *win, int size, int lBorder, int rBorder, int width) {
  int n;
  double pi = acos(-1.0);
  int range;

  if (width%2 == 0)
    width = width+1;
  range = (int) (width/2);

  for ( n=0; n<size; n++) win[n] = 0;
  for ( n=lBorder+range; n<rBorder-range; n++) win[n] = 1;

  for ( n=lBorder-range; n<=lBorder+range; n++)  {
    win[n] = (float) (pow(sin((n-(lBorder-range))*pi/(2*width)),2));
  }
  for ( n=rBorder-range; n<=rBorder+range; n++)  {
    win[n] = (float) (pow(sin(((width+1+n-(rBorder-range))*pi)/(2*width)),2));
  }
}

void createAnalysisWin(float *win, int size) {
  designFlatTopWin(win,size,size/6,1);
}

void createSynthesisWin(float *win, float *anawin, int size) {
  float *synWinTheor = (float*) calloc(size, sizeof(float));
  int n;

  designFlatTopWin(synWinTheor,size,size/4,2);

  for ( n=0; n<size; n++) {
    win[n] = (float) (synWinTheor[n]/(anawin[n] + 1e-12));
  }
  free(synWinTheor);
}


void createFreqDomainWin(float *win, int xOverBin1, int xOverBin2, int ts, int size) {
  int n;  
  for (n=0; n< (xOverBin1-ts/2); n++) {
    win[n] = 0;
  }
  
  for (n=(xOverBin1-ts/2); n<= (xOverBin1+ts/2); n++) {
    win[n] = 0.5 + 0.5 * sin((acos(-1.0)/ts) * (n-xOverBin1));
  }
  
  for (n=(xOverBin1+ts/2+1); n<(xOverBin2-ts/2); n++) {
    win[n] = 1;
  }
  
  for (n=(xOverBin2-ts/2); n<=(xOverBin2+ts/2); n++) {
    win[n] = 0.5 - 0.5 * sin((acos(-1.0)/ts) * (n-xOverBin2));
  }
  
  for (n=(xOverBin2+ts/2+1); n<size; n++) {
    win[n] = 0;
  }
  
}

