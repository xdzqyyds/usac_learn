/***********************************************************************
 
This software module was originally developed by

 Ralph Sperschneider (Fraunhofer IIS)

and edited by

 -

in the course of development of ISO/IEC 13818 and/or ISO/IEC 14496 for
reference purposes and its performance may not have been optimized. This
software module is an implementation of one or more tools as specified
by ISO/IEC 13818 and/or ISO/IEC 14496.

ISO/IEC gives You a royalty-free, worldwide, non-exclusive, copyright
license to copy, distribute, and make derivative works of this software
module or modifications thereof for use in implementations of ISO/IEC
13818 and/or ISO/IEC 14496 in products that satisfy conformance criteria
(if any).

Those intending to use this software module in products are advised that
its use may infringe existing patents. ISO/IEC have no liability for use
of this software module or modifications thereof.

Copyright is not released for products that do not conform to
audiovisual and image-coding related ITU Recommendations and/or ISO/IEC
International Standards.

Fraunhofer IIS retains full right to modify and use the code for its own
purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to
MPEG-related ITU Recommendations and/or ISO/IEC International Standards.

This copyright notice must be included in all copies or derivative
works. Copyright (c) ISO/IEC 2007.

$Id: extension_type.h,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $

***********************************************************************/


#ifndef __EXTENSION_TYPE_H
#define __EXTENSION_TYPE_H


enum {
  EXT_FILL           =  0,    /* '0000' */
  EXT_FILL_DATA      =  1,    /* '0001'+ */
  EXT_DATA_ELEMENT   =  2,    /* '0010' */
  EXT_DYNAMIC_RANGE  = 11,    /* '1011 + */
  EXT_SAC_DATA       = 12,    /* '1100' */
  EXT_SBR_DATA       = 13,    /* '1101'+ */
  EXT_SBR_DATA_CRC   = 14     /* '1110'+ */
};


#endif /* __EXTENSION_TYPE_H */
