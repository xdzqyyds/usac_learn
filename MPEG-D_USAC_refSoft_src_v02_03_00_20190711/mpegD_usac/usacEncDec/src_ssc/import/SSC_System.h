/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright  2001.

Source file: SSC_System.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
03 Dec 2001 JD      m7724 and m7725 change, new file version
25 Jul 2002 TM      RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM      RM2.0: m8541 parametric stereo coding
05 Nov 2002 ES      RM3.0: Laguerre added, num removed
21 Nov 2003 AG      RM4.0: ADPCM added
************************************************************************/


/*===========================================================================
 *
 *  Module          : SSC_System
 *
 *  File            : SSC_System.h
 *
 *  Description     : This header file defines the SSC system constants and types.
 *
 *  Target Platform : ANSI C
 *
 ============================================================================*/

#ifndef _SSC_SYSTEM_H
#define _SSC_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*       INCLUDES                                                            */
/*===========================================================================*/
#include "PlatformTypes.h"
#include "cexcept.h"

/*===========================================================================*/
/*       TYPE DEFINITIONS                                                    */
/*===========================================================================*/

define_exception_type(UInt32);

typedef struct exception_context SSC_EXCEPTION;

#define SSC_INIT_EXCEPTION { 0, 0, (UInt32)0 }
#define UNREFERENCED_PARM(x) ((x) = (x))

typedef struct _SSC_DECODER_LEVEL
{
  UInt  MaxNrOfSinusoids; /* Max simultaneous #sinusoids (excl Meixner)    */
  UInt  MaxDenOrder;      /* Maximum order of noise denominator polynomial */
  UInt  NrOfBits;         /* #bits for NrOfContinuations and NrOfBirths    */

} SSC_DECODER_LEVEL;

typedef enum _SSC_MODE
{
  SSC_MODE_MONO          = 0,
  SSC_MODE_STEREO        = 1,
  SSC_MODE_LCSTEREO      = 2,
  SSC_MODE_RESERVED1     = 3

} SSC_MODE;

typedef enum _SSC_MODE_EXT
{
  SSC_MODE_EXT_DUAL_MONO    = 0,
  SSC_MODE_EXT_PAR_STEREO   = 1,
  SSC_MODE_EXT_RESERVED3    = 2,
  SSC_MODE_EXT_RESERVED4    = 3

} SSC_MODE_EXT;

typedef enum _SSC_STEREO_LAYER
{
  SSC_STEREO_LAYER_BASE     = 0,
  SSC_STEREO_LAYER_EXT1     = 1,
  SSC_STEREO_LAYER_RESERVED3= 2,
  SSC_STEREO_LAYER_RESERVED4= 3

} SSC_STEREO_LAYER;

typedef enum _SSC_ENHANCEMENT
{
  SSC_ENHANCEMENT_CONT_PHASE = 0,
  SSC_ENHANCEMENT_ORIG_PHASE = 1

} SSC_ENHANCEMENT;

typedef enum _SSC_TRANSIENT_TYPE
{
  SSC_TRANSIENT_TYPE_STEP      = 0,
  SSC_TRANSIENT_TYPE_MEIXNER   = 1,
  SSC_TRANSIENT_TYPE_RESERVED0 = 2,
  SSC_TRANSIENT_TYPE_RESERVED1 = 3

} SSC_TRANSIENT_TYPE;

typedef enum _SSC_STEREO_WINDOW_TYPE
{
    SSC_STEREO_WINDOW_TYPE_NORMAL       = 0,
    SSC_STEREO_WINDOW_TYPE_START_FIX    = 1,
    SSC_STEREO_WINDOW_TYPE_START_VAR    = 2,
    SSC_STEREO_WINDOW_TYPE_START_INTERM = 3,
    SSC_STEREO_WINDOW_TYPE_STOP_INTERM  = 4,
    SSC_STEREO_WINDOW_TYPE_STOP_VAR     = 5,
    SSC_STEREO_WINDOW_TYPE_STOP_FIX     = 6

} SSC_STEREO_WINDOW_TYPE;

typedef enum _SSC_STEREO_QGRID
{
    SSC_STEREO_QGRID_FINE   = 0,
    SSC_STEREO_QGRID_COARSE = 1
} SSC_STEREO_QGRID;


/*===========================================================================*/
/*       CONSTANT DEFINITIONS                                                */
/*===========================================================================*/
#define SSC_PI                                      (3.14159265358979323846)

