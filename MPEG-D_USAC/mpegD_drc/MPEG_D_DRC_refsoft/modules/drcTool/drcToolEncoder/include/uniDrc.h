/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc.
 
 in the course of development of the ISO/IEC 23003-4 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-4 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-4 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4
 standard.
 
 Apple Inc. retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#ifndef _UNI_DRC_H_
#define _UNI_DRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Defines for bitstream payload */
#define METHOD_DEFINITION_UNKNOWN_OTHER             0
#define METHOD_DEFINITION_PROGRAM_LOUDNESS          1
#define METHOD_DEFINITION_ANCHOR_LOUDNESS           2
#define METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE     3
#define METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX    4
#define METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX   5
#define METHOD_DEFINITION_LOUDNESS_RANGE            6
#define METHOD_DEFINITION_MIXING_LEVEL              7
#define METHOD_DEFINITION_ROOM_TYPE                 8
#define METHOD_DEFINITION_SHORT_TERM_LOUDNESS       9

#define MEASUREMENT_SYSTEM_UNKNOWN_OTHER            0
#define MEASUREMENT_SYSTEM_EBU_R_128                1
#define MEASUREMENT_SYSTEM_BS_1770_3                2
#define MEASUREMENT_SYSTEM_BS_1770_3_PRE_PROCESSING 3
#define MEASUREMENT_SYSTEM_USER                     4
#define MEASUREMENT_SYSTEM_EXPERT_PANEL             5
#define MEASUREMENT_SYSTEM_BS_1771_1                6
#define MEASUREMENT_SYSTEM_RESERVED_A               7
#define MEASUREMENT_SYSTEM_RESERVED_B               8
#define MEASUREMENT_SYSTEM_RESERVED_C               9
#define MEASUREMENT_SYSTEM_RESERVED_D               10
#define MEASUREMENT_SYSTEM_RESERVED_E               11

#define RELIABILITY_UKNOWN                          0
#define RELIABILITY_UNVERIFIED                      1
#define RELIABILITY_CEILING                         2
#define RELIABILITY_ACCURATE                        3

#define EFFECT_BIT_COUNT                            12

#define EFFECT_BIT_NONE                             (-1)  /* this effect bit is virtual */
#define EFFECT_BIT_NIGHT                            0x0001
#define EFFECT_BIT_NOISY                            0x0002
#define EFFECT_BIT_LIMITED                          0x0004
#define EFFECT_BIT_LOWLEVEL                         0x0008
#define EFFECT_BIT_DIALOG                           0x0010
#define EFFECT_BIT_GENERAL_COMPR                    0x0020
#define EFFECT_BIT_EXPAND                           0x0040
#define EFFECT_BIT_ARTISTIC                         0x0080
#define EFFECT_BIT_CLIPPING                         0x0100
#define EFFECT_BIT_FADE                             0x0200
#define EFFECT_BIT_DUCK_OTHER                       0x0400
#define EFFECT_BIT_DUCK_SELF                        0x0800

#define GAIN_CODING_PROFILE_REGULAR                 0
#define GAIN_CODING_PROFILE_FADING                  1
#define GAIN_CODING_PROFILE_CLIPPING                2
#define GAIN_CODING_PROFILE_CONSTANT                3
#define GAIN_CODING_PROFILE_DUCKING                 GAIN_CODING_PROFILE_CLIPPING

#define GAIN_INTERPOLATION_TYPE_SPLINE              0
#define GAIN_INTERPOLATION_TYPE_LINEAR              1

#if INCLUDE_DECODER_DEFINITIONS
/* Defines for user requests related to target loudness */
#define USER_METHOD_DEFINITION_DEFAULT              0
#define USER_METHOD_DEFINITION_PROGRAM_LOUDNESS     1
#define USER_METHOD_DEFINITION_ANCHOR_LOUDNESS      2

