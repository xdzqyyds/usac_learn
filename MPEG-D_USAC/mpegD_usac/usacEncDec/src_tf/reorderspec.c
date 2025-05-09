/*****************************************************************************
 *                                                                           *
 "This software module was originally developed by 

 Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
 
 in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
 Audio standard. ISO/IEC  gives uers of the MPEG-2 AAC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1997,1998,1999.
 *                                                                           *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "allHandles.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "bitfct.h"
#include "bitstream.h"
#include "buffers.h"
#include "common_m4a.h"
#include "concealment.h"
#include "huffdec2.h"
#include "interface.h"
#include "port.h"
#include "reorderspec.h"
#include "resilience.h"

/* #define DEBUG_HCR_SQUARE */

typedef enum {
  LEFT,
  RIGHT,
  MAX_SIDES
} SIDES;

typedef struct {
  unsigned short alreadyDecoded;
  unsigned long  code;
  unsigned long  codeExt;
  unsigned short nrOfBits;
  int*           qp;
  unsigned short codebook;
  unsigned short step;
  unsigned short maxCWLen;
  unsigned short firstLine;
} BASE_CW_INFO; 

typedef struct {
  unsigned long  word0;
  unsigned long  word1;
  unsigned long  word2;
  unsigned short segmentRestBits;
} SEGMENT_INFO;

typedef struct tag_hcr 
{
  unsigned short lenOfLongestCw;
  unsigned short lenOfSpecData;
  unsigned short remainingBitCnt;
  unsigned short specLineCnt;
  unsigned char  statusFlag;
  unsigned short decodedBitCnt;
  unsigned short currentSegment;
  unsigned short currentCodeword;
  unsigned short nrOfSegments;
  unsigned short nrOfCodewords;
  unsigned short currentSet;
  unsigned short noOfSets;
  SIDES          currentSide;
  HANDLE_BUFFER  hNonPcwBufPtr;
  HANDLE_BUFFER  hDecodeBuffer;
  SEGMENT_INFO   segmentInfo[LN4];
  BASE_CW_INFO   nonPcwBaseInfoTab [LN4];
} T_HCR;

/**** static functions ****/

static unsigned char GetSegmentWidth ( unsigned short     maxCWLen,
                                       const HANDLE_HCR         hHcrInfo )
{
  return ( MIN( maxCWLen, GetLenOfLongestCw ( hHcrInfo ) ) );
}

static void GetSegmentBits ( unsigned short     nrOfBits,
                             SEGMENT_INFO*      toSegmentInfo,
                             HANDLE_BUFFER      fromBuffer,
                             HANDLE_RESILIENCE  hResilience,
                             HANDLE_ESC_INSTANCE_DATA     hEscInstanceData )
{
  unsigned short transferBitsWord0;
  unsigned short transferBitsWord1;
  unsigned short transferBitsWord2;

  if ( nrOfBits > 3 * UINT_BITLENGTH ) {
    CommonExit (1, "nrOfBits too large: %d > %d (GetSegmentBits)",nrOfBits, 3 * UINT_BITLENGTH );
  }
  transferBitsWord2 = MAX ( nrOfBits - 2 * UINT_BITLENGTH, 0              );
  transferBitsWord1 = MAX ( nrOfBits -     UINT_BITLENGTH, 0              );
  transferBitsWord1 = MIN ( transferBitsWord1            , UINT_BITLENGTH );
  transferBitsWord0 = MIN ( nrOfBits                     , UINT_BITLENGTH );
  toSegmentInfo->word2 = GetBits ( transferBitsWord2, 
                                   MAX_ELEMENTS, /* does not read from input buffer */
                                   hResilience,
                                   hEscInstanceData,
                                   fromBuffer );
  toSegmentInfo->word1 = GetBits ( transferBitsWord1, 
                                   MAX_ELEMENTS, /* does not read from input buffer */
                                   hResilience,
                                   hEscInstanceData,
                                   fromBuffer );
  toSegmentInfo->word0 = GetBits ( transferBitsWord0, 
                                   MAX_ELEMENTS, /* does not read from input buffer */
                                   hResilience, 
                                   hEscInstanceData,
                                   fromBuffer );
}

static void PutSegmentBits ( unsigned short nrOfBits,
                             HANDLE_BUFFER  toBuffer,
                             SEGMENT_INFO*  fromSegmentInfo )
{
  unsigned short transferBitsWord0;
  unsigned short transferBitsWord1;
  unsigned short transferBitsWord2;
  
  if ( nrOfBits > 3 * UINT_BITLENGTH ) {
    CommonExit (1, "nrOfBits too large: %d > %d (PutSegmentBits)", nrOfBits, 3 * UINT_BITLENGTH );
  }
  transferBitsWord2 = MAX ( nrOfBits - 2 * UINT_BITLENGTH, 0              );
  transferBitsWord1 = MAX ( nrOfBits -     UINT_BITLENGTH, 0              );
  transferBitsWord1 = MIN ( transferBitsWord1            , UINT_BITLENGTH );
  transferBitsWord0 = MIN ( nrOfBits                     , UINT_BITLENGTH );
  PutBits ( transferBitsWord2, 
            toBuffer, 
            fromSegmentInfo->word2 );
  PutBits ( transferBitsWord1, 
            toBuffer, 
            fromSegmentInfo->word1 );
  PutBits ( transferBitsWord0, 
            toBuffer, 
            fromSegmentInfo->word0 );
}

