/********************************************************************** 
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Ralph Sperschneider (Fraunhofer IIS)
Thomas Schaefer (Fraunhofer IIS)
Olaf Kaehler (Fraunhofer IIS)

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

Copyright (c) 1996, 1997, 1998.



Source file: bitstream.c


Required modules:
common.o                common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
06-jun-96   HP    added buffers, ASCII-header and BsGetBufferAhead()
07-jun-96   HP    use CommonWarning(), CommonExit()
11-jun-96   HP    ...
13-jun-96   HP    changed BsGetBit(), BsPutBit(), BsGetBuffer(),
                  BsGetBufferAhead(), BsPutBuffer()
14-jun-96   HP    fixed bug in BsOpenFileRead(), read header
18-jun-96   HP    fixed bug in BsGetBuffer()
26-aug-96   HP    CVS
23-oct-96   HP    free for BsOpenFileRead() info string
07-nov-96   HP    fixed *HANDLE_BSBITSTREAM info bug
15-nov-96   HP    changed int to long where required
                  added BsGetBitChar(), BsGetBitShort(), BsGetBitInt()
                  improved file header handling
04-dec-96   HP    fixed bug in BsGetBitXxx() for numBit==0
23-dec-96   HP    renamed mode to write in *HANDLE_BSBITSTREAM
10-jan-97   HP    added BsGetBitAhead(), BsGetSkip()
17-jan-97   HP    fixed read file buffer bug
30-jan-97   HP    minor bug fix in read bitstream file magic string
19-feb-97   HP    made internal data structures invisible
04-apr-97   BT/HP added BsGetBufferAppend()
07-max-97   BT    added BsGetBitBack()
14-mrz-98   sfr   added CreateEscInstanceData(), BsReadInfoFile(), BsGetBitEP(),
                        BsGetBitEP()
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP    trying to clean up a bit ...
22-jan-99   HP    added "-" stdin/stdout support
                  variable file buffer size for streaming
05-feb-99   HP    added fflush() after fwrite()
12-feb-99   BT/HP updated ...
19-apr-99   HP    cleaning up some header files again :-(
14-mar-03   OK    added BsRWBitWrapper()
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"          /* bit stream module */
#include "common_m4a.h"         /* common module */

#include "allHandles.h"
#include "block.h"
#include "buffer.h"
#include "obj_descr.h"

#include "interface.h"
#include "statistics.h"         /* undesired, but ... */
#include "resilience.h"         /* really undesired, but but ... */
#include "mp4au.h"

/* ---------- declarations ---------- */

struct tagBsBitBufferStruct	/* bit buffer */
{
  unsigned char* data;          /* data bits */
  long           numBit;        /* number of bits in buffer */
  long           size;          /* buffer size in bits */
};

struct tagBsBitStreamStruct	/* bit stream */
{
  FILE *	file;		/* file or NULL for buffer i/o */
  int		write;		/* 0=read  1=write */
  long		streamId;	/* stream id (for debug) */
  char*		info;		/* info string (for read file) */
  char*         fileName;       
  HANDLE_BSBITBUFFER 	buffer[2];	/* bit buffers */
				/* (buffer[1] is only used for read file) */
  long		currentBit;	/* current bit position in bit stream */
				/* (i.e. number of bits read/written) */
  long		numByte;	/* number of bytes read/written (only file) */
};

#define MIN_FILE_BUF_SIZE          1024 /* min num bytes in file buffer */
#define HEADER_BUF_SIZE            2048 /* max size of ASCII header */
#define COMMENT_LENGTH             1024 /* max nr of comment characters 
                                           in predefinition file */
#define MAX_BS_ERRORS              100
#define NR_OF_ASSIGNMENT_SCHEMES   3

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define BYTE_NUMBIT 8           /* bits in byte (char) */
#define LONG_NUMBIT 32          /* bits in unsigned long */
#define bit2byte(a) (((a)+BYTE_NUMBIT-1)/BYTE_NUMBIT)
#define byte2bit(a) ((a)*BYTE_NUMBIT)


/* ---------- declarations (structures) ---------- */


typedef struct {
  unsigned int escInstanceUsed;           /* signals, whether this esc instance is used */
  unsigned int bitsInEscInstance;         /* number of bits in a class of a frame */
  unsigned int curBitPos;                 /* "pointer" of bit-position in a class */
  unsigned int crcResultInEscInstance;    /* signals a crc error within this class */
  unsigned int readErrorInEscInstance;    /* signals, if a read error occured within this class */
} SINGLE_ESC_INSTANCE_DATA;

typedef struct tagEscInstanceData {
  FILE*                     infoFilePtr;                                                  /* file pointer to EP decoder info file */
  unsigned int              crcResultInFrame;                                             /* signals a crc error within the frame */
  unsigned int              readErrorInFrame;                                             /* signals, if a read error occured within the frame */
  unsigned int              frameLoss;                                                    /* signals, if a frame got lost (bitsInFrame=0) */
  unsigned int              epDebugLevel;
  unsigned int              escForTnsUsed;                                                /* signals, whether the optional esc for tns is used */
  unsigned int              escForRvlcUsed;                                               /* signals, whether the optional esc for rvlc is used */
  unsigned int              numBitsInFrame;                                               /* number of bits in the audio frame */
  SINGLE_ESC_INSTANCE_DATA  escInstance[MAX_NR_OF_INSTANCE_PER_ESC][MAX_NR_OF_ESC];       /* class handler */
  unsigned int              escAssignment[MAX_ELEMENTS][NR_OF_ASSIGNMENT_SCHEMES];        /* array with the ep classes of the bitsream elements */
} TAG_ESC_INSTANCE_DATA;



/* ---------- variables ---------- */

static int  BSdebugLevel  = 0;                  /* debug level */
static int  BSaacEOF      = 0;
static long BSbufSizeByte = MIN_FILE_BUF_SIZE;  /* num bytes file buf */
static long BSstreamId    = 0;                  /* stream id counter */
static int  BSerrorCount  = 0;

static int  assignmentScheme = 0;          /* bitstream element identifier */
static int  Channel          = 1;


/* ---------- internal functions ---------- */


/* BsReadFile() */
/* Read one buffer from file. */

int GetInstanceOfEsc_(int channel, int assignmentScheme)
{
  int instanceOfEsc = 0;
  switch (assignmentScheme) {
  case 0: /* mono */
    instanceOfEsc = 0;
    break;
  case 1: /* stereo, common_window == 0 */
  case 2: /* stereo, common_window == 1*/
    if ( channel == 1 ) { /* left channel : 1 */
      instanceOfEsc = 0;
    }
    else {                /* right channel : 2 */
      instanceOfEsc = 1;
    }
    break;
  default:
    CommonExit (1, "GetInstanceOfEsc: assignmentScheme = %d (this case should not occur)", assignmentScheme);
  }
  return ( instanceOfEsc );
}

int GetInstanceOfEsc(int channel)
{
  return GetInstanceOfEsc_(channel, assignmentScheme);
}

static int BsReadFile ( HANDLE_BSBITSTREAM stream)             /* in: stream
                                                            returns: 0=OK  1=error */
{
  long numByte;
  long numByteRead;
  long curBuf;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
           stream->streamId,stream->numByte,stream->currentBit);

  if (feof(stream->file))
    return 0;

  numByte = bit2byte(stream->buffer[0]->size);
  if (stream->numByte % numByte != 0) {
    CommonWarning("BsReadFile: bit stream buffer error");
    return 1;
  }
  curBuf = (stream->numByte / numByte) % 2;
    numByteRead = fread(stream->buffer[curBuf]->data,sizeof(char),numByte,
                        stream->file);
  if (ferror(stream->file)) {
    CommonWarning("BsReadFile: error reading bit stream file");
    return 1;
  }
  stream->numByte += numByteRead;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: numByte=%ld  numByteRead=%ld\n",numByte,numByteRead);

  return 0;
}


/* BsWriteFile() */
/* Write one buffer to file. */

static int BsWriteFile (
                        HANDLE_BSBITSTREAM stream)            /* in: stream */
                                /* returns: 0=OK  1=error */
{
  long numByte;
  long numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
           stream->streamId,stream->numByte,stream->currentBit);

  if (stream->numByte % bit2byte(stream->buffer[0]->size) != 0) {
    CommonWarning("BsWriteFile: bit stream buffer error");
    return 1;
  }
  numByte = bit2byte(stream->currentBit) - stream->numByte;
  numByteWritten = fwrite(stream->buffer[0]->data,sizeof(char),numByte,
                          stream->file);
  fflush(stream->file);
  if (numByteWritten != numByte || ferror(stream->file)) {
    CommonWarning("BsWriteFile: error writing bit stream file");
    return 1;
  }
  stream->numByte += numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: numByteWritten=%ld\n",numByteWritten);

  return 0;
}


/* BsReadAhead() */
/* If possible, ensure that the next numBit bits are available */
/* in read file buffer. */

static int BsReadAhead (
                        HANDLE_BSBITSTREAM stream,            /* in: bit stream */
                        long numBit)                    /* in: number of bits */
                                /* returns: 0=OK  1=error */
{
  if (stream->write==1 || stream->file==NULL)
    return 0;

  if (bit2byte(stream->currentBit+numBit) > stream->numByte)
    if (BsReadFile(stream)) {
      CommonWarning("BsReadAhead: error reading bit stream file");
      return 1;
    }
  return 0;
}


/* BsCheckRead() */
/* Check if numBit bits could be read from stream. */

static int BsCheckRead (
                        HANDLE_BSBITSTREAM stream,            /* in: bit stream */
                        long numBit)                    /* in: number of bits */
                                /* returns: */
                                /*  0=numBit bits could be read */
                                /*    (or write mode) */
                                /*  1=numBit bits could not be read */
{
  if (stream->write == 1)
    return 0;           /* write mode */
  else
    if (stream->file == NULL)
      return (stream->currentBit+numBit > stream->buffer[0]->numBit) ? 1 : 0;
    else
      return (bit2byte(stream->currentBit+numBit) > stream->numByte) ? 1 : 0;
}


/* BsReadByte() */
/* Read byte from stream. */

