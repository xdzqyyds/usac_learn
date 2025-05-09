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
   MPEG-4 Audio Error Protection Tool
   Outband Information Parse
   1998 Feb. 23  Toshiro Kawahara
*/
/* modification by Markus Multrus, fhg
   2000 Sept. 12
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eptool.h"

#define FMTERR(A) {fprintf(stderr,"%s\n",(A)); exit(1);}
#define D(a) {const void *dummyfilepointer; dummyfilepointer = &a;}

void OutInfoParse(FILE *inffp, EPConfig *epcfg, int reorderFlag, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int CA)
{
  int i;
  int j;
  char buf[1024];
  int npred;
  EPFrameAttrib *fattr;
  int nclass;
  int TotalIninf;
  ClassAttribContent *cattresc;
  ClassAttribContent *cattr;
  int *lenbit;
  int interleave;
  unsigned int short minNrOfValuesInPredFile = 4; /* concat != 1         : +1
                                                     fecType == 0        : +1
                                                     interleaveType == 2 : +1 
                                                     CA == 1             : +1 */

  TotalIninf = 0;
  /* Number of predefined set */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: #pred NULL");
  if(1 != sscanf(buf, "%d", &npred)) FMTERR("#pred!!");
  if (!(epcfg->NPred = npred)){
    fprintf (stderr, "EP_Tool: OutInfoParse(): outinfo.c: Error: Please set number_of_predefined_set unequal 0!");
    exit(1);
  }
  
  /* Interleave ? */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: IL");
  if(1 != sscanf(buf, "%d", &(epcfg->Interleave))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: IL");
  interleave = epcfg->Interleave;
  if ( interleave == 2 ) {
    minNrOfValuesInPredFile++;
  }
  /* BitStuffing ? */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: BS");
  if(1 != sscanf(buf, "%d", &(epcfg->BitStuffing))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: BS");
  if(epcfg->BitStuffing) TotalIninf += 3;

  /* Concatenate ? */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CONCAT");
  if(1 != sscanf(buf, "%d", &(epcfg->NConcat))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CONCAT");
  if (! epcfg->NConcat ) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CONCAT");
  if ( epcfg->NConcat != 1 ) { 
    minNrOfValuesInPredFile++; 
  } 
  if ( CA ){
    TotalIninf = 0;
    minNrOfValuesInPredFile++;
  }

  if(NULL == (fattr = (EPFrameAttrib *)calloc(npred, sizeof(EPFrameAttrib)))){
    fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: Unable to calloc for EPFrameAttrib\n");
    exit(1);
  }
  epcfg->fattr = fattr;
  
  for(i=0; i<npred; i++){
    if ( CA )
      CAbits[i] = 0;
    else
      fattr[i].TotalIninf = TotalIninf;

    if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: NCLASS");
    if(1 != sscanf(buf, "%d", &nclass)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: NCLASS");
    fattr[i].ClassCount = nclass;

    if(nclass > CLASSMAX){
      fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: Error: CLASSMAX (in eptool.h) exceeded: %d\n", nclass);
      exit(1);
    }

    if(NULL == (cattr = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent)))){
      fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: Unable to calloc for Cattr\n");
      exit(1);
    }
    if(NULL == (cattresc = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent)))){
      fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: Unable to calloc for CattrESC\n");
      exit(1);
    }
    if(NULL == (lenbit = (int *)calloc(nclass, sizeof(int)))){
      fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: Unable to calloc for CattrESC\n");
      exit(1);
    }
    fattr[i].cattresc = cattresc;
    fattr[i].cattr = cattr;
    fattr[i].lenbit = lenbit;

    for(j=0; j<nclass; j++){
      unsigned int short minNrOfValuesInPredFileTmp = minNrOfValuesInPredFile;
      int nrOfValues, arg[8], cnt;
      if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: FIXED");
      if ( CA ){
        nrOfValues = sscanf(buf, "%d %d %d %d %d %d %d %d", &arg[0],&arg[1],&arg[2], &arg[3], &arg[4],&arg[5],&arg[6],&arg[7]);
      }
      else{
        nrOfValues = sscanf(buf, "%d %d %d %d %d %d %d", &arg[0],&arg[1],&arg[2], &arg[3], &arg[4],&arg[5],&arg[6]);
      }
      if ( nrOfValues < minNrOfValuesInPredFileTmp ) {
        fprintf (stderr, "EP_Tool: OutInfoParse(): outinfo.c: predefinition file erroneous (1)\n");
        exit(1);
      }
      cnt=0;
      cattresc[j].ClassBitCount = arg[cnt++];
      cattresc[j].ClassCodeRate = arg[cnt++];
      cattresc[j].ClassCRCCount = arg[cnt++];

      if (epcfg->NConcat !=1){
	cattr[j].Concatenate = arg[cnt++];
      }
      
      cattr[j].FECType = arg[cnt++];
      
      if(cattr[j].FECType == 0){
        if ( nrOfValues < ++minNrOfValuesInPredFileTmp ) { 
          fprintf (stderr, "EP_Tool: OutInfoParse(): outinfo.c: predefinition file erroneous (2)\n"); 
           exit(1); 
        } 
	cattr[j].TermSW = arg[cnt++];
      }
      if(interleave == 2){
	cattr[j].ClassInterleave = arg[cnt++];
      }
      
      if (interleave == 0 ) { 
        cattr[j].ClassInterleave = 0; 
      } 
      if (interleave == 1 ) { 
        cattr[j].ClassInterleave = (j != nclass-1 ? 2 : 1); 
      } 
      if ( CA ){
        if ( CAtbl == NULL ){
          fprintf (stderr, "EP_Tool: OutInfoParse(): outinfo.c: Error: CAtbl must not be NULL!");
          exit(1);
        } 
        CAtbl[i][j] = arg[cnt];
        CAbits[i] += CAtbl[i][j];
      }
      
      if(cattresc[j].ClassBitCount){         /* If LEN=ESC # of bit is set*/
        if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: LENBIT");
        if(1 != sscanf(buf, "%d", &lenbit[j])) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: LENBIT");
        fattr[i].TotalIninf += lenbit[j];
      }else{
        if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: LEN");
        if(1 != sscanf(buf, "%ud", &(cattr[j].ClassBitCount))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: LEN");
      }
      if(cattresc[j].ClassCodeRate){
        fattr[i].TotalIninf += 3;
      }else{
        if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: RATE");
        if(1 != sscanf(buf, "%d", &(cattr[j].ClassCodeRate))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: RATE");
      }
      if(cattresc[j].ClassCRCCount){
        fattr[i].TotalIninf += 3;
      }else{
        if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CRCLEN");
        if(1 != sscanf(buf, "%d", &(cattr[j].ClassCRCCount))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CRCLEN");
        switch(cattr[j].ClassCRCCount){
        case 17:
          cattr[j].ClassCRCCount = 24;
          break;
        case 18:
          cattr[j].ClassCRCCount = 32;
          break;
        default:
          break;
        }
      }
    }
    if ( CA )
      fattr[i].TotalIninf = TotalIninf;

    /* Class Reordered Output */
    if(NULL == fgets(buf, sizeof(buf), inffp)) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CRO");
    }
    if(1 != sscanf(buf, "%d", &(fattr[i].ClassReorderedOutput))) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CRO");
    }
    if(fattr[i].ClassReorderedOutput){
      int appeared[CLASSMAX]={0};
      for(j=0; j<nclass; j++){
        if(1 != fscanf(inffp, "%d", &(fattr[i].ClassOutputOrder[j]))) {
          FMTERR("EP_Tool: OutInfoParse(): outinfo.c: Error reading Predefinition File: ClassOutputOrder");
        }
        if ( fattr[i].ClassOutputOrder[j] >= nclass || fattr[i].ClassOutputOrder[j] < 0 || appeared[fattr[i].ClassOutputOrder[j]]) {
          FMTERR("EP_Tool: OutInfoParse(): outinfo.c: CRO-1");
        }
        appeared[fattr[i].ClassOutputOrder[j]]=1;
      }
      fgets(buf, sizeof(buf), inffp);
    }
    if ( !CA ){
      if (fattr[i].ClassReorderedOutput && reorderFlag)
        ReorderOutputClasses(&fattr[i], reorderFlag) ;
    }
  }

  /* Header Protection Extension */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: HEADERPRO");
  if(1 != sscanf(buf, "%d", &(epcfg->HeaderProtectExtend))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: HEADERPRO");
  if(epcfg->HeaderProtectExtend == 1){
    if(NULL == fgets(buf, sizeof(buf), inffp)) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: E-SRCPC");
    }
    if(1 != sscanf(buf, "%d", &(epcfg->HeaderRate))) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: E-SRCPC");
    }
    if(NULL == fgets(buf, sizeof(buf), inffp)) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: E-CRCLEN");
    }
    if(1 != sscanf(buf, "%d", &(epcfg->HeaderCRC))) {
      FMTERR("EP_Tool: OutInfoParse(): outinfo.c: E-CRCLEN");
    }
    switch(epcfg->HeaderCRC){
    case 17:
      epcfg->HeaderCRC = 24;
      break;
    case 18:
      epcfg->HeaderCRC = 32;
      break;
    default:
      break;
    }
  }
  
