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

#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "sac_enc.h"
#include "sac_bitstream_enc.h"

#include "sac_ipd.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))

int *kernels;
static int kernels_10[MAX_HYBRID_BANDS] =
{
    0,
    0,
    0,
    0,
    1,
    1,
    2,
    2,
    3,
    3,
    4,
    4,
    5,
    5,
    6,
    6,
    7,
    7,
    7,
    7,
    7,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9
};

static int kernels_20[MAX_HYBRID_BANDS] = {
    1,
    0,
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    14,
    15,
    15,
    15,
    16,
    16,
    16,
    16,
    17,
    17,
    17,
    17,
    17,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    18,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19,
    19
};

static int kernels_28[MAX_HYBRID_BANDS] = {
    1, 
    0, 
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13, 
    14,
    15,
    16,
    17,
    17,
    18,
    18,
    19,
    19,
    20,
    20,
    21,
    21,
    21,
    22,
    22,
    22,
    23,
    23,
    23,
    23,
    24,
    24,
    24,
    24,
    24,
    25,
    25,
    25,
    25,
    25,
    25,
    26,
    26,
    26,
    26,
    26,
    26,
    26,
    27,
    27,
    27,
    27,
    27,
    27,
    27,
    27,
    27,
    27,
    27, 
    27,
    27, 
    27, 
    27, 
    27 
};


static int ICCQuant(float val)
{
  float pQSteps[7]= {0.9685f, 0.88909f, 0.72105f, 0.48428f, 0.18382f, -0.2945f, -0.7895f};
  int i;

  if(val>=pQSteps[0]) {
    return 0;
  }
  for(i=1;i<7;i++){
    if ((val>=pQSteps[i]) && (val<=pQSteps[i-1])) {
      return i;
    }
  }
  return 7;
}

static int CLDQuant(float val)
{

  float pQSteps[30]= {-47.5, -42.5, -37.5, -32.5, -27.5, -23.5, -20.5, -17.5, -14.5, -11.5, -9.0, -7.0, -5.0, -3.0, -1.0, 1.0, 3.0, 5.0, 7.0, 9.0, 11.5, 14.5, 17.5, 20.5, 23.5, 27.5, 32.5, 37.5, 42.5, 47.5};

  int i;

  if(val<pQSteps[0]) {
    return 0-15;
  }
  for(i=1;i<30;i++){
    if ((val<=pQSteps[i]) && (val>=pQSteps[i-1])) {
      return i-15;
    }
  }
  return 30-15;
}



