/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - FFT and ODFT module by UHD



This software module was originally developed by

Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)

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



Source file: uhd_fft.c

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
11-sep-96   HP    adapted existing code
11-feb-97   BE    optimised odft/fft
12-feb-97   BE    optimised ifft
06-jun-97   HP    added M_PI
26-jan-01   BE    float -> double
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "uhd_fft.h"


/* ---------- declarations (internal) ---------- */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double *sintab;
static int *revtab;
static int fftmax=0;


/* ---------- functions (internal) ---------- */

static void UHD_fftInit(int n)
{
  int i,j,k,exp2;
  double pf;

  /* new tables for increase of fftmax */

  if(n<=fftmax)
    return;

  exp2 = 1;
  while(exp2<n)
    exp2 *= 2;
  if(exp2!=n) {
    fprintf(stderr,"FFT length is not a power of 2!\n");
    exit(1);
  }

  if(fftmax==0) {
    sintab=(double *)malloc((n/2+1)*sizeof(double));
    revtab=(int *)malloc(n*sizeof(int));
  }
  else {
    sintab=(double *)realloc(sintab,(n/2+1)*sizeof(double));
    revtab=(int *)realloc(revtab,n*sizeof(int));
  }
  if(!sintab || !revtab) {
    fprintf(stderr,"memory allocation error in FFT!\n");
    exit(1);
  }
  fftmax = n;

  /* table for bit reversal */

  j = 0;
  for(i=1;i<=n-2;i++) {
    k = n/2;
    while(k<=j) {
      j = j-k;
      k = k/2;
    }
    j = j+k;
    revtab[i] = j;
  }
  revtab[0] = 0;
  revtab[n-1] = n-1;

  pf = M_PI/fftmax;
  for(i=0;i<=fftmax/2;i++) sintab[i] = sin(pf*i);
}


/* ---------- functions (global) ---------- */

void UHD_fft(double xr[], double xi[], int n)
{
  int i,j,len,len2,fftmax2,indf,of,ofmax,ba,nshift;
  double vr,vi,fr,fi;
  double *pxri,*pxii,*pxrj,*pxij;

  if(n>fftmax) {			/* new tables for increase of fftmax */
    UHD_fftInit(n);
    nshift = 0;
  }
  else {				/* fftmax unchanged */
    nshift = 0;
    while((n<<nshift)<fftmax)	nshift++;
    if((n<<nshift)!=fftmax) {
      fprintf(stderr,"FFT length is not a power of 2!!\n");
      exit(1);
    }
  }

  /* input bit reversal */

  for(i=1;i<=n-2;i++) {
    j = revtab[i]>>nshift;
    if(i<j) {
      vr = xr[i];
      xr[i] = xr[j];
      xr[j] = vr;
      vi = xi[i];
      xi[i] = xi[j];
      xi[j] = vi;
    }
  }

  /* butterflies */

  len = 1;
  indf = fftmax;
  fftmax2 = fftmax/2;
  while(len<n) {
    len2 = 2*len;
    ofmax = (len-1)/2;
    i = 0;
    for(of=0;of<len;of++) {
      if(of<=ofmax) {
	fr = sintab[fftmax2-i];
	fi = -sintab[i];
      }
      else {
	fr = -sintab[i-fftmax2];
	fi = -sintab[fftmax-i];
      }
      
      pxri = &xr[of];
      pxii = &xi[of];
      pxrj = &xr[of+len];
      pxij = &xi[of+len];

      for(ba=0;ba<n;ba+=len2) {
	vr = *pxrj*fr-*pxij*fi;
	vi = *pxij*fr+*pxrj*fi;
	*pxrj = *pxri-vr;
	*pxij = *pxii-vi;
	*pxri = *pxri+vr;
	*pxii = *pxii+vi;

	pxri += len2;
	pxii += len2;
	pxrj += len2;
	pxij += len2;
      }
      i += indf;
    }
    len *= 2;
    indf /= 2;
  }
}


