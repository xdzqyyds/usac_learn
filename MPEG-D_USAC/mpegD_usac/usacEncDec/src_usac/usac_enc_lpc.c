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
$Id: usac_enc_lpc.c,v 1.3 2011-02-01 09:55:35 mul Exp $
*******************************************************************************/

/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#include <math.h>
#include <memory.h>
#include "typedef.h"
#include "int3gpp.h"

#include "proto_func.h"

#define ORDER        16          /* order of linear prediction filter   */
#define M            16
#define M16k         20          /* Order of LP filter                  */
#define MP1          (M+1)
#define NC16k        (M16k / 2)
#define MU           10923       /* Prediction factor (1.0/3.0) in Q15  */
#define F_MU         (1.0 / 3.0) /* prediction factor                   */
#ifndef PI
#define PI	         3.14159265358979323846264338327950288
#endif

/* lsp_lsf_conversion */
#define SCALE1  (6400.0/PI)
/* lsf_lsp_conversion */
#define SCALE2  (PI/6400.0)
/* chebyshev */
#define  NO_ITER   4    /* no of iterations for tracking the root */
#define  NO_POINTS 100

#define FREQ_MAX 6400.0f
#define FREQ_DIV 400.0f

#define NC              M/2

/*------------------------------------------------------------------*
 *  table of points for evaluting the Chebyshev polynomials         *
 *------------------------------------------------------------------*/
extern const Float32 E_ROM_grid[];


/*--------------------------------------------------------------*
 * function  chebyshev:                                         *
 *           ~~~~~~~~~~                                         *
 *    Evaluates the Chebyshev polynomial series                 *
 *--------------------------------------------------------------*
 *  The polynomial order is                                     *
 *     n = m/2   (m is the prediction order)                    *
 *  The polynomial is given by                                  *
 *    C(x) = T_n(x) + f(1)T_n-1(x) + ... +f(n-1)T_1(x) + f(n)/2 *
 *--------------------------------------------------------------*
 *   R.A.Salami 25 Oct 1990                                     *
 *--------------------------------------------------------------*/

static float E_LPC_chebyshev(   /* output: the value of the polynomial C(x)       */
  Float32 x,         /* input : value of evaluation; x=cos(freq)       */
  Float32 *f,        /* input : coefficients of sum or diff polynomial */
  Word32 n            /* input : order of polynomial                    */
)
{
  Float32 b1, b2, b0, x2;
  Word32 i;                           /* for the special case of 10th order */
                                      /*       filter (n=5)                 */
  x2 = 2.0f*x;                        /* x2 = 2.0*x;                        */
  b2 = 1.0f;                          /*                                    */
  b1 = x2 + f[1];                     /* b1 = x2 + f[1];                    */
  for (i=2; i<n; i++) {               /*                                    */
    b0 = x2*b1 - b2 + f[i];           /* b0 = x2 * b1 - 1. + f[2];          */
    b2 = b1;                          /* b2 = x2 * b0 - b1 + f[3];          */
    b1 = b0;                          /* b1 = x2 * b2 - b0 + f[4];          */
  }                                   /*                                    */
  return (x*b1 - b2 + 0.5f*f[n]);     /* return (x*b1 - b2 + 0.5*f[5]);     */
}

/*------------------------------------------------------------------*
 *            Begin procedure E_LPC_a_lsp_conversion                *
 *------------------------------------------------------------------*/

