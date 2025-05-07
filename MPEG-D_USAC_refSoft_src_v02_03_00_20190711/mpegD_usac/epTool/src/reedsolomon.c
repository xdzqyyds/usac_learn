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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "eptool.h"
#include "rs.h"

#define verbose 0


int prim_poly[9] = {1,0,1,1,1,0,0,0,1}; /* primitive polynomial (M=8) */

int pl[Q_ARY];                  /* polynomial representation */
                                /* -1   : pl[0]    */
                                /* 0    : pl[1]    */
                                /* 1    : pl[2]     */
                                /* ...  :    ...    */

unsigned int pw[Q_ARY];         /* power representation */
                                /* pw[0] : 10000000 */
                                /* pw[1] : 01000000 */
                                /*  ...  :    ...   */

int gp[2*ET_max+1];             /* generator polynomial */
int gp_size ;

/*
 * Multiplication in Galois Field
 */

static int mul_GF(
     int *fx,        /* power representation (input) */
     int *gx,        /* power representation (input) */
     int fsize,      /* size of input polynomial */
     int gsize,      /* size of input polynomial */
     int *mx)        /* power representation (output) */
{


  int i,j;
  int msize;
  unsigned int MX[2*ET_max+1];
        
  msize = fsize + gsize - 1;

  for(i=0; i<msize; i++) MX[i] = 0;

  for(i=0; i<fsize; i++){
    if(fx[i] >= 0){
      for(j=0; j<gsize; j++){
        if(gx[j] >= 0)
          MX[i+j]=(MX[i+j]^pw[(fx[i]+gx[j])%NN]);
      }
    }
  }

  for(i=0; i<msize; i++) mx[i] = pl[ MX[i] ];

  return 0;

}   


/* 
 *  Define Galois Field
 */

int
def_GF(){

  int i;
 
  int pp=0;               /* primitive polynomial */
  int mask=0;             /* MM bit mask */
  unsigned int msb;       /* msb of shift register */
  unsigned int shift;     /* shift register */
  unsigned int value;     /* polynomial representation of a^i */
 
  for(i=0; i<MM; i++) mask += 0x01 << i;
        
  for(i=0; i<MM; i++) pp += prim_poly[i] << i;
         
  pl[0] = -1;
  pl[1] = 0;  pw[0] = 1; 
  shift = 0x01;
  for(i=1; i<(1<<MM)-1; i++){
 
    msb = (shift >> (MM-1)) & 0x01; 
    shift = (msb==1) ? (shift << 1) ^ pp : ((shift << 1) ) ;
 
    value = shift % Q_ARY;

    pl[value] = i;
    pw[i] = value;
 
  }

  return(0);

}



/*
 * Make generator polynomial
 */

int
make_genpoly(
     int     tt)     /* tt symbol error correcting */
{

  int i;

  int g_size;
  int gx[2*ET_max+1];


  gp_size = 2; gp[0] = 1; gp[1] = 0;
  g_size = 2;  gx[1] = 0;

  for(i=2; i<=2*tt; i++){

    gx[0] = i;
    mul_GF(gp, gx, gp_size, g_size, gp);
    gp_size += g_size-1;
   
  }

  return (0);
  
}


/*
 *  transform the input sequence to the power representation
 *  (e.g. 1000 0000 -> a^1
 *        0100 0000 -> a^2 ....)
 *
 */

int RS_input(
     int     *strm,          /* input bit stream */
     int     *seq,           /* power representation */
     int     len)            /* length of input sequence (bit) */
{
  int i,j;
  unsigned int tmp;

  if(len % MM != 0){
    fprintf(stderr,"EP_Tool: RS_input(): reedsolomon.c: RS input length error\n");
    exit(1);
  }

  for(i=0; i<len/MM; i++){

    /* transform the input sequence to polynomial representation */
    tmp=0;
    for(j=0; j<MM; j++)
      tmp |= strm[i*MM+j] << j;

    /* transform to power representation */
    seq[len/MM - 1 - i] = pl[tmp];
  }

  /* return length (symbol) */
  return(len/MM);


}