#if RS_FEC_CAPABILITY
  /* Reed-Solomon FEC Capability (number of correctable errors) */
  if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: RSCAP");
  if(1 != sscanf(buf, "%d", &(epcfg->RSFecCapability))) FMTERR("EP_Tool: OutInfoParse(): outinfo.c: HEADERPRO");
  if(!epcfg->BitStuffing && epcfg->RSFecCapability){
    fprintf(stderr, "EP_Tool: OutInfoParse(): outinfo.c: RS should be used with stuffing!!\n");
    exit(1);
  }
#endif

}

void 
SideInfoParse(FILE *inffp, EPConfig *epcfg, int *pred, int *check, int CA)
{
  int i;
  char buf[255];
  EPFrameAttrib *fattr;
  int nclass;

  /* Pred Signaling */
  if(epcfg->NPred > 1){
    if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: #pred");
    if(1 != sscanf(buf, "%d", pred)) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: #pred");
  }else
    *pred = 0;
  fattr = &epcfg->fattr[*pred];
  nclass = fattr->ClassCount;
  if ( CA ){
    for (i=0;i<nclass; i++) { /* For CRC Check */
      fscanf (inffp, "%d", &check[i]);
    }
  }
  fgets(buf, sizeof(buf), inffp); /* skip to next line */ 

  for(i=0; i<nclass; i++){
    if(fattr->cattresc[i].ClassBitCount){ /* Class Length ESC */
      if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: LENBIT");
      if(1 != sscanf(buf, "%ud", &(fattr->cattr[i].ClassBitCount))) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: LENBIT");
    }
    if(fattr->cattresc[i].ClassCodeRate){ /* SRCPC rate ESC */
      if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: RATE");
      if(1 != sscanf(buf, "%d", &(fattr->cattr[i].ClassCodeRate))) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: RATE");
    }
    if(fattr->cattresc[i].ClassCRCCount){ /* Class Length ESC */
      if(NULL == fgets(buf, sizeof(buf), inffp)) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: CRCLEN");
      if(1 != sscanf(buf, "%d", &(fattr->cattr[i].ClassCRCCount))) FMTERR("EP_Tool: SideInfoParse(): outinfo.c: CRCLEN");
      switch(fattr->cattr[i].ClassCRCCount){
      case 17:
        fattr->cattr[i].ClassCRCCount = 24;
        break;
      case 18:
        fattr->cattr[i].ClassCRCCount = 32;
        break;
      default:
        break;
      }
    }
  }
}


