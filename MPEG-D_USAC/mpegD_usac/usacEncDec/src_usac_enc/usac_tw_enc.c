/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:
Stefan Bayer      (Fraunhofer IIS)

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

*******************************************************************************/
#include <math.h>
#include "vector_ops.h"
#include "usac_tw_enc.h"
#include "usac_tw_tools.h"
#include "usac_tw_defines.h"


#define CONST_PART_LEN 1024
#define TW_INTERP_DIST (CONST_PART_LEN/NUM_TW_NODES)
#define MDCTLEN 1024
#define SHORT_MDCTLEN 128
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

/*#define SINUSOIDAL_PITCH*/

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
static const double zerod = 0.;
static const float  zerof = 0.;
static double _resamp_filter[IPSIZE];



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


static void  init_resamp_filter(double *impResp)
{
   const float alpha = 8.;

  int i;
  double bAlpha;

  bAlpha = 1./Bessel(alpha);
  vcopy_db(&zerod,impResp,0,1,IPSIZE);
  impResp[0] = 1.;
  for(i=1;i<IPLEN2;i++) {
    if(i%OS_FACTOR)
      impResp[i] = (double)
  Bessel(alpha*sqrt(1.-(double)(i*i)/(double)(IPLEN2*IPLEN2)))*bAlpha
  *sin((double)i*TW_M_PI/(double)OS_FACTOR)
  /TW_M_PI/(double)i*(double)OS_FACTOR;
  }

}
static void  quant_tw_ratio(float *pitch,
                            int   *common_tw,
                            int   *tw_data_present,
                            int   *tw_ratio) {
  float ratio;
  int i;

  *tw_data_present = 1;
  for ( i=0 ; i < NUM_TW_NODES ; i++ ) {
    tw_ratio[i] = 0;
    ratio = pitch[i+1]/pitch[i];
    while (ratio > _tw_ratio_quant_table[tw_ratio[i]]) {
      tw_ratio[i]++;
    }
    if ( tw_ratio[i] > (1<<LEN_TW_RATIO) - 1) {
      tw_ratio[i] = (1<<LEN_TW_RATIO) - 1;
    }
  }


}

void tw_windowing_long_enc(const double *wfIn,
                           double *wfOut,
                           int wStart,
                           int wEnd,
                           int nLong,
                           float warpedTransLenLeft,
                           float warpedTransLenRight,
                           const double *mdctWinTransLeft,
                           const double *mdctWinTransRight)
{
  int i;
  float trScale,trPos;

  vcopy_db(wfIn,wfOut,1,1,2*nLong);

  for(i=0;i<wStart;i++)
    wfOut[i] = 0.;
  for(i=wEnd+1;i<2*nLong;i++)
    wfOut[i] = 0.;

  trScale = 0.5f*(float)MDCTLEN/warpedTransLenLeft*(float)TW_OS_FACTOR_WIN;
  trPos = (warpedTransLenLeft+(float)(wStart-nLong/2)+0.5f)*trScale;

  for(i=nLong-1-wStart;i>=wStart;i--) {
    wfOut[i] = (double)(wfOut[i]*mdctWinTransLeft[(int)floor(trPos)]);
    trPos += trScale;
  }
  trScale = 0.5f*(float)MDCTLEN/warpedTransLenRight*(float)TW_OS_FACTOR_WIN;

  trPos = (1.5f*(float)nLong-(float)wEnd-0.5f+warpedTransLenRight)*trScale;

  for(i=3*nLong-1-wEnd;i<=wEnd;i++) {
    wfOut[i] = (double)(wfOut[i]*mdctWinTransRight[(int)floor(trPos)]);
    trPos += trScale;
  }
}

