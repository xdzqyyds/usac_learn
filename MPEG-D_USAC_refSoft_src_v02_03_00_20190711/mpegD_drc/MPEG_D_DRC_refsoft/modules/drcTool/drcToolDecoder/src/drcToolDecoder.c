/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc. and Fraunhofer IIS
 
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
 
 Apple Inc. and Fraunhofer IIS retains full right to modify and use the code for its
 own purpose, assign or donate the code to a third party and to inhibit third parties
 from using the code for products that do not conform to MPEG-related ITU Recommenda-
 tions and/or ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/***********************************************************************************/

#include "uniDrcCommon.h"
#include "uniDrcInterface.h"
#include "uniDrcBitstreamDecoder_api.h"
#include "uniDrcSelectionProcess_api.h"
#include "uniDrcGainDecoder_api.h"
#include "uniDrcInterfaceDecoder_api.h"
#include "wavIO.h"
#include "uniDrcHostParams.h"
#include "qmflib.h"
#include "peakLimiterlib.h"

/***********************************************************************************/

#define LIM_DEFAULT_THRESHOLD         (0.89125094f) /* -1 dBFS */
#define NUM_GAIN_DEC_INSTANCES 2
#define BITSTREAM_BUFFER_LENGTH 2048
/*#define MEASURE_PROCESSING_TIME 1*/
#define BYTE_ALIGNMENT_TEST 0

#if AMD1_SYNTAX
#define PARAMETRIC_DRC_DELAY_MAX_DEFAULT 4096 /* example */
#define EQ_DELAY_MAX_DEFAULT             256  /* example */
#endif

typedef enum _DEC_TYPE
{
    DEC_TYPE_TD_INTERFACE_TD_APPLICATION = 0,
    DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION = 1,
    DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION = 2,
    DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION = 3
} DEC_TYPE;

typedef enum _BITSTREAM_FILE_FORMAT
{
    BITSTREAM_FILE_FORMAT_DEFAULT = 0,
    BITSTREAM_FILE_FORMAT_SPLIT   = 1
} BITSTREAM_FILE_FORMAT;

char wavFilenameInput[FILENAME_MAX] = "";                                                               /* Wav Input file */
char sbFilenameInputReal[FILENAME_MAX] = "";                                                            /* Subband Domain Input file (real) */
char sbFilenameInputImag[FILENAME_MAX] = "";                                                            /* Subband Domain Input file (imag) */
char bitstreamFilenameInput_uniDrcConfig[FILENAME_MAX] = "";
char bitstreamFilenameInput_loudnessInfoSet[FILENAME_MAX] = "";
char bitstreamFilenameInput_uniDrcGain[FILENAME_MAX] = "";
char bitstreamFilenameInput_uniDrc[FILENAME_MAX] = "";                                                         /* Bitstream Input file */
char interfaceFilenameInput[FILENAME_MAX] = "";                                                         /* Interface Input file */
char wavFilenameOutput[FILENAME_MAX]      = "";                                                         /* Wav Output file */
char sbFilenameOutputReal[FILENAME_MAX]  = "";                                                          /* Subband Domain Output file (real) */
char sbFilenameOutputImag[FILENAME_MAX]  = "";                                                          /* Subband Domain Output file (imag) */

int bFillLastFrame            = 1;
int nSamplesPerChannelFilled  = 0;
unsigned int bytedepth_input  = 0;
#if AMD2_COR3
unsigned int bytedepth_output = 3;
#else
unsigned int bytedepth_output = 0;
#endif
unsigned int isLastFrameRead = 0, isLastFrameReadImag = 0, bitstreamComplete = 0;
unsigned long nTotalSamplesPerChannel, nTotalSamplesPerChannelImag;
unsigned int nSamplesReadPerChannel = 0, nSamplesReadPerChannelImag = 0;
unsigned int nZerosPaddedBeginning = 0, nZerosPaddedEnd = 0;
unsigned int nZerosPaddedBeginningImag = 0, nZerosPaddedEndImag = 0;
unsigned int nSamplesToWritePerChannel = 0, nSamplesToWritePerChannelImag  = 0;
unsigned int nSamplesWrittenPerChannel = 0, nSamplesWrittenPerChannelImag  = 0;
unsigned long int nTotalSamplesWrittenPerChannel = 0;

BITSTREAM_FILE_FORMAT bitstreamFileFormat;
DEC_TYPE          decType;
int               subBandDomainMode;
unsigned int      numInCh;
unsigned int      numOutCh;
unsigned int      samplingRate;
int               controlParameterIndex;
int               delayMode;
int               absorbDelayOn;
#if AMD1_SYNTAX
int               constantDelayOn;
int               audioDelaySamples;
#endif /* AMD1_SYNTAX */
int               gainDelaySamples;
int               subbandDomainIOFlag;
int               audioFrameSize;
int               subBandDownSamplingFactor;
int               subBandCount;
int               peakLimiterPresent;
int               interfaceBitstreamPresent;
#if AMD2_COR2
int               uniDrcConfigBitstreamPresent;
int               uniDrcGainBitstreamPresent;
int               loudnessInfoSetBitstreamPresent;
#endif
#if AMD2_COR2
int               targetLoudnessPresent;
float             targetLoudness;
int               drcEffectTypeRequestPresent;
unsigned long     drcEffectTypeRequest;
#endif
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
int ffLoudnessPresent = 0;
int ffDrcPresent      = 0;
char bitstreamFilenameInput_ffLoudness[FILENAME_MAX] = "";
char bitstreamFilenameInput_ffDrc[FILENAME_MAX]      = "";
#endif
#endif
#endif

/***********************************************************************************/

static void PrintCmdlineHeader (void);
static void PrintCmdlineHelp ( char* argv0 );
static int GetCmdline ( int argc, char** argv );
static int ApplyDownmix ( UniDrcSelProcOutput uniDrcSelProcOutput, float** audioIOBuffer, int frameSize, int numOutCh );
static int downmixIdMatches(int downmixId, int decDownmixId);
static int readInterfaceFile(HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct, char* inFilename, HANDLE_UNI_DRC_INTERFACE hUniDrcInterface);
static int copyBitstreamFileToBuffer(char* bitstreamFilename, unsigned char** pBitstreamBuffer, int* pNBytesBs);
#if AMD2_COR2
int setDrcEffectTypeRequest (unsigned long drcEffectTypeRequestPlusFallbacks, int* numDrcEffectTypeRequests, int* numDrcEffectTypeRequestsDesired, int* drcEffectTypeRequest);
#endif

/***********************************************************************************/