/* System constants - defines the capabilities of the decoder */
#define SSC_MAX_DECODER_LEVEL_MEDIUM       1 /* encoder/decoder level */
#define SSC_S_MAX_SIN_TRACKS_MEDIUM       60
#define SSC_N_MAX_DEN_ORDER_MEDIUM        24

#define SSC_ENCODER_MAX_SINS              60 /* maximum number of sinusoids used during encoding */
#define SSC_ENCODER_MAX_DEN               24 /* maximum number of denominators used during encoding */

#define SSC_ENCODER_MAX_STEREO_BINS       40 /* maximum number of bins used for the stereo part */

/* grid conversion factors (transient huffman parameters to finest
 * scale conversion */
#define ABS_FREQ_FINE_GRID_FACTOR          8
#define ABS_AMP_FINE_GRID_FACTOR           8
#define ABS_PHASE_FINE_GRID_FACTOR         1

/* max filter coefficients used for generating noise is the max of
 * SSC_N_MAX_NUM_ORDER_MEDIUM and SSC_N_MAX_DEN_ORDER_MEDIUM */
#define SSC_N_MAX_FILTER_COEFFICIENTS     SSC_N_MAX_DEN_ORDER_MEDIUM

#define SSC_N_MAX_LSF_ORDER               15

#ifdef SSC_DEC_LEVEL_MEDIUM
  #define SSC_MAX_DECODER_LEVEL           SSC_MAX_DECODER_LEVEL_MEDIUM
  #define SSC_S_MAX_SIN_TRACKS            SSC_S_MAX_SIN_TRACKS_MEDIUM
  #define SSC_N_MAX_DEN_ORDER             SSC_N_MAX_DEN_ORDER_MEDIUM
  #define SSC_N_MAX_POL_ORDER             SSC_N_MAX_DEN_ORDER
#else
#error SSC_System_Defs.h: SSC_DEC_LEVEL_XXX macro not defined or not supported.
#endif

#define SSC_MAX_SUBFRAMES               8
#define SSC_MAX_CHANNELS                2

#define SSC_T_MAX_MEIXNER_SINUSOIDS     8
#define SSC_T_MEIXNER_NROF_CHIS         4
#define SSC_T_MEIXNER_NROF_BS           4

#define SSC_FILE_VERSION_MIN            8
#define SSC_FILE_VERSION_MAX            8

/* System constants: quantisation parameters */
#define SSC_T_FREQUENCY_BASE            11.4
#define SSC_T_AMPLITUDE_BASE            1.1885
#define SSC_T_PHASE_NROF_STEPS          32
#define SSC_T_PHASE_ERROR               (SSC_PI / SSC_T_PHASE_NROF_STEPS)

#define SSC_S_FREQUENCY_BASE            91.2
#define SSC_S_AMPLITUDE_BASE            1.0218
#define SSC_S_PHASE_NROF_STEPS          32
#define SSC_S_PHASE_ERROR               (SSC_PI / SSC_S_PHASE_NROF_STEPS)

#define SSC_N_LAR_BITS                  9
#define SSC_N_LAR_DYN_RANGE             16.0
#define SSC_N_LAR_LEVELS                ((1 << SSC_N_LAR_BITS) - 2)
#define SSC_N_LAR_DELTA                 (SSC_N_LAR_DYN_RANGE / SSC_N_LAR_LEVELS)

#define SSC_N_LSF_BITS                  8
#define SSC_N_LSF_LEVELS                (1 << SSC_N_LSF_BITS)

/* Constants used for allocating buffers */
#define SSC_MAX_SUBFRAME_SAMPLES        384
#define SSC_MAX_FRAME_SIZE              (SSC_MAX_SUBFRAMES*SSC_MAX_SUBFRAME_SAMPLES)

/* stereo parameters quantisation constants */
#define SSC_IID_QGRID_SIZE_FINE         11
#define SSC_IID_QGRID_SIZE_COARSE       6
#define SSC_IID_QGRID_MAX_SIZE          SSC_IID_QGRID_SIZE_FINE
#define SSC_IID_QGRID_NUMBER            2

#define SSC_ITD_QGRID_NUMBER            2

#define SSC_RHO_QGRID_SIZE_FINE         8
#define SSC_RHO_QGRID_SIZE_COARSE       4
#define SSC_RHO_QGRID_MAX_SIZE          SSC_RHO_QGRID_SIZE_FINE
#define SSC_RHO_QGRID_NUMBER            2

