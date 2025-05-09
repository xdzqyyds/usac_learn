/***********************************************************************************
 
 This software module was originally developed by 
 
 Fraunhofer IIS
 
 in the course of development of the ISO/IEC 23003-4 for reference purposes and its 
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-4 standard. ISO/IEC gives 
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute, 
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-4 standard 
 and which satisfy any specified conformance criteria. Those intending to use this 
 software module in products are advised that its use may infringe existing patents. 
 ISO/IEC have no liability for use of this software module or modifications thereof. 
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4 
 standard.
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using 
 the code for products that do not conform to MPEG-related ITU Recommendations and/or 
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works. 
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "wavIO.h"

#include "dmx_main.h"

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

int main(int argc, char *argv[])
{
    
    unsigned int i, j, k;
    int errorFlag  = 0;    
    int frameNo    = -1;   
    
    unsigned long int NumberSamplesWritten = 0;
    unsigned long int NumberSamplesWritten_real = 0;
    unsigned long int NumberSamplesWritten_imag = 0;
    unsigned int isLastFrameWrite = 0;    
    unsigned int isLastFrameRead = 0;    
    
    char* inFilename               = NULL;
    char* outFilename              = NULL;
    char* inFilenameDmxMtx         = NULL;
    char* wavExtension             = ".wav";
    char* txtExtension             = ".txt";   
    
    char freqInFilenameReal[200];
    char freqInFilenameImag[200];
    char freqOutFilenameReal[200];
    char freqOutFilenameImag[200];
    
    WAVIO_HANDLE hWavIO      = NULL;    
    WAVIO_HANDLE hWavIO_real = NULL;
    WAVIO_HANDLE hWavIO_imag = NULL;  
    
    unsigned int frameSize          = 64;
    unsigned int numFreqBands       = 64;
    unsigned int numInChansInFile   = 0;
    unsigned int numInChans         = 0;
    unsigned int numOutChans        = 0;
    unsigned int samplingRateInFile = 0;
    unsigned int samplingRate       = 0;
    int bFillLastFrame              = 1;
    int nSamplesPerChannelFilled;
    int nSamplesPerChannelFilled_real;
    int nSamplesPerChannelFilled_imag;
    int freqIOFlag          = 0;
    int helpFlag            = 0;
    int plotInfo            = 0;

    FILE *pInFile            = NULL;
    FILE *pFreqInRealFile     = NULL;
    FILE *pFreqInImagFile     = NULL;
    FILE *pOutFile           = NULL;
    FILE *pFreqOutRealFile    = NULL;
    FILE *pFreqOutImagFile    = NULL;
    
    unsigned long nTotalSamplesPerChannel;
    unsigned long nTotalSamplesPerChannel_real;
    unsigned long nTotalSamplesPerChannel_imag;
    unsigned long nTotalSamplesWrittenPerChannel;    
    unsigned long nTotalSamplesWrittenPerChannel_real;    
    unsigned long nTotalSamplesWrittenPerChannel_imag;    
    
    unsigned int bytedepth_input  = 0;
    unsigned int bytedepth_output = 0;
    unsigned int bitdepth_output  = 0;
    
    float **inBuffer          = NULL;
    float **inBufferReal      = NULL;
    float **inBufferImag      = NULL;
    float **outBuffer         = NULL;
    float **outBufferReal     = NULL;
    float **outBufferImag     = NULL;
    float **dmxMtx            = NULL;
    
    /***********************************************************************************/
    /* Header */
    /***********************************************************************************/
    
    fprintf( stdout, "\n");
    fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                                 Downmixer                                   *\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                                %s                                  *\n", __DATE__);
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*    This software may only be used in the development of the MPEG-D Part 4:  *\n");
    fprintf( stdout, "*    Dynamic Range Control standard, ISO/IEC 23003-4 or in conforming         *\n");
    fprintf( stdout, "*    implementations or products.                                             *\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*******************************************************************************\n");
    fprintf( stdout, "\n");
    
    /***********************************************************************************/
    /* ----------------- CMDL PARSING -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark CMDL_PARSING
#endif
    
    /* Commandline Parsing */       
    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-if"))      /* Required */
        {
            unsigned long nWavExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename given.\n");
                return -1;
            }                
            nWavExtensionChars = strspn(wavExtension,inFilename);
            if ( nWavExtensionChars != 4 ) {
                freqIOFlag = 1;
                strcpy(freqInFilenameReal, inFilename);
                strcpy(freqInFilenameImag, inFilename);
                strcat(freqInFilenameReal, "_real.qmf");
                strcat(freqInFilenameImag, "_imag.qmf");
            }                
            i++;
        }
        else if (!strcmp(argv[i],"-of"))   /* Required */
        {             
            unsigned long nWavExtensionChars = 0;
            
            if ( argv[i+1] ) {
                outFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No output filename given.\n");
                return -1;
            }
            
            nWavExtensionChars = strspn(wavExtension,outFilename);
            if ( nWavExtensionChars != 4 ) {
                strcpy(freqOutFilenameReal, outFilename);
                strcpy(freqOutFilenameImag, outFilename);
                strcat(freqOutFilenameReal, "_real.qmf");
                strcat(freqOutFilenameImag, "_imag.qmf");
                if ( !freqIOFlag ) {
                    fprintf(stderr, "Mixed time/frequency domain input/output is NOT allowed!\n");
                    return -1;
                }                    
            } else if (freqIOFlag) {
                fprintf(stderr, "Mixed time/frequency domain input/output is NOT allowed!\n");
                return -1;
            }             
            i++;
        } 
        else if (!strcmp(argv[i],"-nInCh")) /* required */
        {
            numInChans = atoi(argv[i+1]);
            i++;
        }  
        else if (!strcmp(argv[i],"-nOutCh")) /* required */
        {
            numOutChans = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-nBands")) /* Optional */
        {
            numFreqBands = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-info")) /* Optional */
        {
            plotInfo = atoi(argv[i+1]);
            i++;
        } 
        else if (!strcmp(argv[i],"-dmxMtx")) /* required */
        {
            unsigned long nTxtExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFilenameDmxMtx = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for dmx matrix given.\n");
                return -1;
            }                    
            nTxtExtensionChars = strspn(txtExtension,inFilenameDmxMtx);
            
            if ( nTxtExtensionChars != 4 ) {
                fprintf(stderr, "TXT file extension missing for dmx matrix input.\n");
                return -1;
            }   
            i++;
        } 
        else if (!strcmp(argv[i],"-bitdepth")) /* Optional */
        {               
            bitdepth_output = atoi(argv[i+1]);
            if (bitdepth_output != 8 && bitdepth_output != 16 && bitdepth_output != 24 && bitdepth_output != 32) {
                fprintf(stderr, "Invalid bitdepth. Has to be 8,16,24 or 32!\n");
                return -1;
            }
            /* divide by 8 to get byte depth */
            bytedepth_output = bitdepth_output/8;
            i++;               
        } 
        else if (!strcmp(argv[i],"-h")) /* Optional */
        {
            helpFlag = 1;
        } 
        else if (!strcmp(argv[i],"-help")) /* Optional */
        {
            helpFlag = 1;
        }
        else {
            fprintf(stderr, "Invalid command line.\n");
            return -1;
        }
    }
    
    if ( ( (inFilename == NULL) || (outFilename == NULL) ||  (inFilenameDmxMtx == NULL) || (numInChans == 0) || (numOutChans == 0)) || helpFlag )
    {            
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "dmxCmdl Usage:\n\n");
        fprintf(stderr, "Example (time domain interface):      <dmxCmdl -if in.wav -of out.wav -dmxMtx dmxMtx.txt -inNch 2 -outNch 1 -bitdepth 32>\n");
        fprintf(stderr, "Example (frequency domain interface): <dmxCmdl -if in -of out -nBands 64 -dmxMtx dmxMtx.txt -inNch 2 -outNch 1 -bitdepth 32 -info 1>\n");
        
        fprintf(stderr, "-if       <input.wav>          time domain input.\n");
        fprintf(stderr, "-if       <input>              freq domain input.\n");
        fprintf(stderr, "                                  Filename will be extended to <input>_real.qmf and <input>_imag.qmf.\n");
        fprintf(stderr, "-of       <output.wav>         time domain output.\n");
        fprintf(stderr, "-of       <output>             freq domain output.\n");
        fprintf(stderr, "                                  Filename will be extended to <output>_real.qmf and <output>_imag.qmf.\n");
        fprintf(stderr, "-dmxMtx   <dmxMtx.txt>         dmx matrix as text file (format: [inChans x outChans], linear gain).\n");
        fprintf(stderr, "-nInCh    <numInChans>         number of input channels (must match with provided dmx matrix)\n");
        fprintf(stderr, "-nOutCh   <numOutChans>        number of output channels (must match with provided dmx matrix)\n");
        fprintf(stderr, "-nBands   <numFreqBands>       number of sub-bands for frequency domain processing (only needed if different from 64-band QMF)\n");
        fprintf(stderr, "-bitdepth                      specify bit depth of output file (otherwise input bitdepth is used; to avoid clipping 32 bit format should be used)\n");
        fprintf(stderr, "-info     <info>               info type (0: nearly silent, 1: print minimal information, 2: print all information).\n");                        
        return -1;
    }
    
    /***********************************************************************************/
    /* ----------------- OPEN FILES -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark OPEN_FILES
#endif
    
    /* Open input file/-s */        
    if ( freqIOFlag ) {
        if (inFilename) {
            
            pFreqInRealFile  = fopen(freqInFilenameReal, "rb");
            pFreqInImagFile  = fopen(freqInFilenameImag, "rb");
        }
        if ( pFreqInRealFile == NULL || pFreqInImagFile == NULL ) {
            fprintf(stderr, "Could not open input files: (%s and/or %s).\n", freqInFilenameReal, freqInFilenameImag );
            return -1;
        } else {
            fprintf(stderr, "Found real input file: %s.\n", freqInFilenameReal );
            fprintf(stderr, "Found imaginary input file: %s.\n", freqInFilenameImag );
        }
    }
    else {
        if (inFilename) {
            pInFile  = fopen(inFilename, "rb");
        }
        if ( pInFile == NULL ) {
            fprintf(stderr, "Could not open input file: %s.\n", inFilename );
            return -1;
        } else {
            fprintf(stderr, "Found input file: %s.\n", inFilename );
        }
    }
    
    /* Open output file/-s */
    if ( freqIOFlag ) {
        if (outFilename) {
            pFreqOutRealFile = fopen(freqOutFilenameReal, "wb");
            pFreqOutImagFile = fopen(freqOutFilenameImag, "wb");
        }
        if ( pFreqOutRealFile == NULL || pFreqOutImagFile == NULL ) {
            fprintf(stderr, "Could not open output files: (%s and/or %s).\n", freqOutFilenameReal, freqOutFilenameImag );
            return -1;
        } else {
            fprintf(stderr, "Found real output file: %s.\n", freqOutFilenameReal );
            fprintf(stderr, "Found imaginary output file: %s.\n", freqOutFilenameImag );
        }        
    }
    else {
        if (outFilename) {
            pOutFile = fopen(outFilename, "wb");
        }
        if ( pOutFile == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", outFilename );
            return -1;
        } else {
            fprintf(stderr, "Found output file: %s.\n", outFilename );
        }
    }        
    
    /*******************************************************************/
    /*                         INIT WAV IO                             */
    /*******************************************************************/
    
