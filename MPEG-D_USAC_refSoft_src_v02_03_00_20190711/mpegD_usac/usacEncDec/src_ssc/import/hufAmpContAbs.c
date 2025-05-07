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

Source file: hufAmpContAbs.c

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
* Definition of hufAmpContAbs huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAmpContAbs_[] =
                    {{     3, {(PHUFFMAN_LUT)     88}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 44}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 48}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 50}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 52}},
                     {     4, {(PHUFFMAN_LUT)    144}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 12}},
                     {     5, {(PHUFFMAN_LUT)     40}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 14}},
                     {     6, {(PHUFFMAN_LUT)    168}},
                     {     7, {(PHUFFMAN_LUT)     16}},
                     {     7, {(PHUFFMAN_LUT)    176}},
                     {     4, {(PHUFFMAN_LUT)     56}},
                     {     4, {(PHUFFMAN_LUT)    136}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 20}},
                     {     4, {(PHUFFMAN_LUT)     64}},
                     {     5, {(PHUFFMAN_LUT)    160}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 22}},
                     {     6, {(PHUFFMAN_LUT)     32}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 24}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 26}},
                     {     7, {(PHUFFMAN_LUT)     24}},
                     {     8, {(PHUFFMAN_LUT)      8}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 28}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 30}},
                     {     9, {(PHUFFMAN_LUT)    184}},
                     {    10, {(PHUFFMAN_LUT)    192}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 32}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 34}},
                     {    11, {(PHUFFMAN_LUT)      0}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 36}},
                     {    12, {(PHUFFMAN_LUT)    200}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 38}},
                     {    13, {(PHUFFMAN_LUT)    208}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 40}},
                     {    14, {(PHUFFMAN_LUT)    216}},
                     {    15, {(PHUFFMAN_LUT)    240}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 42}},
                     {    16, {(PHUFFMAN_LUT)    224}},
                     {    16, {(PHUFFMAN_LUT)    232}},
                     {     4, {(PHUFFMAN_LUT)    128}},
                     {    -1, {(PHUFFMAN_LUT) _hufAmpContAbs_ + 46}},
                     {     5, {(PHUFFMAN_LUT)    152}},
                     {     5, {(PHUFFMAN_LUT)     48}},
                     {     4, {(PHUFFMAN_LUT)     72}},
                     {     4, {(PHUFFMAN_LUT)    120}},
                     {     4, {(PHUFFMAN_LUT)    112}},
                     {     4, {(PHUFFMAN_LUT)     80}},
                     {     4, {(PHUFFMAN_LUT)    104}},
                     {     4, {(PHUFFMAN_LUT)     96}}};

const HUFFMAN_DECODER hufAmpContAbs = {(PHUFFMAN_LUT)_hufAmpContAbs_, 3};
/*********************************************************************/
