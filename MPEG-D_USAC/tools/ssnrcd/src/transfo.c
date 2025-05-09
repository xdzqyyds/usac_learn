/***********************************************************************
 *                                                                     *
 This software module was originally developed by 
 
 AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
 
 in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
 ISO/IEC 13818-7, 14496-1, 2 and 3.
 
 and edited by
 
 Martin Weishart (Fraunhofer IIS, Germany)
 Ralph Sperschneider (Fraunhofer IIS, Germany)
 
 in   the course  of development  of  the  MPEG   Audio conformance test
 software.  This software module  is an implementation  of a part of one
 or more  MPEG tools as  specified by  an MPEG standard.   ISO/IEC gives
 users of the  MPEG standards free  license  to this software module  or
 modifications thereof for use in hardware or software products claiming
 conformance   to the MPEG  standards.    Those  intending to  use  this
 software module in hardware or software  products are advised that this
 use  may infringe existing patents.   The  original  developer of  this
 software  module and his/her company, the  subsequent editors and their
 companies,  and  ISO/IEC have no  liability  for  use of  this software
 module or modifications thereof in an implementation.  Copyright is not
 released for   non  MPEG  conforming products.The   original  developer
 retains full right to  use the code for his/her  own purpose, assign or
 donate the code to a third party and  to inhibit third party from using
 the code for non MPEG conforming products.   This copyright notice must
 be included in all   copies  or derivative works."    Copyright(c)1996,
 2009.
 *                                                                     *
 ***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "common.h"
#include "transfo.h"

#define d2PI  6.283185307179586

#define DEBUG 0
#define SWAP(a,b) tempr=a;a=b;b=tempr   


typedef Float *pFloat;


static ERRORCODE NewFloatMatrix (Float*** temp,
                                 int      N, 
                                 int      M) 
{
  /* Allocate NxM matrix of Floats */
  
  int i;
  
#if DEBUG
  printf("Allocating %d x %d matrix of Floats\n", N, M);
#endif

  /* allocate N pointers to Float arrays */
  *temp = (pFloat *) malloc (N * sizeof (pFloat));
  if (!*temp) {
    printf ("\nERROR: Could not allocate memory for array\n");
    return MEMORYALOCATION_ERROR;
  }
    
  /* Allocate a Float array M long for each of the N array pointers */

  for (i = 0; i < N; i++) {
    (*temp)[i] = (Float *) malloc (M * sizeof (Float));
    if (! (*temp)[i]) {
      printf ("\nERROR: Could not allocate memory for array\n");
      return MEMORYALOCATION_ERROR;
    }
  }
  return OKAY;
}

static void DestroyFloatMatrix(Float*** temp,
                               int      N, 
                               int      M)
{
  int i;

  if ( NULL != *temp ) {
    for (i = 0; i < N; i++) {
      free((*temp)[i]);
    }
    free (*temp);
    *temp = NULL;
  }
}

static void FFT (Float *data, int nn, int isign)  {
  /* Variant of Numerical Recipes code from off the internet.  It takes nn
     interleaved complex input data samples in the array data and returns nn interleaved
     complex data samples in place where the output is the FFT of input if isign==1 and it
     is nn times the IFFT of the input if isign==-1. 
     (Note: it doesn't renormalize by 1/N when doing the inverse transform!!!)
     (Note: this follows physicists convention of +i, not EE of -j in forward
     transform!!!!)
  */

  /* Press, Flannery, Teukolsky, Vettering "Numerical
   * Recipes in C" tuned up ; Code works only when nn is
   * a power of 2  */

  static int n, mmax, m, j, i;
  static Float wtemp, wr, wpr, wpi, wi, theta, wpin;
  static Float tempr, tempi, datar, datai;
  static Float data1r, data1i;

  n = nn * 2;

  /* bit reversal */

  j = 0;
  for (i = 0; i < n; i += 2) {
    if (j > i) {  /* could use j>i+1 to help compiler analysis */
      SWAP (data [j], data [i]);
      SWAP (data [j + 1], data [i + 1]);
    }
    m = nn;
    while (m >= 2 && j >= m) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }

  theta = 3.141592653589795f * .5f;
  if (isign < 0)
    theta = -theta;
  wpin = 0;   /* sin(+-PI) */
  for (mmax = 2; n > mmax; mmax *= 2) {
    wpi = wpin;
    wpin = (float)sin (theta);
    wpr = 1 - wpin * wpin - wpin * wpin; /* cos(theta*2) */
    theta *= .5;
    wr = 1;
    wi = 0;
    for (m = 0; m < mmax; m += 2) {
      j = m + mmax;
      tempr = (Float) wr * (data1r = data [j]);
      tempi = (Float) wi * (data1i = data [j + 1]);
      for (i = m; i < n - mmax * 2; i += mmax * 2) {
        /* mixed precision not significantly more
         * accurate here; if removing float casts,
         * tempr and tempi should be double */
        tempr -= tempi;
        tempi = (Float) wr *data1i + (Float) wi *data1r;
        /* don't expect compiler to analyze j > i + 1 */
        data1r = data [j + mmax * 2];
        data1i = data [j + mmax * 2 + 1];
        data [i] = (datar = data [i]) + tempr;
        data [i + 1] = (datai = data [i + 1]) + tempi;
        data [j] = datar - tempr;
        data [j + 1] = datai - tempi;
        tempr = (Float) wr *data1r;
        tempi = (Float) wi *data1i;
        j += mmax * 2;
      }
      tempr -= tempi;
      tempi = (Float) wr * data1i + (Float) wi *data1r;
      data [i] = (datar = data [i]) + tempr;
      data [i + 1] = (datai = data [i + 1]) + tempi;
      data [j] = datar - tempr;
      data [j + 1] = datai - tempi;
      wr = (wtemp = wr) * wpr - wi * wpi;
      wi = wtemp * wpi + wi * wpr;
    }
  }
}

