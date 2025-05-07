/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 *  MPEG-4 Audio Verification Model (LPC-ABS Core)
 *  
 *  Mode Decision Subroutines
 *
 *  Ver1.0  96.12.16    T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lpc_common.h"
#include "nec_abs_const.h"
#include "nec_exc_proto.h"

#define NEC_LAG_RNG     8
#define NEC_LEN_PITCH_ANALYSIS  80

void nec_mode_decision(float    input_signal[],     /* input */
                       float    wn_alpha[],         /* input */
                       float    wd_alpha[],         /* input */
                       long     lpc_order,          /* configuration input */
                       long     frame_size,         /* configuration input */
                       long     sbfrm_size,         /* configuration input */
                       long     lag_idx_cand[],     /* output */
                       long     *vu_flag,           /* output */
                       long     SampleRateMode      /*input */
)
{
   long         i, j, k;
   long         frame, num_sf;
   long         max_lag, lag, ptr;
   float        *buf_wsp;
   float        cWc, cWx, xWx, cr2ar, max_cr2ar;
   float        frms, gain_pitch_p, gain_pitch_total;
   static long  flag=0, int_to_indx[NEC_PITCH_MAX_FRQ16+1];
   static float pmn[NEC_LPC_ORDER_FRQ16], pmd[NEC_LPC_ORDER_FRQ16];
   static float pst_wsp[NEC_PITCH_MAX_FRQ16];
   static long  pitch_min, pitch_max;
   static float lp1;
   static float thr_spurms, thr_gain[3], rms_m01;
   long         len_pitch_ana;
   long         n_pitch;

   num_sf = frame_size/sbfrm_size;
   len_pitch_ana = sbfrm_size;
   if ( sbfrm_size == 60 ) len_pitch_ana = 120;
   if ( len_pitch_ana < (long)NEC_LEN_PITCH_ANALYSIS )
       len_pitch_ana = (long)NEC_LEN_PITCH_ANALYSIS;
   n_pitch = frame_size/len_pitch_ana;
   
   /*------ memory initialization ------*/
   if ( flag == 0 ) {
       flag = 1;
       
       if(fs8kHz==SampleRateMode) {
           pitch_min = NEC_PITCH_MIN;
           pitch_max = NEC_PITCH_MAX;
           
           for ( i = 0; i < lpc_order; i++ ) pmn[i] = pmd[i] = 0.0;
           for ( i = 0; i < pitch_max; i++ ) pst_wsp[i] = 0.0;
           /* INTEGER DELAY TO INDEX 17-144 */
           int_to_indx[0] = 255;
           for (i = 17; i <= 70; i++) {
               int_to_indx[i] = (i - 17) * 3;
           }
           for (i = 71; i <= 89; i++) {
               int_to_indx[i] = 162 + (i - 71) * 2;
           }
           for (i = 90; i <= 144; i++) {
               int_to_indx[i] = 200 + (i - 90);
           }
       }else {
           pitch_min = NEC_PITCH_MIN_FRQ16;
           pitch_max = NEC_PITCH_MAX_FRQ16;
           
           for ( i = 0; i < lpc_order; i++ ) pmn[i] = pmd[i] = 0.0;
           for ( i = 0; i < pitch_max; i++ ) pst_wsp[i] = 0.0;
           /* INTEGER DELAY TO INDEX 20-295 */
           int_to_indx[0] = 511;
           for (i = 20; i <= 91; i++) {
               int_to_indx[i] = (i - 20) * 3;
           }
           for (i = 92; i <= 182; i++) {
               int_to_indx[i] = 216 + (i - 92) * 2;
           }
           for (i = 183; i <= 295; i++) {
               int_to_indx[i] = 398 + (i - 183);
           }
       }
       
       lp1 = 0.75;
       rms_m01 = 250.0;
       thr_spurms = 10.0;
       thr_gain[0] = 9.0;
       thr_gain[1] = 3.5;
       thr_gain[2] = 1.5;
   }
   
   /*------ memory allocation ----------*/
   if ((buf_wsp = (float *)calloc(pitch_max+frame_size, sizeof(float))) == NULL) {
       printf("\n Memory allocation error in nec_mode_decision \n");
       exit(1);
   }
   
   /*------- open-loop pitch search subframe by subframe -------*/
   for ( i = 0; i < pitch_max; i++ ) buf_wsp[i] = pst_wsp[i];
   for ( frame = 0; frame < num_sf; frame++ ) {
       nec_pw_filt(&buf_wsp[pitch_max+frame*sbfrm_size],
           &input_signal[frame*sbfrm_size],
           lpc_order,
           &wd_alpha[frame*lpc_order], &wn_alpha[frame*lpc_order],
           pmn, pmd, sbfrm_size);
   }
   
   gain_pitch_total = 0.0;
   frms = 0.0;
   for ( frame = 0; frame < n_pitch; frame++ ) {
       ptr = pitch_max+len_pitch_ana * frame;
       
       max_cr2ar = 0.0;
       max_lag = pitch_max;
       for ( lag = pitch_max; lag >= pitch_min; lag-- ) {
           cWx = 0.0;
           cWc = 0.0;
           for ( k = 0; k < len_pitch_ana; k++) {
               cWx += buf_wsp[ptr - lag + k] * buf_wsp[ptr + k];
               cWc += buf_wsp[ptr - lag + k] * buf_wsp[ptr - lag + k];
           }
           
           if ( cWc == 0.0 || cWx < 0.0 ) cr2ar = 0.0;
           else cr2ar = cWx*cWx / cWc;
           
           lp1 = 1.0;
           if ( (max_lag-lag) >= pitch_min ) lp1 = 0.75;
           if ( cr2ar > lp1 * max_cr2ar ) {
               max_lag = lag;
               max_cr2ar = cr2ar;
           }
       }
       
       xWx = 0.0;
       for (i = 0; i < len_pitch_ana; i++) {
           xWx += buf_wsp[ptr + i] * buf_wsp[ptr + i];
           frms += input_signal[frame * len_pitch_ana + i]
               * input_signal[frame * len_pitch_ana + i];
       }
       gain_pitch_p = (float)(xWx != 0.0 ? max_cr2ar/xWx : 0.0);
       
       gain_pitch_p = (float)(1.0 - gain_pitch_p);
       if ( gain_pitch_p < 1.0e-4 ) gain_pitch_p = (float)1.0e-4;
       gain_pitch_total -= (float)(10.0 * log10(gain_pitch_p));
       
       for ( j = 0; j < num_sf/n_pitch; j++ ) {
           lag_idx_cand[frame*num_sf/n_pitch+j] = int_to_indx[max_lag];
       }
   }
   
   gain_pitch_total = gain_pitch_total / n_pitch;
   frms = (float)sqrt(frms / frame_size);
   /*----------- U/V DECISION ---------------*/
   if (frms < thr_spurms || gain_pitch_total < thr_gain[2]) {
       if (frms * gain_pitch_total > rms_m01 * thr_gain[2]) {
           *vu_flag = 1;
       } else {
           *vu_flag = 0;
       }
   } else {
       if (thr_gain[2] <= gain_pitch_total && gain_pitch_total < thr_gain[1]) {
           *vu_flag = 1;
       }
       if (thr_gain[1] <= gain_pitch_total && gain_pitch_total < thr_gain[0]) {
           *vu_flag = 2;
       }
       if (thr_gain[0] <= gain_pitch_total) {
           *vu_flag = 3;
       }
   }
   
   /*--- MEMORY UPDATE ---*/
   for ( i = 0; i < pitch_max; i++ )
       pst_wsp[i] = buf_wsp[frame_size+i];
   
   free( buf_wsp );
   
}
