
/*


This software module was originally developed by

    Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)

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
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#define		RSD_SIZE	256
#define		AL		512
#define		POWER		9

#define		Lmin		20
#define		Lmax		148
#define		FILTER_ORDER	 18

#define		PEAKMAX		(Lmax-Lmin)

#define		FB_STATE_HOLD	4

#define		Np		10

#define		ACL		512


#define		LPF_PI3per4	0
#define		LPF_PIper2	1
#define		LPF_PI3per8	2
#define		LPF_PIper4	3
#define		LPF_PIper8	4

#define		LPF_OFFSET	5

#define		HPF_PI3per4	(0 + LPF_OFFSET)
#define		HPF_PI4per5	(1 + LPF_OFFSET)
#define		HPF_PI7per8	(2 + LPF_OFFSET)
#define		HPF_PI1per8	(3 + LPF_OFFSET)
#define		HPF_PI1per4	(4 + LPF_OFFSET)
#define		THROUGH		(5 + LPF_OFFSET)

#include	"hvxcPchEnc.h"


#define		LARGELEVEL	2000.0


extern int	ipc_encDelayMode;
extern int	ipc_decDelayMode;

typedef struct
{
    float	pitch, framePower, prob, r0r, peak0, ac[AL];
    float	engRatio;
    int		scanLmt, peakPos[PEAKMAX];
}
PitchPrm;

typedef struct 
{
    float	pitch;
    float	prob;
    float	r0r;
    float	rawR0r;
    float	rawPitch;
}
NgbPrm;

static float	global_pitch = 0.0;
static float	prevGp = 0.0;

extern float	ipc_coef[SAMPLE];

extern int      judge;
extern int HVXCclassifierMode;

static void swap_f(
float	s[],
int	i,
int	j)
{
	float	tmp;

	tmp = s[i];
	s[i] = s[j];
	s[j] = tmp;
}


static void swap_i(
int	s[],
int	i,
int	j)
{
	int	tmp;

	tmp = s[i];
	s[i] = s[j];
	s[j] = tmp;
}

static void sort(
int	v[],
int	begin,
int	end)
{
	int		i,last;

	if ( begin>=end )
		return;

	swap_i( v, begin, (begin+end)/2 );

	last = begin;

	for(i=begin+1; i<=end; i++)
		if (v[i]<v[begin]){
			swap_i(v, ++last, i);
		}

	swap_i( v, begin, last );
	sort( v, begin, last-1 );
	sort( v, last+1, end );
}

static void sort2(
float	v[],
int	begin,
int	end,
int	w[])
{
	int		i,last;

	if ( begin>=end )
		return;

	swap_f( v, begin, (begin+end)/2 );
	swap_i( w, begin, (begin+end)/2 );

	last = begin;

	for(i=begin+1; i<=end; i++)
		if (v[i]>v[begin]){
			swap_f(v, last+1, i);
			swap_i(w, ++last, i);
		}

	swap_f( v, begin, last );
	swap_i( w, begin, last );
	sort2( v, begin, last-1, w );
	sort2( v, last+1, end, w );
}

static void auto_corr(
int	n,
int	p,
float	w[],
float	x[],
float	r[])
{
    int	i,j;
    static float	wx[ ACL ];

    
    for(i=0; i<n; i++)
	wx[i] = w[i] * x[i];
    
    for(i=0; i<=p; i++)
    {
	r[i] = 0.0;
	for(j=0; j<=n-1-i; j++)
	{
	    r[i] += wx[j] * wx[j+i];
	}
    }
}

static float lpc2(
int	n,		/* data number */
float	w[],		/* window data */
float	x[],		/* data */
int	p,		/* LPC order */
float	a[])		/* (disired) LPC coef */
{
	float	u;
	float	*r;
	int		i, j, k, l;
	float	a0[Np+1];

	float	lag_w[11];

	lag_w[0]=1.0;
	lag_w[1]=0.9994438;
	lag_w[2]=0.9977772;
	lag_w[3]=0.9950056;
	lag_w[4]=0.9911382;
	lag_w[5]=0.9861880;
	lag_w[6]=0.9801714;
	lag_w[7]=0.9731081;
	lag_w[8]=0.9650213;
	lag_w[9]=0.9559375;
	lag_w[10]=0.9458861;

	r = (float *)malloc( sizeof(float) * (p+1) );
	auto_corr( n, p, w, x, r );

	for(i = 0; i <= p; i++)
	{
	    r[i] *= lag_w[i];
	}

	u = r[0];
	a0[0] = a[0] = 1.0;

	for(i=1; i<=p; i++){

		a[i] = r[i]; 
		for(j=1; j<i; j++)
			a[i] += a0[j]*r[i-j];
		a[i] = -a[i]/(u+EPS);

		for(k=1; k<i; k++)
			a[k] = a0[k] + a[i]*a0[i-k];		

		u = (1.0-a[i]*a[i])*u;

		for(j=0; j<=i; j++)
			a0[j] = a[j];

		if(( u/(r[0]+EPS) < 0.000001) && (i+1<=p ))
		{
		    fprintf(stderr, "stop lpc %d\n", i + 1);
		    for(l=i+1;l<=p;l++)
			a[l]=0.;
		    break;
		}
	}	


	free((char *) r);
	return( u );
}


	
static void get_residual(
int	n,
int	np,
float	a[],
float	x[],
float	r[])
{
	int	i, p;

	for(i=0; i<n; i++){
		r[i] = x[i];
		for(p=1; p<=np; p++){
			if (i-p>=0)
				r[i] += a[p] * x[i-p];
		}
	}
}

static void zero_supress(
int	n,
float	x[])
{
	int	i;

	for(i=0; i<n; i++)
		x[i] = 0.0;
}

static float get_frame_power(
float  s[])
{
    int i;
    float  framePower;

    framePower = 0.0;
    for(i=0; i<RSD_SIZE; i++)
	framePower += s[i]*s[i];

    framePower = framePower * 8.0 / (16384.0*16384.0);

    return( framePower );
}

