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

Source file: NoiseSynth.h

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  m8540 improved noise coding + 8 sf/frame
15-Jul-2003 RJ  RM3a: Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Noise Synthesiser
 *
 *   Description         : This module is responsible for generating noise
 *                         using the ARMA model.
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All functions are prefixed by 'SSC_NOISESYNTH_' 
 *
 *   Functions           :
 *
 ******************************************************************************/
#ifndef  _NOISESYNTH_H
#define  _NOISESYNTH_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#ifdef _SSC_LIB_BUILD
#include "NoiseSynthP.h"
#else
typedef struct _SSC_NOISESYNTH SSC_NOISESYNTH;
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
void            SSC_NOISESYNTH_Initialise(void); 
void            SSC_NOISESYNTH_Free(void); 

SSC_NOISESYNTH* SSC_NOISESYNTH_CreateInstance(UInt FilterLength, int nVariableSfSize); 
void            SSC_NOISESYNTH_DestroyInstance(SSC_NOISESYNTH* pNoiseSynth); 

void            SSC_NOISESYNTH_ResetRandom(SSC_NOISESYNTH* pNoiseSynth,
                                           UInt32          Seed);

void            SSC_NOISESYNTH_ResetFilter(SSC_NOISESYNTH* pNoiseSynth);

void            SSC_NOISESYNTH_GenerateNoise(SSC_NOISESYNTH* pNoiseSynth,
                                SSC_SAMPLE_INT*              pSampleBuffer,
                                UInt                         Length);

void            SSC_NOISESYNTH_Filter(SSC_NOISESYNTH*        pNoiseSynth,
                                      SSC_NOISE_PARAM*       pNoiseParam,
                                      const SSC_SAMPLE_INT*  pInputSamples,
                                      SSC_SAMPLE_INT*        pOutputSamples,
                                      UInt                   Length);

void            SSC_NOISESYNTH_Generate(SSC_NOISESYNTH*        pNoiseSynth,
                                        SSC_NOISE_PARAM*       pNoiseParam,
                                        SSC_SAMPLE_INT*        pSampleBuffer,
                                        UInt                   Length,
                                        SSC_SAMPLE_INT*        pWindowGainEnvelope);


#ifdef __cplusplus
}
#endif

#endif /* _NOISESYNTH_H */
