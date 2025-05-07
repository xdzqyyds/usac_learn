/**********************************************************************
MPEG-4 Audio VM
Fileformat converter converter

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
Source file: converter.c



Required modules:
common_m4a.o            common module
cmdline.o               command line module
streamfile.o            common stream file reader/writer

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "streamfile.h"

#include "common_m4a.h"          /* common module       */
#include "cmdline.h"             /* command line module */

#include "ep_convert.h"
#include "allHandles.h"
#include "flex_mux.h"

#include "mp4au.h"               /* frame work common declarations */


/* maximum number of inputfiles to be processed at once */
#define MAXFILES (8)
#define MAX_LAYER 50
#define MAX_PROGRAMS 8
#define MAX_TRACKS_PER_LAYER 50

int	    debug[256];
void* hSpatialDec = NULL;

static int debugLevel;

static char *outFileName;
static int outFileNameUsed;
static char *oriExt,*bitExt;

static int selectedProgNr, selectedProg;

static char *lowLevelInfo;

static char *epConfigInfo = NULL;
static char *predInfoIn = NULL, *predInfoOut = NULL;
static int removeExtensions;
static char *auSplitInfo = NULL;
static char *auLengthInfo = NULL;
static char *newLayerInfo = NULL;
static int   maxLayer;

static int*  varArgIdx;
static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<bit stream file(s) (- = stdin)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"l",&lowLevelInfo,"%s",NULL,NULL,"format specific settings"},
  {"p",&selectedProgNr,"%i",NULL,&selectedProg,"Read only one program"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,"output file name (- = stdout)"},
  {"ei",&oriExt,"%s",".mp4",NULL,"input file extension and type"},
  {"eo",&bitExt,"%s",".mp4",NULL,"output file extension and type"},
  {"d",&debugLevel,"%i","0",NULL,"debug level"},
  {"ep",&epConfigInfo,"%s",NULL,NULL,"output epCfg, individual values or one for all layers"},
  {"predin",&predInfoIn,"%s",NULL,NULL,"predefinition files for conversion to ep2/3"},
  {"predout",&predInfoOut,"%s",NULL,NULL,"write predefinition files while converting from ep2/3"},
  {"split",&auSplitInfo,"%s",NULL,NULL,"split ep0 frame into given lengths, ',' separates classes, ' ' separates layers"},
  {"length",&auLengthInfo,"%s",NULL,NULL,"resize to these AU lengths, ',' and ' ' as above"},
  {"rmext",&removeExtensions,NULL,NULL,NULL,"Remove extension payload tracks"},
  {"newlayers",&newLayerInfo,"%s",NULL,NULL,"assign input classes to different layers, comma separated list of au's per new layer"},
  {"maxl",&maxLayer,"%d","-1",NULL,"number of enhancement layers (-1 = all layers)"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};


static int stringToVector(char *string, int maxElements, char **vector, char separator)
{
  char *fname=string, *tmp;
  int ele;
  if (separator == 0) separator = ' ';
  for (ele=0; ele<maxElements; ele++) {
    /* go to next word in string */
    while ((fname)&&(*fname==separator)) fname++;
    if (!fname) break;
    /* find end of that word */
    tmp=strchr(fname, separator);
    /* save in list */
    if (tmp!=NULL) *tmp=0;
    vector[ele] = fname;
    /* continue thereafter */
    if (tmp==NULL) {ele++;break;}
    fname=tmp+1;
  }
  return ele;
}




typedef struct tagAUResizer {
  int num_lengths;
  int lengths[MAX_TRACKS_PER_LAYER];
  FILE *infoFile;
} *HANDLE_AU_RESIZER;

