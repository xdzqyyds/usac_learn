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

#define CL_DEFAULT            (-31.f)
#define TL_DEFAULT            (-31.f)

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

static int readLoudnessNormalizationGainFromFile( char* filename, float* txtGain);
static int readSelProcOutFromFile( char* , float*, float* , int* , int* , int*, float*, float*, int*, int*, int* );

int main(int argc, char *argv[])
{
       
    unsigned int i, j;    
    int errorFlag      = 0;    
    int frameNo        = -1;   
    int plotInfo       = 0;

    float CL           = CL_DEFAULT;
    float TL           = TL_DEFAULT;
    float txtGain      = 0;
    float selProcGain  = 0;
    float directGain   = 0;
    float loudnessNormalizationGainDb = 0.f;
    float loudnessNormalizationGain   = 0.f;
    float truePeak = 0.f;
    
    int defaultModePresent = 0;
    int directGainModePresent = 0;
    int txtGainModePresent = 0;
    int selProcDataModePresent = 0;
    
    float loudnessNormalizationGainModificationDb = 0.f;
    int baseChannelCount, targetChannelCount, numSelectedDrcSets;
    int selectedDrcSetIds[3], selectedDownmixIds[3];
    float boost = 1.0f;
    float compress = 1.0f;
    int drcCharacteristicTarget = 0;
    
    unsigned int samplingRate = 0;
    unsigned int frameSize = 1024;    
    
    unsigned long int NumberSamplesWritten = 0;
    unsigned int isLastFrameWrite = 0;    
    unsigned int isLastFrameRead = 0;    
    
    char* inFilename         = NULL;
    char* inFilenameTxt      = NULL;
    char* outFilename        = NULL;
    char* txtExtension       = ".txt";

    WAVIO_HANDLE hWavIO      = NULL;
    
    int moduleDelay            = 0;
    int numSamplesToCompensate = 0;
    unsigned int numChans      = 0;
    int bFillLastFrame         = 1;     
    int nSamplesPerChannelFilled;
    int helpFlag            = 0;
    
    FILE *pInFile            = NULL;
    FILE *pOutFile           = NULL;
    
    unsigned long nTotalSamplesPerChannel;
    unsigned long nTotalSamplesWrittenPerChannel;    
    
    unsigned int bytedepth_input  = 0;
    unsigned int bytedepth_output = 0;
    unsigned int bitdepth_output  = 0;
    
    float **inBuffer          = NULL;
    float **outBuffer         = NULL;

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

    fprintf( stdout, "\n");
    fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                            Loudness Normalizer                              *\n");
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
            if ( argv[i+1] ) {
                inFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename given.\n");
                return -1;
            }
            i++;
        }
        else if (!strcmp(argv[i],"-of"))   /* Required */
        {
            if ( argv[i+1] ) {
                outFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No output filename given.\n");
                return -1;
            }
            i++;
        }
        else if (!strcmp(argv[i],"-CL")) /* Required when no direct gain */
        {
            CL = (float)atof(argv[i+1]);
            if (CL > 0) {
                fprintf(stderr, "CL error: Only negative values supported on dBFS scale.\n");
                return -1;
            }
            defaultModePresent = 1;
            directGainModePresent = 0;
            txtGainModePresent = 0;
            selProcDataModePresent = 0;
            i++;
        }
        else if (!strcmp(argv[i],"-TL")) /* Required when no direct gain */
        {
            TL = (float)atof(argv[i+1]);
            if (TL > 0) {
                fprintf(stderr, "TL error: Only negative values supported on dBFS scale.\n");
                return -1;
            }
            defaultModePresent = 1;
            directGainModePresent = 0;
            txtGainModePresent = 0;
            selProcDataModePresent = 0;
            i++;
        }
        else if (!strcmp(argv[i],"-txtGain")) /* Optional */
        {
            unsigned long nTxtExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFilenameTxt = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for loudness normalization gain given.\n");
                return -1;
            }
            nTxtExtensionChars = strspn(txtExtension,inFilenameTxt);
            
            if ( nTxtExtensionChars != 4 ) {
                fprintf(stderr, "TXT file extension missing for loudness normalization gain input.\n");
                return -1;
            }
            defaultModePresent = 0;
            directGainModePresent = 0;
            txtGainModePresent = 1;
            selProcDataModePresent = 0;
            i++;
        }
        else if (!strcmp(argv[i],"-selProcData")) /* Optional */
        {
            unsigned long nTxtExtensionChars = 0;
            
            if ( argv[i+1] ) {
                inFilenameTxt = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for loudness normalization gain given.\n");
                return -1;
            }
            nTxtExtensionChars = strspn(txtExtension,inFilenameTxt);
            
            if ( nTxtExtensionChars != 4 ) {
                fprintf(stderr, "TXT file extension missing for loudness normalization gain input.\n");
                return -1;
            }
            defaultModePresent = 0;
            directGainModePresent = 0;
            txtGainModePresent = 0;
            selProcDataModePresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-directGain")) /* Optional */
        {
            directGain = (float)atof(argv[i+1]);
            defaultModePresent = 0;
            directGainModePresent = 1;
            txtGainModePresent = 0;
            selProcDataModePresent = 0;
            i++;
        }
        else if (!strcmp(argv[i],"-gainMod")) /* Optional */
        {
            loudnessNormalizationGainModificationDb = (float)atof(argv[i+1]);
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
        else if (!strcmp(argv[i],"-info")) /* Optional */
        {
            plotInfo = atoi(argv[i+1]);
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
    
    if ( ((inFilename == NULL) || (outFilename == NULL) || (!(defaultModePresent && !directGainModePresent && !txtGainModePresent && !selProcDataModePresent) && !(!defaultModePresent && directGainModePresent && !txtGainModePresent && !selProcDataModePresent) && !(!defaultModePresent && !directGainModePresent && txtGainModePresent && !selProcDataModePresent) && !(!defaultModePresent && !directGainModePresent && !txtGainModePresent && selProcDataModePresent))) || helpFlag )
    {
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "loudnessNormalizerCmdl Usage:\n\n");
        fprintf(stderr, "Example (default):              <loudnessNormalizerCmdl -if in.wav -of out.wav -CL -31.0 -TL -16.0 -info 0>\n");
        fprintf(stderr, "Example (direct gain):          <loudnessNormalizerCmdl -if in.wav -of out.wav -directGain -5.0 -info 0 -bitdepth 32>\n");
        fprintf(stderr, "Example (direct gain by txt):   <loudnessNormalizerCmdl -if in.wav -of out.wav -txtGain gain.txt -info 1>\n\n");
        
        fprintf(stderr, "-if           <input.wav>    Time domain input.\n");
        fprintf(stderr, "-of           <output.wav>   Time domain output.\n");
        
        fprintf(stderr, "-CL           <CL>           content loudness in [dBFS] (default mode, see description in ISO/IEC 23003-4).\n");
        fprintf(stderr, "-TL           <TL>           target loudness in [dBFS] (default mode, see description in ISO/IEC 23003-4).\n");
        
        fprintf(stderr, "-directGain   <gain>         loudness normalization gain in dB provided as cmdl parameter.\n");
        fprintf(stderr, "-txtGain      <gain.txt>     loudness normalization gain in dB provided by a txt file (first row, first column).\n");
        fprintf(stderr, "-selProcData  <selProc.txt>  loudness normalization gain from selection process transfer data file (uniDrcSelectionProcessCmdl).\n");
        fprintf(stderr, "-bitdepth                    specify bit depth of output file (otherwise input bitdepth is used).\n");
        fprintf(stderr, "-gainMod      <gain>         loudnessNormalizationGainModificationDb to modify the default loudness normalization gain.\n");
        
        fprintf(stderr, "-info         <info>         info type (0: nearly silent, 1: print minimal information, 2: print all information).\n");
        return -1;
    }
    
    /***********************************************************************************/
    /* ----------------- NORM GAIN -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark NORM_GAIN
#endif
    
    if(defaultModePresent == 1){
        loudnessNormalizationGainDb = TL-CL;
        printf("\nLoudnessNormalizer Cmdl Info: default gain mode\n");
    } else if (directGainModePresent == 1) {
        loudnessNormalizationGainDb = directGain;
        printf("\nLoudnessNormalizer Cmdl Info: direct gain mode. \n");
    } else if (txtGainModePresent == 1) {
        printf("\nLoudnessNormalizer Cmdl Info: txt gain mode.\n");
        errorFlag = readLoudnessNormalizationGainFromFile( inFilenameTxt, &txtGain );
        loudnessNormalizationGainDb = txtGain;
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during readLoudnessNormalizationGainFromFile.\n");
            return -1;
        }
    } else if (selProcDataModePresent == 1) {
        printf("\nLoudnessNormalizer Cmdl Info: selection process transfer data mode.\n");
        errorFlag = readSelProcOutFromFile(inFilenameTxt, &selProcGain, &truePeak, selectedDrcSetIds, selectedDownmixIds, &numSelectedDrcSets, &boost, &compress, &drcCharacteristicTarget, &baseChannelCount, &targetChannelCount);
        loudnessNormalizationGainDb = selProcGain;
        if ( 0 != errorFlag )
        {
            fprintf(stderr, "Error during readSelProcOutFromFile.\n");
            return -1;
        }    }
    loudnessNormalizationGainDb += loudnessNormalizationGainModificationDb;
    loudnessNormalizationGain = (float)pow(10.0,loudnessNormalizationGainDb/20.0);
    printf("\nLoudnessNormalizer Cmdl Info: NormalizationGain = %2.4f [dB] / %2.4f [lin].\n\n", loudnessNormalizationGainDb, loudnessNormalizationGain);

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
        fprintf(stderr, "Found input file: %s.\n", inFilename );
    }   
    
    /* Open output file/-s */
    
    if (outFilename) {
        pOutFile = fopen(outFilename, "wb");
    }
    if ( pOutFile == NULL ) {
        fprintf(stderr, "Could not open output file: %s.\n", outFilename );
        return -1;
    } else {
        fprintf(stderr, "Found output file: %s.\n", outFilename );
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
    
    /* bytedepth output */
    if (bytedepth_output == 0) {
        bytedepth_output = bytedepth_input;
    }
    
    printf("\nLoudnessNormalizer Cmdl Info: bitdepth_input = %d, bitdepth_output = %d.\n", 8*bytedepth_input,8*bytedepth_output);
    
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
    
    /* alloc local buffers */
    inBuffer = (float**)calloc(numChans,sizeof(float*));    
    outBuffer = (float**)calloc(numChans,sizeof(float*));  

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
   
    /* timing */
    accuTime = (clock_t)0.f;
    
    fprintf(stderr,"\nLoudnessNormalizer processing ... \n\n");
    
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
            
        /* start lap timer */
        startLap = clock();
        
        /* normalize */
        for (i=0; i<frameSize; i++) {
            for (j=0; j<numChans; j++) {
                outBuffer[j][i] = loudnessNormalizationGain*inBuffer[j][i];
            }            
        }   
        
        /* stop lap timer and accumulate */
        stopLap = clock();
        accuTime += stopLap - startLap;         
        
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
    
    fprintf(stderr,"LoudnessNormalizer finished. \n");

    /* processing time */
    elapsed_time = ( (double) accuTime ) / CLOCKS_PER_SEC;
    inputDuration = (float) NumberSamplesWritten/samplingRate;

    ratio = (float) (elapsed_time) / inputDuration;
    
    if (plotInfo > 0) {        
        
        fprintf(stderr,"\nProcessed %d frames ( %.2f s ) in %.2f s.\n", frameNo, inputDuration, (float) (elapsed_time) );
        fprintf(stderr,"Processing duration / Input duration = %4.2f %% .\n", (ratio * 100) ); 
        
    }
    
    /***********************************************************************************/
    /* ----------------- EXIT WAV IO ----------------- */
    /***********************************************************************************/

#ifdef __APPLE__
#pragma mark EXIT
#endif
    
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
    for (i=0; i < (unsigned int) numChans; i++) {
        free(inBuffer[i]);
        free(outBuffer[i]);
    }
    free(inBuffer);
    free(outBuffer);
    
    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int readLoudnessNormalizationGainFromFile( char* filename, float *txtGain)
{
    
    FILE* fileHandle;
    char line[512] = {0};
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle ) {
        fprintf(stderr,"Unable to open gain input file: %s\n",filename);
        return -1;
    } else {
        fprintf(stderr, "\nFound gain input file: %s\n", filename );
    }
    
    /* Get new line */
    fprintf(stderr, "Reading loudness normalization gain from file ... \n");
    if ( fgets(line, 511, fileHandle) != NULL )
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
        if ( (*pChar) != '\0')
        {
            while (  (*pChar) == '\n' || (*pChar) == '\r'  )
                pChar++;

            txtGain[0] = (float) atof(pChar);
        }
    }
    
    /* print values */
    fprintf(stderr,"\ntxtGain [dB] = ");
    fprintf(stderr,"%2.4f ",txtGain[0]);
    fprintf(stderr,"\n");
    
    fclose(fileHandle);
    return 0;
    
}

/***********************************************************************************/

static int readSelProcOutFromFile( char* filename, float *loudnessNorm_dB, float* truePeak, int* selectedDrcSetIds, int* selectedDownmixIds, int* numSelectedDrcSets, float* boost, float* compress, int* drcCharacteristicTarget, int* baseChannelCount, int* targetChannelCount)
{
    FILE* fileHandle;
    int i, err = 0;
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open selection process transfer file\n");
        return -1;
    } else {
        fprintf(stderr, "Found selection process transfer file: %s.\n", filename );
    }
    
    /* selected DRC sets*/
    err = fscanf(fileHandle, "%d\n", numSelectedDrcSets);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    for (i=0; i<(*numSelectedDrcSets); i++)
    {
        err = fscanf(fileHandle, "%d %d\n", &selectedDrcSetIds[i], &selectedDownmixIds[i]);
        if (err == 0)
        {
            fprintf(stderr,"Selection process transfer file has wrong format\n");
            return 1;
        }
    }
    
    /* loudness normalization gain */
    err = fscanf(fileHandle, "%f\n", loudnessNorm_dB);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    /* peakValue */
    err = fscanf(fileHandle, "%f\n", truePeak);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    /* decoder user parameters: boost, compress, drcCharacteristicTarget */
    err = fscanf(fileHandle, "%f %f %d\n", boost, compress, drcCharacteristicTarget);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    /* baseChannelCount and targetChannelCount */
    err = fscanf(fileHandle, "%d %d\n", baseChannelCount, targetChannelCount);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    fclose(fileHandle);
    return 0;
}

/***********************************************************************************/
