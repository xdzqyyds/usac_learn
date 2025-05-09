/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

Stefan Bayer               (Fraunhofer IIS)

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp.
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: usac_tw_tools.c,v 1.7 2011-04-11 15:28:38 mul Exp $
*******************************************************************************/

#include "vector_ops.h"
#include "usac_all.h"
#include "usac_interface.h"
#include "usac_allVariables.h"
#include "usac_mainStruct.h"
#include "usac_tw_tools.h"
#include "usac_tw_defines.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define CONST_PART_LEN_1024 1024
#define NSHORT 8

#define MIN_DX (1.0e-6)

#define NOTIME1 (-1.0e5)
#define NOTIME2 (-2.0e5)
#define BESSEL_EPSILON 1.0e-17

#define OS_FACTOR 128

#define IPLEN2 (TW_IPLEN2S*OS_FACTOR+1)
#define IPSIZE (IPLEN2+OS_FACTOR)

#ifndef TW_M_PI
#define TW_M_PI  3.14159265358979323846264338327950288
#endif

static float _tw_ratio_quant_table [1<<LEN_TW_RATIO] = {
    0.982857168f,
    0.988571405f,
    0.994285703f,
    1.0f,
    1.0057143f,
    1.01142859f,
    1.01714289f,
    1.02285719f,
};

extern int debug[];
static const double zero = 0.;
static const float  zerof = 0.;
static float _resamp_filter[IPSIZE];




static double Bessel(double x)
{
  register double p,s,ds,k;

  x *= 0.5;
  k = 0.0;
  p = s = 1.0;
  do {
    k += 1.0;
    p *= x/k;
    ds = p*p;
    s += ds;
  } while (ds>BESSEL_EPSILON*s);

  return s;
}


static void  init_resamp_filter(float *impResp)
{
   const float alpha = 8.;

  int i;
  double bAlpha;

  bAlpha = 1./Bessel(alpha);
  vcopy(&zerof,impResp,0,1,IPSIZE);
  impResp[0] = 1.;
  for(i=1;i<IPLEN2;i++) {
    if(i%OS_FACTOR)
      impResp[i] = (float)(
  Bessel(alpha*sqrt(1.0-(double)(i*i)/(float)(IPLEN2*IPLEN2)))*bAlpha
  *sin((double)i*TW_M_PI/(double)OS_FACTOR)
  /TW_M_PI/(double)i*(double)OS_FACTOR);
  }

}

static void w_ratio_dec(int    tw_data_present,
                        int   *tw_ratio,
                        float *ntwc,
                        int const mdctLen){

  int const twInterpDist = mdctLen / NUM_TW_NODES;
  float decWC[NUM_TW_NODES + 1];
  int i = 0;
  int j = 0;
  float d;

  for ( i = 0 ; i < NUM_TW_NODES + 1 ; i++ ) {
    decWC[i] = 1.0;
  }

  if (tw_data_present) {
    for ( i = 0 ; i < NUM_TW_NODES ; i++ ) {
      decWC[i+1] = decWC[i] * _tw_ratio_quant_table[tw_ratio[i]];
    }
  }

  for ( i = 0 ; i < NUM_TW_NODES ; i++ ) {
    d = (decWC[i+1] - decWC[i]) / (float) twInterpDist;
    for ( j = 0 ; j < twInterpDist ; j++ ) {
      ntwc[i * twInterpDist + j] = decWC[i] + (float) (j + 1) * d;
    }
  }

  return;
}



static float WarpTimeInv(const float *timeContour,const float tWarp, int const mdctLen)
{
  int i=0;
  float tmp1, tmp2;
  if(tWarp<timeContour[0])
    return(NOTIME1);
  if(tWarp>timeContour[3*mdctLen])
    return(NOTIME2);
  while(tWarp>timeContour[i+1])
    i++;
  tmp1 = tWarp-timeContour[i];
  tmp2 = timeContour[i+1]-timeContour[i];
  tmp1 /= tmp2;
  return ( ((float) i) + tmp1 ) ;
}



static void WarpInvVec(const float *timeContour,
                       const float tStart,
                       const int nSamples,
                       float *samplePos,
                       int const mdctLen)
{
  int i,j;
  float tWarp;

  tWarp = tStart;
  j = 0;
  while((i = (int)floor(WarpTimeInv(timeContour,tWarp-0.5f, mdctLen)))==(int)NOTIME1) {
    if(WarpTimeInv(timeContour,tWarp, mdctLen)==NOTIME2) {
      fprintf(stderr,"Error in WarpInvVec: no valid inv. time found!\n\n");
      exit(1);
    }
    tWarp += 1.;
    j++;
  }
  while(j<nSamples && (tWarp+.5)<timeContour[3*mdctLen]) {
    while(tWarp>timeContour[i+1])
      i++;
    samplePos[j] = (float)i+(tWarp-timeContour[i])
    /(timeContour[i+1]-timeContour[i]);
    j++;
    tWarp += 1.;
  }
}

