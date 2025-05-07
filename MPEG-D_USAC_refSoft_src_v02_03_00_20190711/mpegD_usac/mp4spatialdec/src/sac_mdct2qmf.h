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





#ifndef SPACE_MDCT2QMF_H_INCLUDED
#define SPACE_MDCT2QMF_H_INCLUDED
#include "sac_dec.h"
#ifdef __cplusplus
extern "C" {
#endif









#define float_FORMAT            ( "%f" )

#define AAC_FRAME_LENGTH        ( 1024 )
#define AAC_SHORT_FRAME_LENGTH  ( 128 )

#define MDCT_LENGTH_LO          ( AAC_FRAME_LENGTH )
#define MDCT_LENGTH_HI          ( 2 * AAC_FRAME_LENGTH )
#define MDCT_LENGTH_SF          ( 3 * AAC_SHORT_FRAME_LENGTH )

#define MAX_UPD_QMF             ( MAX_TIME_SLOTS / 2 )          
#define MAX_QUART_WINDOW        ( MAX_TIME_SLOTS / 4 )

#define MAX_DFT_LENGTH          ( MAX_UPD_QMF / 2 )


#define NR_UPD_QMF              ( 5 )
#define UPD_QMF_15              ( 15 )
#define UPD_QMF_16              ( 16 )      
#define UPD_QMF_18              ( 18 )      
#define UPD_QMF_24              ( 24 )      
#define UPD_QMF_30              ( 30 )      
#define UPD_QMF_32              ( 32 )      


#define NR_WINDOW               ( 4 )


#define SPACE_MDCT2QMF_ERROR_NO_ERROR                  ( 0 )
#define SPACE_MDCT2QMF_ERROR_INVALID_WINDOW_TYPE       ( 1 )
#define SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE        ( 2 )






typedef int SPACE_MDCT2QMF_ERROR;
  
  
  
  
  
SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Create
(
	spatialDec* self
);

SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Destroy
( 
    void
);

SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Process
(
 int                      qmfBands,
 int                      updQMF,
 float        *   const   mdctIn,
 float                    qmfReal[][ MAX_NUM_QMF_BANDS ],
 float                    qmfImag[][ MAX_NUM_QMF_BANDS ],
 BLOCKTYPE        const   windowType,
 int                      qmfGlobalOffset
 );


#ifdef __cplusplus
}
#endif

#endif
