/**********************************************************************
MPEG-4 Audio VM
ep encode and decode routines

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
Source file: ep_convert.c

 

Required modules:
common_m4a.o            common module

**********************************************************************/

#undef DEBUG_EP_CONVERT

#ifdef FHG_EPTOOL
#include <eptoollib.h>
#else
#include <libeptool.h>
#define HANDLE_ERROR_INFO int
#define MAX_EP_FRAMELENGTH 65536
#endif
#include <stdlib.h>              /* rand() */

#include "bitstream.h"
#include "flex_mux.h"            /* audio object types */
#include "common_m4a.h"          /* common module */

#include "streamfile.h"
#include "streamfile_helper.h"

#include "ep_convert.h"

/* limiting the maximum number of trracks during any step of the conversions */
#define MAX_CONVERSION_TRACKS 50

#define ACTION_decEp3  0x01
#define ACTION_encEp3  0x02
#define ACTION_merge   0x04
#define ACTION_alignEp1  0x08
#define ACTION_split   0x10

struct tagEpConverter {
  int inputEpConfig;
  int outputEpConfig;
  int action;
  int classesToRemove;

  int nOfTracksIn;
  int nOfTracksOut;
#ifdef FHG_EPTOOL
  HANDLE_EP_TOOL hEpToolDec, hEpToolEnc;
  HANDLE_EP_FRAME epFrameDec, epFrameEnc;
#else
  HANDLE_EP_CONFIG hEpToolDec, hEpToolEnc;
#endif
  EP_SPECIFIC_CONFIG outputEpInfo;
  int epDecBufferedFrames;
  int epEncWaitForFrames;
  int set_random_puncture_rate;
  int set_random_crclen;
  int split_num;
  int split_lengths[MAX_CONVERSION_TRACKS];

  int verbose;
};

static const int bitFlag = 0;
static void WriteOutputPredFile(EP_SPECIFIC_CONFIG *esc, const char* filename);

static void setupSplittingForPredSet(HANDLE_EPCONVERTER cfg, int predset);

static HANDLE_EPCONVERTER EPconverter_alloc()
{
  HANDLE_EPCONVERTER ret = malloc(sizeof(struct tagEpConverter));
  if (ret==NULL) return NULL;
  
  ret->inputEpConfig = -1;
  ret->outputEpConfig = -1;
  ret->action = -1;
  ret->classesToRemove = 0;

  ret->nOfTracksIn = -1;
  ret->nOfTracksOut = -1;
  ret->hEpToolDec = NULL;
  ret->hEpToolEnc = NULL;
#ifdef FHG_EPTOOL
  ret->epFrameDec = NULL;
  ret->epFrameEnc = NULL;
#endif
  ret->epDecBufferedFrames=0;
  ret->epEncWaitForFrames =0;
  ret->set_random_puncture_rate = 0;
  ret->set_random_crclen = 0;

  ret->split_num = 0;
  {
    int i;
    for (i=0; i<MAX_CONVERSION_TRACKS; i++) ret->split_lengths[i] = 0;
  }

  ret->verbose = 0;

  return ret;
}


#ifdef FHG_EPTOOL
static const char errorFormat[] = "**error number %# in module %m, line %n (function %f): %e\n";
static int epPrintError(HANDLE_ERROR_INFO err) {
  char *message;
  message = (char*) malloc((unsigned int)errorLength(err, errorFormat, 65536));
  if(message){
    errorText(err, errorFormat, message, 65536);
    CommonWarning("%s", message);
  }
  return 0;
}
#else
static int epPrintError(HANDLE_ERROR_INFO err) {
  CommonWarning("EPTool library returned error %i",err);
  return 0;
}
#endif


/* -------------------------------------------------- */
/* --- various ways of transforming access units  --- */
/* -------------------------------------------------- */