/* Stereo */
#define OCS_SLOTS                       ( 24 )

#define OCS_NO_ENVELOPE_INDICES         ( 4 )
#define OCS_MAX_ENVELOPES               ( 4 )

/* frequency resolution constants */
#define OCS_NO_FREQ_RESOLUTIONS         ( 3 )
#define OCS_MAX_FREQ_RESOLUTION         ( 34 )
#define OCS_QUANTISATION_COARSE         ( 0 )
#define OCS_QUANTISATION_FINE           ( 1 )

#define SSC_OCS_MAX_IID                 ( OCS_MAX_FREQ_RESOLUTION )
#define SSC_OCS_MAX_ICC                 ( OCS_MAX_FREQ_RESOLUTION )
#define SSC_OCS_MAX_IPD                 ( OCS_MAX_FREQ_RESOLUTION/2 )
#define SSC_OCS_MAX_OPD                 ( OCS_MAX_FREQ_RESOLUTION/2 )

/* file header constants */
static const Char SSC_formChunk[]  = "FORM" ;
static const Char SSC_typeChunk[]  = "SSC " ;
static const Char SSC_verChunk[]   = "VER " ;
static const Char SSC_confChunk[]  = "CONF" ;
static const Char SSC_soundChunk[] = "SCHK" ;

static const UInt SSC_sample_freq[] =
{
        0,  44100,      0,      0,
        0,      0,      0,      0,
        0,      0,      0,      0,
        0,      0,      0,      0
};
static const UInt SSC_update_rate[] =
{
       0,
       0,
       0,
       0,
       384,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0
};

static const SSC_DECODER_LEVEL SSC_DecoderLevel[] =
{
  { 0,                           0,                          0},
  { SSC_S_MAX_SIN_TRACKS_MEDIUM, SSC_N_MAX_DEN_ORDER_MEDIUM, 6},
  { 0,                           0,                          0},
  { 0,                           0,                          0}
};

static const Double SSC_Chi[SSC_T_MEIXNER_NROF_CHIS][SSC_T_MEIXNER_NROF_BS] =
{
  {0.9688, 0.9685, 0.9683, 0.9681},
  {0.9763, 0.9756, 0.9750, 0.9744},
  {0.9839, 0.9827, 0.9817, 0.9807},
  {0.9914, 0.9898, 0.9884, 0.9870}
};

static const Double SSC_a_max[SSC_T_MEIXNER_NROF_CHIS][SSC_T_MEIXNER_NROF_BS] =
{
  {0.152713500109658 , 0.131630525645664 , 0.120142673294398 , 0.112550174511598 },
  {0.132843681407528 , 0.115639700421076 , 0.106510539071702 , 0.100663024431527 },
  {0.109279971016712 , 0.0971964875412947, 0.0909719057150294, 0.0872632874594248},
  {0.0797175749717262, 0.0744985442180281, 0.0723059623257423, 0.0715041477354716}
};

static const SInt SSC_StereoNumberFreqBins[] = 
{
    10,
    20,
    34,
    0
};

static const SInt SSC_StereoNumberITDFreqBins[4][4] = 
{
    {  3,  6,  8, 10},
    {  6, 11, 16, 20},
    { 11, 21, 31, 40},
    {  0,  0,  0,  0}
};

static const SInt SSC_StereoUpdateRateFactor[] =
{
    1,
    2,
    4,
    0
};

/* stereo parameters quantisation tables */
static const Double SSC_StereoIIDQuantGrid[SSC_IID_QGRID_NUMBER][SSC_IID_QGRID_MAX_SIZE] =
                      {{0,  2,  4,  6,  8, 10, 13, 16, 19, 22, 25},
                       {0,  4,  8, 13, 19, 25,  0,  0,  0,  0,  0}};

static const Double SSC_StereoITDQuantRange[SSC_RHO_QGRID_NUMBER] =
                      {128.0, 64.0};

static const Double SSC_StereoRHOQuantGrid[SSC_RHO_QGRID_NUMBER][SSC_RHO_QGRID_MAX_SIZE] =
                      {{1.0, 0.95, 0.9,  0.82, 0.75, 0.6, 0.3, 0.0},
                       {1.0, 0.9,  0.75, 0.3,  0.0,  0.0, 0.0, 0.0}};

#ifdef __cplusplus
}
#endif

#endif /* _SSC_SYSTEM_H */