int main ( int argc, char* argv[] )
{
    int error                   = 0;
    int plotInfo                = 1;
    unsigned int i,j;
    
#ifdef MEASURE_PROCESSING_TIME
    clock_t startLap, stopLap, accuTime = 0, accuTimeQMF = 0;
    double elapsed_time, elapsed_time_QMF;
#endif
    
    HANDLE_UNI_DRC_BS_DEC_STRUCT hBitstreamDec = NULL;
    HANDLE_UNI_DRC_GAIN_DEC_STRUCTS hGainDec[NUM_GAIN_DEC_INSTANCES] = { NULL, NULL };
    HANDLE_UNI_DRC_SEL_PROC_STRUCT hSelectionProc = NULL;
    int decDownmixIdList[NUM_GAIN_DEC_INSTANCES] = { 0, 4 };
#if MPEG_D_DRC_EXTENSION_V1
    HANDLE_LOUDNESS_EQ_STRUCT hLoudnessEq  = NULL;
#endif
#if AMD1_SYNTAX
    int parametricDrcDelayGainDecInstance = 0;
    int parametricDrcDelay                = 0;
    int parametricDrcDelayMax             = 0;
    int eqDelayGainDecInstance            = 0;
    int eqDelay                           = 0;
    int eqDelayMax                        = 0;
    int delayLineSamples                  = 0;
#endif
    int flushingSamples                   = 0;
    int leftSamples                       = 0;
    int leftFrames                        = 0;
    int leftSamplesLastFrame              = 0;
    int isLastFrameWrite                  = 0;

    /* time domain I/O */
    WAVIO_HANDLE hWavIO           = NULL;
    FILE *pInFile                 = NULL;
    FILE *pOutFile                = NULL;
    
    /* subband domain I/O */
    WAVIO_HANDLE hWavIO_Real      = NULL;
    WAVIO_HANDLE hWavIO_Imag      = NULL;
    FILE *pInFileReal             = NULL;
    FILE *pInFileImag             = NULL;
    FILE *pOutFileReal            = NULL;
    FILE *pOutFileImag            = NULL;
    
    float **audioIOBuffer = NULL;
    float **audioIOBuffer_Real = NULL;
    float **audioIOBuffer_Imag = NULL;
#if AMD1_SYNTAX
    float **audioIOBufferDelayLine = NULL;
    float **audioIOBufferDelayLine_Real = NULL;
    float **audioIOBufferDelayLine_Imag = NULL;
#endif
    QMFLIB_POLYPHASE_ANA_FILTERBANK **hQmfAna = NULL;
    QMFLIB_POLYPHASE_SYN_FILTERBANK **hQmfSyn = NULL;
    float *audioInterleavedBuffer = NULL;
    TDLimiterPtr hLimiter = NULL;
    unsigned long frame;
    
    unsigned char* bitstream_uniDrcConfig = NULL;
    int nBitsReadBs_uniDrcConfig       = 0;
    int nBytesBs_uniDrcConfig          = 0;
    unsigned char* bitstream_loudnessInfoSet = NULL;
    int nBitsReadBs_loudnessInfoSet    = 0;
    int nBytesBs_loudnessInfoSet       = 0;
    unsigned char* bitstream           = NULL;
    int nBitsReadBs                    = 0;
    int nBytesReadBs                   = 0;
    int nBytesBs                       = 0;
    int nBitsOffsetBs                  = 0;
    int byteIndexBs                    = 0;
    
    /* data structs */
    HANDLE_UNI_DRC_CONFIG hUniDrcConfig = NULL;
    HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet = NULL;
    HANDLE_UNI_DRC_GAIN hUniDrcGain = NULL;
    HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct = NULL;
    
    HANDLE_UNI_DRC_INTERFACE hUniDrcInterface = NULL;
    UniDrcSelProcParams uniDrcSelProcParams;
    UniDrcSelProcOutput uniDrcSelProcOutput;

#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    int nBitsReadFfLoudness   = 0;
    int nBytesFfLoudness      = 0;
    int nBitsOffsetFfLoudness = 0;
    int byteIndexFfLoudness   = 0;
    
    int nBitsReadFfDrc = 0;
    int nBytesFfDrc    = 0;
    int nBitsOffsetFfDrc = 0;
    int byteIndexFfDrc   = 0;
    
    unsigned char* bitstreamFfLoudness = NULL;
    FILE *pInFileFfLoudness   = NULL;
    
    unsigned char* bitstreamFfDrc = NULL;
    FILE *pInFileFfDrc   = NULL;
#endif
#endif
#endif
    
    bitstreamFileFormat       = BITSTREAM_FILE_FORMAT_DEFAULT;
    decType                   = DEC_TYPE_TD_INTERFACE_TD_APPLICATION;
    subBandDomainMode         = SUBBAND_DOMAIN_MODE_OFF;
    subBandCount              = 0;
    subBandDownSamplingFactor = 0;
    samplingRate              = 0;
    audioFrameSize            = 1024;
    numInCh                   = -1;
    numOutCh                  = -1;
    controlParameterIndex     = -1;
    peakLimiterPresent        = 0;
    delayMode                 = 0;
    interfaceBitstreamPresent = 0;
#if AMD2_COR2
    uniDrcConfigBitstreamPresent    = 0;
    uniDrcGainBitstreamPresent      = 0;
    loudnessInfoSetBitstreamPresent = 0;
#endif
#if AMD2_COR2
    targetLoudnessPresent       = -1;
    targetLoudness              = UNDEFINED_LOUDNESS_VALUE;
    drcEffectTypeRequestPresent = -1;
    drcEffectTypeRequest        = 0;
#endif
#if AMD1_SYNTAX
    audioDelaySamples         = 0;
#endif /* AMD1_SYNTAX */
    gainDelaySamples          = -1;
    absorbDelayOn             = 1;
#if AMD1_SYNTAX
    constantDelayOn           = 0;
#endif
    subbandDomainIOFlag       = 0;    
    
    for ( i = 1; i < argc; ++i )
    {
        if ( !strcmp ( argv[i], "-v" ) )
        {
            plotInfo = atoi( argv[i + 1] ) ;
            continue;
        }
    }

    if (plotInfo!=0) {
        PrintCmdlineHeader ( );
    } else {
#if !MPEG_H_SYNTAX
#if !AMD1_SYNTAX
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Decoder (RM11).\n\n");
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Decoder (RM11).\n\n");
#endif
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Decoder (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
        fprintf(stdout,"!!! This tool is only meant for debugging of MPEG-D DRC bitstreams with MPEG-H 3DA syntax. !!!.\n");
        fprintf(stdout,"!!! Note that the processing chain and its output do NOT fulfill any MPEG-H 3DA conformance criteria, !!!.\n\n");
#endif
    }
    error = GetCmdline ( argc, argv );
    if(error) return -1;
    
    /* delay samples */
    if (gainDelaySamples == -1) {
        if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
            gainDelaySamples = 288; /* required for alignment of a time domain gain sequence applied in the QMF domain */
        } else {
            gainDelaySamples = 0;
        }
    }
    
    /* subbandDomainIOFlag check */
    if (subbandDomainIOFlag && (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION)) {
        return -1;
    } else if (subbandDomainIOFlag != 3 && (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION)) {
        return -1;
    }
    
    /* file names extensions */
    if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION) {
        strcat(sbFilenameInputReal, "_real.qmf");
        strcat(sbFilenameInputImag, "_imag.qmf");
        strcat(sbFilenameOutputReal, "_real.qmf");
        strcat(sbFilenameOutputImag, "_imag.qmf");
    } else if (decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
        strcat(sbFilenameInputReal, "_real.stft");
        strcat(sbFilenameInputImag, "_imag.stft");
        strcat(sbFilenameOutputReal, "_real.stft");
        strcat(sbFilenameOutputImag, "_imag.stft");
    }
    
    /**********************************/
    /* OPEN                           */
    /**********************************/
    
    if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
        
        /* Open input wav file */
        if (strlen(wavFilenameInput)) {
            pInFile  = fopen(wavFilenameInput, "rb");
        }
        if (pInFile == NULL) {
            fprintf(stderr, "Could not open input wav file: %s.\n", wavFilenameInput );
            return -1;
        }
        
        /* Open output wav file */
        if (strlen(wavFilenameOutput)) {
            pOutFile = fopen(wavFilenameOutput, "wb");
        }
        if (pOutFile == NULL) {
            fprintf(stderr, "Could not open output wav file: %s.\n", wavFilenameOutput );
            return -1;
        }
        
        /* open wave input/output */
        error = wavIO_init(&hWavIO,
                           audioFrameSize,
                           bFillLastFrame,
                           flushingSamples);
        if (error) goto Exit;
        
        error = wavIO_openRead(hWavIO,
                               pInFile,
                               &numInCh,
                               &samplingRate,
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
        
    } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
        
        /* Open subband domain input file */
        if (strlen(sbFilenameInputReal)) {
            pInFileReal  = fopen(sbFilenameInputReal, "rb");
        }
        if (pInFileReal == NULL) {
            fprintf(stderr, "Could not open real subband domain input file: %s.\n", sbFilenameInputReal );
            return -1;
        }
        if (strlen(sbFilenameInputImag)) {
            pInFileImag  = fopen(sbFilenameInputImag, "rb");
        }
        if (pInFileImag == NULL) {
            fprintf(stderr, "Could not open imaginary subband domain input file: %s.\n", sbFilenameInputImag );
            return -1;
        }
        
        /* Open subband domain output file */
        if (strlen(sbFilenameOutputReal)) {
            pOutFileReal = fopen(sbFilenameOutputReal, "wb");
        }
        if (pOutFileReal == NULL) {
            fprintf(stderr, "Could not open real subband domain output file: %s.\n", sbFilenameOutputReal );
            return -1;
        }
        if (strlen(sbFilenameOutputImag)) {
            pOutFileImag = fopen(sbFilenameOutputImag, "wb");
        }
        if (pOutFileImag == NULL) {
            fprintf(stderr, "Could not open imaginary subband domain output file: %s.\n", sbFilenameOutputImag );
            return -1;
        }
        
        /* open subband domain input/output */
        error = wavIO_init(&hWavIO_Real,
                           audioFrameSize/subBandDownSamplingFactor*subBandCount,
                           bFillLastFrame,
                           flushingSamples);
        if (error) goto Exit;
        error = wavIO_init(&hWavIO_Imag,
                           audioFrameSize/subBandDownSamplingFactor*subBandCount,
                           bFillLastFrame,
                           flushingSamples);
        if (error) goto Exit;
        
        error = wavIO_openRead(hWavIO_Real,
                               pInFileReal,
                               &numInCh,
                               &samplingRate,
                               &bytedepth_input,
                               &nTotalSamplesPerChannel,
                               &nSamplesPerChannelFilled);
        if (error) goto Exit;
        error = wavIO_openRead(hWavIO_Imag,
                               pInFileImag,
                               &numInCh,
                               &samplingRate,
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
        
    }
    
    /* copy bitstream file to buffer */
    if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
#if AMD2_COR2
        if (uniDrcConfigBitstreamPresent) {
            error = copyBitstreamFileToBuffer(bitstreamFilenameInput_uniDrcConfig, &bitstream_uniDrcConfig, &nBytesBs_uniDrcConfig);
            if (error) goto Exit;
        }
        if (loudnessInfoSetBitstreamPresent) {
            error = copyBitstreamFileToBuffer(bitstreamFilenameInput_loudnessInfoSet, &bitstream_loudnessInfoSet, &nBytesBs_loudnessInfoSet);
            if (error) goto Exit;
        }
        if (uniDrcGainBitstreamPresent) {
            error = copyBitstreamFileToBuffer(bitstreamFilenameInput_uniDrcGain, &bitstream, &nBytesBs);
            if (error) goto Exit;
        }
#else
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_uniDrcConfig, &bitstream_uniDrcConfig, &nBytesBs_uniDrcConfig);
        if (error) goto Exit;
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_loudnessInfoSet, &bitstream_loudnessInfoSet, &nBytesBs_loudnessInfoSet);
        if (error) goto Exit;
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_uniDrcGain, &bitstream, &nBytesBs);
        if (error) goto Exit;
#endif
    } else {
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_uniDrc, &bitstream, &nBytesBs);
        if (error) goto Exit;
    }
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    if (ffLoudnessPresent) {
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_ffLoudness, &bitstreamFfLoudness, &nBytesFfLoudness);
        if (error) goto Exit;
    }
    if (ffDrcPresent) {
        error = copyBitstreamFileToBuffer(bitstreamFilenameInput_ffDrc, &bitstreamFfDrc, &nBytesFfDrc);
        if (error) goto Exit;
    }
