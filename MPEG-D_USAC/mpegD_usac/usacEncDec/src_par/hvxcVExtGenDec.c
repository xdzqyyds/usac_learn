
/*

This software module was originally developed by

    Masayuki Nishiguchi and Kazuyuki Iijima (Sony Corporation)

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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

#define WDEVI           0.1
#define SCALEFAC        10.0

#define RND_MAX     0x7fffffff

#define B_TH1_4K 0.5f
#define B_TH2_4K 0.5f
#define B_TH3_4K 0.8f

#define GV_TH1_4K 0.9f
#define GV_TH2_4K 0.9f
#define GV_TH3_4K 1.0f

#define GUV_TH1_4K 0.30f
#define GUV_TH2_4K 0.30f
#define GUV_TH3_4K 0.20f

#define B_TH1_2K 0.5f
#define B_TH2_2K 0.5f
#define B_TH3_2K 0.7f

#define GV_TH1_2K 0.80f
#define GV_TH2_2K 0.90f
#define GV_TH3_2K 1.00f

#define GUV_TH1_2K 0.40f
#define GUV_TH2_2K 0.30f
#define GUV_TH3_2K 0.20f

#define B_TH2_2   0.85f
#define GV_TH2_2  0.5f
#define GUV_TH2_2 0.5f

#define DELTA 1.0e-5	/* for fixing floating point bugs on win32/linux(AI 99/02/25) */

static float c_dis_v_v[FRM];
static float c_dis_v_uv[FRM];
static float c_dis_uv_v[FRM];

static float c_dis[FRM+LD_LEN];

static float c_con[FRM];


#ifdef __cplusplus
extern "C"
{
#endif

long random1(void);

#ifdef __cplusplus
}
#endif


# define P0_V_V		5.0 / 16.0
# define P1_V_V		11.0 / 16.0

# define P0_UV_V	5.0 / 16.0
# define P1_UV_V	11.0 / 16.0

# define P0_V_UV	5.0 / 16.0
# define P1_V_UV	11.0 / 16.0





static void mkDisWin(
float	*win,
float	p0,
float	p1,
int	n)
{
    int		i;

    for (i = 0; i < (int)(n*p0); i++)
	win[i] = 1.0;
    for (i = (int)(n*p0); i < (int)(n*p1); i++)
	win[i] = ((n*p1) - (float)i)/(n*(p1 - p0));
    for (i = (int)(n*p1); i < n; i++)
	win[i] = 0.0;
}



void IPC_make_c_dis(void)
{
    int i;
    
    mkDisWin(c_dis_v_v, P0_V_V, P1_V_V, FRM);
    mkDisWin(c_dis_v_uv, P0_V_UV, P1_V_UV, FRM);
    mkDisWin(c_dis_uv_v, P0_UV_V, P1_UV_V, FRM);

    for (i = 0; i < FRM; i++)
	c_con[i] = ((float)(FRM-i))/((float)FRM);

    for (i = 0; i < (int)(FRM*5.0/16.0); i++)
        c_dis[i] = 1.0;
    for (i = (int)(FRM*5.0/16.0); i < (int)(FRM*11.0/16.0); i++)
        c_dis[i] = ((FRM * 11.0 / 16.0) - (float) i ) / (FRM * 6.0 / 16.0);
    for (i = (int)(FRM*11.0/16.0); i < FRM; i++)
        c_dis[i] = 0.0;
    for (i = FRM; i < FRM+LD_LEN; i++)
        c_dis[i] = 0.0;
}


static float modu2pai(float x)
{
    float y;
    
    if (x >= 0.0){
	y = x - floor(x/(2*M_PI))*(2*M_PI);
	if (y > M_PI )
	    y = y - 2*M_PI;
	return(y);
    }
    else {
	y = x - ceil(x/(2*M_PI))*(2*M_PI);
	if (y < -M_PI)
	    y = y + 2*M_PI;
	return(y);
    }
}

