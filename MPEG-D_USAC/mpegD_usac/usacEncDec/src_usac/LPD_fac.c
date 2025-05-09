/*
This material is strictly confidential and shall remain as such.

Copyright 2007 VoiceAge Corporation. All Rights Reserved. No part of this
material may be reproduced, stored in a retrieval system, or transmitted, in any
form or by any means: photocopying, electronic, mechanical, recording, or
otherwise, without the prior written permission of the copyright holder.

This material is subject to continuous developments and improvements. All
warranties implied or expressed, including but not limited to implied warranties
of merchantability, or fitness for purpose, are excluded.

ACELP is registered trademark of VoiceAge Corporation in Canada and / or other
countries. Any unauthorized use is strictly prohibited.
*/

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cnst.h"
#include "proto_func.h"
#include "int3gpp.h"

#define ORDER 16
#define FREQ_MAX 6400.0f

extern const float sineWindow128[128];

/* sine table for 64 samples overlap (for transition AAC-LPD) */
/*const float ShortWindowSine64[64] =
{
  0.012272F, 0.036807F, 0.061321F, 0.085797F, 0.110222F, 0.134581F, 0.158858F, 0.183040F,
  0.207111F, 0.231058F, 0.254866F, 0.278520F, 0.302006F, 0.325310F, 0.348419F, 0.371317F,
  0.393992F, 0.416430F, 0.438616F, 0.460539F, 0.482184F, 0.503538F, 0.524590F, 0.545325F,
  0.565732F, 0.585798F, 0.605511F, 0.624860F, 0.643832F, 0.662416F, 0.680601F, 0.698376F,
  0.715731F, 0.732654F, 0.749136F, 0.765167F, 0.780737F, 0.795837F, 0.810457F, 0.824589F,
  0.838225F, 0.851355F, 0.863973F, 0.876070F, 0.887640F, 0.898674F, 0.909168F, 0.919114F,
  0.928506F, 0.937339F, 0.945607F, 0.953306F, 0.960431F, 0.966976F, 0.972940F, 0.978317F,
  0.983105F, 0.987301F, 0.990903F, 0.993907F, 0.996313F, 0.998118F, 0.999322F, 0.999925F
};*/



/*---------------------------------------------------------------*
 * Adaptive Low Frequencies Emphasis of spectral coefficients.   *
 *                                                               *
 * Ensure quantization of low frequencies in case where the      *
 * signal dynamic is higher than the LPC noise shaping.          *
 *                                                               *
 * The following gain is applied to each block of coefficients:  *
 *    gain (dB) = 1/2 * (maximum energy - block energy)          *
 *                                                               *
 * The maximum is calculated over all blocks below 1600 Hz.      *
 * The gain of the first block start with a maximum of 20dB and  *
 * decreases by step until the maximum block is reached.         *
 *---------------------------------------------------------------*/

void AdaptLowFreqEmph(float x[], int lg)
{
  int i, j, k, i_max;
  float max, fac, tmp;

  k = 8; 
  i_max = lg/4;    /* ALFE range = 1600Hz (lg = 6400Hz) */

  /* find spectral peak */
  max = 0.01f;
  for(i=0; i<i_max; i+=k)
  {
    tmp = 0.01f;
    for(j=i; j<i+k; j++) 
    {
        tmp += x[j]*x[j];
    }
    if (tmp > max) 
    {
        max = tmp;
    }
  }

  /* emphasis of all blocks below the peak */
  fac = 10.0f;
  for(i=0; i<i_max; i+=k)	
  {	
    tmp = 0.01f;
    for(j=i; j<i+k; j++) 
    {
	    tmp += x[j]*x[j];
    }
    tmp = (float)sqrt(sqrt(max/tmp));
    if (tmp < fac) 
    {
	    fac = tmp;
    }
    for(j=i; j<i+k; j++) 
    {
        x[j] *= fac;
    }
  }
  return;
}


