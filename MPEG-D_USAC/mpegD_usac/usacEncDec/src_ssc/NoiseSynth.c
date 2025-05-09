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

Source file: NoiseSynth.c

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  m8540 improved noise coding + 8 sf/frame
28 Nov 2002 AR  RM3.0: Laguerre added, num removed
15 Jul 2003 RJ  RM3a : Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Noise synthesiser
 *
 *   Description         : Generates Noise Components
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All function names are prefixed by SSC_NOISESYNTH_
 *
 ******************************************************************************/

#define RND_SAMPLES 12

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <assert.h>
#include <string.h>
#include <math.h>

#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Alloc.h"
#include "NoiseSynth.h"
#include "Log.h"
#include "SSC_Error.h"

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
/*       PROTOTYPES LOCAL STATIC FUNCTIONS                                    */
/*============================================================================*/

static void LOCAL_GenerateNoise(UInt32*         pSeed,
                                SSC_SAMPLE_INT* pOutput,
                                UInt            Length);

static void LOCAL_ParcorFilter(SSC_NOISESYNTH*        pNoiseSynth,
                               SSC_NOISE_PARAM*       pNoiseParam,
                               const SSC_SAMPLE_INT*  pInputSamples,
                               SSC_SAMPLE_INT*        pOutputSamples,
                               UInt                   Length);

static void LOCAL_LagSynthFilter(SSC_SAMPLE_INT*       z,
                                 const SSC_SAMPLE_INT* in, 
                                 SSC_SAMPLE_INT*       out,     
                                 UInt                  Length,
                                 const SSC_SAMPLE_INT* alpha,  
                                 UInt                  n,
                                 SSC_SAMPLE_INT        pole);

static void LOCAL_parcors2ABs(const SInt            nCoeffs,
                              const SSC_SAMPLE_INT* rfc,
                              SSC_SAMPLE_INT*       x);

static void LOCAL_A2Lag(const SInt            nCoeffs,
                        const SSC_SAMPLE_INT* a,
                        const SSC_SAMPLE_INT  pole,
                        SSC_SAMPLE_INT*       lag);

static void LOCAL_GenerateEnvelope(const SSC_NOISE_PARAM* pNoiseParam,
                                   SSC_SAMPLE_INT*        envGain, 
                                   UInt                   Length );

static void LOCAL_Lsf2A (SSC_SAMPLE_INT* lsf, 
                         UInt lsfOrder, 
                         SSC_SAMPLE_INT* pResult);

