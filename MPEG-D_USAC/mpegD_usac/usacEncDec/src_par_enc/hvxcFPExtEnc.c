
/*


This software module was originally developed by

    Masayuki Nishiguchi, Kazuyuki Iijima (Sony Corporation)

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
/* #include <values.h> */
#include <stdio.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#define	NUMPITCHFINE	10    
#define NUMPITCHFINE1	3.   
#define NUMPITCHFINE2	7.   
#define KIZAMI2		0.25

#define	RL		3

float ipc_bsamp[SAMPLE*R+1];
float ipc_bsang[SAMPLE*R+1];

extern float	ipc_coef[SAMPLE];
extern float	ipc_coef160[FRM];

void IPC_make_bss(void)
{
    int i,l;
    float w[SAMPLE * R];
    float im[SAMPLE * R];
    float mag[SAMPLE * R];
    float ang[SAMPLE * R];
    float energy;
    
    energy = 0.;
    for(i=0; i < SAMPLE ; i++)
    {
	w[i]=0.54  - 0.46 * cos( 2*M_PI*i/(SAMPLE -1) ); 
	im[i]=0.0;
	energy += w[i] * w[i];
    }
    for(i=0; i < SAMPLE ; i++) 
	w[i]=w[i]/sqrt(energy);
    
    for(i=SAMPLE ; i < SAMPLE * R; i++)
    {
	w[i]=0.0;
	im[i]=0.0;
    }
    
    
    l = L + RL;

    IPC_fft(w,im,l);
    
    for(i=0; i<SAMPLE * R; i++)
    {
	mag[i] = (sqrt(w[i] * w[i] + im[i] * im[i]));
	if(w[i] == 0.) w[i]=0.00000001;
	ang[i] = atan2(im[i],w[i]);  
    }
    for(i=0; i<SAMPLE * R/2; i++){
	ipc_bsamp[i] = mag[SAMPLE * R / 2 + i];
	ipc_bsang[i] = ang[SAMPLE * R / 2 + i];
    }
    for(i=SAMPLE * R/2; i<=SAMPLE * R; i++){
	ipc_bsamp[i]=mag[i - SAMPLE * R/2];
	ipc_bsang[i]=ang[i - SAMPLE * R/2];
    }
}

static void makeAm_pl_reduce(
float	*pch_er,
int	*numsend,
float	*w0,
float	*w0_r,
float	(*am)[3],
float	*rms,
float	*erl,
float	*erh)
{
    int i,j,send,id;
    float deno,nume2;
    int ub_r = 0, lb_r;
    float re,sumsqabsl,sumsqabsh;
    
    *w0 = (float) SAMPLE * R / (*pch_er);   
    *w0_r = (float) SAMPLE / (*pch_er);   
    send= (int)((*pch_er)/2.);
    *numsend=send;
    
    sumsqabsl=0.0;
    sumsqabsh=0.0;
    for(i=0; i<=send; i++){
	if(i==0)
	    lb_r=0; 
	else 
	    lb_r=ub_r+1; 

	ub_r=IPC_inint((float)i * (*w0_r) + (*w0_r)/2.);
	if(ub_r >= SAMPLE/2 ) 
	    ub_r=SAMPLE/2 -1;
	
	deno=0.;
	nume2=0.;
	for(j=lb_r; j<=ub_r; j++){
	    id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i*(*w0));
	    deno=deno+ipc_bsamp[id]*ipc_bsamp[id];
	    nume2=nume2+rms[j]*ipc_bsamp[id];
	}
	if(deno <= 0)
	    am[i][2]=0;
	else
	    am[i][2]=nume2/deno;

	for(j=lb_r; j<=ub_r; j++){
	    id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i*(*w0));
	    re=rms[j]-am[i][2]*ipc_bsamp[id];
	    if(j>=  50) 
		sumsqabsh += re*re;
	    else	
		sumsqabsl += re*re;
	}
	
    }
    *erl=sumsqabsl;
    *erh=sumsqabsh;
}


static void makeAm_pl_reduce_l(float *pch_er, int *numsend, float *w0, float *w0_r, float (*am)[3], float *rms)
{
    int i,j,send,id;
    float deno,nume,nume2;
    int ub_r = 0,lb_r;
    float sumsqabsl,sumsqabsh;

    *w0 = (float)SAMPLE*R/(*pch_er);   
    *w0_r = (float)SAMPLE/(*pch_er);   
    send= (int)((*pch_er)/2.);
    *numsend=send;

    sumsqabsl=0.;
    sumsqabsh=0.;
    for(i=0; i<=send; i++){
	if(i==0)
	    lb_r=0; 
	else 
	    lb_r=ub_r+1; 

	ub_r=IPC_inint((float)i * (*w0_r) + (*w0_r)/2.);
	if(ub_r >= SAMPLE/2 ) 
	    ub_r=SAMPLE/2 -1;

	if((lb_r+ub_r)/2. <= 54.){
	    deno=0.;
	    nume=0.;
	    nume2=0.;
	    for(j=lb_r; j<=ub_r; j++){
		id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i*(*w0));
		deno=deno+ipc_bsamp[id]*ipc_bsamp[id];
		nume2=nume2+rms[j]*ipc_bsamp[id];
	    }
	    if(deno <= 0)
		am[i][2]=0;
	    else
		am[i][2]=nume2/deno;
	}		
    }
}

