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

Copyright © 2001.

Source file: SSC_SigProc_Types.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
28 Nov 2002 AR  RM3.0: Laguerre added, num removed
************************************************************************/

/*===========================================================================
 *
 *  Module         : SSC_SigProc_Types
 *  
 *  File           : SSC_SigProc_Types.h
 *  
 *  Author         : Aad Rijnberg, Philips ASA Lab / Sound Coding
 *
 *  Description    : This header file defines the SSC system types for signal
 *                   processing (absolute parameters).
 *
 * Target Platform : ANSI C
 *
 ============================================================================*/

#ifndef _SSC_SIGPROC_TYPES_H
#define _SSC_SIGPROC_TYPES_H

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
typedef Double SSC_SAMPLE_INT;      /* Internal sample format */

typedef struct _SSC_SIN_PARAM
{
  SSC_SAMPLE_INT        Frequency;
  SSC_SAMPLE_INT        Amplitude;   /* zero order amplitude */
  SSC_SAMPLE_INT        Phase;       /* zero order phase */

} SSC_SIN_PARAM;

typedef struct _SSC_TRANSIENT_MEIXNER_PARAM
{
  SSC_SAMPLE_INT        b;
  SSC_SAMPLE_INT        chi;
  SSC_SAMPLE_INT        a_max;
  UInt                  NrOfSinusoids;
  SSC_SIN_PARAM         SineParam[SSC_T_MAX_MEIXNER_SINUSOIDS];

} SSC_TRANSIENT_MEIXNER_PARAM;

typedef struct _SSC_TRANSIENT_PARAM
{
    Bool                bTransient;
    UInt                Position;
    UInt                Type;

    union
    {
        SSC_TRANSIENT_MEIXNER_PARAM Meixner; /* Meiner parameters */
    } Parameters;

} SSC_TRANSIENT_PARAM;

typedef struct _SSC_SIN_SEGMENT
{
  SInt              State;  /* used to store is_cont (and related state info) */
  UInt              s_cont; /* nrof subframes which will follow for this track */
  SSC_SIN_PARAM     Sin;

} SSC_SIN_SEGMENT;

typedef struct _SSC_SINUSOID_PARAM
{
  UInt                  Continuations;
  UInt                  Births;
  SSC_SIN_SEGMENT       SineParam[SSC_S_MAX_SIN_TRACKS];

} SSC_SINUSOID_PARAM;

typedef struct _SSC_NOISE_PARAM
{
  UInt                  DenOrder;
  SSC_SAMPLE_INT        Denominator[SSC_N_MAX_DEN_ORDER];
  SSC_SAMPLE_INT        EnvGain;
  SSC_SAMPLE_INT        PoleLaguerre;
  SSC_SAMPLE_INT        GridLaguerre;

  UInt                  LsfOrder;
  SSC_SAMPLE_INT        Lsf[SSC_N_MAX_LSF_ORDER];

} SSC_NOISE_PARAM;

typedef struct _SSC_STEREO_BASE_LAYER
{
    SSC_STEREO_WINDOW_TYPE  st_winType;
    
    Bool             st_bTranUsed;
    SInt             st_tranPosIdx;

    SSC_SAMPLE_INT   st_iidGlobal;
    SSC_SAMPLE_INT   st_itdGlobal;
    SSC_SAMPLE_INT   st_rhoGlobal;

    SSC_SAMPLE_INT   st_iidGlobalT;
    SSC_SAMPLE_INT   st_itdGlobalT;
    SSC_SAMPLE_INT   st_rhoGlobalT;

} SSC_STEREO_BASE_LAYER;

typedef struct _SSC_STEREO_EXT_LAYER1
{
    SSC_SAMPLE_INT   st_iid[SSC_ENCODER_MAX_STEREO_BINS];
    SSC_SAMPLE_INT   st_itd[SSC_ENCODER_MAX_STEREO_BINS];
    SSC_SAMPLE_INT   st_rho[SSC_ENCODER_MAX_STEREO_BINS];

    SSC_SAMPLE_INT   st_iidT[SSC_ENCODER_MAX_STEREO_BINS];
    SSC_SAMPLE_INT   st_itdT[SSC_ENCODER_MAX_STEREO_BINS];
    SSC_SAMPLE_INT   st_rhoT[SSC_ENCODER_MAX_STEREO_BINS];
} SSC_STEREO_EXT_LAYER1;

typedef struct _SSC_STEREO_PARAM
{
	SInt   st_numberBins;
	SInt   st_numberItdBins;

    SInt   st_qGridIid;
    SInt   st_qGridItd;
    SInt   st_qGridRho;

    SSC_STEREO_BASE_LAYER base;
    SSC_STEREO_EXT_LAYER1 ext1;

} SSC_STEREO_PARAM;


#ifdef __cplusplus
}
#endif

#endif /* _SSC_SIGPROC_TYPES_H */
