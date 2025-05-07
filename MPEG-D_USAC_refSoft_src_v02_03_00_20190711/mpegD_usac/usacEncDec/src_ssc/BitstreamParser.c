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

Source file: BitstreamParser.c

Required libraries: <none>

Authors:
AP: Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD: Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO: Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
06-Sep-2001 JD  Initial version
03 Dec 2001 JD  m7725 change (different delta gain coding)
03 Dec 2001 JD  m7724 low complexity huffman decoding
25 Jul 2002 TM  RM1.5: m8540 improved noise coding + 8 sf/frame
07 Oct 2002 TM  RM2.0: m8541 parametric stereo coding
28 Nov 2002 AR  RM3.0: Laguerre added, num removed
15 Jul 2003 RJ  RM3a : Added time/pitch scaling, command line based
05 Nov 2003 AG  RM4  : Freq/Phase ADPCM added.
05 Jan 2004 AG  RM5  : Low complexity stereo added.
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
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_SigProc_Types.h"
#include "SSC_BitStrm_Types.h"
#include "SSC_BitStream.h"
#include "SSC_Error.h"
#include "SSC_Alloc.h"

#include "BitstreamParser.h"
#include "Bits.h"
#include "Log.h"

#include "ct_psdec_ssc.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS (Look-up tables)                                */
/*============================================================================*/

extern const HUFFMAN_DECODER huf_s_cont;
extern const HUFFMAN_DECODER huf_s_nrof_births;
extern const HUFFMAN_DECODER hufNoiRelNdgE;
extern const HUFFMAN_DECODER hufNoiRelLsfE;
extern const HUFFMAN_DECODER hufNoiRelLag;

extern const HUFFMAN_DECODER hufAmpContRel_0;
extern const HUFFMAN_DECODER hufAmpContRel_1;
extern const HUFFMAN_DECODER hufAmpContRel_2;
extern const HUFFMAN_DECODER hufAmpContRel_3;

extern const HUFFMAN_DECODER hufFreqContRel_0;
extern const HUFFMAN_DECODER hufFreqContRel_1;
extern const HUFFMAN_DECODER hufFreqContRel_2;
extern const HUFFMAN_DECODER hufFreqContRel_3;

extern const HUFFMAN_DECODER hufAmpBirthRel;
extern const HUFFMAN_DECODER hufAmpSinAbs;
extern const HUFFMAN_DECODER hufFreqBirthRel;
extern const HUFFMAN_DECODER hufFreqSinAbs;
extern const HUFFMAN_DECODER hufFreqContAbs;
extern const HUFFMAN_DECODER hufAmpContAbs;

extern const HUFFMAN_DECODER hufCoarseIid;
extern const HUFFMAN_DECODER hufCoarseItd;
extern const HUFFMAN_DECODER hufCoarseRho;
extern const HUFFMAN_DECODER hufCoarseRhoGlobal;

extern const HUFFMAN_DECODER hufFineIid;
extern const HUFFMAN_DECODER hufFineItd;
extern const HUFFMAN_DECODER hufFineRho;
extern const HUFFMAN_DECODER hufFineRhoGlobal;

extern const HUFFMAN_DECODER hufAdpcmGrid;

/*============================================================================*/
/*       CONSTANT DEFINITIONS (ADPCM quantiser)                               */
/*============================================================================*/
extern const Double SSC_ADPCM_TableRepr[];
extern const Double SSC_ADPCM_TableQuant[];

extern const Double SSC_ADPCM_FF[]; 
extern const Double SSC_ADPCM_FS[];
extern const Double SSC_ADPCM_Q[]; 
extern const Double SSC_ADPCM_R[];

extern const Double SSC_ADPCM_delta_min;
extern const Double SSC_ADPCM_delta_max;

extern const Double SSC_ADPCM_P_h[];
extern const Double SSC_ADPCM_P_m[];    

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/
#define STATISTICS_RESET(pStat)              memset((pStat), 0, sizeof(*pStat))
#define STATISTICS_START(pBits, pStat, Item) (pStat)->Item -= SSC_BITS_GetOffset(pBits)
#define STATISTICS_END(pBits, pStat, Item)   (pStat)->Item += SSC_BITS_GetOffset(pBits)

#define UPDATE_ERROR(pError,pBits,ErrorCode) \
                                       if(SSC_ERROR_CATAGORY(ErrorCode) >  SSC_ERROR_CATAGORY((pError)->ErrorCode)) \
                                       {                                                                            \
                                            (pError)->ErrorCode = ErrorCode;                                        \
                                            (pError)->Position  = SSC_BITS_GetOffset(pBits);                        \
                                            (pError)->SubFrame  = sf;                                               \
                                            (pError)->Channel   = ch;                                               \
                                       }

#define JITTER_SCALE_FACTOR      (8.0)

#define T                        (SSC_MAX_SUBFRAME_SAMPLES)

#define SSC_QMF_SUBFRAME_LENGTH  (NO_QMF_CHANNELS)
#define SSC_UPDATE_OCS_QMF       (2)

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_PrepareParameterSet
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        UInt                NrOfSubFrames,
        UInt                NrOfChannels
    );

static SSC_SINUSOID_QPARAM* LOCAL_GetSinParamBlock
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        SInt                Sf,
        UInt                Ch
    );

static SSC_NOISE_QPARAM* LOCAL_GetNoiseParamBlock
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        SInt                Sf,
        UInt                Ch
    );

static SSC_TRANSIENT_QPARAM* LOCAL_GetTransientParamBlock
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        SInt                Sf,
        UInt                Ch
    );

static UInt* LOCAL_GetNoiseDenominators
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        SSC_NOISE_NUM       noiseFrame
    );

static UInt* LOCAL_GetNoiseLsfs
    (
        SSC_BITSTREAM_DATA* pBitStreamData,
        SSC_NOISE_NUM       noiseFrame
    );

static SSC_ERROR LOCAL_ParseFileHeader
    (
        SSC_BITS*             pBits,
        SSC_BITSTREAMPARSER*  pBitstreamParser,
        SSC_EXCEPTION*        the_exception_context,
        UInt                  nVariableSfSize
    );

static SSC_ERROR LOCAL_ParseSoundHeader
    (
        SSC_BITS*             pBits,
        SSC_BITSTREAMPARSER*  pBitstreamParser,
        SSC_EXCEPTION*        the_exception_context
    );

static SSC_ERROR LOCAL_ParseSubframeTransient
    (
        SSC_BITS*             pBits,
        UInt                  SubframeSize,
        SSC_TRANSIENT_QPARAM* pTransient,
        SSC_EXCEPTION*        the_exception_context
    );

static SSC_ERROR LOCAL_ParseMeixnerParam
    (
        SSC_BITS*             pBits,
        SSC_MEIXNER_QPARAM*   pMeixner,
        SSC_EXCEPTION*        the_exception_context
    );

static SSC_ERROR LOCAL_ParseSubframeSinusoid
    (
        SSC_BITS*                   pBits,
        const SSC_BITSTREAM_HEADER* pHeader,
        SSC_BITSTREAM_DATA*         pBitStreamData,
        SSC_SINUSOID_QPARAM*        pSinusoid,
        SInt                        Sf,
        UInt                        Ch,
        SSC_BITSTREAM_STATISTICS*   pStat,
        SSC_EXCEPTION*              the_exception_context
    );

static void LOCAL_ParseSinDiff
    (
        SSC_BITS*                   pBits,
        SSC_BITSTREAM_DATA*         pBitStreamData,
        SSC_SIN_QSEGMENT*           pSinSeg,
        const SSC_BITSTREAM_HEADER* pHeader,
        UInt                        NrOfSinusoids,
        SInt                        Sf,
        UInt                        Ch,
        UInt*                        p,
        UInt*                        q,
        SSC_EXCEPTION*              the_exception_context
    );

static void LOCAL_ParseSinAbs
    (
        SSC_BITS*                   pBits,
        SSC_BITSTREAM_DATA*         pBitStreamData,
        SSC_SIN_QSEGMENT*           pSinParam,
        const SSC_BITSTREAM_HEADER* pHeader,
        UInt                        NrOfSinusoids,
        SInt                        Sf,
        UInt                        Ch,
        UInt*                        p,
        UInt*                        q,
        Bool                        bContinuations,
        SSC_EXCEPTION*              the_exception_context
    );

static SSC_ERROR LOCAL_ParseSubframeNoise
    (
        SSC_BITS*                   pBits,
        const SSC_BITSTREAM_HEADER* pHeader,
        SSC_BITSTREAM_DATA*         pBitStreamData,
        SSC_NOISE_QPARAM*           pNoiseParam,
        SSC_NOISE_QPARAM*           pPrev2NoiseParam,
        SSC_NOISE_QPARAM*           pPrev4NoiseParam,
        UInt*                       pDenominators,
        UInt*                       pLsfs,
        SInt                        Sf,
        SSC_EXCEPTION*              the_exception_context
     );

static SSC_ERROR LOCAL_PreDecodeNextUpdate
    (
        SSC_SIN_QSEGMENT*   pSinSeg,
        SInt                Sf,
        UInt                Ch,
        SSC_BITSTREAM_DATA* pBitStreamData,
        SSC_EXCEPTION*      the_exception_context
    );

static Double LOCAL_DequantFreq
    (
        Double dFreq,
        UInt   uSamplingRate
    );

static Double LOCAL_DequantAmpl
    (
        SInt Ampl
    );

static Double LOCAL_DequantTransientAmpl
    (
        SInt Ampl
    );

static Double LOCAL_DequantPhase
    (
        SInt Phase
    );

static Double LOCAL_DequantPhasePrecise
    (
        SInt Phase
    );

static SInt LOCAL_QuantPhasePrecise
    (
        Double dPhase
    );

static SInt LOCAL_RequantPhase
    (
        SInt Phase
    );

static Double LOCAL_DequantLAR
    (
        SInt LAR
    );

static Double LOCAL_DequantLsf
    (
        SInt Lsf
    );

static Double LOCAL_CalcContinuousPhase
    (
        Double            OldFreq,
        Double            OldPhase,
        Double            NewFreq,
        UInt              SegmentSize,
        Double            PitchFactor
    );

static void LOCAL_ReadChunkID
    (
        SSC_BITS *pBits,
        Char *szChunkID
    );

static Bool LOCAL_CheckChunkID
    (
        const Char *szChunkExp,
        const Char *szChunkRead
    );


static SInt LOCAL_ReadAmpBirthRel
    (
        SSC_BITS *pBits,
        UInt      nAmpGranularity
    );

static UInt LOCAL_ReadAmpContAbs
    (
        SSC_BITS *pBits,
        UInt      nAmpGranularity
    );

static UInt LOCAL_ReadAmpSinAbs
    (
        SSC_BITS *pBits,
        UInt      nAmpGranularity
    );

static UInt LOCAL_ReadFreqBirthRel
    (
        SSC_BITS *pBits,
        UInt      nFreqGranularity
    );

static UInt LOCAL_ReadFreqContAbs
    (
        SSC_BITS *pBits,
        UInt      nFreqGranularity
    );

static UInt LOCAL_ReadFreqSinAbs
    (
        SSC_BITS *pBits,
        UInt      nFreqGranularity
    );

static SInt LOCAL_ReadAmpDelta
    (
        SSC_BITS *pBits,
        UInt      nAmpGranularity
    );

static UInt LOCAL_ReadPaddingBits
    (
        SSC_BITS *pBits,
        struct exception_context *the_exception_context
    );

static UInt ceil_log2(UInt x)
{
    UInt  t = 1;

    while((UInt)(1 << t) < x)
    {
       t++;
    }

    return t;
}

static SInt LOCAL_CalcJitter
    ( 
        const SSC_BITSTREAM_HEADER *pHeader,
        SSC_BITSTREAM_DATA* pBitStreamData,
        SInt nCh
    );

static SInt LOCAL_ConvertToGrid
    ( 
        UInt val, 
        UInt grid 
    );

static SInt LOCAL_ReadAdpcm
    ( 
        SSC_BITS *pBits 
    );

static UInt LOCAL_ReadAdpcmGrid
    ( 
        SSC_BITS *pBits 
    );

static Double LOCAL_Freq2erb
    ( 
        Double x 
    );

static SInt LOCAL_QuantFreq
    ( 
        Double dFreq, 
        Double dFreq_rl_base, 
        Double dFs, 
        SInt grid_freq 
    );

static SInt LOCAL_DecodeAdpcm
    (
        SSC_BITSTREAMPARSER*  pCurrFrame,
        SInt                  Subframe,
        UInt                  Channel,
        Double                PitchFactor,
        SSC_SIN_SEGMENT*      pSinusoids
    );

static void LOCAL_DecodeAdpcmTrack
    ( 
        UInt            length,      /* In: This flag indicates birth, continuation and first continuation */
        SSC_SIN_ADPCM   * s_adpcmp1, /* In:     Current sub frame - 1 */
        SSC_SIN_ADPCM   * s_adpcm0,  /* In/Out: Current sub frame     */
        SSC_SIN_ADPCM   * s_adpcm1,  /* In/Out: Current sub frame + 1 */
        SSC_SIN_ADPCM   * s_adpcm2,  /* In/Out: Current sub frame + 2 */
        Double          * Frequency, /* Out: Frequency */
        Double          * Phase      /* Out: Phase     */
    );
    
static void LOCAL_decode_ADPCM_frame
    (
        SSC_SIN_ADPCM_STATE * s_adpcm0, 
        UInt AdpcmIn
    );

static void LOCAL_update_delta
    (
        Double * delta,
        Double * repr,
        Double C_next,
        const Double delta_max,
        const Double delta_min,
        const Double * m
    );

static void LOCAL_InitADPCMState
    (
        SSC_SIN_ADPCM* s_adpcmp1, 
        SSC_SIN_ADPCM_STATE * s_adpcm_state
    );

static void LOCAL_UpdateADPCMState
    (
        SSC_SIN_ADPCM_STATE * s_adpcm0,
        UInt AdpcmIn0, 
        SSC_SIN_ADPCM_STATE * s_adpcm1
    );

static void LOCAL_FiltPhaseTrack
    (
        SSC_SIN_ADPCM_STATE * s_adpcmp1, 
        SSC_SIN_ADPCM_STATE * s_adpcm0,  
        SSC_SIN_ADPCM_STATE * s_adpcm1
    );

static void LOCAL_FiltPhaseEnd
    (
        SSC_SIN_ADPCM_STATE * s_adpcm0
    );

static void LOCAL_CalcFreqPhase
    (
        SSC_SIN_ADPCM_STATE * s_adpcmp1, 
        SSC_SIN_ADPCM_STATE * s_adpcm0
    );

static void LOCAL_FiltFreqFirst
    (
        SSC_SIN_ADPCM_STATE * s_adpcm0,  
        SSC_SIN_ADPCM_STATE * s_adpcm1
    );

static void LOCAL_FiltFreqTrack
    (
        SSC_SIN_ADPCM_STATE * s_adpcmp1, 
        SSC_SIN_ADPCM_STATE * s_adpcm0,  
        SSC_SIN_ADPCM_STATE * s_adpcm1
    );
            
static void LOCAL_FiltFreqEnd
    (
        SSC_SIN_ADPCM_STATE * s_adpcmp1, 
        SSC_SIN_ADPCM_STATE * s_adpcm0
    );        

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_Initialise
 *
 * Description : Initialises the bit-stream parser module.
 *               This function must be called before the first instance of
 *               the bit-stream parser is created.
 *
 * Parameters  : -
 *
 * Returns     : -
 *
 *****************************************************************************/
