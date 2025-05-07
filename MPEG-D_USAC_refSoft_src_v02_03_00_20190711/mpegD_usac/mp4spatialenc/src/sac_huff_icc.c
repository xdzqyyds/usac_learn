/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/
/*******************************************************************************
This software module was further modified in light of the JAME project that 
was initiated by Yonsei University and LG Electronics.

The JAME project aims at making a better MPEG Reference Software especially
for the encoder in direction to simplification, quality improvement and 
efficient implementation being still compliant with the ISO/IEC specification. 
Those intending to participate in this project can use this software module 
under consideration of above mentioned ISO/IEC copyright notice. 

Any voluntary contributions to this software module in the course of this 
project can be recognized by proudly listing up their authors here:

- Henney Oh, LG Electronics (henney.oh@lge.com)
- Sungyong Yoon, LG Electronics (sungyong.yoon@lge.com)

Copyright (c) JAME 2010.
*******************************************************************************/
                                                                  
#include "sac_huff_tab.h"                                       


const HUFF_ICC_TABLE huffICCTab =
{
  {
    { 0x1e, 0x0e, 0x06, 0x00, 0x02, 0x01, 0x3e, 0x3f },
    {    5,    4,    3,    2,    2,    2,    6,    6 }
  },
  {
    {
      { 0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x007f },
      {      1,      2,      3,      4,      5,      6,      7,      7 }
    },
    {
      { 0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x007f },
      {      1,      2,      3,      4,      5,      6,      7,      7 }
    }
  },
  { /* escape */
    { 
      { 0x0, 0x0, 0x0ddff, 0x0087b },
      {   0,   0,      16,      12 }
    },
    {
      { 0x0, 0x0, 0x0, 0x0ffff },
      {   0,   0,   0,      16 }
    },
    {
      { 0x0, 0x0, 0x36fff, 0x079ff },
      {   0,   0,      18,      15 }
    },
    {
      { 0x0, 0x0, 0x0, 0x03dff },
      {   0,   0,   0,      16 }
    }
  },
  {
    { /* lav1 */
      {
        { 0x0000, 0x0006, 0x0007, 0x0002, },
        {      1,      3,      3,      2, }
      },
      {
        { 0x0000, 0x0006, 0x0007, 0x0002, },
        {      1,      3,      3,      2, }
      },
      {
        { 0x0000, 0x0006, 0x0007, 0x0002, },
        {      1,      3,      3,      2, }
      },
      {
        { 0x0000, 0x0006, 0x0007, 0x0002, },
        {      1,      3,      3,      2, }
      }
    },
    { /* lav3 */
      {
        { 0x0002, 0x0000, 0x000a, 0x007e, 0x000e, 0x0004, 0x0016, 0x03fe, 0x01fe, 0x00fe, 0x003e, 0x001e, 0x03ff, 0x0017, 0x0006, 0x0003, },
        {      2,      2,      5,      8,      5,      4,      6,     11,     10,      9,      7,      6,     11,      6,      4,      2, }
      },
      {
        { 0x0002, 0x0004, 0x017e, 0x02fe, 0x0000, 0x000e, 0x00be, 0x0016, 0x000f, 0x0014, 0x005e, 0x0006, 0x002e, 0x02ff, 0x0015, 0x0003, },
        {      2,      4,     10,     11,      2,      5,      9,      6,      5,      6,      8,      4,      7,     11,      6,      2, }
      },
      {
        { 0x0002, 0x000e, 0x037e, 0x0dfe, 0x000f, 0x000c, 0x01ba, 0x01bb, 0x00de, 0x00dc, 0x01be, 0x001a, 0x06fe, 0x0dff, 0x0036, 0x0000, },
        {      2,      4,     10,     12,      4,      4,      9,      9,      8,      8,      9,      5,     11,     12,      6,      1, }
      },
      {
        { 0x0002, 0x000e, 0x00fc, 0x0fde, 0x000c, 0x000d, 0x01fe, 0x07ee, 0x01fa, 0x01ff, 0x00fe, 0x003e, 0x0fdf, 0x03f6, 0x001e, 0x0000, },
        {      2,      4,      8,     12,      4,      4,      9,     11,      9,      9,      8,      6,     12,     10,      5,      1, }
      }
    },
    { /* lav5 */
      {
        { 0x0000, 0x0002, 0x000c, 0x006a, 0x00dc, 0x06ee, 0x001e, 0x000c, 0x000d, 0x001e, 0x01ae, 0xddff, 0x00de, 0x007e, 0x001f, 0x01be, 0x6efe, 0xddfe, 0x377e, 0x1bbe, 0x0dde, 0x01bf, 0x00d6, 0x0376, 0xddff, 0xddff, 0x01ba, 0x0034, 0x003e, 0x000e, 0xddff, 0x01af, 0x007f, 0x0036, 0x000e, 0x0002, },
        {      2,      3,      5,      7,      8,     11,      5,      4,      5,      6,      9,     16,      8,      7,      6,      9,     15,     16,     14,     13,     12,      9,      8,     10,     16,     16,      9,      6,      6,      5,     16,      9,      7,      6,      4,      2, }
      },
      {
        { 0x0000, 0x001e, 0x03fc, 0xfffa, 0xfff9e, 0xfff9f, 0x0006, 0x0004, 0x00be, 0x7ffe, 0x7ffce, 0x00fe, 0x0006, 0x001e, 0x03fd, 0xfffb, 0x0ffe, 0x003e, 0x000a, 0x007e, 0x1ffe, 0x7fff, 0x005e, 0x000e, 0x001f, 0x03fe, 0x1fff2, 0x0ffc, 0x002e, 0x000e, 0x00bf, 0x3ffe6, 0xfff8, 0x0ffd, 0x0016, 0x0002, },
        {      2,      5,     10,     16,     20,     20,      3,      4,      9,     15,     19,      8,      4,      6,     10,     16,     12,      6,      5,      7,     13,     15,      8,      5,      6,     10,     17,     12,      7,      4,      9,     18,     16,     12,      6,      2, }
      },
      {
        { 0x0000, 0x000c, 0x01b6, 0x1b7c, 0xdbfe, 0x36fff, 0x000e, 0x001e, 0x01be, 0x0dfe, 0x36ffe, 0x036e, 0x006e, 0x00fe, 0x00d8, 0x36fe, 0x06de, 0x00de, 0x01fa, 0x00da, 0x0dff, 0x1b7e, 0x00d9, 0x00ff, 0x03f6, 0x06fe, 0x6dfe, 0x037e, 0x00fc, 0x001a, 0x07ee, 0x1b7fe, 0x1b7d, 0x07ef, 0x003e, 0x0002, },
        {      1,      4,      9,     13,     16,     18,      4,      5,      9,     12,     18,     10,      7,      8,      8,     14,     11,      8,      9,      8,     12,     13,      8,      8,     10,     11,     15,     10,      8,      5,     11,     17,     13,     11,      6,      2, }
      },
      {
        { 0x0000, 0x000e, 0x003a, 0x0676, 0x19fe, 0xcebe, 0x000f, 0x0002, 0x001e, 0x00fe, 0x19d6, 0x675e, 0x003e, 0x0032, 0x0018, 0x033e, 0x0cfe, 0x0677, 0x0674, 0x019c, 0x00ff, 0x003b, 0x001c, 0x007e, 0x33fe, 0x33ff, 0x0cea, 0x0066, 0x001a, 0x0006, 0xcebf, 0x33ae, 0x067e, 0x019e, 0x001b, 0x0002, },
        {      2,      4,      7,     11,     13,     16,      4,      3,      6,      9,     13,     15,      7,      6,      5,     10,     12,     11,     11,      9,      9,      7,      6,      8,     14,     14,     12,      7,      5,      4,     16,     14,     11,      9,      5,      2, }
      }
    },
    { /* lav7 */
      {
        { 0x0000, 0x000c, 0x002e, 0x0044, 0x0086, 0x069e, 0x043e, 0x087a, 0x001e, 0x000e, 0x002a, 0x0046, 0x015e, 0x0047, 0x034a, 0x087b, 0x00d6, 0x0026, 0x002f, 0x00d7, 0x006a, 0x034e, 0x087b, 0x087b, 0x02be, 0x01a6, 0x01be, 0x0012, 0x01bf, 0x087b, 0x087b, 0x087b, 0x087b, 0x087b, 0x087b, 0x087b, 0x0036, 0x00d0, 0x043c, 0x043f, 0x087b, 0x087b, 0x087b, 0x034b, 0x0027, 0x0020, 0x0042, 0x00d1, 0x087b, 0x087b, 0x02bf, 0x00de, 0x00ae, 0x0056, 0x0016, 0x0014, 0x087b, 0x069f, 0x01a4, 0x010e, 0x0045, 0x006e, 0x001f, 0x0001, },
        {      2,      4,      6,      7,      8,     11,     11,     12,      5,      4,      6,      7,      9,      7,     10,     12,      8,      6,      6,      8,      7,     10,     12,     12,     10,      9,      9,      5,      9,     12,     12,     12,     12,     12,     12,     12,      6,      8,     11,     11,     12,     12,     12,     10,      6,      6,      7,      8,     12,     12,     10,      8,      8,      7,      5,      5,     12,     11,      9,      9,      7,      7,      5,      2, }
      },
      {
        { 0x0002, 0x001e, 0x0ffe, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0x0006, 0x0008, 0x07fe, 0xffff, 0xffff, 0xffff, 0xffff, 0x005a, 0x0006, 0x007a, 0x0164, 0x7ffa, 0xffff, 0xffff, 0x1fee, 0x003c, 0x000e, 0x00fe, 0x02ce, 0x02cf, 0x7ffb, 0x1fec, 0x00b0, 0x002e, 0x003e, 0x03fe, 0x0165, 0x7ffc, 0x1fef, 0x07fa, 0x07f8, 0x001f, 0x002f, 0x00f6, 0x1fed, 0xffff, 0x7ffd, 0x0ff2, 0x00b1, 0x000a, 0x0009, 0x0166, 0xffff, 0xffff, 0x7ffe, 0x3ffc, 0x005b, 0x000e, 0x007e, 0xffff, 0xffff, 0xffff, 0xffff, 0x0ff3, 0x00f7, 0x0000, },
        {      2,      6,     12,     16,     16,     16,     16,     16,      3,      5,     11,     16,     16,     16,     16,      8,      4,      7,     10,     15,     16,     16,     13,      6,      5,      8,     11,     11,     15,     13,      9,      7,      6,     10,     10,     15,     13,     11,     11,      6,      7,      8,     13,     16,     15,     12,      9,      5,      5,     10,     16,     16,     15,     14,      8,      4,      7,     16,     16,     16,     16,     12,      8,      2, }
      },
      {
        { 0x0000, 0x000c, 0x07ee, 0x1e7e, 0x3cfe, 0x79ff, 0x79ff, 0x79ff, 0x000e, 0x001a, 0x01e6, 0x1fbe, 0x79fe, 0x79ff, 0x79ff, 0x06fc, 0x006c, 0x00f6, 0x01ba, 0x0dfc, 0x0dfd, 0x79ff, 0x0f3e, 0x01bb, 0x00dc, 0x01fe, 0x036e, 0x03fe, 0x79ff, 0x0fde, 0x01ee, 0x00f2, 0x01fa, 0x03f6, 0x01be, 0x79ff, 0x1fbf, 0x03ce, 0x03ff, 0x00de, 0x0078, 0x00da, 0x79ff, 0x79ff, 0x06fd, 0x036c, 0x01ef, 0x00fe, 0x036f, 0x0dfe, 0x79ff, 0x79ff, 0x79ff, 0x036d, 0x00fc, 0x003e, 0x0dff, 0x79ff, 0x79ff, 0x79ff, 0x79ff, 0x079e, 0x007a, 0x0002, },
        {      1,      4,     11,     13,     14,     15,     15,     15,      4,      5,      9,     13,     15,     15,     15,     11,      7,      8,      9,     12,     12,     15,     12,      9,      8,      9,     10,     10,     15,     12,      9,      8,      9,     10,      9,     15,     13,     10,     10,      8,      7,      8,     15,     15,     11,     10,      9,      8,     10,     12,     15,     15,     15,     10,      8,      6,     12,     15,     15,     15,     15,     11,      7,      2, }
      },
      {
        { 0x0002, 0x0002, 0x00fe, 0x07be, 0x0ffc, 0x0ffd, 0x1efe, 0x3dfe, 0x0004, 0x0000, 0x003c, 0x00f6, 0x01da, 0x03fe, 0x3dfe, 0x3dff, 0x003c, 0x003e, 0x000a, 0x003a, 0x03de, 0x07be, 0x0f7e, 0x1efe, 0x01de, 0x00ec, 0x007e, 0x000c, 0x01ee, 0x0f7e, 0x07fc, 0x3dff, 0x7ffe, 0x03be, 0x00fe, 0x01fe, 0x001a, 0x001c, 0x07fd, 0x0ffe, 0x3dff, 0x03bf, 0x1ffe, 0x03ff, 0x003e, 0x001b, 0x007e, 0x00f6, 0x7fff, 0x3dff, 0x3ffe, 0x01db, 0x00ee, 0x007a, 0x000e, 0x000b, 0x3dff, 0x3dff, 0x03de, 0x01fe, 0x01ee, 0x007a, 0x0006, 0x0003, },
        {      2,      4,      9,     12,     13,     13,     15,     16,      4,      3,      7,     10,     11,     12,     15,     16,      8,      7,      5,      8,     11,     13,     14,     14,     11,     10,      9,      5,     10,     13,     12,     15,     16,     12,     10,     10,      6,      7,     12,     13,     16,     12,     14,     12,      8,      6,      8,      9,     16,     16,     15,     11,     10,      8,      5,      5,     16,     16,     12,     11,     11,      9,      5,      2, }
      }
    }
  }
};