/* +++ ep1 -> ep3 +++ */
static int epToolEncode(const HANDLE_EPCONVERTER cfg, HANDLE_STREAM_AU *au,
                        int validTracksIn, int maxTracksOut)
{
  HANDLE_CLASS_FRAME inFrame = NULL;
  HANDLE_ERROR_INFO eperr;
  int i;

  if (maxTracksOut<1) {
    CommonWarning("epToolEncode: Track count error: space to write %i tracks, expected 1 track", maxTracksOut);
    return -1;
  }
#ifdef FHG_EPTOOL
  if (noError != (eperr = EPTool_GetClassFrame(cfg->hEpToolEnc, &inFrame))) { /* get number of classes from predFile*/
    epPrintError(eperr);
    return -1;
  }
  inFrame->nrValidBuffer = inFrame->maxClasses;
#else
  if (NULL == (inFrame = EPTool_GetClassFrame(cfg->hEpToolEnc, bitFlag))) { /* get number of classes from predFile*/
    CommonWarning("epToolEncode: EPTool_GetClassFrame returned NULL");
    return -1;
  }
#endif
  if (validTracksIn != inFrame->nrValidBuffer) {
    CommonWarning("epToolEncode: Track count error: recieved %i tracks, expected %i tracks", validTracksIn, inFrame->maxClasses);
    
    /*    return -1;*/
  }

  /* copy mp4classBuffers in epClassBuffer */
  for (i=0; i<inFrame->nrValidBuffer; i++){
    int possible_rates[] = {0,3,4,6,8,12,16,24};
    int possible_crc[] = {0,6,8,10,12,14,16,32};
    inFrame->classBuffer[i] = au[i]->data;
    inFrame->classLength[i] = au[i]->numBits;
    if (cfg->set_random_crclen)
      inFrame->classCRCLength[i] = possible_crc[(int)(8.0*((double)(rand())/RAND_MAX))];
    if (cfg->set_random_puncture_rate)
      inFrame->classCodeRate[i] = possible_rates[(int)(8.0*((double)(rand())/RAND_MAX))];
  }
  inFrame->choiceOfPred = -1; /* eptool should chose by itself */

  /* EP Encode one frame */
#ifdef FHG_EPTOOL
  if (noError != (eperr = EPTool_EncodeFrame( cfg->hEpToolEnc, inFrame, cfg->epFrameEnc ))) {
    epPrintError(eperr);
    return -1;
  }
  StreamFile_AUcopyResize(au[0], cfg->epFrameEnc->frameBuffer, cfg->epFrameEnc->frameLength);
#else
  {
    HANDLE_STREAM_AU tmpAU = StreamFileAllocateAU(MAX_EP_FRAMELENGTH);
    eperr = EPTool_EncodeFrame(cfg->hEpToolEnc, inFrame, tmpAU->data, &i, bitFlag);
    if (eperr==2) {
      cfg->epEncWaitForFrames = 1;
      eperr=0;
    } else if (eperr!=0) {
      epPrintError(eperr);
      return -1;
    } else {
      cfg->epEncWaitForFrames = 0;
    }
    StreamFile_AUresize(tmpAU, i);
    StreamFileFreeAU(au[0]);
    au[0]=tmpAU;
  }
#endif

  return cfg->epEncWaitForFrames?0:1;
}

/* +++ ep3 -> ep1 +++ */
static int epToolDecode(const HANDLE_EPCONVERTER cfg, const HANDLE_STREAM_AU *au,
                        int validTracksIn, int maxTracksOut)
{
  HANDLE_CLASS_FRAME outFrame = NULL;
  HANDLE_ERROR_INFO eperr;
  int j, classes;

  if ((validTracksIn!=1)&&
      (!((validTracksIn==0)&&(cfg->epDecBufferedFrames>0)))) {
    CommonWarning("epToolDecode: Track count error: recieved %i tracks, expected 1 track", validTracksIn);
    return -1;
  }
  if ((validTracksIn==1)&&(cfg->epDecBufferedFrames>0)) {
    CommonWarning("epToolDecode: unexpectedly recieved new frame, dropping those still buffered in epTool!");
  }
#ifdef FHG_EPTOOL
  if (noError != (eperr = EPTool_GetClassFrame(cfg->hEpToolDec, &outFrame))) { /* get number of classes from predFile*/
    epPrintError(eperr);
    return -1;
  }
  outFrame->nrValidBuffer = outFrame->maxClasses;
#else
  if (NULL == (outFrame = EPTool_GetClassFrame(cfg->hEpToolDec, bitFlag))) { /* get number of classes from predFile*/
    CommonWarning("epToolEncode: EPTool_GetClassFrame returned NULL");
    return -1;
  }
#endif

  if (maxTracksOut<outFrame->maxClasses) {
    CommonWarning("epToolDecode: Track count error: space to write %i tracks, expected %i tracks", maxTracksOut, outFrame->maxClasses);
    
    return -1;
  }
  /* copy mp4classBuffers in epClassBuffer */
#ifdef FHG_EPTOOL
  cfg->epFrameDec->frameBuffer = au[0]->data;
  cfg->epFrameDec->frameLength = au[0]->numBits;
  
  /* EP Decode one frame */ 
  if (noError != (eperr = EPTool_DecodeFrame(cfg->hEpToolDec, outFrame, cfg->epFrameDec))) {
    epPrintError(eperr);
    return -1;
  }
#else
  /* if no input is given and epDecBufferedFrames>0, get buffered frame! */
  if ((validTracksIn==0)&&(cfg->epDecBufferedFrames>0)) {
    eperr = EPTool_GetDecodedFrame(cfg->hEpToolDec, outFrame, bitFlag);
  } else {
    eperr = EPTool_DecodeFrame(cfg->hEpToolDec, outFrame, au[0]->data, au[0]->numBits, bitFlag);
  }
  if (eperr<0) {
    epPrintError(eperr);
    return -1;
  }
  cfg->epDecBufferedFrames = eperr;
#endif
  classes = outFrame->nrValidBuffer;

  for (j=0; j<classes; j++) {
    StreamFile_AUcopyResize(au[j], outFrame->classBuffer[j], outFrame->classLength[j]);
  }

  return classes;
}