static void TurnAround ( HANDLE_HCR hHcrInfo ) 
{
  unsigned short currentSegment;
  unsigned short currentBit;
  for ( currentSegment = 0; currentSegment < hHcrInfo->nrOfSegments; currentSegment++ ) {
    unsigned long tmpWord0 = 0;
    unsigned long tmpWord1 = 0;
    unsigned long tmpWord2 = 0;
    for ( currentBit = 0; currentBit < hHcrInfo->segmentInfo[currentSegment].segmentRestBits; currentBit++ ) {
      tmpWord2 <<= 1;
      if ( tmpWord1 & 0x80000000 ) {
        tmpWord2 |= 0x00000001;
      }
      tmpWord1 <<= 1;
      if ( tmpWord0 & 0x80000000 ) {
        tmpWord1 |= 0x00000001;
      }
      tmpWord0 <<= 1;
      tmpWord0 |= ( hHcrInfo->segmentInfo[currentSegment].word0 & 0x00000001 );

      hHcrInfo->segmentInfo[currentSegment].word0 >>= 1;
      if ( hHcrInfo->segmentInfo[currentSegment].word1 & 0x00000001 ) {
        hHcrInfo->segmentInfo[currentSegment].word0 |= 0x80000000;
      }
      hHcrInfo->segmentInfo[currentSegment].word1 >>= 1;
      if ( hHcrInfo->segmentInfo[currentSegment].word2 & 0x00000001 ) {
        hHcrInfo->segmentInfo[currentSegment].word1 |= 0x80000000;
      }
      hHcrInfo->segmentInfo[currentSegment].word2 >>= 1;
    }
    hHcrInfo->segmentInfo[currentSegment].word0 = tmpWord0;
    hHcrInfo->segmentInfo[currentSegment].word1 = tmpWord1;
    hHcrInfo->segmentInfo[currentSegment].word2 = tmpWord2;
  }
}

static void EverythingDecodedQuestionmark ( HANDLE_CONCEALMENT hConcealment,
                                            HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                            HANDLE_HCR         hHcrInfo,
                                            HANDLE_RESILIENCE  hResilience )
{
  if ( hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].nrOfBits ) {
    ConcealmentDetectErrorCodeword ( hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step,
                                     MAX_CW_LENGTH + 1, /* codeword has not been decoded - needs to be set as wrong */
                                     hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].maxCWLen,
                                     0,
                                     hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].codebook,
                                     0,
                                     CODEWORD_NON_PCW,
                                     hResilience,
                                     hHcrInfo,
                                     hConcealment );
    if ( GetEPFlag ( hResilience ) ) {
      unsigned char line;
      if ( ! hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
        printf("longNonPcw not decoded (codeword %d, first line %d, step %d - ReadLongNonPcwsNewKernel)\n", 
               hHcrInfo->currentCodeword,
               hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].firstLine,
               hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step);
      }
#ifdef COMPLETION_CONCEALMENT_PATCH
      printf("%s lines deleted by completion_check_patch (ReadLongNonPcwsNewKernel)\n",
             hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step == 2 ? "two" : "four" );
      for ( line = 0; line < hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step; line++ ) {
        hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].qp[line] = 0;
      }
#endif /*COMPLETION_CONCEALMENT_PATCH*/
      for ( line = 0; line < hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step; line++ ) {
        hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].nrOfBits = 0;
      }
    }
    else {
      CommonExit ( 1, "IMPLEMENTATION ERROR (ReadLongNonPcwsNewKernel)");
    }
  }
}

