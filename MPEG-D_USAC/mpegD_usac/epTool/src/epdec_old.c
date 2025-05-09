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
    MPEG-4 Audio Error Protection Tool Decoder
    1998 Feb. 24  Toshiro Kawahara
*/
/* modification by Markus Multrus, fhg
   2000 Sept. 12
*/


#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"
#include "dcm_io.h"


#define MHDATA 255

int main(int argc, char **argv)
{
  int i,j;
  static int data[MAXDATA];
  int rs_index[256], rs_codelen, rs_detect;
  unsigned int rs_out[256];
  int rs_data[256*8];
  static int coded[MAXCODE];
  static int ibuf[MAXDATA*2];
  static int ncbuf[MAXDATA];
  int trans[MAXDATA*2], translen;
  int check[CLASSMAX];
  char buf[MAXDATA];
  FILE *inpred, *instream, *ininfo, *outstream, *outinfo;
  EPConfig epcfg;
  EPConfig m_Epcfg;
  int pred;
  int payloadlen;
  int codedlen[CLASSMAX];
  int nstuff;
  int nFrames = 0;
  int jcnt;
  int CAtbl[MAXPREDwCA][CLASSMAX], CAbits[MAXPREDwCA];
#ifdef INTEGRATED
  int PredCA[MAXPREDwCA][255];
#else
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];
#endif  
  if(argc != 6){
    fprintf(stderr, "Usage: %s <pred_file> <in_stream> <in_info> <out_stream> <out_info>\n",argv[0]);
    exit(1);
  }

  if(NULL == (inpred = fopen(argv[1], "r"))){
    perror(argv[1]); exit(1);
  }
  if(NULL == (instream = fopen(argv[2], "rb"))){
    perror(argv[2]); exit(1);
  }
  if(NULL == (ininfo = fopen(argv[3], "r"))){
    perror(argv[3]); exit(1);
  }
  if(NULL == (outstream = fopen(argv[4], "wb"))){
    perror(argv[4]); exit(1);
  }
  if(NULL == (outinfo = fopen(argv[5], "w"))){
    perror(argv[5]); exit(1);
  }
  def_GF();                     /* generate GF(2^8) tables*/

#ifdef INTEGRATED
  OutInfoParse(inpred, &epcfg, 0, CAtbl, CAbits, 1);
  PredSetConvert(&epcfg, &m_Epcfg, CAtbl, CAbits, PredCA, 1);
#else /*INTEGRATED*/
  OutInfoParse(inpred, &m_Epcfg, 1, dummyarray2, dummyarray1, 0);
#endif /*INTEGRATED*/

#if RS_FEC_CAPABILITY
  if(m_Epcfg.RSFecCapability){
    /* Initialize tables for RS encoder */
    def_GF();                   /* generate GF(2^8) tables*/
    make_genpoly(m_Epcfg.RSFecCapability); /* generate generator poly */
  }