void E_LPC_a_lsp_conversion(
  Float32 *a,         /* input : LP filter coefficients                     */
  Float32 *lsp,       /* output: Line spectral pairs (in the cosine domain) */
  Float32 *old_lsp    /* input : LSP vector from past frame                 */
)
{
 float f1[NC+1], f2[NC+1];
 float *pa1, *pa2, *pf1, *pf2;
 int   j, i, nf, ip;
 float xlow,ylow,xhigh,yhigh,xmid,ymid,xint;

 /*-------------------------------------------------------------*
  * find the sum and diff polynomials F1(z) and F2(z)           *
  *      F1(z) = [A(z) + z^11 A(z^-1)]/(1+z^-1)                 *
  *      F2(z) = [A(z) - z^11 A(z^-1)]/(1-z^-1)                 *
  *-------------------------------------------------------------*/

 pf1 = f1;                               /* Equivalent code using indices   */
 pf2 = f2;                               /*                                 */
 *pf1++ = 1.0f;                          /* f1[0] = 1.0;                    */
 *pf2++ = 1.0f;                          /* f2[0] = 1.0;                    */
 pa1 = a + 1;                            /*                                 */
 pa2 = a + M;                            /*                                 */
 for (i = 0; i<= NC-1; i++)              /* for (i=1, j=M; i<=NC; i++, j--) */
 {                                       /* {                               */
   /* *pf1++ = *pa1 + *pa2 - *(pf1-1); *//*    f1[i] = a[i]+a[j]-f1[i-1];   */
   *pf1 = *pa1 + *pa2 - *(pf1-1); 
   pf1++;
   /* *pf2++ = *pa1++ - *pa2-- + *(pf2-1);*//* f2[i] = a[i]-a[j]+f2[i-1];   */
   *pf2 = *pa1 - *pa2 + *(pf2-1); 
   pa1++; pa2--; pf2++;
 }                                       /* }                               */

 /*---------------------------------------------------------------------*
  * Find the LSPs (roots of F1(z) and F2(z) ) using the                 *
  * Chebyshev polynomial evaluation.                                    *
  * The roots of F1(z) and F2(z) are alternatively searched.            *
  * We start by finding the first root of F1(z) then we switch          *
  * to F2(z) then back to F1(z) and so on until all roots are found.    *
  *                                                                     *
  *  - Evaluate Chebyshev pol. at grid points and check for sign change.*
  *  - If sign change track the root by subdividing the interval        *
  *    4 times and ckecking sign change.                                *
  *---------------------------------------------------------------------*/

 nf=0;      /* number of found frequencies */
 ip=0;      /* flag to first polynomial   */

 pf1 = f1;  /* start with F1(z) */

 xlow=E_ROM_grid[0];
 ylow = E_LPC_chebyshev(xlow,pf1,NC);

 j = 0;
 while ( (nf < M) && (j < NO_POINTS) )
 {
   j++;
   xhigh = xlow;
   yhigh = ylow;
   xlow = E_ROM_grid[j];
   ylow = E_LPC_chebyshev(xlow,pf1,NC);

   if (ylow*yhigh <= 0.0)  /* if sign change new root exists */
   {
     j--;

     /* divide the interval of sign change by NO_ITER */

     for (i = 0; i < NO_ITER; i++)
     {
       xmid = 0.5f*(xlow + xhigh);
       ymid = E_LPC_chebyshev(xmid,pf1,NC);
       if (ylow*ymid <= 0.0)
       {
         yhigh = ymid;
         xhigh = xmid;
       }
       else
       {
         ylow = ymid;
         xlow = xmid;
       }
     }

     /* linear interpolation for evaluating the root */

     xint = xlow - ylow*(xhigh-xlow)/(yhigh-ylow);

     lsp[nf] = xint;    /* new root */
     nf++;

     ip = 1 - ip;         /* flag to other polynomial    */
     pf1 = ip ? f2 : f1;  /* pointer to other polynomial */

     xlow = xint;
     ylow = E_LPC_chebyshev(xlow,pf1,NC);
   }
 }

 /* Check if M roots found */
 /* if not use the LSPs from previous frame */

 if ( nf < M)
    for(i=0; i<M; i++)  lsp[i] = old_lsp[i];

 return;
}


/*-----------------------------------------------------------*
 * procedure get_lsppol:                                     *
 *           ~~~~~~~~~~~                                     *
 *   Find the polynomial F1(z) or F2(z) from the LSPs.       *
 * This is performed by expanding the product polynomials:   *
 *                                                           *
 * F1(z) =   product   ( 1 - 2 LSF_i z^-1 + z^-2 )           *
 *         i=0,2,4,6,8                                       *
 * F2(z) =   product   ( 1 - 2 LSF_i z^-1 + z^-2 )           *
 *         i=1,3,5,7,9                                       *
 *                                                           *
 * where LSP_i are the LSPs in the cosine domain.            *
 *                                                           *
 *-----------------------------------------------------------*
 *   R.A.Salami    October 1990                              *
 *-----------------------------------------------------------*/

static void get_lsppol(
  float lsp[],    /* input : line spectral freq. (cosine domaine) */
  float f[],      /* output: the coefficients of F1 or F2         */
  int n,          /* input : no of coefficients (m/2)             */
  int flag        /* input : 1 ---> F1(z) ; 2 ---> F2(z)          */
)
{
  float b;
  float *plsp;
  int   i,j;

  plsp = lsp + flag - 1;
  f[0] = 1;
  b = -2.0f * *plsp;
  f[1] = b;
  for (i = 2; i <= n; i++)
  {
    plsp += 2;
    b = -2.0f * *plsp;
    f[i] = b*f[i-1] + 2.0f*f[i-2];
    for (j = i-1; j > 1; j--)
        f[j] += b*f[j-1] + f[j-2];
    f[1] += b;
  }
  return;
}


