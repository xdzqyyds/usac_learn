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
$Id: enc_util.c,v 1.2 2010-03-12 06:54:40 mul Exp $
*******************************************************************************/


/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#include <math.h>
#include <memory.h>
#include "typedef.h"
#include "int3gpp.h"

#ifdef WIN32
#pragma warning( disable : 4310)
#endif
#define M      16
#define MAX_16 (Word16)0x7FFF
#define MIN_16 (Word16)0x8000
#define MAX_31 (Word32)0x3FFFFFFF
#define MIN_31 (Word32)0xC0000000
#define L_FRAME16k   320     /* Frame size at 16kHz         */
#define L_SUBFR16k   80      /* Subframe size at 16kHz      */
#define L_SUBFR      64      /* Subframe size               */
#define M16k         20      /* Order of LP filter          */
#define L_WINDOW     384     /* window size in LP analysis  */
#define PREEMPH_FAC  0.68F   /* preemphasis factor          */
#define L_WINDOW_PLUS 512    /* 448 low rate, 512 using EXTENSION_VA */

/***
 *needed for ACELP+
 *
 */
/*
 * E_UTIL_random
 *
 * Parameters:
 *    seed        I/O: seed for random number
 *
 * Function:
 *    Signed 16 bits random generator.
 *
 * Returns:
 *    random number
 */
Word16 E_UTIL_random(Word16 *seed)
{
   /*static Word16 seed = 21845;*/
   *seed = (Word16) (*seed * 31821L + 13849L);
   return(*seed);
}


/*
 * E_UTIL_hp50_12k8
 *
 * Parameters:
 *    signal       I/O: signal
 *    lg             I: lenght of signal
 *    mem          I/O: filter memory [6]
 *
 * Function:
 *    2nd order high pass filter with cut off frequency at 50 Hz.
 *
 *    Algorithm:
 *
 *    y[i] = b[0]*x[i] + b[1]*x[i-1] + b[2]*x[i-2]
 *                     + a[1]*y[i-1] + a[2]*y[i-2];
 *
 *    b[3] = {0.989501953f, -1.979003906f, 0.989501953f};
 *    a[3] = {1.000000000F,  1.978881836f,-0.966308594f};
 *
 *
 * Returns:
 *    void
 */
void E_UTIL_hp50_12k8(Float32 signal[], Word32 lg, Float32 mem[])
{
   Word32 i;
   Float32 x0, x1, x2, y0, y1, y2;
   y1 = mem[0];
   y2 = mem[1];
   x0 = mem[2];
   x1 = mem[3];
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
   mem[0] = ((y1 > 1e-10) | (y1 < -1e-10)) ? y1 : 0;
   mem[1] = ((y2 > 1e-10) | (y2 < -1e-10)) ? y2 : 0;
   mem[2] = ((x0 > 1e-10) | (x0 < -1e-10)) ? x0 : 0;
   mem[3] = ((x1 > 1e-10) | (x1 < -1e-10)) ? x1 : 0;
   return;
}


void E_UTIL_f_preemph(Float32 *signal, Float32 mu, Word32 L, Float32 *mem)
{
   Word32 i;
   Float32 temp;
   temp = signal[L - 1];
   for (i = L - 1; i > 0; i--)
   {
      signal[i] = signal[i] - mu * signal[i - 1];
   }
   signal[0] -= mu * (*mem);
   *mem = temp;
   return;
}

/*
 * E_UTIL_deemph
 *
 * Parameters:
 *    signal       I/O: signal
 *    mu             I: deemphasis factor
 *    L              I: vector size
 *    mem          I/O: memory (signal[-1])
 *
 * Function:
 *    Filtering through 1/(1-mu z^-1)
 *    Signal is divided by 2.
 *
 * Returns:
 *    void
 */
void E_UTIL_deemph(Float32 *signal, Float32 mu, Word32 L, Float32 *mem)
{
   Word32 i;
   signal[0] = signal[0] + mu * (*mem);
   for (i = 1; i < L; i++)
   {
      signal[i] = signal[i] + mu * signal[i - 1];
   }
   *mem = signal[L - 1];
   if ((*mem < 1e-10) & (*mem > -1e-10))
   {
      *mem = 0;
   }
   return;
}

/*
 * E_UTIL_synthesis
 *
 * Parameters:
 *    a              I: LP filter coefficients
 *    m              I: order of LP filter
 *    x              I: input signal
 *    y              O: output signal
 *    lg             I: size of filtering
 *    mem          I/O: initial filter states
 *    update_m       I: update memory flag
 *
 * Function:
 *    Perform the synthesis filtering 1/A(z).
 *    Memory size is always M.
 *
 * Returns:
 *    void
 */
