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

Source file: huf_s_cont.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
07 Nov 2001	JD	Initial version
25 Jul 2002	TM	m8540 change ( 8 sf in one frame in stead of 9 )
21 Nov 2003 AG  Redefinition of s_cont for ADPCM

************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of huf_s_cont huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _huf_s_cont_[] =
                    {{    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 4}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 8}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 12}},
                     {     2, {(PHUFFMAN_LUT)      9}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 6}},
                     {     3, {(PHUFFMAN_LUT)      4}},
                     {     4, {(PHUFFMAN_LUT)      8}},
                     {     4, {(PHUFFMAN_LUT)      7}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 10}},
                     {     3, {(PHUFFMAN_LUT)      3}},
                     {     4, {(PHUFFMAN_LUT)      1}},
                     {     4, {(PHUFFMAN_LUT)      6}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_cont_ + 14}},
                     {     3, {(PHUFFMAN_LUT)      2}},
                     {     4, {(PHUFFMAN_LUT)      0}},
                     {     4, {(PHUFFMAN_LUT)      5}}};

const HUFFMAN_DECODER huf_s_cont = {(PHUFFMAN_LUT)_huf_s_cont_, 2};
/*********************************************************************/
