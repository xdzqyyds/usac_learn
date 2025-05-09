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

Source file: Bits.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001	JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : Bits
 *
 *   Description         : This module provides bit access functions
 *
 *   Tools               : Microsoft Visual C++
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : SSC_BITS
 *
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <assert.h>

#include "SSC_System.h"
#include "SSC_Error.h"

#include "Bits.h"
#include "Log.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define MASK_BITS(x) ( ((x)==32) ? (0xffffffff) : ((((UInt)1)<<(x))-1) )

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/


/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static SSC_ERROR LOCAL_CheckOffset(const SSC_BITS* pBits,
                                   UInt            Offset);

static UInt      LOCAL_Read(SSC_BITS* pBits,
                            UInt      Length);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_Init
 *
 * Description : Initialise a SSC_BITS structure. 
 *
 * Parameters  : pBits   - Pointer to SSC_BITS structure.
 *
 *               pBuffer - Pointer to buffer containing the bits to be
 *                         accessed.
 *
 *               Length  - Length of the buffer expressed in bits.
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_BITS_Init(SSC_BITS*       pBits,
                   const UByte*    pBuffer,
                   UInt            Length,
                   
                   struct exception_context *the_exception_context)
{
    pBits->pStart     = pBuffer;
    pBits->pCurrent   = pBits->pStart;
    pBits->MaxOffset  = Length;
    pBits->BitBuffer  = *pBits->pCurrent;
    pBits->pCurrent++;
    pBits->BitsLoaded = 8;
    pBits->Offset     = 0;

