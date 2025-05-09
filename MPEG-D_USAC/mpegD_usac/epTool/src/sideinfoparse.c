#include <stdlib.h>
#include "sideinfoparse.h"
#include "eptool.h"

#define FMTERR(A) {fprintf(stderr,"%s\n",(A)); exit(1);}

void MySideInfoParse(char *predFileName, FILE *inInfo, HANDLE_CLASS_FRAME inFrame)
{

  FILE *fp;
  EPConfig epcfg;
  int Npred;
  int i, pred;
  int CAtbl[MAXPREDwCA][CLASSMAX] = {{0}}, CAbits[MAXPREDwCA] = {0};
  int PredCA[MAXPREDwCA][255] = {{0}};

  if ((fp = fopen(predFileName, "r")) == NULL) {
    fprintf(stderr,"predefinition file not found\n");
    return;
  }

  OutInfoParse(fp, &epcfg, 0, CAtbl, CAbits, 1);
  
  SideInfoParse(inInfo, &epcfg, &pred, NULL, 0);

  inFrame->nrValidBuffer = epcfg.fattr[pred].ClassCount;

  for (i=0; i< inFrame->nrValidBuffer; i++) {
    inFrame->classLength[i] = epcfg.fattr[pred].cattr[i].ClassBitCount;
  }

  Npred = epcfg.NPred;

  for(i=0; i<Npred; i++){
    if(epcfg.fattr[i].cattr)
      free(epcfg.fattr[i].cattr);
    if(epcfg.fattr[i].cattresc)
      free(epcfg.fattr[i].cattresc);
    if(epcfg.fattr[i].lenbit)
      free(epcfg.fattr[i].lenbit);
  }
  if(epcfg.fattr)
    free(epcfg.fattr);

  fclose(fp);

}



