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

Source file: Merge.c

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
 *   Module              : Merge.c
 *
 *   Description         : Signal merge module             
 *
 *   Tools               : Microsoft Visual C++ v6.0
 *
 *   Target Platform     : ANSI C
 *
 *   Naming Conventions  : All functions begin SSC_MERGE_
 *
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <stdlib.h>
#include <assert.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Alloc.h"
#include "Merge.h"

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
 * Name        : SSC_MERGE_Initialise
 *
 * Description : Initialises merge module
 *
 * Parameters  : 
 *
 * Returns:    : 
 * 
 *****************************************************************************/

void           SSC_MERGE_Initialise(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_MERGE_Free
 *
 * Description : Frees merge module
 *
 * Parameters  : 
 *
 * Returns:    : 
 * 
 *****************************************************************************/

void           SSC_MERGE_Free(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_MERGE_CreateInstance
 *
 * Description : Creates merge instance
 *
 * Parameters  : 
 *
 * Returns:    : pMerge - pointer to merge instance
 * 
 *****************************************************************************/

SSC_MERGE*     SSC_MERGE_CreateInstance(void)
{
    SSC_MERGE* pMerge;

    pMerge = SSC_MALLOC(sizeof(SSC_MERGE), "SSC_MERGE Instance");

    return pMerge;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_MERGE_DestroyInstance 
 *
 * Description : Destroys merge instance
 *
 * Parameters  : pMerge - pointer to merge instance
 *
 * Returns:    : 
 * 
 *****************************************************************************/

void           SSC_MERGE_DestroyInstance(SSC_MERGE* pMerge)
{
	if (pMerge != NULL)
	{
		SSC_FREE(pMerge, "SSC_MERGE Instance");
	}
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_MERGE_MergeSignal
 *
 * Description : Sequentially merges signal components and handles channel 
 *               interleaving
 *
 * Parameters  : pMerge         - pointer to Merge instance
 *               Channel        - Channel index
 *               TotalChannels  - Total number of audio channels
 *               pInput         - input buffer
 *               Length         - length of buffer
 *               pOutput        - output buffer
 *
 * Returns:    : 
 * 
 *****************************************************************************/

void           SSC_MERGE_MergeSignal( SSC_MERGE*            pMerge,
                                      SInt                  Channel,
                                      SInt                  TotalChannels,
                                      const SSC_SAMPLE_INT* pInput,
                                      SInt                  Length,
                                      SInt                  Offset,
                                      SSC_SAMPLE_INT*       pOutput
				                    )
{
    SInt i;
    SInt j = Channel + Offset * TotalChannels;

    assert(pMerge);
    pMerge = pMerge ; /* suppress compiler warning */
    for (i=0; i<Length; i++)
    {
        pOutput[j] += pInput[i];
        j          += TotalChannels;
    }
}
