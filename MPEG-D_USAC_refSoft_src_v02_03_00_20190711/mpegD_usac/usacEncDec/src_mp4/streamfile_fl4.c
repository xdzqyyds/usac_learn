/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer, FlexMux part

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

Copyright (c) 2003.
Source file: streamfile_fl4.c

 

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
#include "streamfile_fl4.h"
#include "streamfile_helper.h"

/* ---- flexmux specific structures ---- */
#define MODULE_INFORMATION "StreamFile transport lib: FlexMux module"

struct tagStreamSpecificInfo {
  HANDLE_BSBITSTREAM bitStream;
  OBJECT_DESCRIPTOR* objDescr;
  int                useStartBit;
  int                useEndBit;
};

struct tagProgSpecificInfo {
#ifdef VARIABLE_MAXTRACK
  unsigned long      *trackID;
  int                *ALuseAccessUnitStartFlag;
  int                *ALuseAccessUnitEndFlag;
  int                *ALseqNumLength;
#else
  unsigned long      trackID [MAXTRACK];
  int                ALuseAccessUnitStartFlag [MAXTRACK];
  int                ALuseAccessUnitEndFlag [MAXTRACK];
  int                ALseqNumLength [MAXTRACK];
#endif
};



static int commandline(HANDLE_STREAMFILE stream, int argc, char**argv)
{
   int useStartBit;
   int useEndBit;

  CmdLineSwitch switchList[] = {
    {"sb",NULL,"%i","1",NULL,"use AL start bits"},
    {"eb",NULL,"%i","1",NULL,"use AL end bits"},
    /*
    {"sb",&useStartBit,"%i","1",NULL,"use AL start bits"},
    {"eb",&useEndBit,"%i","1",NULL,"use AL end bits"},
    */
    {NULL,NULL,NULL,NULL,NULL,NULL}
  };
  CmdLinePara paraList[] = {
    {NULL,NULL,NULL}
  };
  switchList[0].argument = &useStartBit;
  switchList[1].argument = &useEndBit;

  if (stream!=NULL) {
    if (CmdLineEval(argc,argv,paraList,switchList,1,NULL)) return -10;

    stream->spec->useStartBit = useStartBit;
    stream->spec->useEndBit = useEndBit;
  } else {
    CmdLineHelp(NULL,paraList,switchList,stdout);
  }

  return 0;
}

/* --------------------------------------------------- */
/* ---- FlexMux (FL4)                             ---- */
/* --------------------------------------------------- */


