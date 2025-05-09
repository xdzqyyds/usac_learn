/*******************************************************************************
This software module was originally developed by

Institute for Infocomm Research and Fraunhofer IIS

and edited by

-

in the course of development of ISO/IEC 14496 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 14496. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 14496 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

#ifdef NOT_PUBLISHED

Assurance that the originally developed software module can be used (1) in
ISO/IEC 14496 once ISO/IEC 14496 has been adopted; and (2) to develop ISO/IEC
14496:
Institute for Infocomm Research and Fraunhofer IIS grant ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 14496 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 14496 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
14496. To the extent that Institute for Infocomm Research and Fraunhofer IIS 
own patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
14496 in a conforming product, Institute for Infocomm Research and Fraunhofer IIS 
will assure the ISO/IEC that they are willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 14496.

#endif

Institute for Infocomm Research and Fraunhofer IIS retain full right to
modify and use the code for their own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2005.
*******************************************************************************/


#include "interface.h"
#include "int_win_dctiv.h"
#include "int_kbd_win.h"
#include "int_compact_tables.h"
#include <stdio.h>
#include <stdlib.h>


static int log2int (int x)
{
  int l2;
  if (x<0)
    x = -x;
  for (l2 = 0; x > 1; x >>= 1) {
	l2 ++;
  }
  return (l2);
} 


static void swapInt(int* x, int* y)
{
  int dummy;

  dummy = *x;
  *x = *y;
  *y = dummy;
} 


static int shiftRoundINT32(int y,
			   int shift)
{
  return (((y >> (shift-1)) + 1) >> 1);
} 


static int shiftRoundINT32withErrorFeedback(int y,
					    int* errorFeedback,
					    int shift)
{
  int result;

  y += *errorFeedback;

  result = shiftRoundINT32(y,shift);

  *errorFeedback = (result << shift) - y;

  return (result);
} 


static int multShiftINT32(int x,
			  int y,
			  int shift)
{
  return ( (int)(( (INT64) x * y) >> shift) );
} 


static int multShiftRoundINT32(int x,
			       int y,
			       int shift)
{
  return ( ( multShiftINT32(x, y, shift-1) + 1 ) >> 1);
} 


/*
   /xin\    /xout\   /c -s\ /xin\   /c*xin-s*yin\
         ->        =              =
   \yin/    \yout/   \s  c/ \yin/   \s*xin+c*yin/
*/
static void rotateINT32(int index,
			int xin,
			int yin,
			int* xout,
			int* yout)
{
  /* lifting */
  xin += multShiftINT32(-sineDataFunction_cs(index), yin, SHIFT);
  yin += multShiftINT32(sineDataFunction(index), xin, SHIFT);
  xin += multShiftINT32(-sineDataFunction_cs(index), yin, SHIFT);

  *xout = xin;
  *yout = yin;
} 


static void multHalfSqrt2(int* x)
{
  *x = multShiftINT32(sineDataFunction(SINE_DATA_SIZE/2), *x, SHIFT);
} 


/*
   /xin\    /xout\   /1  1\ /xin\   /xin+yin\
         ->        =              =
   \yin/    \yout/   \1 -1/ \yin/   \xin-yin/
*/
/********************************************************
 * 32-bit complexity:
 *
 *   4 adds
 *******************************************************/

static void rotatePlusMinusINT32(int xin,
				 int yin,
				 int* xout,
				 int* yout)
{
  int xtmp, ytmp;

  xtmp = xin;
  ytmp = yin;

  *xout = xtmp + ytmp;
  *yout = xtmp - ytmp;
} 


static void rotatePlusMinusNormINT32(int xin,
				     int yin,
				     int* xout,
				     int* yout)
{
  rotatePlusMinusINT32(xin, yin, xout, yout);
  multHalfSqrt2(xout);
  multHalfSqrt2(yout);
} 


static void bit_reverse_fixpt(int *x, int N)
{
  int m,k,j;

  for (m=1,j=0; m<N-1; m++) {
    {
      for(k=N>>1; (!((j^=k)&k)); k>>=1);
    }

    if (j>m) {
      swapInt(&x[m],&x[j]);
    }
  }
} 


