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
#include "sac_TPprocess.h"
#include "sac_process.h"
#include "sac_calcM1andM2.h"
#include "sac_sbrconst.h"
#include <math.h>
#include <float.h>
#include <memory.h>


#include <assert.h>

extern void
sbr_dec_from_mps(int channel, int el,
                 float qmfOutputReal[][MAX_NUM_QMF_BANDS],
                 float qmfOutputImag[][MAX_NUM_QMF_BANDS]);

void arbitraryTreeProcess (spatialDec* self);

#ifdef PARTIALLY_COMPLEX
void Exp2Cos ( const float piData[][MAX_NUM_QMF_BANDS], float *prData, int nDim );
#endif


#define BP_SIZE 25
#define HP_SIZE 9

#define STP_LPF_COEFF1 0.950
#define STP_LPF_COEFF2 0.450
#define STP_UPDATE_ENERGY_RATE 32
#define STP_SCALE_LIMIT 2.82

static void subbandTP(spatialDec* self, int ts)
{
  float qmfOutputRealDry[MAX_OUTPUT_CHANNELS][MAX_NUM_QMF_BANDS];
  float qmfOutputImagDry[MAX_OUTPUT_CHANNELS][MAX_NUM_QMF_BANDS];
  float qmfOutputRealWet[MAX_OUTPUT_CHANNELS][MAX_NUM_QMF_BANDS];
  float qmfOutputImagWet[MAX_OUTPUT_CHANNELS][MAX_NUM_QMF_BANDS];
  float dmxReal[MAX_INPUT_CHANNELS][BP_SIZE];
  float dmxImag[MAX_INPUT_CHANNELS][BP_SIZE];
  float DryEner[MAX_INPUT_CHANNELS];
  float WetEner[MAX_OUTPUT_CHANNELS];
  float scale[MAX_OUTPUT_CHANNELS];
  float temp;
  float damp;
  
  static float BP[BP_SIZE] = {0.0000f,0.0005f,0.0092f,0.0587f,0.2580f,0.7392f,0.9791f,0.9993f,1.0000f,1.0000f,
                              1.0000f,1.0000f,0.9999f,0.9984f,0.9908f,0.9639f,0.8952f,0.7711f,0.6127f,0.4609f,
                              0.3391f,0.2493f,0.1848f,0.1387f,0.1053f};


  static float GF[BP_SIZE] = {0.f,0.f,0.f,0.f,0.f,0.f,1e-008f,8.1e-007f,3.61e-006f,8.41e-006f,1.6e-005f,2.704e-005f,
                              3.969e-005f,5.625e-005f,7.396e-005f,9.801e-005f,0.00012321f,0.00015625f,
                              0.00019881f,0.00024964f,0.00032041f,0.00041209f,0.00053824f,0.00070756f,0.00094249f};
 
  static float BP2[BP_SIZE];
  static float runDryEner[MAX_INPUT_CHANNELS ] = {0};
  static float runWetEner[MAX_OUTPUT_CHANNELS] = {0};
  static float oldDryEner[MAX_INPUT_CHANNELS ] = {0};
  static float oldWetEner[MAX_OUTPUT_CHANNELS] = {0};
  static float prev_tp_scale[MAX_OUTPUT_CHANNELS];

  int ch, n, no_scaling, i;
  int i_LF, i_RF, i_C, i_LFE, i_LS, i_RS, i_AL, i_AR;

  static int init = 0;
  static int update_old_ener = 0;

  if (init == 0) {
    for (ch=0; ch<MAX_OUTPUT_CHANNELS; ch++) {
      prev_tp_scale[ch]=1.0f;
      oldWetEner[ch]=32768*32768;
    }
    for (ch=0; ch<MAX_INPUT_CHANNELS; ch++) {
      oldDryEner[ch]=32768*32768;
    }
    for (n=0; n<BP_SIZE; n++) {
      BP2[n] = BP[n]*BP[n];
    }

    init = 1;
  }

  
  if (update_old_ener == STP_UPDATE_ENERGY_RATE) {
    update_old_ener = 1;
    for (ch=0; ch<self->numInputChannels; ch++)
      oldDryEner[ch] = runDryEner[ch];
    for (ch=0; ch<self->numOutputChannels; ch++)
      oldWetEner[ch] = runWetEner[ch];
  }
  else
    update_old_ener++;

  for (ch=0; ch<MAX_OUTPUT_CHANNELS; ch++)
    scale[ch] = 1.0;

  switch (self->treeConfig) {
  case 0: 
    i_LF=0; i_RF=1; i_C=2; i_LFE=3; i_LS=4; i_RS=5; break;
  case 1: 
  case 2: 
    i_LF=0; i_RF=2; i_C=4; i_LFE=5; i_LS=1; i_RS=3; break;
  case 3:   
  case 4:   
  case 5:   
  case 6:   
  i_LF=0; i_RF=3; i_C=6; i_LFE=7; i_LS=2; i_RS=5; i_AL=1; i_AR=4; break;
  case 7: 
    i_LF=0; i_RF=1; break;
  default:
    printf("\n  treeConfig=%d is unsupported by Subband TP",self->treeConfig);
    exit(0);
  }

    
  for (ch=0; ch<self->numOutputChannels; ch++) {

    qmfOutputRealDry[ch][0] = self->hybOutputRealDry[ch][ts][0];
    qmfOutputImagDry[ch][0] = self->hybOutputImagDry[ch][ts][0];
    qmfOutputRealWet[ch][0] = self->hybOutputRealWet[ch][ts][0];
    qmfOutputImagWet[ch][0] = self->hybOutputImagWet[ch][ts][0];

    for (i=1; i<6; i++)
    {
      qmfOutputRealDry[ch][0] += self->hybOutputRealDry[ch][ts][i];
      qmfOutputImagDry[ch][0] += self->hybOutputImagDry[ch][ts][i];
      qmfOutputRealWet[ch][0] += self->hybOutputRealWet[ch][ts][i];
      qmfOutputImagWet[ch][0] += self->hybOutputImagWet[ch][ts][i];
    }

    qmfOutputRealDry[ch][1] = self->hybOutputRealDry[ch][ts][6] + self->hybOutputRealDry[ch][ts][7];
    qmfOutputImagDry[ch][1] = self->hybOutputImagDry[ch][ts][6] + self->hybOutputImagDry[ch][ts][7];
    qmfOutputRealWet[ch][1] = self->hybOutputRealWet[ch][ts][6] + self->hybOutputRealWet[ch][ts][7];
    qmfOutputImagWet[ch][1] = self->hybOutputImagWet[ch][ts][6] + self->hybOutputImagWet[ch][ts][7];

    qmfOutputRealDry[ch][2] = self->hybOutputRealDry[ch][ts][8] + self->hybOutputRealDry[ch][ts][9];
    qmfOutputImagDry[ch][2] = self->hybOutputImagDry[ch][ts][8] + self->hybOutputImagDry[ch][ts][9];
    qmfOutputRealWet[ch][2] = self->hybOutputRealWet[ch][ts][8] + self->hybOutputRealWet[ch][ts][9];
    qmfOutputImagWet[ch][2] = self->hybOutputImagWet[ch][ts][8] + self->hybOutputImagWet[ch][ts][9];

    for (i=3; i<self->qmfBands; i++)
    {
      qmfOutputRealDry[ch][i] = self->hybOutputRealDry[ch][ts][i-3+10];
      qmfOutputImagDry[ch][i] = self->hybOutputImagDry[ch][ts][i-3+10];
      qmfOutputRealWet[ch][i] = self->hybOutputRealWet[ch][ts][i-3+10];
      qmfOutputImagWet[ch][i] = self->hybOutputImagWet[ch][ts][i-3+10];
    }
  }

  
  for (n=1; n<BP_SIZE; n++) {
  switch (self->treeConfig) {
  case 0:   
  case 1:   
      dmxReal[0][n]  = qmfOutputRealDry[i_LF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_RF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_C ][n];
      dmxReal[0][n] += qmfOutputRealDry[i_LS][n];
      dmxReal[0][n] += qmfOutputRealDry[i_RS][n];
      dmxReal[0][n] *= BP[n]*GF[n];
    break;
  case 2:   
      dmxReal[0][n]  = qmfOutputRealDry[i_LF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_LS][n];
      dmxReal[0][n] *= BP[n]*GF[n];

      dmxReal[1][n]  = qmfOutputRealDry[i_RF][n];
      dmxReal[1][n] += qmfOutputRealDry[i_RS][n];
      dmxReal[1][n] *= BP[n]*GF[n];
    break;
  case 3:   
  case 4:   
      dmxReal[0][n]  = qmfOutputRealDry[i_LF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_LS][n];
      dmxReal[0][n] += qmfOutputRealDry[i_AL][n];
      dmxReal[0][n] *= BP[n]*GF[n];

      dmxReal[1][n]  = qmfOutputRealDry[i_RF][n];
      dmxReal[1][n] += qmfOutputRealDry[i_RS][n];
      dmxReal[1][n] += qmfOutputRealDry[i_AR][n];
      dmxReal[1][n] *= BP[n]*GF[n];
    break;
  case 5:   
      dmxReal[0][n]  = qmfOutputRealDry[i_LF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_AL][n];
      dmxReal[0][n] *= BP[n]*GF[n];

      dmxReal[1][n]  = qmfOutputRealDry[i_RF][n];
      dmxReal[1][n] += qmfOutputRealDry[i_AR][n];
      dmxReal[1][n] *= BP[n]*GF[n];
    break;
  case 6:   
      dmxReal[0][n]  = qmfOutputRealDry[i_LS][n];
      dmxReal[0][n] += qmfOutputRealDry[i_AL][n];
      dmxReal[0][n] *= BP[n]*GF[n];

      dmxReal[1][n]  = qmfOutputRealDry[i_RS][n];
      dmxReal[1][n] += qmfOutputRealDry[i_AR][n];
      dmxReal[1][n] *= BP[n]*GF[n];
      break;
  case 7:
      dmxReal[0][n]  = qmfOutputRealDry[i_LF][n];
      dmxReal[0][n] += qmfOutputRealDry[i_RF][n];
      dmxReal[0][n] *= BP[n]*GF[n];
      break;
  default:
    ;
    }
  }

#ifdef PARTIALLY_COMPLEX
  for (n=1; n<PC_NUM_BANDS; n++) {
#else
  for (n=1; n<BP_SIZE; n++) {
#endif
  switch (self->treeConfig) {
  case 0:   
  case 1:   
      dmxImag[0][n]  = qmfOutputImagDry[i_LF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_RF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_C ][n];
      dmxImag[0][n] += qmfOutputImagDry[i_LS][n];
      dmxImag[0][n] += qmfOutputImagDry[i_RS][n];
      dmxImag[0][n] *= BP[n]*GF[n];
    break;
  case 2:   
      dmxImag[0][n]  = qmfOutputImagDry[i_LF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_LS][n];
      dmxImag[0][n] *= BP[n]*GF[n];

      dmxImag[1][n]  = qmfOutputImagDry[i_RF][n];
      dmxImag[1][n] += qmfOutputImagDry[i_RS][n];
      dmxImag[1][n] *= BP[n]*GF[n];
    break;
  case 3:   
  case 4:   
      dmxImag[0][n]  = qmfOutputImagDry[i_LF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_LS][n];
      dmxImag[0][n] += qmfOutputImagDry[i_AL][n];
      dmxImag[0][n] *= BP[n]*GF[n];

      dmxImag[1][n]  = qmfOutputImagDry[i_RF][n];
      dmxImag[1][n] += qmfOutputImagDry[i_RS][n];
      dmxImag[1][n] += qmfOutputImagDry[i_AR][n];
      dmxImag[1][n] *= BP[n]*GF[n];
    break;
  case 5:   
      dmxImag[0][n]  = qmfOutputImagDry[i_LF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_AL][n];
      dmxImag[0][n] *= BP[n]*GF[n];

      dmxImag[1][n]  = qmfOutputImagDry[i_RF][n];
      dmxImag[1][n] += qmfOutputImagDry[i_AR][n];
      dmxImag[1][n] *= BP[n]*GF[n];
    break;
  case 6:   
      dmxImag[0][n]  = qmfOutputImagDry[i_LS][n];
      dmxImag[0][n] += qmfOutputImagDry[i_AL][n];
      dmxImag[0][n] *= BP[n]*GF[n];

      dmxImag[1][n]  = qmfOutputImagDry[i_RS][n];
      dmxImag[1][n] += qmfOutputImagDry[i_AR][n];
      dmxImag[1][n] *= BP[n]*GF[n];
      break;
  case 7:
      dmxImag[0][n]  = qmfOutputImagDry[i_LF][n];
      dmxImag[0][n] += qmfOutputImagDry[i_RF][n];
      dmxImag[0][n] *= BP[n]*GF[n];
      break;
  default:
    ;
    }
  }


  
  for (ch=0; ch<self->numInputChannels; ch++) {

    DryEner[ch]=0;

#ifdef PARTIALLY_COMPLEX
    for (n=1; n<PC_NUM_BANDS; n++) {
      DryEner[ch] += (dmxReal[ch][n]*dmxReal[ch][n]);
      DryEner[ch] += (dmxImag[ch][n]*dmxImag[ch][n]);
    }
    DryEner[ch] *= 0.5;
    for (n=PC_NUM_BANDS; n<BP_SIZE; n++) {
      DryEner[ch] += (dmxReal[ch][n]*dmxReal[ch][n]);
    }
#else
    for (n=1; n<BP_SIZE; n++) {
      DryEner[ch] += (dmxReal[ch][n]*dmxReal[ch][n]);
      DryEner[ch] += (dmxImag[ch][n]*dmxImag[ch][n]);
    }
#endif

    runDryEner[ch] = (float)(STP_LPF_COEFF1*runDryEner[ch] + (1.0-STP_LPF_COEFF1)*DryEner[ch]);
    DryEner[ch] /= (oldDryEner[ch]+ABS_THR);
  }

  
  for (ch=0; ch<self->numOutputChannels; ch++) {

    if ((self->treeConfig != 7) && (ch == i_LFE)) continue; 
    if ((self->treeConfig == 2) && (ch == i_C)) continue; 
    if ((self->treeConfig == 5) && ((ch == i_LS) || (ch == i_RS))) continue;  
    if ((self->treeConfig == 6) && ((ch == i_LF) || (ch == i_RF))) continue;  

    WetEner[ch] = 0;

#ifdef PARTIALLY_COMPLEX
    for (n=1; n<PC_NUM_BANDS; n++) {
      temp  = (qmfOutputRealWet[ch][n]*qmfOutputRealWet[ch][n]);
      temp += (qmfOutputImagWet[ch][n]*qmfOutputImagWet[ch][n]);
      temp *= BP2[n]*GF[n]*GF[n];
      WetEner[ch] += temp;
    }
    WetEner[ch] *= 0.5;
    for (n=PC_NUM_BANDS; n<BP_SIZE; n++) {
      temp  = (qmfOutputRealWet[ch][n]*qmfOutputRealWet[ch][n]);
      temp *= BP2[n]*GF[n]*GF[n];
      WetEner[ch] += temp;
    }
#else
    for (n=1; n<BP_SIZE; n++) {
      temp  = (qmfOutputRealWet[ch][n]*qmfOutputRealWet[ch][n]);
      temp += (qmfOutputImagWet[ch][n]*qmfOutputImagWet[ch][n]);
      temp *= BP2[n]*GF[n]*GF[n];
      WetEner[ch] += temp;
    }
#endif

    runWetEner[ch] = (float)(STP_LPF_COEFF1*runWetEner[ch] + (1.0-STP_LPF_COEFF1)*WetEner[ch]);
    WetEner[ch] /= (oldWetEner[ch]+ABS_THR);
  }

  
  switch (self->treeConfig) {
  case 0:   
  case 1:   
    scale[i_LF] = (float) sqrt((DryEner[0])/(WetEner[i_LF]+1e-9));
    scale[i_RF] = (float) sqrt((DryEner[0])/(WetEner[i_RF]+1e-9));
    scale[i_C]  = (float) sqrt((DryEner[0])/(WetEner[i_C ]+1e-9));
    scale[i_LS] = (float) sqrt((DryEner[0])/(WetEner[i_LS]+1e-9));
    scale[i_RS] = (float) sqrt((DryEner[0])/(WetEner[i_RS]+1e-9));
    break;
  case 2:   
    scale[i_LF] = (float) sqrt((DryEner[0])/(WetEner[i_LF]+1e-9));
    scale[i_RF] = (float) sqrt((DryEner[1])/(WetEner[i_RF]+1e-9));
    scale[i_LS] = (float) sqrt((DryEner[0])/(WetEner[i_LS]+1e-9));
    scale[i_RS] = (float) sqrt((DryEner[1])/(WetEner[i_RS]+1e-9));
    break;
  case 3:   
  case 4:   
    scale[i_LF] = (float) sqrt((DryEner[0])/(WetEner[i_LF]+1e-9));
    scale[i_RF] = (float) sqrt((DryEner[1])/(WetEner[i_RF]+1e-9));
    scale[i_LS] = (float) sqrt((DryEner[0])/(WetEner[i_LS]+1e-9));
    scale[i_RS] = (float) sqrt((DryEner[1])/(WetEner[i_RS]+1e-9));
    scale[i_AL] = (float) sqrt((DryEner[0])/(WetEner[i_AL]+1e-9));
    scale[i_AR] = (float) sqrt((DryEner[1])/(WetEner[i_AR]+1e-9));
    break;
  case 5:   
    scale[i_LF] = (float) sqrt((DryEner[0])/(WetEner[i_LF]+1e-9));
    scale[i_RF] = (float) sqrt((DryEner[1])/(WetEner[i_RF]+1e-9));
    scale[i_AL] = (float) sqrt((DryEner[0])/(WetEner[i_AL]+1e-9));
    scale[i_AR] = (float) sqrt((DryEner[1])/(WetEner[i_AR]+1e-9));
    break;
  case 6:   
    scale[i_LS] = (float) sqrt((DryEner[0])/(WetEner[i_LS]+1e-9));
    scale[i_RS] = (float) sqrt((DryEner[1])/(WetEner[i_RS]+1e-9));
    scale[i_AL] = (float) sqrt((DryEner[0])/(WetEner[i_AL]+1e-9));
    scale[i_AR] = (float) sqrt((DryEner[1])/(WetEner[i_AR]+1e-9));
    break;
  case 7:    
    scale[i_LF] = (float) sqrt((DryEner[0])/(WetEner[i_LF]+1e-9));
    scale[i_RF] = (float) sqrt((DryEner[0])/(WetEner[i_RF]+1e-9));
    break;
  default:
  ;
  }

  
  damp = 0.1f;
  for (ch=0;  ch<self->numOutputChannels; ch++) {
    scale[ch] = damp + (1-damp)*scale[ch];
  }

  
  for (ch=0; ch<self->numOutputChannels; ch++) {
    if (scale[ch] > STP_SCALE_LIMIT)
      scale[ch] = (float) STP_SCALE_LIMIT;
    if (scale[ch] < (1.0/STP_SCALE_LIMIT))
      scale[ch] = (float) (1.0/STP_SCALE_LIMIT);
  }

  
  for (ch=0;  ch<self->numOutputChannels; ch++) {
    scale[ch] = (float)(STP_LPF_COEFF2*scale[ch] + (1.0-STP_LPF_COEFF2)*prev_tp_scale[ch]);
    prev_tp_scale[ch] = scale[ch];
  }

  
  for (ch=0; ch<self->numOutputChannels; ch++) {

    no_scaling = 1;

    i = SpatialDecGetChannelIndex(self,ch);
    if( i != -1 ) {
      no_scaling = !self->tempShapeEnableChannelSTP[i];
    }

    for (n=0; n<HP_SIZE-3+10; n++) {

      const int hybrid2qmfMap[] = { 0, 0, 0, 0, 0, 0, 1, 1, 2, 2 };

      if(n < 10) 
        temp = (no_scaling ? 1.f : (float)(scale[ch]*BP[hybrid2qmfMap[n]]));
      else
        temp = (no_scaling ? 1.f : (float)(scale[ch]*BP[n+3-10]));


      self->hybOutputRealDry[ch][ts][n] += (self->hybOutputRealWet[ch][ts][n]*temp);

#ifdef PARTIALLY_COMPLEX
      if (n < PC_NUM_BANDS-3+10)
        self->hybOutputImagDry[ch][ts][n] += (self->hybOutputImagWet[ch][ts][n]*temp);
#else
      self->hybOutputImagDry[ch][ts][n] += (self->hybOutputImagWet[ch][ts][n]*temp);
#endif

    }

    for (n=HP_SIZE-3+10; n<self->hybridBands; n++) {

      temp = (no_scaling ? 1.f : (float)(scale[ch]));

      self->hybOutputRealDry[ch][ts][n] += (self->hybOutputRealWet[ch][ts][n]*temp);
#ifndef PARTIALLY_COMPLEX
      self->hybOutputImagDry[ch][ts][n] += (self->hybOutputImagWet[ch][ts][n]*temp);
#endif
    }
  }

}

void
SbrProcessFromMps(int channel, int el,
                  float qmfOutputReal[][MAX_NUM_QMF_BANDS],
                  float qmfOutputImag[][MAX_NUM_QMF_BANDS]);

void tpProcess(spatialDec* self, int el)
{

  int ch,ts, hyb, n;

  for (ch=0; ch< self->numOutputChannels; ch++) {
    for (ts=0; ts< self->timeSlots; ts++) {
      for (hyb=0; hyb < self->tp_hybBandBorder; hyb++) {
        self->hybOutputRealDry[ch][ts][hyb] += self->hybOutputRealWet[ch][ts][hyb];
        self->hybOutputImagDry[ch][ts][hyb] += self->hybOutputImagWet[ch][ts][hyb];
        self->hybOutputRealWet[ch][ts][hyb] = 0;
        self->hybOutputImagWet[ch][ts][hyb] = 0;
      }
    }
  }

  
  for (ts=0; ts<self->timeSlots; ts++)
#ifdef PARTIALLY_COMPLEX
    subbandTP(self,ts);
#else
    subbandTP(self,ts);
#endif

  SpatialDecHybridQMFSynthesis(self);

  for (ch = 0; ch < self->numOutputChannelsAT; ch++) {
    sbr_dec_from_mps(ch, el, self->qmfOutputRealDry[ch], self->qmfOutputImagDry[ch]);
  }

  for (ch=0; ch<self->numOutputChannelsAT; ch++) {
    for (ts=0; ts<self->timeSlots; ts++) {
#ifdef PARTIALLY_COMPLEX
      int qs;
      float qmfTemp[MAX_NUM_QMF_BANDS];
      const float invSqrt2 = (float) (1.0 / sqrt(2.0));

      Exp2Cos ( &self->qmfOutputImagDry[ch][ts], qmfTemp, PC_NUM_BANDS );

      for (qs = 0; qs < PC_NUM_BANDS; qs++) {

        qmfTemp[qs] += self->qmfOutputRealDry[ch][ts + PC_FILTERDELAY][qs];
        qmfTemp[qs] *= invSqrt2;
      }

      for (qs = PC_NUM_BANDS; qs < self->qmfBands; qs++) {

        qmfTemp[qs] = self->qmfOutputRealDry[ch][ts + PC_FILTERDELAY][qs];
      }

       SacCalculateSynFilterbank(self->qmfFilterState[ch],
                                 qmfTemp,
                                 NULL,
                                 &self->timeOut[ch][self->qmfBands*ts]);
#else
       SacCalculateSynFilterbank(self->qmfFilterState[ch],
                                 self->qmfOutputRealDry[ch][ts],
                                 self->qmfOutputImagDry[ch][ts],
                                 &self->timeOut[ch][self->qmfBands*ts]);
#endif
    }
  }

  if(!self->bsConfig.arbitraryTree) {
    switch (self->treeConfig) {
    case 0:   
    ;   
    break;
    case 1:   
    case 2:   
      if ((self->upmixType != 2) && (self->upmixType != 3)){
        for (n=0; n<self->frameLength; n++) {
          double ls  = self->timeOut[1][n];
          double rf  = self->timeOut[2][n];
          double rs  = self->timeOut[3][n];
          double c   = self->timeOut[4][n];
          double lfe = self->timeOut[5][n];

          self->timeOut[1][n] = rf;
          self->timeOut[2][n] = c;
          self->timeOut[3][n] = lfe;
          self->timeOut[4][n] = ls;
          self->timeOut[5][n] = rs;
        }
      }
      break;
    case 3:   
    case 4:   
    case 5:   
    case 6:   
      if ((self->upmixType != 2) && (self->upmixType != 3)) {
        for (n=0; n<self->frameLength; n++) {
          double al  = self->timeOut[1][n];
          double ls  = self->timeOut[2][n];
          double rf  = self->timeOut[3][n];
          double ar  = self->timeOut[4][n];
          double c   = self->timeOut[6][n];
          double lfe = self->timeOut[7][n];

          self->timeOut[1][n] = rf;
          self->timeOut[2][n] = c;
          self->timeOut[3][n] = lfe;
          self->timeOut[4][n] = ls;
          self->timeOut[6][n] = al;
          self->timeOut[7][n] = ar;
        }
      }
      break;
    default:
      ;
    }
  }
}
