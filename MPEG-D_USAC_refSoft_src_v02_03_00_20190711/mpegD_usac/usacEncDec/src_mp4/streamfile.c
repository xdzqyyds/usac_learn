/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer

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
Source file: streamfile.c

 

Required modules:
common_m4a.o            common module
bitstream.o             bit stream module
streamfile_*.o          fileformat implementations

**********************************************************************/

/*
  To add another file format:
  - add it in the enum in streamfile.h
  - add extension into struct StreamFileExtTable

  Functions of the "subsystems":

  XXXinitStream:
     input: stream
     effects: allocate format specific data and setup function pointers of a HANDLE_STREAMFILE
  XXXshowHelp:
     effects: show information about the subsystem and specific parameters


  Functions referenced via pointers in HANDLE_STREAMFILE (see streamfile_intern.h)

  XXXinitProgram:
     input: program
     effects: allocate format specific data of a HANDLE_STREAMPROG

  XXXopenRead:
     input: stream with fileName set
     effects: prepare everything for XXXgetAccessUnit
              initialize stream->prog and stream->progCount
     return: trackCount, -1: file is not type XXX, <-1: FAILED
  XXXgetDependency:
     input: stream; trackID to check
     return: -3: FAILED, -2: track disabled, -1: no dependency, else a trackID
  XXXopenTrack:
     input: stream; trackID to open; prog and index where to open the track
     side-effects: prepare everything for XXXgetAccessUnit
                   place decConfig and dependecies of stream
     return: 0: OK, else FAILED

  XXXopenWrite:
     input: stream with fileName set
     side-effects: prepare everything for XXXheaderWrite
     return: 0 for OK, else FAILED
  XXXheaderWrite:
     input: stream with properly set trackCount, dependencies and decConfig
     side-effects: setup everything necessary for XXXputAccessUnit

  XXXgetAccessUnit:
     called after XXXopenRead, get next access unit
     input: trackNr and stream
     return: 0:OK, -1: error, -2: EOF
     side-effects: access unit is stored in the stream-handle
  XXXputAccessUnit
     called after XXXheaderWrite,
     input: properly set up stream (XXXheaderWrite), trackNumber and a bitstream to place as AU

  XXXclose:
     close bitstreams
*/

#undef DEBUG_STREAMFILE

#define AU_BUFFER_SIZE 128


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flex_mux.h"            /* audio object types */
#include "common_m4a.h"          /* common module */
#include "cmdline.h"             /* for parsing commandline */

#include "streamfile.h"          /* public functions */
#include "streamfile_helper.h"   /* helper functions */
#include "streamfile_diagnose.h" /* diagnosis functions */

/* file formats: */
#include "streamfile_mp4.h"
#include "streamfile_fl4.h"
#include "streamfile_latm.h"
#include "streamfile_raw.h"



/* ---- collection of file types ---- */

struct {
  char* ext;                                    /* filetype extension (obligatory) */
  StreamFileType type;                          /* filetype id (obligatory) */
  int (*constructor)(HANDLE_STREAMFILE stream); /* constructor preparing functions and spec-stuff in HANDLE_STREAMFILE (obligatory) */
  void (*showHelp)();                           /* function providing additional information (NULL to skip) */
} StreamFileExt[] = {
  {".mp4", FILETYPE_MP4, MP4initStream, MP4showHelp},
  {".fl4", FILETYPE_FL4, FL4initStream, FL4showHelp},
  {".ass", FILETYPE_LATM,LATMinitStream,LATMshowHelp},
  {".raw", FILETYPE_RAW, RAWinitStream, RAWshowHelp},
  {NULL,   FILETYPE_AUTO,NULL,NULL}
};


/* ---- setup function pointers ---- */

static int StreamFileSetupFunctions(HANDLE_STREAMFILE stream)
     /* return: 0: OK; else FAILED */
{
  int type_idx;
  if (stream->spec) free(stream->spec);
  if (stream->type == FILETYPE_AUTO) return 1;

  /* look for type in the table */
  for (type_idx=0; StreamFileExt[type_idx].ext != NULL; type_idx++) {
    if (stream->type == StreamFileExt[type_idx].type) break;
  }

  /* call constructor if available */
  if ((StreamFileExt[type_idx].ext == NULL)||(StreamFileExt[type_idx].constructor == NULL)) {
    CommonWarning("StreamFile: format not implemented");
    return -1;
  } else {
    return StreamFileExt[type_idx].constructor(stream);
  }
}


