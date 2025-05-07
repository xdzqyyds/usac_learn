#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libeptool.h"
#include "calcoverhead.h"
#include "eptool.h"
#include "concat.h"

/* now... this one is pretty useless... but it's so great i'll leave it in
   here, in case of a bored programmer needs something to laugh at */
#define D(a) {const void *dummyfilepointer; dummyfilepointer = &a;}


typedef struct tagEpConfig {
  EPConfig epConfig;            /* modified perdefinition set */
  EPConfig origEpConfig;        /* original predefinition set */
  int classAvailableTable[MAXPREDwCA][CLASSMAX];
  int classAvailableBits [MAXPREDwCA];
  int origPredForCAPred[MAXPREDwCA];
  /* pointer to arrays managed by HANDLE_CLASS_FRAME */
  /* ...really a GREAT idea... :( */
  unsigned char    **classBuffer;            /* to avoid memory holes ;-) */
  long              *classLength;            /* to avoid memory holes ;-) */
  long              *classCodeRate;          /* SRCPC code rate */
  long              *classCRCLength;         /* CRC length for each class */
  short             *crcError;               /* to avoid memory holes ;-) */

  HANDLE_CONCAT_BUFFER concat_buffer;        /* there be concatenation stuff */
  /* the crcErrorFlags, transport- origpred set used for the frame currently
     stored in the concatenation buffer.
     storage for a maximum of 8 concatenated frames */
  int crcDecodeError[8][CLASSMAX];
  int predInConcatBuffer;
} T_EP_CONFIG;

static void EPTool_BytesToBits ( const unsigned char *inbuf, int *trans, int translen );
static int  EPTool_BitsToBytes ( unsigned char *outbuf, const int *trans, int translen );
static int  EPTool_BytesToBits_IntToChar( const unsigned int *inbuf, unsigned char *outbuf, int bitlen );
static int  EPTool_BitsToBytes_CharToInt( const unsigned char *inbuf, int *outbuf, int bitlen );
static void EPTool_BytesToBits_CharToChar( const unsigned char *inbuf, unsigned char *outbuf, int bitlen );
static int  EPTool_BitsToBytes_CharToChar( unsigned char *outbuf, const unsigned char *inbuf, int bitlen );

/* these allocate the pieces in HANDLE_EP_CONFIG */
static int  FrameHandle_Create ( HANDLE_EP_CONFIG inFrame, int bitFlag );
static int  FrameHandle_Destroy( HANDLE_EP_CONFIG inFrame );

/* these really alloc and destroy a new HANDLE_CLASS_FRAME */
static HANDLE_CLASS_FRAME ClassFrame_Create(const HANDLE_EP_CONFIG inFrame, int bitFlag );
static void ClassFrame_Destroy(HANDLE_CLASS_FRAME classFrame );

static int EPTool_GetMaximumNumberOfClassesInternal(HANDLE_EP_CONFIG hEpConfig);
static int EPTool_ParseEPSpecificConfig( unsigned char *inbuffer, long int configLength, HANDLE_EP_CONFIG hEpConfig );
static long int EPTool_WriteEPSpecificConfig( unsigned char *outbuffer, HANDLE_EP_CONFIG hEpConfig );

static int EPTool_checkPredSet_forClassFrame(HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME buffer, int *choiceOfTransPred);


int EPTool_CreateExpl (HANDLE_EP_CONFIG  *hEpConfig,
                       const char        *predFileName,
                       unsigned char    **configBuffer, /* output */
                       long              *configLength,  /* in bit */
                       int                reorderFlag,
                       int                bitFlag
                       )
{
  FILE *fp=NULL;
  int fileParse = 0;

  if (NULL != predFileName ){
    if (NULL == (fp = fopen(predFileName, "r"))) {
      fprintf(stderr,"EPTool: EPTool_CreateExpl(): Error: predefinition file not found\n");
      return 1;
    }
    fileParse = 1;
  } else {
    if (NULL == configBuffer || NULL == *configBuffer){
      fprintf(stderr, "EPTool: EPTool_CreateExpl(): Error: configBuffer not available\n");
      return 1;
    }
  }

  /* memory allocation */
  *hEpConfig = (HANDLE_EP_CONFIG) calloc(1, sizeof(T_EP_CONFIG));
  
  def_GF();                     /* generate GF(2^8) tables*/

  if ( fileParse ){
    OutInfoParse(fp, &((*hEpConfig)->origEpConfig), 0, (*hEpConfig)->classAvailableTable, (*hEpConfig)->classAvailableBits, 1);
  } else {
    if(EPTool_ParseEPSpecificConfig( *configBuffer, *configLength, *hEpConfig )){
      free(hEpConfig);
      return -1;
    }
  }
  PredSetConvert(&((*hEpConfig)->origEpConfig), &((*hEpConfig)->epConfig), (*hEpConfig)->classAvailableTable , (*hEpConfig)->classAvailableBits, (*hEpConfig)->origPredForCAPred, reorderFlag);

  FrameHandle_Create ( *hEpConfig, bitFlag );

  {
    int i, origpred = (*hEpConfig)->origEpConfig.NPred - 1;

    for (i=0; i< (*hEpConfig)->epConfig.fattr[origpred].ClassCount; i++) {
      (*hEpConfig)->classCodeRate[i]  = (*hEpConfig)->epConfig.fattr[origpred].cattr[i].ClassCodeRate ;
      (*hEpConfig)->classCRCLength[i] = (*hEpConfig)->epConfig.fattr[origpred].cattr[i].ClassCRCCount ;
    }
  }

  (*hEpConfig)->concat_buffer = concatenation_create(&((*hEpConfig)->epConfig));
  (*hEpConfig)->predInConcatBuffer = -1;

  if(fileParse){
    if((configBuffer != NULL) && (*configBuffer !=NULL) && (configLength != NULL)){
      *configLength = EPTool_WriteEPSpecificConfig(*configBuffer, *hEpConfig);
      fclose(fp);
    } else {
      fprintf( stderr, "EP_Tool: EPTool_CreateExpl(): libeptool.c: Failed writing to configBuffer\n");
      return 0;
    }
  }
  return 0;
}

/* modify epConfig to fill in the escaped values for one frame */
static void createEncoderFrameAttrib(
	const EPFrameAttrib *orig_attrib,  /* original attributes of frame */
	int NConcat,                       /* number of concatenated frames */
	const HANDLE_CLASS_FRAME inBuffer, /* escaped length/codeRate/crcLen */
	EPFrameAttrib *out_attrib,         /* storage for the output */
	int *codecClassForEpClass)         /* storage for reordering info */
{
  int codec_idx, epframe_idx;
  int outattr_idx=0;

  out_attrib->TotalData = 0;
  out_attrib->TotalCRC = 0;
  for (epframe_idx=0; epframe_idx<orig_attrib->ClassCount; epframe_idx++) {
    for (codec_idx=0; codec_idx<orig_attrib->ClassCount; codec_idx++) {
      int searchForClass;
      if (!orig_attrib->ClassReorderedOutput) searchForClass = epframe_idx;
      else searchForClass=orig_attrib->ClassOutputOrder[epframe_idx];

      if (codec_idx==searchForClass) {
        /* add the attributes... */
        int classlen, codeRate, crc_len;
        int num_copies;

        if ((NConcat>1)&&(orig_attrib->cattr[codec_idx].Concatenate!=0)) {
          /* if we use concatenation... */
          classlen = NConcat * orig_attrib->cattr[codec_idx].ClassBitCount;
          codeRate = orig_attrib->cattr[codec_idx].ClassCodeRate;
          crc_len  = orig_attrib->cattr[codec_idx].ClassCRCCount;
          num_copies = 1;
        } else {
          /* length or escaped length */
          if (orig_attrib->cattresc[codec_idx].ClassBitCount)
               classlen = inBuffer->classLength[codec_idx];
          else classlen = orig_attrib->cattr[codec_idx].ClassBitCount;

          /* code rate or escaped code rate */
          if (orig_attrib->cattresc[codec_idx].ClassCodeRate)
               codeRate = inBuffer->classCodeRate[codec_idx];
          else codeRate = orig_attrib->cattr[codec_idx].ClassCodeRate;

          /* crc len or escaped crc len */
          if (orig_attrib->cattresc[codec_idx].ClassCRCCount)
               crc_len = inBuffer->classCRCLength[codec_idx];
          else crc_len = orig_attrib->cattr[codec_idx].ClassCRCCount;

          num_copies = NConcat;
        }

        for (; num_copies>0; num_copies--) {
          codecClassForEpClass[outattr_idx] = codec_idx;
          out_attrib->cattr[outattr_idx].ClassBitCount = classlen;
          out_attrib->cattr[outattr_idx].ClassCodeRate = codeRate;
          out_attrib->cattr[outattr_idx].ClassCRCCount = crc_len;
          out_attrib->cattr[outattr_idx].Concatenate =
                orig_attrib->cattr[codec_idx].Concatenate;
          out_attrib->cattr[outattr_idx].FECType =
                orig_attrib->cattr[codec_idx].FECType;
          out_attrib->cattr[outattr_idx].TermSW =
                orig_attrib->cattr[codec_idx].TermSW;
          out_attrib->cattr[outattr_idx].ClassInterleave =
                orig_attrib->cattr[codec_idx].ClassInterleave;

          /* do not create cattresc, most probably noone will ever use it */
          /*...well GenerateHeader() does... :( */
          out_attrib->cattresc[outattr_idx].ClassBitCount =
                orig_attrib->cattresc[codec_idx].ClassBitCount;
          out_attrib->cattresc[outattr_idx].ClassCRCCount =
                orig_attrib->cattresc[codec_idx].ClassCRCCount;
          out_attrib->cattresc[outattr_idx].ClassCodeRate =
                orig_attrib->cattresc[codec_idx].ClassCodeRate;

          /* damn... GenerateHeader() will also need lenbit :( */
          out_attrib->lenbit[outattr_idx] =
                orig_attrib->lenbit[codec_idx];

          out_attrib->TotalData += out_attrib->cattr[outattr_idx].ClassBitCount;
          out_attrib->TotalCRC += out_attrib->cattr[outattr_idx].ClassCRCCount;
          outattr_idx++;
        }
      }
    }
  }

  out_attrib->ClassCount = outattr_idx;
  /* we probably need those in PayloadEncode() somewhere */
  out_attrib->TotalIninf = orig_attrib->TotalIninf;
  out_attrib->UntilTheEnd = orig_attrib->UntilTheEnd;

  /* we definitely won't need these! reordering is done as very first thing */
  out_attrib->ClassReorderedOutput = 0;
}

/* we have read the transport pred set number, now get the frame attributes of
   the frame. we do respect concatenation and reordering here. we do not handle
   optional classes, that part is already done in PredSetConvert() */
