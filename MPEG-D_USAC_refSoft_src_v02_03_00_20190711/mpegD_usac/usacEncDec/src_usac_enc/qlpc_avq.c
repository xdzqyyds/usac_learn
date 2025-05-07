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

$Id: qlpc_avq.c,v 1.3 2010-10-21 15:50:41 mul Exp $
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "proto_func.h"


#define M 16

void qlpc_avq(
   float *LSF,       /* (i) Input LSF vectors              */
   float *LSF_Q,     /* (o) Quantized LFS vectors          */
   int lpc0,         /* (i) LPC0 vector is present         */
   int *index,       /* (o) Quantization indices           */
   int *nb_indices,  /* (o) Number of quantization indices */
   int *nbbits       /* (o) Number of quantization bits    */
)
{
   int i;
   float lsfq[M];
   int *tmp_index, indxt[100], nbits, nbt, nit;

   tmp_index = &index[0];
   *nb_indices = 0;
   *nbbits = 0;


   /* Quantize LPC4 using 1st stage stochastique codebook and 2nd stage AVQ */

   for (i=0; i<M; i++) LSF_Q[3*M+i] = 0.0f;

   tmp_index[0] = vlpc_1st_cod(&LSF[3*M], &LSF_Q[3*M]);

   nbt = vlpc_2st_cod(&LSF[3*M], &LSF_Q[3*M], &tmp_index[1], 0);
   nit = 1 + get_num_prm(index[1], index[2]);

   tmp_index += nit;
   *nb_indices += nit;
   *nbbits += 8 + nbt; /*CHECK IF BIT NUMBER INCLUDES CODEBOOK NUMBER*/

   /* Handle LPC0 case */

   if (lpc0)
   {
      *tmp_index = 0; /* mode abs */
      tmp_index++;
      *nb_indices += 1;
      *nbbits +=1;

      /* LPC0: Abs? */

      for (i=0; i<M; i++) LSF_Q[-M+i] = 0.0f;

      tmp_index[0] = vlpc_1st_cod(&LSF[-M], &LSF_Q[-M]);

      nbits = vlpc_2st_cod(&LSF[-M], &LSF_Q[-M], &tmp_index[1], 0);
      nbt = 8 + nbits;
      nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

      /* LPC0: RelR? */

      for (i=0; i<M; i++) lsfq[i] = LSF_Q[3*M+i];

      nbits = vlpc_2st_cod(&LSF[-M], &lsfq[0], indxt, 3);

      if (nbits < nbt)
      {
         nbt = nbits;
         nit = get_num_prm(indxt[0], indxt[1]);
         tmp_index[-1] = 1; /* mode relR */
         for (i=0; i<M; i++)   LSF_Q[-M+i] = lsfq[i];
         for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
      }

      tmp_index += nit;
      *nb_indices += nit;
      *nbbits += nbt;
   }

   /* Quantize the other LPC filters */

   /*** LPC2 ***/

   *tmp_index = 0; /* mode */
   tmp_index++;
   *nb_indices +=1;
   *nbbits +=1;

   /* LPC2: Abs? */

   for (i=0; i<M; i++) LSF_Q[M+i] = 0.0f;

   tmp_index[0] = vlpc_1st_cod(&LSF[M], &LSF_Q[M]);

   nbits = vlpc_2st_cod(&LSF[M], &LSF_Q[M], &tmp_index[1], 0);
   nbt = 8 + nbits;
   nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

   /* LPC2: RelR? */

   for (i=0; i<M; i++) lsfq[i] = LSF_Q[3*M+i];

   nbits = vlpc_2st_cod(&LSF[M], &lsfq[0], indxt, 3);

   if (nbits < nbt)
   {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 1; /* mode */
      for (i=0; i<M; i++)   LSF_Q[M+i] = lsfq[i];
      for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
   }

   tmp_index += nit;
   *nb_indices += nit;
   *nbbits += nbt;

   /*** LPC1 ***/

   *tmp_index = 0; /* mode */
   tmp_index++;
   *nb_indices +=1;

   /* LPC1: Abs? */

   for (i=0; i<M; i++) LSF_Q[i] = 0.0f;

   tmp_index[0] = vlpc_1st_cod(&LSF[0], &LSF_Q[0]);

   nbits = vlpc_2st_cod(&LSF[0], &LSF_Q[0], &tmp_index[1], 0);
   nbt = 2 + 8 + nbits;
   nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

   /* LPC1: mid 0bit? */

   for (i=0; i<M; i++) lsfq[i] = 0.5f*(LSF_Q[-M+i] + LSF_Q[M+i]);

   nbits = vlpc_2st_cod(&LSF[0], lsfq, indxt, 1);

   if (nbits < 10) /* In practice, <10 means 0 */
   {
      nbt = 2;
      nit = 0;
      tmp_index[-1] = 1; /* mode */
      for (i=0; i<M; i++)   LSF_Q[i] = lsfq[i];
   }

   /* LPC1: relR? */

   for (i=0; i<M; i++) lsfq[i] = LSF_Q[M+i];

   nbits = vlpc_2st_cod(&LSF[0], lsfq, indxt, 2);
   nbits += 1;

   if (nbits < nbt)
   {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 2; /* mode */
      for (i=0; i<M; i++)   LSF_Q[i] = lsfq[i];
      for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
   }

   tmp_index += nit;
   *nb_indices += nit;
   *nbbits += nbt;

   /*** LPC3 ***/

   *tmp_index = 0; /* mode */
   tmp_index++;
   *nb_indices +=1;

   /* LPC3: Abs? */

   for (i=0; i<M; i++) LSF_Q[2*M+i] = 0.0f;

   tmp_index[0] = vlpc_1st_cod(&LSF[2*M], &LSF_Q[2*M]);

   nbits = vlpc_2st_cod(&LSF[2*M], &LSF_Q[2*M], &tmp_index[1], 0);
   nbt = 2 + 8 + nbits;
   nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

   /* LPC3: mid? */

   for (i=0; i<M; i++) lsfq[i] = 0.5f*(LSF_Q[M+i] + LSF_Q[3*M+i]);

   nbits = vlpc_2st_cod(&LSF[2*M], lsfq, indxt, 1);
   nbits += 1;

   if (nbits < nbt)
   {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 1; /* mode */
      for (i=0; i<M; i++)   LSF_Q[2*M+i] = lsfq[i];
      for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
   }

   /* LPC3: relL? */

   for (i=0; i<M; i++) lsfq[i] = LSF_Q[M+i];

   nbits = vlpc_2st_cod(&LSF[2*M], lsfq, indxt, 2);
   nbits += 3;

   if (nbits < nbt)
   {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 2; /* mode */
      for (i=0; i<M; i++)   LSF_Q[2*M+i] = lsfq[i];
      for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
   }

   /* LPC3: relR? */

   for (i=0; i<M; i++) lsfq[i] = LSF_Q[3*M+i];

   nbits = vlpc_2st_cod(&LSF[2*M], lsfq, indxt, 2);
   nbits += 3;

   if (nbits < nbt)
   {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 3; /* mode */
      for (i=0; i<M; i++)   LSF_Q[2*M+i] = lsfq[i];
      for (i=0; i<nit; i++) tmp_index[i] = indxt[i];
   }

   tmp_index += nit;
   *nb_indices += nit;
   *nbbits += nbt;

   return;
}
