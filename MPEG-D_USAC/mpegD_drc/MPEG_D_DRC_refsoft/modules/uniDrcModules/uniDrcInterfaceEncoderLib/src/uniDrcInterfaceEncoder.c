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
#include <math.h>
#include <assert.h>
#include <string.h>

#include "uniDrcInterfaceEncoder.h"
#include "uniDrcInterfaceWriter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
/* open */
int
openUniDrcInterfaceEncoder(HANDLE_UNI_DRC_IF_ENC_STRUCT *phUniDrcIfEncStruct, HANDLE_UNI_DRC_INTERFACE *phUniDrcInterface)
{
    int err = 0;
    UNI_DRC_IF_ENC_STRUCT *hUniDrcIfEncStruct = NULL;
    hUniDrcIfEncStruct = (HANDLE_UNI_DRC_IF_ENC_STRUCT)calloc(1,sizeof(struct T_UNI_DRC_INTERFACE_ENCODER_STRUCT));
    
    memset(&hUniDrcIfEncStruct->bitstreamUniDrcInterface, 0, sizeof(wobitbuf));
    
    *phUniDrcIfEncStruct = hUniDrcIfEncStruct;
    
    if ( *phUniDrcInterface == NULL)
    {
        UniDrcInterface *hUniDrcInterface = NULL;
        hUniDrcInterface = (HANDLE_UNI_DRC_INTERFACE)calloc(1,sizeof(struct T_UNI_DRC_INTERFACE_STRUCT));
        
        *phUniDrcInterface = hUniDrcInterface;
    }
    
    return err;
}

/* init */
int
initUniDrcInterfaceEncoder(HANDLE_UNI_DRC_IF_ENC_STRUCT hUniDrcIfEncStruct)
{
    hUniDrcIfEncStruct->firstFrame = 0;

    return 0;
}

/* process uniDrcConfig */
int
processUniDrcInterfaceEncoder(HANDLE_UNI_DRC_IF_ENC_STRUCT hUniDrcIfEncStruct,
                              HANDLE_UNI_DRC_INTERFACE hUniDrcInterface,
                              unsigned char* bitstream,
                              int nBytesBitstream,
                              int* nBitsWritten)
{
    int err = 0;
    
    wobitbufHandle hBitstream = &hUniDrcIfEncStruct->bitstreamUniDrcInterface;
    
    /* wobitbuf */
    if (hBitstream != NULL) {
        wobitbuf_Init(hBitstream, bitstream, nBytesBitstream*8, 0);
    } else {
        return -1;
    }
    
    /* write uniDrcInterface */
    err = writeUniDrcInterface(hBitstream, hUniDrcInterface);
    if (err) return(err);
    
    *nBitsWritten =wobitbuf_GetBitsWritten(hBitstream);
    
    return err;
    
}

/* close */
int closeUniDrcInterfaceEncoder(HANDLE_UNI_DRC_IF_ENC_STRUCT *phUniDrcIfEncStruct, HANDLE_UNI_DRC_INTERFACE *phUniDrcInterface)
{
    if ( *phUniDrcIfEncStruct != NULL )
    {
        free( *phUniDrcIfEncStruct );
        *phUniDrcIfEncStruct = NULL;
        
    }
    if ( *phUniDrcInterface != NULL )
    {
        free( *phUniDrcInterface );
        *phUniDrcInterface = NULL;
        
    }
    return 0;
}

