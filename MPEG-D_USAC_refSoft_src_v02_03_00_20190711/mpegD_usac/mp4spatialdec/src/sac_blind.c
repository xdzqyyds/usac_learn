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


#include <math.h>
#include <assert.h>
#include "sac_dec.h"
#include "sac_blind.h"
#include "sac_bitdec.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define HOP_SLOTS                       ( 4 )
#define TIME_CONSTANT                   ( 0.06f )

static const int BlindCLDMesh[31][21]=
{
  {  -9, -9, -7, -3, -2, -1,  0,  0,  1,  1,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  1 },
  { -10, -8, -6, -3, -2, -1,  0,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  7 },
  { -10, -6, -5, -2, -1,  0,  1,  1,  2,  2,  2,  3,  3,  4,  4,  4,  5,  5,  6,  6,  9 },
  {  -7, -5, -4, -2,  0,  1,  1,  2,  2,  3,  3,  4,  4,  4,  5,  5,  5,  5,  6,  7, 10 },
  {  -8, -5, -3,  0,  1,  2,  2,  3,  3,  3,  4,  4,  5,  5,  5,  5,  6,  6,  6,  7, 10 },
  { -10, -4, -4,  1,  2,  2,  3,  4,  4,  4,  4,  5,  5,  6,  6,  6,  6,  6,  7,  8, 10 },
  {   1,  1,  1,  2,  3,  2,  2,  3,  4,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  9, 11 },
  {   3,  3,  2,  4,  2,  2,  3,  3,  4,  5,  6,  6,  7,  7,  7,  7,  7,  8,  8,  9, 11 },
  {   4,  4,  4,  1,  3,  2,  3,  4,  5,  6,  6,  6,  7,  7,  8,  8,  8,  8,  8,  9, 11 },
  {   5,  5,  5,  3,  2,  3,  3,  4,  5,  6,  7,  7,  7,  7,  7,  7,  7,  8,  8,  9, 10 },
  {   5,  5,  4,  4,  5,  3,  3,  5,  5,  6,  7,  7,  8,  8,  8,  8,  8,  8,  9,  9, 11 },
  {   5,  5,  5,  5,  6,  5,  4,  5,  5,  7,  7,  8,  8,  8,  8,  9,  8,  8,  8, 10, 11 },
  {   5,  5,  5,  5,  5,  4,  4,  6,  6,  7,  8,  9,  8,  8,  8,  8,  8,  9,  9, 10, 11 },
  {   5,  5,  5,  5,  5,  4,  4,  6,  6,  9,  8,  8,  8,  9,  9,  9,  9,  9,  9, 10, 11 },
  {   5,  5,  5,  5,  5,  5,  5,  6,  7,  8,  8,  8, 10, 10,  9,  9, 10, 10, 10, 11, 11 },
  {   5,  5,  5,  5,  5,  5,  5,  5,  8,  8,  9,  9,  9, 10, 10, 10,  9,  9, 10, 11, 11 },
  {   5,  5,  5,  5,  5,  5,  6,  7,  8,  8,  9, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11 },
  {   5,  5,  5,  5,  5,  5,  6,  7,  7,  8, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11 },
  {   5,  5,  5,  5,  5,  6,  6,  6,  6,  9, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11 },
  {   5,  5,  5,  5,  6,  6,  6,  7,  8,  9, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11 },
  {   5,  5,  5,  5,  6,  6,  7,  7,  8,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11 },
  {   5,  5,  5,  6,  6,  6,  7,  7,  7,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11 },
  {   5,  5,  5,  6,  6,  6,  7,  7,  8,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   5,  5,  6,  6,  6,  7,  7,  8,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   6,  6,  6,  6,  6,  7,  7,  8,  9,  9, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   6,  6,  6,  6,  7,  7,  8,  8,  9, 10, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   6,  6,  6,  6,  7,  7,  8,  9, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   6,  6,  6,  7,  7,  8,  8,  9, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   6,  6,  6,  7,  7,  8,  8,  9, 10, 10, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10 },
  {   9,  9,  8,  7,  8,  8, 10, 10, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10 },
  {  11, 11,  8,  7,  8,  8, 10, 11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10 }
};

