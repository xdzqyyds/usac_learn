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
#include "uniDrcInterfaceDecoder.h"
#include "uniDrcInterfaceParser.h"

/* ====================================================================================
                                Get bits from robitbuf
 ====================================================================================*/

int
getBitsUniDrcInterface(robitbufHandle bitstream,
                 const int nBitsRequested,
                 int* result)
{
    int nBitsRemaining = robitbuf_GetBitsAvail(bitstream);

    if (nBitsRemaining<=0)
    {
        printf("Processing complete. Reached end of bitstream.\n");
        return (1);
    }

    if (result != NULL) {
        *result = (int)robitbuf_ReadBits(bitstream, nBitsRequested);
    } else {
        robitbuf_ReadBits(bitstream, nBitsRequested);
    }

    return (0);
}

/* ====================================================================================
                        Parsing of uniDrcInterface()
 ====================================================================================*/

/* parser for uniDrcInterfaceSignature */
int
parseUniDrcInterfaceSignature(robitbufHandle bitstream,
                              UniDrcInterfaceSignature* uniDrcInterfaceSignature)
{
    int err = 0, uniDrcInterfaceSignatureDataLength = 0, i, tmp;

    err = getBitsUniDrcInterface(bitstream, 8, &(uniDrcInterfaceSignature->uniDrcInterfaceSignatureType));
    if (err) return(err);
    err = getBitsUniDrcInterface(bitstream, 8, &(uniDrcInterfaceSignature->uniDrcInterfaceSignatureDataLength));
    if (err) return(err);

    uniDrcInterfaceSignatureDataLength = uniDrcInterfaceSignature->uniDrcInterfaceSignatureDataLength + 1;
    for (i=0; i<uniDrcInterfaceSignatureDataLength; i++) {
        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
        if (err) return(err);
        uniDrcInterfaceSignature->uniDrcInterfaceSignatureData[i] = (unsigned int)tmp;
    }

    return(0);
}

/* parser for systemInterface */
int
parseSystemInterface(robitbufHandle bitstream,
                     SystemInterface* systemInterface)
{
    int err = 0, i = 0, tmp = 0;

    /* set default */
    /*systemInterface->numDownmixIdRequests        = 1;
    systemInterface->downmixIdRequested[0]       = 0;
    systemInterface->targetLayoutRequested       = 0;
    systemInterface->targetChannelCountRequested = 0;*/

    /* parse */
    err = getBitsUniDrcInterface(bitstream, 2, &(systemInterface->targetConfigRequestType));
    if (err) return(err);

    switch (systemInterface->targetConfigRequestType) {
        case 0:
            err = getBitsUniDrcInterface(bitstream, 4, &(systemInterface->numDownmixIdRequests));
            if (err) return(err);

            if (systemInterface->numDownmixIdRequests == 0) {
                systemInterface->numDownmixIdRequests = 1;
                systemInterface->downmixIdRequested[0] = 0;
                break;
            }
            for (i=0; i<systemInterface->numDownmixIdRequests; i++) {
                err = getBitsUniDrcInterface(bitstream, 7, &(systemInterface->downmixIdRequested[i]));
                if (err) return(err);
            }
            break;
        case 1:
            err = getBitsUniDrcInterface(bitstream, 8, &(systemInterface->targetLayoutRequested));
            if (err) return(err);
            break;
        case 2:
            err = getBitsUniDrcInterface(bitstream, 7, &tmp);
            if (err) return(err);
            systemInterface->targetChannelCountRequested = tmp + 1;
            break;
        default:
            printf("targetConfigRequestType==3 is not supported!\n");
            return(1);
            break;
    }
    return(0);
}

/* parser for loudnessNormalizationControlInterface */
int
parseLoudnessNormalizationControlInterface(robitbufHandle bitstream,
                                           LoudnessNormalizationControlInterface* loudnessNormalizationControlInterface)
{
    int err = 0, tmp = 0;

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationControlInterface->loudnessNormalizationOn));
    if (err) return(err);

    if (loudnessNormalizationControlInterface->loudnessNormalizationOn == 1) {
        err = getBitsUniDrcInterface(bitstream, 12, &tmp);
        if (err) return(err);
        loudnessNormalizationControlInterface->targetLoudness = - tmp * 0.03125f;
    }
    return(0);
}