static int BsReadByte (
                       HANDLE_BSBITSTREAM stream,           /* in: stream */
                       unsigned long*     data,             /* out: data (max 8 bit, right justified) */
                       int                numBit)           /* in: num bits [1..8] */
                                /* returns: num bits read or 0 if error */
{
  long numUsed;
  long idx;
  long buf;

  if (stream->file!=NULL && stream->currentBit==stream->numByte*BYTE_NUMBIT)
    if (BsReadFile(stream)) {
      if ( ! BSaacEOF || BSdebugLevel > 0 )
        CommonWarning("BsReadByte: error reading bit stream file");
      return 0;
    }

  if (BsCheckRead(stream,numBit)) {
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsReadByte: not enough bits left in stream");
    return 0;
  }

  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  buf = (stream->currentBit / BYTE_NUMBIT /
         bit2byte(stream->buffer[0]->size)) % 2;
  numUsed = stream->currentBit % BYTE_NUMBIT;
  *data = (stream->buffer[buf]->data[idx] >> (BYTE_NUMBIT-numUsed-numBit)) &
    ((1<<numBit)-1);
  stream->currentBit += numBit;
  return numBit;
}


/* BsWriteByte() */
/* Write byte to stream. */

static int BsWriteByte (
                        HANDLE_BSBITSTREAM stream,            /* in: stream */
                        unsigned long data,             /* in: data (max 8 bit, right justified) */
                        int numBit)                     /* in: num bits [1..8] */
                                /* returns: 0=OK  1=error */
{
  long numUsed,idx;

  if (stream->file == NULL &&
      stream->buffer[0]->numBit+numBit > stream->buffer[0]->size) {
    CommonWarning("BsWriteByte: not enough bits left in buffer");
    return 1;
  }
  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  numUsed = stream->currentBit % BYTE_NUMBIT;
  if (numUsed == 0)
    stream->buffer[0]->data[idx] = 0;
  stream->buffer[0]->data[idx] |= (data & ((1<<numBit)-1)) <<
    (BYTE_NUMBIT-numUsed-numBit);
  stream->currentBit += numBit;
  if (stream->file==NULL)
    stream->buffer[0]->numBit = stream->currentBit;
  if (stream->file!=NULL && stream->currentBit%stream->buffer[0]->size==0)
    if (BsWriteFile(stream)) {
      CommonWarning("BsWriteByte: error writing bit stream file");
      return 1;
    }
  return 0;
}


/* ---------- functions ---------- */


/* BsInit() */
/* Init bit stream module. */

void BsInit ( long maxReadAhead, /* in: max num bits to be read ahead 
                                    (determines file buffer size) 
                                    (0 = default) */
              int debugLevel,    /* in: debug level 
                                    0=off  1=basic  2=medium  3=full */
              int aacEOF )       /* in: AAC eof detection (default = 0) */
{
  if (maxReadAhead)
    BSbufSizeByte = max(4,bit2byte(maxReadAhead));
  else
    BSbufSizeByte = MIN_FILE_BUF_SIZE;
  BSdebugLevel = debugLevel;
  BSaacEOF = aacEOF;
  if (BSdebugLevel >= 1 )
    printf("BsInit: debugLevel=%d  aacEOF=%d  bufSizeByte=%ld\n",
           BSdebugLevel,BSaacEOF,BSbufSizeByte);
}


/* BsOpenFileRead() */
/* Open bit stream file for reading. */

HANDLE_BSBITSTREAM BsOpenFileRead (
                             char *fileName,            /* in: file name */
                                /*     "-": stdin */
                             char *magic,                       /* in: magic string */
                                /*     or NULL (no ASCII header in file) */
                             char **info)                       /* out: info string */
                                /*      or NULL (no info string in file) */
                                /* returns: */
                                /*  bit stream (handle) */
                                /*  or NULL if error */
{
  HANDLE_BSBITSTREAM stream;
  char header[HEADER_BUF_SIZE];
  int tmp = 0;
  long i,len;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileRead: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
           fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL)
      printf("magic=\"%s\"\n",magic);
    else
      printf("no header\n");
  }

  if ((stream=(HANDLE_BSBITSTREAM )malloc(sizeof(*stream))) == NULL)
    CommonExit(1,"BsOpenFileRead: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));
  stream->buffer[1]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;
 
  if (strcmp(fileName,"-")){
    stream->file = fopen(fileName,"rb");
    stream->fileName=(char*)malloc(strlen(fileName)+1);
    strcpy(stream->fileName,fileName);
  }  else{
    stream->file = stdin;
    stream->fileName=(char*)malloc(strlen("STDIN")+1);
    strcpy(stream->fileName,"STDIN");
  }
  if (stream->file == NULL) {
    CommonWarning("BsOpenFileRead: error opening bit stream file %s",
                  fileName);
    BsFreeBuffer(stream->buffer[0]);
    BsFreeBuffer(stream->buffer[1]);
    free(stream);
    return NULL;
  }  
  
  if (magic != NULL) {
    /* read ASCII header */
    /* read magic string */
    
    len = strlen(magic);
    if (len >= HEADER_BUF_SIZE) {
      CommonWarning("BsOpenFileRead: magic string too long");
      BsClose(stream);
      return NULL;
    }
    for (i=0; i<len; i++)
      header[i] = tmp = fgetc(stream->file);
    if (tmp == EOF) {                   /* EOF requires int (not char) */
      CommonWarning("BsOpenFileRead: "
                    "error reading bit stream file (header)");
      BsClose(stream);
      return NULL;
    }
    header[i] = '\0';
    
    if (strcmp(header,magic) != 0) {
      CommonWarning("BsOpenFileRead: magic string error "
                    "(found \"%s\", need \"%s\")",header,magic);
      BsClose(stream);
      return NULL; 
    }
    
    if (info != NULL) {
      /* read info string */
      i = 0;
      while ((header[i]=tmp=fgetc(stream->file)) != '\0') {
        if (tmp == EOF) {               /* EOF requires int (not char) */
          CommonWarning("BsOpenFileRead: "
                        "error reading bit stream file (header)");
          BsClose(stream);
          return NULL;
        }
        i++;
        if (i >= HEADER_BUF_SIZE) {
          CommonWarning("BsOpenFileRead: info string too long");
          BsClose(stream);
          return NULL;
        }
      }

      if (BSdebugLevel >= 1)
        printf("BsOpenFileRead: info=\"%s\"\n",header);

      if ((stream->info=(char*)malloc((strlen(header)+1)*sizeof(char)))
          == NULL)
        CommonExit(1,"BsOpenFileRead: memory allocation error (info)");
      strcpy(stream->info,header);
      *info = stream->info;
    }
  }

  /* init buffer */
  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}

char *BsGetFileName ( HANDLE_BSBITSTREAM bitStream)

{
  return (bitStream->fileName);
}

/* BsOpenFileWrite() */
/* Open bit stream file for writing. */


HANDLE_BSBITSTREAM BsOpenFileWrite (
                              char *fileName,           /* in: file name */
                                /*     "-": stdout */
                              char *magic,                      /* in: magic string */
                                /*     or NULL (no ASCII header) */
                              char *info)                       /* in: info string */
                                /*     or NULL (no info string) */
                                /* returns: */
                                /*  bit stream (handle) */
                                /*  or NULL if error */
{
  HANDLE_BSBITSTREAM stream;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileWrite: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
           fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL) {
      printf("magic=\"%s\"\n",magic);
      if (info != NULL)
        printf("BsOpenFileWrite: info=\"%s\"\n",info);
      else
        printf("BsOpenFileWrite: no info\n");
    }
    else
      printf("no header\n");
  }

  if ((stream=(HANDLE_BSBITSTREAM )malloc(sizeof(*stream))) == NULL)
    CommonExit(1,"BsOpenFileWrite: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  if (strcmp(fileName,"-")) {
    stream->file = fopen(fileName,"wb");
    stream->fileName=(char*)malloc(strlen(fileName)+1);
    strcpy(stream->fileName, fileName);
  }  else {
    stream->file = stdout;
    stream->fileName=(char*)malloc(strlen("STDOUT")+1);
    strcpy(stream->fileName, "STDOUT");
  }
  if (stream->file == NULL) {
    CommonWarning("BsOpenFileWrite: error opening bit stream file %s",
                  fileName);
    BsFreeBuffer(stream->buffer[0]);
    free(stream);
    return NULL;
  }

  if (magic!=NULL) {
    /* write ASCII header */
    /* write magic string */
    if (fputs(magic,stream->file) == EOF) {
      CommonWarning("BsOpenFileWrite: error writing bit stream file (header)");
      BsClose(stream);
      return NULL;
    }
    if (info!=NULL) {
      /* write info string */
      if (fputs(info,stream->file) == EOF) {
        CommonWarning("BsOpenFileWrite: "
                      "error writing bit stream file (header)");
        BsClose(stream);
        return NULL;
      }
      if (fputc('\0',stream->file) == EOF) {
        CommonWarning("BsOpenFileWrite: "
                      "error writing bit stream file (header)");
        BsClose(stream);
        return NULL;
      }
    }
  }

  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}


/* BsOpenBufferRead() */
/* Open bit stream buffer for reading. */

HANDLE_BSBITSTREAM BsOpenBufferRead (
                               HANDLE_BSBITBUFFER buffer)             /* in: bit buffer */
                                /* returns: */
                                /*  bit stream (handle) */
{
  HANDLE_BSBITSTREAM stream;

  if(buffer == NULL) /* gracefully exit invalid bitstream */
    CommonExit(1,"BsOpenBufferRead: Invalid handle");

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferRead: id=%ld  bufNumBit=%ld  bufSize=%ld  "
           "bufAddr=0x%lx\n",
           BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(HANDLE_BSBITSTREAM )malloc(sizeof(*stream))) == NULL)
    CommonExit(1,"BsOpenBufferRead: memory allocation error");

  stream->file = NULL;
  stream->fileName = NULL;
  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  stream->currentBit = 0;
  stream->numByte = (buffer->numBit + 7) / 8;
  return stream;
}


/* BsOpenBufferWrite() */
/* Open bit stream buffer for writing. */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

HANDLE_BSBITSTREAM BsOpenBufferWrite (
                                HANDLE_BSBITBUFFER buffer)            /* in: bit buffer */
                                /* returns: */
                                /*  bit stream (handle) */
{
  HANDLE_BSBITSTREAM stream;

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferWrite: id=%ld  bufNumBit=%ld  bufSize=%ld  "
           "bufAddr=0x%lx\n",
           BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(HANDLE_BSBITSTREAM )malloc(sizeof(*stream))) == NULL)
    CommonExit(1,"BsOpenBufferWrite: memory allocation error");

  stream->file = NULL;
  stream->fileName = NULL;
  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  BsClearBuffer(buffer);
  stream->currentBit = 0;
  
  return stream;
}


/* BsCurrentBit() */
/* Get number of bits read/written from/to stream. */

long BsCurrentBit (
                   HANDLE_BSBITSTREAM stream)         /* in: bit stream */
                                /* returns: */
                                /*  number of bits read/written */
{
  return stream->currentBit;
}


/* BsEof() */
/* Test if end of bit stream file occurs within the next numBit bits. */