static const int BlindICCMesh[31][21]=
{
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5 },
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 5, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 2, 2, 4, 3, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 2, 2, 2, 3, 2, 3, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 2, 2, 2, 3, 3, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 4, 4 },
  { 2, 2, 3, 3, 3, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4 },
  { 2, 2, 3, 3, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 2, 2, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 2, 2, 2, 3, 3, 2, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5 },
  { 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 5, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 2, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 5, 3, 3, 3, 3, 4, 5, 5, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 }
};

static const int BlindCPC1Mesh[31][21]=
{
  {  8,  9,  8,  7,  7,  6,  6,  6,  6,  6,  5,  4,  4,  3,  3,  2,  1,  0, -1, -1, -5 },
  { 10,  9,  8,  7,  7,  7,  7,  6,  6,  6,  5,  5,  4,  4,  3,  3,  2,  2,  1,  1,  1 },
  {  9,  9,  8,  8,  7,  7,  7,  7,  7,  6,  6,  5,  5,  4,  4,  4,  3,  2,  2,  2,  0 },
  {  9,  7,  6,  6,  7,  7,  7,  7,  7,  7,  6,  6,  5,  5,  4,  4,  3,  3,  3,  3,  2 },
  {  8,  6,  7,  7,  6,  7,  7,  7,  7,  7,  6,  6,  6,  5,  5,  4,  3,  3,  3,  3,  3 },
  { 10,  7,  8,  7,  6,  7,  7,  7,  7,  7,  7,  6,  6,  6,  5,  4,  4,  3,  3,  4,  4 },
  {  7,  7,  7,  6,  7,  7,  7,  8,  7,  7,  7,  6,  6,  6,  5,  4,  4,  4,  4,  5,  4 },
  {  6,  6,  8,  6,  8,  8,  8,  8,  8,  7,  7,  8,  7,  7,  6,  5,  5,  4,  5,  5,  4 },
  {  4,  4,  5,  9,  8,  9,  9,  8,  8,  8,  8,  7,  7,  7,  6,  6,  5,  4,  5,  5,  4 },
  {  3,  3,  7,  6,  9,  9,  9,  9,  8,  8,  8,  8,  7,  7,  5,  5,  5,  5,  5,  5,  5 },
  {  2,  2,  5,  7,  8, 10,  9,  9,  8,  8,  8,  8,  8,  7,  7,  6,  5,  5,  5,  5,  5 },
  {  4,  4,  5,  7,  7,  9,  9,  9,  8,  9,  8,  9,  8,  8,  7,  7,  6,  7,  6,  5,  6 },
  {  5,  5,  6,  7,  8,  9, 10,  8,  9,  9,  9,  9,  7,  7,  7,  6,  6,  6,  6,  5,  6 },
  {  6,  6,  6,  7,  8,  9, 10,  9,  9,  9,  8,  9,  8,  8,  9,  7,  7,  6,  6,  6,  7 },
  {  6,  6,  7,  8,  8,  9,  9,  9,  9,  9,  9,  8,  9,  8,  8,  9,  8,  8,  5,  7,  7 },
  {  7,  7,  7,  8,  9,  9,  9, 10,  9,  9, 10,  9,  9,  9,  8,  8,  8,  8,  8,  8,  7 },
  {  7,  7,  7,  8,  9,  9, 10, 10, 10,  9, 10,  9,  8,  8,  8,  8,  9,  8,  8,  8,  8 },
  {  7,  7,  8,  8,  9,  9, 10, 10, 10, 10,  9, 10,  9,  7,  8,  8,  8,  8,  8,  8,  8 },
  {  8,  8,  8,  8,  9,  9, 10, 10, 10, 10, 10,  9,  9,  8,  8,  8,  8,  8,  9,  9,  8 },
  {  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 10,  9,  9,  9,  8,  8,  8,  8,  9,  8,  8 },
  {  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 10, 10,  9,  9,  8,  8,  8,  8,  8,  8,  8 },
  {  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 10, 10,  9,  9,  9,  8,  8,  8,  8,  8,  8 },
  {  8,  8,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  8,  8,  8,  8,  8 },
  {  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  8,  8,  8,  8 },
  {  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  8,  8,  8,  8 },
  {  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  8,  8,  8 },
  {  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  8,  8 },
  {  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9,  9 },
  {  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9 },
  { 10, 10,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9 },
  { 10, 10,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9 }
};