#endif
#endif
#endif

    /* Open libraries */
    error = openUniDrcBitstreamDec(&hBitstreamDec, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (error) goto Exit;
    
    error = initUniDrcBitstreamDec(hBitstreamDec, samplingRate, audioFrameSize, delayMode, -1, NULL);
    if (error) goto Exit;
    
    error = openUniDrcSelectionProcess(&hSelectionProc);
    if (error) goto Exit;
    

    for (i=0; i<NUM_GAIN_DEC_INSTANCES; i++) {
        error = drcDecOpen(&hGainDec[i], &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
        if (error) goto Exit;
        
        error = drcDecInit(audioFrameSize,
                           samplingRate,
                           gainDelaySamples,
                           delayMode,
                           subBandDomainMode,
                           hGainDec[i]);
        if (error) goto Exit;
    }
    
    /* delay compensation */
    if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
        flushingSamples += 577; /* QMF64 analyis/synthesis delay */
    }
    
    /* uniDrcInterface() */
    error = openUniDrcInterfaceDecoder(&hUniDrcIfDecStruct, &hUniDrcInterface);
    if (error) goto Exit;
    
    /* prepare selection process parameters */
    if (interfaceBitstreamPresent) {
        /* get parameters from interface */
        error = readInterfaceFile(hUniDrcIfDecStruct,interfaceFilenameInput,hUniDrcInterface);
        if (error) goto Exit;
        /* set selection process parameters */
        error = initUniDrcSelectionProcess(hSelectionProc, NULL, hUniDrcInterface, subBandDomainMode);
        if (error) goto Exit;
        /* configure peak limiter */
        if (hUniDrcInterface->loudnessNormalizationParameterInterfacePresent && hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent) {
            peakLimiterPresent = 1;
        }
    }
    else {
        /* get parameters from hostParams */
        error = setDefaultParams_selectionProcess(&uniDrcSelProcParams);
        if (error) goto Exit;
#if AMD2_COR2
        if(controlParameterIndex > -1) {
#endif
        error = setCustomParams_selectionProcess(controlParameterIndex,&uniDrcSelProcParams);
        if (error) goto Exit;
#if AMD2_COR2
        }
#endif
        error = evaluateCustomParams_selectionProcess(&uniDrcSelProcParams);
        if (error) goto Exit;
#if AMD2_COR2
        if (targetLoudnessPresent > -1) {
            uniDrcSelProcParams.loudnessNormalizationOn = targetLoudnessPresent;
            if (uniDrcSelProcParams.loudnessNormalizationOn == TRUE) {
                uniDrcSelProcParams.targetLoudness = targetLoudness;
            }
        }
        
        if (drcEffectTypeRequestPresent > -1) {
            uniDrcSelProcParams.dynamicRangeControlOn = TRUE;
            uniDrcSelProcParams.numDrcFeatureRequests = 1;
            uniDrcSelProcParams.drcFeatureRequestType[0] = MATCH_EFFECT_TYPE;
            error = setDrcEffectTypeRequest (drcEffectTypeRequest, &uniDrcSelProcParams.numDrcEffectTypeRequests[0], &uniDrcSelProcParams.numDrcEffectTypeRequestsDesired[0], &uniDrcSelProcParams.drcEffectTypeRequest[0][0]);
            if (error) return (error);
        }
        
        if (peakLimiterPresent == 1) {
            uniDrcSelProcParams.peakLimiterPresent = peakLimiterPresent;
        }
#endif
        /* set selection process parameters */
        error = initUniDrcSelectionProcess(hSelectionProc, &uniDrcSelProcParams, NULL, subBandDomainMode);
        if (error) goto Exit;
        /* configure peak limiter */
        if (uniDrcSelProcParams.peakLimiterPresent) {
            peakLimiterPresent = 1;
        }
#if MPEG_D_DRC_EXTENSION_V1
        /* set uniDrcInterface struct for updateLoudnessEqualizerParams() function */
        setLoudEqParameters_uniDrcInterface(&uniDrcSelProcParams, hUniDrcInterface);
        if (error) goto Exit;
#endif
    }
    
    /* disable limiter for FD dec type */
    if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
        peakLimiterPresent = 0;
    }
    
    /**********************************/
    /* DECODE_LOOP                    */
    /**********************************/
    
    /* do decoding and application of gain over whole file */
    frame = 0;
    while(!isLastFrameWrite && !bitstreamComplete){
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        if (frame == 0 && ffDrcPresent == 1) {
            error = processUniDrcBitstreamDec_isobmff(hBitstreamDec,
                                                      hUniDrcConfig,
                                                      hLoudnessInfoSet,
                                                      numInCh,
                                                      &bitstreamFfDrc[byteIndexFfDrc],
                                                      nBytesFfDrc,
                                                      nBitsOffsetFfDrc,
                                                      &nBitsReadFfDrc);
            if (error) return(error);
        }
#endif
#endif
#endif
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            if (frame == 0) {
                error = processUniDrcBitstreamDec_uniDrcConfig(hBitstreamDec,
                                                               hUniDrcConfig,
                                                               NULL,
                                                               &bitstream_uniDrcConfig[byteIndexBs],
                                                               nBytesBs_uniDrcConfig,
                                                               0,
                                                               &nBitsReadBs_uniDrcConfig);
                if (error > PROC_COMPLETE) goto Exit;
                error = processUniDrcBitstreamDec_loudnessInfoSet(hBitstreamDec,
                                                                  hLoudnessInfoSet,
                                                                  hUniDrcConfig,
                                                                  &bitstream_loudnessInfoSet[byteIndexBs],
                                                                  nBytesBs_loudnessInfoSet,
                                                                  0,
                                                                  &nBitsReadBs_loudnessInfoSet);
                if (error > PROC_COMPLETE) goto Exit;
            }
        } else {
            error = processUniDrcBitstreamDec_uniDrc(hBitstreamDec,
                                                     hUniDrcConfig,
                                                     hLoudnessInfoSet,
                                                     &bitstream[byteIndexBs],
                                                     nBytesBs,
                                                     nBitsOffsetBs,
                                                     &nBitsReadBs);
            if (error == PROC_COMPLETE) bitstreamComplete = 1;
            if (error > PROC_COMPLETE) goto Exit;
            
            nBytesReadBs  = nBitsReadBs/8;
            nBitsOffsetBs = nBitsReadBs - nBytesReadBs*8;
            byteIndexBs   += nBytesReadBs;
            nBytesBs      -= nBytesReadBs;
        }
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        if (frame == 0 && ffLoudnessPresent == 1) {
            error = processUniDrcBitstreamDec_isobmff(hBitstreamDec,
                                                      hUniDrcConfig,
                                                      hLoudnessInfoSet,
                                                      numInCh,
                                                      &bitstreamFfLoudness[byteIndexFfLoudness],
                                                      nBytesFfLoudness,
                                                      nBitsOffsetFfLoudness,
                                                      &nBitsReadFfLoudness);
            if (error) return(error);
        }
#endif
#endif
#endif
        
#if AMD2_COR2
        if (hUniDrcConfig->channelLayout.baseChannelCount < 0 || bitstream_uniDrcConfig == NULL) {
            hUniDrcConfig->channelLayout.baseChannelCount = numInCh;
        }
