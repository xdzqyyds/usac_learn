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

Source file: huf_s_nrof_births.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
07 Nov 2001	JD	Initial version
************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of huf_s_nrof_births huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _huf_s_nrof_births_[] =
                    {{    -2, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 8}},
                     {     3, {(PHUFFMAN_LUT)      6}},
                     {     3, {(PHUFFMAN_LUT)      0}},
                     {     3, {(PHUFFMAN_LUT)      3}},
                     {     3, {(PHUFFMAN_LUT)      5}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 86}},
                     {     3, {(PHUFFMAN_LUT)      4}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 90}},
                     {    -3, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 12}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 82}},
                     {     4, {(PHUFFMAN_LUT)      8}},
                     {     4, {(PHUFFMAN_LUT)      8}},
                     {     8, {(PHUFFMAN_LUT)     15}},
                     {     8, {(PHUFFMAN_LUT)     24}},
                     {    -2, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 20}},
                     {     8, {(PHUFFMAN_LUT)     14}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 70}},
                     {     8, {(PHUFFMAN_LUT)     22}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 74}},
                     {    -2, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 78}},
                     {    -4, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 24}},
                     {    10, {(PHUFFMAN_LUT)     17}},
                     {     9, {(PHUFFMAN_LUT)     16}},
                     {     9, {(PHUFFMAN_LUT)     16}},
                     {    14, {(PHUFFMAN_LUT)     60}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 40}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 42}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 44}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 46}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 48}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 50}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 52}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 54}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 56}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 58}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 60}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 62}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 64}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 66}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 68}},
                     {    15, {(PHUFFMAN_LUT)     19}},
                     {    15, {(PHUFFMAN_LUT)     20}},
                     {    15, {(PHUFFMAN_LUT)     28}},
                     {    15, {(PHUFFMAN_LUT)     30}},
                     {    15, {(PHUFFMAN_LUT)     31}},
                     {    15, {(PHUFFMAN_LUT)     32}},
                     {    15, {(PHUFFMAN_LUT)     34}},
                     {    15, {(PHUFFMAN_LUT)     36}},
                     {    15, {(PHUFFMAN_LUT)     37}},
                     {    15, {(PHUFFMAN_LUT)     39}},
                     {    15, {(PHUFFMAN_LUT)     40}},
                     {    15, {(PHUFFMAN_LUT)     41}},
                     {    15, {(PHUFFMAN_LUT)     42}},
                     {    15, {(PHUFFMAN_LUT)     43}},
                     {    15, {(PHUFFMAN_LUT)     44}},
                     {    15, {(PHUFFMAN_LUT)     45}},
                     {    15, {(PHUFFMAN_LUT)     46}},
                     {    15, {(PHUFFMAN_LUT)     47}},
                     {    15, {(PHUFFMAN_LUT)     48}},
                     {    15, {(PHUFFMAN_LUT)     49}},
                     {    15, {(PHUFFMAN_LUT)     50}},
                     {    15, {(PHUFFMAN_LUT)     51}},
                     {    15, {(PHUFFMAN_LUT)     52}},
                     {    15, {(PHUFFMAN_LUT)     53}},
                     {    15, {(PHUFFMAN_LUT)     54}},
                     {    15, {(PHUFFMAN_LUT)     55}},
                     {    15, {(PHUFFMAN_LUT)     56}},
                     {    15, {(PHUFFMAN_LUT)     57}},
                     {    15, {(PHUFFMAN_LUT)     58}},
                     {    15, {(PHUFFMAN_LUT)     59}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 72}},
                     {     9, {(PHUFFMAN_LUT)     21}},
                     {    10, {(PHUFFMAN_LUT)     18}},
                     {    10, {(PHUFFMAN_LUT)     25}},
                     {     9, {(PHUFFMAN_LUT)     23}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 76}},
                     {    10, {(PHUFFMAN_LUT)     26}},
                     {    10, {(PHUFFMAN_LUT)     27}},
                     {    10, {(PHUFFMAN_LUT)     29}},
                     {    10, {(PHUFFMAN_LUT)     33}},
                     {    10, {(PHUFFMAN_LUT)     35}},
                     {    10, {(PHUFFMAN_LUT)     38}},
                     {     6, {(PHUFFMAN_LUT)     12}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 84}},
                     {     7, {(PHUFFMAN_LUT)     13}},
                     {     7, {(PHUFFMAN_LUT)     11}},
                     {     4, {(PHUFFMAN_LUT)      1}},
                     {    -1, {(PHUFFMAN_LUT) _huf_s_nrof_births_ + 88}},
                     {     5, {(PHUFFMAN_LUT)     10}},
                     {     5, {(PHUFFMAN_LUT)      9}},
                     {     4, {(PHUFFMAN_LUT)      2}},
                     {     4, {(PHUFFMAN_LUT)      7}}};

const HUFFMAN_DECODER huf_s_nrof_births = {(PHUFFMAN_LUT)_huf_s_nrof_births_, 3};
/*********************************************************************/
