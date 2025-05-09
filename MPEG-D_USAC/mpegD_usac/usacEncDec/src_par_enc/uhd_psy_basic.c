/**********************************************************************
MPEG-4 Audio VM
basic psychoacoustic model for "individual lines" by UHD



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.


NOTE:
=====
This module provides the basic funtionality to generate all elements of
a bit stream for HILN parameter based coding. However it is not fully
optimised for providing the best possible audio quality.



Source file: uhd_psy_basic.c

 

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
11-sep-96   HP    derived from libuhd_psy.h
13-sep-96   HP    
13-nov-97   HP    removed extern
31-mar-98   HP    permit re-allocation
30-jan-01   BE    float -> double
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <uhd_psy_basic.h>		/* indiline psychoacoustic model */

#include "common_m4a.h"		/* common module */


/* ---------- declarations  ---------- */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))


/* ---------- variables ---------- */

static int PSYlen = 0;
static double PSYdf = 0;
static int PSYallocLen = 0;
static double *PSYpwr = NULL;
static double *PSYmask = NULL;


/* ---------- functions ---------- */

/* UHD_init_psycho_acoustic() */
/* Init UHD psychoacoustic model (BASIC). */

int UHD_init_psycho_acoustic (
  double samplerate,
  double fmax,
  int fftlength,
  int blockoffset,
  int anz_SMR,
  int norm_value)
{
  /*
    CommonWarning("UHD_init_psycho_acoustic: basic psychoacoustic model");
    */
  PSYlen = fftlength/2;
  PSYdf = samplerate/fftlength;
  if (PSYlen > PSYallocLen) {
    if (PSYallocLen == 0) {
      if (((PSYpwr=(double*)malloc(PSYlen*sizeof(double))) == NULL) ||
	  ((PSYmask=(double*)malloc(PSYlen*sizeof(double))) == NULL))
	CommonExit(1,"UHD_init_psycho_acoustic: memory allocation error");
    }
    else {
      if (((PSYpwr=(double*)realloc(PSYpwr,PSYlen*sizeof(double))) == NULL) ||
	  ((PSYmask=(double*)realloc(PSYmask,PSYlen*sizeof(double))) == NULL))
	CommonExit(1,"UHD_init_psycho_acoustic: memory allocation error");
    }
    PSYallocLen = PSYlen;
  }
  return 1;
}


/* UHD_psycho_acoustic() */
/* UHD psychoacoustic model (BASIC). */
/* Calculate a masking threshold 20 dB below signal spectrum */
/* using a 1 Bark wide rectagular window. */

double *UHD_psycho_acoustic (
  double *re,
  double *im,
  int chan)
{
  int i;
  int high,mid,mid_old,low,low_old;
  int bark;
  double sum;

  /* calc power spectrum */
  for (i=0; i<PSYlen; i++)
    PSYpwr[i] = re[i]*re[i]+im[i]*im[i];

  /* calc masking threshold */
  /* bark(f)=max(100Hz,f*0.2) */
  sum = 0;
  mid = mid_old = low = low_old = -1;
  bark = (int)(100/PSYdf+.5);
  for (high=0; high<PSYlen; high++) {
    sum += PSYpwr[high];
    bark = max(bark,(int)(high*0.2+.5));
    low = max(low,high-bark);
    if (low > low_old) {
      sum -= PSYpwr[low];
      low_old = low;
    }
    mid = (low+high+1)/2;
    if (mid > mid_old) {
      PSYmask[mid] = sum/(high-low)*0.1 + 1.0;	/* -20 dB + thres. quiet */
      mid_old = mid;
    }
  }
  high--;
  low++;
  for (; low<PSYlen; low++) {
    sum -= PSYpwr[low];
    mid = (low+high+1)/2;
    if (mid > mid_old) {
      PSYmask[mid] = sum/(high-low)*0.1 + 1.0;	/* -20 dB + thres. quiet */
      mid_old = mid;
    }
  }

  return PSYmask;
}


/* end of uhd_psy_basic.c */