ERRORCODE CompFFT (Float *data, int nn, int isign) 
{
  static    int     i, j, k, kk;
  static    int     p1, q1;
  static    int     m, n, logq1;
  static    Float** intermed = 0;
  static    Float   ar, ai;
  static    Float   d2pn;
  static    Float   ca, sa, curcos, cursin, oldcos, oldsin;
  static    Float   ca1, sa1, curcos1, cursin1, oldcos1, oldsin1;
  ERRORCODE         status = OKAY;

  /* Factorize n;  n = p1*q1 where q1 is a power of 2.
     For n = 1152, p1 = 9, q1 = 128.		*/

  n = nn;
  logq1 = 0;

  for (;;) {
    m = n >> 1;  /* shift right by one */
    if ((m << 1) == n) {
      logq1++;
      n = m;
    } 
    else {
      break;
    }
  }

  p1 = n;
  q1 = 1;
  q1 <<= logq1;

  d2pn = (float)(d2PI / nn);

  {
    static int oldp1 = 0, oldq1 = 0;

    if ((oldp1 < p1) || (oldq1 < q1)) {
      if (intermed) {
        DestroyFloatMatrix (&intermed, oldp1, 2 * oldq1);
      }
      if (oldp1 < p1) oldp1 = p1;
      if (oldq1 < q1) oldq1 = q1;
    }
		
    if (!intermed)
      status = NewFloatMatrix (&intermed, oldp1, 2 * oldq1);
  }

  /* Sort the p1 sequences */

  for	(i = 0; i < p1; i++) {
    for (j = 0; j < q1; j++){
      intermed [i] [2 * j] = data [2 * (p1 * j + i)];
      intermed [i] [2 * j + 1] = data [2 * (p1 * j + i) + 1];
    }
  }

  /* compute the power of two fft of the p1 sequences of length q1 */

  for (i = 0; i < p1; i++) {
    /* Forward FFT in place for n complex items */
    FFT (intermed [i], q1, isign);
  }

  /* combine the FFT results into one seqquence of length N */

  ca1 = (float)cos (d2pn);
  sa1 = (float)sin (d2pn);
  curcos1 = 1.;
  cursin1 = 0.;

  for (k = 0; k < nn; k++) {
    data [2 * k] = 0.;
    data [2 * k + 1] = 0.;
    kk = k % q1;

    ca = curcos1;
    sa = cursin1;
    curcos = 1.;
    cursin = 0.;

    for (j = 0; j < p1; j++) {
      ar = curcos;
      ai = isign * cursin;
      data [2 * k] += intermed [j] [2 * kk] * ar - intermed [j] [2 * kk + 1] * ai;
      data [2 * k + 1] += intermed [j] [2 * kk] * ai + intermed [j] [2 * kk + 1] * ar;

      oldcos = curcos;
      oldsin = cursin;
      curcos = oldcos * ca - oldsin * sa;
      cursin = oldcos * sa + oldsin * ca;
    }
    oldcos1 = curcos1;
    oldsin1 = cursin1;
    curcos1 = oldcos1 * ca1 - oldsin1 * sa1;
    cursin1 = oldcos1 * sa1 + oldsin1 * ca1;
  }
  DestroyFloatMatrix (&intermed, p1, 2 * q1);
  return status;
}


