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

Copyright  2001-2002.

Source file: SynthCtrl.c

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001 JD  Initial version
04-Dec-2001 JD  Rounding of output samples added
04-Dec-2001 JD  Symmetric birth/death window created
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
28 Nov 2002 AR  RM3.0: Laguerre added, num removed
15-Jul-2003 RJ  RM3a : Added time/pitch scaling, command line based
21 Nov 2003 AG  RM4.0: ADPCM added
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
 *   Naming Conventions  : All function names are prefixed by SSC_SYNTHCTRL_
 *
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <string.h>
#include <assert.h>
#include <math.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_BitStrm_Types.h"
#include "SSC_Error.h"
#include "SSC_Alloc.h"

#include "SscDec.h"
#include "BitstreamParser.h"
#include "SynthCtrl.h"
#include "SineSynth.h"
#include "NoiseSynth.h"
#include "StereoSynth.h"
#include "TransSynth.h"
#include "Window.h"
#include "Merge.h"

#include "ct_psdec_ssc.h"

#ifdef SSC_CONF_TOOL
#ifdef USE_AFSP
#include <libtsp.h>             /* AFsp audio file library */
#include <libtsp/AFpar.h>       /* AFsp audio file library - definitions */
#endif /* USE_AFSP */
#endif /* SSC_CONF_TOOL */

#ifdef SSC_CONF_TOOL
#define	SSCAUBUFSIZE	(2*3072)
#ifdef I2R_LOSSLESS
char* SSCmonoFileName;
int   SSCmonoFileNameUsed;
#else
extern char* SSCmonoFileName;
extern int   SSCmonoFileNameUsed;
#endif
#ifdef USE_AFSP
AFILE*       SSCmonofd;
int          SSCmonooffset;
#else
FILE*        SSCmonofd;
#endif /* USE_AFSP */
#endif /* SSC_CONF_TOOL */

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSC_QMF_SUBFRAME_LENGTH         (NO_QMF_CHANNELS)
#define SSC_UPDATE_OCS_QMF              (2)

#define DELAY_QMF_ANA                   (4)
#define DELAY_QMF_SYN                   (5)
#define OCS_LOOKAHEAD                   (6)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/


/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static SSC_ERROR LOCAL_SynthesiseTransients(SSC_SYNTHCTRL*              pSynthCtrl,
                                            SSC_BITSTREAMPARSER*        pBitstreamParser,
                                            const SSC_BITSTREAM_HEADER* pHeader,
                                            SInt                        pTransPos[][SSC_MAX_SUBFRAMES+1],
                                            SSC_SAMPLE_EXT*             pOutput);

static SSC_ERROR LOCAL_SynthesiseNoise(SSC_SYNTHCTRL*              pSynthCtrl,
                                       SSC_BITSTREAMPARSER*        pBitstreamParser,
                                       const SSC_BITSTREAM_HEADER* pHeader,
                                       SSC_SAMPLE_EXT*             pOutputBuffer);


static SSC_ERROR LOCAL_SynthesiseSinusoids(SSC_SYNTHCTRL*           pSynthCtrl,
                                        SSC_BITSTREAMPARSER*        pBitstreamParser,
                                        const SSC_BITSTREAM_HEADER* pHeader,
                                        SInt                        pTransPos[][SSC_MAX_SUBFRAMES+1],
                                        SSC_SAMPLE_EXT*             pOutputBuffer,
                                        Double                      PitchFactor);

static SSC_ERROR LOCAL_SynthesiseStereo(SSC_SYNTHCTRL*              pSynthCtrl,
                                        SSC_BITSTREAMPARSER*        pBitstreamParser,
                                        const SSC_BITSTREAM_HEADER* pHeader,
                                        SSC_SAMPLE_EXT*             pInputBuffer,
                                        SSC_SAMPLE_EXT*             pOutputBuffer);

static void LOCAL_ClearBuffer(SSC_SAMPLE_INT* pBuffer,
                              UInt            Length);
 
static UInt LOCAL_SelectSinusoids(const SSC_SIN_SEGMENT* pSinSeg,
                                  UInt                   NrOfSinusoids,
                                  SSC_SIN_PARAM*         pSinParam,
                                  UInt                   Mask,
                                  UInt                   Equals,
                                  Double                 PitchFactor);

static void LOCAL_CreateWindowShapes(SSC_SYNTHCTRL* pSynthCtrl, UInt Size);

static const SSC_SAMPLE_INT* LOCAL_GetWindowSineBirth(SSC_SYNTHCTRL* pSynthCtrl, UInt Pos);
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineDeath(SSC_SYNTHCTRL* pSynthCtrl, UInt Pos);
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineStart(SSC_SYNTHCTRL* pSynthCtrl);
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineEnd(SSC_SYNTHCTRL* pSynthCtrl);

static const SSC_SAMPLE_INT* LOCAL_GetWindow(SSC_SYNTHCTRL*        pSynthCtrl,
                                             UInt                  Pos,
                                             const SSC_SAMPLE_INT* pWindowShape);

static void LOCAL_ComputeStartPhase(SSC_SIN_PARAM*  pSinParam,
                                    UInt            NrOfSinusoids,
                                    UInt            Length,
                                    Bool            bFirstHalve);