static void LOCAL_Freqz(SSC_SAMPLE_INT* a, 
                        UInt aLen,
                        SSC_SAMPLE_INT* pResult,
                        UInt Length);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISESYNTH_Initialise
 *
 * Description : Initialises the noise synthesiser module.
 *               This function must be called before the first instance of
 *               the noise synthesiser is created.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_Initialise(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISESYNTH_Free
 *
 * Description : Frees the noise synthesiser module, undoing the effects
 *               of the SSC_NOISESYNTH_Initialise() function.
 *               This function must be called after the last instance of
 *               the noise synthesiser is destroyed.
 *
 * Parameters  : -
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_Free(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISESYNTH_CreateInstance
 *
 * Description : Creates an instance of the noise synthesiser.
 *
 * Parameters  : FilterLength    - Number of filter taps
 *
 *               nVariableSfSize - Length of a subframe
 *
 * Returns:    : Pointer to instance, or NULL if insufficient memory.
 * 
 *****************************************************************************/
SSC_NOISESYNTH* SSC_NOISESYNTH_CreateInstance(UInt FilterLength, int nVariableSfSize)
{
    SSC_NOISESYNTH* pNoiseSynth = NULL;

    SSC_EXCEPTION  tec =  SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;  

    SSC_ERROR      Error = SSC_ERROR_OK;
    Try
    {

        SInt i;

        /* Check input arguments in debug builds */
        assert(FilterLength > 0);
        assert(FilterLength <= SSC_N_MAX_FILTER_COEFFICIENTS+1);

        /* Allocate instance data structure */
        pNoiseSynth = SSC_MALLOC(sizeof(SSC_NOISESYNTH), "SSC_NOISESYNTH Instance");
        if(pNoiseSynth == NULL)
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for SSC_NOISESYNTH instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        pNoiseSynth->FilterLength = FilterLength;

        pNoiseSynth->pBuffer = SSC_CALLOC(2 * nVariableSfSize, sizeof( *pNoiseSynth->pBuffer ), "SSC_NOISESYNTH - buffer");
        if ( pNoiseSynth->pBuffer == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pNoiseSynth->envGain = SSC_CALLOC(6 * nVariableSfSize, sizeof( *pNoiseSynth->envGain ), "SSC_NOISESYNTH - envelope gain");
        if ( pNoiseSynth->envGain == NULL )
        {
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        SSC_NOISESYNTH_ResetFilter(pNoiseSynth);
        SSC_NOISESYNTH_ResetRandom(pNoiseSynth,0);

        for ( i = 0; i < 2 * nVariableSfSize; i++ )
        {
            pNoiseSynth->pBuffer[i] = 0.0;
        }
    }
    Catch(Error)
    {
        SSC_NOISESYNTH_DestroyInstance( pNoiseSynth );
    }
    return pNoiseSynth;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISESYNTH_DestroyInstance
 *
 * Description : Destroys an instance of the noise synthesiser.
 *
 * Parameters  : pNoiseSynth - Pointer to instance to destroy.
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_DestroyInstance(SSC_NOISESYNTH* pNoiseSynth)
{
    if(pNoiseSynth != NULL)
    {
        if ( pNoiseSynth->pBuffer != NULL )
        {
            SSC_FREE(pNoiseSynth->pBuffer, "SSC_NOISESYNTH - buffer");
            pNoiseSynth->pBuffer = NULL;
        }

        if ( pNoiseSynth->envGain != NULL)
        {
            SSC_FREE(pNoiseSynth->envGain, "SSC_NOISESYNTH - envelope gain");
            pNoiseSynth->envGain = NULL;
        }

        SSC_FREE(pNoiseSynth, "SSC_NOISESYNTH Instance");
    }

}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISE_SYNTH_ResetRandom
 *
 * Description : Resets the random number generator to a specified seed value
 *
 * Parameters  : pNoiseSynth - pointer to noise synthesizer instance
 *
 *               Seed        - seed value for random number generator
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_ResetRandom(SSC_NOISESYNTH* pNoiseSynth,
                                UInt32          Seed)
{
    assert(pNoiseSynth);
    pNoiseSynth->RandomSeed = Seed;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_NOISESYNTH_ResetFilter
 *
 * Description : Resets the internal filter states to zero.
 *
 * Parameters  : pNoiseSynth - Pointer to NoiseSynth instance
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_ResetFilter(SSC_NOISESYNTH* pNoiseSynth)
{
    SInt i;

    assert(pNoiseSynth);

    for(i = 0; i < SSC_N_MAX_FILTER_COEFFICIENTS+1; i++)
    {
        pNoiseSynth->FilterState[i] = 0.0;
    }
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_NOISESYNTH_GenerateNoise
 *
 * Description :  Generates white noise.
 *
 * Parameters  :  pNoiseSynth     - Pointer to instance data.
 *
 *                pSampleBuffer   - Pointer to sample buffer (write-only)
 *
 *                Length          - Number of samples to generate.
 *
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_GenerateNoise(SSC_NOISESYNTH* pNoiseSynth,
                                  SSC_SAMPLE_INT* pSampleBuffer,
                                  UInt            Length)
{
    assert(pNoiseSynth);

    /* Fill buffer with white noise */
    LOCAL_GenerateNoise(&(pNoiseSynth->RandomSeed),
                        pSampleBuffer,
                        Length);
}


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_NOISESYNTH_Generate
 *
 * Description :  Generates the noise component signals.
 *
 * Parameters  :  pNoiseSynth         - Pointer to instance data.
 *
 *                pNoiseParams        - Pointer to noise parameters (input)
 *                
 *                pSampleBuffer       - Pointer to sample buffer(write-only)
 *
 *                Length              - Number of samples to generate.
 *
 *                pWindowGainEnvelope - gains * base noise gain window
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_Generate(SSC_NOISESYNTH*             pNoiseSynth,
                             SSC_NOISE_PARAM*            pNoiseParam,
                             SSC_SAMPLE_INT*             pSampleBuffer,
                             UInt                        Length,
                             SSC_SAMPLE_INT*             pWindowGainEnvelope)
{
    UInt32  tmpSeed;
    UInt i;

    assert(  pNoiseSynth );

    /* generate 4* Length noise samples; store seed */
    LOCAL_GenerateNoise(&(pNoiseSynth->RandomSeed),
                        &pSampleBuffer[0],
                        4*Length);

    tmpSeed = pNoiseSynth->RandomSeed;
    
    /* generate 2* Length noise samples; DO NOT store seed */
    LOCAL_GenerateNoise(&tmpSeed,
                        &pSampleBuffer[4*Length],
                        2*Length );

    /* generate gain samples */
    LOCAL_GenerateEnvelope(pNoiseParam, pNoiseSynth->envGain, (UInt)16*Length/3);

    /* clear the rest */
    for ( i = 16*Length/3; i < 6*Length; i++ )
        pNoiseSynth->envGain[i] = 0;

    /* multiply with gain + window and store it */
    for ( i = 0; i < 6*Length; i++ )
    {
        pSampleBuffer[i] *= pWindowGainEnvelope[i] * pNoiseParam->EnvGain * pNoiseSynth->envGain[i]; 
    }

    
    for ( i = 0; i < 2*Length; i++ )
    {
        /* merge with saved 2*Length samples from previous call */
        pSampleBuffer[i] += pNoiseSynth->pBuffer[i];
        /* store last 2*Length samples for next call*/
        pNoiseSynth->pBuffer[i] = pSampleBuffer[4*Length + i];
    }

}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  SSC_NOISESYNTH_Filter
 *
 * Description :  Filters signal.
 *
 * Parameters  :  pNoiseSynth         - Pointer to instance data.
 *
 *                pNoiseParams        - Pointer to structure containing the
 *                                      reflection filter coefficients (rfc).
 *
 *                pInputSamples       - Pointer to buffer containing the samples
 *                                      to be filtered (read-only)
 *
 *                pOutputSamples      - Pointer to buffer receiving the filtered
 *                                      samples (write-only)
 *
 *                Length              - Number of samples to filter.
 *
 * Returns     : -
 *
 * Comment     : pInputSamples and pOutputSamples can point to the same buffer.
 * 
 *****************************************************************************/
void SSC_NOISESYNTH_Filter(SSC_NOISESYNTH*        pNoiseSynth,
                           SSC_NOISE_PARAM*       pNoiseParam,
                           const SSC_SAMPLE_INT*  pInputSamples,
                           SSC_SAMPLE_INT*        pOutputSamples,
                           UInt                   Length)

{
    LOCAL_ParcorFilter(pNoiseSynth,
                       pNoiseParam,
                       pInputSamples,
                       pOutputSamples,
                       Length);
    
}

/*============================================================================*/
/*       STATIC LOCAL FUNCTION IMPLEMENTATIONS                                */
/*============================================================================*/

/*************************STATIC FUNCTION**************************************
* 
* Name        : LOCAL_GaussianNoiseSource
*
* Description : White noise source for ARMA modelling synthesis
*
* Parameters  : pSeed   - Pointer to 32-bit seed number. (read-write)
*           
*               pOutput - Pointer to buffer receiving the noise samples.
*
*               Length  - Number of samples to generate.
*
* Returns:    : -
* 
*****************************************************************************/
static void LOCAL_GenerateNoise(UInt32*         pSeed,
                                SSC_SAMPLE_INT* pOutput,
                                UInt            Length)
{
    SSC_SAMPLE_INT  rp;
    SSC_SAMPLE_INT  d;
    UInt            i, j;
    UInt32          X; 

    d  = 1/4294967296.0; /* = 2^-32 */
    X  = *pSeed;

    for(i = 0; i < Length; i++)
    {
        /* Generate gaussian distrubuted random number */
        rp = 0;

        for(j = 0; j < RND_SAMPLES; j++)
        {
            X    = 1664525 * X + 1013904223;
            rp  += (SSC_SAMPLE_INT)X;
        }

        /* Store number */
        pOutput[i] = rp*d - RND_SAMPLES/2.0;
    }

    *pSeed = X;
}

/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_ParcorFilter
 *
 * Description : Filter signal based on parcor RFC filter coefficients.
 *
 * Parameters  : pNoiseSynth    - Pointer noise synthesiser instance.
 *
 *               pNoiseParam    - Pointer to structure containing the RFC filter
 *                                coefficients.
 *
 *               pInputSamples  - Pointer to buffer containing the samples to
 *                                be filtered. (read-only)
 *
 *               pOutputSamples - Pointer to buffer receiving the filtered samples.
 *                                Can be the same as pInputSamples.(write-only)
 *
 *               Length         - Number of samples to be filtered.
 *               
 * Returns:    : -
 * 
 *****************************************************************************/
static void LOCAL_ParcorFilter(SSC_NOISESYNTH*        pNoiseSynth,
                               SSC_NOISE_PARAM*       pNoiseParam,
                               const SSC_SAMPLE_INT*  pInputSamples,
                               SSC_SAMPLE_INT*        pOutputSamples,
                               UInt                   Length)
                               
{
    SSC_SAMPLE_INT a[SSC_N_MAX_FILTER_COEFFICIENTS+1];      /* A coeffs              */
    SSC_SAMPLE_INT lag[SSC_N_MAX_FILTER_COEFFICIENTS];      /* Laguerre coeffs       */
    SSC_SAMPLE_INT pole;
    UInt           n, p;

    n    = pNoiseSynth->FilterLength;
    p    = pNoiseParam->DenOrder;
    pole = pNoiseParam->PoleLaguerre;

    /* Compute A coefficients */
    memset(a, 0, sizeof(a));
    a[0] = 1;
    LOCAL_parcors2ABs(p, pNoiseParam->Denominator, a + 1);
    
    /* Convert A coefficients to Laguerre coefficients */
    memset(lag, 0, sizeof(lag));
    LOCAL_A2Lag(p, a, pole, lag);

    /* Filter signal */
    LOCAL_LagSynthFilter(pNoiseSynth->FilterState,
                         pInputSamples, pOutputSamples, Length,
                         lag, n - 1, pole);
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        :  LOCAL_LagSynthFilter
 *
 * Description :  Filters signal with Laguerre coefficients.
 *
 * Input      : z      - Pointer to buffer with filter states;
 *              in     - Pointer to buffer with input data;
 *              Length - Number of elements in the input buffer;
 *              alpha  - Pointer to buffer containing the IIR coefficients;
 *              n      - Number of filter coefficients;
 *              pole   - Laguerre pole.
 *
 * Output  : out    - Pointer to buffer in which the output data
 *                    should be stored. The output buffer must be
 *                    large enough to accommodate 'Length' number
 *                    of elements. This may be the same as the input buffer;
 *           z      - Pointer to the updated filter states. 
 *
 * Returns     : -
 *
 * Comment     : -
 * 
 *****************************************************************************/

static void LOCAL_LagSynthFilter
(   
    Double                 *states, /* In/Out: Filter states */
    const SSC_SAMPLE_INT   *r,      /* Input signal */
    SSC_SAMPLE_INT         *x,      /* Output signal */
    UInt                    N,      /* Length of r */
    const SSC_SAMPLE_INT   *alpha,  /* IIR Coefficients */
    UInt                    Lp,     /* Length of each vector in alpha */
    SSC_SAMPLE_INT          poles   /* Number of filter coefficients */
)
{
    UInt                    i, k;
    SSC_SAMPLE_INT          a[2], b[2];
    SSC_SAMPLE_INT          b1;
    SSC_SAMPLE_INT         *p_states = &states[1];
    
    /* assert(N <= 6 * SSC_MAX_SUBFRAME_SAMPLES); */
    
    
    a[0] = 1;
    a[1] = -1.0 * poles;
    
    b[0] = -1.0 * poles;
    b[1] = 1;
    
    b1 = sqrt(1 - poles * poles);
    
    for (i=0; i < N; i++)
    {
        Double out[SSC_N_MAX_FILTER_COEFFICIENTS+1];
        Double sum = 0.0;
        
        if (i > 0)
        {
           out[0] = b1 * x[i - 1]  + p_states[0];
        }
        else
        {
           out[0] = b1 * states[0] + p_states[0];
        }
            
        p_states[0] = -1.0 * a[1] * out[0];
        
        for (k = 1; k < Lp; k++)
        {
            out[k]      = b[0] * out[k - 1] + p_states[k];
            p_states[k] = b[1] * out[k - 1] - a[1] * out[k];
        }   
                
        for (k = 0; k < Lp; k++)
        {
            sum += out[k] * alpha[k];
        }
        
        x[i] = r[i] + sum;      
    }   
    
    states[0] = x[N - 1];
}   

/****************************************************************
* Name    : LOCAL_parcors2ABs
*
* Purpose : Converts parcors to a/b's
*
* Input   : nCoeffs - number of parcor coefficients
*           rfc     - parcor coefficients
*
* Output  : x       - a/b coefficients
*
* Returns : -
*
*****************************************************************/
static void LOCAL_parcors2ABs(const SInt            nCoeffs,
                              const SSC_SAMPLE_INT* rfc,
                              SSC_SAMPLE_INT*       x)
{
    SInt           i,k;
    SSC_SAMPLE_INT tmp[SSC_N_MAX_FILTER_COEFFICIENTS];

    /* iterate a's or b's */
    for(k = 0; k < nCoeffs; k++)
    {
        x[k] = - rfc[k];

        for (i = 0; i < k; i++)
        {
            x[i] = tmp[i] + rfc[k] * tmp[k-i-1];
        }

        for (i = 0; i <= k; i++)
        {
            tmp[i] = x[i];
        }
    }

    for(i = 0; i < nCoeffs; i++)
    {
        x[i] = - x[i];
    }
}

/****************************************************************
* Name    : LOCAL_A2Lag
*
* Purpose : Converts FIR coefficients to Laguerre Coefficients
*
* Input   : nCoeffs - number of parcor coefficients
*           a       - A coefficients
*           pole    - Laguerre pole
*
* Output  : x       - Laguerre coefficients
*
* Returns : -
*
*****************************************************************/
static void LOCAL_A2Lag(const SInt            nCoeffs,
                        const SSC_SAMPLE_INT* a,
                        const SSC_SAMPLE_INT  pole,
                        SSC_SAMPLE_INT*       lag)
{
    SSC_SAMPLE_INT l;
    SInt           k;
    SSC_SAMPLE_INT tmp[SSC_N_MAX_FILTER_COEFFICIENTS + 1];
    
    assert(nCoeffs <= SSC_N_MAX_FILTER_COEFFICIENTS + 1);
    
    l = (SSC_SAMPLE_INT)sqrt(1.0 - pole * pole);

    tmp[nCoeffs] = a[nCoeffs];
    for (k = nCoeffs - 1; k >= 0; k--)
    {
        tmp[k] = a[k] - tmp[k + 1] * pole;
    }
    
    for (k = 0; k < nCoeffs; k++)
    {
        lag[k] = tmp[k + 1] / tmp[0] * -l;
    }
}

/****************************************************************
* Name    : LOCAL_GenerateEnvelope
*
* Purpose : Generate envelope samples from lsf coefs
*
* Input   : pNoiseParam    - Pointer to structure containing the LSF coefficients.
*
*           Length         - Number of samples to be generated
*
* Output  : envGain        - envelope gain samples pointer
*
* Returns : -
*
*****************************************************************/
static void LOCAL_GenerateEnvelope(const SSC_NOISE_PARAM* pNoiseParam,
                                   SSC_SAMPLE_INT*        envGain, 
                                   UInt                   Length )
{
    SSC_SAMPLE_INT a[16];

    assert( pNoiseParam->LsfOrder < 16 );

    /* convert spectral lines to a params */
    memset(a, 0, sizeof(a));
    a[0] = 1;
    LOCAL_Lsf2A( (SSC_SAMPLE_INT*)(&pNoiseParam->Lsf[0]), pNoiseParam->LsfOrder, a );

    /* get samples according to a - params */
    LOCAL_Freqz(a, pNoiseParam->LsfOrder+1, envGain, Length);

}

/****************************************************************
* Name    : LOCAL_Lsf2A
*
* Purpose : converts lsf coefs to a parameters
*
* Input   : lsf      - Pointer to LSF coefficients.
*
*           lsfOrder - Number of lsf coefficients poinyted by lsf
*
* Output  : pResult  - Pointer to a-parameters
*                      ( number a-parameters is lsfOrder + 1 )
*
* Returns : -
*
*****************************************************************/
static void LOCAL_Lsf2A ( SSC_SAMPLE_INT* lsf, UInt lsfOrder, SSC_SAMPLE_INT* pResult)
{
    UInt            j, i;
    UInt            n1, n2, n3;
    UInt            n4 = 0;
    SInt            odd;
    SSC_SAMPLE_INT  xin1, xin2, xout1, xout2;

    SSC_SAMPLE_INT  w[2*SSC_N_MAX_LSF_ORDER+2];

    memset ( w, 0, sizeof(w) );
    
    odd = lsfOrder % 2;
    for ( j = 0; j < lsfOrder+1; j++)
    {
        xin1 = pResult[j];
        xin2 = pResult[j];
        
        for ( i = 0; i<(lsfOrder>>1); i++)
        {
            n1    = i * 4;
            n2    = n1 + 1;
            n3    = n2 + 1;
            n4    = n3 + 1;
            xout1 = -2.0 * cos(lsf[i * 2    ]) * w[n1] + w[n2] + xin1;
            xout2 = -2.0 * cos(lsf[i * 2 + 1]) * w[n3] + w[n4] + xin2;
            w[n2] = w[n1];
            w[n1] = xin1;
            w[n4] = w[n3];
            w[n3] = xin2;
            xin1  = xout1;
            xin2  = xout2;
        }
        /* if odd, finish with extra odd term */
        if (1 == odd) 
        {
            n1    = i * 4;
            n2    = n1 + 1;
            n4    = n2;
            xout1 = -2.0 * cos(lsf[i * 2]) * w[n1] + w[n2] + xin1;
            w[n2] = w[n1];
            w[n1] = xin1;
        } 
        else            /* else even */
        {
            xout1 = xin1 + w[n4 + 1];
        }
        
        xout2      = xin2 - w[n4 + 2];
        pResult[j] =  0.5 * (xout1 + xout2);
        
        if (1 == odd) 
        {
            w[n4 + 2] = w[n4 + 1];
            w[n4 + 1] = xin2;
        } 
        else 
        {
            w[n4 + 1] = xin1;
            w[n4 + 2] = xin2;
        }
    }
}

/****************************************************************
* Name    : LOCAL_Freqz
*
* Purpose : frequency response from only a-parameters |H(z)| = 1 / |A(z)|
*
* Input   : a       - Pointer to a-parameters ( first must be 1 )
*
*           aLen    - Number of a-parameters
*
*           Length  - Number of samples to return
*
* Output  : pResult - Pointer to frequency reponse samples
*
*
* Returns : -
*
*****************************************************************/
static void LOCAL_Freqz(SSC_SAMPLE_INT* a, UInt aLen, SSC_SAMPLE_INT* pResult, UInt Length)
{
    UInt k,i;

    for ( k = 0; k < Length; k++)
    {
        Double Re = 0.0;
        Double Im = 0.0;

        for ( i = 0; i < aLen; i++)
        {
            Re += a[i] * cos(SSC_PI / Length * k * i);
            Im -= a[i] * sin(SSC_PI / Length * k * i);
        }

        pResult[k] = 1/(sqrt(Re * Re + Im * Im));
    }
}
