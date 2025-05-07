/***********************************************************************************
 
 This software module was originally developed by 
 
 Fraunhofer IIS
 
 in the course of development of the ISO/IEC 23008-3 for reference purposes and its 
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23008-3 standard. ISO/IEC gives 
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute, 
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23008-3 standard 
 and which satisfy any specified conformance criteria. Those intending to use this 
 software module in products are advised that its use may infringe existing patents. 
 ISO/IEC have no liability for use of this software module or modifications thereof. 
 Copyright is not released for products that do not conform to the ISO/IEC 23008-3 
 standard.
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using 
 the code for products that do not conform to MPEG-related ITU Recommendations and/or 
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works. 
 
 Copyright (c) ISO/IEC 2013.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wavIO.h"
#include "qmflib.h"
#include "qmflib_hybfilter.h"

#define QMFLIB_MAX_CHANNELS 32
#define QMF_BYTE_DEPTH 4

int main(int argc, char *argv[])
{
  WAVIO_HANDLE hWavIO = 0;
  WAVIO_HANDLE hWavIO_real = 0;
  WAVIO_HANDLE hWavIO_imag = 0;
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
  int error           = 0;
  unsigned int nInChannels  = 0;
  unsigned int InSampleRate = 0;
  unsigned int InBytedepth  = 0;
  unsigned long nTotalSamplesPerChannel = 0;
  unsigned long nTotalSamplesWrittenPerChannel = 0;
  int nSamplesPerChannelFilled = 0;
  float **inBuffer  = 0;
  float **outBuffer = 0;
  float **qmfRealBuffer = 0;
  float **qmfImagBuffer = 0;

  unsigned long nTotalSamplesReadPerChannel = 0;
  unsigned int isLastFrame = 0;

  QMFLIB_POLYPHASE_ANA_FILTERBANK** pAnalysisQmf;
  QMFLIB_POLYPHASE_SYN_FILTERBANK** pSynthesisQmf;

  int bQmfInput = 0;
  int bQmfOutput = 0;
  int LdMode = 0;
  int numQMFBands = 64;
  int hybridHFAlign = 1; /* 0: Align hybrid HF part with LF part, 1: no delay alignment for HF part */
  unsigned int nChans = 0;

  QMFLIB_HYBRID_FILTER_STATE hybFilterState[QMFLIB_MAX_CHANNELS];
  QMFLIB_HYBRID_FILTER_MODE hybridMode = QMFLIB_HYBRID_OFF;

  unsigned int i = 0;
  int flushingSamplesPerChannel = 0;
  int additionalDelay = 0;
  int outputBitdepth = 0;
  int outputBytedepth = 3;
  int sample = 0;
  int channel = 0;
  int frame = 0;

  if ( argc < 4 )
    {
      fprintf(stderr, "-if <input.wav> or <input> \n the latter will be extended to <input>_real.qmf and <input>_imag.qmf\n");
      fprintf(stderr, "-of <output.wav> or <output> \n the latter will be extended to <output>_real.qmf and <output>_imag.qmf\n");
      fprintf(stderr, "-bands <num_qmf_bands>, default: 64\n");
      fprintf(stderr, "-additionalDelay, \n delay on top of qmf delay in samples, default: NO additional delay\n");
      fprintf(stderr, "-outputBitdepth, in case of qmf2wav - wav output bitdepth, default: 24 bit\n");
      fprintf(stderr, "-ld, use low delay QMF,  default: OFF\n");
      fprintf(stderr, "-hybrid71, apply hybrid analysis and synthesis with 71 bands total\n");
      fprintf(stderr, "-hybrid77, apply hybrid analysis and synthesis with 77 bands total\n");
      fprintf(stderr, "-hybalign, \n delay alignment for hybrid output between LF and HF part, default: NO alignment\n");
      return 1;
    }


  /* Commandline Parsing */
  
  for ( i = 1; i < (unsigned int) argc; ++i )
    {
      if (!strcmp(argv[i],"-if"))      /* Required */
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
      else if (!strcmp(argv[i],"-of"))  /* Required */
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
      else if (!strcmp(argv[i],"-ld"))  /* Optional */
        {
          LdMode = 1;
        } 
      else if (!strcmp(argv[i],"-bands"))  /* Optional */
        {
          if ( argv[i+1] ) {
            numQMFBands = atoi(argv[i+1]);
          }
          else {
            fprintf(stderr, "No number of qmf bands given.\n");
            return -1;
          }
          i++;
        } 
      else if (!strcmp(argv[i],"-hybrid71"))  /* Optional */
        {
          hybridMode = QMFLIB_HYBRID_THREE_TO_TEN;
        } 
      else if (!strcmp(argv[i],"-hybrid77"))  /* Optional */
        {
          hybridMode = QMFLIB_HYBRID_THREE_TO_SIXTEEN;
        } 
      else if (!strcmp(argv[i],"-hybalign"))  /* Optional */
        {
          hybridHFAlign = 0;
        } 
      else if (!strcmp(argv[i],"-additionalDelay"))  /* Optional */
        {
          if ( argv[i+1] ) {
            additionalDelay = atoi(argv[i+1]);
          }
          else {
            fprintf(stderr, "No additional delay given.\n");
            return -1;
          }
          i++;
        } 
      else if (!strcmp(argv[i],"-outputBitdepth"))  /* Optional */
        {
          if ( argv[i+1] ) {
            outputBitdepth = atoi(argv[i+1]);
           }
           else {
             fprintf(stderr, "No bit depth given. Please select 16, 24 or 32 bits.\n");
             return -1;
           }

          switch( outputBitdepth ) {
            case 16:
              outputBytedepth = 2;
              break;

            case 24:
              outputBytedepth = 3;
              break;

            case 32:
              outputBytedepth = 4;
              break;

            default:
              fprintf(stderr, "Invalid bit depth setting. Please select 16, 24 or 32 bits.\n");
              return -1;
              break;
          }

          i++;
        } 
       else {
         fprintf(stderr, "Invalid command line.\n");
         return -1;
       }
    }

  /* sanity check */
  if ( bQmfInput == 1 && bQmfOutput == 1 ) {
      fprintf(stderr, "Either input or output must be \".wav\". Otherwise, there is nothing to do.\n");
      return -1;
  }


  if ( LdMode ) {
    flushingSamplesPerChannel = numQMFBands * 2.5 + /* analysis */
                                numQMFBands * 1.5 ; /* synthesis */
  }
  else {
    flushingSamplesPerChannel = numQMFBands * 5 +     /* analysis */
                                numQMFBands * 4 + 1 ; /* synthesis */
  }

  if ( QMFLIB_HYBRID_OFF != hybridMode ) {
    flushingSamplesPerChannel += 6 * numQMFBands;
  }

  flushingSamplesPerChannel += additionalDelay;

  if ( bQmfInput ) {
    pQmfInRealFile  = fopen(qmfInFileReal, "rb");
    pQmfInImagFile  = fopen(qmfInFileImag, "rb");
  }
  else {
    pInFileName  = fopen(inFile, "rb");
  }

  if ( bQmfOutput ) {
    pQmfOutRealFile = fopen(qmfOutFileReal, "wb");
    pQmfOutImagFile = fopen(qmfOutFileImag, "wb");
  }
  else {
    pOutFileName = fopen(outFile, "wb");
  }

  /* wavIO for wav file */
  error = wavIO_init(&hWavIO, numQMFBands, 0, -flushingSamplesPerChannel);
  if ( 0 != error )
    {
      fprintf(stderr, "Error during initialization.\n");
      return -1;
    }

  /* wavIO for qmf file */
  if ( bQmfInput || bQmfOutput ) {
    error = wavIO_init(&hWavIO_real, numQMFBands, 0, 0);
    if ( 0 != error )
      {
        fprintf(stderr, "Error during initialization.\n");
        return -1;
      }

    error = wavIO_init(&hWavIO_imag, numQMFBands, 0, 0);
    if ( 0 != error )
      {
        fprintf(stderr, "Error during initialization.\n");
        return -1;
      }
  }

  if ( bQmfInput ) {

    error = wavIO_openRead(hWavIO_real, pQmfInRealFile, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error )
      {
        fprintf(stderr, "Error opening input qmf real file\n");
        return -1;
      }

    error = wavIO_openRead(hWavIO_imag, pQmfInImagFile, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error )
      {
        fprintf(stderr, "Error opening input qmf imag file\n");
        return -1;
      }

  }

  /* wav input */
  if( !bQmfInput ) {
    error = wavIO_openRead(hWavIO, pInFileName, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
    if ( 0 != error )
    {
      fprintf(stderr, "Error opening input wav\n");
      return -1;
    }
    outputBytedepth = InBytedepth;
  }

  /* wav output */
  if( !bQmfOutput ) {
    error = wavIO_openWrite(hWavIO, pOutFileName, nInChannels, InSampleRate, outputBytedepth);
    if ( 0 != error )
    {
      fprintf(stderr, "Error opening output wav\n");
      return -1;
    }
  }

  if ( bQmfOutput ) {
    error = wavIO_openWrite(hWavIO_real, pQmfOutRealFile, nInChannels, InSampleRate, QMF_BYTE_DEPTH);
    if ( 0 != error )
      {
        fprintf(stderr, "Error opening output qmf\n");
        return -1;
      }

    error = wavIO_openWrite(hWavIO_imag, pQmfOutImagFile, nInChannels, InSampleRate, QMF_BYTE_DEPTH);
    if ( 0 != error )
      {
        fprintf(stderr, "Error opening output qmf\n");
        return -1;
      }
  }

  /* allocate buffer for qmf output */
  qmfRealBuffer = (float**) calloc(nInChannels, sizeof(float*));
  qmfImagBuffer = (float**) calloc(nInChannels, sizeof(float*));

  for (nChans = 0; nChans < nInChannels; ++nChans )
  {
    qmfRealBuffer[nChans] = calloc(numQMFBands, sizeof(float));
    qmfImagBuffer[nChans] = calloc(numQMFBands, sizeof(float));
  }


  if ( QMFLIB_HYBRID_OFF != hybridMode )
    {
      for ( i = 0; i < nInChannels; ++i )
        QMFlib_InitAnaHybFilterbank(&hybFilterState[i]);

    }


  inBuffer  = (float**) calloc(nInChannels, sizeof(float*));
  outBuffer = (float**) calloc(nInChannels, sizeof(float*));

  pAnalysisQmf = (QMFLIB_POLYPHASE_ANA_FILTERBANK**) calloc(nInChannels, sizeof(QMFLIB_POLYPHASE_ANA_FILTERBANK*));
  pSynthesisQmf = (QMFLIB_POLYPHASE_SYN_FILTERBANK**) calloc(nInChannels, sizeof(QMFLIB_POLYPHASE_SYN_FILTERBANK*));

  /* Init QMFlib Analysis */
  QMFlib_InitAnaFilterbank(numQMFBands, LdMode);

  /* Init QMFLib Synthesis */
  QMFlib_InitSynFilterbank(numQMFBands, LdMode);
  
  for (i = 0; i < nInChannels; ++i )
    {
      inBuffer[i] = calloc(numQMFBands, sizeof(float));
      outBuffer[i] = calloc(numQMFBands, sizeof(float));
      QMFlib_OpenAnaFilterbank( &pAnalysisQmf[i] );
      QMFlib_OpenSynFilterbank( &pSynthesisQmf[i] );
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

    float mHybridReal[QMFLIB_MAX_HYBRID_BANDS] = { 0.0f };
    float mHybridImag[QMFLIB_MAX_HYBRID_BANDS] = { 0.0f };

    unsigned int j = 0;

    if ( !bQmfInput ) {

      /* read frame */
      if ( !isLastFrame ) {
        wavIO_readFrame(hWavIO, inBuffer, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
        nSamplesToWritePerChannel      = nSamplesReadPerChannel;
        nSamplesToWritePerChannel_real = nSamplesReadPerChannel;
        nSamplesToWritePerChannel_imag = nSamplesReadPerChannel;
        nTotalSamplesReadPerChannel   += nSamplesReadPerChannel;
      }

      /* Flush the qmf transformation / Calculate flushing samples */
      if ( nSamplesReadPerChannel < (unsigned int) numQMFBands ) {

        int samplesToAdd = 0;

        if ( flushingSamplesPerChannel < numQMFBands ) {
          if ( flushingSamplesPerChannel == 0 ) {
            flushingSamplesPerChannel = -1;
          } 
          else if ( flushingSamplesPerChannel < 0 ) {
            samplesToAdd = flushingSamplesPerChannel + numQMFBands;
            nSamplesToWritePerChannel = samplesToAdd;
          }
          else {
            samplesToAdd = flushingSamplesPerChannel;
            flushingSamplesPerChannel = -1;
            nSamplesToWritePerChannel = samplesToAdd;
          }
        }
        else {
          samplesToAdd = numQMFBands;
          nSamplesToWritePerChannel = samplesToAdd;
        }

        for ( channel = 0; channel < (int) nInChannels; ++channel ) {
          for ( sample = nSamplesReadPerChannel; sample < numQMFBands; ++sample ) {
            inBuffer[channel][sample] = 0.0f;
          }
        }
      }

      /* Apply the qmf transformation */
      for (j = 0; j < nInChannels; j++)
      {

        QMFlib_CalculateAnaFilterbank(pAnalysisQmf[j],
                                      inBuffer[j],
                                      qmfRealBuffer[j],
                                      qmfImagBuffer[j],
                                      LdMode);
      }
    }
    else {
      /* read qmf frame */
      wavIO_readFrame(hWavIO_real, qmfRealBuffer, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
      wavIO_readFrame(hWavIO_imag, qmfImagBuffer, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);

      nSamplesToWritePerChannel      = nSamplesReadPerChannel;
      nSamplesToWritePerChannel_real = nSamplesReadPerChannel;
      nSamplesToWritePerChannel_imag = nSamplesReadPerChannel;
      nTotalSamplesReadPerChannel   += nSamplesReadPerChannel;

    }

    /* Apply the inverse qmf transformation */
    for (j = 0; j < nInChannels; j++) {

      if ( !bQmfOutput ) {
        if ( QMFLIB_HYBRID_OFF != hybridMode )
        {
          /* Apply hybrid qmf and inverse hybrid qmf transformation */
          QMFlib_ApplyAnaHybFilterbank(&hybFilterState[i], hybridMode, numQMFBands, hybridHFAlign, qmfRealBuffer[j], qmfImagBuffer[j], mHybridReal, mHybridImag);

          QMFlib_ApplySynHybFilterbank(numQMFBands, hybridMode, mHybridReal, mHybridImag, qmfRealBuffer[j], qmfImagBuffer[j]); 
        }

        QMFlib_CalculateSynFilterbank(pSynthesisQmf[j],
                                      qmfRealBuffer[j],
                                      qmfImagBuffer[j],
                                      outBuffer[j],
                                      LdMode);
      }

    }

    /* write frame - wav file */
    if ( !bQmfOutput ) {
      wavIO_writeFrame(hWavIO, outBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
    }
    else {
      wavIO_writeFrame(hWavIO_real, qmfRealBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel_real);
      wavIO_writeFrame(hWavIO_imag, qmfImagBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel_imag);
    }

    flushingSamplesPerChannel -= ( numQMFBands - nSamplesReadPerChannel );
  
  }  while ( !isLastFrame || flushingSamplesPerChannel >= 0 );

  if ( bQmfOutput ) {
    wavIO_updateWavHeader(hWavIO_real, &nTotalSamplesWrittenPerChannel);
    wavIO_updateWavHeader(hWavIO_imag, &nTotalSamplesWrittenPerChannel);
  }
  else {
    wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
  }

  if ( bQmfInput || bQmfOutput ) {
    wavIO_close(hWavIO_real);
    wavIO_close(hWavIO_imag);
  }
  wavIO_close(hWavIO);


 if (nInChannels)
  {
    for (i=0; i< nInChannels; i++) {
      free(inBuffer[i]);
      free(outBuffer[i]);
      QMFlib_CloseAnaFilterbank(pAnalysisQmf[i]); 
      QMFlib_CloseSynFilterbank(pSynthesisQmf[i]); 
      free(qmfRealBuffer[i]);
      free(qmfImagBuffer[i]);
    }

    free(inBuffer);
    free(outBuffer);
    free(qmfRealBuffer);
    free(qmfImagBuffer);
    free(pAnalysisQmf);
    free(pSynthesisQmf);
  }

  return 0;
}