/* +++ ep1 -> ep0 +++ */
static int mergeAccessUnits(const HANDLE_EPCONVERTER cfg, const HANDLE_STREAM_AU *au,
                            int validTracksIn, int maxTracksOut)
{
  int i;
  int totalBitShift;

  if (maxTracksOut<1) {
    CommonWarning("mergeAccessUnits: Track count error: space to write %i tracks, expected 1 track", maxTracksOut);
    return -1;
  }
  if (validTracksIn<1) {
    CommonWarning("mergeAccessUnits: Track count error: recieved %i tracks, expected at least 1 track", validTracksIn);
    return -1;
  }

  totalBitShift = (8-(au[0]->numBits&7))&7;

  for (i=1; i<validTracksIn; i++) {
    long j;
    long unitSize = (au[i]->numBits+7)/8;
    long tmp;
    int pad = (8-(au[i]->numBits&7))&7;

    tmp = (au[0]->numBits+7)>>3;
    StreamFile_AUresize(au[0], au[0]->numBits+au[i]->numBits);

    for (j=0; j<unitSize; j++) {
      if (totalBitShift > 0) {
        au[0]->data[tmp+j-1] &= -(1<<totalBitShift);
        au[0]->data[tmp+j-1] |= (((unsigned char*)(au[i]->data))[j]) >> (unsigned)(8-totalBitShift);
      }
      if ((unsigned)tmp+j<((au[0]->numBits+7)>>3)) {
        au[0]->data[tmp+j] = au[i]->data[j] << totalBitShift;
      }
    }
    totalBitShift += pad;
    if (totalBitShift>=8) totalBitShift-=8;
  }

  return 1;
}


/* +++ ep0 -> ep2 (split access units) +++ */
static int splitAccessUnits(const HANDLE_EPCONVERTER cfg, const HANDLE_STREAM_AU *au,
                            int validTracksIn, int maxTracksOut)
{
  int i;
  unsigned int totalBitPos;
  int totalBytePos;
  int out_num = cfg->split_num;
  int *out_len = cfg->split_lengths;

  if (out_num < 1) {
    CommonWarning("splitAccessUnits: Error determining into how many classes to split the frame");
    return -1;
  }
  if (maxTracksOut<out_num) {
    CommonWarning("splitAccessUnits: Track count error: space to write %i tracks, expected %i tracks", maxTracksOut, out_num);
    return -1;
  }
  if (validTracksIn!=1) {
    CommonWarning("splitAccessUnits: Track count error: recieved %i tracks, expected 1 track", validTracksIn);
    return -1;
  }

  totalBitPos=0;
  for (i=0; i<out_num; i++) {
    totalBitPos += out_len[i];
  }
  if (totalBitPos != au[0]->numBits) {
    CommonWarning("splitAccessUnits: Bit count error: recieved %i bits, expected %i bits", au[0]->numBits, totalBitPos);
    return -1;
  }

  totalBitPos = out_len[0]&7;
  totalBytePos = out_len[0]>>3;

  for (i=1; i<out_num; i++) {
    long j;
    long unitSize = (out_len[i]+7)/8;
    int lastBits = out_len[i] - (unitSize<<3);

    StreamFile_AUresize(au[i], out_len[i]);

    for (j=0; j<unitSize; j++) {
      au[i]->data[j] = au[0]->data[totalBytePos] << totalBitPos;
      totalBytePos++;
      /*     if ((totalBitPos>0) && (((j<<3)+totalBitPos) < (unsigned)out_len[i])) */
      /* _sps-note_2006-04-04_: fix to be reviewed by kaehleof */
      if ((totalBitPos>0) && ((((j+1)<<3)-totalBitPos) < (unsigned)out_len[i]))
        au[i]->data[j] |= au[0]->data[totalBytePos] >> (8-totalBitPos);
    }

    /* _sps-note_2006-04-04_: addition to be reviewe by kaehleof */
    if ( totalBitPos < (unsigned) (-lastBits) ) {
      totalBitPos += 8;
      totalBytePos--;
    }
    totalBitPos += lastBits;
    /* _sps-note_2006-04-04_: sps thinks that this is superfluous (lastBits is negative), to be reviewed by kaehleof */
    if (totalBitPos>=8) totalBitPos-=8;
  }
  StreamFile_AUresize(au[0], out_len[0]);

  return out_num;
}


/* +++ ep1 -> ep1 (byte align the payload) */
static int checkAlignAccessUnits(const HANDLE_EPCONVERTER cfg, const HANDLE_STREAM_AU *au,
                                 int validTracksIn, int maxTracksOut)
{
  int i, pad = 0;
  int totalPaddingBits = 0;

  maxTracksOut=maxTracksOut; /* unused */

  if (validTracksIn<1) {
    CommonWarning("checkAlignAccessUnits: Track count error: recieved %i tracks, expected at least 1 track", validTracksIn);
    return -1;
  }

  for (i=0; i<validTracksIn; i++) {
    pad = (8-(au[i]->numBits&7))&7;

    totalPaddingBits += pad;
    if (totalPaddingBits>=8) {
      totalPaddingBits-=8;
    }
  }
  i--;
  if (totalPaddingBits > 0) {
    unsigned long tmp = au[i]->numBits;
    if (pad>0) au[i]->data[((tmp+7)>>3)-1] &= -(1<<pad);

    /*fprintf(stderr,"padding incorrect, adding %i bits\n",totalPaddingBits);*/

    tmp += totalPaddingBits;
    StreamFile_AUresize(au[i], tmp);

    if (totalPaddingBits>pad) au[i]->data[((tmp+7)>>3)-1] = 0;
  }

  return validTracksIn;
}