/* ---- get file type from filename extension ---- */

static StreamFileType fileTypeFromName(char *filename)
{
  int nameLen = strlen(filename);
  int startPos;
  int l;
  int ext_idx;
  int found_ext = -1;
  int done;

  /* go through the table of known extensions */
  for (startPos=nameLen-1; startPos>0; startPos--) {
    done = 1;
    /* look for the extension in the filename */
    for (ext_idx=0; StreamFileExt[ext_idx].ext != NULL; ext_idx++) {
      l = strlen(StreamFileExt[ext_idx].ext);
      if (l > nameLen-startPos) done = 0;
      if (l == nameLen-startPos) {
        if (!strcmp(StreamFileExt[ext_idx].ext,&filename[startPos])) { found_ext = ext_idx;break; }
      }
    }
    if (done||(found_ext!=-1)) break;
  }

  if (found_ext==-1) return FILETYPE_AUTO;
  else return StreamFileExt[found_ext].type;
}


/* --------------------------------------------------- */
/* ---- helpers for streams                       ---- */
/* --------------------------------------------------- */


static HANDLE_STREAMFILE newStreamFile(char* filename, StreamFileType type)
{
  HANDLE_STREAMFILE sf;

  sf = (HANDLE_STREAMFILE) malloc(sizeof(struct tagStreamFile));
  if (sf==NULL) {
    CommonWarning("StreamFile:open: error in malloc");
    return NULL;
  }
  memset(sf, 0, sizeof(struct tagStreamFile));

  sf->type = type;
  sf->status = STATUS_INVALID;
  sf->fileName = filename;
  sf->providesIndependentReading = 0;

  StreamFileSetupFunctions(sf);
  return sf;
}

static int createAUBuffers(HANDLE_STREAMFILE stream, int size)
{
  int err = 0, i;
  unsigned int j;

  for (i=0; i<stream->progCount; i++) {
    HANDLE_STREAMPROG prog = &stream->prog[i];
    for (j=0; j<prog->trackCount; j++) {
      prog->programData->timeThisFrame[j] = 0;
      prog->programData->fifo_buffer[j] = FIFObufferCreate(size);
      if (prog->programData->fifo_buffer[j]==NULL) err = -1;
    }
  }

  return err;
}

static void freeStreamFile(HANDLE_STREAMFILE* stream)
{
  if ((*stream)->spec) free((*stream)->spec);
  free((*stream));
  *stream=NULL;
}


/* --------------------------------------------------- */
/* ---- public stuff                              ---- */
/* --------------------------------------------------- */


void StreamFileShowHelp( void )
{
  int type_idx;

  /* go through all types in the table */
  for (type_idx=0; StreamFileExt[type_idx].ext != NULL; type_idx++) {
    printf(" * associated with extension '%s':\n",StreamFileExt[type_idx].ext);
    if (StreamFileExt[type_idx].showHelp == NULL) {
      printf("no further help available\n\n");
    } else {
      StreamFileExt[type_idx].showHelp();
    }
  }
}


HANDLE_STREAMFILE StreamFileOpenRead(char *filename, StreamFileType type)
{
  int check, tmp;
  HANDLE_STREAMFILE stream = newStreamFile(filename, type);
  if (stream==NULL) return stream;

  for (check=(int)type; check!=FILETYPE_UNKNOWN; check++) {
    stream->type = (StreamFileType)check;
    if (StreamFileSetupFunctions(stream) != 0) continue;
    if (stream->openRead==NULL) continue;
    if ( (tmp=stream->openRead(stream)) >= 0) { /* opening the file succeded */
      setStreamStatus(stream, STATUS_PREPARE_READ);
      break;
    }
    if (type != FILETYPE_AUTO) break;
  }

  if (stream->status == STATUS_INVALID) {
    if (check == FILETYPE_UNKNOWN)
      CommonWarning("StreamFile:openRead: can not determine file format");
    else
      CommonWarning("StreamFile:openRead: error opening file");
    freeStreamFile(&stream);
    return NULL;
  }

  /* prepare internal buffering */
  if (stream->providesIndependentReading == 0) {
    if (createAUBuffers(stream, AU_BUFFER_SIZE)) {
      CommonWarning("StreamFile:openRead: could not create AU buffers");
      StreamFileClose(stream);
      return NULL;
    }
  }

  /* get sample durations and start diagnostics */
  getAllSampleDurations(stream);

  return stream;
}


