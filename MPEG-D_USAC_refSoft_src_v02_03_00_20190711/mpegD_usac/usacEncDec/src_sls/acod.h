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



#ifndef AC_HEADER
#define AC_HEADER

#include <stdio.h>


typedef struct {
  unsigned char *stream;
  int stream_pt;
  long value;
  long low;
  long high;
  long fbits;
  int buffer;
  int bits_to_go;
  long total_bits;
  short is_lazy;
  int init;

  long bsize;
} ac_coder;


void ac_encoder_init (ac_coder *, unsigned char*);
void ac_encoder_done (ac_coder *);
void ac_decoder_init (ac_coder *, unsigned char *, long bsize);
void ac_decoder_done (ac_coder *);
long ac_bits (ac_coder *);
void ac_encoder_flush(ac_coder *ac);
void ac_encode_symbol (ac_coder *, int,int);
int  ac_decode_symbol (ac_coder *,int);
void ac_encoder_enable_lazy(ac_coder *ac);
void ac_decoder_enable_lazy(ac_coder *ac);
void ac_encoder_disable_lazy(ac_coder *ac);
void ac_decoder_disable_lazy(ac_coder *ac);
int ac_decode_symbol_silence (ac_coder *ac, int layer,int bin);
void ac_encode_symbol_silence (ac_coder *ac, int layer, int bin, int sym);
void write_bits(ac_coder *ac,long data,int len);
long read_bits(ac_coder *ac,int len);

int ac_decode_symbol_with_ambiguity_check(ac_coder *ac, int freq);

#endif

