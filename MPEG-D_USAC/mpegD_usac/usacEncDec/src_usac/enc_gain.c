/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

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
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: enc_gain.c,v 1.2 2010-03-12 06:54:40 mul Exp $
*******************************************************************************/

/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#include <math.h>
#include <memory.h>
#include <string.h>
#include "typedef.h"
#include "int3gpp.h"


#define M               16
#define L_FRAME         256   /* Frame size                                */
#define L_SUBFR         64    /* Subframe size                             */
#define HP_ORDER        3
#define L_INTERPOL1     4
#define L_INTERPOL2     16
#define PIT_SHARP       27853 /* pitch sharpening factor = 0.85 Q15        */
#define F_PIT_SHARP     0.85F /* pitch sharpening factor                   */
#define PIT_MIN         34    /* Minimum pitch lag with resolution 1/4     */
#define UP_SAMP         4
#define DIST_ISF_MAX    120
#define DIST_ISF_THRES  60
#define GAIN_PIT_THRES  0.9F
#define GAIN_PIT_MIN    0.6F

extern const Float32 E_ROM_corrweight[];
extern const Float32 E_ROM_inter4_1[];
extern const Word16 E_ROM_inter4_2[];


/*
 * E_GAIN_sort
 *
 * Parameters:
 *    n              I: number of lags
 *    ra           I/O: lags / sorted lags
 *
 * Function:
 *    Sort open-loop lags
 *
 * Returns:
 *    void
 */
static void E_GAIN_sort(Word32 n, Word32 *ra)
{
   Word32 l, j, ir, i, rra;
   l = (n >> 1) + 1;
   ir = n;
   for (;;)
   {
      if (l > 1)
      {
         rra = ra[--l];
      }
      else
      {
         rra = ra[ir];
         ra[ir] = ra[1];
         if (--ir == 1)
         {
            ra[1] = rra;
            return;
         }
      }
      i = l;
      j = l << 1;
      while (j <= ir)
      {
         if (j < ir && ra[j] < ra[j + 1])
         {
            ++j;                                    
         }
         if (rra < ra[j])
         {
            ra[i] = ra[j];
            j += (i = j);
         }
         else
         {
            j = ir + 1;
         }
      }
      ra[i] = rra;
   }
}

/*
 * E_GAIN_norm_corr
 *
 * Parameters:
 *    exc            I: excitation buffer
 *    xn             I: target signal
 *    h              I: weighted synthesis filter impulse response (Q15)
 *    t0_min         I: minimum value in the searched range
 *    t0_max         I: maximum value in the searched range
 *    corr_norm      O: normalized correlation (Q15)
 *
 * Function:
 *    Find the normalized correlation between the target vector and the
 *    filtered past excitation (correlation between target and filtered
 *    excitation divided by the square root of energy of filtered excitation)
 *    Size of subframe = L_SUBFR.
 *
 * Returns:
 *    void
 */
static void E_GAIN_norm_corr(Float32 exc[], Float32 xn[], Float32 h[],
                             Word32 t_min, Word32 t_max, Float32 *corr_norm)
{
   Float32 excf[L_SUBFR];  /* filtered past excitation */
   Float32 alp, ps, norm;
   Word32 t, j, k;
   k = - t_min;
   /* compute the filtered excitation for the first delay t_min */
   E_UTIL_f_convolve(&exc[k], h, excf);
   /* loop for every possible period */
   for (t = t_min; t <= t_max; t++)
   {
      /* Compute correlation between xn[] and excf[] */
      ps = 0.0F;
      alp = 0.01F;
      for (j = 0; j < L_SUBFR; j++)
      {
         ps += xn[j] * excf[j];
         alp += excf[j] * excf[j];
      }
      /* Compute 1/sqrt(energie of excf[]) */
      norm = (Float32)(1.0F / sqrt(alp));
      /* Normalize correlation = correlation * (1/sqrt(energy)) */
      corr_norm[t] = ps * norm;
      /* update the filtered excitation excf[] for the next iteration */
      if (t != t_max)
      {
         k--;
         for (j = L_SUBFR - 1; j > 0; j--)
         {
            excf[j] = excf[j - 1] + exc[k] * h[j];
         }
         excf[0] = exc[k];
      }
   }
   return;
}
/*
 * E_GAIN_norm_corr_interpolate
 *
 * Parameters:
 *    x           I: input vector
 *    frac        I: fraction (-4..+3)
 *
 * Function:
 *    Interpolating the normalized correlation
 *
 * Returns:
 *    interpolated value
 */
