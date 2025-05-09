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
#include <string.h>
#include <math.h>
#include "uniDrcInterfaceEncoder.h"
#include "uniDrcInterfaceWriter.h"

#define MAX_INTERFACE_PAYLOAD_BYTES  1000

/* ====================================================================================
                                Get bits from robitbuf
 ====================================================================================*/

int
writeBitsUniDrcInterface(wobitbufHandle bitstream,
                         const unsigned int data,
                         const int nBits)
{
    int err = 0;
    
    err = (int)wobitbuf_WriteBits(bitstream, data, nBits);
    if (err) return(err);
    
    return (0);
}

/* ====================================================================================
                        Writing of uniDrcInterface()
 ====================================================================================*/

/* Writer for uniDrcInterfaceSignature */
int
writeUniDrcInterfaceSignature(wobitbufHandle bitstream,
                              UniDrcInterfaceSignature* uniDrcInterfaceSignature)
{
    int err = 0, uniDrcInterfaceSignatureDataLength = 0, i;
    
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterfaceSignature->uniDrcInterfaceSignatureType, 8);
    if (err) return(err);
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterfaceSignature->uniDrcInterfaceSignatureDataLength, 8);
    if (err) return(err);
    
    uniDrcInterfaceSignatureDataLength = uniDrcInterfaceSignature->uniDrcInterfaceSignatureDataLength + 1;
    for (i=0; i<uniDrcInterfaceSignatureDataLength; i++) {
        err = writeBitsUniDrcInterface(bitstream, (unsigned int)uniDrcInterfaceSignature->uniDrcInterfaceSignatureData[i], 8);
        if (err) return(err);
    }
    return(0);
}

/* Writer for systemInterface */
int
writeSystemInterface(wobitbufHandle bitstream,
                     SystemInterface* systemInterface)
{
    int err = 0, i = 0, tmp = 0;

    err = writeBitsUniDrcInterface(bitstream, systemInterface->targetConfigRequestType, 2);
    if (err) return(err);
    
    switch (systemInterface->targetConfigRequestType) {
        case 0:
            err = writeBitsUniDrcInterface(bitstream, systemInterface->numDownmixIdRequests, 4);
            if (err) return(err);
            for (i=0; i<systemInterface->numDownmixIdRequests; i++) {
                err = writeBitsUniDrcInterface(bitstream, systemInterface->downmixIdRequested[i], 7);
                if (err) return(err);
            }
            break;
        case 1:
            err = writeBitsUniDrcInterface(bitstream, systemInterface->targetLayoutRequested, 8);
            if (err) return(err);
            break;
        case 2:
            tmp = systemInterface->targetChannelCountRequested - 1;
            err = writeBitsUniDrcInterface(bitstream, tmp, 7);
            if (err) return(err);
            break;
        default:
            printf("targetConfigRequestType==3 is not supported!\n");
            return(1);
            break;
    }
    return(0);
}

/* Writer for loudnessNormalizationControlInterface */
int
writeLoudnessNormalizationControlInterface(wobitbufHandle bitstream,
                                           LoudnessNormalizationControlInterface* loudnessNormalizationControlInterface)
{
    int err = 0, tmp = 0;
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationControlInterface->loudnessNormalizationOn, 1);
    if (err) return(err);
    
    if (loudnessNormalizationControlInterface->loudnessNormalizationOn == 1) {
        tmp = - loudnessNormalizationControlInterface->targetLoudness*32;
        err = writeBitsUniDrcInterface(bitstream, tmp, 12);
        if (err) return(err);
    }
    return(0);
}

