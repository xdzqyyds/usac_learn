/**********************************************************************
MPEG-4 Audio VM
audio i/o streams (.au format)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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

Copyright (c) 1999.



Source file: austream.c

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NM    Nikolaus Meine, Uni Hannover <meine@tnt.uni-hannover.de>

Changes:
16-sep-98   HP/NM   born, based on au_io.c
06-aug-99   HP      fixed bug in AuOpenRead() numSample
**********************************************************************/

/* Audio i/o streaming with stdin/stdout support. */
/* Header format: .au (Sun audio file format, aka SND or AFsp) */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Data size set to -1 (=unknown) */

/* Multi channel data is interleaved: l0 r0 l1 r1 ... */
/* Total number of samples (over all channels) is used. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "austream.h"
#include "common_m4a.h"


/* ---------- declarations ---------- */
#ifndef min

#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ---------- declarations (structures) ---------- */

struct AuStreamStruct		/* audio stream handle */
{
  FILE *f;			/* stream handle */
  AU_STREAM_FORMAT format;      /* stream format: AU or WAVE */
  long currentSample;		/* number of samples read/written */
  int eof;			/* eof/error flag */
  int write;			/* write (not read) flag */
  float fSample;                /* sample rate */
  int numChannel;               /* number of channels */
  int int24flag;                /* write 24 bit output */
  int waveFmt;                  /* 0x1 for PCM; 0x3 for IEEE float */
};


/* ---------- variables ---------- */

static int AUdebugLevel = 0;	/* debug level */


/* ---------- local functions ---------- */

static void putbyte (char x, AuStream *s)
{
  register int a;

  a = putc(x&255,s->f);
  if (a==EOF)
    s->eof = 1;
}


static void putshort (short x, AuStream *s)
{
  register int a;

  putc((x>>8)&255,s->f);
  a = putc(x&255,s->f);
  if (a==EOF)
    s->eof = 1;
}

static void putshortLE(short x, AuStream *s)
{
  register int a;

  putc(x&255,s->f);
  a = putc((x>>8)&255,s->f);
  if (a==EOF)
    s->eof = 1;
}

static void put24(int x, AuStream *s)
{
  int a;

  putc((x>>16)&255,s->f);
  putc((x>>8)&255,s->f);
  a = putc(x&255,s->f);
  if (a==EOF)
    s->eof = 1;
}

static void put24LE(int x, AuStream *s)
{
  int a;
  
  putc(x&255,s->f);
  putc((x>>8)&255,s->f);
  a = putc((x>>16)&255,s->f);
  if (a==EOF)
    s->eof = 1;
}

static void putint (long x, AuStream *s)
{
  int a;

  putc((x>>24)&255,s->f);
  putc((x>>16)&255,s->f);
  putc((x>>8)&255,s->f);
  a = putc(x&255,s->f);
  if (a==EOF)
    s->eof = 1;
}


static void putintLE (long x, AuStream *s)
{
  int a;

  putc(x&255,s->f);
  putc((x>>8)&255,s->f);
  putc((x>>16)&255,s->f);
  a = putc((x>>24)&255,s->f);
  if (a==EOF)
    s->eof = 1;
}


static short getshort (AuStream *s)
{
  register int a,b;
  
  if (s->eof)
    return 0;
  
  a = getc(s->f);
  b = getc(s->f);
  if (b==EOF) {
    s->eof = 1;
    return 0;
  }
  return (a<<8)|b;
}

static long getint (AuStream *s)
{
  int a,b;

  if (s->eof)
    return 0;
  
  a = getc(s->f)<<24;
  a |= getc(s->f)<<16;
  a |= getc(s->f)<<8;
  b = getc(s->f);
  if (b==EOF) {
    s->eof = 1;
    return 0;
  }
  return a|b;
}


/* ---------- functions ---------- */

static void writeWaveHeader(AuStream *s){

  const int bitsPerSample = (s->waveFmt == 0x3)?32:((s->int24flag)?24:16);
  const int sampleRate = (int)(s->fSample+.5);
  const int blockAlignment = s->numChannel * (bitsPerSample >> 3);
  const int bytesPerSecond = sampleRate * blockAlignment;
  const int dataSize = s->currentSample * (bitsPerSample / 8);
  const int riffSize = dataSize + 36;

  putbyte('R', s);
  putbyte('I', s);
  putbyte('F', s);
  putbyte('F', s);
  putintLE(riffSize, s); /* RIFF size */
  putbyte('W', s);
  putbyte('A', s);
  putbyte('V', s);
  putbyte('E', s);
  putbyte('f', s);
  putbyte('m', s);
  putbyte('t', s);
  putbyte(' ', s);
  putintLE(16, s);     /* format size */
  putshortLE(s->waveFmt, s); /* format tag */
  putshortLE(s->numChannel, s); /* number channels */
  putintLE(sampleRate, s);   /* sample rate */
  putintLE(bytesPerSecond, s); /* bytes per second */
  putshortLE(blockAlignment, s); /* block alignment */
  putshortLE(bitsPerSample, s); /* bits per sample */
  putbyte('d', s);
  putbyte('a', s);
  putbyte('t', s);
  putbyte('a', s);
  putintLE(dataSize, s); /* data size */
}

