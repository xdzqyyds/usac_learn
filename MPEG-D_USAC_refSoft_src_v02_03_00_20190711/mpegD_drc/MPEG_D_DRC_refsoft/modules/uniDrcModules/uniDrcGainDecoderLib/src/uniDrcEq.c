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
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef COMPILE_FOR_DRC_ENCODER
#include "common.h"
#include "uniDrc.h"
#else
#include "uniDrcCommon.h"
#include "uniDrc.h"
#include "uniDrcGainDecoder.h"
#include "uniDrcGainDecoder_api.h"
#include "uniDrcGainDec.h"
#endif

#include "uniDrcEq.h"

#if AMD1_SYNTAX || MPEG_D_DRC_EXTENSION_V1

#define CONFIG_REAL_POLE                    0
#define CONFIG_COMPLEX_POLE                 1
#define CONFIG_REAL_ZERO_RADIUS_ONE         2
#define CONFIG_REAL_ZERO                    3
#define CONFIG_GENERIC_ZERO                 4

#define STEP_RATIO_F_LO                     20.0f
#define STEP_RATIO_F_HI                     24000.0f
#define STEP_RATIO_EQ_NODE_COUNT_MAX        33

#define FILTER_ELEMENT_FORMAT_POLE_ZERO     0
#define FILTER_ELEMENT_FORMAT_FIR           1

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

int
createEqSet(EqSet** hEqSet)
{
    EqSet* eqSet = NULL;
    removeEqSet(hEqSet);
    
    eqSet = (EqSet*) calloc (1, sizeof(EqSet));
    *hEqSet = eqSet;
    
    return (0);
}

int
removeEqSet(EqSet** hEqSet)
{
    if (*hEqSet)
    {
        free(*hEqSet);
        *hEqSet = NULL;
    }
    return (0);
}

int
deriveSubbandCenterFreq(const int eqSubbandGainCount,
                        const int eqSubbandGainFormat,
                        const float audioSampleRate,
                        float* subbandCenterFreq)
{
    int i;
    float width, offset;
    switch (eqSubbandGainFormat) {
        case GAINFORMAT_QMF32:
        case GAINFORMAT_QMF64:
        case GAINFORMAT_QMF128:
        case GAINFORMAT_UNIFORM:
            width = 0.5f * audioSampleRate / (float) eqSubbandGainCount;
            offset = 0.5f * width;
            for (i=0; i<eqSubbandGainCount; i++)
            {
                subbandCenterFreq[i] = i * width + offset;
            }
            break;
        case GAINFORMAT_QMFHYBRID39:
        case GAINFORMAT_QMFHYBRID71:
        case GAINFORMAT_QMFHYBRID135:
            printf("ERROR: Subband gains for hybrid filter bank not implemented\n");
            return(UNEXPECTED_ERROR);
            break;
        default:
            break;
    }
    return (0);
}

int
deriveZeroResponse(const float radius,
                   const float angleRadian,
                   const float frequencyRadian,
                   float *response)
{
    /* magnitude squared response */
    *response = 1.0f + radius * radius - 2.0f * radius * cos(frequencyRadian - angleRadian);
    return (0);
}


int
derivePoleResponse(const float radius,
                   const float angleRadian,
                   const float frequencyRadian,
                   float *response)
{
    int err = 0;
    err = deriveZeroResponse(radius, angleRadian, frequencyRadian, response);
    /* magnitude squared response */
    *response = 1.0f / *response;
    return (err);
}

int
deriveFirFilterResponse(const int firOrder,
                        const int firSymmetry,
                        const float* coeff,
                        const float frequencyRadian,
                        float *response)
{
    int err = 0, m;
    float sum = 0.0f;
    int o2;
    
    if ((firOrder & 0x1) == 0) /* even */
    {
        o2 = firOrder/2;
        if (firSymmetry == 0) /* even */
        {
            for (m=1; m<=o2; m++)
            {
                sum += coeff[o2-m] * cos (m * frequencyRadian);
            }
            sum *= 2.0f;
            sum = coeff[o2];
        }
        else /* odd */
        {
            for (m=1; m<=o2; m++)
            {
                sum += coeff[o2-m] * sin (m * frequencyRadian);
            }
            sum *= 2.0f;
        }
    }
    else /* odd */
    {
        o2 = (firOrder+1)/2;
        if (firSymmetry == 0)
        {
            for (m=1; m<=o2; m++)
            {
                sum += coeff[o2-m] * cos ((m - 0.5f) * frequencyRadian);
            }
        }
        else
        {
            for (m=1; m<=o2; m++)
            {
                sum += coeff[o2-m] * sin ((m - 0.5f) * frequencyRadian);
            }
        }
        sum *= 2.0f;
    }
    /* magnitude response */
    *response = sum;
    return (err);
}

