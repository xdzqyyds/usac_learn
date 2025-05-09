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
23003: Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
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

#include "vector_ops.h"
#include "proto_func.h"
#include "table_decl.h"
#include "int3gpp.h"
#include "re8.h"
#include "acelp_plus.h"

#include <math.h>
#include <assert.h>

void coder_fdfac(float *orig, int lDiv, int lfac, int lowpassLine, int targetBitrate, float *synth, float *Aq, short serial[], int *nBitsFac)
{
    float xn2[LFAC_1024+M];
    float facDec[2*LFAC_1024];
    float rightFacSpec[LFAC_1024];
    float Ap[M+1];
    float gain;
    float x2[LFAC_1024];
    int param[LFAC_1024+1];
    int i,index;
    int nBitsEnc=0;
    float tmp[512];
    HANDLE_TCX_MDCT hTcxMdct;
    int rightStart = 2*lDiv - lfac;

    *nBitsFac=0;
    TCX_MDCT_Open(&hTcxMdct);
    /* Set past signal to 0 for computation of residual */
    set_zero(xn2,M);

    /* Compute AAC error */
    mvr2r(&orig[rightStart],xn2+M,lfac);
    subr2r(xn2+M,&synth[rightStart],xn2+M,lfac);

    /* Find target signal */
    E_LPC_a_weight(Aq,Ap,GAMMA1,M);
    E_UTIL_residu(Ap,xn2+M,x2,lfac);
    smulr2r( (2.0f/(float)lfac),x2,x2,lfac);

    /* Compute DCT of target signal */
    TCX_DCT4_Apply(TCX_MDCT_DCT4_GetHandle(hTcxMdct, lfac), x2, rightFacSpec);

    for ( i = lowpassLine ; i < lfac ; i++ ) rightFacSpec[i] = 0.0f;
    gain = SQ_gain(rightFacSpec,targetBitrate,lfac);
    index = (int)floor(0.5f + (28.0f * (float)log10(gain)));
    if (index < 0) index = 0;
    if (index > 127) index = 127;
    param[0] = index;
    gain = (float)pow(10.0f, ((float)index)/28.0f);
    for ( i = 0 ; i< lfac ; i++) rightFacSpec[i] /= gain;
    for ( i = 0 ; i< lfac ; i+=8 ) {
      RE8_PPV(&rightFacSpec[i],&param[i+1]);
    }

    int2bin(index,7,serial);
    nBitsEnc += 7;
    nBitsEnc += encode_fac(&param[1], &serial[7], lfac);
    set_zero(tmp,512);
    decode_fdfac(&param[0], lDiv, lfac,  Aq, NULL, facDec);
    *nBitsFac = nBitsEnc;
    for ( i = 0 ; i < lfac ; i++ ) synth[rightStart+i] += facDec[i];

    TCX_MDCT_Close(&hTcxMdct);
}

