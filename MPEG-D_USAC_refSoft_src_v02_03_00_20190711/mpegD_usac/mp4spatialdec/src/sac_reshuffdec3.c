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



#include "sac_reshuffinit.h"
#include "sac_reshuffdec_intern.h"
#include "sac_bitinput.h" 



int s_decode_huff_cw(Huffman *h)
{
    int i, j;
    unsigned long cw;
    
    i = h->len;
    cw = s_getbits(i);
    while (cw != h->cw) {
	h++;
	j = h->len-i;
	i += j;
	cw <<= j;
	cw |= s_getbits(j);
    }
    return(h->index);
}


void s_unpack_idx(int *qp, int idx, Hcb *hcb)
{
    int dim = hcb->dim;
    int mod = hcb->mod;
    int off = hcb->off;
    if(dim == 4){
	qp[0] = (idx/(mod*mod*mod)) - off;
	idx -= (qp[0] + off)*(mod*mod*mod);
	qp[1] = (idx/(mod*mod)) - off;
	idx -= (qp[1] + off)*(mod*mod);
	qp[2] = (idx/(mod)) - off;
	idx -= (qp[2] + off)*(mod);
	qp[3] = (idx) - off;
    }
    else {
	qp[0] = (idx/(mod)) - off;
	idx -= (qp[0] + off)*(mod);
	qp[1] = (idx) - off;
    }
}


int s_getescape(int q)
{
  int i, off, neg;

  if(q < 0){
    if(q != -16)
      return q;
    neg = 1;
  } else{
    if(q != +16)
      return q;
    neg = 0;
  }

  for(i=4;; i++){
    if(s_getbits(1) == 0)
      break;
  }

  if(i > 16){
    
    off = s_getbits(i-16) << 16;
    off |= s_getbits(16);
  } else
    off = s_getbits(i);

  i = off + (1<<i);
  if(neg)
    i = -i;
  return i;
}



#define iquant( q ) ( q >= 0 ) ? \
	(Float)iq_exp_tbl[ q ] : (Float)(-iq_exp_tbl[ - q ])

Float s_esc_iquant(int q)
{
  if (q > 0) {
    if (q < MAX_IQ_TBL) {
      return((Float)s_iq_exp_tbl[q]);
    }
    else {
      return(pow(q, 4./3.));
    }
  }
  else {
    q = -q;
    if (q < MAX_IQ_TBL) {
      return((Float)(-s_iq_exp_tbl[q]));
    }
    else {
      return(-pow(q, 4./3.));
    }
  }
}



void s_get_sign_bits(int *q, int n)
{
  while (n) {
    if (*q) {
      if (s_getbits(1)) {      
        *q = -*q;
      }
    }
    n--;q++;
  }
}