#endif
        /**********************************/
        /* FIRST FRAME                    */
        /**********************************/
        
        if (frame == 0) {
            error = processUniDrcSelectionProcess(hSelectionProc,
                                                  hUniDrcConfig,
                                                  hLoudnessInfoSet,
                                                  &uniDrcSelProcOutput);
            if (error) goto Exit;

            for (i=0; i<NUM_GAIN_DEC_INSTANCES; i++) {
                /* Assign the selected DRC sets to the two DRC gain decoder instances:
                 one before, one after downmix */
                int audioChannelCount;
                int numMatchingDrcSets = 0;
                int matchingDrcSetIds[3], matchingDownmixIds[3];
                for (j=0; j<(unsigned int)uniDrcSelProcOutput.numSelectedDrcSets; j++) {
#if AMD1_SYNTAX
                    if (decDownmixIdList[i] == 1) {
                        fprintf(stderr, "decDownmixId==1 not allowed. DRC sets with downmixId = 0x7F have to be applied after DMX in AMD1.\n");
                        return -1;
                    }
#endif
                    if (downmixIdMatches(uniDrcSelProcOutput.selectedDownmixIds[j], decDownmixIdList[i])) {
                        matchingDrcSetIds[numMatchingDrcSets] = uniDrcSelProcOutput.selectedDrcSetIds[j];
                        matchingDownmixIds[numMatchingDrcSets] = uniDrcSelProcOutput.selectedDownmixIds[j];
                        numMatchingDrcSets++;
                    }
                }
                if (i==0) {
                    if (numInCh != uniDrcSelProcOutput.baseChannelCount) goto Exit;
                    audioChannelCount = numInCh;
                } else if (i==1) {
                    numOutCh = uniDrcSelProcOutput.targetChannelCount;
                    audioChannelCount = numOutCh;
                }
                
                error = drcDecInitAfterConfig(audioChannelCount,
                                              matchingDrcSetIds,
                                              matchingDownmixIds,
                                              numMatchingDrcSets
#if MPEG_D_DRC_EXTENSION_V1
                                              , uniDrcSelProcOutput.selectedEqSetIds[i]
#endif
#if MPEG_H_SYNTAX
                                              , -1
                                              , -1
#endif
                                              , hGainDec[i]
                                              , hUniDrcConfig
#if AMD1_SYNTAX
                                              , hLoudnessInfoSet
#endif
                                              );
                if (error) goto Exit;
                
#if AMD1_SYNTAX
                error = getParametricDrcDelay(hGainDec[i], hUniDrcConfig, &parametricDrcDelayGainDecInstance, &parametricDrcDelayMax);
                if (error) goto Exit;
                error = getEqDelay(hGainDec[i], hUniDrcConfig, &eqDelayGainDecInstance, &eqDelayMax);
                if (error) goto Exit;
                parametricDrcDelay += parametricDrcDelayGainDecInstance;
                eqDelay += eqDelayGainDecInstance;
#endif
            }
#if MPEG_D_DRC_EXTENSION_V1
            {
                if (hUniDrcConfig->uniDrcConfigExt.drcExtensionV1Present)
                {                    
                    int loudEqInstructionsIndex;
                    error = findLoudEqInstructionsIndexForId(hUniDrcConfig,
                                                             uniDrcSelProcOutput.selectedLoudEqId,
                                                             &loudEqInstructionsIndex);
                    if (error) return (error);
                    
                    error = initLoudEq(&hLoudnessEq,
                                       hGainDec[0],
                                       &hUniDrcConfig->uniDrcConfigExt,
                                       loudEqInstructionsIndex,
                                       uniDrcSelProcOutput.mixingLevel);
                    if (error) return (error);
                    
                    error = updateLoudnessEqualizerParams(hLoudnessEq, hUniDrcInterface);
                    if (error) return (error);
                }
            }
#endif
#if AMD1_SYNTAX
            {
                if (parametricDrcDelayMax == -1) {
                    parametricDrcDelayMax = PARAMETRIC_DRC_DELAY_MAX_DEFAULT;
                }
                if (eqDelayMax == -1) {
                    eqDelayMax = EQ_DELAY_MAX_DEFAULT;
                }
                
                if (!constantDelayOn) {
                    
                    flushingSamples += parametricDrcDelay + eqDelay + audioDelaySamples;
                    delayLineSamples = audioDelaySamples;
                    
                    if (!absorbDelayOn) {
                        flushingSamples = 0;
                    }
                } else {
                    flushingSamples += parametricDrcDelayMax + eqDelayMax + audioDelaySamples;
                    delayLineSamples = flushingSamples - parametricDrcDelay + eqDelay;
                    
                    if (!absorbDelayOn) {
                        flushingSamples = 0;
                    }
                }
                
                if (parametricDrcDelay+eqDelay > parametricDrcDelayMax+eqDelayMax) {
                    fprintf(stderr, "WARNING: parametricDrcDelay+eqDelay > parametricDrcDelayMax+eqDelayMax.\n");
                }
            }
#endif
            if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
                
                error = wavIO_openWrite(hWavIO,
                                        pOutFile,
                                        numOutCh,
                                        samplingRate,
                                        bytedepth_output);
                if (error) goto Exit;
                
                /* allocate local audio buffers */
                audioIOBuffer = (float**)calloc(numInCh,sizeof(float*));
                if (audioIOBuffer == NULL) { return -1; }
                
                for (i=0; i< numInCh; i++)
                {
                    audioIOBuffer[i] = (float*)calloc(audioFrameSize,sizeof(float));
                    if (audioIOBuffer[i] == NULL) { return -1; }
                }
#if AMD1_SYNTAX
                audioIOBufferDelayLine = (float**)calloc(numOutCh,sizeof(float*));
                if (audioIOBufferDelayLine == NULL) { return -1; }
                for (i=0; i< numOutCh; i++)
                {
                    audioIOBufferDelayLine[i] = (float*)calloc(audioFrameSize+delayLineSamples,sizeof(float));
                    if (audioIOBufferDelayLine[i] == NULL) { return -1; }
                }
#endif
                if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION)
                {
                    audioIOBuffer_Real = (float**)calloc(numInCh,sizeof(float*));
                    if (audioIOBuffer_Real == NULL) { return -1; }
                    audioIOBuffer_Imag = (float**)calloc(numInCh,sizeof(float*));
                    if (audioIOBuffer_Imag == NULL) { return -1; }
                    hQmfAna = (QMFLIB_POLYPHASE_ANA_FILTERBANK**)calloc(numInCh, sizeof(QMFLIB_POLYPHASE_ANA_FILTERBANK*));
                    if (hQmfAna == NULL) { return -1; }
                    hQmfSyn = (QMFLIB_POLYPHASE_SYN_FILTERBANK**)calloc(numOutCh, sizeof(QMFLIB_POLYPHASE_SYN_FILTERBANK*));
                    if (hQmfSyn == NULL) { return -1; }
                    QMFlib_InitAnaFilterbank(64, 0);
                    QMFlib_InitSynFilterbank(64, 0);
                    
                    for (i=0; i < numInCh; i++)
                    {
                        audioIOBuffer_Real[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount,sizeof(float));
                        if (audioIOBuffer_Real[i] == NULL) { return -1; }
                        audioIOBuffer_Imag[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount,sizeof(float));
                        if (audioIOBuffer_Imag[i] == NULL) { return -1; }
                        QMFlib_OpenAnaFilterbank(&(hQmfAna[i]));
                        
                    }
                    for (i=0; i < numOutCh; i++)
                    {
                        QMFlib_OpenSynFilterbank(&(hQmfSyn[i]));
                    }
#if AMD1_SYNTAX
                    audioIOBufferDelayLine_Real = (float**)calloc(numOutCh,sizeof(float*));
                    if (audioIOBufferDelayLine_Real == NULL) { return -1; }
                    audioIOBufferDelayLine_Imag = (float**)calloc(numOutCh,sizeof(float*));
                    if (audioIOBufferDelayLine_Imag == NULL) { return -1; }
                    for (i=0; i< numOutCh; i++)
                    {
                        audioIOBufferDelayLine_Real[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount+delayLineSamples/subBandDownSamplingFactor*subBandCount,sizeof(float));
                        if (audioIOBufferDelayLine_Real[i] == NULL) { return -1; }
                        audioIOBufferDelayLine_Imag[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount+delayLineSamples/subBandDownSamplingFactor*subBandCount,sizeof(float));
                        if (audioIOBufferDelayLine_Imag[i] == NULL) { return -1; }
                    }
#endif
                }
                
                if (peakLimiterPresent)
                {
                    hLimiter = createLimiter(TDL_ATTACK_DEFAULT_MS,
                                             TDL_RELEASE_DEFAULT_MS,
                                             LIM_DEFAULT_THRESHOLD,
                                             numOutCh,
                                             samplingRate);
                    if (hLimiter == NULL) { return -1; }
                    audioInterleavedBuffer = (float*)calloc(numOutCh*audioFrameSize, sizeof(float));
                    if (audioInterleavedBuffer == NULL) { return -1; }
                    
                    if (absorbDelayOn) {
                        flushingSamples += getLimiterDelay(hLimiter);
                    }
                }
                if (flushingSamples != 0) {
                    error = wavIO_setDelay(hWavIO, -flushingSamples);
                    if (error) goto Exit;
                    
                    if (plotInfo == 2) {
                        fprintf(stdout,"Info: %d samples delay compensated.\n",flushingSamples);
                    }
                }
                
            } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
                
                error = wavIO_openWrite(hWavIO_Real,
                                        pOutFileReal,
                                        numOutCh,
                                        samplingRate,
                                        bytedepth_output);
                if (error) goto Exit;
                error = wavIO_openWrite(hWavIO_Imag,
                                        pOutFileImag,
                                        numOutCh,
                                        samplingRate,
                                        bytedepth_output);
                if (error) goto Exit;
                
                /* allocate local audio buffers */
                audioIOBuffer_Real = (float**)calloc(numInCh,sizeof(float*));
                if (audioIOBuffer_Real == NULL) { return -1; }
                audioIOBuffer_Imag = (float**)calloc(numInCh,sizeof(float*));
                if (audioIOBuffer_Imag == NULL) { return -1; }
                
                for (i=0; i < numInCh; i++)
                {
                    audioIOBuffer_Real[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount,sizeof(float));
                    if (audioIOBuffer_Real[i] == NULL) { return -1; }
                    audioIOBuffer_Imag[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount,sizeof(float));
                    if (audioIOBuffer_Imag[i] == NULL) { return -1; }
                }
#if AMD1_SYNTAX
                audioIOBufferDelayLine_Real = (float**)calloc(numOutCh,sizeof(float*));
                if (audioIOBufferDelayLine_Real == NULL) { return -1; }
                audioIOBufferDelayLine_Imag = (float**)calloc(numOutCh,sizeof(float*));
                if (audioIOBufferDelayLine_Imag == NULL) { return -1; }
                for (i=0; i< numOutCh; i++)
                {
                    audioIOBufferDelayLine_Real[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount+delayLineSamples/subBandDownSamplingFactor*subBandCount,sizeof(float));
                    if (audioIOBufferDelayLine_Real[i] == NULL) { return -1; }
                    audioIOBufferDelayLine_Imag[i] = (float*)calloc(audioFrameSize/subBandDownSamplingFactor*subBandCount+delayLineSamples/subBandDownSamplingFactor*subBandCount,sizeof(float));
                    if (audioIOBufferDelayLine_Imag[i] == NULL) { return -1; }
                }
#endif
                if (peakLimiterPresent)
                {
                    /* no peak limiter for FD domain */
                }
                
                if (flushingSamples != 0) {
                    fprintf(stderr, "WARNING: Delay compensation and delay line in the FD domain not supported, yet (numSamplesToCompensate = %d).\n", flushingSamples);
                    return -1;
                    
                    error = wavIO_setDelay(hWavIO_Imag, -flushingSamples);
                    if (error) goto Exit;
                    
                    if (plotInfo == 2) {
                        fprintf(stdout,"Info: %d samples delay compensated.\n",flushingSamples);
                    }
                }
            }
        }
        
        error = processUniDrcBitstreamDec_uniDrcGain(hBitstreamDec,
                                                     hUniDrcConfig,
                                                     hUniDrcGain,
                                                     &bitstream[byteIndexBs],
                                                     nBytesBs,
                                                     nBitsOffsetBs,
                                                     &nBitsReadBs);
        if (error == PROC_COMPLETE) bitstreamComplete = 1;
        if (error > PROC_COMPLETE) goto Exit;
        
        nBytesReadBs  = nBitsReadBs/8;
        nBitsOffsetBs = nBitsReadBs - nBytesReadBs*8;
        byteIndexBs   += nBytesReadBs;
        nBytesBs      -= nBytesReadBs;

