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
   Systematic RCPC Code, Encoder & Decoder 
   Toshiro Kawahara
   1997 Oct. 6
   1997 Oct. 9	Puncturing & Reordering
   1997 Dec.    Modify for UEP Tool
   1998 Feb.    Modify for EP tool (puncture includes info bit)
                No Termination
*/

#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"

#define NMEM  4
#define NOUT  4

int SRCPCInfromStat(int curstat, int survive_stat);
int SRCPCMetric(int transition, int indata[]);
void reorder(int outseq[], int outbuf[][NOUT], int len, int punctbl[]);
/*
 *   !!!! 'len ' and 'inseq' don't include the tail bits. !!!
 */


int
SRCPCEncode(int inseq[], int *outbuf[NOUT], int len, int TerminationSwitch)
{
  int i, j;
  int *in;
  int fb, nm0;
  int mem[NMEM];
  int memm;


  for(i=0; i<NMEM; i++) mem[i] = 0;
  memm = 0;
  in = inseq;
  for(i=0; i<len; i++){
    int tmp;
    fb = ((memm >> 3) ^ (memm >> 1) ^ memm) & 1;
    nm0 = fb ^  (*in);
    outbuf[0][i] = *in;
    tmp = ((memm >> 3) ^ (memm >> 2) ^ nm0) & 1;
    outbuf[1][i] = tmp;
    outbuf[2][i] = (tmp ^ (memm >> 1)) & 1;
    outbuf[3][i] = (tmp ^ memm) & 1;
    memm = ((memm << 1) | nm0) & 0xf;
    in ++;
  }
  if(TerminationSwitch){
    for(j=0; j<4; j++,i++){
      int tmp;
      fb = ((memm >> 3) ^ (memm >> 1) ^ memm) & 1;
      /*    inseq[i] = fb; (bug!!) */
      nm0 = 0;
      outbuf[0][i] = fb;
      outbuf[1][i] = tmp = ((memm >> 3) ^ (memm >> 2) ) & 1;
      outbuf[2][i] = (tmp ^ (memm >> 1)) & 1;
      outbuf[3][i] = (tmp ^ memm) & 1;
      memm = ((memm << 1) | nm0) & 0xf;
    }
  }
  return i;
}


static int punctbl[][3] = {
  {0x00,0x00,0x00},
  {0x80,0x00,0x00}, {0x88,0x00,0x00}, {0xa8,0x00,0x00},{0xaa,0x00,0x00},
  {0xea,0x00,0x00}, {0xee,0x00,0x00}, {0xfe,0x00,0x00},{0xff,0x00,0x00},
  {0xff,0x80,0x00}, {0xff,0x88,0x00}, {0xff,0xa8,0x00},{0xff,0xaa,0x00},
  {0xff,0xea,0x00}, {0xff,0xee,0x00}, {0xff,0xfe,0x00},{0xff,0xff,0x00},
  {0xff,0xff,0x80}, {0xff,0xff,0x88}, {0xff,0xff,0xa8},{0xff,0xff,0xaa},
  {0xff,0xff,0xea}, {0xff,0xff,0xee}, {0xff,0xff,0xfe},{0xff,0xff,0xff}};

int
SRCPCPunc(int *inseq[], int from, int to, int ratefrom, int rateto, int outseq[])
{
  int i, j;
  int pat[3];
  int cnt;

  for(i=0; i<3; i++)
    pat[i] = punctbl[ratefrom][i] ^ punctbl[rateto][i];

  for(i=from, cnt=0; i<to; i++){
   outseq[cnt++] = inseq[0][i];
    for(j=0; j<3; j++){
      if(pat[j] & (0x80 >> (i%8))) /* Not Punctured */
	outseq[cnt++] = inseq[j+1][i];
    }
  }
  return cnt;
}

int 
SRCPCDepunc(int *outseq[], int from, int to, int ratefrom, int rateto, int inseq[])
{
  int i,j;
  int pat[3];
  int cnt;
  
  for(i=0; i<3; i++)
    pat[i] = punctbl[ratefrom][i] ^ punctbl[rateto][i];

  for(i=from, cnt=0; i<to; i++){
    outseq[0][i] = inseq[cnt++];
    for(j=0; j<3; j++){
      if(pat[j] & (0x80 >> (i%8))) /* Not Punctured */
	outseq[j+1][i] = inseq[cnt++];
    }
  }
  return cnt;
}
int 
SRCPCDepuncCount(int from, int to, int ratefrom, int rateto)
{
  int i,j;
  int pat[3];
  int cnt;
  
  for(i=0; i<3; i++)
    pat[i] = punctbl[ratefrom][i] ^ punctbl[rateto][i];

  for(i=from, cnt=0; i<to; i++){
    cnt ++; /* outseq[i][0] = inseq[cnt++];*/
    for(j=0; j<3; j++){
      if(pat[j] & (0x80 >> (i%8))) /* Not Punctured */
	cnt++; /*outseq[i][j+1] = inseq[cnt++];*/
    }
  }
  return cnt;
}