/*---------------------------------------------------------------*
 * Adaptive Low Frequencies Deemphasis of spectral coefficients. *
 *                                                               *
 * Ensure quantization of low frequencies in case where the      *
 * signal dynamic is higher than the LPC noise shaping.          *
 *                                                               *
 * This is the inverse operation of adap_low_freq_emph().        *
 * Output gain of all blocks.                                    *
 *---------------------------------------------------------------*/

void AdaptLowFreqDeemph(float x[], int lg, float gains[])
{
  int i, j, k, i_max;
  float max, fac, tmp;

  k = 8; 
  i_max = lg/4;    /* ALFD range = 1600Hz (lg = 6400Hz) */

  /* find spectral peak */
  max = 0.01f;
  for(i=0; i<i_max; i+=k)
  {
    tmp = 0.01f;
    for(j=i; j<i+k; j++) 
    {
		tmp += x[j]*x[j];
    }
    if (tmp > max) 
    {
		max = tmp;
    }
  }

  /* deemphasis of all blocks below the peak */
  fac = 0.1f;
  for(i=0; i<i_max; i+=k)
  {
    tmp = 0.01f;
    for(j=i; j<i+k; j++) 
    {
        tmp += x[j]*x[j];
    }

    tmp = (float)sqrt(tmp/max);
    if (tmp > fac) 
    {
        fac = tmp;
    }
    for(j=i; j<i+k; j++) 
    {
        x[j] *= fac;
    }
	gains[i/k] = fac;
  }

  return;
}


/*---------------------------------------------------------------*
 * Fast SQ gain estimator (based on energy of quadruples).       *
 * This algorithm is acurate in average bit-rate and provides    *
 * an uniform gain control (distorsion) over the time.           *
 *                                                               *
 * (C) Copyright VoiceAge Corporation. (2007)                    *
 * All Rights Reserved                                           *
 * Author:       B. Bessette                                     *
 *---------------------------------------------------------------*/

float SQ_gain(   /* output: SQ gain                   */ 
  float x[],     /* input:  vector to quantize        */
  int nbitsSQ,   /* input:  number of bits targeted   */ 
  int lg)        /* input:  vector size (2048 max)    */ 
{
  int    i, j, iter;
  float  gain, ener, tmp, target, fac, offset;
  float  en[N_MAX/4];

  /* energy of quadruples with 9dB offset */ 

  for (i=0; i<lg; i+=4)
  {
    ener = 0.01f; 
    for (j=i; j<i+4; j++) {
    	ener += x[j]*x[j];
    }

    tmp=(float)log10(ener);
    en[i/4] = 9.0f + 10.0f*tmp;
  }

  /* SQ scale: 4 bits / 6 dB per quadruple */ 
  target = (6.0f/4.0f)*(float)(nbitsSQ - (lg/16));

  fac = 128.0f;
  offset = fac;
  /* find offset (0 to 128 dB with step of 0.125dB) */
  for (iter=0; iter<10; iter++)
  {
    fac *= 0.5f;
    offset -= fac;
    ener = 0.0f;
    for (i=0; i<lg/4; i++)
    {
      tmp = en[i] - offset;

	  /* avoid SV with 1 bin of amp < 0.5f */
      if (tmp > 3.0f) ener += tmp;
    }
    /* decrease offset while ener is below target */
    if (ener > target) offset += fac;
  }

  gain = (float)pow(10.0f, offset/20.0f);

  return(gain);
}


/*---------------------------------------------------------------*
 * Transform LPC coefficients to mdct gains for noise-shaping.   *
 *                                                               *
 * (C) Copyright Fraunhofer IIS (2007)                           *
 * All Rights Reserved                                           *
 * Initial author:       G. Fuchs                                *
 *---------------------------------------------------------------*/

