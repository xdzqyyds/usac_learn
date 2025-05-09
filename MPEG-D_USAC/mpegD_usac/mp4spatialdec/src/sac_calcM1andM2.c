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

#include "sac_dec.h"
#include "sac_polyphase.h"
#include "sac_config.h"
#include "sac_calcM1andM2.h"
#include "sac_bitdec.h"
#include "sac_hrtf.h"

#include "sac_ipd_dec.h"
#include "sac_smoothing.h"

#include <math.h>
#include <memory.h>

#include <assert.h>


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#undef ABS_THR
#define ABS_THR 1.0e-9f


#define ICC_FIX 
#define UNSINGULARIZE_FIX 
#define QUANTIZE_PARS_FIX

#ifndef PI
#define PI 3.14159265358979f
#endif

static const int row2channelSTP[][MAX_M2_INPUT] =
{
  { 0,  1,  2, -1,  3,  4, -1, -1},
  { 0,  3,  1,  4,  2, -1, -1, -1},
  { 0,  2,  1,  3, -1, -1, -1, -1},
  { 0,  4,  2,  1,  5,  3, -1, -1},
  { 0,  4,  2,  1,  5,  3, -1, -1},
  { 0,  2, -1,  1,  3, -1, -1, -1},
  {-1,  2,  0, -1,  3,  1, -1, -1},
  { 0,  1, -1, -1, -1, -1, -1, -1}
};

static const int row2channelGES[][MAX_M2_INPUT] =
{
  { 0,  1,  2, -1,  3,  4, -1, -1},
  { 0,  3,  1,  4,  2, -1, -1, -1},
  { 0,  3,  1,  4,  2, -1, -1, -1},
  { 0,  5,  3,  1,  6,  4,  2, -1},
  { 0,  5,  3,  1,  6,  4,  2, -1},
  { 0,  2, -1,  1,  3, -1, -1, -1},
  {-1,  2,  0, -1,  3,  1, -1, -1},
  { 0,  1, -1, -1, -1, -1, -1, -1}
};

static const int row2vchannel[][MAX_M2_INPUT] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 2, 2},
  {0, 0, 0, 1, 1, 1, 2, 2},
  {0, 0, 0, 1, 1, 1, 2, 2},
  {0, 0, 0, 1, 1, 1, 2, 2},
  {0, 0, 0, 1, 1, 1, 2, 2},
  {0, 0}
};

static const int row2residual[][MAX_M2_INPUT] =
{
  {-1,  0,  1,  3,  2,  4},
  {-1,  0,  1,  3,  4,  2},
  {-1, -1, -1,  1,  2,  0},
  {-1, -1, -1,  1,  2,  0,  3,  4},
  {-1, -1, -1,  1,  2,  0,  3,  4},
  {-1, -1, -1, -1, -1, -1,  0,  1},
  {-1, -1, -1, -1, -1, -1,  0,  1},
  {-1,  0}
};

static int hybrid2param_28[MAX_HYBRID_BANDS] = {
   1,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
  14, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
  22, 23, 23, 23, 23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25,
  26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 27, 27, 27, 27, 27
};


static void param2UMX_PS_Core(float cld[MAX_PARAMETER_BANDS],
                              float icc[MAX_PARAMETER_BANDS],
                              int numOttBands,
                              int resBands,
                              float H11[MAX_PARAMETER_BANDS],
                              float H12[MAX_PARAMETER_BANDS],
                              float H21[MAX_PARAMETER_BANDS],
                              float H22[MAX_PARAMETER_BANDS],
                              float H12_res[MAX_PARAMETER_BANDS],
                              float H22_res[MAX_PARAMETER_BANDS],
                              float c_l[MAX_PARAMETER_BANDS],
                              float c_r[MAX_PARAMETER_BANDS]);


static void param2UMX_PS(spatialDec* self,
                         float H11[MAX_PARAMETER_BANDS],
                         float H12[MAX_PARAMETER_BANDS],
                         float H21[MAX_PARAMETER_BANDS],
                         float H22[MAX_PARAMETER_BANDS],
                         float H21_res[MAX_PARAMETER_BANDS],
                         float H22_res[MAX_PARAMETER_BANDS],
                         float c_l[MAX_PARAMETER_BANDS],
                         float c_r[MAX_PARAMETER_BANDS],
                         int ottBoxIndx,
                         int parameterSetIndx,
                         int resBands);

static void param2UMX_PS_IPD_OPD(spatialDec* self,
                                 float H11re[MAX_PARAMETER_BANDS],
                                 float H12re[MAX_PARAMETER_BANDS],
                                 float H21re[MAX_PARAMETER_BANDS],
                                 float H22re[MAX_PARAMETER_BANDS],
                                 float H12_res[MAX_PARAMETER_BANDS],
                                 float H22_res[MAX_PARAMETER_BANDS],
                                 float c_l[MAX_PARAMETER_BANDS],
                                 float c_r[MAX_PARAMETER_BANDS],
                                 int ottBoxIndx,
                                 int parameterSetIndx,
                                 int resBands);

static void param2UMX_Prediction(spatialDec* self,
                                 float H11re[MAX_PARAMETER_BANDS],
                                 float H11im[MAX_PARAMETER_BANDS],
                                 float H12re[MAX_PARAMETER_BANDS],
                                 float H12im[MAX_PARAMETER_BANDS],
                                 float H21re[MAX_PARAMETER_BANDS],
                                 float H21im[MAX_PARAMETER_BANDS],
                                 float H22re[MAX_PARAMETER_BANDS],
                                 float H22im[MAX_PARAMETER_BANDS],
                                 float H12_res[MAX_PARAMETER_BANDS],
                                 float H22_res[MAX_PARAMETER_BANDS],
                                 int ottBoxIndx,
                                 int parameterSetIndx,
                                 int resBands);

static void modifyResidualWeights(int numParameterBands,                          
                                  float H12_res[MAX_PARAMETER_BANDS],
                                  float H22_res[MAX_PARAMETER_BANDS],
                                  int resBands);

static void getMatrixInversionWeights(float iid_lf_ls,
                                      float iid_rf_rs,
                                      int predictionMode,
                                      float c1,
                                      float c2,
                                      float *weight1,
                                      float *weight2);

static void invertMatrix(float weight1,
                         float weight2,
                         float hReal[2][2],
                         float hImag[2][2]);

#ifdef PARTIALLY_COMPLEX
static void invertMatrixReal(float weight1,
                             float weight2,
                             float h[2][2]);

static void interp_UMX_IMTX(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                            float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                            int nRows,
                            int nCols,
                            spatialDec* self);
#endif

static void interp_UMX(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       float Mprev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       int nRows,
                       int nCols,
                       spatialDec* self);

static void applyAbsKernels( float Rin [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                             float Rout[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT],
                             int nRows,
                             int nCols,
                             spatialDec* self );

static void interp_UMX_IPD(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][2],
                       float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][2],
                       float Mprev[MAX_PARAMETER_BANDS][2],
                       int nCols,
                       spatialDec* self);

