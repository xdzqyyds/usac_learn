/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author: Philippe Gournay


and edited by: Jeremie Lecomte

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

*******************************************************************************/
/*_____________________________________________________________________
 |                                                                     |
 |  FUNCTION NAME segsnr                                               |
 |      Computes the segmential signal-to-noise ratio between the      |
 |      signal x and its estimate xe of length n samples. The segment  |
 |      length is nseg samples.                                        |
 |
 |  INPUT
 |      x[0:n-1]   Signal of n samples.
 |      xe[0:n-1]  Estimated signal of n samples.
 |      n          Signal length.
 |      nseg       Segment length, must be a submultiple of n.
 |
 |  RETURN VALUE
 |      snr        Segmential signal to noise ratio in dB.
 |_____________________________________________________________________|
*/
#include <math.h>
#include <float.h>

#include "proto_func.h"

float segsnr(float x[], float xe[], short n, short nseg)
{
    float snr = 0.0f;
    float signal, noise, error, fac;
    short i, j;
    for (i = 0; i < n; i += nseg) {
        signal = 1e-6f;
        noise  = 1e-6f;
        for (j = 0; j < nseg; j++) {
            signal += (*x)*(*x);
            error   = *x++ - *xe++;
            noise  += error*error;
        }
        snr += (float)log10((double)(signal/noise));
    }
    fac = ((float)(10*nseg))/(float)n;
    snr = fac*snr;
    if (snr < -99.0f) {
        snr = -99.0f;
    }
    return(snr);
}