static void fcall_ifft(
float *rms, 
float *ang, 
float *aryd
)
{
  int i, m;
  float re[SAMPLE], im[SAMPLE];
    
  for (i = 0; i <= SAMPLE/2 ; i++) {
    re[i] = rms[i]*cos(ang[i]);
    im[i] = rms[i]*sin(ang[i]);
  }
  for (i = SAMPLE/2+1; i < SAMPLE; i++) {
    re[i] = re[SAMPLE-i];
    im[i] = (-1)*im[SAMPLE-i];
  }

  im[0] = 0.0;
  im[SAMPLE/2] = 0.0;

  m = L;

  IPC_ifft(re, im, m); 
  for (i = 0; i < SAMPLE; i++) 
    aryd[i] = re[i];
}

static void ifft_am(
float *rms, 
float *ang, 
float *aryd
)
{
  int i, m;
  float re[SAMPLE/2], im[SAMPLE/2];
    
  for (i = 0; i <= SAMPLE/4 ; i++) {
    re[i] = rms[i]*cos(ang[i]);
    im[i] = rms[i]*sin(ang[i]);
  }
  for (i = SAMPLE/4+1; i < SAMPLE/2; i++) {
    re[i] = re[SAMPLE/2-i];
    im[i] = (-1)*im[SAMPLE/2-i];
  }
    
  re[0] = 0.0;
  im[0] = 0.0;
  re[SAMPLE/4] = 0.0;
  im[SAMPLE/4] = 0.0;

  m = L-1;

  IPC_ifft(re, im, m);
    
  for (i = 0; i < SAMPLE/2; i++)
    aryd[i] = (SAMPLE/2)*re[i];
    
}

static void genwg2(float *x)
{
    int n;
    long ra1, ra2;
    double fra1, fra2, uni1,uni2, s, z, x1;
    
    n = 0;
    while (n < SAMPLE) {
	ra1 = random1();
	ra2 = random1();
	fra1 = (double)ra1/(double)RND_MAX;
	fra2 = (double)ra2/(double)RND_MAX;
	uni1 = fra1*2.0-1.0;
	uni2 = fra2*2.0-1.0;
	s = uni1*uni1 + uni2*uni2;
	if (s <= 1.0) {
	    z = sqrt((-2.0*log(s))/s);
	    x1 = uni1*z;
	    x[n] = x1;  
	    n++;
	}
    }
}	

static void zeropad(float *wnso, float *wnsoz)
{
    int i;
    for (i = 0; i < SAMPLE/2-OVERLAP; i++)
	wnsoz[i] = 0.0;
    for (i = SAMPLE/2-OVERLAP; i < 3*SAMPLE/2-OVERLAP; i++)
	wnsoz[i] = wnso[i-(SAMPLE/2-OVERLAP)];
    for (i = 3*SAMPLE/2-OVERLAP; i < 2*FRM; i++)
	wnsoz[i] = 0.0;
}

/* fixing bug of upperbound of cyclic extention(99/02/15) */
#define UPB	

