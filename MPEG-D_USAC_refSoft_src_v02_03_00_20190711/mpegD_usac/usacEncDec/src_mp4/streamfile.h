/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer

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
Header file: streamfile.h

**********************************************************************/

#ifndef _streamfile_h_
#define _streamfile_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj_descr.h"           /* structs */

#define MAXPROG (8)
#define MAXTRACK (50)


/* ---- enum & consts ---- */
typedef enum {
  FILETYPE_AUTO = 0,
  FILETYPE_MP4,
  FILETYPE_LATM,
  FILETYPE_FL4,
  FILETYPE_RAW,
  FILETYPE_UNKNOWN
} StreamFileType;

typedef enum  {
  SAMPLE_GROUP_NONE,
  SAMPLE_GROUP_ROLL,
  SAMPLE_GROUP_PROL
} SAMPLE_GROUP_ID;

/* ---- Handles ---- */
typedef struct tagStreamFile*       HANDLE_STREAMFILE;
typedef struct StreamProgram*       HANDLE_STREAMPROG;
typedef struct StreamAccessUnit*    HANDLE_STREAM_AU;

/* ---- Structs ---- */
struct StreamProgram {
  unsigned long          trackCount;
  unsigned long          allTracks;
  long                   dependencies[MAXTRACK];
  DEC_CONF_DESCRIPTOR    decoderConfig[MAXTRACK];

  int                    sbrRatioIndex[MAXTRACK];
  SAMPLE_GROUP_ID        sampleGroupID[MAXTRACK];
  int                    isIPF[MAXTRACK];
  unsigned int           auCnt[MAXTRACK];

  /* internal information */
  struct tagProgramData* programData;
  HANDLE_STREAMFILE      fileData;
};

struct StreamAccessUnit {
  unsigned long          numBits;
  unsigned char*         data;
};


/* ---- functions ---- */

/* StreamFileShowHelp()
     Show information about the known file formats and their options
*/
void StreamFileShowHelp( void );


/* ---- HANDLE_STREAMFILE related ---- */

/* StreamFileOpenRead(), StreamFileOpenWrite() 
     filename: prepare and open a stream at this path
     type    : one of the above; force a type or try to detect automatically
     options : string containing requested low-level options, NULL for defaults
     return a valid HANDLE_STREAMFILE
*/
HANDLE_STREAMFILE StreamFileOpenRead(char *filename, StreamFileType type);
HANDLE_STREAMFILE StreamFileOpenWrite(char *filename, StreamFileType type, char *options);

/* StreamFileClose()
     stream: a valid handle
     return 0 on success
*/
int StreamFileClose(HANDLE_STREAMFILE stream);


/* ---- HANDLE_STREAMPROG related ---- */

/* StreamFileAddProgram()
     stream: a valid handle
     return a reference to the new program for editing
*/
HANDLE_STREAMPROG StreamFileAddProgram(HANDLE_STREAMFILE stream);

/* StreamFileGetProgram()
     stream: a valid handle
     progNr: number of the desired program (starting at 0)
     return the specified program for further use
*/
HANDLE_STREAMPROG StreamFileGetProgram(HANDLE_STREAMFILE stream, int progNr);

/* StreamFileGetProgramCount()
     stream: a valid handle
     return the number of programs in the stream
*/
int StreamFileGetProgramCount(HANDLE_STREAMFILE stream);

/* StreamFileFixProgram()
     prog: a valid handle
     fix some known values in the program (dependsOnCoreCoder flags and similar)
     return the number of fixed values
*/
int StreamFileFixProgram(HANDLE_STREAMPROG prog);


/* ---- track related ---- */
int StreamFileAddTrackToProg(HANDLE_STREAMPROG prog);
int StreamFileSetupTrack(HANDLE_STREAMPROG prog, int track, int aot, int bitrate, int avgBitrate, int samplerate, int channels);

/* ---- HANDLE_STREAM_AU related ---- */

/* StreamFileAllocateAU()
     numBits: initial size of the access unit
     return a new and valid access unit
*/
HANDLE_STREAM_AU StreamFileAllocateAU(unsigned long numBits);

/* StreamFile_AUresize()
     au: access unit to resize
     numBits: new size of the access unit
     return 1 on success, 0 if an error occured
*/
int StreamFile_AUresize(HANDLE_STREAM_AU au, const long numBits);

/* StreamFileFreeAU()
     free the memory of a access unit
*/
void StreamFileFreeAU(HANDLE_STREAM_AU au);

/* StreamGetAccessUnit()
     get next access unit from program
     prog   : a valid handle where to get the AU from
     trackNr: specify track inside the program
     au     : valid access unit; au->data will be realloc()ed
     return 0: OK; -1: error; -2: EOF
*/
int StreamGetAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au);

/* StreamPutAccessUnit()
     append access unit in program
     prog   : a valid handle where to put the AU
     trackNr: specify track inside the program
     au     : a valid access unit containing the data
     return error code or 0
*/
int StreamPutAccessUnit(HANDLE_STREAMPROG stream, int trackNr, HANDLE_STREAM_AU au);

/* StreamAUsPerFrame()
     get number of access units to complete a full frame
     prog   : a valid handle where to query info from
     trackNr: specify track inside the program
     return error code or the number of access units to complete a full frame
*/
int StreamAUsPerFrame(HANDLE_STREAMPROG prog, int track);

/* StreamFileGetEditlist()
     get editlistdata from MP4FF
     return error code or 0
*/
int StreamFileGetEditlist(HANDLE_STREAMPROG stream, int trackNr, double *startOffset, double *playTime);

int StreamFileSetEditlist(HANDLE_STREAMPROG stream, int trackNr, int bSampleGroupElst, int rollDistance);

int StreamFileSetEditlistValues(HANDLE_STREAMPROG stream, int nDelay, int sampleRateOut, int originalSampleCount, 
                                int originalSamplingRate, int sampleGroupStartOffset, int sampleGroupLength);

#endif
