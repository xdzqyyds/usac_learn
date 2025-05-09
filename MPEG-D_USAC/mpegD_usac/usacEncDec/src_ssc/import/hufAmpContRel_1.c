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

Source file: hufAmpContRel_1.c

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
* Definition of hufAmpContRel_1 huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAmpContRel_1_[] =
                    {{     1, {(PHUFFMAN_LUT)      0}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 2}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 6}},
                     {     3, {(PHUFFMAN_LUT)      2}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 24}},
                     {     3, {(PHUFFMAN_LUT)     -2}},
                     {     4, {(PHUFFMAN_LUT)      4}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 8}},
                     {     5, {(PHUFFMAN_LUT)      6}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 10}},
                     {     6, {(PHUFFMAN_LUT)      8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 12}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 14}},
                     {     7, {(PHUFFMAN_LUT)    -10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 16}},
                     {     8, {(PHUFFMAN_LUT)     12}},
                     {     9, {(PHUFFMAN_LUT)    -16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 20}},
                     {    10, {(PHUFFMAN_LUT)     18}},
                     {    11, {(PHUFFMAN_LUT)     22}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 22}},
                     {    12, {(PHUFFMAN_LUT)     26}},
                     {    12, {(PHUFFMAN_LUT)    -26}},
                     {     4, {(PHUFFMAN_LUT)     -4}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 26}},
                     {     5, {(PHUFFMAN_LUT)     -6}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 28}},
                     {     6, {(PHUFFMAN_LUT)     -8}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 30}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 34}},
                     {     8, {(PHUFFMAN_LUT)    -12}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 40}},
                     {     8, {(PHUFFMAN_LUT)     10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 36}},
                     {     9, {(PHUFFMAN_LUT)     14}},
                     {    10, {(PHUFFMAN_LUT)    -18}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 38}},
                     {    11, {(PHUFFMAN_LUT)    -24}},
                     {    11, {(PHUFFMAN_LUT)     20}},
                     {     9, {(PHUFFMAN_LUT)    -14}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 42}},
                     {    10, {(PHUFFMAN_LUT)     16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 44}},
                     {    11, {(PHUFFMAN_LUT)    -20}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_1_ + 46}},
                     {    12, {(PHUFFMAN_LUT)    -22}},
                     {    12, {(PHUFFMAN_LUT)     24}}};

const HUFFMAN_DECODER hufAmpContRel_1 = {(PHUFFMAN_LUT)_hufAmpContRel_1_, 1};
/*********************************************************************/
