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
#include "sac_tonality.h"
#include <math.h>

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

void zoomFFT16(float *inReal, float *inImag, float *outReal, float *outImag, int qmfBand, float dfrac);



void spatialMeasureTonality(spatialDec* self, float *tonality)
{
  float qmfReal[MAX_NUM_QMF_BANDS][MAX_TIME_SLOTS];
  float qmfImag[MAX_NUM_QMF_BANDS][MAX_TIME_SLOTS];

  float spec_zoom_real[MAX_NUM_QMF_BANDS * 8];
  float spec_zoom_imag[MAX_NUM_QMF_BANDS * 8];

  float *spec_prev_real = self->tonState.spec_prev_real;
  float *spec_prev_imag = self->tonState.spec_prev_imag;

  float *p_cross_real = self->tonState.p_cross_real;
  float *p_cross_imag = self->tonState.p_cross_imag;

  float *p_sum = self->tonState.p_sum;
  float *p_sum_prev = self->tonState.p_sum_prev;

  float p_max[MAX_NUM_QMF_BANDS * 8];

  float coh_spec[MAX_NUM_QMF_BANDS * 8];
  float pow_spec[MAX_NUM_QMF_BANDS * 8];

  float part4[4]   = { 2.f, 4.f, 17.f, 41.f };
  float part5[5]   = { 1.f, 2.f, 6.f, 14.f, 41.f };
  float part7[7]   = { 1.f, 1.f, 2.f, 4.f, 6.f, 9.f, 41.f };
  float part10[10] = { .5f, .5f, 1.f, 1.f, 2.f, 2.f, 2.f, 5.f, 9.f, 41.f };
  float part14[14] = { .5f, .5f, .5f, .5f, 1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 12.f, 29.f };
  float part20[20] = { .25f, .25f, .25f, .25, .5f, .5f, .5f, .5f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 3.f, 4.f, 5.f, 12.f, 29.f };
  float part28[28] = { .25f,.25f, .25f, .25f, .25f, .25f, .25f, .25f,  .5f, .5f, .5f, .5f, 1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 6.f, 23.f };
  float part40[40] = { .125f, .125f, .125f, .125f, .125f, .125f, .125f, .125f, .25f, .25f, .25f, .25f, .25f, .25f, .25f, .25f, .5f, .5f, .5f, .5f, .5f, .5f, .5f, .5f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 6.f, 7.f, 16.f };

  int g, gmax;
  int i, j, q, s, c;

  float *part;
  int pstart, pstop;
  float pqmf;
  float num, den;
  float tmp_ton;

  float beta;
  float dwin, dfrac;
  int nstart;

  float tmpReal, tmpImag;


  switch(self->numParameterBands) {
  case 4:
    part = part4;
    break;
  case 5:
    part = part5;
    break;
  case 7:
    part = part7;
    break;
  case 10:
    part = part10;
    break;
  case 14:
    part = part14;
    break;
  case 20:
    part = part20;
    break;
  case 28:
    part = part28;
    break;
  case 40:
    part = part40;
    break;
  default:
    fprintf(stderr, "\nIllegal number of parameter bands in sac_tonality.c!\n");
    exit(1);
  }

  
  for (q = 0; q < self->qmfBands; q++) {
    for (s = 0; s < self->timeSlots; s++) {
      tmpReal = 0;
      tmpImag = 0;
      for (c = 0; c < self->numInputChannels; c++) {
        tmpReal += self->qmfInputReal[c][s][q];
        tmpImag += self->qmfInputImag[c][s][q];
      }

      if (s+6 < self->timeSlots) {
        qmfReal[q][s+6] = tmpReal;
        qmfImag[q][s+6] = tmpImag;
      }
      else {
        qmfReal[q][s+6 - self->timeSlots] = self->tonState.bufReal[q][s+6 - self->timeSlots];
        qmfImag[q][s+6 - self->timeSlots] = self->tonState.bufImag[q][s+6 - self->timeSlots];
        self->tonState.bufReal[q][s+6 - self->timeSlots] = tmpReal;
        self->tonState.bufImag[q][s+6 - self->timeSlots] = tmpImag;
      }
    }
  }


  
  gmax = (int) ceil(self->timeSlots / 16.0);
  dwin = (float)(self->timeSlots) / gmax;

  beta = self->qmfBands * dwin / (0.025f * self->samplingFreq);


  
  for (i = 0; i < self->numParameterBands; i++) {
    tonality[i] = 1.0f;
  }


  for (g = 0; g < gmax; g++) {
    nstart = (int) floor((g+1) * dwin + 0.5) - 16;    

    dfrac = (float) floor((g+1) * dwin + 0.5) - ((g+1) * dwin); 

    for (q = 0; q < self->qmfBands; q++) {
      for (i = 0; i < 16; i++) {
        if (nstart + i < 0) {
          self->tonState.winBufReal[q][i] = self->tonState.winBufReal[q][16+nstart+i];
          self->tonState.winBufImag[q][i] = self->tonState.winBufImag[q][16+nstart+i];
        }
        else {
          self->tonState.winBufReal[q][i] = qmfReal[q][nstart+i];
          self->tonState.winBufImag[q][i] = qmfImag[q][nstart+i];
        }
      }
    }

    
    for (q = 0; q < self->qmfBands; q++) {
      zoomFFT16(&(self->tonState.winBufReal[q][0]), &(self->tonState.winBufImag[q][0]), &(spec_zoom_real[q*8]), &(spec_zoom_imag[q*8]), q, dfrac);
    }

    
    for (i = 0; i < 8 * self->qmfBands; i++) {
      p_cross_real[i] = beta * (spec_zoom_real[i] * spec_prev_real[i] + spec_zoom_imag[i] * spec_prev_imag[i]) + (1-beta) * p_cross_real[i];
      p_cross_imag[i] = beta * (spec_zoom_imag[i] * spec_prev_real[i] - spec_zoom_real[i] * spec_prev_imag[i]) + (1-beta) * p_cross_imag[i];

      p_sum[i] = beta * (spec_zoom_real[i] * spec_zoom_real[i] + spec_zoom_imag[i] * spec_zoom_imag[i]) + (1-beta) * p_sum[i];
      p_max[i] = max(p_sum[i], p_sum_prev[i]);
      p_sum_prev[i] = p_sum[i];

      coh_spec[i] = ((float)sqrt(p_cross_real[i] * p_cross_real[i] + p_cross_imag[i] * p_cross_imag[i]) + 1e-20f) / (p_max[i] + 1e-20f);
      pow_spec[i] = (spec_zoom_real[i] * spec_zoom_real[i] + spec_zoom_imag[i] * spec_zoom_imag[i]) + 
        (spec_prev_real[i] * spec_prev_real[i] + spec_prev_imag[i] * spec_prev_imag[i]);

      spec_prev_real[i] = spec_zoom_real[i];
      spec_prev_imag[i] = spec_zoom_imag[i];
    }

    
    pstart = 0;
    pqmf = 0;
    for (i = 0; i < self->numParameterBands; i++) {
      pqmf += part[i];
      pstop = (int)(pqmf * 8 + 0.5);

      num = 0; den = 0;
      for (j = pstart; j < pstop; j++) {
        num += pow_spec[j] * coh_spec[j];
        den += pow_spec[j];
      }

      tmp_ton = (num + 1e-20f) / (den + 1e-20f);
      if (tmp_ton > 1.0f) tmp_ton = 1.0f;

      if (tmp_ton < tonality[i]) tonality[i] = tmp_ton;

      pstart = pstop;
    }
  }
}



