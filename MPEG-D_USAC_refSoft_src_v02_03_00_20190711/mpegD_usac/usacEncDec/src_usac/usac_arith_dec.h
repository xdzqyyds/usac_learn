/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Guillaume Fuchs

and edited by:

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

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: usac_arith_dec.h,v 1.8.4.1 2012-04-19 09:15:33 frd Exp $
*******************************************************************************/


#ifndef _usac_arith_dec_h_
#define _usac_arith_dec_h_

#include "bitmux.h"

#ifndef uchar
#define uchar	unsigned char
#endif

/* types */
typedef struct {
  int	a,b;
  int   c, c_prev;
} Tqi2;

/* function prototypes */
void acSpecFrame(HANDLE_USAC_DECODER      hDec,
                 Info*                    info,
                 float*                   coef,
                 int                      max_spec_coefficients,
                 int                      nlong,
                 int                      noise_level,
                 short*                   factors,
                 int                      arithSize,
                 int                     *arithPreviousSize,
                 Tqi2                     arithQ[],
                 HANDLE_BUFFER            hVm,
                 byte                     max_sfb,
                 int                      reset,
                 unsigned int             *nfSeed,
                 int                      bUseNoiseFilling);

int aencSpecFrame(HANDLE_BSBITSTREAM       bs_data,
		  WINDOW_SEQUENCE          windowSequence,
		  int                      nlong,
		  int                      *quantSpectrum,
		  int                      max_spec_coefficients,
		  Tqi2                     arithQ[],
		  int                      *arithPreviousSize,
		  int                      reset);

int tcxArithDec(int                      tcx_size,
                int*                     quant,
                int                      *arithPreviousSize,
                Tqi2                     arithQ[],
                HANDLE_BUFFER            hVm,
                int 			 reset);

int tcxArithEnc(int      tcx_size,
                int      max_tcx_size,
		int      *quantSpectrum,
		Tqi2     arithQ[],
		int      update_flag,
		unsigned char bitBuffer[]);

int tcxArithReset(Tqi2 arithQ[]);

#endif  /* _usac_arith_h_ */
