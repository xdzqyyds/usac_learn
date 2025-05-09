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
#include "sony_pvcenc.h"
#include "sony_pvclib.h"
#include "sony_pvcparams.h"

/*---> Proto types of local functions <---*/
static PVC_RESULT       set_pvc_mode_param(PVC_PARAM *p_pvcParam);
static void             pvc_sb_grouping_ref(PVC_PARAM *p_pvcParam, unsigned char bnd_bgn, unsigned char bnd_end, float *p_qmfl, float *p_Esg); 
static float            calc_res(float *a_ref, float *a_prd, unsigned char nbHigh);
static void             pvc_packing(PVC_PARAM *p_pvcParam, unsigned char header_flg, 
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
                                    int const bUsacIndependenceFlag,
#endif
                                    PVC_BSINFO *p_pvcBsinfo, unsigned int *p_bit_count);


/******************************************************************************
    API Functions
******************************************************************************/

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

unsigned int pvc_get_handle_size(void) {

    return sizeof(PVC_DATA_STRUCT);

}

/*---> Initialize / Reset internal state <---*/
PVC_RESULT pvc_init_encode(HANDLE_PVC_DATA hPVC) {

    /* check arguments */
    if(hPVC == (HANDLE_PVC_DATA)NULL){
        return -1;
    }

    /* initialize pvcParam */
    hPVC->pvcParam.pvcID_prev       = 0xFF;
    hPVC->pvcParam.bnd_bgn_prev     = 0xFF;
    hPVC->pvcParam.pvc_flg_prev     = 0;
    hPVC->pvcParam.pvc_rate_prev    = 0xFF;

    return 0;

}

