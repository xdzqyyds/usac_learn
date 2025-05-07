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

Source file: hufAmpContRel_3.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
07 Nov 2001	JD	Initial version
03 Dec 2001 JD  Improved huffman table
************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of hufAmpContRel_3 huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAmpContRel_3_[] =
                    {{    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 2}},
                     {     1, {(PHUFFMAN_LUT)      0}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 4}},
                     {     2, {(PHUFFMAN_LUT)     -8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 6}},
                     {     3, {(PHUFFMAN_LUT)      8}},
                     {     4, {(PHUFFMAN_LUT)    -16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 10}},
                     {     5, {(PHUFFMAN_LUT)     16}},
                     {     6, {(PHUFFMAN_LUT)    -24}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 12}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_3_ + 14}},
                     {     7, {(PHUFFMAN_LUT)     24}},
                     {     8, {(PHUFFMAN_LUT)     32}},
                     {     8, {(PHUFFMAN_LUT)    -32}}};

const HUFFMAN_DECODER hufAmpContRel_3 = {(PHUFFMAN_LUT)_hufAmpContRel_3_, 1};
/*********************************************************************/