static HANDLE_AU_RESIZER AuResizer_create(char *string, int numTracks)
{
  int track;
  char *vector[MAX_TRACKS_PER_LAYER];

  HANDLE_AU_RESIZER ret = (HANDLE_AU_RESIZER)malloc(sizeof(struct tagAUResizer));
  ret->infoFile=NULL;
  for (track=0; track<MAX_TRACKS_PER_LAYER; track++) ret->lengths[track] = -1;
  ret->num_lengths = 0;

  for (track=0; track<numTracks; track++) vector[track]=NULL;
  if (strchr(string, ',')) {
    /* comma separated list of constant lengths */
    stringToVector(string, numTracks, vector, ',');
    for (track=0; track<numTracks; track++) if (vector[track]!=NULL) {
      ret->lengths[track] = atoi(vector[track]);
    }
    ret->num_lengths = numTracks;
  } else {
    /* filename to read the lengths from */
    ret->infoFile = fopen(string, "r");
    ret->num_lengths = numTracks;
  }

  return ret;
}

static void AuResizer_free(HANDLE_AU_RESIZER r)
{
  if (r==NULL) return;
  if (r->infoFile) fclose(r->infoFile);
  r->infoFile = NULL;
  free(r);
}

static const int* AuResizer_getFrame(HANDLE_AU_RESIZER r)
{
  if (r==NULL) return NULL;
  if (r->infoFile) {
    int track;
    for (track=0; track<r->num_lengths; track++) {
      fscanf(r->infoFile, "%i", &(r->lengths[track]));
    }
  }
  return r->lengths;
}





static HANDLE_STREAMFILE* openInputFiles(char **argv, int *varArgIdx, int *numFiles)
{
  HANDLE_STREAMFILE *files;
  int fileIdx = 0;
  char oriFileName[STRLEN];
  while (varArgIdx[fileIdx] >= 0) fileIdx++;
  files = (HANDLE_STREAMFILE*)malloc(fileIdx*(sizeof(HANDLE_STREAMFILE)));

  fileIdx=0;
  while (varArgIdx[fileIdx] >= 0) {
    /* compose file names */
    if (ComposeFileName(argv[varArgIdx[fileIdx]],0,"",oriExt,oriFileName,STRLEN))
      CommonExit(1,"composed file name too long");
    DebugPrintf(1, "opening %s\n",oriFileName);

    /* open the input files */
    files[fileIdx] = StreamFileOpenRead(oriFileName,FILETYPE_AUTO);
    if (files[fileIdx]==NULL) {
      fileIdx--;
      while (fileIdx>=0) StreamFileClose(files[fileIdx--]);
      free(files);
      return NULL;
    }

    fileIdx++;
  }
  if (numFiles) *numFiles=fileIdx;
  return files;
}

static HANDLE_STREAMPROG* openInputPrograms(HANDLE_STREAMFILE *inFiles,
                                            int fileNum,
                                            int selectProgram,
                                            int *numPrograms)
{
  HANDLE_STREAMPROG *prog;
  int num = 0, j;
  for (j=0; j<fileNum; j++) {
    int progCount = StreamFileGetProgramCount(inFiles[j]);
    if (progCount<0) return NULL;
    DebugPrintf(3, " file has %i programs\n",progCount);
    num+=progCount;
  }
  prog = (HANDLE_STREAMPROG*)malloc(num*(sizeof(HANDLE_STREAMPROG)));

  num=0;
  for (j=0; j<fileNum; j++) {
    int progCount, k;
    /* open all (selected) programs */
    progCount = StreamFileGetProgramCount(inFiles[j]);

    for (k=0; k<progCount; k++) {
      if ((selectProgram>=0) && (k != selectProgram)) continue;
      prog[num] = StreamFileGetProgram(inFiles[j], k);
      if (prog[num]==NULL) {
        free(prog);
        return NULL;
      }
      num++;
    }
  }
  if (numPrograms) *numPrograms=num;
  return prog;
}



