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
 
 Copyright (c) ISO/IEC 2016.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmlParserConfig.h"

int parseConfigFile(char const * const filename, EncParams* encParams, UniDrcConfig* encUnDrcConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension)
{
    int ret = 0;
    XMLREADER_RETURN error = XMLREADER_NO_ERROR;
    XMLREADER_INSTANCE_HANDLE hXmlReader;
    
    error = xmlReaderLib_New(&hXmlReader, filename);
    if (error != XMLREADER_NO_ERROR) {
        return 1;
    }
    
    ret = parseConfig(hXmlReader, encParams, encUnDrcConfig, encLoudnessInfoSet);
    
    error = xmlReaderLib_Delete(hXmlReader);
    if (error != XMLREADER_NO_ERROR) {
        return 1;
    }
    
    return ret;
}

int parseConfig(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, UniDrcConfig* uniDrcConfig, LoudnessInfoSet* loudnessInfoSet)
{
    unsigned int downmixInstructionsCount = 0, drcInstructionsUniDrcCount = 0, drcCoefficientsUniDrcCount = 0, loudnessInfoCount = 0, loundessInfoAlbumCount = 0;
    int ret = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    if(uniDrcConfig) {
        memset(uniDrcConfig, 0, sizeof(UniDrcConfig) ); /*TODO*/
    } else {
        fprintf(stderr, "Pointer uniDrcConfig is not allowed to be NULL\n");
        return -1;
    }
    
    if(loudnessInfoSet) {
        memset(loudnessInfoSet, 0, sizeof(LoudnessInfoSet) ); /*TODO*/
    } else {
        fprintf(stderr, "Pointer loudnessInfoSet is not allowed to be NULL\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "config") != 0)
    {
        fprintf(stderr, "No valid XML configuration information\n %s", content);
        return -1;
    }
    
    /* Parse configuration */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        
        if(strcmp(content, "uniDrcConfig") == 0) {
            
            /* Parse uniDrcConfig */
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if( strcmp(content, "drcCoefficientsUniDrc") == 0 )
                {
                    if (drcCoefficientsUniDrcCount>=DRC_COEFF_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <drcCoefficientsUniDrc> allowed\n", DRC_COEFF_COUNT_MAX);
                        return -1;
                    }
                    
                    ret = parseDrcCoefficientsUniDrc(hXmlReader, encParams, &uniDrcConfig->drcCoefficientsUniDrc[drcCoefficientsUniDrcCount], 1);
                    if (ret != 0) {
                        fprintf(stderr, "parseDrcCoefficientsUniDrc returns with an error\n");
                        return -1;
                    }
                    drcCoefficientsUniDrcCount++;
                    
                } else if ( strcmp(content, "drcInstructionsUniDrc") == 0 ) {
                    if (drcInstructionsUniDrcCount>=DRC_INSTRUCTIONS_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <drcInstructionsUniDrc> allowed\n", DRC_INSTRUCTIONS_COUNT_MAX);
                        return -1;
                    }
                    
                    ret = parseDrcInstructionsUniDrc(hXmlReader, &uniDrcConfig->drcInstructionsUniDrc[drcInstructionsUniDrcCount], 0);
                    if (ret != 0) {
                        fprintf(stderr, "parseDrcInstructionsUniDrc returns with an error\n");
                        return -1;
                    }
                    
                    drcInstructionsUniDrcCount++;
#if MPEG_H_SYNTAX
                    /* Used in loudness-leveling extension */
                    ++uniDrcConfig->drcInstructionsUniDrcCount;
#endif
                    
                } else if ( strcmp(content, "downmixInstructions") == 0 ) {
                    if (downmixInstructionsCount>=DOWNMIX_INSTRUCTION_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <downmixInstructions> allowed\n", DOWNMIX_INSTRUCTION_COUNT_MAX);
                        return -1;
                    }
                    
                    ret = parseDownmixInstructions(hXmlReader, &uniDrcConfig->downmixInstructions[downmixInstructionsCount], 0);
                    if (ret != 0) {
                        fprintf(stderr, "parseUniDrcInstructions returns with an error\n");
                        return -1;
                    }
                    
                    downmixInstructionsCount++;
                }
                else if ( strcmp(content, "channelLayout") == 0 ) {
                    int speakerPositionCount = 0;
                    
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("baseChannelCount", content) == 0 ) {
                            uniDrcConfig->channelLayout.baseChannelCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("layoutSignalingPresent", content) == 0 ) {
                            uniDrcConfig->channelLayout.layoutSignalingPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("definedLayout", content) == 0 ) {
                            uniDrcConfig->channelLayout.definedLayout = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("speakerPosition", content) == 0 ) {
                            if (speakerPositionCount>=CHANNEL_COUNT_MAX) {
                                fprintf(stderr, "There are not more than %i <speakerPosition> allowed\n", CHANNEL_COUNT_MAX);
                                return -1;
                            }
                            uniDrcConfig->channelLayout.speakerPosition[speakerPositionCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            speakerPositionCount++;
                        }
                    }
                    
                } else if ( strcmp(content, "uniDrcConfigExtension") == 0 ) {
                    uniDrcConfig->uniDrcConfigExtPresent = 1;
                    ret = parseUniDrcConfigExtension(hXmlReader, encParams, uniDrcConfig);
                    if (ret != 0) {
                        fprintf(stderr, "parseUniDrcConfigExtension returns with an error\n");
                        return -1;
                    }
                }
            }
        } else if(strcmp(content, "loudnessInfoSet") == 0) {        
            
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                
                if(strcmp(content, "loudnessInfo") == 0) {
                    
                    if (loudnessInfoCount>=LOUDNESS_INFO_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <loudnessInfo> allowed\n", LOUDNESS_INFO_COUNT_MAX);
                        return -1;
                    }
                    
                    ret = parseLoudnessInfo(hXmlReader, &loudnessInfoSet->loudnessInfo[loudnessInfoCount], 0);
                    if (ret != 0) {
                        fprintf(stderr, "parseLoudnessInfo returns with an error\n");
                        return -1;
                    }
                    
                    loudnessInfoCount++;
                    
                } else if(strcmp(content, "loudnessInfoAlbum") == 0) {
                    
                    if (loudnessInfoCount>=LOUDNESS_INFO_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <loudnessInfoAlbum> allowed\n", LOUDNESS_INFO_COUNT_MAX);
                        return -1;
                    }
                    
                    ret = parseLoudnessInfo(hXmlReader, &loudnessInfoSet->loudnessInfoAlbum[loundessInfoAlbumCount], 0);
                    if (ret != 0) {
                        fprintf(stderr, "parseLoudnessInfo returns with an error\n");
                        return -1;
                    }
                    
                    loundessInfoAlbumCount++;
                    
                } else if(strcmp(content, "loudnessInfoSetExtension") == 0) {
                    
                    /* set extension type; note that this only works since there is just one extension defined up to now */
                    loudnessInfoSet->loudnessInfoSetExtPresent = 1;
                    loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[0] = 1;
                    loudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtType[1] = 0;
                    
                    ret = parseLoudnessInfoSetExtension(hXmlReader, &loudnessInfoSet->loudnessInfoSetExtension);
                    if (ret != 0) {
                        fprintf(stderr, "parseLoudnessInfo returns with an error\n");
                        return -1;
                    }                  
                   
                }
            }
        } else if(strcmp(content, "externalConfig") == 0) {
            
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                
                if ( strcmp("frameSize", content) == 0 ) {
                    encParams->frameSize = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    uniDrcConfig->drcCoefficientsUniDrc[0].drcFrameSizePresent = 1;
                    uniDrcConfig->drcCoefficientsUniDrc[0].drcFrameSize = encParams->frameSize;
                } else if ( strcmp("sampleRate", content) == 0 ) {
                    encParams->sampleRate = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    uniDrcConfig->sampleRatePresent = 1;
                    uniDrcConfig->sampleRate = encParams->sampleRate;
                } else if ( strcmp("frameCount", content) == 0 ) {
                    encParams->frameCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("domain", content) == 0 ) {
                    encParams->domain = atoi( xmlReaderLib_GetContent(hXmlReader) );
                /*} else if ( strcmp("loudnessOnly", content) == 0 ) {
                    encParams->loudnessOnly = atoi( xmlReaderLib_GetContent(hXmlReader) );*/
                } else if ( strcmp("parametricDrcOnly", content) == 0 ) {
                    encParams->parametricDrcOnly = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("frameCount", content) == 0 ) {
                    encParams->frameCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
        }
    }
    
    uniDrcConfig->drcCoefficientsUniDrcCount=drcCoefficientsUniDrcCount;
    uniDrcConfig->downmixInstructionsCount=downmixInstructionsCount;
#if MPEG_H_SYNTAX
    uniDrcConfig->drcInstructionsUniDrcCount = drcInstructionsUniDrcCount > uniDrcConfig->drcInstructionsUniDrcCount
        ? drcInstructionsUniDrcCount
        : uniDrcConfig->drcInstructionsUniDrcCount;
#else
    uniDrcConfig->drcInstructionsUniDrcCount=drcInstructionsUniDrcCount;