static void ReadNonPcwsNewCore ( unsigned short*           codewordLen,
                                 unsigned short            currentCodeword,
                                 unsigned short            currentSegment,
                                 int*                      qp,
                                 Hcb*                      hcb,
                                 Huffman*                  hcw,
                                 unsigned short            step,
                                 unsigned short            codebook,
                                 unsigned short            maxCWLen,
                                 HANDLE_RESILIENCE         hResilience,
                                 HANDLE_HCR                hHcrInfo,
                                 HANDLE_ESC_INSTANCE_DATA  hEscInstanceData,
                                 HANDLE_CONCEALMENT        hConcealment )
{
  unsigned short transferBits;
  short          segmentRestBits;
  unsigned short transferCodeBits;
  unsigned short transferCodeExtBits;

  if ( hHcrInfo->segmentInfo[currentSegment].segmentRestBits && ! hHcrInfo->nonPcwBaseInfoTab[currentCodeword].alreadyDecoded ) {

    PrepareWriting     ( hHcrInfo->hDecodeBuffer );
    ResetWriteBitCnt   ( hHcrInfo->hDecodeBuffer );

    if ( hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits ) {
      transferCodeExtBits = MAX ( hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits - UINT_BITLENGTH, 0 );
      transferCodeBits    = MIN ( hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits, UINT_BITLENGTH );
      PutBits ( transferCodeExtBits, 
                hHcrInfo->hDecodeBuffer, 
                hHcrInfo->nonPcwBaseInfoTab[currentCodeword].codeExt);
      PutBits ( transferCodeBits, 
                hHcrInfo->hDecodeBuffer, 
                hHcrInfo->nonPcwBaseInfoTab[currentCodeword].code);
      PutSegmentBits ( hHcrInfo->segmentInfo[currentSegment].segmentRestBits,
                       hHcrInfo->hDecodeBuffer,
                       &hHcrInfo->segmentInfo[currentSegment] );
    }
    else {
      PutSegmentBits ( hHcrInfo->segmentInfo[currentSegment].segmentRestBits,
                       hHcrInfo->hDecodeBuffer,
                       &hHcrInfo->segmentInfo[currentSegment] );
    }

    FinishWriting   ( hHcrInfo->hDecodeBuffer );
    PrepareReading  ( hHcrInfo->hDecodeBuffer );
    ResetReadBitCnt ( hHcrInfo->hDecodeBuffer );
    StoreBufferPointer ( hHcrInfo->hDecodeBuffer );
    *codewordLen = HuffSpecKernelPure ( qp, 
                                        hcb, 
                                        hcw, 
                                        step, 
                                        hHcrInfo,
                                        hResilience, 
                                        hEscInstanceData,
                                        hConcealment,
                                        hHcrInfo->hDecodeBuffer );
    segmentRestBits = hHcrInfo->segmentInfo[currentSegment].segmentRestBits 
      - *codewordLen + hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits; 
    
    if ( segmentRestBits >= 0 ) { /* Codeword fits into segment and is successfully decoded, 
                                     segmentRestBits have to be stored */
      DecodedBitCnt ( hHcrInfo, *codewordLen );
      GetSegmentBits ( segmentRestBits,
                       &hHcrInfo->segmentInfo[hHcrInfo->currentSegment],
                       hHcrInfo->hDecodeBuffer,
                       hResilience, 
                       hEscInstanceData );
      transferBits = *codewordLen - hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits;
      hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits       = 0;
      hHcrInfo->nonPcwBaseInfoTab[currentCodeword].alreadyDecoded = 1;
      ConcealmentDetectErrorCodeword ( step,
                                       *codewordLen, 
                                       maxCWLen,
                                       hcb->lavInclEsc,
                                       codebook,
                                       qp,
                                       CODEWORD_NON_PCW,
                                       hResilience,
                                       hHcrInfo,
                                       hConcealment );
      LcwConcealmentPatch ( "ReadNonPcwsNewCore",
                            *codewordLen,
                            maxCWLen,
                            qp,
                            step,
                            hHcrInfo );
      Vcb11ConcealmentPatch ( "ReadNonPcwsNewCore",
                              codebook,
                              hcb->lavInclEsc,
                              qp,
                              step,
                              hResilience );
    }
    else {                     /* codeword does not fit into segment and has not been successfully decoded, 
                                  the begin of this codeword has to be stored 
                                  as well as additional decoding information about this codeword */
      unsigned short transferBitsTotal;
      
      RestoreBufferPointer ( hHcrInfo->hDecodeBuffer );

      transferBits        = hHcrInfo->segmentInfo[currentSegment].segmentRestBits;
      transferBitsTotal   = transferBits + hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits;
      transferCodeExtBits = MAX ( transferBitsTotal - UINT_BITLENGTH, 0              );
      transferCodeBits    = MIN ( transferBitsTotal                 , UINT_BITLENGTH );
      
      hHcrInfo->nonPcwBaseInfoTab[currentCodeword].codeExt = GetBits ( transferCodeExtBits, 
                                                                       MAX_ELEMENTS, /* does not read from input buffer */
                                                                       hResilience,
                                                                       hEscInstanceData,
                                                                       hHcrInfo->hDecodeBuffer );
      hHcrInfo->nonPcwBaseInfoTab[currentCodeword].code    = GetBits ( transferCodeBits, 
                                                                       MAX_ELEMENTS, /* does not read from input buffer */
                                                                       hResilience, 
                                                                       hEscInstanceData,
                                                                       hHcrInfo->hDecodeBuffer );
      *codewordLen = 0; /* codewordLen has been used to indicate that a longCodeword has been detected. So far, the
                           codewordLen is not valid, because the codeword is not valid. Therefore the codewordLen 
                           is set to zero */
      hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits     = transferBitsTotal;
    }

/*     printf ( "codewordLen = %2hu (ReadNonPcwsNewCore)\n", *codewordLen ); */

    hHcrInfo->segmentInfo[currentSegment].segmentRestBits -= transferBits; /* = 0 */

    hHcrInfo->specLineCnt += step;

#ifdef DEBUG_HCR_SQUARE
    printf( "segm:%3d cw:%3d transbits:%2d, remaining in cw: %s, remaining in segment: %2d\n", 
            currentSegment, 
            currentCodeword, 
            transferBits,
            hHcrInfo->nonPcwBaseInfoTab[currentCodeword].alreadyDecoded ? "0" : "x",
            hHcrInfo->segmentInfo[currentSegment].segmentRestBits);
#endif /*DEBUG_HCR_SQUARE*/
  }
}

