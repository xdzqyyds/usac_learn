/**********************************************************************
MPEG-4 Audio VM
Audio i/o module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

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

Copyright (c) 1996, 1999.



Source file: audio.c

 

Required libraries:
libtsp.a                AFsp audio file library

Required modules:
common.o                common module
austream.o              audio i/o streams (.au format)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
21-jan-97   HP    born (using AFsp-V2R2)
27-jan-97   HP    set unavailable samples to 0 in AudioReadData()
03-feb-97   HP    fix bug AudioInit formatString=NULL
19-feb-97   HP    made internal data structures invisible
21-feb-97   BT    raw: big-endian
12-sep-97   HP    fixed numSample bug for mch files in AudioOpenRead()
30-dec-98   HP    uses austream for stdin/stdout, evaluates USE_AFSP
07-jan-99   HP    AFsp-v4r1 (AFsp-V3R2 still supported)
11-jan-99   HP    clipping & seeking for austream module
17-jan-99   HP    fixed quantisation to 16 bit
26-jan-99   HP    improved output file format evaluation
17-may-99   HP    improved output file format detection
09-aug-00   RS    added int24 support
31-aug-00   HP    restored AFsp-V3R2 compatibility
                  test for "file not found" to avoid abort in AFopenRead()
16-oct-00   RS/HP made #ifdef {} things nicer ...
28-may-03   DB    made some preparation for future 48 channel support
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef USE_AFSP
#define AFSP_READ
/* #define AFSP_WRITE  */
#endif

#if defined AFSP_READ || defined AFSP_WRITE
#include <libtsp.h>             /* AFsp audio file library */
#include <libtsp/AFpar.h>       /* AFsp audio file library - definitions */
#endif

#include "audio.h"              /* audio i/o module */
#include "common_m4a.h"         /* common module */
#include "austream.h"           /* audio i/o streams (.au format) */

/* ---------- declarations ---------- */

#define SAMPLE_BUF_SIZE 16384   /* local sample buffer size */

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))

#endif
#ifndef max

#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#if defined AFSP_READ || defined AFSP_WRITE
#ifdef FW_SUN
/* only AFsp-V3R2 available: map AFsp-V3R2 to AFsp-v4r1 */
#define AFsetNHpar AFsetNH
#define FTW_AU (FW_SUN/256)
#define FTW_WAVE (FW_WAVE/256)
#define FTW_AIFF_C (FW_AIFF_C/256)
#define FTW_NH_EB (FW_NH_EB/256)
#endif
#endif

/* make it global */
int bWriteIEEEFloat = 1;

/*static	char*          output_format     = "raw";
static	unsigned short output_channels   = 1;
static	unsigned short output_wordsize   = 16;
static	unsigned long  output_sampleRate = 48000;*/

/* ---------- declarations (structures) ---------- */

struct AudioFileStruct          /* audio file handle */
{
#if defined AFSP_READ || defined AFSP_WRITE
  AFILE *fileAfsp;                  /* AFILE handle */
  AFILE *outFileAfsp[48];
#endif
  int *file;
  AuStream *stream;             /* AuStream handle */
  AuStream *outStream[48];
                                /*   NULL if AFsp used */
  int numChannel;               /* number of channels */
  long currentSample;           /* number of samples read/written */
                                /* (samples per channel!) */
  int write;                    /* 0=read  1=write */
  long numClip;                 /* number of samples clipped */
  int numFC;                    /* number of front channels */
  int fCenter;                  /* 1 if front center speaker is present*/
  int numSC;                    /* number of side channels */
  int numBC;                    /* number of back channels */
  int bCenter;                  /* 1 if back center speaker is present */
  int numLFE;                   /* number of LFE channels */  
  int multiC;                   /* 1 if more than 2 Channels present */
  int numC;                     /* number of Channels for multi channel use */
};


/* ---------- variables ---------- */

static int AUdebugLevel = 0;    /* debug level */

/* ---------- local functions ---------- */

static int isfmtstr (char *filename, char *fmtstr)
     /* isfmtstr returns true if filename has extension fmtstr */
{
  int i;

  i = strlen(filename)-strlen(fmtstr);
  if (i<0)
    return 0;
  filename += i;
  while (*filename) {
    if (tolower(*filename) != *fmtstr)
      return 0;
    filename++;
    fmtstr++;
  }
  return 1;
}

