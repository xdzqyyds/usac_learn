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


/******************************************************************** 
/
/ filename : ac.c
/ project : MPEG-4 audio scalable lossless coding
/ author : RSY, Institute for Infocomm Research <rsyu@i2r.a-star.edu.sg>
/ date : Oct., 2003
/ contents/description : ac coding
/   
/ 'bslbf' bug fixed, Jan 2006
/

*/




#include <stdio.h>
#include <stdlib.h>

#include "acod.h"

#define CODE_WL 16

#define TOP_VALUE (((long)1<<CODE_WL)-1)
#define QTR_VALUE (TOP_VALUE/4+1)
#define HALF_VALUE	  (2*QTR_VALUE)
#define TRDQTR_VALUE (3*QTR_VALUE)

#define TOP_POS 0x8000



#define PRE_SHT 14




static void output_bit(ac_coder *ac, int bit)
{
  ac->buffer <<= 1;
  if (bit)
    ac->buffer |= 0x01;
  ac->bits_to_go -= 1;
  ac->total_bits += 1;
  if (ac->bits_to_go==0)  {
    ac->bits_to_go = 8;
	if (ac->stream !=NULL) /*else count only*/
		ac->stream[ac->stream_pt++] = (unsigned char)(ac->buffer&0xff);
  }
  return;
}

static void bit_plus_follow (ac_coder *ac, int bit)
{
  output_bit (ac, bit);
  while (ac->fbits > 0)  {
    output_bit (ac, !bit);
    ac->fbits --;
  }
  return;
}

static int input_bit (ac_coder *ac)
{
  int tmp;

  if (ac->bits_to_go==0)  
  {
    ac->buffer = (unsigned int)ac->stream[ac->stream_pt++];
    ac->bits_to_go = 8;
  }

  tmp = (ac->buffer>>7)&1;
  ac->buffer <<= 1;
  ac->bits_to_go --;
  ac->total_bits ++;
  return tmp;
}


void write_bits(ac_coder *ac,long data,int len)
{
	while (len--)
	{
		int temp = (data>>(len))&0x01;
		output_bit(ac,temp);
	}
}

static int read_bit(ac_coder *ac)
{
	int sym;
	sym = (ac->value&TOP_POS)?1:0;
	ac->value = ac->value*2 + input_bit(ac);
 	ac->value &= 0xffff;
	return sym;
}	

long read_bits(ac_coder *ac,int len)
{
	long temp = 0;

	while (len --)
	{
		int dummy;
		dummy = read_bit(ac);
		temp += dummy<<(len);
	}
	return temp;
}



void ac_encoder_init (ac_coder *ac, unsigned char *stream)
{
  ac->stream = stream;
  ac->bits_to_go = 8;
  ac->stream_pt = 0;
  ac->low = 0;
  ac->high = TOP_VALUE;
  ac->fbits = 0;
  ac->buffer = 0;
  ac->total_bits = 0;
  return;
}

void ac_encoder_done (ac_coder *ac)
{
 {
  ac->fbits += 1;
  if (ac->low < QTR_VALUE)
    bit_plus_follow (ac, 0);
  else 
    bit_plus_follow (ac, 1);
 }
 if (ac->stream != NULL)
	ac->stream[ac->stream_pt++] = (unsigned char)((ac->buffer << ac->bits_to_go)&0xff);
  ac->total_bits += 8 - ac->bits_to_go;
  return;
}

void ac_decoder_init (ac_coder *ac, unsigned char *stream, long bsize)
{
  int i;
  ac->fbits = 0;
  ac->total_bits = 0;
  ac->stream = stream;

  ac->stream_pt = 0;
  ac->bits_to_go = 0;
  ac->init = 0;
  ac->value = 0;
  for (i=1; i<=CODE_WL; i++)  {
      ac->value = 2*ac->value + input_bit(ac);
  }
  ac->low = 0;
  ac->high = TOP_VALUE;
  ac->init = 1;

  ac->bsize = bsize;
  return;
  
}



void ac_decoder_done (ac_coder *ac)
{
  return;
}


long ac_bits (ac_coder *ac)
{
  return ac->total_bits /* + ac->fbits + 2 */ ;
}

void ac_encoder_flush(ac_coder *ac)
{
	ac->fbits += 1;
	if (ac->low < QTR_VALUE)
		 bit_plus_follow (ac, 0);
	else 
		bit_plus_follow (ac, 1);
}

