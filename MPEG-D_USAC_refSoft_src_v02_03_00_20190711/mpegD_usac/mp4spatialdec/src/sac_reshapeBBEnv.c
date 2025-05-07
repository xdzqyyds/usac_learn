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
#include "sac_bitdec.h"
#include "sac_reshapeBBEnv.h"
#include "sac_calcM1andM2.h"
#include <math.h>

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define INP_DRY_WET 0
#define INP_DMX     1

static int BBEnvKernels[MAX_HYBRID_BANDS] =
{  1,  0,  0,  1,  2,  3,  4,  5,  6,  7,
   8,  9, 10, 11, 12, 13, 14, 14, 15, 15,
  15, 16, 16, 16, 16, 17, 17, 17, 17, 17,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19
};




void SacInitBBEnv(spatialDec *self)
{

  int k;

  for(k=0;k<2*MAX_OUTPUT_CHANNELS+MAX_INPUT_CHANNELS;k++) {
    self->reshapeBBEnvState.normNrgPrev[k]=32768.f*32768.f;
  }
}

static void extractBBEnv(spatialDec *self, int inp, int ch, float *env)
{
    float slotNrg[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS];
    float partNrg[MAX_PARAMETER_BANDS], partWeight[MAX_PARAMETER_BANDS];
    int ts, qs, pb;

    int start_p = 10;
    int end_p = 18;

    float alpha = (float)exp(-64 / (0.4f * 44100));
    float beta  = (float)exp(-64 / (0.04f * 44100));

    float frameNrg = 0, normNrg = 0;

    float partNrg_old[MAX_PARAMETER_BANDS];

    int prevChOffs;

    switch (inp) {
        case INP_DRY_WET: prevChOffs = 0; break;
        case INP_DMX:     prevChOffs = 1 * self->numOutputChannels; break;
    }

    
    for (pb = 0; pb < self->numParameterBands; pb++) {
        partNrg[pb] = 0;
        for (ts = 0; ts < self->timeSlots; ts++) {
            slotNrg[ts][pb] = 0;
        }
    }

    
    for (ts = 0; ts < self->timeSlots; ts++) {
        for (qs = 0; qs < self->hybridBands; qs++) {
            switch (inp) {
            case INP_DRY_WET:   

                slotNrg[ts][BBEnvKernels[qs]] += (self->hybOutputRealDry[ch][ts][qs]+self->hybOutputRealWet[ch][ts][qs])*
                                                 (self->hybOutputRealDry[ch][ts][qs]+self->hybOutputRealWet[ch][ts][qs])+
                                                 (self->hybOutputImagDry[ch][ts][qs]+self->hybOutputImagWet[ch][ts][qs])*
                                                 (self->hybOutputImagDry[ch][ts][qs]+self->hybOutputImagWet[ch][ts][qs]);
                break;
            case INP_DMX:   
                slotNrg[ts][BBEnvKernels[qs]] += self->hybInputReal[ch][ts][qs] * self->hybInputReal[ch][ts][qs] +
                                                 self->hybInputImag[ch][ts][qs] * self->hybInputImag[ch][ts][qs];
                break;
            }
        }
    }


    for (pb = start_p; pb <= end_p; pb++) partNrg[pb] = self->reshapeBBEnvState.partNrgPrev[ch+prevChOffs][pb];


    normNrg = self->reshapeBBEnvState.normNrgPrev[ch+prevChOffs];

    for (ts = 0; ts < self->timeSlots; ts++) {
        frameNrg = 0;
        for (pb = start_p; pb <= end_p; pb++) {

            partNrg_old[pb]=partNrg[pb];

            partNrg[pb] = (1-alpha) * slotNrg[ts][pb] + alpha * partNrg[pb];

            frameNrg += slotNrg[ts][pb];
        }
        frameNrg /= (end_p - start_p + 1);

        frameNrg = (1-alpha)*frameNrg+alpha*self->reshapeBBEnvState.frameNrgPrev[ch+prevChOffs];
        self->reshapeBBEnvState.frameNrgPrev[ch+prevChOffs] = frameNrg;

        for (pb = start_p; pb <= end_p; pb++) {
            partWeight[pb] = frameNrg / (partNrg[pb]+ABS_THR);
        }

        env[ts] = 0;
        for (pb = start_p; pb <= end_p; pb++) {
            env[ts] += slotNrg[ts][pb] * partWeight[pb];
        }

        normNrg = (1-beta) * env[ts] + beta * normNrg;

        env[ts] = (float) sqrt(env[ts] / (normNrg+ABS_THR));
    }

    for (pb = start_p; pb <= end_p; pb++) self->reshapeBBEnvState.partNrgPrev[ch+prevChOffs][pb] = partNrg[pb];
    self->reshapeBBEnvState.normNrgPrev[ch+prevChOffs] = normNrg;
}

