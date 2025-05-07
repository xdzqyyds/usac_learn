#ifndef __IPD_H__
#define __IPD_H__

#include "defines.h"
#include "sac_types_enc.h"
#include "sac_nlc_enc.h"
#include "sac_bitstream_enc.h"

void ecDataIPD(Stream *bitstream, int data[MAX_NUM_PARAMS][MAX_NUM_BINS], int oldData[MAX_NUM_BINS], 
               LOSSLESSDATA *losslessData, int paramIdx, int numParamSets, int independencyFlag, 
               int startBand, int stopBand, int *IPDmode, int ipdQuantMode);

void OttBoxCalcIPD(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQIPDs, int PhaseMode, int numBandsIPDOut, int bQuantCoarseIPD);

void OttBoxCalcIPDRes(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQIPDs, int PhaseMode, int numBandsIPDOut);



#endif /* __IPD_H__ */
