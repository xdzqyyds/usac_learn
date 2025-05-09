/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - FFT and ODFT module by UHD



This software module was originally developed by

Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)

and edited by

Jan Dasselaar (Philips Consumer Electronics)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.



Source file: uhd_fft.c



Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
11-sep-96   HP    adapted existing code
11-feb-97   BE    optimised odft/fft
12-feb-97   BE    optimised ifft
06-jun-97   HP    added M_PI
26-jan-01   BE    float -> double
02-nov-01   JD    changed interface
**********************************************************************/

/******************************************************************************
 *
 *   Module              : FFT implementation
 *
 *   Description         : Implementation of the (i)fft process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_FFT_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include <assert.h>

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"

#include "SSC_fft.h"

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

void SSC_FFT_Initialise( void )
{
}

void SSC_FFT_Free( void )
{
}


/* allocates memory buffers and initializes the required buffers */
SSC_FFT_INFO *SSC_FFT_CreateInstance( const SInt nFftLength )
{
    SSC_FFT_INFO *pFft = NULL ;
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_FFT_ERROR_OK ;
    Try
    {
        Double pf ;
        SInt   i,j,k;

        pFft = SSC_CALLOC( sizeof(SSC_FFT_INFO), 1, "" );
        if ( pFft != NULL )
        {
            SInt nBits         = (SInt)ceil(log(nFftLength)/log(2.0));
            pFft->nFFTlength   = (SInt)pow( 2, nBits ) ;
            pFft->pdRe         = SSC_CALLOC( sizeof( *pFft->pdRe ),    pFft->nFFTlength,     "" );
            pFft->pdIm         = SSC_CALLOC( sizeof( *pFft->pdIm ),    pFft->nFFTlength,     "" );
            pFft->pdSintab     = SSC_CALLOC( sizeof( *pFft->pdSintab), pFft->nFFTlength/2+1, "" );
            pFft->pnRevtab     = SSC_CALLOC( sizeof( *pFft->pnRevtab), pFft->nFFTlength,     "" );
        }
        if( (pFft->pdRe == NULL)     || (pFft->pdIm == NULL)     ||
            (pFft->pdSintab == NULL) || (pFft->pnRevtab == NULL)
          )
        {
            Throw SSC_FFT_ERROR_OK ;
        }

        j = 0;
        for(i=1;i<=pFft->nFFTlength-2;i++)
        {
          k = pFft->nFFTlength/2;
          while(k<=j)
          {
            j = j-k;
            k = k/2;
          }
          j = j+k;
          pFft->pnRevtab[i] = j;
        }
        pFft->pnRevtab[0] = 0;
        pFft->pnRevtab[pFft->nFFTlength-1] = pFft->nFFTlength-1;

        pf = SSC_PI/pFft->nFFTlength;
        for(i=0;i<=pFft->nFFTlength/2;i++)
        {
          pFft->pdSintab[i] = sin(pf*i);
        }
    }
    Catch(Error)
    {
        SSC_FFT_DestroyInstance( &pFft );
    }
    return pFft ;
}


