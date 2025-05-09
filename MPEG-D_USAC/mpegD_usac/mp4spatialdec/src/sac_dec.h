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



#ifndef SAC_DEC_H
#define SAC_DEC_H

#include <stdlib.h>
#include <stdio.h>
#include "libtsp.h"

#include "sac_dec_interface.h" 

#include "sac_polyphase.h"
#include "sac_bitinput.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef PRE_BANGKOK
#define MATRIX_SMOOTHING
#define EBQ_INDEXDOMAIN
#define INTERPOLATE_INDEXDOMAIN
#endif


#define SYSTEM_DELAY_HQ					( 1281 )	
#define SYSTEM_DELAY_LP					( 1601 )	

#define PC_FILTERLENGTH                 ( 11 )
#define PC_FILTERDELAY                  ( ( PC_FILTERLENGTH - 1 ) / 2 )
#define PC_NUM_BANDS                    ( 8 )

#define ABS_THR                         ( 1e-9f * 32768 * 32768 )

#define MAX_NUM_QMF_BANDS               ( 128 )
#define MAX_HYBRID_BANDS                ( MAX_NUM_QMF_BANDS - 3 + 10 )
#define MAX_TIME_SLOTS                  ( 72 )
#define MAX_INPUT_CHANNELS              ( 6 )
#define MAX_PARAMETER_BANDS             ( 28 )

#define MAX_RESIDUAL_CHANNELS           ( 10 )
#define MAX_RESIDUAL_FRAMES             ( 4 )
#define MAX_RESIDUAL_BISTREAM           ( 836 ) 
#define MAX_MDCT_COEFFS                 ( 1024 )

#define MAX_OUTPUT_CHANNELS             ( 13 )

#define MAX_NUM_OTT                     ( 5 )
#define MAX_NUM_TTT                     ( 1 )

#define MAX_NUM_PARAMS                  ( MAX_NUM_OTT + 4 * MAX_NUM_TTT + MAX_INPUT_CHANNELS )

#define MAX_NUM_EXT_TYPES               ( 8 )

#define MAX_PARAMETER_SETS              ( 9 )

#define NUM_FREQ_STRIDE                 ( 4 )


#define MAX_M1_INPUT                    ( MAX_INPUT_CHANNELS * 2 + 1 )
#define MAX_M1_OUTPUT                   ( 8 )  
#define MAX_M2_INPUT                    ( 8 )  
#define MAX_M2_OUTPUT                   ( MAX_OUTPUT_CHANNELS )

#define MAX_M_INPUT                     MAX_M1_INPUT   
#define MAX_M_OUTPUT                    MAX_M2_OUTPUT  

#define QMF_BANDS_TO_HYBRID             ( 3 )  
#define MAX_HYBRID_ONLY_BANDS_PER_QMF   ( 8 )
#define PROTO_LEN                       ( 13 )
#define BUFFER_LEN_LF                   (   PROTO_LEN - 1       + MAX_TIME_SLOTS )
#define BUFFER_LEN_HF                   ( ( PROTO_LEN - 1 ) / 2 + MAX_TIME_SLOTS )
#define HYBRID_FILTER_DELAY             ( ( PROTO_LEN - 1 ) / 2 )

#define MAX_NO_DECORR_CHANNELS          ( 10 )

#define HRTF_AZIMUTHS                   ( 5 )

#define MAX_ARBITRARY_TREE_LEVELS       ( 2 )
#define MAX_OUTPUT_CHANNELS_AT          ( MAX_OUTPUT_CHANNELS * (1<<MAX_ARBITRARY_TREE_LEVELS) )
#define MAX_NUM_OTT_AT                  ( MAX_OUTPUT_CHANNELS * ((1<<MAX_ARBITRARY_TREE_LEVELS)-1) )
#define MAX_ARBITRARY_TREE_INDEX        ( (1<<(MAX_ARBITRARY_TREE_LEVELS+1))-1 )

typedef struct
{
    int    stateLength;
    int    numLength;
    int    denLength;
    int    complex;

    float *stateReal;
    float *stateImag;

    float *numeratorReal;
    float *numeratorImag;

    float *denominatorReal;
    float *denominatorImag;

} DECORR_FILTER_INSTANCE;