static void copyINT32(int* xin, int* xout, int N)
{
  int i;

  for (i=0; i<N; i++) {
    xout[i] = xin[i];
  }
} 


static void addINT32(int* xin, int* xout, int N)
{
  int i;

  for (i=0; i<N; i++) {
    xout[i] += xin[i];
  }
} 


static void diffINT32(int* xin, int* xout, int N)
{
  int i;

  for (i=0; i<N; i++) {
    xout[i] -= xin[i];
  }
} 


static void shiftLeftINT32(int* x,
			   int N,
			   int shift)
{
  int i;

  for (i=0; i<N; i++) {
    x[i] <<= shift;
  }
} 


static void shiftRightINT32(int* x,
			    int N,
			    int shift)
{
  int i;

  for (i=0; i<N; i++) {
    x[i] >>= shift;
  }
} 


static int msbHeadroomINT32(int *x,
			    int N)
{
  int max = 0;
  int i;

  for (i=0; i<N; i++) {
    max |= ABS(x[i]);
  }
  return (30-log2int(max));
} 


static int shiftIfRequired(int *xr,
			   int *xi,
			   int N)
{
  int shiftRequired = 0;

  if ((!msbHeadroomINT32(xr,N))||(!msbHeadroomINT32(xi,N))) {
    shiftRequired = 1;
    shiftRightINT32(xr, N, 1);
    shiftRightINT32(xi, N, 1);
  }
  return shiftRequired;
} 


static const int srfftIndexTable[32] = {0, 1, 0, 0,
					0, 1, 0, 1,
					0, 1, 0, 0,
					0, 1, 0, 0,
					0, 1, 0, 0,
					0, 1, 0, 1,
					0, 1, 0, 0,
					0, 1, 0, 1};

static int srfftIndex(int l)
{
  return srfftIndexTable[(srfftIndexTable[l>>4]<<4)|(l&0xF)];   /* 2 shifts + 1 add  */
} 


/* Split-Radix FFT
   returns number of shifts performed */
static int srfft_fixpt(int *xr,
		       int *xi,
		       int N)
{
  int k, l;
  int L, M, M2, M4;
  int numShifts = 0;

  /* L = 1,2,4,...,N/2 */
  for (L=1; L<N; L*=2) {

    M = N/L; /* M = N, N/2,...,2 */
    M2 = M/2;
    M4 = M2/2;

    /* L: number of sub-blocks
       M: length of sub-block */

    numShifts += shiftIfRequired(xr, xi, N);

    for (l=0; l<L; l++) {

      /* butterfly on (x[k],x[M2+k]), k = 0,...,N2-1 on each sub-block */
      for (k=0; k<M2; k++) {                            /* N/L/2 times: */
        rotatePlusMinusINT32(xr[l*M+k],xr[l*M+M2+k],
			     &xr[l*M+k],&xr[l*M+M2+k]);
        rotatePlusMinusINT32(xi[l*M+k],xi[l*M+M2+k],
			     &xi[l*M+k],&xi[l*M+M2+k]);
      }

    }

    numShifts += shiftIfRequired(xr, xi, N);

    if (M > 2) {

      for (l=0; l<L; l++) {

        if (srfftIndex(l) == 0) {

          /* x[N2+N4+k] -> -j*x[N2+N4+k] , k = 0,...,N4-1 on each sub-block */
          for (k=0; k<M4; k++) {
            swapInt(&xr[l*M+M2+M4+k],&xi[l*M+M2+M4+k]);
            /* xi[l*M+M2+M4+k] *= -1; */
            xi[l*M+M2+M4+k] = -xi[l*M+M2+M4+k];
          }

        } else {

          /* complex multiplications */
          for (k = 1; k < M4; k++) {

            rotateINT32(4*k*SINE_DATA_SIZE/(2*M),
                xi[l*M+k],xr[l*M+k],
                &xi[l*M+k],&xr[l*M+k]);
            rotateINT32(4*k*SINE_DATA_SIZE/(2*M),
                xi[l*M+M2-k],-xr[l*M+M2-k],
                &xr[l*M+M2-k],&xi[l*M+M2-k]);
          }

          for (k = 1; 3*k < M4; k++) {
            rotateINT32(4*3*k*SINE_DATA_SIZE/(2*M),
                xi[l*M+M2+k],xr[l*M+M2+k],
                &xi[l*M+M2+k],&xr[l*M+M2+k]);
            rotateINT32(4*3*k*SINE_DATA_SIZE/(2*M),
                -xi[l*M+M-k],xr[l*M+M-k],
                &xr[l*M+M-k],&xi[l*M+M-k]);
          }

          for (; 3*k < 2*M4; k++) {
            rotateINT32(4*(M2-3*k)*SINE_DATA_SIZE/(2*M),
                xi[l*M+M2+k],-xr[l*M+M2+k],
                &xr[l*M+M2+k],&xi[l*M+M2+k]);
            rotateINT32(4*(M2-3*k)*SINE_DATA_SIZE/(2*M),
                -xi[l*M+M-k],-xr[l*M+M-k],
                &xi[l*M+M-k],&xr[l*M+M-k]);
          }

          for (; 3*k < 3*M4; k++) {
            rotateINT32(4*(3*k-M2)*SINE_DATA_SIZE/(2*M),
                -xr[l*M+M2+k],xi[l*M+M2+k],
                &xi[l*M+M2+k],&xr[l*M+M2+k]);
            rotateINT32(4*(3*k-M2)*SINE_DATA_SIZE/(2*M),
                -xr[l*M+M-k],-xi[l*M+M-k],
                &xr[l*M+M-k],&xi[l*M+M-k]);
          }

          rotatePlusMinusNormINT32(xi[l*M+M4],xr[l*M+M4],
                       &xr[l*M+M4],&xi[l*M+M4]);
          rotatePlusMinusNormINT32(-xr[l*M+M-M4],xi[l*M+M-M4],
                       &xr[l*M+M-M4],&xi[l*M+M-M4]);
        }
      }
    }
  }

  bit_reverse_fixpt(xr,N);
  bit_reverse_fixpt(xi,N);

  return numShifts;
} 


