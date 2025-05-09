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

#include "uniDrcBitstreamDecoder_api.h"

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

typedef enum _BITSTREAM_FILE_FORMAT
{
    BITSTREAM_FILE_FORMAT_DEFAULT = 0,
    BITSTREAM_FILE_FORMAT_SPLIT   = 1
} BITSTREAM_FILE_FORMAT;

int convertCicpIndexToNumChan(int cicp);

int main(int argc, char *argv[])
{
       
    int i;
    int err             = 0;
    int nBitsRead       = 0;
    int nBytesRead      = 0;
    int nBytes          = 0;
    int nBitsOffset     = 0;
    int frameNo         = 0;
    int byteIndex       = 0;
    int delayMode       = 0;
    int cicp            = 2;
    int cicpPresent     = 0;
    int baseChannelCount= 2;
    int baseChannelCountPresent = 0;
    int audioSampleRate = 48000;
    int audioFrameSize  = 1024;
    unsigned char* bitstream  = NULL;
    BITSTREAM_FILE_FORMAT bitstreamFileFormat = BITSTREAM_FILE_FORMAT_DEFAULT;
    int plotInfo        = 1;
    int helpFlag        = 0;
    HANDLE_UNI_DRC_BS_DEC_STRUCT hUniDrcBsDecStruct = NULL;
    HANDLE_UNI_DRC_CONFIG hUniDrcConfig             = NULL;
    HANDLE_UNI_DRC_GAIN hUniDrcGain                 = NULL;
    HANDLE_LOUDNESS_INFO_SET hLoudnessInfoSet       = NULL;
    char* inFilename    = NULL;
    char* inFilenameGain= NULL;
    FILE *pInFile       = NULL;

#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    /* ISOBMFF */
    int ffLoudnessPresent     = 0;
    int nBitsReadFfLoudness   = 0;
    int nBytesFfLoudness      = 0;
    int nBitsOffsetFfLoudness = 0;
    int byteIndexFfLoudness   = 0;
    
    int ffDrcPresent   = 0;
    int nBitsReadFfDrc = 0;
    int nBytesFfDrc    = 0;
    int nBitsOffsetFfDrc = 0;
    int byteIndexFfDrc   = 0;
    
    unsigned char* bitstreamFfLoudness = NULL;
    char* inFilenameFfLoudness= NULL;
    FILE *pInFileFfLoudness   = NULL;
    
    unsigned char* bitstreamFfDrc = NULL;
    char* inFilenameFfDrc= NULL;
    FILE *pInFileFfDrc   = NULL;
#endif
#endif
#endif

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
        fprintf( stdout, "*                       Unified DRC Bitstream Decoder                         *\n");
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
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Bitstream Decoder (RM11).\n\n");
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Bitstream Decoder (RM11).\n\n");
#endif
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Bitstream Decoder (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
#endif
    }

    /***********************************************************************************/
    /* ----------------- CMDL PARSING -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark CMDL_PARSING
#endif
    
#if 1    
    /* Commandline Parsing */
    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-ib"))      /* Required */
        {
            if ( argv[i+1] ) {
                inFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for bitstream given.\n");
                return -1;
            }
            i++;
        }
        else if (!strcmp(argv[i],"-ig"))      /* optional */
        {
            if ( argv[i+1] ) {
                inFilenameGain = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for gain bitstream given.\n");
                return -1;
            }
            i++;
        }
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        else if (!strcmp(argv[i],"-ffLoudness"))      /* optional */
        {
            if ( argv[i+1] ) {
                inFilenameFfLoudness = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for ISOBMFF Loudness box given.\n");
                return -1;
            }
            ffLoudnessPresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-ffDrc"))      /* optional */
        {
            if ( argv[i+1] ) {
                inFilenameFfDrc = argv[i+1];
            }
            else {
                fprintf(stderr, "No input filename for ISOBMFF DRC boxes given.\n");
                return -1;
            }
            ffDrcPresent = 1;
            i++;
        }
#endif
#endif
#endif
        else if (!strcmp(argv[i],"-fs")) /* optional */
        {
            audioSampleRate = (int)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-afs")) /* optional */
        {
            audioFrameSize = (int)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-d")) /* optional */
        {
            delayMode = (int)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-cicp")) /* optional */
        {
            cicp = (int)atof(argv[i+1]);
            cicpPresent = 1;
            i++;
        }
        else if (!strcmp(argv[i],"-bcc")) /* optional */
        {
            baseChannelCount = (int)atof(argv[i+1]);
            baseChannelCountPresent = 1;
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
    
    if ( ((inFilename == NULL) || (cicpPresent && baseChannelCountPresent)) || helpFlag )
    {
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "uniDrcBitstreamDecoderCmdl Usage:\n\n");
        fprintf(stderr, "Example:                   <uniDrcBitstreamDecoderCmdl -if input.bs -cicp 2 -info 1>\n");
        fprintf(stderr, "Example (with parameters): <uniDrcBitstreamDecoderCmdl -if input.bs -bcc 6 - fs 48000 -afs 1024 -d 1 -info 2>\n\n");
        
        fprintf(stderr, "-ib        <input.bs>        input bitstream (uniDrc() syntax).\n");
        fprintf(stderr, "-ig        <inputGain.bs>    input bitstream (uniDrcGain() syntax).\n");
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        fprintf(stderr, "-ffLoudness <ffLou.bs>       input bitstream for 'ludt' box according to ISO/IEC 14496-12 (ISOBMFF).\n");
        fprintf(stderr, "-ffDrc      <ffDrc.bs>       input bitstream for 'chnl', 'dmix', 'udc2', 'udi2' and 'udex' boxes according to ISO/IEC 14496-12 (ISOBMFF).\n");
#endif
#endif
#endif
        fprintf(stderr, "-fs        <audioSampleRate>  audio sampling rate in Hz.\n");
        fprintf(stderr, "-afs       <audioFrameSize>   audio frame size in samples.\n");
        fprintf(stderr, "-cicp      <cicpIdx>          CICP input format index (alternative to -bcc).\n");
        fprintf(stderr, "-bcc       <baseChannelCount> base channel count (alternative to -cicp).\n");
        fprintf(stderr, "-d         <delayMode>        delay mode (0: default, 1: low-delay mode).\n");
        
        fprintf(stderr, "-v         <info>             Index for verbose mode (0: short, 1: default, 2: more, 3: print all).\n");
        return -1;
    }
    
    if (cicpPresent) {
        baseChannelCount = convertCicpIndexToNumChan(cicp);
    }
    
#else
    
    char    inputSignal[256];
    strcpy ( inputSignal, "../../inputData/DrcGain3_new.bit" );
    inFilename = inputSignal;
    plotInfo = 3;
    
#endif
    
    /***********************************************************************************/
    /* ----------------- OPEN/READ FILE -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark OPEN_FILES
#endif
    
    pInFile = fopen ( inFilename , "rb" );
    if (pInFile == NULL) {
        fprintf(stderr, "Error opening input file %s\n", inFilename);
        return -1;
    }
    
    fseek( pInFile , 0L , SEEK_END);
    nBytes = (int)ftell( pInFile );
    rewind( pInFile );
    
    bitstream = calloc( 1, nBytes );
    if( !bitstream ) {
        fclose(pInFile);
        fputs("memory alloc fails",stderr);
        exit(1);
    }
    
    /* copy the file into the buffer */
    if( 1!=fread( bitstream , nBytes, 1 , pInFile) ) {
        fclose(pInFile);
        free(bitstream);
        fputs("entire read fails",stderr);
        exit(1);
    }
    fclose(pInFile);
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    if (ffLoudnessPresent) {
        /* ffLoudness */
        pInFileFfLoudness = fopen ( inFilenameFfLoudness , "rb" );
        if (pInFileFfLoudness == NULL) {
            fprintf(stderr, "Error opening ISOBMFF Loudness file %s\n", inFilenameFfLoudness);
            return -1;
        }
        
        fseek( pInFileFfLoudness , 0L , SEEK_END);
        nBytesFfLoudness = (int)ftell( pInFileFfLoudness );
        rewind( pInFileFfLoudness );
        
        bitstreamFfLoudness = calloc( 1, nBytesFfLoudness );
        if( !bitstreamFfLoudness ) {
            fclose(pInFileFfLoudness);
            fputs("memory alloc fails",stderr);
            exit(1);
        }
        
        /* copy the file into the buffer */
        if( 1!=fread( bitstreamFfLoudness , nBytesFfLoudness, 1 , pInFileFfLoudness) ) {
            fclose(pInFileFfLoudness);
            free(bitstreamFfLoudness);
            fputs("entire read fails",stderr);
            exit(1);
        }
        fclose(pInFileFfLoudness);
    }
    
    if (ffDrcPresent) {
        /* ffDrc */
        pInFileFfDrc = fopen ( inFilenameFfDrc , "rb" );
        if (pInFileFfDrc == NULL) {
            fprintf(stderr, "Error opening ISOBMFF DRC file %s\n", inFilenameFfDrc);
            return -1;
        }
        
        fseek( pInFileFfDrc , 0L , SEEK_END);
        nBytesFfDrc = (int)ftell( pInFileFfDrc );
        rewind( pInFileFfDrc );
        
        bitstreamFfDrc = calloc( 1, nBytesFfDrc );
        if( !bitstreamFfDrc ) {
            fclose(pInFileFfDrc);
            fputs("memory alloc fails",stderr);
            exit(1);
        }
        
        /* copy the file into the buffer */
        if( 1!=fread( bitstreamFfDrc , nBytesFfDrc, 1 , pInFileFfDrc) ) {
            fclose(pInFileFfDrc);
            free(bitstreamFfDrc);
            fputs("entire read fails",stderr);
            exit(1);
        }
        fclose(pInFileFfDrc);
    }
#endif
#endif
#endif

    /***********************************************************************************/
    /* ----------------- BITSTREAM DECODER INIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark BITSTREAM_DECODER_INIT
#endif
    
    err = openUniDrcBitstreamDec(&hUniDrcBsDecStruct, &hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (err) return(err);
    
    err = initUniDrcBitstreamDec(hUniDrcBsDecStruct,
                                 audioSampleRate,
                                 audioFrameSize,
                                 delayMode,
                                 -1,
                                 NULL);
    if (err) return(err);

    /***********************************************************************************/
    /* ----------------- BITSTREAM DECODER PROCESS -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark BITSTREAM_DECODER_PROCESS
#endif
    
    while (nBytes != 0) {
        
        frameNo++;
       
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        if (ffDrcPresent == 1) {
            err = processUniDrcBitstreamDec_isobmff(hUniDrcBsDecStruct,
                                                    hUniDrcConfig,
                                                    hLoudnessInfoSet,
                                                    baseChannelCount,
                                                    &bitstreamFfDrc[byteIndexFfDrc],
                                                    nBytesFfDrc,
                                                    nBitsOffsetFfDrc,
                                                    &nBitsReadFfDrc);
            if (err) return(err);
        }
#endif
#endif
#endif
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
            err = processUniDrcBitstreamDec_uniDrcConfig(hUniDrcBsDecStruct,
                                                         hUniDrcConfig,
                                                         hLoudnessInfoSet,
                                                         &bitstream[byteIndex],
                                                         nBytes,
                                                         nBitsOffset,
                                                         &nBitsRead);
            if (err) return(err);
        
            nBytesRead  = nBitsRead/8;
            nBitsOffset = nBitsRead - nBytesRead*8;
            byteIndex   += nBytesRead;
            nBytes      -= nBytesRead;
        } else {
            err = processUniDrcBitstreamDec_uniDrc(hUniDrcBsDecStruct,
                                                   hUniDrcConfig,
                                                   hLoudnessInfoSet,
                                                   &bitstream[byteIndex],
                                                   nBytes,
                                                   nBitsOffset,
                                                   &nBitsRead);
            if (err) return(err);
            
            nBytesRead  = nBitsRead/8;
            nBitsOffset = nBitsRead - nBytesRead*8;
            byteIndex   += nBytesRead;
            nBytes      -= nBytesRead;
        }
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
        if (ffLoudnessPresent == 1) {
            err = processUniDrcBitstreamDec_isobmff(hUniDrcBsDecStruct,
                                                    hUniDrcConfig,
                                                    hLoudnessInfoSet,
                                                    baseChannelCount,
                                                    &bitstreamFfLoudness[byteIndexFfLoudness],
                                                    nBytesFfLoudness,
                                                    nBitsOffsetFfLoudness,
                                                    &nBitsReadFfLoudness);
            if (err) return(err);
        }
#endif
#endif
#endif
        err = processUniDrcBitstreamDec_uniDrcGain(hUniDrcBsDecStruct,
                                                   hUniDrcConfig,
                                                   hUniDrcGain,
                                                   &bitstream[byteIndex],
                                                   nBytes,
                                                   nBitsOffset,
                                                   &nBitsRead);
        if (err) return(err);
        
        nBytesRead  = nBitsRead/8;
        nBitsOffset = nBitsRead - nBytesRead*8;
        byteIndex   = byteIndex + nBytesRead;
        nBytes      = nBytes - nBytesRead;
        
        /* plot info */
        plotInfoBS(hUniDrcConfig, hUniDrcGain, hLoudnessInfoSet, frameNo, nBytes, plotInfo);
        if (plotInfo == 1) {
            break;
        }
    }
    
    /***********************************************************************************/
    /* ----------------- EXIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark EXIT
#endif
    
    err = closeUniDrcBitstreamDec(&hUniDrcBsDecStruct,&hUniDrcConfig, &hLoudnessInfoSet, &hUniDrcGain);
    if (err) return(err);

    free(bitstream);
#if ISOBMFF_SYNTAX
#if !MPEG_H_SYNTAX
#if AMD1_SYNTAX
    free(bitstreamFfLoudness);
    free(bitstreamFfDrc);
#endif
#endif
#endif

    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int convertCicpIndexToNumChan(int cicp)
{
    int numCh = 0;
    
    switch (cicp) {
        case 1: /* 1.0 */
            numCh = 1;
            break;
        case 2: /* 2.0 */
            numCh = 2;
            break;
        case 3: /* 3.0 */
            numCh = 3;
            break;
        case 5: /* 5.0 */
            numCh = 5;
            break;
        case 6: /* 5.1 */
            numCh = 6;
            break;
        case 7: /* 7.1 front */
            numCh = 8;
            break;
        case 12: /* 7.1 rear */
            numCh = 8;
            break;
        case 13: /* 22.2 */
            numCh = 24;
            break;
        case 14: /* 7.1 elev */
            numCh = 8;
            break;
        default:
            numCh = -1;
            break;
    }
    
    return numCh;
}

/***********************************************************************************/
