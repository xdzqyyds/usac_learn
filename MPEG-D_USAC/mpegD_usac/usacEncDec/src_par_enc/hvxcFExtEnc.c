
/*


This software module was originally developed by

    Kazuyuki Iijima, Masayuki Nishiguchi, Jun Matsumoto (Sony Corporation)

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

#include "hvxc.h"
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#define	VUVBAND	12

#define THRESHVUV	0.2
#define THRESHVUVC	0.2

#define HVXC_MAXFLOAT ((float)1e+35)	/* HP 971104 */

extern float	ipc_coef[SAMPLE];

extern float	ipc_bsamp[SAMPLE * R + 1];

extern int      ipc_encMode;

static void vuv_reduce(
float	*pch,
float	(*am)[3],
float	*rms,
int	*vuvflg,
int	*vuvcode,
float	fb)
{
    int i,j,send,ub_r,lb_r,id;
    float s,dif,sdif,w0,w0_r,nsr,pitch;
    float oamp[SAMPLE/2];
    float sums,sumsdif;
    static int cnt = 0;
    float vuvbw;
    int bandnum;
    float bunshi,bunbo;
    
    float	reducedNSR[VUVBAND];

    float	dynamic_thre;
    
    pitch = *pch;
    w0 = (float) SAMPLE * (float) R / (pitch+EPS);   
    w0_r = (float) SAMPLE / (pitch+EPS);   
    send= (int) (pitch / 2.);
    vuvbw = (float) (SAMPLE / 2) / (float) VUVBAND;
    bandnum = 1;
    bunshi = 0.0;
    bunbo = 0.0;
    sums = 0.0;
    sumsdif = 0.0;
    ub_r = 0;


    for(i=0; i<=send; i++)
    {
	if(i==0)
	    lb_r=0; 
	else 
	    lb_r=ub_r+1; 
	
	if (i==send) 
	    ub_r=SAMPLE / 2 - 1;
	else 
	    ub_r=IPC_inint((float)i * w0_r + w0_r/2.);
	
	
	if(ub_r >= SAMPLE/2 ) 
	    ub_r=SAMPLE/2 -1;
	
	s = 0.0;
	sdif = 0.0;
	for(j=lb_r; j<=ub_r; j++)
	{
	    id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i * w0);
	    s += rms[j] * rms[j];
	    dif = rms[j] -ipc_bsamp[id] * am[i][2];
	    oamp[j] = ipc_bsamp[id] * am[i][2];
	    sdif += dif * dif;
	}
	
	if(s==0.0) s=0.0000001;
	nsr=sdif/s;
	dynamic_thre = THRESHVUV;	
	
	if(nsr > dynamic_thre || (pitch==(float)(20+NUMPITCH/2)) || fb==0.)
	{
	    vuvflg[i]=0;
	}
	else
	{
	    vuvflg[i]=1;
	}
	
	if(((float)i * w0_r) <= ((float)bandnum * vuvbw ))
	{
	    bunshi += nsr*am[i][2];
	    bunbo += am[i][2];
	    if(i == send)
	    {
		reducedNSR[VUVBAND - 1] = bunshi / (bunbo+EPS);

		if((bunshi/(bunbo+EPS))>dynamic_thre ||
		   (pitch==(float)(20+NUMPITCH/2)) ||
		   fb==0.0)
		{
		    vuvcode[VUVBAND-1]=0;
		}
		else
		{
		    vuvcode[VUVBAND-1]=1;
		}
	    }
	}
	else
	{
	    reducedNSR[bandnum - 1] = bunshi / (bunbo+EPS);
	    if((bunshi/(bunbo+EPS))>dynamic_thre || (pitch==(float)(20+NUMPITCH/2))
	       || fb==0.0)
		vuvcode[bandnum-1]=0;
	    else
		vuvcode[bandnum-1]=1;
	    
	    bunshi=nsr*am[i][2];
	    bunbo=am[i][2];
	    bandnum++;
	}
	
	sumsdif += sdif;
	sums += s;
    }
    if(sumsdif==0.) sumsdif=0.0000001;
    if(sums==0.) sums=0.0000001;
    
    cnt++;
}

