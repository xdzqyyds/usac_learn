/* 
 *  Sideinfo convertor as an alternative of class_available Func.
 *    1999 March 3   Toshiro Kawahara, NTT DoCoMo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eptool.h"


int main(int argc, char **argv)
{
  int i,j,k,class_cnt,index;
  int reordered[9] = {0};
  EPConfig origepcfg, modepcfg;
  FILE *forigpred = NULL;
  FILE *foriginf = NULL;
  FILE *fmodpred = NULL;
  FILE *fmodinf = NULL;
  int CAtbl[MAXPREDwCA][CLASSMAX], CAbits[MAXPREDwCA];
  int PredCA[MAXPREDwCA][255];
  int CAptr[MAXPREDwCA][CLASSMAX];
  int origpred,modpred;
  int origcheck[CLASSMAX] = {0}; /* The crc check is not read from the info file 
                                    because it should be zero on encoder site 
                                    anyway. Thus this array has to be intitalized. */
  int modcheck[CLASSMAX];
  int CA, skipped;
  int modnpred;
  char lenstr[1024];
  unsigned short only_pred = 0;
  unsigned short only_info = 0;
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];

  if ( strstr (argv[0], "predconv" ) ) { 
    only_pred = 1; 
  } 
  
  if ( strstr (argv[0], "infoconv" ) ) { 
    only_info = 1; 
  } 
  
  /* sideconv */
  if ( ! ( only_pred || only_info ) ) {
    if(argc != 5){
      fprintf(stderr, "Usage: %s <orig_pred> <mod_pred> <orig_side> <mod_side>\n", argv[0]);
      exit(1);
    }
    if(NULL == (forigpred = fopen(argv[1], "r"))){
      perror(argv[1]); exit(1);
    }
    if(NULL == (fmodpred = fopen(argv[2], "w"))){
      perror(argv[2]); exit(1);
    }
    if(NULL == (foriginf = fopen(argv[3], "r"))){
      perror(argv[3]); exit(1);
    }
    if(NULL == (fmodinf = fopen(argv[4], "w"))){
      perror(argv[4]); exit(1);
    }
  }
  
  /* predconv */
  if ( only_pred ) {
    if(argc != 3){
      fprintf(stderr, "Usage: %s <orig_pred> <mod_pred>\n", argv[0]);
      exit(1);
    }
    if(NULL == (forigpred = fopen(argv[1], "r"))){
      perror(argv[1]); exit(1);
    }
    if(NULL == (fmodpred = fopen(argv[2], "w"))){
      perror(argv[2]); exit(1);
    }
  }

  /* infoconv */
  if ( only_info ) {
    if(argc != 5){
      fprintf(stderr, "Usage: %s <orig_pred> <mod_pred> <orig_side> <mod_side>\n", argv[0]);
      exit(1);
    }
    if(NULL == (forigpred = fopen(argv[1], "r"))){
      perror(argv[1]); exit(1);
    }
    /* all fprintf's will not be processed, because the file is opend with "r" */
    if(NULL == (fmodpred = fopen(argv[2], "r"))){
      perror(argv[2]); exit(1);
    }
    if(NULL == (foriginf = fopen(argv[3], "r"))){
      perror(argv[3]); exit(1);
    }
    if(NULL == (fmodinf = fopen(argv[4], "w"))){
      perror(argv[4]); exit(1);
    }
  }

  /* Read Pre-definition file with ClassAvailable switch */
  OutInfoParse(forigpred, &origepcfg, 0 ,CAtbl, CAbits, 1);

  /* Count the number of Pred-set after conversion */
  modnpred = 0;
  for(i=0; i<origepcfg.NPred; i++){
    modnpred += (1 << CAbits[i]);
    /*    printf("CAbits[%d] = %d\n", i, CAbits[i]);*/
  }
  
  /* Convert Pre-definition sets and output them */
  fprintf(fmodpred, "%d\t\t# #pred\n", modnpred);
  fprintf(fmodpred, "%d\t\t# Interleave?\n", origepcfg.Interleave);
  fprintf(fmodpred, "%d\t\t# BitStuffing?\n", origepcfg.BitStuffing);
  fprintf(fmodpred, "%d\t\t# Concatenate?\n", origepcfg.NConcat);
  
  modnpred = 0;
  for (  i = 0; i <  origepcfg.NPred; i++ ) { /* for each original #pred */
    int ptr;
    ptr = 0;
    for ( j = 0; j < origepcfg.fattr[i].ClassCount; j++ ) {
      if ( CAtbl[i][j] == 1 ) { /* CA switch */
        CAptr[i][ptr] = j; /* CAptr[A][B] :Class for  A-th pred, B-th switch */
        ptr++;
      }
    }
    for ( j = 0; j < (1<<CAbits[i]); j++ ) {  /* #modepred for this origpred */ 
      CA = skipped = 0;
      for ( k = 0; k < CAbits[i]; k++ ) {      /* CAbits[i] : #Classes w/ CA */
        if ( j & (0x01 << k) ) {
          CA |= (0x01 << CAptr[i][k]);
          skipped ++;
        }
      }
      PredCA[i][CA] = modnpred;
      fprintf(fmodpred, "%d\t\t# Classes\n", origepcfg.fattr[i].ClassCount - skipped);
      for ( k = 0; k < origepcfg.fattr[i].ClassCount; k++ ) { /* for each class */
        if ( CA & (0x01<<k) ) {
          for ( index = 0; index < origepcfg.fattr[i].ClassCount; index++ ) {
            if ( origepcfg.fattr[i].ClassOutputOrder[index] == k ) {
              reordered[index] = 1;
            }
          }
          continue;
        }
        /*fprintf (stderr, "CA: %d, skipped: %d, i: %d, j: %d, k: %d\n",CA,skipped,i,j,k); */
        fprintf(fmodpred, "%d %d %d "/*\t\t\t# LEN_ESC, RATE_ESC, CRC_ESC \n"*/,
                origepcfg.fattr[i].cattresc[k].ClassBitCount,
                origepcfg.fattr[i].cattresc[k].ClassCodeRate,
                origepcfg.fattr[i].cattresc[k].ClassCRCCount);
        if ( origepcfg.NConcat != 1) {
          fprintf(fmodpred, "%d "/*, CONCAT\n"*/,
                  origepcfg.fattr[i].cattr[k].Concatenate);
        }
        fprintf(fmodpred, "%d "/*, FECTYPE\n"*/,
                origepcfg.fattr[i].cattr[k].FECType);
        if (origepcfg.fattr[i].cattr[k].FECType == 0) 
          fprintf(fmodpred, "%d ", origepcfg.fattr[i].cattr[k].TermSW); /* (Termination-Switch: only needed for SRCPC) 001004 multrums@fhg */
        if (origepcfg.Interleave > 1 )
          fprintf(fmodpred, "%d",origepcfg.fattr[i].cattr[k].ClassInterleave);
        fprintf(fmodpred, "\t\t# LEN_ESC, RATE_ESC, CRC_ESC");
        if (origepcfg.NConcat != 1) {
          fprintf(fmodpred, ", CONCAT");
        }
        fprintf(fmodpred, ", FECTYPE");
        if (origepcfg.fattr[i].cattr[k].FECType == 0)
          fprintf(fmodpred, ", TERMSW");
        if (origepcfg.Interleave > 1 )
          fprintf(fmodpred, ", INT");
        fprintf(fmodpred, "\n");
        if(origepcfg.fattr[i].cattresc[k].ClassBitCount) /* LEN Escape? */
          fprintf(fmodpred, "%d\t\t\t# bits for length \n",
                  origepcfg.fattr[i].lenbit[k]);
        else
          fprintf(fmodpred, "%d\t\t\t# length \n",
                  origepcfg.fattr[i].cattr[k].ClassBitCount);
        fprintf(fmodpred, "%d\t\t\t# RATE\n",
                origepcfg.fattr[i].cattr[k].ClassCodeRate);
        fprintf(fmodpred, "%d\t\t\t# CRC length in bits\n",
                origepcfg.fattr[i].cattr[k].ClassCRCCount);
      }
      fflush(fmodpred);
      fprintf(fmodpred, "%d\t\t# reordered classes ?\n",origepcfg.fattr[i].ClassReorderedOutput);
      if ( origepcfg.fattr[i].ClassReorderedOutput ) {
        /* class number adjustment */
        short unsigned int classIndex;
        short unsigned int validClasses = 0;
        int ClassOutputOrderTmp[CLASSMAX];

        memcpy ( ClassOutputOrderTmp, origepcfg.fattr[i].ClassOutputOrder,sizeof(int)*CLASSMAX );
        for ( class_cnt = 0; class_cnt < origepcfg.fattr[i].ClassCount; class_cnt++ ) {
          if ( reordered[class_cnt] != 1 ) {
            validClasses++;
          }
          else {
            ClassOutputOrderTmp[class_cnt] = -1;
          }
        }
        for ( classIndex = 0; classIndex < validClasses; classIndex++ ) {
          short unsigned int available = 0;
          for ( class_cnt = 0; class_cnt < origepcfg.fattr[i].ClassCount; class_cnt++ ) {
            if ( ClassOutputOrderTmp[class_cnt] == classIndex ) {
              available = 1;
            }
          }
          while ( !available ) {
            for ( class_cnt = 0; class_cnt < origepcfg.fattr[i].ClassCount; class_cnt++ ) {
              if ( ClassOutputOrderTmp[class_cnt] > classIndex ) {
                ClassOutputOrderTmp[class_cnt]--;
              }
            }
            available = 0;
            for ( class_cnt = 0; class_cnt < origepcfg.fattr[i].ClassCount; class_cnt++ ) {
              if ( ClassOutputOrderTmp[class_cnt] == classIndex ) {
                available = 1;
              }
            }
          }
        }
        for ( class_cnt = 0; class_cnt < origepcfg.fattr[i].ClassCount; class_cnt++ ) {
          if ( reordered[class_cnt] != 1 ) {
            fprintf(fmodpred, "%d ", ClassOutputOrderTmp[class_cnt]);
          }
        }
        /* reordered[class_cnt] = 0; */
        fprintf(fmodpred, "\t\t#  class order\n");
      }

      for (index= 0; index < origepcfg.fattr[i].ClassCount; index++) {
        reordered[index] = 0;
      }
      modnpred ++;

      if(modnpred > MAXPREDwCA){
        fprintf(stderr, "Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h") ;
        exit(1);
      }
    }
  }
  fprintf(fmodpred, "%d\t\t# FEC Header Extension?\n", origepcfg.HeaderProtectExtend);
  if(origepcfg.HeaderProtectExtend){
    fprintf(fmodpred, "%d\t\t# Header SRCPC Rate\n", origepcfg.HeaderRate);
    fprintf(fmodpred, "%d\t\t# Header CRC length\n", origepcfg.HeaderCRC);
  }
