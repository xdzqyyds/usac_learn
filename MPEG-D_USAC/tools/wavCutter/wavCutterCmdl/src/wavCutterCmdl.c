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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wavIO.h"

#define QMFLIB_MAX_CHANNELS 32
#define QMF_BYTE_DEPTH 4

typedef enum {
  TIME      = 0,
  FREQUENCY = 1,
} PROCESSING_DOMAIN;

typedef enum _debug_level {
  DEBUG_NORMAL,
  DEBUG_LVL1,
  DEBUG_LVL2
} DEBUG_LEVEL;

int main(int argc, char *argv[])
{
  WAVIO_HANDLE hWavIO_in  = 0;
  WAVIO_HANDLE hWavIO_out = 0;
  WAVIO_HANDLE hWavIO_in_real = 0;
  WAVIO_HANDLE hWavIO_in_imag = 0;
  WAVIO_HANDLE hWavIO_out_real = 0;
  WAVIO_HANDLE hWavIO_out_imag = 0;
  char* inFile        = 0;
  char qmfInFileReal[200];
  char qmfInFileImag[200];
  char* outFile       = 0;
  char qmfOutFileReal[200];
  char qmfOutFileImag[200];
  char* wavExtension  = ".wav";
  FILE *pInFileName   = 0;
  FILE *pQmfInRealFile  = 0;
  FILE *pQmfInImagFile  = 0;
  FILE *pOutFileName    = 0;
  FILE *pQmfOutRealFile = 0;
  FILE *pQmfOutImagFile  = 0;

  float **inBuffer  = 0;
  float **outBuffer = 0;
  float **qmfRealBuffer = 0;
  float **qmfImagBuffer = 0;

  int error                 = 0;
  unsigned int nInChannels  = 0;
  unsigned int InSampleRate = 0;
  unsigned int InBytedepth  = 0;
  unsigned long nTotalSamplesPerChannel = 0;
  unsigned long nTotalSamplesWrittenPerChannel = 0;
  int nSamplesPerChannelFilled = 0;

  unsigned long nTotalSamplesReadPerChannel = 0;
  unsigned long nTotalImagSamplesReadPerChannel = 0;
  unsigned int isLastFrame = 0;
  unsigned int isLastImagFrame = 0;
  int numQMFBands = 64;
  int nSamplesFramelength = 2048;

  int bQmfInput = 0;
  int bQmfOutput = 0;
  PROCESSING_DOMAIN procDomain = TIME;
  DEBUG_LEVEL debugLvl = DEBUG_NORMAL;

  unsigned int nChans = 0;

  unsigned int i = 0;

  int delayToRemove = 0;
  int truncation = 0;
  int nSamplesPerChannelCmdl = -1;
  int bitdepth = -1;
  int outputBitdepth = 0;
  int outputBytedepth = 3;
  int sample = 0;
  int channel = 0;
  int frame = 0;

  fprintf( stdout, "\n"); 
  fprintf( stdout, "******************* MPEG-D USAC Audio Coder - Edition 2.1 *********************\n");
  fprintf( stdout, "*                                                                             *\n");
  fprintf( stdout, "*                                TD/FD wavCutter                              *\n");
  fprintf( stdout, "*                                                                             *\n");
  fprintf( stdout, "*                                  %s                                *\n", __DATE__);
  fprintf( stdout, "*                                                                             *\n");
  fprintf( stdout, "*    This software may only be used in the development of the MPEG USAC Audio *\n");
  fprintf( stdout, "*    standard, ISO/IEC 23003-3 or in conforming implementations or products.  *\n");
  fprintf( stdout, "*                                                                             *\n");
  fprintf( stdout, "*******************************************************************************\n"); 
  fprintf( stdout, "\n");

  if ( argc < 4 )
  {
    fprintf(stdout, "Usage: %s -if in.wav -of out.wav [options]\n\n", argv[0]);
    fprintf(stdout, "-if <in.wav> or <in>       the latter will be extended to <input>_real.qmf\n");
    fprintf(stdout, "                           and <input>_imag.qmf\n");
    fprintf(stdout, "-of <out.wav> or <out>     the latter will be extended to <output>_real.qmf\n");
    fprintf(stdout, "                           and <output>_imag.qmf\n");
    fprintf(stdout, "-delay <i>                 positive value: number of samples to be removed\n");
    fprintf(stdout, "                                           from the beginning of the file\n");
    fprintf(stdout, "                           negative value: number of samples to be added\n");
    fprintf(stdout, "                                           at the beginning of the file\n");
    fprintf(stdout, "\n[options]:\n\n");
    fprintf(stdout, "-nSamplesPerChannel <i>    number of samples per channel of the file\n");
    fprintf(stdout, "-truncate <i>              number of samples to be removed from\n");
    fprintf(stdout, "                           the end of the file. Only applicable for TD.\n");
    fprintf(stderr, "-deblvl <0,1>              Set the debug level: 0 (default), 1\n");
    fprintf(stdout, "-bitdepth <i>              Quantization depth of the output in bits.\n");
    return 1;
  }


  /* Commandline Parsing */
  
  for ( i = 1; i < (unsigned int) argc; ++i )
    {
      if (!strcmp(argv[i],"-if"))                             /* Required */
        {
          int nWavExtensionChars = 0;

          if ( argv[i+1] ) {
            inFile = argv[i+1];
          }
          else {
            fprintf(stderr, "No output filename given.\n");
            return -1;
          }
          nWavExtensionChars = strspn(wavExtension,inFile);
          if ( nWavExtensionChars != 4 ) {
            bQmfInput = 1;
            strcpy(qmfInFileReal, inFile);
            strcpy(qmfInFileImag, inFile);
            strcat(qmfInFileReal, "_real.qmf");
            strcat(qmfInFileImag, "_imag.qmf");
          }
          i++;
        }
      else if (!strcmp(argv[i],"-of"))                        /* Required */
        {
          int nWavExtensionChars = 0;

          if ( argv[i+1] ) {
            outFile = argv[i+1];
          }
          else {
            fprintf(stderr, "No output filename given.\n");
            return -1;
          }

          nWavExtensionChars = strspn(wavExtension,outFile);
          if ( nWavExtensionChars != 4 ) {
            bQmfOutput = 1;
            strcpy(qmfOutFileReal, outFile);
            strcpy(qmfOutFileImag, outFile);
            strcat(qmfOutFileReal, "_real.qmf");
            strcat(qmfOutFileImag, "_imag.qmf");
          }
          i++;
        } 
      else if (!strcmp(argv[i],"-delay"))                     /* Required */
        {
          if ( argv[i+1] ) {
            delayToRemove = atoi(argv[i+1]);
          }
          else {
            fprintf(stderr, "No additional delay given.\n");
            return -1;
          }
          i++;
        } 
      else if (!strcmp(argv[i],"-nSamplesPerChannel"))        /* Optional */
        {
          if ( argv[i+1] ) {
            nSamplesPerChannelCmdl = atoi(argv[i+1]);
          }
          else {
            fprintf(stderr, "No additional sample value given.\n");
            return -1;
          }
          i++;
        }
      else if (!strcmp(argv[i],"-truncate"))                  /* Optional */
        {
          if ( argv[i+1] ) {
            truncation = atoi(argv[i+1]);
          }
          else {
            fprintf(stderr, "No number of samples to truncate given.\n");
            return -1;
          }
          i++;
        }
        else if (!strcmp(argv[i], "-deblvl"))
        {
          int tmp = atoi ( argv[i+1] );
          switch (tmp) {
            case 1:
            case 2:
              debugLvl = (DEBUG_LEVEL)tmp;
              break;
            default:
              debugLvl = DEBUG_NORMAL;
          }
          i++;
          continue;
        }
      else if (!strcmp(argv[i],"-bitdepth"))                  /* Optional */
        {
          if ( argv[i+1] ) {
            bitdepth = atoi(argv[i+1]);
            switch(bitdepth) {
                case 16:
                case 24:
                case 32:
                  outputBytedepth = bitdepth >> 3; /* bits to bytes */
                  break;
                default:
                  error = -1;
                  fprintf(stderr, "Unsupported wave output format. Please choose -bitDepth <16,24,32>, meaning 16, 24 bits LPCM or 32-bit IEE Float wave output.\n");
                  return -1;
              }
          }
          else {
            fprintf(stderr, "No bit depth given.\n");
            return -1;
          }
          i++;
        } 
       else {
         fprintf(stderr, "Invalid command line.\n");
         return -1;
       }
    }

  /* sanity check */
  if ( bQmfInput != bQmfOutput ) {
    fprintf(stderr, "Error: No mixed domains allowed. Either both TD or FD!\n");
    return -1;
  }
  else {
    bQmfInput ? (procDomain = FREQUENCY) : (procDomain = TIME);
  }

  if ( inFile == 0 ) {
    fprintf(stderr, "Error: No input file name given!\n");
    return -1;
  }
  else if ( outFile == 0 ) {
    fprintf(stderr, "Error: No output file name given!\n");
    return -1;
  }

  /* the actual file length */
  if ( nSamplesPerChannelCmdl > 0 ) {
    nSamplesPerChannelCmdl += abs(delayToRemove);
  }

  if ( procDomain == FREQUENCY ) {

    if ( delayToRemove % numQMFBands ) {
      fprintf(stderr, "Warning: using QMF slot granularity for delay (i.e. n*64). Using closest match.\n");
      if ( delayToRemove % numQMFBands < 32 ) {
        delayToRemove = ( delayToRemove / numQMFBands ) * numQMFBands;
      }
      else {
        delayToRemove = ( delayToRemove / numQMFBands + 1 ) * numQMFBands;
      }
    }

    if ( nSamplesPerChannelCmdl != -1 && nSamplesPerChannelCmdl % numQMFBands ) {
      fprintf(stderr, "Error: You can only have file lengths that are multiples of the number of qmfbands (i.e. n*64).\n");
      return -1;
    }

    if ( !strcmp(qmfOutFileReal, qmfInFileReal) ) {
      fprintf(stderr, "Error: Output filename must not be input filename.\n");
      return -1;
    }

    pQmfInRealFile  = fopen(qmfInFileReal, "rb");
    pQmfInImagFile  = fopen(qmfInFileImag, "rb");
    pQmfOutRealFile = fopen(qmfOutFileReal, "wb");
    pQmfOutImagFile = fopen(qmfOutFileImag, "wb");

    if ( pQmfInRealFile == NULL || pQmfInImagFile == NULL ) {
      fprintf(stderr, "Error opening input files.\n");
      return -1;
    }

    if ( pQmfOutRealFile == NULL || pQmfOutImagFile == NULL ) {
      fprintf(stderr, "Error opening output files.\n");
      return -1;
    }

    error = wavIO_init(&hWavIO_in_real, numQMFBands, 0, delayToRemove > 0 ? 0 : -delayToRemove);
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

    error = wavIO_init(&hWavIO_in_imag, numQMFBands, 0, delayToRemove > 0 ? 0 : -delayToRemove);
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

    error = wavIO_init(&hWavIO_out_real, numQMFBands, 0, -delayToRemove);
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

    error = wavIO_init(&hWavIO_out_imag, numQMFBands, 0, -delayToRemove);
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

  }
  else { 

    if ( !strcmp(inFile, outFile) ) {
      fprintf(stderr, "Error: Output filename must not be input filename.\n");
      return -2;
    }

    /* procDomain == TIME */
    pInFileName  = fopen(inFile, "rb");
    pOutFileName = fopen(outFile, "wb");

    if ( pInFileName == NULL ) {
      fprintf(stderr, "Error opening input file.\n");
      return -2;
    }

    if ( pOutFileName == NULL ) {
      fprintf(stderr, "Error opening output file.\n");
      return -2;
    }

    /* wavIO for wav input file */
    error = wavIO_init(&hWavIO_in, nSamplesFramelength, 0, delayToRemove > 0 ? 0 : -delayToRemove );
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

    /* wavIO for wav output file */
    error = wavIO_init(&hWavIO_out, nSamplesFramelength, 0, -delayToRemove);
    if ( 0 != error ) {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

    /* set tags for the output file */
    wavIO_setTags(hWavIO_out, outFile, NULL, NULL, NULL, NULL);

    /* set error level */
    if (DEBUG_LVL1 <= debugLvl) {
      wavIO_setErrorLvl(hWavIO_out, WAVIO_ERR_LVL_STRICT);
    }
  }


  if ( procDomain == FREQUENCY ) {

    error = wavIO_openRead(hWavIO_in_real, pQmfInRealFile, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error ) {
      fprintf(stderr, "Error opening input qmf real file\n");
      return -1;
    }

    error = wavIO_openRead(hWavIO_in_imag, pQmfInImagFile, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error ) {
      fprintf(stderr, "Error opening input qmf imag file\n");
      return -1;
    }

    error = wavIO_openWrite(hWavIO_out_real, pQmfOutRealFile, nInChannels, InSampleRate, QMF_BYTE_DEPTH);
    if ( 0 != error ) {
      fprintf(stderr, "Error opening output qmf\n");
      return -1;
    }

    error = wavIO_openWrite(hWavIO_out_imag, pQmfOutImagFile, nInChannels, InSampleRate, QMF_BYTE_DEPTH);
    if ( 0 != error ) {
      fprintf(stderr, "Error opening output qmf\n");
      return -1;
    }

  }
  else {

    /* wav input */
    error = wavIO_openRead(hWavIO_in, pInFileName, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error )
    {
      fprintf(stderr, "Error opening input wav\n");
      return -1;
    }

    if (bitdepth == -1) {
      /* if -bitdepth was not given, use default number of bytes per sample */
      outputBytedepth = InBytedepth;
    }

    /* wav output */
    error = wavIO_openWrite(hWavIO_out, pOutFileName, nInChannels, InSampleRate, outputBytedepth);
    if ( 0 != error ) {
      fprintf(stderr, "Error opening output wav\n");
      return -1;
    }

  }

  if ( procDomain == FREQUENCY ) {

    /* allocate buffer for qmf output */
    qmfRealBuffer = (float**) calloc(nInChannels, sizeof(float*));
    qmfImagBuffer = (float**) calloc(nInChannels, sizeof(float*));

    for (nChans = 0; nChans < nInChannels; ++nChans ) {
      qmfRealBuffer[nChans] = (float*)calloc(numQMFBands, sizeof(float));
      qmfImagBuffer[nChans] = (float*)calloc(numQMFBands, sizeof(float));
    }

  }
  else { /* procDomain == TIME */

    inBuffer  = (float**) calloc(nInChannels, sizeof(float*));
    outBuffer = (float**) calloc(nInChannels, sizeof(float*));

    for (i = 0; i < nInChannels; ++i ) {
      inBuffer[i]  = (float*)calloc( nSamplesFramelength, sizeof(float));
      outBuffer[i] = (float*)calloc( nSamplesFramelength, sizeof(float));
    }

    /* set nSamplesPerChannelCmdl if Audio Truncation is requested */
    if ((nSamplesPerChannelCmdl == -1) && (truncation > 0)) {
      nSamplesPerChannelCmdl = nTotalSamplesPerChannel - truncation;
    }

  }

  do  /*loop over all frames*/
  {
    unsigned int  nSamplesReadPerChannel = 0;
    unsigned int  nSamplesToWritePerChannel = 0;
    unsigned int  nSamplesWrittenPerChannel = 0;
    unsigned int  nSamplesToWritePerChannel_real = 0;
    unsigned int  nSamplesWrittenPerChannel_real = 0;
    unsigned int  nSamplesToWritePerChannel_imag = 0;
    unsigned int  nSamplesWrittenPerChannel_imag = 0;
    unsigned int  nZerosPaddedBeginning = 0;
    unsigned int  nZerosPaddedEnd = 0;


    if ( procDomain == FREQUENCY ) {

      /* read qmf frame */
      wavIO_readFrame(hWavIO_in_real, qmfRealBuffer, &nSamplesToWritePerChannel_real, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
      wavIO_readFrame(hWavIO_in_imag, qmfImagBuffer, &nSamplesToWritePerChannel_imag, &isLastImagFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);

      if ( isLastFrame != isLastImagFrame ) {
        fprintf(stderr, "QMF file mismatch: length of real and imag file differs.");
        break;
      }

      nTotalSamplesReadPerChannel     += nSamplesToWritePerChannel_real;
      nTotalImagSamplesReadPerChannel += nSamplesToWritePerChannel_imag;

      if ( nSamplesPerChannelCmdl > -1 ) {
        if ( ( nSamplesPerChannelCmdl - (int)nSamplesToWritePerChannel_real ) > 0 ) {
          nSamplesPerChannelCmdl -= (nSamplesToWritePerChannel_real+nZerosPaddedBeginning);
        }
        else if ( ( nSamplesPerChannelCmdl - (int)nSamplesToWritePerChannel_real ) == 0 ) {
          nSamplesPerChannelCmdl -= (nSamplesToWritePerChannel_real+nZerosPaddedBeginning);
          isLastFrame = 1;
        }
        else {
          nSamplesToWritePerChannel_real = nSamplesPerChannelCmdl;
          isLastFrame = 1;
        }
      }

      error  = wavIO_writeFrame(hWavIO_out_real, qmfRealBuffer, nSamplesToWritePerChannel_real+nZerosPaddedBeginning, &nSamplesWrittenPerChannel_real);
      error |= wavIO_writeFrame(hWavIO_out_imag, qmfImagBuffer, nSamplesToWritePerChannel_imag+nZerosPaddedBeginning, &nSamplesWrittenPerChannel_imag);

      if (0 != error) {
        fprintf(stderr, "wavIO_writeFrame returned %d\n", error);
        return error;
      }
    }

    if ( procDomain == TIME ) {

      /* read frame */
      if ( !isLastFrame ) {
        wavIO_readFrame(hWavIO_in, inBuffer, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
        nSamplesToWritePerChannel      = nSamplesReadPerChannel + nZerosPaddedBeginning;
        nTotalSamplesReadPerChannel   += nSamplesReadPerChannel;
      }

      if ( nSamplesPerChannelCmdl > -1 ) {
        if ( ( nSamplesPerChannelCmdl - (int)nSamplesReadPerChannel ) > 0 ) {
          nSamplesPerChannelCmdl -= nSamplesToWritePerChannel;
        }
        else if ( ( nSamplesPerChannelCmdl - (int)nSamplesReadPerChannel ) == 0 ) {
          nSamplesPerChannelCmdl -= nSamplesToWritePerChannel;
          isLastFrame = 1;
        }
        else {
          nSamplesToWritePerChannel = nSamplesPerChannelCmdl;
          isLastFrame = 1;
        }
      }

      error = wavIO_writeFrame(hWavIO_out, inBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);

      if (0 != error) {
        fprintf(stderr, "wavIO_writeFrame returned %d\n", error);
        return error;
      }
    }
 
    fprintf(stderr,"Processing frame: %d\n", frame++);
  }  while ( !isLastFrame && !isLastImagFrame );


  if ( bQmfOutput ) {
    wavIO_updateWavHeader(hWavIO_out_real, &nTotalSamplesWrittenPerChannel);
    wavIO_updateWavHeader(hWavIO_out_imag, &nTotalSamplesWrittenPerChannel);
  }
  else {
    wavIO_updateWavHeader(hWavIO_out, &nTotalSamplesWrittenPerChannel);
  }

  if ( procDomain == FREQUENCY ) {
    wavIO_close(hWavIO_in_real);
    wavIO_close(hWavIO_in_imag);
    wavIO_close(hWavIO_out_real);
    wavIO_close(hWavIO_out_imag);
  }

  if ( procDomain == TIME ) {
    wavIO_close(hWavIO_in);
    wavIO_close(hWavIO_out);
  }


 if (nInChannels)
  {
    if ( procDomain == TIME ) {
      for (i=0; i< nInChannels; i++) {
        free(inBuffer[i]);
        free(outBuffer[i]);
      }
    }
    else {
      for (i=0; i< nInChannels; i++) {
        free(qmfRealBuffer[i]);
        free(qmfImagBuffer[i]);
      }
    }

    if ( procDomain == TIME ) {
      free(inBuffer);
      free(outBuffer);
    }
    else {
      free(qmfRealBuffer);
      free(qmfImagBuffer);
    }
  }

  return 0;
}
