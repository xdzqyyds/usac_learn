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
$Id: harmsbrMathlib.c,v 1.3 2011-02-10 11:52:52 mul Exp $
*******************************************************************************/

#include <math.h>
#include <float.h>

#include "harmsbrMathlib.h"


/**************************************************************************/
/*!  \brief     Circular shift of FFT values                              */
/**************************************************************************/
void fftshift (float *spectrum,
                      int    size)
{
  int n;

  for(n=0;n<size/2;n++){
    float tmp             = spectrum[n];
    spectrum[n]           = spectrum[n+size/2];
    spectrum[n+size/2] = tmp;
  }
}

/**************************************************************************/
/*!  \brief     Cartesian -> Euler conversion                             */
/**************************************************************************/
void karth2polar (float* spectrum,
                  float* mag,
                  float* phase,
                  int    fftSize  )
{
  int n;

  for(n=1; n<fftSize/2; n++) {
    phase[n] = (float) atan2(spectrum[2*n+1], spectrum[2*n]);
    mag[n]   = (float) sqrt(spectrum[2*n]*spectrum[2*n] + spectrum[2*n+1]*spectrum[2*n+1]);
  }

  if(spectrum[0] < 0) {
    phase[0] = (float) acos(-1);
    mag[0]   = -spectrum[0];
  } else {
    phase[0] = 0;
    mag[0]   = spectrum[0];
  }

  if(spectrum[1] < 0) {
    phase[fftSize/2] = (float) acos(-1);
    mag[fftSize/2]   = -spectrum[1];
  } else {
    phase[fftSize/2] = 0;
    mag[fftSize/2]   = spectrum[1];
  }
}

/**************************************************************************/
/*!  \brief     Euler -> Cartesian conversion                             */
/**************************************************************************/
void polar2karth (float* mag,
                         float* phase,
                         float* spectrum,
                         int fftSize)
{
  int n;

  for(n=1;n<fftSize/2;n++) {
    spectrum[2*n  ] = (float) (mag[n] * cos(phase[n]));
    spectrum[2*n+1] = (float) (mag[n] * sin(phase[n]));
  }
  spectrum[0] = (float) (mag[        0] * cos(phase[        0]));
  spectrum[1] = (float) (mag[fftSize/2] * cos(phase[fftSize/2]));
}

/**************************************************************************/
/*!  \brief     Spectral Flatness Measure                                 */
/**************************************************************************/
float sfm(const float* input,
                 const int nSamples)
{
  double geommean = 1;
  double mean = 0;
  double absVal;
  float sfmVal;
  int i;

  for (i=0;i<nSamples;i+=2) {
    absVal = sqrt(input[i]*input[i] + input[i+1]*input[i+1])/nSamples;
    geommean *= (absVal>1e-6)? absVal:1;
    mean += absVal;
  }

  geommean = (double) pow(geommean,(1.f/((double) nSamples/2)));
  mean /= nSamples/2;
  sfmVal = (float) (geommean/mean);

  return sfmVal;
}

/**************************************************************************/
/*!  \brief     Center of gravity                                         */
/**************************************************************************/
float cog (const float* input,
           const int nSamples)
{
  int x;
  float center = 0;
  float sumY = 0;

  for (x=0;x<nSamples;x++) {
    center += (x+1)*input[x];
    sumY   +=input[x];
  }
  return center = center/sumY-1;
}


/**************************************************************************/
/*!  \brief     Center of gravity of absolute values                      */
/**************************************************************************/
float absCog (const float* input,
              const int nSamples)
{
  int x;
  float center = 0;
  float sumY = 0;
  float tmp;

  for (x=0;x<nSamples;x++) {
    tmp = (input[x]<0)? -input[x]:input[x];
    center += (x+1)*tmp;
    sumY   += tmp;
  }
  return center = center/sumY-1;
}