void
SideInfoWrite(FILE *outfp, EPConfig *modepcfg, int modpred, int modcheck[], int reorderFlag, EPConfig *origepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int integrated)
{
  int i;
  int j;
  EPFrameAttrib *fattr;
  int nclass;
  int order[CLASSMAX] = {0} ;
  

#ifdef INTEGRATED
  if ( integrated ){
    int k;
    int revPred[MAXPREDwCA];
    int revCA[MAXPREDwCA];
    int origcheck[CLASSMAX];
    int CA, skipped;
    int modnpred, origpred;
    int CAptr[MAXPREDwCA][CLASSMAX];
    
    
    reorderFlag = 0;
    /* Count the number of Pred-set after conversion */
    modnpred = 0;
    for(i=0; i<origepcfg->NPred; i++){
      int ptr;
      ptr = 0;
      for(j=0; j< origepcfg->fattr[i].ClassCount; j++){
        if(CAtbl[i][j] == 1){ /* CA switch */
          CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
          ptr++;
        }
      }
      
      for(j=0; j< (1<<CAbits[i]); j++){  /* #modepred for this origpred */ 
        CA = 0;
        for(k=0; k<CAbits[i]; k++)
          if(j & (0x01 << k))
            CA |= (0x01 << CAptr[i][k]);
        revPred[modnpred] = i;
        revCA[modnpred] = CA;
        modnpred ++;
        
        if(modnpred > MAXPREDwCA){
          fprintf(stderr, "EP_Tool: SideInfoWrite(): outinfo.c: Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h\n") ;
          exit(1);
        }
      }
    }
    
    origpred = revPred[modpred];
    CA = revCA[modpred];
    skipped = 0;
    
    if (modepcfg->fattr[modpred].ClassReorderedOutput)
      {
        for (i=0; i<modepcfg->fattr[modpred].ClassCount; i++)
          {
            for (j = 0; j <modepcfg->fattr[modpred].ClassCount; j++)
              { 
                if (modepcfg->fattr[modpred].ClassOutputOrder[j] == modepcfg->fattr[modpred].SortedClassOutputOrder[i])
                  order[i] = j;
              }
            /*fprintf(stderr,"wrote class %d, length %d\n",order[i],fattr->cattr[order[i]].ClassBitCount); */
          }
      }
    
    for(i=0; i<origepcfg->fattr[origpred].ClassCount; i++){
      /*      printf("pred %d -> %d,  revCA[%d] = %d\n", origpred, modpred, modpred, revCA[modpred]);*/ 
      if ( origepcfg->fattr[origpred].ClassReorderedOutput ){
        k = order[i-skipped];
      }
      else{ 
        k = i-skipped;
      }
      
      if(CAtbl[origpred][i] == 1 && (revCA[modpred] & (1<<i))){  
        origepcfg->fattr[origpred].cattr[i].ClassBitCount = 0;  
        origcheck[i] = 0;  
        skipped += 1;  
      }else{ 
        origcheck[i] = modcheck[k]; 
        origepcfg->fattr[origpred].cattr[i].ClassBitCount = 
          modepcfg->fattr[modpred].cattr[k].ClassBitCount; 
        origepcfg->fattr[origpred].cattr[i].ClassCodeRate = 
          modepcfg->fattr[modpred].cattr[k].ClassCodeRate; 
        origepcfg->fattr[origpred].cattr[i].ClassCRCCount = 
          modepcfg->fattr[modpred].cattr[k].ClassCRCCount;
      }
      
    }
    modpred = origpred;
    for (i=0; i<modepcfg->fattr[modpred].ClassCount; i++){
      modcheck[i] = origcheck[i];
    }
    if (origepcfg->NPred > 1){
      fprintf(outfp, "%d\n", modpred);
    }
    fattr = &origepcfg->fattr[origpred];
  }
  else{
    /* Pred Signaling */
    if(modepcfg->NPred > 1){
      fprintf(outfp, "%d\n", modpred);
    }
    fattr = &modepcfg->fattr[modpred];
  }
    
#else
  
  /* Pred Signaling */
  if(modepcfg->NPred > 1){
    fprintf(outfp, "%d\n", modpred);
  }
  fattr = &modepcfg->fattr[modpred];
  D(origepcfg);
  D(CAtbl);
  D(CAbits);
  D(integrated);
  
  
#endif

  nclass = fattr->ClassCount;

  if (fattr->ClassReorderedOutput && reorderFlag)
    {
      for (i=0; i<nclass; i++)
        {
          for (j = 0; j <nclass; j++)
            { 
              if (fattr->ClassOutputOrder[j] == fattr->SortedClassOutputOrder[i])
                order[i] = j;
            }
          /*fprintf(stderr,"wrote class %d, length %d\n",order[i],fattr->cattr[order[i]].ClassBitCount); */
        }
    }
  for(i=0; i<nclass; i++)
    {
      if (fattr->ClassReorderedOutput && reorderFlag)
        j = order[i];
      else
        j = i;
        
      fprintf(outfp, "%d ", modcheck[j]);
    }
  fprintf(outfp, "\n");
  for(i=0; i<nclass; i++){
    if (fattr->ClassReorderedOutput && reorderFlag)
      j = order[i];
    else
      j = i;
    if(fattr->cattresc[j].ClassBitCount){ /* Class Length ESC */
      fprintf(outfp, "%d\n", fattr->cattr[j].ClassBitCount);
    }
    if(fattr->cattresc[j].ClassCodeRate){ /* SRCPC rate ESC */
      fprintf(outfp, "%d\n", fattr->cattr[j].ClassCodeRate);
    }
    if(fattr->cattresc[j].ClassCRCCount){ /* Class Length ESC */
      fprintf(outfp, "%d\n", fattr->cattr[j].ClassCRCCount);
    }
  }
}

