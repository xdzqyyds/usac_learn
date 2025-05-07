#include <stdio.h>
#include <stdlib.h>
#include "libeptool.h"
#include "sideinfoparse.h"
#include "dcm_io.h"

#define MAXDATA 4096*7

void WriteDecoderInfoFile(HANDLE_CLASS_FRAME outFrame, FILE *epDecInfo);
void BytesToBits(unsigned char *inbuf, int *trans, int translen);
int BitsToBytes(unsigned char *outbuf, int *trans, int translen );


int main(int argc, char **argv)
{
  HANDLE_EP_CONFIG hEpConfigDec = NULL;
  HANDLE_CLASS_FRAME outFrame = NULL;

  unsigned char **configBuffer = NULL;
  long          *configLength = NULL;
  unsigned char inBuffer[MAXDATA*2] = {0}; 
  int decOutBufferTmp[MAXDATA] = {0}, decOutBuffer[MAXDATA]={0};
  char buf[255];
  int nFrames = 0;
  int i, offsetDec; 
  int bitFlag = 1;
  FILE *inStream = 0L, *outStream = 0L, *inInfo = 0L, *outInfo = 0L;
  offsetDec = 0;

  if (argc < 6) {
    fprintf(stderr,"epdec: not enough input arguments\n");
    fprintf(stderr,"usage: %s <pred_file> <in_stream> <in_info> <out_stream> <out_info>\n",argv[0]);
    return 1;
  }

  if(NULL == (inStream = fopen(argv[2], "rb"))){
    fprintf(stderr,"epdec: file %s not found\n", argv[2]);
    exit(-1);
  }
  
  if(NULL == (inInfo = fopen(argv[3], "r"))){
    fprintf(stderr,"epdec: file %s not found\n", argv[3]);
    exit(-1);
  }
  
  if(NULL == (outStream = fopen(argv[4], "wb"))){
    fprintf(stderr,"epdec: could not create file %s\n", argv[4]);
    exit(-1);
  }

  if(NULL == (outInfo = fopen(argv[5], "w"))){
    fprintf(stderr,"epdec: could not create file %s\n", argv[5]);
    exit(-1);
  }


  if(NULL == (configBuffer = (unsigned char **)malloc(sizeof(char *)))){
    fprintf(stderr, "epdec: Failed malloc for configBuffer");
    exit(-1);
  }
  
  if(NULL == (configLength = (long int *)malloc(sizeof(long int)))){
    fprintf(stderr, "epdec: Failed malloc for configLength");
    free(configBuffer);
    exit(-1);
  }


  EPTool_CreateBufferEPSpecificConfig( configBuffer, argv[1] );

  if (EPTool_CreateExpl(&(hEpConfigDec), argv[1], configBuffer, configLength, 1, bitFlag)) {
    fprintf(stderr,"EPTool: EPTool_CreateExpl(): epdec.c: Error: could not create decoder EP tool handle\n");
    free(configBuffer);
    free(configLength);
    exit(-1);
  }

  /* time to write configBuffer ... */

  EPTool_DestroyBufferEPSpecificConfig(configBuffer);

  outFrame = EPTool_GetClassFrame(hEpConfigDec, bitFlag);

  while (1) {
    int dlen;

    /* read input data and input info */
    if (NULL == fgets(buf, sizeof(buf), inInfo)){
      fprintf(stderr, "epdec: End of info file reached\n");
      break;
    }

    if (1 != sscanf(buf, "%d", &dlen)) {
      fprintf(stderr, "epdec: Error in in_info file: Data length Error\n");
    }

    if (bitFlag){
      int inBufferTmp[sizeof(inBuffer)];
      readbits(inStream, dlen, inBufferTmp);
      for (i=0; i<dlen; i++){
        inBuffer[i] = (unsigned char) inBufferTmp[i];
      }
    } else {  
      fread(inBuffer, 1,(dlen+7)/8, inStream);
    }

    /* EP Decode one frame */
    EPTool_DecodeFrame(hEpConfigDec, outFrame, inBuffer, dlen, bitFlag);

    if (bitFlag){
      int j;
      for (i=0; i<outFrame->nrValidBuffer; i++){
        for (j=0; j<outFrame->classLength[i]; j++)
          decOutBuffer[offsetDec + j] = outFrame->classBuffer[i][j];
        offsetDec += outFrame->classLength[i];
      }
      writebits(outStream, offsetDec, decOutBuffer);
      offsetDec = 0; 
    } else {
      for (i=0; i<outFrame->nrValidBuffer; i++){  
        BytesToBits (outFrame->classBuffer[i], &decOutBufferTmp[offsetDec], ((outFrame->classLength[i]+7)/8)*8);
        offsetDec += outFrame->classLength[i];
      }
      BitsToBytes ((unsigned char *)decOutBuffer, decOutBufferTmp, (offsetDec/8)*8);
      fwrite (decOutBuffer, 1, offsetDec/8, outStream);
      
      if (offsetDec%8){
        for (i=0;i<offsetDec%8;i++)
          decOutBufferTmp[i] = decOutBufferTmp[(offsetDec/8)*8+i];
        offsetDec = offsetDec%8;
      }
      else offsetDec = 0;
    }

    WriteDecoderInfoFile(outFrame, outInfo);
    
    fprintf(stderr, "\r[%d]", nFrames++);
  }

  if (!bitFlag){
    if (offsetDec){
      for (i=offsetDec;i<8;decOutBufferTmp[i++]=0);
      BitsToBytes ((unsigned char *)decOutBuffer, decOutBufferTmp, 8);
      fwrite (decOutBuffer, 1, 1, outStream);
    }
  }
  
  if(outFrame)
    free(outFrame);

  if (EPTool_Destroy( hEpConfigDec )) {
    fprintf(stderr,"EPTool: EPTool_Destroy(): epdec.c: Error: could not destroy decoder EP tool handle\n");
    free(configBuffer);
    free(configLength);
    exit(-1);
  }

  if(configBuffer)
    free(configBuffer);
  if(configLength)
    free(configLength);

  if (inInfo)
    fclose(inInfo);
  if (inStream)
    fclose(inStream);
  if (outInfo)
    fclose(outInfo);
  if (outStream)
    fclose(outStream);

  return 0;
}

