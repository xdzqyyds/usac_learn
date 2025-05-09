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

#ifndef _SONY_PVCDEC_H
#define _SONY_PVCDEC_H

#include "sony_pvclib.h"
#include "sony_pvcprepro.h"


/*---> Data structure <---*/
typedef struct _pvc_data_struct { 
    PVC_FRAME_INFO  frmInfo;
    PVC_PARAM       pvcParam;
} PVC_DATA_STRUCT;

/* Handle */
typedef struct _pvc_data_struct *HANDLE_PVC_DATA;

/* API Functions */
unsigned int		pvc_get_version(void);
HANDLE_PVC_DATA     pvc_get_handle(void);
PVC_RESULT          pvc_free_handle(HANDLE_PVC_DATA hPVC);
PVC_RESULT          pvc_init_decode(HANDLE_PVC_DATA hPVC);
PVC_RESULT          pvc_decode_frame(HANDLE_PVC_DATA hPVC,
                                     int pvc_flg,
                                     unsigned char pvc_rate,
                                     unsigned char bnd_bgn,
                                     int first_pvc_timelost,
                                     float *a_qmfl,
                                     float *a_qmfh);

#endif /* _SONY_PVCDEC_H */
