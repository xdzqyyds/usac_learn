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
$Id: d_gain2p.c,v 1.1.1.1 2009-05-29 14:10:20 mul Exp $
*******************************************************************************/

/*-------------------------------------------------------------------*
 * procedure d_gain2_plus                                            *
 * ~~~~~~~~~~~~~~~~~~~~~~                                            *
 * Decoding of pitch and codebook gains  (see q_gain2_plus.c)        *
 *-------------------------------------------------------------------*
 * input arguments:                                                  *
 *                                                                   *
 *   indice     :Quantization index                                  *
 *   code[]     :Innovative code vector                              *
 *   lcode      :Subframe size                                       *
 *   bfi        :Bad frame indicator                                 *
 *                                                                   *
 * output arguments:                                                 *
 *                                                                   *
 *   gain_pit   :Quantized pitch gain                                *
 *   gain_code  :Quantized codeebook gain                            *
 *                                                                   *
 * Global variables defining quantizer (in qua_gns.h)                *
 *                                                                   *
 *   t_qua_gain[]    :Table of gain quantizers                       *
 *   nb_qua_gain     :Nombre de quantization levels                  *
 *                                                                   *
 *-------------------------------------------------------------------*/
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "acelp_plus.h"
float d_gain2_plus( /* (o)  : 'correction factor' */
  int index,        /* (i)  : index of quantizer                      */
  float code[],     /* (i)  : Innovative code vector                  */
  int lcode,        /* (i)  : Subframe size                           */
  float *gain_pit,  /* (o)  : Quantized pitch gain                    */
  float *gain_code, /* (o)  : Quantized codebook gain                 */
  int bfi,          /* (i)  : Bad frame indicato                      */
  float mean_ener,  /* (i)  : mean_ener defined in open-loop (2 bits) */
  float *past_gpit, /* (i)  : past gain of pitch                      */
  float *past_gcode)/* (i/o): past gain of code                       */
{
  int   i;
  float ener_code, gcode0;
  const float *t_qua_gain;
  float ener_inov, gcode_inov;
  t_qua_gain = E_ROM_qua_gain7b;
  ener_inov = 0.01f;
  for(i=0; i<lcode; i++) {
    ener_inov += code[i] * code[i];
  }
  gcode_inov = (float)(1.0 / sqrt(ener_inov / (float)lcode));
  /*----------------- Test erasure ---------------*/
  if (bfi != 0)
  {
    if (*past_gpit > 0.95f) {
     *past_gpit = 0.95f;
    }
    if (*past_gpit < 0.5f) {
      *past_gpit = 0.5f;
    }
    *gain_pit = *past_gpit;
    *past_gpit *= 0.95f;
    *past_gcode *= (1.4f - *past_gpit);
    *gain_code = *past_gcode * gcode_inov;
    return 0;
  }
  /*-------------- Decode gains ---------------*/
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
  *gain_pit = t_qua_gain[index*2];
  *gain_code = t_qua_gain[index*2+1] * gcode0;
  /* update bad frame handler */
  *past_gpit = *gain_pit;
  *past_gcode = *gain_code/gcode_inov;
  return t_qua_gain[index*2+1];
}
