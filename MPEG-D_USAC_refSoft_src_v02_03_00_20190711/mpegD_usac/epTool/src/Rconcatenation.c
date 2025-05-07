/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT DoCoMo, Inc. 
in the course of development of the
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
copies or derivative works." Copyright(c)2000.
 *                                                                           *
 ****************************************************************************/


/* 
 *  post-processing EPdec for conncatenation function
 *    2000, June 1  NTT DoCoMo Sanae Hotani
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eptool.h"
#include "dcm_io.h"


int main(int argc, char **argv)
{
  int i,j,k,l;
  EPConfig origepcfg, modepcfg;
  FILE *forigpred = NULL;
  FILE *fdecinf = NULL;
  FILE *fdecbit= NULL;
  FILE *fresortbit = NULL;
  FILE *fmoddecinf = NULL;
  FILE *fmodpred = NULL;
  int origpred;
  char lenstr[1024];
  int orglen,modlen;

  int numclass;
  int classlen[CLASSMAX];

  int tmp,tmp2;
  int orgindata[MAXDATA*MAXCONCAT];
  int modindata[MAXDATA*MAXCONCAT];
  int framelen;
  int ie;

  int check[MAXCONCAT*CLASSMAX];
  int modcheck[MAXCONCAT][CLASSMAX];
  int dummyarray1[MAXPREDwCA]={0};
  int dummyarray2[MAXPREDwCA][CLASSMAX];

  /* usage */
  if (argc!= 6){
    fprintf(stderr, "Usage: %s <org_pred> <dec_side> <dec_bit> <resort_bit> <mod_dec_side>\n", argv[0]);
    exit(1);
  }

  /* file open for input file */

  if(NULL == (forigpred = fopen(argv[1], "r"))){
    perror(argv[1]); exit(1);
  }
  if(NULL == (fdecinf = fopen(argv[2], "r"))){
    perror(argv[2]); exit(1);
  }
  if(NULL == (fdecbit = fopen(argv[3], "r"))){
    perror(argv[3]); exit(1);
  }
  

  OutInfoParse(forigpred, &origepcfg, 0, dummyarray2, dummyarray1, 0);
  fprintf(stderr,"NConcat=%i\n",origepcfg.NConcat);
  if(origepcfg.NConcat>1){
    fprintf(stderr,"resort bitstream processing...\n");

    /* file open for output file */
    if(NULL == (fresortbit = fopen(argv[4], "w"))){
      perror(argv[4]); exit(1);
    }

    if(NULL == (fmoddecinf = fopen(argv[5], "w"))){
      perror(argv[5]); exit(1);
    }

    if(NULL == (fmodpred = fopen("tmp_mod_pred", "w"))){
      perror("tmp_mod_pred"); exit(1);
    }

    /*                      */
    /* making mod_pred_file */
    /*                      */

    fprintf(fmodpred,"%d\t/* NPred */\n",origepcfg.NPred);
    fprintf(fmodpred,"%d\t/* Interleave  */\n",origepcfg.Interleave);
    fprintf(fmodpred,"%d\t/* Bit Stuffing  */\n",origepcfg.BitStuffing);
    fprintf(fmodpred,"%d\t/* Number of Concatenated Frames  */\n",1);

    for(i=0;i<origepcfg.NPred;i++){
      numclass=0;

      for(j=0;j<origepcfg.fattr[i].ClassCount;j++){
	if(origepcfg.fattr[i].cattr[j].Concatenate==1){

	  classlen[numclass]=origepcfg.fattr[i].cattr[j].ClassBitCount * origepcfg.NConcat;
	  numclass+=1;

	}else if(origepcfg.fattr[i].cattr[j].Concatenate==0){

	  for(k=0;k<origepcfg.NConcat;k++){
	    classlen[numclass+k]=origepcfg.fattr[i].cattr[j].ClassBitCount;
	  }
	  numclass+=origepcfg.NConcat;

	} else {
	  fprintf(stderr,"Warning: concatenation_flag error 0 or 1!!\n");
	}
      }
      fprintf(fmodpred,"%d\t/* Number of class */\n",numclass);

      tmp=0;
      for(j=0;j<origepcfg.fattr[i].ClassCount;j++){

	if(origepcfg.fattr[i].cattr[j].Concatenate==1){
	  fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassBitCount);
	  fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassCodeRate);
	  fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassCRCCount);

	  if(origepcfg.Interleave < 2 && origepcfg.fattr[i].cattr[j].FECType != 0 ){
	    fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype */\n",origepcfg.fattr[i].cattr[j].FECType);
	  } else if (origepcfg.Interleave < 2 && origepcfg.fattr[i].cattr[j].FECType == 0){
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
            fprintf(fmodpred,"%d \t/* escape_len, escape_srcpc, escape_crc, fectype termination */\n",origepcfg.fattr[i].cattr[j].TermSW);
	  }else if(origepcfg.Interleave == 2 && origepcfg.fattr[i].cattr[j].FECType != 0){
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
	    fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype, interleave SW */\n",origepcfg.fattr[i].cattr[j].ClassInterleave);
	  } else if(origepcfg.Interleave == 2 && origepcfg.fattr[i].cattr[j].FECType == 0){
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].TermSW);
	    fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype, termination, interleave SW */\n",origepcfg.fattr[i].cattr[j].ClassInterleave);

	  }


	  fprintf(fmodpred,"%d\t/* class number */\n",classlen[tmp]);
	  tmp++;
	  fprintf(fmodpred,"%d\t/* SRCPC rate */\n",origepcfg.fattr[i].cattr[j].ClassCodeRate);
	  fprintf(fmodpred,"%d\t/* CRC length */\n",origepcfg.fattr[i].cattr[j].ClassCRCCount);

	}else if(origepcfg.fattr[i].cattr[j].Concatenate==0){

	  for(k=0;k<origepcfg.NConcat;k++){
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassBitCount);
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassCodeRate);
	    fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattresc[j].ClassCRCCount);
	    if(origepcfg.Interleave < 2 && origepcfg.fattr[i].cattr[j].FECType != 0 ){
	      fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype */\n",origepcfg.fattr[i].cattr[j].FECType);
	    } else if (origepcfg.Interleave < 2 && origepcfg.fattr[i].cattr[j].FECType == 0){
	      fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
	      fprintf(fmodpred,"%d \t/* escape_len, escape_srcpc, escape_crc, fectype termination */\n",origepcfg.fattr[i].cattr[j].TermSW);
	    }else if(origepcfg.Interleave == 2 && origepcfg.fattr[i].cattr[j].FECType != 0){
	      fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
	      fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype, interleave SW */\n",origepcfg.fattr[i].cattr[j].ClassInterleave);
	    } else if(origepcfg.Interleave == 2 && origepcfg.fattr[i].cattr[j].FECType == 0){
	      fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].FECType);
	      fprintf(fmodpred,"%d ",origepcfg.fattr[i].cattr[j].TermSW);
	      fprintf(fmodpred,"%d\t/* escape_len, escape_srcpc, escape_crc, fectype, termination, interleave SW */\n",origepcfg.fattr[i].cattr[j].ClassInterleave);
	      
	    }

	    fprintf(fmodpred,"%d\t/* class number */\n",classlen[tmp]);
	    tmp++;
	    fprintf(fmodpred,"%d\t/* SRCPC rate */\n",origepcfg.fattr[i].cattr[j].ClassCodeRate);
	    fprintf(fmodpred,"%d\t/* CRC length */\n",origepcfg.fattr[i].cattr[j].ClassCRCCount);
	  }
	}

      }
      /* Class reordered output */
      fprintf(fmodpred,"%d\t/* class reordered output SW */\n",origepcfg.fattr[i].ClassReorderedOutput);
      if(origepcfg.fattr[i].ClassReorderedOutput==1){
	for(j=0;j<origepcfg.fattr[i].ClassCount;j++){
	  fprintf(fmodpred,"%d",origepcfg.fattr[i].ClassOutputOrder[j]);
	}
	fprintf(fmodpred,"\n");
      }

    }

    /* Header Protection Extention */
    fprintf(fmodpred,"%d\t/* header extension */\n",origepcfg.HeaderProtectExtend);
    if(origepcfg.HeaderProtectExtend == 1){
      fprintf(fmodpred,"%d\t/* header rate */\n", origepcfg.HeaderRate);
      fprintf(fmodpred,"%d\t/* header CRC */\n", origepcfg.HeaderCRC);
    }
