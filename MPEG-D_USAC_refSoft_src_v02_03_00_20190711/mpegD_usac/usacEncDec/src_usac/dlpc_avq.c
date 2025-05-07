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

$Id: dlpc_avq.c,v 1.5 2011-03-11 15:18:59 mul Exp $
*******************************************************************************/

#define DEBUG

/* Prototypes */
#include "proto_func.h"

#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

/* Header files */

#include "td_frame.h"

/* Constants */

#define M 16

#define BFI_FAC 0.9f

extern const float mean_lsf_conc[16];

/***********************************************/
/* Variable bit-rate multiple LPC un-quantizer */
/***********************************************/

void dlpc_avq(
   td_frame_data *td,
   int lpc0,         /* (i)   LPC0 vector is present         */
   float *LSF_Q,     /* (o)   Quantized LSF vectors                      */
   float *past_lsfq, /* (i/o) Past quantized LSFs for bad frame handling */
   int mod[],        /* (i)   amr-wb+ coding mode (acelp/tcx20,40 or 80) */
   int bfi
)
{

  int i, j, nbi;
  int *p_index, q_type;

   if (bfi)
   {
      /* Frame loss concealment (could be improved) */

      if (lpc0)
      {
         /* Reset pas ISF values */
         for (i=0;i<M;i++)
         {
            LSF_Q[i-M] = past_lsfq[i] = mean_lsf_conc[i];
         }

      }

      for (i=0;i<M;i++)
      {
         LSF_Q[i] = BFI_FAC*past_lsfq[i] + (1.0f-BFI_FAC)*mean_lsf_conc[i];
      }

      for (j=1;j<4;j++)
      {
         for (i=0;i<M;i++)
         {
            LSF_Q[M*j+i] = BFI_FAC*LSF_Q[M*(j-1)+i] + (1.0f-BFI_FAC)*mean_lsf_conc[i];
         }
      }

      for (i=0;i<M;i++) past_lsfq[i] = LSF_Q[M*3+i];

      return;
   }

   p_index = td->lpc;

   /* Decode LPC4 */

   for (i=0; i<M; i++) LSF_Q[3*M+i] = 0.0f;

   vlpc_1st_dec(p_index[0], &LSF_Q[3*M]);
   p_index++;

   vlpc_2st_dec(&LSF_Q[3*M], &p_index[0], 0);
   nbi = get_num_prm(p_index[0], p_index[1]);

   p_index += nbi;

   /* Handle LPC0 case*/

   if (lpc0)
   {
      q_type = p_index[0];
      p_index++;

      if (q_type == 0)
      {
         /* LPC0: Abs */
         for (i=0; i<M; i++) LSF_Q[-M+i] = 0.0f;
         vlpc_1st_dec(p_index[0], &LSF_Q[-M]);
         p_index++;
         vlpc_2st_dec(&LSF_Q[-M], &p_index[0], 0);
      }
      else if (q_type == 1)
      {
         /* LPC0: RelR */
         for (i=0; i<M; i++) LSF_Q[-M+i] = LSF_Q[3*M+i];
         vlpc_2st_dec(&LSF_Q[-M], &p_index[0], 3);
      }
      else
      {
         printf("\nInvalid mode for LPC0 quantization\n");
         exit(0);
      }

      /* 2nd stage */
      nbi = get_num_prm(p_index[0], p_index[1]);

      p_index += nbi;
   }

   /* Decode remaining vectors using relative quantizer */

   if (mod[0]!=3)
   {

      /* Decode LPC2*/

      q_type = p_index[0];
      p_index++;

      if (q_type == 0)
      {
         /* LPC2: Abs */
         for (i=0; i<M; i++) LSF_Q[M+i] = 0.0f;
         vlpc_1st_dec(p_index[0], &LSF_Q[M]);
         p_index++;
         vlpc_2st_dec(&LSF_Q[M], &p_index[0], 0);
      }
      else if (q_type == 1)
      {
         /* LPC2: RelR */
         for (i=0; i<M; i++) LSF_Q[M+i] = LSF_Q[3*M+i];
         vlpc_2st_dec(&LSF_Q[M], &p_index[0], 3);
      }
      else
      {
         printf("\nInvalid mode for LPC1 quantization\n");
         exit(0);
      }

      nbi = get_num_prm(p_index[0], p_index[1]);

      p_index += nbi;

      if (mod[0]!=2)
      {
         /* Decode LPC1*/

         q_type = p_index[0];
         p_index++;

         if (q_type == 1)
         {
            /* LPC1: mid 0bit */
            for (i=0; i<M; i++) LSF_Q[i] = 0.5f*(LSF_Q[-M+i] + LSF_Q[M+i]); /*CHECK THAT: LPC0 not always available?*/
         }
         else
         {
            if (q_type == 0)
            {
               /* LPC1: Abs */
               for (i=0; i<M; i++) LSF_Q[i] = 0.0f;
               vlpc_1st_dec(p_index[0], &LSF_Q[0]);
               p_index++;
               vlpc_2st_dec(&LSF_Q[0], &p_index[0], 0);
            }
            else if (q_type == 2)
            {
               /* LPC1: RelR */
               for (i=0; i<M; i++) LSF_Q[i] = LSF_Q[M+i];
               vlpc_2st_dec(&LSF_Q[0], &p_index[0], 2);
            }
            else
            {
               printf("\nInvalid mode for LPC1 quantization\n");
               exit(0);
            }

            nbi = get_num_prm(p_index[0], p_index[1]);

            p_index += nbi;

         }

      }

      if (mod[2]!=2)
      {
         /* Decode LPC3 */

         q_type = p_index[0];
         p_index++;

         if (q_type == 0)
         {
            /* LPC3: Abs */
            for (i=0; i<M; i++) LSF_Q[2*M+i] = 0.0f;
            vlpc_1st_dec(p_index[0], &LSF_Q[2*M]);
            p_index++;
            vlpc_2st_dec(&LSF_Q[2*M], &p_index[0], 0);
         }
         else if (q_type == 1)
         {
            /* LPC3: mid */
            for (i=0; i<M; i++) LSF_Q[2*M+i] = 0.5f*(LSF_Q[M+i] + LSF_Q[3*M+i]);
            vlpc_2st_dec(&LSF_Q[2*M], &p_index[0], 1);
         }
         else if (q_type == 2)
         {
            /* LPC3: relL */
            for (i=0; i<M; i++) LSF_Q[2*M+i] = LSF_Q[M+i];
            vlpc_2st_dec(&LSF_Q[2*M], &p_index[0], 2);
         }
         else if (q_type == 3)
         {
            /* LPC3: relR */
            for (i=0; i<M; i++) LSF_Q[2*M+i] = LSF_Q[3*M+i];
            vlpc_2st_dec(&LSF_Q[2*M], &p_index[0], 2);
         }
         else
         {
            printf("\nInvalid mode for LPC3 quantization\n");
            exit(0);
         }

         nbi = get_num_prm(p_index[0], p_index[1]);

         p_index += nbi;

      }

   }

   /* Update past ISF values for future BFIs */
   
   for (i=0;i<M;i++) past_lsfq[i] = LSF_Q[M*3+i];

}


