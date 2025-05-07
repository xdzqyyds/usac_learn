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
   Recursive Interleaver
   1998 Apr. 11  Toshiro Kawahara
   1998 June 29  by T.K. for all-coded case
   2000 July 10	Sanae Hotani for bitwise interleaver
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eptool.h"

int
recinter(int *coded, int codedlen, int depth, int *noncoded, int noncodedlen, int *buf)
{

  /* note: see also ISO/IEC 14496-3:1999/Amd.1:2000(E) */
  /* variable here (in source code) called "depth" in ISO is called "width" */
  /* variable here (in source code) called "width" in ISO is called "D" */

  int i,j;
  int total;

  int width;
  int m,n;
  int *lenbuf;
  int *tmpbuf;

  if(noncodedlen == 0){
    for(i=0; i<codedlen; i++)
      buf[i] = coded[i];
    return(codedlen);
  }
  if(codedlen == 0){
    for(i=0; i<noncodedlen; i++)
      buf[i] = noncoded[i];
    return(noncodedlen);
  }
/*   printf("interleave\n");fflush(stdout); */
  
  if(depth == 0) depth = 1;
  total = codedlen + noncodedlen;
  width = total / depth;
/*   printf("%d %d %d %d\n", codedlen, noncodedlen, total, depth);   */
  if(NULL == (tmpbuf = (int *)calloc(total, sizeof(int)))){
    perror("tmpbuf in interleaver");
    exit(1);
  }
  if(NULL == (lenbuf = (int *)calloc(depth+1, sizeof(int)))){ /* revised 030121 multrums@iis.fhg.de */
    perror("lenbuf in interleaver");
    exit(1);
  }

  lenbuf[0] = 0;
  for(i=1; i<total - width*depth + 1; i++)
    lenbuf[i] = lenbuf[i-1] + width + 1;
  if (width){ /* revised 010321 multrums@iis.fhg.de */
    for(; i<depth; i++)
      lenbuf[i] = lenbuf[i-1] + width;
  }
  
  for(i=0; i<total; i++)
    tmpbuf[i] = -1;

/*    printf("coded part\n");fflush(stdout);  */
  for(i=0; i<codedlen; i++){
    m = i / depth;
    n = i % depth;
    tmpbuf[m + lenbuf[n]] = coded[i];
  }

  j = 0;
/*    printf("noncoded part\n");fflush(stdout);  */
  for(i=0; i<noncodedlen; i++){
    while(tmpbuf[j] != -1) j++;
    tmpbuf[j++] = noncoded[i];
  }
  for(i=0; i<total; i++) buf[i] = tmpbuf[i];
  free(tmpbuf); free(lenbuf);
/*   printf("j=%d, total = %d\n",j, total);  */
  return total;
}