int
deriveFilterElementResponse(UniqueTdFilterElement* element,
                            const float frequencyRadian,
                            float *response)
{
    int err, i;
    float responsePart, radius, angleRadian;
    double combinedResponse = 1.0;
    
    if (element->eqFilterFormat == FILTER_ELEMENT_FORMAT_POLE_ZERO)
    {
        for (i=0; i<element->realZeroRadiusOneCount; i++)
        {
            err = deriveZeroResponse(1.0f, M_PI * (float) element->zeroSign[i], frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
        }
        for (i=0; i<element->realZeroCount; i++)
        {
            if (element->realZeroRadius[i] < 0.0f)
            {
                radius = - element->realZeroRadius[i];
                angleRadian = M_PI;
            }
            else
            {
                radius = element->realZeroRadius[i];
                angleRadian = 0.0f;
            }
            err = deriveZeroResponse(radius, angleRadian, frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
            err = deriveZeroResponse(1.0f / radius, angleRadian, frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
        }
        
        combinedResponse = sqrt(combinedResponse);
        
        for (i=0; i<element->genericZeroCount; i++)
        {
            radius = element->genericZeroRadius[i];
            err = deriveZeroResponse(radius, M_PI * element->genericZeroAngle[i], frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
            err = deriveZeroResponse(1.0f / radius, M_PI * element->genericZeroAngle[i], frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
        }
        for (i=0; i<element->realPoleCount; i++)
        {
            if (element->realPoleRadius[i] < 0.0f)
            {
                radius = - element->realPoleRadius[i];
                angleRadian = - M_PI;
            }
            else
            {
                radius = element->realPoleRadius[i];
                angleRadian = 0.0f;
            }
            err = derivePoleResponse(radius, angleRadian, frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart;
        }
        for (i=0; i<element->complexPoleCount; i++)
        {
            err = derivePoleResponse(element->realPoleRadius[i], M_PI * element->complexPoleAngle[i], frequencyRadian, &responsePart);
            if (err) return (err);
            combinedResponse *= responsePart * responsePart;
         }
    }
    else
    {
        err = deriveFirFilterResponse(element->firFilterOrder,
                                      element->firSymmetry,
                                      element->firCoefficient,
                                      frequencyRadian,
                                      &responsePart);
        if (err) return (err);
        combinedResponse *= responsePart;
    }
    *response = combinedResponse;
    return (0);
}

int
deriveFilterBlockResponse(UniqueTdFilterElement* uniqueTdFilterElement,
                          FilterBlock* filterBlock,
                          const float frequencyRadian,
                          float *response)
{
    int i, err;
    float responsePart;
    double combinedResponse = 1.0;
    for (i=0; i<filterBlock->filterElementCount; i++)
    {
        FilterElement* filterElement = &filterBlock->filterElement[i];
        err = deriveFilterElementResponse(&(uniqueTdFilterElement[filterElement->filterElementIndex]),
                                           frequencyRadian,
                                           &responsePart);
        combinedResponse *= responsePart;
        if (err) return(err);
        if (filterElement->filterElementGainPresent == 1)
        {
            combinedResponse *= pow(10.0f, 0.05f * filterElement->filterElementGain);
        }
    }
    *response = (float) combinedResponse;
    return (0);
}

int
deriveSubbandGainsFromTdCascade(UniqueTdFilterElement* uniqueTdFilterElement,
                                FilterBlock* filterBlock,
                                TdFilterCascade* tdFilterCascade,
                                const int eqSubbandGainFormat,
                                const int eqChannelGroupCount,
                                const int* subbandGainsIndex,
                                const float audioSampleRate,
                                const int eqFrameSizeSubband,
                                SubbandFilter* subbandFilter)
{
    int i, err, g, b;
    float responsePart, frequencyRadian;
    float subbandCenterFreq[256];
    double combinedResponse;

    int eqSubbandGainCount = subbandFilter->coeffCount;
   
    err = deriveSubbandCenterFreq(eqSubbandGainCount, eqSubbandGainFormat, audioSampleRate, subbandCenterFreq);
    if (err) return(err);
    
    for (g=0; g<eqChannelGroupCount; g++)
    {
        for (b=0; b<eqSubbandGainCount; b++)
        {
            combinedResponse = pow(10.0f, 0.05f * tdFilterCascade->eqCascadeGain[g]);
            frequencyRadian = 2.0f * M_PI * subbandCenterFreq[b] / audioSampleRate;
            for (i=0; i<tdFilterCascade->filterBlockRefs[g].filterBlockCount; i++)
            {
                err = deriveFilterBlockResponse(uniqueTdFilterElement,
                                                &(filterBlock[tdFilterCascade->filterBlockRefs[g].filterBlockIndex[i]]),
                                                frequencyRadian,
                                                &responsePart);
                if (err) return(err);
                combinedResponse *= responsePart;
            }
            subbandFilter[g].subbandCoeff[b] = (float) combinedResponse;
        }
        subbandFilter[g].eqFrameSizeSubband = eqFrameSizeSubband;
    }
    return(0);
}

/* if c1 is present, then add c2 if c2 is not present */
int
checkPresenceAndAddCascade(CascadeAlignmentGroup* cascadeAlignmentGroup,
                           const int c1,
                           const int c2,
                           int* done)
{
    int m, n;
    
    *done = 0;
    for (m=0; m<cascadeAlignmentGroup->memberCount; m++)
    {
        if (cascadeAlignmentGroup->memberIndex[m] == c1)
        {
            for (n=0; n<cascadeAlignmentGroup->memberCount; n++)
            {
                if (cascadeAlignmentGroup->memberIndex[n] == c2)
                {
                    *done = 1;  /* c2 is already in group */
                }
            }
            if (*done == 0)
            {   /* add c2 to group */
                cascadeAlignmentGroup->memberIndex[cascadeAlignmentGroup->memberCount] = c2;
                cascadeAlignmentGroup->memberCount++;
                *done = 1;
            }
        }
    }
    return (0);
}

int
deriveCascadeAlignmentGroups (const int eqChannelGroupCount,
                              const int eqPhaseAlignmentPresent,
                              const int eqPhaseAlignment[][EQ_CHANNEL_GROUP_COUNT_MAX],
                              int* cascadeAlignmentGroupCount,
                              CascadeAlignmentGroup* cascadeAlignmentGroup)
{
    int i, k, g, err, groupCount, done;
    
    groupCount = 0;
    
    if (eqPhaseAlignmentPresent == 0)
    {
        if (eqChannelGroupCount > 1)
        {
            for (i=0; i<eqChannelGroupCount; i++)
            {
                cascadeAlignmentGroup[groupCount].memberIndex[i] = i;
            }
            cascadeAlignmentGroup[groupCount].memberCount = eqChannelGroupCount;
            groupCount = 1;
        }
    }
    else
    {
        for (i=0; i<eqChannelGroupCount; i++)
        {
            for (k=i+1; k<eqChannelGroupCount; k++)
            {
                if (eqPhaseAlignment[i][k] == 1)
                {
                    done = 0;
                    for (g=0; g<groupCount; g++)
                    {
                        err = checkPresenceAndAddCascade(&cascadeAlignmentGroup[g], i, k, &done);
                        if (err) return (err);
                        if (done == 0)
                        {
                            err = checkPresenceAndAddCascade(&cascadeAlignmentGroup[g], k, i, &done);
                            if (err) return (err);
                        }
                    }
                    if (done == 0)
                    {   /* create a new alignment group */
                        cascadeAlignmentGroup[groupCount].memberIndex[0] = i;
                        cascadeAlignmentGroup[groupCount].memberIndex[1] = k;
                        cascadeAlignmentGroup[groupCount].memberCount = 2;
                        groupCount++;
                    }
                }
            }
        }
    }
    *cascadeAlignmentGroupCount = groupCount;
    return (0);
}

int
deriveAllpassChain(FilterCascadeTDomain* filterCascadeTDomain,
                   AllpassChain* allpassChain)
{
    int k, b;
    int allpassCount = 0;
    
    for (b=0; b<filterCascadeTDomain->blockCount; b++)
    {
        /* consider only the filter element with index 0 for phase alignment */
        EqFilterElement* eqFilterElement = &filterCascadeTDomain->eqFilterBlock[b].eqFilterElement[0];
        
        if (filterCascadeTDomain->eqFilterBlock[b].matchingPhaseFilterElement0.isValid == 1)
        {
            allpassChain->matchingPhaseFilter[allpassCount] = filterCascadeTDomain->eqFilterBlock[b].matchingPhaseFilterElement0;
            allpassCount++;
        }
        else
        {
            fprintf(stderr,"ERROR: attempt to add invalid phase alignment filter. (Perhaps caused by FIR element?)\n");
            return (UNEXPECTED_ERROR);
        }
        
        for (k = 0; k<eqFilterElement->phaseAlignmentFilterCount; k++)
        {
            if (eqFilterElement->phaseAlignmentFilter[k].isValid == 1)
            {
                allpassChain->matchingPhaseFilter[allpassCount] = eqFilterElement->phaseAlignmentFilter[k];
                allpassCount++;
            }
            else
            {
                fprintf(stderr,"ERROR: attempt to add invalid phase alignment filter. (Perhaps caused by FIR element?)\n");
                return (UNEXPECTED_ERROR);
            }
        }
    }
    allpassChain->allpassCount = allpassCount;
    return (0);
}

int
addAllpassFilterChain(AllpassChain* allpassChain,
                      FilterCascadeTDomain* filterCascadeTDomain)
{
    int k;
    
    for (k=0; k<allpassChain->allpassCount; k++)
    {
        filterCascadeTDomain->phaseAlignmentFilter[filterCascadeTDomain->phaseAlignmentFilterCount + k] = allpassChain->matchingPhaseFilter[k];
    }
    filterCascadeTDomain->phaseAlignmentFilterCount += allpassChain->allpassCount;

    return (0);
}

int
phaseAlignCascadeGroup(const int cascadeAlignmentGroupCount,
                       CascadeAlignmentGroup cascadeAlignmentGroup[],
                       FilterCascadeTDomain filterCascadeTDomain[])
{
    int g, k, m, cascadeIndex, err;
    AllpassChain allpassChain[EQ_CHANNEL_GROUP_COUNT_MAX];
    
    for (g=0; g<cascadeAlignmentGroupCount; g++)
    {
        /* Generate matching phase filter chain for each cascade */
        for (k=0; k<cascadeAlignmentGroup[g].memberCount; k++)
        {
            cascadeIndex = cascadeAlignmentGroup[g].memberIndex[k];
            
            err = deriveAllpassChain(&filterCascadeTDomain[cascadeIndex],
                                     &allpassChain[k]);
            if (err) return (err);
            allpassChain[k].matchesCascadeIndex = cascadeIndex;
        }
        /* TODO: Remove phase alignment filters from cascade that are identical in all grouped cascades */
        
        /* add matching filter chains to the other cascades */
        for (k=0; k<cascadeAlignmentGroup[g].memberCount; k++)
        {
            cascadeIndex = cascadeAlignmentGroup[g].memberIndex[k];
            for (m=0; m<cascadeAlignmentGroup[g].memberCount; m++)
            {
                if (cascadeIndex != allpassChain[m].matchesCascadeIndex)
                {
                    addAllpassFilterChain(&allpassChain[m],
                                          &filterCascadeTDomain[cascadeIndex]);
                }
            }
        }
    }
    return(0);
}

int
deriveMatchingPhaseFilterParams(const int config,
                                float radius,
                                float angle,
                                PhaseAlignmentFilter* phaseAlignmentFilter)
{
    int channel;
    float zReal, zImag;
    float prod;
    int section = phaseAlignmentFilter->sectionCount;
    FilterSection* filterSection = &phaseAlignmentFilter->filterSection[section];
    switch (config) {
        case CONFIG_REAL_POLE:
            phaseAlignmentFilter->gain *= (-radius);
            filterSection->a1 = - radius;
            filterSection->a2 = 0.0f;
            filterSection->b1 = - 1.0f / radius;
            filterSection->b2 = 0.0f;
            phaseAlignmentFilter->sectionCount++;
            break;
        case CONFIG_COMPLEX_POLE:
            zReal = radius * cos(M_PI * angle);
            zImag = radius * sin(M_PI * angle);
            prod = zReal * zReal + zImag * zImag;
            phaseAlignmentFilter->gain *= prod;
            filterSection->a1 = - 2.0f * zReal;
            filterSection->a2 = prod;
            filterSection->b1 = - 2.0f * zReal / prod;
            filterSection->b2 = 1.0f / prod;
            phaseAlignmentFilter->sectionCount++;
            break;
        default:
            break;
    }
    for (channel=0; channel<EQ_CHANNEL_COUNT_MAX; channel++)
    {
        filterSection->filterSectionState[channel].stateIn1 = 0.0f;
        filterSection->filterSectionState[channel].stateIn2 = 0.0f;
        filterSection->filterSectionState[channel].stateOut1 = 0.0f;
        filterSection->filterSectionState[channel].stateOut2 = 0.0f;
    }
    return(0);
}

int
deriveMatchingPhaseFilterDelay(UniqueTdFilterElement* element,
                               PhaseAlignmentFilter* phaseAlignmentFilter)
{
    int i, delay=0, channel;
    if (element->eqFilterFormat == FILTER_ELEMENT_FORMAT_POLE_ZERO) {
        if (element->realZeroRadiusOneCount == 0)
        {
            delay = element->realZeroCount + 2 * element->genericZeroCount - element->realPoleCount - 2 * element->complexPoleCount;
            delay = max(0, delay);
            phaseAlignmentFilter->isValid = 1;
        }
    }
    phaseAlignmentFilter->audioDelay.delay = delay;
    for (channel=0; channel<EQ_CHANNEL_COUNT_MAX; channel++)
    {
        for (i=0; i<delay; i++)
        {
            phaseAlignmentFilter->audioDelay.state[channel][i] = 0.0f;
        }
    }

    return (0);
}

void
prepareMatchingPhaseFilter (MatchingPhaseFilter* matchingPhaseFilter)
{
    memset(matchingPhaseFilter, 0, sizeof(MatchingPhaseFilter));
    matchingPhaseFilter->gain = 1.0f;
}

int
deriveMatchingPhaseFilter(UniqueTdFilterElement* element,
                          int filterElementIndex,
                          MatchingPhaseFilter* matchingPhaseFilter)
{
    int i, err;
    
    prepareMatchingPhaseFilter(matchingPhaseFilter);
    
    if (element->eqFilterFormat == FILTER_ELEMENT_FORMAT_POLE_ZERO)
    {
        for (i=0; i<element->realPoleCount; i++)
        {
            err = deriveMatchingPhaseFilterParams(CONFIG_REAL_POLE,
                                                  element->realPoleRadius[i],
                                                  0.0f,
                                                  matchingPhaseFilter);
            if (err) return(err);
        }
        for (i=0; i<element->complexPoleCount; i++)
        {
            err = deriveMatchingPhaseFilterParams(CONFIG_COMPLEX_POLE,
                                                  element->complexPoleRadius[i],
                                                  element->complexPoleAngle[i],
                                                  matchingPhaseFilter);
            if (err) return(err);
        }
    }
    err = deriveMatchingPhaseFilterDelay (element, matchingPhaseFilter);
    if (err) return(err);

    matchingPhaseFilter->matchesFilterCount = 1;
    matchingPhaseFilter->matchesFilter[0] = filterElementIndex;
    
    return (0);
}

int
checkPhaseFilterIsEqual (MatchingPhaseFilter* matchingPhaseFilter1,
                         MatchingPhaseFilter* matchingPhaseFilter2,
                         int* isEqual)
{
    int i;
    
    *isEqual = 1;
    if (matchingPhaseFilter1->sectionCount != matchingPhaseFilter2->sectionCount)
    {
        *isEqual = 0;
    }
    else
    {
        for (i=0; i<matchingPhaseFilter1->sectionCount; i++)
        {
            if ((matchingPhaseFilter1->filterSection[i].a1 != matchingPhaseFilter2->filterSection[i].a1) ||
                (matchingPhaseFilter1->filterSection[i].a2 != matchingPhaseFilter2->filterSection[i].a2) ||
                (matchingPhaseFilter1->filterSection[i].b1 != matchingPhaseFilter2->filterSection[i].b1) ||
                (matchingPhaseFilter1->filterSection[i].b2 != matchingPhaseFilter2->filterSection[i].b2))
            {
                *isEqual = 0;
                break;
            }
        }
    }
    if (matchingPhaseFilter1->audioDelay.delay != matchingPhaseFilter2->audioDelay.delay)
    {
        *isEqual = 0;
    }
    return (0);
}

int
addPhaseAlignmentFilter(MatchingPhaseFilter* matchingPhaseFilter,
                        EqFilterElement* eqFilterElement)
{
    if (matchingPhaseFilter->isValid == 1)
    {
        eqFilterElement->phaseAlignmentFilter[eqFilterElement->phaseAlignmentFilterCount] = *matchingPhaseFilter;
        eqFilterElement->phaseAlignmentFilterCount++;
    }
    else
    {
        fprintf(stderr, "ERROR: attempt to add invalid phase aligment filter. (Perhaps caused by FIR filter element?");
        return (UNEXPECTED_ERROR);
    }
    return (0);
}

int
deriveElementPhaseAlignmentFilters(MatchingPhaseFilter* matchingPhaseFilter,
                                   EqFilterBlock* eqFilterBlock)
{
    int i, k, m, err, skip;
    int optimizedPhaseFilterCount;
    int isEqual;
    int pathDelayMin, pathDelay, pathDelayNew, pathDelayToRemove;
    MatchingPhaseFilter matchingPhaseFilterOpt[FILTER_ELEMENT_COUNT_MAX];

    /* (1) Find identical matching phase filters */
    optimizedPhaseFilterCount = 0;
    for (i=0; i<eqFilterBlock->elementCount; i++)
    {
        isEqual = 0;
        for (k=0; k<optimizedPhaseFilterCount; k++)
        {
            err = checkPhaseFilterIsEqual (&matchingPhaseFilter[i], &matchingPhaseFilterOpt[k], &isEqual);
            if (err) return(err);
            if (isEqual == 1) break;
        }
        if (isEqual == 1)
        {
            matchingPhaseFilterOpt[k].matchesFilter[matchingPhaseFilterOpt[k].matchesFilterCount] = i;
            matchingPhaseFilterOpt[k].matchesFilterCount++;
        }
        else
        {
            matchingPhaseFilterOpt[optimizedPhaseFilterCount] = matchingPhaseFilter[i];
            optimizedPhaseFilterCount++;
        }
    }
    /* (2) Add matching phase filters to filter elements */
    for (i=0; i<eqFilterBlock->elementCount; i++)
    {
        for (k=0; k<optimizedPhaseFilterCount; k++)
        {
            skip = 0;
            for (m=0; m<matchingPhaseFilterOpt[k].matchesFilterCount; m++)
            {
                if (matchingPhaseFilterOpt[k].matchesFilter[m] == i)
                {
                    skip = 1;
                    break;
                }
            }
            if (skip == 0)
            {
                err = addPhaseAlignmentFilter(&matchingPhaseFilterOpt[k], &eqFilterBlock->eqFilterElement[i]);
                if (err) return (err);
            }
        }
    }
    /* (3) minimize delay */
    pathDelayMin = 100000;
    for (i=0; i<eqFilterBlock->elementCount; i++)
    {
        EqFilterElement* eqFilterElement = &eqFilterBlock->eqFilterElement[i];
        pathDelay = 0;
        for (m=0; m<eqFilterElement->phaseAlignmentFilterCount; m++)
        {
            pathDelay += eqFilterElement->phaseAlignmentFilter[m].audioDelay.delay;
        }
        if (pathDelayMin > pathDelay)
        {
            pathDelayMin = pathDelay;
        }
    }
    if (pathDelayMin > 0)
    {
        for (i=0; i<eqFilterBlock->elementCount; i++)
        {
            EqFilterElement* eqFilterElement = &eqFilterBlock->eqFilterElement[i];
            pathDelayToRemove = pathDelayMin;
            for (m=0; m<eqFilterElement->phaseAlignmentFilterCount; m++)
            {
                pathDelay = eqFilterElement->phaseAlignmentFilter[m].audioDelay.delay;
                pathDelayNew = max(0, pathDelay - pathDelayToRemove);
                pathDelayToRemove -= pathDelay - pathDelayNew;
                eqFilterElement->phaseAlignmentFilter[m].audioDelay.delay = pathDelayNew;
            }
        }
    }
    return (0);
}

int
convertPoleZeroToFilterParams(const int config,
                              float radius,
                              float angle,
                              int* filterParamCount,
                              SecondOrderFilterParams secondOrderFilterParams[])
{
    float zReal;
    float* coeff;
    
    switch (config) {
        case CONFIG_REAL_ZERO_RADIUS_ONE:
                /* Note: radius parameter contains angle of first zero */
                /*       angle parameter contains angle of second zero */
            {
                float angle1 = radius;
                float angle2 = angle;
                secondOrderFilterParams[0].radius = 1.0f;
                coeff = secondOrderFilterParams[0].coeff;

                if (angle1 != angle2)
                {
                    coeff[0] = 0.0f;
                    coeff[1] = -1.0f;
                }
                else if (angle1 == 1.0f)
                {
                    coeff[0] = -2.0f;
                    coeff[1] = 1.0f;
                }
                else
                {
                    coeff[0] = 2.0f;
                    coeff[1] = 1.0f;
                }
                *filterParamCount = 1;
            }
            break;
        case CONFIG_REAL_ZERO:
            secondOrderFilterParams[0].radius = radius;
            if (fabs(radius) == 1.0f)
            {
                printf("ERROR: real zero with radius of 1 is not permitted\n");
                return (UNEXPECTED_ERROR);
            }
            else
            {
                coeff = secondOrderFilterParams[0].coeff;
                coeff[0] = - (radius + 1.0f / radius);
                coeff[1] = 1.0f;
            }
            *filterParamCount = 1;
            break;
        case CONFIG_GENERIC_ZERO:
            zReal = radius * cos(M_PI * angle);
            secondOrderFilterParams[0].radius = radius;
            coeff = secondOrderFilterParams[0].coeff;
            coeff[0] = -2.0f * zReal;
            coeff[1] = radius * radius;
            zReal = cos(M_PI * angle) / radius;
            secondOrderFilterParams[1].radius = radius;
            coeff = secondOrderFilterParams[1].coeff;
            coeff[0] = -2.0f * zReal;
            coeff[1] = 1.0f / (radius * radius);
            *filterParamCount = 2;
            break;
        case CONFIG_REAL_POLE:
            secondOrderFilterParams[0].radius = radius;
            coeff = secondOrderFilterParams[0].coeff;
            coeff[0] = -2.0f * radius;
            coeff[1] = radius * radius;
            *filterParamCount = 1;
            break;
        case CONFIG_COMPLEX_POLE:
            zReal = radius * cos(M_PI * angle);
            secondOrderFilterParams[0].radius = radius;
            coeff = secondOrderFilterParams[0].coeff;
            coeff[0] = -2.0f * zReal;
            coeff[1] = radius * radius;
            secondOrderFilterParams[1].radius = radius;
            secondOrderFilterParams[1].coeff[0] = coeff[0];
            secondOrderFilterParams[1].coeff[1] = coeff[1];
            *filterParamCount = 2;
            break;
        default:
            break;
    }
    return(0);
}

int
convertFirFilterParams (const int firFilterOrder,
                        const int firSymmetry,
                        float* firCoefficient,
                        FirFilter* firFilter)
{
    int i, channel;
    float* coeff = firFilter->coeff;
    
    firFilter->coeffCount = firFilterOrder + 1;
    for (i=0; i<firFilterOrder/2+1; i++) {
        coeff[i] = firCoefficient[i];
    }
    for (i=0; i<(firFilterOrder+1)/2; i++) {
        if (firSymmetry==1)
        {
            coeff[firFilterOrder-i] = - coeff[i];
        }
        else
        {
            coeff[firFilterOrder-i] = coeff[i];
        }
    }
    if ((firSymmetry==1) && ((firFilterOrder & 1) == 0))
    {
        coeff[firFilterOrder/2] = 0.0f;
    }
    for (channel=0; channel<EQ_CHANNEL_COUNT_MAX; channel++)
    {
        for (i=0; i<firFilterOrder+1; i++) {
            firFilter->state[channel][i] = 0.0f;
        }
    }
    return(0);
}

int
derivePoleZeroFilterParams(UniqueTdFilterElement* element,
                           IntermediateFilterParams* intermediateFilterParams)
{
    int i, err, paramIndex, filterParamCount;
    
    intermediateFilterParams->filterFormat = element->eqFilterFormat;
    if (element->eqFilterFormat == FILTER_ELEMENT_FORMAT_POLE_ZERO)
    {
        SecondOrderFilterParams* secondOrderFilterParamsForZeros = intermediateFilterParams->secondOrderFilterParamsForZeros;
        SecondOrderFilterParams* secondOrderFilterParamsForPoles = intermediateFilterParams->secondOrderFilterParamsForPoles;
        
        paramIndex = 0;
        for (i=0; i<element->realZeroRadiusOneCount; i+=2)
        {
            err = convertPoleZeroToFilterParams(CONFIG_REAL_ZERO_RADIUS_ONE,
                                                (float) element->zeroSign[i],
                                                (float) element->zeroSign[i+1],
                                                &filterParamCount,
                                                &(secondOrderFilterParamsForZeros[paramIndex]));
            if (err) return(err);
            paramIndex += filterParamCount;
        }
        for (i=0; i<element->realZeroCount; i++)
        {
            err = convertPoleZeroToFilterParams(CONFIG_REAL_ZERO,
                                                element->realZeroRadius[i],
                                                0.0f,
                                                &filterParamCount,
                                                &(secondOrderFilterParamsForZeros[paramIndex]));
            if (err) return(err);
            paramIndex += filterParamCount;
        }
        for (i=0; i<element->genericZeroCount; i++)
        {
            err = convertPoleZeroToFilterParams(CONFIG_GENERIC_ZERO,
                                                element->genericZeroRadius[i],
                                                element->genericZeroAngle[i],
                                                &filterParamCount,
                                                &(secondOrderFilterParamsForZeros[paramIndex]));
            if (err) return(err);
            paramIndex += filterParamCount;
        }
        intermediateFilterParams->filterParamCountForZeros = paramIndex;
        paramIndex = 0;
        
        for (i=0; i<element->realPoleCount; i++)
        {
            err = convertPoleZeroToFilterParams(CONFIG_REAL_POLE,
                                                element->realPoleRadius[i],
                                                0.0f,
                                                &filterParamCount,
                                                &(secondOrderFilterParamsForPoles[paramIndex]));
            if (err) return(err);
            paramIndex += filterParamCount;
        }
        for (i=0; i<element->complexPoleCount; i++)
        {
            err = convertPoleZeroToFilterParams(CONFIG_COMPLEX_POLE,
                                                element->complexPoleRadius[i],
                                                element->complexPoleAngle[i],
                                                &filterParamCount,
                                                &(secondOrderFilterParamsForPoles[paramIndex]));
            if (err) return(err);
            paramIndex += filterParamCount;
        }
        intermediateFilterParams->filterParamCountForPoles = paramIndex;
    }
    else
    {
        intermediateFilterParams->filterParamCountForZeros = 0;
        intermediateFilterParams->filterParamCountForPoles = 0;
        
        err = convertFirFilterParams (element->firFilterOrder,
                                      element->firSymmetry,
                                      element->firCoefficient,
                                      &intermediateFilterParams->firFilter);
        if (err) return(err);
    }
    return (0);
}

int
deriveEqFilterElements (IntermediateFilterParams* intermediateFilterParams,
                        EqFilterElement* eqFilterElement)
{
    int i, polesIndex, zerosIndex, poleOrder = 0, section, channel;
    int polesDone[REAL_POLE_COUNT_MAX + COMPLEX_POLE_COUNT_MAX];
    int zerosDone[REAL_ZERO_COUNT_MAX + COMPLEX_ZERO_COUNT_MAX];
    float radiusMax, radiusDiff;
    int coeffCount;
    float* coeff;
    
    for (i=0; i<REAL_POLE_COUNT_MAX + COMPLEX_POLE_COUNT_MAX; i++)
    {
        polesDone[i] = 0;
    }
    for (i=0; i<REAL_ZERO_COUNT_MAX + COMPLEX_ZERO_COUNT_MAX; i++)
    {
        zerosDone[i] = 0;
    }
    section = 0;
    do
    {
        radiusMax = -1.0;
        polesIndex = -1;
        /* Find poles with the maximum radius */
        for (i=0; i<intermediateFilterParams->filterParamCountForPoles; i++)
        {
            if (polesDone[i] == 0)
            {
                if (intermediateFilterParams->filterFormat == 0)
                {
                    if (radiusMax < fabs(intermediateFilterParams->secondOrderFilterParamsForPoles[i].radius))
                    {
                        radiusMax = fabs(intermediateFilterParams->secondOrderFilterParamsForPoles[i].radius);
                        polesIndex = i;
                        if (intermediateFilterParams->secondOrderFilterParamsForPoles[i].coeff[1] != 0.0f)
                        {
                            poleOrder = 2;
                        }
                        else
                        {
                            poleOrder = 1;
                        }
                    }
                }
            }
        }
        if (polesIndex >= 0)
        {
            /* Find zeros with the closest matching radius. Try to match the order as well */
            radiusDiff = 10.0f;
            zerosIndex = -1;
            for (i=0; i<intermediateFilterParams->filterParamCountForZeros; i++)
            {
                if (zerosDone[i] == 0)
                {
                    if (intermediateFilterParams->filterFormat == 0)
                    {
                        if (poleOrder == 2) {
                            if (intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[1] != 0.0f)
                            {
                                if (radiusDiff > fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax)) {
                                     radiusDiff = fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax);
                                    zerosIndex = i;
                                }
                            }
                        }
                        else
                        {
                            if (intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[1] == 0.0f)
                            {
                                if (radiusDiff > fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax)) {
                                    radiusDiff = fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax);
                                    zerosIndex = i;
                                }
                            }
                        }
                    }
                }
            }
            if (zerosIndex == -1)   /* No zeros found with same order. Try other filter order */
            {
                for (i=0; i<intermediateFilterParams->filterParamCountForZeros; i++)
                {
                    if (zerosDone[i] == 0)
                    {
                        if (intermediateFilterParams->filterFormat == 0)
                        {
                            if (poleOrder == 2) {
                                if (intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[1] == 0.0f)
                                {
                                    if (radiusDiff > fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax)) {
                                        radiusDiff = fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax);
                                        zerosIndex = i;
                                    }
                                }
                            }
                            else
                            {
                                if (intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[1] != 0.0f)
                                {
                                    if (radiusDiff > fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax)) {
                                        radiusDiff = fabs(fabs(intermediateFilterParams->secondOrderFilterParamsForZeros[i].radius) - radiusMax);
                                        zerosIndex = i;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            /* TODO: consider angle to find closest zero */
            eqFilterElement->poleZeroFilter.filterSection[section].a1 = intermediateFilterParams->secondOrderFilterParamsForPoles[polesIndex].coeff[0];
            eqFilterElement->poleZeroFilter.filterSection[section].a2 = intermediateFilterParams->secondOrderFilterParamsForPoles[polesIndex].coeff[1];
            if (zerosIndex >= 0)
            {
                eqFilterElement->poleZeroFilter.filterSection[section].b1 = intermediateFilterParams->secondOrderFilterParamsForZeros[zerosIndex].coeff[0];
                eqFilterElement->poleZeroFilter.filterSection[section].b2 = intermediateFilterParams->secondOrderFilterParamsForZeros[zerosIndex].coeff[1];
            }
            else
            {
                eqFilterElement->poleZeroFilter.filterSection[section].b1 = 0.0f;
                eqFilterElement->poleZeroFilter.filterSection[section].b2 = 0.0f;
                eqFilterElement->poleZeroFilter.audioDelay.delay++;   /* This added delay is necessary to match the delay of the corresponding phase alignement filter */
            }                                                         /*  Otherwise it can be removed */
            for (channel=0; channel<EQ_CHANNEL_COUNT_MAX; channel++)
            {
                eqFilterElement->poleZeroFilter.filterSection[section].filterSectionState[channel].stateIn1 = 0.0f;
                eqFilterElement->poleZeroFilter.filterSection[section].filterSectionState[channel].stateIn2 = 0.0f;
                eqFilterElement->poleZeroFilter.filterSection[section].filterSectionState[channel].stateOut1 = 0.0f;
                eqFilterElement->poleZeroFilter.filterSection[section].filterSectionState[channel].stateOut2 = 0.0f;
            }
            if (zerosIndex >= 0) zerosDone[zerosIndex] = 1;
            if (polesIndex >= 0) polesDone[polesIndex] = 1;
            section++;
        }
    } while (polesIndex >= 0);
    
    eqFilterElement->poleZeroFilter.sectionCount = section;

    /* lump remaining zeros into an FIR filter */
    coeffCount = 1;
    coeff = eqFilterElement->poleZeroFilter.firFilter.coeff;
    coeff[0] = 1.0f;
    for (i=0; i<intermediateFilterParams->filterParamCountForZeros; i++)
    {
        if (zerosDone[i] == 0)
        {
            if (intermediateFilterParams->filterFormat == 0)
            {
                int k;
                float b1, b2;
                b1 = intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[0];
                b2 = intermediateFilterParams->secondOrderFilterParamsForZeros[i].coeff[1];

                coeffCount += 2;
                k = coeffCount - 1;
                coeff[k]          =                   b2 * coeff[k-2];
                k--;
                if (k>1)
                {
                    coeff[k]      = b1 * coeff[k-1] + b2 * coeff[k-2];
                    k--;
                    for (  ; k>1; k--)
                    {
                        coeff[k] += b1 * coeff[k-1] + b2 * coeff[k-2];
                    }
                    coeff[1]     += b1 * coeff[0];
                }
                else
                {
                    coeff[1]      = b1 * coeff[0];
                }
            }
        }
        zerosDone[i] = 1;
    }
    if (coeffCount > 1)
    {
        eqFilterElement->poleZeroFilter.firCoeffsPresent = 1;
        eqFilterElement->poleZeroFilter.firFilter.coeffCount = coeffCount;
    }
    else
    {
        eqFilterElement->poleZeroFilter.firCoeffsPresent = 0;
        eqFilterElement->poleZeroFilter.firFilter.coeffCount = 0;
    }
    
    return(0);
}

int
deriveFilterBlock(UniqueTdFilterElement* uniqueTdFilterElement,
                  FilterBlock* filterBlock,
                  EqFilterBlock* eqFilterBlock)
{
    int i, k, err;
    IntermediateFilterParams intermediateFilterParams;
    MatchingPhaseFilter matchingPhaseFilter[FILTER_ELEMENT_COUNT_MAX];

    for (i=0; i<filterBlock->filterElementCount; i++)
    {
        if ((uniqueTdFilterElement[filterBlock->filterElement[i].filterElementIndex].eqFilterFormat == FILTER_ELEMENT_FORMAT_FIR) && (filterBlock->filterElementCount > 1))
        {
            fprintf (stderr, "ERROR: An FIR filter cannot be used in a filter block with multiple filter elements\n");
            return (UNEXPECTED_ERROR);
        }
    }
    for (i=0; i<filterBlock->filterElementCount; i++)
    {
        EqFilterElement* eqFilterElement = &eqFilterBlock->eqFilterElement[i];
        FilterElement* filterElement = &filterBlock->filterElement[i];
        int filterIndex = filterElement->filterElementIndex;
        
        if (uniqueTdFilterElement[filterIndex].eqFilterFormat == FILTER_ELEMENT_FORMAT_POLE_ZERO)
        {
            err = derivePoleZeroFilterParams(&(uniqueTdFilterElement[filterIndex]),
                                             &intermediateFilterParams);
            if (err) return (err);

            err = deriveEqFilterElements (&intermediateFilterParams, eqFilterElement);
            if (err) return (err);
            eqFilterElement->format = FILTER_ELEMENT_FORMAT_POLE_ZERO;
        }
        else
        {
            err = convertFirFilterParams (uniqueTdFilterElement[filterIndex].firFilterOrder,
                                          uniqueTdFilterElement[filterIndex].firSymmetry,
                                          uniqueTdFilterElement[filterIndex].firCoefficient,
                                          &eqFilterElement->firFilter);
            if (err) return (err);
            eqFilterElement->format = FILTER_ELEMENT_FORMAT_FIR;
        }
        if (filterElement->filterElementGainPresent == 1)
        {
            eqFilterElement->elementGainLinear = pow(10.0f, 0.05f * filterElement->filterElementGain);
        }
        else
        {
            eqFilterElement->elementGainLinear = 1.0f;
        }
        for (k=0; k<uniqueTdFilterElement[filterIndex].realZeroCount; k++)
        {
            if (uniqueTdFilterElement[filterIndex].realZeroRadius[k] > 0.0f)
            {
                eqFilterElement->elementGainLinear = - eqFilterElement->elementGainLinear;
            }
        }
        err = deriveMatchingPhaseFilter(&(uniqueTdFilterElement[filterIndex]), i,
                                        &matchingPhaseFilter[i]);
        if (err) return (err);
    }
    eqFilterBlock->elementCount = filterBlock->filterElementCount;
    
    eqFilterBlock->matchingPhaseFilterElement0 = matchingPhaseFilter[0]; /* keep for later */
    
    err = deriveElementPhaseAlignmentFilters(matchingPhaseFilter, eqFilterBlock);
    if (err) return (err);

    return(0);
}

int
deriveCascadePhaseAlignmentFilters(TdFilterCascade* tdFilterCascade,
                                   const int channelGroupCount,
                                   FilterCascadeTDomain filterCascadeTDomain[])
{
    int err;
    int cascadeAlignmentGroupCount = 0;
    CascadeAlignmentGroup cascadeAlignmentGroup[EQ_CHANNEL_GROUP_COUNT_MAX/2];

    err = deriveCascadeAlignmentGroups (channelGroupCount,
                                        tdFilterCascade->eqPhaseAlignmentPresent,
                                        tdFilterCascade->eqPhaseAlignment,
                                        &cascadeAlignmentGroupCount,
                                        cascadeAlignmentGroup);
    if (err) return(err);
    
    if (cascadeAlignmentGroupCount > 0)
    {
        err = phaseAlignCascadeGroup(cascadeAlignmentGroupCount,
                                     cascadeAlignmentGroup,
                                     filterCascadeTDomain);
        if (err) return(err);
    }
    return (0);
}


int
deriveFilterCascade(UniqueTdFilterElement* uniqueTdFilterElement,
                    FilterBlock* filterBlock,
                    TdFilterCascade* tdFilterCascade,
                    int channelGroupCount,
                    FilterCascadeTDomain filterCascadeTDomain[])
{
    int i, err, g;
    
    for (g=0; g<channelGroupCount; g++)
    {
        for (i=0; i<tdFilterCascade->filterBlockRefs[g].filterBlockCount; i++)
        {
            err = deriveFilterBlock(uniqueTdFilterElement,
                                    &(filterBlock[tdFilterCascade->filterBlockRefs[g].filterBlockIndex[i]]),
                                    &(filterCascadeTDomain[g].eqFilterBlock[i]));
            if (err) return(err);
        }
        filterCascadeTDomain[g].blockCount = i;
        filterCascadeTDomain[g].cascadeGainLinear = pow(10.0f, 0.05f * tdFilterCascade->eqCascadeGain[g]);
    }

    err = deriveCascadePhaseAlignmentFilters(tdFilterCascade,
                                             channelGroupCount,
                                             filterCascadeTDomain);
    if (err) return(err);
    /* TODO: remove any unneccessary delay in all filter cascades */
    
    return(0);
}

int
deriveSubbandEq(EqSubbandGainVector* eqSubbandGainVector,
                const int eqSubbandGainCount,
                SubbandFilter* subbandFilter)
{
    int i;
    
    for (i=0; i<eqSubbandGainCount; i++)
    {
        subbandFilter->subbandCoeff[i] = eqSubbandGainVector->eqSubbandGain[i];
    }
    subbandFilter->coeffCount = eqSubbandGainCount;
    return (0);
}

float
getStepRatio ()
{
    return ((log10(STEP_RATIO_F_HI) / log10(STEP_RATIO_F_LO) - 1.0f) / (STEP_RATIO_EQ_NODE_COUNT_MAX - 1.0f));
}

float
decodeEqNodeFreq(const int eqNodeFreqIndex)
{
    float stepRatio = getStepRatio();
    return(pow(STEP_RATIO_F_LO, 1.0f + eqNodeFreqIndex * stepRatio));
}

float
warpFreqDelta(const float fSubband,
              const float nodeFrequency0,
              const int   eqNodeFreqIndex)
{
    float stepRatio = getStepRatio();
    return((log10(fSubband)/log10(nodeFrequency0) - 1.0f) / stepRatio - (float) eqNodeFreqIndex);
}

int
interpolateEqGain(const int bandStep,
                  const float gain0,
                  const float gain1,
                  const float slope0,
                  const float slope1,
                  const float f,
                  float* interpolatedGain)
{
    float k1, k2, a, b, c, d;
    float nodesPerOctaveCount = 3.128f;
    float gainLeft = gain0;
    float gainRight = gain1;
    float slopeLeft = slope0 / nodesPerOctaveCount; float slopeRight = slope1 / nodesPerOctaveCount; float bandStepInv = 1.0 / (float)bandStep;
    float bandStepInv2 = bandStepInv * bandStepInv; k1 = (gainRight - gainLeft) * bandStepInv2;
    k2 = slopeRight + slopeLeft;
    a = bandStepInv * (bandStepInv * k2 - 2.0 * k1); b = 3.0 * k1 - bandStepInv * (k2 + slopeLeft);
    c = slopeLeft;
    d = gainLeft;
    *interpolatedGain = (((a * f + b ) * f + c ) * f ) + d;
    return (0);
}

int
interpolateSubbandSpline(EqSubbandGainSpline* eqSubbandGainSpline,
                         const int eqSubbandGainCount,
                         const int eqSubbandGainFormat,
                         const float audioSampleRate,
                         SubbandFilter* subbandFilter)
{
    int b, n, err;

    float eqGain[32];
    int eqNodeFreqIndex[32];
    float eqNodeFreq[32];
    float subbandCenterFreq[256];
    int nEqNodes = eqSubbandGainSpline->nEqNodes;
    
    
    float* eqSlope = eqSubbandGainSpline->eqSlope;
    int* eqFreqDelta = eqSubbandGainSpline->eqFreqDelta;
    float eqGainInitial = eqSubbandGainSpline->eqGainInitial;
    float* eqGainDelta = eqSubbandGainSpline->eqGainDelta;

    float* subbandCoeff = subbandFilter->subbandCoeff;
    int eqNodeCountMax = 33;
    int eqNodeIndexMax = eqNodeCountMax - 1;
    eqGain[0] = eqGainInitial;
    eqNodeFreqIndex[0] = 0;
    eqNodeFreq[0] = decodeEqNodeFreq(eqNodeFreqIndex[0]);
    for (n=1; n<nEqNodes; n++) {
        eqGain[n] = eqGain[n-1] + eqGainDelta[n];
        eqNodeFreqIndex[n] = eqNodeFreqIndex[n-1] + eqFreqDelta[n];
        eqNodeFreq[n] = decodeEqNodeFreq(eqNodeFreqIndex[n]);
    }
    if ((eqNodeFreq[nEqNodes-1] < audioSampleRate * 0.5f) && (eqNodeFreqIndex[nEqNodes-1] < eqNodeIndexMax)) {
        eqSlope[nEqNodes] = 0;
        eqGain[nEqNodes] = eqGain[nEqNodes-1];
        eqFreqDelta[nEqNodes] = eqNodeIndexMax - eqNodeFreqIndex[nEqNodes-1];
        eqNodeFreqIndex[nEqNodes] = eqNodeIndexMax;
        eqNodeFreq [nEqNodes] = decodeEqNodeFreq(eqNodeFreqIndex[nEqNodes]); nEqNodes += 1;
    }
    
    err = deriveSubbandCenterFreq(eqSubbandGainCount, eqSubbandGainFormat, audioSampleRate, subbandCenterFreq);
    if (err) return (err);
    
    for (n=0; n<nEqNodes-1; n++) {
        for (b=0; b<eqSubbandGainCount; b++) {
            float fSub;
            fSub = max(subbandCenterFreq[b], eqNodeFreq[0]);
            fSub = min(fSub, eqNodeFreq[nEqNodes-1]);
            if ((fSub >= eqNodeFreq[n]) && (fSub <= eqNodeFreq[n+1])) {
                float warpedDeltaFreq = warpFreqDelta (fSub, eqNodeFreq[0], eqNodeFreqIndex[n]);
                float gEqSubbandDb;
                err = interpolateEqGain(eqFreqDelta[n+1], eqGain[n], eqGain[n+1],
                                        eqSlope[n], eqSlope[n+1], warpedDeltaFreq, &gEqSubbandDb);
                if (err) return (err);
                subbandCoeff[b] = pow(2.0, gEqSubbandDb / 6.0f);
            }
        }
    }
    subbandFilter->coeffCount = eqSubbandGainCount;
    return (0);
}

int
deriveSubbandGains(EqCoefficients* eqCoefficients,
                   const int eqChannelGroupCount,
                   const int* subbandGainsIndex,
                   const float audioSampleRate,
                   const int eqFrameSizeSubband,
                   SubbandFilter* subbandFilter)
{
    int g, err;
    int eqSubbandGainRepresentation = eqCoefficients->eqSubbandGainRepresentation;
    int eqSubbandGainCount = eqCoefficients->eqSubbandGainCount;
    int eqSubbandGainFormat = eqCoefficients->eqSubbandGainFormat;
    
    for (g=0; g<eqChannelGroupCount; g++)
    {
        if (eqSubbandGainRepresentation == 1)
        {
            err = interpolateSubbandSpline(&(eqCoefficients->eqSubbandGainSpline[subbandGainsIndex[g]]),
                                           eqSubbandGainCount,
                                           eqSubbandGainFormat,
                                           audioSampleRate,
                                           &(subbandFilter[g]));
            if (err) return(err);
        }
        else
        {
            err = deriveSubbandEq(&(eqCoefficients->eqSubbandGainVector[subbandGainsIndex[g]]),
                                    eqSubbandGainCount,
                                    &(subbandFilter[g]));
            if (err) return(err);
        }
        subbandFilter[g].eqFrameSizeSubband = eqFrameSizeSubband;
    }
    return (0);
}

int
getEqComplexity (EqSet* eqSet,
                 int* eqComplexityLevel)
{
    float cplx;
#ifdef AMD2_COR1
    int i, g, c, b, p;
#else
    int i, g, c, b;
#endif
    int firOrderComplexity = 0;
    int zeroPolePairCountComplexity = 0;
    int subbandFilterComplexity = 0;
    
    for (c=0; c<eqSet->audioChannelCount; c++)
    {
        g = eqSet->eqChannelGroupForChannel[c];
        if (g>=0)
        {
            switch (eqSet->domain)
            {
                case EQ_FILTER_DOMAIN_TIME:
                    {
                        FilterCascadeTDomain* filterCascadeTDomain = &eqSet->filterCascadeTDomain[g];
                        for (b=0; b<filterCascadeTDomain->blockCount; b++)
                        {
                            EqFilterBlock* eqFilterBlock = &filterCascadeTDomain->eqFilterBlock[b];
                            for (i=0; i<eqFilterBlock->elementCount; i++)
                            {
                                EqFilterElement* eqFilterElement = &eqFilterBlock->eqFilterElement[i];
                                switch (eqFilterElement->format)
                                {
                                    case FILTER_ELEMENT_FORMAT_POLE_ZERO:
                                        zeroPolePairCountComplexity += eqFilterElement->poleZeroFilter.sectionCount * 2;
                                        if (eqFilterElement->poleZeroFilter.firCoeffsPresent)
                                        {
                                            firOrderComplexity += eqFilterElement->poleZeroFilter.firFilter.coeffCount - 1;
                                        }
                                        break;
                                    case FILTER_ELEMENT_FORMAT_FIR:
                                        firOrderComplexity += eqFilterElement->firFilter.coeffCount - 1;
                                        break;
                                    default:
                                        break;
                                }
#ifdef AMD2_COR1
                                for (p=0; p<eqFilterElement->phaseAlignmentFilterCount; p++)
                                {
                                    zeroPolePairCountComplexity += eqFilterElement->phaseAlignmentFilter[p].sectionCount * 2;
#else
                                for (i=0; i<eqFilterElement->phaseAlignmentFilterCount; i++)
                                {
                                    zeroPolePairCountComplexity += eqFilterElement->phaseAlignmentFilter[i].sectionCount * 2;
#endif
                                }
                            }
                        }
                        for (b=0; b<filterCascadeTDomain->phaseAlignmentFilterCount; b++)
                        {
                            zeroPolePairCountComplexity += filterCascadeTDomain->phaseAlignmentFilter[b].sectionCount * 2;
                        }
                    }
                    break;
                case EQ_FILTER_DOMAIN_SUBBAND:
                    subbandFilterComplexity++;
                    break;
                case EQ_FILTER_DOMAIN_NONE:
                default:
                    break;
            }
        }
    }
    cplx = COMPLEXITY_W_SUBBAND_EQ * subbandFilterComplexity;
    cplx += COMPLEXITY_W_IIR * zeroPolePairCountComplexity;
    cplx += COMPLEXITY_W_FIR * firOrderComplexity;
    cplx = log10(cplx / eqSet->audioChannelCount) / log10(2.0f);
    *eqComplexityLevel = (int) max(0, ceil(cplx));
    if (*eqComplexityLevel > EQ_COMPLEXITY_LEVEL_MAX)
    {
        fprintf(stderr, "ERROR: EQ set exceeds complexity limit of 15: %d\n", *eqComplexityLevel);
        return(-1);
    }
    return(0);
}

int
getFilterSectionDelay(const int sectionCount,
                      const FilterSection* filterSection,
                      float* delay)
{
    int i;
    float d = 0.0f;
    for (i=0; i<sectionCount; i++)
    {
        if (filterSection[i].b2 != 0.0f)
        {
            d += 1.0f;
        }
        else if (filterSection[i].b1 != 0.0f)
        {
            d += 0.5f;
        }
    }
    *delay = d;
    return(0);
}

int
getEqSetDelay (EqSet* eqSet,
               int* cascadeDelay)
{
    float delay, sectionDelay;
    int k, i, g, c, b, err;
    
    delay = 0;
    for (c=0; c<eqSet->audioChannelCount; c++)
    {
        g = eqSet->eqChannelGroupForChannel[c];
        if (g>=0)
        {
            switch (eqSet->domain)
            {
                case EQ_FILTER_DOMAIN_TIME:
                {
                    FilterCascadeTDomain* filterCascadeTDomain = &eqSet->filterCascadeTDomain[g];
                    for (b=0; b<filterCascadeTDomain->blockCount; b++)
                    {
                        EqFilterElement* eqFilterElement = &filterCascadeTDomain->eqFilterBlock[b].eqFilterElement[0];
                        switch (eqFilterElement->format)
                        {
                            case FILTER_ELEMENT_FORMAT_POLE_ZERO:
                                err = getFilterSectionDelay(eqFilterElement->poleZeroFilter.sectionCount,
                                                      eqFilterElement->poleZeroFilter.filterSection,
                                                      &sectionDelay);
                                if (err) return (err);
                                delay += sectionDelay;
                                if (eqFilterElement->poleZeroFilter.firCoeffsPresent)
                                {
                                    delay += 0.5f * (eqFilterElement->poleZeroFilter.firFilter.coeffCount - 1);
                                }
                                break;
                            case FILTER_ELEMENT_FORMAT_FIR:
                                delay += 0.5f * (eqFilterElement->firFilter.coeffCount - 1);
                                break;
                            default:
                                break;
                        }
                        for (k=0; k<eqFilterElement->phaseAlignmentFilterCount; k++)
                        {
                            PhaseAlignmentFilter* phaseAlignmentFilter = &eqFilterElement->phaseAlignmentFilter[k];
                            err = getFilterSectionDelay(phaseAlignmentFilter->sectionCount,
                                                        phaseAlignmentFilter->filterSection,
                                                        &sectionDelay);
                            if (err) return (err);
                            delay += sectionDelay;
                        }
                    }
                    for (b=0; b<filterCascadeTDomain->phaseAlignmentFilterCount; b++)
                    {
                        PhaseAlignmentFilter* phaseAlignmentFilter = &filterCascadeTDomain->phaseAlignmentFilter[b];
                        err = getFilterSectionDelay(phaseAlignmentFilter->sectionCount,
                                                    phaseAlignmentFilter->filterSection,
                                                    &sectionDelay);
                        if (err) return (err);
                        delay += sectionDelay;
                   }
                }
                    break;
                case EQ_FILTER_DOMAIN_SUBBAND:
                case EQ_FILTER_DOMAIN_NONE:
                default:
                    break;
            }
        }
        break;
    }
    *cascadeDelay = (int)delay;
    return(0);
}

int
deriveEqSet (EqCoefficients* eqCoefficients,
             EqInstructions* eqInstructions,
             const float audioSampleRate,
             const int drcFrameSize,
             const int subBandDomainMode,
             EqSet* eqSet)
{
    int err, i, eqFrameSizeSubband;
    
    eqSet->domain = EQ_FILTER_DOMAIN_NONE;
    
    if (subBandDomainMode == SUBBAND_DOMAIN_MODE_OFF)
    {
        if (eqInstructions->tdFilterCascadePresent== 1)
        {
            err = deriveFilterCascade(eqCoefficients->uniqueTdFilterElement,
                                      eqCoefficients->filterBlock,
                                      &eqInstructions->tdFilterCascade,
                                      eqInstructions->eqChannelGroupCount,
                                      eqSet->filterCascadeTDomain);
            if (err) return (err);
        }
        else
        {
            fprintf(stderr, "WARNING: time domain EQ filter parameters cannot be found. EQ cannot be applied.\n");
        }
        eqSet->domain |= EQ_FILTER_DOMAIN_TIME;
    }
    if (subBandDomainMode != SUBBAND_DOMAIN_MODE_OFF)
    {
        switch (subBandDomainMode) {
                case SUBBAND_DOMAIN_MODE_QMF64:
                    if (eqCoefficients->eqSubbandGainCount != AUDIO_CODEC_SUBBAND_COUNT_QMF64)
                    {
                        fprintf(stderr, "ERROR: Subband count mismatch %d %d", eqCoefficients->eqSubbandGainCount, AUDIO_CODEC_SUBBAND_COUNT_QMF64);
                        return (UNEXPECTED_ERROR);
                    }
                    eqFrameSizeSubband = drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
                    break;
                case SUBBAND_DOMAIN_MODE_QMF71:
                    if (eqCoefficients->eqSubbandGainCount != AUDIO_CODEC_SUBBAND_COUNT_QMF71)
                    {
                        fprintf(stderr, "ERROR: Subband count mismatch %d %d", eqCoefficients->eqSubbandGainCount, AUDIO_CODEC_SUBBAND_COUNT_QMF71);
                        return (UNEXPECTED_ERROR);
                    }
                    eqFrameSizeSubband = drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
                    break;
                case SUBBAND_DOMAIN_MODE_STFT256:
                    if (eqCoefficients->eqSubbandGainCount != AUDIO_CODEC_SUBBAND_COUNT_STFT256)
                    {
                        fprintf(stderr, "ERROR: Subband count mismatch %d %d", eqCoefficients->eqSubbandGainCount, AUDIO_CODEC_SUBBAND_COUNT_STFT256);
                        return (UNEXPECTED_ERROR);
                    }
                    eqFrameSizeSubband = drcFrameSize / AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
                    break;
                default:
                    fprintf(stderr, "ERROR: subBandDomainMode not supported %d", subBandDomainMode);
                    return (UNEXPECTED_ERROR);
                break;
        }
        if (eqInstructions->subbandGainsPresent== 1)
        {
            err = deriveSubbandGains(eqCoefficients,
                                     eqInstructions->eqChannelGroupCount,
                                     eqInstructions->subbandGainsIndex,
                                     audioSampleRate, /* TODO: is this the subband sampling rate? --> derive from subBandDOmainMode and corresponding macros */
                                     eqFrameSizeSubband,
                                     eqSet->subbandFilter);
            if (err) return (err);
        }
        else
        {
            if (eqInstructions->tdFilterCascadePresent== 1)
            {
                err = deriveSubbandGainsFromTdCascade(eqCoefficients->uniqueTdFilterElement,
                                                      eqCoefficients->filterBlock,
                                                      &eqInstructions->tdFilterCascade,
                                                      eqCoefficients->eqSubbandGainFormat,
                                                      eqInstructions->eqChannelGroupCount,
                                                      eqInstructions->subbandGainsIndex,
                                                      audioSampleRate,
                                                      eqFrameSizeSubband,
                                                      eqSet->subbandFilter);
                if (err) return (err);
            }
            else
            {
                fprintf(stderr, "WARNING: no EQ filter parameters present. EQ cannot be applied.\n");
            }
        }
        eqSet->domain |= EQ_FILTER_DOMAIN_SUBBAND;
    }
    eqSet->audioChannelCount = eqInstructions->eqChannelCount;
    eqSet->eqChannelGroupCount = eqInstructions->eqChannelGroupCount;
    
    for (i=0; i<eqInstructions->eqChannelCount; i++)
    {
        eqSet->eqChannelGroupForChannel[i] = eqInstructions->eqChannelGroupForChannel[i];
    }

    return (0);
}

/* ============ Processing ============== */

int
processFilterSection(FilterSection* filterSection,
                     const int channel,
                     float audioIn,
                     float* audioOut)
{
    FilterSectionState* filterSectionState = &filterSection->filterSectionState[channel];

    *audioOut = audioIn + filterSection->b1 * filterSectionState->stateIn1
                        + filterSection->b2 * filterSectionState->stateIn2
                        - filterSection->a1 * filterSectionState->stateOut1
                        - filterSection->a2 * filterSectionState->stateOut2;
    filterSectionState->stateIn2 = filterSectionState->stateIn1;
    filterSectionState->stateIn1 = audioIn;
    filterSectionState->stateOut2 = filterSectionState->stateOut1;
    filterSectionState->stateOut1 = *audioOut;
    return (0);
}

int
processFirFilter(FirFilter* firFilter,
                 const int channel,
                 float audioIn,
                 float* audioOut)
{
    int i;
    float* coeff = firFilter->coeff;
    float* state = firFilter->state[channel];
    float sum;
    sum = coeff[0] * audioIn;
    for (i=1; i<firFilter->coeffCount; i++)
    {
        sum += coeff[i] * state[i-1];
    }
    *audioOut = sum;
    for (i=firFilter->coeffCount-2; i>0; i--)
    {
        state[i] = state[i-1];
    }
    state[0] = audioIn;
    return (0);
}

int
processAudioDelay(AudioDelay* audioDelay,
                  const int channel,
                  float audioIn,
                  float* audioOut)
{
    int i;
    float* state = audioDelay->state[channel];
    if (audioDelay->delay > 0)
    {
        *audioOut = state[audioDelay->delay-1];
        for (i=audioDelay->delay-1; i>0; i--)
        {
            state[i] = state[i-1];
        }
        state[0] = audioIn;
    }
    else
    {
        *audioOut = audioIn;
    }
    return (0);
}

int
processPoleZeroFilter(PoleZeroFilter* poleZeroFilter,
                      const int channel,
                      float audioIn,
                      float* audioOut)
{
    int i, err;
    float aIn = audioIn;
    float aOut = aIn;
    for (i=0; i<poleZeroFilter->sectionCount; i++)
    {
        err = processFilterSection(&poleZeroFilter->filterSection[i], channel, aIn, &aOut);
        if (err) return (err);
        aIn = aOut;
    }
    if (poleZeroFilter->firCoeffsPresent == 1)
    {
        err = processFirFilter(&poleZeroFilter->firFilter, channel, aIn, &aOut);
        if (err) return (err);
        aIn = aOut;
    }
    err = processAudioDelay(&poleZeroFilter->audioDelay, channel, aIn, &aOut);
    if (err) return (err);
    
    *audioOut = aOut;
    return (0);
}

int
processSubbandFilter(SubbandFilter* subbandFilter,
                     float* audioIOBufferReal,
                     float* audioIOBufferImag)
{
    int i;
    float* subbandCoeff = subbandFilter->subbandCoeff;
    for (i=0; i<subbandFilter->coeffCount; i++)
    {
        audioIOBufferReal[i] *= subbandCoeff[i];
        audioIOBufferImag[i] *= subbandCoeff[i];
    }
    return (0);
}

int
processPhaseAlignmentFilter(PhaseAlignmentFilter* phaseAlignmentFilter,
                            const int channel,
                            float audioIn,
                            float* audioOut)
{
    int err, i;
    float aIn = audioIn;
    float aOut = aIn;
    for (i=0; i<phaseAlignmentFilter->sectionCount; i++)
    {
        err = processFilterSection(&phaseAlignmentFilter->filterSection[i], channel, aIn, &aOut);
        if (err) return(err);
        aIn = aOut;
    }
    err = processAudioDelay(&phaseAlignmentFilter->audioDelay, channel, aIn, &aOut);
    if (err) return(err);
    *audioOut = aOut * phaseAlignmentFilter->gain;
    return (0);
}

int
processEqFilterElement(EqFilterElement* eqFilterElement,
                       const int channel,
                       float audioIn,
                       float* audioOut)
{
    int err, i;
    float aIn = audioIn;
    float aOut = aIn;
    switch (eqFilterElement->format)
    {
        case FILTER_ELEMENT_FORMAT_POLE_ZERO:
            err = processPoleZeroFilter(&eqFilterElement->poleZeroFilter, channel, aIn, &aOut);
            if (err) return(err);
            break;
        case FILTER_ELEMENT_FORMAT_FIR:
            err = processFirFilter(&eqFilterElement->firFilter, channel, aIn, &aOut);
            if (err) return(err);
            break;
        default:
            break;
    }
    aOut *= eqFilterElement->elementGainLinear;

    for (i=0; i<eqFilterElement->phaseAlignmentFilterCount; i++)
    {
        aIn = aOut;
        err = processPhaseAlignmentFilter(&eqFilterElement->phaseAlignmentFilter[i], channel, aIn, &aOut);
        if (err) return(err);
    }
    *audioOut = aOut;
    
    return (0);
}

int
processEqFilterBlock(EqFilterBlock* eqFilterBlock,
                     const int channel,
                     float audioIn,
                     float* audioOut)
{
    int err, i;
    float aIn = audioIn;
    float aOut;
    float sum = 0.0f;
    
    for (i=0; i<eqFilterBlock->elementCount; i++)
    {
        err = processEqFilterElement(&eqFilterBlock->eqFilterElement[i], channel, aIn, &aOut);
        if (err) return(err);
        sum += aOut;
    }
    *audioOut = sum;
    return (0);
}

int
processFilterCascade(FilterCascadeTDomain* filterCascadeTDomain,
                     const int channel,
                     float audioIn,
                     float* audioOut)
{
    int err, i;
    float aIn = audioIn;
    float aOut = aIn;

    for (i=0; i<filterCascadeTDomain->blockCount; i++)
    {
        err = processEqFilterBlock(&filterCascadeTDomain->eqFilterBlock[i], channel, aIn, &aOut);
        if (err) return(err);
        aIn = aOut;
    }

    for (i=0; i<filterCascadeTDomain->phaseAlignmentFilterCount; i++)
    {
        err = processPhaseAlignmentFilter(&filterCascadeTDomain->phaseAlignmentFilter[i], channel, aIn, &aOut);
        if (err) return(err);
        aIn = aOut;
    }
    *audioOut = aOut * filterCascadeTDomain->cascadeGainLinear;
    
    return (0);
}

int
processEqSetTimeDomain(EqSet* eqSet,
                       const int channel,
                       float audioIn,
                       float* audioOut)
{
    int err, g;
    
    if (eqSet == NULL)
    {
        *audioOut = audioIn;
    }
    else
    {
        g = eqSet->eqChannelGroupForChannel[channel];
        if (g<0)
        {
            *audioOut = audioIn;
        }
        else
        {
            if (eqSet->domain | EQ_FILTER_DOMAIN_TIME) {
                err = processFilterCascade(&eqSet->filterCascadeTDomain[g], channel, audioIn, audioOut);
                if (err) return (err);
            }
            else
            {
                printf("ERROR: All EQ filters of a set must be in the same domain\n");
                return(UNEXPECTED_ERROR);
            }
        }
    }
    return (0);
}

int
processEqSetSubbandDomain(EqSet* eqSet,
                          const int channel,
                          float* audioIOBufferReal,
                          float* audioIOBufferImag)
{
    int err, g, i, eqFrameSizeSubband, coeffCount, offset;

    if (eqSet != NULL)
    {
        g = eqSet->eqChannelGroupForChannel[channel];
        if (g>=0)
        {
            if (eqSet->domain == 0) {
                printf("ERROR: All EQ filters of a set must be in the same domain\n");
                return(UNEXPECTED_ERROR);
            }
            else
            {
                eqFrameSizeSubband = eqSet->subbandFilter[g].eqFrameSizeSubband;
                coeffCount = eqSet->subbandFilter[g].coeffCount;
                offset = 0;
                for (i=0; i<eqFrameSizeSubband; i++)
                {
                    err = processSubbandFilter(&eqSet->subbandFilter[g], &audioIOBufferReal[offset], &audioIOBufferImag[offset]);
                    if (err) return (err);
                    offset += coeffCount;
                }
            }
        }
    }
    return (0);
}


#endif /* MPEG_D_DRC_EXTENSION_V1 */

