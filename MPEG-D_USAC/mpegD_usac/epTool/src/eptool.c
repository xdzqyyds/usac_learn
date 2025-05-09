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
   Common Error Protection Tool for MPEG-4 Audio
   1998 Feb. 21  Toshiro Kawahara
   2000 Aug. 10  Sanae Hotani (modifed for termination functionality)
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "eptool.h"

#define MHDATA 255

static int setHeaderInterleavingWidth(int protectedHeaderLen, int unprotectedHeaderLen, int *interleavingWidth){
  /* set interleaving width for header */
  if(protectedHeaderLen == 6){ 
    if(unprotectedHeaderLen < 3){ /* case repetition code */
      *interleavingWidth = 3;
    } else { /* case BCH(7,4) */
      *interleavingWidth = 6;
    }
  } else if(protectedHeaderLen >= 50){ /* case RCPC 8/16 */
    *interleavingWidth = 28;
  } else {
    *interleavingWidth = protectedHeaderLen;
  }
  
  return 0;
}

int FECEncodeHeader(int data[], int len, int coded[], BCHTable *bchtbl, int Extend, int rate, int crc)
{
  int i,j,k;
  int *parity;
  int *srcpcbuf[4];
  int headerCrc;
  int headerRate;
  int cnt;

  if(len == 0) return 0;

  switch(len){
  case 1:                       /* Majority */
    coded[0] = coded[1] = coded[2] = data[0];
    return 3;

  case 2:                       /* Majority */
    coded[0] = coded[1] = coded[2] = data[0];
    coded[3] = coded[4] = coded[5] = data[1];
    return 6;
  
  case 3:  case 4:                      /* BCH_74 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_74, 1);
      bchtbl->initialized = 1;
    }
    if(NULL == (parity = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECEncodeHeader(): eptool.c: Header protect BCH_74");
      exit(1);
    }
    for (i=j=0; i<len; i++,j++) {
      coded[i] = data[j];
    }
    for (;i<bchtbl->inflen; i++) {
      coded[i] = 0;
    }
    bchenc(bchtbl, coded, parity);
    for(k=0;i<bchtbl->codelen; i++,j++,k++) coded[j] = parity[k];
    free(parity); 
    return bchtbl->codelen - bchtbl->inflen + len;

  case 5: case 6: case 7:       /* BCH_157 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_157, 2);
      bchtbl->initialized = 1;
    }
    if(NULL == (parity = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECEncodeHeader(): eptool.c: Header protect BCH_157");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) coded[i] = data[j];
    for(;i<bchtbl->inflen; i++)  coded[i] = 0;
    bchenc(bchtbl, coded, parity);
    for(k=0;i<bchtbl->codelen; i++,j++,k++) coded[j] = parity[k];
    free(parity); 
    return bchtbl->codelen - bchtbl->inflen + len;

    /* GOLAY_2312 */
  case 8: case 9: case 10: case 11: case 12:
    if(bchtbl->initialized == 0){
      maketables(bchtbl, GOLAY_2312, 3);
      bchtbl->initialized = 1;
    }
    if(NULL == (parity = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECEncodeHeader(): eptool.c: Header protect GOLAY_2312");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) coded[i] = data[j];
    for(;i<bchtbl->inflen; i++)  coded[i] = 0;
    bchenc(bchtbl, coded, parity);
    for(k=0;i<bchtbl->codelen; i++,j++,k++) coded[j] = parity[k];

    free(parity); 
    return bchtbl->codelen - bchtbl->inflen + len;

  case 13: case 14: case 15: case 16:
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_3116, 3);
      bchtbl->initialized = 1;
    }
    if(NULL == (parity = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECEncodeHeader(): eptool.c: Header protect BCH_3116");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) coded[i] = data[j];
    for(;i<bchtbl->inflen; i++)  coded[i] = 0;
    bchenc(bchtbl, coded, parity);
    for(k=0;i<bchtbl->codelen; i++,j++,k++) coded[j] = parity[k];
    free(parity); 
    return bchtbl->codelen - bchtbl->inflen + len;
    
  default: /* RCPC 8/16 + 4-bit CRC */

    if(Extend){
      headerRate = rate;
      headerCrc = crc;
    } else {
      headerRate = 8;
      headerCrc = 4;
    }

    if(NULL == (parity = (int *)calloc(len+4+headerCrc, sizeof(int)))){ 
      perror("EP_Tool: FECEncodeHeader(): eptool.c: Header protect RCPC8/16"); 
      exit(1); 
    } 

    for(i=0; i<len; i++) 
      parity[i] = data[i];

    enccrc(parity, &parity[len], len, headerCrc);

    for(i=0; i<4; i++){
      if(NULL == (srcpcbuf[i] = (int *)calloc(len+4+headerCrc, sizeof(int)))){
        perror("EP_Tool: FECEncodeHeader(): eptool.c: calloc for srcpc for header");
        exit(1);
      }
    }

    SRCPCEncode(parity, srcpcbuf, len+headerCrc, 1); /* return value not evaluated (RS) */
    cnt = SRCPCPunc(srcpcbuf, 0, len+headerCrc+4, 0, headerRate, coded);
    
    assert (cnt < MAXHEADER); /* if assert fails: MAXHEADER needs to be enlarged */

    free(parity);
    for(i=0; i<4; i++) free(srcpcbuf[i]);
    return cnt;
  }
}

int propdepth(int codedlen)
{
  if(codedlen == 6)  return 3;
  else               return codedlen;
}

int FECDecodeHeader(int data[], int len, int coded[], BCHTable *bchtbl, int Extend, int rate, int crc)
{
  int i,j,k;
  int *ctmp;
  int *parity;
  int *srcpcbuf[4];
  int headerCrc;
  int headerRate;

  if(len == 0) {
    return 0;
  }

  if(len == 1){                 /* majority */
    j = 0;
    for(i=0; i<3; i++) j+= coded[i];
    if(j <= 1) data[0] = 0;
    else       data[0] = 1;
    return 3;
  }
  
  if(len == 2){                 /* Majority */
    j = 0; k = 0;
    for(i=0; i<3; i++){ j+= coded[i]; k+= coded[i+3]; }
    data[0] = (j <= 1 ?  0 : 1 );
    data[1] = (k <= 1 ?  0 : 1 );
    return 6;
  }  
  
  if((3 <= len) && (len <= 4)){ /* BCH_74 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_74, 1);
      bchtbl->initialized = 1;
    }
    if(NULL == (ctmp = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECDecodeHeader(): eptool.c: Header protect");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) ctmp[i] = coded[j];
    for(; i<bchtbl->inflen; i++) ctmp[i] = 0;
    for(; i<bchtbl->codelen; i++,j++) ctmp[i] = coded[j];
    if(bchdec(bchtbl, ctmp, data)==1) {
      free(ctmp); /* 010213 multrums@iis.fhg.de */
      return -1;
    }
    free(ctmp);
    return j;
  }
  
  if((5 <= len) && (len <= 7)){ /* BCH_157 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_157, 2);
      bchtbl->initialized = 1;
    }
    if(NULL == (ctmp = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECDecodeHeader(): eptool.c: Header protect");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) ctmp[i] = coded[j];
    for(; i<bchtbl->inflen; i++) ctmp[i] = 0;
    for(; i<bchtbl->codelen; i++,j++) ctmp[i] = coded[j];
    if(bchdec(bchtbl, ctmp, data)==1) {
      free(ctmp); /* 010213 multrums@iis.fhg.de */
      return -1;
    }
    free(ctmp);
    return j;
  }
  
  if((8 <= len) && (len <= 12)){ /* GOLAY_2312 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, GOLAY_2312, 3);
      bchtbl->initialized = 1;
    }
    if(NULL == (ctmp = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECDecodeHeader(): eptool.c: Header protect");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) ctmp[i] = coded[j];
    for(; i<bchtbl->inflen; i++) ctmp[i] = 0;
    for(; i<bchtbl->codelen; i++,j++) ctmp[i] = coded[j];
    if(bchdec(bchtbl, ctmp, data)==1) {
      free(ctmp); /* 010213 multrums@iis.fhg.de */
      return -1;
    }
    free(ctmp);
    return j;
  }
  
  if((13 <= len) && (len <= 16)){ /* BCH_3116 */
    if(bchtbl->initialized == 0){
      maketables(bchtbl, BCH_3116, 3);
      bchtbl->initialized = 1;
    }
    if(NULL == (ctmp = (int *)calloc(sizeof(int), bchtbl->codelen))){
      perror("EP_Tool: FECDecodeHeader(): eptool.c: Header protect");
      exit(1);
    }
    for(i=j=0; i<len; i++,j++) ctmp[i] = coded[j];
    for(; i<bchtbl->inflen; i++) ctmp[i] = 0;
    for(; i<bchtbl->codelen; i++,j++) ctmp[i] = coded[j];
    if(bchdec(bchtbl, ctmp, data)==1) {
      free(ctmp); /* 010213 multrums@iis.fhg.de */
      return -1;
    }
    free(ctmp);
    return j;
  }
  
  if(len > 16){                 /* SRCPC8/16 + 4-bit CRC */
    int cnt, check, crcbuf[32];

    if(Extend){
      headerCrc = crc;
      headerRate = rate;
    } else {
      headerCrc = 4;
      headerRate = 8;
    }
    if(NULL == (parity = (int *)calloc(sizeof(int), len+4+headerCrc))){
      perror("EP_Tool: FECDecodeHeader(): eptool.c: Extended Header protect RCPC");
      exit(1);
    }
    for(i=0; i<4; i++){
      if(NULL == (srcpcbuf[i] = (int *)calloc(len+4+headerCrc, sizeof(int)))){
        perror("EP_Tool: FECDecodeHeader(): eptool.c: calloc for srcpc for header");
        exit(1);
      }
    }

    for(i=0; i<len+headerCrc+4; i++)
      for(j=0; j<4; j++) srcpcbuf[j][i] = -1;
    cnt = SRCPCDepunc(srcpcbuf, 0, len+headerCrc+4, 0, headerRate, coded);
    SRCPCDecode(srcpcbuf, parity, len+4+headerCrc, 1);
    for(i=0; i<len; i++) data[i] = parity[i];

    for(i=0; i<headerCrc; i++) crcbuf[i] = parity[i+len];
    deccrc(data, crcbuf, len, headerCrc, &check);

    free(parity);
    for(i=0; i<4; i++) free(srcpcbuf[i]);

    if(check)
      return -1;
    else
      return cnt;
  }
  return ( 0 );
}

