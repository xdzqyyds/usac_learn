/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrcrc.h,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  CRC checking routines $Revision: 1.1.1.1 $
*/
#ifndef __CT_SBRCRC_H
#define __CT_SBRCRC_H

#include "ct_sbrbitb.h"

/* G(x) = x^10 + x^9 + x^5 + x^4 + x + 1 */
#define CRCPOLY  0x0233
#define CRCMASK  0x0200
#define CRCSTART 0x0000
#define CRCRANGE 0x03FF

int SbrCrcCheck (HANDLE_BIT_BUFFER hBitBuf, unsigned long NrCrcBits);

#endif