static void tw_data_to_sp (int borderLen,
                      int mdctLen,
                      int frameWasTd,
                      const float *nextPitchContour,
                      WINDOW_SEQUENCE windowSequence,
                      float *warp_cont_mem,
                      float *lastPitchSum,
                      float *curPitchSum,
                      float *samplePos,
                      int *firstPos,
                      int *lastPos,
                      float *warpedTransLenLeft,
                      float *warpedTransLenRight)
{

  int const constPartLen = mdctLen;
  float nextPitchSum = 0.0f;
  float nf = 1.0f;
  float wRes = 1.0f;
  int   i=0;

  float timeContour[3 * CONST_PART_LEN_1024 + 1] = {0};
  float warpContour[3 * CONST_PART_LEN_1024 + 1] = {0};


  vcopy(warp_cont_mem,warpContour,1,1,2*constPartLen);

  nf = 1.0f/warpContour[2*constPartLen-1];
  for ( i = 0 ; i < 2*constPartLen ; i++ ) {
    warpContour[i] = warpContour[i] * nf;
  }
  *lastPitchSum *= nf;
  *curPitchSum *= nf;


  for ( i = 0 ; i < constPartLen ; i++ ) {
    nextPitchSum += nextPitchContour[i];
  }
  vcopy(nextPitchContour, warpContour + 2*constPartLen,1,1,constPartLen);
  vcopy(warpContour+constPartLen,warp_cont_mem,1,1,2*constPartLen);

  wRes = constPartLen / (*curPitchSum);
  timeContour[0] = -(*lastPitchSum)*wRes;
  for ( i = 0 ; i < 3*constPartLen ; i++ ) {
    timeContour[i+1] = timeContour[i] + warpContour[i]*wRes;
  }
  {
      float tStart = 0;
      int nSamples = 0;
      tStart = (float)( mdctLen - 3*mdctLen/2 - borderLen ) + .5f;
      nSamples = 2*mdctLen+2*borderLen;
      WarpInvVec(timeContour,tStart,
                 nSamples,
                 samplePos,
                 mdctLen);
  }

  *warpedTransLenLeft = (float)(constPartLen/2)
  *((*lastPitchSum) > (*curPitchSum) ? 1.0f : (*lastPitchSum)/(*curPitchSum));

  *warpedTransLenRight = (float)(constPartLen/2)
  *(nextPitchSum > (*curPitchSum) ? 1.0f : nextPitchSum/(*curPitchSum));

  switch(windowSequence) {
    case ONLY_LONG_SEQUENCE:
      break;
    case LONG_START_SEQUENCE:
      /**warpedTransLenRight /= (float)NSHORT;*/
      break;
    case LONG_STOP_SEQUENCE:
      if ( frameWasTd ) {
        *warpedTransLenLeft /= (float)(NSHORT/2);
      }
      else {
        *warpedTransLenLeft /= (float)NSHORT;
      }
      break;
    case EIGHT_SHORT_SEQUENCE:
      *warpedTransLenRight /= (float)NSHORT;
      *warpedTransLenLeft /= (float)NSHORT;
      break;
    case STOP_START_SEQUENCE:
      if ( frameWasTd ) {
        *warpedTransLenLeft /= (float)(NSHORT/2);
      }
      else {
        *warpedTransLenLeft /= (float)NSHORT;
      }
      break;
    default:
      fprintf(stderr,"Incorrect window type: %d!\n", windowSequence);
  }

  *firstPos = (int)ceil(0.5*(float)mdctLen-0.5-*warpedTransLenLeft);
  *lastPos = (int)floor(1.5*(float)mdctLen-0.5+*warpedTransLenRight);

  *lastPitchSum = *curPitchSum;
  *curPitchSum = nextPitchSum;

}

void tw_adjust_past ( WINDOW_SEQUENCE windowSequence,
                      WINDOW_SEQUENCE lastWindowSequence,
                      int frameIsTD,
                      int start_stop[],
                      float trans_len[],
                      int const mdctLen
                     )
{



   switch(lastWindowSequence) {
    case ONLY_LONG_SEQUENCE:
      break;
    case LONG_START_SEQUENCE:
      if ( frameIsTD ) {
        trans_len[1] /= (float)(NSHORT/2);
      }
      else {
        trans_len[1] /= (float)NSHORT;
      }
      break;
    case LONG_STOP_SEQUENCE:
       break;
    case EIGHT_SHORT_SEQUENCE:
      break;
    case STOP_START_SEQUENCE:
      if ( frameIsTD ) {
        trans_len[1] /= (float)(NSHORT/2);
      }
      else {
        trans_len[1] /= (float)NSHORT;
      }
      break;
    default:
      fprintf(stderr,"Incorrect window type: %d!\n", windowSequence);
  }

  start_stop[1] = (int)floor(1.5*(float)mdctLen-0.5+trans_len[1]);
}