static void ReadLongNonPcwsNewKernel (unsigned short*    codewordLen, 
                                      unsigned short     trial,
                                      HANDLE_RESILIENCE  hResilience,
                                      HANDLE_HCR         hHcrInfo,
                                      HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                      HANDLE_CONCEALMENT hConcealment ) 
{
  ConcealmentSetLine ( hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].firstLine,
                       hConcealment );
  ReadNonPcwsNewCore ( codewordLen,
                       hHcrInfo->currentCodeword,
                       hHcrInfo->currentSegment,
                       hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].qp,
                       &book[hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].codebook],
                       book[hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].codebook].hcw,
                       hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step,
                       hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].codebook,
                       hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].maxCWLen,
                       hResilience,
                       hHcrInfo,
                       hEscInstanceData,
                       hConcealment );
  if ( trial == hHcrInfo->nrOfSegments - 1 ) {
    EverythingDecodedQuestionmark ( hConcealment,
                                    hEscInstanceData,
                                    hHcrInfo,
                                    hResilience );
  }
}

static void ReadLongNonPcwsNew ( unsigned short     codewordsInSet,
                                 unsigned short*    codewordLen, 
                                 HANDLE_RESILIENCE  hResilience,
                                 HANDLE_HCR         hHcrInfo,
                                 HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                                 HANDLE_CONCEALMENT hConcealment ) 
{
  unsigned short trial; /* each set is tried several times to be stored */
  unsigned short deli1; /* delimiter for first part */
  unsigned short deli2; /* delimiter for second part */
  unsigned short tmpCodewordLen = 0;

  for ( trial = 1; trial < hHcrInfo->nrOfSegments; trial++ ) { /* first trial has been performed by ReadNonPcwsNew() */
    /*       ResetLongNonPcwBuffers ( hEscInstanceData, hHcrInfo, hResilience ); */
    hHcrInfo->currentCodeword = hHcrInfo->nrOfSegments * hHcrInfo->currentSet;
    deli1 = MIN ( hHcrInfo->nrOfSegments, trial + codewordsInSet );
    deli2 = MAX ( 0, trial + codewordsInSet - hHcrInfo->nrOfSegments );
#ifdef DEBUG_HCR_SQUARE
    printf("trial %3d of %3d: ", trial, hHcrInfo->nrOfSegments-1);
    printf("%3d --> %3d, %3d --> %3d\n", trial, deli1, 0, deli2);
#endif /*DEBUG_HCR_SQUARE*/
    for ( hHcrInfo->currentSegment = trial; hHcrInfo->currentSegment < deli1; hHcrInfo->currentSegment++, hHcrInfo->currentCodeword++ ) {
      ReadLongNonPcwsNewKernel ( &tmpCodewordLen,
                                 trial,
                                 hResilience,
                                 hHcrInfo,
                                 hEscInstanceData,
                                 hConcealment );
      *codewordLen = MAX ( *codewordLen, tmpCodewordLen );
    }
    for ( hHcrInfo->currentSegment = 0    ; hHcrInfo->currentSegment < deli2; hHcrInfo->currentSegment++, hHcrInfo->currentCodeword++ ) {
      ReadLongNonPcwsNewKernel ( &tmpCodewordLen,
                                 trial,
                                 hResilience,
                                 hHcrInfo,
                                 hEscInstanceData,
                                 hConcealment );
      *codewordLen = MAX ( *codewordLen, tmpCodewordLen );
    }
  }
}

/**** public functions ****/

void DecodedBitCnt ( HANDLE_HCR hHcrInfo, unsigned short codewordLen )
{
  hHcrInfo->decodedBitCnt += codewordLen;
  /*   printf ( "%4hu, %4hu\n", hHcrInfo->lenOfSpecData, hHcrInfo->decodedBitCnt ); */
}

unsigned short GetDecodedBitCnt (const HANDLE_HCR hHcrInfo )
{
  return ( hHcrInfo->decodedBitCnt );
}

HANDLE_HCR CreateHcrInfo ( void )
{
  HANDLE_HCR     hHcrInfo;

  hHcrInfo = (HANDLE_HCR) malloc ( sizeof(T_HCR) );
  hHcrInfo->hNonPcwBufPtr  = CreateBuffer ( AAC_MAX_INPUT_BUF_BITS ); 
  hHcrInfo->hDecodeBuffer = CreateBuffer ( 2 * MAX_CW_LENGTH ); 
  return ( hHcrInfo );
}

void DeleteHcrInfo (HANDLE_HCR hHcrInfo)
{
  if (hHcrInfo)
  {
    if (hHcrInfo->hNonPcwBufPtr) DeleteBuffer (hHcrInfo->hNonPcwBufPtr) ;
    if (hHcrInfo->hDecodeBuffer) DeleteBuffer (hHcrInfo->hDecodeBuffer) ;

    free (hHcrInfo) ;
  }
}