static Float32 E_GAIN_norm_corr_interpolate(Float32 *x, Word32 frac)
{
   Float32 s, *x1, *x2;
   const Float32 *c1, *c2;
   if (frac < 0)
   {
      frac += 4;
      x--;                                          
   }
   x1 = &x[0];
   x2 = &x[1];
   c1 = &E_ROM_inter4_1[frac];
   c2 = &E_ROM_inter4_1[4 - frac];
   s = x1[0] * c1[0] + x2[0] * c2[0];
   s += x1[-1] * c1[4] + x2[1] * c2[4];
   s += x1[-2] * c1[8] + x2[2] * c2[8];
   s += x1[-3] * c1[12] + x2[3] * c2[12];
   return s;
}


void E_GAIN_f_pitch_sharpening(Float32 *x, Word32 pit_lag)
{
   Word32 i;
   for (i = pit_lag; i < L_SUBFR; i++)
   {
      x[i] += x[i - pit_lag] * F_PIT_SHARP;
   }
   return;
}

/*
 * E_GAIN_open_loop_search
 *
 * Parameters:
 *    wsp               I: signal (end pntr) used to compute the open loop pitch
 *    L_min             I: minimum pitch lag
 *    L_max             I: maximum pitch lag
 *    nFrame            I: length of frame to compute pitch
 *    L_0               I: old open-loop lag
 *    gain              O: open-loop pitch-gain
 *    hp_wsp_mem      I/O: memory of the highpass filter for hp_wsp[] (lg = 9)
 *    hp_old_wsp        O: highpass wsp[]
 *    weight_flg        I: is weighting function used
 *
 * Function:
 *    Find open loop pitch lag
 *
 * Returns:
 *    open loop pitch lag
 */
Word32 E_GAIN_open_loop_search(Float32 *wsp, Word32 L_min, Word32 L_max,
                           Word32 nFrame, Word32 L_0, Float32 *gain,
                           Float32 *hp_wsp_mem, Float32 hp_old_wsp[],
                           UWord8 weight_flg)
{
   Word32  i, j, k, L = 0;
   Float32  o, R0, R1, R2, R0_max = -1.0e23f;
   const Float32 *ww, *we;
   Float32 *data_a, *data_b, *hp_wsp, *p, *p1;
   ww = &E_ROM_corrweight[64 + 198];
   we = &E_ROM_corrweight[64 + 98 + L_max - L_0];
   for (i = L_max; i > L_min; i--)
   {
      p  = &wsp[0];
      p1 = &wsp[-i];
      /* Compute the correlation R0 and the energy R1. */   
      R0 = 0.0;
      for (j = 0; j < nFrame; j += 2)
      {
         R0 += p[j] * p1[j];
         R0 += p[j + 1] * p1[j + 1];
      }
      /* Weighting of the correlation function. */
      R0 *= *ww--;
      /* Weight the neighborhood of the old lag. */
      if ((L_0 > 0) & (weight_flg == 1))
      {
         R0 *= *we--;
      }
      /* Store the values if a currest maximum has been found. */
      if (R0 >= R0_max)
      {
         R0_max = R0;
         L = i;
      }
   }
   data_a = hp_wsp_mem;
   data_b = hp_wsp_mem + HP_ORDER;
   hp_wsp = hp_old_wsp + L_max;
   for (k = 0; k < nFrame; k++)
   {
      data_b[0] = data_b[1];
      data_b[1] = data_b[2];
      data_b[2] = data_b[3];
      data_b[HP_ORDER] = wsp[k];
      o = data_b[0] * 0.83787057505665F;
      o += data_b[1] * -2.50975570071058F;
      o += data_b[2] * 2.50975570071058F;
      o += data_b[3] * -0.83787057505665F;
      o -= data_a[0] * -2.64436711600664F;
      o -= data_a[1] * 2.35087386625360F;
      o -= data_a[2] * -0.70001156927424F;
      data_a[2] = data_a[1];
      data_a[1] = data_a[0];
      data_a[0] = o;
      hp_wsp[k] = o;
   }
   p  = &hp_wsp[0];
   p1 = &hp_wsp[-L];
   R0 = 0.0F;
   R1 = 0.0F;
   R2 = 0.0F;
   for (j = 0; j < nFrame; j++)
   {
      R1 += p1[j] * p1[j];
      R2 += p[j] * p[j];
      R0 += p[j] * p1[j];
   }
   *gain = (Float32)(R0 / (sqrt(R1 * R2) + 1e-5));
   memmove(hp_old_wsp, &hp_old_wsp[nFrame], L_max * sizeof(Float32));
   return(L);
}

