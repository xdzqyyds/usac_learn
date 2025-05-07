/***********************************************************************************
 
 This software module was originally developed by 
 
 Fraunhofer IIS
 
 in the course of development of the ISO/IEC 23003-3 for reference purposes and its 
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-3 standard. ISO/IEC gives 
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute, 
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-3 standard 
 and which satisfy any specified conformance criteria. Those intending to use this 
 software module in products are advised that its use may infringe existing patents. 
 ISO/IEC have no liability for use of this software module or modifications thereof. 
 Copyright is not released for products that do not conform to the ISO/IEC 23003-3 
 standard.
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using 
 the code for products that do not conform to MPEG-related ITU Recommendations and/or 
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works. 
 
 Copyright (c) ISO/IEC 2013 - 2017.
 
 ***********************************************************************************/

#include <math.h>
#include <string.h>

#define CLIP_WARNING 1
#ifdef CLIP_WARNING
#include <stdio.h>
#endif

#include "wavIO.h"

typedef struct _AUDIO_DATA_BUFFER {
  char  *inBufferChar;
  float *inBufferFloat;
  char  *outBufferChar;
  float *outBufferFloat;
  int readPosition;
} AUDIO_DATA_BUFFER, *H_AUDIO_DATA_BUFFER;

#define MAX_TAG_LENGTH 128
#define MAX_NUM_TAGS 5

typedef struct _exif_tag {
  char inam[MAX_TAG_LENGTH];               /* title */
  char iprd[MAX_TAG_LENGTH];               /* product, will show up as "Album" */
  char iart[MAX_TAG_LENGTH];               /* artist */
  char icrd[MAX_TAG_LENGTH];               /* creation date */
  char ignr[MAX_TAG_LENGTH];               /* genre */
} EXIF_TAG, *H_EXIF_TAG;

typedef struct _header_info {
  int totalSizeOfRiffHeader;                /* total number of bytes stored in the riff wav header including all elements */
  int nBytesRiffHeader;                     /* = TotalSizeOfRiffHeader - 8, used to calculate riff_length */
  int nBytesListHeader;                     /* used to calculate list_length */
  int offset_riff_length;                   /* offset from the beginning of the file to the entry of riff_length in the Riff header*/
  int offset_list_length;                   /* offset from the beginning of the file to the entry of list_length in the Riff header*/
  int offset_data_length;                   /* offset from the beginning of the file to the entry of data_length in the Riff header*/
} HEADER_INFO, *H_HEADER_INFO;

struct _WAVIO {
  WAVIO_ERROR_LVL errorLvl;
  unsigned int framesize;
  unsigned int fillLastInclompleteFrame;
  int delay;

  FILE* fIn;
  unsigned int nInputChannels;
  unsigned int nInputSamplerate;
  unsigned int nInputBytesperSample;
  unsigned int nInputSamplesPerChannel;

  FILE* fOut;
  unsigned int nOutputChannels;
  unsigned int nOutputSamplerate;
  unsigned int nOutputBytesperSample;
  unsigned long int nTotalSamplesWrittenPerChannel;
  
  AUDIO_DATA_BUFFER* audioBuffers;

  EXIF_TAG wavTags;
  HEADER_INFO headerInfo;
};

typedef enum io_direction_flag {
  IO_READ,
  IO_WRITE
} IODIR;

typedef struct RiffListChunk {
  char chunk_ID[4];                         /* 4 Byte, sub-chunk header-signature, contains "INAM"*/
  unsigned int chunk_length;                /* 4 Byte, length of the rest of the sub-chunk in Byte*/
  char chunk_data[MAX_TAG_LENGTH];          /* chunk_length Bytes, EXIF 2.3 compliant meta data as string */
} RIFFLISTCHUNK;

typedef struct RiffListInfoHeader {
  /* list-section */
  char list_name[4];                        /* 4 Byte, header-signature, contains "LIST"*/
  unsigned int list_length;                 /* 4 Byte, length of the rest of the list_info header in Byte*/
  char list_ID[4];                          /* 4 Byte, list type, contains "INFO"*/
  unsigned int nSub_chunks;                 /* 0 Byte, number of sub chunks of this list, won't show up in the .wav file */
  RIFFLISTCHUNK sub_chunk[MAX_NUM_TAGS];    /* RIFFLISTCHUNK */
} RIFFLISTHEADER, *HRIFFLISTHEADER;

typedef struct RiffFmtHeader {
  /* format(fmt)-section */
  char fmt_name[4];                         /* 4 Byte, header-signature, contains "fmt "*/
  unsigned int fmt_length;                  /* 4 Byte, length of the rest of the fmt header in Byte*/
  short format_tag;                         /* 2 Byte, data format of the audio samples*/
  short channels;                           /* 2 Byte, number of used channels*/
  unsigned int fs;                          /* 4 Byte, sample frequency in Herz*/
  unsigned int bytes_per_sec;               /* 4 Byte, <fs> * <block_align>*/
  short block_align;                        /* 2 Byte, <channels> * (<bits/sample> / 8)*/
  short bpsample;                           /* 2 Byte, quantizer bit depth, 8, 16 or 24 Bit*/
} RIFFFMTHEADER, *HRIFFFMTHEADER;

typedef struct RiffDataHeader {
  /* data-section */
  char data_name[4];                        /* 4 Byte, header-signature, contains "data"*/
  unsigned int data_length;                 /* 4 Byte, length of the data chunk in Byte*/
} RIFFDATAHEADER, *HRIFFDATAHEADER;

