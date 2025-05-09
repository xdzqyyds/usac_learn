/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-29

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1999.

**********************************************************************/
/* ARITHMETIC ENCODING ALGORITHM. */

#include <stdio.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"
 
#include "tns3.h"                /* structs */
 
#include "sam_encode.h"

/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */
 
/* SIZE OF ARITHMETIC CODE VALUES */
#define Code_value_bits     16          /* Number of bits in a code value */
typedef unsigned long code_value;       /* Type of an arithmetic code value */
 
#define Top_value 0x3fffffff
 
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr 0x10000000        /* Point after first quarter */
#define Half      0x20000000        /* Point after frist half    */
#define Third_qtr 0x30000000        /* Point after third quarter */
 
#define NORMAL       0
#define SCALE_UP_1BIT   1

/* START ENCODING A STREAM OF SYMBOLS. */
static char coding_mode;

/* CURRENT STATE OF THE ENCODING */
static code_value low, range;   /* Ends of the current code region.  */
static long bits_to_follow;    /* Number of opposite bits to output   */
              /* after the next bit.      */
static code_value half, qtr1, qtr3;

struct ArEncoderStr {
  char coding_mode; /* 0 : NORMAL mode  1 : 1-Bit SCALE-UP mode */

  /* CURRENT STATE OF THE ENCODING */
  code_value low, range;   /* Ends of the current code region.  */
  long bits_to_follow;  /* Number of opposite bits to output   */
              /* after the next bit.      */
  code_value half, qtr1, qtr3;
};

typedef struct ArEncoderStr ArEncoder;

/* ARRAY FOR STORING STATES OF SEVERAL DECODING */
static ArEncoder arEnc[64];

static long bit_mask1[32] = 
{ 
  0x00000001,  0x00000002,  0x00000004,  0x00000008, 
  0x00000010,  0x00000020,  0x00000040,  0x00000080,
  0x00000100,  0x00000200,  0x00000400,  0x00000800, 
  0x00001000,  0x00002000,  0x00004000,  0x00008000, 
  0x00010000,  0x00020000,  0x00040000,  0x00080000,
  0x00100000,  0x00200000,  0x00400000,  0x00800000, 
  0x01000000,  0x02000000,  0x04000000,  0x08000000,
  0x10000000,  0x20000000,  0x40000000,  0x80000000, 
};

static long bit_mask0[32] = 
{ 
  0x00000000,  0x00000001,  0x00000003,  0x00000007, 
  0x0000000f,  0x0000001f,  0x0000003f,  0x0000007f,
  0x000000ff,  0x000001ff,  0x000003ff,  0x000007ff, 
  0x00000fff,  0x00001fff,  0x00003fff,  0x00007fff, 
  0x0000ffff,  0x0001ffff,  0x0003ffff,  0x0007ffff,
  0x000fffff,  0x001fffff,  0x003fffff,  0x007fffff, 
  0x00ffffff,  0x01ffffff,  0x03ffffff,  0x07ffffff,
  0x0fffffff,  0x1fffffff,  0x3fffffff,  0x7fffffff, 
};

/* Initialize the Parameteres of the i-th Arithmetic Encoder */
void sam_initArEncode(int arEnc_i)
{
  arEnc[arEnc_i].low = 0;    /* full code range. */
  arEnc[arEnc_i].bits_to_follow = 0;  /* No bits to follow next. */
  arEnc[arEnc_i].range = Top_value + 1;

  arEnc[arEnc_i].coding_mode = NORMAL;
  arEnc[arEnc_i].half = Half;
  arEnc[arEnc_i].qtr1 = First_qtr;
  arEnc[arEnc_i].qtr3 = Third_qtr;
}

/* Store the Working Parameters for the i-th Arithmetic Encoder */
void sam_storeArEncode(int arEnc_i)
{
  arEnc[arEnc_i].low = low;    
  arEnc[arEnc_i].bits_to_follow = bits_to_follow;  
  arEnc[arEnc_i].range =  range;

  arEnc[arEnc_i].coding_mode = coding_mode;
  arEnc[arEnc_i].half = half;
  arEnc[arEnc_i].qtr1 = qtr1;
  arEnc[arEnc_i].qtr3 = qtr3;
}

/* Set the i-th Arithmetic Encoder Working */
void sam_setArEncode(int arEnc_i)
{
  low = arEnc[arEnc_i].low;
  bits_to_follow = arEnc[arEnc_i].bits_to_follow;
  range = arEnc[arEnc_i].range;

  coding_mode = arEnc[arEnc_i].coding_mode;
  half = arEnc[arEnc_i].half;
  qtr1 = arEnc[arEnc_i].qtr1;
  qtr3 = arEnc[arEnc_i].qtr3;
}