#if RS_FEC_CAPABILITY
    /* RS FEC Capability */
    fprintf(fmodpred,"%d\t/* RS FEC Capability */\n",origepcfg.RSFecCapability);
#endif

    fclose(fmodpred);

    /*                         */
    /* resorting for bit file  */
    /*                         */

    if(NULL == (fmodpred = fopen("tmp_mod_pred", "r"))){
      perror("tmp_mod_pred"); exit(1);
    }

    OutInfoParse(fmodpred, &modepcfg, 0, dummyarray2, dummyarray1, 0);

    while(NULL != fgets(lenstr,sizeof(lenstr),fdecinf) ){
      SideInfoParse(fdecinf, &modepcfg, &origpred, check, 1);
      
      framelen=0;
      for(j=0;j<origepcfg.fattr[origpred].ClassCount;j++){
	framelen+=origepcfg.fattr[origpred].cattr[j].ClassBitCount;
      }

      /* sideinfo */
      sscanf(lenstr,"%d", &modlen);


      /* bitstream */
      ie=readbits(fdecbit, modlen, orgindata);

      /*      if(modlen< framelen*origepcfg.NConcat){
	fprintf(stderr,"last frame..., padding by 0 data\n");
	for(j=0;j<(framelen*origepcfg.NConcat-modlen);j++){
	  orgindata[j+modlen]=0;
	}
	} */

      tmp=0;
      tmp2=0;
      for(j=0;j<origepcfg.fattr[origpred].ClassCount;j++){
	for(k=0;k<origepcfg.NConcat;k++){
	  for(l=0; (unsigned int) l<origepcfg.fattr[origpred].cattr[j].ClassBitCount; l++){
	    modindata[l+ k*framelen+tmp2]=orgindata[tmp];
	    tmp++;
	  }
	}
	tmp2+=origepcfg.fattr[origpred].cattr[j].ClassBitCount;
      }

      if(modlen>0)
	writebits(fresortbit,framelen*origepcfg.NConcat, modindata);

      /*                              */
      /*   modified for the side info */
      /*                              */

      orglen = modlen/origepcfg.NConcat;

      tmp=0;
      for(j=0;j<origepcfg.fattr[origpred].ClassCount;j++){
	if(origepcfg.fattr[origpred].cattr[j].Concatenate == 1){
	  for(i=0;i<origepcfg.NConcat;i++){
	    modcheck[i][j] = check[tmp];
	  }
	  tmp++;
	}else{
	  for(i=0;i<origepcfg.NConcat;i++){
	    modcheck[i][j] = check[tmp];
	    tmp++;
	  }
	}
      }


      if(orglen>0){
	for(i=0;i<origepcfg.NConcat;i++){
	  fprintf(fmoddecinf,"%d\n",orglen);
	  SideInfoWrite(fmoddecinf, &origepcfg, origpred, modcheck[i], 0, 0, dummyarray2, dummyarray1, 0); 
	}
      }else{
	for(i=0;i<origepcfg.NConcat;i++){
	  fprintf(fmoddecinf,"%d\n",orglen);
	  NullSideInfoWrite(fmoddecinf,&origepcfg,origpred, 0, 0, dummyarray2, dummyarray1, 0);
	}
      }

    }

    /* file close for output file */
    fclose(fresortbit);
    fclose(fmoddecinf);
      
  }else{
    fprintf(stderr,"Not concatenation...so, please use original files from epdec \n");
  }

  fclose(forigpred);
  fclose(fdecinf);
  fclose(fdecbit);

  return 0;
  
}