/* Writer for loudnessNormalizationParameterInterface */
int
writeLoudnessNormalizationParameterInterface(wobitbufHandle bitstream,
                                             LoudnessNormalizationParameterInterface* loudnessNormalizationParameterInterface)
{
    int err = 0, tmp = 0;
    
    err = writeBitsUniDrcInterface(bitstream,loudnessNormalizationParameterInterface->albumMode, 1);
    if (err) return(err);
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->peakLimiterPresent, 1);
    if (err) return(err);
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessDeviationMax, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessDeviationMax == 1) {
        err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->loudnessDeviationMax, 6);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessMeasurementMethod, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementMethod == 1) {
        err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->loudnessMeasurementMethod, 3);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessMeasurementSystem, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementSystem == 1) {
        err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->loudnessMeasurementSystem, 4);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessMeasurementPreProc, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementPreProc == 1) {
        err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->loudnessMeasurementPreProc, 2);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeDeviceCutOffFrequency, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeDeviceCutOffFrequency == 1) {
        tmp = loudnessNormalizationParameterInterface->deviceCutOffFrequency*0.1f;
        err = writeBitsUniDrcInterface(bitstream, tmp, 6);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainDbMax, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainDbMax == 1) {
        if (loudnessNormalizationParameterInterface->loudnessNormalizationGainDbMax > 31.f) {
            tmp = 255;
        } else {
            tmp = loudnessNormalizationParameterInterface->loudnessNormalizationGainDbMax*2;
        }
        err = writeBitsUniDrcInterface(bitstream, tmp, 6);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainModificationDb, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainModificationDb == 1) {
        tmp = (loudnessNormalizationParameterInterface->loudnessNormalizationGainModificationDb+16)*32;
        err = writeBitsUniDrcInterface(bitstream, tmp, 10);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, loudnessNormalizationParameterInterface->changeOutputPeakLevelMax, 1);
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeOutputPeakLevelMax == 1) {
        tmp = loudnessNormalizationParameterInterface->outputPeakLevelMax*2;
        err = writeBitsUniDrcInterface(bitstream, tmp, 6);
        if (err) return(err);
    }
    
    return(0);
}

/* Writer for dynamicRangeControlInterface */
int
writeDynamicRangeControlInterface(wobitbufHandle bitstream,
                                  DynamicRangeControlInterface* dynamicRangeControlInterface)
{
    int err = 0, i = 0, j = 0, tmp = 0;
    
    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->dynamicRangeControlOn, 1);
    if (err) return(err);
    
    if (dynamicRangeControlInterface->dynamicRangeControlOn == 1) {
        err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->numDrcFeatureRequests, 3);
        if (err) return(err);

        for (i=0; i<dynamicRangeControlInterface->numDrcFeatureRequests; i++) {
            err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->drcFeatureRequestType[i], 2);
            if (err) return(err);
            
            switch (dynamicRangeControlInterface->drcFeatureRequestType[i]) {
                case 0:
                    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->numDrcEffectTypeRequests[i], 4);
                    if (err) return(err);
                    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->numDrcEffectTypeRequestsDesired[i], 4);
                    if (err) return(err);
                    for (j=0; j<dynamicRangeControlInterface->numDrcEffectTypeRequests[i]; j++) {
                        err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->drcEffectTypeRequest[i][j], 4);
                        if (err) return(err);
                    }
                    break;
                case 1:
                    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->dynRangeMeasurementRequestType[i], 2);
                    if (err) return(err);
                    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->dynRangeRequestedIsRange[i], 1);
                    if (err) return(err);
                    if (dynamicRangeControlInterface->dynRangeRequestedIsRange[i] == 0) {
                        if (dynamicRangeControlInterface->dynamicRangeRequestValue[i] < 0.0f)
                            tmp = 0;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValue[i] <= 32.0f)
                            tmp = (int) (4.0f * dynamicRangeControlInterface->dynamicRangeRequestValue[i] + 0.5f);
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValue[i] <= 70.0f)
                            tmp = ((int)(2.0f * (dynamicRangeControlInterface->dynamicRangeRequestValue[i] - 32.0f) + 0.5f)) + 128;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValue[i] < 121.0f)
                            tmp = ((int) ((dynamicRangeControlInterface->dynamicRangeRequestValue[i] - 70.0f) + 0.5f)) + 204;
                        else
                            tmp = 255;
                        err = writeBitsUniDrcInterface(bitstream, tmp, 8);
                        if (err) return(err);
                    } else {
                        if (dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] < 0.0f)
                            tmp = 0;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] <= 32.0f)
                            tmp = (int) (4.0f * dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] + 0.5f);
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] <= 70.0f)
                            tmp = ((int)(2.0f * (dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] - 32.0f) + 0.5f)) + 128;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] < 121.0f)
                            tmp = ((int) ((dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] - 70.0f) + 0.5f)) + 204;
                        else
                            tmp = 255;
                        err = writeBitsUniDrcInterface(bitstream, tmp, 8);
                        if (err) return(err);
                        
                        if (dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] < 0.0f)
                            tmp = 0;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] <= 32.0f)
                            tmp = (int) (4.0f * dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] + 0.5f);
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] <= 70.0f)
                            tmp = ((int)(2.0f * (dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] - 32.0f) + 0.5f)) + 128;
                        else if (dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] < 121.0f)
                            tmp = ((int) ((dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] - 70.0f) + 0.5f)) + 204;
                        else
                            tmp = 255;
                        err = writeBitsUniDrcInterface(bitstream, tmp, 8);
                        if (err) return(err);
                    }
                    break;
                case 2:
                    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlInterface->drcCharacteristicRequest[i], 7);
                    if (err) return(err);
                    break;
                default:
                    printf("drcFeatureRequestType==3 is not supported!\n");
                    return(1);
                    break;
            }
        }
    }
    return(0);
}