static HANDLE_EPCONVERTER* initEpConverters(DEC_CONF_DESCRIPTOR* decConf,    /* in */
                                            int numLayerIn,                  /* in */
                                            int *tracksInLayer,              /* in */
                                            int *extInLayer,                 /* in */
                                            int rmExtTracks,                 /* in */
                                            char *auSplitInfo,               /* in */
                                            char *epConfigInfo,              /* in */
                                            char *predInfoIn,                /* in */
                                            char *predInfoOut,               /* in */
                                            char *newLayerInfo,              /* in */
                                            int *numLayerOut,                /* out */
                                            int *tracksInLayerOut,           /* out */
                                            DEC_CONF_DESCRIPTOR* decConfOut, /* out */
                                            unsigned long *totalTracksOut)   /* out */
{
  int lay;
  int firstTrackInLayer;
  char *v1[MAX_LAYER];
  char *predFile[MAX_LAYER], *writePredFile[MAX_LAYER];
  EP_SPECIFIC_CONFIG epInfoIn[MAX_LAYER],  epInfoOut[MAX_LAYER];
  int                epConfigIn[MAX_LAYER],epConfigOut[MAX_LAYER];
  HANDLE_EPCONVERTER *epConverter = NULL;

  /* get output layer assignment of input access units */
  *numLayerOut=0;
  for (lay=0; lay<MAX_LAYER; lay++) v1[lay]=NULL;
  if (newLayerInfo!=NULL) {
    int totalTracksIn = 0;
    int totalTracksOut = 0;
    stringToVector(newLayerInfo, MAX_LAYER, v1, ',');
    for (lay=0; ; lay++) {
      if (v1[lay]==NULL) break;
      tracksInLayerOut[lay] = atoi(v1[lay]);
      totalTracksOut += tracksInLayerOut[lay];
      (*numLayerOut)++;
      DebugPrintf(2, "assigning to layer %i %i tracks\n",lay, tracksInLayerOut[lay]);
    }
    for (lay=0; lay<numLayerIn; lay++) {
      totalTracksIn += tracksInLayer[lay];
    }
    if (totalTracksOut!=totalTracksIn) {
      CommonWarning("error assigning tracks to new layers: %i input tracks vs %i assigned tracks\nIgnoring this assignment\n",totalTracksIn,totalTracksOut);
      *numLayerOut=0;
    }
  }
  if (*numLayerOut==0) {
    *numLayerOut=numLayerIn;
    for (lay=0; lay<*numLayerOut; lay++) {
      tracksInLayerOut[lay] = tracksInLayer[lay];
    }
  }

  epConverter = (HANDLE_EPCONVERTER*)malloc(*numLayerOut*sizeof(HANDLE_EPCONVERTER));

  /* get input epConfig values and epSpecificConfigs (and possibly dump them to files) */
  for (lay=0; lay<numLayerIn; lay++) writePredFile[lay]=NULL;
  if (predInfoOut!=NULL) stringToVector(predInfoOut, numLayerIn, writePredFile, ' ');
  for (firstTrackInLayer=0,lay=0; lay<numLayerIn; firstTrackInLayer+=tracksInLayer[lay++]) {
    epConfigIn[lay] = decConf[firstTrackInLayer].audioSpecificConfig.epConfig.value;
    epInfoIn[lay]   = decConf[firstTrackInLayer].audioSpecificConfig.epSpecificConfig;
  }

  /* set output epConfig values */
  for (lay=0; lay<*numLayerOut; lay++) v1[lay]=NULL;
  lay=0;
  if (epConfigInfo!=NULL) {
    stringToVector(epConfigInfo, *numLayerOut, v1, ' ');
    for (; lay<*numLayerOut; lay++) {
      if (v1[lay]!=NULL) epConfigOut[lay]=atoi(v1[lay]);
      else break;
    }
    if (lay!=0) {
      int tmp=lay-1;
      for (; lay<*numLayerOut; lay++) epConfigOut[lay]=atoi(v1[tmp]);
    }
  }
  for (; lay<numLayerIn; lay++) epConfigOut[lay]=epConfigIn[lay];
  for (; lay<*numLayerOut; lay++) epConfigOut[lay]=epConfigIn[numLayerIn-1];
  
  /* get output class length info to split ep0 frames */
  for (lay=0; lay<*numLayerOut; lay++) v1[lay]=NULL;
  if (auSplitInfo!=NULL) stringToVector(auSplitInfo, *numLayerOut, v1, ' ');

  /* read output epSpecificConfigs from pred files */
  for (lay=0; lay<*numLayerOut; lay++) predFile[lay]=NULL;
  if (predInfoIn!=NULL) stringToVector(predInfoIn, *numLayerOut, predFile, ' ');

  {
    int j, outTrack;
    for (firstTrackInLayer=0,lay=0,j=0,outTrack=0; lay<*numLayerOut; firstTrackInLayer+=tracksInLayerOut[lay++]) {
      int k, outTracks;
      char *usePredFile = NULL;
      int epc_tmp;
      if (epConfigOut[lay]>=2) {
        if (predFile[j]!=NULL) {
          usePredFile = predFile[j++];
        } else {
          if (epConfigIn[lay]>=2) {
            epInfoOut[lay] = epInfoIn[lay];
          } else {
            CommonWarning("missing pred file for layer %i",lay);
            lay--;
            while (lay>=0) {
              /* TODO: free(*epConfigOut); but not all of them */
              
              lay--;
            }
            free(epConverter);
            return NULL;
          }
        }
      }
      if (lay<numLayerIn) epc_tmp = epConfigIn[lay];
      else epc_tmp = epConfigIn[numLayerIn-1];
      DebugPrintf(5,"layer %i: ep%i -> ep%i\n", lay, epc_tmp, epConfigOut[lay]);
      epConverter[lay] = EPconvert_create(tracksInLayerOut[lay],
                                          /*epConfigIn[lay]*/epc_tmp,
                                          &epInfoIn[lay],
                                          writePredFile[lay],
                                          rmExtTracks?extInLayer[lay]:0,
                                          epConfigOut[lay],
                                          &epInfoOut[lay],
                                          usePredFile);
      if (epConverter[lay]==NULL) {
        lay--;
        while (lay>=0) {
          /* TODO: free(*epConfigOut); but not all of them */
          
          lay--;
        }
        free(epConverter);
        return NULL;
      }

      /* set output class length info to split ep0 frames */
      if (v1[lay]!=NULL) {
        int auLengths[MAX_TRACKS_PER_LAYER];
        char *v2[MAX_LAYER];
        for (k=0; k<MAX_TRACKS_PER_LAYER; k++) v2[k]=NULL;
        k=0;
        stringToVector(v1[lay], MAX_TRACKS_PER_LAYER, v2, ',');
        for (; k<MAX_TRACKS_PER_LAYER; k++) {
          if (v2[k]!=NULL) auLengths[k]=atoi(v2[k]);
          else break;
        }
        if (k>0) EPconvert_setSplitting(epConverter[lay], k, auLengths);
      }

      /* now prepare some output tracks */
      outTracks = EPconvert_expectedOutputClasses(epConverter[lay]);

      for (k=0; k<outTracks; k++) {
        if (outTracks==tracksInLayer[lay]) {
          decConfOut[outTrack] = decConf[firstTrackInLayer+k]; /* memcpy? */
        } else {
          decConfOut[outTrack] = decConf[firstTrackInLayer]; /* memcpy? */
        }
        decConfOut[outTrack].audioSpecificConfig.epConfig.value = epConfigOut[lay];
        decConfOut[outTrack].audioSpecificConfig.epSpecificConfig = epInfoOut[lay];

        

        switch (decConfOut[outTrack].audioSpecificConfig.audioDecoderType.value) {
        case AAC_MAIN:
        case AAC_LC:
        case AAC_SSR:
        case AAC_LTP:
        case AAC_SCAL:
        case ER_AAC_LC:
        case ER_AAC_LTP:
        case ER_AAC_LD:
#ifdef AAC_ELD
        case ER_AAC_ELD:
#endif
        case ER_AAC_SCAL:
          EPconvert_byteAlignOutput(epConverter[lay]);
          break;
        }

        outTrack++;
      }
    }
    if (totalTracksOut) *totalTracksOut = outTrack;
  }

  return epConverter;
}