static AU_STREAM_FORMAT strToAuStreamFmt(char *str){
  AU_STREAM_FORMAT auStreamFmt = AU_STREAM_FORMAT_UNKNOWN;

  if (0 == strcmp("au", str)) {
    auStreamFmt = AU_STREAM_FORMAT_AU;
  } else if (0 == strcmp("snd", str)) {
    auStreamFmt = AU_STREAM_FORMAT_AU;
  } else if (0 == strcmp("wav", str)) {
    auStreamFmt = AU_STREAM_FORMAT_WAVE;
  } else if (0 == strcmp("wave", str)) {
    auStreamFmt = AU_STREAM_FORMAT_WAVE;
  }

  return auStreamFmt;
}

/* ---------- functions ---------- */


/* AudioInit() */
/* Init audio i/o module. */
/* formatString options: see AFsp documentation */

void AudioInit (
                char* formatString,            /* in: file format for headerless files */
                int   audioDebugLevel)         /* in: debug level
                                                  0=off  1=basic  2=full */
{
  AUdebugLevel = audioDebugLevel;
  if (AUdebugLevel >= 1) {
    printf("AudioInit: formatString=\"%s\"\n",
           (formatString!=NULL)?formatString:"(null)");
    printf("AudioInit: debugLevel=%d\n",AUdebugLevel);
#if defined AFSP_READ || defined AFSP_WRITE
    printf("AudioInit: all AFsp file formats supported\n");
#else
    printf("AudioInit: only 16 bit .au format supported\n");
#endif
  }
#if defined AFSP_READ || defined AFSP_WRITE
  if (formatString!=NULL)
    AFsetNHpar(formatString);   /* headerless file support */
#endif
}

#ifdef I2R_LOSSLESS
int AudioType(AudioFile * file)
{
#if defined AFSP_READ || defined AFSP_WRITE
  int type = 0;
  float *testbuffer[32];
  int ch = file->numChannel;
  int i;

  if (file != NULL)
    {
      switch(file->fileAfsp->NbS)
	{
	case 8:
	  type = 0;
	  break;
	case 16:
	  type = 1;
	  break;
	case 20:
	  type = 2;
	  break;
	case 24:
	  for (ch = 0;ch<file->numChannel;ch++)
	    testbuffer[ch] = (float *)malloc(32*sizeof(float));
	  AudioReadData(file,testbuffer,32);
	  for (i=0;i<32;i++)
	    if ((int)(testbuffer[0][i]*256.0)&0x0f) break;
	  AudioSeek(file,0);
	  if (i==32)
	    type = 2;
	  else
	    type = 3;
	  for (ch = 0;ch<file->numChannel;ch++)
	    free(testbuffer[ch]);
                
	  break;
	}
    }
  return type;
#else
  return 0;
#endif
}
#endif

/* AudioOpenRead() */
/* Open audio file for reading. */

AudioFile *AudioOpenRead (
                          char *fileName,               /* in: file name */
                                /*     "-": stdin (only 16 bit .au) */
                          int *numChannel,              /* out: number of channels */
                          float *fSample,               /* out: sampling frequency [Hz] */
                          long *numSample)              /* out: number of samples in file */
                                /*      (samples per channel!) */
                                /*      or -1 if not available */
                                /* returns: */
                                /*  audio file (handle) */
                                /*  or NULL if error */
{
  AudioFile *file;
#ifdef AFSP_READ
  AFILE *af;
  FILE *tf;
#else
  int *af;
#endif  

  AuStream *as;
  long ns;
  long nc;
  int nci;
  float fs;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: fileName=\"%s\"\n",fileName);

  if ((file=(AudioFile*)calloc(1, sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenRead: memory allocation error");


#ifdef AFSP_READ
  if (strcmp(fileName,"-")) {
    if ((tf=fopen(fileName,"rb"))==NULL)
      /* file not found, AFopenRead() would exit ... */
      af = NULL;
    else {
      fclose(tf);
      af = AFopenRead(fileName,&ns,&nc,&fs,
		      AUdebugLevel?stdout:(FILE*)NULL);
    }
    as = NULL;
  }
  else {
#endif
    af = NULL;
    as = AuOpenRead(fileName,&nci,&fs,&ns);
    nc = nci;
    if (ns < 0) /* unkown */
      ns = -nc; /* i.e. numSample = -1*/
    
#ifdef AFSP_READ
  }
#endif

  if (as==NULL && af==NULL) {
    CommonWarning("AudioOpenRead: error opening audio file %s",fileName);
    free(file);
    return (AudioFile*)NULL;
  }

  /*  file->outFile[j]=af;   Not working for mc files yet */
  file->multiC = 0;
#ifdef AFSP_READ
  file->fileAfsp = af;
#else
  file->file = af;
#endif
  file->stream = as;
  file->numChannel = nc;
  file->currentSample = 0;
  file->write = 0;
  file->numClip = 0;
  file->numC = 0;
  *numChannel = nc;
  *fSample = fs;
  *numSample = ns/nc;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: numChannel=%d  fSample=%.1f  numSample=%ld\n",
           *numChannel,*fSample,*numSample);

  return file;
}


/* AudioOpenWrite() */
/* Open audio file for writing. */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Supported file formats: (matching substring of format) */
/*  au, snd:  Sun (AFsp) audio file */
/*  wav:      RIFF WAVE file */
/*  aif:      AIFF-C audio file */
/*  raw:      headerless (raw) audio file (native byte order) */

AudioFile *AudioOpenWrite (
                           char *fileName,              /* in: file name */
                                /*     "-": stdout (only 16 bit .au) */
                           char *format,                        /* in: file format (ignored if stdout) */
                                /*     (au, snd, wav, aif, raw) */
                           int numChannel,              /* in: number of channels */
                           float fSample,               /* in: sampling frequency [Hz] */
                           int int24flag)
                                /* returns: */
                                /*  audio file (handle) */
                                /*  or NULL if error */
{
  AudioFile *file;
#ifdef AFSP_WRITE 
  AFILE *af;
  int fmt = 0;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",FTW_AU*256},
    {"snd",FTW_AU*256},
    {"wav",FTW_WAVE*256},
    {"wave",FTW_WAVE*256},
    {"aif",FTW_AIFF_C*256},
    {"aiff",FTW_AIFF_C*256},
    {"aifc",FTW_AIFF_C*256},
    {"raw",FTW_NH_EB*256},      /* no header big-endian */
    {NULL,-1}
  };
#else
  int *af;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",1},
    {"snd",1},
    {"wav",1},
    {"wave",1},
    {NULL,-1}
  };
