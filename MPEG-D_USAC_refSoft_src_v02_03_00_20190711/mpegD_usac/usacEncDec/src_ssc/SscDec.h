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

Source file: SscDec.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001	JD	Initial version
15-Jul-2003 RJ  Added time/pitch scaling, command line based
21 Nov 2003 AG  RM4.0: ADPCM added
************************************************************************/

/******************************************************************************
 *
 *   Module              : SscDec
 *
 *   Description         : SSC Decoder API
 *
 *   Tools               : Microsoft Visual C++ v6.0
 *
 *   Target Platform     : ANSI C
 *
 *   Naming Conventions  : All function names are prefixed by SSCDEC_
 *
 *
 ******************************************************************************/

#ifndef  SSCDEC_H
#define  SSCDEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include "PlatformTypes.h"
#include "SSC_Error.h"
 
/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef Double SSC_SAMPLE_EXT;

typedef struct _SSCDEC_FRAME_PARAM
{
    UInt SampleFreq;
    UInt NrOfChannels;
    UInt FrameLength;

    UInt DecodeBaseLayerOnly;
} SSCDEC_FRAME_PARAM;

typedef struct _SSCDEC_STATISTICS
{
    int Overhead;
    int Transient;
    int SinAbs;
    int SinDiff;
    int Noise;
} SSCDEC_STATISTICS;

typedef struct _SSCDEC_INFO
{
    int StructSize ;         /* total struct size in bytes */
    int DecoderLevel ;
    int FileVersion ;
    int EncoderVersion ;
    int FormatterVersion ;
    int DecoderVersion ;
    int NumberOfFrames ;
    int Mode ;
    int ModeExt ;
}SSCDEC_INFO;

#ifdef _SSC_LIB_BUILD
#include "SscDecP.h"
#else
typedef struct _SSCDEC SSCDEC;
#endif


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void    SSCDEC_Initialise(void);

void    SSCDEC_Free(void);

SSCDEC* SSCDEC_CreateInstance(int MaxChannels, int BaseLayer, int nVariableSfSize);

void    SSCDEC_DestroyInstance(SSCDEC* pDecoder);

void    SSCDEC_Reset(SSCDEC* pDecoder);
void    SSCDEC_ResetStereoParser(SSCDEC* pDecoder);

SSC_ERROR SSCDEC_ParseFileHeader(SSCDEC*      pDecoder,
                                 const UByte* pStream,
                                 UInt         MaxHeaderLength,
                                 UInt*        pHeaderLength,
                                 SInt*        pErrorPos,
                                 UInt         nVariableSfSize);

SSC_ERROR SSCDEC_ParseSoundHeader(SSCDEC*      pDecoder,
                                  const UByte* pStream,
                                  UInt         MaxHeaderLength,
                                  UInt*        pHeaderLength,
                                  SInt*        pErrorPos);

SSC_ERROR SSCDEC_ParseFrame(SSCDEC*      pDecoder,
                            const UByte* pStream,
                            UInt         MaxFrameLength,
                            UInt*        pFrameLength,
                            SInt*        pErrorPos);

SSC_ERROR SSCDEC_SynthesiseFrame(SSCDEC*         pDecoder,
                                 SSC_SAMPLE_EXT* pOutputBuffer,
                                 Double          PitchFactor);

SSC_ERROR SSCDEC_GetFrameParameters(SSCDEC*             pDecoder,
                                    SSCDEC_FRAME_PARAM* pFrameParam);

SSC_ERROR SSCDEC_GetInfo(SSCDEC*      pDecoder,
                         SSCDEC_INFO* pInfo);

SSC_ERROR SSCDEC_GetInfoForMP4FF(SSCDEC *pDecoder,
                                 UInt   *pSamplingRate,
                                 UInt   *pPsReserved);

SSC_ERROR SSCDEC_GetStatistics(SSCDEC*            pDecoder,
                               SSCDEC_STATISTICS* pStatistics);


/* RJ: added for integration in the MP4 reference software */
#ifdef SSC_MP4REF_BUILD
SSC_ERROR SSCDEC_SetOriginalFileFormat(int DecoderLevel,
                                       int UpdateRate,
                                       int SynthesisMethod,
                                       int ModeExt,
                                       int PsReserved,
                                       SSCDEC * pDecoder,
                                       int Mode,
                                       int SamplingFrequency,
                                       int VarSfSize);

void DecSSCParseString(char *	decPara, int * pSSCDecodeBaseLayer, int * pSSCVarSfSize);

SSC_ERROR DecSSCInit(SSCDEC*	pDecoder);

SSC_ERROR DecSSCFree(SSCDEC* pDecoder);

SSC_ERROR SSCDEC_ParseFrameWrapper(SSCDEC*      pDecoder,
                                   const UByte* pStream,
                                   UInt         MaxFrameLength,
                                   UInt*        pFrameLength,
                                   SInt*        pErrorPos);

SSC_ERROR SSCDEC_SynthesiseFrameWrapper(SSCDEC* pDecoder,
                                        float *** OutSamples,
                                        int * numberOutSamples);

char *DecSSCInfo (FILE *helpStream);	/* in: print decSSC help text to helpStream */
                                      /*     if helpStream not NULL */
                                      /* returns: core version string */
#endif

#ifdef __cplusplus
}
#endif

#endif /* SSCDEC_H */
