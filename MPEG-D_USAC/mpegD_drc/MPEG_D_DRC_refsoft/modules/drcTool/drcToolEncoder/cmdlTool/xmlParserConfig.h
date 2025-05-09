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

#ifndef _XML_PARSER_CONFIG_H_
#define _XML_PARSER_CONFIG_H_

#include "userConfig.h"
#include "xmlReaderLib.h"
#include "drcToolEncoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

int parseConfigFile(char const * const filename, EncParams* encParams, UniDrcConfig* encUniDrcConfig, LoudnessInfoSet* encLoudnessInfoSet, UniDrcGainExt* encGainExtension);
int parseConfig(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, UniDrcConfig* uniDrcConfig, LoudnessInfoSet* encLoudnessInfoSet);
int parseDrcCoefficientsUniDrc(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, DrcCoefficientsUniDrc *drcCoefficientsUniDrc, int version);
int parseGainSetParam(XMLREADER_INSTANCE_HANDLE hXmlReader, GainSetParams *gainSetParams, int version);
int parseDrcInstructionsUniDrc(XMLREADER_INSTANCE_HANDLE hXmlReader, DrcInstructionsUniDrc *drcInstructionUniDrc, int version);
int parseDownmixInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, DownmixInstructions *downmixInstruction, int version);
int parseLoudnessInfo(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudnessInfo *loudnessInfo, int version);
int parseUniDrcConfigExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, EncParams* encParams, UniDrcConfig *uniDrcConfig);
int parseParametricDrcGainSetParam(XMLREADER_INSTANCE_HANDLE hXmlReader, ParametricDrcGainSetParams *parametricDrcGainSetParams);
int parseParametricDrcInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, ParametricDrcInstructions *parametricDrcInstructions);
int parseLoudEqInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudEqInstructions *loudEqInstructions);
int parseEqCoefficients(XMLREADER_INSTANCE_HANDLE hXmlReader, EqCoefficients *eqCoefficients);
int parseEqInstructions(XMLREADER_INSTANCE_HANDLE hXmlReader, EqInstructions *eqInstructions);
int parseLoudnessInfoSetExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, LoudnessInfoSetExtension *loudnessInfoSetExtension);
#ifdef LEVELING_SUPPORT
int parseLoudnessLevelingExtension(XMLREADER_INSTANCE_HANDLE hXmlReader, DrcInstructionsUniDrc* drcInstructionsUniDrc, unsigned int* drcInstructionsUniDrcCount);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _XML_PARSER_CONFIG_H_ */
