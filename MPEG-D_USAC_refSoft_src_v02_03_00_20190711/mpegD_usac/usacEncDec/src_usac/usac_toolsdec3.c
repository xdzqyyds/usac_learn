/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, and VoiceAge Corp.

Initial author:

and edited by
Markus Werner       (SEED / Software Development Karlsruhe)

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
$Id: usac_toolsdec3.c,v 1.1.1.1 2009-05-29 14:10:21 mul Exp $
*******************************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "allHandles.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#include "port.h"
#include "huffdec3.h"
#include "buffers.h"
#include "common_m4a.h"
#include "resilience.h"

/* converts integer to binary string */
static char* bin_str(ulong v, int len)
{
    int i, j;
    static char s[32];

    for (i=len-1, j=0; i>=0; i--, j++) {
	s[j] = (v & (1<<i)) ? '1' : '0';
    }
    s[j] = 0;
    return s;
}

static void print_cb(Hcb *hcb) 
{
  int i;
  Huffman *hcw = hcb->hcw;
  fprintf(stderr,"Huffman Codebook\n");
  fprintf(stderr,"size %d, dim %d, lav %d, mod %d, off %d, signed %d\n", 
	hcb->n, hcb->dim, hcb->lav, hcb->mod, hcb->off, hcb->signed_cb);
  fprintf(stderr,"%6s %6s %8s %-32s\n", "index", "length", "cw_10", "cw_2");
  for (i=0; i<hcb->n; i++)
    fprintf(stderr,"%6d %6d %8ld %-32s\n",
          hcw[i].index, hcw[i].len, hcw[i].cw,
          bin_str(hcw[i].cw, hcw[i].len));
}    

int huffcmp(const void *va, const void *vb)
{
    const Huffman *a, *b;

    a = (Huffman *)va;
    b = (Huffman *)vb;
    if (a->len < b->len)
	return -1;
    if ( (a->len == b->len) && (a->cw < b->cw) )
	return -1;
    return 1;
}

/*
 * initialize the Hcb structure and sort the Huffman
 * codewords by length, shortest (most probable) first
 */
void hufftab ( Hcb*          hcb, 
               Huffman*      hcw, 
               int           dim, 
               int           lav, 
               int           lavInclEsc,
               int           signed_cb,
               unsigned char maxCWLen )
{
    int i, n;
    
    if (!signed_cb) {
	hcb->mod = lav + 1;
        hcb->off = 0;
    }
    else {
	hcb->mod = 2*lav + 1;
        hcb->off = lav;
    }
    n=1;	    
    for (i=0; i<dim; i++)
	n *= hcb->mod;

    hcb->n          = n;
    hcb->dim        = dim;
    hcb->lav        = lav;
    hcb->lavInclEsc = lavInclEsc;
    hcb->signed_cb  = signed_cb;
    hcb->hcw        = hcw;
    hcb->maxCWLen   = maxCWLen;
    
    if (debug['H'] && !debug['S']) print_cb(hcb);
    
    qsort(hcw, n, sizeof(Huffman), huffcmp);
    
    if (debug['H'] && debug['S']) print_cb(hcb);
}

/*
 * Cword is working buffer to which shorts from
 *   bitstream are written. Bits are read from msb to lsb
 * Nbits is number of lsb bits not yet consumed
 * 
 * this uses a minimum-memory method of Huffman decoding
 *
 * dataFlag: 0 decode scale factor data (hcod_sf)
 *           1 decode spectral data     (hcod)
 *           2 decode intensity
 *           3 decode noise energy
 *
 */
int decode_huff_cw ( Huffman*          h,
                     unsigned short    dataFlag, 
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                     HANDLE_BUFFER     hSpecData )

{
    int i, j;
    unsigned long cw  = 0;
    unsigned long tmp = 0;
    
    i = h->len;
    if( dataFlag == 0) {
      cw = GetBits ( i, 
                     HCOD_SF, 
                     hResilience, 
                     hEscInstanceData,
                     hSpecData );
    }
    if( dataFlag == 1) {
      if ( GetReorderSpecFlag ( hResilience )) {
        cw = GetBits ( i, 
                       MAX_ELEMENTS, /* does not read from input buffer */
                       hResilience, 
                       hEscInstanceData,
                       hSpecData );
      }
      else 
        {
          cw = GetBits ( i, 
                         HCOD, 
                         hResilience, 
                         hEscInstanceData,
                         hSpecData );
        }
    }
    while (cw != h->cw) {
	h++;
	j = h->len-i;
        if( j < 0 ) {
          CommonExit(1, "negative number of bits (huffdec3.c)\n");
          return h->index;
        }
	i += j;
	cw <<= j;
        if( dataFlag == 0) 
          tmp = GetBits ( j, 
                          HCOD_SF, 
                          hResilience, 
                          hEscInstanceData,
                          hSpecData );
        if( dataFlag == 1) {
          if ( GetReorderSpecFlag ( hResilience )) {
              tmp = GetBits ( j, 
                              MAX_ELEMENTS, /* does not read from input buffer */
                              hResilience, 
                              hEscInstanceData,
                              hSpecData);
          }
          else 
            {
              tmp = GetBits ( j, 
                              HCOD, 
                              hResilience, 
                              hEscInstanceData,
                              hSpecData);
            }
        }
	cw |= tmp;
    }
    return(h->index);
}

void unpack_idx(int *qp, int idx, Hcb *hcb)
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

void get_sign_bits ( int*              q, 
                     int               n,
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                     HANDLE_BUFFER     hSpecData )

{
  int i,sign;
  for (i=0; i<n; i++) {
    if (q[i]) {
      if ( GetReorderSpecFlag ( hResilience )) {
        sign=GetBits ( 1,
                       MAX_ELEMENTS,
                       hResilience, 
                       hEscInstanceData,
                       hSpecData );
      }
      else 
        {
          sign=GetBits ( 1,
                         SIGN_BITS,
                         hResilience, 
                         hEscInstanceData,
                         hSpecData );
        }
      if (sign == 1) {
        /* 1 signals negative, as in 2's complement */
        q[i] = -q[i];
      }
    }
  }
}