void
NullSideInfoWrite(FILE *outfp, EPConfig *modepcfg, int modpred, int modcheck[], EPConfig *origepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int integrated)
{
  int i;
  EPFrameAttrib *fattr = NULL;
  int nclass;

#ifdef INTEGRATED
  if ( integrated ){
    int j,k;
    int revPred[MAXPREDwCA];
    int revCA[MAXPREDwCA];
    int origcheck[CLASSMAX];
    int CA, skipped;
    int modnpred, origpred;
    EPFrameAttrib *fattr;
    int CAptr[MAXPREDwCA][CLASSMAX];
    int order[CLASSMAX] = {0} ;
    
    /* Count the number of Pred-set after conversion */
    modnpred = 0;
    for(i=0; i<origepcfg->NPred; i++){
      int ptr;
      ptr = 0;
      for(j=0; j< origepcfg->fattr[i].ClassCount; j++){
        if(CAtbl[i][j] == 1){ /* CA switch */
          CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
          ptr++;
        }
      }
      
      for(j=0; j< (1<<CAbits[i]); j++){  /* #modepred for this origpred */ 
        CA = 0;
        for(k=0; k<CAbits[i]; k++)
          if(j & (0x01 << k))
            CA |= (0x01 << CAptr[i][k]);
        revPred[modnpred] = i;
        revCA[modnpred] = CA;
        modnpred ++;
        
        if(modnpred > MAXPREDwCA){
          fprintf(stderr, "EP_Tool: NullSideInfoWrite(): outinfo.c: Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h\n") ;
          exit(1);
        }
      }
    }
    
    origpred = revPred[modpred];
    CA = revCA[modpred];
    skipped = 0;
    
    if (modepcfg->fattr[modpred].ClassReorderedOutput)
      {
        for (i=0; i<modepcfg->fattr[modpred].ClassCount; i++)
          {
            for (j = 0; j <modepcfg->fattr[modpred].ClassCount; j++)
              { 
                if (modepcfg->fattr[modpred].ClassOutputOrder[j] == modepcfg->fattr[modpred].SortedClassOutputOrder[i])
                  order[i] = j;
              }
            /*fprintf(stderr,"wrote class %d, length %d\n",order[i],fattr->cattr[order[i]].ClassBitCount); */
          }
      }
    
    for(i=0; i<origepcfg->fattr[origpred].ClassCount; i++){
      /*      printf("pred %d -> %d,  revCA[%d] = %d\n", origpred, modpred, modpred, revCA[modpred]);*/ 
      if ( origepcfg->fattr[origpred].ClassReorderedOutput ){
        k = order[i-skipped];
      }
      else{ 
        k = i-skipped;
      }
      
      if(CAtbl[origpred][i] == 1 && (revCA[modpred] & (1<<i))){  
        origepcfg->fattr[origpred].cattr[i].ClassBitCount = 0;  
        origcheck[i] = 0;  
        skipped += 1;  
      }else{ 
        origcheck[i] = modcheck[k]; 
        origepcfg->fattr[origpred].cattr[i].ClassBitCount = 
          modepcfg->fattr[modpred].cattr[k].ClassBitCount; 
        origepcfg->fattr[origpred].cattr[i].ClassCodeRate = 
          modepcfg->fattr[modpred].cattr[k].ClassCodeRate; 
        origepcfg->fattr[origpred].cattr[i].ClassCRCCount = 
          modepcfg->fattr[modpred].cattr[k].ClassCRCCount;
      }
      
    }
    modpred = origpred;
    for (i=0; i<modepcfg->fattr[modpred].ClassCount; i++){
      modcheck[i] = origcheck[i];
    }
    if (origepcfg->NPred > 1){
      fprintf(outfp, "%d\n", modpred);
    }
    fattr = &origepcfg->fattr[origpred];
  }
  else{
    /* Pred Signaling */
    if(modepcfg->NPred > 1){
      fprintf(outfp, "%d\n", modpred);
    }
    fattr = &modepcfg->fattr[modpred];
  }
  
#else

  /* Pred Signaling */
  if(modepcfg->NPred > 1){
    fprintf(outfp, "%d\n", modpred);
  }
  fattr = &modepcfg->fattr[modpred];
  D(origepcfg);
  D(CAtbl);
  D(CAbits);
  D(integrated);
  D(modcheck);
  
  
#endif
  
  nclass = fattr->ClassCount;
  for(i=0; i<nclass; i++)
    fprintf(outfp, "%d ", 1);
  fprintf(outfp, "\n");
  for(i=0; i<nclass; i++){
    if(fattr->cattresc[i].ClassBitCount){ /* Class Length ESC */
      fprintf(outfp, "%d\n", 0);
    }
    if(fattr->cattresc[i].ClassCodeRate){ /* SRCPC rate ESC */
      fprintf(outfp, "%d\n", fattr->cattr[i].ClassCodeRate);
    }
    if(fattr->cattresc[i].ClassCRCCount){ /* Class CRC ESC */
      switch(fattr->cattr[i].ClassCRCCount){
      case 24:
        fprintf(outfp, "%d\n", 17);
        break;
      case 32:
        fprintf(outfp, "%d\n", 18);
        break;
      default:
        fprintf(outfp, "%d\n", fattr->cattr[i].ClassCRCCount);
        break;
      }
    }
  }
}

int IntCompare(const int *v1, const int *v2)
{
  return *v1-*v2;
}

