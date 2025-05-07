/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "sac_dec.h"
#include "sac_hrtf.h"
#include "sac_sbrconst.h"
#include "sac_polyphase.h"
#include "sac_hybfilter.h"
#include "sac_calcM1andM2.h"

#ifndef min
#define min(x, y)   ( ((x) < (y) ) ? (x) : (y) )
#endif

#define PROTOTYPE_SLOTS                 (3)
#define PROTOTYPE_SLOTS_IS_EVEN         (1 - PROTOTYPE_SLOTS % 2)
#define KMAX                            (PROTOTYPE_SLOTS / 4 + 10 / 2 - 1)
#define V_SLOTS                         (KMAX * 2 + 1)
#define LAMBDA                          (0.0f)
#define MAX_IMPULSE_LAG                 (50)
#define MAX_PROTOTYPE_LENGTH            (PROTOTYPE_SLOTS * MAX_NUM_QMF_BANDS)
#define MAX_V_LENGTH                    (V_SLOTS * MAX_NUM_QMF_BANDS)
#define MAX_V_LENGTH_EXT                (MAX_V_LENGTH + MAX_PROTOTYPE_LENGTH - 1)
#define MAX_IMPULSE_LENGTH              (2048)
#define MAX_MASK_SLOTS                  (40)
#define MAX_GAIN                        (10.0f)
#undef ABS_THR
#define ABS_THR                         (1e-9f)
#define INPUT_CHANNELS                  (2)
#define OUTPUT_CHANNELS                 (2)





struct HRTF_FILTERBANK
{
  int qmfBands;
  int hybridBands;
  int maskSlots;

  float surroundGain;

  float maskLeftReal[HRTF_AZIMUTHS][MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS];
  float maskLeftImag[HRTF_AZIMUTHS][MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS];
  float maskRightReal[HRTF_AZIMUTHS][MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS];
  float maskRightImag[HRTF_AZIMUTHS][MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS];

  int tauLfLsLeft;
  int tauLfLsRight;
  int tauRfRsLeft;
  int tauRfRsRight;

  int filterSlot[MAX_PARAMETER_SETS + 1];
  int filterSets;

  float filterReal[MAX_PARAMETER_SETS + 1][MAX_HYBRID_BANDS][OUTPUT_CHANNELS][2*INPUT_CHANNELS+MAX_NUM_TTT][MAX_MASK_SLOTS];
  float filterImag[MAX_PARAMETER_SETS + 1][MAX_HYBRID_BANDS][OUTPUT_CHANNELS][2*INPUT_CHANNELS+MAX_NUM_TTT][MAX_MASK_SLOTS];

  float filterStateReal[MAX_HYBRID_BANDS][OUTPUT_CHANNELS][2*INPUT_CHANNELS][MAX_MASK_SLOTS];
  float filterStateImag[MAX_HYBRID_BANDS][OUTPUT_CHANNELS][2*INPUT_CHANNELS][MAX_MASK_SLOTS];
};


static void InvertMatrix(int length,
                         float x[MAX_PROTOTYPE_LENGTH][MAX_PROTOTYPE_LENGTH]);

static void SolveSystem(int prototypeLength,
                        int vextLength,
                        float v[MAX_PROTOTYPE_LENGTH][MAX_V_LENGTH_EXT],
                        float r[MAX_V_LENGTH_EXT],
                        float q[MAX_PROTOTYPE_LENGTH]);

static void GetPrototype(int qmfBands,
                         float *prototype);

static int GetQmfMaskLength(int impulseLength,
                            int qmfBands);

static void ConvertImpulseToQmfMask(HRTF_FILTERBANK *self,
                                    float const *impulse,
                                    int impulseLength,
                                    float *prototype,
                                    float maskReal[MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS],
                                    float maskImag[MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS]);

static int GetImpulseDelayDifference(float const *impulse1,
                                     float const *impulse2,
                                     int impulseLength);

static void CombineFilters(HRTF_FILTERBANK *self,
                           float mask1Real[MAX_MASK_SLOTS],
                           float mask1Imag[MAX_MASK_SLOTS],
                           float mask2Real[MAX_MASK_SLOTS],
                           float mask2Imag[MAX_MASK_SLOTS],
                           int qmfBand,
                           float weight1,
                           float weight2,
                           float *phi_old,
                           int tau,
                           float combinedReal[MAX_MASK_SLOTS],
                           float combinedImag[MAX_MASK_SLOTS]);


static void InvertMatrix(int length,
                         float x[MAX_PROTOTYPE_LENGTH][MAX_PROTOTYPE_LENGTH])
{
  float t[MAX_PROTOTYPE_LENGTH][2 * MAX_PROTOTYPE_LENGTH];
  float temp;
  int i;
  int j;
  int k;

  for (j = 0; j < length; j++) {
    for (i = 0; i < length; i++) {
      t[j][i] = x[j][i];
      t[j][length + i] = (i == j)? 1.0f: 0.0f;
    }
  }

  for (k = 0; k < length; k++) {
    temp = t[k][k];
    assert(temp != 0.0f);

    for (i = k; i < 2 * length; i++) {
      t[k][i] /= temp;
    }

    for (j = 0; j < length; j++) {
      if ((j != k) && (t[j][k] != 0.0f)) {
        temp = t[j][k];

        for (i = k; i < 2 * length; i++) {
          t[j][i] -= temp * t[k][i];
        }
      }
    }
  }

  for (j = 0; j < length; j++) {
    for (i = 0; i < length; i++) {
      x[j][i] = t[j][length + i];
    }
  }
}