void tw_windowing_short_enc(const double *wfIn,
                            double *wfOut,
                            int wStart,
                            int wEnd,
                            float warpedTransLenLeft,
                            float warpedTransLenRight,
                            const double *mdctWinTransLeft,
                            const double *mdctWinTransRight) {

  int i,j,k;
  float trScale,trPos1, trPos2;
  double *wtmp;
  int offset = MDCTLEN-4*SHORT_MDCTLEN-SHORT_MDCTLEN/2;
  int outOffset = 0;
  int wStartShort, wEndShort;


  wtmp = (double*) calloc(2*MDCTLEN/NSHORT,sizeof(double));

  vcopy_db(&zerod,wtmp,0,1,2*MDCTLEN/NSHORT);

  wStartShort = wStart - offset;
  trScale = 0.5f*(float)MDCTLEN/warpedTransLenLeft*(float)TW_OS_FACTOR_WIN;
  trPos1 = (warpedTransLenLeft+(float)(wStart-MDCTLEN/2)+0.5f)*trScale;
  for(i=MDCTLEN-1-wStart, j=SHORT_MDCTLEN-1-wStartShort;i>=wStart;i--,j--) {
    wtmp[j] = wfIn[i]*mdctWinTransLeft[(int)floor(trPos1)];
    trPos1 += trScale;
  }


  vcopy_db(wfIn+MDCTLEN-wStart,wtmp+SHORT_MDCTLEN-wStartShort,1,1,wStartShort);

  trScale = NSHORT*TW_OS_FACTOR_WIN;
  trPos1 = trScale/2;
  trPos2 = TW_OS_FACTOR_WIN*MDCTLEN-trPos1;
  for(i=0;i<SHORT_MDCTLEN;i++) {
    wtmp[i+SHORT_MDCTLEN] = wfIn[i + offset + SHORT_MDCTLEN]*mdctWinTransRight[(int)floor(trPos1)];
    trPos1 += trScale;
  }

  vcopy_db(wtmp,wfOut+outOffset,1,1,2*SHORT_MDCTLEN);
  for ( k=1 ; k < NSHORT - 1 ; k++ ) {
    vcopy_db(&zerod,wtmp,0,1,2*MDCTLEN/NSHORT);
    outOffset = 2*k*SHORT_MDCTLEN;
    offset += SHORT_MDCTLEN;
    trScale = NSHORT*TW_OS_FACTOR_WIN;
    trPos1 = trScale/2;
    trPos2 = TW_OS_FACTOR_WIN*MDCTLEN-trPos1;
    for(i=0;i<SHORT_MDCTLEN;i++) {
      wtmp[i+SHORT_MDCTLEN] = wfIn[offset + i + SHORT_MDCTLEN]*mdctWinTransRight[(int) floor(trPos1)];
      wtmp[i] = wfIn[offset + i]*mdctWinTransRight[(int) floor(trPos2)];

      trPos2 -= trScale;
      trPos1 += trScale;
    }
    vcopy_db(wtmp,wfOut+outOffset,1,1,2*SHORT_MDCTLEN);
  }

  vcopy_db(&zerod,wtmp,0,1,2*MDCTLEN/NSHORT);
  offset += SHORT_MDCTLEN;
  outOffset = 2*(NSHORT-1)*SHORT_MDCTLEN;
  trScale = NSHORT*TW_OS_FACTOR_WIN;
  trPos1 = trScale/2;
  for(i=SHORT_MDCTLEN - 1 ; i>= 0 ;i--) {
    wtmp[i] = wfIn[offset + i]*mdctWinTransRight[(int) floor(trPos1)];

    trPos1 += trScale;
  }

  wEndShort = wEnd - offset;
  vcopy_db(wfIn + offset + SHORT_MDCTLEN,wtmp + SHORT_MDCTLEN,1,1,2*SHORT_MDCTLEN-wEndShort);

  trScale = 0.5f*(float)MDCTLEN/warpedTransLenRight*(float)TW_OS_FACTOR_WIN;
  trPos1 = (1.5f*(float)MDCTLEN-(float)wEnd-0.5f+warpedTransLenRight)*trScale;
  for(i=3*MDCTLEN-1-wEnd, j=3*SHORT_MDCTLEN-1-wEndShort;i<=wEnd; i++, j++) {
    wtmp[j] = wfIn[i]*mdctWinTransRight[(int)floor(trPos1)];
    trPos1 += trScale;
  }
  vcopy_db(wtmp,wfOut+outOffset,1,1,2*SHORT_MDCTLEN);
  free(wtmp);
}

void tw_resamp_enc(const double *samplesIn,
                   float offsetIn,
                   int numSamplesIn,
                   int numSamplesOut,
                   const float *samplePos,
                   double *samplesOut)
{
  int i,j,k;
  int intOffset,intTime,fracTime;
  float fracOffset,sampleTime;


  intOffset = (int)floor(offsetIn);
  fracOffset = offsetIn-(float)intOffset;

  for(i=0;i<numSamplesOut;i++) {
    samplesOut[i] = 0.;

    sampleTime = samplePos[i]-fracOffset;
    intTime = (int)floor(sampleTime);
    fracTime = (int)floor((sampleTime-(float)intTime)*(float)OS_FACTOR);

    j = -TW_IPLEN2S*OS_FACTOR+fracTime;

    for(k = intTime+TW_IPLEN2S+intOffset;k>=intTime-TW_IPLEN2S+intOffset;k--) {
      if(k>=0 && k<numSamplesIn)
        samplesOut[i] += (double)_resamp_filter[abs(j)]*samplesIn[k];
      j += OS_FACTOR;
    }

  }

}

void EncUsac_TwEncode (double           *timeSig,               /* time signal for pitch analysis */
                       int               frameWasTD,
                       WINDOW_SEQUENCE   windowSequence,
                       UsacToolsInfo    *toolsInfo,
                       float             warp_cont_mem[],
                       float             warp_sum[],
                       float             sample_pos[],   /* TW-MDCT sample positions */
                       float             tw_trans_len[],     /* TW-MDCT transition lengths */
                       int               tw_start_stop[],
                       int const mdctLen) {

  float pitch[NUM_TW_NODES+1];
  int i;

#ifndef SINUSOIDAL_PITCH
    for ( i = 0 ; i <= NUM_TW_NODES ; i++ ) {
      pitch[i] = 1.0f;
    }
#else
    for ( i = 0 ; i <= NUM_TW_NODES ; i++ ) {
      pitch[i] = 1.0f + 0.1f*sin(2*PI*i/NUM_TW_NODES);
    }
#endif


  /* quantize pitch contour */
  quant_tw_ratio(pitch,
              &toolsInfo->common_tw,
              &toolsInfo->tw_data_present,
              &toolsInfo->tw_ratio[0]);

  /* tw2data */
  tw_calc_tw(toolsInfo->tw_data_present,
             frameWasTD,
             toolsInfo->tw_ratio,
             windowSequence,
             warp_cont_mem,
             sample_pos,
             tw_trans_len,
             tw_start_stop,
             warp_sum,
             mdctLen);


}

void tw_init_enc(void) {

  init_resamp_filter(_resamp_filter);

}
