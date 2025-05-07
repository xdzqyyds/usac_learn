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

#ifndef DRC_Tool_common_h
#define DRC_Tool_common_h


#define INCLUDE_DECODER_DEFINITIONS     0

/* ISOBMFF */
#define ISOBMFF_SYNTAX                  1

/* Corrigendum on Reference Software */
#define AMD2_COR1                       1
#define AMD2_COR2                       1
#define AMD2_COR3                       1

/* MPEG-D DRC AMD 1 */
#ifndef AMD1
#define AMD1
#endif
#ifdef AMD1
#define AMD1_SYNTAX                     1
#define AMD1_PARAMETRIC_LIMITER         1
#else
#define AMD1_SYNTAX                     0
#define AMD1_PARAMETRIC_LIMITER         0
#endif

#ifdef LEVELING_SUPPORT
#define UNIDRCCONFEXT_LEVELING        ( 0x4 )
#endif


/* MPEG-H 3DA */
#ifdef MPEG_H
#define MPEG_H_SYNTAX                   1
/* #define DEBUG_OUTPUT_FORMAT             1*/
#else
#define MPEG_H_SYNTAX                   0
#endif

#define DEBUG_NODES                     0
/* #define NODE_RESERVOIR              1*/

#if INCLUDE_DECODER_DEFINITIONS
#define DRC_GAIN_DEBUG_FILE             0   /*create a file with the DRC gains*/
#endif
#define DEBUG_BITSTREAM                 0   /*print bitstream bits written and read*/
#if INCLUDE_DECODER_DEFINITIONS
#define DEBUG_DRC_SELECTION             0   /* display information about selected DRC sets */
#endif

#define MAX_DRC_PAYLOAD_BYTES           2048
#define MAX_DRC_PAYLOAD_BITS            MAX_DRC_PAYLOAD_BYTES*8

#define SPEAKER_POS_COUNT_MAX           128
#define DOWNMIX_COEFF_COUNT_MAX         (32*32)  /* 128*128 reduced to save memory */
#define CHANNEL_COUNT_MAX               128
#define BAND_COUNT_MAX                  8   /*16    reduced to save memory*/
#define SEQUENCE_COUNT_MAX              8   /*64    reduced to save memory*/
#define MEASUREMENT_COUNT_MAX           16
#define DOWNMIX_INSTRUCTION_COUNT_MAX   16
#define DRC_COEFF_COUNT_MAX             8
#define DRC_INSTRUCTIONS_COUNT_MAX      (DOWNMIX_INSTRUCTION_COUNT_MAX + 16)  /*64    reduced to save memory*/
#define LOUDNESS_INFO_COUNT_MAX         (DOWNMIX_INSTRUCTION_COUNT_MAX + 16)  /*64    reduced to save memory*/
#define AUDIO_CODEC_FRAME_SIZE_MAX      2048 /*covers AAC and HE-AAC*/
#define DRC_CODEC_FRAME_SIZE_MAX        (AUDIO_CODEC_FRAME_SIZE_MAX/8)  /*assuming min sample rate of 8kHz*/
#define NODE_COUNT_MAX                  DRC_CODEC_FRAME_SIZE_MAX
#define CHANNEL_GROUP_COUNT_MAX         SEQUENCE_COUNT_MAX
#define ACTIVE_DRCSET_COUNT_MAX         4    /* but only 3 of them can be active! */
#define ADDITIONAL_DOWNMIX_ID_MAX       8

#define DELAY_MODE_REGULAR_DELAY        0
#define DELAY_MODE_LOW_DELAY            1
#define DELAY_MODE_DEFAULT              DELAY_MODE_REGULAR_DELAY

#define FEATURE_REQUEST_COUNT_MAX       10
#define EFFECT_TYPE_REQUEST_COUNT_MAX   10

#if INCLUDE_DECODER_DEFINITIONS
#define BITSTREAM_BUFFER_BYTE_SIZE      20000
#endif

#define PROC_COMPLETE                   1
#define UNEXPECTED_ERROR                2
#define PARAM_ERROR                     3
#define EXTERNAL_ERROR                  4

#if INCLUDE_DECODER_DEFINITIONS
#define UNDEFINED_LOUDNESS_VALUE        1000.0f
#define ID_FOR_ANY_DOWNMIX              0x7F
#endif