static void SolveSystem(int prototypeLength,
                        int vextLength,
                        float v[MAX_PROTOTYPE_LENGTH][MAX_V_LENGTH_EXT],
                        float r[MAX_V_LENGTH_EXT],
                        float q[MAX_PROTOTYPE_LENGTH])
{
  float vtv[MAX_PROTOTYPE_LENGTH][MAX_PROTOTYPE_LENGTH];
  float vtvvt[MAX_PROTOTYPE_LENGTH][MAX_V_LENGTH_EXT];
  int i;
  int j;
  int k;

  for (k = 0; k < prototypeLength; k++) {
    for (j = 0; j < prototypeLength; j++) {
      vtv[k][j] = 0.0f;

      for (i = 0; i < vextLength; i++) {
        vtv[k][j] += v[k][i] * v[j][i];
      }
    }
  }

  InvertMatrix(prototypeLength, vtv);

  for (k = 0; k < prototypeLength; k++) {
    for (j = 0; j < vextLength; j++) {
      vtvvt[k][j] = 0.0f;

      for (i = 0; i < prototypeLength; i++) {
        vtvvt[k][j] += vtv[k][i] * v[i][j];
      }
    }
  }

  for (j = 0; j < prototypeLength; j++) {
    q[j] = 0.0f;

    for (i = 0; i < vextLength; i++) {
      q[j] += vtvvt[j][i] * r[i];
    }
  }
}


static void GetPrototype(int qmfBands,
                         float *prototype)
{
  float qmfPrototype[10 * MAX_NUM_QMF_BANDS];
  float extPrototype[2 * MAX_PROTOTYPE_LENGTH + 20 * MAX_NUM_QMF_BANDS - 1];
  float v[MAX_PROTOTYPE_LENGTH][MAX_V_LENGTH_EXT];
  float r[MAX_V_LENGTH_EXT];
  int offset1;
  int offset2;
  int length;
  int i;
  int j;
  int k;

  memset(extPrototype, 0, sizeof(extPrototype));
  memset(v, 0, sizeof(v));
  memset(r, 0, sizeof(r));

  SacGetFilterbankPrototype(qmfBands, qmfPrototype);

  
  offset1 = qmfBands * PROTOTYPE_SLOTS;
  offset2 = qmfBands * 20 - 2 + offset1;
  length  = qmfBands * 10 - 1;

  for (i = 0; i <= length; i++) {
    for (j = 0, k = length - i; j <= i; j++, k++) {
      extPrototype[offset1 + i] += qmfPrototype[j] * qmfPrototype[k];
    }
    extPrototype[offset2 - i] = extPrototype[offset1 + i];
  }

  
  offset1 = qmfBands * PROTOTYPE_SLOTS / 2               - qmfBands * (PROTOTYPE_SLOTS / 2) - qmfBands / 2;
  offset2 = qmfBands * (PROTOTYPE_SLOTS + 10 + 2 * KMAX) - qmfBands * (PROTOTYPE_SLOTS / 2) - qmfBands / 2;

  for (i = 0; i < V_SLOTS; i++) {
    for (j = 0; j < qmfBands; j++) {
      for (k = 0; k < PROTOTYPE_SLOTS; k++) {
        v[k * qmfBands + offset1 + j][i * qmfBands + j] = extPrototype[(k - 2 * i) * qmfBands + offset2 + j];
      }
    }
  }

  
  offset1 = qmfBands * V_SLOTS;
  length  = qmfBands * PROTOTYPE_SLOTS - 1;

  for (i = 0; i < length; i++) {
    v[i    ][offset1 + i] = LAMBDA;
    v[i + 1][offset1 + i] = -LAMBDA;
  }

  
  offset1 = qmfBands * KMAX;

  for (i = 0; i < qmfBands; i++) {
    r[offset1 + i] = (float) qmfBands;
  }

  
  offset1 = qmfBands * PROTOTYPE_SLOTS;
  length  = qmfBands * (V_SLOTS + PROTOTYPE_SLOTS) - 1;

  SolveSystem(offset1, length, v, r, prototype);
}


static int GetQmfMaskLength(int impulseLength,
                            int qmfBands)
{
  int impulseSlots;
  int maskSlots;

  impulseSlots = (int) ceil(impulseLength / (float) qmfBands);
  maskSlots = impulseSlots + PROTOTYPE_SLOTS - 1;

  assert((impulseSlots * qmfBands) <= MAX_IMPULSE_LENGTH);
  assert(maskSlots <= MAX_MASK_SLOTS);

  return maskSlots;
}