int BsEof (
           HANDLE_BSBITSTREAM stream,         /* in: bit stream */
           long numBit)                 /* in: number of bits ahead scanned for EOF */
                                /* returns: */
                                /*  0=not EOF  1=EOF */
{
  int eof;

  if (BSdebugLevel >= 2)
    printf("BsEof: %s  id=%ld  curBit=%ld  numBit=%ld\n",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,stream->currentBit,numBit);

  if (stream->file != NULL && numBit > stream->buffer[0]->size)
    CommonExit(1,"BsEof: "
               "number of bits to look ahead too high (%ld)",numBit);

  if (BsReadAhead(stream,numBit+1)) {
    CommonWarning("BsEof: error reading bit stream");
    eof = 0;
  }
  else
    eof = BsCheckRead(stream,numBit+1);

  if (BSdebugLevel >= 2)
    printf("BsEof: eof=%d\n",eof);

  return eof;
}


static void BsRemoveBufferOffset (HANDLE_BSBITBUFFER buffer, long startPosBit)
{
  int           bitsToCopy;
  HANDLE_BSBITSTREAM   offset_stream;
  HANDLE_BSBITBUFFER   helpBuffer;

  /* open bit stream buffer for reading */
  offset_stream = BsOpenBufferRead(buffer);

  /* create help buffer */
  helpBuffer = BsAllocBuffer(buffer->size);
  

  /* read bits from bit stream to help buffer */
  offset_stream->currentBit =  startPosBit;
  bitsToCopy = buffer->numBit - startPosBit;
  if (BsGetBuffer(offset_stream, helpBuffer, bitsToCopy))
    CommonExit(1, "BsRemoveBufferOffset: error reading bit stream");

  /* close bit stream (no remove) */
  BsCloseRemove(offset_stream, 0);

  /* memcpy the offset free data from help buffer to buffer */
  memcpy(buffer->data, helpBuffer->data, bit2byte(buffer->size));

  /* free help buffer */
  BsFreeBuffer(helpBuffer);

  buffer->numBit = bitsToCopy;
}


/* BsCloseRemove() */
/* Close bit stream. */

int BsCloseRemove (
                   HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                   int remove)                  /* in: if opened with BsOpenBufferRead(): */
                                /*       0 = keep buffer unchanged */
                                /*       1 = remove bits read from buffer */
                                /* returns: */
                                /*  0=OK  1=error */
{
  int returnFlag = 0;
  int tmp,i;
  long startPosBit;

  if ((stream->file != NULL) && (BSdebugLevel >= 1) )
    printf("BsClose: %s  %s  id=%ld  curBit=%ld\n",
           (stream->write)?"write":"read",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,stream->currentBit);

  if (stream->file != NULL) {
    if (stream->write == 1)
      /* flush buffer to file */
      if (BsWriteFile(stream)) {
        CommonWarning("BsClose: error writing bit stream");
        returnFlag = 1;
      }

    BsFreeBuffer(stream->buffer[0]);
    if (stream->write == 0)
      BsFreeBuffer(stream->buffer[1]);
    
    if (stream->file!=stdin && stream->file!=stdout)
      if (fclose(stream->file)) {
        CommonWarning("BsClose: error closing bit stream file");
        returnFlag = 1;
      }
  }
  else {
    if (stream->write == 0 && remove) {
      /* remove all completely read bytes from buffer */
      tmp = stream->currentBit/8;
      for (i=0; i<((stream->buffer[0]->size+7)>>3)-tmp; i++)
        stream->buffer[0]->data[i] = stream->buffer[0]->data[i+tmp];
      startPosBit = stream->currentBit - tmp*8;
      if ((startPosBit>7) || (startPosBit<0)) {
        CommonExit(1,"BsClose: Error removing bit in buffer");        
      }
      stream->buffer[0]->numBit -= tmp*8;      

      /* better located here ???   HP/BT 990520 */
      if (stream->buffer[0]->numBit <= startPosBit) {
        stream->buffer[0]->numBit=0;
        startPosBit=0;
      }

      /* remove remaining read bits from buffer          */
      /* usually we do not have remaining bits in buffer 
         reply: that not really true, eg. HVXC frames are not byte aligned, thereofore you have remaining bits after every second frame, 
         BT*/
      if (startPosBit != 0) {
        BsRemoveBufferOffset(stream->buffer[0], startPosBit);
        if ((stream->currentBit - startPosBit) < 0)
          CommonExit(1,"BsClose: Error decreasing currentBit");
        else
          stream->currentBit -= startPosBit;
      }
    }
  }

  if (stream->info != NULL) {
    free(stream->info);
  }
  free(stream); 
  return returnFlag;
}


/* BsClose() */
/* Close bit stream. */

int BsClose (
             HANDLE_BSBITSTREAM stream)               /* in: bit stream */
                                /* returns: */
                                /*  0=OK  1=error */
{
  return BsCloseRemove(stream,0);
}


/* BsRWBitWrapper() */
/* Read bits from or write bits to bit stream (depending on WriteFlag) */
/* (Current position in bit stream is advanced !!!) */

int BsRWBitWrapper(HANDLE_BSBITSTREAM stream,	/* in: bit stream */
                          unsigned long *data,	/* out: bits read/write */
                          int numBit,		/* in: num bits to read */
                          int WriteFlag)	/* in: 0: read, 1: write
                                                   returns: number of bits read/written */
{
  if (WriteFlag == 1) {
    BsPutBit(stream,*data,numBit);
  } else if (WriteFlag == 0) {
    BsGetBit(stream,data,numBit);
  }
  return numBit;
}


/* BsGetBit() */
/* Read bits from bit stream. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBit (
              HANDLE_BSBITSTREAM stream,              /* in: bit stream */
              unsigned long *data,              /* out: bits read */
                                /*      (may be NULL if numBit==0) */
              int numBit)                       /* in: num bits to read */
                                /*     [0..32] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  int           num;
  int           maxNum;
  int           curNum;
  unsigned long bits;

  if (BSdebugLevel >= 3)
    printf("BsGetBit: %s  id=%4ld  numBit=%2d  curBit=%4ld  ",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit,stream->currentBit);

  if (stream->write != 0)
    CommonExit(1,"BsGetBit: stream not in read mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsGetBit: number of bits out of range (%d)",numBit);

  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;

  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    if (BsReadByte(stream,&bits,curNum) != curNum) {
      if ( ! BSaacEOF || BSdebugLevel > 0  ) 
        CommonWarning("BsGetBit: error reading bit stream");

      if ( BSerrorCount++ > MAX_BS_ERRORS) {
        CommonExit(1,"BsGetBit: maximum error count reached");
      }
      if ( BSaacEOF  ) {        
        return -1;/* end of stream */
      } else {
        return 1; 
      }
    }
    *data |= bits<<(numBit-num-curNum);
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  if (BSdebugLevel >= 3) {
    /* printf("BsGetBit: data=0x%lx\n",*data); */
    printf(" data=0x%lx\n",*data);
  }

  return 0;
}


/* this function is mainly for debugging purpose, */
/* you can call it from the debugger */
long int BsGetBitBack (
                       HANDLE_BSBITSTREAM stream,             /* in: bit stream */
                       int numBit)                      /* in: num bits to read */
                                /*     [0..32] */
                                /* returns: */
                                /*  if numBit is positive
                                      return the last numBits bit from stream
                                    else
                                      return the next -numBits from stream 
                                    stream->currentBit is always unchanged */
{
  int           num;
  int           maxNum;
  int           curNum;
  unsigned long bits;
  long int      data;
  int           rewind = 0;

  if (BSdebugLevel >= 3)
    printf("BsGetBitBack: %s  id=%ld  numBit=%d  curBit=%ld\n",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit,stream->currentBit);

  /*   if (stream->write != 0) */
  /*     CommonWarning("BsGetBitBack: stream not in read mode"); */
  if (numBit<-32 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsGetBitBack: number of bits out of range (%d)",numBit);

  data = 0;
  if (numBit == 0)
    return 0;
  if (numBit > 0)
    stream->currentBit-=numBit;
  else {
    rewind = 1;
    numBit = -numBit;
  }
    
  if (stream->currentBit<0){
    stream->currentBit+=numBit;
    CommonWarning("BsGetBitBack: stream enough bits in stream ");
    return 0;
  }

  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    if (BsReadByte(stream,&bits,curNum) != curNum) {
      CommonWarning("BsGetBitBack: error reading bit stream");
      return 0;
    }
    data |= bits<<(numBit-num-curNum);
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }
  if (rewind) /* rewind */
    stream->currentBit-=numBit;

  if (BSdebugLevel >= 3)
    printf("BsGetBitBack: data=0x%lx\n",data);

  return data;
}


/* BsGetBitChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitChar (
                  HANDLE_BSBITSTREAM stream,          /* in: bit stream */
                  unsigned char *data,          /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                  int numBit)                   /* in: num bits to read */
                                /*     [0..8] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 8)
    CommonExit(1,"BsGetBitChar: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = (unsigned char ) ultmp;
  return result;
}


/* BsGetBitShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitShort (
                   HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                   unsigned short *data,                /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                   int numBit)                  /* in: num bits to read */
                                /*     [0..16] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitShort: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = (unsigned short) ultmp;
  return result;
}


/* BsGetBitInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitInt (
                 HANDLE_BSBITSTREAM stream,           /* in: bit stream */
                 unsigned int *data,            /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                 int numBit)                    /* in: num bits to read */
                                /*     [0..16] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitInt: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitAhead() */
