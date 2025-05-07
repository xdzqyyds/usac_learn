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




#include"sac_process.h"
#include"sac_hybfilter.h"
#include"sac_decor.h"
#include"sac_mdct2qmf.h"
#include"sac_calcM1andM2.h"

#include <assert.h>

#define TSD_SQRT_HALF 0.707106781186548f
static float tsdPmulReal[] = 
{   1.0f, TSD_SQRT_HALF, 0.0f, -TSD_SQRT_HALF,
   -1.0f, -TSD_SQRT_HALF, 0.0f, TSD_SQRT_HALF  };
static float tsdPmulImag[] = 
{   0.0f, TSD_SQRT_HALF, 1.0f, TSD_SQRT_HALF,
    0.0f, -TSD_SQRT_HALF, -1.0f, -TSD_SQRT_HALF  };


void SpatialDecCopyPcmResidual(spatialDec* self) {

  int ch, box, ts, qs;

  if (self->upmixType != 2) {
    ch = self->numInputChannels;
    for (box = 0; box < self->numOttBoxes + self->numTttBoxes; box++) {
      if (self->bsConfig.bsResidualPresent[box]) {
        for (ts = 0; ts < self->timeSlots; ts++) {
          for (qs = 0; qs < self->qmfBands; qs++) {
            self->qmfResidualReal[box][ts][qs] = self->qmfInputReal[ch][ts][qs];
            self->qmfResidualImag[box][ts][qs] = self->qmfInputImag[ch][ts][qs];
          }
        }
        ch++;
      }
    }
  }
}


void SpatialDecMDCT2QMF(spatialDec* self) {

  int ch, rfpsf;
  int qmfGlobalOffset; 

  if (self->upmixType != 2) {
    for (ch=0; ch<self->numOttBoxes + self->numTttBoxes; ch++) {
      if (self->bsConfig.bsResidualPresent[ch]) {
        qmfGlobalOffset = 0; 
        for (rfpsf=0; rfpsf < self->residualFramesPerSpatialFrame; rfpsf++) {
          SPACE_MDCT2QMF_Process  (self->qmfBands,
                                   self->updQMF,
                                   self->resMDCT[ch][rfpsf],
                                   self->qmfResidualReal[ch],
                                   self->qmfResidualImag[ch],
                                   self->resBlocktype[ch][rfpsf],
                                   qmfGlobalOffset);
          qmfGlobalOffset += self->updQMF;
        }
      }
    }
  }

  if (self->arbitraryDownmix == 2) {
    int offset = self->numOttBoxes + self->numTttBoxes;
    for (ch = 0; ch < self->numInputChannels; ch++) {
      qmfGlobalOffset = 0;
      for (rfpsf = 0; rfpsf < self->arbdmxFramesPerSpatialFrame; rfpsf++) {
        SPACE_MDCT2QMF_Process  (self->qmfBands,
                                 self->arbdmxUpdQMF,
                                 self->resMDCT[offset + ch][rfpsf],
                                 self->qmfResidualReal[offset + ch],
                                 self->qmfResidualImag[offset + ch],
                                 self->resBlocktype[offset + ch][rfpsf],
                                 qmfGlobalOffset);
        qmfGlobalOffset += self->arbdmxUpdQMF;
      }
    }
  }
}



