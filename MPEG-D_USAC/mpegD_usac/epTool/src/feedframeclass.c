#include "feedframeclass.h"

static int BytesToBits(unsigned char *inbuf, int *trans, int translen);
static void BitsToBytes(unsigned char *outbuf, int *trans, int translen );

int MyFeedFrameClass (unsigned char *buffer, HANDLE_CLASS_FRAME inFrame, int bitFlag)
{
  int i, j ,offset;
  int tmpBuffer[4096*7] = {0}, tmpLen = 0;
  int bitrem = 0;

  for (i = 0; i< inFrame->nrValidBuffer; i++)
    tmpLen += inFrame->classLength[i];

  bitrem = BytesToBits(buffer,tmpBuffer,tmpLen); 

  offset = 0;
  for (i = 0; i< inFrame->nrValidBuffer; i++) {
    if(!bitFlag){
      BitsToBytes(inFrame->classBuffer[i], &tmpBuffer[offset], ((inFrame->classLength[i] + 7)/8) *8);
    }else{
      for (j=0;j<inFrame->classLength[i];j++)
        inFrame->classBuffer[i][j] = tmpBuffer[offset + j];
    }
    offset += inFrame->classLength[i];
  }

  return bitrem;
  
}

static int BytesToBits(unsigned char *inbuf, int *trans, int translen)
{
  int i; 
  static int bitrem = 0;
  static unsigned char tmp = 0;

  for(i=0; i< translen; i++){
    if(bitrem == 0){
      tmp = inbuf[i/8] ;
      bitrem = 8;
    }
    trans[i] = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
  return bitrem;
}

static void BitsToBytes(unsigned char *outbuf, int *trans, int translen )
{
  int bitrem = 0;
  unsigned char tmp = 0;
  int i;

  for(i=0; i<translen; i++){
    tmp <<= 1;
    tmp |= *(trans++);
    bitrem ++;
    if(bitrem == 8){
      outbuf[i/8] = tmp;      
      bitrem = 0;
    }
  }

}
