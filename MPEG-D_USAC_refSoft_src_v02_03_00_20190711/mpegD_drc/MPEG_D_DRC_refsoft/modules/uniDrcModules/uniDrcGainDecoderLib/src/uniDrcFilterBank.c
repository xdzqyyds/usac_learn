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
#include <stdlib.h>
#include <math.h>
#ifdef COMPILE_FOR_DRC_ENCODER
#include "common.h"
#else
#include "uniDrcCommon.h"
#endif
#include "uniDrc.h"
#include "uniDrcFilterBank.h"

const
FilterBankParams filterBankParams[FILTER_BANK_PARAMETER_COUNT] = {
    {2.0f/1024.0f,	0.0000373252f,	0.9913600345f},
    {3.0f/1024.0f,	0.0000836207f,	0.9870680830f},
    {4.0f/1024.0f,	0.0001480220f,	0.9827947083f},
    {5.0f/1024.0f,	0.0002302960f,	0.9785398263f},
    {6.0f/1024.0f,	0.0003302134f,	0.9743033527f},
    {2.0f/256.0f,	0.0005820761f,	0.9658852897f},
    {3.0f/256.0f,	0.0012877837f,	0.9492662926f},
    {2.0f/128.0f,	0.0022515827f,	0.9329321561f},
    {3.0f/128.0f,	0.0049030350f,	0.9010958535f},
    {2.0f/64.0f,	0.0084426929f,	0.8703307793f},
    {3.0f/64.0f,	0.0178631928f,	0.8118317459f},
    {2.0f/32.0f,	0.0299545822f,	0.7570763753f},
    {3.0f/32.0f,	0.0604985076f,	0.6574551915f},
    {2.0f/16.0f,	0.0976310729f,	0.5690355937f},
    {3.0f/16.0f,	0.1866943331f,	0.4181633458f},
    {2.0f/8.0f,     0.2928932188f,	0.2928932188f},
};

/* compute filter coefficients for a pair of Linkwitz-Riley filters */
int
initLrFilter(const int crossoverFreqIndex,
             FilterCoefficients* lowPassCoeff,
             FilterCoefficients* highPassCoeff)
{
    float gamma = filterBankParams[crossoverFreqIndex].gamma;
    float delta = filterBankParams[crossoverFreqIndex].delta;

    lowPassCoeff->a0 = 1.0f;
    lowPassCoeff->a1 = 2.0f * (gamma - delta);
    lowPassCoeff->a2 = 2.0f * (gamma + delta) - 1.0f;
    lowPassCoeff->b0 = gamma;
    lowPassCoeff->b1 = 2.0f * gamma;
    lowPassCoeff->b2 = gamma;
    
    highPassCoeff->a0 = 1.0f;
    highPassCoeff->a1 = lowPassCoeff->a1;
    highPassCoeff->a2 = lowPassCoeff->a2;
    highPassCoeff->b0 = delta;
    highPassCoeff->b1 = - 2.0f * delta;
    highPassCoeff->b2 = delta;
    
    return (0);
}

int
initAllPassFilter(const int crossoverFreqIndex,
             FilterCoefficients* allPassCoeff)
{
    float gamma = filterBankParams[crossoverFreqIndex].gamma;
    float delta = filterBankParams[crossoverFreqIndex].delta;
    
    allPassCoeff->a0 = 1.0f;
    allPassCoeff->a1 = 2.0f * (gamma - delta);
    allPassCoeff->a2 = 2.0f * (gamma + delta) - 1.0f;
    allPassCoeff->b0 = allPassCoeff->a2;
    allPassCoeff->b1 = allPassCoeff->a1;
    allPassCoeff->b2 = 1.0f;
    
    return (0);
}