static int Ambiguous(
float	p0,
float	p1,
float	d)
{
    int	cnd;
    cnd = fabs(p0 - 2.0 * p1) / (p0+EPS) < d || fabs(p0 - p1 / 2.0) / (p0+EPS) < d ||
	fabs(p0 - 3.0 * p1) / (p0+EPS) < d || fabs(p0 - p1 / 3.0) / (p0+EPS) < d;

    return(cnd);
}

static int NearPitch(
float	p0,
float	p1,
float	ratio)
{
    return((p0 * (1.0 - ratio) < p1) && (p1 < p0 * (1.0 + ratio)));
}



static int CheckAmbiguousPitch(
float	lev,
float	*peak,
int	peak_pos[PEAKMAX])
{
    float	ambgRatio;

    if(lev>1000.0)
    {
	ambgRatio = 1.2;
    }
    else
    {
	ambgRatio = 1.5;
    }
        
    if(peak[0] / (peak[1]+EPS) < ambgRatio &&
       (abs(peak_pos[0] - 2 * peak_pos[1]) < 5 ||
	abs(2 * peak_pos[0] - peak_pos[1]) < 5))
    {
	return(1);
    }
    else
    {
	return(0);
    }
}    


static float gpMax = 0.0;
static float gpMin = 148.0;

static int CheckGlobalPitch(
NgbPrm	*crntPrm,
NgbPrm	*prevPrm,
float	lev,
int	peakPos[PEAKMAX],
float	fb)
{
    static int	gpState = 0;
    static int	gpcd = 0;
    static int	fbState = 0;
    static int	gpHoldState = 0;
    int	gpSetFlag;
    
    gpSetFlag = 0;

    if(((peakPos[0] > global_pitch * 0.6) &&
       (peakPos[0] < global_pitch * 1.8) &&
       (crntPrm->r0r > 0.39 && lev > LARGELEVEL && peakPos[0] <= 100)) ||
       crntPrm->r0r > 0.65 ||
       ((crntPrm->r0r > 0.30 && abs(peakPos[0] - gpcd) < 8) && lev > 400.0))
      {
	if(gpState==1 &&
	   abs(peakPos[0] - gpcd) < 8 &&
	   ((!Ambiguous(global_pitch, (float) peakPos[0], 0.11) &&
	     !Ambiguous(prevPrm->pitch, (float) peakPos[0], 0.11)) ||
	    crntPrm->r0r > 0.65))
	{
	    gpState=1;
	    if(global_pitch != 0.0)
	    {
		prevGp = global_pitch;
	    }
	    global_pitch = (float)peakPos[0];
	    gpSetFlag = 1;
	    gpHoldState = 0;
	}
	else
	{
	    if(NearPitch(gpcd, (float) peakPos[0], 0.2) &&
	       !NearPitch(gpcd, global_pitch, 0.2))
	    {
		global_pitch = 0.0;
		gpHoldState = 0;
	    }

	    gpState=1;
	}

	gpcd=peakPos[0];

	if(!NearPitch(gpcd, global_pitch, 0.2) &&
	   !Ambiguous(gpcd, global_pitch, 0.11))
	{
	    global_pitch = 0.0;
	    gpHoldState = 0;
	}


    }
    else
    {
	gpState=0;
    }
    
    if(gpHoldState == 5)
    {
	if(global_pitch != 0.0)
	{
	    prevGp = global_pitch;
	}
	global_pitch = 0.0;
	gpHoldState = 0;
    }
    else
    {
	gpHoldState++;
    }
    
    if(fb == 0.0)
    {
	if(fbState == FB_STATE_HOLD)
	{
	    if(global_pitch != 0.0)
	    {
		prevGp = global_pitch;
	    }
	    global_pitch = 0.0;
	}
	else
	{
	    fbState++;
	}
    }
    else
    {
	fbState=0;
    }

    if(gpMax < global_pitch)
    {
	gpMax = global_pitch;
    }
    if(global_pitch != 0 && gpMin > global_pitch)
    {
	gpMin = global_pitch;
    }

    return(gpSetFlag);
}


static void ScanPeakPos(
int	*currP,
float	refp,
float	from,
float	to,
int	slim,
int	peakPos[PEAKMAX],
float	*peak,
float	*s,
NgbPrm	*ngbPrm)
{
    int		i, loopFlag;

    loopFlag = 1;
    i = 0;
    while(loopFlag && i < slim)
    {
	if(refp * from < (float) peakPos[i] &&
	   (float) peakPos[i] < refp * to)
	{
	    loopFlag = 0;
	    *currP = peakPos[i];
	    ngbPrm->pitch = (float) *currP;
	    ngbPrm->prob = peak[0] / (peak[i]+EPS);
	    ngbPrm->r0r = peak[0] / (s[0]+EPS);
	}
	i++;
    }
    return;
}