/*---> PVC encode processing <---*/
PVC_RESULT pvc_encode_frame(HANDLE_PVC_DATA hPVC, 
                            unsigned char   pvc_mode,
                            unsigned char   pvc_rate,
                            unsigned char   header_flg,
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
							int const bUsacIndependenceFlag,
#endif
                            float           *p_qmfl, 
                            float           *p_qmfh,
                            unsigned char   bnd_bgn,
                            unsigned char   bnd_end,
                            unsigned int    *p_bit_count)
{

    int             i, t, ib;
    unsigned short  *p_pvcID;
    PVC_RESULT      ret;
    unsigned char   pvc_flg, res_init_flg;
    float           sumPow, sumPowPrev;
    float           a_SEsg[3], a_EsgHigh[8], a_EsgHighRef[PVC_NTIMESLOT*8];
    unsigned short  pvcID, pvcID_tmp;
    float           res_min;
    float           a_resAll[128];

    hPVC->pvcParam.pvc_rate = pvc_rate;
    hPVC->pvcParam.brMode = pvc_mode;
    if (hPVC->pvcParam.brMode) {
        pvc_flg = 1;
    } else {
        pvc_flg = 0;
    }

    /* PVC encoding process */
    if (pvc_flg) {

        if ((ret = set_pvc_mode_param(&(hPVC->pvcParam))) != 0) {
          return ret;
        } 

        pvc_sb_grouping(&(hPVC->pvcParam), bnd_bgn, p_qmfl, (float *)hPVC->frmInfo.Esg, 0);

        pvc_sb_grouping_ref(&(hPVC->pvcParam), bnd_bgn, bnd_end, p_qmfh, a_EsgHighRef);

        hPVC->pvcParam.nsMode = 0;
        for (t=0; t<PVC_NTIMESLOT; t++) {
            sumPow = 0.0f;
            for (ib=0; ib<hPVC->pvcParam.nbHigh; ib++) {
                sumPow += a_EsgHighRef[t*hPVC->pvcParam.nbHigh+ib];
            }
            if (t && (sumPow > sumPowPrev*PVC_NS_PRED_THRE)) {
                hPVC->pvcParam.nsMode = 1;
            }
            sumPowPrev = sumPow;
        }

        for (t=0; t<PVC_NTIMESLOT; t++) {
            for (ib=0; ib<hPVC->pvcParam.nbHigh; ib++) {
                a_EsgHighRef[t*hPVC->pvcParam.nbHigh+ib] = 10*PVC_LOG10(a_EsgHighRef[t*hPVC->pvcParam.nbHigh+ib]);
            }
        }

        if (hPVC->pvcParam.nsMode == 0) {
            switch (hPVC->pvcParam.brMode) {
                case 1: /* mode 1 */
                    hPVC->pvcParam.ns = 16;
                    hPVC->pvcParam.p_SC = g_a_SC0_mode1;
                    break;
                case 2: /* mode 2 */
                    hPVC->pvcParam.ns = 12;
                    hPVC->pvcParam.p_SC = g_a_SC0_mode2;
                    break;
            }
        } else {
            switch (hPVC->pvcParam.brMode) {
                case 1: /* mode 1 */
                    hPVC->pvcParam.ns = 4;
                    hPVC->pvcParam.p_SC = g_a_SC1_mode1;
                    break;
                case 2: /* mode 2 */
                    hPVC->pvcParam.ns = 3;
                    hPVC->pvcParam.p_SC = g_a_SC1_mode2;
                    break;
            }
        }

        p_pvcID = hPVC->pvcParam.pvcID;
        res_init_flg = 1;
        for (t=0; t<PVC_NTIMESLOT; t++) {

            pvc_time_smooth(&(hPVC->pvcParam), t, &(hPVC->frmInfo), a_SEsg);

            for (pvcID=0; pvcID<hPVC->pvcParam.pvc_nTab2; pvcID++) {
                        
                hPVC->pvcParam.pvcID[t] = pvcID;
                pvc_predict(&(hPVC->pvcParam), t, a_SEsg, a_EsgHigh);

                if (res_init_flg) {
                    a_resAll[pvcID]  = calc_res(&(a_EsgHighRef[t*hPVC->pvcParam.nbHigh]), a_EsgHigh, hPVC->pvcParam.nbHigh);
                } else {
                    a_resAll[pvcID] += calc_res(&(a_EsgHighRef[t*hPVC->pvcParam.nbHigh]), a_EsgHigh, hPVC->pvcParam.nbHigh);
                }

            }

            if ((t&(PVC_NTS_GROUPID-1)) == (PVC_NTS_GROUPID-1)) {

                res_min = PVC_RES_INIT_VAL;
                for (pvcID=0; pvcID<hPVC->pvcParam.pvc_nTab2; pvcID++) {
                    if (a_resAll[pvcID] < res_min) {
                        pvcID_tmp = pvcID;
                        res_min = a_resAll[pvcID];
                    }
                }
                
                for (i=0; i<PVC_NTS_GROUPID; i++) {
                    *(p_pvcID++) = pvcID_tmp;
                }

                res_init_flg = 1;

            } else {

                res_init_flg = 0;

            }
                
        }
    	
        for (t=0; t<16-1; t++) {
            for (ib=0; ib<3; ib++) {
                hPVC->frmInfo.Esg[t][ib] = hPVC->frmInfo.Esg[t+PVC_NTIMESLOT][ib];
            }
        }

        pvc_packing(&(hPVC->pvcParam), header_flg, 
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
			bUsacIndependenceFlag,
#endif
			&(hPVC->pvcBsinfo), p_bit_count);

        hPVC->pvcParam.pvcID_prev = hPVC->pvcParam.pvcID[PVC_NTIMESLOT-1];

    } else {

        *p_bit_count = 0;
        hPVC->pvcParam.pvcID_prev = 0xFF;

    }

    hPVC->pvcParam.bnd_bgn_prev = bnd_bgn;
    hPVC->pvcParam.pvc_flg_prev = pvc_flg;
    hPVC->pvcParam.pvc_rate_prev = pvc_rate;

    return 0;

}


/******************************************************************************
    Local Functions
******************************************************************************/

