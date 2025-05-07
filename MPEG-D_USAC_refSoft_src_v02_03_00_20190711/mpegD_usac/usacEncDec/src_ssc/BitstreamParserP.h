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

Source file: BitstreamParserP.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001 JD  Initial version
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
28 Nov 2002 AR  RM3.0: Laguerre added, num removed
21 Nov 2003 AG  RM4.0: ADPCM added
05 Jan 2004 AG  RM5.0: Low complexity stereo added
************************************************************************/

/******************************************************************************
 *
 *   Module              : Bit-stream parser
 *
 *   Description         : Implementation of the SSC bit-stream parser.
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All public functions must be prefixed by
 *                         SSC_BITSTREAMPARSER_
 *
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include "SSC_BitStrm_Types.h"
#include "Bits.h"
#include "Log.h"
#include "ct_psdec_ssc.h"

#define DECODER_VERSION     2

#define HISTORY_TRANSIENT   1
#define HISTORY_SINUSOID    1
#define FUTURE_SINUSOID     2
#define HISTORY_NOISE       4
#define HISTORY_STEREO      4

typedef struct _SSC_BITSTREAM_ERROR
{
    SSC_ERROR   ErrorCode;
    UInt        Position;
    SInt        SubFrame;
    UInt        Channel;
} SSC_BITSTREAM_ERROR;

typedef enum _SSC_PARSER_STATE
{
    Reset, FirstFrame, Streaming, Error
} SSC_PARSER_STATE;


typedef struct _SSC_FRAME_QPARAM
{
  SSC_TRANSIENT_QPARAM  Transient[SSC_MAX_SUBFRAMES+HISTORY_TRANSIENT][SSC_MAX_CHANNELS];
  SSC_SINUSOID_QPARAM   Sinusoid[HISTORY_SINUSOID+SSC_MAX_SUBFRAMES+FUTURE_SINUSOID][SSC_MAX_CHANNELS];
  SSC_NOISE_QPARAM      Noise[SSC_MAX_SUBFRAMES+HISTORY_NOISE][SSC_MAX_CHANNELS];

  SSC_STEREO_QPARAM     Stereo[SSC_MAX_SUBFRAMES+HISTORY_STEREO];

                        /* num/den of previous and current frame */
  UInt                  n_nrof_den[2] ;
  UInt                  n_nrof_lsf[2] ;
                        /* set to True when first absolute values have been read */
  Bool                  bNoiseValid ;
  Bool                  bStereoValid ;
  SInt                  nTrackID ; /* id of track */
  UInt32                nJitterRandomSeed[SSC_MAX_CHANNELS] ;

} SSC_FRAME_QPARAM;

typedef SSC_FRAME_QPARAM SSC_BITSTREAM_DATA ;


typedef struct _SSC_BITSTREAMPARSER
{
    SSC_BITSTREAM_HEADER     Header;
    SSC_BITSTREAM_STATISTICS Statistics;

    SSC_BITSTREAM_DATA*      pBitStreamData;

    SSC_PARSER_STATE         State;

    HANDLE_PS_DEC            h_PS_DEC;

    SSC_BITSTREAM_ERROR      Error;
    UInt                     MaxNrOfChannels;
    UInt                     MaxFrameLength;
    UInt                     FramesParsed;
} SSC_BITSTREAMPARSER;

