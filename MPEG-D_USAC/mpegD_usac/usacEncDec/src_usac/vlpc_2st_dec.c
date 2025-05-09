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

$Id: vlpc_2st_dec.c,v 1.2 2010-03-12 06:54:41 mul Exp $
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "proto_func.h"


#define ORDER    16
#define LSF_GAP  50.0f
#define FREQ_MAX 6400.0f
#define FREQ_DIV 400.0f

void lsf_weight_2st(float *lsfq, float *w, int mode);

void reorder_lsf(float *lsf, float min_dist, int n);

void SAVQ_dec(
  int *indx,    /* input:  index[] (4 bits per words)      */
  int *nvecq,   /* output: vector quantized                */
  int Nsv);      /* input:  number of subvectors (lg=Nsv*8) */

void vlpc_2st_dec( 
  float *lsfq,    /* i/o:    i:1st stage   o:1st+2nd stage   */
  int *indx,      /* input:  index[] (4 bits per words)      */
  int mode)       /* input:  0=abs, >0=rel                   */
{
  int i;
  float w[ORDER];
  int xq[ORDER];

  /* weighting from the 1st stage */
  lsf_weight_2st(lsfq, w, mode);

  /* quantize */
  SAVQ_dec(indx, xq, 2);

  /* quantized lsf */
  for (i=0; i<ORDER; i++) lsfq[i] += (w[i]*(float)xq[i]);

  /* reorder */
  reorder_lsf(lsfq, LSF_GAP, ORDER);

  return;
}
