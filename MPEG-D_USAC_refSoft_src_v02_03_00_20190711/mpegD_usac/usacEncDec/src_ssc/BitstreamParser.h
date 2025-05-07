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

Source file: BitstreamParser.h

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
15-Jul-2003 RJ  RM3a : Added time/pitch scaling, command line based
21 Nov 2003 AG  RM4.0: ADPCM added
************************************************************************/

/******************************************************************************
 *
 *   Module              :  Bit-stream parser
 *
 *   Description         :
 *
 *   Tools               :  Microsoft Visual C++
 *
 *   Target Platform     :  ANSI-C
 *
 *   Naming Conventions  :  All functions are prefixed by 'SSC_BITSTREAMPARSER_'
 *
 *
 ******************************************************************************/
#ifndef  BITSTREAMPARSER_H
#define  BITSTREAMPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSC_SINSTATE_BIRTH   0x01
#define SSC_SINSTATE_DEATH   0x02
#define SSC_SINSTATE_VALID   0x04

#define SSC_STATE_MASK       0x0000FFFF /* mask to get state bits */
#define SSC_SINMASK_FADJ     0xFF000000 /* mask for frequency adjust bits */
#define SSC_FADJ_SHIFT       24         /* times to shift to get signed Fadj value (-64..63) */

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _SSC_BITSTREAM_HEADER
{
  /* literal file header parameters */
  Char                  formChunk[4] ;       /* 'FORM' */
  UInt                  formChunkLen ;
  Char                  typeChunk[4] ;       /* 'SSC ' */
  Char                  verChunk[4] ;        /* 'VER ' */
  UInt                  verChunkLen ;        /* 16      */
  UInt                  file_version ;
  UInt                  encoder_version ;
  UInt                  formatter_version ;
  UInt                  reserved_version ;
  Char                  confChunk[4] ;       /* 'CONF' */
  UInt                  confChunkLen ;       /* 3      */
  UInt                  decoder_level;
  UInt                  sampling_rate;
  UInt                  update_rate;
  UInt                  synthesis_method;
  UInt                  enhancement;
  UInt                  mode;
  UInt                  mode_ext;
  UInt                  ps_reserved;
  UInt                  freq_granularity;
  UInt                  amp_granularity;
  UInt                  reserved;

  /* Derived parameters */
  UInt                  nrof_subframes;
  UInt                  nrof_channels_out;
  UInt                  nrof_channels_bs;
  UInt                  frame_size;

  /* sound header */
  Char                  soundChunk[4] ;      /* 'SCHK' */
  UInt                  soundChunkLen ;      /*        */
  UInt                  nrof_frames;

  /* audio frame header parameters */
  UInt                  refresh_sinusoids ;
  UInt                  refresh_sinusoids_next_frame ;
  UInt                  refresh_noise ;
  UInt                  refresh_stereo ;

  UInt                  stereo_upd_rate;
  UInt                  stereo_nrof_bins_idx ;
  UInt                  stereo_nrof_bins_itd_idx ;

  UInt                  phase_jitter_present ;
  UInt                  phase_max_jitter ;
  UInt                  phase_jitter_band ;
  /* version of decoder */
  UInt                  decoder_version ;

} SSC_BITSTREAM_HEADER;


typedef struct _SSC_BITSTREAM_STATISTICS
{
    int Overhead;
    int Transient;
    int SinAbs;
    int SinDiff;
    int Noise;
    int Stereo;
} SSC_BITSTREAM_STATISTICS;


#ifdef _SSC_LIB_BUILD
#include "BitstreamParserP.h"
#else
typedef struct _SSC_BITSTREAMPARSER SSC_BITSTREAMPARSER;
#endif

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void                  SSC_BITSTREAMPARSER_Initialise(void);
void                  SSC_BITSTREAMPARSER_Free(void);

SSC_BITSTREAMPARSER*  SSC_BITSTREAMPARSER_CreateInstance(UInt MaxChannels, UInt nVariableSfSize); 
void                  SSC_BITSTREAMPARSER_DestroyInstance(SSC_BITSTREAMPARSER* pBitStreamParser); 


void                  SSC_BITSTREAMPARSER_Reset(SSC_BITSTREAMPARSER* pBitStreamParser);

void                  SSC_BITSTREAMPARSER_SetMaxFrameLength(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                            UInt                 MaxFrameLength);

SSC_ERROR             SSC_BITSTREAMPARSER_ParseFileHeader(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                          const UByte*         pStream,
                                                          const UInt           BufLength,
                                                          UInt*                pFileHeaderLength,
                                                          UInt                 nVariableSfSize);

SSC_ERROR             SSC_BITSTREAMPARSER_ParseSoundHeader(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                           const UByte*         pStream,
                                                           const UInt           BufLength,
                                                           UInt*                pSoundHeaderLength);

SSC_ERROR             SSC_BITSTREAMPARSER_ParseFrame(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                     const UByte*         pStream,
                                                     UInt*                pFrameLength);
 
SSC_ERROR             SSC_BITSTREAMPARSER_GetHeader(SSC_BITSTREAMPARSER*  pBitstreamParser,
                                                    SSC_BITSTREAM_HEADER* pHeader);

SSC_ERROR             SSC_BITSTREAMPARSER_GetStatistics(SSC_BITSTREAMPARSER*       pBitstreamParser,
                                                        SSC_BITSTREAM_STATISTICS*  pStatistics);

UInt                  SSC_BITSTREAMPARSER_GetNumberOfTransients(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                                UInt                 Channel);

UInt                  SSC_BITSTREAMPARSER_GetFrameSize(SSC_BITSTREAMPARSER* pBitstreamParser);

SSC_ERROR             SSC_BITSTREAMPARSER_GetTransient(SSC_BITSTREAMPARSER*  pBitstreamParser,
                                                       UInt                  Channel,
                                                       UInt                  Number,
                                                       SInt*                 pSubframe, 
                                                       SSC_TRANSIENT_PARAM*  pTransient);

UInt                  SSC_BITSTREAMPARSER_GetSinusoids(SSC_BITSTREAMPARSER* pBitstreamParser,
                                                       SInt                 Subframe,
                                                       UInt                 Channel,
                                                       Double               PitchFactor,
                                                       SSC_SIN_SEGMENT*     pSinusoids);

SSC_ERROR             SSC_BITSTREAMPARSER_GetNoise(SSC_BITSTREAMPARSER*     pBitstreamParser,
                                                   SInt                     Subframe,
                                                   UInt                     Channel,
                                                   SSC_NOISE_PARAM*         pNoiseParam);


#ifdef __cplusplus
}
#endif

#endif /* BITSTREAMPARSER_H */
