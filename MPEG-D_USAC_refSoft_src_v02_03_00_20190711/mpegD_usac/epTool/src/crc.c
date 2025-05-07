/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT Mobile
Communications Network, Inc. in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and
3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC gives users of the MPEG-2 AAC/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 Audio standards. Those intending to use this
software module in hardware or software products are advised that this
use may infringe existing patents. The original developer of this
software module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-2 AAC/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4 Audio
conforming products. This copyright notice must be included in all
copies or derivative works." Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
/*
  crc.c
  subroutine for CRC encode & decode.
  Generator polynomial is described as "crcdata" in function "crcsub".
  Example:
    G(x) = x^16 + x^12 + x^5 + 1  --> crcdata = 69665 ;
*/
/* reviewed 2002-06-12 Markus Multrus mailto:multrums@iis.fhg.de */

#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"


#define MAXCRC 32


static const unsigned int gcrc[MAXCRC+1] = {
    1, /* 0: 1 */
    3, /* 1: x+1 */
    7, /* 2: x2+x+1 */
   11, /* 3: x3+x+1 */
   29, /* 4: x4+x3+x2+1 = (x+1)(x3+x+1)   */
   53, /* 5: x5+x4+x2+x+1 = (x+1)(x4+x+1)  */
   111, /* 6: x6+x5+x3+x2+x+1 = (x+1)(x5+x2+1)  */
   197, /* 7: x7+x6+x2+1=(x+1)(x6+x+1) */
   263, /*263,*/ /*411,*/ /* 8: x8+x2+x+1 = (x+1)(x7+x6+x5+x4+x3+x2+1)  */
   807, /* 9: x9+x8+x5+x2+x+1 = (x+1)(x8+x4+x3+x2+1) */
  1587, /* 10: x10+x9+x5+x4+x+1 = (x+1)(x9+x4+1) */
  3099, /* 11: x11+x10+x4+x3+x+1 = (x+1)(x10+x3+1) */
  6159, /* 12: x12+x11+x3+x2+x+1 = (x+1)(x11+x2+1) */
 12533, /* 13: x13+x12+x7+x6+x5+x4+x2+1 */
 24621, /* 14: x14+x13+x5+x3+x2+1  */
 52421, /* 15: x15+x14+x11+x10+x7+x6+x2+1 */ 
 69665,  /* 69965, */ 
        /* 16: x16+x12+x5+1 = (x+1)(x15+x14+x13+x12+x4+x3+x2+x+1) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  25165923,  /* 24: x24+x23+x6+x5+x+1 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    79764919 /* should be 4374732215, which would be casted to 79764919, whenever using int with 32bit -> is o.k., 'cause we don't need the first bit; to avoid warnings */
            /* 32: x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 */
} ;
  

static void crcsub(int inseq[], int n, int crc[],int  n_crc) 
{

  int i ;
  unsigned int shift_reg ;
  unsigned int crcdata = 0;

  if(n_crc < 33) {
    crcdata = gcrc[n_crc] ;
  }
  else{
    fprintf(stderr, "EP_Tool: crcsub(): crc.c: Error: N_CRC = %d\n", n_crc) ;
    exit(1) ;
  }

  if(crcdata == 0) {
    fprintf(stderr, "EP_Tool: crcsub(): crc.c: Error: N_CRC = %d\n", n_crc) ;
    exit(1) ;
  }

  shift_reg = 0 ; 

  if(n >= n_crc){
    for(i=0 ; i<n_crc ; i++)
      shift_reg = (shift_reg<<1)|inseq[i] ;
    for(i=n_crc ; i<n ; i++)
      shift_reg = ((shift_reg<<1)|inseq[i])^(((shift_reg>>(n_crc-1))&0x1)*crcdata) ;
    for(i=n ; i<n_crc+n ; i++)
      shift_reg = (shift_reg<<1)^(((shift_reg>>(n_crc-1))&0x1)*crcdata) ;
  }else{
    for(i=0; i<n; i++)
      shift_reg = (shift_reg<<1)|inseq[i];
    for(i=n; i<n_crc; i++)
      shift_reg = (shift_reg<<1);
    for(i=n_crc ; i<n_crc+n ; i++)
      shift_reg = (shift_reg<<1)^(((shift_reg>>(n_crc-1))&0x1)*crcdata) ;
  }
  for(i=0 ; i<n_crc ; i++) {
    crc[i] = shift_reg & 0x1 ; 
    shift_reg >>= 1 ;
  }
  return ; 
}

 
void enccrc(int in_buf[], int out_buf[], int n, int n_crc)
{
  int i ;
  int *inseq ;
  int crc[MAXCRC];

  /* memory allocation */
  if ( NULL == (inseq = (int *)malloc(n * sizeof(int))) ) { 
    fprintf(stderr, "EP_Tool: enccrc(): crc.c: Error: Cannot allocate memory for inseq buffer.\n") ;
    exit(1) ;
  }

  for(i=0 ; i<n ; i++) {
    inseq[i] = in_buf[i] ; /* removed cast to (int) 010307 multrums@iis.fhg.de */
  }
  crcsub(inseq, n, crc, n_crc);

  for(i=0 ; i<n_crc ; i++) {
    out_buf[i] = (int)crc[i]; 
  }
  /* reverse bit */
  for(i=0;i<n_crc;i++) {
    if(out_buf[i]==0) { 
      out_buf[i]=1;
    }
    else {
      out_buf[i]=0;
    }
  }
  free(inseq);

  return;
}

void deccrc(int in_buf[], int crc_buf[], int n, int n_crc, int *chk)
{
  int i ;
  int *outseq ;
  int crc[MAXCRC], d_crc[MAXCRC];
  
  /* memory allocation */

  if((outseq = (int *)malloc(n*sizeof(int))) == (int *)0) {
    fprintf(stderr, "EP_Tool: deccrc(): crc.c: Error: Can not allocate memory for outseq buffer.\n") ;
    exit(1) ;
  }

  for(i=0 ; i<n ; i++)  outseq[i] = (int)in_buf[i] ;

  for(i=0 ; i<n_crc ; i++)  d_crc[i] = (int)crc_buf[i] ; 
  /* reverse bit */
  for (i=0;i<n_crc;i++){
    if(d_crc[i]==0) d_crc[i]=1;  
    else d_crc[i]=0;
  }

  crcsub(outseq, n, crc, n_crc) ;

/*  for(i=0;i<n_crc;i++)fprintf(stderr,"crc[%i]=%i\n",i,crc[i]); */

  *chk = 0 ;
  for(i=0 ; i<n_crc ; i++)  *chk = *chk | (crc[i]^d_crc[i]) ;

  free(outseq);

  return ;
}


