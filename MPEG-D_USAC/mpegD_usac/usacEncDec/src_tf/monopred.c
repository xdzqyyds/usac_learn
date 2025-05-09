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
 *                                                                           *
 ****************************************************************************/

/* Minor modifications by Pierre Lauber, Fraunhofer Institute Erlangen */
/* and Michael Matejko (Fraunhofer IIS) */

/***********************************************************************************
 * MONOPRED									   *
 *										   *
 *	Contains the core functions for an intra channel (or mono) predictor	   *
 *	using a backward adaptive lattice predictor.				   *
 *										   *
 *	init_pred_stat():	initialisation of all predictor parameters	   *
 *	monopred():		calculation of a predicted value from		   *
 *				preceeding (quantised) samples		 	   *
 *	predict():		carry out prediction for all spectral lines	   *
 *	predict_reset():	carry out cyclic predictor reset mechanism	   *
 *				(long blocks) resp. full reset (short blocks)	   *
 *										   *
 *	Internal Functions:							   *
 *	  reset_pred_state():	reset the predictor state variables		   *
 *										   *
 **********************************************************************************/

#include <math.h>
#include <stdio.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "port.h"
#include "common_m4a.h"
#include "monopred.h"
#include "concealment.h"

#define	Q_ZERO	    0x0000
#define	Q_ONE	    0x3F80


#define GRAD	PRED_ORDER
static	float	ALPHA;
static	float	A;
static	float	B;
static	float	mnt_table[128];
static	float	exp_table[256];


#ifdef	NOT_USED
static void
flt_trunc(float *pf)
{
    ulong tmp;
    float *pt = (float *)&tmp;
    *pt = *pf;
    tmp &= (ulong)0xffff0000;
    *pf = *pt;
}
#endif

/* this works for all float values, 
 * but does not conform to IEEE conventions of
 * round to nearest, even
 */

/* Matejko's bug fix for gcc-4.1.1 */
static void
flt_round(float *pf)
{
     int exp, rounded;
     float mnt;

    /*printf("flt_round(%f)=", *pf);*/

    /* decompose mantissa (0.xxxx) and exponent */
    mnt = frexp(*pf, &exp);

    /* upscale mantissa to 7 bit accuracy (xxxxxxx.mmm)  */
    mnt = ldexp(mnt, 8);

    /* round mantissa (xxxxxxx.0) [towards zero!] */
    rounded = (long) (mnt + ((mnt>0)? 0.5 : -0.5));

    /* compose mantissa and exponent to previous scale */
    *pf = ldexp(rounded, exp-8);

    /*printf(" %f\n", *pf);*/

#ifdef NOT_USED
/* old function which didn't work with gcc 4.1.x */

    /* more stable version for clever compilers like gcc 3.x */
    int flg;
    ulong tmp, tmp1, tmp2;

    tmp = *(ulong*)pf;
    flg = tmp & (ulong)0x00008000;
    tmp &= (ulong)0xffff0000;
    tmp1 = tmp;
    /* round 1/2 lsb toward infinity */
    if (flg) {
      tmp &= (ulong)0xff800000;       /* extract exponent and sign */
      tmp |= (ulong)0x00010000;       /* insert 1 lsb */
      tmp2 = tmp;                             /* add 1 lsb and elided one */
      tmp &= (ulong)0xff800000;       /* extract exponent and sign */
      
      *pf = *(float*)&tmp1+*(float*)&tmp2-*(float*)&tmp;/* subtract elided one */
    } else {
      *pf = *(float*)&tmp;
    }
#endif
}


/* This only works for 1.0 < float < 2.0 - 2^-24 !
 * 
 * Comparison of the performance of the two rounding routines:
 *		old (above)	new (below)
 * Max error	0.00385171	0.00179992
 * RMS error	0.00194603	0.00109221
 */

/* New bug fixed version */
static void
inv_table_flt_round(float *ftmp)
     /* new implementation producing the same results for any optimization-level */
{
  int exp,a;
  float tmp;

  frexp((double)*ftmp, &exp);
  tmp = *ftmp * (1<<(8-exp));
  a = (int)tmp;
  if ((tmp-a) >= 0.5) a++;
  if ((tmp-a) == 0.5) a&=-2;
  *ftmp = (float)a/(1<<(8-exp));
}

