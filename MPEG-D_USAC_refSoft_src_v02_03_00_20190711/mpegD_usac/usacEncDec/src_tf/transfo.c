/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
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


#define d2PI  6.283185307179586

#define DEBUG 0
#define SWAP(a,b) tempr=a;a=b;b=tempr   
/* MDCT.cpp - Implementation file for fast MDCT transform */
/* Note: Defines Transform() and ITransform() for compatibility w/
   header files in previous versions. Replace previous Transfo.c file! */

#include <stdio.h>
#include <math.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "obj_descr.h"           /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "scal_dec.h"
#include "common_m4a.h"
#include "tf_main.h"
#include "transfo.h"
#include "util.h"
#ifndef PI
#define PI 3.141592653589795
#endif

void mdct_core ( TF_DATA*      tfData,
                 float         time_sig[], 
                 double        spectrum[],
                 int           block_len )
{
  int           i;
  /*   static int    window_shape_last[2]; */
  /*   static double mdct_overlap_buffer[2][1024];*/  /* could be only 1024/up-sampling-factor,
                                                         since only every ifactor value is != 0 */
  double        up_samp_buffer[1024];

  if( block_len > 1024 ) { 
    CommonExit( 1, "Block length too large!" );
  }


  for( i=0; i<block_len; i++ ) {
    up_samp_buffer[i] = time_sig[i] ;
  }

  buffer2freq( up_samp_buffer,
               spectrum, 
               tfData->mdct_overlap_buffer, 
               tfData->windowSequence[0], 
               tfData->windowShape[0], /* window_shape_last[ch], */
               tfData->prev_windowShape[0],
               block_len, block_len/8, 
               OVERLAPPED_MODE, 0,0,8);			/* HP 970509 */
} 




/* In-place forward MDCT transform */

void Transform(Float *data, int N, int b) {
    MDCT ((double*)data, N, b, 1);
}

/* In-place inverse MDCT transform */
/* Note: 2/N factor ! */
void ITransform (Float *data, int N, int b) {
    MDCT ((double*)data, N, b, -1);

}

static double *NewDouble (int N) {
  /* Allocate array of N doubles */

  double *temp;

#if DEBUG
  printf ("Allocating a double array of %d data points\n", N);
#endif
  
  temp = (double *) malloc (N * sizeof (double));
  if (!temp) {
    printf ("\nERROR util.c 154: Could not allocate memory for array\nExiting program...\n");
    exit (1);
  }
  
  return temp;
}

static void DeleteDouble (double *var) {
  if (var != NULL)
    free (var);
}

typedef double *pDouble;

static double **NewDoubleMatrix (int N, int M) {
  /* Allocate NxM matrix of doubles */
  
  double **temp;
  int i;
  
#if DEBUG
  printf("Allocating %d x %d matrix of doubles\n", N, M);
#endif
  
  /* allocate N pointers to double arrays */
  temp = (pDouble *) malloc (N * sizeof (pDouble));
  if (!temp) {
    printf ("\nERROR: util.c 252: Could not allocate memory for array\nExiting program...\n");
    exit (1);
  }
  
  /* Allocate a double array M long for each of the N array pointers */
  for (i = 0; i < N; i++) {
    temp [i] = (double *) malloc (M * sizeof (double));
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

void MDCT (double *data, int N, int b, int isign) {

    static double *FFTarray = 0;    /* the array for in-place FFT */
    double tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
    double freq = 2. * PI / N;
    double fac;
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
	FFTarray = NewDouble (N / 2);  /* holds N/4 complex values */
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
    
    /*    DeleteDouble (FFTarray);   */
    
}


#ifdef AAC_ELD
void LDFB(double* data, int N, int b, int isign, int modoff){
    /* Very slow implementation for brute force testing */
    /* Note: 2/N factor allocated to forward Xform */

    double phi = 2.*PI/N;
    double no = 0.5*(modoff*b+1);
    double facForward, facBackward, temp;
    double * outData;
    int k,n,xi;
    int a = N-b;    

    outData = NewDouble(2*N); /* 4*framelength*/

    facForward = -2.0;
    facBackward = -2.0 / (double)N;

    if (isign==1) { /* Forward */
        for (k=0;k<b;k++) { /* maybe bug free */
            temp=0; xi=0;            
            for (n=-N;n<N;n++) {            
                temp += data[xi++]*cos(phi*(n+no)*(k+0.5));
            }
            outData[k] = facForward*temp;
        }
    } else { /* Inverse */
        xi=0;
        for (n=0;n<2*N;n++) {
            temp=0; 
            for (k=0;k<b;k++) {
                temp += data[k]*cos(phi*(n+no)*(k+0.5));
            }
            outData[xi++] = temp*facBackward;
        }
    }
    if (isign==1) {/* forward */
      for (n=0;n<(N/2);n++) data[n] = outData[n];
    } else {
      for (n=0;n<(2*N);n++) data[n] = outData[n];
    }
    DeleteDouble(outData);
}
#endif

void CompFFT (double *data, int nn, int isign) {

  int i, j, k, kk;
  int p1, q1;
  int m, n, logq1;
  static double **intermed = 0;
  double ar, ai;
  double d2pn;
  double ca, sa, curcos, cursin, oldcos, oldsin;
  double ca1, sa1, curcos1, cursin1, oldcos1, oldsin1;


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

  {
    static int oldp1 = 0, oldq1 = 0;

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
      intermed = NewDoubleMatrix (oldp1, 2 * oldq1);
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


void FFT (double *data, int nn, int isign)  {
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
  double wtemp, wr, wpr, wpi, wi, theta, wpin;
  double tempr, tempi, datar, datai;
  double data1r, data1i;

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
      tempr = (double) wr * (data1r = data [j]);
      tempi = (double) wi * (data1i = data [j + 1]);
      for (i = m; i < n - mmax * 2; i += mmax * 2) {
        /* mixed precision not significantly more
         * accurate here; if removing float casts,
         * tempr and tempi should be double */
        tempr -= tempi;
        tempi = (double) wr *data1i + (double) wi *data1r;
        /* don't expect compiler to analyze j > i + 1 */
        data1r = data [j + mmax * 2];
        data1i = data [j + mmax * 2 + 1];
        data [i] = (datar = data [i]) + tempr;
        data [i + 1] = (datai = data [i + 1]) + tempi;
        data [j] = datar - tempr;
        data [j + 1] = datai - tempi;
        tempr = (double) wr *data1r;
        tempi = (double) wi *data1i;
        j += mmax * 2;
      }
      tempr -= tempi;
      tempi = (double) wr * data1i + (double) wi *data1r;
      data [i] = (datar = data [i]) + tempr;
      data [i + 1] = (datai = data [i + 1]) + tempi;
      data [j] = datar - tempr;
      data [j + 1] = datai - tempi;
      wr = (wtemp = wr) * wpr - wi * wpi;
      wi = wtemp * wpi + wi * wpr;
    }
  }
}
