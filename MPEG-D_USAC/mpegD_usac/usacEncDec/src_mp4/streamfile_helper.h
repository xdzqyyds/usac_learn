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
Source file: streamfile_helper.h

**********************************************************************/

#ifndef _STREAMFILE_HELPER_INCLUDED_
#define _STREAMFILE_HELPER_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum { STATUS_INVALID=0, STATUS_READING, STATUS_WRITING, STATUS_PREPARE_WRITE, STATUS_PREPARE_READ } StreamStatus;


/* handle for a FIFO buffer */
typedef struct tagFIFObuffer* FIFO_BUFFER;


/* struct for a whole bitstream file with multiple programs */
struct tagStreamFile {
  char*                         fileName;
  StreamFileType                type;
  StreamStatus                  status;
  int                           providesIndependentReading;

  int                           progCount;
  struct StreamProgram          prog[MAXPROG];

  struct tagStreamSpecificInfo* spec; /* specific format dependent info */

  /* functions */
  int (*initProgram)(HANDLE_STREAMPROG prog);

  int (*openRead)(HANDLE_STREAMFILE stream);
  int (*openWrite)(HANDLE_STREAMFILE stream, int argc, char** argv);
  int (*headerWrite)(HANDLE_STREAMFILE stream);
  int (*close)(HANDLE_STREAMFILE stream);

  int (*getDependency)(HANDLE_STREAMFILE stream, int trackID);
  int (*openTrack)(HANDLE_STREAMFILE stream, int trackID, int prog, int index);

  int (*getAU)(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU auStream);
  int (*putAU)(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU auStream);
  int (*getEditlist)(HANDLE_STREAMPROG prog, int trackNr, double *startOffset, double *playTime);
  int (*setEditlist)(HANDLE_STREAMPROG stream, int trackNr, int bSampleGroupElst, int rollDistance);
  int (*setEditlistValues)(HANDLE_STREAMPROG stream, int nDelay, int sampleRateOut, int originalSampleCount, 
                           int originalSamplingRate, int sampleGroupStartOffset, int sampleGroupLength);
};

/* handle for buffer simulation */
typedef struct t_fullness_sim *HANDLE_BUFFER_SIMULATION;

/* struct for internal variables necessary for one program */
struct tagProgramData {
  StreamStatus                status;
  unsigned long               sampleDuration[MAXTRACK];
  unsigned long               timePerFrame;
  unsigned long               timeThisFrame[MAXTRACK];
  unsigned long               timePerAU[MAXTRACK];
  FIFO_BUFFER                 fifo_buffer[MAXTRACK];
  HANDLE_BUFFER_SIMULATION    buffer_sim;

  struct tagProgSpecificInfo* spec; /* specific format dependent info */
};


/* helpers for the whole stream */
void setStreamStatus(HANDLE_STREAMFILE stream, StreamStatus status);

/* helpers for programs */
HANDLE_STREAMPROG newStreamProg(HANDLE_STREAMFILE stream);
void closeProgram(HANDLE_STREAMPROG prog);
int openTrackInProg(HANDLE_STREAMFILE stream, int trackID, int prog);

/* helper for dependencies */
int genericOpenRead(HANDLE_STREAMFILE stream, unsigned long trackCount);

/* helper for AUs */
int StreamFile_AUcopyResize(HANDLE_STREAM_AU au, const unsigned char* src, const long numBits);
void StreamFile_AUfree(HANDLE_STREAM_AU au);


/* FIFO helper */

FIFO_BUFFER FIFObufferCreate(int size);
void FIFObufferFree(FIFO_BUFFER fifo);

int FIFObufferPush(FIFO_BUFFER fifo, void *item);
void* FIFObufferPop(FIFO_BUFFER fifo);

void* FIFObufferGet(FIFO_BUFFER fifo, int idx);
int FIFObufferLength(FIFO_BUFFER fifo);

#endif