void ReorderOutputClasses (EPFrameAttrib *cfattr, int reorderFlag)
{
  int i,j ;
  EPFrameAttrib *fattr;
  ClassAttribContent *cattresc;
  ClassAttribContent *cattr;
  int *lenbit;

  int nclass = cfattr->ClassCount;

  for(i=0; i<nclass; i++){
    cfattr->SortedClassOutputOrder[i] = cfattr->ClassOutputOrder[i];
  }

  qsort(cfattr->SortedClassOutputOrder,nclass, sizeof(int),(int (*)(const void *, const void *))IntCompare);

  for(i=0; i<nclass; i++)
    {
      for (j = 0; j <nclass; j++)
        { 
          if (cfattr->ClassOutputOrder[i] == cfattr->SortedClassOutputOrder[j])
            cfattr->OutputIndex[i] = j ;
        }
    } 

  if (reorderFlag) {
    if(NULL == (fattr = (EPFrameAttrib *)calloc(1, sizeof(EPFrameAttrib))))
      {
        fprintf(stderr, "EP_Tool: ReorderOutputClasses(): outinfo.c: Unable to calloc for EPFrameAttrib\n");
        exit(1);
      }

    if(NULL == (cattr = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent))))
      {
        fprintf(stderr, "EP_Tool: ReorderOutputClasses(): outinfo.c: Unable to calloc for Cattr\n");
        exit(1);
      }
  
    if(NULL == (cattresc = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent))))
      {
        fprintf(stderr, "EP_Tool: ReorderOutputClasses(): outinfo.c: Unable to calloc for CattrESC\n");
        exit(1);
      }
    if(NULL == (lenbit = (int *)calloc(nclass, sizeof(int))))
      {
        fprintf(stderr, "EP_Tool: ReorderOutputClasses(): outinfo.c: Unable to calloc for CattrESC\n");
        exit(1);
      }

    fattr->cattresc = cattresc;
    fattr->cattr = cattr;
    fattr->lenbit = lenbit;

    /* store temporary */
    for (i = 0; i< nclass; i++)
      {
        fattr->cattresc[i].ClassBitCount   = cfattr->cattresc[i].ClassBitCount;
        fattr->cattresc[i].ClassCodeRate   = cfattr->cattresc[i].ClassCodeRate;
        fattr->cattresc[i].ClassCRCCount   = cfattr->cattresc[i].ClassCRCCount;
        fattr->cattresc[i].Concatenate     = cfattr->cattresc[i].Concatenate;
        fattr->cattresc[i].FECType         = cfattr->cattresc[i].FECType;
        fattr->cattresc[i].TermSW          = cfattr->cattresc[i].TermSW;
        fattr->cattresc[i].ClassInterleave = cfattr->cattresc[i].ClassInterleave;

        fattr->cattr[i].ClassBitCount   = cfattr->cattr[i].ClassBitCount;
        fattr->cattr[i].ClassCodeRate   = cfattr->cattr[i].ClassCodeRate;
        fattr->cattr[i].ClassCRCCount   = cfattr->cattr[i].ClassCRCCount;
        fattr->cattr[i].Concatenate     = cfattr->cattr[i].Concatenate;
        fattr->cattr[i].FECType         = cfattr->cattr[i].FECType;
        fattr->cattr[i].TermSW          = cfattr->cattr[i].TermSW;
        fattr->cattr[i].ClassInterleave = cfattr->cattr[i].ClassInterleave;
    
        fattr->lenbit[i]                = cfattr->lenbit[i];
      }
   
    /* reorder */
    for (i = 0; i< cfattr->ClassCount; i++)
      {
        cfattr->cattresc[i].ClassBitCount   = fattr->cattresc[cfattr->OutputIndex[i]].ClassBitCount;
        cfattr->cattresc[i].ClassCodeRate   = fattr->cattresc[cfattr->OutputIndex[i]].ClassCodeRate;
        cfattr->cattresc[i].ClassCRCCount   = fattr->cattresc[cfattr->OutputIndex[i]].ClassCRCCount;
        cfattr->cattresc[i].Concatenate     = fattr->cattresc[cfattr->OutputIndex[i]].Concatenate;
        cfattr->cattresc[i].FECType         = fattr->cattresc[cfattr->OutputIndex[i]].FECType;
        cfattr->cattresc[i].TermSW          = fattr->cattresc[cfattr->OutputIndex[i]].TermSW;
        cfattr->cattresc[i].ClassInterleave = fattr->cattresc[cfattr->OutputIndex[i]].ClassInterleave;

        cfattr->cattr[i].ClassBitCount   = fattr->cattr[cfattr->OutputIndex[i]].ClassBitCount;
        cfattr->cattr[i].ClassCodeRate   = fattr->cattr[cfattr->OutputIndex[i]].ClassCodeRate;
        cfattr->cattr[i].ClassCRCCount   = fattr->cattr[cfattr->OutputIndex[i]].ClassCRCCount;
        cfattr->cattr[i].Concatenate     = fattr->cattr[cfattr->OutputIndex[i]].Concatenate;
        cfattr->cattr[i].FECType         = fattr->cattr[cfattr->OutputIndex[i]].FECType;
        cfattr->cattr[i].TermSW          = fattr->cattr[cfattr->OutputIndex[i]].TermSW;
        cfattr->cattr[i].ClassInterleave = fattr->cattr[cfattr->OutputIndex[i]].ClassInterleave;
    
        cfattr->lenbit[i]                = fattr->lenbit[cfattr->OutputIndex[i]];
      }

    free(cattresc);
    free(cattr);
    free(lenbit);
    free(fattr);
  }
}


#ifdef INTEGRATED
void SideInfoParseMod(FILE *foriginf, EPConfig *origepcfg, EPConfig *modepcfg, int origpred, int *modpred, int CAtbl[MAXPREDwCA][CLASSMAX], int PredCA[MAXPREDwCA][255]){  
  int i;
  int CA = 0;
  int skipped;
  int modcheck[CLASSMAX];
  int origcheck[CLASSMAX] = {0};
       

  SideInfoParse(foriginf, origepcfg, &origpred, NULL, 0);
  /*    printf(".");fflush(stdout);*/
  /* First Pass -> to check CA application */
  CA = 0;
  for (i=0; i<origepcfg->fattr[origpred].ClassCount; i++){
    if (CAtbl[origpred][i] == 1 &&
        origepcfg->fattr[origpred].cattr[i].ClassBitCount == 0)
         CA |= 1 << i;
  }
  
  *modpred = PredCA[origpred][CA];
  skipped = 0;
  for  (i=0; i < origepcfg->fattr[origpred].ClassCount; i++){
    if (CAtbl [origpred][i] == 1 && origepcfg->fattr[origpred].cattr[i].ClassBitCount == 0)
      skipped += 1;
       else{
         modepcfg->fattr[*modpred].cattr[i-skipped].ClassBitCount =
           origepcfg->fattr[origpred].cattr[i].ClassBitCount;
         modepcfg->fattr[*modpred].cattr[i-skipped].ClassCodeRate =
           origepcfg->fattr[origpred].cattr[i].ClassCodeRate;
         modepcfg->fattr[*modpred].cattr[i-skipped].ClassCRCCount = 
           origepcfg->fattr[origpred].cattr[i].ClassCRCCount;
         modcheck[i-skipped] = origcheck[i];
       }
  }
}
#endif

