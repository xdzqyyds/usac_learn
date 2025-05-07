#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "sac_types_enc.h"
#include "sac_nlc_enc.h"
#include "sac_bitstream_enc.h"
#include "sac_enc.h"
#include "sac_bitstream_enc.h"
#include "defines.h"

#include "sac_ipd.h"

#ifndef PI
#define PI 3.141592653589795
#endif

extern int *kernels;
extern int FreqResStrideTable[];

void ecDataIPD(Stream *bitstream, int data[MAX_NUM_PARAMS][MAX_NUM_BINS], int oldData[MAX_NUM_BINS], LOSSLESSDATA *losslessData, int paramIdx, int numParamSets, int independencyFlag, int startBand, int stopBand, int *IPDmode, int bsQuantCoarseIPD)
{
  int setIdx, dataBands, i, pbStride;
  int si2ps[MAX_NUM_PARAMS] = {0};

  DATA_TYPE dataType = t_IPD;
  *IPDmode = 1;
  writeBits(bitstream, *IPDmode, 1);
  if(*IPDmode==0)
  {	  
    return;
  }

  setIdx = 0;
  losslessData->bsDataPair[paramIdx][setIdx] = 0;

  writeBits(bitstream, 0, 1);/* bsOPDSmoothingMode */
  writeBits(bitstream, 3, 2);/* bsXXXdataMode */
  writeBits(bitstream, losslessData->bsDataPair[paramIdx][setIdx], 1);
  writeBits(bitstream, bsQuantCoarseIPD, 1); 
  writeBits(bitstream, losslessData->bsFreqResStrideXXX[paramIdx][setIdx], 2);

  pbStride = FreqResStrideTable[losslessData->bsFreqResStrideXXX[paramIdx][setIdx]];
  dataBands = (stopBand-startBand-1)/pbStride+1;

  EcDataPairEnc(bitstream, data, oldData, dataType, si2ps[setIdx], startBand, dataBands, losslessData->bsDataPair[paramIdx][setIdx], bsQuantCoarseIPD, independencyFlag && (setIdx == 0));

  for(i=0; i<MAX_NUM_BINS; i++) {
    oldData[i] = data[si2ps[setIdx]+losslessData->bsDataPair[paramIdx][setIdx]][i];
  }

  return;
}


static int IPDQuant( float fAngle )
{
  int index = 0;

  float pQSteps[8] ={ 0.39269908169872f,   1.17809724509617f,   1.96349540849362f,   2.74889357189107f,
                      3.53429173528852f,   4.31968989868597f,   5.10508806208341f,   5.89048622548086f };

  if( fAngle <= pQSteps[0] || fAngle >= pQSteps[7] ){ 
    index = 0;
  }  else  {
    for( index = 6;  fAngle < pQSteps[index]; index--);
    index++;
  }

  return index;
}

static int IPDQuantFine( float fAngle )
{
  int index = 0;

  float pQSteps[16] = {0.196349541f,	0.589048623f,	0.981747704f,	1.374446786f,	1.767145868f,	2.159844949f,	2.552544031f,	2.945243113f,
                       3.337942194f,	3.730641276f,	4.123340358f,	4.51603944f,	4.908738521f,	5.301437603f,	5.694136685f,	6.086835766f};

  if( fAngle <= pQSteps[0] || fAngle >= pQSteps[15] ) { 
    index = 0;
  }
  else
  {
    for( index = 14;  fAngle < pQSteps[index]; index--);
    index++;
  }

  return index;
}

void OttBoxCalcIPD(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQIPDs, int PhaseMode, int numBandsIPDOut, int bsQuantCoarseIPD)
{
  int i,j;

  float IPDs[PARAMETER_BANDS];

  float pXCorReal[MAX_HYBRID_BANDS];
  float pXCorImag[MAX_HYBRID_BANDS];

  float pXCorParBand[PARAMETER_BANDS];
  float pXCorParBandImag[PARAMETER_BANDS];

  if(PhaseMode==0) return;

  memset(pXCorReal, 0, sizeof(pXCorReal));
  memset(pXCorParBand, 0, sizeof(pXCorParBand));

  memset(pXCorImag, 0, sizeof(pXCorImag));
  memset(pXCorParBandImag, 0, sizeof(pXCorParBandImag));

  for(i = 0; i < slots; i++) {
    for(j = 0; j < MAX_HYBRID_BANDS; j++) {
      pXCorReal[j] += pReal1[i][j]*pReal2[i][j] + pImag1[i][j]*pImag2[i][j];
      pXCorImag[j] += -pImag2[i][j]*pReal1[i][j] + pImag1[i][j]*pReal2[i][j];
    }
  }
  for(i=0;i<MAX_HYBRID_BANDS;i++) {
    pXCorParBand[kernels[i]] += pXCorReal[i];
    pXCorParBandImag[kernels[i]] += pXCorImag[i];
  }
  for(i=0;i<numBandsIPDOut;i++) {
    IPDs[i] = (float)atan2(pXCorParBandImag[i], pXCorParBand[i]);
    IPDs[i] = IPDs[i]>0 ? IPDs[i] : 2.*PI+IPDs[i];
    if(bsQuantCoarseIPD){
      pQIPDs[i] = IPDQuant(IPDs[i]);
    } else {
      pQIPDs[i] = IPDQuantFine(IPDs[i]);
    }
  }

  return;
}

void OttBoxCalcIPDRes(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], 
					  int* pQIPDs, int PhaseMode, int numBandsIPDOut)
{
  int i;

  for(i=0;i<numBandsIPDOut;i++) {
    pQIPDs[i] = IPDQuant(0.0);
  }
}
