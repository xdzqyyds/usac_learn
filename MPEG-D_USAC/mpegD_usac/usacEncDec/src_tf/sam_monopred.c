/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
Bernd Edler and Hendrik Fuchs, University of Hannover in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.

This software was modified by Y.B. Thomas Kim on 1997-11-06
 *                                                                           *
 ****************************************************************************/

/***********************************************************************************
 * MONOPRED                     *
 *                       *
 *  Contains the core functions for an intra channel (or mono) predictor     *
 *  using a backward adaptive lattice predictor.           *
 *                       *
 *  init_pred_stat():  initialisation of all predictor parameters     *
 *  monopred():    calculation of a predicted value from       *
 *        preceeding (quantised) samples          *
 *  predict():    carry out prediction for all spectral lines     *
 *  predict_reset():  carry out cyclic predictor reset mechanism     *
 *        (long blocks) resp. full reset (short blocks)     *
 *                       *
 *  Internal Functions:                 *
 *    reset_pred_state():  reset the predictor state variables       *
 *                       *
 **********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* HP 980211 */
#include <math.h>

#include "allHandles.h"
#include "block.h"

#include "monopredStruct.h"      /* structs */
#include "sam_tns.h"             /* structs */

#include "sam_dec.h"
#include "monopred.h"

#define  PRED_ORDER  2
#define  PRED_ALPHA  0.90625
#define  PRED_A    0.953125
#define  PRED_B    0.953125

#define  MAX_PRED_BINS  672
#define  LEN_PRED_RSTGRP  5
#define  GRAD  PRED_ORDER

static  float  ALPHA;
static  float  A;
static  float  B;
static  float  mnt_table[128];
static  float  exp_table[256];
static  int    last_rstgrp_num = 0;

static PRED_STATUS  sp_status[2][1024];

static void sam_init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b);
static void sam_inv_quant_pred_state(TMP_PRED_STATUS *tmp_psp, PRED_STATUS *psp);
static void sam_quant_pred_state(PRED_STATUS *psp, TMP_PRED_STATUS *tmp_psp);
static void sam_flt_round(float *pf);
static void sam_inv_table_flt_round(float *pf);
static void sam_make_inv_tables(void);
static void sam_reset_pred_state(PRED_STATUS *psp);

void sam_predinit(void)
{
  int  i, ch;
  for(ch = 0; ch < 2; ch++)
    for(i = 0; i < 1024; i++)
      sam_init_pred_stat(&sp_status[ch][i], PRED_ORDER, PRED_ALPHA, PRED_A, PRED_B);
}

/* this works for all float values, 
 * but does not conform to IEEE conventions of
 * round to nearest, even
 */
static void sam_flt_round(float *pf)
{
    int           flg;
    unsigned long tmp;
    float *pt = (float *)&tmp;
    *pt = *pf;
    flg = tmp & (unsigned long)0x00008000;
    tmp &= (unsigned long)0xffff0000;
    *pf = *pt;
    /* round 1/2 lsb toward infinity */
    if (flg) {
        tmp &= (unsigned long)0xff800000;       /* extract exponent and sign */
        tmp |= (unsigned long)0x00010000;       /* insert 1 lsb */
        *pf += *pt;                     /* add 1 lsb and elided one */
        tmp &= (unsigned long)0xff800000;       /* extract exponent and sign */
        *pf -= *pt;                     /* subtract elided one */
    }
}

/* This only works for 1.0 < float < 2.0 - 2^-24 !
 * 
 * Comparison of the performance of the two rounding routines:
 *		old (above)	new (below)
 * Max error	0.00385171	0.00179992
 * RMS error	0.00194603	0.00109221
 */

/* New bug fixed version */
static void sam_inv_table_flt_round(float *ftmp)
{
  int exp;
  double mnt;
  float descale;

  mnt = frexp((double)*ftmp, &exp);
  descale = (float)ldexp(1.0, exp + 15);
  *ftmp += descale;
  *ftmp -= descale;
}

static void sam_make_inv_tables(void)
{
    int i;
    unsigned long tmp1, tmp;
    float *pf = (float *)&tmp;
    float ftmp;

    *pf = 1.0;
    tmp1 = tmp;        /* float 1.0 */
    for (i=0; i<128; i++) {
      tmp = tmp1 + (i<<16);    /* float 1.m, 7 msb only */
      ftmp = B / *pf;
      sam_inv_table_flt_round(&ftmp);  /* round to 16 bits */
      mnt_table[i] = ftmp;
      /* printf("%3d %08x %f\n", i, tmp, ftmp); */
    }
    for (i=0; i<256; i++) {
      tmp = (i<<23);      /* float 1.0 * 2^exp */
      if (*pf > MINVAR) {
          ftmp = 1.0 / *pf;
      }
      else {
        ftmp = 0;
      }
      exp_table[i] = ftmp;
    }
}


