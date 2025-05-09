/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Markus Multrus

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
$Id: usac_tcx_mdct.c,v 1.4 2011-03-07 22:28:11 gournape2 Exp $
*******************************************************************************/

#include <stdlib.h>
#include <math.h>

#include "usac_tcx_mdct.h"
#include "proto_func.h"

#ifdef __RESTRICT
#define restrict _Restrict
#else
#define restrict
#endif

#define addFLOAT addFLOAT_NoOpt
#define multFLOAT multFLOAT_NoOpt
#define smulFLOAT smulFLOAT_NoOpt
#define subFLOAT subFLOAT_NoOpt

#ifndef TCX_MDCT_PI
#define TCX_MDCT_PI   3.14159265358979323846264338327950288
#endif

struct T_TCX_DCT4 {

  int    nLines;
  float *pPreTwiddleReal;
  float *pPreTwiddleImag;
  float *pBufferOdd;
  float *pBufferEven;
  float *pBufferReal;
  float *pBufferImag;
  float *pBufferTmp1;
  float *pBufferTmp2;
  float *pPostTwiddleReal;
  float *pPostTwiddleImag;

};


typedef struct T_TCX_MDCT {

  HANDLE_TCX_DCT4 hDct4_1024;
  HANDLE_TCX_DCT4 hDct4_0768;
  HANDLE_TCX_DCT4 hDct4_0512;
  HANDLE_TCX_DCT4 hDct4_0384;
  HANDLE_TCX_DCT4 hDct4_0256;
  HANDLE_TCX_DCT4 hDct4_0192;
  HANDLE_TCX_DCT4 hDct4_0128;
  HANDLE_TCX_DCT4 hDct4_0096;
  HANDLE_TCX_DCT4 hDct4_0064;
  HANDLE_TCX_DCT4 hDct4_0048;

} TCX_MDCT;

static void addFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n);
static void multFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n);
static void smulFLOAT_NoOpt(float a, const float * restrict X, float * restrict Z, int n);
static void subFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n);

int TCX_MDCT_Open(HANDLE_TCX_MDCT *phTcxMdct){

  TCX_MDCT_ERROR error =  TCX_MDCT_NO_ERROR;

  if(error == TCX_MDCT_NO_ERROR){
    if(NULL == ((*phTcxMdct) = (HANDLE_TCX_MDCT) calloc(1, sizeof( struct T_TCX_MDCT)))){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }
  
  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_1024), 1024)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0768),  768)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0512),  512)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0384),  384)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0256),  256)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0192),  192)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0128),  128)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0096),  96)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0064),  64)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Open(&((*phTcxMdct)->hDct4_0048),  48)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  return error;
}

int TCX_MDCT_Close(HANDLE_TCX_MDCT *phTcxMdct){

  if(phTcxMdct){
    if(*phTcxMdct){
      if((*phTcxMdct)->hDct4_1024){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_1024);
      }
      (*phTcxMdct)->hDct4_1024 = NULL;

      if((*phTcxMdct)->hDct4_0768){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0768);
      }
      (*phTcxMdct)->hDct4_0768 = NULL;

      if((*phTcxMdct)->hDct4_0512){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0512);
      }
      (*phTcxMdct)->hDct4_0512 = NULL;

      if((*phTcxMdct)->hDct4_0384){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0384);
      }
      (*phTcxMdct)->hDct4_0384 = NULL;

      if((*phTcxMdct)->hDct4_0256){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0256);
      }
      (*phTcxMdct)->hDct4_0256 = NULL;

      if((*phTcxMdct)->hDct4_0192){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0192);
      }
      (*phTcxMdct)->hDct4_0192 = NULL;

      if((*phTcxMdct)->hDct4_0128){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0128);
      }
      (*phTcxMdct)->hDct4_0128 = NULL;

      if((*phTcxMdct)->hDct4_0096){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0096);
      }
      (*phTcxMdct)->hDct4_0096 = NULL;

      if((*phTcxMdct)->hDct4_0064){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0064);
      }
      (*phTcxMdct)->hDct4_0064 = NULL;

      if((*phTcxMdct)->hDct4_0048){
        TCX_DCT4_Close(&(*phTcxMdct)->hDct4_0048);
      }
      (*phTcxMdct)->hDct4_0048 = NULL;

      free(*phTcxMdct);
    }
    *phTcxMdct = NULL;
  }


  return TCX_MDCT_NO_ERROR;
}