static void restore_vuv_e(
short int	*vuvPos,
float		vh,
int		*vuv,
float		*mfdpch,
int		numZeroXP,
float		tranRatio,
float		*r0r,
float		fb,
float		lev)
{
    int		i;

    IPC_VUVDecisionRule(fb, lev, vuvPos, vh, numZeroXP, tranRatio, r0r, *mfdpch);

    if( *mfdpch > 145)
	*vuvPos = 0;
    
    if( *vuvPos == 0 )
	*mfdpch=(float)(20+NUMPITCH/2);
    
    for(i=0; i< *vuvPos; i++)
	vuv[i] = 1;
    
    for(i= *vuvPos; i<VUVBAND; i++)
	vuv[i] = 0;
}


static int edge_n(int e, int *s)
{
    while( (s[e]^s[e+1]) == 0 ) {
	e++;
	if ( e==VUVBAND-1 )
	    break;
    }
    return( ++e );
}


static int transient_n(int *s)
{
    int		tr,i;
    
    tr = 0;
    for(i=1; i<VUVBAND; i++){
	if (s[i]^s[i-1])
	    tr++;
    }
    
    return( tr );
}

static int search_vuv_edge(int *orig_vuv)
{
#define     VRATE_THRE 	0.8
#define     V_BALANCE   4
    
    int		i;
    int		last_1,cnt_of_1; 
    int		local_vuv[ VUVBAND ];
    
    if(transient_n(orig_vuv)>1)
    {
	i=VUVBAND-1;
	
	while(i>0 && orig_vuv[i]==0)
	    i--;
	
	last_1=i;
	
	for(i = cnt_of_1 = 0; i <= last_1; i++)
	{
	    if (orig_vuv[i])
		cnt_of_1++;
	}
	if ((float) cnt_of_1 / (float)last_1 > VRATE_THRE)
	{
	    for(i=0; i<=last_1; i++)
		local_vuv[i] = 1;
	    for(i=last_1+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	else if (last_1<=3 && (float)cnt_of_1/(float)last_1>0.5)
	{
	    for(i=0; i<=last_1; i++)
		local_vuv[i] = 1;
	    for(i=last_1+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	else
	{
	    for(i=0; i<=last_1/V_BALANCE; i++)
		local_vuv[i] = 1;
	    
	    for(i=last_1/V_BALANCE+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	
	if (!orig_vuv[0] && !orig_vuv[1] && local_vuv[0] && local_vuv[1])
	    return( 0 );
	else
	    return( edge_n( 0,local_vuv ) );
    }
    else if ( transient_n( orig_vuv )==1 )
    {
	if ( orig_vuv[0]==1 )
	{
	    return(edge_n( 0, orig_vuv ));
	}
	else
	{ 
	    if(edge_n(0,orig_vuv)<4)
		return(VUVBAND);
	    else
		return( 0 );
	}
    }
    else if( orig_vuv[0]==1 )
    {
	return( VUVBAND );
    }
    else
    {
	return( 0 );
    }
}




static int CntZeroXP(float *sbuf)
{
    int		i;
    int		numZeroXP;

    numZeroXP = 0;
    for(i = 48; i < FRM + 48; i++)
    {
	if(((sbuf[i] < 0.0) && (sbuf[i - 1] > 0.0)) ||
	   ((sbuf[i] > 0.0) && (sbuf[i - 1] < 0.0)))
	{
	    numZeroXP++;
	}
    }
    
    return(numZeroXP);
}    

static float transient(
float	*sbuf,
int	n)
{
    int         i;
    float       po1,po2,po2rms,ratio;

    po1 = 0.;
    for(i = 0; i < 90; i++)
	po1 += sbuf[i] * sbuf[i];

    po2 = 0.;
    for(i = (n-90); i < n; i++)
	po2 += sbuf[i] * sbuf[i];

    if(po1 >= 1 )
    	ratio = po2/po1;
    else
	ratio = 0.;
    
    po2rms = sqrt(po2/90.);

    if(po2rms <= 1000.)
	ratio=0.;

    return(ratio);
}    


#define BLNUM	32

static void FreqBalance(
float	*rms,
float	*arys,
float	*fb,
float	*lev)
{
    int i,j;
    float lbl,hbl,level,th;
    float ene[BLNUM],ave,avem,std;
    float pw,peak,av,rmsf;
    short rmss;
    int grn;
    static int stdm=0;
    static int  co=0;
    static float pl=0.;
    float devi,l1devi;

    lbl=0.;
    for(i=0; i<SAMPLE/4; i++)
	lbl=lbl+rms[i]*rms[i];
    hbl=0.;
    for(i=SAMPLE/4;i<SAMPLE/2; i++)
	hbl=hbl+rms[i]*rms[i];

    for(i=0;i<BLNUM;i++)
	ene[i]=0.;
    for(i=0;i<BLNUM;i++){
	for(j=0;j<SAMPLE/BLNUM;j++)
	    ene[i] += arys[i*SAMPLE/BLNUM+j]*arys[i*SAMPLE/BLNUM+j];
    }

    for(i=0;i<BLNUM;i++)
	ene[i]=sqrt(ene[i]/(float)(SAMPLE/BLNUM));
    for(i=0, ave=0.0;i<BLNUM;i++)
	ave += ene[i];
    ave=ave/BLNUM;

    avem=1.;
    for(i=0;i<BLNUM;i++){
	if(ene[i] != 0)
	    avem = avem * pow(ene[i], 1./(float)BLNUM);
	else
	    avem = 0.;
    }

    for(i=0, devi=l1devi=0;i<BLNUM;i++){
	devi += (ene[i]-ave)*(ene[i]-ave);
	l1devi += fabs(ene[i]-ave);
    }
    devi=sqrt(devi/(float)BLNUM);
    l1devi=(l1devi/(float)BLNUM);
    if(ave>0.001){
	devi=devi/ave;
	l1devi=l1devi/ave;
    }
    else{
	devi=0.1;
	l1devi=0.1;
    }

    if(avem > 0.01)
	std = ave/avem;
    else
	std = 1.;

    for(i=0, av=0.0;i<SAMPLE;i++)
	av += arys[i];
    av=av/SAMPLE;

    for(i=0,pw=peak=0.0;i<SAMPLE;i++){
	pw += (arys[i]-av)*(arys[i]-av);
	if(fabs(arys[i]-av)>peak) 
	    peak=fabs(arys[i]-av);
    }
    rmsf=sqrt(pw/(float)SAMPLE);

    rmss=(short)rmsf & 0xffff;
    if(rmss <= 7.5)
	grn=1;
    else
	grn=0;

    if(hbl != 0. )
	*fb= lbl/hbl ;
    else
	*fb= 20.;
    if(hbl ==0. && lbl==0.) *fb=0.;

    *lev = level = sqrt((lbl+hbl)/(SAMPLE/2));

    if(level > pl)
	pl = level;

    if(pl >0. && level>0.)
	th=20. * log10(level/pl);
    else
	th= -60.;

    if (th <= -60.)
	th= -60.;

    if(devi < 0.4)  
	stdm++;
    else
	stdm=0;

    if(stdm >=5 )
	stdm =5;

    if(grn==1)
    {
	*fb=0;
    }
    else if((*fb < 5.) && (level < 550.) && (stdm >=1))
    {
	*fb=0.;
    }
    else if( (level < 550. && devi < 0.45) || (level < 600. && devi < 0.4))
    {
	*fb=2.;
    }
    else
    {
	*fb=1.;
    }

    co++;
}

static void ModifyFB(
float	*fb,
float	*lev)
{
    
    float  fbb;
    float  levv;
    static float    threshold=10.;
    static float    fb0=0.;
    static float    fb1=0.;
    static float    fb2=0.;
    static float    fb3=0.;
    static float    fb4=0.;
    static float    fb5=0.;
    
    fbb = *fb;
    levv = *lev;
    
    fb5 = fb4;
    fb4 = fb3;
    fb3 = fb2;
    fb2 = fb1;
    fb1 = fb0;
    fb0 = *fb;  
    
    if( ( fb5+fb4+fb3+fb2+fb1+fb0)==0. )
	threshold = 10.;
    
    if( levv> 500. && threshold !=200.)
	threshold =  90.;

    if( levv>  2000.)
	threshold = 200.;

    if( levv<threshold && fbb!=0 )
	*fb = 0.;
}


static void PostVFineModeDecisionVR(
int     *vuvFlg,
float   *r0r,
float   r0h,
float   fb,
float   lev,
float   refLev,
float   gml,
int     cont)
{
    static int oldvuvFlg[2] = {0, 0};
    int onset;

    static int bgnCnt = 0;
    static int bgnIntvl = 0;

    if(oldvuvFlg[0] == 0 && oldvuvFlg[1] == 0 && *vuvFlg == 1)
    {
        onset = 1;
    }
    else
    {
        onset = 0;
    }


    if(*vuvFlg != 0)
    {
        if(r0h < 0.20)
        {
            /* mixed voiced UV */
            *vuvFlg = 2;
        }
        else if(r0r[1] < 0.7)
        {
            /* mixed voiced V */
            *vuvFlg = 2;
        }
        else
        {
            /* perfect V */
            *vuvFlg = 3;
        }

        if(ipc_encMode == ENC2K)
            if(refLev < gml * 2.0 && cont < 2)
            {
                *vuvFlg = 1;
            }
    }

    oldvuvFlg[1] = oldvuvFlg[0];
    oldvuvFlg[0] = *vuvFlg;

}

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define max(x, y)       (x > y ? x : y)
#define min(x, y)       (x < y ? x : y)

static int PostUVFineModeDecisionVR(
int     *idVUV,
int     dec,
float   lev,
float   fb,
float   tranRatio,
float   refLev,
float   gml)
{
    static int bgnCnt = 0;
    static int bgnIntvl = 0;

    if(ipc_encMode == ENC2K) {
        if(*idVUV == 0)
        {
            if(refLev < gml * 2.0)
            {
                if(bgnCnt < 3)
                {
                    bgnCnt++;
                }
                else
                {
                    if(bgnIntvl < BGN_INTVL)
                    {
                        *idVUV = 1;
                        bgnIntvl++;
                    }
                    else
                    {
                        bgnIntvl = 0;
                    }
                }
            }
            else
            {
                bgnCnt = 0;
            }
        }
    } else {
	if(*idVUV == 0) {
	    if (dec == 1) {
		bgnCnt = 0; bgnIntvl = 0;
            } else if (dec == 0 || dec == 2) {
		if (dec == 0) bgnCnt++;

		if (bgnCnt == BGN_CNT) {
		    bgnIntvl = 0;
		    *idVUV = 1;
                } else if (bgnCnt > BGN_CNT) {
		    bgnCnt = BGN_CNT + 1;
		    *idVUV = 1;
		    if (dec == 2) {
                        bgnIntvl = 0;
                    } else {
			bgnIntvl++;
			if (bgnIntvl >= BGN_INTVL_TX) bgnIntvl = 0;
                    }
                }
            }
        } else {
            bgnCnt = bgnIntvl = 0;
	}
    }

    if (bgnIntvl == 0) return (1); else return (0);	/* upFlag */
}

static void VFineModeDecision(
int	*vuvFlg,
float	*r0r,
float	r0h)
{
    static int oldvuvFlg[2] = {0, 0};
    int onset;

    if(oldvuvFlg[0] == 0 && oldvuvFlg[1] == 0 && *vuvFlg == 1)
    {
	onset = 1;
    }
    else
    {
	onset = 0;
    }
    

    if(*vuvFlg != 0)
    {
        if(r0h < 0.20)
        {
            *vuvFlg = 1;
        }
        else if(r0r[1] < 0.7)
        {
            *vuvFlg = 2;
        }
        else
        {
            *vuvFlg = 3;
        }
    }

    oldvuvFlg[1] = oldvuvFlg[0];
    oldvuvFlg[0] = *vuvFlg;
}    

void vuv_decision(
float	*arys,
float	*mfdpch,
float	(*am)[3],
float	*rms,
float	*r0r,
int	*vuvFlg,
float   r0h)
{
    int		i;
    int		vuvflg[SAMPLE/2];
    int		vuvcode[VUVBAND];
    float	tranRatio;
    int		numZeroXP;

    float	aryd[SAMPLE];
    float	ang2[SAMPLE], re2[SAMPLE], im2[SAMPLE], rms2[SAMPLE];

    short	vuvPos;

    static float	fb[2], lev[2];

    float	arysTmp[SAMPLE];

    static int	frm = 0;

    float	w;

    float	vh, sum;

    for(i = 0; i < SAMPLE; i++)
    {
	arysTmp[i] = arys[i];
    }

    IPC_hp_filterp2(arysTmp, frm);

    IPC_window(ipc_coef, arysTmp, aryd, SAMPLE);
    IPC_fcall_fft(aryd, rms2, ang2, re2, im2);
    
    FreqBalance(rms2, arys, &fb[0], &lev[0]);

    if(fb[0] == 2.0)
    {
	if (r0r[1] < 0.4)
	{
	    fb[0] = 0.0;
	}
	else
	{
	    fb[0] = 1.0;
	}
    }
    
    if(r0r[1] > 0.5)
    {
	fb[0] = 1.0;
    }

    ModifyFB(&fb[0],&lev[0]);

    numZeroXP = CntZeroXP(arysTmp);

    tranRatio = transient(arysTmp, SAMPLE);

    vuv_reduce(mfdpch, am, rms, vuvflg, vuvcode, fb[0]);
    
    vh = 0.0;
    sum = 0.0;
    for(i = 1; i < (int) (*mfdpch / 2.0); i++)
    {
	w = 1.0;
	vh += vuvflg[i] * w;
	sum += w;
    }

    vh /= sum;

    vuvPos = (short) search_vuv_edge( vuvcode );

    restore_vuv_e(&vuvPos, vh, vuvcode , mfdpch, numZeroXP, tranRatio, r0r,
		  fb[0], lev[0]);

    if(vuvPos == 0)
    {
	*vuvFlg = 0;
    }
    else
    {
	*vuvFlg = 1;
    }

    VFineModeDecision(vuvFlg, r0r, r0h);

    frm++;

}

static float calc_alpha256(float *ary, float *alpha, int p)
{
    int i,j,k,kk;
    float ary_w[SAMPLE];
    float r[P+1];
    float a0[P+1];
    float kpara[P+1];
    float u;

    /* original lag window */
    static float lag_w[P+1] = 
    {
	1.0,
	0.9994438,
	0.9977772,
	0.9950056,
	0.9911382,
	0.9861880,
	0.9801714,
	0.9731081,
	0.9650213,
	0.9559375,
	0.9458861,
    };

    for(i=0;i<SAMPLE;i++)
    {
	ary_w[i]=ary[i] * ipc_coef[i];
    }
    
    /** calc auto corr **/
    
    for(i=0;i<=p;i++){
	r[i]=0;
	for(j=0;j<SAMPLE-i;j++)   

	    r[i] += ary_w[j]*ary_w[j+i];
    }
    if(r[0]<=0.0000001){
	alpha[0]=1;
	for(i=1;i<=LPCORDER;i++)
	    alpha[i]=0.;
	return(0.0);
    }
    
    /** mpy lag window to auto corr **/
    
    r[0] *= 1.0001;

    for(i=0;i<=p;i++)
	r[i] *= lag_w[i];
    
    /**** Levinson Durbin ****/
    
    u = r[0];
    a0[0]= alpha[0] = 1.0;
    
    for(i=1;i<=p; i++){
	
	/* calc Ai(i) */
	alpha[i]=r[i];
	for(j=1;j<i;j++)
	    alpha[i] += a0[j]*r[i-j];
	alpha[i] = -alpha[i]/u;
	kpara[i] = alpha[i];
	
	/* calc A1(i)...Ai-1(i) */
	
	for(k=1;k<i;k++)
	    alpha[k]=a0[k]+alpha[i]*a0[i-k];
	
	/* update u */
	u=(1.0-alpha[i]*alpha[i])*u;
	
	for(j=0;j<=i;j++)
	    a0[j]=alpha[j];
	
	/* stop LPC analysis */
	if(( u/r[0] < 0.000001) && (i+1<=p )){
	    fprintf(stderr, "stop lpc %d\n", i + 1);
	    for(kk=i+1;kk<=p;kk++)
		alpha[kk]=0.;
	    break;
	}
    }
    return(u/r[0]);
}

#define CDIM 16

static void cepstrum( float *lpc_a, float *CL )
{
    float alp[CDIM+1];
    int i, l, d, m;
    float tmp;

    for (i = 1; i <= P; i++) alp[i] = lpc_a[i];
    for (i = P+1; i <= CDIM; i++) alp[i] = 0;

    CL[1] = -alp[1];
    for (m = 2; m <= CDIM; m++) {
	for (l = 1, tmp = 0; l < m; l++)
	    tmp += (float)l * CL[l] * alp[m-l];
        CL[m] = -alp[m] - tmp / (float)m;
    }
}

static void responce( float *CL, float *logAmp )
{
    float th1, th2, thd = M_PI/8;
    float tmp, H[5];
    int i, m;

    H[0] = 0;

    for (i = 1, th2=thd; i < 5; i++, th2+=thd) {
	for (m = 1, tmp = 0; m <= CDIM; m++) {
	    tmp += CL[m] / (float)m * sin( th2 * (float)m );
        }
	H[i] = tmp / thd;
    }
    for (i = 0; i < 4; i++)
	logAmp[i] = H[i+1] - H[i];
}

static void rms_border( float *s, float *bdrms )
{
    int i, j;
    float ene[SAMPLE/BLNUM];
    float eng0, eng1, t0, t1, mul0, mul1;

    bdrms[0] = bdrms[1] = 0;

    for (i = 0; i < SAMPLE/BLNUM; i++) {
	eng0 = 0;
	for (j = 0; j < BLNUM; j++)
	    eng0 += s[i*BLNUM+j] * s[i*BLNUM+j];
        ene[i] = eng0;
    }
    eng0 = ene[0];
    for (i = 1, eng1 = 0; i < SAMPLE/BLNUM; i++) eng1 += ene[i];
    for (i = j = 2, mul1 = 0.0; i < 7; i++) {
	eng0 += ene[i-1]; t0 = eng0 / (float)(i*BLNUM);
	eng1 -= ene[i-1]; t1 = eng1 / (float)(SAMPLE-i*BLNUM);
	if (t0 > t1)
	    mul0 = t0 / (t1+1.0e-6);
        else
	    mul0 = t1 / (t0+1.0e-6);
        if (mul0 > mul1) {
	    j = i; mul1 = mul0;
	    bdrms[0] = sqrt( t0 ); bdrms[1] = sqrt( t1 );
        }
    }
}

#define STD_LEVEL 1000.0
#define MIN_LEVEL   16.0	/* revious : 32 */

static int fuzzy_logic( float *logAmp, float *rms )
{
    int i, j, k, l, dec;
    float avAmp[4], out[3], eva1[3], eva2[3], wdif;
    float tmp, tmp2, maxval, minval;
    float near_rms, tout[3];
    float big_rms, small_rms, trms[4];
    float num, denom, tot_eval;

    static int st_cnt = 0;
    static float min_rms=100.0, strms[4], far_rms[2];
    static float slAmp[4][4];

    if (rms[0] > rms[1]) {
	big_rms = rms[0]; small_rms = rms[1];
    } else {
	big_rms = rms[1]; small_rms = rms[0];
    }

    for (i = 0; i < 4; i++) {
	avAmp[i] = 0;
	for (j = 0; j < 4; j++) avAmp[i] += slAmp[j][i];
	avAmp[i] /= 4;
    }
    for (i = 0, wdif = 0; i < 4; i++)
	wdif += (logAmp[i] - avAmp[i]) * (logAmp[i] - avAmp[i]);
    wdif = sqrt( wdif / 4 );

    for (i = 0; i < 4; i++) {
	for (j = 0; j < 3; j++) slAmp[j][i] = slAmp[j+1][i];
        slAmp[3][i] = logAmp[i];
    }

/* fuzzy classification for CL */

    if (big_rms < MIN_LEVEL) {
	eva1[0] = 1.0; eva1[1] = eva1[2] = 0.0;
    } else if (wdif < 0.1) {
	eva1[0] = wdif / 0.1; eva1[1] = 0; eva1[2] = 1.0 - eva1[0];
    } else if (wdif < 0.125) {
	eva1[0] = 1.0; eva1[1] = eva1[2] = 0;
    } else if (wdif < 0.225) {
        eva1[0] = (0.225 - wdif) / 0.1; eva1[1] = 1.0 - eva1[0];
	eva1[2] = 0.0;
    } else if (wdif < 0.3) {
	eva1[0] = eva1[2] = 0.0; eva1[1] = 1.0;
    } else if (wdif < 0.4) {
	eva1[0] = 0.0; eva1[1] = (0.4 - wdif) / 0.1;
	eva1[2] = 1.0 - eva1[1];
    } else {
	eva1[0] = eva1[1] = 0.0; eva1[2] = 1.0;
    }

/* fuzzy classification for RMS */

    near_rms = 0;

    if (st_cnt >= 4) {
	for (j = 0; j < 4; j++) trms[j] = strms[j];
	for (j = 0; j < 2; j++) {
	    maxval = 0;
	    for (k = l = j; k < 4; k++) {
		if (trms[k] > maxval) {
		    maxval = trms[k];
		    l = k;
                }
            }
	    tmp = trms[j];
	    trms[j] = trms[l];
	    trms[l] = tmp;
        }
	near_rms = trms[1];

	if (far_rms[0] < far_rms[1]) minval = far_rms[0];
	else minval = far_rms[1];
        if (near_rms < minval) minval = near_rms;

	if (minval > min_rms)
	    min_rms = 0.8 * min_rms + 0.2 * minval;

        far_rms[0] = far_rms[1];
	far_rms[1] = near_rms;
    }

    maxval = (big_rms < STD_LEVEL) ? big_rms : STD_LEVEL;

    if (maxval < min_rms)
	min_rms = 0.4 * min_rms + 0.6 * maxval;
    else
        min_rms = min_rms + maxval * 0.001;

    if (min_rms < MIN_LEVEL) min_rms = MIN_LEVEL;

    if (big_rms < 2 * small_rms && big_rms < STD_LEVEL) {
	if (st_cnt < 4) st_cnt++;
    } else st_cnt = 0;

    for (j = 0; j < 3; j++) strms[j] = strms[j+1];
    strms[3] = big_rms;

    tmp = big_rms/min_rms;
    if (tmp < 1) { eva2[0] = 1; eva2[2] = 0; }
    else if (tmp < 2) { eva2[0] = 2 - tmp; eva2[2] = 1 - eva2[0]; }
    else { eva2[0] = 0; eva2[2] = 1; }
    eva2[1] = eva2[0];

/* fuzzy decision */
    out[0] = out[1] = out[2] = 0;
    if (eva1[2] < 0.9999 && eva2[2] < 0.9999) {
	out[0] = (eva1[0] < eva2[0]) ? eva1[0] : eva2[0];
	out[1] = (eva1[1] < eva2[1]) ? eva1[1] : eva2[1];
	out[2] = (eva1[2] < eva2[2]) ? eva1[2] : eva2[2];
	num = 0.1389 * out[0] * (1.0 - out[0] / 3.0) / 2.0
	    + 0.5000 * out[1] * (2.0 / 3.0 - out[1] / 3.0)
	    + 0.8611 * out[2] * (1.0 - out[2] / 3.0) / 2.0; 
	denom =  out[0] * (1.0 - out[0] / 3.0) / 2.0
	    + out[1] * (2.0 / 3.0 - out[1] / 3.0)
	    + out[2] * (1.0 - out[2] / 3.0) / 2.0;
        tot_eval = num / denom;
    } else {
	tot_eval = 1;
    }

    if (tot_eval < 0.34) dec = 0;
    else if (tot_eval < 0.66) dec = 2;
    else dec = 1;

    return ( dec );
}

int HvxcVUVDecisionVR(

float   *arys,
float   pitch,
float   *r0r,
float   r0h,
int     *idVUV)
{
    int         i;
    float       arysTmp[SAMPLE];
    float       aryd[SAMPLE];
    int         numZeroXP;
    float       tranRatio;
    float       rms2[SAMPLE], ang2[SAMPLE], re2[SAMPLE], im2[SAMPLE];

    int         dec, upFlag;
    float       alpha_v[P+1], CL[CDIM], logAmp[4], bdrms[2];

    static float        fb = 0.0, lev = 0.0;


    static float refLev = 1000.0;

    static int  frm = 0;

    static int  prevVUV, cont = 0;

    static float        gml = HVXC_MAXFLOAT;         /* Global Minimum Level */

    for(i = 0; i < SAMPLE; i++)
    {
        arysTmp[i] = arys[i];
    }

    IPC_hp_filterp2(arysTmp, frm);

    IPC_window(ipc_coef, arysTmp, aryd, SAMPLE);
    IPC_fcall_fft(aryd, rms2, ang2, re2, im2);

    /* fb 0:unvoiced, 1:voiced, 2:unknown */

    FreqBalance(rms2, arys, &fb, &lev);


    if(fb == 2.0)
    {
        if (r0r[1] < 0.4)
        {
            fb = 0.0;
        }
        else
        {
            fb = 1.0;
        }
    }

    if(r0r[1] > 0.5)
    {
        fb = 1.0;
    }

    ModifyFB(&fb,&lev);

    numZeroXP = CntZeroXP(arysTmp);

    tranRatio = transient(arysTmp, SAMPLE);

    HvxcVUVDecisionRuleVR(fb, lev, numZeroXP, tranRatio, r0r, pitch, &gml, idVUV);

    calc_alpha256(arysTmp, alpha_v, P);
    cepstrum( alpha_v, CL );
    responce( CL, logAmp );
    rms_border( arysTmp, bdrms );

    dec = fuzzy_logic( logAmp, bdrms );

    if(prevVUV == *idVUV)
    {
        cont++;
    }
    else
    {
        cont = 0;
    }

    prevVUV = *idVUV;

    refLev = 0.75 * max(lev, refLev) + 0.25 * min(lev, refLev);

    PostVFineModeDecisionVR(idVUV, r0r, r0h, fb, lev, refLev, gml, cont);
    upFlag =
        PostUVFineModeDecisionVR(idVUV, dec, lev, fb, tranRatio, refLev, gml);

    frm++;

    return ( upFlag );
}