static int FL4initProgram(HANDLE_STREAMPROG prog)
{
  if ((prog->programData->spec = (struct tagProgSpecificInfo*)malloc(sizeof(struct tagProgSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initProgram: error in malloc");
    return -1;
  }
  memset(prog->programData->spec, 0, sizeof(struct tagProgSpecificInfo));
  return 0;
}


static int FL4openRead(HANDLE_STREAMFILE stream)
{
  OBJECT_DESCRIPTOR objDescr;
  char *info = NULL;
  unsigned long align, x;
  unsigned long trackCount;

  /* open bit stream file */
  stream->spec->bitStream = BsOpenFileRead(stream->fileName,MP4_MAGIC,&info);
  if (stream->spec->bitStream==NULL) {
    DebugPrintf(3,"StreamFile:openRead(FL4): file is not FlexMux");
    return -1;
  }

  /* read from bitstream */
  initObjDescr(&objDescr);
  stream->spec->objDescr = &objDescr;
  if (BsGetBit(stream->spec->bitStream,&(objDescr.ODLength.value),objDescr.ODLength.length) ) {
    DebugPrintf(1,"StreamFile:openRead(FL4): error reading bit stream header");
    return -2;
  }

  advanceODescr(stream->spec->bitStream, &objDescr, 0);	/* read Object Descriptor */
  trackCount = objDescr.streamCount.value;

  for (x=0;x<trackCount;x++) {
    initESDescr(&(objDescr.ESDescriptor[x]));
    if (advanceESDescr(stream->spec->bitStream,objDescr.ESDescriptor[x], 0/*read*/, 0/*no systems-specific stuff*/) < 0) {
      DebugPrintf(1,"StreamFile:openRead(FL4): unrecoverable error in ES descriptor");	/* read ES Descriptor */
      return -2;
    }
  }

  align = 8 - BsCurrentBit(stream->spec->bitStream) % 8;
  if (align == 8) align = 0;
  BsGetBit(stream->spec->bitStream,(unsigned long*)&x,align) ;

  genericOpenRead(stream, trackCount); /* open all tracks, handle dependencies */

  return trackCount;
}


static int FL4getDependency(HANDLE_STREAMFILE stream, int trackID)
{
  if (stream->spec->objDescr->ESDescriptor[trackID]->streamDependence.value) {
    unsigned long es_id = stream->spec->objDescr->ESDescriptor[trackID]->dependsOn_Es_number.value;
    unsigned int i;
    int index=-1;
    for (i=0; i<stream->spec->objDescr->streamCount.value; i++) {
      if (es_id == stream->spec->objDescr->ESDescriptor[i]->ESNumber.value) { index = i;break; }
    }
    return index;
  } else {
    return -1;
  }
}


static int FL4openTrack(HANDLE_STREAMFILE stream, int trackID, int progIdx, int index)
{
  HANDLE_STREAMPROG prog = &stream->prog[progIdx];

  /* get dependencies */
  /*prog->dependencies[index] = stream->getDependency(stream, trackID);*/
  prog->dependencies[index] = index-1;
  prog->programData->spec->trackID[index] = stream->spec->objDescr->ESDescriptor[trackID]->ESNumber.value;

  /* get decoder configuration */
  prog->decoderConfig[index] = stream->spec->objDescr->ESDescriptor[trackID]->DecConfigDescr;

  /* FlexMux internal setup */
#ifdef VARIABLE_MAXTRACK
  {
    int s = sizeof(int)*prog->trackCount;
    prog->programData->spec->trackID                  = (int*)realloc(prog->programData->spec->trackID,s);
    prog->programData->spec->ALuseAccessUnitStartFlag = (int*)realloc(prog->programData->spec->ALuseAccessUnitStartFlag,s);
    prog->programData->spec->ALuseAccessUnitEndFlag   = (int*)realloc(prog->programData->spec->ALuseAccessUnitEndFlag,s);
    prog->programData->spec->ALseqNumLength           = (int*)realloc(prog->programData->spec->ALseqNumLength,s);
    if ((prog->programData->spec->ALuseAccessUnitStartFlag == NULL)||
        (prog->programData->spec->ALuseAccessUnitEndFlag == NULL)||
        (prog->programData->spec->ALseqNumLength == NULL)||
        (prog->programData->spec->trackID == NULL)) {
      CommonWarning("StreamFile:openTrack(FL4): Error in malloc");
      return -1;
    }
  }
#endif

  prog->programData->spec->ALuseAccessUnitStartFlag[index] = stream->spec->objDescr->ESDescriptor[trackID]->ALConfigDescriptor.useAccessUnitStartFlag.value;
  prog->programData->spec->ALuseAccessUnitEndFlag[index]   = stream->spec->objDescr->ESDescriptor[trackID]->ALConfigDescriptor.useAccessUnitEndFlag.value;
  prog->programData->spec->ALseqNumLength[index]           = stream->spec->objDescr->ESDescriptor[trackID]->ALConfigDescriptor.seqNumLength.value;

  return 0;
}


static int FL4getAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
     /* TODO:
        currently requires AUs to come in same order as requested, eg.:
        <track0,au0><track1,au0><track0,au1><track1,au1>... (for a file with two tracks)
     */
{
  unsigned long  index, length, n = 0, id = prog->programData->spec->trackID[trackNr];
  unsigned long  AUStartFlag=1,AUEndFlag=0;
  unsigned long  seq_number, dummy;
  unsigned long  totalLength = 0;
  unsigned char *destData;
  HANDLE_BSBITSTREAM infile = prog->fileData->spec->bitStream;
  
  if (BsEof(infile, 8)) return -2;

  do {
    /* read default AL-PDU header */
    BsGetBitAhead(infile,&index,8);
    if (index != id) {
      if (n == 0) {      /* if we have not read anything yet */
        DebugPrintf(1,"StreamFile:getAU(FL4): random AU order in FlexMux not correctly supported");
        id = index;      /* then we read whatever track we find */
      } else {           /* else we already have some other track */
        DebugPrintf(3,"StreamFile:getAU(FL4): AU ends at start of AU from different track");
        break;
      }
    }
    BsGetBit(infile,&index,8);

    BsGetBit(infile,&length,8);

    if (prog->programData->spec->ALuseAccessUnitStartFlag[trackNr]) {
      BsGetBit(infile,&AUStartFlag,1);
      if ((AUStartFlag)^(n==0)) {
        DebugPrintf(1,"StreamFile:getAU(FL4): AccessUnit StartFlag error");
      }
    }

    if (prog->programData->spec->ALuseAccessUnitEndFlag[trackNr])
      BsGetBit(infile,&AUEndFlag,1);

    if (prog->programData->spec->ALseqNumLength[trackNr] > 0)
      BsGetBit(infile,&seq_number,prog->programData->spec->ALseqNumLength[trackNr]);
    else
      BsGetBit(infile,&dummy,6);  /*6 padding bits (alm) */

    n++;

    totalLength += length << 3;
    StreamFile_AUresize(au, totalLength);
    destData = &au->data[(totalLength>>3)-length];
    for (;length;length--, destData++) {
      BsGetBitChar(infile, destData, 8);
    }

  } while (AUEndFlag!=1);

  return (n == 0);
}


static int FL4openWrite(HANDLE_STREAMFILE stream, int argc, char** argv)
{
  char info[256] = "";
  int result;

  /* - options parsing */

  result = commandline(stream, argc, argv);
  if (result) return result;

  /* - open bit stream file */
  stream->spec->bitStream = BsOpenFileWrite(stream->fileName,MP4_MAGIC,info);
  if (stream->spec->bitStream==NULL) {
    DebugPrintf(1,"StreamFile:openWrite(FL4): error opening bit stream for writing");
    return -1;
  }
  return 0;
}


static int FL4headerWrite(HANDLE_STREAMFILE stream)
{
  HANDLE_BSBITSTREAM tmpStream;
  HANDLE_BSBITBUFFER tmpBitBuffer;
  OBJECT_DESCRIPTOR  objDescr;
  ES_DESCRIPTOR* es;
  int progIdx;
  unsigned long track, trackCount=0;
  unsigned long ltmp;
  unsigned long align;

  /* 1) initialize the descriptors */
  initObjDescr(&objDescr);
  for (progIdx=0; progIdx<stream->progCount; progIdx++) {
    HANDLE_STREAMPROG prog = &stream->prog[progIdx];
#ifdef VARIABLE_MAXTRACK
    prog->programData->spec->ALuseAccessUnitStartFlag = (int*) malloc(prog->trackCount*sizeof(int));
    prog->programData->spec->ALuseAccessUnitEndFlag = (int*) malloc(prog->trackCount*sizeof(int));
    prog->programData->spec->ALseqNumLength = (int*) malloc(prog->trackCount*sizeof(int));
    prog->programData->spec->trackID = (int*) malloc(prog->trackCount*sizeof(int));
    if ((prog->programData->spec->ALuseAccessUnitStartFlag == NULL)||
        (prog->programData->spec->ALuseAccessUnitEndFlag == NULL)||
        (prog->programData->spec->ALseqNumLength == NULL)||
        (prog->programData->spec->trackID == NULL)) {
      CommonWarning("StreamFile:headerWrite(FL4): Error in malloc");
      return -1;
    }
#endif
    for (track=0;track<prog->trackCount;track++) {
      prog->programData->spec->trackID[track] = ++trackCount;
    }
  }
  presetObjDescr(&objDescr);
  objDescr.streamCount.value = trackCount;

  /* 2) write output bit stream header (FlexMux) */
  tmpBitBuffer = BsAllocBuffer(BITHEADERBUFSIZE);
  tmpStream = BsOpenBufferWrite(tmpBitBuffer);

  /* a) write an objectDescriptor */
  advanceODescr(tmpStream, &objDescr, 1);

  /* b) prepare and write ES_DESCRIPTORs */
  initESDescr(&es);
  for (progIdx=0; progIdx<stream->progCount; progIdx++) {
    HANDLE_STREAMPROG prog = &stream->prog[progIdx];
    for (track=0;track<prog->trackCount;track++) {
      presetESDescr(es, trackCount);

      es->ESNumber.value = prog->programData->spec->trackID[track];
      es->DecConfigDescr = prog->decoderConfig[track];

      es->ALConfigDescriptor.useAccessUnitStartFlag.value = stream->spec->useStartBit;
      es->ALConfigDescriptor.useAccessUnitEndFlag.value = stream->spec->useEndBit;
      es->ALConfigDescriptor.seqNumLength.value = 0;

      if (prog->dependencies[track] != -1) { /* this Elementary Stream has dependency */
        es->streamDependence.value = 1;
        es->dependsOn_Es_number.value = prog->programData->spec->trackID[prog->dependencies[track]];
      } else {                             /* this Elementary Stream has no dependency */
        es->streamDependence.value = 0;
        es->dependsOn_Es_number.value = 0;
      }

      if (advanceESDescr(tmpStream, es, 1/*write*/, 0/*no systems-specific stuff*/) != 0) {
        DebugPrintf(1,"StreamFile:headerWrite(FL4): unrecoverable error writing ES descriptor");
        return -1;
      }

      prog->programData->spec->ALuseAccessUnitStartFlag[track] = es->ALConfigDescriptor.useAccessUnitStartFlag.value;
      prog->programData->spec->ALuseAccessUnitEndFlag[track] = es->ALConfigDescriptor.useAccessUnitEndFlag.value;
      prog->programData->spec->ALseqNumLength[track] = es->ALConfigDescriptor.seqNumLength.value;
    }
  }

  /* c) write the wohle stuff into file */

  BsClose(tmpStream);
  ltmp=BsBufferNumBit(tmpBitBuffer)/8;
  align = 8 - BsBufferNumBit(tmpBitBuffer) % 8;
  if (align == 8) align = 0;
  if (align != 0) ltmp += 1;

  BsPutBit(stream->spec->bitStream,ltmp,32);
  if (BsPutBuffer(stream->spec->bitStream,tmpBitBuffer)) {
    DebugPrintf(1,"StreamFile:headerWrite(FL4): error writing bit stream header");
    return -1;
  }

  /* 3) clean up */
  BsPutBit(stream->spec->bitStream,0,align);
  BsFreeBuffer(tmpBitBuffer);

  return 0;
}


static int FL4putAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  unsigned long totalLength;
  unsigned long length,AUStartFlag,AUEndFlag;
  unsigned long maxBytes=255;
  unsigned char *srcPtr = au->data;
  HANDLE_BSBITSTREAM outfile = prog->fileData->spec->bitStream;

  AUStartFlag=1;
  AUEndFlag=0;

  totalLength = (au->numBits+7) >> 3;

  /* write blocks of maxBytes bytes each separated with AL-stuff */
  do {
    length = totalLength;
    if (length > maxBytes) length = maxBytes;
    totalLength -= length;

    AUEndFlag=(length!=maxBytes);

    /* put track index, length of AU, ... */
    BsPutBit(outfile,prog->programData->spec->trackID[trackNr],8);
    BsPutBit(outfile,length,8);
    if (prog->programData->spec->ALuseAccessUnitStartFlag[trackNr])
      BsPutBit(outfile,AUStartFlag,1);
    if (prog->programData->spec->ALuseAccessUnitEndFlag[trackNr])
      BsPutBit(outfile,AUEndFlag,1);
    if (prog->programData->spec->ALseqNumLength[trackNr] > 0)
      BsPutBit(outfile,0,prog->programData->spec->ALseqNumLength[trackNr]);
    else
      BsPutBit(outfile,0,6);  /*6 padding bits (alm)  (?) */

    /* put AU into track */
    for (;length;length--,srcPtr++) {
      unsigned long tmp = *srcPtr;
      BsPutBit(outfile, tmp, 8);
    }

    AUStartFlag=0;
  } while ( !AUEndFlag );

  return 0;
}


static int FL4close(HANDLE_STREAMFILE stream)
{
#ifdef VARIABLE_MAXTRACK
  int progIdx;
  for (progIdx=0; progIdx<stream->progCount; progIdx++) {
    HANDLE_STREAMPROG prog = &stream->prog[progIdx];
    if (prog->programData->spec->trackID) free(prog->programData->spec->trackID);
    if (prog->programData->spec->ALuseAccessUnitStartFlag) free(prog->programData->spec->ALuseAccessUnitStartFlag);
    if (prog->programData->spec->ALuseAccessUnitEndFlag) free(prog->programData->spec->ALuseAccessUnitEndFlag);
    if (prog->programData->spec->ALseqNumLength) free(prog->programData->spec->ALseqNumLength);
  }
#endif
  return BsClose(stream->spec->bitStream);
}


/* --------------------------------------------------- */
/* ---- Constructor                               ---- */
/* --------------------------------------------------- */


int FL4initStream(HANDLE_STREAMFILE stream)
{
  stream->initProgram=FL4initProgram;
  stream->openRead=FL4openRead;
  stream->getDependency=FL4getDependency;
  stream->openTrack=FL4openTrack;
  stream->openWrite=FL4openWrite;
  stream->headerWrite=FL4headerWrite;
  stream->close=FL4close;
  stream->getAU=FL4getAccessUnit;
  stream->putAU=FL4putAccessUnit;

  if ((stream->spec = (struct tagStreamSpecificInfo*)malloc(sizeof(struct tagStreamSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initStream: error in malloc");
    return -1;
  }
  memset(stream->spec, 0, sizeof(struct tagStreamSpecificInfo));
  return 0;
}

void FL4showHelp( void )
{
  printf(MODULE_INFORMATION);
  commandline(NULL,0,NULL);
}
