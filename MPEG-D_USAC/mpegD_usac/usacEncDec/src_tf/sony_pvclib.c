/*******************************************************************************
This software module was originally developed by

  Sony Corporation

Initial author: 
Yuki Yamamoto       (Sony Corporation)
Mitsuyuki Hatanaka  (Sony Corporation)
Hiroyuki Honma      (Sony Corporation)
Toru Chinen         (Sony Corporation)
Masayuki Nishiguchi (Sony Corporation)

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
23003: Sony Corporation grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Sony Corporation
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Sony Corporation 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Sony Corporation retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sony_pvclib.h"
#include "sony_pvcparams.h"

/******************************************************************************
    Functions
******************************************************************************/
void pvc_sb_grouping(PVC_PARAM *p_pvcParam, unsigned char bnd_bgn, float *p_qmfl, float *p_Esg,int first_pvc_timeslot) {
  int     ksg, t, ib;
  float   Esg_tmp;
  int     lbw, sb, eb;
  int     nqmf_lb;
  
  nqmf_lb = PVC_NQMFBAND / p_pvcParam->pvc_rate;
  lbw = 8 / p_pvcParam->pvc_rate;
    

  for (t=0; t<PVC_NTIMESLOT; t++) {
    for (ksg=0; ksg<p_pvcParam->nbLow; ksg++) {
      sb = bnd_bgn - lbw*p_pvcParam->nbLow + lbw*ksg;
      eb = sb + lbw - 1;
      if(sb >= 0){
        Esg_tmp = 0.0f;
        for (ib=sb; ib<=eb; ib++) {
          Esg_tmp += p_qmfl[t*nqmf_lb+ib];
        }
        Esg_tmp = PVC_DIV(Esg_tmp, lbw);
      }else{
        /* no grouped core band */
        Esg_tmp = PVC_POW_LLIMIT_LIN;
      }
                
      if (Esg_tmp > PVC_POW_LLIMIT_LIN) {
        p_Esg[(t+16-1)*3+ksg] = 10*PVC_LOG10(Esg_tmp);
      } else {
        p_Esg[(t+16-1)*3+ksg] = 10*PVC_LOG10(PVC_POW_LLIMIT_LIN);
      }            
    }
  }
    
  if (   (p_pvcParam->pvc_flg_prev == 0)
         || ((bnd_bgn * p_pvcParam->pvc_rate) != (p_pvcParam->bnd_bgn_prev * p_pvcParam->pvc_rate_prev))) {

    for (t=0; t<16-1+first_pvc_timeslot; t++) {
      for (ksg=0; ksg<p_pvcParam->nbLow; ksg++) {
        p_Esg[t*3+ksg] = p_Esg[(16-1+first_pvc_timeslot)*3+ksg];
      }
    }
  }

  return;

}

void pvc_time_smooth(PVC_PARAM *p_pvcParam, int t, PVC_FRAME_INFO *p_frmInfo, float *p_SEsg) {

  int ksg, ti;

  for (ksg=0; ksg<p_pvcParam->nbLow; ksg++) {
    p_SEsg[ksg] = 0.0f;
    for (ti=0; ti<p_pvcParam->ns; ti++) {
      p_SEsg[ksg] += p_frmInfo->Esg[t+16-1-ti][ksg] * p_pvcParam->p_SC[ti];
    }
  }

  return;

}

void pvc_predict(PVC_PARAM *p_pvcParam, int t, float *p_SEsg, float *p_EsgHigh) {

  int                     ksg, kb;
  int                     pvcTab1ID, pvcTab2ID;
  const unsigned char     *p_tab1, *p_tab2;
  float					valTab1, valTab2;

  /* get pvc coefficient */
  pvcTab2ID = p_pvcParam->pvcID[t];

  if (pvcTab2ID < p_pvcParam->p_pvcTab1_dp[0]) {
    pvcTab1ID = 0;
  } else if (pvcTab2ID < p_pvcParam->p_pvcTab1_dp[1]) {
    pvcTab1ID = 1;
  } else {
    pvcTab1ID = 2;
  }

  p_tab1 = &(p_pvcParam->p_pvcTab1[pvcTab1ID*p_pvcParam->nbLow*p_pvcParam->nbHigh]);
  p_tab2 = &(p_pvcParam->p_pvcTab2[pvcTab2ID*p_pvcParam->nbHigh]);

  /* prediction using pvc coefficient */
  for (ksg = 0; ksg<p_pvcParam->nbHigh; ksg++) {
    valTab2 = (float)(char)(*(p_tab2++)) * p_pvcParam->p_scalingCoef[p_pvcParam->nbLow];
    p_EsgHigh[ksg] = valTab2;
  }
  for (kb=0; kb<p_pvcParam->nbLow; kb++) {
    for (ksg=0; ksg<p_pvcParam->nbHigh; ksg++) { 
      valTab1 = (float)(char)(*(p_tab1++)) * p_pvcParam->p_scalingCoef[kb];
      p_EsgHigh[ksg] += valTab1 * p_SEsg[kb];            
    }
  }

  return;

}