#define USER_MEASUREMENT_SYSTEM_DEFAULT             0
#define USER_MEASUREMENT_SYSTEM_BS_1770_3           1
#define USER_MEASUREMENT_SYSTEM_USER                2
#define USER_MEASUREMENT_SYSTEM_EXPERT_PANEL        3
#define USER_MEASUREMENT_SYSTEM_RESERVED_A          4
#define USER_MEASUREMENT_SYSTEM_RESERVED_B          5
#define USER_MEASUREMENT_SYSTEM_RESERVED_C          6
#define USER_MEASUREMENT_SYSTEM_RESERVED_D          7
#define USER_MEASUREMENT_SYSTEM_RESERVED_E          8

#define USER_LOUDNESS_PREPROCESSING_DEFAULT         0
#define USER_LOUDNESS_PREPROCESSING_OFF             1
#define USER_LOUDNESS_PREPROCESSING_HIGHPASS        2

/* Requestable dynamic range measurement values */
#define SHORT_TERM_LOUDNESS_TO_AVG                  0
#define MOMENTARY_LOUDNESS_TO_AVG                   1
#define TOP_OF_LOUDNESS_RANGE_TO_AVG                2
#endif
    
#define LOUDNESS_DEVIATION_MAX_DEFAULT              63
#define LOUDNESS_NORMALIZATION_GAIN_MAX_DEFAULT     1000 /* infinity as default */
#define GAIN_SET_COUNT_MAX                          8 /* reduced size */
    
#if AMD1_SYNTAX
    
#define LEFT_SIDE                                   0
#define RIGHT_SIDE                                  1
#define SPLIT_CHARACTERISTIC_COUNT_MAX              8
#define SPLIT_CHARACTERISTIC_NODE_COUNT_MAX         4  /* one side of characteristic */
    
#define GAINFORMAT_QMF32                            0x1
#define GAINFORMAT_QMFHYBRID39                      0x2
#define GAINFORMAT_QMF64                            0x3
#define GAINFORMAT_QMFHYBRID71                      0x4
#define GAINFORMAT_QMF128                           0x5
#define GAINFORMAT_QMFHYBRID135                     0x6
#define GAINFORMAT_UNIFORM                          0x7
    
#define EQ_PURPOSE_GENERIC                          1
#define EQ_PURPOSE_LARGE_ROOM                       2
#define EQ_PURPOSE_SMALL_SPACE                      3
    
#define DRC_INPUT_LOUDNESS_TARGET                   (-31.0f)  /* dB */
    
#define SHAPE_FILTER_COUNT_MAX                      8
    
#define SHAPE_FILTER_TYPE_OFF                       0
#define SHAPE_FILTER_TYPE_LF_CUT                    1
#define SHAPE_FILTER_TYPE_LF_BOOST                  2
#define SHAPE_FILTER_TYPE_HF_CUT                    3
#define SHAPE_FILTER_TYPE_HF_BOOST                  4
    
#define SHAPE_FILTER_DRC_GAIN_MAX_MINUS_ONE         1583.8931924611f  /* 10^3.2 - 1 */
    
#define DOWNMIX_INSTRUCTIONS_COUNT_MAX              8  /* reduced size */
#define DRC_COEFFICIENTS_UNIDRC_V1_COUNT_MAX        2  /* reduced size */
#define DRC_INSTRUCTIONS_UNIDRC_V1_COUNT_MAX        8  /* reduced size */
#define DRC_BAND_COUNT_MAX                          BAND_COUNT_MAX
#define SPLIT_CHARACTERISTIC_COUNT_MAX              8 /* reduced size */
#define SHAPE_FILTER_COUNT_MAX                      8 /* reduced size */
#define ADDITIONAL_DOWNMIX_ID_COUNT_MAX             ADDITIONAL_DOWNMIX_ID_MAX
#define ADDITIONAL_DRC_SET_ID_COUNT_MAX             16
#define ADDITIONAL_EQ_SET_ID_COUNT_MAX              8
#define LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX             4
#define FILTER_ELEMENT_COUNT_MAX                    16  /* reduced size */
#define REAL_ZERO_RADIUS_ONE_COUNT_MAX              14
#define REAL_ZERO_COUNT_MAX                         64
#define COMPLEX_ZERO_COUNT_MAX                      64
#define REAL_POLE_COUNT_MAX                         16
#define COMPLEX_POLE_COUNT_MAX                      16
#define FIR_ORDER_MAX                               128
#define EQ_NODE_COUNT_MAX                           33
#define EQ_SUBBAND_GAIN_COUNT_MAX                   135
#define UNIQUE_SUBBAND_GAIN_COUNT_MAX               16  /* reduced size */
#define FILTER_BLOCK_COUNT_MAX                      16
#define FILTER_ELEMENT_COUNT_MAX                    16  /* reduced size */
#define UNIQUE_SUBBAND_GAINS_COUNT_MAX              8   /* reduced size */
#define EQ_CHANNEL_GROUP_COUNT_MAX                  4   /* reduced size */
#define EQ_FILTER_BLOCK_COUNT_MAX                   4   /* reduced size */
#define LOUD_EQ_INSTRUCTIONS_COUNT_MAX              8   /* reduced size */
#define EQ_INSTRUCTIONS_COUNT_MAX                   8
    
