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

Source file: SynthCtrlP.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits,  Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
15-Jul-2003 RJ  RM3a : Added time/pitch scaling, command line based
05 Jan 2004 AG  RM5.0: Low complexity stereo added
************************************************************************/

/******************************************************************************
 *
 *   Module              : Synthesiser Control
 *
 *   Description         : The synthesiser control module initiates and 
 *                         co-ordinates the operations needed to synthesise
 *                         the output signal based on the information it
 *                         receives from the bit-stream parser. The synthesiser
 *                         control does not perform any signal synthesis and
 *                         signal processing functions itself; it delegates
 *                         these tasks to the appropriate modules. 
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  :
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#ifdef SSC_CONF_TOOL
#ifdef USE_AFSP
#include <libtsp.h>             /* AFsp audio file library */
#include <libtsp/AFpar.h>       /* AFsp audio file library - definitions */
#endif /* USE_AFSP */
#endif /* SSC_CONF_TOOL */
#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_BitStrm_Types.h"

#include "ct_polyphase_ssc.h"

#include "SscDec.h"
#include "BitstreamParser.h"
#include "SynthCtrl.h"
#include "SineSynth.h"
#include "NoiseSynth.h"
#include "StereoSynth.h"
#include "TransSynth.h"
#include "Merge.h"
#include "Window.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSC_INTERMEDIATE_BUFFER_SIZE  (6 * SSC_MAX_SUBFRAME_SAMPLES)
#define SSC_MAX_WINDOW_SIZE           (2 * SSC_MAX_SUBFRAME_SAMPLES)


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef struct _SSC_INSTANCES
{
    SSC_TRANSSYNTH* pTransSynth;
    SSC_SINESYNTH*  pSineSynth;
    SSC_NOISESYNTH* pNoiseSynth;
    SSC_WINDOW*     pWindow;
    SSC_SAMPLE_INT* pOverflow;
} SSC_INSTANCES;

typedef struct _SSC_SYNTHCTRL
{
    UInt             MaxChannels;
    SSC_INSTANCES    Instances[SSC_MAX_CHANNELS];

    struct SSC_ANA_FILTERBANK qmfAnalysisFilterBank;
    struct SSC_SYN_FILTERBANK qmfSynthFilterBankLeft;
    struct SSC_SYN_FILTERBANK qmfSynthFilterBankRight;

    SSC_MERGE*       pMerge;
    SSC_SAMPLE_INT*  pIntermediate;
    SSC_SAMPLE_INT*  pIntermediateNoise;
    
    SSC_SAMPLE_INT*  pMonoMixed;

    SSC_SAMPLE_INT*  pStereoInput;
    SSC_SAMPLE_INT*  pStereoOutput;
    UInt             nStereoOutputIndex;
    UInt             nStereoIdx;

    UInt*            pNumberOfSlots;
    SSC_SAMPLE_INT*  pStereoIntermediateAnalysisReal; 
    SSC_SAMPLE_INT*  pStereoIntermediateAnalysisImag;
    UInt             qmfNrSlots;
    SSC_SAMPLE_INT** qmfLeftReal;  
    SSC_SAMPLE_INT** qmfLeftImag; 
    SSC_SAMPLE_INT** qmfRightReal;
    SSC_SAMPLE_INT** qmfRightImag;

    UInt             WindowSize;
    SSC_SAMPLE_INT*  pWindowNoise;
    SSC_SAMPLE_INT*  pWindowSine;
    SSC_SAMPLE_INT*  pWindowBirthDeath;
     
    SSC_SAMPLE_INT*  pWindowGainEnvelope;
} SSC_SYNTHCTRL;

