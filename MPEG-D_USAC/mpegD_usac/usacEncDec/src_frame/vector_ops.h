/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author: Markus Multrus

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
*******************************************************************************/
#ifndef __INCLUDED_VECTOR_OPS_H
#define __INCLUDED_VECTOR_OPS_H

void vcopy( const float src[], float dest[], int inc_src, int inc_dest, int vlen );

void vcopy_db( const double src[], double dest[], int inc_src, int inc_dest, int vlen );

void vmult( const float src1[], const float src2[], float dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vmult_db( const double src1[], const double src2[], double dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vdiv( const float src1[], const float src2[], float dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vdiv_db( const double src1[], const double src2[], double dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vadd( const float src1[], const float src2[], float dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vadd_db( const double src1[], const double src2[], double dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vsub( const float src1[], const float src2[], float dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

void vsub_db( const double src1[], const double src2[], double dest[], int inc_src1, int inc_src2, int inc_dest, int vlen );

#endif
