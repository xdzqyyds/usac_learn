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
#include <string.h>

#include "sac_dec.h"

#ifdef PARTIALLY_COMPLEX
static struct _state
{
  double corrs[MAX_PARAMETER_BANDS][2];
  double qs   [MAX_PARAMETER_BANDS][2];

} state ;


void Cos2Exp ( const float  prData[][PC_NUM_BANDS],
               float       *piData,
               int          nDim )
{
  int k, n;
  float d0, d1e, d1o;

  const int lengtha0 = 6;

  const float a0[] =
  {
     0.00375672984184f,  0.07159908629242f,  0.56743883685217f,
    -0.56743883685217f, -0.07159908629242f, -0.00375672984184f
  };

  const int lengtha1e = 6;

  const float a1e[] =
  {
    0.00087709635503f,  0.04670597747406f,  0.20257613284430f,
    0.20257613284430f,  0.04670597747406f,  0.00087709635503f
  };

  const int lengtha1o = 5;

  const float a1o[] =
  {
    0.00968961250934f,  0.12080166385305f,  0.23887175675672f,
    0.12080166385305f,  0.00968961250934f
  };

  

  
  d0 = 0;
  for(n = 0; n < lengtha0; n++)
    d0 +=  prData[2*n][0]   * a0[n];

  d1e = 0;
  for(n = 0; n < lengtha1e; n++)
    d1e += prData[2*n][0]   * a1e[n];     

  d1o = 0;
  for(n = 0; n < lengtha1o; n++)
    d1o += prData[2*n+1][0] * a1o[n];     

  piData[0]  =  d0;                       
  piData[0] -= (d1e + d1o);               
  piData[1]  =  d1e - d1o ;               


  for(k = 1; k < nDim-1; k += 2)
  {

    
    d0 = 0;
    for(n = 0; n < lengtha0; n++)
      d0  += prData[2*n][k]   * a0[n];

    d1e = 0;
    for(n = 0; n < lengtha1e; n++)
      d1e += prData[2*n][k]   * a1e[n];   

    d1o = 0;
    for(n = 0; n < lengtha1o; n++)
      d1o += prData[2*n+1][k] * a1o[n];   

    piData[k  ] -=   d0;                  
    piData[k-1] -=  (d1e - d1o);          
    piData[k+1]  = -(d1e + d1o);          

    
    d0 = 0;
    for(n = 0; n < lengtha0; n++)
      d0  += prData[2*n][k+1]   * a0[n];

    d1e = 0;
    for(n = 0; n < lengtha1e; n++)
      d1e += prData[2*n][k+1]   * a1e[n]; 

    d1o = 0;
    for(n = 0; n < lengtha1o; n++)
      d1o += prData[2*n+1][k+1] * a1o[n]; 

    piData[k+1] +=   d0;                  
    piData[k  ] +=  (d1e + d1o);          
    piData[k+2]  =   d1e - d1o ;          

  }

  
  d0 = 0;
  for(n = 0; n < lengtha0; n++)
    d0  += prData[2*n][nDim-1]   * a0[n];

  d1e = 0;
  for(n = 0; n < lengtha1e; n++)
    d1e += prData[2*n][nDim-1]   * a1e[n];

  d1o = 0;
  for(n = 0; n < lengtha1o; n++)
    d1o += prData[2*n+1][nDim-1] * a1o[n];

  piData[nDim-1] -=  d0;                  
  piData[nDim-2] -= (d1e - d1o);          
  piData[nDim-1] += (d1e + d1o);          

}