static void createDecoderFrameAttrib(const EPConfig *epcfg, int pred, EPFrameAttrib *out_fattr, int *class_index)
{
  int i;
  int in_class, out_class;

  /* have to calculate these again, the decode-header things don't know of
     concatenation */
  out_fattr->TotalData = 0; /* total output bits */
  out_fattr->TotalCRC = 0;  /* total crc bits */

  /* create out_fattr from epcfg */
  out_class = 0;
  for(i=0; i<epcfg->fattr[pred].ClassCount; i++) {
    int repeat;
    int len_multiply;

    /* find the matching in_class for the output class index i */
    if (epcfg->fattr[pred].ClassReorderedOutput!=0) {
      in_class = epcfg->fattr[pred].ClassOutputOrder[i];
    } else {
      in_class = i;
    } 

    /* how many instances of this class? */
    if ((epcfg->NConcat>1)&&(epcfg->fattr[pred].cattr[in_class].Concatenate==0)) {
      repeat = epcfg->NConcat;
      len_multiply = 1;
    } else {
      repeat = 1;
      len_multiply = epcfg->NConcat;
    }

    for (;repeat>0; repeat--) {
      out_fattr->cattr[out_class].ClassBitCount =
          epcfg->fattr[pred].cattr[in_class].ClassBitCount * len_multiply;
      out_fattr->cattr[out_class].ClassCodeRate =
          epcfg->fattr[pred].cattr[in_class].ClassCodeRate;
      out_fattr->cattr[out_class].ClassCRCCount =
          epcfg->fattr[pred].cattr[in_class].ClassCRCCount;
      out_fattr->cattr[out_class].Concatenate =
          epcfg->fattr[pred].cattr[in_class].Concatenate;
      out_fattr->cattr[out_class].FECType =
          epcfg->fattr[pred].cattr[in_class].FECType;
      out_fattr->cattr[out_class].TermSW =
          epcfg->fattr[pred].cattr[in_class].TermSW;
      out_fattr->cattr[out_class].ClassInterleave =
          epcfg->fattr[pred].cattr[in_class].ClassInterleave;
      out_fattr->ClassOutputOrder[out_class] = out_class;
      class_index[out_class] = in_class;

      out_fattr->TotalData += out_fattr->cattr[out_class].ClassBitCount;
      out_fattr->TotalCRC += out_fattr->cattr[out_class].ClassCRCCount;

      out_class++;
    }
    /* cattresc is skipped... */
  }

  out_fattr->ClassCount = out_class;
  /* we probably need those in PayloadDecode() */
  out_fattr->TotalIninf = epcfg->fattr[pred].TotalIninf;
  out_fattr->UntilTheEnd = epcfg->fattr[pred].UntilTheEnd;
  /* most likely this one is not needed */
  out_fattr->lenbit = epcfg->fattr[pred].lenbit;

  /* we will NOT need this one! */
  out_fattr->ClassReorderedOutput = 0;
}

/** bring the integers coming from PayloadDecode() to something readable by
    the codec.
    @param class_num number of classes coming from PayloadDecode()
    @param data input data as from PayloadDecode()
    @param crc_error indicator of CRC-error as coming from PayloadDecode()
    @param class_index the i-th block of data is using attrib for class i. coming from one of the createConfig functions
    @param frame_attrib attributes for the frame as coming from the same createConfig function
    @param out_data array of unallocated pointers where the output classes are written
    @param out_length allocated array where the output class lengths are written
    @param out_crc_error will store the (reordered) crc-error-indicator for each class of concatenated frame
*/
static void DecodeReordering(
	int class_num,
	const int *data,
	const int *crc_error,
	const int *class_index,
	const EPFrameAttrib *frame_attrib,
	unsigned char **out_data,
	long *out_length,
	int *out_crc_error)
{
  int concatenated_class = 0;
  int epframe_class;
  int codec_class;

  unsigned long offsets[CLASSMAX+1];
  /* build an offset table, telling us for epframe-class[i] where in the data
     to look for it. use class_index to do so, it will tell us for the i-th
     block in data, which class attributes apply. */
  {
    unsigned long offset = 0;
    for (epframe_class=0; epframe_class<class_num; epframe_class++) {
      offsets[epframe_class] = offset;
      offset += frame_attrib->cattr[epframe_class].ClassBitCount;
    }
  }

  /* go through all codec-classes... funny loop btw. :) */
  for (codec_class=0; concatenated_class<class_num; codec_class++) {
    /* find all epFrame-blocks for this codec class (may be multiple if
       NConcat>1 and cattr.Concatenate==0)
    */
    for (epframe_class=0; epframe_class<class_num; epframe_class++) {
      if (class_index[epframe_class]==codec_class) {
        int class_len_bytes;
        out_length[concatenated_class] = frame_attrib->cattr[epframe_class].ClassBitCount;
        class_len_bytes = (out_length[concatenated_class]+7)/8;

        out_data[concatenated_class] = malloc(class_len_bytes);
        EPTool_BitsToBytes(out_data[concatenated_class], &data[offsets[epframe_class]], class_len_bytes * 8);
        out_crc_error[concatenated_class] = crc_error[epframe_class];

        concatenated_class++;
    }
  }
}
}


/* reorder the class data in a way, that the classes in codec-order are put
   into epFrame-order. convert bytes to bits as well, and reorder the lengths */
static void EncodeReordering(
	int numClasses,
	const unsigned char **class_data,
	const long *class_length,
	const int *classIsCodecClass,
	const int *codecClassForEpClass,
	int *trans_data,
	long *trans_length)
{
  unsigned long offset;
  long orig_len[CLASSMAX];
  int orig_order[CLASSMAX];
  int codec_idx, epframe_idx;

  for (codec_idx=0; codec_idx<numClasses; codec_idx++) {
    orig_len[codec_idx] = class_length[codec_idx];
    orig_order[codec_idx] = classIsCodecClass[codec_idx];
  }

  offset = 0;
  for (epframe_idx=0; epframe_idx<numClasses; epframe_idx++) {
    for (codec_idx=0; codec_idx<numClasses; codec_idx++) {
      if (orig_order[codec_idx] == codecClassForEpClass[epframe_idx]) {
        orig_order[codec_idx] = -1;
        trans_length[epframe_idx] = orig_len[codec_idx];

        /* write bytes to bits */
        EPTool_BytesToBits(class_data[codec_idx], &(trans_data[offset]), trans_length[epframe_idx]);
        offset += trans_length[epframe_idx];
      } 
    }
  }
}


/* FIXME:
   this one is BROKEN, at least for NConcat!=1, but hardly makes sense then
   anyway. it is untested anyway. so fasten your seatbelt, before trying!
*/
int EPTool_CalculateOverhead( HANDLE_EP_CONFIG hEpConfig ,  HANDLE_CLASS_FRAME inBuffer, int *outLength)
{
  int pred = 0, hlen = 0, h1len = 0, h2len = 0, bitstuffing = 0, dlen = 0;
  int nclass, nstuff, interleave, total;
  int i;
  int codedlen [CLASSMAX] = {0};
  int origpred;
  EPFrameAttrib frame_attrib;

  if (hEpConfig->epConfig.NConcat!=1) {
    fprintf(stderr, "EPTool_CalculateOverhead() is broken for epConfig.NConcat!=1\n");
  }

  if (inBuffer->choiceOfPred!=-1) {
    if ((i=EPTool_checkPredSet_forClassFrame(hEpConfig, inBuffer, &pred))) return i;
    origpred = inBuffer->choiceOfPred;
  } else {
    if ((i=EPTool_selectPredSet_forClassFrame(hEpConfig, inBuffer, &origpred, &pred))) return i;
  }

  /* FIXME: should do a reordering here!!! */

  bitstuffing = hEpConfig->epConfig.BitStuffing;

  /* modify epConfig to fill in the escaped values */
  frame_attrib.cattr = malloc(inBuffer->nrValidBuffer*sizeof(ClassAttribContent));
  frame_attrib.cattresc = malloc(inBuffer->nrValidBuffer*sizeof(ClassAttribContent));
  frame_attrib.lenbit = malloc(inBuffer->nrValidBuffer*sizeof(int));
  {
    int codecClassForEpClass[CLASSMAX];
    createEncoderFrameAttrib(&hEpConfig->epConfig.fattr[pred], /*hEpConfig->epConfig.NConcat*/1, inBuffer, &frame_attrib, codecClassForEpClass);
  }

  hlen = CalculateHeaderOverhead(&hEpConfig->epConfig, pred, &h1len, &h2len, bitstuffing);

  nclass = frame_attrib.ClassCount;
  interleave = hEpConfig->epConfig.Interleave; 

  dlen = 0;
  for (i=0; i< nclass; i++) {
    dlen += frame_attrib.cattr[i].ClassBitCount ;
  }

  total =  CalculatePayloadOverhead(dlen, &frame_attrib, codedlen);

  if(bitstuffing) {
    nstuff = ((total + hlen + 7) /8)*8 - (total + hlen);
    hlen = CalculateHeaderOverhead(&hEpConfig->epConfig, pred, &h1len, &h2len, bitstuffing);
    total += nstuff;
  } else {
    nstuff = 0;
  }
  
  *outLength = h1len + h2len + total;
  
  free(frame_attrib.cattr);
  free(frame_attrib.cattresc);
  free(frame_attrib.lenbit);

  return 0;
}

