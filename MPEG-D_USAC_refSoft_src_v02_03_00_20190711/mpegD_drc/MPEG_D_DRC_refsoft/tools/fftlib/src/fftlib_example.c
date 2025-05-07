#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wavIO.h"
#include "fftlib.h"

#include "fftlib_example.h"

#define FFTLIB_MAX_CHANNELS 32
#define STFT_BYTE_DEPTH 4

int main(int argc, char *argv[])
{
    WAVIO_HANDLE hWavIO = 0;
    WAVIO_HANDLE hWavIO_real = 0;
    WAVIO_HANDLE hWavIO_imag = 0;
    char* inFile        = 0;
    char* outFile       = 0;
    char inFileRe[200];
    char inFileIm[200];
    char outFileRe[200];
    char outFileIm[200];
    char* wavExtension = ".wav";
    FILE *pInFile   = 0;
    FILE *pOutFile  = 0;
    FILE *pInFileRe  = 0;
    FILE *pInFileIm  = 0;
    FILE *pOutFileRe = 0;
    FILE *pOutFileIm = 0;
    int error           = 0;
    unsigned int nInChannels  = 0;
    unsigned int InSampleRate = 0;
    unsigned int InBytedepth  = 0;
    unsigned long nTotalSamplesPerChannel = 0;
    unsigned long nTotalSamplesWrittenPerChannel = 0;
    int nSamplesPerChannelFilled = 0;
    float **inBufferRe  = 0;
    float **inBufferIm  = 0;
    float **outBufferRe = 0;
    float **outBufferIm = 0;
    
    float *fftBufRe    = 0;
    float *fftBufIm    = 0;
    
    unsigned long nTotalSamplesReadPerChannel = 0;
    unsigned int isLastFrame = 0;
    
    int numSamplesToCompensate = 0;
    
    int frameSize = 1024;
    int fftlen    = 1024;
    int numTimeSlots = 1;
    HANDLE_FFTLIB hFFTlibF;
    HANDLE_FFTLIB hFFTlibB;
    
    int stftFlag = 0;
    int stftFrameSize = 0;
    float *stftWindow			        = NULL;
    float *stftWindowedSignalInputRe    = NULL;
    float *stftWindowedSignalOutputRe	= NULL;
    float *stftWindowedSignalInputIm	= NULL;
    float *stftWindowedSignalOutputIm	= NULL;
    float **stftWindowedSignalInputTmp  = NULL;
    float **stftWindowedSignalOutputTmp = NULL;
    int stftInputFlag = 0;
    int stftOutputFlag = 0;

    unsigned int i = 0, j = 0, k = 0, t = 0, s = 0;
    
    if ( argc < 4 )
    {
        fprintf(stderr, "-if <input.wav> or <input> \n the latter will be extended to <input>_real.stft and <input>_imag.stft\n");
        fprintf(stderr, "       STFT spec: 512 FFT, hopsize of 256, sqrt(Hann) analysis and synthesis window (see ISO/IEC 23008-3:2015/AMD3)\n");
        fprintf(stderr, "       STFT file format: channels x fft values in interleaved fashion (separate files for real and imaginary part)\n");
        fprintf(stderr, "-of <output.wav> or <output> \n the latter will be extended to <output>_real.stft and <output>_imag.stft\n");
        fprintf(stderr, "       STFT spec: 512 FFT, hopsize of 256, sqrt(Hann) analysis and synthesis window (see ISO/IEC 23008-3:2015/AMD3)\n");
        fprintf(stderr, "       STFT file format: channels x fft values in interleaved fashion (separate files for real and imaginary part)\n");
        fprintf(stderr, "-fftlen <fft_length>, default: 1024, not in combination with STFT mode or STFT input/output interface\n");
        /*fprintf(stderr, "-stft512, STFT 512 according to ISO/IEC 23008-3:2015/AMD3, (default: FFT 1024)\n");*/

        return 1;
    }
    
    /* Commandline Parsing */
    
    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-if"))      /* Required */
        {
            unsigned long nWavExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFile = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename given.\n");
                return -1;
            }
            nWavExtensionChars = strspn(wavExtension,inFile);
            if ( nWavExtensionChars != 4 ) {
                stftFlag = 1;
                stftInputFlag = 1;
                strcpy(inFileRe, inFile);
                strcpy(inFileIm, inFile);
                strcat(inFileRe, "_real.stft");
                strcat(inFileIm, "_imag.stft");
            }
            i++;
        }
        else if (!strcmp(argv[i],"-of"))  /* Required */
        {
            unsigned long nWavExtensionChars = 0;
            
            if ( argv[i+1] ) {
                outFile = argv[i+1];
            }
            else {
                fprintf(stderr, "No output filename given.\n");
                return -1;
            }
            
            nWavExtensionChars = strspn(wavExtension,outFile);
            if ( nWavExtensionChars != 4 ) {
                stftFlag = 1;
                stftOutputFlag = 1;
                strcpy(outFileRe, outFile);
                strcpy(outFileIm, outFile);
                strcat(outFileRe, "_real.stft");
                strcat(outFileIm, "_imag.stft");
            }
            i++;
        }
        else if (!strcmp(argv[i],"-fftlen"))  /* Optional */
        {
            fftlen = atoi(argv[i+1]);

            i++;
        }
        else if (!strcmp(argv[i],"-stft512")) /* Optional */
        {
            stftFlag = 1;
        }
    }
    
    /* config fft and stft */
    if (stftFlag) {
        frameSize = 1024;
        fftlen = 512;
        stftFrameSize = fftlen/2;
        numTimeSlots = frameSize/stftFrameSize;
        stftWindow = stftWindow_512;
    } else {
        frameSize = fftlen;
        stftFrameSize = 0;
        stftFlag = 0;
        numTimeSlots = 1;
        stftWindow = NULL;
    }
    
    /* init FFTLIB */
    FFTlib_Create(&hFFTlibF, fftlen, -1);
    FFTlib_Create(&hFFTlibB, fftlen, 1);
    
    /* open files*/
    if (stftInputFlag) {
        pInFileRe  = fopen(inFileRe, "rb");
        pInFileIm  = fopen(inFileIm, "rb");
    } else {
        pInFile  = fopen(inFile, "rb");
    }
    if (stftOutputFlag) {
        pOutFileRe = fopen(outFileRe, "wb");
        pOutFileIm = fopen(outFileIm, "wb");
    } else {
        pOutFile = fopen(outFile, "wb");
    }
    
    /* delay compensation */
    if (!stftOutputFlag) {
        if (stftFlag==0) {
            numSamplesToCompensate = 0;
        } else {
            numSamplesToCompensate = -stftFrameSize;
        }
    }
    
    /* init wavIO */
    error = wavIO_init(&hWavIO, frameSize, 0, numSamplesToCompensate);
    if ( 0 != error )
    {
        fprintf(stderr, "Error during initialization.\n");
        return -1;
    }
    
    if ( stftInputFlag || stftOutputFlag ) {
        
        error = wavIO_init(&hWavIO_real, numTimeSlots*fftlen/2, 0, 0);
        if ( 0 != error )
        {
            fprintf(stderr, "Error during initialization.\n");
            return -1;
        }
        
        error = wavIO_init(&hWavIO_imag, numTimeSlots*fftlen/2, 0, 0);
        if ( 0 != error )
        {
            fprintf(stderr, "Error during initialization.\n");
            return -1;
        }
    }

    if ( !stftInputFlag ) {
        
        error = wavIO_openRead(hWavIO, pInFile, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening input wav\n");
            return -1;
        }
    
    } else {
    
        error = wavIO_openRead(hWavIO_real, pInFileRe, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening input stft real file\n");
            return -1;
        }
        
        error = wavIO_openRead(hWavIO_imag, pInFileIm, &nInChannels, &InSampleRate, &InBytedepth, &nTotalSamplesPerChannel, &nSamplesPerChannelFilled);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening input stft imag file\n");
            return -1;
        }
        
    }
    
    if ( !stftOutputFlag ) {
        
        error = wavIO_openWrite(hWavIO, pOutFile, nInChannels, InSampleRate, InBytedepth);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening output wav\n");
            return -1;
        }
        
    } else {
    
        error = wavIO_openWrite(hWavIO_real, pOutFileRe, nInChannels, InSampleRate, STFT_BYTE_DEPTH);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening output stft\n");
            return -1;
        }
        
        error = wavIO_openWrite(hWavIO_imag, pOutFileIm, nInChannels, InSampleRate, STFT_BYTE_DEPTH);
        if ( 0 != error )
        {
            fprintf(stderr, "Error opening output stft\n");
            return -1;
        }
    }
    
    /* memory allocation */
    inBufferRe  = (float**) calloc(nInChannels, sizeof(float*));
    inBufferIm  = (float**) calloc(nInChannels, sizeof(float*));
    outBufferRe = (float**) calloc(nInChannels, sizeof(float*));
    outBufferIm = (float**) calloc(nInChannels, sizeof(float*));
    
    fftBufRe    = calloc(fftlen, sizeof(float));
    fftBufIm    = calloc(fftlen, sizeof(float));
    
    for (i = 0; i < nInChannels; ++i )
    {
        if (!stftInputFlag) {
            inBufferRe[i]  = calloc(frameSize, sizeof(float));
            inBufferIm[i]  = calloc(frameSize, sizeof(float));
        } else {
            inBufferRe[i]  = calloc(numTimeSlots*fftlen/2, sizeof(float));
            inBufferIm[i]  = calloc(numTimeSlots*fftlen/2, sizeof(float));
        }
        if (!stftOutputFlag) {
            outBufferRe[i] = calloc(frameSize, sizeof(float));
            outBufferIm[i] = calloc(frameSize, sizeof(float));
        } else {
            outBufferRe[i] = calloc(numTimeSlots*fftlen/2, sizeof(float));
            outBufferIm[i] = calloc(numTimeSlots*fftlen/2, sizeof(float));
        }
    }
    
    if (stftFlag) {
        stftWindowedSignalInputRe  = (float*) calloc (fftlen, sizeof (float));
        stftWindowedSignalOutputRe = (float*) calloc (fftlen, sizeof (float));
        stftWindowedSignalInputIm   = (float*) calloc (fftlen, sizeof (float));
        stftWindowedSignalOutputIm  = (float*) calloc (fftlen, sizeof (float));
        
        stftWindowedSignalInputTmp = (float**) calloc(nInChannels, sizeof (float*));
        stftWindowedSignalOutputTmp = (float**) calloc(nInChannels, sizeof (float*));
        for(i=0;i<nInChannels;i++)
        {
            stftWindowedSignalInputTmp[i] = (float*) calloc(stftFrameSize, sizeof (float));
            stftWindowedSignalOutputTmp[i] = (float*) calloc(stftFrameSize, sizeof (float));
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
        
        /* read frame */
        if (!stftInputFlag) {
            wavIO_readFrame(hWavIO, inBufferRe, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
        } else {
            wavIO_readFrame(hWavIO_real, inBufferRe, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
            wavIO_readFrame(hWavIO_imag, inBufferIm, &nSamplesReadPerChannel, &isLastFrame, &nZerosPaddedBeginning, &nZerosPaddedEnd);
        }
        
        nSamplesToWritePerChannel      = nSamplesReadPerChannel;
        nSamplesToWritePerChannel_real = nSamplesReadPerChannel;
        nSamplesToWritePerChannel_imag = nSamplesReadPerChannel;
        nTotalSamplesReadPerChannel   += nSamplesReadPerChannel;
        
        for (j = 0; j < nInChannels; j++)
        {
            for(t=0;t<numTimeSlots;t++)
            {
                if (!stftFlag) {
                    
                    FFTlib_Apply(hFFTlibF, &inBufferRe[j][t*stftFrameSize+s], &inBufferIm[j][t*stftFrameSize+s], fftBufRe, fftBufIm);
                    FFTlib_Apply(hFFTlibB, fftBufRe, fftBufIm, &outBufferRe[j][t*stftFrameSize+s], &outBufferIm[j][t*stftFrameSize+s]);
                    
                    for (k = 0; k < fftlen; ++k )
                        outBufferRe[j][k] = outBufferRe[j][k] / (float) fftlen;
                    
                } else {
                    
                    if (!stftInputFlag) {
                        
                        for(s=0;s<stftFrameSize;s++)
                        {
                            /*part of  previous frame*/
                            stftWindowedSignalInputRe[s]                 = stftWindowedSignalInputTmp[j][s] * stftWindow[s];
                            /*current frame*/
                            stftWindowedSignalInputRe[stftFrameSize + s] = stftWindow[stftFrameSize+s] * inBufferRe[j][t*stftFrameSize+s];
                            /*Update tmp buffer*/
                            stftWindowedSignalInputTmp[j][s]           = inBufferRe[j][t*stftFrameSize+s];
                        }
                        
                        /* Apply fft to input signal */
                        FFTlib_Apply(hFFTlibF,stftWindowedSignalInputRe, stftWindowedSignalInputIm, fftBufRe, fftBufIm);
                        
                    } else {
                        
                        /* full representation */
                        fftBufRe[0]        = inBufferRe[j][t*fftlen/2];
                        fftBufRe[fftlen/2] = inBufferIm[j][t*fftlen/2];
                        fftBufIm[0]        = 0;
                        fftBufIm[fftlen/2] = 0;
                        for(s=1;s<fftlen/2;s++)
                        {
                            fftBufRe[s] = inBufferRe[j][t*fftlen/2+s];
                            fftBufIm[s] = inBufferIm[j][t*fftlen/2+s];
                        }
                    }
                    
                    /* do something */
                    for(s=0;s<fftlen/2+1;s++)
                    {

                    }
                    
                    /*second half of spectrum*/
                    for(s=1;s<fftlen/2;s++)
                    {
                        fftBufRe[fftlen-s] = fftBufRe[s];
                        fftBufIm[fftlen-s] = -fftBufIm[s];
                    }
                    
                    if (!stftOutputFlag) {
                        
                        /*Apply inverse fft*/
                        FFTlib_Apply(hFFTlibB,fftBufRe, fftBufIm, stftWindowedSignalOutputRe, stftWindowedSignalOutputIm);
                        
                        for(s=0;s<stftFrameSize;s++)
                        {
                            /* overlap add */
                            outBufferRe[j][t*stftFrameSize+s] = (stftWindowedSignalOutputRe[s] / ((float) fftlen)) * stftWindow[s] + stftWindowedSignalOutputTmp[j][s] * stftWindow[stftFrameSize+s];
                            /* update output buffer */
                            stftWindowedSignalOutputTmp[j][s] = stftWindowedSignalOutputRe[stftFrameSize + s] / ((float) fftlen);
                        }
                        
                    } else {
                        
                        /* sparse representation */
                        outBufferRe[j][t*fftlen/2] = fftBufRe[0];
                        outBufferIm[j][t*fftlen/2] = fftBufRe[fftlen/2];
                        for(s=1;s<fftlen/2;s++)
                        {
                            outBufferRe[j][t*fftlen/2+s] = fftBufRe[s];
                            outBufferIm[j][t*fftlen/2+s] = fftBufIm[s];
                        }
                        
                    }
                }
            }
        }
       
        /* write frame */
        if ( !stftOutputFlag ) {
            wavIO_writeFrame(hWavIO, outBufferRe, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
        } else {
            wavIO_writeFrame(hWavIO_real, outBufferRe, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel_real);
            wavIO_writeFrame(hWavIO_imag, outBufferIm, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel_imag);
        }
        
        
    }  while (!isLastFrame);
    
    /* update header */
    if ( !stftOutputFlag ) {
        wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
    } else {
        wavIO_updateWavHeader(hWavIO_real, &nTotalSamplesWrittenPerChannel);
        wavIO_updateWavHeader(hWavIO_imag, &nTotalSamplesWrittenPerChannel);
    }
    
    /* ----------------- EXIT WAV IO ----------------- */
    wavIO_close(hWavIO);
    
    if ( stftInputFlag || stftOutputFlag ) {
        wavIO_close(hWavIO_real);
        wavIO_close(hWavIO_imag);
    };
    
    /* free memory */
    if (nInChannels)
    {
        for (i=0; i< nInChannels; i++)
        {
            free(inBufferRe[i]);
            free(inBufferIm[i]);
            free(outBufferRe[i]);
            free(outBufferIm[i]);
        }
        
        free(inBufferRe);
        free(inBufferIm);
        free(outBufferRe);
        free(outBufferIm);
        
        free(fftBufRe);
        free(fftBufIm);
    }
    
    FFTlib_Destroy(&hFFTlibF);
    FFTlib_Destroy(&hFFTlibB);
    
    if (stftFlag) {
        free(stftWindowedSignalInputRe); stftWindowedSignalInputRe = NULL;
        free(stftWindowedSignalOutputRe); stftWindowedSignalOutputRe = NULL;
        free(stftWindowedSignalInputIm); stftWindowedSignalInputIm = NULL;
        free(stftWindowedSignalOutputIm); stftWindowedSignalOutputIm = NULL;

        for(i=0;i<nInChannels;i++)
        {
            free(stftWindowedSignalInputTmp[i]);
            free(stftWindowedSignalOutputTmp[i]);
        }
        free(stftWindowedSignalInputTmp); stftWindowedSignalInputTmp = NULL;
        free(stftWindowedSignalOutputTmp); stftWindowedSignalOutputTmp = NULL;
    }
    
    return 0;
}
