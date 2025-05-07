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

Source file: HuffmanDecoder.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
07-Sep-2001	JD	Initial version
************************************************************************/

#ifndef _HUFFMANDECODER_H
#define _HUFFMANDECODER_H

#include <stdio.h>
#include "PlatformTypes.h"

/**************************************************************
* The MAX_CODEWORD_BITS defines the maximum number of bits in a
* huffman code word that this module is supposed to handle. It
* must be a multiple of 8.
**************************************************************/
#ifndef HUFFMAN_MAX_CODEWORD_BITS
#define HUFFMAN_MAX_CODEWORD_BITS 128
#endif

typedef int  SYMBOL_TYPE;

/************************************************************************************************************
*  Type    : HUFFMAN_LUT
*
*  Purpose : The HUFFMAN_LUT defines an element of the huffman look-up table for the decoder.
*
*  Fields  : Length   - > 0  : huffman code decoded, the symbol value can be found in the v.Value member. 
*                              This field contains the number of bits in the decoded huffman code.
*                       < 0  : huffman code is too long to be decoded by this table. The v.pTable member
*                              contains the pointer to the next level table to use to continue decoding.
*                              This field contains the negated number of bits to use as index for the
*                              next level table.
*                       == 0 : Invalid code / Unused table entry.
*
*            v.Value  - Value associated with the decoded code word. (Length > 0)
*
*            v.pTable - Pointer to next level table. (Length < 0)
*
*************************************************************************************************************/
typedef struct _HUFFMAN_LUT
{
	int Length;               
	union
	{
		struct _HUFFMAN_LUT* pTable;
		SYMBOL_TYPE          Value;
    } v;
} HUFFMAN_LUT, *PHUFFMAN_LUT;

/************************************************************************************************************
*  Type    : HUFFMAN_DECODER
*
*  Purpose : Stores instance information for the huffman decoder.
*
*  Fields  : pTable    - Pointer to the first level decoder table.
*
*            IndexBits - Number of bits to use as an index for the first level decoder table.
*
*************************************************************************************************************/
typedef struct _HUFFMAN_DECODER
{
    PHUFFMAN_LUT pTable;
    int          IndexBits;
} HUFFMAN_DECODER, *PHUFFMAN_DECODER;

/************************************************************************************************************
*  Type    : HUFFMAN_CODE
*
*  Purpose : Stores the value and the huffman codeword of a symbol.
*
*  Fields  : Value    - Symbol value.
*
*            Length   - The number of bits in the huffman code word.
*
*            Code     - The huffman code word. The first bit of the code word should go in the most
*                       significant bit of Code[0]. Unused bits should be set to 0.
*
*            pNext    - This field is intended for internal use only - calling functions can ignore this field.
*                       This field is used to store a linked list of code words in a branch when creating the
*                       decoder tables.
*
*************************************************************************************************************/
typedef struct _HUFFMAN_CODE
{
    SYMBOL_TYPE            Value;
    int                    Length;
    UByte                  Code[(HUFFMAN_MAX_CODEWORD_BITS+7)/8];
    struct _HUFFMAN_CODE*  pNext; /* internal use */
} HUFFMAN_CODE, *PHUFFMAN_CODE;


/************************************************************************************************************
*  
*   Public functions
*
*************************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

PHUFFMAN_DECODER huffmanCreateDecoder(const PHUFFMAN_CODE HuffmanCodes,
                                      int   NumberOfCodes,
                                      int   IndexBits);

void             huffmanDestroyDecoder(PHUFFMAN_DECODER pDecoder);

SYMBOL_TYPE      huffmanDecodeSymbol(const PHUFFMAN_DECODER pDecoder,
                                     const UByte*           pStream,
                                     int*                   pOffset);


/************************************************************************************************************
*  
*   Utility functions
*
*************************************************************************************************************/
int              huffmanDecoderTableSize(const PHUFFMAN_DECODER pDecoder);

Bool             huffmanGenerateDecoderCode(const PHUFFMAN_DECODER pDecoder,
                                            FILE*                  pFile,
                                            const char*            pName);

void             huffmanEncodeCode(const HUFFMAN_CODE* pCode,
                                   UByte*              pStream,
                                   int*                pOffset);

#ifdef __cplusplus
}
#endif
#endif
