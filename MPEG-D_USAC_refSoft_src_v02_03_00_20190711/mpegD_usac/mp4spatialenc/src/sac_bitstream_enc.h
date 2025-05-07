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






#ifndef _BITSTREAM_H
#define _BITSTREAM_H

#include "sac_stream_enc.h"
#include "defines.h"




#define MAX_NUM_BOXES                   5
#define MAX_NUM_BINS                    40
#define MAX_NUM_PARAMS                  7
#define MAX_NUM_OUTPUTCHANNELS          16
#define MAX_AAC_SHORTWINDOWGROUPS       4
#define MAX_AAC_MDCT                    1024
#define MAX_AAC_FRAMES                  2
#define BITBUFSIZE                      8192
#define NUM_DATATYPES                   3

enum {
    ERROR_NONE              = 0,
    ERROR_NO_INSTANCE       = 1,
    ERROR_NO_FREE_MEMORY    = 2,
    ERROR_CANT_OPEN_FILE    = 3,
    ERROR_NOT_IMPLEMENTED   = 4,
    ERROR_INVALID_ARGUMENT  = 5
} ERROR;

enum {
    TREE_5151   = 0,
    TREE_5152   = 1,
    TREE_525    = 2,
    TREE_ESCAPE = 15
} TREE;

enum {
    FREQ_RES_40 = 0,
    FREQ_RES_20 = 1,
    FREQ_RES_10 = 2,
    FREQ_RES_5  = 3
} FREQUENCY;

enum {
    QUANTMODE_FINE  = 0,
    QUANTMODE_EBQ1  = 1,
    QUANTMODE_EBQ2  = 2
} QUANTMODE;

enum {
    TTTMODE_PRED_CORR   = 0,
    TTTMODE_PRED_NOCORR = 1,
    TTTMODE_NRG_SUB_EM  = 2,
    TTTMODE_NRG_SUB     = 3,
    TTTMODE_NRG_EM      = 4,
    TTTMODE_NRG         = 5
} TTTMODE;

enum {
  DEFAULT = 0,
  KEEP = 1,
  INTERPOLATE = 2,
  FINECOARSE = 3
} DATA_MODE;

typedef struct {
    int                     numOttBoxes;
    int                     ottModeLfe[MAX_NUM_BOXES];
    int                     numTttBoxes;
    int                     numInChan;
    int                     numOutChan;
    int                     arbitraryTree;
} TREEDESCRIPTION;

typedef struct {
    int                     bsOttBands;
} OTTCONFIG;

typedef struct {
    int                     bsTttDualMode;
    int                     bsTttModeLow;
    int                     bsTttModeHigh;
    int                     bsTttBandsLow;
} TTTCONFIG;

typedef struct {
    int                     bsResidualPresent;
    int                     bsResidualBands;
} RESIDUALCONFIG;

typedef struct {
    int                     bsSmoothConfig;
    int                     bsTreeConfig;
    TREEDESCRIPTION         treeDescription;
    int                     bsFrameLength;
    int                     bsFreqRes;
    int                     bsQuantMode;
    int                     bsFixedGainSur;
    int                     bsFixedGainLFE;
    int                     bsFixedGainDMX;
    int                     bsMatrixMode;
    int                     bsTempShapeConfig;
    int                     bsDecorrConfig;
    int                     bs3DaudioMode;
    int                     bsOneIcc;
    int                     bsArbitraryDownmix;
    int                     bsResidualCoding;
    int                     bsResidualBands;
    OTTCONFIG               ottConfig[MAX_NUM_BOXES];
    TTTCONFIG               tttConfig[MAX_NUM_BOXES];
    RESIDUALCONFIG          residualConfig[MAX_NUM_BOXES];
    int                     aacResidualFs;
    int                     aacResidualFramesPerSpatialFrame;
    int                     aacResidualUseTNS;
    float                   aacResidualBWHz[MAX_NUM_BOXES];
    int                     aacResidualBitRate[MAX_NUM_BOXES];

    int                     lowBitrateMode;
	
	int                     bsPhaseCoding;
    int                     bsPhaseMode;
    int                     bsQuantCoarseIPD;
    int                     bsOttBandsPhase;
    int                     bsOttBandsPhasePresent;

} SPATIALSPECIFICCONFIG;

typedef struct {
    int                     bsFramingType;
    int                     bsNumParamSets;
    int                     bsParamSlots[MAX_NUM_PARAMS];
} FRAMINGINFO;

