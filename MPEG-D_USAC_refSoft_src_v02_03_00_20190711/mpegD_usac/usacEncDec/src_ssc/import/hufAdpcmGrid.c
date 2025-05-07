/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
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

Copyright  2003.

Source file: hufAdpcmGrid.c

Required libraries: <none>

Authors:
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
21 Nov 2003	AG	Initial version
************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of hufAdpcmGrid huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufAdpcmGrid_[] =
                    {{    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 8}},
                     {     3, {(PHUFFMAN_LUT)      7}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 16}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 18}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 24}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 26}},
                     {    -2, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 28}},
                     {     4, {(PHUFFMAN_LUT)     12}},
                     {     4, {(PHUFFMAN_LUT)     13}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 12}},
                     {     4, {(PHUFFMAN_LUT)     11}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 14}},
                     {     5, {(PHUFFMAN_LUT)     15}},
                     {     6, {(PHUFFMAN_LUT)     20}},
                     {     6, {(PHUFFMAN_LUT)     19}},
                     {     4, {(PHUFFMAN_LUT)      9}},
                     {     4, {(PHUFFMAN_LUT)      6}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 20}},
                     {     4, {(PHUFFMAN_LUT)     10}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 22}},
                     {     5, {(PHUFFMAN_LUT)     21}},
                     {     6, {(PHUFFMAN_LUT)     18}},
                     {     6, {(PHUFFMAN_LUT)      0}},
                     {     4, {(PHUFFMAN_LUT)      5}},
                     {     4, {(PHUFFMAN_LUT)      8}},
                     {     4, {(PHUFFMAN_LUT)      3}},
                     {     4, {(PHUFFMAN_LUT)      4}},
                     {     5, {(PHUFFMAN_LUT)     14}},
                     {     5, {(PHUFFMAN_LUT)      1}},
                     {     5, {(PHUFFMAN_LUT)      2}},
                     {    -1, {(PHUFFMAN_LUT) _hufAdpcmGrid_ + 32}},
                     {     6, {(PHUFFMAN_LUT)     17}},
                     {     6, {(PHUFFMAN_LUT)     16}}};

const HUFFMAN_DECODER hufAdpcmGrid = {(PHUFFMAN_LUT)_hufAdpcmGrid_, 3};
/*********************************************************************/