#define DRC_COMPLEXITY_LEVEL_MAX                    15
#define EQ_COMPLEXITY_LEVEL_MAX                     15
#define COMPLEXITY_W_SUBBAND_EQ                     2.5f
#define COMPLEXITY_W_FIR                            0.4f
#define COMPLEXITY_W_IIR                            5.0f
#define COMPLEXITY_W_MOD_TIME                       1.0f
#define COMPLEXITY_W_MOD_SUBBAND                    2.0f
#define COMPLEXITY_W_LAP                            2.0f
#define COMPLEXITY_W_SHAPE                          6.0f
#define COMPLEXITY_W_SPLINE                         5.0f
#define COMPLEXITY_W_LINEAR                         2.5f
#define COMPLEXITY_W_PARAM_DRC_FILT                 5.0f
#define COMPLEXITY_W_PARAM_DRC_SUBBAND              5.0f
#define COMPLEXITY_W_PARAM_LIM_FILT                 4.5f
#define COMPLEXITY_W_PARAM_DRC_ATTACK               136.0f
    
typedef struct {
    int   type;
    float gainOffset;
    float y1Bound;
    float warpedGainMax;
    float factor;
    float coeffSum;
    float partialCoeffSum;
    float gNorm;
    float a1;
    float a2;
    float b1;
    float b2;
    float audioInState1;
    float audioInState2;
    float audioOutState1;
    float audioOutState2;
} ShapeFilter;

typedef struct {
    int shapeFilterBlockPresent;
    float drcGainLast;
    ShapeFilter shapeFilter[4];
} ShapeFilterBlock;
    
#endif /* AMD1_SYNTAX */

/* ======================================================================
 Structures to reflect the information conveyed by the bitstream syntax
 ====================================================================== */

#if AMD1_SYNTAX
typedef struct {
    int levelEstimKWeightingType;
    int levelEstimIntegrationTimePresent;
    int levelEstimIntegrationTime;
    int drcCurveDefinitionType;
    int drcCharacteristic;
    int nodeCount;
    int nodeLevel[PARAM_DRC_TYPE_FF_NODE_COUNT_MAX];
    int nodeGain[PARAM_DRC_TYPE_FF_NODE_COUNT_MAX];
    int drcGainSmoothParametersPresent;
    int gainSmoothAttackTimeSlow;
    int gainSmoothReleaseTimeSlow;
    int gainSmoothTimeFastPresent;
    int gainSmoothAttackTimeFast;
    int gainSmoothReleaseTimeFast;
    int gainSmoothThresholdPresent;
    int gainSmoothAttackThreshold;
    int gainSmoothReleaseThreshold;
    int gainSmoothHoldOffCountPresent;
    int gainSmoothHoldOff;
    
    /* derived data */
    int disableParamtricDrc;
} ParametricDrcTypeFeedForward;
    
#ifdef AMD1_PARAMETRIC_LIMITER
typedef struct {
    int parametricLimThresholdPresent;
    float parametricLimThreshold;
    int parametricLimAttack;
    int parametricLimReleasePresent;
    int parametricLimRelease;
    int drcCharacteristic;
    
    /* derived data */
    int disableParamtricDrc;
} ParametricDrcTypeLim;
#endif

