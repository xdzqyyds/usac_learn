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

#include "uniDrcInterfaceConfig.h"

int
uniDrcInterfaceConfigEnc1(UniDrcInterface* uniDrcInterface)
{
    
    /********************************************************************************/
    /* uniDrcInterfaceSignature */
    /********************************************************************************/
    
    uniDrcInterface->uniDrcInterfaceSignaturePresent                               = 1;
    
    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureType         = 0;
    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength   = 2;
    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[0]      = 'E';
    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[1]      = 'N';
    uniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[2]      = 'G';

    /********************************************************************************/
    /* systemInterface */
    /********************************************************************************/
    
    uniDrcInterface->systemInterfacePresent                                        = 1;
    uniDrcInterface->systemInterface.targetConfigRequestType                       = 0;
    
    uniDrcInterface->systemInterface.numDownmixIdRequests                          = 3;
    uniDrcInterface->systemInterface.downmixIdRequested[0]                         = 7;
    uniDrcInterface->systemInterface.downmixIdRequested[1]                         = 8;
    uniDrcInterface->systemInterface.downmixIdRequested[2]                         = 9;

    uniDrcInterface->systemInterface.targetLayoutRequested                         = 4;
    
    uniDrcInterface->systemInterface.targetChannelCountRequested                   = 24;
    
    /********************************************************************************/
    /* loudnessNormalizationControlInterface */
    /********************************************************************************/
    
    uniDrcInterface->loudnessNormalizationControlInterfacePresent                  = 1;
    
    uniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn = 1;
    uniDrcInterface->loudnessNormalizationControlInterface.targetLoudness          = -16;
    
    /********************************************************************************/
    /* loudnessNormalizationParameterInterface */
    /********************************************************************************/
    
    uniDrcInterface->loudnessNormalizationParameterInterfacePresent                                        = 1;
    
    uniDrcInterface->loudnessNormalizationParameterInterface.albumMode                                     = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent                            = 1;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax                    = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax                          = 3;
    
    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod               = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod                     = 0;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem               = 1;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem                     = 3;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc              = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc                    = 0;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency                   = 1;
    uniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency                         = 420;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax          = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax                = 10;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb = 1;
    uniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb       = - 1.45f;

    uniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax                      = 0;
    uniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax                            = 10;
    
    /********************************************************************************/
    /* dynamicRangeControlInterface */
    /********************************************************************************/
    
    uniDrcInterface->dynamicRangeControlInterfacePresent                            = 1;
    
    uniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn             = 1;
    uniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests             = 3;
    
    uniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[0]          = 0;
    uniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[0]       = 3;
    uniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[0]= 3;
    uniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[0][0]        = 0;
    uniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[0][1]        = 1;
    uniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[0][2]        = 2;

    uniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[1]          = 1;
    uniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[1] = 1;
    uniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[1]       = 0;
    uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[1]       = 20;
    uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[1]    = 30;
    uniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[1]    = 10;

    uniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[2]          = 2;
    uniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[2]       = 4;
    
    /********************************************************************************/
    /* dynamicRangeControlParameterInterface */
    /********************************************************************************/
    
    uniDrcInterface->dynamicRangeControlParameterInterfacePresent                  = 1;
    
    uniDrcInterface->dynamicRangeControlParameterInterface.changeCompress          = 1;
    uniDrcInterface->dynamicRangeControlParameterInterface.changeBoost             = 1;
    
    uniDrcInterface->dynamicRangeControlParameterInterface.compress                = 0.1456f;
    uniDrcInterface->dynamicRangeControlParameterInterface.boost                   = 0.834f;

    uniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget = 1;
    uniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget       = 3;
    
    /********************************************************************************/
    /* uniDrcInterfaceExtension */
    /********************************************************************************/
    
    uniDrcInterface->uniDrcInterfaceExtensionPresent                               = 1;
    
    uniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].uniDrcInterfaceExtType = 0x1;
    
    uniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extSizeBits            = 4;
    uniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extBitSize             = 12;

    /********************************************************************************/
    /* Exit */
    /********************************************************************************/
    
    return (0);
}