void PredSetConvert(EPConfig *origepcfg, EPConfig *modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int origPredForCAPred[MAXPREDwCA], int reorderFlag )
{
  int modnpred = 0, nclass = 0;
  int i, j, k, index, cpNdx;
  int class_cnt;
  int offset ;
  int CAptr[MAXPREDwCA][CLASSMAX];
  int CA, skipped;
  int skippedClasses[CLASSMAX] = {0};
  int TotalIninf = 0;
  EPFrameAttrib *fattr;
  ClassAttribContent *cattresc, *cattr;
  int *lenbit; 
  int reordered[9] = {0};
  
  /* calculate number of predefinition sets after conversion */
  for(i=0; i<origepcfg->NPred; i++)
  {
    modnpred += (1 << CAbits[i]);
  }

  /* save values for new pred set */
  modepcfg->NPred = modnpred ;
  modepcfg->Interleave  = origepcfg->Interleave;
  modepcfg->BitStuffing = origepcfg->BitStuffing;
  modepcfg->NConcat     = origepcfg->NConcat;   

  if (NULL == (fattr = (EPFrameAttrib *)calloc (modnpred,sizeof (EPFrameAttrib)))) 
      fprintf(stderr, "EP_Tool: PredSetConvert(): outinfo.c: Unable to calloc for EPFrameAttrib\n");

  modepcfg->fattr = fattr;

  modnpred = 0;
  for(i=0; i<origepcfg->NPred; i++)
  {
    int ptr;
    ptr = 0;
    for(j=0; j< origepcfg->fattr[i].ClassCount; j++)
    {
      if(CAtbl[i][j] == 1)  /* CA switch */
      { 
        CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
        ptr++;
      }
    }

    for(j=0; j< (1<<CAbits[i]); j++) /* #modepred for this origpred */ 
    {  
      int output_cfg = modnpred; /* kaehleof 2004-07-27: was 'j' before, does not work for more than one pred set then */
      CA = skipped = TotalIninf = 0; 

      if (origepcfg->BitStuffing)
        TotalIninf += 3;
            
      for(k=0; k<CAbits[i]; k++)       /* CAbits[i] : #Classes w/ CA */
      {
        if (j & (0x01 << k))
        {
          CA |= (0x01 << CAptr[i][k]);
          skipped++;
        }
      }
      
      nclass = origepcfg->fattr[i].ClassCount - skipped ;
      modepcfg->fattr[output_cfg].ClassCount = nclass ;

      if(NULL == (cattr    = (ClassAttribContent *)calloc (nclass, sizeof (ClassAttribContent))))  
         fprintf (stderr, "EP_Tool: PredSetConvert(): outinfo.c: Unable to calloc for ClassAttribContent");
      if(NULL == (cattresc = (ClassAttribContent *)calloc (nclass, sizeof (ClassAttribContent))))  
         fprintf (stderr, "EP_Tool: PredSetConvert(): outinfo.c: Unable to calloc for ClassAttribContent");
      if(NULL == (lenbit   = (int *)calloc (nclass, sizeof (int))))
         fprintf (stderr, "EP_Tool: PredSetConvert(): outinfo.c: Unable to calloc enough memory");

      origPredForCAPred[output_cfg] = i;
      modepcfg->fattr[output_cfg].cattresc = cattresc; 
      modepcfg->fattr[output_cfg].cattr = cattr; 
      modepcfg->fattr[output_cfg].lenbit = lenbit; 
      cpNdx = offset = 0 ;

      for(class_cnt=0; class_cnt<origepcfg->fattr[i].ClassCount; class_cnt++)
      {
        if(CA & (0x01<<class_cnt)) 
        {
          for (index= 0; index < origepcfg->fattr[i].ClassCount; index++)
          {
            if (origepcfg->fattr[i].ClassOutputOrder[index] == class_cnt)
              reordered[index] = 1;
          }
          continue;
        }
      }
      
      if ( origepcfg->fattr[i].ClassReorderedOutput ) {
        /* class number adjustment */
        short unsigned int classIndex;
        short unsigned int validClasses = 0;
        int ClassOutputOrderTmp[CLASSMAX];
        int class_cnt_tmp;

        memcpy ( ClassOutputOrderTmp, origepcfg->fattr[i].ClassOutputOrder,sizeof(int)*CLASSMAX );
        for ( class_cnt = 0; class_cnt < origepcfg->fattr[i].ClassCount; class_cnt++ ) {
          if ( reordered[class_cnt] != 1 ) {
            validClasses++;
          }
          else {
            ClassOutputOrderTmp[class_cnt] = -1;
          }
        }
        for ( classIndex = 0; classIndex < validClasses; classIndex++ ) {
          short unsigned int available = 0;
          for ( class_cnt = 0; class_cnt < origepcfg->fattr[i].ClassCount; class_cnt++ ) {
            if ( ClassOutputOrderTmp[class_cnt] == classIndex ) {
              available = 1;
            }
          }
          while ( !available ) {
            for ( class_cnt = 0; class_cnt < origepcfg->fattr[i].ClassCount; class_cnt++ ) {
              if ( ClassOutputOrderTmp[class_cnt] > classIndex ) {
                ClassOutputOrderTmp[class_cnt]--;
              }
            }
            available = 0;
            for ( class_cnt = 0; class_cnt < origepcfg->fattr[i].ClassCount; class_cnt++ ) {
              if ( ClassOutputOrderTmp[class_cnt] == classIndex ) {
                available = 1;
              }
            }
          }
        }
        for ( class_cnt = 0, class_cnt_tmp = 0; class_cnt < origepcfg->fattr[i].ClassCount; class_cnt++ ) {
          if ( reordered[class_cnt] != 1 ) {
            modepcfg->fattr[output_cfg].ClassOutputOrder[class_cnt_tmp]= ClassOutputOrderTmp[class_cnt];
            class_cnt_tmp++;
          }
        }
        for (index= 0; index < origepcfg->fattr[i].ClassCount; index++) {
        reordered[index] = 0;
        }
      }
      
      /* fix fec_type if last class of concatenated SRS set is optional */
      {
        int nIdx=0;
        for(class_cnt=0; class_cnt<origepcfg->fattr[i].ClassCount; class_cnt++) {

          if(origepcfg->fattr[i].ClassReorderedOutput) {
            nIdx = fattr->ClassOutputOrder[class_cnt]; 
          }
          else {
            nIdx = class_cnt;
          }   
                 
          if(CA & (0x01<<nIdx)) {
            /* if class with fec_Type 1 is skipped */
            if(origepcfg->fattr[i].cattr[nIdx].FECType==1) {              
              
              /* if fec_type of previous class was 2, set it to 1 !!! */
              if(class_cnt>0) {
              /* get index of previous class */
                if(origepcfg->fattr[i].ClassReorderedOutput) {
                  nIdx=origepcfg->fattr[i].ClassOutputOrder[class_cnt-1];
                }
                else {
                  nIdx = class_cnt-1;
                }   
                if(origepcfg->fattr[i].cattr[nIdx].FECType==2) {
                  origepcfg->fattr[i].cattr[nIdx].FECType=21; /* use 21 as temporary flag for a fec_type 2,1 sequence!!! */
                }
              }           
            }          
          }
        }
      }

      for(class_cnt=0; class_cnt<origepcfg->fattr[i].ClassCount; class_cnt++)
      {
        if(CA & (0x01<<class_cnt)) 
        {
          continue;
        }

        if ( origepcfg->fattr[i].ClassReorderedOutput )
          k = class_cnt;
        
        else modepcfg->fattr[output_cfg].ClassOutputOrder[cpNdx] = k = class_cnt;
      
        modepcfg->fattr[output_cfg].cattresc[cpNdx].ClassBitCount = origepcfg->fattr[i].cattresc[k].ClassBitCount ;
        modepcfg->fattr[output_cfg].cattresc[cpNdx].ClassCodeRate = origepcfg->fattr[i].cattresc[k].ClassCodeRate ;
        modepcfg->fattr[output_cfg].cattresc[cpNdx].ClassCRCCount = origepcfg->fattr[i].cattresc[k].ClassCRCCount ;
        modepcfg->fattr[output_cfg].cattr[cpNdx].Concatenate      = origepcfg->fattr[i].cattr[k].Concatenate ;

        /* fec_type==21 means that the following class with fec_type==1 is skipped */
        if(origepcfg->fattr[i].cattr[k].FECType== 21) {
          origepcfg->fattr[i].cattr[k].FECType = 2;
          modepcfg->fattr[output_cfg].cattr[cpNdx].FECType = 1;  
        }
        else {
          modepcfg->fattr[output_cfg].cattr[cpNdx].FECType         = origepcfg->fattr[i].cattr[k].FECType ; 
        }

        modepcfg->fattr[output_cfg].cattr[cpNdx].TermSW           = origepcfg->fattr[i].cattr[k].TermSW ;

        if (origepcfg->Interleave > 1 )
          modepcfg->fattr[output_cfg].cattr[cpNdx].ClassInterleave = origepcfg->fattr[i].cattr[k].ClassInterleave ;
        if (modepcfg->Interleave == 0 ) 
          modepcfg->fattr[output_cfg].cattr[cpNdx].ClassInterleave = 0; 
        if (modepcfg->Interleave == 1 ) 
          modepcfg->fattr[output_cfg].cattr[cpNdx].ClassInterleave = (cpNdx != modepcfg->fattr[output_cfg].ClassCount-1 ? 2 : 1); 
      

        if(origepcfg->fattr[i].cattresc[k].ClassBitCount)  /* LEN Escape? */
        {
          modepcfg->fattr[output_cfg].lenbit[cpNdx] = origepcfg->fattr[i].lenbit[k] ;
          TotalIninf += origepcfg->fattr[i].lenbit[k] ;
        }
        else
          modepcfg->fattr[output_cfg].cattr[cpNdx].ClassBitCount = origepcfg->fattr[i].cattr[k].ClassBitCount ;

         if (origepcfg->fattr[i].cattresc[cpNdx].ClassCodeRate) 
         { 
           TotalIninf += 3 ;   
         } 
         else 
         { 
           modepcfg->fattr[output_cfg].cattr[cpNdx].ClassCodeRate = origepcfg->fattr[i].cattr[k].ClassCodeRate ; 
         } 

         if (origepcfg->fattr[i].cattresc[cpNdx].ClassCRCCount) 
         {
                 TotalIninf += 3 ; 
         } 
         else 
         { 
           modepcfg->fattr[output_cfg].cattr[cpNdx].ClassCRCCount = origepcfg->fattr[i].cattr[k].ClassCRCCount ; 
         } 

        cpNdx++;
      }
      
      if (cpNdx != nclass){
        fprintf (stderr, "EP_Tool: PredSetConvert(): outinfo.c: Assert: cpNdx != nclass");
        exit(EXIT_FAILURE);}
      
      modepcfg->fattr[output_cfg].TotalIninf = TotalIninf; 
      origepcfg->fattr[i].TotalIninf = TotalIninf;
      modepcfg->fattr[output_cfg].ClassReorderedOutput = origepcfg->fattr[i].ClassReorderedOutput; 

      for (index= 0; index < origepcfg->fattr[i].ClassCount; index++)
      {
         skippedClasses[index] = 0;
      }

      if ( reorderFlag && modepcfg->fattr[output_cfg].ClassReorderedOutput ){
        /* new reordering is done different, keep the pred-set class order! */
        /*ReorderOutputClasses(&modepcfg->fattr[output_cfg], reorderFlag);*/
      }

      offset = 0;

      modnpred ++;

      if(modnpred > MAXPREDwCA){
        fprintf (stderr, "EP_Tool: PredSetConvert(): outinfo.c: Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h\n");
        exit(1);
      }
    }
  }

  modepcfg->HeaderProtectExtend = origepcfg->HeaderProtectExtend ;

  if(origepcfg->HeaderProtectExtend)
  {
    modepcfg->HeaderRate = origepcfg->HeaderRate ;
    modepcfg->HeaderCRC = origepcfg->HeaderCRC ;
  }

#if RS_FEC_CAPABILITY
  modepcfg->RSFecCapability = origepcfg->RSFecCapability ;
#endif
}