/* Read bits from bit stream. */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAhead (
                   HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                   unsigned long *data,         /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                   int numBit)                  /* in: num bits to read */
                                /*     [0..32] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long oldCurrentBit;
  int  result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAhead: %s  id=%ld  numBit=%d\n",
           (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBit(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAhead: error reading bit stream");

  return result;
}


/* BsGetBitAheadChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadChar (
                       HANDLE_BSBITSTREAM stream,             /* in: bit stream */
                       unsigned char *data,             /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                       int numBit)                      /* in: num bits to read */
                                /*     [0..8] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadChar: %s  id=%ld  numBit=%d\n",
           (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitChar(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadChar: error reading bit stream");

  return result;
}


/* BsGetBitAheadShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadShort (
                        HANDLE_BSBITSTREAM stream,            /* in: bit stream */
                        unsigned short *data,           /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                        int numBit)                     /* in: num bits to read */
                                /*     [0..16] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadShort: %s  id=%ld  numBit=%d\n",
           (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitShort(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadShort: error reading bit stream");

  return result;
}


/* BsGetBitAheadInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadInt (
                      HANDLE_BSBITSTREAM stream,              /* in: bit stream */
                      unsigned int *data,               /* out: bits read */
                                /*      (may be NULL if numBit==0) */
                      int numBit)                       /* in: num bits to read */
                                /*     [0..16] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadInt: %s  id=%ld  numBit=%d\n",
           (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitInt(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadInt: error reading bit stream");

  return result;
}

/* BsGetBuffer() */
/* Read bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBuffer (
                 HANDLE_BSBITSTREAM stream,           /* in: bit stream */
                 HANDLE_BSBITBUFFER buffer,           /* out: buffer read */
                                /*      (may be NULL if numBit==0) */
                 long numBit)                   /* in: num bits to read */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long i,numByte,numRemain;
  unsigned long data;

  if (BSdebugLevel >= 2) {
    printf("BsGetBuffer: %s  id=%ld  numBit=%ld  ",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit);
    if (buffer != NULL)
      printf("bufSize=%ld  bufAddr=0x%lx  ",
             buffer->size,(unsigned long)buffer);
    else
      printf("(bufAddr=(NULL)  ");
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetBuffer: stream not in read mode");

  if (numBit == 0)
    return 0;

  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsGetBuffer: can not get buffer from itself");
  if (numBit < 0 || numBit > buffer->size)
    CommonExit(1,"BsGetBuffer: number of bits out of range (%ld)",numBit);

  BsClearBuffer(buffer);

  numByte = bit2byte(numBit)-1;
  for (i=0; i<numByte; i++) {
    if (BsGetBit(stream,&data,BYTE_NUMBIT)) {
      if ( ! BSaacEOF || BSdebugLevel > 0  )
        CommonWarning("BsGetBuffer: error reading bit stream");
      buffer->numBit = i*BYTE_NUMBIT;
      return 1;
    }
    buffer->data[i] = (unsigned char) data;
  }
  numRemain = numBit-numByte*BYTE_NUMBIT;
  if (BsGetBit(stream,&data,numRemain)) {
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsGetBuffer: error reading bit stream");
    buffer->numBit = numByte*BYTE_NUMBIT;
    return 1;
  }
  buffer->data[i] = (unsigned char) (data<<(BYTE_NUMBIT-numRemain));
  buffer->numBit = numBit;

  return 0;
}


/* BsGetBufferAppend() */
/* Append bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBufferAppend (
                       HANDLE_BSBITSTREAM stream,             /* in: bit stream */
                       HANDLE_BSBITBUFFER buffer,             /* out: buffer read */
                                /*      (may be NULL if numBit==0) */
                       int append,                      /* in: if != 0: append bits to buffer */
                                /*              (don't clear buffer) */
                       long numBit)                     /* in: num bits to read */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long i,numByte,last_byte,numRemain;
  unsigned long data;
  int tmp,shift_cnt,eof;

  if (BSdebugLevel >= 2) {
    printf("BsGetBufferAppend: %s  id=%ld  numBit=%ld  ",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit);
    if (buffer != NULL)
      printf("bufSize=%ld  bufAddr=0x%lx  ",
             buffer->size,(unsigned long)buffer);
    else
      printf("(bufAddr=(NULL)  ");
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetBufferAppend: stream not in read mode");

  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsGetBufferAppend: cannot get buffer from itself");

  if (numBit < 0)
    CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
               numBit);

  /* check whether number of bits to be read exceeds the end of bitstream */
  eof = BsEof(stream, numBit);
  /*  if (BsEof(stream, numBit-1)) { */
  if (eof) {
    numBit = BYTE_NUMBIT*stream->numByte - stream->currentBit;
    if (BSdebugLevel >= 2) {
      printf("*** numBits(modified)=%ld\n", numBit);
    }
  }

  if (append) {

    /* append to buffer (don't clear buffer) */

    if ((numBit+buffer->numBit) > buffer->size)
      CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
                 numBit);

    /* fill up the last possible incomplete byte */
    tmp = 8 - buffer->numBit%8;
    if (tmp == 8)
      tmp = 0;

    if (tmp <= numBit) {
      shift_cnt = 0;
    }
    else {
      shift_cnt = tmp - numBit;
      tmp = numBit;
    }

    if (tmp) {
      if (BsGetBit(stream,&data,tmp)) {
        CommonWarning("BsGetBufferAppend: error reading bit stream");
        return 1;
      }
      data <<= shift_cnt;
      numBit -= tmp;
      last_byte = buffer->numBit/8;
      data |= buffer->data[last_byte];
      buffer->numBit += tmp;
      buffer->data[last_byte] = (unsigned char) data;
      last_byte++;
    }
    else
      last_byte = buffer->numBit/8;

  }
  else { /* if (append) */
    /* clear buffer */
    if (numBit > buffer->size)
      CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
                 numBit);
    BsClearBuffer(buffer);
    last_byte = 0;
  }

  if (numBit > 0) {
    numByte = bit2byte(numBit)-1;
    for (i=last_byte; i<last_byte+numByte; i++) {
      if ((tmp = BsGetBit(stream,&data,BYTE_NUMBIT))) {
        buffer->numBit += (i-last_byte)*BYTE_NUMBIT;
        if (tmp==-1)
          return -1;    /* end of file */
        CommonWarning("BsGetBufferAppend: error reading bit stream");
        return 1;
      }
      buffer->data[i] = (unsigned char) data;
    }
    numRemain = numBit-numByte*BYTE_NUMBIT;
    if (BsGetBit(stream,&data,numRemain)) {
      CommonWarning("BsGetBufferAppend: error reading bit stream");
      buffer->numBit += numByte*BYTE_NUMBIT;
      return 1;
    }
    buffer->data[i] = (unsigned char) (data<<(BYTE_NUMBIT-numRemain));
    buffer->numBit += numBit;
  }

  if ( eof ) {
    /* just reached to the end of bitstream */
    if (stream->currentBit == BYTE_NUMBIT*stream->numByte) {
      if (BSdebugLevel >= 2) {
        printf("*** just reached the end of bitstream\n");
      }
      return -1;        /* end of file */
    }
  }

  /*  } */
  return 0;
}