static const int BlindCPC2Mesh[31][21]=
{
  {  8,  9,  8,  7,  7,  6,  6,  6,  6,  6,  5,  4,  4,  3,  3,  2,  1,  0, -1, -1, -5 },
  {  7,  7,  7,  6,  6,  6,  6,  6,  5,  5,  4,  4,  3,  3,  2,  2,  0,  0,  0,  0,  0 },
  { 10,  6,  6,  6,  5,  5,  5,  5,  5,  5,  4,  4,  3,  3,  2,  2,  2,  1,  1,  0, -3 },
  {  7,  3,  5,  5,  5,  5,  4,  5,  5,  4,  4,  4,  3,  3,  2,  2,  2,  2,  1,  0, -2 },
  {  2,  0,  3,  4,  5,  4,  4,  3,  4,  4,  3,  3,  3,  2,  2,  2,  2,  2,  2,  0, -5 },
  {  0,  2,  2,  3,  2,  4,  3,  1,  2,  2,  3,  2,  2,  2,  2,  2,  2,  2,  1,  0, -6 },
  {  1,  1,  1,  1,  2,  2,  1,  1,  0,  0, -1,  0,  0,  1,  2,  2,  2,  1,  0, -2, -8 },
  {  0,  0,  0, -2, -2,  1,  2,  0,  0, -1, -1, -2, -3, -3, -1,  0,  0,  0, -2, -5, -5 },
  {  2,  2,  5,  3, -1, -1,  0,  0,  0, -1, -2, -2, -1, -1, -3, -2, -3, -1, -1, -3, -5 },
  {  1,  1,  5,  4,  3,  0, -1,  1,  0, -1, -1, -2, -1,  0,  0,  0,  1,  0,  0, -3, -3 },
  {  1,  1,  3,  4,  5,  2, -1, -1,  1,  2,  1,  0,  0,  0,  1,  1,  0,  1, -1, -2, -5 },
  {  2,  2,  3,  4,  4,  1, -1,  2,  3,  3,  2,  1,  1,  0,  2, -1,  0,  3, -1, -3, -5 },
  {  3,  3,  3,  3,  3,  2,  2,  3,  4,  1,  2,  1,  3, -1,  1,  3, -2, -4,  1, -3, -5 },
  {  3,  3,  3,  3,  3,  2,  2,  0,  3,  3,  1,  1, -3,  0, -1, -1, -1,  1,  0, -3, -5 },
  {  3,  3,  3,  3,  3,  2,  2,  2,  0,  7,  2,  3,  0, -3, -4, -4, -4, -6, -5, -6, -5 },
  {  3,  3,  3,  3,  2,  2,  2,  3,  4,  5,  3,  3,  1, -1, -3, -6,  0, -2, -5, -6, -7 },
  {  3,  3,  3,  3,  2,  2,  3,  3,  2,  6,  3,  2,  2,  0, -2, -3, -1, -4, -4, -6, -6 },
  {  3,  3,  3,  3,  2,  3,  3,  3,  3,  0,  0,  0,  1,  3,  0, -2, -2, -3, -6, -7, -6 },
  {  3,  3,  3,  3,  3,  3,  3,  3,  3,  0, -2,  0,  1,  1,  1, -1, -2, -3, -5, -9, -6 },
  {  3,  3,  3,  3,  3,  3,  3,  2,  1, -1,  2,  0,  1,  1,  0,  0, -2, -3, -5, -6, -6 },
  {  3,  3,  3,  3,  3,  3,  2,  2,  1,  1,  1,  1,  1,  1,  0,  0, -1, -3, -4, -6, -6 },
  {  3,  3,  3,  3,  3,  2,  2,  2,  4,  1,  1,  1,  1,  1,  0,  0, -1, -2, -4, -5, -5 },
  {  3,  3,  3,  3,  3,  2,  2,  3,  2,  2,  1,  1,  1,  1,  0,  0, -1, -2, -3, -4, -4 },
  {  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  1,  1,  0,  0, -1, -2, -3, -3, -3 },
  {  3,  3,  3,  3,  2,  2,  2,  2,  2,  3,  4,  2,  1,  1,  0,  0, -1, -2, -2, -3, -3 },
  {  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  1,  2,  2,  1,  1,  0, -1, -1, -2, -2, -2 },
  {  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  1,  1,  2,  1,  1,  0,  0, -1, -2, -2, -2 },
  {  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  0,  1,  1,  1,  1,  0,  0, -1, -1, -2, -2 },
  {  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  0,  0, -1, -1, -1, -1 },
  {  5,  5,  4,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  0,  0, -1, -1, -1 },
  {  8,  8,  4,  2,  2,  2,  2,  3,  3,  4, -3,  3, -1,  1,  1,  1,  0,  0, -1, -1, -1 }
};

