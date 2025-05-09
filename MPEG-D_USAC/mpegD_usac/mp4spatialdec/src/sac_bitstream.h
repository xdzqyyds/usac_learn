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








#define MAX_NUM_OTT           ( 5)
#define MAX_NUM_TTT           ( 1)
#define MAX_INPUT_CHANNELS    ( 2)
#define MAX_PARAMETER_BANDS   (40)
#define MAX_RESIDUAL_CHANNELS ( 3)
#define MAX_OUTPUT_CHANNELS   ( 7)
#define MAX_PARAMETER_SETS    ( 8)

#define MAX_NUM_PARAMS        (MAX_NUM_OTT + 4*MAX_NUM_TTT + MAX_INPUT_CHANNELS)



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
} FREQ_RES;

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
    int                     ottModeLfe[MAX_NUM_PARAMS];
    int                     numTttBoxes;
    int                     numInChan;
    int                     numOutChan;
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
    int                     bsTreeConfig;
    TREEDESCRIPTION         treeDescription;
    int                     bsFrameLength;
    int                     bsFreqRes;
    int                     bsQuantMode;
    int                     bsFixedGainSur;
    int                     bsFixedGainLFE;
    int                     bsFixedGainDMX;
    int                     bsMatrixMode;
    int                     bsOneIcc;
    int                     bsArbitraryDownmix;
    int                     bsResidualCoding;
    OTTCONFIG               ottConfig[MAX_NUM_OTT];
    TTTCONFIG               tttConfig[MAX_NUM_TTT];
    RESIDUALCONFIG          *residualConfig;
} SPATIALSPECIFICCONFIG;

typedef struct {
    int                     bsFramingType;
    int                     bsNumParamSets;
    int                     bsParamSlots[MAX_PARAMETER_SETS];
} FRAMINGINFO;

typedef struct {
    int                     cld[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
    int                     icc[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
	int						ipd[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
} OTTDATA;

typedef struct {
    int                     cpc_cld1[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
    int                     cpc_cld2[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
    int                     icc[MAX_NUM_PARAMS][MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
} TTTDATA;

typedef struct {
    int                     bsSmoothControl;
    int                     bsSmoothMode[MAX_PARAMETER_SETS];
    int                     bsSmoothTime[MAX_PARAMETER_SETS];
    int                     bsFreqResStride[MAX_PARAMETER_SETS];
    int                     bsSmgData[MAX_PARAMETER_SETS][MAX_PARAMETER_BANDS];
} SMGDATA;

typedef struct {
    FRAMINGINFO             framingInfo;
    OTTDATA                 ottData;
    TTTDATA                 tttData;
    SMGDATA                 smgData;
    LOSSLESSDATA            CLDLosslessData;
    LOSSLESSDATA            ICCLosslessData;
    LOSSLESSDATA            CPCLosslessData;

} SPATIALFRAME;


#endif