int EPTool_EncodeFrame( HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME inBuffer, unsigned char *outBuffer, int *outLength , int bitFlag)
{
  int pred, hlen, bitstuffing;
  int nphdr[MAXHEADER], bdyhdr[MAXHEADER] ;
  int nstuff, interleave, total;
  int i, j, k;
  static int coded [MAXCODE] = {0};
  int codedlen [CLASSMAX] = {0}, codedstart[CLASSMAX] = {0};
  int clen, nclen, cptr;
  int trans[MAXDATA*2], translen = 0;
#if RS_FEC_CAPABILITY
  int rs_index[256], rs_codelen;
  unsigned int rs_code[256];
  int rs_code_bin[256*8];
  int rs_parity[127*(MAXDATA/255)];
#endif
  int origpred;
  headerLength h1len, h2len; /* header lengths for header part choiceOfPred + choiceOfPredParity, classAttrib + classAttrobParity */

  /* for concatenation */
  int class_buf_num;
  int class_buf_id[CLASSMAX];
  long class_buf_len[CLASSMAX];
  unsigned char *class_buf[CLASSMAX];

  HANDLE_CLASS_FRAME shortenedFrame;
  EPFrameAttrib frame_attrib;

  /* determine the pred set used in this frame */
  if (inBuffer->choiceOfPred!=-1) {
    if ((i=EPTool_checkPredSet_forClassFrame(hEpConfig, inBuffer, &pred))) return i;
    origpred = inBuffer->choiceOfPred;
  } else {
    if ((i=EPTool_selectPredSet_forClassFrame(hEpConfig, inBuffer, &origpred, &pred))) return i;
  }

  /* umm... looks like a sanity check, doesn't it?
     but does it make sense as well?
  */
  bitstuffing = hEpConfig->epConfig.BitStuffing;
  if (!bitstuffing && !bitFlag){
    fprintf(stderr, "EP_Tool: EPTool_EncodeFrame(): Error: Your in- and output is in bytes and *not* in real bits. Therefore you have to use the bitstuffing-functionality. If you change your in- and output, please change the bitFlag, too!\n");
    return -1;
  }

  /* create a new "shortenedFrame":
     - throw out optional classes!
     - if bitFlag, convert to bytes
     - do not yet reorder the frame here! do it after the concat-buffer
     store in class_buf
  */
  {
    int out_idx;
    int optionalClasses;
    int *classOptional = hEpConfig->classAvailableTable[origpred];
    shortenedFrame = ClassFrame_Create(hEpConfig, 0);
    shortenedFrame->nrValidBuffer = hEpConfig->epConfig.fattr[pred].ClassCount;

    optionalClasses=0;
    for (i=0, out_idx=0; i<inBuffer->nrValidBuffer; i++) {
      int putClass=1;
      if (classOptional[i]==1) {
        if (pred & (1<<optionalClasses)) putClass=0;
        optionalClasses++;
      }
      if (!putClass) continue;
      shortenedFrame->classCodeRate[out_idx] = inBuffer->classCodeRate[i];
      shortenedFrame->classCRCLength[out_idx] = inBuffer->classCRCLength[i];
      shortenedFrame->classLength[out_idx] = inBuffer->classLength[i];
      if (bitFlag) {
        EPTool_BitsToBytes_CharToChar(shortenedFrame->classBuffer[out_idx], (const unsigned char*)inBuffer->classBuffer[i], ((shortenedFrame->classLength[out_idx]+7)/8)*8);
      } else {
        memcpy(shortenedFrame->classBuffer[out_idx], inBuffer->classBuffer[i], (shortenedFrame->classLength[out_idx]+7)/8);
      }
      out_idx++;
    }

    /* put the new frame into the concatenation buffer */
    i=concatenation_add(pred, hEpConfig->concat_buffer, shortenedFrame);
  }

  if (i<0) {
    /* especially if i==-2 (i.e. the concat-buffer is full)
       something went really wrong! */
    ClassFrame_Destroy(shortenedFrame);
    fprintf(stderr, "oops... something went wrong with the concatenation tool\n");
    return -1;
  } else if (i>0) {
    /* we still need i frames, until we can start encoding */
    ClassFrame_Destroy(shortenedFrame);
    *outLength=0;
    return 2;
  }

  /* else the concat_buffer just got full, start encoding */
  class_buf_num = concatenation_classes_after_concat(hEpConfig->concat_buffer);
  for (i=0; i<class_buf_num; i++) class_buf[i] = NULL;
  concatenation_concat_buffer(hEpConfig->concat_buffer, class_buf, class_buf_len, class_buf_id);

  /* create new frame attributes from epConfig to:
     - fill in the escaped values
     - reorder the classes
  */

  {
    int codecClassForEpClass[CLASSMAX];

    frame_attrib.cattr = malloc(class_buf_num*sizeof(ClassAttribContent));
    frame_attrib.cattresc = malloc(class_buf_num*sizeof(ClassAttribContent));
    frame_attrib.lenbit = malloc(class_buf_num*sizeof(int));
    createEncoderFrameAttrib(&hEpConfig->epConfig.fattr[pred], hEpConfig->epConfig.NConcat, shortenedFrame, &frame_attrib, codecClassForEpClass);
    ClassFrame_Destroy(shortenedFrame);

    /* we now know the attributes for the concatenated classes...
       the real (escaped) lengths are correctly stored in frame_attrib
       but the frame yet has to be reordered...
    */

    /* for reordering the frame we have:
       - codecClassForEpClass[i] says, which codec class is in epFrame-class[i]
       - class_buf_id[i] gives the codec class stored in class_buf[i]

       - also we have to reorder class_buf_len
       - and convert bytes to bits
    */

    EncodeReordering(frame_attrib.ClassCount, (const unsigned char**)class_buf, class_buf_len, class_buf_id, codecClassForEpClass, trans, class_buf_len);

    for (i=0; i<class_buf_num; i++) free(class_buf[i]);
  }

  hlen = GenerateHeader (&hEpConfig->epConfig, &frame_attrib, pred, nphdr, &h1len, bdyhdr, &h2len, bitstuffing, 0 );

  interleave = hEpConfig->epConfig.Interleave; 

  for (i=0; i<class_buf_num; i++) {
    translen += class_buf_len[i];
  }

  /* check, if at least inputbuffers are sufficent */
  if(translen > MAXDATA){
    fprintf(stderr, "EPTool: EPTool_EncodeFrame: Inputlength exceeds max. Buffersize. Please increase MAXDATA in eptool.h.");
    exit(-1);
  }

      
  /* we pass some fantasy-classes with appropriate class attributes to the
     PayloadEncode() function, which are the result of:
     - removing optional classes [done before moving to concat tool]
     - concatenating the frames [done by concatenation_concat_buffer()]
     - filling in the escaped values [done by createEncoderFrameAttrib()]
     - reordering the classes [done in EncodeReordering()]
  */

  total =  PayloadEncode(trans, translen, &frame_attrib, coded, codedlen);

  /* there be dragons... */
  if(bitstuffing) {
    nstuff = ((total + hlen + 7) /8)*8 - (total + hlen); 
    hlen = GenerateHeader(&hEpConfig->epConfig, &frame_attrib, pred, nphdr, &h1len, bdyhdr, &h2len, bitstuffing, nstuff);
    for(i=0; i<nstuff;i++){
      coded[total+i] = 0;
    }
    total += nstuff;
  } else {
    nstuff = 0;
  }

  /* the following has NOTHING to do with concatenation, just someone thought
     it was clever to rename interleaving to concatenation */

  if(interleave) {
    int *tmp3;
    if(NULL == (tmp3 = (int *)calloc(sizeof(int), total))){
      perror("EP_Tool: EPTool_EncodeFrame(): tmp3"); exit(1);
    }
    clen = nclen = cptr = 0;

    translen = 0;
    codedstart[0] = 0;
    for(i=0; i<class_buf_num-1; i++)
      codedstart[i+1] = codedstart[i] + codedlen[i];

    /* Concat Non-Interleaved classes */
    for(i=0; i<class_buf_num; i++) {
      if (frame_attrib.ClassReorderedOutput)
        k = frame_attrib.OutputIndex[i] ;
      else
        k = i;
      if(!frame_attrib.cattr[k].ClassInterleave) {
        for(j=0; j<codedlen[i]; j++)
          tmp3[nclen++] = coded[codedstart[i]+j];
      }
    }
    
    /* Concat "Concatenation:3" classes */
    for(i=0; i<class_buf_num; i++) {
      if (frame_attrib.ClassReorderedOutput)
        k = frame_attrib.OutputIndex[i] ;
      else
        k = i;
      if(frame_attrib.cattr[k].ClassInterleave == 3) {
        for(j=0; j<codedlen[i]; j++)
          trans[translen++] = coded[codedstart[i]+j];
        /*      printf("translen=%d\n",translen); */
      }
    }

    /* Interleave classes */
    for(i=class_buf_num-1; i>=0; i--){
      int Interleave;
      if (frame_attrib.ClassReorderedOutput)
        k = frame_attrib.OutputIndex[i] ;
      else
        k = i;
      Interleave = frame_attrib.cattr[k].ClassInterleave;
      if(frame_attrib.cattr[k].FECType==0) {
        if(Interleave == 1){ /* Intra w/o Inter */
          translen = recinter(&coded[codedstart[i]], codedlen[i], codedlen[i], trans, translen, trans);
          cptr += codedlen[i];
        }else if(Interleave == 2){ /* Intra w/ Inter */
          translen = recinter(&coded[codedstart[i]], codedlen[i], PINTDEPTH, trans, translen, trans);
          cptr += codedlen[i];
        }
      }
      else { 
        if(Interleave == 1 || Interleave == 2){
          /* bytewise interleave for RS  */
          translen = recinterBytewise(&coded[codedstart[i]], codedlen[i], frame_attrib.cattr[k].ClassCodeRate,trans, translen, trans);
          cptr += codedlen[i];
        }
      }
    }

    for(i=0; i<nstuff; i++)  
      tmp3[nclen++] = 0; 
      
    for(i=0; i<nclen; i++) {
      trans[translen++] = tmp3[i];
    }
    translen = recinter(bdyhdr, h2len.headerLen, h2len.interleavingWidth, trans, translen, trans);
    translen = recinter(nphdr, h1len.headerLen, h1len.interleavingWidth, trans, translen, trans);
    
    free(tmp3);
  } else {
    for(i=j=0; i<h1len.headerLen; i++)
      trans[j++] = nphdr[i];
    for(i=0; i<h2len.headerLen; i++)
      trans[j++] = bdyhdr[i];
    for(i=0; i<total; i++)
      trans[j++] = coded[i]; /* there are the stuffin bits also in ... in case there is _no_ interleaving 020410 multrums@iis.fhg.de*/
    translen = j;
  }

  /* well... i really have no clue, but reading this it is safe to assume
     the RS_FEC_CAPABILITY of this epTool is broken */
  if (bitFlag){
    for(i=0;i<translen;i++)
      outBuffer[i] = trans[i];
  } else {
  EPTool_BitsToBytes (outBuffer, trans, translen);
  }

  free(frame_attrib.cattr);
  free(frame_attrib.cattresc);
  free(frame_attrib.lenbit);

#if RS_FEC_CAPABILITY
  /* RS Encoding */
  if(hEpConfig->epConfig.RSFecCapability){
    int e_target, rs_inflenmax;
    int nparity;
    int pcnt, icnt;
    int inflen;

    /* Initialize tables for RS encoder */
    make_genpoly(hEpConfig->epConfig.RSFecCapability); /* generate generator poly */

    e_target = hEpConfig->epConfig.RSFecCapability;
    rs_inflenmax = 255 - 2 * e_target;
    nparity = (translen/8 + rs_inflenmax - 1) / rs_inflenmax;
    pcnt = icnt = 0;

    /* if(nparity != 1) fprintf(stderr, "EP_Tool: EPTool_EncodeFrame(): Parity %d\n", nparity);*//* what's that??? */
    for(i=0; i< nparity; i++){
      if(i == nparity - 1) /*last*/
        inflen = (translen - icnt)/8;
      else
        inflen = rs_inflenmax;
      /*      printf("[inflen = %d/%d]", inflen, translen/8); */

      rs_codelen = RS_input(&trans[icnt], rs_index, inflen*8);
      rs_codelen = RS_enc(rs_index, rs_codelen, rs_code, e_target);
      rs_codelen = RS_output(rs_code, rs_code_bin, rs_codelen, e_target, 0);

      for(j=0; j<inflen*8; j++){
        if(rs_code_bin[j] != trans[icnt+j])
          fprintf(stderr, "EP_Tool: EPTool_EncodeFrame(): !BUG!\n");
      }
      for(j=0; j<2*e_target*8; j++){
        rs_parity[pcnt++] = rs_code_bin[inflen*8+j];
      }

      icnt += inflen*8;
    }
    if(icnt != translen) fprintf(stderr, "EP_Tool: EPTool_EncodeFrame(): <%d,%d>\n", icnt,translen);

    if(bitFlag){
      for(i=0;i<pcnt;i++)
        outBuffer[translen+i] = rs_parity[i];
    } else {
    EPTool_BitsToBytes (&outBuffer[(translen+7)/8], rs_parity, pcnt);
    }
    
    translen += pcnt;
  }
#endif
  
  *outLength = translen ; 
  
  return 0;
}