static float getpitch(
NgbPrm	*crntPrm,
NgbPrm	*prevPrm,
int	*scanlimit,
float	*framepower,
float	*s,
int	*peakPos,
float	*peak0,
float	lev)
{
    int		i, j, loopFlag;
    float	peak[PEAKMAX];
    float	r0;
    int		currP;
    
    int		trkp;
    
    int		reason;

    static int	ambgs_p;
    static int	new_ambgs_p=0;
    r0 = s[0];
    
    for (i=0; i<PEAKMAX; i++) {
      peak[i]=0.0f;
    }

    for(i=Lmin+1,j=0; i<Lmax-1; i++)
    {
	if(s[i]-s[i-1]>0 && s[i]-s[i+1]>0 && s[i]>0.0)
	{
	    peak[j] = s[i];
	    peakPos[j] = i;
	    j++;
	}
    }
    
    *scanlimit = j;
    if(j != 0)
    {
	sort2( peak, 0, j-1, peakPos );
    }
    else
    {
	peakPos[0] = 0;
    }
    
    if(r0 == 0.0)
    {
	currP = (int) global_pitch;
	crntPrm->r0r = 0.0;
    }
    else
    {
	currP = peakPos[0];
	crntPrm->r0r = peak[0] / r0;
    }
    
    crntPrm->rawR0r = crntPrm->r0r;

    crntPrm->pitch = currP;
    crntPrm->prob = peak[0] / (peak[1]+EPS);

    *peak0 = peak[0];

    ambgs_p = new_ambgs_p;
    new_ambgs_p = 0;
    
    new_ambgs_p = CheckAmbiguousPitch(lev, peak, peakPos);
    
    if(((peakPos[0] > global_pitch * 0.4) &&
       !new_ambgs_p &&
       (crntPrm->r0r > 0.39 && lev > LARGELEVEL && peakPos[0] <= 100)) ||
       crntPrm->r0r > 0.65)
    {
	currP = peakPos[0];
	crntPrm->pitch = (float) currP;
	crntPrm->prob = peak[0] / (peak[1]+EPS);

	crntPrm->r0r = s[currP]/(r0+EPS);
    }
    else
    {
	if(prevPrm->pitch == 0.0)
	{
	    if(crntPrm->r0r > 0.25)
	    {
		currP = peakPos[0];
		crntPrm->pitch = (float) currP;
		crntPrm->prob = peak[0] / (peak[1]+EPS);
		crntPrm->r0r = s[currP]/(r0+EPS);
	    }
	    else
	    {
		if(global_pitch != 0.0)
		{
		    ScanPeakPos(&currP, global_pitch, 0.9, 1.1, j, peakPos,
				peak, s, crntPrm);
		}
	    }
	}
	else
	{
	    if(
	       crntPrm->r0r > 0.30 &&
	       lev > 1000.0 &&
	       !Ambiguous((float) peakPos[0], global_pitch, 0.11) &&
	       !new_ambgs_p)
	    {
		currP = peakPos[0];
		crntPrm->pitch = (float) currP;
		crntPrm->prob = peak[0] / (peak[1]+EPS);

	    }
	    else if (crntPrm->r0r > 0.3 && lev>1000.0 )
	    { 
		currP = peakPos[0];
		crntPrm->pitch = (float) currP;
		crntPrm->prob = peak[0] / (peak[1]+EPS);

		if(global_pitch != 0.0)
		{
		    if(Ambiguous(prevPrm->pitch, global_pitch, 0.11))
		    {
			trkp = (int) global_pitch;
		    }
		    else
		    {
			trkp = (int) prevPrm->pitch;
		    }
		}
		else if(prevGp != 0.0)
		{
		    trkp = (int) prevGp;
		}
		else
		{
		    trkp = (int) prevPrm->pitch;
		}
		
		loopFlag = 1;
		i=1;
		while(loopFlag && (i < *scanlimit) &&
		      (s[peakPos[i]]/(r0+EPS) > 0.25))
		{
		    if(abs(peakPos[i]-trkp) < abs(currP-trkp))
		    {
			loopFlag = 0;

			currP = peakPos[i];
			crntPrm->pitch = (float) currP;
			crntPrm->prob = peak[i] / (peak[0]+EPS);
			crntPrm->r0r = peak[0] / (r0+EPS);
		    }
		    i++;
		}
	    }
	    else if (crntPrm->r0r<0.39 && lev>4000.0)
	    {
		currP = peakPos[0];
		crntPrm->pitch = (float) currP;
		crntPrm->prob = peak[0] / (peak[1]+EPS);

		crntPrm->r0r = s[currP]/(r0+EPS);
	    }
	    else if (crntPrm->r0r > 0.1)
	    {
		reason = 0;
		if(new_ambgs_p)
		{
		    if(global_pitch != 0.0)
		    {

			if(Ambiguous(prevPrm->pitch, global_pitch, 0.11) ||
			   Ambiguous(prevPrm->pitch, (float) peakPos[0],
				     0.11))
			{
			    if(abs(peakPos[0] - (int) global_pitch) <
			       abs(peakPos[1] - (int) global_pitch))
			    {
				reason = 1;
			    }
			    else
			    {
				reason = 2;
			    }
			}
			else
			{
			    if(abs(peakPos[0] - (int) prevPrm->pitch) <
			       abs(peakPos[1] - (int) prevPrm->pitch))
			    {
				reason = 3;
			    }
			    else
			    {
				reason = 4;
			    }
			}
		    }
		    else
		    {
			if(peak[0] / (peak[1]+EPS) < 1.1)
			{
			    if(abs(peakPos[0] - (int) prevPrm->pitch) <
			       abs(peakPos[1] - (int) prevPrm->pitch))
			    {
				reason = 7;
			    }
			    else
			    {
				reason = 8;
			    }
			}
		    }
		}
		else
		{
		    reason = 9;
		}


		switch(reason)
		{
		case 1:
		case 3:
		case 5:
		case 7:
		case 9:
		    currP = peakPos[0];
		    crntPrm->prob = peak[0] / (peak[1]+EPS);
		    break;
		case 2:
		case 4:
		case 6:
		case 8:
		    currP = peakPos[1];
		    crntPrm->prob = peak[1] / (peak[0]+EPS);
		    break;
		}		    
		crntPrm->pitch = (float) currP;
		crntPrm->r0r = peak[0]/(r0+EPS);
	    }
	    else if (prevPrm->r0r > 0.3)
	    {
		currP = 0;
		for(i=0; i<j; i++)
		{
		    if ((float) peakPos[i] / (prevPrm->pitch+EPS) > 0.75 &&
			(float) peakPos[i] / (prevPrm->pitch+EPS) < 1.3)
		    {
			currP = peakPos[i];
			crntPrm->pitch = (float) currP;
			if(i != 0)
			{
			    crntPrm->prob = peak[i] / (peak[0]+EPS);
			}
			else
			{
			    crntPrm->prob = peak[0] / (peak[1]+EPS);
			}					
			break;
		    }
		}

		if (currP == 0)
		{
		    crntPrm->r0r = 0.0;
		}
		else
		{
		    if (s[currP] / (r0+EPS) < 0.3)
		    {
			if(global_pitch != 0.0)
			{
			    for(i=0; i<j; i++)
			    {
				if((float) peakPos[i] / (global_pitch+EPS)>0.8 &&
				   (float) peakPos[i] / (global_pitch+EPS)<1.2)
				{
				    currP = peakPos[i];	
				    crntPrm->pitch = (float) currP;
				    if(i != 0)
				    {
					crntPrm->prob =
					    peak[i] / (peak[0]+EPS);
				    }
				    else
				    {
					crntPrm->prob =
					    peak[0] / (peak[1]+EPS);
				    }					
				    break;
				}
			    }
			}
		    }	
		}
	    }
	    else
	    {
		if (global_pitch != 0.0)
		{
		    ScanPeakPos(&currP, global_pitch, 0.9, 1.1, j, peakPos,
				peak, s, crntPrm);
		}
	    }
	}
    }
    
    if (r0 < 0.00001)
	crntPrm->r0r = 0.0;
    
    return( currP );
}