/*
 * E_GAIN_olag_median
 *
 * Parameters:
 *    prev_ol_lag            I: previous open-loop lag
 *    old_ol_lag             I: old open-loop lags
 *
 * Function:
 *    Median of 5 previous open-loop lags
 *
 * Returns:
 *    median of 5 previous open-loop lags
 */
Word32 E_GAIN_olag_median(Word32 prev_ol_lag, Word32 old_ol_lag[5])
{
   Word32 tmp[6] = {0};
   Word32 i;
   /* Use median of 5 previous open-loop lags as old lag */
   for (i = 4; i > 0; i--)
   {    
      old_ol_lag[i] = old_ol_lag[i-1];
   }    
   old_ol_lag[0] = prev_ol_lag;
   for (i = 0; i < 5; i++)
   {
      tmp[i+1] = old_ol_lag[i];
   }
   E_GAIN_sort(5, tmp);
   return tmp[3];
}

/*
 * E_GAIN_clip_init
 *
 * Parameters:
 *    mem        O: memory of gain of pitch clipping algorithm
 *
 * Function:
 *    Initialises state memory
 *
 * Returns:
 *    void
 */
void E_GAIN_clip_init(Float32 mem[])
{
   mem[0] = DIST_ISF_MAX;
   mem[1] = GAIN_PIT_MIN;
}

/*
 * E_GAIN_clip_test
 *
 * Parameters:
 *    mem         I: memory of gain of pitch clipping algorithm
 *
 * Function:
 *    Gain clipping test to avoid unstable synthesis on frame erasure
 *
 * Returns:
 *    Test result
 */
Word32 E_GAIN_clip_test(Float32 mem[])
{
   Word32 clip;
   clip = 0;
   if ((mem[0] < DIST_ISF_THRES) && (mem[1] > GAIN_PIT_THRES))
   {
      clip = 1;
   }
   return (clip);
}
/*
 * E_GAIN_clip_isf_test
 *
 * Parameters:
 *    isf         I: isf values (in frequency domain)
 *    mem       I/O: memory of gain of pitch clipping algorithm
 *
 * Function:
 *    Check resonance for pitch clipping algorithm
 *
 * Returns:
 *    void
 */
void E_GAIN_clip_isf_test(Float32 isf[], Float32 mem[])
{
   Word32 i;
   Float32 dist, dist_min;
   dist_min = isf[1] - isf[0];
   for (i = 2; i < M - 1; i++)
   {
      dist = isf[i] - isf[i-1];
      if (dist < dist_min)
      {
         dist_min = dist;
      }
   }
   dist = 0.8F * mem[0] + 0.2F * dist_min;
   if (dist > DIST_ISF_MAX)
   {
      dist = DIST_ISF_MAX;
   }
   mem[0] = dist;
   return;
}

/*
 * E_GAIN_closed_loop_search
 *
 * Parameters:
 *    exc            I: excitation buffer
 *    xn             I: target signal
 *    h              I: weighted synthesis filter impulse response
 *    t0_min         I: minimum value in the searched range
 *    t0_max         I: maximum value in the searched range
 *    pit_frac       O: chosen fraction
 *    i_subfr        I: flag to first subframe
 *    t0_fr2         I: minimum value for resolution 1/2
 *    t0_fr1         I: minimum value for resolution 1
 *
 * Function:
 *    Find the closed loop pitch period with 1/4 subsample resolution.
 *
 * Returns:
 *    chosen integer pitch lag
 */