#endif
  AuStream *as;
  AU_STREAM_FORMAT auStreamFmt = AU_STREAM_FORMAT_UNKNOWN;

  if (AUdebugLevel >= 1) {
    printf("AudioOpenWrite: fileName=\"%s\"  format=\"%s\"\n",fileName,format);
    printf("AudioOpenWrite: numChannel=%d  fSample=%.1f\n",
           numChannel,fSample);
  }

  if (strcmp(fileName,"-")) {
    int fmti = 0;
    while (fmtstr[fmti].str && !isfmtstr(format,fmtstr[fmti].str)) {
      fmti++;
    }
#ifdef AFSP_WRITE
    if (fmtstr[fmti].str) {
      if (int24flag) {
#ifdef FW_SUN
        /* only AFsp-V3R2 available */
        CommonWarning("AudioOpenWrite: AFsp-V3R2 has no INT24 support");
        return NULL;
#else
        fmt = FD_INT24 + fmtstr[fmti].fmt;
#endif
      }
      else {
        fmt = FD_INT16 + fmtstr[fmti].fmt;
      }
    }
#endif /*AFSP_WRITE*/
    if (!fmtstr[fmti].str) {
      CommonWarning("AudioOpenWrite: unkown audio file format \"%s\"", format);
      return (AudioFile*)NULL;
    }
    auStreamFmt = strToAuStreamFmt(fmtstr[fmti].str);
  } else {
    auStreamFmt = AU_STREAM_FORMAT_AU;
  }
  
  if ((file=(AudioFile*)calloc(1, sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenWrite: memory allocation error");
    
#ifdef AFSP_WRITE
  if (strcmp(fileName,"-")) {
    af = AFopenWrite(fileName,fmt,numChannel,fSample,
                     AUdebugLevel?stdout:(FILE*)NULL);
    as = NULL;
  }
  else {
#endif
    af = NULL;
    as = AuOpenWrite(fileName, auStreamFmt, numChannel, fSample, int24flag, bWriteIEEEFloat);  
#ifdef AFSP_WRITE
  }
#endif
    
  if (as==NULL && af==NULL) {
    CommonWarning("AudioOpenWrite: error opening audio file %s",fileName);
    free(file);
    return (AudioFile*)NULL;
  }

#ifdef AFSP_WRITE
  file->fileAfsp = af;
#else
  file->file = af;
#endif
  file->stream = as;
  file->numChannel = numChannel;
  file->currentSample = 0;
  file->write = 1;
  file->numClip = 0;
  file->multiC = 0;
  file->numC = 0;

  return file;
}

AudioFile *AudioOpenWriteMC (char *fileName, /* in: file name */
                             /* "-": stdout (only 16 bit .au) */
                             char *format,   /* in: file format (ignored if stdout) */
                             /* (au, snd, wav, aif, raw) */                         
                             float fSample,  /* in: sampling frequency [Hz] */
                             int int24flag,
                             int numFC,    
                             int fCenter,  
                             int numSC,    
                             int bCenter,
                             int numBC,
                             int numLFE)                           
     /* returns: */
     /*  audio file (handle) */
     /*  or NULL if error */
{
  int i, j=0, lenBaseName;
  char extendedFileName[1024]={0};    /*new name of outfile*/
  AudioFile *file;
  int numberOfChannels = (numFC + numSC + numBC + numLFE);
#ifdef AFSP_WRITE 
  AFILE *af;
  int fmt = 0; 
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",FTW_AU*256},
    {"snd",FTW_AU*256},
    {"wav",FTW_WAVE*256},
    {"wave",FTW_WAVE*256},
    {"aif",FTW_AIFF_C*256},
    {"aiff",FTW_AIFF_C*256},
    {"aifc",FTW_AIFF_C*256},
    {"raw",FTW_NH_EB*256},      /* no header big-endian */
    {NULL,-1}
  };
#else
  int *af;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",1},
    {"snd",1},
    {"wav",1},
    {"wave",1},
    {NULL,-1}
  };
#endif
  AU_STREAM_FORMAT auStreamFmt = AU_STREAM_FORMAT_UNKNOWN;
  AuStream *as;

  if (AUdebugLevel >= 1) {
    printf("AudioOpenWrite: fileName=\"%s\"  format=\"%s\"\n",fileName,format);
    /*   printf("AudioOpenWrite: numChannel=%d  fSample=%.1f\n",
         numChannel,fSample);*/
  }

  if (strcmp(fileName,"-")) {
    int fmti = 0;
    while (fmtstr[fmti].str && !isfmtstr(format,fmtstr[fmti].str)) {
      fmti++;
    }
#ifdef AFSP_WRITE
    if (fmtstr[fmti].str) {
      if (int24flag) {
#ifdef FW_SUN
        /* only AFsp-V3R2 available */
        CommonWarning("AudioOpenWrite: AFsp-V3R2 has no INT24 support");
        return NULL;
#else
        fmt = FD_INT24 + fmtstr[fmti].fmt;
#endif
      } else {
        fmt = FD_INT16 + fmtstr[fmti].fmt;
      }
    }
#endif /*AFSP_WRITE*/
    if (!fmtstr[fmti].str) {
      CommonWarning("AudioOpenWrite: unkown audio file format \"%s\"", format);
      return (AudioFile*)NULL;
    }
    auStreamFmt = strToAuStreamFmt(fmtstr[fmti].str); 
  } else {
    auStreamFmt = AU_STREAM_FORMAT_AU;
  }

  if ((file=(AudioFile*)calloc(1, sizeof(AudioFile))) == NULL)  
    CommonExit(1,"AudioOpenWrite: memory allocation error");

  lenBaseName = strlen(fileName)-strlen(format);
  strncpy(extendedFileName, fileName, lenBaseName);


  /* preparing files for front channels */
  /* ================================== */
  for (i=((fCenter==1) ? 0 : 1); i<numFC + (fCenter?0:1); i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'f', i, format);
   
#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, auStreamFmt, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else 
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for side channels */
  /* ================================= */
  for (i = 0; i<numSC; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 's', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, auStreamFmt, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for back channels */
  /* ================================= */
  for (i=0; i<numBC; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'b', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);    
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, auStreamFmt, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for LFE channels */
  /* ================================ */
  for (i = 0; i<numLFE; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'l', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);    
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, auStreamFmt, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  file->numChannel = 1;
  file->currentSample = 0;
  file->write = 1;
  file->numClip = 0;
  file->numFC = numFC;
  file->fCenter = fCenter;
  file->numSC = numSC;
  file->bCenter = bCenter;
  file->numBC = numBC/*11*/;
  file->numLFE = numLFE;
  file->numC = numberOfChannels;
  file->multiC = 1; 

  return file;

}

AudioFile *AudioOpenWriteMC_BSAC (char *fileName, /* in: file name */
                                  /* "-": stdout (only 16 bit .au) */
                                  char *format,   /* in: file format (ignored if stdout) */
                                  /* (au, snd, wav, aif, raw) */                         
                                  float fSample,  /* in: sampling frequency [Hz] */
                                  int int24flag,
                                  int numFC,    
                                  int fCenter,  
                                  int numSC,    
                                  int bCenter,
                                  int numBC,
                                  int numLFE)                           
     /* returns: */
     /*  audio file (handle) */
     /*  or NULL if error */
{
  int i, j=0, lenBaseName;
  char extendedFileName[1024]={0};    /*new name of outfile*/
  AudioFile *file;
  int numberOfChannels = (numFC + numSC + numBC + numLFE);
#ifdef AFSP_WRITE 
  AFILE *af;
  int fmt = 0; 
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",FTW_AU*256},
    {"snd",FTW_AU*256},
    {"wav",FTW_WAVE*256},
    {"wave",FTW_WAVE*256},
    {"aif",FTW_AIFF_C*256},
    {"aiff",FTW_AIFF_C*256},
    {"aifc",FTW_AIFF_C*256},
    {"raw",FTW_NH_EB*256},      /* no header big-endian */
    {NULL,-1}
  };
#else
  int *af;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",1},
    {"snd",1},
    {"wav",1},
    {"wave",1},
    {NULL,-1}
  };
  AU_STREAM_FORMAT auStreamFmt = AU_STREAM_FORMAT_UNKNOWN;
