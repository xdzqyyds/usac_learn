/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#ifndef _intrins_h_
#define _intrins_h_

#include "sac_resdefs.h"

extern	long		total1;
extern	long		total2;


void		s_byteclr(byte *ip1, int cnt);
long		s_f2ir(Float x);
void		s_fltclr(Float *dp1, int cnt);
void		s_fltcpy(Float *dp1, Float *dp2, int cnt);
void		s_fltset(Float *dp1, Float dval, int cnt);
void		s_intclr(int *ip1, int cnt);
void		s_intcpy(int *ip1, int *ip2, int cnt);
void*		s_mal1(long size);
void            s_free1(void* pointer);
void*		s_mal2(long size);
void		s_shortclr(short *ip1, int cnt);

#endif