typedef struct RiffWavHeader {
  /* total length of the .wav Header: 44 Byte + (list_length + 8) */
  /* riff-section */
  char riff_name[4];                        /* 4 Byte, header-signature, contains "RIFF"*/
  unsigned int riff_length;                 /* 4 Byte, length of the rest of the header (36 Byte) + <data_length> in Byte*/
  char riff_typ[4];                         /* 4 Byte, contains the audio type, here "WAVE"*/
} RIFFWAVHEADER, *HRIFFWAVHEADER;


#define WAVIO_RIFF_HEADER_SIZE 36

static int ConvertIntToFloat(float *pfOutBuf, const char *pbInBuf, unsigned int nInBytes, unsigned int nBytesPerSample, unsigned int* bytesLeft);
static WAVIO_RETURN_CODE ConvertFloatToInt(char *OutBuf, float *InBuf, unsigned int length, unsigned int nBytesPerSample, const WAVIO_ERROR_LVL errorLvl);
static short LittleToNativeEndianShort(short x);
static int LittleToNativeEndianLong(int x);
static __inline short LittleEndian16 (short v);
static __inline int IsLittleEndian (void);
static unsigned int BigEndian32 (char a, char b, char c, char d);

static int getPathOffset(const char* fileName) {
  int i      = 0;
  int length = (NULL != fileName) ? strlen(fileName) : 0;

  for(i = length - 1; i > 0; i--) {
    if ((fileName[i] == '\\') || (fileName[i] == '/')) {
      return (i + 1);
    }
  }

  return 0;
}

static int removeWavExtension(char* fileName)
{
  char* wavSuffix = strstr(fileName, ".wav");

  if (wavSuffix != NULL) {
    strcpy(wavSuffix, "\0");
  } else {
    return -1;
  }

  return 0;
}

static int call_RIFF_header(
                    HRIFFWAVHEADER            header,
                    FILE                     *file,
                    const IODIR               io_dir
) {
  int headerSize = sizeof(RIFFWAVHEADER);

  if (IO_WRITE == io_dir) {
    header->riff_name[0]  = 'R';
    header->riff_name[1]  = 'I';
    header->riff_name[2]  = 'F';
    header->riff_name[3]  = 'F';

    header->riff_length   = LittleToNativeEndianLong(0);

    header->riff_typ[0]   = 'W';
    header->riff_typ[1]   = 'A';
    header->riff_typ[2]   = 'V';
    header->riff_typ[3]   = 'E';

    fwrite(header, headerSize, 1, file);
  } else {
    fread(header, headerSize, 1, file);
  }

  return headerSize;
}

static int call_DATA_header(
                    HRIFFDATAHEADER           header,
                    FILE                     *file,
                    const IODIR               io_dir
) {
  int headerSize = sizeof(RIFFDATAHEADER);

  if (IO_WRITE == io_dir) {
    header->data_name[0]  = 'd';
    header->data_name[1]  = 'a';
    header->data_name[2]  = 't';
    header->data_name[3]  = 'a';

    header->data_length   = LittleToNativeEndianLong(0);

    fwrite(header, headerSize, 1, file);
  } else {
    fread(header, headerSize, 1, file);
  }

  return headerSize;
}

static int call_FMT_header(
                    HRIFFFMTHEADER            header,
                    unsigned int             *nChannels,
                    unsigned int             *sampleRate,
                    unsigned int             *bytesPerSample,
                    FILE                     *file,
                    const IODIR               io_dir
) {
  int headerSize = sizeof(RIFFFMTHEADER);

  if (IO_WRITE == io_dir) {
    int tmp_nOutChannels   = *nChannels;
    int tmp_OutSampleRate  = *sampleRate;
    int tmp_bytesPerSample = *bytesPerSample;

    header->fmt_name[0]   = 'f';
    header->fmt_name[1]   = 'm';
    header->fmt_name[2]   = 't';
    header->fmt_name[3]   = ' ';

    header->fmt_length    = LittleToNativeEndianLong(16);

    if (4 == tmp_bytesPerSample) {
      header->format_tag  = LittleToNativeEndianShort(3);
    }
    else {
      header->format_tag  = LittleToNativeEndianShort(1);
    }

    header->channels      = LittleToNativeEndianShort(tmp_nOutChannels);
    header->fs            = LittleToNativeEndianLong(tmp_OutSampleRate);
    header->bpsample      = LittleToNativeEndianShort(tmp_bytesPerSample * 8);
    header->bytes_per_sec = LittleToNativeEndianLong(tmp_nOutChannels * tmp_OutSampleRate * tmp_bytesPerSample);
    header->block_align   = LittleToNativeEndianShort(tmp_nOutChannels * tmp_bytesPerSample);

    fwrite(header, headerSize, 1, file);
  } else {
    fread(header, headerSize, 1, file);

    *nChannels   = header->channels;
    *sampleRate  = header->fs;
    *bytesPerSample = header->bpsample / 8;
  }

  return headerSize;
}