static void lpf19(
int	n,
float	*in,
float	*out,
int	cwFlag)
{
    int		i, j;
    float	coef[FILTER_ORDER + 1];
    
    switch(cwFlag)
    {
    case LPF_PI3per4:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = lpfCoefPI3per4[i];
	}
	break;
    case LPF_PIper2:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = lpfCoefPIper2[i];
	}
	break;
    case LPF_PI3per8:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = lpfCoefPI3per8[i];
	}
	break;
    case LPF_PIper4:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = lpfCoefPIper4[i];
	}
	break;
    case LPF_PIper8:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = lpfCoefPIper8[i];
	}
	break;
    }

    for(i=0; i<n; i++)
    {
	out[i] = 0.0;
	for(j = - FILTER_ORDER / 2; j <= FILTER_ORDER / 2; j++)
	{
	    if(i + j >= 0 && i + j < n)
		out[i] += in[i + j] * coef[FILTER_ORDER / 2 - j];  
	}
    }
}

static void hpf19(
int	n,
float	*in,
float	*out,
int	cwFlag)
{
    int		i, j;
    float	coef[FILTER_ORDER + 1];
    
    switch(cwFlag)
    {
    case HPF_PI3per4:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = hpfCoefPI3per4[i];
	}
	break;
    case HPF_PI4per5:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = hpfCoefPI4per5[i];
	}
	break;
    case HPF_PI7per8:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = hpfCoefPI7per8[i];
	}
	break;
    case HPF_PI1per8:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = hpfCoefPI1per8[i];
	}
	break;
    case HPF_PI1per4:
	for(i = 0; i < FILTER_ORDER + 1; i++)
	{
	    coef[i] = hpfCoefPI1per4[i];
	}
	break;
    }

    for(i=0; i<n; i++)
    {
	out[i] = 0.0;
	for(j = - FILTER_ORDER / 2; j <= FILTER_ORDER / 2; j++)
	{
	    if(i + j >= 0 && i + j < n)
		out[i] += in[i + j] * coef[FILTER_ORDER / 2 - j];  
	}
    }
}



static void zero_padding(
int	n1,
int	n2,
float	*s)
{
    int	i;

    for(i=n1; i<n2; i++) s[i] = 0.0;
}

static void z_init(
int	n,
float	*d)
{
    int	i;
    for(i=0; i<n; i++) d[i] = 0.0;
}

static void power(
int	n,
float	*re,
float	*im,
float	*p)
{
    int	i;
    for(i=0; i<n; i++)
    {
	p[i] =  re[i] * re[i] + im[i] * im[i];
    }
}

static void shift_filter_state(
int	n,
float	s[])
{
    int	i;

    for(i=0; i<n-1; i++)
	s[i] = s[i+1];
}

static void shift_ngbPrm(
int	n,
NgbPrm *ngbPrm)
{
    int	i;

    for(i=0; i<n-1; i++)
    {
	ngbPrm[i].pitch = ngbPrm[i + 1].pitch;
	ngbPrm[i].prob = ngbPrm[i + 1].prob;
	ngbPrm[i].r0r = ngbPrm[i + 1].r0r;
	ngbPrm[i].rawR0r = ngbPrm[i + 1].rawR0r;
	ngbPrm[i].rawPitch = ngbPrm[i + 1].rawPitch;
    }
}

static int p_scan(
float	refp,
float	from,
float	to,
int	stop,
int	position_data[],
float	*prevAc)
{
    int	i;
    int	scanp;

    for(i=0,scanp=0; i<stop; i++)
    {
	if ((float) position_data[i] > refp * from &&
	    (float) position_data[i] < refp * to)
	{
	    scanp = position_data[i];
	    break;
	}
    }

    if(prevAc[0]==0.0)
    {
	scanp = 0;
    }
    else if (scanp==0 || prevAc[scanp] / (prevAc[0]+EPS)<0.1)
    {
	scanp = (int)refp;
    }

    return( scanp );
}



static void shift_framePower(
int	n,
float	framePower[])
{
    int	i;
    
    for(i = 0; i < n - 1; i++)
    {
	framePower[i] = framePower[i + 1];
    }
}

static void shift_ac(
float	s[],
float	d[])
{
    int	i;
    for(i=0; i<AL; i++)
	d[i] = s[i];
}


static int levelchk(
int	n,
float	*x)
{
    float	lvl;
    int		i;
    
    for(i=0,lvl=0.0; i<n; i++) lvl += x[i]*x[i];
    
    if(lvl < 4.0 * 4.0 * n)
	return(0);
    else
	return(1);
}