void UHD_ifft(double xr[], double xi[], int n)
{
  int i,j,len,len2,fftmax2,indf,of,ofmax,ba,nshift;
  double vr,vi,fr,fi,fnorm;
  double *pxri,*pxii,*pxrj,*pxij;

  /* new tables for increase of fftmax */

  if(n>fftmax) {			/* new tables for increase of fftmax */
    UHD_fftInit(n);
    nshift = 0;
  }
  else {				/* fftmax unchanged */
    nshift = 0;
    while((n<<nshift)<fftmax)	nshift++;
    if((n<<nshift)!=fftmax) {
      fprintf(stderr,"FFT length is not a power of 2!!\n");
      exit(1);
    }
  }

  /* input bit reversal */

  for(i=1;i<=n-2;i++) {
    j = revtab[i]>>nshift;
    if(i<j) {
      vr = xr[i];
      xr[i] = xr[j];
      xr[j] = vr;
      vi = xi[i];
      xi[i] = xi[j];
      xi[j] = vi;
    }
  }

  /* butterflies */

  len = 1;
  indf = fftmax;
  fftmax2 = fftmax/2;
  while(len<n) {
    len2 = 2*len;
    ofmax = (len-1)/2;
    i = 0;
    for(of=0;of<len;of++) {
      if(of<=ofmax) {
	fr = sintab[fftmax2-i];
	fi = sintab[i];
      }
      else {
	fr = -sintab[i-fftmax2];
	fi = sintab[fftmax-i];
      }
      
      pxri = &xr[of];
      pxii = &xi[of];
      pxrj = &xr[of+len];
      pxij = &xi[of+len];

      for(ba=0;ba<n;ba+=len2) {
	vr = *pxrj*fr-*pxij*fi;
	vi = *pxij*fr+*pxrj*fi;
	*pxrj = *pxri-vr;
	*pxij = *pxii-vi;
	*pxri = *pxri+vr;
	*pxii = *pxii+vi;

	pxri += len2;
	pxii += len2;
	pxrj += len2;
	pxij += len2;
      }
      i += indf;
    }
    len *= 2;
    indf /= 2;
  }
  fnorm = 1./n;
  for(i=0;i<n;i++) {
    xr[i] *= fnorm;
    xi[i] *= fnorm;
  }
}


void UHD_odft(double xr[], double xi[], int n)
{
  int i,of;
  double *psr,*psi,fr,fi,xrh;

  if(n>fftmax) {			/* new tables for increase of fftmax */
    UHD_fftInit(n);
  }

  of = fftmax/n;
  psr = &sintab[fftmax/2];
  psi = &sintab[0];
  for(i=0;i<n;i++) {
    if(i<n/2) {
      fr = *psr;
      fi = -*psi;
      psr -= of;
      psi += of;
    }
    else {
      fr = -*psr;
      fi = -*psi;
      psr += of;
      psi -= of;
    }
    xrh = fr*xr[i]-fi*xi[i];
    xi[i] = fr*xi[i]+fi*xr[i];
    xr[i] = xrh;
  }
  UHD_fft(xr,xi,n);
}


void UHD_iodft(double xr[], double xi[], int n)
{
  int i,of;
  double *psr,*psi,fr,fi,xrh;

  if(n>fftmax) {			/* new tables for increase of fftmax */
    UHD_fftInit(n);
  }

  UHD_ifft(xr,xi,n);

  of = fftmax/n;
  psr = &sintab[fftmax/2];
  psi = &sintab[0];
  for(i=0;i<n;i++) {
    if(i<n/2) {
      fr = *psr;
      fi = *psi;
      psr -= of;
      psi += of;
    }
    else {
      fr = -*psr;
      fi = *psi;
      psr += of;
      psi -= of;
    }
    xrh = fr*xr[i]-fi*xi[i];
    xi[i] = fr*xi[i]+fi*xr[i];
    xr[i] = xrh;
  }
}

/* end of uhd_fft.c */