static void preModulationDCT_fixpt(int *x,
				   int *xr,
				   int *xi,
				   int N)
{
  int i;

  for(i=0;i<N/4;i++) {
    rotateINT32((4*i+1)*SINE_DATA_SIZE/(2*N),
		x[N-1-2*i],x[2*i],
		&xi[i],&xr[i]);
    rotateINT32((4*i+3)*SINE_DATA_SIZE/(2*N),
		x[2*i+1],-x[N-2-2*i],
		&xr[N/2-1-i],&xi[N/2-1-i]);
  }
} 


static void postModulationDCT_fixpt(int *xr,
				    int *xi,
				    int *x,
				    int N)
{
  int i;

  x[0] = xr[0];
  x[N-1] = -xi[0];

  for(i=1;i<N/4;i++){
    rotateINT32(2*i*SINE_DATA_SIZE/N,
		xr[i],-xi[i],
		&x[2*i],&x[N-2*i-1]);
    rotateINT32(2*i*SINE_DATA_SIZE/N,
		xr[N/2-i],xi[N/2-i],
		&x[2*i-1],&x[N-2*i]);
  }

  rotatePlusMinusNormINT32(xr[N/4],xi[N/4],
			   &x[N/2],&x[N/2-1]);
} 


/* sqrt(2)*DCT-IV using INT32
   - input/output is INT32
   - return value needed to normalize */

static int DCTIVsqrt2_fixpt(int *data,
			    int N)
{
  int xr[FRAME_LEN_LONG*MAX_OSF/2];
  int xi[FRAME_LEN_LONG*MAX_OSF/2];
  int i;
  int ldN = log2int(N);
  int sqrt2Normalize; /* indicates if 1/sqrt(2) is needed for normalization */
  int shiftNormalize;
  int preShift = 0;
  int fftShift = 0;

  preShift = msbHeadroomINT32(data, N) - 1;
  if (preShift > 15) preShift = 15;
  if (preShift < 0) preShift = 0;


  shiftLeftINT32(data, N, preShift);

  preModulationDCT_fixpt(data, xr, xi, N);

  fftShift = srfft_fixpt(xr, xi, N/2);

  postModulationDCT_fixpt(xr, xi, data, N);

  /* FFT of length N/2 needs factor of (1/sqrt(2))^(ldN-1) for normalization,
     multiplication with sqrt(2) leads to factor of (1/sqrt(2))^(ldN-2) */

  shiftNormalize = (ldN - 2)/2 + preShift - fftShift;
  sqrt2Normalize = (ldN - 2) % 2;


  /* if log N = 9, 11, 13; N = 512, 2K, 8K: */
  if (sqrt2Normalize) {
    for (i=0; i<N; i++) {
      multHalfSqrt2(&data[i]);
    }
  }

  return shiftNormalize;
} 