int EPTool_DecodeFrame( HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME outBuffer, unsigned char *inBuffer, int inLength, int bitFlag)
{
  int i, j;
  int trans[MAXDATA*2];
  int translen = inLength;
#if RS_FEC_CAPABILITY
  int rs_index[256], rs_codelen, rs_detect;
  unsigned int rs_out[256];
  int rs_data[256*8];
#endif
  int hlen, pred, nclass, nstuff = 0, clen = 0;
  static int ibuf[MAXDATA*2];
  static int ncbuf[MAXDATA];
  static int coded[MAXCODE];
  static int data[MAXDATA];
  int payloadlen;
  int check[CLASSMAX];
  int codedlen[CLASSMAX];
  int bound_punc[CLASSMAX];
  int class_index[CLASSMAX];

  EPFrameAttrib frame_attrib;
  frame_attrib.cattr = malloc(CLASSMAX*sizeof(ClassAttribContent));
  frame_attrib.cattresc = malloc(CLASSMAX*sizeof(ClassAttribContent));

  /* check at least buffersize for inputlength */
  if(inLength > MAXCODE){
    fprintf(stderr, "EPTool: EPTool_DecodeFrame: Inputlength exceeds max. buffersize. Please increase MAXCODE in eptool.h.");
    exit(-1);
  }

  if (bitFlag){
    for (i=0; i<inLength; i++)
      trans[i] = inBuffer[i];
  } else {
    EPTool_BytesToBits(inBuffer, trans, ((inLength+7)/8)*8);
  }

  if (!hEpConfig->epConfig.BitStuffing && !bitFlag){ 
    fprintf(stderr, "EP_Tool: EPTool_DecodeFrame(): Error: Your in- and output is in bytes and *not* in real bits. Therefore you have to use the bitstuffing-functionality. If you change your in- and output, please change the bitFlag, too!\n"); 
    exit(1); 
   } 

#if RS_FEC_CAPABILITY
  if (hEpConfig->epConfig.RSFecCapability) {
    int e_target, rs_inflenmax;
    int nparity, lastinflen;
    int inflen;
    int icnt, dcnt, pcnt;

    /* Initialize tables for RS encoder */
    make_genpoly(hEpConfig->epConfig.RSFecCapability); /* generate generator poly */

    e_target = hEpConfig->epConfig.RSFecCapability;
    rs_inflenmax = 255 - 2 * e_target;

    nparity = (translen/8 + 254) / 255;
    lastinflen = translen/8 - 255*(nparity-1) - 2*e_target;
    if(nparity != 1) fprintf(stderr, "EPTool: EPTool_DecodeFrame(): Parity %d\n",nparity);

    pcnt = translen - 2*e_target*8*nparity;
    icnt = 0;
    for(i=0; i<nparity; i++){
      dcnt = 0;
      inflen = (i == nparity-1 ? lastinflen : rs_inflenmax);
      /*      printf("[%d/%d]", inflen, translen/8);*/
      
      for(j=0; j< inflen*8; j++)
        rs_data[dcnt++]= trans[icnt + j]; 
      for(j=0; j<2*e_target*8; j++)
        rs_data[dcnt++] = trans[pcnt++];
      
      
      rs_codelen = RS_input(rs_data, rs_index, dcnt);
      rs_detect = RS_dec(rs_index, rs_out, rs_codelen, e_target);
      rs_codelen = RS_output(rs_out, rs_data, rs_codelen, e_target, 1);
      
      for(j=0; j< rs_codelen; j++){
        trans[icnt + j] = rs_data[j];
      }
      
      icnt += j;
    }
    translen -=  2*e_target*8 * nparity;
  }
#endif

  if (hEpConfig->epConfig.Interleave) {
    int *tmp1,*tmp2;
    int nclen, sptr, nptr, jcnt;

    int SRCPCStart  = 0;
    int rsconcat    = 0;
    int rsconcatlen = 0;

    /* transport pred is specified in header, so we just pass the transport
       pred sets and all will work out!
       the escaped values are stored in the original pred set, which at the
       least is absolutely not nice style. that one was not my idea! :)
       luckily escaping and concatenation are mutually exclusive
    */
    hlen = RetrieveHeaderDeinter(&hEpConfig->epConfig, &pred, trans, translen, ibuf, &nstuff);
    /* ibuf safes the remaining interleaved data of the body */

    if (hlen == -1) {
      /* FIXME: write an output frame signaling error... in case anyone does
         not check the return value, you know... */
      return -1;
    }
    
    /* the escaped values are in epConfig, copy them to frame_attrib. create
       the correct number of instances of the attributes fitting to
       concatenation. also store all the reordering information in class_index,
       which will be needed later to map epFrame-classes to codec-classes */
    createDecoderFrameAttrib(&hEpConfig->epConfig, pred, &frame_attrib, class_index);
    nclass = frame_attrib.ClassCount;

    if(NULL == (tmp1 = (int *)calloc(sizeof(int), translen))){
      perror("EPTool: EPTool_DecodeFrame(): tmp1"); exit(1);
    }
    if(NULL == (tmp2 = (int *)calloc(sizeof(int), translen))){
      perror("EPTool: EPTool_DecodeFrame(): tmp2"); exit(1);
    }

    clen = nclen = sptr = 0;
    /* Length other than last class */
    sptr = 0;
    for(i=0; i< CLASSMAX; i++) codedlen[i] = 0;  /* Initialization */
    bound_punc[0] = 0;
  
    for(i=0; i<nclass-1; i++){
      int srclen, rate;

      srclen = frame_attrib.cattr[i].ClassBitCount + (frame_attrib.cattr[i].ClassBitCount?frame_attrib.cattr[i].ClassCRCCount:0);

      if(frame_attrib.cattr[i].FECType == 0){
        bound_punc[i+1] = srclen + bound_punc[i];
        if(frame_attrib.cattr[i].TermSW){
          if(bound_punc[i+1] - bound_punc[SRCPCStart]){
            bound_punc[i+1] += 4;                       /* add tailbits */
          }
          SRCPCStart = i+1;
        }
      }else{
        bound_punc[i+1] = bound_punc[i];
      }
      rate = frame_attrib.cattr[i].ClassCodeRate;

      if (frame_attrib.cattr[i].FECType == 0){ /* SRCPC */
          int tmp, m, tmpClassRate;
        
          tmp = 8 - bound_punc[i]%8;
        if (tmp == 8) tmp = 0;
          
          if(tmp){
            if(tmp > bound_punc[i+1]-bound_punc[i]){
              tmp = bound_punc[i+1]-bound_punc[i];
            }
            
            tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
            for(m=i-1; m>=0; m--){
            if (frame_attrib.cattr[m].FECType == 0){
              tmpClassRate = frame_attrib.cattr[m].ClassCodeRate;
                if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
                  break;
                }
              }
            }
            codedlen[i] = SRCPCDepuncCount(bound_punc[i], bound_punc[i]+tmp, 0, tmpClassRate);
          }
        
        codedlen[i] += SRCPCDepuncCount(bound_punc[i]+tmp, bound_punc[i+1], 0, frame_attrib.cattr[i].ClassCodeRate); /* Calculate length of SRCPC encode class */
        /*  codedlen[i] = SRCPCDepuncCount(sptr, sptr+srclen, 0, rate); */
      } else { /* RS */
        /* FEC sequence */
        /* ... 2 2 ... */
        if ( frame_attrib.cattr[i].FECType == 2 ) {
          rsconcat++;
          codedlen[i]   = 0;
          rsconcatlen += srclen; 
        }
        if ( frame_attrib.cattr[i].FECType == 1) {
          /* ... 2 1 ... */
          if ( rsconcat ) {
            rsconcat=0;
            rsconcatlen += srclen;
            codedlen[i] = RSEncodeCount(0, rsconcatlen, rate);
          }
          else 
            /*  ... 1 1 ... */
            codedlen[i] = RSEncodeCount(0, srclen, rate);
        }
      }
      if(frame_attrib.cattr[i].ClassInterleave) {
        clen += codedlen[i];
      } else {
        nclen += codedlen[i];
      }
    }

    if(translen < clen + hlen){ /* Check the length other than last */
      /* FIXME: write an output frame signaling error... in case anyone does
         not check the return value, you know... */
      return -1;
    }

    if (translen < hlen+clen+nclen+nstuff) { /* check all length */
      /* these double checks are really necessary, you know? */
      /* FIXME: write an output frame signaling error... in case anyone does
         not check the return value, you know... */
      return -1;
    }

    /* Last class length */
    codedlen[nclass-1] = translen-hlen-clen-nclen-nstuff;

    if(frame_attrib.cattr[nclass-1].ClassInterleave)
      clen += codedlen[nclass-1];
    else
      nclen += codedlen[nclass-1];
    
    /* The number of Stuffing Bits */
    nclen += nstuff;
    
    /* De-interleaving !!*/
    sptr = 0; nptr = 0;
    for(i=0; i<nclen; i++) ncbuf[i] = ibuf[i+clen];
    for(i=0; i<nclass; i++){
      int Interleave;
      Interleave = frame_attrib.cattr[i].ClassInterleave;
      if(Interleave == 0){ /* No interleave */
        for(j=0; j<codedlen[i]; j++)
          coded[sptr++] = ncbuf[nptr++];
      }else if(Interleave == 1 && frame_attrib.cattr[i].FECType == 0){ /*Intra w/o Inter */
        recdeinter(&coded[sptr], codedlen[i], codedlen[i], ibuf, clen-codedlen[i], ibuf);
        sptr += codedlen[i];
        clen -= codedlen[i];
      }else if(Interleave == 2 && frame_attrib.cattr[i].FECType == 0){ /*Intra w/o Inter */
        recdeinter(&coded[sptr], codedlen[i], PINTDEPTH, ibuf, clen-codedlen[i], ibuf);
        sptr += codedlen[i];
        clen -= codedlen[i];
        
      } else if((Interleave == 1 || Interleave == 2) && (frame_attrib.cattr[i].FECType == 1 || frame_attrib.cattr[i].FECType == 2)){
        recdeinterBytewise(&coded[sptr], codedlen[i], frame_attrib.cattr[i].ClassCodeRate, ibuf, clen-codedlen[i], ibuf);
        sptr += codedlen[i];
        clen -= codedlen[i];
      }else if(Interleave == 3){ 
        sptr += codedlen[i];
      }  
    }
    
    /* insert data for ClassInterleave == 3 */
    sptr=0;
    for(i=jcnt=0;i<nclass;i++){
      if(frame_attrib.cattr[i].ClassInterleave == 3){
        for(j=0;j<codedlen[i];j++){
          coded[sptr++]=ibuf[j+jcnt];
        }
        jcnt+= codedlen[i];
      }else{
        sptr+=codedlen[i];
      }
    }
    
    clen = sptr;
    free(tmp1); free(tmp2);
   
  } else { /* Not Interleaved */
    hlen = RetrieveHeader(&hEpConfig->epConfig, &pred, trans, &nstuff);
    translen -= nstuff;
    
    /* the escaped values are in epConfig, copy them to frame_attrib. create
       the correct number of instances of the attributes fitting to
       concatenation. also store all the reordering information in class_index,
       which will be needed later to map epFrame-classes to codec-classes */
    createDecoderFrameAttrib(&hEpConfig->epConfig, pred, &frame_attrib, class_index);
    nclass = frame_attrib.ClassCount;

    {
      int i, from, to, rate;
      int SRCPCStart = 0;
      clen = from = 0;
      bound_punc[0] = 0;
      for(i=0; i<nclass-1; i++){

        to = from + frame_attrib.cattr[i].ClassBitCount + (frame_attrib.cattr[i].ClassBitCount?frame_attrib.cattr[i].ClassCRCCount:0);

        if(frame_attrib.cattr[i].FECType == 0 && frame_attrib.cattr[i].TermSW){
          bound_punc[i+1] = bound_punc[i] + to - from;
          if((bound_punc[i+1] - bound_punc[SRCPCStart]) && frame_attrib.cattr[i].ClassCodeRate){
            bound_punc[i+1] += 4;
          }
          SRCPCStart = i+1;
        }else{
          bound_punc[i+1] = bound_punc[i];
        }

        rate = frame_attrib.cattr[i].ClassCodeRate;
        if(frame_attrib.cattr[i].FECType == 0){ /*SRCPC*/
          {
            int tmp, m, tmpClassRate;
            
            tmp = 8 - bound_punc[i]%8;
            if(tmp == 8){
              tmp = 0;
            }
            
            if(tmp){
              if(tmp > bound_punc[i+1]-bound_punc[i]){
                tmp = bound_punc[i+1]-bound_punc[i];
              }
              
              tmpClassRate = 0;                   /* to prevent reading of not initialized mem */
              for(m=i-1; m>=0; m--){
                if(frame_attrib.cattr[m].FECType == 0){
                  tmpClassRate = frame_attrib.cattr[m].ClassCodeRate;
                  if(bound_punc[i]-bound_punc[m] >= bound_punc[i]%8){
                    break;
                  }
                }
              }
              clen += SRCPCDepuncCount(from, from+tmp, 0, tmpClassRate);
            }
            
            clen -= SRCPCDepuncCount(from + tmp, to, 0, frame_attrib.cattr[i].ClassCodeRate); /* Calculate length of SRCPC encode class */
          }
          
        /*  clen += SRCPCDepuncCount(from, to, 0, rate); */
        } else{                  /* RS */
          int rsconcat;
          rsconcat = 0;
          for(j=i; frame_attrib.cattr[j].FECType == 2; j++, rsconcat ++) ;
          for(j=0; j<rsconcat; j++){
            to += frame_attrib.cattr[i+j+1].ClassBitCount + (frame_attrib.cattr[i+j+1].ClassBitCount?frame_attrib.cattr[i+j+1].ClassCRCCount:0);
          }
          clen += RSEncodeCount(from, to, rate);
          i += rsconcat;
        }
        from = to;
      }
    }
    
    if ( hlen == -1) { /*check header-length*/
      /* FIXME: write an output frame signaling error... in case anyone does
         not check the return value, you know... */
      return -1;
    }

     if ( translen < clen + hlen ) {
      /* FIXME: write an output frame signaling error... in case anyone does
         not check the return value, you know... */
      return -1;
    }

    clen =translen - hlen;
    for(i=0; i<clen; i++)
      coded[i] = (int)(unsigned)trans[i+hlen];
  }

  payloadlen = PayloadDecode(data, coded, clen, &frame_attrib, check);

  {
    unsigned char *class_data[CLASSMAX];
    long class_length[CLASSMAX];
    int class_crc_error[CLASSMAX];

    /* reorder the classes and convert from the excplicit one-bit-per-int
       representation to unsigned char array (the only true thing! :) ) */
    DecodeReordering(frame_attrib.ClassCount, data, check, class_index,
            &frame_attrib, class_data, class_length, class_crc_error);

    /* demultiplex the frame, i.e. undo the concatenation */
    concatenation_demux(hEpConfig->concat_buffer, pred, (const unsigned char**)class_data, class_length);

    /* okay, we have really succeded in:
       - finding the pred set and optional classes
       - decoding all ep-stuff
       - undo the reordering
       - moving the decoded classes into the concatenation buffer
       now we can get them off there, frame by frame. nice! :)
    */

    /* store crc errors and pred set to be able to build nice output frames */
    j=0;
    for (i=0; i<hEpConfig->epConfig.fattr[pred].ClassCount; i++) {
      int repeat = hEpConfig->epConfig.NConcat;
        int k; 
      if (hEpConfig->epConfig.fattr[pred].cattr[i].Concatenate==1) repeat=1;
      for (k=0; k<repeat; k++, j++) {
        hEpConfig->crcDecodeError[k][i] = class_crc_error[j];
      }
    }
    hEpConfig->predInConcatBuffer = pred;
    
    /* be nice and clean up */
    for (i=0; i<nclass; i++) free(class_data[i]);
    free(frame_attrib.cattr);
    free(frame_attrib.cattresc);
  }

  /* get one frame from the concatenation buffer and write it to outBuffer */
  return EPTool_GetDecodedFrame(hEpConfig, outBuffer, bitFlag);
}
    
    
int EPTool_GetDecodedFrame(HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME outBuffer, int bitFlag)
{
  int ret_val;
  int frame;
  int i;
  int pred, origpred;
  EPFrameAttrib *frame_attrib;
  HANDLE_CLASS_FRAME inBuffer;

  /* nothing in concat buffer, nothing returned! */
  if ((hEpConfig==NULL)||(outBuffer==NULL)) return -1;
  pred = hEpConfig->predInConcatBuffer;
  if (pred==-1) return -1;
  origpred = hEpConfig->origPredForCAPred[pred];

  /* get one frame from buffer */
  inBuffer = ClassFrame_Create(hEpConfig, 0);
  ret_val = concatenation_retrieve(hEpConfig->concat_buffer, inBuffer);
  if (ret_val<0) return ret_val;
  if (ret_val==0) {
    hEpConfig->predInConcatBuffer = -1;
  }

  frame = hEpConfig->epConfig.NConcat - ret_val - 1;
  frame_attrib = &(hEpConfig->epConfig.fattr[origpred]);

  /* fill in values for the crc decoder errors */
  for (i=0; i<frame_attrib->ClassCount; i++) {
    outBuffer->crcError[i] = hEpConfig->crcDecodeError[frame][i];
  }

  /* hmm... thinking about it... optional classes might get mixed up a little.
     they are at this point just completely left out of the frame, the classes
     following optional classes are moved forward accordingly. this is no
     good... to fix this, we insert new empty classes at some randomly picked
     places... :-)
  */
  {
    int in_idx=0, out_pos;
    int opt_class_num = 0;
    int *classOptional = hEpConfig->classAvailableTable[origpred];
    for (out_pos=0; out_pos<outBuffer->nrValidBuffer; out_pos++) {
      if ((classOptional[out_pos])&&(pred & (1<<opt_class_num))) {
        /* put an empty class */
        outBuffer->classCodeRate[out_pos] = 0;
        outBuffer->classCRCLength[out_pos] = 0;
        outBuffer->crcError[out_pos] = 0;
        outBuffer->classLength[out_pos] = 0;
      } else {
        /* convert data[in_idx] -> data_out[out_pos] */
        outBuffer->classCodeRate[out_pos] = inBuffer->classCodeRate[in_idx];
        outBuffer->classCRCLength[out_pos] = inBuffer->classCRCLength[in_idx];
        outBuffer->crcError[out_pos] = inBuffer->crcError[in_idx];
        outBuffer->classLength[out_pos] = inBuffer->classLength[in_idx];
      if (bitFlag){
          EPTool_BytesToBits_CharToChar(inBuffer->classBuffer[in_idx], outBuffer->classBuffer[out_pos], outBuffer->classLength[out_pos]);
      } else {
          memcpy(outBuffer->classBuffer[out_pos], inBuffer->classBuffer[in_idx], (outBuffer->classLength[out_pos]+7)/8);
        }
        in_idx++;
      }
      if (classOptional[out_pos]) opt_class_num++;
    }
  }

  ClassFrame_Destroy(inBuffer);

  return ret_val;
}


