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



#ifndef SAC_DECOR_H
#define SAC_DECOR_H


#include "sac_dec.h"

#define ERR_NONE                        ( 0 )
#define ERR_UGLY_ERROR                  ( 1 )

#define MAX_NO_DECORR_CHANNELS          ( 10 )
#define NO_DECORR_BANDS                 (  4 )

#define MAX_NO_TIME_SLOTS_DELAY         ( 14 )

#define DECORR_FILTER_ORDER_BAND_0      ( 10 )
#define DECORR_FILTER_ORDER_BAND_1      (  8 )
#define DECORR_FILTER_ORDER_BAND_2      (  3 )
#define DECORR_FILTER_ORDER_BAND_3      (  2 )

#define MAX_DECORR_FILTER_ORDER         ( DECORR_FILTER_ORDER_BAND_0 )

#define DUCK_ALPHA                      ( 0.8 )
#define DUCK_GAMMA                      ( 1.5 )














int     SpatialDecDecorrelateCreate(    HANDLE_DECORR_DEC *hDecorrDec,
                                                      int  subbands,
                                                      int  seed,
                                                      int  decType,
                                                      int  decorrType,
                                                      int  decorrConfig,
													  int  treeConfig
                                   );

void    SpatialDecDecorrelateDestroy(   HANDLE_DECORR_DEC *hDecorrDec
                                    );

void    SpatialDecDecorrelateApply(     HANDLE_DECORR_DEC hDecorrDec,
                                                    float InputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                                    float InputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                                    float OutputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                                    float OutputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                                      int length
                                  );

#endif 