/*
   permute1():

   / 1 0                 \
   | 0 1                 |
   |     0 1             |
   |     1 0             |
   |         1 0         |
   |         0 1         |
   |             0 1     |
   |             1 0     |
   \                 ... /
*/
static void permute1(int* signal, int n)
{
  int i;
  for (i=0; i<n; i+=4) {
    swapInt(&signal[i+2], &signal[i+3]);
  }
} 


/*
   permute2(), forward:

   / 1 0 0 0 0 0 0 0 ... \
   | 0 0 1 0 0 0 0 0     |
   | 0 0 0 0 1 0 0 0     |
   | 0 0 0 0 0 0 1 0     |
   | ...                 |
   |                     |
   | 0 1 0 0 0 0 0 0 ... |
   | 0 0 0 1 0 0 0 0     |
   | 0 0 0 0 0 1 0 0     |
   | 0 0 0 0 0 0 0 1     |
   \ ...                 /
*/
static void permute2(int* signal, int n, int direction)
{
  int temp[MAX_OSF * FRAME_LEN_LONG];
  int i;

  for (i=0; i<n; i++) {
    temp[i] = signal[i];
  }
  if (direction == 1) {
    for (i=0; i<n/2; i++) {
      signal[i] = temp[2*i];
      signal[n/2+i] = temp[2*i+1];
    }
  }
  if (direction == -1) {
    for (i=0; i<n/2; i++) {
      signal[2*i] = temp[i];
      signal[2*i+1] = temp[n/2+i];
    }
  }
} 


/*
   Windowing with Noise Shaping:
   - Integer Windowing is done by N/2 x 3 times adding a rounded value
   - this is equal to 3 x N/2 times adding a rounded value
   - in 3 stages N/2 rounded values are added to N/2 succeeding time domain
     values
   - instead of independently rounding the N/2 succeding values, a rounding
     with error feedback is applied to achieve a noise shaping of the
     rounding error
   - technique is similar to noise shaping in Sigma-Delta-Modulation, but
     in the context of IntMDCT a lowpass filter is desired
   - here, a simple 1+z^(-1) lowpass filter is applied
*/
void windowingTDA(int *signal,
		  int N,
		  int windowShape,
		  int direction)
{
  int i;
  int errorFeedback;
  int tmp;
  int window[SINE_DATA_SIZE/2];
  int window_cs[SINE_DATA_SIZE/2];

  if (windowShape == 0) {
    for (i=0; i<N/2; i++) {
      window[i] = sineDataFunction((2*i+1)*SINE_DATA_SIZE/(2*N));
      window_cs[i] = sineDataFunction_cs((2*i+1)*SINE_DATA_SIZE/(2*N));
    }
  } else {
    for (i=0; i<N/2; i++) {
      window[i] = KBDWindow[(2*i+1)*SINE_DATA_SIZE/(2*N)];
      window_cs[i] = KBDWindow_cs[(2*i+1)*SINE_DATA_SIZE/(2*N)];
    }
  }

  errorFeedback = 0;
  for (i=0; i<N/2; i++) {
    tmp = multShiftINT32(-window_cs[i],
             signal[N-1-i],
             SHIFT - SHIFT_FOR_ERROR_FEEDBACK);
    signal[i] -= direction *
      shiftRoundINT32withErrorFeedback(tmp,
				       &errorFeedback,
				       SHIFT_FOR_ERROR_FEEDBACK);
  }

  errorFeedback = 0;
  for (i=0; i<N/2; i++) {
    tmp = multShiftINT32(window[i],
             signal[i],
             SHIFT - SHIFT_FOR_ERROR_FEEDBACK);
    signal[N-1-i] -= direction *
      shiftRoundINT32withErrorFeedback(tmp,
				       &errorFeedback,
				       SHIFT_FOR_ERROR_FEEDBACK);
  }

  errorFeedback = 0;
  for (i=0; i<N/2; i++) {
    tmp = multShiftINT32(-window_cs[i],
			 signal[N-1-i],
			 SHIFT - SHIFT_FOR_ERROR_FEEDBACK);
    signal[i] -= direction *
      shiftRoundINT32withErrorFeedback(tmp,
				       &errorFeedback,
				       SHIFT_FOR_ERROR_FEEDBACK);
  }

} 


