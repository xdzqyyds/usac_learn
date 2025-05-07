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
#include "sony_pvcdec.h"
#include "sony_pvcparams.h"

#define V2_2_WARNINGS_ERRORS_FIX

/******************************************************************************
    Definitions
******************************************************************************/

/*---> Library versions <---*/
#define PVC_MAJOR_VERSION     1
#define PVC_MINOR_VERSION     0
#define PVC_BRANCH_VERSION    0

/*---> Proto types of local functions <---*/
static unsigned int        pvc_get_handle_size(void);
static PVC_RESULT          pvc_set_param(PVC_PARAM *p_pvcParam);
static void                pvc_sb_parsing(HANDLE_PVC_DATA hPVC, int t, unsigned char bnd_bgn, float *p_EsgHigh, float *p_qmfh); 



/******************************************************************************
    API Functions
******************************************************************************/

/*---> Obtain the library version <---*/
unsigned int  pvc_get_version(void) {
  return (PVC_MAJOR_VERSION << 16) | (PVC_MINOR_VERSION << 8) | (PVC_BRANCH_VERSION);
}

/*---> Create a PVC handle <---*/
HANDLE_PVC_DATA pvc_get_handle(void) {

  HANDLE_PVC_DATA hPVC;
    
  if ((hPVC = (HANDLE_PVC_DATA)malloc(pvc_get_handle_size())) == (HANDLE_PVC_DATA)NULL) {
    return (HANDLE_PVC_DATA)NULL;
  }

  return hPVC;

}

/*---> Free a PVC handle <---*/
PVC_RESULT pvc_free_handle(HANDLE_PVC_DATA hPVC) {

  /* check arguments */
  if(hPVC == (HANDLE_PVC_DATA)NULL){
    return -1;
  }

  /* free handle */
  free(hPVC);

  return 0;

}

/*---> Initialize / Reset internal state <---*/
PVC_RESULT pvc_init_decode(HANDLE_PVC_DATA hPVC) {


  /* check arguments */
  if(hPVC == (HANDLE_PVC_DATA)NULL){
    return -1;
  }

  /* initialize pvcParam */
  hPVC->pvcParam.bnd_bgn_prev		= 0xFF;
  hPVC->pvcParam.pvc_flg_prev     = 0;
  hPVC->pvcParam.pvc_mode_prev    = 0;
  hPVC->pvcParam.pvc_rate_prev    = 0xFF;

  return 0;

}


/*---> PVC deode processing <---*/
PVC_RESULT pvc_decode_frame(HANDLE_PVC_DATA hPVC,
                            int             pvc_flg,
                            unsigned char   pvc_rate,
                            unsigned char   bnd_bgn,
                            int first_pvc_timeslot,
                            float           *p_qmfl,
                            float           *p_qmfh)
{

  int         t, ib;
  PVC_RESULT  ret;
  float       a_SEsg[3];
  float       a_EsgHigh[8];

  /* check arguments */
  if (hPVC == (HANDLE_PVC_DATA)NULL) {
    return -1;
  }
  if ((pvc_flg != 0) && (pvc_flg != 1)) {
    return -1;
  }
  if ((pvc_rate != 2) && (pvc_rate != 4)) {
    return -1;
  }
  if (bnd_bgn < 0) {
    return -1;
  }
  if (   (p_qmfl == (float *)NULL)  
         || (p_qmfh == (float *)NULL)
         ) {
    return -1;
  }

  hPVC->pvcParam.pvc_rate = pvc_rate;

  /* PVC decoding process */
  if (pvc_flg) {

    if ((ret = pvc_set_param(&(hPVC->pvcParam))) != 0) {
      return ret;
    }       

    /* subband grouping in QMF subbands below SBR range */
    pvc_sb_grouping(&(hPVC->pvcParam), bnd_bgn, p_qmfl, (float *)hPVC->frmInfo.Esg,first_pvc_timeslot);
      
    for (t=0; t<PVC_NTIMESLOT; t++) {
        
      /* Time domain smoothing of subband-grouped energy */
      pvc_time_smooth(&(hPVC->pvcParam), t, &(hPVC->frmInfo), a_SEsg);
        
      /* SBR envelope scalefactor prediction */
      pvc_predict(&(hPVC->pvcParam), t, a_SEsg, a_EsgHigh);
        
      /* subband parsing for QMF subband resolution */
      pvc_sb_parsing(hPVC, t, bnd_bgn, a_EsgHigh, p_qmfh);
    }
    	
    for (t=0; t<16-1; t++) {
      for (ib=0; ib<3; ib++) {
        hPVC->frmInfo.Esg[t][ib] = hPVC->frmInfo.Esg[t+PVC_NTIMESLOT][ib];
      }
    }
      
  } else {
    for (t=0; t<1024; t++) {
      p_qmfh[t] = 0.0f;
    }
  }

  hPVC->pvcParam.bnd_bgn_prev = bnd_bgn;
  hPVC->pvcParam.pvc_flg_prev = (unsigned char)pvc_flg;  
  hPVC->pvcParam.pvc_rate_prev = pvc_rate;

  return 0;
}