void SSC_BITSTREAMPARSER_Initialise(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_Free
 *
 * Description : Frees the bit-stream parser module, undoing the effects
 *               of the SSC_BITSTREAMPARSER_Initialise() function.
 *               This function must be called after the last instance of
 *               the bit-stream parser is destroyed.
 *
 * Parameters  :  -
 *
 * Returns     : -
 *
 *****************************************************************************/
void SSC_BITSTREAMPARSER_Free(void)
{
    return;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_CreateInstance
 *
 * Description : Creates an instance of the bit-stream parser.
 *
 * Parameters  : MaxNrOfChannels - The maximum number of channels that the
 *                                 bit-stream parser should be able to handle.
 *
 * Returns     : Pointer to instance, or NULL if insufficient memory.
 *
 *****************************************************************************/
SSC_BITSTREAMPARSER* SSC_BITSTREAMPARSER_CreateInstance(UInt MaxNrOfChannels, UInt nVariableSfSize)
{
    SSC_BITSTREAMPARSER* pBitstreamParser;
    SSC_BITSTREAM_DATA* pBitStreamData;
    UInt nNrSlots, MaxSfSize; 
    UInt Error; 
    
    MaxSfSize = (UInt)(((Double)nVariableSfSize/(Double)SSC_QMF_SUBFRAME_LENGTH)+1.0) *  SSC_QMF_SUBFRAME_LENGTH;
    nNrSlots = 2 * (MaxSfSize/NO_QMF_CHANNELS) * SSC_MAX_SUBFRAMES/SSC_UPDATE_OCS_QMF;

    if(MaxNrOfChannels > SSC_MAX_CHANNELS)
    {
        return NULL;
    }

    /* Allocate instance data structure */
    pBitstreamParser = SSC_MALLOC(sizeof(SSC_BITSTREAMPARSER), "SSC_BITSTREAMPARSER Instance");

    if(pBitstreamParser == NULL)
    {
        return NULL;
    }

    /* Initialise instance data */
    memset(pBitstreamParser, 0, sizeof(SSC_BITSTREAMPARSER));

    pBitstreamParser->MaxNrOfChannels = MaxNrOfChannels;
    pBitstreamParser->MaxFrameLength  = 8192;

    Error = initPsDec(&(pBitstreamParser->h_PS_DEC), nNrSlots);
    if (Error)
    {
        SSC_FREE(pBitstreamParser, "SSC_BITSTREAMPARSER Instance");
        pBitstreamParser = NULL;
        return pBitstreamParser;
    }

    /* Allocate parameter buffer */
    pBitStreamData = SSC_MALLOC(sizeof(SSC_BITSTREAM_DATA), "SSC_BITSTREAM_DATA");

    if(pBitStreamData != NULL)
    {
        /* Set the instance to its initial state */
        pBitstreamParser->pBitStreamData = pBitStreamData;
        SSC_BITSTREAMPARSER_Reset(pBitstreamParser);
    }
    else
    {
        /* Clean up */
        DeletePsDec(&(pBitstreamParser->h_PS_DEC));
        SSC_FREE(pBitstreamParser, "SSC_BITSTREAMPARSER Instance");
        pBitstreamParser = NULL;
    }

    return pBitstreamParser;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_DestroyInstance
 *
 * Description : Destroys an instance of the bit-stream parser.
 *
 * Parameters  : pBitstreamParser - Pointer to instance to destroy.
 *
 * Returns     : -
 *
 *****************************************************************************/
void SSC_BITSTREAMPARSER_DestroyInstance(SSC_BITSTREAMPARSER* pBitstreamParser)
{
    SSC_BITSTREAM_DATA* pBitStreamData;

    if(pBitstreamParser == NULL)
    {
        return;
    }

    /* Free parameter buffer(s) */
    pBitStreamData = pBitstreamParser->pBitStreamData;

    if(pBitStreamData != NULL)
    {
        SSC_FREE(pBitStreamData, "BitStreamData");
    }

    DeletePsDec(&(pBitstreamParser->h_PS_DEC));

    /* Free instance data structure */
    memset(pBitstreamParser, 0, sizeof(SSC_BITSTREAMPARSER));
    SSC_FREE(pBitstreamParser, "SSC_BITSTREAMPARSER Instance");
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_Reset
 *
 * Description : Reset the bit-stream parser to its initial state. After
 *               return the bit-stream parser is in the same state as if it had
 *               just been created.
 *
 * Parameters  : pBitstreamParser - Pointer to instance to reset.
 *
 * Returns     : -
 *
 *****************************************************************************/
void SSC_BITSTREAMPARSER_Reset(SSC_BITSTREAMPARSER* pBitstreamParser)
{
    /* Check input arguments in debug builds */
    assert(pBitstreamParser);

    STATISTICS_RESET(&(pBitstreamParser->Statistics));

    pBitstreamParser->State = Reset;

    ResetPsDec( pBitstreamParser->h_PS_DEC );

    /* Clear parameter set. */
    memset(pBitstreamParser->pBitStreamData,
           0,
           sizeof(SSC_BITSTREAM_DATA) );

}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_SetMaxFrameLength
 *
 * Description : Specify the maximum frame length. Use this function to specify
 *               the size of the stream buffer.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               MaxFrameLength   - The maximum frame length in bytes.
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
void SSC_BITSTREAMPARSER_SetMaxFrameLength(SSC_BITSTREAMPARSER* pBitstreamParser,
                                           UInt                 MaxFrameLength)
{
    /* Check input arguments in debug builds */
    assert(pBitstreamParser);

    /* Set frame length limit */
    pBitstreamParser->MaxFrameLength = MaxFrameLength;
}




/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_ParseFileHeader
 *
 * Description : Parse the file header from the stream.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               pStream - pointer to stream
 *
 *               BufLength - length of pStream in bytes
 *
 *               pFileHeaderLength - length of parsed file header in bytes
 *
 *               nVariableSfSize - length of a subframe
 *
 * Returns     : Error code.
 *
 *****************************************************************************/

SSC_ERROR SSC_BITSTREAMPARSER_ParseFileHeader(SSC_BITSTREAMPARSER* pBitstreamParser,
                                              const UByte*         pStream,
                                              const UInt           BufLength,
                                              UInt*                pFileHeaderLength,
                                              UInt                 nVariableSfSize)
{
    SSC_ERROR                 ErrorCode;      /* Error code                         */
    SSC_BITSTREAM_HEADER*     pHeader;        /* Ptr to header information          */
    SSC_BITSTREAM_ERROR*      pError;         /* Ptr to error information           */
    SSC_BITS                  Bits;           /* Structure for reading bit-fields   */
    SSC_BITSTREAM_STATISTICS* pStat;          /* Ptr to statistic information      */
    SSC_BITSTREAM_DATA*       pBitStreamData; /* Ptr to the current bitstream data  */
    UInt                      ch=0 ;
    SInt                      sf=0 ;

    struct exception_context  tec = SSC_INIT_EXCEPTION ;
    struct exception_context* the_exception_context = &tec;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    assert(pStream);
    assert(pFileHeaderLength);

    /* Prepare for file header parsing. */
    SSC_BITS_Init(&Bits,
                  pStream,
                  BufLength * 8,
                  the_exception_context);

    pHeader           = &(pBitstreamParser->Header);
    pError            = &(pBitstreamParser->Error);
    pStat             = &(pBitstreamParser->Statistics);
    pBitStreamData    = pBitstreamParser->pBitStreamData;
    pError->ErrorCode = SSC_ERROR_OK;
    STATISTICS_RESET(pStat);

    Try
    {
        /* Read file header. */
        STATISTICS_START(&Bits, pStat, Overhead);

        ErrorCode = LOCAL_ParseFileHeader(&Bits,
                                          pBitstreamParser,
                                          the_exception_context,
                                          nVariableSfSize);

        if( 
            (pHeader->nrof_channels_bs  > pBitstreamParser->MaxNrOfChannels) ||
            (pHeader->nrof_channels_out > pBitstreamParser->MaxNrOfChannels)
          )
        {
            Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_NROF_CHANNELS;
        }

        UPDATE_ERROR(pError, &Bits, ErrorCode);
        STATISTICS_END(&Bits, pStat, Overhead);

        LOCAL_ReadPaddingBits( &Bits, the_exception_context );

    } /* Try */
    Catch(ErrorCode)
    {
        pBitstreamParser->Error.ErrorCode = ErrorCode;
        pBitstreamParser->Error.Position  = SSC_BITS_GetOffset(&Bits);
        pBitstreamParser->Error.Channel   = ch;
        pBitstreamParser->Error.SubFrame  = sf;
        LOG_Printf("SSC_BITSTREAMPARSER_ParseFileHeader - Error 0x%x at bit %d\n",
                   pBitstreamParser->Error.ErrorCode,
                   pBitstreamParser->Error.Position);
    } /* Catch */

    *pFileHeaderLength = (SSC_BITS_GetOffset(&Bits) + 7) >> 3;

    return pError->ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_ParseSoundHeader
 *
 * Description : Parse the sound header from the stream.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               pStream - pointer to stream
 *
 *               BufLength - length of pStream in bytes
 *
 *               pSoundHeaderLength - length of parsed sound header in bytes
 *
 * Returns     : Error code.
 *
 *****************************************************************************/

SSC_ERROR SSC_BITSTREAMPARSER_ParseSoundHeader(SSC_BITSTREAMPARSER* pBitstreamParser,
                                              const UByte*          pStream,
                                              const UInt            BufLength,
                                              UInt*                 pSoundHeaderLength)
{
    SSC_ERROR                 ErrorCode;      /* Error code                         */
    SSC_BITSTREAM_HEADER*     pHeader;        /* Ptr to header information          */
    SSC_BITSTREAM_ERROR*      pError;         /* Ptr to error information           */
    SSC_BITS                  Bits;           /* Structure for reading bit-fields   */
    SSC_BITSTREAM_STATISTICS* pStat;          /* Ptr to statistic information      */
    SSC_BITSTREAM_DATA*       pBitStreamData; /* Ptr to the current bitstream data  */
    UInt                      ch=0 ;
    SInt                      sf=0 ;

    struct exception_context  tec = SSC_INIT_EXCEPTION ;
    struct exception_context* the_exception_context = &tec;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    assert(pStream);
    assert(pSoundHeaderLength);

    /* Prepare for sound header parsing. */
    SSC_BITS_Init(&Bits,
                  pStream,
                  BufLength * 8,
                  the_exception_context);

    pHeader           = &(pBitstreamParser->Header);
    pError            = &(pBitstreamParser->Error);
    pStat             = &(pBitstreamParser->Statistics);
    pBitStreamData    = pBitstreamParser->pBitStreamData;
    pError->ErrorCode = SSC_ERROR_OK;
    STATISTICS_RESET(pStat);

    Try
    {
        /* Read sound header. */
        STATISTICS_START(&Bits, pStat, Overhead);

        ErrorCode = LOCAL_ParseSoundHeader(&Bits,
                                           pBitstreamParser,
                                           the_exception_context);

        UPDATE_ERROR(pError, &Bits, ErrorCode);
        STATISTICS_END(&Bits, pStat, Overhead);

    } /* Try */
    Catch(ErrorCode)
    {
        pBitstreamParser->Error.ErrorCode = ErrorCode;
        pBitstreamParser->Error.Position  = SSC_BITS_GetOffset(&Bits);
        pBitstreamParser->Error.Channel   = ch;
        pBitstreamParser->Error.SubFrame  = sf;
        LOG_Printf("SSC_BITSTREAMPARSER_ParseSoundHeader - Error 0x%x at bit %d\n",
                   pBitstreamParser->Error.ErrorCode,
                   pBitstreamParser->Error.Position);
    } /* Catch */

    *pSoundHeaderLength = (SSC_BITS_GetOffset(&Bits) + 7) >> 3;

    return pError->ErrorCode;
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_ParseFrame
 *
 * Description : Parse one audio frame of elementary stream data.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *               
 *               PitchFactor - scaling factor for pitch
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
SSC_ERROR SSC_BITSTREAMPARSER_ParseFrame(SSC_BITSTREAMPARSER* pBitstreamParser,
                                         const UByte*         pStream,
                                         UInt*                pFrameLength)
{
    SSC_ERROR                 ErrorCode;        /* Error code                         */
    UInt                      ch = 0;           /* Channel and subframe loop counters */
    SInt                      sf = 0;           /* Channel and subframe loop counters */
    SSC_BITSTREAM_HEADER*     pHeader;          /* Ptr to header information          */
    SSC_BITSTREAM_ERROR*      pError;           /* Ptr to error information           */
    SSC_BITSTREAM_STATISTICS* pStat;            /* Ptr to statistic information       */
    SSC_BITSTREAM_DATA*       pBitStreamData;   /* Ptr to the current bitstream data  */
    SSC_SINUSOID_QPARAM*      pParam = NULL;    /* Ptr to sinusoid parameter block    */
    SSC_TRANSIENT_QPARAM*     pQTransient;      /* Ptr to transient parameter block   */
    SSC_BITS                  Bits;             /* Structure for reading bit-fields   */
    SSC_ERROR                 PrevError;        /* Error code of the previous frame   */
    UInt                      NrOfBits;         /* nr of bits for continuations       */

    struct exception_context  tec= SSC_INIT_EXCEPTION ;
    struct exception_context* the_exception_context = &tec;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    assert(pStream);
    assert(pFrameLength);

    #ifdef LOG_GENERAL
    {
        LOG_Printf( "\nParsing frame %d\n\n", pBitstreamParser->FramesParsed);
    }
    #endif
    /* Prepare for frame parsing. */
    SSC_BITS_Init(&Bits,
                  pStream,
                  pBitstreamParser->MaxFrameLength * 8,
                  the_exception_context);

    pHeader           = &(pBitstreamParser->Header);
    pError            = &(pBitstreamParser->Error);
    pStat             = &(pBitstreamParser->Statistics);
    pBitStreamData    = pBitstreamParser->pBitStreamData;
    PrevError         = pError->ErrorCode;
    pError->ErrorCode = SSC_ERROR_OK;
    STATISTICS_RESET(pStat);

    Try
    {
        #ifdef LOG_PARMS
            LOGPARMS_Printf("*%d\n", pBitstreamParser->FramesParsed);
        #endif

        /* Move last parameters of the previous frame to the beginning. */
        if(SSC_ERROR_CATAGORY(PrevError) <= SSC_ERROR_WARNING)
        {
            LOCAL_PrepareParameterSet(pBitStreamData, pHeader->nrof_subframes,
                                      pHeader->nrof_channels_bs);
        }

        /* start processing audio frame header */
        pHeader->refresh_sinusoids            = SSC_BITS_Read(&Bits, SSC_BITS_REFRESH_SINUSOIDS);
        pHeader->refresh_sinusoids_next_frame = SSC_BITS_Read(&Bits, SSC_BITS_REFRESH_SINUSOIDS);
        pHeader->refresh_noise                = SSC_BITS_Read(&Bits, SSC_BITS_REFRESH_NOISE);

        NrOfBits = SSC_DecoderLevel[pHeader->decoder_level].NrOfBits ;
        assert( NrOfBits >= SSC_BITS_S_NROF_CONTINUATIONS_MIN );
        assert( NrOfBits <= SSC_BITS_S_NROF_CONTINUATIONS_MAX );
        assert( pHeader->nrof_channels_bs > 0 );

        for(ch = 0; ch < pHeader->nrof_channels_bs; ch++)
        {
            pParam = LOCAL_GetSinParamBlock( pBitStreamData, 0, ch);
            pParam->s_nrof_continuations = SSC_BITS_Read(&Bits, NrOfBits);
        }

        /* read current number of noise denominators */
        pBitStreamData->n_nrof_den[SSC_NOISE_NUM_CURRENT] =
                                        SSC_BITS_Read(&Bits, SSC_BITS_N_NROF_DEN);
        pBitStreamData->n_nrof_lsf[SSC_NOISE_NUM_CURRENT] =
                                        SSC_BITS_Read(&Bits, SSC_BITS_N_NROF_LSF);

        pHeader->freq_granularity = SSC_BITS_Read(&Bits, SSC_BITS_FREQ_GRANULARITY );
        assert( pHeader->freq_granularity < 4 );

        pParam->s_qgrid_freq = pHeader->freq_granularity;

        pHeader->amp_granularity = SSC_BITS_Read(&Bits, SSC_BITS_AMP_GRANULARITY );
        assert( pHeader->amp_granularity < 4 );

        pParam->s_qgrid_amp  = pHeader->amp_granularity;

        /*-----------------------------------------------------------------*/
        /* Read phase_jitter_present                                       */
        /*-----------------------------------------------------------------*/
        pHeader->phase_jitter_present = SSC_BITS_Read(&Bits,
                                            SSC_BITS_PHASE_JITTER_PRESENT);

        if(pHeader->phase_jitter_present == 1)
        {
            UInt jitter_percentage     = SSC_BITS_Read(&Bits, SSC_BITS_PHASE_JITTER_PERCENTAGE );

            pHeader->phase_max_jitter       = (UInt)(JITTER_SCALE_FACTOR * (1<<pHeader->freq_granularity)/2.0 * (jitter_percentage + 1.0 ) / 4.0) ;
            pHeader->phase_jitter_band      = 800*SSC_BITS_Read(&Bits, SSC_BITS_PHASE_JITTER_BAND);
        }
        else
        {
            /* disable jitter by setting the jitter band above the maximum erb value */
            /* (max erb value around 4000) */
            pHeader->phase_jitter_band       = 100000 ;
            pHeader->phase_max_jitter        = 0 ;
        }

        /*-----------------------------------------------------------------*/
        /* end of audio frame header                                       */
        /*-----------------------------------------------------------------*/


        /*-----------------------------------------------------------------*/
        /* start of audio frame data                                       */
        /*-----------------------------------------------------------------*/
        for( sf = 0; sf < (SInt)pHeader->nrof_subframes ; sf++ )
        {
            for( ch = 0; ch < pHeader->nrof_channels_bs; ch++ )
            {
                #ifdef LOG_GENERAL
                {
                    LOG_Printf( "\nProcessing subframe %d, ch %d\n", sf, ch );
                }
                #endif
                pQTransient = LOCAL_GetTransientParamBlock(pBitStreamData,sf,ch);

                /* transient subframe */
                STATISTICS_START(&Bits, pStat, Transient);
                ErrorCode = LOCAL_ParseSubframeTransient(&Bits,
                                  pHeader->update_rate,
                                  pQTransient,
                                  the_exception_context);

                UPDATE_ERROR(pError, &Bits, ErrorCode);
                STATISTICS_END(&Bits, pStat, Transient);

                /* sinusoid subframe */
                ErrorCode = LOCAL_ParseSubframeSinusoid(&Bits,
                                  pHeader,
                                  pBitStreamData,
                                  LOCAL_GetSinParamBlock(pBitStreamData,sf,ch),
                                  sf,
                                  ch,
                                  pStat,
                                  the_exception_context);
                UPDATE_ERROR(pError, &Bits, ErrorCode);

                /* Parse noise parameters */
                STATISTICS_START(&Bits, pStat, Noise);
                ErrorCode = LOCAL_ParseSubframeNoise(&Bits,
                                  pHeader,
                                  pBitStreamData,
                                  LOCAL_GetNoiseParamBlock(pBitStreamData,sf,ch),
                                  LOCAL_GetNoiseParamBlock(pBitStreamData,sf-2,ch),
                                  LOCAL_GetNoiseParamBlock(pBitStreamData,sf-4,ch),
                                  LOCAL_GetNoiseDenominators(pBitStreamData,SSC_NOISE_NUM_CURRENT),
                                  LOCAL_GetNoiseLsfs        (pBitStreamData,SSC_NOISE_NUM_CURRENT),
                                  sf,
                                  the_exception_context);

                UPDATE_ERROR(pError, &Bits, ErrorCode);
                STATISTICS_END(&Bits, pStat, Noise);
            }

            /* Parse low complexity parametric stereo if applicable */
            if ( 
                (pHeader->mode     == SSC_MODE_LCSTEREO      ) &&
                (pHeader->mode_ext == SSC_MODE_EXT_PAR_STEREO) &&
                ( ((sf+1)%SSC_StereoUpdateRateFactor[SSC_UPDATE_OCS_QMF]) == 0)
               )
            {
                UInt Length;

                STATISTICS_START(&Bits, pStat, Stereo);

                Length = ReadPsData(pBitstreamParser->h_PS_DEC, &Bits);
                DecodePs(pBitstreamParser->h_PS_DEC);
                ErrorCode = SSC_ERROR_OK;

                UPDATE_ERROR(pError, &Bits, ErrorCode);
                
                STATISTICS_END(&Bits, pStat, Stereo);
            }
        }
        LOCAL_ReadPaddingBits(&Bits, the_exception_context);
    } /* Try */
    Catch(ErrorCode)
    {
        pBitstreamParser->Error.ErrorCode = ErrorCode;
        pBitstreamParser->Error.Position  = SSC_BITS_GetOffset(&Bits);
        pBitstreamParser->Error.Channel   = ch;
        pBitstreamParser->Error.SubFrame  = sf;
        LOG_Printf("SSC_BITSTREAMPARSER_ParseFrame - Error 0x%x at bit %d\n",
                   pBitstreamParser->Error.ErrorCode,
                   pBitstreamParser->Error.Position);
    } /* Catch */

    /* Update administration and prepare to return to caller */
    if(SSC_ERROR_CATAGORY(pError->ErrorCode) <= SSC_ERROR_WARNING)
    {
        if(pBitstreamParser->State != Streaming)
        {
            pBitstreamParser->State++;
        }
    }

    pBitstreamParser->FramesParsed++;

    *pFrameLength = (SSC_BITS_GetOffset(&Bits) + 7) >> 3;
    #ifdef LOG_GENERAL
    {
        LOG_Printf( "Frame length %d bytes\n\n", *pFrameLength);
    }
    #endif

    return pError->ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetFrameSize
 *
 * Description : Retrieve the number of samples per frame per channel.
 *
 * Parameters  : pBitstreamParser - Pointer bit-stream parser instance.
 *
 * Returns     : The number of samples per frame per channel.
 *
 *****************************************************************************/
UInt SSC_BITSTREAMPARSER_GetFrameSize(SSC_BITSTREAMPARSER* pBitstreamParser)
{
    /* Check input arguments in debug builds */
    assert(pBitstreamParser);

    /* Return the number of samples in the last parsed frame */
    return pBitstreamParser->Header.frame_size;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetNumberOfTransients
 *
 * Description : Retrieve the number of transients in a channel.
 *
 * Parameters  : pBitstreamParser - Pointer bit-stream parser instance.
 *
 *               Channel          - The channel number.
 *
 * Returns     : Number transient in the specified channel.
 *
 *****************************************************************************/
UInt SSC_BITSTREAMPARSER_GetNumberOfTransients(SSC_BITSTREAMPARSER* pBitstreamParser,
                                               UInt                 Channel)
{
    SInt                  Sf ;
    UInt                  nTransients ;
    SSC_BITSTREAM_DATA*   pBitStreamData ;
    SSC_TRANSIENT_QPARAM* pQTransient ;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    assert(Channel < pBitstreamParser->Header.nrof_channels_bs);

    pBitStreamData = pBitstreamParser->pBitStreamData ;
    nTransients = 0 ;
    /* include the last subframe of the previous frame */
    for( Sf=-1; Sf < (SInt)pBitstreamParser->Header.nrof_subframes ; Sf++ )
    {
        pQTransient = LOCAL_GetTransientParamBlock(pBitStreamData,Sf,Channel);

        if( pQTransient->t_bPresent != 0 )
            nTransients++ ;
    }
    /* Return the number of transients in the channel ch */
    return nTransients;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetTransient
 *
 * Description : Retrieve the parameters of the specified transient.
 *
 * Parameters  : pBitstreamParser - Pointer bit-stream parser instance.
 *
 *               Channel          - The channel in which the transient resides.
 *
 *               Number           - The number of the transient to retrieve.
 *
 *               pTransient       - Pointer to structure receiving the
 *                                  dequantisised transient parameters.
 *
 * Returns     :
 *
 *****************************************************************************/
SSC_ERROR SSC_BITSTREAMPARSER_GetTransient(SSC_BITSTREAMPARSER* pBitstreamParser,
                                           UInt                 Channel,
                                           UInt                 Number,
                                           SInt*                pSubframe,
                                           SSC_TRANSIENT_PARAM* pTransient)
{
    SSC_TRANSIENT_QPARAM* pQTransient ;
    SSC_BITSTREAM_DATA*   pBitStreamData ;
    SSC_BITSTREAM_HEADER* pHeader ;
    UInt                  nTransient = 0 ;
    SInt                  sf ;
    SSC_ERROR             nReturn = SSC_ERROR_NOT_AVAILABLE;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    pHeader = &(pBitstreamParser->Header);
    assert(pHeader);
    assert(Channel < pHeader->nrof_channels_bs);

    *pSubframe = -2 ;
    pQTransient = NULL ;
    pBitStreamData = pBitstreamParser->pBitStreamData ;
    /* include the last subframe of the previous frame */
    for( sf=-1; sf < (SInt)pBitstreamParser->Header.nrof_subframes; sf++ )
    {
        pQTransient = LOCAL_GetTransientParamBlock(pBitStreamData,sf,Channel);

        if( pQTransient->t_bPresent != 0 )
            nTransient++ ;

        if( (Number+1) == nTransient )  /* number based 0 */
        {
            /* found the correct one, copy parameters */
            *pSubframe = sf ;

            pTransient->Position = pQTransient->t_loc;
            pTransient->Type     = pQTransient->t_type;

            switch(pTransient->Type)
            {
                case 0:     /* Step transient - no parameters. */
                {
                    break;
                }

                case 1:     /* Mexner transient - dequantisise parameters. */
                {
                    SSC_TRANSIENT_MEIXNER_PARAM* pMeixner;
                    SSC_MEIXNER_QPARAM*          pQMeixner;
                    UInt                         i;
                    UInt                         b, chi;

                    pMeixner  = &(pTransient->Parameters.Meixner);
                    pQMeixner = &(pQTransient->Parameters.Meixner);

                    b  = pQMeixner->t_b_par;
                    chi = pQMeixner->t_chi_par;

                    pMeixner->b             = b + 2;
                    pMeixner->chi           = SSC_Chi[chi][b];
                    pMeixner->a_max         = SSC_a_max[chi][b];
                    pMeixner->NrOfSinusoids = pQMeixner->t_nrof_sin;

                    for(i = 0; i < pMeixner->NrOfSinusoids; i++)
                    {
                        pMeixner->SineParam[i].Frequency = LOCAL_DequantFreq(pQMeixner->SineParam[i].Frequency,
                                                                             pHeader->sampling_rate);
                        pMeixner->SineParam[i].Amplitude = LOCAL_DequantTransientAmpl(pQMeixner->SineParam[i].Amplitude);
                        pMeixner->SineParam[i].Phase     = LOCAL_DequantPhasePrecise(
                                                                             pQMeixner->SineParam[i].Phase);
                    }
                    break;
                }

                default:    /* Should have catched this situation already. */
                {
                    assert(0);
                    break;
                }
            }
            nReturn = SSC_ERROR_OK;
            break ;
        }
    }
    return nReturn ;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetSinusoids
 *
 * Description : Retrieve the parameters of the sinusoids in the specified
 *               sub-frame of the specified track.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *
 *               Subframe         - Sub-frame of the sinusoids to retrieve.
 *                                  To retrieve the last sinusoid parameters
 *                                  of the previous frame, use -1 for this
 *                                  parameter.
 *
 *               Channel          - Channel of the sinusoids to retrieve.
 *
 *               pSinusoids       - Pointer to buffer receiving the sinusoid
 *                                  parameters. The buffer should be large
 *                                  enough to accommodate SSC_MAX_SIN_TRACKS
 *                                  sinusoids.
 *
 * Returns     : Number of sinusoids retrieved.
 *
 *****************************************************************************/
UInt SSC_BITSTREAMPARSER_GetSinusoids(SSC_BITSTREAMPARSER* pBitstreamParser,
                                      SInt                 Subframe,
                                      UInt                 Channel,
                                      Double               PitchFactor,
                                      SSC_SIN_SEGMENT*     pSinusoids)
{
    UInt                  NrOfSinusoids;

    /* Retrieve the Sinusoidal data */
    NrOfSinusoids = LOCAL_DecodeAdpcm(pBitstreamParser, Subframe, Channel, PitchFactor, pSinusoids);
    
    return NrOfSinusoids;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetNoise
 *
 * Description : Retrieve the noise parameters in the specified sub-frame.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               Subframe         - Sub-frame of the noise data to retrieve.
 *                                  To retrieve the last noise parameters
 *                                  of the previous frame, use -1 for this
 *                                  parameter.
 *
 *               Channel          - Channel of the noise data to retrieve.
 *
 *               pNoiseParam      - Pointer to buffer receiving the noise
 *                                  parameters.
 *
 * Returns     : Error code
 *
 *****************************************************************************/
SSC_ERROR SSC_BITSTREAMPARSER_GetNoise(SSC_BITSTREAMPARSER*     pBitstreamParser,
                                       SInt                     Subframe,
                                       UInt                     Channel,
                                       SSC_NOISE_PARAM*         pNoiseParam)
{
    UInt                i;
    SSC_BITSTREAM_DATA* pBitStreamData ;
    SSC_NOISE_QPARAM*   pQNoise;
    SSC_ERROR           ErrorCode;
    UInt*               pDenominators ;
    UInt*               pLsfs ;
    SSC_NOISE_NUM       noiseFrame ;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);
    assert(Channel  <  pBitstreamParser->Header.nrof_channels_bs);
    assert(Subframe >= -1);
    assert(Subframe <  (SInt)pBitstreamParser->Header.nrof_subframes);

    /* Check availability of parameters */
    ErrorCode = pBitstreamParser->Error.ErrorCode;

    pBitStreamData = pBitstreamParser->pBitStreamData;
    if( (pBitStreamData->bNoiseValid == False) || (pBitstreamParser->State == Reset) ||
        ((pBitstreamParser->State == FirstFrame) && (Subframe < 0)) )
    {
        ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_NOT_AVAILABLE;
    }
    /* Dequantise numerators */
    pQNoise = LOCAL_GetNoiseParamBlock( pBitStreamData, Subframe, Channel);

    if( Subframe < 0 )
    {
        noiseFrame = SSC_NOISE_NUM_PREV ;
    }
    else
    {
        noiseFrame = SSC_NOISE_NUM_CURRENT ;
    }

    pDenominators = LOCAL_GetNoiseDenominators( pBitStreamData, noiseFrame);
    pLsfs         = LOCAL_GetNoiseLsfs        ( pBitStreamData, noiseFrame);

    /* Dequantise denominators */
    pNoiseParam->DenOrder = *pDenominators;

    for(i = 0; i < *pDenominators; i++)
    {
        pNoiseParam->Denominator[i] = LOCAL_DequantLAR(pQNoise->n_lar_den[i]);
    }

    /* Dequantise lsf */
    pNoiseParam->LsfOrder = *pLsfs;
    for(i = 0; i < *pLsfs; i++)
    {
        pNoiseParam->Lsf[i] = LOCAL_DequantLsf(pQNoise->n_lsf[i]);
    }

    /* dequantize the gain */
    if(pQNoise->n_env_gain == 0)
    {
        pNoiseParam->EnvGain = 0;
    }
    else
    {
        /* Dequantise gains */
        pNoiseParam->EnvGain = pow(10.0, ((Double)pQNoise->n_env_gain-21) / 20.0);
    }

    /* dequantise the pole */
    if(pQNoise->n_pole_laguerre == 0)
    {
        pNoiseParam->PoleLaguerre = 0;
    }
    else if(pQNoise->n_pole_laguerre == 1)
    {
        pNoiseParam->PoleLaguerre = 0.5;
    }
    else if(pQNoise->n_pole_laguerre == 2)
    {
        pNoiseParam->PoleLaguerre = 0.7;
    }

    /* 'dequantise' the grid */
    if(pQNoise->n_grid_laguerre == 0)
    {
        pNoiseParam->GridLaguerre = 0;
    }
    else
    {
        pNoiseParam->GridLaguerre = 1;
    }

    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetHeader
 *
 * Description : Get the header information of the last parsed frame.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               pHeader          - Pointer to buffer receiving the header
 *                                  information.
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
SSC_ERROR SSC_BITSTREAMPARSER_GetHeader(SSC_BITSTREAMPARSER*  pBitstreamParser,
                                        SSC_BITSTREAM_HEADER* pHeader)
{
    SSC_ERROR ErrorCode;

    /* Check input arguments in debug builds */
    assert(pBitstreamParser);

    /* Copy header info */
    *pHeader  = pBitstreamParser->Header;
    ErrorCode = pBitstreamParser->Error.ErrorCode;

    /* Check if header information is available */
    if(pBitstreamParser->State == Reset)
    {
        ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_NOT_AVAILABLE;
    }

    return ErrorCode;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : SSC_BITSTREAMPARSER_GetStatistics
 *
 * Description : Get the statistics of the last parsed frame.
 *
 * Parameters  : pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               pStatistics      - Pointer to buffer receiving the staticstic
 *                                  information.
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
SSC_ERROR SSC_BITSTREAMPARSER_GetStatistics(SSC_BITSTREAMPARSER*      pBitstreamParser,
                                            SSC_BITSTREAM_STATISTICS* pStatistics)
{
    SSC_ERROR ErrorCode = SSC_ERROR_OK;

    /* Copy statistics */
    *pStatistics = pBitstreamParser->Statistics;

    /* Check if statistics are available */
    if(pBitstreamParser->State == Reset)
    {
        memset(pStatistics, 0, sizeof(SSC_BITSTREAM_STATISTICS));
        ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_NOT_AVAILABLE;
    }

    return ErrorCode;
}




/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseFileHeader
 *
 * Description : Parse file header from bit-stream.
 *
 * Parameters  : pBits            - Pointer to structure maintaining the
 *                                  current bit-offset of the bit-field
 *                                  reader (update).
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance.
 *
 *               nVariableSfSize  - Length of a subframe
 *
 * Returns:    : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseFileHeader(SSC_BITS*             pBits,
                                       SSC_BITSTREAMPARSER*  pBitstreamParser,
                                       SSC_EXCEPTION*        the_exception_context,
                                       UInt                  nVariableSfSize)
{
    int                    s;
    SSC_ERROR              ErrorCode;
    SSC_BITSTREAM_HEADER*  pHeader;   /* Pointer to header information. */

    ErrorCode = SSC_ERROR_OK;
    pHeader   = &(pBitstreamParser->Header);

    #ifdef LOG_GENERAL
    {
        LOG_Printf( "\nParsing file header\n\n" );
    }
    #endif
    /* fill in the current decoder version */
    pHeader->decoder_version = DECODER_VERSION ;

    /* check FORM chunk */
    LOCAL_ReadChunkID(pBits, pHeader->formChunk);
    if( LOCAL_CheckChunkID( SSC_formChunk, pHeader->formChunk ))
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }
    pHeader->formChunkLen = SSC_BITS_Read(pBits, SSC_BITS_CHUNK_LEN);
    if( pHeader->formChunkLen == 0 )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }

    /* check SSC chunk */
    LOCAL_ReadChunkID(pBits, pHeader->typeChunk);
    if( LOCAL_CheckChunkID( SSC_typeChunk, pHeader->typeChunk ))
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }

    /* check VER chunk */
    LOCAL_ReadChunkID(pBits, pHeader->verChunk);
    if( LOCAL_CheckChunkID( SSC_verChunk, pHeader->verChunk ))
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }
    pHeader->verChunkLen = SSC_BITS_Read(pBits, SSC_BITS_CHUNK_LEN);
    if( pHeader->verChunkLen != 4 )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }

    /* read file version */
    pHeader->file_version = SSC_BITS_Read(pBits, SSC_BITS_FILE_VERSION);

    if( pHeader->file_version < SSC_FILE_VERSION_MIN )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_OLD_FILE ;
    }
    if( pHeader->file_version > SSC_FILE_VERSION_MAX )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_OLD_DECODER ;
    }

    /* read encoder version */
    pHeader->encoder_version = SSC_BITS_Read(pBits, SSC_BITS_ENCODER_VERSION);

    /* read formatter version */
    pHeader->formatter_version = SSC_BITS_Read(pBits, SSC_BITS_FORMATTER_VERSION);

    /* read reserved version bits */
    pHeader->reserved_version = SSC_BITS_Read(pBits, SSC_BITS_RESERVED_VERSION);
    assert( pHeader->reserved_version == 0 );
    /* end of VER chunk */


    /* check CONF chunk */
    LOCAL_ReadChunkID(pBits, pHeader->confChunk);
    if( LOCAL_CheckChunkID( SSC_confChunk, pHeader->confChunk ))
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }
    pHeader->confChunkLen = SSC_BITS_Read(pBits, SSC_BITS_CHUNK_LEN);
    if( pHeader->confChunkLen != 3 )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }

    /*-----------------------------------------------------------------*/
    /* Read decoder_level                                              */
    /*-----------------------------------------------------------------*/
    pHeader->decoder_level = SSC_BITS_Read(pBits, SSC_BITS_DECODER_LEVEL);

    if(pHeader->decoder_level != SSC_MAX_DECODER_LEVEL_MEDIUM)
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_DECODER_LEVEL;
    }

    /*-----------------------------------------------------------------*/
    /* Read sampling_rate & update_rate                                */
    /*-----------------------------------------------------------------*/
    s                       = SSC_BITS_Read(pBits, SSC_BITS_SAMPLING_RATE);
    pHeader->sampling_rate  = SSC_sample_freq[s];
    pHeader->nrof_subframes = SSC_MAX_SUBFRAMES;

    if(pHeader->sampling_rate == 0)
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_SAMPLING_RATE;
    }

    s = SSC_BITS_Read(pBits, SSC_BITS_UPDATE_RATE);
    if( (s < 0) ||  (s >= (int)(sizeof(SSC_update_rate)/sizeof(SSC_update_rate[0]))) )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_UPDATE_RATE;
    }
    pHeader->update_rate    = nVariableSfSize;

    if( pHeader->update_rate == 0 )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_UPDATE_RATE;
    }

    /*-----------------------------------------------------------------*/
    /* Read synthesis_method                                           */
    /*-----------------------------------------------------------------*/
    pHeader->synthesis_method = SSC_BITS_Read(pBits, SSC_BITS_SYNTHESIS_METHOD);

    if(pHeader->synthesis_method != 0)
    {
        Throw SSC_ERROR_RESERVED | SSC_ERROR_SYNTHESIS_METHOD;
    }

    /*-----------------------------------------------------------------*/
    /* Read mode & mode_ext                                            */
    /*-----------------------------------------------------------------*/
    pHeader->mode = SSC_BITS_Read(pBits, SSC_BITS_MODE);

    switch(pHeader->mode)
    {
        case SSC_MODE_MONO:
            {
                pHeader->mode_ext         = 0;
                pHeader->nrof_channels_out= 1;
                pHeader->nrof_channels_bs = 1;
                break;
            }
        case SSC_MODE_STEREO:
            {
                pHeader->nrof_channels_out = 2;
                pHeader->mode_ext      = SSC_BITS_Read(pBits, SSC_BITS_MODE_EXT_STEREO);
                switch (pHeader->mode_ext)
                {
                    case SSC_MODE_EXT_DUAL_MONO:
                        /* do nothing */
                        pHeader->nrof_channels_bs = 2;
                        break;
                    default:
                        Throw SSC_ERROR_RESERVED | SSC_ERROR_MODE_EXT;
                        break;
                }
                break;
            }
        case SSC_MODE_LCSTEREO:
            {
                pHeader->nrof_channels_out = 2;
                pHeader->mode_ext          = SSC_BITS_Read(pBits, SSC_BITS_MODE_EXT_STEREO);
                switch (pHeader->mode_ext)
                {
                    case SSC_MODE_EXT_DUAL_MONO:
                        /* do nothing */
                        pHeader->nrof_channels_bs = 2;
                        break;
                    case SSC_MODE_EXT_PAR_STEREO:
                        pHeader->nrof_channels_bs = 1;
                        /*-----------------------------------------------------------------*/
                        /* Read reserved bits                                              */
                        /*-----------------------------------------------------------------*/
                        pHeader->ps_reserved = SSC_BITS_Read(pBits, SSC_BITS_PS_RESERVED );
                        break;
                    default:
                        Throw SSC_ERROR_RESERVED | SSC_ERROR_MODE_EXT;
                        break;
                }
                break;
            }
        default:
            {
                pHeader->mode_ext          = 0;
                pHeader->nrof_channels_out = 0;
                pHeader->nrof_channels_bs  = 0;
                Throw SSC_ERROR_RESERVED | SSC_ERROR_MODE;
            }
    }

    pHeader->reserved = SSC_BITS_Read( pBits, SSC_BITS_RESERVED );
    if(pHeader->reserved != 0)
    {
        Throw SSC_ERROR_RESERVED | SSC_ERROR_RESERVED_FIELD;
    }

    /*-----------------------------------------------------------------*/
    /* Interpret header data                                           */
    /*-----------------------------------------------------------------*/
    pHeader->frame_size = pHeader->update_rate * pHeader->nrof_subframes;

    if( 
        (pHeader->nrof_channels_out > pBitstreamParser->MaxNrOfChannels) ||
        (pHeader->nrof_channels_bs  > pBitstreamParser->MaxNrOfChannels)
      )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_NROF_CHANNELS;
    }

    return ErrorCode;
}


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseSoundHeader
 *
 * Description : Parse sound header from bit-stream.
 *
 * Parameters  : pBits            - Pointer to structure maintaining the
 *                                  current bit-offset of the bit-field
 *                                  reader (update).
 *
 *               pBitstreamParser - Pointer to bit-stream parser instance.
 *
 * Returns:    : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseSoundHeader(SSC_BITS*             pBits,
                                        SSC_BITSTREAMPARSER*  pBitstreamParser,
                                        SSC_EXCEPTION*        the_exception_context)
{
    SSC_ERROR              ErrorCode;
    SSC_BITSTREAM_HEADER*  pHeader;   /* Pointer to header information. */

    ErrorCode = SSC_ERROR_OK;
    pHeader   = &(pBitstreamParser->Header);

    #ifdef LOG_GENERAL
    {
        LOG_Printf( "\nParsing sound header\n\n" );
    }
    #endif

    /* check SCHK chunk */
    LOCAL_ReadChunkID(pBits, pHeader->soundChunk);
    if( LOCAL_CheckChunkID( SSC_soundChunk, pHeader->soundChunk ))
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }
    pHeader->soundChunkLen = SSC_BITS_Read(pBits, SSC_BITS_CHUNK_LEN);
    if( pHeader->soundChunkLen < 4 )
    {
        Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_INVALID_FILE ;
    }

    /* check SSC chunk */
    pHeader->nrof_frames = SSC_BITS_Read(pBits, SSC_BITS_NROF_FRAMES);

    return ErrorCode;
}



