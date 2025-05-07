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

Source file: SSC_BitStr_Types.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
25 Jul 2002 TM      RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM      RM2.0: m8541 parametric stereo coding
21 Nov 2003 AG      RM4.0: ADPCM added
05 Jan 2004 AG      RM5.0: Low complexity stereo added
************************************************************************/

/*===========================================================================
 *
 *  Module          : SSC_BitStrm_Types
 *
 *  File            : SSC_BitStrm_Types.h
 *
 *  Description     : This header file defines the SSC system types for bit
 *                    streaming of SSC-streams.
 *
 *  Target Platform : ANSI C
 *
 ============================================================================*/
#ifndef _SSC_BITSTRM_TYPES_H
#define _SSC_BITSTRM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include "PlatformTypes.h"
#include "SSC_System.h"

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _SSC_SIN_QPARAM
{
  UInt                  Frequency;
  UInt                  Amplitude;   /* zero order amplitude */
  SInt                  Phase;       /* zero order phase */

} SSC_SIN_QPARAM;

typedef struct _SSC_MEIXNER_QPARAM
{
  UInt                  t_b_par;
  UInt                  t_chi_par;
  UInt                  t_nrof_sin;
  SSC_SIN_QPARAM        SineParam[SSC_T_MAX_MEIXNER_SINUSOIDS];

} SSC_MEIXNER_QPARAM;

typedef struct _SSC_TRANSIENT_QPARAM
{
  Bool                  t_bPresent ;
  UInt                  t_loc;
  UInt                  t_type;

  union
  {
    SSC_MEIXNER_QPARAM	Meixner;
  } Parameters;

} SSC_TRANSIENT_QPARAM;

typedef struct _SSC_SIN_ADPCM_STATE
{
  Double       yz[2];	        /* Prediction memory of ADPCM quantiser     */
  Double       delta[5];      /* ADPCM quantiser boundaries 		      */
  Double       repr[4];       /* ADPCM quantiser represenation level      */
  Double       PQ;	          /* Birth phase  			                  */
  Double       FQ;	          /* Birth frequency		        		  */
  Double       Yphase;        /* Reconstructed unwrapped phase  	      */
  Double       Yphasefilt;    /* Reconstructed filtered unwrapped phase   */
  Double       Freq;          /* Reconstructed frequency                  */
  Double       Phase;         /* Reconstructed phase                      */
  Double       Freqfilt;      /* Filtered reconstructed frequency         */
} SSC_SIN_ADPCM_STATE;

typedef struct _SSC_SIN_ADPCM
{
  Bool                  bValid;         /* 2 bit are read from bit stream */
  SInt                  s_fr_ph;        /* 2 bit to be decoded */
  Bool                  bGridValid;     /* ADPCM grid is different from inital grid */
  UInt                  s_adpcm_grid;   /* ADPCM grid for refresh frame */
  SSC_SIN_ADPCM_STATE   s_adpcm_state;  /* ADPCM state for decoding index */

} SSC_SIN_ADPCM;

typedef struct _SSC_SIN_QSEGMENT
{
  SInt                  State;     /* used to store is_cont (and related state info) */
  UInt                  s_cont;    /* nrof subframes which will follow for this track */
  UInt                  s_length;  /* total length of sinus track in subframes */
  UInt                  s_trackID; /* unique ID of this track */
  SSC_SIN_QPARAM        Sin;

  UInt                  s_length_refresh; /* total length of sinus track in subframes after last refresh */
  SSC_SIN_ADPCM         s_adpcm;   /* ADPCM data */

} SSC_SIN_QSEGMENT;

typedef struct _SSC_SINUSOID_QPARAM
{
  UInt                  s_nrof_continuations;
  UInt                  s_nrof_births;

  UInt                  s_qgrid_freq;
  UInt                  s_qgrid_amp;

  SSC_SIN_QSEGMENT      SineParam[SSC_S_MAX_SIN_TRACKS];

} SSC_SINUSOID_QPARAM;

typedef struct _SSC_NOISE_QPARAM
{
  SInt                  n_lar_den[SSC_N_MAX_DEN_ORDER]; /* quantised LAR's */
  UInt                  n_env_gain;
  UInt                  n_pole_laguerre;
  UInt                  n_grid_laguerre;
  UInt                  n_lsf[SSC_N_MAX_LSF_ORDER];
  SInt                  n_lsf_overlap;
} SSC_NOISE_QPARAM;

typedef enum _SSC_NOISE_NUM
{
  SSC_NOISE_NUM_PREV      = 0,
  SSC_NOISE_NUM_CURRENT   = 1   /* num/den of current frame */

} SSC_NOISE_NUM;


typedef struct _SSC_STEREO_QPARAM
{
    Bool  header;

    Bool  iidEnable;
    Bool  iccEnable;
    Bool  pcaEnable;
    Bool  extEnable;
    Bool  ipdEnable;

    UInt  iidFreqRes;
    UInt  iccFreqRes;

    UInt  iidQuant;

    UInt  frameClass;
    UInt  frameSlotLength;

    UInt  envelopes;
    UInt  envBorder[OCS_MAX_ENVELOPES + 1];

    SInt  iid[OCS_MAX_ENVELOPES][OCS_MAX_FREQ_RESOLUTION];
    SInt  icc[OCS_MAX_ENVELOPES][OCS_MAX_FREQ_RESOLUTION];
    SInt  ipd[OCS_MAX_ENVELOPES][OCS_MAX_FREQ_RESOLUTION / 2];
    SInt  opd[OCS_MAX_ENVELOPES][OCS_MAX_FREQ_RESOLUTION / 2];

} SSC_STEREO_QPARAM;


#ifdef __cplusplus
}
#endif

#endif /* _SSC_BITSTRM_TYPES_H */
