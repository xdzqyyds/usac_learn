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
  $Id: SceneDescriptionMediaHeader.c,v 1.1.1.1 2002/09/20 08:53:35 julien Exp $
*/

#include "MP4Atoms.h"
#include <stdlib.h>

static void destroy(MP4AtomPtr s)
{
  if(s == NULL) return;
  if(s->super) s->super->destroy(s);
}

static MP4Err serialize(struct MP4Atom *s, char *buffer)
{
  MP4Err err;
  MP4SceneDescriptionMediaHeaderAtomPtr self = (MP4SceneDescriptionMediaHeaderAtomPtr)s;
  err                                        = MP4NoErr;

  err = MP4SerializeCommonFullAtomFields((MP4FullAtomPtr)s, buffer);
  if(err) goto bail;
  buffer += self->bytesWritten;
  assert(self->bytesWritten == self->size);
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err calculateSize(struct MP4Atom *s)
{
  MP4Err err;
  /*MP4SceneDescriptionMediaHeaderAtomPtr self = (MP4SceneDescriptionMediaHeaderAtomPtr) s;*/
  err = MP4NoErr;

  err = MP4CalculateFullAtomFieldSize((MP4FullAtomPtr)s);
  if(err) goto bail;
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err createFromInputStream(MP4AtomPtr s, MP4AtomPtr proto, MP4InputStreamPtr inputStream)
{
  MP4Err err;
  MP4SceneDescriptionMediaHeaderAtomPtr self = (MP4SceneDescriptionMediaHeaderAtomPtr)s;

  err = MP4NoErr;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr)
  err = self->super->createFromInputStream(s, proto, (char *)inputStream);
  if(err) goto bail;
bail:
  TEST_RETURN(err);

  return err;
}

MP4Err MP4CreateSceneDescriptionMediaHeaderAtom(MP4SceneDescriptionMediaHeaderAtomPtr *outAtom)
{
  MP4Err err;
  MP4SceneDescriptionMediaHeaderAtomPtr self;

  self =
    (MP4SceneDescriptionMediaHeaderAtomPtr)calloc(1, sizeof(MP4SceneDescriptionMediaHeaderAtom));
  TESTMALLOC(self)

  err = MP4CreateFullAtom((MP4AtomPtr)self);
  if(err) goto bail;
  self->type                  = MP4SceneDescriptionMediaHeaderAtomType;
  self->name                  = "scene description media header";
  self->createFromInputStream = (cisfunc)createFromInputStream;
  self->destroy               = destroy;
  self->calculateSize         = calculateSize;
  self->serialize             = serialize;
  *outAtom                    = self;
bail:
  TEST_RETURN(err);

  return err;
}