void ac_encode_symbol (ac_coder *ac, int freq, int sym)
{
  long range;

  range = (long)(ac->high-ac->low)+1;

  if (sym)
 	ac->high = ac->low + (range*freq>>PRE_SHT)-1;
  else
	ac->low = ac->low + (range*freq>>PRE_SHT);
  for (;;)  {
    if (ac->high<HALF_VALUE)  {
      bit_plus_follow (ac, 0);
    }  else if (ac->low>=HALF_VALUE)  {
      bit_plus_follow (ac, 1);
      ac->low -= HALF_VALUE;
      ac->high -= HALF_VALUE;
    }  else if (ac->low>=QTR_VALUE && ac->high<TRDQTR_VALUE)  {
      ac->fbits += 1;
      ac->low -= QTR_VALUE;
      ac->high -= QTR_VALUE;
    }  else
      break;
    ac->low = 2*ac->low;
    ac->high = 2*ac->high+1;
  }

  return;
}

int ac_decode_symbol (ac_coder *ac, int freq)
{
  long range;
  long cum;
  int sym;

  range = (long)(ac->high-ac->low)+1;
  cum = ((long)((ac->value-ac->low)+1)<<PRE_SHT);
  if (cum<(range)*freq+1)
  {
    sym = 1;
    ac->high = ac->low + (range*freq>>PRE_SHT)-1;
  }
  else
  {
    sym = 0;
    ac->low = ac->low + (range*freq>>PRE_SHT);
  }

  for (;;)  {
    if (ac->high<HALF_VALUE)  {
      /* do nothing */
    }  else if (ac->low>=HALF_VALUE)  {
      ac->value -= HALF_VALUE;
      ac->low -= HALF_VALUE;
      ac->high -= HALF_VALUE;
    }  else if (ac->low>=QTR_VALUE && ac->high<TRDQTR_VALUE)  {
      ac->value -= QTR_VALUE;
      ac->low -= QTR_VALUE;
      ac->high -= QTR_VALUE;
    }  else
      break;
    ac->low = 2*ac->low;
    ac->high = 2*ac->high+1;
    ac->value = 2*ac->value + input_bit(ac);
  }

  return sym;
}


int ac_decode_symbol_with_ambiguity_check(ac_coder *ac, int freq)
{
  long range;
  long cum;
  int sym,sym2;
  long temp;
  int remain;

  remain = ac_bits(ac)-ac->bsize;

  temp = ((ac->value>>remain)<<remain);
  range = (long)(ac->high-ac->low)+1;
  cum = ((long)((temp-ac->low)+1)<<PRE_SHT);
  if (cum<(range)*freq+1)
  {
	sym = 1;
  }
  else
  {
	sym = 0;
  }

  temp = ((ac->value>>remain)<<remain)+ (1<<remain)-1; /* padding 111111 */
  range = (long)(ac->high-ac->low)+1;
  cum = ((long)((temp-ac->low)+1)<<PRE_SHT);
  if (cum<(range)*freq+1)
  {
	sym2 = 1;
  }
  else
  {
	sym2 = 0;
  }
  if(sym!=sym2)
	  return -1; /* exit condition */

  /* actual decoding */
  range = (long)(ac->high-ac->low)+1;
  cum = ((long)((ac->value-ac->low)+1)<<PRE_SHT);

  if (cum<(range)*freq+1)
  {
	sym = 1;
 	ac->high = ac->low + (range*freq>>PRE_SHT)-1;
  }
  else
  {
	sym = 0;
    ac->low = ac->low + (range*freq>>PRE_SHT);
  }

  for (;;)  {
    if (ac->high<HALF_VALUE)  {
      /* do nothing */
    }  else if (ac->low>=HALF_VALUE)  {
      ac->value -= HALF_VALUE;
      ac->low -= HALF_VALUE;
      ac->high -= HALF_VALUE;
    }  else if (ac->low>=QTR_VALUE && ac->high<TRDQTR_VALUE)  {
      ac->value -= QTR_VALUE;
      ac->low -= QTR_VALUE;
      ac->high -= QTR_VALUE;
    }  else
      break;
    ac->low = 2*ac->low;
    ac->high = 2*ac->high+1;

    ac->value = 2*ac->value;
	ac->total_bits++;
  }

  return sym;
}