#if BYTE_ALIGNMENT_TEST
        if ( nBitsReadBs % 8 == 4){
        } else if ( nBitsReadBs % 8 < 4 && nBitsReadBs % 8 != 0) {
          nBitsReadBs = (((nBitsReadBs+7)/8)*8) - 4;
          nBitsOffsetBs = 4;
        } else if ( nBitsReadBs % 8 > 4 || nBitsReadBs % 8 == 0){
          if(nBitsReadBs % 8 != 0) {
            byteIndexBs   = byteIndexBs + 1;
            nBytesBs      = nBytesBs - 1;
          }
          nBitsReadBs = (((nBitsReadBs+7)/8)*8) + 4;
          nBitsOffsetBs = 4;
        }
        
        nBitsReadBs   = nBitsReadBs + 8 - nBitsOffsetBs;
        nBytesReadBs  = nBitsReadBs/8;
        nBitsOffsetBs = 0;
        byteIndexBs   = byteIndexBs + 1;
        nBytesBs      = nBytesBs - 1;
        if(nBytesBs == 0) {
          bitstreamComplete = 1;
        }
#endif
        
        
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
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
        
        /**********************************/
        /* PROCESS                        */
        /**********************************/
        
        /* read input audio frame */
        if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
            if (!isLastFrameRead) {
                wavIO_readFrame(hWavIO, audioIOBuffer, &nSamplesReadPerChannel, &isLastFrameRead, &nZerosPaddedBeginning, &nZerosPaddedEnd);
                if (isLastFrameRead) {
                    leftSamples = nSamplesReadPerChannel + flushingSamples;
                    
                    if (leftSamples <= audioFrameSize) {
                        leftSamplesLastFrame = leftSamples;
                        leftSamples = 0;
                        leftFrames  = 0;
                        isLastFrameWrite = 1;
                    } else {
                        leftSamples = leftSamples-audioFrameSize;
                        leftFrames = (int) ceil(((double)leftSamples)/audioFrameSize);
                        leftSamplesLastFrame = leftSamples-(leftFrames-1)*audioFrameSize;
                    }
                }
            } else {
                if (leftFrames == 1) {
                    isLastFrameWrite = 1;
                    leftFrames--;
                } else {
                    leftFrames--;
                }
                
                /* reset input once */
                if (isLastFrameRead == 1) {
                    isLastFrameRead = 2;
                    for (i=0; i < numInCh; i++) {
                        for (j=0; j < audioFrameSize; j++) {
                            audioIOBuffer[i][j] = 0.f;
                        }
                    }
                }
            }
        } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
            if (!isLastFrameRead) {
                wavIO_readFrame(hWavIO_Real, audioIOBuffer_Real, &nSamplesReadPerChannel, &isLastFrameRead, &nZerosPaddedBeginning, &nZerosPaddedEnd);
                wavIO_readFrame(hWavIO_Imag, audioIOBuffer_Imag, &nSamplesReadPerChannelImag, &isLastFrameReadImag, &nZerosPaddedBeginningImag, &nZerosPaddedEndImag);
                if (isLastFrameRead) {
                    leftSamples = nSamplesReadPerChannel + flushingSamples;
                    
                    if (leftSamples <= audioFrameSize) {
                        leftSamplesLastFrame = leftSamples;
                        leftSamples = 0;
                        leftFrames  = 0;
                        isLastFrameWrite = 1;
                    } else {
                        leftSamples = leftSamples-audioFrameSize;
                        leftFrames = (int) ceil(((double)leftSamples)/audioFrameSize);
                        leftSamplesLastFrame = leftSamples-(leftFrames-1)*audioFrameSize;
                    }
                }
            } else {
                if (leftFrames == 1) {
                    isLastFrameWrite = 1;
                    leftFrames--;
                } else {
                    leftFrames--;
                }
                
                /* reset input once */
                if (isLastFrameRead == 1) {
                    isLastFrameRead = 2;
                    for (i=0; i < numInCh; i++) {
                        for (j=0; j < audioFrameSize; j++) {
                            audioIOBuffer_Real[i][j] = 0.f;
                            audioIOBuffer_Imag[i][j] = 0.f;
                        }
                    }
                }
            }
        }
        
        if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION) {
            
#ifdef MEASURE_PROCESSING_TIME
            /* start lap timer */
            startLap = clock();
#endif
            
            /* DRC processing before DMX */
            error = drcProcessTime(hGainDec[0],
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer,
                                   uniDrcSelProcOutput.loudnessNormalizationGainDb,
                                   uniDrcSelProcOutput.boost,
                                   uniDrcSelProcOutput.compress,
                                   uniDrcSelProcOutput.drcCharacteristicTarget);
            if (error) goto Exit;
            
            error = ApplyDownmix(uniDrcSelProcOutput,
                                 audioIOBuffer,
                                 audioFrameSize,
                                 numOutCh);
            if (error) goto Exit;
            
            /* DRC processing after DMX */
            error = drcProcessTime(hGainDec[1],
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer,
                                   uniDrcSelProcOutput.loudnessNormalizationGainDb,
                                   uniDrcSelProcOutput.boost,
                                   uniDrcSelProcOutput.compress,
                                   uniDrcSelProcOutput.drcCharacteristicTarget);
            if (error) goto Exit;
            
#ifdef MEASURE_PROCESSING_TIME
            /* stop lap timer and accumulate */
            stopLap = clock();
            accuTime += stopLap - startLap;
#endif
            
        }
        else if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
            
            if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
#ifdef MEASURE_PROCESSING_TIME
                /* start lap timer */
                startLap = clock();
#endif
                
                /* QMF analysis */
                for (i=0; i < numInCh; i++) {
                    for (j=0; j < audioFrameSize; j += 64) {
                        QMFlib_CalculateAnaFilterbank(hQmfAna[i],
                                                      &(audioIOBuffer[i][j]),
                                                      &(audioIOBuffer_Real[i][j]),
                                                      &(audioIOBuffer_Imag[i][j]),
                                                      0);
                    }
                }
                
#ifdef MEASURE_PROCESSING_TIME
                /* stop lap timer and accumulate */
                stopLap = clock();
                accuTimeQMF += stopLap - startLap;
#endif
            }
            
#ifdef MEASURE_PROCESSING_TIME
            /* start lap timer */
            startLap = clock();
#endif
            
            /* DRC processing before DMX */
            error = drcProcessFreq(hGainDec[0],
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer_Real,
                                   audioIOBuffer_Imag,
                                   uniDrcSelProcOutput.loudnessNormalizationGainDb,
                                   uniDrcSelProcOutput.boost,
                                   uniDrcSelProcOutput.compress,
                                   uniDrcSelProcOutput.drcCharacteristicTarget);
            if (error) goto Exit;
            
            error = ApplyDownmix(uniDrcSelProcOutput,
                                 audioIOBuffer_Real,
                                 audioFrameSize,
                                 numOutCh);
            if (error) goto Exit;
            
            error = ApplyDownmix(uniDrcSelProcOutput,
                                 audioIOBuffer_Imag,
                                 audioFrameSize,
                                 numOutCh);
            if (error) goto Exit;
            
            /* DRC processing after DMX */
            error = drcProcessFreq(hGainDec[1],
                                   hUniDrcConfig,
                                   hUniDrcGain,
                                   audioIOBuffer_Real,
                                   audioIOBuffer_Imag,
                                   uniDrcSelProcOutput.loudnessNormalizationGainDb,
                                   uniDrcSelProcOutput.boost,
                                   uniDrcSelProcOutput.compress,
                                   uniDrcSelProcOutput.drcCharacteristicTarget);
            if (error) goto Exit;
            
#ifdef MEASURE_PROCESSING_TIME
            /* stop lap timer and accumulate */
            stopLap = clock();
            accuTime += stopLap - startLap;
#endif
            
            if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
#ifdef MEASURE_PROCESSING_TIME
                /* start lap timer */
                startLap = clock();
#endif
                
                /* QMF synthesis */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j += 64) {
                        QMFlib_CalculateSynFilterbank(hQmfSyn[i],
                                                      &(audioIOBuffer_Real[i][j]),
                                                      &(audioIOBuffer_Imag[i][j]),
                                                      &(audioIOBuffer[i][j]),
                                                      0);
                    }
                }
                
#ifdef MEASURE_PROCESSING_TIME
                /* stop lap timer and accumulate */
                stopLap = clock();
                accuTimeQMF += stopLap - startLap;
#endif
            }
        }
        
        /* loudness normalization */
        if (uniDrcSelProcOutput.loudnessNormalizationGainDb != 0.0f)
        {
            float loudnessNormalizationGain = (float)pow(10.0,uniDrcSelProcOutput.loudnessNormalizationGainDb/20.0);
            for (i=0; i < numOutCh; i++) {
                for (j=0; j < audioFrameSize; j++) {
                    if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
                        audioIOBuffer[i][j] *= loudnessNormalizationGain;
                    } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
                        audioIOBuffer_Real[i][j] *= loudnessNormalizationGain;
                        audioIOBuffer_Imag[i][j] *= loudnessNormalizationGain;
                    }
                }
            }
        }
        
        if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
            
            if (peakLimiterPresent)
            {
                /* interleave signal */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioInterleavedBuffer[j*numOutCh + i] = audioIOBuffer[i][j];
                    }
                }
                
                error = applyLimiter(hLimiter,
                                     audioInterleavedBuffer,
                                     audioFrameSize);
                if (error) goto Exit;
                
                /* deinterleave signal */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioIOBuffer[i][j] = audioInterleavedBuffer[j*numOutCh + i];
                    }
                }
            }
            
#if AMD1_SYNTAX
            if (delayLineSamples) {
                /* store samples in delay line */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioIOBufferDelayLine[i][delayLineSamples+j] = audioIOBuffer[i][j];
                    }
                }
                /* get samples from delay line */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioIOBuffer[i][j] = audioIOBufferDelayLine[i][j];
                    }
                }
                /* advance delay line */
                for (i=0; i < numOutCh; i++) {
                    memmove(audioIOBufferDelayLine[i], &audioIOBufferDelayLine[i][audioFrameSize],sizeof(float) * delayLineSamples);
                }
            }
#endif
            /* write output audio frame */
            if (isLastFrameWrite) {
                nSamplesToWritePerChannel = leftSamplesLastFrame;
            } else {
                nSamplesToWritePerChannel = audioFrameSize;
            }
            wavIO_writeFrame(hWavIO, audioIOBuffer, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
            
        } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
            
