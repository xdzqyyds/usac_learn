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

Source file: SSC_fft.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
25-Sep-2001	AP/JD	Initial version
************************************************************************/

/************************************************************************** 
 *
 * FFT module implementation
 *
 * SSC_fft.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __SSC_FFT_h
#define __SSC_FFT_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSC_FFT_ERROR_OK              0
#define SSC_FFT_MEMORY_ERROR          1
#define SSC_FFT_INVALID_LENGTH        2
#define SSC_FFT_INVALID_SOURCE_BUFFER 3
#define SSC_FFT_INVALID_STATE_POINTER 4
/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct fftinfo_tag
{
    SInt    nFFTlength ;
    Double *pdRe ;
    Double *pdIm ;
    Double *pdSintab ;
    SInt   *pnRevtab ;
}SSC_FFT_INFO, *PSSC_FFT_INFO;



/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void          SSC_FFT_Initialise( void );
void          SSC_FFT_Free( void );

SSC_FFT_INFO *SSC_FFT_CreateInstance ( const SInt nFftLength );
void          SSC_FFT_DestroyInstance( SSC_FFT_INFO **ppFft );

/* do the actual FFT */
SInt          SSC_FFT_Process        ( SSC_FFT_INFO * const pFft, SInt nFftLength );
SInt          SSC_FFT_Process_Inverse( SSC_FFT_INFO * const pFft, SInt nFftLength );

/* support function to prepare for FFT process */
Double *      SSC_FFT_GetPr          ( const SSC_FFT_INFO * const pFft );
Double *      SSC_FFT_GetPi          ( const SSC_FFT_INFO * const pFft );
SInt          SSC_FFT_GetLength      ( const SSC_FFT_INFO * const pFft );
void          SSC_FFT_ClearRe( SSC_FFT_INFO * const pFft );
void          SSC_FFT_ClearIm( SSC_FFT_INFO * const pFft );
SInt          SSC_FFT_SetRe( const SSC_FFT_INFO * const pFft, const Double * const pdIm, const SInt nLen );
SInt          SSC_FFT_SetIm( const SSC_FFT_INFO * const pFft, const Double * const pdIm, const SInt nLen );

#ifdef __cplusplus
}
#endif /* __SSC_FFT_h */

#endif