static void liftingStep2and3(int* signal1, int* liftBuffer, int N)
{
  int k;
  int shiftNormalize;
  int errorFeedback;

  /* lifting steps 2 and 3 are combined for rounding,
     lifting step 3 is performed before lifting step 2 for implementation
     reasons, to avoid an additional buffer */

  /* lifting step 3:
     / I   1/sqrt(2)*DCTIV \
     \ 0          I        /
  */
  copyINT32(signal1, liftBuffer, N/2);

  shiftNormalize = DCTIVsqrt2_fixpt(liftBuffer, N/2) + 1;
  /* + 1 for 0.5 * sqrt(2)DCTIV */

  if (shiftNormalize > SHIFT_FOR_ERROR_FEEDBACK) {
    shiftRightINT32(liftBuffer, N/2, shiftNormalize - SHIFT_FOR_ERROR_FEEDBACK);
    shiftNormalize = SHIFT_FOR_ERROR_FEEDBACK;
  }

  /* lifting step 2:
     / I  -0.5*I \
     \ 0   I     /
  */
  for (k=0; k<N/2; k++) {
    liftBuffer[k] -= signal1[k] << (shiftNormalize - 1);  /* -1 for 0.5* */
  }

  /* now do the rounding for lifting step 2 and 3 */
  errorFeedback = 0;
  for (k=0; k<N/2; k++) {
    liftBuffer[k] = shiftRoundINT32withErrorFeedback(liftBuffer[k],
						     &errorFeedback,
						     shiftNormalize);
  }
} 


static void liftingStep4(int* signal0, int* liftBuffer, int N)
{
  int shiftNormalize;
  int k;

  /* lifting step 4:
     /       I          0 \
     \ -sqrt(2)*DCTIV   I /
  */

  copyINT32(signal0, liftBuffer, N/2);
  shiftNormalize = DCTIVsqrt2_fixpt(liftBuffer, N/2);
  for (k=0; k<N/2; k++) {
    liftBuffer[k] = -shiftRoundINT32(liftBuffer[k],shiftNormalize);
  }
} 


static void liftingStep5and6(int* signal1, int* liftBuffer, int N, int mono)
{
  int shiftNormalize;
  int k;
  int step = SINE_DATA_SIZE/(2*N);

 /* lifting step 5:
     / I   1/sqrt(2)*DCTIV \
     \ 0          I        /
  */

  copyINT32(signal1, liftBuffer, N/2);

  shiftNormalize = DCTIVsqrt2_fixpt(liftBuffer, N/2);

  shiftRightINT32(liftBuffer, N/2, shiftNormalize);

  /* output rounded and added at later stage */

  if (mono) {

    /* post-modulation: one stage of rotations */

    /* lifting step 6:
       / 1                     cs0 \
       |    1               cs1    |
       |       1         cs2       |
       |          1   cs3          |
       |           ...             |
       |             ...           |
       |                1          |
       |                   1       |
       |                      1    |
       \                         1 /
    */
    for (k=0; k<N/2; k++) {
      liftBuffer[k] += multShiftINT32(-sineDataFunction_cs(step*(2*k+1)),
				      signal1[N/2-1-k],
				      (SHIFT-1));
    }
  } /* if mono */

  /* now do the rounding for lifting step 5 and 6 */
  for (k=0; k<N/2; k++) {
    liftBuffer[k] = ((liftBuffer[k]+1)>>1);
  }
} 