static void GetPitchNFD(
float		*s,
float		*w,
float		lev,
int		filterType,
NgbPrm		*crntPrm,
NgbPrm		*prevPrm,
int		*scanlimit,
PitchPrm	*pchPrm)
{
    float	fdata[AL];
    float	a[ Np+1 ];
    float	residual[AL];
    float	data_r_lpf[AL];
    float	data_i[AL];
    float	eng0, eng1;

    int		fftpow = POWER;
    int		i;

    for(i = 0; i < RSD_SIZE; i++)
    {
	fdata[i] = s[i];
    }

    if(levelchk(RSD_SIZE, fdata))
    {
	lpc2(RSD_SIZE, w, fdata, Np, a);

	get_residual( RSD_SIZE, Np, a, fdata, residual );
	zero_supress( Np, residual );
	
	if(filterType == THROUGH)
	{
	    for(i = 0; i < 256; i++)
	    {
		data_r_lpf[i] = residual[i];
	    }
	}
	else if(filterType >= LPF_OFFSET)
	{
	    hpf19(RSD_SIZE, residual, data_r_lpf, filterType);
	}
	else
	{
	    lpf19(RSD_SIZE, residual, data_r_lpf, filterType);
	}
    }
    else
    {
	z_init(RSD_SIZE, data_r_lpf);
    }
    
    zero_padding(RSD_SIZE, AL, data_r_lpf);

    z_init(AL, data_i);
    
    IPC_fft( data_r_lpf, data_i, fftpow );
    power( AL, data_r_lpf, data_i, pchPrm->ac);
    z_init( AL, data_i );
    IPC_ifft(pchPrm->ac, data_i, fftpow );

    pchPrm->framePower = get_frame_power(data_r_lpf);

    pchPrm->pitch =  (float) getpitch(crntPrm, prevPrm, scanlimit,
				      &(pchPrm->framePower), pchPrm->ac,
				      pchPrm->peakPos, &pchPrm->peak0, lev);

    pchPrm->scanLmt = *scanlimit;

    pchPrm->prob = crntPrm->prob;
    pchPrm->r0r = crntPrm->r0r;

    eng0 = 0.0;
    for(i = 0; i < RSD_SIZE / 2; i++)
    {
	eng0 += (float) s[i] * s[i];
    }

    eng1 = 0.0;
    for(i = RSD_SIZE / 2; i < RSD_SIZE; i++)
    {
	eng1 += (float) s[i] * s[i];
    }

    pchPrm->engRatio = eng0 / (eng1+EPS);

}



static void DecidePitchPrm(
PitchPrm	*pchPrm0,
PitchPrm	*pchPrm1,
int		*scanlimit,
NgbPrm		*crntPrm,
float		*framePower,
float		*peak0,
float		*ac,
int		peakPos[PEAKMAX])
{
    int		i;
    int		reason;

    if((pchPrm0->pitch * 0.96 < pchPrm1->pitch) &&
       (pchPrm1->pitch < pchPrm0->pitch * 1.04))
    {
	reason = 0;
    }	
    else if(pchPrm0->scanLmt > 40)
    {
	reason = 1;
    }	
    else if(pchPrm1->r0r / (pchPrm0->r0r+EPS) > 1.2)
    {
	reason = 2;
    }
    else if(global_pitch != 0.0)
    {
	if(fabs(pchPrm1->pitch - global_pitch) >
	   fabs(pchPrm0->pitch - global_pitch))
	{
	    reason = 3;
	}
	else
	{
	    reason = 4;
	}
    }
    else if(pchPrm0->prob / (pchPrm1->prob+EPS) > 1.2)
    {
	reason = 5;
    }
    else
    {
	reason = 6;
    }	



    switch(reason)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
	*scanlimit = pchPrm1->scanLmt;
	crntPrm->pitch = pchPrm1->pitch;
	crntPrm->prob = pchPrm1->prob;
	framePower[1] = pchPrm1->framePower;
	crntPrm->r0r = pchPrm1->r0r;
	peak0[1] = pchPrm1->peak0;

	for(i = 0; i < AL; i++)
	{
	    ac[i] = pchPrm1->ac[i];
	}
	for(i = 0; i < PEAKMAX; i++)
	{
	    peakPos[i] = pchPrm1->peakPos[i];
	}


	break;
    case 4:
	*scanlimit = pchPrm0->scanLmt;
	crntPrm->pitch = pchPrm0->pitch;
	crntPrm->prob = pchPrm0->prob;
	framePower[1] = pchPrm0->framePower;
	crntPrm->r0r = pchPrm0->r0r;
	peak0[1] = pchPrm0->peak0;

	for(i = 0; i < AL; i++)
	{
	    ac[i] = pchPrm0->ac[i];
	}
	for(i = 0; i < PEAKMAX; i++)
	{
	    peakPos[i] = pchPrm0->peakPos[i];
	}

	break;
    }
}


#define PS_FROM		0.81
#define PS_TO		1.2

static float GetStdPch2Elms(
NgbPrm	*ngbPrmCrnt,
NgbPrm	*ngbPrmRef,
int	*scanlimit,
int	peakPos[PEAKMAX],
float	*prevAc)
{
    float	stdPch;

    if((ngbPrmCrnt->r0r >= ngbPrmRef->r0r)
       || ngbPrmCrnt->r0r > 0.7)
    {
	stdPch = ngbPrmCrnt->pitch;
    }
    else
    {
	stdPch = (float) p_scan(ngbPrmRef->pitch, PS_FROM, PS_TO, *scanlimit,
				&peakPos[0], prevAc);
    }

    return(stdPch);
}    

