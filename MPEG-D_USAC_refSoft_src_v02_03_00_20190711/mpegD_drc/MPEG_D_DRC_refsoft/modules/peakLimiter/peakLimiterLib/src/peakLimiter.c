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


#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "peakLimiterlib.h"

#ifndef max
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#endif

struct TDLimiter {
  unsigned int  attack;
  float         attackConst, releaseConst;
  float         attackMs, releaseMs, maxAttackMs;
  float         threshold;
  unsigned int  channels, maxChannels;
  unsigned int  sampleRate, maxSampleRate;
  float         cor;
  float*        maxBuf;
  float*        maxBufSlow;
  float*        delayBuf;
  unsigned int  maxBufIdx, maxBufSlowIdx, delayBufIdx;
  unsigned int  secLen, nMaxBufSec;
  unsigned int  maxBufSecIdx, maxBufSecCtr;
  double        smoothState0;
  float         minGain;
  int           isActive;
};

/* create limiter */
TDLimiterPtr createLimiter(
                           float         maxAttackMs, 
                           float         releaseMs, 
                           float         threshold, 
                           unsigned int  maxChannels, 
                           unsigned int  maxSampleRate
                           )
{
  TDLimiterPtr limiter = NULL;
  unsigned int attack, secLen;

  /* calc attack time in samples */
  attack = (unsigned int)(maxAttackMs * maxSampleRate / 1000);

  if (attack < 1) /* attack time is too short */
    return NULL; 

  /* length of maxBuf sections */
  secLen = (unsigned int)sqrt(attack+1);
  /* sqrt(attack+1) leads to the minimum 
     of the number of maximum operators:
     nMaxOp = secLen + (attack+1)/secLen */

  /* alloc limiter struct */
  limiter = (TDLimiterPtr)calloc(1, sizeof(struct TDLimiter));
  if (!limiter) return NULL;

  limiter->secLen = secLen;
  limiter->nMaxBufSec = (attack+1)/secLen;
  if (limiter->nMaxBufSec*secLen < (attack+1))
    limiter->nMaxBufSec++; /* create a full section for the last samples */

  /* alloc maximum and delay buffers */
  limiter->maxBuf   = (float*)calloc(limiter->nMaxBufSec * secLen, sizeof(float));
  limiter->delayBuf = (float*)calloc(attack * maxChannels, sizeof(float));
  limiter->maxBufSlow = (float*)calloc(limiter->nMaxBufSec, sizeof(float));

  if (!limiter->maxBuf || !limiter->delayBuf || !limiter->maxBufSlow) {
    destroyLimiter(limiter);
    return NULL;
  }

  /* init parameters & states */
  limiter->maxBufIdx = 0;
  limiter->delayBufIdx = 0;
  limiter->maxBufSlowIdx = 0;
  limiter->maxBufSecIdx = 0;
  limiter->maxBufSecCtr = 0;

  limiter->attackMs      = maxAttackMs;
  limiter->maxAttackMs   = maxAttackMs;
  limiter->releaseMs     = releaseMs;
  limiter->attack        = attack;
  limiter->attackConst   = (float)pow(0.1, 1.0 / (attack + 1));
  limiter->releaseConst  = (float)pow(0.1, 1.0 / (releaseMs * maxSampleRate / 1000 + 1));
  limiter->threshold     = threshold;
  limiter->channels      = maxChannels;
  limiter->maxChannels   = maxChannels;
  limiter->sampleRate    = maxSampleRate;
  limiter->maxSampleRate = maxSampleRate;

  limiter->cor = 1.0f;
  limiter->smoothState0 = 1.0;
  limiter->minGain = 1.0f;

  limiter->isActive = 1;

  return limiter;
}


/* reset limiter */
int resetLimiter(TDLimiterPtr limiter)
{
  if (limiter) {

    limiter->maxBufIdx = 0;
    limiter->delayBufIdx = 0;
    limiter->maxBufSlowIdx = 0;
    limiter->maxBufSecIdx = 0;
    limiter->maxBufSecCtr = 0;
    limiter->cor = 1.0f;
    limiter->smoothState0 = 1.0;
    limiter->minGain = 1.0f;

    memset(limiter->maxBuf,     0, (limiter->attack + 1) * sizeof(float) );
    memset(limiter->delayBuf,   0, limiter->attack * limiter->channels * sizeof(float) );
    memset(limiter->maxBufSlow, 0, limiter->nMaxBufSec * sizeof(float) );

  }
  
  return TDLIMIT_OK;
}


