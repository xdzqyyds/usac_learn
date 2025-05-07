/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

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
$Id: pit_fr4.c,v 1.1.1.1 2009-05-29 14:10:20 mul Exp $
*******************************************************************************/


/*-------------------------------------------------------------------*
 * procedure pitch_fr4                                               *
 * ~~~~~~~~~~~~~~~~~~~                                               *
 * Find the closed loop pitch period with 1/4 subsample resolution.  *
 *-------------------------------------------------------------------*/
#include <math.h>
#include "acelp_plus.h"
/* locals functions */
/*-------------------------------------------------------------------*
 * Function  pred_lt4:                                               *
 *           ~~~~~~~~~                                               *
 *-------------------------------------------------------------------*
 * Compute the result of long term prediction with fractionnal       *
 * interpolation of resolution 1/4.                                  *
 *                                                                   *
 * On return exc[0..L_subfr-1] contains the interpolated signal      *
 *   (adaptive codebook excitation)                                  *
 *-------------------------------------------------------------------*/
void pred_lt4(
  float exc[],       /* in/out: excitation buffer */
  int   T0,          /* input : integer pitch lag */
  int   frac,        /* input : fraction of lag   */
  int   L_subfr      /* input : subframe size     */
)
{
  int   i, j;
  float s, *x0, *x1, *x2;
  const float *c1, *c2;

  x0 = &exc[-T0];
  frac = -frac;
  if (frac < 0) {
    frac += PIT_UP_SAMP;
    x0--;
  }
  for (j=0; j<L_subfr; j++)
  {
    x1 = x0++;
    x2 = x1+1;
    c1 = &inter4_2[frac];
    c2 = &inter4_2[PIT_UP_SAMP-frac];
    s = 0.0;
    for(i=0; i<PIT_L_INTERPOL2; i++, c1+=PIT_UP_SAMP, c2+=PIT_UP_SAMP) {
      s += (*x1--) * (*c1) + (*x2++) * (*c2);
    }
    exc[j] = s;
  }
  return;
}
