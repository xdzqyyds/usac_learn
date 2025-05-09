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

Source file: StereoSynth.c

Required libraries: <none>

Authors:
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
16-Sep-2002 TM  Initial version ( m8541 )
15-Jul-2003 RJ	Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Stereosynthesiser
 *
 *   Description         : Generates Stereo Components
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All function names are prefixed by SSC_STEREOSYNTH_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <assert.h>
#include <string.h>
#include <math.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Alloc.h"
#include "StereoSynth.h"
#include "Log.h"
#include "SSC_Error.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define ERBRATE                 (1.8)
#define WRAP_TOLERANCE          (1.5*SSC_PI)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

static SInt cntInstance = 0;

/*============================================================================*/
/*       PROTOTYPES LOCAL STATIC FUNCTIONS                                    */
/*============================================================================*/

static void LOCAL_Initialise(void); 
static void LOCAL_Free(void); 

static void LOCAL_GenerateSynthFilters( Double* iid,
                                        Double* itd,
                                        Double* rho,
                                        Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS], 
                                        SInt   numBinsIdx,
                                        SInt   numItdBinsIdx,
                                        Double scaleLeft [NUM_COMPLEX_SAMPLES], Double scaleRight[NUM_COMPLEX_SAMPLES],
                                        Double phaseLeft [NUM_COMPLEX_SAMPLES], Double phaseRight[NUM_COMPLEX_SAMPLES],
                                        Double alpha     [NUM_COMPLEX_SAMPLES], Double beta      [NUM_COMPLEX_SAMPLES]);

static void LOCAL_GenerateSynthFiltersBL( Double iid,
                                          Double itd,
                                          Double rho,
                                          Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS], 
                                          SInt   numBinsIdx,
                                          SInt   numItdBinsIdx,
                                          Double scaleLeft [NUM_COMPLEX_SAMPLES], Double scaleRight[NUM_COMPLEX_SAMPLES],
                                          Double phaseLeft [NUM_COMPLEX_SAMPLES], Double phaseRight[NUM_COMPLEX_SAMPLES],
                                          Double alpha     [NUM_COMPLEX_SAMPLES], Double beta      [NUM_COMPLEX_SAMPLES]);

static void   LOCAL_Convolute(SSC_SAMPLE_INT* pIn1, SInt size1, 
                              SSC_SAMPLE_INT* pIn2, SInt size2, 
                              SSC_SAMPLE_INT* pOut);

static void   LOCAL_GetWindow(SSC_SAMPLE_INT* pWinL, SInt lenL, 
                              SSC_SAMPLE_INT* pWinS, SInt lenS, 
                              SSC_STEREO_WINDOW_TYPE type, 
                              SInt pos, 
                              SSC_SAMPLE_INT* pWin );

static void   LOCAL_Schroeder(SSC_FFT_INFO* pFft, SInt len, Double* pData);

static void   LOCAL_CreateKernel( Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS], SInt nrBinsIdx );

static Double LOCAL_Erb2F( Double erb );

static void   LOCAL_Unwrap( Double* data, SInt len, Double tol );

static void   LOCAL_FillWindow( SSC_SAMPLE_INT* pWin, SInt len);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_STEREOSYNTH_CreateInstance
 *
 * Description : Creates an instance of the stereo synthesiser.
 *
 * Parameters  : BaseLayer: 1 if decode baselayer only is deserid
 *
 *				 nVariableSfSize: Length of a subframesize
 *
 * Returns:    : Pointer to instance, or NULL if insufficient memory.
 * 
 *****************************************************************************/

