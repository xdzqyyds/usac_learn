#include <stdio.h>
#include <stdlib.h>
#include "calcoverhead.h"
#include "eptool.h"

#define NMEM     4
#define MHDATA 255

static int FakeEncodeHeader(int hlen, int extend, int rate, int crc);
static int FakeSRCPCEncodeTerm(int len);
static int FakeSRCPCEncode(int len);
static int FakeRSEncode(int from, int to, int e_target);
static int FakeSRCPCPunc(int from, int to, int ratefrom, int rateto);

int CalculateHeaderOverhead (EPConfig *epcfg, int pred, int *d1len, int *d2len, int bitstuffing)
{
  int i,j;
  int hlen;
  int hptr;
  int dptr;
  int nclass;
  EPFrameAttrib *cfattr;

  /* Pred signaling */
  i = epcfg->NPred - 1; 
  for(hlen=0; i>0; hlen++) i >>= 1;

  dptr = FakeEncodeHeader(hlen,epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  *d1len = dptr;

  /* Other Information */
  hptr = 0;
  cfattr = &(epcfg->fattr[pred]);
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

    if(cfattr->cattresc[j].ClassBitCount) {
      if(cfattr->lenbit[j] > 0) { /* Not Until the end */
        if (cfattr->cattr[j].ClassBitCount > (unsigned int)(1<<cfattr->lenbit[j])-1) {
          fprintf(stderr, "Calc Overhead: Exceed class length limitation from # of bits for this info.\n");
          exit(1);
        }

        hptr += cfattr->lenbit[j];
        cfattr->TotalData += cfattr->cattr[j].ClassBitCount;
      
      } else {                    /* Until The End */
        if(j != nclass -1 ) {  /* i-> j bugs ?? modified by Sanae 000703 */
          fprintf(stderr, "Until the end is only allowed for the last class!!\n");
          exit(1);
        }
        cfattr->UntilTheEnd = 1;
      }
    }
    
    if(cfattr->cattresc[j].ClassCodeRate) {
      hptr += 3;
    }

    if(cfattr->cattresc[j].ClassCRCCount){
      hptr += 3;
    }

    cfattr->TotalCRC += cfattr->cattr[j].ClassCRCCount;
  }
  
  if(bitstuffing) {
    hptr += 3;
    dptr += *d2len = FakeEncodeHeader(hptr, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
  } else {
    dptr = FakeEncodeHeader(hptr, epcfg->HeaderProtectExtend, epcfg->HeaderRate, epcfg->HeaderCRC);
    *d2len = dptr;
  }

  return dptr;
}

int CalculatePayloadOverhead (int srclen, EPFrameAttrib *fattr, int codedlen[])
{
  int i,j,k,offset,index;
  int tbenclen;
  int bound[CLASSMAX+1],bound_punc[CLASSMAX+1];
  int nclass, ctrt;
  int ctr;
  int total;
  int cnt_srcpc_out,tmp_cnt_srcpc_out;
  int SRCPCStart = 0;

  /* Test the validity of the source length and the definition */
  if(srclen < fattr->TotalData){
    fprintf(stderr, "Source length is shorter than that defined (%d<%d).\n",
            srclen, fattr->TotalData);
    return 0;
  }
  /* Set the last class length from srclen */
  nclass = fattr->ClassCount;

  tbenclen = srclen + fattr->TotalCRC+4*CLASSMAX;

  /* Insert the CRC Check bits */
  offset = ctrt = 0;
  for(i=0; i<nclass; i++){
    bound[i] = ctrt;

    if (fattr->ClassReorderedOutput) {
      k = fattr->OutputIndex[i] ;
      offset = 0;
      for (index = 0; index < k;index++)
        offset += fattr->cattr[index].ClassBitCount;
    } else {
      k = i;
      if (i>0)
        offset += fattr->cattr[i-1].ClassBitCount;
    }

    ctrt += (fattr->cattr[k].ClassBitCount?(fattr->cattr[k].ClassBitCount + fattr->cattr[k].ClassCRCCount):0) ;

  }
  
  bound[i] = ctrt;

  /*  encode SRCPC */

  ctrt=0;
  cnt_srcpc_out=0;

  for(i = 0;i < nclass; i++){
    bound_punc[i]=ctrt;
    tmp_cnt_srcpc_out =0; /* initialization */
    
    if (fattr->ClassReorderedOutput)
      k = fattr->OutputIndex[i] ;
    else
      k = i;
    if(fattr->cattr[k].TermSW==1 && (bound[i+1] - bound[i] + bound_punc[i] - bound_punc[SRCPCStart])) {
      ctrt+=bound[i+1]-bound[i]+4;
      tmp_cnt_srcpc_out = FakeSRCPCEncodeTerm(bound[i+1]-bound[i]);

      cnt_srcpc_out+= tmp_cnt_srcpc_out;
      SRCPCStart = i+1;

    } else {
      ctrt+=bound[i+1]-bound[i];
      tmp_cnt_srcpc_out=FakeSRCPCEncode(bound[i+1]-bound[i]);
      
      cnt_srcpc_out+= tmp_cnt_srcpc_out;
    }

  }
  
  bound_punc[i]=ctrt;

  total = ctr = 0;

  for(i=0; i<nclass; i++) {
    if (fattr->ClassReorderedOutput) {
      k = fattr->OutputIndex[i] ;
    } else {
      k = i;
    }

    if(fattr->cattr[k].FECType == 0) { /* SRCPC */
      
      {
        if(fattr->cattr[k].FECType == 0){ /* SRCPC */
          int tmp, tmpClassRate;
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
            
            codedlen[i] += FakeSRCPCPunc(bound_punc[i], bound_punc[i]+tmp, 0, tmpClassRate);
          }
          
          codedlen[i] += FakeSRCPCPunc(bound_punc[i]+tmp, bound_punc[i+1], 0, fattr->cattr[k].ClassCodeRate );
        }
        ctr += codedlen[i];
      }
      
    } else {                      /* SRS */
      int rsconcat;     /* Number of succeeding frames to be concatenated */
      rsconcat = 0;
      /* revised by RS 000918 */
      if (fattr->ClassReorderedOutput) {
        for(j=i; fattr->cattr[fattr->OutputIndex[j]].FECType == 2; j++, rsconcat ++){
          if (fattr->cattr[fattr->OutputIndex[j+1]].FECType == 0 ) {
            fprintf(stderr, "FECType 2 must not be followed by FECType 0\n");
            exit(1);
          }
        }
      } else {
        for(j=i; fattr->cattr[j].FECType == 2; j++, rsconcat ++) {
          if (fattr->cattr[j].FECType == 0 ) {
            fprintf(stderr, "FECType 2 must not be followed by FECType 0\n");
            exit(1);
          }
        }
      }
      total += codedlen[i] = FakeRSEncode(bound[i], bound[i+rsconcat+1], fattr->cattr[k].ClassCodeRate);
      ctr += codedlen[i];
      i += rsconcat;
    }
  }
  
  return total;
}