int recinterBytewise(int *coded,        /* X */
                     int codedlen,      /* len of X in bit */
                     int rs_byte,       /* number of correctable bytes */
                     int *noncoded,     /* Y */
                     int noncodedlen,   /* len of Y in bit */
                     int *buf)
{
  int i,j;
  int total;

  int width;
  int m,n;
  int *lenbuf;
  int *tmpbuf;
  int depth;
  int classlen;
  int rest;
  int stuffingBits;

  if(noncodedlen == 0){
    for(i=0; i<codedlen; i++)
      buf[i] = coded[i];
    return(codedlen);
  }
  if(codedlen == 0){
    for(i=0; i<noncodedlen; i++)
      buf[i] = noncoded[i];
    return(noncodedlen);
  }

  /*   printf("interleave\n");fflush(stdout);  */

  depth = codedlen / (255*8);
  if(codedlen%(255*8) !=0){
    depth++;
  }
  
  classlen = codedlen - 2*rs_byte*depth*8;
  depth = depth*2*rs_byte;
  depth += classlen/8;
  if(classlen%8 != 0) depth++;

  total = codedlen + noncodedlen;
  
  
  stuffingBits = 8 - classlen%8;
  if(stuffingBits == 8){
    stuffingBits = 0;
  }
  width = (total + stuffingBits) / depth;
  rest = (total + stuffingBits)%depth;
  
  /*   printf("%d %d %d %d %d %d\n", codedlen, noncodedlen, total, depth, classlen, width); */
  if(NULL == (tmpbuf = (int *)calloc(total, sizeof(int)))){
    perror("tmpbuf in interleaver");
    exit(1);
  }
  if(NULL == (lenbuf = (int *)calloc(depth+1, sizeof(int)))){ /*revised 010426 multrums@iis.fhg.de */
    perror("lenbuf in interleaver");
    exit(1);
  }

  lenbuf[0] = 0;
  for(i=1; i< rest + 1; i++)
    lenbuf[i] = lenbuf[i-1] + width + 1;
  if(width){
    for(; i<depth; i++)
      lenbuf[i] = lenbuf[i-1] + width;
  }

  for(i=0; i<total; i++)
    tmpbuf[i] = -1;

  /*    printf("coded part\n");fflush(stdout);   */
  for(i=0; i<classlen; i++){
    m = i / 8;
    n = i % 8;

 /*   tmpbuf[m + lenbuf[n]] = coded[i]; */
	tmpbuf[n + lenbuf[m]] = coded[i];
  }

  /*  printf("i=%i\n",i); */
  for(;i<codedlen;i++){
	 m = (i + stuffingBits) / 8;
	 n = (i + stuffingBits) % 8;
	 tmpbuf[n + lenbuf[m] - stuffingBits] = coded[i];
  }

  j = 0;
  /*    printf("noncoded part\n");fflush(stdout);   */
  for(i=0; i<noncodedlen; i++){
    while(tmpbuf[j] != -1) j++;
    tmpbuf[j++] = noncoded[i];
  }
  for(i=0; i<total; i++) buf[i] = tmpbuf[i];
  free(tmpbuf); free(lenbuf);
  /*   printf("j=%d, total = %d\n",j, total);   */

  return total;
}

int
recdeinter(int *coded, int codedlen, int depth, int *noncoded, int noncodedlen, int *buf)
{
  int i,j;
  int total;
  int width;
  int *tmpbuf;
  int m,n;
  int *lenbuf;
  

  if(depth == 0) depth = 1;
  if(codedlen == 0){
    for(i=0; i<noncodedlen; i++)
      noncoded[i] = buf[i];
    return noncodedlen;
  }
  if(noncodedlen == 0){
    for(i=0; i<codedlen; i++)
      coded[i] = buf[i];
    return codedlen;
  }

  total = codedlen + noncodedlen;
  width = total / depth;
  
  if(NULL == (lenbuf = (int *)calloc(depth+1, sizeof(int)))){ /* revised 010426 multrums@iis.fhg.de */
    perror("malloc for interleaver length buffer");
    exit(1);
  }
  lenbuf[0] = 0;
  for(i=1; i<total - width*depth + 1; i++)
    lenbuf[i] = lenbuf[i-1] + width + 1;
 if (width){
  for(; i<depth; i++)
    lenbuf[i] = lenbuf[i-1] + width;
}

  if(NULL == (tmpbuf = (int *)calloc(total, sizeof(int)))){
    perror("malloc for interleaver buffer");
    exit(1);
  }
/*  printf("total = %d\n", total);*/
  memcpy(tmpbuf, buf, total * sizeof(int));

/*
  printf("\n(");
  for(i=0; i<total; i++) printf("%d",tmpbuf[i]);
  printf(")\n");
*/

  for(i=0; i<codedlen; i++){
    m = i / depth;
    n = i % depth;
    coded[i] = tmpbuf[m + lenbuf[n]];
    tmpbuf[m + lenbuf[n]] = -1;
  }

  j = 0;
  for(i=0; i<noncodedlen; i++,j++){
    while(tmpbuf[j] == -1) j++;
/*    printf("j=%d\n",j);*/
    noncoded[i] = tmpbuf[j];
  }

/*
  printf("\n(");
  for(i=0; i<noncodedlen; i++) printf("%d",noncoded[i]);
  printf(")\n");
*/
  free(lenbuf); free(tmpbuf);
  return total;
}

