/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18                                       */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 18-apr-97  NI  merged long, medium, & short procedure into single module */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void ntt_dec_gain(/* Parameters */
		  int    nsf,
		  /* Input */
		  int    index_pow[],
		  int    numChannel,
		  /* Output */
		  double gain[])
{
    int i_sup, isf, iptop;
    double dmu_power, dec_power, g_gain;

    if (nsf==1){
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    dmu_power = index_pow[i_sup] * ntt_STEP +ntt_STEP/2.;
	    dec_power = ntt_mulawinv(dmu_power, ntt_AMP_MAX, ntt_MU);
	    gain[i_sup] = dec_power/ ntt_AMP_NM;
	}
    }
    else{
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    iptop = i_sup * (nsf + 1);
	    dmu_power = index_pow[iptop] * ntt_STEP +ntt_STEP/2.;
	    dec_power = ntt_mulawinv(dmu_power, ntt_AMP_MAX, ntt_MU);
	    g_gain = dec_power/ ntt_AMP_NM;
	    for(isf=0; isf<nsf; isf++){ 
		dmu_power = index_pow[isf+1+iptop] * ntt_SUB_STEP +ntt_SUB_STEP/2.;
		dec_power = ntt_mulawinv(dmu_power, ntt_SUB_AMP_MAX, ntt_MU);
		gain[isf+i_sup*nsf] = g_gain * dec_power/ ntt_SUB_AMP_NM;
	    }
	}
    }
}
