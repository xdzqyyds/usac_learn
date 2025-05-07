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
#include <math.h>

#include "uniDrcSelectionProcess_main.h"
#include "uniDrcBitstreamDecoder_api.h"
#include "uniDrcInterfaceDecoder_api.h"
#include "uniDrcSelectionProcess_api.h"
#include "uniDrcHostParams.h"

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN
#endif

typedef enum _BITSTREAM_FILE_FORMAT
{
    BITSTREAM_FILE_FORMAT_DEFAULT = 0,
    BITSTREAM_FILE_FORMAT_SPLIT   = 1
} BITSTREAM_FILE_FORMAT;

int readDmxSelIdxFromFile( char* filename, int* downmixIdRequested);
int setDrcEffectTypeRequest (unsigned long drcEffectTypeRequestPlusFallbacks, int* numDrcEffectTypeRequests, int* numDrcEffectTypeRequestsDesired, int* drcEffectTypeRequest);
#if MPEG_H_SYNTAX
static int readDownmixMatrixSetFromFile( char* filename, int* downmixId, int* targetChannelCountFromDownmixId, int* downmixIdCount);
static int readMpegh3daParametersFromFile( char* filename, int* groupIdRequested, int* numGroupIdsRequested, int* groupPresetIdRequested, int* numMembersGroupPresetIdsRequested, int* numGroupPresetIdsRequested, int* groupPresetIdRequestedPreference);
#endif

