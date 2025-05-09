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

Source file: hufAmpBirthRel.c

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
* Definition of hufAmpBirthRel huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAmpBirthRel_[] =
                    {{     2, {(PHUFFMAN_LUT)     -8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 4}},
                     {     2, {(PHUFFMAN_LUT)      0}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 8}},
                     {     3, {(PHUFFMAN_LUT)    -16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 6}},
                     {     4, {(PHUFFMAN_LUT)     16}},
                     {     4, {(PHUFFMAN_LUT)    -24}},
                     {     3, {(PHUFFMAN_LUT)      8}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 10}},
                     {     5, {(PHUFFMAN_LUT)     24}},
                     {     5, {(PHUFFMAN_LUT)    -32}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 14}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 22}},
                     {     6, {(PHUFFMAN_LUT)    -48}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 16}},
                     {     7, {(PHUFFMAN_LUT)    -56}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 20}},
                     {     8, {(PHUFFMAN_LUT)    -64}},
                     {     9, {(PHUFFMAN_LUT)     48}},
                     {     9, {(PHUFFMAN_LUT)    -72}},
                     {     6, {(PHUFFMAN_LUT)    -40}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 24}},
                     {     7, {(PHUFFMAN_LUT)     32}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 26}},
                     {     8, {(PHUFFMAN_LUT)     40}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 28}},
                     {    10, {(PHUFFMAN_LUT)    -80}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 32}},
                     {    10, {(PHUFFMAN_LUT)     56}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 92}},
                     {    11, {(PHUFFMAN_LUT)     64}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 34}},
                     {    -2, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 36}},
                     {    12, {(PHUFFMAN_LUT)    -96}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 40}},
                     {    14, {(PHUFFMAN_LUT)   -112}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 90}},
                     {    14, {(PHUFFMAN_LUT)     88}},
                     {    15, {(PHUFFMAN_LUT)    112}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 42}},
                     {    -4, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 44}},
                     {    16, {(PHUFFMAN_LUT)     96}},
                     {    20, {(PHUFFMAN_LUT)    240}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 60}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 62}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 64}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 66}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 68}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 70}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 72}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 74}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 76}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 78}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 80}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 82}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 84}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 86}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 88}},
                     {    21, {(PHUFFMAN_LUT)   -240}},
                     {    21, {(PHUFFMAN_LUT)   -232}},
                     {    21, {(PHUFFMAN_LUT)   -224}},
                     {    21, {(PHUFFMAN_LUT)   -216}},
                     {    21, {(PHUFFMAN_LUT)   -208}},
                     {    21, {(PHUFFMAN_LUT)   -200}},
                     {    21, {(PHUFFMAN_LUT)   -192}},
                     {    21, {(PHUFFMAN_LUT)   -184}},
                     {    21, {(PHUFFMAN_LUT)   -176}},
                     {    21, {(PHUFFMAN_LUT)   -168}},
                     {    21, {(PHUFFMAN_LUT)   -160}},
                     {    21, {(PHUFFMAN_LUT)   -152}},
                     {    21, {(PHUFFMAN_LUT)   -144}},
                     {    21, {(PHUFFMAN_LUT)   -136}},
                     {    21, {(PHUFFMAN_LUT)   -128}},
                     {    21, {(PHUFFMAN_LUT)    120}},
                     {    21, {(PHUFFMAN_LUT)    128}},
                     {    21, {(PHUFFMAN_LUT)    136}},
                     {    21, {(PHUFFMAN_LUT)    144}},
                     {    21, {(PHUFFMAN_LUT)    152}},
                     {    21, {(PHUFFMAN_LUT)    160}},
                     {    21, {(PHUFFMAN_LUT)    168}},
                     {    21, {(PHUFFMAN_LUT)    176}},
                     {    21, {(PHUFFMAN_LUT)    184}},
                     {    21, {(PHUFFMAN_LUT)    192}},
                     {    21, {(PHUFFMAN_LUT)    200}},
                     {    21, {(PHUFFMAN_LUT)    208}},
                     {    21, {(PHUFFMAN_LUT)    216}},
                     {    21, {(PHUFFMAN_LUT)    224}},
                     {    21, {(PHUFFMAN_LUT)    232}},
                     {    15, {(PHUFFMAN_LUT)   -120}},
                     {    15, {(PHUFFMAN_LUT)    104}},
                     {    11, {(PHUFFMAN_LUT)    -88}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 94}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpBirthRel_ + 96}},
                     {    12, {(PHUFFMAN_LUT)     72}},
                     {    13, {(PHUFFMAN_LUT)   -104}},
                     {    13, {(PHUFFMAN_LUT)     80}}};

const HUFFMAN_DECODER hufAmpBirthRel = {(PHUFFMAN_LUT)_hufAmpBirthRel_, 2};
/*********************************************************************/
