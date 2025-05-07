/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer helper functions

This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

and edited by

Manuela Schinn (Fraunhofer IIS)

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
Source file: streamfile_helper.c

 

Required modules:
common_m4a.o            common module

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_m4a.h"          /* common module */

#include "streamfile.h"          /* public functions */
#include "streamfile_helper.h"



/* --------------------------------------------------- */
/* ---- helpers for whole streams                 ---- */
/* --------------------------------------------------- */


void setStreamStatus(HANDLE_STREAMFILE stream, StreamStatus status)
{
  int progIdx;
  stream->status=status;
  for (progIdx=0; progIdx<stream->progCount; progIdx++)
    stream->prog[progIdx].programData->status=status;
}


/* --------------------------------------------------- */
/* ---- helpers for handling programs             ---- */
/* --------------------------------------------------- */

HANDLE_STREAMPROG newStreamProg(HANDLE_STREAMFILE stream)
{
  HANDLE_STREAMPROG ret = &stream->prog[stream->progCount];

  if (stream->progCount >= MAXPROG) {
    CommonWarning("StreamFile:addProg: stream already has maximum number of programs");
    return NULL;
  }

  if ((ret->programData = (struct tagProgramData*)malloc(sizeof(struct tagProgramData))) == NULL) {
    CommonWarning("StreamFile:addProg: error in malloc");
    return NULL;
  }
  memset(ret->programData,0,sizeof(struct tagProgramData));

  if (stream->initProgram(ret) != 0) {
    free(ret->programData);
    return NULL;
  }

  ret->fileData=stream;

  stream->progCount++;

  return ret;
}

static int freeAUBuffers(HANDLE_STREAMPROG prog)
{
  int err = 0;
  unsigned int j;

  for (j=0; j<prog->trackCount; j++) {
    HANDLE_STREAM_AU tmpAU;
    if (prog->programData->fifo_buffer[j]==NULL) continue;
    while ((tmpAU = (HANDLE_STREAM_AU)FIFObufferPop(prog->programData->fifo_buffer[j]))) {
      StreamFileFreeAU(tmpAU);
    }
    FIFObufferFree(prog->programData->fifo_buffer[j]);
  }

  return err;
}

void closeProgram(HANDLE_STREAMPROG prog)
{
  if (prog->programData) {
    if (prog->programData->spec)
      free(prog->programData->spec);
    prog->programData->spec=NULL;
    freeAUBuffers(prog);
    free(prog->programData);
  }
  prog->programData=NULL;
  prog->fileData=NULL;
}



int openTrackInProg(HANDLE_STREAMFILE stream, int trackID, int prog)
{
  int err;
  err = stream->openTrack(stream, trackID, prog, stream->prog[prog].trackCount);
  stream->prog[prog].allTracks++;
  if (err != 0) { 
    return 0;
  }
  stream->prog[prog].trackCount++;
  return 1;
}


/* --------------------------------------------------- */
/* ---- helper for stream dependencies            ---- */
/* --------------------------------------------------- */


int genericOpenRead(HANDLE_STREAMFILE stream, unsigned long trackCount)
{
  int err = 0;
  unsigned long x;
  long tmp;
  int* trackProg;
  int done=0;

  trackProg = (int*)malloc(sizeof(int)*trackCount);
  if (trackProg == NULL) {
    CommonWarning("StreamFile:openRead: malloc failed");
    return -1;
  }
  for (x=0; x<trackCount; x++) {
    trackProg[x] = -1;
  }
  while (!done) {
    done = 1;
    for (x=0; x<trackCount; x++) {
      if (trackProg[x] == -1) { /* if track not assigned yet */
        tmp = stream->getDependency(stream,x);
        switch (tmp) {
        case -3: /* error in getDependency */
          err=-1;
          goto bail;
        case -2: /* track not available */
          trackProg[x] = -2;
          break;
        case -1: /* track depends on nothing */
          newStreamProg(stream);
          trackProg[x] = stream->progCount-1;
          if(!openTrackInProg(stream, x, trackProg[x])) {
            CommonWarning("StreamFile:openRead: audioObjectType of base layer not implemented");
            err=-1;
            goto bail;
          }
          break;
        default: /* check dependency */
          if (tmp == (signed)x) { CommonWarning("StreamFile:openRead: track depends on itself");err=-1;goto bail; }
          if (trackProg[tmp] == -2) { CommonWarning("StreamFile:openRead: Track depends on disabled track");err=-1;goto bail; }
          if (trackProg[tmp] == -1) { done = 0; }
          else {
            trackProg[x] = trackProg[tmp];
            openTrackInProg(stream, x, trackProg[x]);
          }
        }
      }
    }
  }
 bail:
  free(trackProg);
  return err;
}