void zoomFFT16(float *inReal, float *inImag, float *outReal, float *outImag, int qmfBand, float dfrac)
{
  const float wReal[16] = { 1.000000f, 0.980785f, 0.923880f, 0.831470f, 0.707107f, 0.555570f, 0.382683f, 0.195090f, 
                            0.000000f,-0.195090f,-0.382683f,-0.555570f,-0.707107f,-0.831470f,-0.923880f,-0.980785f };
  const float wImag[16] = { 0.000000f,-0.195090f,-0.382683f,-0.555570f,-0.707107f,-0.831470f,-0.923880f,-0.980785f,
                            -1.000000f,-0.980785f,-0.923880f,-0.831470f,-0.707107f,-0.555570f,-0.382683f,-0.195090f };

  const int bitrev[16] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };

  const double pi = 3.14159265359;

  float blackman[16];

  float vReal[16], vImag[16];
  float tReal, tImag;
  float eReal, eImag;

  int i, j, s1, s2;

  
  for (i = 0; i < 16; i++) {
    blackman[i] = 0.42f - 0.5f*(float)cos(2*pi*(i+dfrac)/15) + 0.08f*(float)cos(4*pi*(i+dfrac)/15);
  }

  
  for (i = 0; i < 16; i++) {
    vReal[bitrev[i]] = (inReal[i] * wReal[i] - inImag[i] * wImag[i]) * blackman[i];
    vImag[bitrev[i]] = (inReal[i] * wImag[i] + inImag[i] * wReal[i]) * blackman[i];
  }

  
  for (s1=1, s2=16; s1 < 8; s1<<=1, s2>>=1) {
    for (i = 0; i < 16; i+=2*s1) {
      for (j = 0; j < s1; j++) {
        tReal = vReal[i+j+s1] * wReal[j*s2] - vImag[i+j+s1] * wImag[j*s2];
        tImag = vReal[i+j+s1] * wImag[j*s2] + vImag[i+j+s1] * wReal[j*s2];
        
        vReal[i+j+s1] = vReal[i+j] - tReal;
        vImag[i+j+s1] = vImag[i+j] - tImag;
        
        vReal[i+j] = vReal[i+j] + tReal;
        vImag[i+j] = vImag[i+j] + tImag;
      }
    }
  }

  
  for (j = 0; j < 8; j++) {
    tReal       = vReal[j+8] * wReal[j*2] - vImag[j+8] * wImag[j*2];
    tImag       = vReal[j+8] * wImag[j*2] + vImag[j+8] * wReal[j*2];

    
    if ((qmfBand % 2) == 0) {
      outReal[j]   = vReal[j] + tReal;
      outImag[j]   = vImag[j] + tImag;
    }
    else {
      outReal[j] = vReal[j] - tReal;
      outImag[j] = vImag[j] - tImag;
    }
  }

  
  for (i = 0; i < 8; i++) {
    if ((qmfBand % 2) == 0) {
      eReal = (float)cos(-2*pi/16*i*dfrac);
      eImag = (float)sin(-2*pi/16*i*dfrac);
    }
    else {
      eReal = (float)cos(-2*pi/16*(i-8)*dfrac);
      eImag = (float)sin(-2*pi/16*(i-8)*dfrac);
    }

    tReal      = outReal[i] * eReal - outImag[i] * eImag;
    outImag[i] = outReal[i] * eImag + outImag[i] * eReal;
    outReal[i] = tReal;
  }
}



void initSpatialTonality(spatialDec* self)
{
  int i, j;

  for (i = 0; i < self->qmfBands * 8; i++) {
    self->tonState.spec_prev_real[i] = 0.0f;
    self->tonState.spec_prev_imag[i] = 0.0f;

    self->tonState.p_cross_real[i] = 0.0f;
    self->tonState.p_cross_imag[i] = 0.0f;

    self->tonState.p_sum[i] = 0.0f;
    self->tonState.p_sum_prev[i] = 0.0f;
  }

  for (i = 0; i < self->qmfBands; i++) {
    for (j = 0; j < 6; j++) {
      self->tonState.bufReal[i][j] = 0.0f;
      self->tonState.bufImag[i][j] = 0.0f;
    }

    for (j = 0; j < 16; j++) {
      self->tonState.winBufReal[i][j] = 0.0f;
      self->tonState.winBufImag[i][j] = 0.0f;
    }
  }
}
