/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#include "sac_bitinput.h"

#include "sac_config.h"
#include "sac_reshuffinit.h"



struct s_tagBitInput {
  HANDLE_BYTE_READER reader;
  long  byte_count;
  int   nbits;
  unsigned long cword;
};

HANDLE_S_BITINPUT s_static_input = NULL;




static HANDLE_S_BITINPUT bitInput_alloc()
{
  HANDLE_S_BITINPUT ret = (HANDLE_S_BITINPUT)malloc(sizeof(struct s_tagBitInput));
  if (ret==NULL) return NULL;

  ret->reader = NULL;
  ret->byte_count = 0;
  ret->nbits = 0;
  ret->cword = 0;

  if (s_static_input == NULL) s_static_input = ret;

  return ret;
}

static void bitInput_free(HANDLE_S_BITINPUT input)
{
  if (s_static_input != NULL) s_static_input = NULL;

  free(input);
}




int s_bitinput_resetCounter(HANDLE_S_BITINPUT input)
     
{
  int read_bytes = input->byte_count-(input->nbits>>3);
  input->byte_count = (input->nbits>>3);
  return read_bytes;
}



long s_getbits(int n)
{
  return s_GetBits(s_static_input, n);
}

long s_GetBits(HANDLE_S_BITINPUT input, long n)
{
  int retval;
  if (!n) return 0;

  while (input->nbits < n) {
    int c;
	HANDLE_BYTE_READER tmp = input->reader;
    if ((c = input->reader->ReadByte(tmp)) < 0) {
      CommonExit(1,"GetBits: End of Bitstream\n");
    }
    input->byte_count++;
    input->cword = (input->cword<<8) | c;
    input->nbits += 8;
  }

  input->nbits -= n;
  retval = (input->cword>>input->nbits) & ((1<<n)-1);
  return retval;
}

int s_byte_align(void)
{
  return s_byteAlign(s_static_input);
}

int s_byteAlign(HANDLE_S_BITINPUT input)
{
  int i=0;

  if (debug['a']) PRINT(SE,"align: %lx %d\n", input->cword, input->nbits);

  i=(input->nbits & ((1<<3)-1));
  s_getbits(i);

  return(i);
}

long s_getNumBits(HANDLE_S_BITINPUT input)
{
  return (input->byte_count<<3) - input->nbits;
}




HANDLE_S_BITINPUT s_bitinput_open(HANDLE_BYTE_READER byteReader)
{
  HANDLE_S_BITINPUT ret = bitInput_alloc();
  if (ret==NULL) return NULL;

  ret->reader = byteReader;

  return ret;
}

void s_bitinput_close(HANDLE_S_BITINPUT input)
{
  if (input==NULL) return;
  bitInput_free(input);
}




void s_set_log_level(int id, int add_bits_num, int add_bits)
{
    ( id );
    ( add_bits_num );
    ( add_bits );
}