#endif
    loudnessInfoSet->loudnessInfoCount = loudnessInfoCount;
    loudnessInfoSet->loudnessInfoAlbumCount = loundessInfoAlbumCount;
    
    return 0;
}

int parseDrcCoefficientsUniDrc(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, DrcCoefficientsUniDrc *drcCoefficientsUniDrc, int version)
{
    unsigned int characteristicLeftCount = 0, characteristicRightCount = 0, characteristicNodeCount = 0, shapeFilterCount = 0, gainSetCount = 0;
    const char * content;
    int ret = 0;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "drcCoefficientsUniDrc") != 0)
    {
        fprintf(stderr, "No valid XML information for drcCoefficientsUniDrc\n %s", content);
        return -1;
    }
    
    if(! drcCoefficientsUniDrc) {
        fprintf(stderr, "Pointer drcCoefficientsUniDrc is not allowed to be NULL\n");
        return -1;
    }
    
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if ( strcmp("drcFrameSizePresent", content) == 0 ) {
            drcCoefficientsUniDrc->drcFrameSizePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcFrameSize", content) == 0 ) {
            if (drcCoefficientsUniDrc->drcFrameSizePresent) { /* only overwrite default if present == 1 */
                drcCoefficientsUniDrc->drcFrameSize = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else  if ( strcmp("drcLocation", content) == 0 ) {
            drcCoefficientsUniDrc->drcLocation = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else  if ( strcmp("drcCharacteristicLeftPresent", content) == 0 ) {
            drcCoefficientsUniDrc->drcCharacteristicLeftPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else  if ( strcmp("drcCharacteristicLeft", content) == 0 ) {
            if (drcCoefficientsUniDrc->drcCharacteristicLeftPresent) {
                if (characteristicLeftCount>=SPLIT_CHARACTERISTIC_COUNT_MAX+1) {
                    fprintf(stderr, "There are not more than %i <shapeFilter> allowed\n", SPLIT_CHARACTERISTIC_COUNT_MAX+1);
                    return -1;
                }
                characteristicNodeCount = 0;
                xmlReaderLib_GoToChilds(hXmlReader);
                while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                    if( strcmp("characteristicFormat", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].characteristicFormat = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsIoRatio", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].bsIoRatio = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsGain", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].bsGain = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsExp", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].bsExp = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("flipSign", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].flipSign = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("node", content) == 0 ) {
                        xmlReaderLib_GoToChilds(hXmlReader);
                        while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                            if( strcmp("nodeLevel", content) == 0 ) {
                                drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].nodeLevel[characteristicNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            } else if( strcmp("nodeGain", content) == 0 ) {
                                drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].nodeGain[characteristicNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            }
                        }
                        characteristicNodeCount++;
                    }
                }
                drcCoefficientsUniDrc->splitCharacteristicLeft[characteristicLeftCount+1].characteristicNodeCount = characteristicNodeCount - 1;
                characteristicLeftCount++;
            }
        } else  if ( strcmp("drcCharacteristicRightPresent", content) == 0 ) {
            drcCoefficientsUniDrc->drcCharacteristicRightPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else  if ( strcmp("drcCharacteristicRight", content) == 0 ) {
            if (drcCoefficientsUniDrc->drcCharacteristicRightPresent) {
                if (characteristicRightCount>=SPLIT_CHARACTERISTIC_COUNT_MAX+1) {
                    fprintf(stderr, "There are not more than %i <shapeFilter> allowed\n", SPLIT_CHARACTERISTIC_COUNT_MAX+1);
                    return -1;
                }
                characteristicNodeCount = 0;
                xmlReaderLib_GoToChilds(hXmlReader);
                while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                    if( strcmp("characteristicFormat", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].characteristicFormat = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsIoRatio", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].bsIoRatio = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsGain", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].bsGain = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("bsExp", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].bsExp = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("flipSign", content) == 0 ) {
                        drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].flipSign = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("node", content) == 0 ) {
                        xmlReaderLib_GoToChilds(hXmlReader);
                        while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                            if( strcmp("nodeLevel", content) == 0 ) {
                                drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].nodeLevel[characteristicNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            } else if( strcmp("nodeGain", content) == 0 ) {
                                drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].nodeGain[characteristicNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            }
                        }
                        characteristicNodeCount++;
                    }
                }
                drcCoefficientsUniDrc->splitCharacteristicRight[characteristicRightCount+1].characteristicNodeCount = characteristicNodeCount - 1;
                characteristicRightCount++;
            }
        } else  if ( strcmp("shapeFiltersPresent", content) == 0 ) {
            drcCoefficientsUniDrc->shapeFiltersPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else  if ( strcmp("shapeFilter", content) == 0 ) {
            if (drcCoefficientsUniDrc->shapeFiltersPresent) {
                if (shapeFilterCount>=SHAPE_FILTER_COUNT_MAX+1) {
                    fprintf(stderr, "There are not more than %i <shapeFilter> allowed\n", SHAPE_FILTER_COUNT_MAX+1);
                    return -1;
                }
                xmlReaderLib_GoToChilds(hXmlReader);
                while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                    if( strcmp("lfCutFilterPresent", content) == 0 ) {
                        drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfCutFilterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("lfBoostFilterPresent", content) == 0 ) {
                        drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfBoostFilterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("hfCutFilterPresent", content) == 0 ) {
                        drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfCutFilterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("hfBoostFilterPresent", content) == 0 ) {
                        drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfBoostFilterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if( strcmp("lfCutParams", content) == 0 ) {
                        if (drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfCutFilterPresent) {
                            xmlReaderLib_GoToChilds(hXmlReader);
                            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                                if( strcmp("cornerFreqIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfCutParams.cornerFreqIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                } else if( strcmp("filterStrengthIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfCutParams.filterStrengthIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            }
                        }
                    } else if( strcmp("lfBoostParams", content) == 0 ) {
                        if (drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfBoostFilterPresent) {
                            xmlReaderLib_GoToChilds(hXmlReader);
                            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                                if( strcmp("cornerFreqIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfBoostParams.cornerFreqIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                } else if( strcmp("filterStrengthIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].lfBoostParams.filterStrengthIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            }
                        }
                    } else if( strcmp("hfCutParams", content) == 0 ) {
                        if (drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfCutFilterPresent) {
                            xmlReaderLib_GoToChilds(hXmlReader);
                            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                                if( strcmp("cornerFreqIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfCutParams.cornerFreqIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                } else if( strcmp("filterStrengthIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfCutParams.filterStrengthIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            }
                        }
                    } else if( strcmp("hfBoostParams", content) == 0 ) {
                        if (drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfBoostFilterPresent) {
                            xmlReaderLib_GoToChilds(hXmlReader);
                            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                                if( strcmp("cornerFreqIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfBoostParams.cornerFreqIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                } else if( strcmp("filterStrengthIndex", content) == 0 ) {
                                    drcCoefficientsUniDrc->shapeFilterBlockParams[shapeFilterCount+1].hfBoostParams.filterStrengthIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            }
                        }
                    }
                }
                shapeFilterCount++;
            }
        } else  if ( strcmp("gainSequenceCount", content) == 0 ) {
            drcCoefficientsUniDrc->gainSequenceCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if( strcmp("gainSetParams", content) == 0 ) {
            if (gainSetCount>=SEQUENCE_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <gainSetParams> allowed\n", SEQUENCE_COUNT_MAX);
                return -1;
            }
            drcCoefficientsUniDrc->gainSetParams[gainSetCount].deltaTmin = getDeltaTmin(encParams->sampleRate);
            ret = parseGainSetParam(hXmlReader, &drcCoefficientsUniDrc->gainSetParams[gainSetCount], 0);
            if (ret != 0) {
                fprintf(stderr, "parseGainSetParam returns with an error\n");
                return -1;
            }
            gainSetCount++;
        }
    }
    drcCoefficientsUniDrc->characteristicLeftCount = characteristicLeftCount;
    drcCoefficientsUniDrc->characteristicRightCount = characteristicRightCount;
    drcCoefficientsUniDrc->shapeFilterCount = shapeFilterCount;
    drcCoefficientsUniDrc->gainSetCount = gainSetCount;

    return 0;
}

int parseGainSetParam(XMLREADER_INSTANCE_HANDLE hXmlReader, GainSetParams *gainSetParams, int version)
{
    unsigned int gainParamsCount = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "gainSetParams") != 0)
    {
        fprintf(stderr, "No valid XML information for sequenceParams\n %s", content);
        return -1;
    }
    
    if(! gainSetParams) {
        fprintf(stderr, "Pointer gainSetParams is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "gainCodingProfile") == 0 ) {
            gainSetParams->gainCodingProfile = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("gainInterpolationType", content) == 0 ) {
            gainSetParams->gainInterpolationType = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("fullFrame", content) == 0 ) {
            gainSetParams->fullFrame = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("timeAlignment", content) == 0 ) {
            gainSetParams->timeAlignment = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("timeDeltaMinPresent", content) == 0 ) {
            gainSetParams->timeDeltaMinPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("timeDeltaMin", content) == 0 ) {
            if (gainSetParams->timeDeltaMinPresent) { /* only overwrite default if preset == 1 */
                gainSetParams->deltaTmin = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("drcBandType", content) == 0 ) {
            gainSetParams->drcBandType = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("gainParams", content) == 0 ) {
            if (gainParamsCount>=BAND_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <GainParams> allowed\n", BAND_COUNT_MAX);
                return -1;
            }
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if( strcmp("gainSequenceIndex", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].gainSequenceIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("drcCharacteristicPresent", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].drcCharacteristicPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("drcCharacteristicFormatIsCICP", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].drcCharacteristicFormatIsCICP = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("drcCharacteristic", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].drcCharacteristic = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("drcCharacteristicLeftIndex", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].drcCharacteristicLeftIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("drcCharacteristicRightIndex", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].drcCharacteristicRightIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("crossoverFreqIndex", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].crossoverFreqIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("startSubBandIndex", content) == 0 ) {
                    gainSetParams->gainParams[gainParamsCount].startSubBandIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
            gainParamsCount++;
        }
    }
    gainSetParams->bandCount = gainParamsCount;
    return 0;
}

int parseDrcInstructionsUniDrc(XMLREADER_INSTANCE_HANDLE hXmlReader, DrcInstructionsUniDrc *drcInstructionUniDrc, int version)
{
    unsigned int additionalDownmixCount = 0;
    unsigned int drcChannelCount = 0;
    unsigned int nDrcChannelGroups = 0;
    unsigned int num_gainModifiers = 0;
    unsigned int num_duckingModifiers = 0;
    unsigned int k;
    int tmp, match, uniqueIndex[CHANNEL_COUNT_MAX];
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "drcInstructionsUniDrc") != 0)
    {
        fprintf(stderr, "No valid XML information for drcInstructionsUniDrc\n %s", content);
        return -1;
    }
    
    if(!drcInstructionUniDrc) {
        fprintf(stderr, "Pointer drcInstructionsUniDrc is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "drcSetId") == 0 ) {
            drcInstructionUniDrc->drcSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcSetComplexityLevel", content) == 0 ) {
            drcInstructionUniDrc->drcSetComplexityLevel = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixIdPresent", content) == 0 ) {
            drcInstructionUniDrc->downmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixId", content) == 0 ) {
            drcInstructionUniDrc->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcApplyToDownmix", content) == 0 ) {
            drcInstructionUniDrc->drcApplyToDownmix = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixIdPresent", content) == 0 ) {
            drcInstructionUniDrc->additionalDownmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixId", content) == 0 ) {
            if (drcInstructionUniDrc->additionalDownmixIdPresent) { /* only overwrite if present == 1 */
                if (additionalDownmixCount>=ADDITIONAL_DOWNMIX_ID_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalDownmixId> allowed\n", ADDITIONAL_DOWNMIX_ID_MAX);
                    return -1;
                }
                drcInstructionUniDrc->additionalDownmixId[additionalDownmixCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalDownmixCount++;
            }
        } else if ( strcmp("drcLocation", content) == 0 ) {
            drcInstructionUniDrc->drcLocation = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("dependsOnDrcSetPresent", content) == 0 ) {
            drcInstructionUniDrc->dependsOnDrcSetPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("dependsOnDrcSet", content) == 0 ) {
            if (drcInstructionUniDrc->dependsOnDrcSetPresent) { /* only overwrite if present == 1 */
                drcInstructionUniDrc->dependsOnDrcSet = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("noIndependentUse", content) == 0 ) {
            drcInstructionUniDrc->noIndependentUse = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("requiresEq", content) == 0 ) {
            drcInstructionUniDrc->requiresEq = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcSetEffect", content) == 0 ) {
            drcInstructionUniDrc->drcSetEffect = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("gainSetIndex", content) == 0 ) {
            if (drcChannelCount>=CHANNEL_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <gainSetIndex> allowed\n", CHANNEL_COUNT_MAX);
                return -1;
            }
            if (nDrcChannelGroups>=CHANNEL_GROUP_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i channel groups allowed\n", CHANNEL_GROUP_COUNT_MAX);
                return -1;
            }
            drcInstructionUniDrc->gainSetIndex[drcChannelCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
            /* get number of drc channel groups */
            tmp = drcInstructionUniDrc->gainSetIndex[drcChannelCount];
            if (tmp >= 0) {
                match = 0;
                for (k=0; k < nDrcChannelGroups; k++) {
                    if (uniqueIndex[k] == tmp) match = 1;
                }
                if (match == 0)
                {
                    uniqueIndex[nDrcChannelGroups] = tmp;
                    nDrcChannelGroups++;
                }
            }
            drcChannelCount++;
        } else if ( strcmp("gainModifiers", content) == 0 ) {
            unsigned int num_gainModifierBands = 0;
            if (num_gainModifiers>=CHANNEL_GROUP_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <gainModifiers> allowed\n", CHANNEL_GROUP_COUNT_MAX);
                return -1;
            }
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if (version == 0) {
                    if ( strcmp("gainScalingPresent", content) == 0 ) {
                        drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[0] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("attenuationScaling", content) == 0 ) {
                        if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[0]) {
                            drcInstructionUniDrc->gainModifiers[num_gainModifiers].attenuationScaling[0] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    } else if ( strcmp("amplificationScaling", content) == 0 ) {
                        if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[0]) {
                            drcInstructionUniDrc->gainModifiers[num_gainModifiers].amplificationScaling[0] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    } else if ( strcmp("gainOffsetPresent", content) == 0 ) {
                        drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffsetPresent[0] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainOffset", content) == 0 ) {
                        if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffsetPresent[0]) {
                            drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffset[0] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    }
                } else {
                    if ( strcmp("shapeFilterPresent", content) == 0 ) {
                        drcInstructionUniDrc->gainModifiers[num_gainModifiers].shapeFilterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("shapeFilterIndex", content) == 0 ) {
                        drcInstructionUniDrc->gainModifiers[num_gainModifiers].shapeFilterIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainModifierBand", content) == 0 ) {
                        if (num_gainModifierBands>=BAND_COUNT_MAX) {
                            fprintf(stderr, "There are not more than %i <gainModifierBand> allowed\n", BAND_COUNT_MAX);
                            return -1;
                        }
                        
                        xmlReaderLib_GoToChilds(hXmlReader);
                        while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                            if ( strcmp("gainScalingPresent", content) == 0 ) {
                                drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            } else if ( strcmp("attenuationScaling", content) == 0 ) {
                                if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[num_gainModifierBands]) {
                                    drcInstructionUniDrc->gainModifiers[num_gainModifiers].attenuationScaling[num_gainModifierBands] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            } else if ( strcmp("amplificationScaling", content) == 0 ) {
                                if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainScalingPresent[num_gainModifierBands]) {
                                    drcInstructionUniDrc->gainModifiers[num_gainModifiers].amplificationScaling[num_gainModifierBands] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            } else if ( strcmp("gainOffsetPresent", content) == 0 ) {
                                drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffsetPresent[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            } else if ( strcmp("gainOffset", content) == 0 ) {
                                if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffsetPresent[num_gainModifierBands]) {
                                    drcInstructionUniDrc->gainModifiers[num_gainModifiers].gainOffset[num_gainModifierBands] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            } else if ( strcmp("targetCharacteristicLeftPresent", content) == 0 ) {
                                drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicLeftPresent[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            } else if ( strcmp("targetCharacteristicLeftIndex", content) == 0 ) {
                                if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicLeftPresent[num_gainModifierBands]) {
                                    drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicLeftIndex[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            } else if ( strcmp("targetCharacteristicRightPresent", content) == 0 ) {
                                drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicRightPresent[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            } else if ( strcmp("targetCharacteristicRightIndex", content) == 0 ) {
                                if (drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicRightPresent[num_gainModifierBands]) {
                                    drcInstructionUniDrc->gainModifiers[num_gainModifiers].targetCharacteristicRightIndex[num_gainModifierBands] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                                }
                            }
                        }
                        num_gainModifierBands++;
                    }
                }
            }
            num_gainModifiers++;
        } else if ( strcmp("duckingModifiers", content) == 0 ) {
            if (num_duckingModifiers>=CHANNEL_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <duckingModifiers> allowed\n", CHANNEL_COUNT_MAX);
                return -1;
            }
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("duckingScaling", content) == 0 ) {
                    drcInstructionUniDrc->duckingModifiersForChannel[num_duckingModifiers].duckingScaling = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if (strcmp("duckingScalingPresent", content) == 0 ) {
                    drcInstructionUniDrc->duckingModifiersForChannel[num_duckingModifiers].duckingScalingPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
            num_duckingModifiers++;
        } else if ( strcmp("limiterPeakTargetPresent", content) == 0 ) {
            drcInstructionUniDrc->limiterPeakTargetPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("limiterPeakTarget", content) == 0 ) {
            if (drcInstructionUniDrc->limiterPeakTargetPresent) { /* only overwrite if present == 1*/
                drcInstructionUniDrc->limiterPeakTarget = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("drcSetTargetLoudnessPresent", content) == 0 ) {
            drcInstructionUniDrc->drcSetTargetLoudnessPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcSetTargetLoudnessValueUpper", content) == 0 ) {
            if (drcInstructionUniDrc->drcSetTargetLoudnessPresent) { /* only overwrite if present == 1*/
                drcInstructionUniDrc->drcSetTargetLoudnessValueUpper = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("drcSetTargetLoudnessValueLowerPresent", content) == 0 ) {
            drcInstructionUniDrc->drcSetTargetLoudnessValueLowerPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcSetTargetLoudnessValueLower", content) == 0 ) {
            if (drcInstructionUniDrc->drcSetTargetLoudnessValueLowerPresent) { /* only overwrite if present == 1*/
                drcInstructionUniDrc->drcSetTargetLoudnessValueLower = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
#if MPEG_H_SYNTAX
        } else if ( strcmp("drcInstructionsType", content) == 0 ) {
            drcInstructionUniDrc->drcInstructionsType = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("mae_groupID", content) == 0 ) {
            drcInstructionUniDrc->mae_groupID = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("mae_groupPresetID", content) == 0 ) {
            drcInstructionUniDrc->mae_groupPresetID = atoi( xmlReaderLib_GetContent(hXmlReader) );
        }
#else
        }
#endif
    }
    drcInstructionUniDrc->drcChannelCount          = drcChannelCount;
    drcInstructionUniDrc->nDrcChannelGroups        = nDrcChannelGroups;
    drcInstructionUniDrc->additionalDownmixIdCount = additionalDownmixCount;
    return 0;
}

int parseDownmixInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, DownmixInstructions *downmixInstruction, int version)
{
    unsigned int downmixCoefficientsCount = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "downmixInstructions") != 0)
    {
        fprintf(stderr, "No valid XML information for downmixInstruction\n %s", content);
        return -1;
    }
    
    if(!downmixInstruction) {
        fprintf(stderr, "Pointer downmixInstruction is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if ( strcmp("downmixId", content) == 0 ) {
            downmixInstruction->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("targetChannelCount", content) == 0 ) {
            downmixInstruction->targetChannelCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("targetLayout", content) == 0 ) {
            downmixInstruction->targetLayout = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixCoefficientsPresent", content) == 0 ) {
            downmixInstruction->downmixCoefficientsPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixCoefficient", content) == 0 ) {
            if (downmixInstruction->downmixCoefficientsPresent) { /* only overwrite default if present == 1 */
                if (downmixCoefficientsCount>=DOWNMIX_COEFF_COUNT_MAX) {
                    fprintf(stderr, "There are not more than %i <downmixCoefficient> allowed\n", DOWNMIX_COEFF_COUNT_MAX);
                    return -1;
                }
                downmixInstruction->downmixCoefficient[downmixCoefficientsCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                downmixCoefficientsCount++;
            }
        }
    }

    return 0;
}

int parseLoudnessInfo(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudnessInfo *loudnessInfo, int version)
{
    const char * content;
    int num_loudnessMeasure = 0;

    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp("drcSetId", content) == 0 ) {
            loudnessInfo->drcSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSetId", content) == 0 ) {
            loudnessInfo->eqSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixId", content) == 0 ) {
            loudnessInfo->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("samplePeakLevelPresent", content) == 0 ) {
            loudnessInfo->samplePeakLevelPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("samplePeakLevel", content) == 0 ) {
            if (loudnessInfo->samplePeakLevelPresent) {
                loudnessInfo->samplePeakLevel = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("truePeakLevelPresent", content) == 0 ) {
            loudnessInfo->truePeakLevelPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("truePeakLevel", content) == 0 ) {
            if (loudnessInfo->truePeakLevelPresent) {
                loudnessInfo->truePeakLevel = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("truePeakLevelMeasurementSystem", content) == 0 ) {
            loudnessInfo->truePeakLevelMeasurementSystem = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("truePeakLevelReliability", content) == 0 ) {
            loudnessInfo->truePeakLevelReliability = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("loudnessMeasure", content) == 0 ) {
            if (num_loudnessMeasure>=MEASUREMENT_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <LoudnessMeasure> allowed\n", MEASUREMENT_COUNT_MAX);
                return -1;
            }
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if( strcmp("methodDefinition", content) == 0 )
                {
                    loudnessInfo->loudnessMeasure[num_loudnessMeasure].methodDefinition = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("measurementSystem", content) == 0 ) {
                    loudnessInfo->loudnessMeasure[num_loudnessMeasure].measurementSystem = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("methodValue", content) == 0 ) {
                    loudnessInfo->loudnessMeasure[num_loudnessMeasure].methodValue = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("reliability", content) == 0 ) {
                    loudnessInfo->loudnessMeasure[num_loudnessMeasure].reliability = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
            num_loudnessMeasure++;
#if MPEG_H_SYNTAX
        } else if ( strcmp("loudnessInfoType", content) == 0 ) {
            loudnessInfo->loudnessInfoType = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("mae_groupID", content) == 0 ) {
            loudnessInfo->mae_groupID = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("mae_groupPresetID", content) == 0 ) {
            loudnessInfo->mae_groupPresetID = atoi( xmlReaderLib_GetContent(hXmlReader) );
        }
#else
        }
#endif
        loudnessInfo->measurementCount=num_loudnessMeasure;
    }
    return 0;
}

int parseUniDrcConfigExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, UniDrcConfig *uniDrcConfig)
{
    unsigned int configExtensionCount = 0, parametricDrcInstructionsCount = 0, downmixInstructionsCount = 0, drcCoefficientsUniDrcCount = 0, drcInstructionsUniDrcCount = 0, loudEqInstructionsCount = 0, eqInstructionsCount = 0;
#ifdef LEVELING_SUPPORT
    unsigned int loudnessLevelingExtensionPresent = 0;
#endif
    const char * content;
    int ret = 0;
    UniDrcConfigExt *uniDrcConfigExt = &uniDrcConfig->uniDrcConfigExt;
#if MPEG_H_SYNTAX
    drcInstructionsUniDrcCount = uniDrcConfig->drcInstructionsUniDrcCount;
#endif
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "uniDrcConfigExtension") != 0)
    {
        fprintf(stderr, "No valid XML information for uniDrcConfigExtension\n %s", content);
        return -1;
    }
    
    if(! uniDrcConfigExt) {
        fprintf(stderr, "Pointer uniDrcConfigExt is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "drcCoefficientsParametricDrc") == 0 ) {
            
            int gainSetCount = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("drcLocation", content) == 0 ) {
                    uniDrcConfigExt->drcCoefficientsParametricDrc.drcLocation = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("parametricDrcFrameSize", content) == 0 ) {
                    uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcFrameSize = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("parametricDrcDelayMaxPresent", content) == 0 ) {
                    uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcDelayMaxPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("parametricDrcDelayMax", content) == 0 ) {
                    uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcDelayMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("resetParametricDrc", content) == 0 ) {
                    uniDrcConfigExt->drcCoefficientsParametricDrc.resetParametricDrc = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("parametricDrcGainSetParams", content) == 0 ) {
                    if (gainSetCount>=SEQUENCE_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <gainSetParams> allowed\n", SEQUENCE_COUNT_MAX);
                        return -1;
                    }
                    ret = parseParametricDrcGainSetParam(hXmlReader, &uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcGainSetParams[gainSetCount]);
                    if (ret != 0) {
                        fprintf(stderr, "parseParametricDrcGainSetParam returns with an error\n");
                        return -1;
                    }
                    gainSetCount++;
                }
            }
            uniDrcConfigExt->drcCoefficientsParametricDrc.parametricDrcGainSetCount = gainSetCount;
        } else if ( strcmp("parametricDrcInstructions", content) == 0 ) {
            
            ret = parseParametricDrcInstructions(hXmlReader, &uniDrcConfigExt->parametricDrcInstructions[parametricDrcInstructionsCount]);
            if (ret != 0) {
                fprintf(stderr, "parseParametricDrcInstructions returns with an error\n");
                return -1;
            }
            parametricDrcInstructionsCount++;
            
        } else if ( strcmp(content, "downmixInstructions") == 0 ) {
            if (downmixInstructionsCount>=DOWNMIX_INSTRUCTION_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <downmixInstructions> allowed\n", DOWNMIX_INSTRUCTION_COUNT_MAX);
                return -1;
            }
            
            uniDrcConfigExt->downmixInstructionsV1Present = 1;
            ret = parseDownmixInstructions(hXmlReader, &uniDrcConfigExt->downmixInstructionsV1[downmixInstructionsCount], 1);
            if (ret != 0) {
                fprintf(stderr, "parseUniDrcInstructions returns with an error\n");
                return -1;
            }
            
            downmixInstructionsCount++;
          
        } else if( strcmp(content, "drcCoefficientsUniDrc") == 0 ) {
            
            if (drcCoefficientsUniDrcCount>=DRC_COEFF_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <drcCoefficientsUniDrc> allowed\n", DRC_COEFF_COUNT_MAX);
                return -1;
            }
            
            uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
            ret = parseDrcCoefficientsUniDrc(hXmlReader, encParams, &uniDrcConfigExt->drcCoefficientsUniDrcV1[drcCoefficientsUniDrcCount], 1);
            if (ret != 0) {
                fprintf(stderr, "parseDrcCoefficientsUniDrc returns with an error\n");
                return -1;
            }
            drcCoefficientsUniDrcCount++;
            
        } else if ( strcmp(content, "drcInstructionsUniDrc") == 0 ) {
            if (drcInstructionsUniDrcCount>=DRC_INSTRUCTIONS_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <drcInstructionsUniDrc> allowed\n", DRC_INSTRUCTIONS_COUNT_MAX);
                return -1;
            }
            
            uniDrcConfigExt->drcCoeffsAndInstructionsUniDrcV1Present = 1;
            ret = parseDrcInstructionsUniDrc(hXmlReader, &uniDrcConfigExt->drcInstructionsUniDrcV1[drcInstructionsUniDrcCount], 1);
            if (ret != 0) {
                fprintf(stderr, "parseDrcInstructionsUniDrc returns with an error\n");
                return -1;
            }
            
            drcInstructionsUniDrcCount++;
            
        } else if ( strcmp(content, "loudEqInstructions") == 0 ) {
            if (loudEqInstructionsCount>=LOUD_EQ_INSTRUCTIONS_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <loudEqInstructions> allowed\n", LOUD_EQ_INSTRUCTIONS_COUNT_MAX);
                return -1;
            }
            
            uniDrcConfigExt->loudEqInstructionsPresent = 1;
            ret = parseLoudEqInstructions(hXmlReader, &uniDrcConfigExt->loudEqInstructions[loudEqInstructionsCount]);
            if (ret != 0) {
                fprintf(stderr, "parseLoudEqInstructions returns with an error\n");
                return -1;
            }
            
            loudEqInstructionsCount++;
            
        } else if ( strcmp("eqCoefficients", content) == 0 ) {
            
            ret = parseEqCoefficients(hXmlReader, &uniDrcConfigExt->eqCoefficients);
            if (ret != 0) {
                fprintf(stderr, "parseEqCoefficients returns with an error\n");
                return -1;
            }
            
        } else if ( strcmp("eqInstructions", content) == 0 ) {
            
            if (eqInstructionsCount>=EQ_INSTRUCTIONS_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <eqInstructions> allowed\n", EQ_INSTRUCTIONS_COUNT_MAX);
                return -1;
            }
            
            ret = parseEqInstructions(hXmlReader, &uniDrcConfigExt->eqInstructions[eqInstructionsCount]);
            if (ret != 0) {
                fprintf(stderr, "parseEqInstructions returns with an error\n");
                return -1;
            }
            
            eqInstructionsCount++;
        }
#ifdef LEVELING_SUPPORT
        else if (strcmp("loudnessLeveling", content) == 0) {
            ret = parseLoudnessLevelingExtension(
                hXmlReader,
#if MPEG_H_SYNTAX
                uniDrcConfig->drcInstructionsUniDrc,
#else
                uniDrcConfigExt->drcInstructionsUniDrcV1,
#endif
                &drcInstructionsUniDrcCount);
            if (ret != 0) {
                fprintf(stderr, "parseLoudnessLevelingExtension returns with an error\n");
                return ret;
            }

#if MPEG_H_SYNTAX
            uniDrcConfig->drcInstructionsUniDrcCount = drcInstructionsUniDrcCount;
#endif
            loudnessLevelingExtensionPresent = 1;
        }
#endif
    }
    
    if (parametricDrcInstructionsCount) {
        uniDrcConfigExt->parametricDrcPresent = 1;
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount]   = UNIDRCCONFEXT_PARAM_DRC;
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount+1] = UNIDRCCONFEXT_TERM;
        configExtensionCount++;
    }
    
    if (eqInstructionsCount) {
        uniDrcConfigExt->eqPresent = 1;
    }    
    
#if !MPEG_H_SYNTAX
    if (downmixInstructionsCount || drcInstructionsUniDrcCount || drcCoefficientsUniDrcCount || loudEqInstructionsCount || eqInstructionsCount) {
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount]   = UNIDRCCONFEXT_V1;
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount+1] = UNIDRCCONFEXT_TERM;
        configExtensionCount++;
    }