typedef struct DUCKER_INTERFACE DUCKER_INTERFACE;

struct DECORR_DEC {

    int            decorr_seed;
    int            numbins;

    DECORR_FILTER_INSTANCE  *Filter[MAX_HYBRID_BANDS];

    DUCKER_INTERFACE *ducker;

    
    int   noSampleDelay[MAX_HYBRID_BANDS];  

    float **DelayBufferReal;       
    float **DelayBufferImag;       

};

typedef struct DECORR_DEC *HANDLE_DECORR_DEC;

typedef struct {

  
  float bufferLFReal[QMF_BANDS_TO_HYBRID][BUFFER_LEN_LF];
  float bufferLFImag[QMF_BANDS_TO_HYBRID][BUFFER_LEN_LF];
  float bufferHFReal[MAX_NUM_QMF_BANDS][BUFFER_LEN_HF];
  float bufferHFImag[MAX_NUM_QMF_BANDS][BUFFER_LEN_HF];

} tHybFilterState;

typedef struct spatialBsConfig_struct{
  
  int bsFrameLength;
  int bsFreqRes;
  int bsTreeConfig;
  int bsQuantMode;
  int bsOneIcc;
  int bsArbitraryDownmix;
  int bsResidualCoding;
  int bsSmoothConfig;
  int bsFixedGainSur;
  int bsFixedGainLFE;
  int bsFixedGainDMX;
  int bsMatrixMode;
  int bsTempShapeConfig;
  int bsDecorrConfig;
  int bs3DaudioMode;
  int bs3DaudioHRTFset;
  int bsHRTFfreqRes;
  int HRTFnumBand;
  int bsHRTFnumChan;
  int bsHRTFasymmetric;
  int bsHRTFlevelLeft[MAX_OUTPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int bsHRTFlevelRight[MAX_OUTPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int bsHRTFphase[MAX_OUTPUT_CHANNELS];
  int bsHRTFphaseLR[MAX_OUTPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int bsHRTFicc[MAX_OUTPUT_CHANNELS];
  int bsHRTFiccLR[MAX_OUTPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int bsOttBands[MAX_NUM_OTT];
  int bsTttDualMode[MAX_NUM_TTT];
  int bsTttModeLow[MAX_NUM_TTT];
  int bsTttModeHigh[MAX_NUM_TTT];
  int bsTttBandsLow[MAX_NUM_TTT];

  int bsSacExtType[MAX_NUM_EXT_TYPES];
  int sacExtCnt;

  int bsResidualPresent[MAX_RESIDUAL_CHANNELS];
  int bsResidualSamplingFreqIndex;
  int bsResidualFramesPerSpatialFrame;

  int bsResidualBands[MAX_RESIDUAL_CHANNELS];

  int bsArbitraryDownmixResidualSamplingFreqIndex;
  int bsArbitraryDownmixResidualFramesPerSpatialFrame;
  int bsArbitraryDownmixResidualBands;

  int bsTttBands[MAX_NUM_TTT];
  int bsNumParameterSets;

  int bsEnvQuantMode;

  int arbitraryTree;
  int numOutChanAT;
  int numOttBoxesAT;
  int bsOutputChannelPosAT[MAX_OUTPUT_CHANNELS_AT];
  int bsOttBoxPresentAT[MAX_OUTPUT_CHANNELS][MAX_ARBITRARY_TREE_INDEX];
  int bsOttDefaultCldAT[MAX_OUTPUT_CHANNELS*MAX_ARBITRARY_TREE_INDEX];
  int bsOttModeLfeAT[MAX_OUTPUT_CHANNELS*MAX_ARBITRARY_TREE_INDEX];
  int bsOttBandsAT[MAX_OUTPUT_CHANNELS*MAX_ARBITRARY_TREE_INDEX];

  int bsHighRateMode;
  
  int bsPhaseCoding;

  int bsOttBandsPhasePresent;
  int bsOttBandsPhase;

} SPATIAL_BS_CONFIG;

typedef struct {
  int bsXXXDataMode[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)][MAX_PARAMETER_SETS];
  int bsDataPairXXX[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)][MAX_PARAMETER_SETS];   
  int bsQuantCoarseXXX[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)][MAX_PARAMETER_SETS];
  int bsFreqResStrideXXX[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)][MAX_PARAMETER_SETS];

  int bsQuantCoarseXXXprev[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)];
  int nocmpQuantCoarseXXX[max(MAX_NUM_PARAMS,MAX_NUM_OTT+MAX_NUM_OTT_AT)][MAX_PARAMETER_SETS];

} LOSSLESSDATA;

