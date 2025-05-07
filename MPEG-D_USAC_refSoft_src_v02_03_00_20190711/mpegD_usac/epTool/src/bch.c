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
  BCH Code dencoder/decoder with table lookup
  1998 Feb. 20  Toshiro Kawahara
*/
#include <stdio.h>
#include <stdlib.h>
#include "bch.h"

#define FXTMPSIZE 4096     /* 256 */
/* fx = ax gx + hx                           */
/*  fsize: size of fx      gsize: size of gx */
static int Divide_GF2(int fx[], int gx[] ,int fsize, int gsize, int hx[])
{
  int i,j;
  static int fxtmp[FXTMPSIZE];
  
  if( fsize > FXTMPSIZE ){
    printf("ERROR in Divide_GF2: fsize too large!!\n");
    return(-1);
  }
  for(i=0; i<fsize; i++)	/* Copy input data(fx) to temporary */
    fxtmp[i] = fx[i];
	  
  for(i=0; i<fsize-gsize+1; i++){
    if( fxtmp[i] ){
      for(j=0; j<gsize; j++)
	fxtmp[i+j] ^= gx[j];
    }
  }
  for(i=0; i<gsize-1; i++)
    hx[i] = fxtmp[fsize-gsize+1+i];

  return(0);
}


static unsigned int bgptbl[] = {
  0107657,			/* BCH_3116 */
  05423325,			/* BCH_3111 */
  023,				/* BCH_1511 */
  02467,			/* BCH_155  */
  05343,			/* GOLAY_2312 */
  013,				/* BCH_74 */
  0721				/* BCH_157 */
};

void 
maketables(BCHTable *BCHtbl, BCHType type, int ThreshBCH)
{
  int i, j;
  int CODE = 0;
  int INF = 0;
  int infmax;
  int gp[GPMAX], ftmp[CODEMAX], hx[GPMAX-1];
  unsigned int bgp;

  switch (type){
  case BCH_3116:
    CODE = 31; INF = 16;
    break;
  case BCH_3111:
    CODE = 31; INF = 11;
    break;
  case BCH_1511:
    CODE = 15; INF =11;
    break;
  case BCH_155:
    CODE = 15; INF = 5;
    break;
  case GOLAY_2312:
    CODE = 23; INF = 12;
    break;
  case BCH_74:
    CODE = 7; INF = 4;
    break;
  case BCH_157:
    CODE = 15; INF = 7;
    break;
  default:
    fprintf(stderr, "Unsupported BCH type %d\n", type);
    exit(1);
  }

  BCHtbl->codelen = CODE;
  BCHtbl->inflen = INF;

  /* Make genmat */

  bgp = bgptbl[type];
  for(i=0, j=CODE-INF ; i<CODE-INF+1; i++,j--){
    gp[j] = bgp & 1;
    bgp >>= 1;
  }

  for(i=0; i<INF; i++){
    for(j=0; j<CODE; j++)
      ftmp[j] = (j == i ? 1 : 0);
    Divide_GF2(ftmp, gp, CODE, CODE-INF+1, hx);
    BCHtbl->gentbl[i] = 0;
    for(j=0; j<CODE-INF; j++)
      BCHtbl->gentbl[i] ^= hx[j] << (CODE-INF-1-j);
    
    for(j=INF; j<CODE; j++) ftmp[j] = hx[j-INF];
  }
  for(i=0; i<CODE-INF; i++)
    BCHtbl->gentbl[i+INF] = (1 << (CODE-INF - 1 - i));

  /* Make inftbl */
  infmax = 1<<INF;
  for(i=0; i<infmax; i++){
    int tmp;
    tmp = 0;
    for(j=0; j<INF; j++){
      if((i << j) & (infmax/2)){
	tmp ^= BCHtbl->gentbl[j];
      }
    }
    BCHtbl->inftbl[i] = tmp;
  }

  /* Make paritytbl */
  {
    int i,j,k,l,m;		/* 5 error correction */
    int ip,jp,kp,lp,mp, totalp;
    int is,js,ks,ls,ms, totals;
    int paritymax;
  
    paritymax = 1<< (CODE-INF);
    for(i=0; i<paritymax; i++)
      BCHtbl->paritytbl[i] = -1;
  
    /* No Error */
    BCHtbl->paritytbl[0] = 0;

    if(ThreshBCH >= 1){
      /* Single Error */
      for(i=0; i<CODE; i++){
	ip = BCHtbl->gentbl[i]; is = (1<<(CODE-1-i));
	totalp = ip;
	totals = is;
	if(BCHtbl->paritytbl[totalp] != -1 && BCHtbl->paritytbl[totalp] != totals){
	  fprintf(stderr,"BUG 1 ????? %d %d<>%d\n", totalp, totals, BCHtbl->paritytbl[totalp]);
	}
	BCHtbl->paritytbl[totalp] = totals;
      }
    }

    if(type == BCH_1511 || type == BCH_74) return;

    if(ThreshBCH >= 2){
      /* 2 Errors */
      for(i=0; i<CODE-1; i++){
	ip = BCHtbl->gentbl[i]; is = (1<<(CODE-1-i));
	for(j=i+1; j<CODE; j++){
	  jp = BCHtbl->gentbl[j]; js = (1<<(CODE-1-j));
	  totalp = ip^jp;
	  totals = is^js;
	  if(BCHtbl->paritytbl[totalp] != -1 && BCHtbl->paritytbl[totalp] != totals){
	    fprintf(stderr,"BUG 2 ????? %d %d<>%d\n", totalp, totals, BCHtbl->paritytbl[totalp]);
	  }
	  BCHtbl->paritytbl[totalp] = totals;
	}
      }
    }
    if(type == BCH_157) return;

    if(ThreshBCH >= 3){
      /* 3 Errors */
      for(i=0; i<CODE-2; i++){
	ip = BCHtbl->gentbl[i]; is = (1<<(CODE-1-i));
	for(j=i+1; j<CODE-1; j++){
	  jp = BCHtbl->gentbl[j]; js = (1<<(CODE-1-j));
	  for(k=j+1; k<CODE; k++){
	    kp = BCHtbl->gentbl[k]; ks = (1<<(CODE-1-k));
	    totalp = ip^jp^kp;
	    totals = is^js^ks;
	    if(BCHtbl->paritytbl[totalp] != -1 && BCHtbl->paritytbl[totalp] != totals){
	      fprintf(stderr,"BUG 3????? %d %d<>%d\n", totalp, totals, BCHtbl->paritytbl[totalp]);
	    }
	    BCHtbl->paritytbl[totalp] = totals;
	  }
	}
      }
    }
    if(type == BCH_3116 || type == BCH_155 || type == GOLAY_2312) return;

    if(ThreshBCH >= 4){
      /* 4 Errors */
      for(i=0; i<CODE-3; i++){
	ip = BCHtbl->gentbl[i]; is = (1<<(CODE-1-i));
	for(j=i+1; j<CODE-2; j++){
	  jp = BCHtbl->gentbl[j]; js = (1<<(CODE-1-j));
	  for(k=j+1; k<CODE-1; k++){
	    kp = BCHtbl->gentbl[k]; ks = (1<<(CODE-1-k));
	    for(l=k+1; l<CODE; l++){
	      lp = BCHtbl->gentbl[l]; ls = (1<<(CODE-1-l));
	      totalp = ip^jp^kp^lp;
	      totals = is^js^ks^ls;
	      if(BCHtbl->paritytbl[totalp] != -1 && BCHtbl->paritytbl[totalp] != totals){
		fprintf(stderr,"BUG????? %d %d<>%d\n", totalp, totals, BCHtbl->paritytbl[totalp]);
	      }
	      BCHtbl->paritytbl[totalp] = totals;
	    }
	  }
	}
      }
    }
    if(ThreshBCH >= 5){
      /* 5 Errors */
      for(i=0; i<CODE-4; i++){
	ip = BCHtbl->gentbl[i]; is = (1<<(CODE-1-i));
	for(j=i+1; j<CODE-3; j++){
	  jp = BCHtbl->gentbl[j]; js = (1<<(CODE-1-j));
	  for(k=j+1; k<CODE-2; k++){
	    kp = BCHtbl->gentbl[k]; ks = (1<<(CODE-1-k));
	    for(l=k+1; l<CODE-1; l++){
	      lp = BCHtbl->gentbl[l]; ls = (1<<(CODE-1-l));
	      for(m=l+1; m<CODE; m++){
		mp = BCHtbl->gentbl[m]; ms = (1<<(CODE-1-m));
		totalp = ip^jp^kp^lp^mp;
		totals = is^js^ks^ls^mp;
		if(BCHtbl->paritytbl[totalp] != -1 && BCHtbl->paritytbl[totalp] != totals){
		  fprintf(stderr,"BUG????? %d %d<>%d\n", totalp, totals, BCHtbl->paritytbl[totalp]);
		}
		BCHtbl->paritytbl[totalp] = totals;
	      }
	    }
	  }
	}
      }
    }
    
  }
  
}