static void showAccessUnits(int tracks, HANDLE_STREAM_AU au[])
{
  int i;
  for (i=0; i<tracks; i++) {
    DebugPrintf(0, "AU %2i: %5i bits", i, au[i]->numBits);
  }
}


/* -------------------------------------------------- */
/* --- initialisation routines                    --- */
/* -------------------------------------------------- */


/* +++ determine, which things to do +++ */
static int determineActions(int inputEpConfig, int outputEpConfig)
{
  int action = -1;

  switch ( inputEpConfig ) {
  case 0:
    switch ( outputEpConfig ) {
    case -1:
    case 0:
      action = 0;
      break;
    case 1:
      action = ACTION_split;
      break;
    case 2:
    case 3:
      action = ACTION_split | ACTION_encEp3;
      break;
    }
    break;
  case 1:
    switch ( outputEpConfig ) {
    case 0:
      action = ACTION_merge;
      break;
    case -1:
    case 1:
      action = 0;
      break;
    case 2:
      action = ACTION_merge | ACTION_split | ACTION_encEp3;
      break;
    case 3:
      action = ACTION_encEp3;
      break;
    }
    break;
  case 2:
    switch ( outputEpConfig ) {
    case 0:
      action = ACTION_decEp3 | ACTION_merge;
      break;
    case 1:
      action = ACTION_decEp3 | ACTION_merge | ACTION_split;
      break;
    case -1:
      action = 0;
    case 2:
    case 3:
      action = ACTION_decEp3 | ACTION_merge | ACTION_split | ACTION_encEp3;
      break;
    }
    break;
  case 3:
    switch ( outputEpConfig ) {
    case 0:
      action = ACTION_decEp3 | ACTION_merge;
      break;
    case 1:
      action = ACTION_decEp3;
      break;
    case 2:
      action = ACTION_decEp3 | ACTION_merge | ACTION_split | ACTION_encEp3;
      break;
    case -1:
      action = 0;
      break;
    case 3:
      action = ACTION_decEp3 | ACTION_encEp3;
      break;
    }
  }

  return action;
}



/* +++ prepare structures necessary for the given actions +++ */
static unsigned char *writeEpConfig(EP_SPECIFIC_CONFIG *epSpecificInfoStruct, long *len)
{
  unsigned char *configBuffer = NULL;
  unsigned int configLength;
  HANDLE_BSBITBUFFER bb;
  HANDLE_BSBITSTREAM epSpecificInfoStream;

  configLength = advanceEpSpecConf( NULL, epSpecificInfoStruct, 2 /* 2=get length */);

  if (NULL == (configBuffer = (unsigned char *)malloc((configLength+7)/8))) {
    CommonWarning("epdec/epenc: Failed malloc for configBuffer");
    return NULL;
  }

  bb = BsAllocPlainDirtyBuffer(configBuffer, configLength);
  epSpecificInfoStream = BsOpenBufferWrite(bb);
  advanceEpSpecConf( epSpecificInfoStream, epSpecificInfoStruct, 1 /*write*/ );
  BsCloseRemove( epSpecificInfoStream, 0 );
  free(bb);

  if (len!=NULL) *len = configLength;
  return configBuffer;
}


static int initEPdec(EP_SPECIFIC_CONFIG *epSpecificInfoStruct,
#ifdef FHG_EPTOOL
                     HANDLE_EP_TOOL *hEpTool,
                     HANDLE_EP_FRAME *epFrame,
#else
                     HANDLE_EP_CONFIG *hEpTool,
#endif
                     const char *writePredFile
                     )
{
  unsigned char *configBuffer = NULL;
  long configLength;
  int maxClasses;
  HANDLE_ERROR_INFO eperr;

  configBuffer = writeEpConfig(epSpecificInfoStruct, &configLength);
  if (configBuffer==NULL) {
    CommonWarning("epdec: error writing ep config buffer");
    return -1;
  }

  /* create epTool */
#ifdef FHG_EPTOOL
  if (noError != (eperr = EPTool_CreateEPTool(hEpTool, epFrame, configBuffer, configLength, 1/*decode*/ ))) {
    epPrintError(eperr);
    free(configBuffer);
    return -1;
  }
#else
  if (0 != (eperr = EPTool_CreateExpl(hEpTool, NULL, &configBuffer, &configLength, 1/*reorder flag*/, bitFlag ))) {
    epPrintError(eperr);
    free(configBuffer);
    return -1;
  }
#endif
  free(configBuffer);

  /* write predefinition file */
  if (writePredFile!=NULL) {
    WriteOutputPredFile(epSpecificInfoStruct, writePredFile);
  }

  maxClasses=EPTool_GetMaximumNumberOfClasses(*hEpTool);

  return maxClasses;
}