HANDLE_STREAMFILE StreamFileOpenWrite(char *filename, StreamFileType type, char *options)
{
  HANDLE_STREAMFILE stream;

  /* look at the options for this file */
  int num;
  char ** list = CmdLineParseString (options," ",&num);

  if (type==FILETYPE_AUTO) type = fileTypeFromName(filename);
  if (type==FILETYPE_AUTO) {
    CommonWarning("StreamFile:openWrite: can't autodetect write format");
    return NULL;
  }

  stream = newStreamFile(filename, type);
  if (stream==NULL) return stream;

  if (stream->openWrite==NULL) {
    CommonWarning("StreamFile:openWrite: can't write this format");
    freeStreamFile(&stream);
  } else {
    if (stream->openWrite(stream, num, list)) {
      CommonWarning("StreamFile:openWrite: can't open file for writing");
      freeStreamFile(&stream);
    } else {
      stream->status=STATUS_PREPARE_WRITE;
    }
  }

  CmdLineParseFree(list);

  return stream;
}


static int StreamFileWriteHeader(HANDLE_STREAMFILE stream)
{
  int err = 0;

  if (stream == NULL) {
    return -1;
  }
  if (stream->status == STATUS_WRITING) {
    /* header already written */
    return -1;
  }
  if (stream->status != STATUS_PREPARE_WRITE) {
    CommonWarning("StreamFile:writeHeader: can not write header to this stream");
    return -1;
  }

  /* get sample durations and start diagnostics */
  getAllSampleDurations(stream);
  startStreamDiagnose(stream);

  /* write the headers */
  if ((err = stream->headerWrite(stream))) {
    CommonWarning("StreamFile:writeHeader: error writing headers");
    return err;
  }

  /* update status */
  setStreamStatus(stream,STATUS_WRITING);

  return err;
}


int StreamFileClose(HANDLE_STREAMFILE stream)
{
  int i;

  if (stream == NULL) {
    return -1;
  }
  if (stream->status == STATUS_PREPARE_WRITE) {
    CommonWarning("StreamFile:close: close uninitialized stream");
  }

  if (stream->close(stream)) {
    CommonWarning("StreamFile:close: error while closing stream");
  }

  stream->status=STATUS_INVALID;

  for (i=0;i<stream->progCount;i++) {
    closeProgram(&(stream->prog[i]));
  }

  freeStreamFile(&stream);

  return 0;
}

HANDLE_STREAMPROG StreamFileAddProgram(HANDLE_STREAMFILE stream)
{
  HANDLE_STREAMPROG prog;
  if (stream == NULL) {
    CommonWarning("StreamFile:addProg: stream not initialized");
    return NULL;
  }
  switch (stream->status) {
  case STATUS_PREPARE_WRITE:
    break;
  case STATUS_WRITING:
    CommonWarning("StreamFile:addProg: stream-header already fixed");
    return NULL;
  default:
    CommonWarning("StreamFile:addProg: stream is not writable");
    return NULL;
  }

  prog = newStreamProg(stream);
  prog->programData->status = STATUS_WRITING;
  return prog;
}

HANDLE_STREAMPROG StreamFileGetProgram(HANDLE_STREAMFILE stream, int progNr)
{
  if (stream == NULL) {
    CommonWarning("StreamFile:getProg: stream not initialized");
    return NULL;
  }
  if ((stream->status != STATUS_READING)&&(stream->status != STATUS_PREPARE_READ)) {
    CommonWarning("StreamFile:getProg: stream not readable");
    return NULL;
  }
  if (progNr>=stream->progCount) {
    CommonWarning("StreamFile:getProg: program does not exist");
    return NULL;
  }

  return (&stream->prog[progNr]);
}

