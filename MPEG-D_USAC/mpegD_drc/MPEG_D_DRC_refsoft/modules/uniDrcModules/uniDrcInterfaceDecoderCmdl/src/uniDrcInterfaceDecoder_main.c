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

#include "uniDrcInterfaceDecoder_api.h"

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

int main(int argc, char *argv[])
{
    int i               = 0;
    int err             = 0;
    int nBitsRead       = 0;
    int nBytesBitstream = 0;
    int nBitsBitstream  = 0;
    int helpFlag        = 0;
    int plotInfo        = 1;

    HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct = NULL;
    HANDLE_UNI_DRC_INTERFACE hUniDrcInterface       = NULL;
    unsigned char* bitstream                        = NULL;
    char* inFilename                                = NULL;
    FILE *pInFile                                   = NULL;

    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-v"))
        {
            plotInfo = (int)atof(argv[i+1]);
            i++;
        }
    }
    
    if (plotInfo!=0) {
        fprintf( stdout, "\n");
        fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*                       Unified DRC Interface Decoder                         *\n");
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
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Interface Decoder (RM11).\n\n");
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Interface Decoder (RM11).\n\n");
#endif
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Interface Decoder (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
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
    for ( i = 1; i < argc; ++i )
    {
        if (!strcmp(argv[i],"-if"))      /* Required */
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
        else if (!strcmp(argv[i],"-h")) /* Optional */
        {
            helpFlag = 1;
        }
        else if (!strcmp(argv[i],"-help")) /* Optional */
        {
            helpFlag = 1;
        }
        else if (!strcmp(argv[i],"-v"))
        {
            plotInfo = (int)atof(argv[i+1]);
            i++;
        }
        else {
            fprintf(stderr, "Invalid command line.\n");
            return -1;
        }
    }
    
    if ( (inFilename == NULL) || helpFlag )
    {
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "uniDrcInterfaceDecoderCmdl Usage:\n\n");
        fprintf(stderr, "Example:                   <uniDrcInterfaceDecoderCmdl -if input.bs>\n");
        
        fprintf(stderr, "-if        <input.bs>         input bitstream (uniDrcInterface() syntax according to ISO/IEC 23003-4).\n");
        return -1;
    }
        
#else
    
    char    inputSignal[256];
    strcpy ( inputSignal, "../../../uniDrcInterfaceEncoderCmdl/outputData/test.bit" );
    inFilename = inputSignal;
    
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
    
    /***********************************************************************************/
    /* ----------------- INTERFACE DECODER INIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark INTERFACE_DECODER_INIT
#endif
    
    err = openUniDrcInterfaceDecoder(&hUniDrcIfDecStruct, &hUniDrcInterface);
    if (err) return(err);
    
    err = initUniDrcInterfaceDecoder(hUniDrcIfDecStruct);
    if (err) return(err);

    /***********************************************************************************/
    /* ----------------- INTERFACE DECODER PROCESS -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark INTERFACE_DECODER_PROCESS
#endif
    
    /* decode bitstream */
    err = processUniDrcInterfaceDecoder(hUniDrcIfDecStruct,
                                        hUniDrcInterface,
                                        bitstream,
                                        nBitsBitstream,
                                        &nBitsRead);
    if (err) return(err);
    
    printf("nBitsRead = %d\n",nBitsRead);
    
    /* plot info */
    plotInfoUniDrcInterfaceDecoder(hUniDrcIfDecStruct,hUniDrcInterface);
    
    printf("\nProcess Complete.\n\n");
    
    /***********************************************************************************/
    /* ----------------- EXIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark EXIT
#endif
    
    /* close */
    err = closeUniDrcInterfaceDecoder(&hUniDrcIfDecStruct,&hUniDrcInterface);
    if (err) return(err);

    free(bitstream);

    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/