static float TrackingPitch(
NgbPrm	*crntPrm,
NgbPrm	*prevPrm,
int	*scanlimit,
float	*ac,
int	peakPos[PEAKMAX],
int	prevVUV)
{
    float	kimete = 0.0;

    float	pitch;

    static float	prevRawp= 0.0;

    int		st0, st1;

    static float	rblPch = 0.0;

    static float	prevRblPch = 0.0;

    rblPch = global_pitch;

    
    if(prevVUV != 0 && rblPch != 0.0)
    {
	st0 = Ambiguous(prevPrm->pitch, rblPch, 0.11);
	st1 = Ambiguous(crntPrm->pitch, rblPch, 0.11);

	if(!(st0 || st1))
	{
	    if(NearPitch(crntPrm->pitch, prevPrm->pitch, 0.2))
	    {
		pitch = crntPrm->pitch;
	    }
	    else if(NearPitch(crntPrm->pitch, rblPch, 0.2))
	    {
		pitch = crntPrm->pitch;
	    }
	    else if(NearPitch(prevPrm->pitch, rblPch, 0.2))
	    {
		if(crntPrm->r0r > prevPrm->r0r &&
		   crntPrm->prob > prevPrm->prob)
		{
		    pitch = crntPrm->pitch;
		}
		else
		{
		    pitch = prevPrm->pitch;
		}
	    }
	    else
	    {
		pitch = GetStdPch2Elms(crntPrm, prevPrm, scanlimit,
				       peakPos, ac);
	    }
	}
	else if(!st0)
	{
	    if(NearPitch(prevPrm->pitch, crntPrm->pitch, 0.2))
	    {
		pitch = crntPrm->pitch;
	    }
	    else if((gpMax * 1.2 > crntPrm->pitch) &&
		    NearPitch(prevPrm->rawPitch, crntPrm->pitch, 0.2))
	    {
		pitch = crntPrm->pitch;
	    }
	    else
	    {
		pitch = prevPrm->pitch;
	    }		
	}
	else if(!st1)
	{
	    if((crntPrm->rawPitch != crntPrm->pitch) &&
		NearPitch(crntPrm->rawPitch, prevPrm->rawPitch, 0.2))
	    {
		pitch = crntPrm->rawPitch;
	    }
	    else
	    {
		pitch = crntPrm->pitch;
	    }
	    
	}
	else
	{
	    if(NearPitch(prevPrm->pitch, crntPrm->pitch, 0.2))
	    {
		pitch = crntPrm->pitch;
	    }
	    else
	    {
		pitch = rblPch;
	    }
	}
    }
    else if(prevVUV == 0 && rblPch != 0.0)
    {
	st1 = Ambiguous(crntPrm->pitch, rblPch, 0.11);

	if(!st1)
	{
	    pitch = crntPrm->pitch;
	}
	else
	{
	    pitch = rblPch;
	}
    }
    else if(prevVUV != 0 && rblPch == 0.0)
    {
	st1 = Ambiguous(crntPrm->pitch, prevPrm->pitch, 0.11);

	if(!st1)
	{
	    pitch = GetStdPch2Elms(crntPrm, prevPrm, scanlimit, peakPos,
				   ac);
	}
	else
	{
	    if(prevPrm->r0r < crntPrm->r0r)
	    {
		pitch = crntPrm->pitch;
	    }
	    else
	    {
		pitch = prevPrm->pitch;
	    }
	}	    
    }
    else
    {
	pitch = crntPrm->pitch;
    }
    
    
    crntPrm->pitch = pitch;

    prevRblPch = rblPch;

    prevRawp = pitch;

    return(pitch);
}




/* check of music and speech          */
/* return value                       */
/*   0 .... unknown                   */
/*   1 .... music                     */
/*   2 .... speech                    */

#define DEC_PRM 250		/* parameters for decision */
#define DC_CUT

static int check_music_speech( float *x, float r0r )
{
    int i, j, k, intflag;
    static int cnt = 0, mvflag = 0;
    static float avdpow, avr0r, sqrpow, sqrr0r;
    float  lvl, avpow, avpow1, dfpow, mavr0r, dfr0r, rms, num;
    static int mutecount=0, sndcount=0;
    static float spow[4], dpow[DEC_PRM];
    static float R0r[DEC_PRM], Avpow[DEC_PRM];
#ifdef DC_CUT
    static float z1=0;
    float y[160];

    for(i=0; i<160; i++) {
	y[i] = x[i+96]-x[i+95]+0.99*z1;
	z1 = y[i];
    }
    for(i=0,lvl=0.0; i<160; i++) lvl += y[i]*y[i];
#else
    for(i=96,lvl=0.0; i<RSD_SIZE; i++) lvl += x[i]*x[i];
#endif

    for (i = 3; i >= 1; i--) spow[i] = spow[i-1];
    spow[0] = lvl/160;

    for(i=0,avpow1=0.0; i<4; i++) avpow1 += spow[i];
    avpow1 /= 4.0;

    avr0r += (r0r-R0r[cnt%DEC_PRM]);
    sqrr0r += (r0r * r0r - R0r[cnt%DEC_PRM] * R0r[cnt%DEC_PRM]);
    R0r[cnt%DEC_PRM] = r0r;
    
    avpow = fabs( spow[0] - avpow1 ) / avpow1;
    avdpow += (avpow - dpow[cnt%DEC_PRM]);
    sqrpow += (avpow * avpow - dpow[cnt%DEC_PRM] * dpow[cnt%DEC_PRM]);
    dpow[cnt%DEC_PRM] = avpow;

    if (avpow1 >= 400) sndcount++;

    if (cnt >= DEC_PRM) {
	num = (float)DEC_PRM;
	avpow = avdpow / num;
	dfpow = sqrpow - num * avpow * avpow;
        dfpow = sqrt( dfpow/num );
	
	mavr0r = avr0r / num;
	dfr0r = sqrr0r - num * mavr0r * mavr0r;
        dfr0r = sqrt( dfr0r/num );

	if (dfr0r <= (0.07 * mavr0r + 0.137))
	    intflag = 1;
        else if (dfr0r >= (0.153 * mavr0r + 0.113))
	    intflag = 2;
        else if (dfpow < (-0.5 * avpow + 0.8))
	    intflag = 1;
        else
	    intflag = 2;

	if (HVXCclassifierMode == 0) {
	    if (Avpow[cnt%DEC_PRM] < 20000) {
	        mutecount = (mutecount == 10) ? 10 : mutecount+1;
            } else {
	        mutecount = 0;
            }
	    if (Avpow[cnt%DEC_PRM] > 400) sndcount--;
        
	    if ((mutecount >= 9 || mvflag == 0) && sndcount >= 0.6*DEC_PRM)
                mvflag = intflag;
	} else {
	    if (Avpow[cnt%DEC_PRM] > 400) sndcount--;
	
	    if (sndcount >= 0.6*DEC_PRM) mvflag = intflag;
	}
    }
    
    Avpow[cnt%DEC_PRM] = avpow1;
    cnt = (cnt == 2*DEC_PRM) ? DEC_PRM : (cnt+1);
    
    return (mvflag);
}