/* compute all the filter coefficients */
int
initFilterBank(const int nBands,
               const GainParams* gainParams,
               DrcFilterBank* drcFilterBank)
{
    TwoBandBank* twoBandBank;
    ThreeBandBank* threeBandBank;
    FourBandBank* fourBandBank;
    drcFilterBank->complexity = 0;
    drcFilterBank->nBands = nBands;
    switch (nBands) {
        case 1:
            break;
        case 2:
            drcFilterBank->twoBandBank = (TwoBandBank*) calloc(1, sizeof(TwoBandBank));
            twoBandBank = drcFilterBank->twoBandBank;
            initLrFilter(gainParams[1].crossoverFreqIndex,
                         &(twoBandBank->lowPass),
                         &(twoBandBank->highPass));
            drcFilterBank->complexity = 8;
            break;
        case 3:
            drcFilterBank->threeBandBank = (ThreeBandBank*) calloc(1, sizeof(ThreeBandBank));
            threeBandBank = drcFilterBank->threeBandBank;
            initLrFilter(gainParams[1].crossoverFreqIndex,
                         &(threeBandBank->lowPassStage2),
                         &(threeBandBank->highPassStage2));
            
            initLrFilter(gainParams[2].crossoverFreqIndex,
                         &(threeBandBank->lowPassStage1),
                         &(threeBandBank->highPassStage1));
            
            initAllPassFilter(gainParams[1].crossoverFreqIndex,
                              &(threeBandBank->allPassStage2));
            drcFilterBank->complexity = 18;
            break;
        case 4:
            drcFilterBank->fourBandBank = (FourBandBank*) calloc(1, sizeof(FourBandBank));
            fourBandBank = drcFilterBank->fourBandBank;
            initLrFilter(gainParams[2].crossoverFreqIndex,
                         &(fourBandBank->lowPassStage1),
                         &(fourBandBank->highPassStage1));
            
            initAllPassFilter(gainParams[3].crossoverFreqIndex,
                              &(fourBandBank->allPassStage2Low));
            
            initAllPassFilter(gainParams[1].crossoverFreqIndex,
                              &(fourBandBank->allPassStage2High));
            
            initLrFilter(gainParams[1].crossoverFreqIndex,
                         &(fourBandBank->lowPassStage3Low),
                         &(fourBandBank->highPassStage3Low));
            
            initLrFilter(gainParams[3].crossoverFreqIndex,
                         &(fourBandBank->lowPassStage3High),
                         &(fourBandBank->highPassStage3High));
            drcFilterBank->complexity = 28;
            break;
            
        default:
            fprintf(stderr, "ERROR: unsupported number of DRC bands: %d\n", nBands);
            return (PARAM_ERROR);
            break;
    }
    return (0);
}

int
initAllPassCascade(const int filterCount,
                   const int* crossoverFreqIndex,
                   DrcFilterBank* drcFilterBank)
{
    int n;
    
    if (filterCount > 0)
    {
        drcFilterBank->allPassCascade = (AllPassCascade*) calloc(1, sizeof(AllPassCascade));

        for (n=0; n<filterCount; n++)
        {
            initAllPassFilter(crossoverFreqIndex[n],
                              &(drcFilterBank->allPassCascade->allPassCascadeFilter[n].allPassStage));
            drcFilterBank->complexity += 2;
        }
        drcFilterBank->allPassCascade->filterCount = filterCount;
    }
    else
    {
        drcFilterBank->allPassCascade = NULL;
    }
    
    return (0);
}


int
initAllFilterBanks(const DrcCoefficientsUniDrc* drcCoefficientsUniDrc,
                   const DrcInstructionsUniDrc* drcInstructionsUniDrc,
                   FilterBanks* filterBanks)
{
    int b, g, i, k, m, s, err, crossoverFreqIndex, nChannelsInGroups, channelsWithoutDrcPresent, nPhaseAlignmentChannelGroups;
    int found = FALSE;
    int cascadeCrossoverIndices[CHANNEL_GROUP_COUNT_MAX+1][CHANNEL_GROUP_COUNT_MAX * 3];
    int count[CHANNEL_GROUP_COUNT_MAX + 1];
    
    nChannelsInGroups = 0;
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
    {
        nChannelsInGroups += drcInstructionsUniDrc->nChannelsPerChannelGroup[g];
    }
    nPhaseAlignmentChannelGroups = drcInstructionsUniDrc->nDrcChannelGroups;
    channelsWithoutDrcPresent = FALSE;
#ifdef COMPILE_FOR_DRC_ENCODER
    if (nChannelsInGroups < drcInstructionsUniDrc->drcChannelCount)
#else
    if (nChannelsInGroups < drcInstructionsUniDrc->audioChannelCount)
#endif
    {
        nPhaseAlignmentChannelGroups++;
        channelsWithoutDrcPresent = TRUE;
    }
    
    filterBanks->drcFilterBank = (DrcFilterBank*) calloc(nPhaseAlignmentChannelGroups, sizeof(DrcFilterBank));
    filterBanks->nFilterBanks = drcInstructionsUniDrc->nDrcChannelGroups;
    filterBanks->nPhaseAlignmentChannelGroups = nPhaseAlignmentChannelGroups;
    if (drcCoefficientsUniDrc == NULL)
    {
        filterBanks->drcFilterBank->nBands = 1;
    }
    else
    {
        for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
        {
#ifdef AMD2_COR1
            if (drcInstructionsUniDrc->gainSetIndexForChannelGroup[g] >= 0) {
#endif
                err = initFilterBank(drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].bandCount,
                                     drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].gainParams,
                                     &(filterBanks->drcFilterBank[g]));
                if (err) return (err);
            }
        }
