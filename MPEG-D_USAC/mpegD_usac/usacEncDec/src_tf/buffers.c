/*****************************************************************************
 *                                                                           *
"This software module was originally developed by

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7,
14496-1,2 and 3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio
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
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "allHandles.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#include "buffers.h"
#include "bitfct.h"
#include "bitstream.h"
#include "common_m4a.h"
#include "resilience.h"

/*******************/
/*** definitions ***/
/*******************/

typedef struct tag_buffer
{
  unsigned char* physBuf;
  unsigned long  readBitCnt;
  unsigned long  readBitCntMemo;
  unsigned long  writeBitCnt;
  unsigned long  writeBitCntMemo;
  BIT_BUF        bitBuf;
  BIT_BUF        bitBufMemo;
} T_BUFFER;


/*****************/
/*** functions ***/
/*****************/

/****************************/
/**** framework handling ****/
static HANDLE_BSBITSTREAM vmBitStream;

void setHuffdec2BitBuffer ( HANDLE_BSBITSTREAM  fixed_stream )
{
  vmBitStream = fixed_stream;
}

/* align byte if necessary */
void byte_align ( void )
{
  int i;
  int curBitPos;

  curBitPos = BsCurrentBit ( vmBitStream );
  i = curBitPos % 8;
  if (i) {
    i = 8-i;
    BsGetSkip( vmBitStream, i);
  }
}

void byte_align_on_bs (HANDLE_BSBITSTREAM bs)
{
  int i;
  int curBitPos;

  curBitPos = BsCurrentBit ( bs );
  i = curBitPos % 8;
  if (i) {
    i = 8-i;
    BsGetSkip( bs, i);
  }
}


HANDLE_BUFFER CreateBuffer ( unsigned long bufferSize )
{
  HANDLE_BUFFER handle;

  handle = (HANDLE_BUFFER) malloc ( sizeof(T_BUFFER) );
  if ( bufferSize )
    handle->physBuf = (unsigned char*) malloc ( sizeof ( char ) * bufferSize );
  else
    handle->physBuf = NULL;
  return ( handle );
}

void DeleteBuffer (HANDLE_BUFFER handle)
{
  if (handle)
  {
    if (handle->physBuf) {
      free (handle->physBuf) ;
    }
    free (handle) ;
  }
}

void FinishWriting ( HANDLE_BUFFER handle )
{
  FlushWriteBitBuf ( &(handle->bitBuf) );
}

long GetBits ( int               n,
               CODE_TABLE        code,
               HANDLE_RESILIENCE hResilience,
               HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
               HANDLE_BUFFER     handle )
{
  unsigned long  value = 0;

  if ( n != 0 ) {
    if ( handle->physBuf == NULL ) {
      if( GetEPFlag ( hResilience ) ) {
        if ( code == MAX_ELEMENTS ) {
          if ( BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
            printf ( "GetBits: an unsupported data element occured (probably due to a bitsteam error)\n" );
          }
        }
        else {
          handle->readBitCnt += BsGetBitEP (code, hEscInstanceData, vmBitStream, &value, n );
          if ( GetReadBitCnt ( handle ) != (unsigned long) ( BsCurrentBit ( vmBitStream ) ) ) {
            CommonExit(1,"IMPLEMENTATION ERROR (buffers.c)\n");
          }
        }
      } else {
	  handle->readBitCnt += n;
          BsGetBit ( vmBitStream, &value, n );
      }
      if (debug['R'])
        fprintf(stderr, "  AAC: GetBits: val=%5ld      num=%5d\n", value, n );
    }
    else {
      if ( code != MAX_ELEMENTS ) {
        CommonWarning ("GetBits: element not needed");
      }
      handle->readBitCnt += n;
      value = ReadBitBuf ( &(handle->bitBuf), n );
    }
    /*printf("bit count: %d - retval: %ld\n.",n,value);*/
  }

  return ( value );
}



unsigned long GetReadBitCnt ( HANDLE_BUFFER handle )
{
  return ( handle->readBitCnt );
}

unsigned long GetWriteBitCnt ( HANDLE_BUFFER handle )
{
  return ( handle->writeBitCnt );
}

void PrepareReading ( HANDLE_BUFFER handle )
{
  InitReadBitBuf ( &(handle->bitBuf), handle->physBuf );
}

void PrepareWriting ( HANDLE_BUFFER handle )
{
  InitWriteBitBuf ( &(handle->bitBuf), handle->physBuf );
}

void PutBits ( int               n,
               HANDLE_BUFFER     handle,
               unsigned long     value )
{
  if ( n != 0 ) {
    handle->writeBitCnt += n;
    if ( handle->physBuf == NULL ) {
      CommonExit( 1, "buffers.c: PutBits()" );
    }
    else {
      WriteBitBuf ( &(handle->bitBuf), value, n );
    }
  }
}

