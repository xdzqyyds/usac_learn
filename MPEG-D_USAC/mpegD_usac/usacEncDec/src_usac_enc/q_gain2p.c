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

$Id: q_gain2p.c,v 1.1.1.1 2009-05-29 14:10:22 mul Exp $
*******************************************************************************/

/*-------------------------------------------------------------------------*
 * procedure q_gain2_plus                                                  *
 * ~~~~~~~~~~~~~~~~~~~~~~                                                  *
 * Quantization of pitch and codebook gains.                               *
 * The following routines is Q_gains updated for AMR_WB_PLUS.              *
 * MA prediction is removed and MEAN_ENER is now quantized with 2 bits and *
 * transmitted once every ACELP frame to the gains decoder.                *
 * The pitch gain and the code gain are vector quantized and the           *
 * mean-squared weighted error criterion is used in the quantizer search.  *
 *-------------------------------------------------------------------------*/
#include <math.h>
#include <float.h>
#include "../src_usac/acelp_plus.h"
#define RANGE         64
int q_gain2_plus(   /* (o)  : index of quantizer                      */
  float code[],     /* (i)  : Innovative code vector                  */
  int lcode,        /* (i)  : Subframe size                           */
  float *gain_pit,  /* (i/o): Pitch gain / Quantized pitch gain       */
  float *gain_code, /* (i/o): code gain / Quantized codebook gain     */
  float *coeff,     /* (i)  : correlations <y1,y1>, -2<xn,y1>,        */
                    /*                <y2,y2>, -2<xn,y2> and 2<y1,y2> */
  float mean_ener  /* (i)  : mean_ener defined in open-loop (2 bits) */
)
{
  int   i, indice=0, min_ind, size;
  float ener_code, gcode0;
  float dist, dist_min, g_pitch, g_code;
  const float *t_qua_gain, *p;
/*-----------------------------------------------------------------*
 * - Find the initial quantization pitch index                     *
 * - Set gains search range                                        *
 *-----------------------------------------------------------------*/
  t_qua_gain = E_ROM_qua_gain7b;
  p = (const float *) (E_ROM_qua_gain7b + RANGE);       /* pt at 1/4th of table */
  min_ind = 0;
  g_pitch = *gain_pit;
  for (i=0; i<(NB_QUA_GAIN7B-RANGE); i++, p+=2) {
    if (g_pitch > *p) {
      min_ind++;
    }
  }
  size = RANGE;
  min_ind=0;
  size=128;             
  /* innovation energy (without gain) */
  ener_code = 0.01F;
  for(i=0; i<lcode; i++) {
    ener_code += code[i] * code[i];
  }
  ener_code = (float)(10.0 * log10(ener_code /(float)lcode));
  /* predicted codebook gain */
  /* mean energy quantized with 2 bits : 18, 30, 42 or 54 dB */
  gcode0 = mean_ener - ener_code;				
  gcode0 = (float)pow(10.0,gcode0/20.0);   /* predicted gain */
  /* Search for best quantizer */
  dist_min = FLT_MAX;
  p = (const float *) (t_qua_gain + min_ind*2);
  for (i = 0; i<size; i++)
  {
    g_pitch = *p++;                  /* pitch gain */
    g_code = gcode0 * *p++;         /* codebook gain */
    dist = g_pitch*g_pitch * coeff[0]
         + g_pitch         * coeff[1]
         + g_code*g_code   * coeff[2]
         + g_code          * coeff[3]
         + g_pitch*g_code  * coeff[4];				
    if (dist < dist_min)
    {
      dist_min = dist;
      indice = i;
    }
  }
  indice += min_ind;
  *gain_pit  = t_qua_gain[indice*2];
  *gain_code = t_qua_gain[indice*2+1] * gcode0;

  return indice;
}
