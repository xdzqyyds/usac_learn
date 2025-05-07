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


                                                                  
#include "sac_huff_nodes.h"                                     


HUFF_PT0_NODES huffPart0Nodes =
{
  { { 2, 1}, { 4, 3}, { 6, 5}, { 8, 7}, {10, 9}, {12,11}, {14,13}, {-8,15}, {-9,16}, {-10,17}, {-18,18}, {-17,-19}, {-16,19}, {-11,-20}, {-15,-21}, {-7,20}, {-22,21}, {-12,-14}, {-13,-23}, {23,22}, {-24,-31}, {-6,24}, {-25,-26}, {26,25}, {-5,-27}, {-28,27}, {-4,28}, {-29,29}, {-1,-30}, {-2,-3} },
  { { 2, 1}, {-5, 3}, {-4,-6}, {-3, 4}, {-2, 5}, {-1, 6}, {-7,-8} },
  { { 2, 1}, { 4, 3}, { 6, 5}, {-15, 7}, {-14,-16}, {-13, 8}, {-12, 9}, {-11,10}, {-10,11}, {-8,-9}, {-17,12}, {14,13}, {-7,15}, {-18,16}, {-6,17}, {-5,18}, {-4,-19}, {-3,19}, {-1,20}, {-2,-20}, {22,21}, {-21,23}, {-22,-26}, {-23,24}, {-24,-25}          }
};

HUFF_PT0_NODES huffPilotNodes =
{
  { { 2, 1}, { 4, 3}, { 6, 5}, { 8, 7}, {10, 9}, {12,11}, {14,13}, {-8,15}, {-9,16}, {-10,17}, {-18,18}, {-17,-19}, {-16,19}, {-11,-20}, {-15,-21}, {-7,20}, {-22,21}, {-12,-14}, {-13,-23}, {23,22}, {-24,-31}, {-6,24}, {-25,-26}, {26,25}, {-5,-27}, {-28,27}, {-4,28}, {-29,29}, {-1,-30}, {-2,-3} },
  { { 2, 1}, {-5, 3}, {-4,-6}, {-3, 4}, {-2, 5}, {-1, 6}, {-7,-8} },
  { { 2, 1}, { 4, 3}, { 6, 5}, {-15, 7}, {-14,-16}, {-13, 8}, {-12, 9}, {-11,10}, {-10,11}, {-8,-9}, {-17,12}, {14,13}, {-7,15}, {-18,16}, {-6,17}, {-5,18}, {-4,-19}, {-3,19}, {-1,20}, {-2,-20}, {22,21}, {-21,23}, {-22,-26}, {-23,24}, {-24,-25}          }
};

HUFF_LAV_NODES huffLavIdxNodes = 
{
  { {-1, 1}, {-2, 2}, {-3,-4} }
};