/* BsGetBufferAhead() */
/* Read bits ahead of current position from bit stream to buffer. */
/* (Current position in bit stream is NOT advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBufferAhead (
                      HANDLE_BSBITSTREAM stream,              /* in: bit stream */
                      HANDLE_BSBITBUFFER buffer,              /* out: buffer read */
                                /*      (may be NULL if numBit==0) */
                      long numBit)                      /* in: num bits to read */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 2)
    printf("BsGetBufferAhead: %s  id=%ld  numBit=%ld\n",
           (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  if (numBit > stream->buffer[0]->size)
    CommonExit(1,"BsGetBufferAhead: "
               "number of bits to look ahead too high (%ld)",numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBuffer(stream,buffer,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsGetBufferAhead: error reading bit stream");

  return result;
}


/* BsGetSkip() */
/* Skip bits in bit stream (read). */
/* (Current position in bit stream is advanced !!!) */

int BsGetSkip (
               HANDLE_BSBITSTREAM stream,             /* in: bit stream */
               long numBit)                     /* in: num bits to skip */
                                /* returns: */
                                /*  0=OK  1=error */
{
  if (BSdebugLevel >= 2) {
    printf("BsGetSkip: %s  id=%ld  numBit=%ld  ",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit);
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetSkip: stream not in read mode");
  if (numBit < 0)
    CommonExit(1,"BsGetSkip: number of bits out of range (%ld)",numBit);

  if (numBit == 0)
    return 0;

  if (BsReadAhead(stream,numBit) || BsCheckRead(stream,numBit)) {
    CommonWarning("BsGetSkip: error reading bit stream");
    return 1;
  }
  stream->currentBit += numBit;
  return 0;
}


/* BsPutBit() */
/* Write bits to bit stream. */

int BsPutBit (
              HANDLE_BSBITSTREAM stream,              /* in: bit stream */
              unsigned long data,               /* in: bits to write */
              int numBit)                       /* in: num bits to write */
                                /*     [0..32] */
                                /* returns: */
                                /*  0=OK  1=error */
{
  int num,maxNum,curNum;
  unsigned long bits;

  if (BSdebugLevel > 3)
    printf("BsPutBit: %s  id=%ld  numBit=%d  data=0x%lx,%ld  curBit=%ld\n",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,numBit,data,data,stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBit: stream not in write mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsPutBit: number of bits out of range (%d)",numBit);
  if (numBit < 32 && data > ((unsigned long)1<<numBit)-1)
    CommonExit(1,"BsPutBit: data requires more than %d bits (0x%lx)",
               numBit,data);

  if (numBit == 0)
    return 0;

  /* write bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    bits = data>>(numBit-num-curNum);
    if (BsWriteByte(stream,bits,curNum)) {
      CommonWarning("BsPutBit: error writing bit stream");
      return 1;
    }
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  return 0;
}


/* BsPutBuffer() */
/* Write bits from buffer to bit stream. */

int BsPutBuffer (
                 HANDLE_BSBITSTREAM stream,           /* in: bit stream */
                 HANDLE_BSBITBUFFER buffer)           /* in: buffer to write */
                                /* returns: */
                                /*  0=OK  1=error */
{
  long i,numByte,numRemain;

  if (buffer->numBit == 0)
    return 0;

  if (BSdebugLevel >= 2)
    printf("BsPutBuffer: %s  id=%ld  numBit=%ld  bufAddr=0x%lx  curBit=%ld\n",
           (stream->file!=NULL)?"file":"buffer",
           stream->streamId,buffer->numBit,(unsigned long)buffer,
           stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBuffer: stream not in write mode");
  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsPutBuffer: can not put buffer into itself");

  numByte = bit2byte(buffer->numBit)-1;
  for (i=0; i<numByte; i++)
    if (BsPutBit(stream,buffer->data[i],BYTE_NUMBIT)) {
      CommonWarning("BsPutBuffer: error writing bit stream");
      return 1;
    }
  numRemain = buffer->numBit-numByte*BYTE_NUMBIT;
  if (BsPutBit(stream,buffer->data[i]>>(BYTE_NUMBIT-numRemain),numRemain)) {
    CommonWarning("BsPutBuffer: error reading bit stream");
    return 1;
  }

  return 0;
}


/* BsAllocBuffer() */
/* Allocate bit buffer. */

HANDLE_BSBITBUFFER BsAllocBuffer (
                            long numBit)                        /* in: buffer size in bits */
                                /* returns: */
                                /*  bit buffer (handle) */
{
  HANDLE_BSBITBUFFER buffer;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: size=%ld\n",numBit);

  if ((buffer=(HANDLE_BSBITBUFFER )malloc(sizeof(*buffer))) == NULL)
    CommonExit(1,"BsAllocBuffer: memory allocation error (buffer)");
#ifndef USAC_REFSOFT_COR1_NOFIX_02
  if ((buffer->data=(unsigned char*)malloc(bit2byte(numBit)*sizeof(char)))
      == NULL){
#else
    if ((buffer->data=(unsigned char*)malloc(bit2byte(numBit+16)*sizeof(char)))
        == NULL){
#endif
      CommonExit(1,"BsAllocBuffer: memory allocation error (data)");
    }
  buffer->numBit = 0;
  buffer->size = numBit;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: bufAddr=0x%lx\n",(unsigned long)buffer);

  return buffer;
}


/* BsFreeBuffer() */
/* Free bit buffer. */

void BsFreeBuffer (
                   HANDLE_BSBITBUFFER buffer)         /* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsFreeBuffer: size=%ld  bufAddr=0x%lx\n",
           buffer->size,(unsigned long)buffer);

  free(buffer->data);
  free(buffer);
}


/* BsBufferNumBit() */
/* Get number of bits in buffer. */

long BsBufferNumBit (
                     HANDLE_BSBITBUFFER buffer)               /* in: bit buffer */
                                /* returns: */
                                /*  number of bits in buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsBufferNumBit: numBit=%ld  size=%ld  bufAddr=0x%lx\n",
           buffer->numBit,buffer->size,(unsigned long)buffer);

  return buffer->numBit ;
}


/* BsClearBuffer() */
/* Clear bit buffer (set numBit to 0). */

void BsClearBuffer (
                    HANDLE_BSBITBUFFER buffer)                /* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsClearBuffer: size=%ld  bufAddr=0x%lx\n",
           buffer->size,(unsigned long)buffer);

  buffer->numBit = 0;
}



static void SetReadError(unsigned short           esc,
                         HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  hEscInstanceData->readErrorInFrame = 1;
  hEscInstanceData->escInstance[GetInstanceOfEsc(Channel)][esc].readErrorInEscInstance = 1;
}

/* BsReadByteEP() */
/* Read byte from stream.  It's a clon of BsReadByte() with an additional parameter list */

static int BsReadByteEP ( CODE_TABLE               code,             /* in: code of the actual bitstream element */
                          HANDLE_ESC_INSTANCE_DATA hEscInstanceData, /* in: EP Info handler */
                          HANDLE_BSBITSTREAM              stream,           /* in: stream */
                          unsigned long*           data,             /* out: data (max 8 bit, right justified) */
                          int                      numBit )          /* in: num bits [1..8] 
                                                                        returns: really read bits */
{
  long           numUsed;
  long           idx;
  long           buf;
  unsigned short sumBits;
  int            reallyReadBits;
  unsigned short esc = 0;
  unsigned short instanceOfEsc;

  if (data != NULL) {
    *data = 0;
  }
  if ( ( stream->file != NULL ) && ( stream->currentBit == stream->numByte*BYTE_NUMBIT ) ) {
    if (BsReadFile(stream)) {
      CommonWarning("BsReadByteEP: error reading bit stream file");
      return 0;
    }
  }
  if (BsCheckRead(stream,numBit)) {
    CommonWarning("BsReadByteEP: not enough bits left in stream");
    return 0;
  }
  if ( numBit == 0 || BsGetReadErrorForDataElementFlagEP ( code, hEscInstanceData )) {
    return 0;
  }

  /* computes max position of current bit pointer */
  sumBits = 0;
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        sumBits += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
      }
      if ( esc == hEscInstanceData->escAssignment[(int)code][assignmentScheme] && instanceOfEsc == GetInstanceOfEsc ( Channel )  ) {
        goto endOfTheLoop;
      }
    }
  }
 endOfTheLoop:
  /* read bits, only if current bit pointer in the correct range */
  if ( ( hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos + numBit ) <= sumBits ) {
    idx = (hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
    buf = (hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos / BYTE_NUMBIT /
           bit2byte(stream->buffer[0]->size)) % 2;
    numUsed =  hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos % BYTE_NUMBIT;
    *data = (stream->buffer[buf]->data[idx] >> (BYTE_NUMBIT-numUsed-numBit)) &
      ((1<<numBit)-1);
    reallyReadBits = numBit;
    stream->currentBit += numBit;
    hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos += numBit;
  }
  else {
    if ( hEscInstanceData->epDebugLevel >= 2 ) {
      printf ( "BsReadByteEP: end of instance %hu of esc %hu reached prematurely while reading bitstream element no %2u (bitstream.c)\n", 
               instanceOfEsc,
               esc,
               code );
    }
    reallyReadBits = sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
    stream->currentBit += reallyReadBits;
    hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = sumBits;
    SetReadError(esc, hEscInstanceData);
  }
  return ( reallyReadBits );
}

void BsCheckClassBufferFullnessEPprematurely ( int                            decoded_bits,
                                               const HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                               HANDLE_ESC_INSTANCE_DATA*      epInfoPtr )

     /* No changes on hEscInstanceData and on the buffer positions!   */
     /* Sets the expected error status on ANOTHER EpInfo struct.      */
{
  unsigned short sumBits = 0;
  unsigned short esc;
  unsigned short instanceOfEsc;

  if (!*epInfoPtr) {
    *epInfoPtr = (HANDLE_ESC_INSTANCE_DATA)calloc(sizeof(TAG_ESC_INSTANCE_DATA),1 );
  }
  memcpy(*epInfoPtr, hEscInstanceData, sizeof(TAG_ESC_INSTANCE_DATA));
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        sumBits += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
        if ( ( hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos < sumBits ) 
             && ! ( ( esc == hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme] )
                    && ( instanceOfEsc == GetInstanceOfEsc ( Channel ) ) ) ) {
          decoded_bits += sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
          SetReadError(esc, *epInfoPtr);
        }
      }
    }
  }
  sumBits = 0;
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        sumBits += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
      }
      if ( esc == hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme] && instanceOfEsc == GetInstanceOfEsc ( Channel )  ) {
        goto endOfTheLoop;
      }
    }
  }
 endOfTheLoop:
  if ( hEscInstanceData->escInstance[GetInstanceOfEsc(Channel)][hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme]].curBitPos 
       + (7 & (8 - (decoded_bits & 7))) != sumBits) /* check align bits */
    SetReadError(esc, *epInfoPtr);
}

int BsCheckClassBufferFullnessEP1 ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                    HANDLE_BSBITSTREAM    stream )
     /* return: really read bits */
{
  unsigned short sumBits = 0;
  int            reallyReadBits = 0;
  unsigned short esc;
  unsigned short instanceOfEsc;

  for ( instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        sumBits += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
        if ( ( hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos < sumBits ) 
             && ! ( ( esc == hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme] )
                    && ( instanceOfEsc == GetInstanceOfEsc ( Channel ) ) ) ) {
          if ( hEscInstanceData->epDebugLevel >= 2 ) {
            printf( "BsCheckClassBufferFullnessEP1: end of instance %hu of esc %hu not reached at end of frame (bitstream.c)\n",
                    instanceOfEsc, esc );
          }
          reallyReadBits     += sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
          stream->currentBit += sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
          hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = sumBits;
          SetReadError(esc, hEscInstanceData);
        }
      }
    }
  }
  return reallyReadBits;
}

int BsCheckClassBufferFullnessEP2 ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                    HANDLE_BSBITSTREAM    stream )
     /* return: really read bits */
{
  unsigned short sumBits = 0;
  int            reallyReadBits = 0;
  unsigned short esc;
  unsigned short instanceOfEsc;

  instanceOfEsc = GetInstanceOfEsc(Channel);
  esc = hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme];

  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        sumBits += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
      }
      if ( esc == hEscInstanceData->escAssignment[ALIGN_BITS][assignmentScheme] && instanceOfEsc == GetInstanceOfEsc ( Channel )  ) {
        goto endOfTheLoop;
      }
    }
  }
 endOfTheLoop:

  if ( hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos < sumBits ) {
    if ( hEscInstanceData->epDebugLevel >= 2 ) {
      printf( "BsCheckClassBufferFullnessEP2: end of instance %hu of esc %hu not reached at end of frame (bitstream.c)\n",
              instanceOfEsc,
              esc );
    }
    reallyReadBits     += sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
    stream->currentBit += sumBits - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos;
    hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = sumBits;
    SetReadError(esc, hEscInstanceData);
  }
  if ( stream->currentBit != hEscInstanceData->numBitsInFrame ) {
    CommonExit(1,"BsCheckClassBufferFullnessEP2: IMPLEMENTATION ERROR");
  }
  StatisticsCountBsErrors ( hEscInstanceData );
  return reallyReadBits;
}


/* ---------- external functions ---------- */

/* BsGetBitEP() */
/* Read bits from bit stream. It's a clon of BSGetBit() with an additional parameter list */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitEP ( CODE_TABLE     code,      /* in: code of the aac bitstream element */
                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData,   /* in: handle with info to decode EP bitstream */    
                 HANDLE_BSBITSTREAM    stream,    /* in: bit stream */
                 unsigned long* data,      /* out: bits read (may be NULL if numBit==0) */
                 int            numBit )   /* in: num bits to read [0..32] 
                                              returns: really read bits
                                              obsolete: 0=OK  1=error */
{
#define E(BITSTREAM_ELEMENT) #BITSTREAM_ELEMENT
  enum { maxbitstreamElementLen = 50 };
  char bitstreamElement[MAX_ELEMENTS+1][maxbitstreamElementLen] = {
#include "er_main_elements.h"
    "MAX_ELEMENTS"
  };
#undef E
  int            num;
  int            maxNum;
  int            curNum;
  int            readBits;
  int            reallyReadBits = 0;
  unsigned long  bits;
  unsigned short esc = hEscInstanceData->escAssignment[(int)code][assignmentScheme];
  unsigned short instanceOfEsc = GetInstanceOfEsc(Channel);

  if (stream->write != 0) {
    CommonExit(1,"BsGetBitEP: stream not in read mode");
    return 0;
  }
  if (numBit<0 || numBit>LONG_NUMBIT) {
    CommonExit(1,"BsGetBitEP: number of bits out of range (%d)",numBit);
    return 0;
  }   
  if (data != NULL) {
    *data = 0;
  }
  if (numBit == 0) {
    return 0;
  }
  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min ( numBit - num, maxNum );
    readBits = BsReadByteEP(code, hEscInstanceData, stream, &bits, curNum ); 
    reallyReadBits += readBits;
    if ( hEscInstanceData->epDebugLevel >= 6 ) {
      printf("BsGetBitEP: %s (%2u): %2i bit\n", bitstreamElement[code], code, numBit );
    }
    if ( readBits != curNum ) {
      if ( hEscInstanceData->epDebugLevel >= 2 ) {
        printf("BsGetBitEP: error while reading bitstream element %s (%2u)\n", bitstreamElement[code], code );
      }
      return ( reallyReadBits );
    }
    *data |= bits << ( numBit - num - curNum );
    num   += curNum;
    maxNum = BYTE_NUMBIT;
  }
  return reallyReadBits;
}

