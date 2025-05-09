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

 $Id: ct_envdec.h,v 1.2.12.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Envelope decoding $Revision: 1.2.12.1 $
*/
#ifndef __CT_ENVDEC_H
#define __CT_ENVDEC_H

#include "ct_envextr.h"

void
decodeSbrData(float* iEnvelope,
              float* noiseLevel,
              int* domain_vec1,
              int* domain_vec2,
              int* nSfb,
              int* frameInfo,
              int nNfb,
              int ampRes,
              int nScaleFactors,
              int nNoiseFactors,
              int offset,
              int coupling,
              int ch,
              int el);

void
sbr_envelope_unmapping (float* iEnvelopeLeft,
                        float* iEnvelopeRight,
                        float* noiseFloorLeft,
                        float* noiseFloorRight,
                        int nScaleFactors,
                        int nNoiseFactors,
                        int ampRes);

#ifdef SONY_PVC_DEC
void
decodeSbrData_for_pvc(float* noiseLevel, int* domain_vec2,
              int* frameInfo, int nNfb, int nNoiseFactors, int coupling, int ch, int el);

void
sbr_envelope_unmapping_for_pvc (float* iEnvelopeLeft,
                        float* iEnvelopeRight,
                        float* noiseFloorLeft,
                        float* noiseFloorRight,
                        int nScaleFactors,
                        int nNoiseFactors,
                        int ampRes);
#endif /* SONY_PVC_DEC */

#endif
