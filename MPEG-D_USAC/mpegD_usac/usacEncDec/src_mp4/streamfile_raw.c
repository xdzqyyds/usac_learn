/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer, RAW-bitstream part

This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

and edited by


in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 2004.
Source file: streamfile_raw.c

 

Required modules:
cmdline.o               parse commandline
common_m4a.o            common module
bitstream.o             bit stream module
flex_mux.o		flexmux module
streamfile_helper.o     stream file helper module

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj_descr.h"           /* structs */

#include "allHandles.h"
#include "common_m4a.h"          /* common module       */
#include "bitstream.h"           /* bit stream module   */
#include "flex_mux.h"            /* parse object descriptors */
#include "cmdline.h"             /* parse commandline options */
#include "mp4au.h"               /* frame work common declarations */


#include "streamfile.h"          /* public functions */
#include "streamfile_raw.h"
#include "streamfile_helper.h"

/* ---- flexmux specific structures ---- */
#define MODULE_INFORMATION "StreamFile transport lib: Raw-Stream module"

struct tagStreamSpecificInfo {
  HANDLE_BSBITSTREAM bitStream;
  int                useByteAlignment;
};

static int commandline(HANDLE_STREAMFILE stream, int argc, char**argv)
{
  int skipByteAlign;

  CmdLineSwitch switchList[] = {
    {"no_align",NULL,NULL,NULL,NULL,"skip byte alginment between access units"},
    {NULL,NULL,NULL,NULL,NULL,NULL}
  };
  CmdLinePara paraList[] = {
    {NULL,NULL,NULL}
  };
  switchList[0].argument = &skipByteAlign;

  if (stream!=NULL) {
    if (CmdLineEval(argc,argv,paraList,switchList,1,NULL)) return -10;

    stream->spec->useByteAlignment = !skipByteAlign;
  } else {
    CmdLineHelp(NULL,paraList,switchList,stdout);
  }

  return 0;
}

/* --------------------------------------------------- */
/* ---- Raw-Stream (RAW)                          ---- */
/* --------------------------------------------------- */


static int RAWinitProgram(HANDLE_STREAMPROG prog)
{
  prog->programData->spec = NULL;
  return 0;
}


static int RAWopenWrite(HANDLE_STREAMFILE stream, int argc, char** argv)
{
  char info[256] = "";
  int result;

  /* - options parsing */

  result = commandline(stream, argc, argv);
  if (result) return result;

  /* - open bit stream file */
  stream->spec->bitStream = BsOpenFileWrite(stream->fileName,NULL,info);
  if (stream->spec->bitStream==NULL) {
    DebugPrintf(1,"StreamFile:openWrite(RAW): error opening bit stream for writing");
    return -1;
  }
  return 0;
}


static int RAWheaderWrite(HANDLE_STREAMFILE stream)
{
  stream=stream; /* avoid warning */
  return 0;
}


static int RAWputAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  unsigned long totalLength;
  unsigned long paddingBits;
  unsigned char *srcPtr = au->data;
  HANDLE_BSBITSTREAM outfile = prog->fileData->spec->bitStream;

  trackNr=trackNr; /* avoid warning */
  totalLength = au->numBits >> 3;
  paddingBits = 8-(au->numBits&7);

  /* put AU into track */
  for (;totalLength;totalLength--,srcPtr++) {
    unsigned long tmp = *srcPtr;
    BsPutBit(outfile, tmp, 8);
  }

  if (paddingBits!=8) {
    BsPutBit(outfile, (*srcPtr)>>paddingBits, 8-paddingBits);
    if (prog->fileData->spec->useByteAlignment) {
      BsPutBit(outfile, 0, paddingBits);
    }
  }

  return 0;
}


static int RAWclose(HANDLE_STREAMFILE stream)
{
  return BsClose(stream->spec->bitStream);
}


/* --------------------------------------------------- */
/* ---- Constructor                               ---- */
/* --------------------------------------------------- */


int RAWinitStream(HANDLE_STREAMFILE stream)
{
  stream->initProgram=RAWinitProgram;
  stream->openRead=NULL;
  stream->getDependency=NULL;
  stream->openTrack=NULL;
  stream->openWrite=RAWopenWrite;
  stream->headerWrite=RAWheaderWrite;
  stream->close=RAWclose;
  stream->getAU=NULL;
  stream->putAU=RAWputAccessUnit;

  if ((stream->spec = (struct tagStreamSpecificInfo*)malloc(sizeof(struct tagStreamSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initStream: error in malloc");
    return -1;
  }
  memset(stream->spec, 0, sizeof(struct tagStreamSpecificInfo));
  return 0;
}

void RAWshowHelp( void )
{
  printf(MODULE_INFORMATION);
  commandline(NULL,0,NULL);
}