/* BsGetBufferAheadEP () */
/* Read bits ahead of current position from bit stream to buffer. It is a clon of BsGetBufferAhead()*/
/* (Current position in bit stream is NOT advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBufferAheadEP ( HANDLE_BSBITSTREAM    stream,          /* in: bit stream */
                         HANDLE_BSBITBUFFER    buffer,          /* out: buffer read (may be NULL if numBit==0) */
                         long           numBit )         /* in: num bits to read */
{
  long                  oldCurrentBit;
  int                   result;

  if ( BSdebugLevel >= 2 ) {
    printf("BsGetBufferAheadEP: %s  id=%ld  numBit=%ld\n",
           (stream->file != NULL) ? "file" : "buffer",
           stream->streamId,
           numBit );
  }
  if ( numBit > stream->buffer[0]->size ) {
    CommonExit(1,"BsGetBufferAheadEP: number of bits to look ahead too high (%ld)",numBit);
  }
  oldCurrentBit      = stream->currentBit;
  result             = BsGetBuffer ( stream, buffer, numBit );
  stream->currentBit = oldCurrentBit;
  if ( result ) {
    if ( ! BSaacEOF || BSdebugLevel > 0  ) {
      CommonWarning("BsGetBufferAheadEP: error reading bit stream");
    }
  }
  return ( result );
}


/*********************************************************************************
   Function : CreateEscInstanceData()
   Purpose  : creates an info handler and reads all the important data from files
   Argumnets: decParam     -- contents the name of the aac priority table file
              infoFileName -- name of the info file
   Returns  : pointer to the info in the heap
***********************************************************************************/

HANDLE_ESC_INSTANCE_DATA CreateEscInstanceData ( char* infoFileName, 
                                                 int   epDebugLevel ) 
{
  unsigned short           esc;
  unsigned short           instanceOfEsc;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData;
  
  /* allocate memory in the heap for the EP handler */
  hEscInstanceData = (HANDLE_ESC_INSTANCE_DATA) calloc ( sizeof(TAG_ESC_INSTANCE_DATA), 1 );
  /* open the pre-definition file and read in the data */

  /* open EP info file (only ep mode) */
  if ( infoFileName != NULL ) {
    hEscInstanceData->infoFilePtr = fopen(infoFileName, "r"); 
    if( hEscInstanceData->infoFilePtr == NULL ) {
      CommonExit(1, "CreateEscInstanceData: Can't open info file %s", infoFileName);
    }
  }

  /* set variables to zero */
  hEscInstanceData->numBitsInFrame = 0;
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance      = 0;
        hEscInstanceData->escInstance[instanceOfEsc][esc].crcResultInEscInstance = 0;
        hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos              = 0;
      }
    }
  }
  hEscInstanceData->epDebugLevel = epDebugLevel;
  if ( epDebugLevel ) {
    printf("CreateEscInstanceData: epDebugLevel=%d\n",
           epDebugLevel );
  }
  return ( hEscInstanceData );
}

unsigned short GetEsc( HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                       CODE_TABLE                   bsElement )
{
  return GetEscAssignment(hEscInstanceData, bsElement, assignmentScheme);
}

unsigned short GetEscAssignment( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                 CODE_TABLE               bsElement,
                                 int                      assignmentScheme )
{
  return hEscInstanceData->escAssignment[bsElement][assignmentScheme];
}


/*********************************************************************************
   Function : BsReadInfoFile()
   Purpose  : reads frame data from file and init bit position array
   Argumnets: handle = contents all needed data for computing
   Returns  : nothing
***********************************************************************************/

void BsReadInfoFile ( HANDLE_ESC_INSTANCE_DATA     hEscInstanceData ) 
{
  unsigned short bitsInFrame = 0;
  unsigned short esc;
  unsigned short instanceOfEsc;

  /* reads from the EP decoder info file */

  /* number of bits in frame */
  hEscInstanceData->frameLoss = 0;
  if ( fscanf( hEscInstanceData->infoFilePtr, "%hu\n", &hEscInstanceData->numBitsInFrame) == EOF) { 
    CommonExit(1, "BsReadInfoFile: error reading info file");             
  }
  /* CRC errors in each instance */
  hEscInstanceData->crcResultInFrame = 0;
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        if ( fscanf( hEscInstanceData->infoFilePtr, "%hu", &hEscInstanceData->escInstance[instanceOfEsc][esc].crcResultInEscInstance) == EOF ) {
          CommonExit(1, "BsReadInfoFile: error reading info file");
        }
        if ( hEscInstanceData->escInstance[instanceOfEsc][esc].crcResultInEscInstance ) {
          hEscInstanceData->crcResultInFrame = 1;
          if ( hEscInstanceData->epDebugLevel >= 1 ) {
            printf ( "BsReadInfoFile: crc error in instance %hu of esc %hu\n", instanceOfEsc, esc );
          }
        }
      }
    }
  }
  /* number of bits in each instance */
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        if ( fscanf( hEscInstanceData->infoFilePtr, "%hu\n", &hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance) == EOF ) { 
          CommonExit(1, "BsReadInfoFile: error reading info file"); 
        }
        if ( hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance > hEscInstanceData->numBitsInFrame ) {
          if ( hEscInstanceData->epDebugLevel >= 1 ) {
            printf ( "BsReadInfoFile: FRAMELOSS! INFO-FILE INCONSISTENT : bitsInEscInstance (%hu) > bitsInFrame (%hu)\n", 
                     hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance,
                     hEscInstanceData->numBitsInFrame );
          }
          hEscInstanceData->frameLoss = 1;
        }
        hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = bitsInFrame;              /* the total number of bits in the previos instances is 
                                                                                                   similar to the start position within the current instance */
        bitsInFrame += hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance;
        hEscInstanceData->escInstance[instanceOfEsc][esc].readErrorInEscInstance = 0;
      }
    }
  }

  hEscInstanceData->readErrorInFrame = 0;
  if ( bitsInFrame != hEscInstanceData->numBitsInFrame ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! INFO-FILE INCONSISTENT : sum of bitsInEscInstance (%hu) != bitsInFrame (%hu)\n", 
               bitsInFrame, 
               hEscInstanceData->numBitsInFrame );
    }
    hEscInstanceData->frameLoss = 1;
  }
  if ( hEscInstanceData->numBitsInFrame % 8 != 0 ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! bitsInFrame (%hu) %% 8 != 0\n",
               hEscInstanceData->numBitsInFrame);
    }
    hEscInstanceData->frameLoss = 1;
  }
  if ( hEscInstanceData->numBitsInFrame == 0 ) {
    hEscInstanceData->frameLoss = 1;
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! signalled by ep decoder\n");
    }
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
        if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
          hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance = 0;
        }
      }
    }    
  }
}


/***************************************************************************************
   Function     : BsPrepareScalEp1Stream()
   Purpose      : initialization of esc structure and concatination of ep1 class streams
                  needful for ER_SCAL epConfig 1 bitstreams
   Arguments    : hEscInstanceData = esc structure handle
                  layer            = array containing class stream handles
                  layerNumber      = layer number of the array containing class stream
                                     handles
                  numStreams       = number of ep1 streams in current layer
                  firstStream      = first ep1 stream
   Returns      : nothing
   Author       : Thomas Breitung, FhG IIS-A
   last changed : 2003-11-24
                  2004-04-19 Olaf Kaehler, FhG IIS-A
****************************************************************************************/


/* should be merged with BsPrepareEp1Stream later */
void BsPrepareScalEp1Stream( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                             LAYER_DATA *layer,
                             int layerNumber,
                             int numStreams,
                             int firstStream )
{
  unsigned short esc = 0;
  unsigned short instanceOfEsc = 0;
  unsigned short strmCnt = 0;
  unsigned short lastStream = firstStream + numStreams;

  HANDLE_BSBITSTREAM appStream = NULL;
  HANDLE_BSBITBUFFER outBuffer = BsAllocBuffer(MAX_BITBUF);
  unsigned long outLength = 0;
  int numBits;
  int increasedAUcounter = 0;

  strmCnt = firstStream;

  /* number of bits in frame */
  hEscInstanceData->numBitsInFrame = 0;
  hEscInstanceData->frameLoss = 0;

  /* number of bits in each instance */
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        if ( strmCnt >= lastStream ) {
          if ( hEscInstanceData->epDebugLevel >= 1 ) {
            printf ( "BsPrepareEp1Stream: no stream available for ESC %hu in instance %hu", esc, instanceOfEsc );
          }
          SetReadError( esc, hEscInstanceData );
        }
        else {
          numBits = layer[strmCnt].AULength[0];
          hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance = numBits;
          hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = hEscInstanceData->numBitsInFrame;
          hEscInstanceData->numBitsInFrame += numBits;

          appStream = BsOpenBufferRead( layer[strmCnt].bitBuf );
          BsGetBufferAppend( appStream, outBuffer, 1, numBits );
          outLength += numBits;
          BsRemoveAllCompletelyReadBytesFromBuffer(appStream, layer[strmCnt].bitBuf);
          if (strmCnt!=layerNumber) {
            unsigned int i;
            for (i=0;i<layer[strmCnt].NoAUInBuffer-1;i++)
              layer[strmCnt].AULength[i]=layer[strmCnt].AULength[i+1];
            layer[strmCnt].NoAUInBuffer--;
          } else {
            increasedAUcounter = 1;
          }
          strmCnt++;
        }
      }
    }
  }

  if ( strmCnt < lastStream ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsPrepareEp1Stream: supernumerous streams ignored" );
    }
    for ( ; strmCnt < lastStream; strmCnt++ ) {
      numBits = layer[strmCnt].AULength[0];
      appStream = BsOpenBufferRead( layer[strmCnt].bitBuf );
      BsGetSkip(appStream, numBits);
      BsRemoveAllCompletelyReadBytesFromBuffer(appStream, layer[strmCnt].bitBuf);
      if (strmCnt!=layerNumber) {
        unsigned int i;
        for (i=0;i<layer[strmCnt].NoAUInBuffer-1;i++)
          layer[strmCnt].AULength[i]=layer[strmCnt].AULength[i+1];
        layer[strmCnt].NoAUInBuffer--;
      } else {
        increasedAUcounter = 1;
      }
    }
  }

  numBits = BsBufferNumBit(layer[layerNumber].bitBuf);
  appStream = BsOpenBufferRead( layer[layerNumber].bitBuf );
  BsGetBufferAppend( appStream, outBuffer, 1, numBits );
  BsFreeBuffer(layer[layerNumber].bitBuf);
  layer[layerNumber].bitBuf = outBuffer;
  if (!increasedAUcounter) {
    unsigned int i;
    for (i=layer[layerNumber].NoAUInBuffer;i>0;i--)
      layer[layerNumber].AULength[i]=layer[layerNumber].AULength[i-1];
    layer[layerNumber].NoAUInBuffer++;
  }
  layer[layerNumber].AULength[0]=outLength;
   

  if ( hEscInstanceData->numBitsInFrame % 8 != 0 ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! bitsInFrame (%hu) %% 8 != 0\n",
               hEscInstanceData->numBitsInFrame);
    }
    hEscInstanceData->frameLoss = 1;
  }

  if ( hEscInstanceData->numBitsInFrame == 0 ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! signalled by ep decoder\n");
    }
    hEscInstanceData->frameLoss = 1;
  }
}