#endif
  AuStream *as;

  if (AUdebugLevel >= 1) {
    printf("AudioOpenWrite: fileName=\"%s\"  format=\"%s\"\n",fileName,format);
    /*   printf("AudioOpenWrite: numChannel=%d  fSample=%.1f\n",
         numChannel,fSample);*/
  }

  if (strcmp(fileName,"-")) {
    int fmti = 0;
    while (fmtstr[fmti].str && !isfmtstr(format,fmtstr[fmti].str)) {
      fmti++;
    }
#ifdef AFSP_WRITE
    if (fmtstr[fmti].str) {
      if (int24flag) {
#ifdef FW_SUN
        /* only AFsp-V3R2 available */
        CommonWarning("AudioOpenWrite: AFsp-V3R2 has no INT24 support");
        return NULL;
#else
        fmt = FD_INT24 + fmtstr[fmti].fmt;
#endif
      } else {
        fmt = FD_INT16 + fmtstr[fmti].fmt;
      }
    }
#endif /*AFSP_WRITE*/
    if (!fmtstr[fmti].str) {
      CommonWarning("AudioOpenWrite: unkown audio file format \"%s\"", format);
      return (AudioFile*)NULL;
    }
  }

  if ((file=(AudioFile*)calloc(1, sizeof(AudioFile))) == NULL)  
    CommonExit(1,"AudioOpenWrite: memory allocation error");

  lenBaseName = strlen(fileName)-strlen(format);
  strncpy(extendedFileName, fileName, lenBaseName);


  /* preparing files for front channels */
  /* ================================== */
  for (i=((fCenter==1) ? 0 : 1); i<numFC + (fCenter?0:1); i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'f', i, format);
   