/* plot info */
void plotInfoUniDrcInterfaceEncoder(HANDLE_UNI_DRC_INTERFACE hUniDrcInterface)
{    
    int i, j;
    printf("\n");
    printf("=========================================\n");
    printf("uniDrcInterface() info: \n");
    printf("-----------------------------------------\n");
    printf("uniDrcInterfaceSignaturePresent = %d\n", hUniDrcInterface->uniDrcInterfaceSignaturePresent);
    if (hUniDrcInterface->uniDrcInterfaceSignaturePresent) {
        printf("     uniDrcInterfaceSignature->uniDrcInterfaceSignatureType = %d\n", hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureType);
        printf("     uniDrcInterfaceSignature->uniDrcInterfaceSignatureDataLength = %d\n", hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength);
        printf("     uniDrcInterfaceSignature->uniDrcInterfaceSignatureData = '");
        for (i=0; i<hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength+1; i++) {
            printf("%s", (char*)&hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[i]);
        }
        printf("'\n");
    }
    printf("-----------------------------------------\n");
    printf("systemInterfacePresent = %d\n", hUniDrcInterface->systemInterfacePresent);
    if (hUniDrcInterface->systemInterfacePresent) {
        printf("     systemInterface->targetConfigRequestType = %d\n", hUniDrcInterface->systemInterface.targetConfigRequestType);
        switch (hUniDrcInterface->systemInterface.targetConfigRequestType) {
            case 0:
                printf("     systemInterface->numDownmixIdRequests = %d\n", hUniDrcInterface->systemInterface.numDownmixIdRequests);
                for (i=0; i<hUniDrcInterface->systemInterface.numDownmixIdRequests; i++) {
                    printf("     systemInterface->downmixIdRequested[%d] = %d\n", i, hUniDrcInterface->systemInterface.downmixIdRequested[i]);
                }
                break;
            case 1:
                printf("     systemInterface->targetLayoutRequested = %d\n", hUniDrcInterface->systemInterface.targetLayoutRequested);
                break;
            case 2:
                printf("     systemInterface->targetChannelCountRequested = %d\n", hUniDrcInterface->systemInterface.targetChannelCountRequested);
                break;
            default:
                break;
        }
    }
    printf("-----------------------------------------\n");
    printf("loudnessNormalizationControlInterfacePresent = %d\n", hUniDrcInterface->loudnessNormalizationControlInterfacePresent);
    if (hUniDrcInterface->loudnessNormalizationControlInterfacePresent) {
        printf("     loudnessNormalizationControlInterface->loudnessNormalizationOn = %d\n", hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn);
        if (hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn) {
            printf("     loudnessNormalizationControlInterface->targetLoudness = %2.2f dB\n", hUniDrcInterface->loudnessNormalizationControlInterface.targetLoudness);
        }
    }
    printf("-----------------------------------------\n");
    printf("loudnessNormalizationParameterInterfacePresent = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterfacePresent);
    if (hUniDrcInterface->loudnessNormalizationParameterInterfacePresent) {
        printf("     loudnessNormalizationParameterInterface->albumMode = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.albumMode);
        printf("     loudnessNormalizationParameterInterface->peakLimiterPresent = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent);
        printf("     loudnessNormalizationParameterInterface->changeLoudnessDeviationMax = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax) {
            printf("     loudnessNormalizationParameterInterface->loudnessDeviationMax = %d dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax);
        }
        printf("     loudnessNormalizationParameterInterface->changeLoudnessMeasurementMethod = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod) {
            printf("     loudnessNormalizationParameterInterface->loudnessMeasurementMethod = %d dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod);
        }
        printf("     loudnessNormalizationParameterInterface->changeLoudnessMeasurementSystem = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem) {
            printf("     loudnessNormalizationParameterInterface->loudnessMeasurementSystem = %d dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem);
        }
        printf("     loudnessNormalizationParameterInterface->changeLoudnessMeasurementPreProc = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc) {
            printf("     loudnessNormalizationParameterInterface->loudnessMeasurementPreProc = %d dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc);
        }
        printf("     loudnessNormalizationParameterInterface->changeDeviceCutOffFrequency = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency) {
            printf("     loudnessNormalizationParameterInterface->deviceCutOffFrequency = %d Hz\n", hUniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency);
        }
        printf("     loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainDbMax = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax) {
            printf("     loudnessNormalizationParameterInterface->loudnessNormalizationGainDbMax = %2.2f dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax);
        }
        printf("     loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainModificationDb = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb) {
            printf("     loudnessNormalizationParameterInterface->loudnessNormalizationGainModificationDb = %2.2f dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb);
        }
        printf("     loudnessNormalizationParameterInterface->changeOutputPeakLevelMax = %d\n", hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax) {
            printf("     loudnessNormalizationParameterInterface->outputPeakLevelMax = %2.2f dB\n", hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax);
        }
    }
    printf("-----------------------------------------\n");
    printf("dynamicRangeControlInterfacePresent = %d\n", hUniDrcInterface->dynamicRangeControlInterfacePresent);
    if (hUniDrcInterface->dynamicRangeControlInterfacePresent) {
        printf("     dynamicRangeControlInterface->dynamicRangeControlOn = %d\n", hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn);
        if (hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn) {
            printf("     dynamicRangeControlInterface->numDrcFeatureRequests = %d\n", hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests);
            for (i=0; i<hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests; i++) {
                printf("     dynamicRangeControlInterface->drcFeatureRequestType[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]);
                switch (hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]) {
                    case 0:
                        printf("     dynamicRangeControlInterface->numDrcEffectTypeRequests[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]);
                        printf("     dynamicRangeControlInterface->numDrcEffectTypeRequestsDesired[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[i]);
                        for (j=0; j<hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]; j++) {
                            printf("     dynamicRangeControlInterface->drcEffectTypeRequest[%d][%d] = %d\n", i, j, hUniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[i][j]);
                            
                        }
                        break;
                    case 1:
                        printf("     dynamicRangeControlInterface->dynRangeMeasurementRequestType[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[i]);
                        printf("     dynamicRangeControlInterface->dynRangeRequestedIsRange[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i]);
                        printf("     dynamicRangeControlInterface->dynamicRangeRequestValue[%d] = %2.2f dB\n", i, hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[i]);
                        printf("     dynamicRangeControlInterface->dynamicRangeRequestValueMin[%d] = %2.2f dB\n", i, hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[i]);
                        printf("     dynamicRangeControlInterface->dynamicRangeRequestValueMax[%d] = %2.2f dB\n", i, hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[i]);
                        break;
                    case 2:
                        printf("     dynamicRangeControlInterface->drcCharacteristicRequest[%d] = %d\n", i, hUniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[i]);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    printf("-----------------------------------------\n");
    printf("dynamicRangeControlParameterInterfacePresent = %d\n", hUniDrcInterface->dynamicRangeControlParameterInterfacePresent);
    if (hUniDrcInterface->dynamicRangeControlParameterInterfacePresent) {
        printf("     dynamicRangeControlParameterInterface->changeCompress = %d\n", hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress);
        printf("     dynamicRangeControlParameterInterface->changeBoost = %d\n", hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost);
        
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress) {
            printf("     dynamicRangeControlParameterInterface->compress = %2.2f\n", hUniDrcInterface->dynamicRangeControlParameterInterface.compress);
        }
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost) {
            printf("     dynamicRangeControlParameterInterface->boost = %2.2f\n", hUniDrcInterface->dynamicRangeControlParameterInterface.boost);
        }
        
        printf("     dynamicRangeControlParameterInterface->changeDrcCharacteristicTarget = %d\n", hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget);
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget) {
            printf("     dynamicRangeControlParameterInterface->drcCharacteristicTarget = %d\n", hUniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget);
        }
    }
    printf("-----------------------------------------\n");
    printf("uniDrcInterfaceExtensionPresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtensionPresent);
    if (hUniDrcInterface->uniDrcInterfaceExtensionPresent) {
        printf("    uniDrcInterfaceExtension->uniDrcInterfaceExtType = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].uniDrcInterfaceExtType);
        printf("    uniDrcInterfaceExtension->extSizeBits = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extSizeBits);
        printf("    uniDrcInterfaceExtension->extBitSize = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extBitSize);
#if MPEG_D_DRC_EXTENSION_V1
        printf("    uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent);
        if (hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent)
        {
            printf("        loudnessEqParameterInterface.loudnessEqRequestPresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequestPresent);
            if (hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequestPresent)
            {
                printf("        loudnessEqParameterInterface.loudnessEqRequest = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.loudnessEqRequest);
            }
            printf("        loudnessEqParameterInterface.sensitivityPresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivityPresent);
            if (hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivityPresent)
            {
                printf("        loudnessEqParameterInterface.sensitivity = %f\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.sensitivity);
            }
            printf("        loudnessEqParameterInterface.playbackGainPresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGainPresent);
            if (hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGainPresent)
            {
                printf("        loudnessEqParameterInterface.playbackGain = %f\n", hUniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface.playbackGain);
            }
        }
        printf("    uniDrcInterfaceExtension->equalizationControlInterfacePresent = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent);
        if (hUniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent)
        {
            printf("        equalizationControlInterface.eqSetPurposeRequest = %d\n", hUniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterface.eqSetPurposeRequest);
        }
#endif
    }
    printf("=========================================\n");
}
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