static int call_LIST_header(
                    HRIFFLISTHEADER          header,
                    const char               *info_id,
                    const char               *info_data,
                    FILE                     *file,
                    const IODIR               io_dir
) {
  int info_length  = 0;
  int info_padding = 0;
  unsigned int ID  = 0;
  int headerSize   = 0;

  if ((IO_WRITE == io_dir) && (strlen(info_data) > 0)) {
    ID = header->nSub_chunks;

    if (0 == ID) {
      header->list_name[0] = 'L';
      header->list_name[1] = 'I';
      header->list_name[2] = 'S';
      header->list_name[3] = 'T';

      header->list_length  = LittleToNativeEndianLong(0);

      header->list_ID[0]   = 'I';
      header->list_ID[1]   = 'N';
      header->list_ID[2]   = 'F';
      header->list_ID[3]   = 'O';

      fwrite(header->list_name,    sizeof(char),         4, file);
      fwrite(&header->list_length, sizeof(unsigned int), 1, file);
      fwrite(header->list_ID,      sizeof(char),         4, file);

      /* add 4 Bytes list_name, 4 Bytes list_length, 4 Bytes list_ID */
      headerSize += (4 + 4 + 4);

      header->list_length = 4;
    }

    if (ID < MAX_NUM_TAGS) {
      info_length = min(strlen(info_data), MAX_TAG_LENGTH - 1);

      /* determine the number of bytes to be padded at the end of the data in info_data */
      if (info_length % 2 == 1) {
        info_padding = 1;           /* make the number of bytes even */
      } else {
        info_padding = 2;           /* add padding for a proper display */
      }

      info_length += info_padding;

      memcpy(header->sub_chunk[ID].chunk_ID, info_id, sizeof(char) * 4);
      header->sub_chunk[ID].chunk_length = LittleToNativeEndianLong(info_length);

      memset(header->sub_chunk[ID].chunk_data, 0, sizeof(char) * MAX_TAG_LENGTH);
      memcpy(header->sub_chunk[ID].chunk_data, info_data, sizeof(char) * (info_length - info_padding));

      fwrite(header->sub_chunk[ID].chunk_ID,      sizeof(char),         4,           file);
      fwrite(&header->sub_chunk[ID].chunk_length, sizeof(unsigned int), 1,           file);
      fwrite(header->sub_chunk[ID].chunk_data,    sizeof(char),         info_length, file);

      /* increase cunk counter */
      header->nSub_chunks++;

      /* add 4 Bytes chunk_ID, 4 Bytes chunk_length, info_length Bytes */
      header->list_length += LittleToNativeEndianLong((4 + 4) + info_length);

      /* add 4 Bytes chunk_ID, 4 Bytes chunk_length, info_length Bytes */
      headerSize += 4 + 4 + info_length;
    }
  } else {
    
  }

  return headerSize;
}

static void updateHeaderInfo(H_HEADER_INFO headerInfo, const int TotalHeaderSize, const int ListHeaderSize) {
  headerInfo->totalSizeOfRiffHeader = TotalHeaderSize;
  headerInfo->nBytesRiffHeader      = headerInfo->totalSizeOfRiffHeader - 8;
  headerInfo->nBytesListHeader      = ListHeaderSize;
  headerInfo->offset_riff_length    = 4;
  headerInfo->offset_list_length    = sizeof(RIFFWAVHEADER) + 4;
  headerInfo->offset_data_length    = headerInfo->totalSizeOfRiffHeader - 4;
}

int wavIO_setTags(WAVIO_HANDLE hWavIO, const char* title, const char* artist, const char* album, const char* year, const char* genre) {
  int length = 0;
  const char* inam = title + getPathOffset(title);
  const char* iart = artist;
  const char* iprd = album;
  const char* icrd = year;
  const char* ignr = genre;

  if (NULL != inam) {
    char tmp[MAX_TAG_LENGTH] = {'\0'};
    strcpy(tmp, inam);
    removeWavExtension(tmp);

    length = min(strlen(tmp), MAX_TAG_LENGTH);
    memset(hWavIO->wavTags.inam, 0, sizeof(char) * MAX_TAG_LENGTH);
    memcpy(hWavIO->wavTags.inam, tmp, sizeof(char) * length);
  }

  if (NULL != iart) {
    length = min(strlen(iart), MAX_TAG_LENGTH);
    memset(hWavIO->wavTags.iart, 0, sizeof(char) * MAX_TAG_LENGTH);
    memcpy(hWavIO->wavTags.iart, iart, sizeof(char) * length);
  }

  if (NULL != iprd) {
    length = min(strlen(iprd), MAX_TAG_LENGTH);
    memset(hWavIO->wavTags.iprd, 0, sizeof(char) * MAX_TAG_LENGTH);
    memcpy(hWavIO->wavTags.iprd, iprd, sizeof(char) * length);
  }

  if (NULL != icrd) {
    length = min(strlen(icrd), MAX_TAG_LENGTH);
    memset(hWavIO->wavTags.icrd, 0, sizeof(char) * MAX_TAG_LENGTH);
    memcpy(hWavIO->wavTags.icrd, icrd, sizeof(char) * length);
  }

  if (NULL != ignr) {
    length = min(strlen(ignr), MAX_TAG_LENGTH);
    memset(hWavIO->wavTags.ignr, 0, sizeof(char) * MAX_TAG_LENGTH);
    memcpy(hWavIO->wavTags.ignr, ignr, sizeof(char) * length);
  }

  return 0;
}

int wavIO_setErrorLvl(WAVIO_HANDLE            hWavIO,
                      const WAVIO_ERROR_LVL   errorLvl
) {
  int err = 0;

  if (NULL != hWavIO) {
    hWavIO->errorLvl = errorLvl;
  } else {
    err = (int)WAVIO_ERROR_INIT;
  }

  return err;
}

int wavIO_init(WAVIO_HANDLE *hWavIO, const unsigned int framesize, const unsigned int fillLastIncompleteInputFrame, int delay)
{

  H_AUDIO_DATA_BUFFER hBuffer = NULL;

  WAVIO_HANDLE hWavIOtemp = (WAVIO_HANDLE)calloc(1,sizeof(struct _WAVIO));

  hWavIOtemp->framesize = framesize;
  hWavIOtemp->fillLastInclompleteFrame = fillLastIncompleteInputFrame;
  hWavIOtemp->delay = delay;

  hBuffer = (AUDIO_DATA_BUFFER*) calloc(1,sizeof(AUDIO_DATA_BUFFER));
  hWavIOtemp->audioBuffers = hBuffer;

  /* pre-initialize .wav tags with default entries */
  wavIO_setTags(hWavIOtemp,
                NULL,
                "MPEG-D USAC Audio",
                "Edition 2.3",
                "2019",
                "ISO/IEC 23003-3");

  hWavIOtemp->errorLvl = WAVIO_ERR_LVL_WARNING;

 *hWavIO = hWavIOtemp;

  return 0;

}

