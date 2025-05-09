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
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "sac_bd_embedder.h"

#define MIN(x,y)   (((x) < (y)) ? (x) :  (y))
#define MAX(x,y)   (((x) > (y)) ? (x) :  (y))

#define MAX_CHANNELS           8
#define MAX_FRMSIZE         8192
#define MAX_SUBFRAMES         16
#define MAX_BLOCKSIZE       8192

#define PCM_BLOCK_SIZE        64

#define CRC16_POLYNOMIAL    0x8005
#define CRC8_POLYNOMIAL     0x0007

#define MAX_BITS   15

static int getbits (unsigned char *buffer, int bits, int *byte_ptr, int *bit_ptr)
{
  int bit, nr = bits, out = 0;

  while (nr > 0)
  {
    nr--;
    bit = ((int) buffer[*byte_ptr] & (1 << *bit_ptr)) >> *bit_ptr;
    out += bit << nr;
    if (*bit_ptr == 0)
    {
      (*byte_ptr)++;
      *bit_ptr = 7;
    }
    else
    {
      (*bit_ptr)--;
    }
  }

  return (out);
}

static void putbits (int data, unsigned char *buffer, int bits, int *byte_ptr, int *bit_ptr)
{
  int bit, nr = bits;

  while (nr > 0)
  {
    nr--;
    bit = (data & (1 << nr)) >> nr;
    if (*bit_ptr == 7)
      buffer[*byte_ptr] = (unsigned char) (bit << *bit_ptr);
    else
      buffer[*byte_ptr] = (unsigned char) (buffer[*byte_ptr] + (bit << *bit_ptr));
    if (*bit_ptr == 0)
    {
      (*byte_ptr)++;
      *bit_ptr = 7;
    }
    else
    {
      (*bit_ptr)--;
    }
  }
}

static void update_CRC8 (unsigned int data, unsigned int length,
                         unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1))
  {
    carry = *crc & 0x80;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC8_POLYNOMIAL;
  }
  *crc &= 0xff;
}

static void update_CRC16 (unsigned int data, unsigned int length,
                          unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1))
  {
    carry = *crc & 0x8000;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC16_POLYNOMIAL;
  }
  *crc &= 0xffff;
}

static unsigned int randomize (unsigned int data, unsigned int length,
                               unsigned int *state)
{
  int i;
  unsigned int x, result = 0;

  for (i = length-1; i >= 0; i--)
  {
    x = (data&(1<<i))!=0;
    x ^= *state & 0x0001;
    x ^= ((*state & 0x0004) >> 2);
    x ^= ((*state & 0x2000) >> 13);
    x ^= ((*state & 0x8000) >> 15);
    *state = ((*state << 1) + x) & 0xFFFF;
    result |= x<<i;
  }
  return (result);
}

static void interleave_header (unsigned char *bd_header, long channels, long bytes)
{
  unsigned char buffer[(MAX_CHANNELS+1)*32];
  int i, ch;
  int byte_ptr, bit_ptr;

  byte_ptr = 0;
  bit_ptr = 7;
  for (i = 0; i < 8 * bytes * channels; i++)
  {
    buffer[i] = (unsigned char) getbits (bd_header, 1, &byte_ptr, &bit_ptr);
  }

  byte_ptr = 0;
  bit_ptr = 7;
  for (i = 0; i < 8 * bytes; i++)
  {
    for (ch = 0; ch < channels; ch++)
    {
      putbits (buffer[i+ch*8*bytes], bd_header, 1, &byte_ptr, &bit_ptr);
    }
  }
}

static int calc_Hbytes (int channels, int subframes)
{
  int Hbits = 40 + subframes * channels * 4;
  int Hsize = (Hbits + channels - 1) / channels;
  int Hbytes = ((Hsize + 7) / 8);

  Hbytes = MAX (Hbytes, 4);

  return (Hbytes);
}

