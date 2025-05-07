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

#ifndef __SAC_NLC_ENC_H__
#define __SAC_NLC_ENC_H__


#ifdef __cplusplus
extern "C"
{
#endif


#include "sac_stream_enc.h"
#include "sac_types_enc.h"


typedef enum {

  BACKWARDS = 0x0,
  FORWARDS  = 0x1

} DIRECTION;


typedef enum
{

  DIFF_FREQ = 0x0,
  DIFF_TIME = 0x1

} DIFF_TYPE;


typedef enum
{

  HUFF_1D = 0x0,
  HUFF_2D = 0x1

} CODING_SCHEME;

typedef enum
{

  FREQ_PAIR = 0x0,
  TIME_PAIR = 0x1

} PAIRING;

typedef enum
{

  DF_FP = 0x0,
  DF_TP = 0x1,
  DT_FP = 0x2,
  DT_TP = 0x3

} HCOD_TYPE;

#define PAIR_SHIFT   4
#define PAIR_MASK  0xf

#define PBC_MIN_BANDS 5

#define MAXPARAM  5
#define MAXSETS   4
#define MAXBANDS 40



int EcDataPairEnc( Stream*    strm,
                   int        aaInData[][MAXBANDS],
                   int        aHistory[MAXBANDS],
                   DATA_TYPE  data_type,
                   int        setIdx,
                   int        startBand,
                   int        dataBands,
                   int        pair_flag,
                   int        coarse_flag,
                   int        independency_flag);

#ifdef __cplusplus
}
#endif


#endif