/******************************************************************************
    Local Functions
******************************************************************************/
static unsigned int pvc_get_handle_size(void) {

  return sizeof(PVC_DATA_STRUCT);

}

PVC_RESULT pvc_set_param(PVC_PARAM *p_pvcParam) {

  /* check arguments */
  if ((p_pvcParam->brMode != 1) && (p_pvcParam->brMode != 2)) {
    return -1;
  }
  if ((p_pvcParam->nsMode != 0) && (p_pvcParam->nsMode != 1)) {
    return -1;
  }

  p_pvcParam->nbLow = PVC_NBLOW;
  p_pvcParam->pvc_nTab1 = PVC_NTAB1;
  p_pvcParam->pvc_nTab2 = PVC_NTAB2;

  switch(p_pvcParam->brMode){
  case 1: /* mode 1 */
    p_pvcParam->nbHigh           = PVC_NBHIGH_MODE1;
    p_pvcParam->hbw              = 8 / p_pvcParam->pvc_rate;
    p_pvcParam->p_pvcTab1        = (unsigned char *)g_3a_pvcTab1_mode1;
    p_pvcParam->p_pvcTab2        = (unsigned char *)g_2a_pvcTab2_mode1;
    p_pvcParam->p_pvcTab1_dp     = g_a_pvcTab1_dp_mode1;
    p_pvcParam->p_scalingCoef    = g_a_scalingCoef_mode1;
    break;

  case 2: /* mode 2 */
    p_pvcParam->nbHigh           = PVC_NBHIGH_MODE2;
    p_pvcParam->hbw              = 12 / p_pvcParam->pvc_rate;
    p_pvcParam->p_pvcTab1        = (unsigned char *)g_3a_pvcTab1_mode2;
    p_pvcParam->p_pvcTab2        = (unsigned char *)g_2a_pvcTab2_mode2;
    p_pvcParam->p_pvcTab1_dp     = g_a_pvcTab1_dp_mode2;            
    p_pvcParam->p_scalingCoef    = g_a_scalingCoef_mode2;
    break;

  default:
    return -1;

  }

  if (p_pvcParam->nsMode) {
    switch(p_pvcParam->brMode) {
    case 1:
      p_pvcParam->ns = 4;
      p_pvcParam->p_SC = g_a_SC1_mode1;
      break;

    case 2:
      p_pvcParam->ns = 3;
      p_pvcParam->p_SC = g_a_SC1_mode2;
      break;

    default:
      return -1;
    }
  } else {
    switch(p_pvcParam->brMode) {
    case 1:
      p_pvcParam->ns = 16;
      p_pvcParam->p_SC = g_a_SC0_mode1;
      break;

    case 2:
      p_pvcParam->ns = 12;
      p_pvcParam->p_SC = g_a_SC0_mode2;
      break;

    default:
      return -1;
    }
  }

  p_pvcParam->pvcID_prev = p_pvcParam->pvcID[PVC_NTIMESLOT-1];
    
  return 0;

}

static void pvc_sb_parsing(HANDLE_PVC_DATA hPVC, int t, unsigned char bnd_bgn, float *p_EsgHigh, float *p_qmfh) {

  int ksg, k;
  int sb, eb;

  sb = bnd_bgn;
  eb = sb + hPVC->pvcParam.hbw - 1;

  for (ksg=0; ksg<hPVC->pvcParam.nbHigh; ksg++) {
    for (k=sb; k<=eb; k++) {
      p_qmfh[t*PVC_NQMFBAND+k] = PVC_POW(10.0f, PVC_DIV(p_EsgHigh[ksg], 10));
    }
    sb += hPVC->pvcParam.hbw;
    if(ksg >= hPVC->pvcParam.nbHigh-2){
      eb = PVC_NQMFBAND-1;
    } else {
      eb = sb + hPVC->pvcParam.hbw - 1;
      if(eb >= PVC_NQMFBAND-1){
        eb = PVC_NQMFBAND-1;
      }
    }		
  }

  return;
}