#ifdef INTEGRATED
void NullSideInfoWriteMod(FILE *outfp, EPConfig origepcfg, EPConfig modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int modpred, int modcheck[CLASSMAX])
{

  int i,j,k;
  int revPred[MAXPREDwCA];
  int revCA[MAXPREDwCA];
  int origcheck[CLASSMAX];
  int CA, skipped;
  int modnpred, origpred;
  EPFrameAttrib *fattr;
  int nclass;
  int CAptr[MAXPREDwCA][CLASSMAX];
  int order[CLASSMAX] = {0};


  /* Count the number of Pred-set after conversion */
  modnpred = 0;
  for(i=0; i<origepcfg.NPred; i++){
    int ptr;
    ptr = 0;
    for(j=0; j< origepcfg.fattr[i].ClassCount; j++){
      if(CAtbl[i][j] == 1){ /* CA switch */
        CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
        ptr++;
      }
    }
    for(j=0; j< (1<<CAbits[i]); j++){  /* #modepred for this origpred */ 
      CA = 0;
      for(k=0; k<CAbits[i]; k++)
        if(j & (0x01 << k))
          CA |= (0x01 << CAptr[i][k]);
      revPred[modnpred] = i;
      revCA[modnpred] = CA;
      modnpred ++;

      if(modnpred > MAXPREDwCA){
        fprintf(stderr, "EP_Tool: NullSideInfoWriteMod(): outinfo.c: Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h\n") ;
        exit(1);
      }
    }
  }

  origpred = revPred[modpred];
  CA = revCA[modpred];
  skipped = 0;

  if (modepcfg.fattr[modpred].ClassReorderedOutput)
    {
      for (i=0; i<modepcfg.fattr[modpred].ClassCount; i++)
        {
          for (j = 0; j <modepcfg.fattr[modpred].ClassCount; j++)
            { 
              if (modepcfg.fattr[modpred].ClassOutputOrder[j] == modepcfg.fattr[modpred].SortedClassOutputOrder[i])
                order[i] = j;
            }
          /*fprintf(stderr,"wrote class %d, length %d\n",order[i],fattr->cattr[order[i]].ClassBitCount); */
        }
    }


  for(i=0; i<origepcfg.fattr[origpred].ClassCount; i++){
    /*      printf("pred %d -> %d,  revCA[%d] = %d\n", origpred, modpred, modpred, revCA[modpred]);*/
    if ( origepcfg.fattr[origpred].ClassReorderedOutput ){
        k = order[i-skipped];
      }
      else{ 
        k = i-skipped;
      }
    if(CAtbl[origpred][i] == 1 && (revCA[modpred] & (1<<i))){
      origepcfg.fattr[origpred].cattr[i].ClassBitCount = 0;
      origcheck[i] = 0;
      skipped += 1;
    }else{
      origcheck[i] = modcheck[k];
      origepcfg.fattr[origpred].cattr[i].ClassBitCount =
        modepcfg.fattr[modpred].cattr[k].ClassBitCount;
      origepcfg.fattr[origpred].cattr[i].ClassCodeRate =
        modepcfg.fattr[modpred].cattr[k].ClassCodeRate;
      origepcfg.fattr[origpred].cattr[i].ClassCRCCount =
        modepcfg.fattr[modpred].cattr[k].ClassCRCCount;
    }
  }

/* Pred Signaling */
  if(origepcfg.NPred > 1){
    fprintf(outfp, "%d\n", origpred);
  }
  fattr = &origepcfg.fattr[origpred];
  nclass = fattr->ClassCount;
  for(i=0; i<nclass; i++)
    fprintf(outfp, "%d ", 1);
  fprintf(outfp, "\n");
  for(i=0; i<nclass; i++){
    if(fattr->cattresc[i].ClassBitCount){ /* Class Length ESC */
      fprintf(outfp, "%d\n", 0);
    }
    if(fattr->cattresc[i].ClassCodeRate){ /* SRCPC rate ESC */
      fprintf(outfp, "%d\n", fattr->cattr[i].ClassCodeRate);
    }
    if(fattr->cattresc[i].ClassCRCCount){ /* Class CRC ESC */
      switch(fattr->cattr[i].ClassCRCCount){
      case 24:
        fprintf(outfp, "%d\n", 17);
        break;
      case 32:
        fprintf(outfp, "%d\n", 18);
        break;
      default:
        fprintf(outfp, "%d\n", fattr->cattr[i].ClassCRCCount);
        break;
      }
    }
  }  
}
#endif