static void LOCAL_WindowAndMerge(SSC_SYNTHCTRL*        pSynthCtrl,
                                 const SSC_SAMPLE_INT* pWindowShape,
                                 SSC_SAMPLE_INT*       pInputBuffer,
                                 UInt                  Length,
                                 SSC_SAMPLE_EXT*       pOutputBuffer,
                                 UInt                  Offset,
                                 UInt                  Channel,
                                 UInt                  TotalChannels);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_Initialise
 *
 * Description : Initialise the synthesiser control module. This function must
 *               be called before the first instance of the synthesiser control
 *               module is created.
 *
 * Parameters  : -
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSC_SYNTHCTRL_Initialise(void)
{

}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_Free
 *
 * Description : Free the synthesiser control module. This function must be
 *               called after the last instance of the synthesiser control
 *               module has been destroyed.
 *
 * Parameters  : -
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSC_SYNTHCTRL_Free(void)
{
    
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_CreateInstance
 *
 * Description : Create an instance of the synthesiser control module. 
 *
 * Parameters  : MaxChannels     - The maximum number of channels that the 
 *                                 synthesiser control module should be able
 *                                 to handle.
 *
 *               BaseLayer       - Set to 1 when decoding base layer only
 *
 *               nVariableSfSize - Length of a subframe
 *
 * Returns     : Pointer to synthesiser control module instance, or NULL if
 *               insufficient memory.
 * 
 *****************************************************************************/
SSC_SYNTHCTRL* SSC_SYNTHCTRL_CreateInstance(UInt MaxChannels, UInt BaseLayer, UInt nVariableSfSize)
{
    SSC_SYNTHCTRL* pSynthCtrl;
    UInt           ch, sf;
    Bool           Failed;
    UInt           MaxSfSize, nNrSlots;

    UNREFERENCED_PARM(BaseLayer);

    /* Make mulitple of SSC_QMF_SUBFRAME_LENGTH for sub-frame size */ 
    MaxSfSize = (UInt)(((Double)nVariableSfSize/(Double)SSC_QMF_SUBFRAME_LENGTH)+1.0) *  SSC_QMF_SUBFRAME_LENGTH;
    nNrSlots  = 2* (MaxSfSize/NO_QMF_CHANNELS) * SSC_MAX_SUBFRAMES/SSC_UPDATE_OCS_QMF;

    /* Check input parameters */
    if(MaxChannels > SSC_MAX_CHANNELS)
    {
        return NULL;
    }

    /* Allocate instance data structure */
    pSynthCtrl = SSC_MALLOC(sizeof(SSC_SYNTHCTRL), "SSC_SYNTHCTRL Instance");

    /* Fill instance data structure */
    if(pSynthCtrl != NULL)
    {
        /* Clear instance data structure */
        memset(pSynthCtrl, 0, sizeof(SSC_SYNTHCTRL));
    
        Failed                    = False;
        pSynthCtrl->MaxChannels   = MaxChannels;
        pSynthCtrl->pMerge        = SSC_MERGE_CreateInstance();

        for(ch = 0; ch < MaxChannels; ch++)
        {
            /* Allocate overflow buffer & create module instances */ 
            pSynthCtrl->Instances[ch].pTransSynth = SSC_TRANSSYNTH_CreateInstance();
            pSynthCtrl->Instances[ch].pSineSynth  = SSC_SINESYNTH_CreateInstance();
            pSynthCtrl->Instances[ch].pNoiseSynth = SSC_NOISESYNTH_CreateInstance(SSC_N_MAX_FILTER_COEFFICIENTS + 1, nVariableSfSize);
            pSynthCtrl->Instances[ch].pWindow     = SSC_WINDOW_CreateInstance();
            pSynthCtrl->Instances[ch].pOverflow   = SSC_MALLOC(sizeof(SSC_SAMPLE_INT) * (6*nVariableSfSize) ,
                                                               "SSC_SYNTHCTRL - Overflow buffer");

            /* Check if all allocations and instance creations were successfull. */ 
            if((pSynthCtrl->Instances[ch].pTransSynth == NULL) ||
               (pSynthCtrl->Instances[ch].pSineSynth  == NULL) ||
               (pSynthCtrl->Instances[ch].pNoiseSynth == NULL) ||
               (pSynthCtrl->Instances[ch].pWindow     == NULL) ||
               (pSynthCtrl->Instances[ch].pOverflow   == NULL))
            {
                Failed = True;
                break;
            }
        }

        /* Create buffers for QMF-OCS */ 
        InitAnaFilterbank(&(pSynthCtrl->qmfAnalysisFilterBank));
        InitSynFilterbank(&(pSynthCtrl->qmfSynthFilterBankLeft),  False);
        InitSynFilterbank(&(pSynthCtrl->qmfSynthFilterBankRight), False);
        
        /* stereo instance defaults*/
        if ( Failed == False )
        {
            pSynthCtrl->pIntermediate = SSC_CALLOC((6*nVariableSfSize)*pSynthCtrl->MaxChannels, sizeof(*pSynthCtrl->pIntermediate), "SSC_SYNTHCTRL - intermediate");
            if ( pSynthCtrl->pIntermediate == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            pSynthCtrl->pIntermediateNoise = SSC_CALLOC((6*nVariableSfSize)*pSynthCtrl->MaxChannels, sizeof(*pSynthCtrl->pIntermediateNoise), "SSC_SYNTHCTRL - intermediate noise");
            if ( pSynthCtrl->pIntermediateNoise == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            /* create stereo intermediate buffer */
            pSynthCtrl->pNumberOfSlots = SSC_CALLOC(2 * SSC_UPDATE_OCS_QMF * sizeof(*pSynthCtrl->pNumberOfSlots), 1, "SSC_SYNTHCTRL - stereo intermediate");
            if ( pSynthCtrl->pNumberOfSlots == NULL )
            {
                Failed = True;
            }
        }
        
        if ( Failed == False )
        {
            pSynthCtrl->pStereoOutput = SSC_CALLOC(2 * 2 * SSC_MAX_SUBFRAMES * MaxSfSize * pSynthCtrl->MaxChannels*sizeof(*pSynthCtrl->pStereoOutput), 1, "SSC_SYNTHCTRL - stereo intermediate");
            if ( pSynthCtrl->pStereoOutput == NULL )
            {
                Failed = True;
            }
        }
        if ( Failed == False )
        {
            pSynthCtrl->pStereoInput = SSC_CALLOC(2 * 2 * SSC_MAX_SUBFRAMES * MaxSfSize * pSynthCtrl->MaxChannels*sizeof(*pSynthCtrl->pStereoOutput), 1, "SSC_SYNTHCTRL - stereo intermediate");
            if ( pSynthCtrl->pStereoInput == NULL )
            {
                Failed = True;
            }
        }
        pSynthCtrl->nStereoOutputIndex = 0;
        if ( Failed == False )
        {
            pSynthCtrl->pStereoIntermediateAnalysisReal = SSC_CALLOC((2 * SSC_MAX_SUBFRAMES * MaxSfSize)*sizeof(*pSynthCtrl->pStereoIntermediateAnalysisReal), 1, "SSC_SYNTHCTRL - stereo intermediate analysis real");
            if ( pSynthCtrl->pStereoIntermediateAnalysisReal == NULL )
            {
                Failed = True;
            }
        }
        if ( Failed == False )
        {
            pSynthCtrl->pStereoIntermediateAnalysisImag = SSC_CALLOC((2 * SSC_MAX_SUBFRAMES * MaxSfSize)*sizeof(*pSynthCtrl->pStereoIntermediateAnalysisImag), 1, "SSC_SYNTHCTRL - stereo intermediate analysis imag");
            if ( pSynthCtrl->pStereoIntermediateAnalysisImag == NULL )
            {
                Failed = True;
            }
        }
        if ( Failed == False )
        {            
            pSynthCtrl->qmfNrSlots = nNrSlots;
            
            pSynthCtrl->qmfLeftReal = SSC_CALLOC((2*nNrSlots)*sizeof(*pSynthCtrl->qmfLeftReal), 1, "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfLeftImag = SSC_CALLOC((2*nNrSlots)*sizeof(*pSynthCtrl->qmfLeftImag), 1, "SSC_SYNTHCTRL - stereo intermediate");
            if (( pSynthCtrl->qmfLeftReal == NULL) || (pSynthCtrl->qmfLeftImag == NULL))
            {
                Failed = True;
            }   
            for (sf = 0; (Failed == False) && sf < 2*nNrSlots; sf++)
            {
                pSynthCtrl->qmfLeftReal[sf] = SSC_CALLOC((SSC_QMF_SUBFRAME_LENGTH)*sizeof(*pSynthCtrl->qmfLeftReal[sf]), 1, "SSC_SYNTHCTRL - stereo intermediate");
                pSynthCtrl->qmfLeftImag[sf] = SSC_CALLOC((SSC_QMF_SUBFRAME_LENGTH)*sizeof(*pSynthCtrl->qmfLeftReal[sf]), 1, "SSC_SYNTHCTRL - stereo intermediate");
                if (( pSynthCtrl->qmfLeftReal[sf] == NULL) ||  ( pSynthCtrl->qmfLeftImag[sf] == NULL))
                {
                    Failed = True;
                }   
            }
        }
        if ( Failed == False )
        {
            pSynthCtrl->qmfRightReal = SSC_CALLOC((2*nNrSlots)*sizeof(*pSynthCtrl->qmfRightReal), 1, "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfRightImag = SSC_CALLOC((2*nNrSlots)*sizeof(*pSynthCtrl->qmfRightImag), 1, "SSC_SYNTHCTRL - stereo intermediate");
            if (( pSynthCtrl->qmfRightReal == NULL) || (pSynthCtrl->qmfRightImag == NULL))
            {
                Failed = True;
            }   
            for (sf = 0; (Failed == False) && sf < 2*nNrSlots; sf++)
            {
                pSynthCtrl->qmfRightReal[sf] = SSC_CALLOC((SSC_QMF_SUBFRAME_LENGTH)*sizeof(*pSynthCtrl->qmfRightReal[sf]), 1, "SSC_SYNTHCTRL - stereo intermediate");
                pSynthCtrl->qmfRightImag[sf] = SSC_CALLOC((SSC_QMF_SUBFRAME_LENGTH)*sizeof(*pSynthCtrl->qmfRightReal[sf]), 1, "SSC_SYNTHCTRL - stereo intermediate");
                if (( pSynthCtrl->qmfRightReal[sf] == NULL) ||  ( pSynthCtrl->qmfRightImag[sf] == NULL))
                {
                    Failed = True;
                }   
            }
        }

        if ( Failed == False )
        {
            /* create mono mixed */
            pSynthCtrl->pMonoMixed = SSC_CALLOC(2*SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof(*pSynthCtrl->pMonoMixed), "SSC_SYNTHCTRL - mono mixed");
            if ( pSynthCtrl->pMonoMixed == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            /* create sine window array */
            pSynthCtrl->pWindowSine = SSC_CALLOC(2 * nVariableSfSize, sizeof( *pSynthCtrl->pWindowSine ), "SSC_SYNTHCTRL - sine window");
            if ( pSynthCtrl->pWindowSine == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            /* create sine window array */
            pSynthCtrl->pWindowNoise = SSC_CALLOC(2 * nVariableSfSize, sizeof( *pSynthCtrl->pWindowSine ), "SSC_SYNTHCTRL - noise window");
            if ( pSynthCtrl->pWindowSine == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            /* create birth/death window array */
            pSynthCtrl->pWindowBirthDeath = SSC_CALLOC(4 * nVariableSfSize,  sizeof( *pSynthCtrl->pWindowBirthDeath ), "SSC_SYNTHCTRL - birth/death window");
            if ( pSynthCtrl->pWindowBirthDeath == NULL )
            {
                Failed = True;
            }
        }

        if ( Failed == False )
        {
            /* create envelope gain array */
            pSynthCtrl->pWindowGainEnvelope = SSC_CALLOC(6 * nVariableSfSize,  sizeof( *pSynthCtrl->pWindowBirthDeath ), "SSC_SYNTHCTRL - window envelope gain");
            if ( pSynthCtrl->pWindowGainEnvelope == NULL )
            {
                Failed = True;
            }
        }

#ifdef SSC_CONF_TOOL
        if ( Failed == False )
        {
            if (SSCmonoFileNameUsed)
            {
#ifdef USE_AFSP
                long nsamp,nchan;
                float srate;
        	    SSCmonofd = AFopenRead (SSCmonoFileName, &nsamp, &nchan, &srate, NULL );
        	    if (SSCmonofd == NULL)
            	{
            		fprintf(stderr,"Could not open audio-file <%s>\n",SSCmonoFileName);
            		Failed = True;
            	}
            	else if ( nchan != 1 )
            	{
            		fprintf(stderr,"Audio-file not mono <channels=%d>\n",nchan);
            		Failed = True;
            	}
            	else if ( srate != 44100 )
            	{
            		fprintf(stderr,"Audio-file not 44.1 kHz <fs=%8.2f Hz>\n",srate);
            		Failed = True;
            	}
                else
            	{
            	    /* reset sample offset */
            	    SSCmonooffset = 0;
            	}
#else
        	    SSCmonofd = fopen (SSCmonoFileName, "rb" );
#endif /* USE_AFSP */
            }
            else
            {
        	    SSCmonofd = NULL;
            }
        }
#endif /* SSC_CONF_TOOL */



        if ( Failed == False)
        {
            SSC_SYNTHCTRL_Reset(pSynthCtrl, nVariableSfSize);
            LOCAL_CreateWindowShapes(pSynthCtrl, 2*nVariableSfSize);
        }
        else
        {
            SSC_SYNTHCTRL_DestroyInstance(pSynthCtrl);
            pSynthCtrl = NULL;            
        }
    }

    return pSynthCtrl;
}

 /*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_DestroyInstance
 *
 * Description : Destroy an instance of the synthesiser control module. 
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSC_SYNTHCTRL_DestroyInstance(SSC_SYNTHCTRL* pSynthCtrl)
{
    UInt ch;
    UInt nNrSlots, sf;

    if(pSynthCtrl == NULL)
    {
        return;
    }

    /* Free channel buffers and module instances */
    for(ch = 0; ch < pSynthCtrl->MaxChannels; ch++)
    {
        if ( pSynthCtrl->Instances[ch].pTransSynth != NULL )
        {
            SSC_TRANSSYNTH_DestroyInstance(pSynthCtrl->Instances[ch].pTransSynth);
            pSynthCtrl->Instances[ch].pTransSynth = NULL;

        }

        if ( pSynthCtrl->Instances[ch].pSineSynth != NULL )
        {
            SSC_SINESYNTH_DestroyInstance(pSynthCtrl->Instances[ch].pSineSynth);
            pSynthCtrl->Instances[ch].pSineSynth = NULL;
        }
        
        if ( pSynthCtrl->Instances[ch].pNoiseSynth != NULL )
        {
            SSC_NOISESYNTH_DestroyInstance(pSynthCtrl->Instances[ch].pNoiseSynth);
            pSynthCtrl->Instances[ch].pNoiseSynth = NULL;
        }

        if ( pSynthCtrl->Instances[ch].pWindow != NULL )
        {
            SSC_WINDOW_DestroyInstance(pSynthCtrl->Instances[ch].pWindow);
            pSynthCtrl->Instances[ch].pWindow = NULL;
        }

        if(pSynthCtrl->Instances[ch].pOverflow != NULL)
        {
            SSC_FREE(pSynthCtrl->Instances[ch].pOverflow, "SSC_SYNTHCTRL - Overflow buffer");
            pSynthCtrl->Instances[ch].pOverflow = NULL;
        }
    }

    /* Common buffers and module instances */
    if ( pSynthCtrl->pStereoOutput != NULL )
    {
        SSC_FREE(pSynthCtrl->pStereoOutput, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->pStereoOutput = NULL;
    }

    if ( pSynthCtrl->pStereoInput != NULL )
    {
        SSC_FREE(pSynthCtrl->pStereoInput, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->pStereoInput = NULL;
    }

    if ( pSynthCtrl->pNumberOfSlots != NULL )
    {
        SSC_FREE(pSynthCtrl->pNumberOfSlots, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->pNumberOfSlots = NULL;
    }

    if ( pSynthCtrl->pStereoIntermediateAnalysisReal != NULL )
    {
        SSC_FREE(pSynthCtrl->pStereoIntermediateAnalysisReal, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->pStereoIntermediateAnalysisReal = NULL;
    }
    if ( pSynthCtrl->pStereoIntermediateAnalysisImag != NULL )
    {
        SSC_FREE(pSynthCtrl->pStereoIntermediateAnalysisImag, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->pStereoIntermediateAnalysisImag = NULL;
    }

    if ( pSynthCtrl->pIntermediate != NULL )
    {
        SSC_FREE(pSynthCtrl->pIntermediate, "SSC_SYNTHCTRL - intermediate");
        pSynthCtrl->pIntermediate = NULL;
    }

    if ( pSynthCtrl->pIntermediateNoise != NULL )
    {
        SSC_FREE(pSynthCtrl->pIntermediateNoise, "SSC_SYNTHCTRL - intermediate noise");
        pSynthCtrl->pIntermediateNoise = NULL;
    }

    nNrSlots = pSynthCtrl->qmfNrSlots;

    /* Free the allocated memory */
    for (sf = 0; sf < 2*nNrSlots; sf++)
    {
        if ( pSynthCtrl->qmfLeftReal[sf] != NULL )
        {
            SSC_FREE(pSynthCtrl->qmfLeftReal[sf] , "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfLeftReal[sf] = NULL;
        }
        if ( pSynthCtrl->qmfLeftImag[sf] != NULL )
        {
            SSC_FREE(pSynthCtrl->qmfLeftImag[sf] , "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfLeftImag[sf] = NULL;
        }
    }
    if ( pSynthCtrl->qmfLeftReal != NULL )
    {
        SSC_FREE(pSynthCtrl->qmfLeftReal,  "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->qmfLeftReal = NULL;
    }
    if ( pSynthCtrl->qmfLeftImag != NULL )
    {
        SSC_FREE(pSynthCtrl->qmfLeftImag,  "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->qmfLeftImag = NULL;
    }

    for (sf = 0; sf < 2*nNrSlots; sf++)
    {
        if ( pSynthCtrl->qmfRightReal[sf] != NULL )
        {
            SSC_FREE(pSynthCtrl->qmfRightReal[sf] , "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfRightReal[sf] = NULL;
        }
        if ( pSynthCtrl->qmfRightImag[sf] != NULL )
        {
            SSC_FREE(pSynthCtrl->qmfRightImag[sf] , "SSC_SYNTHCTRL - stereo intermediate");
            pSynthCtrl->qmfRightImag[sf] = NULL;
        }
    }
    if ( pSynthCtrl->qmfRightReal != NULL )
    {
        SSC_FREE(pSynthCtrl->qmfRightReal, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->qmfRightReal = NULL;
    }
    if ( pSynthCtrl->qmfRightImag != NULL )
    {
        SSC_FREE(pSynthCtrl->qmfRightImag, "SSC_SYNTHCTRL - stereo intermediate");
        pSynthCtrl->qmfRightImag = NULL;
    }


    if ( pSynthCtrl->pMonoMixed != NULL )
    {
        SSC_FREE(pSynthCtrl->pMonoMixed, "SSC_SYNTHCTRL - mono mixed");
        pSynthCtrl->pMonoMixed = NULL;
    }

    if ( pSynthCtrl->pWindowSine != NULL )
    {
        SSC_FREE(pSynthCtrl->pWindowSine, "SSC_SYNTHCTRL - sine window");
        pSynthCtrl->pWindowSine = NULL;
    }

    if ( pSynthCtrl->pWindowNoise != NULL )
    {
        SSC_FREE(pSynthCtrl->pWindowNoise, "SSC_SYNTHCTRL - noise window");
        pSynthCtrl->pWindowNoise = NULL;
    }

    if ( pSynthCtrl->pWindowBirthDeath != NULL )
    {
        SSC_FREE(pSynthCtrl->pWindowBirthDeath, "SSC_SYNTHCTRL - birth/death window");
        pSynthCtrl->pWindowBirthDeath = NULL;
    }

    if ( pSynthCtrl->pWindowGainEnvelope != NULL )
    {
        SSC_FREE(pSynthCtrl->pWindowGainEnvelope, "SSC_SYNTHCTRL - window envelope gain");
        pSynthCtrl->pWindowGainEnvelope = NULL;
    }

    if ( pSynthCtrl->pMerge != NULL )
    {
        SSC_MERGE_DestroyInstance(pSynthCtrl->pMerge);
        pSynthCtrl->pMerge = NULL;
    }

    /* Clear and free instance data structure */
    SSC_FREE(pSynthCtrl, "SSC_SYNTHCTRL Instance");
}


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_SynthesiseFrame
 *
 * Description : Synthesise one frame of audio based on the parameters
 *               retrieved from the bit-stream parser. 
 *
 * Parameters  : pSynthCtrl       - Pointer to synthesiser control instance.
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance
 *                                  containing the parameters to synthesise.
 *
 *               pOutputBuffer    - Pointer to buffer receiving the synthesised
 *                                  samples.
 *
 *               PitchFactor      - Scale factor for pitch
 *
 * Returns     : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSC_SYNTHCTRL_SynthesiseFrame(SSC_SYNTHCTRL*       pSynthCtrl,
                                        SSC_BITSTREAMPARSER* pBitstreamParser,
                                        SSC_SAMPLE_EXT*      pOutputBuffer,
                                        Double               PitchFactor)
{
    SSC_ERROR            ErrorCode = SSC_ERROR_OK;
    SSC_ERROR            Result;
    SSC_BITSTREAM_HEADER Header;
    UInt                 ch,i;
    SInt                 TransPos[SSC_MAX_CHANNELS][SSC_MAX_SUBFRAMES+1];
    SSC_SAMPLE_INT*      pOverflow;
    SSC_SAMPLE_INT*      pWorkBuffer;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(pBitstreamParser);
    assert(pOutputBuffer);

    /* Get header info */
    ErrorCode = SSC_BITSTREAMPARSER_GetHeader(pBitstreamParser, &Header);

    if((SSC_ERROR_CATAGORY(ErrorCode)    >  SSC_ERROR_WARNING) || 
       (SSC_ERROR_DESCRIPTION(ErrorCode) == SSC_ERROR_NOT_AVAILABLE))
    {
        return ErrorCode;
    }
    if( Header.nrof_subframes == 0 )
    {
        return SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_UPDATE_RATE;
    }

    if( (Header.frame_size/Header.nrof_subframes) > (UInt)Header.update_rate)
    {
        return SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_UPDATE_RATE;
    }

    /* synth in separate buffer */
    if ( (Header.mode == SSC_MODE_LCSTEREO) && (Header.mode_ext == SSC_MODE_EXT_PAR_STEREO) )
    {
        memcpy( pSynthCtrl->pMonoMixed, &pSynthCtrl->pMonoMixed[8*((UInt)Header.update_rate)], 8*((UInt)Header.update_rate) * sizeof(*pSynthCtrl->pMonoMixed));
        
        pWorkBuffer = &(pSynthCtrl->pMonoMixed[8*((UInt)Header.update_rate)]);

        /* Clear workbuffer */
        for(i = 0; i < Header.frame_size * Header.nrof_channels_bs; i++)
        {
            pWorkBuffer[i] = 0.0;
        }    
    }
    else
    {
        /* Clear output buffer */
        for(i = 0; i < Header.frame_size * Header.nrof_channels_out; i++)
        {
            pOutputBuffer[i] = 0.0;
        }    

        pWorkBuffer = pOutputBuffer;
    }

    /* Merge overflow signal from previous frame and clear overflow buffers */
    for(ch = 0; ch < Header.nrof_channels_bs; ch++)
    {
        pOverflow = pSynthCtrl->Instances[ch].pOverflow;

        SSC_MERGE_MergeSignal(pSynthCtrl->pMerge,
                              ch, Header.nrof_channels_bs,
                              pOverflow, 6*Header.update_rate,
                              0, pWorkBuffer);

        LOCAL_ClearBuffer(pOverflow, 6*Header.update_rate);
    }

    /* Synthesise transients */
    Result = LOCAL_SynthesiseTransients(pSynthCtrl,
                                        pBitstreamParser,
                                        &Header,
                                        TransPos,
                                        pWorkBuffer);

    if(SSC_ERROR_CATAGORY(Result) > SSC_ERROR_CATAGORY(ErrorCode))
    {
        ErrorCode = Result;
    }

    /* Synthesise noise */
    Result = LOCAL_SynthesiseNoise(pSynthCtrl,
                                   pBitstreamParser,
                                   &Header,
                                   pWorkBuffer);
    if(SSC_ERROR_CATAGORY(Result) > SSC_ERROR_CATAGORY(ErrorCode))
    {
        ErrorCode = Result;
    }

    /* Synthesise sinusoids */
    Result = LOCAL_SynthesiseSinusoids(pSynthCtrl,
                                       pBitstreamParser,
                                       &Header,
                                       TransPos,
                                       pWorkBuffer,
                                       PitchFactor);

    if(SSC_ERROR_CATAGORY(Result) > SSC_ERROR_CATAGORY(ErrorCode))
    {
        ErrorCode = Result;
    }

    /* if parametric stereo do stereo synthesis */
    if ( (Header.mode == SSC_MODE_LCSTEREO) && (Header.mode_ext == SSC_MODE_EXT_PAR_STEREO) ) 
    {
        Result = LOCAL_SynthesiseStereo(pSynthCtrl,
                                        pBitstreamParser,
                                        &Header,
                                        pSynthCtrl->pMonoMixed,
                                        pOutputBuffer);

    }

    if(SSC_ERROR_CATAGORY(Result) > SSC_ERROR_CATAGORY(ErrorCode))
    {
        ErrorCode = Result;
    }
    
    /* clip the result output samples to -32768 and 32767 */
    for( i=0; i<Header.frame_size * Header.nrof_channels_out ; i++ )
    {
        if( pOutputBuffer[i] > 32767.0 )
        {
            pOutputBuffer[i] = 32767.0 ;
        }
        else if( pOutputBuffer[i] < -32768.0 )
        {
            pOutputBuffer[i] = -32768.0 ;
        }
    }
    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_SYNTHCTRL_Reset
 *
 * Description : Reset the synthesiser control to its initial state. After
 *               executing reset function the synthesiser control is in the
 *               same state as if it had just been created. 
 *
 * Parameters  : pSynthCtrl      - Pointer to synthesiser control instance to reset.
 *
 *               nVariableSfSize - Length of a subframe
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSC_SYNTHCTRL_Reset(SSC_SYNTHCTRL* pSynthCtrl, UInt nVariableSfSize)
{
    UInt ch;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Reset buffers and synthesisers */
    for(ch = 0; ch < pSynthCtrl->MaxChannels; ch++)
    {
        UInt MaxSfSize, nNrSlots;
        UInt i;
            
        /* Make mulitple of SSC_QMF_SUBFRAME_LENGTH for sub-frame size */ 
        MaxSfSize = (UInt)(((Double)nVariableSfSize/(Double)SSC_QMF_SUBFRAME_LENGTH)+0.5) *  SSC_QMF_SUBFRAME_LENGTH;
  
        nNrSlots = pSynthCtrl->qmfNrSlots;

        LOCAL_ClearBuffer(pSynthCtrl->Instances[ch].pOverflow,
                  6*nVariableSfSize);

        LOCAL_ClearBuffer(pSynthCtrl->pStereoIntermediateAnalysisReal, 
                         2* SSC_MAX_SUBFRAMES * MaxSfSize );
        LOCAL_ClearBuffer(pSynthCtrl->pStereoIntermediateAnalysisImag, 
                         2* SSC_MAX_SUBFRAMES * MaxSfSize );
        LOCAL_ClearBuffer(pSynthCtrl->pStereoOutput, 
                         2* SSC_MAX_SUBFRAMES * MaxSfSize );
        LOCAL_ClearBuffer(pSynthCtrl->pStereoInput, 
                         2* SSC_MAX_SUBFRAMES * MaxSfSize );

        for (i=0; i < 2 * SSC_UPDATE_OCS_QMF; i++)
        {
             pSynthCtrl->pNumberOfSlots[i] = 0;
        }
        pSynthCtrl->nStereoOutputIndex = 0;
        
        for (i=0; i < 2*nNrSlots; i++)
        {
            LOCAL_ClearBuffer(pSynthCtrl->qmfLeftReal[i], SSC_QMF_SUBFRAME_LENGTH );
            LOCAL_ClearBuffer(pSynthCtrl->qmfLeftImag[i], SSC_QMF_SUBFRAME_LENGTH );
            LOCAL_ClearBuffer(pSynthCtrl->qmfRightReal[i], SSC_QMF_SUBFRAME_LENGTH );
            LOCAL_ClearBuffer(pSynthCtrl->qmfRightImag[i], SSC_QMF_SUBFRAME_LENGTH );
        }

        SSC_NOISESYNTH_ResetRandom(pSynthCtrl->Instances[ch].pNoiseSynth, ch);
        SSC_NOISESYNTH_ResetFilter(pSynthCtrl->Instances[ch].pNoiseSynth);
    }
}


/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_SynthesiseTransients
 *
 * Description : Synthesise transients.
 *
 * Parameters  : pSynthCtrl       - Pointer to synthesiser control instance.
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance 
 *                                  containing the transient data to synthesise.
 *
 *               pHeader          - Pointer to bit-stream header data.
 *                                  (strictly speaking this parameter is
 *                                  redundant because we already have the
 *                                  bit-stream parser instance, this parameter
 *                                  is included for efficiency)
 *
 *               pTransPos        - Pointer to array receiving the transient
 *                                  positions relative to sub-frame start.
 *                                  For sub-frame without transient this 
 *                                  parameter will be set to -1. (write-only)
 *
 *               pOutputBuffer    - Pointer to buffer to receive the output samples.
 *
 * Returns     : Error code.
 * 
 *****************************************************************************/
static SSC_ERROR LOCAL_SynthesiseTransients(SSC_SYNTHCTRL*              pSynthCtrl,
                                            SSC_BITSTREAMPARSER*        pBitstreamParser,
                                            const SSC_BITSTREAM_HEADER* pHeader,
                                            SInt                        pTransPos[][SSC_MAX_SUBFRAMES+1],
                                            SSC_SAMPLE_EXT*             pOutputBuffer)
{
    SSC_ERROR           ErrorCode;
    SSC_TRANSIENT_PARAM Transient;
    SSC_TRANSSYNTH*     pTransSynth;
    SSC_MERGE*          pMerge;
    SSC_SAMPLE_INT*     pIntermediate;
    SSC_SAMPLE_INT*     pOverflow;
    UInt                Ch, i, Length;
    UInt                NrOfTransients;
    UInt                s, sf_pos;
    SInt                Sf ;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(pBitstreamParser);
    assert(pHeader);
    assert(pOutputBuffer);

    ErrorCode      = SSC_ERROR_OK;
    pMerge         = pSynthCtrl->pMerge;
    /* pIntermediate  = pSynthCtrl->Intermediate; */
    pIntermediate  = pSynthCtrl->pIntermediate;

    s              = pHeader->update_rate;

    for(Ch = 0; Ch < pHeader->nrof_channels_bs; Ch++)
    {
        /* Clear sub-frame relative transients (1 parameter set extra) */
        for(Sf = 0; Sf <= (SInt)pHeader->nrof_subframes; Sf++)
        {
            pTransPos[Ch][Sf] = -1;
        }

        /* Obtain number of transients */
        NrOfTransients = SSC_BITSTREAMPARSER_GetNumberOfTransients(pBitstreamParser, Ch);

        pTransSynth    = pSynthCtrl->Instances[Ch].pTransSynth;
        pOverflow      = pSynthCtrl->Instances[Ch].pOverflow;

        for(i = 0; i < NrOfTransients; i++)
        {
            /* Get transient parameters */
            SSC_BITSTREAMPARSER_GetTransient(pBitstreamParser,
                                             Ch,
                                             i,
                                             &Sf,
                                             &Transient);
            assert( Sf != -2 );

            /* Process transient position */
            sf_pos = Transient.Position ;

            /* sf == 1 is now the first subframe of the current frame !!! */
            if(pTransPos[Ch][1+Sf] != -1)
            {
                /* More than one transient per subframe! */
                ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_T_LOC;
            }

            pTransPos[Ch][1+Sf]   = sf_pos;
            Length              = 3 * pHeader->update_rate - sf_pos;
            
            /* last subframe of previous frame, only pos interesting */
            /* no synthesise in this case */
            if( Sf != -1 ) 
            {
                /* Synthesise transient in intermediate buffer, if not step transient */
                switch(Transient.Type)
                {
                    case 0:     /*  Step transient */
                                break;

                    case 1:     /*  Meixner transient */
                                SSC_TRANSSYNTH_GenerateMeixner(pTransSynth,
                                                               &Transient.Parameters.Meixner,
                                                               Length,
                                                               pIntermediate);
                                break;

                    default:    /* Unknown: bit-stream parser should have detected this already */
                                assert(0);
                                ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_T_TYPE;
                                break;
                }
            
                /* Merge intermediate buffer into output buffer */
                if(Transient.Type != 0)
                {
                    UInt MergeLength, OverflowLength;
                    UInt tPosition ;

                    MergeLength    = Length;
                    OverflowLength = 0;
                    /* transient position now index in subframe instead of index in frame */
                    tPosition = Sf * pHeader->update_rate + Transient.Position ;

                    if((tPosition + Length) > pHeader->frame_size)
                    {
                        MergeLength    = pHeader->frame_size - tPosition;
                        OverflowLength = Length - MergeLength;
                    }

                    SSC_MERGE_MergeSignal(pMerge,
                                          Ch, pHeader->nrof_channels_bs,
                                          pIntermediate, MergeLength,
                                          tPosition, pOutputBuffer);

                    if(OverflowLength != 0)
                    {
                        SSC_MERGE_MergeSignal(pMerge,
                                              0, 1,
                                              pIntermediate + MergeLength, OverflowLength,
                                              0, pOverflow);
                    }
                } 
            }
        } /* for i */
    } /* for Ch */

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_SynthesiseNoise
 *
 * Description : Synthesise noise.
 *
 * Parameters  : pSynthCtrl       - Pointer to synthesiser control instance.
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance 
 *                                  containing the transient data to synthesise.
 *
 *               pHeader          - Pointer to bit-stream header data.
 *                                  (strictly speaking this parameter is
 *                                  redundant because we already have the
 *                                  bit-stream parser instance, this parameter
 *                                  is included for efficiency)
 *
 *               pTransPos        - Pointer to array containing the transient
 *                                  positions relative to sub-frame start.(read-only)
 *
 *               pOutputBuffer    - Pointer to buffer to receive the output samples.
 *                                  (update)
 *
 * Returns     : Error code.
 * 
 *****************************************************************************/
static SSC_ERROR LOCAL_SynthesiseNoise(SSC_SYNTHCTRL*              pSynthCtrl,
                                       SSC_BITSTREAMPARSER*        pBitstreamParser,
                                       const SSC_BITSTREAM_HEADER* pHeader,
                                       SSC_SAMPLE_EXT*             pOutputBuffer)
{
    SSC_ERROR       ErrorCode = SSC_ERROR_OK;
    UInt            ch, sf, Offset;
    SSC_SAMPLE_INT* pIntermediate;
    SSC_SAMPLE_INT* pIntermediateNoise;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(pBitstreamParser);
    assert(pHeader);
    assert(pOutputBuffer);

    pIntermediate      = pSynthCtrl->pIntermediate;
    pIntermediateNoise = pSynthCtrl->pIntermediateNoise;


    for(ch = 0; ch < pHeader->nrof_channels_bs; ch++)
    {
        SSC_NOISE_PARAM  NoiseParam;
        SSC_NOISESYNTH*  pNoiseSynth;

        Offset      = 0;
        pNoiseSynth = pSynthCtrl->Instances[ch].pNoiseSynth;

        for (sf = 0; sf < pHeader->nrof_subframes; sf += 2)
        {
            ErrorCode = SSC_BITSTREAMPARSER_GetNoise(pBitstreamParser, sf, ch, &NoiseParam);

            if ( (sf % 4) == 0 )
            {
                /* Synthesise last part of previous subframe (if any) */
                if(ErrorCode != (SSC_ERROR_WARNING | SSC_ERROR_NOT_AVAILABLE))
                {
                    SSC_NOISESYNTH_Generate(pNoiseSynth,
                                            &NoiseParam,
                                            pIntermediate,
                                            pHeader->update_rate,
                                            pSynthCtrl->pWindowGainEnvelope);
                                            /*pSynthCtrl->WindowGainEnvelope); */
                }
            }

            if(ErrorCode != (SSC_ERROR_WARNING | SSC_ERROR_NOT_AVAILABLE))
            {
                /* filter 2 subframes */
                SSC_NOISESYNTH_Filter(pNoiseSynth,
                                      &NoiseParam,
                                      &pIntermediate[(sf%4)*pHeader->update_rate],
                                      &pIntermediateNoise[(sf%4)*pHeader->update_rate],
                                      2*pHeader->update_rate);

                /* merge the signal into output buffer */
                
                SSC_MERGE_MergeSignal(pSynthCtrl->pMerge,
                                      ch, pHeader->nrof_channels_bs,
                                      &pIntermediateNoise[(sf%4)*pHeader->update_rate], 2 * pHeader->update_rate,
                                      Offset, pOutputBuffer);
                
            }

            /* Update sample offset */
            Offset += 2 * pHeader->update_rate;

        } /* for sf */
    } /* for ch */

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_SynthesiseSinuoids
 *
 * Description : Synthesise sinusoids.
 *
 * Parameters  : pSynthCtrl       - Pointer to synthesiser control instance.
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance 
 *                                  containing the transient data to synthesise.
 *
 *               pHeader          - Pointer to bit-stream header data.
 *                                  (strictly speaking this parameter is
 *                                  redundant because we already have the
 *                                  bit-stream parser instance, this parameter
 *                                  is included for efficiency)
 *
 *               pTransPos        - Pointer to array containing the transient
 *                                  positions relative to sub-frame start. (read-only)
 *
 *               pOutputBuffer    - Pointer to buffer to receive the output samples.
 *                                  (update)
 *
 *               PitchFactor      - Scaling factor for pitch
 *
 * Returns     : -
 * 
 *****************************************************************************/
static SSC_ERROR LOCAL_SynthesiseSinusoids(SSC_SYNTHCTRL*              pSynthCtrl,
                                           SSC_BITSTREAMPARSER*        pBitstreamParser,
                                           const SSC_BITSTREAM_HEADER* pHeader,
                                           SInt                        pTransPos[][SSC_MAX_SUBFRAMES+1],
                                           SSC_SAMPLE_EXT*             pOutputBuffer,
                                           Double                      PitchFactor)
{
    SSC_ERROR       ErrorCode = SSC_ERROR_OK;
    UInt            ch, sf, Offset;
    SSC_SAMPLE_INT* pIntermediate;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(pBitstreamParser);
    assert(pHeader);
    assert(pOutputBuffer);

    pIntermediate = pSynthCtrl->pIntermediate;

    for(ch = 0; ch < pHeader->nrof_channels_bs; ch++)
    {
        SSC_SIN_SEGMENT SinSeg[SSC_S_MAX_SIN_TRACKS];
        UInt            TotalSinusoids;
        SSC_SINESYNTH*  pSineSynth;
        SSC_WINDOW*     pWindow;

        Offset      = 0;
        pSineSynth  = pSynthCtrl->Instances[ch].pSineSynth;
        pWindow     = pSynthCtrl->Instances[ch].pWindow;

        /* Get sinusoid parameters of the last sub-frame in the previous frame */
        TotalSinusoids = SSC_BITSTREAMPARSER_GetSinusoids(pBitstreamParser, -1, ch, PitchFactor, SinSeg);

        for(sf = 0; sf < pHeader->nrof_subframes; sf++)
        {
            const SSC_SAMPLE_INT*  pStartShape;
            const SSC_SAMPLE_INT*  pEndShape;
            const SSC_SAMPLE_INT*  pDeathShape;
            const SSC_SAMPLE_INT*  pBirthShape;
            SSC_SIN_PARAM          SinParam[SSC_S_MAX_SIN_TRACKS];
            UInt                   SelectedSinusoids;
            SInt                   nCurTransPos = pTransPos[ch][1+sf] ;
             
            /* Determine window shapes */
            pStartShape = LOCAL_GetWindowSineStart(pSynthCtrl);
            pEndShape   = LOCAL_GetWindowSineEnd(pSynthCtrl);

            if( nCurTransPos >= 0 )
            {
                pBirthShape = LOCAL_GetWindowSineBirth(pSynthCtrl, nCurTransPos);
                pDeathShape = LOCAL_GetWindowSineDeath(pSynthCtrl, nCurTransPos);
            }
            else
            {
                pBirthShape = pStartShape;
                pDeathShape = pEndShape;
            }

            /* Synthesise deaths of previous sub-frame */
            SelectedSinusoids = LOCAL_SelectSinusoids(SinSeg, TotalSinusoids,
                                                      SinParam,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_DEATH,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_DEATH,
                                                      PitchFactor);

            if(SelectedSinusoids > 0)
            {
                LOCAL_ClearBuffer(pIntermediate, 2*pHeader->update_rate);
                LOCAL_ComputeStartPhase(SinParam, SelectedSinusoids, pHeader->update_rate, False );

                if( nCurTransPos >= 0 )
                {
                    SSC_SINESYNTH_GeneratePoly( pSynthCtrl->Instances[ch].pSineSynth,
                                                SinParam, SelectedSinusoids,
                                                pHeader->update_rate, 
                                                pIntermediate,                       /* low frequency (<400Hz) sinusoids) */
                                                &pIntermediate[pHeader->update_rate],/* high frequency sinusoids) */
                                                pHeader->sampling_rate); 

                    LOCAL_WindowAndMerge( pSynthCtrl,
                                          pEndShape,                               /* sine window */
                                          pIntermediate, pHeader->update_rate,
                                          pOutputBuffer, Offset,
                                          ch,            pHeader->nrof_channels_bs);
                    LOCAL_WindowAndMerge( pSynthCtrl,
                                          pDeathShape,                             /* transient birth window */
                                          &pIntermediate[pHeader->update_rate], pHeader->update_rate,
                                          pOutputBuffer, Offset,
                                          ch,            pHeader->nrof_channels_bs);
                }
                else
                {
                    SSC_SINESYNTH_Generate(pSynthCtrl->Instances[ch].pSineSynth,
                                           SinParam, SelectedSinusoids,
                                           pHeader->update_rate, pIntermediate);

                    LOCAL_WindowAndMerge(pSynthCtrl,
                                         pEndShape,
                                         pIntermediate, pHeader->update_rate,
                                         pOutputBuffer, Offset,
                                         ch,            pHeader->nrof_channels_bs);
                }
            }

            /* Synthesise births + continuing tracks of previous sub-frame */
            SelectedSinusoids = LOCAL_SelectSinusoids(SinSeg, TotalSinusoids,
                                                      SinParam,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_DEATH,
                                                      SSC_SINSTATE_VALID,
                                                      PitchFactor);
            if(SelectedSinusoids > 0)
            {
                LOCAL_ClearBuffer(pIntermediate, pHeader->update_rate);
                LOCAL_ComputeStartPhase(SinParam, SelectedSinusoids, pHeader->update_rate, False );

                SSC_SINESYNTH_Generate(pSynthCtrl->Instances[ch].pSineSynth,
                                       SinParam, SelectedSinusoids,
                                       pHeader->update_rate, pIntermediate);

                LOCAL_WindowAndMerge(pSynthCtrl,
                                     pEndShape,
                                     pIntermediate, pHeader->update_rate,
                                     pOutputBuffer, Offset,
                                     ch,            pHeader->nrof_channels_bs);
            }

            /* Get sine parameters of current sub-frame */
            TotalSinusoids = SSC_BITSTREAMPARSER_GetSinusoids(pBitstreamParser, sf, ch, PitchFactor, SinSeg);

            /* Synthesise births in current sub-frame */
            SelectedSinusoids = LOCAL_SelectSinusoids(SinSeg, TotalSinusoids,
                                                      SinParam,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_BIRTH,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_BIRTH,
                                                      PitchFactor);
            if(SelectedSinusoids > 0)
            {
                /* a temp buffer is also used this time, clear it */
                LOCAL_ClearBuffer(pIntermediate, 2*pHeader->update_rate);
                LOCAL_ComputeStartPhase(SinParam, SelectedSinusoids, pHeader->update_rate, True);

                if( nCurTransPos >= 0 )
                {
                    SSC_SINESYNTH_GeneratePoly( pSynthCtrl->Instances[ch].pSineSynth,
                                                SinParam, SelectedSinusoids,
                                                pHeader->update_rate, 
                                                pIntermediate,                       /* low frequency (<400Hz) sinusoids) */
                                                &pIntermediate[pHeader->update_rate],/* high frequency sinusoids) */
                                                pHeader->sampling_rate); 
                    
                    LOCAL_WindowAndMerge( pSynthCtrl,
                                          pStartShape,                               /* sine window */
                                          pIntermediate, pHeader->update_rate,
                                          pOutputBuffer, Offset,
                                          ch,            pHeader->nrof_channels_bs);
                    LOCAL_WindowAndMerge( pSynthCtrl,
                                          pBirthShape,                               /* transient birth window */
                                          &pIntermediate[pHeader->update_rate], pHeader->update_rate,
                                          pOutputBuffer, Offset,
                                          ch,            pHeader->nrof_channels_bs);
                }
                else
                {
                    SSC_SINESYNTH_Generate( pSynthCtrl->Instances[ch].pSineSynth,
                                            SinParam, SelectedSinusoids,
                                            pHeader->update_rate, pIntermediate);
                    LOCAL_WindowAndMerge( pSynthCtrl,
                                          pStartShape,
                                          pIntermediate, pHeader->update_rate,
                                          pOutputBuffer, Offset,
                                          ch,            pHeader->nrof_channels_bs);
                }
            }

            /* Synthesise death + continuations in current sub-frame */
            SelectedSinusoids = LOCAL_SelectSinusoids(SinSeg, TotalSinusoids,
                                                      SinParam,
                                                      SSC_SINSTATE_VALID | SSC_SINSTATE_BIRTH,
                                                      SSC_SINSTATE_VALID,
                                                      PitchFactor);
            if(SelectedSinusoids > 0)
            {
                LOCAL_ClearBuffer(pIntermediate, pHeader->update_rate);
                LOCAL_ComputeStartPhase(SinParam, SelectedSinusoids, pHeader->update_rate, True);

                SSC_SINESYNTH_Generate(pSynthCtrl->Instances[ch].pSineSynth,
                                       SinParam, SelectedSinusoids,
                                       pHeader->update_rate, pIntermediate);
                LOCAL_WindowAndMerge(pSynthCtrl,
                                     pStartShape,
                                     pIntermediate, pHeader->update_rate,
                                     pOutputBuffer, Offset,
                                     ch,            pHeader->nrof_channels_bs);
            }
            /* Update sample offset */
            Offset += pHeader->update_rate;
        } /* for sf */
    } /* for ch */

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_SynthesiseStereo
 *
 * Description : Synthesise strereo.
 *
 * Parameters  : pSynthCtrl       - Pointer to synthesiser control instance.
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance 
 *                                  containing the transient data to synthesise.
 *
 *               pHeader          - Pointer to bit-stream header data.
 *                                  (strictly speaking this parameter is
 *                                  redundant because we already have the
 *                                  bit-stream parser instance, this parameter
 *                                  is included for efficiency)
 *
 *               pOutputBuffer    - Pointer to buffer to receive the output samples.
 *                                  (update)
 *
 * Returns     : -
 * 
 *****************************************************************************/
static SSC_ERROR LOCAL_SynthesiseStereo(SSC_SYNTHCTRL*              pSynthCtrl,
                                        SSC_BITSTREAMPARSER*        pBitstreamParser,
                                        const SSC_BITSTREAM_HEADER* pHeader,
                                        SSC_SAMPLE_EXT*             pInputBuffer,
                                        SSC_SAMPLE_EXT*             pOutputBuffer)
{
    SSC_ERROR       ErrorCode = SSC_ERROR_OK;
    UInt            sf, Offset;
    UInt            NrSamples[2 * SSC_UPDATE_OCS_QMF];
    UInt            i, rem;
    UInt            nNumberSlots;
    UInt            NrSamples1, NrSamples2;
    UInt            NrSlots1, NrSlots2;
    UInt            NrOutputSamples;
    UInt            SamplesAvailable;
    Double          dSlots;

    /* Initialise arrays */
    SSC_SAMPLE_EXT OutputLeft[SSC_QMF_SUBFRAME_LENGTH];
    SSC_SAMPLE_EXT OutputRight[SSC_QMF_SUBFRAME_LENGTH];

    SSC_SAMPLE_EXT    **ocsLeftReal;
    SSC_SAMPLE_EXT    **ocsLeftImag;
    SSC_SAMPLE_EXT    **ocsRightReal;
    SSC_SAMPLE_EXT    **ocsRightImag;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(pBitstreamParser);
    assert(pHeader);
    assert(pInputBuffer);

    UNREFERENCED_PARM(pHeader);

    ocsLeftReal  = pSynthCtrl->qmfLeftReal;
    ocsLeftImag  = pSynthCtrl->qmfLeftImag;
    ocsRightReal = pSynthCtrl->qmfRightReal;
    ocsRightImag = pSynthCtrl->qmfRightImag;

    NrOutputSamples = pHeader->update_rate * SSC_MAX_SUBFRAMES;

    /* Determine number of slots for frame */
    if (pBitstreamParser->State != FirstFrame)
    {
        SamplesAvailable = 
           2 * NrOutputSamples 
           - pSynthCtrl->nStereoOutputIndex
           - pSynthCtrl->pNumberOfSlots[0] * SSC_QMF_SUBFRAME_LENGTH
           - pSynthCtrl->pNumberOfSlots[1] * SSC_QMF_SUBFRAME_LENGTH;
    }
    else
    {
        SamplesAvailable = NrOutputSamples;
        pSynthCtrl->pNumberOfSlots[0] = pSynthCtrl->qmfNrSlots/2;
        pSynthCtrl->pNumberOfSlots[1] = pSynthCtrl->qmfNrSlots/2;
    }

    rem = (UInt)fmod((Double)SamplesAvailable, (Double)SSC_QMF_SUBFRAME_LENGTH); 
    if (rem != 0)
    {
        SamplesAvailable += (SSC_QMF_SUBFRAME_LENGTH - rem);
    }    
    dSlots = (Double)SamplesAvailable/(Double)SSC_QMF_SUBFRAME_LENGTH;
    
    /* Check for even or odd number of slots */
    if ((UInt)fmod(dSlots, 2) == 0)
    {
        pSynthCtrl->pNumberOfSlots[2] = ((UInt)dSlots)/2;   /* Slot 3 */
        pSynthCtrl->pNumberOfSlots[3] = ((UInt)dSlots)/2;   /* Slot 4 */
    }
    else
    {
        pSynthCtrl->pNumberOfSlots[2] = ((UInt)dSlots-1)/2;   /* Slot 3 */
        pSynthCtrl->pNumberOfSlots[3] = ((UInt)dSlots+1)/2;   /* Slot 4 */
    }

    nNumberSlots = 0;
    for (i = 0; i < 2 * SSC_UPDATE_OCS_QMF; i++)
    {
         NrSamples[i] = pSynthCtrl->pNumberOfSlots[i] * SSC_QMF_SUBFRAME_LENGTH;
         nNumberSlots += pSynthCtrl->pNumberOfSlots[i];
    }
    /* Determine number of samples for first and second frame */
    NrSamples1 = NrSamples[0]+NrSamples[1];
    NrSlots1   = pSynthCtrl->pNumberOfSlots[0] + pSynthCtrl->pNumberOfSlots[1];
    NrSamples2 = NrSamples[2]+NrSamples[3];
    NrSlots2   = pSynthCtrl->pNumberOfSlots[2] + pSynthCtrl->pNumberOfSlots[3];
    if (pBitstreamParser->State == FirstFrame)
    {
        pSynthCtrl->nStereoIdx = NrSamples1; 
    }

    /* Copy new input samples into buffer */
    for (i = 0; i < NrOutputSamples; i++)
    {
        /* Get input samples to use in QMF analysis */
        pSynthCtrl->pStereoInput[pSynthCtrl->nStereoIdx + i] = pInputBuffer[NrOutputSamples + i];
    }
    pSynthCtrl->nStereoIdx += NrOutputSamples;
    
#ifdef SSC_CONF_TOOL
    if (SSCmonofd)
    {
        UInt i, Nread;
#ifdef USE_AFSP
        float   buffer[SSCAUBUFSIZE];
        Nread=AFreadData (SSCmonofd,SSCmonooffset,buffer,NrOutputSamples);
        SSCmonooffset += Nread;
#else
        short   buffer[SSCAUBUFSIZE];
        Nread = fread (buffer,2,NrOutputSamples,SSCmonofd);
#endif /* USE_AFSP */

        for (i = 0; i < Nread; i++)
        {
            /* Overwrite input samples to use in QMF analysis */
            pSynthCtrl->pStereoInput[pSynthCtrl->nStereoIdx - NrOutputSamples + i] = buffer[i];
        }
        if ( Nread < NrOutputSamples )
        {
            for (i = Nread; i < NrOutputSamples; i++)
                pSynthCtrl->pStereoInput[pSynthCtrl->nStereoIdx - NrOutputSamples + i] = 0.0;
        }
    }
#endif /* SSC_CONF_TOOL */

    /* QMF Analysis: put output of QMF analysis bank in slot 3 and 4 */    
    if (pBitstreamParser->State != FirstFrame)
    {
        /* Filter the last subframe */
        Offset = (NrSlots1-1) * SSC_QMF_SUBFRAME_LENGTH;
        /* qmf analysis */

        CalculateAnaFilterbank( &(pSynthCtrl->qmfAnalysisFilterBank), 
            &pSynthCtrl->pStereoInput[Offset], 
            &(pSynthCtrl->pStereoIntermediateAnalysisReal[Offset]),
            &(pSynthCtrl->pStereoIntermediateAnalysisImag[Offset]));       
    }

    Offset = NrSamples1;
    for (sf = 0; sf < NrSlots2-1; sf++)
    {
        SSC_SAMPLE_EXT  *Input;

        /* Get input samples to use in QMF analysis */
        Input =  &(pSynthCtrl->pStereoInput[Offset + sf * SSC_QMF_SUBFRAME_LENGTH]);

        /* qmf analysis */
        CalculateAnaFilterbank( &(pSynthCtrl->qmfAnalysisFilterBank), Input, 
            &(pSynthCtrl->pStereoIntermediateAnalysisReal[Offset+sf * SSC_QMF_SUBFRAME_LENGTH]),
            &(pSynthCtrl->pStereoIntermediateAnalysisImag[Offset+sf * SSC_QMF_SUBFRAME_LENGTH]));
                                

    }

    /* Decode OCS parameters for slot 2 */ 
    if (pBitstreamParser->State != FirstFrame)
    {
        /* Decode OCS parameters for slot 2 */ 
        Offset = (pSynthCtrl->pNumberOfSlots[0] + DELAY_QMF_ANA) * SSC_QMF_SUBFRAME_LENGTH;
        for (sf = 0; sf < pSynthCtrl->pNumberOfSlots[1] + OCS_LOOKAHEAD; sf++)  /*  look ahead for input OCS */
        {
            memcpy( ocsLeftReal[pSynthCtrl->pNumberOfSlots[0] + sf], &(pSynthCtrl->pStereoIntermediateAnalysisReal[Offset + sf * SSC_QMF_SUBFRAME_LENGTH]), 
                 SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftReal[pSynthCtrl->pNumberOfSlots[0] + sf][0]));
            memcpy( ocsLeftImag[pSynthCtrl->pNumberOfSlots[0] + sf], &(pSynthCtrl->pStereoIntermediateAnalysisImag[Offset + sf * SSC_QMF_SUBFRAME_LENGTH]), 
                 SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftImag[pSynthCtrl->pNumberOfSlots[0] + sf][0]));
        }

        SetNumberOfSlots(pBitstreamParser->h_PS_DEC, pSynthCtrl->pNumberOfSlots[1]);

        Offset = pSynthCtrl->pNumberOfSlots[0];
        ApplyPsFrame(pBitstreamParser->h_PS_DEC,
                     &(ocsLeftReal[Offset]),  &(ocsLeftImag[Offset]), 
                     &(ocsRightReal[Offset]), &(ocsRightImag[Offset]) );
    }
    else
    {
        /* The OCS decoder has to be called to shift parameter pipe */
        Offset = pSynthCtrl->pNumberOfSlots[0];
        ApplyPsFrame(pBitstreamParser->h_PS_DEC,
                     &(ocsLeftReal[Offset]),  &(ocsLeftImag[Offset]), 
                     &(ocsRightReal[Offset]), &(ocsRightImag[Offset]) );
    }


    /* Decode OCS parameters for slot 3 */ 
    Offset = (NrSlots1 + DELAY_QMF_ANA) * SSC_QMF_SUBFRAME_LENGTH;
    for (sf = 0; sf < pSynthCtrl->pNumberOfSlots[2] + OCS_LOOKAHEAD; sf++)  /*  look ahead for input OCS */
    {
        memcpy( ocsLeftReal[NrSlots1 + sf], &(pSynthCtrl->pStereoIntermediateAnalysisReal[Offset + sf * SSC_QMF_SUBFRAME_LENGTH]), 
             SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftReal[NrSlots1 + sf][0]));
        memcpy( ocsLeftImag[NrSlots1 + sf], &(pSynthCtrl->pStereoIntermediateAnalysisImag[Offset + sf * SSC_QMF_SUBFRAME_LENGTH]), 
             SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftImag[NrSlots1 + sf][0]));
    }

    SetNumberOfSlots(pBitstreamParser->h_PS_DEC, pSynthCtrl->pNumberOfSlots[2]);

    ApplyPsFrame( pBitstreamParser->h_PS_DEC,
                  &(ocsLeftReal[NrSlots1]),  &(ocsLeftImag[NrSlots1]), 
                  &(ocsRightReal[NrSlots1]), &(ocsRightImag[NrSlots1]));

    /* Check for first frame */
    if (pBitstreamParser->State == FirstFrame)
    {
        /* Initialise QMF filter bank */
        for (sf = 0; sf < DELAY_QMF_SYN; sf++)
        {
            /* qmf synthesis */
            CalculateSynFilterbank( &(pSynthCtrl->qmfSynthFilterBankLeft),
                                    ocsLeftReal [NrSlots1 + sf],
                                    ocsLeftImag [NrSlots1 + sf],
                                    OutputLeft,
                                    False,
                                    0);

            CalculateSynFilterbank( &(pSynthCtrl->qmfSynthFilterBankRight),
                                    ocsRightReal [NrSlots1 + sf],
                                    ocsRightImag [NrSlots1 + sf],
                                    OutputRight,
                                    False,
                                    0);
        }
    
        /* Zero output */
        memset( pOutputBuffer, 0, NrSamples1 * sizeof(pOutputBuffer[0]) );
    }
    else
    {
        /* qmf synthesis */
        Offset = DELAY_QMF_SYN;
        for (sf = 0; sf < NrSlots1; sf++) 
        {
            CalculateSynFilterbank( &(pSynthCtrl->qmfSynthFilterBankLeft),
                                    ocsLeftReal [Offset + sf],
                                    ocsLeftImag [Offset + sf],
                                    OutputLeft,
                                    False,
                                    0);
        
            CalculateSynFilterbank( &(pSynthCtrl->qmfSynthFilterBankRight),
                                    ocsRightReal [Offset + sf],
                                    ocsRightImag [Offset + sf],
                                    OutputRight,
                                    False,
                                    0);
    
            /* copy to synthesis buffer */
            for (i = 0; i < SSC_QMF_SUBFRAME_LENGTH; i++)
            {
                pSynthCtrl->pStereoOutput[ 2*(pSynthCtrl->nStereoOutputIndex  + sf * SSC_QMF_SUBFRAME_LENGTH + i) + 0] = OutputLeft [i];
                pSynthCtrl->pStereoOutput[ 2*(pSynthCtrl->nStereoOutputIndex  + sf * SSC_QMF_SUBFRAME_LENGTH + i) + 1] = OutputRight[i];
            }
        }
    };

    /* Fill output buffer with the correct number of samples */
    for (i = 0; i < 2*NrOutputSamples; i++)
    {
        pOutputBuffer[i] = pSynthCtrl->pStereoOutput[i];
    }
 
    /* Calculate the number of samples left in the output buffer */
    if (pBitstreamParser->State != FirstFrame)
    {
        pSynthCtrl->nStereoOutputIndex = (NrSamples1 + pSynthCtrl->nStereoOutputIndex) - NrOutputSamples;
    }

    /* Shift the output buffer  */
    for (i = 0; i < 2*(pSynthCtrl->nStereoOutputIndex); i++)
    {
        pSynthCtrl->pStereoOutput[i] =  pSynthCtrl->pStereoOutput[i + 2*NrOutputSamples];
    }

    /* Shift the input buffer  */
    pSynthCtrl->nStereoIdx -= NrSamples1;
    for (i = 0; i < pSynthCtrl->nStereoIdx; i++)
    {
        pSynthCtrl->pStereoInput[i] =  pSynthCtrl->pStereoInput[i + NrSamples1];
    }

    /* Store slot 3 into in slot 0 for next decoding */
    Offset = NrSlots1;
    for (sf = 0; sf < pSynthCtrl->pNumberOfSlots[2]; sf++)  
    {
        memcpy( ocsLeftReal [sf], 
                ocsLeftReal [Offset + sf], SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftReal [sf][0]));
        memcpy( ocsLeftImag [sf], 
                ocsLeftImag [Offset + sf], SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsLeftImag [sf][0]));
        memcpy( ocsRightReal [sf], 
                ocsRightReal [Offset + sf], SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsRightReal [sf][0]));
        memcpy( ocsRightImag [sf], 
                ocsRightImag [Offset + sf], SSC_QMF_SUBFRAME_LENGTH * sizeof(ocsRightImag [sf][0]));
    }   

    /* Update the memory for QMF Analysis (real part) */
    memmove( pSynthCtrl->pStereoIntermediateAnalysisReal, &pSynthCtrl->pStereoIntermediateAnalysisReal[NrSamples1], NrSamples2 * sizeof(pSynthCtrl->pStereoIntermediateAnalysisReal[0]) );
    /* memset ( &pSynthCtrl->pStereoIntermediateAnalysisReal[NrSamples2], 0, NrSamples1 * sizeof(pSynthCtrl->pStereoIntermediateAnalysisReal[0]) );  */
    
    /* Update the memory for QMF Analysis (imag part) */
    memmove( pSynthCtrl->pStereoIntermediateAnalysisImag, &pSynthCtrl->pStereoIntermediateAnalysisImag[NrSamples1], NrSamples2 * sizeof(pSynthCtrl->pStereoIntermediateAnalysisImag[0]) );
    /* memset ( &pSynthCtrl->pStereoIntermediateAnalysisImag[NrSamples2], 0, NrSamples1 * sizeof(pSynthCtrl->pStereoIntermediateAnalysisImag[0]) );  */

    /* Store number of slots */
    for (i = 0; i < SSC_UPDATE_OCS_QMF; i++)
    {
        pSynthCtrl->pNumberOfSlots[i] = pSynthCtrl->pNumberOfSlots[i+SSC_UPDATE_OCS_QMF];
    }


    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_CreateWindowShapes
 *
 * Description : Create window shapes for a specified segment size.
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 *               Size       - Segment size in samples.
 *
 * Returns     : -
 * 
 *****************************************************************************/

static void LOCAL_CreateWindowShapes(SSC_SYNTHCTRL* pSynthCtrl, UInt Size )
{
    UInt            i,s;
    UInt            envLen, envLen2;
    Double          f, p, a;
    SSC_SAMPLE_INT* ptr;
    Double          d, out ;

    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Check if windows need to be updated */
    if(Size == pSynthCtrl->WindowSize)
    {
        return;
    }

    /* generate noise envelope window */
    envLen  = 2*Size + 2*Size/3; 
    envLen2 = envLen / 2;  /* 920 */
    f = 2*SSC_PI / envLen2;
    p = SSC_PI/envLen2 - SSC_PI;
    a = 0.5;

    for (i = 0; i < envLen2/2; i++)
    {
        pSynthCtrl->pWindowGainEnvelope[i] = a*cos(p) + 0.5;
        p += f;
    }
    for ( ; i < envLen2/2 + envLen2; i++)
    {
        pSynthCtrl->pWindowGainEnvelope[i] = 1;
    }
    for ( ; i < envLen; i++)
    {
        pSynthCtrl->pWindowGainEnvelope[i] = a*cos(p) + 0.5;
        p += f;
    }
    for ( ; i < 3*Size; i++)
    {
        pSynthCtrl->pWindowGainEnvelope[i] = 0;
    }


    /* Generate sine window */
    f = 2*SSC_PI/Size;
    p = SSC_PI/Size - SSC_PI;
    a = 0.5;

    for(i = 0; i < Size; i++)
    {
        pSynthCtrl->pWindowSine[i] =  a*cos(p) + 0.5;
        p += f;
    }

    /* Generate noise window */
    f = SSC_PI / Size;
    p = f / 2;

    for(i = 0; i < Size; i++)
    {
        pSynthCtrl->pWindowNoise[i] = sin(p);
        p += f;
    }

    pSynthCtrl->WindowSize = Size;


    /* Create birth and death windows */
    /*           ___________          */
    /* _________/           \_________*/
    s   = Size/2 - 21;

    ptr =  pSynthCtrl->pWindowBirthDeath;

    for(i = 0; i < s; i++)
    {
        *ptr = 0;
        ptr++;
    }

    d   = 1.0/22.0;
    out = d;
    for(i = 0; i < 21; i++)
    {
        *ptr = out;
        out += d;
        ptr++;
    }

    for(i = 0; i < s; i++)
    {
        *ptr = 1;
        ptr++;
    }

    out -= d;
    for(i = 0; i < 21; i++)
    {
        *ptr = out;
        out -= d;
        ptr++;
    }

    for(i = 0; i < s; i++)
    {
        *ptr = 0;
        ptr++;
    }


}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_GetWindowSineBirth
 *
 * Description : Get the sine birth window.
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 *               Pos        - Transient position
 *
 * Returns     : Pointer to window shape.
 * 
 *****************************************************************************/
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineBirth(SSC_SYNTHCTRL* pSynthCtrl, UInt Pos)
{
    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Return birth window */
    return LOCAL_GetWindow(pSynthCtrl, Pos, pSynthCtrl->pWindowBirthDeath);
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_GetWindowSineDeath
 *
 * Description : Get the sine death window.
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 *               Pos        - Transient position
 *
 * Returns     : Pointer to window shape.
 * 
 *****************************************************************************/
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineDeath(SSC_SYNTHCTRL* pSynthCtrl, UInt Pos)
{
    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Return death window */
    return LOCAL_GetWindow(pSynthCtrl, Pos, pSynthCtrl->pWindowBirthDeath + pSynthCtrl->WindowSize/2);
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_GetWindowSineStart
 *
 * Description : Get the noise start window.
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 * Returns     : Pointer to window shape.
 * 
 *****************************************************************************/
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineStart(SSC_SYNTHCTRL* pSynthCtrl)
{
    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Return start window */
    return pSynthCtrl->pWindowSine;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_GetWindowSineEnd
 *
 * Description : Get the sine end window.
 *
 * Parameters  : pSynthCtrl - Pointer to synthesiser control instance.
 *
 * Returns     : Pointer to window shape.
 * 
 *****************************************************************************/
static const SSC_SAMPLE_INT* LOCAL_GetWindowSineEnd(SSC_SYNTHCTRL* pSynthCtrl)
{
    /* Check input arguments in debug builds */
    assert(pSynthCtrl);

    /* Return death window */
    return pSynthCtrl->pWindowSine + pSynthCtrl->WindowSize/2;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_GetWindow
 *
 * Description : Get window shape based on transient position and shift the 
 *               window if transient is near a sub-frame boundary.
 *
 * Parameters  : pSynthCtrl   - Pointer to synthesiser control instance.
 *
 *               Position     - Transient position
 *
 *               pWindowShape - Pointer to window shape
 *
 * Returns     : Pointer to window shape.
 * 
 *****************************************************************************/
static const SSC_SAMPLE_INT* LOCAL_GetWindow(SSC_SYNTHCTRL*        pSynthCtrl,
                                             UInt                  Pos,
                                             const SSC_SAMPLE_INT* pWindowShape)
{
    UInt SubFrameSize;
    /* Check input arguments in debug builds */
    assert(pSynthCtrl);
    assert(Pos < pSynthCtrl->WindowSize/2);

    /* Adjust position if near a sub-frame boundary */
    SubFrameSize = pSynthCtrl->WindowSize/2;

    if(Pos < 10)
    {
        Pos = 10;
    }
    else if (Pos > (SubFrameSize - 1 - 10))
    {
        Pos = SubFrameSize - 1 - 10 ;
    }

    /* Select window shape */
    return pWindowShape + SubFrameSize - 1 - 10 - Pos;

}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_ClearBuffer
 *
 * Description : Set contents of buffer to zero.
 *
 * Parameters  : pBuffer - Pointer to buffer.
 *
 *               Length  - Number of samples to clear.
 *
 * Returns     : -
 * 
 *****************************************************************************/
static void LOCAL_ClearBuffer(SSC_SAMPLE_INT* pBuffer,
                              UInt            Length)
{
    UInt i;

    for(i = 0; i < Length; i++)
    {
        pBuffer[i] = 0;
    }
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_SelectSinusoids
 *
 * Description : Select sinusoids based on the state and copy the parameters
 *               of the selected sinuoids.
 *
 * Parameters  : pSinSeg       - Pointer to buffer containing the sinusoid
 *                               segment data. (read-only)
 *
 *               NrOfSinusoids - Number of elements in the pSinSeg buffer.
 *
 *               pSinParam     - Pointer to buffer in which the parameters of
 *                               the selected sinuoids should be copied.
 *                               (write-only)
 *
 *               Mask          - Specifies the bits that should be used for
 *                               comparision.
 *
 *               Select        - Specifies the selection criteria.
 *
 *               PitchFactor   - Scale factor for pitch
 *
 * Returns     : Number of sinuoids that match the selection criteria.
 * 
 *****************************************************************************/
static UInt LOCAL_SelectSinusoids(const SSC_SIN_SEGMENT* pSinSeg,
                                  UInt                   NrOfSinusoids,
                                  SSC_SIN_PARAM*         pSinParam,
                                  UInt                   Mask,
                                  UInt                   Select,
                                  Double                 PitchFactor)
{
    UInt Count = 0;
    UInt i;

    for(i = 0; i < NrOfSinusoids; i++)
    {
        if((pSinSeg[i].State & Mask) == Select)
        {
            pSinParam[Count] = pSinSeg[i].Sin;
            if ( pSinParam[Count].Frequency*PitchFactor <= SSC_PI)
            {
                pSinParam[Count].Frequency *= PitchFactor;
            }
            else
            {
                pSinParam[Count].Amplitude = 0;
            }
            Count++;
        }
    }

    return Count;
}


/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_WindowAndMerge
 *
 * Description : Window input siganl and merge the result into the output
 *               buffer
 *
 * Parameters  : pSynthCtrl    - Pointer to synthesiser control instance.
 *  
 *               pWindowShape  - Pointer to buffer containing the window shape.
 *                               (read-only)
 *
 *               pInputBuffer  - Pointer to buffer contaning the siganl to
 *                               window and merge. On return this buffer
 *                               contains the windowed version of the signal.
 *                               (update).
 *
 *               Length        - Number of samples in the input buffer.
 *
 *               pOutputBuffer - Pointer to buffer in which the windowed signal
 *                               should be merged. (update)
 *
 *               Offset        - Sample offset at which the windowed signal
 *                               should be merged into the output buffer.
 *
 *               Channel       - The channel at which which the windowed signal
 *                               should be merged into the output buffer.
 *
 *               TotalChannels - The total number of channels in the output
 *                               buffer.
 *
 * Returns     : -
 * 
 *****************************************************************************/
static void LOCAL_WindowAndMerge(SSC_SYNTHCTRL*        pSynthCtrl,
                                 const SSC_SAMPLE_INT* pWindowShape,
                                 SSC_SAMPLE_INT*       pInputBuffer,
                                 UInt                  Length,
                                 SSC_SAMPLE_EXT*       pOutputBuffer,
                                 UInt                  Offset,
                                 UInt                  Channel,
                                 UInt                  TotalChannels)
{
    SSC_WINDOW_WindowSignal(pSynthCtrl->Instances[Channel].pWindow,
                            pWindowShape,
                            pInputBuffer, Length,
                            pInputBuffer);

    SSC_MERGE_MergeSignal(pSynthCtrl->pMerge,
                          Channel, TotalChannels,
                          pInputBuffer, Length,
                          Offset, pOutputBuffer);
 }

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_ComputeStartPhase
 *
 * Description : Compute the starting phase.
 *
 * Parameters  : pSinParam     - Pointer to buffer containing the parameters 
 *                               of the sinusoids to convert.
 *
 *               NrOfSinusoids - Number of sinuoids to convert.
 *
 *               Length        - The length of a sub-frame.
 *
 * Returns     : -
 * 
 *****************************************************************************/
static void LOCAL_ComputeStartPhase(SSC_SIN_PARAM*  pSinParam,
                                    UInt            NrOfSinusoids,
                                    UInt            Length,
                                    Bool            bFirstHalve)    
{
    UInt i;

    for(i = 0; i < NrOfSinusoids; i++)
    {
        if( bFirstHalve )
        {
            pSinParam[i].Phase -= pSinParam[i].Frequency * (Length-0.5);
        }
        else
        {
            pSinParam[i].Phase += pSinParam[i].Frequency * (0.5);
        }
    }
}