typedef struct {
    int                     cld[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
    int                     icc[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
    int                     cld_old[MAX_NUM_BOXES][MAX_NUM_BINS];
    int                     icc_old[MAX_NUM_BOXES][MAX_NUM_BINS];
	int						ipd[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
	int						ipd_old[MAX_NUM_BOXES][MAX_NUM_BINS];
} OTTDATA;

typedef struct {
    int                     cpc_cld1[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
    int                     cpc_cld2[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
    int                     icc[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
    int                     cpc_cld1_old[MAX_NUM_BOXES][MAX_NUM_BINS];
    int                     cpc_cld2_old[MAX_NUM_BOXES][MAX_NUM_BINS];
    int                     icc_old[MAX_NUM_BOXES][MAX_NUM_BINS];
} TTTDATA;

typedef struct {
    int                     bsSmoothControl;
    int                     bsSmoothMode[MAX_NUM_PARAMS];
    int                     bsSmoothTime[MAX_NUM_PARAMS];
    int                     bsFreqResStride[MAX_NUM_PARAMS];
    int                     bsSmgData[MAX_NUM_PARAMS][MAX_NUM_BINS];
} SMGDATA;

typedef struct {
    int                     bsTempShapeEnable;
    int                     bsTempShapeEnableChannel[MAX_NUM_OUTPUTCHANNELS];
} TEMPSHAPEDATA;

typedef struct {
    int                     bsTsdEnable;
    int                     tsdSepData[MAX_TIME_SLOTS];
    int                     bsTsdTrPhaseData[MAX_TIME_SLOTS];
    int                     bsTsdNumTrSlots;
    unsigned short          bsTsdCodedPos[4];
    int                     TsdCodewordLength;
} TSDDATA;

typedef struct {
    int                     yadayada;
} ARBITRARYDOWNMIXDATA;

typedef struct {
    int                     bsIccDiffPresent[MAX_NUM_BOXES][MAX_NUM_PARAMS];
	int                     bsIccDiff[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
	float                   residualMdct[MAX_NUM_BOXES][MAX_AAC_FRAMES][MAX_AAC_MDCT];
} RESIDUALDATA;

typedef struct {
    int                     bsXXXDataMode[MAX_NUM_BOXES][MAX_NUM_PARAMS];
    int                     bsDataPair[MAX_NUM_BOXES][MAX_NUM_PARAMS];
    int                     bsQuantCoarseXXX[MAX_NUM_BOXES][MAX_NUM_PARAMS];
    int                     bsFreqResStrideXXX[MAX_NUM_BOXES][MAX_NUM_PARAMS];
} LOSSLESSDATA;

typedef struct {
    FRAMINGINFO             framingInfo;
    int                     bsIndependencyFlag;
    OTTDATA                 ottData;
    TTTDATA                 tttData;
    SMGDATA                 smgData;
    TEMPSHAPEDATA           tempShapeData;
    ARBITRARYDOWNMIXDATA    arbitraryDownmixData;
    RESIDUALDATA            residualData;
    LOSSLESSDATA            CLDLosslessData;
    LOSSLESSDATA            ICCLosslessData;

	LOSSLESSDATA            IPDLosslessData;
	int						bsPhaseMode;

    TSDDATA                 tsdData;

    LOSSLESSDATA            CPCLosslessData;
    int                     aacWindowGrouping[MAX_AAC_FRAMES][MAX_AAC_SHORTWINDOWGROUPS];
	int                     aacWindowSequence;
} SPATIALFRAME;

typedef struct {
    SPATIALSPECIFICCONFIG   spatialSpecificConfig;
    SPATIALFRAME            currentFrame;

    int                     numBins;
    int                     totalBits;
    int                     frameCounter;
} BSF_INSTANCE;


void DestroySpatialEncoder(BSF_INSTANCE **selfPtr);
int CreateSpatialEncoder(BSF_INSTANCE **selfPtr);

SPATIALSPECIFICCONFIG *GetSpatialSpecificConfig(BSF_INSTANCE *selfPtr);
int getSpatialSpecificConfigLength(SPATIALSPECIFICCONFIG *config);
int WriteSpatialSpecificConfig(Stream *bitstream, BSF_INSTANCE *selfPtr);

SPATIALFRAME *GetSpatialFrame(BSF_INSTANCE *selfPtr);
int WriteSpatialFrame(Stream *bitstream, BSF_INSTANCE *selfPtr, int bUsacIndependencyFlag);

#endif