static void Signal2Parameters(spatialDec* self, int ps);
static void UpdateDownMixState(spatialDec* self, int offset);




void SpatialDecInitBlind(spatialDec* self)
{
  BLIND_DECODER * blind = &self->blindDecoder;
  int i;

  
  blind->filterCoeff = (float) exp(-self->qmfBands / (TIME_CONSTANT * self->samplingFreq));

  for (i=0; i < MAX_PARAMETER_BANDS; i++) {
    blind->excitation[0][i] = ABS_THR;
    blind->excitation[1][i] = ABS_THR;
    blind->excitation[2][i] = ABS_THR;
  }

  for (i = 0; i < MAX_OUTPUT_CHANNELS; i++) {
    self->tempShapeEnableChannelSTP[i] = 0;
    self->tempShapeEnableChannelGES[i] = 0;
  }
}




void SpatialDecCloseBlind(spatialDec* self)
{
}




void SpatialDecApplyBlind(spatialDec* self)
{
  SPATIAL_BS_FRAME *frame = &(self->bsFrame[self->curFrameBs]);
  int ts;
  int ps;

  assert(self->timeSlots % HOP_SLOTS == 0);

  for (ts = 0, ps = 0; ts < self->timeSlots; ts += HOP_SLOTS, ps++) {
    self->paramSlot[ps] = ts + HOP_SLOTS - 1;

    Signal2Parameters(self, ps);
    UpdateDownMixState(self, ts);
  }

  self->numParameterSetsPrev = ps;
  self->numParameterSets     = ps;
  frame->bsIndependencyFlag  = 0;
  self->numOttBands[0]       = self->numParameterBands;
}




static void Signal2Parameters(spatialDec* self, int ps)
{
  BLIND_DECODER * blind = &self->blindDecoder;
  float *dequantCLD;
  float *dequantICC;
  float *dequantCPC;
  float cld;
  float icc;
  float cldDelta;
  float iccDelta;
  int cldIndex;
  int iccIndex;
  int mesh[2][2];
  int pb;

  SpatialGetDequantTables(&dequantCLD, &dequantICC, &dequantCPC);

  for (pb = 0; pb < self->numParameterBands; pb++) {
    cld = 10.0f * (float)log10(blind->excitation[0][pb] / blind->excitation[1][pb]);
    icc = blind->excitation[2][pb] / (float)sqrt(blind->excitation[0][pb] * blind->excitation[1][pb]);

    cldDelta = min((float)fabs(cld), 30.0f);
    iccDelta = (icc + 1.0f) * 10.0f;

    cldIndex = min((int)floor(cldDelta), 29);
    iccIndex = min((int)floor(iccDelta), 19);

    cldDelta -= cldIndex;
    iccDelta -= iccIndex;

    
    mesh[0][0] = BlindCLDMesh[cldIndex    ][iccIndex    ] + 15;
    mesh[0][1] = BlindCLDMesh[cldIndex    ][iccIndex + 1] + 15;
    mesh[1][0] = BlindCLDMesh[cldIndex + 1][iccIndex    ] + 15;
    mesh[1][1] = BlindCLDMesh[cldIndex + 1][iccIndex + 1] + 15;

    self->ottCLD[0][ps][pb] =                 mesh[0][0]             +
      (             mesh[0][1]              - mesh[0][0]) * iccDelta +
      (                          mesh[1][0] - mesh[0][0]) * cldDelta +
      (mesh[1][1] - mesh[0][1] - mesh[1][0] + mesh[0][0]) * iccDelta * cldDelta;

    self->ottCLD[0][ps][pb] = dequantCLD[(int)floor(self->ottCLD[0][ps][pb] + 0.5f)];

    mesh[0][0] = BlindICCMesh[cldIndex    ][iccIndex    ];
    mesh[0][1] = BlindICCMesh[cldIndex    ][iccIndex + 1];
    mesh[1][0] = BlindICCMesh[cldIndex + 1][iccIndex    ];
    mesh[1][1] = BlindICCMesh[cldIndex + 1][iccIndex + 1];

    self->ottICC[0][ps][pb] =                 mesh[0][0]             +
      (             mesh[0][1]              - mesh[0][0]) * iccDelta +
      (                          mesh[1][0] - mesh[0][0]) * cldDelta +
      (mesh[1][1] - mesh[0][1] - mesh[1][0] + mesh[0][0]) * iccDelta * cldDelta;

    self->ottICC[0][ps][pb] = dequantICC[(int)floor(self->ottICC[0][ps][pb] + 0.5f)];

    mesh[0][0] = BlindCPC1Mesh[cldIndex    ][iccIndex    ] + 20;
    mesh[0][1] = BlindCPC1Mesh[cldIndex    ][iccIndex + 1] + 20;
    mesh[1][0] = BlindCPC1Mesh[cldIndex + 1][iccIndex    ] + 20;
    mesh[1][1] = BlindCPC1Mesh[cldIndex + 1][iccIndex + 1] + 20;

    self->tttCPC1[0][ps][pb] =                mesh[0][0]             +
      (             mesh[0][1]              - mesh[0][0]) * iccDelta +
      (                          mesh[1][0] - mesh[0][0]) * cldDelta +
      (mesh[1][1] - mesh[0][1] - mesh[1][0] + mesh[0][0]) * iccDelta * cldDelta;

    self->tttCPC1[0][ps][pb] = dequantCPC[(int)floor(self->tttCPC1[0][ps][pb] + 0.5f)];

    mesh[0][0] = BlindCPC2Mesh[cldIndex    ][iccIndex    ] + 20;
    mesh[0][1] = BlindCPC2Mesh[cldIndex    ][iccIndex + 1] + 20;
    mesh[1][0] = BlindCPC2Mesh[cldIndex + 1][iccIndex    ] + 20;
    mesh[1][1] = BlindCPC2Mesh[cldIndex + 1][iccIndex + 1] + 20;

    self->tttCPC2[0][ps][pb] =                mesh[0][0]             +
      (             mesh[0][1]              - mesh[0][0]) * iccDelta +
      (                          mesh[1][0] - mesh[0][0]) * cldDelta +
      (mesh[1][1] - mesh[0][1] - mesh[1][0] + mesh[0][0]) * iccDelta * cldDelta;

    self->tttCPC2[0][ps][pb] = dequantCPC[(int)floor(self->tttCPC2[0][ps][pb] + 0.5f)];

    if (cld < 0.0f) {
      
      cld = self->tttCPC2[0][ps][pb];
      self->tttCPC2[0][ps][pb] = self->tttCPC1[0][ps][pb];
      self->tttCPC1[0][ps][pb] = cld;
    }
  }
}




