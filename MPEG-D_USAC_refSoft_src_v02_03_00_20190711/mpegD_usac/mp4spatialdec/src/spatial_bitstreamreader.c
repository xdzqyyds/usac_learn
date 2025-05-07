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


#include <stdio.h>
#include <stdlib.h>

#define PRINT fprintf
#define SE stderr

#include "spatial_bitstreamreader.h"

struct BITSTREAM_READER {
  HANDLE_BYTE_READER byteReader;
  HANDLE_BSBITSTREAM bitstream;
};



static int ReadByte(HANDLE_BYTE_READER byteReader)
{
  HANDLE_BITSTREAM_READER self = *(HANDLE_BITSTREAM_READER*)&byteReader[1];
  char data;
  BsGetBitChar(self->bitstream, 
               &data, 
               8);

  return data;
}


HANDLE_BITSTREAM_READER BitstreamReaderOpen(HANDLE_BSBITSTREAM bitstream)
{
  int error = 0;
  HANDLE_BITSTREAM_READER self = calloc(1, sizeof(*self));
  if (self == NULL) error = 1;

  if (!error) {
    self->byteReader = calloc(1, sizeof(*self->byteReader) + sizeof(self));
    if (self->byteReader == NULL) error = 1;
  }

  if (!error) {
    self->byteReader->ReadByte = ReadByte;
    *(HANDLE_BITSTREAM_READER*)&self->byteReader[1] = self;
    if (bitstream == NULL) {
      PRINT(SE, "Invalid Bitstream.");
      error = 1;
    } else {
      self->bitstream = bitstream;
    }
  }

  if (error) {
    BitstreamReaderClose(self);
    self = NULL;
  }

  return self;
}


void BitstreamReaderClose(HANDLE_BITSTREAM_READER self)
{
  if (self != NULL) {
    if (self->byteReader != NULL) free(self->byteReader);
    free(self);
  }
}


HANDLE_BYTE_READER FileReaderGetByteReader(HANDLE_BITSTREAM_READER self)
{
  return self->byteReader;
}