#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, AU_STREAM_FORMAT_AU, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else 
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for LFE channels */
  /* ================================ */
  for (i = 0; i<numLFE; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'l', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);    
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, AU_STREAM_FORMAT_AU, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for back channels */
  /* ================================= */
  for (i=0; i<numBC; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 'b', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);    
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, AU_STREAM_FORMAT_AU, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  /* preparing files for side channels */
  /* ================================= */
  for (i = 0; i<numSC; i++) {
    sprintf(extendedFileName+lenBaseName, "_%c%02d%s", 's', i, format);

#ifdef AFSP_WRITE
    if (strcmp( extendedFileName,"-")) {
      af = AFopenWrite( extendedFileName, fmt, 1, fSample,
                        AUdebugLevel?stdout:(FILE*)NULL);
      as = NULL;
    }
    else {
#endif
      af = NULL;
      as = AuOpenWrite( extendedFileName, AU_STREAM_FORMAT_AU, 1, fSample, int24flag, bWriteIEEEFloat);
#ifdef AFSP_WRITE
    }
#endif    
    if (as==NULL && af==NULL) {
      CommonWarning("AudioOpenWrite: error opening audio file %s", extendedFileName);
      free(file);
      return (AudioFile*)NULL;
    }
#ifdef AFSP_WRITE
    file->outFileAfsp[j] = af;
#else
    file->outStream[j] = as;
#endif
    j++;
  }

  file->numChannel = 1;
  file->currentSample = 0;
  file->write = 1;
  file->numClip = 0;
  file->numFC = numFC;
  file->fCenter = fCenter;
  file->numSC = numSC;
  file->bCenter = bCenter;
  file->numBC = numBC/*11*/;
  file->numLFE = numLFE;
  file->numC = numberOfChannels;
  file->multiC = 1; 

  return file;

}
/* AudioReadData() */
/* Read data from audio file. */
/* Requested samples that could not be read from the file are set to 0. */

