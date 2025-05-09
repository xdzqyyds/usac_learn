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

Source file: SscDec.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits,  Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001	JD	Initial version
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
15-Jul-2003 RJ  RM3a : Added time/pitch scaling, command line based
21 Nov 2003 AG  RM4.0: ADPCM added
05 Jan 2004 AG  RM5.0: Low complexity stereo added
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


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_Error.h"
#include "SSC_Alloc.h"

#include "SscDec.h"
#include "BitstreamParser.h"
#include "SynthCtrl.h"
#include "Window.h"
#include "Merge.h"
#include "NoiseSynth.h"
#include "SineSynth.h"
#include "TransSynth.h"

#ifdef SSC_MP4REF_BUILD
#include "common_m4a.h"
#include "cmdline.h"		/* command line module */
#endif

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PROGVER "SSC decoder core 01-May-2003"

#define SEPACHAR " ,="

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

#ifdef SSC_MP4REF_BUILD
static int   SSCDecodeBaseLayer = -1;
static int   SSCVariableSfSize  = -1;
static float SSCPitchFactor     = -1.0;

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"b",&SSCDecodeBaseLayer,"%i","0",NULL,"Decode base layer only (1=yes 0=no)"},
  {"s",&SSCVariableSfSize,"%i","384",NULL,"Time scaling/Speed (=subframe size) (22..512)"},
  {"p",&SSCPitchFactor,"%f","1.0",NULL,"Pitch Factor"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};
