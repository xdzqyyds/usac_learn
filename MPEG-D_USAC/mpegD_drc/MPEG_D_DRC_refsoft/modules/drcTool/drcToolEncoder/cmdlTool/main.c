/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc.
 
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
 
 Apple Inc. retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

/* activate MPEG-H mode by adding MPEG_H to preprocessor macros */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "drcToolEncoder.h"
#include "drcConfig.h"
#include "userConfig.h"
#include "xmlParserConfig.h"

typedef enum _BITSTREAM_FILE_FORMAT
{
    BITSTREAM_FILE_FORMAT_DEFAULT    = 0, /* sequence of uniDrc() payloads in one file */
    BITSTREAM_FILE_FORMAT_SPLIT      = 1, /* loudnessInfoSet(), uniDrcConfig() and sequence of uniDrcGain() payloads in three files */
    BITSTREAM_FILE_FORMAT_FF_DEFAULT = 2, /* 'ludt' ISOBMFF payload, [‘chnl’, ‘dmix’, ‘udc2’, ‘udi2’, ‘udex’] ISOBMFF payload and sequence of uniDrc() payloads in three files */
    BITSTREAM_FILE_FORMAT_FF_SPLIT   = 3  /* 'ludt' ISOBMFF payload, [‘chnl’, ‘dmix’, ‘udc2’, ‘udi2’, ‘udex’] ISOBMFF payload and sequence of uniDrcGain() payloads in three files */
} BITSTREAM_FILE_FORMAT;

