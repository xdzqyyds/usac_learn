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

#ifndef _SONY_PVCLIB_H
#define _SONY_PVCLIB_H

#include "sony_pvcprepro.h"

/* Result code definition */
typedef int     PVC_RESULT;

/* Definition */
#define PVC_NTIMESLOT           16
#define PVC_NQMFBAND            64
#define PVC_POW_LLIMIT_LIN      0.1f
#define PVC_TAB_WORDLEN         8

#define PVC_NBIT_DIVMODE        3
#define PVC_NBIT_NSMODE         1
#define PVC_NBIT_GRID_INFO      1
#define PVC_NBIT_REUSE_PVCID    1

#define PVC_NBLOW				3
#define PVC_NTAB1				3
#define PVC_NTAB2		   		128
#define PVC_ID_NBIT				7
#define PVC_NBHIGH_MODE1        8
#define PVC_NBHIGH_MODE2        6
#define PVC_ID_NBIT_RESERVED    0

/*---> Data structure <---*/
typedef struct _pvc_frame_info {
    float          Esg[PVC_NTIMESLOT+16-1][3];
} PVC_FRAME_INFO;

typedef struct _pvc_param {
    unsigned char       brMode;
    unsigned char       divMode;
    unsigned char       nsMode;
    unsigned short      pvcID[PVC_NTIMESLOT];
    unsigned short      pvcID_prev;
    unsigned char       ns;
    const float			*p_SC;
    unsigned char       nbLow;
    unsigned char       nbHigh;
    unsigned char       hbw;
    const float         *p_scalingCoef;
    const unsigned char *p_pvcTab1; 
    const unsigned char *p_pvcTab2;    
    const unsigned char *p_pvcTab1_dp;
    unsigned char       pvc_nTab1;
    unsigned char       pvc_nTab2;
    unsigned char       pvc_id_nbit;  
    unsigned char       bnd_bgn_prev;
    unsigned char       pvc_flg_prev;
    unsigned char       pvc_mode_prev;

    unsigned char      pvc_rate;
    unsigned char      pvc_rate_prev;

#ifdef SONY_PVC_DEC
	int					pvcIdLen;
	int					prev_sbr_mode;
	int					prev_terminate_pos;
#endif /* SONY_PVC_DEC */

} PVC_PARAM;

/* Proto types */
void    pvc_sb_grouping(PVC_PARAM *p_pvcParam, unsigned char bnd_bgn, float *p_qmfl, float *p_Esg,int first_pvc_timeslot); 
void    pvc_time_smooth(PVC_PARAM *p_pvcParam, int t, PVC_FRAME_INFO *p_frmInfo, float *p_SEsg);
void    pvc_predict(PVC_PARAM *p_pvcParam, int t, float *p_SEsg, float *p_EsgHigh);
/* Macro functions */
#define PVC_DIV(n, d)       (n / d)
#define PVC_LOG10(x)        ((float)log10(x))
#define PVC_POW(x, y)       ((float)pow(x, y))
#define PVC_SQRT(x)         ((float)sqrt(x))

#endif /* _SONY_PVCLIB_H */