#endif

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_Initialise
 *
 * Description : This function initialises the SSC decoder library. This
 *               function must be called before the first instance of the SSC
 *               decoder is created.
 *
 * Parameters  : -
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSCDEC_Initialise(void)
{
    SSC_BITSTREAMPARSER_Initialise();
    SSC_SYNTHCTRL_Initialise();
    SSC_WINDOW_Initialise();
    SSC_MERGE_Initialise();
    SSC_NOISESYNTH_Initialise();
    SSC_SINESYNTH_Initialise();
    SSC_TRANSSYNTH_Initialise();
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_Free
 *
 * Description : This function frees the SSC decoder library. This function
 *               must be called after the last instance of the SSC decoder has 
 *               been destroyed.
 *
 * Parameters  : -
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSCDEC_Free(void)
{
    SSC_BITSTREAMPARSER_Free();
    SSC_SYNTHCTRL_Free();
    SSC_WINDOW_Free();
    SSC_MERGE_Free();
    SSC_NOISESYNTH_Free();
    SSC_SINESYNTH_Free();
    SSC_TRANSSYNTH_Free();
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_CreateInstance
 *
 * Description : Creates (allocates memory for) SSC decoder instance
 *               
 *
 * Parameters  : int MaxChannels - the maximum no. of channels requested
 *
 *               BaseLayer       - Set to one when decoding baselayer only
 *
 *               nVariableSfSize - Length of a subframe
 *
 * Returns:    : SSCDEC* pDecoder - Decoder instance pointer
 * 
 *****************************************************************************/

SSCDEC* SSCDEC_CreateInstance(int MaxChannels, int BaseLayer, int nVariableSfSize)
{
    SSCDEC* pDecoder;

    pDecoder = SSC_MALLOC(sizeof(SSCDEC), "SSCDEC Instance");

    if (pDecoder != NULL)
    {
        pDecoder -> pSynthCtrl = SSC_SYNTHCTRL_CreateInstance(MaxChannels, BaseLayer, nVariableSfSize);
        pDecoder -> pBitstreamParser = SSC_BITSTREAMPARSER_CreateInstance(MaxChannels, nVariableSfSize);
    }

    return pDecoder; 
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_DestroyInstance
 *
 * Description : This function destroys an instance of the SSC decoder library. 
 *
 * Parameters  : SSCDEC* pDecoder - Pointer to decoder instance to destroy.
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSCDEC_DestroyInstance(SSCDEC* pDecoder)
{
    if(pDecoder == NULL)
    {
        return;
    }

    SSC_BITSTREAMPARSER_DestroyInstance(pDecoder->pBitstreamParser);
    SSC_SYNTHCTRL_DestroyInstance(pDecoder->pSynthCtrl);
    SSC_FREE(pDecoder, "SSCDEC Instance");
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_Reset
 *
 * Description : This function reset the decoder to its initial state. After
 *               executing reset function the decoder is in the same state as
 *               if it had just been created. 
 *
 * Parameters  : pDecoder - Pointer to decoder instance to reset.
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSCDEC_Reset(SSCDEC* pDecoder)
{
    assert(pDecoder);

    SSC_BITSTREAMPARSER_Reset(pDecoder->pBitstreamParser);
    SSC_SYNTHCTRL_Reset(pDecoder->pSynthCtrl, pDecoder->pBitstreamParser->Header.update_rate);
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_ResetStereoParser
 *
 * Description : This function reset the stereo parser. Only needed in MPEG4 
 *               framework.
 *
 * Parameters  : pDecoder - Pointer to decoder instance to reset.
 *
 * Returns     : -
 * 
 *****************************************************************************/
void SSCDEC_ResetStereoParser(SSCDEC* pDecoder)
{
    assert(pDecoder);

    /* Shift OCS parameter pipe by one slot */
    if (pDecoder->pBitstreamParser->h_PS_DEC->pipe_length > 1)
    {
        memmove( &pDecoder->pBitstreamParser->h_PS_DEC->aOcsParams[0], 
        &pDecoder->pBitstreamParser->h_PS_DEC->aOcsParams[1], ( pDecoder->pBitstreamParser->h_PS_DEC->pipe_length - 1 ) * sizeof( OCS_PARAMETERS ) );
    }
    /* Reset parser target */
    pDecoder->pBitstreamParser->h_PS_DEC->parsertarget = 1;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_ParseFileHeader
 *
 * Description : Parses a file header from the bitstream
 *
 * Parameters  : pDecoder        - Decoder instance pointer.
 *
 *               pStream         - Pointer to buffer containing the elementary
 *                                 bit-stream.
 *
 *               MaxHeaderLength - The number of available bytes in the pStream
 *                                 buffer.
 *
 *               pHeaderLength   - Pointer integer receicing the actual frame-
 *                                 length in bytes.
 *
 *               pErrorPos       - Not used.
 *
 *               nVariableSfSize - Length of a subframe
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/

SSC_ERROR SSCDEC_ParseFileHeader(SSCDEC*      pDecoder,
                                 const UByte* pStream,
                                 UInt         MaxHeaderLength,
                                 UInt*        pHeaderLength,
                                 SInt*        pErrorPos,
                                 UInt         nVariableSfSize)
{
    SSC_ERROR ErrorCode;

    assert(pDecoder);
    pErrorPos = pErrorPos ; /* suppress compiler warning */

    ErrorCode = SSC_BITSTREAMPARSER_ParseFileHeader(pDecoder->pBitstreamParser,
                                                    pStream,
                                                    MaxHeaderLength,
                                                    pHeaderLength,
                                                    nVariableSfSize);
    return ErrorCode;

}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_ParseSoundHeader
 *
 * Description : Parse the sound header from the stream.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               pStream         - Pointer to buffer containing the elementary
 *                                 bit-stream.
 *
 *               MaxHeaderLength - The number of available bytes in the pStream
 *                                 buffer.
 *
 *               pHeaderLength   - Pointer integer receicing the actual frame-
 *                                 length in bytes.
 *
 *               pErrorPos      - Not used.
 *
 * Returns     : Error code.
 * 
 *****************************************************************************/

SSC_ERROR SSCDEC_ParseSoundHeader(SSCDEC*      pDecoder,
                                  const UByte* pStream,
                                  UInt         MaxHeaderLength,
                                  UInt*        pHeaderLength,
                                  SInt*        pErrorPos)
{
    SSC_ERROR ErrorCode;

    assert(pDecoder);
    pErrorPos = pErrorPos ; /* suppress compiler warning */

    ErrorCode = SSC_BITSTREAMPARSER_ParseSoundHeader(pDecoder->pBitstreamParser,
                                                     pStream,
                                                     MaxHeaderLength,
                                                     pHeaderLength);
    return ErrorCode ;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_ParseFrame
 *
 * Description : Parses a single frame of the bitstream
 *
 * Parameters  : pDecoder       - Decoder instance pointer.
 *
 *               pStream        - Pointer to buffer containing the elementary
 *                                bit-stream.
 *
 *               MaxFrameLength - The number of available bytes in the pStream
 *                                buffer.
 *
 *               pFrameLength   - Pointer integer receicing the actual frame-
 *                                length in bytes.
 *
 *               pErrorPos      - Not used.
 *
 *               PitchFactor    - Scale factor for pitch
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_ParseFrame(SSCDEC*     pDecoder,
                           const UByte* pStream,
                           UInt         MaxFrameLength,
                           UInt*        pFrameLength,
                           SInt*        pErrorPos)
{
    SSC_ERROR ErrorCode;

    assert(pDecoder);
    pErrorPos = pErrorPos ; /* suppress compiler warning */

    SSC_BITSTREAMPARSER_SetMaxFrameLength(pDecoder->pBitstreamParser,MaxFrameLength);
    ErrorCode = SSC_BITSTREAMPARSER_ParseFrame(pDecoder->pBitstreamParser,
                                               pStream,
                                               pFrameLength);

    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_SynthesiseFrame
 *
 * Description : This function synthesises a frame based on the parameters
 *               received in the last parsed frame
 *               
 * Parameters  : pDecoder      - Decoder instance pointer.
 *
 *               pOutputBuffer - Pointer to buffer receiving the synthesised
 *                               samples. If there more than one channel the 
 *                               output samples are channel interleaved.
 *
 *               PitchFactor    - Scale factor for pitch
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_SynthesiseFrame(SSCDEC*         pDecoder,
                                 SSC_SAMPLE_EXT* pOutputBuffer,
                                 Double          PitchFactor)
{
    SSC_ERROR ErrorCode;
    assert(pDecoder);

    ErrorCode = SSC_SYNTHCTRL_SynthesiseFrame(pDecoder->pSynthCtrl,
                                              pDecoder->pBitstreamParser,
                                              pOutputBuffer,
                                              PitchFactor);
    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_GetFrameParameters
 *
 * Description : Retrieve the frame parameters received in the last parsed
 *               frame. Frame parameters include sampling frequency, number
 *               of channels and the number of samples per frame.
 *
 * Parameters  : pDecoder    - Decoder instance pointer
 *
 *               pFrameParam - Pointer to structure receiving the frame
 *                             parameters.
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_GetFrameParameters(SSCDEC*             pDecoder,
                                    SSCDEC_FRAME_PARAM* pFrameParam)
{
    SSC_ERROR ErrorCode;
    SSC_BITSTREAM_HEADER Header;

    assert(pDecoder);

    ErrorCode = SSC_BITSTREAMPARSER_GetHeader(pDecoder->pBitstreamParser, 
                                              &Header);

    pFrameParam->SampleFreq   = Header.sampling_rate;
    pFrameParam->NrOfChannels = Header.nrof_channels_out;
    pFrameParam->FrameLength  = Header.frame_size;
    
    /* DIRTY WAY of telling the caller how many samples to interpret */
    if ( pDecoder->pBitstreamParser->State == FirstFrame )
    {
        if ( (Header.mode == SSC_MODE_LCSTEREO) && (Header.mode_ext == SSC_MODE_EXT_PAR_STEREO) )
        {
            pFrameParam->FrameLength  = 0;
        }
    }

    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_GetInfo
 *
 * Description : Retrieve the version info of the decoded file and decoder
 *
 * Parameters  : pDecoder    - Decoder instance pointer
 *
 *               pInfo       - Pointer to structure receiving the version 
 *                             info.
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_GetInfo(SSCDEC*      pDecoder,
                         SSCDEC_INFO* pInfo)
{
    SSC_ERROR ErrorCode;
    SSC_BITSTREAM_HEADER Header;

    assert(pDecoder);

    /* do a sanity check on the StructSize parameter */
    if( pInfo == NULL )
        return SSC_ERROR_USER ;
    if( pInfo->StructSize != 9*sizeof(int) )
        return SSC_ERROR_USER | SSC_ERROR_STRUCT_FIELD ;

    ErrorCode = SSC_BITSTREAMPARSER_GetHeader(pDecoder->pBitstreamParser, 
                                              &Header);
    pInfo->DecoderLevel = Header.decoder_level ;
    pInfo->FileVersion = Header.file_version ;
    pInfo->EncoderVersion = Header.encoder_version ;
    pInfo->FormatterVersion = Header.formatter_version ;
    pInfo->DecoderVersion = Header.decoder_version ;
    pInfo->NumberOfFrames = Header.nrof_frames ;
    pInfo->Mode = Header.mode ;
    pInfo->ModeExt = Header.mode_ext ;

    return SSC_ERROR_OK ;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_GetInfoForMP4FF
 *
 * Description : Get some relevant SSC bitstream parameters to be able to
 *               write SSC FF to MP4FF
 *
 * Parameters  : pDecoder    - Decoder instance pointer
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_GetInfoForMP4FF(SSCDEC *pDecoder,
                                  UInt  *pSamplingRate,
                                  UInt  *pPsReserved)
{
    SSC_ERROR ErrorCode;
    SSC_BITSTREAM_HEADER Header;

    assert(pDecoder);

    ErrorCode = SSC_BITSTREAMPARSER_GetHeader(pDecoder->pBitstreamParser, 
                                              &Header);
    *pSamplingRate    = Header.sampling_rate;
    *pPsReserved      = Header.ps_reserved;

    return ErrorCode ;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_GetStatistics
 *
 * Description : Retrieve statistic information of the last parsed frame.
 *
 * Parameters  : pDecoder    - Decoder instance pointer.
 *
 *               pStatistics - Pointer to statistics structure.
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_GetStatistics(SSCDEC*            pDecoder,
                               SSCDEC_STATISTICS* pStatistics)
{
    SSC_ERROR ErrorCode;

    assert(pDecoder);

    ErrorCode = SSC_BITSTREAMPARSER_GetStatistics(pDecoder->pBitstreamParser,
                                                  (SSC_BITSTREAM_STATISTICS*)pStatistics);

    return ErrorCode;
}


#ifdef SSC_MP4REF_BUILD
/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSCDEC_SetOriginalFileFormat
 *
 * Description : Retrieve statistic information of the last parsed frame.
 *
 * Parameters  : pDecoder    - Decoder instance pointer.
 *
 *               pStatistics - Pointer to statistics structure.
 *
 * Returns:    : Error code.
 * 
 *****************************************************************************/
SSC_ERROR SSCDEC_SetOriginalFileFormat(int DecoderLevel,
                                       int UpdateRate,
                                       int SynthesisMethod,
                                       int ModeExt,
                                       int PsReserved,
                                       SSCDEC * pDecoder,
                                       int Mode,
                                       int SamplingFrequency,
                                       int VarSfSize)
{
    SSC_ERROR ErrorCode = SSC_ERROR_OK;

    assert(pDecoder);

  pDecoder->pBitstreamParser->Header.synthesis_method        = SynthesisMethod;
  pDecoder->pBitstreamParser->Header.decoder_level           = DecoderLevel;
  if (VarSfSize == 384)
  {
    /* Default selected, use the bitstream value */
    pDecoder->pBitstreamParser->Header.update_rate         = SSC_update_rate[UpdateRate];
  }
  else
  {
    pDecoder->pBitstreamParser->Header.update_rate         = VarSfSize;
  }

  pDecoder->pBitstreamParser->Header.enhancement             = 0;
  pDecoder->pBitstreamParser->Header.mode_ext                = ModeExt;
  pDecoder->pBitstreamParser->Header.ps_reserved             = PsReserved;
  if (Mode == 1)
  {
      pDecoder->pBitstreamParser->Header.mode				 = SSC_MODE_MONO;
  }
  else
  {
      pDecoder->pBitstreamParser->Header.mode				 = SSC_MODE_LCSTEREO;
  }
  pDecoder->pBitstreamParser->Header.sampling_rate		   = SamplingFrequency;

  /* pDecoder->pBitstreamParser->Header.amp_granularity = ; */ /*Part of each audio_frame_header */

  pDecoder->pBitstreamParser->Header.confChunk[0] = 'C';
  pDecoder->pBitstreamParser->Header.confChunk[1] = 'O';
  pDecoder->pBitstreamParser->Header.confChunk[2] = 'N';
  pDecoder->pBitstreamParser->Header.confChunk[3] = 'F';

  pDecoder->pBitstreamParser->Header.confChunkLen = 3;		/* Must be 3 */
  /* pDecoder->pBitstreamParser->Header.decoder_level */ /* Filled in at the function start */
                                /* Set to 1 */
  pDecoder->pBitstreamParser->Header.decoder_version = DECODER_VERSION;
  pDecoder->pBitstreamParser->Header.encoder_version = 0;		/* Set to unknown */
  pDecoder->pBitstreamParser->Header.enhancement = 0;			/* Set to zero */
  pDecoder->pBitstreamParser->Header.file_version = SSC_FILE_VERSION_MIN;		
                                /* File version set to 7 */
  pDecoder->pBitstreamParser->Header.formatter_version = 7;   /* Not actually used, however*/
                                /* current SSC File Format */
                                /* gave 7 as value */

  pDecoder->pBitstreamParser->Header.formChunk[0] = 'F';
  pDecoder->pBitstreamParser->Header.formChunk[1] = 'O';
  pDecoder->pBitstreamParser->Header.formChunk[2] = 'R';
  pDecoder->pBitstreamParser->Header.formChunk[3] = 'M';

  pDecoder->pBitstreamParser->Header.formChunkLen = 1;		/* May not be equal to zero */
                                /* actual byte size not important */

  if (VarSfSize == 384)
  {
    /* Default selected, use the bitstream value */
    pDecoder->pBitstreamParser->Header.frame_size = 
        SSC_MAX_SUBFRAMES * SSC_update_rate[UpdateRate];
  }
  else
  {
    pDecoder->pBitstreamParser->Header.frame_size = 
        SSC_MAX_SUBFRAMES * VarSfSize;
  }



  /* 8 * 360 = 2880 */

  /* pDecoder->pBitstreamParser->Header.freq_granularity = 0; */ /*Part of each audio_frame_header */
  /* pDecoder->pBitstreamParser->Header.mode = ; */ /* mode filled in at function start */
  /* pDecoder->pBitstreamParser->Header.mode_ext = 0; */ /* Filled in at start of function */

  switch ((pDecoder->pBitstreamParser->Header.mode))
  {
  case SSC_MODE_MONO:
    {
      pDecoder->pBitstreamParser->Header.nrof_channels_out = 1;
      pDecoder->pBitstreamParser->Header.nrof_channels_bs  = 1;
      break;
    }
  case SSC_MODE_STEREO:
    {
      pDecoder->pBitstreamParser->Header.nrof_channels_out = 2;
      switch (pDecoder->pBitstreamParser->Header.mode_ext)
      {
      case SSC_MODE_EXT_DUAL_MONO:
        pDecoder->pBitstreamParser->Header.nrof_channels_bs = 2;
        break;
      default:
        pDecoder->pBitstreamParser->Header.nrof_channels_bs   = 0;
        pDecoder->pBitstreamParser->Header.nrof_channels_out	= 0;
        break;
      }
      break;
    }
  case SSC_MODE_LCSTEREO:
    {
      pDecoder->pBitstreamParser->Header.nrof_channels_out = 2;
      switch (pDecoder->pBitstreamParser->Header.mode_ext)
      {
      case SSC_MODE_EXT_DUAL_MONO:
        pDecoder->pBitstreamParser->Header.nrof_channels_bs = 2;
        break;
      case SSC_MODE_EXT_PAR_STEREO:
        pDecoder->pBitstreamParser->Header.nrof_channels_bs = 1;
        break;
      default:
        pDecoder->pBitstreamParser->Header.nrof_channels_bs   = 0;
        pDecoder->pBitstreamParser->Header.nrof_channels_out	= 0;
        break;
      }
      break;
    }
  default:
    {
      pDecoder->pBitstreamParser->Header.nrof_channels_bs   = 0;
      pDecoder->pBitstreamParser->Header.nrof_channels_out	= 0;
      break;
    }
  }

  pDecoder->pBitstreamParser->Header.nrof_frames = 0;         /* SSC FF sound chunk nr_of_frames */
                                /* unknown. This member is just informative */
  pDecoder->pBitstreamParser->Header.nrof_subframes = SSC_MAX_SUBFRAMES;
  /* pDecoder->pBitstreamParser->Header.phase_jitter_band = ; */		/*Part of each audio_frame_header */
  /* pDecoder->pBitstreamParser->Header.phase_jitter_present = ; */	/*Part of each audio_frame_header */
  /* pDecoder->pBitstreamParser->Header.phase_max_jitter = ; */		/* Filled in when parsing a frame */
  /* pDecoder->pBitstreamParser->Header.refresh_noise = ; */			/*Part of each audio_frame_header */
  /* pDecoder->pBitstreamParser->Header.refresh_sinusoids = ; */		/*Part of each audio_frame_header */
  pDecoder->pBitstreamParser->Header.reserved = 0;					/* Must be 0 */
  pDecoder->pBitstreamParser->Header.reserved_version = 0;			/* Must be 0 */
  /* pDecoder->pBitstreamParser->Header.sampling_rate = ; */			/* sampling rate filled in at function start */

  pDecoder->pBitstreamParser->Header.soundChunk[0] = 'S';
  pDecoder->pBitstreamParser->Header.soundChunk[1] = 'C';
  pDecoder->pBitstreamParser->Header.soundChunk[2] = 'H';
  pDecoder->pBitstreamParser->Header.soundChunk[3] = 'K';

  pDecoder->pBitstreamParser->Header.soundChunkLen = 4;				/* Length unknown, must be at least 4 */
  pDecoder->pBitstreamParser->Header.synthesis_method = 0;			/* Must be 0 */

  pDecoder->pBitstreamParser->Header.typeChunk[0] = 'S';
  pDecoder->pBitstreamParser->Header.typeChunk[1] = 'S';
  pDecoder->pBitstreamParser->Header.typeChunk[2] = 'C';


  /* pDecoder->pBitstreamParser->Header.update_rate = 0; */ /* Filled in at start of function */

  pDecoder->pBitstreamParser->Header.verChunk[0] = 'V';
  pDecoder->pBitstreamParser->Header.verChunk[1] = 'E';
  pDecoder->pBitstreamParser->Header.verChunk[2] = 'R';

  pDecoder->pBitstreamParser->Header.verChunkLen = 4;		/* Must be 4 */

  return ErrorCode;
}


void DecSSCParseString(char *	decPara, int * pSSCDecodeBaseLayer, int * pSSCVarSfSize)
{
  int parac;
  int result;
  char **parav;

  /* evaluate decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
  if (result==1) {
    DecSSCInfo(stdout);
    CommonExit(1,"decoder core aborted ...");
  }
  else
    CommonExit(1,"decoder parameter string error");
  }

  *pSSCDecodeBaseLayer = SSCDecodeBaseLayer == 0 ? 0:1;
    *pSSCVarSfSize       = SSCVariableSfSize;
}



SSC_ERROR DecSSCInit(SSCDEC*	pDecoder)
{
  SSC_ERROR ErrorCode = SSC_ERROR_OK;
  int SamplesPerFrame;

  SamplesPerFrame = pDecoder->pBitstreamParser->Header.nrof_subframes *
    pDecoder->pBitstreamParser->Header.update_rate;

    pDecoder->sampleBuf = (double*)malloc(2 * SSC_MAX_FRAME_SIZE * 8 * sizeof(double));

  if (pDecoder->sampleBuf == NULL)
  {
    ErrorCode = SSC_ERROR_GENERAL|SSC_ERROR_OUT_OF_MEMORY;
  }

  pDecoder->sampleBufFloat[0] = (float*)malloc(2 * SSC_MAX_FRAME_SIZE * 8 * sizeof(float));

  if (pDecoder->sampleBufFloat[0] == NULL)
  {
    ErrorCode = SSC_ERROR_GENERAL|SSC_ERROR_OUT_OF_MEMORY;
  }

    pDecoder->sampleBufFloat[1] = (float*)malloc(2 * SSC_MAX_FRAME_SIZE * 8 * sizeof(float));

  if (pDecoder->sampleBufFloat[1] == NULL)
  {
    ErrorCode = SSC_ERROR_GENERAL|SSC_ERROR_OUT_OF_MEMORY;
  }

  return (ErrorCode);
}

SSC_ERROR DecSSCFree(SSCDEC* pDecoder)
{
    SSC_ERROR ErrorCode = SSC_ERROR_OK;

  if (pDecoder->sampleBuf != NULL)
  {
    free(pDecoder->sampleBuf);
  }
  else
  {
    ErrorCode = SSC_ERROR_WARNING|SSC_ERROR_GENERAL;
  }

  if (pDecoder->sampleBufFloat[0] != NULL)
  {
    free(pDecoder->sampleBufFloat[0]);
  }
  else
  {
    ErrorCode = SSC_ERROR_WARNING|SSC_ERROR_GENERAL;
  }

  if (pDecoder->sampleBufFloat[1] != NULL)
  {
    free(pDecoder->sampleBufFloat[1]);
  }
  else
  {
    ErrorCode = SSC_ERROR_WARNING|SSC_ERROR_GENERAL;
  }


  return (ErrorCode);
}


SSC_ERROR SSCDEC_ParseFrameWrapper(SSCDEC*      pDecoder,
                                   const UByte* pStream,
                                   UInt         MaxFrameLength,
                                   UInt*        pFrameLength,
                                   SInt*        pErrorPos)
{
  SSC_ERROR ErrorCode;

    /* RJ: this function is called with the values */
  /* of the static variables for pitch and sfsize */

  ErrorCode = SSCDEC_ParseFrame(pDecoder,
                                  pStream,
                                  MaxFrameLength,
                                  pFrameLength,
                                  pErrorPos);
  
  return ErrorCode;
}




SSC_ERROR SSCDEC_SynthesiseFrameWrapper(SSCDEC* pDecoder,
                                        float *** OutSamples,
                                        int * numberOutSamples)
{
    SSC_ERROR ErrorCode = SSC_ERROR_OK;
    int i = 0;

    *OutSamples = pDecoder->sampleBufFloat;

    *numberOutSamples = pDecoder->pBitstreamParser->Header.nrof_subframes *
      pDecoder->pBitstreamParser->Header.update_rate;

      /* RJ: this function is called with the values */
    /* of the static variables for pitch and sfsize */
    ErrorCode = SSCDEC_SynthesiseFrame(pDecoder,pDecoder->sampleBuf, SSCPitchFactor);

    /* convert the doubles to floats */

    for (i=0 ; i < *numberOutSamples; i++)
    {
      if ( (pDecoder->pBitstreamParser->Header.mode == SSC_MODE_STEREO) ||
           (pDecoder->pBitstreamParser->Header.mode == SSC_MODE_LCSTEREO) )  
      {
        pDecoder->sampleBufFloat[0][i] = (float)pDecoder->sampleBuf[2*i];
        pDecoder->sampleBufFloat[1][i] = (float)pDecoder->sampleBuf[2*i+1];
      }
      else
      {
        pDecoder->sampleBufFloat[0][i] = (float)pDecoder->sampleBuf[i];
      }
    }

    return ErrorCode;
}

char *DecSSCInfo ( FILE *helpStream)		/* in: print decSSC help text to helpStream */
/*     if helpStream not NULL */
/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
            PROGVER "\n"
            "decoder parameter string format:\n"
            "  list of tokens (tokens separated by characters in \"%s\")\n",
            SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
  }
  return PROGVER;
}
#endif