int GenerateHeader(const EPConfig *epcfg, EPFrameAttrib *fattr, int pred, int data1[], headerLength *d1len, int data2[], headerLength *d2len, int bitstuffing, int nstuff)
{
  int i, j;
  int hlen;
  int hdata[MHDATA];
  int hptr;
  int dptr;
  int nclass;
  EPFrameAttrib *cfattr;
  static int srcpcsub[] =
    /* 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
  {0,-1,-1, 1, 2,-1, 3,-1, 4,-1,-1,-1, 5,-1,-1,-1,
   6,-1,-1,-1,-1,-1,-1,-1, 7};
  static int crclensub[] = 
    /* 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
  {0,-1,-1,-1,-1,-1, 1,-1, 2,-1, 3,-1, 4,-1, 5,-1,
   6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   7};

  static BCHTable bchtbl_pred, bchtbl_other;
  bchtbl_pred.initialized = bchtbl_other.initialized = 0;

  /* Pred signaling */
  i = epcfg->NPred - 1;
  for(hlen=0; i>0; hlen++) i >>= 1;
  inttobin(hdata, hlen, pred);
  dptr = FECEncodeHeader(hdata, hlen, data1, &bchtbl_pred, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  d1len->headerLen = dptr;
  
  setHeaderInterleavingWidth(d1len->headerLen, hlen, &(d1len->interleavingWidth));

  /* Other Information */
  hptr = 0;
  /*cfattr = &(epcfg->fattr[pred]);*/
  cfattr = fattr;
  nclass = cfattr->ClassCount;
  cfattr->TotalData = cfattr->UntilTheEnd = cfattr->TotalCRC = 0;

  if (cfattr->ClassReorderedOutput) {
    ReorderOutputClasses (cfattr,0) ;
  }

  for(i=0; i<nclass; i++){
    if (cfattr->ClassReorderedOutput)
      j = cfattr->OutputIndex[i];
    else
      j = i;

    if(cfattr->cattresc[j].ClassBitCount){
      if(cfattr->lenbit[j] > 0){ /* Not Until the end */
        /*      fprintf(stderr, "[%d <-> %d\]\n", cfattr->cattr[j].ClassBitCount,  (1<<cfattr->lenbit[j])-1);*/
        if(cfattr->cattr[j].ClassBitCount > (unsigned int)(1<<cfattr->lenbit[j])-1){
          fprintf(stderr, "EP_Tool: GenerateHeader(): eptool.c: Error: Exceed class length limitation from # of bits for this info in class #%d.\n", i);
          exit(1);
        }
        inttobin(&hdata[hptr], cfattr->lenbit[j],
                 cfattr->cattr[j].ClassBitCount);
        hptr += cfattr->lenbit[j];
        cfattr->TotalData += cfattr->cattr[j].ClassBitCount;
      }else{                    /* Until The End */
        if(i != nclass -1 ){ /* i-> j bugs ?? modified by Sanae 000703 *//* obviously not! re-modified2002-05-08 multrums@iis.fhg.de */
          fprintf(stderr, "EP_Tool: GenerateHeader(): eptool.c: Error: Until the end is only allowed for the last class!!\n");
          exit(1);
        }
        cfattr->UntilTheEnd = 1;
      }
    }
    
    if(cfattr->cattresc[j].ClassCodeRate){
      if(srcpcsub[cfattr->cattr[j].ClassCodeRate] == -1){
        fprintf(stderr, "EP_Tool: GenerateHeader(): eptool.c: Error: Unsupported SRCPC Code rate for inband Signalling!\n");
        exit(1);
      }
      inttobin(&hdata[hptr], 3, srcpcsub[cfattr->cattr[j].ClassCodeRate]);
      hptr += 3;
    }
    if(cfattr->cattresc[j].ClassCRCCount){
      inttobin(&hdata[hptr], 3, crclensub[cfattr->cattr[j].ClassCRCCount]);
      if(crclensub[cfattr->cattr[j].ClassCRCCount] == -1){
        fprintf(stderr, "EP_Tool: GenerateHeader(): eptool.c: Error: Unsupported CRC Length for inband Signalling!\n");
        exit(1);
      }
      hptr += 3;
    }
    cfattr->TotalCRC += cfattr->cattr[j].ClassCRCCount;
  } 

  if(bitstuffing) {
    inttobin(&hdata[hptr], 3, nstuff);
    hptr += 3;
    dptr += d2len->headerLen = FECEncodeHeader(hdata, hptr, data2, &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  }
  else {
    /*dptr = FECEncodeHeader(hdata, hptr, data2, &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
      d2len->headerLen = dptr; */ /* it's a bug, isnt it? Wrong length is returned!? :( 2002-03-16 multrums@iis.fhg.de */
    dptr += d2len->headerLen = FECEncodeHeader(hdata, hptr, data2, &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  }
  
  setHeaderInterleavingWidth(d2len->headerLen, hptr, &(d2len->interleavingWidth));

  return dptr;
}  

int RetrieveHeader(EPConfig *epcfg, int *pred, int data[], int *nstuff)
{
  int i;
  int hlen;
  int hdata[MHDATA];
  int hptr;
  int dptr;
  int nclass;
  EPFrameAttrib *cfattr;
  static int srcpcfull[] =  {0,3,4,6,8,12,16,24};
  static int crclenfull[] = {0,6,8,10,12,14,16,32};
  static BCHTable bchtbl_pred, bchtbl_other;
  int errhdr;
  bchtbl_pred.initialized = bchtbl_other.initialized = 0;


  /* Pred Signaling */
  i = epcfg->NPred - 1; 
  for(hlen=0; i>0; hlen++) i >>= 1;
  errhdr = FECDecodeHeader(hdata, hlen, data, &bchtbl_pred, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  if(errhdr == -1) return -1;
  dptr = errhdr;
  *pred = bintoint(hdata, hlen);

  if(*pred >= epcfg->NPred){    /* pred out of range!! */
    fprintf(stderr, "EP_Tool: RetrieveHeader(): eptool.c: x[%d]",*pred);
    return -1;
  }

  /* Other Information */
  cfattr = &(epcfg->fattr[*pred]);
  nclass = cfattr->ClassCount;

  errhdr = FECDecodeHeader(hdata, epcfg->fattr[*pred].TotalIninf, &data[dptr],
                           &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  if(errhdr == -1) return -1;
  dptr += errhdr;

  hptr = 0;
  cfattr->ClassCount = nclass;
  cfattr->TotalData = cfattr->UntilTheEnd = cfattr->TotalCRC = 0;

  for(i=0; i<nclass; i++){
    int epClass;
    if (cfattr->ClassReorderedOutput!=0) epClass=cfattr->ClassOutputOrder[i];
    else epClass=i;

    if(cfattr->cattresc[epClass].ClassBitCount){
      if(cfattr->lenbit[epClass] > 0){ /* Not Until the end */
        cfattr->cattr[epClass].ClassBitCount = bintoint(&hdata[hptr], cfattr->lenbit[epClass]);
        hptr += cfattr->lenbit[epClass];
      }else{                    /* Until The End */
        if(epClass != nclass-1){    /* Not Last Class <- Invalid */
          fprintf(stderr, "EP_Tool: RetrieveHeader(): eptool.c: Error: Until the end is only allowed for the last epClass!!\n");
          exit(1);
        }
        cfattr->UntilTheEnd = 1;
        cfattr->cattr[epClass].ClassBitCount = 0; /* will be overwritten in EPTool_DecodeFrame, libeptool.c */
      }
    }
    cfattr->TotalData += cfattr->cattr[epClass].ClassBitCount;
    if(cfattr->cattresc[epClass].ClassCodeRate){
      cfattr->cattr[epClass].ClassCodeRate = srcpcfull[bintoint(&hdata[hptr], 3)];
      hptr += 3;
    }
    if(cfattr->cattresc[epClass].ClassCRCCount){
      cfattr->cattr[epClass].ClassCRCCount = crclenfull[bintoint(&hdata[hptr],3)];
      hptr += 3;
    }
    cfattr->TotalCRC += cfattr->cattr[epClass].ClassCRCCount;
  }
  if(epcfg->BitStuffing)
    *nstuff = bintoint(&hdata[hptr], 3);
  else
    *nstuff = 0;
  return dptr;
}

int RetrieveHeaderDeinter(EPConfig *epcfg, int *pred, int data[], int datalen, int remdata[], int *nstuff)
{
  int i;
  int h1len, h1lenenc, h2len;
  int h1data[MHDATA];
  int h2data[MHDATA];
  int htmp[MHDATA*4] = {-1}; /* 010307 multrums@iis.fhg.de */
  int *dtmp;
  int hptr;
  int dptr;
  int nclass;
  EPFrameAttrib *cfattr;
  static int srcpcfull[] =  {0,3,4,6,8,12,16,24};
  static int crclenfull[] = {0,6,8,10,12,14,16,32};
  static BCHTable bchtbl_pred, bchtbl_other;
  int errhdr;
  int interleavingWidth = 0;
  bchtbl_pred.initialized = bchtbl_other.initialized = 0;

  /* Pred Signaling */
  i = epcfg->NPred - 1; /* standard has to be adapted 011001 multrums@iis.fhg.de */
  for ( h1len = 0; i > 0; h1len++ ) {
    i >>= 1;
  }

  h1lenenc = FECEncodeHeader(htmp, h1len, htmp, &bchtbl_pred, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);

  /*  fprintf(stderr,"h1len = %d datalen = %d\n", h1lenenc,datalen); */

  if ( datalen - h1lenenc < 0 ) { /* probable datalen == 0 due to sync loss */
    return -1;
  }

  if(NULL == (dtmp = (int *)calloc(sizeof(int), datalen-h1lenenc))){
    perror("EP_Tool: RetrieveHeaderDeinter(): eptool.c: dtmp in retrieveheaderdeinter"); 
    exit(1);
  }

  setHeaderInterleavingWidth(h1lenenc, h1len, &interleavingWidth);

  recdeinter(htmp, h1lenenc, interleavingWidth, dtmp, datalen-h1lenenc, data);

  errhdr = FECDecodeHeader(h1data, h1len, htmp, &bchtbl_pred, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  if(errhdr == -1){ 
    /*printf("1");*/ 
    return -1;
  }
  dptr = errhdr;
  *pred = bintoint(h1data, h1len);

  if(*pred >= epcfg->NPred){    /* pred out of range!! */
    fprintf(stderr, "EP_Tool: RetrieveHeaderDeinter(): eptool.c: x(%d)", *pred);
    return -1;
  }
  /*  printf("[%d]",*pred);*/

  /* Other Information */
  cfattr = &(epcfg->fattr[*pred]);
  nclass = cfattr->ClassCount;

  h2len = FECEncodeHeader(htmp, epcfg->fattr[*pred].TotalIninf, htmp, &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);

  /*  printf("h2len = %d propdepth(h2len) = %d\n", h2len, propdepth(h2len));*/
  /*  printf("datalen-h1len-h2len = %d\n", datalen-h1len-h2len);*/

  setHeaderInterleavingWidth(h2len, epcfg->fattr[*pred].TotalIninf, &interleavingWidth);

  recdeinter(htmp, h2len, interleavingWidth, remdata, datalen-h1lenenc-h2len, dtmp);
  /*
    printf("\n[");
    for(i=0; i<datalen-h1len-h2len; i++) printf("%d", remdata[i]);
    printf("]\n");
  */
  free(dtmp);
  /*  printf("{%d[%d]}", epcfg->fattr[*pred].TotalIninf, *pred);*/
  dptr = FECDecodeHeader(h2data, epcfg->fattr[*pred].TotalIninf, htmp, &bchtbl_other, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  if(dptr == -1){/*printf("2");*/ return -1;}

  hptr = 0;
  cfattr->ClassCount = nclass;
  cfattr->TotalData = cfattr->UntilTheEnd = cfattr->TotalCRC = 0;

  for(i=0; i<nclass; i++){
    int class;
    if (cfattr->ClassReorderedOutput) class=cfattr->ClassOutputOrder[i];
    else class=i;
 
    if(cfattr->cattresc[class].ClassBitCount){
      if(cfattr->lenbit[class] > 0){ /* Not Until the end */
        cfattr->cattr[class].ClassBitCount = bintoint(&h2data[hptr], cfattr->lenbit[class]);
        hptr += cfattr->lenbit[class];
      }else{                    /* Until The End */        
        if(class != nclass - 1){    /* Not Last Class <- Invalid */ /* reviewed 010724 multrums@iis.fhg.de*/
          fprintf(stderr, "EP_Tool: RetrieveHeaderDeinter(): eptool.c: Error: Until the end is only allowed for the last class!!\n");
          exit(1);
        }
        cfattr->UntilTheEnd = 1;
        cfattr->cattr[class].ClassBitCount = 0; /* will be overwritten in EPTool_DecodeFrame(), libeptool.c */
      }
    }
    cfattr->TotalData += cfattr->cattr[class].ClassBitCount;
    if(cfattr->cattresc[class].ClassCodeRate){
      cfattr->cattr[class].ClassCodeRate = srcpcfull[bintoint(&h2data[hptr], 3)];
      hptr += 3;
    }
    if(cfattr->cattresc[class].ClassCRCCount){
      cfattr->cattr[class].ClassCRCCount = crclenfull[bintoint(&h2data[hptr],3)];
      hptr += 3;
    }
    cfattr->TotalCRC += cfattr->cattr[class].ClassCRCCount;
  }
  if(epcfg->BitStuffing)
    *nstuff = bintoint(&h2data[hptr], 3);
  else
    *nstuff = 0;
  /*  printf("<%d,%d>\n",h1lenenc,h2len);*/
  return h1lenenc+h2len;
}
    
/* Return encoded bit length (0: Invalid) */
/* int codedlen[] contains the coded length of each class */

int PayloadEncode(int src[], int srclen, EPFrameAttrib *fattr, int coded[], int codedlen[])
{
  int i,j,k,offset,index;
  int *tbenc, *tmpSrcpcBuffer;
  int *srcpcbuf[4],*srcpcbufbuf[4];
  int tbenclen;
  int bound[CLASSMAX+1],bound_punc[CLASSMAX+1];
  int crcbits[CODEMAX];
  int nclass, ctrt;
  int ctr;
  int total;
  int jj,cnt_srcpc_out,tmp_cnt_srcpc_out;
  unsigned int tmpSrcpcLen;

  /* Test the validity of the source length and the definition */
  if(srclen < fattr->TotalData){
    fprintf(stderr, "EP_Tool: PayloadEncode(): eptool.c: Source length is shorter than that defined (%d<%d).\n",
            srclen, fattr->TotalData);
    return 0;
  }
  /* Set the last class length from srclen */
  nclass = fattr->ClassCount;

  tbenclen = srclen + fattr->TotalCRC+4*CLASSMAX;

  for (i=0; i<nclass; i++){ /* correct, but waste */
    if (!fattr->cattr[i].ClassBitCount)
      tbenclen -= fattr->cattr[i].ClassCRCCount;
  }
  
  if(NULL == (tbenc = (int *)calloc(tbenclen, sizeof(int)))){
    perror("EP_Tool: PayloadEncode(): eptool.c: failed calloc for tbenc");
    exit(1);
  }
  if(NULL == (tmpSrcpcBuffer = (int *)calloc(tbenclen, sizeof(int)))){
    perror("EPTool: PayloadEncode(): eptool.c: failed calloc for tmpSrcpcBuffer");
    exit(1);
  }
  for(i=0; i<4; i++){
    if(NULL == (srcpcbuf[i] = (int *)calloc(tbenclen, sizeof(int)))){
      perror("EP_Tool: PayloadEncode(): eptool.c: failed calloc for srcpcbuf");
      exit(1);
    }
    if(NULL == (srcpcbufbuf[i] = (int *)calloc(tbenclen, sizeof(int)))){
	perror("EP_Tool: PayloadEncode(): eptool.c: failed calloc for srcpcbufbuf");
	exit(1);
    }
  }

  /* Insert the CRC Check bits */
  offset = ctrt = 0;
  for(i=0; i<nclass; i++){
    int crclen;
    bound[i] = ctrt;

    if (fattr->ClassReorderedOutput)
      {
        k = fattr->OutputIndex[i] ;
        offset = 0;
        for (index = 0; index < k;index++)
          offset += fattr->cattr[index].ClassBitCount;
      }
    else
      {
        k = i;
        if (i>0)
          offset += fattr->cattr[i-1].ClassBitCount;
      }
    
    if(fattr->cattr[k].ClassBitCount){
      crclen = fattr->cattr[k].ClassCRCCount;
      for(j=0; (unsigned int)j<fattr->cattr[k].ClassBitCount; j++)
        tbenc[ctrt++] = src[offset + j];
      if ( crclen > 0 ) {
        enccrc(&tbenc[bound[i]],crcbits, ctrt-bound[i], crclen);
      }
      for(j=0; j<crclen; j++) /* insert CRC-bits */
        tbenc[ctrt++] = crcbits[j];
    }
  }
  bound[i] = ctrt;
 
  /*  encode SRCPC */

  ctrt=0;
  cnt_srcpc_out=0;

  tmpSrcpcLen = 0;
  
  for(i = 0;i < nclass; i++){ /* class loop */

    bound_punc[i]=ctrt;
    tmp_cnt_srcpc_out =0; /* initialization */

    if (fattr->ClassReorderedOutput)
      k = fattr->OutputIndex[i] ;
    else
      k = i;

/*     ctrt+=bound[i+1]-bound[i]+((fattr->cattr[k].ClassBitCount && fattr->cattr[k].ClassCodeRate && fattr->cattr[k].TermSW)?4:0);  */
   
    { /* 2002-07-09 multrums@iis.fhg.de */
      if(fattr->cattr[k].FECType == 0){

        ctrt+=bound[i+1]-bound[i]+(((fattr->cattr[k].ClassBitCount+tmpSrcpcLen) && fattr->cattr[k].TermSW)?4:0);
         
        for(j=0; j<bound[i+1]-bound[i]; j++){
          tmpSrcpcBuffer[tmpSrcpcLen + j] = tbenc[bound[i]+j];
        }
        tmpSrcpcLen += j;
      }
      
      
      if(fattr->cattr[k].TermSW  && tmpSrcpcLen){
        tmp_cnt_srcpc_out = SRCPCEncode(tmpSrcpcBuffer, srcpcbufbuf, tmpSrcpcLen, 1);
        tmpSrcpcLen = 0;
      
        for(j=0;j<4;j++){
          for(jj=0;jj<tmp_cnt_srcpc_out;jj++){
            srcpcbuf[j][cnt_srcpc_out+jj]=srcpcbufbuf[j][jj];
          }
        }
      
        cnt_srcpc_out+= tmp_cnt_srcpc_out;
      }
      
    }
  }

  { /* 2002-07-09 multrums@iis.fhg.de */
    tmp_cnt_srcpc_out = SRCPCEncode(tmpSrcpcBuffer, srcpcbufbuf, tmpSrcpcLen, 0);
    for(j=0;j<4;j++){
      for(jj=0;jj<tmp_cnt_srcpc_out;jj++){
        srcpcbuf[j][cnt_srcpc_out+jj]=srcpcbufbuf[j][jj];
      }
    }
    cnt_srcpc_out+= tmp_cnt_srcpc_out;
  }

  bound_punc[i]=ctrt;

  total = ctr = 0;

  for(i=0; i<nclass; i++) {
    if (fattr->ClassReorderedOutput)
      k = fattr->OutputIndex[i] ;
    else
      k = i;

    if(fattr->cattr[k].FECType == 0){ /* SRCPC */
      int tmp, tmpClassRate, tmpCtr = 0;
      int m, n;
      
      tmp = 8 - bound_punc[i]%8;
      if(tmp == 8){
        tmp = 0;
      }
      codedlen[i] = 0;

      if(tmp){
        if(tmp > (bound_punc[i+1]-bound_punc[i])){
          tmp = bound_punc[i+1]-bound_punc[i];
        }
        tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
        for(m=i-1; m>=0; m--){
          if (fattr->ClassReorderedOutput){
            n = fattr->OutputIndex[m] ;
          } else {
            n = m;
          }
          if(fattr->cattr[n].FECType == 0){
            tmpClassRate = fattr->cattr[n].ClassCodeRate;
            if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
              break;
            }
          }
        }
        
        tmpCtr = codedlen[i] += SRCPCPunc(srcpcbuf, bound_punc[i], bound_punc[i]+tmp,
                                          0, tmpClassRate, &coded[ctr]);
      }

      codedlen[i] += SRCPCPunc(srcpcbuf, bound_punc[i]+tmp, bound_punc[i+1],
                               0, fattr->cattr[k].ClassCodeRate, &coded[ctr+tmpCtr]);
       

      total += codedlen[i];

      ctr += codedlen[i];

    } else {            /* SRS */
      
      int rsconcat = 0;     /* Number of succeeding frames to be concatenated */
      /* if (k == nclass-1){
         if(fattr->UntilTheEnd){
         fprintf(stderr, "EP_Tool: PayloadEncode(): eptool.c: Error: RS cannot be used for UntilTheEnd Class\n");
         exit(1);
         }
         }*/ /* 010720 removed by multrums@iis.fhg.de */
      
      /* revised by RS 000918 */
      if (fattr->ClassReorderedOutput) {
        for(j=i; fattr->cattr[fattr->OutputIndex[j]].FECType == 2; j++, rsconcat ++){
          if (fattr->cattr[fattr->OutputIndex[j+1]].FECType == 0 ) {
            fprintf(stderr, "EP_Tool: PayloadEncode(): eptool.c: FECType 2 must not be followed by FECType 0\n");
            exit(1);
          }
        }
      }
      else {
        for(j=i; fattr->cattr[j].FECType == 2; j++, rsconcat ++) {
          if (fattr->cattr[j].FECType == 0 ) {
            fprintf(stderr, "EP_Tool: PayloadEncode(): eptool.c: FECType 2 must not be followed by FECType 0\n");
            exit(1);
          }
        }
      }
      total += codedlen[i] = RSEncode(tbenc, bound[i], bound[i+rsconcat+1],
                                      fattr->cattr[k].ClassCodeRate, &coded[ctr]);
      ctr += codedlen[i];
      i += rsconcat;
    }
  }
  free(tbenc);
  free(tmpSrcpcBuffer);
  for(i=0; i<4; i++){
    free(srcpcbuf[i]); 
    free(srcpcbufbuf[i]);
  }
  return total;
}

/* Return output payload length  (0: Invalid) */
int PayloadDecode(int payload[], int coded[], int clen, EPFrameAttrib *fattr, int *check)
{
  int i,j;
  int *srcpcbuf[4],*srcpcbufbuf[4];
  int *decbuf,*tmp_decbuf;
  int *rsbuf;
  int crcbits[CODEMAX];
  int bound[CLASSMAX+1],bound_punc[CLASSMAX+1];
  int payloadlen;
  int nclass;
  int lastcode;
  int lastpayload = 0;
  int rsconcat = 0;
  int ctrs, ctrt;
  int total_tail = 0;
  int jj;
  int SRCPCStart;

  nclass = fattr->ClassCount;
  payloadlen = fattr->TotalData + fattr->TotalCRC;

  for (i=0; i<nclass-1; i++){             /* no overhead added, if ClassLength == 0 */
    if (!fattr->cattr[i].ClassBitCount)
      payloadlen -= fattr->cattr[i].ClassCRCCount;
  }

  if(!fattr->UntilTheEnd){
    if (nclass>0) {
    if(!fattr->cattr[nclass-1].ClassBitCount)
      payloadlen -= fattr->cattr[nclass-1].ClassCRCCount;
    }
  }

  lastcode = clen;
  bound[0] = 0;
  for(i=0; i<nclass; i++){
    bound[i+1] = bound[i] +
      fattr->cattr[i].ClassBitCount + (fattr->cattr[i].ClassBitCount?fattr->cattr[i].ClassCRCCount:0);
  }
  bound_punc[0]=0;

  SRCPCStart = 0;
  for(i=0;i<nclass;i++){ 
    
    if(fattr->cattr[i].FECType == 0){
      bound_punc[i+1] = bound_punc[i] + fattr->cattr[i].ClassBitCount + (fattr->cattr[i].ClassBitCount?fattr->cattr[i].ClassCRCCount:0);
      if(fattr->cattr[i].TermSW == 1){
        if((bound_punc[i+1]-bound_punc[SRCPCStart])){
          bound_punc[i+1] += 4;
          total_tail+=4;
        }
        SRCPCStart = i+1;
      } 
    } else { /* no overhead */
      bound_punc[i+1] = bound_punc[i];
    }   
  }

  /* to prevent writing of not allocated mem */
  total_tail+=4;

  for(i=0; i<nclass-1; i++){
    if(fattr->cattr[i].FECType == 0){ /* SRCPC */
      {
        int tmp, m, tmpClassRate;

        tmp = 8 - bound_punc[i]%8;
        if(tmp == 8){
          tmp = 0;
        }
        
        if(tmp){
          if(tmp > bound_punc[i+1]-bound_punc[i]){
            tmp = bound_punc[i+1]-bound_punc[i];
          }

          tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
          for(m=i-1; m>=0; m--){
            if(fattr->cattr[m].FECType == 0){
              tmpClassRate = fattr->cattr[m].ClassCodeRate;
              if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
                break;
              }
            }
          }
          lastcode -= SRCPCDepuncCount(bound_punc[i], bound_punc[i]+tmp, 0, tmpClassRate);
        }
        
        lastcode -= SRCPCDepuncCount(bound_punc[i]+tmp, bound_punc[i+1],
                                     0, fattr->cattr[i].ClassCodeRate); /* Calculate length of SRCPC encode class */
      }

    } else {                       /* RS */
      rsconcat = 0;
      for(j=i; fattr->cattr[j].FECType == 2; j++, rsconcat ++) ;
      for(j=0; j<rsconcat; j++){
        bound[i+1+j] = bound[i+1+rsconcat];
      }
      lastcode -= RSEncodeCount(bound[i], bound[i+1], fattr->cattr[i].ClassCodeRate); /* Calculate Length of SRS encoded class */
      i+= rsconcat;
    }
  }

  /* last class not included for RS code ?*/
  if(i == nclass - 1){
    if(fattr->cattr[i].FECType == 0){ /* SRCPC */
      int tmpLastCode = lastcode;

      {
        int tmp, tmpTmp, m, tmpClassRate, tmpLastPayload = 0;

        tmp = 8 - bound_punc[i]%8;
        if(tmp == 8){
          tmp = 0;
        }
        
        if(tmp){
          tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
          for(m=i-1; m>=0; m--){
            if(fattr->cattr[m].FECType == 0){
              tmpClassRate = fattr->cattr[m].ClassCodeRate;
              if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
                break;
              }
            }
          }
          if(tmp > (tmpTmp = SRCPCDepuncUTECount(bound_punc[i], 0, tmpClassRate, lastcode))){
            tmp = tmpTmp;
          }
          lastcode -= SRCPCDepuncCount(bound_punc[i], bound_punc[i]+tmp, 0, tmpClassRate);
        }
        lastpayload = SRCPCDepuncUTECount(bound_punc[i]+tmp,0, fattr->cattr[i].ClassCodeRate, lastcode); /* Calculate length of SRCPC encode class */
        lastpayload += tmp;
        
        bound_punc[i+1] = bound_punc[i] + lastpayload;

        if(fattr->cattr[i].TermSW && tmpLastCode){
          lastpayload -= 4;                         /* tailbits */
        }
      }

    } else if(fattr->cattr[i].FECType == 1){                       /* RS */
      
      bound_punc[i+1] = bound_punc[i];

      if(fattr->UntilTheEnd){
        if(lastcode){
          lastpayload = lastcode - (8*2*(++rsconcat)*fattr->cattr[i].ClassCodeRate); 
        } else lastpayload = 0;
      } else {
        lastpayload = fattr->cattr[i].ClassBitCount +
          (fattr->cattr[i].ClassBitCount?fattr->cattr[i].ClassCRCCount:0);
      }

    } else if(fattr->cattr[i].FECType == 2){
      fprintf(stderr, "FECType 2 not allowed for last class. Program aborted.");
      exit(1);
    }

    
    if(!fattr->UntilTheEnd){
      if(((unsigned int)lastpayload !=
          fattr->cattr[i].ClassBitCount + (fattr->cattr[i].ClassBitCount?fattr->cattr[i].ClassCRCCount:0))){
        fprintf(stderr, "EP_Tool: PayloadDecode(): eptool.c: x(%d!=%d)",lastpayload, fattr->cattr[i].ClassBitCount + (fattr->cattr[i].ClassBitCount?fattr->cattr[i].ClassCRCCount:0)); /* Error calculating the payload */
        return 0;
      } 
      bound[i+1] = payloadlen; 
    } else {                      /* Until the End */
      bound[i+1] = bound[i] + lastpayload;
      payloadlen +=
        fattr->cattr[i].ClassBitCount =
        lastpayload - (lastpayload?fattr->cattr[i].ClassCRCCount:0);
    } 
  }
  fflush(stdout);
  
    if(NULL == (decbuf = (int *)calloc(payloadlen+total_tail, sizeof(int)))){
      perror("EP_Tool: PayloadDecode(): eptool.c: failed malloc for decbuf");
      exit(1);
    }
    if(NULL == (rsbuf = (int *)calloc(payloadlen+total_tail, sizeof(int)))){
      perror("EP_Tool: PayloadDecode(): eptool.c: failed malloc for rsbuf");
      exit(1);
    }
    for(i=0; i<4; i++){
      if(NULL == (srcpcbuf[i] = (int *)calloc(payloadlen+total_tail, sizeof(int)))){
        perror("EP_Tool: PayloadDecode(): eptool.c: failed malloc for srcpcbuf (decode)");
        exit(1);
      }
    }
    for(i=0; i<payloadlen+total_tail; i++)
      srcpcbuf[1][i] = srcpcbuf[2][i] = srcpcbuf[3][i] = -1;
    
    ctrt = 0;

  /* initialization for rsbuf */
  for(i = 0; i<payloadlen + total_tail; i++) 
    rsbuf[i] = 0;
  for(i=0; i<nclass; i++){
    
    if(fattr->cattr[i].FECType == 0){ /* SRCPC */
      {
        int tmp, m, tmpClassRate;
        
        if(bound_punc[i] == bound_punc[i+1])
          continue;

        tmp = 8 - bound_punc[i]%8;
        if(tmp == 8){
          tmp = 0;
        }
        
        if(tmp){
          if(tmp > bound_punc[i+1]-bound_punc[i]){
            tmp = bound_punc[i+1]-bound_punc[i];
          }
          tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
          for(m=i-1; m>=0; m--){
            if(fattr->cattr[m].FECType == 0){
              tmpClassRate = fattr->cattr[m].ClassCodeRate;
              if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
                break;
              }
            }
          }
          ctrt += SRCPCDepunc(srcpcbuf, bound_punc[i], bound_punc[i]+tmp, 0, tmpClassRate, &coded[ctrt]);
        }
        
        ctrt += SRCPCDepunc(srcpcbuf, bound_punc[i]+tmp, bound_punc[i+1],
                                     0, fattr->cattr[i].ClassCodeRate, &coded[ctrt]); 
      }
    }
    else{                      /* RS */
      ctrt += RSDecode(&rsbuf[bound[i]], bound[i], bound[i+1],
                       fattr->cattr[i].ClassCodeRate, &coded[ctrt]);
      /*      for(j=bound_punc[i]; j<bound_punc[i]+bound[i+1]-bound[i]; j++) srcpcbuf[0][j] = rsbuf[j-bound_punc[i]+bound[i]]; */
    }
  }

  {
    int bufferPosition = 0;
    int tmpLength = 0;
    int startClass = -1;
    int m;

    for(j=0; j<4; j++){
      if(NULL == (srcpcbufbuf[j] = (int *)calloc(payloadlen+total_tail, sizeof(int)))){
        perror("EP_Tool: PayloadDecode(): eptool.c: failed calloc for tbenc");
        exit(1);
      }
    }

    if(NULL == (tmp_decbuf = (int *)calloc(payloadlen+total_tail, sizeof(int)))){
      perror("EP_Tool: PayloadDecode(): eptool.c: failed malloc for decbuf");
      exit(1);
    }

    for(i=0;i<nclass;i++){
      if(fattr->cattr[i].FECType==0){
    
        if(startClass == -1){
          startClass = i;
        }

        for(j=0;j<NOUT;j++){
          for(jj=0;jj < bound_punc[i+1]-bound_punc[i];jj++){
            srcpcbufbuf[j][tmpLength + jj]=srcpcbuf[j][jj+bound_punc[i]];
          }
        }

        tmpLength += jj;

        if(fattr->cattr[i].TermSW == 1 && (bound_punc[i+1]-bound_punc[startClass])){
          SRCPCDecode(srcpcbufbuf, tmp_decbuf, tmpLength, 1);

          tmpLength -= 4;
          
          bufferPosition = 0;
          for(m=startClass; tmpLength > 0; tmpLength -= bound_punc[m+1]-bound_punc[m], m++){
            for(jj=0;jj<bound_punc[m+1]-bound_punc[m];jj++){
              decbuf[jj+bound[m]]=tmp_decbuf[bufferPosition + jj];
            }
            bufferPosition += jj;
          }
          startClass = -1;
          tmpLength = 0;
        }
      }
    }
    SRCPCDecode(srcpcbufbuf, tmp_decbuf, tmpLength, 0);
          
    bufferPosition = 0;
    for(m=startClass; tmpLength > 0; tmpLength -= bound_punc[m+1]-bound_punc[m], m++){
      for(jj=0;jj<bound_punc[m+1]-bound_punc[m];jj++){
        decbuf[jj+bound[m]]=tmp_decbuf[bufferPosition + jj];
      }
      bufferPosition += jj;
    }
    

    free(tmp_decbuf);
    for(j=0;j<4;j++)free(srcpcbufbuf[j]);
  }

  for(i=0; i<nclass; i++){       /* RS decoded bits */
    if(fattr->cattr[i].FECType == 1 || fattr->cattr[i].FECType==2)
      for(j=bound[i]; j<bound[i+1]; j++) 
        decbuf[j] = rsbuf[j];
  }
  ctrs = ctrt = 0;
  for(i=0; i<nclass; i++){
    int crclen, from;
    crclen = fattr->cattr[i].ClassCRCCount;
    from = ctrs;
    for(j=0; (unsigned int)j<fattr->cattr[i].ClassBitCount; j++)
      payload[ctrs++] = decbuf[ctrt++];
    if(fattr->cattr[i].ClassBitCount){
      for(j=0; j<crclen; j++)
        crcbits[j] = decbuf[ctrt++];
      if(crclen != 0){
        deccrc(&payload[from], crcbits, ctrs-from, crclen, &check[i]);
      } else {
        check[i] = 0;
      }
    }else
      check[i] = 0;
  }
  free(decbuf); free(rsbuf);
  for(i=0; i<4; i++) free(srcpcbuf[i]);
  return ctrs;
}

#ifdef DEBUG_PHEADER

void main(void)
{
  int i,j;
  int inf[200], rinf[200];
  int code[800];
  int nerr, ntotal;
  int ndata, ncode;
  BCHTable bchtbl;
  double ber;

  ber = 0.000;
  bchtbl.initialized = 0;
  ndata = 17;
  nerr = ntotal = 0;
  for(i=0; i<100; i++){
    for(j=0; j<ndata; j++){
      inf[j] = (drand48() < 0.5 ? 1 : 0);
      ntotal ++;
    }
    ncode = FECEncodeHeader(inf, ndata, code, &bchtbl, 0,0,0);
    /*    printf("%d -> %d\n", ndata, ncode);*/
    for(j=0; j<ncode; j++)
      code[j] ^= (drand48() < ber ? 1 : 0 );
    FECDecodeHeader(rinf, ndata, code, &bchtbl,0,0,0);
    for(j=0; j<ndata; j++)
      if(inf[j] != rinf[j]) nerr ++;
  }
  printf("nerr = %d/%d\n", nerr, ntotal);
}
#endif

