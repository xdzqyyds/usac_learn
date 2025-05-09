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
$Id: hp50.c,v 1.1.1.1 2009-05-29 14:10:20 mul Exp $
*******************************************************************************/

#include "acelp_plus.h"
/*
 * hp50_12k8
 *
 * Function:
 *    2nd order high pass filter with nominal cut off frequency at 50 Hz.
 *
 * Returns:
 *    void
 */
void hp50_12k8(Float32 signal[], Word32 lg, Float32 mem[], Word32 fscale)
{
   Word16 i;
   Float32 x0, x1, x2, y0, y1, y2;
   Float32 a1, a2, b1, b2, frac;
   y1 = mem[0];
   y2 = mem[1];
   x0 = mem[2];
   x1 = mem[3];
   if (fscale >= FSCALE_DENOM)
   {
      /* design in matlab (-3dB @ 20 Hz)
         [b,a] = butter(2, 20.0/6400.0, 'high');
         a = [1.00000000000000  -1.98611621154089   0.98621192916075];
         b = [0.99308203517541  -1.98616407035082   0.99308203517541];
         [b,a] = butter(2, 20.0/12800.0, 'high');
         a = [1.00000000000000  -1.99305802314321   0.99308203546221];
         b = [0.99653501465135  -1.99307002930271   0.99653501465135];
         difference:
         a = [               0  -0.00694181160232   0.00687010630146];
         b = [0.00345297947594  -0.00690595895189   0.00345297947594];
      */
      /* this approximation is enough (max overshoot = +0.05dB) */
      frac = ((Float32)(fscale-FSCALE_DENOM)) / ((Float32)FSCALE_DENOM);
      a1 =  1.98611621154089f + (frac*0.00694181160232f);
      a2 = -0.98621192916075f - (frac*0.00687010630146f);
      b1 = -1.98616407035082f - (frac*0.00690595895189f);
      b2 =  0.99308203517541f + (frac*0.00345297947594f);
      for(i = 0; i < lg; i++)
      {
         x2 = x1;
         x1 = x0;
         x0 = signal[i];
         y0 = (y1*a1) + (y2*a2) + (x0*b2) + (x1*b1) + (x2*b2);
         signal[i] = y0;
         y2 = y1;                                                              
         y1 = y0;
      }
   }
   else           /* -6dB 24Hz fs/2=6400Hz (3GPP AMR-WB coef) */
   {
      for(i = 0; i < lg; i++)
      {
         x2 = x1;
         x1 = x0;
         x0 = signal[i];
         y0 = y1 * 1.978881836F + y2 * -0.979125977F + x0 * 0.989501953F +
            x1 * -1.979003906F + x2 * 0.989501953F;
         signal[i] = y0;
         y2 = y1;                                                              
         y1 = y0;
      }
   }
   mem[0] = ((y1 > 1e-10) | (y1 < -1e-10)) ? y1 : 0;
   mem[1] = ((y2 > 1e-10) | (y2 < -1e-10)) ? y2 : 0;
   mem[2] = ((x0 > 1e-10) | (x0 < -1e-10)) ? x0 : 0;
   mem[3] = ((x1 > 1e-10) | (x1 < -1e-10)) ? x1 : 0;
   return;
}
