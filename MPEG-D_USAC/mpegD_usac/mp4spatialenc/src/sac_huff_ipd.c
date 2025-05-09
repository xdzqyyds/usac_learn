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

const HUFF_IPD_TABLE huffIPDTab =
{
  {
    { 0x00, 0x06, 0x1c, 0x3e, 0x1d, 0x3f, 0x1e, 0x02 },
    {    1,    3,    5,    6,    5,    6,    5,    2 }
  },    
  {
    {
      { 0x0000, 0x0006, 0x001e, 0x003a, 0x003b, 0x001c, 0x001f, 0x0002 },
      {      1,      3,      5,      6,      6,      5,      5,      2 }
    },
    {
      { 0x0000, 0x0002, 0x000e, 0x003e, 0x007e, 0x007f, 0x001e, 0x0006 },
      {      1,      2,      4,      6,      7,      7,      5,      3 }
    }
  },
  { /* escape */
    { 
      { 0x00007, 0x000ff, 0x001bf, 0x0057f },
      {       3,       8,       9,      11 }
    },
    {
      { 0x00007, 0x000ff, 0x001bf, 0x0057f },
      {       3,       8,       9,      11 }
    },
    {
      { 0x00007, 0x000bf, 0x005ff, 0x01fbf },
      {       3,       8,      11,      13 }
    },
    {
      { 0x00007, 0x000bf, 0x005ff, 0x01fbf },
      {       3,       8,      11,      13 }
    }
  },
  {
	{ /* lav1 */
	  {
		{ 0x0000, 0x0007, 0x0006, 0x0002, },
		{      1,      3,      3,      2, }
	  },
	  {
		{ 0x0000, 0x0007, 0x0006, 0x0002, },
		{      1,      3,      3,      2, }
	  },
	  {
		{ 0x0000, 0x0007, 0x0006, 0x0002, },
		{      1,      3,      3,      2, }
	  },
	  {
		{ 0x0000, 0x0007, 0x0006, 0x0002, },
		{      1,      3,      3,      2, }
	  },
	},
	{ /* lav3 */
	  {
		{ 0x0000, 0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x0006, 0x00ff, 0x00fe, 0x007c, 0x007e, 0x000e, 0x0002, 0x007d, 0x00ff, 0x00ff, 0x001e, },
		{      1,      8,      8,      8,      8,      3,      8,      8,      7,      7,      4,      2,      7,      8,      8,      5, }
	  },
	  {
		{ 0x0000, 0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x0006, 0x00ff, 0x00fe, 0x007c, 0x007e, 0x000e, 0x0002, 0x007d, 0x00ff, 0x00ff, 0x001e, },
		{      1,      8,      8,      8,      8,      3,      8,      8,      7,      7,      4,      2,      7,      8,      8,      5, }
	  },
	  {
		{ 0x0000, 0x00bf, 0x00bf, 0x00bf, 0x0016, 0x0006, 0x00bf, 0x005e, 0x002e, 0x000e, 0x000a, 0x000f, 0x00be, 0x00bf, 0x00bf, 0x0004, },
		{      1,      8,      8,      8,      5,      3,      8,      7,      6,      4,      4,      4,      8,      8,      8,      3, }
	  },
	  {
		{ 0x0000, 0x00bf, 0x00bf, 0x00bf, 0x0016, 0x0006, 0x00bf, 0x005e, 0x002e, 0x000e, 0x000a, 0x000f, 0x00be, 0x00bf, 0x00bf, 0x0004, },
		{      1,      8,      8,      8,      5,      3,      8,      7,      6,      4,      4,      4,      8,      8,      8,      3, }
	  },
	},
	{ /* lav5 */
	  {
		{ 0x0000, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x006e, 0x001e, 0x01bf, 0x01bf, 0x01bf, 0x002a, 0x007e, 0x00fe, 0x0036, 0x01bf, 0x0018, 0x0014, 0x002b, 0x002e, 0x001a, 0x0019, 0x003e, 0x01be, 0x00ff, 0x000e, 0x01bf, 0x01bf, 0x0016, 0x002f, 0x00de, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x0004, },
		{      1,      9,      9,      9,      9,      9,      7,      5,      9,      9,      9,      6,      7,      8,      6,      9,      5,      5,      6,      6,      5,      5,      6,      9,      8,      4,      9,      9,      5,      6,      8,      9,      9,      9,      9,      3, }
	  },
	  {
		{ 0x0000, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x006e, 0x001e, 0x01bf, 0x01bf, 0x01bf, 0x002a, 0x007e, 0x00fe, 0x0036, 0x01bf, 0x0018, 0x0014, 0x002b, 0x002e, 0x001a, 0x0019, 0x003e, 0x01be, 0x00ff, 0x000e, 0x01bf, 0x01bf, 0x0016, 0x002f, 0x00de, 0x01bf, 0x01bf, 0x01bf, 0x01bf, 0x0004, },
		{      1,      9,      9,      9,      9,      9,      7,      5,      9,      9,      9,      6,      7,      8,      6,      9,      5,      5,      6,      6,      5,      5,      6,      9,      8,      4,      9,      9,      5,      6,      8,      9,      9,      9,      9,      3, }
	  },
	  {
		{ 0x0000, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x0016, 0x005e, 0x05ff, 0x05ff, 0x05ff, 0x05fe, 0x02fc, 0x00ba, 0x000e, 0x05ff, 0x003e, 0x007e, 0x02fd, 0x02fe, 0x005c, 0x0006, 0x000a, 0x017c, 0x017d, 0x007f, 0x05ff, 0x05ff, 0x00bb, 0x001e, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x0004, },
		{      1,     11,     11,     11,     11,     11,      5,      7,     11,     11,     11,     11,     10,      8,      4,     11,      6,      7,     10,     10,      7,      3,      4,      9,      9,      7,     11,     11,      8,      5,     11,     11,     11,     11,     11,      3, }
	  },
	  {
		{ 0x0000, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x0016, 0x005e, 0x05ff, 0x05ff, 0x05ff, 0x05fe, 0x02fc, 0x00ba, 0x000e, 0x05ff, 0x003e, 0x007e, 0x02fd, 0x02fe, 0x005c, 0x0006, 0x000a, 0x017c, 0x017d, 0x007f, 0x05ff, 0x05ff, 0x00bb, 0x001e, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x05ff, 0x0004, },
		{      1,     11,     11,     11,     11,     11,      5,      7,     11,     11,     11,     11,     10,      8,      4,     11,      6,      7,     10,     10,      7,      3,      4,      9,      9,      7,     11,     11,      8,      5,     11,     11,     11,     11,     11,      3, }
	  },
	},
	{ /* lav7 */
	  {
		{ 0x0000, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x000c, 0x001c, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x001e, 0x007e, 0x00ae, 0x0056, 0x057f, 0x057f, 0x057f, 0x00ee, 0x017e, 0x02be, 0x00ef, 0x001a, 0x001e, 0x057f, 0x001f, 0x001b, 0x00ba, 0x015e, 0x003e, 0x0014, 0x0002, 0x0006, 0x0016, 0x005e, 0x00be, 0x057e, 0x00ec, 0x002a, 0x057f, 0x057f, 0x005c, 0x00bb, 0x007f, 0x017f, 0x00ed, 0x057f, 0x057f, 0x057f, 0x057f, 0x001c, 0x001d, 0x003a, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x0004, },
		{      2,     11,     11,     11,     11,     11,     11,     11,      5,      5,     11,     11,     11,     11,     11,      6,      8,      8,      7,     11,     11,     11,      8,      9,     10,      8,      6,      5,     11,      5,      6,      8,      9,      7,      5,      3,      3,      5,      7,      8,     11,      8,      6,     11,     11,      7,      8,      8,      9,      8,     11,     11,     11,     11,      6,      6,      6,     11,     11,     11,     11,     11,     11,      3, }
	  },
	  {
		{ 0x0000, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x000c, 0x001c, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x001e, 0x007e, 0x00ae, 0x0056, 0x057f, 0x057f, 0x057f, 0x00ee, 0x017e, 0x02be, 0x00ef, 0x001a, 0x001e, 0x057f, 0x001f, 0x001b, 0x00ba, 0x015e, 0x003e, 0x0014, 0x0002, 0x0006, 0x0016, 0x005e, 0x00be, 0x057e, 0x00ec, 0x002a, 0x057f, 0x057f, 0x005c, 0x00bb, 0x007f, 0x017f, 0x00ed, 0x057f, 0x057f, 0x057f, 0x057f, 0x001c, 0x001d, 0x003a, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x057f, 0x0004, },
		{      2,     11,     11,     11,     11,     11,     11,     11,      5,      5,     11,     11,     11,     11,     11,      6,      8,      8,      7,     11,     11,     11,      8,      9,     10,      8,      6,      5,     11,      5,      6,      8,      9,      7,      5,      3,      3,      5,      7,      8,     11,      8,      6,     11,     11,      7,      8,      8,      9,      8,     11,     11,     11,     11,      6,      6,      6,     11,     11,     11,     11,     11,     11,      3, }
	  },
	  {
		{ 0x0000, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x003e, 0x0030, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x0032, 0x019a, 0x01fc, 0x01fe, 0x1fbf, 0x1fbf, 0x1fbf, 0x01f8, 0x01fa, 0x1fbe, 0x067e, 0x019b, 0x0036, 0x1fbf, 0x0062, 0x033c, 0x07fe, 0x0fde, 0x067f, 0x01f9, 0x001a, 0x0002, 0x0063, 0x07ff, 0x07ee, 0x033d, 0x033e, 0x00cc, 0x1fbf, 0x1fbf, 0x00de, 0x03fe, 0x03f6, 0x00df, 0x01fd, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x00ce, 0x006e, 0x001e, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x000e, },
		{      1,     13,     13,     13,     13,     13,     13,     13,      6,      6,     13,     13,     13,     13,     13,      6,      9,      9,      9,     13,     13,     13,      9,      9,     13,     11,      9,      6,     13,      7,     10,     11,     12,     11,      9,      5,      2,      7,     11,     11,     10,     10,      8,     13,     13,      8,     10,     10,      8,      9,     13,     13,     13,     13,      8,      7,      5,     13,     13,     13,     13,     13,     13,      4, }
	  },
	  {
		{ 0x0000, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x003e, 0x0030, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x0032, 0x019a, 0x01fc, 0x01fe, 0x1fbf, 0x1fbf, 0x1fbf, 0x01f8, 0x01fa, 0x1fbe, 0x067e, 0x019b, 0x0036, 0x1fbf, 0x0062, 0x033c, 0x07fe, 0x0fde, 0x067f, 0x01f9, 0x001a, 0x0002, 0x0063, 0x07ff, 0x07ee, 0x033d, 0x033e, 0x00cc, 0x1fbf, 0x1fbf, 0x00de, 0x03fe, 0x03f6, 0x00df, 0x01fd, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x00ce, 0x006e, 0x001e, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x1fbf, 0x000e, },
		{      1,     13,     13,     13,     13,     13,     13,     13,      6,      6,     13,     13,     13,     13,     13,      6,      9,      9,      9,     13,     13,     13,      9,      9,     13,     11,      9,      6,     13,      7,     10,     11,     12,     11,      9,      5,      2,      7,     11,     11,     10,     10,      8,     13,     13,      8,     10,     10,      8,      9,     13,     13,     13,     13,      8,      7,      5,     13,     13,     13,     13,     13,     13,      4, }
	  },
	}
  }
};