int StreamFileGetProgramCount(HANDLE_STREAMFILE stream)
{
  if (stream == NULL) {
    CommonWarning("StreamFile:getNumProg: stream not initialized");
    return -1;
  }
  return stream->progCount;
}

int StreamFileFixProgram(HANDLE_STREAMPROG prog)
{ return StreamDiagnoseAndFixProgram(prog); }


int StreamFileAddTrackToProg(HANDLE_STREAMPROG prog)
{
  int track = prog->trackCount;
  prog->trackCount++;
  prog->allTracks++;

  prog->dependencies[track] = track-1;  /* -1 first track! */  
  
  return track;
}

int StreamFileSetupTrack(HANDLE_STREAMPROG prog, int track, int aot, int bitrate, int avgBitrate, int samplerate, int channels)
{
  const int sfreq[] = { 96000,  88200,  64000,  48000,  44100,  32000,  24000,  22050,  16000,  12000,  11025,  8000,  7350, -1, -1, -1, -1 };  
  int idx;
  AUDIO_SPECIFIC_CONFIG *asc;

  setupDecConfigDescr(&prog->decoderConfig[0]);
  presetDecConfigDescr(&prog->decoderConfig[0]);

  asc = &prog->decoderConfig[0].audioSpecificConfig;
  asc->audioDecoderType.value  = aot;
  asc->samplingFrequency.value = samplerate;
  /* get sampleFrequencyIndex */
  for (idx=0; (sfreq[idx]!=samplerate) && (sfreq[idx]!=-1); idx++);
  if (idx == -1) return 0;
  asc->samplingFreqencyIndex.value = idx;
  asc->channelConfiguration.value = channels;                   


  prog->decoderConfig[0].bufferSizeDB.value = 8192;
  prog->decoderConfig[0].maxBitrate.value = bitrate;
  prog->decoderConfig[0].avgBitrate.value = avgBitrate;
  /*  mpeg4prog->decoderConfig[0].specificInfoFlag = 1;*/
  /*  mpeg4prog->decoderConfig[0].specificInfoLength;*/
  
  return 1;
}



