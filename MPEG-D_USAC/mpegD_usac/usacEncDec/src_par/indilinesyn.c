/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - individual lines: synthesis



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

Copyright (c) 1996.



Source file: indilinesyn.c


Required libraries:
(none)

Required modules:
indilinecom.o		indiline common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
01-sep-96   HP    first version based on indiline.c
10-sep-96   BE
11-sep-96   HP
26-sep-96   HP    new indiline module interfaces
17-jan-97   HP    fixed linePred>prevNumLine bug
14-apr-97   HP    new line start phase: cos(M_PI/2)
22-apr-97   HP    noisy stuff ...
23-apr-97   BE    noise synthesis ...
11-jun-97   HP    added IndiLineSynthFree()
23-jun-97   HP    improved env/harm/noise interface
27-jun-97   HP    fixed short fade
29-aug-97   HP    added random start phase
16-feb-98   HP    improved noiseFreq
06-apr-98   HP    ILSconfig
06-may-98   HP    linePhaseValid
24-feb-99   NM/HP LPC noise
08-oct-99   HP    fast line sythesiser
22-oct-99   HP    dito indilinesyn_fast3.c
16-nov-99   HP    dito indilinesyn_8.c
19-nov-99   HP    fixed fast syn solaris bug (32 bit long)
04-oct-00   HP    fixed fast syn VC++ bug (32 bit long)
26-jan-01   BE    float -> double
17-Jan-2002 HP    added lineSynMode 3, 4 (no sweep = overlap/add)
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "indilinecom.h"	/* indiline common module */
#include "indilinesyn.h"	/* indiline synthesiser module */
#include "uhd_fft.h"		/* fft/ifft/odft/iodft module */

#include "common_m4a.h"		/* for random() and RND_MAX */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */


/* ---------- declarations (global) ---------- */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/* #define SHORT_THR 5 */		/* env rate threshold for short fade */
#define SHORT_THR 4.99			/* BE20010126  changed for numerical accuracy */
					/*  tan(pi/2*(8-.5)/15)/.2 = 5 */
					/*  i.e. not <5 thus not short_x_fade */
#define SHORT_FAC (1./8.)		/* factor for short fade length */
#define NOISE_FAC (1./4.)		/* factor for noise fade length */

#define ANTIALIAS_FREQ 0.49		/* anti alias filter cutoff freq */


/* ---------- declarations (data structures) ---------- */

/* status variables and arrays for individual line synthesis (ILS) */

struct ILSstatusStruct		/* ILS status handle struct */
{
 /* general parameters */
  ILSconfig *cfg;
  int debugLevel;		/* debug level (0=off) */
  int maxFrameLen;		/* max num samples per frame */
  double fSample;		/* sampling frequency */
  int maxNumLine;		/* max num lines */
  int maxNumEnv;		/* max num envelopes */
  int maxNumNoisePara;		/* max num noise parameters */
  int maxNoiseLen;		/* >= maxFrameLen */
  int bsFormat;			/* bit stream format */

  /* parameters for synthesis */
  int frameLen;			/* num samples in current frame */
  int shortOff;			/* short fade offset (in, out, x-fade) */
  int noiseOff;			/* noise fade offset */

  /* pointers to fixed content arrays */
  double *longWin;		/* long fade-in window  [0..frameLen-1] */
  double *shortWin;		/* short fade-in window */
				/*  [0..frameLen-2*shortOff-1] */
  double *noiseWin;		/* noise fade-in window */
				/*  [0..frameLen-2*noiseOff-1] */
  double *fsTab;			/* FastSine Table */

  /* pointers to variable content arrays */
  double *tmpLinePhase;		/* phase or phase deviation */
				/*   [0..maxNumLine-1] */
  double **env;			/* current frame envelope  */
				/*   [0..numEnv-1][0..2*frameLen-1] */
  double **prevEnv;		/* prev frame envelope */
				/*   [0..numEnv-1][0..2*frameLen-1] */
  int *prevLineSyn;		/* prev frame line synthesised */
				/*   [0..numLine-1] */
  int *prevLongEnvOut;		/* long fadeout  [0..numEnv-1] */
  int *longEnvIn;		/* long fadein  [0..numEnv-1] */
  double *noiseIdctIn;		/* idct in [0..numNoisePara-1] */
  double *noiseIdctOut;		/* idct out [0..noiseLen-1] */
  double *noiseRe;		/* noise real [0..2*noiseLen-1] */
  double *noiseIm;		/* noise imag [0..2*noiseLen-1] */
  double *synthPrevEnv;		/* syn lines with prev env [0..frameLen-1] */
  double *synthEnv;		/* syn lines with cur env [0..frameLen-1] */

  /* variables and pointers to arrays for frame-to-frame memory */
  int prevNumLine;		/* num lines in prev frame */
  int prevNumEnv;		/* num env in prev frame */
  double **prevEnvPara;		/* envelope parameters  */
				/*   [0..numEnv-1][0..ENVPARANUM-1] */
  double *prevLineFreq;		/* line frequency  [0..numLine-1] */
  double *prevLineAmpl;		/* line amplitude  [0..numLine-1] */
  double *prevLinePhase;		/* line phase  [0..numLine-1] */
  int *prevLineEnv;		/* line envelope flag  [0..numLine-1] */
  int prevNoiseLen;		/* prev noise length */
  double *prevNoise;		/* prev noise [0..2*noiseLen-1] */
  int prevNoiseEnv;		/* prev noise envelope */

};


/* ---------- functions (internal) ---------- */


/* GenILSconfig() */
/* Generate default ILSconfig. */

static ILSconfig *GenILSconfig ()
{
  ILSconfig *cfg;

  if ((cfg = (ILSconfig*)malloc(sizeof(ILSconfig)))==NULL)
    IndiLineExit("GenILSconfig: memory allocation error");
  cfg->rndPhase = 1;
  cfg->lineSynMode = 1;
  return cfg;
}


/* NoiseIdct */
/* IDCT of arbitrary length (sizex), uses only the sizey */
/* first IDCT input values. Note: input values are MODIFIED!!! */

static void NoiseIdct(
  double *y,			/* in:  IDCT input, MODIFIED!!! */
  double *x,			/* out: IDCT output */
  int sizey,			/* in:  length of vector y */
  int sizex,			/* in:  length of vector x */
  int sizedct)			/* in:  length of DCT */
{
  int i,j;

  y[0] /= sqrt(2.);
  for(i=0;i<sizey;i++)
    y[i] *= sqrt(2./(double)sizedct);

  for(j=0;j<sizex;j++) {
    x[j] = 0;
    for(i=0;i<sizey;i++)
      x[j] += y[i]*cos(M_PI*(double)i*((double)j+.5)/(double)sizedct);
  }
}


/* BuildEnv() */
/* Generate envelope from parameters. */