void tw_reset(const int  frame_len,
              float     *tw_cont_mem,
              float     *pitch_sum ) {

  float tmp = 1.0f;

  pitch_sum[0]=pitch_sum[1]=(float) frame_len;
  vcopy(&tmp,tw_cont_mem,0,1,2*frame_len);

}


int tw_calc_tw (int        tw_data_present,
                int        frameWasTd,
                int        *tw_ratio,
                WINDOW_SEQUENCE windowSequence,
                float     *tw_cont_mem,
                float     *sample_pos,
                float     *tw_trans_len,
                int        *tw_start_stop,
                float     *pitch_sum,
                int const mdctLen) {

  float ntwc[CONST_PART_LEN_1024] = {0};

  memset(sample_pos,0,3*1024*sizeof(float));

  w_ratio_dec(tw_data_present,
              tw_ratio,
              ntwc,
              mdctLen);

  tw_data_to_sp(TW_IPLEN2S,
                mdctLen, /*MDCTLEN,*/
                frameWasTd,
                ntwc,
                windowSequence,
                tw_cont_mem,
                &pitch_sum[0],
                &pitch_sum[1],
                sample_pos,
                &tw_start_stop[0],
                &tw_start_stop[1],
                &tw_trans_len[0],
                &tw_trans_len[1]);

  return 0;

}

void tw_windowing_long(const float *wfIn,
                       float *wfOut,
                       int wStart,
                       int wEnd,
                       int nLong,
                       float warpedTransLenLeft,
                       float warpedTransLenRight,
                       const float *mdctWinTransLeft,
                       const float *mdctWinTransRight)
{
  int i;
  float trScale,trPos;

  vcopy(wfIn,wfOut,1,1,2*nLong);

  for(i=0;i<wStart;i++)
    wfOut[i] = 0.;
  for(i=wEnd+1;i<2*nLong;i++)
    wfOut[i] = 0.;

  trScale = 0.5f*(float)nLong/warpedTransLenLeft*(float)TW_OS_FACTOR_WIN;
  trPos = (warpedTransLenLeft+(float)(wStart-nLong/2)+0.5f)*trScale;

  for(i=nLong-1-wStart;i>=wStart;i--) {
    wfOut[i] = (float)wfOut[i]*mdctWinTransLeft[(int)floor(trPos)];
    trPos += trScale;
  }

}

void tw_windowing_past(const float *wfIn,
                       float *wfOut,
                       int wEnd,
                       int nLong,
                       float warpedTransLenRight,
                       const float *mdctWinTransRight)
{
  int i;
  float trScale,trPos;

  vcopy(wfIn,wfOut,1,1,2*nLong);


  trScale = 0.5f*(float)nLong/warpedTransLenRight*(float)TW_OS_FACTOR_WIN;

  trPos = (1.5f*(float)nLong-(float)wEnd-0.5f+warpedTransLenRight)*trScale;

  for(i=3*nLong-1-wEnd;i<=wEnd;i++) {
    wfOut[i] = (float)wfOut[i]*mdctWinTransRight[(int)floor(trPos)];
    trPos += trScale;
  }

  for(i=wEnd+1;i<2*nLong;i++)
    wfOut[i] = 0.0f;
}