HANDLE_CLASS_FRAME EPTool_GetClassFrame ( HANDLE_EP_CONFIG hEpConfig, int bitFlag)
{
  HANDLE_CLASS_FRAME tmpHandle;
  int scaling = 1, i;
  
  if(bitFlag)
    scaling = 8;

  tmpHandle = (HANDLE_CLASS_FRAME) calloc (1, sizeof ( T_CLASS_FRAME));
  
  tmpHandle->nrValidBuffer  = EPTool_GetMaximumNumberOfClasses(hEpConfig);
  tmpHandle->maxClasses     = CLASSMAX;
  tmpHandle->maxBufferSize  = (MAXBITS * scaling / 8);

  tmpHandle->classLength    = hEpConfig->classLength;
  tmpHandle->classCRCLength = hEpConfig->classCRCLength;
  tmpHandle->classCodeRate  = hEpConfig->classCodeRate;

  for(i=0; i<tmpHandle->nrValidBuffer; i++) {
    tmpHandle->classLength[i] = 0;
  }
  tmpHandle->crcError       = hEpConfig->crcError;

  tmpHandle->classBuffer    = hEpConfig->classBuffer;

  return tmpHandle;
}

int EPTool_Destroy( HANDLE_EP_CONFIG hEpConfig )
{
  int pred;

  FrameHandle_Destroy( hEpConfig );
  
  for (pred = 0; pred < hEpConfig->epConfig.NPred; pred++) {
    if (hEpConfig->epConfig.fattr[pred].cattr)
      free(hEpConfig->epConfig.fattr[pred].cattr);
    
    if (hEpConfig->epConfig.fattr[pred].cattresc)
      free(hEpConfig->epConfig.fattr[pred].cattresc);

    if (hEpConfig->epConfig.fattr[pred].lenbit)
      free(hEpConfig->epConfig.fattr[pred].lenbit);

  }

  for (pred = 0; pred < hEpConfig->origEpConfig.NPred; pred++) {

    if (hEpConfig->origEpConfig.fattr[pred].cattr) 
      free(hEpConfig->origEpConfig.fattr[pred].cattr); 
    
    if (hEpConfig->origEpConfig.fattr[pred].cattresc) 
      free(hEpConfig->origEpConfig.fattr[pred].cattresc);

    if (hEpConfig->origEpConfig.fattr[pred].lenbit) 
      free(hEpConfig->origEpConfig.fattr[pred].lenbit); 

  }

  if (hEpConfig->epConfig.fattr)
    free(hEpConfig->epConfig.fattr);

  if (hEpConfig->origEpConfig.fattr)
    free(hEpConfig->origEpConfig.fattr);

  if (hEpConfig)
    free(hEpConfig);

  return 0;
}