/* old implementation */
/*{
  int exp;
  double mnt;
  float descale;

    mnt = frexp((double)*ftmp, &exp);
    descale = (float)ldexp(1.0, exp + 15);
    *ftmp += descale;
---> BUG: on x86 with optimization enabled: doesn't round to float!
    *ftmp -= descale;
}*/

/* Bug-fixed versions for clever compilers/optimizers */
static void
make_inv_tables(void)
{
  int i;
  float ftmp, var;

  for (i=0; i<128; i++) {
    var = 1.0 + ((float)i/(1<<7));      /* float 1.m, 7 msb only */
    ftmp = B / var;
    inv_table_flt_round(&ftmp);         /* round to 16 bits */
    mnt_table[i] = ftmp;
    /*printf("%3d %f\n", i, ftmp);*/
  }

  for (i=0; i<256; i++) {
    var = (float) pow(2,i-127);         /* float 1.0 * 2^exp */
    if (var > MINVAR) {
      ftmp = 1.0 / var;
    } else {
      ftmp = 0;
    }
    exp_table[i] = ftmp;
    /*printf("%3d %g\n", i, ftmp);*/
  }
  
#ifdef NOT_USED
  int i;
  ulong tmp1, tmp;
  float *pf = (float *)&tmp1;
  float ftmp;
  
  *pf = 1.0;
  for (i=0; i<128; i++) {
    tmp = tmp1 + (i<<16);		/* float 1.m, 7 msb only */
    ftmp = B / *(float*)&tmp;
    inv_table_flt_round(&ftmp);	/* round to 16 bits */
    mnt_table[i] = ftmp;
    /*printf("%3d %08x %f\n", i, tmp, ftmp);*/
  }
  for (i=0; i<256; i++) {
    tmp = (i<<23);			/* float 1.0 * 2^exp */
    if (*(float*)&tmp > MINVAR) {
      ftmp = 1.0 / *(float*)&tmp;
    } else {
      ftmp = 0;
    }
    exp_table[i] = ftmp;
    /*printf("%3d %08x %g\n", i, tmp, ftmp);*/
  }
#endif
}

/* Bug-fixed version (big-little endian problem) */
void
inv_quant_pred_state(TMP_PRED_STATUS *tmp_psp, PRED_STATUS *psp)
{
    unsigned int i;
    short *p2;
    unsigned long *p1_tmp;


    p1_tmp = (unsigned long *)tmp_psp;
    p2 = (short *) psp;
    
    for (i=0; i<MAX_PRED_BINS*(sizeof(*psp)/sizeof(short)); i++)
	    p1_tmp[i] = ((unsigned long)p2[i])<<16;
   
}

#define FAST_QUANT

/* Bug-fixed version (big-little endian problem) */
static void
quant_pred_state(PRED_STATUS *psp, TMP_PRED_STATUS *tmp_psp)
{
    unsigned int i;
    short *p1;
    ulong *p2_tmp;

#ifdef	FAST_QUANT
    p1 = (short *) psp;
    p2_tmp = (ulong *)tmp_psp;

    for (i=0; i<MAX_PRED_BINS*(sizeof(*psp)/sizeof(short));i++)
      p1[i] = (short) (p2_tmp[i]>>16);
	    
#else
    for (i=0; i<MAX_PRED_BINS; i++) {
      int j;
      p1 = (short *) &psp[i];
      p2_tmp = (ulong *)tmp_psp;
      for (j=0; j<(sizeof(*psp)/sizeof(short)); j++) {
        p1[j] = (short) (p2_tmp[i]>>16); /* Is this a bug?!: p2_tmp[i] is wrong. p2_tmp[i][j] is right. P. Lauber */
      }
    }
#endif
}

/********************************************************************************
 *** FUNCTION: reset_pred_state()						*
 ***										*
 ***    reset predictor state variables						*
 ***										*
 ********************************************************************************/