static void ConvertImpulseToQmfMask(HRTF_FILTERBANK *self,
                                    float const *impulse,
                                    int impulseLength,
                                    float *prototype,
                                    float maskReal[MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS],
                                    float maskImag[MAX_NUM_QMF_BANDS][MAX_MASK_SLOTS])
{
  float modulationReal[MAX_NUM_QMF_BANDS][MAX_PROTOTYPE_LENGTH];
  float modulationImag[MAX_NUM_QMF_BANDS][MAX_PROTOTYPE_LENGTH];
  float extImpulse[MAX_IMPULSE_LENGTH + 2 * MAX_PROTOTYPE_LENGTH];
  float modImpulse[MAX_PROTOTYPE_LENGTH];
  float temp;
  int offset;
  int i;
  int j;
  int k;

  
  memset(extImpulse, 0, sizeof(extImpulse));

  offset = self->qmfBands * (PROTOTYPE_SLOTS - 1);

  for (i = 0; i < impulseLength; i++) {
    extImpulse[offset + i] = impulse[i];
  }

  
  offset = self->qmfBands * PROTOTYPE_SLOTS / 2 - 1;

  for (j = 0; j < MAX_NUM_QMF_BANDS; j++) {
    for (i = 0; i < PROTOTYPE_SLOTS * self->qmfBands; i++) {
      temp = -PI / self->qmfBands * (j + 0.5f) * (i - offset);
      modulationReal[j][i] = (float) cos(temp);
      modulationImag[j][i] = (float) sin(temp);
    }
  }

  
  for (k = 0; k < self->maskSlots; k++) {
    offset = self->qmfBands * k;

    for (i = 0; i < PROTOTYPE_SLOTS * self->qmfBands; i++) {
      modImpulse[i] = extImpulse[offset + i] * prototype[i];
    }

    for (j = 0; j < MAX_NUM_QMF_BANDS; j++) {
      maskReal[j][k] = 0.0f;
      maskImag[j][k] = 0.0f;

      for (i = 0; i < PROTOTYPE_SLOTS * self->qmfBands; i++) {
        maskReal[j][k] += modulationReal[j][i] * modImpulse[i];
        maskImag[j][k] += modulationImag[j][i] * modImpulse[i];
      }
    }
  }
}


static int GetImpulseDelayDifference(float const *impulse1,
                                     float const *impulse2,
                                     int impulseLength)
{
  int maxPowerIndex = 0;
  float maxPower = 0.0f;
  float power;
  int i;
  int j;
  int k;

  assert(impulseLength >= MAX_IMPULSE_LAG);

  for (k = 0; k <= MAX_IMPULSE_LAG; k++) {
    power = 0.0f;

    for (i = 0, j = MAX_IMPULSE_LAG - k; j < impulseLength; i++, j++) {
      power += impulse1[i] * impulse2[j];
    }

    if (maxPower < power) {
      maxPower = power;
      maxPowerIndex =  MAX_IMPULSE_LAG - k;
    }
  }

  for (k = 0; k < MAX_IMPULSE_LAG; k++) {
    power = 0.0f;

    for (i = k + 1, j = 0; i < impulseLength; i++, j++) {
      power += impulse1[i] * impulse2[j];
    }

    if (maxPower < power) {
      maxPower = power;
      maxPowerIndex = -(k + 1);
    }
  }

  return maxPowerIndex;
}


static void CombineFilters(HRTF_FILTERBANK *self,
                           float mask1Real[MAX_MASK_SLOTS],
                           float mask1Imag[MAX_MASK_SLOTS],
                           float mask2Real[MAX_MASK_SLOTS],
                           float mask2Imag[MAX_MASK_SLOTS],
                           int qmfBand,
                           float weight1,
                           float weight2,
                           float *phi_old,
                           int tau,
                           float combinedReal[MAX_MASK_SLOTS],
                           float combinedImag[MAX_MASK_SLOTS])
{
  float w1tau;
  float w2tau;
  float t1Real;
  float t1Imag;
  float t2Real;
  float t2Imag;
  int i;
  float crossNrgReal = 0;
  float crossNrgImag = 0;
  float norm1 = 0;
  float norm2 = 0;
  float Etarget;
  float Eint;
  float Gcomp;

  float RReal, RImag;
  float cfb, phi, icc, a1, a2;
  float phiFract, phiInt;

  for (i = 0; i < self->maskSlots; i++) {
    norm1 += mask1Real[i] * mask1Real[i] + mask1Imag[i] * mask1Imag[i];
    norm2 += mask2Real[i] * mask2Real[i] + mask2Imag[i] * mask2Imag[i];
    crossNrgReal += mask1Real[i] * mask2Real[i] + mask1Imag[i] * mask2Imag[i];
    crossNrgImag += -mask1Real[i] * mask2Imag[i] + mask2Real[i] * mask1Imag[i];
  }

  RReal = (float)(crossNrgReal/(sqrt(norm1*norm2)+ABS_THR));
  RImag = (float)(crossNrgImag/(sqrt(norm1*norm2)+ABS_THR));

  cfb = (float)(sqrt(norm1)/(sqrt(norm2) + ABS_THR));
  phi = (float)atan2(RImag, RReal);
  if (qmfBand==0){
    *phi_old = phi;
  }


  phiInt = (float)(int)((*phi_old)/(2*PI));
  phiFract = *phi_old-2*PI*phiInt;

  if (phiFract > PI){
    phiFract -= 2*PI;
    phiInt += 1;
  }

  if (fabs(phi-phiFract) > fabs(2*PI+phi-phiFract)){
    phi += 2*PI;
  }
  if (fabs(phi-phiFract) > fabs(-2*PI+phi-phiFract)){
    phi -= 2*PI;
  }
  phi += phiInt*2*PI;

  *phi_old = phi;
  icc = (float)sqrt(RReal*RReal+RImag*RImag);

  w1tau = weight1/(weight1+weight2*(self->surroundGain*self->surroundGain)+ABS_THR);
  w2tau = 1-w1tau;
  a1 = (float)sqrt(weight1);
  a2 = (float)sqrt(weight2);

  t1Real = (float)(a1*cos(phi*w2tau));
  t1Imag = (float)(-a1*sin(phi*w2tau));

  t2Real = (float)(a2*cos(phi*w1tau)*self->surroundGain);
  t2Imag = (float)(a2*sin(phi*w1tau)*self->surroundGain);

  Etarget   = weight1*cfb*cfb+weight2*(self->surroundGain*self->surroundGain);
  Eint      = weight1*cfb*cfb+weight2*(self->surroundGain*self->surroundGain)+2*a1*a2*cfb*icc*self->surroundGain;
  Gcomp     = (float)sqrt(Etarget/(Eint+ABS_THR));

  t1Real *= Gcomp;
  t1Imag *= Gcomp;
  t2Real *= Gcomp;
  t2Imag *= Gcomp;

  for (i = 0; i < self->maskSlots; i++) {
    combinedReal[i]  = mask1Real[i] * t1Real - mask1Imag[i] * t1Imag;
    combinedImag[i]  = mask1Real[i] * t1Imag + mask1Imag[i] * t1Real;
    combinedReal[i] += mask2Real[i] * t2Real - mask2Imag[i] * t2Imag;
    combinedImag[i] += mask2Real[i] * t2Imag + mask2Imag[i] * t2Real;
  }
}