long AudioReadData (
                    AudioFile *file,            /* in: audio file (handle) */
                    float **data,                       /* out: data[channel][sample] */
                                /*      (range [-32768 .. 32767]) */
                    long numSample)             /* in: number of samples to be read */
                                /*     (samples per channel!) */
                                /* returns: */
                                /*  number of samples read */
{
  long tot,cur,num;
  long tmp = 0;
  long i;
  long numRead;
  int j=0;
#ifdef AFSP_READ
  float buf[SAMPLE_BUF_SIZE];
#else
  short bufs[SAMPLE_BUF_SIZE];
#endif

  if (AUdebugLevel >= 2)
    printf("AudioReadData: numSample=%ld (currentSample=%ld)\n",
           numSample,file->currentSample);

  if (file->write != 0)
    CommonExit(1,"AudioReadData: audio file not in read mode");

  /* set initial unavailable samples to 0 */
  tot = file->numChannel*numSample;
  cur = 0;
  if (file->stream && file->currentSample < 0) {
    cur = min(-file->numChannel*file->currentSample,tot);
    for (i=0; i<cur; i++)
      data[i%file->numChannel][i/file->numChannel] = 0;
  }

  /* read samples from file */
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef AFSP_READ  
    if(file->multiC){
      for(j=0; j<file->numC; j++){ 
        if (file->outFileAfsp[j]) {
          tmp = AFreadData(file->outFileAfsp[j],
                           file->numChannel*file->currentSample+cur,buf,num);
        }      
      }
      for (i=0; i<tmp; i++)
        data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = buf[i];
    } else {
      if (file->fileAfsp) {
        tmp = AFreadData(file->fileAfsp,
                         file->numChannel*file->currentSample+cur,buf,num);
        for (i=0; i<tmp; i++)
          data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = buf[i];
      }
      
    }
#else
    if(file->multiC){
      for(j=0; j<file->numC; j++) {
        if(file->outStream[j]) {
          tmp = AuReadData(file->outStream[j], bufs, num);
        }
      }
      for (i=0; i<tmp; i++)
        data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = bufs[i];
    } else {
      if (file->stream) {
        tmp = AuReadData(file->stream,bufs,num);
        for (i=0; i<tmp; i++)
          data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = bufs[i];
      }
    }
#endif

    cur += tmp;
    if (tmp < num)
      break;
  }
  numRead = cur/file->numChannel;
  file->currentSample += numRead;

  /* set remaining unavailable samples to 0 */
  for (i=cur; i<tot; i++)
    data[i%file->numChannel][i/file->numChannel] = 0;

  return numRead;
}


/* AudioWriteData() */
/* Write data to audio file. */

void AudioWriteData (
                     AudioFile *file,           /* in: audio file (handle) */
                     float **data,              /* in: data[channel][sample] */
                                                /*     (range [-32768 .. 32767]) */
                     long numSample,            /* in: number of samples to be written */
                                                /*     (samples per channel!) */
                     long skipSample            /* in: number of samples/channel to skip */
                     )
{
  long tot,cur,num=0;
  long i;
  long numClip,tmp;
  int j=1;
#ifdef AFSP_WRITE
  float buf[SAMPLE_BUF_SIZE];
#else
  short bufs[SAMPLE_BUF_SIZE];
#endif

  if (AUdebugLevel >= 2)
    printf("AudioWriteData: numSample=%ld (currentSample=%ld)\n",
           numSample,file->currentSample);

  if (file->write != 1)
    CommonExit(1,"AudioWriteData: audio file not in write mode");

  tot = file->numChannel*numSample;
  cur = max(0,-file->numChannel*file->currentSample);
  cur += skipSample*file->numChannel;
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef AFSP_WRITE
    if(file->multiC){
      for(j=0; j<file->numC; j++) {
        if (file->outFileAfsp[j]) {          
          for (i=0; i<num; i++)
            buf[i] = data[j][(cur+i)];
          AFwriteData(file->outFileAfsp[j],buf,num);
        }
      }
    } else {
      if (file->fileAfsp) {
        for (i=0; i<num; i++)
          buf[i] = data[(cur+i)%file->numChannel][(cur+i)/file->numChannel];
        AFwriteData(file->fileAfsp,buf,num);
      }
    }
     
#else
    if(file->numC) {
      for(j=0; j<file->numC; j++) {
        if(file->outStream[j]) {
          numClip = 0;
          for (i=0; i<num; i++) {
            tmp = ((long)(data[j][(cur+i)]));
            if (tmp>32767) {
              tmp = 32767;
              numClip++;
            }
            if (tmp<-32768) {
              tmp = -32768;
              numClip++;
            }
            bufs[i] = (short)tmp;
          }
          if (numClip && !file->numClip)
            CommonWarning("AudioWriteData: output samples clipped");
          file->numClip += numClip;
          AuWriteData(file->outStream[j],bufs,num);
        }
      }
    } else {
      if (file->stream) {
        numClip = 0;
        for (i=0; i<num; i++) {
          tmp = ((long)(data[(cur+i)%file->numChannel]
                        [(cur+i)/file->numChannel]+32768.5))-32768;
          if (tmp>32767) {
            tmp = 32767;
            numClip++;
          }
          if (tmp<-32768) {
            tmp = -32768;
            numClip++;
          }
          bufs[i] = (short)tmp;
        }
        if (numClip && !file->numClip)
          CommonWarning("AudioWriteData: output samples clipped");
        file->numClip += numClip;
        AuWriteData(file->stream,bufs,num);
      }
    }
#endif
    cur += num;
  }

  file->currentSample += tot/file->numChannel;
}

