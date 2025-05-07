/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

and edited by:

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

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: spatial_bitstreamreader_buffer.c,v 1.1.1.1 2009-05-29 14:10:20 mul Exp $
*******************************************************************************/



#include <stdio.h>
#include <stdlib.h>

#define PRINT fprintf
#define SE stderr

#include "spatial_bitstreamreader_buffer.h"

#define V2_2_WARNINGS_ERRORS_FIX

struct BITSTREAM_READER_BUFFER {
  HANDLE_BYTE_READER       byteReader;
  HANDLE_BUFFER            hVm;
  CODE_TABLE               code;
  HANDLE_RESILIENCE        hResilience;
  HANDLE_ESC_INSTANCE_DATA hEPInfo;
};



static int ReadByte(HANDLE_BYTE_READER byteReader)
{
  HANDLE_BITSTREAM_READER_BUFFER self = *(HANDLE_BITSTREAM_READER_BUFFER*)&byteReader[1];
  long data;

  data = GetBits (8,
                  self->code,
                  self->hResilience,
                  self->hEPInfo,
                  self->hVm );

  return data;
}


HANDLE_BITSTREAM_READER_BUFFER BitstreamReaderBufferOpen(HANDLE_BUFFER hVm, 
                                                         CODE_TABLE               code, 
                                                         HANDLE_RESILIENCE        hResilience, 
                                                         HANDLE_ESC_INSTANCE_DATA hEPInfo)
{
  int error = 0;
  HANDLE_BITSTREAM_READER_BUFFER self = (HANDLE_BITSTREAM_READER_BUFFER) calloc(1, sizeof(*self));
  if (self == NULL) error = 1;

  if (!error) {
    self->byteReader = (HANDLE_BYTE_READER) calloc(1, sizeof(*self->byteReader) + sizeof(self));
    if (self->byteReader == NULL) error = 1;
  }

  if (!error) {
    self->byteReader->ReadByte = ReadByte;
    *(HANDLE_BITSTREAM_READER_BUFFER*)&self->byteReader[1] = self;
    if ( (hVm == NULL) ) {
      PRINT(SE, "Invalid Bitstream.");
      error = 1;
    } else {
      self->hVm = hVm;
	  self->code = code;
	  self->hEPInfo = hEPInfo;
	  self->hResilience = hResilience;
    }
  }

  if (error) {
    BitstreamReaderBufferClose(self);
    self = NULL;
  }

  return self;
}


void BitstreamReaderBufferClose(HANDLE_BITSTREAM_READER_BUFFER self)
{
  if (self != NULL) {
    if (self->byteReader != NULL) free(self->byteReader);
    free(self);
  }
}


HANDLE_BYTE_READER BitstreamReaderBufferGetByteReader(HANDLE_BITSTREAM_READER_BUFFER self)
{
  return self->byteReader;
}