/*
 *  transform the RS code (polynomial representation) to bit stream
 */
int
RS_output(
     unsigned int    *SEQ,   /* input sequence (polynomial representation) */
     int             *strm,  /* output bit stream */
     int             len,    /* length of input sequence */
     int             tt,     /* tt symbol error correcting */
     int             sw)     /* 0: encoder output, 1: decoder output */
{

  int i,j;
  int length = 0;

  for(i=0; i<len; i++){

    for(j=0; j<MM; j++){
      strm[i*MM+j] = (SEQ[len-1-i] >> j) & 0x01;
      length ++;
    }

  }

  if(sw == 0) return(length);
  if(sw == 1) return(length-2*tt*MM);
  return(0);
}
 


/*
 * Reed Solomon Encoder
 * (refer to Shu Lin, Daniel J. Costello, "Error Control Coding" page 172, Figure 6.13)
 */

int RS_enc (
           int            *seq,    /* input sequence (power representation) */
           int             length, /* length of input sequence (symbol) */
           unsigned int   *CODE,   /* encoded sequence (polynomial representatin) */
           int             tt
           )
{
  int i,j;

  unsigned int SEQ;
  unsigned int SHIFT[2*ET_max];   /* shift register (polynomial)*/
 
  int          add;      /* value added to each register */
  unsigned int ADD;      /* value added to each register */

  for(i=0;i<2*tt;i++) 
    SHIFT[i] = 0;

  if(tt){
    for(i=length-1; i>=0; i--){
      /* input the sequence into the divider */
      SEQ = (seq[i]!=-1) ? pw[seq[i]] : 0;
      ADD = SHIFT[2*tt-1] ^ SEQ ;
      add = pl[ADD];
      
      if(add != -1){
        
        for(j=2*tt-1; j>0; j--){
          if(gp[j] != -1)
            SHIFT[j]=SHIFT[j-1]^pw[(gp[j]+add)%NN];
          else
            SHIFT[j]=SHIFT[j-1] ;
        }
        
      SHIFT[0] = pw[(gp[0]+add)%NN] ;
      
      }else{
        
        for(j=2*tt-1; j>0; j--)
          SHIFT[j] = SHIFT[j-1];
        
        SHIFT[0] = 0;
        
      }

    }
  } /* end if(tt) */
    
  for(i=0,j=0; i<2*tt; i++,j++)
    CODE[j] = SHIFT[i];
  for(i=0; i<length; i++,j++)
    CODE[j] = (seq[i] != -1) ? pw[seq[i]] : 0;
 
  return(length + 2*tt); /* return(j); */
}



/*
 * Syndrome computation
 */

static int synd[2*ET_max+1];            /* Syndrome (power representation) */
static unsigned int SYND[2*ET_max+1];   /* Syndrome (polynomial representation) */

static int
Syndrome(
     int     *seq,   /* input sequence (power representation) */
     int     len,
     int     tt)     /* tt symbol error correcting code */
{

  int i,j;
  int syn_error=0;

  for(i=1; i<=2*tt; i++){

    SYND[i] = 0;
    for(j=0; j<len; j++)
      if(seq[j]!=-1)
        SYND[i] ^= pw[(seq[j]+i*j)%NN];

    synd[i] = pl[SYND[i]];  /* index form*/

    if(SYND[i] != 0) syn_error = 1;
  }

  return(syn_error);

}

/*
 * Compute the Error-Location Polynomial
 * using the Berlekamp iterative algorithm.
 * (refering to Shu Lin, Daniel J. Costello, "Error Control Coding", page 170)
 *
 */

