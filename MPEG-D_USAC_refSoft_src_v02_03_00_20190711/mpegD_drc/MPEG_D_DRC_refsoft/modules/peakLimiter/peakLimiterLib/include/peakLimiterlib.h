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


#ifndef __peakLimiter_h__
#define __peakLimiter_h__

#include "peakLimiterlib_errorcodes.h"

#define TDL_ATTACK_DEFAULT_MS      (5.0f)               /* default attack  time in ms */
#define TDL_RELEASE_DEFAULT_MS     (50.0f)              /* default release time in ms */

#ifdef __cplusplus
extern "C" {
#endif

struct TDLimiter;
typedef struct TDLimiter* TDLimiterPtr;

/******************************************************************************
* createLimiter                                                               *
* maxAttackMs:   maximum attack/lookahead time in milliseconds                *
* releaseMs:     release time in milliseconds (90% time constant)             *
* threshold:     limiting threshold                                           *
* maxChannels:   maximum number of channels                                   *
* maxSampleRate: maximum sampling rate in Hz                                  *
* returns:       limiter handle                                               *
******************************************************************************/
TDLimiterPtr createLimiter(float         maxAttackMs, 
                           float         releaseMs, 
                           float         threshold, 
                           unsigned int  maxChannels, 
                           unsigned int  maxSampleRate);



/******************************************************************************
* resetLimiter                                                                *
* limiter: limiter handle                                                     *
* returns: error code                                                         *
******************************************************************************/
int resetLimiter(TDLimiterPtr limiter);


/******************************************************************************
* destroyLimiter                                                              *
* limiter: limiter handle                                                     *
* returns: error code                                                         *
******************************************************************************/
int destroyLimiter(TDLimiterPtr limiter);

/******************************************************************************
* applyLimiter                                                                *
* limiter:  limiter handle                                                    *
* samples:  input/output buffer containing interleaved samples                *
* nSamples: number of samples per channel                                     *
* returns:  error code                                                        *
******************************************************************************/
int applyLimiter(TDLimiterPtr limiter, 
                 float*       samples, 
                 unsigned int nSamples);

/******************************************************************************
* getLimiterDelay                                                             *
* limiter: limiter handle                                                     *
* returns: exact delay caused by the limiter in samples                       *
******************************************************************************/
unsigned int getLimiterDelay(TDLimiterPtr limiter);

/******************************************************************************
* getLimiterMaxGainReduction                                                  *
* limiter: limiter handle                                                     *
* returns: maximum gain reduction in last processed block in dB               *
******************************************************************************/
float getLimiterMaxGainReduction(TDLimiterPtr limiter);

/******************************************************************************
* setLimiterNChannels                                                         *
* limiter:   limiter handle                                                   *
* nChannels: number of channels ( <= maxChannels specified on create)         *
* returns:   error code                                                       *
******************************************************************************/
int setLimiterNChannels(TDLimiterPtr limiter, unsigned int nChannels);

/******************************************************************************
* setLimiterSampleRate                                                        *
* limiter:    limiter handle                                                  *
* sampleRate: sampling rate in Hz ( <= maxSampleRate specified on create)     *
* returns:    error code                                                      *
******************************************************************************/
int setLimiterSampleRate(TDLimiterPtr limiter, unsigned int sampleRate);

/******************************************************************************
* setLimiterAttack                                                            *
* limiter:    limiter handle                                                  *
* attackMs:   attack time in ms ( <= maxAttackMs specified on create)         *
* returns:    error code                                                      *
******************************************************************************/
int setLimiterAttack(TDLimiterPtr limiter, float attackMs);

/******************************************************************************
* setLimiterRelease                                                           *
* limiter:    limiter handle                                                  *
* releaseMs:  release time in ms                                              *
* returns:    error code                                                      *
******************************************************************************/
int setLimiterRelease(TDLimiterPtr limiter, float releaseMs);

/******************************************************************************
* setLimiterThreshold                                                         *
* limiter:    limiter handle                                                  *
* threshold:  limiter threshold                                               *
* returns:    error code                                                      *
******************************************************************************/
int setLimiterThreshold(TDLimiterPtr limiter, float threshold);

/******************************************************************************
* setLimiterActive                                                            *
* limiter:    limiter handle                                                  *
* isActive:   1: activate limiter (default)                                   *
*             0: delay only (if peak<threshold is known in advance)           *
* returns:    error code                                                      *
******************************************************************************/
int setLimiterActive(TDLimiterPtr limiter, int isActive);


#ifdef __cplusplus
}
#endif

#endif