static void UpdateDownMixState(spatialDec* self, int offset)
{
  BLIND_DECODER * blind = &self->blindDecoder;
  float excitation[3][MAX_PARAMETER_BANDS];
  int ts;
  int hb;
  int pb;

  for (ts = offset; ts < offset + HOP_SLOTS; ts++) {
    
    for (pb = 0; pb < self->numParameterBands; pb++) {
      excitation[0][pb] = ABS_THR;
      excitation[1][pb] = ABS_THR;
      excitation[2][pb] = ABS_THR;
    }

    for (hb = 0; hb < self->hybridBands; hb++) {
      pb = self->kernels[hb][0];

      excitation[0][pb] += self->hybInputReal[0][ts][hb] * self->hybInputReal[0][ts][hb] +
                           self->hybInputImag[0][ts][hb] * self->hybInputImag[0][ts][hb];
      excitation[1][pb] += self->hybInputReal[1][ts][hb] * self->hybInputReal[1][ts][hb] +
                           self->hybInputImag[1][ts][hb] * self->hybInputImag[1][ts][hb];
      excitation[2][pb] += self->hybInputReal[0][ts][hb] * self->hybInputReal[1][ts][hb] +
                           self->hybInputImag[0][ts][hb] * self->hybInputImag[1][ts][hb];
    }

    
    for (pb = 0; pb < self->numParameterBands; pb++) {
      blind->excitation[0][pb] *= blind->filterCoeff;
      blind->excitation[1][pb] *= blind->filterCoeff;
      blind->excitation[2][pb] *= blind->filterCoeff;
      blind->excitation[0][pb] += ( 1.0f - blind->filterCoeff ) * excitation[0][pb];
      blind->excitation[1][pb] += ( 1.0f - blind->filterCoeff ) * excitation[1][pb];
      blind->excitation[2][pb] += ( 1.0f - blind->filterCoeff ) * excitation[2][pb];
    }
  }
}

