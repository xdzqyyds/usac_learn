#include <stdio.h>
#include <stdlib.h>
#include "libeptool.h"
#include "sideinfoparse.h"
#include "feedframeclass.h"
#include "dcm_io.h"

#define MAXDATA 4096*7


void BytesToBits(unsigned char *inbuf, int *trans, int translen);
int BitsToBytes(unsigned char *outbuf, int *trans, int translen );

int main(int argc, char **argv)
{
  HANDLE_EP_CONFIG hEpConfigEnc = NULL;
  HANDLE_CLASS_FRAME inFrame = NULL;

  unsigned char **configBuffer = NULL;
  long          *configLength = NULL;
  unsigned char inBuffer[MAXDATA] = {0}, outBuffer[MAXDATA*2] = {0}; 
  int encOutBufferTmp[MAXDATA*2] = {0}, encOutBuffer[MAXDATA*2] = {0};
  char buf[255];
  int outLength = 0, nFrames = 0;
  int i, offsetEnc; 
  int bitFlag = 1;
  int bitrem = 0; /* for not_byte_aligned input: bits of a byte that remain unused */


  FILE *inStream = 0L, *outStream = 0L, *inInfo = 0L, *outInfo = 0L;
  offsetEnc = 0;

  if (argc < 6) {
    fprintf(stderr,"epenc: not enough input arguments\n");
    fprintf(stderr,"usage: %s <pred_file> <in_stream> <in_info> <out_stream> <out_info> <dec_out_stream>\n",argv[0]);
    return 1;
  }

  if(NULL == (inStream = fopen(argv[2], "rb"))){
    fprintf(stderr,"epenc: file %s not found\n", argv[2]);
    exit(-1);
  }
  
  if(NULL == (inInfo = fopen(argv[3], "r"))){
    fprintf(stderr,"epenc: file %s not found\n", argv[3]);
    exit(-1);
  }
  
  if(NULL == (outStream = fopen(argv[4], "wb"))){
    fprintf(stderr,"epenc: could not create file %s\n", argv[4]);
    exit(-1);
  }

  if(NULL == (outInfo = fopen(argv[5], "w"))){
    fprintf(stderr,"could not create file %s\n", argv[5]);
    exit(-1);
  }

  if(NULL == (configBuffer = (unsigned char **)malloc(sizeof(char *)))){
    fprintf(stderr, "epenc: Failed malloc for configBuffer");
    exit(-1);
  }
  
  if(NULL == (configLength = (long int *)malloc(sizeof(long int)))){
    fprintf(stderr, "epenc: Failed malloc for configLength");
    free(configBuffer);
    exit(-1);
  }

  EPTool_CreateBufferEPSpecificConfig(configBuffer, argv[1]);

  if (EPTool_CreateExpl(&(hEpConfigEnc), argv[1], configBuffer, configLength, 0, bitFlag)) {
    fprintf(stderr,"EPTool: EPTool_CreateExpl(): epenc.c Error: Not able to create Encoder EP tool handle\n");
    free(configBuffer);
    free(configLength);
    exit(-1);
  }

  /* time to write configBuffer ... */

  EPTool_DestroyBufferEPSpecificConfig(configBuffer);

  inFrame  = EPTool_GetClassFrame(hEpConfigEnc, bitFlag);

  fprintf (stderr, "epenc: created %d classes\n", inFrame->nrValidBuffer);


  while (1) {
    int dlen;
    
    /* read input data and input info */
    if (NULL == fgets(buf, sizeof(buf), inInfo)){
      fprintf(stderr, "epenc: End of info file reached\n");
      break;
    }

    if (1 != sscanf(buf, "%d", &dlen)) {
      fprintf(stderr, "epenc: Data length Error\n");
    }

    fread(inBuffer, 1,(dlen-bitrem+7)/8, inStream);
    
    /* read side info from file */
    MySideInfoParse( argv[1], inInfo, inFrame); 
    
    /* feed class freme from input buffer */
    bitrem = MyFeedFrameClass (inBuffer, inFrame , bitFlag); 
    
    /* EP Encode one frame */
    EPTool_EncodeFrame(hEpConfigEnc, inFrame, outBuffer, &outLength, bitFlag);

    if (bitFlag){
      for (i=0; i<outLength; i++)
        encOutBuffer[i] = outBuffer[i];
      writebits(outStream, outLength, encOutBuffer);

    } else {

      BytesToBits (outBuffer, &encOutBufferTmp[offsetEnc], ((outLength+7)/8)*8);
      offsetEnc += outLength;
      BitsToBytes ((unsigned char *)encOutBuffer, encOutBufferTmp, (offsetEnc/8)*8);
      fwrite (encOutBuffer, 1, offsetEnc/8, outStream);
      
      if (offsetEnc%8){
        for (i=0; i<offsetEnc%8; i++)
          encOutBufferTmp[i] = encOutBufferTmp[(offsetEnc/8)*8+i];
        offsetEnc = offsetEnc%8;
      }
      else offsetEnc = 0; 
    }

    fprintf(outInfo, "%d\n", outLength); fflush(outInfo);

    fprintf(stderr, "\r[%d]", nFrames++);
  }

  if (!bitFlag){
    if (offsetEnc){
      for (i=offsetEnc;i<8;encOutBufferTmp[i++]=0);
      BitsToBytes ((unsigned char *)encOutBuffer, encOutBufferTmp, 8);
      fwrite (encOutBuffer, 1, 1, outStream);
    }
  } else {
    if((bitrem = outLength%8)){
      for(i=0; i<bitrem; i++){
        encOutBuffer[i] = encOutBuffer[outLength - bitrem + i];
      } 
      for(;i<8; i++){
        encOutBuffer[i] = 0;
      } 
      writebits(outStream, 8, encOutBuffer);
    } 
  }  
  
  if(inFrame)
    free(inFrame);
  
  if (EPTool_Destroy( hEpConfigEnc )) {
    fprintf(stderr,"EPTool: EPTool_Destroy(): epenc.c: Error: could not destroy encoder EP tool handle\n");
    free(configLength);
    free(configBuffer);
    exit(-1);
  }

  if(configLength)
    free(configLength);
  if(configBuffer)
    free(configBuffer);

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
