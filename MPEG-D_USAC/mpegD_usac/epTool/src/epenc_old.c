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
    MPEG-4 Audio Error Protection Tool Encoder
    1998 Feb. 24  Toshiro Kawahara
*/
/* modification by Markus Multrus, fhg
   2000 Sept. 12
*/

#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"
#include "dcm_io.h"


int main(int argc, char **argv)
{
  int i,j,k, nFrames = 0;
  int data[MAXDATA];
  int rs_index[256], rs_codelen, rs_dindex[256];
  unsigned int rs_code[256];
  int rs_code_bin[256*8];
  int rs_parity[127*(MAXDATA/255)];
  int nphdr[MAXHEADER];
  int bdyhdr[MAXHEADER];
  int h1len, h2len;
  int coded[MAXCODE];
  int trans[MAXDATA*2], translen;
  int codedlen[CLASSMAX], codedstart[CLASSMAX];
  char buf[256];
  FILE *inpred = NULL, *instream = NULL, *ininfo = NULL, *outstream = NULL, *outinfo = NULL;
  EPConfig nepcfg;
  int pred = 0;
  int nstuff;
  EPConfig  m_Epcfg;
#ifdef INTEGRATED
  EPConfig epcfg;
  int modpred;
  int CAtbl[MAXPREDwCA][CLASSMAX], CAbits[MAXPREDwCA];
  int PredCA[MAXPREDwCA][255];
#else
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];
#endif /*INTEGRATED*/  

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
  PredSetConvert(&epcfg, &m_Epcfg, CAtbl, CAbits, PredCA, 0);
#else /*INTEGRATED*/
  OutInfoParse(inpred, &m_Epcfg, 0, dummyarray2, dummyarray1, 0);