static int
synd(BCHTable *BCHtbl, int inf, int parity)
{
  return (BCHtbl->inftbl[inf] ^ parity);
}


void
bchenc(BCHTable *BCHtbl, int inf[], int parity[])
{
  int i,j;
  int paritylen;
  int iparity;
  int iinf;

  iinf = 0;
  for(i=0; i<BCHtbl->inflen; i++){
    iinf <<= 1;
    iinf |= inf[i];
  }

  iparity = BCHtbl->inftbl[iinf];
  paritylen = BCHtbl->codelen - BCHtbl->inflen;
  for(i=0,j=paritylen-1; i<paritylen; i++,j--){
    parity[j] = iparity & 0x01;
    iparity >>= 1;
  }
}



int
bchdec(BCHTable *BCHtbl, int code[], int dec[])
{
  int i,j;
  int iinf, iparity, s;

  iinf = 0;
  for(i=0; i<BCHtbl->inflen; i++){
    iinf <<= 1;
    iinf |= code[i];
  }
  iparity = 0;
  for(;i<BCHtbl->codelen; i++){
    iparity <<= 1;
    iparity |= code[i];
  }
  s = synd(BCHtbl, iinf, iparity);
  if(BCHtbl->paritytbl[s] == -1) { /* Error Detected */
    fprintf (stderr,"Error in bchdec detected (bch.c)\n");
    return 1;
  }
/*  printf("%d\n", BCHtbl->paritytbl[s]);*/
  
  iinf ^= BCHtbl->paritytbl[s] >> (BCHtbl->codelen - BCHtbl->inflen);

  for(i=0,j=BCHtbl->inflen - 1; i<BCHtbl->inflen; i++,j--){
    dec[j] = iinf & 0x01;
    iinf >>= 1;
  }
  return 0;
}
