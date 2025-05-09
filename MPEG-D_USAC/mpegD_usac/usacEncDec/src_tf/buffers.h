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
#ifndef _buffers_h_
#define _buffers_h_

#include "allHandles.h"
#include "buffer.h"

HANDLE_BUFFER  CreateBuffer ( unsigned long bufferSize );
void DeleteBuffer (HANDLE_BUFFER handle) ;


void           FinishWriting ( HANDLE_BUFFER handle );


unsigned long  GetReadBitCnt( HANDLE_BUFFER handle );

unsigned long  GetWriteBitCnt( HANDLE_BUFFER handle );

long           GetBits ( int               n,
                         CODE_TABLE        code,
                         HANDLE_RESILIENCE hResilience,
                         HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                         HANDLE_BUFFER     hVm );

long           GetBitsAhead ( int               n,
                              CODE_TABLE        code,
                              HANDLE_RESILIENCE hResilience,
                              HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                              HANDLE_BUFFER     hVm );

HANDLE_BSBITBUFFER GetRemainingBufferBits ( HANDLE_BUFFER handle );

void SkipBits ( HANDLE_BUFFER handle, long numBits);



void           PrepareReading ( HANDLE_BUFFER handle );

void           PrepareWriting ( HANDLE_BUFFER handle );

void PutBits ( int               n,
               HANDLE_BUFFER     handle,
               unsigned long     value );


void           ResetReadBitCnt ( HANDLE_BUFFER handle );

void           ResetWriteBitCnt ( HANDLE_BUFFER handle );

void           RestoreBufferPointer ( HANDLE_BUFFER handle );

void           StoreBufferPointer ( HANDLE_BUFFER handle );

void           StoreDataInBuffer ( HANDLE_BUFFER  fromHandle,
                                   HANDLE_BUFFER  toHandle,
                                   unsigned short nrOfBits );


void TransferBits ( unsigned long     nrOfBits,
                    CODE_TABLE        code,
                    HANDLE_BUFFER     fromHandle,
                    HANDLE_BUFFER     toHandle,
                    HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                    HANDLE_RESILIENCE hResilience );


void           setHuffdec2BitBuffer ( HANDLE_BSBITSTREAM  fixed_stream );

void            byte_align ( void ); /* make it void */
void            byte_align_on_bs (HANDLE_BSBITSTREAM bs);

#endif /* _buffers_h_  */