int main(int argc, char *argv[])
{
    
#if DEBUG_DRC_SELECTION_CMDL
    int loopIdx;
    int startIdx;
    int stopIdx;
    int selectedDrcSetsIdentical;
    if (testIdx == 0) {
        startIdx = 0;
        stopIdx = NUM_TESTS;
    } else {
        startIdx = testIdx-1;
        stopIdx = testIdx;
    }
    for (loopIdx=startIdx; loopIdx<stopIdx; loopIdx++) {
        char inputSignal[256];
        char inputSignal2[256];
        char inputSignal3[256];
        char inputInterface[256];
        char outputSignal[256];
        char outputSignal2[256];
#endif
        
        unsigned int i, j;
        int err;
        int required            = 0;
        int plotInfo            = 1;
        BITSTREAM_FILE_FORMAT bitstreamFileFormat = BITSTREAM_FILE_FORMAT_DEFAULT;
        int delayMode           = 0;
        int audioSampleRate     = 48000;
        int audioFrameSize      = 1024;
        int helpFlag            = 0;
        int multiBandDrcPresent[3] = {0};
        int numSets[3]          = {0};
#if MPEG_H_SYNTAX
        int downmixIdCount       = 0;
        int downmixId[128]       = {0};
        int targetChannelCountFromDownmixId[128] = {0};
        int numGroupIdsRequested = 0;
        int groupIdRequested[MAX_NUM_GROUP_ID_REQUESTS] = {0};
        int numGroupPresetIdsRequested = 0;
        int groupPresetIdRequested[MAX_NUM_GROUP_PRESET_ID_REQUESTS] = {0};
        int numMembersGroupPresetIdsRequested[MAX_NUM_MEMBERS_GROUP_PRESET] = {0};
        int groupPresetIdRequestedPreference = -1;
#endif
        /* uniDrc/uniDrcConfig bitstream */
        unsigned char* bitstream = NULL;
        int nBytes               = 0;
        int nBitsOffset          = 0;
        int nBitsRead            = 0;
        
        /* loudnessInfoSet bitstream */
        unsigned char* bitstreamLoudness = NULL;
        int nBytesLoudness               = 0;
        int nBitsReadLoudness            = 0;
        int nBitsOffsetLoudness          = 0;
        
        /* uniDrcInterface bitstream */
        unsigned char* bitstreamInterface = NULL;
        int nBitsReadInterface            = 0;
        int nBytesInterfaceBitstream      = 0;
        int nBitsInterfaceBitstream       = 0;
        
        /* handles */
        HANDLE_UNI_DRC_BS_DEC_STRUCT hUniDrcBsDecStruct     = NULL;
        HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct     = NULL;
        HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct = NULL;
        HANDLE_UNI_DRC_GAIN hUniDrcGain                     = NULL;
        HANDLE_UNI_DRC_CONFIG hUniDrcConfig                 = NULL;
        HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet           = NULL;
        HANDLE_UNI_DRC_INTERFACE hUniDrcInterface           = NULL;
        HANDLE_UNI_DRC_SEL_PROC_PARAMS hUniDrcSelProcParams = NULL;
        HANDLE_UNI_DRC_SEL_PROC_OUTPUT hUniDrcSelProcOutput = NULL;
        
        /*structs */
        UniDrcSelProcParams uniDrcSelProcParams = {0};
        UniDrcSelProcOutput uniDrcSelProcOutput = {0};
        
        int peakLimiterPresent             = -1;
        int downmixIdRequested             = -1;
        int controlParameterIndex          = -1;
        int targetLoudnessPresent          = -1;
        float targetLoudness               = UNDEFINED_LOUDNESS_VALUE;
        int drcEffectTypeRequestPresent    = -1;
        unsigned long drcEffectTypeRequest = 0;
        
        /* strings */
        char* inFilenameBitstream          = NULL;
        char* inFilenameLoudnessBitstream  = NULL;
#if MPEG_H_SYNTAX
        char* inFilenameDownmixMatrixSet   = NULL;
        char* inFilenameMpegh3daParams     = NULL;
#endif
        char* inFilenameInterfaceBitstream = NULL;
        char* inFilenameDownmixId          = NULL;
        char* outFilenameTransferData      = NULL;
        char* outFilenameDmxMtx            = NULL;
        char* outFilenamemultiBandDrcPresent  = NULL;
        char* txtExtension                 = ".txt";
        
        /* file pointers */
        FILE *pInFileBitstream             = NULL;
        FILE *pInFileBitstreamLoudness     = NULL;
        FILE *pInFileInterfaceBitstream    = NULL;
        FILE *pOutFileTransferData         = NULL;
        FILE *pOutFileDmxMtx               = NULL;
        FILE *pOutFilemultiBandDrcPresent     = NULL;
        
        for ( i = 1; i < (unsigned int) argc; ++i )
        {
            if (!strcmp(argv[i],"-v")) /* Optional */
            {
                plotInfo = atoi(argv[i+1]);
                i++;
            }
        }
        
        if (plotInfo!=0) {
            fprintf( stdout, "\n");
            fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
            fprintf( stdout, "*                                                                             *\n");
            fprintf( stdout, "*                        Unified DRC Selection Process                        *\n");
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
            fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Selection Process (RM11).\n\n");
#else
            fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Selection Process (RM11).\n\n");
#endif
#else
            fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Selection Process (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
#endif
        }
        
        /***********************************************************************************/
        /* ----------------- CMDL PARSING -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark CMDL_PARSING
#endif
        
#if !DEBUG_DRC_SELECTION_CMDL
        
        /* Commandline Parsing */
        for ( i = 1; i < (unsigned int) argc; ++i )
        {
            if (!strcmp(argv[i],"-if"))      /* required */
            {
                if ( argv[i+1] ) {
                    inFilenameBitstream = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for bitstream given.\n");
                    return -1;
                }
                required ++;
                i++;
            }
            else if (!strcmp(argv[i],"-ic"))      /* required */
            {
                bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
                if ( argv[i+1] ) {
                    inFilenameBitstream = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for config bitstream given.\n");
                    return -1;
                }
                required++;
                i++;
            }
            else if (!strcmp(argv[i],"-il"))      /* required */
            {
                bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
                if ( argv[i+1] ) {
                    inFilenameLoudnessBitstream = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for loudness bitstream given.\n");
                    return -1;
                }
                required++;
                i++;
            }
            else if (!strcmp(argv[i],"-ii"))      /* optional */
            {
                if ( argv[i+1] ) {
                    inFilenameInterfaceBitstream = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for interface bitstream given.\n");
                    return -1;
                }
                i++;
            }
            else if (!strcmp(argv[i],"-of")) /* required */
            {
                unsigned long nTxtExtensionChars = 0;
                
                if ( argv[i+1] ) {
                    outFilenameTransferData = argv[i+1];
                }
                else {
                    fprintf(stderr, "No output filename for selection process transfer data given.\n");
                    return -1;
                }
                nTxtExtensionChars = strspn(txtExtension,outFilenameTransferData);
                
                if ( nTxtExtensionChars != 4 ) {
                    fprintf(stderr, "TXT file extension missing for selection process transfer output.\n");
                    return -1;
                }
                required++;
                i++;
            }
            else if (!strcmp(argv[i],"-id")) /* optional */
            {
                downmixIdRequested = (int)atof(argv[i+1]);
                i++;
            }
            else if (!strcmp(argv[i],"-dmx")) /* optional */
            {
                unsigned long nTxtExtensionChars = 0;
                
                if ( argv[i+1] ) {
                    outFilenameDmxMtx = argv[i+1];
                }
                else {
                    fprintf(stderr, "No output filename for downmix matrix given.\n");
                    return -1;
                }
                nTxtExtensionChars = strspn(txtExtension,outFilenameDmxMtx);
                
                if ( nTxtExtensionChars != 4 ) {
                    fprintf(stderr, "TXT file extension missing for downmix matrix output.\n");
                    return -1;
                }
                i++;
            }
            else if (!strcmp(argv[i],"-multiBandDrcPresent")) /* optional */
            {
                unsigned long nTxtExtensionChars = 0;
                
                if ( argv[i+1] ) {
                    outFilenamemultiBandDrcPresent = argv[i+1];
                }
                else {
                    fprintf(stderr, "No output filename for multiband present given.\n");
                    return -1;
                }
                nTxtExtensionChars = strspn(txtExtension,outFilenamemultiBandDrcPresent);
                
                if ( nTxtExtensionChars != 4 ) {
                    fprintf(stderr, "TXT file extension missing for multiband present output.\n");
                    return -1;
                }
                i++;
            }
            else if (!strcmp(argv[i],"-cp")) /* optional */
            {
                controlParameterIndex = (int)atof(argv[i+1]);
                i++;
            }
            else if (!strcmp(argv[i],"-pl")) /* optional */
            {
                peakLimiterPresent = (int)atof(argv[i+1]);
                i++;
            }
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
#if MPEG_H_SYNTAX
            else if (!strcmp(argv[i],"-dms"))      /* required */
            {
                if ( argv[i+1] ) {
                    inFilenameDownmixMatrixSet = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for downmixMatrixSet() parameters given.\n");
                    return -1;
                }
                required++;
                i++;
            }
            else if (!strcmp(argv[i],"-mpegh3daParams"))      /* optional */
            {
                if ( argv[i+1] ) {
                    inFilenameMpegh3daParams = argv[i+1];
                }
                else {
                    fprintf(stderr, "No input filename for MPEG-H 3DA parameters given.\n");
                    return -1;
                }
                i++;
            }
#endif
            else if (!strcmp(argv[i],"-type")) /* Optional */
            {
              /* type = atoi(argv[i+1]); */ /* ignore as long as used in MPEG-H 3DA refsoft */
              i++;
            }
            else if (!strcmp(argv[i],"-v")) /* Optional */
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
        
#if MPEG_H_SYNTAX
        if (required != 4 || bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
            helpFlag = 1;
        }
#else
        if ((bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT && required != 2) || (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT && required != 3)) {
            helpFlag = 1;
        }
#endif
        
        if ( (inFilenameBitstream == NULL) || helpFlag || (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT && inFilenameLoudnessBitstream == NULL))
        {
            if (!helpFlag) {
                fprintf( stderr, "Invalid input arguments!\n\n");
            }
            fprintf(stderr, "uniDrcSelectionProcessCmdl Usage:\n\n");
#if MPEG_H_SYNTAX
            fprintf(stderr, "Example 1: <uniDrcSelectionProcessCmdl -ic uniDrcConfig.bs il loudnessInfoSet.bs -dms dms.txt -of drcTransferData.txt -cp 1 -info 0>\n");
            fprintf(stderr, "Example 2: <uniDrcSelectionProcessCmdl -ic uniDrcConfig.bs il loudnessInfoSet.bs -dms dms.txt -ii interface.bs -of drcTransferData.txt -pl 1 -info 2>\n");
#else
            fprintf(stderr, "Example 1: <uniDrcSelectionProcessCmdl -if input.bs -of drcTransferData.txt -cp 1 -info 0>\n");
            fprintf(stderr, "Example 2: <uniDrcSelectionProcessCmdl -if input.bs -ii interface.bs -of drcTransferData.txt -pl 1 -info 2>\n");
            fprintf(stderr, "Example 3: <uniDrcSelectionProcessCmdl -ic uniDrcConfig.bs il loudnessInfoSet.bs -ii interface.bs -of drcTransferData.txt -pl 1 -info 2>\n\n");
#endif

#if MPEG_H_SYNTAX
            fprintf ( stderr, "-ic\n" );
            fprintf ( stderr, "\t input bitstream (uniDrcConfig() syntax according to ISO/IEC 23008-3:2015).\n");
            fprintf ( stderr, "-il\n" );
            fprintf ( stderr, "\t input bitstream (loudnessInfoSet() syntax according to ISO/IEC 23008-3:2015).\n");
            fprintf ( stderr, "-dms\n" );
            fprintf ( stderr, "\t input txt for downmixId related parameters provided by MPEG-H 3DA DownmixMatrixSet() syntax (downmixIdCount, downmixId, targetChannelCountFromDownmixId).\n");
            fprintf ( stderr, "-mpegh3daParams\n" );
            fprintf ( stderr, "\t input txt for selection process parameters implicitly provided by MPEG-H 3DA decoder.\n");
#else
            fprintf ( stderr, "-if\n" );
            fprintf ( stderr, "\t input bitstream (uniDrc() syntax according to ISO/IEC 23003-4).\n");
            fprintf ( stderr, "-ic\n" );
            fprintf ( stderr, "\t input bitstream (uniDrcConfig() syntax according to ISO/IEC 23003-4).\n");
            fprintf ( stderr, "-il\n" );
            fprintf ( stderr, "\t input bitstream (loudnessInfoSet() syntax according to ISO/IEC 23003-4).\n");
#endif
            fprintf ( stderr, "-ii\n" );
            fprintf ( stderr, "\t interface bitstream (uniDrcInterface() syntax according to ISO/IEC 23003-4).\n");
            fprintf ( stderr, "-of\n" );
            fprintf ( stderr, "\t loudness normalization gain, peak value, and selected DRC sets as txt file output.\n");
            fprintf ( stderr, "-dmx\n" );
            fprintf ( stderr, "\t active downmix matrix as txt file (format: [inChans x outChans], linear gain).\n");
            fprintf ( stderr, "-cp\n" );
            fprintf ( stderr, "\t index of control parameter set\n");
            fprintf ( stderr, "-id\n" );
            fprintf ( stderr, "\t requested downmixId.\n");
            fprintf ( stderr, "-tl\n" );
            fprintf ( stderr, "\t target Loudness [LKFS].\n");
            fprintf ( stderr, "-drcEffect\n" );
            fprintf ( stderr, "\t DRC effect request index according to Table 11 of ISO/IEC 23003-4:2015 in hexadecimal format.\n");
            fprintf ( stderr, "\t Least significant hexadecimal number declares desired request type, others declare fallback request types.\n");
            fprintf ( stderr, "-pl\n" );
            fprintf ( stderr, "\t peak limiter present flag (0: not present (default), 1: present).\n");
            fprintf ( stderr, "-v\n" );
            fprintf ( stderr, "\t verbose output (0: nearly silent, 1: print minimal information (default), 2: print all information).\n");
            return -1;
        }
        
#else /* DEBUG_DRC_SELECTION_CMDL */
        
#ifdef __APPLE__
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
            sprintf (inputSignal, "../../../../drcTool/drcToolEncoder/TestDataOut/DrcGain%d.bit",filenameAndRequestIdx[loopIdx][0]);
        } else {
            sprintf (inputSignal, "../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig%d.bit",filenameAndRequestIdx[loopIdx][0]);
            sprintf (inputSignal3, "../../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet%d.bit",filenameAndRequestIdx[loopIdx][0]);
        }
#else
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
            sprintf (inputSignal, "../../../drcTool/drcToolEncoder/TestDataOut/DrcGain%d.bit",filenameAndRequestIdx[loopIdx][0]);
        } else {
            sprintf (inputSignal, "../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daUniDrcConfig%d.bit",filenameAndRequestIdx[loopIdx][0]);
            sprintf (inputSignal3, "../../../drcTool/drcToolEncoder/TestDataOut/mpegh3daLoudnessInfoSet%d.bit",filenameAndRequestIdx[loopIdx][0]);
        }
#endif
        inFilenameBitstream = inputSignal;
        inFilenameLoudnessBitstream = inputSignal3;
        
#ifdef __APPLE__
        strcpy ( inputSignal2, "../../../uniDrcInterfaceEncoderCmdl/outputData/test.bit" );
#else
        strcpy ( inputSignal2, "../../uniDrcInterfaceEncoderCmdl/outputData/test.bit" );
#endif
        /*inFilenameInterfaceBitstream = inputSignal2;*/
        
#ifdef __APPLE__
        sprintf (outputSignal, "../../outputData/selProcTransferData%d.txt",loopIdx+1);
#else
        sprintf (outputSignal, "../outputData/selProcTransferData%d.txt",loopIdx+1);
#endif
        outFilenameTransferData = outputSignal;
        
#ifdef __APPLE__
        strcpy ( outputSignal2, "../../outputData/DrcGain1_matrix.txt" );
#else
        strcpy ( outputSignal2, "../outputData/DrcGain1_matrix.txt" );
#endif
        /* outFilenameDmxMtx = outputSignal2; */
        
        peakLimiterPresent = -1;
        plotInfo = 2;
        controlParameterIndex = filenameAndRequestIdx[loopIdx][1];
        
        /*
         controlParameterIndex = loopIdx;
         sprintf (inputInterface, "../../../uniDrcInterfaceEncoderCmdl/outputData/uniDrcInterface%d.bit",filenameAndRequestIdx[loopIdx][1]);
         inFilenameInterfaceBitstream = inputInterface;
         */
        
#endif
        
        /***********************************************************************************/
        /* ----------------- REQUEST CONFIG -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark REQUEST_CONFIG
#endif
        
        if (controlParameterIndex > 0) {
            err = setDefaultParams_selectionProcess(&uniDrcSelProcParams);
            if (err) return (err);
            err = setCustomParams_selectionProcess(controlParameterIndex,&uniDrcSelProcParams);
            if (err) return (err);
            err = evaluateCustomParams_selectionProcess(&uniDrcSelProcParams);
            if (err) return (err);
            hUniDrcSelProcParams = &uniDrcSelProcParams;
        } else {
           err = setDefaultParams_selectionProcess(&uniDrcSelProcParams);
           if (err) return (err);
           hUniDrcSelProcParams = &uniDrcSelProcParams;
        }
        
        if (peakLimiterPresent > -1) {
            uniDrcSelProcParams.peakLimiterPresent = peakLimiterPresent;
            if (uniDrcSelProcParams.peakLimiterPresent == TRUE) {
#if !MPEG_H_SYNTAX
                uniDrcSelProcParams.outputPeakLevelMax = 6.0f;
#endif
            }
        }
        
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
            err = setDrcEffectTypeRequest (drcEffectTypeRequest, &uniDrcSelProcParams.numDrcEffectTypeRequests[0], &uniDrcSelProcParams.numDrcEffectTypeRequestsDesired[0], &uniDrcSelProcParams.drcEffectTypeRequest[0][0]);
            if (err) return (err);
        }
        
        if (downmixIdRequested > -1) {
            uniDrcSelProcParams.downmixIdRequested[0] = downmixIdRequested;
            uniDrcSelProcParams.targetConfigRequestType = 0;
            uniDrcSelProcParams.numDownmixIdRequests  = 1;
        }
        
        if (inFilenameDownmixId != NULL) {
            err = readDmxSelIdxFromFile(inFilenameDownmixId, &(uniDrcSelProcParams.downmixIdRequested[0]));
            if (err) return (err);
            uniDrcSelProcParams.targetConfigRequestType = 0;
            uniDrcSelProcParams.numDownmixIdRequests  = 1;
        }
        
#if MPEG_H_SYNTAX
        if (inFilenameDownmixMatrixSet != NULL) {
            err = readDownmixMatrixSetFromFile(inFilenameDownmixMatrixSet, downmixId, targetChannelCountFromDownmixId, &downmixIdCount);
            if (err) return (err);
        }
        if (inFilenameMpegh3daParams != NULL) {
            err = readMpegh3daParametersFromFile( inFilenameMpegh3daParams, groupIdRequested, &numGroupIdsRequested, groupPresetIdRequested, numMembersGroupPresetIdsRequested, &numGroupPresetIdsRequested, &groupPresetIdRequestedPreference);
            if (err) return (err);
        }
#endif
        
        /***********************************************************************************/
        /* ----------------- OPEN/READ FILE -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark OPEN_FILES
#endif
        
        /* uniDrc/uniDrcConfig bitstream */
        pInFileBitstream = fopen ( inFilenameBitstream , "rb" );
        if (pInFileBitstream == NULL) {
            fprintf(stderr, "Error opening input file %s\n", inFilenameBitstream);
            return -1;
        }
        
        fseek( pInFileBitstream , 0L , SEEK_END);
        nBytes = (int)ftell( pInFileBitstream );
        rewind( pInFileBitstream );
        
        bitstream = calloc( 1, nBytes );
        if( !bitstream ) {
            fclose(pInFileBitstream);
            fputs("memory alloc fails",stderr);
            exit(1);
        }
        
        if( 1!=fread( bitstream , nBytes, 1 , pInFileBitstream) ) {
            fclose(pInFileBitstream);
            free(bitstream);
            fputs("entire read fails",stderr);
            exit(1);
        }
        fclose(pInFileBitstream);
        
        
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            /* loudnessInfoSet bitstream */
            pInFileBitstreamLoudness = fopen ( inFilenameLoudnessBitstream , "rb" );
            if (pInFileBitstreamLoudness == NULL) {
                fprintf(stderr, "Error opening input file %s\n", inFilenameLoudnessBitstream);
                return -1;
            }
            
            fseek( pInFileBitstreamLoudness , 0L , SEEK_END);
            nBytesLoudness = (int)ftell( pInFileBitstreamLoudness );
            rewind( pInFileBitstreamLoudness );
            
            bitstreamLoudness = calloc( 1, nBytesLoudness );
            if( !bitstreamLoudness ) {
                fclose(pInFileBitstreamLoudness);
                fputs("memory alloc fails",stderr);
                exit(1);
            }
            
            if( 1!=fread( bitstreamLoudness , nBytesLoudness, 1 , pInFileBitstreamLoudness) ) {
                fclose(pInFileBitstreamLoudness);
                free(bitstreamLoudness);
                fputs("entire read fails",stderr);
                exit(1);
            }
            fclose(pInFileBitstreamLoudness);
        }
        
        /* uniDrcInterface bitstream */
        if (inFilenameInterfaceBitstream != NULL) {
            pInFileInterfaceBitstream = fopen ( inFilenameInterfaceBitstream , "rb" );
            if (pInFileInterfaceBitstream == NULL) {
                fprintf(stderr, "Error opening input file %s\n", inFilenameInterfaceBitstream);
                return -1;
            }
            
            fseek( pInFileInterfaceBitstream , 0L , SEEK_END);
            nBytesInterfaceBitstream = (int)ftell( pInFileInterfaceBitstream );
            rewind( pInFileInterfaceBitstream );
            
            bitstreamInterface = calloc( 1, nBytesInterfaceBitstream );
            if( !bitstreamInterface ) {
                fclose(pInFileInterfaceBitstream);
                fputs("memory alloc fails",stderr);
                exit(1);
            }
            
            /* copy the file into the buffer */
            if( 1!=fread( bitstreamInterface , nBytesInterfaceBitstream, 1 , pInFileInterfaceBitstream) ) {
                fclose(pInFileInterfaceBitstream);
                free(bitstreamInterface);
                fputs("entire read fails",stderr);
                exit(1);
            }
            fclose(pInFileInterfaceBitstream);
            
            nBitsInterfaceBitstream = nBytesInterfaceBitstream * 8;
        }
        
        /***********************************************************************************/
        /* ----------------- INIT -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark BITSTREAM_DECODER_INIT
#endif
        
        /* interface decoder */
        err = openUniDrcInterfaceDecoder(&hUniDrcIfDecStruct, &hUniDrcInterface);
        if (err) return(err);
        
        err = initUniDrcInterfaceDecoder(hUniDrcIfDecStruct);
        if (err) return(err);
        
        /* decode interface bitstream */
        if (nBitsInterfaceBitstream) {
            err = processUniDrcInterfaceDecoder(hUniDrcIfDecStruct,
                                                hUniDrcInterface,
                                                bitstreamInterface,
                                                nBitsInterfaceBitstream,
                                                &nBitsReadInterface);
            if (err) return(err);
            
            if (plotInfo == 2) {
                fprintf( stdout, "*******************************************************************************\n");
                printf("UniDrcInterface():\n");
                fprintf( stdout, "*******************************************************************************\n\n");
                
                printf("nBitsReadInterface = %d\n",nBitsReadInterface);
                
                /* plot info */
                plotInfoUniDrcInterfaceDecoder(hUniDrcIfDecStruct,hUniDrcInterface);
                
                printf("\nProcess Complete.\n");
                fprintf( stdout, "*******************************************************************************\n");
            }
        }
        
        /* bitstream decoder */
        err = openUniDrcBitstreamDec(&hUniDrcBsDecStruct,&hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
        if (err) return(err);
        
        err = initUniDrcBitstreamDec(hUniDrcBsDecStruct,
                                     audioSampleRate,
                                     audioFrameSize,
                                     delayMode,
                                     -1,
                                     NULL);
        if (err) return(err);
        
        /* selection process */
        err = openUniDrcSelectionProcess(&hUniDrcSelProcStruct);
        if (err) return(err);
        /* TODO: add subbandDomainMode */
        err = initUniDrcSelectionProcess(hUniDrcSelProcStruct,
                                         hUniDrcSelProcParams,
                                         hUniDrcInterface,
                                         0 /* subbandDomainMode */);
        if (err) return(err);
        
        hUniDrcSelProcOutput = &uniDrcSelProcOutput;
        
        /***********************************************************************************/
        /* ----------------- PROCESS -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark PROCESS
#endif

#if MPEG_H_SYNTAX
        /* set MPEG-H 3DA selection process parameters */
        if (inFilenameMpegh3daParams != NULL) {
            err = setMpeghParamsUniDrcSelectionProcess(hUniDrcSelProcStruct, numGroupIdsRequested, groupIdRequested, numGroupPresetIdsRequested, groupPresetIdRequested, numMembersGroupPresetIdsRequested, groupPresetIdRequestedPreference);
            if (err) return(err);
        }
        
        /* set MPEG-H 3DA downmixMatrixSet() parameters */
        err = setMpeghDownmixMatrixSetParamsUniDrcSelectionProcess(hUniDrcConfig, downmixIdCount, downmixId, targetChannelCountFromDownmixId);
        if (err) return(err);
#endif
        

        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            if (nBytes) {
                /* decode uniDrcConfig */
                err = processUniDrcBitstreamDec_uniDrcConfig(hUniDrcBsDecStruct,
                                                             hUniDrcConfig,
                                                             hLoudnessInfoSet,
                                                             bitstream,
                                                             nBytes,
                                                             nBitsOffset,
                                                             &nBitsRead);
                if (err) return(err);
            }
            
            /* decode loudnessInfoSet */
            if (nBytesLoudness)
            {
                err = processUniDrcBitstreamDec_loudnessInfoSet(hUniDrcBsDecStruct,
                                                                hLoudnessInfoSet,
                                                                hUniDrcConfig,
                                                                bitstreamLoudness,
                                                                nBytesLoudness,
                                                                nBitsOffsetLoudness,
                                                                &nBitsReadLoudness);
                if (err) return(err);
            }
        } else {
            if (nBytes) {
                /* decode uniDrc */
                err = processUniDrcBitstreamDec_uniDrc(hUniDrcBsDecStruct,
                                                       hUniDrcConfig,
                                                       hLoudnessInfoSet,
                                                       bitstream,
                                                       nBytes,
                                                       nBitsOffset,
                                                       &nBitsRead);
                if (err) return(err);
            }
        }
        
        /* execute selection process */
        if (nBytes) {
            err = processUniDrcSelectionProcess(hUniDrcSelProcStruct,
                                                hUniDrcConfig,
                                                hLoudnessInfoSet,
                                                hUniDrcSelProcOutput);
            if (err) return(err);
        }
        
        /* selected DRC sets */
        if (plotInfo > 1) {
            fprintf( stdout, "*******************************************************************************\n");
            printf("Result of selection process:\n");
            fprintf( stdout, "*******************************************************************************\n\n");
            if(hUniDrcSelProcOutput->numSelectedDrcSets>1) {
                printf("Selected DRC Sets:\n");
            } else {
                printf("Selected DRC Set:\n");
            }
            
            for (i=0; i<hUniDrcSelProcOutput->numSelectedDrcSets; i++) {
                printf("drcSetId = %d, downmixId = %d\n", hUniDrcSelProcOutput->selectedDrcSetIds[i], hUniDrcSelProcOutput->selectedDownmixIds[i]);
            }
            
            printf("\nLevels and Gain:\n");
            printf("loudnessNormalizationGainDb = %4.2f\n",hUniDrcSelProcOutput->loudnessNormalizationGainDb);
            printf("outputPeakLevel = %4.2f\n",hUniDrcSelProcOutput->outputPeakLevelDb);
            printf("outputLoudness = %4.2f\n",hUniDrcSelProcOutput->outputLoudness);
            
            printf("\nSelected Downmix Instructions:\n");
            printf("activeDownmixId = %d\n",hUniDrcSelProcOutput->activeDownmixId);
            printf("baseChannelCount = %d\n",hUniDrcSelProcOutput->baseChannelCount);
            printf("targetChannelCount = %d\n",hUniDrcSelProcOutput->targetChannelCount);
            if (hUniDrcSelProcOutput->downmixMatrixPresent) {
                printf("downmixMatrix:\n");
                for( i =0 ; i < hUniDrcSelProcOutput->baseChannelCount; i++)
                {
                    printf("          ");
                    for( j =0 ; j < hUniDrcSelProcOutput->targetChannelCount; j++)
                    {
                        printf("%2.4f ", hUniDrcSelProcOutput->downmixMatrix[i][j]);
                    }
                    printf("\n");
                }
            }
            fprintf( stdout, "\n*******************************************************************************\n");
            
#if DEBUG_DRC_SELECTION_CMDL
            i = loopIdx;
            testIdx = loopIdx+1;
            printf("Desired output Test #%d (input: %s, req: #%d):\n", testIdx, inputSignal, controlParameterIndex);
            printf("Selected DRC Sets:\n");
            if (desiredOutputDrcSets[i][0] > 0) {
                printf("drcSetId = %d\n", desiredOutputDrcSets[i][0]);
            }
            if (desiredOutputDrcSets[i][1] > 0) {
                printf("drcSetId = %d\n", desiredOutputDrcSets[i][1]);
            }
            if (desiredOutputDrcSets[i][2] > 0) {
                printf("drcSetId = %d\n", desiredOutputDrcSets[i][2]);
            }
            printf("\nSelected Downmix Instructions:\n");
            printf("loudnessNormalizationGainDb = %4.2f\n",desiredOutputLevelsGains[i][0]);
            printf("outputPeakLevel = %4.2f\n",desiredOutputLevelsGains[i][1]);
            printf("outputLoudness = %4.2f\n",desiredOutputLevelsGains[i][2]);
            
            selectedDrcSetsIdentical = 1;
            for (j=0; j<3; j++) {
                if (j<hUniDrcSelProcOutput->numSelectedDrcSets) {
                    if (desiredOutputDrcSets[i][j] != hUniDrcSelProcOutput->selectedDrcSetIds[j]) {
                        selectedDrcSetsIdentical = 0;
                    }
                } else {
                    if (desiredOutputDrcSets[i][j] >= 0) {
                        selectedDrcSetsIdentical = 0;
                    }
                }
            }
            if (selectedDrcSetsIdentical) {
                fprintf( stdout, "\n*******************************************************************************\n");
                printf("Selected DRC sets are identical!\n");
                fprintf( stdout, "\n*******************************************************************************\n");
            } else {
                fprintf( stdout, "\n*******************************************************************************\n");
                printf("ERROR: Selected DRC sets are NOT identical!\n");
                fprintf( stdout, "\n*******************************************************************************\n");
                break;
            }
            
            if ( (fabs(desiredOutputLevelsGains[i][0]-hUniDrcSelProcOutput->loudnessNormalizationGainDb))<0.01f &&
                fabs(desiredOutputLevelsGains[i][1]-hUniDrcSelProcOutput->outputPeakLevelDb)<0.01f &&
                fabs(desiredOutputLevelsGains[i][2]-hUniDrcSelProcOutput->outputLoudness)<0.01f) {
                fprintf( stdout, "\n*******************************************************************************\n");
                printf("Levels and Gains are identical!\n");
                fprintf( stdout, "\n*******************************************************************************\n");
            } else {
                fprintf( stdout, "\n*******************************************************************************\n");
                printf("ERROR: Levels and Gains are NOT identical!\n");
                fprintf( stdout, "\n*******************************************************************************\n");
                break;
            }
#endif
        }
        
        if (outFilenamemultiBandDrcPresent != NULL) {
            err = getMultiBandDrcPresent(hUniDrcConfig, numSets, multiBandDrcPresent);
            if (err) return(err);
        }
        
        /***********************************************************************************/
        /* ----------------- EXIT -----------------*/
        /***********************************************************************************/
        
#ifdef __APPLE__
#pragma mark EXIT
#endif
        
        err = closeUniDrcInterfaceDecoder(&hUniDrcIfDecStruct,&hUniDrcInterface);
        if (err) return(err);
        
        err = closeUniDrcBitstreamDec(&hUniDrcBsDecStruct,&hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
        if (err) return(err);
        
        err = closeUniDrcSelectionProcess(&hUniDrcSelProcStruct);
        if (err) return(err);
        
        free(bitstream);
        free(bitstreamLoudness);
        
        /***********************************************************************************/
        
        /* write selection transfer data */
        if (outFilenameTransferData != NULL) { /* TODO: add selected eqSetIds such that they can be read by uniDrcGainDecoderCmdl, note that info about before or after dmx is also needed */
            
            /* open file */
            pOutFileTransferData = fopen(outFilenameTransferData, "wb");
            
            if (plotInfo!=0) {
                fprintf(stderr,"\nWriting selection transfer data to file ... \n");
            }
            
            /* selected DRC sets*/
            fprintf(pOutFileTransferData,"%d\n", hUniDrcSelProcOutput->numSelectedDrcSets);
            for( i =0 ; i < hUniDrcSelProcOutput->numSelectedDrcSets; i++)
            {
                fprintf(pOutFileTransferData,"%d %d\n", hUniDrcSelProcOutput->selectedDrcSetIds[i], hUniDrcSelProcOutput->selectedDownmixIds[i]);
            }
            
            /* loudness normalization gain */
            fprintf(pOutFileTransferData,"%2.10f\n", hUniDrcSelProcOutput->loudnessNormalizationGainDb);
            
            /* peak value */
            fprintf(pOutFileTransferData,"%2.10f\n", hUniDrcSelProcOutput->outputPeakLevelDb);
            
            /* decoder user parameters: boost, compress, drcCharacteristicTarget */
            fprintf(pOutFileTransferData,"%2.10f %2.10f %d\n", hUniDrcSelProcOutput->boost, hUniDrcSelProcOutput->compress, hUniDrcSelProcOutput->drcCharacteristicTarget);
            
            /* baseChannelCount and targetChannelCount */
            fprintf(pOutFileTransferData,"%d %d\n", hUniDrcSelProcOutput->baseChannelCount, hUniDrcSelProcOutput->targetChannelCount);
            
#if MPEG_D_DRC_EXTENSION_V1
            /* selectedLoudEqId and mixingLevel */
            fprintf(pOutFileTransferData,"%d %2.10f\n", hUniDrcSelProcOutput->selectedLoudEqId, hUniDrcSelProcOutput->mixingLevel);
            
            /* selected EQ set (before and after DMX) */
            fprintf(pOutFileTransferData,"%d %d\n", hUniDrcSelProcOutput->selectedEqSetIds[0], hUniDrcSelProcOutput->selectedEqSetIds[1]);
#else
            fprintf(pOutFileTransferData,"0 85.0000000000\n0 0\n");
#endif
            /* close file */
            fclose(pOutFileTransferData);
        }
        
        /* write downmix matrix */
        if (hUniDrcSelProcOutput->downmixMatrixPresent && outFilenameDmxMtx != NULL) {
            
            /* open file */
            pOutFileDmxMtx = fopen(outFilenameDmxMtx, "wb");
            
            /* write file */
            for( i =0 ; i < hUniDrcSelProcOutput->baseChannelCount; i++)
            {
                for( j =0 ; j < hUniDrcSelProcOutput->targetChannelCount; j++)
                {
                    fprintf(pOutFileDmxMtx,"%2.10f ", hUniDrcSelProcOutput->downmixMatrix[i][j]);
                }
                fprintf(pOutFileDmxMtx,"\n");
            }
            fprintf(stderr,"\nWriting dmx matrix to file ... \n");
            fclose(pOutFileDmxMtx);
        }
        
        /* write if multiband is present to file */
        if (outFilenamemultiBandDrcPresent != NULL) {
            
            /* open file */
            pOutFilemultiBandDrcPresent = fopen(outFilenamemultiBandDrcPresent, "wb");
            
            fprintf(pOutFilemultiBandDrcPresent,"numSets    isMultiBand    downmixId\n");
            /* write file */
            for( i =0 ; i < 3; i++)
            {
                if (i == 0)
                {
                    fprintf(pOutFilemultiBandDrcPresent,"%i          %i              0", numSets[i], multiBandDrcPresent[i]);
                }
                else if(i == 1)
                {
                    fprintf(pOutFilemultiBandDrcPresent,"%i          %i              0x7F", numSets[i], multiBandDrcPresent[i]);
                }
                else if(i == 2)
                {
                    fprintf(pOutFilemultiBandDrcPresent,"%i          %i              ((!0)&&(!0x7F))", numSets[i], multiBandDrcPresent[i]);
                }
                fprintf(pOutFilemultiBandDrcPresent,"\n");
            }
            fprintf(stderr,"\nWriting multiband present to file ... \n");
            fclose(pOutFilemultiBandDrcPresent);
        }
        
#if DEBUG_DRC_SELECTION_CMDL
    } /* end of test loop */
#endif
    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int readDmxSelIdxFromFile( char* filename, int* downmixIdRequested)
{
    FILE* fileHandle;
    
    /* open file */
    fileHandle = fopen(filename, "r");    
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open downmix selection index file\n");        
        return -1;
    } else {
        fprintf(stderr, "\nFound downmix selection index file: %s.\n", filename );
    }
    fprintf(stderr, "Reading downmix selection index file ... \n");
    fscanf(fileHandle, "%d\n", downmixIdRequested);
    
    fclose(fileHandle);
    return 0;
}

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

