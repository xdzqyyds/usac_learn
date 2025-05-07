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

Source file: hufNoiRelLsfE.c

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
* Definition of hufNoiRelLsfE huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufNoiRelLsfE_[] =
                    {{     2, {(PHUFFMAN_LUT)     15}},
                     {     2, {(PHUFFMAN_LUT)     17}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 4}},
                     {     2, {(PHUFFMAN_LUT)     16}},
                     {     4, {(PHUFFMAN_LUT)     14}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 8}},
                     {     4, {(PHUFFMAN_LUT)     18}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 10}},
                     {     5, {(PHUFFMAN_LUT)     20}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 12}},
                     {     6, {(PHUFFMAN_LUT)     13}},
                     {     7, {(PHUFFMAN_LUT)     11}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 14}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 16}},
                     {     8, {(PHUFFMAN_LUT)     22}},
                     {     9, {(PHUFFMAN_LUT)     24}},
                     {     9, {(PHUFFMAN_LUT)     23}},
                     {    -2, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 20}},
                     {     5, {(PHUFFMAN_LUT)     19}},
                     {     7, {(PHUFFMAN_LUT)     12}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 24}},
                     {     7, {(PHUFFMAN_LUT)     21}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 26}},
                     {     8, {(PHUFFMAN_LUT)     10}},
                     {     8, {(PHUFFMAN_LUT)      7}},
                     {     8, {(PHUFFMAN_LUT)     25}},
                     {    -1, {(PHUFFMAN_LUT) _hufNoiRelLsfE_ + 28}},
                     {     9, {(PHUFFMAN_LUT)      8}},
                     {     9, {(PHUFFMAN_LUT)      9}}};

const HUFFMAN_DECODER hufNoiRelLsfE = {(PHUFFMAN_LUT)_hufNoiRelLsfE_, 2};
/*********************************************************************/