static void TttBox(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], float pReal3[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag3[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQClds1, int* pQClds2)
{
  int i,j;

  float CLDs1[PARAMETER_BANDS];
  float CLDs2[PARAMETER_BANDS];
  float pPow1[MAX_HYBRID_BANDS];
  float pPow2[MAX_HYBRID_BANDS];
  float pPow3[MAX_HYBRID_BANDS];

  float pPowParBand1[PARAMETER_BANDS];
  float pPowParBand2[PARAMETER_BANDS];
  float pPowParBand3[PARAMETER_BANDS];

  memset(CLDs1,0,sizeof(CLDs1));
  memset(CLDs2,0,sizeof(CLDs2));
  memset(pPow1,0,sizeof(pPow1));
  memset(pPow2,0,sizeof(pPow2));
  memset(pPow3,0,sizeof(pPow3));
  memset(pPowParBand1,0,sizeof(pPowParBand1));
  memset(pPowParBand2,0,sizeof(pPowParBand2));
  memset(pPowParBand3,0,sizeof(pPowParBand3));


  for(i = 0; i < slots; i++) {


    for(j=0;j<MAX_HYBRID_BANDS;j++) {
      pPow1[j] += pReal1[i][j]*pReal1[i][j]+pImag1[i][j]*pImag1[i][j];
      pPow2[j] += pReal2[i][j]*pReal2[i][j]+pImag2[i][j]*pImag2[i][j];
      pPow3[j] += pReal3[i][j]*pReal3[i][j]+pImag3[i][j]*pImag3[i][j];


      pReal1[i][j] = (pReal1[i][j]+pReal3[i][j]*0.7071f);
      pImag1[i][j] = (pImag1[i][j]+pImag3[i][j]*0.7071f);

      pReal2[i][j] = (pReal2[i][j]+pReal3[i][j]*0.7071f);
      pImag2[i][j] = (pImag2[i][j]+pImag3[i][j]*0.7071f);
    }
  }
  for(i=0;i<MAX_HYBRID_BANDS;i++) {
    pPowParBand1[kernels[i]] += pPow1[i];
    pPowParBand2[kernels[i]] += pPow2[i];
    pPowParBand3[kernels[i]] += pPow3[i];
  }
  for(i=0;i<PARAMETER_BANDS;i++) {
    CLDs1[i] = ((pPowParBand1[i]+pPowParBand2[i])/(pPowParBand3[i]+1e-10f));
    CLDs1[i] = (float)(10.0*log(CLDs1[i]+1e-10)/log(10.0));
    pQClds1[i] = CLDQuant(CLDs1[i]);

    CLDs2[i] = (pPowParBand1[i]/(pPowParBand2[i]+1e-10f));
    CLDs2[i] = (float)(10*log(CLDs2[i]+1e-10f)/log(10.0));
    pQClds2[i] = CLDQuant(CLDs2[i]);
  }
}

static void OttBox(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQCLDs, int* pQICCs, int bIPD)
{
  int i,j;

  float CLDs[PARAMETER_BANDS];
  float ICCs[PARAMETER_BANDS];
  float pPow1[MAX_HYBRID_BANDS];
  float pPow2[MAX_HYBRID_BANDS];

  float pXCorReal[MAX_HYBRID_BANDS];
  float pXCorImag[MAX_HYBRID_BANDS];

  float pPowParBand1[PARAMETER_BANDS];
  float pPowParBand2[PARAMETER_BANDS];

  float pXCorParBand_Real[PARAMETER_BANDS];
  float pXCorParBand_Imag[PARAMETER_BANDS];

  memset(CLDs,0,sizeof(CLDs));
  memset(ICCs,0,sizeof(ICCs));
  memset(pPow1,0,sizeof(pPow1));
  memset(pPow2,0,sizeof(pPow2));
  memset(pXCorReal, 0, sizeof(pXCorReal));
  memset(pPowParBand1,0,sizeof(pPowParBand1));
  memset(pPowParBand2,0,sizeof(pPowParBand2));

  memset(pXCorParBand_Real, 0, sizeof(pXCorParBand_Real));
  memset(pXCorParBand_Imag, 0, sizeof(pXCorParBand_Imag));

  for(i = 0; i < slots; i++) {


    for(j = 0; j < MAX_HYBRID_BANDS; j++) {
      pPow1[j] += pReal1[i][j]*pReal1[i][j]+pImag1[i][j]*pImag1[i][j];
      pPow2[j] += pReal2[i][j]*pReal2[i][j]+pImag2[i][j]*pImag2[i][j];

      pXCorReal[j] += pReal1[i][j]*pReal2[i][j] + pImag1[i][j]*pImag2[i][j];
      pXCorImag[j] += (pImag1[i][j]*pReal2[i][j] - pReal1[i][j]*pImag2[i][j]);

      if(!bIPD){
        pReal1[i][j] = (pReal1[i][j]+pReal2[i][j]);
        pImag1[i][j] = (pImag1[i][j]+pImag2[i][j]);
      } 

    }
  }
  for(i=0;i<MAX_HYBRID_BANDS;i++) {
    pPowParBand1[kernels[i]] += pPow1[i];
    pPowParBand2[kernels[i]] += pPow2[i];

    pXCorParBand_Real[kernels[i]] += pXCorReal[i];
    pXCorParBand_Imag[kernels[i]] += pXCorImag[i];

  }
  for(i=0;i<PARAMETER_BANDS;i++) {
    CLDs[i] = (pPowParBand1[i]/(pPowParBand2[i]+1e-10f));
    ICCs[i] = (float)sqrt(pXCorParBand_Real[i] * pXCorParBand_Real[i] + pXCorParBand_Imag[i] * pXCorParBand_Imag[i])/(float)sqrt((pPowParBand1[i] * pPowParBand2[i] + 1e-10));
    CLDs[i] = (float)(10*log(CLDs[i]+1e-10)/log(10.0));
    pQCLDs[i] = CLDQuant(CLDs[i]);
    pQICCs[i] = ICCQuant(ICCs[i]);
  }
}

static void OttBoxRes(int slots, float pReal1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag1[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pReal2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],float pImag2[MAX_TIME_SLOTS][MAX_HYBRID_BANDS], int* pQCLDs, int* pQICCs, int bIPD) {
  int i;

  for(i=0;i<PARAMETER_BANDS;i++) {
    pQCLDs[i] = CLDQuant(0.0);
    pQICCs[i] = ICCQuant(0.0);
  }
}

static void GetDownmixWeights(int* pQCLDs, int* pQICCs, int* pQIPDs, int quantCoarseIPD, int band, float *pW1, float *pW2) {
  float pDqStepsCLD[] = {-150.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0, -16.0, -13.0, -10.0, -8.0, -6.0, -4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 13.0, 16.0, 19.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 150.0};
  float pDqStepsICC[] = {1.0000f, 0.9370f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.5890f, -0.9900f};
  float pDqStepsIPD[] = {0.0f, 0.78539f, 1.57079f, 2.35619f, 3.14159f, 3.92699f, 4.71238f, 5.49778f};
  float pDqStepsIPDFine[] = {0.0f, 0.39269f, 0.78539f, 1.17809f, 1.57079f, 1.96349f, 2.35619f, 2.74889f, 3.14159f, 3.53429f, 3.92699f, 4.31968f, 4.71238f, 5.10508f, 5.49778f, 5.89048f};

  float CLD = (float) pow(10.0f, pDqStepsCLD[pQCLDs[kernels[band]] + 15] / 10.0f);
  float ICC = pDqStepsICC[pQICCs[kernels[band]]];
  float IPD;
  float ER;

  if(quantCoarseIPD)
    IPD = pDqStepsIPD[pQIPDs[kernels[band]]];
  else
    IPD = pDqStepsIPDFine[pQIPDs[kernels[band]]];

  ER = (CLD + 1.0f + 2.0f * ICC * (float) cos(IPD) * (float) sqrt(CLD)) / (CLD + 1.0f + 2.0f * ICC * (float) sqrt(CLD));
           
  *pW2 = (float) pow(max(ER, 0.0f), 0.25f);
  *pW1 = 2.0f - *pW2;
}

static void longadd(unsigned short a[], unsigned short b[], int lena, int lenb)
{
    int h;
    long carry = 0;

    for (h=0; h < lenb; h++)
    {
        carry += ((unsigned long)a[h]) + ((unsigned long)b[h]);  
        a[h]  = (unsigned short) carry;                         
        carry = carry >> 16;                                     
    }
                                                                
    for (; h < lena; h++)
    {
        carry = ((unsigned long)a[h]) + carry;                   
        a[h] = (unsigned short) carry;                          
        carry = carry >> 16;                                    
    }

    return;
}

static void longmult1(unsigned short a[], unsigned short b, unsigned short d[], int len)
{
  int k;
  unsigned long tmp;
  unsigned long b0 = (unsigned long)b;

  tmp  = ((unsigned long)a[0])*b0;
  d[0]  = (unsigned short)tmp;

  for (k=1; k < len; k++)
  {
    tmp  = (tmp >>16) + ((unsigned long)a[k])*b0; 
    d[k]  = (unsigned short)tmp;                                 
  }

}

static void longdiv(unsigned short b[], unsigned short a, unsigned short d[], unsigned short *pr, int len)
{
  unsigned long r;
  unsigned long tmp;
  int k;

  r = 0;                                                         
                                                                 
  for (k=len-1; k >= 0; k--)
  {
    tmp = ((unsigned long)b[k]) + (r << 16);                     
                                                                 
    if (tmp)
    {
       d[k] = (unsigned short) (tmp/a);                         
       r = tmp - d[k]*a;                                         
    }
    else
    {
      d[k] = 0;                                                  
    }
  }
  *pr = (unsigned short)r;                                       

}

static short long_norm_l (int x)
{
  short bits = 0;

  if (x != 0)
  {
    if (x < 0)
    {
        x = ~x;
        if (x == 0)  return 31;
    }
    for (bits = 0; x < (int) 0x40000000L; bits++)
    {
      x <<= 1;
    }
  }

  return (bits);
}

static void longsub(unsigned short a[], unsigned short b[], int lena, int lenb)
{
    int h;
    long carry = 0;

    for (h=0; h < lenb; h++)
    {
        carry += ((unsigned long)a[h]) - ((unsigned long)b[h]);
        a[h]  = (unsigned short) carry;
        carry = carry >> 16;
    }

    for (; h < lena; h++)
    {
        carry = ((unsigned long)a[h]) + carry;
        a[h] = (unsigned short) carry;
        carry = carry >> 16;
    }

    return;
}

static int longcntbits(unsigned short a[], int len)
{
  int k;
  int bits = 0;

  for (k=len-1; k>=0; k--)
  {
                                                               
    if (a[k] != 0)
    {
      bits = k*16 + 31 - long_norm_l((int)a[k]);             
      break;
    }
  }

  return bits;
}

tSpatialEnc *SpatialEncOpen(int treeConfig, int timeSlots, int sampleRate, int *bufferSize, Stream *bitstream, int flag_ipd, int flag_mpsres, char* tsdInputFileName)
{
  int i, sacHeaderLen, inputChannels;
  SPATIALSPECIFICCONFIG *pConfig;
  tSpatialEnc *enc = calloc(1, sizeof(tSpatialEnc));

  enc->treeConfig=treeConfig;

  switch(treeConfig) {
  case 2121:
    enc->lowBitrateMode = 0;
    if (flag_mpsres) {
      enc->outputChannels=2;
    }
    else {
      enc->outputChannels=1;
    }
    inputChannels = 2;
    if (strlen(tsdInputFileName) > 0) {
      const char* tsdInputFile = tsdInputFileName;
      enc->pTsdDataFile=fopen(tsdInputFile, "rb");
    }
    break;
  case 2122:
    enc->lowBitrateMode = 1;
    if (flag_mpsres) {
      enc->outputChannels=2;
    }
    else {
      enc->outputChannels=1;
    }
    inputChannels = 2;
    if (strlen(tsdInputFileName) > 0) {
      const char* tsdInputFile = tsdInputFileName;
      enc->pTsdDataFile=fopen(tsdInputFile, "rb");
    }
    break;
  case 5151:
  case 5152:
    inputChannels = 6;
    enc->outputChannels=1;
    break;
  case 525:
    inputChannels = 6;
    enc->outputChannels=2;
  }

  enc->timeSlots = timeSlots;

  if (sampleRate < 27713) {
    enc->qmfBands = 32;
    enc->timeSlots = timeSlots*2;
  }
  else {
    enc->qmfBands = 64;
  }

  enc->frameSize = enc->qmfBands * enc->timeSlots;
  *bufferSize = enc->frameSize;


  for(i = 0; i < 2; i++) {
    SacInitSynFilterbank_enc(NULL, enc->qmfBands);
    SacOpenSynFilterbank_enc(&enc->synfilterbank[i]);
  }

  for(i = 0; i < inputChannels; i++) {
    SacInitAnaFilterbank_enc(NULL, enc->qmfBands);
    SacOpenAnaFilterbank_enc(&enc->anafilterbank[i]);
    SacInitAnaHybFilterbank_enc(&enc->hybState[i]);
  }


  CreateSpatialEncoder(&enc->bitstreamFormatter);
  pConfig=GetSpatialSpecificConfig(enc->bitstreamFormatter);

  memset(pConfig,0,sizeof(SPATIALSPECIFICCONFIG));

  pConfig->bsFreqRes=2; /* 10/20/28 parameter bands are supported */

  if(pConfig->bsFreqRes==4) /* 10 bins */
    kernels = kernels_10;
  else if(pConfig->bsFreqRes==2)
    kernels = kernels_20; /* 20 bins */
  else if(pConfig->bsFreqRes==1)
    kernels = kernels_28; /* 28 bins */
  else
  {
    fprintf(stderr, "unsupported parameter bins!");
    exit(-1);
  }

  switch(treeConfig) {
  case 2121:
    pConfig->bsTreeConfig=7;
    pConfig->lowBitrateMode = 0;
    pConfig->bsPhaseCoding = flag_ipd;
    pConfig->bsOttBandsPhasePresent = 0;
    pConfig->bsPhaseMode = flag_ipd;
    pConfig->bsQuantCoarseIPD = 1; /* 0: Fine IPD quantization, 1: Coarse IPD quantization */
    pConfig->bsResidualCoding = flag_mpsres;
    if (pConfig->bsResidualCoding) {
      pConfig->bsResidualBands = 18;
      pConfig->bsOttBandsPhase = 18;
    }
    else {
      pConfig->bsResidualBands = 0;
      if(pConfig->bsFreqRes == 4)
        pConfig->bsOttBandsPhase = 5;
      else
        pConfig->bsOttBandsPhase = 10;
    }
    if (strlen(tsdInputFileName) > 0) {
      pConfig->bsTempShapeConfig = 3;
    }
    else {
      pConfig->bsTempShapeConfig = 0;
    }

    break;
  case 2122:
    pConfig->bsTreeConfig=7;
    pConfig->lowBitrateMode = 1;
    pConfig->bsPhaseCoding = flag_ipd;
    pConfig->bsOttBandsPhasePresent = 0;
    pConfig->bsPhaseMode = flag_ipd;
    pConfig->bsQuantCoarseIPD = 1; /* 0: Fine IPD quantization, 1: Coarse IPD quantization */
    pConfig->bsResidualCoding = flag_mpsres;
    if (pConfig->bsResidualCoding) {
      pConfig->bsResidualBands = 18;
      pConfig->bsOttBandsPhase = 18;
    }
    else {
      pConfig->bsResidualBands = 0;
      if(pConfig->bsFreqRes == 4)
        pConfig->bsOttBandsPhase = 5;
      else
        pConfig->bsOttBandsPhase = 10;
    }
    if (strlen(tsdInputFileName) > 0) {
      pConfig->bsTempShapeConfig = 3;
    }
    else {
      pConfig->bsTempShapeConfig = 0;
    }
    break;
  case 5151:
    pConfig->bsTreeConfig=0;
    pConfig->ottConfig[4].bsOttBands=2;
    pConfig->bsResidualCoding = 0;
    break;
  case 5152:
    pConfig->bsTreeConfig=1;
    pConfig->ottConfig[2].bsOttBands=2;
    pConfig->bsResidualCoding = 0;
    break;
  case 525:
    pConfig->bsTreeConfig=2;
    pConfig->ottConfig[0].bsOttBands=2;
    pConfig->tttConfig->bsTttBandsLow=PARAMETER_BANDS;
    pConfig->tttConfig->bsTttModeLow=5;
    pConfig->bsResidualCoding = 0;
    break;
  default:
    /* invalid tree config */
    exit(-1);
  }

  pConfig->bsFrameLength = enc->timeSlots-1;


  pConfig->bsFixedGainSur = 2;
  pConfig->bsFixedGainLFE = 1;
  pConfig->bsFixedGainDMX = 0;

  writeBits(bitstream, 0, 1); 
  sacHeaderLen = getSpatialSpecificConfigLength(&enc->bitstreamFormatter->spatialSpecificConfig);
  if (sacHeaderLen < 127) {
    writeBits(bitstream, sacHeaderLen, 7);
  }
  else {
    writeBits(bitstream, 127, 7);
    writeBits(bitstream, sacHeaderLen-127, 16);
  } 

  WriteSpatialSpecificConfig(bitstream, enc->bitstreamFormatter);
  ByteAlignWrite(bitstream);

  return enc; 
}

void SpatialEncApply(tSpatialEnc *self, float *audioInput, float *audioOutput, Stream *bitstream, int bUsacIndependencyFlag)
{
  static int independencyCounter = 0;

  int inputChannels = 0;
  int outputChannels = 0;
  int bufferSize = 0;
  int offset = 0;

  float in[MAX_INPUT_CHANNELS][MAX_BUFFER_SIZE];
  float out[2][MAX_BUFFER_SIZE];

  float mQmfReal[6][MAX_TIME_SLOTS][NUM_QMF_BANDS] = {0};
  float mQmfImag[6][MAX_TIME_SLOTS][NUM_QMF_BANDS] = {0};
  float mHybridReal[6][MAX_TIME_SLOTS][MAX_HYBRID_BANDS] = {0};
  float mHybridImag[6][MAX_TIME_SLOTS][MAX_HYBRID_BANDS] = {0};

  float mQmfRealOut[6][MAX_TIME_SLOTS][NUM_QMF_BANDS] = {0};
  float mQmfImagOut[6][MAX_TIME_SLOTS][NUM_QMF_BANDS] = {0};

  SPATIALFRAME *pFrameData;
  SPATIALSPECIFICCONFIG *config;

  int i, j, k, l;

  float pPreScale[6]={1.f, 1.f, 1.f, 0.3162f, 0.7071f, 0.7071f};

  if(self->treeConfig == 2121 || self->treeConfig == 2122) {
    inputChannels = 2;
  } else {
    inputChannels = 6;
  }

  for(i = 0; i < self->frameSize; i++) {
    for(j = 0; j < inputChannels; j++) {
      in[j][i]=audioInput[i*inputChannels+j]*pPreScale[j];
    }
  }


  for(l = 0; l < self->timeSlots; l++) {
    for(j = 0; j < inputChannels; j++) {
      SacCalculateAnaFilterbank_enc(self->anafilterbank[j], &in[j][l*self->qmfBands], mQmfReal[j][l], mQmfImag[j][l]);
    }
  }


  for(k = 0; k < inputChannels; k++) {
    SacApplyAnaHybFilterbank_enc(
                                 &self->hybState[k],
                                 mQmfReal[k],
                                 mQmfImag[k],
                                 self->timeSlots,
                                 mHybridReal[k],
                                 mHybridImag[k]);
  }

  pFrameData=GetSpatialFrame(self->bitstreamFormatter);
  config=GetSpatialSpecificConfig(self->bitstreamFormatter);

  if (config->bsTempShapeConfig == 3) {
    char bsCharRead;

    fread(&bsCharRead, sizeof(char), 1, self->pTsdDataFile);
    
    pFrameData->tsdData.bsTsdEnable = (int)bsCharRead;

    if (pFrameData->tsdData.bsTsdEnable) {
      unsigned short c[5];
      unsigned short b;
      unsigned short r[1];

      int i, k, h;
      int ts;
      int tsdPos[MAX_TIME_SLOTS];
      int tsdPosLen = 0;
      
      for (ts=0; ts<self->timeSlots; ts++) {
        if (ts%5 == 0) {
          pFrameData->tsdData.tsdSepData[ts] = 1;
          tsdPos[tsdPosLen] = ts;
	      tsdPosLen++;
        }
        else {
          pFrameData->tsdData.tsdSepData[ts] = 0;
        }
      }

      pFrameData->tsdData.bsTsdNumTrSlots = tsdPosLen;

      { 
        int slots, positions;
                
        for(i=0; i<4 ; i++)
          pFrameData->tsdData.bsTsdCodedPos[i] = 0;

        for (k=tsdPosLen-1; k>=0; k--) {
          positions = k+1;
          slots = tsdPos[k];

          if (positions >= slots+1)  break;

          c[0] = 1;
          for(i=1; i<5 ; i++)
            c[i] = 0;

          for (h=1; h<=positions; h++) {
            b = slots - positions + h;
            longmult1(c,b,c,5);  
            b = h;
            longdiv(c,b,c,r,5); 
          }
          longadd(pFrameData->tsdData.bsTsdCodedPos, c, 4, 4);
        }
      } 

      {    
        int N = self->timeSlots;
        int *bits = &pFrameData->tsdData.TsdCodewordLength;

        c[0] = 1;
        for(i=1; i<5 ; i++)
          c[i] = 0;
	 
        for (k=0; k<tsdPosLen; k++) {
          b = N-k;
          longmult1(c,b,c,5);   
	      b = k+1;
          longdiv(c,b,c,r,5);    
        }
    
        b = 1;
        longsub(c,&b,5,1);
        *bits = longcntbits(c,5) ; 
      }

      for (ts=0; ts<self->timeSlots; ts++) {
        if (pFrameData->tsdData.tsdSepData[ts] == 1) {
          pFrameData->tsdData.bsTsdTrPhaseData[ts] = rand() % 8;
        }
      }
    }
  }

  switch(self->treeConfig) {
  case 2121:
  case 2122:
    if (self->outputChannels == 2) {
      OttBoxRes(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],pFrameData->ottData.cld[0][0], pFrameData->ottData.icc[0][0], config->bsPhaseCoding);

      if(config->bsPhaseCoding){
        OttBoxCalcIPDRes(self->timeSlots, mHybridReal[0], mHybridImag[0], mHybridReal[1], mHybridImag[1],
                         pFrameData->ottData.ipd[0][0], pFrameData->bsPhaseMode, config->bsOttBandsPhase);
      }

      for(i = 0; i < self->timeSlots; i++) {
        for(j = 0; j < MAX_HYBRID_BANDS; j++) {
          float tmpReal = mHybridReal[0][i][j]-mHybridReal[1][i][j];
          float tmpImag = mHybridImag[0][i][j]-mHybridImag[1][i][j];

          mHybridReal[0][i][j] = (mHybridReal[0][i][j]+mHybridReal[1][i][j]);
          mHybridImag[0][i][j] = (mHybridImag[0][i][j]+mHybridImag[1][i][j]);
          mHybridReal[1][i][j] = tmpReal;
          mHybridImag[1][i][j] = tmpImag;
        }
      }
    }
    else {
      OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],pFrameData->ottData.cld[0][0], pFrameData->ottData.icc[0][0], config->bsPhaseCoding);
      
      if(config->bsPhaseCoding){
        OttBoxCalcIPD(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],
                      pFrameData->ottData.ipd[0][0],pFrameData->bsPhaseMode,config->bsOttBandsPhase, config->bsQuantCoarseIPD);
      
        for(i = 0; i < self->timeSlots; i++) {
          for(j = 0; j < MAX_HYBRID_BANDS; j++) {
            float w1, w2;

            GetDownmixWeights(pFrameData->ottData.cld[0][0], pFrameData->ottData.icc[0][0], pFrameData->ottData.ipd[0][0], config->bsQuantCoarseIPD, j, &w1, &w2);

            mHybridReal[0][i][j] = (w1*mHybridReal[0][i][j]+w2*mHybridReal[1][i][j]);
            mHybridImag[0][i][j] = (w1*mHybridImag[0][i][j]+w2*mHybridImag[1][i][j]);
          }
        }
      }
    } 
    break;
  case 5151:

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],pFrameData->ottData.cld[3][0], pFrameData->ottData.icc[3][0], 0);

    OttBox(self->timeSlots,mHybridReal[4],mHybridImag[4],mHybridReal[5],mHybridImag[5],pFrameData->ottData.cld[2][0], pFrameData->ottData.icc[2][0], 0);

    OttBox(self->timeSlots,mHybridReal[2],mHybridImag[2],mHybridReal[3],mHybridImag[3],pFrameData->ottData.cld[4][0], pFrameData->ottData.icc[4][0], 0);

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[2],mHybridImag[2],pFrameData->ottData.cld[1][0], pFrameData->ottData.icc[1][0], 0);

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[4],mHybridImag[4],pFrameData->ottData.cld[0][0], pFrameData->ottData.icc[0][0], 0);
    break;
  case 5152:

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[4],mHybridImag[4],pFrameData->ottData.cld[3][0], pFrameData->ottData.icc[3][0], 0);

    OttBox(self->timeSlots,mHybridReal[1],mHybridImag[1],mHybridReal[5],mHybridImag[5],pFrameData->ottData.cld[4][0], pFrameData->ottData.icc[4][0], 0);

    OttBox(self->timeSlots,mHybridReal[2],mHybridImag[2],mHybridReal[3],mHybridImag[3],pFrameData->ottData.cld[2][0], pFrameData->ottData.icc[2][0], 0);

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],pFrameData->ottData.cld[1][0], pFrameData->ottData.icc[1][0], 0);

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[2],mHybridImag[2],pFrameData->ottData.cld[0][0], pFrameData->ottData.icc[0][0], 0);
    break;
  case 525:

    OttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[4],mHybridImag[4],pFrameData->ottData.cld[1][0],pFrameData->ottData.icc[1][0], 0);

    OttBox(self->timeSlots,mHybridReal[1],mHybridImag[1],mHybridReal[5],mHybridImag[5],pFrameData->ottData.cld[2][0],pFrameData->ottData.icc[2][0], 0);

    OttBox(self->timeSlots,mHybridReal[2],mHybridImag[2],mHybridReal[3],mHybridImag[3],pFrameData->ottData.cld[0][0],pFrameData->ottData.icc[0][0], 0);

    TttBox(self->timeSlots,mHybridReal[0],mHybridImag[0],mHybridReal[1],mHybridImag[1],mHybridReal[2],mHybridImag[2],pFrameData->tttData.cpc_cld1[0][0],pFrameData->tttData.cpc_cld2[0][0]);
  }


  if(self->treeConfig == 2121 || self->treeConfig == 2122) {
    pFrameData->framingInfo.bsFramingType=0;
  }
  else{
    pFrameData->framingInfo.bsFramingType=1;
  }
  pFrameData->framingInfo.bsNumParamSets=1;
  pFrameData->framingInfo.bsParamSlots[0]=31;

  pFrameData->bsIndependencyFlag=0;
  if(independencyCounter-- == 0) {
    independencyCounter = 9;
    pFrameData->bsIndependencyFlag=1;
  }
  if(self->treeConfig == 2121 || self->treeConfig == 2122) {
    if(bUsacIndependencyFlag){
      pFrameData->bsIndependencyFlag = 1;
    }
  }


  WriteSpatialFrame(bitstream, self->bitstreamFormatter, ((self->treeConfig == 2121 || self->treeConfig == 2122) && bUsacIndependencyFlag)?1:0);


  if(self->outputChannels == 1) {
    SacApplySynHybFilterbank_enc(
                                 mHybridReal[0],
                                 mHybridImag[0],
                                 self->timeSlots,
                                 mQmfRealOut[0],
                                 mQmfImagOut[0]);

    for(i = 0; i < self->timeSlots; i++) {
      SacCalculateSynFilterbank_enc(self->synfilterbank[0], mQmfRealOut[0][i], mQmfImagOut[0][i], &audioOutput[i*self->qmfBands]);
    }
  } else {
    SacApplySynHybFilterbank_enc(
                                 mHybridReal[0],
                                 mHybridImag[0],
                                 self->timeSlots,
                                 mQmfRealOut[0],
                                 mQmfImagOut[0]);
    SacApplySynHybFilterbank_enc(
                                 mHybridReal[1],
                                 mHybridImag[1],
                                 self->timeSlots,
                                 mQmfRealOut[1],
                                 mQmfImagOut[1]);

    for(i = 0; i < self->timeSlots; i++) {
      SacCalculateSynFilterbank_enc(self->synfilterbank[0],
                                    mQmfRealOut[0][i],
                                    mQmfImagOut[0][i],
                                    out[0]);
      SacCalculateSynFilterbank_enc(self->synfilterbank[1],
                                    mQmfRealOut[1][i],
                                    mQmfImagOut[1][i],
                                    out[1]);

      for(j=0;j<self->qmfBands;j++) {
        for(k=0;k<2;k++) {
          audioOutput[j*2+k+2*self->qmfBands*i]=out[k][j];
        }
      }
    }
  }
}

