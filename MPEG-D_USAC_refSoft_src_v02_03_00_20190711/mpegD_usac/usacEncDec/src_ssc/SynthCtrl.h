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

Source file: SynthCtrl.h

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001 JD  Initial version
15-Jul-2003 RJ  RM3a: Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Synthesiser Control
 *
 *   Description         :  
 *
 *   Tools               : Microsoft Visual C++
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All functions are prefixed by 'SSC_SYNTHCTRL_'
 *
 *   Functions           :
 *
 ******************************************************************************/
#ifndef  SYNTHCTRL_H
#define  SYNTHCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

#ifdef _SSC_LIB_BUILD
#include "SynthCtrlP.h"
#else
typedef struct _SSC_SYNTHCTRL SSC_SYNTHCTRL;
#endif

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void           SSC_SYNTHCTRL_Initialise(void); 
void           SSC_SYNTHCTRL_Free(void); 

SSC_SYNTHCTRL* SSC_SYNTHCTRL_CreateInstance(UInt MaxChannels, UInt BaseLayer, UInt nVariableSfSize); 

void           SSC_SYNTHCTRL_DestroyInstance(SSC_SYNTHCTRL* pSynthCtrl); 

void           SSC_SYNTHCTRL_Reset(SSC_SYNTHCTRL* pSynthCtrl, UInt nVariableSfSize);

SSC_ERROR      SSC_SYNTHCTRL_SynthesiseFrame(SSC_SYNTHCTRL*       pSynthCtrl,
                                             SSC_BITSTREAMPARSER* pBitstreamParser,
                                             SSC_SAMPLE_EXT*      pOutputBuffer,
                                             Double               PitchFactor);

void           SSC_SYNTHCTRL_SynthEnable(SSC_SYNTHCTRL* pSynthCtrl,
                                         UInt32         SynthEnable);
#ifdef __cplusplus
}
#endif

#endif /* SYNTHCTRL_H */
