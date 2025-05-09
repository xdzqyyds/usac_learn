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

#include "uniDrcInterfaceConfig.h"
#include "uniDrcInterfaceEncoder_api.h"
#include "uniDrcSelectionProcess_api.h"
#include "uniDrcHostParams.h"
#include "xmlParserInterface.h"

#define NBYTESBITSTREAM                             512

#define DEBUG_DRC_INTERFACE_ENC_CMDL 0

/***********************************************************************************/

#ifdef __APPLE__
#pragma mark MAIN 
#endif

int translateSelectionProcessParamsToInterfaceParams(UniDrcSelProcParams *hUniDrcSelProcParams,UniDrcInterface *hUniDrcInterface);

int main(int argc, char *argv[])
{
    int i               = 0;
    int err             = 0;
    int nBitsWritten    = 0;
    int nBytesWritten   = 0;
    int nBytesBitstream = NBYTESBITSTREAM;
    int cfg1Idx         = 0;
    int cfg2Idx         = 0;
    int helpFlag        = 0;
    int validCmdl       = 0;
    int plotInfo        = 1;

    unsigned char bitstream[NBYTESBITSTREAM] = {0};
    UniDrcSelProcParams uniDrcSelProcParams;
    HANDLE_UNI_DRC_IF_ENC_STRUCT hUniDrcIfEncStruct = NULL;
    HANDLE_UNI_DRC_INTERFACE hUniDrcInterface       = NULL;
    char* outFilename                               = NULL;
    char* xmlFilename                               = NULL;
    FILE *pOutFile                                  = NULL;
#if MPEG_H_SYNTAX
    char* outFilenameMpegh3daParams                 = NULL;
    FILE *pOutFileMpegh3daParams                    = NULL;
#endif
    UniDrcInterface uniDrcInterfaceXml;
    
    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-v"))
        {
            plotInfo = (int)atof(argv[i+1]);
            i++;
        }
    }

    if (plotInfo != 0) {
        fprintf( stdout, "\n");
        fprintf( stdout, "********************* MPEG-D DRC Coder - Reference Model 11 *******************\n");
        fprintf( stdout, "*                                                                             *\n");
        fprintf( stdout, "*                       Unified DRC Interface Encoder                         *\n");
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
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Interface Encoder (RM11).\n\n");
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015/AMD1 - MPEG-D DRC Interface Encoder (RM11).\n\n");
#endif
#else
        fprintf(stdout,"ISO/IEC 23003-4:2015 - MPEG-D DRC Interface Encoder (RM11) (ISO/IEC 23008-3:2015 (MPEG-H 3DA) Syntax).\n\n");
#endif
    }

    /***********************************************************************************/
    /* ----------------- CMDL PARSING -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark CMDL_PARSING
#endif
    
#if !DEBUG_DRC_INTERFACE_ENC_CMDL
    
    /* Commandline Parsing */
    for ( i = 1; i < (unsigned int) argc; ++i )
    {
        if (!strcmp(argv[i],"-of"))      /* Required */
        {
            if ( argv[i+1] ) {
                outFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No output filename for bitstream given.\n");
                return -1;
            }
            i++;
        }
#if MPEG_H_SYNTAX
        else if (!strcmp(argv[i],"-mpegh3daParams"))      /* Optional */
        {
            if ( argv[i+1] ) {
                outFilenameMpegh3daParams = argv[i+1];
            }
            else {
                fprintf(stderr, "No output filename for MPEG-H 3DA parameters given.\n");
                return -1;
            }
            i++;
        }
#endif
        else if (!strcmp(argv[i],"-cfg1")) /* Optional */
        {
            cfg1Idx = (int)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-cfg2")) /* Optional */
        {
            cfg2Idx = (int)atof(argv[i+1]);
            i++;
        }
        else if (!strcmp(argv[i],"-xml")) /* Optional */
        {
            if ( argv[i+1] ) {
                xmlFilename = argv[i+1];
            }
            else {
                fprintf(stderr, "No xml filename for configuration given.\n");
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
    
    if (cfg1Idx != 0 && cfg2Idx != 0) {
        validCmdl = 0;
    } else if ( cfg1Idx > 0 && cfg1Idx < 8 ) {
        validCmdl = 1;
#if !MPEG_H_SYNTAX
    } else if ( cfg2Idx > 0 && cfg2Idx < 40 ) {
#else
    } else if ( cfg2Idx > 0 && cfg2Idx < 21 ) {
#endif
        validCmdl = 2;
    } else if (xmlFilename != NULL) {
        validCmdl = 3;
    }
    
    if ( (outFilename == NULL) || !validCmdl || helpFlag )
    {
        if (!helpFlag) {
            fprintf( stderr, "Invalid input arguments!\n\n");
        }
        fprintf(stderr, "uniDrcInterfaceEncoderCmdl Usage:\n\n");
        fprintf(stderr, "Example:                   <uniDrcInterfaceEncoderCmdl -of output.bs -cfgIdx 1>\n");
        
        fprintf(stderr, "-of        <output.bs>        output bitstream (uniDrcInterface() syntax according to ISO/IEC 23003-4).\n");
        fprintf(stderr, "-cfg1       <cfgIdx>          configuration index for example configs (1 ... 7).\n");
#if !MPEG_H_SYNTAX
        fprintf(stderr, "-cfg2       <cfgIdx>          configuration index for host param configs (1 ... 39).\n");
#else
        fprintf(stderr, "-cfg2       <cfgIdx>          configuration index for host param configs (1 ... 20).\n");
        fprintf(stderr, "-mpegh3daParams <output.txt>  output txt for host parameters selection process parameters implicitly provided by MPEG-H 3DA decoder.\n");
#endif
        fprintf(stderr, "-xml        <cfg.xml>         xml configuration for host param configs.\n");
        fprintf(stderr, "-v          <int>             verbose output: 0 = short, 1 = default, 2 = print all.\n");
        
        return -1;
    }
        
#else
    
    cfg1Idx = 1;
    cfg2Idx = 1;
    validCmdl = 1;
    char    outputSignal[256];
    strcpy ( outputSignal, "../../outputData/test.bit" );
    outFilename = outputSignal;
    
#endif
    
    /***********************************************************************************/
    /* ----------------- INTERFACE DECODER INIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark INTERFACE_DECODER_INIT
#endif
    
    err = openUniDrcInterfaceEncoder(&hUniDrcIfEncStruct, &hUniDrcInterface);
    if (err) return(err);
    
    err = initUniDrcInterfaceEncoder(hUniDrcIfEncStruct);
    if (err) return(err);
    
    /* load config */
    if (validCmdl == 1) {
        switch (cfg1Idx) {
            case 1:
                uniDrcInterfaceConfigEnc1(hUniDrcInterface);
                break;
            case 2:
                uniDrcInterfaceConfigEnc2(hUniDrcInterface);
                break;
            case 3:
                uniDrcInterfaceConfigEnc3(hUniDrcInterface);
                break;
            case 4:
                uniDrcInterfaceConfigEnc4(hUniDrcInterface);
                break;
            case 5:
                uniDrcInterfaceConfigEnc5(hUniDrcInterface);
                break;
            case 6:
                uniDrcInterfaceConfigEnc6(hUniDrcInterface);
                break;
#if MPEG_D_DRC_EXTENSION_V1
            case 7:
                uniDrcInterfaceConfigEnc7(hUniDrcInterface);
                break;
#endif /* MPEG_D_DRC_EXTENSION_V1 */
            default:
                printf("Error: configIdx == %d not supported.\n",cfg1Idx);
                return 1;
                break;
        }
    } else if (validCmdl == 2) {
        err = setDefaultParams_selectionProcess(&uniDrcSelProcParams);
        if (err) return (err);
        err = setCustomParams_selectionProcess(cfg2Idx,&uniDrcSelProcParams);
        if (err) return (err);
        err = evaluateCustomParams_selectionProcess(&uniDrcSelProcParams);
        if (err) return (err);
        err = translateSelectionProcessParamsToInterfaceParams(&uniDrcSelProcParams,hUniDrcInterface);
        if (err) return (err);
    } else if (validCmdl == 3) {
        err = parseInterfaceFile(xmlFilename, &uniDrcInterfaceXml);
        memcpy(hUniDrcInterface, &uniDrcInterfaceXml, sizeof(UniDrcInterface));
    } else {
        printf("Invalid Config.\n\n");
        return 1;
    }

    /***********************************************************************************/
    /* ----------------- INTERFACE DECODER PROCESS -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark INTERFACE_DECODER_PROCESS
#endif
    
    /* encode bitstream */
    err = processUniDrcInterfaceEncoder(hUniDrcIfEncStruct,
                                        hUniDrcInterface,
                                        bitstream,
                                        nBytesBitstream,
                                        &nBitsWritten);
    if (err) return(err);
    
    if (plotInfo!=0) {
        printf("nBitsWritten = %d\n",nBitsWritten);
    }
    
    /* plot info */
    if (plotInfo > 1) {
        plotInfoUniDrcInterfaceEncoder(hUniDrcInterface);
    }
    
    /***********************************************************************************/
    /* ----------------- OPEN/WRITE FILE -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark OPEN_FILES
#endif
    
    /* write file */
    pOutFile = fopen ( outFilename , "wb" );

    nBytesWritten = nBitsWritten/8+1;
    
    fwrite(bitstream, nBytesWritten, 1, pOutFile);
    fseek( pOutFile , 0L , SEEK_END);
    
    fclose(pOutFile);
    
    if (plotInfo!=0) {
        printf("\nProcess Complete.\n\n");
    }
    
    /***********************************************************************************/
    /* ----------------- EXIT -----------------*/
    /***********************************************************************************/
    
#ifdef __APPLE__
#pragma mark EXIT
#endif
    
    /* close */
    err = closeUniDrcInterfaceEncoder(&hUniDrcIfEncStruct,&hUniDrcInterface);
    if (err) return(err);
   
    /***********************************************************************************/

#if MPEG_H_SYNTAX

    /* write MPEG-H 3DA parameters */
    if (outFilenameMpegh3daParams != NULL) {
        
        /* open file */
        pOutFileMpegh3daParams = fopen(outFilenameMpegh3daParams, "wb");
        
        if (plotInfo!=0) {
            fprintf(stderr,"\nWriting MPEG-H 3DA parameters to file ... \n");
        }
        
        /* groupIds */
        fprintf(pOutFileMpegh3daParams,"%d\n", uniDrcSelProcParams.numGroupIdsRequested);
        for( i =0 ; i < uniDrcSelProcParams.numGroupIdsRequested; i++)
        {
            fprintf(pOutFileMpegh3daParams,"%d\n", uniDrcSelProcParams.groupIdRequested[i]);
        }
        
        /* groupPresetIds */
        fprintf(pOutFileMpegh3daParams,"%d\n", uniDrcSelProcParams.numGroupPresetIdsRequested);
        for( i =0 ; i < uniDrcSelProcParams.numGroupPresetIdsRequested; i++)
        {
            fprintf(pOutFileMpegh3daParams,"%d %d\n", uniDrcSelProcParams.groupPresetIdRequested[i], uniDrcSelProcParams.numMembersGroupPresetIdsRequested[i]);
        }
        /* groupPresetIdRequestedPreference */
        fprintf(pOutFileMpegh3daParams,"%d\n", uniDrcSelProcParams.groupPresetIdRequestedPreference);

        /* close file */
        fclose(pOutFileMpegh3daParams);
        
    }

#endif

    /***********************************************************************************/
    
    return 0;
    
}

/***********************************************************************************/

int translateSelectionProcessParamsToInterfaceParams(UniDrcSelProcParams *hUniDrcSelProcParams,UniDrcInterface *hUniDrcInterface)
{
    int i,j;
    
    /********************************************************************************/
    /* uniDrcInterfaceSignature */
    /********************************************************************************/
    
    hUniDrcInterface->uniDrcInterfaceSignaturePresent                               = 0;
    
    hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureType         = 0;
    hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength   = 2;
    hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[0]      = 'E';
    hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[1]      = 'N';
    hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[2]      = 'G';
    
    /********************************************************************************/
    /* systemInterface */
    /********************************************************************************/
    
    hUniDrcInterface->systemInterfacePresent                                        = 1;
    hUniDrcInterface->systemInterface.targetConfigRequestType                       = hUniDrcSelProcParams->targetConfigRequestType;
    
    if (hUniDrcSelProcParams->targetConfigRequestType == 0) {
        hUniDrcInterface->systemInterface.numDownmixIdRequests                      = hUniDrcSelProcParams->numDownmixIdRequests;
        for (i=0; i<hUniDrcSelProcParams->numDownmixIdRequests; i++) {
            hUniDrcInterface->systemInterface.downmixIdRequested[i]                 = hUniDrcSelProcParams->downmixIdRequested[i];
        }
    } else if (hUniDrcSelProcParams->targetConfigRequestType == 1) {
        hUniDrcInterface->systemInterface.targetLayoutRequested                     = hUniDrcSelProcParams->targetLayoutRequested;

    } else if (hUniDrcSelProcParams->targetConfigRequestType == 2) {
        hUniDrcInterface->systemInterface.targetChannelCountRequested               = hUniDrcSelProcParams->targetChannelCountRequested;
    } else {
        return 1;
    }
    
    /********************************************************************************/
    /* loudnessNormalizationControlInterface */
    /********************************************************************************/
    
    hUniDrcInterface->loudnessNormalizationControlInterfacePresent                  = 1;
    
    hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn = hUniDrcSelProcParams->loudnessNormalizationOn;
    if (hUniDrcSelProcParams->loudnessNormalizationOn) {
        hUniDrcInterface->loudnessNormalizationControlInterface.targetLoudness      = hUniDrcSelProcParams->targetLoudness;
    }
    
    /********************************************************************************/
    /* loudnessNormalizationParameterInterface */
    /********************************************************************************/
    
    hUniDrcInterface->loudnessNormalizationParameterInterfacePresent                                        = 1;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.albumMode                                     = hUniDrcSelProcParams->albumMode;
    hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent                            = hUniDrcSelProcParams->peakLimiterPresent;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax                    = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax                          = hUniDrcSelProcParams->loudnessDeviationMax;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod               = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod                     = hUniDrcSelProcParams->loudnessMeasurementMethod;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem               = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem                     = hUniDrcSelProcParams->loudnessMeasurementSystem;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc              = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc                    = hUniDrcSelProcParams->loudnessMeasurementPreProc;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency                   = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency                         = hUniDrcSelProcParams->deviceCutOffFrequency;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax          = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax                = hUniDrcSelProcParams->loudnessNormalizationGainDbMax;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb       = hUniDrcSelProcParams->loudnessNormalizationGainModificationDb;
    
    hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax                      = 1;
    hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax                            = hUniDrcSelProcParams->outputPeakLevelMax;
    
    /********************************************************************************/
    /* dynamicRangeControlInterface */
    /********************************************************************************/
    
    hUniDrcInterface->dynamicRangeControlInterfacePresent                            = 1;
    
    hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn             = hUniDrcSelProcParams->dynamicRangeControlOn;
    hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests             = hUniDrcSelProcParams->numDrcFeatureRequests;
    
    for (i=0; i<hUniDrcSelProcParams->numDrcFeatureRequests; i++) {
        hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]               = hUniDrcSelProcParams->drcFeatureRequestType[i];
        if (hUniDrcSelProcParams->drcFeatureRequestType[i] == 0) {
            hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]        = hUniDrcSelProcParams->numDrcEffectTypeRequests[i];
            hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[i] = hUniDrcSelProcParams->numDrcEffectTypeRequestsDesired[i];
            for (j=0; j<hUniDrcSelProcParams->numDrcEffectTypeRequests[i]; j++) {
                hUniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[i][j]     = hUniDrcSelProcParams->drcEffectTypeRequest[i][j];

            }
        } else if (hUniDrcSelProcParams->drcFeatureRequestType[i] == 1) {
            hUniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[i]  = hUniDrcSelProcParams->dynamicRangeMeasurementRequestType[i];
            hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i]        = hUniDrcSelProcParams->dynamicRangeRequestedIsRange[i];
            hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[i]        = hUniDrcSelProcParams->dynamicRangeRequestValue[i];
            hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[i]     = hUniDrcSelProcParams->dynamicRangeRequestValueMin[i];
            hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[i]     = hUniDrcSelProcParams->dynamicRangeRequestValueMax[i];
        } else if (hUniDrcSelProcParams->drcFeatureRequestType[i] == 2) {
            hUniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[i]        = hUniDrcSelProcParams->drcCharacteristicRequest[i];
        } else {
            return 1;
        }
    }
    
    /********************************************************************************/
    /* dynamicRangeControlParameterInterface */
    /********************************************************************************/
    
    hUniDrcInterface->dynamicRangeControlParameterInterfacePresent                  = 1;
    
    hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress          = 1;
    hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost             = 1;
    
    hUniDrcInterface->dynamicRangeControlParameterInterface.compress                = hUniDrcSelProcParams->compress;
    hUniDrcInterface->dynamicRangeControlParameterInterface.boost                   = hUniDrcSelProcParams->boost;
    
    hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget = 1;
    hUniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget       = hUniDrcSelProcParams->drcCharacteristicTarget;
    
    /********************************************************************************/
    /* uniDrcInterfaceExtension */
    /********************************************************************************/
    
#if MPEG_D_DRC_EXTENSION_V1
    hUniDrcInterface->uniDrcInterfaceExtensionPresent                              = 1;
    
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent = 1;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequestPresent = 1;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequest = hUniDrcSelProcParams->loudnessEqRequest;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGainPresent = 1;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGain = hUniDrcSelProcParams->playbackGain;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivityPresent = 1;
    hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivity = hUniDrcSelProcParams->sensitivity;

    hUniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent = 1;
    hUniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterface.eqSetPurposeRequest = hUniDrcSelProcParams->eqSetPurposeRequest;
#else
    hUniDrcInterface->uniDrcInterfaceExtensionPresent                               = 0;
#endif
    
    /********************************************************************************/
    /* Exit */
    /********************************************************************************/


    return 0;
}

/***********************************************************************************/