static void makeAm_pl_reduce_h(
float	*pch_er,
int	*numsend,
float	*w0,
float	*w0_r,
float	(*am)[3],
float	*rms)
{
    int i,j,send,id;
    float deno,nume,nume2;
    int ub_r = 0, lb_r;
    float sumsqabsl,sumsqabsh;

    *w0 = (float)SAMPLE*R/(*pch_er);   
    *w0_r = (float)SAMPLE/(*pch_er);   
    send= (int)((*pch_er)/2.);
    *numsend=send;

    sumsqabsl=0.;
    sumsqabsh=0.;
    for(i=0; i<=send; i++){
	if(i==0)
	    lb_r=0; 
	else 
	    lb_r=ub_r+1; 

	ub_r=IPC_inint((float)i * (*w0_r) + (*w0_r)/2.);
	if(ub_r >= SAMPLE/2 ) 
	    ub_r=SAMPLE/2 -1;

	if((lb_r+ub_r)/2. >= 50.){
	    deno=0.;
	    nume=0.;
	    nume2=0.;
	    for(j=lb_r; j<=ub_r; j++){
		id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i*(*w0));
		deno=deno+ipc_bsamp[id]*ipc_bsamp[id];
		nume2=nume2+rms[j]*ipc_bsamp[id];
	    }
	    if(deno <= 0)
		am[i][2]=0;
	    else
		am[i][2]=nume2/deno;
	}
    }
}

static void errorUbFine_reduce_fang(
float	*initpch,
float	*mfdpch,
float	(*am)[3],
float	*rms)
{
    int k,send;
    float eubfine,mineub,pitch,finalpitch,finalpitch_l,finalpitch_h;
    float w0,w0_r,erl,erh,min_erh = 0.0, min_erl = 0.0;
    static int cnt=0;
    static float old_pch_l=0.;
    static float old_pch_h=0.;

    if(*initpch <= (float)(20 + NUMPITCH/2 - (NUMPITCHFINE-1.)/2. )){
	mineub=1000000000.;
	pitch = *initpch - (NUMPITCHFINE1-1.)/2. ;
	finalpitch = pitch;
	for(k=0; k<NUMPITCHFINE1; k++){
	    makeAm_pl_reduce(&pitch, &send, &w0, &w0_r, am, rms,
			     &erl, &erh);
	    eubfine= (erl + erh)  ;
	    if(eubfine < mineub || k==0) {
		mineub = eubfine;
		min_erh = erh;
		min_erl = erl;
		finalpitch = pitch;
	    }
	    pitch=pitch+1.0;
	}
	pitch = finalpitch - (NUMPITCHFINE2-1.)/2. * KIZAMI2;
	for(k=0; k<NUMPITCHFINE2; k++){
	    if(k == (NUMPITCHFINE2-1.)/2.){
		eubfine = min_erh;
	    }	
	    else{
		makeAm_pl_reduce(&pitch, &send, &w0, &w0_r, am,
				 rms, &erl, &erh);
		eubfine=  erh   ; 
	    }
	    if(eubfine <= mineub) {
		mineub = eubfine;
		finalpitch_h = pitch;
	    }
	    pitch=pitch+KIZAMI2;
	}

	mineub=1000000000.;
	pitch = finalpitch - (NUMPITCHFINE2-1.)/2. * KIZAMI2;
	for(k=0; k<NUMPITCHFINE2; k++){
	    if(k == (NUMPITCHFINE2-1.)/2.){
		eubfine = min_erl;
	    }	
	    else{
		makeAm_pl_reduce(&pitch,&send,&w0,&w0_r,am,
				 rms,&erl,&erh);
		eubfine=  erl   ; 
	    }
	    if(eubfine <= mineub) {
		mineub = eubfine;
		finalpitch_l = pitch;
	    }
	    pitch=pitch+KIZAMI2;
	}
    }

    else {
	finalpitch_l = finalpitch_h  = *initpch ;
    }

    makeAm_pl_reduce_l(&finalpitch_l,&send,&w0,&w0_r,am,rms);
    makeAm_pl_reduce_h(&finalpitch_h,&send,&w0,&w0_r,am,rms);

    if(finalpitch_h < 20.) finalpitch_h = 20.;
    if(finalpitch_l < 20.) finalpitch_l = 20.;
    *mfdpch=finalpitch_l;

    old_pch_l=finalpitch_l;
    old_pch_h=finalpitch_h;
    cnt=cnt+1;
}


void harm_srew(
float	frmPwr,
float	arysres[SAMPLE],
int	winSize,
int	offset,
float	pitchOL,
float	*mfdpch,
float	(*am)[3],
float	*rms,
float	*dummyHarm,
int	*normMode)
{
    float	dummyPitch;
    float	ang[SAMPLE], re[SAMPLE], im[SAMPLE], aryd[SAMPLE];

    if(winSize != 256 || offset != -48)
    {
	fprintf(stderr, "Unsupported window size or frame offset!!\n");
	exit(1);
    }

    IPC_window(ipc_coef, arysres, aryd, SAMPLE);

    IPC_fcall_fft(aryd, rms, ang, re, im);

    dummyPitch = pitchOL;

    errorUbFine_reduce_fang(&dummyPitch, mfdpch, am, rms);

    *normMode = 0;

    return;
}