void Exp2Cos ( const float   piData[][MAX_NUM_QMF_BANDS],
               float        *prData,
               int           nDim )
{
  int k, n;
  float d0, d1e, d1o;

  const int lengtha0 = 6;

  const float a0[] =
  {
    -0.00375672984184f, -0.07159908629242f, -0.56743883685217f,
     0.56743883685217f,  0.07159908629242f,  0.00375672984184f
  };

  const int lengtha1e = 6;

  const float a1e[] =
  {
    0.00087709635503f,  0.04670597747406f,  0.20257613284430f,
    0.20257613284430f,  0.04670597747406f,  0.00087709635503f
  };

  const int lengtha1o = 5;

  const float a1o[] =
  {
    0.00968961250934f,  0.12080166385305f,  0.23887175675672f,
    0.12080166385305f,  0.00968961250934f
  };

  

  
  d0 = 0;
  for(n = 0; n < lengtha0; n++)
    d0 +=  piData[2*n][0]   * a0[n];

  d1e = 0;
  for(n = 0; n < lengtha1e; n++)
    d1e += piData[2*n][0]   * a1e[n];       

  d1o = 0;
  for(n = 0; n < lengtha1o; n++)
    d1o += piData[2*n+1][0] * a1o[n];       

  prData[0]  =  d0;                         
  prData[0] -= (d1e + d1o);                 
  prData[1]  = -d1e + d1o ;                 


  for(k = 1; k < nDim-1; k += 2)
  {

    
    d0 = 0;
    for(n = 0; n < lengtha0; n++)
      d0  += piData[2*n][k]   * a0[n];

    d1e = 0;
    for(n = 0; n < lengtha1e; n++)
      d1e += piData[2*n][k]   * a1e[n];    

    d1o = 0;
    for(n = 0; n < lengtha1o; n++)
      d1o += piData[2*n+1][k] * a1o[n];    

    prData[k  ] -=   d0;                   
    prData[k-1] +=  (d1e - d1o);           
    prData[k+1]  =   d1e + d1o ;           

    
    d0 = 0;
    for(n = 0; n < lengtha0; n++)
      d0  += piData[2*n][k+1]   * a0[n];

    d1e = 0;
    for(n = 0; n < lengtha1e; n++)
      d1e += piData[2*n][k+1]   * a1e[n];  

    d1o = 0;
    for(n = 0; n < lengtha1o; n++)
      d1o += piData[2*n+1][k+1] * a1o[n];  

    prData[k+1] +=   d0;                   
    prData[k  ] -=  (d1e + d1o);           
    prData[k+2]  = - d1e + d1o ;           

  }

  
  d0 = 0;
  for(n = 0; n < lengtha0; n++)
    d0  += piData[2*n][nDim-1]   * a0[n];

  d1e = 0;
  for(n = 0; n < lengtha1e; n++)
    d1e += piData[2*n][nDim-1]   * a1e[n]; 

  d1o = 0;
  for(n = 0; n < lengtha1o; n++)
    d1o += piData[2*n+1][nDim-1] * a1o[n]; 

  prData[nDim-1] -=  d0;                   
  prData[nDim-2] += (d1e - d1o);           
  prData[nDim-1] += (d1e + d1o);           

}