typedef struct {
    int parametricDrcId;
    int parametricDrcLookAheadPresent;
    int parametricDrcLookAhead;
    int parametricDrcPresetIdPresent;
    int parametricDrcPresetId;
    int parametricDrcType;
    int lenBitSize;
    ParametricDrcTypeFeedForward parametricDrcTypeFeedForward;
#ifdef AMD1_PARAMETRIC_LIMITER
    ParametricDrcTypeLim parametricDrcTypeLim;
#endif
    
    /* derived data */
    int disableParamtricDrc;
} ParametricDrcInstructions;

typedef struct {
    int parametricDrcId;
    int sideChainConfigType;
    int downmixId;
    int levelEstimChannelWeightFormat;
    float levelEstimChannelWeight[CHANNEL_COUNT_MAX];
    int drcInputLoudnessPresent;
    float drcInputLoudness;
    
    /* derived data */
    int channelCountFromDownmixId;
} ParametricDrcGainSetParams;

typedef struct {
    int drcLocation;
    int parametricDrcFrameSizeFormat;
    int parametricDrcFrameSize;
    int parametricDrcDelayMaxPresent;
    int parametricDrcDelayMax;
    int resetParametricDrc;
    int parametricDrcGainSetCount;
    ParametricDrcGainSetParams parametricDrcGainSetParams[SEQUENCE_COUNT_MAX];
} DrcCoefficientsParametricDrc;
#endif /* AMD1_SYNTAX */

typedef struct {
    int baseChannelCount;
    int layoutSignalingPresent;
    int definedLayout;
    int speakerPosition[SPEAKER_POS_COUNT_MAX];
} ChannelLayout;

typedef struct {
    int downmixId;
    int targetChannelCount;
    int targetLayout;
    int downmixCoefficientsPresent;
    float downmixCoefficient[DOWNMIX_COEFF_COUNT_MAX];
} DownmixInstructions;

typedef struct {
    int gainSequenceIndex;
#if AMD1_SYNTAX
    int drcCharacteristicPresent;
    int drcCharacteristicFormatIsCICP;
    int drcCharacteristic;
    int drcCharacteristicLeftIndex;
    int drcCharacteristicRightIndex;
#else /* !AMD1_SYNTAX */
    int drcCharacteristic;
#endif /* AMD1_SYNTAX */
    int crossoverFreqIndex;
    int startSubBandIndex;
} GainParams;

typedef struct {
    int duckingScalingPresent;
    float duckingScaling;
    float duckingScalingQuantized;
} DuckingModifiers;

typedef struct {
#if AMD1_SYNTAX
    int targetCharacteristicLeftPresent [BAND_COUNT_MAX];
    int targetCharacteristicLeftIndex   [BAND_COUNT_MAX];
    int targetCharacteristicRightPresent[BAND_COUNT_MAX];
    int targetCharacteristicRightIndex  [BAND_COUNT_MAX];
    int shapeFilterPresent;
    int shapeFilterIndex;
    
    int     gainScalingPresent  [BAND_COUNT_MAX];
    float   attenuationScaling  [BAND_COUNT_MAX];
    float   amplificationScaling[BAND_COUNT_MAX];
    int     gainOffsetPresent   [BAND_COUNT_MAX];
    float   gainOffset          [BAND_COUNT_MAX];
#else /* !AMD1_SYNTAX */
    int     gainScalingPresent[1];
    float   attenuationScaling[1];
    float   amplificationScaling[1];
    int     gainOffsetPresent[1];
    float   gainOffset[1];
#endif /* AMD1_SYNTAX */
} GainModifiers;

typedef struct {
    int gainCodingProfile;
    int gainInterpolationType;
    int fullFrame;
    int timeAlignment;
    int timeDeltaMinPresent;
    int deltaTmin;
    int bandCount;
    int drcBandType;
    GainParams gainParams[BAND_COUNT_MAX];
} GainSetParams;
    
#if AMD1_SYNTAX
typedef struct {
    int characteristicFormat;
    int bsGain;
    int bsIoRatio;
    int bsExp;
    int flipSign;
    int characteristicNodeCount;
    float nodeLevel[SPLIT_CHARACTERISTIC_NODE_COUNT_MAX+1];
    float nodeGain [SPLIT_CHARACTERISTIC_NODE_COUNT_MAX+1];
} SplitDrcCharacteristic;