/* parser for loudnessNormalizationParameterInterface */
int
parseLoudnessNormalizationParameterInterface(robitbufHandle bitstream,
                                             LoudnessNormalizationParameterInterface* loudnessNormalizationParameterInterface)
{
    int err = 0, tmp = 0;

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->albumMode));
    if (err) return(err);
    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->peakLimiterPresent));
    if (err) return(err);

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessDeviationMax));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessDeviationMax == 1) {
        err = getBitsUniDrcInterface(bitstream, 6, &(loudnessNormalizationParameterInterface->loudnessDeviationMax));
        if (err) return(err);
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessMeasurementMethod));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementMethod == 1) {
        err = getBitsUniDrcInterface(bitstream, 3, &(loudnessNormalizationParameterInterface->loudnessMeasurementMethod));
        if (err) return(err);
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessMeasurementSystem));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementSystem == 1) {
        err = getBitsUniDrcInterface(bitstream, 4, &(loudnessNormalizationParameterInterface->loudnessMeasurementSystem));
        if (err) return(err);
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessMeasurementPreProc));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessMeasurementPreProc == 1) {
        err = getBitsUniDrcInterface(bitstream, 2, &(loudnessNormalizationParameterInterface->loudnessMeasurementPreProc));
        if (err) return(err);
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeDeviceCutOffFrequency));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeDeviceCutOffFrequency == 1) {
        err = getBitsUniDrcInterface(bitstream, 6, &tmp);
        if (err) return(err);
        loudnessNormalizationParameterInterface->deviceCutOffFrequency = max(min(tmp*10,500),20);
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainDbMax));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainDbMax == 1) {
        err = getBitsUniDrcInterface(bitstream, 6, &tmp);
        if (err) return(err);
        if (tmp<63) {
            loudnessNormalizationParameterInterface->loudnessNormalizationGainDbMax = tmp*0.5f;
        } else {
            loudnessNormalizationParameterInterface->loudnessNormalizationGainDbMax = LOUDNESS_NORMALIZATION_GAIN_MAX_DEFAULT;
        }
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainModificationDb));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeLoudnessNormalizationGainModificationDb == 1) {
        err = getBitsUniDrcInterface(bitstream, 10, &tmp);
        if (err) return(err);
        loudnessNormalizationParameterInterface->loudnessNormalizationGainModificationDb = -16+tmp*0.03125f;
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessNormalizationParameterInterface->changeOutputPeakLevelMax));
    if (err) return(err);
    if (loudnessNormalizationParameterInterface->changeOutputPeakLevelMax == 1) {
        err = getBitsUniDrcInterface(bitstream, 6, &tmp);
        if (err) return(err);
        loudnessNormalizationParameterInterface->outputPeakLevelMax = tmp*0.5f;
    }

    return(0);
}