void SpatialEncClose(tSpatialEnc *self)
{
  int i, inputChannels = 0;

  if(self != NULL) {
    
    if(self->treeConfig == 2121 || self->treeConfig == 2122) {
      inputChannels = 2;
      if (self->pTsdDataFile != NULL)  
        fclose(self->pTsdDataFile);
    } else {
      inputChannels = 6;
    }

    for(i = 0; i < 2; i++) {
      SacCloseSynFilterbank_enc(self->synfilterbank[i]);
    }
    for(i = 0; i < inputChannels; i++) {
      SacCloseAnaFilterbank_enc(self->anafilterbank[i]);
    }

    if(self->ppBitstreamDelayBuffer){
      for(i=0; i<self->nBitstreamDelayBuffer; i++){
        free(self->ppBitstreamDelayBuffer[i]);
      }
      free(self->ppBitstreamDelayBuffer);
    }

    DestroySpatialEncoder(&self->bitstreamFormatter);
    free(self);
  }
}

void SpatialEncWriteSSC(tSpatialEnc *self, Stream *bitstream)
{
  WriteSpatialSpecificConfig(bitstream, self->bitstreamFormatter);
}

void SpaceEncInitDelayCompensation(tSpatialEnc *self, const int delay, const int APRFrames)
{
  int i,j;  
  SPATIALFRAME *pFrameData;
  Stream bs;
    
  if(self->pnOutputBits != NULL){
    free(self->pnOutputBits);
  }
  self->pnOutputBits = NULL;
      
  if(self->ppBitstreamDelayBuffer != NULL){
    free(self->ppBitstreamDelayBuffer);
    self->ppBitstreamDelayBuffer = NULL;
  }
  self->decoderDelay = delay;
      
  self->nBitstreamDelayBuffer = (self->decoderDelay + self->frameSize - 1)/self->frameSize + 1;
  self->nOutputBufferDelay    = ((self->nBitstreamDelayBuffer-1)*self->frameSize - self->decoderDelay);
  
  self->nBitstreamBufferRead  = 0;
  self->nBitstreamBufferWrite = self->nBitstreamDelayBuffer-1;
  
  if ((self->ppBitstreamDelayBuffer=(unsigned char **)calloc(self->nBitstreamDelayBuffer, sizeof(char *)))==NULL){
    /*  memory allocation failed */
    exit(-1);
  }
  for (i=0; i<self->nBitstreamDelayBuffer; i++){
    if ((self->ppBitstreamDelayBuffer[i]=(unsigned char *)calloc(MAX_MPEGS_BITS/8, sizeof(char)))==NULL){
      /* memory allocation failed */
      exit(-1);
    }
  }

  self->pnOutputBits = (int *)calloc(self->nBitstreamDelayBuffer, sizeof(int));

  pFrameData = GetSpatialFrame(self->bitstreamFormatter); 
  
  pFrameData->bsIndependencyFlag = 1;
  pFrameData->framingInfo.bsNumParamSets = 1;
  pFrameData->framingInfo.bsFramingType = 0;
  pFrameData->framingInfo.bsParamSlots[0]= 31;

  for(i=0; i<self->nBitstreamDelayBuffer-1; i++){
    unsigned char *databuf;
    int bIsUsac = (self->treeConfig == 2121 || self->treeConfig == 2122)?1:0;
    int usacIndepFlag = 0;

    InitStream(&bs, NULL, STREAM_WRITE);

    if ((self->nBitstreamDelayBuffer-1 - APRFrames) == i) {
      usacIndepFlag = 1;
    }
    WriteSpatialFrame(&bs, self->bitstreamFormatter, usacIndepFlag);

    self->pnOutputBits[i] = GetBitsInStream(&bs);
    ByteAlignWrite(&bs);
    
    databuf = bs.buffer;
    for (j=0; j<(self->pnOutputBits[i] + 7)/8; j++) {
      self->ppBitstreamDelayBuffer[i][j] = databuf[j];
    }
  } 
}


