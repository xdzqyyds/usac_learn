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

Source file: hufCoarseItd.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
21 Nov 2002	AR	Initial version
************************************************************************/

#include "HuffmanDecoder.h"

/*********************************************************************
* Definition of hufCoarseItd huffman decoder
* (Automatically generated - casting needed to avoid warnings on MSVC)
**********************************************************************/
static const HUFFMAN_LUT _hufCoarseItd_[] =
                    {{     1, {(PHUFFMAN_LUT)      0}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 2}},
                     {     3, {(PHUFFMAN_LUT)     -1}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 6}},
                     {    -3, {(PHUFFMAN_LUT) _hufCoarseItd_ + 22}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 170}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 10}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 16}},
                     {     5, {(PHUFFMAN_LUT)      3}},
                     {     5, {(PHUFFMAN_LUT)     -3}},
                     {     7, {(PHUFFMAN_LUT)     15}},
                     {     7, {(PHUFFMAN_LUT)      9}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 14}},
                     {     7, {(PHUFFMAN_LUT)     -8}},
                     {     8, {(PHUFFMAN_LUT)    -14}},
                     {     8, {(PHUFFMAN_LUT)    -13}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 18}},
                     {     6, {(PHUFFMAN_LUT)      5}},
                     {     7, {(PHUFFMAN_LUT)      8}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 20}},
                     {     8, {(PHUFFMAN_LUT)     12}},
                     {     8, {(PHUFFMAN_LUT)     13}},
                     {     6, {(PHUFFMAN_LUT)     -5}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 30}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 156}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 158}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 162}},
                     {     6, {(PHUFFMAN_LUT)      4}},
                     {     6, {(PHUFFMAN_LUT)     -4}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 166}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 34}},
                     {     8, {(PHUFFMAN_LUT)    -12}},
                     {     7, {(PHUFFMAN_LUT)      7}},
                     {     7, {(PHUFFMAN_LUT)      7}},
                     {    -3, {(PHUFFMAN_LUT) _hufCoarseItd_ + 36}},
                     {     9, {(PHUFFMAN_LUT)     14}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 44}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 46}},
                     {    12, {(PHUFFMAN_LUT)    -21}},
                     {    12, {(PHUFFMAN_LUT)    -17}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 146}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 148}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 150}},
                     {    -2, {(PHUFFMAN_LUT) _hufCoarseItd_ + 152}},
                     {    13, {(PHUFFMAN_LUT)     17}},
                     {    13, {(PHUFFMAN_LUT)     20}},
                     {    13, {(PHUFFMAN_LUT)     28}},
                     {    13, {(PHUFFMAN_LUT)     28}},
                     {    -6, {(PHUFFMAN_LUT) _hufCoarseItd_ + 50}},
                     {    14, {(PHUFFMAN_LUT)    -29}},
                     {    20, {(PHUFFMAN_LUT)    -32}},
                     {    20, {(PHUFFMAN_LUT)    -31}},
                     {    20, {(PHUFFMAN_LUT)    -30}},
                     {    20, {(PHUFFMAN_LUT)    -27}},
                     {    20, {(PHUFFMAN_LUT)    -26}},
                     {    20, {(PHUFFMAN_LUT)    -24}},
                     {    20, {(PHUFFMAN_LUT)    -19}},
                     {    20, {(PHUFFMAN_LUT)     22}},
                     {    20, {(PHUFFMAN_LUT)     24}},
                     {    20, {(PHUFFMAN_LUT)     25}},
                     {    20, {(PHUFFMAN_LUT)     26}},
                     {    20, {(PHUFFMAN_LUT)     27}},
                     {    20, {(PHUFFMAN_LUT)     29}},
                     {    20, {(PHUFFMAN_LUT)     30}},
                     {    20, {(PHUFFMAN_LUT)     31}},
                     {    20, {(PHUFFMAN_LUT)     32}},
                     {    20, {(PHUFFMAN_LUT)     33}},
                     {    20, {(PHUFFMAN_LUT)     34}},
                     {    20, {(PHUFFMAN_LUT)     35}},
                     {    20, {(PHUFFMAN_LUT)     36}},
                     {    20, {(PHUFFMAN_LUT)     37}},
                     {    20, {(PHUFFMAN_LUT)     38}},
                     {    20, {(PHUFFMAN_LUT)     39}},
                     {    20, {(PHUFFMAN_LUT)     40}},
                     {    20, {(PHUFFMAN_LUT)     41}},
                     {    20, {(PHUFFMAN_LUT)     42}},
                     {    20, {(PHUFFMAN_LUT)     43}},
                     {    20, {(PHUFFMAN_LUT)     44}},
                     {    20, {(PHUFFMAN_LUT)     45}},
                     {    20, {(PHUFFMAN_LUT)     46}},
                     {    20, {(PHUFFMAN_LUT)     47}},
                     {    20, {(PHUFFMAN_LUT)     48}},
                     {    20, {(PHUFFMAN_LUT)     49}},
                     {    20, {(PHUFFMAN_LUT)     50}},
                     {    20, {(PHUFFMAN_LUT)     51}},
                     {    20, {(PHUFFMAN_LUT)     52}},
                     {    20, {(PHUFFMAN_LUT)     53}},
                     {    20, {(PHUFFMAN_LUT)     54}},
                     {    20, {(PHUFFMAN_LUT)     55}},
                     {    20, {(PHUFFMAN_LUT)     56}},
                     {    20, {(PHUFFMAN_LUT)     57}},
                     {    20, {(PHUFFMAN_LUT)     58}},
                     {    20, {(PHUFFMAN_LUT)     59}},
                     {    20, {(PHUFFMAN_LUT)     60}},
                     {    20, {(PHUFFMAN_LUT)     61}},
                     {    20, {(PHUFFMAN_LUT)     62}},
                     {    20, {(PHUFFMAN_LUT)     63}},
                     {    20, {(PHUFFMAN_LUT)     64}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 114}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 116}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 118}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 120}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 122}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 124}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 126}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 128}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 130}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 132}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 134}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 136}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 138}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 140}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 142}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 144}},
                     {    21, {(PHUFFMAN_LUT)    -64}},
                     {    21, {(PHUFFMAN_LUT)    -63}},
                     {    21, {(PHUFFMAN_LUT)    -62}},
                     {    21, {(PHUFFMAN_LUT)    -61}},
                     {    21, {(PHUFFMAN_LUT)    -60}},
                     {    21, {(PHUFFMAN_LUT)    -59}},
                     {    21, {(PHUFFMAN_LUT)    -58}},
                     {    21, {(PHUFFMAN_LUT)    -57}},
                     {    21, {(PHUFFMAN_LUT)    -56}},
                     {    21, {(PHUFFMAN_LUT)    -55}},
                     {    21, {(PHUFFMAN_LUT)    -54}},
                     {    21, {(PHUFFMAN_LUT)    -53}},
                     {    21, {(PHUFFMAN_LUT)    -52}},
                     {    21, {(PHUFFMAN_LUT)    -51}},
                     {    21, {(PHUFFMAN_LUT)    -50}},
                     {    21, {(PHUFFMAN_LUT)    -49}},
                     {    21, {(PHUFFMAN_LUT)    -48}},
                     {    21, {(PHUFFMAN_LUT)    -47}},
                     {    21, {(PHUFFMAN_LUT)    -46}},
                     {    21, {(PHUFFMAN_LUT)    -45}},
                     {    21, {(PHUFFMAN_LUT)    -44}},
                     {    21, {(PHUFFMAN_LUT)    -43}},
                     {    21, {(PHUFFMAN_LUT)    -42}},
                     {    21, {(PHUFFMAN_LUT)    -41}},
                     {    21, {(PHUFFMAN_LUT)    -40}},
                     {    21, {(PHUFFMAN_LUT)    -39}},
                     {    21, {(PHUFFMAN_LUT)    -38}},
                     {    21, {(PHUFFMAN_LUT)    -37}},
                     {    21, {(PHUFFMAN_LUT)    -36}},
                     {    21, {(PHUFFMAN_LUT)    -35}},
                     {    21, {(PHUFFMAN_LUT)    -34}},
                     {    21, {(PHUFFMAN_LUT)    -33}},
                     {    13, {(PHUFFMAN_LUT)    -28}},
                     {    13, {(PHUFFMAN_LUT)    -25}},
                     {    13, {(PHUFFMAN_LUT)     18}},
                     {    13, {(PHUFFMAN_LUT)     19}},
                     {    13, {(PHUFFMAN_LUT)     21}},
                     {    13, {(PHUFFMAN_LUT)     23}},
                     {    14, {(PHUFFMAN_LUT)    -23}},
                     {    14, {(PHUFFMAN_LUT)    -22}},
                     {    14, {(PHUFFMAN_LUT)    -20}},
                     {    14, {(PHUFFMAN_LUT)    -18}},
                     {     7, {(PHUFFMAN_LUT)     -7}},
                     {     7, {(PHUFFMAN_LUT)    -16}},
                     {     7, {(PHUFFMAN_LUT)     16}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 160}},
                     {     8, {(PHUFFMAN_LUT)     11}},
                     {     8, {(PHUFFMAN_LUT)    -10}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 164}},
                     {     7, {(PHUFFMAN_LUT)     -6}},
                     {     8, {(PHUFFMAN_LUT)    -11}},
                     {     8, {(PHUFFMAN_LUT)     10}},
                     {     7, {(PHUFFMAN_LUT)      6}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 168}},
                     {     8, {(PHUFFMAN_LUT)    -15}},
                     {     8, {(PHUFFMAN_LUT)     -9}},
                     {     4, {(PHUFFMAN_LUT)      1}},
                     {    -1, {(PHUFFMAN_LUT) _hufCoarseItd_ + 172}},
                     {     5, {(PHUFFMAN_LUT)      2}},
                     {     5, {(PHUFFMAN_LUT)     -2}}};

const HUFFMAN_DECODER hufCoarseItd = {(PHUFFMAN_LUT)_hufCoarseItd_, 1};
/*********************************************************************/