static void liftingStep7(int* signal0, int* liftBuffer, int N)
{
  int k;

  /* lifting step 7:
     / 1                         \
     |    1                      |
     |       1                   |
     |          1                |
     |           ...             |
     |             ...           |
     |          s3    1          |
     |       s2          1       |
     |    s1                1    |
     \ s0                      1 /
  */
  for (k=0; k<N/2; k++) {
    liftBuffer[N/2-1-k] = multShiftRoundINT32(sineDataFunction((2*k+1)*SINE_DATA_SIZE/(2*N)),
					      signal0[k],
					      SHIFT);
  }
} 


static void liftingStep8(int* signal1, int* liftBuffer, int N)
{
  int k;

  /* lifting step 8:
     / 1                     cs0 \
     |    1               cs1    |
     |       1         cs2       |
     |          1   cs3          |
     |           ...             |
     |             ...           |
     |                1          |
     |                   1       |
     |                      1    |
     \                         1 /
  */
  for (k=0; k<N/2; k++) {
    liftBuffer[k] = multShiftRoundINT32(-sineDataFunction_cs((2*k+1)*SINE_DATA_SIZE/(2*N)),
					signal1[N/2-1-k],
					SHIFT);
  }
} 

/*
   Multi-dimensional Lifting:

   Let A be an invertible NxN-Matrix. Then the block matrix

   /A     0 \
   \0 inv(A)/

   can be decomposed into three multi-dimensional lifting steps
   (and some trivial operations) (I: identity matrix)

   /A     0 \  =  /-I 0\  /I      0\  /I -A\  /I      0\  /0 I\
   \0 inv(A)/  =  \ 0 I/  \inv(A) I/  \0  I/  \inv(A) I/  \I 0/

   The inverse is given by inverting all steps:

    /0 I\  /I       0\  /I A\  /I       0\  /-I 0\
    \I 0/  \-inv(A) I/  \0 I/  \-inv(A) I/  \ 0 I/

   To get an invertible integer approximation, the results of A*x
   resp. inv(A)*x just have to be rounded to integer before adding them.
   In contrast to traditional lifting, there is no need for a lifting based
   invertible integer implementation of A resp. inv(A). Any float or
   fixed-point implementation can be used.

   Here a special case is used:
   A == DCT-IV == inv(A)
*/
void intDCTIV(int* signal0,
	      int* signal1,
	      int N,
	      int mono)
{
  int k;
  int liftBuffer[MAX_OSF * FRAME_LEN_LONG];

  /* pre-modulation: permutation and one stage of plus-minus-butterflies */
  if (mono) {
    permute1(signal0, N);
    permute2(signal0, N, 1);
  }

  /* lifting step 1:
     / I  0 \
     \ I  I /
  */
  addINT32(signal0, signal1, N/2);

  /* apply two length N/2 DCT-IV to signal[0..N/2-1] and signal[N/2..N-1]
     by multi-dimensional lifting */

  /* lifting step 2:
     / I  -0.5*I \
     \ 0   I     /
  */
  /* lifting step 3:
     / I   1/sqrt(2)*DCTIV \
     \ 0          I        /
  */
  liftingStep2and3(signal1, liftBuffer, N);
  addINT32(liftBuffer, signal0, N/2);

  /* lifting step 4:
     /       I          0 \
     \ -sqrt(2)*DCTIV   I /
  */
  liftingStep4(signal0, liftBuffer, N);
  addINT32(liftBuffer, signal1, N/2);

  /* lifting step 5:
     / I   1/sqrt(2)*DCTIV \
     \ 0          I        /
  */

  /* if (mono): post-modulation: one stage of rotations */

  /* lifting step 6:
     / 1                     cs0 \
     |    1               cs1    |
     |       1         cs2       |
     |          1   cs3          |
     |           ...             |
     |             ...           |
     |                1          |
     |                   1       |
     |                      1    |
     \                         1 /
  */

  liftingStep5and6(signal1, liftBuffer, N, mono);
  addINT32(liftBuffer, signal0, N/2);

  if (mono) {

    /* lifting step 7:
       / 1                         \
       |    1                      |
       |       1                   |
       |          1                |
       |           ...             |
       |             ...           |
       |          s3    1          |
       |       s2          1       |
       |    s1                1    |
       \ s0                      1 /
    */
    liftingStep7(signal0, liftBuffer, N);
    addINT32(liftBuffer, signal1, N/2);

    /* lifting step 8:
       / 1                     cs0 \
       |    1               cs1    |
       |       1         cs2       |
       |          1   cs3          |
       |           ...             |
       |             ...           |
       |                1          |
       |                   1       |
       |                      1    |
       \                         1 /
    */
    liftingStep8(signal1, liftBuffer, N);
    addINT32(liftBuffer, signal0, N/2);

  } /* if (mono) */

  /*
     / I  0 \
     \ 0 -I /
  */
  for (k=0; k<N/2; k++) {
    signal1[k] *= -1;
  }
} 


