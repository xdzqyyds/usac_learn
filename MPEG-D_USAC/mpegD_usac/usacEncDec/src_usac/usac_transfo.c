/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, and VoiceAge Corp.

Initial author:

and edited by

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
$Id: usac_transfo.c,v 1.1.1.1 2009-05-29 14:10:21 mul Exp $
*******************************************************************************/


#define d2PI  6.283185307179586

#define DEBUG 0
#define SWAP(a,b) tempr=a;a=b;b=tempr
/* MDCT.cpp - Implementation file for fast MDCT transform */
/* Note: Defines Transform() and ITransform() for compatibility w/
   header files in previous versions. Replace previous Transfo.c file! */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>






#include "usac_transfo.h"

#ifndef PI
#define PI 3.141592653589795
#endif

static void FFT (float *data, int nn, int isign);
static void CompFFT (float *data, int nn, int isign);

/* In-place forward MDCT transform */
static float *NewFloat (int N) {
/* Allocate array of N doubles */

    float *temp;

#if DEBUG
    printf ("Allocating a double array of %d data points\n", N);
#endif

    temp = (float *) malloc (N * sizeof (float));
    if (!temp) {
		printf ("\nERROR util.c 154: Could not allocate memory for array\nExiting program...\n");
		exit (1);
		}
    return temp;
}

static void DeleteFloat (float *var) {
    if (var != NULL)
        free (var);
}

typedef float *pFloat;

static float **NewFloatMatrix (int N, int M) {
/* Allocate NxM matrix of doubles */

    float **temp;
    int i;

#if DEBUG
    printf("Allocating %d x %d matrix of doubles\n", N, M);
#endif

/* allocate N pointers to double arrays */
    temp = (pFloat *) malloc (N * sizeof (pFloat));
    if (!temp) {
		printf ("\nERROR: util.c 252: Could not allocate memory for array\nExiting program...\n");
		exit (1);
		}

/* Allocate a double array M long for each of the N array pointers */

    for (i = 0; i < N; i++) {
		temp [i] = (float *) malloc (M * sizeof (float));
		if (! temp [i]) {
			printf ("\nERROR: Could not allocate memory for array\nExiting program...\n");
			exit (1);
			}
		}
    return temp;
}

/*****************************
  Fast MDCT Code
*****************************/

void floatMDCT (float *data, int N, int b, int isign) {

    static float *FFTarray = 0;    /* the array for in-place FFT */
    float tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
    float freq = 2. * PI / N;
    float fac;
    int i, n;
    int a = N - b;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
    if (isign == 1) {
      fac = 2.; /* 2 from MDCT inverse  to forward */
    }
    else {
      fac = 2. / N; /* remaining 2/N from 4/N IFFT factor */
    }


    {
      static int oldN = 0;
      if (N > oldN) {
	if (FFTarray) {
	  free (FFTarray);
	  FFTarray = 0;
	}
	oldN = N;
      }

      if (! FFTarray)
	FFTarray = NewFloat (N / 2);  /* holds N/4 complex values */
    }

    /* prepare for recurrence relation in pre-twiddle */
    cfreq = cos (freq);
    sfreq = sin (freq);
    c = cos (freq * 0.125);
    s = sin (freq * 0.125);

    for (i = 0; i < N / 4; i++) {

      /* calculate real and imaginary parts of g(n) or G(p) */

      if (isign == 1) { /* Forward Transform */
	n = N / 2 - 1 - 2 * i;
	if (i < b / 4) {
	  tempr = data [a / 2 + n] + data [N + a / 2 - 1 - n]; /* use second form of e(n) for n = N / 2 - 1 - 2i */
	}
	else {
	  tempr = data [a / 2 + n] - data [a / 2 - 1 - n]; /* use first form of e(n) for n = N / 2 - 1 - 2i */
	}
	n = 2 * i;
	if (i < a / 4) {
	  tempi = data [a / 2 + n] - data [a / 2 - 1 - n]; /* use first form of e(n) for n=2i */
	}
	else {
	  tempi = data [a / 2 + n] + data [N + a / 2 - 1 - n]; /* use second form of e(n) for n=2i*/
	}
      }
      else { /* Inverse Transform */
	tempr = -data [2 * i];
	tempi = data [N / 2 - 1 - 2 * i];
      }

      /* calculate pre-twiddled FFT input */
      FFTarray [2 * i] = tempr * c + isign * tempi * s;
      FFTarray [2 * i + 1] = tempi * c - isign * tempr * s;

      /* use recurrence to prepare cosine and sine for next value of i */
      cold = c;
      c = c * cfreq - s * sfreq;
      s = s * cfreq + cold * sfreq;
    }

    /* Perform in-place complex FFT (or IFFT) of length N/4 */
    /* Note: FFT has physics (opposite) sign convention and doesn't do 1/N factor */
    CompFFT (FFTarray, N / 4, -isign);

    /* prepare for recurrence relations in post-twiddle */
    c = cos (freq * 0.125);
    s = sin (freq * 0.125);

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < N / 4; i++) {

      /* get post-twiddled FFT output  */
      /* Note: fac allocates 4/N factor from IFFT to forward and inverse */
      tempr = fac * (FFTarray [2 * i] * c + isign * FFTarray [2 * i + 1] * s);
      tempi = fac * (FFTarray [2 * i + 1] * c - isign * FFTarray [2 * i] * s);

      /* fill in output values */
      if (isign == 1) { /* Forward Transform */
	data [2 * i] = -tempr;   /* first half even */
	data [N / 2 - 1 - 2 * i] = tempi;  /* first half odd */
	data [N / 2 + 2 * i] = -tempi;  /* second half even */
	data [N - 1 - 2 * i] = tempr;  /* second half odd */
      }
      else { /* Inverse Transform */
	data [N / 2 + a / 2 - 1 - 2 * i] = tempr;
	if (i < b / 4) {
	  data [N / 2 + a / 2 + 2 * i] = tempr;
	}
	else {
	  data [2 * i - b / 2] = -tempr;
	}
	data [a / 2 + 2 * i] = tempi;
	if (i < a / 4) {
	  data [a / 2 - 1 - 2 * i] = -tempi;
	}
	else {
	  data [a / 2 + N - 1 - 2*i] = tempi;
	}
      }

      /* use recurrence to prepare cosine and sine for next value of i */
      cold = c;
      c = c * cfreq - s * sfreq;
      s = s * cfreq + cold * sfreq;
    }

        DeleteFloat (FFTarray);

}