static void EPTool_BytesToBits(const unsigned char *inbuf, int *trans, int translen)
{
  int i, bitrem = 0;
  unsigned char tmp = 0;

  for(i=0; i< translen; i++){
    if(bitrem == 0){
      tmp = inbuf[i/8] ;
      bitrem = 8;
    }
    trans[i] = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
}

static int EPTool_BitsToBytes(unsigned char *outbuf, const int *trans, int translen )
{
  int bitrem = 0;
  unsigned char tmp = 0;
  int i;

  if(translen == -1){		/* Flash */
    if(bitrem == 0) return bitrem;
    tmp <<= (8 - bitrem);
    outbuf[0] = tmp; /* ?????? dh */ 
    /* fwrite(&tmp, sizeof(char), 1, fpout); */
    return bitrem;
  }
  
  for(i=0; i<translen; i++){
    tmp <<= 1;
    tmp |= *(trans++);
    bitrem ++;
    if(bitrem == 8){
      outbuf[i/8] = tmp;
      /* fwrite(&tmp, sizeof(char), 1, fpout); */
      bitrem = 0;
    }
  }

  return bitrem;
}

/* the same as before... yippee! */
static int EPTool_BitsToBytes_CharToChar(unsigned char *outbuf, const unsigned char *trans, int translen )
{
  int bitrem = 0;
  unsigned char tmp = 0;
  int i;
  
  if(translen == -1){		/* Flash */
    if(bitrem == 0) return bitrem;
    tmp <<= (8 - bitrem);
    outbuf[0] = tmp; /* ?????? dh */ 
    /* fwrite(&tmp, sizeof(char), 1, fpout); */
    return bitrem;
  }

  for(i=0; i<translen; i++){
    tmp <<= 1;
    tmp |= *(trans++);
    bitrem ++;
    if(bitrem == 8){
      outbuf[i/8] = tmp;
      /* fwrite(&tmp, sizeof(char), 1, fpout); */
      bitrem = 0;
}
  }

  return bitrem;
}

static HANDLE_CLASS_FRAME ClassFrame_Create (const HANDLE_EP_CONFIG inFrame, int bitFlag )
{
  int i, classmax;
  int scaling = 1;
  HANDLE_CLASS_FRAME ret = malloc(sizeof(struct tagClassFrame));

  classmax = EPTool_GetMaximumNumberOfClassesInternal(inFrame);
  ret->nrValidBuffer = classmax;
  ret->maxClasses = classmax;
      
  if (bitFlag) scaling = 8;
        
  ret->classLength    = (long*)calloc (classmax, sizeof(long));
  ret->classCRCLength = (long*)calloc (classmax, sizeof(long));
  ret->classCodeRate  = (long*)calloc (classmax, sizeof(long));
    
  ret->classBuffer    = (unsigned char**)calloc (classmax * (MAXBITS*scaling/8), sizeof(char));
    
  for  (i = 0; i < ret->maxClasses; i++) {
    ret->classBuffer[i] = (unsigned char*)calloc (MAXBITS*scaling/8, sizeof(char));
    }
    
  ret->crcError       = (short*)calloc (classmax, sizeof(short));
      
  return ret;
  }
static void ClassFrame_Destroy(HANDLE_CLASS_FRAME classFrame)
{
  int i;
  
  if (classFrame->classLength) free(classFrame->classLength);
  if (classFrame->classCRCLength) free(classFrame->classCRCLength);
  if (classFrame->classCodeRate) free(classFrame->classCodeRate);
  
  for (i=0; i < classFrame->maxClasses; i++) {
    if (classFrame->classBuffer[i])
      free(classFrame->classBuffer[i]);
  }
    
  if (classFrame->classBuffer) free (classFrame->classBuffer); 
  if (classFrame->crcError) free(classFrame->crcError); 

  free(classFrame);
  }


 

int FrameHandle_Create( HANDLE_EP_CONFIG inFrame, int bitFlag)
{
  int i, classmax;
  int scaling = 1;
        
  classmax = EPTool_GetMaximumNumberOfClassesInternal(inFrame);
    
  if (bitFlag) scaling = 8;
    
  inFrame->classLength    = (long*)calloc (classmax, sizeof(long));
  inFrame->classCRCLength = (long*)calloc (classmax, sizeof(long));
  inFrame->classCodeRate  = (long*)calloc (classmax, sizeof(long));
      
  inFrame->classBuffer    = (unsigned char**)calloc (classmax * (MAXBITS*scaling/8), sizeof(char));
      
  for  (i = 0; i < classmax; i++) {
    inFrame->classBuffer[i] = (unsigned char*)calloc (MAXBITS*scaling/8, sizeof(char));
    }
  
  inFrame->crcError       = (short*)calloc (classmax, sizeof(short));

  return 0;
}

int FrameHandle_Destroy( HANDLE_EP_CONFIG inFrame )
{
  int i, classmax;
  
  classmax = EPTool_GetMaximumNumberOfClassesInternal(inFrame);
  
  if (inFrame->classLength )
    free (inFrame->classLength );
  
  if (inFrame->classCRCLength)
    free(inFrame->classCRCLength);
    
  if (inFrame->classCodeRate)
    free(inFrame->classCodeRate);
      
  for (i= 0; i < classmax; i++) {
    if (inFrame->classBuffer[i])
      free(inFrame->classBuffer[i]);
  }
  
  if (inFrame->classBuffer)
    free (inFrame->classBuffer); 
    
  if (inFrame->crcError)
    free (inFrame->crcError); 
    
  return 0;
      }
      


int EPTool_CreateBufferEPSpecificConfig(unsigned char **outBuffer, const char *predFileName){

  int npred;
  char buffer[1024];
  FILE *fp;

  if (NULL == (fp = fopen(predFileName, "r"))) {
    fprintf(stderr,"EPTool: EPTool_CreateExpl(): Error: predefinition file not found\n");
    exit(-1);
  }

  if(NULL == fgets(buffer, sizeof(buffer), fp)) {
    fprintf(stderr, "EP_Tool: EPTool_CreateBufferEPSpecificConfig(): libeptool.c: #pred NULL");
  }

  if(1 != sscanf(buffer, "%d", &npred)) {
    fprintf(stderr, "EP_Tool: EPTool_CreateBufferEPSpecificConfig(): libeptool.c: Error: Please check PredefinitionFile!");
  }

  if (!npred){
    fprintf (stderr, "EP_Tool: EPTool_CreateBufferEPSpecificConfig(): libeptool.c: Error: Please set number_of_predefined_set unequal 0!");
    exit(-1);
  }

  if (NULL == ((*outBuffer) = (unsigned char *)malloc((32+npred*429)*sizeof(char)))){
    fprintf(stderr, "EPTool: EPTool_CreateBufferEPSpecificConfig(): libeptool.c: Failed malloc for outBuffer");
    exit(-1);
  }

  fclose(fp);
  return 0;

}


int EPTool_DestroyBufferEPSpecificConfig(unsigned char **outBuffer){
  
  free(*outBuffer);
  return 0;

}


static long int EPTool_WriteEPSpecificConfig(unsigned char *outBuffer, HANDLE_EP_CONFIG hEpConfig){

  int i, j, k, offset;
  unsigned char tmp[16];
  int *tmpBuffer;
  unsigned int aux;

  offset = 0;


  if (NULL == (tmpBuffer = (int *)calloc(1, (32+hEpConfig->origEpConfig.NPred*429)*sizeof(int)))){
    fprintf(stderr, "EPTool: EPTool_WriteEPSpecificConfig(): libeptool.c: Failed malloc for tmpBuffer");
    exit(-1);
  }

  EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.NPred, tmp, 8);
  for (k=0; k<8; k++){
    tmpBuffer[k] = tmp[k];
  }
  offset += k;

  EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.Interleave, tmp, 2);
  for (k=0; k<2; k++){
    tmpBuffer[offset+k] = tmp[k];
  }
  offset += k;

  EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.BitStuffing, tmp, 3);
  for (k=0; k<3; k++){
    tmpBuffer[offset+k] = tmp[k];
  }
  offset += k;
  
  EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.NConcat, tmp, 3);
  for (k=0; k<3; k++){
    tmpBuffer[offset+k] = tmp[k];
  }
  offset += k;


  for(i=0; i<hEpConfig->origEpConfig.NPred; i++){
    
    EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].ClassCount, tmp, 6);
    for (k=0; k<6; k++){
      tmpBuffer[offset+k] = tmp[k];
    }
    offset += k;

    for(j=0; j<hEpConfig->origEpConfig.fattr[i].ClassCount; j++){
      
      switch(hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassBitCount){
      case 0:
        tmpBuffer[offset++] = 0;
        break;
      case 1:
        tmpBuffer[offset++] = 1;
          break;
      }

      switch(hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCodeRate){
      case 0:
        tmpBuffer[offset++] = 0;
        break;
      case 1:
        tmpBuffer[offset++] = 1;
        break;
      }

      switch(hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCRCCount){
      case 0:
        tmpBuffer[offset++] = 0;
        break;
      case 1:
        tmpBuffer[offset++] = 1;
        break;
      }

      if(hEpConfig->origEpConfig.NConcat != 1){
        switch(hEpConfig->origEpConfig.fattr[i].cattr[j].Concatenate){
        case 0:
          tmpBuffer[offset++] = 0;
          break;
        case 1:
          tmpBuffer[offset++] = 1;
          break;
        }
      }

      EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].cattr[j].FECType, tmp, 2);
      for(k=0; k<2; k++){
        tmpBuffer[offset+k] = tmp[k];
      }
      offset += k;
   
      if(!hEpConfig->origEpConfig.fattr[i].cattr[j].FECType){
        switch(hEpConfig->origEpConfig.fattr[i].cattr[j].TermSW){
        case 0:
          tmpBuffer[offset++] = 0;
          break;
        case 1:
          tmpBuffer[offset++] = 1;
          break;
        }
      }

      if(hEpConfig->origEpConfig.Interleave == 2){
        EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].cattr[j].ClassInterleave, tmp, 2);
        for(k=0; k<2; k++){
          tmpBuffer[offset+k] = tmp[k];
        }
        offset += k;
      }

      switch(hEpConfig->classAvailableTable[i][j]){
      case 0:
        tmpBuffer[offset++] = 0;
        break;
      case 1:
        tmpBuffer[offset++] = 1;
        break;
      }

      if(hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassBitCount){
        EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].lenbit[j], tmp, 4);
        for(k=0; k<4; k++){
          tmpBuffer[offset+k] = tmp[k];
        }
        offset += k;
      } else {
        EPTool_BytesToBits_IntToChar(&hEpConfig->origEpConfig.fattr[i].cattr[j].ClassBitCount, tmp, 16);
        for(k=0; k<16; k++){
          tmpBuffer[offset+k] = tmp[k];
        }
        offset += k;
      }
      
      if(!hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCodeRate){
        if(hEpConfig->origEpConfig.fattr[i].cattr[j].FECType){
          EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCodeRate, tmp, 7);
          for(k=0; k<7; k++){
            tmpBuffer[offset+k] = tmp[k];
          }
        } else {
          EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCodeRate, tmp, 5);
          for(k=0; k<5; k++){
            tmpBuffer[offset+k] = tmp[k];
          }
        }
        offset += k;
      }

      if(!hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCRCCount){
        switch(hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCRCCount){
        case 24:
          aux = 17;
          break;
        case 32:
          aux = 18;
          break;
        default: 
          aux = hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCRCCount;
          break;
        }
        EPTool_BytesToBits_IntToChar(&aux, tmp, 5);
        for(k=0; k<5; k++){
          tmpBuffer[offset+k] = tmp[k];
        }
        offset += k;
      }
    }

    switch(hEpConfig->origEpConfig.fattr[i].ClassReorderedOutput){
    case 0:
      tmpBuffer[offset++] = 0;
      break;
    case 1:
      tmpBuffer[offset++] = 1;
    }
    
    if(hEpConfig->origEpConfig.fattr->ClassReorderedOutput){
      for(j=0; j<hEpConfig->origEpConfig.fattr[i].ClassCount; j++){
        EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.fattr[i].ClassOutputOrder[j], tmp, 6);
        for(k=0; k<6; k++){
          tmpBuffer[offset+k] = tmp[k];
        }
        offset += k;
      }
    }
  }

  switch(hEpConfig->origEpConfig.HeaderProtectExtend){
  case 0:
    tmpBuffer[offset++] = 0;
    break;
  case 1:
    tmpBuffer[offset++] = 1;
    break;
  }

  if(hEpConfig->origEpConfig.HeaderProtectExtend){
    EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.HeaderRate, tmp, 5);
    for(k=0; k<5; k++){
      tmpBuffer[offset+k] = tmp[k];
    }
    offset += k;

    switch(hEpConfig->origEpConfig.HeaderCRC){
    case 24:
      aux = 17;
      break;
    case 32:
      aux = 18;
      break;
    default:
      aux = hEpConfig->origEpConfig.HeaderCRC;
      break;
    }
    EPTool_BytesToBits_IntToChar(&aux, tmp, 5);
    for(k=0; k<5; k++){
      tmpBuffer[offset+k] = tmp[k];
    }
    offset += k;
  }

