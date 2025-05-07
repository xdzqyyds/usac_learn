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

Source file: hufAmpContRel_0.c

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
* Definition of hufAmpContRel_0 huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAmpContRel_0_[] =
                    {{    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 4}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 34}},
                     {     2, {(PHUFFMAN_LUT)      0}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 62}},
                     {     4, {(PHUFFMAN_LUT)      3}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 30}},
                     {     4, {(PHUFFMAN_LUT)     -3}},
                     {     5, {(PHUFFMAN_LUT)      5}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 10}},
                     {     7, {(PHUFFMAN_LUT)      9}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 14}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 16}},
                     {     7, {(PHUFFMAN_LUT)     -9}},
                     {     8, {(PHUFFMAN_LUT)     11}},
                     {     8, {(PHUFFMAN_LUT)    -12}},
                     {     9, {(PHUFFMAN_LUT)    -14}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 20}},
                     {     9, {(PHUFFMAN_LUT)     13}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 26}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 22}},
                     {    10, {(PHUFFMAN_LUT)     16}},
                     {    11, {(PHUFFMAN_LUT)    -20}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 24}},
                     {    12, {(PHUFFMAN_LUT)     23}},
                     {    12, {(PHUFFMAN_LUT)    -23}},
                     {    10, {(PHUFFMAN_LUT)    -17}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 28}},
                     {    11, {(PHUFFMAN_LUT)     19}},
                     {    11, {(PHUFFMAN_LUT)     25}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 32}},
                     {     5, {(PHUFFMAN_LUT)     -5}},
                     {     6, {(PHUFFMAN_LUT)     -7}},
                     {     6, {(PHUFFMAN_LUT)      6}},
                     {     3, {(PHUFFMAN_LUT)      1}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 36}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 38}},
                     {     4, {(PHUFFMAN_LUT)      2}},
                     {     5, {(PHUFFMAN_LUT)      4}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 40}},
                     {     7, {(PHUFFMAN_LUT)      8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 44}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 54}},
                     {     7, {(PHUFFMAN_LUT)     -8}},
                     {     8, {(PHUFFMAN_LUT)    -11}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 46}},
                     {     9, {(PHUFFMAN_LUT)    -13}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 48}},
                     {    10, {(PHUFFMAN_LUT)    -16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 50}},
                     {    11, {(PHUFFMAN_LUT)    -19}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 52}},
                     {    12, {(PHUFFMAN_LUT)    -22}},
                     {    12, {(PHUFFMAN_LUT)     22}},
                     {     8, {(PHUFFMAN_LUT)     10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 56}},
                     {     9, {(PHUFFMAN_LUT)     12}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 58}},
                     {    10, {(PHUFFMAN_LUT)     15}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 60}},
                     {    11, {(PHUFFMAN_LUT)    -25}},
                     {    11, {(PHUFFMAN_LUT)     18}},
                     {     3, {(PHUFFMAN_LUT)     -1}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 64}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 66}},
                     {     4, {(PHUFFMAN_LUT)     -2}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 68}},
                     {     5, {(PHUFFMAN_LUT)     -4}},
                     {     6, {(PHUFFMAN_LUT)     -6}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 70}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 72}},
                     {     7, {(PHUFFMAN_LUT)      7}},
                     {     8, {(PHUFFMAN_LUT)    -10}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 74}},
                     {    10, {(PHUFFMAN_LUT)    -15}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 78}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 88}},
                     {    10, {(PHUFFMAN_LUT)     14}},
                     {    12, {(PHUFFMAN_LUT)    -21}},
                     {    12, {(PHUFFMAN_LUT)     21}},
                     {    12, {(PHUFFMAN_LUT)     20}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 82}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 84}},
                     {    13, {(PHUFFMAN_LUT)    -24}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContRel_0_ + 86}},
                     {    14, {(PHUFFMAN_LUT)     24}},
                     {    15, {(PHUFFMAN_LUT)    -26}},
                     {    15, {(PHUFFMAN_LUT)     26}},
                     {    11, {(PHUFFMAN_LUT)    -18}},
                     {    11, {(PHUFFMAN_LUT)     17}}};

const HUFFMAN_DECODER hufAmpContRel_0 = {(PHUFFMAN_LUT)_hufAmpContRel_0_, 2};
/*********************************************************************/
