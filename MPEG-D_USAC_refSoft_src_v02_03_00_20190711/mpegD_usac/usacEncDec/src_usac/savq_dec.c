/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:


and edited by: Philippe Gournay, Bruno Bessette

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

$Id: savq_dec.c,v 1.3 2010-10-28 18:57:32 mul Exp $
*******************************************************************************/

/*---------------------------------------------------------------------------*
 *         SPLIT ALGEBRAIC VECTOR QUANTIZER BASED ON RE8 LATTICE             *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "re8.h"
#include "proto_func.h"


void SAVQ_dec(
  int *indx,    /* input:  index[] (4 bits per words)      */
  int *nvecq,   /* output: vector quantized                */
  int Nsv)      /* input:  number of subvectors (lg=Nsv*8) */  
{			  
  int    i, l, n, nq, nk, pos, c[8], kv[8];
  long   I;
  
  pos = Nsv;

  /* decode all subvectors */
  for (l=0; l<Nsv; l++) {
    nq = indx[l];        /* quantizer number (0,2,3..n) */

    nk = 0;
    n = nq;
    if (nq > 4)
    {
      nk = (nq-3)>>1;
      n = nq - nk*2;
    }

    if (nq > 0) {
      /* read base codebook index (I) */
      I = indx[pos++];
      
      /* read Voronoi indices (k[]) */
      for (i=0; i<8; i++) {
        kv[i] = indx[pos++];
      }
    } else {
      I = 0;
      for (i=0;i<8;i++) {
        kv[i] = 0;
      }
    }

    /* multi-rate RE8 decoder */
    RE8_dec(nq, I, kv, c);

    /* write decoded RE8 vector */
    for (i=0; i<8; i++) {
      nvecq[(l*8)+i] = c[i];
    }
  }

  return;
}
