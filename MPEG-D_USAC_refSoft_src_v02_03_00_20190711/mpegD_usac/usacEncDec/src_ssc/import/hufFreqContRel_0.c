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

Source file: hufFreqContRel_0.c

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
* Definition of hufFreqContRel_0 huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufFreqContRel_0_[] =
                    {{     2, {(PHUFFMAN_LUT)      0}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 4}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 62}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 90}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 8}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 28}},
                     {     4, {(PHUFFMAN_LUT)     -2}},
                     {     4, {(PHUFFMAN_LUT)      2}},
                     {     6, {(PHUFFMAN_LUT)      8}},
                     {     6, {(PHUFFMAN_LUT)     -8}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 12}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 14}},
                     {     7, {(PHUFFMAN_LUT)     11}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 16}},
                     {     8, {(PHUFFMAN_LUT)    -16}},
                     {     9, {(PHUFFMAN_LUT)    -21}},
                     {     9, {(PHUFFMAN_LUT)     21}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 20}},
                     {     7, {(PHUFFMAN_LUT)    -11}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 22}},
                     {     8, {(PHUFFMAN_LUT)    -15}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 24}},
                     {     9, {(PHUFFMAN_LUT)    -37}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 26}},
                     {    10, {(PHUFFMAN_LUT)    -27}},
                     {    11, {(PHUFFMAN_LUT)    -33}},
                     {    11, {(PHUFFMAN_LUT)     33}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 32}},
                     {     6, {(PHUFFMAN_LUT)     -7}},
                     {     6, {(PHUFFMAN_LUT)      7}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 54}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 36}},
                     {     8, {(PHUFFMAN_LUT)     15}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 42}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 46}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 38}},
                     {     9, {(PHUFFMAN_LUT)     20}},
                     {    10, {(PHUFFMAN_LUT)    -26}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 40}},
                     {    11, {(PHUFFMAN_LUT)    -32}},
                     {    11, {(PHUFFMAN_LUT)     31}},
                     {     9, {(PHUFFMAN_LUT)    -20}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 44}},
                     {    10, {(PHUFFMAN_LUT)     25}},
                     {    10, {(PHUFFMAN_LUT)    -25}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 48}},
                     {     9, {(PHUFFMAN_LUT)     19}},
                     {    10, {(PHUFFMAN_LUT)     26}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 50}},
                     {    11, {(PHUFFMAN_LUT)     32}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 52}},
                     {    12, {(PHUFFMAN_LUT)     36}},
                     {    12, {(PHUFFMAN_LUT)    -36}},
                     {     7, {(PHUFFMAN_LUT)     10}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 56}},
                     {     8, {(PHUFFMAN_LUT)     14}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 58}},
                     {     9, {(PHUFFMAN_LUT)    -19}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 60}},
                     {    10, {(PHUFFMAN_LUT)     24}},
                     {    10, {(PHUFFMAN_LUT)    -24}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 64}},
                     {     3, {(PHUFFMAN_LUT)     -1}},
                     {     5, {(PHUFFMAN_LUT)     -4}},
                     {     5, {(PHUFFMAN_LUT)      4}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 68}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 80}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 70}},
                     {     6, {(PHUFFMAN_LUT)      6}},
                     {     7, {(PHUFFMAN_LUT)    -10}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 72}},
                     {     8, {(PHUFFMAN_LUT)    -14}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 74}},
                     {     9, {(PHUFFMAN_LUT)     18}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 76}},
                     {    11, {(PHUFFMAN_LUT)     30}},
                     {    11, {(PHUFFMAN_LUT)    -31}},
                     {    11, {(PHUFFMAN_LUT)     28}},
                     {    11, {(PHUFFMAN_LUT)    -30}},
                     {     6, {(PHUFFMAN_LUT)     -6}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 82}},
                     {     7, {(PHUFFMAN_LUT)      9}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 84}},
                     {     8, {(PHUFFMAN_LUT)     13}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 86}},
                     {     9, {(PHUFFMAN_LUT)    -18}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 88}},
                     {    10, {(PHUFFMAN_LUT)     23}},
                     {    10, {(PHUFFMAN_LUT)    -23}},
                     {     3, {(PHUFFMAN_LUT)      1}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 92}},
                     {     5, {(PHUFFMAN_LUT)     -3}},
                     {     5, {(PHUFFMAN_LUT)      3}},
                     {    -2, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 96}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 128}},
                     {     7, {(PHUFFMAN_LUT)     -9}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 100}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 110}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 120}},
                     {     8, {(PHUFFMAN_LUT)    -13}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 102}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 104}},
                     {     9, {(PHUFFMAN_LUT)    -17}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 106}},
                     {    10, {(PHUFFMAN_LUT)    -22}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 108}},
                     {    11, {(PHUFFMAN_LUT)    -29}},
                     {    12, {(PHUFFMAN_LUT)     35}},
                     {    12, {(PHUFFMAN_LUT)    -35}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 112}},
                     {     8, {(PHUFFMAN_LUT)     12}},
                     {     9, {(PHUFFMAN_LUT)     17}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 114}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 116}},
                     {    10, {(PHUFFMAN_LUT)     22}},
                     {    11, {(PHUFFMAN_LUT)     29}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 118}},
                     {    12, {(PHUFFMAN_LUT)    -34}},
                     {    12, {(PHUFFMAN_LUT)     34}},
                     {     8, {(PHUFFMAN_LUT)    -12}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 122}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 124}},
                     {     9, {(PHUFFMAN_LUT)     16}},
                     {    10, {(PHUFFMAN_LUT)     37}},
                     {    -1, {(PHUFFMAN_LUT) _hufFreqContRel_0_ + 126}},
                     {    11, {(PHUFFMAN_LUT)    -28}},
                     {    11, {(PHUFFMAN_LUT)     27}},
                     {     6, {(PHUFFMAN_LUT)      5}},
                     {     6, {(PHUFFMAN_LUT)     -5}}};

const HUFFMAN_DECODER hufFreqContRel_0 = {(PHUFFMAN_LUT)_hufFreqContRel_0_, 2};
/*********************************************************************/