/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseSubframeTransient
 *
 * Description : Parse transient parameters.
 *
 * Parameters  : pBits           - Pointer to structure maintaining the current
 *                                 bit-offset of the bit-field reader (update).
 *
 *               SubframeSize    - Number of samples in the subframe.
 *
 *               pTransient      - pointer to transientinfo
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseSubframeTransient(SSC_BITS*             pBits,
                                              UInt                  SubframeSize,
                                              SSC_TRANSIENT_QPARAM* pTransient,
                                              SSC_EXCEPTION*        the_exception_context)
{
    UInt                  t_loc_size;
    SSC_ERROR             ErrorCode;

    ErrorCode = SSC_ERROR_OK;

    /* Read the transient present bit */
    pTransient->t_bPresent = SSC_BITS_Read(pBits, SSC_BITS_T_TRANSIENT_PRESENT);
    if( pTransient->t_bPresent == 1 )
    {
        t_loc_size = ceil_log2(SSC_MAX_SUBFRAME_SAMPLES);
        assert( t_loc_size >= SSC_BITS_T_LOC_MIN );
        assert( t_loc_size <= SSC_BITS_T_LOC_MAX );

        /* Read transient position */
        pTransient->t_loc = SSC_BITS_Read(pBits, t_loc_size);
        pTransient->t_loc = pTransient->t_loc * SubframeSize / SSC_MAX_SUBFRAME_SAMPLES;

        if(pTransient->t_loc >= SubframeSize)
        {
            Throw SSC_ERROR_VIOLATION | SSC_ERROR_T_LOC;
        }

        /* Read transient type and associated parameters (if any). */
        pTransient->t_type = SSC_BITS_Read(pBits, SSC_BITS_T_TYPE);

        switch(pTransient->t_type)
        {
            case 0:  /* Step transient (no parameters) */
                     break;

            case 1:  /* Meixner transient */
                     ErrorCode = LOCAL_ParseMeixnerParam(pBits,
                                                        &(pTransient->Parameters.Meixner),
                                                         the_exception_context);
                     break;

            default: /* Reserved/unrecognised type */
                     Throw SSC_ERROR_RESERVED | SSC_ERROR_T_TYPE;
                     break;
        }
        #ifdef LOG_TRANSIENT
        {
            LOG_Printf("Transient - pos %d, type %d\n", pTransient->t_loc, pTransient->t_type);
            if( pTransient->t_type == 1 )
            {
                UInt nParm ;
                SSC_MEIXNER_QPARAM *pMeixner = &pTransient->Parameters.Meixner ;

                LOG_Printf("          - t_b_par, %d, t_chi_par %d, t_nrof_sin %d\n",
                            pMeixner->t_b_par, pMeixner->t_chi_par, pMeixner->t_nrof_sin );

                for( nParm = 0; nParm < pMeixner->t_nrof_sin; nParm++ )
                    LOG_Printf("          - sin %d: freq %d, ampl %d, phi %lf\n", nParm,
                                pMeixner->SineParam[nParm].Frequency,
                                pMeixner->SineParam[nParm].Amplitude,
                                LOCAL_DequantPhasePrecise(pMeixner->SineParam[nParm].Phase));
            }
        }
        #endif
    } /* transient present */

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseMeixnerParam
 *
 * Description : Parser meixner parameters.
 *
 * Parameters  : pBits      - Pointer to bit parser structure.
 *
 *               pMeixner   - Pointer to buffer receiving the quantisised meixner
 *                            parameters.
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseMeixnerParam(SSC_BITS*             pBits,
                                         SSC_MEIXNER_QPARAM*   pQMeixner,
                                         SSC_EXCEPTION*        the_exception_context)
{
    UInt      i;
    SSC_ERROR ErrorCode;

    ErrorCode = SSC_ERROR_OK;

    /* Read b parameter (t_b_par) */
    pQMeixner->t_b_par  = SSC_BITS_Read(pBits, SSC_BITS_T_B_PAR);
    if(pQMeixner->t_b_par > 3)
    {
        Throw SSC_ERROR_RESERVED | SSC_ERROR_T_B_PAR;
    }

    /* Read xi parameter (t_chi_par)*/
    pQMeixner->t_chi_par = SSC_BITS_Read(pBits, SSC_BITS_T_CHI_PAR);
    if(pQMeixner->t_chi_par > 3)
    {
        Throw SSC_ERROR_RESERVED | SSC_ERROR_T_CHI_PAR;
    }

    /* Read number of sinusoids */
    pQMeixner->t_nrof_sin = SSC_BITS_Read(pBits, SSC_BITS_T_NROF_SIN) + 1;

    /* Read sinusoid parameters */
    for(i = 0; i < pQMeixner->t_nrof_sin; i++)
    {
        pQMeixner->SineParam[i].Frequency =
                  ABS_FREQ_FINE_GRID_FACTOR  * SSC_BITS_Read(pBits, SSC_BITS_T_FREQ);
        pQMeixner->SineParam[i].Amplitude =    SSC_BITS_Read(pBits, SSC_BITS_T_AMP);
        pQMeixner->SineParam[i].Phase     = LOCAL_RequantPhase(
                  ABS_PHASE_FINE_GRID_FACTOR * SSC_BITS_ReadSigned(pBits, SSC_BITS_T_PHI) );
    }

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_SubframeSinusoid
 *
 * Description : Parse parameters of a sinusoid parameter block.
 *
 * Parameters  : pBits          - Pointer to structure maintaining the current
 *                                bit-offset of the bit-field reader (update).
 *
 *               pHeader        - Pointer to structure with header information.
 *                                (read-only)
 *
 *               pBitStreamData - Pointer to bitstream data
 *
 *               pSinusoid      - Pointer to sinusoid parameter block. (update)
 *
 *               Sf             - Sub-frame of parameter block.
 *
 *               Ch             - Channel of parameter block
 *
 *               PitchFactor    - Scaling factor for pitch
 *
 * Returns:    : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseSubframeSinusoid(SSC_BITS*                   pBits,
                                             const SSC_BITSTREAM_HEADER* pHeader,
                                             SSC_BITSTREAM_DATA*         pBitStreamData,
                                             SSC_SINUSOID_QPARAM*        pSinusoid,
                                             SInt                        Sf,
                                             UInt                        Ch,
                                             SSC_BITSTREAM_STATISTICS*   pStat,
                                             SSC_EXCEPTION*              the_exception_context)
{
    SSC_ERROR            ErrorCode = SSC_ERROR_OK;
    SSC_SIN_QSEGMENT*    pSinParam;
    UInt                 i;
    UInt                 TotalTracks;
    UInt                 SegmentSize;
    UInt                 p = 0; /* Index of sinusoids in the next sub-frame */
    UInt                 q = 0; /* Index of sinusoids in the next next sub-frame */

    /* Read continuations */
    pSinParam   = pSinusoid->SineParam;
    SegmentSize = pHeader->update_rate * 2;

    #ifdef LOG_SINUSOIDS
    {
        LOG_Printf("Nr of Continuations: %d\n", pSinusoid->s_nrof_continuations );
    }
    #endif
    if((pHeader->refresh_sinusoids == 1) && (Sf == 0)) /* Read absolute coded continuations */
    {
        STATISTICS_START(pBits, pStat, SinAbs);

        LOCAL_ParseSinAbs(pBits,
                          pBitStreamData,
                          pSinParam,
                          pHeader,
                          pSinusoid->s_nrof_continuations,
                          Sf,
                          Ch,
                          &p,
                          &q,
                          True,
                          the_exception_context);

        STATISTICS_END(pBits, pStat, SinAbs);
    }
    else /* Read differentially coded parameters */
    {
        STATISTICS_START(pBits, pStat, SinDiff);

        LOCAL_ParseSinDiff(pBits,
                           pBitStreamData,
                           pSinParam,
                           pHeader,
                           pSinusoid->s_nrof_continuations,
                           Sf,
                           Ch,
                           &p,
                           &q,
                           the_exception_context);

        STATISTICS_END(pBits, pStat, SinDiff);
    }

    /* Read number of births */
    pSinusoid                = LOCAL_GetSinParamBlock(pBitStreamData, Sf, Ch);
    pSinusoid->s_nrof_births = SSC_BITS_ReadHuffman( pBits, &huf_s_nrof_births );

    #ifdef LOG_SINUSOIDS
    {
        LOG_Printf("Nr of births %d\n", pSinusoid->s_nrof_births );
    }
    #endif

    /* Check if the total number of tracks is still within limits */
    TotalTracks    = pSinusoid->s_nrof_continuations + pSinusoid->s_nrof_births ;

    if(TotalTracks > SSC_DecoderLevel[pHeader->decoder_level].MaxNrOfSinusoids)
    {
        ErrorCode = SSC_ERROR_WARNING | SSC_ERROR_TOO_MANY_SIN_TRACKS;

        if(TotalTracks > SSC_S_MAX_SIN_TRACKS)
        {
            Throw SSC_ERROR_NOT_SUPPORTED | SSC_ERROR_TOO_MANY_SIN_TRACKS;
        }
    }

    pSinParam += pSinusoid->s_nrof_continuations ;

    /* Read births - always absolute coded */
    STATISTICS_START(pBits, pStat, SinAbs);

    LOCAL_ParseSinAbs(pBits,
                      pBitStreamData,
                      pSinParam,
                      pHeader,
                      pSinusoid->s_nrof_births,
                      Sf,
                      Ch,
                      &p,
                      &q,
                      False, /* not continuations (so births) */
                      the_exception_context);

    STATISTICS_END(pBits, pStat, SinAbs);
    for(i = 0; i < pSinusoid->s_nrof_births; i++)
    {
        pSinParam[i].State |= SSC_SINSTATE_BIRTH;
    }

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseSinDiff
 *
 * Description : Parse differentially coded sine parameters
 *
 * Parameters  : pBits            - Pointer to structure maintaining the current
 *                                  bit-offset of the bit-field reader (update).
 *
 *               pHeader          - Pointer to structure with header information.
 *                                  (read-only)
 *
 *               pBitStreamData   - Pointer to parameter block. (update)
 *
 *               NrOfSinusoids    - number of sinusoids to process
 *
 *               Sf               - Sub-frame of parameter block.
 *
 *               Ch               - Channel number
 *
 *               p                - Counter for number of sinusoids in next frame.
 *
 *               q                - Counter for number of sinusoids in next next frame.
 *
 *               OrigPhase        - If phase information is included with
 *                                  differentially coded parameters this OrigPhase
 *                                  should be 1 otherwise it should be 0.
 *
 *               PitchFactor      - Scaling factor for pitch
 *
 * Returns:    : -
 *
 *****************************************************************************/
static void LOCAL_ParseSinDiff(SSC_BITS*                   pBits,
                               SSC_BITSTREAM_DATA*         pBitStreamData,
                               SSC_SIN_QSEGMENT*           pSinParam,
                               const SSC_BITSTREAM_HEADER* pHeader,
                               UInt                        NrOfSinusoids,
                               SInt                        Sf,
                               UInt                        Ch,
                               UInt*                       p,
                               UInt*                       q,
                               SSC_EXCEPTION*              the_exception_context)
{
    UInt c;

    for(c = 0; c < NrOfSinusoids; c++)
    {
        SInt           FPadpcm   = 0;
        SInt           sDeltaAmp = 0 ;

        /* Read differential parameters of sinusoid i */
        if( Sf == 0 )
        {
            pSinParam->s_cont = SSC_BITS_ReadHuffman(pBits, &huf_s_cont);
        }
        else
        {
            assert( pSinParam->s_cont > 0 );
            pSinParam->s_cont-- ;
        }

        /* Read ADPCM data */
        if (pSinParam->s_cont > 0)
        {
            /* The sinusoid is continuing in the next sub-frame */
            (*p)++;
        }
        
        if (pSinParam->s_cont > 1)
        {
            /* The sinusoid is continuing in the next next sub-frame */
 			if ((pHeader->refresh_sinusoids_next_frame == False) || 
                (SSC_MAX_SUBFRAMES - Sf > 2))
            {
                FPadpcm    = LOCAL_ReadAdpcm( pBits );
                #ifdef LOG_SINUSOIDS
                    LOG_Printf("FreqPhase bits sf + 2: %d %d\n", FPadpcm, *q );
                #endif
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 2][Ch].SineParam[*q].s_adpcm.s_fr_ph = FPadpcm;
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 2][Ch].SineParam[*q].s_adpcm.bValid  = True;
            }
            (*q)++;
        }
        
        sDeltaAmp  = LOCAL_ReadAmpDelta ( pBits, pHeader->amp_granularity );
        #ifdef LOG_SINUSOIDS
          LOG_Printf("Delta amp:  %d\n", sDeltaAmp );
        #endif
        
        pSinParam->State &= ~SSC_SINMASK_FADJ; /* clear Fadj   */
        
        /* Process differential values */
        if(pSinParam->State & SSC_SINSTATE_VALID)
        {
            pSinParam->Sin.Amplitude = LOCAL_ConvertToGrid(pSinParam->Sin.Amplitude, pHeader->amp_granularity ) + sDeltaAmp;
        }
        
        /* Update next parameter block and end-window of current block */
        if(pSinParam->s_cont > 0)
        {
            pSinParam->State &= ~SSC_SINSTATE_DEATH;  /* Continuation */

            LOCAL_PreDecodeNextUpdate(pSinParam,
                                      Sf,
                                      Ch,
                                      pBitStreamData,
                                      the_exception_context);
        }
        else
        {
            pSinParam->State |= SSC_SINSTATE_DEATH; /* Death */
        }
        #ifdef LOG_SINUSOIDS
        {
            LOG_Printf("Sin Continuation - tracklen %d len %d state %x freq %d ampl %d phase %lf (%d) trackid %d fadj %.1f\n",
                   pSinParam->s_length, pSinParam->s_cont, pSinParam->State & SSC_STATE_MASK, pSinParam->Sin.Frequency,
                   pSinParam->Sin.Amplitude,
                   LOCAL_DequantPhasePrecise(pSinParam->Sin.Phase),pSinParam->Sin.Phase, pSinParam->s_trackID, ((pSinParam->State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR));
        }
        #endif

        pSinParam++;
    } /* for c */
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseSinAbs
 *
 * Description : Parse absolute coded sine parameters
 *
 * Parameters  : pBits           - Pointer to structure maintaining the current
 *                                 bit-offset of the bit-field reader (update).
 *
 *               pBitStreamData  - Pointer to parameter block. (update)
 *
 *               pSinParam       - Pointer to Sinusoid parameter block
 *
 *               pHeader         - Pointer to Header structure
 *
 *               NrOfSinusoids   - nuber of sinusoids to process
 *
 *               Sf              - Sub-frame of parameter block.
 *
 *               Ch              - Channel number
 *
 *               p                - Counter for number of sinusoids in next frame.
 *
 *               q                - Counter for number of sinusoids in next next frame.
 *
 *               Enhancement     - If 1 phase information in bitstream is used.
 *                                 If 0 the phase information in the bitstream
 *                                 is ignored and computed phases will be used
 *                                 instead.
 *
 *               bContinuations  - if 1 sinusoids are continuations
 *                                 if 0 sinusoids are births
 *
 *               PitchFactor     - Scaling factor for pitch
 *
 * Returns:    :
 *
 *****************************************************************************/
static void LOCAL_ParseSinAbs(SSC_BITS*                   pBits,
                              SSC_BITSTREAM_DATA*         pBitStreamData,
                              SSC_SIN_QSEGMENT*           pSinParam,
                              const SSC_BITSTREAM_HEADER* pHeader,
                              UInt                        NrOfSinusoids,
                              SInt                        Sf,
                              UInt                        Ch,
                              UInt*                       p,
                              UInt*                       q,
                              Bool                        bContinuations,
                              SSC_EXCEPTION*              the_exception_context)
{
    UInt   c;
    UInt   Freq, Ampl ;
    SInt   Phase;

    Freq = 0 ;
    Ampl = 0 ;
    for(c = 0; c < NrOfSinusoids; c++)
    {
        SInt           FPadpcm = 0;

        /* Read absolute parameters of sinusoid i */
        /* can be continuation sf==0 and refresh_sinusoids bit set, or */
        /* when a number of sine birth occur */

        pSinParam->s_cont = SSC_BITS_ReadHuffman(pBits, &huf_s_cont);
        if( !bContinuations )
        {
            pSinParam->s_length         = 1 ;
            pSinParam->s_length_refresh = 1 ;
            pSinParam->s_trackID        = pBitStreamData->nTrackID++ ;
        }
        /* Continuations: read always the absolute values */
        /* Births: read the first time the absolute values, the next */
        /* values are the differences of the previous values */
        if( c == 0 || bContinuations )
        {
            /* first birth or continuations: absolute values */
            if( bContinuations )
            {
                Freq = LOCAL_ReadFreqContAbs( pBits, pHeader->freq_granularity );
                Ampl = LOCAL_ReadAmpContAbs ( pBits, pHeader->amp_granularity );
            }
            else
            {
                Freq = LOCAL_ReadFreqSinAbs( pBits, pHeader->freq_granularity );
                Ampl = LOCAL_ReadAmpSinAbs ( pBits, pHeader->amp_granularity );
            }
        }
        else
        {
            /* next births: relative to previous */
            Freq += LOCAL_ReadFreqBirthRel( pBits, pHeader->freq_granularity);
            Ampl += LOCAL_ReadAmpBirthRel( pBits, pHeader->amp_granularity );
        }

        pSinParam->State &= ~SSC_SINMASK_FADJ; /* clear Fadj   */

        Phase = SSC_BITS_ReadSigned(pBits, SSC_BITS_S_PHI);

        /* Use phase information in the bitstream */
        pSinParam->Sin.Frequency = Freq;
        pSinParam->Sin.Amplitude = Ampl;
        pSinParam->Sin.Phase     = LOCAL_RequantPhase(Phase);

        pSinParam->State |= SSC_SINSTATE_VALID;

        /* Read the ADPCM grid info for refresh frame and if it is a continuation */
        if (( bContinuations ) && (pSinParam->s_cont > 0) )
        {
          /* This is a refresh frame, read the adpcm grid data */   
          UInt grid = LOCAL_ReadAdpcmGrid( pBits );
          
          /* Store the grid info */ 
          pSinParam->s_adpcm.s_adpcm_grid = grid ;   
          pSinParam->s_adpcm.bGridValid   = True ;   
         
          pSinParam->s_length_refresh    = 1 ; /* Restart length counter */
        }
        else
        {
          /* No grid information is present; use default grid */
          pSinParam->s_adpcm.bGridValid  = False ;   
        }

        /* Read ADPCM data for the next sub-frame */
        if (pSinParam->s_cont > 0)
        {
            /* The sinusoid is continuing in the next sub-frame */
            /* Read ADPCM data from bit stream*/
            
 			if ((pHeader->refresh_sinusoids_next_frame == False) || 
                (SSC_MAX_SUBFRAMES - Sf > 1))
            {
                FPadpcm = LOCAL_ReadAdpcm( pBits );
                #ifdef LOG_SINUSOIDS
                    LOG_Printf("FreqPhase bits sf + 1: %d %d\n", FPadpcm, *p );
                #endif
                /* Write ADPCM data in next sub-frame */
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 1][Ch].SineParam[*p].s_adpcm.s_fr_ph = FPadpcm;
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 1][Ch].SineParam[*p].s_adpcm.bValid  = True;
            }
            (*p)++;
        }

        /* Read ADPCM data for the next next sub-frame */
        if (pSinParam->s_cont > 1)
        {
            /* The sinusoid is continuing in the next next sub-frame */
            /* Read ADPCM data from bit stream*/
 			if ((pHeader->refresh_sinusoids_next_frame == False) || 
                (SSC_MAX_SUBFRAMES - Sf > 2))
            {
                FPadpcm    = LOCAL_ReadAdpcm( pBits );
                #ifdef LOG_SINUSOIDS
                    LOG_Printf("FreqPhase bits sf + 2: %d %d\n", FPadpcm, *q );
                #endif
                /* Write ADPCM data in next sub-frame */
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 2][Ch].SineParam[*q].s_adpcm.s_fr_ph = FPadpcm;
                pBitStreamData->Sinusoid[HISTORY_SINUSOID + Sf + 2][Ch].SineParam[*q].s_adpcm.bValid  = True;
            }
            (*q)++;
        }

        /* Update next parameter block and end-window of current block */
        if(pSinParam->s_cont > 0) /* Continuation */
        {
            LOCAL_PreDecodeNextUpdate(pSinParam,
                                      Sf,
                                      Ch,
                                      pBitStreamData,
                                      the_exception_context);
        }
        else /* Death */
        {
            pSinParam->State |= SSC_SINSTATE_DEATH;
        }
        #ifdef LOG_SINUSOIDS
        {
            if( bContinuations )
            {
                LOG_Printf("Sin Continuation - tracklen %d len %d state %x freq %d ampl %d phase %lf (%d) trackid %d fadj %.1f\n",
                       pSinParam->s_length, pSinParam->s_cont, pSinParam->State & SSC_STATE_MASK, pSinParam->Sin.Frequency,
                       pSinParam->Sin.Amplitude,
                       LOCAL_DequantPhasePrecise(pSinParam->Sin.Phase), pSinParam->Sin.Phase, pSinParam->s_trackID, ((pSinParam->State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR));
            }
            else
            {
                LOG_Printf("Sin Birth        - len %d state %x freq %d ampl %d phase %lf (%d) trackid %d fadj %.1f\n",
                       pSinParam->s_cont, pSinParam->State & SSC_STATE_MASK, pSinParam->Sin.Frequency,
                       pSinParam->Sin.Amplitude,
                       LOCAL_DequantPhasePrecise(pSinParam->Sin.Phase), pSinParam->Sin.Phase, pSinParam->s_trackID, ((pSinParam->State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR));
            }
        }
        #endif
        pSinParam++;
    }
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ParseSubframeNoise
 *
 * Description : Parse noise parameters.
 *
 * Parameters  : pBits           - Pointer to structure maintaining the current
 *                                 bit-offset of the bit-field reader (update).
 *
 *               pHeader         - Pointer to structure containing header
 *                                 information.(read-only)
 *
 *               pBitStreamData  - Pointer to parameter block. (update)
 *
 *               pNoiseInfo      - Pointer to structure receiving the noise
 *                                 parameters. (write-only)
 *
 *               pPrevNoiseParam - Pointer to structure with noise params of
 *                                 previous subframe
 *
 *               pNumerators     - number of noise numerators
 *
 *               pDenumerators   - number of noise denumerators
 *
 * Returns     : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_ParseSubframeNoise(SSC_BITS*                pBits,
                                       const SSC_BITSTREAM_HEADER* pHeader,
                                       SSC_BITSTREAM_DATA*         pBitStreamData,
                                       SSC_NOISE_QPARAM*           pNoiseParam,
                                       SSC_NOISE_QPARAM*           pPrev2NoiseParam,
                                       SSC_NOISE_QPARAM*           pPrev4NoiseParam,
                                       UInt*                       pDenominators,
                                       UInt*                       pLsfs,
                                       SInt                        Sf,
                                       SSC_EXCEPTION*              the_exception_context)
{
    SSC_ERROR ErrorCode;
    UInt      c;

    ErrorCode = SSC_ERROR_OK;
    the_exception_context = the_exception_context ; /* suppres compiler warning */
    if( (pHeader->refresh_noise == 1) && (Sf == 0) )
    {
        /* read absolute noise parameters */
        
        pNoiseParam->n_pole_laguerre = SSC_BITS_Read(pBits, 2);
        pNoiseParam->n_grid_laguerre = SSC_BITS_Read(pBits, 1);

        /* Parse denominator coeffients */
        for(c = 0; c < *pDenominators; c++)
        {
            pNoiseParam->n_lar_den[c] = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLag);
            assert( pNoiseParam->n_lar_den[c] >= -256 );
            assert( pNoiseParam->n_lar_den[c] <=  256 );

            if (pNoiseParam->n_grid_laguerre == 1)
            {
                pNoiseParam->n_lar_den[c] += SSC_BITS_ReadSigned(pBits, 2);
            }
        }
        for( ; c < SSC_N_MAX_DEN_ORDER; c++ )
            pNoiseParam->n_lar_den[c] = 0 ;

        /* parse gain parameters */
        pNoiseParam->n_env_gain = SSC_BITS_Read(pBits, SSC_BITS_N_ENV_GAIN);
        assert( pNoiseParam->n_env_gain <= 101 );

        pNoiseParam->n_lsf[0] = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLsfE);
        if      ( pNoiseParam->n_lsf[0] == 7 )
        {
            pNoiseParam->n_lsf[0] = SSC_BITS_Read(pBits, 3);
        }
        else if ( pNoiseParam->n_lsf[0] == 25 )
        {
            pNoiseParam->n_lsf[0] = SSC_BITS_Read(pBits, 8);
        }
        assert(pNoiseParam->n_lsf[0] <= 255 );

        /* process the rest */
        for ( c = 1; c < *pLsfs; c++)
        {
            pNoiseParam->n_lsf[c] = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLsfE);
            if      ( pNoiseParam->n_lsf[c] == 7 )
            {
                pNoiseParam->n_lsf[c] = SSC_BITS_Read(pBits, 3);
            }
            else if ( pNoiseParam->n_lsf[c] == 25 )
            {
                pNoiseParam->n_lsf[c] = SSC_BITS_Read(pBits, 8);
            }
            pNoiseParam->n_lsf[c] += pNoiseParam->n_lsf[c-1];

            assert(pNoiseParam->n_lsf[c] <= 255 );
        }
        for( ; c < SSC_N_MAX_LSF_ORDER; c++ )
            pNoiseParam->n_lsf[c] = 0 ;

        /* now the noise values are valid until the next reset (seek) call */
        pBitStreamData->bNoiseValid = True ;

    }
    else
    {
        /* read differentially coded noise parameters */

        SInt nDelta ;
        SInt nLarPrev2 ;

        if ( (Sf % 2) == 0 )
        {
            pNoiseParam->n_pole_laguerre = pPrev2NoiseParam->n_pole_laguerre;
            pNoiseParam->n_grid_laguerre = SSC_BITS_Read(pBits, 1);

            /* Parse denominator coeffients */
            for(c = 0; c < *pDenominators; c++)
            {
                nDelta = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLag);

                if ( pNoiseParam->n_grid_laguerre == 1 )
                {
                    nDelta    += SSC_BITS_ReadSigned( pBits, 2 );
                    nLarPrev2  = pPrev2NoiseParam->n_lar_den[c];
                }                
                else if ( pPrev2NoiseParam->n_grid_laguerre == 1 )
                {
                    /* Convert to 7 bits value (instead of 9 bits) */
                    nLarPrev2  = 4 * (SInt)floor( (Double)pPrev2NoiseParam->n_lar_den[c] / 4.0 + 0.5 );
                }
                else
                {
                    nLarPrev2 = pPrev2NoiseParam->n_lar_den[c];
                }

                if( pBitStreamData->bNoiseValid )
                {
                    pNoiseParam->n_lar_den[c] = nLarPrev2 + nDelta;
                }

                assert( pNoiseParam->n_lar_den[c] >= -256 );
                assert( pNoiseParam->n_lar_den[c] <=  256 );
            }
            for( ; c < SSC_N_MAX_DEN_ORDER; c++ )
            {
                pNoiseParam->n_lar_den[c] = 0 ;
            }
        }

        if ( (Sf % 4) == 0 )
        {
            /* n_delta_gain */
            nDelta = SSC_BITS_ReadHuffman(pBits, &hufNoiRelNdgE);           /* Read out the value */
            if ( nDelta == -13 )
            {
                nDelta = SSC_BITS_ReadSigned(pBits, 8);
            }
            if ( pBitStreamData->bNoiseValid )
            {
                pNoiseParam->n_env_gain = pPrev4NoiseParam->n_env_gain + nDelta ;
            }
            assert( pNoiseParam->n_env_gain <= 101 );

            /* n_overlap_lsf */
            pNoiseParam->n_lsf_overlap = SSC_BITS_Read(pBits, SSC_BITS_N_OVERLAP_LSF);
            assert(pNoiseParam->n_lsf_overlap >= 0 );
            assert(pNoiseParam->n_lsf_overlap <= 1 );

            /* overlap lsf */
            if ( pNoiseParam->n_lsf_overlap == 1 )
            {
                UInt prevOrder = 0;
                /* if first in frame look at previous order */
                if ( Sf == 0 )
                    prevOrder = *LOCAL_GetNoiseLsfs(pBitStreamData,SSC_NOISE_NUM_PREV);
                else
                    prevOrder = *pLsfs;
                
                assert(prevOrder <= SSC_N_MAX_LSF_ORDER );

                for ( c = 0; c < prevOrder; c++ )
                {
                    if ( pPrev4NoiseParam->n_lsf[c] >= 192 )
                    {
                        pNoiseParam->n_lsf[pNoiseParam->n_lsf_overlap - 1] = pPrev4NoiseParam->n_lsf[c] - 192;
                        pNoiseParam->n_lsf_overlap++;
                    }
                }
                pNoiseParam->n_lsf_overlap -= 1;

                c = pNoiseParam->n_lsf_overlap;
            }
            else
            {
                pNoiseParam->n_lsf[0] = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLsfE);
                if      ( pNoiseParam->n_lsf[0] == 7 )
                {
                    pNoiseParam->n_lsf[0] = SSC_BITS_Read(pBits, 3);
                }
                else if ( pNoiseParam->n_lsf[0] == 25 )
                {
                    pNoiseParam->n_lsf[0] = SSC_BITS_Read(pBits, 8);
                }
                assert(pNoiseParam->n_lsf[0] <= 255 );

                c = 1;
            }

            /* process the rest */
            for (; c < *pLsfs; c++)
            {
                pNoiseParam->n_lsf[c] = SSC_BITS_ReadHuffman(pBits, &hufNoiRelLsfE);
                if      ( pNoiseParam->n_lsf[c] == 7 )
                {
                    pNoiseParam->n_lsf[c] = SSC_BITS_Read(pBits, 3);
                }
                else if ( pNoiseParam->n_lsf[c] == 25 )
                {
                    pNoiseParam->n_lsf[c] = SSC_BITS_Read(pBits, 8);
                }
                pNoiseParam->n_lsf[c] += pNoiseParam->n_lsf[c-1];

                assert(pNoiseParam->n_lsf[c] <= 255 );
            }
            for( ; c < SSC_N_MAX_LSF_ORDER; c++ )
                pNoiseParam->n_lsf[c] = 0 ;
        }
    }
    #ifdef LOG_NOISE
    {
        UInt nIndex ;

        LOG_Printf(    "Noise pole laguerre - %d\n", pNoiseParam->n_pole_laguerre );
        LOG_Printf(    "Noise grid laguerre - %d\n", pNoiseParam->n_grid_laguerre );
        for( nIndex = 0; nIndex < *pDenominators; nIndex++ )
        {
            LOG_Printf("Noise den  - [%d] %d\n", nIndex, pNoiseParam->n_lar_den[nIndex] );
        }
        LOG_Printf(    "Noise env gain - %d\n", pNoiseParam->n_env_gain );
    }
    #endif /* LOG_NOISE */

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_PrepareParameterSet
 *
 * Description : Prepares parameter set for next decoding run.
 *
 * Parameters  : pParameterSet - pointer to parameter set to prepare.
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_PrepareParameterSet(SSC_BITSTREAM_DATA* pBitStreamData,
                                      UInt NrOfSubFrames,
                                      UInt NrOfChannels)
{
    UInt                  Sf, nCh;
    SSC_SINUSOID_QPARAM*  pSrc;
    SSC_SINUSOID_QPARAM*  pDest;
    SSC_NOISE_QPARAM*     pSrcNoise ;
    SSC_NOISE_QPARAM*     pDestNoise ;
    SSC_TRANSIENT_QPARAM* pSrcTransient ;
    SSC_TRANSIENT_QPARAM* pDestTransient ;
    SSC_STEREO_QPARAM*     pSrcStereo ;
    SSC_STEREO_QPARAM*     pDestStereo ;

    /* Move last transient params to beginning */
    for ( Sf = 0; Sf < HISTORY_TRANSIENT; Sf++ )
    {
        pSrcTransient  = pBitStreamData->Transient[Sf+NrOfSubFrames] ;
        pDestTransient = pBitStreamData->Transient[Sf] ;
        memmove(pDestTransient, pSrcTransient, sizeof(SSC_TRANSIENT_QPARAM)*NrOfChannels);
    }

    /* Move last 4 stereo params to beginning */
    for ( Sf = 0; Sf < HISTORY_STEREO; Sf++ )
    {
        pSrcStereo  = &pBitStreamData->Stereo[Sf+NrOfSubFrames] ;
        pDestStereo = &pBitStreamData->Stereo[Sf] ;
        memmove(pDestStereo, pSrcStereo, sizeof(SSC_STEREO_QPARAM) );
    }

    /* Move last 4 noise params to beginning */
    for ( Sf = 0; Sf < HISTORY_NOISE; Sf++ )
    {
        pSrcNoise  = pBitStreamData->Noise[Sf+NrOfSubFrames] ;
        pDestNoise = pBitStreamData->Noise[Sf] ;
        memmove(pDestNoise, pSrcNoise, sizeof(SSC_NOISE_QPARAM)*NrOfChannels);
    }

    /* copy current den values to beginning */
    pBitStreamData->n_nrof_den[SSC_NOISE_NUM_PREV] =
                        pBitStreamData->n_nrof_den[SSC_NOISE_NUM_CURRENT] ;
    pBitStreamData->n_nrof_den[SSC_NOISE_NUM_CURRENT] = 0 ;

    /* copy current lsf values to beginning */
    pBitStreamData->n_nrof_lsf[SSC_NOISE_NUM_PREV] =
                        pBitStreamData->n_nrof_lsf[SSC_NOISE_NUM_CURRENT] ;
    pBitStreamData->n_nrof_lsf[SSC_NOISE_NUM_CURRENT] = 0 ;

    /* move sine parameter from last to subframes to the first 2 */
    for( Sf = 0; Sf < (HISTORY_SINUSOID + FUTURE_SINUSOID); Sf++ )
    {
        pSrc  = pBitStreamData->Sinusoid[Sf+NrOfSubFrames] ;
        pDest = pBitStreamData->Sinusoid[Sf] ;
        /* copy sinusoid data of all channels in one time */
        memmove(pDest, pSrc, sizeof(SSC_SINUSOID_QPARAM)*NrOfChannels);
    }

    Sf = HISTORY_SINUSOID ;
    for( nCh = 0; nCh < NrOfChannels; nCh++ )
    {
        /* Invalidate unused entries in sub-frame 0. ( index 1 ) */
        pDest = &pBitStreamData->Sinusoid[Sf][nCh] ;
        /* clear all birth sine parameters to 0 */
        memset( &pDest->SineParam[pDest->s_nrof_continuations], 0,
                sizeof(SSC_SIN_QSEGMENT)*(SSC_S_MAX_SIN_TRACKS - pDest->s_nrof_continuations) );
    }

    for (Sf = HISTORY_SINUSOID + 1; Sf < HISTORY_SINUSOID + FUTURE_SINUSOID; Sf++)
    {
        for( nCh = 0; nCh < NrOfChannels; nCh++ )
        {
            UInt nTr;

            /* Invalidate all entries in sub-frame 1. */
            pDest = &pBitStreamData->Sinusoid[Sf][nCh] ;
            /* clear all sine parameters to 0 except ADPCM part */
            pDest->s_nrof_births = 0;
            pDest->s_nrof_continuations = 0;
            pDest->s_qgrid_amp = 0;
            pDest->s_qgrid_freq = 0;
            for (nTr = 0; nTr < SSC_S_MAX_SIN_TRACKS; nTr++)
            {
                pDest->SineParam[nTr].State = 0;
                pDest->SineParam[nTr].s_cont = 0;
                pDest->SineParam[nTr].s_length = 0;
                pDest->SineParam[nTr].s_trackID = 0;
                pDest->SineParam[nTr].s_length_refresh = 0;
                memset(&(pDest->SineParam[nTr].Sin), 0, sizeof(SSC_SIN_QPARAM));
            }
        }
    }

    /* Clear remaining parameter blocks */
    for(Sf = (HISTORY_SINUSOID + FUTURE_SINUSOID); Sf < (NrOfSubFrames + HISTORY_SINUSOID + FUTURE_SINUSOID); Sf++)
    {
        for( nCh = 0; nCh < NrOfChannels; nCh++ )
        {
            pDest = &pBitStreamData->Sinusoid[Sf][nCh] ;
            /* clear all sineparameters in all subframes to 0 */
            memset( pDest, 0, sizeof(SSC_SINUSOID_QPARAM) );
        }
    }
}


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_PreDecodeNextUpdate
 *
 * Description : Fill the entry where the next update for the track will take
 *               place with pre-decode information.
 *
 * Parameters  : pSinSeg    - Pointer to structure containing the parameters
 *                            of the current sinusoid segment.
 *
 *               Sf         - The current sub-frame.
 *
 *               Ch         - The current channel
 *
 * Returns:    : Error code.
 *
 *****************************************************************************/