void lpc2mdct(float *lpcCoeffs, int lpcOrder, float *mdct_gains, int lg)
{
  float InRealData[N_MAX*2];
  float OutRealData[N_MAX*2];
  float InImagData[N_MAX*2];
  float OutImagData[N_MAX*2];
  float tmp;
  int i, sizeN;

  sizeN = 2*lg;

  /*ODFT*/
  for(i=0; i<lpcOrder+1; i++)
  {
    tmp = ((float)i)*PI/(float)(sizeN);
    InRealData[i] = lpcCoeffs[i]*cos(tmp);
    InImagData[i] = -lpcCoeffs[i]*sin(tmp);
  }
  for(;i<sizeN; i++)
  {
    InRealData[i]=0.f;
    InImagData[i]=0.f;
  }

  CFFTN_NI(InRealData, InImagData, OutRealData, OutImagData, sizeN, -1);
  
  /*Get amplitude*/
  for(i=0; i<sizeN/2; i++)
  {
    mdct_gains[i] = 1.0f/sqrt(OutRealData[i]*OutRealData[i] + OutImagData[i]*OutImagData[i]);
  }

  return;
}


/*---------------------------------------------------------------*
 * Interpolated Noise Shaping for mdct coefficients.             *
 * This algorithm shapes temporally the spectral noise between   *
 * the two spectral noise represention (FDNS_NPTS of resolution).*
 * - old_gains[] is the spectral gain at the left folding point. *
 * - new_gains[] is the spectral gain at the right folding point.*
 *                                                               *
 * The noise is shaped monotonically between the two points      *
 * using a curved shape to favor the lower gain in mid-frame.    *
 *                                                               *
 * (C) Copyright VoiceAge Corporation. (2007)                    *
 * All Rights Reserved                                           *
 * Author:       B. Bessette                                     *
 *---------------------------------------------------------------*/

void mdct_IntNoiseShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[])
{
  int i, k;
  float y, mem_y, g1, g2, a, b;

  k = lg/FDNS_NPTS;   /* frequency resolution */

  mem_y = 0;
  for (i=0; i<lg; i++) 
  {
    if ((i%k) == 0)
	{
      g1 = old_gains[i/k];
	  g2 = new_gains[i/k];

      a = 2.0f*g1*g2/(g1+g2);
      b = (g2-g1)/(g1+g2);
	}

    y = a*x[i] + b*mem_y;

    x[i] = y;
	mem_y = y;
  }

  return;
}


/*---------------------------------------------------------------*
 * Interpolated Pre-Shaping for mdct coefficients.               *
 * This algorithm apply the inverse noise-shaping using the      *
 * the two spectral noise represention (FDNS_NPTS of resolution).*
 * (See the mdct_AdaptNoiseShaping function for more details).   *
 *                                                               *
 * (C) Copyright VoiceAge Corporation. (2007)                    *
 * All Rights Reserved                                           *
 * Author:       B. Bessette                                     *
 *---------------------------------------------------------------*/

void mdct_IntPreShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[])
{
  int i, k;
  float y, mem_x, g1, g2, a, b;

  k = lg/FDNS_NPTS;   /* frequency resolution */

  mem_x = 0;
  for (i=0; i<lg; i++) 
  {
    if ((i%k) == 0)
	{
      g1 = old_gains[i/k];
	  g2 = new_gains[i/k];

      a = (g1+g2)/(2.0f*g1*g2);
      b = (g1-g2)/(2.0f*g1*g2);
	}

    y = a*x[i] + b*mem_x;

	mem_x = x[i];
	x[i] = y;
  }

  return;
}


/*---------------------------------------------------------------*
 * Find the TCX interpolated LPC parameters in every subframes.  *
 * This algorithm resolves the TDAC mismatch problem coming from *
 * the 2nd extrapolated subframe.  Now the 2 first subframes are *
 * interpolated using the previous LPC.                          *
 *                                                               *
 * (C) Copyright VoiceAge Corporation. (2007)                    *
 * All Rights Reserved                                           *
 * Author:       B. Bessette                                     *
 *---------------------------------------------------------------*/

