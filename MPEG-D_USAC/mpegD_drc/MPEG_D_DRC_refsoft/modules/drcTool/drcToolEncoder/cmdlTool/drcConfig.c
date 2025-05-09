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

#include <stdio.h>
#include "drcToolEncoder.h"
#include "drcConfig.h"

static int
generateChannelLayout (UniDrcConfig* uniDrcConfig)
{
    return (0);
}

static int
generateLoudnessInfo(LoudnessInfoSet* loudnessInfoSet)
{
    return (0);
}

static int
generateLoudnessInfoAlbum(LoudnessInfoSet* loudnessInfoSet)
{
    return (0);
}


static int
generateDownmixInstructions(UniDrcConfig* uniDrcConfig)
{
    return (0);
}

static int
generateDrcInstructions (UniDrcConfig* uniDrcConfig)
{
    int k, n;
    for (n=0; n<uniDrcConfig->drcInstructionsUniDrcCount; n++)
    {
        int profileFound = FALSE;
        
        for (k=0; k<uniDrcConfig->drcCoefficientsUniDrcCount; k++)
        {
            if (uniDrcConfig->drcCoefficientsUniDrc[k].drcLocation == 1)
            {
                profileFound = TRUE;
                break;
            }
        }
#if AMD1_SYNTAX
        if (uniDrcConfig->uniDrcConfigExtPresent && uniDrcConfig->uniDrcConfigExt.parametricDrcPresent && uniDrcConfig->uniDrcConfigExt.drcCoefficientsParametricDrc.drcLocation == 1) {
            profileFound = TRUE;
        }
#endif /* AMD1_SYNTAX */
        if (profileFound == FALSE)
        {
            fprintf(stderr, "ERROR: Cannot match drcInstructionsUniDrc with drcCoefficientsUniDrc.\n");
            return (UNEXPECTED_ERROR);
        }
    }
    return (0);
}

static int
generateDrcCoefficients(UniDrcConfig* uniDrcConfig)
{
    return (0);
}



int
generateUniDrcConfig(UniDrcConfig* encConfig,
                     LoudnessInfoSet* encLoudnessInfoSet,
                     UniDrcGainExt* encGainExtension)
{
    int err = 0;
    
    /* compute any derived data necessary */
    generateChannelLayout(encConfig);
    if (err) return (err);
    err = generateDownmixInstructions(encConfig);
    if (err) return (err);
    err = generateDrcCoefficients(encConfig);
    if (err) return (err);
    err = generateDrcInstructions(encConfig);
    if (err) return (err);
    err = generateLoudnessInfoAlbum(encLoudnessInfoSet);
    if (err) return (err);
    err = generateLoudnessInfo(encLoudnessInfoSet);
    if (err) return (err);
    
    return (0);
}