void IPC_vExt_fft(
float *pch, 
float (*am)[3], 
int *vuv, 
float *sv,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i;
    float w01,w02,w0s;
    int send1,send2,send,sendm,wdevif;
    float am2[SAMPLE/2];
    int	phrst;
    
    float phai2[SAMPLE/2];
    float ovsr1,ovsr2,ovsrc;
    float lp1,lp2,lp12,ilp12;
    float lp1r,lp2r,lp12r;
    float wave1[SAMPLE/2];

    float out[2000],out2[2000],out3[2000];
    float c_dis_lp12[2000];

    int st;
    float ffi,fi,ffim,ffip;
    float sv1[FRM],sv2[FRM];
    float pha1[SAMPLE/2];

    int iflat = 0,iflat2 = 0;

    float	b_th1;
    float	b_th2;
    float	b_th3;

    float	gv_th1;
    float	gv_th2;
    float	gv_th3;

    float	b_th2_2;

    float	gv_th2_2;

    /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
    int Lp12;
#endif
    float 	tmp;	/* AI 99/05/28 */

    w01 = (float)(2.*M_PI/pch[0]);  
    w02 = (float)(2.*M_PI/pch[1]);
    w0s = (float)(2.*M_PI/(SAMPLE/2.));
    
    send1= (int)(pch[0]/2.);   
    send2= (int)(pch[1]/2.);

    /* fixing bug of ambiguous casting(99/05/28) */
    tmp = 1.0/WDEVI*fabs(pch[0]-pch[1]);
    /* if(fabs((w02-w01)/w02) >= WDEVI ) */
    if(tmp >= pch[0]) 
      wdevif = 1;
    else 
      wdevif = 0;

    if(vuv[0] ==0 && HDS->old_old_vuv ==0)
	phrst=1;
    else
	phrst=0;
    
    if(HDS->decMode == DEC2K)
    {
	b_th1 = B_TH1_2K;
	b_th2 = B_TH2_2K;
	b_th3 = B_TH3_2K;
	
	gv_th1 = GV_TH1_2K;
	gv_th2 = GV_TH2_2K;
	gv_th3 = GV_TH3_2K;

	b_th2_2 = B_TH2_2;
	gv_th2_2 = GV_TH2_2;
    }
    else
    {
	b_th1 = B_TH1_4K;
	b_th2 = B_TH2_4K;
	b_th3 = B_TH3_4K;
	
	gv_th1 = GV_TH1_4K;
	gv_th2 = GV_TH2_4K;
	gv_th3 = GV_TH3_4K;

	b_th2_2 = B_TH2_2;
	gv_th2_2 = GV_TH2_2;
    }
    
    for(i=0; i<SAMPLE/2; i++){
	
	am2[i]=am[i][2];
	pha1[i] = HDS->pha2[i] ;

	if(phrst==1)
	{
	  if (HDS->testMode & TM_INIT_PHASE_ZERO) {	/* AI 99/01/12 */
	    HDS->pha2[i] = 0.0f;
	  }
	  else {
	    HDS->pha2[i] = 0.5 * M_PI * random1() / (float) RND_MAX;
	  }
	}
	else
	{
	    if(HDS->decDelayMode == DM_SHORT)
	    {
		HDS->pha2[i] += (float)i * ((float)(FRM-LD_LEN) * (w01+w02) * 0.5 + w01 * (float)LD_LEN) ;
	    }
	    else
	    {
		HDS->pha2[i] += (w01+w02)*(float)i*(float)FRM/2.0;
	    }
	}

	HDS->pha2[i] = modu2pai( HDS->pha2[i] );

    }

    /* for fixing floating point bugs on win32/linux(AI 99/02/25) */
    if (vuv[1] == 1) {
      for (i = 0; i < (float)send2*b_th1 - DELTA; i++) {
      }
      for (; i <= send2; i++) {
	am2[i] = am2[i] * gv_th1;
      }
    }
    else if (vuv[1] == 2) {
      for (i = 0; i < (float)send2*b_th2 - DELTA; i++) {
      }
      for (; i < (float)send2*b_th2_2 - DELTA; i++) {
	am2[i] = am2[i] * gv_th2; 
      }
      for (; i <= send2; i++) {
	am2[i] = am2[i] * gv_th2_2; 
      }
    }
    else if (vuv[1] == 3) {
      for (i = 0; i < (float)send2*b_th3 - DELTA; i++) {
      }
      for (; i <= send2; i++) {
	am2[i] = am2[i] * gv_th3; 
      }
    }
    else {	/* vuv[1] == 0 */
      for (i = 0; i <= send2; i++)
	am2[i] = 0.0f;  
    }

    for (i = send2+1; i < SAMPLE/2; i++) {
      am2[i] = 0.0f;
    }

    if(send1 <= send2){
	send=send1;
	sendm=send2;
    }
    else {
	send=send2;
	sendm=send1;
    }
    
    if(wdevif == 0){
	
	for(i=0;i<SAMPLE/2;i++)
	    wave1[i]=HDS->wave2[i];
	
	ovsr1=(float)(SAMPLE/2.0)/pch[0];
	ovsr2=(float)(SAMPLE/2.0)/pch[1];
	
	lp1=ceil((float)FRM*ovsr1);
	lp1r=floor((float)FRM*ovsr1 + 0.5);
	lp2=ceil((float)FRM*ovsr2);
	lp2r=floor((float)FRM*ovsr2 + 0.5);

	if(HDS->decDelayMode == DM_SHORT)
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    Lp12=(int)ceil((float)(FRM-LD_LEN+1)*(ovsr1*0.5+ovsr2*0.5)
			   +(float)(LD_LEN+1)*ovsr1);
#endif	
	    lp12=ceil((float)(FRM-LD_LEN)*(ovsr1*0.5+ovsr2*0.5)+(float)LD_LEN*ovsr1);
	    lp12r=floor((float)(FRM-LD_LEN)*(ovsr1*0.5+ovsr2*0.5)+(float)LD_LEN*ovsr1 + 0.5);

	    iflat=(int)floor(ovsr1*(float)LD_LEN + 0.5);
	    iflat2=(int)floor(ovsr2*(float)LD_LEN + 0.5);
	}
	else
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    Lp12=(int)ceil((float)(FRM+1)*(ovsr1*0.5+ovsr2*0.5));
#endif
	    lp12=ceil((float)FRM*(ovsr1*0.5+ovsr2*0.5));
	    lp12r=floor((float)FRM*(ovsr1*0.5+ovsr2*0.5) + 0.5);
	}


	if(HDS->decDelayMode == DM_SHORT)
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    for(i=0;i<Lp12+iflat2;i++)
#else
	    for(i=0;i<=lp12+iflat2+10;i++)