/* Dummy to be able to link encoder without inluding sbr decoder library */

sbr_dec_from_mps(int channel, float qmfOutputReal[][1], float qmfOutputImag[][1]) {}

void SpatialEncGetUsacMps212Config(tSpatialEnc *self, SAC_ENC_USAC_MPS212_CONFIG *pUsacMps212Config){

  SPATIALSPECIFICCONFIG *config = GetSpatialSpecificConfig(self->bitstreamFormatter);
  
  pUsacMps212Config->bsFreqRes              = config->bsFreqRes;
  pUsacMps212Config->bsFixedGainDMX         = config->bsFixedGainDMX;
  pUsacMps212Config->bsTempShapeConfig      = config->bsTempShapeConfig;
  pUsacMps212Config->bsDecorrConfig         = config->bsDecorrConfig;
  pUsacMps212Config->bsHighRateMode         = 1 - config->lowBitrateMode;
  pUsacMps212Config->bsPhaseCoding          = config->bsPhaseCoding;
  pUsacMps212Config->bsOttBandsPhasePresent = config->bsOttBandsPhasePresent;
  pUsacMps212Config->bsOttBandsPhase        = config->bsOttBandsPhase;
  pUsacMps212Config->bsResidualBands        = config->bsResidualBands;
  pUsacMps212Config->bsPseudoLr             = 1; /* always used in case residual coding is active */
  pUsacMps212Config->bsEnvQuantMode         = 0; /* not supported */

  return;
}
