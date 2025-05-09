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

$Id: vlpc_2st_cod.c,v 1.2 2010-03-12 06:54:44 mul Exp $
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "proto_func.h"

#define ORDER    16
#define LSF_GAP  50.0f


/* external function */
void SAVQ_cod(  
  float *nvec,  /* input:  vector to quantize (normalized) */
  int *nvecq,   /* output: quantized vector                */
  int *indx,    /* output: index[] (4 bits per words)      */
  int Nsv);     /* input:  number of subvectors (lg=Nsv*8) */  
void SAVQ_dec(
  int *indx,    /* input:  index[] (4 bits per words)      */
  int *nvecq,   /* output: vector quantized                */
  int Nsv);     /* input:  number of subvectors (lg=Nsv*8) */  


int vlpc_2st_cod( /* output: number of allocated bits        */ 
  float *lsf,     /* input:  normalized vector to quantize   */
  float *lsfq,    /* i/o:    i:1st stage   o:1st+2nd stage   */
  int *indx,      /* output: index[] (4 bits per words)      */
  int mode)       /* input:  0=abs, >0=rel                   */
{
  int    i, nbits;
  float  w[ORDER], x[ORDER], tmp;
  int nq, xq[ORDER];

  /* 0 bit with true weighting: save 0.5 bit */

  lsf_weight_2st(lsf, w, 1);

  for (i=0; i<ORDER; i++) x[i] = lsf[i] - lsfq[i];
  for (i=0; i<ORDER; i++) x[i] /= w[i];
 
  tmp = 0.0f;
  for (i=0; i<ORDER; i++) tmp += x[i]*x[i];

  if (tmp < 8.0f)
  {
    indx[0] = 0;
    indx[1] = 0;
    if ((mode == 0) || (mode == 3)) return(10);     /* 2*(2+3) */
    else if (mode == 1) return(2);                  /* 2*1     */
    else return (6);                                /* 2*(2+1) */
  }

  /* weighting from the 1st stage */
  lsf_weight_2st(lsfq, w, mode);

  /* find lsf residual */
  for (i=0; i<ORDER; i++) x[i] = lsf[i] - lsfq[i];

  /* scale the residual */
  for (i=0; i<ORDER; i++) x[i] /= w[i];

  /* quantize */
  SAVQ_cod(x, xq, indx, 2);

  /* quantized lsf */
  for (i=0; i<ORDER; i++) lsfq[i] += (w[i]*(float)xq[i]);

  /* total number of bits using entropic code to index the quantizer number */
  nbits = 0;
  for (i=0; i<2; i++)
  {
    nq = indx[i];

    if ((mode == 0) || (mode == 3))    /* abs, rel2 */
	{
          nbits += (2+(nq*4));             /* 2 bits to specify Q2,Q3,Q4,ext */
          if (nq > 6) nbits += nq-3;       /* unary code (Q7=1110, ...) */
          else if (nq > 4) nbits += nq-4;  /* Q5=0, Q6=10 */
          else if (nq == 0) nbits += 3;    /* Q0=110 */
	}
    else if (mode == 1)                /* mid */
	{
          nbits += nq*5;                   /* unary code (Q0=0, Q1=10, ...) */
      if (nq == 0) nbits += 1;
	}
    else {                             /* rel1 */
      nbits += (2+(nq*4));             /* 2 bits to specify Q2,Q3,Q4,ext */
      if (nq == 0) nbits += 1;         /* Q0 = 0 */
      else if (nq > 4) nbits += nq-3;  /* unary code (Q5=10, Q6=110, ...) */
	}
  }

  /* reorder */
  reorder_lsf(lsfq, LSF_GAP, ORDER);

  return(nbits);
}