void ResetReadBitCnt ( HANDLE_BUFFER handle )
{
  if ( handle != NULL )
    handle->readBitCnt = 0;
}

void ResetWriteBitCnt ( HANDLE_BUFFER handle )
{
  handle->writeBitCnt = 0;
}

void RestoreBufferPointer ( HANDLE_BUFFER handle )
{
  handle->readBitCnt  = handle->readBitCntMemo;
  handle->writeBitCnt = handle->writeBitCntMemo;
  handle->bitBuf      = handle->bitBufMemo;
}

HANDLE_BSBITBUFFER GetRemainingBufferBits ( HANDLE_BUFFER handle ) { /* does not advance underlying bitstream! */

  if ( handle->physBuf != NULL ) {

    return 0;
  }
  else {
    long tot_nb = BsBufferNumBit (BsGetBitBufferHandle(0,vmBitStream));
    unsigned long cb = BsCurrentBit (vmBitStream);
    if (cb != handle->readBitCnt) {
      return 0;
    }
    else {
      HANDLE_BSBITBUFFER newBuf = BsAllocBuffer(tot_nb-cb);
      BsGetBufferAhead(vmBitStream,newBuf,tot_nb-cb);
      return newBuf;
    }
  }
}

void SkipBits ( HANDLE_BUFFER handle, long numBits) {
  if ( handle->physBuf != NULL ) {
    return;
  }
  else {
    handle->readBitCnt += numBits;
    BsGetSkip(vmBitStream, numBits);
  }
  return;
}

void StoreBufferPointer ( HANDLE_BUFFER handle )
{
  handle->readBitCntMemo  = handle->readBitCnt;
  handle->writeBitCntMemo = handle->writeBitCnt;
  handle->bitBufMemo      = handle->bitBuf;
}


void TransferBits ( unsigned long     nrOfBits,
                    CODE_TABLE        code,
                    HANDLE_BUFFER     fromHandle,
                    HANDLE_BUFFER     toHandle,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_RESILIENCE hResilience )
{
  if ( toHandle->physBuf == NULL )
    CommonExit( 1, "buffers.c: Transferbits()" );
  if ( fromHandle->physBuf == NULL )
    {
      unsigned long  lengthOfLong;
      unsigned long  readBitCnt;

      lengthOfLong = sizeof ( long ) * 8;
      for ( readBitCnt = 0; ( readBitCnt + lengthOfLong ) <= nrOfBits; readBitCnt += lengthOfLong ) {
        WriteBitBuf ( &(toHandle->bitBuf),
                      GetBits ( lengthOfLong,
                                code,
                                hResilience,
                                hEscInstanceData,
                                fromHandle ),
                      lengthOfLong );
        toHandle->writeBitCnt += lengthOfLong; /* readBitCnt is set by GetBits */
      }
      WriteBitBuf ( &(toHandle->bitBuf),
                    GetBits ( nrOfBits%lengthOfLong,
                              code,
                              hResilience,
                              hEscInstanceData,
                              fromHandle ),
                    nrOfBits%lengthOfLong );
      toHandle->writeBitCnt += nrOfBits%lengthOfLong; /* readBitCnt is set by GetBits */
    }
  else {
    /* Error Check */
    if ( code != MAX_ELEMENTS ) {
      CommonWarning ("GetBits: element not needed");
    }
    if (fromHandle->readBitCnt > AAC_MAX_INPUT_BUF_BITS) {
      CommonWarning( "ERROR: TransferBits: Bitbuffer overflow.  (buffers.c)\n");
      CommonWarning( "ERROR: fromHandle->readBitCnt(%li) > AAC_MAX_INPUT_BUF_BITS(%i)\n",
                     fromHandle->readBitCnt,
                     AAC_MAX_INPUT_BUF_BITS);
      CommonExit(1, "IMPLEMENTATION ERROR.");
    }
    if (toHandle->writeBitCnt > AAC_MAX_INPUT_BUF_BITS) {
      CommonWarning( "ERROR: TransferBits: Bitbuffer overflow.  (buffers.c)\n");
      CommonWarning( "ERROR: toHandle->writeBitCnt(%li) > AAC_MAX_INPUT_BUF_BITS(%i)\n",
                     toHandle->writeBitCnt,
                     AAC_MAX_INPUT_BUF_BITS);
      CommonExit(1, "IMPLEMENTATION ERROR.");
    }

    TransferBitsBetweenBitBuf ( &(fromHandle->bitBuf),
                                &(toHandle->bitBuf),
                                nrOfBits );
    fromHandle->readBitCnt += nrOfBits;
    toHandle->writeBitCnt += nrOfBits;
  }
}