static void BuildEnv (
  double *envPara,		/* in: envelope parameters */
  int frameLen,			/* in: frame length */
  double *env)			/* out: envelope  [0..2*frameLen-1] */
{
  /* HP20020228 fixed time axis sampling */
  int i;
  double t,e;

  for (i=0; i<2*frameLen; i++) {
    t = (i+0.5)/(double)frameLen-0.5;
    if (t<envPara[0])
      e = 1-(envPara[0]-t)*envPara[1];
    else
      e = 1-(t-envPara[0])*envPara[2];
    if (e<0)
      e = 0;
    env[i] = e;
  }

  /* old ...
  double atk_ratei,dec_ratei;
  int maxi;
  int i;

  maxi = (int)(envPara[0]*frameLen);
  atk_ratei = envPara[1]/frameLen;
  dec_ratei = envPara[2]/frameLen;
  for(i=0;i<2*frameLen;i++)
    env[i] = 0;
  i = maxi+frameLen/2;
  while (i>=0) {
    env[i] = 1-atk_ratei*(maxi-(i-frameLen/2));
    if (env[i] <= 0) {
      env[i] = 0;
      i = -1;
    } 
    else
      i--;
  }
  i = maxi+frameLen/2;
  while (i<2*frameLen) {
    env[i] = 1-dec_ratei*((i-frameLen/2)-maxi);
    if (env[i] <= 0) {
      env[i] = 0;
      i = 2*frameLen;
    } 
    else
      i++;
  }
  */
}


/* AntiAlias() */
/* Anti alias filter. */

static double AntiAlias (
  double fSample,		/* in: sampling frequency */
  double freq,			/* in: signal frequency */
  int hard)			/* in: 1 = hard cut-off lpf */
				/* returns: ampl. factor (0..1) */
{
  if (freq>=fSample/2 || freq<=0)
    return 0;
  else if (hard==0 && freq>fSample*ANTIALIAS_FREQ)
    return 1-(freq/fSample-ANTIALIAS_FREQ)/(0.5-ANTIALIAS_FREQ);
  else
    return 1;
}


/* function LPCconvert_k_to_h */
/* -------------------------- */
/* Converts the LPC reflection coefficients into the time response	*/
/* of the prediction filter (in place)					*/

static void LPCconvert_k_to_h(	double *x,	/* in+out: reflection coeff./time response	*/
				long n)		/* in    : number of parameters to convert	*/
{
	register long	i,j;
	register double	a,b,c;

	for (i=1; i<n; i++)
	{
		c=x[i];

		for (j=0; j<i-j-1; j++)
		{
			a=x[j];
			b=x[i-j-1];
			x[  j  ]=a-c*b;
			x[i-j-1]=b-c*a;
		}
		if ( j==i-j-1 ) x[j]-=c*x[j];
	}
}

/* function MakeLPFilter */
/* --------------------- */
/* Generates a lowpass filter with given cutoff freqency fg	*/
/* fg relative to nyquist frequency: fg=1 means interpolate	*/

static void GenLPFilter(double *h,	/* out: time response		*/
			long n,		/* in : filter order		*/
			long os,	/* in : oversampling factor	*/
			double fg)	/* in : cutoff frequency	*/
{
	register double	x,d,f;

	d=M_PI/((double) os);
	x=0.0;
	f=1.0/((double) n);
	*(h++)=(double) fg;
	n*=os;
	while (--n)
	{
		x+=d;
		*(h++)=(double) ((0.54+0.46*cos(f*x))*sin(fg*x)/x);
	}
}


/* function LPInterpolate */
/* ---------------------- */
/* Interpolates the signal in y using the interpolation filter	*/
/* in h at a position pos between two samples.			*/
/* The interpolation is improved by a linear interpolation	*/
/* glen is the whole length (length*os) of the time respone	*/

static double LPInterpolate(	double *y,	/* in    : the signal				*/
				double *h,	/* in    : the filter (time response)		*/
				long glen,	/* in    : whole length of the time response	*/
				long os,	/* in    : oversampling factor of the filter	*/
				double pos)	/* in    : relative position (0<=pos<1)		*/
						/* return: interpolated value			*/
{
	register long	j;
	register double	s,t;

	pos*=(double) os;
	j=(long) pos;
	pos-=(double) j;

	s=t=0.0;
	j=glen-j;

	if ( j==glen ) { t=h[j-1]*(*y); y++; j-=os; }
	while (j>0)
	{
		s+=h[j  ]*(*y);
		t+=h[j-1]*(*y);
		y++;
		j-=os;
	}
	j=-j;
	while (j<glen-1)
	{
		s+=h[j  ]*(*y);
		t+=h[j+1]*(*y);
		y++;
		j+=os;
	}
	if (j<glen) s+=h[j]*(*y);
	
	return (double) (s+pos*(t-s));
}


/* Syn. LPC Noise Back Buffer Size	*/

#define SLNBBSize	(max(LPCMaxNumPara,2*LPCsynLPlen))

/*
static double lar2k(double k)
{
	k=exp(k);
	return (k-1.0)/(k+1.0);
}

static double k2lar(double k)
{
	return log((1.0+k)/(1.0-k));
}
*/

/* function LPCSynNoise */
/* -------------------- */
/* synthesizes a noise like signal as a result of an AR process	*/
/* of arbitrary length and pitch				*/

static void LPCSynNoise(	double *x,	/* out   : synthesized signal				*/
				double *p,	/* in    : p[0]      : Power of the resulting signal,	*/
						/*         p[1..np-1]: LARs				*/
				long np,	/* in    : number of parameters				*/
				long n,		/* in    : number of values to be calculated		*/
				double pitch)	/* in    : pitch factor					*/

{
	double		lp[LPCsynLPlen*LPCsynLPos];
	double		y[SLNBBSize];
	double		h[LPCMaxNumPara];
	register long	i,j,pc;
	register double	a,t,s;

	if ( np<1 )
	{
		for (i=0; i<n; i++) x[i]=0.0;
		return;
	}

	pc=(pitch<0.9999) || (pitch>1.0001);
	np--;

	if (np>=LPCMaxNumPara)
	  np = LPCMaxNumPara-1; /* HP20010102 for -c pf=s */
	for (i=0; i<np; i++) h[i]=p[i+1];
	for (i=0; i<SLNBBSize; i++) y[i]=0.0;

	a=(double) (p[0]*p[0]);
	/* LPCnoiseEncPar.pmin*pow( LPCnoiseEncPar.pmax/LPCnoiseEncPar.pmin, p[1] );	*/


/* fprintf(stderr,"NA=%f, %f %f %f %f %f\n",p[0],k2lar(p[1]),k2lar(p[2]),k2lar(p[3]),k2lar(p[4]),k2lar(p[5])); */

/*
	a=1.0e7;
	{
		double		k[18] = {
			2.0, -0.8, 1.5, 0.5, 1.2, 0.3, 0.7,-0.1, 0.05, 0.15, 0.05,0.0, 0.01, -0.07, 0.4, 0.1, 0.0, 0.0
		};
		int	i;
		for (i=0; i<18; i++) h[i]=lar2k(k[i]);
	}
*/

	for (i=0; i<np; i++) a-=a*((double) (h[i]*h[i]));
	a=sqrt(3.0*a)*(2.0/((double) RND_MAX));

	LPCconvert_k_to_h(h,np);

	if ( pc ) GenLPFilter(lp,LPCsynLPlen,LPCsynLPos, (pitch>1.0) ? (1.0/pitch) : 1.0 );

	t=((double) (SLNBBSize+np))-0.5*pitch;

	j=0;
	for (i=0; i<n; i++)
	{
		t+=pitch;
		while ( t>=1.0 )
		{
			s=a*(((double) random1())-0.5*((double) RND_MAX));
			for (j=          0; j<np; j++) s+=y[j]*h[j];
			for (j=SLNBBSize-1; j> 0; j--) y[j]=y[j-1];
			y[0]=(double) s;
			t-=1.0;

			/* printf("%f\n",y[0]); */
		}

		j=i+n/2;
		if ( j>=n ) j-=n;

		if ( pc ) x[j]=LPInterpolate(y,lp,LPCsynLPlen*LPCsynLPos,LPCsynLPos,t); else x[j]=y[0];
	}
}