/* parser for dynamicRangeControlInterface */
int
parseDynamicRangeControlInterface(robitbufHandle bitstream,
                                  DynamicRangeControlInterface* dynamicRangeControlInterface)
{
    int err = 0, i = 0, j = 0, tmp = 0;

    err = getBitsUniDrcInterface(bitstream, 1, &(dynamicRangeControlInterface->dynamicRangeControlOn));
    if (err) return(err);

    if (dynamicRangeControlInterface->dynamicRangeControlOn == 1) {
        err = getBitsUniDrcInterface(bitstream, 3, &(dynamicRangeControlInterface->numDrcFeatureRequests));
        if (err) return(err);

        for (i=0; i<dynamicRangeControlInterface->numDrcFeatureRequests; i++) {
            err = getBitsUniDrcInterface(bitstream, 2, &(dynamicRangeControlInterface->drcFeatureRequestType[i]));
            if (err) return(err);

            switch (dynamicRangeControlInterface->drcFeatureRequestType[i]) {
                case 0:
                    err = getBitsUniDrcInterface(bitstream, 4, &(dynamicRangeControlInterface->numDrcEffectTypeRequests[i]));
                    if (err) return(err);
                    err = getBitsUniDrcInterface(bitstream, 4, &(dynamicRangeControlInterface->numDrcEffectTypeRequestsDesired[i]));
                    if (err) return(err);
                    for (j=0; j<dynamicRangeControlInterface->numDrcEffectTypeRequests[i]; j++) {
                        err = getBitsUniDrcInterface(bitstream, 4, &(dynamicRangeControlInterface->drcEffectTypeRequest[i][j]));
                        if (err) return(err);
                    }
                    break;
                case 1:
                    err = getBitsUniDrcInterface(bitstream, 2, &(dynamicRangeControlInterface->dynRangeMeasurementRequestType[i]));
                    if (err) return(err);
                    err = getBitsUniDrcInterface(bitstream, 1, &(dynamicRangeControlInterface->dynRangeRequestedIsRange[i]));
                    if (err) return(err);
                    if (dynamicRangeControlInterface->dynRangeRequestedIsRange[i] == 0) {
                        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
                        if (err) return(err);
                        if (tmp == 0)
                            dynamicRangeControlInterface->dynamicRangeRequestValue[i] = 0.0f;
                        else if(tmp <= 128)
                            dynamicRangeControlInterface->dynamicRangeRequestValue[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            dynamicRangeControlInterface->dynamicRangeRequestValue[i] = 0.5f * tmp - 32.0f;
                        else
                            dynamicRangeControlInterface->dynamicRangeRequestValue[i] = tmp - 134.0f;
                    } else {
                        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
                        if (err) return(err);
                        if (tmp == 0)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] = 0.0f;
                        else if(tmp <= 128)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] = 0.5f * tmp - 32.0f;
                        else
                            dynamicRangeControlInterface->dynamicRangeRequestValueMin[i] = tmp - 134.0f;
                        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
                        if (err) return(err);
                        if (tmp == 0)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] = 0.0f;
                        else if(tmp <= 128)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] = 0.5f * tmp - 32.0f;
                        else
                            dynamicRangeControlInterface->dynamicRangeRequestValueMax[i] = tmp - 134.0f;
                    }
                    break;
                case 2:
                    err = getBitsUniDrcInterface(bitstream, 7, &(dynamicRangeControlInterface->drcCharacteristicRequest[i]));
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

/* parser for dynamicRangeControlParameterInterface */
int
parseDynamicRangeControlParameterInterface(robitbufHandle bitstream,
                                           DynamicRangeControlParameterInterface* dynamicRangeControlParameterInterface)
{
    int err = 0, tmp = 0;

    err = getBitsUniDrcInterface(bitstream, 1, &(dynamicRangeControlParameterInterface->changeCompress));
    if (err) return(err);
    err = getBitsUniDrcInterface(bitstream, 1, &(dynamicRangeControlParameterInterface->changeBoost));
    if (err) return(err);

    if (dynamicRangeControlParameterInterface->changeCompress == 1) {
        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
        if (err) return(err);
        if (tmp<255) {
            dynamicRangeControlParameterInterface->compress = 1 - tmp * 0.00390625f;
        } else {
            dynamicRangeControlParameterInterface->compress = 0.f;
        }
    }

    if (dynamicRangeControlParameterInterface->changeBoost == 1) {
        err = getBitsUniDrcInterface(bitstream, 8, &tmp);
        if (err) return(err);
        if (tmp<255) {
            dynamicRangeControlParameterInterface->boost = 1 - tmp * 0.00390625f;
        } else {
            dynamicRangeControlParameterInterface->boost = 0.f;
        }
    }

    err = getBitsUniDrcInterface(bitstream, 1, &(dynamicRangeControlParameterInterface->changeDrcCharacteristicTarget));
    if (err) return(err);

    if (dynamicRangeControlParameterInterface->changeDrcCharacteristicTarget == 1) {
        err = getBitsUniDrcInterface(bitstream, 8, &(dynamicRangeControlParameterInterface->drcCharacteristicTarget));
        if (err) return(err);
    }
    return(0);
}

#if MPEG_D_DRC_EXTENSION_V1
int
parseLoudnessEqParameterInterface(robitbufHandle bitstream,
                                  LoudnessEqParameterInterface* loudnessEqParameterInterface)
{
    int err = 0;
    int bsSensitivity, bsPlaybackGain;

    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessEqParameterInterface->loudnessEqRequestPresent));
    if (err) return(err);
    if (loudnessEqParameterInterface->loudnessEqRequestPresent) {
        err = getBitsUniDrcInterface(bitstream, 2, &(loudnessEqParameterInterface->loudnessEqRequest));
        if (err) return(err);
    }
    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessEqParameterInterface->sensitivityPresent));
    if (err) return(err);
    if (loudnessEqParameterInterface->sensitivityPresent) {
        err = getBitsUniDrcInterface(bitstream, 7, &bsSensitivity);
        if (err) return(err);
        loudnessEqParameterInterface->sensitivity = bsSensitivity + 23.0f;
    }
    err = getBitsUniDrcInterface(bitstream, 1, &(loudnessEqParameterInterface->playbackGainPresent));
    if (err) return(err);
    if (loudnessEqParameterInterface->playbackGainPresent) {
        err = getBitsUniDrcInterface(bitstream, 7, &bsPlaybackGain);
        if (err) return(err);
        loudnessEqParameterInterface->playbackGain = (float) - bsPlaybackGain;
    }
    return (0);
}
#endif /* MPEG_D_DRC_EXTENSION_V1 */