static int FakeEncodeHeader(int hlen, int extend, int rate, int crc)
{

  if (hlen == 0)
    return 0;

  if (extend && (hlen > 16)) {
    hlen = (hlen + 4 + crc) * (1 + rate/8) ;
    return hlen;
  }

  switch (hlen) {
  case 1: case 2: /* majority */
    hlen *= 3;
    break;
  case 3:  case 4: /* BCH (7,4) */ 
    hlen += 3;
    break;
  case 5: case 6: case 7: /* BCH (15,7) */
    hlen += 8;
    break;
  case 8: case 9: case 10: case 11: case 12: /* Golay (23,12) */
    hlen += 11;
    break;
  case 13: case 14: case 15: case 16: /* BCH (31,16) */
    hlen += 15;
    break;
  default:
    hlen = (hlen + 4 + 4 /*tailbits */) * 2; /* RCPC 8/16 + 4-bit CRC */
  }

  return hlen;
}

static int FakeSRCPCEncodeTerm(int len)
{
  return len + 4;
}

static int FakeSRCPCEncode(int len)
{
  return len;
}

static int FakeRSEncode(int from, int to, int e_target)
{
  int translen = to - from;
  int pcnt     = 2*e_target*8;

  return translen + pcnt;
}

static int punctbl[][3] = {
  {0x00,0x00,0x00},
  {0x80,0x00,0x00}, {0x88,0x00,0x00}, {0xa8,0x00,0x00},{0xaa,0x00,0x00},
  {0xea,0x00,0x00}, {0xee,0x00,0x00}, {0xfe,0x00,0x00},{0xff,0x00,0x00},
  {0xff,0x80,0x00}, {0xff,0x88,0x00}, {0xff,0xa8,0x00},{0xff,0xaa,0x00},
  {0xff,0xea,0x00}, {0xff,0xee,0x00}, {0xff,0xfe,0x00},{0xff,0xff,0x00},
  {0xff,0xff,0x80}, {0xff,0xff,0x88}, {0xff,0xff,0xa8},{0xff,0xff,0xaa},
  {0xff,0xff,0xea}, {0xff,0xff,0xee}, {0xff,0xff,0xfe},{0xff,0xff,0xff}};


static int FakeSRCPCPunc(int from, int to, int ratefrom, int rateto)
{
  int i, j;
  int pat[3];
  int cnt;

  for(i=0; i<3; i++)
    pat[i] = punctbl[ratefrom][i] ^ punctbl[rateto][i];

  for(i=from, cnt=0; i<to; i++){
    cnt++;
    for(j=0; j<3; j++){
      if(pat[j] & (0x80 >> (i%8))) /* Not Punctured */
        cnt++;
    }
  }

  return cnt;
}