unsigned short GetLenOfLongestCw ( const HANDLE_HCR hHcrInfo )
{
  return ( hHcrInfo->lenOfLongestCw );
}

unsigned short GetLenOfSpecData ( const HANDLE_HCR hHcrInfo )
{
  return ( hHcrInfo->lenOfSpecData );
}

HANDLE_BUFFER GetNonPcwBufPtrHdl ( const HANDLE_HCR     hHcrInfo )
{
  return ( hHcrInfo->hNonPcwBufPtr );
}

unsigned short GetReorderStatusFlag (const HANDLE_HCR hHcrInfo )
{
  return ( hHcrInfo->statusFlag );
}

void ReadLenOfLongestCw ( HANDLE_HCR         hHcrInfo, 
                          HANDLE_RESILIENCE  hResilience,
                          HANDLE_BUFFER      hVm,
                          HANDLE_ESC_INSTANCE_DATA     hEscInstanceData )
{
  hHcrInfo->lenOfLongestCw  = (unsigned short) GetBits ( LEN_LCW, 
                                                         LENGTH_OF_LONGEST_CODEWORD, 
                                                         hResilience, 
                                                         hEscInstanceData,
                                                         hVm );
  /*   printf("lenOfLongestCodeword: %d\n",hHcrInfo->lenOfLongestCw); */
  if ( hHcrInfo->lenOfLongestCw > MAX_CW_LENGTH ) {
    if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
      CommonExit( 2, "ReadLenOfLongestCw: lenOfLongestCw too long (%u>%i)", hHcrInfo->lenOfLongestCw, MAX_CW_LENGTH );
    }
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
        printf( "ReadLenOfLongestCw: lenOfLongestCw too long (%u>%i)", hHcrInfo->lenOfLongestCw, MAX_CW_LENGTH );
        printf( "ReadLenOfLongestCw: lenOfLongestCw is clipped at %hu",MAX_CW_LENGTH );
        hHcrInfo->lenOfLongestCw = MAX_CW_LENGTH;
      }
    }
  }
}

void ReadLenOfSpecData ( HANDLE_HCR         hHcrInfo, 
                         HANDLE_RESILIENCE  hResilience,
                         HANDLE_BUFFER      hVm, 
                         HANDLE_ESC_INSTANCE_DATA     hEscInstanceData )
{
  hHcrInfo->lenOfSpecData      = (unsigned short) GetBits ( LEN_SPEC_DATA, 
                                                            LENGTH_OF_REORDERED_SPECTRAL_DATA, 
                                                            hResilience, 
                                                            hEscInstanceData,
                                                            hVm );
  if ( hHcrInfo->lenOfSpecData > AAC_MAX_INPUT_BUF_BITS ) {
    if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
      CommonExit ( 2, "ReadLenOfSpecData: lenOfSpecData too long (%u>%i)", hHcrInfo->lenOfSpecData, AAC_MAX_INPUT_BUF_BITS );
    }
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
        printf ( "ReadLenOfSpecData: lenOfSpecData too long (%u>%i)\n", hHcrInfo->lenOfSpecData, AAC_MAX_INPUT_BUF_BITS );
        printf ( "ReadLenOfSpecData: lenOfSpecData has been clipped at %hu\n", AAC_MAX_INPUT_BUF_BITS);
      }
      hHcrInfo->lenOfSpecData = AAC_MAX_INPUT_BUF_BITS;
    }
  }
}

void InitHcr ( HANDLE_BUFFER hSpecData, HANDLE_HCR hHcrInfo )
{
  unsigned short currentCodeword;
  hHcrInfo->specLineCnt         = 0;
  hHcrInfo->statusFlag          = 0;
  hHcrInfo->decodedBitCnt       = 0;
  hHcrInfo->currentSegment      = 0;
  hHcrInfo->currentCodeword     = 0;
  hHcrInfo->currentSet          = 1; /* PCWs are set zero */
  hHcrInfo->remainingBitCnt     = hHcrInfo->lenOfSpecData;
  FinishWriting    ( hSpecData );
  PrepareReading   ( hSpecData );
  ResetReadBitCnt  ( hSpecData );
  PrepareWriting   ( hHcrInfo->hNonPcwBufPtr );
  ResetWriteBitCnt ( hHcrInfo->hNonPcwBufPtr );
  for ( currentCodeword = 0; currentCodeword < LN4; currentCodeword++ ) {
    hHcrInfo->nonPcwBaseInfoTab[currentCodeword].nrOfBits       = 0;
    hHcrInfo->nonPcwBaseInfoTab[currentCodeword].alreadyDecoded = 0;    
  }
}

void SetNrOfCodewords ( unsigned short    nrOfCodewords,
                        const HANDLE_HCR  hHcrInfo )
{
  hHcrInfo->nrOfCodewords = nrOfCodewords;
}

unsigned char GetCurrentMaxCodewordLen ( unsigned short     maxCWLen,
                                         const HANDLE_HCR   hHcrInfo )
{
  return ( MIN ( maxCWLen, GetLenOfLongestCw ( hHcrInfo ) ) );
}