static void sam_inv_quant_pred_state(TMP_PRED_STATUS *tmp_psp, PRED_STATUS *psp)
{
  int i;
  short *p2;
  unsigned long *p1_tmp;

  p1_tmp = (unsigned long *)tmp_psp;
  p2 = (short *)psp;

  for (i=0; i<MAX_PRED_BINS*6; i++)
    p1_tmp[i] = ((unsigned long)p2[i])<<16;
}

#define FAST_QUANT

static void
sam_quant_pred_state(PRED_STATUS *psp, TMP_PRED_STATUS *tmp_psp)
{
  int i;
  short *p1;
  unsigned long *p2_tmp;

#ifdef	FAST_QUANT
  p1 = (short *) psp;
  p2_tmp = (unsigned long *)tmp_psp;

  for (i=0; i<MAX_PRED_BINS*6;i++)
    p1[i] = (short) (p2_tmp[i]>>16);
	    
#else
  for (i=0; i<MAX_PRED_BINS; i++) {
    p1 = (short *) &psp[i];
    p2_tmp = (unsigned long *)tmp_psp;
    {
      int j
      for (j=0; j<6; j++)
        p1[j] = (short) (p2_tmp[i]>>16);
    }
  }
#endif
}

/********************************************************************************
 *** FUNCTION: reset_pred_state()            *
 ***                    *
 ***    reset predictor state variables            *
 ***                    *
 ********************************************************************************/
static void sam_reset_pred_state(PRED_STATUS *psp)
{
    psp->r[0] = Q_ZERO;
    psp->r[1] = Q_ZERO;
    psp->kor[0] = Q_ZERO;
    psp->kor[1] = Q_ZERO;
    psp->var[0] = Q_ONE;
    psp->var[1] = Q_ONE;
}


/********************************************************************************
 *** FUNCTION: init_pred_stat()              *
 ***                    *
 ***    initialisation of all predictor parameter        *
 ***                    *
 ********************************************************************************/
void
sam_init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b)
{
    static int first_time = 1;

    /* Test of parameters */

    if(grad<0 || grad>MAX_PGRAD) {
      fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
      fprintf(stderr,"\nwrong predictor order: %d\n",grad);
      fprintf(stderr,"range of allowed values: 0 ... %d (=MAX_PGRAD)\n\n",MAX_PGRAD);
      exit(1);
    }
    if(alpha<0 || alpha>=1) {
      fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
      fprintf(stderr,"\nwrong time constant alpha: %e\n",alpha);
      fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
      exit(1);
    }
    if(a<0 || a>1) {
      fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
      fprintf(stderr,"\nwrong attenuation factor a: %e\n",a);
      fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
      exit(1);
    }
    if(b<0 || b>1) {
      fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
      fprintf(stderr,"\nwrong attenuation factor b: %e\n",b);
      fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
      exit(1);
    }

    /* Initialisation */
    if (first_time) {
      ALPHA = alpha;
      A = a;
      B = b;
      sam_make_inv_tables();
      first_time = 0;
    }

    sam_reset_pred_state(psp);
}

/********************************************************************************
 *** FUNCTION: monopred()              *
 ***                    *
 ***    calculation of a predicted value from preceeding (quantised) samples  *
 ***  using a second order backward adaptive lattice predictor with full  *
 ***  LMS adaption algorithm for calculation of predictor coefficients  *
 ***                    *
 ***    parameters:  pc:  pointer to this quantised sample    *
 ***      psp:  pointer to structure with predictor status  *
 ***      pred_flag:  1 if prediction is used      *
 ***                    *
 ********************************************************************************/