Word32 E_GAIN_closed_loop_search(Float32 exc[], Float32 xn[], Float32 h[],
                             Word32 t0_min, Word32 t0_max, Word32 *pit_frac,
                             Word32 i_subfr, Word32 t0_fr2, Word32 t0_fr1)
{
   Float32 corr_v[15 + 2 * L_INTERPOL1 + 1];
   Float32 cor_max, max, temp;
   Float32 *corr;
   Word32 i, fraction, step;
   Word32 t0, t_min, t_max;
   /* Find interval to compute normalized correlation */
   t_min = t0_min - L_INTERPOL1;
   t_max = t0_max + L_INTERPOL1;
   /* allocate memory to normalized correlation vector */
   corr = &corr_v[-t_min];      /* corr[t_min..t_max] */
   /* Compute normalized correlation between target and filtered excitation */
   E_GAIN_norm_corr(exc, xn, h, t_min, t_max, corr);
   /*  find integer pitch */
   max = corr[t0_min];
   t0  = t0_min;
   for(i = t0_min + 1; i <= t0_max; i++)
   {
      if( corr[i] > max)
      {
         max = corr[i];
         t0 = i;
      }
   }
   /* If first subframe and t0 >= t0_fr1, do not search fractionnal pitch */
   if((i_subfr == 0) & (t0 >= t0_fr1))
   {
      *pit_frac = 0;
      return(t0);
   }
   /*
    * Search fractionnal pitch with 1/4 subsample resolution.
    * Test the fractions around t0 and choose the one which maximizes
    * the interpolated normalized correlation.
    */
   step = 1;                /* 1/4 subsample resolution */
   fraction = -3;
   if (((i_subfr == 0) & (t0 >= t0_fr2)) | (t0_fr2 == PIT_MIN))
   {
      step = 2;              /* 1/2 subsample resolution */
      fraction = -2;
   }
   if (t0 == t0_min)
   {
      fraction = 0;
   }
   cor_max = E_GAIN_norm_corr_interpolate(&corr[t0], fraction);
   for (i = (fraction + step); i <= 3; i += step)
   {
;
      temp = E_GAIN_norm_corr_interpolate(&corr[t0], i);                
      if (temp > cor_max)
      {
         cor_max = temp;
         fraction = i;
      }
   }
   /* limit the fraction value in the interval [0,1,2,3] */
   if (fraction < 0)
   {
      fraction += 4;
      t0 -= 1;
   }
   *pit_frac = fraction;
   return (t0);
}
/*
 * E_GAIN_adaptive_codebook_excitation
 *
 * Parameters:
 *    exc          I/O: excitation buffer
 *    T0             I: integer pitch lag
 *    frac           I: fraction of lag
 *    L_subfr        I: subframe size
 *
 * Function:
 *    Compute the result of Word32 term prediction with fractional
 *    interpolation of resolution 1/4.
 *
 * Returns:
 *    interpolated signal (adaptive codebook excitation)
 */
void E_GAIN_adaptive_codebook_excitation(Word16 exc[], Word16 T0, Word32 frac, Word16 L_subfr)
{
   Word32 i, j, k, L_sum;
   Word16 *x;
   x = &exc[-T0];
   frac = -(frac);
   if (frac < 0)
   {
      frac = (frac + UP_SAMP);
      x--;                                                      
   }
   x = x - L_INTERPOL2 + 1;
   for (j = 0; j < L_subfr; j++)
   {
      L_sum = 0L;
      for (i = 0, k = ((UP_SAMP - 1) - frac); i < 2 * L_INTERPOL2; i++, k += UP_SAMP)
      {
         L_sum = L_sum + (x[i] * E_ROM_inter4_2[k]);
      }
      L_sum = (L_sum + 0x2000) >> 14;
      exc[j] = E_UTIL_saturate(L_sum);
      x++;
   }
   return;
}

/*
 * E_GAIN_lp_decim2
 *
 * Parameters:
 *    x            I/O: signal to process
 *    l              I: size of filtering
 *    mem          I/O: memory (size = 3)
 *
 * Function:
 *    Decimate a vector by 2 with 2nd order fir filter.
 *
 * Returns:
 *    void
 */
void E_GAIN_lp_decim2(Float32 x[], Word32 l, Float32 *mem)
{
   Float32 x_buf[L_FRAME + 3];
   Float32 temp;
   Word32 i, j;
   /* copy initial filter states into buffer */
   memcpy(x_buf, mem, 3 * sizeof(Float32));
   memcpy(&x_buf[3], x, l * sizeof(Float32));
   for (i = 0; i < 3; i++)
   {
      mem[i] =
         ((x[l - 3 + i] > 1e-10) | (x[l - 3 + i] < -1e-10)) ? x[l - 3 + i] : 0;
   }
   for (i = 0, j = 0; i < l; i += 2, j++)
   {
      temp = x_buf[i] * 0.13F;
      temp += x_buf[i + 1] * 0.23F;
      temp += x_buf[i + 2] * 0.28F;
      temp += x_buf[i + 3] * 0.23F;
      temp += x_buf[i + 4] * 0.13F;
      x[j] = temp;
   }
   return;
}