#if AMD1_SYNTAX
            if (delayLineSamples) {
                /* store samples in delay line */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioIOBufferDelayLine_Real[i][delayLineSamples+j] = audioIOBuffer_Real[i][j];
                        audioIOBufferDelayLine_Imag[i][delayLineSamples+j] = audioIOBuffer_Imag[i][j];
                    }
                }
                /* get delayed samples from delay line */
                for (i=0; i < numOutCh; i++) {
                    for (j=0; j < audioFrameSize; j++) {
                        audioIOBuffer_Real[i][j] = audioIOBufferDelayLine_Real[i][j];
                        audioIOBuffer_Imag[i][j] = audioIOBufferDelayLine_Imag[i][j];
                    }
                }
                /* advance delay line */
                for (i=0; i < numOutCh; i++) {
                    memmove(audioIOBufferDelayLine_Real[i], &audioIOBufferDelayLine_Real[i][audioFrameSize],sizeof(float) * delayLineSamples);
                    memmove(audioIOBufferDelayLine_Imag[i], &audioIOBufferDelayLine_Imag[i][audioFrameSize],sizeof(float) * delayLineSamples);
                }
            }
#endif
            
            /* write output audio frame */
            if (isLastFrameWrite) {
                nSamplesToWritePerChannel     = leftSamplesLastFrame;
                nSamplesToWritePerChannelImag = leftSamplesLastFrame;
            } else {
                nSamplesToWritePerChannel     = audioFrameSize;
                nSamplesToWritePerChannelImag = audioFrameSize;
            }
            wavIO_writeFrame(hWavIO_Real, audioIOBuffer_Real, nSamplesToWritePerChannel, &nSamplesWrittenPerChannel);
            wavIO_writeFrame(hWavIO_Imag, audioIOBuffer_Imag, nSamplesToWritePerChannelImag, &nSamplesWrittenPerChannelImag);
        }
        
#if MPEG_D_DRC_EXTENSION_V1
        /* loudness EQ is not applied to the audio signal here. This is up to the implementer */
        /* This implementation is limited to decode the loudness data only */
        if (hUniDrcConfig->uniDrcConfigExt.loudEqInstructionsPresent) {
            if (uniDrcSelProcOutput.selectedLoudEqId >= 0) {
                error = processLoudEq(hLoudnessEq, hUniDrcConfig, hUniDrcGain);
                if (error) goto Exit;
            }
        }
#endif
        frame++;
    }
    
#ifdef MEASURE_PROCESSING_TIME
    /* processing time */
    elapsed_time = ( (double) accuTime ) / CLOCKS_PER_SEC;
    elapsed_time_QMF = ( (double) accuTimeQMF ) / CLOCKS_PER_SEC;
    
    fprintf(stderr,"Processing times: DRC %.2f s, QMF = %.2f s, ALL = %.2f s.\n", (float) elapsed_time, (float) elapsed_time_QMF, (float) (elapsed_time+elapsed_time_QMF) );
#endif
    
Exit:
    /**********************************/
    /* CLOSE                          */
    /**********************************/
    
    if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
        if (hWavIO != NULL) {
            error = wavIO_updateWavHeader(hWavIO, &nTotalSamplesWrittenPerChannel);
            error = wavIO_close(hWavIO);
        }
    } else if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
        if (hWavIO_Real != NULL) {
            error = wavIO_updateWavHeader(hWavIO_Real, &nTotalSamplesWrittenPerChannel);
            error = wavIO_close(hWavIO_Real);
        }
        if (hWavIO_Imag != NULL) {
            error = wavIO_updateWavHeader(hWavIO_Imag, &nTotalSamplesWrittenPerChannel);
            error = wavIO_close(hWavIO_Imag);
        }
    }
    for (i=0; i<NUM_GAIN_DEC_INSTANCES; i++) {
        if (hGainDec[i] != NULL) {
            error = drcDecClose(&hGainDec[i],&hUniDrcConfig,&hLoudnessInfoSet,&hUniDrcGain
#if MPEG_D_DRC_EXTENSION_V1
                                , &hLoudnessEq
#endif
                                );
        }
    }
    error = closeUniDrcSelectionProcess(&hSelectionProc);
    error = closeUniDrcBitstreamDec(&hBitstreamDec,&hUniDrcConfig,&hLoudnessInfoSet,&hUniDrcGain);
    
    if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
        if (peakLimiterPresent)
        {
            free(audioInterleavedBuffer);
            audioInterleavedBuffer = NULL;
            destroyLimiter(hLimiter);
            hLimiter = NULL;
        }
    }
    
    error = closeUniDrcInterfaceDecoder(&hUniDrcIfDecStruct,&hUniDrcInterface);
    
    if (decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION)
    {
        
        for (i=0; i < numInCh; i++)
        {
            QMFlib_CloseAnaFilterbank(hQmfAna[i]);
            free(audioIOBuffer_Imag[i]);
            audioIOBuffer_Imag[i] = NULL;
            free(audioIOBuffer_Real[i]);
            audioIOBuffer_Real[i] = NULL;
        }
        free(audioIOBuffer_Imag);
        audioIOBuffer_Imag = NULL;
        free(audioIOBuffer_Real);
        audioIOBuffer_Real = NULL;
        for (i=0; i < numOutCh; i++)
        {
            QMFlib_CloseSynFilterbank(hQmfSyn[i]);
        }
        free(hQmfSyn);
        hQmfSyn = NULL;
        free(hQmfAna);
        hQmfAna = NULL;

#if AMD1_SYNTAX
        for (i=0; i < numOutCh; i++)
        {
            free(audioIOBufferDelayLine_Imag[i]);
            audioIOBufferDelayLine_Imag[i] = NULL;
            free(audioIOBufferDelayLine_Real[i]);
            audioIOBufferDelayLine_Real[i] = NULL;
        }
        free(audioIOBufferDelayLine_Imag);
        audioIOBufferDelayLine_Imag = NULL;
        free(audioIOBufferDelayLine_Real);
        audioIOBufferDelayLine_Real = NULL;
#endif
    }
    
    if (decType == DEC_TYPE_TD_INTERFACE_TD_APPLICATION || decType == DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION) {
        
        for (i=0; i< numInCh; i++)
        {
            free(audioIOBuffer[i]);
            audioIOBuffer[i] = NULL;
        }
        free(audioIOBuffer);
        audioIOBuffer = NULL;

#if AMD1_SYNTAX
        for (i=0; i< numOutCh; i++)
        {
            free(audioIOBufferDelayLine[i]);
            audioIOBufferDelayLine[i] = NULL;
        }
        free(audioIOBufferDelayLine);
        audioIOBufferDelayLine = NULL;
#endif
 
        free(bitstream_uniDrcConfig);
        bitstream_uniDrcConfig = NULL;
        free(bitstream_loudnessInfoSet);
        bitstream_uniDrcConfig = NULL;
        free(bitstream);
        bitstream = NULL;
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        free(bitstreamFfLoudness);
        bitstreamFfLoudness = NULL;
        free(bitstreamFfDrc);
        bitstreamFfDrc = NULL;
#endif
#endif
#endif
        
    }
    
    if (decType == DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION || decType == DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION) {
        for (i=0; i < numInCh; i++)
        {
            free(audioIOBuffer_Imag[i]);
            audioIOBuffer_Imag[i] = NULL;
            free(audioIOBuffer_Real[i]);
            audioIOBuffer_Real[i] = NULL;
        }
        free(audioIOBuffer_Imag);
        audioIOBuffer_Imag = NULL;
        free(audioIOBuffer_Real);
        audioIOBuffer_Real = NULL;

#if AMD1_SYNTAX
        for (i=0; i < numOutCh; i++)
        {
            free(audioIOBufferDelayLine_Imag[i]);
            audioIOBufferDelayLine_Imag[i] = NULL;
            free(audioIOBufferDelayLine_Real[i]);
            audioIOBufferDelayLine_Real[i] = NULL;
        }
        free(audioIOBufferDelayLine_Imag);
        audioIOBufferDelayLine_Imag = NULL;
        free(audioIOBufferDelayLine_Real);
        audioIOBufferDelayLine_Real = NULL;
#endif
    }
    
    return 0;
}

/***********************************************************************************/

static void PrintCmdlineHeader (void)
{
    fprintf( stdout, "\n");
    fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                          Dynamic Range Control Decoder                      *\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*                                %s                                  *\n", __DATE__);
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*    This software may only be used in the development of the MPEG-D Part 4:  *\n");
    fprintf( stdout, "*    Dynamic Range Control standard, ISO/IEC 23003-4 or in conforming         *\n");
    fprintf( stdout, "*    implementations or products.                                             *\n");
    fprintf( stdout, "*                                                                             *\n");
    fprintf( stdout, "*******************************************************************************\n");
    fprintf( stdout, "\n");
    
#if MPEG_H_SYNTAX
    fprintf(stdout,"!!! This tool is only meant for debugging of MPEG-D DRC bitstreams with MPEG-H 3DA syntax. !!!.\n");
    fprintf(stdout,"!!! Note that the processing chain and its output do NOT fulfill any MPEG-H 3DA conformance criteria, !!!.\n\n");
#endif
}

/***********************************************************************************/