#endif

  while(NULL != fgets(buf, sizeof(buf), ininfo)){
    int hlen, nclass;
    int clen, nclen, sptr, nptr, *tmp1, *tmp2;

   fprintf(stderr, "\r[%5d]",nFrames++); 

    if(1 != sscanf(buf, "%d", &translen)) {
      fprintf(stderr, "Data length Error\n");
    }
    readbits(instream, translen, trans); /* return value not evaluated (RS) */

#if RS_FEC_CAPABILITY
    /* RS Decoding */
    if(m_Epcfg.RSFecCapability){
      int e_target, rs_inflenmax;
      int nparity, lastinflen;
      int inflen;
      int icnt, dcnt, pcnt;

      e_target = m_Epcfg.RSFecCapability;
      rs_inflenmax = 255 - 2 * e_target;

      nparity = (translen/8 + 254) / 255;
      lastinflen = translen/8 - 255*(nparity-1) - 2*e_target;
      if(nparity != 1) printf("%d",nparity);

      pcnt = translen - 2*e_target*8*nparity;
      icnt = 0;
      for(i=0; i<nparity; i++){
        dcnt = 0;
        inflen = (i == nparity-1 ? lastinflen : rs_inflenmax);
        /*      printf("[%d/%d]", inflen, translen/8);*/

        for(j=0; j< inflen*8; j++)
          rs_data[dcnt++]= trans[icnt + j]; 
        for(j=0; j<2*e_target*8; j++){
          rs_data[dcnt++] = trans[pcnt++];
        }

        rs_codelen = RS_input(rs_data, rs_index, dcnt);
        rs_detect = RS_dec(rs_index, rs_out, rs_codelen, e_target);
        rs_codelen = RS_output(rs_out, rs_data, rs_codelen, e_target, 1);

        for(j=0; j< rs_codelen; j++){
          trans[icnt + j] = rs_data[j];
        }

        icnt += j;
      }
      translen -= 2*e_target*8 * nparity;
    }
#endif



    if(m_Epcfg.Interleave) {

      hlen = RetrieveHeaderDeinter(&m_Epcfg, &pred, trans, translen, ibuf, &nstuff);

      if(hlen == -1){ /*check header-length*/
        printf("H\n");
        fflush(stdout);
        fprintf(outinfo,"0\n");
/* #ifdef INTEGRATED */
/*         NullSideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
        NullSideInfoWrite(outinfo, &m_Epcfg, 0, check, &epcfg, CAtbl, CAbits, 1);
/* #endif */
        continue;
      }

      if(NULL == (tmp1 = (int *)calloc(sizeof(int), translen))){
        perror("tmp1"); exit(1);
      }
      if(NULL == (tmp2 = (int *)calloc(sizeof(int), translen))){
        perror("tmp2"); exit(1);
      }

      nclass = m_Epcfg.fattr[pred].ClassCount;
      clen = nclen = sptr = 0;
      /* Length other than last class */
      sptr = 0;
      for(i=0; i< CLASSMAX; i++) codedlen[i] = 0;  /* Initialization */

      for(i=0; i<nclass-1; i++){
        int srclen, rate;
        int rsconcat = 0;
	if(m_Epcfg.fattr[pred].cattr[i].TermSW==1 && m_Epcfg.fattr[pred].cattr[i].ClassCodeRate != 0){
	  srclen = m_Epcfg.fattr[pred].cattr[i].ClassBitCount +
	    m_Epcfg.fattr[pred].cattr[i].ClassCRCCount+4;
	}else{
	  srclen = m_Epcfg.fattr[pred].cattr[i].ClassBitCount +
	    m_Epcfg.fattr[pred].cattr[i].ClassCRCCount;
	}
        rate = m_Epcfg.fattr[pred].cattr[i].ClassCodeRate;
        if(m_Epcfg.fattr[pred].cattr[i].FECType == 0) /* SRCPC */
          codedlen[i] = SRCPCDepuncCount(sptr, sptr+srclen, 0, rate);
        else {                    /* RS */


          /* int rsconcat;*/
          rsconcat = 0;
          for(j=i; m_Epcfg.fattr[pred].cattr[j].FECType == 2; j++, rsconcat ++) ;
          for(j=0; j<rsconcat; j++){
            srclen += m_Epcfg.fattr[pred].cattr[j+1].ClassBitCount +  m_Epcfg.fattr[pred].cattr[j+1].ClassCRCCount;
          }  
          codedlen[i] = RSEncodeCount(sptr, sptr+srclen, rate);
          /* i += rsconcat;  */
          }
        sptr += srclen;
        if(m_Epcfg.fattr[pred].cattr[i].ClassInterleave) {
          clen += codedlen[i];
        }
        else {
          nclen += codedlen[i];
        }
        if (m_Epcfg.fattr[pred].cattr[i].FECType == 2)
          i += rsconcat;


      }



      if(translen < clen + hlen){ /* Check the length other than last */
        printf("s\n");
        fprintf(outinfo,"0\n");
/* #ifdef INTEGRATED */
/*         NullSideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
        NullSideInfoWrite(outinfo, &m_Epcfg, 0, check, &epcfg, CAtbl, CAbits, 1);
/* #endif */

        continue;
      }
      if (translen < hlen+clen+nclen+nstuff) { /* check all length */
        fprintf(outinfo,"0\n");
/* #ifdef INTEGRATED */
/*         NullSideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
        NullSideInfoWrite(outinfo, &m_Epcfg, 0, check, &epcfg, CAtbl, CAbits, 1);
/* #endif */
        continue;
      }
      /* Last class length */
      codedlen[nclass-1] = translen-hlen-clen-nclen-nstuff;
      if(m_Epcfg.fattr[pred].cattr[nclass-1].ClassInterleave)
        clen += codedlen[nclass-1];
      else
        nclen += codedlen[nclass-1];

      /* The number of Stuffing Bits */
      nclen += nstuff;

      /* De-interleaveing !!*/
      sptr = 0; nptr = 0;
      for(i=0; i<nclen; i++) ncbuf[i] = ibuf[i+clen];
      for(i=0; i<nclass; i++){
        int Interleave;
        Interleave = m_Epcfg.fattr[pred].cattr[i].ClassInterleave;
        if(Interleave == 0){ /* No interleave */
          for(j=0; j<codedlen[i]; j++)
            coded[sptr++] = ncbuf[nptr++];
        }else if(Interleave == 1 && m_Epcfg.fattr[pred].cattr[i].FECType == 0){ /*Intra w/o Inter */
          recdeinter(&coded[sptr], codedlen[i], codedlen[i], ibuf, clen-codedlen[i], ibuf);
          sptr += codedlen[i];
          clen -= codedlen[i];
        }else if(Interleave == 2 && m_Epcfg.fattr[pred].cattr[i].FECType == 0){ /*Intra w/o Inter */
          recdeinter(&coded[sptr], codedlen[i], PINTDEPTH, ibuf, clen-codedlen[i], ibuf);
          sptr += codedlen[i];
          clen -= codedlen[i];
	       
        } else if((Interleave == 1 || Interleave == 2) && (m_Epcfg.fattr[pred].cattr[i].FECType == 1 || m_Epcfg.fattr[pred].cattr[i].FECType == 2)){
	  recdeinterBytewise(&coded[sptr], codedlen[i], m_Epcfg.fattr[pred].cattr[i].ClassCodeRate, ibuf, clen-codedlen[i], ibuf);
	  sptr += codedlen[i];
	  clen -= codedlen[i];
	}else if(Interleave == 3){ 
	  sptr += codedlen[i];
	}
		
      }

      /* insert data for ClassInterleave == 3 */
      sptr=0;
      for(i=jcnt=0;i<nclass;i++){
	if(m_Epcfg.fattr[pred].cattr[i].ClassInterleave == 3){
	  for(j=0;j<codedlen[i];j++){
	    coded[sptr++]=ibuf[j+jcnt];
	  }
	  jcnt+= codedlen[i];
	}else{
	  sptr+=codedlen[i];
	}
      }

      clen = sptr;
      free(tmp1); free(tmp2);
    }
    else { /* Not Interleaved */
      hlen = RetrieveHeader(&m_Epcfg, &pred, trans, &nstuff);
      translen -= nstuff;
      nclass = m_Epcfg.fattr[pred].ClassCount;
      {
        int i, from, to, rate;
        clen = from = 0;
        for(i=0; i<nclass-1; i++){
	  if(m_Epcfg.fattr[pred].cattr[i].TermSW==1 && m_Epcfg.fattr[pred].cattr[i].ClassCodeRate != 0){
	    to = from + m_Epcfg.fattr[pred].cattr[i].ClassBitCount +
	      m_Epcfg.fattr[pred].cattr[i].ClassCRCCount+4;
	  }else{
	    to = from + m_Epcfg.fattr[pred].cattr[i].ClassBitCount +
	      m_Epcfg.fattr[pred].cattr[i].ClassCRCCount;
	  }

          rate = m_Epcfg.fattr[pred].cattr[i].ClassCodeRate;
          if(m_Epcfg.fattr[pred].cattr[i].FECType == 0) /*SRCPC*/
            clen += SRCPCDepuncCount(from, to, 0, rate);
          else{                  /* RS */
            int rsconcat;
            rsconcat = 0;
            for(j=i; m_Epcfg.fattr[pred].cattr[j].FECType == 2; j++, rsconcat ++) ;
            for(j=0; j<rsconcat; j++){
              to += m_Epcfg.fattr[pred].cattr[j+1].ClassBitCount +  m_Epcfg.fattr[pred].cattr[j+1].ClassCRCCount;
              }           
            clen += RSEncodeCount(from, to, rate);
            i += rsconcat;
	  }
          from = to;
        }
      }

      if ( hlen == -1) { /*check header-length*/
        printf("h\n");
        fflush(stdout);
        fprintf(outinfo,"0\n"); 
/* #ifdef INTEGRATED */
/*         NullSideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
        NullSideInfoWrite(outinfo, &m_Epcfg, 0, check, &epcfg, CAtbl, CAbits, 1);
/* #endif */
        continue;
      }

      if ( translen < clen + hlen ) {
        printf("s\n");
        fflush(stdout);
        fprintf(outinfo,"0\n");
/* #ifdef INTEGRATED */
/*         NullSideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
        NullSideInfoWrite(outinfo, &m_Epcfg, 0, check, &epcfg, CAtbl, CAbits, 1);
/* #endif */
        continue;
      }

      clen =translen - hlen;
      for(i=0; i<clen; i++)
        coded[i] = trans[i+hlen];
    }



    payloadlen = PayloadDecode(data, coded, clen, &m_Epcfg.fattr[pred], check);

    fprintf(outinfo, "%d\n", payloadlen);


    if(m_Epcfg.fattr[pred].ClassReorderedOutput){
      int bound[CLASSMAX+1];
      int order[CLASSMAX] = {0};
      bound[0] = 0;
      for(i=0; i<nclass; i++){
        bound[i+1] = bound[i] + m_Epcfg.fattr[pred].cattr[i].ClassBitCount;
        /*   printf("%d-th = %d bits\n",i, m_Epcfg.fattr[pred].cattr[i].ClassBitCount);*/
        for (j = 0; j <nclass; j++)
          { 
            if (m_Epcfg.fattr[pred].ClassOutputOrder[j] == m_Epcfg.fattr[pred].SortedClassOutputOrder[i])
              order[i] = j;
          }
      }
      
      for(i=0; i<nclass; i++){
        j = order[i];
        writebits(outstream, bound[j+1]-bound[j], &data[bound[j]]);
        /*printf("write from %d, %d bits, j = %d\n", bound[j] , bound[j+1]-bound[j],j);*/
      }
/* #ifdef INTEGRATED */
/*       SideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
      SideInfoWrite(outinfo, &m_Epcfg, pred, check, 1, &epcfg, CAtbl, CAbits, 1);
/* #endif */
    }else{
/* #ifdef INTEGRATED */
/*       SideInfoWriteMod(outinfo, epcfg, m_Epcfg, CAtbl, CAbits, pred, check); */
/* #else */
      SideInfoWrite(outinfo, &m_Epcfg, pred, check, 1, &epcfg, CAtbl, CAbits, 1);
/* #endif */
      writebits(outstream, payloadlen, data); fflush(outstream);
    }

    printf(".");
    fflush(stdout);
    fflush(outinfo);
  }
  writebits(outstream, -1, coded); /* Flush */
  printf("\n");
  return ( 0 );
}