void E_LPC_f_lsp_a_conversion(
  float *lsp,  /* input : LSF vector (in the cosine domain) */
  float *a     /* output: LP filter coefficients            */
)
{
  float f1[NC+1], f2[NC+1];
  int   i, k;
  float *pf1, *pf2, *pf1_1, *pf2_1, *pa1, *pa2;

 /*-----------------------------------------------------*
  *  Find the polynomials F1(z) and F2(z)               *
  *-----------------------------------------------------*/

  get_lsppol(lsp,f1,NC,1);
  get_lsppol(lsp,f2,NC,2);

 /*-----------------------------------------------------*
  *  Multiply F1(z) by (1+z^-1) and F2(z) by (1-z^-1)   *
  *-----------------------------------------------------*/
  pf1 = f1 + NC;
  pf1_1 = pf1 - 1;
  pf2 = f2 + NC;                      /* Version using indices            */
  pf2_1 = pf2 - 1;                    /*                                  */
  k = NC-1;                           /*                                  */
  for (i = 0; i <= k; i++)            /* for (i = NC; i > 0; i--)         */
  {                                   /* {                                */
    *pf1-- += *pf1_1--;               /*   f1[i] += f1[i-1];              */
    *pf2-- -= *pf2_1--;               /*   f2[i] -= f2[i-1];              */
  }                                   /* }                                */

 /*-----------------------------------------------------*
  *  A(z) = (F1(z)+F2(z))/2                             *
  *  F1(z) is symmetric and F2(z) is antisymmetric      *
  *-----------------------------------------------------*/

  pa1 = a;                            /*                                  */
  *pa1++ = 1.0;                       /* a[0] = 1.0;                      */
  pa2 = a + M;                        /*                                  */
  pf1 = f1 + 1;                       /*                                  */
  pf2 = f2 + 1;                       /*                                  */
  for (i = 0; i <= k; i++)            /* for (i=1, j=M; i<=NC; i++, j--)  */
  {                                   /* {                                */
    *pa1++ = 0.5f*(*pf1 + *pf2);       /*   a[i] = 0.5*(f1[i] + f2[i]);    */
    *pa2-- = 0.5f*(*pf1++ - *pf2++);   /*   a[j] = 0.5*(f1[i] - f2[i]);    */
  }                                   /* }                                */

  return;
}

void reorder_lsf(float *lsf, float min_dist, int n)
{
  int   i;
  float lsf_min;

  lsf_min = min_dist;
  for (i = 0; i < n; i++)
  {
    if (lsf[i] < lsf_min) lsf[i] = lsf_min;

    lsf_min = lsf[i] + min_dist;
  }

  /* reverse */
  lsf_min = FREQ_MAX - min_dist;
  for (i = n-1; i >=0; i--)
  {
    if (lsf[i] > lsf_min) lsf[i] = lsf_min;

    lsf_min = lsf[i] - min_dist;
  }

  return;
}

void lsf_weight_2st(float *lsfq, float *w, int mode)
{
  int i;
  float d[ORDER+1];

  /* compute lsf distance */
  d[0] = lsfq[0];
  d[ORDER] = FREQ_MAX - lsfq[ORDER-1];
  for (i=1; i<ORDER; i++)
  {
    d[i] = lsfq[i] - lsfq[i-1];
  }

  /* weighting function */
  for (i=0; i<ORDER; i++)
  {

    if (mode == 0)
      w[i] = (float)(60.0f / (FREQ_DIV/sqrt(d[i]*d[i+1])));    /* abs */
    else if (mode == 1)
      w[i] = (float)(65.0f / (FREQ_DIV/sqrt(d[i]*d[i+1])));    /* mid */
    else if (mode == 2)
      w[i] = (float)(64.0f / (FREQ_DIV/sqrt(d[i]*d[i+1])));    /* rel1 */
    else 
      w[i] = (float)(63.0f / (FREQ_DIV/sqrt(d[i]*d[i+1])));    /* rel2 */
  }

  return;
}

