/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrbitb.c,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  Bitbuffer management $Revision: 1.1.1.1 $
*/
#include "ct_sbrbitb.h"


/*******************************************************************************
 Functionname:  get_short
 *******************************************************************************

 Description: Reads 16 bits from Bitbuffer

 Arguments:   hBitBuf Handle to Bitbuffer

 Return:      16 bits

*******************************************************************************/
static unsigned long
get_short (HANDLE_BIT_BUFFER hBitBuf)
{
  unsigned long short_value;
  unsigned long c1, c2;

  c1 = (unsigned long) (hBitBuf->char_ptr[0]);
  c2 = (unsigned long) (hBitBuf->char_ptr[1]);
  hBitBuf->char_ptr += 2;
  short_value = ((c1 << 8) | c2);

  return (short_value);
}


/*******************************************************************************
 Functionname:  BufGetBits
 *******************************************************************************

 Description: Reads n bits from Bitbuffer

 Arguments:   hBitBuf Handle to Bitbuffer
              n       Number of bits to read

 Return:      bits

*******************************************************************************/
unsigned long
BufGetBits (HANDLE_BIT_BUFFER hBitBuf, int n)
{
  unsigned long ret_value;

  /* read bits from MSB side */
  if (hBitBuf->buffered_bits <= 16) {
    hBitBuf->buffer_word = (hBitBuf->buffer_word << 16) | get_short (hBitBuf);
    hBitBuf->buffered_bits += 16;
  }
  hBitBuf->buffered_bits -= n;
  ret_value =
    (hBitBuf->buffer_word >> hBitBuf->buffered_bits) & ((1 << n) - 1);


  hBitBuf->nrBitsRead += n;

  return (ret_value);
}

/*******************************************************************************
 Functionname:  initBitBuffer
 *******************************************************************************

 Description: Initialize variables for reading the bitstream buffer

 Arguments:   hBitBuf     Handle to Bitbuffer
              start_ptr   start of  bitstream
              
 Return:      none

*******************************************************************************/
void
initBitBuffer (HANDLE_BIT_BUFFER hBitBuf,
	       unsigned char *start_ptr, unsigned long bufferLen)
{
  hBitBuf->char_ptr = start_ptr;
  hBitBuf->buffer_word = 0;
  hBitBuf->buffered_bits = 0;
  hBitBuf->half_word_left = 0;
  hBitBuf->nrBitsRead = 0;
  hBitBuf->bufferLen = bufferLen;
}

/*******************************************************************************
 Functionname:  CopyBitbufferState
 *******************************************************************************

 Description: 

 Arguments:   hBitBuf        Handle to Bitbuffer
              hBitBufDest    Handle to Bitbuffer
 Return:      none

*******************************************************************************/
void
CopyBitbufferState (HANDLE_BIT_BUFFER hBitBuf, HANDLE_BIT_BUFFER hBitBufDest)
{

  hBitBufDest->char_ptr = hBitBuf->char_ptr;
  hBitBufDest->buffer_word = hBitBuf->buffer_word;
  hBitBufDest->buffered_bits = hBitBuf->buffered_bits;
  hBitBufDest->half_word_left = hBitBuf->half_word_left;
  hBitBufDest->nrBitsRead = hBitBuf->nrBitsRead;
  hBitBufDest->bufferLen = hBitBuf->bufferLen;

}


/*******************************************************************************
 Functionname:  GetNrBitsAvailable
 *******************************************************************************

 Description: returns number bits available in bit buffer 

 Arguments:   hBitBuf     Handle to Bitbuffer
              
 Return:      none

*******************************************************************************/
unsigned long
GetNrBitsAvailable (HANDLE_BIT_BUFFER hBitBuf)
{

  return (hBitBuf->bufferLen - hBitBuf->nrBitsRead);
}