int wavIO_setDelay(WAVIO_HANDLE hWavIO, int delay)
{
  hWavIO->delay = delay;

  return 0;
}

int wavIO_openRead(WAVIO_HANDLE hWavIO, FILE *pInFileName, unsigned int *nInChannels, unsigned int *InSampleRate, unsigned int * InBytedepth, unsigned long *nTotalSamplesPerChannel, int *nSamplesPerChannelFilled)
{
  RIFFWAVHEADER  riff_header     = {{0}};
  RIFFFMTHEADER  fmt_header      = {{0}};
  RIFFDATAHEADER data_header     = {{0}};

  const unsigned int riff = BigEndian32 ('R','I','F','F');
  const unsigned int list = BigEndian32 ('L','I','S','T');
  const unsigned int fmt  = BigEndian32 ('f','m','t',' ');
  const unsigned int data = BigEndian32 ('d','a','t','a');

  int error              = 0;
  int headerSize         = 0;
  unsigned int safetyCnt = 0;
  unsigned int chunkID   = 0;
  unsigned int skipBytes = 0;
  unsigned long int pos  = 0;
  char headerOK          = 0;

  hWavIO->fIn = pInFileName;
  if (hWavIO->fIn == NULL) {
    error = -1;
    return error;
  }

  /* loop over chunks in the .wav header*/
  while (0x07 != headerOK) {
    fread(&chunkID, sizeof(unsigned int), 1, hWavIO->fIn);
    headerSize += 4;

    if (riff == chunkID) {
      /* required header */
      headerSize -= 4;
      pos         = ftell(hWavIO->fIn);
      fseek(hWavIO->fIn, pos - 4, SEEK_SET);

      headerSize += call_RIFF_header(&riff_header,
                                     hWavIO->fIn,
                                     IO_READ);
      safetyCnt  = 0;
      headerOK  |= 1;
    }
    else if (fmt == chunkID) {
      /* required header */
      headerSize -= 4;
      pos         = ftell(hWavIO->fIn);
      fseek(hWavIO->fIn, pos - 4, SEEK_SET);

      headerSize += call_FMT_header(&fmt_header,
                                    &hWavIO->nInputChannels,
                                    &hWavIO->nInputSamplerate,
                                    &hWavIO->nInputBytesperSample,
                                    hWavIO->fIn,
                                    IO_READ);
      safetyCnt = 0;
      headerOK  |= 1 << 1;
    }
    else if (data == chunkID) {
      /* required header */
      headerSize -= 4;
      pos         = ftell(hWavIO->fIn);
      fseek(hWavIO->fIn, pos - 4, SEEK_SET);

      headerSize += call_DATA_header(&data_header,
                                     hWavIO->fIn,
                                     IO_READ);

      safetyCnt = 0;
      headerOK  |= 1 << 2;
    }
    else if (list == chunkID) {
      /* optional header */
      fread(&skipBytes, sizeof(unsigned int), 1, hWavIO->fIn);

      pos = ftell(hWavIO->fIn);
      fseek(hWavIO->fIn, pos + skipBytes, SEEK_SET);

      headerSize += skipBytes + 8;

      safetyCnt = 0;
    }
    else {
      /* all other data / unknown header */
      safetyCnt++;

      if (safetyCnt > 5000) {
        error = -2;
        break;
      }

      headerSize -= 3;
      pos         = ftell(hWavIO->fIn);
      fseek(hWavIO->fIn, pos - 3, SEEK_SET);
    }
  }

  if (0 != error) {
    return error;
  }

  *nTotalSamplesPerChannel = data_header.data_length / hWavIO->nInputChannels / hWavIO->nInputBytesperSample;
  *nInChannels             = hWavIO->nInputChannels;
  *InSampleRate            = hWavIO->nInputSamplerate;
  *InBytedepth             = hWavIO->nInputBytesperSample;

  hWavIO->audioBuffers->inBufferChar  = NULL;
  hWavIO->audioBuffers->inBufferFloat = NULL;

  hWavIO->audioBuffers->inBufferChar  = (char*)calloc(hWavIO->framesize * hWavIO->nInputChannels * hWavIO->nInputBytesperSample, sizeof(char));
  hWavIO->audioBuffers->inBufferFloat = (float*)calloc(hWavIO->framesize * hWavIO->nInputChannels, sizeof(float));
  hWavIO->audioBuffers->readPosition  = 0;
  hWavIO->nInputSamplesPerChannel     = *nTotalSamplesPerChannel;

  if (hWavIO->fillLastInclompleteFrame && (*nTotalSamplesPerChannel % hWavIO->framesize)) {
    *nSamplesPerChannelFilled = hWavIO->framesize - (*nTotalSamplesPerChannel % hWavIO->framesize);
  } else {
    *nSamplesPerChannelFilled = 0;
  }

  return error;
}