static SSC_ERROR LOCAL_PreDecodeNextUpdate(SSC_SIN_QSEGMENT*   pSinSeg,
                                           SInt                Sf,
                                           UInt                Ch,
                                           SSC_BITSTREAM_DATA* pBitStreamData,
                                           SSC_EXCEPTION*      the_exception_context)
{
    SSC_SINUSOID_QPARAM* pNextParam;
    SSC_SIN_QSEGMENT*    pNextSinSeg;
    SSC_ERROR            ErrorCode = SSC_ERROR_OK;

    /* Copy current absolute sine segment data to next update */
    pNextParam = LOCAL_GetSinParamBlock(pBitStreamData, Sf + 1, Ch);
    if(pNextParam->s_nrof_continuations >= SSC_S_MAX_SIN_TRACKS)
    {
        Throw SSC_ERROR_VIOLATION | SSC_ERROR_S_NROF_CONTINUATIONS;
    }

    pNextSinSeg  = pNextParam->SineParam + (pNextParam->s_nrof_continuations);

    /* Copy everything except the ADPCM data into the next sub-frame */
    pNextSinSeg->State     = pSinSeg->State;
    pNextSinSeg->s_cont    = pSinSeg->s_cont;
    pNextSinSeg->s_length  = pSinSeg->s_length;
    pNextSinSeg->s_trackID = pSinSeg->s_trackID;
    pNextSinSeg->Sin       = pSinSeg->Sin;
    pNextSinSeg->s_length_refresh = pSinSeg->s_length_refresh;

    pNextSinSeg->State &= ~SSC_SINSTATE_BIRTH;

    pNextSinSeg->s_length++ ;
    pNextSinSeg->s_length_refresh++ ;
    pNextParam->s_nrof_continuations++;

    return ErrorCode;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_GetSinParamBlock
 *
 * Description : Gets the sinusoid parameter block for the specified
 *               sub-frame
 *
 * Parameters  : pParamSet - Pointer to parameter set containing the sinusoid
 *                           block
 *
 *               Sf        - Subframe number of the sinusoid block, use -1 to
 *                           retrieve the last sinusoid block of the previous
 *                           sub-frame.
 *
 *               Ch        - Channel number of the sinusoid block
 *
 * Returns:    : Pointer to the requested sinusoid parameter block.
 *
 *****************************************************************************/
static SSC_SINUSOID_QPARAM* LOCAL_GetSinParamBlock(SSC_BITSTREAM_DATA* pBitStreamData,
                                                   SInt                Sf,
                                                   UInt                Ch)
{
    assert( Sf >= -HISTORY_SINUSOID);
    assert( Sf <= SSC_MAX_SUBFRAMES );

    return &pBitStreamData->Sinusoid[HISTORY_SINUSOID+Sf][Ch] ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_GetNoiseParamBlock
 *
 * Description : Gets the noise parameter block for the specified
 *               sub-frame
 *
 * Parameters  : pBitStreamData - Pointer to parameter set containing the sinusoid
 *                                block
 *
 *               Ch             - Channel number of the noise block
 *               Sf             - Subframe number of the noise block, use -1 to
 *                                retrieve the last noise block of the previous
 *                                sub-frame.
 *
 * Returns:    : Pointer to the requested noise parameter block.
 *
 *****************************************************************************/
static SSC_NOISE_QPARAM* LOCAL_GetNoiseParamBlock(SSC_BITSTREAM_DATA* pBitStreamData,
                                                  SInt                Sf,
                                                  UInt                Ch)
{
    assert( Sf >= -HISTORY_NOISE);
    assert( Sf <= SSC_MAX_SUBFRAMES );

    return &pBitStreamData->Noise[HISTORY_NOISE+Sf][Ch] ;
}


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_GetTransientParamBlock
 *
 * Description : Gets the transient parameter block for the specified
 *               sub-frame
 *
 * Parameters  : pBitStreamData - Pointer to parameter set containing the sinusoid
 *                                block
 *
 *               Ch             - Channel number of the noise block
 *               Sf             - Subframe number of the noise block, use -1 to
 *                                retrieve the last noise block of the previous
 *                                sub-frame.
 *
 * Returns:    : Pointer to the requested noise parameter block.
 *
 *****************************************************************************/
static SSC_TRANSIENT_QPARAM* LOCAL_GetTransientParamBlock(SSC_BITSTREAM_DATA* pBitStreamData,
                                                          SInt                Sf,
                                                          UInt                Ch)
{
    assert( Sf >= -HISTORY_TRANSIENT);
    assert( Sf <= SSC_MAX_SUBFRAMES );

    return &pBitStreamData->Transient[HISTORY_TRANSIENT+Sf][Ch];
}


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_GetNoiseDenominators
 *
 * Description : Gets a pointer to the number of noise denumerators for the
 *               current or previous frame
 *
 * Parameters  : pBitStreamData - Pointer to parameter set containing the noise
 *                                info
 *
 *               Ch             - The channel number
 *
 * Returns:    : Number of denumerators
 *
 *****************************************************************************/
static UInt* LOCAL_GetNoiseDenominators(SSC_BITSTREAM_DATA* pBitStreamData,
                                       SSC_NOISE_NUM       noiseFrame)
{
    return &pBitStreamData->n_nrof_den[noiseFrame] ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_GetNoiseLsfs
 *
 * Description : Gets a pointer to the number of noise Lsfs for the
 *               current or previous frame
 *
 * Parameters  : pBitStreamData - Pointer to parameter set containing the noise
 *                                info
 *
 *               Ch             - The channel number
 *
 * Returns:    : Number of Lsfs
 *
 *****************************************************************************/
static UInt* LOCAL_GetNoiseLsfs(SSC_BITSTREAM_DATA* pBitStreamData,
                                       SSC_NOISE_NUM       noiseFrame)
{
    return &pBitStreamData->n_nrof_lsf[noiseFrame] ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantFreq
 *
 * Description : De-quantise frequency parameter.
 *
 * Parameters  : Freq  - Quantised frequency
 *
 * Returns:    : De-quantised frequency in rad.
 *
 * Comment     : Dequantisation formula can be replaced with table lookup.
 *
 *****************************************************************************/
static Double LOCAL_DequantFreq(Double dFreq, UInt uSamplingRate)
{
    Double dX, dFreqHz ;

    dX = dFreq / SSC_S_FREQUENCY_BASE ;
    dFreqHz = (pow(10, (dX/21.4)) - 1) / 0.00437 ;        /* frequency in Hz */
    return dFreqHz * (2*SSC_PI)/(double)(uSamplingRate) ; /* convert to rad */
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantAmpl
 *
 * Description : De-quantise amplitude parameter.
 *
 * Parameters  : Ampl  - Quantised amplitude.
 *
 * Returns:    : De-quantised amplitude.
 *
 * Comment     : Dequantisation formula can be replaced with table lookup.
 *
 *****************************************************************************/
static Double LOCAL_DequantAmpl(SInt Ampl)
{
    Double Ab;
    Double Arl;

    Ab  = SSC_S_AMPLITUDE_BASE;
    Arl = (Double)Ampl;

    return pow(Ab, 2.0 * Arl);
}

static Double LOCAL_DequantTransientAmpl( SInt Ampl)
{
    Double Ab;
    Double Arl;

    Ab  = SSC_T_AMPLITUDE_BASE;
    Arl = (Double)Ampl;

    return pow(Ab, 2.0 * Arl);
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantPhase
 *
 * Description : De-quantise phase parameter. (range -16 to 15 = 2^5)
 *
 * Parameters  : Phase  - Quantised phase.
 *
 * Returns     : De-quantised phase.
 *
 *****************************************************************************/
static Double LOCAL_DequantPhase(SInt Phase)
{
    return (Double)Phase * SSC_PI / 16.0;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantPhasePrecise
 *
 * Description : De-quantise phase parameter (range -MAXINT to MAXINT = 2^32).
 *
 * Parameters  : Phase  - Quantised phase.
 *
 * Returns     : De-quantised phase.
 *
 *****************************************************************************/
static Double LOCAL_DequantPhasePrecise(SInt Phase)
{
    return (Double)Phase * SSC_PI / 2147483648.0 ;   /* 2^31 */
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_QuantPhasePrecise
 *
 * Description : Quantise phase parameter (range -MAXINT to MAXINT = 2^32).
 *
 * Parameters  : Phase  - Quantised phase.
 *
 * Returns     : De-quantised phase.
 *
 *****************************************************************************/
static SInt LOCAL_QuantPhasePrecise(Double dPhase)
{
    Double dValue ;
    Double dEps ;

    /* Make Phase between -pi and +pi */
    dPhase = fmod(dPhase, SSC_PI*2);
    if(dPhase > SSC_PI)
    {
        dPhase -= SSC_PI*2;
    }
    if(dPhase < -SSC_PI)
    {
        dPhase += SSC_PI*2;
    }

    dEps = SSC_PI / (1073741824.0 ) ;  /* PI / ((2^31 / 2)) */
    if( dPhase >= SSC_PI - dEps )
        dPhase = -SSC_PI ;

    dValue = (((dPhase*(2147483648.0 - 1.0)) / (double)SSC_PI)+0.5);

    assert( dValue <= INT_MAX );
    assert( dValue >= INT_MIN );

    if( dValue >= INT_MAX )
        dValue = INT_MIN ; /* value was rounded to MAXINT (= PI), return MININT (= -PI) */
    if( dValue < INT_MIN )
        dValue = INT_MIN ;

    return (SInt)dValue ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_RequantPhase
 *
 * Description : Re-quantise phase parameter from -16 to 15 range to
 *               range -MAXINT to MAXINT.
 *
 * Parameters  : Phase  - Quantised phase (from bitstream, range -16 to 15).
 *
 * Returns     : Re-quantised phase ranging from -MAXINT to MAXINT.
 *
 *****************************************************************************/
static SInt LOCAL_RequantPhase(SInt Phase)
{
    Double dPhase = LOCAL_DequantPhase(Phase);
    SInt sPhase = LOCAL_QuantPhasePrecise(dPhase);

    return sPhase ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantLAR
 *
 * Description : De-quantise LAR parameter.
 *
 * Parameters  : LAR  - Quantised LAR coefficient.
 *
 * Returns     : De-quantised LAR coefficient.
 *
 * Comment     : Dequantisation formula can be replaced with table lookup.
 *
 *****************************************************************************/
static Double LOCAL_DequantLAR(SInt LAR)
{
    Double d, temp;

    /* Dequantise LAR coefficients */
    d = (Double)SSC_N_LAR_DYN_RANGE / (Double)SSC_N_LAR_LEVELS;
    temp = exp( d * (Double)LAR );

    return (temp - 1)/(temp + 1);
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_DequantLsf
 *
 * Description : De-quantise Lsf parameter.
 *
 * Parameters  : Lsf  - Quantised Lsf coefficient.
 *
 * Returns     : De-quantised Lsf coefficient.
 *
 * Comment     : Dequantisation formula can be replaced with table lookup.
 *
 *****************************************************************************/
static Double LOCAL_DequantLsf(SInt Lsf)
{
    Double d, rvalue;

    /* Dequantise Lsf coefficients */
    d = (Double)SSC_PI / (Double)SSC_N_LSF_LEVELS;
    rvalue = (Double)Lsf * d + d/2;

    return rvalue;
}


/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_CalcContinuousPhase
 *
 * Description : Compute continuos phase update.
 *
 * Parameters  : OldFreq     - Old frequency.
 *
 *               OldPhase    - Old phase.
 *
 *               NewFreq     - New frequency.
 *
 *               SegmentSize - Size of the synthesis segment.
 *
 *               PitchFactor - Scaling factor for pitch
 *
 * Returns     : 
 *
 *****************************************************************************/
static Double LOCAL_CalcContinuousPhase(Double            OldFreq,
                                        Double            OldPhase,
                                        Double            NewFreq,
                                        UInt              SegmentSize,
                                        Double            PitchFactor)
{
    Double n1, n2;
    Double f1, f2;
    Double L;
    Double dPhase ;

    L  = SegmentSize;
    n1 = L/4 ;
    n2 = L/4 ;
    f1 = OldFreq;
    f2 = NewFreq;
    dPhase = OldPhase;
    dPhase += (n1*f1 + n2*f2) * PitchFactor;

    return dPhase;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadChunkID
 *
 * Description : read a chunk ID (4 chars) from the stream
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               szChunkID - chunk ID buffer which is to be filled. The buffer
 *                           must be large enough to hold at least 4 chars
 *
 * Returns     :
 *
 *****************************************************************************/

static void LOCAL_ReadChunkID( SSC_BITS *pBits, Char *szChunkID )
{
    UInt i;

    for( i=0; i<4; i++ )
        szChunkID[i] = (UByte)SSC_BITS_Read(pBits, 8);
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_CheckChunkID
 *
 * Description : checks if the chunk ID from the stream is the expected one
 *
 * Parameters  : szChunkExp - chunk ID which is expected (zero terminated string)
 *
 *               szChunkRead - chunk ID read from stream
 *
 * Returns     : False - equal
 *               True  - unequal
 *
 *****************************************************************************/

static Bool LOCAL_CheckChunkID( const Char *szChunkExp, const Char *szChunkRead )
{
    if( 0 == strncmp( szChunkExp, szChunkRead, 4 ))
        return False ;
    else
        return True ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadFine
 *
 * Description : Reads the fine frequency/amplitude value depending on the 
 *               granularity value 
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               Granularity - current granularity to use
 *
 * Returns     : fine value - the value to be added to the coarse value
 *
 *****************************************************************************/

static SInt LOCAL_ReadFine( SSC_BITS *pBits, UInt nGranularity )
{
    SInt nFine = 0 ;

    switch( nGranularity )
    {
        case 0:
            nFine = SSC_BITS_ReadSigned(pBits, 3);
            break ;
        case 1:
            nFine = SSC_BITS_ReadSigned(pBits, 2) * 2;
            break ;
        case 2:
            nFine = SSC_BITS_ReadSigned(pBits, 1) * 4;
            break ;
        case 3:
            /* do nothing*/
            break ;
        default:
            assert(0);
            break ;
    }
    return nFine ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadFreqSinAbs
 *
 * Description : Reads the abs sine frequency value with different granularity values
 *               depending on the defined freq granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               FreqGranularity - current granularity to use
 *
 * Returns     : Frequency - the abs sine frequency in the most fine grid
 *
 *****************************************************************************/
static UInt LOCAL_ReadFreqSinAbs( SSC_BITS *pBits, UInt nFreqGranularity)
{
    UInt                   Frequency ;
    HUFFMAN_DECODER const *pHuffman = &hufFreqSinAbs;

    assert( nFreqGranularity < 4 );
    Frequency = SSC_BITS_ReadHuffman( pBits, pHuffman );
    Frequency += LOCAL_ReadFine( pBits, nFreqGranularity );
    assert( Frequency <= 3880 );

    return Frequency ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadFreqContAbs
 *
 * Description : Reads the abs sine frequency value with different granularity values
 *               depending on the defined freq granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               FreqGranularity - current granularity to use
 *
 * Returns     : Frequency - the abs cont sine frequency in the most fine grid
 *
 *****************************************************************************/
static UInt LOCAL_ReadFreqContAbs( SSC_BITS *pBits, UInt nFreqGranularity)
{
    UInt                   Frequency ;
    HUFFMAN_DECODER const *pHuffman = &hufFreqContAbs;

    assert( nFreqGranularity < 4 );
    Frequency = SSC_BITS_ReadHuffman( pBits, pHuffman );
    Frequency += LOCAL_ReadFine( pBits, nFreqGranularity );

    assert( Frequency <= 3880 );

    return Frequency ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadFreqBirthRel
 *
 * Description : Reads the abs birth frequency value with different granularity values
 *               depending on the defined freq granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               FreqGranularity - current granularity to use
 *
 * Returns     : DeltaFreq - the sine birth delta frequency in the most fine grid
 *
 *****************************************************************************/
static UInt LOCAL_ReadFreqBirthRel( SSC_BITS *pBits, UInt nFreqGranularity)
{
    UInt                   DeltaFreq ;
    HUFFMAN_DECODER const *pHuffman = &hufFreqBirthRel;

    assert( nFreqGranularity < 4 );
    DeltaFreq = SSC_BITS_ReadHuffman( pBits, pHuffman );
    DeltaFreq += LOCAL_ReadFine( pBits, nFreqGranularity );

    assert( DeltaFreq <= 3880 );

    return DeltaFreq ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadAmpSinAbs
 *
 * Description : Reads the abs birth amplitude value with different granularity values
 *               depending on the defined amp granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               nAmpGranularity - current granularity to use
 *
 * Returns     : Amplitude - the sine abs amplitude in the most fine grid
 *
 *****************************************************************************/
static UInt LOCAL_ReadAmpSinAbs( SSC_BITS *pBits, UInt nAmpGranularity)
{
    UInt                   Amplitude ;
    HUFFMAN_DECODER const *pHuffman = &hufAmpSinAbs;

    assert( nAmpGranularity < 4 );
    Amplitude = SSC_BITS_ReadHuffman( pBits, pHuffman );
    Amplitude += LOCAL_ReadFine( pBits, nAmpGranularity );

    assert( Amplitude <= 241 );

    return Amplitude ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadAmpContAbs
 *
 * Description : Reads the abs cont amplitude value with different granularity values
 *               depending on the defined amp granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               nAmpGranularity - current granularity to use
 *
 * Returns     : Amplitude - the sine abs amplitude in the most fine grid
 *
 *****************************************************************************/
static UInt LOCAL_ReadAmpContAbs( SSC_BITS *pBits, UInt nAmpGranularity)
{
    UInt                   Amplitude ;
    HUFFMAN_DECODER const *pHuffman = &hufAmpContAbs;

    assert( nAmpGranularity < 4 );
    Amplitude = SSC_BITS_ReadHuffman( pBits, pHuffman );
    Amplitude += LOCAL_ReadFine( pBits, nAmpGranularity );

    assert( Amplitude <= 241 );

    return Amplitude ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadAmpBirthRel
 *
 * Description : Reads the birth delta amplitude value with different granularity values
 *               depending on the defined amp granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               AmpGranularity - current granularity to use
 *
 * Returns     : DeltaFreq - the sine birth delta amplitude in the most fine grid
 *
 *****************************************************************************/
static SInt LOCAL_ReadAmpBirthRel( SSC_BITS *pBits, UInt nAmpGranularity)
{
    SInt                   sDeltaAmp ;
    HUFFMAN_DECODER const *pHuffman = &hufAmpBirthRel;

    assert( nAmpGranularity < 4 );
    sDeltaAmp = SSC_BITS_ReadHuffman( pBits, pHuffman );
    sDeltaAmp += LOCAL_ReadFine( pBits, nAmpGranularity );

    assert( sDeltaAmp >= -241 );
    assert( sDeltaAmp <=  241 );

    return sDeltaAmp ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadAmpDelta
 *
 * Description : Reads the delta amplitude value with different granularity values
 *               depending on the defined amp granularity for this frame.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               AmpGranularity - current granularity to use
 *
 * Returns     : sDeltaAmp - the amplitude delta in the most fine grid
 *
 *****************************************************************************/
static SInt LOCAL_ReadAmpDelta( SSC_BITS *pBits, UInt nAmpGranularity)
{
    SInt                   sDeltaAmp ;
    HUFFMAN_DECODER const *pHuffman = &hufAmpContRel_3;

    assert( nAmpGranularity < 4 );
    switch( nAmpGranularity )
    {
        case 0:
            pHuffman = &hufAmpContRel_0 ;
            break ;
        case 1:
            pHuffman = &hufAmpContRel_1 ;
            break ;
        case 2:
            pHuffman = &hufAmpContRel_2 ;
            break ;
        case 3:
            pHuffman = &hufAmpContRel_3 ;
            break ;
        default:
            assert(0);
            break ;
    }
    sDeltaAmp = SSC_BITS_ReadHuffman( pBits, pHuffman );

    assert( sDeltaAmp >= -32 );
    assert( sDeltaAmp <=  32 );

    return sDeltaAmp ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadPaddingBits
 *
 * Description : read padding bits from bitstream
 *
 * Parameters  : - pBits : pointer to bit stream
 *
 *               - the_exception_context : pointer to the current exception
 *                 context environment
 *
 * Returns     : value of the padding bits read (should be zero)
 *
 *****************************************************************************/


static UInt LOCAL_ReadPaddingBits(SSC_BITS *pBits,
                   struct exception_context *the_exception_context)

{
    UInt PadBits ;
    UInt Padding = 0 ;
    PadBits = 8 - (SSC_BITS_GetOffset(pBits) & 7);
    if((PadBits != 8) && (PadBits > 0))
    {
        Padding = SSC_BITS_Read(pBits, PadBits);

        if(Padding != 0)
        {
            Throw SSC_ERROR_VIOLATION | SSC_ERROR_PADDING;
        }
    }
    assert( Padding == 0 );

    return Padding ;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_CalcJitter
 *
 * Description : calculate freq jitter random value
 *
 * Parameters  : - pHeader : pointer to bit stream header data structure
 *
 *               - pBitStreamData : pointer to bit stream data instance
 *
 *               - channel number to generate random value of
 *
 * Returns     : random value to be used as phase (freq) jitter
 *
 *****************************************************************************/

static SInt LOCAL_CalcJitter( const SSC_BITSTREAM_HEADER *pHeader, SSC_BITSTREAM_DATA* pBitStreamData, SInt nCh )
{
    SSC_SAMPLE_INT d           = 1/4294967296.0; /* = 2^-32 */
    UInt32         X           = pBitStreamData->nJitterRandomSeed[nCh];
    Double         dMaxJitter  = pHeader->phase_max_jitter ;

    /* Generate normally distributed random number */
    X    = 1664525 * X + 1013904223;

    /* Store number */
    pBitStreamData->nJitterRandomSeed[nCh] = X;
    return (SInt)floor( dMaxJitter * (2.0*X*d-1) + 0.5  );
}


/*************************LOCAL FUNCTION**************************************
 * 
 * Name        : LOCAL_ConvertToGrid
 *
 * Description : converts/rounds val to specified grid 
 *
 * Parameters  : 
 *               
 *
 * Returns     : -
 * 
 *****************************************************************************/

static SInt LOCAL_ConvertToGrid( UInt val, UInt grid )
{
    return (SInt) (floor( ((Double)val/(1 << grid)) + 0.5) * (1 << grid));
}


/*************************LOCAL FUNCTION**************************************
 * 
 * Name        : LOCAL_Freq2Erb
 *
 * Description : onvert frequency from Hertz to Erb scale. 
 *
 * Parameters  : 
 *               
 *
 * Returns     : -
 * 
 *****************************************************************************/
static Double LOCAL_Freq2erb( Double x )
{
    return 21.4*log10(0.00437*x+1);
}

/*************************LOCAL FUNCTION**************************************
 * 
 * Name        : LOCAL_QuantFreq
 *
 * Description : Quantises a frequency to a specified grid 
 *
 * Parameters  : 
 *               
 *
 * Returns     : -
 * 
 *****************************************************************************/
static SInt LOCAL_QuantFreq( Double dFreq, Double dFreq_rl_base, Double dFs, SInt grid_freq )
{
    SSC_ERROR      Error = SSC_ERROR_OK ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           nFreq = 0;
    Try
    {
        nFreq = (SInt)( floor( dFreq_rl_base * LOCAL_Freq2erb( dFreq * dFs / (2*SSC_PI)) / pow(2, grid_freq) + 0.5) * pow( 2, grid_freq ) );
    }
    Catch(Error)
    {
        nFreq = 0 ;
    }
    return nFreq ;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : LOCAL_DecodeAdpcm
 *
 * Description : Decode the adpcm coded frequency/phase values and check 
 *               resulting range.
 *
 * Parameters  : pCurrFrame - Pointer to bit-stream parser instance for the current frame.
 *
 *               Subframe   - Number of subframe to be decoded.
 *
 *               Channel    - Number of channel to be decoded.
 *
 8               pSinusoids - Decoded sinusoids 
 *
 * Returns     : Number of sinusoids
 *               
 *
 *****************************************************************************/
SInt LOCAL_DecodeAdpcm(SSC_BITSTREAMPARSER*  pCurrFrame,
                       SInt                  Subframe,
                       UInt                  Channel,
                       Double                PitchFactor,
                       SSC_SIN_SEGMENT*      pSinusoids)
{
    SSC_BITSTREAM_HEADER* pHeader;
    SSC_SIN_SEGMENT*      pSineParam0 = pSinusoids;
    UInt                  sf_size; 
    UInt                  Ch = Channel;
    UInt                  Sf = Subframe + HISTORY_SINUSOID;
    UInt                  NumSinusoids;
    Bool                  bContPhase = False;

    /* Check input arguments in debug builds */
    assert(pCurrFrame);
    pHeader = &(pCurrFrame->Header);
    assert(Channel  <  pHeader->nrof_channels_bs);
    assert(Subframe >= -1);
    assert(Subframe <  (SInt)pHeader->nrof_subframes);

    /* Get sub-frame size */
    sf_size = pHeader->update_rate;
    if ((PitchFactor != 1.0)  ||  (sf_size != T))
    {
        /* In case of time/pitch scaling switch to continuous mode */ 
        bContPhase = True;
    }

    /* Get number of sinusoids */
    NumSinusoids = pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].s_nrof_continuations
                   + pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].s_nrof_births;

    if (Subframe < 0)
    {
        /* For the history frames, get the correct parameters from the memory */
        UInt i;

        SSC_SIN_QSEGMENT *pSineParamQ0 = pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].SineParam;

        for (i = 0; i < pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].s_nrof_continuations; i++)
        {
            SSC_SIN_ADPCM *pSineAdpcm0   = &(pSineParamQ0[i].s_adpcm);
            pSineParam0[i].Sin.Frequency = pSineAdpcm0->s_adpcm_state.Freqfilt; 
            pSineParam0[i].Sin.Phase     = pSineAdpcm0->s_adpcm_state.Phase;
            pSineParam0[i].Sin.Amplitude = LOCAL_DequantAmpl(pSineParamQ0[i].Sin.Amplitude);   
            pSineParam0[i].State         = pSineParamQ0[i].State;
        }

        /* Process births in current sub-frame */
        for (; i < NumSinusoids; i++)
        {
            /* This is only called for births */
            if (bContPhase)
            {
                pSineParam0[i].Sin.Frequency = LOCAL_DequantFreq(pSineParamQ0[i].Sin.Frequency
                    +(pSineParamQ0[i].State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR,
                    pHeader->sampling_rate);
            }
            else
            {
                pSineParam0[i].Sin.Frequency = LOCAL_DequantFreq(pSineParamQ0[i].Sin.Frequency,
                    pHeader->sampling_rate);
            }
            pSineParam0[i].Sin.Amplitude = LOCAL_DequantAmpl(pSineParamQ0[i].Sin.Amplitude);
            pSineParam0[i].Sin.Phase     = LOCAL_DequantPhasePrecise(pSineParamQ0[i].Sin.Phase);

            pSineParam0[i].State         = pSineParamQ0[i].State;
        }
    }
    else
    {
        /* pSineParamp1,0..2 contain pointers to the SineParam structures for 
         * the previous, current, next and next next subframe respectively   */
        SSC_SIN_QSEGMENT *pSineParamQp1 = pCurrFrame->pBitStreamData->Sinusoid[Sf-1][Ch].SineParam;
        SSC_SIN_QSEGMENT *pSineParamQ0  = pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].SineParam;
        SSC_SIN_QSEGMENT *pSineParamQ1  = pCurrFrame->pBitStreamData->Sinusoid[Sf+1][Ch].SineParam;
        SSC_SIN_QSEGMENT *pSineParamQ2  = pCurrFrame->pBitStreamData->Sinusoid[Sf+2][Ch].SineParam;
        UInt i, p, q;
        
        /* Process continuations in current sub-frame */
        p = 0; /* Index of sinusoid in current sub-frame + 1 */
        q = 0; /* Index of sinusoid in current sub-frame + 2 */
        for (i = 0; i < pCurrFrame->pBitStreamData->Sinusoid[Sf][Ch].s_nrof_continuations; i++)
        {
            SSC_SIN_ADPCM *pSineAdpcm0  = &(pSineParamQ0[i].s_adpcm);
            SSC_SIN_ADPCM *pSineAdpcmp1 = NULL;
            SSC_SIN_ADPCM *pSineAdpcm1  = NULL;
            SSC_SIN_ADPCM *pSineAdpcm2  = NULL;
            UInt nSinus = pCurrFrame->pBitStreamData->Sinusoid[Sf-1][Ch].s_nrof_continuations 
                + pCurrFrame->pBitStreamData->Sinusoid[Sf-1][Ch].s_nrof_births;
            Double Freq;
            Double Phase;
            Double Amp;

            UInt n;
            UInt m = 0;
            Bool stop = False;

            if (pSineAdpcm0->bValid) /* ADPCM encoded continuations, excluding refresh subframes */
            {
                /* Obtain the ADPCM structure of the previous sub-frame */
                for ( n = 0; (n < nSinus) && (stop == False); n++ )
                {
                    if ( pSineParamQp1[n].s_cont > 0 )
                    {
                        m++;
                    }
                    if ( m == i + 1 )
                    {
                        stop = True;
                        pSineAdpcmp1 = &(pSineParamQp1[n].s_adpcm);
                    }
                }

                if (stop == False)
                {
                    /* Previous structure could not be found, give error */
                    return 0;
                }
            
                /* verify sequentially whether the track continues in the next 2 sub-frames and no refresh occurs
                   and assign proper pointer values to the ADPCM structs */

                if ((pSineParamQ0[i].s_cont > 0) && (pSineParamQ1[p].s_adpcm.bValid==True))
                {
                    pSineAdpcm1 = &(pSineParamQ1[p].s_adpcm);
                    if ((pSineParamQ0[i].s_cont > 1) && (pSineParamQ2[q].s_adpcm.bValid==True))
                    {
                        pSineAdpcm2 = &(pSineParamQ2[q].s_adpcm);
                        q++;
                    }
                    p++;
                }

                /* pSineAdpcmx is NULL if track is not present in that sub-frame */
                
                LOCAL_DecodeAdpcmTrack( pSineParamQ0[i].s_length_refresh,
                                        pSineAdpcmp1,
                                        pSineAdpcm0,
                                        pSineAdpcm1,
                                        pSineAdpcm2,
                                        &Freq, 
                                        &Phase);
        
                Amp = LOCAL_DequantAmpl(pSineParamQ0[i].Sin.Amplitude);            
            }
            else /* Refreshed continuations */
            {
                /* Dequantise the sinusoidal parameters */
                Amp   = LOCAL_DequantAmpl(pSineParamQ0[i].Sin.Amplitude);
                Freq  = LOCAL_DequantFreq(pSineParamQ0[i].Sin.Frequency, pHeader->sampling_rate);
                Phase = LOCAL_DequantPhasePrecise(pSineParamQ0[i].Sin.Phase);           
                /* Save data from Refreshed continuations into ADPCM structure */
                if (pSineParamQ0[i].s_cont > 0)
                {
                    SSC_SIN_ADPCM *pSineAdpcm0 = &(pSineParamQ0[i].s_adpcm);
    
                    pSineAdpcm0->s_adpcm_state.PQ            = Phase;
                    pSineAdpcm0->s_adpcm_state.Yphase        = Phase;     /* Unsmoothed unwrapped phase */
                    pSineAdpcm0->s_adpcm_state.Yphasefilt    = Phase;     /* Smoothed unwrapped phase   */
                    pSineAdpcm0->s_adpcm_state.Phase         = Phase;     /* Smoothed wrapped phase     */
                    pSineAdpcm0->s_adpcm_state.FQ            = Freq;
                    pSineAdpcm0->s_adpcm_state.Freq          = Freq; /* Unsmoothed frequency       */
                    pSineAdpcm0->s_adpcm_state.Freqfilt      = Freq; /* Smoothed frequency         */
                }
            }
            
            /* Check if pitch scaling is needed */
            if (bContPhase && (pSineAdpcmp1 != NULL))
            {
                Double PrevFreq, PrevPhase;
                Double PrevFreqCP, FreqCP;

                /* Get data from previous frame */
                PrevFreq  = pSineAdpcmp1->s_adpcm_state.Freqfilt; 
                PrevPhase = pSineAdpcmp1->s_adpcm_state.Phase;

                /* If in continuous phase mode: calculate jitter */
                if (pHeader->phase_jitter_present)
                {
                    Double NewFreqQ, OldFreqQ, FreqQ;

                    /* Calculate jitter */
                    SInt Fadj = LOCAL_CalcJitter( pHeader, pCurrFrame->pBitStreamData, Ch );
                    
                    pSineParamQ0[i].State &= ~SSC_SINMASK_FADJ; /* clear Fadj   */

                    /* Quantise the frequencies in order to apply jitter on the indices */
                    NewFreqQ = LOCAL_QuantFreq(Freq, SSC_S_FREQUENCY_BASE , pHeader->sampling_rate, pHeader->freq_granularity );
                    FreqQ    = NewFreqQ;
                    OldFreqQ = LOCAL_QuantFreq(PrevFreq, SSC_S_FREQUENCY_BASE , pHeader->sampling_rate, pHeader->freq_granularity );
               
                    /* Update the jitter state */
                    if( NewFreqQ > (Double)pHeader->phase_jitter_band )
                    {
                        pSineParamQ0[i].State |= (Fadj << SSC_FADJ_SHIFT) ;
                    }
                    
                    /* Add jitter to the frequencies before phase continuation */
                    NewFreqQ += (Double)(Fadj/JITTER_SCALE_FACTOR);
                    OldFreqQ += (Double)((pSineParamQ0[i].State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR);
                              
                    /* De-quantise the frequencies */
                    FreqCP      = LOCAL_DequantFreq(NewFreqQ, pHeader->sampling_rate);
                    PrevFreqCP  = LOCAL_DequantFreq(OldFreqQ, pHeader->sampling_rate);
                
                    /* Add the jitter to the reconstructed frequency */
                    FreqQ      += (Double)((pSineParamQ0[i].State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR);
                    Freq        =  LOCAL_DequantFreq(NewFreqQ, pHeader->sampling_rate);
                }
                else
                {
                    FreqCP     = Freq;
                    PrevFreqCP = PrevFreq;
                }

                /* Calculate continuous phase */
                Phase = LOCAL_CalcContinuousPhase(PrevFreqCP, PrevPhase, FreqCP, 2*sf_size, PitchFactor);

                /* Store modified values for freq and phase in ADPCM state */
                pSineAdpcm0->s_adpcm_state.Freqfilt = Freq; 
                pSineAdpcm0->s_adpcm_state.Phase    = Phase;
            }

            /* Store results */
            pSineParam0[i].Sin.Frequency = Freq; 
            pSineParam0[i].Sin.Phase     = Phase;
            pSineParam0[i].Sin.Amplitude = Amp;
            pSineParam0[i].State         = pSineParamQ0[i].State;
        }
    
        /* Process births in current sub-frame */
        for (; i < NumSinusoids; i++)
        {
            /* This is only called for births */
            pSineParam0[i].Sin.Frequency = LOCAL_DequantFreq(pSineParamQ0[i].Sin.Frequency,
                pHeader->sampling_rate);
            pSineParam0[i].Sin.Amplitude = LOCAL_DequantAmpl(pSineParamQ0[i].Sin.Amplitude);
            pSineParam0[i].Sin.Phase     = LOCAL_DequantPhasePrecise(
                pSineParamQ0[i].Sin.Phase);

            pSineParam0[i].State         = pSineParamQ0[i].State;
            
            /* Save data from births into ADPCM structure */
            if (pSineParamQ0[i].s_cont > 0)
            {
                SSC_SIN_ADPCM *pSineAdpcm0 = &(pSineParamQ0[i].s_adpcm);

                pSineAdpcm0->s_adpcm_state.PQ            = pSinusoids[i].Sin.Phase;
                pSineAdpcm0->s_adpcm_state.Yphase        = pSinusoids[i].Sin.Phase;     /* Unsmoothed unwrapped phase */
                /* pSineAdpcm0->s_adpcm_state.Yphasefilt will be filled during first continuation */
                pSineAdpcm0->s_adpcm_state.Phase         = pSinusoids[i].Sin.Phase;     /* Smoothed wrapped phase     */
                pSineAdpcm0->s_adpcm_state.FQ            = pSinusoids[i].Sin.Frequency;
                pSineAdpcm0->s_adpcm_state.Freq          = pSinusoids[i].Sin.Frequency; /* Unsmoothed frequency       */
                pSineAdpcm0->s_adpcm_state.Freqfilt      = pSinusoids[i].Sin.Frequency; /* Smoothed frequency         */
            }
            /* Add jitter in case of continuous phase mode  */
            if (bContPhase)
            {
                pSineParam0[i].Sin.Frequency = LOCAL_DequantFreq(pSineParamQ0[i].Sin.Frequency
                    +(pSineParamQ0[i].State >> SSC_FADJ_SHIFT)/JITTER_SCALE_FACTOR,
                    pHeader->sampling_rate);
            }

            /* Pitch scaling is done in synthesis control */
        }

    }
    return NumSinusoids;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : LOCAL_DecodeAdpcmTrack
 *
 * Description : Decode the adpcm coded frequency/phase values 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_DecodeAdpcmTrack( 
      UInt              length,    /* In: This flag indicates birth, continuation and first continuation */
      SSC_SIN_ADPCM   * s_adpcmp1, /* In:     Current sub frame - 1 */
      SSC_SIN_ADPCM   * s_adpcm0,  /* In/Out: Current sub frame     */
      SSC_SIN_ADPCM   * s_adpcm1,  /* In/Out: Current sub frame + 1 */
      SSC_SIN_ADPCM   * s_adpcm2,  /* In/Out: Current sub frame + 2 */
      Double          * Frequency, /* Out: Frequency */
      Double          * Phase      /* Out: Phase     */
)
{
  /* length == 1: Birth */
  /* length == 2: First continuation */
  /* length > 2: All later continuations */
  if (length == 2)
  {
      /* Decode first continuation */
      LOCAL_InitADPCMState(s_adpcmp1, &(s_adpcm0->s_adpcm_state));
      LOCAL_decode_ADPCM_frame(&(s_adpcm0->s_adpcm_state), s_adpcm0->s_fr_ph);  

      /* Calculate the Yphasefilt of the birth (known as Psi(K) in the standard) */
      s_adpcmp1->s_adpcm_state.Yphasefilt = 0.25 * s_adpcm0->s_adpcm_state.yz[1] + 
                                            0.5  * s_adpcm0->s_adpcm_state.yz[0] + 
                                            0.25 * s_adpcm0->s_adpcm_state.Yphase;

      /* Calculate all sub-frames if present */
      if (s_adpcm2 != NULL)      /* Current + 1 and Current + 2 are present */
      {
          LOCAL_UpdateADPCMState(&(s_adpcm0->s_adpcm_state), s_adpcm0->s_fr_ph, &(s_adpcm1->s_adpcm_state));
          LOCAL_decode_ADPCM_frame(&(s_adpcm1->s_adpcm_state), s_adpcm1->s_fr_ph);
          
          LOCAL_UpdateADPCMState(&(s_adpcm1->s_adpcm_state), s_adpcm1->s_fr_ph, &(s_adpcm2->s_adpcm_state));
          LOCAL_decode_ADPCM_frame(&(s_adpcm2->s_adpcm_state), s_adpcm2->s_fr_ph);
          
          LOCAL_FiltPhaseTrack(&(s_adpcmp1->s_adpcm_state),&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
          LOCAL_FiltPhaseTrack(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state), &(s_adpcm2->s_adpcm_state));
 
          LOCAL_CalcFreqPhase(&(s_adpcmp1->s_adpcm_state), &(s_adpcm0->s_adpcm_state));
          LOCAL_CalcFreqPhase(&(s_adpcm0->s_adpcm_state),  &(s_adpcm1->s_adpcm_state));
          
          LOCAL_FiltFreqFirst(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
      }
      else
      {
         if (s_adpcm1 != NULL)     /* Current + 1 is present */
         {
             LOCAL_UpdateADPCMState(&(s_adpcm0->s_adpcm_state), s_adpcm0->s_fr_ph, &(s_adpcm1->s_adpcm_state));
             LOCAL_decode_ADPCM_frame(&(s_adpcm1->s_adpcm_state), s_adpcm1->s_fr_ph);
          
             LOCAL_FiltPhaseTrack(&(s_adpcmp1->s_adpcm_state),&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
             LOCAL_FiltPhaseEnd(&(s_adpcm1->s_adpcm_state));
             
             LOCAL_CalcFreqPhase(&(s_adpcmp1->s_adpcm_state),&(s_adpcm0->s_adpcm_state));
             LOCAL_CalcFreqPhase(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));

             LOCAL_FiltFreqFirst(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
         } 
         else /* Only current frame is present */
         {
             LOCAL_FiltPhaseEnd(&(s_adpcm0->s_adpcm_state));
             LOCAL_CalcFreqPhase(&(s_adpcmp1->s_adpcm_state), &(s_adpcm0->s_adpcm_state));
             LOCAL_FiltFreqEnd(&(s_adpcmp1->s_adpcm_state),&(s_adpcm0->s_adpcm_state));
         }
      }            
  }    
  else
  {
      if (s_adpcm2 != NULL)  /* Current + 1 and current + 2 are present */
      {
          LOCAL_UpdateADPCMState(&(s_adpcm1->s_adpcm_state), s_adpcm1->s_fr_ph, &(s_adpcm2->s_adpcm_state));
          LOCAL_decode_ADPCM_frame(&(s_adpcm2->s_adpcm_state), s_adpcm2->s_fr_ph);

             LOCAL_FiltPhaseTrack(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state), &(s_adpcm2->s_adpcm_state));
          LOCAL_CalcFreqPhase(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
          LOCAL_FiltFreqTrack(&(s_adpcmp1->s_adpcm_state),&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
      }
      else
      {
          if (s_adpcm1 != NULL)  /* Current frame + 1 is present */
          {
              LOCAL_FiltPhaseEnd(&(s_adpcm1->s_adpcm_state));
              LOCAL_CalcFreqPhase(&(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
              LOCAL_FiltFreqTrack(&(s_adpcmp1->s_adpcm_state), &(s_adpcm0->s_adpcm_state), &(s_adpcm1->s_adpcm_state));
          }
          else           /* Only current frame is present */
          {
              LOCAL_FiltFreqEnd(&(s_adpcmp1->s_adpcm_state), &(s_adpcm0->s_adpcm_state));
          }  
      }
  }    
  /* Output */
  *Phase      = s_adpcm0->s_adpcm_state.Phase;
  *Frequency  = s_adpcm0->s_adpcm_state.Freqfilt;
    
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_InitADPCMState(
      SSC_SIN_ADPCM       * s_adpcmp1,      
      SSC_SIN_ADPCM_STATE * s_adpcm_state
)
{
    Double F;
    Double scalefac;
    Double Phase, Frequency;
    UInt i;

    Frequency = s_adpcmp1->s_adpcm_state.FQ;   /* Frequency of the birth */
    Phase     = s_adpcmp1->s_adpcm_state.PQ;   /* Phase of the birth     */

    /* Initialise the memory */
    s_adpcm_state->yz[0] = Phase;
    s_adpcm_state->yz[1] = Phase - Frequency * T;

    /* Determine scale factor */
    F = Frequency;
    i = 0;
    while (F > SSC_ADPCM_FF[i])
    { 
        i++;
    }    
    scalefac = SSC_ADPCM_FS[i-1];

    /* Initialise the tables */
    if (s_adpcmp1->bGridValid)
    {
        /* For a refresh, use the table that is indicated by the transmitted index */
        UInt index, ii;
        UInt Levels = 1 << SSC_BITS_S_FREQ_PHASE_ADPCM;
  
        /* Calculate table index from adpcm_grid */
        index =  s_adpcmp1->s_adpcm_grid * Levels;

        /* Put into s_adpcm->delta and s_adpcm->repr */  
        for (ii=0; ii < Levels; ii++)
        {
           s_adpcm_state->repr[ii] = SSC_ADPCM_TableRepr[index+ii];
        }    

        /* Calculate table index from adpcm_grid, table has one extra column */
        index =  s_adpcmp1->s_adpcm_grid * (Levels+1);

        for (ii=0; ii < Levels+1; ii++)
        {
           s_adpcm_state->delta[ii] = SSC_ADPCM_TableQuant[index+ii];
        }    
            
    }    
    else
    {    
        /* Use default table (for births) */
        s_adpcm_state->delta[0] = SSC_ADPCM_Q[0];
        s_adpcm_state->delta[4] = SSC_ADPCM_Q[4];
        for (i=1; i < 4; i++)
        {
          s_adpcm_state->delta[i] = SSC_ADPCM_Q[i]/scalefac;
        }
        for (i=0; i < 4; i++)
        {
          s_adpcm_state->repr[i] = SSC_ADPCM_R[i]/scalefac;
        }
    }
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_UpdateADPCMState(
      SSC_SIN_ADPCM_STATE * s_adpcm0,  /* In:  Previous sub frame        */
      UInt                  AdpcmIn0,  /* In:  Previous 2 ADPCM bits   */
      SSC_SIN_ADPCM_STATE * s_adpcm1   /* Out: Current sub frame       */
)        
{
    UInt i;
    Double D_q;
    
    /* Take dequantised value from the previous sub-frame */
    D_q = s_adpcm0->repr[AdpcmIn0]; 

    /* Update the ADPCM memory of the current sub-frame */
    s_adpcm1->yz[1] = s_adpcm0->yz[0];
    s_adpcm1->yz[0] = s_adpcm0->Yphase;

    /* UPDATE DELTA for current sub-frame */
    for (i=0; i < 5; i++)
    {
        s_adpcm1->delta[i] = s_adpcm0->delta[i];
    }
    for (i=0; i < 4; i++)
    {
        s_adpcm1->repr[i] = s_adpcm0->repr[i];
    }
    LOCAL_update_delta(s_adpcm1->delta, s_adpcm1->repr,  D_q, SSC_ADPCM_delta_max, SSC_ADPCM_delta_min, SSC_ADPCM_P_m); 
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_update_delta(
        Double * delta,
        Double * repr,
        Double C_next,
        const Double delta_max,
        const Double delta_min,
        const Double * m
)
{
    Double din, deltaIN, c;
    UInt i;
    
    din      = C_next;
    deltaIN  = delta[3] - delta[2]; /* Inner level */
    
    /* Select multiplier */
    if ( fabs(din) < deltaIN )
    {
        c = m[0];
    }
    else
    {
        c = m[1];
    }    
        
    /* Check if scaling is still needed */
    deltaIN = deltaIN * c;
    if ((deltaIN <= delta_min ) || (deltaIN >= delta_max))
    {
        c = 1.0;
    }
       
    /* Scale table */
    repr[0] *= c;
    for (i=1; i<4; i++)
    {
        delta[i] *= c;
        repr[i]  *= c;
    }     
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_decode_ADPCM_frame(
      SSC_SIN_ADPCM_STATE * s_adpcm0,  /* In/Out: Current sub frame       */
      UInt          AdpcmIn            /* In: 2 ADPCM bits */
)
{        
    /* Variables */
    Double Xh, D_q, Decoder_out;
                    
    /* Calculate output for first continuation */
    Xh  = s_adpcm0->yz[0] * SSC_ADPCM_P_h[0] + s_adpcm0->yz[1] * SSC_ADPCM_P_h[1];
    D_q = s_adpcm0->repr[AdpcmIn]; 
    Decoder_out = D_q + Xh;
    
    /* Generate output for current frame */
    s_adpcm0->Yphase = Decoder_out;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_FiltPhaseTrack(
      SSC_SIN_ADPCM_STATE * s_adpcmp1, /* In:     Current sub frame - 1 */
      SSC_SIN_ADPCM_STATE * s_adpcm0,  /* In/Out: Current sub frame     */
      SSC_SIN_ADPCM_STATE * s_adpcm1   /* In:     Current sub frame + 1 */
)        
{
    Double Pprev = s_adpcmp1->Yphase;
    Double Pcurr = s_adpcm0->Yphase;
    Double Pnext = s_adpcm1->Yphase; 
    Double sum;
    
    /* Low pass filter */
    sum = 0.25 * Pprev + 0.5 * Pcurr + 0.25 * Pnext;
    
    s_adpcm0->Yphasefilt = sum;
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_FiltPhaseEnd(
      SSC_SIN_ADPCM_STATE * s_adpcm0   /* In/Out: Current sub frame       */
)        
{
    Double Pcurr = s_adpcm0->Yphase;
    
    s_adpcm0->Yphasefilt = Pcurr;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_CalcFreqPhase(
      SSC_SIN_ADPCM_STATE * s_adpcmp1, /* In:     Current sub frame - 1 */
      SSC_SIN_ADPCM_STATE * s_adpcm0   /* In/Out: Current sub frame        */
)        
{
     Double FUprev = s_adpcmp1->Yphasefilt;
    Double FUcurr = s_adpcm0->Yphasefilt;
    
    Double Fprev  = s_adpcmp1->Freq;        
    
    Double Phase  = fmod(FUcurr-SSC_PI, 2*SSC_PI)-SSC_PI;

    Double Fnew   = ((FUcurr-FUprev))/(T/2.0) - Fprev;
     
    s_adpcm0 -> Freq  = Fnew;
    s_adpcm0 -> Phase = Phase;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_FiltFreqFirst(
      SSC_SIN_ADPCM_STATE * s_adpcm0,  /* In/Out: Current sub frame       */
      SSC_SIN_ADPCM_STATE * s_adpcm1   /* In:     Current sub frame + 1 */
)        
{
    Double Fcurr = s_adpcm0->Freq;
    Double Fnext = s_adpcm1->Freq; 
    Double sum;
    
    /* Low pass filter */
    sum = 0.5 * Fcurr + 0.5 * Fnext;
    
    s_adpcm0->Freqfilt = sum;
}

/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_FiltFreqTrack(
      SSC_SIN_ADPCM_STATE * s_adpcmp1, /* In:     Current sub frame - 1 */
      SSC_SIN_ADPCM_STATE * s_adpcm0,  /* In/Out: Current sub frame     */
      SSC_SIN_ADPCM_STATE * s_adpcm1   /* In:     Current sub frame + 1 */
)        
{
    Double Fprev = s_adpcmp1->Freq;
    Double Fcurr = s_adpcm0->Freq;
    Double Fnext = s_adpcm1->Freq; 
    Double sum;
    
    /* Low pass filter */
    sum = 0.25 * Fprev + 0.5 * Fcurr + 0.25 * Fnext;
    
    s_adpcm0->Freqfilt = sum;
}


/*************************GLOBAL FUNCTION**************************************
 *
 * Name        : 
 *
 * Description : 
 *
 * Parameters  : 
 *
 * Returns     : -
 *
 *****************************************************************************/
static void LOCAL_FiltFreqEnd(
      SSC_SIN_ADPCM_STATE * s_adpcmp1, /* In:     Current sub frame - 1 */
      SSC_SIN_ADPCM_STATE * s_adpcm0   /* In/Out: Current sub frame       */
)        
{
    Double Fprev = s_adpcmp1->Freq;
    Double Fcurr = s_adpcm0->Freq;
    
    s_adpcm0->Freqfilt = 0.5 * Fcurr + 0.5 * Fprev;
}

/*************************LOCAL FUNCTION**************************************
 * 
 * Name        : LOCAL_ReadAdpcm
 *
 * Description : Reads a two bits ADPCM value from the stream
 *
 * Parameters  : 
 *               
 *
 * Returns     : a signed char containing the value of the dibit.
 * 
 *****************************************************************************/

static SInt LOCAL_ReadAdpcm( SSC_BITS *pBits )
{
    SInt FPadpcm;

    FPadpcm = SSC_BITS_Read( pBits, SSC_BITS_S_FREQ_PHASE_ADPCM );

    return FPadpcm;
}

/*************************STATIC FUNCTION**************************************
 *
 * Name        : LOCAL_ReadAdpcmGrid
 *
 * Description : Reads the ADPCM grid information from the bit stream.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 * Returns     : ADPCMGrid - the grid number to be used by ADPCM quantiser
 *
 *****************************************************************************/
static UInt LOCAL_ReadAdpcmGrid( SSC_BITS *pBits )
{
    UInt                   Grid ;
    HUFFMAN_DECODER const *pHuffman = &hufAdpcmGrid;

    Grid = SSC_BITS_ReadHuffman( pBits, pHuffman );

    return Grid ;
}