#if RS_FEC_CAPABILITY
  EPTool_BytesToBits_IntToChar((unsigned int *)&hEpConfig->origEpConfig.RSFecCapability, tmp, 7);
  for(k=0; k<7; k++){
    tmpBuffer[offset+k] = tmp[k];
  }
  offset += k;
#endif

  EPTool_BitsToBytes(outBuffer, tmpBuffer, ((offset+7)/8)*8);
 
  free(tmpBuffer);

  return offset;
}


static int EPTool_ParseEPSpecificConfig(unsigned char *inBuffer, long int configLength, HANDLE_EP_CONFIG hEpConfig){

  int i, j;
  int offset = 0;
  int npred = 0;
  int nclass = 0;
  int tmp[1];
  unsigned char *tmpBuffer = NULL;

  hEpConfig->origEpConfig.NPred =  npred = inBuffer[0];
  offset += 8;

  if(NULL == (hEpConfig->origEpConfig.fattr = (EPFrameAttrib *)calloc(npred, sizeof(EPFrameAttrib)))){
    fprintf(stderr, "EP_Tool: EPTool_ParseEPSpecificConfig(): libeptool.c: Unable to calloc for EPFrameAttrib\n");
    exit(1);
  }

  if (NULL == (tmpBuffer = (unsigned char *)malloc(configLength*sizeof(char)))){
    fprintf(stderr, "EPTool: EPTool_ParseEPSpecificConfig: libeptool.c: Failed malloc tmpBuffer");
    exit(-1);
  }

  EPTool_BytesToBits_CharToChar(inBuffer, tmpBuffer, configLength);
      
  EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 2);
  hEpConfig->origEpConfig.Interleave = *tmp;
  offset += 2;

  EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 3);
  hEpConfig->origEpConfig.BitStuffing = *tmp;
  offset += 3;

  EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 3);
  hEpConfig->origEpConfig.NConcat = *tmp;
  offset += 3;

  for (i=0; i<hEpConfig->origEpConfig.NPred; i++){
    
    EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 6);
    hEpConfig->origEpConfig.fattr[i].ClassCount = nclass = *tmp;
    offset += 6;

    if(NULL == (hEpConfig->origEpConfig.fattr[i].cattr = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent)))){
      fprintf(stderr, "EP_Tool: EPTool_ParseEPSpecificConfig(): libeptool.c: Unable to calloc for Cattr\n");
      exit(1);
    }
    if(NULL == (hEpConfig->origEpConfig.fattr[i].cattresc = (ClassAttribContent *)calloc(nclass, sizeof(ClassAttribContent)))){
      fprintf(stderr, "EP_Tool: EPTool_ParseEPSpecificConfig(): libeptool.c: Unable to calloc for CattrESC\n");
      exit(1);
    }
    if(NULL == (hEpConfig->origEpConfig.fattr[i].lenbit = (int *)calloc(nclass, sizeof(int)))){
      fprintf(stderr, "EP_Tool: EPTool_ParseEPSpecificConfig(): libeptool.c: Unable to calloc for CattrESC\n");
      exit(1);
    }
    

    for (j=0; j<hEpConfig->origEpConfig.fattr[i].ClassCount; j++){

      EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
      hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassBitCount = *tmp;
      offset += 1;

      EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
      hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCodeRate = *tmp;
      offset += 1;

      EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
      hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCRCCount = *tmp;
      offset += 1;

      if (hEpConfig->origEpConfig.NConcat !=1){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
        hEpConfig->origEpConfig.fattr[i].cattr[j].Concatenate = *tmp;
        offset += 1;
      }

      EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 2);
      hEpConfig->origEpConfig.fattr[i].cattr[j].FECType = *tmp;
      offset += 2;

      if (!hEpConfig->origEpConfig.fattr[i].cattr[j].FECType){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
        hEpConfig->origEpConfig.fattr[i].cattr[j].TermSW = *tmp;
        offset += 1;
      }

      if (hEpConfig->origEpConfig.Interleave == 2){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 2);
        hEpConfig->origEpConfig.fattr[i].cattr[j].ClassInterleave = *tmp;
        offset += 2;
      }

      EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
      hEpConfig->classAvailableTable[i][j] = *tmp;
      hEpConfig->classAvailableBits[i] += *tmp;
      offset += 1;
      

      if (hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassBitCount){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 4);
        hEpConfig->origEpConfig.fattr[i].lenbit[j] = *tmp;
        offset += 4; 
      } else {
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 16);
        hEpConfig->origEpConfig.fattr[i].cattr[j].ClassBitCount = *tmp;
        offset += 16;
      }

      if (!hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCodeRate){
        if(hEpConfig->origEpConfig.fattr[i].cattr[j].FECType){
          EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 7);
          hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCodeRate = *tmp;
          offset += 7;
        } else {
          EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 5);
          hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCodeRate = *tmp;
          offset += 5;
        }
      }
        
       
      if (!hEpConfig->origEpConfig.fattr[i].cattresc[j].ClassCRCCount){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 5);
        switch(*tmp){ /* handle right crc-len */
        case 17:
          hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCRCCount = 24;
          break;
        case 18:
          hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCRCCount = 32;
          break;
        default: 
          hEpConfig->origEpConfig.fattr[i].cattr[j].ClassCRCCount = *tmp;
          break;
        }
        offset += 5;
      }
    }
      
    EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
    hEpConfig->origEpConfig.fattr[i].ClassReorderedOutput = *tmp;
    offset += 1;

    if (hEpConfig->origEpConfig.fattr[i].ClassReorderedOutput){
      for (j=0; j<hEpConfig->origEpConfig.fattr[i].ClassCount; j++){
        EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 6);
        hEpConfig->origEpConfig.fattr[i].ClassOutputOrder[j] = *tmp;
        offset += 6;
      }
    }
  }

  EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 1);
  hEpConfig->origEpConfig.HeaderProtectExtend = *tmp;
  offset += 1;

  if (hEpConfig->origEpConfig.HeaderProtectExtend){
    EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 5);
    hEpConfig->origEpConfig.HeaderRate = *tmp;
    offset += 5;

    EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 5);
    switch(*tmp){
    case 17:
      hEpConfig->origEpConfig.HeaderCRC = 24;
      break;
    case 18:
      hEpConfig->origEpConfig.HeaderCRC = 32;
      break;
    default:
      hEpConfig->origEpConfig.HeaderCRC = *tmp;
      break;
    }
    offset += 5;
  }

#if RS_FEC_CAPABILITY
  EPTool_BitsToBytes_CharToInt(&tmpBuffer[offset], tmp, 7);
  hEpConfig->origEpConfig.RSFecCapability = *tmp;
  offset += 7;
#endif  

  if(offset != configLength){
    fprintf(stderr, "EPTool_ParseEPSpecificConfig(): libeptool.c: Error Parsing configBuffer");
    free(tmpBuffer);
    return -1;
  }
      
  free(tmpBuffer);

  return 0;
  
}


static int EPTool_BytesToBits_IntToChar(const unsigned int *inbuf, unsigned char *outbuf, int bitlen)
{
  int i, j, k;
  unsigned char tmpBuffer[16]={0};
  int tmp;

  k = bitlen;

  for(i=0; i<(bitlen+15)/16; i++){
    tmp = inbuf[i];
    for (j=16; j>0; ){
      tmpBuffer[--j] = tmp%2;
      tmp >>= 1;
    }
    
    if (k<16){
      for (j=0; j<k; j++){
        outbuf[j] = tmpBuffer[16-k+j];
      }
      return 0;
    } else {
      for (j=0; j<16; j++){
        outbuf[j] = tmpBuffer[j];
      }
      k -= 16;
    }
  }
  return 0;
}


static int EPTool_BitsToBytes_CharToInt(const unsigned char *inbuf, int *outbuf, int bitlen)
{
  int i, j, k;
  int factor;
  int tmp = 0;

  k = bitlen;

  for (i=0; i<(bitlen+15)/16; i++){
    if (k > 16){
      for (j=0, factor = 32768; j< 16; i++){
        tmp += inbuf[j]*factor;
        factor >>= 1;
      }
      outbuf[i] = tmp;
      k -= 16;
    } else {
      for (j=0, factor = (32768 >> (16-k)); j<k; j++){
        tmp += inbuf[j]*factor;
        factor >>= 1;
      }
      outbuf[i] = tmp;
      return 0;
    }
  }
  return 0;
}


static void EPTool_BytesToBits_CharToChar(const unsigned char *inbuf, unsigned char *outbuf, int bitlen)
{
  int i, bitrem = 0;
  unsigned char tmp = 0;

  for(i=0; i< bitlen; i++){
    if(bitrem == 0){
      tmp = inbuf[i/8] ;
      bitrem = 8;
    }
    outbuf[i] = ((tmp & 0x80) == 0x80);
    tmp <<= 1;
    bitrem --;
  }
}

void EPTool_GetClassLength(HANDLE_EP_CONFIG self, int choiceOfPred, unsigned int* class_length)
{
  int i;

  for(i=0; i<(int)self->epConfig.fattr[choiceOfPred].ClassCount; i++) {
    class_length[i] = self->epConfig.fattr[choiceOfPred].cattr[i].ClassBitCount;
  }
}






/* ------------------------------------------ */
/* --- routines to select a good pred set --- */
/* --- added by kaehleof/Fraunhofer IIS   --- */
/* ------------------------------------------ */

