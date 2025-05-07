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



#ifndef	_tns_h_
#define	_tns_h_


#include "sac_interface.h"

#define TNS_MAX_BANDS	49
#define TNS_MAX_ORDER	31
#define	TNS_MAX_WIN	8
#define TNS_MAX_FILT	3

typedef struct
{
    int start_band;
    int stop_band;
    int order;
    int direction;
    int coef_compress;
    short coef[TNS_MAX_ORDER];
} TNSfilt;

typedef struct 
{
    int n_filt;
    int coef_res;
    TNSfilt filt[TNS_MAX_FILT];
} TNSinfo;

typedef struct 
{
    int n_subblocks;
    TNSinfo info[TNS_MAX_WIN];
} TNS_frame_info;

void		s_clr_tns( Info *info, TNS_frame_info *tns_frame_info );
int		s_get_tns( Info *info, TNS_frame_info *tns_frame_info );
void		s_print_tns( TNSinfo *tns_info);
void		s_tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order );
void		s_tns_decode_coef( int order, int coef_res, short *coef, Float *a );
void		s_tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong, TNSinfo *tns_info);
void            s_tns_filter_subblock(Float *spec, int nbands, short *sfb_top, int islong,
				    TNSinfo *tns_info);


#endif	
