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

Source file: TransSynth.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001	JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : Transient synthesiser.
 *
 *   Description         : This module is responsible for generationg meixner
 *                         type transient signals.
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All functions are prefixed by 'SSC_TRANSSYNTH_'
 *
 *   Functions           :
 *
 ******************************************************************************/
#ifndef TRANSSYNTH_H
#define TRANSSYNTH_H

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
typedef struct _SSC_TRANSSYNTH
{
    SInt dummy;
} SSC_TRANSSYNTH;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/

/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void            SSC_TRANSSYNTH_Initialise(void); 
void            SSC_TRANSSYNTH_Free(void); 

SSC_TRANSSYNTH* SSC_TRANSSYNTH_CreateInstance(void);
void            SSC_TRANSSYNTH_DestroyInstance(SSC_TRANSSYNTH* pTransSynth); 

void            SSC_TRANSSYNTH_GenerateMeixner(SSC_TRANSSYNTH*                    pTransSynth,
                                               const SSC_TRANSIENT_MEIXNER_PARAM* pMeixnerParam,
                                               SInt                               Length,
                                               SSC_SAMPLE_INT*                    pSampleBuffer); 

#ifdef __cplusplus
}
#endif

#endif /* TRANSSYNTH_H */