static PVC_RESULT set_pvc_mode_param(PVC_PARAM *p_pvcParam) {
	
    p_pvcParam->nbLow            = PVC_NBLOW;
	p_pvcParam->pvc_nTab1        = PVC_NTAB1;
    p_pvcParam->pvc_nTab2        = PVC_NTAB2;
    p_pvcParam->pvc_id_nbit      = PVC_ID_NBIT;

    switch (p_pvcParam->brMode) {

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

    return 0;

}

static void pvc_sb_grouping_ref(PVC_PARAM *p_pvcParam, unsigned char bnd_bgn, unsigned char bnd_end, float *p_qmfh, float *p_Esg) {

    int     ksg, t, ib;
    float   Esg_tmp;
	int     sb, eb;
	int     nqmf_hb;

    nqmf_hb = PVC_NQMFBAND;

    if (bnd_end < bnd_bgn+p_pvcParam->nbHigh*p_pvcParam->hbw-1) {
        bnd_end = bnd_bgn+p_pvcParam->nbHigh*p_pvcParam->hbw-1;
    }

    for (t=0; t<PVC_NTIMESLOT; t++) {
        sb = bnd_bgn;
        eb = sb + p_pvcParam->hbw - 1;
        for (ksg=0; ksg<p_pvcParam->nbHigh; ksg++) {
            Esg_tmp = 0.0f;
			for (ib=sb; ib<=eb; ib++) {
				Esg_tmp += p_qmfh[t*nqmf_hb+ib];
	        }
	        Esg_tmp = PVC_DIV(Esg_tmp, (eb-sb+1));

            if (Esg_tmp > PVC_POW_LLIMIT_LIN) {
                p_Esg[t*p_pvcParam->nbHigh+ksg] = Esg_tmp;
            } else {
                p_Esg[t*p_pvcParam->nbHigh+ksg] = PVC_POW_LLIMIT_LIN;
            }   

            sb += p_pvcParam->hbw;
		    if(ksg >= p_pvcParam->nbHigh-2){
			    eb = bnd_end;
		    }else{
                eb = sb + p_pvcParam->hbw - 1;
                if(eb >= bnd_end){
			        eb = bnd_end;
		        }
		    }		
        }
    }

    return;

}


static float calc_res(float *a_ref, float *a_prd, unsigned char nbHigh) {

    float   res;
    int     i;
    float   offset = 0;
    float   error;

    res = 0.0f;
    for (i=0; i<nbHigh; i++) {
        error = offset + a_ref[i] - a_prd[i];
        res += error * error;
    }
    res = PVC_SQRT(res);

    return res;

}

static void pvc_packing(PVC_PARAM *p_pvcParam, unsigned char header_flg, 
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
						int const bUsacIndependenceFlag,
#endif
						PVC_BSINFO *p_pvcBsinfo, unsigned int *p_bit_count) {

    unsigned short  *p_pvcID_bs;
    unsigned int    bit_count;

    p_pvcID_bs = p_pvcBsinfo->pvcID_bs;

    bit_count = 0;

    bit_count += PVC_NBIT_DIVMODE;

    p_pvcBsinfo->nsMode = p_pvcParam->nsMode;
    bit_count += PVC_NBIT_NSMODE;     
    
    if (   (p_pvcParam->pvcID[0]  == p_pvcParam->pvcID_prev)
        && (header_flg            == 0)
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
        && (bUsacIndependenceFlag == 0)
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */
        ) {

        p_pvcBsinfo->grid_info[0] = 0;
        bit_count += PVC_NBIT_GRID_INFO;

    } else {

        p_pvcBsinfo->grid_info[0] = 1;
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
		if(!bUsacIndependenceFlag){
			bit_count += PVC_NBIT_GRID_INFO;
		}
#else
        bit_count += PVC_NBIT_GRID_INFO;
#endif

        *(p_pvcID_bs++) = p_pvcParam->pvcID[0];
        bit_count += p_pvcParam->pvc_id_nbit;

    }

    if (p_pvcParam->pvcID[8] != p_pvcParam->pvcID[7]) {

        p_pvcBsinfo->divMode = 4;
        p_pvcBsinfo->num_grid_info = 2;

        p_pvcBsinfo->grid_info[1] = 1;
        bit_count += PVC_NBIT_GRID_INFO;

        *(p_pvcID_bs++) = p_pvcParam->pvcID[8];
        bit_count += p_pvcParam->pvc_id_nbit;        

    } else {

        p_pvcBsinfo->divMode = 0;

    }
 
    *p_bit_count = bit_count;

    return ;

}