static int initEPenc(EP_SPECIFIC_CONFIG *epSpecificInfoStruct,
                     const char *readPredFile,
#ifdef FHG_EPTOOL
                     HANDLE_EP_TOOL *hEpTool,
                     HANDLE_EP_FRAME *hEpFrame
#else
                     HANDLE_EP_CONFIG *hEpTool
#endif
                     )
{
  unsigned char *configBuffer = NULL;
  long configLength;
  HANDLE_ERROR_INFO eperr;
  int fileparse = (readPredFile!=NULL);

  if (!fileparse) {
    configBuffer = writeEpConfig(epSpecificInfoStruct, &configLength);
    if (configBuffer==NULL) {
      CommonWarning("epenc: error writing ep config buffer");
      return -1;
    }
  } else {
    if (0 != (eperr = EPTool_CreateBufferEPSpecificConfig(&configBuffer, readPredFile))) {
      epPrintError(eperr);
      return -1;
    }
  }

#ifdef FHG_EPTOOL
  if (noError != (eperr = EPTool_ParseOutBandInfoFile(readPredFile, configBuffer, (unsigned int*)&configLength))) {
    epPrintError(eperr);
    if (configBuffer) free(configBuffer);
    return -1;
  }
  if (noError != (eperr = EPTool_CreateEPTool(hEpTool, hEpFrame, configBuffer, configLength, 0 /* encoding */ )))
#else
  if (0 != (eperr = EPTool_CreateExpl(hEpTool, readPredFile, &configBuffer, &configLength, 0/*reorder flag*/, bitFlag )))
#endif
    {
      epPrintError(eperr);
      if (configBuffer) free(configBuffer);
      return -1;
    }

  if ((fileparse)&&(epSpecificInfoStruct!=NULL)) {
    HANDLE_BSBITSTREAM epSpecificInfoStream;
    HANDLE_BSBITBUFFER bb;

    initEpSpecConf (epSpecificInfoStruct);
    bb = BsAllocPlainDirtyBuffer((unsigned char *)configBuffer, configLength);
    epSpecificInfoStream = BsOpenBufferRead(bb);

    advanceEpSpecConf( epSpecificInfoStream,
                       epSpecificInfoStruct,
                       0 /* 0=read */ );
    BsClose(epSpecificInfoStream);
  }

  free(configBuffer);
  return 1;
}




static void setupSplittingForPredSet(HANDLE_EPCONVERTER cfg, int predset)
{
  PRED_SET_CONFIG *pred_set;
  unsigned int class;

  if (cfg==NULL) return;
  if (!(cfg->action & ACTION_encEp3)) return;

  if (predset>=(signed)cfg->outputEpInfo.numberOfPredifinedSet.value) return;

  pred_set=&(cfg->outputEpInfo.predSetConfig[predset]);

  cfg->split_num = 0;
  for (class=0; class<pred_set->numberOfClass.value; class++) {
    int lengthEscape = pred_set->epClassConfig[class].lengthEscape.value;
    int classLength =  pred_set->epClassConfig[class].classLength.value;
    if (lengthEscape == 1) cfg->split_lengths[cfg->split_num] = -1; /*variable*/
    else cfg->split_lengths[cfg->split_num] = classLength;
    cfg->split_num++;
  }
}


static int computeTrackCounts(HANDLE_EPCONVERTER cfg,
                              EP_SPECIFIC_CONFIG *inputEpInfo,
                              const char *writePredFile,
                              EP_SPECIFIC_CONFIG *outputEpInfo,
                              const char *readPredFile)
{
  int tracks;
  /* +++ further actions unspecified */
  if (cfg->action == -1) {
    CommonWarning("don't know how continue for ep%i->ep%i, breaking",cfg->inputEpConfig,cfg->outputEpConfig);
    return -1;
  }

  /* +++ start with all tracks */
  tracks = cfg->nOfTracksIn;

  /* +++ decode ep3: one in through eptool / multiple out */
  if (cfg->action & ACTION_decEp3) {
    if (inputEpInfo == NULL) {
      CommonWarning("EPSpecificConfig missing to decode from ep%i",cfg->inputEpConfig);
      return -1;
    }
#ifdef FHG_EPTOOL
    tracks = initEPdec(inputEpInfo, &cfg->hEpToolDec, &cfg->epFrameDec, writePredFile);
#else
    tracks = initEPdec(inputEpInfo, &cfg->hEpToolDec, writePredFile);
#endif
    if (tracks<=0) {
      CommonWarning("Error initializing decoding of ep%i",cfg->inputEpConfig);
      return -1;
    }
  }

  /* +++ remove ext payload for ep1 */
  if (cfg->classesToRemove>0) {
    tracks -= cfg->classesToRemove;
  }

  /* +++ "encode" ep0: multiple in / one out */
  if (cfg->action & ACTION_merge) {
    tracks = 1;
  }

  /* +++ split ep0 frame at some fixed lengths */
  if (cfg->action & ACTION_split) {
    tracks = cfg->split_num;
  }

  /* +++ encode ep3: multiple in / one out through eptool */
  if (cfg->action & ACTION_encEp3) {
    EP_SPECIFIC_CONFIG *esc = outputEpInfo;
    if ((esc == NULL)&&(readPredFile == NULL)) {
      CommonWarning("EPSpecificConfig missing to encode to ep%i",cfg->outputEpConfig);
      return -1;
    }

    if (esc==NULL) esc=&cfg->outputEpInfo;
#ifdef FHG_EPTOOL
    tracks = initEPenc(esc, readPredFile, &cfg->hEpToolEnc, &cfg->epFrameEnc);
#else
    tracks = initEPenc(esc, readPredFile, &cfg->hEpToolEnc);
#endif
    if (outputEpInfo!=NULL) cfg->outputEpInfo = *outputEpInfo; /* memcpy?*/
    
    if (tracks<=0) {
      CommonWarning("Error initializing encoding of ep%i",cfg->outputEpConfig);
      return -1;
    }
  }

  cfg->nOfTracksOut = tracks;
  return 0;
}


