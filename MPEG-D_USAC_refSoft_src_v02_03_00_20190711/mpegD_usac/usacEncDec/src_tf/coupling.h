/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Eugen Dodenhoeft (Fraunhofer IIS)

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

#ifndef _coupling_h_
#define _coupling_h_

#include "all.h"
#include "tf_mainStruct.h"

#if (CChans > 0)
int     getcc ( MC_Info*       mip,
                Info*          sfbInfo,
                HANDLE_AACDECODER hDec,
                PRED_TYPE      pred_type,
                WINDOW_SEQUENCE*          cc_wnd, 
                Wnd_Shape*     cc_wnd_shape, 
                double**        cc_coef,
                double*         cc_gain[CChans][Chans],
                byte*          cb_map[Chans],
                enum AAC_BIT_STREAM_TYPE bitStreamType,
                QC_MOD_SELECT	qc_select,
		HANDLE_BSBITSTREAM gc_stream,
		HANDLE_BUFFER  hVm,
                HANDLE_BUFFER  hHcrSpecData,
                HANDLE_HCR     hHcrInfo,
                HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                HANDLE_RESILIENCE hResilience,
		HANDLE_CONCEALMENT	hConcealment);

void coupling(Info *info, 
              MC_Info *mip, 
              double **coef, 
              double **cc_coef, 
              double *cc_gain[CChans][Chans], 
              int ch, 
              int cc_dom, 
              WINDOW_SEQUENCE *cc_wnd,
	      byte** cb_map,
	      int cc_ind,
	      int tl_channel);

void ind_coupling(MC_Info *mip, 
                  WINDOW_SEQUENCE *wnd, 
                  Wnd_Shape *wnd_shape,
                  WINDOW_SEQUENCE *cc_wnd, 
                  Wnd_Shape *cc_wnd_shape, 
                  double **cc_coef, 
                  double **cc_state,
		  int blockSize
                  );

#endif

#endif /* _coupling_h_ */