void QmfAlias( spatialDec  *self )
{

  int parIdx, hybIdx, parBand, n, k, ch;
  double q;

  int *qmfBands = self->aliasQmfBands;

  
  double tau = 16.0 * self->qmfBands / self->samplingFreq;      
  double dt  = (double) self->qmfBands / self->samplingFreq;    
  double a   = exp(-dt / tau);                                 
  double thr = ABS_THR / (1-a);                          

  int qmfOffset = self->hybridBands - self->qmfBands;

  static int init = 0;

  for (n = 0; n < (self->timeSlots - HYBRID_FILTER_DELAY); n++) {
    for (k = 0; k < self->qmfBands; k++) {
      self->qmfAliasInput[n + HYBRID_FILTER_DELAY][k] = 0;
      for (ch = 0; ch < self->numInputChannels; ch++) {
        self->qmfAliasInput[n + HYBRID_FILTER_DELAY][k] += self->qmfInputReal[ch][n][k];
      }
    }
  }

  for (n = 0; n < self->nAliasBands; n++) {
    self->alias[n][0] = self->alias[n][self->numParameterSetsPrev];
  }

  if(!init) {



    hybIdx = qmfOffset + PC_NUM_BANDS - 1;

    parBand = self->kernels[hybIdx][0];
    self->nAliasBands = 0;
    while( hybIdx < self->hybridBands) {
      if( self->kernels[hybIdx][0] != parBand) {
        qmfBands[self->nAliasBands] = hybIdx -  1 - qmfOffset;
        self->nAliasBands++;
        parBand = self->kernels[hybIdx][0];
      }
      hybIdx++;
    }

    init = 1;
  }

  for(parIdx = 0; parIdx < self->numParameterSets; parIdx++) {

    
    for(n = 0; n < self->nAliasBands; n++) {

      k = (parIdx == 0) ? 0 : self->paramSlot[parIdx-1] + 1;

      
      while(k <= self->paramSlot[parIdx]) {
        
        q = self->qmfAliasInput[k][qmfBands[n]];
        state.corrs[n][0] = a * state.corrs[n][0] + q * state.qs[n][0];
        state.qs[n][0] = q;
        
        q = self->qmfAliasInput[k][qmfBands[n] + 1];
        state.corrs[n][1] = a * state.corrs[n][1] + q * state.qs[n][1];
        state.qs[n][1] = q;
        k++;
      }

      
      self->alias[n][parIdx + 1] = 0;

      
      if( qmfBands[n] & 1 ) { 
        if( (state.corrs[n][0] >  thr) && (state.corrs[n][1] >  thr))
          self->alias[n][parIdx + 1] = 1;
      } else {                
        if( (state.corrs[n][0] < -thr) && (state.corrs[n][1] < -thr))
          self->alias[n][parIdx + 1] = 1;
      }

    }


  }



  for (n = 0; n < HYBRID_FILTER_DELAY; n++) {
    for (k = 0; k < self->qmfBands; k++) {
      self->qmfAliasInput[n][k] = 0;
      for (ch = 0; ch < self->numInputChannels; ch++) {
        self->qmfAliasInput[n][k] += self->qmfInputReal[ch][self->timeSlots - HYBRID_FILTER_DELAY + n][k];
      }
    }
  }

}


void AliasLock( float R[MAX_TIME_SLOTS][MAX_HYBRID_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
                int historyIndex,
                spatialDec* self )
{

  int   ab, ps, ts, qb, row, col, prevSlot, currSlot;
  float startVal0, startVal1, stopVal0, stopVal1;
  float alpha;

  int qmfOffset = self->hybridBands - self->qmfBands;

  for(ab = 0; ab < self->nAliasBands; ab++) {

    qb = self->aliasQmfBands[ab] + qmfOffset;

    for(row = 0; row < self->numOutputChannels; row++) {

      for(col = 0; col < self->numWChannels; col++){

        for(ps = 0; ps < self->numParameterSets; ps++) {

          if (ps == 0) {
            prevSlot  = -1;
            startVal0 = self->qmfAliasHistory[historyIndex][qb  ][row][col];
            startVal1 = self->qmfAliasHistory[historyIndex][qb+1][row][col];
          }
          else {
            prevSlot  = self->paramSlot[ps-1];
            startVal0 = R[prevSlot][qb  ][row][col];
            startVal1 = R[prevSlot][qb+1][row][col];
          }

          currSlot = self->paramSlot[ps];
          stopVal0 = R[currSlot][qb  ][row][col];
          stopVal1 = R[currSlot][qb+1][row][col];

          if(self->alias[ab][ps]) {
            stopVal0 = stopVal1 = 0.5f *(stopVal0 + stopVal1);
          }

          for (ts = prevSlot+1; ts <= currSlot; ts++) {

            alpha = (ts - prevSlot) / (float) (currSlot - prevSlot);

            R[ts][qb  ][row][col] = (1-alpha) * startVal0 + alpha * stopVal0;
            R[ts][qb+1][row][col] = (1-alpha) * startVal1 + alpha * stopVal1;

          }
        }
      }
    }
  }

  for(ab = 0; ab < self->nAliasBands; ab++) {
    qb = self->aliasQmfBands[ab] + qmfOffset;
    for(row = 0; row < self->numOutputChannels; row++) {
      for(col = 0; col < self->numWChannels; col++){
        self->qmfAliasHistory[historyIndex][qb  ][row][col] = R[self->timeSlots-1][qb  ][row][col];
        self->qmfAliasHistory[historyIndex][qb+1][row][col] = R[self->timeSlots-1][qb+1][row][col];
      }
    }
  }
}

#endif