HANDLE_EPCONVERTER EPconvert_create(int inputTracks,
                                    int inputEpConfig,
                                    EP_SPECIFIC_CONFIG *inputEpInfo,
                                    const char *writePredFile,
                                    int removeLastNClasses,
                                    int outputEpConfig,
                                    EP_SPECIFIC_CONFIG *outputEpInfo,
                                    const char *readPredFile)
{
  HANDLE_EPCONVERTER ret;

  if (inputTracks<0) {
    CommonWarning("epConvertInit: number of input tracks unknown");
    return NULL;
  }
  if (inputEpConfig<0) {
    CommonWarning("epConvertInit: input epConfig unknown");
    return NULL;
  }

  ret = EPconverter_alloc();
  if (ret==NULL) return NULL;

  ret->nOfTracksIn = inputTracks;
  ret->inputEpConfig = inputEpConfig;
  ret->outputEpConfig = outputEpConfig;

  ret->action = determineActions(inputEpConfig, outputEpConfig);
  if (ret->action == -1) {
    CommonWarning("epConvertInit: error determining actions for ep%i->ep%i",ret->inputEpConfig,ret->outputEpConfig);
    free(ret);
    return NULL;
  }

  ret->classesToRemove = removeLastNClasses;

  if (computeTrackCounts(ret, inputEpInfo, writePredFile, outputEpInfo, readPredFile)<0) {
    CommonWarning("epConvertInit: error preparing steps for ep%i->ep%i",ret->inputEpConfig,ret->outputEpConfig);
    
    free(ret);
    return NULL;
  }

  return ret;
}