void SpatialDecOpenHrtfFilterbank(HRTF_FILTERBANK **selfPtr,
                                  HRTF_DATA const *hrtfModel,
                                  float surroundGain,
                                  int qmfBands)
{
  HRTF_FILTERBANK *self = NULL;
  float prototype[MAX_PROTOTYPE_LENGTH];
  int i;

  HRTF_DIRECTIONAL_DATA const *hrtf = hrtfModel->directional;
  int impulseLength = hrtfModel->impulseLength;

  self = calloc(1, sizeof(*self));

  if (self != NULL) {
    self->qmfBands = qmfBands;
    self->hybridBands = SacGetHybridSubbands(qmfBands);
    self->surroundGain = surroundGain;
    self->filterSlot[0] = -1;

#ifdef HRTF_DYNAMIC_UPDATE
    if (impulseLength > 0) {
      self->maskSlots = GetQmfMaskLength(impulseLength, qmfBands);

      GetPrototype(qmfBands, prototype);

      for (i = 0; i < HRTF_AZIMUTHS; i++) {
        ConvertImpulseToQmfMask(self, hrtf[i].impulseL, impulseLength, prototype, self->maskLeftReal[i], self->maskLeftImag[i]);
        ConvertImpulseToQmfMask(self, hrtf[i].impulseR, impulseLength, prototype, self->maskRightReal[i], self->maskRightImag[i]);
      }

      self->tauLfLsLeft  = GetImpulseDelayDifference(hrtf[0].impulseL, hrtf[3].impulseL, impulseLength);
      self->tauLfLsRight = GetImpulseDelayDifference(hrtf[0].impulseR, hrtf[3].impulseR, impulseLength);
      self->tauRfRsLeft  = GetImpulseDelayDifference(hrtf[1].impulseL, hrtf[4].impulseL, impulseLength);
      self->tauRfRsRight = GetImpulseDelayDifference(hrtf[1].impulseR, hrtf[4].impulseR, impulseLength);
    }
#else 
    self->maskSlots = GetQmfMaskLength(hrtfModel->impulseLength, qmfBands);

    GetPrototype(qmfBands, prototype);

    for (i = 0; i < HRTF_AZIMUTHS; i++) {
      ConvertImpulseToQmfMask(self, hrtf[i].impulseL, impulseLength, prototype, self->maskLeftReal[i], self->maskLeftImag[i]);
      ConvertImpulseToQmfMask(self, hrtf[i].impulseR, impulseLength, prototype, self->maskRightReal[i], self->maskRightImag[i]);
    }

    self->tauLfLsLeft  = GetImpulseDelayDifference(hrtf[0].impulseL, hrtf[3].impulseL, impulseLength);
    self->tauLfLsRight = GetImpulseDelayDifference(hrtf[0].impulseR, hrtf[3].impulseR, impulseLength);
    self->tauRfRsLeft  = GetImpulseDelayDifference(hrtf[1].impulseL, hrtf[4].impulseL, impulseLength);
    self->tauRfRsRight = GetImpulseDelayDifference(hrtf[1].impulseR, hrtf[4].impulseR, impulseLength);
#endif 
  }

  *selfPtr = self;
}


void SpatialDecCloseHrtfFilterbank(HRTF_FILTERBANK *self)
{
  if (self != NULL) {
    free(self);
  }
}

