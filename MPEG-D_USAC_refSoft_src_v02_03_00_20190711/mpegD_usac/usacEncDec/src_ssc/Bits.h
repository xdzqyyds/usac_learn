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

Copyright © 2001.

Source file: Bits.h

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
#ifndef  BITS_H
#define  BITS_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include "HuffmanDecoder.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef struct _SSC_BITS
{
    UInt         BitBuffer;
    SInt         BitsLoaded;
    UInt         Offset;
    UInt         MaxOffset;
    const UByte* pCurrent;
    const UByte* pStart;

    struct exception_context *the_exception_context;
} SSC_BITS;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void SSC_BITS_Init(SSC_BITS*       pBits,
                   const UByte*    pBuffer,
                   UInt            Length,
                   struct exception_context *the_exception_context);

void SSC_BITS_Seek(SSC_BITS* pBits,
                   UInt      Offset);

 
UInt SSC_BITS_GetOffset(SSC_BITS* pBits);

UInt SSC_BITS_Read(SSC_BITS* pBits,
                   UInt      Length);

SInt SSC_BITS_ReadSigned(SSC_BITS* pBits,
                         UInt      Length);

SInt SSC_BITS_ReadHuffman(SSC_BITS*              pBits,
                          const HUFFMAN_DECODER* pDecoder);

#ifdef __cplusplus
}
#endif

#endif /* BITS_H */