#endif
		out[i]=wave1[(i)%(SAMPLE/2)];
	}
	else
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    for(i=0;i<Lp12;i++)
#else
	    for(i=0;i<lp12;i++)
#endif
		out[i]=wave1[(i)%(SAMPLE/2)];
	}

	
	for(i=0;i<SAMPLE/2;i++)
	    phai2[i]= modu2pai(HDS->pha2[i]);
	
	ifft_am(am2,phai2,HDS->wave2);
	
	st=SAMPLE/2-(((int)lp12r)%(SAMPLE/2));

	if(HDS->decDelayMode == DM_SHORT)
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    for(i=0; i<Lp12+iflat2; i++)
#else
	    for(i=0; i<lp12+iflat2+10; i++)
#endif
		out2[i]=HDS->wave2[(st+i)%(SAMPLE/2)];
	}
	else
	{
	  /* fixing bug of upperbound of cyclic extention(99/02/15) */
#ifdef UPB
	    for(i=0; i<Lp12; i++)
#else
	    for(i=0; i<lp12; i++)
#endif
		out2[i]=HDS->wave2[(st+i)%(SAMPLE/2)];
	}

	if(HDS->decDelayMode == DM_SHORT)
	{
	    for(i=0;i<iflat;i++)
		c_dis_lp12[i]=1.0;
	    
	    ilp12=1./(lp12-(float)iflat);
	    for(i=iflat;i<(int)lp12;i++)
                c_dis_lp12[i]=((float)lp12 - (float)i)*ilp12;
	    
	    for(i=(int)lp12;i<(int)lp12+iflat2;i++)
		c_dis_lp12[i]=0.;
	    
	    for(i=0;i<(int)lp12+iflat2;i++)
		out3[i]=out[i]*c_dis_lp12[i] + out2[i]*(1.0-c_dis_lp12[i]);
#ifdef UPB
	    for(;i<Lp12+iflat2;i++)
	      out3[i]=out2[i];
#endif
	}
	else
	{
	    ilp12=1./lp12;
	    for(i=0;i<(int)lp12;i++)
		out3[i]=out[i]*(lp12-(float)i)*ilp12 + out2[i]*(float)i*ilp12;
#ifdef UPB
	    for(;i<Lp12;i++)
	      out3[i]=out2[i];
#endif
	}


	if(HDS->decDelayMode == DM_SHORT)
	{
	    sv[0]=out3[iflat];
	    ffi=0;
	    for(i=1;i<FRM;i++){
		if(i<FRM-LD_LEN)
		    ovsrc=ovsr1*((float)(FRM-LD_LEN-i)/(float)(FRM-LD_LEN)) + ovsr2*(float)i/(float)(FRM-LD_LEN);
		else
		    ovsrc=ovsr2;
		ffi=ffi+ovsrc;
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip) 
		  sv[i]=out3[(int)ffip + iflat];
		else 
		  sv[i]=(ffi-ffim)*out3[(int)ffip+iflat]+(ffip-ffi)*out3[(int)ffim+iflat];
		
#ifdef UPB
		if ((int)ffip+iflat >= Lp12+iflat2) {
		  printf("IPC_vExt_fft: undetermined value is used(frame #%d,ffip+iflat=%d,Lp12+iflat2=%d)\n",
			 (int)ffip+iflat, (int)Lp12+iflat2, HDS->fr0);
		  printf("(short delay mode, pitch continuous)\n");
#endif
		}
	    }
	}
	else
	{
	    sv[0]=out3[0];
	    ffi=0.;
	    for(i=1;i<FRM;i++){
		fi=(float)i;
		ovsrc=ovsr1*c_con[i] + ovsr2*(1.0-c_con[i]); 
		ffi=ffi+ovsrc;
	    
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip)
		    sv[i]=out3[(int)ffip];
		else
		    sv[i]=(ffi-ffim)*out3[(int)ffip]+(ffip-ffi)*out3[(int)ffim];
#ifdef UPB
		if ((int)ffip >= Lp12) {
		  printf("IPC_vExt_fft: undetermined value is used(frame #%d,ffip=%d,Lp12=%d)\n",
			 (int)ffip, Lp12, HDS->fr0);
		  printf("(normal delay mode, pitch continuous)\n");
#endif
		}
	    }
	}
    }
    else
    {
	for(i=0;i<SAMPLE/2;i++)
	    wave1[i]=HDS->wave2[i];
	
	ovsr1=(float)(SAMPLE/2.0)/pch[0];
	ovsr2=(float)(SAMPLE/2.0)/pch[1];
	
	lp1=ceil((float)FRM*ovsr1);
	lp2=ceil((float)FRM*ovsr2);
	lp2r=floor((float)FRM*ovsr2 + 0.5);

	if(HDS->decDelayMode == DM_SHORT)
	{
	    iflat=(int)floor(ovsr1*(float)LD_LEN + 0.5);
	    iflat2=(int)floor(ovsr2*(float)LD_LEN + 0.5);
	    
#ifdef UPB
	    for(i=0;i<=(int)lp1+iflat;i++)
#else
	    for(i=0;i<=lp1+iflat+10;i++)
#endif
		out[i]=wave1[(i)%(SAMPLE/2)];
	}
	else
	{
#ifdef UPB
	    for(i=0;i<=(int)lp1;i++)
#else
	    for(i=0;i<lp1;i++)
#endif
		out[i]=wave1[(i)%(SAMPLE/2)];
	}
	
	for(i=0;i<SAMPLE/2;i++)
	    phai2[i]= modu2pai(HDS->pha2[i]); 
	
	ifft_am(am2,phai2,HDS->wave2);
	
	st=SAMPLE/2-(((int)lp2r)%(SAMPLE/2));


	if(HDS->decDelayMode == DM_SHORT)
	{
#ifdef UPB
	    for(i=0; i<=(int)lp2+iflat2; i++)
#else
	    for(i=0; i<=lp2+iflat2+10; i++)
#endif
		out2[i]=HDS->wave2[(st+i)%(SAMPLE/2)];
	}
	else
	{
#ifdef UPB
	    for(i=0;i<=(int)lp2;i++)
#else
	    for(i=0;i<lp2;i++)
#endif
		out2[i]=HDS->wave2[(st+i)%(SAMPLE/2)];
	}


	if(HDS->decDelayMode == DM_SHORT)
	{
	    sv1[0]=out[iflat];
	    ffi=0.;
	    for(i=1;i<FRM;i++){
		fi=(float)i;
		ffi=ffi+ovsr1;
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip)
		    sv1[i]=out[(int)ffip+iflat];
		else
		    sv1[i]=(ffi-ffim)*out[(int)ffip+iflat]+(ffip-ffi)*out[(int)ffim+iflat];
	    }

	    sv2[0]=out2[iflat2];
	    ffi=0.;
	    for(i=1;i<FRM;i++){
		fi=(float)i;
		ffi=ffi+ovsr2;
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip)
		    sv2[i]=out2[(int)ffip+iflat2];
		else
		    sv2[i]=(ffi-ffim)*out2[(int)ffip+iflat2]+(ffip-ffi)*out2[(int)ffim+iflat2];
	    }

	    for(i=0;i<FRM;i++)
		sv[i]=sv1[i]*c_dis[i+LD_LEN]+sv2[i]*(1.0-c_dis[i+LD_LEN]);
	}
	else
	{
	    sv1[0]=out[0];
	    ffi=0.;
	    for(i=1;i<FRM;i++){
		fi=(float)i;
		ffi=ffi+ovsr1;
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip)
		    sv1[i]=out[(int)ffip];
		else
		    sv1[i]=(ffi-ffim)*out[(int)ffip]+(ffip-ffi)*out[(int)ffim];
	    }

	    sv2[0]=out2[0];
	    ffi=0.;
	    for(i=1;i<FRM;i++){
		fi=(float)i;
		ffi=ffi+ovsr2;
		ffim=floor(ffi);
		ffip=ceil(ffi);
		if(ffim == ffip)
		    sv2[i]=out2[(int)ffip];
		else
		    sv2[i]=(ffi-ffim)*out2[(int)ffip]+(ffip-ffi)*out2[(int)ffim];
	    }

	    if(vuv[0] != 0 && vuv[1] != 0)
	    {
		for(i = 0; i < FRM; i++)
		    sv[i] = sv1[i] * c_dis_v_v[i]
			+ sv2[i] * (1.0 - c_dis_v_v[i]);
	    }
	    if(vuv[0] != 0 && vuv[1] == 0)
	    {
		for(i = 0; i < FRM; i++)
		    sv[i] = sv1[i] * c_dis_v_uv[i]
			+ sv2[i] * (1.0 - c_dis_v_uv[i]);
	    }
	    if(vuv[0] == 0 && vuv[1] != 0)
	    {
		for(i = 0; i < FRM; i++)
		    sv[i] = sv1[i] * c_dis_uv_v[i]
			+ sv2[i] * (1.0 - c_dis_uv_v[i]);
	    }
	}
    }

    HDS->old_old_vuv=vuv[0];
    
}