void WriteDecoderInfoFile(HANDLE_CLASS_FRAME outFrame, FILE *epDecInfo)
{
  int i, totalLength = 0;
  for ( i=0; i< outFrame->nrValidBuffer; i++)
    totalLength += outFrame->classLength[i];

  fprintf(epDecInfo, "%d\n", totalLength);
  for ( i=0; i< outFrame->nrValidBuffer; i++)
    fprintf(epDecInfo, "%d ", outFrame->crcError[i]);
  fprintf(epDecInfo, "\n");
  for ( i=0; i< outFrame->nrValidBuffer; i++)
    fprintf(epDecInfo, "%d\n", (int)outFrame->classLength[i]);
}


void BytesToBits(unsigned char *inbuf, int *trans, int translen)
{
  int i, bitrem = 0;
  unsigned char tmp = 0;

  for(i=0; i< translen; i++){
    if(bitrem == 0){
      tmp = inbuf[i/8] ;
      bitrem = 8;
    }
    trans[i] = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
}

int BitsToBytes(unsigned char *outbuf, int *trans, int translen )
{
  int bitrem = 0;
  unsigned char tmp = 0;
  int i;

  if(translen == -1){		/* Flash */
    if(bitrem == 0) return bitrem;
    tmp <<= (8 - bitrem);
    outbuf[0] = tmp; /* ?????? dh */ 
    /* fwrite(&tmp, sizeof(char), 1, fpout); */
    return bitrem;
  }
  
  for(i=0; i<translen; i++){
    tmp <<= 1;
    tmp |= *(trans++);
    bitrem ++;
    if(bitrem == 8){
      outbuf[i/8] = tmp;
      /* fwrite(&tmp, sizeof(char), 1, fpout); */
      bitrem = 0;
    }
  }
  return bitrem;
}