int 
TCX_MDCT_Apply(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r){

  TCX_MDCT_ERROR error = TCX_MDCT_NO_ERROR;
  int dctLength = l/2 + m + r/2;
  float dctInBuffer[1024] = {0.f};
  HANDLE_TCX_DCT4 hDct = NULL;

  if(error == TCX_MDCT_NO_ERROR){
    switch(dctLength){
    case 1024:
      hDct = hTcxMdct->hDct4_1024;
      break;
    case 768:
      hDct = hTcxMdct->hDct4_0768;
      break;
    case 512:
      hDct = hTcxMdct->hDct4_0512;
      break;
    case 384:
      hDct = hTcxMdct->hDct4_0384;
      break;
    case 256:
      hDct = hTcxMdct->hDct4_0256;
      break;
    case 192:
      hDct = hTcxMdct->hDct4_0192;
      break;
    case 128:
      hDct = hTcxMdct->hDct4_0128;
      break;
    case 96:
      hDct = hTcxMdct->hDct4_0096;
      break;
    case 64:
      hDct = hTcxMdct->hDct4_0064;
      break;
    case 48:
      hDct = hTcxMdct->hDct4_0048;
      break;
    default:
      error = TCX_MDCT_FATAL_ERROR;
      break;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    /* check in buffers (out buffer checked in dct) */
    if(x == NULL){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    int i;

    for(i=0; i<m/2; i++){
      dctInBuffer[m/2 + r/2 + i]           = -1.0f * x [l + m/2 - 1 - i]; /* -b_r */
    }
    for(i=0; i<l/2; i++){
      dctInBuffer[m/2 + r/2 + m/2 + i]     = x[i]  - x [l - 1 - i]; /* a - b_r */
    }
    for(i=0; i<m/2; i++){
      dctInBuffer[m/2 + r/2 - 1 - i]       = -1.0f * x[l + m/2 + i]; /* -c_r */
    }
    for(i=0; i<r/2; i++){
      dctInBuffer[m/2 + r/2 - 1 - m/2 - i] = -1.0f * x[l + m   + i] - x[l + m + r - 1 - i]; /* -c_r - d */
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Apply(hDct, dctInBuffer, y)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  return error;
}



int 
TCX_MDCT_ApplyInverse(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r){

  TCX_MDCT_ERROR error = TCX_MDCT_NO_ERROR;
  int dctLength = l/2 + m + r/2;
  float dctOutBuffer[1024] = {0.f};
  HANDLE_TCX_DCT4 hDct = NULL;

  if(error == TCX_MDCT_NO_ERROR){
    switch(dctLength){
    case 1024:
      hDct = hTcxMdct->hDct4_1024;
      break;
    case 768:
      hDct = hTcxMdct->hDct4_0768;
      break;
    case 512:
      hDct = hTcxMdct->hDct4_0512;
      break;
    case 384:
      hDct = hTcxMdct->hDct4_0384;
      break;
    case 256:
      hDct = hTcxMdct->hDct4_0256;
      break;
    case 192:
      hDct = hTcxMdct->hDct4_0192;
      break;
    case 128:
      hDct = hTcxMdct->hDct4_0128;
      break;
    case 96:
      hDct = hTcxMdct->hDct4_0096;
      break;
    case 64:
      hDct = hTcxMdct->hDct4_0064;
      break;
    case 48:
      hDct = hTcxMdct->hDct4_0048;
      break;
    default:
      error = TCX_MDCT_FATAL_ERROR;
      break;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    if(TCX_DCT_NO_ERROR != TCX_DCT4_Apply(hDct, x, dctOutBuffer)){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    /* check out buffers (in buffer checked in dct) */
    if(y == NULL){
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if(error == TCX_MDCT_NO_ERROR){
    int i;
   
    for(i=0; i<m/2; i++){
      /*       dctInBuffer[dctLength/2 + i]           = -1.0f * x [l + m/2 - 1 - i];  *//* -b_r */
      y[l + m/2 - 1 - i] = -1.0f*dctOutBuffer[m/2 + r/2 + i];
    }
    for(i=0; i<l/2; i++){
      /* dctInBuffer[dctLength/2 + m/2 + i]     = x[i]  - x [l - 1 - i]; */ /* a - b_r */
      y[i]         =         dctOutBuffer[m/2 + r/2 + m/2 + i];
      y[l - 1 - i] = -1.0f * dctOutBuffer[m/2 + r/2 + m/2 + i];
    }
    for(i=0; i<m/2; i++){
      /* dctInBuffer[dctLength/2 - 1 - i]       = -1.0f * x[l + m/2 + i]; */ /* -c_r */
      y[l + m/2 + i] = -1.0f*dctOutBuffer[m/2 + r/2 - 1 - i];
    }
    for(i=0; i<r/2; i++){
      /* dctInBuffer[dctLength/2 - 1 - m/2 - i] = -1.0f * x[l + m   + i] - x[l + m + r - 1 - i]; */ /* -c_r - d */
      y[l + m   + i] = -1.0f*dctOutBuffer[m/2 + r/2 - 1 - m/2 - i];
      y[l + m + r - 1 - i] = -1.0f*dctOutBuffer[m/2 + r/2 - 1 - m/2 - i];
    }
  }


  return error;
}


int
TCX_DCT4_Open(HANDLE_TCX_DCT4 *phDct4, int nLines){

  TCX_DCT_ERROR error = TCX_DCT_NO_ERROR;

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == (*phDct4 = (HANDLE_TCX_DCT4) calloc(1, sizeof(struct T_TCX_DCT4)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(nLines%2 == 0){
      (*phDct4)->nLines = nLines;
    } else {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pPreTwiddleReal = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pPreTwiddleImag = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    int i;

    for(i=0; i<(*phDct4)->nLines/2; i++){
      double bds=TCX_MDCT_PI;
      (*phDct4)->pPreTwiddleReal[i] = (float) cos((-1.0f*i - 0.25f)*TCX_MDCT_PI/((float)(*phDct4)->nLines));
      (*phDct4)->pPreTwiddleImag[i] = (float) sin((-1.0f*i - 0.25f)*TCX_MDCT_PI/((float)(*phDct4)->nLines));
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferOdd = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferEven = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferReal = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferImag = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferTmp1 = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pBufferTmp2 = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pPostTwiddleReal = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    if(NULL == ((*phDct4)->pPostTwiddleImag = (float *) calloc((*phDct4)->nLines/2, sizeof(float)))){
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if(error == TCX_DCT_NO_ERROR){
    int i;

    for(i=0; i<(*phDct4)->nLines/2; i++){
      (*phDct4)->pPostTwiddleReal[i] = (float) cos((-1.0f*i)*TCX_MDCT_PI/((float)(*phDct4)->nLines));
      (*phDct4)->pPostTwiddleImag[i] = (float) sin((-1.0f*i)*TCX_MDCT_PI/((float)(*phDct4)->nLines));
    }
  }

  return error;
}


int
TCX_DCT4_Close(HANDLE_TCX_DCT4 *phDct4){

  if(phDct4){
    if(*phDct4){
      if((*phDct4)->pPreTwiddleReal){
        free((*phDct4)->pPreTwiddleReal);
      }
      (*phDct4)->pPreTwiddleReal = NULL;

      if((*phDct4)->pPreTwiddleImag){
        free((*phDct4)->pPreTwiddleImag);
      }
      (*phDct4)->pPreTwiddleImag = NULL;

      if((*phDct4)->pBufferOdd){
        free((*phDct4)->pBufferOdd);
      }
      (*phDct4)->pBufferOdd = NULL;

      if((*phDct4)->pBufferEven){
        free((*phDct4)->pBufferEven);
      }
      (*phDct4)->pBufferEven = NULL;

      if((*phDct4)->pBufferReal){
        free((*phDct4)->pBufferReal);
      }
      (*phDct4)->pBufferReal = NULL;

      if((*phDct4)->pBufferImag){
        free((*phDct4)->pBufferImag);
      }
      (*phDct4)->pBufferImag = NULL;

      if((*phDct4)->pBufferTmp1){
        free((*phDct4)->pBufferTmp1);
      }
      (*phDct4)->pBufferTmp1 = NULL;

      if((*phDct4)->pBufferTmp2){
        free((*phDct4)->pBufferTmp2);
      }
      (*phDct4)->pBufferTmp2 = NULL;

      if((*phDct4)->pPostTwiddleReal){
        free((*phDct4)->pPostTwiddleReal);
      }
      (*phDct4)->pPostTwiddleReal = NULL;

      if((*phDct4)->pPostTwiddleImag){
        free((*phDct4)->pPostTwiddleImag);
      }
      (*phDct4)->pPostTwiddleImag = NULL;

      free(*phDct4);
    }
    *phDct4 = NULL;
  }

  return TCX_DCT_NO_ERROR;
}


int
TCX_DCT4_Apply(HANDLE_TCX_DCT4 hDct4, float *pIn, float *pOut){

  TCX_DCT_ERROR error = TCX_DCT_NO_ERROR;

  if(hDct4){
    /* check in buffer */
    if(error == TCX_DCT_NO_ERROR){
      if(pIn == NULL){
        error = TCX_DCT_FATAL_ERROR;
      }
    }

    /* resorting */
    if(error == TCX_DCT_NO_ERROR){
      int i;

      for(i=0; i<hDct4->nLines/2; i++){
        hDct4->pBufferEven[i] = pIn[2*i];
        hDct4->pBufferOdd[i] = pIn[hDct4->nLines - 1 -2*i];
      }
    }

    /* pre-twiddling */
    if(error == TCX_DCT_NO_ERROR){
      multFLOAT(hDct4->pBufferEven, hDct4->pPreTwiddleReal, hDct4->pBufferTmp1, hDct4->nLines/2);
      multFLOAT(hDct4->pBufferOdd,  hDct4->pPreTwiddleImag, hDct4->pBufferTmp2, hDct4->nLines/2);
      subFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferReal, hDct4->nLines/2);

      multFLOAT(hDct4->pBufferEven, hDct4->pPreTwiddleImag, hDct4->pBufferTmp1, hDct4->nLines/2);
      multFLOAT(hDct4->pBufferOdd,  hDct4->pPreTwiddleReal, hDct4->pBufferTmp2, hDct4->nLines/2);
      addFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferImag, hDct4->nLines/2);
    }

    /* FFT */
    if(error == TCX_DCT_NO_ERROR){
      CFFTNRI(hDct4->pBufferReal, hDct4->pBufferImag, hDct4->nLines/2, -1);
    }

    /* post-twiddling */
    if(error == TCX_DCT_NO_ERROR){
      multFLOAT(hDct4->pBufferReal, hDct4->pPostTwiddleReal, hDct4->pBufferTmp1, hDct4->nLines/2);
      multFLOAT(hDct4->pBufferImag, hDct4->pPostTwiddleImag, hDct4->pBufferTmp2, hDct4->nLines/2);
      subFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferEven, hDct4->nLines/2);

      multFLOAT(hDct4->pBufferReal, hDct4->pPostTwiddleImag, hDct4->pBufferTmp1, hDct4->nLines/2);
      multFLOAT(hDct4->pBufferImag, hDct4->pPostTwiddleReal, hDct4->pBufferTmp2, hDct4->nLines/2);
      addFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferOdd, hDct4->nLines/2);
      smulFLOAT(-1.0f, hDct4->pBufferOdd, hDct4->pBufferOdd, hDct4->nLines/2);
    }

    /* check out buffer */
    if(error == TCX_DCT_NO_ERROR){
      if(pOut == NULL){
        error = TCX_DCT_FATAL_ERROR;
      }
    }

    /* resorting */
    if(error == TCX_DCT_NO_ERROR){
      int i;

      for(i=0; i<hDct4->nLines/2; i++){
        pOut[2*i]                     = hDct4->pBufferEven[i];
        pOut[hDct4->nLines - 1 - 2*i] = hDct4->pBufferOdd[i];
      }
    }

  } else {
    error = TCX_DCT_FATAL_ERROR;
  }

  return error;
}

static void addFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n)
{
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] + Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] + Y[i], _b = X[i + 1] + Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}


static void multFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n)
{
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] * Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] * Y[i], _b = X[i + 1] * Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

static void smulFLOAT_NoOpt(float a, const float * restrict X, float * restrict Z, int n)
{
  int i;
  if (n & 1) {
    Z[0] = (float) a *(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float) a * (X[i]), _b = (float) a * (X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

static void subFLOAT_NoOpt(const float * restrict X, const float * restrict Y, float * restrict Z, int n)
{
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] - Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] - Y[i], _b = X[i + 1] - Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

HANDLE_TCX_DCT4
TCX_MDCT_DCT4_GetHandle(HANDLE_TCX_MDCT hTcxMdct, int nCoeff){

  HANDLE_TCX_DCT4 hDct4 = NULL;

  if(hTcxMdct){
    switch(nCoeff){
    case 1024:
      hDct4 = hTcxMdct->hDct4_1024;
      break;
    case  768:
      hDct4 = hTcxMdct->hDct4_0768;
      break;
    case  512:
      hDct4 = hTcxMdct->hDct4_0512;
      break;
    case  384:
      hDct4 = hTcxMdct->hDct4_0384;
      break;
    case  256:
      hDct4 = hTcxMdct->hDct4_0256;
      break;
    case  192:
      hDct4 = hTcxMdct->hDct4_0192;
      break;
    case  128:
      hDct4 = hTcxMdct->hDct4_0128;
      break;
    case  96:
      hDct4 = hTcxMdct->hDct4_0096;
      break;
    case   64:
      hDct4 = hTcxMdct->hDct4_0064;
      break;
    case   48:
      hDct4 = hTcxMdct->hDct4_0048;
      break;
    default:
      break;
    }
  }

  return hDct4;
}
