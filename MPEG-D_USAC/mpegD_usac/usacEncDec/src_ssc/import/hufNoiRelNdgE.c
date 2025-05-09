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

Source file: hufNoiRelNdgE.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
25 Jul 2002	TM	Initial version
************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of hufNoiRelNdgE huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufNoiRelNdgE_[] =
                    {{    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 2}},
                     {     1, {(PHUFFMAN_LUT)      0}},
                     {     2, {(PHUFFMAN_LUT)     -1}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 4}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 6}},
                     {     3, {(PHUFFMAN_LUT)      1}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 10}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 16}},
                     {     5, {(PHUFFMAN_LUT)      2}},
                     {     5, {(PHUFFMAN_LUT)     -2}},
                     {     6, {(PHUFFMAN_LUT)      3}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 12}},
                     {     7, {(PHUFFMAN_LUT)     -4}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 14}},
                     {     8, {(PHUFFMAN_LUT)     -5}},
                     {     8, {(PHUFFMAN_LUT)      5}},
                     {     6, {(PHUFFMAN_LUT)     -3}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 22}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 24}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 32}},
                     {     8, {(PHUFFMAN_LUT)      4}},
                     {     9, {(PHUFFMAN_LUT)     -6}},
                     {     9, {(PHUFFMAN_LUT)      6}},
                     {    10, {(PHUFFMAN_LUT)      8}},
                     {    10, {(PHUFFMAN_LUT)     -7}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 28}},
                     {    10, {(PHUFFMAN_LUT)      7}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 30}},
                     {    11, {(PHUFFMAN_LUT)      9}},
                     {    12, {(PHUFFMAN_LUT)     12}},
                     {    12, {(PHUFFMAN_LUT)    -12}},
                     {     9, {(PHUFFMAN_LUT)    -13}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 34}},
                     {    11, {(PHUFFMAN_LUT)     -9}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 38}},
                     {    11, {(PHUFFMAN_LUT)     -8}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelNdgE_ + 40}},
                     {    12, {(PHUFFMAN_LUT)     11}},
                     {    12, {(PHUFFMAN_LUT)    -10}},
                     {    12, {(PHUFFMAN_LUT)    -11}},
                     {    12, {(PHUFFMAN_LUT)     10}}};

const HUFFMAN_DECODER hufNoiRelNdgE = {(PHUFFMAN_LUT)_hufNoiRelNdgE_, 1};
/*********************************************************************/