/* no rounding from float to short, just truncation! */
void AudioWriteDataTruncat (
                            AudioFile *file,           /* in: audio file (handle) */
                            float **data,              /* in: data[channel][sample] , (range [-32768 .. 32767]) */
                            long numSample,            /* in: number of samples to be written, (samples per channel!) */
                            long skipSample            /* in: number of samples/channel to skip */
                            )
{
  long tot,cur,num=0;
  long i;
  long numClip,tmp;
  int j=1;
#ifdef AFSP_WRITE
  float buf[SAMPLE_BUF_SIZE];
#else
  short bufs[SAMPLE_BUF_SIZE];
#endif

  if (AUdebugLevel >= 2)
    printf("AudioWriteData: numSample=%ld (currentSample=%ld)\n",
           numSample,file->currentSample);

  if (file->write != 1)
    CommonExit(1,"AudioWriteData: audio file not in write mode");

  tot = file->numChannel*numSample;
  cur = max(0,-file->numChannel*file->currentSample);
  cur += skipSample*file->numChannel;
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef AFSP_WRITE
    CommonExit(1,"AFSP not supported for truncated mode.");

    if(file->multiC){
      for(j=0; j<file->numC; j++) {
        if (file->outFileAfsp[j]) {          
          for (i=0; i<num; i++){
            buf[i] = (data[j][(cur+i)] - ((data[j][(cur+i)]>=0)?0.5:(-0.5)));
          }
          AFwriteData(file->outFileAfsp[j],buf,num);
        }
      }
    } else {
      if (file->fileAfsp) {
        for (i=0; i<num; i++){
          buf[i] = data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] - ((data[(cur+i)%file->numChannel][(cur+i)/file->numChannel]>=0.5)?0.5:(-0.5));
        }
        AFwriteData(file->fileAfsp,buf,num);
      }
    }
#else
    if(file->numC) {
      for(j=0; j<file->numC; j++) {

        if (bWriteIEEEFloat) {
          float buf[SAMPLE_BUF_SIZE] = {0};

          for (i=0; i<num; i++) {
            buf[i] = ((float)data[j][i])/32768.f;
          }
          AuWriteDataFloat(file->outStream[j], buf, num);
        } else {
          CommonExit(1,"Only float wav output for multichannel supported!");
        }
      }
    } else {
      if (file->stream) { 
        if (bWriteIEEEFloat) {
          float buf[SAMPLE_BUF_SIZE] = {0};

          for (i=0; i<num; i++) {
            buf[i] = ((float)data[(cur+i)%file->numChannel][(cur+i)/file->numChannel])/32768.f;
          }
          AuWriteDataFloat(file->stream, buf, num);	
          
        } else if (Getint24flag(file->stream)) {
          int bufs24[SAMPLE_BUF_SIZE];
          float tmp24;
          numClip = 0;
          for (i=0; i<num; i++) {
            tmp24 = (data[(cur+i)%file->numChannel]
                     [(cur+i)/file->numChannel]);
            tmp24 *= 256.0f;
            if (tmp24>8388607) {
              tmp24 = 8388607;
              numClip++;
            }
            if (tmp24<-8388608) {
              tmp24 = -8388608;
              numClip++;
            }
            bufs24[i] = (int)tmp24;
          }
          if (numClip && !file->numClip)
            CommonWarning("AudioWriteData: output samples clipped");
          file->numClip += numClip;
          AuWriteData24(file->stream,bufs24,num);	

        } else {
          numClip = 0;
          for (i=0; i<num; i++) {
            tmp = ((long)(data[(cur+i)%file->numChannel]
                          [(cur+i)/file->numChannel]));
            if (tmp>32767) {
              tmp = 32767;
              numClip++;
            }
            if (tmp<-32768) {
              tmp = -32768;
              numClip++;
            }
            bufs[i] = (short)tmp;
          }
          if (numClip && !file->numClip)
            CommonWarning("AudioWriteData: output samples clipped");
          file->numClip += numClip;
          AuWriteData(file->stream,bufs,num);
        }
      }
    }