/* Depuncture for "Until the End" */
/* Return: the last index to outseq[] */

int 
SRCPCDepuncUTECount(int from, int ratefrom, int rateto, int inlen)
{
  int i,j;
  int pat[3];
  int cnt;
  /* codeto is for the length of inseq */
  
  for(i=0; i<3; i++)
    pat[i] = punctbl[ratefrom][i] ^ punctbl[rateto][i];

  for(i=from,cnt=0; cnt<inlen; i++){
    cnt++; /* outseq[i][0] = inseq[cnt++];*/
    for(j=0; j<3; j++){
      if(pat[j] & (0x80 >> (i%8))) /* Not Punctured */
	cnt++; /* outseq[i][j+1] = inseq[cnt++];*/
    }
  }
  return i-from;
}

#define NSTAT (1<<NMEM)


int
SRCPCDecode(int *inseq[], int decseq[], int len, int TerminationSwitch)
{
  int i,j;
  int metmp1, metmp2;
  int metric1, metric2;
  int metric[NSTAT];
  int metric_next[NSTAT];
  static int pathmem[MAXCODE][NSTAT];
  int incod[NOUT];
  int puncnum = 0;
  int curstat;
  int survive_stat;
  

  metric[0] = 0;		/* Initialize metric */
  for(i=1; i<NSTAT; i++)
    metric[i] = 100000;

/*   if(i > MAXNPATH){ */ /* 2002-07-11 multrums@iis.fhg.de */
/*     fprintf(stderr, "MAXNPATH in SRCPCDecode Exteeded!!\n"); */
/*     exit(1); */
/*   } */

  if(len > MAXCODE){
    fprintf(stderr, "MAXCODE exceeded.");
    exit(1);
  }

  for(i=0; i<len; i++){
    puncnum = 0;
    for(j=0; j<NOUT; j++){
      incod[j] = inseq[j][i];
      if(incod[j] == -1) puncnum ++;
    }

    for(j=0; j<NSTAT; j+=2){
      metmp1 = SRCPCMetric(j, incod);
      metmp2 = 4 - puncnum - metmp1;
      metric1 = metric[j/2] + metmp1;
      metric2 = metric[j/2+NSTAT/2] + metmp2;
      if(metric1 < metric2){
	pathmem[i][j] = j/2;
	metric_next[j] = metric1;
      }else{
	pathmem[i][j] = j/2 + NSTAT/2;
	metric_next[j] = metric2;
      }
      metric1 = metric[j/2] + metmp2;
      metric2 = metric[j/2+NSTAT/2] + metmp1;
      if(metric1 < metric2){
	pathmem[i][j+1] = j/2;
	metric_next[j+1] = metric1;
      }else{
	pathmem[i][j+1] = j/2 + NSTAT/2;
	metric_next[j+1] = metric2;
      }
    }
    for(j=0; j<NSTAT; j++){
      metric[j] = metric_next[j];
    }
  }

  if(TerminationSwitch){
    curstat = 0; /* Terminated !!*/
  } else {
    int minmet, minstat;
    minmet = metric[0]; minstat = 0;
    for(j=1; j<NSTAT; j++){
      if(minmet >= metric[j]){
	minstat = j;
	minmet = metric[j];
      }
    }
    curstat = minstat;
  }
  
  for(i=i-1; i>=0; i--){
    survive_stat = pathmem[i][curstat];
    decseq[i] = SRCPCInfromStat(curstat, survive_stat);
    curstat = survive_stat;
  }
  
  return metric[0];
}


int
SRCPCInfromStat(int curstat, int survive_stat)
{
  int mem[NMEM];
  int nm0;
  int i;

  for(i=0; i<NMEM; i++){
    mem[i] = survive_stat & 1;
    survive_stat >>= 1;
  }
  nm0 = curstat & 1;
  return mem[3] ^ mem[1] ^ mem[0] ^ nm0;
}

int
SRCPCMetric(int transition, int indata[])
{
  int i,j;
  int met;
  int inxorfb;
  int mem[NMEM];
  int t;
  static int out[NOUT][NSTAT];
  static int Initialize = 1;
  
  if(Initialize){
    Initialize = 0;
    for(i = 0; i < NSTAT; i ++){
      t = i;
      inxorfb = t & 1;
      t >>= 1;
      for(j=0; j<NOUT; j++){
	mem[j] = t & 1;
	t >>= 1;
      }
      
      out[0][i] = mem[3] ^ mem[1] ^ mem[0] ^ inxorfb;
      out[1][i] = mem[3] ^ mem[2] ^ inxorfb;
      out[2][i] = mem[3] ^ mem[2] ^ mem[1] ^ inxorfb;
      out[3][i] = mem[3] ^ mem[2] ^ mem[0] ^ inxorfb;
    }
  }

  met = 0;
  for(i=0; i<NOUT; i++)
    if(indata[i] != -1)
      met += out[i][transition] ^ indata[i];
  return met;
}