#ifdef AMD2_COR1
    }
#endif
    
    /* Init all-pass cascade for phase alignment of channel groups */
    for (g=0; g<CHANNEL_GROUP_COUNT_MAX + 1; g++)
    {
        count[g] = 0;
    }
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
    {
#ifdef AMD2_COR1
        if (drcInstructionsUniDrc->gainSetIndexForChannelGroup[g] >= 0) {
#endif
            /* collect all crossover frequency indices for each channelGroup that are used outside that channel group */
            for (b=1; b<drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].bandCount; b++)
            {
                crossoverFreqIndex = drcCoefficientsUniDrc->gainSetParams[drcInstructionsUniDrc->gainSetIndexForChannelGroup[g]].gainParams[b].crossoverFreqIndex;
                for (k=0; k<nPhaseAlignmentChannelGroups; k++)
                {
                    if (k!=g)
                    {
                        cascadeCrossoverIndices[k][count[k]] = crossoverFreqIndex;
                        count[k]++;
                        if (count[k] > CHANNEL_GROUP_COUNT_MAX * 3)
                        {
                            fprintf(stderr, "ERROR: array size for crossover frequencies of Linkwitz-Riley filter exceeded\n");
                            return (UNEXPECTED_ERROR);
                        }
                    }
                }
            }
        }
#ifdef AMD2_COR1
    }
#endif
    
    i=0;
    while (i<count[0])
    {
        crossoverFreqIndex = cascadeCrossoverIndices[0][i];
        found = FALSE;
        for (g=1; g<nPhaseAlignmentChannelGroups; g++)
        {
            found = FALSE;
            for (k=0; k<count[g]; k++)
            {
                if (cascadeCrossoverIndices[g][k] == crossoverFreqIndex)
                {
                    found = TRUE;
                    break;
                }
            }
            if (found == FALSE) break;
        }
        if (found == TRUE)
        {
            /* eliminate common crossover indices */
            for (g=0; g<nPhaseAlignmentChannelGroups; g++)
            {
                for (m=0; m<count[g]; m++)
                {
                    if (cascadeCrossoverIndices[g][m] == crossoverFreqIndex)
                    {
                        for (s=m+1; s<count[g]; s++)
                        {
                            cascadeCrossoverIndices[g][s-1] = cascadeCrossoverIndices[g][s];
                        }
                        count[g]--;
                        break;
                    }
                }
            }
            i = 0;
        }
        else
        {
            i++;
        }
    }
    for (g=0; g<nPhaseAlignmentChannelGroups; g++)
    {
        err = initAllPassCascade(count[g], cascadeCrossoverIndices[g], &(filterBanks->drcFilterBank[g]));
        if (err) return (err);
    }
    filterBanks->complexity = 0;
    for (g=0; g<drcInstructionsUniDrc->nDrcChannelGroups; g++)
    {
        filterBanks->complexity += drcInstructionsUniDrc->nChannelsPerChannelGroup[g] * filterBanks->drcFilterBank[g].complexity;
    }
    return (0);
}

