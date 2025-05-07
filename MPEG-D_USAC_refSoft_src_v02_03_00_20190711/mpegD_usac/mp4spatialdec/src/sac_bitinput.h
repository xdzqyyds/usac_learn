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



#ifndef _BITINPUT_H_
#define _BITINPUT_H_

#include "sac_dec_interface.h"
#include "sac_config.h"

typedef struct s_tagBitInput *HANDLE_S_BITINPUT;



long s_getbits(int n);
long s_GetBits(HANDLE_S_BITINPUT input, long n);

int s_byte_align(void);
int s_byteAlign(HANDLE_S_BITINPUT input);

long s_getNumBits(HANDLE_S_BITINPUT input);

void s_set_log_level(int id, int add_bits_num, int add_bits);

HANDLE_S_BITINPUT	s_bitinput_open(HANDLE_BYTE_READER byteReader);

int s_bitinput_resetCounter(HANDLE_S_BITINPUT input);

void s_bitinput_close(HANDLE_S_BITINPUT input);

#endif
