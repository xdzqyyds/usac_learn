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
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "wavIO.h"

#include "peakLimiterlib.h"

#define LIM_DEFAULT_THRESHOLD         (0.89125094f) /* -1 dBFS */

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

int readPeakValueFromFile( char* filename, float* peakValue);

int main(int argc, char *argv[])
{
       
    unsigned int i, j;    
    int errorFlag  = 0;    
    int frameNo    = -1;   
    int info   = 0;
    
    unsigned int samplingRate = 0;
    unsigned int frameSize = 1024;    
    
    unsigned long int NumberSamplesWritten = 0;
    unsigned int isLastFrameWrite = 0;    
    unsigned int isLastFrameRead = 0;    
    
    char* inFilename         = NULL;
    char* inFilenameTxt      = NULL;
    char* outFilename        = NULL;
    WAVIO_HANDLE hWavIO      = NULL;
    char* txtExtension       = ".txt";
    
    int moduleDelay            = 0;
    int numSamplesToCompensate = 0;
    unsigned int numChans      = 0;
    int bFillLastFrame         = 1;     
    int nSamplesPerChannelFilled;
    int helpFlag            = 0;
    int compensateDelayFlag = 0; /* set by -d cmdl flag */
    
    FILE *pInFile            = NULL;
    FILE *pOutFile           = NULL;
    
    unsigned long nTotalSamplesPerChannel;
    unsigned long nTotalSamplesWrittenPerChannel;    
    
    unsigned int bytedepth_input  = 0;
    unsigned int bytedepth_output = 0;
    unsigned int bitdepth_output  = 0;
    
    float **inBuffer          = NULL;
    float **outBuffer         = NULL;
    float *bufferInterleaved  = NULL;

    int        errLimiter;
    TDLimiterPtr hlimiter;
    float limiterAttack             = TDL_ATTACK_DEFAULT_MS;
    float limiterRelease            = TDL_RELEASE_DEFAULT_MS;
    float limiterThreshold          = LIM_DEFAULT_THRESHOLD;
   
    /* timing */
    clock_t startLap, stopLap, accuTime;    
    double elapsed_time;
    float inputDuration;
    float ratio;
   
    /* aux stuff */
    unsigned int numSamplesStillToProcess = 0;
    unsigned int numFramesStillToProcess = 0;
    unsigned int frameSizeLastFrame = 0;
    unsigned int samplesReadPerChannel = 0;
    unsigned int samplesToWritePerChannel = 0;
    unsigned int samplesWrittenPerChannel = 0;
    unsigned int nZerosPaddedBeginning = 0;
    unsigned int nZerosPaddedEnd = 0;    

    /* check for clipping */
    unsigned int this_sample_clipped = 0;
    unsigned int clip_count_before = 0;
    unsigned int clip_count_after = 0;
    float frameGainReduction, maxGainReduction = 0.0f;
    float peakValue = 0;
    int peakControlPresent = 0;
    int peakControlTxtPresent = 0;

    fprintf( stdout, "\n");
    fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                                Peak Limiter                                 *\n");
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
            inFilename = argv[i+1];
            i++;
        }
        else if (!strcmp(argv[i],"-of"))   /* Required */
        {                                                  
            outFilename = argv[i+1];
            i++;
        }       
        else if (!strcmp(argv[i],"-d")) /* Optional */
        {
            compensateDelayFlag = 1;
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
        else if (!strcmp(argv[i],"-t")) /* Optional */
        {
            limiterThreshold = (float)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-a")) /* Optional */
        {
            limiterAttack = (float)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-r")) /* Optional */
        {
            limiterRelease = (float)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-p")) /* Optional */
        {
            peakValue = (float)atof(argv[i+1]);
            peakControlPresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-pt")) /* Optional */
        {
            unsigned long nTxtExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFilenameTxt = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for peak value given.\n");
                return -1;
            }
            nTxtExtensionChars = strspn(txtExtension,inFilenameTxt);
            
            if ( nTxtExtensionChars != 4 ) {
                fprintf(stderr, "TXT file extension missing for peak value input.\n");
                return -1;
            }
            peakControlTxtPresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-info")) /* Optional */
        {
            info = atoi(argv[i+1]);
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
            fprintf(stderr, "Invalid command line.\n\n");
            helpFlag = 1;
        }
    }
    
    if ( ((inFilename == NULL) || (outFilename == NULL) || (peakControlPresent && peakControlTxtPresent)) || helpFlag )
    {            
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "peakLimiterCmdl Usage:\n\n");
        fprintf(stderr, "Example without peak control (default):  <peakLimiterCmdl -if in.wav -of out.wav -bitdepth 32 -info 0>\n");
        fprintf(stderr, "Example with peak control:               <peakLimiterCmdl -if in.wav -of out.wav -p -5.45 -bitdepth 32 -info 1>\n");
        fprintf(stderr, "Example with peak control from txt file: <peakLimiterCmdl -if in.wav -of out.wav -pt gain.txt -bitdepth 32 -info 1>\n\n");

        fprintf(stderr, "-if       <input.wav>      Time domain input.\n");
        fprintf(stderr, "-of       <output.wav>     Time domain output.\n"); 
        fprintf(stderr, "-t        <threshold>      set limiter threshold, 0.0<threshold<1.0. Default: %0.2f.\n",LIM_DEFAULT_THRESHOLD);
        fprintf(stderr, "-a        <attack>         set limiter attack time in ms. Default: %0.1f.\n",TDL_ATTACK_DEFAULT_MS);
        fprintf(stderr, "-r        <release>        set limiter release time in ms. Default: %0.1f.\n",TDL_RELEASE_DEFAULT_MS);
        fprintf(stderr, "-p        <peak>           measured peak value of input signal in dBFS (if peak <= threshold_dB, peak limiter processing is set to bypass).\n");
        fprintf(stderr, "-pt       <peak.txt>       measured peak value of input signal in dBFS provided by a txt file (second row, first column).\n");
        fprintf(stderr, "-d                         automatically compensate limiter look-ahead delay.\n");
        fprintf(stderr, "-bitdepth                  specify bitdepth of output file (otherwise input bitdepth is used).\n");
        fprintf(stderr, "-info     <info>           info type, 0: nearly silent, 1: print minimal information (default), 2: print all information.\n");
        return -1;
    }
    
    
    /***********************************************************************************/
    /* ----------------- PEAK CONTROL INIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark PEAK_CONTROL_INIT
#endif

    if (peakControlPresent == 1) {
        if (info > 0)
            printf("\nPeakLimiter Cmdl Info: Peak value control enabled (peakValue = %2.4f [dBFS]).\n",peakValue);
    } else if (peakControlTxtPresent == 1) {
        errorFlag = readPeakValueFromFile( inFilenameTxt, &peakValue );
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during readPeakValueFromFile.\n");
            return -1;
        }
        if (info > 0)
            printf("\nPeakLimiter Cmdl Info: Txt Peak value control enabled (peakValue = %2.4f [dBFS]).\n",peakValue);
    } else {
        if (info > 0)
            printf("\nPeakLimiter Cmdl Info: Peak value control disabled (peakValue = undefined).\n");
    }
    
    /***********************************************************************************/
    /* ----------------- OPEN FILES -----------------*/
    /***********************************************************************************/

#ifdef __APPLE__
#pragma mark OPEN_FILES
#endif   

    /* Open input file/-s */   
    if (inFilename) {
        pInFile  = fopen(inFilename, "rb");
    }
    if ( pInFile == NULL ) {
        fprintf(stderr, "Could not open input file: %s.\n", inFilename );
        return -1;
    } else {
        if (info > 1)
            printf("Found input file: %s.\n", inFilename );
    }  
    
    /* Open output file/-s */
    
    if (outFilename) {
        pOutFile = fopen(outFilename, "wb");
    }
    if ( pOutFile == NULL ) {
        fprintf(stderr, "Could not open output file: %s.\n", outFilename );
        return -1;
    } else {
        if (info > 1)
            printf("Found output file: %s.\n", outFilename );
    }   
    
    /***********************************************************************************/
    /* ----------------- INIT WAV IO -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark WAVIO_INIT
#endif
        
    /* wavIO_init */       
    errorFlag = wavIO_init(&hWavIO,
                           frameSize,
                           bFillLastFrame,
                           numSamplesToCompensate);    
    if ( 0 != errorFlag )
    {
        fprintf(stderr, "Error during wavIO_init.\n");
        return -1;
    }    
    
    /* wavIO_openRead */  
    errorFlag = wavIO_openRead(hWavIO,
                               pInFile,
                               &numChans,
                               &samplingRate, 
                               &bytedepth_input,
                               &nTotalSamplesPerChannel, 
                               &nSamplesPerChannelFilled);
    if ( 0 != errorFlag )
    {
        fprintf(stderr, "Error during wavIO_openRead.\n");
        return -1;
    }
    
    /* take output bytedepth from input if commandline parameter is not set */
    if (bytedepth_output == 0) {
        bytedepth_output = bytedepth_input;
    }  

    if (info > 1)
      printf("\nPeakLimiter Cmdl Info: bytedepth_input = %d, bytedepth_output = %d.\n", bytedepth_input,bytedepth_output);

    /* wavIO_openWrite */
    errorFlag = wavIO_openWrite(hWavIO,
                                pOutFile,
                                numChans,
                                samplingRate,
                                bytedepth_output);        
    if ( 0 != errorFlag )
    {
        fprintf(stderr, "Error during wavIO_openWrite.\n");
        return -1;
    }  
    
    /***********************************************************************************/
    /* ----------------- INIT LIMITER -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark LIMITER_INIT
#endif    
    

    if (info > 0)
      printf("\nOpen PeakLimiter ...\n");
    
    /* open */
    hlimiter = createLimiter( limiterAttack, 
                              limiterRelease, 
                              limiterThreshold, 
                              numChans, 
                              samplingRate); 
    if (hlimiter == NULL) {
      fprintf(stderr, "error opening PeakLimiter instance\n");
    }

    if (peakControlPresent || peakControlTxtPresent) {
      /* activate or deactivate limiter, dependent on peakValue */
      peakValue = (float)pow(10.0f, peakValue/20.0f); /* get linear value from dB value */
      if (peakValue<limiterThreshold) {
          errLimiter = setLimiterActive(hlimiter, 0);
          if (info > 1)
              printf("PeakLimiter not active, delay only.\nLinear peak value %.3f < limiterThreshold %.3f\n", peakValue, limiterThreshold);
      }
      else {
          errLimiter = setLimiterActive(hlimiter, 1);
          if (info > 1)
              printf("PeakLimiter active.\nLinear peak value %.3f >= limiterThreshold %.3f\n", peakValue, limiterThreshold);
      }
      if (errLimiter != 0) {
          fprintf(stderr, "PeakLimiter active error\n");
      }
    }

    /* The following 5 calls of setLimiter<parameter> are not necessary, as the
       parameters are already set with the createLimiter function. They are given 
       here for reference only. */
    
    /* sampleRate */    
    errLimiter =  setLimiterSampleRate(hlimiter, samplingRate);
    if (errLimiter != 0) {
        fprintf(stderr, "PeakLimiter sample rate error\n");
    }    
    
    /* channels */    
    errLimiter =  setLimiterNChannels(hlimiter, numChans);
    if (errLimiter != 0) {
        fprintf(stderr, "PeakLimiter channel error\n");
    }    
    
    /* threshold */
    errLimiter =  setLimiterThreshold(hlimiter, limiterThreshold);
    if (errLimiter != 0) {
        fprintf(stderr, "PeakLimiter threshold error\n");
    }  
    
    /* attack */
    errLimiter =  setLimiterAttack(hlimiter, limiterAttack);
    if (errLimiter != 0) {
        fprintf(stderr, "PeakLimiter attack error\n");
    }  
    
    /* release */ 
    errLimiter =  setLimiterRelease(hlimiter, limiterRelease);
    if (errLimiter != 0) {
        fprintf(stderr, "PeakLimiter release error\n");
    }  
    
    /***********************************************************************************/
    /* ----------------- DELAY -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark DELAY
#endif
    
    /* module delay */           
    moduleDelay = getLimiterDelay(hlimiter);
    
    if (info > 1)
      printf("\nLook-Ahead delay = %d samples.\n", moduleDelay);

    /* compensate delay */
    if (compensateDelayFlag) {
        numSamplesToCompensate = -moduleDelay;
        wavIO_setDelay(hWavIO, numSamplesToCompensate);
    }      

    /* alloc local buffers */
    inBuffer = (float**)calloc(numChans,sizeof(float*));    
    outBuffer = (float**)calloc(numChans,sizeof(float*));  
    bufferInterleaved = (float*)calloc(numChans*frameSize,sizeof(float));    

    for (i=0; i< (unsigned int) numChans; i++)
    {
        inBuffer[i] = (float*)calloc(frameSize,sizeof(float));
        outBuffer[i] = (float*)calloc(frameSize,sizeof(float));
    }       
    
    /***********************************************************************************/
    /* ----------------- LOOP -----------------*/
    /***********************************************************************************/

#ifdef __APPLE__
#pragma mark PROCESS_LOOP
#endif

    accuTime = (clock_t)0.f;

    if (info > 0)
      printf("\nPeakLimiter processing ... \n");
    
    do  /*loop over all frames*/
    {
        samplesReadPerChannel = 0;
        samplesToWritePerChannel = 0;
        samplesWrittenPerChannel = 0;
        nZerosPaddedBeginning = 0;
        nZerosPaddedEnd = 0;        
        
        frameNo++;
                    
        /* read frame */
        if (!isLastFrameRead) {                    
            
            errorFlag = wavIO_readFrame(hWavIO,inBuffer,&samplesReadPerChannel,&isLastFrameRead,&nZerosPaddedBeginning,&nZerosPaddedEnd); 
            
            if (isLastFrameRead) {                     
                numSamplesStillToProcess = samplesReadPerChannel + moduleDelay;
                
                if (numSamplesStillToProcess <= frameSize) {
                    frameSizeLastFrame = numSamplesStillToProcess;                      
                    numSamplesStillToProcess = 0;
                    numFramesStillToProcess = 0;
                    isLastFrameWrite = 1;
                } else {
                    numSamplesStillToProcess = numSamplesStillToProcess-frameSize;
                    numFramesStillToProcess = (unsigned int)ceil(((double)numSamplesStillToProcess)/frameSize);
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
                
                for (i=0; i < (unsigned int) numChans; i++)
                {
                    for (j=0; j < (unsigned int) frameSize; j++)
                    {
                        inBuffer[i][j] = 0.f;
                    }
                }   
            }
        }                                          
            
        /* interleave and check for clipping */
        for (i=0; i<frameSize; i++) {
            this_sample_clipped = 0;
            for (j=0; j<numChans; j++) {
                bufferInterleaved[i*numChans+j] = inBuffer[j][i];
                if (fabs(inBuffer[j][i]) > limiterThreshold)
                  this_sample_clipped = 1;
            }
            clip_count_before += this_sample_clipped;
        }

        /* start lap timer */
        startLap = clock();

        /* limiter processing */
        if (applyLimiter(hlimiter, bufferInterleaved, (unsigned int)(frameSize)) )
        {
            fprintf(stderr,"PeakLimiter application error\n");
        }

        frameGainReduction = getLimiterMaxGainReduction(hlimiter);
        if (frameGainReduction > maxGainReduction)
        {
            maxGainReduction = frameGainReduction;
        }
        
        /* stop lap timer and accumulate */
        stopLap = clock();
        accuTime += stopLap - startLap;         

        /* deinterleave and check for clipping */
        for (i=0; i<frameSize; i++) {
            this_sample_clipped = 0;
            for (j=0; j<numChans; j++) {
                outBuffer[j][i] = bufferInterleaved[i*numChans+j];
                if (fabs(outBuffer[j][i]) > limiterThreshold)
                  this_sample_clipped = 1;
            }
            clip_count_after += this_sample_clipped;
        }
        
        /* write frame */            
        if (isLastFrameWrite) {
            samplesToWritePerChannel = frameSizeLastFrame;
        } else {
            samplesToWritePerChannel = frameSize;
        }                
        
        errorFlag = wavIO_writeFrame(hWavIO,outBuffer,samplesToWritePerChannel,&samplesWrittenPerChannel);
        
        NumberSamplesWritten += samplesWrittenPerChannel;     
        
    } 
    while (!isLastFrameWrite);    
    
    if (info > 0)
      printf("PeakLimiter finished. \n");

    /* processing time */
    elapsed_time = ( (double) accuTime ) / CLOCKS_PER_SEC;
    inputDuration = (float) NumberSamplesWritten/samplingRate;

    ratio = (float) (elapsed_time) / inputDuration;
    
    if (info > 1) {        
        printf("\nProcessed %d frames ( %.2f s ) in %.2f s.\n", frameNo, inputDuration, (float) (elapsed_time) );
        printf("Processing duration / Input duration = %4.2f %% .\n", (ratio * 100) );
        printf("\n%d samples exceed threshold before limiter,\n",clip_count_before);
        printf("%d samples exceed threshold after limiter.\n",clip_count_after);
        printf("Maximum gain reduction: %.1f dB\n",maxGainReduction);
    }
    
    /***********************************************************************************/
    /* ----------------- EXIT WAV IO ----------------- */
    /***********************************************************************************/

#ifdef __APPLE__
#pragma mark EXIT_WAVIO
#endif
    
    errorFlag = wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
    if (errorFlag != 0 ) 
    {
        fprintf(stderr, "Error during wavIO_updateWavHeader.\n");
        return -1;
    } else {
        assert(nTotalSamplesWrittenPerChannel==NumberSamplesWritten);            
        printf("\nOutput file %s is written.\n", outFilename);
    }
    
    errorFlag = wavIO_close(hWavIO);
    if (errorFlag != 0 ) 
    {
        fprintf(stderr, "Error during wavIO_close.\n");
        return -1;
    }
    
    /* free buffers */
    for (i=0; i < (unsigned int) numChans; i++) {
        free(inBuffer[i]);
        free(outBuffer[i]);
    }
    free(inBuffer);
    free(outBuffer);
    free(bufferInterleaved);
                
    /***********************************************************************************/
    /* ----------------- EXIT LIMITER ----------------- */
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark EXIT_LIMITER
#endif
    
    /* clean up, free memory */
    if (info > 0)
      printf("Close PeakLimiter ... \n");
    destroyLimiter(hlimiter);
    
    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int readPeakValueFromFile( char* filename, float* peakValue)
{
    
    FILE* fileHandle;
    char line[512] = {0};
    int rowNumber = 0;
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open peak value input file: %s\n",filename);
        return -1;
    } else {
        fprintf(stderr, "\nFound peak value input file: %s\n", filename );
    }
    
    /* Get new line */
    fprintf(stderr, "Reading peak value from file ... \n");
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
        
        /* Parse line */
        if ( (*pChar) != '\0' && rowNumber == 1)
        {
            while (  (*pChar) == '\n' || (*pChar) == '\r'  )
                pChar++;
            
            peakValue[0] = (float) atof(pChar);
        }
        
        rowNumber++;
    }
    
    /* print values */
    fprintf(stderr,"\npeakValue [dBFS] = %2.4f\n",peakValue[0]);
    
    fclose(fileHandle);
    return 0;
    
}

/***********************************************************************************/