/* --------------------------------------------------- */
/* ---- helper for access units                   ---- */
/* --------------------------------------------------- */


int StreamFile_AUresize(HANDLE_STREAM_AU au, const long numBits)
{
  int numBytes = ((numBits+7)>>3);

  if (numBytes > 0) {
    if (au->data == NULL)
      au->data = (unsigned char*) malloc(numBytes);
    else
      au->data = (unsigned char*) realloc(au->data, numBytes);
  } else {
    if (au->data != NULL) {
      free(au->data);
      au->data=NULL;
    }
  }
  au->numBits = numBits;

  if (au->numBits != 0) {
    return (au->data==NULL);
  } else {
    return 0;
  }
}


int StreamFile_AUcopyResize(HANDLE_STREAM_AU au, const unsigned char* src, const long numBits)
{
  if (StreamFile_AUresize(au, numBits)) {
    CommonWarning("StreamFile: AU: error while allcating memory");
    return -1;
  }

  if (((numBits+7)>>3) > 0)
    memcpy(au->data, src, (numBits+7)>>3 );

  return 0;
}


void StreamFile_AUfree(HANDLE_STREAM_AU au)
{
  au->numBits = 0;
  if (au->data == NULL) return;
  free(au->data);
  au->data = NULL;
}



/* --------------------------------------------------- */
/* ---- FIFO helper                               ---- */
/* --------------------------------------------------- */


struct tagFIFObuffer {
  int readIdx, writeIdx; /* readIdx: next item to read; writeIdx: last item written */
  int size;

  void **content;
};

FIFO_BUFFER FIFObufferCreate(int size)
{
  FIFO_BUFFER ret;
  if (size<=0) return NULL;

  ret = (FIFO_BUFFER)malloc(sizeof(struct tagFIFObuffer));
  if (ret==NULL) return ret;

  ret->size = size;
  ret->readIdx = 0;
  ret->writeIdx = 0;

  ret->content = (void**)malloc(ret->size*sizeof(void*));
  if (ret->content==0) {
    free(ret);
    return NULL;
  }

  return ret;
}

void FIFObufferFree(FIFO_BUFFER fifo)
{
  if (fifo->content) free(fifo->content);
  free(fifo);
}

int FIFObufferPush(FIFO_BUFFER fifo, void *item)
{
  int newWriteIdx = (fifo->writeIdx+1) % fifo->size;
  if (newWriteIdx == fifo->readIdx) return -1; /* FIFO is full */

  fifo->content[fifo->writeIdx] = item;
  fifo->writeIdx = newWriteIdx;
  return 0;
}

void* FIFObufferPop(FIFO_BUFFER fifo)
{
  void *ret;
  if (fifo->readIdx == fifo->writeIdx) return NULL; /* FIFO is empty */
  ret = fifo->content[fifo->readIdx];
  fifo->readIdx = (fifo->readIdx+1) % fifo->size;
  return ret;
}

int FIFObufferLength(FIFO_BUFFER fifo)
{
  int ret = fifo->writeIdx - fifo->readIdx;
  if (ret < 0) ret += fifo->size;
  return ret;
}

void* FIFObufferGet(FIFO_BUFFER fifo, int idx)
{
  int tmp_readIdx = (fifo->readIdx + idx) % fifo->size;

  if (idx > fifo->size) return NULL;
  if (fifo->readIdx <= fifo->writeIdx) {
    if (tmp_readIdx < fifo->writeIdx) return fifo->content[tmp_readIdx];
    else return NULL;
  } else {
    if ((tmp_readIdx >= fifo->readIdx)||(tmp_readIdx < fifo->writeIdx)) return fifo->content[tmp_readIdx];
    else return NULL;
  }
}