/***********************************************************************************/

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

static int readMpegh3daParametersFromFile( char* filename, int* groupIdRequested, int* numGroupIdsRequested, int* groupPresetIdRequested, int* numMembersGroupPresetIdsRequested, int* numGroupPresetIdsRequested, int* groupPresetIdRequestedPreference)
{
    FILE* fileHandle;
    int i, err = 0;
    int plotInfo = 0;
    
    /* open file */
    fileHandle = fopen(filename, "r");
    if ( !fileHandle )
    {
        fprintf(stderr,"Unable to open MPEG-H 3DA parameter file\n");
        return -1;
    } else {
        if (plotInfo!=0) {
            fprintf(stderr, "Found MPEG-H 3DA parameter file: %s.\n", filename );
        }
    }
    
    /* groupIds */
    err = fscanf(fileHandle, "%d\n", numGroupIdsRequested);
    if (err == 0)
    {
        fprintf(stderr,"MPEG-H 3DA parameter file has wrong format\n");
        return 1;
    }
    for (i=0; i<(*numGroupIdsRequested); i++)
    {
        err = fscanf(fileHandle, "%d\n", &groupIdRequested[i]);
        if (err == 0)
        {
            fprintf(stderr,"MPEG-H 3DA parameter file has wrong format\n");
            return 1;
        }
    }
    
    /* groupPresetIds */
    err = fscanf(fileHandle, "%d\n", numGroupPresetIdsRequested);
    if (err == 0)
    {
        fprintf(stderr,"MPEG-H 3DA parameter file has wrong format\n");
        return 1;
    }
    for (i=0; i<(*numGroupPresetIdsRequested); i++)
    {
        err = fscanf(fileHandle, "%d %d\n", &groupPresetIdRequested[i], &numMembersGroupPresetIdsRequested[i]);
        if (err == 0)
        {
            fprintf(stderr,"MPEG-H 3DA parameter file has wrong format\n");
            return 1;
        }
    }
    err = fscanf(fileHandle, "%d\n", groupPresetIdRequestedPreference);
    if (err == 0)
    {
        *groupPresetIdRequestedPreference = -1;
    }

    fclose(fileHandle);
    return 0;
}
#endif

/***********************************************************************************/