/* AuInit() */
/* Init audio i/o streams. */

void AuInit (
  int debugLevel)		/* in: debug level */
				/*     0=off  1=basic  2=full */
{
  AUdebugLevel = debugLevel;
  if (AUdebugLevel) {
    printf("AuInit: debugLevel=%d\n",AUdebugLevel);
  }
}


/* AuOpenRead() */
/* Open audio stream for reading. */

AuStream *AuOpenRead (
  char *streamName,		/* in: stream name, "-" for stdin */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample)		/* out: number of samples in stream */
				/*      or -1 if not available */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */
{
  AuStream *s;
  long h,ofs,dsize,formc,srate,nchan;

  if (AUdebugLevel)
    printf("AuOpenRead: fileName=\"%s\"\n",streamName);

  if ((s=(AuStream*)malloc(sizeof(AuStream)))==NULL)
    CommonExit(-1,"AuOpenRead: Can not allocate memory");
  s->currentSample = 0;
  s->eof = 0;
  s->write = 0;
  s->numChannel = 0;
  s->fSample = 0;
  s->int24flag = 0;
  s->format = AU_STREAM_FORMAT_AU;

  if (streamName[0]=='-' && streamName[1]==0)
    s->f = stdin;
  else
    s->f = fopen(streamName,"rb");
  if (s->f==NULL) {
    CommonWarning("AuOpenRead: Can not open \"%s\"",streamName);
    free(s);
    return NULL;
  }

  h = getint(s);
  if (h!=0x2e736e64) {		/* magic string: .snd */
    CommonWarning("AuOpenRead: Wrong magic string in \"%s\"",streamName);
    free(s);
    return NULL;
  }
  ofs = getint(s);
  dsize = getint(s);
  formc = getint(s);		/* 3 = 16 bit uniform 2compl */
  srate = getint(s);
  nchan = getint(s);
  for (h=24; h<ofs; h++)
    if (getc(s->f)==EOF)
      s->eof = 1;
  if (s->eof || nchan<1 || formc!=3) {
    CommonWarning("AuOpenRead: Unsupported audio format in \"%s\"",streamName);
    free(s);
    return NULL;
  }

  s->numChannel = *numChannel = nchan;
  s->fSample = *fSample = (float)srate;
  *numSample = (dsize<0)?-1:dsize/2;

  if (AUdebugLevel)
    printf("AuOpenRead: numChannel=%d  fSample=%.1f numSample=%ld\n",
	   *numChannel,*fSample,*numSample);

  return s;
}


/* AuOpenWrite() */
/* Open audio stream for writing. */

AuStream *AuOpenWrite (
  char *streamName,		/* in: stream name, "-" for stdout */
  AU_STREAM_FORMAT format,      /* in: stream format: AU or WAVE */
  int numChannel,		/* in: number of channels */
  float fSample,		/* in: sampling frequency [Hz] */
  int int24flag,                /* in: write 24 bit output */
  int bWriteIEEEFloat)          /* in: write IEEE float */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */
{
  AuStream *s;

  if (AUdebugLevel) {
    printf("AuOpenWrite: fileName=\"%s\"\n",streamName);
    printf("AuOpenWrite: numChannel=%d  fSample=%.1f\n",
	   numChannel,fSample);
  }

  /* check allowed stream formats */
  switch(format){
  case AU_STREAM_FORMAT_AU:
  case AU_STREAM_FORMAT_WAVE:
    break;
  default:
    CommonExit(-1,"AuOpenWrite: Stream format not supported (support only for Wave and AU)!");
    break;
  }

  /* check for unsupported features */
  switch(format){
  case AU_STREAM_FORMAT_AU:
    if(bWriteIEEEFloat || int24flag){
      CommonExit(-1, "AuOpenWrite: Feature not supported.");
    }
    break;
  default:
    break;
  }

  if ((s=(AuStream*)malloc(sizeof(AuStream)))==NULL)
    CommonExit(-1,"AuOpenWrite: Can not allocate memory");
  s->format = format;
  s->currentSample = 0;
  s->eof = 0;
  s->write = 1;
  s->numChannel = numChannel;
  s->int24flag = int24flag;
  s->waveFmt = (bWriteIEEEFloat)?0x3:0x1;
  s->fSample = fSample;
  

  if (streamName[0]=='-' && streamName[1]==0)
    s->f = stdout;
  else
    s->f = fopen(streamName,"wb");
  if (s->f==NULL) {
    CommonWarning("AuOpenWrite: Can not open \"%s\"",streamName);
    free(s);
    return NULL;
  }

  switch(format){
  case AU_STREAM_FORMAT_AU:
    putint(0x2e736e64,s);	/* magic string: .snd */
    putint(28,s);		/* header size */
    putint(-1,s);		/* -1 = data size unknown */
    putint(3,s);		/* 3 = 16 bit uniform 2compl */
    putint((int)(fSample+.5),s);
    putint(numChannel,s);
    putint(0,s);		/* info string	*/
    break;
  case AU_STREAM_FORMAT_WAVE:
    writeWaveHeader(s);
    break;
  default:
    CommonExit(-1,"AuOpenWrite: Stream format not supported (support only for Wave and AU)!");
    break;
  }

  if (s->eof) {
    CommonWarning("AuOpenWrite: Can not write to \"%s\"",streamName);
    free(s);
    return NULL;
  }

  return s;
}