void IPC_UvAdd(
float *pch, 
float (*am)[3], 
int *vuv, 
float *add_uv,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i,j;
    float w01;
    int send1;
    float s,bunbo;
    
    float ns[SAMPLE];
    float wns[SAMPLE];
    float wnsoz[2*FRM];

    float coefz[2*FRM+LD_LEN];

    float rms[SAMPLE];
    float ang[SAMPLE];
    float re[SAMPLE];
    float im[SAMPLE];

    float s_uv[FRM];
    float zenhan_uv[FRM];
    float kouhan_uv[FRM];
    
    int ub = 0,lb,bw;
    
    float am1[SAMPLE/2];
    int v_uv = 0;

    float	b_th1;
    float	b_th2;
    float	b_th3;

    float	guv_th1;
    float	guv_th2;
    float	guv_th3;

    float	b_th2_2;
    float	guv_th2_2;

    if(HDS->decMode == DEC2K)
    {
	b_th1 = B_TH1_2K;
	b_th2 = B_TH2_2K;
	b_th3 = B_TH3_2K;
	
	guv_th1 = GUV_TH1_2K;
	guv_th2 = GUV_TH2_2K;
	guv_th3 = GUV_TH3_2K;

	b_th2_2 = B_TH2_2;
	guv_th2_2 = GUV_TH2_2;
    }
    else
    {
	b_th1 = B_TH1_4K;
	b_th2 = B_TH2_4K;
	b_th3 = B_TH3_4K;

	guv_th1 = GUV_TH1_4K;
	guv_th2 = GUV_TH2_4K;
	guv_th3 = GUV_TH3_4K;

	b_th2_2 = B_TH2_2;
	guv_th2_2 = GUV_TH2_2;
    }



    if(vuv[0] == 0 && vuv[1] == 0)
        v_uv=0;
    if(vuv[0] == 0 && vuv[1] != 0)
        v_uv=1;
    if(vuv[0] != 0 && vuv[1] == 0)
        v_uv=2;
    if(vuv[0] != 0 && vuv[1] != 0)
        v_uv=3;

    w01 = (float)SAMPLE/pch[1];
    send1= (int)(pch[1]/2.);
    
    for(i=0; i<= send1; i++)
	am1[i]=am[i][2]*(float)SCALEFAC;
    
    for(i=send1+1; i<SAMPLE/2; i++)
	am1[i]=0.;

    /* for fixing floating point bugs on win32/linux(AI 99/02/25) */
    if (vuv[1] == 1) {
      for (i = 0; i < (float)send1*b_th1 - DELTA; i++) {
	am1[i] = 0.0f;
      }
      for (; i <= send1; i++) {
	am1[i] = am1[i] * guv_th1;
      }
    }
    else if (vuv[1] == 2) {
      for (i = 0; i < (float)send1*b_th2 - DELTA; i++) {
	am1[i] = 0.0f;
      }
      for (i = 0; i < (float)send1*b_th2_2 - DELTA; i++) {
	am1[i] = am1[i] * guv_th2; 
      }
      for (; i <= send1; i++) {
	am1[i] = am1[i] * guv_th2_2; 
      }
    }
    else if (vuv[1] == 3) {
      for (i = 0; i < (float)send1*b_th3 - DELTA; i++) {
	am1[i] = 0.0f;
      }
      for (; i <= send1; i++) {
	am1[i] = am1[i] * guv_th3; 
      }
    }
    for (; i < SAMPLE/2; i++) {
      am1[i] = 0.0f;
    }

    genwg2(ns);

    if(HDS->decDelayMode == DM_SHORT)
    {
	IPC_window(HDS->ipc_coefLD, ns, wns, SAMPLE);
    }
    else
    {
	IPC_window(HDS->ipc_coef, ns, wns, SAMPLE);
    }

    IPC_fcall_fft(wns,rms,ang,re,im);
    
    for(i=0;i<=send1;i++){
	if (i==0)
	    lb=0;
	else
	    lb=ub+1;
	if(i==send1)
	    ub=SAMPLE-1;
	else
	    ub=IPC_inint((float)i * w01 + w01/2.);
	if(ub >= SAMPLE/2)
	    ub=SAMPLE/2 ;
	bw=ub-lb+1;
	s=0.;
	for(j=lb;j<=ub; j++)
	    s=s+rms[j]*rms[j];
	s=sqrt(s/(float)bw);
	for(j=lb;j<=ub;j++){
	    rms[j]=am1[i]*rms[j]/s;    
	}
    }
    
    fcall_ifft(rms,ang,wns);
    
    zeropad(wns,wnsoz); 
    
    if(HDS->decDelayMode == DM_SHORT)
    {
	zeropad(HDS->ipc_coefLD,coefz);

	for(i=320;i<320+LD_LEN;i++)
	    coefz[i]=0.;
    }
    else
    {
	zeropad(HDS->ipc_coef,coefz);
    }

    if(HDS->decDelayMode == DM_SHORT)
    {
	for(i=0;i<FRM;i++){
	    bunbo=coefz[FRM+i+LD_LEN]*coefz[FRM+i+LD_LEN]+coefz[i+LD_LEN]*coefz[i+LD_LEN];
	    s_uv[i]=(HDS->wnsozp[i+LD_LEN]*coefz[FRM+i+LD_LEN]+wnsoz[i+LD_LEN]*coefz[i+LD_LEN])/bunbo;
	}
	
	for(i=0;i<80;i++){
	    bunbo=coefz[FRM+i+LD_LEN]*coefz[FRM+i+LD_LEN]+coefz[i+LD_LEN]*coefz[i+LD_LEN];
	    zenhan_uv[i]=HDS->wnsozp[i+LD_LEN]*coefz[FRM+i+LD_LEN]/bunbo;
	}
	for(i=80;i<FRM;i++)
	    zenhan_uv[i]=0.;
	
	for(i=0;i<FRM;i++){
	    bunbo=coefz[FRM+i+LD_LEN]*coefz[FRM+i+LD_LEN]+coefz[i+LD_LEN]*coefz[i+LD_LEN];
	    kouhan_uv[i]=wnsoz[i+LD_LEN]*coefz[i+LD_LEN]/bunbo;
	}
    }
    else
    {
	for(i=0;i<FRM;i++){
	    bunbo=coefz[FRM+i]*coefz[FRM+i]+coefz[i]*coefz[i];
	    s_uv[i]=(HDS->wnsozp[FRM+i]*coefz[FRM+i]+wnsoz[i]*coefz[i])/bunbo;
	}
	
	for(i=0;i<SAMPLE/2;i++){
	    bunbo=coefz[FRM+i]*coefz[FRM+i]+coefz[i]*coefz[i];
	    zenhan_uv[i]=HDS->wnsozp[FRM+i]*coefz[FRM+i]/bunbo;
	}
	for(i=SAMPLE/2;i<FRM;i++)
	    zenhan_uv[i]=0.;
	for(i=0;i<(SAMPLE/2-OVERLAP);i++)
	    kouhan_uv[i]=0.;
	for(i=(SAMPLE/2-OVERLAP);i<FRM;i++){
	    bunbo=coefz[FRM+i]*coefz[FRM+i]+coefz[i]*coefz[i];
	    kouhan_uv[i]=wnsoz[i]*coefz[i]/bunbo;
	}
    }

    if(v_uv == 0){
	for(i=0;i<FRM;i++)
		add_uv[i]=0.;
    }
    else if(v_uv == 1){
	for(i=0;i<FRM;i++)
		add_uv[i]=kouhan_uv[i];
    }
    else if(v_uv == 2){
	for(i=0;i<FRM;i++)
		add_uv[i]=zenhan_uv[i];
    }
    else if(v_uv == 3){
	for(i=0;i<FRM;i++)
		add_uv[i]=s_uv[i];
    }
    else{
	fprintf(stderr, "v_uv flag ERROR \n");
        exit(10);
    }

    if(HDS->decDelayMode == DM_SHORT)
    {
	for(i=0;i<FRM;i++)
	    HDS->wnsozp[i]=wnsoz[i+FRM];
	for(i=FRM;i<FRM*2;i++)
	    HDS->wnsozp[i]=0.;
    }
    else
    {
	for(i=0;i<2*FRM;i++)
	    HDS->wnsozp[i]=wnsoz[i];
    }
}