/*
 * E_LPC_lev_dur
 *
 * Parameters:
 *    r_h         I: vector of autocorrelations (msb)
 *    r_l         I: vector of autocorrelations (lsb)
 *    A           O: LP coefficients (a[0] = 1.0) (m = 16)
 *    rc          O: reflection coefficients
 *    mem       I/O: static memory
 *
 * Function:
 *    Wiener-Levinson-Durbin algorithm to compute
 *    the LPC parameters from the autocorrelations of speech.
 *
 * Returns:
 *    void
 */
float E_LPC_lev_dur(float LPC[], float CC[], long Order)
{

int Loop, i;
float Value, Sum, Sigma2, Gain;
float RC[24]; /* Reflection coefficients (allow for maximum possible order) */

   /*   Initialisation of Levinson Durbin regression   */

   /*   Compute first two LP parameters   */

   LPC[0] = 1.0f;

   RC[0] = -CC[1]/CC[0];
   LPC[1] = RC[0];
   Sigma2 = CC[0] + CC[1] * RC[0];

   /*   Levinson Durbin regression   */

   for (Loop=2; Loop<=Order; Loop++)
   {

      /*   Update reflection coefficient   */

      Sum = 0.0f;
      for (i=0; i<Loop; i++) Sum += CC[Loop-i]*LPC[i];
      RC[Loop-1] = -Sum/Sigma2;

      /*   Update Energy of the prediction residual   */

      Sigma2 = Sigma2*(1.0f - RC[Loop-1]*RC[Loop-1]);
      if (Sigma2<=1.0E-09f)
      {

         /*   The autocorrelation matrix is not positive definite   */

#ifdef DEBUG
         printf("\n   cc2lpc : Autocorrelation matrix is not positive definite !\n");
#endif

         /*   The linear prediction filter corresponds to the   */
         /*   autocorrelation sequence truncated to the point   */
         /*   where it is positive definite.                    */

         Sigma2 = 1.0E-09f;
         for (i=Loop; i<=Order; i++)
         {
            RC[i-1] = 0.0f;
            LPC[i] = 0.0f;
         }
         break;

      }

      /*   Compute LPC coefficients   */

      for (i=1; i<=(Loop/2); i++)
      {
         Value = LPC[i] + RC[Loop-1]*LPC[Loop-i];
         LPC[Loop-i] += RC[Loop-1]*LPC[i];
         LPC[i] = Value;
      }

      LPC[Loop] = RC[Loop-1];

   }

   /*   Compute gain of the predictor   */

   Gain = (float)(10.0*log10(CC[0]/Sigma2));

   return (Gain);

}
/*
 * E_LPC_a_weight
 *
 * Parameters:
 *    a              I: LP filter coefficients
 *    ap             O: weighted LP filter coefficients
 *    gamma          I: weighting factor
 *    m              I: order of LP filter
 *
 * Function:
 *    Weighting of LP filter coefficients, ap[i] = a[i] * (gamma^i).
 *
 * Returns:
 *    void
 */
void E_LPC_a_weight(Float32 *a, Float32 *ap, Float32 gamma, Word32 m)
{
   Float32 f;
   Word32 i;
   ap[0] = a[0];
   f = gamma;
   for (i = 1; i <= m; i++)
   {
      ap[i] = f*a[i];
      f *= gamma;
   }
   return;
}

/*
 * E_LPC_isp_isf_conversion
 *
 * Parameters:
 *    isp            I: isp[m] (range: -1 <= val < 1) (Q15)
 *    isf            O: isf[m] normalized (range: 0 <= val <= 6400)
 *    m              I: LPC order
 *
 * Function:
 *    Transformation isp to isf
 *
 *    ISP are immitance spectral pair in cosine domain (-1 to 1).
 *    ISF are immitance spectral pair in frequency domain (0 to 6400).
 * Returns:
 *    energy of prediction error
 */
void E_LPC_lsp_lsf_conversion(float lsp[], float lsf[], long m)
{
   short i;
   /* convert LSPs to frequency domain 0..6400 */
   for(i = 0; i < m; i++)
   {
      lsf[i] = (float)(acos(lsp[i]) * SCALE1);
   }
   return;
}


void E_LPC_lsf_lsp_conversion(
  float lsf[],    /* input : lsf[m] normalized (range: 0<=val<=6400)  */
  float lsp[],    /* output: lsp[m] (range: -1<=val<1)                */
  int   m         /* input : LPC order                                */
)
{
  int i;
  /*  convert ISFs to the cosine domain */
  for(i=0; i<m; i++) {
    lsp[i] = (float)cos((double)lsf[i] * (double)SCALE2);
  }
  return;
}
