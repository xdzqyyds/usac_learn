/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

and edited by: Jeremie Lecomte
               Stefan  Bayer

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
*******************************************************************************/


#include "interface.h"
#include "cnst.h"


#include "proto_func.h"
#include "table_decl.h"
#include "int3gpp.h"
#include "re8.h"
#include "acelp_plus.h"

#include <math.h>

void decode_fdfac(int *facPrm, int lDiv, int lfac, float *Aq, float *zir, float *facDec) {

  float x[LFAC_1024];
  float xn2[2*LFAC_1024+M];
  float gain;
  int i;
  const float *sineWindow;
  float facWindow[2*LFAC_1024];
  float Ap[M+1];
  HANDLE_TCX_MDCT hTcxMdct=NULL;
  int j = 0;

  if (lfac == 48) {
    sineWindow = sineWindow96;
  }
  else if (lfac == 64) {
    sineWindow = sineWindow128;
  }
  else if (lfac == 96) {
    sineWindow = sineWindow192;
  }
  else {
    sineWindow = sineWindow256;
  }

  if ( Aq != NULL && facDec != NULL ) {
    if(!hTcxMdct) TCX_MDCT_Open(&hTcxMdct);

    /* Build FAC spectrum */
    gain = (float)pow(10.0f, ((float)facPrm[0])/28.0f);
    for ( i = 0 ; i < lfac ; i++ ) {
      x[i] = (float) facPrm[i+1]*gain;
    }

    /* Compute inverse DCT */
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, lfac), x, xn2);

    /* Apply synthesis filter */
    E_LPC_a_weight(Aq, Ap, GAMMA1, M);

    set_zero(xn2+lfac, lfac);
    E_UTIL_synthesis(Ap, xn2, facDec, 2*lfac, xn2+lfac, 0);

    /* add ACELP Zir, if available */
    if ( zir != NULL ) {
      for (i=0; i<lfac; i++)
      {
        facWindow[i] = sineWindow[i]*sineWindow[(2*lfac)-1-i];
        facWindow[lfac+i] = 1.0f - (sineWindow[lfac+i]*sineWindow[lfac+i]);
      }
      for (i=0; i<lfac; i++)
      {
        facDec[i] +=   zir[1+(lDiv/2)+i]*facWindow[lfac+i]
                  + zir[1+(lDiv/2)-1-i]*facWindow[lfac-1-i];
      }
    }
  }

  return;
}
