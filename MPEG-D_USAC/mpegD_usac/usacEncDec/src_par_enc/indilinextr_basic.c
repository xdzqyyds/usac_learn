/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - HILN: harmonic/individual lines and noise
                              extraction



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


Source file: indilinextr.c



Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
13-nov-97   HP/BE first version based on advanced indilinextr.c
27-nov-97   HP    fixes for some C compilers (no empty struct's)
16-feb-98   HP    fixed longwin seg fault bug (thanks to Matti Koskinen)
16-feb-98   HP    improved noiseFreq, fixed harm maxLineFreq
01-mar-98   HP    ILXharmCorrEnable
31-mar-98   HP    completed IndiLineExtractFree()
09-apr-98   HP    ILXconfig
29-apr-98   HP    fixed specPwr calc
24-feb-99   NM/HP LPC noise
10-sep-99   HP    prev/nextSamples
26-Feb-2002 HP    added added constant frequency estimation code
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "uhd_psy_basic.h"	/* psychoacoustic model */
#include "indilinecom.h"	/* indiline common module */
#include "indilinextr.h"	/* indiline extraction module */

#include "uhd_fft.h"		/* FFT and ODFT module */

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

#define IL_MINFLOAT ((double)1e-35)
#define ATAN2M_MIN_P 1e-6


/* debug flags */

#define MINENVAMPL 0.1

/* window functions */
#define SHORT_RAT 8			/* ratio for short overlap */

#define LONG_THR 5

#define FMAX_FSAMPLE_RATIO 0.49

#define NUM_CONST_FE 4			/* last value for freq. estimation */
#define CONST_OFFS_RAT 12		/* offset ratio relative to frame */


/* ---------- declarations (internal, for IndiLineExtract) ---------- */

/* parameters for array declarations */

/* line continuation and line parameter limits */
#define DFMAX	(.05)		/* max. freq. change for continued lines */
#define DFQMAX	(.15)		/* max. quant. freq. change for cont. lines */
#define FAMAX	(4.)		/* max. ampl. factor for continued lines */
#define FAMIN	(1./FAMAX)	/* min. ampl. factor for continued lines */

#define ILX_FMIN (30.0)		/* min. frequency of extracted line */
#define ILX_MINAMPL (0.1/32768)	/* min. amplitude of extracted line */
				/* 1.0/32768 doesn't give enough lines */
#define ILX_HARMMINAMPL (0.01)	/* count harm lines above -40dB re. max harm */
#define ILX_MINNUMHARM (3)
#define MAX_HARMAMPLPEAK (5.)

#define MAXHARMINDI (3)
#define HARMSPECTHRES (0.5)		/* 0.7 */
#define HARMPWRTHRES (0.8)
#define HARMCORRTHRES (0.9)
#define LINESPECTHRES (0.5)		/* 0.7 */

/* ---------- declarations for envelope and window switching ---------- */

#define ENV_OVER_F (1./16.)	/* overlap factor to find as,ae */
				/* should be 1/(2*OV_RAT) in asdec_o */
#define SINGLE_RATE_THR (1)	/* rate threshold for single slope envelope */

/* parameters for hilbert transform and filter */
#define HWINLEN2_F (1./16.)	/* hilbert window length */
				/* (-HWINLEN2_F*FRMLEN..HWINLEN2_F*FRMLEN) */
#define HWINHAN (1)		/* 0=rectangular 1=hanning window */
#define FWINLEN2_F (1./16.)	/* filter window length (0 -> filter off) */
				/* (-FWINLEN2_F*FRMLEN..FWINLEN2_F*FRMLEN) */
#define FWINHAN (1)		/* 0=rectangular 1=hanning window */

#define ENV_MINFREQ (100.)

/* ---------- declarations for harmonic component extraction ---------- */

#define SPECZEROPAD (2)		/* spectrum zero padding factor (1=off) */
#define SPECMAXWIN (4000)	/* max spectrum window width [Hz] */
#define SPECMINPWR (1e-10)	/* min power spectrum value */
#define MAXFUNDFREQ (1500)	/* max fundamental frequency [Hz] */

static double ILXHminFundFreq = 20.0;
static double ILXHmaxFundFreq = 20000.0;


/* ---------- typedef's for data structures ---------- */

typedef struct ILXEstatusStruct ILXEstatus;	/* ILXE status handle */

typedef struct ILXHstatusStruct ILXHstatus;	/* ILXH status handle */

typedef struct ILXLstatusStruct ILXLstatus;	/* ILXL status handle */

/* ---------- declarations (data structures) ---------- */

/* status variables and arrays for envelope parameter calculation */

struct ILXEstatusStruct {
  /* general parameters */
  int debugLevel;		/* debug level (0=off) */
  int frameLen;			/* frame length */
  int envOver;			/* overlap for envelope calculation */
  int hwinlen2;			/* half of hilbert transform filter length */
  int fwinlen2;			/* half of band pass filter length */
  int envLen;			/* size of env array */
  /* pointers to fixed content arrays ("static") */
  double *hilb;			/* impulse response hilbert transform */
  double *filt;			/* impulse response band pass filter */
  /* pointers to variable content arrays */
  double *xfilt;			/* filter output */
  double *env;			/* output of filter and hilbert transform */
};

/* status variables and arrays for harmonic tone parameter calculation */

struct ILXHstatusStruct {
  /* general parameters */
  int debugLevel;		/* debug level (0=off) */
  int winLen;			/* spectrum window length */
  int specLen;			/* total spectrum length */
  double specWidth;		/* total spectrum bandwidth */

  /* fixed content arrays */
  double *win;			/* frequency window */

  /* variable content arrays */
  double *re;			/* cepstrum (real) */
  double *im;			/* cepstrum (imag.) */
};

/* status variables and arrays for line parameter calculation */

struct ILXLstatusStruct {
  int dummy;			/* some compilers don't like empty struct's */
};

/* status variables and arrays for individual line extraction (ILX) */

struct ILXstatusStruct		/* ILX status handle struct */
{
  /* general parameters */
  int debugLevel;		/* debug level (0=off) */
  int frameLen;			/* frame length */
  double fSample;		/* sampling frequency */
  double maxLineFreq;		/* max. allowed frequency */
  double maxLineAmpl;		/* max. allowed amplitude */
  int maxNumLine;		/* maximum nuber of lines */
  int maxNumEnv;
  int maxNumHarm;
  int maxNumHarmLine;
  int maxNumNoisePara;
  double maxNoiseFreq;
  int maxNumAllLine;
  int bufLen;			/* buffer length */
  int bsFormat;
  int prevSamples;
  int nextSamples;

  /* parameters for frequency and amplitude estimation and psychoacoustics */
  int winLen;			/* window length  */
  int fftLen;			/* transform length  */
  int addStart;			/* add. samples before center window */

  /* fixed content arrays */
  double *costab;		/* cosine table for dft coefficients */
  double *longWin;		/* long window (full overlap) */

  /* arrays for return values */
  double **envPara;		/* envelope parameters */
  double *lineFreq;		/* line frequencies [Hz] */
  double *lineAmpl;		/* line amplitudes */
  double *linePhase;		/* line phase values */
  int *lineEnv;			/* line envelope flags */
  int *linePred;		/* line predecessor indices */
  int *numHarmLine;
  double *harmFreq;
  double *harmFreqStretch;
  int *harmLineIdx;
  int *harmEnv;
  int *harmPred;
  double *harmRate;
  double *noisePara;

  /* variable content arrays */
  double *cos;			/* cosine with current constant frequency */
  double *sin;			/* sine with current constant frequency */
  double *envWin;		/* window with current envelope */
  double *x;			/* residual signal */
  double *xOrgRe;		/* real in/out of dft */
  double *xOrgIm;		/* imag in/out of dft */
  double *xmodRe;		/* real modulated residual */
  double *xmodIm;		/* imag modulated residual */
  double *xsyn;			/* accumulated synth. signal (all lines) */
  double *xsynRe;		/* spectrum of synth. signal (real part) */
  double *xsynIm;		/* spectrum of synth. signal (imag part) */
  double *lineDf;		/* freq. change within the frame */
  int *lflag;			/* flag for excluding lines from freq. est. */
  int *prevLineCont;		/* index of prev. lines for continuation */
  double *harmCosFact;
  double *harmSinFact;
  double *harmFc;
  double *harmFe;
  int *harmPsy;
  
  /* PSYCHOACOUSTIC, currently under test */
  double *excitSynthFft; /* FFT-related excitation of synth. signal */
 
  /* frame-to-frame memory */
  int prevNumEnv;
  double **prevEnvPara;		/* envelope parameters */
  int prevNumLine;		/* number of extracted lines */
  double *prevLineFreq;		/* line frequencies [Hz] */
  double *prevLineAmpl;		/* line amplitudes */
  double *prevLinePhase;		/* line phase values */
  int *prevLineEnv;		/* line envelope flags */
  int prevNumHarm;
  int *prevNumHarmLine;
  double *prevHarmFreq;
  double *prevHarmFreqStretch;
  int *prevHarmLineIdx;
  int *prevHarmEnv;

  double prevFundaF;

  /* status handles */
  ILXEstatus *envStatus;		/* envelope parameter estimation */
  ILXLstatus *lineStatus;		/* line parameter estimation */
  ILXHstatus *harmStatus;		/* harm. tone parameter estimation */
};


/* ---------- functions (internal) ---------- */


/* atan2m  */

static double atan2m(double im, double re)
{
  if(fabs(im)<ATAN2M_MIN_P && fabs(re)<ATAN2M_MIN_P)
    return(0);
  else if(fabs(re)<ATAN2M_MIN_P && im>0)
    return(M_PI/2);
  else if(fabs(re)<ATAN2M_MIN_P && im<0)
    return(-M_PI/2);
  else
    return(atan2(im,re));
}


/* ConstFreqEst() */
/* Estimation of constant frequency by phase regression. */
/*   f(i) = df            t=i/f_sample */
/* phi(i) = phi0 + df*i   t=i/f_sample */

static double ConstFreqEst (
  double *xmodRe,		/* in: complex vector of input samples */
  double *xmodIm,		/*      modulated with frequency -f0 */
  int numSamp2,			/* in: num. regr. samples: 2*numSamp2+1 */
  int sampInt,			/* in: sampling interval for regr. */
  double *win,			/* in: window (corresp. to low pass) */
  int winLen)			/* in: window length */
				/* returns: frequency (df) */
{
  int i,n,numSamp;
  double lxRe,lxIm,nxRe,nxIm,pr,pi,phi,dphi,df;
  double s2,s1p;

  numSamp = 2*numSamp2+1;		/* total number of samples */

  s2 = 0;
  for(i=1;i<=numSamp2;i++) {
    s2 += 2*i*i;
  }

  lxRe = 0;				/* first value           */
  lxIm = 0;				/*  for phase difference */
  for(i=0;i<winLen;i++) {
    lxRe += win[i]*xmodRe[i];
    lxIm += win[i]*xmodIm[i];
  }
  phi = atan2m(lxIm,lxRe);		/* first phase value */
  s1p = -numSamp2*phi;

  for(n=1;n<numSamp;n++) {
    nxRe = 0;				/* calc. new sample after convol. */
    nxIm = 0;				/*  with window function win and */
    for(i=0;i<winLen;i++) {		/*  sampling with int. sweepOffs */
	nxRe += win[i]*xmodRe[n*sampInt+i];
	nxIm += win[i]*xmodIm[n*sampInt+i];
    }
    pr = nxRe*lxRe+nxIm*lxIm;		/* p =  nx * lx'                 */
    pi = nxIm*lxRe-nxRe*lxIm;		/* with ()' = conjugate          */
    dphi = atan2m(pi,pr);		/* -> arg(p) = arg(nx)-arg(lx)   */
    phi += dphi;			/* -> arg(nx) w/o phase discont. */
    i = n-numSamp2;
    s1p += i*phi;			/* sums for 1st order regression */
    lxRe = nxRe;			/* store for next difference */
    lxIm = nxIm;
  }

  df = s1p/s2/sampInt;				/* mean */

  return(df);
}


/* ILXEnvInit() */
/* Init envelope parameter estimation. */

static ILXEstatus *ILXEnvInit (
  int frameLen,			/* in: samples per frame */
  int *addSampStart,		/* out: add. samp. at start */
  int *addSampEnd,		/* out: add. samp. at end */
  int debugLevel)		/* in: debug level (0=off) */
				/* returns: ILXE status handle */
{
  ILXEstatus	*ILXE;
  int i,envAdd,xfiltLen;
  double temp;

  if ((ILXE = (ILXEstatus *) malloc(sizeof(ILXEstatus)))==NULL)
    IndiLineExit("ILXEnvInit: memory allocation error");

  ILXE->debugLevel = debugLevel;
  ILXE->frameLen = frameLen;
  ILXE->envOver = (int)(ENV_OVER_F*frameLen);
  ILXE->hwinlen2 = (int)(HWINLEN2_F*frameLen);
  ILXE->fwinlen2 = (int)(FWINLEN2_F*frameLen);
  envAdd = max(ILXE->envOver,ILXE->frameLen/2);	/* add. samples start & end */
  ILXE->envLen = frameLen+2*envAdd;
  *addSampStart = *addSampEnd = envAdd+ILXE->hwinlen2+ILXE->fwinlen2;

  /* fixed content arrays */
  if(((ILXE->hilb = (double *)malloc((2*ILXE->hwinlen2+1)*sizeof(double)))==NULL)
  || ((ILXE->filt = (double *)malloc((2*ILXE->fwinlen2+1)*sizeof(double)))==NULL))
    IndiLineExit("ILXEnvInit: memory allocation error");

  /* variable content arrays */
  xfiltLen = ILXE->envLen+2*ILXE->hwinlen2;
  if(((ILXE->xfilt = (double *)malloc(xfiltLen*sizeof(double)))==NULL)
  || ((ILXE->env = (double *)malloc(ILXE->envLen*sizeof(double)))==NULL))
    IndiLineExit("ILXEnvInit: memory allocation error");

  /* init hilbert and filter */
  ILXE->hilb[ILXE->hwinlen2] = 0;
  for (i=1;i<=ILXE->hwinlen2;i++) {
    if (i%2) {
      ILXE->hilb[ILXE->hwinlen2+i] = 1/(M_PI/2*i);
      if (HWINHAN)
	ILXE->hilb[ILXE->hwinlen2+i] *= .5+.5*cos(M_PI/ILXE->hwinlen2*i);
    }
    else
      ILXE->hilb[ILXE->hwinlen2+i] = 0;
    ILXE->hilb[ILXE->hwinlen2-i] = -ILXE->hilb[ILXE->hwinlen2+i];
  }
  if (ILXE->fwinlen2==0)
    ILXE->filt[0] = 1;
  else {
    ILXE->filt[ILXE->fwinlen2] = (ILXE->frameLen/2-1)/(double)(ILXE->frameLen/2);
    temp = 0;
    for (i=1;i<=ILXE->fwinlen2;i++) {
      if (i%2==0) {
	ILXE->filt[ILXE->fwinlen2+i] = -1/(double)(ILXE->frameLen/2);
	if (FWINHAN)
	  ILXE->filt[ILXE->fwinlen2+i] *= .5+.5*cos(M_PI/ILXE->fwinlen2*i);
      }
      else
	ILXE->filt[ILXE->fwinlen2+i] = 0;
      ILXE->filt[ILXE->fwinlen2-i] = ILXE->filt[ILXE->fwinlen2+i];
      temp += 2*ILXE->filt[ILXE->fwinlen2+i];
    }
    /* make FILT(f=0) -> 0 */
    temp = -ILXE->filt[ILXE->fwinlen2]/temp;
    for (i=1;i<=ILXE->fwinlen2;i++) {
      ILXE->filt[ILXE->fwinlen2+i] *= temp;
      ILXE->filt[ILXE->fwinlen2-i] *= temp;
    }
    /* make FILT(f=1/4) -> 1 */
    temp = ILXE->filt[ILXE->fwinlen2];
    for (i=2;i<=ILXE->fwinlen2;i+=2) {
      if (i%4==0)
	temp += 2*ILXE->filt[ILXE->fwinlen2+i];
      else
	temp -= 2*ILXE->filt[ILXE->fwinlen2+i];
    }
    for(i=0;i<ILXE->fwinlen2*2+1;i++)
      ILXE->filt[i] /= temp;
  }

  return(ILXE);
}


/* ILXEnvFree() */
/* Free memory allocated by ILXEnvInit(). */

static void ILXEnvFree (
  ILXEstatus *ILXE)		/* in: ILXE status handle*/
{
  free(ILXE->hilb);
  free(ILXE->filt);
  free(ILXE->xfilt);
  free(ILXE->env);
  free(ILXE);
}


/* ILXLineInit() */
/* Init line parameter estimation. */

static ILXLstatus *ILXLineInit (
  int debugLevel)		/* in: debug level (0=off) */
				/* returns: ILXL status handle */
{
  return NULL;
}


/* ILXLineFree() */
/* Free memory allocated by ILXLineInit(). */

static void ILXLineFree (
  ILXLstatus *ILXL)		/* in: ILXL status handle*/
{
}


/* ILXHarmInit() */
/* Init harmonic tone parameter estimation. */

static ILXHstatus *ILXHarmInit (
  int debugLevel,		/* in: debug level (0=off) */
  int specLen,			/* in: spectrum length */
  double specWidth)		/* in: spectrum bandwidth [Hz] */
				/* returns: ILXH status handle */
{
  ILXHstatus	*ILXH;
  int i,len;

  if ((ILXH = (ILXHstatus *) malloc(sizeof(ILXHstatus)))==NULL)
    IndiLineExit("ILXHarmInit: memory allocation error");

  ILXH->debugLevel = debugLevel;
  ILXH->winLen = (int)(min(specWidth,SPECMAXWIN)/specWidth*specLen+.5);
  len = 1;
  while (len < ILXH->winLen)
    len *= 2;
  ILXH->specLen = len*SPECZEROPAD*2;
  ILXH->specWidth = (specWidth/specLen)*ILXH->specLen;

  if(((ILXH->win = (double *)malloc(ILXH->winLen*sizeof(double)))==NULL)
  || ((ILXH->re = (double *)malloc(ILXH->specLen*sizeof(double)))==NULL)
  || ((ILXH->im = (double *)malloc(ILXH->specLen*sizeof(double)))==NULL))
    IndiLineExit("ILXHarmInit: memory allocation error");

  for (i=0; i<ILXH->winLen; i++)
    ILXH->win[i] = (1+cos((i+.5)/(ILXH->winLen)*M_PI))/2;

  return ILXH;
}


/* ILXHarmFree() */
/* Free memory allocated by ILXHarmInit(). */

static void ILXHarmFree (
  ILXHstatus *ILXH)		/* in: ILXH status handle*/
{
  free(ILXH->win);
  free(ILXH->im);
  free(ILXH->re);
  free(ILXH);
}


/* ILXEnvEstim() */
/* Estimate envelope parameter. */

static int ILXEnvEstim (
  ILXEstatus *ILXE,		/* in: ILXE status handle */
  double *x,			/* in: input signal */
  double *envPara,		/* out: envelope parameters */
  double *envWin,		/* out: env. built from par. */
  int envWinLen)		/* length of env. window */
				/* returns: envFlag */
{
  int i,ii,im,maxi,envFlag,envAdd,envWinOff;
  double rms,am,r0,r1,r2,rx0,rx1,ra,rb,wght,temp;
  double atk_ratei,dec_ratei;
  double *xfilt,*filt,*hilb,*env;

  /* set pointers */
  envAdd = max(ILXE->envOver,ILXE->frameLen/2);	/* add. samples start & end */
  xfilt = &ILXE->xfilt[envAdd+ILXE->hwinlen2];	/* filter output */
  filt = &ILXE->filt[ILXE->fwinlen2];		/* filter coefficients */
  hilb = &ILXE->hilb[ILXE->hwinlen2];		/* hilbert coefficients */
  env = &ILXE->env[envAdd];			/* output of filt. and hilb. */

  /* approximate signal envelope based on filter and hilbert transform */
  for (i=-envAdd-ILXE->hwinlen2;i<ILXE->frameLen+envAdd+ILXE->hwinlen2;i++){
    temp = 0;
    for (ii=-ILXE->fwinlen2;ii<=ILXE->fwinlen2;ii++)
      if ((i-ii>=-(ILXE->frameLen/2))&&(i-ii<3*(ILXE->frameLen/2))) /* HP20020124 */
	temp += filt[ii]*x[i-ii];
    xfilt[i] = temp;
  }
  for (i=-envAdd;i<ILXE->frameLen+envAdd;i++){
    temp = 0;
    for (ii=-ILXE->hwinlen2;ii<=ILXE->hwinlen2;ii++)
      temp += hilb[ii]*xfilt[i-ii];
    env[i] = sqrt(xfilt[i]*xfilt[i]+temp*temp);
  }

  /* calc max and rms */
  rms = 0;
  am = 1;
  im = 0;
  for(i=-ILXE->envOver;i<ILXE->frameLen+ILXE->envOver;i++) {
    rms += env[i]*env[i];
    if (env[i]>am) {
      am = env[i];
      im = i;
    }
  }
  im = max(0,min(ILXE->frameLen-1,im));
  rms = sqrt(rms/(ILXE->frameLen+2*ILXE->envOver))/am;

  /* calc envelope parameters using regression */

  /* attack */
  r2 = rx1 = 0;
  for(i=0;i<max(im+ILXE->envOver,ILXE->frameLen/2);i++) {
    temp = env[im-i]/am;
    if (temp<=rms)
      wght = 0;
    else
      wght = pow((temp-rms)/(1-rms),1+i/(double)(ILXE->frameLen/4))*
	(1+i/(double)(ILXE->frameLen/4));
    /* wght = pow((temp-rms)/(1-rms),
       max(1/3.,min(3,i/(double)(ILXE->frameLen/2))))*
       (1+i/(double)(ILXE->frameLen/2)); */
    temp = 1-temp;
    r2 += wght*i*i;
    rx1 += wght*temp*i;
  }
  /* line env[i]=1-ra*(im-i) */
  if (r2==0)
    ra = 0;
  else
    ra = rx1/r2;
  if (ILXE->debugLevel)
    printf("atk: ra=%f",ra);
  atk_ratei = max(0,ra);
  /* calc tmax */;
  maxi = im;

  /* decay */
  r2 = rx1 = 0;
  for(i=0;i<max(ILXE->frameLen+ILXE->envOver-maxi,ILXE->frameLen/2);i++) {
    temp = env[maxi+i]/am;
    if (temp<=rms)
      wght = 0;
    else
      wght = pow((temp-rms)/(1-rms),1+i/(double)(ILXE->frameLen/4))*
	(1+i/(double)(ILXE->frameLen/4));
    /* wght = pow((temp-rms)/(1-rms),
       max(1/3.,min(3,i/(double)(ILXE->frameLen/2))))*
       (1+i/(double)(ILXE->frameLen/2)); */
    temp = 1-temp;
    r2 += wght*i*i;
    rx1 += wght*temp*i;
  }
  /* line env[i]=1-ra*(i-maxi) */
  if (r2==0)
    ra = 0;
  else
    ra = rx1/r2;
  if (ILXE->debugLevel)
    printf("   dec: ra=%f\n",ra);
  dec_ratei = max(0,ra);

  if (ILXE->debugLevel) {
    printf("rms=%f am=%f im=%d\n",rms,am,im);
    printf("prelim. atk_ratei=%f dec_ratei=%f maxi=%d (%f)\n",
	   atk_ratei,dec_ratei,maxi,maxi/(double)ILXE->frameLen);
  }
  /* check if single slope envelope */
  if ((atk_ratei*ILXE->frameLen < SINGLE_RATE_THR) &&
      (dec_ratei*ILXE->frameLen < SINGLE_RATE_THR)) {
    r0 = r1 = r2 = rx0 = rx1 = 0;
    for(i=-ILXE->envOver;i<=ILXE->frameLen+ILXE->envOver;i++) {
      temp = env[i]/am;
      if (temp<rms)
	wght = 0;
      else
	wght = (temp-rms)/(1-rms);
      r0 += wght;
      r1 += wght*i;
      r2 += wght*i*i;
      rx0 += wght*temp;
      rx1 += wght*temp*i;
    }
    /* line: env[i]=ra*i+rb */
    temp = (r1*r1-r2*r0);
    if (temp==0)
      ra = rb = 0;
    else {
      ra = (rx0*r1-rx1*r0)/temp;
      rb = (r1*rx1-r2*rx0)/temp;
    }
    if (ILXE->debugLevel)
      printf("single slope: ra=%f rb=%f",ra,rb);
    if (fabs(ra*ILXE->frameLen) < 0.5) {
      atk_ratei = dec_ratei = 0;
      maxi = 0;
      if (ILXE->debugLevel)
	printf("no env\n");
    }
    else {
      if (ra >= 0) {
	atk_ratei = ra;
	dec_ratei = 0;
	maxi = ILXE->frameLen-1;
      }
      else {
	atk_ratei = 0;
	dec_ratei = -ra;
	maxi = 0;
      }
      if (ILXE->debugLevel)
	printf(" (maxi=%d)\n",maxi);
    }
  }

  envFlag = (atk_ratei != 0) || (dec_ratei != 0);

  if (ILXE->debugLevel)
    printf("env=%d atk_ratei=%f dec_ratei=%f maxi=%d (%f)\n",
	   envFlag,atk_ratei,dec_ratei,maxi,maxi/(double)ILXE->frameLen);

  /* build window according to envelope */

  envWinOff = (envWinLen-ILXE->frameLen)/2;
  for(i=0;i<envWinLen;i++)
    envWin[i] = 0;

  i = maxi+envWinOff;
  while (i>=0) {
    envWin[i] = 1-(atk_ratei)*(maxi-(i-envWinOff));
    if (envWin[i] <= 0) {
      envWin[i] = 0;
      i = -1;
    } 
    else
      i--;
  }
  i = maxi+envWinOff;
  while (i<envWinLen) {
    envWin[i] = 1-(dec_ratei)*((i-envWinOff)-maxi);
    if (envWin[i] <= 0) {
      envWin[i] = 0;
      i = envWinLen;
    } 
    else
      i++;
  }

  envPara[0] = maxi/(double)ILXE->frameLen;
  envPara[1] = atk_ratei*(double)ILXE->frameLen;
  envPara[2] = dec_ratei*(double)ILXE->frameLen;

  return(envFlag);
}


/* FindPeak() */
/* Find peak (local maximum) in array. */
/* x[p-2] < x[p-1] < x[p] > x[p+1] > x[p+2] */

static int FindPeak (
  double *x,			/* in: array */
  double xmin,			/* in: min value of peak (treshold) */
  int imin,			/* in: min index (imin+2<=p) */
  int imax)			/* in: max index (p<=imax-2) */
				/* returns: index p (or -1 if not found) */
{
  int i,ipeak;
  int state;

  ipeak = -1;
  state = 0;
  for (i=imin; i<imax; i++) {
    if (x[i] < x[i+1])
      switch (state) {
	case 0: state=1; break;
	case 1: state=2; break;
	case 2: state=2; break;
	case 3: state=0; break;
      }
    else
      switch (state) {
	case 0: state=0; break;
	case 1: state=0; break;
	case 2: state=3; break;
	case 3:
	  state=0;
	  if (x[i-1] > xmin) {
	    ipeak = i-1;
	    xmin = x[i-1];
	  }
	  break;
      }
  }
  return ipeak;
}


/* ILXHarmFundFreq() */
/* Estimate fundamental frequency (maximum in real cepstrum). */

static double ILXHarmFundFreq (
  ILXHstatus *ILXH,		/* in: ILXH status handle*/
  double *xRe,			/* in: ODFT spectrum (real) */
  double *xIm)			/* in: ODFT spectrum (imag.) */
				/* returns: fundamental frequency [Hz] */
{
  int i,i0;
  double idx,idxmax;
  double cepmax;
  int t,tt;
  int i1,i2;

  if (ILXHminFundFreq == ILXHmaxFundFreq)
    return ILXHmaxFundFreq;		/* no funda freq estim. */

  for (i=0; i<ILXH->winLen; i++) {
    ILXH->re[i] = ILXH->re[ILXH->specLen-1-i] =
      log(max(SPECMINPWR,xRe[i]*xRe[i]+xIm[i]*xIm[i]))*ILXH->win[i];
    ILXH->im[i] = ILXH->im[ILXH->specLen-1-i] = 0;
  }
  for (i=ILXH->winLen; i<ILXH->specLen/2; i++) {
    ILXH->re[i] = ILXH->re[ILXH->specLen-1-i] = 0;
    ILXH->im[i] = ILXH->im[ILXH->specLen-1-i] = 0;
  }
  UHD_iodft(ILXH->re,ILXH->im,ILXH->specLen);

  i0 = (int)(ILXH->specWidth/MAXFUNDFREQ+.5);
  i1 = (int)(ILXH->specWidth/ILXHmaxFundFreq+.5);
  i2 = (int)(ILXH->specWidth/ILXHminFundFreq+.5);

  if (ILXH->debugLevel) {
    printf("0.123456789 ");
    for (i=0; i<ILXH->specLen/2; i++)
      printf("%7f ",ILXH->re[i]);
  }

  i = FindPeak(ILXH->re,0,max(i0-2,i1-2),min(ILXH->specLen/2,i2+2));
  cepmax = ILXH->re[i];
  idxmax = i;
  t = 1;
  for (tt=2; tt<=5; tt++) {
    i = (int)(idxmax*tt);
    if (i+3 < ILXH->specLen/2 &&
	(i = FindPeak(ILXH->re,cepmax/2,i-3,i+3)) != -1) {
      idxmax = (double)i/tt;
      t = tt;
    }
  }
  for (tt=4; tt>1; tt--) {
    idx = idxmax/tt;
    i = (int)(idx+.5);
    if (i >= i1 &&
	FindPeak(ILXH->re,cepmax/2,i-3,i+3) != -1) {
      idxmax = idx;
      t = -tt;
    }
  }

  if (ILXH->debugLevel) {
    printf("%7f %d\n",idxmax,t);
    printf("fndfrq=%7.1f  cep0=%7f  cepmax=%7f  idxmax=%5.1f  r=1/%1d\n",
	   ILXH->specWidth/idxmax,*ILXH->re,cepmax,idxmax,t);
    printf("98765 %7.1f %7f %7f %5.1f %1d\n",
	   ILXH->specWidth/idxmax,*ILXH->re,cepmax,idxmax,t);
  }

  return ILXH->specWidth/idxmax;
}


/* CalcFactors */
/* calculate factors for spectral line with and without envelope, */
/* calc. reduction of signal variance */

static void CalcFactors(
	double	*x,			/* in: input signal */
	double	*cos,			/* in: cosine for given line */
	double	*sin,			/* in: sine for given line */
	double	*win,			/* in: window for corr. and error */
	double	*envWin,		/* in: add. env. (NULL==no env.) */
	int	winLen,			/* in: window length */
	double	*cosFact,		/* out: factor for cosine */
	double	*sinFact,		/* out: factor for sine */
	double	*qeDiff,		/* out: neg. signal power */
	double	*cosFactEnv,		/* out: factor for cosine (env) */
	double	*sinFactEnv,		/* out: factor for sine (env) */
	double	*qeDiffEnv,		/* out: neg. signal power (env) */
					/* use env if qeDiffEnv<qeDiff */
	int	debugLevel)		/* in: debug level (0=off) */
{
  int i;
  double xCos,xSin,wi_2,wi_ex;
  double ccSum,ssSum,csSum,cxSum,sxSum;
  double ccSumEnv,ssSumEnv,csSumEnv,cxSumEnv,sxSumEnv;
  double det;

  ccSum = ssSum = csSum = cxSum = sxSum = 0;
  ccSumEnv = ssSumEnv = csSumEnv = cxSumEnv = sxSumEnv = 0;
  for(i=0;i<winLen;i++) {
    xCos = x[i]*cos[i];
    xSin = x[i]*sin[i];
    wi_2 = win[i]*win[i];
    ccSum += wi_2*cos[i]*cos[i];
    ssSum += wi_2*sin[i]*sin[i];
    csSum += wi_2*cos[i]*sin[i];
    cxSum += wi_2*xCos;
    sxSum += wi_2*xSin;
    if(envWin!=NULL) {
      wi_2 = (envWin[i]*win[i])*(envWin[i]*win[i]);
      wi_ex = (envWin[i]*win[i])*win[i];
      ccSumEnv += wi_2*cos[i]*cos[i];
      ssSumEnv += wi_2*sin[i]*sin[i];
      csSumEnv += wi_2*cos[i]*sin[i];
      cxSumEnv += wi_ex*xCos;
      sxSumEnv += wi_ex*xSin;
    }
  }
  det = max(ccSum*ssSum-csSum*csSum,IL_MINFLOAT);
  *cosFact = (ssSum*cxSum-csSum*sxSum)/det;
  *sinFact = (-ccSum*sxSum+csSum*cxSum)/det;
  *qeDiff = -(*cosFact)*cxSum+(*sinFact)*sxSum;

  if (debugLevel)
    printf("noenv: a=%7.1f, de=%7.1e",
	   sqrt((*cosFact)*(*cosFact)+(*sinFact)*(*sinFact)),*qeDiff);

  if(envWin!=NULL) {
    det = max(ccSumEnv*ssSumEnv-csSumEnv*csSumEnv,IL_MINFLOAT);
    *cosFactEnv =(ssSumEnv*cxSumEnv-csSumEnv*sxSumEnv)/det;
    *sinFactEnv =(-ccSumEnv*sxSumEnv+csSumEnv*cxSumEnv)/det;
    *qeDiffEnv = -(*cosFactEnv)*cxSumEnv+(*sinFactEnv)*sxSumEnv;

    if (debugLevel)
      printf(" env: a=%7.1f, de=%7.1e",
	     sqrt((*cosFactEnv)*(*cosFactEnv)+(*sinFactEnv)*(*sinFactEnv)),
	     *qeDiffEnv);
  }

  if (debugLevel)
    printf("\n");
}


/* WinOdft */
/* multiply with window function and calc. ODFT */

static void WinOdft (
  double	*x,			/* in: input signal */
  double	*win,			/* in: window function */
  int	winLen,			/* in: length of input vector and window */
  int	fftLen,			/* in: transform length */
  double	*xReal,			/* out: transformed vector (real part) */
  double	*xImag)			/* out: transformed vector (imag. part) */
{
  int i;

  if(winLen>fftLen)
    IndiLineExit("WinOdft: winLen>fftLen");

  for(i=0;i<winLen;i++) {
    xReal[i] = win[i]*x[i];
    xImag[i] = 0;
  }
  for(i=winLen;i<fftLen;i++)
    xReal[i] = xImag[i] = 0;
  UHD_odft(xReal,xImag,fftLen);
}


/* EnvPwrFact() */
/* calculate power ratio for envelope */

static double EnvPwrFact (
  double *envPara)
{
  double tAtk, tDec;
  double pAtk, pDec, ac1, ac2;
    
  tAtk = min(envPara[0], 1./max(1.,envPara[1]));
  tDec = min(1.-envPara[0], 1./max(1.,envPara[2]));
  
  ac1 = 1.-envPara[1]*tAtk;
  ac2 = 1.-envPara[2]*tDec;
  
  pAtk = (envPara[1]==0) ? tAtk : 1./(3.*envPara[1])*(1.-ac1*ac1*ac1);
  pDec = (envPara[2]==0) ? tDec : 1./(3.*envPara[2])*(1.-ac2*ac2*ac2);

  return(pAtk+pDec);
}


/* NoiseDct */
/* DCT of arbitrary length (sizex), calculates */
/* only the sizey first output values */

static void NoiseDct(
  double *x,			/* in:  DCT input */
  double *y,			/* out: DCT output */
  int sizex,			/* in:  length of vector x */
  int sizey,			/* in:  length of vector y */
  int sizedct)			/* in:  length of DCT */
{
  int i,j;

  for(i=0;i<sizey;i++) {
    y[i] = 0;
    for(j=0;j<sizex;j++)
      y[i] += x[j]*cos(M_PI*(double)i*((double)j+.5)/(double)sizedct);
    y[i] *= sqrt(2./(double)sizedct);
  }
  y[0] /= sqrt(2.);
}


/* function durbin_akf_to_kh	*/
/* -----------------------------*/
/* calculates optimal reflection coefficients and time response of a	*/
/* prediction filter in LPC analysis					*/
/* prevents the filter from instability by not letting the power of	*/
/* the residual error go below pmin					*/

static void durbin_akf_to_kh(	double *k,	/* out   : reflection coefficients		*/
				double *h,	/* out   : time response			*/
				double *akf,	/* in    : autocorrelation function (0..n used)	*/
				long n)		/* in    : number of parameters to calculate	*/
{
	register long	i,j;
	register double	s;
	register double	a,b,tk,e,pmin;

	e=akf[0];

	pmin=e*((double) (1.0/lpc_max_amp));		/* minimal residual error	*/

	for (i=0; i<n; i++)
	{
		for (j=0,s=0.0; j<i; j++) s+=(double) (h[j]*akf[i-j]);
		tk=(akf[i+1]-((double) s))/e;

		a=e; e-=e*tk*tk;

		if ( e<=pmin )
		{
			b=1.0-pmin/a;
			if ( b<=0.0 ) tk=0.0; else
			{
				if ( tk<0.0 ) tk=-sqrt(b); else tk=sqrt(b);
			}
			e=pmin;
		}

		h[i]=k[i]=tk;

		for (j=0; j<i-j-1; j++)
		{
			a=h[j];
			b=h[i-j-1];
			h[  j  ]=a-tk*b;
			h[i-j-1]=b-tk*a;
		}
		if ( j==i-j-1 ) h[j]-=tk*h[j];
	}
}


static double PWRSpec(double *h,long n,double omega)
{
	register double	 sre,sim,zre,zim,znre,znim,d;
	register long	j;

	znre=sre=1.0;
	znim=sim=0.0;

	zre= cos(omega);
	zim=-sin(omega);
		
	for (j=0; j<n; j++)
	{
		d    = znre*zim;
		znre = znre*zre-znim*zim;
		znim = znim*zre+d;

		sre-=((double) h[j])*znre;
		sim-=((double) h[j])*znim;
	}
	return sre*sre+sim*sim;
}


/* function lpc_noise	*/
/*----------------------*/
/* calculates reflection coefficients (LPC parameters) out off an absolute spectrum	*/

static void lpc_noise(	double *k,	/* out   : reflection koefficients		*/
			long m,		/* in    : calculate m of them			*/
			double *re,	/* in+out: absolute spectrum of the signal	*/
			double *im,	/* out   : only used as scratch	area		*/
			long n)		/* in    : length of re and im (power of 2)	*/
					/* Attention! re and im are overwritten		*/
{
	register double	h;
	register long	i;

	if ( m<=0 ) return;

	for (i=0; i<n; i++) im[i]=0.0;

#ifdef DO_USELESS_LP_OPERATION

	h=re[n/2-1]+re[n/2];
	for (i=n/2-1; i>0; i--)
	{
		register double	h2;

		h2=h+re[i-1]+re[n-i];
		h =re[i]+re[n-i-1];
		h2=0.25*h+0.125*h2;
		h2*=h2;
		re[i]=re[n-i-1]=h2;
	}
	h=0.375*(re[0]+re[n-1]) + 0.125*h;
	re[0]=re[n-1]=h*h;

#else
	for (i=0; i<n/2; i++) re[i]=re[n-1-i]=0.25*(re[i]+re[n-1-i])*(re[i]+re[n-1-i]);
#endif


	UHD_iodft(re,im,(int) n);			/* get the autocorrelation function		*/

	if ( re[0]<0.0 )	/* if the signal is nearly all zero	*/
	{
		k[0]=0.99;
		for (i=1; i<m; i++) k[i]=0.0;
		return;
	}

	durbin_akf_to_kh(k,im,re,m);		/* calculate the reflection-coefficients	*/
						/* and time-response (into im, ignored)		*/
}


/* ---------- functions (global) ---------- */

/* IndiLineExtractInit */
/* init for IndiLineExtract functions */

ILXstatus *IndiLineExtractInit (
  int frameLen,			/* in: samples per frame */
  double fSample,		/* in: sampling frequency */
  double maxLineFreq,		/* in: max line frequency */
  double maxLineAmpl,		/* in: max line amplitude */
  int maxNumLine,		/* in: max num lines */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumHarm,		/* in: max num harmonic tones */
  int maxNumHarmLine,		/* in: max num lines of harmonic tone */
  int maxNumNoisePara,		/* in: max num noise parameters */
  double maxNoiseFreq,		/* in: max noise frequency */
  ILXconfig *cfg,		/* in: ILX configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int bsFormat,			/* in: bitstream format	*/
				/*     <4: DCT-noise  >=4: LPC-noise */
  int prevSamples,		/* in: num samples of previous frame */
  int nextSamples)		/* in: num samples of next frame */
				/* returns: ILX status handle */
{
  ILXstatus *ILX;
  int i,imax_psy,winLen;
  int addStartEnv,addEndEnv;
  double pf;

  if ((ILX = (ILXstatus *) malloc(sizeof(ILXstatus)))==NULL)
    IndiLineExit("IndiLineExtractInit: memory allocation error");

  ILX->debugLevel = debugLevel;
  ILX->frameLen = frameLen;
  ILX->fSample = fSample;
  ILX->maxLineFreq = min(maxLineFreq,fSample*FMAX_FSAMPLE_RATIO);;
  ILX->maxLineAmpl = maxLineAmpl;
  ILX->maxNumLine = maxNumLine;
  ILX->maxNumEnv = maxNumEnv;
  ILX->maxNumHarm = maxNumHarm;
  ILX->maxNumHarmLine = maxNumHarmLine;
  ILX->maxNumNoisePara = maxNumNoisePara;
  ILX->maxNoiseFreq = maxNoiseFreq;
  ILX->maxNumAllLine = maxNumLine+maxNumHarm*maxNumHarmLine;
  ILX->bsFormat = bsFormat;
  ILX->prevSamples = prevSamples;
  ILX->nextSamples = nextSamples;

  if (ILX->debugLevel)
    printf("frmlen=%d fs=%9.3f maxf=%9.3f maxa=%9.3f numlin=%d\n",
	   frameLen,fSample,ILX->maxLineFreq,maxLineAmpl,maxNumLine);

  ILX->winLen = winLen = 2*frameLen;
  ILX->fftLen = 1;				/* transform length  */
  while (ILX->fftLen<winLen)
    ILX->fftLen*=2;
						/*  win. for const freq est. */

  ILX->addStart = (winLen-frameLen)/2;
  ILX->bufLen = ILX->fftLen;		    /* buffer length */

  if (ILX->debugLevel)
    printf("buflen=%d,addStart=%d\n",ILX->bufLen,ILX->addStart);

  /* allocate memory for arrays */

  if (
      /* fixed content arrays ("static") */
      (ILX->costab=(double*)malloc(2*ILX->fftLen*sizeof(double)))==NULL ||
      (ILX->longWin=(double*)malloc(ILX->winLen*sizeof(double)))==NULL ||
      /* arrays for return values */
      (ILX->envPara=(double**)malloc(max(ILX->maxNumEnv,1)*
				    sizeof(double*)))==NULL ||
      (ILX->envPara[0]=(double*)malloc(ILX->maxNumEnv*ENVPARANUM*
				      sizeof(double)))==NULL ||
      (ILX->lineFreq=(double*)malloc(ILX->maxNumAllLine*sizeof(double)))==NULL ||
      (ILX->lineAmpl=(double*)malloc(ILX->maxNumAllLine*sizeof(double)))==NULL ||
      (ILX->linePhase=(double*)malloc(ILX->maxNumAllLine*
				     sizeof(double)))==NULL ||
      (ILX->lineEnv=(int*)malloc(ILX->maxNumAllLine*sizeof(int)))==NULL ||
      (ILX->linePred=(int*)malloc(ILX->maxNumAllLine*sizeof(int)))==NULL ||
      (ILX->numHarmLine=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->harmFreq=(double*)malloc(max(ILX->maxNumHarm,1)*sizeof(double)))==NULL ||
      (ILX->harmFreqStretch=(double*)malloc(max(ILX->maxNumHarm,1)*
					   sizeof(double)))==NULL ||
      (ILX->harmLineIdx=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->harmEnv=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->harmPred=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->harmRate=(double*)malloc(max(ILX->maxNumHarm,1)*sizeof(double)))==NULL ||
      (ILX->noisePara=(double*)malloc(ILX->maxNumNoisePara*
				     sizeof(double)))==NULL ||
      /* variable content arrays */
      (ILX->cos=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->sin=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->envWin=(double*)malloc(ILX->winLen*sizeof(double)))==NULL ||
      (ILX->x=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->xOrgRe=(double*)malloc(ILX->fftLen*sizeof(double)))==NULL ||
      (ILX->xOrgIm=(double*)malloc(ILX->fftLen*sizeof(double)))==NULL ||
      (ILX->xmodRe=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->xmodIm=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->xsyn=(double*)malloc(ILX->bufLen*sizeof(double)))==NULL ||
      (ILX->xsynRe=(double*)malloc(ILX->fftLen*sizeof(double)))==NULL ||
      (ILX->xsynIm=(double*)malloc(ILX->fftLen*sizeof(double)))==NULL ||
      (ILX->lineDf=(double*)malloc(ILX->maxNumAllLine*sizeof(double)))==NULL ||
      (ILX->lflag=(int *)malloc(ILX->fftLen*sizeof(int)))==NULL ||
      (ILX->prevLineCont=(int *)malloc(ILX->maxNumAllLine*
				       sizeof(int)))==NULL ||
      (ILX->harmCosFact=(double*)malloc(ILX->maxNumAllLine*
				       sizeof(double)))==NULL ||
      (ILX->harmSinFact=(double*)malloc(ILX->maxNumAllLine*
				       sizeof(double)))==NULL ||
      (ILX->harmFc=(double*)malloc(ILX->maxNumAllLine*sizeof(double)))==NULL ||
      (ILX->harmFe=(double*)malloc(ILX->maxNumAllLine*sizeof(double)))==NULL ||
      (ILX->harmPsy=(int*)malloc(ILX->maxNumAllLine*sizeof(int)))==NULL ||
      /* arrays for frame-to-frame memory */
      (ILX->prevEnvPara=(double**)malloc(max(ILX->maxNumEnv,1)*
					sizeof(double*)))==NULL ||
      (ILX->prevEnvPara[0]=(double*)malloc(ILX->maxNumEnv*ENVPARANUM*
					  sizeof(double)))==NULL ||
      (ILX->prevLineFreq=(double*)malloc(ILX->maxNumAllLine*
					sizeof(double)))==NULL ||
      (ILX->prevLineAmpl=(double*)malloc(ILX->maxNumAllLine*
					sizeof(double)))==NULL ||
      (ILX->prevLinePhase=(double*)malloc(ILX->maxNumAllLine*
					 sizeof(double)))==NULL ||
      (ILX->prevLineEnv=(int*)malloc(ILX->maxNumAllLine*sizeof(int)))==NULL ||
      (ILX->prevNumHarmLine=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->prevHarmFreq=(double*)malloc(max(ILX->maxNumHarm,1)*
					sizeof(double)))==NULL ||
      (ILX->prevHarmFreqStretch=(double*)malloc(max(ILX->maxNumHarm,1)*
					   sizeof(double)))==NULL ||
      (ILX->prevHarmLineIdx=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL ||
      (ILX->prevHarmEnv=(int*)malloc(max(ILX->maxNumHarm,1)*sizeof(int)))==NULL)
    IndiLineExit("IndiLineExtractInit: memory allocation error");

  for (i=1;i<ILX->maxNumEnv;i++) {
    ILX->envPara[i] = ILX->envPara[0]+i*ENVPARANUM;
    ILX->prevEnvPara[i] = ILX->prevEnvPara[0]+i*ENVPARANUM;
  }
  
  /* init cosine-table */
  pf = M_PI/ILX->fftLen;
  for(i=0;i<2*ILX->fftLen;i++)
    ILX->costab[i] = cos(pf*i);

  /* init window functions and tables */
  for(i=0;i<winLen;i++)			/* long window */
    ILX->longWin[i] = .5*(1.-cos(M_PI*(2.*i+1.)/winLen));
  /* HP 980216   this could caused a seg fault ... thanks to Matti Koskinen
  for(i=winLen;i<ILX->fftLen;i++)
    ILX->longWin[i] = 0;
  */

  /* init frame-to-frame memory */
  ILX->prevNumLine = 0;

  ILX->prevHarmFreq[0] = 0;
  ILX->prevFundaF = 0;

  /* init envelope, line, harmonic tone parameter estimation */
  ILX->envStatus = ILXEnvInit(frameLen,&addStartEnv,&addEndEnv,
				   ILX->debugLevel);
  ILX->lineStatus = ILXLineInit(0);
  ILX->harmStatus = ILXHarmInit((ILX->debugLevel>>8)&3,
				ILX->fftLen,ILX->fSample);

  /* init psychoacoustic model */
  imax_psy = (int)(min(1.2*floor(ILX->fftLen*ILX->maxLineFreq/ILX->fSample),
			ILX->fftLen/2));

  UHD_init_psycho_acoustic(ILX->fSample,imax_psy*ILX->fSample/ILX->fftLen,
			   ILX->fftLen,frameLen,imax_psy,frameLen);

  return(ILX);
}


/* IndiLineExtract */
/* Extraction of individual lines and envelope from the current */
/* residual signal. */

void IndiLineExtract (
  ILXstatus *ILX,		/* in: ILX status handle */
  float *residualSignal,	/* in: residual signal */
				/*  [0..prevSamples+frameLen+nextSamples-1] */
				/*  prevSamples  previous frame, */
				/*  frameLen     current frame, */
				/*  nextSamples  next frame */
  float *synthSignal,		/* in: synthesised signal */
				/*  [as residualSignal] */
				/*  (currently not used) */
  float *finePitchPeriod,	/* in: (currently not used) */
  float *slowHarm,		/* in: (currently not used) */
  float *LPCcoef,		/* in: (currently not used) */
  int numLine,			/* in: num lines to extract */
				/*     -1 to disable extraction */
  int *numEnv,			/* out: num envelopes */
  double ***envPara,		/* out: envelope parameters */
				/*      [0..numEnv-1][0..ENVPARANUM-1] */
  int *numLineExtract,		/* out: num lines extracted */
  double **lineFreq,		/* out: line frequency [Hz] */
				/*      [0..numLine-1] */
  double **lineAmpl,		/* out: line amplitude */
				/*      [0..numLine-1] */
  double **linePhase,		/* out: line phase [rad] */
				/*      [0..numLine-1] */
  int **lineEnv,		/* out: line envelope flag/idx */
				/*      [0..numLine-1] */
  int **linePred,		/* out: line predecessor idx */
				/*      [0..numLine-1] */
  int *numHarm,			/* out: num harmonic tones */
  int **numHarmLine,		/* out: num lines of harmonic tone */
				/*      [0..numHarm-1] */
  double **harmFreq,		/* out: harm fundamental frequency [Hz] */
				/*      [0..numHarm-1] */
  double **harmFreqStretch,	/* out: harm freq stretching */
				/*      [0..numHarm-1] */
  int **harmLineIdx,		/* out: harm line para start idx */
				/*      [0..numHarm-1] */
				/*      ampl: lineAmpl[idx] etc. */
				/*      idx=harmLineIdx+(0..numHarmLine-1) */
  int **harmEnv,		/* out: harm envelope flag/idx */
				/*      [0..numHarm-1] */
  int **harmPred,		/* out: harm tone predecessor idx */
				/*      [0..numHarm-1] */
  double **harmRate,		/* out: bitrate for harm data (0..1) */
				/*      [0..numHarm-1] */
  int *numNoisePara,		/* out: num noise parameter */
  double *noiseFreq,		/* out: max noise freq (bandwidth) [Hz] */
  double **noisePara,		/* out: noise parameter (DCT) */
				/*      [0..numNoisePara-1] */
  int *noiseEnv,		/* out: noise envelope flag/idx */
  double *noiseRate)		/* out: bitrate for noise data (0..1) */

{
  int i,i0,iline,imin,imax,envFlag;	/* counters and indices */

  int frameCenter;			/* index of frame center */
  int constOffsC;			/* offset center window */
  int constOffs;
  int constOffs1;			/* offset first window */

  double qx;				/* square sum of residual */

  /* actual line parameters */
  double actLineFreq = 0;
  double actLineAmpl,actLinePhase;
  double actLineDf = 0;
  int actLineEnv = 0;


  double *win;

  /* variables for psychoacoustic model */
  double err;		/* spectral residual error */
  double emr,emrMax;	/* psychoacoustic criterion */
  double f0;		/* freq. of line with max. emr */
  double df0,fc,fe;	/* sweep rate, center & end frequency */
  double phi1,phi2,phiT;	/* for synthesis */

  /* parameters for lines with const. frequency  */
  double df,f,phi;
  double cfCosFact,cfSinFact,cfQeDiff;
  double cfCosFactEnv,cfSinFactEnv,cfQeDiffEnv;
  double qe;
  int cfEnvFlag;

  /* parameters for synthesis  */
  double *optCos = 0;
  double *optSin = 0;
  double optCosFact = 0;
  double optSinFact = 0;
  double xsynt,asn,aen;

  /* variables for line continuation */
  int bestPred;
  double envAmplE,envAmplS,as,ae,fd,fdq,dfq,fp,fa,q1,q2,q,bestQ;

  /* fundamental frequency */
  double fundaF = 0;
  double fundaRelSweep = 0;
  double fundaFstretch = 0;
  double fundaF_t;
  int iharm;
  int totline;
  double freq;
  double qxOri,qeSum,qeSumEnv;
  int numHarmPsy;
  int harmEnable,checkHarm;
  int di,j;
  int baseIdx = 0;
  int addToMask;

  int harmIndi[MAXHARMINDI];

  int lineGood;
  int numLineBad;
  double harmPwr,harmPwr1,harmPwr2,harmPwr3;
  int harmCorr;
  double minHarmAmpl;

  double a,b,det,sfi1,sfi2,si2,si3,si4;

  /* spectrum pwr */
  double winNorm,specPwr,linePwr,envPwr;

  /* noise */
  double lfNoiseMean;
  double noiseEnvPwrFact;
  int noiseBW;

  /* init internal variables */
  frameCenter = ILX->addStart+ILX->frameLen/2;	/* index of frame center */
  constOffsC = max(0,frameCenter-ILX->winLen/2); /* HP20020124 */
  constOffs = (int)(ILX->frameLen/CONST_OFFS_RAT); /* window shift freq est */
  constOffs1 = constOffsC-NUM_CONST_FE*constOffs; /* offs 1st win. cfe */

  /* index for minimum and maximum frequency */
  imin = min((int)(ILX->fftLen*ILX_FMIN/ILX->fSample),ILX->fftLen/2);
  imax = min((int)(ILX->fftLen*ILX->maxLineFreq/ILX->fSample),ILX->fftLen/2);

  /* core */

  if (ILX->debugLevel)
    printf("IndiLineExtract core\n");

  /* copy input samples */
  for (i=0; i<ILX->bufLen; i++)
    ILX->x[i]=(ILX->prevSamples-ILX->addStart+i <
	       ILX->prevSamples+ILX->frameLen+ILX->nextSamples
	       && ILX->prevSamples-ILX->addStart+i >= 0)
      ? residualSignal[ILX->prevSamples-ILX->addStart+i] : 0;
  /* current frame: x[addStart .. addStart+frameLen-1] */

  if (numLine==-1) {
    /* don't extract anything - return pointers to alloc'ed arrays */

    ILX->prevNumLine = 0;
    ILX->prevNumHarm = 0;
    
    *envPara = ILX->envPara;
    *lineFreq = ILX->lineFreq;
    *lineAmpl = ILX->lineAmpl;
    *linePhase = ILX->linePhase;
    *lineEnv = ILX->lineEnv;
    *linePred = ILX->linePred;
    *numHarmLine = ILX->numHarmLine;
    *harmFreq = ILX->harmFreq;
    *harmFreqStretch = ILX->harmFreqStretch;
    *harmLineIdx = ILX->harmLineIdx;
    *harmEnv = ILX->harmEnv;
    *harmPred = ILX->harmPred;
    *harmRate = ILX->harmRate;
    *noisePara = ILX->noisePara;
    *numEnv=0;
    *numLineExtract=0;
    *numHarm=0;
    *numNoisePara=0;
    *noiseFreq=ILX->maxNoiseFreq;
    *noiseEnv=0;
    *noiseRate=0.0;
    
    return;
  }

  if (ILX->maxNumEnv == 0) {
    *numEnv = 0;
    envFlag = 0;
  }
  else {
    *numEnv = 1;
    /* calculate envelope parameters */
    envFlag = ILXEnvEstim(ILX->envStatus,&ILX->x[ILX->addStart],
			  ILX->envPara[0],ILX->envWin,ILX->winLen);

    if (ILX->debugLevel)
      printf("env: tm=%7.5f atk=%7.5f dec=%7.5f\n",
	     ILX->envPara[0][0],ILX->envPara[0][1],ILX->envPara[0][2]);
  }

  win = ILX->longWin;

  /* reset synth. signal */  
  memset((char *)ILX->xsyn, 0, ILX->bufLen*sizeof(double));
  
  winNorm = 0;
  for(i=0;i<ILX->winLen;i++)
    winNorm += win[i]*win[i];
  /* hanning: winNorm = 3./8.*winLen */

  qx = 0;
  for(i=0;i<ILX->winLen;i++)			/* calc. variance */
    qx += win[i]*win[i]*ILX->x[constOffsC+i]*ILX->x[constOffsC+i];
  qxOri = qx;
  if (ILX->debugLevel)
    printf(" qx=%e\n",qx);

  /* transform original signal to frequency domain */
  WinOdft(&ILX->x[constOffsC],win,ILX->winLen,ILX->fftLen,ILX->xOrgRe,ILX->xOrgIm);

  /* extract harmonic components ? */
  if (ILX->maxNumHarm == 0) {
    *numHarm = 0;
    ILX->numHarmLine[0] = 0;
  }
  else {
    /* fundamental frequency estimation */
    fundaF = ILXHarmFundFreq(ILX->harmStatus,ILX->xOrgRe,ILX->xOrgIm);
    fundaFstretch = 0;
    fundaF_t = fundaF;
    
    fundaRelSweep = 0;
    if (fundaF && ILX->prevFundaF &&
	max(fundaF/ILX->prevFundaF,ILX->prevFundaF/fundaF) < 1.15)
      fundaRelSweep = fundaF/ILX->prevFundaF - 1.;
    ILX->prevFundaF = fundaF;
    if (ILX->debugLevel)
      printf("fundaF %7.1f fundaFstretch %8.5f fundaRelSweep %7.4f\n",
	     fundaF,fundaFstretch,fundaRelSweep);

    for (i=0; i<ILX->maxNumHarm; i++)
      ILX->harmLineIdx[i] = ILX->maxNumLine+i*ILX->maxNumHarmLine;

    *numHarm = 1;
    baseIdx = ILX->harmLineIdx[0];
    ILX->numHarmLine[0] = min(ILX->maxNumHarmLine,
			      (int)(min(ILX->fSample*FMAX_FSAMPLE_RATIO,
					ILX->maxLineFreq)/fundaF));
    if (ILX->debugLevel)
      printf("numHarmLine %2d\n",ILX->numHarmLine[0]);
  }

  iline = -1;
  qeSum = qeSumEnv = 0;
  harmPwr = harmPwr1 = harmPwr2 = harmPwr3 = 0.;

  /* harmonic line loop */
  for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++) {

    /* transform synth. signal to frequency domain */
    WinOdft(&ILX->xsyn[constOffsC],win,ILX->winLen,ILX->fftLen,
	    ILX->xsynRe,ILX->xsynIm);

    f0 = (iharm+1)*(1+(iharm+1)*(iharm+1)*fundaFstretch)*
      2.*M_PI*(fundaF/ILX->fSample);
    df0 = f0*fundaRelSweep;
    i0 = (int)(f0/2./M_PI*ILX->fftLen);
    if (ILX->debugLevel)
      printf("harm idx %2d  harm freq %7.1f\n",
	     iharm+1,(iharm+1)*(1+(iharm+1)*(iharm+1)*fundaFstretch)*fundaF);

    specPwr = 0;
    for (i=max(0,i0-2); i<=min(ILX->fftLen/2-1,i0+2); i++)
      specPwr += ILX->xOrgRe[i]*ILX->xOrgRe[i]+ILX->xOrgIm[i]*ILX->xOrgIm[i];
    specPwr *= (2/(ILX->fftLen*winNorm));

    /* HP20020226 added constant frequency estimation code */
    fc = 2*M_PI*(i0+.5)/ILX->fftLen;	/* frequency of corr. basis function */
    for(i=0;i<ILX->bufLen;i++) {	
      ILX->xmodRe[i] = ILX->x[i]*ILX->costab[((2*i0+1)*i)%(2*ILX->fftLen)];
      ILX->xmodIm[i] = ILX->x[i]
	*ILX->costab[((2*i0+1)*i+ILX->fftLen/2)%(2*ILX->fftLen)];
    }

    df = ConstFreqEst(&ILX->xmodRe[constOffs1],&ILX->xmodIm[constOffs1],
		      NUM_CONST_FE,constOffs,win,ILX->winLen);
      
    if(fabs(df) > 3*M_PI/ILX->fftLen) {
      df = 0;
      if (ILX->debugLevel)
	printf("const df reset!\n");
    }
      
    fc = fc+df;						/* center frequency */
    fe = fc+df0/2;					/* end frequency */

    /* limit fc & fe to FMIN..FMAX !!!   HP 960502 */
    fc = max(min(fc,ILX->maxLineFreq/ILX->fSample*2*M_PI),
		ILX_FMIN/ILX->fSample*2*M_PI);
    fe = max(min(fe,ILX->maxLineFreq/ILX->fSample*2*M_PI),
		ILX_FMIN/ILX->fSample*2*M_PI);

    /* calc. amplitudes and phases  - frequency sweep */
    phi1 = fc;
    /* phi1+2*phi2*ILX->frameLen/2 = fe */
    phi2 = (fe-phi1)/ILX->frameLen;
    /*      printf("fs=%7.2f, ",dphi*ILX->fSample/2/M_PI); */
    for(i=0;i<ILX->bufLen;i++) {		/* gen. complex sine wave */
      phiT = i-frameCenter+0.5;			/*  with frequency sweep */
      phi = phi1*phiT+phi2*phiT*phiT;
      ILX->cos[i] = cos(phi);
      ILX->sin[i] = sin(phi);
    }
    cfEnvFlag = envFlag && fc>ENV_MINFREQ*2*M_PI/ILX->fSample;
    CalcFactors(&ILX->x[constOffsC],&ILX->cos[constOffsC],
		&ILX->sin[constOffsC],win,
		cfEnvFlag ? ILX->envWin : (double*)NULL,
		ILX->winLen,
		&cfCosFact,&cfSinFact,&cfQeDiff,
		&cfCosFactEnv,&cfSinFactEnv,&cfQeDiffEnv,
		ILX->debugLevel);
    qeSum -= cfQeDiff;
    qeSumEnv -= cfEnvFlag ? cfQeDiffEnv : cfQeDiff;
    if (cfEnvFlag && cfQeDiffEnv < cfQeDiff) {
      cfCosFact = cfCosFactEnv;
      cfSinFact = cfSinFactEnv;
      cfQeDiff = cfQeDiffEnv;
    }
    else
      cfEnvFlag = 0;
    qe = qx + cfQeDiff;


    /* store parameters for best synthesis method */
    optCos = ILX->cos;
    optSin = ILX->sin;
    optCosFact = cfCosFact;
    optSinFact = cfSinFact;
    actLineFreq = fc*ILX->fSample/2/M_PI;		/* frequency in Hz */
    actLineDf = (fe-fc)*ILX->fSample/M_PI;		/* frequency change */
    actLineEnv = cfEnvFlag;
    qx = qe;					/* square sum residual error */
    actLineAmpl = sqrt(optCosFact*optCosFact+optSinFact*optSinFact);
    actLinePhase = atan2m(optSinFact,optCosFact);

    ILX->harmCosFact[baseIdx+iharm] = optCosFact;
    ILX->harmSinFact[baseIdx+iharm] = optSinFact;
    ILX->harmFc[baseIdx+iharm] = fc;
    ILX->harmFe[baseIdx+iharm] = fe;
    ILX->lineEnv[baseIdx+iharm] = cfEnvFlag;

    linePwr = actLineAmpl*actLineAmpl/2;
    if (actLineEnv)
      envPwr = EnvPwrFact(ILX->envPara[0]);
    else
      envPwr = 1;
    linePwr *= envPwr;
    if (ILX->debugLevel)
      printf("spec=%f line=%f l/s=%f env=%f\n",
	     specPwr,linePwr,linePwr/specPwr,envPwr);

    harmPwr += linePwr;
    if (linePwr/specPwr > HARMSPECTHRES) {
      harmPwr1 += linePwr;
      if ((iharm+1)%2 == 0)
	harmPwr2 += linePwr;
      if ((iharm+1)%3 == 0)
	harmPwr3 += linePwr;
    }

    /* remove spectral line */
    if(!actLineEnv) {			/* line with const. ampl. */
      for(i=0;i<ILX->bufLen;i++) {
	xsynt = optCosFact*optCos[i]-optSinFact*optSin[i];
	ILX->x[i] -= xsynt;
	ILX->xsyn[i] += xsynt;
      }
    }
    else {				/* line with ampl. envelope */
      asn = ILX->envWin[0];		/* start factor of env window */
      for(i=0;i<constOffsC;i++) {
	xsynt = asn*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	ILX->x[i] -= xsynt;
	ILX->xsyn[i] += xsynt;
      }
      for(i=constOffsC;i<constOffsC+ILX->winLen;i++) {
	xsynt = ILX->envWin[-constOffsC+i]
	  *(optCosFact*optCos[i]-optSinFact*optSin[i]);
	ILX->x[i] -= xsynt;
	ILX->xsyn[i] += xsynt;
      }
      aen = ILX->envWin[ILX->winLen-1];	/* end factor of env window */
      for(i=constOffsC+ILX->winLen;i<ILX->bufLen;i++) {
	xsynt = aen*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	ILX->x[i] -= xsynt;
	ILX->xsyn[i] += xsynt;
      }
    }

    if (ILX->debugLevel) {
      printf("h %2d ",iharm+1);
      printf("%s ",cfEnvFlag?" env ":"noenv");
      printf("sweep: f=%7.1f df=%7.1f a=%7.1f p=%5.2f e2=%7.1e\n",
	     fc*ILX->fSample/2/M_PI,
	     (fe-fc)*ILX->fSample/M_PI,
	     sqrt(cfCosFact*cfCosFact+cfSinFact*cfSinFact),
	     atan2m(cfSinFact,cfCosFact),qe);
    }

    /* store harmonic components */
    ILX->lineFreq[baseIdx+iharm] = actLineFreq;
    ILX->lineAmpl[baseIdx+iharm] = actLineAmpl;
      
  }
  /* end of harmonic line loop */
  
  /* improve funda freq estimate (*1, *2, *3) */
  harmCorr = 0;
  if (harmPwr1/harmPwr > HARMPWRTHRES) {
    harmCorr = 1;
    if (harmPwr2/harmPwr1 > HARMCORRTHRES)
      harmCorr = 2;
    if (harmPwr3/harmPwr1 > HARMCORRTHRES)
      harmCorr = 3;
  }

  if (ILX->debugLevel)
    printf("harmPwr (corr=%1d)  t=%10.2e  1=%10.2e  2=%10.2e  3=%10.2e\n",
	   harmCorr,harmPwr,harmPwr1,harmPwr2,harmPwr3);

  /* funda freq correction */
  if (harmCorr==0)
    ILX->numHarmLine[0] = 0;
  else
    ILX->numHarmLine[0] = ILX->numHarmLine[0]/harmCorr;
  if (harmCorr > 1)
    for (i=0; i<ILX->numHarmLine[0]; i++) {
      ILX->harmCosFact[baseIdx+i] = ILX->harmCosFact[baseIdx+(i+1)*harmCorr-1];
      ILX->harmSinFact[baseIdx+i] = ILX->harmSinFact[baseIdx+(i+1)*harmCorr-1];
      ILX->harmFc[baseIdx+i] = ILX->harmFc[baseIdx+(i+1)*harmCorr-1];
      ILX->harmFe[baseIdx+i] = ILX->harmFe[baseIdx+(i+1)*harmCorr-1];
      ILX->lineEnv[baseIdx+i] = ILX->lineEnv[baseIdx+(i+1)*harmCorr-1];
      ILX->lineFreq[baseIdx+i] = ILX->lineFreq[baseIdx+(i+1)*harmCorr-1];
      ILX->lineAmpl[baseIdx+i] = ILX->lineAmpl[baseIdx+(i+1)*harmCorr-1];
    }

  /* check for low-ampl harm lines */
  minHarmAmpl = 0;
  for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++)
    if (ILX->lineAmpl[baseIdx+iharm] > minHarmAmpl)
      minHarmAmpl = ILX->lineAmpl[baseIdx+iharm];
  minHarmAmpl = max(ILX->maxLineAmpl*ILX_MINAMPL,minHarmAmpl*ILX_HARMMINAMPL);
  numHarmPsy = 0;
  for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++)
    numHarmPsy += (ILX->lineAmpl[baseIdx+iharm] >= minHarmAmpl) ? 1 : 0;
  if (ILX->debugLevel)
    printf("numHarm gt minAmpl %2d\n",numHarmPsy);
  if (numHarmPsy<ILX_MINNUMHARM)
    ILX->numHarmLine[0] = 0;

  ILX->harmFreq[0] = 0;
  ILX->harmFreqStretch[0] = 0;
  if (ILX->numHarmLine[0]) {
    /* stretching: f(i) = f*i(1+s*i) = f*i + s*f*i^2   a=f   b=s*f */
    si2 = si3 = si4 = sfi1 = sfi2 = 0;
    for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++)
      if (ILX->lineAmpl[baseIdx+iharm] >= minHarmAmpl) {
	/* HP 970902   f0,stretch only froum "loud" lines */
	i = iharm+1;
	si2 += (double)i*i;
	si3 += (double)i*i*i;
	si4 += (double)i*i*i*i;
	sfi1 += (double)i*ILX->lineFreq[baseIdx+iharm];
	sfi2 += (double)i*i*ILX->lineFreq[baseIdx+iharm];
      }
    if (ILX->numHarmLine[0] > 1) {
      det = si2*si4-si3*si3;
      a = (sfi1*si4-sfi2*si3)/det;
      b = (si2*sfi2-si3*sfi1)/det;
      ILX->harmFreq[0] = a;
      ILX->harmFreqStretch[0] = b/a;
    }
    else
      ILX->harmFreq[0] = ILX->lineFreq[baseIdx+0];
  }
  if (ILX->debugLevel)
    printf("harmFreq %7.1f  harmFreqStretch %8.1e\n",
	   ILX->harmFreq[0],ILX->harmFreqStretch[0]);
  if (ILX->debugLevel)
    for (i=1;i<=ILX->numHarmLine[0];i++)
      printf("stretch %2d %7.1f (f=%7.1f a=%7.1f)\n",
	     i-1,(ILX->harmFreq[0])*i*(1+(ILX->harmFreqStretch[0])*i),
	     ILX->lineFreq[baseIdx+i-1],ILX->lineAmpl[baseIdx+i-1]);

  ILX->harmEnv[0] = 0;	/* no envelope */
  if (ILX->numHarmLine[0] && envFlag && qeSumEnv>qeSum)
    ILX->harmEnv[0] = 1;
  ILX->harmPred[0] = 0;	/* new harm */
  if (ILX->prevHarmFreq[0]>0 && ILX->harmFreq[0]>0) {
    df = ILX->harmFreq[0]/ILX->prevHarmFreq[0];
    if (max(df,1/df)<1.15)
      ILX->harmPred[0] = 1;	/* continued harm */
  }
  if (ILX->debugLevel)
    printf("harmFreq %7.1f  harmEnv %d  harmPred %d  qe %7.1e  qeEnv %7.1e\n",
	   ILX->harmFreq[0],ILX->harmEnv[0],ILX->harmPred[0],qeSum,qeSumEnv);

  ILX->harmRate[0] = 0.;
  for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++)
    ILX->harmPsy[baseIdx+iharm] = 0;

  /* copy input samples (again...) */
  for (i=0; i<ILX->bufLen; i++)
    ILX->x[i]=(ILX->prevSamples-ILX->addStart+i <
	       ILX->prevSamples+ILX->frameLen+ILX->nextSamples
	       && ILX->prevSamples-ILX->addStart+i >= 0)
      ? residualSignal[ILX->prevSamples-ILX->addStart+i] : 0;
  /* current frame: x[addStart .. addStart+frameLen-1] */

  /* reset synth. signal (again...) */  
  memset((char *)ILX->xsyn, 0, ILX->bufLen*sizeof(double));
  
  qx = qxOri;

  *numLineExtract = numLine;

  totline = 0;
  numHarmPsy = 0;
  numLineBad = 0;
  harmEnable = 0;
  addToMask = 0;
  /* individual line loop */
  for (iline=0; iline<numLine; iline++) {

    /* residual before first iteration = original */
    if (totline==0) {
      /* transform original signal to frequency domain */
      /* obsolete HP20020226
	 WinOdft(&ILX->x[constOffsC],win,ILX->winLen,ILX->fftLen,ILX->xOrgRe,ILX->xOrgIm);
      */

      for (i=imin;i<=imax;i++)
	ILX->lflag[i] = 0;
    }

    /* transform synth. signal to frequency domain */
    WinOdft(&ILX->xsyn[constOffsC],win,ILX->winLen,ILX->fftLen,
	    ILX->xsynRe,ILX->xsynIm);

    /* calc. masking threshold caused by synth. signal (Re,Im,Chan) */
    ILX->excitSynthFft = UHD_psycho_acoustic(ILX->xsynRe,ILX->xsynIm,0); 

    emrMax = IL_MINFLOAT;
    i0 = 0;

    /* find line with maximum "error-to-mask-ratio" */
    for(i=imin;i<=imax;i++) {
      err = sqrt(ILX->xOrgRe[i]*ILX->xOrgRe[i]+ILX->xOrgIm[i]*ILX->xOrgIm[i])
	-sqrt(ILX->xsynRe[i]*ILX->xsynRe[i]+ILX->xsynIm[i]*ILX->xsynIm[i]);
      err = err*err;
      emr = err / ILX->excitSynthFft[i];	/* "error-to-mask-ratio" */

      if(!ILX->lflag[i] && emr>emrMax) {	/* max. value */
	i0 = i;
	emrMax = emr;
      }
    }

    freq = ILX->fSample*(i0+.5)/ILX->fftLen;
    iharm = -1;
    if (ILX->numHarmLine[0] > 0) {
      iharm = (int)(freq/(ILX->harmFreq[0])+.5)-1;
      iharm = (int)(freq/((ILX->harmFreq[0])*(1+(ILX->harmFreqStretch[0])*(iharm+1)))+.5)-1;
    }
    if (iharm >= ILX->numHarmLine[0])
      iharm = -1;

    if (ILX->debugLevel)
      printf("%2d: freq=%7.1f  ",totline,freq);

    specPwr = 0;
    for (i=max(0,i0-2); i<=min(ILX->fftLen/2-1,i0+2); i++)
      specPwr += ILX->xOrgRe[i]*ILX->xOrgRe[i]+ILX->xOrgIm[i]*ILX->xOrgIm[i];
    specPwr *= (2/(ILX->fftLen*winNorm));

    addToMask = 1;

    checkHarm = (iharm>=0 && ILX->harmPsy[baseIdx+iharm]==0 &&
		 fabs(freq-(ILX->harmFreq[0])*
		      (1+(ILX->harmFreqStretch[0])*(iharm+1))*(iharm+1))<20.);
    if (checkHarm) {
      /* take harmonic line */
      if (ILX->debugLevel)
	printf("harm %2d\n",iharm);

      ILX->harmPsy[baseIdx+iharm] = totline+1;

      if (numHarmPsy<MAXHARMINDI)
	harmIndi[numHarmPsy] = iline;
      if (numHarmPsy+1==MAXHARMINDI) {
	harmEnable = 1;
	/* remove MAXHARMINDI harmonic lines from indi line list */
	for (i=0; i<iline-MAXHARMINDI+1; i++) {
	  di = 0;
	  for (j=0; j<MAXHARMINDI; j++)
	    if (harmIndi[j] <= i+di)
	      di++;
	  if (di>0) {
	    ILX->lineFreq[i] = ILX->lineFreq[i+di];
	    ILX->lineAmpl[i] = ILX->lineAmpl[i+di];
	    ILX->linePhase[i] = ILX->linePhase[i+di];
	    ILX->lineEnv[i] = ILX->lineEnv[i+di];
	    ILX->lineDf[i] = ILX->lineDf[i+di];
	    if (ILX->debugLevel)
	      printf("moveindi %2d -> %2d\n",i+di,i);
	  }
	}
	iline -= MAXHARMINDI-1;
	if (ILX->debugLevel)
	  printf("set harmEnable  new iline=%2d (old-%d)\n",
		 iline,MAXHARMINDI-1);
      }
      numHarmPsy++;

      if (harmEnable) {
	fc = ILX->harmFc[baseIdx+iharm];
	fe = ILX->harmFe[baseIdx+iharm];
	phi1 = fc;
	/* phi1+2*phi2*ILX->frameLen/2 = fe */
	phi2 = (fe-phi1)/ILX->frameLen;
	/*      printf("fs=%7.2f, ",dphi*ILX->fSample/2/M_PI); */
	for(i=0;i<ILX->bufLen;i++) {		/* gen. complex sine wave */
	  phiT = i-frameCenter+0.5;		/*  with frequency sweep */
	  phi = phi1*phiT+phi2*phiT*phiT;
	  ILX->cos[i] = cos(phi);
	  ILX->sin[i] = sin(phi);
	}
	optCos = ILX->cos;
	optSin = ILX->sin;
	optCosFact = ILX->harmCosFact[baseIdx+iharm];
	optSinFact = ILX->harmSinFact[baseIdx+iharm];
	actLineEnv = ILX->lineEnv[baseIdx+iharm];
	actLineFreq = ILX->harmFreq[0]*(iharm+1);	/* frequency in Hz stretch???*/
	qx = qx;				/* should be reduced by qe */
      }
    }

    if (!checkHarm || !harmEnable) {
      /* estimate indiline parameter */
      iharm = -1;
      if (ILX->debugLevel)
	printf("indi %2d\n",iline);

      ILX->lflag[i0] = 1;			/* prevent further selection */

      f0 = 2*M_PI*(i0+.5)/ILX->fftLen;	/* frequency of corr. basis function */
      for(i=0;i<ILX->bufLen;i++) {	
	ILX->xmodRe[i] = ILX->x[i]*ILX->costab[((2*i0+1)*i)%(2*ILX->fftLen)];
	ILX->xmodIm[i] = ILX->x[i]
	  *ILX->costab[((2*i0+1)*i+ILX->fftLen/2)%(2*ILX->fftLen)];
      }

      /* HP20020226 added constant frequency estimation code */
      df = ConstFreqEst(&ILX->xmodRe[constOffs1],&ILX->xmodIm[constOffs1],
			NUM_CONST_FE,constOffs,win,ILX->winLen);
      
      if(fabs(df) > 3*M_PI/ILX->fftLen) {
	df = 0;
	if (ILX->debugLevel)
	  printf("const df reset!\n");
      }
      
      f = f0+df;					/* frequency of line */

      /* limit f to FMIN..FMAX !!!   HP 960502 */
      f = max(min(f,ILX->maxLineFreq/ILX->fSample*2*M_PI),
	      ILX_FMIN/ILX->fSample*2*M_PI);

      /* calc. amplitudes and phases   - const. frequency */

      for(i=0;i<ILX->bufLen;i++) {		/* gen. complex sine wave */
	ILX->cos[i] = cos(f*(i-frameCenter+0.5));	/*  e**jf */
	ILX->sin[i] = sin(f*(i-frameCenter+0.5));
      }
      cfEnvFlag = envFlag && f>ENV_MINFREQ*2*M_PI/ILX->fSample;
      CalcFactors(&ILX->x[constOffsC],&ILX->cos[constOffsC],
		  &ILX->sin[constOffsC],win,
		  cfEnvFlag ? ILX->envWin : (double*)NULL,
		  ILX->winLen,
		  &cfCosFact,&cfSinFact,&cfQeDiff,
		  &cfCosFactEnv,&cfSinFactEnv,&cfQeDiffEnv,
		  ILX->debugLevel);
      if (cfEnvFlag && cfQeDiffEnv < cfQeDiff) {
	cfCosFact = cfCosFactEnv;
	cfSinFact = cfSinFactEnv;
	cfQeDiff = cfQeDiffEnv;
      }
      else
	cfEnvFlag = 0;
      qe = qx + cfQeDiff;

      /* store parameters for best synthesis method */
      optCos = ILX->cos;
      optSin = ILX->sin;
      optCosFact = cfCosFact;
      optSinFact = cfSinFact;
      actLineFreq = f*ILX->fSample/2/M_PI;		/* frequency in Hz */
      actLineDf = 0;
      actLineEnv = cfEnvFlag;
      qx = qe;					/* square sum residual error */

      if (ILX->debugLevel) {
	printf("%2d ",iline);
	printf("%s ",cfEnvFlag?" env ":"noenv");
	printf("const: f=%7.1f df=%7s a=%7.1f p=%5.2f e1=%7.1e\n",
	       f*ILX->fSample/2/M_PI,"",
	       sqrt(cfCosFact*cfCosFact+cfSinFact*cfSinFact),
	       atan2m(cfSinFact,cfCosFact),qe);
      }
    }

    actLineAmpl = sqrt(optCosFact*optCosFact+optSinFact*optSinFact);
    actLinePhase = atan2m(optSinFact,optCosFact);

    linePwr = actLineAmpl*actLineAmpl/2;
    if (actLineEnv)
      envPwr = EnvPwrFact(ILX->envPara[0]);
    else
      envPwr = 1;
    linePwr *= envPwr;
    lineGood = linePwr/specPwr > LINESPECTHRES;
    if (ILX->debugLevel)
      printf("lineGood %1d spec=%f line=%f l/s=%f env=%f\n",
	     lineGood,specPwr,linePwr,linePwr/specPwr,envPwr);

    /* remove spectral line */
    if(!actLineEnv) {			/* line with const. ampl. */
      for(i=0;i<ILX->bufLen;i++) {
	xsynt = optCosFact*optCos[i]-optSinFact*optSin[i];
	if (lineGood)
	  ILX->x[i] -= xsynt;
	if (addToMask)
	  ILX->xsyn[i] += xsynt;
      }
    }
    else {				/* line with ampl. envelope */
      asn = ILX->envWin[0];		/* start factor of env window */
      for(i=0;i<constOffsC;i++) {
	xsynt = asn*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	if (lineGood)
	  ILX->x[i] -= xsynt;
	if (addToMask)
	  ILX->xsyn[i] += xsynt;
      }
      for(i=constOffsC;i<constOffsC+ILX->winLen;i++) {
	xsynt = ILX->envWin[-constOffsC+i]
	  *(optCosFact*optCos[i]-optSinFact*optSin[i]);
	if (lineGood)
	  ILX->x[i] -= xsynt;
	if (addToMask)
	  ILX->xsyn[i] += xsynt;
      }
      aen = ILX->envWin[ILX->winLen-1];	/* end factor of env window */
      for(i=constOffsC+ILX->winLen;i<ILX->bufLen;i++) {
	xsynt = aen*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	if (lineGood)
	  ILX->x[i] -= xsynt;
	if (addToMask)
	  ILX->xsyn[i] += xsynt;
      }
    }

    totline++;
    if (iharm>=0)
      iline--;
    else {
      if (lineGood==0)
	numLineBad++;
      /* exit line loop if amplitude is too low */
      /* or if more than numline (!!!) "lineGood=0"-lines were encountered */
      if (actLineAmpl < ILX->maxLineAmpl*ILX_MINAMPL ||
	  numLineBad > numLine) {
	*numLineExtract = iline;
	if (ILX->debugLevel)
	  printf("exit line loop (ampl %8.2f < %8.2f)\n",
		 actLineAmpl,ILX->maxLineAmpl*ILX_MINAMPL);
	break;   /* exit line loop */
      }

      if (lineGood ||
	  (checkHarm && !harmEnable)) {	/* HP 970901, moveindi & lineGood=0 */
	/* output line parameters */
	ILX->lineFreq[iline] = actLineFreq;
	ILX->lineAmpl[iline] = actLineAmpl;
	ILX->linePhase[iline] = actLinePhase;
	ILX->lineEnv[iline] = actLineEnv;
	ILX->lineDf[iline] = actLineDf;
      }
      else
	iline--;
    }

  }
  /* end of individual line loop */

  /* check num harm used in il loop */
  ILX->harmRate[0] = numHarmPsy/(double)(numHarmPsy+*numLineExtract);
  if (ILX->debugLevel)
    printf("totline %2d  numLineBad %2d  numHarmPsy %2d  numLineExtract %2d\n"
	   "harmEnable %1d  harmRate %5.3f\n",
	   totline,numLineBad,numHarmPsy,*numLineExtract,
	   harmEnable,ILX->harmRate[0]);

  if (!harmEnable)
    ILX->numHarmLine[0] = 0;

  if (ILX->numHarmLine[0] == 0)
    *numHarm = 0;

  /* remove remaining harm lines from residual signal */
  if (ILX->debugLevel) {
    q = 0;
    for(i=0;i<ILX->winLen;i++)			/* calc. variance */
      q += win[i]*win[i]*ILX->x[constOffsC+i]*ILX->x[constOffsC+i];
    printf("remain harm   pre qx=%e",q);
  }

  if (harmEnable) {
    for (iharm=0; iharm<ILX->numHarmLine[0]; iharm++) {
      if (ILX->harmPsy[baseIdx+iharm]==0) {

	fc = ILX->harmFc[baseIdx+iharm];
	fe = ILX->harmFe[baseIdx+iharm];
	phi1 = fc;
	/* phi1+2*phi2*ILX->frameLen/2 = fe */
	phi2 = (fe-phi1)/ILX->frameLen;
	/*      printf("fs=%7.2f, ",dphi*ILX->fSample/2/M_PI); */
	for(i=0;i<ILX->bufLen;i++) {		/* gen. complex sine wave */
	  phiT = i-frameCenter+0.5;		/*  with frequency sweep */
	  phi = phi1*phiT+phi2*phiT*phiT;
	  ILX->cos[i] = cos(phi);
	  ILX->sin[i] = sin(phi);
	}

	actLineEnv = ILX->lineEnv[baseIdx+iharm];
	CalcFactors(&ILX->x[constOffsC],&ILX->cos[constOffsC],
		    &ILX->sin[constOffsC],win,
		    actLineEnv ? ILX->envWin : (double*)NULL,
		    ILX->winLen,
		    &cfCosFact,&cfSinFact,&cfQeDiff,
		    &cfCosFactEnv,&cfSinFactEnv,&cfQeDiffEnv,
		    ILX->debugLevel);

	optCos = ILX->cos;
	optSin = ILX->sin;

	if(!actLineEnv) {			/* line with const. ampl. */
	  optCosFact = cfCosFact;
	  optSinFact = cfSinFact;
	}
	else {
	  optCosFact = cfCosFactEnv;
	  optSinFact = cfSinFactEnv;
	}

	/* remove spectral line */
	if(!actLineEnv) {			/* line with const. ampl. */
	  for(i=0;i<ILX->bufLen;i++) {
	    xsynt = optCosFact*optCos[i]-optSinFact*optSin[i];
	    ILX->x[i] -= xsynt;
	  }
	}
	else {				/* line with ampl. envelope */
	  asn = ILX->envWin[0];		/* start factor of env window */
	  for(i=0;i<constOffsC;i++) {
	    xsynt = asn*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	    ILX->x[i] -= xsynt;
	  }
	  for(i=constOffsC;i<constOffsC+ILX->winLen;i++) {
	    xsynt = ILX->envWin[-constOffsC+i]
	      *(optCosFact*optCos[i]-optSinFact*optSin[i]);
	    ILX->x[i] -= xsynt;
	  }
	  aen = ILX->envWin[ILX->winLen-1];	/* end factor of env window */
	  for(i=constOffsC+ILX->winLen;i<ILX->bufLen;i++) {
	    xsynt = aen*(optCosFact*optCos[i]-optSinFact*optSin[i]);
	    ILX->x[i] -= xsynt;
	  }
	}

      }
    }
  }

  if (ILX->debugLevel) {
    q = 0;
    for(i=0;i<ILX->winLen;i++)			/* calc. variance */
      q += win[i]*win[i]*ILX->x[constOffsC+i]*ILX->x[constOffsC+i];
    printf("   post qx=%e\n",q);
  }