static void CompFFT (float *data, int nn, int isign) {

    int i, j, k, kk;
    int p1, q1;
    int m, n, logq1;
    static float **intermed = 0;
    float ar, ai;
    float d2pn;
    float ca, sa, curcos, cursin, oldcos, oldsin;
    float ca1, sa1, curcos1, cursin1, oldcos1, oldsin1;


/* Factorize n;  n = p1*q1 where q1 is a power of 2.
    For n = 1152, p1 = 9, q1 = 128.		*/

    n = nn;
    logq1 = 0;

    for (;;) {
		m = n >> 1;  /* shift right by one*/
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

    d2pn = d2PI / nn;

{static int oldp1 = 0, oldq1 = 0;

	if ((oldp1 < p1) || (oldq1 < q1)) {
		if (intermed) {
      for (i = 0; i < oldp1; i++)
        free(intermed[i]);
			free (intermed);
			intermed = 0;
			}
		if (oldp1 < p1) oldp1 = p1;
		if (oldq1 < q1) oldq1 = q1;
		}

	if (!intermed)
		intermed = NewFloatMatrix (oldp1, 2 * oldq1);
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

    ca1 = cos (d2pn);
    sa1 = sin (d2pn);
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

}


static void FFT (float *data, int nn, int isign)  {
/* Varient of Numerical Recipes code from off the internet.  It takes nn
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

    int n, mmax, m, j, i;
    float wtemp, wr, wpr, wpi, wi, theta, wpin;
    float tempr, tempi, datar, datai;
    float data1r, data1i;

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

    theta = 3.141592653589795 * .5;
    if (isign < 0)
		theta = -theta;
    wpin = 0;   /* sin(+-PI) */
    for (mmax = 2; n > mmax; mmax *= 2) {
		wpi = wpin;
		wpin = sin (theta);
		wpr = 1 - wpin * wpin - wpin * wpin; /* cos(theta*2) */
		theta *= .5;
		wr = 1;
		wi = 0;
		for (m = 0; m < mmax; m += 2) {
			j = m + mmax;
			tempr = (float) wr * (data1r = data [j]);
			tempi = (float) wi * (data1i = data [j + 1]);
			for (i = m; i < n - mmax * 2; i += mmax * 2) {
/* mixed precision not significantly more
 * accurate here; if removing float casts,
 * tempr and tempi should be double */
				tempr -= tempi;
				tempi = (float) wr *data1i + (float) wi *data1r;
/* don't expect compiler to analyze j > i + 1 */
				data1r = data [j + mmax * 2];
				data1i = data [j + mmax * 2 + 1];
				data [i] = (datar = data [i]) + tempr;
				data [i + 1] = (datai = data [i + 1]) + tempi;
				data [j] = datar - tempr;
				data [j + 1] = datai - tempi;
				tempr = (float) wr *data1r;
				tempi = (float) wi *data1i;
				j += mmax * 2;
				}
			tempr -= tempi;
			tempi = (float) wr * data1i + (float) wi *data1r;
			data [i] = (datar = data [i]) + tempr;
			data [i + 1] = (datai = data [i + 1]) + tempi;
			data [j] = datar - tempr;
			data [j + 1] = datai - tempi;
			wr = (wtemp = wr) * wpr - wi * wpi;
			wi = wtemp * wpi + wi * wpr;
			}
		}
}