    pBits->the_exception_context = the_exception_context;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_Seek
 *
 * Description : Seek to a specified bit-offset.
 *
 * Parameters  : pBits - Pointer to SSC_BITS structure.
 *
 *               Offset - The bit-offset relative to the start of the buffer
 *                        to go to.
 *
 * Returns:    : -
 * 
 *****************************************************************************/
void SSC_BITS_Seek(SSC_BITS* pBits,
                   UInt      Offset)
{
    UInt BytePos;
    UInt BitPos;

    LOCAL_CheckOffset(pBits, Offset);

    BytePos = Offset >> 3;
    BitPos  = Offset & 7;

    pBits->pCurrent   =  pBits->pStart + BytePos;
    pBits->BitBuffer  = *pBits->pCurrent;
    pBits->pCurrent++;
    pBits->BitsLoaded = 8 - BitPos;
    pBits->Offset     = Offset;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_GetOffset
 *
 * Description : Get the current bit-offset.
 *
 * Parameters  : pBits  - Pointer to SSC_BITS stucture.
 *
 * Returns:    : The current bit-offset relative to start of buffer.
 * 
 *****************************************************************************/
UInt SSC_BITS_GetOffset(SSC_BITS* pBits)
{
    return pBits->Offset;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_Read
 *
 * Description : Read a unsigned bit-field from the buffer.
 *               This function throws an exception if an attempt is made to
 *               read past the end of the buffer
 *
 * Parameters  : pBits  - Pointer to SSC_BITS structure.
 *
 *               Length - Number of bits to read.
 *
 * Returns:    : 
 * 
 *****************************************************************************/
UInt SSC_BITS_Read(SSC_BITS* pBits,
                   UInt      Length)
{
    UInt Value;

    Value = LOCAL_Read(pBits, Length);
    #ifdef LOG_BITS
    {
        LOG_Printf("Value: %u\n", Value);
    }
    #endif
    #ifdef LOG_PARMS
        LOGPARMS_Printf("Value: %u\n", Value);
    #endif
    return Value;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_ReadSigned
 *
 * Description : Read a signed bit-field from the buffer. The value read
 *               is sign-extended to the full width of the return type.
 *
 * Parameters  : pBits - Pointer to SSC_BITS structure.
 *
 *               Length - Number of bits to read.
 *
 * Returns:    : 
 * 
 *****************************************************************************/
SInt SSC_BITS_ReadSigned(SSC_BITS* pBits,
                         UInt      Length)
{
    UInt v;
    SInt s;
    SInt Value;

    /* Read bits without sign extension */
    v = LOCAL_Read(pBits, Length);

    /* Sign-extend to full integer */
    s     = (1 << (Length-1));
    Value = (v ^ s) - s;
    #ifdef LOG_BITS
    {
        LOG_Printf("Value: %d\n", Value);
    }
    #endif
    #ifdef LOG_PARMS
        LOGPARMS_Printf("Value: %d\n", Value);
    #endif
    return Value;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : SSC_BITS_ReadHuffman
 *
 * Description : Read a huffman coded symbol from the buffer. 
 *
 * Parameters  : pBits    - Pointer to SSC_BITS structure.
 *
 *               pDecoder - Pointer to huffamn decoder table.
 *
 * Returns:    : 
 * 
 *****************************************************************************/
SInt SSC_BITS_ReadHuffman(SSC_BITS* pBits, const HUFFMAN_DECODER* pDecoder)
{
    PHUFFMAN_LUT pTable;      /* Pointer to current look-up table.              */
    UInt         Index, IndexBits, Offset;

    IndexBits  = pDecoder->IndexBits;
    pTable     = pDecoder->pTable;
    Offset     = SSC_BITS_GetOffset(pBits);

    Index = LOCAL_Read(pBits, IndexBits);
 
    /* While symbol cannot be decoded by the current table go to the next level table */
    while(pTable[Index].Length < 0)
    {
        /* Go to next level table */
        IndexBits   = -(pTable[Index].Length);
        pTable      = pTable[Index].v.pTable;

        /* Read index */
        Index = LOCAL_Read(pBits, IndexBits);
    } 

    /* Advance offset to beginning of next code */
    SSC_BITS_Seek(pBits, pTable[Index].Length + Offset);

    /* Check if huffman code is valid */
#ifndef BITS_CHECK_DISABLE
    if(pTable[Index].Length == 0)
    {
        struct exception_context *the_exception_context = pBits->the_exception_context;

        if(the_exception_context != NULL)
        {
            Throw SSC_ERROR_VIOLATION | SSC_ERROR_ILLEGAL_HUFFMAN_CODE;
        }
    }
#endif 
    #ifdef LOG_BITS
    {
        LOG_Printf("Value: %d\n", pTable[Index].v.Value);
    }
    #endif
    return pTable[Index].v.Value;
}

/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_CheckOffset
 *
 * Description : This function checks if an offset is still within range.
 *               If not it will throw an exception. If no exception handler
 *               is provided the return value will be used instead to indicate
 *               an error.
 *
 * Parameters  : pBits - Pointer to SSC_BITS structure.
 * 
 *               Offset - The bit-offset relative to the start of the buffer
 *                        to check.
 * Returns:    : 
 * 
 *****************************************************************************/
static SSC_ERROR LOCAL_CheckOffset(const SSC_BITS* pBits,
                                   UInt            Offset)
{
    SSC_ERROR ErrorCode = SSC_ERROR_OK;

    if(Offset > pBits->MaxOffset)
    {
        struct exception_context *the_exception_context = pBits->the_exception_context;

        if(the_exception_context != NULL)
        {
            Throw SSC_ERROR_VIOLATION | SSC_ERROR_FRAME_TOO_LONG;
        }
        else
        {
            ErrorCode = SSC_ERROR_VIOLATION | SSC_ERROR_FRAME_TOO_LONG;
        }
    }

    return ErrorCode;
}


/*************************STATIC FUNCTION**************************************
 * 
 * Name        : LOCAL_Read
 *
 * Description : This function checks if an offset is still within range.
 *               If not it will throw an exception. If no exception handler
 *               is provided the return value will be used instead to indicate
 *               an error.
 *
 * Parameters  : pBits - Pointer to SSC_BITS structure.
 * 
 *               Offset - The bit-offset relative to the start of the buffer
 *                        to check.
 * Returns:    : 
 * 
 *****************************************************************************/
UInt LOCAL_Read(SSC_BITS* pBits,
                UInt      Length)
{
    UInt Value;
    /* Check if we don't run out of buffer space. */
    pBits->Offset += Length;

    assert( Length <= 32 );

#ifndef BITS_CHECK_DISABLE
    LOCAL_CheckOffset(pBits, pBits->Offset);
#endif

    /* Make sure there are sufficient bits in the bit-buffer. */
    while(pBits->BitsLoaded < (SInt)Length)
    {
        pBits->BitBuffer   = (pBits->BitBuffer << 8) | *pBits->pCurrent;
        pBits->BitsLoaded += 8;
        pBits->pCurrent++;
    }
    #ifdef LOG_BITS
    {
        if( pBits->BitsLoaded > 0 )
        {
            UInt nMask = 1<<(pBits->BitsLoaded-1) ;
            UInt  i ;

            LOG_Printf( "Bits: " );
            for( i=0; i<Length; i++ )
            {
                if( nMask & pBits->BitBuffer )
                    LOG_Printf( "1" );
                else
                    LOG_Printf( "0" );

                nMask >>= 1 ;
            }
            LOG_Printf( "\n" );
        }
    }
    #endif


    /* Extract bits from bit-buffer. */
    pBits->BitsLoaded -= Length;
    Value              = (pBits->BitBuffer) >> (pBits->BitsLoaded);
    Value             &= MASK_BITS(Length);

    return Value;
}

