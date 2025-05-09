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

#include "uniDrcCommon.h"
#include "uniDrcInterface.h"
#include "xmlParserInterface.h"

int parseInterfaceFile(char const * const filename, UniDrcInterface* uniDrcInterface)
{
    int ret = 0;
    XMLREADER_RETURN error = XMLREADER_NO_ERROR;
    XMLREADER_INSTANCE_HANDLE hXmlReader;
    
    error = xmlReaderLib_New(&hXmlReader, filename);
    if (error != XMLREADER_NO_ERROR) {
        return 1;
    }
    
    ret = parseInterface(hXmlReader, uniDrcInterface);
    
    error = xmlReaderLib_Delete(hXmlReader);
    if (error != XMLREADER_NO_ERROR) {
        return 1;
    }
    
    return ret;
}

int parseInterface(XMLREADER_INSTANCE_HANDLE hXmlReader, UniDrcInterface* uniDrcInterface)
{
    const char * content;
    int numDrcEffectTypeRequests = 0;

    /* Check hXmlReader */
    if( hXmlReader == NULL )
    {
        fprintf(stderr, "Error: pointer to XML-reader not valid!\n");
        return -1;
    }
    
    if(uniDrcInterface) {
        memset(uniDrcInterface, 0, sizeof(UniDrcInterface) ); /*TODO*/
    } else {
        fprintf(stderr, "Pointer uniDrcInterface is not allowed to be NULL\n");
        return -1;
    }
    
    /*first node has to be the masternode*/
    content=xmlReaderLib_GetNodeName(hXmlReader);
    
    if(strcmp(content, "uniDrcInterface") != 0)
    {
        fprintf(stderr, "No valid XML interface information\n %s", content);
        return -1;
    }
    
    /* Parse configuration */
    xmlReaderLib_GoToChilds(hXmlReader);
    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
        
        if( strcmp(content, "uniDrcInterfaceSignature") == 0 )
        {
            int uniDrcInterfaceSignatureDataLength = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("uniDrcInterfaceSignaturePresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceSignaturePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("uniDrcInterfaceSignatureType", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureType = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("uniDrcInterfaceSignatureData", content) == 0 ) {
                    if (uniDrcInterfaceSignatureDataLength>=MAX_SIGNATURE_DATA_LENGTH_PLUS_ONE) {
                        fprintf(stderr, "There are not more then %i <uniDrcInterfaceSignatureData> allowed\n", MAX_SIGNATURE_DATA_LENGTH_PLUS_ONE);
                        return -1;
                    }
                    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[uniDrcInterfaceSignatureDataLength] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    uniDrcInterfaceSignatureDataLength++;
                }
            }
            uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength = uniDrcInterfaceSignatureDataLength;
        } else if ( strcmp(content, "systemInterface") == 0 ) {
            int numDownmixIdRequests = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("systemInterfacePresent", content) == 0 ) {
                    uniDrcInterface->systemInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("targetConfigRequestType", content) == 0 ) {
                    uniDrcInterface->systemInterface.targetConfigRequestType = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("targetLayoutRequested", content) == 0 ) {
                    uniDrcInterface->systemInterface.targetLayoutRequested = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("targetChannelCountRequested", content) == 0 ) {
                    uniDrcInterface->systemInterface.targetChannelCountRequested = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if( strcmp("downmixIdRequested", content) == 0 ) {
                    if (numDownmixIdRequests>=MAX_NUM_DOWNMIX_ID_REQUESTS) {
                        fprintf(stderr, "There are not more then %i <downmixIdRequested> allowed\n", MAX_NUM_DOWNMIX_ID_REQUESTS);
                        return -1;
                    }
                    uniDrcInterface->systemInterface.downmixIdRequested[numDownmixIdRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                    numDownmixIdRequests++;
                }
            }
            uniDrcInterface->systemInterface.numDownmixIdRequests = numDownmixIdRequests;
        } else if ( strcmp(content, "loudnessNormalizationControlInterface") == 0 ) {
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("loudnessNormalizationControlInterfacePresent", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationControlInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessNormalizationOn", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("targetLoudness", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationControlInterface.targetLoudness = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
        } else if ( strcmp(content, "loudnessNormalizationParameterInterface") == 0 ) {
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("loudnessNormalizationParameterInterfacePresent", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("albumMode", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.albumMode = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("peakLimiterPresent", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessDeviationMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessDeviationMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessMeasurementMethod", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessMeasurementMethod", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessMeasurementSystem", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessMeasurementSystem", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessMeasurementPreProc", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessMeasurementPreProc", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeDeviceCutOffFrequency", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("deviceCutOffFrequency", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessNormalizationGainDbMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessNormalizationGainDbMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeLoudnessNormalizationGainModificationDb", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessNormalizationGainModificationDb", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeOutputPeakLevelMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("outputPeakLevelMax", content) == 0 ) {
                    uniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
        } else if ( strcmp(content, "dynamicRangeControlInterface") == 0 ) {
            int numDrcFeatureRequests = 0;
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("dynamicRangeControlInterfacePresent", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("dynamicRangeControlOn", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("drcFeatureRequest", content) == 0 ) {
                    if (numDrcFeatureRequests>=MAX_NUM_DRC_FEATURE_REQUESTS) {
                        fprintf(stderr, "There are not more then %i <drcFeatureRequest> allowed\n", MAX_NUM_DRC_FEATURE_REQUESTS);
                        return -1;
                    }
                    numDrcEffectTypeRequests = 0;
                    xmlReaderLib_GoToChilds(hXmlReader);
                    while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                        if ( strcmp("drcFeatureRequestType", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[numDrcFeatureRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("numDrcEffectTypeRequestsDesired", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[numDrcFeatureRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("drcEffectTypeRequest", content) == 0 ) {
                            if (numDrcEffectTypeRequests>=MAX_NUM_DRC_EFFECT_TYPE_REQUESTS) {
                                fprintf(stderr, "There are not more then %i <drcEffectTypeRequest> allowed\n", MAX_NUM_DRC_EFFECT_TYPE_REQUESTS);
                                return -1;
                            }
                            uniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[numDrcFeatureRequests][numDrcEffectTypeRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                            numDrcEffectTypeRequests++;
                        } else if ( strcmp("dynRangeMeasurementRequestType", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[numDrcFeatureRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("dynRangeRequestedIsRange", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[numDrcFeatureRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("dynamicRangeRequestValue", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[numDrcFeatureRequests] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("dynamicRangeRequestValueMin", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[numDrcFeatureRequests] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("dynamicRangeRequestValueMax", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[numDrcFeatureRequests] = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                        } else if ( strcmp("drcCharacteristicRequest", content) == 0 ) {
                            uniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[numDrcFeatureRequests] = atoi( xmlReaderLib_GetContent(hXmlReader) );
                        }
                    }
                    uniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[numDrcFeatureRequests] = numDrcEffectTypeRequests;
                    numDrcFeatureRequests++;
                }
            }
            uniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests = numDrcFeatureRequests;
        } else if ( strcmp(content, "dynamicRangeControlParameterInterface") == 0 ) {
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("dynamicRangeControlParameterInterfacePresent", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeCompress", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.changeCompress = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("compress", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.compress = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeBoost", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.changeBoost = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("boost", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.boost = (float) atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("changeDrcCharacteristicTarget", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("drcCharacteristicTarget", content) == 0 ) {
                    uniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
            }
        } else if ( strcmp(content, "uniDrcInterfaceExtension") == 0 ) {
            xmlReaderLib_GoToChilds(hXmlReader);
            while((content=xmlReaderLib_GetNextNode(hXmlReader))!=NULL) {
                if ( strcmp("uniDrcInterfaceExtensionPresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtensionPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
#ifdef LEVELING_SUPPORT
                else if( strcmp("levelingControlInterfacePresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.levelingControlInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
                else if( strcmp("loudnessLevelingOn", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.levelingControlInterface.loudnessLevelingOn = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
#endif

#ifdef AMD1
                else if ( strcmp("loudnessEqParameterInterfacePresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessEqRequestPresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequestPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("loudnessEqRequest", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequest = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("sensitivityPresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivityPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("sensitivity", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivity = atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("playbackGainPresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGainPresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("playbackGain", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGain = atof( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("equalizationControlInterfacePresent", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent = atoi( xmlReaderLib_GetContent(hXmlReader) );
                } else if ( strcmp("eqSetPurposeRequest", content) == 0 ) {
                    uniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterface.eqSetPurposeRequest = atoi( xmlReaderLib_GetContent(hXmlReader) );
                }
#endif
            }
        }
    }

    return 0;
}
