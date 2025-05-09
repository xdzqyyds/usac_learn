
/*

This software module was originally developed by

    Masayuki Nishiguchi, Kazuyuki Iijima, and Jun Matsumoto (Sony Corporation)

    and edited by

    Akira Inoue (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#include <math.h>
#include <stdio.h>

#include "hvxcCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#define	SAMPLE		256	/* in short integer word */
#define	FRM		160

/* only used in encoder */
float	ipc_coef[SAMPLE];
float	ipc_coef160[FRM];


int IPC_inint(float x)
{
    return((int) floor(x + 0.5));
}

void IPC_window(float *coef, float *aryi, float *aryo, int n)
{
      int    i;

      for(i=0; i < n; i++)
          aryo[i]=aryi[i] * coef[i];
}

static void btfly(float c, float s, float r, float i, float *x, float *y)
{
    *x = c*r + s*i;
    *y = c*i - s*r;
    return;
}

void IPC_fft(float *x, float *y, int dim)
{
    int		i,j,k,m,m1,m2;
    int		p,q;
    float	px, py;
    float	th, dth;
    float	a,b;
    int	    ptr1, ptr2;	

    m = 1 << dim;

    q = m;
    dth = 2.0*M_PI / (float)m;

    for(i=1; i<=dim; i++) {
	p = q;
	q /= 2;
	th = 0.0;
	for(j=1; j<=q; j++) {
	    px = cos(th); py = sin(th);
	    th += dth;
	    for(k=p; k<=m; k+=p) {
		ptr1 = k - p + j;
		ptr2 = ptr1 + q;
		a = x[ptr1-1] - x[ptr2-1];
		b = y[ptr1-1] - y[ptr2-1];
		x[ptr1-1] = x[ptr1-1] + x[ptr2-1];
		y[ptr1-1] = y[ptr1-1] + y[ptr2-1];
		btfly(px, py, a, b, &x[ptr2-1], &y[ptr2-1]);
	    }
	}
	dth *= 2.0;
    }

    j=1;
    m1= m-1;
    m2= m/2;

    for(i=1; i<=m1; i++) {
	if(i<j) {
	    a = x[j-1];
	    b = y[j-1];
	    x[j-1] = x[i-1];
	    y[j-1] = y[i-1];
	    x[i-1] = a;
	    y[i-1] = b;
	}
	k = m2;
	while(k<j) {
	    j -= k;
	    k /= 2;
	}
	j += k;
    }
}

void IPC_ifft(float *x, float *y, int dim)
{
    int		m;
    int		i;

    m = 1 << dim;

    for(i=1; i<=m; i++) 
	y[i-1] = -y[i-1];
    IPC_fft(x, y, dim);
    for(i=1; i<=m; i++) {
	x[i-1] = x[i-1]/(float)m;
	y[i-1] = -y[i-1]/(float)m;
    }
}

void IPC_fcall_fft(float *arys, float *rms, float *ang, float *re, float *im)
{
    double rrr;
    int    i, m;
    
    for(i=0; i < SAMPLE; i++) {
       	re[i]=arys[i] ;
    	im[i]=0.;
    }
    
    m=L;

    IPC_fft(re,im,m);
    for(i=0; i < SAMPLE; i++) {
	rrr =  (double)( re[i]*re[i] + im[i]*im[i] );
	rms[i]= (float)(sqrt (rrr));
	
	if (re[i] == 0.)
	    re[i] = 0.00000001f;
	
       	ang[i]= atan2((double)(im[i]),(double)(re[i]));
    }
}

void IPC_lsp_lpc(float *lsf, float *alpha) 
{
    double p[11], q[11], c[11];  
    int i,j,n;

    for (i = 1; i < 11; i++) {
        c[i] = -2.0 * cos((double)(2.0 * M_PI * lsf[i]));
    }


    for(j=3;j<=10;j++)
	p[j]=0.;

    p[0]=1.0;
    p[1]= c[1];
    p[2]= 1.0;

    for(n=1;n<=4;n++){
	for(j=2+n*2;j>=0;j--){
	    if((j-2)>=0)
		p[j]=p[j-2]+p[j-1]*c[2*n+1]+p[j];
	    else if((j-1)>=0)
		p[j]=p[j-1]*c[2*n+1]+p[j];
	}
    }
    for (i = 5; i > 0; i--) {
	p[i] += p[i - 1];

    }


    for(j=3;j<=10;j++)
	q[j]=0.;

    q[0]=1.0;
    q[1]= c[2];
    q[2]= 1.0;

    for(n=1;n<=4;n++){
	for(j=2+n*2;j>=0;j--){
	    if((j-2)>=0)
		q[j]=q[j-2]+q[j-1]*c[2*n+2]+q[j];
	    else if((j-1)>=0)
		q[j]=q[j-1]*c[2*n+2]+q[j];
	}
    }

    for (i = 5; i > 0; i--) {
	q[i] -= q[i - 1];
    }


    for(i = 1; i < 6; i++) {
	alpha[i] = -0.5 * (p[i] + q[i]);
    }

    for(i = 6; i < 11; i++) {
	alpha[i] = -0.5 * (p[11 - i] - q[11 - i]);
    }
}

void IPC_makeCoef(
float	*coef,
int	n)
{
    int i;
    float energy;
    float c4;
    
    energy = 0.;
    for(i=0; i < n ; i++){
	coef[i]=(0.54  - 0.46 * cos( 2*M_PI*i/(n-1))); 
	energy=energy + coef[i] * coef[i] ;
    }
    for(i=0; i < n ; i++)
	coef[i]=coef[i]/sqrt(energy);
    
    c4 = 0.;
    for(i=0; i < n ; i++)
	c4 = c4 + pow(coef[i], 4.);
}