#endif
    
#ifdef LEVELING_SUPPORT
    if(loudnessLevelingExtensionPresent) {
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount] = UNIDRCCONFEXT_LEVELING;
        uniDrcConfigExt->uniDrcConfigExtType[configExtensionCount+1] = UNIDRCCONFEXT_TERM;
        configExtensionCount++;
    }
#endif

    uniDrcConfigExt->parametricDrcInstructionsCount = parametricDrcInstructionsCount;
    uniDrcConfigExt->downmixInstructionsV1Count = downmixInstructionsCount;
    uniDrcConfigExt->drcCoefficientsUniDrcV1Count = drcCoefficientsUniDrcCount;
    uniDrcConfigExt->drcInstructionsUniDrcV1Count = drcInstructionsUniDrcCount;
    uniDrcConfigExt->loudEqInstructionsCount = loudEqInstructionsCount;
    uniDrcConfigExt->eqInstructionsCount = eqInstructionsCount;
    
    return 0;
}

int parseParametricDrcGainSetParam(XMLREADER_INSTANCE_HANDLE hXmlReader, ParametricDrcGainSetParams *parametricDrcGainSetParams)
{
    unsigned int channelWeightCount = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "parametricDrcGainSetParams") != 0)
    {
        fprintf(stderr, "No valid XML information for parametricDrcGainSetParams\n %s", content);
        return -1;
    }
    
    if(! parametricDrcGainSetParams) {
        fprintf(stderr, "Pointer gainSetParams is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "parametricDrcId") == 0 ) {
            parametricDrcGainSetParams->parametricDrcId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("sideChainConfigType", content) == 0 ) {
            parametricDrcGainSetParams->sideChainConfigType = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixId", content) == 0 ) {
            parametricDrcGainSetParams->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("levelEstimChannelWeightFormat", content) == 0 ) {
            if (parametricDrcGainSetParams->sideChainConfigType == 1) { /* only overwrite default if sideChainConfigType == 1 */
                parametricDrcGainSetParams->levelEstimChannelWeightFormat = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("levelEstimChannelWeight", content) == 0 ) {
            if (parametricDrcGainSetParams->sideChainConfigType == 1) { /* only overwrite default if sideChainConfigType == 1 */
                parametricDrcGainSetParams->levelEstimChannelWeight[channelWeightCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                channelWeightCount++;
            }
        } else if ( strcmp("drcInputLoudnessPresent", content) == 0 ) {
            parametricDrcGainSetParams->drcInputLoudnessPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcInputLoudness", content) == 0 ) {
            if (parametricDrcGainSetParams->drcInputLoudnessPresent) { /* only overwrite default if present == 1 */
                parametricDrcGainSetParams->drcInputLoudness = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
            }
        }
    }
    return 0;
}

int parseParametricDrcInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, ParametricDrcInstructions *parametricDrcInstructions)
{
    unsigned int nodeCount = 0, nodeCount2 = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "parametricDrcInstructions") != 0)
    {
        fprintf(stderr, "No valid XML information for parametricDrcInstructions\n %s", content);
        return -1;
    }
    
    if(! parametricDrcInstructions) {
        fprintf(stderr, "Pointer parametricDrcInstructions is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "parametricDrcId") == 0 ) {
            parametricDrcInstructions->parametricDrcId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("parametricDrcLookAheadPresent", content) == 0 ) {
            parametricDrcInstructions->parametricDrcLookAheadPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("parametricDrcLookAhead", content) == 0 ) {
            if (parametricDrcInstructions->parametricDrcLookAheadPresent) { /* only overwrite default if present == 1 */
                parametricDrcInstructions->parametricDrcLookAhead = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("parametricDrcPresetIdPresent", content) == 0 ) {
            parametricDrcInstructions->parametricDrcPresetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("parametricDrcPresetId", content) == 0 ) {
            if (parametricDrcInstructions->parametricDrcPresetIdPresent) { /* only overwrite default if present == 1 */
                parametricDrcInstructions->parametricDrcPresetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("parametricDrcType", content) == 0 ) {
            if (parametricDrcInstructions->parametricDrcPresetIdPresent == 0) { /* only overwrite default if present == 0 */
                parametricDrcInstructions->parametricDrcType = atoi( xmlReaderLib_GetContent(hXmlReader) );
            }
        } else if ( strcmp("parametricDrcTypeFeedForward", content) == 0 ) {
            if (parametricDrcInstructions->parametricDrcType == 0) {
                xmlReaderLib_GoToChilds(hXmlReader);
                while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                    if( strcmp("levelEstimKWeightingType", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimKWeightingType = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("levelEstimIntegrationTimePresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimIntegrationTimePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("levelEstimIntegrationTime", content) == 0 ) {
                        if (parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimIntegrationTimePresent) { /* only overwrite default if present == 1 */
                            parametricDrcInstructions->parametricDrcTypeFeedForward.levelEstimIntegrationTime = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    } else if ( strcmp("drcCurveDefinitionType", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.drcCurveDefinitionType = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("drcCharacteristic", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.drcCharacteristic = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("nodeLevel", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.nodeLevel[nodeCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        nodeCount++;
                    } else if ( strcmp("nodeGain", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.nodeGain[nodeCount2] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        nodeCount2++;
                    } else if ( strcmp("drcGainSmoothParametersPresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.drcGainSmoothParametersPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothAttackTimeSlow", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothAttackTimeSlow = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothReleaseTimeSlow", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothReleaseTimeSlow = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothTimeFastPresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothTimeFastPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothAttackTimeFast", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothAttackTimeFast = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothReleaseTimeFast", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothReleaseTimeFast = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothThresholdPresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothThresholdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothAttackThreshold", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothAttackThreshold = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothReleaseThreshold", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothReleaseThreshold = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothHoldOffCountPresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothHoldOffCountPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("gainSmoothHoldOff", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeFeedForward.gainSmoothHoldOff = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    }      
                }
                if (nodeCount == nodeCount2) {
                    parametricDrcInstructions->parametricDrcTypeFeedForward.nodeCount = nodeCount;
                } else {
                    return -1;
                }
            }            
        } else if ( strcmp("parametricDrcTypeLim", content) == 0 ) {
            if (parametricDrcInstructions->parametricDrcType == 1) {
                xmlReaderLib_GoToChilds(hXmlReader);
                while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                    if( strcmp("parametricLimThresholdPresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeLim.parametricLimThresholdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("parametricLimThreshold", content) == 0 ) {
                        if (parametricDrcInstructions->parametricDrcTypeLim.parametricLimThresholdPresent) { /* only overwrite default if present == 1 */
                            parametricDrcInstructions->parametricDrcTypeLim.parametricLimThreshold = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    } else if ( strcmp("parametricLimReleasePresent", content) == 0 ) {
                        parametricDrcInstructions->parametricDrcTypeLim.parametricLimReleasePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    } else if ( strcmp("parametricLimRelease", content) == 0 ) {
                        if (parametricDrcInstructions->parametricDrcTypeLim.parametricLimReleasePresent) { /* only overwrite default if present == 1 */
                            parametricDrcInstructions->parametricDrcTypeLim.parametricLimRelease = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int parseLoudEqInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudEqInstructions *loudEqInstructions)
{
    unsigned int additionalDownmixIdCount = 0, additionalDrcSetIdCount = 0, additionalEqSetIdCount = 0, num_loudEqSequences = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "loudEqInstructions") != 0)
    {
        fprintf(stderr, "No valid XML information for loudEqInstructions\n %s", content);
        return -1;
    }
    
    if(! loudEqInstructions) {
        fprintf(stderr, "Pointer loudEqInstructions is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "loudEqSetId") == 0 ) {
            loudEqInstructions->loudEqSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcLocation", content) == 0 ) {
            loudEqInstructions->drcLocation = atoi( xmlReaderLib_GetContent(hXmlReader) );
            
        } else if ( strcmp("downmixIdPresent", content) == 0 ) {
            loudEqInstructions->downmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixId", content) == 0 ) {
            loudEqInstructions->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixIdPresent", content) == 0 ) {
            loudEqInstructions->additionalDownmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixId", content) == 0 ) {
            if (loudEqInstructions->additionalDownmixIdPresent) { /* only overwrite if present == 1 */
                if (additionalDownmixIdCount>=ADDITIONAL_DOWNMIX_ID_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalDownmixId> allowed\n", ADDITIONAL_DOWNMIX_ID_MAX);
                    return -1;
                }
                loudEqInstructions->additionalDownmixId[additionalDownmixIdCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalDownmixIdCount++;
            }
        } else if ( strcmp("drcSetIdPresent", content) == 0 ) {
            loudEqInstructions->drcSetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("drcSetId", content) == 0 ) {
            loudEqInstructions->drcSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDrcSetIdPresent", content) == 0 ) {
            loudEqInstructions->additionalDrcSetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDrcSetId", content) == 0 ) {
            if (loudEqInstructions->additionalDrcSetIdPresent) { /* only overwrite if present == 1 */
                if (additionalDrcSetIdCount>=ADDITIONAL_DRC_SET_ID_COUNT_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalDrcSetId> allowed\n", ADDITIONAL_DRC_SET_ID_COUNT_MAX);
                    return -1;
                }
                loudEqInstructions->additionalDrcSetId[additionalDrcSetIdCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalDrcSetIdCount++;
            }
        } else if ( strcmp("eqSetIdPresent", content) == 0 ) {
            loudEqInstructions->eqSetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSetId", content) == 0 ) {
            loudEqInstructions->eqSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalEqSetIdPresent", content) == 0 ) {
            loudEqInstructions->additionalEqSetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalEqSetId", content) == 0 ) {
            if (loudEqInstructions->additionalEqSetIdPresent) { /* only overwrite if present == 1 */
                if (additionalEqSetIdCount>=ADDITIONAL_EQ_SET_ID_COUNT_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalEqSetId> allowed\n", ADDITIONAL_EQ_SET_ID_COUNT_MAX);
                    return -1;
                }
                loudEqInstructions->additionalEqSetId[additionalEqSetIdCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalEqSetIdCount++;
            }
        } else if ( strcmp("loudnessAfterDrc", content) == 0 ) {
            loudEqInstructions->loudnessAfterDrc = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("loudnessAfterEq", content) == 0 ) {
            loudEqInstructions->loudnessAfterEq = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("loudEqSequenceParams", content) == 0 ) {
            if (num_loudEqSequences>=LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <loudEqSequenceParams> allowed\n", LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX);
                return -1;
            }
            
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("gainSequenceIndex", content) == 0 ) {
                    loudEqInstructions->gainSequenceIndex[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("drcCharacteristicFormatIsCICP", content) == 0 ) {
                    loudEqInstructions->drcCharacteristicFormatIsCICP[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );                    
                } else if ( strcmp("drcCharacteristic", content) == 0 ) {
                    loudEqInstructions->drcCharacteristic[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("drcCharacteristicLeftIndex", content) == 0 ) {
                    loudEqInstructions->drcCharacteristicLeftIndex[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("drcCharacteristicRightIndex", content) == 0 ) {
                    loudEqInstructions->drcCharacteristicRightIndex[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("frequencyRangeIndex", content) == 0 ) {
                    loudEqInstructions->frequencyRangeIndex[num_loudEqSequences] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudEqScaling", content) == 0 ) {
                    loudEqInstructions->loudEqScaling[num_loudEqSequences] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudEqOffset", content) == 0 ) {
                    loudEqInstructions->loudEqOffset[num_loudEqSequences] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
            num_loudEqSequences++;
        }
    }
    
    loudEqInstructions->additionalDownmixIdCount = additionalDownmixIdCount;
    loudEqInstructions->additionalDrcSetIdCount = additionalDrcSetIdCount;
    loudEqInstructions->additionalEqSetIdCount = additionalEqSetIdCount;
    loudEqInstructions->loudEqGainSequenceCount = num_loudEqSequences;
    
    return 0;
}

int parseEqCoefficients(XMLREADER_INSTANCE_HANDLE hXmlReader, EqCoefficients *eqCoefficients)
{
    unsigned int filterBlockCount = 0, tdFilterElementCount = 0, filterBlockElementCount = 0, firCoefficientCount = 0, realZeroRadiusOneCount = 0, realZeroCount = 0, genericZeroCount = 0, complexPoleCount = 0, realPoleCount = 0, uniqueEqSubbandGainsCount = 0, eqSubbandGainCount = 0, eqSplineNodeCount = 0, bandCount = 0;
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "eqCoefficients") != 0)
    {
        fprintf(stderr, "No valid XML information for eqCoefficients\n %s", content);
        return -1;
    }
    
    if(! eqCoefficients) {
        fprintf(stderr, "Pointer eqCoefficients is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "filterBlock") == 0 ) {
            if (filterBlockCount>=FILTER_BLOCK_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <filterBlock> allowed\n", FILTER_BLOCK_COUNT_MAX);
                return -1;
            }
            
            filterBlockElementCount = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("filterElement", content) == 0 ) {
                    if (filterBlockElementCount>=FILTER_ELEMENT_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <filterElement> allowed\n", FILTER_ELEMENT_COUNT_MAX);
                        return -1;
                    }
                    
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("filterElementIndex", content) == 0 ) {
                            eqCoefficients->filterBlock[filterBlockCount].filterElement[filterBlockElementCount].filterElementIndex = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("filterElementGainPresent", content) == 0 ) {
                            eqCoefficients->filterBlock[filterBlockCount].filterElement[filterBlockElementCount].filterElementGainPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("filterElementGain", content) == 0 ) {
                            eqCoefficients->filterBlock[filterBlockCount].filterElement[filterBlockElementCount].filterElementGain = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    }
                    filterBlockElementCount++;
                }
            }
            eqCoefficients->filterBlock[filterBlockCount].filterElementCount = filterBlockElementCount;
            filterBlockCount++;
        } else if ( strcmp("tdFilterElement", content) == 0 ) {
            if (tdFilterElementCount>=FILTER_ELEMENT_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <tdFilterElement> allowed\n", FILTER_ELEMENT_COUNT_MAX);
                return -1;
            }
            
            realZeroRadiusOneCount = 0;
            realZeroCount = 0;
            genericZeroCount = 0;
            realPoleCount = 0;
            complexPoleCount = 0;
            firCoefficientCount = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("eqFilterFormat", content) == 0 ) {
                    eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].eqFilterFormat = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("realZeroRadiusOne", content) == 0 ) {
                    if (realZeroRadiusOneCount>=REAL_ZERO_RADIUS_ONE_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <realZeroRadiusOne> allowed\n", REAL_ZERO_RADIUS_ONE_COUNT_MAX);
                        return -1;
                    }
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("zeroSign", content) == 0 ) {
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].zeroSign[realZeroRadiusOneCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            realZeroRadiusOneCount++;
                        }
                    }
                } else if ( strcmp("realZero", content) == 0 ) {
                    if (realZeroCount>=REAL_ZERO_RADIUS_ONE_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <realZero> allowed\n", REAL_ZERO_RADIUS_ONE_COUNT_MAX);
                        return -1;
                    }
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("radius", content) == 0 ) {
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].realZeroRadius[realZeroCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            realZeroCount++;
                        }
                    }
                } else if ( strcmp("genericZero", content) == 0 ) {
                    bool radiusIsSet = FALSE;
                    bool angleIsSet = FALSE;
                    if (genericZeroCount>=COMPLEX_ZERO_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <genericZero> allowed\n", COMPLEX_ZERO_COUNT_MAX);
                        return -1;
                    }
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("radius", content) == 0 ) {
                            radiusIsSet = TRUE;
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].genericZeroRadius[genericZeroCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("angle", content) == 0 ) {
                            angleIsSet = TRUE;
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].genericZeroAngle[genericZeroCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                        if (radiusIsSet && angleIsSet) {
                            radiusIsSet = angleIsSet = FALSE;
                            genericZeroCount++;
                        }
                    }
                } else if ( strcmp("realPole", content) == 0 ) {
                    if (realPoleCount>=REAL_POLE_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <realPole> allowed\n", REAL_POLE_COUNT_MAX);
                        return -1;
                    }
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("radius", content) == 0 ) {
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].realPoleRadius[realPoleCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            realPoleCount++;
                        }
                    }
                } else if ( strcmp("complexPole", content) == 0 ) {
                    bool radiusIsSet = FALSE;
                    bool angleIsSet = FALSE;
                    if (complexPoleCount>=COMPLEX_POLE_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <complexPole> allowed\n", COMPLEX_POLE_COUNT_MAX);
                        return -1;
                    }
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("radius", content) == 0 ) {
                            radiusIsSet = TRUE;
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].complexPoleRadius[complexPoleCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("angle", content) == 0 ) {
                            angleIsSet = TRUE;
                            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].complexPoleAngle[complexPoleCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        }
                        if (radiusIsSet && angleIsSet) {
                            radiusIsSet = angleIsSet = FALSE;
                            complexPoleCount++;
                        }
                    }
                } else if ( strcmp("firFilterOrder", content) == 0 ) {
                    eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].firFilterOrder = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("firSymmetry", content) == 0 ) {
                    eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].firSymmetry = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("firCoefficient", content) == 0 ) {
                    if (firCoefficientCount>=FIR_ORDER_MAX/2) {
                        fprintf(stderr, "There are not more than %i <firCoefficient> allowed\n", FIR_ORDER_MAX/2);
                        return -1;
                    }
                    eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].firCoefficient[firCoefficientCount] = atof( xmlReaderLib_GetContent(hXmlReader) );
                    firCoefficientCount++;
                }
            }
            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].realZeroRadiusOneCount = realZeroRadiusOneCount;
            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].realZeroCount = realZeroCount;
            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].genericZeroCount = genericZeroCount;
            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].realPoleCount = realPoleCount;
            eqCoefficients->uniqueTdFilterElement[tdFilterElementCount].complexPoleCount = complexPoleCount;
            tdFilterElementCount++;
        } else if ( strcmp("eqSubbandGainRepresentation", content) == 0 ) {
            eqCoefficients->eqSubbandGainRepresentation = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqDelayMaxPresent", content) == 0 ) {
            eqCoefficients->eqDelayMaxPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqDelayMax", content) == 0 ) {
            eqCoefficients->eqDelayMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSubbandGainFormat", content) == 0 ) {
            eqCoefficients->eqSubbandGainFormat = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSubbandGainCount", content) == 0 ) {
            eqCoefficients->eqSubbandGainCount = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSubbandGains", content) == 0 ) {
            bool splineIsSet = FALSE;
            bool gainVectorIsSet = FALSE;
            if (uniqueEqSubbandGainsCount>=UNIQUE_SUBBAND_GAIN_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <eqSubbandGains> allowed\n", UNIQUE_SUBBAND_GAIN_COUNT_MAX);
                return -1;
            }
            
            eqSplineNodeCount = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("eqSubbandSplineNode", content) == 0 ) {
                    bool gainInitialIsSet = FALSE;
                    bool slopeIsSet = FALSE;
                    bool gainDeltaIsSet = FALSE;
                    bool eqFreqDeltaIsSet = FALSE;
                    if (eqSplineNodeCount>=UNIQUE_SUBBAND_GAIN_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <eqSubbandSplineNode> allowed\n", UNIQUE_SUBBAND_GAIN_COUNT_MAX);
                        return -1;
                    }

                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("eqGainInitial", content) == 0 ) {
                            splineIsSet = TRUE;
                            if (eqSplineNodeCount == 0) {
                                gainInitialIsSet = TRUE;
                                eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].eqGainInitial = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            }
                        } else if ( strcmp("eqSlope", content) == 0 ) {
                            eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].eqSlope[eqSplineNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            slopeIsSet = TRUE;
                        } else if ( strcmp("eqGainDelta", content) == 0 ) {
                            eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].eqGainDelta[eqSplineNodeCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                            gainDeltaIsSet = TRUE;
                        } else if ( strcmp("eqFreqDelta", content) == 0 ) {
                            eqFreqDeltaIsSet = TRUE;
                            eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].eqFreqDelta[eqSplineNodeCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        }
                        if (((eqSplineNodeCount>0) && slopeIsSet && gainDeltaIsSet && eqFreqDeltaIsSet) || ((eqSplineNodeCount==0) && gainInitialIsSet && slopeIsSet)){
                            slopeIsSet = gainDeltaIsSet = eqFreqDeltaIsSet = gainInitialIsSet = FALSE;
                            eqSplineNodeCount++;
                        }
                    }
                    eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].nEqNodes = eqSplineNodeCount;
                } else if ( strcmp("eqSubbandGain", content) == 0 ) {
                    gainVectorIsSet = TRUE;
                    if (eqSubbandGainCount>=EQ_SUBBAND_GAIN_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <eqSubbandGainCount> allowed\n", EQ_SUBBAND_GAIN_COUNT_MAX);
                        return -1;
                    }
                    eqCoefficients->eqSubbandGainVector[uniqueEqSubbandGainsCount].eqSubbandGain[bandCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                    bandCount++;
                }
            }
            
            if (eqCoefficients->eqSubbandGainRepresentation == 0) {
                if (bandCount != eqCoefficients->eqSubbandGainCount) {
                    fprintf(stderr, "<eqSubbandGainCount> value of %i does not match the number of provided gain values of %i\n", eqCoefficients->eqSubbandGainCount, bandCount);
                    return -1;
                }
            }
            
            eqCoefficients->eqSubbandGainSpline[uniqueEqSubbandGainsCount].nEqNodes = eqSplineNodeCount;
            if (splineIsSet || gainVectorIsSet) {
                splineIsSet = gainVectorIsSet = FALSE;
                uniqueEqSubbandGainsCount++;
            }
        }
    }
    
    eqCoefficients->uniqueFilterBlockCount = filterBlockCount;
    eqCoefficients->uniqueTdFilterElementCount = tdFilterElementCount;
    eqCoefficients->uniqueEqSubbandGainsCount = uniqueEqSubbandGainsCount;
    
    return 0;
}

