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
        $Id: SimpleFileMappingObject.h,v 1.1.1.1 2002/09/20 08:53:34 julien Exp $
*/
#ifndef INCLUDED_SIMPLEFILEMAPPING_OBJECT_H
#define INCLUDED_SIMPLEFILEMAPPING_OBJECT_H

#include "FileMappingObject.h"

typedef struct SimpleFileMappingObjectRecord
{
  COMMON_FILEMAPPING_OBJECT_FIELDS
  char *fileName;
  int fd;
  u32 size;
} SimpleFileMappingObjectRecord, *SimpleFileMappingObject;

MP4Err MP4CreateSimpleFileMappingObject(char *filename, FileMappingObject *outObject);

#endif