int
RS_dec(
     int             *seq,           /* input sequence (power) */
     unsigned int    *SEQ,           /* output sequence (polynomial) */
     int             len,            /* length of input sequence */
     int             tt)             /* tt symbol error correcting */
{

  int             i,j;
  int             count = 0;
  int             q;
  int             u;              /* u = 'mu'+1 */
  int             d[2*ET_max+2];  /* d[u] = 'mu'th discrepancy */
  unsigned int    D[2*ET_max+2];  /* polynomial representation */

  int             elp[2*ET_max+2][2*ET_max];
  /* error location polynomial */
  unsigned int    ELP[2*ET_max+2][2*ET_max];
  /* polynomial representation */

  int             l[2*ET_max+2];  /* degree of elp */
  int             u_lu[2*ET_max+2]; /* 'mu' - l */
  int             root[ET_max];
  int             loc[ET_max];
  int             z[ET_max+1];
  int             Z[ET_max+1];
  int             err[NN];
  int             ERR[NN];
  int             reg[ET_max+1];  /* register */


  if( Syndrome(seq, len, tt) ){

    /* initiliaze table entries */
    D[0] = 1;        d[0] = pl[D[0]]; 
    D[1] = SYND[1];  d[1] = pl[D[1]]; 
    ELP[0][0] = 1; elp[0][0] = pl[ELP[0][0]];
    ELP[1][0] = 1; elp[1][0] = pl[ELP[1][0]];

    for(i=1; i<2*tt; i++){
      ELP[0][i] = 0;
      ELP[1][i] = 0;
    }
  
    l[0] = l[1] = 0;
    u_lu[0] = -1;
    u_lu[1] = 0;
    u = 0;
   
    /* carry out the iteration of finding error location polynomial */
    do{

      u++;

      if(D[u] == 0){
        l[u+1] = l[u];
       
        for(i=0; i<=l[u]; i++){
          ELP[u+1][i] = ELP[u][i];
          elp[u+1][i] = pl[ELP[u+1][i]];
        }

      }
      else{

        /* search for words with greatest u_lu[q] for which d[q] != 0 */

        q = u-1;
        while((D[q]==0) && (q>0)) q--;
  
        /* have found first non-zero d[q] */
        if(q>0){
          j = q;
  
          do{
            j--;
            if((d[j]!=-1)&&(u_lu[q]<u_lu[j]))
              q = j;
          }while(j>0);
  
        }
           
        /* have now found q such that d[u]!=0 and u_lu[q] is maximum */
        /* store degree of new elp polynomial */

        l[u+1] = (l[u]>l[q]+u-q) ? (l[u]):(l[q]+u-q);
  
        /* form new elp(x) */
        for(i=0; i<2*tt; i++){
          ELP[u+1][i] = 0;
          elp[u+1][i] = pl[ ELP[u+1][i] ];
        }
  
        for(i=0; i<=l[q]; i++){
          if(elp[q][i]!=-1){
            elp[u+1][i+u-q] =
              (d[u]+NN-d[q]+elp[q][i])%NN;
            if(elp[u+1][i+u-q]!=-1)
              ELP[u+1][i+u-q] =
                pw[ elp[u+1][i+u-q] ];
            else
              ELP[u+1][i+u-q] = 0;
          }
        }

        for(i=0; i<=l[u]; i++){
          ELP[u+1][i] ^= ELP[u][i];
          elp[u+1][i] = pl[ ELP[u+1][i] ];
        }
      }
  
      u_lu[u+1] = u - l[u+1];
  
      /* form (u+1)th discrepancy */
      if(u < 2*tt){
        D[u+1] = SYND[u+1];
      
        for(i=1; i<=l[u+1]; i++){

          if((synd[u+1-i]!=-1)&&(elp[u+1][i]!=-1) ) 
            D[u+1] ^=
              pw[(synd[u+1-i]+elp[u+1][i])%NN];

        }
  
        d[u+1] = pl[ D[u+1] ];
      }
       
    } while( (u<2*tt) && (l[u+1]<=tt) );
     
    u++;
    if(l[u] <= tt){ /* can correct error */
  
      /* find roots of the error location polynomial */
      /* using Chien's procedure */
      for(i=1; i<=l[u]; i++) reg[i] = elp[u][i];
      count = 0;
  
      for(i=1; i<=NN; i++){
        q=1;
  
        for(j=1; j<=l[u]; j++){
          if(reg[j]!=-1){
            reg[j] = (reg[j]+j)%NN;
            q ^= pw[reg[j]];
          }
        }

  
        if(!q){ 
          root[count] = i;
          loc[count] = NN -i;
          count ++;
        }
      }
  
      /* No. roots = degree of elp hence <= tt */
      if(count == l[u]){
        if (verbose) {
          fprintf(stderr,"EP_Tool: RS_dec(): reedsolomon.c: Found an error\n"); 
        }
        /* form polynomial z(x) */
        for(i=1; i<=l[u]; i++){
          Z[i] = SYND[i] ^ ELP[u][i];
  
          for(j=1; j<i; j++){

            if((synd[j]!=-1)&&(elp[u][i-j]!=-1))
              Z[i] ^=
                pw[(elp[u][i-j]+synd[j])%NN];

          }
  
          z[i] = pl[ Z[i] ];
        }
  
        /* evaluate errors at locations */
        /* given by error location numbers loc[i] */
  
        for(i=0; i<len; i++){
          ERR[i] = 0;
          SEQ[i] = (seq[i]!=-1)?pw[seq[i]]:0;
        }
  
        /* compute numerator of error term first */
        for(i=0; i<l[u]; i++){
          /* accounts for z[0] */
          ERR[loc[i]] = 1; 
  
          for(j=1; j<=l[u]; j++){
            if(z[j]!=-1)
              ERR[loc[i]] ^=
                pw[(z[j]+j*root[i])%NN];
          }
  
          if(ERR[loc[i]]!=0){
            /* form denominator of error term */
            err[loc[i]] = pl[ ERR[loc[i]] ];
            q=0;
  
            for(j=0; j<l[u]; j++){
              if(j!=i)
                q +=
                  pl[1^pw[(loc[j]+root[i])%NN]];
            }
             
            q = q%NN;
            ERR[loc[i]]=pw[(err[loc[i]]-q+NN)%NN];
            SEQ[loc[i]]^=ERR[loc[i]];
          }
        }
      }
      else{   /* No. roots != degree of elp
                 => No. of err> tt and cannot solve */

        for(i=0; i<len; i++)
          SEQ[i] = (seq[i]!=-1) ? pw[seq[i]]:0;

        return(1);
      }
    }
    else{  /* elp has degree > tt hence cannot solve */
      for(i=0; i<len; i++)
        SEQ[i] = (seq[i]!=-1) ? pw[seq[i]] : 0;
      return(1);
    }
  
  }

  /* syndromes are all 0 => no errors */
  else{

    for(i=0; i<len; i++)
      SEQ[i] = (seq[i]!=-1) ? pw[seq[i]] : 0;
  }

  return(0);
}
   