#ifdef HRTF_DYNAMIC_UPDATE

void SpatialDecReloadHrtfFilterbank(HRTF_FILTERBANK *self,
                                    SPATIAL_HRTF_DATA *hrtfData)
{
  self->maskSlots = hrtfData->impulseLength;

  
}
#endif 


void SpatialDecUpdateHrtfFilterbank(HRTF_FILTERBANK *self,
                                    float mReal[MAX_PARAMETER_BANDS][3][5],
                                    float mImag[MAX_PARAMETER_BANDS][3][5],
                                    float lf[MAX_PARAMETER_BANDS],
                                    float ls[MAX_PARAMETER_BANDS],
                                    float rf[MAX_PARAMETER_BANDS],
                                    float rs[MAX_PARAMETER_BANDS],
                                    float c1[MAX_PARAMETER_BANDS],
                                    float c2[MAX_PARAMETER_BANDS],
                                    float icc_c[MAX_PARAMETER_BANDS],
                                    int   aPredictionMode[MAX_PARAMETER_BANDS],
                                    int parametersSlot)
{
  float center1Real[MAX_MASK_SLOTS];
  float center1Imag[MAX_MASK_SLOTS];
  float center2Real[MAX_MASK_SLOTS];
  float center2Imag[MAX_MASK_SLOTS];
  float left1Real[MAX_MASK_SLOTS];
  float left1Imag[MAX_MASK_SLOTS];
  float left2Real[MAX_MASK_SLOTS];
  float left2Imag[MAX_MASK_SLOTS];
  float right1Real[MAX_MASK_SLOTS];
  float right1Imag[MAX_MASK_SLOTS];
  float right2Real[MAX_MASK_SLOTS];
  float right2Imag[MAX_MASK_SLOTS];
  float oldPhi1;
  float oldPhi2;
  float oldPhi3;
  float oldPhi4;
  int ps = self->filterSets + 1;
  int sign;
  int hs;
  int qs;
  int pb;
  int i;


  float alpha;
  float sigma;
  float beta;
  float p;
  float L;
  float R;
  float C;
  float dE;
  float EB1, EB2, dEB1, dEB2;
  int viable_flag;
  float g_comp1, g_comp2;


  self->filterSlot[ps] = parametersSlot;


  for (hs = 0; hs < self->hybridBands; hs++) {
    sign = SacGetParameterPhase(hs);
    qs = SacGetQmfSubband(hs);
    pb = SpatialDecGetParameterBand(MAX_PARAMETER_BANDS, hs);

    for (i = 0; i < self->maskSlots; i++) {
      center1Real[i] = self->maskLeftReal[2][qs][i];
      center1Imag[i] = self->maskLeftImag[2][qs][i];
      center2Real[i] = self->maskRightReal[2][qs][i];
      center2Imag[i] = self->maskRightImag[2][qs][i];
    }

    CombineFilters(self,
                   self->maskLeftReal[0][qs],
                   self->maskLeftImag[0][qs],
                   self->maskLeftReal[3][qs],
                   self->maskLeftImag[3][qs],
                   qs,
                   lf[pb],
                   ls[pb],
                   &oldPhi1,
                   self->tauLfLsLeft,
                   left1Real,
                   left1Imag);

    CombineFilters(self,
                   self->maskRightReal[0][qs],
                   self->maskRightImag[0][qs],
                   self->maskRightReal[3][qs],
                   self->maskRightImag[3][qs],
                   qs,
                   lf[pb],
                   ls[pb],
                   &oldPhi2,
                   self->tauLfLsRight,
                   left2Real,
                   left2Imag);

    CombineFilters(self,
                   self->maskLeftReal[1][qs],
                   self->maskLeftImag[1][qs],
                   self->maskLeftReal[4][qs],
                   self->maskLeftImag[4][qs],
                   qs,
                   rf[pb],
                   rs[pb],
                   &oldPhi3,
                   self->tauRfRsLeft,
                   right1Real,
                   right1Imag);

    CombineFilters(self,
                   self->maskRightReal[1][qs],
                   self->maskRightImag[1][qs],
                   self->maskRightReal[4][qs],
                   self->maskRightImag[4][qs],
                   qs,
                   rf[pb],
                   rs[pb],
                   &oldPhi4,
                   self->tauRfRsRight,
                   right2Real,
                   right2Imag);

    alpha = (1-c1[pb])/3;
    beta  = (1-c2[pb])/3;
    sigma = alpha+beta;

    if (fabs(alpha) < ABS_THR){
      alpha = 0;
    }
    if (fabs(beta) < ABS_THR){
      beta = 0;
    }
    if (fabs(1-sigma) < ABS_THR){
      sigma = 1;
    }

    p = alpha * beta;
    viable_flag = alpha>0 && beta>0 && sigma < 1;

    if (!aPredictionMode[pb]){
      viable_flag= 0;
    }

    L = beta*(1-sigma);
    R = alpha*(1-sigma);
    C = p;

    dE =  p*(1-sigma);

    if (viable_flag){
      float PowLeft1 = 0;
      float PowRight1 = 0;
      float PowCenter1 = 0;
      float PowLeft2 = 0;
      float PowRight2 = 0;
      float PowCenter2 = 0;
      float PowLR_C1 = 0;
      float PowLR_C2 = 0;

      for (i = 0; i < self->maskSlots; i++) {
        PowLeft1 += left1Real[i]*left1Real[i] + left1Imag[i]*left1Imag[i];
        PowRight1 += right1Real[i]*right1Real[i] + right1Imag[i]*right1Imag[i];
        PowCenter1 += 2 * (center1Real[i]*center1Real[i] + center1Imag[i]*center1Imag[i]);
        PowLeft2 += left2Real[i]*left2Real[i] + left2Imag[i]*left2Imag[i];
        PowRight2 += right2Real[i]*right2Real[i] + right2Imag[i]*right2Imag[i];
        PowCenter2 += 2 * (center2Real[i]*center2Real[i] + center2Imag[i]*center2Imag[i]);

        PowLR_C1 += (left1Real[i]+right1Real[i]-(float)sqrt(2)*center1Real[i])*(left1Real[i]+right1Real[i]-(float)sqrt(2)*center1Real[i]) +
                    (left1Imag[i]+right1Imag[i]-(float)sqrt(2)*center1Imag[i])*(left1Imag[i]+right1Imag[i]-(float)sqrt(2)*center1Imag[i]);
        PowLR_C2 += (left2Real[i]+right2Real[i]-(float)sqrt(2)*center2Real[i])*(left2Real[i]+right2Real[i]-(float)sqrt(2)*center2Real[i]) +
                    (left2Imag[i]+right2Imag[i]-(float)sqrt(2)*center2Imag[i])*(left2Imag[i]+right2Imag[i]-(float)sqrt(2)*center2Imag[i]);
      }

      EB1 = L*PowLeft1 + R*PowRight1 +C*PowCenter1;
      EB2 = L*PowLeft2 + R*PowRight2 +C*PowCenter2;
      dEB1 = dE*PowLR_C1;
      dEB2 = dE*PowLR_C2;

      g_comp1 = (float)(icc_c[pb]*min(2,sqrt((EB1+ABS_THR)/(fabs(EB1-dEB1)+ABS_THR))));
      g_comp2 = (float)(icc_c[pb]*min(2,sqrt((EB2+ABS_THR)/(fabs(EB2-dEB2)+ABS_THR))));
    }
    else{
      g_comp1 = 1;
      g_comp2 = 1;
    }

    for (i = 0; i < self->maskSlots; i++) {
        
          
      self->filterReal[ps][hs][0][0][i]  = mReal[pb][0][0] * left1Real[i]   - sign * mImag[pb][0][0] * left1Imag[i];
      self->filterImag[ps][hs][0][0][i]  = mReal[pb][0][0] * left1Imag[i]   + sign * mImag[pb][0][0] * left1Real[i];
      self->filterReal[ps][hs][0][0][i] += mReal[pb][1][0] * right1Real[i]  - sign * mImag[pb][1][0] * right1Imag[i];
      self->filterImag[ps][hs][0][0][i] += mReal[pb][1][0] * right1Imag[i]  + sign * mImag[pb][1][0] * right1Real[i];
      self->filterReal[ps][hs][0][0][i] += mReal[pb][2][0] * center1Real[i] - sign * mImag[pb][2][0] * center1Imag[i];
      self->filterImag[ps][hs][0][0][i] += mReal[pb][2][0] * center1Imag[i] + sign * mImag[pb][2][0] * center1Real[i];

          
      self->filterReal[ps][hs][0][1][i]  = mReal[pb][0][1] * left1Real[i]   - sign * mImag[pb][0][1] * left1Imag[i];
      self->filterImag[ps][hs][0][1][i]  = mReal[pb][0][1] * left1Imag[i]   + sign * mImag[pb][0][1] * left1Real[i];
      self->filterReal[ps][hs][0][1][i] += mReal[pb][1][1] * right1Real[i]  - sign * mImag[pb][1][1] * right1Imag[i];
      self->filterImag[ps][hs][0][1][i] += mReal[pb][1][1] * right1Imag[i]  + sign * mImag[pb][1][1] * right1Real[i];
      self->filterReal[ps][hs][0][1][i] += mReal[pb][2][1] * center1Real[i] - sign * mImag[pb][2][1] * center1Imag[i];
      self->filterImag[ps][hs][0][1][i] += mReal[pb][2][1] * center1Imag[i] + sign * mImag[pb][2][1] * center1Real[i];

          
      self->filterReal[ps][hs][0][2][i]  = mReal[pb][0][2] * left1Real[i]   - sign * mImag[pb][0][2] * left1Imag[i];
      self->filterImag[ps][hs][0][2][i]  = mReal[pb][0][2] * left1Imag[i]   + sign * mImag[pb][0][2] * left1Real[i];
      self->filterReal[ps][hs][0][2][i] += mReal[pb][1][2] * right1Real[i]  - sign * mImag[pb][1][2] * right1Imag[i];
      self->filterImag[ps][hs][0][2][i] += mReal[pb][1][2] * right1Imag[i]  + sign * mImag[pb][1][2] * right1Real[i];
      self->filterReal[ps][hs][0][2][i] += mReal[pb][2][2] * center1Real[i] - sign * mImag[pb][2][2] * center1Imag[i];
      self->filterImag[ps][hs][0][2][i] += mReal[pb][2][2] * center1Imag[i] + sign * mImag[pb][2][2] * center1Real[i];

          
      self->filterReal[ps][hs][0][3][i]  = mReal[pb][0][3] * left1Real[i]   - sign * mImag[pb][0][3] * left1Imag[i];
      self->filterImag[ps][hs][0][3][i]  = mReal[pb][0][3] * left1Imag[i]   + sign * mImag[pb][0][3] * left1Real[i];
      self->filterReal[ps][hs][0][3][i] += mReal[pb][1][3] * right1Real[i]  - sign * mImag[pb][1][3] * right1Imag[i];
      self->filterImag[ps][hs][0][3][i] += mReal[pb][1][3] * right1Imag[i]  + sign * mImag[pb][1][3] * right1Real[i];
      self->filterReal[ps][hs][0][3][i] += mReal[pb][2][3] * center1Real[i] - sign * mImag[pb][2][3] * center1Imag[i];
      self->filterImag[ps][hs][0][3][i] += mReal[pb][2][3] * center1Imag[i] + sign * mImag[pb][2][3] * center1Real[i];

          
      self->filterReal[ps][hs][0][4][i]  = 0;


        
          
      self->filterReal[ps][hs][1][0][i]  = mReal[pb][0][0] * left2Real[i]   - sign * mImag[pb][0][0] * left2Imag[i];
      self->filterImag[ps][hs][1][0][i]  = mReal[pb][0][0] * left2Imag[i]   + sign * mImag[pb][0][0] * left2Real[i];
      self->filterReal[ps][hs][1][0][i] += mReal[pb][1][0] * right2Real[i]  - sign * mImag[pb][1][0] * right2Imag[i];
      self->filterImag[ps][hs][1][0][i] += mReal[pb][1][0] * right2Imag[i]  + sign * mImag[pb][1][0] * right2Real[i];
      self->filterReal[ps][hs][1][0][i] += mReal[pb][2][0] * center2Real[i] - sign * mImag[pb][2][0] * center2Imag[i];
      self->filterImag[ps][hs][1][0][i] += mReal[pb][2][0] * center2Imag[i] + sign * mImag[pb][2][0] * center2Real[i];

          
      self->filterReal[ps][hs][1][1][i]  = mReal[pb][0][1] * left2Real[i]   - sign * mImag[pb][0][1] * left2Imag[i];
      self->filterImag[ps][hs][1][1][i]  = mReal[pb][0][1] * left2Imag[i]   + sign * mImag[pb][0][1] * left2Real[i];
      self->filterReal[ps][hs][1][1][i] += mReal[pb][1][1] * right2Real[i]  - sign * mImag[pb][1][1] * right2Imag[i];
      self->filterImag[ps][hs][1][1][i] += mReal[pb][1][1] * right2Imag[i]  + sign * mImag[pb][1][1] * right2Real[i];
      self->filterReal[ps][hs][1][1][i] += mReal[pb][2][1] * center2Real[i] - sign * mImag[pb][2][1] * center2Imag[i];
      self->filterImag[ps][hs][1][1][i] += mReal[pb][2][1] * center2Imag[i] + sign * mImag[pb][2][1] * center2Real[i];

          
      self->filterReal[ps][hs][1][2][i]  = mReal[pb][0][2] * left2Real[i]   - sign * mImag[pb][0][2] * left2Imag[i];
      self->filterImag[ps][hs][1][2][i]  = mReal[pb][0][2] * left2Imag[i]   + sign * mImag[pb][0][2] * left2Real[i];
      self->filterReal[ps][hs][1][2][i] += mReal[pb][1][2] * right2Real[i]  - sign * mImag[pb][1][2] * right2Imag[i];
      self->filterImag[ps][hs][1][2][i] += mReal[pb][1][2] * right2Imag[i]  + sign * mImag[pb][1][2] * right2Real[i];
      self->filterReal[ps][hs][1][2][i] += mReal[pb][2][2] * center2Real[i] - sign * mImag[pb][2][2] * center2Imag[i];
      self->filterImag[ps][hs][1][2][i] += mReal[pb][2][2] * center2Imag[i] + sign * mImag[pb][2][2] * center2Real[i];

          
      self->filterReal[ps][hs][1][3][i]  = mReal[pb][0][3] * left2Real[i]   - sign * mImag[pb][0][3] * left2Imag[i];
      self->filterImag[ps][hs][1][3][i]  = mReal[pb][0][3] * left2Imag[i]   + sign * mImag[pb][0][3] * left2Real[i];
      self->filterReal[ps][hs][1][3][i] += mReal[pb][1][3] * right2Real[i]  - sign * mImag[pb][1][3] * right2Imag[i];
      self->filterImag[ps][hs][1][3][i] += mReal[pb][1][3] * right2Imag[i]  + sign * mImag[pb][1][3] * right2Real[i];
      self->filterReal[ps][hs][1][3][i] += mReal[pb][2][3] * center2Real[i] - sign * mImag[pb][2][3] * center2Imag[i];
      self->filterImag[ps][hs][1][3][i] += mReal[pb][2][3] * center2Imag[i] + sign * mImag[pb][2][3] * center2Real[i];

          
      self->filterReal[ps][hs][1][4][i]  = 0;
        
      self->filterReal[ps][hs][0][0][i] *= g_comp1;
      self->filterImag[ps][hs][0][0][i] *= g_comp1;
      self->filterReal[ps][hs][0][1][i] *= g_comp1;
      self->filterImag[ps][hs][0][1][i] *= g_comp1;
      self->filterReal[ps][hs][0][2][i] *= g_comp1;
      self->filterImag[ps][hs][0][2][i] *= g_comp1;
      self->filterReal[ps][hs][0][3][i] *= g_comp1;
      self->filterImag[ps][hs][0][3][i] *= g_comp1;
      self->filterReal[ps][hs][1][0][i] *= g_comp2;
      self->filterImag[ps][hs][1][0][i] *= g_comp2;
      self->filterReal[ps][hs][1][1][i] *= g_comp2;
      self->filterImag[ps][hs][1][1][i] *= g_comp2;
      self->filterReal[ps][hs][1][2][i] *= g_comp2;
      self->filterImag[ps][hs][1][2][i] *= g_comp2;
      self->filterReal[ps][hs][1][3][i] *= g_comp2;
      self->filterImag[ps][hs][1][3][i] *= g_comp2;
    }
  }
  self->filterSets++;
}