int EPconvert_processAUs(const HANDLE_EPCONVERTER cfg,
                         const HANDLE_STREAM_AU *input, int validTracksIn,
                         HANDLE_STREAM_AU *output, int maxTracksOut)
{
  HANDLE_STREAM_AU tmpAUs[MAX_CONVERSION_TRACKS];
  int i;
  int validTracks, allocatedTracks;
  int paddingKnown = 1; 

  if ((validTracksIn != cfg->nOfTracksIn)&&
      (!((validTracksIn==0)&&(cfg->epDecBufferedFrames>0)))) {
    CommonWarning("epConvertAccessUnits: Track count error: recieved %i tracks, expected %i tracks", validTracksIn, cfg->nOfTracksIn);
    return -1;
  }
  if (maxTracksOut < cfg->nOfTracksOut) {
    CommonWarning("epConvertAccessUnits: Track count error: space to write %i tracks, expected %i tracks", maxTracksOut, cfg->nOfTracksOut);
    return -1;
  }

  for (i=0; i<validTracksIn; i++) {
    tmpAUs[i] = StreamFileAllocateAU(0);
    StreamFile_AUcopyResize(tmpAUs[i], input[i]->data, input[i]->numBits);
  }
  for (; i<MAX_CONVERSION_TRACKS; i++) {
    tmpAUs[i] = StreamFileAllocateAU(0);
  }
  validTracks = validTracksIn;
  allocatedTracks = MAX_CONVERSION_TRACKS;

  /* +++ ep3 -> ep1 +++ */
  if (cfg->action & ACTION_decEp3) {
    int tracks;
    if (cfg->verbose>0) {
      DebugPrintf(0,"converting ep3->ep1 (epTool decode):\n");
      showAccessUnits(validTracks, tmpAUs);
    }
    tracks = epToolDecode(cfg, tmpAUs, validTracks, allocatedTracks);
    if (tracks <= 0) {
      CommonWarning("error while decoding ep3 frame");
      return -1;
    }
    validTracks = tracks;
    paddingKnown = 1;
  }

  /* +++ ep1 -> ep1 (remove extension payload) +++ */
  if ( cfg->classesToRemove>0 ) {
    if (cfg->verbose)
      DebugPrintf(0,"converting ep1->ep1 (removing last %i classes)",cfg->classesToRemove);
    validTracks -= cfg->classesToRemove;
  }
  
  /* +++ ep1 -> ep1 (adding padding) +++ */
  if ( cfg->action & ACTION_alignEp1 ) {
    int tracks;
    if (cfg->verbose) {
      DebugPrintf(0,"converting ep1->ep1 (checking and adding padding):");
      showAccessUnits(validTracks, tmpAUs);
      if (!paddingKnown) DebugPrintf(0, "...padding bit count unknown: expect errors");
    }
    tracks = checkAlignAccessUnits(cfg, tmpAUs, validTracks, allocatedTracks);
    if (tracks<=0) {
      CommonWarning("error while merging access units");
      return -1;
    }
    validTracks = tracks;
    paddingKnown = 1;
  }

  /* +++ ep1 -> ep0 +++ */
  if ( cfg->action & ACTION_merge ) {
    int tracks;
    if (cfg->verbose>0) {
      DebugPrintf(0,"converting ep1->ep0 (concatenate):\n");
      showAccessUnits(validTracks, tmpAUs);
      if (!paddingKnown) DebugPrintf(0, "...padding bit count unknown: expect errors");
    }
    tracks = mergeAccessUnits(cfg, tmpAUs, validTracks, allocatedTracks);
    if (tracks<=0) {
      CommonWarning("error while merging access units");
      return -1;
    }
    validTracks = tracks;
    paddingKnown = 1;
  }

  /* +++ ep0 -> ep1/2 +++ */
  if ( cfg->action & ACTION_split ) {
    int tracks = validTracks;
    if (cfg->verbose>0) {
      DebugPrintf(0,"converting ep0->ep1 (split):\n");
      showAccessUnits(tracks, tmpAUs);
      if (!paddingKnown) DebugPrintf(0, "...padding bit count unknown: expect errors");
    }
    if ((cfg->split_num<=0)&&(cfg->action & ACTION_encEp3)) {
      int predset;
      EPTool_selectPredSet_byLength(cfg->hEpToolEnc, tmpAUs[0]->numBits, &predset, NULL);
      setupSplittingForPredSet(cfg, predset);
    }
    tracks = splitAccessUnits(cfg, tmpAUs, tracks, allocatedTracks);
    if (tracks<=0) {
      CommonWarning("error while merging access units");
      return -1;
    }
    validTracks = tracks;
    paddingKnown = 1;
  }

  /* +++ ep1 -> ep3 +++ */
  if ( cfg->action & ACTION_encEp3 ) {
    int tracks = validTracks;
    if (cfg->verbose>0) {
      DebugPrintf(0,"converting ep1->ep3 (epTool encode):\n");
      showAccessUnits(tracks, tmpAUs);
      if (!paddingKnown) DebugPrintf(0,"...padding bit count unknown: expect errors");
    }
    tracks = epToolEncode(cfg, tmpAUs, tracks, allocatedTracks);
    if (tracks<0) {
      CommonWarning("error while encoding ep3 frame");
      return -1;
    }
    validTracks = tracks;
    paddingKnown = 1;
  }

  /* +++ done +++ */
  if (cfg->verbose) {
    DebugPrintf(0,"...all conversions done:\n");
    showAccessUnits(validTracks, tmpAUs);
  }

  if ((validTracks!=cfg->nOfTracksOut)&&
      (!cfg->epEncWaitForFrames)) {
    CommonWarning("epConvertAccessUnits: oops... created %i tracks while expected to create %i tracks",validTracks,cfg->nOfTracksOut);
    if (validTracks>maxTracksOut) {
      CommonWarning("... now i don't have enough space to write them");
      return -1;
    }
  }

  for (i=0; i<validTracks; i++) {
    if (output[i]==NULL) output[i] = StreamFileAllocateAU(0);
    StreamFile_AUcopyResize(output[i], tmpAUs[i]->data, tmpAUs[i]->numBits);
    StreamFileFreeAU(tmpAUs[i]);
  }
  for (; i<allocatedTracks; i++) {
    StreamFileFreeAU(tmpAUs[i]);
  }
  for (i=validTracks; i<cfg->nOfTracksOut; i++) {
    /*fprintf(stderr,"resize AU %i to zero!\n",i);*/
    StreamFile_AUresize(output[i], 0);
  }

#ifdef DEBUG_EP_CONVERT
  for (i=0; i<validTracks; i++) {
    unsigned char *tmp = output[i]->data;
    unsigned long length = (output[i]->numBits+7) >> 3;
    unsigned long j;
    for (j=0;j<length;j++) {
      DebugPrintf(8,"convert[%4i]: 0x%02x",j,tmp[j]);
    }
  }
#endif

  return validTracks;
}


int EPconvert_expectedOutputClasses(const HANDLE_EPCONVERTER cfg)
{
  if (cfg==NULL) return -1;
  return cfg->nOfTracksOut;
}


void EPconvert_byteAlignOutput(const HANDLE_EPCONVERTER cfg)
{
  if (cfg==NULL) return;
  cfg->action |= ACTION_alignEp1;
}

void EPconvert_setSplitting(HANDLE_EPCONVERTER cfg, int num, int lengths[])
{
  int i;
  if (cfg==NULL) return;
  if (!(cfg->action & ACTION_split)) return;

  cfg->split_num = num;

  for (i=0; i<cfg->split_num; i++) {
    cfg->split_lengths[i] = lengths[i];
  }

  if (!(cfg->action & ACTION_encEp3))
    cfg->nOfTracksOut = cfg->split_num;
}



int EPconvert_numFramesBuffered(HANDLE_EPCONVERTER cfg)
{
  if (cfg==NULL) return -1;
  return cfg->epDecBufferedFrames;
}

int EPconvert_needMoreFrames(HANDLE_EPCONVERTER cfg)
{
  if(cfg==NULL) return -1;
  return cfg->epEncWaitForFrames;
}

