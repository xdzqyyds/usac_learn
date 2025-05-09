
/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Fraunhofer IIS

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

$Id: bits.c,v 1.3 2010-10-28 18:57:32 mul Exp $
*******************************************************************************/

#include "../src_usac/acelp_plus.h"
#define MASK      0x0001
/*---------------------------------------------------------------------------*
 * function:  bin2int                                                        *
 *            ~~~~~~~                                                        *
 * Read "no_of_bits" bits from the array bitstream[] and convert to integer  *
 *--------------------------------------------------------------------------*/
int bin2int(           /* output: recovered integer value              */
  int   no_of_bits,    /* input : number of bits associated with value */
  short *bitstream     /* input : address where bits are read          */
)
{
   int   value, i;
   value = 0;
   for (i = 0; i < no_of_bits; i++)	
   {
     value <<= 1;
     value += (int)((*bitstream++) & MASK);
   }
   return(value);
}
/*---------------------------------------------------------------------------*
 * function:  int2bin                                                        *
 *            ~~~~~~~                                                        *
 * Convert integer to binary and write the bits to the array bitstream[].    *
 * Most significant bits (MSB) are output first                              *
 *--------------------------------------------------------------------------*/
void int2bin(
  int   value,         /* input : value to be converted to binary      */
  int   no_of_bits,    /* input : number of bits associated with value */
  short *bitstream     /* output: address where bits are written       */
)
{
   short *pt_bitstream;
   int   i;
   pt_bitstream = bitstream + no_of_bits;
   for (i = 0; i < no_of_bits; i++)
   {
     *--pt_bitstream = (short)(value & MASK);
     value >>= 1;
   }
}

int get_num_prm(int qn1, int qn2)
{
  return 2 + ((qn1 > 0) ? 9 : 0) + ((qn2 > 0) ? 9 : 0);
}