#ifdef ASDFASDFASDF
  /* gen 1..5 harm at f0 ... */
  if (numLine <= 5) {
    fundaA = 0;
    for (iline=0; iline<numLine; iline++)
      if (ILX->lineAmpl[iline] > fundaA)
	fundaA = ILX->lineAmpl[iline];
    *numLineExtract = numLine;
    for (iline=0; iline<numLine; iline++) {
      ILX->lineFreq[iline] = (iline+1)*fundaF;
      ILX->lineAmpl[iline] = fundaA/(iline+1);
      ILX->lineAmpl[iline] = fundaA;
      ILX->linePhase[iline] = 0;
      ILX->lineEnv[iline] = 0;
      ILX->lineDf[iline] = 0;
    }
  }

  for (iline=0; iline<*numLineExtract; iline++)
    ILX->lineFreq[iline] = (iline+1)*fundaF_t;
#endif

  
  /* find predecessors for continued lines */
  
  envAmplE = max(MINENVAMPL,1-((1-ILX->prevEnvPara[0][0])*ILX->prevEnvPara[0][2]));
  envAmplS = max(MINENVAMPL,1-(ILX->envPara[0][0]*ILX->envPara[0][1]));
  for (i=0;i<ILX->prevNumLine;i++)
    ILX->prevLineCont[i]=0;
  for (iline=0;iline<*numLineExtract;iline++) {
    as = ILX->lineAmpl[iline]*(ILX->lineEnv[iline]?envAmplS:1);
    fdq = ILX->lineFreq[iline];
    fd = fdq-ILX->lineDf[iline];
    bestPred = 0;
    bestQ = 0;
    for (i=0;i<ILX->prevNumLine;i++) {
      if (!ILX->prevLineCont[i]) {
	ae = ILX->prevLineAmpl[i]*(ILX->prevLineEnv[i]?envAmplE:1);
	fp = ILX->prevLineFreq[i];
	/* calc q */
	if (ae>0 && as>0 && fp>0 && fd>0) {
	  dfq = fabs(fdq-fp)/fp;
	  df = fabs(fd-fp)/fp;
	  fa = as/ae;
	  if(dfq<DFQMAX && df<DFMAX && fa>FAMIN && fa<FAMAX) {
	    q1 = 1.-df/DFMAX;
	    if(fa<1)
	      q2 = (fa-FAMIN)/(1-FAMIN);
	    else
	      q2 = (1./fa-FAMIN)/(1-FAMIN);
	    q = q1*q2;
	    if (q > bestQ) {
	      bestQ = q;
	      bestPred = i+1;
	    }
	  }
	}
      }
    }
    ILX->linePred[iline] = bestPred;
    if (bestPred)
      ILX->prevLineCont[bestPred-1] = 1;
  }

  /* calc. parameters for residual noise */
  if (ILX->maxNumNoisePara == 0)
    *numNoisePara = 0;
  else {
    if (ILX->maxNumEnv <= 1) {
      *noiseEnv = 0;
      noiseEnvPwrFact = 1;
    }
    else {
      *numEnv = 2;
      /* calculate new envelope parameters for the residual signal */
      if (ILXEnvEstim(ILX->envStatus,&ILX->x[ILX->addStart],
		      ILX->envPara[1],ILX->envWin,ILX->winLen))
	*noiseEnv = 2;
      else
	*noiseEnv = 0;
#ifdef NOISE_ENV_THRESHOLD
      /* doesn't help, simply use me=1 (default) also for casta !!! */
      if (ILX->envPara[1][0]*ILX->envPara[1][1] < 1 &&
	  (1-ILX->envPara[1][0])*ILX->envPara[1][2] < 1) {
	/* env: a(t)>0 for all t=0..1 -> don't use this env */
	*noiseEnv = 0;
	ILX->envPara[1][0] = ILX->envPara[1][1] = ILX->envPara[1][2] = 0;
      }
#endif
      noiseEnvPwrFact = EnvPwrFact(ILX->envPara[1]);
    }
    
    /* residual variance */
    qx = 0;
    for(i=0;i<ILX->winLen;i++) {
      qx += win[i]*win[i]*ILX->x[constOffsC+i]*ILX->x[constOffsC+i];
    }

    if ( ILX->bsFormat<4 ) {
      /* DCT noise */
      WinOdft(&ILX->x[constOffsC],win,ILX->winLen,ILX->fftLen,
	      ILX->xsynRe,ILX->xsynIm);

      for(i=0;i<ILX->fftLen/2;i++)
	ILX->xsynRe[i] = sqrt(ILX->xsynRe[i]*ILX->xsynRe[i]+
			      +ILX->xsynIm[i]*ILX->xsynIm[i])/noiseEnvPwrFact;
      lfNoiseMean = 0;
      for(i=3;i<10;i++)
	lfNoiseMean += ILX->xsynRe[i];
      lfNoiseMean /= 8;
      ILX->xsynRe[0] = lfNoiseMean;
      ILX->xsynRe[1] = .7*lfNoiseMean+.3*ILX->xsynRe[1];
      ILX->xsynRe[2] = .3*lfNoiseMean+.7*ILX->xsynRe[2];
    
      noiseBW = (int)(ILX->fftLen/ILX->fSample*ILX->maxNoiseFreq+.5);
      NoiseDct(ILX->xsynRe,ILX->noisePara,min(ILX->fftLen/2,noiseBW),
	       ILX->maxNumNoisePara,noiseBW);
    
      for(i=0;i<ILX->maxNumNoisePara;i++)
	ILX->noisePara[i] *= 512.0/ILX->fftLen;
      /* HP 980216   compensate /=fftLen in ODFT (256 sample/frame)*/

    }
    else {
      /* LPC noise */
      register int	i;
      register double	p;

      /* HP20020226 */
      WinOdft(&ILX->xsyn[constOffsC],win,ILX->winLen,ILX->fftLen,
	      ILX->xsynRe,ILX->xsynIm);

      p=0.0;
      for(i=0;i<ILX->fftLen;i++)
	{
	  register double a = sqrt(ILX->xOrgRe[i]*ILX->xOrgRe[i]+
				  ILX->xOrgIm[i]*ILX->xOrgIm[i]);
	  register double b = sqrt(ILX->xsynRe[i]*ILX->xsynRe[i]+
				  ILX->xsynIm[i]*ILX->xsynIm[i]);
	  register double e;

	  if ( a>b ) { e=a-b; p+=((double) e)*((double) e); }

	  /* HP 991014 */
	  /* e=a-0.85*b; if ( e<0.0 ) e=0.0; ILX->xsynRe[i] = e; */
	  e=a-b; if ( e<0.0 ) e=0.0; ILX->xsynRe[i] = e;
	}

      p/=((double) ILX->fftLen) * ((double) (winNorm*noiseEnvPwrFact));

      lpc_noise(ILX->noisePara+1,ILX->maxNumNoisePara-1,
		ILX->xsynRe,ILX->xsynIm,ILX->fftLen);

      ILX->noisePara[0]=sqrt(p);	/* first parameter is the amplitude */
    }

    *numNoisePara = ILX->maxNumNoisePara;
    *noiseRate = qx/max(qxOri,1.);
    *noiseFreq = ILX->maxNoiseFreq;
    
    if (ILX->debugLevel) {
      printf("qxOri=%e, qx=%e, noiseRate=%4.2f\n",qxOri,qx,*noiseRate);
      /*     printf("NoisePara="); */
      /*     for(i=0;i<ILX->maxNoise;i++) */
      /*       printf(" %6.1f",ILX->noisePara[i]); */
      /*     printf("\n"); */
    }
  }

  /* set pointers to arrays with return values */

  *envPara = ILX->envPara;
  *lineFreq = ILX->lineFreq;
  *lineAmpl = ILX->lineAmpl;
  *linePhase = ILX->linePhase;
  *lineEnv = ILX->lineEnv;
  *linePred = ILX->linePred;
  *numHarmLine = ILX->numHarmLine;
  *harmFreq = ILX->harmFreq;
  *harmFreqStretch = ILX->harmFreqStretch;
  *harmLineIdx = ILX->harmLineIdx;
  *harmEnv = ILX->harmEnv;
  *harmPred = ILX->harmPred;
  *harmRate = ILX->harmRate;
  *noisePara = ILX->noisePara;

  /* copy parameters into frame-to-frame memory */

  ILX->prevNumLine = *numLineExtract;
  for (j=0; j<*numEnv; j++)
    for (i=0; i<ENVPARANUM; i++)
      ILX->prevEnvPara[j][i] = ILX->envPara[j][i];
  for (iline=0; iline<*numLineExtract; iline++) {
    ILX->prevLineFreq[iline] = ILX->lineFreq[iline];
    ILX->prevLineAmpl[iline] = ILX->lineAmpl[iline];
    ILX->prevLinePhase[iline] = ILX->linePhase[iline];
    ILX->prevLineEnv[iline] = ILX->lineEnv[iline];
  }
  ILX->prevNumHarm = *numHarm;
  for (i=0; i<*numHarm; i++) {
    ILX->prevNumHarmLine[i] = ILX->numHarmLine[i];
    ILX->prevHarmLineIdx[i] = ILX->harmLineIdx[i];
    ILX->prevHarmFreq[i] = ILX->harmFreq[i];
    ILX->prevHarmFreqStretch[i] = ILX->harmFreqStretch[i];
    ILX->prevHarmEnv[i] = ILX->harmEnv[i];
    for (iline=ILX->harmLineIdx[i];
	 iline<ILX->harmLineIdx[i]+ILX->numHarmLine[i]; iline++) {
      ILX->prevLineFreq[iline] = ILX->lineFreq[iline];
      ILX->prevLineAmpl[iline] = ILX->lineAmpl[iline];
      ILX->prevLinePhase[iline] = ILX->linePhase[iline];
      ILX->prevLineEnv[iline] = ILX->lineEnv[iline];
    }
  }
  

  if (ILX->debugLevel)
    for (i=0;i<*numLineExtract;i++)
      printf("%2d: f=%7.1f (df=%7.1f) a=%7.1f p=%5.2f e=%1d c=%2d\n",
	     i,ILX->lineFreq[i],ILX->lineDf[i],ILX->lineAmpl[i],
	     ILX->linePhase[i],ILX->lineEnv[i],ILX->linePred[i]);

}
  

