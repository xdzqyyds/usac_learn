/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author: Philippe Gournay, Bruno Bessette


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

$Id: vlpc_1st_cod.c,v 1.2 2010-03-12 06:54:43 mul Exp $
*******************************************************************************/

#include <math.h>
#include <float.h>
#include "proto_func.h"

#define ORDER 16
#define FREQ_MAX 6400.0f
#define LSF_GAP  50.0f


static void lsf_weight(float *lsfq, float *w)
{
  int i;
  float d[ORDER+1];

  /* compute lsf distance */
  d[0] = lsfq[0];
  d[ORDER] = FREQ_MAX - lsfq[ORDER-1];
  for (i=1; i<ORDER; i++)
  {
	  d[i] = lsfq[i] - lsfq[i-1];
  }

  /* weighting function */
  for (i=0; i<ORDER; i++)
  {
     w[i] = (1.0f/d[i]) + (1.0f/d[i+1]);
  }

  return;
}


int vlpc_1st_cod( /* output: codebook index                  */ 
  float *lsf,     /* input:  vector to quantize              */
  float *lsfq)    /* i/o:    i:prediction   o:quantized lsf  */
{
  int    i, j, index;
  float  w[ORDER], x[ORDER];
  float dist_min, dist, temp;
  const float *p_dico;
  extern float dico_lsf_abs_8b[];

  /* weighting */
  lsf_weight(lsf, w);

  /* remove lsf prediction/means */
  for (i=0; i<ORDER; i++) {
    x[i] = lsf[i] - lsfq[i];
  }

  dist_min = 1.0e30f;
  p_dico = dico_lsf_abs_8b;
  index = 0;

  for (i = 0; i < 256; i++)
  {
    dist = 0.0;
    for (j = 0; j < ORDER; j++) {
      temp = x[j] - *p_dico++;
      dist += w[j] * temp * temp;
    }
    if (dist < dist_min)
    {
      dist_min = dist;
      index = i;
    }
  }

  /* quantized vector */
  p_dico = &dico_lsf_abs_8b[index * ORDER];
  for (j = 0; j < ORDER; j++) {
    lsfq[j] += *p_dico++;
  }

  return index;
}