#endif /*INTEGRATED*/
  
  if(m_Epcfg.NConcat > 1){        /* Concatenation! */
    ConcatAttrib(&m_Epcfg, &nepcfg);
  }


  while(NULL != fgets(buf, sizeof(buf), ininfo)){
    int dlen, hlen, nclass, interleave, total, bitstuffing;
    int clen, nclen, cptr;

    if(1 != sscanf(buf, "%d", &dlen)) fprintf(stderr, "Data length Error\n");
    readbits(instream, dlen, data);

#ifdef INTEGRATED
    SideInfoParseMod (ininfo, &epcfg, &m_Epcfg, pred, &modpred, CAtbl, PredCA);
    pred = modpred;
#else /*INTEGRATED*/
    SideInfoParse(ininfo, &m_Epcfg, &pred, NULL, 0);
#endif /*INTEGRATED*/

    bitstuffing = m_Epcfg.BitStuffing;
    hlen = GenerateHeader(&m_Epcfg, pred, nphdr, &h1len, bdyhdr, &h2len, bitstuffing, 0);
    nclass = m_Epcfg.fattr[pred].ClassCount;
    interleave = m_Epcfg.Interleave; 

    /* initialize for codedlen */
    for(k=0;k<CLASSMAX;k++)codedlen[k]=0;
    total =  PayloadEncode(data, dlen, coded, &m_Epcfg.fattr[pred], codedlen);


    if(bitstuffing){
      nstuff = ((total + hlen + 7) /8)*8 - (total + hlen);
      hlen = GenerateHeader(&m_Epcfg, pred, nphdr, &h1len, bdyhdr, &h2len, bitstuffing, nstuff);
      total += nstuff;
    }else
      nstuff = 0;



    if(interleave){
      int *tmp3;
      if(NULL == (tmp3 = (int *)calloc(sizeof(int), total))){
        perror("tmp3"); exit(1);
      }
      clen = nclen = cptr = 0;

      translen = 0;
      codedstart[0] = 0;
      for(i=0; i<nclass-1; i++)
        codedstart[i+1] = codedstart[i] + codedlen[i];

      /* Concat Non-Interleaved classes */
      for(i=0; i<nclass; i++) {
        if (m_Epcfg.fattr[pred].ClassReorderedOutput)
          k = m_Epcfg.fattr[pred].OutputIndex[i] ;
        else
          k = i;
        if(!m_Epcfg.fattr[pred].cattr[k].ClassInterleave) {
          for(j=0; j<codedlen[i]; j++)
            tmp3[nclen++] = coded[codedstart[i]+j];
        }
      }
      /* Concat "Concatenation:3" classes */
      for(i=0; i<nclass; i++) {
        if (m_Epcfg.fattr[pred].ClassReorderedOutput)
          k = m_Epcfg.fattr[pred].OutputIndex[i] ;
        else
          k = i;
        if(m_Epcfg.fattr[pred].cattr[k].ClassInterleave == 3) {
          for(j=0; j<codedlen[i]; j++)
            trans[translen++] = coded[codedstart[i]+j];
          /*      printf("translen=%d\n",translen); */
        }
      }

      /* Interleave classes */
      for(i=nclass-1; i>=0; i--){
        int Interleave;
        if (m_Epcfg.fattr[pred].ClassReorderedOutput)
          k = m_Epcfg.fattr[pred].OutputIndex[i] ;
        else
          k = i;
        Interleave = m_Epcfg.fattr[pred].cattr[k].ClassInterleave;

        if(m_Epcfg.fattr[pred].cattr[k].FECType==0){
          if(Interleave == 1){ /* Intra w/o Inter */
            translen = recinter(&coded[codedstart[i]], codedlen[i], codedlen[i], trans, translen, trans);
            cptr += codedlen[i];
          }else if(Interleave == 2){ /* Intra w/ Inter */
            translen = recinter(&coded[codedstart[i]], codedlen[i], PINTDEPTH, trans, translen, trans); 
            cptr += codedlen[i];
          }

        }
        else 
          if(Interleave == 1 || Interleave == 2){
            /* bytewise interleave for RS  */
            translen = recinterBytewise(&coded[codedstart[i]], codedlen[i], m_Epcfg.fattr[pred].cattr[k].ClassCodeRate,trans, translen, trans);
            cptr += codedlen[i];
          }
      }

      for(i=0; i<nstuff; i++)
        tmp3[nclen++] = 0;
      
      for(i=0; i<nclen; i++)
        trans[translen++] = tmp3[i];
      translen = recinter(bdyhdr, h2len, propdepth(h2len), trans, translen, trans);
      translen = recinter(nphdr, h1len, propdepth(h1len), trans, translen, trans);
      free(tmp3);
    }else{
      for(i=j=0; i<h1len; i++)
        trans[j++] = nphdr[i];
      for(i=0; i<h2len; i++)
        trans[j++] = bdyhdr[i];
      for(i=0; i<total; i++)
        trans[j++] = coded[i];
      translen = j;
    }
    writebits(outstream, translen, trans);

#if RS_FEC_CAPABILITY
    /* RS Encoding */
    if(m_Epcfg.RSFecCapability){
      int e_target, rs_inflenmax;
      int nparity;
      int pcnt, icnt;
      int inflen;

      /* Initialize tables for RS encoder */
      make_genpoly(m_Epcfg.RSFecCapability); /* generate generator poly */

      e_target = m_Epcfg.RSFecCapability;
      rs_inflenmax = 255 - 2 * e_target;
      nparity = (translen/8 + rs_inflenmax - 1) / rs_inflenmax;
      pcnt = icnt = 0;

      if(nparity != 1) printf("%d", nparity);
      for(i=0; i< nparity; i++){
        if(i == nparity - 1) /*last*/
          inflen = (translen - icnt)/8;
        else
          inflen = rs_inflenmax;
        /*      printf("[inflen = %d/%d]", inflen, translen/8); */

        rs_codelen = RS_input(&trans[icnt], rs_index, inflen*8);
        rs_codelen = RS_enc(rs_index, rs_codelen, rs_code, e_target);
        rs_codelen = RS_output(rs_code, rs_code_bin, rs_codelen, e_target, 0);

        {
          /* Local Decode for Debugging. */
          unsigned int test[255];
          int testbin[255*8], tmp, flg, err;

          tmp = RS_input(rs_code_bin, rs_dindex, rs_codelen);
          err = RS_dec(rs_dindex, test, tmp, e_target);
          tmp = RS_output(test, testbin, tmp, e_target, 1);

          flg = 0;
          for(j=0; j<tmp; j++)
            if(testbin[j] != rs_code_bin[j]){
              printf("![%d]",j);
              flg = 1;
            }
          if(flg){
            printf("\nBUG!!(%d) tmp=%d, inflen =%d, translen/8 = %d rs_codelen = %d\n",
                   i, tmp, inflen, translen/8, rs_codelen);
          }
        }

        for(j=0; j<inflen*8; j++){
          if(rs_code_bin[j] != trans[icnt+j])
            printf("!BUG!");
        }
        for(j=0; j<2*e_target*8; j++){
          rs_parity[pcnt++] = rs_code_bin[inflen*8+j];
        }

        icnt += inflen*8;
      }
      if(icnt != translen) printf("<%d,%d>", icnt,translen);
      writebits(outstream, pcnt, rs_parity);
      translen += pcnt;
    }
#endif

    fprintf(outinfo, "%d\n", translen); fflush(outinfo);
    printf("\r[%d]", nFrames++);fflush(stdout);
  }
  writebits(outstream, -1, coded); /* Flush */
  printf("\n");
  return ( 0 );
}
  