/* IndiLineExtractFree() */
/* Free memory allocated by IndiLineExtractInit(). */

void IndiLineExtractFree (
  ILXstatus *ILX)		/* in: ILX status handle */
{
  free(ILX->costab);
  free(ILX->longWin);
  free(ILX->envPara[0]);
  free(ILX->envPara);
  free(ILX->lineFreq);
  free(ILX->lineAmpl);
  free(ILX->linePhase);
  free(ILX->lineEnv);
  free(ILX->linePred);
  free(ILX->numHarmLine);
  free(ILX->harmFreq);
  free(ILX->harmFreqStretch);
  free(ILX->harmLineIdx);
  free(ILX->harmEnv);
  free(ILX->harmPred);
  free(ILX->harmRate);
  free(ILX->noisePara);
  free(ILX->cos);
  free(ILX->sin);
  free(ILX->envWin);
  free(ILX->x);
  free(ILX->xOrgRe);
  free(ILX->xOrgIm);
  free(ILX->xmodRe);
  free(ILX->xmodIm);
  free(ILX->xsyn);
  free(ILX->xsynRe);
  free(ILX->xsynIm);
  free(ILX->lineDf);
  free(ILX->lflag);
  free(ILX->prevLineCont);
  free(ILX->harmCosFact);
  free(ILX->harmSinFact);
  free(ILX->harmFc);
  free(ILX->harmFe);
  free(ILX->harmPsy);
  free(ILX->prevEnvPara[0]);
  free(ILX->prevEnvPara);
  free(ILX->prevLineFreq);
  free(ILX->prevLineAmpl);
  free(ILX->prevLinePhase);
  free(ILX->prevLineEnv);
  free(ILX->prevNumHarmLine);
  free(ILX->prevHarmFreq);
  free(ILX->prevHarmFreqStretch);
  free(ILX->prevHarmLineIdx);
  free(ILX->prevHarmEnv);

  ILXEnvFree(ILX->envStatus);
  ILXLineFree(ILX->lineStatus);
  ILXHarmFree(ILX->harmStatus);

  free(ILX);
}

/* end of indilinextr.c */