/* write an ep specific config as a predefinition file */
static void WriteOutputPredFile(EP_SPECIFIC_CONFIG *esc, const char* filename)
{
  FILE *fp;
  unsigned int i,j;
  
  /* set correct crcLenght values */
  if (esc->headerCrclen.value == 17)  /* look at eptoollib.c L353 */
    esc->headerCrclen.value = 24;
  if (esc->headerCrclen.value == 18)
    esc->headerCrclen.value = 32;
  for (i=0;i<esc->numberOfPredifinedSet.value;i++){
    for (j=0;j<esc->predSetConfig[i].numberOfClass.value;j++){
      if (esc->predSetConfig[i].epClassConfig[j].classCrclen.value == 17)
        esc->predSetConfig[i].epClassConfig[j].classCrclen.value = 24;
      if (esc->predSetConfig[i].epClassConfig[j].classCrclen.value == 18)
        esc->predSetConfig[i].epClassConfig[j].classCrclen.value = 32;
    }
  }

  fp = fopen( filename, "w" );
  fprintf(fp, "%ld                  /* number of predefined sets */\n", esc->numberOfPredifinedSet.value);
  fprintf(fp, "%ld                  /* interleave type */\n", esc->interleaveType.value);
  fprintf(fp, "%ld                  /* bit stuffing */\n", esc->bitStuffing.value);
  fprintf(fp, "%ld                  /* number of concatenated frames */\n", esc->numberOfConcatenatedFrame.value);
  for (j=0;j<esc->numberOfPredifinedSet.value;j++){
    fprintf(fp, "%ld                  /* number of classes */\n", esc->predSetConfig[j].numberOfClass.value);
    for (i=0; i<esc->predSetConfig[j].numberOfClass.value;i++){
      fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].lengthEscape.value  );
      fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].rateEscape.value  );
      fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].crclenEscape.value  );
      if (esc->numberOfConcatenatedFrame.value>1) {
        fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].concatenateFlag.value  );
      }
      fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].fecType.value  );
      if (esc->predSetConfig[j].epClassConfig[i].fecType.value == 0)
        fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].terminationSwitch.value );
      if (esc->interleaveType.value == 2)
        fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].interleaveSwitch.value );
      fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].classOptional.value );
      fprintf(fp, "     /*length_esc, rate_esc, crclen_esc,");
      if (esc->numberOfConcatenatedFrame.value>1) {
        fprintf(fp, " concatenate,");
      }
      fprintf(fp, " fec_type,");
      if (esc->predSetConfig[j].epClassConfig[i].fecType.value == 0)
        fprintf(fp, " termination switch,");
      if (esc->interleaveType.value == 2)
        fprintf(fp, " interleave switch,");
      fprintf(fp, " class_optional */\n");
      if (esc->predSetConfig[j].epClassConfig[i].lengthEscape.value == 1)
        fprintf(fp, "%ld                 /* bits used for class length (0 = until the end) */\n",esc->predSetConfig[j].epClassConfig[i].numberOfBitsForLength.value);
      else fprintf(fp, "%ld                  /* class length */\n",esc->predSetConfig[j].epClassConfig[i].classLength.value);
      if (esc->predSetConfig[j].epClassConfig[i].rateEscape.value != 1)
        fprintf(fp, "%ld                  /* SRCPC: puncture rate 0 = 8/8 ... 24 = 32/8 / SRS: erroneous bytes that can be corrected */\n",esc->predSetConfig[j].epClassConfig[i].classRate.value);
      if (esc->predSetConfig[j].epClassConfig[i].crclenEscape.value != 1)
        fprintf(fp, "%ld                  /* class  crc length */\n",esc->predSetConfig[j].epClassConfig[i].classCrclen.value);
    }
    fprintf(fp, "%ld                  /* class reordered output */\n",esc->predSetConfig[j].classReorderedOutput.value);
    if (esc->predSetConfig[j].classReorderedOutput.value == 1){
      for (i=0; i<esc->predSetConfig[j].numberOfClass.value;i++){
        fprintf(fp, "%ld ",esc->predSetConfig[j].epClassConfig[i].classOutputOrder.value);
      }
      fprintf(fp, "     /* class output order */\n");
    }
  }
  fprintf(fp, "%ld                  /* header protection */\n", esc->headerProtection.value );
  if (esc->headerProtection.value == 1){
    fprintf(fp, "%ld                  /* header rate */\n", esc->headerRate.value );
    fprintf(fp, "%ld                  /* header crclen */\n", esc->headerCrclen.value );
  }
  fclose(fp);
  /* set back */
  if (esc->headerCrclen.value == 24)
    esc->headerCrclen.value = 17;
  if (esc->headerCrclen.value == 32)
    esc->headerCrclen.value = 18;
  for (i=0;i<esc->numberOfPredifinedSet.value;i++){
    for (j=0;j<esc->predSetConfig[i].numberOfClass.value;j++){
      if (esc->predSetConfig[i].epClassConfig[j].classCrclen.value == 24)
        esc->predSetConfig[i].epClassConfig[j].classCrclen.value = 17;
      if (esc->predSetConfig[i].epClassConfig[j].classCrclen.value == 32)
        esc->predSetConfig[i].epClassConfig[j].classCrclen.value = 18;
    }
  }
}