typedef struct {
  
  int bsIccDiffPresent[MAX_RESIDUAL_CHANNELS][MAX_PARAMETER_SETS] ;
  int bsIccDiff[MAX_RESIDUAL_CHANNELS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];

  void* residualDataIntern;
} RESIDUAL_FRAME_DATA;


typedef struct {
  

  int bsIndependencyFlag;

  int ottCLDidx[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  int ottICCidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int tttCPC1idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int tttCPC2idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int tttCLD1idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int tttCLD2idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int tttICCidx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  
  int ottCLDidxPrev[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_BANDS]; 
  int ottICCidxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS];
  int tttCPC1idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int tttCPC2idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int tttCLD1idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int tttCLD2idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int tttICCidxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];

  

  int cmpOttCLDidx[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  int cmpOttICCidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int ottICCdiffidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpTttCPC1idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpTttCPC2idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpTttCLD1idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpTttCLD2idx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpTttICCidx[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpOttCLDidxPrev[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_BANDS]; 
  int cmpOttICCidxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS];
  int cmpTttCPC1idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int cmpTttCPC2idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int cmpTttCLD1idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int cmpTttCLD2idxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  int cmpTttICCidxPrev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
#ifdef VERIFICATION_TEST_COMPATIBILITY
  int cmpOttICCtempidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpOttICCtempidxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS];
  int ottICCdiffidxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS];
