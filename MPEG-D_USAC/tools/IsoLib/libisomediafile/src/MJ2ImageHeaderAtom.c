/*
This software module was originally developed by Apple Computer, Inc.
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
products. Apple Computer, Inc. retains full right to use the code for its own
purpose, assign or donate the code to a third party and to
inhibit third parties from using the code for non
MPEG-4 conforming products.
This copyright notice must be included in all copies or
derivative works. Copyright (c) 1999.
*/
/*
  $Id: MJ2ImageHeaderAtom.c,v 1.1.1.1 2002/09/20 08:53:34 julien Exp $
*/

#include "MJ2Atoms.h"
#include <stdlib.h>
#include <string.h>

static void destroy(MP4AtomPtr s)
{
  MJ2ImageHeaderAtomPtr self = (MJ2ImageHeaderAtomPtr)s;
  if(self == NULL) return;
  if(self->super) self->super->destroy(s);
}

static ISOErr serialize(struct MP4Atom *s, char *buffer)
{
  ISOErr err;
  MJ2ImageHeaderAtomPtr self = (MJ2ImageHeaderAtomPtr)s;

  err = ISONoErr;

  err = MP4SerializeCommonBaseAtomFields(s, buffer);
  if(err) goto bail;
  buffer += self->bytesWritten;

  PUTBYTES(&self->height, sizeof(u32));
  PUTBYTES(&self->width, sizeof(u32));
  PUTBYTES(&self->compCount, sizeof(u16));
  PUTBYTES(&self->compBits, sizeof(u8));
  PUTBYTES(&self->compressionType, sizeof(u8));
  PUTBYTES(&self->colorspaceKnown, sizeof(u8));
  PUTBYTES(&self->ip, sizeof(u8));

  assert(self->bytesWritten == self->size);
bail:
  TEST_RETURN(err);

  return err;
}

static ISOErr calculateSize(struct MP4Atom *s)
{
  ISOErr err;
  MJ2ImageHeaderAtomPtr self = (MJ2ImageHeaderAtomPtr)s;
  err                        = ISONoErr;

  err = MP4CalculateBaseAtomFieldSize(s);
  if(err) goto bail;
  self->size += 2 * sizeof(u32);
  self->size += 1 * sizeof(u16);
  self->size += 4 * sizeof(u8);
bail:
  TEST_RETURN(err);

  return err;
}

static ISOErr createFromInputStream(MP4AtomPtr s, MP4AtomPtr proto, MP4InputStreamPtr inputStream)
{
  ISOErr err;
  MJ2ImageHeaderAtomPtr self = (MJ2ImageHeaderAtomPtr)s;

  err = ISONoErr;
  if(self == NULL) BAILWITHERROR(ISOBadParamErr)
  err = self->super->createFromInputStream(s, proto, (char *)inputStream);
  if(err) goto bail;

  GET32(height);
  GET32(width);
  GET16(compCount);
  GET8(compBits);
  GET8(compressionType);
  GET8(colorspaceKnown);
  GET8(ip);

bail:
  TEST_RETURN(err);

  return err;
}

ISOErr MJ2CreateImageHeaderAtom(MJ2ImageHeaderAtomPtr *outAtom)
{
  ISOErr err;
  MJ2ImageHeaderAtomPtr self;

  self = (MJ2ImageHeaderAtomPtr)calloc(1, sizeof(MJ2ImageHeaderAtom));
  TESTMALLOC(self);

  err = MP4CreateBaseAtom((MP4AtomPtr)self);
  if(err) goto bail;

  self->type                  = MJ2ImageHeaderAtomType;
  self->name                  = "JPEG 2000 image header atom";
  self->destroy               = destroy;
  self->createFromInputStream = (cisfunc)createFromInputStream;
  self->calculateSize         = calculateSize;
  self->serialize             = serialize;
  *outAtom                    = self;
bail:
  TEST_RETURN(err);

  return err;
}