/* main -------------*/

int main (int argc, char *argv[])
{
  /* file i/o variables */
  HANDLE_STREAMFILE* inFiles = NULL;
  HANDLE_STREAMPROG* inPrograms = NULL;
  HANDLE_STREAMFILE outFile;
  HANDLE_STREAMPROG* outPrograms = NULL;
  int numPrograms = 0;
  int numFiles    = 0;

  /* ep-conversion variables */
  HANDLE_EPCONVERTER *epConverter[MAX_PROGRAMS];
  int numLayerIn[MAX_PROGRAMS];
  int tracksInLayerIn[MAX_PROGRAMS][MAX_TRACKS_PER_LAYER];
  int numLayerOut[MAX_PROGRAMS];
  int tracksInLayerOut[MAX_PROGRAMS][MAX_TRACKS_PER_LAYER];

  /* others */
  int err, prog, trackNr, progNr;

  oriExt = ".mp4";
  bitExt = ".mp4";

  /* init */
  for (trackNr=0; trackNr<MAX_TRACKS_PER_LAYER; trackNr++) {
    for (progNr=0; progNr<MAX_PROGRAMS; progNr++) {
        tracksInLayerIn[progNr][trackNr] = 0;
    }
  }
  
  /* ---- evaluate command line ---- */
  {
    char *progName = "<no program name>";

    CmdLineInit(0);
    err = CmdLineEval(argc,argv,paraList,switchList,1,&progName);
    SetDebugLevel(debugLevel);
    if (err) {
      if (err==1) {
        printf("%s\n",progName);
        CmdLineHelp(progName,paraList,switchList,stdout);
        StreamFileShowHelp();
        exit (1);
      } else
        CommonExit(1,"command line error (\"-h\" for help)");
    }

    DebugPrintf(1,"%s\n",progName);
  }

  /* process all files on command line */
  if (varArgIdx[0] < 0) {
    CommonExit(-1,"missing filenames");
  }
  
  /* ---- open input files ---- */
  inFiles = openInputFiles(argv, varArgIdx, &numFiles);
  if (inFiles==NULL) {
    CommonExit(-1,"error opening input");
  }
  inPrograms = openInputPrograms(inFiles, numFiles, (selectedProg?selectedProgNr:-1), &numPrograms);
  if (inPrograms==NULL) {
    CommonExit(-1,"error reading input");
  }

  /* ---- open output file ---- */
  {
    char bitFileName[STRLEN];

    if (outFileNameUsed) {
      if (ComposeFileName(outFileName,0,"",bitExt,bitFileName,STRLEN))
        CommonExit(1,"composed file name too long");
    } else
      if (ComposeFileName(argv[varArgIdx[0]],1,"",bitExt,bitFileName,STRLEN))
        CommonExit(1,"composed file name too long");
    outFile = StreamFileOpenWrite(bitFileName,FILETYPE_AUTO,lowLevelInfo);
    if (outFile == NULL) goto bail_out;
  }

  /* ---- copy and ep-convert program data ---- */
  outPrograms = (HANDLE_STREAMPROG*)malloc(numPrograms*sizeof(HANDLE_STREAMPROG));
  for (prog=0; prog<numPrograms; prog++) {
    int track;
    int extInLayer[MAX_TRACKS_PER_LAYER];
    HANDLE_STREAMPROG prog_data = inPrograms[prog];
    outPrograms[prog] = StreamFileAddProgram(outFile);
    
    {
      /* ---- count layers and tracks ---- */
      int streamCount = prog_data->trackCount;
      
      if (maxLayer == -1) {
        maxLayer = prog_data->trackCount;
      }
      
      tracksPerLayer(prog_data->decoderConfig, &maxLayer, &streamCount, tracksInLayerIn[prog], extInLayer);
      numLayerIn[prog] = maxLayer+1;
    }
    DebugPrintf(3, "input program %i has %i tracks in %i layers ", prog, prog_data->trackCount, numLayerIn[prog]);

    /* ---- initialise ep-coverter ---- */
    epConverter[prog] = initEpConverters(prog_data->decoderConfig, numLayerIn[prog],
                                         tracksInLayerIn[prog], extInLayer, removeExtensions,
                                         auSplitInfo,
                                         epConfigInfo, predInfoIn, predInfoOut, newLayerInfo,
                                         &(numLayerOut[prog]), tracksInLayerOut[prog],
                                         outPrograms[prog]->decoderConfig, &(outPrograms[prog]->trackCount));
    if (epConverter[prog]==NULL) goto bail_out;

    for (track=0; track<(signed)outPrograms[prog]->trackCount; track++) {
      outPrograms[prog]->dependencies[track] = track-1;
    }
  }

  /* ---- fix/update some header values ---- */
  for (prog = 0; prog < numPrograms; prog++) {
    StreamFileFixProgram(outPrograms[prog]);
  }
      
  /*StreamFileWriteHeader(outFile); (not necessary) */
  {
    /* ---- read and write all access units ---- */
    int frame, layer;
    unsigned long track, firstTrackInLayer, firstTrackOutLayer;
    int input_track[MAX_PROGRAMS][MAX_LAYER][MAX_TRACKS_PER_LAYER];
    HANDLE_STREAM_AU inAUs[MAX_TRACKS_PER_LAYER];
    HANDLE_STREAM_AU outAUs[MAX_TRACKS_PER_LAYER];
    HANDLE_AU_RESIZER length_overrides[MAX_PROGRAMS][MAX_LAYER];

    /* if layers are split up new, this structure will indicate, which newly assigned layer/track can be found where */
    {
      for (prog=0; prog<numPrograms; prog++) {
        int track_i=0;
        for (layer=0; layer<numLayerOut[prog]; layer++) {
          for (track=0; track<(unsigned)tracksInLayerOut[prog][layer]; track++) {
            input_track[prog][layer][track] = track_i++;
          }
        }
      }
    }

    for (track=0; track<MAX_TRACKS_PER_LAYER; track++) {
      inAUs[track] = StreamFileAllocateAU(0);
      outAUs[track] = StreamFileAllocateAU(0);
    }

    /* get output class length info e.g. to override padding read from file */
    for (prog=0; prog<numPrograms; prog++)
      for (layer=0; layer<numLayerOut[prog]; layer++)
        length_overrides[prog][layer] = NULL;

    if (auLengthInfo!=NULL) {
      char *v1[MAX_LAYER];
      prog=0;
      for (layer=0; layer<numLayerOut[prog]; layer++) v1[layer]=NULL;
      stringToVector(auLengthInfo, numLayerOut[prog], v1, ' ');
      for (layer=0; layer<numLayerOut[prog]; layer++) if (v1[layer]!=NULL) {
        if (v1[layer]) length_overrides[prog][layer] = AuResizer_create(v1[layer],tracksInLayerOut[prog][layer]);
      }
    }

    for (frame=0; 1; frame++) {                              /* loop until EOF */
      if (debugLevel==1) fprintf(stderr,"\rframe %i",frame);
      DebugPrintf(2,"frame %ld",frame);
      for (prog = 0; prog < numPrograms; prog++) {           /* loop through all programs in the inputfiles */
        int AUs;
        HANDLE_STREAMPROG inprog = inPrograms[prog];
        DebugPrintf(4," prog  %ld",prog);
        firstTrackInLayer = firstTrackOutLayer = 0;
        for (layer = 0; layer < numLayerOut[prog]; layer++) {   /* loop through all layers in that program */
          int outTracks = -1;
          DebugPrintf(5," layer  %ld",layer);
          /*for (AUs = 0; AUs < StreamAUsPerFrame(inprog, firstTrackInLayer); AUs++) {*/
          for (AUs = 0; AUs < StreamAUsPerFrame(inprog, input_track[prog][layer][0]); AUs++) {
            const int *resize_sizes = NULL;
            unsigned int epconv_input_tracks;
            if (length_overrides[prog][layer]) resize_sizes = AuResizer_getFrame(length_overrides[prog][layer]);
            outTracks = EPconvert_expectedOutputClasses(epConverter[prog][layer]);
            
            /* if epConverter still has some buffered frames, use them up! */
            if (EPconvert_numFramesBuffered(epConverter[prog][layer])>0) {
              epconv_input_tracks=0;
            } else {
              epconv_input_tracks = tracksInLayerOut[prog][layer];
            }

            /* read AUs */
            for (track=0; track<epconv_input_tracks; track++) { /* loop through all tracks of that layer */
              /*
                DebugPrintf(3," reading track %ld (%ld/%ld)", firstTrackInLayer + track, track, epconv_input_tracks);
                err = StreamGetAccessUnit(inprog, firstTrackInLayer + track, inAUs[track]);
              */
              DebugPrintf(3," reading track %ld (%ld/%ld)", input_track[prog][layer][track], track, epconv_input_tracks);
              err = StreamGetAccessUnit(inprog, input_track[prog][layer][track], inAUs[track]);
              if (err == -2) goto xeof;
              if (err) goto bail;

              if (resize_sizes) if (resize_sizes[track]>=0) {
                DebugPrintf(3," resizing AccessUnit from %ld bits to %ld bits", inAUs[track]->numBits, resize_sizes[track]);
                StreamFile_AUresize(inAUs[track], resize_sizes[track]);
              }
            }

            /* convert AUs */
            err = EPconvert_processAUs(epConverter[prog][layer],
                                       inAUs, epconv_input_tracks,
                                       outAUs, MAX_TRACKS_PER_LAYER);
              if (err<0) goto bail;
            if (err!=outTracks) {
              if (EPconvert_needMoreFrames(epConverter[prog][layer])) {
                outTracks=err;
              } else {
              CommonWarning("Expected %i AUs after ep-conversion, but got %i AUs", outTracks, err);
              }
            }

            /* write AUs */
            for (track=0; track<(unsigned)outTracks; track++) { /* loop through all tracks of the output layer */
              DebugPrintf(3," writing track %ld (%ld/%ld)", firstTrackOutLayer + track, track, outTracks);
              err = StreamPutAccessUnit(outPrograms[prog], firstTrackOutLayer + track, outAUs[track]);
              if (err) goto bail;
            }
          }
          firstTrackInLayer += tracksInLayerIn[prog][layer];
          firstTrackOutLayer += outTracks;
        }
      }
    }

  xeof:

    /* close all files again */
    {
      int fileIdx;
      for (fileIdx=0; fileIdx<numFiles; fileIdx++) {
        StreamFileClose(inFiles[fileIdx]);
      }
      StreamFileClose(outFile);
    }

 bail:
    for (track=0; track<MAX_TRACKS_PER_LAYER; track++) {
      StreamFileFreeAU(inAUs[track]);
      StreamFileFreeAU(outAUs[track]);
    }
    for (prog=0; prog<numPrograms; prog++) {
      for (layer=0; layer < numLayerOut[prog]; layer++) {
        AuResizer_free(length_overrides[prog][layer]);
        
      }
      free(epConverter[prog]);
    }
    free(outPrograms);
    free(inPrograms);
    free(inFiles);
  }

 bail_out:
  CmdLineEvalFree(paraList);

  DebugPrintf(1,"finished\n");
  return(0);
}