/* Writer for dynamicRangeControlParameterInterface */
int
writeDynamicRangeControlParameterInterface(wobitbufHandle bitstream,
                                           DynamicRangeControlParameterInterface* dynamicRangeControlParameterInterface)
{
    int err = 0, tmp = 0;
    
    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlParameterInterface->changeCompress, 1);
    if (err) return(err);
    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlParameterInterface->changeBoost, 1);
    if (err) return(err);
    
    if (dynamicRangeControlParameterInterface->changeCompress == 1) {
        if (dynamicRangeControlParameterInterface->compress == 0.f) {
            tmp = 255;
        } else {
            tmp = (1-dynamicRangeControlParameterInterface->compress)*256;
        }
        err = writeBitsUniDrcInterface(bitstream, tmp, 8);
        if (err) return(err);
    }
    
    if (dynamicRangeControlParameterInterface->changeBoost == 1) {
        if (dynamicRangeControlParameterInterface->boost == 0.f) {
            tmp = 255;
        } else {
            tmp = (1-dynamicRangeControlParameterInterface->boost)*256;
        }
        err = writeBitsUniDrcInterface(bitstream, tmp, 8);
        if (err) return(err);
    }
    
    err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlParameterInterface->changeDrcCharacteristicTarget, 1);
    if (err) return(err);
    
    if (dynamicRangeControlParameterInterface->changeDrcCharacteristicTarget == 1) {
        err = writeBitsUniDrcInterface(bitstream, dynamicRangeControlParameterInterface->drcCharacteristicTarget, 8);
        if (err) return(err);
    }
    return(0);
}

#if MPEG_D_DRC_EXTENSION_V1
int
writeLoudnessEqParameterInterface(wobitbufHandle bitstream,
                                  LoudnessEqParameterInterface* loudnessEqParameterInterface)
{
    int err = 0;
    
    err = writeBitsUniDrcInterface(bitstream, loudnessEqParameterInterface->loudnessEqRequestPresent, 1);
    if (err) return(err);
    if (loudnessEqParameterInterface->loudnessEqRequestPresent)
    {
        err = writeBitsUniDrcInterface(bitstream, loudnessEqParameterInterface->loudnessEqRequest, 2);
        if (err) return(err);
    }
    err = writeBitsUniDrcInterface(bitstream, loudnessEqParameterInterface->sensitivityPresent, 1);
    if (err) return(err);
    if (loudnessEqParameterInterface->sensitivityPresent)
    {
        int bsSensitivity = (int) (loudnessEqParameterInterface->sensitivity - 22.5f);
        bsSensitivity = max(0, min(127, bsSensitivity));
        err = writeBitsUniDrcInterface(bitstream, bsSensitivity, 7);
        if (err) return(err);
    }
    err = writeBitsUniDrcInterface(bitstream, loudnessEqParameterInterface->playbackGainPresent, 1);
    if (err) return(err);
    if (loudnessEqParameterInterface->playbackGainPresent)
    {
        int bsPlaybackGain = (int) (- loudnessEqParameterInterface->playbackGain + 0.5f);
        bsPlaybackGain = max(0, min(127, bsPlaybackGain));
        err = writeBitsUniDrcInterface(bitstream, bsPlaybackGain, 7);
        if (err) return(err);
    }
    return (err);
}