void SpatialDecHybridQMFAnalysis(spatialDec* self)
{

    int ch;

    for(ch=0; ch <self->numInputChannels; ch++) 
    {
        SacApplyAnaHybFilterbank(&self->hybFilterState[ch],
                                 self->qmfInputReal[ch],
                                 self->qmfInputImag[ch],
                                 self->qmfBands,
                                 self->timeSlots,
                                 self->hybInputReal[ch],
                                 self->hybInputImag[ch]);
    }

  
  if ((self->residualCoding) && (self->upmixType != 2)) {
    for (ch = 0; ch < self->numOttBoxes + self->numTttBoxes; ch++) {
      if (self->resBands[ch] > 0) {
        SacApplyAnaHybFilterbank(&self->hybFilterState[ch + self->numInputChannels],
                                 self->qmfResidualReal[ch],
                                 self->qmfResidualImag[ch],
                                 self->qmfBands,
                                 self->timeSlots,
                                 self->hybResidualReal[ch],
                                 self->hybResidualImag[ch]);
      }
    }
  }

  if (self->arbitraryDownmix == 2) {
    int offset = self->numOttBoxes + self->numTttBoxes;
    for (ch = 0; ch < self->numInputChannels; ch++) {
      SacApplyAnaHybFilterbank(&self->hybFilterState[offset + ch + self->numInputChannels],
                               self->qmfResidualReal[offset + ch],
                               self->qmfResidualImag[offset + ch],
                               self->qmfBands,
                               self->timeSlots,
                               self->hybResidualReal[offset + ch],
                               self->hybResidualImag[offset + ch]);
    }
  }
}


void SpatialDecHybridQMFSynthesis(spatialDec* self)
{
    int ch;

    for(ch=0; ch < self->numOutputChannelsAT; ch++) {
#ifdef PARTIALLY_COMPLEX
      int ts, qs;

      for (ts=0; ts < PC_FILTERLENGTH - 1; ts++) {
        for (qs=0; qs< self->qmfBands; qs++) {
          self->qmfOutputRealDry[ch][ts][qs] = self->qmfOutputRealDry[ch][ts + self->timeSlots][qs];
          self->qmfOutputImagDry[ch][ts][qs] = self->qmfOutputImagDry[ch][ts + self->timeSlots][qs];
          self->qmfOutputRealWet[ch][ts][qs] = self->qmfOutputRealWet[ch][ts + self->timeSlots][qs];
          self->qmfOutputImagWet[ch][ts][qs] = self->qmfOutputImagWet[ch][ts + self->timeSlots][qs];
        }
      }
#endif
      SacApplySynHybFilterbank(self->hybOutputRealDry[ch],
                               self->hybOutputImagDry[ch],
                               self->qmfBands,
                               self->timeSlots,
#ifdef PARTIALLY_COMPLEX
                              &self->qmfOutputRealDry[ch][PC_FILTERLENGTH - 1],
                              &self->qmfOutputImagDry[ch][PC_FILTERLENGTH - 1]
#else
                               self->qmfOutputRealDry[ch],
                               self->qmfOutputImagDry[ch]
#endif
                               );

      SacApplySynHybFilterbank(self->hybOutputRealWet[ch],
                               self->hybOutputImagWet[ch],
                               self->qmfBands,
                               self->timeSlots,
#ifdef PARTIALLY_COMPLEX
                              &self->qmfOutputRealWet[ch][PC_FILTERLENGTH - 1],
                              &self->qmfOutputImagWet[ch][PC_FILTERLENGTH - 1]
#else
                               self->qmfOutputRealWet[ch],
                               self->qmfOutputImagWet[ch]
#endif
                               );
  }
}