#ifdef LEVELING_SUPPORT
static int
parseLevelingControlInterface(
    robitbufHandle bitstream,
    LevelingControlInterface *levelingControlInterface,
    int *levelingControlInterfacePresent)
{
    if(levelingControlInterfacePresent) {
        *levelingControlInterfacePresent = 1;
    }

    return
        getBitsUniDrcInterface(
            bitstream,
            1,
            &levelingControlInterface->loudnessLevelingOn);
}
#endif

/* parser for uniDrcInterfaceExtension */
int
parseUniDrcInterfaceExtension(robitbufHandle bitstream,
                              UniDrcInterface* uniDrcInterface,
                              UniDrcInterfaceExtension* uniDrcInterfaceExtension)
{
    int err = 0, i = 0, tmp = 0;
    int uniDrcInterfaceExtType;
    SpecificInterfaceExtension* specificInterfaceExtension;

    err = getBitsUniDrcInterface(bitstream, 4, &uniDrcInterfaceExtType);
    if (err) return(err);

    while (uniDrcInterfaceExtType != UNIDRCINTERFACEEXT_TERM) {
        specificInterfaceExtension = &(uniDrcInterfaceExtension->specificInterfaceExtension[i]);
        specificInterfaceExtension->uniDrcInterfaceExtType = uniDrcInterfaceExtType;
        err = getBitsUniDrcInterface(bitstream, 4, &tmp);
        if (err) return(err);
        specificInterfaceExtension->extSizeBits = tmp+4;

        err = getBitsUniDrcInterface(bitstream, specificInterfaceExtension->extSizeBits, &tmp);
        if (err) return(err);
        specificInterfaceExtension->extBitSize  = tmp+1;

        switch (uniDrcInterfaceExtType) {
#if MPEG_D_DRC_EXTENSION_V1
            case UNIDRCINTERFACEEXT_EQ:
                err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent));
                if (err) return(err);
                if (uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterfacePresent) {
                    err = parseLoudnessEqParameterInterface(bitstream, &(uniDrcInterface->uniDrcInterfaceExtension.loudnessEqParameterInterface));
                    if (err) return(err);
                }
                err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent));
                if (err) return(err);
                if (uniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterfacePresent) {
                    err = getBitsUniDrcInterface(bitstream, 16, &(uniDrcInterface->uniDrcInterfaceExtension.equalizationControlInterface.eqSetPurposeRequest));
                    if (err) return(err);
                }
                break;