#endif


  
  LOSSLESSDATA CLDLosslessData;
  LOSSLESSDATA ICCLosslessData;
  LOSSLESSDATA CPCLosslessData;
  LOSSLESSDATA IPDLosslessData;
  int cmpOttIPDidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  int cmpOttIPDidxPrev[MAX_NUM_OTT][MAX_PARAMETER_BANDS];
  int ottIPDidx[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  int ottIPDidxPrev[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_BANDS]; 
 
  int bsPcmCodingXXX[MAX_NUM_PARAMS][MAX_PARAMETER_SETS];
  int bsXXXmsb[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];  
  
  int bsDiffTypeXXX[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][2];
  int bsCodingSchemeXXX[MAX_NUM_PARAMS][MAX_PARAMETER_SETS];
  int bsPairingXXX[MAX_NUM_PARAMS][MAX_PARAMETER_SETS];   
  int bsDiffTimeDirectionXXX[MAX_NUM_PARAMS][MAX_PARAMETER_SETS];
  
  int bsXXXlsb[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];  

  
  int bsSmoothControl;
  int bsSmoothMode[MAX_PARAMETER_SETS];
  int bsSmoothTime[MAX_PARAMETER_SETS];
  int bsFreqResStrideSmg[MAX_PARAMETER_SETS];
  int bsSmgData[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];

  RESIDUAL_FRAME_DATA resData;

  int arbdmxGainIdx[MAX_INPUT_CHANNELS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int arbdmxGainIdxPrev[MAX_INPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int cmpArbdmxGainIdx[MAX_INPUT_CHANNELS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  int cmpArbdmxGainIdxPrev[MAX_INPUT_CHANNELS][MAX_PARAMETER_BANDS];
  int bsArbitraryDownmixResidualAbs[MAX_INPUT_CHANNELS];
  int bsArbitraryDownmixResidualAlphaUpdateSet[MAX_INPUT_CHANNELS];

} SPATIAL_BS_FRAME;



typedef struct {
  float spec_prev_real[MAX_NUM_QMF_BANDS * 8];
  float spec_prev_imag[MAX_NUM_QMF_BANDS * 8];
  float p_cross_real[MAX_NUM_QMF_BANDS * 8];
  float p_cross_imag[MAX_NUM_QMF_BANDS * 8];

  float p_sum[MAX_NUM_QMF_BANDS * 8];
  float p_sum_prev[MAX_NUM_QMF_BANDS * 8];

  float bufReal[MAX_NUM_QMF_BANDS][6];
  float bufImag[MAX_NUM_QMF_BANDS][6];

  float winBufReal[MAX_NUM_QMF_BANDS][16];
  float winBufImag[MAX_NUM_QMF_BANDS][16];
} TONALITY_STATE;

typedef struct {
  int prevSmgTime;
  int prevSmgData[MAX_PARAMETER_BANDS];
} SMOOTHING_STATE;

typedef struct {
  float partNrgPrev[2*MAX_OUTPUT_CHANNELS+MAX_INPUT_CHANNELS][MAX_PARAMETER_BANDS];
  float normNrgPrev[2*MAX_OUTPUT_CHANNELS+MAX_INPUT_CHANNELS];
  float frameNrgPrev[2*MAX_OUTPUT_CHANNELS+MAX_INPUT_CHANNELS];
} RESHAPE_BBENV_STATE;

typedef struct {
  int   useTTTdecorr;  
  int   mode;  
  int   startBand;
  int   stopBand;
  int   bitstreamStartBand;
  int   bitstreamStopBand;
} TTT_CONFIG;

typedef struct {
  float excitation[3][MAX_PARAMETER_BANDS];
  float filterCoeff;
  int tsFlag[MAX_INPUT_CHANNELS];
} BLIND_DECODER;

typedef struct {
  float powerL[MAX_PARAMETER_BANDS];
  float powerR[MAX_PARAMETER_BANDS];
  float icc[MAX_PARAMETER_BANDS];
  float ipd[MAX_PARAMETER_BANDS];
  float const *impulseL;
  float const *impulseR;
} HRTF_DIRECTIONAL_DATA;

typedef struct {
  int sampleRate;
  int impulseLength;
  HRTF_DIRECTIONAL_DATA directional[HRTF_AZIMUTHS];
  int reverbIRLength;
  float const *reverbL;
  float const *reverbR;
} HRTF_DATA;

typedef struct {
  int hrtfParameterBands;
  int hrtfNumChannels;
  HRTF_DIRECTIONAL_DATA directional[HRTF_AZIMUTHS];
} _3D_STEREO_DATA;

typedef struct HRTF_FILTERBANK HRTF_FILTERBANK;

struct spatialDec_struct {


  int sacTimeAlignFlag;   
  int sacTimeAlign;       

  int samplingFreq;       

  int treeConfig;         
  int numInputChannels;   
  int numOutputChannels;  
  int numOttBoxes;        
  int numTttBoxes;        

  int numOutputChannelsAT;
  float prevGainAT[MAX_OUTPUT_CHANNELS_AT][MAX_PARAMETER_BANDS];

  int quantMode;          
  int oneIcc;             
  int arbitraryDownmix;   
  int residualCoding;     
  int smoothConfig;       
  int tempShapeConfig;    
  int decorrConfig;       
  int mtxInversion;       
  int _3DStereoInversion; 
  int envQuantMode;       

  float surroundGain;     
  float lfeGain;          
  float clipProtectGain;  

  int ottCLDdefault[MAX_NUM_OTT];
  int tttCLD1default[MAX_NUM_TTT];
  int tttCLD2default[MAX_NUM_TTT];
  int CPCdefault;
  int ICCdefault;
  int arbdmxGainDefault;

  
  int numDirektSignals; 
  int numResidualSignals; 
  int numDecorSignals;    
  int numVChannels;   
  int numWChannels;   
  int wStartResidualIdx;  
  int numXChannels;   

  int timeSlots;     
  int curTimeSlot;   
  int frameLength;   
  int decType;       
  int upmixType;     
  int binauralQuality;

  int tp_hybBandBorder; 

  int tempShapeEnableChannelSTP[MAX_OUTPUT_CHANNELS];
  int tempShapeEnableChannelGES[MAX_OUTPUT_CHANNELS];

  float envShapeData[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS];

  HANDLE_S_BITINPUT bitstream;

  
  AFILE* outputFile;

  int parseNextBitstreamFrame;

#ifdef PARTIALLY_COMPLEX
  int alias[MAX_PARAMETER_BANDS][MAX_PARAMETER_SETS + 1];
  int aliasQmfBands[MAX_PARAMETER_BANDS];
  int nAliasBands;
#endif


  int   qmfBands;
  int   hybridBands;
  int   kernels[MAX_HYBRID_BANDS][2]; 


  
  int residualPresent[MAX_RESIDUAL_CHANNELS];
  int residualBands[MAX_RESIDUAL_CHANNELS];
  int residualFramesPerSpatialFrame;
  int updQMF;
  int coreCodedResidual;
  int numResChannels;

  
  int arbdmxResidualBands;
  int arbdmxFramesPerSpatialFrame;
  int arbdmxUpdQMF;

  
  int   resMdctPresent[MAX_RESIDUAL_CHANNELS][MAX_RESIDUAL_FRAMES];
  BLOCKTYPE resBlocktype[MAX_RESIDUAL_CHANNELS][MAX_RESIDUAL_FRAMES];
  float resMDCT[MAX_RESIDUAL_CHANNELS][MAX_RESIDUAL_FRAMES][2*MAX_MDCT_COEFFS];

  int   resBands[MAX_RESIDUAL_CHANNELS]; 

  int   bitstreamParameterBands;  
  int   numParameterBands;        
  int   ottModeLfe[MAX_NUM_OTT];  
  int   numOttBands[MAX_NUM_OTT]; 
  int   bitstreamOttBands[MAX_NUM_OTT];

  TTT_CONFIG tttConfig[2][MAX_NUM_TTT];


  int   extendFrame;
  int   numParameterSets;
  int   numParameterSetsPrev;
  int   paramSlot[MAX_PARAMETER_SETS];



  int	systemDelay;
  int	binReverbIROffset;


  SPATIAL_BS_CONFIG bsConfig;   
  SPATIAL_BS_FRAME bsFrame[2];   


  int   curFrameBs;  
  int   prevFrameBs; 


  int smoothControl;
  int smgTime[MAX_PARAMETER_SETS];
  int smgData[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];

  int bsTsdEnable;
  int TsdSepData[MAX_TIME_SLOTS];
  int bsTsdTrPhaseData[MAX_TIME_SLOTS];
  int nBitsTrSlots;
  int TsdNumTrSlots;
  int TsdCodewordLength;

  float ottCLD[MAX_NUM_OTT+MAX_NUM_OTT_AT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  float ottICC[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  float ottIPD[MAX_NUM_OTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  
  int IPDdefault;
  int bsPhaseMode;
  int OpdSmoothingMode;
  int numOttBandsIPD; 

  float PhaseLeft[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float PhaseRight[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float PhasePrevLeft[MAX_PARAMETER_BANDS];
  float PhasePrevRight[MAX_PARAMETER_BANDS];

  float tttCPC1[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tttCPC2[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tttCLD1[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tttCLD2[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
  float tttICC[MAX_NUM_TTT][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];


  float arbdmxGain[MAX_INPUT_CHANNELS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS]; 
  float arbdmxAlpha[MAX_INPUT_CHANNELS];
  float arbdmxAlphaPrev[MAX_INPUT_CHANNELS];
  int   arbdmxResidualAbs[MAX_INPUT_CHANNELS];
  int   arbdmxAlphaUpdSet[MAX_INPUT_CHANNELS];


  float ottCLDprev[MAX_NUM_OTT][MAX_PARAMETER_BANDS]; 
  float ottICCprev[MAX_NUM_OTT][MAX_PARAMETER_BANDS]; 

  float tttCPC1prev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  float tttCPC2prev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  float tttCLD1prev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  float tttCLD2prev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];
  float tttICCprev[MAX_NUM_TTT][MAX_PARAMETER_BANDS];


  float M0arbdmx[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  float M1paramReal[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M1paramImag[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  float M2decorReal[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2decorImag[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2residReal[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2residImag[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  float M0arbdmxPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  float M1paramRealPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M1paramImagPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  float M2decorRealPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2decorImagPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2residRealPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  float M2residImagPrev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];




#ifdef PARTIALLY_COMPLEX
  float qmfCos2ExpInput[MAX_INPUT_CHANNELS][PC_FILTERLENGTH][PC_NUM_BANDS];
  float qmfAliasInput[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
  float qmfAliasHistory[7][MAX_HYBRID_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
#else
  float qmfInputDelayImag[MAX_INPUT_CHANNELS][PC_FILTERDELAY][MAX_NUM_QMF_BANDS];
#endif
  float qmfInputDelayReal[MAX_INPUT_CHANNELS][PC_FILTERDELAY][MAX_NUM_QMF_BANDS];
  int   qmfInputDelayIndex;

  float qmfInputReal[MAX_INPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
  float qmfInputImag[MAX_INPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];

  float hybInputReal[MAX_INPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float hybInputImag[MAX_INPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  float *binInputReverb;


  float qmfResidualReal[MAX_RESIDUAL_CHANNELS][2 * MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
  float qmfResidualImag[MAX_RESIDUAL_CHANNELS][2 * MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];

  float hybResidualReal[MAX_RESIDUAL_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float hybResidualImag[MAX_RESIDUAL_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];


  float xReal[MAX_M1_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float xImag[MAX_M1_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  float vReal[MAX_M1_OUTPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float vImag[MAX_M1_OUTPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  float wWetReal[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float wWetImag[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  float wDryReal[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float wDryImag[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

  float resReal[MAX_RESIDUAL_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float resImag[MAX_RESIDUAL_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];




  float hybOutputRealDry[MAX_OUTPUT_CHANNELS_AT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float hybOutputImagDry[MAX_OUTPUT_CHANNELS_AT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

#ifdef PARTIALLY_COMPLEX
  float qmfOutputRealDry[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS + PC_FILTERLENGTH - 1][MAX_NUM_QMF_BANDS];
  float qmfOutputImagDry[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS + PC_FILTERLENGTH - 1][MAX_NUM_QMF_BANDS];
#else
  float qmfOutputRealDry[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
  float qmfOutputImagDry[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
#endif

  
  float hybOutputRealWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];
  float hybOutputImagWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_HYBRID_BANDS];

#ifdef PARTIALLY_COMPLEX
  float qmfOutputRealWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS + PC_FILTERLENGTH - 1][MAX_NUM_QMF_BANDS];
  float qmfOutputImagWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS + PC_FILTERLENGTH - 1][MAX_NUM_QMF_BANDS];
#else
  float qmfOutputRealWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
  float qmfOutputImagWet[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS];
#endif

  float binOutputReverb[2][MAX_TIME_SLOTS * MAX_NUM_QMF_BANDS];

  double timeOut[MAX_OUTPUT_CHANNELS][MAX_TIME_SLOTS*MAX_NUM_QMF_BANDS];



  tHybFilterState hybFilterState[MAX_INPUT_CHANNELS+MAX_RESIDUAL_CHANNELS];
  SAC_POLYPHASE_SYN_FILTERBANK *qmfFilterState[2*MAX_OUTPUT_CHANNELS];

  HANDLE_DECORR_DEC apDecor[MAX_NO_DECORR_CHANNELS];

  
  TONALITY_STATE tonState;
  SMOOTHING_STATE smoothState;

  RESHAPE_BBENV_STATE reshapeBBEnvState;

  BLIND_DECODER blindDecoder;

#ifdef HRTF_DYNAMIC_UPDATE
  HANDLE_HRTF_SOURCE hrtfSource;
#endif 
  HRTF_DATA const *hrtfData;
  _3D_STEREO_DATA _3DStereoData;

  HRTF_FILTERBANK *hrtfFilter;
  
  int bsHighRateMode;

};


#endif 
