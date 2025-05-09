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
/* ARITHMETIC DECODING ALGORITHM. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "tf_mainHandle.h"
#include "sam_tns.h"             /* struct */
#include "sam_dec.h"

/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */
 
/* SIZE OF ARITHMETIC CODE VALUES */
#define Code_value_bits     16          /* Number of bits in a code value */
typedef unsigned long code_value;       /* Type of an arithmetic code value */
 
#define Top_value 0x3fffffff
 
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr 0x10000000        /* Point after first quarter */
#define Half      0x20000000        /* Point after frist half    */
#define Third_qtr 0x30000000        /* Point after third quarter */
 
/* CURRENT STATE OF THE DECODING */
static code_value value;  /* Currently-seen code value           */
static code_value range;  /* Size of the current code region */
static int est_cw_len=30;  /* estimated codeword length */

struct ArDecoderStr {
  code_value value;  /* Currently-seen code value           */
  code_value range;  /* Size of the current code region */
  int cw_len;      /* estimated codeword length */
};

typedef struct ArDecoderStr ArDecoder;

/* ARRAY FOR STORING STATES OF SEVERAL DECODING */
static ArDecoder arDec[64];

static code_value half[16] = 
{
  0x20000000, 0x10000000, 0x08000000, 0x04000000, 
  0x02000000, 0x01000000, 0x00800000, 0x00400000,  
  0x00200000, 0x00100000, 0x00080000, 0x00040000, 
  0x00020000, 0x00010000, 0x00008000, 0x00004000 
};


/* Initialize the Parameteres of the i-th Arithmetic Decoder */
void sam_initArDecode(int arDec_i)
{
  arDec[arDec_i].value = 0;
  arDec[arDec_i].range = 1;
  arDec[arDec_i].cw_len = 30;
}

/* Store the Working Parameters for the i-th Arithmetic Decoder */
void sam_storeArDecode(int arDec_i)
{
  arDec[arDec_i].value = value;
  arDec[arDec_i].range = range;
  arDec[arDec_i].cw_len = est_cw_len;
}

/* Set the i-th Arithmetic Decoder Working */
void sam_setArDecode(int arDec_i)
{
  value = arDec[arDec_i].value;
  range = arDec[arDec_i].range;
  est_cw_len = arDec[arDec_i].cw_len;
}

/* DECODE THE NEXT SYMBOL. */
int sam_decode_symbol (int cum_freq[], int *symbol)
{
  int  cum;    /* Cumulative frequency calculated */
  int  sym;      /* Symbol decoded       */

  if (est_cw_len) {
    range = (range<<est_cw_len);
    value = (value<<est_cw_len) | sam_getbitsfrombuf(est_cw_len);
  }

  range >>= 14;
  cum = value/range;        /* Find cum freq */ 

  /* Find symbol */
  for (sym=0; cum_freq[sym]>cum; sym++);

  *symbol = sym;

  /* Narrow the code region to that allotted to this symbol. */ 
  value -= (range * cum_freq[sym]);

  if (sym > 0) {
    range = range * (cum_freq[sym-1]-cum_freq[sym]);
  }
  else {
    range = range * (16384-cum_freq[sym]);
  }

  for(est_cw_len=0; range<half[est_cw_len]; est_cw_len++);

  return est_cw_len;
}

/* Binary Arithmetic decoder */
int sam_decode_symbol2 (int freq0, int *symbol)
{
  if (est_cw_len) {
    range = (range<<est_cw_len);
    value = (value<<est_cw_len) | sam_getbitsfrombuf(est_cw_len);
  }

  range >>= 14;

  /* Find symbol */
  if ( (freq0 * range) <= value ) {
    *symbol = 1;

    /* Narrow the code region to that allotted to this symbol. */ 
    value -= range * freq0;
    range =  range * (16384-freq0);
  }
  else {
    *symbol = 0;

    /* Narrow the code region to that allotted to this symbol. */ 
    range = range * freq0;
  }
    
  for(est_cw_len=0; range<half[est_cw_len]; est_cw_len++);

  return est_cw_len;
}