void
reset_pred_state(PRED_STATUS *psp)
{
    psp->r[0] = Q_ZERO;
    psp->r[1] = Q_ZERO;
    psp->kor[0] = Q_ZERO;
    psp->kor[1] = Q_ZERO;
    psp->var[0] = Q_ONE;
    psp->var[1] = Q_ONE;
}


/********************************************************************************
 *** FUNCTION: init_pred_stat()							*
 ***										*
 ***    initialisation of all predictor parameter				*
 ***										*
 ********************************************************************************/
void
init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b)
{
    static int first_time = 1;

    /* Test of parameters */

    if(grad<0 || grad>MAX_PGRAD) {
	fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
	fprintf(stderr,"\nwrong predictor order: %d\n",grad);
	fprintf(stderr,"range of allowed values: 0 ... %d (=MAX_PGRAD)\n\n",MAX_PGRAD);
	CommonExit(1,"");
    }
    if(alpha<0 || alpha>=1) {
	fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
	fprintf(stderr,"\nwrong time constant alpha: %e\n",alpha);
	fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
	CommonExit(1,"");
    }
    if(a<0 || a>1) {
	fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
	fprintf(stderr,"\nwrong attenuation factor a: %e\n",a);
	fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
	CommonExit(1,"");
    }
    if(b<0 || b>1) {
	fprintf(stderr,"\n\n ****** error in routine init_pred_stat ******\n");
	fprintf(stderr,"\nwrong attenuation factor b: %e\n",b);
	fprintf(stderr,"range of allowed values: 0 ... 1\n\n");
	CommonExit(1,"");
    }

    /* Initialisation */
    if (first_time) {
	ALPHA = alpha;
	A = a;
	B = b;
	make_inv_tables();
	first_time = 0;
    }

    reset_pred_state(psp);
}

/********************************************************************************
 *** FUNCTION: monopred()                                                       *
 ***                                                                            *
 ***    calculation of a predicted value from preceeding (quantised) samples    *
 ***    using a second order backward adaptive lattice predictor with full      *
 ***    LMS adaption algorithm for calculation of predictor coefficients        *
 ***                                                                            *
 ***    parameters:    pc:     pointer to this quantised sample                 *
 ***                   psp:    pointer to structure with predictor status       *
 ***                   pred_flag:       1 if prediction is used                 *
 ***                                                                            *
 ********************************************************************************/