#define EXT_COUNT_MAX                   2

#define UNIDRCGAINEXT_TERM              0x0
#define UNIDRCLOUDEXT_TERM              0x0
#define UNIDRCCONFEXT_TERM              0x0

#if AMD1_SYNTAX

#define UNIDRCCONFEXT_PARAM_DRC         0x1
#define UNIDRCCONFEXT_V1                0x2
#define UNIDRCLOUDEXT_EQ                0x1

#define PARAM_DRC_INSTRUCTIONS_COUNT_MAX 8
#define PARAM_DRC_INSTANCE_COUNT_MAX     8
#define PARAM_DRC_TYPE_FF                0x0
#define PARAM_DRC_TYPE_FF_NODE_COUNT_MAX 9
#define PARAM_DRC_TYPE_FF_LEVEL_ESTIM_FRAME_COUNT_MAX 64

#ifdef AMD1_PARAMETRIC_LIMITER
#define PARAM_DRC_TYPE_LIM                    0x1
#define PARAM_DRC_TYPE_LIM_THRESHOLD_DEFAULT  (-1.f)
#define PARAM_DRC_TYPE_LIM_ATTACK_DEFAULT     5
#define PARAM_DRC_TYPE_LIM_RELEASE_DEFAULT    50
#endif

/* SUBBAND DOMAIN */
#define AUDIO_CODEC_SUBBAND_COUNT_MAX   256

#define SUBBAND_DOMAIN_MODE_OFF         0
#define SUBBAND_DOMAIN_MODE_QMF64       1
#define SUBBAND_DOMAIN_MODE_QMF71       2
#define SUBBAND_DOMAIN_MODE_STFT256     3

/* QMF64 */
#define AUDIO_CODEC_SUBBAND_COUNT_QMF64                     64
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64       64
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF64             320

/* QMF71 (according to ISO/IEC 23003-1:2007) */
#define AUDIO_CODEC_SUBBAND_COUNT_QMF71                     71
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71       64
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF71             320+384

/* STFT256 (according to ISO/IEC 23008-3:2015/AMD3) */
#define AUDIO_CODEC_SUBBAND_COUNT_STFT256                   256
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256     256
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_STFT256           256

#define TIME_DOMAIN     1
#define SUBBAND_DOMAIN  2

#endif /* AMD1_SYNTAX */

#define SLOPE_FACTOR_DB_TO_LINEAR       0.1151f               /* ln(10) / 20 */

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef bool
#define bool int
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#if INCLUDE_DECODER_DEFINITIONS
typedef struct {
    int     dynamicRangeMeasurementType;
    int     dynamicRangeRequestedIsRange;
    float   dynamicRangeRequested;
    float   dynamicRangeMinRequested;
    float   dynamicRangeMaxRequested;
    int     drcCharacteristicRequested;
    int     effectTypeRequestedTotalCount;
    int     effectTypeRequestedDesiredCount;
    int     effectTypeRequested[EFFECT_TYPE_REQUEST_COUNT_MAX];
} FeatureRequestData;

typedef struct {
    int featureRequestType;
    FeatureRequestData featureRequestData;
} DrcFeatureRequest;

typedef struct {
    int featureRequestCount;
    DrcFeatureRequest drcFeatureRequest[FEATURE_REQUEST_COUNT_MAX];
} DrcFeatureRequestSequence;

typedef struct {
    int nBandsSupported;
    DrcFeatureRequestSequence drcFeatureRequestSequence;
} RequestParams;

typedef struct {
    int   albumMode;
    int   loudnessDeviationMaxPresent;
    float loudnessDeviationMax;
    int   methodDefinitionRequested;
    int   measurementSystemRequested;
    int   preprocessingRequested;
    int   loudnessNormalizationGainDbMaxPresent;
    float loudnessNormalizationGainDbMax;
    int   externalLimiterPresent;
    float outputPeakLevelMax;
    int   targetLoudnessPresent;
    float targetLoudness;
    float deviceCutoffFreq;    /* lower cutoff frequency of playback system */
} LoudnessParamExtern;

typedef struct {
    float   compress;
    float   boost;
    int     downmixIdRequested;
    RequestParams requestParams;
    LoudnessParamExtern loudnessParamExtern;
} DecoderHostControlParameters;
#endif

#endif