/* various buffering helpers for READING access units */
static int realGetAU(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  int err;
  DebugPrintf(7,"StreamFile:getAU: real get AU called, %i",trackNr);
  if ((err = prog->fileData->getAU(prog, trackNr, au))) {
    if (err == -2) {
      DebugPrintf(1,"StreamFile:getAU: EOF");
    } else {
      CommonWarning("StreamFile:getAU: error reading access unit");
    }
  } else {
    prog->programData->timeThisFrame[trackNr] += prog->programData->timePerAU[trackNr];
    StreamDiagnoseAccessUnit(prog, trackNr, au);
  }

  return err;
}
static int bufferPop(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  int err;
  HANDLE_STREAM_AU tmpAU;
  if (!(tmpAU = FIFObufferPop(prog->programData->fifo_buffer[trackNr]))) {
    return -1;
  }
  err = StreamFile_AUcopyResize(au, tmpAU->data, tmpAU->numBits);
  StreamFileFreeAU(tmpAU);
  return err;
}
static int bufferPush(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{
  if (FIFObufferPush(prog->programData->fifo_buffer[trackNr], au)) {
    DebugPrintf(2,"StreamFile:buffering: could not save access unit, discarding");
    return -1;
  }
  return 0;
}
static int bufferRemainingFrame(HANDLE_STREAMFILE stream, unsigned int referenceTime)
{
  int err = 0, i;
  unsigned int j;
  int done, frame_complete;
  HANDLE_STREAMPROG prog;
  DebugPrintf(7, "buffering remaining frame");
  do {
    done = 1; frame_complete = 1;
    for (i=0; i<stream->progCount; i++) {
      prog = &stream->prog[i];
      for (j=0; j<prog->trackCount; j++) {
        DebugPrintf(7,"checking track %i: %i < %i (%i)?",j,prog->programData->timeThisFrame[j],referenceTime,prog->programData->timePerFrame);
        if ( prog->programData->timeThisFrame[j] < referenceTime ) {
          int tmp_err;
          HANDLE_STREAM_AU tmpAU = StreamFileAllocateAU(0);
          tmp_err = realGetAU(prog, j, tmpAU);
          if (!tmp_err) bufferPush(prog, j, tmpAU);
          else err=tmp_err;
          if (prog->programData->timeThisFrame[j] <= referenceTime) done = 0;
        }
        if (prog->programData->timeThisFrame[j] < prog->programData->timePerFrame) frame_complete=0;
      }
    }
  } while ((!done)&&(!err));
  if (frame_complete) {
    for (i=0; i<stream->progCount; i++) {
      prog = &stream->prog[i];
      for (j=0; j<prog->trackCount; j++) {
        prog->programData->timeThisFrame[j] -= prog->programData->timePerFrame;
      }
    }
  }
  return err;
}




int StreamGetAccessUnit(HANDLE_STREAMPROG stream, int trackNr, HANDLE_STREAM_AU au)
{
  int err = 0, gotOne = 0;

  if (stream == NULL) {
    return -1;
  }
  if ((unsigned)trackNr >= stream->trackCount) {
    CommonWarning("StreamFile:getAU: reading from non-existing track");
    return -1;
  }
  if (stream->programData == NULL) {
    CommonWarning("StreamFile:getAU: reading from uninitialized program");
    return -1;
  }
  if (stream->programData->status == STATUS_PREPARE_READ) {
    startStreamDiagnose(stream->fileData);
    setStreamStatus(stream->fileData,STATUS_READING);
  }
  if (stream->programData->status != STATUS_READING) {
    CommonWarning("StreamFile:getAU: reading from non-readable program");
    return -1;
  }
  if (stream->fileData == NULL) {
    CommonWarning("StreamFile:getAU: reading from uninitialized stream");
    return -1;
  }
  if (stream->fileData->status != STATUS_READING) {
    CommonWarning("StreamFile:getAU: reading from non-readable stream");
    return -1;
  }
  if (au == NULL) {
    CommonWarning("StreamFile:getAU: no valid place to put access unit");
    return -1;
  }

  if (stream->fileData->providesIndependentReading == 0) {     /* use internal buffering */
    if ((err = bufferPop(stream, trackNr, au))==0) { /* got buffered AU? */
      DebugPrintf(6,"StreamFile:getAU: got AU from buffer, %i\n",trackNr);
      gotOne = 1;
    } else {                                         /* ...else we still have to do something */
      DebugPrintf(7,"check whether to start buffering: %i, %i\n",stream->programData->timeThisFrame[trackNr],stream->programData->timePerFrame);
      if ( /* stream->programData->timeThisFrame[trackNr] == stream->programData->timePerFrame*/1 ) {
        /* already got one AU from this track in this frame */
        DebugPrintf(6,"StreamFile:getAU: start buffering\n");
        err = bufferRemainingFrame(stream->fileData, stream->programData->timeThisFrame[trackNr]);
        DebugPrintf(6,"StreamFile:getAU: done buffering\n");
        if ((err)&&(err!=-2)) {
          CommonWarning("StreamFile:getAU: error while buffering the remaining unread frame");
          err = 0;
        }
      }
    }
  }

  if (gotOne==0) {          /* get new AU from stream */
    err = realGetAU(stream, trackNr, au);
  }

  if (err==0) {
    unsigned long length = (au->numBits+7) >> 3;

    DebugPrintf(3,"StreamFile:getAU: successfully got %i bytes",length);
#ifdef DEBUG_STREAMFILE
    {
      unsigned char* tmp = au->data;
      unsigned long i;
      for (i=0;i<length;i++) {
        DebugPrintf(8,"  got [%4i]: 0x%02x",i,tmp[i]);
      }
    }
#endif
  }

  return err;
}


int StreamPutAccessUnit(HANDLE_STREAMPROG stream, int trackNr, HANDLE_STREAM_AU au)
{
  int err;

  if (stream == NULL) {
    return -1;
  }
  if ((unsigned)trackNr >= stream->trackCount) {
    CommonWarning("StreamFile:putAU: write to non-existing track");
    return -1;
  }
  if (stream->programData == NULL) {
    CommonWarning("StreamFile:putAU: write to uninitialized program");
    return -1;
  }
  if (stream->programData->status != STATUS_WRITING) {
    CommonWarning("StreamFile:putAU: write to non-writable program");
    return -1;
  }
  if (stream->fileData == NULL) {
    CommonWarning("StreamFile:putAU: write to uninitialized stream");
    return -1;
  }
  if (stream->fileData->status == STATUS_PREPARE_WRITE) {
    StreamFileWriteHeader(stream->fileData);
  }
  if (stream->fileData->status != STATUS_WRITING) {
    CommonWarning("StreamFile:putAU: write to non-writable stream");
    return -1;
  } 
  if (au == NULL) {
    CommonWarning("StreamFile:putAU: no vaild access unit");
    return -1;
  }

  if ((err = stream->fileData->putAU(stream, trackNr, au))) {
    CommonWarning("StreamFile:getAU: error writing access unit");
  }

  if (err==0) {
    unsigned long length = (au->numBits+7) >> 3;

    DebugPrintf(3,"StreamFile:putAU: successfully put %i bytes",length);
#ifdef DEBUG_STREAMFILE
    {
      unsigned char* tmp = au->data;
      unsigned long i;
      for (i=0;i<length;i++) {
        DebugPrintf(8,"  put [%4i]: 0x%02x",i,tmp[i]);
      }
    }
#endif
  }

  return err;
}


int StreamAUsPerFrame(HANDLE_STREAMPROG prog, int trackNr)
{
  if (prog == NULL) {
    return -1;
  }
  if ((unsigned)trackNr >= prog->trackCount) {
    CommonWarning("StreamFile:AUsPerFrame: selected non-existing track");
    return -1;
  }
  if (prog->programData == NULL) {
    CommonWarning("StreamFile:AUsPerFrame: selected uninitialized program");
    return -1;
  }
  if (prog->programData->timePerAU[trackNr] <= 0) {
    CommonWarning("StreamFile:AUsPerFrame: selected incorrectly initialized program");
    return -1;
  }

  return (prog->programData->timePerFrame / prog->programData->timePerAU[trackNr]);
}


HANDLE_STREAM_AU StreamFileAllocateAU(unsigned long numBits)
{
  HANDLE_STREAM_AU ret = (HANDLE_STREAM_AU)malloc(sizeof(struct StreamAccessUnit));
  if (ret == NULL) {
    CommonWarning("StreamFile: allocAU: malloc returned NULL");
    return ret;
  }
  memset(ret,0,sizeof(struct StreamAccessUnit));

  StreamFile_AUresize(ret, numBits);

  return ret;
}

void StreamFileFreeAU(HANDLE_STREAM_AU au)
{
  if (au == NULL) {
    return;
  }
  StreamFile_AUfree(au);
  free(au);
}


int StreamFileGetEditlist(HANDLE_STREAMPROG stream, int trackNr, double *startOffset, double *playTime){

  int err = 0;

  if (!err) {
    if ((unsigned)trackNr >= stream->trackCount) {
      CommonWarning("StreamFile:getAU: reading from non-existing track");
      err = -1;
    }
  }

  if (!err) {
    if ((startOffset == NULL) || (playTime    == NULL)) {
      err = -1;
    }
  }

  if (!err) {
    if (stream->fileData->getEditlist) {
      err = stream->fileData->getEditlist(stream, trackNr, startOffset, playTime);
    } else {
      *startOffset =  0.0;
      *playTime    = -1.0;
    }
  }

  return err;
}

int StreamFileSetEditlist(HANDLE_STREAMPROG stream, int trackNr, int bSampleGroupElst, int rollDistance){

  int err = 0;

  if (!err) {
    if (stream->fileData->setEditlist) {
      err = stream->fileData->setEditlist(stream, trackNr, bSampleGroupElst, rollDistance);
    }
  }

  return err;
}

int StreamFileSetEditlistValues(HANDLE_STREAMPROG stream, int nDelay, int sampleRateOut, int originalSampleCount, 
                               int originalSamplingRate, int sampleGroupStartOffset, int sampleGroupLength){

  int err = 0;

  if (!err) {
    if (stream->fileData->setEditlistValues) {
      err = stream->fileData->setEditlistValues(stream, nDelay, sampleRateOut, originalSampleCount, 
                                                originalSamplingRate, sampleGroupStartOffset, sampleGroupLength);
    }
  }
  
  return err;
}