void  SpatialDecDecorrelate(spatialDec* self) {
    int k, l, sb_sample, idx;

    float wWetRealTemp[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
    float wWetImagTemp[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

    float vTsdReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
    float vTsdImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
    float wTsdReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
    float wTsdImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
    float tsdMulReal, tsdMulImag;
    int tsdStartBand = 7; 

   
    for(k=self->numDirektSignals; k< self->numDirektSignals+self->numDecorSignals; k++)
    {
        l = k-self->numDirektSignals;

        if(self->bsTsdEnable){ 
	
            for (sb_sample = 0; sb_sample < self->timeSlots; sb_sample++){
	        if (self->TsdSepData[sb_sample]){ 
                    for (idx = tsdStartBand; idx < self->apDecor[l]->numbins; idx++){
                        vTsdReal[sb_sample][idx] = self->vReal[k][sb_sample][idx];
	                vTsdImag[sb_sample][idx] = self->vImag[k][sb_sample][idx];
                        self->vReal[k][sb_sample][idx] = 0.0f;
    	                self->vImag[k][sb_sample][idx] = 0.0f;
           	    }
     	        }
            }
	    
	}
	
        SpatialDecDecorrelateApply( self->apDecor[l],
                                    self->vReal[k],
                                    self->vImag[k],
                                    wWetRealTemp[l],
                                    wWetImagTemp[l],
                                    self->timeSlots
                                  );

        for (sb_sample = 0; sb_sample < self->timeSlots; sb_sample++)
        {
            for (idx = 0; idx < self->apDecor[l]->numbins; idx++)
            {
                self->wWetReal[k][sb_sample][idx] = wWetRealTemp[l][sb_sample][idx];
                self->wWetImag[k][sb_sample][idx] = wWetImagTemp[l][sb_sample][idx];
            }
        }
	
	if(self->bsTsdEnable){ 
            
            for (sb_sample = 0; sb_sample < self->timeSlots; sb_sample++){
	        if (self->TsdSepData[sb_sample]) { /* transient slot -> calculate TSD output */

                    tsdMulReal = tsdPmulReal[self->bsTsdTrPhaseData[sb_sample]];
                    tsdMulImag = tsdPmulImag[self->bsTsdTrPhaseData[sb_sample]];    

                    for (idx = tsdStartBand; idx < self->apDecor[l]->numbins; idx++){
                        wTsdReal[sb_sample][idx] = tsdMulReal*vTsdReal[sb_sample][idx] - tsdMulImag*vTsdImag[sb_sample][idx];
                        wTsdImag[sb_sample][idx] = tsdMulImag*vTsdReal[sb_sample][idx] + tsdMulReal*vTsdImag[sb_sample][idx];
                    }
		}    
		else{ 
                    for (idx = tsdStartBand; idx < self->apDecor[l]->numbins; idx++){
                        wTsdReal[sb_sample][idx] = 0.0f;
                        wTsdImag[sb_sample][idx] = 0.0f;
                    }
		}
            }

            for (sb_sample = 0; sb_sample < self->timeSlots; sb_sample++){
                for (idx = tsdStartBand; idx < self->apDecor[l]->numbins; idx++){
                    self->wWetReal[k][sb_sample][idx] += wTsdReal[sb_sample][idx];
                    self->wWetImag[k][sb_sample][idx] += wTsdImag[sb_sample][idx];
                }
            }
	}
	
    }
}


void SpatialDecMergeResDecor(spatialDec* self) {

  int ts,qs, row, res, indx;

  for (ts=0; ts< self->timeSlots; ts++) {
    for (qs=0; qs< self->hybridBands; qs++) {
      indx = self->kernels[qs][0];

      
      
      for(row=0; row < self->numDirektSignals; row++){
        self->wDryReal[row][ts][qs] = self->vReal[row][ts][qs];
        self->wDryImag[row][ts][qs] = self->vImag[row][ts][qs];
      }

      
      for(row = self->numDirektSignals ; row < self->numWChannels; row++) {
        res = SpatialDecGetResidualIndex(self, row);

        if( indx < self->resBands[res] ) {
          self->wDryReal[row][ts][qs] = self->hybResidualReal[res][ts][qs];
          self->wDryImag[row][ts][qs] = self->hybResidualImag[res][ts][qs];
        } else {
          self->wDryReal[row][ts][qs] = 0.0f;
          self->wDryImag[row][ts][qs] = 0.0f;
        }
      }

      
      
      if ((self->kernels[qs][0] <  self->tttConfig[0][0].stopBand  && self->tttConfig[0][0].useTTTdecorr) ||
          (self->kernels[qs][0] >= self->tttConfig[1][0].startBand && self->tttConfig[1][0].useTTTdecorr))
      {
        self->wWetReal[5][ts][qs] = self->wWetReal[3][ts][qs] + self->wWetReal[4][ts][qs] + 0.707107f * self->wWetReal[5][ts][qs];
        self->wWetImag[5][ts][qs] = self->wWetImag[3][ts][qs] + self->wWetImag[4][ts][qs] + 0.707107f * self->wWetImag[5][ts][qs] ;
      }

      
      for(row=0; row < self->numDirektSignals; row++){
        self->wWetReal[row][ts][qs] = 0.0f;
        self->wWetImag[row][ts][qs] = 0.0f;
      }

      
      for(row = self->numDirektSignals; row < self->numWChannels; row++){
        if( indx < self->resBands[SpatialDecGetResidualIndex(self, row)] ) {  
          self->wWetReal[row][ts][qs] = 0.0f;
          self->wWetImag[row][ts][qs] = 0.0f;
        }
      }
    }
  }


}


void SpatialDecCreateW(spatialDec* self) {
  SpatialDecDecorrelate(self);

  
  SpatialDecMergeResDecor(self);


}


void SpatialDecCreateX(spatialDec* self) {
  int ts,qs, row, res, indx;

  for (ts=0; ts< self->timeSlots; ts++) {
    for (qs=0; qs < self->hybridBands; qs++) {
      indx = self->kernels[qs][0];

      
      
      for(row=0; row < self->numInputChannels; row++){
        self->xReal[row][ts][qs] = self->hybInputReal[row][ts][qs];
        self->xImag[row][ts][qs] = self->hybInputImag[row][ts][qs];
      }

      
      res = self->numOttBoxes;
      for(row = self->numInputChannels ; row < self->numInputChannels + self->numTttBoxes; row++, res++){

        if(self->residualCoding && indx < self->resBands[res] ) {
          self->xReal[row][ts][qs] = self->hybResidualReal[res][ts][qs];
          self->xImag[row][ts][qs] = self->hybResidualImag[res][ts][qs];
        } else {
          self->xReal[row][ts][qs] = 0.0f;
          self->xImag[row][ts][qs] = 0.0f;
        }
      }

      
      if (self->arbitraryDownmix == 2) {
        for (row = self->numInputChannels + self->numTttBoxes;
             row < self->numInputChannels + self->numTttBoxes + self->numInputChannels; row++, res++) {
          if (indx < self->arbdmxResidualBands) {
            self->xReal[row][ts][qs] = self->hybResidualReal[res][ts][qs];
            self->xImag[row][ts][qs] = self->hybResidualImag[res][ts][qs];
          }
          else {
            self->xReal[row][ts][qs] = 0.0f;
            self->xImag[row][ts][qs] = 0.0f;
          }
        }

      }
    }
  }
}


void SpatialDecUpdateBuffers(spatialDec* self) {

  int ts1, ts2, qs, ch;
  int boxes = self->numOttBoxes + self->numTttBoxes;

  if (self->residualCoding) {
    for (ch = 0; ch < boxes; ch++) {
      if (self->resBands[ch] > 0) {
        for (ts1 = 0, ts2 = self->timeSlots; ts1 < self->timeSlots; ts1++, ts2++) {
          for (qs = 0; qs < self->qmfBands; qs++) {
            
            self->qmfResidualReal[ch][ts1][qs] = self->qmfResidualReal[ch][ts2][qs];
            self->qmfResidualImag[ch][ts1][qs] = self->qmfResidualImag[ch][ts2][qs];
            
            self->qmfResidualReal[ch][ts2][qs] = 0.0f;
            self->qmfResidualImag[ch][ts2][qs] = 0.0f;
          }
        }
      }
    }
  }

  if (self->arbitraryDownmix == 2) {
    for (ch = 0; ch < self->numInputChannels; ch++) {
      for (ts1 = 0, ts2 = self->timeSlots; ts1 < self->timeSlots; ts1++, ts2++) {
        for (qs = 0; qs < self->qmfBands; qs++) {
          
          self->qmfResidualReal[boxes + ch][ts1][qs] = self->qmfResidualReal[boxes + ch][ts2][qs];
          self->qmfResidualImag[boxes + ch][ts1][qs] = self->qmfResidualImag[boxes + ch][ts2][qs];
          
          self->qmfResidualReal[boxes + ch][ts2][qs] = 0.0f;
          self->qmfResidualImag[boxes + ch][ts2][qs] = 0.0f;
        }
      }
    }
  }
}




