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
#include "dcm_io.h"

int 
writebits(FILE *fpout,int packetlen,int *buffer)
{
  static int bitrem = 0;
  static unsigned char tmp;
  int i;

  if(packetlen == -1){		/* Flash */
    if(bitrem == 0) return bitrem;
    tmp <<= (8 - bitrem);
    fwrite(&tmp, sizeof(char), 1, fpout);
    return bitrem;
  }
  
  for(i=0; i<packetlen; i++){
    tmp <<= 1;
    tmp |= *(buffer++);
    bitrem ++;
    if(bitrem == 8){
      fwrite(&tmp, sizeof(char), 1, fpout);
      bitrem = 0;
    }
  }

  return bitrem;
}

int 
readbits(FILE *fpin, int  packetlen, int *buffer)
{
  static int bitrem = 0;
  static unsigned char tmp;
  int i;

  for(i=0; i<packetlen; i++){
    if(bitrem == 0){
      if(fread(&tmp, sizeof(char), 1, fpin) == 0)
	return i;		/* EOF */
      bitrem = 8;
    }
    *(buffer++) = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
  return i;
}