#define FASTSINEBITS 8
#define FASTSINEXTRABITS 24

static double *FastSineInit (void)
{
  double *tab;
  int i,j;

  j = 1<<FASTSINEBITS;
  if ((tab = (double *) malloc(sizeof(double)*2*j))==NULL)
    IndiLineExit("FastSineInit: memory allocation error");
  for (i=0; i<j; i++)
    tab[2*i] = cos(2*M_PI*i/(double)j);
  for (i=0; i<j; i++)
    tab[2*i+1] = tab[2*((i+1)&(j-1))]-tab[2*i];
  return (tab);
}

static void FastSineCalc (double phi0, double phi1, double phi2,
		   unsigned long *p, unsigned long *pd, unsigned long *pdd)
{
  /* i = 0..frameLen-1 */
  /* phiT = i+0.5 */
  /* phi = phi0+phi1*phiT+phi2*phiT*phiT */
  /* d phi / dt = phi1+2*phi2*phiT */
  unsigned long j = 1UL<<(FASTSINEXTRABITS+FASTSINEBITS-1);

  phi0 = fmod((phi0+phi1*.5+phi2*.25)/(2*M_PI),1.);
  if (phi0 < 0)
    phi0 += 1.;
  *p = (unsigned long)(phi0*2.*j+.5);   /* phi @ 0.5 */
  if (ULONG_MAX != 0xfffffffful) {      /* long is not 32 bit */
    *p = *p & (unsigned long)((1UL<<(FASTSINEXTRABITS+FASTSINEBITS))-1);
  }
  *pd = (unsigned long)((phi1+2*phi2*1.)/(2*M_PI)*2.*j+.5);   /* w @ 1.0 */
  *pdd = (unsigned long)((2*phi2)/(2*M_PI)*2.*j+j+.5)-j;
}

static void FastSineInc (unsigned long *p, unsigned long pd, int n)
{
  *p += pd*(unsigned)n;
  if (ULONG_MAX != 0xfffffffful) {      /* long is not 32 bit */
    *p = *p & (unsigned long)((1UL<<(FASTSINEXTRABITS+FASTSINEBITS))-1);
  }
}

#ifdef __cplusplus
inline
#endif
static double FastSine (double* tab,
		unsigned long *p, unsigned long pd)
{
  register int i;
  register double k;

  i = ((*p>>FASTSINEXTRABITS)&((1<<FASTSINEBITS)-1))<<1;
  k = 1./(1<<FASTSINEXTRABITS);
  k *= *p&((1<<FASTSINEXTRABITS)-1);
  *p += pd;
  if (ULONG_MAX != 0xfffffffful) {      /* long is not 32 bit */
    *p = *p & (unsigned long)((1UL<<(FASTSINEXTRABITS+FASTSINEBITS))-1);
  }
  return (tab[i]+tab[i+1]*k);
}

#ifdef __cplusplus
inline
#endif
static double FastSineSwp (double* tab,
		   unsigned long *p, unsigned long *pd, unsigned long pdd)
{
  register int i;
  register double k;

  i = ((*p>>FASTSINEXTRABITS)&((1<<FASTSINEBITS)-1))<<1;
  k = 1./(1<<FASTSINEXTRABITS);
  k *= *p&((1<<FASTSINEXTRABITS)-1);
  *p += *pd;
  if (ULONG_MAX != 0xfffffffful) {      /* long is not 32 bit */
    *p = *p & (unsigned long)((1UL<<(FASTSINEXTRABITS+FASTSINEBITS))-1);
  }
  *pd += pdd;
  return (tab[i]+tab[i+1]*k);
}

#ifdef __cplusplus
inline
#endif
static double FastSineSwpAa (double* tab,
		     unsigned long *p, unsigned long *pd, unsigned long pdd)
{
  register int i;
  register double k;

  i = ((*p>>FASTSINEXTRABITS)&((1<<FASTSINEBITS)-1))<<1;
  k = 1./(1<<FASTSINEXTRABITS);
  k *= *p&((1<<FASTSINEXTRABITS)-1);
  *p += *pd;
  if (ULONG_MAX != 0xfffffffful) {      /* long is not 32 bit */
    *p = *p & (unsigned long)((1UL<<(FASTSINEXTRABITS+FASTSINEBITS))-1);
  }
  *pd += pdd;
  if (*pd >= (unsigned long)1UL<<(FASTSINEXTRABITS+FASTSINEBITS-1))
    return (0);   /* anti-alias */
  return (tab[i]+tab[i+1]*k);
}

static void FastSineFree (double *tab)
{
  free (tab);
}


/* ---------- functions (global) ---------- */

/* IndiLineSynthInit */
/* init for IndiLine functions */