#define DB_DOMAIN_GAIN_INPUT    1   /* set to 1 if the input gain values are given in dB */
/* set to 0 if the input gain values are linear */
int main(int argc, const char * argv[])
{
    int err=0, k;
    BITSTREAM_FILE_FORMAT bitstreamFileFormat = BITSTREAM_FILE_FORMAT_DEFAULT;
    HANDLE_DRC_ENCODER hDrcEnc;
    UniDrcConfig encConfig = {0};
    LoudnessInfoSet encLoudnessInfoSet = {0};
    UniDrcGainExt encGainExtension = {0};
    unsigned char bitstreamBuffer[MAX_DRC_PAYLOAD_BYTES] = {0};
    int bitBufCount = 0;
    float* interleavedGainBuffer = NULL;
    float** gainBuffer = NULL;
    int n, allBandGainCount;
    FILE *fGainIn, *fBsOut, *fBsOut_uniDrcConfig, *fBsOut_loudnessInfoSet, *fBsOut_uniDrcGain;
#ifdef DEBUG_OUTPUT_FORMAT
    int length = 0;
    char txtFilenameOutputDrcBitrate[FILENAME_MAX] = "";
    char txtFilenameOutputLoudnessInfoSet[FILENAME_MAX] = "";
    char bsFilenameOutputLoudnessInfo[FILENAME_MAX] = "";
    FILE *fTxtOut_drcBitrate, *fTxtOut_loudnessInfoSet, *fBsOut_loudnessInfo;
#endif
    int writeDrcConfig = TRUE;  /* write DRC Config in first frame */
    int writeDrcLoudnessInfoSet = TRUE;
    int frameCount = 0, endCount = 0;
    EncParams encParams = {0};
    
#if !AMD1_SYNTAX
    printf("ISO/IEC 23003-4:2015 Software DRC Encoder (informative).\n");
    printf("Version 1.0  (01/2014)\n\n");
#else /* AMD1_SYNTAX */
    printf("ISO/IEC 23003-4:2015/AMD1 Software DRC Encoder (informative).\n");
    printf("Version 1.1  (07/2017)\n\n");
#endif /* AMD1_SYNTAX */
    
#if !MPEG_H_SYNTAX
#if ISOBMFF_SYNTAX
    if (argc != 7 && argc != 6 && argc != 4) {
#else
    if (argc != 6 && argc != 4) {
#endif
        printf("DRC Gain Encoder to convert LPCM gain values into splines (default delay mode)\n\n");
        printf("Usage 1: DrcEnc N DRC_Gain_File_Name uniDrcBsFilename\n");
        printf("Usage 2: DrcEnc N DRC_Gain_File_Name uniDrcConfigBsFilename loudnessInfoSetBsFilename uniDrcGainBsFilename\n");
        printf("Usage 3: DrcEnc N DRC_Gain_File_Name ffDrcFilename ffLoudnessFilename uniDrcBsFilename ff_default\n");
        printf("Usage 4: DrcEnc N DRC_Gain_File_Name ffDrcFilename ffLoudnessFilename uniDrcGainBsFilename ff_split\n\n");
#if !AMD1_SYNTAX
        printf("N:                         Configuration index [1...6 or 1_ld]\n");
#else /* AMD1_SYNTAX */
        printf("N:                         Configuration index [1...20 or 1_ld]\n");
#endif /* AMD1_SYNTAX */
        printf("DRC_Gain_File_Name:        Name of file with interleaved DRC gain values [dB] in float32 format at the audio sample rate\n");
        printf("uniDrcBsFilename:          Name of file with uniDrc() bitstream for decoding with DRC tool\n");
        printf("uniDrcConfigBsFilename:    Name of file with uniDrcConfig() bitstream for decoding with DRC tool\n");
        printf("loudnessInfoSetBsFilename: Name of file with loudnessInfoSet() bitstream for decoding with DRC tool\n");
        printf("uniDrcGainBsFilename:      Name of file with uniDrcGain() bitstream for decoding with DRC tool\n");
        printf("ffDrcFilename:             Name of file with [‘chnl’, ‘dmix’, ‘udc2’, ‘udi2’, ‘udex’] ISOBMFF boxes for decoding with DRC tool\n");
        printf("ffLoudnessFilename:        Name of file with [‘ludt’] ISOBMFF box for decoding with DRC tool\n");
        printf("ff_default:                Mode string for ISOBMFF format plus uniDrc bitstream\n");
        printf("ff_split:                  Mode string for ISOBMFF format plus uniDrcGain bitstream\n");
#else /* MPEG_H_SYNTAX */
    if (argc != 6) {
        printf("DRC Gain Encoder to convert LPCM gain values into splines (default delay mode)\n");
        printf("Usage: DrcEnc N DRC_Gain_File_Name uniDrcConfigBsFilename loudnessInfoSetBsFilename uniDrcGainBsFilename\n\n");
        printf("N:                         Configuration index [1...4] (MPEG-H 3DA Syntax)\n");
        printf("DRC_Gain_File_Name:        Name of file with interleaved DRC gain values [dB] in float32 format at the audio sample rate\n");
        printf("uniDrcConfigBsFilename:    Name of file with uniDrcConfig bitstream for decoding with DRC tool (MPEG-H 3DA Syntax)\n");
        printf("loudnessInfoSetBsFilename: Name of file with loudnessInfoSet bitstream for decoding with DRC tool (MPEG-H 3DA Syntax)\n");
        printf("uniDrcGainBsFilename:      Name of file with uniDrcGain bitstream for decoding with DRC tool (MPEG-D DRC syntax)\n");
#endif
        return(UNEXPECTED_ERROR);
    }
    
#if !MPEG_H_SYNTAX
    if      (strcmp(argv[1], "1") == 0) configureEnc1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* one DRC gain sequence (2 audio channels) */
    else if (strcmp(argv[1], "1_ld") == 0) configureEnc1_ld(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* the same in low-delay mode */
    else if (strcmp(argv[1], "2") == 0) configureEnc2(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* three gain sequences  (2 audio channels) */
    else if (strcmp(argv[1], "3") == 0) configureEnc3(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* 3 channel groups, multi-band DRC, fading  (5 audio channels) */
    else if (strcmp(argv[1], "4") == 0) configureEnc4(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* downmix 5->4, channel 4 ducks channel 1,2,3 */
    else if (strcmp(argv[1], "5") == 0) configureEnc5(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* various DRCs */
    else if (strcmp(argv[1], "6") == 0) configureEnc6(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* new DIS features */
#if AMD1_SYNTAX
    else if (strcmp(argv[1], "7") == 0) configureEnc1_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 1 */
    else if (strcmp(argv[1], "8") == 0) configureEnc2_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 2 */
    else if (strcmp(argv[1], "9") == 0) configureEnc3_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 3 */
    else if (strcmp(argv[1], "10") == 0) configureEnc4_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 4 */
    else if (strcmp(argv[1], "11") == 0) configureEnc5_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 5 */
    else if (strcmp(argv[1], "12") == 0) configureEnc6_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 6 */
    else if (strcmp(argv[1], "13") == 0) configureEnc7_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 7 */
    else if (strcmp(argv[1], "14") == 0) configureEnc8_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 8 */
    else if (strcmp(argv[1], "15") == 0) configureEnc9_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 9 */
    else if (strcmp(argv[1], "16") == 0) configureEnc10_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 10 */
    else if (strcmp(argv[1], "17") == 0) configureEnc11_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 11 */
    else if (strcmp(argv[1], "18") == 0) configureEnc12_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 12 */
    else if (strcmp(argv[1], "19") == 0) configureEnc13_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 12 */
    else if (strcmp(argv[1], "20") == 0) configureEnc14_Amd1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* MPEG-D DRC AMD1 syntax bitstream 12 */
#endif /* AMD1_SYNTAX */
    else
    {
        if ((int)strspn(".xml",argv[1]) == 4)
        {
            err = parseConfigFile(argv[1], &encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension);
        }
        else
        {
#if !AMD1_SYNTAX
            fprintf(stderr,"ERROR: need to specify configuration 1, ..., 6 or 1_ld\n");
#else /* AMD1_SYNTAX */
            fprintf(stderr,"ERROR: need to specify configuration 1, ..., 20 or 1_ld\n");
#endif
            return (UNEXPECTED_ERROR);
        }
    }
#else /* MPEG_H_SYNTAX */
    if      (strcmp(argv[1], "1") == 0) configureEncMpegh1(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* mpegh3da selection process test */
    else if (strcmp(argv[1], "2") == 0) configureEncMpegh2(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* mpegh3da decoding test 1 */
    else if (strcmp(argv[1], "3") == 0) configureEncMpegh3(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* mpegh3da seection process test 2 */
    else if (strcmp(argv[1], "4") == 0) configureEncMpegh4(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* mpegh3da decoding test 2 */
    else if (strcmp(argv[1], "d") == 0) configureEncMpeghDebug(&encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension); /* mpegh3da debug test */
    else
    {
        if ((int)strspn(".xml",argv[1]) == 4)
        {
            err = parseConfigFile(argv[1], &encParams, &encConfig, &encLoudnessInfoSet, &encGainExtension);
        }
        else
        {
            fprintf(stderr,"ERROR: need to specify configuration 1, ..., 4\n");
            return (UNEXPECTED_ERROR);
        }
    }
#endif
        
    if (argc == 6) {
        bitstreamFileFormat = BITSTREAM_FILE_FORMAT_SPLIT;
#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX
    } else if (argc == 7) {
        if (strcmp(argv[6], "ff_default") == 0) bitstreamFileFormat = BITSTREAM_FILE_FORMAT_FF_DEFAULT;
        else if (strcmp(argv[6], "ff_split") == 0) bitstreamFileFormat = BITSTREAM_FILE_FORMAT_FF_SPLIT;
        else (UNEXPECTED_ERROR);
#endif
#endif
#endif
    }
        
    encParams.gainSequencePresent = FALSE;
    for (k=0; k<encConfig.drcCoefficientsUniDrcCount; k++) {
        if (encConfig.drcCoefficientsUniDrc[k].drcLocation == 1) {
            if (encConfig.drcCoefficientsUniDrc[k].gainSetCount > 0) {
                encParams.gainSequencePresent = TRUE;
            }
        }
    }
    for (k=0; k<encConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1Count; k++) {
        if (encConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[k].drcLocation == 1) {
            if (encConfig.uniDrcConfigExt.drcCoefficientsUniDrcV1[k].gainSequenceCount > 0) {
                encParams.gainSequencePresent = TRUE;
            }
        }
    }

    err = openDrcToolEncoder(&hDrcEnc, encConfig, encLoudnessInfoSet, encParams.frameSize, encParams.sampleRate, encParams.delayMode, encParams.domain);
    if (err) goto ErrorExit;
    
    allBandGainCount = getNSequences(hDrcEnc);
    
    /* set up configuration*/
    err = generateUniDrcConfig(&encConfig, &encLoudnessInfoSet, &encGainExtension);
    if (err) goto ErrorExit;
    
    interleavedGainBuffer = (float*)calloc(allBandGainCount * encParams.frameSize, sizeof(float));
    gainBuffer = (float**)calloc(allBandGainCount, sizeof(float*));
    for (k=0; k<allBandGainCount; k++) {
        gainBuffer[k] = (float*)calloc(encParams.frameSize, sizeof(float));
    }

    if (encParams.gainSequencePresent) {
        fGainIn  = fopen(argv[2], "rb");
        if (fGainIn == NULL)
        {
            fprintf(stderr, "Error: cannot open input file %s\n", argv[2]);
            return (-1);
        }
    }
    if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
        fBsOut = fopen(argv[3], "wb");
        if (fBsOut == NULL)
        {
            fprintf(stderr, "Error: cannot open output file %s\n", argv[3]);
            return (-1);
        }
    } else {
        fBsOut_uniDrcConfig = fopen(argv[3], "wb");
        if (fBsOut_uniDrcConfig == NULL)
        {
            fprintf(stderr, "Error: cannot open output file %s\n", argv[3]);
            return (-1);
        }
        fBsOut_loudnessInfoSet = fopen(argv[4], "wb");
        if (fBsOut_loudnessInfoSet == NULL)
        {
            fprintf(stderr, "Error: cannot open output file %s\n", argv[4]);
            return (-1);
        }
        fBsOut_uniDrcGain = fopen(argv[5], "wb");
        if (fBsOut_uniDrcGain == NULL)
        {
            fprintf(stderr, "Error: cannot open output file %s\n", argv[5]);
            return (-1);
        }
#ifdef DEBUG_OUTPUT_FORMAT
        {
            /* Bitrate file for uniDrcConfig and uniDrcGain payloads */
            strncpy ( txtFilenameOutputDrcBitrate, argv[5], FILENAME_MAX ) ;
            length = (int)strlen(argv[5]);
            strncpy(&txtFilenameOutputDrcBitrate[length-4],".txt",4);
            fTxtOut_drcBitrate = fopen(txtFilenameOutputDrcBitrate, "wb");
            
            /* loudnessInfoSet txt and loudnessInfo payloads*/
            strncpy ( txtFilenameOutputLoudnessInfoSet, argv[4], FILENAME_MAX ) ;
            length = (int)strlen(argv[4]);
            strncpy(&txtFilenameOutputLoudnessInfoSet[length-4],".txt",4);
            fTxtOut_loudnessInfoSet = fopen(txtFilenameOutputLoudnessInfoSet, "wb");
            
            sprintf(bsFilenameOutputLoudnessInfo, "loudnessInfoX.bit");
            length = 17;
        }
#endif
    }
    
    while (1) {
        if (encParams.gainSequencePresent) {
            /* read a frame of DRC gains*/
            size_t framesRead = fread(interleavedGainBuffer, sizeof(float), allBandGainCount * encParams.frameSize, fGainIn);
            
            /* fill with neutral gain at the end */
            if(framesRead != allBandGainCount * encParams.frameSize)
            {
                for(n=framesRead; n < allBandGainCount * encParams.frameSize; n++)
                {
#if DB_DOMAIN_GAIN_INPUT
                    interleavedGainBuffer[n] = 0.f;
#else
                    interleavedGainBuffer[n] = 1.f;
#endif
                }
                endCount++;
            }
            
            for (n=0; n<allBandGainCount * encParams.frameSize; n++)
            {
#if DB_DOMAIN_GAIN_INPUT
#if 0
                interleavedGainBuffer[n] *= 24.0f;  /* only necessary if gains are scaled by 1/24 dB/dB */
#endif
#else
                /* convert to dB */
                interleavedGainBuffer[n] = (float)(20.0*log10(interleavedGainBuffer[n]));
#endif
            }
            for (k=0; k<allBandGainCount; k++)
            {
                for (n=0; n<encParams.frameSize; n++)
                {
                    /* de-interleave */
                    gainBuffer[k][n] = interleavedGainBuffer[k + n*allBandGainCount];
                }
            }
        }
        
        err = encodeUniDrcGain(hDrcEnc, gainBuffer);
        if (err) goto ErrorExit;
        
        if (frameCount>0) /* remove encoder delay by skipping first frame */
        {
            if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
                err = writeUniDrc(hDrcEnc, writeDrcConfig, writeDrcLoudnessInfoSet, encGainExtension, bitstreamBuffer, &bitBufCount);
                if (err) goto ErrorExit;
                writeDrcConfig = FALSE;
                writeDrcLoudnessInfoSet = FALSE;
                
                fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut);
                /* copy incomplete byte to beginning */
                bitstreamBuffer[0] = bitstreamBuffer[bitBufCount/8];
                bitBufCount = bitBufCount%8;
                
            } else {
                
#ifdef DEBUG_OUTPUT_FORMAT
#if MPEG_H_SYNTAX
                if (frameCount == 1) {
                    err = writeUniDrcConfig(hDrcEnc, bitstreamBuffer, &bitBufCount);
                    if (err) goto ErrorExit;
                    if (bitBufCount%8)
                    {
                        /* set fill-bits for frame byte alignment */
                        bitstreamBuffer[bitBufCount/8] |= 0xFF>>(bitBufCount%8);
                        bitBufCount += 8;
                    }
                    fprintf(fTxtOut_drcBitrate,"%d\n", bitBufCount-bitBufCount%8);
                    fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut_uniDrcGain);
                    bitBufCount = 0;
                }
#endif
#endif /* DEBUG_OUTPUT_FORMAT */

#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX
                if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_FF_DEFAULT) {
                    err = writeUniDrc(hDrcEnc, 0, 0, encGainExtension, bitstreamBuffer, &bitBufCount);
                    if (err) goto ErrorExit;
                    
                    fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut_uniDrcGain);
                    /* copy incomplete byte to beginning */
                    bitstreamBuffer[0] = bitstreamBuffer[bitBufCount/8];
                    bitBufCount = bitBufCount%8;
                    
                } else {
#endif
#endif
#endif
                err = writeUniDrcGain(hDrcEnc, encGainExtension, bitstreamBuffer, &bitBufCount);
                if (err) goto ErrorExit;

                writeDrcConfig = FALSE;
                writeDrcLoudnessInfoSet = FALSE;
                if (bitBufCount%8)
                {
                    /* set fill-bits for frame byte alignment */
                    bitstreamBuffer[bitBufCount/8] |= 0xFF>>(bitBufCount%8);
                    bitBufCount += 8;
                }
#ifdef DEBUG_OUTPUT_FORMAT
                fprintf(fTxtOut_drcBitrate,"%d\n", bitBufCount-bitBufCount%8);
#endif
                
                fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut_uniDrcGain);
                bitBufCount = 0;
#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX
                }
#endif
#endif
#endif
            }
        }
        
        frameCount++;
#if DEBUG_NODES
        printf("Frame: %d\n", frameCount);
#endif
        if (encParams.gainSequencePresent) {
            if (feof(fGainIn) && (endCount >= 2)) break;
        }
        else
        {
            if (frameCount == encParams.frameCount) break;
        }
    }
    
    if (bitBufCount) {
        /* fill last byte with ones */
        bitstreamBuffer[0] |= 0xFF>>bitBufCount;
        if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
            fwrite(bitstreamBuffer, sizeof(unsigned char), 1, fBsOut);
        } else {
            fwrite(bitstreamBuffer, sizeof(unsigned char), 1, fBsOut_uniDrcGain);
        }
    }
    
    if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_SPLIT) {
        bitBufCount = 0;
        err = writeUniDrcConfig(hDrcEnc, bitstreamBuffer, &bitBufCount);
        if (err) goto ErrorExit;
        fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8+1, fBsOut_uniDrcConfig);
        
        bitBufCount = 0;
        err = writeLoudnessInfoSet(hDrcEnc, bitstreamBuffer, &bitBufCount);
        if (err) goto ErrorExit;
        fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8+1, fBsOut_loudnessInfoSet);
        
#ifdef DEBUG_OUTPUT_FORMAT
#if MPEG_H_SYNTAX
        {
            n = 1;
            char charBuffer[100];
            fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoCount = %d\n", encLoudnessInfoSet.loudnessInfoCount);
            for (k=0; k<encLoudnessInfoSet.loudnessInfoCount; k++) {
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoType = %d\n", encLoudnessInfoSet.loudnessInfo[k].loudnessInfoType);
                if (encLoudnessInfoSet.loudnessInfo[k].loudnessInfoType == 1 || encLoudnessInfoSet.loudnessInfo[k].loudnessInfoType == 2 ) {
                    fprintf(fTxtOut_loudnessInfoSet,"mae_groupID = %d\n", encLoudnessInfoSet.loudnessInfo[k].mae_groupID);
                } else if (encLoudnessInfoSet.loudnessInfo[k].loudnessInfoType == 3) {
                    fprintf(fTxtOut_loudnessInfoSet,"mae_groupCollectionID = %d\n", encLoudnessInfoSet.loudnessInfo[k].mae_groupPresetID);
                }
                bitBufCount = 0;
                err = writeLoudnessInfoStruct(&encLoudnessInfoSet.loudnessInfo[k], bitstreamBuffer, &bitBufCount);
                if (err) goto ErrorExit;
                
                sprintf(charBuffer, "%d.bit", n);
                strncpy(&bsFilenameOutputLoudnessInfo[length-5],charBuffer,6); /* only works for counter < 10 ? */
                
                fBsOut_loudnessInfo = fopen(bsFilenameOutputLoudnessInfo, "wb");
                fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8+1, fBsOut_loudnessInfo);
                fclose(fBsOut_loudnessInfo);
                
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoSize = %d\n", bitBufCount);
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfo = \"%s\"\n", bsFilenameOutputLoudnessInfo);
                n++;
            }
            fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoAlbumCount = %d\n", encLoudnessInfoSet.loudnessInfoAlbumCount);
            for (k=0; k<encLoudnessInfoSet.loudnessInfoAlbumCount; k++) {
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoType = %d\n", encLoudnessInfoSet.loudnessInfoAlbum[k].loudnessInfoType);
                if (encLoudnessInfoSet.loudnessInfoAlbum[k].loudnessInfoType == 1 || encLoudnessInfoSet.loudnessInfoAlbum[k].loudnessInfoType == 2 ) {
                    fprintf(fTxtOut_loudnessInfoSet,"mae_groupID = %d\n", encLoudnessInfoSet.loudnessInfoAlbum[k].mae_groupID);
                } else if (encLoudnessInfoSet.loudnessInfoAlbum[k].loudnessInfoType == 3) {
                    fprintf(fTxtOut_loudnessInfoSet,"mae_groupCollectionID = %d\n", encLoudnessInfoSet.loudnessInfoAlbum[k].mae_groupPresetID);
                }
                bitBufCount = 0;
                err = writeLoudnessInfoStruct(&encLoudnessInfoSet.loudnessInfoAlbum[k], bitstreamBuffer, &bitBufCount);
                if (err) goto ErrorExit;
                
                sprintf(charBuffer, "%d.bit", n);
                strncpy(&bsFilenameOutputLoudnessInfo[length-5],charBuffer,6); /* only works for counter < 10 ? */
                
                fBsOut_loudnessInfo = fopen(bsFilenameOutputLoudnessInfo, "wb");
                fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8+1, fBsOut_loudnessInfo);
                fclose(fBsOut_loudnessInfo);
                
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfoSize = %d\n", bitBufCount);
                fprintf(fTxtOut_loudnessInfoSet,"loudnessInfo = \"%s\"\n", bsFilenameOutputLoudnessInfo);
                n++;
            }
        }