tBdEmbedder* BdEmbedderOpen(int channels, int frameSlots)
{
  int error = 0;
  tBdEmbedder* self = calloc(1, sizeof(*self));
  if (self == NULL)
  {
    error = 1;
  }
  if (!error)
  {
    self->header = calloc((MAX_CHANNELS+1)*128, sizeof(*self->header));
    if (self->header == NULL)
    {
      error = 1;
    }
  }
  if (!error)
  {
    self->buffer = calloc(MAX_CHANNELS*MAX_FRMSIZE, sizeof(*self->buffer));
    if (self->buffer == NULL)
    {
      error = 1;
    }
  }
  if (!error)
  {
    self->channels = channels;
    self->frameSize = frameSlots * PCM_BLOCK_SIZE;
  }
  if (error)
  {
    BdEmbedderClose(self);
    self = NULL;
  }

  return self;
}

void BdEmbedderClose(tBdEmbedder* self)
{
  if (self != NULL)
  {
    if (self->header != NULL)
    {
      free(self->header);
    }
    if (self->buffer != NULL)
    {
      free(self->buffer);
    }
    free(self);
  }
}

void BdEmbedderApply(tBdEmbedder* self, unsigned char* dataBuffer, int dataSize, int isHeader, float* pcmBuffer)
{
  int i, blk, ch;
  int get_byte_ptr1, get_bit_ptr1;
  int get_byte_ptr2, get_bit_ptr2;
  int put_byte_ptr, put_bit_ptr;
  int bit_alloc, alloc_sum, Hbits, Hbytes, Hsize, Dbytes, payloadLength;
  int start, rem_bytes, padding_bits, subfrmsize, dataptr;
  int bsChannels, bsFramelength, bsSubframes;
  unsigned int crc8, crc, state, code;

  long          subframes    = 2;
  long          frmsize      = self->frameSize;
  long          channels     = self->channels;
  unsigned char *bd_header   = self->header;
  unsigned char *bd_raw_buf  = self->buffer;
  unsigned char *data_buffer = dataBuffer;
  long          data_bytes   = dataSize;
  float         *pcmbuf      = pcmBuffer;
  long          bd_syncframe = isHeader;

  subfrmsize = frmsize / subframes;

  Hbits = 40 + subframes * channels * 4;
  Hbytes = calc_Hbytes (channels, subframes);
  Hsize = Hbytes * channels;
  padding_bits = Hsize * 8 - Hbits;
  payloadLength = data_bytes;

  bit_alloc = (int) ceil(8.0f * (Hsize + payloadLength) / (frmsize * channels));

  bsChannels = channels - 1;
  bsFramelength = (frmsize / PCM_BLOCK_SIZE) - 1;
  bsSubframes = subframes - 1;

  put_byte_ptr = 0;
  put_bit_ptr = 7;
  crc8 = 0xff;
  putbits (0xAA, bd_header, 8, &put_byte_ptr, &put_bit_ptr);
  putbits (0x95, bd_header, 8, &put_byte_ptr, &put_bit_ptr);
  putbits (bsChannels, bd_header, 4, &put_byte_ptr, &put_bit_ptr);
  update_CRC8 (bsChannels, 4, &crc8);
  putbits (bsFramelength, bd_header, 7, &put_byte_ptr, &put_bit_ptr);
  update_CRC8 (bsFramelength, 7, &crc8);
  putbits (bsSubframes, bd_header, 4, &put_byte_ptr, &put_bit_ptr);
  update_CRC8 (bsSubframes, 4, &crc8);
  putbits (0L, bd_header, 1, &put_byte_ptr, &put_bit_ptr);
  update_CRC8 (0L, 1, &crc8);
  for (blk = alloc_sum = 0; blk < subframes; blk++)
  {
    for (ch = 0; ch < channels; ch++)
    {
      putbits (bit_alloc, bd_header, 4, &put_byte_ptr, &put_bit_ptr);
      update_CRC8 (bit_alloc, 4, &crc8);
      alloc_sum += bit_alloc;
    }
  }
  Dbytes = alloc_sum * subfrmsize / 8;
  for (ch = 0; ch < channels; ch++)
  {
    if (bit_alloc != 0)
    {
      Dbytes -= Hbytes;
    }
  }
  putbits (crc8, bd_header, 8, &put_byte_ptr, &put_bit_ptr);
  for (i=0; i<padding_bits; i++)
  {
    putbits (0L, bd_header, 1, &put_byte_ptr, &put_bit_ptr);
  }

  start=dataptr=Hsize;
  rem_bytes = Dbytes - (dataptr - Hsize);

  if (rem_bytes >= 3)
  {
    if (payloadLength > 0)
    {
      if (payloadLength > 65535)
      {
        if ((payloadLength > rem_bytes - 6) && (rem_bytes >= 6))
        {
          fprintf (stderr, "Error: payload bytes (%d) exceeds Burried Data capacity (%d), data is truncated!\n",
                           payloadLength, rem_bytes - 6);
          payloadLength = rem_bytes - 6;
        }
        if (bd_syncframe)
        {
          bd_raw_buf[dataptr++] = 0x23;
        }
        else
        {
          bd_raw_buf[dataptr++] = 0x03;
        }
        bd_raw_buf[dataptr++] = (unsigned char) (payloadLength >> 16);
        bd_raw_buf[dataptr++] = (unsigned char) ((payloadLength & 0xffff) >> 8);
        bd_raw_buf[dataptr++] = (unsigned char) (payloadLength & 0xff);
      }
      else if (payloadLength > 255)
      {
        if ((payloadLength > rem_bytes - 5) && (rem_bytes >= 5))
        {
          fprintf (stderr, "Error: payload bytes (%d) exceeds Burried Data capacity (%d), data is truncated!\n",
                           payloadLength, rem_bytes - 5);
          payloadLength = rem_bytes - 5;
        }
        if (bd_syncframe)
        {
          bd_raw_buf[dataptr++] = 0x22;
        }
        else
        {
          bd_raw_buf[dataptr++] = 0x02;
        }
        bd_raw_buf[dataptr++] = (unsigned char) ((payloadLength & 0xffff) >> 8);
        bd_raw_buf[dataptr++] = (unsigned char) (payloadLength & 0xff);
      }
      else
      {
        if ((payloadLength > rem_bytes - 4) && (rem_bytes >= 4))
        {
          fprintf (stderr, "Error: payload bytes (%d) exceeds Burried Data capacity (%d), data is truncated!\n",
                           payloadLength, rem_bytes - 4);
          payloadLength = rem_bytes - 4;
        }
        if (bd_syncframe)
        {
          bd_raw_buf[dataptr++] = 0x21;
        }
        else
        {
          bd_raw_buf[dataptr++] = 0x01;
        }
        bd_raw_buf[dataptr++] = (unsigned char) (payloadLength & 0xff);
      }

      for (i = 0; i < data_bytes; i++)
      {
        bd_raw_buf[dataptr++] = data_buffer[i];
      }
    }
    else
    {
      bd_raw_buf[dataptr++] = 0x00;
    }
    crc = 0xffff;
    for (i = start; i < dataptr; i++)
    {
      update_CRC16 (bd_raw_buf[i], 8, &crc);
    }
    bd_raw_buf[dataptr++] = (unsigned char) (crc >> 8);
    bd_raw_buf[dataptr++] = (unsigned char) (crc & 0xff);
  }

  start=dataptr;
  rem_bytes = Dbytes - (dataptr - Hsize);
  if (rem_bytes >= 3)
  {
    if (rem_bytes > 65540)
    {
      rem_bytes -= 6;
      bd_raw_buf[dataptr++] = 0xE3;
      bd_raw_buf[dataptr++] = (unsigned char) (rem_bytes >> 16);
      bd_raw_buf[dataptr++] = (unsigned char) ((rem_bytes & 0xffff) >> 8);
      bd_raw_buf[dataptr++] = (unsigned char) (rem_bytes & 0xff);
    }
    else if (rem_bytes > 259)
    {
      rem_bytes -= 5;
      bd_raw_buf[dataptr++] = 0xE2;
      bd_raw_buf[dataptr++] = (unsigned char) ((rem_bytes & 0xffff) >> 8);
      bd_raw_buf[dataptr++] = (unsigned char) (rem_bytes & 0xff);
    }
    else if (rem_bytes > 3)
    {
      rem_bytes -= 4;
      bd_raw_buf[dataptr++] = 0xE1;
      bd_raw_buf[dataptr++] = (unsigned char) (rem_bytes & 0xff);
    }
    else
    {
      rem_bytes -= 3;
      bd_raw_buf[dataptr++] = 0xE0;
    }
    for (i = 0; i < rem_bytes; i++)
    {
      bd_raw_buf[dataptr++] = 0;
    }
    crc = 0xffff;
    for (i = start; i < dataptr; i++)
    {
      update_CRC16 (bd_raw_buf[i], 8, &crc);
    }
    bd_raw_buf[dataptr++] = (unsigned char) (crc >> 8);
    bd_raw_buf[dataptr++] = (unsigned char) (crc & 0xff);
  }
  while (dataptr - Hsize < Dbytes)
  {
    bd_raw_buf[dataptr++] = 0;
  }

  state = 0xffff;

  for (i = 2; i < Hsize; i++)
  {
    bd_header[i] = (unsigned char) randomize (bd_header[i], 8, &state);
  }

  for (i = Hsize; i < dataptr; i++)
  {
    bd_raw_buf[i] = (unsigned char) randomize (bd_raw_buf[i], 8, &state);
  }

  for (i = Hsize; i < Hsize*MAX_BITS; i++)
  {
    bd_header[i] = bd_raw_buf[i];
  }

  interleave_header(bd_header, channels, Hbytes);

  put_byte_ptr = 0;
  put_bit_ptr = 7;
  get_byte_ptr1 = 0;
  get_bit_ptr1 = 7;
  get_byte_ptr2 = Hsize;
  get_bit_ptr2 = 7;
  for (i = 0; i < Hbytes*8; i++)
  {
    for (ch = 0; ch < channels; ch++)
    {
      if (bit_alloc > 1)
      {
        code = getbits (bd_header, bit_alloc-1, &get_byte_ptr2, &get_bit_ptr2);
        putbits (code, bd_raw_buf, bit_alloc-1, &put_byte_ptr, &put_bit_ptr);
      }
      code = getbits (bd_header, 1, &get_byte_ptr1, &get_bit_ptr1);
      putbits (code, bd_raw_buf, 1, &put_byte_ptr, &put_bit_ptr);
    }
  }
  get_byte_ptr1 = 0;
  get_bit_ptr1 = 7;
  for (i = 0; i < frmsize; i++)
  {
    for (ch = 0; ch < channels; ch++)
    {
      int bits = bit_alloc;

      if ((bits == 0) && (i < Hbytes*8))
      {
        bits = 1;
      }

      if (bits > 0)
      {
        int sample = (int) (pcmbuf[i * channels + ch] * 32768.0f);
        int buried = getbits (bd_raw_buf, bits, &get_byte_ptr1, &get_bit_ptr1);

#define CLIPPING_PROTECTION

#ifdef CLIPPING_PROTECTION
        if (sample >= 32768)      sample =  32767;
        else if (sample < -32768) sample = -32768;
#endif
        sample &= ~((1 << bits) - 1);
        sample |= buried;

        pcmbuf[i * channels + ch] = sample / 32768.0f;
      }
    }
  }
}