#if RS_FEC_CAPABILITY
  fprintf(fmodpred, "%d\t\t# RS FEC Capability (byte)\n", origepcfg.RSFecCapability);
#endif  

  fclose(fmodpred);

  if ( ! only_info ) {
    printf("modified <pred file> with %d pred set is saved as '%s'\n\n",
           modnpred, argv[2]);
  }

  if(NULL == (fmodpred = fopen(argv[2], "r"))){
    perror(argv[2]); exit(1);
  }
  /* Read modified Pre-definition file without ClassAvailable switch */
  OutInfoParse(fmodpred, &modepcfg, 0, dummyarray2, dummyarray1, 0);


  if ( ! only_pred ) {
    /* Convert Side Information */
  
    while(NULL != fgets(lenstr, sizeof(lenstr), foriginf)){
      SideInfoParse(foriginf, &origepcfg, &origpred, NULL, 0);
      /*    printf(".");fflush(stdout);*/
      /* First Pass -> to check CA application */
      CA = 0;
      for(i=0; i<origepcfg.fattr[origpred].ClassCount; i++){
        if(CAtbl[origpred][i] == 1 &&
           origepcfg.fattr[origpred].cattr[i].ClassBitCount == 0)
          CA |= 1 << i;
      }
      modpred = PredCA[origpred][CA];
      /*    printf("modpred = %d\n", modpred);fflush(stdout);*/
      /* Second Pass -> set fattr configuration */
      skipped = 0;
      for(i=0; i<origepcfg.fattr[origpred].ClassCount; i++){
        if(CAtbl[origpred][i] == 1 &&
           origepcfg.fattr[origpred].cattr[i].ClassBitCount == 0)
          skipped += 1;
        else{
          modepcfg.fattr[modpred].cattr[i-skipped].ClassBitCount =
            origepcfg.fattr[origpred].cattr[i].ClassBitCount;
          modepcfg.fattr[modpred].cattr[i-skipped].ClassCodeRate =
            origepcfg.fattr[origpred].cattr[i].ClassCodeRate;
          modepcfg.fattr[modpred].cattr[i-skipped].ClassCRCCount =
            origepcfg.fattr[origpred].cattr[i].ClassCRCCount;
          modcheck[i-skipped] = origcheck[i];
        }
      }
      fputs(lenstr, fmodinf);
      SideInfoWrite(fmodinf, &modepcfg, modpred, modcheck, 0, 0, dummyarray2, dummyarray1, 0);
    }
    fclose(foriginf);
    fclose(fmodinf);
    printf("modified <side info file> is saved as '%s'\n\n", argv[4]);
    /*     printf("Please use these two files as input to 'epenc'\n\n"); */
  }
  return ( 0 );
}