#ifdef INTEGRATED
void SideInfoWriteMod(FILE *outfp, EPConfig origepcfg, EPConfig modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int modpred, int modcheck[CLASSMAX])
{
  int i,j, k;
  int revPred[MAXPREDwCA];
  int revCA[MAXPREDwCA];
  int origcheck[CLASSMAX];
  int CA, skipped;
  int modnpred, origpred;
  int CAptr[MAXPREDwCA][CLASSMAX];
  int order[CLASSMAX] = {0};
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];
    
  /* Count the number of Pred-set after conversion */
  modnpred = 0;
  for(i=0; i<origepcfg.NPred; i++){
    int ptr;
    ptr = 0;
    for(j=0; j< origepcfg.fattr[i].ClassCount; j++){
      if(CAtbl[i][j] == 1){ /* CA switch */
        CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
        ptr++;
      }
    }

    for(j=0; j< (1<<CAbits[i]); j++){  /* #modepred for this origpred */ 
      CA = 0;
      for(k=0; k<CAbits[i]; k++)
        if(j & (0x01 << k))
          CA |= (0x01 << CAptr[i][k]);
      revPred[modnpred] = i;
      revCA[modnpred] = CA;
      modnpred ++;

      if(modnpred > MAXPREDwCA){
        fprintf(stderr, "EP_Tool: SideInfoWriteMod(): outinfo.c: Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h") ;
        exit(1);
      }
    }
  }

  origpred = revPred[modpred];
  CA = revCA[modpred];
  skipped = 0;

  if (modepcfg.fattr[modpred].ClassReorderedOutput)
    {
      for (i=0; i<modepcfg.fattr[modpred].ClassCount; i++)
        {
          for (j = 0; j <modepcfg.fattr[modpred].ClassCount; j++)
            { 
              if (modepcfg.fattr[modpred].ClassOutputOrder[j] == modepcfg.fattr[modpred].SortedClassOutputOrder[i])
                order[i] = j;
            }
          /*fprintf(stderr,"wrote class %d, length %d\n",order[i],fattr->cattr[order[i]].ClassBitCount); */
        }
    }

    for(i=0; i<origepcfg.fattr[origpred].ClassCount; i++){
    /*      printf("pred %d -> %d,  revCA[%d] = %d\n", origpred, modpred, modpred, revCA[modpred]);*/ 
      if ( origepcfg.fattr[origpred].ClassReorderedOutput ){
        k = order[i-skipped];
      }
      else{ 
        k = i-skipped;
      }
      
      if(CAtbl[origpred][i] == 1 && (revCA[modpred] & (1<<i))){  
        origepcfg.fattr[origpred].cattr[i].ClassBitCount = 0;  
        origcheck[i] = 0;  
        skipped += 1;  
      }else{ 
        origcheck[i] = modcheck[k]; 
        origepcfg.fattr[origpred].cattr[i].ClassBitCount = 
          modepcfg.fattr[modpred].cattr[k].ClassBitCount; 
        origepcfg.fattr[origpred].cattr[i].ClassCodeRate = 
          modepcfg.fattr[modpred].cattr[k].ClassCodeRate; 
        origepcfg.fattr[origpred].cattr[i].ClassCRCCount = 
          modepcfg.fattr[modpred].cattr[k].ClassCRCCount;
      }
      
    }
    SideInfoWrite (outfp, &origepcfg, origpred, origcheck, 0, 0, dummyarray2, dummyarray1, 0); 
}
#endif