int
removeAllFilterBanks(FilterBanks* filterBanks)
{
    int n;
    if (filterBanks->drcFilterBank)
    {
        for (n=0; n<filterBanks->nFilterBanks; n++)
        {
            if (filterBanks->drcFilterBank[n].twoBandBank)
            {
                free(filterBanks->drcFilterBank[n].twoBandBank);
                filterBanks->drcFilterBank[n].twoBandBank = NULL;
            }
            if (filterBanks->drcFilterBank[n].threeBandBank)
            {
                free(filterBanks->drcFilterBank[n].threeBandBank);
                filterBanks->drcFilterBank[n].threeBandBank = NULL;
            }
            if (filterBanks->drcFilterBank[n].fourBandBank)
            {
                free(filterBanks->drcFilterBank[n].fourBandBank);
                filterBanks->drcFilterBank[n].fourBandBank = NULL;
            }
        }
        for (n=0; n<filterBanks->nPhaseAlignmentChannelGroups; n++)
        {
            if (filterBanks->drcFilterBank[n].allPassCascade)
            {
                free(filterBanks->drcFilterBank[n].allPassCascade);
                filterBanks->drcFilterBank[n].allPassCascade =  NULL;
            }
        }
        free(filterBanks->drcFilterBank);
        filterBanks->drcFilterBank = NULL;
    }
    return(0);
}


int
runLrFilter (const FilterCoefficients* filterCoefficients,
             LrFilterState* lrFilterState,
             const int size,
             const float* audioIn,
             float* audioOut)
{
    int n;
    float y;
    const float* x = audioIn;
    float* z = audioOut;

    float a1 = filterCoefficients->a1;
    float a2 = filterCoefficients->a2;
    float b0 = filterCoefficients->b0;
    float b1 = filterCoefficients->b1;
    float b2 = filterCoefficients->b2;

    float s00 = lrFilterState->s00;
    float s01 = lrFilterState->s01;
    float s10 = lrFilterState->s10;
    float s11 = lrFilterState->s11;
    
    /* a0 must be 1.0 */
    if (filterCoefficients->a0 != 1.0f)
    {
        fprintf(stderr, "ERROR: filter coefficient a0 must be 1.0. Found %f\n", filterCoefficients->a0);
        return (UNEXPECTED_ERROR);
    }

    for (n=0; n<size; n++)
    {
        y = b0 * x[n] + s00;
        s00 = b1 * x[n] - a1 * y + s01;
        s01 = b2 * x[n] - a2 * y;
        
        z[n] = b0 * y + s10;
        s10 = b1 * y - a1 * z[n] + s11;
        s11 = b2 * y - a2 * z[n];
    }
    lrFilterState->s00 = s00;
    lrFilterState->s01 = s01;
    lrFilterState->s10 = s10;
    lrFilterState->s11 = s11;
    return (0);

}


int
runAllPassFilter (const FilterCoefficients* filterCoefficients,
                  AllPassFilterState* allPassFilterState,
                  const int size,
                  const float* audioIn,
                  float* audioOut)
{
    int n;
    float xTmp;
    const float* x = audioIn;
    float* y = audioOut;
    
    float a1 = filterCoefficients->a1;
    float a2 = filterCoefficients->a2;
    float b0 = filterCoefficients->b0;
    float b1 = filterCoefficients->b1;
    float b2 = filterCoefficients->b2;
    
    float s0 = allPassFilterState->s0;
    float s1 = allPassFilterState->s1;
    
    /* a0 must be 1.0 */
    if (filterCoefficients->a0 != 1.0f)
    {
        fprintf(stderr, "ERROR: filter coefficient a0 must be 1.0. Found %f\n", filterCoefficients->a0);
        return (UNEXPECTED_ERROR);
    }

    for (n=0; n<size; n++)
    {
        xTmp = x[n];               /* needed for in-place filtering*/
        y[n] = b0 * xTmp + s0;
        s0 = b1 * xTmp - a1 * y[n] + s1;
        s1 = b2 * xTmp - a2 * y[n];
    }
    allPassFilterState->s0 = s0;
    allPassFilterState->s1 = s1;
    
    return (0);
}

