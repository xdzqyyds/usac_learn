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
$Id: util_td.c,v 1.1.1.1 2009-05-29 14:10:21 mul Exp $
*******************************************************************************/


#include <math.h>
#include "proto_func.h"
#include "cnst.h"
/*-----------------------------------------------------------*
 * procedure set_zero                                        *
 * ~~~~~~~~~~~~~~~~~~                                        *
 * Set a vector x[] of dimension n to zero.                  *
 *-----------------------------------------------------------*/
void set_zero(float *x, int n)
{
  int i;
  for (i = 0; i < n; i++) {
    x[i] = 0.0;
  }
  return;
}
/*-----------------------------------------------------------*
 * procedure   mvr2r:                                        *
 *             ~~~~~~                                        *
 *  Transfer the contents of the vector x[] (real format)    *
 *  to the vector y[] (real format)                          *
 *-----------------------------------------------------------*/
void mvr2r(
  const float x[],       /* input : input vector  */
  float y[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
   y[i] = x[i];
  }
  return;
}
/*-----------------------------------------------------------*
 * procedure   mvs2s:                                        *
 *             ~~~~~~                                        *
 *  Transfer the contents of the vector x[] (short format)   *
 *  to the vector y[] (short format)                         *
 *-----------------------------------------------------------*/
void mvs2s(
  short x[],       /* input : input vector  */
  short y[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
    y[i] = x[i];
  }
  return;
}
/*-----------------------------------------------------------*
 * procedure   mvi2i:                                        *
 *             ~~~~~~                                        *
 *  Transfer the contents of the vector x[] (int format)   *
 *  to the vector y[] (int format)                         *
 *-----------------------------------------------------------*/
void mvi2i(
  int x[],         /* input : input vector  */
  int y[],         /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
    y[i] = x[i];
  }
  return;
}
/*-----------------------------------------------------------*
 * procedure   mvr2s:                                        *
 *             ~~~~~~                                        *
 *  Transfer the contents of the vector x[] (real format)    *
 *  to the vector y[] (short format)                         *
 *-----------------------------------------------------------*/
void mvr2s(
  float x[],       /* input : input vector  */
  short y[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  float temp;
  for (i = 0; i < n; i++) {
    temp = x[i];
    temp = (float)floor(temp + 0.5);
    if (temp >  32767.0 ) temp =  32767.0;
    if (temp < -32768.0 ) temp = -32768.0;
    y[i] = (short)temp;
  }
  return;
}
/*-----------------------------------------------------------*
 * procedure   mvs2r:                                        *
 *             ~~~~~~                                        *
 *  Transfer the contents of the vector x[] (short format)   *
 *  to the vector y[] (reel format)                          *
 *-----------------------------------------------------------*/
void mvs2r(
  short x[],       /* input : input vector  */
  float y[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
    y[i] = (float)x[i];
  }
  return;
}


/*----------------------------------------------------------*
 * procedure pessimize:                                     *
 * A fake do-nothing routine to break program flow and     *
 * prevent optimisation                                     *
 *----------------------------------------------------------*/
void pessimize()
{
	  return;
}

/*-----------------------------------------------------------*
 * procedure   addr2r:                                       *
 *             ~~~~~~                                        *
 *  Add the contents of the vector x[] (real format)         *
 *  and the vector y[] (real format)                         *
 *  to the vector y[] (real format)                          *
 *-----------------------------------------------------------*/
void addr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
   z[i] = x[i]+y[i];
  }
  return;
}

/*-----------------------------------------------------------*
 * procedure   multr2r:                                       *
 *             ~~~~~~                                        *
 *  Multiply the contents of the vector x[] (real format)         *
 *  with the vector y[] (real format)                         *
 *  to the vector y[] (real format)                          *
 *-----------------------------------------------------------*/
void multr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
   z[i] = x[i]*y[i];
  }
  return;
}

/*-----------------------------------------------------------*
 * procedure   subr2r:                                       *
 *             ~~~~~~                                        *
 *  Substract the contents of the vector x[] (real format)         *
 *  with the vector y[] (real format)                         *
 *  to the vector y[] (real format)                          *
 *-----------------------------------------------------------*/
void subr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
   z[i] = x[i]-y[i];
  }
  return;
}

/*-----------------------------------------------------------*
 * procedure   smulr2r:                                       *
 *             ~~~~~~                                        *
 *  Multiply the contents of the vector x[] (real format)         *
 *  with the factor a (real format)                         *
 *  to the vector y[] (real format)                          *
 *-----------------------------------------------------------*/
void smulr2r(
  float a,         /* input : input factor  */
  float x[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n            /* input : vector size   */
)
{
  int i;
  for (i = 0; i < n; i++) {
   z[i] = (float)a*x[i];
  }
  return;
}