static void interp_phase(float pl[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
                         float pr[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
                         float plPrev[MAX_PARAMETER_BANDS],
                         float prPrev[MAX_PARAMETER_BANDS],
                         float Rreal[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                         float Rimag[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                         spatialDec* self);

static void applyAbsKernels_IPD( float Rin [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                             float Rout[MAX_TIME_SLOTS][MAX_HYBRID_BANDS][2],
                             int nCols,
                             spatialDec* self );

static void SpatialDecCalculateM0(spatialDec* self);
static void SpatialDecCalculateM1andM2_212(spatialDec* self);
static void SpatialDecCalculateM1andM2_5121(spatialDec* self);
static void SpatialDecCalculateM1andM2_5122(spatialDec* self);
static void SpatialDecCalculateM1andM2_5151(spatialDec* self);
static void SpatialDecCalculateM1andM2_5152(spatialDec* self);
static void SpatialDecCalculateM1andM2_51S1(spatialDec* self);
static void SpatialDecCalculateM1andM2_51S2(spatialDec* self);
static void SpatialDecCalculateM1andM2_5221(spatialDec* self);
static void SpatialDecCalculateM1andM2_5227(spatialDec* self);
static void SpatialDecCalculateM1andM2_5251(spatialDec* self);
static void SpatialDecCalculateM1andM2_EMM(spatialDec* self);
static void SpatialDecCalculateM1andM2_7271(spatialDec* self);
static void SpatialDecCalculateM1andM2_7272(spatialDec* self);
static void SpatialDecCalculateM1andM2_7571(spatialDec* self);
static void SpatialDecCalculateM1andM2_7572(spatialDec* self);

#ifdef PARTIALLY_COMPLEX
void AliasLock( float R[MAX_TIME_SLOTS][MAX_HYBRID_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                int historyIndex,
                spatialDec* self );
#endif

#ifdef PARTIALLY_COMPLEX

void imtx_515(spatialDec* self) {
  int col,row,ps,pb;

  for (ps = 0; ps < self->numParameterSets; ps++) {
    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col=0; col<5; col++) {
        for (row=0; row<6; row++) {
          self->M2decorReal[ps][pb][row][col] *= self->M1paramReal[ps][pb][col][0];

          if (col == 0) { 
            self->M2residReal[ps][pb][row][col] *= self->M1paramReal[ps][pb][col][0];
          }
        }
      }
      for (col=0; col<5; col++) {
        self->M1paramReal[ps][pb][col][0]=1.0;
      }
    } 
  } 
}
#endif


int SpatialDecGetChannelIndex(spatialDec* self, int row) {
  
  switch(self->tempShapeConfig) {
  case 1:
    return row2channelSTP[self->treeConfig][row];
  case 2:
    return row2channelGES[self->treeConfig][row];
  default:
    return -1;
  }

}



int SpatialDecGetM1OutputIndex(spatialDec* self, int row) {

  return row2vchannel[self->treeConfig][row];
}



int SpatialDecGetResidualIndex(spatialDec* self, int row) {

  return row2residual[self->treeConfig][row];
}



int SpatialDecGetParameterBand(int parameterBands, int hybridBand) {

  assert(parameterBands == MAX_PARAMETER_BANDS);

  return hybrid2param_28[hybridBand];
}


#ifndef PHASE_COD_NOFIX
/* wrap phase in rad to the range of 0 <= x < 2*pi */
static float wrapPhase(float phase) {
  const float pi_2 = 2.0f * (float)P_PI;
 
  while (phase < 0.0f) phase += pi_2;
  while (phase >= pi_2) phase -= pi_2;
  assert( (phase >= 0.0f) && (phase < pi_2) );
 
  return phase;
}
#endif


static void bufferM1andM2(spatialDec* self) {

  int pb, row, col;

  for(pb = 0; pb < self->numParameterBands; pb++){
    for(row = 0; row < MAX_M_INPUT; row++){
      for(col = 0; col < MAX_M_OUTPUT; col++){
        self->M0arbdmxPrev[pb][row][col]    = self->M0arbdmx[self->numParameterSetsPrev - 1][pb][row][col];
        self->M1paramRealPrev[pb][row][col] = self->M1paramReal[self->numParameterSetsPrev - 1][pb][row][col];
        self->M1paramImagPrev[pb][row][col] = self->M1paramImag[self->numParameterSetsPrev - 1][pb][row][col];
        self->M2decorRealPrev[pb][row][col] = self->M2decorReal[self->numParameterSetsPrev - 1][pb][row][col];
        self->M2decorImagPrev[pb][row][col] = self->M2decorImag[self->numParameterSetsPrev - 1][pb][row][col];
        self->M2residRealPrev[pb][row][col] = self->M2residReal[self->numParameterSetsPrev - 1][pb][row][col];
        self->M2residImagPrev[pb][row][col] = self->M2residImag[self->numParameterSetsPrev - 1][pb][row][col];
      }
    }
  }

  for(pb = 0; pb < self->numParameterBands; pb++){
    self->PhasePrevLeft[pb]  = self->PhaseLeft[self->numParameterSetsPrev - 1][pb];
    self->PhasePrevRight[pb] = self->PhaseRight[self->numParameterSetsPrev - 1][pb];
  }
}



static void updateAlpha(spatialDec* self) {

  float alpha;
  int nChIn = self->numInputChannels;
  int ch;

  for (ch = 0; ch < nChIn; ch++) {
    self->arbdmxAlphaPrev[ch] = self->arbdmxAlpha[ch];

    if (self->arbitraryDownmix == 2) {
      
      alpha = self->arbdmxAlpha[ch];

      if (self->arbdmxResidualAbs[ch]) {
        alpha -= 0.33f;
        if (alpha < 0.0f) {
          alpha = 0.0f;
        }
      }
      else {
        alpha += 0.33f;
        if (alpha > 1.0f) {
          alpha = 1.0f;
        }
      }
    }
    else {
      alpha = 1.0f;
    }

    self->arbdmxAlpha[ch] = alpha;
  }
}


#ifdef HRTF_DYNAMIC_UPDATE

static void UpdateHRTF(spatialDec* self) {

  SPATIAL_HRTF_DATA *hrtfData = self->hrtfSource->GetHRTF(self->hrtfSource);
  HRTF_DATA *localHrtfData = (HRTF_DATA*)self->hrtfData;
  int az;
  int pb;

  assert(hrtfData->sampleRate == self->samplingFreq);
  assert(hrtfData->parameterBands == MAX_PARAMETER_BANDS);
  assert(hrtfData->azimuths == HRTF_AZIMUTHS);

  if (localHrtfData->sampleRate == 0) {
    
    localHrtfData->sampleRate    = hrtfData->sampleRate;
    localHrtfData->impulseLength = hrtfData->impulseLength;
  }

  assert(hrtfData->impulseLength == localHrtfData->impulseLength);

  assert(hrtfData->impulseLength == 0); 

  if (hrtfData->impulseLength == 0) {
    
    for (az = 0; az < hrtfData->azimuths; az++) {
      for (pb = 0; pb < hrtfData->parameterBands; pb++) {
        localHrtfData->directional[az].powerL[pb] = hrtfData->directional[az].powerLeft[pb];
        localHrtfData->directional[az].powerR[pb] = hrtfData->directional[az].powerRight[pb];
        localHrtfData->directional[az].icc[pb]    = hrtfData->directional[az].icc[pb];
        localHrtfData->directional[az].ipd[pb]    = hrtfData->directional[az].ipd[pb];
      }
    }
  }
  else {
    
    assert(hrtfData->qmfBands == self->qmfBands);

    SpatialDecReloadHrtfFilterbank(self->hrtfFilter, hrtfData);
  }
}
#endif 


void SpatialDecCalculateM1andM2(spatialDec* self) {

  bufferM1andM2(self);


  if (self->arbitraryDownmix != 0) {
    updateAlpha(self);
  }

  switch(self->treeConfig){
  case TREE_212:
    SpatialDecCalculateM1andM2_212(self);
    break;
  case TREE_5151:
    if (self->upmixType == 2) {
      SpatialDecCalculateM1andM2_5121(self);
    }
    else {
      if (self->upmixType == 3) {
        SpatialDecCalculateM1andM2_51S1(self);
      }
      else {
        SpatialDecCalculateM1andM2_5151(self);
      }
    }
    break;
  case TREE_5152:
    if (self->upmixType == 2) {
      SpatialDecCalculateM1andM2_5122(self);
    }
    else {
      if (self->upmixType == 3) {
        SpatialDecCalculateM1andM2_51S2(self);
      }
      else {
        SpatialDecCalculateM1andM2_5152(self);
      }
    }
    break;
  case TREE_525:
    if (self->upmixType == 1) {
      SpatialDecCalculateM1andM2_EMM(self);
    }
    else if (self->upmixType == 2) {
      if (self->binauralQuality == 1) {
        SpatialDecCalculateM1andM2_5227(self);
      }
      else {
        SpatialDecCalculateM1andM2_5221(self);
      }
    }
    else {
      SpatialDecCalculateM1andM2_5251(self);
    }
    break;
  case TREE_7271:
    if (self->upmixType == 0) {
      SpatialDecCalculateM1andM2_7271(self);
    } 
    else if (self->upmixType == 2) {  
      if (self->binauralQuality == 1) {
        SpatialDecCalculateM1andM2_5227(self);
      }
      else {
        SpatialDecCalculateM1andM2_5221(self);
      }
    }
    break;
  case TREE_7272:
    if (self->upmixType == 0) {
      SpatialDecCalculateM1andM2_7272(self);
    }
    else if (self->upmixType == 2) {  
      if (self->binauralQuality == 1) {
        SpatialDecCalculateM1andM2_5227(self);
      }
      else {
        SpatialDecCalculateM1andM2_5221(self);
      }
    }
    break;
  case TREE_7571:
    SpatialDecCalculateM1andM2_7571(self);
    break;
  case TREE_7572:
    SpatialDecCalculateM1andM2_7572(self);
    break;
  default:
    assert(0);
    break;
  };
}



static void CalculateArbDmxMtx(spatialDec* self, int ps, int pb, float G_real[]) {

  int ch;
  float gain;

  int nChIn = self->numInputChannels;

  for (ch = 0; ch < nChIn; ch++) {

    gain = (float) pow(10.0f, self->arbdmxGain[ch][ps][pb] / 20.0f);

    if (pb < self->arbdmxResidualBands) {
      if ((ps == 0) && (self->arbdmxAlphaUpdSet[ch] == 1)) {
        G_real[ch] = self->arbdmxAlphaPrev[ch] * gain;
      }
      else {
        G_real[ch] = self->arbdmxAlpha[ch] * gain;
      }
    }
    else {
      G_real[ch] = gain;
    }
  }
}


static void SpatialDecCalculateM1andM2_212(spatialDec* self) {

  int ps, pb;

  float H11re    [MAX_PARAMETER_BANDS];
  float H11im    [MAX_PARAMETER_BANDS] = {0};
  float H12re    [MAX_PARAMETER_BANDS];
  float H12im    [MAX_PARAMETER_BANDS] = {0};
  float H21re    [MAX_PARAMETER_BANDS];
  float H21im    [MAX_PARAMETER_BANDS] = {0};
  float H22re    [MAX_PARAMETER_BANDS];
  float H22im    [MAX_PARAMETER_BANDS] = {0};
  float H12_res  [MAX_PARAMETER_BANDS];
  float H22_res  [MAX_PARAMETER_BANDS];
  float c_l      [MAX_PARAMETER_BANDS];
  float c_r      [MAX_PARAMETER_BANDS];
  
  for (ps=0; ps < self->numParameterSets; ps++) {
    switch (self->bsConfig.bsPhaseCoding) {
    case 0:
#ifdef RESIDUAL_CODING_WO_PHASE_NOFIX
      param2UMX_PS(self, H11re,  H12re,  H21re,  H22re,  H12_res,  H22_res,  c_l,  c_r,  0, ps, self->resBands[0]);
#else /* RESIDUAL_CODING_WO_PHASE_NOFIX */
      if (self->bsConfig.bsResidualCoding) {
        param2UMX_Prediction(self, H11re, H11im, H12re, H12im, H21re, H21im, H22re, H22im, H12_res, H22_res, 0, ps, self->resBands[0]);
      }
      else{
        param2UMX_PS(self, H11re,  H12re,  H21re,  H22re,  H12_res,  H22_res,  c_l,  c_r,  0, ps, self->resBands[0]);
      }
#endif /* RESIDUAL_CODING_WO_PHASE_NOFIX */
      break;
    case 1:
      param2UMX_PS_IPD_OPD(self, H11re, H12re, H21re, H22re, H12_res, H22_res, c_l, c_r, 0, ps, self->resBands[0]);
      break;
    case 2:
      param2UMX_Prediction(self, H11re, H11im, H12re, H12im, H21re, H21im, H22re, H22im, H12_res, H22_res, 0, ps, self->resBands[0]);
      break;
    }

    for(pb = 0; pb < self->numParameterBands; pb++){
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][0] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorImag[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][1] = H12re[pb];
      self->M2decorImag[ps][pb][0][1] = H12im[pb];

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorImag[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][1] = H22re[pb];
      self->M2decorImag[ps][pb][1][1] = H22im[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2residReal[ps][pb][0][0] = H11re[pb];
      self->M2residImag[ps][pb][0][0] = H11im[pb];
      self->M2residReal[ps][pb][0][1] = H12_res[pb];
      self->M2residImag[ps][pb][0][1] = 0;

      self->M2residReal[ps][pb][1][0] = H21re[pb];
      self->M2residImag[ps][pb][1][0] = H21im[pb];
      self->M2residReal[ps][pb][1][1] = H22_res[pb];
      self->M2residImag[ps][pb][1][1] = 0;
    }

  }	
  
  SpatialDecSmoothOPD(self);

}


static void SpatialDecCalculateM1andM2_5121(spatialDec* self) {

  int ps, pb;

  float lf[MAX_PARAMETER_BANDS];
  float rf[MAX_PARAMETER_BANDS];
  float c [MAX_PARAMETER_BANDS];
  float ls[MAX_PARAMETER_BANDS];
  float rs[MAX_PARAMETER_BANDS];
  float l [MAX_PARAMETER_BANDS];
  float r [MAX_PARAMETER_BANDS];
  float lrReal[MAX_PARAMETER_BANDS];
  float lrImag[MAX_PARAMETER_BANDS];
  float iid[MAX_PARAMETER_BANDS];
  float ipd[MAX_PARAMETER_BANDS];
  float icc[MAX_PARAMETER_BANDS];
  float H11Real[MAX_PARAMETER_BANDS];
  float H11Imag[MAX_PARAMETER_BANDS];
  float H12Real[MAX_PARAMETER_BANDS];
  float H12Imag[MAX_PARAMETER_BANDS];
  float H21Real[MAX_PARAMETER_BANDS];
  float H21Imag[MAX_PARAMETER_BANDS];
  float H22Real[MAX_PARAMETER_BANDS];
  float H22Imag[MAX_PARAMETER_BANDS];

  HRTF_DIRECTIONAL_DATA const * hrtf = self->hrtfData->directional;

  for (ps = 0; ps < self->numParameterSets; ps++) {
    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iidPowFS = (float) pow(10.0f, self->ottCLD[0][ps][pb] / 10.0f);
      float iidPowC  = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
      float iidPowS  = (float) pow(10.0f, self->ottCLD[2][ps][pb] / 10.0f);
      float iidPowF  = (float) pow(10.0f, self->ottCLD[3][ps][pb] / 10.0f);

      float powerFSf = iidPowFS / (1 + iidPowFS);
      float powerFSs = 1        / (1 + iidPowFS);
      float powerCf  = iidPowC  / (1 + iidPowC);
      float powerCc  = 1        / (1 + iidPowC);
      float powerSl  = iidPowS  / (1 + iidPowS);
      float powerSr  = 1        / (1 + iidPowS);
      float powerFl  = iidPowF  / (1 + iidPowF);
      float powerFr  = 1        / (1 + iidPowF);

      lf[pb] = powerFSf * powerCf * powerFl;
      rf[pb] = powerFSf * powerCf * powerFr;
      c[pb]  = powerFSf * powerCc;
      ls[pb] = powerFSs * powerSl * self->surroundGain * self->surroundGain;
      rs[pb] = powerFSs * powerSr * self->surroundGain * self->surroundGain;
    }

#ifdef HRTF_DYNAMIC_UPDATE
    if ((self->hrtfSource != NULL) && ((ps < (self->numParameterSets - 1)) || (!self->extendFrame))) {
      UpdateHRTF(self);
    }
#endif 

    for (pb = 0; pb < self->numParameterBands; pb++) {
#ifndef ICC_FIX
      float sqrtIccLfRf = (float) sqrt(lf[pb] * rf[pb]) * self->ottICC[3][ps][pb];
      float sqrtIccLsRs = (float) sqrt(ls[pb] * rs[pb]) * self->ottICC[2][ps][pb];
      float iccF = hrtf[0].icc[pb] * (float) cos(hrtf[0].ipd[pb]) + hrtf[1].icc[pb] * (float) cos(hrtf[1].ipd[pb]);
      float iccS = hrtf[3].icc[pb] * (float) cos(hrtf[3].ipd[pb]) + hrtf[4].icc[pb] * (float) cos(hrtf[4].ipd[pb]);
      float temp;

      l[pb]  = hrtf[0].powerL[pb] * hrtf[0].powerL[pb] * lf[pb];
      l[pb] += hrtf[1].powerL[pb] * hrtf[1].powerL[pb] * rf[pb];
      l[pb] += hrtf[2].powerL[pb] * hrtf[2].powerL[pb] * c[pb];
      l[pb] += hrtf[3].powerL[pb] * hrtf[3].powerL[pb] * ls[pb];
      l[pb] += hrtf[4].powerL[pb] * hrtf[4].powerL[pb] * rs[pb];
      l[pb] += hrtf[0].powerL[pb] * hrtf[1].powerL[pb] * sqrtIccLfRf * iccF;
      l[pb] += hrtf[3].powerL[pb] * hrtf[4].powerL[pb] * sqrtIccLsRs * iccS;

      r[pb]  = hrtf[0].powerR[pb] * hrtf[0].powerR[pb] * lf[pb];
      r[pb] += hrtf[1].powerR[pb] * hrtf[1].powerR[pb] * rf[pb];
      r[pb] += hrtf[2].powerR[pb] * hrtf[2].powerR[pb] * c[pb];
      r[pb] += hrtf[3].powerR[pb] * hrtf[3].powerR[pb] * ls[pb];
      r[pb] += hrtf[4].powerR[pb] * hrtf[4].powerR[pb] * rs[pb];
      r[pb] += hrtf[0].powerR[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf * iccF;
      r[pb] += hrtf[3].powerR[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs * iccS;

      temp        = hrtf[0].powerL[pb] * hrtf[0].powerR[pb] * hrtf[0].icc[pb] * lf[pb];
      lrReal[pb]  = (float) cos(hrtf[0].ipd[pb]) * temp;
      lrImag[pb]  = (float) sin(hrtf[0].ipd[pb]) * temp;
      temp        = hrtf[1].powerL[pb] * hrtf[1].powerR[pb] * hrtf[1].icc[pb] * rf[pb];
      lrReal[pb] += (float) cos(hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[1].ipd[pb]) * temp;
      temp        = hrtf[2].powerL[pb] * hrtf[2].powerR[pb] * hrtf[2].icc[pb] * c[pb];
      lrReal[pb] += (float) cos(hrtf[2].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[2].ipd[pb]) * temp;
      temp        = hrtf[3].powerL[pb] * hrtf[3].powerR[pb] * hrtf[3].icc[pb] * ls[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb]) * temp;
      temp        = hrtf[4].powerL[pb] * hrtf[4].powerR[pb] * hrtf[4].icc[pb] * rs[pb];
      lrReal[pb] += (float) cos(hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[4].ipd[pb]) * temp;
      lrReal[pb] += hrtf[0].powerL[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf;
      lrReal[pb] += hrtf[3].powerL[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs;
      temp        = hrtf[1].powerL[pb] * hrtf[0].powerR[pb] * sqrtIccLfRf * hrtf[0].icc[pb];
      lrReal[pb] += (float) cos(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      temp        = hrtf[4].powerL[pb] * hrtf[3].powerR[pb] * sqrtIccLsRs * hrtf[3].icc[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
#else 
      float sqrtIccLfRf = (float) sqrt(lf[pb] * rf[pb]) * self->ottICC[3][ps][pb];
      float sqrtIccLsRs = (float) sqrt(ls[pb] * rs[pb]) * self->ottICC[2][ps][pb];
      float iccRf = hrtf[1].icc[pb] * (float) cos(hrtf[1].ipd[pb]);
      float iccRs = hrtf[4].icc[pb] * (float) cos(hrtf[4].ipd[pb]);

      float iccLf = hrtf[0].icc[pb] * (float) cos(hrtf[0].ipd[pb]);
      float iccLs = hrtf[3].icc[pb] * (float) cos(hrtf[3].ipd[pb]);
      float temp;

      l[pb]  = hrtf[0].powerL[pb] * hrtf[0].powerL[pb] * lf[pb];
      l[pb] += hrtf[1].powerL[pb] * hrtf[1].powerL[pb] * rf[pb];
      l[pb] += hrtf[2].powerL[pb] * hrtf[2].powerL[pb] * c[pb];
      l[pb] += hrtf[3].powerL[pb] * hrtf[3].powerL[pb] * ls[pb];
      l[pb] += hrtf[4].powerL[pb] * hrtf[4].powerL[pb] * rs[pb];
      l[pb] += 2 * hrtf[0].powerL[pb] * hrtf[1].powerL[pb] * sqrtIccLfRf * iccRf;
      l[pb] += 2 * hrtf[3].powerL[pb] * hrtf[4].powerL[pb] * sqrtIccLsRs * iccRs;

      r[pb]  = hrtf[0].powerR[pb] * hrtf[0].powerR[pb] * lf[pb];
      r[pb] += hrtf[1].powerR[pb] * hrtf[1].powerR[pb] * rf[pb];
      r[pb] += hrtf[2].powerR[pb] * hrtf[2].powerR[pb] * c[pb];
      r[pb] += hrtf[3].powerR[pb] * hrtf[3].powerR[pb] * ls[pb];
      r[pb] += hrtf[4].powerR[pb] * hrtf[4].powerR[pb] * rs[pb];
      r[pb] += 2 * hrtf[0].powerR[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf * iccLf;
      r[pb] += 2 * hrtf[3].powerR[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs * iccLs;

      temp        = hrtf[0].powerL[pb] * hrtf[0].powerR[pb] * hrtf[0].icc[pb] * lf[pb];
      lrReal[pb]  = (float) cos(hrtf[0].ipd[pb]) * temp;
      lrImag[pb]  = (float) sin(hrtf[0].ipd[pb]) * temp;

      temp        = hrtf[1].powerL[pb] * hrtf[1].powerR[pb] * hrtf[1].icc[pb] * rf[pb];
      lrReal[pb] += (float) cos(hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[1].ipd[pb]) * temp;

      temp        = hrtf[2].powerL[pb] * hrtf[2].powerR[pb] * hrtf[2].icc[pb] * c[pb];
      lrReal[pb] += (float) cos(hrtf[2].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[2].ipd[pb]) * temp;

      temp        = hrtf[3].powerL[pb] * hrtf[3].powerR[pb] * hrtf[3].icc[pb] * ls[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb]) * temp;

      temp        = hrtf[4].powerL[pb] * hrtf[4].powerR[pb] * hrtf[4].icc[pb] * rs[pb];
      lrReal[pb] += (float) cos(hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[4].ipd[pb]) * temp;

      lrReal[pb] += hrtf[0].powerL[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf;
      lrReal[pb] += hrtf[3].powerL[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs;

      temp        = hrtf[1].powerL[pb] * hrtf[0].powerR[pb] * sqrtIccLfRf * hrtf[0].icc[pb] * hrtf[1].icc[pb];
      lrReal[pb] += (float) cos(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;

      temp        = hrtf[4].powerL[pb] * hrtf[3].powerR[pb] * sqrtIccLsRs * hrtf[3].icc[pb] * hrtf[4].icc[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
#endif 
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float temp;

      iid[pb] = (l[pb] + ABS_THR) / (r[pb] + ABS_THR);
      temp    = (float) sqrt(l[pb] * r[pb]) + ABS_THR;
      icc[pb] = ((float) sqrt(lrReal[pb] * lrReal[pb] + lrImag[pb] * lrImag[pb]) + ABS_THR) / temp;

      if (icc[pb] < 0.6f) {
        ipd[pb] = 0;
        icc[pb] = (lrReal[pb] + ABS_THR) / temp;
      }
      else {
        if (pb < 12) {
          ipd[pb] = (float) atan2(lrImag[pb], lrReal[pb]);
        }
        else {
          ipd[pb] = 0;
        }
      }

	    
      if (icc[pb] > 1.0f)
        {
          icc[pb] = 1.0f;
        }
      if (icc[pb] < -1.0f)
        {
          icc[pb] = -1.0f;
        }

    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float gain   = (float) sqrt(l[pb] + r[pb]);
      float cL     = (float) sqrt(iid[pb] / (1 + iid[pb]));
      float cR     = (float) sqrt(1       / (1 + iid[pb]));
      float alpha  = 0.5f * (float) acos(icc[pb]);
      float beta   = (float) atan(tan(alpha) * (cR - cL) / (cR + cL));
      float cosIpd = (float) cos(0.5f * ipd[pb]);
      float sinIpd = (float) sin(0.5f * ipd[pb]);

      H11Real[pb] = gain * cL * (float) cos( alpha + beta);
      H12Real[pb] = gain * cL * (float) sin( alpha + beta);
      H21Real[pb] = gain * cR * (float) cos(-alpha + beta);
      H22Real[pb] = gain * cR * (float) sin(-alpha + beta);

      H11Imag[pb] = H11Real[pb] * sinIpd;
      H12Imag[pb] = H12Real[pb] * sinIpd;
      H21Imag[pb] = H21Real[pb] * -sinIpd;
      H22Imag[pb] = H22Real[pb] * -sinIpd;

      H11Real[pb] *= cosIpd;
      H12Real[pb] *= cosIpd;
      H21Real[pb] *= cosIpd;
      H22Real[pb] *= cosIpd;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][0] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;

#ifndef PARTIALLY_COMPLEX
      if (self->arbitraryDownmix > 0)
        {
          float gReal[1];

          CalculateArbDmxMtx(self, ps, pb, gReal);

          
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];

          
          if (self->arbitraryDownmix == 2) {
            self->M1paramReal[ps][pb][0][1] = 1;
            self->M1paramReal[ps][pb][1][1] = 1;
          }

        }
#endif
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorImag[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][1] = H12Real[pb];
      self->M2decorImag[ps][pb][0][1] = H12Imag[pb];

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorImag[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][1] = H22Real[pb];
      self->M2decorImag[ps][pb][1][1] = H22Imag[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = H11Real[pb];
      self->M2residImag[ps][pb][0][0] = H11Imag[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residImag[ps][pb][0][1] = 0;

      self->M2residReal[ps][pb][1][0] = H21Real[pb];
      self->M2residImag[ps][pb][1][0] = H21Imag[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residImag[ps][pb][1][1] = 0;
    }
#ifdef PARTIALLY_COMPLEX
    if (self->arbitraryDownmix > 0) {
      float gReal[1];

      for (pb = 0; pb < self->numParameterBands; pb++) {
        CalculateArbDmxMtx(self, ps, pb, gReal);

          
        if (self->arbitraryDownmix == 1) {
            
          self->M2decorReal[ps][pb][0][1] *= gReal[0];
          self->M2decorImag[ps][pb][0][1] *= gReal[0];

          self->M2decorReal[ps][pb][1][1] *= gReal[0];
          self->M2decorImag[ps][pb][1][1] *= gReal[0];

            
          self->M2residReal[ps][pb][0][0] *= gReal[0];
          self->M2residImag[ps][pb][0][0] *= gReal[0];

          self->M2residReal[ps][pb][1][0] *= gReal[0];
          self->M2residImag[ps][pb][1][0] *= gReal[0];
        }

          
        if (self->arbitraryDownmix == 2) {
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];

            
          self->M1paramReal[ps][pb][0][1] = 1;
          self->M1paramReal[ps][pb][1][1] = 1;
        }
      }
    }
#endif
  }
}



static void SpatialDecCalculateM1andM2_5122(spatialDec* self) {

  int ps, pb;

  float lf[MAX_PARAMETER_BANDS];
  float rf[MAX_PARAMETER_BANDS];
  float c [MAX_PARAMETER_BANDS];
  float ls[MAX_PARAMETER_BANDS];
  float rs[MAX_PARAMETER_BANDS];
  float l [MAX_PARAMETER_BANDS];
  float r [MAX_PARAMETER_BANDS];
  float lrReal[MAX_PARAMETER_BANDS];
  float lrImag[MAX_PARAMETER_BANDS];
  float iid[MAX_PARAMETER_BANDS];
  float ipd[MAX_PARAMETER_BANDS];
  float icc[MAX_PARAMETER_BANDS];
  float H11Real[MAX_PARAMETER_BANDS];
  float H11Imag[MAX_PARAMETER_BANDS];
  float H12Real[MAX_PARAMETER_BANDS];
  float H12Imag[MAX_PARAMETER_BANDS];
  float H21Real[MAX_PARAMETER_BANDS];
  float H21Imag[MAX_PARAMETER_BANDS];
  float H22Real[MAX_PARAMETER_BANDS];
  float H22Imag[MAX_PARAMETER_BANDS];

  HRTF_DIRECTIONAL_DATA const * hrtf = self->hrtfData->directional;

  for (ps = 0; ps < self->numParameterSets; ps++) {
    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iidPowC  = (float) pow(10.0f, self->ottCLD[0][ps][pb] / 10.0f);
      float iidPowLR = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
      float iidPowL  = (float) pow(10.0f, self->ottCLD[3][ps][pb] / 10.0f);
      float iidPowR  = (float) pow(10.0f, self->ottCLD[4][ps][pb] / 10.0f);

      float powerCs  = iidPowC  / (1 + iidPowC);
      float powerCc  = 1        / (1 + iidPowC);
      float powerLRl = iidPowLR / (1 + iidPowLR);
      float powerLRr = 1        / (1 + iidPowLR);
      float powerLf  = iidPowL  / (1 + iidPowL);
      float powerLs  = 1        / (1 + iidPowL);
      float powerRf  = iidPowR  / (1 + iidPowR);
      float powerRs  = 1        / (1 + iidPowR);

      lf[pb] = powerCs * powerLRl * powerLf;
      rf[pb] = powerCs * powerLRr * powerRf;
      c[pb]  = powerCc;
      ls[pb] = powerCs * powerLRl * powerLs * self->surroundGain * self->surroundGain;
      rs[pb] = powerCs * powerLRr * powerRs * self->surroundGain * self->surroundGain;
    }

#ifdef HRTF_DYNAMIC_UPDATE
    if ((self->hrtfSource != NULL) && ((ps < (self->numParameterSets - 1)) || (!self->extendFrame))) {
      UpdateHRTF(self);
    }
#endif 

    for (pb = 0; pb < self->numParameterBands; pb++) {
#ifndef ICC_FIX
      float sqrtIccLfRf = (float) sqrt(lf[pb] * rf[pb]) * self->ottICC[1][ps][pb];
      float sqrtIccLsRs = (float) sqrt(ls[pb] * rs[pb]) * self->ottICC[1][ps][pb];
      float iccF = hrtf[0].icc[pb] * (float) cos(hrtf[0].ipd[pb]) + hrtf[1].icc[pb] * (float) cos(hrtf[1].ipd[pb]);
      float iccS = hrtf[3].icc[pb] * (float) cos(hrtf[3].ipd[pb]) + hrtf[4].icc[pb] * (float) cos(hrtf[4].ipd[pb]);
      float temp;

      l[pb]  = hrtf[0].powerL[pb] * hrtf[0].powerL[pb] * lf[pb];
      l[pb] += hrtf[1].powerL[pb] * hrtf[1].powerL[pb] * rf[pb];
      l[pb] += hrtf[2].powerL[pb] * hrtf[2].powerL[pb] * c[pb];
      l[pb] += hrtf[3].powerL[pb] * hrtf[3].powerL[pb] * ls[pb];
      l[pb] += hrtf[4].powerL[pb] * hrtf[4].powerL[pb] * rs[pb];
      l[pb] += hrtf[0].powerL[pb] * hrtf[1].powerL[pb] * sqrtIccLfRf * iccF;
      l[pb] += hrtf[3].powerL[pb] * hrtf[4].powerL[pb] * sqrtIccLsRs * iccS;

      r[pb]  = hrtf[0].powerR[pb] * hrtf[0].powerR[pb] * lf[pb];
      r[pb] += hrtf[1].powerR[pb] * hrtf[1].powerR[pb] * rf[pb];
      r[pb] += hrtf[2].powerR[pb] * hrtf[2].powerR[pb] * c[pb];
      r[pb] += hrtf[3].powerR[pb] * hrtf[3].powerR[pb] * ls[pb];
      r[pb] += hrtf[4].powerR[pb] * hrtf[4].powerR[pb] * rs[pb];
      r[pb] += hrtf[0].powerR[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf * iccF;
      r[pb] += hrtf[3].powerR[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs * iccS;

      temp        = hrtf[0].powerL[pb] * hrtf[0].powerR[pb] * hrtf[0].icc[pb] * lf[pb];
      lrReal[pb]  = (float) cos(hrtf[0].ipd[pb]) * temp;
      lrImag[pb]  = (float) sin(hrtf[0].ipd[pb]) * temp;
      temp        = hrtf[1].powerL[pb] * hrtf[1].powerR[pb] * hrtf[1].icc[pb] * rf[pb];
      lrReal[pb] += (float) cos(hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[1].ipd[pb]) * temp;
      temp        = hrtf[2].powerL[pb] * hrtf[2].powerR[pb] * hrtf[2].icc[pb] * c[pb];
      lrReal[pb] += (float) cos(hrtf[2].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[2].ipd[pb]) * temp;
      temp        = hrtf[3].powerL[pb] * hrtf[3].powerR[pb] * hrtf[3].icc[pb] * ls[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb]) * temp;
      temp        = hrtf[4].powerL[pb] * hrtf[4].powerR[pb] * hrtf[4].icc[pb] * rs[pb];
      lrReal[pb] += (float) cos(hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[4].ipd[pb]) * temp;
      lrReal[pb] += hrtf[0].powerL[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf;
      lrReal[pb] += hrtf[3].powerL[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs;
      temp        = hrtf[1].powerL[pb] * hrtf[0].powerR[pb] * sqrtIccLfRf * hrtf[0].icc[pb];
      lrReal[pb] += (float) cos(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      temp        = hrtf[4].powerL[pb] * hrtf[3].powerR[pb] * sqrtIccLsRs * hrtf[3].icc[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
#else 
      float sqrtIccLfRf = (float) sqrt(lf[pb] * rf[pb]) * self->ottICC[1][ps][pb];
      float sqrtIccLsRs = (float) sqrt(ls[pb] * rs[pb]) * self->ottICC[1][ps][pb];
      float iccRf = hrtf[1].icc[pb] * (float) cos(hrtf[1].ipd[pb]);
      float iccRs = hrtf[4].icc[pb] * (float) cos(hrtf[4].ipd[pb]);

      float iccLf = hrtf[0].icc[pb] * (float) cos(hrtf[0].ipd[pb]);
      float iccLs = hrtf[3].icc[pb] * (float) cos(hrtf[3].ipd[pb]);

      float temp;

      l[pb]  = hrtf[0].powerL[pb] * hrtf[0].powerL[pb] * lf[pb];
      l[pb] += hrtf[1].powerL[pb] * hrtf[1].powerL[pb] * rf[pb];
      l[pb] += hrtf[2].powerL[pb] * hrtf[2].powerL[pb] * c[pb];
      l[pb] += hrtf[3].powerL[pb] * hrtf[3].powerL[pb] * ls[pb];
      l[pb] += hrtf[4].powerL[pb] * hrtf[4].powerL[pb] * rs[pb];
      l[pb] += 2 * hrtf[0].powerL[pb] * hrtf[1].powerL[pb] * sqrtIccLfRf * iccRf;
      l[pb] += 2 * hrtf[3].powerL[pb] * hrtf[4].powerL[pb] * sqrtIccLsRs * iccRs;

      r[pb]  = hrtf[0].powerR[pb] * hrtf[0].powerR[pb] * lf[pb];
      r[pb] += hrtf[1].powerR[pb] * hrtf[1].powerR[pb] * rf[pb];
      r[pb] += hrtf[2].powerR[pb] * hrtf[2].powerR[pb] * c[pb];
      r[pb] += hrtf[3].powerR[pb] * hrtf[3].powerR[pb] * ls[pb];
      r[pb] += hrtf[4].powerR[pb] * hrtf[4].powerR[pb] * rs[pb];
      r[pb] += 2 * hrtf[0].powerR[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf * iccLf;
      r[pb] += 2 * hrtf[3].powerR[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs * iccLs;

      temp        = hrtf[0].powerL[pb] * hrtf[0].powerR[pb] * hrtf[0].icc[pb] * lf[pb];
      lrReal[pb]  = (float) cos(hrtf[0].ipd[pb]) * temp;
      lrImag[pb]  = (float) sin(hrtf[0].ipd[pb]) * temp;

      temp        = hrtf[1].powerL[pb] * hrtf[1].powerR[pb] * hrtf[1].icc[pb] * rf[pb];
      lrReal[pb] += (float) cos(hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[1].ipd[pb]) * temp;

      temp        = hrtf[2].powerL[pb] * hrtf[2].powerR[pb] * hrtf[2].icc[pb] * c[pb];
      lrReal[pb] += (float) cos(hrtf[2].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[2].ipd[pb]) * temp;

      temp        = hrtf[3].powerL[pb] * hrtf[3].powerR[pb] * hrtf[3].icc[pb] * ls[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb]) * temp;

      temp        = hrtf[4].powerL[pb] * hrtf[4].powerR[pb] * hrtf[4].icc[pb] * rs[pb];
      lrReal[pb] += (float) cos(hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[4].ipd[pb]) * temp;

      lrReal[pb] += hrtf[0].powerL[pb] * hrtf[1].powerR[pb] * sqrtIccLfRf;
      lrReal[pb] += hrtf[3].powerL[pb] * hrtf[4].powerR[pb] * sqrtIccLsRs;

      temp        = hrtf[1].powerL[pb] * hrtf[0].powerR[pb] * sqrtIccLfRf * hrtf[0].icc[pb] * hrtf[1].icc[pb];
      lrReal[pb] += (float) cos(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[0].ipd[pb] + hrtf[1].ipd[pb]) * temp;

      temp        = hrtf[4].powerL[pb] * hrtf[3].powerR[pb] * sqrtIccLsRs * hrtf[3].icc[pb] * hrtf[4].icc[pb];
      lrReal[pb] += (float) cos(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
      lrImag[pb] += (float) sin(hrtf[3].ipd[pb] + hrtf[4].ipd[pb]) * temp;
#endif 
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float temp;

      iid[pb] = (l[pb] + ABS_THR) / (r[pb] + ABS_THR);
      temp    = (float) sqrt(l[pb] * r[pb]) + ABS_THR;
      icc[pb] = ((float) sqrt(lrReal[pb] * lrReal[pb] + lrImag[pb] * lrImag[pb]) + ABS_THR) / temp;

      if (icc[pb] < 0.6f) {
        ipd[pb] = 0;
        icc[pb] = (lrReal[pb] + ABS_THR) / temp;
      }
      else {
        if (pb < 12) {
          ipd[pb] = (float) atan2(lrImag[pb], lrReal[pb]);
        }
        else {
          ipd[pb] = 0;
        }
      }

	    
      if (icc[pb] > 1.0f)
        {
          icc[pb] = 1.0f;
        }
      if (icc[pb] < -1.0f)
        {
          icc[pb] = -1.0f;
        }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float gain   = (float) sqrt(l[pb] + r[pb]);
      float cL     = (float) sqrt(iid[pb] / (1 + iid[pb]));
      float cR     = (float) sqrt(1       / (1 + iid[pb]));
      float alpha  = 0.5f * (float) acos(icc[pb]);
      float beta   = (float) atan(tan(alpha) * (cR - cL) / (cR + cL));
      float cosIpd = (float) cos(0.5f * ipd[pb]);
      float sinIpd = (float) sin(0.5f * ipd[pb]);

      H11Real[pb] = gain * cL * (float) cos( alpha + beta);
      H12Real[pb] = gain * cL * (float) sin( alpha + beta);
      H21Real[pb] = gain * cR * (float) cos(-alpha + beta);
      H22Real[pb] = gain * cR * (float) sin(-alpha + beta);

      H11Imag[pb] = H11Real[pb] * sinIpd;
      H12Imag[pb] = H12Real[pb] * sinIpd;
      H21Imag[pb] = H21Real[pb] * -sinIpd;
      H22Imag[pb] = H22Real[pb] * -sinIpd;

      H11Real[pb] *= cosIpd;
      H12Real[pb] *= cosIpd;
      H21Real[pb] *= cosIpd;
      H22Real[pb] *= cosIpd;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][0] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;

#ifndef PARTIALLY_COMPLEX
      if (self->arbitraryDownmix > 0)
        {
          float gReal[1];

          CalculateArbDmxMtx(self, ps, pb, gReal);

          
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];

          
          if (self->arbitraryDownmix == 2) {
            self->M1paramReal[ps][pb][0][1] = 1;
            self->M1paramReal[ps][pb][1][1] = 1;
          }

        }
#endif
    }


    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorImag[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][1] = H12Real[pb];
      self->M2decorImag[ps][pb][0][1] = H12Imag[pb];

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorImag[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][1] = H22Real[pb];
      self->M2decorImag[ps][pb][1][1] = H22Imag[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = H11Real[pb];
      self->M2residImag[ps][pb][0][0] = H11Imag[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residImag[ps][pb][0][1] = 0;

      self->M2residReal[ps][pb][1][0] = H21Real[pb];
      self->M2residImag[ps][pb][1][0] = H21Imag[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residImag[ps][pb][1][1] = 0;
    }

#ifdef PARTIALLY_COMPLEX
    if (self->arbitraryDownmix > 0) {
      float gReal[1];

      for (pb = 0; pb < self->numParameterBands; pb++) {
        CalculateArbDmxMtx(self, ps, pb, gReal);

          
        if (self->arbitraryDownmix == 1) {

            
          self->M2decorReal[ps][pb][0][1] *= gReal[0];
          self->M2decorImag[ps][pb][0][1] *= gReal[0];

          self->M2decorReal[ps][pb][1][1] *= gReal[0];
          self->M2decorImag[ps][pb][1][1] *= gReal[0];

            
          self->M2residReal[ps][pb][0][0] *= gReal[0];
          self->M2residImag[ps][pb][0][0] *= gReal[0];

          self->M2residReal[ps][pb][1][0] *= gReal[0];
          self->M2residImag[ps][pb][1][0] *= gReal[0];
        }

          
        if (self->arbitraryDownmix == 2) {
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];

            
          self->M1paramReal[ps][pb][0][1] = 1;
          self->M1paramReal[ps][pb][1][1] = 1;
        }
      }
    }
#endif
  }
}



static void SpatialDecCalculateM1andM2_5151(spatialDec* self) {

  int ps,pb;

  float H11_FS     [MAX_PARAMETER_BANDS];    float H11_C      [MAX_PARAMETER_BANDS];
  float H12_FS     [MAX_PARAMETER_BANDS];    float H12_C      [MAX_PARAMETER_BANDS];
  float H21_FS     [MAX_PARAMETER_BANDS];    float H21_C      [MAX_PARAMETER_BANDS];
  float H22_FS     [MAX_PARAMETER_BANDS];    float H22_C      [MAX_PARAMETER_BANDS];
  float H12_res_FS [MAX_PARAMETER_BANDS];    float H12_res_C  [MAX_PARAMETER_BANDS];
  float H22_res_FS [MAX_PARAMETER_BANDS];    float H22_res_C  [MAX_PARAMETER_BANDS];
  float c_l_FS     [MAX_PARAMETER_BANDS];    float c_l_C      [MAX_PARAMETER_BANDS];
  float c_r_FS     [MAX_PARAMETER_BANDS];    float c_r_C      [MAX_PARAMETER_BANDS];

  float H11_F      [MAX_PARAMETER_BANDS];    float H11_S      [MAX_PARAMETER_BANDS];
  float H12_F      [MAX_PARAMETER_BANDS];    float H12_S      [MAX_PARAMETER_BANDS];
  float H21_F      [MAX_PARAMETER_BANDS];    float H21_S      [MAX_PARAMETER_BANDS];
  float H22_F      [MAX_PARAMETER_BANDS];    float H22_S      [MAX_PARAMETER_BANDS];
  float H12_res_F  [MAX_PARAMETER_BANDS];    float H12_res_S  [MAX_PARAMETER_BANDS];
  float H22_res_F  [MAX_PARAMETER_BANDS];    float H22_res_S  [MAX_PARAMETER_BANDS];
  float c_l_F      [MAX_PARAMETER_BANDS];    float c_l_S      [MAX_PARAMETER_BANDS];
  float c_r_F      [MAX_PARAMETER_BANDS];    float c_r_S      [MAX_PARAMETER_BANDS];

  float c_l_CLFE   [MAX_PARAMETER_BANDS];
  float c_r_CLFE   [MAX_PARAMETER_BANDS];

  for (ps=0; ps < self->numParameterSets; ps++) {


    param2UMX_PS(self, H11_FS, H12_FS, H21_FS, H22_FS, H12_res_FS, H22_res_FS, c_l_FS, c_r_FS, 0, ps, self->resBands[0]);
    param2UMX_PS(self, H11_C,  H12_C,  H21_C,  H22_C,  H12_res_C,  H22_res_C,  c_l_C,  c_r_C,  1, ps, self->resBands[1]);
    param2UMX_PS(self, H11_S,  H12_S,  H21_S,  H22_S,  H12_res_S,  H22_res_S,  c_l_S,  c_r_S,  2, ps, self->resBands[2]);
    param2UMX_PS(self, H11_F,  H12_F,  H21_F,  H22_F,  H12_res_F,  H22_res_F,  c_l_F,  c_r_F,  3, ps, self->resBands[3]);


    for(pb = 0; pb < self->numOttBands[4]; pb++){
      float iid_lin  = (float) (pow(10,( self->ottCLD[4][ps][pb]/20.0f)));
      c_l_CLFE[pb] = (float) (sqrt( iid_lin*iid_lin / ( 1 + iid_lin*iid_lin)));
      c_r_CLFE[pb] = (float) (sqrt(               1 / ( 1 + iid_lin*iid_lin)));
    }
    for(pb = self->numOttBands[4]; pb < self->numParameterBands; pb++){
      c_l_CLFE[pb] = 1;
      c_r_CLFE[pb] = 0;
    }


    for(pb = 0; pb < self->numParameterBands; pb++){
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][0] = 1;
      self->M1paramReal[ps][pb][2][0] = c_l_FS[pb];
      self->M1paramReal[ps][pb][3][0] = c_l_FS[pb] * c_l_C[pb];
      self->M1paramReal[ps][pb][4][0] = c_r_FS[pb];

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;
      self->M1paramImag[ps][pb][2][0] = 0;
      self->M1paramImag[ps][pb][3][0] = 0;
      self->M1paramImag[ps][pb][4][0] = 0;
    }



    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2decorReal[ps][pb][0][0] = H11_F[pb] * H11_C[pb] * H11_FS[pb];
      self->M2decorReal[ps][pb][0][1] = H11_F[pb] * H11_C[pb] * H12_FS[pb];
      self->M2decorReal[ps][pb][0][2] = H11_F[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][0][3] = H12_F[pb];
      self->M2decorReal[ps][pb][0][4] = 0;


      self->M2decorReal[ps][pb][1][0] = H21_F[pb] * H11_C[pb] * H11_FS[pb];
      self->M2decorReal[ps][pb][1][1] = H21_F[pb] * H11_C[pb] * H12_FS[pb];
      self->M2decorReal[ps][pb][1][2] = H21_F[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][1][3] = H22_F[pb];
      self->M2decorReal[ps][pb][1][4] = 0;


      self->M2decorReal[ps][pb][2][0] = c_l_CLFE[pb] * H21_C[pb] * H11_FS[pb];
      self->M2decorReal[ps][pb][2][1] = c_l_CLFE[pb] * H21_C[pb] * H12_FS[pb];
      self->M2decorReal[ps][pb][2][2] = c_l_CLFE[pb] * H22_C[pb];
      self->M2decorReal[ps][pb][2][3] = 0;
      self->M2decorReal[ps][pb][2][4] = 0;


      self->M2decorReal[ps][pb][3][0] = c_l_FS[pb] * c_r_C[pb] * c_r_CLFE[pb];
      self->M2decorReal[ps][pb][3][1] = 0;
      self->M2decorReal[ps][pb][3][2] = 0;
      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = 0;


      self->M2decorReal[ps][pb][4][0] = H11_S[pb] * H21_FS[pb];
      self->M2decorReal[ps][pb][4][1] = H11_S[pb] * H22_FS[pb];
      self->M2decorReal[ps][pb][4][2] = 0;
      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = H12_S[pb];


      self->M2decorReal[ps][pb][5][0] = H21_S[pb] * H21_FS[pb];
      self->M2decorReal[ps][pb][5][1] = H21_S[pb] * H22_FS[pb];
      self->M2decorReal[ps][pb][5][2] = 0;
      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = H22_S[pb];
    }


    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2residReal[ps][pb][0][0] = H11_F[pb] * H11_C[pb] * H11_FS[pb];
      self->M2residReal[ps][pb][0][1] = H11_F[pb] * H11_C[pb] * H12_res_FS[pb];
      self->M2residReal[ps][pb][0][2] = H11_F[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][0][3] = H12_res_F[pb];
      self->M2residReal[ps][pb][0][4] = 0;


      self->M2residReal[ps][pb][1][0] = H21_F[pb] * H11_C[pb] * H11_FS[pb];
      self->M2residReal[ps][pb][1][1] = H21_F[pb] * H11_C[pb] * H12_res_FS[pb];
      self->M2residReal[ps][pb][1][2] = H21_F[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][1][3] = H22_res_F[pb];
      self->M2residReal[ps][pb][1][4] = 0;


      self->M2residReal[ps][pb][2][0] = c_l_CLFE[pb] * H21_C[pb] * H11_FS[pb];
      self->M2residReal[ps][pb][2][1] = c_l_CLFE[pb] * H21_C[pb] * H12_res_FS[pb];
      self->M2residReal[ps][pb][2][2] = c_l_CLFE[pb] * H22_res_C[pb];
      self->M2residReal[ps][pb][2][3] = 0;
      self->M2residReal[ps][pb][2][4] = 0;


      self->M2residReal[ps][pb][3][0] = c_r_CLFE[pb] * c_r_C[pb] * c_l_FS[pb];
      self->M2residReal[ps][pb][3][1] = 0;
      self->M2residReal[ps][pb][3][2] = 0;
      self->M2residReal[ps][pb][3][3] = 0;
      self->M2residReal[ps][pb][3][4] = 0;


      self->M2residReal[ps][pb][4][0] = H11_S[pb] * H21_FS[pb];
      self->M2residReal[ps][pb][4][1] = H11_S[pb] * H22_res_FS[pb];
      self->M2residReal[ps][pb][4][2] = 0;
      self->M2residReal[ps][pb][4][3] = 0;
      self->M2residReal[ps][pb][4][4] = H12_res_S[pb];


      self->M2residReal[ps][pb][5][0] = H21_S[pb] * H21_FS[pb];
      self->M2residReal[ps][pb][5][1] = H21_S[pb] * H22_res_FS[pb];
      self->M2residReal[ps][pb][5][2] = 0;
      self->M2residReal[ps][pb][5][3] = 0;
      self->M2residReal[ps][pb][5][4] = H22_res_S[pb];
    }
  }


  if (self->arbitraryDownmix > 0)
    {
#ifdef PARTIALLY_COMPLEX
      if (self->arbitraryDownmix == 2) {
        imtx_515(self);
      }
#endif

      for (ps = 0; ps < self->numParameterSets; ps++) {
        for (pb = 0; pb < self->numParameterBands; pb++) {
          float gReal[1];

          CalculateArbDmxMtx(self, ps, pb, gReal);

          
          if (self->arbitraryDownmix == 2) {

            self->M1paramReal[ps][pb][0][1] = self->M1paramReal[ps][pb][0][0];
            self->M1paramReal[ps][pb][1][1] = self->M1paramReal[ps][pb][1][0];
            self->M1paramReal[ps][pb][2][1] = self->M1paramReal[ps][pb][2][0];
            self->M1paramReal[ps][pb][3][1] = self->M1paramReal[ps][pb][3][0];
            self->M1paramReal[ps][pb][4][1] = self->M1paramReal[ps][pb][4][0];

            self->M1paramImag[ps][pb][0][1] = self->M1paramImag[ps][pb][0][0];
            self->M1paramImag[ps][pb][1][1] = self->M1paramImag[ps][pb][1][0];
            self->M1paramImag[ps][pb][2][1] = self->M1paramImag[ps][pb][2][0];
            self->M1paramImag[ps][pb][3][1] = self->M1paramImag[ps][pb][3][0];
            self->M1paramImag[ps][pb][4][1] = self->M1paramImag[ps][pb][4][0];
          }

          
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];
          self->M1paramReal[ps][pb][2][0] *= gReal[0];
          self->M1paramReal[ps][pb][3][0] *= gReal[0];
          self->M1paramReal[ps][pb][4][0] *= gReal[0];
        }
      }
    }

#ifdef PARTIALLY_COMPLEX
  if (self->arbitraryDownmix != 2) {
    imtx_515(self);
  }
#endif
}



static void SpatialDecCalculateM1andM2_5152(spatialDec* self) {

  int ps,pb;

  float H11_LR     [MAX_PARAMETER_BANDS];      float H11_C     [MAX_PARAMETER_BANDS];
  float H12_LR     [MAX_PARAMETER_BANDS];      float H12_C     [MAX_PARAMETER_BANDS];
  float H21_LR     [MAX_PARAMETER_BANDS];      float H21_C     [MAX_PARAMETER_BANDS];
  float H22_LR     [MAX_PARAMETER_BANDS];      float H22_C     [MAX_PARAMETER_BANDS];
  float H12_res_LR [MAX_PARAMETER_BANDS];      float H12_res_C [MAX_PARAMETER_BANDS];
  float H22_res_LR [MAX_PARAMETER_BANDS];      float H22_res_C [MAX_PARAMETER_BANDS];
  float c_l_LR     [MAX_PARAMETER_BANDS];      float c_l_C     [MAX_PARAMETER_BANDS];
  float c_r_LR     [MAX_PARAMETER_BANDS];      float c_r_C     [MAX_PARAMETER_BANDS];

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_l_L     [MAX_PARAMETER_BANDS];       float c_l_R     [MAX_PARAMETER_BANDS];
  float c_r_L     [MAX_PARAMETER_BANDS];       float c_r_R     [MAX_PARAMETER_BANDS];

  float c_l_CLFE  [MAX_PARAMETER_BANDS];
  float c_r_CLFE  [MAX_PARAMETER_BANDS];



  for (ps=0; ps < self->numParameterSets; ps++) {
    param2UMX_PS(self, H11_C,  H12_C,  H21_C,  H22_C,  H12_res_C,  H22_res_C,  c_l_C,  c_r_C,  0, ps, self->resBands[0]);
    param2UMX_PS(self, H11_LR, H12_LR, H21_LR, H22_LR, H12_res_LR, H22_res_LR, c_l_LR, c_r_LR, 1, ps, self->resBands[1]);
    param2UMX_PS(self, H11_L,  H12_L,  H21_L,  H22_L,  H12_res_L,  H22_res_L,  c_l_L,  c_r_L,  3, ps, self->resBands[3]);
    param2UMX_PS(self, H11_R,  H12_R,  H21_R,  H22_R,  H12_res_R,  H22_res_R,  c_l_R,  c_r_R,  4, ps, self->resBands[4]);


    for(pb = 0; pb < self->numOttBands[2]; pb++){
      float iid_lin  = (float) (pow(10,( self->ottCLD[2][ps][pb]/20.0f)));
      c_l_CLFE[pb] = (float) (sqrt( iid_lin*iid_lin / ( 1 + iid_lin*iid_lin)));
      c_r_CLFE[pb] = (float) (sqrt(               1 / ( 1 + iid_lin*iid_lin)));
    }
    for(pb = self->numOttBands[2]; pb < self->numParameterBands; pb++){
      c_l_CLFE[pb] = 1;
      c_r_CLFE[pb] = 0;
    }

    for(pb = 0; pb < self->numParameterBands; pb++){
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][0] = 1;
      self->M1paramReal[ps][pb][2][0] = c_l_C[pb];
      self->M1paramReal[ps][pb][3][0] = c_l_C[pb] * c_l_LR[pb];
      self->M1paramReal[ps][pb][4][0] = c_l_C[pb] * c_r_LR[pb];

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;
      self->M1paramImag[ps][pb][2][0] = 0;
      self->M1paramImag[ps][pb][3][0] = 0;
      self->M1paramImag[ps][pb][4][0] = 0;

    }



    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2decorReal[ps][pb][0][0] = H11_L[pb] * H11_LR[pb] * H11_C[pb];
      self->M2decorReal[ps][pb][0][1] = H11_L[pb] * H11_LR[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][0][2] = H11_L[pb] * H12_LR[pb];
      self->M2decorReal[ps][pb][0][3] = H12_L[pb];
      self->M2decorReal[ps][pb][0][4] = 0;


      self->M2decorReal[ps][pb][1][0] = H21_L[pb] * H11_LR[pb] * H11_C[pb];
      self->M2decorReal[ps][pb][1][1] = H21_L[pb] * H11_LR[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][1][2] = H21_L[pb] * H12_LR[pb];
      self->M2decorReal[ps][pb][1][3] = H22_L[pb];
      self->M2decorReal[ps][pb][1][4] = 0;


      self->M2decorReal[ps][pb][2][0] = H11_R[pb] * H21_LR[pb] * H11_C[pb];
      self->M2decorReal[ps][pb][2][1] = H11_R[pb] * H21_LR[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][2][2] = H11_R[pb] * H22_LR[pb];
      self->M2decorReal[ps][pb][2][3] = 0;
      self->M2decorReal[ps][pb][2][4] = H12_R[pb];


      self->M2decorReal[ps][pb][3][0] = H21_R[pb] * H21_LR[pb] * H11_C[pb];
      self->M2decorReal[ps][pb][3][1] = H21_R[pb] * H21_LR[pb] * H12_C[pb];
      self->M2decorReal[ps][pb][3][2] = H21_R[pb] * H22_LR[pb];
      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = H22_R[pb];


      self->M2decorReal[ps][pb][4][0] = c_l_CLFE[pb] * H21_C[pb];
      self->M2decorReal[ps][pb][4][1] = c_l_CLFE[pb] * H22_C[pb];
      self->M2decorReal[ps][pb][4][2] = 0;
      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = 0;


      self->M2decorReal[ps][pb][5][0] = c_r_C[pb] * c_r_CLFE[pb];
      self->M2decorReal[ps][pb][5][1] = 0;
      self->M2decorReal[ps][pb][5][2] = 0;
      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = 0;
    }


    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2residReal[ps][pb][0][0] = H11_L[pb] * H11_LR[pb] * H11_C[pb];
      self->M2residReal[ps][pb][0][1] = H11_L[pb] * H11_LR[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][0][2] = H11_L[pb] * H12_res_LR[pb];
      self->M2residReal[ps][pb][0][3] = H12_res_L[pb];
      self->M2residReal[ps][pb][0][4] = 0;


      self->M2residReal[ps][pb][1][0] = H21_L[pb] * H11_LR[pb] * H11_C[pb];
      self->M2residReal[ps][pb][1][1] = H21_L[pb] * H11_LR[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][1][2] = H21_L[pb] * H12_res_LR[pb];
      self->M2residReal[ps][pb][1][3] = H22_res_L[pb];
      self->M2residReal[ps][pb][1][4] = 0;


      self->M2residReal[ps][pb][2][0] = H11_R[pb] * H21_LR[pb] * H11_C[pb];
      self->M2residReal[ps][pb][2][1] = H11_R[pb] * H21_LR[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][2][2] = H11_R[pb] * H22_res_LR[pb];
      self->M2residReal[ps][pb][2][3] = 0;
      self->M2residReal[ps][pb][2][4] = H12_res_R[pb];


      self->M2residReal[ps][pb][3][0] = H21_R[pb] * H21_LR[pb] * H11_C[pb];
      self->M2residReal[ps][pb][3][1] = H21_R[pb] * H21_LR[pb] * H12_res_C[pb];
      self->M2residReal[ps][pb][3][2] = H21_R[pb] * H22_res_LR[pb];
      self->M2residReal[ps][pb][3][3] = 0;
      self->M2residReal[ps][pb][3][4] = H22_res_R[pb];


      self->M2residReal[ps][pb][4][0] = c_l_CLFE[pb] * H21_C[pb];
      self->M2residReal[ps][pb][4][1] = c_l_CLFE[pb] * H22_res_C[pb];
      self->M2residReal[ps][pb][4][2] = 0;
      self->M2residReal[ps][pb][4][3] = 0;
      self->M2residReal[ps][pb][4][4] = 0;


      self->M2residReal[ps][pb][5][0] = c_r_C[pb] * c_r_CLFE[pb];
      self->M2residReal[ps][pb][5][1] = 0;
      self->M2residReal[ps][pb][5][2] = 0;
      self->M2residReal[ps][pb][5][3] = 0;
      self->M2residReal[ps][pb][5][4] = 0;
    }
  }

  if (self->arbitraryDownmix > 0)
    {
#ifdef PARTIALLY_COMPLEX
      if (self->arbitraryDownmix == 2) {
        imtx_515(self);
      }
#endif

      for (ps = 0; ps < self->numParameterSets; ps++) {
        for (pb = 0; pb < self->numParameterBands; pb++) {
          float gReal[1];

          CalculateArbDmxMtx(self, ps, pb, gReal);


          
          if (self->arbitraryDownmix == 2) {

            self->M1paramReal[ps][pb][0][1] = self->M1paramReal[ps][pb][0][0];
            self->M1paramReal[ps][pb][1][1] = self->M1paramReal[ps][pb][1][0];
            self->M1paramReal[ps][pb][2][1] = self->M1paramReal[ps][pb][2][0];
            self->M1paramReal[ps][pb][3][1] = self->M1paramReal[ps][pb][3][0];
            self->M1paramReal[ps][pb][4][1] = self->M1paramReal[ps][pb][4][0];

            self->M1paramImag[ps][pb][0][1] = self->M1paramImag[ps][pb][0][0];
            self->M1paramImag[ps][pb][1][1] = self->M1paramImag[ps][pb][1][0];
            self->M1paramImag[ps][pb][2][1] = self->M1paramImag[ps][pb][2][0];
            self->M1paramImag[ps][pb][3][1] = self->M1paramImag[ps][pb][3][0];
            self->M1paramImag[ps][pb][4][1] = self->M1paramImag[ps][pb][4][0];
          }

          
          self->M1paramReal[ps][pb][0][0] *= gReal[0];
          self->M1paramReal[ps][pb][1][0] *= gReal[0];
          self->M1paramReal[ps][pb][2][0] *= gReal[0];
          self->M1paramReal[ps][pb][3][0] *= gReal[0];
          self->M1paramReal[ps][pb][4][0] *= gReal[0];
        }
      }
    }

#ifdef PARTIALLY_COMPLEX
  if (self->arbitraryDownmix != 2) {
    imtx_515(self);
  }
#endif

}



static void SpatialDecCalculateM1andM2_51S1(spatialDec* self) {

  int ps,pb;

  float iid     [MAX_PARAMETER_BANDS];
  float icc     [MAX_PARAMETER_BANDS];
  float H11     [MAX_PARAMETER_BANDS];
  float H12     [MAX_PARAMETER_BANDS];
  float H21     [MAX_PARAMETER_BANDS];
  float H22     [MAX_PARAMETER_BANDS];
  float H12_res [MAX_PARAMETER_BANDS];
  float H22_res [MAX_PARAMETER_BANDS];
  float c_l     [MAX_PARAMETER_BANDS];
  float c_r     [MAX_PARAMETER_BANDS];

  for (ps=0; ps < self->numParameterSets; ps++) {

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iid_pow;
      float p_l_FS, p_r_FS;
      float p_l_C, p_r_C;
      float p_l_S, p_r_S;
      float p_l_F, p_r_F;
      float left_f;
      float right_f;
      float center;
      float left_s;
      float right_s;
      float left;
      float right;
      float cross;

      iid_pow = (float) pow(10.0f, self->ottCLD[0][ps][pb] / 10.0f);
      p_l_FS  = iid_pow / (1.0f + iid_pow);
      p_r_FS  =    1.0f / (1.0f + iid_pow);

      iid_pow = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
      p_l_C   = iid_pow / (1.0f + iid_pow);
      p_r_C   =    1.0f / (1.0f + iid_pow);

      iid_pow = (float) pow(10.0f, self->ottCLD[2][ps][pb] / 10.0f);
      p_l_S   = iid_pow / (1.0f + iid_pow);
      p_r_S   =    1.0f / (1.0f + iid_pow);

      iid_pow = (float) pow(10.0f, self->ottCLD[3][ps][pb] / 10.0f);
      p_l_F   = iid_pow / (1.0f + iid_pow);
      p_r_F   =    1.0f / (1.0f + iid_pow);

      left_f  = p_l_FS * p_l_C * p_l_F;
      right_f = p_l_FS * p_l_C * p_r_F;
      center  = p_l_FS * p_r_C * 0.5f;
      left_s  = p_r_FS * p_l_S;
      right_s = p_r_FS * p_r_S;

      left  = center + left_f + left_s;
      right = center + right_f + right_s;
      cross = center + self->ottICC[3][ps][pb] * (float) sqrt(left_f * right_f) + self->ottICC[2][ps][pb] * (float) sqrt(left_s * right_s);

      iid[pb] = 10.0f * (float) log10((left + ABS_THR) / (right + ABS_THR));
      icc[pb] = (cross + ABS_THR) / ((float) sqrt(left * right) + ABS_THR);

      if (icc[pb] > 1.0f) {
        icc[pb] = 1.0f;
      }
      else {
        if (icc[pb] < -0.99f) {
          icc[pb] = -0.99f;
        }
      }
 
#ifdef QUANTIZE_PARS_FIX
      iid[pb] = SpatialDecQuantizeCLD(iid[pb]);
      icc[pb] = SpatialDecQuantizeICC(icc[pb]);
#endif
    }

    param2UMX_PS_Core(iid, icc, self->numParameterBands, 0, H11, H12, H21, H22, H12_res, H22_res, c_l, c_r);

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][3][0] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][3][0] = 0;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][3] = H12[pb];

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][3] = H22[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2residReal[ps][pb][0][0] = H11[pb];
      self->M2residReal[ps][pb][0][3] = 0;

      self->M2residReal[ps][pb][1][0] = H21[pb];
      self->M2residReal[ps][pb][1][3] = 0;
    }
  }
}



static void SpatialDecCalculateM1andM2_51S2(spatialDec* self) {

  int ps,pb;

  float iid     [MAX_PARAMETER_BANDS];
  float icc     [MAX_PARAMETER_BANDS];
  float H11     [MAX_PARAMETER_BANDS];
  float H12     [MAX_PARAMETER_BANDS];
  float H21     [MAX_PARAMETER_BANDS];
  float H22     [MAX_PARAMETER_BANDS];
  float H12_res [MAX_PARAMETER_BANDS];
  float H22_res [MAX_PARAMETER_BANDS];
  float c_l     [MAX_PARAMETER_BANDS];
  float c_r     [MAX_PARAMETER_BANDS];
  float g_s     [MAX_PARAMETER_BANDS];

  for (ps=0; ps < self->numParameterSets; ps++) {

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iid_pow;
      float p_l_C, p_r_C;
      float p_l_LR, p_r_LR;
      float left;
      float right;
      float center;
      float cross;

      iid_pow = (float) pow(10.0f, self->ottCLD[0][ps][pb] / 10.0f);
      p_l_C   = iid_pow / (1.0f + iid_pow);
      p_r_C   =    1.0f / (1.0f + iid_pow);

      iid_pow = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
      p_l_LR  = iid_pow / (1.0f + iid_pow);
      p_r_LR  =    1.0f / (1.0f + iid_pow);

      left   = p_l_C * p_l_LR;
      right  = p_l_C * p_r_LR;
      center = p_r_C * 0.5f;
      cross  = self->ottICC[1][ps][pb] * (float) sqrt(left * right);
      cross += center + self->ottICC[0][ps][pb] * (float) sqrt(center * (left + right + cross));
      left  += center + 2.0f * self->ottICC[0][ps][pb] * (float) sqrt(center * left);
      right += center + 2.0f * self->ottICC[0][ps][pb] * (float) sqrt(center * right);

      iid[pb] = 10.0f * (float) log10((left + ABS_THR) / (right + ABS_THR));
      icc[pb] = (cross + ABS_THR) / ((float) sqrt(left * right) + ABS_THR);
      g_s[pb] = (float) sqrt(left + right);

      if (icc[pb] > 1.0f) {
        icc[pb] = 1.0f;
      }
      else {
        if (icc[pb] < -0.99f) {
          icc[pb] = -0.99f;
        }
      }

#ifdef QUANTIZE_PARS_FIX
      iid[pb] = SpatialDecQuantizeCLD(iid[pb]);
      icc[pb] = SpatialDecQuantizeICC(icc[pb]);
#endif
    }

    param2UMX_PS_Core(iid, icc, self->numParameterBands, 0, H11, H12, H21, H22, H12_res, H22_res, c_l, c_r);

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][2][0] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][2][0] = 0;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][2] = g_s[pb] * H12[pb];

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][2] = g_s[pb] * H22[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {

      self->M2residReal[ps][pb][0][0] = g_s[pb] * H11[pb];
      self->M2residReal[ps][pb][0][2] = 0;

      self->M2residReal[ps][pb][1][0] = g_s[pb] * H21[pb];
      self->M2residReal[ps][pb][1][2] = 0;
    }
  }
}



static void CalculateTTT(spatialDec* self, int ps, int pb, int TTTmode, float M_ttt[3][3])
{
  int row, col;

  if(TTTmode < 2) {
    M_ttt[0][0] = self->tttCPC1[0][ps][pb] + 2;
    M_ttt[0][1] = self->tttCPC2[0][ps][pb] - 1;
    M_ttt[1][0] = self->tttCPC1[0][ps][pb] - 1;
    M_ttt[1][1] = self->tttCPC2[0][ps][pb] + 2;
    M_ttt[2][0] = 1 - self->tttCPC1[0][ps][pb];
    M_ttt[2][1] = 1 - self->tttCPC2[0][ps][pb];

    if(pb >= self->resBands[3]){
      M_ttt[0][0] /= self->tttICC[0][ps][pb];
      M_ttt[0][1] /= self->tttICC[0][ps][pb];
      M_ttt[1][0] /= self->tttICC[0][ps][pb];
      M_ttt[1][1] /= self->tttICC[0][ps][pb];
      M_ttt[2][0] /= self->tttICC[0][ps][pb];
      M_ttt[2][1] /= self->tttICC[0][ps][pb];
    }
  }
  else {
    int centerWiener;
    int centerSubtraction;
    float q = 1;
    float c1d = (float) pow(10,self->tttCLD1[0][ps][pb]/10.0f);
    float c2d = (float) pow(10,self->tttCLD2[0][ps][pb]/10.0f);

    M_ttt[0][0] = (float) (3*sqrt(c1d*c2d/(c2d+1+c1d*c2d)));
    M_ttt[0][1] = 0;
    M_ttt[1][0] = 0;
    M_ttt[1][1] = (float) (3*sqrt(c1d/(c1d+c2d+1)));
    M_ttt[2][0] = (float) (3*0.5*sqrt((c2d+1)/(c2d+1+c1d*c2d)));
    M_ttt[2][1] = (float) (3*0.5*sqrt((c2d+1)/(c1d+c2d+1)));

    centerWiener = 0;
    centerSubtraction = (TTTmode == 2 || TTTmode == 3);

    if(centerWiener){
      M_ttt[2][0] = (float) (3*q*1.0/sqrt(c1d*c2d+(q*q)*(1+c2d)*(1+c2d)));
      M_ttt[2][1] = (float) (c2d*M_ttt[2][0]);
    }

    if(centerSubtraction){
      float C = 1/(1+c1d);
      float R = C*c1d/(1+c2d);
      float L = c2d*R;


      float wl1 = (float) sqrt((R+q*q*C)/(R+q*q*C*(1+R/L)));
      float wl2 = -q*q*C*wl1/(R+q*q*C);


      float wr1 = (float) sqrt((L+q*q*C)/(L+q*q*C*(1+L/R)));
      float wr2 = -q*q*C*wr1/(L+q*q*C);

      M_ttt[0][0] = 3 * wl1;
      M_ttt[0][1] = 3 * wl2;
      M_ttt[1][0] = 3 * wr2;
      M_ttt[1][1] = 3 * wr1;
    }
  }

      
  M_ttt[0][2] =  1;
  M_ttt[1][2] =  1;
  M_ttt[2][2] = -1;

    
  for(row = 0; row < 3; row++){
    for(col = 0; col < 3; col++){
      M_ttt[row][col] *= (1.0f/3.0f);
    }
  }

    
  for(col = 0; col < 3; col++) {
    M_ttt[2][col] *= ((float) (sqrt(2)));
  }
}



static void CalculateMTXinv(spatialDec* self, int ps, int pb, int mode, float H_real[2][2], float H_imag[2][2])
{
  float iidLeft = self->ottCLD[1][ps][pb];
  float iidRight = self->ottCLD[2][ps][pb];
  float c1;
  float c2;
  int predictionMode;
  float weight1;
  float weight2;
#ifdef PARTIALLY_COMPLEX 
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif 

  if (mode < 2) {
    c1 = self->tttCPC1[0][ps][pb];
    c2 = self->tttCPC2[0][ps][pb];
    predictionMode = 1;
  }
  else {
    c1 = self->tttCLD1[0][ps][pb];
    c2 = self->tttCLD2[0][ps][pb];
    predictionMode = 0;
  }

  getMatrixInversionWeights(iidLeft, iidRight, predictionMode, c1, c2, &weight1, &weight2);

#ifdef PARTIALLY_COMPLEX
  if (pb < complexBands) {
    invertMatrix(weight1, weight2, H_real, H_imag);
  }
  else {
    invertMatrixReal(weight1, weight2, H_real);
    H_imag[0][0] = 0.0f;
    H_imag[0][1] = 0.0f;
    H_imag[1][0] = 0.0f;
    H_imag[1][1] = 0.0f;
  }
#else
  invertMatrix(weight1, weight2, H_real, H_imag);
#endif
}



static void svd2x2(float x_real[2][2],
                   float x_imag[2][2],
                   float u_real[2][2],
                   float u_imag[2][2],
                   float s[2],
                   float v_real[2][2],
                   float v_imag[2][2])
{
  float p_real[2][2];
  float p_imag[2][2];
  float poly[3];
  float temp;
  float root[2];

  p_real[0][0] = x_real[0][0] * x_real[0][0] + x_imag[0][0] * x_imag[0][0] + x_real[1][0] * x_real[1][0] + x_imag[1][0] * x_imag[1][0];
  p_imag[0][0] = 0.0f;
  p_real[0][1] = x_real[0][0] * x_real[0][1] + x_imag[0][0] * x_imag[0][1] + x_real[1][0] * x_real[1][1] + x_imag[1][0] * x_imag[1][1];
  p_imag[0][1] = x_real[0][0] * x_imag[0][1] - x_real[0][1] * x_imag[0][0] + x_real[1][0] * x_imag[1][1] - x_real[1][1] * x_imag[1][0];
  p_real[1][0] = p_real[0][1];
  p_imag[1][0] = -p_imag[0][1];
  p_real[1][1] = x_real[0][1] * x_real[0][1] + x_imag[0][1] * x_imag[0][1] + x_real[1][1] * x_real[1][1] + x_imag[1][1] * x_imag[1][1];
  p_imag[1][1] = 0.0f;

  poly[2] = 1.0f;
  poly[1] = p_real[0][0] + p_real[1][1];
  poly[0] = p_real[0][0] * p_real[1][1] - p_real[0][1] * p_real[1][0] + p_imag[0][1] * p_imag[1][0];

  temp = (float) sqrt(poly[1] * poly[1] - 4 * poly[0]);

  root[0] = (poly[1] + temp) / 2.0f;
  root[1] = (poly[1] - temp) / 2.0f;

  if (root[0] < root[1]) {
    float swap = root[0];
    root[0] = root[1];
    root[1] = swap;
  }

  s[0] = (float) sqrt(root[0]);
  s[1] = (float) sqrt(root[1]);
  
  root[0] -= p_real[0][0];
  root[1] -= p_real[0][0];

  temp = (float) -sqrt(p_real[0][1] * p_real[0][1] + p_imag[0][1] * p_imag[0][1] + root[0] * root[0]);

  v_real[0][0] = p_real[0][1] / temp;
  v_imag[0][0] = p_imag[0][1] / temp;
  v_real[1][0] = root[0] / temp;
  v_imag[1][0] = 0.0f;

  temp = (float) -sqrt(p_real[0][1] * p_real[0][1] + p_imag[0][1] * p_imag[0][1] + root[1] * root[1]);

  v_real[0][1] = p_real[0][1] / temp;
  v_imag[0][1] = p_imag[0][1] / temp;
  v_real[1][1] = root[1] / temp;
  v_imag[1][1] = 0.0f;

  u_real[0][0] = x_real[0][0] * v_real[0][0] - x_imag[0][0] * v_imag[0][0] + v_real[1][0] * x_real[0][1];
  u_imag[0][0] = x_imag[0][0] * v_real[0][0] + x_real[0][0] * v_imag[0][0] + v_real[1][0] * x_imag[0][1];
  u_real[0][1] = x_real[0][0] * v_real[0][1] - x_imag[0][0] * v_imag[0][1] + v_real[1][1] * x_real[0][1];
  u_imag[0][1] = x_imag[0][0] * v_real[0][1] + x_real[0][0] * v_imag[0][1] + v_real[1][1] * x_imag[0][1];
  u_real[1][0] = x_real[1][0] * v_real[0][0] - x_imag[1][0] * v_imag[0][0] + v_real[1][0] * x_real[1][1];
  u_imag[1][0] = x_imag[1][0] * v_real[0][0] + x_real[1][0] * v_imag[0][0] + v_real[1][0] * x_imag[1][1];
  u_real[1][1] = x_real[1][0] * v_real[0][1] - x_imag[1][0] * v_imag[0][1] + v_real[1][1] * x_real[1][1];
  u_imag[1][1] = x_imag[1][0] * v_real[0][1] + x_real[1][0] * v_imag[0][1] + v_real[1][1] * x_imag[1][1];

  temp = (float) sqrt(u_real[0][0] * u_real[0][0] + u_imag[0][0] * u_imag[0][0] + u_real[1][0] * u_real[1][0] + u_imag[1][0] * u_imag[1][0]);

  u_real[0][0] /= temp;
  u_imag[0][0] /= temp;
  u_real[1][0] /= temp;
  u_imag[1][0] /= temp;

  temp = (float) sqrt(u_real[0][1] * u_real[0][1] + u_imag[0][1] * u_imag[0][1] + u_real[1][1] * u_real[1][1] + u_imag[1][1] * u_imag[1][1]);

  u_real[0][1] /= temp;
  u_imag[0][1] /= temp;
  u_real[1][1] /= temp;
  u_imag[1][1] /= temp;
}




static void Unsingularize3DMatrix(float *h11_real, float *h12_real, float *h21_real, float *h22_real, 
                                  float *h11_imag, float *h12_imag, float *h21_imag, float *h22_imag, 
                                  float *inv_norm_real, float *inv_norm_imag, float *inv_norm_abs)
{
#define DETERMINANT_THRESHOLD           (0.1f)
#define DETERMINANT_THRESHOLD_SQRT      (0.3162277660168379f)

  if (sqrt(*inv_norm_abs) < DETERMINANT_THRESHOLD) {
    float h_real[2][2];
    float h_imag[2][2];
    float u_real[2][2];
    float u_imag[2][2];
    float s[2];
    float v_real[2][2];
    float v_imag[2][2];

    h_real[0][0] = *h11_real;
    h_real[0][1] = *h12_real;
    h_real[1][0] = *h21_real;
    h_real[1][1] = *h22_real;

    h_imag[0][0] = *h11_imag;
    h_imag[0][1] = *h12_imag;
    h_imag[1][0] = *h21_imag;
    h_imag[1][1] = *h22_imag;

    svd2x2(h_real, h_imag, u_real, u_imag, s, v_real, v_imag);

    if ((s[0] < DETERMINANT_THRESHOLD_SQRT) && (s[1] < DETERMINANT_THRESHOLD_SQRT)) {
      s[0] = DETERMINANT_THRESHOLD_SQRT;
      s[1] = DETERMINANT_THRESHOLD_SQRT;
    }
    else{
      s[1] += (DETERMINANT_THRESHOLD - s[0] * s[1]) / s[0];
    }

    u_real[0][0] *= s[0];
    u_imag[0][0] *= s[0];
    u_real[0][1] *= s[1];
    u_imag[0][1] *= s[1];
    u_real[1][0] *= s[0];
    u_imag[1][0] *= s[0];
    u_real[1][1] *= s[1];
    u_imag[1][1] *= s[1];

    *h11_real = u_real[0][0] * v_real[0][0] + u_imag[0][0] * v_imag[0][0] + u_real[0][1] * v_real[0][1] + u_imag[0][1] * v_imag[0][1];
    *h11_imag = u_imag[0][0] * v_real[0][0] - u_real[0][0] * v_imag[0][0] + u_imag[0][1] * v_real[0][1] - u_real[0][1] * v_imag[0][1];
    *h21_real = u_real[1][0] * v_real[0][0] + u_imag[1][0] * v_imag[0][0] + u_real[1][1] * v_real[0][1] + u_imag[1][1] * v_imag[0][1];
    *h21_imag = u_imag[1][0] * v_real[0][0] - u_real[1][0] * v_imag[0][0] + u_imag[1][1] * v_real[0][1] - u_real[1][1] * v_imag[0][1];
    *h12_real = u_real[0][0] * v_real[1][0] + u_real[0][1] * v_real[1][1];
    *h12_imag = u_imag[0][0] * v_real[1][0] + u_imag[0][1] * v_real[1][1];
    *h22_real = u_real[1][0] * v_real[1][0] + u_real[1][1] * v_real[1][1];
    *h22_imag = u_imag[1][0] * v_real[1][0] + u_imag[1][1] * v_real[1][1];

    *inv_norm_real = (*h11_real) * (*h22_real) - (*h11_imag) * (*h22_imag) - (*h12_real) * (*h21_real) + (*h12_imag) * (*h21_imag);
    *inv_norm_imag = (*h11_real) * (*h22_imag) + (*h11_imag) * (*h22_real) - (*h21_real) * (*h12_imag) - (*h21_imag) * (*h12_real);

    *inv_norm_abs  = (*inv_norm_real) * (*inv_norm_real) + (*inv_norm_imag) * (*inv_norm_imag);
  }
}




static void Calculate3Dinv(spatialDec* self, int ps, int pb, float M_pre[3][5], float Q_real[2][2], float Q_imag[2][2])
{
  float pc1Real, pc1Imag, pc2Real, pc2Imag;
  float pl1Real, pl1Imag, pl2Real, pl2Imag;
  float pr1Real, pr1Imag, pr2Real, pr2Imag;

  float h11_real, h11_imag, h12_real, h12_imag;
  float h21_real, h21_imag, h22_real, h22_imag;

  float inv_norm_real, inv_norm_imag, inv_norm_abs;


  HRTF_DIRECTIONAL_DATA *hrtf[HRTF_AZIMUTHS];
  int ch;

    
  for(ch = 0; ch < self->_3DStereoData.hrtfNumChannels; ch++) {
    hrtf[ch] = &self->_3DStereoData.directional[ch];
  }

  { 
    float temp1;
    float temp2;

    temp1 = (float) sqrt(2) * hrtf[2]->powerL[pb];
    temp2 = (float) sqrt(2) * hrtf[2]->powerR[pb];

    if (pb < 12) {
      pc1Real = (float) cos(hrtf[2]->ipd[pb] / 2) * temp1;
      pc1Imag = (float) sin(hrtf[2]->ipd[pb] / 2) * temp1;
      pc2Real = (float) cos(-hrtf[2]->ipd[pb] / 2) * temp2;
      pc2Imag = (float) sin(-hrtf[2]->ipd[pb] / 2) * temp2;
    }
    else {
      pc1Real = temp1;
      pc1Imag = 0;
      pc2Real = temp2;
      pc2Imag = 0;
    }
  }

  { 
    float iid = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
    float lf = iid / (1 + iid);
    float ls = 1   / (1 + iid);

    float lfL = lf * hrtf[0]->powerL[pb] * hrtf[0]->powerL[pb];
    float lfR = lf * hrtf[0]->powerR[pb] * hrtf[0]->powerR[pb];
    float lsL = ls * hrtf[3]->powerL[pb] * hrtf[3]->powerL[pb] * self->surroundGain * self->surroundGain;
    float lsR = ls * hrtf[3]->powerR[pb] * hrtf[3]->powerR[pb] * self->surroundGain * self->surroundGain;

    float temp = (float) sqrt(lfR + lsR);

    pl1Real = (float) sqrt(lfL + lsL);
    pl1Imag = 0.f;

    if (pb < 12) {
      pl2Real = (float) cos(-(lfR * hrtf[0]->ipd[pb] + lsR * hrtf[3]->ipd[pb]) / (lfR + lsR)) * temp;
      pl2Imag = (float) sin(-(lfR * hrtf[0]->ipd[pb] + lsR * hrtf[3]->ipd[pb]) / (lfR + lsR)) * temp;
    }
    else {
      pl2Real = temp;
      pl2Imag = 0;
    }
  }

  { 
    float iid = (float) pow(10.0f, self->ottCLD[2][ps][pb] / 10.0f);
    float rf = iid / (1 + iid);
    float rs = 1   / (1 + iid);

    float rfL = rf * hrtf[1]->powerL[pb] * hrtf[1]->powerL[pb];
    float rfR = rf * hrtf[1]->powerR[pb] * hrtf[1]->powerR[pb];
    float rsL = rs * hrtf[4]->powerL[pb] * hrtf[4]->powerL[pb] * self->surroundGain * self->surroundGain;
    float rsR = rs * hrtf[4]->powerR[pb] * hrtf[4]->powerR[pb] * self->surroundGain * self->surroundGain;

    float temp = (float) sqrt(rfL + rsL);

    pr2Real = (float) sqrt(rfR + rsR);
    pr2Imag = 0.f;

    if (pb < 12) {
      pr1Real = (float) cos((rfL * hrtf[1]->ipd[pb] + rsL * hrtf[4]->ipd[pb]) / (rfL + rsL)) * temp;
      pr1Imag = (float) sin((rfL * hrtf[1]->ipd[pb] + rsL * hrtf[4]->ipd[pb]) / (rfL + rsL)) * temp;
    }
    else {
      pr1Real = temp;
      pr1Imag = 0;
    }
  }

  
  h11_real = M_pre[0][0] * pl1Real + M_pre[1][0] * pr1Real + M_pre[2][0] * pc1Real;
  h11_imag = M_pre[0][0] * pl1Imag + M_pre[1][0] * pr1Imag + M_pre[2][0] * pc1Imag;
  h12_real = M_pre[0][1] * pl1Real + M_pre[1][1] * pr1Real + M_pre[2][1] * pc1Real;
  h12_imag = M_pre[0][1] * pl1Imag + M_pre[1][1] * pr1Imag + M_pre[2][1] * pc1Imag;
  h21_real = M_pre[0][0] * pl2Real + M_pre[1][0] * pr2Real + M_pre[2][0] * pc2Real;
  h21_imag = M_pre[0][0] * pl2Imag + M_pre[1][0] * pr2Imag + M_pre[2][0] * pc2Imag;
  h22_real = M_pre[0][1] * pl2Real + M_pre[1][1] * pr2Real + M_pre[2][1] * pc2Real;
  h22_imag = M_pre[0][1] * pl2Imag + M_pre[1][1] * pr2Imag + M_pre[2][1] * pc2Imag;


  
  inv_norm_real = h11_real*h22_real - h11_imag*h22_imag - h12_real*h21_real + h12_imag*h21_imag;
  inv_norm_imag = h11_real*h22_imag + h11_imag*h22_real - h21_real*h12_imag - h21_imag*h12_real;

  inv_norm_abs  = inv_norm_real*inv_norm_real + inv_norm_imag*inv_norm_imag;

#ifdef UNSINGULARIZE_FIX
  
  Unsingularize3DMatrix(&h11_real, &h12_real, &h21_real, &h22_real, 
                        &h11_imag, &h12_imag, &h21_imag, &h22_imag,
                        &inv_norm_real, &inv_norm_imag, &inv_norm_abs);
#endif 

  if (inv_norm_abs != 0.f) {
    inv_norm_real /=  inv_norm_abs;
    inv_norm_imag /= -inv_norm_abs;
  }

  Q_real[ 0 ][ 0 ] =  h22_real*inv_norm_real - h22_imag*inv_norm_imag;
  Q_imag[ 0 ][ 0 ] =  h22_real*inv_norm_imag + h22_imag*inv_norm_real;
  Q_real[ 0 ][ 1 ] = -h12_real*inv_norm_real + h12_imag*inv_norm_imag;
  Q_imag[ 0 ][ 1 ] = -h12_real*inv_norm_imag - h12_imag*inv_norm_real;
  Q_real[ 1 ][ 0 ] = -h21_real*inv_norm_real + h21_imag*inv_norm_imag;
  Q_imag[ 1 ][ 0 ] = -h21_real*inv_norm_imag - h21_imag*inv_norm_real;
  Q_real[ 1 ][ 1 ] =  h11_real*inv_norm_real - h11_imag*inv_norm_imag;
  Q_imag[ 1 ][ 1 ] =  h11_real*inv_norm_imag + h11_imag*inv_norm_real;
}




static void SpatialDecCalculateM1andM2_5221(spatialDec* self) {

  int ps, pb, i, row, col, ch;
  float mReal[MAX_PARAMETER_BANDS][3][5];
  float mImag[MAX_PARAMETER_BANDS][3][5];

  float pc1Real[MAX_PARAMETER_BANDS];
  float pc1Imag[MAX_PARAMETER_BANDS];
  float pc2Real[MAX_PARAMETER_BANDS];
  float pc2Imag[MAX_PARAMETER_BANDS];
  float pl1    [MAX_PARAMETER_BANDS];
  float pl2Real[MAX_PARAMETER_BANDS];
  float pl2Imag[MAX_PARAMETER_BANDS];
  float pr1Real[MAX_PARAMETER_BANDS];
  float pr1Imag[MAX_PARAMETER_BANDS];
  float pr2    [MAX_PARAMETER_BANDS];

  HRTF_DIRECTIONAL_DATA const * hrtf = self->hrtfData->directional;

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  memset(mReal,0,sizeof(mReal));
  memset(mImag,0,sizeof(mImag));

  for (ps = 0; ps < self->numParameterSets; ps++) {
    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {
        float M_pre[3][5];
        float M_ttt[3][3];
        int mtxInversion = self->mtxInversion;

        memset(M_pre,0,sizeof(M_pre));

        if (self->tttConfig[i][0].mode >= 2)
          {
            mtxInversion = mtxInversion && (self->tttConfig[i][0].mode == 2 || self->tttConfig[i][0].mode == 4);
          }

          
        CalculateTTT(self, ps, pb, self->tttConfig[i][0].mode, M_ttt);

        for (row = 0; row < 3; row++) {
          for (col = 0; col < 3; col++) {
            M_pre[row][col] = M_ttt[row][col];
          }
        }

          
        if (self->arbitraryDownmix != 0) {
          float gReal[2];

          CalculateArbDmxMtx(self, ps, pb, gReal);

            
          for (ch = 0; ch < self->numInputChannels; ch++) {
            for (row = 0; row < 3; row++) {
              M_pre[row][ch] *= gReal[ch];

                
              if (self->arbitraryDownmix == 2 && pb < self->arbdmxResidualBands) {
                M_pre[row][3 + ch] = M_ttt[row][ch];
              }
            }
          }
        }



        if (mtxInversion) {
          float hReal[2][2];
          float hImag[2][2];

            
          CalculateMTXinv(self, ps, pb, self->tttConfig[i][0].mode, hReal, hImag);

            
          mReal[pb][0][0] = M_pre[0][0] * hReal[ 0 ][ 0 ] + M_pre[0][1] * hReal[ 1 ][ 0 ];
          mReal[pb][0][1] = M_pre[0][0] * hReal[ 0 ][ 1 ] + M_pre[0][1] * hReal[ 1 ][ 1 ];
          mReal[pb][1][0] = M_pre[1][0] * hReal[ 0 ][ 0 ] + M_pre[1][1] * hReal[ 1 ][ 0 ];
          mReal[pb][1][1] = M_pre[1][0] * hReal[ 0 ][ 1 ] + M_pre[1][1] * hReal[ 1 ][ 1 ];
          mReal[pb][2][0] = M_pre[2][0] * hReal[ 0 ][ 0 ] + M_pre[2][1] * hReal[ 1 ][ 0 ];
          mReal[pb][2][1] = M_pre[2][0] * hReal[ 0 ][ 1 ] + M_pre[2][1] * hReal[ 1 ][ 1 ];

          mImag[pb][0][0] = M_pre[0][0] * hImag[ 0 ][ 0 ] + M_pre[0][1] * hImag[ 1 ][ 0 ];
          mImag[pb][0][1] = M_pre[0][0] * hImag[ 0 ][ 1 ] + M_pre[0][1] * hImag[ 1 ][ 1 ];
          mImag[pb][1][0] = M_pre[1][0] * hImag[ 0 ][ 0 ] + M_pre[1][1] * hImag[ 1 ][ 0 ];
          mImag[pb][1][1] = M_pre[1][0] * hImag[ 0 ][ 1 ] + M_pre[1][1] * hImag[ 1 ][ 1 ];
          mImag[pb][2][0] = M_pre[2][0] * hImag[ 0 ][ 0 ] + M_pre[2][1] * hImag[ 1 ][ 0 ];
          mImag[pb][2][1] = M_pre[2][0] * hImag[ 0 ][ 1 ] + M_pre[2][1] * hImag[ 1 ][ 1 ];
        }
        else if (self->_3DStereoInversion) {
          float qReal[2][2];
          float qImag[2][2];

            
          Calculate3Dinv(self, ps, pb, M_pre, qReal, qImag);

            
          mReal[pb][0][0] = M_pre[0][0] * qReal[ 0 ][ 0 ] + M_pre[0][1] * qReal[ 1 ][ 0 ];
          mReal[pb][0][1] = M_pre[0][0] * qReal[ 0 ][ 1 ] + M_pre[0][1] * qReal[ 1 ][ 1 ];
          mReal[pb][1][0] = M_pre[1][0] * qReal[ 0 ][ 0 ] + M_pre[1][1] * qReal[ 1 ][ 0 ];
          mReal[pb][1][1] = M_pre[1][0] * qReal[ 0 ][ 1 ] + M_pre[1][1] * qReal[ 1 ][ 1 ];
          mReal[pb][2][0] = M_pre[2][0] * qReal[ 0 ][ 0 ] + M_pre[2][1] * qReal[ 1 ][ 0 ];
          mReal[pb][2][1] = M_pre[2][0] * qReal[ 0 ][ 1 ] + M_pre[2][1] * qReal[ 1 ][ 1 ];

          mImag[pb][0][0] = M_pre[0][0] * qImag[ 0 ][ 0 ] + M_pre[0][1] * qImag[ 1 ][ 0 ];
          mImag[pb][0][1] = M_pre[0][0] * qImag[ 0 ][ 1 ] + M_pre[0][1] * qImag[ 1 ][ 1 ];
          mImag[pb][1][0] = M_pre[1][0] * qImag[ 0 ][ 0 ] + M_pre[1][1] * qImag[ 1 ][ 0 ];
          mImag[pb][1][1] = M_pre[1][0] * qImag[ 0 ][ 1 ] + M_pre[1][1] * qImag[ 1 ][ 1 ];
          mImag[pb][2][0] = M_pre[2][0] * qImag[ 0 ][ 0 ] + M_pre[2][1] * qImag[ 1 ][ 0 ];
          mImag[pb][2][1] = M_pre[2][0] * qImag[ 0 ][ 1 ] + M_pre[2][1] * qImag[ 1 ][ 1 ];
        }
        else
          {
            mReal[pb][0][0] = M_pre[0][0];
            mReal[pb][0][1] = M_pre[0][1];
            mReal[pb][1][0] = M_pre[1][0];
            mReal[pb][1][1] = M_pre[1][1];
            mReal[pb][2][0] = M_pre[2][0];
            mReal[pb][2][1] = M_pre[2][1];

            mImag[pb][0][0] = 0;
            mImag[pb][0][1] = 0;
            mImag[pb][1][0] = 0;
            mImag[pb][1][1] = 0;
            mImag[pb][2][0] = 0;
            mImag[pb][2][1] = 0;
          }

          
        mReal[pb][0][2] = M_pre[0][3];
        mReal[pb][0][3] = M_pre[0][4];
        mReal[pb][0][4] = M_pre[0][2];
        mReal[pb][1][2] = M_pre[1][3];
        mReal[pb][1][3] = M_pre[1][4];
        mReal[pb][1][4] = M_pre[1][2];
        mReal[pb][2][2] = M_pre[2][3];
        mReal[pb][2][3] = M_pre[2][4];
        mReal[pb][2][4] = M_pre[2][2];

      }
    }

#ifdef HRTF_DYNAMIC_UPDATE
    if ((self->hrtfSource != NULL) && ((ps < (self->numParameterSets - 1)) || (!self->extendFrame))) {
      UpdateHRTF(self);
    }
#endif 

      
    for (pb = 0; pb < self->numParameterBands; pb++) {
      float temp1;
      float temp2;

      temp1 = (float) hrtf[2].powerL[pb];
      temp2 = (float) hrtf[2].powerR[pb];

      if (pb < 12) {
        pc1Real[pb] = (float) cos(hrtf[2].ipd[pb] / 2) * temp1;
        pc1Imag[pb] = (float) sin(hrtf[2].ipd[pb] / 2) * temp1;
        pc2Real[pb] = (float) cos(-hrtf[2].ipd[pb] / 2) * temp2;
        pc2Imag[pb] = (float) sin(-hrtf[2].ipd[pb] / 2) * temp2;
      }
      else {
        pc1Real[pb] = temp1;
        pc1Imag[pb] = 0;
        pc2Real[pb] = temp2;
        pc2Imag[pb] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iid = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
      float lf = iid / (1 + iid);
      float ls = 1   / (1 + iid);
      float lfL = lf / (lf + ls) * hrtf[0].powerL[pb] * hrtf[0].powerL[pb];
      float lfR = lf / (lf + ls) * hrtf[0].powerR[pb] * hrtf[0].powerR[pb];
      float lsL = ls / (lf + ls) * hrtf[3].powerL[pb] * hrtf[3].powerL[pb] * self->surroundGain * self->surroundGain;
      float lsR = ls / (lf + ls) * hrtf[3].powerR[pb] * hrtf[3].powerR[pb] * self->surroundGain * self->surroundGain;
      float temp;

      pl1[pb] = (float) sqrt(lfL + lsL);
      temp    = (float) sqrt(lfR + lsR);

      if (pb < 12) {
        pl2Real[pb] = (float) cos(-(lfR * hrtf[0].ipd[pb] + lsR * hrtf[3].ipd[pb]) / (lfR + lsR)) * temp;
        pl2Imag[pb] = (float) sin(-(lfR * hrtf[0].ipd[pb] + lsR * hrtf[3].ipd[pb]) / (lfR + lsR)) * temp;
      }
      else {
        pl2Real[pb] = temp;
        pl2Imag[pb] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float iid = (float) pow(10.0f, self->ottCLD[2][ps][pb] / 10.0f);
      float rf = iid / (1 + iid);
      float rs = 1   / (1 + iid);
      float rfL = rf / (rf + rs) * hrtf[1].powerL[pb] * hrtf[1].powerL[pb];
      float rfR = rf / (rf + rs) * hrtf[1].powerR[pb] * hrtf[1].powerR[pb];
      float rsL = rs / (rf + rs) * hrtf[4].powerL[pb] * hrtf[4].powerL[pb] * self->surroundGain * self->surroundGain;
      float rsR = rs / (rf + rs) * hrtf[4].powerR[pb] * hrtf[4].powerR[pb] * self->surroundGain * self->surroundGain;
      float temp;

      temp    = (float) sqrt(rfL + rsL);
      pr2[pb] = (float) sqrt(rfR + rsR);

      if (pb < 12) {
        pr1Real[pb] = (float) cos((rfL * hrtf[1].ipd[pb] + rsL * hrtf[4].ipd[pb]) / (rfL + rsL)) * temp;
        pr1Imag[pb] = (float) sin((rfL * hrtf[1].ipd[pb] + rsL * hrtf[4].ipd[pb]) / (rfL + rsL)) * temp;
      }
      else {
        pr1Real[pb] = temp;
        pr1Imag[pb] = 0;
      }
    }

      
    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][0][1] = 0;
      self->M1paramReal[ps][pb][1][0] = 0;
      self->M1paramReal[ps][pb][1][1] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;

        
      self->M1paramReal[ps][pb][2][3] = 1;
      self->M1paramReal[ps][pb][2][4] = 0;
      self->M1paramReal[ps][pb][3][3] = 0;
      self->M1paramReal[ps][pb][3][4] = 1;

      self->M1paramImag[ps][pb][2][3] = 0;
      self->M1paramImag[ps][pb][2][4] = 0;
      self->M1paramImag[ps][pb][3][3] = 0;
      self->M1paramImag[ps][pb][3][4] = 0;

    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorImag[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][1] = 0;
      self->M2decorImag[ps][pb][0][1] = 0;

      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorImag[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][1] = 0;
      self->M2decorImag[ps][pb][1][1] = 0;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0]  = mReal[pb][0][0] * pl1[pb];
      self->M2residImag[ps][pb][0][0]  = mImag[pb][0][0] * pl1[pb];
      self->M2residReal[ps][pb][0][0] += mReal[pb][1][0] * pr1Real[pb] - mImag[pb][1][0] * pr1Imag[pb];
      self->M2residImag[ps][pb][0][0] += mImag[pb][1][0] * pr1Real[pb] + mReal[pb][1][0] * pr1Imag[pb];
      self->M2residReal[ps][pb][0][0] += mReal[pb][2][0] * pc1Real[pb] - mImag[pb][2][0] * pc1Imag[pb];
      self->M2residImag[ps][pb][0][0] += mImag[pb][2][0] * pc1Real[pb] + mReal[pb][2][0] * pc1Imag[pb];

      self->M2residReal[ps][pb][0][1]  = mReal[pb][0][1] * pl1[pb];
      self->M2residImag[ps][pb][0][1]  = mImag[pb][0][1] * pl1[pb];
      self->M2residReal[ps][pb][0][1] += mReal[pb][1][1] * pr1Real[pb] - mImag[pb][1][1] * pr1Imag[pb];
      self->M2residImag[ps][pb][0][1] += mImag[pb][1][1] * pr1Real[pb] + mReal[pb][1][1] * pr1Imag[pb];
      self->M2residReal[ps][pb][0][1] += mReal[pb][2][1] * pc1Real[pb] - mImag[pb][2][1] * pc1Imag[pb];
      self->M2residImag[ps][pb][0][1] += mImag[pb][2][1] * pc1Real[pb] + mReal[pb][2][1] * pc1Imag[pb];

      self->M2residReal[ps][pb][1][0]  = mReal[pb][0][0] * pl2Real[pb] - mImag[pb][0][0] * pl2Imag[pb];
      self->M2residImag[ps][pb][1][0]  = mImag[pb][0][0] * pl2Real[pb] + mReal[pb][0][0] * pl2Imag[pb];
      self->M2residReal[ps][pb][1][0] += mReal[pb][1][0] * pr2[pb];
      self->M2residImag[ps][pb][1][0] += mImag[pb][1][0] * pr2[pb];
      self->M2residReal[ps][pb][1][0] += mReal[pb][2][0] * pc2Real[pb] - mImag[pb][2][0] * pc2Imag[pb];
      self->M2residImag[ps][pb][1][0] += mImag[pb][2][0] * pc2Real[pb] + mReal[pb][2][0] * pc2Imag[pb];

      self->M2residReal[ps][pb][1][1]  = mReal[pb][0][1] * pl2Real[pb] - mImag[pb][0][1] * pl2Imag[pb];
      self->M2residImag[ps][pb][1][1]  = mImag[pb][0][1] * pl2Real[pb] + mReal[pb][0][1] * pl2Imag[pb];
      self->M2residReal[ps][pb][1][1] += mReal[pb][1][1] * pr2[pb];
      self->M2residImag[ps][pb][1][1] += mImag[pb][1][1] * pr2[pb];
      self->M2residReal[ps][pb][1][1] += mReal[pb][2][1] * pc2Real[pb] - mImag[pb][2][1] * pc2Imag[pb];
      self->M2residImag[ps][pb][1][1] += mImag[pb][2][1] * pc2Real[pb] + mReal[pb][2][1] * pc2Imag[pb];

        
      if (self->arbitraryDownmix == 2)
        {
          self->M2residReal[ps][pb][0][2]  = mReal[pb][0][2] * pl1[pb];
          self->M2residImag[ps][pb][0][2]  = mImag[pb][0][2] * pl1[pb];
          self->M2residReal[ps][pb][0][2] += mReal[pb][1][2] * pr1Real[pb] - mImag[pb][1][2] * pr1Imag[pb];
          self->M2residImag[ps][pb][0][2] += mImag[pb][1][2] * pr1Real[pb] + mReal[pb][1][2] * pr1Imag[pb];
          self->M2residReal[ps][pb][0][2] += mReal[pb][2][2] * pc1Real[pb] - mImag[pb][2][2] * pc1Imag[pb];
          self->M2residImag[ps][pb][0][2] += mImag[pb][2][2] * pc1Real[pb] + mReal[pb][2][2] * pc1Imag[pb];

          self->M2residReal[ps][pb][0][3]  = mReal[pb][0][3] * pl1[pb];
          self->M2residImag[ps][pb][0][3]  = mImag[pb][0][3] * pl1[pb];
          self->M2residReal[ps][pb][0][3] += mReal[pb][1][3] * pr1Real[pb] - mImag[pb][1][3] * pr1Imag[pb];
          self->M2residImag[ps][pb][0][3] += mImag[pb][1][3] * pr1Real[pb] + mReal[pb][1][3] * pr1Imag[pb];
          self->M2residReal[ps][pb][0][3] += mReal[pb][2][3] * pc1Real[pb] - mImag[pb][2][3] * pc1Imag[pb];
          self->M2residImag[ps][pb][0][3] += mImag[pb][2][3] * pc1Real[pb] + mReal[pb][2][3] * pc1Imag[pb];

          self->M2residReal[ps][pb][1][2]  = mReal[pb][0][2] * pl2Real[pb] - mImag[pb][0][2] * pl2Imag[pb];
          self->M2residImag[ps][pb][1][2]  = mImag[pb][0][2] * pl2Real[pb] + mReal[pb][0][2] * pl2Imag[pb];
          self->M2residReal[ps][pb][1][2] += mReal[pb][1][2] * pr2[pb];
          self->M2residImag[ps][pb][1][2] += mImag[pb][1][2] * pr2[pb];
          self->M2residReal[ps][pb][1][2] += mReal[pb][2][2] * pc2Real[pb] - mImag[pb][2][2] * pc2Imag[pb];
          self->M2residImag[ps][pb][1][2] += mImag[pb][2][2] * pc2Real[pb] + mReal[pb][2][2] * pc2Imag[pb];

          self->M2residReal[ps][pb][1][3]  = mReal[pb][0][3] * pl2Real[pb] - mImag[pb][0][3] * pl2Imag[pb];
          self->M2residImag[ps][pb][1][3]  = mImag[pb][0][3] * pl2Real[pb] + mReal[pb][0][3] * pl2Imag[pb];
          self->M2residReal[ps][pb][1][3] += mReal[pb][1][3] * pr2[pb];
          self->M2residImag[ps][pb][1][3] += mImag[pb][1][3] * pr2[pb];
          self->M2residReal[ps][pb][1][3] += mReal[pb][2][3] * pc2Real[pb] - mImag[pb][2][3] * pc2Imag[pb];
          self->M2residImag[ps][pb][1][3] += mImag[pb][2][3] * pc2Real[pb] + mReal[pb][2][3] * pc2Imag[pb];
        }

    }
  }
}