/* we have an array of ep1 classes and check whether
   the pred set works with that

   lengths_num and lengths[] define the frame to be checked
   if null_padded_lengths, then optional classes appear as
     classes with length 0
   classes_done define the recursion step
*/
static int predSetValidForClasses(const EPFrameAttrib *pred_cfg,
                                  const int classOptional[],
                                  int lengths_num,
                                  const int lengths[],
                                  int null_padded_lengths,
                                  int classes_done_pred,
                                  int classes_done_length,
                                  unsigned long *classes_taken)
{
  int match = 1;
  int class_index;
  int class_can_be_skipped = -1;

  /*fprintf(stderr,"predSetValid? done classes in set %i, in values %i/%i, took %x\n",classes_done_pred, classes_done_length, lengths_num, *classes_taken);*/
  /*fprintf(stderr,"predSetValid? ...current length %i\n", lengths[bits_done]);*/

  /* --- end conditions of the recursion */
  {
    int endOfPredSet = (classes_done_pred >= pred_cfg->ClassCount);
    int endOfLengths = (classes_done_length >= lengths_num);

    if (endOfPredSet&&endOfLengths) return 1;  /* all classes match pred set */
    if (endOfLengths) match=0;   /* maybe all remaining classes are optional */
    if (endOfPredSet) {          /* more classes than defined in pred set */
      match = 0;                 /* maybe all remaining length can be skipped */
      class_can_be_skipped = ((!null_padded_lengths)||(lengths[classes_done_length]==0));
    }
  }

  class_index = classes_done_pred;
  if (class_can_be_skipped==-1) class_can_be_skipped = classOptional[class_index];

  /* --- processing step */
  
  if (match) {
    ClassAttribContent *cls_cfg = &(pred_cfg->cattr[class_index]);
    ClassAttribContent *cls_cfg_esc = &(pred_cfg->cattresc[class_index]);
    if ((cls_cfg_esc->ClassBitCount==0)&&
        (cls_cfg->ClassBitCount!=(unsigned)lengths[classes_done_length]))
      match = 0;

    class_can_be_skipped = (class_can_be_skipped &&
                            ((!null_padded_lengths)||(lengths[classes_done_length]==0)));

  }
  /*fprintf(stderr,"predSetValid? match %i,  can be skipped %i\n", match, class_can_be_skipped);*/


  /* --- recursion step */

  {
    int ret = 0;

    /* if possible, we first try to SKIP the class... */
    if (class_can_be_skipped) {
      if (classes_taken) *classes_taken &= ~(1<<class_index);
      ret = predSetValidForClasses(pred_cfg,
                                   classOptional,
                                   lengths_num, lengths, null_padded_lengths,
                                   classes_done_pred+1,
                                    classes_done_length+(null_padded_lengths?1:0),
                                   classes_taken);
    }

    /* if we have no other choice or it works anyway, we stay with that result */
    if ((!match)||(ret!=0)) return ret;

    /* ...otherwise we try again this time taking this class */
    if (classes_taken) *classes_taken |= 1<<class_index;
    return predSetValidForClasses(pred_cfg,
                                  classOptional,
                                  lengths_num, lengths, null_padded_lengths,
                                  classes_done_pred+1,
                                 classes_done_length+1,
                                  classes_taken);

  }
}

/*
  only total frame length is known, ie we are looking
  for any configuration we could split the length_bits bits of our
  ep0 frame into
  bits_done_exact means:
  - 1: we used exactly bits_done bits up to now
  - 0: we could have used arbitrarily more...
*/
static int predSetValidForLength(const EPFrameAttrib *pred_cfg,
                                 const int classOptional[],
                                 int classes_done,
                                 int length_bits_total,
                                 int length_bits_done,
                                 int bits_done_is_minimum,
                                 unsigned long *classes_taken)
{
  int bits_done_if_taken;
  int is_minimum_if_taken;
  int class_index;

  /*fprintf(stderr,"predSetValid? done classes %i, bits %i/%i(%i), took %x\n",classes_done, length_bits_done, length_bits_total, bits_done_is_minimum, *classes_taken);*/

  /* --- end conditions of the recursion */
  {
    int endOfPredSet = (classes_done >= pred_cfg->ClassCount);
    int endOfBits = (length_bits_done == length_bits_total);
    
    if (endOfPredSet&&endOfBits) return 1;  /* all classes match pred set */
    if (endOfPredSet) {
      if ((bits_done_is_minimum)&&
          (length_bits_done <= length_bits_total)) return 1; /* we can store the bits somehow */
      return 0;  /* we have more bits than we can store in this pred set */
    }
    if (length_bits_done > length_bits_total) return 0; /* we would need more bits than we have to use this pred set */
  }

  /* --- processing step */
  
  /* first find the next class index in the reordered stuff we've got... */
  class_index = classes_done;
  /*fprintf(stderr,"predSetValid? checking current length %i\n", lengths[bits_done]);*/

  {
    ClassAttribContent *cls_cfg = &(pred_cfg->cattr[class_index]);
    ClassAttribContent *cls_cfg_esc = &(pred_cfg->cattresc[class_index]);
    bits_done_if_taken = length_bits_done;
    is_minimum_if_taken = bits_done_is_minimum;
    if (cls_cfg_esc->ClassBitCount==0) bits_done_if_taken = cls_cfg->ClassBitCount;
    else is_minimum_if_taken = 1;
  }


  /* --- recursion step */

  {
    int ret = 0;

    /* we first try to take the class config... */
    if (classes_taken) *classes_taken |= 1<<class_index;
    ret = predSetValidForLength(pred_cfg,
                                classOptional,
                                classes_done+1,
                                length_bits_total,
                                bits_done_if_taken,
                                is_minimum_if_taken,
                                classes_taken);

    /* if we have no other choice or it works anyway, we stay with that result */
    if ((classOptional[class_index]==0)||(ret!=0)) return ret;

    /* ...otherwise we try again skipping this class */
    if (classes_taken) *classes_taken &= ~(1<<class_index);
    return predSetValidForLength(pred_cfg,
                                 classOptional,
                                 classes_done+1,
                                 length_bits_total,
                                 length_bits_done,
                                 bits_done_is_minimum,
                                 classes_taken);
  }
}




/* the number and lengths of the classes are known, now look for a valid pred set */
int EPTool_selectPredSet_byClasses(HANDLE_EP_CONFIG hEpConfig, int num, const unsigned int class_lengths[], int *choiceOfPred, int *choiceOfTransPred)
{
  int origpred, virtualpred;
  int set_possible = 0;
  EPConfig *epcfg;
  if (hEpConfig==NULL) return -1;

  epcfg = &(hEpConfig->origEpConfig);

  virtualpred = 0;
  for (origpred=0; origpred<epcfg->NPred; origpred++) {
    int i;
    int lengths[CLASSMAX];
    int *classOptional = hEpConfig->classAvailableTable[origpred];
    unsigned long classes_taken = 0;

    for  (i=0; i < num; i++) {
      lengths[i] = class_lengths[i];
    }

    set_possible = predSetValidForClasses(&(epcfg->fattr[origpred]),
                                          classOptional,
                                          num,
                                          lengths,
                                          1 /* null_padded_lengths */,
                                          0 /* classes_done_pred */,
                                          0 /* classes_done_length */,
                                          &classes_taken);
    if (set_possible) {
      int bit = 0;
      int index = 0;
      for (i=0; i<epcfg->fattr[origpred].ClassCount; i++) {
        if (classOptional[i]) {
          if (!((classes_taken>>i)&1)) {
            index += 1<<bit;
          }
          bit++;
        }
      }
      virtualpred += index;
      break;
    }

    virtualpred += (1<<hEpConfig->classAvailableBits[origpred]);
  }

  if (!set_possible) {
    fprintf(stderr,"EPTool_selectPredSet: no matching predefinition set found");
    return -1;
  }

  if (choiceOfPred!=NULL) *choiceOfPred = origpred;
  if (choiceOfTransPred!=NULL) *choiceOfTransPred = virtualpred;

  return 0;
}

int EPTool_selectPredSet_forClassFrame(HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME buffer, int *choiceOfPred, int *choiceOfTransPred)
{
  if (buffer==NULL) return -1;
  return EPTool_selectPredSet_byClasses(hEpConfig, buffer->nrValidBuffer, (unsigned int*)buffer->classLength, choiceOfPred, choiceOfTransPred);
}

static int EPTool_checkPredSet_forClassFrame(HANDLE_EP_CONFIG hEpConfig, HANDLE_CLASS_FRAME buffer, int *choiceOfTransPred)
{
  int origpred = -1, virtualpred = -1;
  int set_possible;
  EPConfig *epcfg;
  if ((buffer==NULL)||(hEpConfig==NULL)) return -1;
  epcfg = &(hEpConfig->origEpConfig);

  if ((buffer->choiceOfPred<0)||(buffer->choiceOfPred>=epcfg->NPred)) return -1;

  virtualpred = 0;
  for (origpred=0; origpred<buffer->choiceOfPred; origpred++) {
    virtualpred += hEpConfig->classAvailableBits[origpred];
  }

  {
    int i;
    int lengths[CLASSMAX];
    int *classOptional = hEpConfig->classAvailableTable[origpred];
    unsigned long classes_taken = 0;

    for  (i=0; i < buffer->nrValidBuffer; i++) {
      lengths[i] = buffer->classLength[i];
    }

    set_possible = predSetValidForClasses(&(epcfg->fattr[origpred]),
                                          classOptional,
                                          i,
                                          lengths,
                                          1 /* null_padded_lengths */,
                                          0 /* classes_done_pred */,
                                          0 /* classes_done_length */,
                                          &classes_taken);

    if (set_possible==0) {
      fprintf(stderr, "selected predefined set does not match class frame");
      return -1;
    }

    {
      int bit = 0;
      int index = 0;
      for (i=0; i<epcfg->fattr[origpred].ClassCount; i++) {
        if (classOptional[i]) {
          if (!((classes_taken>>i)&1)) {
            index += 1<<bit;
          }
          bit++;
        }
      }
      virtualpred += index;
    }
  }

  if (choiceOfTransPred) *choiceOfTransPred = virtualpred;

  return 0;
}


/* the number of bits in one ep0 frame is known, now look through the pred sets
   whether we can split exactly this number somehow into classes for the ep encoding */
int EPTool_selectPredSet_byLength(HANDLE_EP_CONFIG hEpConfig, int length, int *choiceOfPred, int *choiceOfTransPred)
{
  int origpred, virtualpred = 0;
  int set_possible = 0;
  EPConfig *epcfg;
  if (hEpConfig==NULL) return -1;

  epcfg = &(hEpConfig->origEpConfig);

  for (origpred=0; origpred<epcfg->NPred; origpred++) {
    int *classOptional = hEpConfig->classAvailableTable[origpred];
    unsigned long classes_taken = 0;

    set_possible = predSetValidForLength(&(epcfg->fattr[origpred]),
                                         classOptional,
                                         0 /*classes_done*/,
                                         length,
                                         0 /* length_bits_done */,
                                         0 /* bits_done_is_minimum */,
                                         &classes_taken);

    if (set_possible) {
      int i, bit = 0;
      int index = 0;
      for (i=0; i<epcfg->fattr[origpred].ClassCount; i++) {
        if (classOptional[i]) {
          if (!((classes_taken>>i)&1)) {
            index += 1<<bit;
          }
          bit++;
        }
      }
      virtualpred += index;
      break;
    }

    virtualpred += (1<<hEpConfig->classAvailableBits[origpred]);
  }

  if (!set_possible) {
    fprintf(stderr,"EPTool_selectPredSet: no matching predefinition set found");
    return -1;
  }

  if (choiceOfPred!=NULL) *choiceOfPred = origpred;
  if (choiceOfTransPred!=NULL) *choiceOfTransPred = virtualpred;

  return 0;
}

static int EPTool_GetMaximumNumberOfClassesInternal(HANDLE_EP_CONFIG hEpConfig)
{
  EPConfig *epcfg = &(hEpConfig->epConfig);
  int pred, max;

  max=0;
  for (pred=0; pred<epcfg->NPred; pred++) {
    int tmp = epcfg->fattr[pred].ClassCount;
    if (epcfg->NConcat>1) {
      int numOrig = tmp;
      tmp = 0;
      for (;numOrig>0; numOrig--) {
        if (epcfg->fattr[pred].cattr[numOrig-1].Concatenate==1) tmp++;
        else tmp+=epcfg->NConcat;
      }
    }
    if (tmp>max) max=tmp;
  }

  return max;
}

int EPTool_GetMaximumNumberOfClasses(HANDLE_EP_CONFIG hEpConfig)
{
  EPConfig *epcfg = &(hEpConfig->epConfig);
  int pred, max;

  max=0;
  for (pred=0; pred<epcfg->NPred; pred++) {
    int tmp = epcfg->fattr[pred].ClassCount;
    if (tmp>max) max=tmp;
  }

  return max;
}