#ifdef __APPLE__
#pragma mark WAVIO_INIT 
#endif
    
    /* wavIO_init */
    if ( freqIOFlag ) {
        if (frameSize != numFreqBands ) {
            fprintf(stderr, "Error: frameSize != numFreqBands.\n");
            return -1;
        }
        
        errorFlag = wavIO_init(&hWavIO_real,
                               numFreqBands,
                               0,
                               0);        
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_init real.\n");
            return -1;
        }
        
        errorFlag = wavIO_init(&hWavIO_imag,
                               numFreqBands,
                               0,
                               0);        
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_init imag.\n");
            return -1;
        }
    } else {        
        errorFlag = wavIO_init(&hWavIO,
                               frameSize,
                               bFillLastFrame,
                               0);    
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_init.\n");
            return -1;
        }
    }
    
    /*******************************************************************/
    /*                      WAV IO openRead()                          */
    /*******************************************************************/
    
    /* wavIO_openRead */
    if ( freqIOFlag ) {
        errorFlag = wavIO_openRead( hWavIO_real,
                                   pFreqInRealFile,
                                   &numInChansInFile,
                                   &samplingRateInFile, 
                                   &bytedepth_input,
                                   &nTotalSamplesPerChannel_real, 
                                   &nSamplesPerChannelFilled_real);
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openRead real.\n");
            return -1;
        }
        
        errorFlag = wavIO_openRead( hWavIO_imag,
                                   pFreqInImagFile,
                                   &numInChansInFile,
                                   &samplingRateInFile, 
                                   &bytedepth_input,
                                   &nTotalSamplesPerChannel_imag, 
                                   &nSamplesPerChannelFilled_imag);
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openRead imag.\n");
            return -1;
        }        
    } else {   
        errorFlag = wavIO_openRead( hWavIO,
                                   pInFile,
                                   &numInChansInFile,
                                   &samplingRateInFile, 
                                   &bytedepth_input,
                                   &nTotalSamplesPerChannel, 
                                   &nSamplesPerChannelFilled);
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openRead.\n");
            return -1;
        }
    }    
    
    samplingRate = samplingRateInFile;
    
    /*******************************************************************/
    /*                      WAV IO openWrite()                         */
    /*******************************************************************/ 
    
    /* bytedepth output */
    if (freqIOFlag || bytedepth_output == 0) {
        bytedepth_output = bytedepth_input;
    }
    
    printf("\nDMX Cmdl Info: bitdepth_input = %d, bitdepth_output = %d.\n", 8*bytedepth_input,8*bytedepth_output);
    
    /* wavIO_openWrite */
    if ( freqIOFlag ) {
        errorFlag = wavIO_openWrite( hWavIO_real,
                                    pFreqOutRealFile,
                                    numOutChans,
                                    samplingRate,
                                    bytedepth_output);        
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openWrite real.\n");
            return -1;
        }     
        
        errorFlag = wavIO_openWrite( hWavIO_imag,
                                    pFreqOutImagFile,
                                    numOutChans,
                                    samplingRate,
                                    bytedepth_output);        
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openWrite imag.\n");
            return -1;
        }     
    } else {
        errorFlag = wavIO_openWrite( hWavIO,
                                    pOutFile,
                                    numOutChans,
                                    samplingRate,
                                    bytedepth_output);        
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during wavIO_openWrite.\n");
            return -1;
        }        
    }    
    
    /* check input */
    
    /* alloc local buffers */
    if ( !freqIOFlag ) {   
        inBuffer = (float**)calloc(numInChans,sizeof(float*));
        outBuffer = (float**)calloc(numOutChans,sizeof(float*));
        
        for (i=0; i< (unsigned int) numInChans; i++)
        {
            inBuffer[i] = (float*)calloc(frameSize,sizeof(float));
        }
        
        for (i=0; i< (unsigned int) numOutChans; i++)
        {
            outBuffer[i] = (float*)calloc(frameSize,sizeof(float));
        }
    } else {
        inBufferReal = (float**)calloc(numInChans,sizeof(float*));
        inBufferImag = (float**)calloc(numInChans,sizeof(float*));
        outBufferReal = (float**)calloc(numOutChans,sizeof(float*));
        outBufferImag = (float**)calloc(numOutChans,sizeof(float*));
        
        for (i=0; i< (unsigned int) numInChans; i++)
        {
            inBufferReal[i] = (float*)calloc(numFreqBands,sizeof(float));
            inBufferImag[i] = (float*)calloc(numFreqBands,sizeof(float));
        }
        
        for (i=0; i< (unsigned int) numOutChans; i++)
        {
            outBufferReal[i] = (float*)calloc(numFreqBands,sizeof(float));
            outBufferImag[i] = (float*)calloc(numFreqBands,sizeof(float));
        }
    }
    
    /* alloc memory for dmxMtx and read dmxMtx from file */  
    dmxMtx = (float**)calloc(numInChans,sizeof(float*));        
    for (i=0; i < numInChans; i++)
    {
        dmxMtx[i] = (float*)calloc(numOutChans,sizeof(float));
    }
    
    errorFlag = readDmxMtxFromFile( inFilenameDmxMtx, dmxMtx, numInChans, numOutChans );
    if ( 0 != errorFlag )
    {
        fprintf(stderr, "Error during readDmxMtxFromFile.\n");
        return -1;
    }
    
    if (numInChansInFile != numInChans) {
        fprintf(stderr, "Error: number of input channels different for input file and downmix matrix.\n");
        return -1;
    }
    
    /***********************************************************************************/
    /* ----------------- LOOP -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark PROCESS_LOOP
#endif
    
    {
        /* timing */
        clock_t startLap, stopLap, accuTime;    
        double elapsed_time;
        float inputDuration;
        float ratio;
        
        /* aux stuff */
        unsigned int numSamplesStillToProcess = 0;
        unsigned int numFramesStillToProcess = 0;
        unsigned int frameSizeLastFrame = 0;
        accuTime = 0;
        
        fprintf(stderr,"\nDMX processing ... \n\n");
        
        do  /*loop over all frames*/
        {
            unsigned int samplesReadPerChannel = 0;
            unsigned int samplesToWritePerChannel = 0;
            unsigned int samplesWrittenPerChannel = 0;
            unsigned int samplesWrittenPerChannel_real = 0;
            unsigned int samplesWrittenPerChannel_imag = 0;
            unsigned int nZerosPaddedBeginning = 0;
            unsigned int nZerosPaddedEnd = 0;        
            frameNo++;
            
            /* freq IO ??? */
            if (!freqIOFlag) {
                
                /* read frame */
                if (!isLastFrameRead) {                    
                    
                    errorFlag = wavIO_readFrame(hWavIO,inBuffer,&samplesReadPerChannel,&isLastFrameRead,&nZerosPaddedBeginning,&nZerosPaddedEnd); 
                    
                    if (isLastFrameRead) {                     
                        numSamplesStillToProcess = samplesReadPerChannel;
                        
                        if (numSamplesStillToProcess <= frameSize) {
                            frameSizeLastFrame = numSamplesStillToProcess;                      
                            numSamplesStillToProcess = 0;
                            numFramesStillToProcess = 0;
                            isLastFrameWrite = 1;
                        } else {
                            numSamplesStillToProcess = numSamplesStillToProcess-frameSize;
                            numFramesStillToProcess = (int) ceil(((double)numSamplesStillToProcess)/frameSize);
                            frameSizeLastFrame = numSamplesStillToProcess-(numFramesStillToProcess-1)*frameSize;                          
                        }               
                    }
                    
                } else {
                    if (numFramesStillToProcess == 1) {
                        isLastFrameWrite = 1;
                        numFramesStillToProcess--;
                    } else {
                        numFramesStillToProcess--;
                    }
                    
                    /* reset input once */
                    if (isLastFrameRead == 1) {
                        
                        isLastFrameRead = 2;
                        
                        for (i=0; i < (unsigned int) numInChans; i++)
                        {
                            for (j=0; j < (unsigned int) frameSize; j++)
                            {
                                inBuffer[i][j] = 0.f;
                            }
                        }   
                    }
                }
            } else {                                    
                errorFlag = wavIO_readFrame(hWavIO_real,inBufferReal,&samplesReadPerChannel,&isLastFrameRead,&nZerosPaddedBeginning,&nZerosPaddedEnd);
                errorFlag = wavIO_readFrame(hWavIO_imag,inBufferImag,&samplesReadPerChannel,&isLastFrameRead,&nZerosPaddedBeginning,&nZerosPaddedEnd);
                
                if (isLastFrameRead) {
                    isLastFrameWrite = 1; 
                }
            }                                        
            
            /* start lap timer */
            startLap = clock();
            
            /* process */
            if ( !freqIOFlag ) {
                for (j=0; j < (unsigned int) numOutChans; j++)
                {
                    for (k=0; k < (unsigned int) frameSize; k++)
                    {
                        outBuffer[j][k] = 0.0;
                    }
                }
                for (i=0; i < (unsigned int) numInChans; i++)
                {
                    for (j=0; j < (unsigned int) numOutChans; j++)
                    {
                        for (k=0; k < (unsigned int) frameSize; k++)
                        {
                            outBuffer[j][k] += inBuffer[i][k]*dmxMtx[i][j];
                        }
                    }
                }
            } else {
                for (j=0; j < (unsigned int) numOutChans; j++)
                {
                    for (k=0; k < (unsigned int) numFreqBands; k++)
                    {
                        outBufferReal[j][k] = 0.0;
                        outBufferImag[j][k] = 0.0;
                    }
                }
                for (i=0; i < (unsigned int) numInChans; i++)
                {
                    for (j=0; j < (unsigned int) numOutChans; j++)
                    {
                        for (k=0; k < (unsigned int) numFreqBands; k++)
                        {
                            outBufferReal[j][k] += inBufferReal[i][k]*dmxMtx[i][j];
                            outBufferImag[j][k] += inBufferImag[i][k]*dmxMtx[i][j];
                        }
                    }
                }
            }
            

            
            /* stop lap timer and accumulate */
            stopLap = clock();
            accuTime += stopLap - startLap;
            
            /* write frame */
            if (!freqIOFlag) {
                
                if (isLastFrameWrite) {
                    samplesToWritePerChannel = frameSizeLastFrame;
                } else {
                    samplesToWritePerChannel = frameSize;
                }                
                
                errorFlag = wavIO_writeFrame(hWavIO,outBuffer,samplesToWritePerChannel,&samplesWrittenPerChannel);
                
                NumberSamplesWritten += samplesWrittenPerChannel;     
                
            } else {                           
                
                samplesToWritePerChannel = numFreqBands;
                
                errorFlag = wavIO_writeFrame(hWavIO_real,outBufferReal,samplesToWritePerChannel,&samplesWrittenPerChannel_real);
                errorFlag = wavIO_writeFrame(hWavIO_imag,outBufferImag,samplesToWritePerChannel,&samplesWrittenPerChannel_imag);
                
                NumberSamplesWritten_real += samplesWrittenPerChannel_real;     
                NumberSamplesWritten_imag += samplesWrittenPerChannel_imag;     
            }       
            
        } 
        while (!isLastFrameWrite);    
        
        fprintf(stderr,"DMX finished. \n");
        
        /* processing time */
        elapsed_time = ( (double) accuTime ) / CLOCKS_PER_SEC;
        if (!freqIOFlag) {
            inputDuration = (float) NumberSamplesWritten/samplingRate;
        } else {
            inputDuration = (float) NumberSamplesWritten_real/samplingRate;
        }   
        ratio = (float) (elapsed_time) / inputDuration;
        
        if (plotInfo > 0) {        
            
            fprintf(stderr,"\nProcessed %d frames ( %.2f s ) in %.2f s.\n", frameNo, inputDuration, (float) (elapsed_time) );
            fprintf(stderr,"Processing duration / Input duration (dmxCmdl) = %4.2f %% .\n", (ratio * 100) );             
        }
    }
    
    /***********************************************************************************/
    /* ----------------- EXIT ----------------- */
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark EXIT
#endif
    
    /* free memory of dmx matrix */
    for (i=0; i < numInChans; i++) {
        free(dmxMtx[i]);
    }
    free(dmxMtx);
    
    if ( !freqIOFlag ) {
        
        errorFlag = wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_updateWavHeader.\n");
            return -1;
        } else {
            assert(nTotalSamplesWrittenPerChannel==NumberSamplesWritten);            
            fprintf(stderr, "\nOutput file %s is written.\n", outFilename);
        }
        
        errorFlag = wavIO_close(hWavIO);
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_close.\n");
            return -1;
        }
        
        /* free buffers */
        for (i=0; i < (unsigned int) numInChans; i++) {
            free(inBuffer[i]);
        }
        free(inBuffer);
        
        for (i=0; i < (unsigned int) numOutChans; i++) {
            free(outBuffer[i]);
        }
        free(outBuffer);
        
    } else {
        
        errorFlag = wavIO_updateWavHeader(hWavIO_real, &nTotalSamplesWrittenPerChannel_real);
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_updateWavHeader real.\n");
            return -1;
        } else {
            assert(nTotalSamplesWrittenPerChannel_real==NumberSamplesWritten_real);            
            fprintf(stderr, "\nReal Output file %s is written.\n", freqOutFilenameReal);
        }
        
        errorFlag = wavIO_updateWavHeader(hWavIO_imag, &nTotalSamplesWrittenPerChannel_imag);
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_updateWavHeader imag.\n");
            return -1;
        } else {
            assert(nTotalSamplesWrittenPerChannel_imag==NumberSamplesWritten_imag);            
            fprintf(stderr, "Imaginary Output file %s is written.\n", freqOutFilenameImag);
        }        
        
        errorFlag = wavIO_close(hWavIO_real);  
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_close real.\n");
            return -1;
        }
        
        errorFlag = wavIO_close(hWavIO_imag);   
        if (errorFlag != 0 ) 
        {
            fprintf(stderr, "Error during wavIO_close imag.\n");
            return -1;
        }
        
        /* free buffers */
        for (i=0; i < (unsigned int) numInChans; i++) {
            free(inBufferReal[i]);
            free(inBufferImag[i]);
        }
        free(inBufferReal);
        free(inBufferImag);
        
        for (i=0; i < (unsigned int) numOutChans; i++) {
            free(outBufferReal[i]);
            free(outBufferImag[i]);
        }
        free(outBufferReal);
        free(outBufferImag);
    }
    
    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int readDmxMtxFromFile( char* filename, float **dmxMtx, int numInputChans, int numOutputChans )
{
    
    FILE* fileHandle;
    char line[512] = {0};
    int inputChannelIdx = 0;
    int outputChannelIdx = 0;
    int i, j;
    
    /* open file */
    fileHandle = fopen(filename, "r");    
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open DMX matrix input file: %s\n",filename);        
        return -1;
    } else {
        fprintf(stderr, "\nFound DMX matrix input file: %s\n", filename );
    }         
    
    /* Get new line */
    fprintf(stderr, "Reading DMX matrix from file ... \n");    
    while ( fgets(line, 511, fileHandle) != NULL )
    {
        int i = 0;
        char* pChar = line;
        
        /* Add newline at end of line (for eof line), terminate string after newline */
        line[strlen(line)+1] = '\0';
        line[strlen(line)] = '\n';
        
        /* Replace all white spaces with new lines for easier parsing */
        while ( pChar[i] != '\0')
        {
            if ( pChar[i] == ' ' || pChar[i] == '\t')
                pChar[i] = '\n';            
            i++;
        }
        
        pChar = line;
        outputChannelIdx = 0;
        
        /* Parse line */        
        while ( (*pChar) != '\0')
        {
            while (  (*pChar) == '\n' || (*pChar) == '\r'  )
                pChar++;
            
            if ( outputChannelIdx == numOutputChans ) {
                fprintf(stderr,"DMX matrix read from file has wrong format (too many output channels)!\n");        
                return -1;
            }
            if ( inputChannelIdx == numInputChans ) {
                fprintf(stderr,"DMX matrix read from file has wrong format (too many input channels)!\n");        
                return -1;
            }
            dmxMtx[inputChannelIdx][outputChannelIdx] = (float) atof(pChar);
            
            /* Jump over parsed float value */
            while (  (*pChar) != '\n' )
                pChar++;
            
            /* Jump over new lines */
            while (  (*pChar) == '\n' || (*pChar) == '\r'  )
                pChar++;
            
            outputChannelIdx++;            
        }        
        
        if (outputChannelIdx != numOutputChans) {
            fprintf(stderr,"DMX matrix read from file has wrong format (too little columns)!\n");    
            return -1;
        }
        
        inputChannelIdx++;       
    }
    
    if (inputChannelIdx != numInputChans) {
        fprintf(stderr,"DMX matrix read from file has wrong format (too little rows)!\n");
        return -1;
    }
    
    /* print values */
    fprintf(stderr,"\ndmxMtx [linear] = [\n");
    for (i = 0; i < numInputChans; i++) {
        for (j = 0; j < numOutputChans; j++) {
            fprintf(stderr,"%2.4f ",dmxMtx[i][j]);
        }
        fprintf(stderr,"\n");
    }
    fprintf(stderr,"]\n");
    
    fclose(fileHandle);
    return 0;
    
} 

/***********************************************************************************/