int
writeEqualizationControlInterface(wobitbufHandle bitstream,
                                  EqualizationControlInterface* equalizationControlInterface)
{
    int err;
    
    err = writeBitsUniDrcInterface(bitstream, equalizationControlInterface->eqSetPurposeRequest, 16);
    return(err);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */

/* Writer for uniDrcInterfaceExtension */
int
writeUniDrcInterfaceExtension(wobitbufHandle bitstream,
                              UniDrcInterfaceExtension* uniDrcInterfaceExtension)
{
    int err = 0, num_ext = 0;
#if MPEG_D_DRC_EXTENSION_V1
    if (uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent || uniDrcInterfaceExtension->equalizationControlInterfacePresent)
    {
        int bitSize, bitSizeLen;
        SpecificInterfaceExtension* specificInterfaceExtension = &uniDrcInterfaceExtension->specificInterfaceExtension[0];

        unsigned char bitstreamBufferTmp[MAX_INTERFACE_PAYLOAD_BYTES];
        wobitbuf bs;
        wobitbufHandle bitstreamTmp = &bs;
        wobitbuf_Init(bitstreamTmp, bitstreamBufferTmp, MAX_INTERFACE_PAYLOAD_BYTES*8, 0);
        
        err = writeBitsUniDrcInterface(bitstreamTmp, uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent, 1);
        if (err) return(err);
        if (uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent)
        {
            err = writeLoudnessEqParameterInterface(bitstreamTmp, &uniDrcInterfaceExtension->loudnessEqParameterInterface);
            if (err) return(err);
        }
        err = writeBitsUniDrcInterface(bitstreamTmp, uniDrcInterfaceExtension->equalizationControlInterfacePresent, 1);
        if (err) return(err);
        if (uniDrcInterfaceExtension->equalizationControlInterfacePresent)
        {
            err = writeEqualizationControlInterface(bitstreamTmp, &uniDrcInterfaceExtension->equalizationControlInterface);
            if (err) return(err);
        }
        specificInterfaceExtension->uniDrcInterfaceExtType = UNIDRCINTERFACEEXT_EQ;
        specificInterfaceExtension->extBitSize = wobitbuf_GetBitsWritten(bitstreamTmp);
        bitSize     = specificInterfaceExtension->extBitSize - 1;
        specificInterfaceExtension->extSizeBits = (int)(log((float)bitSize)/log(2.f))+1;
        bitSizeLen  = specificInterfaceExtension->extSizeBits - 4;
        
        err = writeBitsUniDrcInterface(bitstream, specificInterfaceExtension->uniDrcInterfaceExtType, 4);
        if (err) return(err);
        err = writeBitsUniDrcInterface(bitstream, bitSizeLen, 4);
        if (err) return(err);
        err = writeBitsUniDrcInterface(bitstream, bitSize, specificInterfaceExtension->extSizeBits);
        if (err) return(err);
        switch(specificInterfaceExtension->uniDrcInterfaceExtType)
        {
            case UNIDRCINTERFACEEXT_EQ:
                err = writeBitsUniDrcInterface(bitstream, uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent, 1);
                if (err) return(err);
                if (uniDrcInterfaceExtension->loudnessEqParameterInterfacePresent)
                {
                    err = writeLoudnessEqParameterInterface(bitstream, &uniDrcInterfaceExtension->loudnessEqParameterInterface);
                    if (err) return(err);
                }
                err = writeBitsUniDrcInterface(bitstream, uniDrcInterfaceExtension->equalizationControlInterfacePresent, 1);
                if (err) return(err);
                if (uniDrcInterfaceExtension->equalizationControlInterfacePresent)
                {
                    err = writeEqualizationControlInterface(bitstream, &uniDrcInterfaceExtension->equalizationControlInterface);
                    if (err) return(err);
                }
                break;
            default:
                break;
        }

        ++num_ext;
    }
#endif

#ifdef LEVELING_SUPPORT
    if(uniDrcInterfaceExtension->levelingControlInterfacePresent)
    {
        SpecificInterfaceExtension* specificInterfaceExtension = &uniDrcInterfaceExtension->specificInterfaceExtension[num_ext];
        specificInterfaceExtension->uniDrcInterfaceExtType = UNIDRCINTERFACEEXT_LEVELING;
        specificInterfaceExtension->extBitSize = 1;
        specificInterfaceExtension->extSizeBits = 4;
        ++num_ext;

        err = writeBitsUniDrcInterface(bitstream, specificInterfaceExtension->uniDrcInterfaceExtType, 4);
        if (err) return(err);
        err = writeBitsUniDrcInterface(bitstream, specificInterfaceExtension->extSizeBits - 4, 4);
        if (err) return(err);
        err = writeBitsUniDrcInterface(bitstream, specificInterfaceExtension->extBitSize - 1, specificInterfaceExtension->extSizeBits);
        if (err) return(err);

        err = writeBitsUniDrcInterface(bitstream, uniDrcInterfaceExtension->levelingControlInterface.loudnessLevelingOn, 1);
        if (err) return(err);
    }
#endif

    err = writeBitsUniDrcInterface(bitstream, UNIDRCINTERFACEEXT_TERM, 4);
    if (err) return(err);
    return(0);
}

/* Writer for uniDrcInterface */
int
writeUniDrcInterface(wobitbufHandle bitstream,
                     UniDrcInterface* uniDrcInterface)
{
    int err = 0;

    /* uniDrcInterfaceSignature */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->uniDrcInterfaceSignaturePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->uniDrcInterfaceSignaturePresent == 1) {
        err = writeUniDrcInterfaceSignature(bitstream, &(uniDrcInterface->uniDrcInterfaceSignature));
        if (err) return(err);
    }
    
    /* systemInterface */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->systemInterfacePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->systemInterfacePresent == 1) {
        err = writeSystemInterface(bitstream, &(uniDrcInterface->systemInterface));
        if (err) return(err);
    }
    
    /* loudnessNormalizationControlInterface */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->loudnessNormalizationControlInterfacePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->loudnessNormalizationControlInterfacePresent == 1) {
        err = writeLoudnessNormalizationControlInterface(bitstream, &(uniDrcInterface->loudnessNormalizationControlInterface));
        if (err) return(err);
    }
    
    /* loudnessNormalizationParameterInterface */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->loudnessNormalizationParameterInterfacePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->loudnessNormalizationParameterInterfacePresent == 1) {
        err = writeLoudnessNormalizationParameterInterface(bitstream, &(uniDrcInterface->loudnessNormalizationParameterInterface));
        if (err) return(err);
    }
    
    /* dynamicRangeControlInterface */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->dynamicRangeControlInterfacePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->dynamicRangeControlInterfacePresent == 1) {
        err = writeDynamicRangeControlInterface(bitstream, &(uniDrcInterface->dynamicRangeControlInterface));
        if (err) return(err);
    }
    
    /* dynamicRangeControlParameterInterface */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->dynamicRangeControlParameterInterfacePresent, 1);
    if (err) return(err);
    if (uniDrcInterface->dynamicRangeControlParameterInterfacePresent == 1) {
        err = writeDynamicRangeControlParameterInterface(bitstream, &(uniDrcInterface->dynamicRangeControlParameterInterface));
        if (err) return(err);
    }
    
    /* uniDrcInterfaceExtension */
    err = writeBitsUniDrcInterface(bitstream, uniDrcInterface->uniDrcInterfaceExtensionPresent, 1);
    if (err) return(err);
    if (uniDrcInterface->uniDrcInterfaceExtensionPresent == 1) {
        err = writeUniDrcInterfaceExtension(bitstream, &(uniDrcInterface->uniDrcInterfaceExtension));
        if (err) return(err);
    }
    
    return(0);
}