void ReorderSpecDecPCWFinishedCheck ( unsigned short     maxCWLen,
                                      HANDLE_BUFFER      hSpecData,
                                      HANDLE_RESILIENCE  hResilience,
                                      HANDLE_HCR         hHcrInfo, 
                                      HANDLE_ESC_INSTANCE_DATA     hEscInstanceData )
{
  if ( GetSegmentWidth ( maxCWLen,
                         hHcrInfo ) > hHcrInfo->remainingBitCnt ) { /* end of buffer reached ? */
    hHcrInfo->statusFlag = 1;
    hHcrInfo->nrOfSegments = hHcrInfo->currentSegment;
    if ( GetConsecutiveReorderSpecFlag ( hResilience ) ) {
      if ( hHcrInfo->currentSegment > 0 ) { /* this might happen, if lengthOfSpectralData is distorted */
        unsigned short transferBits;
        PrepareWriting     ( hHcrInfo->hDecodeBuffer );
        ResetWriteBitCnt   ( hHcrInfo->hDecodeBuffer );
        PutSegmentBits ( hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1].segmentRestBits,
                         hHcrInfo->hDecodeBuffer,
                         &hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1] );
        TransferBits ( hHcrInfo->remainingBitCnt,
                       MAX_ELEMENTS,
                       hSpecData, 
                       hHcrInfo->hDecodeBuffer, 
                       hEscInstanceData,
                       hResilience );
        FinishWriting   ( hHcrInfo->hDecodeBuffer );
        PrepareReading  ( hHcrInfo->hDecodeBuffer );
        ResetReadBitCnt ( hHcrInfo->hDecodeBuffer );
        transferBits =  hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1].segmentRestBits + hHcrInfo->remainingBitCnt;
        if ( transferBits > 3 * UINT_BITLENGTH ) {
          CommonExit (1, "nrOfBits too large: %d > %d (ReorderSpecDecPCWFinishedCheck)", transferBits, 3 * UINT_BITLENGTH );
        }
        GetSegmentBits ( transferBits,
                         &hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1],
                         hHcrInfo->hDecodeBuffer,
                         hResilience, 
                         hEscInstanceData );
        hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1].segmentRestBits += hHcrInfo->remainingBitCnt;
#ifdef DEBUG_HCR_SQUARE
        printf ( ", update: %3d: %2d\n",hHcrInfo->currentSegment-1, hHcrInfo->segmentInfo[hHcrInfo->currentSegment-1].segmentRestBits);
        printf ( "number of codewords : %3d, number of segments : %3d\n", hHcrInfo->nrOfCodewords,hHcrInfo->nrOfSegments);
#endif /*DEBUG_HCR_SQUARE*/
        hHcrInfo->noOfSets = ( unsigned short ) ( (float) ( hHcrInfo->nrOfCodewords + hHcrInfo->nrOfSegments - 1 ) / hHcrInfo->nrOfSegments ); 
      }
    }
    else {
      TransferBits ( hHcrInfo->remainingBitCnt,
                     MAX_ELEMENTS,
                     hSpecData, 
                     hHcrInfo->hNonPcwBufPtr, 
                     hEscInstanceData,
                     hResilience );
      FinishWriting   ( hHcrInfo->hNonPcwBufPtr );
      PrepareReading  ( hHcrInfo->hNonPcwBufPtr );
      ResetReadBitCnt ( hHcrInfo->hNonPcwBufPtr );
    }
    hHcrInfo->currentSegment = 0;
  }
}

