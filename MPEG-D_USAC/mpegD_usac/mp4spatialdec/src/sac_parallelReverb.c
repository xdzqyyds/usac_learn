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



#include"sac_parallelReverb.h"
#include"sac_dec.h"
#include"sac_dec_interface.h"



void BinauralInitParallelReverb(spatialDec *self)
{
  int i;
  int ReverbIRLength;

#ifdef PARTIALLY_COMPLEX
  self->systemDelay = SYSTEM_DELAY_LP;
#else
  self->systemDelay = SYSTEM_DELAY_HQ;
#endif

  
  if (self->upmixType == 2 && self->binauralQuality == 0)
  {

     ReverbIRLength = self->hrtfData->reverbIRLength;

    
    self->binInputReverb = (float*)calloc(sizeof(float), self->systemDelay + ReverbIRLength - 1 + self->timeSlots * self->qmfBands - 1 + 1);

      
    for (i = 0; i < self->timeSlots * self->qmfBands; i++)
    {
      self->binOutputReverb[0][i] = 0;
      self->binOutputReverb[1][i] = 0;
    }

      
      self->binReverbIROffset = ReverbIRLength;
    for (i = 0; i < ReverbIRLength; i++)
    {
      if (self->hrtfData->reverbL[i] != 0 || self->hrtfData->reverbR[i] != 0)
      {
        self->binReverbIROffset = i;
        break;
      }
    }
  }
}


void BinauralDoParallelReverb(spatialDec *self, float *inSamples, float DMXScalefactor)
{
  int i, l, channel;
  float temp;

  int TimeSlots = self->timeSlots;
  int QMFBands = self->qmfBands;
  int RevIRLen;
  int FrameLength;
  int InBufferLen;
  int NrChannels;
  int IROffset; 
  const float *ReverbIR[2];

  
  if (self->upmixType == 2 && self->binauralQuality == 0)
  {

    RevIRLen = self->hrtfData->reverbIRLength;
    FrameLength = TimeSlots * QMFBands;
    InBufferLen = FrameLength - 1 + RevIRLen - 1 + self->systemDelay + 1;
    NrChannels = self->numInputChannels;

    IROffset = self->binReverbIROffset;

    ReverbIR[0] = self->hrtfData->reverbL;
    ReverbIR[1] = self->hrtfData->reverbR;

    
    for (i = 0; i < InBufferLen - FrameLength; i++)
    {
      self->binInputReverb[i] = self->binInputReverb[i + FrameLength];
    }

    
    for (i = 0; i < FrameLength; i++)
    {
      self->binInputReverb[InBufferLen - FrameLength + i] = 0.0;
      for (channel = 0; channel < NrChannels; channel++)
      {
          self->binInputReverb[InBufferLen - FrameLength + i] += inSamples[NrChannels * i + channel] * DMXScalefactor;
      }
    }

    
    for (channel = 0; channel < 2; channel++)
    {
      for (i = 0; i < FrameLength; i++)
      {
        temp = 0.0;

        for (l = 0; l < RevIRLen - IROffset; l++)
        {
          temp += ReverbIR[channel][l + IROffset] * self->binInputReverb[RevIRLen - 1 + i - l - IROffset];
        }

        self->binOutputReverb[channel][i] = temp;
      }
    }
  }
}


void BinauralApplyParallelReverb(spatialDec *self)
{
  int i;
  int FrameLength = self->timeSlots * self->qmfBands;

  
  if (self->upmixType == 2 && self->binauralQuality == 0)
  {
    for (i = 0; i < FrameLength; i++)
    {
      self->timeOut[0][i] += self->binOutputReverb[0][i];
      self->timeOut[1][i] += self->binOutputReverb[1][i];
    }
  }
}




void BinauralDestroyParallelReverb(spatialDec *self)
{
  
  free(self->binInputReverb);
}