int RSEncode(int *tbenc, int from, int to, int e_target, int *out)
{
  int nparity, rs_inflenmax;
  int pcnt, icnt;
  int translen, stufflen, enclen;
  int rs_index[256], rs_codedlen;
/*   int rs_code[256]; */
  unsigned int rs_code[256]; /* 010104 multrums@iis.fhg.de */
  int rs_code_bin[256*8];
  int rs_parity[127*(MAXDATA/255)];

  int inflen;
  int *encseq;
  int i,j;

  if (0 == (translen = to - from)) /* 010522 multrums@iis.fhg.de */
    return 0;

  make_genpoly(e_target);
  rs_inflenmax = 255 - 2 * e_target;
  
  /* Stuffing */
  /* Note: Class length is known for RS case !!*/
  stufflen = ((translen + 7) / 8) * 8 - translen;
  enclen = translen + stufflen;
  if(NULL == (encseq = (int *)calloc(enclen, sizeof(int)))){
    perror("EP_Tool: RSEncode(): reedsolomon.c: enclen in RSEncode");
    exit(1);
  }
  memcpy(encseq, &tbenc[from], sizeof(int)*translen);
  /*for(i=to, j=0; j<stufflen; i++, j++) encseq[i] = 0;*/ /*010228 multrums@iis.fhg.de*/

  nparity = (enclen/8 + rs_inflenmax - 1) / rs_inflenmax;

  pcnt = icnt = 0;
  for(i=0; i<nparity; i++){
    if(i == nparity - 1) /* last */
      inflen = (enclen - icnt) / 8;
    else
      inflen = rs_inflenmax;
  
    rs_codedlen = RS_input(&encseq[icnt], rs_index, inflen*8); 
    rs_codedlen = RS_enc(rs_index, rs_codedlen, rs_code, e_target);
    rs_codedlen = RS_output(rs_code, rs_code_bin, rs_codedlen, e_target, 0);

    for(j=0; j<2*e_target*8; j++){
      rs_parity[pcnt++] = rs_code_bin[inflen*8+j];
    }
    icnt += inflen*8;
  }
  memcpy(out, &tbenc[from], sizeof(int)* translen);
  memcpy(&out[translen], rs_parity, sizeof(int)*pcnt);

  free(encseq);

  return translen+pcnt;
}

