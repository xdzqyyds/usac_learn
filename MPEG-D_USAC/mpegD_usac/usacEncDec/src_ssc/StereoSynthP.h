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

Source file: StereoSynthP.h

Required libraries: <none>

Authors:
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
16-Sep-2002 TM  Initial version ( m8541 )
15-Jul-2003 RJ  RM3a: Added time/pitch scaling, command line based
************************************************************************/

/******************************************************************************
 *
 *   Module              : Stereo synthesiser
 *
 *   Description         : Generates Stereo Components
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All function names are prefixed by SSC_STEREOSYNTH_
 *
 *   Build options       : 
 *
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_fft.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define FFT_LENGTH              (4096)
#define NUM_COMPLEX_SAMPLES     (FFT_LENGTH/2 + 1)
#define LEN_SCHROEDER_LONG      (512)
#define LEN_SCHROEDER_SHORT     (32)

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef union _SSC_STS_RATE_PIPE
{
    struct _SSC_STS_RATE_PIPE_BYTES
    {
        UByte  current;
        UByte  hist1;
        UByte  hist2;
        UByte  hist3;
    } rate;

    UInt32    pipe;

} SSC_STS_RATE_PIPE;

typedef struct _SSC_STEREOSYNTH
{
    SSC_SAMPLE_INT  pDecorL[LEN_SCHROEDER_LONG ];
    SSC_SAMPLE_INT  pDecorS[LEN_SCHROEDER_SHORT];

    SSC_STS_RATE_PIPE updateRateIdx;
    UInt            decodeStereoBaseLayerOnly;

    SSC_SAMPLE_INT* pMonoSignal;

    Double          kernels0[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS];
    Double          kernels1[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS];
    Double          kernels2[NUM_COMPLEX_SAMPLES][SSC_ENCODER_MAX_STEREO_BINS];

    SSC_FFT_INFO*   pFft;

    SSC_SAMPLE_INT* pWin0L;
    SSC_SAMPLE_INT* pWin0S;
    SInt            len0L;
    SInt            len0S;

    SSC_SAMPLE_INT* pWin1L;
    SSC_SAMPLE_INT* pWin1S;
    SInt            len1L;
    SInt            len1S;

    SSC_SAMPLE_INT* pWin2L;
    SSC_SAMPLE_INT* pWin2S;
    SInt            len2L;
    SInt            len2S;

    SSC_SAMPLE_INT* pDecorTemp;
    SSC_SAMPLE_INT* pWindow;
    SSC_SAMPLE_INT* pWinSwitch;

    SSC_SAMPLE_INT* pWindowedMono;
    SSC_SAMPLE_INT* pWindowedDecor;

    SSC_SAMPLE_INT* pSignalLeft;
    SSC_SAMPLE_INT* pSignalRight;
} SSC_STEREOSYNTH;