static void SpatialDecCalculateM1andM2_5227(spatialDec* self) {

  int ps, pb, i;
  int row, col, ch;

  float mReal[MAX_PARAMETER_BANDS][3][5];
  float mImag[MAX_PARAMETER_BANDS][3][5];
  float lf[MAX_PARAMETER_BANDS];
  float ls[MAX_PARAMETER_BANDS];
  float rf[MAX_PARAMETER_BANDS];
  float rs[MAX_PARAMETER_BANDS];
  float aC1[MAX_PARAMETER_BANDS];
  float aC2[MAX_PARAMETER_BANDS];
  float aIcc_c[MAX_PARAMETER_BANDS];
  int   aPredictionMode[MAX_PARAMETER_BANDS];

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  memset(mReal,0,sizeof(mReal));
  memset(mImag,0,sizeof(mImag));

  for (ps = 0; ps < self->numParameterSets; ps++) {
    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {
        float M_pre[3][5];
        float M_ttt[3][3];
        int mtxInversion = self->mtxInversion;

        memset(M_pre,0,sizeof(M_pre));

        if (self->tttConfig[i][0].mode >= 2)
          {
            mtxInversion = mtxInversion && (self->tttConfig[i][0].mode == 2 || self->tttConfig[i][0].mode == 4);
          }

          
        CalculateTTT(self, ps, pb, self->tttConfig[i][0].mode, M_ttt);

        for (row = 0; row < 3; row++) {
          for (col = 0; col < 3; col++) {
            M_pre[row][col] = M_ttt[row][col];
          }
        }

          
        if (self->arbitraryDownmix != 0) {
          float gReal[2];

          CalculateArbDmxMtx(self, ps, pb, gReal);

            
          for (ch = 0; ch < self->numInputChannels; ch++) {
            for (row = 0; row < 3; row++) {
              M_pre[row][ch] *= gReal[ch];

                
              if (self->arbitraryDownmix == 2 && pb < self->arbdmxResidualBands) {
                M_pre[row][3 + ch] = M_ttt[row][ch];
              }
            }
          }
        }

        if (mtxInversion) {
          float hReal[2][2];
          float hImag[2][2];

            
          CalculateMTXinv(self, ps, pb, self->tttConfig[i][0].mode, hReal, hImag);

            
          mReal[pb][0][0] = M_pre[0][0] * hReal[ 0 ][ 0 ] + M_pre[0][1] * hReal[ 1 ][ 0 ];
          mReal[pb][0][1] = M_pre[0][0] * hReal[ 0 ][ 1 ] + M_pre[0][1] * hReal[ 1 ][ 1 ];
          mReal[pb][1][0] = M_pre[1][0] * hReal[ 0 ][ 0 ] + M_pre[1][1] * hReal[ 1 ][ 0 ];
          mReal[pb][1][1] = M_pre[1][0] * hReal[ 0 ][ 1 ] + M_pre[1][1] * hReal[ 1 ][ 1 ];
          mReal[pb][2][0] = M_pre[2][0] * hReal[ 0 ][ 0 ] + M_pre[2][1] * hReal[ 1 ][ 0 ];
          mReal[pb][2][1] = M_pre[2][0] * hReal[ 0 ][ 1 ] + M_pre[2][1] * hReal[ 1 ][ 1 ];

          mImag[pb][0][0] = M_pre[0][0] * hImag[ 0 ][ 0 ] + M_pre[0][1] * hImag[ 1 ][ 0 ];
          mImag[pb][0][1] = M_pre[0][0] * hImag[ 0 ][ 1 ] + M_pre[0][1] * hImag[ 1 ][ 1 ];
          mImag[pb][1][0] = M_pre[1][0] * hImag[ 0 ][ 0 ] + M_pre[1][1] * hImag[ 1 ][ 0 ];
          mImag[pb][1][1] = M_pre[1][0] * hImag[ 0 ][ 1 ] + M_pre[1][1] * hImag[ 1 ][ 1 ];
          mImag[pb][2][0] = M_pre[2][0] * hImag[ 0 ][ 0 ] + M_pre[2][1] * hImag[ 1 ][ 0 ];
          mImag[pb][2][1] = M_pre[2][0] * hImag[ 0 ][ 1 ] + M_pre[2][1] * hImag[ 1 ][ 1 ];
        }
        else if (self->_3DStereoInversion) {
          float qReal[2][2];
          float qImag[2][2];

            
          Calculate3Dinv(self, ps, pb, M_pre, qReal, qImag);

            
          mReal[pb][0][0] = M_pre[0][0] * qReal[ 0 ][ 0 ] + M_pre[0][1] * qReal[ 1 ][ 0 ];
          mReal[pb][0][1] = M_pre[0][0] * qReal[ 0 ][ 1 ] + M_pre[0][1] * qReal[ 1 ][ 1 ];
          mReal[pb][1][0] = M_pre[1][0] * qReal[ 0 ][ 0 ] + M_pre[1][1] * qReal[ 1 ][ 0 ];
          mReal[pb][1][1] = M_pre[1][0] * qReal[ 0 ][ 1 ] + M_pre[1][1] * qReal[ 1 ][ 1 ];
          mReal[pb][2][0] = M_pre[2][0] * qReal[ 0 ][ 0 ] + M_pre[2][1] * qReal[ 1 ][ 0 ];
          mReal[pb][2][1] = M_pre[2][0] * qReal[ 0 ][ 1 ] + M_pre[2][1] * qReal[ 1 ][ 1 ];

          mImag[pb][0][0] = M_pre[0][0] * qImag[ 0 ][ 0 ] + M_pre[0][1] * qImag[ 1 ][ 0 ];
          mImag[pb][0][1] = M_pre[0][0] * qImag[ 0 ][ 1 ] + M_pre[0][1] * qImag[ 1 ][ 1 ];
          mImag[pb][1][0] = M_pre[1][0] * qImag[ 0 ][ 0 ] + M_pre[1][1] * qImag[ 1 ][ 0 ];
          mImag[pb][1][1] = M_pre[1][0] * qImag[ 0 ][ 1 ] + M_pre[1][1] * qImag[ 1 ][ 1 ];
          mImag[pb][2][0] = M_pre[2][0] * qImag[ 0 ][ 0 ] + M_pre[2][1] * qImag[ 1 ][ 0 ];
          mImag[pb][2][1] = M_pre[2][0] * qImag[ 0 ][ 1 ] + M_pre[2][1] * qImag[ 1 ][ 1 ];
        }
        else
          {
            mReal[pb][0][0] = M_pre[0][0];
            mReal[pb][0][1] = M_pre[0][1];
            mReal[pb][1][0] = M_pre[1][0];
            mReal[pb][1][1] = M_pre[1][1];
            mReal[pb][2][0] = M_pre[2][0];
            mReal[pb][2][1] = M_pre[2][1];

            mImag[pb][0][0] = 0;
            mImag[pb][0][1] = 0;
            mImag[pb][1][0] = 0;
            mImag[pb][1][1] = 0;
            mImag[pb][2][0] = 0;
            mImag[pb][2][1] = 0;
          }

          
        mReal[pb][0][2] = M_pre[0][3];
        mReal[pb][0][3] = M_pre[0][4];
        mReal[pb][0][4] = M_pre[0][2];
        mReal[pb][1][2] = M_pre[1][3];
        mReal[pb][1][3] = M_pre[1][4];
        mReal[pb][1][4] = M_pre[1][2];
        mReal[pb][2][2] = M_pre[2][3];
        mReal[pb][2][3] = M_pre[2][4];
        mReal[pb][2][4] = M_pre[2][2];
      }
    }


    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {
        float iid;
        if (self->tttConfig[i][0].mode < 2) {
          aPredictionMode[pb] = 1;
          aC1[pb] = self->tttCPC1[0][ps][pb];
          aC2[pb] = self->tttCPC2[0][ps][pb];
          aIcc_c[pb] = self->tttICC[0][ps][pb];
        }
        else{
          aPredictionMode[pb] = 0;
          aC1[pb] = self->tttCLD1[0][ps][pb];
          aC2[pb] = self->tttCLD2[0][ps][pb];
          aIcc_c[pb] = 0;
        }
        iid = (float) pow(10.0f, self->ottCLD[1][ps][pb] / 10.0f);
        lf[pb] = iid / (1 + iid);
        ls[pb] = 1   / (1 + iid);

        iid = (float) pow(10.0f, self->ottCLD[2][ps][pb] / 10.0f);
        rf[pb] = iid / (1 + iid);
        rs[pb] = 1   / (1 + iid);
      }
    }

    SpatialDecUpdateHrtfFilterbank(self->hrtfFilter, mReal, mImag, lf, ls, rf, rs, aC1, aC2, aIcc_c, aPredictionMode, self->paramSlot[ps]);


    for (pb = 0; pb < self->numParameterBands; pb++) {
        
      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][0][1] = 0;
      self->M1paramReal[ps][pb][1][0] = 0;
      self->M1paramReal[ps][pb][1][1] = 1;

      self->M1paramImag[ps][pb][0][0] = 0;
      self->M1paramImag[ps][pb][0][1] = 0;
      self->M1paramImag[ps][pb][1][0] = 0;
      self->M1paramImag[ps][pb][1][1] = 0;

        
      self->M1paramReal[ps][pb][2][3] = 1;
      self->M1paramReal[ps][pb][2][4] = 0;
      self->M1paramReal[ps][pb][3][3] = 0;
      self->M1paramReal[ps][pb][3][4] = 1;

      self->M1paramImag[ps][pb][2][3] = 0;
      self->M1paramImag[ps][pb][2][4] = 0;
      self->M1paramImag[ps][pb][3][3] = 0;
      self->M1paramImag[ps][pb][3][4] = 0;

    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][0] = 0;
      self->M2decorReal[ps][pb][0][1] = 0;
      self->M2decorReal[ps][pb][1][0] = 0;
      self->M2decorReal[ps][pb][1][1] = 0;

      self->M2decorImag[ps][pb][0][0] = 0;
      self->M2decorImag[ps][pb][0][1] = 0;
      self->M2decorImag[ps][pb][1][0] = 0;
      self->M2decorImag[ps][pb][1][1] = 0;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = 0;
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residReal[ps][pb][1][0] = 0;
      self->M2residReal[ps][pb][1][1] = 0;

      self->M2residImag[ps][pb][0][0] = 0;
      self->M2residImag[ps][pb][0][1] = 0;
      self->M2residImag[ps][pb][1][0] = 0;
      self->M2residImag[ps][pb][1][1] = 0;
    }
  }
}