static void PrintCmdlineHelp ( char* argv0 )
{
    fprintf ( stderr, "invalid arguments:\n" );
    fprintf ( stderr, "\ndrcToolDecoder Usage:\n\n" );
    fprintf ( stderr, "Example (TD interface): %s -if <input.wav> -ib <inBitstream.bs> (-ii >inInterfaceBitstream.bs>) -of <output.wav> -cp <idx> (-dt <idx>)\n", argv0);
    fprintf ( stderr, "Example (TD interface, splitted payload): %s -if <input.wav> -ic <inBitstreamConfig.bs> -il <inBitstreamLoudness.bs> -ig <inBitstreamGain.bs> (-ii >inInterfaceBitstream.bs>) -of <output.wav> -cp <idx> (-dt <idx>)\n", argv0);
    fprintf ( stderr, "Example (FD interface): %s -if <input> -ib <inBitstream.bs> (-ii <inInterfaceBitstream.bs>) -of <output> -cp <idx> -dt 2\n", argv0);
#if AMD1_SYNTAX
    fprintf ( stderr, "Additional parameters: (-dm <idx>) (-gd <int>) (-ad <int>) (-pl) (-cd) (-ado)\n");
#else
    fprintf ( stderr, "Additional parameters: (-dm <idx>) (-gd <int>) (-pl) (-ado)\n");
#endif
    fprintf ( stderr, "\n" );
    fprintf ( stderr, "-if\n" );
    fprintf ( stderr, "\t TD interface: Path to input wav\n" );
    fprintf ( stderr, "\t FD interface: Path to subband domain input\n" );
    fprintf ( stderr, "\t               Filename will be extended to <input>_real.qmf and <input>_imag.qmf or to <input>_real.stft and <input>_imag.stft dependent on -dt option\n" );
    fprintf ( stderr, "-ib\n" );
    fprintf ( stderr, "\t Path to input uniDrc-bitstream\n" );
    fprintf ( stderr, "-ic\n" );
    fprintf ( stderr, "\t Path to input uniDrcConfig-bitstream\n" );
    fprintf ( stderr, "-il\n" );
    fprintf ( stderr, "\t Path to input loudnessInfoSet-bitstream\n" );
    fprintf ( stderr, "-ig\n" );
    fprintf ( stderr, "\t Path to input uniDrcGain-bitstream (byte alignment expected after each uniDrcGain() payload)\n" );
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    fprintf ( stderr, "-ffLoudness\n" );
    fprintf ( stderr, "\t Path to input file with ['ludt'] box according to ISO/IEC 14496-12 (ISOBMFF)\n" );
    fprintf ( stderr, "-ffDrc\n" );
    fprintf ( stderr, "\t Path to input file with ['chnl', 'dmix', 'udc2', 'udi2', 'udex'] boxes according to ISO/IEC 14496-12 (ISOBMFF)\n" );
#endif
#endif
#endif
    fprintf ( stderr, "-ii\n" );
    fprintf ( stderr, "\t Path to input uniDrcInterface-bitstream\n" );
    fprintf ( stderr, "-of\n" );
    fprintf ( stderr, "\t TD interface: Path to time domain output wav\n" );
    fprintf ( stderr, "\t FD interface: Path to subband domain output\n" );
    fprintf ( stderr, "\t               Filename will be extended to <input>_real.qmf and <input>_imag.qmf or to <input>_real.stft and <input>_imag.stft dependent on -dt option\n" );
    fprintf ( stderr, "-cp\n" );
#if MPEG_H_SYNTAX
    fprintf ( stderr, "\t Index of control parameter set [1...6]\n" );
#else
    fprintf ( stderr, "\t Index of control parameter set [1...39]\n" );
#endif
    fprintf ( stderr, "-dt\n" );
    fprintf ( stderr, "\t Index of decoding type (0 = TD interface, TD application; 1 = TD interface, QMF64 application; 2 = QMF64 interface, QMF64 application,\n" );
    fprintf ( stderr, "\t                         3 = STFT256 interface, STFT256 application (see ISO/IEC 23008-3:2015/AMD3))\n" );

    fprintf ( stderr, "-dm\n" );
    fprintf ( stderr, "\t Index of delay mode for gain value decoding (0 = default delay mode; 1 = low-delay mode)\n" );
    fprintf ( stderr, "-gd\n" );
    fprintf ( stderr, "\t Additional gain delay in samples (default: 0 samples).\n" );
#if AMD1_SYNTAX
    fprintf ( stderr, "-ad\n" );
    fprintf ( stderr, "\t Additional audio delay in samples (default: 0 samples). Note that this option serves debugging purposes only.\n" );
#endif /* AMD1_SYNTAX */
#if AMD2_COR2
    fprintf ( stderr, "-tl\n" );
    fprintf ( stderr, "\t target Loudness [LKFS]. Can be used optionally with \"-cp\". Cannot be used with \"-ii\".\n");
    fprintf ( stderr, "-drcEffect\n" );
    fprintf ( stderr, "\t DRC effect request index according to Table 11 of ISO/IEC 23003-4:2015 in hexadecimal format.\n");
    fprintf ( stderr, "\t Least significant hexadecimal number declares desired request type, others declare fallback request types.\n");
    fprintf ( stderr, "\t Can be used optionally with \"-cp\". Cannot be used with \"-ii\".\n");
#endif
    fprintf ( stderr, "-pl\n" );
    fprintf ( stderr, "\t Flag: Enable peak limiter (only applicable for TD interface)\n" );
#if AMD1_SYNTAX
    fprintf ( stderr, "-cd\n" );
    fprintf ( stderr, "\t Flag: Constant delay mode for dynamic DRC/EQ set switching.\n" );
#endif
    fprintf ( stderr, "-ado\n" );
    fprintf ( stderr, "\t Flag: Absorb delay off. Note that this option serves debugging purposes only.\n" );
    fprintf ( stderr, "-af\n" );
    fprintf ( stderr, "\t Audio frame size (optional, default = 1024)\n" );
    fprintf ( stderr, "-v\n" );
    fprintf ( stderr, "\t Index of verbose mode: 0 = short, 1 = default, 2 = print all\n" );
}

/***********************************************************************************/