void ReadNonPcwsNew ( unsigned short*    codewordLen, 
                      int*               qp, 
                      Hcb*               hcb, 
                      Huffman*           hcw, 
                      unsigned short     step,
                      unsigned short     codebook,
                      unsigned short*    codewordsInSet,
                      HANDLE_RESILIENCE  hResilience,
                      HANDLE_HCR         hHcrInfo,
                      HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                      HANDLE_CONCEALMENT hConcealment ) 
{
#ifdef DEBUG_HCR_SQUARE
  unsigned short deli1; /* delimiter for first part */
  unsigned short deli2; /* delimiter for second part */
#endif /*DEBUG_HCR_SQUARE*/

  if ( hHcrInfo->currentSegment == 0 ) {
    TurnAround (hHcrInfo);
    *codewordsInSet = MIN ( hHcrInfo->nrOfSegments, hHcrInfo->nrOfCodewords - ( hHcrInfo->nrOfSegments * hHcrInfo->currentSet ) );
#ifdef DEBUG_HCR_SQUARE
    deli1 = MIN ( hHcrInfo->nrOfSegments, *codewordsInSet );
    deli2 = MAX ( 0, *codewordsInSet - hHcrInfo->nrOfSegments );
    printf("set %d of %d (%3d codewords)\n",hHcrInfo->currentSet, hHcrInfo->noOfSets - 1, *codewordsInSet );
    printf("trial %3d of %3d: ", 0, hHcrInfo->nrOfSegments - 1 );
    printf("%3d --> %3d, %3d --> %3d (***)\n", 0, deli1, 0, deli2 );
#endif /*DEBUG_HCR_SQUARE*/
  }
  ReadNonPcwsNewCore ( codewordLen,
                       hHcrInfo->currentCodeword,
                       hHcrInfo->currentSegment,
                       qp,
                       hcb,
                       hcw,
                       step,
                       codebook,
                       hcb->maxCWLen,
                       hResilience,
                       hHcrInfo,
                       hEscInstanceData,
                       hConcealment );
  hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].qp           = qp;
  hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].codebook     = codebook;
  hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].step         = step;
  hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].maxCWLen     = hcb->maxCWLen;
  hHcrInfo->nonPcwBaseInfoTab[hHcrInfo->currentCodeword].firstLine    = ConcealmentGetLine ( hConcealment );
  if ( hHcrInfo->nrOfSegments - 1 == 0 ) { /* special case, only one segment */
    EverythingDecodedQuestionmark ( hConcealment,
                                    hEscInstanceData,
                                    hHcrInfo,
                                    hResilience );
  }
  hHcrInfo->currentSegment++;
  hHcrInfo->currentCodeword++;
  if ( hHcrInfo->currentSegment == *codewordsInSet ) {
    unsigned short base;
    ReadLongNonPcwsNew ( *codewordsInSet,
                         codewordLen, 
                         hResilience,
                         hHcrInfo,
                         hEscInstanceData,
                         hConcealment );
    for ( base = 0; base < LN4; base++ ) {
      if ( hHcrInfo->nonPcwBaseInfoTab[base].nrOfBits ) { /* all bits have to be processed at this stage */
        CommonExit ( 1, "IMPLEMENTATION ERROR (ReadNonPcwsNew)");
      }
    }
    hHcrInfo->currentSegment = 0;
    hHcrInfo->currentSet++;
  }
}

void ReadNonPcws ( unsigned short*          codewordLen, 
                   int*                     qp, 
                   Hcb*                     hcb, 
                   Huffman*                 hcw, 
                   unsigned short           step,
                   unsigned short           codebook,
                   HANDLE_RESILIENCE        hResilience,
                   HANDLE_HCR               hHcrInfo,
                   HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                   HANDLE_CONCEALMENT       hConcealment ) 
{
  *codewordLen = HuffSpecKernelPure ( qp, 
                                      hcb, 
                                      hcw, 
                                      step, 
                                      hHcrInfo,
                                      hResilience, 
                                      hEscInstanceData,
                                      hConcealment,
                                      GetNonPcwBufPtrHdl ( hHcrInfo ) );
  /*   printf ( "codewordLen = %2hu (ReadNonPcws)\n", *codewordLen ); */
  DecodedBitCnt ( hHcrInfo, *codewordLen );
  ConcealmentDetectErrorCodeword ( step,
                                   *codewordLen, 
                                   hcb->maxCWLen,
                                   hcb->lavInclEsc,
                                   codebook,
                                   qp,
                                   CODEWORD_NON_PCW,
                                   hResilience,
                                   hHcrInfo,
                                   hConcealment );
  LcwConcealmentPatch ( "ReadNonPcws",
                        *codewordLen,
                        hcb->maxCWLen,
                        qp,
                        step,
                        hHcrInfo );
  Vcb11ConcealmentPatch ( "ReadNonPcws",
                          codebook,
                          hcb->lavInclEsc,
                          qp,
                          step,
                          hResilience );
}

