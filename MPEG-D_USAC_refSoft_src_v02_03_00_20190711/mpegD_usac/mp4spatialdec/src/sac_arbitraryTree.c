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




#include <math.h>

#include "sac_dec.h"
#include "sac_bitdec.h"
#include "sac_types.h"


void arbitraryTreeProcess (spatialDec* self);

void arbitraryTreeProcess (spatialDec* self)
{
  int   i, inCh, liveCh, band, paramIndex, wavCh, ts, qb;
  float alpha, cldLin, cu, cl;
  float stackGain [(1<<MAX_ARBITRARY_TREE_LEVELS)-1] [MAX_PARAMETER_BANDS];

  float pTempReal [MAX_OUTPUT_CHANNELS_AT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float pTempImag [MAX_OUTPUT_CHANNELS_AT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  SPATIAL_BS_CONFIG *pConfig     = &(self->bsConfig);
  SPATIAL_BS_FRAME  *pCurrFrame  = &(self->bsFrame[self->curFrameBs]);

  float (*pDryReal)[MAX_TIME_SLOTS][MAX_HYBRID_BANDS] = self->hybOutputRealDry;   
  float (*pDryImag)[MAX_TIME_SLOTS][MAX_HYBRID_BANDS] = self->hybOutputImagDry;

  int  *pChannelPos = pConfig->bsOutputChannelPosAT;                             
  int (*pOttBoxPresent)[MAX_ARBITRARY_TREE_INDEX] = pConfig->bsOttBoxPresentAT; 
  int  *pOttBands   = pConfig->bsOttBandsAT;                                       

  float (*prevGain)[MAX_PARAMETER_BANDS] = self->prevGainAT;                     
  float (*pCLD)[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS] = self->ottCLD + self->numOttBoxes; 

  int numInputChannels  = self->numOutputChannels;
  int numOutputChannels = self->numOutputChannelsAT;
  int numOTTBoxes       = pConfig->numOttBoxesAT;
  int numParamSets      = self->numParameterSets;


  if(!pConfig->arbitraryTree) return;


  for(paramIndex = 0; paramIndex < numParamSets; paramIndex++) {
            
    int ottIdx   = -1;
    int prevSlot = (paramIndex == 0 ? 0 : self->paramSlot[paramIndex-1]+1);
    int currSlot = self->paramSlot[paramIndex] + 1;
    int atCh     = 0;
    int outCh    = 0;

    for(inCh = 0; inCh < numInputChannels; inCh++) {

      float gain = 1.0f;

      switch (self->treeConfig) {
      case 0:   
        if(inCh == 3)               gain = self->lfeGain;      
        if(inCh == 4 || inCh == 5)  gain = self->surroundGain; 
      break;
      case 1:   
      case 2:   
        if(inCh == 5)               gain = self->lfeGain;      
        if(inCh == 1 || inCh == 3)  gain = self->surroundGain; 
        break;
      case 3:   
      case 4:   
      case 5:   
      case 6:         
        if(inCh == 7)               gain = self->lfeGain;      
        if(inCh == 1 || inCh == 2 ||
           inCh == 4 || inCh == 5)  gain = self->surroundGain; 
        break;
      }

      if((liveCh = pOttBoxPresent[inCh][0])) {
      
        int atInd    = 0;
      
        for(band = 0; band < MAX_PARAMETER_BANDS; band++) stackGain[0][band] = 1.0f;
    
        while(liveCh) { 

          if(pOttBoxPresent[inCh][atInd++]) { 
    
            liveCh++;
            ottIdx++;

            for(band = 0; band < pOttBands[ottIdx]; band++) {
               
              cldLin  = (float) pow ( 10.0, pCLD[ottIdx][paramIndex][band] / 10.0f );
              cu      = (float) sqrt( cldLin / ( 1 + cldLin ) );
              cl      = (float) sqrt( 1.0    / ( 1 + cldLin ) );

              
              for(i = liveCh-1; i > 0; i--)
                stackGain[i][band] = stackGain[i-1][band];
              stackGain[1][band] *= cl;
              stackGain[0][band] *= cu;
            }
                         
          } else { 
           
            liveCh--;
          
            wavCh = pChannelPos[outCh];

            for(qb = 0; qb < self->hybridBands; qb++) {
       
              int   band     = self->kernels[qb][0];
              float startVal = prevGain [atCh][band];
              float stopVal  = stackGain[0][band];
              for (ts = prevSlot; ts < currSlot; ts++) {

                alpha = (ts + 1 - prevSlot) / (float) (currSlot - prevSlot);
                
                pTempReal[wavCh][ts][qb] = gain * ((1-alpha) * startVal + alpha * stopVal) * pDryReal[inCh][ts][qb];
                pTempImag[wavCh][ts][qb] = gain * ((1-alpha) * startVal + alpha * stopVal) * pDryImag[inCh][ts][qb];
              }
            }

            for(band = 0; band < pOttBands[ottIdx]; band++) {
              prevGain [atCh][band] = stackGain[0][band];
              
              for(i = 0; i < liveCh; i++)
                stackGain[i][band] = stackGain[i+1][band];
            }
          
            outCh++;
            atCh++;
        
          }
        }
      
      } else {

        wavCh = pChannelPos[outCh];
        
        for (ts = prevSlot; ts < currSlot; ts++) {
          for(qb = 0; qb < self->hybridBands; qb++) {
            pTempReal[wavCh][ts][qb] = gain * pDryReal[inCh][ts][qb];
            pTempImag[wavCh][ts][qb] = gain * pDryImag[inCh][ts][qb];
          }
        }

        outCh++;

      }

    }

    
    for(outCh = 0; outCh < numOutputChannels; outCh++) {
      for (ts = prevSlot; ts < currSlot; ts++) {
        for(qb = 0; qb < self->hybridBands; qb++) {
          pDryReal[outCh][ts][qb] = pTempReal[outCh][ts][qb];
          pDryImag[outCh][ts][qb] = pTempImag[outCh][ts][qb];
        }
      }
    }

  }
}