SInt SSC_FFT_Process( SSC_FFT_INFO * const pFft, SInt nFftLength )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_FFT_ERROR_OK ;
    Try
    {
      SInt i,j,len,len2,fftmax2,indf,of,ofmax,ba,nshift;
      Double vr,vi,fr,fi;
      Double *pxri,*pxii,*pxrj,*pxij;

      /* input bit reversal */
      nshift = 0;
      while((nFftLength<<nshift)< pFft->nFFTlength)
      {
        nshift++;
      }
      for(i=1;i<=nFftLength-2;i++)
      {
        j = pFft->pnRevtab[i]>>nshift;
        if(i<j)
        {
          vr = pFft->pdRe[i];
          pFft->pdRe[i] = pFft->pdRe[j];
          pFft->pdRe[j] = vr;
          vi = pFft->pdIm[i];
          pFft->pdIm[i] = pFft->pdIm[j];
          pFft->pdIm[j] = vi;
        }
      }

      /* butterflies */

      len = 1;
      indf = pFft->nFFTlength;
      fftmax2 = pFft->nFFTlength/2;
      while(len<nFftLength)
      {
        len2 = 2*len;
        ofmax = (len-1)/2;
        i = 0;
        for(of=0;of<len;of++)
        {
          if(of<=ofmax)
          {
	        fr = pFft->pdSintab[fftmax2-i];
	        fi = -pFft->pdSintab[i];
          }
          else
          {
	        fr = -pFft->pdSintab[i-fftmax2];
	        fi = -pFft->pdSintab[pFft->nFFTlength-i];
          }
          pxri = &pFft->pdRe[of];
          pxii = &pFft->pdIm[of];
          pxrj = &pFft->pdRe[of+len];
          pxij = &pFft->pdIm[of+len];

          for(ba=0;ba<nFftLength;ba+=len2)
          {
	        vr = *pxrj*fr-*pxij*fi;
	        vi = *pxij*fr+*pxrj*fi;
	        *pxrj = *pxri-vr;
	        *pxij = *pxii-vi;
	        *pxri = *pxri+vr;
	        *pxii = *pxii+vi;

	        pxri += len2;
	        pxii += len2;
	        pxrj += len2;
	        pxij += len2;
          }
          i += indf;
        }
        len *= 2;
        indf /= 2;
      }
    }
    Catch(Error)
    {
        Error = SSC_FFT_MEMORY_ERROR ;
    }
    return Error ;
}


SInt SSC_FFT_Process_Inverse( SSC_FFT_INFO * const pFft, SInt nFftLength )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_FFT_ERROR_OK ;
    Try
    {
          SInt i,j,len,len2,fftmax2,indf,of,ofmax,ba,nshift;
          Double vr,vi,fr,fi,fnorm;
          Double *pxri,*pxii,*pxrj,*pxij;

          /* new tables for increase of fftmax */

          nshift = 0;
          while( (nFftLength << nshift) < pFft->nFFTlength)
          {
              nshift++;
          }

          /* input bit reversal */

          for(i=1;i<=nFftLength-2;i++) {
            j = pFft->pnRevtab[i]>>nshift;
            if(i<j) {
              vr = pFft->pdRe[i];
              pFft->pdRe[i] = pFft->pdRe[j];
              pFft->pdRe[j] = vr;
              vi = pFft->pdIm[i];
              pFft->pdIm[i] = pFft->pdIm[j];
              pFft->pdIm[j] = vi;
            }
          }

          /* butterflies */

          len = 1;
          indf = pFft->nFFTlength;
          fftmax2 = pFft->nFFTlength/2;
          while(len<nFftLength)
          {
            len2 = 2*len;
            ofmax = (len-1)/2;
            i = 0;
            for(of=0;of<len;of++)
            {
              if(of<=ofmax)
              {
	            fr = pFft->pdSintab[fftmax2-i];
	            fi = pFft->pdSintab[i];
              }
              else
              {
	            fr = -pFft->pdSintab[i-fftmax2];
	            fi = pFft->pdSintab[pFft->nFFTlength-i];
              }

              pxri = &pFft->pdRe[of];
              pxii = &pFft->pdIm[of];
              pxrj = &pFft->pdRe[of+len];
              pxij = &pFft->pdIm[of+len];

              for(ba=0;ba<nFftLength;ba+=len2)
              {
	            vr = *pxrj*fr-*pxij*fi;
	            vi = *pxij*fr+*pxrj*fi;
	            *pxrj = *pxri-vr;
	            *pxij = *pxii-vi;
	            *pxri = *pxri+vr;
	            *pxii = *pxii+vi;

	            pxri += len2;
	            pxii += len2;
	            pxrj += len2;
	            pxij += len2;
              }
              i += indf;
            }
            len *= 2;
            indf /= 2;
          }
          fnorm = 1./nFftLength;
          for(i=0;i<nFftLength;i++) {
            pFft->pdRe[i] *= fnorm;
            pFft->pdIm[i] *= fnorm;
          }
    }
    Catch(Error)
    {
        Error = SSC_FFT_MEMORY_ERROR ;
    }
    return Error ;

}


/* returns a pointer to the real buffer */
Double *SSC_FFT_GetPr( const SSC_FFT_INFO * const pFft )
{
    if( pFft != NULL )
    {
        return pFft->pdRe ;
    }
    else
    {
        return NULL ;
    }
}