static float GetPitch(
float	*s,
float	*r0r,
float	fb,
float	lev,
float	*w,
int	prevVUV,
float	*r0h)
{
    int		i;
    
    float	ac[ AL ];
    int	scanlimit = Lmax-1;
    int	peakPos[PEAKMAX];

    float	pitchOL;

    PitchPrm	pchPrm0, pchPrm1;

    static NgbPrm	ngbPrm[2] = {{0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}};
    static float	peak0[2];
    static float	framePower[2] = {.0, .0};

    shift_ngbPrm(2, ngbPrm);
    shift_framePower(2, framePower);
    shift_filter_state(2, peak0);

    GetPitchNFD(s, w, lev, HPF_PI1per4, &ngbPrm[1], &ngbPrm[0], &scanlimit,
		&pchPrm0);

    *r0h = ngbPrm[1].rawR0r;

    GetPitchNFD(s, w, lev, LPF_PIper4, &ngbPrm[1], &ngbPrm[0], &scanlimit,
		&pchPrm1);
    judge = check_music_speech( s, ngbPrm[1].rawR0r );

    DecidePitchPrm(&pchPrm0, &pchPrm1, &scanlimit, &ngbPrm[1], framePower,
		   peak0, ac, peakPos);

    ngbPrm[1].rawPitch = peakPos[0];
    
    CheckGlobalPitch(&ngbPrm[1], &ngbPrm[0], lev, peakPos, fb);

    pitchOL = TrackingPitch(&ngbPrm[1], &ngbPrm[0], &scanlimit, ac, peakPos, prevVUV);


    for(i = 0; i < 2; i++)
    {
	r0r[i] = ngbPrm[i].r0r;
    }

    return(pitchOL); 
}


#define BLNUM	32

static void freq_balance(
float rms[],
float arys[],
float *fb,
float *lev)
{
    int i,j;
    float lbl,hbl,level,th;
    float ene[BLNUM],ave,avem,std;
    float pw,peak,pf,av,rmsf;
    short rmss;
    int grn;
    static int stdm=0;
    static int  co=0;
    static float pl=0.;
    float devi,l1devi;

    float	devio, stdo;

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
    for(i=0,ave=0.0;i<BLNUM;i++)
	ave += ene[i];
    ave=ave/BLNUM;

    avem=1.;
    for(i=0;i<BLNUM;i++){
	if(ene[i] != 0)
	    avem = avem * pow(ene[i], 1./(float)BLNUM);
	else
	    avem = 0.;
    }

    for(i=0,devi=l1devi=0.0;i<BLNUM;i++){
	devi += (ene[i]-ave)*(ene[i]-ave);
	l1devi += fabs(ene[i]-ave);
    }
    devi=sqrt(devi/(float)BLNUM);
    l1devi=(l1devi/(float)BLNUM);
    if(ave>0.001){
	devi=devi/(ave+EPS);
	l1devi=l1devi/(ave+EPS);
    }
    else{
	devi=0.1;
	l1devi=0.1;
    }
    devio=devi;

    if(avem > 0.01)
	std = ave/(avem+EPS);
    else
	std = 1.;
    stdo=std;

    for(i=0,av=0.0;i<SAMPLE;i++)
	av += arys[i];
    av=av/SAMPLE;

    for(i=0,pw=peak=0.0;i<SAMPLE;i++){
	pw += (arys[i]-av)*(arys[i]-av);
	if(fabs(arys[i]-av)>peak) 
	    peak=fabs(arys[i]-av);
    }
    rmsf=sqrt(pw/(float)SAMPLE);
    pf=peak/(rmsf+EPS);

    rmss=(short)rmsf & 0xffff;
    if(rmss <= 7.5)
	grn=1;
    else
	grn=0;

    if(hbl != 0. )
	*fb= lbl/(hbl+EPS) ;
    else
	*fb= 20.;
    if(hbl ==0. && lbl==0.) *fb=0.;

    *lev = level = sqrt((lbl+hbl)/(SAMPLE/2));

    if(level > pl)
	pl = level;

    if(pl >0. && level>0.)
	th=20. * log10(level/(pl+EPS));
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
	*fb=0;


    else if ((*fb < 5.) && (level < 550.) && (stdm >=1)) {
	*fb=0.;
    }
    else if  ( (level < 550. && devi < 0.45) || (level < 600. && devi < 0.4)) 
	*fb=2.;
    else
	*fb=1.;

    co++;
}

static void modify_fb(
float *fb,
float *lev)
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

static void shift_peak_pos(
int	peak_pos[][PEAKMAX])
{
    int	i;

    for(i=0; i<PEAKMAX; i++){
	peak_pos[0][i] = peak_pos[1][i];
    }
}

static void shift_scanlimit(
int	scl[])
{
    scl[0] = scl[1];
}


static float GetStdPch3Elms(
NgbPrm	*ngbPrm,
int	*scanlimit,
int	(*peak_pos)[PEAKMAX],
float	*prevAc)
{
    float	stdPch;

    if((ngbPrm[1].r0r >= ngbPrm[2].r0r && ngbPrm[1].r0r >= ngbPrm[0].r0r)
       || ngbPrm[1].r0r > 0.7)
    {
	stdPch = ngbPrm[1].pitch;
    }
    else
    {
	if(ngbPrm[0].r0r < ngbPrm[2].r0r)
	{
	    stdPch = (float) p_scan(ngbPrm[2].pitch, PS_FROM, PS_TO,
				    scanlimit[0], &peak_pos[0][0], prevAc);
	}
	else
	{
	    stdPch = (float) p_scan(ngbPrm[0].pitch, PS_FROM, PS_TO,
				    scanlimit[0], &peak_pos[0][0], prevAc);
	}
    }
    return(stdPch);
}    

