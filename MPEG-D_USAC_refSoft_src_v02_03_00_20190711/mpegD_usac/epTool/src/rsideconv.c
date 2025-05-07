/* 
 *  Sideinfo convertor as an alternative of class_available Func.
 *    1999 March 3   Toshiro Kawahara, NTT DoCoMo
 */

#include <stdio.h>
#include <stdlib.h>
#include "eptool.h"

int main(int argc, char **argv)
{
  int i,j,k;
  EPConfig origepcfg, modepcfg;
  FILE *forigpred, *foriginf, *fmodpred, *fmodinf;
  int CAtbl[MAXPREDwCA][CLASSMAX], CAbits[MAXPREDwCA];
  int revPred[MAXPREDwCA];
  int revCA[MAXPREDwCA];
  int CAptr[MAXPREDwCA][CLASSMAX];
  int origpred,modpred;
  int origcheck[CLASSMAX], modcheck[CLASSMAX];
  int CA, skipped;
  int modnpred;
  char lenstr[1024];
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];

  if(argc != 5){
    fprintf(stderr, "Usage: %s <orig_pred> <mod_pred> <ep_output_info(R)> <re-converted_info(W)>\n", argv[0]);
    exit(1);
  }

  if(NULL == (forigpred = fopen(argv[1], "r"))){
    perror(argv[1]); exit(1);
  }
  if(NULL == (fmodpred = fopen(argv[2], "r"))){
    perror(argv[2]); exit(1);
  }
  if(NULL == (foriginf = fopen(argv[3], "r"))){
    perror(argv[3]); exit(1);
  }
  if(NULL == (fmodinf = fopen(argv[4], "w"))){
    perror(argv[4]); exit(1);
  }
  
  /* Read Pre-definition file with ClassAvailable switch */
  OutInfoParse(forigpred, &origepcfg, 0, CAtbl, CAbits, 1);

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

      /*      printf("%d %d %d\n",modnpred, i, CA); */
      revPred[modnpred] = i;
      revCA[modnpred] = CA;
      modnpred ++;

      if(modnpred > MAXPREDwCA){
        fprintf(stderr, "Error: number of predifined sets is to high, please increase MAXPREDwCA in eptool.h") ;
        exit(1);
      }
    }
  }

  /* Read modified Pre-definition file without ClassAvailable switch */
  OutInfoParse(fmodpred, &modepcfg, 0, dummyarray2, dummyarray1, 0);

  /* Convert Side Information */
  
  while(NULL != fgets(lenstr, sizeof(lenstr), foriginf)){
    SideInfoParse(foriginf, &modepcfg, &modpred, modcheck, 1);
    origpred = revPred[modpred];
    CA = revCA[modpred];
    skipped = 0;
    
    for(i=0; i<origepcfg.fattr[origpred].ClassCount; i++){
/*      printf("pred %d -> %d,  revCA[%d] = %d\n", origpred, modpred, modpred, revCA[modpred]);*/
      if(CAtbl[origpred][i] == 1 && (revCA[modpred] & (1<<i))){
        origepcfg.fattr[origpred].cattr[i].ClassBitCount = 0;
        origcheck[i] = 0;
        skipped += 1;
      }else{
        origcheck[i] = modcheck[i-skipped];
        origepcfg.fattr[origpred].cattr[i].ClassBitCount =
          modepcfg.fattr[modpred].cattr[i-skipped].ClassBitCount;
        origepcfg.fattr[origpred].cattr[i].ClassCodeRate =
          modepcfg.fattr[modpred].cattr[i-skipped].ClassCodeRate;
        origepcfg.fattr[origpred].cattr[i].ClassCRCCount =
          modepcfg.fattr[modpred].cattr[i-skipped].ClassCRCCount;
      }
    }
    fputs(lenstr, fmodinf);
    SideInfoWrite(fmodinf, &origepcfg, origpred, origcheck, 0, 0, dummyarray2, dummyarray1, 0); 
  }
  printf("re-modified <side info file> is saved as '%s'\n", argv[4]);
  printf("Please use this file for audio decoder.\n");
  return ( 0 );
}