int
RSEncodeCount(int from, int to, int e_target)
{
  int nparity, rs_inflenmax;
  int translen, stufflen, enclen;

  if(e_target == 0) return to-from;

  rs_inflenmax = 255 - 2 * e_target;
  translen = to - from;
  
  /* Stuffing */
  /* Note: Class length is known for RS case !!*/
  stufflen = ((translen + 7) / 8) * 8 - translen;
  enclen = translen + stufflen;

  nparity = (enclen/8 + rs_inflenmax - 1) / rs_inflenmax;

  return translen + nparity * (2*e_target*8);
}

int
RSDecode(int *decseq, int from, int to, int e_target, int *inseq)
{
  int nparity, rs_inflenmax;
  int pcnt, dcnt, icnt, ocnt;
  int translen, stufflen, enclen, lastinflen;
  int rs_index[256], rs_codelen;
  unsigned int rs_out[256]; /*010104 multrums@iis.fhg.de */
/*   int rs_out[256]; */
  int rs_data[255*8];  
  int rs_detect;

  int inflen;
  int i,j;

  make_genpoly(e_target);
  rs_inflenmax = 255 - 2 * e_target;
  translen = to - from;

  
  /* Stuffing */
  /* Note: Class length is known for RS case !!*/
  stufflen = ((translen + 7) / 8) * 8 - translen;
  enclen = translen + stufflen;

  nparity = (enclen/8 + rs_inflenmax - 1) / rs_inflenmax;
  lastinflen = enclen/8 - (nparity - 1) * rs_inflenmax;
    
  icnt = ocnt = 0;
  pcnt = translen;

  for(i=0; i<nparity; i++){
    dcnt = 0;
    inflen = (i==nparity-1 ? lastinflen : rs_inflenmax);
    for(j=0; j<inflen*8; j++)
      rs_data[dcnt++] = (icnt + j < translen ? inseq[icnt+j] : 0);
    for(j=0; j<2*e_target*8; j++)
      rs_data[dcnt++] = inseq[pcnt++];

    rs_codelen = RS_input(rs_data, rs_index, dcnt);
    rs_detect = RS_dec(rs_index, rs_out, rs_codelen, e_target);
    rs_codelen = RS_output(rs_out, rs_data, rs_codelen, e_target, 1);

    for(j=0; j<(i==nparity-1?rs_codelen-stufflen:rs_codelen); j++)
      decseq[ocnt++] = rs_data[j];

    icnt += j;
  }
  return pcnt;
}

