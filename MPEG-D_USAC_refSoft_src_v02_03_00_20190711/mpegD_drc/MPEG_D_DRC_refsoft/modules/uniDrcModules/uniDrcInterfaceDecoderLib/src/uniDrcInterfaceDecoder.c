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

#include "uniDrcInterfaceDecoder.h"
#include "uniDrcInterfaceParser.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
/* open */
int
openUniDrcInterfaceDecoder(HANDLE_UNI_DRC_IF_DEC_STRUCT *phUniDrcIfDecStruct, HANDLE_UNI_DRC_INTERFACE *phUniDrcInterface)
{
    int err = 0;
    UNI_DRC_IF_DEC_STRUCT *hUniDrcIfDecStruct = NULL;
    hUniDrcIfDecStruct = (HANDLE_UNI_DRC_IF_DEC_STRUCT)calloc(1,sizeof(struct T_UNI_DRC_INTERFACE_DECODER_STRUCT));
    
    memset(&hUniDrcIfDecStruct->bitstreamUniDrcInterface, 0, sizeof(robitbuf));
    
    *phUniDrcIfDecStruct = hUniDrcIfDecStruct;
    
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
initUniDrcInterfaceDecoder(HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct)
{
    hUniDrcIfDecStruct->firstFrame = 0;

    return 0;
}

/* process uniDrcConfig */
int
processUniDrcInterfaceDecoder(HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct,
                              HANDLE_UNI_DRC_INTERFACE hUniDrcInterface,
                              unsigned char* bitstream,
                              const int nBitsBitstream,
                              int* nBitsRead)
{
    int err = 0;
    
    robitbufHandle hBitstream = &hUniDrcIfDecStruct->bitstreamUniDrcInterface;
    
    /* robitbuf */
    if (hBitstream != NULL && nBitsBitstream) {
        robitbuf_Init(hBitstream, bitstream, nBitsBitstream, 0);
    } else {
        return -1;
    }
    
    /* parse uniDrcInterface */
    err = parseUniDrcInterface(hBitstream, hUniDrcInterface);
    if (err) return(err);
    
    *nBitsRead =robitbuf_GetBitsRead(hBitstream);
    
    return err;
    
}

/* close */
int closeUniDrcInterfaceDecoder(HANDLE_UNI_DRC_IF_DEC_STRUCT *phUniDrcIfDecStruct, HANDLE_UNI_DRC_INTERFACE *phUniDrcInterface)
{
    if ( *phUniDrcIfDecStruct != NULL )
    {
        free( *phUniDrcIfDecStruct );
        *phUniDrcIfDecStruct = NULL;
        
    }
    if ( *phUniDrcInterface != NULL )
    {
        free( *phUniDrcInterface );
        *phUniDrcInterface = NULL;
        
    }
    return 0;
}

/* plot info */
void plotInfoUniDrcInterfaceDecoder(HANDLE_UNI_DRC_IF_DEC_STRUCT hUniDrcIfDecStruct, HANDLE_UNI_DRC_INTERFACE hUniDrcInterface)
{
    int i,j;
    
    printf("\n");
    printf("uniDrcInterfaceSignaturePresent                = %d\n",hUniDrcInterface->uniDrcInterfaceSignaturePresent);
    if (hUniDrcInterface->uniDrcInterfaceSignaturePresent) {
        printf("   uniDrcInterfaceSignatureType                = %d\n",hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureType);
        printf("   uniDrcInterfaceSignatureDataLength          = %d\n",hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength+1);
        printf("   uniDrcInterfaceSignatureData                = ");
        for (i=0; i<hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureDataLength+1; i++) {
            printf("%s",(unsigned char*)(&hUniDrcInterface->uniDrcInterfaceSignature.uniDrcInterfaceSignatureData[i]));
        }
        printf("\n");
    }

    printf("\n");
    printf("systemInterfacePresent                         = %d\n",hUniDrcInterface->systemInterfacePresent);
    if (hUniDrcInterface->systemInterfacePresent) {
        printf("   targetConfigRequestType                     = %d\n",hUniDrcInterface->systemInterface.targetConfigRequestType);
        switch (hUniDrcInterface->systemInterface.targetConfigRequestType) {
            case 0:
                printf("   numDownmixIdRequests                        = %d\n",hUniDrcInterface->systemInterface.numDownmixIdRequests);
                for (i=0; i<hUniDrcInterface->systemInterface.numDownmixIdRequests; i++) {
                    printf("   downmixIdRequested[%d]                       = %d\n",i,hUniDrcInterface->systemInterface.downmixIdRequested[i]);
                }
                break;
            case 1:
                printf("   targetLayoutRequested                       = %d\n",hUniDrcInterface->systemInterface.targetLayoutRequested);
                break;
            case 2:
                printf("   targetChannelCountRequested                 = %d\n",hUniDrcInterface->systemInterface.targetChannelCountRequested);
                break;
        }
    }
    
    printf("\n");
    printf("loudnessNormalizationControlInterfacePresent   = %d\n",hUniDrcInterface->loudnessNormalizationControlInterfacePresent);
    if (hUniDrcInterface->loudnessNormalizationControlInterfacePresent) {
        printf("   loudnessNormalizationOn                     = %d\n",hUniDrcInterface->loudnessNormalizationControlInterface.loudnessNormalizationOn);
        printf("   targetLoudness                              = %4.4f [LKFS]\n",hUniDrcInterface->loudnessNormalizationControlInterface.targetLoudness);
    }

    printf("\n");
    printf("loudnessNormalizationParameterInterfacePresent = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterfacePresent);
    if (hUniDrcInterface->loudnessNormalizationParameterInterfacePresent) {
        printf("   albumMode                                   = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.albumMode);
        printf("   peakLimiterPresent                          = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.peakLimiterPresent);
        
        printf("   changeLoudnessDeviationMax                  = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessDeviationMax) {
            printf("   loudnessDeviationMax                        = %d [LKFS]\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessDeviationMax);
        }
        
        printf("   changeLoudnessMeasurementMethod             = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementMethod) {
            printf("   loudnessMeasurementMethod                   = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementMethod);
        }
        
        printf("   changeLoudnessMeasurementSystem             = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementSystem) {
            printf("   loudnessMeasurementSystem                   = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementSystem);
        }
        
        printf("   changeLoudnessMeasurementPreProc            = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessMeasurementPreProc) {
            printf("   loudnessMeasurementPreProc                  = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessMeasurementPreProc);
        }
        
        printf("   changeDeviceCutOffFrequency                 = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeDeviceCutOffFrequency) {
            printf("   deviceCutOffFrequency                       = %d [Hz]\n",hUniDrcInterface->loudnessNormalizationParameterInterface.deviceCutOffFrequency);
        }
        
        printf("   changeLoudnessNormalizationGainDbMax        = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainDbMax) {
            printf("   loudnessNormalizationGainDbMax              = %4.2f [dB]\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainDbMax);
        }
        
        printf("   changeLoudnessNormalizationGainModificationDb = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeLoudnessNormalizationGainModificationDb) {
            printf("   loudnessNormalizationGainModificationDb     = %4.4f [dB]\n",hUniDrcInterface->loudnessNormalizationParameterInterface.loudnessNormalizationGainModificationDb);
        }
        
        printf("   changeOutputPeakLevelMax                    = %d\n",hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax);
        if (hUniDrcInterface->loudnessNormalizationParameterInterface.changeOutputPeakLevelMax) {
            printf("   outputPeakLevelMax                          = %4.4f [dBFS]\n",hUniDrcInterface->loudnessNormalizationParameterInterface.outputPeakLevelMax);
        }
    }
    
    printf("\n");
    printf("dynamicRangeControlInterfacePresent            = %d\n",hUniDrcInterface->dynamicRangeControlInterfacePresent);
    if (hUniDrcInterface->dynamicRangeControlInterfacePresent) {
        printf("   dynamicRangeControlOn                       = %d\n",hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeControlOn);
        
        printf("   numDrcFeatureRequests                       = %d\n",hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests);
        
        for (i=0; i<hUniDrcInterface->dynamicRangeControlInterface.numDrcFeatureRequests; i++) {
            printf("   drcFeatureRequestType[%d]                    = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]);
            
            switch (hUniDrcInterface->dynamicRangeControlInterface.drcFeatureRequestType[i]) {
                case 0:
                    printf("      numDrcEffectTypeRequests[%d]              = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]);
                    printf("      numDrcEffectTypeRequestsDesired[%d]       = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequestsDesired[i]);

                    for (j=0; j<hUniDrcInterface->dynamicRangeControlInterface.numDrcEffectTypeRequests[i]; j++) {
                        printf("      drcEffectTypeRequest[%d][%d]               = %d\n",i,j,hUniDrcInterface->dynamicRangeControlInterface.drcEffectTypeRequest[i][j]);
                    }
                    break;
                case 1:
                    printf("      dynRangeMeasurementRequestType[%d]        = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.dynRangeMeasurementRequestType[i]);
                    printf("      dynRangeRequestedIsRange[%d]              = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i]);
                    if (hUniDrcInterface->dynamicRangeControlInterface.dynRangeRequestedIsRange[i]==0) {
                        printf("      dynamicRangeRequestValue[%d]              = %4.4f [LU]\n",i,hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValue[i]);
                    } else {
                        printf("      dynamicRangeRequestValueMin[%d]           = %4.4f [LU]\n",i,hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMin[i]);
                        printf("      dynamicRangeRequestValueMax[%d]           = %4.4f [LU]\n",i,hUniDrcInterface->dynamicRangeControlInterface.dynamicRangeRequestValueMax[i]);
                    }
                    break;
                case 2:
                    printf("      drcCharacteristicRequest[%d]              = %d\n",i,hUniDrcInterface->dynamicRangeControlInterface.drcCharacteristicRequest[i]);
                    break;
            }
        }
    }
    
    printf("\n");
    printf("dynamicRangeControlParameterInterfacePresent   = %d\n",hUniDrcInterface->dynamicRangeControlParameterInterfacePresent);
    if (hUniDrcInterface->dynamicRangeControlParameterInterfacePresent) {
        printf("   changeCompress                              = %d\n",hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress);
        printf("   changeBoost                                 = %d\n",hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost);
        
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeCompress) {
            printf("   compress                                    = %1.4f\n",hUniDrcInterface->dynamicRangeControlParameterInterface.compress);
        }
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeBoost) {
            printf("   boost                                       = %1.4f\n",hUniDrcInterface->dynamicRangeControlParameterInterface.boost);
        }
        
        printf("   changeDrcCharacteristicTarget               = %d\n",hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget);
        if (hUniDrcInterface->dynamicRangeControlParameterInterface.changeDrcCharacteristicTarget) {
            printf("   drcCharacteristicTarget                     = %d\n",hUniDrcInterface->dynamicRangeControlParameterInterface.drcCharacteristicTarget);
        }
    }
    
    printf("\n");
    printf("uniDrcInterfaceExtensionPresent                = %d\n",hUniDrcInterface->uniDrcInterfaceExtensionPresent);
    if (hUniDrcInterface->uniDrcInterfaceExtensionPresent) {
        printf("   uniDrcInterfaceExtType[0]                   = %d\n",hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].uniDrcInterfaceExtType);
        printf("   extSizeBits                                 = %d\n",hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extSizeBits);
        printf("   extBitSize                                  = %d\n",hUniDrcInterface->uniDrcInterfaceExtension.specificInterfaceExtension[0].extBitSize);
    }
    
}
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

