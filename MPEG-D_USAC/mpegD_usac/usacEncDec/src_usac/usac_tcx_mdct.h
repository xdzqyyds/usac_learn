/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Markus Multrus

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
$Id: usac_tcx_mdct.h,v 1.2 2010-05-11 11:41:54 mul Exp $
*******************************************************************************/

#ifndef __INCLUDED_TCX_MDCT_H
#define __INCLUDED_TCX_MDCT_H

typedef struct T_TCX_MDCT *HANDLE_TCX_MDCT;
typedef struct T_TCX_DCT4 *HANDLE_TCX_DCT4;

typedef enum {

  TCX_MDCT_NO_ERROR = 0, 
  TCX_MDCT_FATAL_ERROR 

} TCX_MDCT_ERROR;

typedef enum {

  TCX_DCT_NO_ERROR = 0, 
  TCX_DCT_FATAL_ERROR 

} TCX_DCT_ERROR;

int 
TCX_MDCT_Open(HANDLE_TCX_MDCT *phTcxMdct);

int 
TCX_MDCT_Close(HANDLE_TCX_MDCT *phTcxMdct);

int 
TCX_MDCT_Apply(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r);

int 
TCX_MDCT_ApplyInverse(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r);

int
TCX_DCT4_Open(HANDLE_TCX_DCT4 *phDct4, int nLines);

int
TCX_DCT4_Close(HANDLE_TCX_DCT4 *phDct4);

int
TCX_DCT4_Apply(HANDLE_TCX_DCT4 hDct4, float *pIn, float *pOut);

HANDLE_TCX_DCT4
TCX_MDCT_DCT4_GetHandle(HANDLE_TCX_MDCT hTcxMdct, int nCoeff);

#endif