int
recdeinterBytewise(int *coded, int codedlen, int rs_byte, int *noncoded, int noncodedlen, int *buf)
{
  int i,j;
  int total;
  int width;
  int *tmpbuf;
  int m,n;
  int *lenbuf;
  int depth;
  int classlen;
  int rest;
  int stuffingBits;

  if(codedlen == 0){
    for(i=0; i<noncodedlen; i++)
      noncoded[i] = buf[i];
    return noncodedlen;
  }
  if(noncodedlen == 0){
    for(i=0; i<codedlen; i++)

      coded[i] = buf[i];
    return codedlen;
  }

  depth = codedlen / (255*8); /* get number of parts used for RS_encoding */
  if(codedlen%(255*8) !=0){
    depth++;
  }

  classlen = codedlen - 2*rs_byte*depth*8; /* get number of source bits */
  depth = depth*2*rs_byte; /* get number of parity bytes */
  depth += classlen/8; /* add number of source bytes -> we should have total number of bytes */
  if(classlen%8 != 0) depth++; 

  total = codedlen + noncodedlen;

  stuffingBits = 8 - classlen%8;
  if(stuffingBits == 8){
    stuffingBits = 0;
  }
  width = (total + stuffingBits) / depth;
  rest = (total + stuffingBits)%depth;
  
  /*   printf("%d %d %d %d %d %d\n", codedlen, noncodedlen, total, depth, classlen, width); */
  if(NULL == (lenbuf = (int *)calloc(depth+1, sizeof(int)))){ /* revised 010426 multrums@iis.fhg.de */
    perror("malloc for interleaver length buffer");
    exit(1);
  }
  lenbuf[0] = 0;
  for(i=1; i< rest + 1; i++)
    lenbuf[i] = lenbuf[i-1] + width + 1;
  if (width){
    for(; i<depth; i++)
      lenbuf[i] = lenbuf[i-1] + width;
  }


  if(NULL == (tmpbuf = (int *)calloc(total, sizeof(int)))){
    perror("malloc for interleaver buffer");
    exit(1);
  }
  /*  printf("total = %d\n", total); */
  memcpy(tmpbuf, buf, total * sizeof(int));


  /*  printf("\n(");
  for(i=0; i<total; i++) printf("%d",tmpbuf[i]);
  printf(")\n"); */

  for(i=0; i<classlen; i++){
    m = i / 8;
    n = i % 8;
    coded[i] = tmpbuf[n + lenbuf[m]];
    tmpbuf[n + lenbuf[m]] = -1;
  }

  for(;i<codedlen;i++){
	  m = ( i + stuffingBits ) / 8;
	  n = (i + stuffingBits) % 8;
	  coded[i] = tmpbuf[n + lenbuf[m] - stuffingBits];
	  tmpbuf[n + lenbuf[m] - stuffingBits] = -1;
  }

  j = 0;
  for(i=0; i<noncodedlen; i++,j++){
    while(tmpbuf[j] == -1) j++;
    /*    printf("j=%d\n",j); */
    noncoded[i] = tmpbuf[j];
  }


  /*  printf("\n(");
  for(i=0; i<noncodedlen; i++) printf("%d",noncoded[i]);
  printf(")\n"); */

  free(lenbuf); free(tmpbuf);
  return total;
}


#ifdef DEBUG_INTERLEAVE


void main(void)
{
  int i,j;
  int cod[300],rcod[300];
  int non[1000],rnon[1000];
  int buf[1300];
  int total;
  int clen,nlen,dep;
  int count;

  count =0;
  while (count < 50){
  
  clen = 300- count;
  nlen = 100;
  dep = 8;
  for(i=0; i<clen; i++) cod[i] = 1;
  for(i=0; i<nlen; i++) non[i] = 2;

    
  total = recinterBytewise(cod, clen, dep, non, nlen, buf);

  for(i=0; i<total; i++) printf("%d", buf[i]);
  printf("\n");

  recdeinterBytewise(rcod,clen,dep, rnon, nlen,buf);

  for(i=0; i<clen; i++)
    if(cod[i] != rcod[i]) printf("%i %d.",i, rcod[i]);
  for(i=0; i<nlen; i++)
    if(non[i] != rnon[i]) printf("%i %dx",i,rnon[i]);
  count++;
  printf("count=%i\n",count);

  }
}

#endif

