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

Source file: TransSynth.c

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
 *   Module              : Transient synthesiser
 *
 *   Description         : Generates Meixner transients
 *
 *   Tools               : Microsoft Visual C++ v6.0
 *
 *   Target Platform     : ANSI C
 *
 *   Naming Conventions  : All function names are prefixed by SSC_TRANSSYNTH_
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Error.h"
#include "SSC_Alloc.h"
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
 * Name        : SSC_TRANSSYNTH_Initialise
 *
 * Description : Initialises the transient synthesiser module.
 *               This function must be called before the first instance of
 *               the transient synthesiser is created.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_TRANSSYNTH_Initialise(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_TRANSSYNTH_Free
 *
 * Description : Frees the transient synthesiser module, undoing the effects
 *               of the SSC_TRANSSYNTH_Initialise() function.
 *               This function must be called after the last instance of
 *               the transient synthesiser is destroyed.
 *
 * Parameters  :  -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_TRANSSYNTH_Free(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_TRANSSYNTH_CreateInstance
 *
 * Description : Creates an instance of the transient synthesiser.
 *
 * Parameters  : -
 *
 * Returns:    : Pointer to instance, or NULL if insufficient memory.
 * 
 *****************************************************************************/
SSC_TRANSSYNTH* SSC_TRANSSYNTH_CreateInstance(void)
{
    SSC_TRANSSYNTH* pTransSynth;

    pTransSynth = SSC_MALLOC(sizeof(SSC_TRANSSYNTH), "SSC_TRANSSYNTH Instance");

    return pTransSynth;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_TRANSSYNTH_DestroyInstance
 *
 * Description : Destroys an instance of the transient synthesiser.
 *
 * Parameters  : pTransSynth - Pointer to instance to destroy.
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_TRANSSYNTH_DestroyInstance(SSC_TRANSSYNTH* pTransSynth)
{
    if(pTransSynth != NULL)
    {
        SSC_FREE(pTransSynth, "SSC_TRANSSYNTH Instance");
    }
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_TRANSSYNTH_GenerateMeixner
 *
 * Description :  Generates a Meixner transient signal.
 *                The output signal is added to the signal already in the buffer.
 *
 * Parameters  :  pTransSynth    - Pointer to instance data.
 *
 *                pMeixnerParam  - Pointer to meixner parameters (input)
 *
 *                Length         - Number of samples to generate.
 *
 *                pSampleBuffer  - Pointer to buffer in which the samples
 *                                 are to be stored (output).
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_TRANSSYNTH_GenerateMeixner(SSC_TRANSSYNTH*                    pTransSynth,
                                    const SSC_TRANSIENT_MEIXNER_PARAM* pMeixnerParam,
                                    SInt                               Length,
                                    SSC_SAMPLE_INT*                    pSampleBuffer)
{
	SInt i,j ;
    Double chi,b,curr_value;
	Double amp, freq, phase;

	assert(pTransSynth);
    pTransSynth = pTransSynth ; /* suppress compiler warning */

	/* Clear pSampleBuffer */

	for (i=0; i<Length; i++)
	{
		pSampleBuffer[i]=0;
	}

	/* Generate/Sum sinusoids */

	for (j = 0; j < (SInt)pMeixnerParam->NrOfSinusoids; j++)
	{
		/* Init cos() parameters */
		amp   = pMeixnerParam->SineParam[j].Amplitude;
		freq  = pMeixnerParam->SineParam[j].Frequency;
		phase = pMeixnerParam->SineParam[j].Phase;

		for (i=0; i<Length; i++)
		{
            pSampleBuffer[i] += amp * cos( freq * i + phase );
		}
	}

	/* Generate envelope for transient(s) */

	chi = pMeixnerParam->chi;
	b  = pMeixnerParam->b;

	/* Init first value */ 
	curr_value = pow ((1 - chi*chi), 0.5*b) / pMeixnerParam -> a_max;
	
	/* Envelope calculated */
	for (i=0; i<Length; i++)
	{
		pSampleBuffer[i] *= curr_value;
		curr_value *= chi * sqrt((b+i)/(i+1));
	}
}