/***************************************************************************************
   Function     : BsPrepareEp1Stream()
   Purpose      : initialization of esc structure and concatination of ep1 class streams
   Argumnets    : hEscInstanceData = esc structure handle
                  layer            = array containing class stream handles
                  numStreams       = number of ep1 streams in current frame
   Returns      : nothing
   Author       : Karsten Linzmeier, FhG IIS-A
   last changed : 2002-11-07
****************************************************************************************/

void BsPrepareEp1Stream( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                         LAYER_DATA *layer,
                         int numStreams )
{
  unsigned short esc;
  unsigned short instanceOfEsc;
  unsigned short strmCnt = 0;

  HANDLE_BSBITSTREAM appStream = NULL;

  /* number of bits in frame */
  hEscInstanceData->numBitsInFrame = 0;
  hEscInstanceData->frameLoss = 0;

  /* number of bits in each instance */
  for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
    for ( esc = 0; esc < MAX_NR_OF_ESC; esc++ ) {
      if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
        if ( strmCnt >= numStreams ) {
          if ( hEscInstanceData->epDebugLevel >= 1 ) {
            printf ( "BsPrepareEp1Stream: no stream available for ESC %hu in instance %hu", esc, instanceOfEsc );
          }
          SetReadError( esc, hEscInstanceData );
        }
        else {
          hEscInstanceData->escInstance[instanceOfEsc][esc].bitsInEscInstance = BsBufferNumBit( layer[strmCnt].bitBuf );
          hEscInstanceData->escInstance[instanceOfEsc][esc].curBitPos = hEscInstanceData->numBitsInFrame;
          hEscInstanceData->numBitsInFrame += BsBufferNumBit( layer[strmCnt].bitBuf );
          if ( (strmCnt > 0) ) {
            if ( BsBufferNumBit( layer[strmCnt].bitBuf ) > 0 ) {
              appStream = BsOpenBufferRead( layer[strmCnt].bitBuf );
              BsGetBufferAppend( appStream, layer[0].bitBuf, 1, BsBufferNumBit( layer[strmCnt].bitBuf ) );
              layer[0].AULength[layer[0].NoAUInBuffer-1] += BsBufferNumBit( layer[strmCnt].bitBuf );
              layer[strmCnt].AULength[layer[strmCnt].NoAUInBuffer-1] = 0;
              BsCloseRemove( appStream, 1 );
            }
            layer[strmCnt].NoAUInBuffer--;
          }
          strmCnt++;
        }
      }
    }
  }

  if ( strmCnt < numStreams ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsPrepareEp1Stream: supernumerous streams ignored" );
    }
    for ( ; strmCnt < numStreams; strmCnt++ ) {
      layer[strmCnt].AULength[layer[strmCnt].NoAUInBuffer-1] = 0;
      layer[strmCnt].NoAUInBuffer--;
      BsClearBuffer( layer[strmCnt].bitBuf );
    }
  }

  if ( hEscInstanceData->numBitsInFrame % 8 != 0 ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! bitsInFrame (%hu) %% 8 != 0\n",
               hEscInstanceData->numBitsInFrame);
    }
    hEscInstanceData->frameLoss = 1;
  }

  if ( hEscInstanceData->numBitsInFrame == 0 ) {
    if ( hEscInstanceData->epDebugLevel >= 1 ) {
      printf ( "BsReadInfoFile: FRAMELOSS! signalled by ep decoder\n");
    }
    hEscInstanceData->frameLoss = 1;
  }

}


/* error info on frame */
unsigned char BsGetFrameLostFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return (unsigned char)( hEscInstanceData->frameLoss );
}

unsigned char BsGetErrorInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return ( BsGetReadErrorInFrameFlagEP ( hEscInstanceData ) ||
           BsGetCrcResultInFrameFlagEP ( hEscInstanceData ) );
}

unsigned char BsGetReadErrorInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return (unsigned char)( hEscInstanceData->readErrorInFrame );
}

unsigned char BsGetCrcResultInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return (unsigned char)( hEscInstanceData->crcResultInFrame );
}

/* error info on instance */

unsigned char BsGetErrorInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                           unsigned short           instanceOfEsc, 
                                           unsigned short           esc )
{
  return ( BsGetCrcResultInInstanceFlagEP ( hEscInstanceData, instanceOfEsc, esc ) ||
           BsGetReadErrorInInstanceFlagEP ( hEscInstanceData, instanceOfEsc, esc ) );
}

unsigned char BsGetCrcResultInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                               unsigned short           instanceOfEsc, 
                                               unsigned short           esc )
{
  return (unsigned char)( hEscInstanceData->escInstance[instanceOfEsc][esc].crcResultInEscInstance );
}

unsigned char BsGetReadErrorInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                               unsigned short           instanceOfEsc, 
                                               unsigned short           esc )
{
  return (unsigned char)( hEscInstanceData->escInstance[instanceOfEsc][esc].readErrorInEscInstance );
}

/* error info on data element */
unsigned char BsGetErrorForDataElementFlagEP ( CODE_TABLE     code,
                                               HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return ( BsGetReadErrorForDataElementFlagEP ( code,
                                                hEscInstanceData ) ||
           BsGetCrcResultForDataElementFlagEP ( code,
                                                hEscInstanceData ));
}

unsigned char BsGetReadErrorForDataElementFlagEP ( CODE_TABLE     code,
                                                   HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return (unsigned char)( hEscInstanceData->escInstance[GetInstanceOfEsc(Channel)][hEscInstanceData->escAssignment[(int)code][assignmentScheme]].readErrorInEscInstance );
}

unsigned char BsGetCrcResultForDataElementFlagEP ( CODE_TABLE     code,
                                                   HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return (unsigned char)( hEscInstanceData->escInstance[GetInstanceOfEsc(Channel)][hEscInstanceData->escAssignment[(int)code][assignmentScheme]].crcResultInEscInstance );
}
/* end of error info */
 
int BsGetEpDebugLevel ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  return ( hEscInstanceData->epDebugLevel );
}

/* appended by Sony on 990616 */
/*********************************************************************************
   Function : BsReadInfoFileHvx()
   Purpose  : reads frame data from file and init bit position array
   Argumnets: handle = contents all needed data for computing
   Returns  : nothing
***********************************************************************************/

void BsReadInfoFileHvx ( HANDLE_ESC_INSTANCE_DATA  hEscInstanceData, 
                         unsigned short*           crcResultInEscInstance )
{
  unsigned short esc;
  unsigned short length, s;
  char mystring[256];

  /* reads from the EP decoder info file */

  /* number of bits in frame */
  hEscInstanceData->frameLoss = 0;
  if ( fscanf( hEscInstanceData->infoFilePtr, "%hu\n", &hEscInstanceData->numBitsInFrame) == EOF) { 
    CommonExit(1, "BsReadInfoFileHvx: error reading info file");             
  }
  
  for (esc = 0; esc < MAX_NR_OF_ESC; esc++) 
    hEscInstanceData->escInstance[0][esc].crcResultInEscInstance = 0;
  
  /* CRC errors in each instance */
  if ( fgets( mystring, 256, hEscInstanceData->infoFilePtr ) == NULL ) {
    CommonExit(1, "BsReadInfoFileHvx: error reading info file");
  }
  length = (short)strlen( mystring );

  hEscInstanceData->crcResultInFrame = 0;
  for (s = esc = 0; s < length; s++) {
    if ( !strncmp( mystring+s, "0", 1 ) ) {
      hEscInstanceData->escInstance[0][esc].crcResultInEscInstance = 0;
      esc++;
    } else if ( !strncmp( mystring+s, "1", 1 ) ) {
      hEscInstanceData->escInstance[0][esc].crcResultInEscInstance = 1;
      hEscInstanceData->crcResultInFrame = 1;
      if ( hEscInstanceData->epDebugLevel >= 1 ) {
        printf ( "BsReadInfoFileHvx: crc error in esc %hu\n", esc );
      }
      esc++;
    } 
    if (esc == MAX_NR_OF_ESC) break;
  }
	
  if ( hEscInstanceData->epDebugLevel >= 2 ) {
    printf ( "CRC :");
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) 
      printf ( " %d", hEscInstanceData->escInstance[0][esc].crcResultInEscInstance );
    printf ( "\n");
  }
  hEscInstanceData->readErrorInFrame = 0;
  
  for (esc = 0; esc < MAX_NR_OF_ESC; esc++) 
      crcResultInEscInstance[esc] = hEscInstanceData->escInstance[0][esc].crcResultInEscInstance;
      /* crcResultInClass[esc] = hEscInstanceData->escInstance[0][esc].crcResultInEscInstance; */
}

unsigned short BsGetNumClassHvx ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  hEscInstanceData = hEscInstanceData;
  return ( 0 );
}

void BsCloseInfoFileHvx( HANDLE_ESC_INSTANCE_DATA hEscInstanceData )
{
  fclose( hEscInstanceData->infoFilePtr );
}

void BsSetSynElemId (int elemID)
{
  assignmentScheme = elemID ;
}

void BsSetChannel (int chan)
{
  Channel = chan ;
}

int BsGetChannel (void)
{
  return ( Channel );
}

unsigned short BsGetInstanceUsed ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                   unsigned short           instanceOfEsc, 
                                   unsigned short           esc  )
{
  unsigned short instanceUsed = 0;
  if ( hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed ) {
    instanceUsed = 1;
  }
  return ( instanceUsed );
}