static void monopred( Float           *pc,
                      PRED_STATUS     *psp,
                      TMP_PRED_STATUS *pst,
                      int             pred_flag)
{
  Float qc = *pc;             /* quantized coef */
  float predictedvalue;       /* predicted value */
  float dr1;                  /* difference in the R-branch */
  float e0,e1;                /* "partial" prediction errors (E-branch) */
  float r0,r1;                /* content of delay elements */
  float k1,k2;                /* predictor coefficients */
  
  float *R = pst->r;          /* content of delay elements */
  float *KOR = pst->kor;      /* estimates of correlations */
  float *VAR = pst->var;      /* estimates of variances */
  ulong tmp;
  int i, j;

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
  predictedvalue  = k1*r0 + k2*r1;
  flt_round(&predictedvalue);
  if (pred_flag)
    *pc = qc + predictedvalue;
  /* printf("P1: %8.2f %8.2f\n", predictedvalue, *pc); */
  
  /* Calculate state for use in next block */
  
  /* E-Branch:
   *    Calculate the partial prediction errors using the old predictor coefficients
   *    and the old r-values in order to reconstruct the predictor status of the 
   *    previous step
   */
  
  e0 = *pc;
  e1 = e0-k1*r0;
  
  /* Difference in the R-Branch:
   *    Calculate the difference in the R-Branch using the old predictor coefficients and
   *    the old partial prediction errors as calculated above in order to reconstruct the
   *    predictor status of the previous step
   */
  
  dr1 = k1*e0;
  
  /* Adaption of variances and correlations for predictor coefficients:
   *    These calculations are based on the predictor status of the previous step and give
   *    the new estimates of variances and correlations used for the calculations of the
   *    new predictor coefficients to be used for calculating the current predicted value
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
 *** FUNCTION: predict()							*
 ***										*
 ***    carry out prediction for all allowed spectral lines			*
 ***										*
 ********************************************************************************/


void predict( Info*               info,
              int*                lpflag,
              PRED_STATUS*        psp,
              HANDLE_CONCEALMENT  hConcealment,
              Float*              coef )
{
    int j, k, b, to, flag0;
    const short *top;

    if (info->islong) {
	TMP_PRED_STATUS tmp_ps[MAX_PRED_BINS];
	inv_quant_pred_state(tmp_ps, psp);
	
	b = 0;
	k = 0;
	top = info->sbk_sfb_top[b];
	flag0 = *lpflag++;
	for (j = 0; j < pred_max_bands(mc_info.sampling_rate_idx); j++) {
	    to = *top++;
	    if (flag0 && *lpflag++) {
		for ( ; k < to; k++) {
                  Concealment0SetOfErrCWCoeff("prediction error", &coef[k], k, 0, hConcealment);
                  monopred(&coef[k], &psp[k], &tmp_ps[k], 1);
		}
	    }
	    else {
		for ( ; k < to; k++) {
                  Concealment0SetOfErrCWCoeff("predictior input", &coef[k], k, 0, hConcealment);
                  monopred(&coef[k], &psp[k], &tmp_ps[k], 0);
		}
	    }
	}
	quant_pred_state(psp, tmp_ps);
    }
}





/********************************************************************************
 *** FUNCTION: predict_reset()							*
 ***										*
 ***    carry out cyclic predictor reset mechanism (long blocks)		*
 ***    resp. full reset (short blocks)						*
 ***										*
 ********************************************************************************/
void
predict_reset(Info* info, int *prstflag, PRED_STATUS **psp, 
    int firstCh, int lastCh, int* last_reset_group)
{
    int j, prstflag0, prstgrp, ch;
    static int warn_flag = 1;

    prstgrp = 0;
    if (info->islong) {
	prstflag0 = *prstflag++;
	if (prstflag0) {

          /* for loop modified because of bit-reversed group number */
	    for (j=0; j<LEN_PRED_RSTGRP-1; j++) {
		prstgrp |= prstflag[j];
		prstgrp <<= 1;
	    }
	    prstgrp |= prstflag[LEN_PRED_RSTGRP-1];
          
            
 
              /* check if two consecutive reset group numbers are incremented by one 
                 (this is a poor check, but we don't have much alternatives) */
	      if (NULL != last_reset_group) {
                if ((warn_flag == 1) && (last_reset_group[firstCh] > 0) && (last_reset_group[firstCh] < 31)) {
                  if ((last_reset_group[firstCh] + 1) != prstgrp) {
                    if (30 == last_reset_group[firstCh] && prstgrp == 1) {
                      /* everything is ok */
                    }
                    else {
                      CommonWarning("suspicious Predictor Reset sequence!");
                      warn_flag = 0;
                    }
                  } 
                }
            
                last_reset_group[firstCh] = prstgrp;
              }
	      else {
		      fprintf (stderr, "predict reset group NULL!\n");
	      }

	    if (debug['r']) {
		fprintf(stderr,"PRST: prstgrp: %d  prstbits: ", prstgrp);
		for (j=0; j<LEN_PRED_RSTGRP; j++)
		   fprintf(stderr,"%d ", prstflag[j]);
		fprintf(stderr,"FIRST: %d LAST %d\n", firstCh, lastCh);
	    }

	    if ( (prstgrp<1) || (prstgrp>30) ) {
		fprintf(stderr, "ERROR in prediction reset pattern\n");
		return;
	    }

	    for (ch=firstCh; ch<=lastCh; ch++) {
		for (j=prstgrp-1; j<LN2; j+=30) {
		    reset_pred_state(&psp[ch][j]);
		}
	    }
	} /* end predictor reset */
    } /* end islong */
    else { /* short blocks */
	/* complete prediction reset in all bins */
	for (ch=firstCh; ch<=lastCh; ch++) {
	    for (j=0; j<LN2; j++)
		reset_pred_state(&psp[ch][j]);
	}
    }
}
