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



#ifndef _hufftables_h_
#define _hufftables_h_

#include "sac_reshuffinit.h"

extern	Huffman		s_book1[];
extern	Huffman		s_book2[];
extern	Huffman		s_book3[];
extern	Huffman		s_book4[];
extern	Huffman		s_book5[];
extern	Huffman		s_book6[];
extern	Huffman		s_book7[];
extern	Huffman		s_book8[];
extern	Huffman		s_book9[];
extern	Huffman		s_book10[];
extern	Huffman		s_book11[];
extern	Huffman		s_bookscl[];

typedef struct {
    int	    samp_rate;
    int	    nsfb1024;
    short*  SFbands1024;
    int	    nsfb128;
    short*  SFbands128;
} SR_Info;

extern	SR_Info		s_samp_rate_info[(1<<LEN_SAMP_IDX)] ;

#endif
