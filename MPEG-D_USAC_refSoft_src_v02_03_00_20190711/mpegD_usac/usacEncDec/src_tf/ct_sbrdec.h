/****************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

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
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrdec.h,v 1.12.6.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr decoder $Revision: 1.12.6.1 $
*/
#ifndef __CT_SBRDEC_H
#define __CT_SBRDEC_H

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_envcalc.h"

#define MAX_NUM_QMF_BANDS 128


void
sbr_dec_from_mps(int ch, int el,
                 float qmfOutputReal[][MAX_NUM_QMF_BANDS],
                 float qmfOutputImag[][MAX_NUM_QMF_BANDS]);

void sbr_dec ( float* ftimeInPtr,
               float* ftimeOutPtr,
               HANDLE_SBR_FRAME_DATA hFrameData,
               int applyProcessing,
               int channel, int el
#ifdef SBR_SCALABLE
              ,int maxSfbFreqLine
#endif
              ,int bDownSampledSbr
#ifdef PARAMETRICSTEREO
              ,int sbrEnablePS,
               float* ftimeOutPtrPS,
               HANDLE_PS_DEC hParametricStereoDec
#endif
              ,int muxMode
               /* 20060107 */
               ,int	core_bandwidth
               ,int	bUsedBSAC
#ifdef SONY_PVC_DEC
			   ,HANDLE_PVC_DATA	hPvcData
			   ,int sbr_mode
				,int varlen
#endif /* SONY_PVC_DEC */
               );

void
initSbrDec ( int codecSampleRate,
             int bDownSampledSbr,
             int coreCodecFrameSize,
             SBR_RATIO_INDEX sbrRatioIndex
#ifdef AAC_ELD
             ,int ldsbr
#endif
             ,int * bUseHBE
             ,int   bUseHQtransposer
#ifdef HUAWEI_TFPP_DEC
             ,int actATFPP
#endif
             );

void 
deleteSbrDec(void);

void
resetSbrDec ( HANDLE_SBR_FRAME_DATA hFrameData,
              float upSampleFac,
              int channel, int el);

void
sbrDecGetQmfSamples(int numChannels, int el,
                    float ***qmfBufferReal, /* [ch][timeSlots][QMF Bands] */
                    float ***qmfBufferImag  /* [ch][timeSlots][QMF Bands] */
                    );

#endif