typedef struct {
    int cornerFreqIndex;
    int filterStrengthIndex;
} ShapeFilterParams;

typedef struct {
    int lfCutFilterPresent;
    ShapeFilterParams lfCutParams;
    int lfBoostFilterPresent;
    ShapeFilterParams lfBoostParams;
    int hfCutFilterPresent;
    ShapeFilterParams hfCutParams;
    int hfBoostFilterPresent;
    ShapeFilterParams hfBoostParams;
} ShapeFilterBlockParams;
#endif /* AMD1_SYNTAX */

typedef struct {
    int drcLocation;
    int drcCharacteristic;
} DrcCoefficientsBasic;

typedef struct {
    int drcLocation;
    int drcFrameSizePresent;
    int drcFrameSize;
#if AMD1_SYNTAX
    int drcCharacteristicLeftPresent;
    int characteristicLeftCount;
    SplitDrcCharacteristic splitCharacteristicLeft [SPLIT_CHARACTERISTIC_COUNT_MAX+1];
    int drcCharacteristicRightPresent;
    int characteristicRightCount;
    SplitDrcCharacteristic splitCharacteristicRight[SPLIT_CHARACTERISTIC_COUNT_MAX];
    int shapeFiltersPresent;
    int shapeFilterCount;
    ShapeFilterBlockParams shapeFilterBlockParams[SHAPE_FILTER_COUNT_MAX+1];
    int gainSequenceCount;
#endif /* AMD1_SYNTAX */
    int gainSetCount;
    GainSetParams gainSetParams[GAIN_SET_COUNT_MAX];
} DrcCoefficientsUniDrc;

typedef struct {
    int drcSetId;
    int drcLocation;
    int downmixId;
    int additionalDownmixIdPresent;
    int additionalDownmixIdCount;
    int additionalDownmixId[ADDITIONAL_DOWNMIX_ID_MAX];
    int drcSetEffect;
    int limiterPeakTargetPresent;
    float limiterPeakTarget;
    int drcSetTargetLoudnessPresent;
    int drcSetTargetLoudnessValueUpper;
    int drcSetTargetLoudnessValueLowerPresent;
    int drcSetTargetLoudnessValueLower;
} DrcInstructionsBasic;

typedef struct {
    int drcSetId;
#if AMD1_SYNTAX
    int drcSetComplexityLevel;
    int drcApplyToDownmix;
    int requiresEq;
    int downmixIdPresent;
#endif /* AMD1_SYNTAX */
    int drcLocation;
    int downmixId;
    int additionalDownmixIdPresent;
    int additionalDownmixIdCount;
    int additionalDownmixId[ADDITIONAL_DOWNMIX_ID_MAX];
    int dependsOnDrcSetPresent;
    int dependsOnDrcSet;
    int noIndependentUse;
    int drcSetEffect;
    int gainSetIndex[CHANNEL_COUNT_MAX];
    GainModifiers gainModifiers[CHANNEL_GROUP_COUNT_MAX];
    DuckingModifiers duckingModifiersForChannel[CHANNEL_COUNT_MAX];
    int limiterPeakTargetPresent;
    float limiterPeakTarget;
    int drcSetTargetLoudnessPresent;
    int drcSetTargetLoudnessValueUpper;
    int drcSetTargetLoudnessValueLowerPresent;
    int drcSetTargetLoudnessValueLower;
#if MPEG_H_SYNTAX
    int drcInstructionsType;
    int mae_groupID;
    int mae_groupPresetID;
#endif
    
    /* derived data */
    int drcChannelCount;
    int nDrcChannelGroups;
    int gainSetIndexForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int bandCountForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int gainCodingProfileForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int gainInterpolationTypeForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int timeDeltaMinForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int timeAlignmentForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    DuckingModifiers duckingModifiersForChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int channelGroupForChannel[CHANNEL_COUNT_MAX];
    int nChannelsPerChannelGroup[CHANNEL_GROUP_COUNT_MAX];
    int gainElementCount;               /* number of different DRC gains inluding all DRC bands*/
    int multibandAudioSignalCount;      /* number of audio signals including all channels with all subbands*/
#if AMD1_SYNTAX
    int channelGroupIsParametricDrc[CHANNEL_GROUP_COUNT_MAX];
    int gainSetIndexForChannelGroupParametricDrc[CHANNEL_GROUP_COUNT_MAX];
#endif /* AMD1_SYNTAX */

#ifdef LEVELING_SUPPORT
    int levelingPresent;
    int duckingOnlyDrcSet;
#endif
} DrcInstructionsUniDrc;

