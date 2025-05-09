/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
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

Copyright © 2002.

Source file: StereoSynth.h

Required libraries: <none>

Authors:
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
16-Sep-2002 TM  Initial version ( m8541 )
15-Jul-2003 RJ  RM3a: Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Stereo Synthesiser
 *
 *   Description         : This module is responsible for generating stereo from mono 
 *                         using the stereo parameters.
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All functions are prefixed by 'SSC_STEREOSYNTH_' 
 *
 *   Functions           :
 *
 ******************************************************************************/
#ifndef  _STEREOSYNTH_H
#define  _STEREOSYNTH_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#ifdef _SSC_LIB_BUILD
#include "StereoSynthP.h"
#else
typedef struct _SSC_STEREOSYNTH SSC_STEREOSYNTH;
#endif


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

SSC_STEREOSYNTH* SSC_STEREOSYNTH_CreateInstance(UInt BaseLayer, UInt    nVariableSfSize);
SSC_STEREOSYNTH* SSC_STEREOSYNTH_DestroyInstance(SSC_STEREOSYNTH* pms); 

void             SSC_STEREOSYNTH_Generate(SSC_STEREOSYNTH*  pms, 
                                          SSC_SAMPLE_INT*   pInput, 
                                          SSC_STEREO_PARAM* pStereoParam, 
                                          Bool              skipSf,
                                          SInt              pos, 
                                          SInt              updateRateIdx, 
                                          SSC_SAMPLE_INT*   pOutput,
                                          SInt*             numSamples,
                                          UInt              nVarSfSize);

#ifdef __cplusplus
}
#endif

#endif /* _STEREOSYNTH_H */
