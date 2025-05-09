/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Takehiro Moriya (NTT) on 1997-08-01,                                    */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Takehiro Moriya (NTT) on 1997-10-14,                                    */
/*   Kazuaki Chikira (NTT) on 2000-08-11,                                    */
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

/* 25-aug-97   NI   bugfixes */

#include <math.h>
#include <stdio.h>

#include "allHandles.h"


/* #include "allHandles.h" */ /* no C++ style comments, please HP20001029 */

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "common_m4a.h"

void     ntt_extend_pitch(
      /* Input */
      int     index_pit[], 
      double  reconst[], 
      int    numChannel,
      int    block_size_samples,
      int    isampf,
      double    bandUpper_d,
      /* Output */
      double  pit_seq[])
{
    /*--- Variables ---*/
    int  bandUpper;
    int    i_smp;
    int pitch_i;
    int tmpnp0_i, tmpnp1_i;
    int npcount, iscount, top, ptop;
    int i_sup, ii,jj;
    int fcmin_i;
    int bandwidth_i;

    bandUpper =  (int)(bandUpper_d*16384.+0.1);
    fcmin_i=
     (int)( (float)block_size_samples/(float)isampf*0.2*1024.0 + 0.5);

    bandwidth_i = 8;
    if ( isampf == 8 ) bandwidth_i = 3;
    if ( isampf >= 11) bandwidth_i = 4;
    if ( isampf >= 22) bandwidth_i = 8;

    for(i_sup=0; i_sup< numChannel; i_sup++){
        top = i_sup * block_size_samples;
        ptop = i_sup * ntt_N_FR_P;
        pitch_i = (int)(pow( 1.009792, (double)index_pit[i_sup] )*4096.0 + 0.5);
        pitch_i *= fcmin_i;
        pitch_i = pitch_i/256  ;
        if (bandwidth_i * bandUpper < 32768 ) {
            tmpnp0_i = pitch_i * 16384 / bandUpper;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i/ block_size_samples;
            npcount = tmpnp0_i / 16384;
        } 
        else {
            tmpnp0_i = pitch_i * bandwidth_i;
            tmpnp1_i = ntt_N_FR_P * tmpnp0_i;
            tmpnp0_i = tmpnp1_i / block_size_samples;
            npcount = tmpnp0_i / 32768;
        }

        iscount=0;
        for (jj=0; jj<block_size_samples; jj++){
          pit_seq[jj+top] = 0.0;
        }
        for (jj=0; jj<npcount/2; jj++){
          pit_seq[jj+top] = reconst[jj+ptop];
          iscount ++;
        }
        for (ii=0; ii<(ntt_N_FR_P )&& (iscount<ntt_N_FR_P); ii++){
          tmpnp0_i = pitch_i*(ii+1);
          tmpnp0_i += 8192;
          i_smp = tmpnp0_i/16384;
          if(i_smp+(npcount-1)/2+1>=block_size_samples) continue;
          for (jj=-npcount/2; jj<(npcount-1)/2+1; jj++){
            pit_seq[i_smp+jj+top] = reconst[iscount+ptop];
            iscount ++;
            if(iscount >= ntt_N_FR_P) break;
          }
        }
    }
}