void spatialDecReshapeBBEnv(spatialDec *self)
{
  float envDry[MAX_TIME_SLOTS];
  float envDmx[2][MAX_TIME_SLOTS];
  int ch, ch2, ts, qs;
  
  int start_hsb;
  float dryFac, tmp;
  
  float slotAmp_dry=0;
  float slotAmp_wet=0;
  float slotAmp_ratio;
  
  start_hsb = 6;
  
  
  switch( self->treeConfig ) {
  case TREE_7572:
    for (ch = 0; ch < 2; ch++) {
      extractBBEnv(self, INP_DMX, ch+4, envDmx[ch]);
    }
    break;
  default:
    for (ch = 0; ch < min(self->numInputChannels,2); ch++) {
      extractBBEnv(self, INP_DMX, ch, envDmx[ch]);
    }
  }
  
  for (ch = 0; ch < self->numOutputChannels; ch++) {
    
    ch2 = SpatialDecGetChannelIndex(self,ch);
    if (ch2 == -1) continue;
    
    
    extractBBEnv(self, INP_DRY_WET, ch, envDry);
    
    if (self->tempShapeEnableChannelGES[ch2]) {
      
      for (ts = 0; ts < self->timeSlots; ts++) {
        
        switch( self->treeConfig ) {
        
		case TREE_212:	
        case TREE_5151:
        case TREE_5152:
          tmp = self->envShapeData[ch2][ts] * envDmx[0][ts];
          break;
          
        case TREE_525:
        case TREE_7271:
        case TREE_7272:
          switch (ch2) {
          case 0: 
          case 3: 
          case 5: 
            tmp = self->envShapeData[ch2][ts] * envDmx[0][ts];
            break;
          case 1: 
          case 4: 
          case 6: 
            tmp = self->envShapeData[ch2][ts] * envDmx[1][ts];
            break;
          case 2: 
            tmp = self->envShapeData[ch2][ts] * (envDmx[0][ts] + envDmx[1][ts]) * 0.5f;
          }
          break;
          
        case TREE_7571:
        case TREE_7572:
          switch (ch2) {
          case 0:
          case 2:
            tmp = self->envShapeData[ch2][ts] * envDmx[0][ts];
            break;
          case 1:
          case 3:
            tmp = self->envShapeData[ch2][ts] * envDmx[1][ts];
          }
          
        }
        
        
        dryFac = tmp / (envDry[ts]+1e-9f);

        slotAmp_dry=0;
        slotAmp_wet=0;
        
        for (qs = start_hsb; qs < self->hybridBands; qs++) {
          slotAmp_dry += self->hybOutputRealDry[ch][ts][qs]*self->hybOutputRealDry[ch][ts][qs]+
                         self->hybOutputImagDry[ch][ts][qs]*self->hybOutputImagDry[ch][ts][qs];
          
          slotAmp_wet += self->hybOutputRealWet[ch][ts][qs]*self->hybOutputRealWet[ch][ts][qs]+
                         self->hybOutputImagWet[ch][ts][qs]*self->hybOutputImagWet[ch][ts][qs];
          
        }
        
        slotAmp_ratio = (float) sqrt(slotAmp_wet/(slotAmp_dry+ABS_THR));
        
        dryFac = min(4.f, max(0.25f,max(0.f, dryFac + slotAmp_ratio*(dryFac-1.f))));
        
        
        for (qs = start_hsb; qs < self->hybridBands; qs++) {
          self->hybOutputRealDry[ch][ts][qs] *= dryFac;
          self->hybOutputImagDry[ch][ts][qs] *= dryFac;
        }
      }
    }
  }
}