#endif
#endif /* DEBUG_OUTPUT_FORMAT */

#if ISOBMFF_SYNTAX
#if AMD1_SYNTAX
#if !MPEG_H_SYNTAX
    } else if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_FF_DEFAULT || bitstreamFileFormat == BITSTREAM_FILE_FORMAT_FF_SPLIT) {
        bitBufCount = 0;
        err = writeIsobmffDrc(hDrcEnc, bitstreamBuffer, &bitBufCount);
        if (err) goto ErrorExit;
        fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut_uniDrcConfig);
        
        bitBufCount = 0;
        err = writeIsobmffLoudness(hDrcEnc, bitstreamBuffer, &bitBufCount);
        if (err) goto ErrorExit;
        fwrite(bitstreamBuffer, sizeof(unsigned char), bitBufCount/8, fBsOut_loudnessInfoSet);
#endif
#endif
#endif
    }
    
    goto NormalExit;
ErrorExit:
    fprintf(stderr, "ERROR\n");
NormalExit:
    if (encParams.gainSequencePresent) {
        fclose(fGainIn);
    }
    if (bitstreamFileFormat == BITSTREAM_FILE_FORMAT_DEFAULT) {
        fclose(fBsOut);
    } else {
        fclose(fBsOut_uniDrcConfig);
        fclose(fBsOut_loudnessInfoSet);
        fclose(fBsOut_uniDrcGain);
#ifdef DEBUG_OUTPUT_FORMAT
        fclose(fTxtOut_drcBitrate);
        fclose(fTxtOut_loudnessInfoSet);
#endif
    }
    closeDrcToolEncoder(&hDrcEnc);
    free(interleavedGainBuffer);
    for (k=0; k<allBandGainCount; k++) {
        free(gainBuffer[k]);
    }
    free(gainBuffer);
    return 0;
}

