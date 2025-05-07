/*
This software module was originally developed by Apple Computer, Inc. and Ericsson Research
in the course of development of MPEG-4.
This software module is an implementation of a part of one or
more MPEG-4 tools as specified by MPEG-4.
ISO/IEC gives users of MPEG-4 free license to this
software module or modifications thereof for use in hardware
or software products claiming conformance to MPEG-4.
Those intending to use this software module in hardware or software
products are advised that its use may infringe existing patents.
The original developer of this software module and his/her company,
the subsequent editors and their companies, and ISO/IEC have no
liability for use of this software module or modifications thereof
in an implementation.
Copyright is not released for non MPEG-4 conforming
products. The copyright owners retain full right to use the code for its own
purpose, assign or donate the code to a third party and to
inhibit third parties from using the code for non
MPEG-4 conforming products.
This copyright notice must be included in all copies or
derivative works. Copyright (c) Apple Computer and Telefonaktiebolaget LM Ericsson 2001
*/
/*
  $Id: BitRateAtom.c,v 1.1 2004/09/15 17:00:48 erapefh Exp $
*/

#include "MP4Atoms.h"
#include <stdlib.h>

static void destroy(MP4AtomPtr s)
{
  MP4BitRateAtomPtr self;
  self = (MP4BitRateAtomPtr)s;
  if(self == NULL) return;
  if(self->super) self->super->destroy(s);
}

static MP4Err serialize(struct MP4Atom *s, char *buffer)
{
  MP4Err err;
  MP4BitRateAtomPtr self = (MP4BitRateAtomPtr)s;
  err                    = MP4NoErr;

  err = MP4SerializeCommonBaseAtomFields(s, buffer);
  if(err) goto bail;
  buffer += self->bytesWritten;
  if(self->type == MP4BitRateAtomType)
  {
    PUT32(buffersizeDB);
    PUT32(max_bitrate);
    PUT32(avg_bitrate);
  }
  else
  {
    PUT32(avg_bitrate);
    PUT32(max_bitrate);
  }

  assert(self->bytesWritten == self->size);
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err calculateSize(struct MP4Atom *s)
{
  MP4Err err;
  MP4BitRateAtomPtr self = (MP4BitRateAtomPtr)s;
  err                    = MP4NoErr;

  err = MP4CalculateBaseAtomFieldSize(s);
  if(err) goto bail;
  self->size += 4 + 4;
  if(self->type == MP4BitRateAtomType) self->size += 4;
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err createFromInputStream(MP4AtomPtr s, MP4AtomPtr proto, MP4InputStreamPtr inputStream)
{
  MP4Err err;
  MP4BitRateAtomPtr self = (MP4BitRateAtomPtr)s;

  err = MP4NoErr;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr)
  err = self->super->createFromInputStream(s, proto, (char *)inputStream);
  if(err) goto bail;
  if(self->type == MP4BitRateAtomType)
  {
    GET32(buffersizeDB);
    GET32(max_bitrate);
    GET32(avg_bitrate);
  }
  else
  {
    GET32(avg_bitrate);
    GET32(max_bitrate);
  }

bail:
  TEST_RETURN(err);

  return err;
}

MP4Err MP4CreateBitRateAtom(MP4BitRateAtomPtr *outAtom)
{
  MP4Err err;
  MP4BitRateAtomPtr self;

  self = (MP4BitRateAtomPtr)calloc(1, sizeof(MP4BitRateAtom));
  TESTMALLOC(self);

  err = MP4CreateBaseAtom((MP4AtomPtr)self);
  if(err) goto bail;
  self->type                  = MP4BitRateAtomType;
  self->name                  = "Bitrate";
  self->createFromInputStream = (cisfunc)createFromInputStream;
  self->destroy               = destroy;
  self->calculateSize         = calculateSize;
  self->serialize             = serialize;
  *outAtom                    = self;
bail:
  TEST_RETURN(err);

  return err;
}