void tw_windowing_short(const float *wfIn,
                                 float *wfOut,
                                 int wStart,
                                 int wEnd,
                                 float warpedTransLenLeft,
                                 float warpedTransLenRight,
                                 const float *mdctWinTransLeft,
                                 const float *mdctWinTransRight,
                                 int const mdctLenShort)
{
  int i, j, k;
  int const mdctLen = mdctLenShort * 8;
  float trScale1, trScale2 ,trPos1, trPos2;
  float *wtmp = (float*) calloc(2*mdctLenShort,sizeof(float));
  int offset = mdctLen-4*mdctLenShort-mdctLenShort/2;
  int mdctOffset = 0;
  int wEndShort;

  vcopy(wfIn+mdctOffset,wtmp,1,1,2*mdctLenShort);

  trScale1 = 0.5f*(float)mdctLen/warpedTransLenLeft*(float)TW_OS_FACTOR_WIN;
  trPos1 = (warpedTransLenLeft+(float)(wStart-mdctLen/2)+0.5f)*trScale1;
  trScale2 = NSHORT*TW_OS_FACTOR_WIN;
  trPos2 = trScale2/2;

  for ( i = 0 ; i < mdctLenShort ; i++ ) {
    wfOut[i+offset] = wtmp[i];
  }

  for(i=0;i<wStart;i++)
    wfOut[i] = 0.;

  for(i=mdctLen-1-wStart, j=mdctLenShort - 1;i>=wStart;i--, j--) {
    wfOut[i] *= mdctWinTransLeft[(int)floor(trPos1)];
    trPos1 += trScale1;
  }

  for(i=0;i<mdctLenShort;i++) {
    wfOut[offset + i + mdctLenShort] = wtmp[i + mdctLenShort]*mdctWinTransRight[(int) floor(trPos2)];
    trPos2 += trScale2;
  }

  offset += mdctLenShort;
  mdctOffset += 2*mdctLenShort;

  for ( k = 1 ; k < NSHORT - 1 ; k++ ) {
    vcopy(wfIn + mdctOffset , wtmp,1,1,2*mdctLenShort);
    trScale1 = NSHORT*TW_OS_FACTOR_WIN;
    trPos1 = trScale1/2;
    trPos2 = TW_OS_FACTOR_WIN*mdctLen-trPos1;
    for ( i = 0 ; i < mdctLenShort ; i++ ) {
      wfOut[i + offset] += wtmp[i]*mdctWinTransRight[(int) floor(trPos2)];
      wfOut[offset + mdctLenShort + i] = wtmp[mdctLenShort + i]*mdctWinTransRight[(int) floor(trPos1)];
      trPos1 += trScale1;
      trPos2 -= trScale1;
    }
    offset += mdctLenShort;
    mdctOffset += 2*mdctLenShort;
  }

  vcopy(wfIn + mdctOffset, wtmp,1,1,2*mdctLenShort);
  trScale1 = NSHORT*TW_OS_FACTOR_WIN;
  trPos1 = trScale1/2;

  for ( i = mdctLenShort - 1 ; i >= 0 ; i-- ) {
    wfOut[i + offset] += wtmp[i]*mdctWinTransRight[(int) floor(trPos1)];
    trPos1 += trScale1;
  }

  for ( i = 0 ; i < mdctLenShort ; i++ ) {
    wfOut[offset + mdctLenShort + i] = wtmp[mdctLenShort + i];
  }


  trScale2 = 0.5f*(float)mdctLen/warpedTransLenRight*(float)TW_OS_FACTOR_WIN;
  trPos2 = 0.5f*trScale2+0.5f;

  trPos2 = (1.5f*(float)mdctLen-(float)wEnd-0.5f+warpedTransLenRight)*trScale2;
  wEndShort = wEnd - offset;
  for(i=3*mdctLen-1-wEnd, j=3*mdctLenShort - wEndShort -1;i<=wEnd;i++, j++) {
    wfOut[i] *= mdctWinTransRight[(int)floor(trPos2)];
    trPos2 += trScale2;
  }

  for(i=wEnd+1;i<2*mdctLen;i++)
    wfOut[i] = 0.;

  free(wtmp);
}


int tw_resamp(const float *samplesIn,
              const int    startPos,
              int numSamplesIn,
              int numSamplesOut,
              const float *samplePos,
              float offsetPos,
              float *samplesOut)
{
  int i,j,k,jCenter;
  int fracTime;
  float tmp;

  jCenter = 0;
  for(i=startPos;i<numSamplesOut;i++) {
    tmp = 0.0;
    while(jCenter<numSamplesIn && samplePos[jCenter]-offsetPos<=(float)i)
      jCenter++;
    jCenter--;
    samplesOut[i] = 0.;
    if(jCenter<numSamplesIn-1 && jCenter>0) {
      fracTime = (int)floor(((float)i-(samplePos[jCenter]-offsetPos))
                            /(samplePos[jCenter+1]-samplePos[jCenter])
                            *(float)OS_FACTOR);
      if(fracTime>OS_FACTOR-1) {
        fprintf(stderr,"WARNING: fracTime>OS_FACTOR-1, jCenter=%d\n",jCenter);
        fracTime = OS_FACTOR-1;
      }
      j = TW_IPLEN2S*OS_FACTOR+fracTime;

      for(k=jCenter-TW_IPLEN2S;k<=jCenter+TW_IPLEN2S;k++) {
        if(k>=0 && k<numSamplesIn)
          tmp += _resamp_filter[abs(j)]* (float) samplesIn[k];
        j -= OS_FACTOR;
      }

    }
    if(jCenter<0)
      jCenter++;
    samplesOut[i] = tmp;
  }

  return 0;

}




void tw_init(void) {

  init_resamp_filter(_resamp_filter);

}