void SpatialDecApplyHrtfFilterbank(HRTF_FILTERBANK *self,
                                   float hybInputReal[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                   float hybInputImag[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                   float hybOutputReal[OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                   float hybOutputImag[OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                   int   arbDmxResEnabled)
{
  float filterReal[OUTPUT_CHANNELS][2*INPUT_CHANNELS][MAX_MASK_SLOTS];
  float filterImag[OUTPUT_CHANNELS][2*INPUT_CHANNELS][MAX_MASK_SLOTS];
  int ps;
  int ts;
  int hs;
  int row;
  int col;
  int i;

  int nrInCh = (arbDmxResEnabled ? 2*INPUT_CHANNELS:INPUT_CHANNELS);


  for (ps = 0; ps < self->filterSets; ps++) {
    int prevSlot = self->filterSlot[ps];
    int currSlot = self->filterSlot[ps + 1];

    for (ts = prevSlot + 1; ts <= currSlot; ts++) {
      float alpha = (float) (ts - prevSlot) / (currSlot - prevSlot);

      for (hs = 0; hs < self->hybridBands; hs++) {

        
        for (row = 0; row < OUTPUT_CHANNELS; row++) {
          for (col = 0; col < nrInCh; col++) {
            for (i = 0; i < self->maskSlots; i++) {
              filterReal[row][col][i] = (1.0f - alpha) * self->filterReal[ps][hs][row][col][i] + alpha * self->filterReal[ps + 1][hs][row][col][i];
              filterImag[row][col][i] = (1.0f - alpha) * self->filterImag[ps][hs][row][col][i] + alpha * self->filterImag[ps + 1][hs][row][col][i];
            }
          }
        }

        
        for (row = 0; row < OUTPUT_CHANNELS; row++) {
          for (col = 0; col < nrInCh; col++) {
            for (i = 0; i < self->maskSlots; i++) {
              self->filterStateReal[hs][row][col][i] += filterReal[row][col][i] * hybInputReal[col][ts][hs] - filterImag[row][col][i] * hybInputImag[col][ts][hs];
              self->filterStateImag[hs][row][col][i] += filterReal[row][col][i] * hybInputImag[col][ts][hs] + filterImag[row][col][i] * hybInputReal[col][ts][hs];
            }
          }
        }

        
        for (row = 0; row < OUTPUT_CHANNELS; row++) {
          hybOutputReal[row][ts][hs] = 0.0f;
          hybOutputImag[row][ts][hs] = 0.0f;

          for (col = 0; col < nrInCh; col++) {
            hybOutputReal[row][ts][hs] += self->filterStateReal[hs][row][col][0];
            hybOutputImag[row][ts][hs] += self->filterStateImag[hs][row][col][0];

            for (i = 1; i < self->maskSlots; i++) {
              self->filterStateReal[hs][row][col][i - 1] = self->filterStateReal[hs][row][col][i];
              self->filterStateImag[hs][row][col][i - 1] = self->filterStateImag[hs][row][col][i];
            }

            self->filterStateReal[hs][row][col][self->maskSlots - 1] = 0;
            self->filterStateImag[hs][row][col][self->maskSlots - 1] = 0;
          }
        }
      }
    }
  }

  
  for (hs = 0; hs < self->hybridBands; hs++) {
    for (row = 0; row < OUTPUT_CHANNELS; row++) {
      for (col = 0; col < nrInCh; col++) {
        for (i = 0; i < self->maskSlots; i++) {
          self->filterReal[0][hs][row][col][i] = self->filterReal[self->filterSets][hs][row][col][i];
          self->filterImag[0][hs][row][col][i] = self->filterImag[self->filterSets][hs][row][col][i];
        }
      }
    }
  }

  self->filterSets = 0;
}