void ReadPcws ( unsigned short*          codewordLen, 
                int*                     qp,
                Hcb*                     hcb, 
                Huffman*                 hcw, 
                unsigned short           codebook, 
                unsigned short           maxCWLen, 
                unsigned short           step, 
                HANDLE_RESILIENCE        hResilience,
                HANDLE_BUFFER            hSpecData,
                HANDLE_HCR               hHcrInfo, 
                HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                HANDLE_CONCEALMENT       hConcealment )
{
  char          segmentRestBits;
  unsigned char segmentWidth;
  unsigned long transferCodeExtBits;
  unsigned long transferCodeBits;
  
  /* decodes codeword */
  *codewordLen = HuffSpecKernelPure ( qp, 
                                      hcb, 
                                      hcw, 
                                      step, 
                                      hHcrInfo,
                                      hResilience,
                                      hEscInstanceData,
                                      hConcealment,
                                      hSpecData );
/*   printf ( "codewordLen = %2hu (ReadPcws)\n", *codewordLen ); */
  DecodedBitCnt ( hHcrInfo, *codewordLen );
  /* checks, whether codeword was too long (due to errors) */
  ConcealmentDetectErrorCodeword ( step,
                                   *codewordLen, 
                                   hcb->maxCWLen,
                                   hcb->lavInclEsc,
                                   codebook,
                                   qp,
                                   CODEWORD_PCW,
                                   hResilience,
                                   hHcrInfo,
                                   hConcealment );
  LcwConcealmentPatch ( "ReadPcws",
                        *codewordLen,
                        hcb->maxCWLen,
                        qp,
                        step,
                        hHcrInfo );
  Vcb11ConcealmentPatch ( "ReadPcws",
                          codebook,
                          hcb->lavInclEsc,
                          qp,
                          step,
                          hResilience );

  segmentWidth               = GetSegmentWidth ( maxCWLen,
                                                 hHcrInfo );
  hHcrInfo->remainingBitCnt -= segmentWidth;
  segmentRestBits            = segmentWidth - *codewordLen;
  if ( segmentRestBits >= 0 ) { /* Codeword fits into segment and is sucessfully decoded, 
                                   segmentRestBits have to be stored */
    hHcrInfo->segmentInfo[hHcrInfo->currentSegment].segmentRestBits = segmentRestBits;
    if ( GetConsecutiveReorderSpecFlag ( hResilience ) ) {
      GetSegmentBits ( segmentRestBits,
                       &hHcrInfo->segmentInfo[hHcrInfo->currentSegment],
                       hSpecData,
                       hResilience, 
                       hEscInstanceData );
    }
    else {
      TransferBits ( segmentRestBits,
                     MAX_ELEMENTS,
                     hSpecData, 
                     hHcrInfo->hNonPcwBufPtr, 
                     hEscInstanceData,
                     hResilience );
    }
  }
  else {                     /* codeword does not fit into segment and has not been successfully decoded  */

    RestoreBufferPointer ( hSpecData );

    /* dummy reads to adjust the pointer */
    transferCodeExtBits = MAX ( segmentWidth - UINT_BITLENGTH, 0 );
    transferCodeBits    = MIN ( segmentWidth, UINT_BITLENGTH );
    GetBits ( transferCodeExtBits, 
              MAX_ELEMENTS, /* does not read from input buffer */
              hResilience, 
              hEscInstanceData,
              hSpecData );
    GetBits ( transferCodeBits, 
              MAX_ELEMENTS, /* does not read from input buffer */
              hResilience, 
              hEscInstanceData,
              hSpecData );
    hHcrInfo->segmentInfo[hHcrInfo->currentSegment].segmentRestBits = 0;
  }
#ifdef DEBUG_HCR_SQUARE
  printf ( "\n%3d: %2d",hHcrInfo->currentSegment, hHcrInfo->segmentInfo[hHcrInfo->currentSegment].segmentRestBits);
#endif /*DEBUG_HCR_SQUARE*/
  hHcrInfo->currentSegment++;
  hHcrInfo->currentCodeword++;
  hHcrInfo->specLineCnt += step;
}

void CheckDecodingProgress ( HANDLE_BUFFER      hSpecData,
                             HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                             HANDLE_HCR         hHcrInfo,
                             HANDLE_RESILIENCE  hResilience )
{
  if ( ! GetReorderStatusFlag ( hHcrInfo ) ) {
    TransferBits ( hHcrInfo->remainingBitCnt,
                   MAX_ELEMENTS,
                   hSpecData, 
                   hHcrInfo->hNonPcwBufPtr, 
                   hEscInstanceData,
                   hResilience );
    FinishWriting   ( hHcrInfo->hNonPcwBufPtr );
    PrepareReading  ( hHcrInfo->hNonPcwBufPtr );
    ResetReadBitCnt ( hHcrInfo->hNonPcwBufPtr );
  }
}

void Vcb11ConcealmentPatch ( char*             calledFrom,
                             unsigned short    codebook,
                             int               lavInclEsc,
                             int*              qp,
                             unsigned short    step,
                             HANDLE_RESILIENCE hResilience )
{
#ifdef  VCB11_CONCEALMENT_PATCH
  if ( ( GetVcb11Flag ( hResilience ) ) && ( Vcb11Used ( codebook ) ) ) {
    if(abs ( qp[0] ) > lavInclEsc || abs( qp[1] ) > lavInclEsc ) { 
      unsigned char i;
      printf("two lines deleted by vcb11_concealment_patch:%4d >=%4d, line %i (%s)\n", 
             abs(qp[0]) > abs(qp[1]) ? abs(qp[0]) : abs(qp[1]), 
             abs(qp[0]) > abs(qp[1]) ?        0   :        1, 
             lavInclEsc,
             calledFrom );
      for ( i = 0; i < step; i++ ){
        qp[i] = 0;
      }
    }
  }
#endif /*VCB11_CONCEALMENT_PATCH*/
}

void LcwConcealmentPatch ( char*          calledFrom,
                           unsigned short codewordLen,
                           unsigned short maxCWLen,
                           int*           qp,
                           unsigned short step,
                           HANDLE_HCR     hHcrInfo )
{
#ifdef LCW_CONCEALMENT_PATCH
  if ( codewordLen > GetCurrentMaxCodewordLen (  maxCWLen, hHcrInfo ) ) {
    unsigned char i;
     printf("two lines deleted by lcw_concealment_patch  : %3d >= %3d          (%s)\n",
           codewordLen, 
           GetCurrentMaxCodewordLen (  maxCWLen, hHcrInfo ),
           calledFrom );
    for ( i = 0; i < step; i++ ){
      qp[i] = 0;
    }
  }
#endif /*LCW_CONCEALMENT_PATCH*/
}