static void SpatialDecCalculateM1andM2_5251(spatialDec* self) {

  int ch, ps, pb,col, row, i;

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_f_L     [MAX_PARAMETER_BANDS];       float c_f_R     [MAX_PARAMETER_BANDS];
  float dummy1    [MAX_PARAMETER_BANDS];       float dummy2    [MAX_PARAMETER_BANDS];

  float c_l_CLFE  [MAX_PARAMETER_BANDS];
  float c_r_CLFE  [MAX_PARAMETER_BANDS];

  float kappa[MAX_PARAMETER_BANDS];
  float G_dd [MAX_PARAMETER_BANDS];

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  for (ps=0; ps < self->numParameterSets; ps++) {

      
    for (i=0; i<2; i++){  
      for(pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++){
        float M_pre[3][5];
        float M_ttt[3][3];
        int mtxInversion = self->mtxInversion;

        memset(M_pre,0,sizeof(M_pre));

        if (self->tttConfig[i][0].mode >= 2)
          {
            mtxInversion = mtxInversion && (self->tttConfig[i][0].mode == 2 || self->tttConfig[i][0].mode == 4);
          }

          
        CalculateTTT(self, ps, pb, self->tttConfig[i][0].mode, M_ttt);

        for (row = 0; row < 3; row++) {
          for (col = 0; col < 3; col++) {
            M_pre[row][col] = M_ttt[row][col];
          }
        }

          
        if (self->arbitraryDownmix != 0) {
          float gReal[2];

          CalculateArbDmxMtx(self, ps, pb, gReal);

            
          for (ch = 0; ch < self->numInputChannels; ch++) {
            for (row = 0; row < 3; row++) {
              M_pre[row][ch] *= gReal[ch];

                
              if (self->arbitraryDownmix == 2 && pb < self->arbdmxResidualBands) {
                M_pre[row][3 + ch] = M_ttt[row][ch];
              }
            }
          }
        }

        if (mtxInversion) {
          float hReal[2][2];
          float hImag[2][2];

            
          CalculateMTXinv(self, ps, pb, self->tttConfig[i][0].mode, hReal, hImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * hReal[ 0 ][ 0 ] + M_pre[0][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * hReal[ 0 ][ 1 ] + M_pre[0][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * hReal[ 0 ][ 0 ] + M_pre[1][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * hReal[ 0 ][ 1 ] + M_pre[1][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * hReal[ 0 ][ 0 ] + M_pre[2][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * hReal[ 0 ][ 1 ] + M_pre[2][1] * hReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * hImag[ 0 ][ 0 ] + M_pre[0][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * hImag[ 0 ][ 1 ] + M_pre[0][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * hImag[ 0 ][ 0 ] + M_pre[1][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * hImag[ 0 ][ 1 ] + M_pre[1][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * hImag[ 0 ][ 0 ] + M_pre[2][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * hImag[ 0 ][ 1 ] + M_pre[2][1] * hImag[ 1 ][ 1 ];
        }
        else if (self->_3DStereoInversion) {
          float qReal[2][2];
          float qImag[2][2];

            
          Calculate3Dinv(self, ps, pb, M_pre, qReal, qImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * qReal[ 0 ][ 0 ] + M_pre[0][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * qReal[ 0 ][ 1 ] + M_pre[0][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * qReal[ 0 ][ 0 ] + M_pre[1][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * qReal[ 0 ][ 1 ] + M_pre[1][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * qReal[ 0 ][ 0 ] + M_pre[2][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * qReal[ 0 ][ 1 ] + M_pre[2][1] * qReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * qImag[ 0 ][ 0 ] + M_pre[0][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * qImag[ 0 ][ 1 ] + M_pre[0][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * qImag[ 0 ][ 0 ] + M_pre[1][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * qImag[ 0 ][ 1 ] + M_pre[1][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * qImag[ 0 ][ 0 ] + M_pre[2][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * qImag[ 0 ][ 1 ] + M_pre[2][1] * qImag[ 1 ][ 1 ];
        }
        else
          {
            self->M1paramReal[ps][pb][0][0] = M_pre[0][0];
            self->M1paramReal[ps][pb][0][1] = M_pre[0][1];
            self->M1paramReal[ps][pb][1][0] = M_pre[1][0];
            self->M1paramReal[ps][pb][1][1] = M_pre[1][1];
            self->M1paramReal[ps][pb][2][0] = M_pre[2][0];
            self->M1paramReal[ps][pb][2][1] = M_pre[2][1];

            self->M1paramImag[ps][pb][0][0] = 0;
            self->M1paramImag[ps][pb][0][1] = 0;
            self->M1paramImag[ps][pb][1][0] = 0;
            self->M1paramImag[ps][pb][1][1] = 0;
            self->M1paramImag[ps][pb][2][0] = 0;
            self->M1paramImag[ps][pb][2][1] = 0;
          }

          
        self->M1paramReal[ps][pb][0][2] = M_pre[0][2];
        self->M1paramReal[ps][pb][0][3] = M_pre[0][3];
        self->M1paramReal[ps][pb][0][4] = M_pre[0][4];
        self->M1paramReal[ps][pb][1][2] = M_pre[1][2];
        self->M1paramReal[ps][pb][1][3] = M_pre[1][3];
        self->M1paramReal[ps][pb][1][4] = M_pre[1][4];
        self->M1paramReal[ps][pb][2][2] = M_pre[2][2];
        self->M1paramReal[ps][pb][2][3] = M_pre[2][3];
        self->M1paramReal[ps][pb][2][4] = M_pre[2][4];


        
        for(col = 0; col < self->numXChannels; col++){
          self->M1paramReal[ps][pb][3][col] = self->M1paramReal[ps][pb][0][col];
          self->M1paramImag[ps][pb][3][col] = self->M1paramImag[ps][pb][0][col];
          self->M1paramReal[ps][pb][4][col] = self->M1paramReal[ps][pb][1][col];
          self->M1paramImag[ps][pb][4][col] = self->M1paramImag[ps][pb][1][col];
          if(self->tttConfig[i][0].useTTTdecorr) {
            self->M1paramReal[ps][pb][5][col] = self->M1paramReal[ps][pb][2][col];
            self->M1paramImag[ps][pb][5][col] = self->M1paramImag[ps][pb][2][col];
          }
        }
      }
    }


      
    param2UMX_PS(self, H11_L,  H12_L,  H21_L,  H22_L,  H12_res_L,  H22_res_L,  c_f_L,  dummy1,  1, ps, self->resBands[1]);
    param2UMX_PS(self, H11_R,  H12_R,  H21_R,  H22_R,  H12_res_R,  H22_res_R,  c_f_R,  dummy2,  2, ps, self->resBands[2]);

    for(pb = 0; pb < self->numOttBands[0]; pb++){
      float iid_lin  = (float) (pow(10,( self->ottCLD[0][ps][pb]/20.0f)));
      c_l_CLFE[pb]   = (float) (sqrt( iid_lin*iid_lin / ( 1 + iid_lin*iid_lin)));
      c_r_CLFE[pb]   = (float) (sqrt(               1 / ( 1 + iid_lin*iid_lin)));
    }

    for(pb = self->numOttBands[0]; pb < self->numParameterBands; pb++){
      c_l_CLFE[pb] = 1;
      c_r_CLFE[pb] = 0;
    }


      
    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2decorReal[ps][pb][0][0] = H11_L[pb];
      self->M2decorReal[ps][pb][0][1] = 0;
      self->M2decorReal[ps][pb][0][2] = 0;
      self->M2decorReal[ps][pb][0][3] = H12_L[pb];
      self->M2decorReal[ps][pb][0][4] = 0;


      self->M2decorReal[ps][pb][1][0] = H21_L[pb];
      self->M2decorReal[ps][pb][1][1] = 0;
      self->M2decorReal[ps][pb][1][2] = 0;
      self->M2decorReal[ps][pb][1][3] = H22_L[pb];
      self->M2decorReal[ps][pb][1][4] = 0;


      self->M2decorReal[ps][pb][2][0] = 0;
      self->M2decorReal[ps][pb][2][1] = H11_R[pb];
      self->M2decorReal[ps][pb][2][2] = 0;
      self->M2decorReal[ps][pb][2][3] = 0;
      self->M2decorReal[ps][pb][2][4] = H12_R[pb];


      self->M2decorReal[ps][pb][3][0] = 0;
      self->M2decorReal[ps][pb][3][1] = H21_R[pb];
      self->M2decorReal[ps][pb][3][2] = 0;
      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = H22_R[pb];


      self->M2decorReal[ps][pb][4][0] = 0;
      self->M2decorReal[ps][pb][4][1] = 0;
      self->M2decorReal[ps][pb][4][2] = c_l_CLFE[pb];
      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = 0;


      self->M2decorReal[ps][pb][5][0] = 0;
      self->M2decorReal[ps][pb][5][1] = 0;
      self->M2decorReal[ps][pb][5][2] = c_r_CLFE[pb];
      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = 0;
    }

    for(pb = 0; pb < self->numParameterBands; pb++){

      self->M2residReal[ps][pb][0][0] = H11_L[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residReal[ps][pb][0][2] = 0;
      self->M2residReal[ps][pb][0][3] = H12_res_L[pb];
      self->M2residReal[ps][pb][0][4] = 0;


      self->M2residReal[ps][pb][1][0] = H21_L[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residReal[ps][pb][1][2] = 0;
      self->M2residReal[ps][pb][1][3] = H22_res_L[pb];
      self->M2residReal[ps][pb][1][4] = 0;



      self->M2residReal[ps][pb][2][0] = 0;
      self->M2residReal[ps][pb][2][1] = H11_R[pb];
      self->M2residReal[ps][pb][2][2] = 0;
      self->M2residReal[ps][pb][2][3] = 0;
      self->M2residReal[ps][pb][2][4] = H12_res_R[pb];


      self->M2residReal[ps][pb][3][0] = 0;
      self->M2residReal[ps][pb][3][1] = H21_R[pb];
      self->M2residReal[ps][pb][3][2] = 0;
      self->M2residReal[ps][pb][3][3] = 0;
      self->M2residReal[ps][pb][3][4] = H22_res_R[pb];


      self->M2residReal[ps][pb][4][0] = 0;
      self->M2residReal[ps][pb][4][1] = 0;
      self->M2residReal[ps][pb][4][2] = c_l_CLFE[pb];
      self->M2residReal[ps][pb][4][3] = 0;
      self->M2residReal[ps][pb][4][4] = 0;


      self->M2residReal[ps][pb][5][0] = 0;
      self->M2residReal[ps][pb][5][1] = 0;
      self->M2residReal[ps][pb][5][2] = c_r_CLFE[pb];
      self->M2residReal[ps][pb][5][3] = 0;
      self->M2residReal[ps][pb][5][4] = 0;
    }

    for (i=0; i<2; i++){
      for(pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++){
        if (self->tttConfig[i][0].useTTTdecorr){
          for(row = 0; row < 6; row++){
            self->M2residReal[ps][pb][row][5] = 0;
            self->M2decorReal[ps][pb][row][5] = 0;
          }
        }
      }
    }


    for(pb = 0; pb < self->numParameterBands; pb++){
      kappa[pb] = self->tttICC[0][ps][pb];
      G_dd[pb]  = (float) (sqrt(1-kappa[pb]*kappa[pb])/sqrt(3));
    }


    for (i=0; i<2; i++){
      for(pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++){
        if (self->tttConfig[i][0].mode < 2 && pb >= self->resBands[3]){
          if (self->tttConfig[i][0].useTTTdecorr){
            self->M2decorReal[ps][pb][0][0] *= kappa[pb];
            self->M2decorReal[ps][pb][2][1] *= kappa[pb];
            self->M2decorReal[ps][pb][4][2] *= kappa[pb];

            self->M2decorReal[ps][pb][0][5] = G_dd[pb] * c_f_L[pb];
            self->M2decorReal[ps][pb][2][5] = G_dd[pb] * c_f_R[pb];
            self->M2decorReal[ps][pb][4][5] = (float) (-sqrt(2)*G_dd[pb]*c_l_CLFE[pb]);


            self->M2residReal[ps][pb][0][0] *= kappa[pb];
            self->M2residReal[ps][pb][2][1] *= kappa[pb];
            self->M2residReal[ps][pb][4][2] *= kappa[pb];

            self->M2residReal[ps][pb][0][5] = G_dd[pb] * c_f_L[pb];
            self->M2residReal[ps][pb][2][5] = G_dd[pb] * c_f_R[pb];
            self->M2residReal[ps][pb][4][5] = (float) (-sqrt(2)*G_dd[pb]*c_l_CLFE[pb]);
          }
        }
      }
    }
  }
}





void SpatialDecCalculateM1andM2_EMM(spatialDec* self)
{
  float H11[MAX_PARAMETER_BANDS];
  float H12[MAX_PARAMETER_BANDS];
  float H21[MAX_PARAMETER_BANDS];
  float H22[MAX_PARAMETER_BANDS];
  float dummy1[MAX_PARAMETER_BANDS];
  float dummy2[MAX_PARAMETER_BANDS];
  float dummy3[MAX_PARAMETER_BANDS];
  float dummy4[MAX_PARAMETER_BANDS];
  int ps;
  int pb;
  int col;
  int row;

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  for (ps = 0; ps < self->numParameterSets; ps++) {
    param2UMX_PS(self, H11, H12, H21, H22, dummy1, dummy2, dummy3, dummy4, 0, ps, 0);

    for (pb = 0; pb < self->numParameterBands; pb++) {
      float m11 = self->tttCPC1[0][ps][pb] + 2;
      float m12 = self->tttCPC2[0][ps][pb] - 1;
      float m21 = self->tttCPC1[0][ps][pb] - 1;
      float m22 = self->tttCPC2[0][ps][pb] + 2;
      float m31 = 1 - self->tttCPC1[0][ps][pb];
      float m32 = 1 - self->tttCPC2[0][ps][pb];
      float weight1;
      float weight2;
      float hReal[2][2];
      float hImag[2][2];

      getMatrixInversionWeights(self->ottCLD[0][ps][pb],
                                self->ottCLD[0][ps][pb],
                                1,
                                self->tttCPC1[0][ps][pb],
                                self->tttCPC2[0][ps][pb],
                                &weight1,
                                &weight2);

#ifdef PARTIALLY_COMPLEX
      if (pb < complexBands) {
        invertMatrix(weight1, weight2, hReal, hImag);
      }
      else {
        invertMatrixReal(weight1, weight2, hReal);
        hImag[0][0] = 0.0f;
        hImag[0][1] = 0.0f;
        hImag[1][0] = 0.0f;
        hImag[1][1] = 0.0f;
      }
#else
      invertMatrix(weight1, weight2, hReal, hImag);
#endif

      self->M1paramReal[ps][pb][0][0] = m11 * hReal[ 0 ][ 0 ] + m12 * hReal[ 1 ][ 0 ];
      self->M1paramReal[ps][pb][0][1] = m11 * hReal[ 0 ][ 1 ] + m12 * hReal[ 1 ][ 1 ];
      self->M1paramReal[ps][pb][1][0] = m21 * hReal[ 0 ][ 0 ] + m22 * hReal[ 1 ][ 0 ];
      self->M1paramReal[ps][pb][1][1] = m21 * hReal[ 0 ][ 1 ] + m22 * hReal[ 1 ][ 1 ];
      self->M1paramReal[ps][pb][2][0] = m31 * hReal[ 0 ][ 0 ] + m32 * hReal[ 1 ][ 0 ];
      self->M1paramReal[ps][pb][2][1] = m31 * hReal[ 0 ][ 1 ] + m32 * hReal[ 1 ][ 1 ];

      self->M1paramImag[ps][pb][0][0] = m11 * hImag[ 0 ][ 0 ] + m12 * hImag[ 1 ][ 0 ];
      self->M1paramImag[ps][pb][0][1] = m11 * hImag[ 0 ][ 1 ] + m12 * hImag[ 1 ][ 1 ];
      self->M1paramImag[ps][pb][1][0] = m21 * hImag[ 0 ][ 0 ] + m22 * hImag[ 1 ][ 0 ];
      self->M1paramImag[ps][pb][1][1] = m21 * hImag[ 0 ][ 1 ] + m22 * hImag[ 1 ][ 1 ];
      self->M1paramImag[ps][pb][2][0] = m31 * hImag[ 0 ][ 0 ] + m32 * hImag[ 1 ][ 0 ];
      self->M1paramImag[ps][pb][2][1] = m31 * hImag[ 0 ][ 1 ] + m32 * hImag[ 1 ][ 1 ];

      self->M1paramReal[ps][pb][0][2] =  1;
      self->M1paramReal[ps][pb][1][2] =  1;
      self->M1paramReal[ps][pb][2][2] = -1;

      self->M1paramImag[ps][pb][0][2] = 0;
      self->M1paramImag[ps][pb][1][2] = 0;
      self->M1paramImag[ps][pb][2][2] = 0;

      for (col = 0; col < 3; col++) {
        for (row = 0; row < 3; row++) {
          self->M1paramReal[ps][pb][row][col] *= 1.0f / 3.0f;
          self->M1paramImag[ps][pb][row][col] *= 1.0f / 3.0f;
        }

        self->M1paramReal[ps][pb][2][col] *= (float) sqrt(2);
        self->M1paramImag[ps][pb][2][col] *= (float) sqrt(2);

        
        self->M1paramReal[ps][pb][3][col] = self->M1paramReal[ps][pb][0][col];
        self->M1paramImag[ps][pb][3][col] = self->M1paramImag[ps][pb][0][col];
        self->M1paramReal[ps][pb][4][col] = self->M1paramReal[ps][pb][1][col];
        self->M1paramImag[ps][pb][4][col] = self->M1paramImag[ps][pb][1][col];
        self->M1paramReal[ps][pb][5][col] = 0;
        self->M1paramImag[ps][pb][5][col] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][3] = H12[pb];
      self->M2decorReal[ps][pb][0][4] = 0;

      self->M2decorReal[ps][pb][1][3] = H22[pb];
      self->M2decorReal[ps][pb][1][4] = 0;

      self->M2decorReal[ps][pb][2][3] = 0;
      self->M2decorReal[ps][pb][2][4] = H12[pb];

      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = H22[pb];

      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = 0;

      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = 0;

      for (row = 0; row < 6; row++) {
        self->M2decorReal[ps][pb][row][0] = 0;
        self->M2decorReal[ps][pb][row][1] = 0;
        self->M2decorReal[ps][pb][row][2] = 0;
        self->M2decorReal[ps][pb][row][5] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = H11[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residReal[ps][pb][0][2] = 0;

      self->M2residReal[ps][pb][1][0] = H21[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residReal[ps][pb][1][2] = 0;

      self->M2residReal[ps][pb][2][0] = 0;
      self->M2residReal[ps][pb][2][1] = H11[pb];
      self->M2residReal[ps][pb][2][2] = 0;

      self->M2residReal[ps][pb][3][0] = 0;
      self->M2residReal[ps][pb][3][1] = H21[pb];
      self->M2residReal[ps][pb][3][2] = 0;

      self->M2residReal[ps][pb][4][0] = 0;
      self->M2residReal[ps][pb][4][1] = 0;
      self->M2residReal[ps][pb][4][2] = 1;

      self->M2residReal[ps][pb][5][0] = 0;
      self->M2residReal[ps][pb][5][1] = 0;
      self->M2residReal[ps][pb][5][2] = 0;

      for (row = 0; row < 6; row++) {
        self->M2residReal[ps][pb][row][3] = 0;
        self->M2residReal[ps][pb][row][4] = 0;
        self->M2residReal[ps][pb][row][5] = 0;
      }
    }
  }
}



static void SpatialDecCalculateM1andM2_7271(spatialDec* self) {

  int ps, pb, col, row, i;

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_f_L     [MAX_PARAMETER_BANDS];       float c_f_R     [MAX_PARAMETER_BANDS];

  float H11_LC    [MAX_PARAMETER_BANDS];       float H11_RC    [MAX_PARAMETER_BANDS];
  float H12_LC    [MAX_PARAMETER_BANDS];       float H12_RC    [MAX_PARAMETER_BANDS];
  float H21_LC    [MAX_PARAMETER_BANDS];       float H21_RC    [MAX_PARAMETER_BANDS];
  float H22_LC    [MAX_PARAMETER_BANDS];       float H22_RC    [MAX_PARAMETER_BANDS];
  float H12_res_LC[MAX_PARAMETER_BANDS];       float H12_res_RC[MAX_PARAMETER_BANDS];
  float H22_res_LC[MAX_PARAMETER_BANDS];       float H22_res_RC[MAX_PARAMETER_BANDS];
  float c_f_LC    [MAX_PARAMETER_BANDS];       float c_f_RC    [MAX_PARAMETER_BANDS];

  float c_l_CLFE  [MAX_PARAMETER_BANDS];
  float c_r_CLFE  [MAX_PARAMETER_BANDS];
  float dummy     [MAX_PARAMETER_BANDS];
  float kappa     [MAX_PARAMETER_BANDS];
  float G_dd      [MAX_PARAMETER_BANDS];

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  for (ps = 0; ps < self->numParameterSets; ps++) {

    param2UMX_PS(self, H11_L,  H12_L,  H21_L,  H22_L,  H12_res_L,  H22_res_L,  c_f_L,  dummy, 1, ps, self->resBands[1]);
    param2UMX_PS(self, H11_R,  H12_R,  H21_R,  H22_R,  H12_res_R,  H22_res_R,  c_f_R,  dummy, 2, ps, self->resBands[2]);
    param2UMX_PS(self, H11_LC, H12_LC, H21_LC, H22_LC, H12_res_LC, H22_res_LC, c_f_LC, dummy, 3, ps, self->resBands[3]);
    param2UMX_PS(self, H11_RC, H12_RC, H21_RC, H22_RC, H12_res_RC, H22_res_RC, c_f_RC, dummy, 4, ps, self->resBands[4]);

    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {

        float M_pre[3][5];
        float M_ttt[3][3];
        int   mtxInversion = self->mtxInversion;

        memset(M_pre,0,sizeof(M_pre));

        if (self->tttConfig[i][0].mode >= 2)
          {
            mtxInversion = mtxInversion && (self->tttConfig[i][0].mode == 2 || self->tttConfig[i][0].mode == 4);
          }

          
        CalculateTTT(self, ps, pb, self->tttConfig[i][0].mode, M_ttt);

        for (row = 0; row < 3; row++) {
          for (col = 0; col < 3; col++) {
            M_pre[row][col] = M_ttt[row][col];
          }
        }


          
        if (self->arbitraryDownmix != 0) {
          float gReal[2];
          int   ch;

          CalculateArbDmxMtx(self, ps, pb, gReal);

            
          for (ch = 0; ch < self->numInputChannels; ch++) {
            for (row = 0; row < 3; row++) {
              M_pre[row][ch] *= gReal[ch];

                
              if (self->arbitraryDownmix == 2 && pb < self->arbdmxResidualBands) {
                M_pre[row][3 + ch] = M_ttt[row][ch];
              }
            }
          }
        }

        if (mtxInversion) {
          float hReal[2][2];
          float hImag[2][2];

            
          CalculateMTXinv(self, ps, pb, self->tttConfig[i][0].mode, hReal, hImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * hReal[ 0 ][ 0 ] + M_pre[0][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * hReal[ 0 ][ 1 ] + M_pre[0][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * hReal[ 0 ][ 0 ] + M_pre[1][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * hReal[ 0 ][ 1 ] + M_pre[1][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * hReal[ 0 ][ 0 ] + M_pre[2][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * hReal[ 0 ][ 1 ] + M_pre[2][1] * hReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * hImag[ 0 ][ 0 ] + M_pre[0][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * hImag[ 0 ][ 1 ] + M_pre[0][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * hImag[ 0 ][ 0 ] + M_pre[1][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * hImag[ 0 ][ 1 ] + M_pre[1][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * hImag[ 0 ][ 0 ] + M_pre[2][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * hImag[ 0 ][ 1 ] + M_pre[2][1] * hImag[ 1 ][ 1 ];
        }
        else if (self->_3DStereoInversion) {
          float qReal[2][2];
          float qImag[2][2];

            
          Calculate3Dinv(self, ps, pb, M_pre, qReal, qImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * qReal[ 0 ][ 0 ] + M_pre[0][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * qReal[ 0 ][ 1 ] + M_pre[0][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * qReal[ 0 ][ 0 ] + M_pre[1][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * qReal[ 0 ][ 1 ] + M_pre[1][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * qReal[ 0 ][ 0 ] + M_pre[2][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * qReal[ 0 ][ 1 ] + M_pre[2][1] * qReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * qImag[ 0 ][ 0 ] + M_pre[0][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * qImag[ 0 ][ 1 ] + M_pre[0][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * qImag[ 0 ][ 0 ] + M_pre[1][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * qImag[ 0 ][ 1 ] + M_pre[1][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * qImag[ 0 ][ 0 ] + M_pre[2][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * qImag[ 0 ][ 1 ] + M_pre[2][1] * qImag[ 1 ][ 1 ];
        }
        else
          {
            self->M1paramReal[ps][pb][0][0] = M_pre[0][0];
            self->M1paramReal[ps][pb][0][1] = M_pre[0][1];
            self->M1paramReal[ps][pb][1][0] = M_pre[1][0];
            self->M1paramReal[ps][pb][1][1] = M_pre[1][1];
            self->M1paramReal[ps][pb][2][0] = M_pre[2][0];
            self->M1paramReal[ps][pb][2][1] = M_pre[2][1];

            self->M1paramImag[ps][pb][0][0] = 0;
            self->M1paramImag[ps][pb][0][1] = 0;
            self->M1paramImag[ps][pb][1][0] = 0;
            self->M1paramImag[ps][pb][1][1] = 0;
            self->M1paramImag[ps][pb][2][0] = 0;
            self->M1paramImag[ps][pb][2][1] = 0;
          }

        self->M1paramReal[ps][pb][0][2] = M_pre[0][2];
        self->M1paramReal[ps][pb][0][3] = M_pre[0][3];
        self->M1paramReal[ps][pb][0][4] = M_pre[0][4];
        self->M1paramReal[ps][pb][1][2] = M_pre[1][2];
        self->M1paramReal[ps][pb][1][3] = M_pre[1][3];
        self->M1paramReal[ps][pb][1][4] = M_pre[1][4];
        self->M1paramReal[ps][pb][2][2] = M_pre[2][2];
        self->M1paramReal[ps][pb][2][3] = M_pre[2][3];
        self->M1paramReal[ps][pb][2][4] = M_pre[2][4];

        for (col = 0; col < self->numXChannels; col++) {

          self->M1paramReal[ps][pb][3][col] = self->M1paramReal[ps][pb][0][col];
          self->M1paramImag[ps][pb][3][col] = self->M1paramImag[ps][pb][0][col];
          self->M1paramReal[ps][pb][4][col] = self->M1paramReal[ps][pb][1][col];
          self->M1paramImag[ps][pb][4][col] = self->M1paramImag[ps][pb][1][col];

          if (self->tttConfig[i][0].useTTTdecorr) {
            self->M1paramReal[ps][pb][5][col] = self->M1paramReal[ps][pb][2][col];
            self->M1paramImag[ps][pb][5][col] = self->M1paramImag[ps][pb][2][col];
          } else {
            self->M1paramReal[ps][pb][5][col] = 0;
            self->M1paramImag[ps][pb][5][col] = 0;
          }

          self->M1paramReal[ps][pb][6][col] = c_f_L[pb] * self->M1paramReal[ps][pb][0][col];
          self->M1paramImag[ps][pb][6][col] = c_f_L[pb] * self->M1paramImag[ps][pb][0][col];
          self->M1paramReal[ps][pb][7][col] = c_f_R[pb] * self->M1paramReal[ps][pb][1][col];
          self->M1paramImag[ps][pb][7][col] = c_f_R[pb] * self->M1paramImag[ps][pb][1][col];
        }

      }
    }

      
    for (pb = 0; pb < self->numOttBands[0]; pb++) {
      float iid_lin = (float) (pow(10, (self->ottCLD[0][ps][pb] / 20.0f)));
      c_l_CLFE[pb] = (float) (sqrt(iid_lin * iid_lin / (1 + iid_lin * iid_lin)));
      c_r_CLFE[pb] = (float) (sqrt(                1 / (1 + iid_lin * iid_lin)));
    }
    for (pb = self->numOttBands[0]; pb < self->numParameterBands; pb++) {
      c_l_CLFE[pb] = 1;
      c_r_CLFE[pb] = 0;
    }

      
    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][3] = H11_LC[pb] * H12_L[pb];
      self->M2decorReal[ps][pb][0][4] = 0;
      self->M2decorReal[ps][pb][0][6] = H12_LC[pb];
      self->M2decorReal[ps][pb][0][7] = 0;

      self->M2decorReal[ps][pb][1][3] = H21_LC[pb] * H12_L[pb];
      self->M2decorReal[ps][pb][1][4] = 0;
      self->M2decorReal[ps][pb][1][6] = H22_LC[pb];
      self->M2decorReal[ps][pb][1][7] = 0;

      self->M2decorReal[ps][pb][2][3] = H22_L[pb];
      self->M2decorReal[ps][pb][2][4] = 0;
      self->M2decorReal[ps][pb][2][6] = 0;
      self->M2decorReal[ps][pb][2][7] = 0;

      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = H11_RC[pb] * H12_R[pb];
      self->M2decorReal[ps][pb][3][6] = 0;
      self->M2decorReal[ps][pb][3][7] = H12_RC[pb];

      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = H21_RC[pb] * H12_R[pb];
      self->M2decorReal[ps][pb][4][6] = 0;
      self->M2decorReal[ps][pb][4][7] = H22_RC[pb];

      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = H22_R[pb];
      self->M2decorReal[ps][pb][5][6] = 0;
      self->M2decorReal[ps][pb][5][7] = 0;

      self->M2decorReal[ps][pb][6][3] = 0;
      self->M2decorReal[ps][pb][6][4] = 0;
      self->M2decorReal[ps][pb][6][6] = 0;
      self->M2decorReal[ps][pb][6][7] = 0;

      self->M2decorReal[ps][pb][7][3] = 0;
      self->M2decorReal[ps][pb][7][4] = 0;
      self->M2decorReal[ps][pb][7][6] = 0;
      self->M2decorReal[ps][pb][7][7] = 0;

      for (row = 0; row < 8; row++) {
        self->M2decorReal[ps][pb][row][0] = 0;
        self->M2decorReal[ps][pb][row][1] = 0;
        self->M2decorReal[ps][pb][row][2] = 0;
        self->M2decorReal[ps][pb][row][5] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = H11_LC[pb] * H11_L[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residReal[ps][pb][0][2] = 0;
      self->M2residReal[ps][pb][0][3] = H11_LC[pb] * H12_res_L[pb];
      self->M2residReal[ps][pb][0][4] = 0;
      self->M2residReal[ps][pb][0][6] = H12_res_LC[pb];
      self->M2residReal[ps][pb][0][7] = 0;

      self->M2residReal[ps][pb][1][0] = H21_LC[pb] * H11_L[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residReal[ps][pb][1][2] = 0;
      self->M2residReal[ps][pb][1][3] = H21_LC[pb] * H12_res_L[pb];
      self->M2residReal[ps][pb][1][4] = 0;
      self->M2residReal[ps][pb][1][6] = H22_res_LC[pb];
      self->M2residReal[ps][pb][1][7] = 0;

      self->M2residReal[ps][pb][2][0] = H21_L[pb];
      self->M2residReal[ps][pb][2][1] = 0;
      self->M2residReal[ps][pb][2][2] = 0;
      self->M2residReal[ps][pb][2][3] = H22_res_L[pb];
      self->M2residReal[ps][pb][2][4] = 0;
      self->M2residReal[ps][pb][2][6] = 0;
      self->M2residReal[ps][pb][2][7] = 0;

      self->M2residReal[ps][pb][3][0] = 0;
      self->M2residReal[ps][pb][3][1] = H11_RC[pb] * H11_R[pb];
      self->M2residReal[ps][pb][3][2] = 0;
      self->M2residReal[ps][pb][3][3] = 0;
      self->M2residReal[ps][pb][3][4] = H11_RC[pb] *  H12_res_R[pb];
      self->M2residReal[ps][pb][3][6] = 0;
      self->M2residReal[ps][pb][3][7] = H12_res_RC[pb];

      self->M2residReal[ps][pb][4][0] = 0;
      self->M2residReal[ps][pb][4][1] = H21_RC[pb] * H11_R[pb];
      self->M2residReal[ps][pb][4][2] = 0;
      self->M2residReal[ps][pb][4][3] = 0;
      self->M2residReal[ps][pb][4][4] = H21_RC[pb] * H12_res_R[pb];
      self->M2residReal[ps][pb][4][6] = 0;
      self->M2residReal[ps][pb][4][7] = H22_res_RC[pb];

      self->M2residReal[ps][pb][5][0] = 0;
      self->M2residReal[ps][pb][5][1] = H21_R[pb];
      self->M2residReal[ps][pb][5][2] = 0;
      self->M2residReal[ps][pb][5][3] = 0;
      self->M2residReal[ps][pb][5][4] = H22_res_R[pb];
      self->M2residReal[ps][pb][5][6] = 0;
      self->M2residReal[ps][pb][5][7] = 0;

      self->M2residReal[ps][pb][6][0] = 0;
      self->M2residReal[ps][pb][6][1] = 0;
      self->M2residReal[ps][pb][6][2] = c_l_CLFE[pb];
      self->M2residReal[ps][pb][6][3] = 0;
      self->M2residReal[ps][pb][6][4] = 0;
      self->M2residReal[ps][pb][6][6] = 0;
      self->M2residReal[ps][pb][6][7] = 0;

      self->M2residReal[ps][pb][7][0] = 0;
      self->M2residReal[ps][pb][7][1] = 0;
      self->M2residReal[ps][pb][7][2] = c_r_CLFE[pb];
      self->M2residReal[ps][pb][7][3] = 0;
      self->M2residReal[ps][pb][7][4] = 0;
      self->M2residReal[ps][pb][7][6] = 0;
      self->M2residReal[ps][pb][7][7] = 0;

      for (row = 0; row < 8; row++) {
        self->M2residReal[ps][pb][row][5] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      kappa[pb] = self->tttICC[0][ps][pb];
      G_dd[pb]  = (float) (sqrt(1 - kappa[pb] * kappa[pb]) / sqrt(3));
    }

    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {
        if (self->tttConfig[i][0].mode < 2 && pb >= self->resBands[5]) {
          if (self->tttConfig[i][0].useTTTdecorr) {
            self->M2decorReal[ps][pb][0][5] = G_dd[pb] * c_f_L[pb] * H11_LC[pb];
            self->M2decorReal[ps][pb][1][5] = G_dd[pb] * c_f_L[pb] * H21_LC[pb];
            self->M2decorReal[ps][pb][3][5] = G_dd[pb] * c_f_R[pb] * H11_RC[pb];
            self->M2decorReal[ps][pb][4][5] = G_dd[pb] * c_f_R[pb] * H21_RC[pb];
            self->M2decorReal[ps][pb][6][5] = (float) (-sqrt(2) * G_dd[pb] * c_l_CLFE[pb]);

            self->M2residReal[ps][pb][0][0] *= kappa[pb];
            self->M2residReal[ps][pb][1][0] *= kappa[pb];
            self->M2residReal[ps][pb][3][1] *= kappa[pb];
            self->M2residReal[ps][pb][4][1] *= kappa[pb];
            self->M2residReal[ps][pb][6][2] *= kappa[pb];
          }
        }
      }
    }
  }
}



static void SpatialDecCalculateM1andM2_7272(spatialDec* self) {

  int ps, pb, col, row, i;

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_1_L     [MAX_PARAMETER_BANDS];       float c_1_R     [MAX_PARAMETER_BANDS];
  float c_2_L     [MAX_PARAMETER_BANDS];       float c_2_R     [MAX_PARAMETER_BANDS];

  float H11_LS    [MAX_PARAMETER_BANDS];       float H11_RS    [MAX_PARAMETER_BANDS];
  float H12_LS    [MAX_PARAMETER_BANDS];       float H12_RS    [MAX_PARAMETER_BANDS];
  float H21_LS    [MAX_PARAMETER_BANDS];       float H21_RS    [MAX_PARAMETER_BANDS];
  float H22_LS    [MAX_PARAMETER_BANDS];       float H22_RS    [MAX_PARAMETER_BANDS];
  float H12_res_LS[MAX_PARAMETER_BANDS];       float H12_res_RS[MAX_PARAMETER_BANDS];
  float H22_res_LS[MAX_PARAMETER_BANDS];       float H22_res_RS[MAX_PARAMETER_BANDS];
  float c_f_LS    [MAX_PARAMETER_BANDS];       float c_f_RS    [MAX_PARAMETER_BANDS];

  float c_l_CLFE  [MAX_PARAMETER_BANDS];
  float c_r_CLFE  [MAX_PARAMETER_BANDS];
  float dummy     [MAX_PARAMETER_BANDS];
  float kappa     [MAX_PARAMETER_BANDS];
  float G_dd      [MAX_PARAMETER_BANDS];

#ifdef PARTIALLY_COMPLEX
  int complexBands = self->kernels[self->hybridBands - self->qmfBands + PC_NUM_BANDS][0] - 1;
#endif

  for (ps = 0; ps < self->numParameterSets; ps++) {

    param2UMX_PS(self, H11_L,  H12_L,  H21_L,  H22_L,  H12_res_L,  H22_res_L,  c_1_L, c_2_L,  1, ps, self->resBands[1]);
    param2UMX_PS(self, H11_R,  H12_R,  H21_R,  H22_R,  H12_res_R,  H22_res_R,  c_1_R, c_2_R,  2, ps, self->resBands[2]);
    param2UMX_PS(self, H11_LS, H12_LS, H21_LS, H22_LS, H12_res_LS, H22_res_LS, dummy, c_f_LS, 3, ps, self->resBands[3]);
    param2UMX_PS(self, H11_RS, H12_RS, H21_RS, H22_RS, H12_res_RS, H22_res_RS, dummy, c_f_RS, 4, ps, self->resBands[4]);


      
    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {

        float M_pre[3][5];
        float M_ttt[3][3];
        int   mtxInversion = self->mtxInversion;

        memset(M_pre,0,sizeof(M_pre));

        if (self->tttConfig[i][0].mode >= 2)
          {
            mtxInversion = mtxInversion && (self->tttConfig[i][0].mode == 2 || self->tttConfig[i][0].mode == 4);
          }

          
        CalculateTTT(self, ps, pb, self->tttConfig[i][0].mode, M_ttt);

        for (row = 0; row < 3; row++) {
          for (col = 0; col < 3; col++) {
            M_pre[row][col] = M_ttt[row][col];
          }
        }


          
        if (self->arbitraryDownmix != 0) {
          float gReal[2];
          int   ch;

          CalculateArbDmxMtx(self, ps, pb, gReal);

            
          for (ch = 0; ch < self->numInputChannels; ch++) {
            for (row = 0; row < 3; row++) {
              M_pre[row][ch] *= gReal[ch];

                
              if (self->arbitraryDownmix == 2 && pb < self->arbdmxResidualBands) {
                M_pre[row][3 + ch] = M_ttt[row][ch];
              }
            }
          }
        }


        if (mtxInversion) {
          float hReal[2][2];
          float hImag[2][2];

            
          CalculateMTXinv(self, ps, pb, self->tttConfig[i][0].mode, hReal, hImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * hReal[ 0 ][ 0 ] + M_pre[0][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * hReal[ 0 ][ 1 ] + M_pre[0][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * hReal[ 0 ][ 0 ] + M_pre[1][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * hReal[ 0 ][ 1 ] + M_pre[1][1] * hReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * hReal[ 0 ][ 0 ] + M_pre[2][1] * hReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * hReal[ 0 ][ 1 ] + M_pre[2][1] * hReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * hImag[ 0 ][ 0 ] + M_pre[0][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * hImag[ 0 ][ 1 ] + M_pre[0][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * hImag[ 0 ][ 0 ] + M_pre[1][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * hImag[ 0 ][ 1 ] + M_pre[1][1] * hImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * hImag[ 0 ][ 0 ] + M_pre[2][1] * hImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * hImag[ 0 ][ 1 ] + M_pre[2][1] * hImag[ 1 ][ 1 ];
        }
        else if (self->_3DStereoInversion) {
          float qReal[2][2];
          float qImag[2][2];

            
          Calculate3Dinv(self, ps, pb, M_pre, qReal, qImag);

            
          self->M1paramReal[ps][pb][0][0] = M_pre[0][0] * qReal[ 0 ][ 0 ] + M_pre[0][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][0][1] = M_pre[0][0] * qReal[ 0 ][ 1 ] + M_pre[0][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][1][0] = M_pre[1][0] * qReal[ 0 ][ 0 ] + M_pre[1][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][1][1] = M_pre[1][0] * qReal[ 0 ][ 1 ] + M_pre[1][1] * qReal[ 1 ][ 1 ];
          self->M1paramReal[ps][pb][2][0] = M_pre[2][0] * qReal[ 0 ][ 0 ] + M_pre[2][1] * qReal[ 1 ][ 0 ];
          self->M1paramReal[ps][pb][2][1] = M_pre[2][0] * qReal[ 0 ][ 1 ] + M_pre[2][1] * qReal[ 1 ][ 1 ];

          self->M1paramImag[ps][pb][0][0] = M_pre[0][0] * qImag[ 0 ][ 0 ] + M_pre[0][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][0][1] = M_pre[0][0] * qImag[ 0 ][ 1 ] + M_pre[0][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][1][0] = M_pre[1][0] * qImag[ 0 ][ 0 ] + M_pre[1][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][1][1] = M_pre[1][0] * qImag[ 0 ][ 1 ] + M_pre[1][1] * qImag[ 1 ][ 1 ];
          self->M1paramImag[ps][pb][2][0] = M_pre[2][0] * qImag[ 0 ][ 0 ] + M_pre[2][1] * qImag[ 1 ][ 0 ];
          self->M1paramImag[ps][pb][2][1] = M_pre[2][0] * qImag[ 0 ][ 1 ] + M_pre[2][1] * qImag[ 1 ][ 1 ];
        }
        else
          {
            self->M1paramReal[ps][pb][0][0] = M_pre[0][0];
            self->M1paramReal[ps][pb][0][1] = M_pre[0][1];
            self->M1paramReal[ps][pb][1][0] = M_pre[1][0];
            self->M1paramReal[ps][pb][1][1] = M_pre[1][1];
            self->M1paramReal[ps][pb][2][0] = M_pre[2][0];
            self->M1paramReal[ps][pb][2][1] = M_pre[2][1];

            self->M1paramImag[ps][pb][0][0] = 0;
            self->M1paramImag[ps][pb][0][1] = 0;
            self->M1paramImag[ps][pb][1][0] = 0;
            self->M1paramImag[ps][pb][1][1] = 0;
            self->M1paramImag[ps][pb][2][0] = 0;
            self->M1paramImag[ps][pb][2][1] = 0;
          }

        self->M1paramReal[ps][pb][0][2] = M_pre[0][2];
        self->M1paramReal[ps][pb][0][3] = M_pre[0][3];
        self->M1paramReal[ps][pb][0][4] = M_pre[0][4];
        self->M1paramReal[ps][pb][1][2] = M_pre[1][2];
        self->M1paramReal[ps][pb][1][3] = M_pre[1][3];
        self->M1paramReal[ps][pb][1][4] = M_pre[1][4];
        self->M1paramReal[ps][pb][2][2] = M_pre[2][2];
        self->M1paramReal[ps][pb][2][3] = M_pre[2][3];
        self->M1paramReal[ps][pb][2][4] = M_pre[2][4];


        for (col = 0; col < self->numXChannels; col++) {

          self->M1paramReal[ps][pb][3][col] = self->M1paramReal[ps][pb][0][col];
          self->M1paramImag[ps][pb][3][col] = self->M1paramImag[ps][pb][0][col];
          self->M1paramReal[ps][pb][4][col] = self->M1paramReal[ps][pb][1][col];
          self->M1paramImag[ps][pb][4][col] = self->M1paramImag[ps][pb][1][col];

          if (self->tttConfig[i][0].useTTTdecorr) {
            self->M1paramReal[ps][pb][5][col] = self->M1paramReal[ps][pb][2][col];
            self->M1paramImag[ps][pb][5][col] = self->M1paramImag[ps][pb][2][col];
          } else {
            self->M1paramReal[ps][pb][5][col] = 0;
            self->M1paramImag[ps][pb][5][col] = 0;
          }

          self->M1paramReal[ps][pb][6][col] = c_2_L[pb] * self->M1paramReal[ps][pb][0][col];
          self->M1paramImag[ps][pb][6][col] = c_2_L[pb] * self->M1paramImag[ps][pb][0][col];
          self->M1paramReal[ps][pb][7][col] = c_2_R[pb] * self->M1paramReal[ps][pb][1][col];
          self->M1paramImag[ps][pb][7][col] = c_2_R[pb] * self->M1paramImag[ps][pb][1][col];
        }
      }
    }


      
    for (pb = 0; pb < self->numOttBands[0]; pb++) {
      float iid_lin = (float) (pow(10, (self->ottCLD[0][ps][pb] / 20.0f)));
      c_l_CLFE[pb] = (float) (sqrt(iid_lin * iid_lin / (1 + iid_lin * iid_lin)));
      c_r_CLFE[pb] = (float) (sqrt(                1 / (1 + iid_lin * iid_lin)));
    }
    for (pb = self->numOttBands[0]; pb < self->numParameterBands; pb++) {
      c_l_CLFE[pb] = 1;
      c_r_CLFE[pb] = 0;
    }


      
    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2decorReal[ps][pb][0][3] = H12_L[pb];
      self->M2decorReal[ps][pb][0][4] = 0;
      self->M2decorReal[ps][pb][0][6] = 0;
      self->M2decorReal[ps][pb][0][7] = 0;

      self->M2decorReal[ps][pb][1][3] = H11_LS[pb] * H22_L[pb];
      self->M2decorReal[ps][pb][1][4] = 0;
      self->M2decorReal[ps][pb][1][6] = H12_LS[pb];
      self->M2decorReal[ps][pb][1][7] = 0;

      self->M2decorReal[ps][pb][2][3] = H21_LS[pb] * H22_L[pb];
      self->M2decorReal[ps][pb][2][4] = 0;
      self->M2decorReal[ps][pb][2][6] = H22_LS[pb];
      self->M2decorReal[ps][pb][2][7] = 0;

      self->M2decorReal[ps][pb][3][3] = 0;
      self->M2decorReal[ps][pb][3][4] = H12_R[pb];
      self->M2decorReal[ps][pb][3][6] = 0;
      self->M2decorReal[ps][pb][3][7] = 0;

      self->M2decorReal[ps][pb][4][3] = 0;
      self->M2decorReal[ps][pb][4][4] = H11_RS[pb] * H22_R[pb];
      self->M2decorReal[ps][pb][4][6] = 0;
      self->M2decorReal[ps][pb][4][7] = H12_RS[pb];

      self->M2decorReal[ps][pb][5][3] = 0;
      self->M2decorReal[ps][pb][5][4] = H21_RS[pb] * H22_R[pb];
      self->M2decorReal[ps][pb][5][6] = 0;
      self->M2decorReal[ps][pb][5][7] = H22_RS[pb];

      self->M2decorReal[ps][pb][6][3] = 0;
      self->M2decorReal[ps][pb][6][4] = 0;
      self->M2decorReal[ps][pb][6][6] = 0;
      self->M2decorReal[ps][pb][6][7] = 0;

      self->M2decorReal[ps][pb][7][3] = 0;
      self->M2decorReal[ps][pb][7][4] = 0;
      self->M2decorReal[ps][pb][7][6] = 0;
      self->M2decorReal[ps][pb][7][7] = 0;

      for (row = 0; row < 8; row++) {
        self->M2decorReal[ps][pb][row][0] = 0;
        self->M2decorReal[ps][pb][row][1] = 0;
        self->M2decorReal[ps][pb][row][2] = 0;
        self->M2decorReal[ps][pb][row][5] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      self->M2residReal[ps][pb][0][0] = H11_L[pb];
      self->M2residReal[ps][pb][0][1] = 0;
      self->M2residReal[ps][pb][0][2] = 0;
      self->M2residReal[ps][pb][0][3] = H12_res_L[pb];
      self->M2residReal[ps][pb][0][4] = 0;
      self->M2residReal[ps][pb][0][6] = 0;
      self->M2residReal[ps][pb][0][7] = 0;

      self->M2residReal[ps][pb][1][0] = H11_LS[pb] * H21_L[pb];
      self->M2residReal[ps][pb][1][1] = 0;
      self->M2residReal[ps][pb][1][2] = 0;
      self->M2residReal[ps][pb][1][3] = H11_LS[pb] * H22_res_L[pb];
      self->M2residReal[ps][pb][1][4] = 0;
      self->M2residReal[ps][pb][1][6] = H12_res_LS[pb];
      self->M2residReal[ps][pb][1][7] = 0;

      self->M2residReal[ps][pb][2][0] = H21_LS[pb] * H21_L[pb];
      self->M2residReal[ps][pb][2][1] = 0;
      self->M2residReal[ps][pb][2][2] = 0;
      self->M2residReal[ps][pb][2][3] = H21_LS[pb] * H22_res_L[pb];
      self->M2residReal[ps][pb][2][4] = 0;
      self->M2residReal[ps][pb][2][6] = H22_res_LS[pb];
      self->M2residReal[ps][pb][2][7] = 0;

      self->M2residReal[ps][pb][3][0] = 0;
      self->M2residReal[ps][pb][3][1] = H11_R[pb];
      self->M2residReal[ps][pb][3][2] = 0;
      self->M2residReal[ps][pb][3][3] = 0;
      self->M2residReal[ps][pb][3][4] = H12_res_R[pb];
      self->M2residReal[ps][pb][3][6] = 0;
      self->M2residReal[ps][pb][3][7] = 0;

      self->M2residReal[ps][pb][4][0] = 0;
      self->M2residReal[ps][pb][4][1] = H11_RS[pb] * H21_R[pb];
      self->M2residReal[ps][pb][4][2] = 0;
      self->M2residReal[ps][pb][4][3] = 0;
      self->M2residReal[ps][pb][4][4] = H11_RS[pb] * H22_res_R[pb];
      self->M2residReal[ps][pb][4][6] = 0;
      self->M2residReal[ps][pb][4][7] = H12_res_RS[pb];

      self->M2residReal[ps][pb][5][0] = 0;
      self->M2residReal[ps][pb][5][1] = H21_RS[pb] * H21_R[pb];
      self->M2residReal[ps][pb][5][2] = 0;
      self->M2residReal[ps][pb][5][3] = 0;
      self->M2residReal[ps][pb][5][4] = H21_RS[pb] * H22_res_R[pb];
      self->M2residReal[ps][pb][5][6] = 0;
      self->M2residReal[ps][pb][5][7] = H22_res_RS[pb];

      self->M2residReal[ps][pb][6][0] = 0;
      self->M2residReal[ps][pb][6][1] = 0;
      self->M2residReal[ps][pb][6][2] = c_l_CLFE[pb];
      self->M2residReal[ps][pb][6][3] = 0;
      self->M2residReal[ps][pb][6][4] = 0;
      self->M2residReal[ps][pb][6][6] = 0;
      self->M2residReal[ps][pb][6][7] = 0;

      self->M2residReal[ps][pb][7][0] = 0;
      self->M2residReal[ps][pb][7][1] = 0;
      self->M2residReal[ps][pb][7][2] = c_r_CLFE[pb];
      self->M2residReal[ps][pb][7][3] = 0;
      self->M2residReal[ps][pb][7][4] = 0;
      self->M2residReal[ps][pb][7][6] = 0;
      self->M2residReal[ps][pb][7][7] = 0;

      for (row = 0; row < 8; row++) {
        self->M2residReal[ps][pb][row][5] = 0;
      }
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      kappa[pb] = self->tttICC[0][ps][pb];
      G_dd[pb]  = (float) (sqrt(1 - kappa[pb] * kappa[pb]) / sqrt(3));
    }

    for (i = 0; i < 2; i++) {
      for (pb = self->tttConfig[i][0].startBand; pb < self->tttConfig[i][0].stopBand; pb++) {
        if (self->tttConfig[i][0].mode < 2 && pb >= self->resBands[5]) {
          if (self->tttConfig[i][0].useTTTdecorr) {
            self->M2decorReal[ps][pb][0][5] = G_dd[pb] * c_1_L[pb];
            self->M2decorReal[ps][pb][3][5] = G_dd[pb] * c_1_R[pb];
            self->M2decorReal[ps][pb][6][5] = (float) (-sqrt(2) * G_dd[pb] * c_l_CLFE[pb]);

            self->M2residReal[ps][pb][0][0] *= kappa[pb];
            self->M2residReal[ps][pb][3][1] *= kappa[pb];
            self->M2residReal[ps][pb][6][2] *= kappa[pb];
          }
        }
      }
    }
  }
}



static void SpatialDecCalculateM1andM2_7571(spatialDec* self) {

  int ps, pb, col, row;

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_f_L     [MAX_PARAMETER_BANDS];       float c_f_R     [MAX_PARAMETER_BANDS];

  float dummy     [MAX_PARAMETER_BANDS];

  for (ps=0; ps < self->numParameterSets; ps++) {

    param2UMX_PS(self, H11_L, H12_L, H21_L, H22_L, H12_res_L, H22_res_L, c_f_L, dummy, 0, ps, self->resBands[0]);
    param2UMX_PS(self, H11_R, H12_R, H21_R, H22_R, H12_res_R, H22_res_R, c_f_R, dummy, 1, ps, self->resBands[1]);

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 6; col++) {
        for (row = 0; row < 8; row++) {
          self->M1paramReal[ps][pb][row][col] = 0;
          self->M1paramImag[ps][pb][row][col] = 0;
        }
      }

      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][1] = 1;
      self->M1paramReal[ps][pb][2][2] = 1;
      self->M1paramReal[ps][pb][3][3] = 1;
      self->M1paramReal[ps][pb][4][4] = 1;
      self->M1paramReal[ps][pb][5][5] = 1;
      self->M1paramReal[ps][pb][6][0] = 1;
      self->M1paramReal[ps][pb][7][1] = 1;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 8; col++) {
        for (row = 0; row < 8; row++) {
          self->M2decorReal[ps][pb][row][col] = 0;
        }
      }

      self->M2decorReal[ps][pb][0][6] = H12_L[pb];

      self->M2decorReal[ps][pb][1][6] = H22_L[pb];

      self->M2decorReal[ps][pb][3][7] = H12_R[pb];

      self->M2decorReal[ps][pb][4][7] = H22_R[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 8; col++) {
        for (row = 0; row < 8; row++) {
          self->M2residReal[ps][pb][row][col] = 0;
        }
      }

      self->M2residReal[ps][pb][0][0] = H11_L[pb];
      self->M2residReal[ps][pb][0][6] = H12_res_L[pb];

      self->M2residReal[ps][pb][1][0] = H21_L[pb];
      self->M2residReal[ps][pb][1][6] = H22_res_L[pb];

      self->M2residReal[ps][pb][2][4] = 1;

      self->M2residReal[ps][pb][3][1] = H11_R[pb];
      self->M2residReal[ps][pb][3][7] = H12_res_R[pb];

      self->M2residReal[ps][pb][4][1] = H21_R[pb];
      self->M2residReal[ps][pb][4][7] = H22_res_R[pb];

      self->M2residReal[ps][pb][5][5] = 1;

      self->M2residReal[ps][pb][6][2] = 1;

      self->M2residReal[ps][pb][7][3] = 1;
    }
  }
}



static void SpatialDecCalculateM1andM2_7572(spatialDec* self) {

  int ps, pb, col, row;

  float H11_L     [MAX_PARAMETER_BANDS];       float H11_R     [MAX_PARAMETER_BANDS];
  float H12_L     [MAX_PARAMETER_BANDS];       float H12_R     [MAX_PARAMETER_BANDS];
  float H21_L     [MAX_PARAMETER_BANDS];       float H21_R     [MAX_PARAMETER_BANDS];
  float H22_L     [MAX_PARAMETER_BANDS];       float H22_R     [MAX_PARAMETER_BANDS];
  float H12_res_L [MAX_PARAMETER_BANDS];       float H12_res_R [MAX_PARAMETER_BANDS];
  float H22_res_L [MAX_PARAMETER_BANDS];       float H22_res_R [MAX_PARAMETER_BANDS];
  float c_f_L     [MAX_PARAMETER_BANDS];       float c_f_R     [MAX_PARAMETER_BANDS];

  float dummy     [MAX_PARAMETER_BANDS];

  for (ps=0; ps < self->numParameterSets; ps++) {

    param2UMX_PS(self, H11_L, H12_L, H21_L, H22_L, H12_res_L, H22_res_L, c_f_L, dummy, 0, ps, self->resBands[0]);
    param2UMX_PS(self, H11_R, H12_R, H21_R, H22_R, H12_res_R, H22_res_R, c_f_R, dummy, 1, ps, self->resBands[1]);

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 6; col++) {
        for (row = 0; row < 8; row++) {
          self->M1paramReal[ps][pb][row][col] = 0;
          self->M1paramImag[ps][pb][row][col] = 0;
        }
      }

      self->M1paramReal[ps][pb][0][0] = 1;
      self->M1paramReal[ps][pb][1][1] = 1;
      self->M1paramReal[ps][pb][2][2] = 1;
      self->M1paramReal[ps][pb][3][3] = 1;
      self->M1paramReal[ps][pb][4][4] = 1;
      self->M1paramReal[ps][pb][5][5] = 1;
      self->M1paramReal[ps][pb][6][4] = 1;
      self->M1paramReal[ps][pb][7][5] = 1;
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 8; col++) {
        for (row = 0; row < 8; row++) {
          self->M2decorReal[ps][pb][row][col] = 0;
        }
      }

      self->M2decorReal[ps][pb][1][6] = H22_L[pb];

      self->M2decorReal[ps][pb][2][6] = H12_L[pb];

      self->M2decorReal[ps][pb][4][7] = H22_R[pb];

      self->M2decorReal[ps][pb][5][7] = H12_R[pb];
    }

    for (pb = 0; pb < self->numParameterBands; pb++) {
      for (col = 0; col < 8; col++) {
        for (row = 0; row < 8; row++) {
          self->M2residReal[ps][pb][row][col] = 0;
        }
      }

      self->M2residReal[ps][pb][0][0] = 1;

      self->M2residReal[ps][pb][1][4] = H21_L[pb];
      self->M2residReal[ps][pb][1][6] = H22_res_L[pb];

      self->M2residReal[ps][pb][2][4] = H11_L[pb];
      self->M2residReal[ps][pb][2][6] = H12_res_L[pb];

      self->M2residReal[ps][pb][3][1] = 1;

      self->M2residReal[ps][pb][4][5] = H21_R[pb];
      self->M2residReal[ps][pb][4][7] = H22_res_R[pb];

      self->M2residReal[ps][pb][5][5] = H11_R[pb];
      self->M2residReal[ps][pb][5][7] = H12_res_R[pb];

      self->M2residReal[ps][pb][6][2] = 1;

      self->M2residReal[ps][pb][7][3] = 1;
    }
  }
}



static void param2UMX_PS_Core(float cld[MAX_PARAMETER_BANDS],
                              float icc[MAX_PARAMETER_BANDS],
                              int numOttBands,
                              int resBands,
                              float H11[MAX_PARAMETER_BANDS],
                              float H12[MAX_PARAMETER_BANDS],
                              float H21[MAX_PARAMETER_BANDS],
                              float H22[MAX_PARAMETER_BANDS],
                              float H12_res[MAX_PARAMETER_BANDS],
                              float H22_res[MAX_PARAMETER_BANDS],
                              float c_l[MAX_PARAMETER_BANDS],
                              float c_r[MAX_PARAMETER_BANDS])
{

  int band;
  float iid_lin[MAX_PARAMETER_BANDS];
  float max_inv_det = 1.2f;


  for(band = 0; band < numOttBands; band++){
    iid_lin[band] = (float) (pow(10,( cld[band]/20.0f)));
    c_l[band]     = (float) (sqrt( (iid_lin[band]*iid_lin[band]) / ( 1 + (iid_lin[band]*iid_lin[band]))));
    c_r[band]     = (float) (sqrt(                             1 / ( 1 + (iid_lin[band]*iid_lin[band]))));
  }


  for(band = 0; band < numOttBands; band++){
    if(band < resBands){
      
      float iccCorrLim = max(icc[band], (1/(max_inv_det*max_inv_det) - 1.0f)*(iid_lin[band] + 1.0f/iid_lin[band])/2.0f);
      float alpha      = (float) (0.5f * acos(iccCorrLim));
      float beta       = (float) (atan( tan(alpha) * (c_r[band] - c_l[band])/(c_r[band] + c_l[band]) ));

      H11[band] = (float) (c_l[band] * cos( alpha + beta)); 
      H21[band] = (float) (c_r[band] * cos(-alpha + beta)); 
      H12[band] = 0;                                        
      H22[band] = 0;                                        
      H12_res[band] = 1;
      H22_res[band] = -1;
    }
    else{
        
      float alpha = (float) (0.5 * acos(icc[band]));
      float beta  = (float) (atan( tan(alpha) * (c_r[band] - c_l[band])/(c_r[band] + c_l[band]) ));

      H11[band] = (float) (c_l[band] * cos( alpha + beta)); 
      H21[band] = (float) (c_r[band] * cos(-alpha + beta)); 
      H12[band] = (float) (c_l[band] * sin( alpha + beta)); 
      H22[band] = (float) (c_r[band] * sin(-alpha + beta)); 
      H12_res[band] = 0;
      H22_res[band] = 0;
    }
  }
}



static void param2UMX_PS(spatialDec* self,
                         float H11[MAX_PARAMETER_BANDS],
                         float H12[MAX_PARAMETER_BANDS],
                         float H21[MAX_PARAMETER_BANDS],
                         float H22[MAX_PARAMETER_BANDS],
                         float H12_res[MAX_PARAMETER_BANDS],
                         float H22_res[MAX_PARAMETER_BANDS],
                         float c_l[MAX_PARAMETER_BANDS],
                         float c_r[MAX_PARAMETER_BANDS],
                         int ottBoxIndx,
                         int parameterSetIndx,
                         int resBands)
{
  int band;

  param2UMX_PS_Core(self->ottCLD[ottBoxIndx][parameterSetIndx],
                    self->ottICC[ottBoxIndx][parameterSetIndx],
                    self->numOttBands[ottBoxIndx],
                    resBands, H11, H12, H21, H22, H12_res, H22_res, c_l, c_r);

  for(band = self->numOttBands[ottBoxIndx]; band < self->numParameterBands; band++){
    H11[band] = H21[band] = H12[band] = H22[band] =  H12_res[band] = H22_res[band] = 0;
  }
}


static void calculateOpd(spatialDec* self,
                         int ottBoxIndx,
                         int parameterSetIndx,
                         float opd[MAX_PARAMETER_BANDS])
{
  int band;

  for (band = 0; band < self->numOttBandsIPD; band++) {
    float cld   = self->ottCLD[ottBoxIndx][parameterSetIndx][band];
    float ipd   = self->ottIPD[ottBoxIndx][parameterSetIndx][band];
    float icc   = self->ottICC[ottBoxIndx][parameterSetIndx][band];
    int ipd_idx = self->bsFrame[self->curFrameBs].ottIPDidx[ottBoxIndx][parameterSetIndx][band];

    /* ipd_idx = 8 => ipd = Pi */
    if ((cld == 0.0f) && ((ipd_idx & 0xf) == 8)) {
      opd[band] = 0.0f;
    }
    else {
      float iidLin = (float) pow(10.0f, cld / 20.0f);
      float iidLin2 = iidLin * iidLin;
      float iidLin21 = iidLin2 + 1.0f;
      float sinIpd = (float) sin(ipd);
      float cosIpd = (float) cos(ipd);
      float temp1 = 2.0f * icc * iidLin;
      float temp1c = temp1 * cosIpd;
      float ratio = (iidLin21 + temp1c) / (iidLin21 + temp1) + ABS_THR;
      float w2 = (float) pow(ratio, 0.25f);
      float w1 = 2.0f - w2;

      opd[band] = (float) atan2(w2 * sinIpd, w1 * iidLin + w2 * cosIpd);
    }
  }
}


static void param2UMX_PS_IPD_OPD(spatialDec* self,
                                 float H11re[MAX_PARAMETER_BANDS],
                                 float H12re[MAX_PARAMETER_BANDS],
                                 float H21re[MAX_PARAMETER_BANDS],
                                 float H22re[MAX_PARAMETER_BANDS],
                                 float H12_res[MAX_PARAMETER_BANDS],
                                 float H22_res[MAX_PARAMETER_BANDS],
                                 float c_l[MAX_PARAMETER_BANDS],
                                 float c_r[MAX_PARAMETER_BANDS],
                                 int ottBoxIndx,
                                 int parameterSetIndx,
                                 int resBands)
{
  float icc[MAX_PARAMETER_BANDS];
  float opd[MAX_PARAMETER_BANDS];
  int numOttBands = self->numOttBands[ottBoxIndx];
  int numIpdBands = self->numOttBandsIPD;
  int band;

  for (band = 0; band < self->numParameterBands; band++) {
    icc[band] = self->ottICC[ottBoxIndx][parameterSetIndx][band];
  }

  for (band = 0; band < resBands; band++) {
    icc[band] *= (float) cos(self->ottIPD[ottBoxIndx][parameterSetIndx][band]);
  }

  param2UMX_PS_Core(self->ottCLD[ottBoxIndx][parameterSetIndx],
                    icc,
                    numOttBands,
                    resBands, H11re, H12re, H21re, H22re, H12_res, H22_res, c_l, c_r);

  for (band = numOttBands; band < self->numParameterBands; band++) {
    H11re[band] = H21re[band] = H12re[band] = H22re[band] = H12_res[band] = H22_res[band] = 0;
  }

  if (self->bsPhaseMode) {
    calculateOpd(self, ottBoxIndx, parameterSetIndx, opd);

    for (band = 0; band < numIpdBands; band++) {
      float ipd = self->ottIPD[ottBoxIndx][parameterSetIndx][band];

#ifndef PHASE_COD_NOFIX
      self->PhaseLeft[parameterSetIndx][band] = wrapPhase(opd[band]);
      self->PhaseRight[parameterSetIndx][band] = wrapPhase(opd[band] - ipd);
#else
      self->PhaseLeft[parameterSetIndx][band] = opd[band];
      self->PhaseRight[parameterSetIndx][band] = opd[band] - ipd;
#endif

    }
  }
  else {
    numIpdBands = 0;
  }

  for (band = numIpdBands; band < numOttBands; band++) {
    self->PhaseLeft[parameterSetIndx][band] = 0.0f;
    self->PhaseRight[parameterSetIndx][band] = 0.0f;
  }
}


static void param2UMX_Prediction(spatialDec* self,
                                 float H11re[MAX_PARAMETER_BANDS],
                                 float H11im[MAX_PARAMETER_BANDS],
                                 float H12re[MAX_PARAMETER_BANDS],
                                 float H12im[MAX_PARAMETER_BANDS],
                                 float H21re[MAX_PARAMETER_BANDS],
                                 float H21im[MAX_PARAMETER_BANDS],
                                 float H22re[MAX_PARAMETER_BANDS],
                                 float H22im[MAX_PARAMETER_BANDS],
                                 float H12_res[MAX_PARAMETER_BANDS],
                                 float H22_res[MAX_PARAMETER_BANDS],
                                 int ottBoxIndx,
                                 int parameterSetIndx,
                                 int resBands)
{
  static const float maxWeight = 1.2f;
  int band;

  for (band = 0; band < self->numParameterBands; band++) {
    float cld = self->ottCLD[ottBoxIndx][parameterSetIndx][band];
    float icc = self->ottICC[ottBoxIndx][parameterSetIndx][band];
    float ipd = self->ottIPD[ottBoxIndx][parameterSetIndx][band];

    if ((band < self->numOttBandsIPD) && (cld == 0.0f) && (icc == 1.0f) && (fabs(ipd - P_PI) < 0.01f)) {
      float gain = 0.5f / maxWeight;

      if (band < resBands) {
        H11re[band] = gain;
        H11im[band] = 0.0f;
        H21re[band] = gain;
        H21im[band] = 0.0f;

        H12re[band] = 0.0f;
        H12im[band] = 0.0f;
        H22re[band] = 0.0f;
        H22im[band] = 0.0f;

        H12_res[band] = gain;
        H22_res[band] = -gain;
      }
      else {
        H11re[band] = gain;
        H11im[band] = 0.0f;
        H21re[band] = -gain;
        H21im[band] = 0.0f;

        H12re[band] = 0.0f;
        H12im[band] = 0.0f;
        H22re[band] = 0.0f;
        H22im[band] = 0.0f;

        H12_res[band] = 0.0f;
        H22_res[band] = 0.0f;
      }
    }
    else {
      float iidLin = (float) pow(10.0f, cld / 20.0f);
      float iidLin2 = iidLin * iidLin;
      float iidLin21 = iidLin2 + 1.0f;
      float iidIcc2 = iidLin * icc * 2.0f;
      float temp, weight;
      float cosIpd, sinIpd;
      float alphaRe, alphaIm;

      if (band < self->numOttBandsIPD) {
        cosIpd = (float) cos(ipd);
        sinIpd = (float) sin(ipd);
      }
      else {
        cosIpd = 1.0f;
        sinIpd = 0.0f;
      }

      temp    = iidLin21 + iidIcc2 * cosIpd;
      weight  = (float) sqrt(iidLin21 / temp);
      weight  = 0.5f / min(maxWeight, weight);
      alphaRe = (1.0f - iidLin2) / temp;
      alphaIm = -iidIcc2 * sinIpd / temp;

      H11re[band] = weight - alphaRe * weight;
      H11im[band] = -alphaIm * weight;
      H21re[band] = weight + alphaRe * weight;
      H21im[band] = alphaIm * weight;

      if (band < resBands) {
        H12re[band] = 0.0f;
        H12im[band] = 0.0f;
        H22re[band] = 0.0f;
        H22im[band] = 0.0f;

        H12_res[band] = weight;
        H22_res[band] = -weight;
      }
      else {
        float beta = 2.0f * iidLin * (float) sqrt(1.0f - icc * icc) * weight / temp;

        H12re[band] = beta;
        H12im[band] = 0.0f;
        H22re[band] = -beta;
        H22im[band] = 0.0f;

        H12_res[band] = 0.0f;
        H22_res[band] = 0.0f;
      }
    }
  }
}


/* Modify residual weights to get upmix matrix [a, a; a, -a] for ICC = CLD = 0 */
static void modifyResidualWeights(int numParameterBands,                          
                                  float H12_res[MAX_PARAMETER_BANDS],
                                  float H22_res[MAX_PARAMETER_BANDS],
                                  int resBands)
{
  int band;
  for (band = 0; band < resBands; band++) {
    H12_res[band] /= 2.0f;
    H22_res[band] /= 2.0f;    
  }
}


static void getMatrixInversionWeights(float iid_lf_ls,
                                      float iid_rf_rs,
                                      int predictionMode,
                                      float c1,
                                      float c2,
                                      float *weight1,
                                      float *weight2)
{
  float w1  = (float) pow(10.0f, -iid_lf_ls / 20.0f);
  float w2  = (float) pow(10.0f, -iid_rf_rs / 20.0f);

  if (predictionMode == 1) {

    if (fabs(c1) >= 1.0f) {
      c1 = 1.0f;
    }
    else if ((c1 < -0.5f) && (c1 > -1.0f)) {
      c1 = -1.0f - 2.0f * c1;
    }
    else {
      c1 = 1.0f / 3.0f +  2.0f / 3.0f * c1;
    }

    if (fabs(c2) >= 1.0f) {
      c2 = 1.0f;
    }
    else if ((c2 < -0.5f) && (c2 > -1.0f)) {
      c2 = -1.0f - 2.0f * c2;
    }
    else {
      c2 = 1.0f / 3.0f +  2.0f / 3.0f * c2;
    }
  }
  else {
    float c1p, c2p;

    
    c1p = (float) pow(10.0f, c1 / 10.0f);
    c2p = (float) pow(10.0f, c2 / 10.0f);

    
    c1 = (float) sqrt((c1p * c2p) / (c1p * c2p + 2.0f * (c2p + 1)));
    c2 = (float) sqrt( c1p        / (c1p       + 2.0f * (c2p + 1)));
  }

  *weight1 = c1 * w1 / (1.0f + w1);
  *weight2 = c2 * w2 / (1.0f + w2);
}



static void invertMatrix(float weight1,
                         float weight2,
                         float hReal[2][2],
                         float hImag[2][2])
{
  float H11_f_real, H12_f_real, H21_f_real, H22_f_real;
  float H11_f_imag, H12_f_imag, H21_f_imag, H22_f_imag;

  float inv_norm_real, inv_norm_imag, inv_norm;

  float len1, len2;

  len1 = (float) (sqrt(1.0f - 2.0f * weight1 + 2.0f * weight1 * weight1));
  len2 = (float) (sqrt(1.0f - 2.0f * weight2 + 2.0f * weight2 * weight2));

  H11_f_real = (1.0f - weight1) / len1;
  H11_f_imag = weight1 / len1;

  H12_f_real = 0.0f;
  H12_f_imag = (float) (( 1.0f / sqrt(3.0f)) * (weight2 / len2));

  H21_f_real = 0.0f;
  H21_f_imag = (float) ((-1.0f / sqrt(3.0f)) * (weight1 / len1));

  H22_f_real = (1.0f - weight2) / len2;
  H22_f_imag = -weight2 / len2;


  inv_norm_real = (H11_f_real*H22_f_real - H11_f_imag*H22_f_imag) - (H12_f_real*H21_f_real - H12_f_imag*H21_f_imag);
  inv_norm_imag = (H11_f_real*H22_f_imag + H11_f_imag*H22_f_real) - (H12_f_real*H21_f_imag + H12_f_imag*H21_f_real);
  inv_norm      = inv_norm_real*inv_norm_real + inv_norm_imag*inv_norm_imag;

  inv_norm_real =  inv_norm_real / inv_norm;
  inv_norm_imag = -inv_norm_imag / inv_norm;


  hReal[0][0] =  (H22_f_real*inv_norm_real - H22_f_imag*inv_norm_imag);
  hImag[0][0] =  (H22_f_real*inv_norm_imag + H22_f_imag*inv_norm_real);

  hReal[0][1] = -(H12_f_real*inv_norm_real - H12_f_imag*inv_norm_imag);
  hImag[0][1] = -(H12_f_real*inv_norm_imag + H12_f_imag*inv_norm_real);

  hReal[1][0] = -(H21_f_real*inv_norm_real - H21_f_imag*inv_norm_imag);
  hImag[1][0] = -(H21_f_real*inv_norm_imag + H21_f_imag*inv_norm_real);

  hReal[1][1] =  (H11_f_real*inv_norm_real - H11_f_imag*inv_norm_imag);
  hImag[1][1] =  (H11_f_real*inv_norm_imag + H11_f_imag*inv_norm_real);
}

#ifdef PARTIALLY_COMPLEX


static void invertMatrixReal(float weight1,
                             float weight2,
                             float h[2][2])
{
  float q1;
  float q2;
  float p;
  float b;
  float r;
  float g11;
  float g12;
  float g21;
  float g22;
  float c;
  float w12 = weight1 * weight1;
  float w22 = weight2 * weight2;

  q1 = w12 / (1.0f - 2.0f * weight1 + 2.0f * w12);
  q2 = w22 / (1.0f - 2.0f * weight2 + 2.0f * w22);

  p = q1 * q2;
  b = 1.0f - 5.0f / 9.0f * p - (float) sqrt(-11.0f / 9.0f * p * p + (4.0f * (q1 + q2) - 14.0f) * p + 9.0f) / 3.0f;

  if ((q1 > 0.0f) && (q1 < 1.0f)) {
    r = 3.0f * b / (2.0f * (q1 - q1 * q1));
  }
  else {
    r = (q2 - q2 * q2) / (3.0f * (1.0f - 5.0f / 9.0f * p));
  }

  g11 = 1.0f - q1 * r / 3.0f;
  g12 = (q2 - q1 * r) / (float) sqrt(3.0f);

  c = (float) sqrt(g11 * g11 * (1.0f - q1) + q1 * (1.0f - q2 / 3.0f) * (1.0f - q2 / 3.0f));

  h[0][0] = g11 / c;
  h[0][1] = g12 / c;

  if ((q2 > 0.0f) && (q2 < 1.0f)) {
    r = 3.0f * b / (2.0f * (q2 - q2 * q2));
  }
  else {
    r = (q1 - q1 * q1) / (3.0f * (1.0f - 5.0f / 9.0f * p));
  }

  g22 = 1.0f - q2 * r / 3.0f;
  g21 = (q1 - q2 * r) / (float) sqrt(3.0f);

  c = (float) sqrt(g22 * g22 * (1.0f - q2) + q2 * (1.0f - q1 / 3.0f) * (1.0f - q1 / 3.0f));

  h[1][0] = g21 / c;
  h[1][1] = g22 / c;
}

#endif


void SpatialDecApplyM0(spatialDec* self) {

  int ts, qs, row, col;
  int pb;
  int nChIn = self->numInputChannels;
  int offset;

  float Rout_kernel[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
  float Rout       [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  interp_UMX(self->M0arbdmx, Rout, self->M0arbdmxPrev, nChIn, 2 * nChIn, self);

  applyAbsKernels(Rout, Rout_kernel, nChIn, 2 * nChIn, self);

#ifdef PARTIALLY_COMPLEX
  AliasLock( Rout_kernel, 0, self );
#endif

  
  for (ts = 0; ts < self->timeSlots; ts++) {
    for (qs = 0; qs < self->hybridBands; qs++) {
      for (row = 0; row < nChIn; row++) {
        float real = 0.0f;
        float imag = 0.0f;
        for (col = 0; col < nChIn; col++) {
          real += self->xReal[row][ts][qs] * Rout_kernel[ts][qs][row][col];
          imag += self->xImag[row][ts][qs] * Rout_kernel[ts][qs][row][col];
        }
        self->xReal[row][ts][qs] = real;
        self->xImag[row][ts][qs] = imag;
      }
    }
  }

  if (self->arbitraryDownmix == 2) {
    offset = self->numOttBoxes + self->numTttBoxes;
    for (ts = 0; ts < self->timeSlots; ts++ ) {
      for (qs = 0; qs < self->hybridBands; qs++ ) {
        pb = self->kernels[qs][0];
        if (pb < self->arbdmxResidualBands) {
          for (row = 0; row < nChIn; row++ ) {
            for (col = nChIn; col < 2 * nChIn; col++ ) {
              self->xReal[row][ts][qs] += Rout_kernel[ts][qs][row][col] * self->hybResidualReal[offset + row][ts][qs];
              self->xImag[row][ts][qs] += Rout_kernel[ts][qs][row][col] * self->hybResidualImag[offset + row][ts][qs];
            }
          }
        }
      }
    }
  }
}

static float Rout_kernelReal [MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float Rout_kernelImag [MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutReal        [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutImag        [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];


void SpatialDecApplyM1(spatialDec* self) {

  int ts,qs, row, col;

  
  
#ifdef PARTIALLY_COMPLEX
  if (((self->treeConfig == TREE_5151) || (self->treeConfig == TREE_5152))  
      && (self->arbitraryDownmix != 2))                                 
    {
      interp_UMX_IMTX(self->M1paramReal, RoutReal, self->numVChannels, self->numXChannels, self);
      interp_UMX_IMTX(self->M1paramImag, RoutImag, self->numVChannels, self->numXChannels, self);
    }
  else
    {
      interp_UMX(self->M1paramReal, RoutReal, self->M1paramRealPrev, self->numVChannels, self->numXChannels, self);
      interp_UMX(self->M1paramImag, RoutImag, self->M1paramImagPrev, self->numVChannels, self->numXChannels, self);
    }
#else
  interp_UMX(self->M1paramReal, RoutReal, self->M1paramRealPrev, self->numVChannels, self->numXChannels, self);
  interp_UMX(self->M1paramImag, RoutImag, self->M1paramImagPrev, self->numVChannels, self->numXChannels, self);
#endif

  applyAbsKernels(RoutReal, Rout_kernelReal, self->numVChannels, self->numXChannels, self);
  applyAbsKernels(RoutImag, Rout_kernelImag, self->numVChannels, self->numXChannels, self);

#ifdef PARTIALLY_COMPLEX
  AliasLock( Rout_kernelReal, 1, self );
  AliasLock( Rout_kernelImag, 2, self );
#endif

  
  for (ts=0; ts< self->timeSlots; ts++) {
    for (qs=0; qs< self->hybridBands; qs++) {
      for(row=0; row < self->numVChannels; row++){
        self->vReal[row][ts][qs] = 0;
        self->vImag[row][ts][qs] = 0;
        for (col=0; col < self->numXChannels; col++){
          int sign = self->kernels[qs][1];
          float real = self->xReal[col][ts][qs]*Rout_kernelReal[ts][qs][row][col]      - self->xImag[col][ts][qs]*Rout_kernelImag[ts][qs][row][col]*sign;
          float imag = self->xReal[col][ts][qs]*Rout_kernelImag[ts][qs][row][col]*sign + self->xImag[col][ts][qs]*Rout_kernelReal[ts][qs][row][col];
          self->vReal[row][ts][qs] +=  real;
          self->vImag[row][ts][qs] +=  imag;

        }
      }
    }
  }
}

static float RoutReal          [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutImag          [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutWetReal       [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutWetImag       [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
static float Rout_kernelReal   [MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float Rout_kernelImag   [MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutWet_kernelReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutWet_kernelImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT];
static float RoutPhaseReal[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2];
static float RoutPhaseImag[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2];

void SpatialDecApplyM2(spatialDec* self) {

  int ts,qs, row, col;
  int complexM2 = ((self->upmixType == 2) || (self->bsConfig.bsPhaseCoding != 0));
  int interpPhase = (self->bsConfig.bsPhaseCoding == 1);
  
  interp_UMX(self->M2decorReal, RoutWetReal, self->M2decorRealPrev, self->numOutputChannels, self->numWChannels, self); 
  interp_UMX(self->M2residReal, RoutReal,    self->M2residRealPrev, self->numOutputChannels, self->numWChannels, self); 

  if (complexM2 && !interpPhase) {
    interp_UMX(self->M2decorImag, RoutWetImag, self->M2decorImagPrev, self->numOutputChannels, self->numWChannels, self);
    interp_UMX(self->M2residImag, RoutImag,    self->M2residImagPrev, self->numOutputChannels, self->numWChannels, self);
  }

  if (interpPhase) {
    interp_phase(self->PhaseLeft, self->PhaseRight, self->PhasePrevLeft, self->PhasePrevRight, RoutPhaseReal, RoutPhaseImag, self);
    
    for (ts = 0; ts < self->timeSlots; ts++) {
      int pb;
      for (pb = 0; pb < self->numParameterBands; pb++) {
        for (row = 0; row < self->numOutputChannels; row++) {
          for (col = 0; col < self->numWChannels; col++) {

            RoutImag[ts][pb][row][col] = RoutReal[ts][pb][row][col] * RoutPhaseImag[ts][pb][row];
            RoutReal[ts][pb][row][col] = RoutReal[ts][pb][row][col] * RoutPhaseReal[ts][pb][row];

            RoutWetImag[ts][pb][row][col] = RoutWetReal[ts][pb][row][col] * RoutPhaseImag[ts][pb][row];
            RoutWetReal[ts][pb][row][col] = RoutWetReal[ts][pb][row][col] * RoutPhaseReal[ts][pb][row];
          }
        }
      }
    }
  }

  applyAbsKernels(RoutReal,    Rout_kernelReal,    self->numOutputChannels, self->numWChannels, self);  
  applyAbsKernels(RoutWetReal, RoutWet_kernelReal, self->numOutputChannels, self->numWChannels, self);  

  if (complexM2) {
    applyAbsKernels(RoutImag,    Rout_kernelImag,    self->numOutputChannels, self->numWChannels, self);
    applyAbsKernels(RoutWetImag, RoutWet_kernelImag, self->numOutputChannels, self->numWChannels, self);
  }

#ifdef PARTIALLY_COMPLEX
  AliasLock( Rout_kernelReal   , 3, self );
  AliasLock( RoutWet_kernelReal, 4, self );

  if (self->upmixType == 2) {
    AliasLock( Rout_kernelImag   , 5, self );
    AliasLock( RoutWet_kernelImag, 6, self );
  }
#endif

  
  for (ts = 0; ts < self->timeSlots; ts++) {
    for (qs = 0; qs < self->hybridBands; qs++) {
      for (row = 0; row < self->numOutputChannels; row++) {
        self->hybOutputRealDry[row][ts][qs] = 0;
        self->hybOutputImagDry[row][ts][qs] = 0;
        self->hybOutputRealWet[row][ts][qs] = 0;
        self->hybOutputImagWet[row][ts][qs] = 0;
      }
    }
  }

    
  if (self->upmixType == 2) {
    if (self->binauralQuality == 1) {
      SpatialDecApplyHrtfFilterbank(self->hrtfFilter,
                                    self->wDryReal,
                                    self->wDryImag,
                                    self->hybOutputRealDry,
                                    self->hybOutputImagDry,
                                    self->arbitraryDownmix == 2);
    }
  }


  
  for (ts = 0; ts < self->timeSlots; ts++) {
    for (qs = 0; qs < self->hybridBands; qs++) {
      for (row = 0; row < self->numOutputChannels; row++) {
        for (col = 0; col < self->numWChannels; col++) {
          self->hybOutputRealDry[row][ts][qs] += self->wDryReal[col][ts][qs] * Rout_kernelReal[ts][qs][row][col];
          self->hybOutputImagDry[row][ts][qs] += self->wDryImag[col][ts][qs] * Rout_kernelReal[ts][qs][row][col];
          self->hybOutputRealWet[row][ts][qs] += self->wWetReal[col][ts][qs] * RoutWet_kernelReal[ts][qs][row][col];
          self->hybOutputImagWet[row][ts][qs] += self->wWetImag[col][ts][qs] * RoutWet_kernelReal[ts][qs][row][col];
        }
      }
    }
  }

  if (complexM2) {
    for (ts = 0; ts < self->timeSlots; ts++) {
      for (qs = 0; qs < self->hybridBands; qs++) {
        int sign = self->kernels[qs][1];

        for (row = 0; row < self->numOutputChannels; row++) {
          for (col = 0; col < self->numWChannels; col++) {
            self->hybOutputRealDry[row][ts][qs] -= self->wDryImag[col][ts][qs] * sign * Rout_kernelImag[ts][qs][row][col];
            self->hybOutputImagDry[row][ts][qs] += self->wDryReal[col][ts][qs] * sign * Rout_kernelImag[ts][qs][row][col];
            self->hybOutputRealWet[row][ts][qs] -= self->wWetImag[col][ts][qs] * sign * RoutWet_kernelImag[ts][qs][row][col];
            self->hybOutputImagWet[row][ts][qs] += self->wWetReal[col][ts][qs] * sign * RoutWet_kernelImag[ts][qs][row][col];
          }
        }
      }
    }
  }
}



static void interp_UMX(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       float Mprev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                       int nRows,
                       int nCols,
                       spatialDec* self)
{

  int ts, ps, pb, row, col;

  for(pb = 0; pb < self->numParameterBands; pb++){
    for(row = 0; row < nRows; row++){
      for(col = 0; col < nCols; col++){


        float prevSlot = (float) -1;
        float currSlot = (float) self->paramSlot[0];
        ps = 0;
        for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
          float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));

          R[ts][pb][row][col] = Mprev[pb][row][col]*(1-alpha) + alpha*M[ps][pb][row][col];
        }


        for(ps = 1; ps < self->numParameterSets; ps++){
          float prevSlot = (float) self->paramSlot[ps-1];
          float currSlot = (float) self->paramSlot[ps];

          for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
            float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));

            R[ts][pb][row][col] = M[ps-1][pb][row][col]*(1-alpha) + alpha*M[ps][pb][row][col];
          }
        }
      }
    }
  }
}

static void interp_UMX_IPD(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][2],
                       float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][2],
                       float Mprev[MAX_PARAMETER_BANDS][2],
                       int nCols,
                       spatialDec* self)
{

  int ts, ps, pb, col;

  for(pb = 0; pb < self->numParameterBands; pb++){
      for(col = 0; col < nCols; col++){
		  
		  
		  float prevSlot = (float) -1;
		  float currSlot = (float) self->paramSlot[0];
		  ps = 0;
		  for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
			  float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));
			  
			  R[ts][pb][col] = Mprev[pb][col]*(1-alpha) + alpha*M[ps][pb][col];
		  }
		  
		  
		  for(ps = 1; ps < self->numParameterSets; ps++){
			  float prevSlot = (float) self->paramSlot[ps-1];
			  float currSlot = (float) self->paramSlot[ps];
			  
			  for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
				  float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));
				  
				  R[ts][pb][col] = M[ps-1][pb][col]*(1-alpha) + alpha*M[ps][pb][col];
			  }
		  }
      }
  }
}

static float interp_angle(float angle1, float angle2, float alpha)
{

#ifndef PHASE_COD_NOFIX
  while (angle2 - angle1 > (float)P_PI) angle1 = angle1 + 2.0f * (float)P_PI;
  while (angle1 - angle2 > (float)P_PI) angle2 = angle2 + 2.0f * (float)P_PI;
#else  
  while (angle2 - angle1 > P_PI) angle2 -= 2*P_PI;
  while (angle1 - angle2 > P_PI) angle1 -= 2*P_PI;
#endif

  return (1-alpha)*angle1 + alpha*angle2;
}

static void interp_phase(float pl[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
                         float pr[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS],
                         float plPrev[MAX_PARAMETER_BANDS],
                         float prPrev[MAX_PARAMETER_BANDS],
                         float Rreal[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                         float Rimag[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                         spatialDec* self)
{
  int ts, ps, pb;

  for(pb = 0; pb < self->numParameterBands; pb++){
    float prevSlot = (float) -1;
		float currSlot = (float) self->paramSlot[0];
		ps = 0;
		for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
			float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));
      float t;
			
			t = interp_angle(plPrev[pb], pl[ps][pb], alpha);
      Rreal[ts][pb][0] = (float)cos(t);
      Rimag[ts][pb][0] = (float)sin(t);
			
			t = interp_angle(prPrev[pb], pr[ps][pb], alpha);
      Rreal[ts][pb][1] = (float)cos(t);
      Rimag[ts][pb][1] = (float)sin(t);
		}
		
		
		for(ps = 1; ps < self->numParameterSets; ps++){
			float prevSlot = (float) self->paramSlot[ps-1];
			float currSlot = (float) self->paramSlot[ps];
			
			for (ts = (int) (prevSlot) + 1; ts <= (int) (currSlot); ts++){
				float alpha = (float) (ts  / (currSlot - prevSlot) - prevSlot / (currSlot - prevSlot));
        float t;
				
				t = interp_angle(pl[ps-1][pb], pl[ps][pb], alpha);
        Rreal[ts][pb][0] = (float)cos(t);
        Rimag[ts][pb][0] = (float)sin(t);
			
				t = interp_angle(pr[ps-1][pb], pr[ps][pb], alpha);
        Rreal[ts][pb][1] = (float)cos(t);
        Rimag[ts][pb][1] = (float)sin(t);
			}
		}
  }
}

#ifdef PARTIALLY_COMPLEX

static void interp_UMX_IMTX(float M[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                            float R[MAX_TIME_SLOTS]    [MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                            int nRows,
                            int nCols,
                            spatialDec* self)
{

  int ts, ps, pb, row, col;
  int prevSlot, currSlot;

  for(pb = 0; pb < self->numParameterBands; pb++){
    for(row = 0; row < nRows; row++){
      for(col = 0; col < nCols; col++){
        prevSlot = -1;
        for(ps = 0; ps < self->numParameterSets; ps++){
          currSlot = self->paramSlot[ps];

          for (ts = prevSlot + 1; ts <= currSlot; ts++){
            R[ts][pb][row][col] = M[ps][pb][row][col];
          }
          prevSlot = currSlot;
        } 
      } 
    } 
  } 
}
#endif


static void applyAbsKernels( float Rin [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                             float Rout[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [MAX_M_OUTPUT][MAX_M_INPUT],
                             int nRows,
                             int nCols,
                             spatialDec* self )
{

  int ts, qb, row, col;


  for(ts = 0; ts < self->timeSlots; ts++){
    for(qb = 0; qb < self->hybridBands; qb++){
      int indx = self->kernels[qb][0];
      for(row=0; row < nRows; row++){
        for (col=0; col < nCols; col++){
          Rout[ts][qb][row][col] = Rin[ts][indx][row][col];
        }
      }
    }
  }
}

static void applyAbsKernels_IPD( float Rin [MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
                             float Rout[MAX_TIME_SLOTS][MAX_HYBRID_BANDS]   [2],
                             int nCols,
                             spatialDec* self )
{

  int ts, qb, col;


  for(ts = 0; ts < self->timeSlots; ts++){
    for(qb = 0; qb < self->hybridBands; qb++){
      int indx = self->kernels[qb][0];
        for (col=0; col < nCols; col++){
          Rout[ts][qb][col] = Rin[ts][indx][col];
        }
    }
  }
}

void SpatialDecInitM1andM2(spatialDec* self,
                           int hrtfModel)
{
  int i,j,k;

  for (i = 0; i < MAX_PARAMETER_BANDS; i++) {
    for (j = 0; j< MAX_M_OUTPUT; j++) {
      for (k = 0; k < MAX_M_INPUT; k++) {
        self->M0arbdmxPrev[i][j][k] = 0;
        self->M1paramRealPrev[i][j][k] = 0;
        self->M1paramImagPrev[i][j][k] = 0;
        self->M2decorRealPrev[i][j][k] = 0;
        self->M2decorImagPrev[i][j][k] = 0;
        self->M2residRealPrev[i][j][k] = 0;
        self->M2residImagPrev[i][j][k] = 0;
      }
    }
  }

  
  if (self->upmixType == 2) {
    switch (hrtfModel) {
    case 0:
      self->hrtfData = &SpatialDecKemarHRTF;
      break;
    case 1:
      self->hrtfData = &SpatialDecVastHRTF;
      break;
    case 2:
      self->hrtfData = &SpatialDecMPSVTHRTF;
      break;
#ifdef HRTF_DYNAMIC_UPDATE
    case 3:
      self->hrtfData = calloc(sizeof(HRTF_DATA),1);
      break;
#endif
    case 4:
      self->hrtfData = &SpatialDecTestHRTF;
      break;
    default:
      assert(0);
      break;
    }

#ifdef HRTF_DYNAMIC_UPDATE
    assert((self->samplingFreq == self->hrtfData->sampleRate) || (0 == self->hrtfData->sampleRate));
#else 
    assert(self->samplingFreq == self->hrtfData->sampleRate);
#endif 

    if (self->binauralQuality == 1) {
      SpatialDecOpenHrtfFilterbank(&self->hrtfFilter, self->hrtfData, self->surroundGain, self->qmfBands);
    }
  }

}