#endif
    cur += num;
  }

  file->currentSample += tot/file->numChannel;
}



/* AudioSeek() */
/* Set position in audio file to curSample. */
/* (Beginning of file: curSample=0) */
/* NOTE: It is not possible to seek backwards in a output file if */
/*       any samples were already written to the file. */

void AudioSeek (
                AudioFile *file,        /* in: audio file (handle) */
                long curSample)         /* in: new position [samples] */
                                /*     (samples per channel!) */
{ 
  long tot,cur,num,tmp;
  int j;
#if defined AFSP_WRITE
  float buf[SAMPLE_BUF_SIZE];
#endif
  short bufs[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 1)
    printf("AudioSeek: curSample=%ld (currentSample=%ld)\n",
           curSample,file->currentSample);

  if (file->write==0) {
    if (file->stream) {
      if (file->currentSample <= 0) {
        /* nothing read from stream yet */
        if (curSample <= 0)
          file->currentSample = curSample;
        else
          file->currentSample = 0;
      }
      if (curSample < file->currentSample)
        CommonWarning("AudioSeek: can not seek backward in input stream");
      else {
        /* read samples to skip */
        tot = file->numChannel*(curSample-file->currentSample);
        cur = 0;
        while (cur < tot) {
          num = min(tot-cur,SAMPLE_BUF_SIZE);
          tmp = AuReadData(file->stream,bufs,num);
          cur += tmp;
          if (tmp < num)
            break;
        }
        file->currentSample = curSample;
      }
    }
    else
      file->currentSample = curSample;
  }
  else {
    if (file->currentSample <= 0) {
      /* nothing written to file yet */
      if (curSample <= 0)
        file->currentSample = curSample;
      else
        file->currentSample = 0;
    }
    if (curSample < file->currentSample)
      CommonExit(1,"AudioSeek: error seeking backwards in output file");
    if (curSample > file->currentSample) {
      /* seek forward, fill skipped region with silence */
#ifdef AFSP_WRITE
      memset(buf,0,SAMPLE_BUF_SIZE*sizeof(float));
#endif
      memset(bufs,0,SAMPLE_BUF_SIZE*sizeof(short));
      tot = file->numChannel*(curSample-file->currentSample);
      cur = 0;
      while (cur < tot) {
        num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef AFSP_WRITE
        if(file->multiC){
          for(j=0; j<file->numC; j++){
            if (file->outFileAfsp[j]){           
              AFwriteData(file->outFileAfsp[j],buf,num);           
            }
          }
        } else {
          if (file->fileAfsp)           
            AFwriteData(file->fileAfsp, buf, num);          
        }
#else
        if(file->multiC){
          for(j=0; j<file->numC; j++){
            if(file->outStream[j]){
              AuWriteData(file->outStream[j],bufs,num);
            }
          }
        } else {
          if (file->stream)
            AuWriteData(file->stream,bufs,num);
        }        
#endif      
        cur += num;
      }
      file->currentSample = curSample;
    }
  }
}


/* AudioClose() */
/* Close audio file.*/

void AudioClose (
                 AudioFile *file)               /* in: audio file (handle) */
{
  int j;
  if (AUdebugLevel >= 1)
    printf("AudioClose: (currentSample=%ld)\n",file->currentSample);

  if (file->numClip)
    CommonWarning("AudioClose: %ld samples clipped",file->numClip);

#if defined AFSP_READ || defined AFSP_WRITE

  if(file->multiC) {
    for(j=0; j<file->numC; j++) {
      if (file->outFileAfsp[j]) {  
        AFclose(file->outFileAfsp[j]);
        /*free(file->outFile[j]);*/ /* SAMSUNG_2005-09-30 */
      }
    }
  } else {
    if (file->fileAfsp)
      AFclose(file->fileAfsp);
  }
#endif

  if(file->multiC) {
    for(j=0; j<file->numC; j++){
      if(file->outStream[j]){
        AuClose(file->outStream[j]);
      }
    }
  } else {
    if (file->stream)
      AuClose(file->stream);
  }
  
  free(file);
}


float AudioGetSamplingFreq( AudioFile *file )
{
#if defined AFSP_WRITE || defined AFSP_READ
  if(file->multiC){
    return (float)file->outFileAfsp[2]->Sfreq;
  } else {
    return (float)file->fileAfsp->Sfreq;
  }
#else
  return 0;
#endif  
}

/* end of audio.c */