static int GetCmdline ( int argc, char** argv )
{
    int i;
    int required = 0;
    
    for ( i = 1; i < argc; ++i )
    {
        if ( !strcmp ( argv[i], "-if" ) )  	/* Required */
        {
            int nWavExtensionChars = 0;
            strncpy ( wavFilenameInput, argv[i + 1], FILENAME_MAX ) ;
            nWavExtensionChars = (int)strspn(".wav",wavFilenameInput);
            if ( nWavExtensionChars != 4 ) {
                subbandDomainIOFlag = subbandDomainIOFlag|1;
                strcpy(sbFilenameInputReal, wavFilenameInput);
                strcpy(sbFilenameInputImag, wavFilenameInput);
            }
            i++;
            required++;
            continue;
        }
        if ( !strcmp ( argv[i], "-ic" ) )  	/* Required */
        {
            bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy ( bitstreamFilenameInput_uniDrcConfig, argv[i + 1], FILENAME_MAX ) ;
#if AMD2_COR2
            uniDrcConfigBitstreamPresent = 1;
#endif
            i++;
            required++;
            continue;
        }
        if ( !strcmp ( argv[i], "-il" ) )  	/* Required */
        {
            bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy ( bitstreamFilenameInput_loudnessInfoSet, argv[i + 1], FILENAME_MAX ) ;
#if AMD2_COR2
            loudnessInfoSetBitstreamPresent = 1;
#endif
            i++;
            required++;
            continue;
        }
        if ( !strcmp ( argv[i], "-ig" ) )  	/* Required */
        {
            bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
            strncpy ( bitstreamFilenameInput_uniDrcGain, argv[i + 1], FILENAME_MAX ) ;
#if AMD2_COR2
            uniDrcGainBitstreamPresent = 1;
#endif
            i++;
            required++;
            continue;
        }
        if ( !strcmp ( argv[i], "-ib" ) )  	/* Required */
        {
            strncpy ( bitstreamFilenameInput_uniDrc, argv[i + 1], FILENAME_MAX ) ;
            i++;
            required++;
            continue;
        }
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        if ( !strcmp ( argv[i], "-ffLoudness" ) )      /* optional */
        {
            strncpy ( bitstreamFilenameInput_ffLoudness, argv[i + 1], FILENAME_MAX ) ;
            i++;
            required++;
            ffLoudnessPresent = 1;
            continue;
        }
        if ( !strcmp ( argv[i], "-ffDrc" ) )      /* optional */
        {
            strncpy ( bitstreamFilenameInput_ffDrc, argv[i + 1], FILENAME_MAX ) ;
            i++;
            required++;
            ffDrcPresent = 1;
            continue;
        }
#endif
#endif
#endif
        if ( !strcmp ( argv[i], "-ii" ) )  	/* Either -ii or -cp required */
        {
            strncpy ( interfaceFilenameInput, argv[i + 1], FILENAME_MAX ) ;
            interfaceBitstreamPresent = 1;
            i++;
            required++;
            continue;
        }
        else if ( !strcmp ( argv[i], "-of" ) ) /* Required */
        {
            int nWavExtensionChars = 0;
            strncpy ( wavFilenameOutput, argv[i + 1], FILENAME_MAX ) ;
            nWavExtensionChars = (int)strspn(".wav",wavFilenameOutput);
            if ( nWavExtensionChars != 4 ) {
                subbandDomainIOFlag = subbandDomainIOFlag|2;
                strcpy(sbFilenameOutputReal, wavFilenameOutput);
                strcpy(sbFilenameOutputImag, wavFilenameOutput);
            }
            i++;
            required++;
            continue;
        }
        else if ( !strcmp ( argv[i], "-cp" ) ) /* Either -ii or -cp required */
        {
            controlParameterIndex = atoi( argv[i + 1] ) ;
            i++;
            required++;
            continue;
        }
        else if ( !strcmp ( argv[i], "-dt" ) ) /* optional */
        {
            int tmp = atoi( argv[i + 1] ) ;
            switch (tmp) {
                case 1:
                    decType = DEC_TYPE_TD_INTERFACE_QMF64_APPLICATION;
                    subBandDomainMode = SUBBAND_DOMAIN_MODE_QMF64;
                    subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
                    subBandCount = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
                    break;
                case 2:
                    decType = DEC_TYPE_QMF64_INTERFACE_QMF64_APPLICATION;
                    subBandDomainMode = SUBBAND_DOMAIN_MODE_QMF64;
                    subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
                    subBandCount = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
                    break;
                case 3:
                    decType = DEC_TYPE_STFT256_INTERFACE_STFT256_APPLICATION;
                    subBandDomainMode = SUBBAND_DOMAIN_MODE_STFT256;
                    subBandDownSamplingFactor = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
                    subBandCount = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
                    break;
                case 0:
                default:
                    decType = DEC_TYPE_TD_INTERFACE_TD_APPLICATION;
                    subBandDomainMode = SUBBAND_DOMAIN_MODE_OFF;
                    break;
            }
            i++;
            continue;
        }
        else if ( !strcmp ( argv[i], "-gd" ) ) /* optional */
        {
            int tmp = atoi( argv[i + 1] ) ;
            if (tmp>=0) {
                gainDelaySamples = tmp;
            } else {
                fprintf ( stderr, "invalid parameter: gain delay has to be a positive integer\n" );
                return -1;
            }
            i++;
            continue;
        }
#if AMD1_SYNTAX
        else if ( !strcmp ( argv[i], "-ad" ) ) /* optional */
        {
            int tmp = atoi( argv[i + 1] ) ;
            if (tmp>=0) {
                audioDelaySamples = tmp;
            } else {
                fprintf ( stderr, "invalid parameter: audio delay has to be a positive integer\n" );
                return -1;
            }
            i++;
            continue;
        }
#endif /* AMD1_SYNTAX */
        else if ( !strcmp ( argv[i], "-pl" ) ) /* Optional */
        {
            peakLimiterPresent = 1;
            continue;
        }
        else if (!strcmp(argv[i],"-dm")) /* Optional */
        {
            int tmp = atoi( argv[i + 1] );
            switch (tmp) {
                case 1:
                    delayMode = 1;
                    break;
                case 0:
                default:
                    delayMode = 0;
                    break;
            }
            i++;
        }
#if AMD1_SYNTAX
        else if (!strcmp(argv[i],"-cd")) /* Optional */
        {
            constantDelayOn = 1;
            continue;
        }
#endif
        else if (!strcmp(argv[i],"-ado")) /* Optional */
        {
            absorbDelayOn = 0;
            continue;
        }
        else if (!strcmp(argv[i],"-af")) /* Optional */
        {
            audioFrameSize = atoi( argv[i + 1] ) ;
#if AMD2_COR2
            i++;
#endif
            continue;
        }
#if AMD2_COR2
        else if (!strcmp(argv[i],"-tl")) /* optional */
        {
            targetLoudness = (float)atof(argv[i+1]);
            targetLoudnessPresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-drcEffect")) /* optional */
        {
            drcEffectTypeRequest = (unsigned long)strtol(argv[i+1],NULL,16);
            drcEffectTypeRequestPresent = 1;
            i++;
        }
#endif
    }
#if AMD2_COR3
    if ((bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT && required != 4 && required != 6) && !(targetLoudnessPresent == 1 || drcEffectTypeRequestPresent == 1) ||
        (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT && required != 6 && !(required == 4 && loudnessInfoSetBitstreamPresent == 1)) && !(targetLoudnessPresent == 1 || drcEffectTypeRequestPresent == 1) ||
        ((targetLoudnessPresent == 1 || drcEffectTypeRequestPresent == 1) && interfaceBitstreamPresent == 1)
       )
#elif AMD2_COR2
    if ((bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT && required != 4) || (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT && required != 6 && !(required == 4 && loudnessInfoSetBitstreamPresent == 1)))
#else
    if ((bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT && required != 4) || (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT && required != 6))
#endif
    {
        PrintCmdlineHelp( argv[0] );
        return -1;
    }
    else
    {
        return 0;
    }
}

/***********************************************************************************/

static int ApplyDownmix ( UniDrcSelProcOutput uniDrcSelProcOutput, float** audioIOBuffer, int frameSize, int numOutCh )
{
    int baseChCnt = uniDrcSelProcOutput.baseChannelCount;
    int targetChCnt = uniDrcSelProcOutput.targetChannelCount;
    int n, ic, oc;
    float tmp_out[CHANNEL_COUNT_MAX];
    
    if (uniDrcSelProcOutput.downmixMatrixPresent == 0)
        return 0;  /* no downmix */
    
    if (audioIOBuffer == NULL)
        return 0;
    
    if (targetChCnt > CHANNEL_COUNT_MAX)
        return -1;
    
    if (targetChCnt > baseChCnt)
        return -1;
    
    if (targetChCnt > numOutCh)
        return -1;
    
    /* in-place downmix */
    for (n=0; n<frameSize; n++) {
        for (oc=0; oc<targetChCnt; oc++) {
            tmp_out[oc] = 0.0f;
            for (ic=0; ic<baseChCnt; ic++) {
                tmp_out[oc] += audioIOBuffer[ic][n] * uniDrcSelProcOutput.downmixMatrix[ic][oc];
            }
        }
        for (oc=0; oc<targetChCnt; oc++) {
            audioIOBuffer[oc][n] = tmp_out[oc];
        }
        for ( ; oc<baseChCnt; oc++) {
            audioIOBuffer[oc][n] = 0.0f;
        }
    }
    
    
    return 0;
}

/***********************************************************************************/

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

/***********************************************************************************/

static int readInterfaceFile(HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct, char* inFilename, HANDLE_UNI_DRC_INTERFACE hUniDrcInterface)
{
    int err             = 0;
    int nBitsRead       = 0;
    int nBytesBitstream = 0;
    int nBitsBitstream  = 0;
    
    unsigned char* bitstream                        = NULL;
    FILE *pInFile                                   = NULL;
    
    pInFile = fopen ( inFilename , "rb" );
    if (pInFile == NULL) {
        fprintf(stderr, "Error opening input file %s\n", inFilename);
        return -1;
    }
    
    fseek( pInFile , 0L , SEEK_END);
    nBytesBitstream = (int)ftell( pInFile );
    rewind( pInFile );
    
    bitstream = calloc( 1, nBytesBitstream );
    if( !bitstream ) {
        fclose(pInFile);
        fputs("memory alloc fails",stderr);
        exit(1);
    }
    
    /* copy the file into the buffer */
    if( 1!=fread( bitstream , nBytesBitstream, 1 , pInFile) ) {
        fclose(pInFile);
        free(bitstream);
        fputs("entire read fails",stderr);
        exit(1);
    }
    fclose(pInFile);
    
    nBitsBitstream = nBytesBitstream * 8;
    
    err = initUniDrcInterfaceDecoder(hUniDrcIfDecStruct);
    if (err) return(err);
    
    /* decode bitstream */
    err = processUniDrcInterfaceDecoder(hUniDrcIfDecStruct,
                                        hUniDrcInterface,
                                        bitstream,
                                        nBitsBitstream,
                                        &nBitsRead);
    if (err) return(err);
    
#if AMD2_COR2
    if(hUniDrcInterface->loudnessNormalizationParameterInterfacePresent == 0 && peakLimiterPresent == 1) {
        hUniDrcInterface->loudnessNormalizationParameterInterfacePresent = 1;
        memset(&hUniDrcInterface->loudnessNormalizationParameterInterface, 0, sizeof(LoudnessNormalizationParameterInterface));
        hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent = peakLimiterPresent;
        hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax = 1;
        hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax = 6.f;
    }
    else {
        if(hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent == 1) {
           if(hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax == 0) {
                hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax = 1;
                hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax = 6.f;
            }
        }
        else {
            if(peakLimiterPresent == 1) {
                hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent = peakLimiterPresent;
                if(hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax == 0) {
                    hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax = 1;
                    hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax = 6.f;
                }
            }
        }
    }
#endif
    
    free(bitstream);
    
    /***********************************************************************************/
    
    return 0;
}

static int copyBitstreamFileToBuffer(char* bitstreamFilename, unsigned char** pBitstreamBuffer, int* pNBytesBs)
{
    FILE *pBsFile                 = NULL;
    unsigned char* bitstream      = NULL;
    int nBytesBs                  = 0;
    
    /* Open input bitstream file */
    if (strlen(bitstreamFilename)) {
        pBsFile = fopen ( bitstreamFilename , "rb" );
    }
    if (pBsFile == NULL) {
        fprintf(stderr, "Could not open input bitstream file %s\n", bitstreamFilename);
        return -1;
    }
    
    fseek( pBsFile , 0L , SEEK_END);
    nBytesBs = (int)ftell( pBsFile );
    rewind( pBsFile );
    
    bitstream = calloc( 1, nBytesBs );
    if( !bitstream ) {
        fclose(pBsFile);
        fputs("memory alloc fails",stderr);
        return -1;
    }
    
    /* copy the file into the buffer */
    if( 1!=fread( bitstream , nBytesBs, 1 , pBsFile) ) {
        fclose(pBsFile);
        free(bitstream);
        fputs("entire read fails",stderr);
        return -1;
    }
    fclose(pBsFile);
    
    *pBitstreamBuffer = bitstream;
    *pNBytesBs = nBytesBs;
    
    return 0;
}

#if AMD2_COR2
/***********************************************************************************/

int setDrcEffectTypeRequest (unsigned long drcEffectTypeRequestPlusFallbacks, int* numDrcEffectTypeRequests, int* numDrcEffectTypeRequestsDesired, int* drcEffectTypeRequest)
{
    int i = 0, drcEffectTypeRequested = 0;
    unsigned long tmp = drcEffectTypeRequestPlusFallbacks;
    *numDrcEffectTypeRequests        = 0;
    *numDrcEffectTypeRequestsDesired = 1;
    for (i=0; i<8; i++) {
        drcEffectTypeRequested = tmp & 15;
        if (i == 0 || drcEffectTypeRequested != 0) {
            *numDrcEffectTypeRequests += 1;
            drcEffectTypeRequest[i] = drcEffectTypeRequested;
        } else {
            break;
        }
        tmp >>= 4;
    }
    
    return 0;
}

#endif
