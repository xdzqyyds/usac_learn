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



#ifndef __SAC_NLC_ENC_H__
#define __SAC_NLC_ENC_H__


#ifdef __cplusplus
extern "C"
{
#endif


#include "sac_types.h"
#include "sac_bitinput.h"
#include "sac_dec.h"


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

#define PAIR_SHIFT   4
#define PAIR_MASK  0xf

#define PCM_PLT	   0x2

#define MAXPARAM  MAX_NUM_PARAMS
#define MAXSETS   MAX_PARAMETER_SETS
#define MAXBANDS  MAX_PARAMETER_BANDS


#define HANDLE_HUFF_NODE const int (*)[][2]


int EcDataPairDec(HANDLE_S_BITINPUT strm,
		   int        aaOutData[][MAXBANDS],
		   int        aHistory[MAXBANDS],
		   DATA_TYPE  data_type,
		   int        setIdx,
		   int        startBand,
		   int        dataBands,
		   int        pair_flag,
		   int        coarse_flag,
		   int        independency_flag );



int huff_dec_reshape( HANDLE_S_BITINPUT strm,
                      int*            out_data,
                      int             num_val );


#ifdef __cplusplus
}
#endif



#endif