/* ENCODE A SYMBOL. */
int sam_encode_symbol ( int symbol, int cum_freq[] )
{
  int      est_len;
  code_value   high;
	int				bits;					/* hyungk !!! (2003.11.18) */


  /* Narrow the code region to that allotted to this symbol. */ 
  range = (range>>14);
  low = low + (range * cum_freq[symbol]);
  if (symbol>0)
    range = (range * (cum_freq[symbol-1]-cum_freq[symbol]));
  else
    range = (range * (16384-cum_freq[symbol]));


  est_len = 0;
  while(1) { 
    if ( low > half ) {
			if (bits_to_follow > 31) {		/* hyungk !!! (2003.11.18) */
				sam_putbits2bs(bit_mask1[31], 32);
				bits_to_follow -= 31;
				while (bits_to_follow > 0) {
					bits = bits_to_follow > 32 ? 32 : bits_to_follow;
					sam_putbits2bs(0, bits);
					bits_to_follow -= bits;
				}
			}
			else {
				sam_putbits2bs(bit_mask1[bits_to_follow], bits_to_follow+1);
			}
			bits_to_follow = 0;
      low -= half;
    } 
    else {
      high = low + range; 
      if (high <= half) {
				if (bits_to_follow > 31) {	/* hyungk !!! (2003.11.18) */
					sam_putbits2bs(bit_mask0[31], 32);
					bits_to_follow -= 31;
					while (bits_to_follow > 0) {
						bits = bits_to_follow > 16 ? 16 : bits_to_follow;
						sam_putbits2bs(0xffff, bits);
						bits_to_follow -= bits;
					}
				}
				else {
					sam_putbits2bs(bit_mask0[bits_to_follow], bits_to_follow+1);
				}
        bits_to_follow = 0;
      }
      else if (low > qtr1 &&
         high <= qtr3 ) {
        low -= qtr1;
        bits_to_follow++;
      }
      else {
        break;
      }
    }

    est_len++;
    low = 2 * low;    /* Scale up code range. */
    range = 2 * range;
  }

  if (range < half ) {
    if (coding_mode == NORMAL){ 
      low = 2 * low;    /* Scale up code range. */
      range = 2 * range;
      coding_mode = SCALE_UP_1BIT;

      half = half * 2;
      qtr1 = qtr1 * 2;
      qtr3 = qtr3 * 2;
      est_len++;
    }
  }
  else {
    if (coding_mode == SCALE_UP_1BIT) {
      low = low / 2;    /* Scale down code range. */
      range = range / 2;

      half = half / 2;
      qtr1 = qtr1 / 2;
      qtr3 = qtr3 / 2;
      coding_mode = NORMAL;
      est_len--;
    }
  }

  return (est_len);
}

int sam_encode_symbol2 ( int symbol, int freq0 )
{
  int      est_len;
  code_value   high;
	int				bits;					/* hyungk !!! (2003.11.18) */


  /* Narrow the code region to that allotted to this symbol. */ 
  range = (range>>14);
  if (symbol==0) {
    range = range * freq0;
  }
  else {
    low = low + (range * freq0);
    range = range * (16384 -  freq0);
  }

  est_len = 0;
  while(1) { 
    if ( low > half ) {
			if (bits_to_follow > 31) {		/* hyungk !!! (2003.11.18) */
				sam_putbits2bs(bit_mask1[31], 32);
				bits_to_follow -= 31;
				while (bits_to_follow > 0) {
					bits = bits_to_follow > 32 ? 32 : bits_to_follow;
					sam_putbits2bs(0, bits);
					bits_to_follow -= bits;
				}
			}
			else {
				sam_putbits2bs(bit_mask1[bits_to_follow], bits_to_follow+1);
			}
      bits_to_follow = 0;
      low -= half;
    } 
    else {
      high = low + range; 
      if (high <= half) {
				if (bits_to_follow > 31) {	/* hyungk !!! (2003.11.18) */
					sam_putbits2bs(bit_mask0[31], 32);
					bits_to_follow -= 31;
					while (bits_to_follow > 0) {
						bits = bits_to_follow > 16 ? 16 : bits_to_follow;
						sam_putbits2bs(0xffff, bits);
						bits_to_follow -= bits;
					}
				}
				else {
					sam_putbits2bs(bit_mask0[bits_to_follow], bits_to_follow+1);
				}
        bits_to_follow = 0;
      }
      else if (low > qtr1 &&
         high <= qtr3 ) {
        low -= qtr1;
        bits_to_follow++;
      }
      else {
        break;
      }
    }

    est_len++;
    low = 2 * low;    /* Scale up code range. */
    range = 2 * range;
  }

  if (range < half ) {
    if (coding_mode == NORMAL){ 
      low = 2 * low;    /* Scale up code range. */
      range = 2 * range;
      coding_mode = SCALE_UP_1BIT;

      half = half * 2;
      qtr1 = qtr1 * 2;
      qtr3 = qtr3 * 2;
      est_len++;
    }
  }
  else {
    if (coding_mode == SCALE_UP_1BIT) {
      low = low / 2;    /* Scale down code range. */
      range = range / 2;

      half = half / 2;
      qtr1 = qtr1 / 2;
      qtr3 = qtr3 / 2;
      coding_mode = NORMAL;
      est_len--;
    }
  }

  return (est_len);
}

/* FINISH ENCODING THE STREAM. */
int sam_done_encoding (void)
{
	int				bits;					/* hyungk !!! (2003.11.18) */


  if (coding_mode == SCALE_UP_1BIT)
    bits_to_follow++;

	if (bits_to_follow > 31) {		/* hyungk !!! (2003.11.18) */
		sam_putbits2bs(bit_mask1[31], 32);
		bits_to_follow -= 31;
		while (bits_to_follow > 0) {
			bits = bits_to_follow > 32 ? 32 : bits_to_follow;
			sam_putbits2bs(0, bits);
			bits_to_follow -= bits;
		}
	}
	else {
		sam_putbits2bs(bit_mask1[bits_to_follow], bits_to_follow+1);
	}

  return (bits_to_follow);
}