static float TrackingPitch1FA(
NgbPrm	*ngbPrm,
int	*scanlimit,
float	*ac,
int	(*peak_pos)[PEAKMAX],
float	*prevAc,
int	prevVUV)
{
    float	kimete = 0.0;
    float	pitch;

    static float	prevRawp= 0.0;

    int		st0, st1, st2;

    static float	rblPch = 0.0;

    static float	prevRblPch = 0.0;

    if(prevVUV != 0 && rblPch != 0.0)
    {
	st0 = Ambiguous(ngbPrm[0].pitch, rblPch, 0.11);
	st1 = Ambiguous(ngbPrm[1].pitch, rblPch, 0.11);
	st2 = Ambiguous(ngbPrm[2].pitch, rblPch, 0.11);

	if(!(st0 || st1 || st2))
	{
	    if(Ambiguous(ngbPrm[2].pitch, ngbPrm[1].pitch, 0.11))
	    {
		pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[0], scanlimit,
				       peak_pos[0], prevAc);
	    }		
	    else if(Ambiguous(ngbPrm[0].pitch, ngbPrm[1].pitch, 0.11))
	    {
		pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[2], scanlimit,
				       peak_pos[0], prevAc);
	    }
	    else
	    {
		pitch = GetStdPch3Elms(ngbPrm, scanlimit, peak_pos, prevAc);
	    }
	}
	else if(!(st0 || st1))
	{
	    pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[0], scanlimit,
				   peak_pos[0], prevAc);
	}
	else if(!(st1 || st2))
	{
	    pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[2], scanlimit,
				   peak_pos[0], prevAc);
	}
	else if(!(st2 || st0))
	{
	    pitch = ngbPrm[0].pitch;
	}
	else if(!st0)
	{
	    pitch = ngbPrm[0].pitch;
	}
	else if(!st1)
	{
	    pitch = ngbPrm[1].pitch;
	}
	else if(!st2)
	{
	    pitch = ngbPrm[2].pitch;
	}
	else
	{
	    pitch = rblPch;
	}
    }
    else if(prevVUV == 0 && rblPch != 0.0)
    {
	st1 = Ambiguous(ngbPrm[1].pitch, rblPch, 0.11);
	st2 = Ambiguous(ngbPrm[2].pitch, rblPch, 0.11);

	if(!(st1 || st2))
	{
	    pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[2], scanlimit,
				   peak_pos[0], prevAc);
	}
	else if(!st1)
	{
	    pitch = ngbPrm[1].pitch;
	}
	else if(!st2)
	{
	    pitch = ngbPrm[2].pitch;
	}
	else
	{
	    pitch = rblPch;
	}
    }
    else if(prevVUV != 0 && rblPch == 0.0)
    {
	st0 = Ambiguous(ngbPrm[0].pitch, ngbPrm[2].pitch, 0.11);
	
	if(!st0)
	{
	    pitch = GetStdPch3Elms(ngbPrm, scanlimit, peak_pos, prevAc);
	}
	else
	{
	    pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[0], scanlimit,
				   peak_pos[0], prevAc);
	}
    }
    else
    {
	pitch = GetStdPch2Elms(&ngbPrm[1], &ngbPrm[2], scanlimit, peak_pos[0],
			       prevAc);
    }
    
    
    ngbPrm[1].pitch = pitch;

    rblPch = global_pitch;

    prevRblPch = rblPch;

    prevRawp = pitch;

    return(pitch);
}



static float GetPitch1FA(
float	*s,
float	*r0r,
float	fb,
float	lev,
float	*w,
int	prevVUV,
float	*r0h)
{
    static float	ac[ AL ];
    static float	prev_ac[ AL ];
    
    float	pitchOL;

    int		i;
    
    PitchPrm	pchPrm0, pchPrm1;

    static int	scanlimit[2] = {Lmax-1, Lmax-1};
    static float framePower[3] = {.0, .0, .0};

    static NgbPrm ngbPrm[3] = {{0.0, 0.0, 0.0, 0.0, 0.0},
                                 {0.0, 0.0, 0.0, 0.0, 0.0},
                                 {0.0, 0.0, 0.0, 0.0, 0.0}};

    static int	peak_pos[2][PEAKMAX];
    static float	peak0[3] = {.0, .0, .0};


    shift_ac(ac, prev_ac);

    shift_ngbPrm(3, ngbPrm);
    shift_framePower(3, framePower);
    
    shift_filter_state(3, peak0 );
    shift_scanlimit( scanlimit );
    shift_peak_pos( peak_pos );

    GetPitchNFD(s, w, lev, HPF_PI1per4, &ngbPrm[2], &ngbPrm[1], scanlimit,
		&pchPrm0);

    *r0h = ngbPrm[2].rawR0r;

    GetPitchNFD(s, w, lev, LPF_PIper4, &ngbPrm[2], &ngbPrm[1], scanlimit,
		   &pchPrm1);


    DecidePitchPrm(&pchPrm0, &pchPrm1, scanlimit, ngbPrm, framePower, peak0,
		   ac, peak_pos[1]);

    CheckGlobalPitch(&ngbPrm[2], &ngbPrm[1], lev, peak_pos[1], fb);


    pitchOL = TrackingPitch1FA(ngbPrm, scanlimit, ac, peak_pos, prev_ac, prevVUV);


    for(i = 0; i < 3; i++)
    {
	r0r[i] = ngbPrm[i].r0r;
    }

    return(pitchOL); 
}




void pitch_estimation(
float	*arysRaw,
float	*r0r,
int	idVUV,
float	*pchOL,
float	*r0h)
{
    float	arys[SAMPLE];
    float	aryd[SAMPLE];
    float	ang2[SAMPLE], re2[SAMPLE], im2[SAMPLE], rms2[SAMPLE];

    int		i;

    static float	fb[2] = {0.0, 0.0};
    static float	lev[2] = {0.0, 0.0};
    static int		frm = 0;

    for(i = 0; i < SAMPLE; i++)
    {
	arys[i] = arysRaw[i];
    }

    IPC_hp_filterp(arys, frm);

    IPC_window(ipc_coef, arys, aryd, SAMPLE);
    IPC_fcall_fft(aryd, rms2, ang2, re2, im2);
    
    fb[0] = fb[1];
    lev[0] = lev[1];

    freq_balance(rms2, arys, &fb[1], &lev[1]);

    modify_fb(&fb[0],&lev[0]);

    if(ipc_encDelayMode == DM_LONG)
    {
	*pchOL = GetPitch1FA(arysRaw, r0r, fb[1], lev[1], ipc_coef, idVUV,
			     r0h);
    }
    else
    {
	*pchOL = GetPitch(arysRaw, r0r, fb[1], lev[1], ipc_coef, idVUV, r0h);
    }
    
    if(*pchOL == 0.0)
    {
	*pchOL = (float) (NUMPITCH / 2 + 20);
    }
    
    frm++;

    return;
}