static void
sam_monopred(Float *pc, PRED_STATUS *psp, TMP_PRED_STATUS *pst, int pred_flag)
{
    float qc = *pc;    /* quantized coef */
    float pv;      /* predicted value */
    float dr1;      /* difference in the R-branch */
    float e0,e1;    /* "partial" prediction errors (E-branch) */
    float r0,r1;    /* content of delay elements */
    float k1,k2;    /* predictor coefficients */

    float *R = pst->r;    /* content of delay elements */
    float *KOR = pst->kor;  /* estimates of correlations */
    float *VAR = pst->var;  /* estimates of variances */
    unsigned long tmp;
    int i, j;

    /* check that enc and dec track */
    r0=R[0];
    r1=R[1];

    /* Calculation of predictor coefficients to be used for the 
     * calculation of the current predicted value based on previous
     * block's state
     */
     
    /* the test, division and rounding is be pre-computed in the tables 
     * equivalent calculation is:
     * k1 = (VAR[1-1]>MINVAR) ? KOR[1-1]/VAR[1-1]*B : 0.0F;
     * k2 = (VAR[2-1]>MINVAR) ? KOR[2-1]/VAR[2-1]*B : 0.0F;
     */
    tmp = psp->var[1-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k1 = KOR[1-1] * exp_table[j] * mnt_table[i];
    
    tmp = psp->var[2-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k2 = KOR[2-1] * exp_table[j] * mnt_table[i];
    
    /* Predicted value */
    pv  = k1*r0 + k2*r1;
    sam_flt_round(&pv);
    
    if (pred_flag)
      *pc = qc + pv;
/* printf("P1: %8.2f %8.2f\n", pv, *pc); */

    /* Calculate state for use in next block */
       
    /* E-Branch:
     *  Calculate the partial prediction errors using the old predictor coefficients
     *  and the old r-values in order to reconstruct the predictor status of the 
     *  previous step
     */

    e0 = *pc;
    e1 = e0-k1*r0;

    /* Difference in the R-Branch:
     *  Calculate the difference in the R-Branch using the old predictor coefficients and
     *  the old partial prediction errors as calculated above in order to reconstruct the
     *  predictor status of the previous step
     */

    dr1 = k1*e0;

    /* Adaption of variances and correlations for predictor coefficients:
     *  These calculations are based on the predictor status of the previous step and give
     *  the new estimates of variances and correlations used for the calculations of the
     *  new predictor coefficients to be used for calculating the current predicted value
     */

    VAR[1-1] = ALPHA*VAR[1-1]+(0.5F)*(r0*r0 + e0*e0);   /* float const */
    KOR[1-1] = ALPHA*KOR[1-1] + r0*e0;
    VAR[2-1] = ALPHA*VAR[2-1]+(0.5F)*(r1*r1 + e1*e1);   /* float const */
    KOR[2-1] = ALPHA*KOR[2-1] + r1*e1;

    /* Summation and delay in the R-Branch => new R-values */

    r1 = A*(r0-dr1);
    r0 = A*e0;

    R[0]=r0;
    R[1]=r1;
}




/********************************************************************************
 *** FUNCTION: predict()              *
 ***                    *
 ***    carry out prediction for all allowed spectral lines      *
 ***                    *
 ********************************************************************************/


void
sam_predict(int windowSequence, int lpflag[], Float coef[], int sfb[], int ch)
{
  int j, k, to, flag0;
  TMP_PRED_STATUS tmp_ps[MAX_PRED_BINS];

  if (windowSequence == 2)
    return;

  sam_inv_quant_pred_state(tmp_ps, sp_status[ch]);
  
  flag0 = lpflag[0];
  k = 0;
  for (j = 0; j < sam_pred_max_bands(); j++) {
    to = sfb[j+1];
    if (flag0 && lpflag[j+7]) {
      for ( ; k < to; k++) {
        sam_monopred(&coef[k], &sp_status[ch][k], &tmp_ps[k], 1);
      }
    }
    else {
      for ( ; k < to; k++) {
        sam_monopred(&coef[k], &sp_status[ch][k], &tmp_ps[k], 0);
      }
    }
  }
  
  sam_quant_pred_state(sp_status[ch], tmp_ps);
}





/********************************************************************************
 *** FUNCTION: predict_reset()              *
 ***                    *
 ***    carry out cyclic predictor reset mechanism (long blocks)    *
 ***    resp. full reset (short blocks)            *
 ***                    *
 ********************************************************************************/
void
sam_predict_reset(int windowSequence, int prstflag[], int ch)
{
  int j, prstflag0, prstgrp;
  static int warn_flag = 1;

  prstgrp = 0;
  if (windowSequence != 2) {
    prstflag0 = prstflag[1];
    if (prstflag0) {
      for (j=LEN_PRED_RSTGRP-1; j>0; j--) {
        prstgrp |= prstflag[j+2];
        prstgrp <<= 1;
      }
      prstgrp |= prstflag[2];
      
      /* check if two consecutive reset group numbers are incremented by one 
         (this is a poor check, but we don't have much alternatives) */
      if ((warn_flag == 1) && (last_rstgrp_num > 0) && (last_rstgrp_num < 30)) {
        if ((prstgrp != last_rstgrp_num) && ((last_rstgrp_num + 1) != prstgrp)) {
          /* fprintf(stderr,"\nWARNING: suspicious Predictor Reset sequence !\n"); */
          warn_flag = 0;
        } 
      }
      last_rstgrp_num = prstgrp;
      
      if ( (prstgrp<1) || (prstgrp>30) ) {
        fprintf(stderr, "ERROR in prediction reset pattern\n");
        return;
      }
      for (j=prstgrp-1; j<1024; j+=30)
        sam_reset_pred_state(&sp_status[ch][j]);
    } /* end predictor reset */
  } /* end islong */
  else { /* short blocks */
    /* complete prediction reset in all bins */
    for (j=0; j<1024; j++)
      sam_reset_pred_state(&sp_status[ch][j]);
  }
}

void
sam_predict_pns_reset1(int maxSfb, int *pns_sfb_flag, int *swb_offset, int ch)
{
  int  i, sfb;

  for(sfb = 0; sfb < maxSfb; sfb++) {
    if(pns_sfb_flag[sfb])
      for(i = swb_offset[sfb]; i < swb_offset[sfb+1]; i++)
        sam_reset_pred_state(&sp_status[ch][i]);
  }
}