int wavIO_openWrite(WAVIO_HANDLE hWavIO, FILE *pOutFileName, unsigned int nOutChannels, unsigned int OutSampleRate, unsigned int bytesPerSample)
{ 
  RIFFWAVHEADER  riff_header     = {{0}};
  RIFFFMTHEADER  fmt_header      = {{0}};
  RIFFLISTHEADER list_header     = {{0}};
  RIFFDATAHEADER data_header     = {{0}};
  int headerSize                 = 0;

  hWavIO->fOut = pOutFileName;
  if (NULL == hWavIO->fOut) {
    return -1;
  }

  headerSize += call_RIFF_header(&riff_header,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_LIST_header(&list_header,
                                 "INAM",
                                 hWavIO->wavTags.inam,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_LIST_header(&list_header,
                                 "IART",
                                 hWavIO->wavTags.iart,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_LIST_header(&list_header,
                                 "IPRD",
                                 hWavIO->wavTags.iprd,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_LIST_header(&list_header,
                                 "ICRD",
                                 hWavIO->wavTags.icrd,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_LIST_header(&list_header,
                                 "IGNR",
                                 hWavIO->wavTags.ignr,
                                 hWavIO->fOut,
                                 IO_WRITE);

  headerSize += call_FMT_header(&fmt_header,
                                &nOutChannels,
                                &OutSampleRate,
                                &bytesPerSample,
                                hWavIO->fOut,
                                IO_WRITE);

  headerSize += call_DATA_header(&data_header,
                                 hWavIO->fOut,
                                 IO_WRITE);

  updateHeaderInfo(&hWavIO->headerInfo, headerSize, list_header.list_length);

  hWavIO->nOutputChannels       = nOutChannels;
  hWavIO->nOutputSamplerate     = OutSampleRate;
  hWavIO->nOutputBytesperSample = bytesPerSample;

  hWavIO->audioBuffers->outBufferChar  = NULL;
  hWavIO->audioBuffers->outBufferFloat = NULL;
  hWavIO->audioBuffers->outBufferChar  = (char*)calloc(hWavIO->framesize * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample, sizeof(char));
  hWavIO->audioBuffers->outBufferFloat = (float*)calloc(hWavIO->framesize * hWavIO->nOutputChannels, sizeof(float));

  return 0;
}

int wavIO_readFrame(WAVIO_HANDLE hWavIO, float **inBuffer, unsigned int *nSamplesReadPerChannel, unsigned int *isLastFrame, unsigned int * nZerosPaddedBeginning,  unsigned int * nZerosPaddedEnd)
{
  int i                = 0;
  unsigned int left    = 0;
  unsigned int *h_left = &left;
  float tempfloat      = 0;
  unsigned int j       = 0;
  int ct_pos           = -1;
  int error            = 0;
  int samplesToRead    = 0;
  int zerosToRead      = 0;
  int delaySamples     = hWavIO->delay;

  if (delaySamples > 0)
  {
    zerosToRead = min( (unsigned int)delaySamples, hWavIO->framesize );
    samplesToRead = hWavIO->framesize - zerosToRead;
    for (i=0;i<zerosToRead * (int)hWavIO->nInputChannels;i= i+hWavIO->nInputChannels)
    {
      for (j=0;j<hWavIO->nInputChannels;j++)
      {
        hWavIO->audioBuffers->inBufferFloat[i+j] = 0.0f;
      }
    }
    if (hWavIO->nInputBytesperSample > 1)
    {
      fread(hWavIO->audioBuffers->inBufferChar, hWavIO->nInputBytesperSample, samplesToRead * hWavIO->nInputChannels, hWavIO->fIn); 
      error = ConvertIntToFloat(hWavIO->audioBuffers->inBufferFloat, hWavIO->audioBuffers->inBufferChar, samplesToRead * hWavIO->nInputChannels * hWavIO->nInputBytesperSample * sizeof(char), hWavIO->nInputBytesperSample, h_left);
    }
    else
    {
      fread(hWavIO->audioBuffers->inBufferFloat,hWavIO->nInputBytesperSample, samplesToRead * hWavIO->nInputChannels, hWavIO->fIn); 
    }
    delaySamples -= zerosToRead;
    hWavIO->delay = delaySamples;
    *nSamplesReadPerChannel = samplesToRead;
    for (j=0;j<(int)(hWavIO->nInputChannels);j++)
    {
      ct_pos = -1;
      for (i = 0;i <  (int)(samplesToRead * hWavIO->nInputChannels); i = i+hWavIO->nInputChannels)
      {
        ct_pos++;
        tempfloat = hWavIO->audioBuffers->inBufferFloat[i+j];
        inBuffer[j][ct_pos + zerosToRead] = tempfloat;
      }
    }

  }
  else
  {
    int leftSamples = hWavIO->nInputSamplesPerChannel - (hWavIO->audioBuffers->readPosition + hWavIO->framesize);
    if ( leftSamples  > 0)     /* more than one frame left */
    {
      samplesToRead = hWavIO->framesize;
      *isLastFrame = 0;
    }
    else if (leftSamples == 0) /* exactly one frame left */
    {
      samplesToRead = hWavIO->framesize;
      *isLastFrame = 1;
    }
    else                       /* less than one frame left */
    {
      samplesToRead = hWavIO->nInputSamplesPerChannel - hWavIO->audioBuffers->readPosition;
      *isLastFrame = 1;
    }

    if (hWavIO->nInputBytesperSample > 1)
    {
      fread(hWavIO->audioBuffers->inBufferChar, hWavIO->nInputBytesperSample, samplesToRead * hWavIO->nInputChannels, hWavIO->fIn); 
      error = ConvertIntToFloat(hWavIO->audioBuffers->inBufferFloat, hWavIO->audioBuffers->inBufferChar, samplesToRead * hWavIO->nInputChannels * hWavIO->nInputBytesperSample * sizeof(char), hWavIO->nInputBytesperSample, h_left);
    }
    else
    {
      fread(hWavIO->audioBuffers->inBufferFloat,hWavIO->nInputBytesperSample, samplesToRead * hWavIO->nInputChannels, hWavIO->fIn); 
    }
    *nSamplesReadPerChannel = samplesToRead;


    if ( *isLastFrame )
    {
      
      /* Fill up frame with zeros if wanted */
      if ( hWavIO->fillLastInclompleteFrame )
        {
          int i = 0;
          
          /* Calculate number of samples to add  */
          int nSamplesToAdd = hWavIO->framesize - samplesToRead; 
          *nZerosPaddedEnd = nSamplesToAdd;

          for ( i = 0; i <  (int)(hWavIO->nInputChannels * nSamplesToAdd); ++i )
          {
            hWavIO->audioBuffers->inBufferFloat[i + hWavIO->nInputChannels * samplesToRead] = 0.0f;
          }
        }
    }

    for (j=0;j<(int)(hWavIO->nInputChannels);j++)
    {
      ct_pos = -1;
      for (i = 0;i <  (int)(samplesToRead * hWavIO->nInputChannels); i = i+hWavIO->nInputChannels)
      {
        ct_pos++;
        tempfloat = hWavIO->audioBuffers->inBufferFloat[i+j];
        inBuffer[j][ct_pos] = tempfloat;
      }
    }
  }

  hWavIO->audioBuffers->readPosition +=  samplesToRead;
  *nZerosPaddedBeginning = zerosToRead;

  return 0;
}

int wavIO_writeFrame_withOffset(WAVIO_HANDLE hWavIO, float **outBuffer, unsigned int nSamplesToWritePerChannel, unsigned int *nSamplesWrittenPerChannel, unsigned int offset)
{
  WAVIO_RETURN_CODE err = WAVIO_OK;
  unsigned int i        = 0;
  int ct_pos            = -1;
  unsigned int j        = 0;
  float tempfloat       = 0.0f;
  int delaySamples      = hWavIO->delay;

  if (delaySamples < 0)
  {
    unsigned int i= 0;
    int samplesToWrite = nSamplesToWritePerChannel - (-1)*delaySamples;
    int samplesToSkip = 0;
    ct_pos = -1;
    ct_pos = ct_pos + samplesToSkip;
    if (samplesToWrite < 0)
    {
      samplesToWrite = 0;
    }
    samplesToSkip = nSamplesToWritePerChannel - samplesToWrite;

    ct_pos = samplesToSkip-1;
    for (i=0; i < samplesToWrite*hWavIO->nOutputChannels; i=i+hWavIO->nOutputChannels)
    {
      ct_pos++;
      for (j=0;j<hWavIO->nOutputChannels;j++)
      {
        tempfloat = outBuffer[j][ct_pos+offset];
        hWavIO->audioBuffers->outBufferFloat[i+j] = tempfloat;
      }
    }

    err = ConvertFloatToInt(hWavIO->audioBuffers->outBufferChar,
                            hWavIO->audioBuffers->outBufferFloat,
                            samplesToWrite * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample,
                            hWavIO->nOutputBytesperSample,
                            hWavIO->errorLvl);
    fwrite(hWavIO->audioBuffers->outBufferChar, hWavIO->nOutputBytesperSample, samplesToWrite * hWavIO->nOutputChannels,hWavIO->fOut);

    if (samplesToWrite == 0)
    {
      delaySamples += nSamplesToWritePerChannel;
    }
    else
    {
      delaySamples = 0;
    }

    hWavIO->delay = delaySamples;
    *nSamplesWrittenPerChannel = samplesToWrite;
  }
  
  else
  {
    /* Interleave channels */
    for (i=0; i < nSamplesToWritePerChannel * hWavIO->nOutputChannels; i=i+hWavIO->nOutputChannels)
    {
      ct_pos++;
      for (j=0;j<hWavIO->nOutputChannels;j++)
      {
        tempfloat = outBuffer[j][ct_pos+offset];
        hWavIO->audioBuffers->outBufferFloat[i+j] = tempfloat;
      }
    }

    err = ConvertFloatToInt(hWavIO->audioBuffers->outBufferChar,
                            hWavIO->audioBuffers->outBufferFloat,
                            nSamplesToWritePerChannel * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample,
                            hWavIO->nOutputBytesperSample,
                            hWavIO->errorLvl);
    fwrite(hWavIO->audioBuffers->outBufferChar, hWavIO->nOutputBytesperSample, nSamplesToWritePerChannel * hWavIO->nOutputChannels,hWavIO->fOut);

    *nSamplesWrittenPerChannel = nSamplesToWritePerChannel;
  }

  hWavIO->nTotalSamplesWrittenPerChannel += *nSamplesWrittenPerChannel;

  return (int)err;
}

int wavIO_writeFrame(WAVIO_HANDLE hWavIO, float **outBuffer, unsigned int nSamplesToWritePerChannel, unsigned int *nSamplesWrittenPerChannel)
{
  WAVIO_RETURN_CODE err = WAVIO_OK;
  unsigned int i        = 0;
  int ct_pos            = -1;
  unsigned int j        = 0;
  float tempfloat       = 0.0f;
  int delaySamples      = hWavIO->delay;

  if (delaySamples < 0)
  {
    unsigned int i= 0;
    int samplesToWrite = nSamplesToWritePerChannel - (-1)*delaySamples;
    int samplesToSkip = 0;
    ct_pos = -1;
    ct_pos = ct_pos + samplesToSkip;
    if (samplesToWrite < 0)
    {
      samplesToWrite = 0;
    }
    samplesToSkip = nSamplesToWritePerChannel - samplesToWrite;

    ct_pos = samplesToSkip-1;
    for (i=0; i < samplesToWrite*hWavIO->nOutputChannels; i=i+hWavIO->nOutputChannels)
    {
      ct_pos++;
      for (j=0;j<hWavIO->nOutputChannels;j++)
      {
        tempfloat = outBuffer[j][ct_pos];
        hWavIO->audioBuffers->outBufferFloat[i+j] = tempfloat;
      }
    }

    err = ConvertFloatToInt(hWavIO->audioBuffers->outBufferChar,
                            hWavIO->audioBuffers->outBufferFloat,
                            samplesToWrite * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample,
                            hWavIO->nOutputBytesperSample,
                            hWavIO->errorLvl);
    fwrite(hWavIO->audioBuffers->outBufferChar, hWavIO->nOutputBytesperSample, samplesToWrite * hWavIO->nOutputChannels,hWavIO->fOut);

    if (samplesToWrite == 0)
    {
      delaySamples += nSamplesToWritePerChannel;
    }
    else
    {
      delaySamples = 0;
    }

    hWavIO->delay = delaySamples;
    *nSamplesWrittenPerChannel = samplesToWrite;
  }
  
  else
  {
    /* Interleave channels */
    for (i=0; i < nSamplesToWritePerChannel * hWavIO->nOutputChannels; i=i+hWavIO->nOutputChannels)
    {
      ct_pos++;
      for (j=0;j<hWavIO->nOutputChannels;j++)
      {
        tempfloat = outBuffer[j][ct_pos];
        hWavIO->audioBuffers->outBufferFloat[i+j] = tempfloat;
      }
    }

    err = ConvertFloatToInt(hWavIO->audioBuffers->outBufferChar,
                            hWavIO->audioBuffers->outBufferFloat,
                            nSamplesToWritePerChannel * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample,
                            hWavIO->nOutputBytesperSample,
                            hWavIO->errorLvl);
    fwrite(hWavIO->audioBuffers->outBufferChar, hWavIO->nOutputBytesperSample, nSamplesToWritePerChannel * hWavIO->nOutputChannels,hWavIO->fOut);

    *nSamplesWrittenPerChannel = nSamplesToWritePerChannel;
  }

  hWavIO->nTotalSamplesWrittenPerChannel += *nSamplesWrittenPerChannel;

  return (int)err;
}

int wavIO_updateWavHeader( WAVIO_HANDLE hWavIO, unsigned long * nTotalSamplesWrittenPerChannel )
{
  unsigned long AudioBytesWritten = 0;
  unsigned long TotalBytesWritten = 0;
  unsigned int tmp;

  /* calculate the total number of audio bytes to be published in data_length: */
  AudioBytesWritten = hWavIO->nTotalSamplesWrittenPerChannel * hWavIO->nOutputChannels * hWavIO->nOutputBytesperSample; 

  /* calucluate the total number of bytes to be published in riff_length: */
  TotalBytesWritten = AudioBytesWritten + hWavIO->headerInfo.nBytesRiffHeader;

  /* rewind file to the position of riff_length and update the field with TotalBytesWritten: */
  fseek(hWavIO->fOut, hWavIO->headerInfo.offset_riff_length, SEEK_SET);
  tmp = LittleToNativeEndianLong(TotalBytesWritten);
  fwrite(&tmp, sizeof(int), 1, hWavIO->fOut);

  /* rewind file to the position of list_length and update the field with TotalBytesWritten: */
  fseek(hWavIO->fOut, hWavIO->headerInfo.offset_list_length, SEEK_SET);
  tmp = LittleToNativeEndianLong(hWavIO->headerInfo.nBytesListHeader);
  fwrite(&tmp, sizeof(int), 1, hWavIO->fOut);

  /* rewind file to the position of data_length and update the field with AudioBytesWritten: */
  fseek(hWavIO->fOut, hWavIO->headerInfo.offset_data_length, SEEK_SET);
  tmp = LittleToNativeEndianLong(AudioBytesWritten);
  fwrite(&tmp, sizeof(int), 1, hWavIO->fOut);

  /* return the actual number of written audio samples */
  *nTotalSamplesWrittenPerChannel = hWavIO->nTotalSamplesWrittenPerChannel;
  return 0;
}

int wavIO_close(WAVIO_HANDLE hWavIO)
{
  if ( hWavIO->fIn )
  {
    fclose(hWavIO->fIn);
    free(hWavIO->audioBuffers->inBufferChar);
    free(hWavIO->audioBuffers->inBufferFloat);
  }

  if ( hWavIO->fOut )
  {
    fclose(hWavIO->fOut);
    free(hWavIO->audioBuffers->outBufferChar);
    free(hWavIO->audioBuffers->outBufferFloat);
  }

  free(hWavIO->audioBuffers);

  free(hWavIO);

  return 0;
}

static int ConvertIntToFloat(float *pfOutBuf, const char *pbInBuf, unsigned int nInBytes, unsigned int nBytesPerSample, unsigned int* bytesLeft)
{
  unsigned int i,j, nSamples, nOffset;


  if ( nBytesPerSample == 4 ) {
    memcpy(pfOutBuf, pbInBuf, nInBytes*sizeof(char));
    return 0;
  }

  if (nBytesPerSample == sizeof(short))
    {
      const short* shortBuf = (const short*) pbInBuf;
      for(j = 0; j < nInBytes/nBytesPerSample; j++) 
      {
        pfOutBuf[j] = ((float) LittleEndian16(shortBuf[j]))/32768.f;
      }
    }

  else if ( nBytesPerSample > 2 )
    {

      union { signed long s; char c[sizeof(long)]; } u;
      float fFactor =  (float)(1 << ( nBytesPerSample*8 - 1 ));
      fFactor = 1.0f / fFactor;
      
      u.s = 0;
      
      nOffset = (sizeof(long) - nBytesPerSample) * 8;
      
      nSamples = nInBytes / nBytesPerSample;

      *bytesLeft = nInBytes % nBytesPerSample;
      
      for(j = 0; j < nSamples; j++) 
        {
          int n = j * nBytesPerSample;
          
          /* convert chars to 32 bit */
          if (IsLittleEndian())
            {
              for (i = 0; i < nBytesPerSample; i++)
                {
                  u.c[sizeof(long) - 1 - i] = pbInBuf[n + nBytesPerSample - i - 1];
                }
            } 
          else
            {
              for (i = 0; i < nBytesPerSample; i++)
                {
                  u.c[i] = pbInBuf[n + nBytesPerSample - i - 1];
                }
              
            } 
          
          u.s = u.s >> nOffset;
          
          pfOutBuf[j] = ((float) u.s) * fFactor;

        }
    }
  return 0;
}

static WAVIO_RETURN_CODE ConvertFloatToInt(char *OutBuf, float *InBuf, unsigned int length, unsigned int nBytesPerSample, const WAVIO_ERROR_LVL errorLvl)
{
  WAVIO_RETURN_CODE err      = WAVIO_OK;
  unsigned int clippedMaxVal = 0;
  unsigned int clippedMinVal = 0;
  unsigned int j             = 0;

  if ( nBytesPerSample == 4 ) {
    memcpy(OutBuf, InBuf, length*sizeof(char));
  }
  else if (nBytesPerSample == sizeof(short))
  {
    union { signed short s; char c[sizeof(short)]; } u;
    int i;
    float fFactor   = (float)(1 << ( nBytesPerSample*8 - 1 ));
    u.s             = 0;
    for (j=0; j < length / nBytesPerSample; j++)
    {
      float maxVal  =  32767.f;
      float minVal  = -32768.f;
      float tmpVal  = 0.f;

      int n = j * nBytesPerSample;

      tmpVal = InBuf[j] * fFactor;

      if ( tmpVal > maxVal ) {
        tmpVal = maxVal;
        clippedMaxVal++;
      }
      if ( tmpVal < minVal ) {
        tmpVal = minVal;
        clippedMinVal++;
      }

      u.s = (signed short) tmpVal;

      if (IsLittleEndian())
      {
        for (i=0; i< (int)nBytesPerSample; i++)
        {
          OutBuf[n + i] = u.c[i];
        }
      }
      else
      {
        for (i = 0; i < (int)nBytesPerSample; i++)
        {
          OutBuf[n + nBytesPerSample - i - 1] = u.c[i + (sizeof(short) - (sizeof(short)-1))];
        }
      }
    }
  }
  else
  {
    union { signed long s; char c[sizeof(long)]; } u;
    int i;

    /* Calculate scaling factor for 24bit */
    float fFactor   = (float)(1 << ( nBytesPerSample*8 - 1 ));
    u.s             = 0;
    for (j=0; j < length / nBytesPerSample; j++)
    {
      int maxVal = (int) fFactor - 1;
      int minVal = (int) -fFactor;

      int n = j * nBytesPerSample;

      u.s = (signed long) (InBuf[j] * fFactor);

      if ( u.s > maxVal ) {
        u.s = maxVal;
        clippedMaxVal++;
      }
      if ( u.s < minVal ) {
        u.s = minVal;
        clippedMinVal++;
      }

      if (IsLittleEndian())
      {
        for (i=0;i< (int)nBytesPerSample; i++)
        {
          OutBuf[n + i] = u.c[i];
        }
      }
      else
      {
        for (i = 0; i < (int)nBytesPerSample; i++)
        {
          OutBuf[n + nBytesPerSample - i - 1] = u.c[i + (sizeof(long) - 3)];
        }
      }
    }
  }

  if (WAVIO_ERR_LVL_WARNING <= errorLvl) {
    if (clippedMaxVal != 0) {
      fprintf(stderr,"wavIO warning: sample > maxVal clipped %d times this frame\n", clippedMaxVal);
    }
    if (clippedMinVal != 0) {
      fprintf(stderr,"wavIO warning: sample < minVal clipped %d times this frame\n", clippedMinVal);
    }
  }
  if (WAVIO_ERR_LVL_STRICT <= errorLvl) {
    if ((clippedMaxVal != 0) || (clippedMinVal != 0)) {
      err = WAVIO_ERROR_CLIPPED;
    }
  }

  return err;
}

static __inline int IsLittleEndian (void)
{
  short s = 0x01 ;
  
  return *((char *) &s) ? 1 : 0;
}


static __inline short LittleEndian16 (short v)
{ /* get signed little endian (2-compl.) and returns in native format, signed */
  if (IsLittleEndian ()) return v ;
  
  else return ((v << 8) & 0xFF00) | ((v >> 8) & 0x00FF) ;
}

static unsigned int BigEndian32 (char a, char b, char c, char d)
{
  if (IsLittleEndian ())
  {
    return (unsigned int) d << 24 |
      (unsigned int) c << 16 |
      (unsigned int) b <<  8 |
      (unsigned int) a ;
  }
  else
  {
    return (unsigned int) a << 24 |
      (unsigned int) b << 16 |
      (unsigned int) c <<  8 |
      (unsigned int) d ;
  }
}


#if defined __BIG_ENDIAN__
static short LittleToNativeEndianShort(short x) 
{
  char *t = (char*)(&x);
  char tmp = t[0];
  t[0] = t[1];
  t[1] = tmp;  
  return *((short*)t);
}

static int LittleToNativeEndianLong(int x) 
{
  char *t = (char*)(&x); 
  char tmp = t[0]; 
  t[0] = t[3]; 
  t[3] = tmp;
  tmp  = t[1]; 
  t[1] = t[2];
  t[2] = tmp;
  return *((int*)t); 
}

#elif defined (_M_IX86) || defined (__i386) || defined (__amd64) || defined (__x86_64__) || defined (WIN64) || defined (_WIN64 ) || (defined(__GNUC__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
static short LittleToNativeEndianShort(short x) { return x; }
static int LittleToNativeEndianLong(int x) { return x; }

#else
#error "Not sure if we are big or little endian"
#endif