typedef struct {
    int methodDefinition;
    float methodValue;
    int measurementSystem;
    int reliability;
} LoudnessMeasure;

typedef struct {
    int drcSetId;
#if AMD1_SYNTAX
    int eqSetId;
#endif /* AMD1_SYNTAX */
    int downmixId;
    int samplePeakLevelPresent;
    float samplePeakLevel;
    int truePeakLevelPresent;
    float truePeakLevel;
    int truePeakLevelMeasurementSystem;
    int truePeakLevelReliability;
    int measurementCount;
    LoudnessMeasure loudnessMeasure[MEASUREMENT_COUNT_MAX];
#if MPEG_H_SYNTAX
    int loudnessInfoType;
    int mae_groupID;
    int mae_groupPresetID;
#endif
} LoudnessInfo;
    
#if AMD1_SYNTAX
typedef struct {
    int loudEqSetId;
    int drcLocation;
    int downmixIdPresent;
    int downmixId;
    int additionalDownmixIdPresent;
    int additionalDownmixIdCount;
    int additionalDownmixId [ADDITIONAL_DOWNMIX_ID_COUNT_MAX];
    int drcSetIdPresent;
    int drcSetId;
    int additionalDrcSetIdPresent;
    int additionalDrcSetIdCount;
    int additionalDrcSetId [ADDITIONAL_DRC_SET_ID_COUNT_MAX];
    int eqSetIdPresent;
    int eqSetId;
    int additionalEqSetIdPresent;
    int additionalEqSetIdCount;
    int additionalEqSetId [ADDITIONAL_EQ_SET_ID_COUNT_MAX];
    int loudnessAfterDrc;
    int loudnessAfterEq;
    int loudEqGainSequenceCount;
    int gainSequenceIndex               [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    int drcCharacteristicFormatIsCICP   [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    int drcCharacteristic               [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    int drcCharacteristicLeftIndex      [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    int drcCharacteristicRightIndex     [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    int frequencyRangeIndex             [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    float loudEqScaling                 [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
    float loudEqOffset                  [LOUD_EQ_GAIN_SEQUENCE_COUNT_MAX];
} LoudEqInstructions;

typedef struct {
    int filterElementIndex;
    int filterElementGainPresent;
    float filterElementGain;
} FilterElement;

typedef struct {
    int filterElementCount;
    FilterElement filterElement[FILTER_ELEMENT_COUNT_MAX];
} FilterBlock;

typedef struct {
    int eqFilterFormat;
    int realZeroRadiusOneCount;
    int realZeroCount;
    int genericZeroCount;
    int realPoleCount;
    int complexPoleCount;
    float zeroSign          [REAL_ZERO_RADIUS_ONE_COUNT_MAX];
    float realZeroRadius    [REAL_ZERO_COUNT_MAX];
    float genericZeroRadius [COMPLEX_ZERO_COUNT_MAX];
    float genericZeroAngle  [COMPLEX_ZERO_COUNT_MAX];
    float realPoleRadius    [REAL_POLE_COUNT_MAX];
    float complexPoleRadius [COMPLEX_POLE_COUNT_MAX];
    float complexPoleAngle  [COMPLEX_POLE_COUNT_MAX];
    int firFilterOrder;
    int firSymmetry;
    float firCoefficient [FIR_ORDER_MAX/2];
} UniqueTdFilterElement;

typedef struct {
    int nEqNodes;
    float eqSlope       [EQ_NODE_COUNT_MAX];
    int eqFreqDelta     [EQ_NODE_COUNT_MAX];
    float eqGainInitial;
    float eqGainDelta   [EQ_NODE_COUNT_MAX];
} EqSubbandGainSpline;

typedef struct {
    int eqSubbandGain [EQ_SUBBAND_GAIN_COUNT_MAX];
} EqSubbandGainVector;

typedef struct {
    int eqDelayMaxPresent;
    int eqDelayMax;
    int uniqueFilterBlockCount;
    FilterBlock filterBlock [FILTER_BLOCK_COUNT_MAX];
    int uniqueTdFilterElementCount;
    UniqueTdFilterElement uniqueTdFilterElement [FILTER_ELEMENT_COUNT_MAX];
    int uniqueEqSubbandGainsCount;
    int eqSubbandGainRepresentation;
    int eqSubbandGainFormat;
    int eqSubbandGainCount;
    EqSubbandGainSpline eqSubbandGainSpline [UNIQUE_SUBBAND_GAIN_COUNT_MAX];
    EqSubbandGainVector eqSubbandGainVector [UNIQUE_SUBBAND_GAIN_COUNT_MAX];
} EqCoefficients;

typedef struct {
    int filterBlockCount;
    int filterBlockIndex[EQ_FILTER_BLOCK_COUNT_MAX];
} FilterBlockRefs;

typedef struct {
    int eqCascadeGainPresent        [EQ_CHANNEL_GROUP_COUNT_MAX];
    int eqCascadeGain               [EQ_CHANNEL_GROUP_COUNT_MAX];
    FilterBlockRefs filterBlockRefs [EQ_CHANNEL_GROUP_COUNT_MAX];
    int eqPhaseAlignmentPresent;
    int eqPhaseAlignment [EQ_CHANNEL_GROUP_COUNT_MAX][EQ_CHANNEL_GROUP_COUNT_MAX];
} TdFilterCascade;

typedef struct {
    int eqSetId;
    int eqSetComplexityLevel;
    int downmixIdPresent;
    int downmixId;
    int eqApplyToDownmix;
    int additionalDownmixIdPresent;
    int additionalDownmixIdCount;
    int additionalDownmixId [ADDITIONAL_DOWNMIX_ID_COUNT_MAX];
    int drcSetId;
    int additionalDrcSetIdPresent;
    int additionalDrcSetIdCount;
    int additionalDrcSetId  [ADDITIONAL_DRC_SET_ID_COUNT_MAX];
    int eqSetPurpose;
    int dependsOnEqSetPresent;
    int dependsOnEqSet;
    int noIndependentEqUse;
    int eqChannelCount;
    int eqChannelGroupCount;
    int eqChannelGroupForChannel [CHANNEL_COUNT_MAX];
    int tdFilterCascadePresent;
    TdFilterCascade tdFilterCascade;
    int subbandGainsPresent;
    int subbandGainsIndex [EQ_CHANNEL_GROUP_COUNT_MAX];
    int eqTransitionDurationPresent;
    float eqTransitionDuration;
} EqInstructions;
#endif /* AMD1_SYNTAX */

typedef struct {
    int uniDrcConfigExtType[EXT_COUNT_MAX+1];
    int extBitSize[EXT_COUNT_MAX];
    
#if AMD1_SYNTAX
    /* UNIDRCCONFEXT_PARAM_DRC */
    int parametricDrcPresent;
    DrcCoefficientsParametricDrc drcCoefficientsParametricDrc;
    int parametricDrcInstructionsCount;
    ParametricDrcInstructions parametricDrcInstructions[PARAM_DRC_INSTRUCTIONS_COUNT_MAX];
    
    /* UNIDRCCONFEXT_V1 */
    int drcExtensionV1Present;
    int downmixInstructionsV1Present;
    int downmixInstructionsV1Count;
    DownmixInstructions downmixInstructionsV1[DOWNMIX_INSTRUCTIONS_COUNT_MAX];
    int drcCoeffsAndInstructionsUniDrcV1Present;
    int drcCoefficientsUniDrcV1Count;
    DrcCoefficientsUniDrc drcCoefficientsUniDrcV1 [DRC_COEFFICIENTS_UNIDRC_V1_COUNT_MAX];
    int drcInstructionsUniDrcV1Count;
    DrcInstructionsUniDrc drcInstructionsUniDrcV1 [DRC_INSTRUCTIONS_UNIDRC_V1_COUNT_MAX];
    int loudEqInstructionsPresent;
    int loudEqInstructionsCount;
    LoudEqInstructions loudEqInstructions [LOUD_EQ_INSTRUCTIONS_COUNT_MAX];
    int eqPresent;
    EqCoefficients eqCoefficients;
    int eqInstructionsCount;
    EqInstructions eqInstructions [EQ_INSTRUCTIONS_COUNT_MAX];
#endif /* AMD1_SYNTAX */
} UniDrcConfigExt;

typedef struct {
    int sampleRatePresent;
    int sampleRate;
    int downmixInstructionsCount;
    int drcCoefficientsUniDrcCount;
    int drcInstructionsUniDrcCount;
    int drcInstructionsCountPlus; /* includes (pseudo) DRC instructions for all configurations with DRC off */
    int drcDescriptionBasicPresent;
    int drcCoefficientsBasicCount;
    int drcInstructionsBasicCount;
    int uniDrcConfigExtPresent;
    UniDrcConfigExt uniDrcConfigExt;
    DrcCoefficientsBasic drcCoefficientsBasic[DRC_COEFF_COUNT_MAX];
    DrcInstructionsBasic drcInstructionsBasic[DRC_INSTRUCTIONS_COUNT_MAX];
    DrcCoefficientsUniDrc drcCoefficientsUniDrc[DRC_COEFF_COUNT_MAX];
    DrcInstructionsUniDrc drcInstructionsUniDrc[DRC_INSTRUCTIONS_COUNT_MAX];
    ChannelLayout channelLayout;
    DownmixInstructions downmixInstructions[DOWNMIX_INSTRUCTION_COUNT_MAX];
#if MPEG_H_SYNTAX
    int loudnessInfoSetPresent;
#endif
} UniDrcConfig;

#if AMD1_SYNTAX
typedef struct {
    int loudnessInfoV1AlbumCount;
    int loudnessInfoV1Count;
    LoudnessInfo loudnessInfoV1Album [LOUDNESS_INFO_COUNT_MAX];
    LoudnessInfo loudnessInfoV1      [LOUDNESS_INFO_COUNT_MAX];
} LoudnessInfoSetExtEq;
#endif /* AMD1_SYNTAX */
    
typedef struct {
    int loudnessInfoSetExtType[EXT_COUNT_MAX+1];
    int extBitSize[EXT_COUNT_MAX];
    
#if AMD1_SYNTAX
    LoudnessInfoSetExtEq loudnessInfoSetExtEq;
#endif /* AMD1_SYNTAX */
} LoudnessInfoSetExtension;

typedef struct {
    int loudnessInfoAlbumCount;
    int loudnessInfoCount;
    int loudnessInfoSetExtPresent;
    LoudnessInfo loudnessInfoAlbum[LOUDNESS_INFO_COUNT_MAX];
    LoudnessInfo loudnessInfo[LOUDNESS_INFO_COUNT_MAX];
    LoudnessInfoSetExtension loudnessInfoSetExtension;
} LoudnessInfoSet;

typedef struct {
    float gainDb;
    float slope;
    int time;
} Node;

typedef struct {
    int drcGainCodingMode;
    int nNodes;
    Node node[NODE_COUNT_MAX];
} SplineNodes;

#if AMD1_SYNTAX
typedef struct {
    SplineNodes splineNodes [1];
} DrcGainSequence;
#else /* !AMD1_SYNTAX */
typedef struct {
    int bandCount;
    SplineNodes splineNodes[BAND_COUNT_MAX];
} DrcGainSequence;
#endif /* AMD1_SYNTAX */
    
typedef struct {
    int uniDrcGainExtPresent;
    int uniDrcGainExtType[EXT_COUNT_MAX+1];
    int extBitSize[EXT_COUNT_MAX];
} UniDrcGainExt;

typedef struct {
    int nDrcGainSequences;
    DrcGainSequence drcGainSequence[SEQUENCE_COUNT_MAX];
    int uniDrcGainExtPresent;
    UniDrcGainExt uniDrcGainExt;
} UniDrcGain;
    
#ifdef __cplusplus
}
#endif
#endif
