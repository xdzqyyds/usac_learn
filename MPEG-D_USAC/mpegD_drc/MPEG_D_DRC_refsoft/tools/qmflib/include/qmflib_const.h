/*******************************************************************************
This software module was originally developed by

Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2007.
*******************************************************************************/

#ifndef __QMFLIB_CONST_H__
#define __QMFLIB_CONST_H__


#define QMFLIB_PI                             3.14159265358979f
#define QMFLIB_MAX_NUM_QMF_BANDS              128
#define QMFLIB_MAX_HYBRID_BANDS               ( QMFLIB_MAX_NUM_QMF_BANDS - 3 + 16 )
#define QMFLIB_QMF_BANDS_TO_HYBRID            3
#define QMFLIB_PROTO_LEN                      13
#define QMFLIB_BUFFER_LEN_LF                  ( QMFLIB_PROTO_LEN - 1 ) + 1
#define QMFLIB_BUFFER_LEN_HF                  ( ( QMFLIB_PROTO_LEN - 1 ) / 2 ) + 1
#define QMFLIB_MAX_HYBRID_ONLY_BANDS_PER_QMF  8


#endif
