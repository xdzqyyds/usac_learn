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

Source file: SineSynth.c

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
 *   Module              : Sinusoidal synthesiser
 *
 *   Description         : Generates Sinusoidal Components
 *
 *   Tools               : Miscrosoft Visual C++ v6.0
 *
 *   Target Platform     : ANSI C
 *
 *   Naming Conventions  : All function names are prefixed by SSC_SINESYNTH_
 *
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Error.h"
#include "SSC_Alloc.h"
#include "BitstreamParser.h"
#include "SineSynth.h"
#include "NoiseSynth.h"
#include "TransSynth.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/


/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/


/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SINESYNTH_Initialise
 *
 * Description : Initialises the sinusoidal synthesiser module.
 *               This function must be called before the first instance of
 *               the sinusoidal synthesiser is created.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_SINESYNTH_Initialise(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SINESYNTH_Free
 *
 * Description : Frees the sinusoidal synthesiser module, undoing the effects
 *               of the SSC_SINESYNTH_Initialise() function.
 *               This function must be called after the last instance of
 *               the sinusoidal synthesiser is destroyed.
 *
 * Parameters  :  -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_SINESYNTH_Free(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SINESYNTH_CreateInstance
 *
 * Description : Creates an instance of the sinusoidal synthesiser.
 *
 * Parameters  : -
 *
 * Returns:    : Pointer to instance, or NULL if insufficient memory.
 * 
 *****************************************************************************/
SSC_SINESYNTH* SSC_SINESYNTH_CreateInstance(void)
{
    SSC_SINESYNTH* pSineSynth;

    pSineSynth = SSC_MALLOC(sizeof(SSC_SINESYNTH), "SSC_SINESYNTH Instance");

    return pSineSynth;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SINESYNTH_DestroyInstance
 *
 * Description : Destroys an instance of the sinusoidal synthesiser.
 *
 * Parameters  : pSineSynth - Pointer to instance to destroy.
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_SINESYNTH_DestroyInstance(SSC_SINESYNTH* pSineSynth)
{
    if(pSineSynth != NULL)
    {
        SSC_FREE(pSineSynth, "SSC_SINESYNTH Instance");
    }
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_SINESYNTH_Generate
 *
 * Description :  Generates a sinusoidal component signals.
 *                The output signal is added to the signal already in the buffer.
 *
 * Parameters  :  pSineSynth     - Pointer to instance data.
 *
 *                pSineParams    - Pointer to sinusoidal parameters (input)
 *                
 *                NrOfSineWaves  - Number of sinusoidal components to be generated.
 *
 *                Length         - Number of samples to generate.
 *
 *                pSampleBuffer  - Pointer to buffer in which the samples
 *                                 are to be stored (output).
 *
 * Returns:    : -
 * 
 *****************************************************************************/

void SSC_SINESYNTH_Generate( SSC_SINESYNTH*     pSineSynth,
                             SSC_SIN_PARAM*     pSineParams,
                             UInt               NrOfSineWaves,
	                         UInt               Length,
                             SSC_SAMPLE_INT*    pSampleBuffer)
{
	UInt   i,j ;
    Double amp, freq, phase ;        

    assert(pSineSynth);
    pSineSynth = pSineSynth ; /* suppress compiler warning */

	for (i=0; i<NrOfSineWaves; i++)
	{
		amp   = pSineParams[i].Amplitude;
		freq  = pSineParams[i].Frequency;
		phase = pSineParams[i].Phase;

		for (j=0; j<Length; j++)
		{
            pSampleBuffer[j] += amp * cos( freq * j + phase );
		}
	}
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_SINESYNTH_GeneratePoly
 *
 * Description :  Generates a sinusoidal component signals.
 *                The output signal is added to the signal already in the buffer.
 *
 * Parameters  :  pSineSynth        - Pointer to instance data.
 *
 *                pSineParams       - Pointer to sinusoidal parameters (input)
 *                
 *                NrOfSineWaves     - Number of sinusoidal components to be generated.
 *
 *                Length            - Number of samples to generate.
 *
 *                pSampleBufferLow  - Pointer to buffer in which the samples
 *                                    are to be stored (output). 
 *                                    First used as buffer to store all 
 *                                    frequencies < 400 Hz
 *
 *                pSampleBufferHigh - buffer to store all sines with 
 *                                    frequencies >= 400 Hz
 *
 *                After windowing the two samplebuffers the pSampleBufferHigh is added
 *                to the pSampleBufferLow.
 *
 * Returns:    : -
 * 
 *****************************************************************************/

void SSC_SINESYNTH_GeneratePoly( SSC_SINESYNTH*        pSineSynth,
                                 SSC_SIN_PARAM*        pSineParams,
                                 UInt                  NrOfSineWaves,
							     UInt                  Length,
                                 SSC_SAMPLE_INT*       pSampleBufferLow,
                                 SSC_SAMPLE_INT*       pSampleBufferHigh,
                                 UInt                  SamplingRate
                               )
{
	UInt            i,j;
    SSC_SAMPLE_INT* pBuf ;
	Double          amp, freq, phase ;        
    Double          dFreq400Rad = 400 * 2 * SSC_PI / SamplingRate ;

    /* NOTE: phase has been adjusted already. */

    assert(pSineSynth);
    pSineSynth = pSineSynth ; /* suppress compiler warning */

    /* zero order sinusoids */
	for (i=0; i<NrOfSineWaves; i++)
	{
		amp   = pSineParams[i].Amplitude;
		freq  = pSineParams[i].Frequency;
		phase = pSineParams[i].Phase;

        if( freq < dFreq400Rad )
            pBuf = pSampleBufferLow ;
        else
            pBuf = pSampleBufferHigh ;

		for (j=0; j<Length; j++)
		{
            pBuf[j] += amp * cos( freq * j + phase );
		}
	}
}