int FDFac(int sfbOffsets[],
          int sfbActive,
          double *origTimeSig,
          WINDOW_SEQUENCE windowSequence,
          double *synthTime,
          HANDLE_USAC_TD_ENCODER     hAcelp,
          int              lastSubFrameWasAcelp,
          int              nextFrameIsLPD,
          short           *facPrmOut,
          int             *nBitsFac) {


  double leftFacTimeData[2*LFAC_1024+M];
  float  leftFacTimeData_fl[2*LFAC_1024+M];
  float leftFacSpec[LFAC_1024];
  const float *sineWindow;
  double facWindow[2*LFAC_1024];
  int facPrm[LFAC_1024+1];
  int   i;
  HANDLE_TCX_DCT4 hTcxDctIV = NULL;
  short serial[5000];
  float facelp[LFAC_1024];
  float *zir = NULL;
  float *Aq = NULL;
  int index;
  int lowpassLine;

  *nBitsFac = 0;

  /* left FAC, use instantly (ACELP->FD) */
  if ( lastSubFrameWasAcelp ) {
    float tmp[2*LFAC_1024];
    float Ap[M+1];
    float ener, gain;
    int leftStart;
    int lfac;

    lfac = windowSequence == EIGHT_SHORT_SEQUENCE ? (hAcelp->lFrame/16) : (hAcelp->lFrame/8);

    TCX_DCT4_Open(&hTcxDctIV,lfac);
    assert(hTcxDctIV != NULL);

    switch(lfac){
    case 48:
      sineWindow = sineWindow96;
      break;
    case 64:
      sineWindow = sineWindow128;
      break;
    case 96:
      sineWindow = sineWindow192;
      break;
    case 128:
      sineWindow = sineWindow256;
      break;
    default:
      assert(0);
      break;
    }

    for (i=0; i<lfac; i++)
    {
      facWindow[i] = sineWindow[i]*sineWindow[(2*lfac)-1-i];
      facWindow[lfac+i] = 1.0f - (sineWindow[lfac+i]*sineWindow[lfac+i]);
    }
    leftStart = (hAcelp->lFrame/2) - lfac - M;
    for (i=0; i<2*lfac+M; i++){
    	leftFacTimeData[i]=origTimeSig[leftStart+i];
    }

    vsub_db(&leftFacTimeData[lfac+M],&synthTime[leftStart+lfac+M],&leftFacTimeData[lfac+M],1,1,1,lfac);

    zir = acelpGetZir(hAcelp);

    /* past Error */
    for(i=0; i<M; i++){
      leftFacTimeData[lfac+i] = leftFacTimeData[lfac+i] - zir[1+128-M+i];
    }

    /* windowing and folding of ACELP part including the ZIR */
    for (i=0; i<lfac; i++)
    {
      facelp[i] = zir[1+128+i]*facWindow[lfac+i]
                                         + zir[1+128-1-i]*facWindow[lfac-1-i];
    }
    /* gain limitation to avoid spending bits on artefact */

    {
      float tmp;
      ener = 0.0f;
      for (i=0; i<lfac; i++) ener += leftFacTimeData[i+M+lfac]*leftFacTimeData[i+M+lfac];
      ener *= 2.0f;
      tmp = 0.0f;
      for (i=0; i<lfac; i++) tmp += facelp[i]*facelp[i];
      if (tmp > ener) gain = (float)sqrt(ener/tmp);
      else gain = 1.0f;

      /* remove folded ACELP part */
      for (i=0; i<lfac; i++)
      {
        leftFacTimeData[i+M+lfac] -= gain*facelp[i];
      }
    }

    /* conversion to float */
    for(i=0; i<2*lfac+M; i++){
      leftFacTimeData_fl[i] = (float) leftFacTimeData[i];
    }

    /* filter with last LPC from LPD */
    Aq = acelpGetLastLPC(hAcelp);
    Aq += M+1;
    E_LPC_a_weight(Aq, Ap, GAMMA1, M);       /* Wz of old LPC */
    E_UTIL_residu(Ap, leftFacTimeData_fl+M+lfac, tmp+lfac, lfac);
    smulFLOAT( (2.0f/(float)lfac),tmp,tmp,2*lfac);
    set_zero(tmp,lfac);

    TCX_DCT4_Apply(hTcxDctIV,&tmp[lfac],leftFacSpec);

    lowpassLine = (int)((float)sfbOffsets[sfbActive]*(float)lfac/(float)hAcelp->lFrame);
    for ( i = lowpassLine ; i < lfac ; i++) leftFacSpec[i] = 0.0f;

    gain = SQ_gain(leftFacSpec,240,lfac);
    /* quantize gain with step of 0.714 dB */
     index = (int)floor(0.5f + (28.0f * (float)log10(gain)));
     if (index < 0) index = 0;
     if (index > 127) index = 127;
     int2bin(index,7,serial);
     *nBitsFac += 7;
      gain = (float)pow(10.0f, ((float)index)/28.0f);
    for ( i = 0 ; i < lfac ; i++ )
      leftFacSpec[i] /= gain;

    for ( i = 0 ; i< lfac ; i+=8 ) {
      RE8_PPV(&leftFacSpec[i],&facPrm[i]);
    }

    *nBitsFac += encode_fac(facPrm, &serial[7],lfac);

    for(i=0; i<(*nBitsFac+7)/8; i++){
      facPrmOut[i] = (short) (
          (serial[8*i+0] & 0x1)<< 7 |
          (serial[8*i+1] & 0x1) << 6 |
          (serial[8*i+2] & 0x1) << 5 |
          (serial[8*i+3] & 0x1) << 4 |
          (serial[8*i+4] & 0x1) << 3 |
          (serial[8*i+5] & 0x1) << 2 |
          (serial[8*i+6] & 0x1) << 1 |
          (serial[8*i+7] & 0x1) << 0);
    }

    TCX_DCT4_Close(&hTcxDctIV);
  }
  else {
    *nBitsFac = 0;
  }

  /* right FAC , save for possible use in the next frame (FD->ACELP)*/
  if ( nextFrameIsLPD ) {
    int lfac;
    lfac = windowSequence == EIGHT_SHORT_SEQUENCE ? (hAcelp->lFrame/16) : (hAcelp->lFrame/8);
    lowpassLine = (int)((float)sfbOffsets[sfbActive]*(float)lfac/(float)hAcelp->lFrame);

    /* keep decoded frame to initialize LPD buffers */
    acelpUpdatePastFDSynthesis(hAcelp,&synthTime[hAcelp->lFrame-1],origTimeSig+(hAcelp->lFrame)-1, lowpassLine);

  }

  return (*nBitsFac);
}