/* destroy limiter */
int destroyLimiter(TDLimiterPtr limiter)
{
  if (limiter) {
    free(limiter->maxBuf);
    free(limiter->delayBuf);
    free(limiter->maxBufSlow);

    free(limiter);
  }
  return TDLIMIT_OK;
}

/* apply limiter */
int applyLimiter(TDLimiterPtr limiter, float *samples, unsigned int nSamples)
{
  unsigned int i, j;
  float tmp, gain;
  float minGain = 1;

  if ( limiter == NULL ) return TDLIMIT_INVALID_HANDLE;

  {
    unsigned int channels       = limiter->channels;
    unsigned int attack         = limiter->attack;
    float        attackConst    = limiter->attackConst;
    float        releaseConst   = limiter->releaseConst;
    float        threshold      = limiter->threshold;

    float*       maxBuf         = limiter->maxBuf;
    unsigned int maxBufIdx      = limiter->maxBufIdx;
    float*       maxBufSlow     = limiter->maxBufSlow;
    unsigned int maxBufSlowIdx  = limiter->maxBufSlowIdx;
    unsigned int maxBufSecIdx   = limiter->maxBufSecIdx;
    unsigned int maxBufSecCtr   = limiter->maxBufSecCtr;
    unsigned int secLen         = limiter->secLen;
    unsigned int nMaxBufSec     = limiter->nMaxBufSec;
    float        cor            = limiter->cor;
    float*       delayBuf       = limiter->delayBuf;
    unsigned int delayBufIdx    = limiter->delayBufIdx;

    double       smoothState0   = limiter->smoothState0;
    float maximum, sectionMaximum;

    if (limiter->isActive || (float)smoothState0 < 1.0f) {
      for (i = 0; i < nSamples; i++) {
        /* get maximum absolute sample value of all channels */
        tmp = threshold;
        for (j = 0; j < channels; j++) {
          tmp = max(tmp, (float)fabs(samples[i * channels + j]));
        }
        
        /* running maximum over attack+1 samples */
        maxBuf[maxBufIdx] = tmp;

        /* search section of maxBuf */
        sectionMaximum = maxBuf[maxBufSecIdx];
        for (j = 1; j < secLen; j++) {
          if (maxBuf[maxBufSecIdx+j] > sectionMaximum) sectionMaximum = maxBuf[maxBufSecIdx+j];
        }

        /* find maximum of slow (downsampled) max buffer */
        maximum = sectionMaximum;
        for (j = 0; j < nMaxBufSec; j++) {
          if (maxBufSlow[j] > maximum) maximum = maxBufSlow[j];
        }

        maxBufIdx++;
        maxBufSecCtr++;

        /* if maxBuf section is finished, or end of maxBuf is reached,
           store the maximum of this section and open up a new one */
        if ((maxBufSecCtr >= secLen)||(maxBufIdx >= attack+1)) {
          maxBufSecCtr = 0;

          maxBufSlow[maxBufSlowIdx] = sectionMaximum;
          maxBufSlowIdx++;
          if (maxBufSlowIdx >= nMaxBufSec)
            maxBufSlowIdx = 0;
          maxBufSlow[maxBufSlowIdx] = 0.0f;  /* zero out the value representing the new section */

          maxBufSecIdx += secLen;
        }

        if (maxBufIdx >= (attack+1)) {
          maxBufIdx = 0;
          maxBufSecIdx = 0;
        }

        /* calc gain */
        if (maximum > threshold) {
          gain = threshold / maximum;
        }
        else {
          gain = 1;
        }

        /* gain smoothing */
        /* first order IIR filter with attack correction to avoid overshoots */
        
        /* correct the 'aiming' value of the exponential attack to avoid the remaining overshoot */
        if (gain < smoothState0) {
          cor = min(cor, (gain - 0.1f * (float)smoothState0) * 1.11111111f);
        }
        else {
          cor = gain;
        }
        
        /* smoothing filter */
        if (cor < smoothState0) {
          smoothState0 = attackConst * (smoothState0 - cor) + cor;  /* attack */
          smoothState0 = max(smoothState0, gain); /* avoid overshooting target */
        }
        else {
          smoothState0 = releaseConst * (smoothState0 - cor) + cor; /* release */
        }
        
        gain = (float)smoothState0;


        /* lookahead delay, apply gain */
        for (j = 0; j < channels; j++) {
          tmp = delayBuf[delayBufIdx * channels + j];
          delayBuf[delayBufIdx * channels + j] = samples[i * channels + j];

          tmp *= gain;
          if (tmp > threshold) tmp = threshold;
          if (tmp < -threshold) tmp = -threshold;

          samples[i * channels + j] = tmp;
        }

        delayBufIdx++;
        if (delayBufIdx >= attack) delayBufIdx = 0;

        /* save minimum gain factor */
        if (gain < minGain) minGain = gain;
      }
    }
    else { /* !(limiter->isActive || (float)smoothState0 < 1.0f): limiter not active, only delay */
      for (i = 0; i < nSamples; i++) {
        for (j = 0; j < channels; j++) {
          tmp = delayBuf[delayBufIdx * channels + j];
          delayBuf[delayBufIdx * channels + j] = samples[i * channels + j];
          samples[i * channels + j] = tmp;
        }

        delayBufIdx++;
        if (delayBufIdx >= attack) delayBufIdx = 0;
      }
    }

    limiter->maxBufIdx = maxBufIdx;

    limiter->maxBufSecIdx = maxBufSecIdx;
    limiter->maxBufSecCtr = maxBufSecCtr;
    limiter->maxBufSlowIdx = maxBufSlowIdx;

    limiter->cor = cor;
    limiter->delayBufIdx = delayBufIdx;

    limiter->smoothState0 = smoothState0;

    limiter->minGain = minGain;

    return TDLIMIT_OK;
  }
}