void InitAacSpecificInstanceData ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                   HANDLE_RESILIENCE        hResilience,
                                   int                      channelConfiguration,
                                   unsigned short           tnsNotUsed )
{
  /* initialization of the table containing the AAC error sensitivity assignment */
  unsigned short esc;
  unsigned short instanceOfEsc;
  const unsigned short escAssignment [MAX_ELEMENTS][NR_OF_ASSIGNMENT_SCHEMES] = {{3, 3, 3},  /* COEF */
                                                                                 {3, 3, 3},  /* COEF_COMPRESS */
                                                                                 {3, 3, 3},  /* COEF_RES */
                                                                                 {9, 0, 0},  /* COMMON_WINDOW */
                                                                                 {1, 1, 1},  /* DIFF_CONTROL */
                                                                                 {1, 1, 1},  /* DIFF_CONTROL_LR */
                                                                                 {3, 3, 3},  /* DIRECTION */
                                                                                 {1, 1, 1},  /* DPCM_NOISE_LAST_POSITION */
                                                                                 {1, 1, 1},  /* DPCM_NOISE_NRG */
                                                                                 {1, 0, 0},  /* ELEMENT_INSTANCE_TAG */
                                                                                 {1, 1, 1},  /* GAIN_CONTROL_DATA_PRESENT */
                                                                                 {1, 1, 1},  /* GLOBAL_GAIN */
                                                                                 {4, 4, 4},  /* HCOD */
                                                                                 {4, 4, 4},  /* HCOD_ESC */
                                                                                 {1, 1, 1},  /* HCOD_SF */
                                                                                 {1, 1, 0},  /* ICS_RESERVED */
                                                                                 {3, 3, 3},  /* LENGTH */
                                                                                 {1, 1, 1},  /* LENGTH_OF_LONGEST_CODEWORD */
                                                                                 {1, 1, 1},  /* LENGTH_OF_REORDERED_SPECTRAL_DATA */
                                                                                 {1, 1, 1},  /* LENGTH_OF_RVLC_ESCAPES */
                                                                                 {1, 1, 1},  /* LENGTH_OF_RVLC_SF */
                                                                                 {1, 1, 1},  /* LTP_COEF */
                                                                                 {1, 1, 1},  /* LTP_DATA_PRESENT */
                                                                                 {1, 1, 1},  /* LTP_LAG */
                                                                                 {1, 1, 1},  /* LTP_LAG_UPDATE */
                                                                                 {1, 1, 1},  /* LTP_LONG_USED */
                                                                                 {1, 1, 1},  /* LTP_SHORT_LAG */
                                                                                 {1, 1, 1},  /* LTP_SHORT_LAG_PRESENT */
                                                                                 {1, 1, 1},  /* LTP_SHORT_USED */
                                                                                 {1, 1, 0},  /* MAX_SFB_CODE */
                                                                                 {9, 0, 0},  /* MS_MASK_PRESENT */
                                                                                 {9, 0, 0},  /* MS_USED */
                                                                                 {3, 3, 3},  /* N_FILT */
                                                                                 {1, 1, 1},  /* NUMBER_PULSE */
                                                                                 {3, 3, 3},  /* ORDER */
                                                                                 {1, 1, 0},  /* PREDICTOR_DATA_PRESENT */
                                                                                 {1, 1, 1},  /* PULSE_AMP */
                                                                                 {1, 1, 1},  /* PULSE_DATA_PRESENT */
                                                                                 {1, 1, 1},  /* PULSE_OFFSET */
                                                                                 {1, 1, 1},  /* PULSE_START_SFB */
                                                                                 {4, 4, 4},  /* REORDERED_SPECTRAL_DATA */
                                                                                 {1, 1, 1},  /* REV_GLOBAL_GAIN */
                                                                                 {2, 2, 2},  /* RVLC_CODE_SF */
                                                                                 {2, 2, 2},  /* RVLC_ESC_SF */
                                                                                 {1, 1, 0},  /* SCALE_FACTOR_GROUPING */
                                                                                 {1, 1, 1},  /* SECT_CB */
                                                                                 {1, 1, 1},  /* SECT_LEN_INCR */
                                                                                 {1, 1, 1},  /* SF_CONCEALMENT */
                                                                                 {1, 1, 1},  /* SF_ESCAPES_PRESENT */
                                                                                 {4, 4, 4},  /* SIGN_BITS */
                                                                                 {9, 9, 0},  /* TNS_CHANNEL_MONO */
                                                                                 {1, 1, 1},  /* TNS_DATA_PRESENT */
                                                                                 {1, 1, 0},  /* WINDOW_SEQ */
                                                                                 {1, 1, 0},  /* WINDOW_SHAPE_CODE */
                                                                                 {4, 4, 4}}; /* ALIGN_BITS */

  memcpy( hEscInstanceData->escAssignment, escAssignment, MAX_ELEMENTS*NR_OF_ASSIGNMENT_SCHEMES*sizeof(unsigned short) );
  
  /* selecting the error sensitivity category instances used within the current setup  */
  switch (channelConfiguration) {
  case 1:
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
        hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed = 0;
        if (esc != 0 && instanceOfEsc == 0 ) {
          hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed = 1;
        }
      }
    }
    break;
  case 2:
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
        hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed = 0;
        if ( ( esc == 0 && instanceOfEsc == 0 ) || ( esc > 0 ) ) {
          hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed = 1;
        }
      }
    }
    break;
  default:
    CommonExit(1, "this case should not occur");
  }
  if ( ! GetScfBitFlag ( hResilience ) ) {
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      hEscInstanceData->escInstance[instanceOfEsc][2].escInstanceUsed = 0;
    }
  }
  if ( tnsNotUsed ) {
    for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
      hEscInstanceData->escInstance[instanceOfEsc][3].escInstanceUsed = 0;
    }
  }
  if ( hEscInstanceData->epDebugLevel ) {
    printf("InitAacSpecificInstanceData: class to esc instance mapping\n");
    for (esc = 0; esc < MAX_NR_OF_ESC; esc++) {
      for (instanceOfEsc = 0; instanceOfEsc < MAX_NR_OF_INSTANCE_PER_ESC; instanceOfEsc++ ) {
        printf(" %i ",hEscInstanceData->escInstance[instanceOfEsc][esc].escInstanceUsed);
      }
      printf ("\n");
    }
  }
}


/******************************************************************************/

/* The following functions have been created due to code import from other
   locations. The intension is to do an code clean up and remove the 'static'
   class from the bitstream handlers. */

/* from files

src_frame/flex_mux.c
src_frame/mp4audec.c
src_frame/mp4ifc.c
src_latm/latm_modules.c
src_lpc/fe_sub.c
src_lpc_enc/enc_lpc.c
src_mp4/fl4_to_mp4.c
src_mp4/mp4_to_fl4.c
src_tf/adif.c
src_tf/son_gc_unpack.c
src_tf_enc/enc_tf.c
src_tf_enc/scal_enc_frame.c */

long BsBufferGetSize(HANDLE_BSBITBUFFER buffer)
{
  return(buffer->size);
}
void BsBufferSetData(LAYER_DATA *layer, unsigned char* data, unsigned int numBits)
{
  memcpy(layer->bitBuf->data, data, sizeof(char)*((numBits + 7) / 8));
  layer->bitBuf->numBit = numBits;
  *(layer->AULength) = numBits;
  layer->NoAUInBuffer = 1;
}
unsigned char* BsBufferGetDataBegin(HANDLE_BSBITBUFFER buffer)
{
  return(buffer->data);
}

long BsBufferFreeBits(HANDLE_BSBITBUFFER buffer)
{
  return(buffer->size - buffer->numBit);
}

FILE *BsFileHandle(HANDLE_BSBITSTREAM stream)
{
  return(stream->file);
}

void BsManipulateSetFileHandle(FILE *file,
                               HANDLE_BSBITSTREAM stream)
{
  stream->file = file;
}

long BsStreamId(HANDLE_BSBITSTREAM stream)
{
  return(stream->streamId);
}

int BsWriteMode(HANDLE_BSBITSTREAM stream)
{
  return(stream->write);
}

void BsRemoveAllCompletelyReadBytesFromBuffer(HANDLE_BSBITSTREAM stream,
                                              HANDLE_BSBITBUFFER buffer)
{
  unsigned long tmp,i;

  tmp = stream->currentBit/8;
  for (i = 0; i < (buffer->size/8) - tmp; i++)
    buffer->data[i] = buffer->data[i+tmp];
  stream->currentBit -= tmp*8;
  buffer->numBit -= tmp*8;
}

HANDLE_BSBITBUFFER BsGetBitBufferHandle(long curBuf,
                                        HANDLE_BSBITSTREAM stream)
{
  return(stream->buffer[curBuf]);
}

long BsNumByte(HANDLE_BSBITSTREAM stream)
{
  return(stream->numByte);
}

void BsManipulateSetNumByte(long numByte,
                            HANDLE_BSBITSTREAM stream)
{
  stream->numByte = numByte;
}

void BsBufferManipulateSetNumBit(long numBit,
                                 HANDLE_BSBITBUFFER buffer)
{
  buffer->numBit = numBit;
}

void BsManipulateSetCurrentBit(long currentBit,
                               HANDLE_BSBITSTREAM stream)
{
  stream->currentBit = currentBit;
}

void BsBufferManipulateSetDataElement(unsigned char data,
                                      long index,
                                      HANDLE_BSBITBUFFER buffer)
{
  buffer->data[index] = data;
}

HANDLE_BSBITBUFFER BsAllocPlainDirtyBuffer(unsigned char* data, long size)
{
  HANDLE_BSBITBUFFER buffer;

  buffer = (HANDLE_BSBITBUFFER )malloc (sizeof (*buffer)) ;
  buffer->data = data;
  buffer->numBit = size;
  buffer->size = size;
  return(buffer);
}

void BsDirtyForceRewindAndWrite(HANDLE_BSBITSTREAM stream)
{
  stream->currentBit = 0;
  stream->write = 0;
}

void BsDirtyDirectOnBuffer(unsigned char* p,
                           unsigned int byteSize,
                           int debugLevel,
                           HANDLE_BSBITBUFFER buffer)
{
  unsigned int i;

  buffer->numBit = 0;
  for (i = 0; i < byteSize; i++) {
    buffer->data[i] = *p++;
    if (debugLevel >= 5)
      printf("%3i 0x%02x\n", i, *(p-1)); 
    buffer->numBit += 8;
  }
}

/* SAMSUNG_2005-09-30 : set buffer position to n */
void BsSetBufRPos(HANDLE_BSBITSTREAM stream, int n)
{
	stream->currentBit = n;
}


/* end of bitstream.c */

