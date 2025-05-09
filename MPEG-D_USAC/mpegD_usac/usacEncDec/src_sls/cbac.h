/*******************************************************************************
This software module was originally developed by

Institute for Infocomm Research and Fraunhofer IIS

and edited by

-

in the course of development of ISO/IEC 14496 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 14496. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 14496 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

#ifdef NOT_PUBLISHED

Assurance that the originally developed software module can be used (1) in
ISO/IEC 14496 once ISO/IEC 14496 has been adopted; and (2) to develop ISO/IEC
14496:
Institute for Infocomm Research and Fraunhofer IIS grant ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 14496 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 14496 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
14496. To the extent that Institute for Infocomm Research and Fraunhofer IIS 
own patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
14496 in a conforming product, Institute for Infocomm Research and Fraunhofer IIS 
will assure the ISO/IEC that they are willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 14496.

#endif

Institute for Infocomm Research and Fraunhofer IIS retain full right to
modify and use the code for their own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2005.
*******************************************************************************/



#ifndef _CBAC_H
#define _CBAC_H


typedef struct 
{
	int sampling_rate;
	int low_thr;
	int high_thr;
} FB_THR;

int cbac_model(int index,int frame_type, int sampling_rate,int layer, int *p_sig, int core_sig,int int_cx,int osf);

#endif


