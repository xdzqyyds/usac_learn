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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "uniDrcGainDecoder_api.h"
#include "uniDrcCommon.h"
#include "wavIO.h"
#include "uniDrcBitstreamDecoder_api.h"

#define PROC_COMPLETE          1
#define GAINS_ONLY_IN_DB       0

typedef enum _GAIN_DEC_TYPE
{
    GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION = 0,
    GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION = 1,
    GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION = 2,
    GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION = 3,
    GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION = 4,
    GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION = 5
} GAIN_DEC_TYPE;

typedef enum _BITSTREAM_FILE_FORMAT
{
    BITSTREAM_FILE_FORMAT_DEFAULT = 0,
    BITSTREAM_FILE_FORMAT_SPLIT   = 1
} BITSTREAM_FILE_FORMAT;

typedef struct
{
    char wavFilenameInput[FILENAME_MAX];
    char wavFilenameInputReal[FILENAME_MAX];
    char wavFilenameInputImag[FILENAME_MAX];
    char wavFilenameOutput[FILENAME_MAX];
    char wavFilenameOutputImag[FILENAME_MAX];
    char wavFilenameOutputReal[FILENAME_MAX];
    char bitstreamFilenameInput[FILENAME_MAX];
    char selProcTransDataFilenameInput[FILENAME_MAX];
    BITSTREAM_FILE_FORMAT bitstreamFileFormat;
    GAIN_DEC_TYPE gainDecType;
    unsigned int audioFrameSize;
    unsigned int audioChannelCount;
    unsigned int audioSampleRate;
    int gainDelaySamples;
#if AMD1_SYNTAX
    int audioDelaySamples;
#endif
    int subBandDomainMode;
    int subBandDownSamplingFactor;
    int subBandCount;
    int delayMode;
    int decDownmixId;
    int numDecoderSubbands;
    float loudnessNormalizationGainModificationDb;
    int compensateDelay;
    char configBitstreamFilenameInput[FILENAME_MAX];
    char loudnessBitstreamFilenameInput[FILENAME_MAX];
    char gainBitstreamFilenameInput[FILENAME_MAX];
#if MPEG_H_SYNTAX
    char downmixMatrixSetParametersFilenameInput[FILENAME_MAX];
    int  channelOffset;
    int  numChannelsProcess;
#endif
} ConfigDRCDecoder;

/* forward declaration of prototypes */
static int get_config(int argc, char* argv[], ConfigDRCDecoder* config);
static int readSelProcOutFromFile( char* , float*, float* , int* , int* , int*, float*, float*, int*, int*, int*
#if MPEG_D_DRC_EXTENSION_V1
                                  , int*, float*, int*);
#else
                                    );
#endif
static int downmixIdMatches(int downmixId, int decDownmixId);
#if MPEG_H_SYNTAX
static int readDownmixMatrixSetFromFile( char* filename, int* downmixId, int* targetChannelCountFromDownmixId, int* downmixIdCount);
#endif