int
runTwoBandBank(TwoBandBank* twoBandBank,
               const int c,
               const int size,
               const float* audioIn,
               float* audioOut[])
{
    int err = 0;
    err = runLrFilter (&(twoBandBank->lowPass),
                       &(twoBandBank->lowPassState[c]),
                       size,
                       audioIn,
                       audioOut[0]);
    
    if (err) return (err);
    err = runLrFilter (&(twoBandBank->highPass),
                       &(twoBandBank->highPassState[c]),
                       size,
                       audioIn,
                       audioOut[1]);
    if (err) return (err);
    return (0);
}

int
runThreeBandBank(ThreeBandBank* threeBandBank,
                 const int c,
                 const int size,
                 const float* audioIn,
                 float* audioOut[])
{
    float* buf;
    int err = 0;
    
    buf = audioOut[2];
    err = runLrFilter (&(threeBandBank->lowPassStage1),
                       &(threeBandBank->lowPassStage1State[c]),
                       size,
                       audioIn,
                       buf);
    if (err) return (err);
    
    err = runLrFilter (&(threeBandBank->lowPassStage2),
                       &(threeBandBank->lowPassStage2State[c]),
                       size,
                       buf,
                       audioOut[0]);
    if (err) return (err);
    
    err = runLrFilter (&(threeBandBank->highPassStage2),
                       &(threeBandBank->highPassStage2State[c]),
                       size,
                       buf,
                       audioOut[1]);
    if (err) return (err);
    
    
    err = runLrFilter (&(threeBandBank->highPassStage1),
                       &(threeBandBank->highPassStage1State[c]),
                       size,
                       audioIn,
                       buf);
    if (err) return (err);
    err = runAllPassFilter (&(threeBandBank->allPassStage2),
                            &(threeBandBank->allPassStage2State[c]),
                            size,
                            buf,
                            audioOut[2]);
    if (err) return (err);
    
    return (0);
}

int
runFourBandBank(FourBandBank* fourBandBank,
                const int c,
                const int size,
                const float* audioIn,
                float* audioOut[])
{
    float* buf;
    int err = 0;
    
    buf = audioOut[3];
    err = runLrFilter (&(fourBandBank->lowPassStage1),
                       &(fourBandBank->lowPassStage1State[c]),
                       size,
                       audioIn,
                       buf);
    if (err) return (err);
    err = runAllPassFilter (&(fourBandBank->allPassStage2Low),
                            &(fourBandBank->allPassStage2LowState[c]),
                            size,
                            buf,
                            buf);
    if (err) return (err);
    err = runLrFilter (&(fourBandBank->lowPassStage3Low),
                       &(fourBandBank->lowPassStage3LowState[c]),
                       size,
                       buf,
                       audioOut[0]);
    if (err) return (err);
    err = runLrFilter (&(fourBandBank->highPassStage3Low),
                       &(fourBandBank->highPassStage3LowState[c]),
                       size,
                       buf,
                       audioOut[1]);
    if (err) return (err);
    
    
    
    err = runLrFilter (&(fourBandBank->highPassStage1),
                       &(fourBandBank->highPassStage1State[c]),
                       size,
                       audioIn,
                       buf);
    if (err) return (err);
    err = runAllPassFilter (&(fourBandBank->allPassStage2High),
                            &(fourBandBank->allPassStage2HighState[c]),
                            size,
                            buf,
                            buf);
    if (err) return (err);
    err = runLrFilter (&(fourBandBank->lowPassStage3High),
                       &(fourBandBank->lowPassStage3HighState[c]),
                       size,
                       buf,
                       audioOut[2]);
    if (err) return (err);
    err = runLrFilter (&(fourBandBank->highPassStage3High),
                       &(fourBandBank->highPassStage3HighState[c]),
                       size,
                       buf,
                       audioOut[3]);
    if (err) return (err);
    
    return (0);
}

int
runAllPassCascade(AllPassCascade *allPassCascade,
                  const int c,
                  const int size,
                  float* audioIo) /* result in place */
{
    int n;
    int err = 0;
    float* buf = audioIo;

    for (n=0; n<allPassCascade->filterCount; n++)
    {
        err = runAllPassFilter (&(allPassCascade->allPassCascadeFilter[n].allPassStage),
                                &(allPassCascade->allPassCascadeFilter[n].allPassStageState[c]),
                                size,
                                buf,
                                buf);
        if (err) return (err);
    }
    
    return (err);
}