ILSstatus *IndiLineSynthInit (
  int maxFrameLen,		/* in: max num samples per frame */
  double fSample,		/* in: sampling frequency */
  int maxNumLine,		/* in: max num lines */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumNoisePara,		/* in: max num noise parameters */
  ILSconfig *cfg,		/* in: ILS configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int bsFormat)			/* in: bit stream format	*/
				/*     <4: DCT-noise  >=4: LPC-noise */
				/* returns: ILS status handle */
{
  ILSstatus *ILS;
  int i;

  if ((ILS = (ILSstatus *) malloc(sizeof(ILSstatus)))==NULL)
    IndiLineExit("IndiLineSynthInit: memory allocation error");

  ILS->cfg = GenILSconfig();

  if (cfg != NULL)
    memcpy(ILS->cfg,cfg,sizeof(ILSconfig));

  ILS->maxFrameLen = maxFrameLen;
  ILS->fSample = fSample;
  ILS->maxNumLine = maxNumLine;
  ILS->maxNumEnv = maxNumEnv;
  ILS->maxNumNoisePara = maxNumNoisePara;
  ILS->debugLevel = debugLevel;
  ILS->maxNoiseLen = 1;
  while (ILS->maxNoiseLen < maxFrameLen)
    ILS->maxNoiseLen *= 2;
  ILS->bsFormat = bsFormat;

  /* allocate memory for arrays */
  if (
      /* fixed content arrays */
      (ILS->longWin=(double*)malloc(maxFrameLen*sizeof(double)))==NULL ||
      (ILS->shortWin=(double*)malloc(maxFrameLen*sizeof(double)))==NULL ||
      (ILS->noiseWin=(double*)malloc(maxFrameLen*sizeof(double)))==NULL ||
      /* variable content arrays */
      (ILS->tmpLinePhase=(double*)malloc(ILS->maxNumLine*
					sizeof(double)))==NULL ||
      (ILS->env=(double**)malloc(max(ILS->maxNumEnv,1)*sizeof(double*)))==NULL ||
      (ILS->env[0]=(double*)malloc(ILS->maxNumEnv*2*maxFrameLen*
				  sizeof(double)))==NULL ||
      (ILS->prevEnv=(double**)malloc(max(ILS->maxNumEnv,1)*
				    sizeof(double*)))==NULL ||
      (ILS->prevEnv[0]=(double*)malloc(ILS->maxNumEnv*2*maxFrameLen*
				      sizeof(double)))==NULL ||
      (ILS->prevLineSyn=(int*)malloc(ILS->maxNumLine*sizeof(int)))==NULL ||
      (ILS->prevLongEnvOut=(int*)malloc(ILS->maxNumEnv*sizeof(int)))==NULL ||
      (ILS->longEnvIn=(int*)malloc(ILS->maxNumEnv*sizeof(int)))==NULL ||
      (ILS->noiseIdctIn=(double*)malloc(ILS->maxNumNoisePara*
				       sizeof(double)))==NULL ||
      (ILS->noiseIdctOut=(double*)malloc(ILS->maxNoiseLen*
					sizeof(double)))==NULL ||
      (ILS->noiseRe=(double*)malloc(ILS->maxNoiseLen*2*sizeof(double)))==NULL ||
      (ILS->noiseIm=(double*)malloc(ILS->maxNoiseLen*2*sizeof(double)))==NULL ||
      (ILS->synthPrevEnv=(double*)malloc(maxFrameLen*sizeof(double)))==NULL ||
      (ILS->synthEnv=(double*)malloc(maxFrameLen*sizeof(double)))==NULL ||
      /* arrays for frame-to-frame memory */
      (ILS->prevEnvPara=(double**)malloc(max(ILS->maxNumEnv,1)*
					sizeof(double*)))==NULL ||
      (ILS->prevEnvPara[0]=(double*)malloc(ILS->maxNumEnv*ENVPARANUM*
					sizeof(double)))==NULL ||
      (ILS->prevLineFreq=(double*)malloc(ILS->maxNumLine*
					sizeof(double)))==NULL ||
      (ILS->prevLineAmpl=(double*)malloc(ILS->maxNumLine*
					sizeof(double)))==NULL ||
      (ILS->prevLinePhase=(double*)malloc(ILS->maxNumLine*
					 sizeof(double)))==NULL ||
      (ILS->prevLineEnv=(int*)malloc(ILS->maxNumLine*sizeof(int)))==NULL ||
      (ILS->prevNoise=(double*)malloc(ILS->maxNoiseLen*2*sizeof(double)))==NULL)
    IndiLineExit("IndiLineSynthInit: memory allocation error");
  
  for (i=1;i<ILS->maxNumEnv;i++) {
    ILS->env[i] = ILS->env[0]+i*2*maxFrameLen;
    ILS->prevEnv[i] = ILS->prevEnv[0]+i*2*maxFrameLen;
    ILS->prevEnvPara[i] = ILS->prevEnvPara[0]+i*ENVPARANUM;
  }

  if (ILS->cfg->lineSynMode==2 || ILS->cfg->lineSynMode==4)
    ILS->fsTab = FastSineInit();

  /* clear frame-to-frame memory */
  ILS->prevNumLine = 0;
  ILS->prevNumEnv = 0;
  ILS->prevNoiseLen = 0;

  /* reset frameLen */
  ILS->frameLen = 0;

  if (ILS->debugLevel>=1)
    printf("maxfrmlen=%d fs=%9.3f numlin=%d\n",
	   ILS->maxFrameLen,ILS->fSample,ILS->maxNumLine);

  return ILS;
}


/* IndiLineSynth */
/* Synthesis of individual lines according to quantised parameters. */
/* enhaMode=0: For basic bitstream: phase information updated internally! */
/* enhaMode=1: For basic+enhancement bitstream: phase information required! */

void IndiLineSynth (
  ILSstatus *ILS,		/* ILS status handle */
  int enhaMode,			/* in: 0=basic mode  1=enhanced mode */
  int frameLen,			/* in: num samples in current frame */
  int numEnv,			/* in: num envelopes */
  double **envPara,		/* in: envelope parameters */
				/*     [0..numEnv-1][0..ENVPARANUM-1] */
  int numLine,			/* in: num lines */
  double *lineFreq,		/* in: line frequency */
				/*     [0..numLine-1] */
  double *lineAmpl,		/* in: line amplitude */
				/*     [0..numLine-1] */
  double *linePhase,		/* in: line phase */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     Note: values may be MODIFIED !!! */
  int *linePhaseValid,		/* in: line phase validity flag */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
  double *linePhaseDelta,	/* in: line phase deviation from */
				/*     linear sweep for cont lines */
				/*     [0..numLine-1] */
				/*     or NULL if not available */
				/*     (utilised only in enha mode) */
  int *lineEnv,			/* in: line envelope flag */
				/*     [0..numLine-1] */
  int *linePred,		/* in: line predecessor idx */
				/*     in prev frame */
				/*     [0..numLine-1] */
  int numNoisePara,		/* in: num noise parameter */
  double noiseFreq,		/* in: max noise freq (bandwidth) [Hz] */
  double *noisePara,		/* in: noise parameter */
				/*     DCT[] / StdDev+RefCoef[] */
				/*     [0..numNoisePara-1] */
  int noiseEnv,			/* in: noise envelope flag/idx */
  float *synthSignal)		/* out: synthesised signal */
				/*      [0..frameLen-1] */
				/*      2nd half previous frame and */
				/*      1st half current frame */
{
  int il,ilPrev;
  int i,j;
  double phi,phiT,phi0,phi1,phi2,phi3,phase,freq;
  double ae,as,a;
  double aed,asd;
  double syn;

  double noiseAmpl,randFlt,qsNoise;
  int noiseLen,noiseBW;

  /* not unused ... */
  void *dmyptr; dmyptr = &linePhaseDelta;

  if (numLine>ILS->maxNumLine || frameLen>ILS->maxFrameLen)
	IndiLineExit("IndiLineSynth: error");

  if (frameLen != ILS->frameLen) {
    ILS->frameLen = frameLen;
    /* calc long and short fade-in windows for new frameLen */
    for (i=0;i<frameLen;i++)
      ILS->longWin[i] = 0.5*(1-cos(M_PI*(i+.5)/frameLen));
    /* HP20020228 fixed time axis sampling for odd frameLen */
    ILS->shortOff = (int)(frameLen/2.*(1-SHORT_FAC)+.5);
    for (i=0;i<frameLen-2*ILS->shortOff;i++)
      ILS->shortWin[i] = 0.5*(1-cos(M_PI*((i+ILS->shortOff-frameLen/2.+.5)/
					  (frameLen*SHORT_FAC)+0.5)));
    /* old ...
    ILS->shortOff = (int)(frameLen/2.*(1-SHORT_FAC));
    for (i=0;i<frameLen-2*ILS->shortOff;i++)
      ILS->shortWin[i] = 0.5*(1-cos(M_PI*(i+.5)/(frameLen-2*ILS->shortOff)));
    */
    /* calc noise window */
    ILS->noiseOff = (int)(frameLen/2.*(1-NOISE_FAC)+.5);
    for (i=0;i<frameLen-2*ILS->noiseOff;i++)
      ILS->noiseWin[i] = sin(M_PI/2.*((i+ILS->noiseOff-frameLen/2.+.5)/
				      (frameLen*NOISE_FAC)+0.5));
    /* old ...
    ILS->noiseOff = (int)(frameLen/2.*(1-NOISE_FAC));
    for (i=0;i<frameLen-2*ILS->noiseOff;i++)
      ILS->noiseWin[i] = sin(M_PI/2.*(i+.5)/(frameLen-2*ILS->noiseOff));
    */
  }

  /* build previous & current envelope */
  for (i=0; i<ILS->prevNumEnv; i++)
    BuildEnv(ILS->prevEnvPara[i],frameLen,ILS->prevEnv[i]);
  for (i=0; i<numEnv; i++)
    BuildEnv(envPara[i],frameLen,ILS->env[i]);

  /* detect slow atk/dec */
  for (i=0; i<ILS->prevNumEnv; i++) {
    ILS->prevLongEnvOut[i] = (ILS->prevEnvPara[i][2] < SHORT_THR) &&
      ((ILS->prevEnvPara[i][0] < 0.5) || (ILS->prevEnvPara[i][1] < SHORT_THR));
    if (ILS->debugLevel>=1)
      printf("synt prev %d env t=%5.2f a=%5.2f d=%5.2f longOut=%1d\n",
	     i,ILS->prevEnvPara[i][0],ILS->prevEnvPara[i][1],
	     ILS->prevEnvPara[i][2],ILS->prevLongEnvOut[i]);
  }
  for (i=0; i<numEnv; i++) {
    ILS->longEnvIn[i] = (envPara[i][1] < SHORT_THR) &&
      ((envPara[i][0] > 0.5) || (envPara[i][2] < SHORT_THR));
    if (ILS->debugLevel>=1)
      printf("          %d env t=%5.2f a=%5.2f d=%5.2f  longIn=%1d\n",
	     i,envPara[i][0],envPara[i][1],envPara[i][2],ILS->longEnvIn[i]);
  }
  if (enhaMode==0)
    linePhase = ILS->tmpLinePhase;

  /* lineSynMode 3, 4 (no sweep = overlap/add)  HP20020117 */
  /* NOTE: this modifies linePred[], linePhase[], linePhaseValid[] */
  if (ILS->cfg->lineSynMode>2 && linePhaseValid)
    for (il=0;il<numLine;il++)
      if (linePred[il] && linePred[il]<=ILS->prevNumLine) {
	/* continued line */
	ilPrev = linePred[il]-1;
	phi0 = ILS->prevLinePhase[ilPrev];
	phi1 = ILS->prevLineFreq[ilPrev]/ILS->fSample*2*M_PI;
	freq = lineFreq[il]/ILS->fSample*2*M_PI;
	/* phi1+2*phi2*frameLen = freq */
	phi2 = (freq-phi1)/(2*frameLen);
	phase = phi0+phi1*frameLen+phi2*frameLen*frameLen;
	linePhase[il] = fmod(phase,2*M_PI);
	linePhaseValid[il] = 1;
	linePred[il] = 0;
      }

  /* clear synthSignal */
  for (i=0;i<frameLen;i++)
    synthSignal[i] = 0;

  for (ilPrev=0;ilPrev<ILS->prevNumLine;ilPrev++)
    ILS->prevLineSyn[ilPrev] = 0;

  if(ILS->cfg->lineSynMode!=2 && ILS->cfg->lineSynMode!=4) {

    /* old line synthesiser */

    for (il=0;il<numLine;il++)
      if (linePred[il] && linePred[il]<=ILS->prevNumLine) {
	/* synthesise continued line */
	ilPrev = linePred[il]-1;
	if (ILS->debugLevel>=2) {
	  printf(" cont ilp=%2d f=%7.1f a=%7.1f ",
		 ilPrev,ILS->prevLineFreq[ilPrev],ILS->prevLineAmpl[ilPrev]);
	  printf("   to  il=%2d f=%7.1f a=%7.1f ",
		 il,lineFreq[il],lineAmpl[il]);
	}
	ILS->prevLineSyn[ilPrev] = 1;

	phi0 = ILS->prevLinePhase[ilPrev];
	phi1 = ILS->prevLineFreq[ilPrev]/ILS->fSample*2*M_PI;
	freq = lineFreq[il]/ILS->fSample*2*M_PI;
	/* phi1+2*phi2*frameLen = freq */
	phi2 = (freq-phi1)/(2*frameLen);
	phase = phi0+phi1*frameLen+phi2*frameLen*frameLen;
	linePhase[il] = fmod(phase,2*M_PI);
	phi3 = 0;

	if ((lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) ||
	    (ILS->prevLineEnv[ilPrev] &&
	     !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1])) {
	  /* short x-fade */
	  if (ILS->debugLevel>=2)
	    printf("short x\n");
	  /* HP20020301 fixed AntiAlias() call */
	  a = ILS->prevLineAmpl[ilPrev];
	  for (i=0;i<ILS->shortOff;i++) {
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT+phi2*phiT*phiT+phi3*phiT*phiT*phiT;
	    synthSignal[i] += (float)
	      (AntiAlias(ILS->fSample,
			 (phi1+2*phi2*phiT+3*phi3*phiT*phiT)
			 /2/M_PI*ILS->fSample,
			 ILS->cfg->lineSynMode)*
	       a*cos(phi)*
	       ((ILS->prevLineEnv[ilPrev])?
		ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1));
	  }
	  for (j=0;j<frameLen-2*ILS->shortOff;j++) {
	    i = ILS->shortOff+j;
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT+phi2*phiT*phiT+phi3*phiT*phiT*phiT;
	    as = ILS->prevLineAmpl[ilPrev]*
	      ((ILS->prevLineEnv[ilPrev])?
	       ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1);
	    ae = lineAmpl[il]*
	      ((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1);
	    /* HP20020228 fixed time axis sampling for odd frameLen */
	    synthSignal[i] += (float)
	      (AntiAlias(ILS->fSample,
			 (phi1+2*phi2*phiT+3*phi3*phiT*phiT)
			 /2/M_PI*ILS->fSample,
			 ILS->cfg->lineSynMode)*
	       (as+(ae-as)*((i-frameLen/2.+0.5)/(frameLen*SHORT_FAC)+0.5))*
	       cos(phi));
	    /* old ...
	    synthSignal[i] += (float)
	      (AntiAlias(ILS->fSample,
			 (phi1+2*phi2*phiT+3*phi3*phiT*phiT)
			 /2/M_PI*ILS->fSample,
			 ILS->cfg->lineSynMode)*
	       (as+(ae-as)*((j+0.5)/(frameLen-2*ILS->shortOff)))*cos(phi));
	    */
	  }
	  /* HP20020301 fixed AntiAlias() call */
	  a = lineAmpl[il];
	  for (i=frameLen-ILS->shortOff;i<frameLen;i++) {
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT+phi2*phiT*phiT+phi3*phiT*phiT*phiT;
	    synthSignal[i] += (float)
	      (AntiAlias(ILS->fSample,
			 (phi1+2*phi2*phiT+3*phi3*phiT*phiT)
			 /2/M_PI*ILS->fSample,
			 ILS->cfg->lineSynMode)*
	       a*cos(phi)*
	       ((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1));
	  }
	}
	else {
	  /* long x-fade */
	  if (ILS->debugLevel>=2)
	    printf("long x\n");
	  for (i=0;i<frameLen;i++) {
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT+phi2*phiT*phiT+phi3*phiT*phiT*phiT;
	    as = ILS->prevLineAmpl[ilPrev]*
	      ((ILS->prevLineEnv[ilPrev])?
	       ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1);
	    ae = lineAmpl[il]*
	      ((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1);
	    synthSignal[i] += (float)
	      (AntiAlias(ILS->fSample,
			 (phi1+2*phi2*phiT+3*phi3*phiT*phiT)
			 /2/M_PI*ILS->fSample,
			 ILS->cfg->lineSynMode)*
	       (as+(ae-as)*((i+0.5)/frameLen))*cos(phi));
	  }
	}
	if (ILS->debugLevel>=2)
	  printf("p0=%9.6f p1=%9.6f p2=%9.6f p3=%9.6f\n",phi0,phi1,phi2,phi3);
      }
      else {
	/* synthesise starting line */
	if (ILS->debugLevel>=2)
	  printf("start  il=%2d f=%7.1f a=%7.1f ",
		 il,lineFreq[il],lineAmpl[il]);
	if (enhaMode==0 || (enhaMode==1 && linePhaseValid[il]==0)) {
	  if (ILS->cfg->rndPhase==1)
	    linePhase[il] = 2.*M_PI*random1()/(double)RND_MAX;
	  else
	    linePhase[il] = M_PI/2;	/* HP 970414 changed 0 -> M_PI/2 */
	}
	phi0 = linePhase[il];
	phi1 = lineFreq[il]/ILS->fSample*2*M_PI;
	a = lineAmpl[il]*AntiAlias(ILS->fSample,lineFreq[il],
				   ILS->cfg->lineSynMode);
	if (lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) {
	  /* short fade-in */
	  if (ILS->debugLevel>=2)
	    printf("short in\n");
	  for (j=0;j<frameLen-2*ILS->shortOff;j++) {
	    i = ILS->shortOff+j;
	    phiT = i-frameLen+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ILS->shortWin[j]*((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1));
	  }
	  for (i=frameLen-ILS->shortOff;i<frameLen;i++) {
	    phiT = i-frameLen+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1));
	  }
	}
	else {
	  /* long fade-in */
	  if (ILS->debugLevel>=2)
	    printf("long in\n");
	  for (i=0;i<frameLen;i++) {
	    phiT = i-frameLen+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ILS->longWin[i]*((lineEnv[il])?ILS->env[lineEnv[il]-1][i]:1));
	  }
	}
	if (ILS->debugLevel>=2)
	  printf("p0=%9.6f p1=%9.6f\n",phi0,phi1);
      }
  
    for (ilPrev=0;ilPrev<ILS->prevNumLine;ilPrev++)
      if (!ILS->prevLineSyn[ilPrev]) {
	/* synthesise ending line */
	if (ILS->debugLevel>=2)
	  printf("  end ilp=%2d f=%7.1f a=%7.1f ",
		 ilPrev,ILS->prevLineFreq[ilPrev],ILS->prevLineAmpl[ilPrev]);
	phi0 = ILS->prevLinePhase[ilPrev];
	phi1 = ILS->prevLineFreq[ilPrev]/ILS->fSample*2*M_PI;
	a = ILS->prevLineAmpl[ilPrev]*AntiAlias(ILS->fSample,
						ILS->prevLineFreq[ilPrev],
						ILS->cfg->lineSynMode);
	if (ILS->prevLineEnv[ilPrev] &&
	    !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1]) {
	  /* short fade-out */
	  if (ILS->debugLevel>=2)
	    printf("short out\n");
	  for (i=0;i<ILS->shortOff;i++) {
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ((ILS->prevLineEnv[ilPrev])?
	       ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1));
	  }
	  for (j=0;j<frameLen-2*ILS->shortOff;j++) {
	    i = ILS->shortOff+j;
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ILS->shortWin[frameLen-2*ILS->shortOff-1-j]*
	      ((ILS->prevLineEnv[ilPrev])?
	       ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1));
	  }
	}
	else {
	  /* long fade-out */
	  if (ILS->debugLevel>=2)
	    printf("long out\n");
	  for (i=0;i<frameLen;i++) {
	    phiT = i+0.5;
	    phi = phi0+phi1*phiT;
	    synthSignal[i] += (float)(a*cos(phi)*
	      ILS->longWin[frameLen-1-i]*
	      ((ILS->prevLineEnv[ilPrev])?
	       ILS->prevEnv[ILS->prevLineEnv[ilPrev]-1][frameLen+i]:1));
	  }
	}
	if (ILS->debugLevel>=2)
	  printf("p0=%9.6f p1=%9.6f\n",phi0,phi1);
      }

  }
  else {

    /* fast line synthesiser */
    /* NOTE: all lines may only use 1st env, no other envs !!! */
    unsigned long p,pd,pdd;

    for (i=0;i<frameLen;i++)
      ILS->synthPrevEnv[i] = ILS->synthEnv[i] = 0;

    for (il=0;il<numLine;il++)
      if (linePred[il] && linePred[il]<=ILS->prevNumLine) {
	/* synthesise continued line */
	ilPrev = linePred[il]-1;
	ILS->prevLineSyn[ilPrev] = 1;
	phi0 = ILS->prevLinePhase[ilPrev];
	phi1 = ILS->prevLineFreq[ilPrev]/ILS->fSample*2*M_PI;
	freq = lineFreq[il]/ILS->fSample*2*M_PI;
	/* phi1+2*phi2*frameLen = freq */
	phi2 = (freq-phi1)/(2*frameLen);
	phase = phi0+phi1*frameLen+phi2*frameLen*frameLen;
	linePhase[il] = fmod(phase,2*M_PI);
	if (ILS->prevLineFreq[ilPrev]==lineFreq[il] && lineFreq[il]/ILS->fSample<.5) {
	  /* no sweep */
#ifdef FASTSYNDBG
	  printf("noswp %2d->%2d %7.1f->%7.1f\n",ilPrev,il,ILS->prevLineFreq[ilPrev],lineFreq[il]);
#endif
	  FastSineCalc(phi0,phi1,phi2,&p,&pd,&pdd);
	  if ((lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) ||
	      (ILS->prevLineEnv[ilPrev] &&
	       !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1])) {
	    /* short x-fade */
	    a = ILS->prevLineAmpl[ilPrev];
	    if (ILS->prevLineEnv[ilPrev])
	      for (i=0;i<ILS->shortOff;i++)
		ILS->synthPrevEnv[i] += a*FastSine(ILS->fsTab,&p,pd);
	    else
	      for (i=0;i<ILS->shortOff;i++)
		synthSignal[i] += (float)(a*FastSine(ILS->fsTab,&p,pd));
	    /* HP20020228 fixed time axis sampling for odd frameLen */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen*SHORT_FAC);
	    as += asd*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    aed = lineAmpl[il]/(frameLen*SHORT_FAC);
	    ae = aed*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    /* old ...
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen-2*ILS->shortOff);
	    as += asd/2;
	    aed = lineAmpl[il]/(frameLen-2*ILS->shortOff);
	    ae = aed/2;
	    */
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  synthSignal[i] += (float)((as+ae)*FastSine(ILS->fsTab,&p,pd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    a = lineAmpl[il];
	    if (lineEnv[il])
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		ILS->synthEnv[i] += a*FastSine(ILS->fsTab,&p,pd);
	    else
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		synthSignal[i] += (float)(a*FastSine(ILS->fsTab,&p,pd));
	  }
	  else {
	    /* long x-fade */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/frameLen;
	    as += asd/2;
	    aed = lineAmpl[il]/frameLen;
	    ae = aed/2;
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSine(ILS->fsTab,&p,pd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  synthSignal[i] += (float)((as+ae)*FastSine(ILS->fsTab,&p,pd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	  }
	}
	else if (ILS->prevLineFreq[ilPrev]/ILS->fSample<.5 &&
		 lineFreq[il]/ILS->fSample<.5) {
	  /* sweep, no anti-alias */
#ifdef FASTSYNDBG
	  printf("sweep %2d->%2d %7.1f->%7.1f\n",ilPrev,il,ILS->prevLineFreq[ilPrev],lineFreq[il]);
#endif
	  FastSineCalc(phi0,phi1,phi2,&p,&pd,&pdd);
	  if ((lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) ||
	      (ILS->prevLineEnv[ilPrev] &&
	       !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1])) {
	    /* short x-fade */
	    a = ILS->prevLineAmpl[ilPrev];
	    if (ILS->prevLineEnv[ilPrev])
	      for (i=0;i<ILS->shortOff;i++)
		ILS->synthPrevEnv[i] += a*FastSineSwp(ILS->fsTab,&p,&pd,pdd);
	    else
	      for (i=0;i<ILS->shortOff;i++)
		synthSignal[i] += (float)(a*FastSineSwp(ILS->fsTab,&p,&pd,pdd));
	    /* HP20020228 fixed time axis sampling for odd frameLen */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen*SHORT_FAC);
	    as += asd*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    aed = lineAmpl[il]/(frameLen*SHORT_FAC);
	    ae = aed*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    /* old ...
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen-2*ILS->shortOff);
	    as += asd/2;
	    aed = lineAmpl[il]/(frameLen-2*ILS->shortOff);
	    ae = aed/2;
	    */
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  synthSignal[i] += (float)((as+ae)*
		    FastSineSwp(ILS->fsTab,&p,&pd,pdd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    a = lineAmpl[il];
	    if (lineEnv[il])
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		ILS->synthEnv[i] += a*FastSineSwp(ILS->fsTab,&p,&pd,pdd);
	    else
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		synthSignal[i] += (float)(a*FastSineSwp(ILS->fsTab,&p,&pd,pdd));
	  }
	  else {
	    /* long x-fade */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/frameLen;
	    as += asd/2;
	    aed = lineAmpl[il]/frameLen;
	    ae = aed/2;
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwp(ILS->fsTab,&p,&pd,pdd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  synthSignal[i] += (float)((as+ae)*
		    FastSineSwp(ILS->fsTab,&p,&pd,pdd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	  }
	}
	else if (ILS->prevLineFreq[ilPrev]/ILS->fSample<.5 ||
		 lineFreq[il]/ILS->fSample<.5) {
	  /* sweep, anti-alias */
#ifdef FASTSYNDBG
	  printf("antia %2d->%2d %7.1f->%7.1f\n",ilPrev,il,ILS->prevLineFreq[ilPrev],lineFreq[il]);
#endif
	  FastSineCalc(phi0,phi1,phi2,&p,&pd,&pdd);
	  if ((lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) ||
	      (ILS->prevLineEnv[ilPrev] &&
	       !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1])) {
	    /* short x-fade */
	    a = ILS->prevLineAmpl[ilPrev];
	    if (ILS->prevLineEnv[ilPrev])
	      for (i=0;i<ILS->shortOff;i++)
		ILS->synthPrevEnv[i] += a*FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
	    else
	      for (i=0;i<ILS->shortOff;i++)
		synthSignal[i] += (float)(a*FastSineSwpAa(ILS->fsTab,&p,&pd,pdd));
	    /* HP20020228 fixed time axis sampling for odd frameLen */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen*SHORT_FAC);
	    as += asd*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    aed = lineAmpl[il]/(frameLen*SHORT_FAC);
	    ae = aed*(frameLen*(SHORT_FAC-1)*0.5+ILS->shortOff+0.5);
	    /* old ...
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/(frameLen-2*ILS->shortOff);
	    as += asd/2;
	    aed = lineAmpl[il]/(frameLen-2*ILS->shortOff);
	    ae = aed/2;
	    */
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=ILS->shortOff;i<frameLen-ILS->shortOff;i++) {
		  synthSignal[i] += (float)((as+ae)*
		    FastSineSwpAa(ILS->fsTab,&p,&pd,pdd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    a = lineAmpl[il];
	    if (lineEnv[il])
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		ILS->synthEnv[i] += a*FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
	    else
	      for (i=frameLen-ILS->shortOff;i<frameLen;i++)
		synthSignal[i] += (float)(a*FastSineSwpAa(ILS->fsTab,&p,&pd,pdd));
	  }
	  else {
	    /* long x-fade */
	    as = ILS->prevLineAmpl[ilPrev];
	    asd = -as/frameLen;
	    as += asd/2;
	    aed = lineAmpl[il]/frameLen;
	    ae = aed/2;
	    if (ILS->prevLineEnv[ilPrev]) {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  ILS->synthPrevEnv[i] += as*syn;
		  synthSignal[i] += (float)(ae*syn);
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	    else {
	      if (lineEnv[il]) {
		for (i=0;i<frameLen;i++) {
		  syn = FastSineSwpAa(ILS->fsTab,&p,&pd,pdd);
		  synthSignal[i] += (float)(as*syn);
		  ILS->synthEnv[i] += ae*syn;
		  as += asd;
		  ae += aed;
		}
	      }
	      else {
		for (i=0;i<frameLen;i++) {
		  synthSignal[i] += (float)((as+ae)*
		    FastSineSwpAa(ILS->fsTab,&p,&pd,pdd));
		  as += asd;
		  ae += aed;
		}
	      }
	    }
	  }
	}
#ifdef FASTSYNDBG
	else
	  printf("quiet %2d->%2d %7.1f->%7.1f\n",ilPrev,il,ILS->prevLineFreq[ilPrev],lineFreq[il]);
#endif

      }
      else {
	/* synthesise starting line */
#ifdef FASTSYNDBG
	printf("start     %2d           %7.1f\n",il,lineFreq[il]);
#endif
	if (enhaMode==0 || (enhaMode==1 && linePhaseValid[il]==0))
	  linePhase[il] = 2.*M_PI*random1()/(double)RND_MAX;
	phi0 = linePhase[il];
	phi1 = lineFreq[il]/ILS->fSample*2*M_PI;
	phi0 = fmod(phi0-phi1*frameLen,2*M_PI);
	if (lineFreq[il]/ILS->fSample<.5) {
	  FastSineCalc(phi0,phi1,0,&p,&pd,&pdd);
	  a = lineAmpl[il];
	  if (lineEnv[il] && !ILS->longEnvIn[lineEnv[il]-1]) {
	    /* short fade-in */
	    FastSineInc(&p,pd,ILS->shortOff);
	    for (j=0;j<frameLen-2*ILS->shortOff;j++) {
	      i = ILS->shortOff+j;
	      ILS->synthEnv[i] += a*FastSine(ILS->fsTab,&p,pd)*
		ILS->shortWin[j];
	    }
	    for (i=frameLen-ILS->shortOff;i<frameLen;i++)
	      ILS->synthEnv[i] += a*FastSine(ILS->fsTab,&p,pd);
	  }
	  else {
	    /* long fade-in */
	    if (lineEnv[il])
	      for (i=0;i<frameLen;i++)
		ILS->synthEnv[i] += a*FastSine(ILS->fsTab,&p,pd)*
		  ILS->longWin[i];
	    else
	      for (i=0;i<frameLen;i++)
		synthSignal[i] += (float)(a*FastSine(ILS->fsTab,&p,pd)*
		  ILS->longWin[i]);
	  }
	}
      }
  
    for (ilPrev=0;ilPrev<ILS->prevNumLine;ilPrev++)
      if (!ILS->prevLineSyn[ilPrev]) {
	/* synthesise ending line */
#ifdef FASTSYNDBG
	printf("end.. %2d     %7.1f         \n",ilPrev,ILS->prevLineFreq[ilPrev]);
#endif
	phi0 = ILS->prevLinePhase[ilPrev];
	phi1 = ILS->prevLineFreq[ilPrev]/ILS->fSample*2*M_PI;
	if (ILS->prevLineFreq[ilPrev]/ILS->fSample<.5) {
	  FastSineCalc(phi0,phi1,0,&p,&pd,&pdd);
	  a = ILS->prevLineAmpl[ilPrev];
	  if (ILS->prevLineEnv[ilPrev] &&
	      !ILS->prevLongEnvOut[ILS->prevLineEnv[ilPrev]-1]) {
	    /* short fade-out */
	    for (i=0;i<ILS->shortOff;i++)
	      ILS->synthPrevEnv[i] += a*FastSine(ILS->fsTab,&p,pd);
	    for (j=0;j<frameLen-2*ILS->shortOff;j++) {
	      i = ILS->shortOff+j;
	      ILS->synthPrevEnv[i] += a*FastSine(ILS->fsTab,&p,pd)*
		ILS->shortWin[frameLen-2*ILS->shortOff-1-j];
	    }
	  }
	  else {
	    /* long fade-out */
	    if (ILS->prevLineEnv[ilPrev])
	      for (i=0;i<frameLen;i++)
		ILS->synthPrevEnv[i] += a*FastSine(ILS->fsTab,&p,pd)*
		  ILS->longWin[frameLen-1-i];
	    else
	      for (i=0;i<frameLen;i++)
		synthSignal[i] += (float)(a*FastSine(ILS->fsTab,&p,pd)*
		  ILS->longWin[frameLen-1-i]);
	  }
	}
      }

    /* window with envelopes and mix */
    for (i=0;i<frameLen;i++)
      synthSignal[i] += (float)
	(ILS->synthPrevEnv[i]*ILS->prevEnv[0][frameLen+i] +
	 ILS->synthEnv[i]*ILS->env[0][i]);
  }

  /* synthesise noise */

  if ( ILS->bsFormat<4 )
  {
    if (numNoisePara) {
      noiseLen = 1;
      while (noiseLen<frameLen)
	noiseLen *= 2;
      for (i=0;i<numNoisePara;i++)
        ILS->noiseIdctIn[i] = noisePara[i]/256.0*noiseLen;
        /* HP 980216   compensate /=2*noiseLen in IODFT (256 sample/frame)*/
      noiseBW = (int)(noiseLen*noiseFreq*2/ILS->fSample+.5);
      NoiseIdct(ILS->noiseIdctIn,ILS->noiseIdctOut,numNoisePara,
		min(noiseLen,noiseBW),noiseBW);
      for (i=0;i<noiseLen;i++) {
        if (i<noiseBW) {
	  noiseAmpl = ILS->noiseIdctOut[i];
	  /* high pass and low pass */
	  if (i==0 || i==noiseBW-1)
	    noiseAmpl = 0;
	  if (i==1 || i==noiseBW-2)
	    noiseAmpl *= 0.3;
	  if (i==2 || i==noiseBW-3)
	    noiseAmpl *= 0.7;
	  noiseAmpl = max(0,noiseAmpl);	/* no negative noise ampl */
        }
        else
	  noiseAmpl = 0;
        randFlt = 2.*M_PI*random1()/(double)RND_MAX;
        ILS->noiseRe[i] = noiseAmpl*cos(randFlt);
        ILS->noiseIm[i] = noiseAmpl*sin(randFlt);
        ILS->noiseRe[2*noiseLen-1-i] = ILS->noiseRe[i];
        ILS->noiseIm[2*noiseLen-1-i] = -ILS->noiseIm[i];
      }
      UHD_iodft(ILS->noiseRe,ILS->noiseIm,2*noiseLen);
    }
    else
      noiseLen = 0;
  }
  else
  {
	noiseLen = 0;

	if ( numNoisePara )
	{
		noiseLen=frameLen;

		LPCSynNoise(ILS->noiseRe,noisePara,numNoisePara,2*noiseLen,(double) (2.0*noiseFreq/ILS->fSample));
	}
  }


  if (ILS->prevNoiseLen)
    for (i=0;i<ILS->noiseOff;i++)
      synthSignal[i] += (float)(ILS->prevNoise[i%(2*ILS->prevNoiseLen)]*
	(ILS->prevNoiseEnv?ILS->prevEnv[ILS->prevNoiseEnv-1][frameLen+i]:1));
  for (i=ILS->noiseOff;i<frameLen-ILS->noiseOff;i++) {
    if (ILS->prevNoiseLen)
      synthSignal[i] += (float)(ILS->prevNoise[i%(2*ILS->prevNoiseLen)]*
	ILS->noiseWin[frameLen-ILS->noiseOff-1-i]*
	(ILS->prevNoiseEnv?ILS->prevEnv[ILS->prevNoiseEnv-1][frameLen+i]:1));
    if (noiseLen)
      synthSignal[i] += (float)(ILS->noiseRe[2*noiseLen-frameLen+i]*
	ILS->noiseWin[i-ILS->noiseOff]*
	(noiseEnv?ILS->env[noiseEnv-1][i]:1));
  }
  if (noiseLen)
    for (i=frameLen-ILS->noiseOff;i<frameLen;i++)
      synthSignal[i] += (float)(ILS->noiseRe[2*noiseLen-frameLen+i]*
	(noiseEnv?ILS->env[noiseEnv-1][i]:1));

  if (ILS->debugLevel>=1) {
    printf("NoiseLen=%4d\n",noiseLen);
    qsNoise = 0;
    for(i=0;i<frameLen;i++)
      qsNoise += (double)(synthSignal[i]*synthSignal[i]);
    printf("qsNoise=%e\n",qsNoise);
  }



  /* copy parameters into frame-to-frame memory */
  ILS->prevNumLine = numLine;
  ILS->prevNumEnv = numEnv;
  for (j=0; j<numEnv; j++)
    for (i=0;i<ENVPARANUM;i++)
      ILS->prevEnvPara[j][i] = envPara[j][i];
  for (i=0;i<numLine;i++) {
    ILS->prevLineFreq[i] = lineFreq[i];
    ILS->prevLineAmpl[i] = lineAmpl[i];
    ILS->prevLinePhase[i] = linePhase[i];
    ILS->prevLineEnv[i] = lineEnv[i];
  }
  ILS->prevNoiseLen = noiseLen;
  ILS->prevNoiseEnv = noiseEnv;

  if ( ILS->bsFormat<4 )
  {
    for (i=0;i<2*noiseLen;i++)
      ILS->prevNoise[i] = -ILS->noiseRe[i];	/* odft: x[0]=-x[N] */
  }
  else
  {
    for (i=0;i<2*noiseLen;i++)
      ILS->prevNoise[i] = ILS->noiseRe[i];	/* ? */
  }
}


/* IndiLineSynthFree() */
/* Free memory allocated by IndiLineSynthInit(). */

void IndiLineSynthFree (
  ILSstatus *ILS)		/* ILS status handle */
{
  if (ILS->cfg->lineSynMode==2 || ILS->cfg->lineSynMode==4)
    FastSineFree(ILS->fsTab);
  free(ILS->longWin);
  free(ILS->shortWin);
  free(ILS->noiseWin);
  free(ILS->tmpLinePhase);
  free(ILS->env[0]);
  free(ILS->env);
  free(ILS->prevEnv[0]);
  free(ILS->prevEnv);
  free(ILS->prevLineSyn);
  free(ILS->prevLongEnvOut);
  free(ILS->longEnvIn);
  free(ILS->noiseIdctIn);
  free(ILS->noiseIdctOut);
  free(ILS->noiseRe);
  free(ILS->noiseIm);
  free(ILS->synthPrevEnv);
  free(ILS->synthEnv);
  free(ILS->prevEnvPara[0]);
  free(ILS->prevEnvPara);
  free(ILS->prevLineFreq);
  free(ILS->prevLineAmpl);
  free(ILS->prevLinePhase);
  free(ILS->prevLineEnv);
  free(ILS->prevNoise);
  free(ILS);
}


/* end of indilinesyn.c */