/* get delay in samples */
unsigned int getLimiterDelay(TDLimiterPtr limiter)
{
  return limiter->attack;
}

/* get maximum gain reduction of last processed block */
float getLimiterMaxGainReduction(TDLimiterPtr limiter)
{
  return -20 * (float)log10(limiter->minGain);
}

/* set number of channels */
int setLimiterNChannels(TDLimiterPtr limiter, unsigned int nChannels)
{
  if (nChannels > limiter->maxChannels) return TDLIMIT_INVALID_PARAMETER;

  limiter->channels = nChannels;
  resetLimiter(limiter);

  return TDLIMIT_OK;
}

/* set sampling rate */
int setLimiterSampleRate(TDLimiterPtr limiter, unsigned int sampleRate)
{
  unsigned int attack, secLen;
  if (sampleRate > limiter->maxSampleRate) return TDLIMIT_INVALID_PARAMETER;

  /* update attack/release constants */
  attack = (unsigned int)(limiter->attackMs * sampleRate / 1000);

  if (attack < 1) /* attack time is too short */
    return TDLIMIT_INVALID_PARAMETER; 

  /* length of maxBuf sections */
  secLen = (unsigned int)sqrt(attack+1);

  limiter->attack        = attack;
  limiter->secLen        = secLen;
  limiter->nMaxBufSec    = (attack+1)/secLen;
  if (limiter->nMaxBufSec*secLen < (attack+1))
    limiter->nMaxBufSec++;
  limiter->attackConst   = (float)pow(0.1, 1.0 / (attack + 1));
  limiter->releaseConst  = (float)pow(0.1, 1.0 / (limiter->releaseMs * sampleRate / 1000 + 1));
  limiter->sampleRate    = sampleRate;

  /* reset */
  resetLimiter(limiter);

  return TDLIMIT_OK;
}

/* set attack time */
int setLimiterAttack(TDLimiterPtr limiter, float attackMs)
{
  unsigned int attack, secLen;
  if (attackMs > limiter->maxAttackMs) return TDLIMIT_INVALID_PARAMETER;

  /* calculate attack time in samples */
  attack = (unsigned int)(attackMs * limiter->sampleRate / 1000);

  if (attack < 1) /* attack time is too short */
    return TDLIMIT_INVALID_PARAMETER; 

  /* length of maxBuf sections */
  secLen = (unsigned int)sqrt(attack+1);

  limiter->attack       = attack;
  limiter->secLen       = secLen;
  limiter->nMaxBufSec   = (attack+1)/secLen;
  if (limiter->nMaxBufSec*secLen < (attack+1))
    limiter->nMaxBufSec++;
  limiter->attackConst  = (float)pow(0.1, 1.0 / (attack + 1));
  limiter->attackMs     = attackMs;

  /* reset */
  resetLimiter(limiter);

  return TDLIMIT_OK;
}

/* set release time */
int setLimiterRelease(TDLimiterPtr limiter, float releaseMs)
{ 
  limiter->releaseConst = (float)pow(0.1, 1.0 / (releaseMs * limiter->sampleRate / 1000 + 1));
  limiter->releaseMs = releaseMs;

  return TDLIMIT_OK;
}

/* set limiter threshold */
int setLimiterThreshold(TDLimiterPtr limiter, float threshold)
{
  limiter->threshold = threshold;

  return TDLIMIT_OK;
}

/* set limiter active or delay only */
int setLimiterActive(TDLimiterPtr limiter, int isActive)
{
  limiter->isActive = isActive;

  return TDLIMIT_OK;
}