void invIntDCTIV(int* signal0,
		 int* signal1,
		 int N,
		 int mono)
{
  int k;
  int liftBuffer[MAX_OSF * FRAME_LEN_LONG];

  /*
    / I  0 \
    \ 0 -I /
  */
  for (k=0; k<N/2; k++) {
    signal1[k] *= -1;
  }

  if (mono) {
    /* inverse lifting step 8:
       / 1                    -cs0 \
       |    1              -cs1    |
       |       1        -cs2       |
       |          1  -cs3          |
       |           ...             |
       |             ...           |
       |                1          |
       |                   1       |
       |                      1    |
       \                         1 /
    */
    liftingStep8(signal1, liftBuffer, N);
    diffINT32(liftBuffer, signal0, N/2);

    /* inverse lifting step 7:
       / 1                         \
       |    1                      |
       |       1                   |
       |          1                |
       |           ...             |
       |             ...           |
       |         -s3    1          |
       |      -s2          1       |
       |   -s1                1    |
       \-s0                      1 /
    */
    liftingStep7(signal0, liftBuffer, N);
    diffINT32(liftBuffer, signal1, N/2);
  } /* if (mono) */

  /* inverse lifting step 5:
     / I   -1/sqrt(2)*DCTIV \
     \ 0           I        /
  */
  /* if (mono): inverse lifting step 6:
     / 1                    -cs0 \
     |    1              -cs1    |
     |       1        -cs2       |
     |          1  -cs3          |
     |           ...             |
     |             ...           |
     |                1          |
     |                   1       |
     |                      1    |
     \                         1 /
  */

  liftingStep5and6(signal1, liftBuffer, N, mono);
  diffINT32(liftBuffer, signal0, N/2);

  /* inverse lifting step 4:
     /      I          0 \
     \ sqrt(2)*DCTIV   I /
  */
  liftingStep4(signal0, liftBuffer, N);
  diffINT32(liftBuffer, signal1, N/2);

  /* inverse lifting step 3:
     / I   -1/sqrt(2)*DCTIV \
     \ 0           I        /
  */
  /* inverse lifting step 2:
     / I   0.5*I \
     \ 0     I   /
  */
  liftingStep2and3(signal1, liftBuffer, N);
  diffINT32(liftBuffer, signal0, N/2);

  /* inverse lifting step 1:
     /  I  0 \
     \ -I  I /
  */
  diffINT32(signal0, signal1, N/2);

  if (mono) {
    permute2(signal0, N, -1);
    permute1(signal0, N);
  }
} 


void IntMidSideINT32(int* l, int* r) /* L/R -> M/S */
{
  int m, s;

  m = *l;
  s = *r;
  m += multShiftRoundINT32(-sineDataFunction_cs(SINE_DATA_SIZE/2), s, SHIFT);
  s += multShiftRoundINT32( sineDataFunction(SINE_DATA_SIZE/2),    m, SHIFT);
  m += multShiftRoundINT32(-sineDataFunction_cs(SINE_DATA_SIZE/2), s, SHIFT);
  *l = s;
  *r = m;
}


void InverseIntMidSideINT32(int* l, int* r) /* M/S -> L/R */
{
  int m, s;

  m = *l;
  s = *r;
  s -= multShiftRoundINT32(-sineDataFunction_cs(SINE_DATA_SIZE/2), m, SHIFT);
  m -= multShiftRoundINT32( sineDataFunction(SINE_DATA_SIZE/2),    s, SHIFT);
  s -= multShiftRoundINT32(-sineDataFunction_cs(SINE_DATA_SIZE/2), m, SHIFT);
  *l = s;
  *r = m;  
}