void int_lpc_tcx(float lsf_old[],  /* input : LSFs from past frame              */
                 float lsf_new[],  /* input : LSFs from present frame           */
                 float a[],        /* output: LP coefficients in both subframes */
                 int   nb_subfr,   /* input: number of subframe                 */
                 int   m           /* input : order of LP filter                */
)
{
  float lsf[M], *p_a, inc, fnew, fold; 
  int i;

  p_a = a;

  inc = 1.0f / (float)nb_subfr;
  fnew = 0.5f - 0.5f*inc;
  fold = 1.0f - fnew;
  /* average of LSF */
  for (i = 0; i < m; i++)
  {
    lsf[i] = (float)(lsf_old[i]*fold + lsf_new[i]*fnew);
  }
  E_LPC_f_lsp_a_conversion(lsf, p_a);
  p_a += (m+1);
  /* Apply interpolation in FD domain */
  E_LPC_f_lsp_a_conversion(lsf_old, p_a);
  p_a += (m+1);
  E_LPC_f_lsp_a_conversion(lsf_new, p_a);
  p_a += (m+1);

  return;
}

/*---------------------------------------------------------------*
 * Find the ACELP interpolated LPC parameters in every subframes.*
 * Include modifications for LPC alignment with the frame        *
 * boundary.                                                     *
 *                                                               *
 * (C) Copyright VoiceAge Corporation. (2007)                    *
 * All Rights Reserved                                           *
 * Author:       B. Bessette                                     *
 *---------------------------------------------------------------*/

void int_lpc_acelp(float lsf_old[],  /* input : LSFs from past frame              */
                   float lsf_new[],  /* input : LSFs from present frame           */
                   float a[],        /* output: LP coefficients in both subframes */
                   int   nb_subfr,   /* input: number of subframe                 */
                   int   m           /* input : order of LP filter                */
)
{
  float lsf[M], *p_a, inc, fnew, fold; 
  int i, k;

  inc = 1.0f / (float)nb_subfr;
  p_a = a;

  fnew = 1.0f/(2.0f*nb_subfr);

  for (k = 0; k < nb_subfr; k++)
  {
    fold = 1.0f - fnew;
    for (i = 0; i < m; i++){
      lsf[i] = (float)(lsf_old[i]*fold + lsf_new[i]*fnew);
    }
    fnew += inc;
    E_LPC_f_lsp_a_conversion(lsf, p_a);
    p_a += (m+1);
  }  
  /* extrapolation for ZIR: use lsf_new */
  E_LPC_f_lsp_a_conversion(lsf_new, p_a);

  return;
}


int lsf_mid_side(   /* output: 0=old_lpc, 1=new_lpc              */
  float lsf_old[],  /* input : LSFs from past frame              */
  float lsf_mid[],  /* input : LSFs from mid frame               */
  float lsf_new[])  /* input : LSFs from present frame           */ 
{
  int i, side;
  float d[ORDER+1], w[ORDER], tmp, dist1, dist2;

  /* compute lsf distance */
  d[0] = lsf_mid[0];
  d[ORDER] = FREQ_MAX - lsf_mid[ORDER-1];
  for (i=1; i<ORDER; i++)
  {
	  d[i] = lsf_mid[i] - lsf_mid[i-1];
  }

  /* weighting function */
  for (i=0; i<ORDER; i++)
  {
     w[i] = (1.0f/d[i]) + (1.0f/d[i+1]);
  }

  dist1 = 0.0f;
  dist2 = 0.0f;
  for(i=0; i<ORDER; i++)
  {
	  tmp = lsf_mid[i]-lsf_old[i];
	  dist1 += tmp*tmp*w[i];

	  tmp = lsf_mid[i]-lsf_new[i];
	  dist2 += tmp*tmp*w[i];
  }

  if (dist1 < dist2) side = 0;
  else side = 1;

  return (side);
}

void copyFLOAT(const float *X, float *Y, int n){
  memmove(Y, X, n*sizeof(float));
}