#ifdef LEVELING_SUPPORT
int parseLoudnessLevelingExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, DrcInstructionsUniDrc* drcInstructionsUniDrc, unsigned int* drcInstructionsUniDrcCount)
{
    char const* content = xmlReaderLib_GetNodeName(hXmlReader);
    if (strcmp(content, "loudnessLeveling") != 0)
    {
        fprintf(stderr, "No valid XML information for eqInstructions\n %s", content);
        return -1;
    }

    xmlReaderLib_GoToChilds(hXmlReader);

    int const drcInstructionsUniDrcCountInit = *drcInstructionsUniDrcCount;
    for (int i = 0; i < drcInstructionsUniDrcCountInit; ++i)
    {
        DrcInstructionsUniDrc* inst = &drcInstructionsUniDrc[i];
        if (!(inst->drcSetEffect & (1 << 11)))
        {
            continue;
        }

        while ((content = xmlReaderLib_GetNextNode(hXmlReader)) != NULL)
        {
            if (strcmp(content, "levelingPresent") == 0)
            {
                inst->levelingPresent = atoi(xmlReaderLib_GetContent(hXmlReader));
            }

            if (strcmp(content, "drcInstructionsUniDrc") == 0)
            {
                DrcInstructionsUniDrc* duckingOnlySet = &drcInstructionsUniDrc[(*drcInstructionsUniDrcCount)++];
                duckingOnlySet->duckingOnlyDrcSet = 1;

                int ret = parseDrcInstructionsUniDrc(hXmlReader, duckingOnlySet, 1);
                if (ret != 0)
                {
                    fprintf(stderr, "parseDrcInstructionsUniDrc returns with an error\n");
                    return -1;
                }
            }
        }
    }

    return 0;
}
#endif

    int parseEqInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader,
                            EqInstructions * eqInstructions)
{
    unsigned int additionalDownmixIdCount = 0, additionalDrcSetIdCount = 0, eqChannelCount = 0, eqChannelGroupCount = 0, channelGroupCount = 0, filterBlockCount = 0;
    int k, tmp, match, uniqueIndex[CHANNEL_COUNT_MAX];
    const char * content;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "eqInstructions") != 0)
    {
        fprintf(stderr, "No valid XML information for eqInstructions\n %s", content);
        return -1;
    }
    
    if(! eqInstructions) {
        fprintf(stderr, "Pointer eqInstructions is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if( strcmp(content, "eqSetId") == 0 ) {
            eqInstructions->eqSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqSetComplexityLevel", content) == 0 ) {
            eqInstructions->eqSetComplexityLevel = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixIdPresent", content) == 0 ) {
            eqInstructions->downmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("downmixId", content) == 0 ) {
            eqInstructions->downmixId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqApplyToDownmix", content) == 0 ) {
            eqInstructions->eqApplyToDownmix = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixIdPresent", content) == 0 ) {
            eqInstructions->additionalDownmixIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDownmixId", content) == 0 ) {
            if (eqInstructions->additionalDownmixIdPresent) { /* only overwrite if present == 1 */
                if (additionalDownmixIdCount>=ADDITIONAL_DOWNMIX_ID_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalDownmixId> allowed\n", ADDITIONAL_DOWNMIX_ID_MAX);
                    return -1;
                }
                eqInstructions->additionalDownmixId[additionalDownmixIdCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalDownmixIdCount++;
            }
        } else if ( strcmp("drcSetId", content) == 0 ) {
            eqInstructions->drcSetId = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDrcSetIdPresent", content) == 0 ) {
            eqInstructions->additionalDrcSetIdPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("additionalDrcSetId", content) == 0 ) {
            if (eqInstructions->additionalDrcSetIdPresent) { /* only overwrite if present == 1 */
                if (additionalDrcSetIdCount>=ADDITIONAL_DRC_SET_ID_COUNT_MAX) {
                    fprintf(stderr, "There are not more than %i <additionalDrcSetId> allowed\n", ADDITIONAL_DRC_SET_ID_COUNT_MAX);
                    return -1;
                }
                eqInstructions->additionalDrcSetId[additionalDrcSetIdCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                additionalDrcSetIdCount++;
            }
        } else if ( strcmp("eqSetPurpose", content) == 0 ) {
            eqInstructions->eqSetPurpose = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("dependsOnEqSetPresent", content) == 0 ) {
            eqInstructions->dependsOnEqSetPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("dependsOnEqSet", content) == 0 ) {
            eqInstructions->dependsOnEqSet = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("noIndependentEqUse", content) == 0 ) {
            eqInstructions->noIndependentEqUse = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqChannelGroupForChannel", content) == 0 ) {
            if (eqChannelCount>=CHANNEL_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <eqChannelGroupForChannel> allowed\n", CHANNEL_COUNT_MAX);
                return -1;
            }
            if (eqChannelCount>=CHANNEL_GROUP_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i channel groups allowed\n", CHANNEL_GROUP_COUNT_MAX);
                return -1;
            }
            eqInstructions->eqChannelGroupForChannel[eqChannelCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
            
            /* get number of drc channel groups */
            tmp = eqInstructions->eqChannelGroupForChannel[eqChannelCount];
            if (tmp >= 0) {
                match = 0;
                for (k=0; k < eqChannelGroupCount; k++) {
                    if (uniqueIndex[k] == tmp) match = 1;
                }
                if (match == 0)
                {
                    uniqueIndex[eqChannelGroupCount] = tmp;
                    eqChannelGroupCount++;
                }
            }
            eqChannelCount++;
        } else if ( strcmp("tdFilterCascadePresent", content) == 0 ) {
            eqInstructions->tdFilterCascadePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqPhaseAlignmentPresent", content) == 0 ) {
            eqInstructions->tdFilterCascade.eqPhaseAlignmentPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("subbandGainsPresent", content) == 0 ) {
            eqInstructions->subbandGainsPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqChannelGroupParams", content) == 0 ) {
            if (channelGroupCount>=EQ_CHANNEL_GROUP_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <tdFilterCascade> allowed\n", EQ_CHANNEL_GROUP_COUNT_MAX);
                return -1;
            }
            tmp = channelGroupCount+1;
            filterBlockCount = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("eqCascadeGainPresent", content) == 0 ) {
                    eqInstructions->tdFilterCascade.eqCascadeGainPresent[channelGroupCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("eqCascadeGain", content) == 0 ) {
                    eqInstructions->tdFilterCascade.eqCascadeGain[channelGroupCount] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("filterBlockIndex", content) == 0 ) {
                    if (filterBlockCount>=EQ_FILTER_BLOCK_COUNT_MAX) {
                        fprintf(stderr, "There are not more than %i <filterBlockIndex> allowed\n", EQ_FILTER_BLOCK_COUNT_MAX);
                        return -1;
                    }
                    eqInstructions->tdFilterCascade.filterBlockRefs[channelGroupCount].filterBlockIndex[filterBlockCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    filterBlockCount++;
                } else if ( strcmp("eqPhaseAlignment", content) == 0 ) {
                    eqInstructions->tdFilterCascade.eqPhaseAlignment[channelGroupCount][tmp] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    tmp++;
                } else if ( strcmp("subbandGainsIndex", content) == 0 ) {
                    eqInstructions->subbandGainsIndex[channelGroupCount] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
            eqInstructions->tdFilterCascade.filterBlockRefs[channelGroupCount].filterBlockCount = filterBlockCount;
            channelGroupCount++;
        } else if ( strcmp("eqTransitionDurationPresent", content) == 0 ) {
            eqInstructions->eqTransitionDurationPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
        } else if ( strcmp("eqTransitionDuration", content) == 0 ) {
            eqInstructions->eqTransitionDuration = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
        }
    }
    
    eqInstructions->eqChannelCount = eqChannelCount;
    eqInstructions->eqChannelGroupCount = eqChannelGroupCount;
    
    return 0;
}

int parseLoudnessInfoSetExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudnessInfoSetExtension *loudnessInfoSetExtension)
{
    unsigned int loudnessInfoCount = 0, loundessInfoAlbumCount = 0;
    const char * content;
    int ret = 0;
    
    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "loudnessInfoSetExtension") != 0)
    {
        fprintf(stderr, "No valid XML information for uniDrcConfigExtension\n %s", content);
        return -1;
    }
    
    if(! loudnessInfoSetExtension) {
        fprintf(stderr, "Pointer loudnessInfoSetExtension is not allowed to be NULL\n");
        return -1;
    }
    
    /* Parse Groupdefinitions */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        if(strcmp(content, "loudnessInfo") == 0) {
            
            if (loudnessInfoCount>=LOUDNESS_INFO_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <loudnessInfo> allowed\n", LOUDNESS_INFO_COUNT_MAX);
                return -1;
            }
            
            ret = parseLoudnessInfo(hXmlReader, &loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1[loudnessInfoCount], 1);
            if (ret != 0) {
                fprintf(stderr, "parseLoudnessInfo returns with an error\n");
                return -1;
            }
            
            loudnessInfoCount++;
            
        } else if(strcmp(content, "loudnessInfoAlbum") == 0) {
            
            if (loudnessInfoCount>=LOUDNESS_INFO_COUNT_MAX) {
                fprintf(stderr, "There are not more than %i <loudnessInfoAlbum> allowed\n", LOUDNESS_INFO_COUNT_MAX);
                return -1;
            }
            
            ret = parseLoudnessInfo(hXmlReader, &loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Album[loundessInfoAlbumCount], 1);
            if (ret != 0) {
                fprintf(stderr, "parseLoudnessInfo returns with an error\n");
                return -1;
            }
            
            loundessInfoAlbumCount++;
        }
    }
    loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count = loudnessInfoCount;
    loudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount = loundessInfoAlbumCount;
    return 0;
}