/* return a pointer to the imaginairy buffer */

Double *SSC_FFT_GetPi( const SSC_FFT_INFO * const pFft )
{
    if( pFft != NULL )
    {
        return pFft->pdIm ;
    }
    else
    {
        return NULL ;
    }
}


/* copies the data to the real buffer and set the rest of the buffer
 * to 0. If a NULL pointer is supplied the buffer is cleared */
SInt SSC_FFT_SetRe( const SSC_FFT_INFO * const pFft, const Double * const pdRe, const SInt nLen )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_FFT_ERROR_OK ;
    Try
    {
        if( pFft == NULL )
        {
            Throw SSC_FFT_INVALID_STATE_POINTER ;
        }
        if( (nLen > pFft->nFFTlength) || (nLen < 0) )
        {
            Throw SSC_FFT_INVALID_LENGTH ;
        }
        if( pdRe != NULL )
        {
            Try
            {
                memmove( pFft->pdRe, pdRe, nLen * sizeof(Double));
            }
            Catch(Error)
            {
                Throw SSC_FFT_INVALID_SOURCE_BUFFER ;
            }
            memset (&pFft->pdRe[nLen], 0, (pFft->nFFTlength - nLen ) * sizeof(Double) );
        }
        else
        {
            memset ( pFft->pdRe, 0, pFft->nFFTlength * sizeof(Double) );
        }
    }
    Catch(Error)
    {
    }
    return Error ;
}


/* copies the data to the imaginairy buffer and set the rest of the
 * buffer to 0. If a NULL pointer is supplied the buffer is cleared */
SInt SSC_FFT_SetIm( const SSC_FFT_INFO * const pFft, const Double * const pdIm, const SInt nLen )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    SInt Error = SSC_FFT_ERROR_OK ;
    Try
    {
        if( pFft == NULL )
        {
            Throw SSC_FFT_INVALID_STATE_POINTER ;
        }
        if( (nLen > pFft->nFFTlength) || (nLen < 0) )
        {
            Throw SSC_FFT_INVALID_LENGTH ;
        }
        if( pdIm != NULL )
        {
            Try
            {
                memmove( pFft->pdIm, pdIm, nLen * sizeof(Double));
            }
            Catch(Error)
            {
                Throw SSC_FFT_INVALID_SOURCE_BUFFER ;
            }
            memset (&pFft->pdIm[nLen], 0, (pFft->nFFTlength - nLen ) * sizeof(Double) );
        }
        else
        {
            memset ( pFft->pdIm, 0, pFft->nFFTlength * sizeof(Double) );
        }
    }
    Catch(Error)
    {
    }
    return Error ;
}

/* return the length of the allocated buffers in elements */
SInt SSC_FFT_GetLength( const SSC_FFT_INFO * const pFft )
{
    if( pFft != NULL )
    {
        return pFft->nFFTlength ;
    }
    else
    {
        return 0 ;
    }
}

/* Clears real data buffer */
void SSC_FFT_ClearRe( SSC_FFT_INFO * const pFft )
{
    if( pFft != NULL )
    {
        if( pFft->pdRe != NULL )
        {
            memset( pFft->pdRe, 0, pFft->nFFTlength * sizeof( *pFft->pdRe ) );
        }
    }
}

/* Clears imaginairy data buffer */
void SSC_FFT_ClearIm( SSC_FFT_INFO * const pFft )
{
    if( pFft != NULL )
    {
        if( pFft->pdIm != NULL )
        {
            memset( pFft->pdIm, 0, pFft->nFFTlength * sizeof( *pFft->pdIm ) );
        }
    }
}

/* frees all allocated memory */
void SSC_FFT_DestroyInstance( SSC_FFT_INFO **ppFft )
{
    SSC_FFT_INFO *pFft = *ppFft ;
    if ( pFft != NULL )
    {
        SSC_FREE( pFft->pdRe,     "" );
        SSC_FREE( pFft->pdIm,     "" );
        SSC_FREE( pFft->pdSintab, "" );
        SSC_FREE( pFft->pnRevtab, "" );

        SSC_FREE( pFft,           "" );
    }
    *ppFft = NULL;
}