/* AuReadData() */
/* Read data from audio stream. */

long AuReadData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* out: data[] */
  long numSample)		/* in: number of samples to be read */
				/* returns: */
				/*  number of samples read */
{
  long i;

  if (AUdebugLevel > 1)
    printf("AuReadData: numSample=%ld\n",numSample);

  if (stream->write)
    CommonExit(1,"AuReadData: stream not in read mode");

  i = 0;
  while (!stream->eof && i<numSample)
    data[i++] = getshort(stream);
  stream->currentSample += i;

  return i;
}


/* AuWriteData() */
/* Write data to audio stream. */

void AuWriteData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* in: data[] */
  long numSample)		/* in: number of samples to be written */
{
  long i;

  if (AUdebugLevel > 1)
    printf("AuWriteData: numSample=%ld\n",numSample);

  if (!stream->write)
    CommonExit(1,"AuWriteData: audio file not in write mode");

  switch(stream->format){
  case AU_STREAM_FORMAT_AU:
    for (i=0; i<numSample; i++){
      putshort(data[i],stream);
    }
    break;
  case AU_STREAM_FORMAT_WAVE:
    for (i=0; i<numSample; i++){
      putshortLE(data[i],stream);
    }
    break;
  default:
    CommonExit(1,"AuWriteData: stream format not supported.");
    break;
  }
  stream->currentSample += numSample;

  if (stream->eof)
    CommonWarning("AuWriteDate: Can not write to au stream");
}


void AuWriteDataFloat (
                       AuStream *stream,	/* in: audio stream (handle) */
                       float *data,		/* in: data[] */
                       long numSample)		/* in: number of samples to be written */
{
  if (AUdebugLevel > 1)
    printf("AuWriteData: numSample=%ld\n",numSample);

  if (!stream->write)
    CommonExit(1,"AuWriteData: audio file not in write mode");

  switch(stream->format){
  case AU_STREAM_FORMAT_WAVE:
    fwrite(data, sizeof(float), numSample, stream->f);
    stream->eof = feof(stream->f);
    break;
  default:
    CommonExit(1,"AuWriteData: stream format not supported.");
    break;
  }
  stream->currentSample += numSample;

  if (stream->eof)
    CommonWarning("AuWriteDate: Can not write to au stream");
}

void AuWriteData24 (
  AuStream *stream,		/* in: audio stream (handle) */
  int *data,			/* in: data[] */
  long numSample)		/* in: number of samples to be written */
{
  long i;

  if (AUdebugLevel > 1)
    printf("AuWriteData: numSample=%ld\n",numSample);

  if (!stream->write)
    CommonExit(1,"AuWriteData: audio file not in write mode");

  switch(stream->format){
  case AU_STREAM_FORMAT_AU:
    for (i=0; i<numSample; i++){
      put24(data[i],stream);
    }
    break;
  case AU_STREAM_FORMAT_WAVE:
	for (i=0; i<numSample; i++){
      put24LE(data[i],stream);
	}
    break;
  default:
    CommonExit(1,"AuWriteData: stream format not supported.");
    break;
  }
  stream->currentSample += numSample;

  if (stream->eof)
    CommonWarning("AuWriteDate: Can not write to au stream");
}


/* AuClose() */
/* Close audio stream.*/

void AuClose (
  AuStream *stream)		/* in: audio stream (handle) */
{
  if (AUdebugLevel)
    printf("AuClose: currentSample=%ld\n",stream->currentSample);

  if (stream->f!=stdin && stream->f!=stdout){
    if(stream->write && (stream->format == AU_STREAM_FORMAT_WAVE) ){
      fseek(stream->f, 0, SEEK_SET);
      /* overwrite old header, since data size was unknown before */
      writeWaveHeader(stream);
    }
    fclose(stream->f);
  }
  

  free(stream);
}

int Getint24flag(AuStream *stream)
{
  return stream->int24flag;
}


/* end of austream.c */