#endif /* MPEG_D_DRC_EXTENSION_V1 */

#ifdef LEVELING_SUPPORT
            case UNIDRCINTERFACEEXT_LEVELING:
                err = parseLevelingControlInterface(bitstream, &uniDrcInterfaceExtension->levelingControlInterface, &uniDrcInterfaceExtension->levelingControlInterfacePresent);
                if (err) return(err);
                break;
#endif
                /* add future extensions here */
            default:
                err = getBitsUniDrcInterface(bitstream, specificInterfaceExtension->extBitSize, NULL);
                if (err) return(err);
                break;
        }

        i++;
        if (i==EXT_COUNT_MAX) {
            printf("numInterfaceExtension>%d not supported!\n",EXT_COUNT_MAX);
            return(1);
        }
        err = getBitsUniDrcInterface(bitstream, 4, &uniDrcInterfaceExtType);
        if (err) return(err);
    }
    uniDrcInterfaceExtension->interfaceExtensionCount = i;
    return(0);
}

/* Parser for uniDrcInterface */
int
parseUniDrcInterface(robitbufHandle bitstream,
                     UniDrcInterface* uniDrcInterface)
{
    int err = 0;

    /* uniDrcInterfaceSignature */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->uniDrcInterfaceSignaturePresent));
    if (err) return(err);
    if (uniDrcInterface->uniDrcInterfaceSignaturePresent == 1) {
        err = parseUniDrcInterfaceSignature(bitstream, &(uniDrcInterface->uniDrcInterfaceSignature));
        if (err) return(err);
    }

    /* systemInterface */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->systemInterfacePresent));
    if (err) return(err);
    if (uniDrcInterface->systemInterfacePresent == 1) {
        err = parseSystemInterface(bitstream, &(uniDrcInterface->systemInterface));
        if (err) return(err);
    }

    /* loudnessNormalizationControlInterface */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->loudnessNormalizationControlInterfacePresent));
    if (err) return(err);
    if (uniDrcInterface->loudnessNormalizationControlInterfacePresent == 1) {
        err = parseLoudnessNormalizationControlInterface(bitstream, &(uniDrcInterface->loudnessNormalizationControlInterface));
        if (err) return(err);
    }

    /* loudnessNormalizationParameterInterface */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->loudnessNormalizationParameterInterfacePresent));
    if (err) return(err);
    if (uniDrcInterface->loudnessNormalizationParameterInterfacePresent == 1) {
        err = parseLoudnessNormalizationParameterInterface(bitstream, &(uniDrcInterface->loudnessNormalizationParameterInterface));
        if (err) return(err);
    }

    /* dynamicRangeControlInterface */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->dynamicRangeControlInterfacePresent));
    if (err) return(err);
    if (uniDrcInterface->dynamicRangeControlInterfacePresent == 1) {
        err = parseDynamicRangeControlInterface(bitstream, &(uniDrcInterface->dynamicRangeControlInterface));
        if (err) return(err);
    }

    /* dynamicRangeControlParameterInterface */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->dynamicRangeControlParameterInterfacePresent));
    if (err) return(err);
    if (uniDrcInterface->dynamicRangeControlParameterInterfacePresent == 1) {
        err = parseDynamicRangeControlParameterInterface(bitstream, &(uniDrcInterface->dynamicRangeControlParameterInterface));
        if (err) return(err);
    }

    /* uniDrcInterfaceExtension */
    err = getBitsUniDrcInterface(bitstream, 1, &(uniDrcInterface->uniDrcInterfaceExtensionPresent));
    if (err) return(err);
    if (uniDrcInterface->uniDrcInterfaceExtensionPresent == 1) {
        err = parseUniDrcInterfaceExtension(bitstream, uniDrcInterface, &(uniDrcInterface->uniDrcInterfaceExtension));
        if (err) return(err);
    }

    return(0);
}