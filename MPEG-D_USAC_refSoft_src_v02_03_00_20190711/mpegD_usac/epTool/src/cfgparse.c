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
#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"

/* the define CONVERT might be used to output the non-integrated version
   while input the integrated version of a predefinition file */
#define CONVERT

int main(int argc, char **argv)
{
  int i,j;
  FILE *fpin;
  EPConfig  m_Epcfg;
#ifdef INTEGRATED
  /*   short int k; */
  int CAtbl[MAXPREDwCA][CLASSMAX], CAbits[MAXPREDwCA];
#ifdef CONVERT
  EPConfig epcfg;
  int PredCA[MAXPREDwCA][255];
#endif /*CONVERT*/
#else
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];
#endif /*INTEGRATED*/  

  if(argc != 2){
    fprintf(stderr,"Usage: %s <pred-file>\n", argv[0]);
    exit(1);
  }
  if(NULL == (fpin = fopen(argv[1], "r"))){
    perror(argv[1]);
    exit(1);
  }

#ifdef INTEGRATED
#ifdef CONVERT
  OutInfoParse(fpin, &epcfg, 0, CAtbl, CAbits, 1);
  PredSetConvert(&epcfg, &m_Epcfg, CAtbl, CAbits, PredCA, 0);
#else /*CONVERT*/
  OutInfoParse(fpin, &m_Epcfg, 0, CAtbl, CAbits, 1);
#endif /*CONVERT*/
#else /*INTEGRATED*/
  OutInfoParse(fpin, &m_Epcfg, 0, dummyarray2, dummyarray1, 0);
#endif /*INTEGRATED*/
 
  printf("NPred = %d\n", m_Epcfg.NPred);
  printf("Interleave = %d\n", m_Epcfg.Interleave);
  printf("BitStuffing = %d\n", m_Epcfg.BitStuffing);
  printf("Number of Concatenated Frames = %d\n", m_Epcfg.NConcat);

  for(i=0; i<m_Epcfg.NPred; i++){
    printf("For Definition #: %d\n", i);
    printf("\tClassCount = %d\n", m_Epcfg.fattr[i].ClassCount);
    for(j=0; j<m_Epcfg.fattr[i].ClassCount; j++){
      printf("\tFor Class:%d\n",j);
      printf("\t\tFECType: ");
      switch(m_Epcfg.fattr[i].cattr[j].FECType){
      case 0: printf("SRCPC\n"); break;
      case 1: printf("RS\n"); break;
      case 2: printf("RS (encoded together with the next class)\n"); break;
      default: printf("FEC Type ERROR!!! no type for value %d\n", m_Epcfg.fattr[i].cattr[j].FECType);
        exit(1);
      }

      if(m_Epcfg.fattr[i].cattresc[j].ClassBitCount){
        if(m_Epcfg.fattr[i].lenbit[j] != 0)
          printf("\t\tClassLength is variable and signalled with %d bits\n",
                 m_Epcfg.fattr[i].lenbit[j]);
        else
          printf("\t\tClassLength is variable but <Until the end>\n");
      }
      else {
        printf("\t\tClassLength is %d (fixed)\n",
               m_Epcfg.fattr[i].cattr[j].ClassBitCount);
      }
      if (!m_Epcfg.fattr[i].cattr[j].FECType) {
        if(m_Epcfg.fattr[i].cattresc[j].ClassCodeRate){
          printf("\t\tSRCPC Code rate is variable\n");
        }
        else {
          printf("\t\tSRCPC Code rate is %d (fixed)\n",
                 m_Epcfg.fattr[i].cattr[j].ClassCodeRate);
        }
      }
      else {
        if(m_Epcfg.fattr[i].cattresc[j].ClassCodeRate){
          printf("\t\tNr of correctable symboles is variable\n");
        }
        else {
          printf("\t\tNr of correctable symboles is %d (fixed)\n",
                 m_Epcfg.fattr[i].cattr[j].ClassCodeRate);
        }
      }
      if(m_Epcfg.fattr[i].cattresc[j].ClassCRCCount){
        printf("\t\tCRC Length is variable\n");
      }else{
        printf("\t\tCRC Length is %d (fixed)\n",
               m_Epcfg.fattr[i].cattr[j].ClassCRCCount);
      }
      if(m_Epcfg.NConcat > 1){
        printf("\t\tThis class is treated as %s class%s after concatenation\n",
               m_Epcfg.fattr[i].cattr[j].Concatenate ? "new one" : "independent",
               m_Epcfg.fattr[i].cattr[j].Concatenate ? "" : "s");
      }

      if(m_Epcfg.Interleave == 2){
        printf("\t\tThis class is%s interleaved (%d)\n",  
               m_Epcfg.fattr[i].cattr[j].ClassInterleave ? "" : " not",  
               m_Epcfg.fattr[i].cattr[j].ClassInterleave); 
      }
#ifdef INTEGRATED
#ifndef CONVERT
      if(CAtbl[i][j]) {
        printf("\t\t** This class is optional **\n");
      }
#endif /*CONVERT*/
#endif /*INTEGRATED*/
    }
    printf("\tClass Reordered Output is %s\n",
           m_Epcfg.fattr[i].ClassReorderedOutput ? "On" : "Off");
    if(m_Epcfg.fattr[i].ClassReorderedOutput){
      printf("\t Order: ");
      for(j=0; j<m_Epcfg.fattr[i].ClassCount; j++){
        printf("%2d ", m_Epcfg.fattr[i].ClassOutputOrder[j]);
      }
      printf("\n");
    }
  }

  printf("Header FEC extension is %sused.\n",m_Epcfg.HeaderProtectExtend?"":"not ");
  if(m_Epcfg.HeaderProtectExtend){
    printf("\tSRCPC Code rate is %d\n", m_Epcfg.HeaderRate);
    printf("\tCRC Length is %d\n", m_Epcfg.HeaderCRC);
  }
#if RS_FEC_CAPABILITY 
  printf("Correctable Errors of RS-code is %d\n", m_Epcfg.RSFecCapability);
  printf("\t-->%d bytes parity are added per %d bytes\n",
         2*m_Epcfg.RSFecCapability, 255-2*m_Epcfg.RSFecCapability);
#endif  

  return ( 0 );
}