void E_UTIL_synthesis(Float32 a[], Float32 x[], Float32 y[], Word32 l,
                      Float32 mem[], Word32 update_m)
{
   Float32 buf[L_FRAME16k + M16k];     /* temporary synthesis buffer */
   Float32 s;
   Float32 *yy;
   Word32 i, j;
   /* copy initial filter states into synthesis buffer */
   memcpy(buf, mem, M * sizeof(Float32));
   yy = &buf[M];
   for (i = 0; i < l; i++)
   {
      s = x[i];
      for (j = 1; j <= M; j += 4)
      {
         s -= a[j] * yy[i - j];
         s -= a[j + 1] * yy[i - (j + 1)];
         s -= a[j + 2] * yy[i - (j + 2)];
         s -= a[j + 3] * yy[i - (j + 3)];
      }
      yy[i] = s;
      y[i] = s;
   }
   /* Update memory if required */
   if (update_m)
   {
      memcpy(mem, &yy[l - M], M * sizeof(Float32));
   }
   return;
}

void E_UTIL_synthesisPlus(Float32 a[], Word32 m, Float32 x[], Float32 y[], Word32 l,
                      Float32 mem[], Word32 update_m)
{
   Float32 buf[L_FRAME16k + M16k];     /* temporary synthesis buffer */
   Float32 s;
   Float32 *yy;
   Word32 i, j;
   /* copy initial filter states into synthesis buffer */
   memcpy(buf, mem, m * sizeof(Float32));
   yy = &buf[m];
   for (i = 0; i < l; i++)
   {
     s = x[i];
      for (j = 1; j <= m; j++)
      {
        s -= a[j] * yy[i - j];
      }
      yy[i] = s;
      y[i] = s;
   }
   /* Update memory if required */
   if (update_m)
   {
      memcpy(mem, &yy[l - m], m * sizeof(Float32));
   }
   return;
}


/*
 * E_UTIL_residu
 *
 * Parameters:
 *    a           I: LP filter coefficients (Q12)
 *    x           I: input signal (usually speech)
 *    y           O: output signal (usually residual)
 *    l           I: size of filtering
 *
 * Function:
 *    Compute the LP residual by filtering the input speech through A(z).
 *    Order of LP filter = M.
 *
 * Returns:
 *    void
 */
void E_UTIL_residu(Float32 *a, Float32 *x, Float32 *y, Word32 l)
{
   Float32 s;
   Word32 i;
   for (i = 0; i < l; i++)
   {
      s = x[i];
      s += a[1] * x[i - 1];
      s += a[2] * x[i - 2];
      s += a[3] * x[i - 3];
      s += a[4] * x[i - 4];
      s += a[5] * x[i - 5];
      s += a[6] * x[i - 6];
      s += a[7] * x[i - 7];
      s += a[8] * x[i - 8];
      s += a[9] * x[i - 9];
      s += a[10] * x[i - 10];
      s += a[11] * x[i - 11];
      s += a[12] * x[i - 12];
      s += a[13] * x[i - 13];
      s += a[14] * x[i - 14];
      s += a[15] * x[i - 15];
      s += a[16] * x[i - 16];
      y[i] = s;
   }
   return;
}

/* This is a modified residu to suppot AMR-WB+ */
void E_UTIL_residuPlus(Float32 *a, Word32 m, Float32 *x, Float32 *y, Word32 l)
{
   Float32 s;
   Word32   i, j;
   for (i = 0; i < l; i++)
   {
      s = x[i];
      for (j = 1; j <= m; j++) {
         s += a[j]*x[i-j];
      } 
      y[i] = s;
   }
   return;
}


void E_UTIL_f_convolve(Float32 x[], Float32 h[], Float32 y[])
{
   Float32 temp;
   Word32 i, n;
   for (n = 0; n < L_SUBFR; n += 2)
   {
      temp = 0.0;
      for (i = 0; i <= n; i++)
      {
         temp += x[i] * h[n - i];
      }
      y[n] = temp;
      temp = 0.0;
      for (i = 0; i <= (n + 1); i += 2)
      {
         temp += x[i] * h[(n + 1) - i];
         temp += x[i + 1] * h[n - i];
      }
      y[n + 1] = temp;
   }
   return;
}


void E_UTIL_autocorrPlus(
  float *x,         /* input : input signal            */
  float *r,         /* output: autocorrelations vector */
  int m,            /* input : order of LP filter      */
  int n,            /* input : window size             */
  float *fh         /* input : analysis window         */
)
{
  float t[L_WINDOW_PLUS];
  float s;
  Word16 i, j;
  for (i = 0; i < n; i++) {
    t[i] = x[i]*fh[i];
  }
  for (i = 0; i <= m; i++)
  {
    s = 0.0;
    for (j = 0; j < n-i; j++) {
      s += t[j]*t[j+i];
    }
    r[i] = s;
  }
  if (r[0] < 1.0) {
    r[0] = 1.0;
  }
  return;
}

/*
 * E_UTIL_saturate
 *
 * Parameters:
 *    inp        I: 32-bit number
 *
 * Function:
 *    Saturation to 16-bit number
 *
 * Returns:
 *    16-bit number
 */
Word16 E_UTIL_saturate(Word32 inp)
{
   Word16 out;
   if ((inp < MAX_16) & (inp > MIN_16))
   {
      out = (Word16)inp;
   }
   else
   {
      if (inp > 0)
      {
         out = MAX_16;
      }
      else
      {
         out = MIN_16;
      }
   }
   return(out);
}