SSC_STEREOSYNTH* SSC_STEREOSYNTH_CreateInstance(UInt BaseLayer, UInt	nVariableSfSize)
{
    SSC_STEREOSYNTH* pms = NULL;

    SSC_EXCEPTION  tec =  SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;  

    SSC_ERROR      Error = SSC_ERROR_OK;

    Try
    {
        /* an empty module state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_STEREOSYNTH), 1, "SSC_STEREOSYNTH Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for SSC_STEREOSYNTH instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        pms->pMonoSignal = SSC_CALLOC(LEN_SCHROEDER_LONG + 2 * SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pMonoSignal ), "SSC_STEREOSYNTH - mono signal");
        if ( pms->pMonoSignal == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pDecorTemp = SSC_CALLOC((2*LEN_SCHROEDER_LONG) + SSC_MAX_SUBFRAMES*nVariableSfSize-1, sizeof( *pms->pDecorTemp ), "SSC_STEREOSYNTH - decor temp");
        if ( pms->pDecorTemp == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pWindow = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pWindow ), "SSC_STEREOSYNTH - window");
        if ( pms->pWindow == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pWinSwitch = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pWinSwitch ), "SSC_STEREOSYNTH - windowSwitch");
        if ( pms->pWinSwitch == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pWindowedMono = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pWindowedMono ), "SSC_STEREOSYNTH - windowed mono");
        if ( pms->pWindowedMono == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pWindowedDecor = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pWindowedDecor ), "SSC_STEREOSYNTH - windowed decor");
        if ( pms->pWindowedDecor == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pSignalLeft = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pSignalLeft ), "SSC_STEREOSYNTH - saved left");
        if ( pms->pSignalLeft == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pSignalRight = SSC_CALLOC(SSC_MAX_SUBFRAMES * nVariableSfSize, sizeof( *pms->pSignalRight ), "SSC_STEREOSYNTH - saved left");
        if ( pms->pSignalRight == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* ******************************************* */
        /* init module global data( if nesseccary )    */
        /* ******************************************* */
        LOCAL_Initialise();

        /* **************************************************************** */
        /* do some 'real' initialisation/allocation of data member/pointers */
        /* **************************************************************** */

		pms->decodeStereoBaseLayerOnly = BaseLayer;
        
        /* fill all the kernels */
        LOCAL_CreateKernel(pms->kernels0, 0);
        LOCAL_CreateKernel(pms->kernels1, 1);
        LOCAL_CreateKernel(pms->kernels2, 2);

        /* create fft object */
        pms->pFft = SSC_FFT_CreateInstance(FFT_LENGTH);
        if ( pms->pFft == NULL )
        {
            fprintf(stdout,"\nError: Unable to create fft object inside SSC_STEREOSYNTH instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* fill the decorelation sequences */
        LOCAL_Schroeder(pms->pFft, LEN_SCHROEDER_LONG , pms->pDecorL);
        LOCAL_Schroeder(pms->pFft, LEN_SCHROEDER_SHORT, pms->pDecorS);

        /* fill all the long+short windows */
        pms->len0L = 2 * SSC_StereoUpdateRateFactor[0] * nVariableSfSize;

        pms->pWin0L = SSC_CALLOC( pms->len0L * sizeof(*pms->pWin0L), 1, "Long 0");
        if ( pms->pWin0L == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin0L, pms->len0L);
        
        pms->len0S = pms->len0L/8;
        pms->pWin0S = SSC_CALLOC( pms->len0S * sizeof(*pms->pWin0S), 1, "Short 0");
        if ( pms->pWin0S == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin0S, pms->len0S);

        pms->len1L = 2 * SSC_StereoUpdateRateFactor[1] * nVariableSfSize;
        pms->pWin1L = SSC_CALLOC( pms->len1L * sizeof(*pms->pWin1L), 1, "Long 1");
        if ( pms->pWin1L == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin1L, pms->len1L);
        
        pms->len1S = pms->len1L/8;
        pms->pWin1S = SSC_CALLOC( pms->len1S * sizeof(*pms->pWin1S), 1, "Short 1");
        if ( pms->pWin1S == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin1S, pms->len1S);

        pms->len2L = 2 * SSC_StereoUpdateRateFactor[2] * nVariableSfSize;
        pms->pWin2L = SSC_CALLOC( pms->len2L * sizeof(*pms->pWin2L), 1, "Long 2");
        if ( pms->pWin2L == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin2L, pms->len2L);
        
        pms->len2S = pms->len2L/8;
        pms->pWin2S = SSC_CALLOC( pms->len2S * sizeof(*pms->pWin2S), 1, "Short 2");
        if ( pms->pWin2S == NULL )
        {
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        LOCAL_FillWindow(pms->pWin2S, pms->len2S);

    }
    Catch(Error)
    {
        pms = SSC_STEREOSYNTH_DestroyInstance( pms );    
    }

    return pms;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_STEREOSYNTH_DestroyInstance
 *
 * Description : Destroys an instance of the stereo synthesiser.
 *
 * Parameters  : pms - Pointer to instance to destroy.
 *
 * Returns:    : -
 * 
 *****************************************************************************/

SSC_STEREOSYNTH* SSC_STEREOSYNTH_DestroyInstance(SSC_STEREOSYNTH* pms)
{
    if ( pms != NULL )
    {
        /* clean up this module state instance structure */

        if ( pms->pDecorTemp != NULL )
        {
            SSC_FREE(pms->pDecorTemp, "decor temp");
            pms->pDecorTemp = NULL;
        }

        if ( pms->pMonoSignal != NULL )
        {
            SSC_FREE(pms->pMonoSignal, "mono signal");
            pms->pMonoSignal = NULL;
        }

        if ( pms->pWindow != NULL )
        {
            SSC_FREE(pms->pWindow, "window");
            pms->pWindow = NULL;
        }

        if ( pms->pWinSwitch != NULL )
        {
            SSC_FREE(pms->pWinSwitch, "windowSwitch");
            pms->pWinSwitch = NULL;
        }

        if ( pms->pWindowedMono != NULL )
        {
            SSC_FREE(pms->pWindowedMono, "windowed mono");
            pms->pWindowedMono = NULL;
        }

        if ( pms->pWindowedDecor != NULL )
        {
            SSC_FREE(pms->pWindowedDecor, "windowed decor");
            pms->pWindowedDecor = NULL;
        }

        if ( pms->pSignalLeft != NULL )
        {
            SSC_FREE(pms->pSignalLeft, "signal left");
            pms->pSignalLeft = NULL;
        }

        if ( pms->pSignalRight != NULL )
        {
            SSC_FREE(pms->pSignalRight, "signal right");
            pms->pSignalRight = NULL;
        }

        if ( pms->pFft != NULL )
        {
            SSC_FFT_DestroyInstance(&pms->pFft);
            pms->pFft = NULL;
        }

        if ( pms->pWin0L != NULL )
        {
            SSC_FREE(pms->pWin0L, "Long 0");
            pms->pWin0L = NULL;
        }
        
        if ( pms->pWin0S != NULL )
        {
            SSC_FREE(pms->pWin0S, "Short 0");
            pms->pWin0S = NULL;
        }

        if ( pms->pWin1L != NULL )
        {
            SSC_FREE(pms->pWin1L, "Long 1");
            pms->pWin1L = NULL;
        }
        
        if ( pms->pWin1S != NULL )
        {
            SSC_FREE(pms->pWin1S, "Short 1");
            pms->pWin1S = NULL;
        }

        if ( pms->pWin2L != NULL )
        {
            SSC_FREE(pms->pWin2L, "Long 2");
            pms->pWin2L = NULL;
        }
        
        if ( pms->pWin2S != NULL )
        {
            SSC_FREE(pms->pWin2S, "Short 2");
            pms->pWin2S = NULL;
        }

        /* clean up module global data( if nesseccary ) */
        LOCAL_Free();

        /* clean up the instance structure */
        SSC_FREE(pms, "SSC_STEREOSYNTH Instance");

    }
    else
    {
        fprintf( stdout, "\nError: SSC_STEREOSYNTH_DestroyInstance invalid module state pointer\n" );
    }

    return NULL;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_STEREOSYNTH_Generate
 *
 * Description : Generates from mono stereo
 *
 * Parameters  : pms           - Pointer to instance
 *               pInput        - mono input buffer
 *               pStereoParam  - Pointer to stereo parameter struct
 *               skipSf        - boolean to indicate if stereo parameteres must be ignored
 *               pos           - transient position
 *               updateRateIdx - idx indicating the stereo update rate
 *               pOutput       - stereo output buffer
 *               nVarSfSize    - subframe size
 *
 * Returns:    : -
 * 
 *****************************************************************************/

void SSC_STEREOSYNTH_Generate(SSC_STEREOSYNTH* pms, SSC_SAMPLE_INT* pInput, SSC_STEREO_PARAM* pStereoParam, Bool skipSf, SInt pos, SInt updateRateIdx, SSC_SAMPLE_INT* pOutput, SInt* numSamples, UInt nVarSfSize)
{
    SInt i;
    SInt lenL;
    SInt lenLP;
    SInt lenLC;
    SInt lenLPP;

    SSC_SAMPLE_INT* pWinL = NULL;
    SSC_SAMPLE_INT* pWinS = NULL;

    SSC_SAMPLE_INT freqMonoR [NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqMonoI [NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqDecorR[NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqDecorI[NUM_COMPLEX_SAMPLES];

    SSC_SAMPLE_INT freqLeftR [NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqLeftI [NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqRightR[NUM_COMPLEX_SAMPLES];
    SSC_SAMPLE_INT freqRightI[NUM_COMPLEX_SAMPLES];

    SSC_SAMPLE_INT* pDecor = NULL;
    SInt decorLen = 0;
    
    Double (*kernels)[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS] = NULL;

    Double scaleLeft [NUM_COMPLEX_SAMPLES]; 
    Double scaleRight[NUM_COMPLEX_SAMPLES];
    Double phaseLeft [NUM_COMPLEX_SAMPLES];
    Double phaseRight[NUM_COMPLEX_SAMPLES];
    Double alpha     [NUM_COMPLEX_SAMPLES];
    Double beta      [NUM_COMPLEX_SAMPLES];

    if ( skipSf == True )
    {
        memset( &pms->updateRateIdx.pipe, updateRateIdx, 4 );
    }
    else
    {
        pms->updateRateIdx.pipe       <<= 8 * sizeof( pms->updateRateIdx.rate.current );
        pms->updateRateIdx.rate.current = ( UByte )updateRateIdx;
    }

    /* get the length of a long window */
    lenLC  = SSC_StereoUpdateRateFactor[pms->updateRateIdx.rate.current] * nVarSfSize;
    lenLP  = SSC_StereoUpdateRateFactor[pms->updateRateIdx.rate.hist1]   * nVarSfSize;
    lenLPP = SSC_StereoUpdateRateFactor[pms->updateRateIdx.rate.hist2]   * nVarSfSize;
    lenL   = lenLP + lenLC;

    pInput  -=     lenLP ; /* Now points 1/4 of the original input buffer */
    pOutput -= 2 * lenLP ; /* Now points 1/4 of the original output buffer */

    
	/* Copy inputbuffer to instance mono signal */
	/* Convolution with shorter subframe size (time scaling) can now be applied */

	memcpy(&pms->pMonoSignal[LEN_SCHROEDER_LONG],(pInput-(lenL/2)),2*lenL*sizeof((*pms->pMonoSignal))); 
    
	*numSamples = 0;

    /* look if further processing is necessary */
    if ( skipSf == False )
    {

        /* select the windows */
        switch ( pms->updateRateIdx.rate.current )
        {
            case 0:
                pWinS = pms->pWin0S;
                pWinL = pms->pWin0L;
                break;
            case 1:
                pWinS = pms->pWin1S;
                pWinL = pms->pWin1L;
                break;
            case 2:
                pWinS = pms->pWin2S;
                pWinL = pms->pWin2L;
                break;
            default :
                pWinS = NULL;
                pWinL = NULL;
                break;
        }

        if ( lenLC != lenLP )
        {
            memcpy( &pms->pWinSwitch[lenLP], &pWinL[lenLC], lenLC * sizeof(pWinL[0]) );
            switch ( pms->updateRateIdx.rate.hist1 )
            {
                case 0:
                    pWinL = pms->pWin0L;
                    break;
                case 1:
                    pWinL = pms->pWin1L;
                    break;
                case 2:
                    pWinL = pms->pWin2L;
                    break;
                default :
                    pWinL = NULL;
                    break;
            }
            memcpy( pms->pWinSwitch, pWinL, lenLP * sizeof(pWinL[0]) );
            pWinL = pms->pWinSwitch;
        }

        /* select the correct kernel */
        switch (pStereoParam->st_numberBins)
        {
            case 10:
                kernels = &pms->kernels0;
                break;
            case 20:
                kernels = &pms->kernels1;
                break;
            case 40:
                kernels = &pms->kernels2;
                break;
            default:
                kernels = NULL;           
                break;
        }

        /* ********************************************* */
        /* TRANSIENT PROCESSING                          */
        /* ********************************************* */
        if ( 
            (pStereoParam->base.st_winType == SSC_STEREO_WINDOW_TYPE_STOP_INTERM) ||
            (pStereoParam->base.st_winType == SSC_STEREO_WINDOW_TYPE_STOP_VAR   )
           )
        {
            /* convolution with decor sequence*/
			LOCAL_Convolute(pms->pDecorS,
							LEN_SCHROEDER_SHORT,
							&pms->pMonoSignal[LEN_SCHROEDER_LONG + (lenL/2) + pos - (lenL/8/2) - LEN_SCHROEDER_SHORT],
							LEN_SCHROEDER_SHORT+(lenL/8),
							pms->pDecorTemp);
            

            /* window data + conv signal */
            for ( i = 0; i < (lenL/8); i++)
            {
				pms->pWindowedMono [i] = pms->pMonoSignal[LEN_SCHROEDER_LONG + (lenL/2) + pos-(lenL/8/2)+i] * pWinS[i];
                pms->pWindowedDecor[i] = pms->pDecorTemp[LEN_SCHROEDER_SHORT+i] * pWinS[i];
            }

            /* spectrum of windowed mono */
            SSC_FFT_SetRe(pms->pFft, pms->pWindowedMono, (lenL/8));
            SSC_FFT_SetIm(pms->pFft, NULL, (lenL/8));
            SSC_FFT_Process(pms->pFft, FFT_LENGTH);
            memcpy(freqMonoR, SSC_FFT_GetPr(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
            memcpy(freqMonoI, SSC_FFT_GetPi(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
            /* scale position 0 and pi */
            freqMonoR[                    0] /= 2; freqMonoI[                    0] /= 2;
            freqMonoR[NUM_COMPLEX_SAMPLES-1] /= 2; freqMonoI[NUM_COMPLEX_SAMPLES-1] /= 2;

            /* spectrum of windowed decor */
            SSC_FFT_SetRe(pms->pFft, pms->pWindowedDecor, (lenL/8));
            SSC_FFT_SetIm(pms->pFft, NULL, (lenL/8));
            SSC_FFT_Process(pms->pFft, FFT_LENGTH);
            memcpy(freqDecorR, SSC_FFT_GetPr(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
            memcpy(freqDecorI, SSC_FFT_GetPi(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
            /* scale position 0 and pi */
            freqDecorR[                    0] /= 2; freqDecorI[                    0] /= 2;
            freqDecorR[NUM_COMPLEX_SAMPLES-1] /= 2; freqDecorI[NUM_COMPLEX_SAMPLES-1] /= 2;

            /* generate filter params from iid/itd/rho */
			if (pms->decodeStereoBaseLayerOnly == 0)
			{
				LOCAL_GenerateSynthFilters(pStereoParam->ext1.st_iidT, 
					                       pStereoParam->ext1.st_itdT,
						                   pStereoParam->ext1.st_rhoT, 
							               *kernels, 
								           pStereoParam->st_numberBins,
                                           pStereoParam->st_numberItdBins,
									       scaleLeft, scaleRight, 
										   phaseLeft, phaseRight, 
										   alpha, beta);
			}
			else
			{
				LOCAL_GenerateSynthFiltersBL(pStereoParam->base.st_iidGlobalT, 
					                         pStereoParam->base.st_itdGlobalT,
						                     pStereoParam->base.st_rhoGlobalT, 
 							                 *kernels, 
   	 	 						             pStereoParam->st_numberBins,
                                             pStereoParam->st_numberItdBins,
									         scaleLeft, scaleRight, 
										     phaseLeft, phaseRight, 
										     alpha, beta);
			}

            /* generate spectrum_left/right from spectrum_mono + spectrum_decor + filter params */
            for ( i = 0; i < NUM_COMPLEX_SAMPLES; i++ )
            {
				Double cab;
				Double sab;
				Double cba;
				Double sba;

				if (pms->decodeStereoBaseLayerOnly == 0)
				{
					cab = cos(alpha[i]+beta[i]);
					sab = sin(alpha[i]+beta[i]);

					cba = cos(beta[i]-alpha[i]);
					sba = sin(beta[i]-alpha[i]);
				}
				else
				{
					cab = cos(alpha[0]+beta[0]);
					sab = sin(alpha[0]+beta[0]);

					cba = cos(beta[0]-alpha[0]);
					sba = sin(beta[0]-alpha[0]);
				}

                freqLeftR[i]  = 2 * freqMonoR[i] * cab  +  2 * freqDecorR[i] * sab;
                freqLeftI[i]  = 2 * freqMonoI[i] * cab  +  2 * freqDecorI[i] * sab;

                freqRightR[i] = 2 * freqMonoR[i] * cba  +  2 * freqDecorR[i] * sba;
                freqRightI[i] = 2 * freqMonoI[i] * cba  +  2 * freqDecorI[i] * sba;
            }

            /* adjust spectrum_left + spectrum right on phase/amp (complex multiplication) */
            for ( i = 0; i < NUM_COMPLEX_SAMPLES; i++ )
            {   
                Double rA,iA, rB, iB;

                rA = freqLeftR[i];
                iA = freqLeftI[i];
                rB = cos( phaseLeft[i]);
                iB = sin( phaseLeft[i]);

				if (pms->decodeStereoBaseLayerOnly == 0)
				{				
					freqLeftR[i]  = scaleLeft[i]*(rA*rB - iA*iB);
					freqLeftI[i]  = scaleLeft[i]*(rA*iB + iA*rB);
				}
				else
				{
					freqLeftR[i]  = scaleLeft[0]*(rA*rB - iA*iB);
					freqLeftI[i]  = scaleLeft[0]*(rA*iB + iA*rB);
				}

                rA = freqRightR[i];
                iA = freqRightI[i];
                rB = cos( phaseRight[i]);
                iB = sin( phaseRight[i]);

				if (pms->decodeStereoBaseLayerOnly == 0)
				{								
					freqRightR[i]  = scaleRight[i]*(rA*rB - iA*iB);
					freqRightI[i]  = scaleRight[i]*(rA*iB + iA*rB);
				}
				else
				{
					freqRightR[i]  = scaleRight[0]*(rA*rB - iA*iB);
					freqRightI[i]  = scaleRight[0]*(rA*iB + iA*rB);
				}
            }

            /* spectrum_left to time */
            SSC_FFT_SetRe(pms->pFft, freqLeftR, NUM_COMPLEX_SAMPLES); 
            SSC_FFT_SetIm(pms->pFft, freqLeftI, NUM_COMPLEX_SAMPLES);
            SSC_FFT_Process_Inverse(pms->pFft, FFT_LENGTH);
            /* scaling by factor 2 of data-samples */
            for ( i = 0; i < (lenL/8); i++)
            {
                pms->pSignalLeft[i] = 2 * SSC_FFT_GetPr(pms->pFft)[i];
            }

            /* spectrum_right to time */
            SSC_FFT_SetRe(pms->pFft, freqRightR, NUM_COMPLEX_SAMPLES); 
            SSC_FFT_SetIm(pms->pFft, freqRightI, NUM_COMPLEX_SAMPLES);
            SSC_FFT_Process_Inverse(pms->pFft, FFT_LENGTH);
            /* scaling by factor 2 of data-samples */
            for ( i = 0; i < (lenL/8); i++)
            {
                pms->pSignalRight[i] = 2 * SSC_FFT_GetPr(pms->pFft)[i];                
            }

            /* window the generated signals */
            for ( i = 0; i < (lenL/8); i++)
            {
                pms->pSignalLeft [i] *= pWinS[i];
                pms->pSignalRight[i] *= pWinS[i];
            }

            /* save found data in the right place*/
            for ( i = 0; i < (lenL/8); i++)
            {
                pOutput[2*(i+pos-(lenL/8/2))+0] += pms->pSignalLeft [i];
                pOutput[2*(i+pos-(lenL/8/2))+1] += pms->pSignalRight[i];
            }
        }
    
        
        /* ********************************************* */
        /* NORMAL PROCESSING                             */
        /* ********************************************* */
        /* do convolution with decor sequence */
        if ( 
            (pStereoParam->base.st_winType == SSC_STEREO_WINDOW_TYPE_STOP_INTERM) ||
            (pStereoParam->base.st_winType == SSC_STEREO_WINDOW_TYPE_STOP_VAR   )
           )
        {
            pDecor = pms->pDecorS;
            decorLen = LEN_SCHROEDER_SHORT;
        }
        else
        {
            pDecor = pms->pDecorL;
            decorLen = LEN_SCHROEDER_LONG;
        }
		
		LOCAL_Convolute(pDecor, decorLen, &pms->pMonoSignal[LEN_SCHROEDER_LONG + (lenL/2) - decorLen], decorLen+lenL, pms->pDecorTemp); /* in1, len1, in2, len2, out */
        /* get window to be used */
        LOCAL_GetWindow(pWinL, lenL, pWinS, lenL/8, pStereoParam->base.st_winType, pos, pms->pWindow );        

        /* window mono + decor */
        for ( i = 0; i < lenL; i++)
        {
			pms->pWindowedMono [i] = pms->pMonoSignal[LEN_SCHROEDER_LONG + (lenL/2) + i] * pms->pWindow[i];
            pms->pWindowedDecor[i] = pms->pDecorTemp[decorLen+i] * pms->pWindow[i];
        }
    
         /* spectrum of windowed mono */
        SSC_FFT_SetRe(pms->pFft, pms->pWindowedMono, lenL);         
        SSC_FFT_SetIm(pms->pFft, NULL, lenL);
        SSC_FFT_Process(pms->pFft, FFT_LENGTH);
        memcpy(freqMonoR, SSC_FFT_GetPr(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
        memcpy(freqMonoI, SSC_FFT_GetPi(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
        /* scale position 0 and pi */
        freqMonoR[                    0] /= 2; freqMonoI[                    0] /= 2;
        freqMonoR[NUM_COMPLEX_SAMPLES-1] /= 2; freqMonoI[NUM_COMPLEX_SAMPLES-1] /= 2;
    
        /* spectrum of windowed decor */
        SSC_FFT_SetRe(pms->pFft, pms->pWindowedDecor, lenL); 
        SSC_FFT_SetIm(pms->pFft, NULL, lenL);
        SSC_FFT_Process(pms->pFft, FFT_LENGTH);
        memcpy(freqDecorR, SSC_FFT_GetPr(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
        memcpy(freqDecorI, SSC_FFT_GetPi(pms->pFft), NUM_COMPLEX_SAMPLES*sizeof(Double) );
        /* scale position 0 and pi */
        freqDecorR[                    0] /= 2; freqDecorI[                    0] /= 2;
        freqDecorR[NUM_COMPLEX_SAMPLES-1] /= 2; freqDecorI[NUM_COMPLEX_SAMPLES-1] /= 2;

        /* generate filter params from iid/itd/rho */
		if (pms->decodeStereoBaseLayerOnly == 0)
		{								
			LOCAL_GenerateSynthFilters(pStereoParam->ext1.st_iid, 
									   pStereoParam->ext1.st_itd,
									   pStereoParam->ext1.st_rho, 
									   *kernels, 
			    			           pStereoParam->st_numberBins,
                                       pStereoParam->st_numberItdBins,
									   scaleLeft, scaleRight, 
									   phaseLeft, phaseRight, 
									   alpha, beta);
		}
		else
		{
			LOCAL_GenerateSynthFiltersBL(pStereoParam->base.st_iidGlobal, 
					                     pStereoParam->base.st_itdGlobal,
						                 pStereoParam->base.st_rhoGlobal, 
							             *kernels, 
  							             pStereoParam->st_numberBins,
                                         pStereoParam->st_numberItdBins,
									     scaleLeft, scaleRight, 
										 phaseLeft, phaseRight, 
										 alpha, beta);
		}
    
        /* generate spectrum_left/right from spectrum_mono + spectrum_decor + filter params */
        for ( i = 0; i < NUM_COMPLEX_SAMPLES; i++ )
        {
			Double cab;
			Double sab;
			Double cba;
			Double sba;

			if (pms->decodeStereoBaseLayerOnly == 0)
			{
				cab = cos(alpha[i]+beta[i]);
				sab = sin(alpha[i]+beta[i]);

				cba = cos(beta[i]-alpha[i]);
				sba = sin(beta[i]-alpha[i]);
			}
			else
			{
				cab = cos(alpha[0]+beta[0]);
				sab = sin(alpha[0]+beta[0]);

				cba = cos(beta[0]-alpha[0]);
				sba = sin(beta[0]-alpha[0]);
			}

            freqLeftR[i]  = 2 * freqMonoR[i] * cab  +  2 * freqDecorR[i] * sab;
            freqLeftI[i]  = 2 * freqMonoI[i] * cab  +  2 * freqDecorI[i] * sab;

            freqRightR[i] = 2 * freqMonoR[i] * cba  +  2 * freqDecorR[i] * sba;
            freqRightI[i] = 2 * freqMonoI[i] * cba  +  2 * freqDecorI[i] * sba;
        }

        /* adjust spectrum_left + spectrum right on phase/amp */
        for ( i = 0; i < NUM_COMPLEX_SAMPLES; i++ )
        {   
            Double rA,iA, rB, iB;

            rA = freqLeftR[i];
            iA = freqLeftI[i];
            rB = cos( phaseLeft[i]);
            iB = sin( phaseLeft[i]);

			if (pms->decodeStereoBaseLayerOnly == 0)
			{				
				freqLeftR[i]  = scaleLeft[i]*(rA*rB - iA*iB);
				freqLeftI[i]  = scaleLeft[i]*(rA*iB + iA*rB);
			}
			else
			{
				freqLeftR[i]  = scaleLeft[0]*(rA*rB - iA*iB);
				freqLeftI[i]  = scaleLeft[0]*(rA*iB + iA*rB);
			}

            rA = freqRightR[i];
            iA = freqRightI[i];
            rB = cos( phaseRight[i]);
            iB = sin( phaseRight[i]);

			if (pms->decodeStereoBaseLayerOnly == 0)
			{								
				freqRightR[i]  = scaleRight[i]*(rA*rB - iA*iB);
				freqRightI[i]  = scaleRight[i]*(rA*iB + iA*rB);
			}
			else
			{
				freqRightR[i]  = scaleRight[0]*(rA*rB - iA*iB);
				freqRightI[i]  = scaleRight[0]*(rA*iB + iA*rB);
			}
        }

        /* spectrum_left to time */
        SSC_FFT_SetRe(pms->pFft, freqLeftR, NUM_COMPLEX_SAMPLES); 
        SSC_FFT_SetIm(pms->pFft, freqLeftI, NUM_COMPLEX_SAMPLES);
        SSC_FFT_Process_Inverse(pms->pFft, FFT_LENGTH);
        /* scaling by factor 2 of data-samples */
        for ( i = 0; i < lenL; i++)
        {
            pms->pSignalLeft[i] = 2 * SSC_FFT_GetPr(pms->pFft)[i];
        }

        /* spectrum_right to time */
        SSC_FFT_SetRe(pms->pFft, freqRightR, NUM_COMPLEX_SAMPLES); 
        SSC_FFT_SetIm(pms->pFft, freqRightI, NUM_COMPLEX_SAMPLES);
        SSC_FFT_Process_Inverse(pms->pFft, FFT_LENGTH);
        /* scaling by factor 2 of data-samples */
        for ( i = 0; i < lenL; i++)
        {
            pms->pSignalRight[i] = 2 * SSC_FFT_GetPr(pms->pFft)[i];
        }

        /* window the generated signals */
        for ( i = 0; i < lenL; i++)
        {
            pms->pSignalLeft [i] *= pms->pWindow[i];
            pms->pSignalRight[i] *= pms->pWindow[i];
        }

        /* Put to output buffer */
        for ( i = 0; i < lenL; i++)
        {
            pOutput[2*i+0] += pms->pSignalLeft [i];
            pOutput[2*i+1] += pms->pSignalRight[i];
        }
    }
    *numSamples = lenLC;
}

/*============================================================================*/
/*       LOCAL FUNCTION IMPLEMENTATIONS                                       */
/*============================================================================*/

/*************************LOCAL  FUNCTION**************************************
 * 
 * Name        : LOCAL_Initialise
 *
 * Description : Initialises the stereo synthesiser module.
 *               This function must be called before the first instance of
 *               the stereo synthesiser is created.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_Initialise(void)
{
    /* do some module static variable initialisation only the first time*/
    if ( cntInstance == 0 )
    {
    }

    /* increment the number of instances */
    cntInstance += 1;
}

/*************************LOCAL  FUNCTION**************************************
 * 
 * Name        : LOCAL_Free
 *
 * Description : Frees the stereo synthesiser module, undoing the effects
 *               of the LOCAL_Initialise() function.
 *               This function must be called after the last instance of
 *               the stereo synthesiser is destroyed.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_Free(void)
{
    if ( cntInstance > 0 )
    {
        /* decrement the number of instances */
        cntInstance -= 1;

        /* do some module static variable cleaning up only the last time*/
        if ( cntInstance == 0 )
        {
        }
    }
    else
    {
        /* error */
        fprintf( stdout, "\nError: SSC_STEREOSYNTH_Free invalid instance count\n" );
    }
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_Convolute
 *
 * Description : Do convolution on 2 arrays(pIn1,pIn2). The result is stored in the third array 
 *               (pOut) of length size1+size2-1
 *
 * Parameters  : pIn1  - first array with data samples
 *               size1 - length of pIn1
 *               pIn2  - second array with data samples
 *               size2 - length of pIn2
 *
 * Returns:    : pOut  the convoluted signal
 * 
 *****************************************************************************/

static void LOCAL_Convolute(SSC_SAMPLE_INT* pIn1, SInt size1, 
                            SSC_SAMPLE_INT* pIn2, SInt size2, 
                            SSC_SAMPLE_INT* pOut)
{
    SInt c1;
    SInt c2;

    /* clear output buffer */
    memset(pOut, 0, (size1+size2-1)*sizeof(*pOut) );

    for( c2 = 0 ; c2 < size1 ; c2++ )
    {
        for( c1 = c2 ; c1 < size2+c2 ; c1++ )
        {
            pOut[c1] += pIn1[c2] * pIn2[c1-c2];
        }
    }
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_GetWindow
 *
 * Description : generates the correct window from a long/short window, the type 
 *               and the transient position
 *
 * Parameters  : pWinL - pointer to long window
 *               lenL  - length of the long window
 *               pWinS - pointer to short window
 *               lenS  - length of the short window
 *               type  - to window type to be generated
 *               pos   - transient position
 *
 * Returns:    : pWin  - pointer to the generated window with length lenL
 * 
 *****************************************************************************/

static void LOCAL_GetWindow(SSC_SAMPLE_INT* pWinL, SInt lenL, 
                            SSC_SAMPLE_INT* pWinS, SInt lenS, 
                            SSC_STEREO_WINDOW_TYPE type, 
                            SInt pos, 
                            SSC_SAMPLE_INT* pWin )
{
    SInt i;

    switch (type)
    {
        case SSC_STEREO_WINDOW_TYPE_NORMAL:
            /* pos not used */
            memcpy(&pWin[0], &pWinL[0], lenL*sizeof(*pWin) );
            break;
        case SSC_STEREO_WINDOW_TYPE_START_FIX:
            /* pos not used */
            memcpy(&pWin[     0], &pWinL[     0], (lenL/2)*sizeof(*pWin) );
            memcpy(&pWin[lenL/2], &pWinS[lenS/2], (lenS/2)*sizeof(*pWin) );
            for ( i = (lenS+lenL)/2; i < lenL; i++)
            {
                pWin[i] = 0.0;
            }
            break;
        case SSC_STEREO_WINDOW_TYPE_START_VAR:
            memcpy(&pWin[0], &pWinL[0], (lenL/2)*sizeof(*pWin) );
            for ( i = lenL/2; i < (lenL-lenS)/2+pos; i++)
            {
                pWin[i] = 1.0;
            }
            memcpy(&pWin[(lenL-lenS)/2+pos], &pWinS[lenS/2], (lenS/2)*sizeof(*pWin) );
            for ( i = (lenL/2)+pos; i < lenL; i++)
            {
                pWin[i] = 0.0;
            }
            break;
        case SSC_STEREO_WINDOW_TYPE_START_INTERM:
            memcpy(&pWin[0], &pWinS[0],  (lenS/2)*sizeof(*pWin) );
            for ( i = (lenS/2); i < (lenL-lenS)/2+pos; i++)
            {
                pWin[i] = 1.0;
            }
            memcpy(&pWin[(lenL-lenS)/2+pos], &pWinS[lenS/2], (lenS/2)*sizeof(*pWin) );
            for ( i = (lenL/2)+pos; i < lenL; i++)
            {
                pWin[i] = 0.0;
            }
            break;
        case SSC_STEREO_WINDOW_TYPE_STOP_INTERM:
            for ( i = 0; i < pos; i++)
            {
                pWin[i] = 0.0;
            }
            memcpy(&pWin[pos], &pWinS[0], (lenS/2)*sizeof(*pWin) );
            for ( i = (lenS/2)+pos; i < lenL-(lenS/2); i++)
            {
                pWin[i] = 1.0;
            }
            memcpy(&pWin[lenL-(lenS/2)], &pWinS[lenS/2], (lenS/2)*sizeof(*pWin) );
            break;
        case SSC_STEREO_WINDOW_TYPE_STOP_VAR:
            for ( i = 0; i < pos; i++)
            {
                pWin[i] = 0.0;
            }
            memcpy(&pWin[pos], &pWinS[0], (lenS/2)*sizeof(*pWin) );
            for ( i = (lenS/2)+pos; i < (lenL/2); i++)
            {
                pWin[i] = 1.0;
            }
            memcpy(&pWin[lenL/2], &pWinL[lenL/2], (lenL/2)*sizeof(*pWin) );
            break;
        case SSC_STEREO_WINDOW_TYPE_STOP_FIX:
            /* pos not used */
            for ( i = 0; i < (lenL-lenS)/2; i++)
            {
                pWin[i] = 0.0;
            }
            memcpy(&pWin[(lenL-lenS)/2], &pWinS[0     ], (lenS/2)*sizeof(*pWin) );
            memcpy(&pWin[       lenL/2], &pWinL[lenL/2], (lenL/2)*sizeof(*pWin) );
            break;
        default:
            /* error not regognized window type encountered */
            break;
    }
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_GenerateSynthFilters
 *
 * Description : Generates from iid/itd/rho parameters the corresponding scale/phase 
 *               for left and right and the alpha and beta data
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_GenerateSynthFilters( Double* iid,
                                        Double* itd,
                                        Double* rho,
                                        Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS], 
                                        SInt   numBins,
                                        SInt   numItdBins,
                                        Double scaleLeft [NUM_COMPLEX_SAMPLES], Double scaleRight[NUM_COMPLEX_SAMPLES],
                                        Double phaseLeft [NUM_COMPLEX_SAMPLES], Double phaseRight[NUM_COMPLEX_SAMPLES],
                                        Double alpha     [NUM_COMPLEX_SAMPLES], Double beta      [NUM_COMPLEX_SAMPLES]
                                       )
{
    SInt i,n;
    
    Double itdLeft [SSC_ENCODER_MAX_STEREO_BINS];
    Double itdRight[SSC_ENCODER_MAX_STEREO_BINS];

    Double sc1[SSC_ENCODER_MAX_STEREO_BINS];
    Double sc2[SSC_ENCODER_MAX_STEREO_BINS];

    /* first clear all output */
    for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
    {
        scaleLeft [n] = 0.0;
        scaleRight[n] = 0.0;
        phaseLeft [n] = 0.0;
        phaseRight[n] = 0.0;
        alpha     [n] = 0.0;
    }

    /* first make sc1 + sc2 */
    for ( i = 0; i < numBins; i++)
    {
        Double  tmp = pow(10,iid[i]/20);
        sc1[i] = 1/(1+tmp);
        sc2[i] = tmp * sc1[i];
    }

    for ( i = 0; i < numBins; i++)
    {   
        Double alphaCurr = 0.5 * acos ( rho[i] );

        for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
        {
            Double kVal = kernels[n][i];
            
            scaleLeft [n] += kVal * sc2[i];
            scaleRight[n] += kVal * sc1[i];

            alpha     [n] += kVal * alphaCurr;
        }
    }

    for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
    {
        beta[n] = atan( tan(alpha[n]) * (1-scaleLeft[n]/scaleRight[n]) / (1+scaleLeft[n]/scaleRight[n]) );
    }

    for ( i = 0; i < numItdBins; i++)
    {
        itdLeft [i] =  sc1[i] * itd[i];
        itdRight[i] = -sc2[i] * itd[i];
    }

    LOCAL_Unwrap(itdLeft , numItdBins, WRAP_TOLERANCE);
    LOCAL_Unwrap(itdRight, numItdBins, WRAP_TOLERANCE);

    for ( i = 0; i < numItdBins; i++)
    {
        for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
        {
            Double kVal = kernels[n][i];
            
            phaseLeft [n] += kVal * itdLeft [i];
            phaseRight[n] += kVal * itdRight[i];
        }
    }
}


/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_GenerateSynthFiltersBL
 *
 * Description : Generates from iid/itd/rho parameters the corresponding scale/phase 
 *               for left and right and the alpha and beta data (Base layer dec. only)
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/
static void LOCAL_GenerateSynthFiltersBL( Double iid,
                                          Double itd,
                                          Double rho,
                                          Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS],
										  SInt   numBins,
                                          SInt   numItdBins,
                                          Double scaleLeft [NUM_COMPLEX_SAMPLES], Double scaleRight[NUM_COMPLEX_SAMPLES],
                                          Double phaseLeft [NUM_COMPLEX_SAMPLES], Double phaseRight[NUM_COMPLEX_SAMPLES],
                                          Double alpha     [NUM_COMPLEX_SAMPLES], Double beta      [NUM_COMPLEX_SAMPLES]
                                        ) 
{
    SInt i, n;
    
    Double itdLeft; 
    Double itdRight;
	Double tmp;

    numBins = numBins;

    /* first clear phase output */
    for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
    {
        phaseLeft [n] = 0.0;
        phaseRight[n] = 0.0;
    }

    /* first make scales, alpha and beta */
    tmp = pow(10,iid/20);
    scaleRight [0] = 1/(1+tmp);
    scaleLeft  [0] = tmp * scaleRight[0];

	alpha[0] = 0.5 * acos ( rho );

	beta [0] = atan( tan(alpha[0]) * (scaleRight[0]-scaleLeft[0]) / (scaleRight[0]+scaleLeft[0]) );

    itdLeft  =  scaleRight[0] * itd;
    itdRight = -scaleLeft [0] * itd;

	/* Note that the following is equivalent to description in m9214! */
    for ( i = 0; i < numItdBins; i++)
    {
        for ( n = 0; n < NUM_COMPLEX_SAMPLES; n++)
        {
            Double kVal = kernels[n][i];
            
            phaseLeft [n] += kVal * itdLeft ;
            phaseRight[n] += kVal * itdRight;
        }
    }
}


/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_Schroeder
 *
 * Description : Generates 'len' data samples appointed by pData pointer
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_Schroeder(SSC_FFT_INFO* pFft, SInt len, Double* pData)
{
    SInt i;

    Double* pReal = SSC_FFT_GetPr(pFft);
    Double* pImag = SSC_FFT_GetPi(pFft);
    
    /* fill first part */
    for ( i = 0; i < (len/2+1); i++ )
    {
        Double a = SSC_PI*i*((Double)(i-1)/(len/2));
        pReal[i] = cos( a );
        pImag[i] = sin( a );
    }
    /* clear the rest */
    for ( i = (len/2+1); i < len; i++)
    {
        pReal[i] = 0.0;
        pImag[i] = 0.0;
    }

    pReal[0] /= 2;
    pImag[0] /= 2;

    SSC_FFT_Process_Inverse(pFft, len);

    /* scaling by factor 2 of data-samples */
    for ( i = 0; i < len; i++)
    {
        pData[i] = 2 * SSC_FFT_GetPr(pFft)[i];
    }
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_CreateKernel
 *
 * Description : Creates for a given number of freq bins the windows 
 *               for each freq bin with length FFT_LENGTH
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_CreateKernel( Double kernels[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS], SInt nrBinsIdx )
{
    SInt slices[SSC_ENCODER_MAX_STEREO_BINS+1];

    SInt numBins = SSC_StereoNumberFreqBins[nrBinsIdx];

    Double  erbRate = 2 * ERBRATE / (1 << nrBinsIdx);
    Double freqRate = (Double)FFT_LENGTH/44100;

    SInt i, n, idx1, idx2;

    /* fill first slices */
    slices[0] = 0;
    for ( i = 1; i < numBins; i++)
    {
        slices[i] = ((SInt)floor(LOCAL_Erb2F( i * erbRate ) * freqRate + 0.5)) - 1;
    }
    slices[numBins] = NUM_COMPLEX_SAMPLES-1;


    /* fill kernels */
    idx1 = (SInt)floor(((slices[0] + slices[1] )/ 2.0)+0.5);
    /* pre */
    for ( n = 0; n < idx1; n++ )
    {
        kernels[n][0] = 1;
    }
    for ( i = 0; i < (numBins-1); i++ )
    {
        idx2 = (SInt)floor(((slices[i+1] + slices[i+2] )/ 2.0)+0.5);
        for ( n = idx1; n < idx2; n ++ )
        {
            kernels[n][i+0] = 0.5 + 0.5 * cos(SSC_PI * (n-idx1)/(idx2-idx1-1));
            kernels[n][i+1] = 1 - kernels[n][i+0];
        }
        /* clear the rest */
        for ( n = 0; n < idx1; n++ )
        {
            kernels[n][i+1] = 0.0;
        }
        for ( n = idx2; n < NUM_COMPLEX_SAMPLES; n++ )
        {
            kernels[n][i] = 0.0;
        }
        idx1 = idx2;
    }
    /* post */
    idx2 = ((SInt)floor(slices[numBins] + 0.5)) +1;
    for ( n = idx1; n < idx2; n++ )
    {
        kernels[n][numBins-1] = 1;
    }

}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_Erb2F
 *
 * Description : Compute frequencies corresponding to erb
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static Double LOCAL_Erb2F( Double erb )
{
  return (exp(0.11 * erb) - 1) / 0.00437;
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_Unwrap
 *
 * Description : unwraps phases with tolerance tol
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_Unwrap( Double* data, SInt len, Double tol )
{
    SInt i, k;
    Double prev, curr;

    k = 0;
    prev = data[0];
    for ( i = 1; i < len; i++)
    {
        curr = data[i];
        
        /* test if step is to big */
        if      ( (prev-curr) > tol )
        {
            k += 1;
        }
        else if ( (curr-prev) > tol)
        {
            k -= 1;
        }
        
        data[i] += 2*k*SSC_PI;

        /* save curr for next loop */
        prev = curr;
    }
}

/*************************LOCAL FUNCTION***************************************
 * 
 * Name        : LOCAL_FillWindow
 *
 * Description : Creates a sin window with length 'len'
 *
 * Parameters  : 
 *
 * Returns:    : -
 * 
 *****************************************************************************/

static void LOCAL_FillWindow( SSC_SAMPLE_INT* pWin, SInt len)
{
    SInt i;

    for ( i = 0; i < len; i++ )
    {
        pWin[i] = sin ( SSC_PI*(i+0.5) / len );
    }
}