int main(int argc, char *argv[])
{
    int error = 0;
    int plotInfo = 1;
    int i,k, baseChannelCount, targetChannelCount, numSelectedDrcSets, numMatchingDrcSets, audioChannelCountInternal;
    int selectedDrcSetIds[3], matchingDrcSetIds[3], selectedDownmixIds[3], matchingDownmixIds[3];
    float loudnessNormalizationGainDb, truePeak;
    float **audioIOBuffer = NULL;
    float **audioIOBufferReal = NULL, **audioIOBufferImag = NULL;
    unsigned long frame;
    float boost = 1.0f;
    float compress = 1.0f;
    int drcCharacteristicTarget = 0;
    unsigned char* bitstream  = NULL;
#if AMD1_SYNTAX
    int audioDelaySamples = -1;
#endif /* AMD1_SYNTAX */
#if MPEG_D_DRC_EXTENSION_V1
    int selectedLoudEqId;
    float mixingLevel;
    int selectedEqSetIds[2];
    int matchingEqSetId;
#endif
    unsigned char* bitstreamConfig    = NULL;
    unsigned char* bitstreamLoudness  = NULL;
    int nBitsReadBsConfig       = 0;
    int nBytesBsConfig          = 0;
    int nBitsReadBsLoudness     = 0;
    int nBytesBsLoudness        = 0;
#if MPEG_H_SYNTAX
    int downmixIdCount          = 0;
    int downmixId[128]          = {0};
    int targetChannelCountFromDownmixId[128] = {0};
#endif
    int nBitsReadBs       = 0;
    int nBytesReadBs      = 0;
    int nBytesBs          = 0;
    int nBitsOffsetBs     = 0;
    int byteIndexBs       = 0;
    
    HANDLE_UNI_DRC_GAIN_DEC_STRUCTS hUniDrcGainDecStructs     = NULL;
    HANDLE_UNI_DRC_BS_DEC_STRUCT hUniDrcBsDecStruct           = NULL;
    HANDLE_UNI_DRC_CONFIG hUniDrcConfig                       = NULL;
    HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet                 = NULL;
    HANDLE_UNI_DRC_GAIN   hUniDrcGain                         = NULL;
#if MPEG_D_DRC_EXTENSION_V1
    HANDLE_LOUDNESS_EQ_STRUCT hLoudnessEq                     = NULL;
#endif
    WAVIO_HANDLE hWavIO                                       = NULL;
    WAVIO_HANDLE hWavIOReal                                   = NULL;
    WAVIO_HANDLE hWavIOImag                                   = NULL;
    
    FILE *pInFile                 = NULL;
    FILE *pOutFile                = NULL;
    FILE *pInFileReal             = NULL;
    FILE *pInFileImag             = NULL;
    FILE *pOutFileReal            = NULL;
    FILE *pOutFileImag            = NULL;
    FILE *pBsFile                 = NULL;
    
    int bFillLastFrame            = 1;
    int numSamplesToCompensate    = 0;
    int nSamplesPerChannelFilled  = 0;
    int nSamplesPerChannelFilledReal = 0;
    int nSamplesPerChannelFilledImag = 0;
    unsigned int bytedepth_input  = 0;
#if AMD2_COR3
    unsigned int bytedepth_output = 3;
#else
    unsigned int bytedepth_output = 0;
#endif
    unsigned long nTotalSamplesPerChannel;
    unsigned long nTotalSamplesPerChannelReal;
    unsigned long nTotalSamplesPerChannelImag;
    
    unsigned int audioComplete = 0, bitstreamComplete = 0;
    unsigned int nSamplesReadPerChannel     = 0;
    unsigned int nZerosPaddedBeginning = 0, nZerosPaddedEnd = 0;
    unsigned int nSamplesToWritePerChannel  = 0;
    unsigned int nSamplesWrittenPerChannel  = 0;
    unsigned int nSamplesReadPerChannelImag     = 0;
    unsigned int nZerosPaddedBeginningImag = 0, nZerosPaddedEndImag = 0;
    unsigned int nSamplesToWritePerChannelImag  = 0;
    unsigned int nSamplesWrittenPerChannelImag  = 0;
    unsigned long int nTotalSamplesWrittenPerChannel = 0;
    
    ConfigDRCDecoder config;
    
    for ( i = 1; i < argc; ++i )
    {
        if (!strcmp(argv[i], "-v"))
        {
            plotInfo = atoi(argv[i+1]);
            i++;
        }
    }
    
    if (plotInfo!=0) {
        fprintf( stdout, "\n");
        fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*                      Dynamic Range Control Gain Decoder                     *\n");
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*                                %s                                  *\n", __DATE__);
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*    This software may only be used in the development of the MPEG-D Part 4:  *\n");
        fprintf( stdout, "*    Dynamic Range Control standard, ISO/IEC 23003-4 or in conforming         *\n");
        fprintf( stdout, "*    implementations or products.                                             *\n");
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*******************************************************************************\n");
        fprintf( stdout, "\n");
    } else {
#if !MPEG_H_SYNTAX
#if !AMD1_SYNTAX
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Gain Decoder (RM11).\n\n");
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Gain Decoder (RM11).\n\n");
#endif
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Gain Decoder (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
#endif
    }

    /* parse command line parameters */
    error = get_config(argc, argv, &config);
    if (error) return -1;
    
    /* open functions */
    error = drcDecOpen(&hUniDrcGainDecStructs, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (error) return -1;
    
    error = openUniDrcBitstreamDec(&hUniDrcBsDecStruct, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (error) return -1;
    
    /* input file parsing */
    /* parse selection process transfer data file */
    error = readSelProcOutFromFile(config.selProcTransDataFilenameInput, &loudnessNormalizationGainDb, &truePeak, selectedDrcSetIds, selectedDownmixIds, &numSelectedDrcSets, &boost, &compress, &drcCharacteristicTarget, &baseChannelCount, &targetChannelCount
#if MPEG_D_DRC_EXTENSION_V1
                                   , &selectedLoudEqId, &mixingLevel, selectedEqSetIds);
#else
                                   );
#endif
    if (error) return -1;
    
#if MPEG_H_SYNTAX
    if (config.downmixMatrixSetParametersFilenameInput != NULL) {
        error = readDownmixMatrixSetFromFile(config.downmixMatrixSetParametersFilenameInput, downmixId, targetChannelCountFromDownmixId, &downmixIdCount);
   	if (error) return -1;
    }
#endif
    
    /* add loudnessNormalizationGainModificationDb to loudnessNormalizationGainDb to have it implicitly */
    loudnessNormalizationGainDb += config.loudnessNormalizationGainModificationDb;
    
    /**********************************/
    /* OPEN                           */
    /**********************************/
    
    /* wav file open */
    if (config.gainDecType == GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION) /* time domain */
    {
        /* Open input file/-s */
        if (config.wavFilenameInput) {
            pInFile  = fopen(config.wavFilenameInput, "rb");
        }
        if ( pInFile == NULL ) {
            fprintf(stderr, "Could not open input file: %s.\n", config.wavFilenameInput );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found input file: %s.\n", config.wavFilenameInput );
            }
        }
        
        /* Open output file/-s */
        
        if (config.wavFilenameOutput) {
            pOutFile = fopen(config.wavFilenameOutput, "wb");
        }
        if ( pOutFile == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutput );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutput );
            }
        }
        
        /* wavIO_init */
        error = wavIO_init(&hWavIO,
                           config.audioFrameSize,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
        /* wavIO_openRead */
        error = wavIO_openRead(hWavIO,
                               pInFile,
                               &config.audioChannelCount,
                               &config.audioSampleRate,
                               &bytedepth_input,
                               &nTotalSamplesPerChannel,
                               &nSamplesPerChannelFilled);
        if (error) goto Exit;
        
        /* bytedepth output */
#if AMD2_COR3
        if (bytedepth_output == 0 || bytedepth_input == 4) {
#else
        if (bytedepth_output == 0) {
#endif
            bytedepth_output = bytedepth_input;
        }
        
        /* wavIO_openWrite */
        error = wavIO_openWrite(hWavIO,
                                pOutFile,
                                config.audioChannelCount,
                                config.audioSampleRate,
                                bytedepth_output);
        if (error) goto Exit;
        
        /* alloc local buffers */
        audioIOBuffer = (float**)calloc(config.audioChannelCount,sizeof(float*));
        
        for (i=0; i< config.audioChannelCount; i++)
        {
            audioIOBuffer[i] = (float*)calloc(config.audioFrameSize,sizeof(float));
        }
    }
    
    else if (config.gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || config.gainDecType == GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) /* frequency domain */
    {
        /* Open input file/-s */
        if (config.wavFilenameInputReal) {
            pInFileReal  = fopen(config.wavFilenameInputReal, "rb");
        }
        if ( pInFileReal == NULL ) {
            fprintf(stderr, "Could not open input file: %s.\n", config.wavFilenameInputReal );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found input file: %s.\n", config.wavFilenameInputReal );
            }
        }
        
        if (config.wavFilenameInputImag) {
            pInFileImag  = fopen(config.wavFilenameInputImag, "rb");
        }
        if ( pInFileImag == NULL ) {
            fprintf(stderr, "Could not open input file: %s.\n", config.wavFilenameInputImag );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found input file: %s.\n", config.wavFilenameInputImag );
            }
        }
        
        /* Open output file/-s */
        if (config.wavFilenameOutputReal) {
            pOutFileReal = fopen(config.wavFilenameOutputReal, "wb");
        }
        if ( pOutFileReal == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutputReal );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutputReal );
            }
        }
        
        if (config.wavFilenameOutputImag) {
            pOutFileImag = fopen(config.wavFilenameOutputImag, "wb");
        }
        if ( pOutFileImag == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutputImag );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutputImag );
            }
        }
        
        /* wavIO_init */
        error = wavIO_init(&hWavIOReal,
                           config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
        /* wavIO_init */
        error = wavIO_init(&hWavIOImag,
                           config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
        /* wavIO_openRead */
        error = wavIO_openRead(hWavIOReal,
                               pInFileReal,
                               &config.audioChannelCount,
                               &config.audioSampleRate,
                               &bytedepth_input,
                               &nTotalSamplesPerChannelReal,
                               &nSamplesPerChannelFilledReal);
        if (error) goto Exit;
        
        error = wavIO_openRead(hWavIOImag,
                               pInFileImag,
                               &config.audioChannelCount,
                               &config.audioSampleRate,
                               &bytedepth_input,
                               &nTotalSamplesPerChannelImag,
                               &nSamplesPerChannelFilledImag);
        if (error) goto Exit;
        
        /* bytedepth output */
#if AMD2_COR3
        if (bytedepth_output == 0 || bytedepth_input == 4) {
#else
        if (bytedepth_output == 0) {
#endif
            bytedepth_output = bytedepth_input;
        }
        
        /* wavIO_openWrite */
        error = wavIO_openWrite(hWavIOReal,
                                pOutFileReal,
                                config.audioChannelCount,
                                config.audioSampleRate,
                                bytedepth_output);
        if (error) goto Exit;
        
        error = wavIO_openWrite(hWavIOImag,
                                pOutFileImag,
                                config.audioChannelCount,
                                config.audioSampleRate,
                                bytedepth_output);
        if (error) goto Exit;
        
        /* alloc local buffers */
        audioIOBufferReal  = (float**)calloc(config.audioChannelCount,sizeof(float*));
        audioIOBufferImag  = (float**)calloc(config.audioChannelCount,sizeof(float*));
        
        for (i=0; i< config.audioChannelCount; i++)
        {
            audioIOBufferReal[i] = (float*)calloc(config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,sizeof(float));
            audioIOBufferImag[i] = (float*)calloc(config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,sizeof(float));
        }
    }
    else if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION)
    {
        
        /* Open output file/-s */
        
        if (config.wavFilenameOutput) {
            pOutFile = fopen(config.wavFilenameOutput, "wb");
        }
        if ( pOutFile == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutput );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutput );
            }
        }
        
        /* wavIO_init */
        error = wavIO_init(&hWavIO,
                           config.audioFrameSize,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
    }
    else if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION || config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION)
    {
        
        /* Open output file/-s */
        if (config.wavFilenameOutputReal) {
            pOutFileReal = fopen(config.wavFilenameOutputReal, "wb");
        }
        if ( pOutFileReal == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutputReal );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutputReal );
            }
        }
        
        if (config.wavFilenameOutputImag) {
            pOutFileImag = fopen(config.wavFilenameOutputImag, "wb");
        }
        if ( pOutFileImag == NULL ) {
            fprintf(stderr, "Could not open output file: %s.\n", config.wavFilenameOutputImag );
            return -1;
        } else {
            if (plotInfo!=0) {
                fprintf(stderr, "Found output file: %s.\n", config.wavFilenameOutputImag );
            }
        }
        
        /* wavIO_init */
        error = wavIO_init(&hWavIOReal,
                           config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
        /* wavIO_init */
        error = wavIO_init(&hWavIOImag,
                           config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,
                           bFillLastFrame,
                           numSamplesToCompensate);
        if (error) goto Exit;
        
    }
    
    /* gain decoder init has to be called after decoding drc config! */
    error = drcDecInit(config.audioFrameSize,
                       config.audioSampleRate,
                       config.gainDelaySamples,
                       config.delayMode,
                       config.subBandDomainMode,
                       hUniDrcGainDecStructs);
    if (error) goto Exit;
    
    /* Init BS decoder */
    error = initUniDrcBitstreamDec( hUniDrcBsDecStruct, config.audioSampleRate, config.audioFrameSize, config.delayMode, -1, NULL);
    if (error) goto Exit;
    
    /* open and read bitstream file(s) */
    if (config.bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
        pBsFile = fopen ( config.configBitstreamFilenameInput , "rb" );
        if (pBsFile == NULL) {
            fprintf(stderr, "Error opening input file %s\n", config.configBitstreamFilenameInput);
            return -1;
        }
        
        fseek( pBsFile , 0L , SEEK_END);
        nBytesBsConfig = (int)ftell( pBsFile );
        rewind( pBsFile );
        
        bitstreamConfig = calloc( 1, nBytesBsConfig );
        if( !bitstreamConfig ) {
            fclose(pBsFile);
            fputs("memory alloc fails",stderr);
            exit(1);
        }
        
        /* copy the file into the buffer */
        if( 1!=fread( bitstreamConfig , nBytesBsConfig, 1 , pBsFile) ) {
            fclose(pBsFile);
            free(bitstreamConfig);
            fputs("entire read fails",stderr);
            exit(1);
        }
        fclose(pBsFile);
        
        pBsFile = fopen ( config.loudnessBitstreamFilenameInput , "rb" );
        if (pBsFile == NULL) {
            fprintf(stderr, "Error opening input file %s\n", config.loudnessBitstreamFilenameInput);
            return -1;
        }
        
        fseek( pBsFile , 0L , SEEK_END);
        nBytesBsLoudness = (int)ftell( pBsFile );
        rewind( pBsFile );
        
        bitstreamLoudness = calloc( 1, nBytesBsLoudness );
        if( !bitstreamLoudness ) {
            fclose(pBsFile);
            fputs("memory alloc fails",stderr);
            exit(1);
        }
        
        /* copy the file into the buffer */
        if( 1!=fread( bitstreamLoudness , nBytesBsLoudness, 1 , pBsFile) ) {
            fclose(pBsFile);
            free(bitstreamLoudness);
            fputs("entire read fails",stderr);
            exit(1);
        }
        fclose(pBsFile);
        
        pBsFile = fopen ( config.gainBitstreamFilenameInput , "rb" );
        if (pBsFile == NULL) {
            fprintf(stderr, "Error opening input file %s\n", config.gainBitstreamFilenameInput);
            return -1;
        }
    } else {
        pBsFile = fopen ( config.bitstreamFilenameInput , "rb" );
        if (pBsFile == NULL) {
            fprintf(stderr, "Error opening input file %s\n", config.bitstreamFilenameInput);
            return -1;
        }
    }
    fseek( pBsFile , 0L , SEEK_END);
    nBytesBs = (int)ftell( pBsFile );
    rewind( pBsFile );
    
    bitstream = calloc( 1, nBytesBs );
    if( !bitstream ) {
        fclose(pBsFile);
        fputs("memory alloc fails",stderr);
        exit(1);
    }
    
    /* copy the file into the buffer */
    if( 1!=fread( bitstream , nBytesBs, 1 , pBsFile) ) {
        fclose(pBsFile);
        free(bitstream);
        fputs("entire read fails",stderr);
        exit(1);
    }
    fclose(pBsFile);
    
    /* compare selectedDownmixIds with decDownmixId */
    numMatchingDrcSets = 0;
    for (i = 0; i < numSelectedDrcSets; i++)
    {
        if (downmixIdMatches(selectedDownmixIds[i], config.decDownmixId)) {
            matchingDrcSetIds[numMatchingDrcSets] = selectedDrcSetIds[i];
            matchingDownmixIds[numMatchingDrcSets] = selectedDownmixIds[i];
            numMatchingDrcSets++;
        }
    }
    
    if(numMatchingDrcSets == 0 && plotInfo!=0) {
        printf("\nNo Drc set selected/available for the chosen decoder downmixID\n");
    }
    
    /**********************************/
    /* DECODE_LOOP                    */
    /**********************************/
    
    /* do decoding and application of gain over whole file */
    frame = 0;
    while((error == 0) && (audioComplete == 0) && (bitstreamComplete == 0)){
        /* read bitstream frame */
        
        /**********************************/
        /* FIRST FRAME                    */
        /**********************************/
        
        if (config.bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            if (frame == 0)
            {
#if MPEG_H_SYNTAX
                error = setMpeghDownmixMatrixSetParamsUniDrcSelectionProcess(hUniDrcConfig, downmixIdCount, downmixId, targetChannelCountFromDownmixId);
                if (error) goto Exit;
#endif
                
                error = processUniDrcBitstreamDec_uniDrcConfig(hUniDrcBsDecStruct,
                                                               hUniDrcConfig,
                                                               hLoudnessInfoSet,
                                                               bitstreamConfig,
                                                               nBytesBsConfig,
                                                               0,
                                                               &nBitsReadBsConfig);
                if (error > PROC_COMPLETE) goto Exit;
                
                error = processUniDrcBitstreamDec_loudnessInfoSet(hUniDrcBsDecStruct,
                                                                  hLoudnessInfoSet,
                                                                  hUniDrcConfig,
                                                                  bitstreamLoudness,
                                                                  nBytesBsLoudness,
                                                                  0,
                                                                  &nBitsReadBsLoudness);
                if (error > PROC_COMPLETE) goto Exit;
            }
        } else {
            error = processUniDrcBitstreamDec_uniDrc(hUniDrcBsDecStruct,
                                                     hUniDrcConfig,
                                                     hLoudnessInfoSet,
                                                     &bitstream[byteIndexBs],
                                                     nBytesBs,
                                                     nBitsOffsetBs,
                                                     &nBitsReadBs);
            if (error > PROC_COMPLETE) goto Exit;
            
            nBytesReadBs  = nBitsReadBs/8;
            nBitsOffsetBs = nBitsReadBs - nBytesReadBs*8;
            byteIndexBs   += nBytesReadBs;
            nBytesBs      -= nBytesReadBs;
        }
    
        /* do init of variables dependent on the config data */
        if (frame == 0)
        {
            audioChannelCountInternal = config.audioChannelCount;
#if MPEG_H_SYNTAX
            if (numMatchingDrcSets && matchingDownmixIds[0] == ID_FOR_BASE_LAYOUT) { /* it is sufficient to look into the first donwmixId at this location */
                audioChannelCountInternal = baseChannelCount;
            }
#endif
#if MPEG_D_DRC_EXTENSION_V1
            if (numMatchingDrcSets && matchingDownmixIds[0] == ID_FOR_BASE_LAYOUT) { /* it is sufficient to look into the first donwmixId at this location */
                matchingEqSetId = selectedEqSetIds[0];
            } else {
                matchingEqSetId = selectedEqSetIds[1];
            }
#endif
            error = drcDecInitAfterConfig(audioChannelCountInternal,
                                          matchingDrcSetIds,
                                          matchingDownmixIds,
                                          numMatchingDrcSets,
#if MPEG_D_DRC_EXTENSION_V1
                                          matchingEqSetId,
#endif
#if MPEG_H_SYNTAX
                                          config.channelOffset,
                                          config.numChannelsProcess,
#endif
                                          hUniDrcGainDecStructs,
                                          hUniDrcConfig
#if AMD1_SYNTAX
                                          , hLoudnessInfoSet
#endif
                                          );
            if (error) goto Exit;
            
            if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION)
            {
                bytedepth_output = 4;  /* 32 bit */
                /* wavIO_openWrite */
                error = wavIO_openWrite(hWavIO,
                                        pOutFile,
                                        config.audioChannelCount,
                                        config.audioSampleRate,
                                        bytedepth_output);
                if (error) goto Exit;
                
                /* alloc local buffers */
                audioIOBuffer = (float**)calloc(config.audioChannelCount,sizeof(float*));
                
                for (i=0; i< config.audioChannelCount; i++)
                {
                    audioIOBuffer[i] = (float*)calloc(config.audioFrameSize,sizeof(float));
                }
            }
            else if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION ||config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION)
            {
                bytedepth_output = 4;  /* 32 bit */
                
                /* wavIO_openWrite */
                error = wavIO_openWrite(hWavIOReal,
                                        pOutFileReal,
                                        config.audioChannelCount,
                                        config.audioSampleRate,
                                        bytedepth_output);
                if (error) goto Exit;
                
                error = wavIO_openWrite(hWavIOImag,
                                        pOutFileImag,
                                        config.audioChannelCount,
                                        config.audioSampleRate,
                                        bytedepth_output);
                if (error) goto Exit;
                
                /* alloc local buffers */
                audioIOBufferReal  = (float**)calloc(config.audioChannelCount,sizeof(float*));
                audioIOBufferImag  = (float**)calloc(config.audioChannelCount,sizeof(float*));
                
                for (i=0; i< config.audioChannelCount; i++)
                {
                    audioIOBufferReal[i] = (float*)calloc(config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,sizeof(float));
                    audioIOBufferImag[i] = (float*)calloc(config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount,sizeof(float));
                }
            }
        }
        
        error = processUniDrcBitstreamDec_uniDrcGain(hUniDrcBsDecStruct,
                                                     hUniDrcConfig,
                                                     hUniDrcGain,
                                                     &bitstream[byteIndexBs],
                                                     nBytesBs,
                                                     nBitsOffsetBs,
                                                     &nBitsReadBs);
        if (error > PROC_COMPLETE) goto Exit;
        if (error == PROC_COMPLETE) bitstreamComplete = 1;
        
        nBytesReadBs  = nBitsReadBs/8;
        nBitsOffsetBs = nBitsReadBs - nBytesReadBs*8;
        byteIndexBs   = byteIndexBs + nBytesReadBs;
        nBytesBs      = nBytesBs - nBytesReadBs;
        
        if (config.bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            /* shift over fill-bits for frame byte alignment */
            if (nBitsOffsetBs != 0)
            {
                nBitsReadBs   = nBitsReadBs + 8 - nBitsOffsetBs;
                nBytesReadBs  = nBytesReadBs + 1;
                nBitsOffsetBs = 0;
                byteIndexBs   = byteIndexBs + 1;
                nBytesBs      = nBytesBs - 1;
            }
        }
        
        /* DRC gain decoding */
        /* applies DRC in time domain */
        if (config.gainDecType == GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION)
        {
            /* read input audio frame */
            wavIO_readFrame(hWavIO, audioIOBuffer, &nSamplesReadPerChannel, &audioComplete, &nZerosPaddedBeginning, &nZerosPaddedEnd);
            
            error = drcProcessTime(hUniDrcGainDecStructs,
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer,
                                   loudnessNormalizationGainDb,
                                   boost,
                                   compress,
                                   drcCharacteristicTarget);
            if (error) goto Exit;
            
            /* write output audio frame */
            nSamplesToWritePerChannel = nSamplesReadPerChannel;
            wavIO_writeFrame(hWavIO, audioIOBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
        }
        /* applies DRC in frequency domain */
        else if (config.gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || config.gainDecType == GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION)
        {
            /* read input audio frame */
            wavIO_readFrame(hWavIOReal, audioIOBufferReal, &nSamplesReadPerChannel, &audioComplete, &nZerosPaddedBeginning, &nZerosPaddedEnd);
            wavIO_readFrame(hWavIOImag, audioIOBufferImag, &nSamplesReadPerChannelImag, &audioComplete, &nZerosPaddedBeginningImag, &nZerosPaddedEndImag);
            
            error = drcProcessFreq(hUniDrcGainDecStructs,
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBufferReal,
                                   audioIOBufferImag,
                                   loudnessNormalizationGainDb,
                                   boost,
                                   compress,
                                   drcCharacteristicTarget);
            if (error) goto Exit;
            
            /* write output audio frame */
            nSamplesToWritePerChannel     = nSamplesReadPerChannel;
            nSamplesToWritePerChannelImag = nSamplesReadPerChannelImag;
            wavIO_writeFrame(hWavIOReal, audioIOBufferReal, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
            wavIO_writeFrame(hWavIOImag, audioIOBufferImag, nSamplesToWritePerChannelImag, &nSamplesWrittenPerChannelImag);
        }
        /* returns DRC gains w/o applying them */
        else if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION)
        {
            for (i=0; i<config.audioChannelCount; i++) {
                for(k = 0; k < config.audioFrameSize; k++)
                {
                    audioIOBuffer[i][k] = 1.f;
                }
            }
            error = drcProcessTime(hUniDrcGainDecStructs,
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer,
                                   loudnessNormalizationGainDb,
                                   boost,
                                   compress,
                                   drcCharacteristicTarget);
#if GAINS_ONLY_IN_DB
            for (i=0; i<config.audioChannelCount; i++) {
                for(k = 0; k < config.audioFrameSize; k++)
                {
                    audioIOBuffer[i][k] = 20*(float)log10(audioIOBuffer[i][k]);
                }
            }
#endif
            if (error == ERRORHANDLING)
            {
                /* reset gain buffer to 1 for error handling */
                for(i = 0; i<config.audioChannelCount; i++)
                {
                    for(k = 0; k < config.audioFrameSize; k++)
                    {
                        audioIOBuffer[i][k] = 1.f;
                    }
                }
            }
            else if(error) goto Exit;
            
            /* write output gain frame */
            nSamplesToWritePerChannel = config.audioFrameSize;
            wavIO_writeFrame(hWavIO, audioIOBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
        }
        else if (config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION ||config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION)
        {
            for(i = 0; i<config.audioChannelCount; i++)
            {
                for (k = 0; k < config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount; k++) {
                    audioIOBufferReal[i][k] = 1.f;
                    audioIOBufferImag[i][k] = 1.f;
                }
            }
            error = drcProcessFreq(hUniDrcGainDecStructs,
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBufferReal,
                                   audioIOBufferImag,
                                   loudnessNormalizationGainDb,
                                   boost,
                                   compress,
                                   drcCharacteristicTarget);
#if GAINS_ONLY_IN_DB
            for (i=0; i<config.audioChannelCount; i++) {
                for(k = 0; k < config.audioFrameSize; k++)
                {
                    audioIOBufferReal[i][k] = 20*(float)log10(audioIOBufferReal[i][k]);
                    audioIOBufferImag[i][k] = 20*(float)log10(audioIOBufferImag[i][k]);
                }
            }
#endif
            if (error == ERRORHANDLING)
            {
                /* reset gain buffer to 1 for error handling */
                for(i = 0; i<config.audioChannelCount; i++)
                {
                    for (k = 0; k < config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount; k++) {
                        audioIOBufferReal[i][k] = 1.f;
                        audioIOBufferImag[i][k] = 1.f;
                    }
                }
            }
            else if(error) goto Exit;
            
            /* write output gain frame */
            nSamplesToWritePerChannel     = config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount;
            nSamplesToWritePerChannelImag = config.audioFrameSize/config.subBandDownSamplingFactor*config.subBandCount;
            wavIO_writeFrame(hWavIOReal, audioIOBufferReal, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
            wavIO_writeFrame(hWavIOImag, audioIOBufferImag, nSamplesToWritePerChannelImag, &nSamplesWrittenPerChannelImag);
        }
        
        if (error > 1) goto Exit;
        if (error == 1) /* equals PROCESSING_COMPLETE */
        {
            break; /* stop while loop when processing is completed */
        }
        
#if (!MPEG_H_SYNTAX)
        /*checkDrcConfigPresentFlag(hUniDrcGainDecStructs);*/
#endif
        
        if (error > 1) goto Exit;
        if ((error == 1) || (audioComplete == 1) || (bitstreamComplete == 1)) /* equals PROCESSING_COMPLETE; first condition if bitstream is at end, second if audio is at end */
        {
            break; /* stop while loop when processing is completed */
        }
        frame++;
    }
    
Exit:
    /**********************************/
    /* CLOSE                          */
    /**********************************/
    
    if (config.gainDecType == GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION || config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION)
    {
        error = wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
        error = wavIO_close(hWavIO);
        if (error != 0 )
        {
            fprintf(stderr, "Error during wavIO_close.\n");
            return -1;
        }
    }
    else if (config.gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || config.gainDecType == GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION ||
             config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION ||config.gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION)
    {
        error = wavIO_updateWavHeader(hWavIOReal, &nTotalSamplesWrittenPerChannel);
        error = wavIO_close(hWavIOReal);
        if (error != 0 )
        {
            fprintf(stderr, "Error during wavIO_close real.\n");
            return -1;
        }
        nTotalSamplesWrittenPerChannel = 0;
        error = wavIO_updateWavHeader(hWavIOImag, &nTotalSamplesWrittenPerChannel);
        error = wavIO_close(hWavIOImag);
        if (error != 0 )
        {
            fprintf(stderr, "Error during wavIO_close imag.\n");
            return -1;
        }
    }
    
    /* free allocated memory */
    if (bitstream != NULL)
    {
        free(bitstream);
        bitstream = NULL;
    }
    if (bitstreamConfig != NULL)
    {
        free(bitstreamConfig);
        bitstreamConfig = NULL;
    }
    if (bitstreamLoudness != NULL)
    {
        free(bitstreamLoudness);
        bitstreamLoudness = NULL;
    }
    if (audioIOBuffer != NULL)
    {
        for (i=0; i< config.audioChannelCount; i++)
        {
            free(audioIOBuffer[i]);
            audioIOBuffer[i] = NULL;
        }
        free(audioIOBuffer);
        audioIOBuffer = NULL;
    }
    
    if (audioIOBufferReal != NULL)
    {
        for (i=0; i< config.audioChannelCount; i++)
        {
            free(audioIOBufferReal[i]);
            audioIOBufferReal[i] = NULL;
        }
        free(audioIOBufferReal);
        audioIOBufferReal = NULL;
    }
    
    if (audioIOBufferImag != NULL)
    {
        for (i=0; i< config.audioChannelCount; i++)
        {
            free(audioIOBufferImag[i]);
            audioIOBufferImag[i] = NULL;
        }
        free(audioIOBufferImag);
        audioIOBufferImag = NULL;
    }
    
    error = drcDecClose(&hUniDrcGainDecStructs, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain
#if MPEG_D_DRC_EXTENSION_V1
                        , &hLoudnessEq
#endif
                        );
    if (error) return (error);
    
    error = closeUniDrcBitstreamDec(&hUniDrcBsDecStruct, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (error) return (error);
    
    return error;
}

static void PrintCmdlineHelp ( char* argv0 )
{
    fprintf ( stderr, "invalid arguments:\n" );
    fprintf ( stderr, "\nuniDrcGainDecoderCmdl Usage:\n\n" );
    fprintf ( stderr, "Example (TD interface): uniDrcGainDecoderCmdl -if <input.wav> -is <selProcData.txt> -ib <inBitstream.bs> -decId <idx> -of <output.wav>");
#if MPEG_H_SYNTAX
    fprintf ( stderr, " -dms <dms.txt>\n>");
#else
    fprintf ( stderr, "\n");
#endif
    fprintf ( stderr, "Example (TD interface, splitted payload): uniDrcGainDecoderCmdl -if <input.wav> -is <selProcData.txt> -ic <inBitstreamConfig.bs> -il <inBitstreamLoudness.bs> -ig <inBitstreamGain.bs> -decId <idx> -of <output.wav>");
#if MPEG_H_SYNTAX
    fprintf ( stderr, " -dms <dms.txt>\n>");
#else
    fprintf ( stderr, "\n");
#endif
    fprintf ( stderr, "Example (FD interface): uniDrcGainDecoderCmdl -if <input> -is <selProcData.txt> -ib <inBitstream.bs> -decId <idx> -gdt 1 -of <output>");
#if MPEG_H_SYNTAX
    fprintf ( stderr, " -dms <dms.txt>\n>");
#else
    fprintf ( stderr, "\n");
#endif
    fprintf ( stderr, "Example (Gains-only interface): uniDrcGainDecoderCmdl -is <selProcData.txt> -ib <inBitstream.bs> -decId <idx> -gdt 3 -of <output> -acc <int> -fs <int>\n");
#if MPEG_H_SYNTAX
    fprintf ( stderr, "Additional parameters: (-aco <int>) (-acp <int>) (-dm <idx>) (-gd <int>) (-dc)\n");
#else
#if AMD1_SYNTAX
    fprintf ( stderr, "Additional parameters: (-dm <idx>) (-gd <int>) (-ad <int>) (-dc)\n");
#else
    fprintf ( stderr, "Additional parameters: (-dm <idx>) (-gd <int>) (-dc)\n");
#endif
#endif
    fprintf ( stderr, "\n" );
    fprintf ( stderr, "-if\n" );
    fprintf ( stderr, "\t TD interface: Path to input wav\n" );
    fprintf ( stderr, "\t FD interface: Path to subband domain input\n" );
    fprintf ( stderr, "\t               Filename will be extended to <input>_real.qmf and <input>_imag.qmf or to <input>_real.stft and <input>_imag.stft dependent on -gdt option\n" );
    fprintf ( stderr, "-ib\n" );
    fprintf ( stderr, "\t Path to input uniDrc-bitstream\n" );
    fprintf ( stderr, "-ic\n" );
    fprintf ( stderr, "\t Path to input uniDrcConfig-bitstream\n" );
    fprintf ( stderr, "-il\n" );
    fprintf ( stderr, "\t Path to input loudnessInfoSet-bitstream\n" );
    fprintf ( stderr, "-ig\n" );
    fprintf ( stderr, "\t Path to input uniDrcGain-bitstream (byte alignment expected after each uniDrcGain() payload)\n" );
#if MPEG_H_SYNTAX
    fprintf ( stderr, "-dms\n" );
    fprintf ( stderr, "\t Path to input txt for downmixId related parameters provided by MPEG-H 3DA DownmixMatrixSet() syntax (downmixIdCount, downmixId, targetChannelCountFromDownmixId)\n" );
#endif
    fprintf ( stderr, "-is\n" );
    fprintf ( stderr, "\t Path to input selection process transfer data txt\n" );
    fprintf ( stderr, "-decId\n" );
#if AMD1_SYNTAX
    fprintf ( stderr, "\t DownmixId of decoder instance (0=baseLayout, 2=0x7F, 3=targetLayout without 0x7F, 4=targetLayout and 0x7F).\n" );
#else
    fprintf ( stderr, "\t DownmixId of decoder instance (0=baseLayout, 1=baseLayout and 0x7F, 2=0x7F, 3=targetLayout without 0x7F, 4=targetLayout and 0x7F).\n" );
#endif
    fprintf ( stderr, "-of\n" );
    fprintf ( stderr, "\t TD interface: Path to output wav\n" );
    fprintf ( stderr, "\t FD interface: Path to subband domain output\n" );
    fprintf ( stderr, "\t               Filename will be extended to <input>_real.qmf and <input>_imag.qmf or to <input>_real.stft and <input>_imag.stft dependent on -gdt option\n" );
    fprintf ( stderr, "-gdt\n" );
    fprintf ( stderr, "\t Index of gain decoding type (0 = TD interface, TD application; 1 = QMF64 interface, QMF64 application; 2 = STFT256 interface, STFT256 application; ...\n" );
    fprintf ( stderr, "\t                              3 = GAIN ONLY interface, TD application; 4 = GAIN ONLY interface, QMF64 application; 5 = GAIN ONLY interface, STFT256 application)\n" );
    fprintf ( stderr, "-acc\n" );
    fprintf ( stderr, "\t Audio channel count (for -gdt 3,4 and 5).\n" );
    fprintf ( stderr, "-afs\n" );
    fprintf ( stderr, "\t Audio frame size (default: 1024).\n" );
#if MPEG_H_SYNTAX
    fprintf ( stderr, "-acp\n" );
    fprintf ( stderr, "\t Audio channel count processed (<= audio channel count).\n" );
    fprintf ( stderr, "-aco\n" );
    fprintf ( stderr, "\t Audio channel count offset.\n" );
#endif
    fprintf ( stderr, "-fs\n" );
    fprintf ( stderr, "\t Audio sample rate (default: 48000 [Hz], for -gdt 3,4 and 5).\n" );
    fprintf ( stderr, "-gd\n" );
    fprintf ( stderr, "\t Gain delay in samples (default = 0).\n" );
#if AMD1_SYNTAX
    fprintf ( stderr, "-ad\n" );
    fprintf ( stderr, "\t Audio delay in samples (default=0). Note that this value also defines parametricDrcLookAheadMax.\n" );
#endif /* AMD1_SYNTAX */
    fprintf ( stderr, "-dm\n" );
    fprintf ( stderr, "\t Index of delay mode (0 = default delay mode; 1 = low-delay mode)\n" );
    fprintf ( stderr, "-dc\n" );
    fprintf ( stderr, "\t Flag: use processing delay compensation\n" );
    fprintf ( stderr, "-v\n" );
    fprintf ( stderr, "\t Index for verbose mode (0: short, 1: default, 2: print all)\n" );
}

static int get_config(int argc, char* argv[], ConfigDRCDecoder* config)
{
    int i = 0;
    int ret = 0;
    int required = 0;
    int nWavExtensionChars;
    
    /* default parameters */
    config->wavFilenameInput[0]  = '\0';
    config->wavFilenameOutput[0] = '\0';
    config->bitstreamFilenameInput[0] = '\0';
    config->selProcTransDataFilenameInput[0] = '\0';
    config->configBitstreamFilenameInput[0] = '\0';
    config->loudnessBitstreamFilenameInput[0] = '\0';
    config->gainBitstreamFilenameInput[0] = '\0';
#if MPEG_H_SYNTAX
    config->channelOffset      = -1;
    config->numChannelsProcess = -1;
#endif
    config->bitstreamFileFormat = BITSTREAM_FILE_FORMAT_DEFAULT;
    config->subBandDomainMode = 0;
    config->gainDecType = 0;
    config->audioFrameSize = 1024;
    config->numDecoderSubbands = -1;
    config->audioChannelCount = 0;
    config->audioSampleRate = 48000;
    config->loudnessNormalizationGainModificationDb = 0.f;
    config->gainDelaySamples = 0;
#if AMD1_SYNTAX
    config->audioDelaySamples = 0;
#endif
    config->delayMode = 0;
    config->compensateDelay = 0;
    config->decDownmixId = 0;
    config->subBandCount = 0;
    config->subBandDownSamplingFactor = 0;
    
    for ( i = 1; i < argc; ++i )
    {
        if (!strcmp(argv[i], "-if")) /* Optional */
        {
            strncpy(config->wavFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
        }
        else if (!strcmp(argv[i], "-of"))    /* required */
        {
            strncpy(config->wavFilenameOutput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-ib"))    /* required */
        {
            strncpy(config->bitstreamFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-is"))    /* required */
        {
            strncpy(config->selProcTransDataFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-gdt"))    /* optional */
        {
            int tmp = atoi(argv[i+1]);
            switch (tmp) {
                case 1:
                    config->gainDecType = GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_QMF64;
                    config->subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
                    config->subBandCount = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
                    break;
                case 2:
                    config->gainDecType = GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_STFT256;
                    config->subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
                    config->subBandCount = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
                    break;
                case 3:
                    config->gainDecType = GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_OFF;
                    break;
                case 4:
                    config->gainDecType = GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_QMF64;
                    config->subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
                    config->subBandCount = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
                    break;
                case 5:
                    config->gainDecType = GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_STFT256;
                    config->subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
                    config->subBandCount = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
                    break;
                case 0:
                default:
                    config->gainDecType = GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION;
                    config->subBandDomainMode = SUBBAND_DOMAIN_MODE_OFF;
                    break;
            }
            i++;
        }
        else if (!strcmp(argv[i], "-decId"))    /* required */
        {
            config->decDownmixId = atoi(argv[i+1]);
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-ic"))    /* required */
        {
            config->bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy(config->configBitstreamFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-il"))    /* required */
        {
            config->bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy(config->loudnessBitstreamFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
        else if (!strcmp(argv[i], "-ig"))    /* required */
        {
            config->bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy(config->gainBitstreamFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
#if MPEG_H_SYNTAX
        else if (!strcmp(argv[i], "-aco"))    /* Optional */
        {
            config->channelOffset = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-acp"))    /* Optional */
        {
            config->numChannelsProcess = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-dms"))      /* required */
        {
            strncpy(config->downmixMatrixSetParametersFilenameInput, argv[i+1], FILENAME_MAX) ;
            i++;
            required++;
        }
#endif
        else if (!strcmp(argv[i], "-acc"))    /* Optional */
        {
            config->audioChannelCount = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-afs"))    /* Optional */
        {
            config->audioFrameSize = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-fs"))    /* Optional */
        {
            config->audioSampleRate = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i], "-gd"))    /* Optional */
        {
            config->gainDelaySamples = atoi(argv[i+1]);
            i++;
        }
#if AMD1_SYNTAX
        else if (!strcmp(argv[i], "-ad"))    /* Optional */
        {
            config->audioDelaySamples = atoi(argv[i+1]);
            i++;
        }
#endif
        else if (!strcmp(argv[i], "-dm"))    /* Optional */
        {
            config->delayMode = atoi(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-dc")) /* Optional */
        {
            config->compensateDelay = 1;
            continue;
        }
        else if (!strcmp(argv[i], "-v"))    /* Optional */
        {
            i++;
        }
        else
        {
            ret = -1;
        }
    }
#if MPEG_H_SYNTAX
    if ( (config->bitstreamFileFormat != BITSTREAM_FILE_FORMAT_SPLIT) || (required != 7) )
#else
    if ( ((config->bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) && (required != 6)) || ((config->bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) && (required != 4)) )
#endif
    {
        PrintCmdlineHelp( argv[0] );
        return -1;
    }
    
    /* check parameter values */
    if (config->audioFrameSize == 0)
    {
        fprintf(stderr, "Missing or wrong command line parameter: audio frame size (-afs VALUE)\n");
        ret = -1;
    }
    if (config->bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
        if (config->configBitstreamFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: uniDrcConfig bitstream input filename (-ic FILE)\n");
            ret = -1;
        }
        if (config->loudnessBitstreamFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: loudnessInfoSet bitstream input filename (-il FILE)\n");
            ret = -1;
        }
        if (config->gainBitstreamFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: uniDrcGain bitstream input filename (-ig FILE)\n");
            ret = -1;
        }
    } else {
        if (config->bitstreamFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: bitstream input filename (-bs FILE)\n");
            ret = -1;
        }
    }
#if MPEG_H_SYNTAX
    if (config->channelOffset < -1)
    {
        fprintf(stderr, "Wrong command line parameter: channel offset (-co VALUE)\n");
        ret = -1;
    }
    if (config->numChannelsProcess < -1)
    {
        fprintf(stderr, "Wrong command line parameter: number of channels to be processed (-cn VALUE)\n");
        ret = -1;
    }
    if (((config->channelOffset > -1) && (config->numChannelsProcess < 0)) || ((config->channelOffset < 0) && (config->numChannelsProcess > -1)))
    {
        fprintf(stderr, "Missing command line parameter: -co and -cn can only be used in combination \n");
        ret = -1;
    }
    if (config->downmixMatrixSetParametersFilenameInput[0] == '\0')
    {
        fprintf(stderr, "Missing or wrong command line parameter: downmixMatrixSet() parameter input filename (-dms FILE)\n");
        ret = -1;
    }
#endif
    if (config->selProcTransDataFilenameInput[0] == '\0')
    {
        fprintf(stderr, "Missing or wrong command line parameter: selection process transfer data input filename (-is FILE)\n");
        ret = -1;
    }
#if AMD1_SYNTAX
    if ((config->decDownmixId < 0 || config->decDownmixId > 4) && config->decDownmixId != 1) /* 0x7F always after downmix in AMD1 */
#else
    if (config->decDownmixId < 0 || config->decDownmixId > 4)
#endif
    {
        fprintf(stderr, "Missing or wrong command line parameter: decoded downmixId (-decDownmixId VALUE)\n");
        ret = -1;
    }
    if (config->gainDecType == GAIN_DEC_TYPE_TD_INTERFACE_TD_APPLICATION)
    {
        if (config->wavFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav input filename (-if FILE)\n");
            ret = -1;
        }
        if (config->wavFilenameOutput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav output filename (-of FILE)\n");
            ret = -1;
        }
    }
    else if(config->gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || config->gainDecType == GAIN_DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION)
    {
        if (config->wavFilenameInput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav input filename (-if FILE)\n");
            ret = -1;
        }
        else
        {
            nWavExtensionChars = 0;
            nWavExtensionChars = (int)strspn(".wav",config->wavFilenameInput);
            if ( nWavExtensionChars != 4 ) {
                strcpy(config->wavFilenameInputReal, config->wavFilenameInput);
                strcpy(config->wavFilenameInputImag, config->wavFilenameInput);
                if (config->gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION) {
                    strcat(config->wavFilenameInputReal, "_real.qmf");
                    strcat(config->wavFilenameInputImag, "_imag.qmf");
                } else {
                    strcat(config->wavFilenameInputReal, "_real.stft");
                    strcat(config->wavFilenameInputImag, "_imag.stft");
                }
            }
        }
        if (config->wavFilenameOutput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav output filename (-of FILE)\n");
            ret = -1;
        }
        else
        {
            nWavExtensionChars = 0;
            nWavExtensionChars = (int)strspn(".wav",config->wavFilenameOutput);
            if ( nWavExtensionChars != 4 ) {
                strcpy(config->wavFilenameOutputReal, config->wavFilenameOutput);
                strcpy(config->wavFilenameOutputImag, config->wavFilenameOutput);
                if (config->gainDecType == GAIN_DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION) {
                    strcat(config->wavFilenameOutputReal, "_real.qmf");
                    strcat(config->wavFilenameOutputImag, "_imag.qmf");
                } else {
                    strcat(config->wavFilenameOutputReal, "_real.stft");
                    strcat(config->wavFilenameOutputImag, "_imag.stft");
                }
            }
        }
    }
    else if(config->gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION || config->gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION || config->gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION)
    {
        if (config->wavFilenameOutput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav output filename (-of FILE)\n");
            ret = -1;
        }
        if (config->audioSampleRate == 0)
        {
            fprintf(stderr, "Missing or wrong command line parameter: sample rate (-fs VALUE)\n");
            ret = -1;
        }
        if (config->wavFilenameOutput[0] == '\0')
        {
            fprintf(stderr, "Missing or wrong command line parameter: wav output filename (-of FILE)\n");
            ret = -1;
        }
        else
        {
            nWavExtensionChars = 0;
            nWavExtensionChars = (int)strspn(".wav",config->wavFilenameOutput);
            if ( nWavExtensionChars != 4 ) {
                strcpy(config->wavFilenameOutputReal, config->wavFilenameOutput);
                strcpy(config->wavFilenameOutputImag, config->wavFilenameOutput);
                if (config->gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_QMF64_APPLICATION) {
                    strcat(config->wavFilenameOutputReal, "_real.qmf");
                    strcat(config->wavFilenameOutputImag, "_imag.qmf");
                } else if (config->gainDecType == GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_STFT256_APPLICATION) {
                    strcat(config->wavFilenameOutputReal, "_real.stft");
                    strcat(config->wavFilenameOutputImag, "_imag.stft");
                } else {
                    ret = -1;
                }
            } else {
                if (config->gainDecType != GAIN_DEC_TYPE_GAINS_ONLY_INTERFACE_TD_APPLICATION) {
                    ret = -1;
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "Missing or wrong command line parameter: gain decoding type (-gdt VALUE)\n");
        ret = -1;
    }
    
    if (ret<0) {
        PrintCmdlineHelp( argv[0] );
    }
    
    return ret;
}

static int readSelProcOutFromFile( char* filename, float *loudnessNorm_dB, float* truePeak, int* selectedDrcSetIds, int* selectedDownmixIds, int* numSelectedDrcSets, float* boost, float* compress, int* drcCharacteristicTarget, int* baseChannelCount, int* targetChannelCount
#if MPEG_D_DRC_EXTENSION_V1
                                  , int* selectedLoudEqId, float* mixingLevel, int* selectedEqSetIds)
#else
                                  )
#endif
{
    FILE* fileHandle;
    int i, err = 0, tmpInt;
    int plotInfo = 0;
    float tmpFloat;
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open selection process transfer file\n");
        return -1;
    } else {
        if (plotInfo!=0) {
            fprintf(stderr, "Found selection process transfer file: %s.\n", filename );
        }
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
    
#if MPEG_D_DRC_EXTENSION_V1
    /* selectedLoudEqId and mixingLevel */
    err = fscanf(fileHandle, "%d %f\n", selectedLoudEqId, mixingLevel);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    /* selected EQ set (before and after DMX) */
    err = fscanf(fileHandle, "%d %d\n", &selectedEqSetIds[0], &selectedEqSetIds[1]);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
#else  /* read and ignore */
    err = fscanf(fileHandle, "%d %f\n", &tmpInt, &tmpFloat);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
    
    err = fscanf(fileHandle, "%d %d\n", &tmpInt, &tmpInt);
    if (err == 0)
    {
        fprintf(stderr,"Selection process transfer file has wrong format\n");
        return 1;
    }
#endif
    
    fclose(fileHandle);
    return 0;
}

static int downmixIdMatches(int downmixId, int decDownmixId)
{
    int match = 0;
    /* decDowmixId  -- downmix ID
     0:    0x0                base layout
     1:    0x0 + 0x7F         base layout + 0x7F
     2:    0x7F               0x7F only
     3:    !0x0 && !0x7F      target layout
     4:    !0x0               target layout + 0x7F */
    
    /* 0x0: base layout - before downmix
     0x7F: before or after downmix
     other downmixId: target layout - after downmix */
    
    switch (decDownmixId )
    {
        case 0:
            match = (downmixId == 0);
            break;
        case 1:
            match = ( (downmixId == 0) || (downmixId == 0x7F) );
            break;
        case 2:
            match = (downmixId == 0x7F);
            break;
        case 3:
            match = ( (downmixId != 0) && (downmixId != 0x7F) );
            break;
        case 4:
            match = (downmixId != 0);
            break;
    }
    return match;
}

#if MPEG_H_SYNTAX

static int readDownmixMatrixSetFromFile( char* filename, int* downmixId, int* targetChannelCountFromDownmixId, int* downmixIdCount)
{
    FILE* fileHandle;
    int i, err = 0;
    int plotInfo = 0;
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open downmix matrix set file\n");
        return -1;
    } else {
        if (plotInfo!=0) {
            fprintf(stderr, "Found downmix matrix set file: %s.\n", filename );
        }
    }
    
    /* downmixIds */
    err = fscanf(fileHandle, "%d\n", downmixIdCount);
    if (err == 0)
    {
        fprintf(stderr,"Downmix matrix set file has wrong format\n");
        return 1;
    }
    for (i=0; i<(*downmixIdCount); i++)
    {
        err = fscanf(fileHandle, "%d %d\n", &downmixId[i], &targetChannelCountFromDownmixId[i]);
        if (err == 0)
        {
            fprintf(stderr,"Downmix matrix set file has wrong format\n");
            return 1;
        }
    }
    
    fclose(fileHandle);
    return 0;
}

#endif
