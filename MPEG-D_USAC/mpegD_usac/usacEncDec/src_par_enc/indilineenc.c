/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - HILN: harmonic/individual lines and noise
                              bitstream encoder



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



Source file: indilineenc.c


Required libraries:
(none)

Required modules:
bitstream.o		bit stream module
indilinecom.o		indiline common module
indilineqnt.o		indiline quantiser module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
25-sep-96   HP    first version based on enc_par.c
26-sep-96   HP    new indiline module interfaces
19-nov-96   HP    adapted to new bitstream module
11-apr-97   HP    harmonic stuff ...
16-apr-97   BE    coding of harmonic stuff ...
22-apr-97   HP    noisy stuff ...
22-apr-97   BE    noise para ...
20-jun-97   HP    improved env/harm/noise interface
09-jul-97   HP    improved harmMaxNumBit, noiseMaxNumBit
16-nov-97   HP    harm/indi cont mode
16-feb-98   HP    improved noiseFreq
09-apr-98   HP    ILEconfig
18-jun-98   HP    fixed TransfHarm / proposed noise quant
24-feb-99   NM/HP LPC noise, bsf=4
30-jan-01   BE    float -> double
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bitstream.h"		/* bit stream module */
#include "common_m4a.h"

#include "indilinecom.h"	/* indiline common module */
#include "indilineqnt.h"	/* indiline quantiser module */
#include "indilineenc.h"	/* indiline bitstream encoder module */


/* units & formats: */
/*  Freq: frequency in Hz */
/*  Ampl: peak amplitude of sine wave */
/*  Phase: phase in rad at frame center */
/*  Index: NumBit LSBs, to be transmitted MSB first */
/*  Pred: line predecessor index: 0=no pred., 1=line[0], 2=line[1], ... */

/* ---------- globals --------------- */

extern Tlar_enc_par	lar_enc_par[3];
extern Tlar_enc_par	hlar_enc_par[8];
extern TLPCEncPara	LPCnoiseEncPar;
extern TLPCEncPara	LPCharmEncPar;
extern int		sdcILFTab[8][32];
extern int		sdcILATab[32];
extern int		numHarmLineTab[32];
extern int		sampleRateTable[16];
extern int		maxFIndexTable[16];


/* ---------- declarations ---------- */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define HARM_PHASE_NUMBIT 5
#define HARM_PHASE_NUMSTEP 32
#define HARM_PHASE_RANGE (2.*M_PI)
#define HARM_MAX_FBASE 4		/* max base for freq enh bit num */
#define HFREQRELSTEP (exp(log(4000./30.)/2048.))
#define HFREQRELSTEP4 (exp(log(4000./20.)/2048.))

#define HARMPHASEBITS 6		/* bsf=5 */

/* ---------- declarations (data structures) ---------- */

/* status variables and arrays for individual line encoder (ILE) */

static int freqThrE[HARM_MAX_FBASE] = {1177, 1467, 1757, 2047};
static int freqThrEBsf4[HARM_MAX_FBASE] = {1243, 1511, 1779, 2047};
static int fEnhaBitsHarm[10] = {0,1,2,2,3,3,3,3,4,4};

struct ILEstatusStruct {
  /* general parameters */
  ILQstatus *ILQ;
  ILEconfig *cfg;
  int debugLevel;
  int maxNumLine;
  int maxNumLineBS;
  int maxNumEnv;
  int maxNumHarm;
  int maxNumHarmLine;
  int maxNumNoisePara;
  int maxNumAllLine;
  int numLineNumBit;
  int quantMode;
  int contMode;
  int enhaFlag;
  int enhaQuantMode;
  int sampleRateCode;
  int maxFIndex;

  int envNumBit;
  int newLineNumBit;
  int contLineNumBit;
  double fSample;
  int noiseAmplCW;


  /* pointers to variable content arrays */
  int *lineParaSFI;
  int *lineParaEnv;
  int *lineContFlag;
  int *linePredEnc;
  int *harmPredEnc;
  double	*transAmpl;
  unsigned int	*harmFreqIndex;
  unsigned int	*harmFreqStretchIndex;
  unsigned int	**harmAmplIndex;
  long		*harmFreqCW;
  long		*harmAmplCW;
  int		*harmFreqStretchQuant;
  double	*harmLPCAmpl;
  double	*harmLPCAmplTmp;
  double	*harmLPCPara;
  int		**harmLPCIndex;
  double	**harmLPCDeqOld;
  double	*harmLPCNonLin;
  
  unsigned int	*noiseParaIndex;
  int		*noiseLPCIndex;
  double	*noiseLPCNonLin;
  double	*noiseLPCDeqOld;
  unsigned int	*harmEnhaIndex;
  unsigned int	*harmEnhaNumBit;

  /* variables for frame-to-frame memory */
  int prevQuantNumLine;
  int prevNumTrans;
  int prevNumNoise;
  int layPrevNumLine[MAX_LAYER];

};


/* ---------- internal functions ---------- */


static void LPCCompressK(double *y,double *x,long n,TLPCEncPara *ep)
{
	register Tlar_enc_par	*hcp;
	register long		i;
	register double		h;

	hcp=ep->hcp;
	for (i=0; i<n; i++)
	{
		h=(double) x[i];
		h=log((1.0+h)/(1.0-h));		/* -1.0 < h < 1.0 must be guaranteed	*/
		y[i]=(double) h;

		if ( i<ep->nhcp-1 ) hcp++;
	}
}

static void ResetLPCPara(double *x,int s,int n,TLPCEncPara *ep)
{
	register int	i,m;

	n+=s;
	m=n;
	if ( m>2 ) m=2;
  	for (i=s; i<m; i++) x[i]=ep->hcp[i].mean;
  	for (   ; i<n; i++) x[i]=0.0;
}

static void ScaleLPCPredMean(double *x,int n,TLPCEncPara *ep)
{
	register Tlar_enc_par	*hcp;
	register int		i;

	hcp=ep->hcp;
	if ( n>ep->nhcp )
	{
		for (i=0; i<ep->nhcp-1; i++)
		{
			x[i]=hcp->corr*(x[i]-hcp->mean)+hcp->mean;
			hcp++;
		}
		for (; i<n; i++)
		{
			x[i]=hcp->corr*(x[i]-hcp->mean)+hcp->mean;
		}
	}
	else
	{
		for (i=0; i<n; i++)
		{
			x[i]=hcp->corr*(x[i]-hcp->mean)+hcp->mean;
			hcp++;
		}
	}
}

/* function QuantizeLPCSet */
/* ----------------------- */
/* quantizes an LPC parameter set according to the information in ep	*/

static long QuantizeLPCSet(	int		*q,	/* out   : quantized values (corresponding to x)			*/
				double		*x,	/* in    : LPC parameters						*/
				double		*y,	/* in/out: dequantized parameters (in:old, out:new)			*/
				int		*nip,	/* in/out: (max) number of parameters (index)				*/
				TLPCEncPara	*ep,	/* in    : Encoder parameters						*/
				int	maxbits)	/* in    : Max. number of bits to use					*/
							/* return: number of bits used						*/
{
#define MAX_LAP_CWL (15)

	register Tlar_enc_par	*hcp;
	register int		i,j,k,ni,b,bits,l;
	register double		h;

	ni=*nip;
	if ( (maxbits<2) || (ni<0) ) { *nip=-1; return 0; }

	bits=0;

	for (k=l=0; k<=ni; k++)
	{
		b=0;

		for (i=l; i<ep->numParaTable[k]; i++)
		{
			if ( i<ep->nhcp ) hcp=ep->hcp+i; else hcp=ep->hcp+ep->nhcp-1;

			h=(double) (x[i]-y[i]);		/* prediction			*/
			h/=hcp->qstep;

			if ( i<ep->zstart )
			{
				h*=(double) (1<<hcp->xbits);
				j=(long) h;
				if ( h<0.0 ) j--;

/* fprintf(stderr,"lpcidx[%2d]=%3d ",i,j); */
if(j>((MAX_LAP_CWL+1)<<hcp->xbits)-1||j<-((MAX_LAP_CWL+1)<<hcp->xbits))
	fprintf(stderr,"Alpcidx[%2d]=%3d CLIPPED (max. +/-%i)\n",i,j,((MAX_LAP_CWL+1)<<hcp->xbits)-1);

				if (j < -((MAX_LAP_CWL+1)<<hcp->xbits))
				  j = -((MAX_LAP_CWL+1)<<hcp->xbits);
				if (j > ((MAX_LAP_CWL+1)<<hcp->xbits)-1)
				  j = ((MAX_LAP_CWL+1)<<hcp->xbits)-1;

				q[i]=j;
				b+=hcp->xbits;
				j>>=hcp->xbits;
				if ( j<0 ) b+=1-j; else b+=2+j;
			}
			else
			{
				if ( h<0.0 ) j=(long) (h-0.5); else j=(long) (h+0.5);

/* fprintf(stderr,"lpcidx[%2d]=%3d ",i,j); */
if(j>MAX_LAP_CWL||j<-MAX_LAP_CWL)fprintf(stderr,"Blpcidx[%2d]=%3d CLIPPED (max. +/-%i)\n",i,j,MAX_LAP_CWL);

				if ( j<-MAX_LAP_CWL ) j=-MAX_LAP_CWL;
				if ( j> MAX_LAP_CWL ) j= MAX_LAP_CWL;

				q[i]=j;
				/* h=(double) j; */
				b+=1;
				if ( j<0 ) b+=1-j;
				if ( j>0 ) b+=1+j;
			}

/* fprintf(stderr,"h=%7.3f j=%2d b=%2d xbits=%1d %s\n",h,j,b,hcp->xbits,(i<ep->zstart)?"nozero":"zero"); */

		}

/* fprintf(stderr,"k=%d b=%d bits=%d max=%d\n",k,b,bits,maxbits); */

		if ( bits+b>maxbits ) { *nip=k-1; return bits; }

		bits+=b;

		for (i=l; i<ep->numParaTable[k]; i++)
		{
			if ( i<ep->nhcp ) hcp=ep->hcp+i; else hcp=ep->hcp+ep->nhcp-1;

			j=q[i];

			h=(double) j;
			if ( i<ep->zstart )
			{
				if ( j<0 ) h+=1.0-hcp->rpos; else h+=hcp->rpos;
				if ( hcp->xbits>0 ) h/=(double) (1<<hcp->xbits);
			}
			else
			{
				if ( j<0 ) h+=0.5-hcp->rpos;
				if ( j>0 ) h-=0.5-hcp->rpos;
			}
			h*=hcp->qstep;
			y[i]+=h;
		}
		l=i;
	}
	return bits;
}

/* function PutLPCParaBits */
/* ----------------------- */
/* Puts the quantized LPC parameters set on the bitstream	*/

static void PutLPCParaBits(	HANDLE_BSBITSTREAM stream,	/* the stream			*/
				int *q,			/* in: quantized parameters	*/
				int ni,			/* in: number of param. (index)	*/
				TLPCEncPara	*ep)	/* in: Encoder parameters	*/
{
	register char		*t = "PutLPCParaBits: error generating bit stream (LPC)";
	register Tlar_enc_par	*hcp;
	register int		i,j,l,vz;

	for (i=0; i<ep->numParaTable[ni]; i++)
	{
		if ( i<ep->nhcp ) hcp=ep->hcp+i; else hcp=ep->hcp+ep->nhcp-1;

		j=q[i];

		if ( i>=ep->zstart )
		{
			if ( j==0 )
			{
				if ( BsPutBit(stream,0,1) ) IndiLineExit(t);
				continue;
			}
			if ( BsPutBit(stream,1,1) ) IndiLineExit(t);
			l=0;
		}
		else
		{
			l=j & ((1<<hcp->xbits)-1);
			j>>=hcp->xbits;
			if ( j>=0 ) j++;
		}
		vz=0;
		if ( j<0 ) { vz=1; j=-j; }
		if ( BsPutBit(stream,(unsigned int) vz,1) ) IndiLineExit(t);
		for (; j>1; j--) if ( BsPutBit(stream,0,1) ) IndiLineExit(t);
		if ( BsPutBit(stream,1,1) ) IndiLineExit(t);

		if ( hcp->xbits>0 )
		{
			if ( BsPutBit(stream,(unsigned int) l,hcp->xbits) ) IndiLineExit(t);
		}
	}
}


/*$ BE991125>>> $*/

/* function HilnBsErr */
/* ------------------ */
/* exits with an error message specifying syntax element (elName) */
/* and error sensitivity class (esc)*/
static void HilnBsErr(char *elName, int esc)
{
  IndiLineExit("HILN: error generating bit stream (%s) for ESC %d",
	       elName,esc);
}

/* function PutLPCParaFmt6 */
/* ----------------------- */
/* Puts the quantized LPC parameters set on the bitstream	*/
/* (only with index>=si and index<=ei) */
/* returns: number of bits written to bit stream */
static int PutLPCParaFmt6(HANDLE_BSBITSTREAM stream,	/* the stream */
			   int *q,		/* in: quantized parameters */
			   int ni,		/* in: tot. num. param. */
			   int si,		/* in: start index */
			   int ei,		/* in: end index (0=no lim.) */
			   int esc,		/* in: error sensit. class */
			   char *elName,	/* in: bs elem. name */
			   TLPCEncPara	*ep)	/* in: Encoder parameters */
{
  register Tlar_enc_par	*hcp;
  register int		i,j,l,vz;
  int numBits = 0;

  if(ei==0) ei = ep->numParaTable[ni]-1;
  if(ei>=ep->numParaTable[ni]) ei = ep->numParaTable[ni]-1;

  for (i=si; i<=ei; i++) {
    if (i<ep->nhcp) hcp=ep->hcp+i; else hcp=ep->hcp+ep->nhcp-1;

    j=q[i];

    if (i>=ep->zstart) {
      if (j==0) {
	if (BsPutBit(stream,0,1)) HilnBsErr(elName,esc);
	numBits += 1;
	continue;
      }
      if (BsPutBit(stream,1,1)) HilnBsErr(elName,esc);
      numBits += 1;
      l=0;
    }
    else {
      l=j & ((1<<hcp->xbits)-1);
      j>>=hcp->xbits;
      if (j>=0) j++;
    }
    vz=0;
    if (j<0) { vz=1; j=-j; }
    if (BsPutBit(stream,(unsigned int) vz,1)) HilnBsErr(elName,esc);
    numBits += 1;
    for (; j>1; j--) {
      if (BsPutBit(stream,0,1)) HilnBsErr(elName,esc);
      numBits += 1;
    }
    if (BsPutBit(stream,1,1)) HilnBsErr(elName,esc);
    numBits += 1;
    if (hcp->xbits>0) {
      if (BsPutBit(stream,(unsigned int) l,hcp->xbits))
	HilnBsErr(elName,esc);
      numBits += hcp->xbits;
    }
  }
  return(numBits);
}

/* function PutHStretchFmt6 */
/* ------------------------ */
/* writes freq. stretching parameter to bit stream */
/* returns: number of bits written to bit stream */
static int PutHStretchFmt6(HANDLE_BSBITSTREAM stream,long s,int esc)
{
  register long	vz;
  int numBits = 0;

  if(s==0) {
    if(BsPutBit(stream,0,1)) HilnBsErr("harmFreqStretch",esc);
    numBits += 1;
    return(numBits);
  }
  vz=0;
  if(s<0) { vz=1; s=-s; }

  if(BsPutBit(stream,1,1)) HilnBsErr("harmFreqStretch",esc);
  numBits += 1;
  if(BsPutBit(stream,vz,1)) HilnBsErr("harmFreqStretch",esc);
  numBits += 1;

  if(s==1) {
    if(BsPutBit(stream,0,1)) HilnBsErr("harmFreqStretch",esc);
    numBits += 1;
    return(numBits);
  }
  if(BsPutBit(stream,1,1)) HilnBsErr("harmFreqStretch",esc);
  numBits += 1;

  if(BsPutBit(stream,s-2,4)) HilnBsErr("harmFreqStretch",esc);
  numBits += 4;

  return(numBits);
}
/*$ <<<BE991125 $*/




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

static double tp_interp(double *x,long p,long n)
{
	static double	k[8] = { 0.614303112, -0.153575778, 0.051191926, -0.014626265,
				 0.003102541, -0.000423074, 0.000027537,  0.000000000 };
	register double	s;
	register long	i,l,r;

	s=0.0;
	l=p;
	r=p+1;

	for (i=0; i<7; i++)
	{
		if ( (l>=0) && (l<n) ) s+=(double) (k[i]*x[l]);
		if ( (r>=0) && (r<n) ) s+=(double) (k[i]*x[r]);
		l--;
		r++;
	}
	return s;
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

static double LPC_lar2k(double lar)
{
	lar=exp(lar);
	return (lar-1.0)/(lar+1.0);
}

static double LPC_k2lar(double k)
{
	return log((1.0+k)/(1.0-k));
}

static double LPCharmSP(double *k,double *x,long n,long m)
{
	double		h[LPCMaxNumPara];
	register double	t,omega,omega0,aa,ax,xx;
	register long	i;

	memcpy(h,k,n*sizeof(double));

	LPCconvert_k_to_h(h,n);

	omega0=M_PI/((double) (m+1));

	aa=ax=xx=0.0;

	for (i=0,omega=omega0; i<m; i++,omega+=omega0)
	{
		t=PWRSpec(h,n,omega);
		if ( t>1.0/(lpc_max_amp*lpc_max_amp) ) t=1.0/t; else t=lpc_max_amp*lpc_max_amp;
		t=sqrt(t);
		aa+=t*t;
		ax+=t*((double) x[i]);
		xx+=(double) (x[i]*x[i]);		
	}

	return (ax*ax)/(aa*xx);
}

static double LPCharmIterate(double *k,double *x,long n,long m,double w)
{
	register long	i;
	register double	k0,l0,ka,kb,q,qa,qb;
        register double qs = 0;

	q=LPCharmSP(k,x,n,m);

/*	fprintf(stderr,"%f: vorher: %f, ",w,q);	*/

	for (i=0; i<n; i++)
	{
		if ( i<LPCharmEncPar.nhcp ) qs=w*LPCharmEncPar.hcp[i].qstep/((double) (1<<LPCharmEncPar.hcp[i].xbits));
		k0=k[i];
		l0=LPC_k2lar(k0);
		k[i]=ka=LPC_lar2k(l0-qs); qa=LPCharmSP(k,x,n,m);
		k[i]=kb=LPC_lar2k(l0+qs); qb=LPCharmSP(k,x,n,m);
		if ( (qa>q) && (qa>qb) )
		{
			q=qa; k[i]=ka;
		}
		else if ( (qb>q) && (qb>qa) )
		{
			q=qb; k[i]=kb;
		}
		else
		{
			k[i]=k0;
		}
	}

/*	fprintf(stderr,"nachher: %f\n",q); */

	return q;
}

static void LPCharmGetPK(double *k,int *nip, double *x,int m)
{
	double		akf[LPCMaxNumPara];
	double		h[LPCMaxNumPara];
	register int	i,j,n;
	register double	ps,a,t,dt,omega0,q;

	n = LPCharmEncPar.numParaTable[*nip]+1;

	for (i=0; i<n; i++) akf[i]=0.0;

	/* omega0=0.5*M_PI/((double) (m)); */
	omega0=0.5*M_PI/((double) (m+1));

	dt=omega0;
	ps=0.0;
	for (i=1; i<2*m+2; i++)
	{
		j=(i>>1)-1;
		if ( i&1 )
		{
			a=tp_interp(x,j,m);
			a*=a;
		}
		else
		{
			a=(double) x[j];
			a*=a;
			ps+=a;
		}
		t=0.0;
		for (j=0; j<n; j++) { akf[j]+=(double) a*cos(t); t+=dt; }
		dt+=omega0;
	}

	if ( akf[0]<=0.0 )
	{
		fprintf(stderr,"LPCharmGetPK: Power=%f, m=%i\n",akf[0],m);
	}


	k[0]=sqrt(0.5*ps);

#define NO_HLPC_ITER

	for (j=0; j<(*nip); j++)
	{
		n = LPCharmEncPar.numParaTable[*nip-j]+1;
		durbin_akf_to_kh(k+1,h,akf,n-1);

#ifdef NO_HLPC_ITER
		break;
#endif
		for (i=0; i<8; i++) q=LPCharmIterate(k+1,x,n-1,m,0.5*pow(0.8,(double) i));
		if ( q<0.995 ) break;
	}
	if ( j>0 )
	{
		j--;
		(*nip)-=j;
		n = LPCharmEncPar.numParaTable[*nip]+1;
		durbin_akf_to_kh(k+1,h,akf,n-1);
		for (i=0; i<8; i++) q=LPCharmIterate(k+1,x,n-1,m,0.5*pow(0.8,(double) i));
	}

/*
	t=LPCharmEncPar.hcp[0].qstep/((double) (1<<LPCharmEncPar.hcp[0].xbits));
	k[1]=LPC_lar2k(LPC_k2lar(k[1])+t);
*/

}



#ifdef NOT_UPDATED
static void LPCharmGetPKrec (double *k, long n, double *x, long m, double *a, double *at)
/* recursive ...   HP 990302 */
{
	double		h[LPCMaxNumPara];
	double		kold[LPCMaxNumPara];
	register double	ps,omega,omega0;
	register double	pf;
	register long	i,j;
	register double dbold;
	register double db;
	register double dbs;

	/* omega0=M_PI/((double) (m)); */
	omega0=M_PI/((double) (m+1));

      	for (i=0; i<m; i++)
	  at[i] = x[i];
	dbs = -1.;
	j = 0;

      do {
	dbold = dbs;
	j++;

	if (dbs>=0)
	  for (i=0; i<n; i++) kold[i]=k[i];

	LPCharmGetPK(k,n,at,m);

	for (i=0; i<n-1; i++) h[i]=k[i+1];

	LPCconvert_k_to_h(h,n-1);

	ps=0.0;
	for (i=0,omega=omega0; i<m; omega+=omega0,i++)
	{
		register double	ha = PWRSpec(h,n-1,omega);

		if ( ha>1.0/(lpc_max_amp*lpc_max_amp) ) ha=1.0/ha; else ha=lpc_max_amp*lpc_max_amp;

		a[i]=(double) sqrt(ha);

		ps+=ha;
	}

	pf=(double) sqrt(((double) (k[0]*k[0]))/(0.5*ps));

	for (i=0; i<m; i++) a[i]*=pf;

/*	fprintf(stderr,"try %d:  numharm=%d  order=%d\n",j,m,n-1);	*/
	dbs=0.0;
	for (i=0; i<m; i++) {
	  db = log(a[i]/x[i])/log(10.)*20;
/*	  fprintf(stderr,"%2d ori=%10.3f in=%10.3f dec=%10.3f %6.2fdB\n",i,x[i],at[i],a[i],db);	*/
	  dbs += max(db,-db);
	}
/*	fprintf(stderr,"dbs=%7.3fdB\n",dbs);	*/

      	for (i=0; i<m; i++)
	  at[i] *= sqrt(x[i]/a[i]);

#ifndef HARM_LPC_NO_REC
      } while (dbold < 0 || (dbs < dbold*0.95 && dbs > 0.5));

      if (dbs>dbold)
	  for (i=0; i<n; i++) k[i]=kold[i];
#else
      } while (0);
#endif

/*	for(i=0;i<n;i++)fprintf(stderr,"k%2d = %10.5f\n",i-1,k[i]);	*/


/* printf("NIKO: %7.3f\n",log((1.0+k[1])/(1.0-k[1]))); */

}
#endif

static long QuantizeHStretch(int *q,double x)
{
	register long	i;

	x*=(double) (1.0/HSTRETCHQ);

	if ( x<0.0 )
	{
		i=(long) (x-0.5);
		if ( i<-HSTRETCHMAX ) i=-HSTRETCHMAX;
		*q=i;
		i=-i;
	}
	else
	{
		i=(long) (x+0.5);
		if ( i>HSTRETCHMAX ) i=HSTRETCHMAX;
		*q=i;
	}

	switch ( i ) {
		case  0 : return 1;
		case  1 : return 3;
		default : return 7;
	}
}

static long PutHStretchBits(HANDLE_BSBITSTREAM stream,long s)
{
	register long	vz;

	if ( s==0 ) return BsPutBit(stream,0,1);

	vz=0;
	if ( s<0 ) { vz=1; s=-s; }

	if ( BsPutBit(stream,1,1) ) return -1;
	if ( BsPutBit(stream,vz,1) ) return -1;

	if ( s==1 ) return BsPutBit(stream,0,1);

	if ( BsPutBit(stream,1,1) ) return -1;

	return BsPutBit(stream,s-2,4);
}

static int InsertSorted(int *y,int n,int x)
{
	if ( n==0 )
	{
		y[0]=x;
		return 0;
	}
	if ( y[0] >= x )
	{
		memmove(y+1,y,n*sizeof(int));
		y[0]=x;
		return 0;
	}
	while ( y[n-1]>x )
	{
		y[n]=y[n-1];
		n--;
	}
	y[n]=x;
	return n;
}

static long FContEncode(int *px)
{
	register int	x,v;

	x=*px;
	if ( x<-42 ) { *px=x=-42; }
	if ( x> 42 ) { *px=x= 42; }

	if ( x==0 ) return 2;
	v=0;
	if ( x<0 ) { v=1; x=-x; }

	if ( x== 1 ) return ((  2+ v      )<<8)| 3;
	if ( x<= 3 ) return ((  6+(v<<1)+x)<<8)| 4;
	if ( x<= 4 ) return (( 24+ v      )<<8)| 5;
	if ( x<= 6 ) return (( 47+(v<<1)+x)<<8)| 6;
	if ( x<=10 ) return ((105+(v<<2)+x)<<8)| 7;
	return ((949+(v<<5)+x)<<8)|10;
}

static long FHContEncode(int *px)
{
	register int	x,v;

	x=*px;
	if ( x<-69 ) { *px=x=-69; }
	if ( x> 69 ) { *px=x= 69; }

	if ( x==0 ) return 2;

	v=0;
	if ( x<0 ) { v=1; x=-x; }

	if ( x== 1 ) return ((  2+ v      )<<8)| 3;
	if ( x<= 5 ) return (( 14+(v<<2)+x)<<8)| 5;
		     return ((378+(v<<6)+x)<<8)| 9;
}

static long AContEncode(int *px)
{
	register int	v,x;

	x=*px;
	if ( x<-25 ) { *px=x=-25; }
	if ( x> 25 ) { *px=x= 25; }

	if ( x==0 ) return 3;
	v=0;
	if ( x<0 ) { v=1; x=-x; }
	if ( x== 1 ) return (  (1+ v      )<<8)|3;
	if ( x== 2 ) return (  (6+ v      )<<8)|4;
	if ( x== 3 ) return (  (8+ v      )<<8)|4;
	if ( x<= 5 ) return (( 16+(v<<1)+x)<<8)|5;
	if ( x<= 9 ) return (( 42+(v<<2)+x)<<8)|6;
		     return ((214+(v<<4)+x)<<8)|8;
}

static long SDCEncode(int k,int i,int *tab)
{
	register int	*pp;
	register int	g,dp;
	register int	min,max;
	register int	cwl;
	register long	cw;

	cw=cwl=0;
	min=0;
	max=k-1;

	pp=tab+(1<<(5-1));
	dp=1<<(5-1);

	while ( min!=max )
	{
		if ( dp ) g=(k*(*pp))>>10; else g=(max+min)>>1;
		dp>>=1;
		cw<<=1;
		cwl++;
		if ( i<=g ) { pp-=dp; max=g; } else { cw|=1; pp+=dp; min=g+1; }
	}
	return (cw<<8)+cwl;
}

static long FNewEncode(int n,int k,int i)
{
	n--;
	if ( n>7 ) n=7;

	return SDCEncode(k,i,sdcILFTab[n]);
}

static long ALNewEncode(int *px,int max,int fine)
{
	register int	x = (*px) - max;

	if ( x<0 ) IndiLineExit("ALNewEncode: max too big\n");

	if ( fine )
	{
		if ( x>=MAXAMPLDIFF ) x=MAXAMPLDIFF-1;
		*px = max+x;
		return SDCEncode(MAXAMPLDIFF,x,sdcILATab);
	}
	else
	{
		x=(x+1)&(-2);
		if ( x>=(MAXAMPLDIFF&(-2)) ) x=(MAXAMPLDIFF&(-2))-2;
		*px = max+x;
		return SDCEncode(MAXAMPLDIFF>>1,x>>1,sdcILATab);
	}
}

static long ANNewEncode(int *px,int max)
{
	register int	x = (*px) - max;

	if ( x< 0 ) x= 0;
	if ( x>63 ) x=63;

	*px = max+x;
	return (((long) x)<<8)+6L;
}

static long AHNewEncode(int *px,int max)
{
	register int	x = (*px) - max;

	if ( x< 0 ) x= 0;
	if ( x>63 ) x=63;
	*px = max+x;

	return (((long) x)<<8)+6L;
}

static int BSearch(int *x,int n,int s)
{
	int	l,m;

	if ( s>=x[n-1] ) return n-1;
	if ( s< x[  0] ) return -1;
	l=0;
	do
	{
		m=(l+n)>>1;
		if ( s<x[m] ) n=m; else l=m;
	}
	while ( n-l>1 );

	return l;
}

static int calcSRC(int *tab,int n,double fs)
{
	register int	i,m;
	register double	d,h;

	d=fs-((double) tab[0]);
	if ( d<0.0 ) d=-d;
	m=0;
	for (i=1; i<n; i++)
	{
		h=fs-((double) tab[i]);
		if ( h<0.0 ) h=-h;
		if ( h<d ) { m=i; d=h; }
	}
	return m;
}


/* GenILEconfig() */
/* Generate default ILEconfig. */

static ILEconfig *GenILEconfig ()
{
  ILEconfig *cfg;

  if ((cfg = (ILEconfig*)malloc(sizeof(ILEconfig)))==NULL)
    IndiLineExit("GenILEconfig: memory allocation error");
  cfg->quantMode = -1;
  cfg->enhaQuantMode = -1;
  cfg->contMode = 0;
  cfg->encContMode = 0;
  cfg->bsFormat = 4;
  cfg->phaseFlag = 0;
  cfg->maxPhaseIndi = 0;
  cfg->maxPhaseExt = 0;
  cfg->maxPhaseHarm = 0;
  return cfg;
}


/* SmoothHarmAmpl() */
/* Smoothing of harm. amplitudes by frequency dependent triangular filter */

static void SmoothHarmAmpl (
  double f0,			/* in:  fundamental freq. */
  int numHarm,			/* in:  number of harmonics */
  double *harmAmpl,		/* in:  harm. amplitudes */
  double *smoothAmpl)		/* out: smoothed ampl. */
{
  int i,j,il,ih;
  double f,df,fl,fh,fi,coeff,sumCoeff;

  for(j=0;j<numHarm;j++) {
    f = (double)j*f0;
    df = 20.+f/5.;
    fh = min(f+df/2.,(double)numHarm*f0);
    fl = max(fh-df,f0);
    il = (int)(fl/f0+.999);
    ih = (int)(fh/f0+.001);
    sumCoeff = 1;
    smoothAmpl[j] = harmAmpl[j];
    for(i=il;i<j;i++) {
      fi = (double)i*f0;
      coeff = (fi-fl)/(f-fl);
      smoothAmpl[j] += coeff*harmAmpl[i];
      sumCoeff += coeff;
    }
    for(i=j+1;i<=ih;i++) {
      fi = (double)i*f0;
      coeff = (fh-fi)/(fh-f);
      smoothAmpl[j] += coeff*harmAmpl[i];
      sumCoeff += coeff;
    }
    smoothAmpl[j] /= sumCoeff;
  }
}


/* TransfHarmAmpl() */
/* Transform  harm. amplitudes by freq. dependent averaging */

static int TransfHarmAmpl (
  int numAmpl,
  double *Ampl,
  double *transAmpl)
{
#define MAXGRP 8

  int i,j,igrp,il,numTrans;
  int numGrp[MAXGRP] = {10,6,5,4,3,2,1,33};	/* 10 groups of 1 line ... */
  double sumAmpl;

  il = igrp = i = numTrans = 0;
  while(il<numAmpl) {
    sumAmpl = 0;
    for(j=il;j<min(il+igrp+1,numAmpl);j++)
      sumAmpl += Ampl[j];
    transAmpl[numTrans++] = sumAmpl/sqrt((double)(igrp+1));
    il = il+igrp+1;
    i++;
    if(i>=numGrp[igrp]) {
      i = 0;
      igrp++;
      if (igrp>=MAXGRP)
	IndiLineExit("TransfHarmAmpl: numAmpl error");
    }
  }
  return(numTrans);
}


/* ---------- functions ---------- */

/* IndiLineEncodeInit() */
/* Init individual lines bitstream encoder. */

ILEstatus *IndiLineEncodeInit (
  int frameLenQuant,		/* in: samples per frame */
				/*     used for quantiser selection */
  double fSampleQuant,		/* in: sampling frequency */
				/*     used for quantiser selection */
  double maxLineAmpl,		/* in: max line amplitude */
  int enhaFlag,			/* in: enha flag (0=basic 1=enha) */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int maxNumLine,		/* in: max num lines */
				/*     if maxNumLine < 0 use maxNumLineBS */
				/*     calculated from frameMaxNumBit */
  int maxNumEnv,		/* in: max num envelopes */
  int maxNumHarm,		/* in: max num harmonic tones */
  int maxNumHarmLine,		/* in: max num lines of harmonic tone */
  int maxNumNoisePara,		/* in: max num noise parameters */
  ILEconfig *cfg,		/* in: ILE configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  HANDLE_BSBITSTREAM hdrStream,	/* out: bit stream header */
  int *maxNumLineBS,		/* out: max num lines in bitstream */
  double *maxNoiseFreq)		/* out: max noise freq */
				/* returns: ILE status handle */
{
  ILEstatus *ILE;
  int i;
  int hdrFrameLengthBase = 0;
  int hdrFrameLength;


  if ((ILE = (ILEstatus*)malloc(sizeof(ILEstatus)))==NULL)
    IndiLineExit("IndiLineEncodeInit: memory allocation error");

  ILE->cfg = GenILEconfig();

  if (cfg != NULL)
    memcpy(ILE->cfg,cfg,sizeof(ILEconfig));

  ILE->debugLevel = debugLevel;
  ILE->maxNumLine = maxNumLine;
  ILE->maxNumEnv = maxNumEnv;
  ILE->maxNumHarm = maxNumHarm;
  ILE->maxNumHarmLine = maxNumHarmLine;
  ILE->maxNumNoisePara = maxNumNoisePara;
  ILE->maxNumAllLine = maxNumLine+maxNumHarm*maxNumHarmLine;

  if ( ILE->cfg->bsFormat<4 ) {

    ILE->fSample = fSampleQuant;
    if (ILE->cfg->quantMode < 0) {
      /* HP 980216 */
      if (fSampleQuant <= 8000.5)
        ILE->quantMode = 0;
      else if (fSampleQuant <= 16000.5)
        ILE->quantMode = 2;
      else
        ILE->quantMode = 4;
    }
    else
      ILE->quantMode = ILE->cfg->quantMode;
    switch (ILE->quantMode) {
    case 0:
    case 1:
      *maxNoiseFreq = 4000.;
      break;
    case 2:
    case 3:
      *maxNoiseFreq = 8000.;
      break;
    case 4:
    case 5:
      *maxNoiseFreq = 20000.;
      break;
    }

    hdrFrameLengthBase = ((int)(fSampleQuant+.5)%11025 == 0) ? 1 : 0;
    hdrFrameLength = (int)(frameLenQuant/fSampleQuant*
			   (hdrFrameLengthBase ? 44100.0 : 48000.0));
  }
  else
  {
    if (ILE->cfg->quantMode < 0)
      ILE->quantMode = 0;	/* auto: new lines with 3dB ampl quant */
    else
      ILE->quantMode = ILE->cfg->quantMode;
    ILE->sampleRateCode = calcSRC(sampleRateTable,13,fSampleQuant);
    ILE->maxFIndex = maxFIndexTable[ILE->sampleRateCode];
    *maxNoiseFreq = 0.5 * ((double) sampleRateTable[ILE->sampleRateCode]);
    hdrFrameLength = frameLenQuant;

    ILE->fSample = sampleRateTable[ILE->sampleRateCode];
  }


  ILE->contMode = ILE->cfg->contMode;
  ILE->enhaFlag = enhaFlag;
  if (ILE->enhaFlag) {
    if (ILE->cfg->enhaQuantMode < 0) {
      /* ... in bsf4, quantMode is 0 or 1 anyway ...   HP 990603 */
      /* (enhaQuantMode=2 for 32ms frames and bsf4 freq quant */
      if (ILE->quantMode <= 1)
	ILE->enhaQuantMode = 2;
      else
	ILE->enhaQuantMode = 1;
    }
    else
      ILE->enhaQuantMode = ILE->cfg->enhaQuantMode;
  }
  else
    ILE->enhaQuantMode = 0;


  /* first calculation with minimum number of bits */
  IndiLineNumBit(NULL,&(ILE->envNumBit),&(ILE->newLineNumBit),
		 &(ILE->contLineNumBit));
  if (ILE->cfg->bsFormat < 4)
    ILE->maxNumLineBS = (frameMaxNumBit-1)/(ILE->contLineNumBit+1)+1;
  else	/* bsf=4: min. avg. len(da)+len(df) = 5 (1st guess ;-)   HP990225 */
    ILE->maxNumLineBS = (frameMaxNumBit-1)/(5+1)+1;
  if (maxNumLine >= 0)
    ILE->maxNumLineBS = min(maxNumLine,ILE->maxNumLineBS);

  ILE->ILQ = IndiLineQuantInit(ILE->quantMode,ILE->enhaQuantMode,maxLineAmpl,
			       ILE->maxNumLineBS,0,ILE->maxFIndex,max(0,debugLevel-1),ILE->cfg->bsFormat);

  IndiLineNumBit(ILE->ILQ,&(ILE->envNumBit),&(ILE->newLineNumBit),
		 &(ILE->contLineNumBit));
  if (ILE->cfg->bsFormat < 4)
    ILE->maxNumLine = (frameMaxNumBit-1)/(ILE->contLineNumBit+1)+1;
  else	/* bsf=4: min. avg. len(da)+len(df) = 5 (1st guess ;-)   HP990225 */
    ILE->maxNumLine = (frameMaxNumBit-1)/(5+1)+1;
  if (maxNumLine >= 0)
    ILE->maxNumLineBS = min(maxNumLine,ILE->maxNumLineBS);

  *maxNumLineBS = ILE->maxNumLineBS;

  /* calc num bits required for numLine in bitstream */
  ILE->numLineNumBit = 0;
  while (1<<ILE->numLineNumBit <= ILE->maxNumLineBS)
    ILE->numLineNumBit++;

  /* put header info bits to header */
  if (ILE->cfg->bsFormat < 2) {
    /* old header */
    if (BsPutBit(hdrStream,ILE->maxNumLineBS,16) ||
	BsPutBit(hdrStream,(long)(fSampleQuant+.5),32) ||
	BsPutBit(hdrStream,frameLenQuant,16))
      IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
  }
  else {
    if ( ILE->cfg->bsFormat<4 ) {
      if (BsPutBit(hdrStream,ILE->quantMode,3))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->maxNumLineBS,8))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,hdrFrameLengthBase,1))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,hdrFrameLength,12))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->contMode,2))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->enhaFlag,1))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (ILE->enhaFlag) {
	if (BsPutBit(hdrStream,ILE->enhaQuantMode,2))
	  IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      }
    }
    else
    {
      if (BsPutBit(hdrStream,ILE->quantMode,1))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->maxNumLineBS,8))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->sampleRateCode,4))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,hdrFrameLength,12))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->contMode,2))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (BsPutBit(hdrStream,ILE->enhaFlag,1))
	IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      if (ILE->enhaFlag) {
	if (BsPutBit(hdrStream,ILE->enhaQuantMode,2))
	  IndiLineExit("IndiLineEncodeInit: error generating bit stream header");
      }
    }
  }

  if (
      /* variable content arrays and arrays for frame-to-frame memory */
      (ILE->lineParaSFI=(int*)malloc(ILE->maxNumLine*sizeof(int)))==NULL ||
      (ILE->lineParaEnv=(int*)malloc(ILE->maxNumLine*sizeof(int)))==NULL ||
      (ILE->lineContFlag=(int*)malloc(ILE->maxNumLine*sizeof(int)))==NULL ||
      (ILE->linePredEnc=(int*)malloc(ILE->maxNumLine*sizeof(int)))==NULL ||
      (ILE->harmPredEnc=(int*)malloc(ILE->maxNumHarm*sizeof(int)))==NULL ||

      (ILE->transAmpl=(double*)malloc(ILE->maxNumHarmLine*
				     sizeof(double)))==NULL ||
      (ILE->harmFreqIndex=(unsigned int*)malloc(ILE->maxNumHarm*
						sizeof(unsigned int)))==NULL ||
      (ILE->harmFreqCW=(long*)malloc(ILE->maxNumHarm*sizeof(long)))==NULL ||
      (ILE->harmAmplCW=(long*)malloc(ILE->maxNumHarm*sizeof(long)))==NULL ||

      (ILE->harmFreqStretchIndex=(unsigned int*)malloc(ILE->maxNumHarm*
					        sizeof(unsigned int)))==NULL ||
      (ILE->harmAmplIndex=(unsigned int**)malloc((ILE->maxNumHarm+1)*			/* +1 to avoid illegal pointer */
					       sizeof(unsigned int*)))==NULL ||
      (ILE->harmAmplIndex[0]=(unsigned int*)malloc(ILE->maxNumHarm*
						   ILE->maxNumHarmLine*
						sizeof(unsigned int)))==NULL ||
	(ILE->harmLPCAmpl=(double*)malloc(ILE->maxNumHarmLine*sizeof(double)))==NULL ||
	(ILE->harmLPCAmplTmp=(double*)malloc(ILE->maxNumHarmLine*sizeof(double)))==NULL ||
	(ILE->harmLPCPara=(double*)malloc(ILE->maxNumHarmLine*sizeof(double)))==NULL ||
	(ILE->harmFreqStretchQuant=(int    *) malloc(ILE->maxNumHarm*sizeof(int)))==NULL ||
	(ILE->harmLPCIndex        =(int   **) malloc((ILE->maxNumHarm+1)*sizeof(int *)))==NULL ||
	(ILE->harmLPCIndex[0]     =(int    *) malloc(ILE->maxNumHarm*LPCMaxNumPara*sizeof(int)))==NULL ||
	(ILE->harmLPCNonLin       =(double  *) malloc(LPCMaxNumPara*sizeof(double)))==NULL ||
	(ILE->harmLPCDeqOld       =(double **) malloc((ILE->maxNumHarm+1)*sizeof(double *)))==NULL ||
	(ILE->harmLPCDeqOld[0]    =(double  *) malloc(ILE->maxNumHarm*LPCMaxNumPara*sizeof(double)))==NULL ||

      (ILE->noiseParaIndex=(unsigned int*)malloc(ILE->maxNumNoisePara*
						sizeof(unsigned int)))==NULL ||
      (ILE->noiseLPCIndex=(int*)malloc(LPCMaxNumPara*sizeof(int)))==NULL ||
      (ILE->noiseLPCDeqOld=(double *)malloc(LPCMaxNumPara*sizeof(double)))==NULL ||
      (ILE->noiseLPCNonLin=(double *)malloc(LPCMaxNumPara*sizeof(double)))==NULL ||

      (ILE->harmEnhaIndex=(unsigned int*)malloc(ILE->maxNumHarmLine*
					       sizeof(unsigned int)))==NULL ||
      (ILE->harmEnhaNumBit=(unsigned int*)malloc(ILE->maxNumHarmLine*
					       sizeof(unsigned int)))==NULL)
    IndiLineExit("IndiLineEncodeInit: memory allocation error");

  for (i=0; i<LPCMaxNumPara; i++) ILE->noiseLPCIndex[i]=0;
  for (i=0;i<ILE->maxNumHarm;i++) {
	register int	j;

	ILE->harmAmplIndex[i] = ILE->harmAmplIndex[0]+i*ILE->maxNumHarmLine;

	ILE->harmLPCIndex[i]  = ILE->harmLPCIndex[0]+i*LPCMaxNumPara;
	ILE->harmLPCDeqOld[i] = ILE->harmLPCDeqOld[0]+i*LPCMaxNumPara;
  	for (j=0; j<LPCMaxNumPara; j++) ILE->harmLPCIndex[i][j]=0;
	ILE->harmLPCDeqOld[i][0]=0.0;
	ResetLPCPara(ILE->harmLPCDeqOld[i]+1,0,LPCMaxNumPara-1,&LPCharmEncPar);
  }
  ILE->noiseLPCDeqOld[0]=0.0;
  ResetLPCPara(ILE->noiseLPCDeqOld+1,0,LPCMaxNumPara-1,&LPCnoiseEncPar);


  /* clear frame-to-frame memory */
  ILE->prevQuantNumLine = 0;
  ILE->prevNumTrans = 0;
  ILE->prevNumNoise=0;


  for (i=0; i<MAX_LAYER; i++) ILE->layPrevNumLine[i]=0;


  if (debugLevel >= 2) {
    printf("IndiLineEncodeInit: envNumBit=%d  newLineNumBit=%d\n"
	   "                    contLineNumBit=%d\n"
	   "                    maxNumLineBS=%d  numLineNumBit=%d\n",
	   ILE->envNumBit,ILE->newLineNumBit,ILE->contLineNumBit,
	   ILE->maxNumLine,ILE->numLineNumBit);
    printf("IndiLineEncodeInit: frmLenBase=%d  frmLen=%d  (%8.6f)\n",
	   hdrFrameLengthBase,hdrFrameLength,
	   hdrFrameLength/(hdrFrameLengthBase ? 44100.0 : 48000.0));
    printf("IndiLineEncodeInit: quantMode=%d  contMode=%d  "
	   "enhaFlag=%d  enhaQuantMode=%d\n",
	   ILE->quantMode,ILE->contMode,ILE->enhaFlag,ILE->enhaQuantMode);
  }

  return ILE;
}


/* IndiLineEncodeFrame() */
/* Encode individual lines frame to bitstream. */

void IndiLineEncodeFrame (
  ILEstatus *ILE,		/* in: ILE status handle */
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
  int *lineEnv,			/* in: line envelope flag/idx */
				/*     [0..numLine-1] */
  int *linePred,		/* in: line predecessor idx */
				/*     [0..numLine-1] */
  int numHarm,			/* in: num harmonic tones */
  int *numHarmLine,		/* in: num lines of harmonic tone */
				/*     [0..numHarm-1] */
  double *harmFreq,		/* in: harm fundamental frequency [Hz] */
				/*     [0..numHarm-1] */
  double *harmFreqStretch,	/* in: harm freq stretch ratio */
				/*     [0..numHarm-1] */
  int *harmLineIdx,		/* in: harm line para start idx */
				/*     [0..numHarm-1] */
				/*     ampl: lineAmpl[idx] etc. */
				/*     idx=harmLineIdx+(0..numHarmLine-1) */
  int *harmEnv,			/* in: harm envelope flag/idx */
				/*     [0..numHarm-1] */
  int *harmPred,		/* in: harm tone predecessor idx */
				/*     [0..numHarm-1] */
  double *harmRate,		/* in: bitrate for harm data (0..1) */
				/*     [0..numHarm-1] */
  int numNoisePara,		/* in: num noise parameter */
  double noiseFreq,		/* in: max noise freq (bandwidth) [Hz] */
  double *noisePara,		/* in: noise parameter */
				/*     DCT[] / StdDev+RefCoef[] */
				/*     [0..numNoisePara-1] */
  int noiseEnv,			/* in: noise envelope flag/idx */
  double noiseRate,		/* in: bitrate for noise data (0..1) */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int numExtLayer,		/* in: num extension layers */
  int *extLayerNumBit,		/* in: num bits for extension layer(s) */
				/*     [0..numExtLayer-1] */
				/*     or NULL */
  HANDLE_BSBITSTREAM *streamESC,	/* out: bit stream (ESC 0..4) */
  HANDLE_BSBITSTREAM streamEnha,	/* out: enhancement bit stream */
				/*      or NULL for basic mode */
  HANDLE_BSBITSTREAM *streamExt)	/* out: extension layer bit stream(s) */
				/*      [0..numExtLayer-1] */
				/*      or NULL */
{
  int i,il,ilTrans,j,layer;
  int firstEnvLine;
  int basicMaxNumBit,basicNumBit,chkNumBit;
  int tmpNumBit,contNumBit,newNumBit,envNumBit;
  int quantNumLine;
  double *quantEnvPara;
  double *quantEnvParaEnha;
  int envParaFlag,quantEnvParaFlag;
  int envParaNumBit,envParaEnhaNumBit;
  unsigned int envParaIndex,envParaEnhaIndex;
  double *quantLineFreq;
  double *quantLineAmpl;
  int *quantLineEnv;
  int *quantLinePred;
  double *quantLineFreqEnha;
  double *quantLineAmplEnha;
  double *quantLinePhase;
  int *lineSeq;
  int *lineParaPred;
  int *lineParaNumBit;
  unsigned long *lineParaIndex;
  int *lineParaEnhaNumBit;
  unsigned long *lineParaEnhaIndex;
  int	*lineParaFIndex=NULL;
  int	*lineParaAIndex=NULL;
  int	*prevLineParaFIndex=NULL;
  int	*prevLineParaAIndex=NULL;

  int harmMaxNumBit,harmNumBit;
  double quantHarmFreq = 0;
  double quantHarmFreqStretch = 0;
  int harmAmplInt,harmAmplIntOverl,prevAmplInt;
  int addHarmAmplBits = 0;
  int harmAmplMax,harmAmplOff;

  int numTrans,numTransIndex;
  int numTransDesired = 0;
  int numHarmLineCod = 0;
  int numHarmLineIndex = 0;

  int noiseMaxNumBit,noiseNumBit;
  int quantNumNoise;
  int quantNumNoiseIndex = 0;
  unsigned int noiseNormIndex = 0;
  double noiseNorm;
  unsigned int noiseEnvParaIndex;
  int numNoiseParaNumBit = 0;
  int noiseParaNumBit = 0;
  int noiseParaSteps;
  int noiseEnvParaFlag,noiseEnvParaNumBit,noiseEnvParaEnhaNumBit;
  unsigned int noiseEnvParaEnhaIndex;
  int xint;
  int maxAmplIndex = 0;

  int fhint,ahint,anint;
  int fNumBitBase = 0;
  int fNumBitE;
  int freqNumStepE = 0;
  int fintE = 0;
  int pintE;
  double fq = 0;
  double hfreqrs = 0;

  int indiMaxNumBit;
  int extTotNumBit = 0;

  int layNumBit[MAX_LAYER];
  int layNumLine[MAX_LAYER];
  int layMaxNumBit[MAX_LAYER];
  int numLayer = numExtLayer+1;

  int layNumPhase[MAX_LAYER];
  int harmNumPhase = 0;

  int noisePredEnc;

  /*$ BE991125>>> $*/
  HANDLE_BSBITSTREAM bsESC0,bsESC1,bsESC2,bsESC3,bsESC4,stream;
  int numBitESC0,numBitESC1,numBitESC2,numBitESC3,numBitESC4;
  int harmContFlag = 0;
  int noiseContFlag = 0;
  int lfi;
  long tmpCW;

  /* HP20010131 indilineenc streamESC vs old bsFormat */
  stream = streamESC[0]; /* NULL; */ /* legacy old bsFormat */
  bsESC0 = streamESC[0];
  bsESC1 = streamESC[1];
  bsESC2 = streamESC[2];
  bsESC3 = streamESC[3];
  bsESC4 = streamESC[4];
  numBitESC0 = numBitESC1 = numBitESC2 = numBitESC3 = numBitESC4 = 0;
  /*$ <<<BE991125 $*/

  numLine = min(numLine,ILE->maxNumLine);	/* BE 980617 */

  /* indiline continuation parameters */
  for (i=0; i<numLine; i++) {
    if (linePred[i]-1<ILE->maxNumLine)		/* HP 980624 */
      ILE->linePredEnc[i] = linePred[i];
    else
      ILE->linePredEnc[i] = 0;
  }
  ILE->harmPredEnc[0] = harmPred[0];
  noisePredEnc = 1;

  if (ILE->cfg->encContMode) {
    /* no continued individual lines */
    for (i=0; i<numLine; i++)
      ILE->linePredEnc[i] = 0;
    ILE->harmPredEnc[0] = 0;
    noisePredEnc = 0;
  }


  /* quantise envelope parameters if required */
  firstEnvLine = -1;
  for (il=0; il<numLine; il++)
    if (lineEnv[il] && firstEnvLine<0)
      firstEnvLine = il;
  if (firstEnvLine >= 0 || (numHarm && numHarmLine[0] && harmEnv[0]))
    IndiLineEnvQuant(ILE->ILQ,envPara[0],&envParaFlag,
		     &envParaIndex,&envParaNumBit,
		     &envParaEnhaIndex,&envParaEnhaNumBit);
  else
    envParaFlag = 0;
  if (envParaFlag && numHarm && harmEnv)
    firstEnvLine = -1;	/* force env para coding */
  for (il=0; il<numLine; il++)
    ILE->lineParaEnv[il] = lineEnv[il] && envParaFlag;
  harmEnv[0] = harmEnv[0] && envParaFlag;

  /* dequantise envelope parameters */
  IndiLineEnvDequant(ILE->ILQ,envParaFlag,envParaIndex,envParaNumBit,
		     envParaEnhaIndex,0,
		     &quantEnvPara,&quantEnvParaEnha);

  /* quantise line parameters */
  if ( ILE->cfg->bsFormat<4 )
	  IndiLineQuant(ILE->ILQ,numLine,lineFreq,lineAmpl,linePhase,
			ILE->lineParaEnv,
			ILE->linePredEnc,
			&lineParaPred,&lineParaIndex,&lineParaNumBit,
			&lineParaEnhaIndex,&lineParaEnhaNumBit,NULL,NULL,NULL,NULL);
  else
	  IndiLineQuant(ILE->ILQ,numLine,lineFreq,lineAmpl,linePhase,
			ILE->lineParaEnv,
			ILE->linePredEnc,
			&lineParaPred,&lineParaIndex,&lineParaNumBit,
			&lineParaEnhaIndex,&lineParaEnhaNumBit,
			&lineParaFIndex,&lineParaAIndex,
			&prevLineParaFIndex,&prevLineParaAIndex);

  /* determine number of bits to be used for this frame */
  basicMaxNumBit = frameAvailNumBit;

  /* extension layer stuff NM/HP 990608 */
  for (i=1; i<numLayer; i++) {
    layMaxNumBit[i] = extLayerNumBit[i-1];
    basicMaxNumBit -= layMaxNumBit[i];
  }
  layMaxNumBit[0] = basicMaxNumBit;


  /* determine number of lines which can be transmitted with basicMaxNumBit */
  basicNumBit = ILE->numLineNumBit;	/* numLine bits */
  if (ILE->cfg->bsFormat>=4)
    basicNumBit += 4;	/* maxAmplIndex	*/
  if (ILE->cfg->bsFormat>=5) {
    basicNumBit += 1;				/* phaseFlag */
    if (ILE->cfg->phaseFlag)
      basicNumBit += ILE->numLineNumBit;	/* numLinePhase */
  }
  basicNumBit += 1+ILE->layPrevNumLine[0];	/* env flag and continue bits */


  if (ILE->cfg->bsFormat<4) {

    /* noise stuff */

    numNoiseParaNumBit = 2;
    noiseParaNumBit = 3;
    if (ILE->cfg->bsFormat>=3) {
      numNoiseParaNumBit = 4;
      noiseParaNumBit = 4;
    }

    quantNumNoise = 0;
    if(noiseRate>.01)
      quantNumNoise = 4;
    if(noiseRate>.03)
      quantNumNoise = 5;
    if(noiseRate>.1)
      quantNumNoise = 6;
    if(noiseRate>.3)
      quantNumNoise = 7;
    if (ILE->cfg->bsFormat>=3) {
      quantNumNoise = 0;
      if(noiseRate>.01)
        quantNumNoise = 4;
      if(noiseRate>.03)
        quantNumNoise = max(5,numNoisePara/2);
      if(noiseRate>.1)
        quantNumNoise = max(6,(numNoisePara*2)/3);
      if(noiseRate>.3)
        quantNumNoise = max(7,numNoisePara);
      quantNumNoise = min(19,quantNumNoise);
    }
    quantNumNoise = min(numNoisePara,quantNumNoise);   /* fixed HP980618 */


  /* HP 970709 */
    noiseMaxNumBit = (int)((basicMaxNumBit-ILE->numLineNumBit
			  -1-ILE->layPrevNumLine[0]
			  -envParaFlag*envParaNumBit)
			 *(1.0+noiseRate)/(1.0+1.0)*0.5);
    quantNumNoise = min(quantNumNoise,(noiseMaxNumBit-1-numNoiseParaNumBit-
				     1-6)/noiseParaNumBit);
    if (quantNumNoise < 4)
      quantNumNoise = 0;

    noiseNumBit = 1;
    if(quantNumNoise) {
      noiseNumBit += numNoiseParaNumBit;	/* quantNumNoise */
      noiseNumBit += 1;			/* noiseEnv */
      noiseNumBit += 6;			/* noiseNormIndex */
      noiseNumBit += quantNumNoise*
			noiseParaNumBit;	/* noiseParaIndex[0] ... */

      if(noiseEnv) {
        IndiLineEnvQuant(ILE->ILQ,envPara[noiseEnv-1],&noiseEnvParaFlag
			,&noiseEnvParaIndex,&noiseEnvParaNumBit
			,&noiseEnvParaEnhaIndex,&noiseEnvParaEnhaNumBit);
      }
      else
        noiseEnvParaFlag = 0;
      if(noiseEnvParaFlag)
        noiseNumBit += noiseEnvParaNumBit;
    }
    if(quantNumNoise) {
      noiseParaSteps = 1<<noiseParaNumBit;
      noiseNorm = 1;
      for(i=0;i<quantNumNoise;i++)
        noiseNorm = max(noiseNorm,fabs(noisePara[i]));
      noiseNormIndex = min((int)(2.*log(max(noiseNorm,1))/log(2.)),63);
      noiseNorm = exp((double)(noiseNormIndex+1)*log(2.)/2.)/noiseParaSteps;
      ILE->noiseParaIndex[0] = min(max((int)(noisePara[0]/noiseNorm),0),
				 noiseParaSteps-1);
      for(i=1;i<quantNumNoise;i++)
	ILE->noiseParaIndex[i] = min(max((int)(noisePara[i]/noiseNorm/2.+
					     noiseParaSteps/2.),0),
				   noiseParaSteps-1);

      if(ILE->debugLevel>=3) {
	printf("numNoise=%2d, noiseNumBit=%3d, noiseNormIndex=%2d\n"
		,quantNumNoise,noiseNumBit,noiseNormIndex);
	printf("noiseIndex: %3d",(int)(noisePara[0]/noiseNorm));
	for(i=1;i<quantNumNoise;i++)
	  printf("%3d",(int)(noisePara[i]/noiseNorm/2.+(1<<noiseParaNumBit)));
	printf("\n");
      }
    }

    /* harmonic stuff */

    if ( numHarm ) {
      if (numHarmLine[0]==1)
	IndiLineExit("IndiLineEncodeFrame: harm. with 1 line not supported!");

      numTransDesired = numTrans =
        TransfHarmAmpl(numHarmLine[0],lineAmpl+harmLineIdx[0],ILE->transAmpl);
    }
    else
      numTrans = 0;

    for(i=0;i<numTrans;i++) {
      ILE->transAmpl[i] = log(max(ILE->transAmpl[i],1.))/log(2.);
    }

    /* HP 970709   ILE->numLineNumBit was neglected ... */
    /*  harmMaxNumBit = min((int)(basicMaxNumBit*.75), */  /* 75% for harm */

    harmMaxNumBit = (int)((basicMaxNumBit-ILE->numLineNumBit
			   -1-ILE->layPrevNumLine[0]
			   -envParaFlag*envParaNumBit-noiseNumBit
			   -(numLine?ILE->newLineNumBit+envParaFlag:0))
			   *(0.5+harmRate[0])/(0.5+1.0));

    harmNumBit = 1;	/* harm flag */
    if (numTrans) {
      harmNumBit += 6;	/* numTrans */
      harmNumBit += 1;	/* add. ampl bits */
      harmNumBit += 1;	/* cont */
      harmNumBit += 1;	/* env */
      harmNumBit += 11;	/* f */
      harmNumBit += 5;	/* fs */
      harmNumBit += 6;	/* transAmpl0 */

      if(numTrans<=16)
        addHarmAmplBits = 1;
      else
        addHarmAmplBits = 0;

      numTrans = min(numTrans,(harmMaxNumBit-harmNumBit)/(4+addHarmAmplBits)+1);

      numTrans = max(0,min(64,numTrans));		/* BE/HP 971115 */

      numHarmLineCod = numTrans;

      harmNumBit += (4+addHarmAmplBits)*(numTrans-1);	/* transAmpl1... */

      if(ILE->debugLevel>=3) {
	printf("numTrans=%2d harmNumBit=%3d harmMaxNumBit=%3d",
		numTrans,harmNumBit,harmMaxNumBit);
	if(numTrans<numTransDesired)
	  printf("Desired=%2d\n",numTransDesired);
	else
	  printf(" \n");
      }

      /* quantise harm */
      harmAmplMax = 16*(1+addHarmAmplBits)-1;
      harmAmplOff = harmAmplMax/3;

      fhint = (int)(log(harmFreq[0]/30.)/log(4000./30.)*2048.);
      ILE->harmFreqIndex[0] = max(0,min(2047,fhint));
      quantHarmFreq = exp((ILE->harmFreqIndex[0]+.5)/2048.*log(4000./30.))*30.;


      xint = (int)((harmFreqStretch[0]/0.002+0.5)*32.+.5);
      ILE->harmFreqStretchIndex[0] = max(0,min(31,xint));
      quantHarmFreqStretch = (ILE->harmFreqStretchIndex[0]/32.-.5)*0.002;
      harmAmplInt = (int)(ILE->transAmpl[0]*3.);
      prevAmplInt = ILE->harmAmplIndex[0][0] = max(0,min(63,harmAmplInt));
      for(i=1;i<numTrans;i++) {
	harmAmplInt = (int)(ILE->transAmpl[i]*3.);
	ILE->harmAmplIndex[0][i] = max(0,harmAmplInt-prevAmplInt+harmAmplOff);
	prevAmplInt += ILE->harmAmplIndex[0][i]-harmAmplOff;
	harmAmplIntOverl = ILE->harmAmplIndex[0][i]-harmAmplMax;
	j = i;
	while(harmAmplIntOverl>0 && j>0) {

	  if(ILE->debugLevel>=3 && harmAmplIntOverl>0)
	    printf("harm ampl overload correction: line# %2d (%2d), index %2d\n",
		   j,i,ILE->harmAmplIndex[0][j]);

	  ILE->harmAmplIndex[0][j] = harmAmplMax;
	  ILE->harmAmplIndex[0][j-1] += harmAmplIntOverl;
	  harmAmplIntOverl = ILE->harmAmplIndex[0][j-1]-harmAmplMax;
	  j--;
	}
      }

      /* quantise harm enhancement */

      fNumBitBase = 0;
      while (fNumBitBase<HARM_MAX_FBASE && fhint>freqThrE[fNumBitBase])
	fNumBitBase++;
      hfreqrs = HFREQRELSTEP;
    }
  }
  else	/* bsFormat>=4	*/
  {
	int i;
	double f;

	if ( envParaFlag && ((firstEnvLine<0) || (numLayer>1)) )
	  basicNumBit += envParaNumBit;

	maxAmplIndex=60;	/* only the 4 MSBs are valid	*/

	for (i=0; i<numLine; i++)
	{
		register int p = lineParaPred[i];
		register int a = lineParaAIndex[i];

		if ( p && (numLayer==1) ) continue;
		if ( p ) a += prevLineParaAIndex[p-1];
		if ( a<maxAmplIndex ) maxAmplIndex=a&(-4);
	}

	/* for(i=0;i<numNoisePara-1;i++)fprintf(stderr,"nk%2d = %10.5f (%10.3f)\n",i,noisePara[i+1],log((1.0+noisePara[i+1])/(1.0-noisePara[i+1]))); */



	/* try some noise bit allocation */
	noiseMaxNumBit = basicMaxNumBit-basicNumBit;
	f = 0.2 + 0.5*log(max(noiseRate,0.01)/0.01)/log(1/0.01);
	noiseMaxNumBit = min(noiseMaxNumBit,(int)(noiseMaxNumBit*f+.5));

	i=-1;
	if ( numNoisePara>0 )
	  {
		i = BSearch(LPCnoiseEncPar.numParaTable,16,numNoisePara-1);

		if (noiseRate < 0.002)
		  i = -1;
		else {
		  f = .5+.5*log(noiseRate/0.002)/log(0.1/0.002);
		  i = min(i,(int)(i*f+.5));
		}
		/*
		if ( noiseRate<=0.078125 ) i--;
		if ( noiseRate<=0.031250 ) i--;
		if ( noiseRate<=0.012500 ) i--;
		if ( noiseRate<=0.005000 ) i--;
		if ( noiseRate<=0.002000 ) i=-1;
		*/
	}

	if ( ILE->debugLevel>=2 ) printf("NoiseIndex: %i, noiseRate=%f\n",i,noiseRate);


	if ( i>=0 )
	{
		register int	j = LPCnoiseEncPar.numParaTable[i];
		register int	n;

		n=min(j,(noisePredEnc?ILE->prevNumNoise:0));

		if ( n>0 ) ScaleLPCPredMean(ILE->noiseLPCDeqOld+1,n,&LPCnoiseEncPar);
		ResetLPCPara(ILE->noiseLPCDeqOld+1,n,j-n,&LPCnoiseEncPar);

		LPCCompressK(ILE->noiseLPCNonLin+1,noisePara+1,j,&LPCnoiseEncPar);


		anint = AQuant(ILE->ILQ,noisePara[0]);
		if ( (noisePredEnc?ILE->prevNumNoise:0)>0 )
		{
			anint-=ILE->noiseLPCIndex[0];
			ILE->noiseAmplCW=AContEncode(&anint);
			anint+=ILE->noiseLPCIndex[0];
		}
		else
		{
			if ( anint < maxAmplIndex ) maxAmplIndex=anint&(-4);
			ILE->noiseAmplCW=ANNewEncode(&anint,maxAmplIndex);
		}

		ILE->noiseLPCIndex[0] = anint;

		noiseNumBit = (ILE->noiseAmplCW&255) + 1 + 1 + 1 + 4;
				/* ampl + flag +  cont. bit + env flag + num.para*/
		if( noiseEnv ) {
			IndiLineEnvQuant(ILE->ILQ,envPara[noiseEnv-1],&noiseEnvParaFlag,
				&noiseEnvParaIndex,&noiseEnvParaNumBit,
				&noiseEnvParaEnhaIndex,&noiseEnvParaEnhaNumBit);
		}
		else
			noiseEnvParaFlag = 0;

		if ( noiseEnvParaFlag ) noiseNumBit += noiseEnvParaNumBit;

		noiseNumBit += QuantizeLPCSet(ILE->noiseLPCIndex+1,ILE->noiseLPCNonLin+1,
					   ILE->noiseLPCDeqOld+1,&i,&LPCnoiseEncPar,
					   noiseMaxNumBit-noiseNumBit);

		if (i<0) {
		  noiseNumBit=1;
		  quantNumNoiseIndex = -1;
		  quantNumNoise = 0;
		  /* if (ILE->debugLevel) */
		  fprintf(stderr,"Warning: no bits allocated for noise\n");
		}
		else {
		  quantNumNoiseIndex = i;
		  quantNumNoise = LPCnoiseEncPar.numParaTable[i];
		}
	}
	else
	{
		noiseNumBit=1;
		quantNumNoiseIndex = -1;
		quantNumNoise = 0;
	}

	/* try some harm bit allocation */
	harmMaxNumBit = basicMaxNumBit-basicNumBit-noiseNumBit;
	f = 0.5 + 0.3*log(max(harmRate[0],0.1)/0.1)/log(1/0.1);
	harmMaxNumBit = min(harmMaxNumBit,(int)(harmMaxNumBit*f+.5));

	if ( numHarm )
	{
		int	nhl,nhp;

		/* harmPred[0]=0; */

		nhp=ILE->prevNumTrans;
		if ( ILE->harmPredEnc[0]<1 ) nhp=0;
		ResetLPCPara(ILE->harmLPCDeqOld[0]+1,nhp,LPCMaxHarmPara-1-nhp,&LPCharmEncPar);
		if ( nhp>0 ) ScaleLPCPredMean(ILE->harmLPCDeqOld[0]+1,nhp,&LPCharmEncPar);

		fhint = (int)(log(harmFreq[0]/20.)/log(4000./20.)*2048.);
		fhint = max(0,min(2047,fhint));
		quantHarmFreq = exp((fhint+.5)/2048.*log(4000./20.))*20.;

		harmNumBit = 1 + 4 + 1 + 1 + 5 +
			/* enable + numPara + cont + env + num.lines	*/
			QuantizeHStretch(ILE->harmFreqStretchQuant,harmFreqStretch[0]);

		quantHarmFreqStretch = ((double) HSTRETCHQ) * ((double) ILE->harmFreqStretchQuant[0]);

		numHarmLineIndex=BSearch(numHarmLineTab,32,numHarmLine[0]);
		numHarmLineCod=nhl=numHarmLineTab[numHarmLineIndex];

		harmNumPhase = 0;
		if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag && !(ILE->harmPredEnc[0]>0 && ILE->prevNumTrans>0)) {
		  harmNumPhase = min(numHarmLineCod,ILE->cfg->maxPhaseHarm);
		  harmNumPhase = min(harmNumPhase,(1<<HARMPHASEBITS)-1);
		  harmNumBit += HARMPHASEBITS+harmNumPhase*HARM_PHASE_NUMBIT;
		}

		numTransIndex = BSearch(LPCharmEncPar.numParaTable,16,
					nhl
					);
/*
*/
		numTrans      = LPCharmEncPar.numParaTable[numTransIndex];

		LPCharmGetPK(ILE->harmLPCPara,&numTransIndex,lineAmpl+harmLineIdx[0],nhl);
	/*	LPCharmGetPKrec(ILE->harmLPCPara,numTrans+1,lineAmpl+harmLineIdx[0],nhl,ILE->harmLPCAmpl,ILE->harmLPCAmplTmp);	*/
		LPCCompressK(ILE->harmLPCNonLin+1,ILE->harmLPCPara+1,numTrans,&LPCharmEncPar);


		numTrans      = LPCharmEncPar.numParaTable[numTransIndex];

		ahint = AQuant(ILE->ILQ,ILE->harmLPCPara[0]);

		if ( ILE->harmPredEnc[0]>0 && ILE->prevNumTrans>0 ) 
		{
			fhint-=ILE->harmFreqIndex[0];
			ILE->harmFreqCW[0]=FHContEncode(&fhint);
			fhint+=ILE->harmFreqIndex[0];
			ahint-=ILE->harmLPCIndex[0][0];
			ILE->harmAmplCW[0]=AContEncode(&ahint);
			ahint+=ILE->harmLPCIndex[0][0];
		}
		else
		{
			if ( ahint<maxAmplIndex )
			{
				maxAmplIndex=ahint&(-4);
				if ( (noisePredEnc?ILE->prevNumNoise:0)<1 &&
				     quantNumNoise /* HP20001211 */)
				{
					noiseNumBit-=(ILE->noiseAmplCW&255);
					ILE->noiseAmplCW=ANNewEncode(&anint,maxAmplIndex);
					noiseNumBit+=(ILE->noiseAmplCW&255);
				}
			}

			ILE->harmFreqCW[0]=(fhint<<8)|11;
			ILE->harmAmplCW[0]=AHNewEncode(&ahint,maxAmplIndex);
		}
		ILE->harmFreqIndex[0]  =fhint; harmNumBit+=ILE->harmFreqCW[0]&255;
		ILE->harmLPCIndex[0][0]=ahint; harmNumBit+=ILE->harmAmplCW[0]&255;

/*		harmMaxNumBit = (int)((basicMaxNumBit-ILE->numLineNumBit-1-ILE->layPrevNumLine[0]
					-envParaFlag*envParaNumBit-noiseNumBit-(numLine?ILE->newLineNumBit+envParaFlag:0))
					*(0.5+harmRate[0])/(0.5+1.0)); */

		harmNumBit += QuantizeLPCSet(	ILE->harmLPCIndex[0]+1,
						ILE->harmLPCNonLin+1,
						ILE->harmLPCDeqOld[0]+1,
						&numTransIndex,
						&LPCharmEncPar,
						harmMaxNumBit-harmNumBit);

		if ( numTransIndex<0 )
		{
		  numTrans=numHarm=0; harmNumBit=1;
		  /* if (ILE->debugLevel) */
		  fprintf(stderr,"Warning: no bits allocated for harm\n");
		}
		else
		{
			numTrans=LPCharmEncPar.numParaTable[numTransIndex];
			if ( numTrans<ILE->prevNumTrans )
				ResetLPCPara(ILE->harmLPCDeqOld[0]+1,numTrans,ILE->prevNumTrans-numTrans,&LPCnoiseEncPar);

			/* harm enhancement */

			fNumBitBase = 0;
			while (fNumBitBase<HARM_MAX_FBASE && fhint>freqThrEBsf4[fNumBitBase])
				fNumBitBase++;

			hfreqrs = HFREQRELSTEP4;
		}


		if ( ILE->debugLevel>=2 ) 
		{
			printf("Harm NumTrans: %i, harmMaxNumBit=%i, harmNumBit=%i, harmNumTrans=%i, harmNumLine=%i\n",
				numTrans,harmMaxNumBit,harmNumBit,numTrans,numHarmLineCod);
		}

	}
	else
	{
		harmNumBit=1;
		numTransIndex = -1;
		numTrans = 0;
	}
  }

  if ( numTrans>0 )
  {
      if (fNumBitBase==HARM_MAX_FBASE)
	IndiLineExit("IndiLineEncodeFrame: fnumbite error");
      fNumBitBase += max(0,min(3,ILE->enhaQuantMode))-3;

      for(i=0;i<numHarmLineCod;i++) {

	if(ILE->debugLevel>=5)
	  printf("harmEnh: ");

	if (i<10)
	  fNumBitE = max(0,fNumBitBase+fEnhaBitsHarm[i]);
	else
	  fNumBitE = 0;   /* freq enha only for first 10 harms   HP 990701 */

	if(fNumBitE>0) {
	  freqNumStepE = 1<<fNumBitE;
	  fq = quantHarmFreq*(i+1)*(1+quantHarmFreqStretch*(i+1));

	  fintE = (int)(((lineFreq[harmLineIdx[0]+i]/fq-1)/(hfreqrs-1.0)+.5)*
		    freqNumStepE);
	  fintE = max(min(fintE,freqNumStepE-1),0);
	}
	pintE = (int)((linePhase[harmLineIdx[0]+i]/HARM_PHASE_RANGE+.5)
		   *HARM_PHASE_NUMSTEP);
	pintE = max(min(pintE,HARM_PHASE_NUMSTEP-1),0);

	if(ILE->debugLevel>=5) {
 	  if(fNumBitE>0)
	    printf("f=%7.2f, fq=%7.2f, fe=%7.2f, ",
		   lineFreq[harmLineIdx[0]+i],fq,
		   fq*(1+((fintE+.5)/freqNumStepE-.5)*(hfreqrs-1.0)));
	  else
	    printf("f=%7.2f, fq=%7.2f, fe=%7.2f, ",
		   lineFreq[harmLineIdx[0]+i],
		   quantHarmFreq*(i+1)*(1+quantHarmFreqStretch*(i+1)),
		   quantHarmFreq*(i+1)*(1+quantHarmFreqStretch*(i+1)));

	  printf("p=%5.2f, pq=%5.2f\n",linePhase[harmLineIdx[0]+i],
	         ((pintE+.5)/HARM_PHASE_NUMSTEP-.5)*HARM_PHASE_RANGE);
	}

	ILE->harmEnhaIndex[i] = 0;
	ILE->harmEnhaNumBit[i] = 0;
	if (fNumBitE > 0) {	
	  ILE->harmEnhaIndex[i] = ILE->harmEnhaIndex[i]<<fNumBitE | fintE;
	  ILE->harmEnhaNumBit[i] += fNumBitE;
        }
	ILE->harmEnhaIndex[i] =  ILE->harmEnhaIndex[i]<<HARM_PHASE_NUMBIT
				  | pintE;
	ILE->harmEnhaNumBit[i] += HARM_PHASE_NUMBIT;
      }
  }

  basicNumBit += harmNumBit;	/* includes harm flag */
  basicNumBit += noiseNumBit;	/* includes noise flag */
  contNumBit = 0;
  newNumBit = 0;

  /* try some indi bit allocation & bit reservoir control */
  if (frameMaxNumBit!=frameNumBit) {
    j = min(10,numLine);
    i = 0;
    for(il=0; il<j; il++)
      if (linePred[il] && linePred[il]-1<ILE->layPrevNumLine[0])
	i++;
    i = j-i;
    indiMaxNumBit = max(basicMaxNumBit-basicNumBit,frameNumBit/3);
    indiMaxNumBit = (int)(indiMaxNumBit*(0.8+0.4*i/(double)max(j,1))+.5);
    basicMaxNumBit = basicNumBit+indiMaxNumBit;
    basicMaxNumBit = min(basicMaxNumBit,frameAvailNumBit-extTotNumBit);
    layMaxNumBit[0] = basicMaxNumBit;
    if (ILE->debugLevel>=2)
      printf("noiseMax=%d harmMax=%d new/all=%d/%d indiMax=%d basicMax=%d\n",
	     noiseMaxNumBit,harmMaxNumBit,i,j,indiMaxNumBit,basicMaxNumBit);
  }

  if ( ILE->cfg->bsFormat<4 )
  {
	  for (il=0; il<numLine; il++) {
	    if (envParaFlag && il>=firstEnvLine)
	      /* number of bits if envelope is needed */
	      chkNumBit = basicNumBit+envParaNumBit+(il+1)+lineParaNumBit[il];
	    else
	      /* number of bits if envelope is not needed */
	      chkNumBit = basicNumBit+lineParaNumBit[il];
	    if (chkNumBit > basicMaxNumBit)
	      break;
	    if (lineParaPred[il])
	      contNumBit += lineParaNumBit[il];
	    else
	      newNumBit += lineParaNumBit[il];
	    basicNumBit += lineParaNumBit[il];	/* line para bits */
	  }
	  layNumLine[0] = quantNumLine = il;	/* shared code uses layNumLine[numLayer-1]: bugfix NM990813	*/

	  /* determine if envelope parameters are transmitted */
	  quantEnvParaFlag = envParaFlag && quantNumLine>firstEnvLine;
	  if (quantEnvParaFlag)
	    basicNumBit += envParaNumBit+quantNumLine;	/* env para bits and */
							/* line env flags */
  }
  else	/* bsFormat>=4	*/
  {
	register int	*sfi = ILE->lineParaSFI;
	register int	pred,cont,nbits,n,i,m;
        register int    tf = 0;
	int		ta;

/*	simplification: if ( numLayer>1 && envParaFlag ) always generate envPara	*/

	layNumBit[0] = basicNumBit;
	for (layer=1; layer<numLayer; layer++) {
	  layNumBit[layer] = ILE->numLineNumBit + ILE->layPrevNumLine[layer] - ILE->layPrevNumLine[layer-1];
	  if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag)
	    layNumBit[layer] += ILE->numLineNumBit;	/* numLinePhase */
	}

	for (layer=0; layer<numLayer; layer++)
	  layNumPhase[layer] = 0;

	nbits=0;
	chkNumBit = 0;

	il=n=layer=0;
	while ( il<numLine )
	{
		pred=lineParaPred[il];
		cont = pred && ((pred-1)<ILE->layPrevNumLine[layer]);
		if ( cont )
		{
			tmpNumBit = (FContEncode(lineParaFIndex+il) & 255)
				  + (AContEncode(lineParaAIndex+il) & 255);
		}
		else
		{
			ta=lineParaAIndex[il]; if ( pred ) ta+=prevLineParaAIndex[pred-1];
			tmpNumBit = ALNewEncode(&ta,maxAmplIndex, ILE->quantMode) & 255;
			tmpNumBit -= nbits;

			tf=lineParaFIndex[il]; if ( pred ) tf+=prevLineParaFIndex[pred-1];
			InsertSorted(sfi,n,tf);
			n++;
			for (i=m=nbits=0; i<n; i++)
			{
				nbits+=FNewEncode(n-i,ILE->maxFIndex-m,sfi[i]-m) & 255;
				m=sfi[i];
			}
			tmpNumBit += nbits;

			if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag &&
			    layNumPhase[layer]<(layer ? ILE->cfg->maxPhaseExt : ILE->cfg->maxPhaseIndi))
			  tmpNumBit += HARM_PHASE_NUMBIT;
		}
		chkNumBit += tmpNumBit;

		if ( envParaFlag && il==firstEnvLine && (numLayer==1) )
		  chkNumBit += envParaNumBit+il;
		if ( envParaFlag && ((il>=firstEnvLine) || (numLayer>1)) )
		  chkNumBit++;

		if ( layNumBit[layer] + chkNumBit > layMaxNumBit[layer] )
		{
			layNumLine[layer] = il;
			layer++;
			if ( layer >= numLayer )
			  break;

			n=ILE->layPrevNumLine[layer-1];				/* continue bits for this layer		*/
			for (i=0; i<il; i++) if ( lineParaPred[i] ) n--;	/* no cont.bit for lines already continued	*/
			layNumBit[layer] += n;

			n=nbits=0;
		}
		else
		{
			if ( cont )
			  contNumBit += tmpNumBit;
			else
			  newNumBit += tmpNumBit;

			layNumBit[layer] += chkNumBit;

			if ( !cont )
			{
				lineParaFIndex[il]=tf;
				lineParaAIndex[il]=ta;

				if ( pred )
				{
					lineParaPred[il]=0;
					lineParaIndex[il] = (long) (-1024-tf);
				}

			if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag &&
			    layNumPhase[layer]<(layer ? ILE->cfg->maxPhaseExt : ILE->cfg->maxPhaseIndi))
			  layNumPhase[layer]++;
			}

			il++;
		}
		chkNumBit=0;
	}

	if ( layer<numLayer )
	{
		layNumBit[layer]+=chkNumBit;
		for (i=layer; i<numLayer; i++) layNumLine[i] = il;
	}


	if ( ILE->debugLevel>=2 )
	{
		printf("\nL0: %i lines, %i (%i) bits\n",layNumLine[0],layNumBit[0],layNumBit[0]-basicNumBit);
		for (layer=1; layer<numLayer; layer++)
			printf("L%i: %i lines, %i bits\n",layer,layNumLine[layer],layNumBit[layer]);
	}

	basicNumBit=0;
	for (layer=0; layer<numLayer; layer++)
	  basicNumBit += layNumBit[layer];
	quantNumLine=layNumLine[0];
	quantEnvParaFlag = envParaFlag && ((layNumLine[0]>firstEnvLine) || (numLayer>1));
  }

  envNumBit = (quantEnvParaFlag)?(envParaNumBit+layNumLine[0]):0;
  if (ILE->debugLevel) {
    int bpc = frameNumBit/50+1;
    int nhl[33] = {0,1,2,3,4,5,6,7,8,9,10,12,14,16,18,20,22,25,28,31,34,37,41,
		   45,49,53,58,63,68,74,80,87,99};
    i = 0;
    for (il=0; il<layNumLine[0]; il++)
      if (lineParaPred[il])
	i++;
    printf("      hdr env   hrm %%rt #l [Hz] #p   noi %%rt #p   "
	   "cnt #l new #l   use max avl\n"
	   "bits: %3d %3d   %3d %3.0f %2d %4.0f %2d   %3d %3.0f %2d   "
	   "%3d %2d %3d %2d   %3d %3d %3d\n",
	   basicNumBit-envNumBit-harmNumBit+1-noiseNumBit+1-
	   contNumBit-newNumBit,
	   envNumBit,
	   harmNumBit-1,harmRate[0]*100.,
	   (ILE->cfg->bsFormat>=4)?
	   ((numTrans)?numHarmLineCod:0):
	   (nhl[min(32,numTrans)]),
	   (numTrans)?harmFreq[0]:0,numTrans,
	   noiseNumBit-1,noiseRate*100.,quantNumNoise,
	   contNumBit,i,
	   newNumBit,layNumLine[0]-i,
	   basicNumBit,basicMaxNumBit,frameAvailNumBit);
    printf("alloc: ");
    i = 0;
    for (; i<=(basicNumBit-envNumBit-harmNumBit+1-noiseNumBit+1-
	       contNumBit-newNumBit)/bpc; i++)
      printf("X%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=(basicNumBit-harmNumBit+1-noiseNumBit+1-contNumBit-
	       newNumBit)/bpc; i++)
      printf("^%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=(basicNumBit-noiseNumBit+1-contNumBit-newNumBit)/bpc; i++)
      printf("H%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=(basicNumBit-contNumBit-newNumBit)/bpc; i++)
      printf("/%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=(basicNumBit-newNumBit)/bpc; i++)
      printf("c%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=basicNumBit/bpc; i++)
      printf("+%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=basicMaxNumBit/bpc; i++)
      printf("O%s",(i==frameNumBit/bpc)?"|":"");
    for (; i<=frameAvailNumBit/bpc; i++)
      printf("-%s",(i==frameNumBit/bpc)?"|":"");
    printf(" \n");
  }

  if (ILE->debugLevel >= 2) {
    int i;
    printf("IndiLineEncodeFrame: "
	   "quantNumLine=%d  basicMaxNumBit=%d  basicNumBit=%d\n",
	   layNumLine[0],basicMaxNumBit,basicNumBit);
    printf("IndiLineEncodeFrame: "
	   "numHarm=%d  numHarmLine=%d  numTrans=%d  harmNumBit=%d\n",
	   numHarm,numHarmLine[0],numTrans,harmNumBit);
    printf("IndiLineEncodeFrame: "
	   "envParaFlag=%d  quantEnvParaFlag=%d  firstEnvLine=%d\n",
	   envParaFlag,quantEnvParaFlag,firstEnvLine);

    if (ILE->cfg->bsFormat>=5) {
      printf("layNumPhase[0]=%d harmNumPhase=%d",
	     layNumPhase[0],harmNumPhase);
      for (i=1;i<numLayer;i++)
	printf(" layNumPhase[%d]=%d",
	       i,layNumPhase[i]);
      printf(" \n");
    }
  }

  /* determine line transmission sequence */
  IndiLineSequence(ILE->ILQ,layNumLine[numLayer-1],lineParaPred,lineParaFIndex,&lineSeq,layNumLine,numLayer);

  /* determine continue bits for all lines transmitted in the previous frame (layer 0) */

  for (il=0; il<ILE->layPrevNumLine[0]; il++)
    ILE->lineContFlag[il] = 0;
  for (il=0; il<layNumLine[0]; il++)
    if (lineParaPred[il])
      ILE->lineContFlag[lineParaPred[il]-1] = 1;


  /*$ BE991125>>> $*/
  if ( ILE->cfg->bsFormat<6 ) {
  /*$ <<<BE991125 $*/

  /* put quantNumLine to bitstream */
  if (BsPutBit(stream,layNumLine[0],ILE->numLineNumBit))
    IndiLineExit("IndiLineEncodeFrame: error generating bit stream (numLine)");

  if ( ILE->cfg->bsFormat<4 )
  {
    /* put envelope parameters to bitstream */
    if (BsPutBit(stream,quantEnvParaFlag,1))
      IndiLineExit("IndiLineEncodeFrame: error generating bit stream (envFlag)");
    if (quantEnvParaFlag)
      if (BsPutBit(stream,envParaIndex,envParaNumBit))
        IndiLineExit("IndiLineEncodeFrame: error generating bit stream (envPara)");

    /* put continue bits to bitstream */
    for (il=0; il<ILE->layPrevNumLine[0]; il++)
      if (BsPutBit(stream,ILE->lineContFlag[il],1))
        IndiLineExit("IndiLineEncodeFrame: error generating bit stream (contFlag %d)",il);
  }

  if (ILE->cfg->bsFormat!=0) {
    /* harmonic stuff */

    if ( ILE->cfg->bsFormat<4 )
    {
      if (BsPutBit(stream,numTrans?1:0,1))
	IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm)");
      if (numTrans) {
	if (BsPutBit(stream,numTrans-1,6))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm n)");
	if (BsPutBit(stream,addHarmAmplBits,1))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm add)");
	if (BsPutBit(stream,(ILE->harmPredEnc[0]>0 && ILE->prevNumTrans>0)?1:0,1))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm c)");
	if (BsPutBit(stream,(unsigned int)harmEnv[0],1))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm e)");
	if (BsPutBit(stream,ILE->harmFreqIndex[0],11))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm f)");
	if (BsPutBit(stream,ILE->harmFreqStretchIndex[0],5))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm fs)");
	if (BsPutBit(stream,ILE->harmAmplIndex[0][0],6))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm ta0)");
	for (il=1; il<numTrans; il++)
	  if (BsPutBit(stream,ILE->harmAmplIndex[0][il],4+addHarmAmplBits))
	    IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm ta%d)",il);
      }

    /* noise stuff */

      if (BsPutBit(stream,quantNumNoise?1:0,1))
	IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise)");

      if (quantNumNoise) {
	if (BsPutBit(stream,quantNumNoise-4,numNoiseParaNumBit))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise n)");
	if (BsPutBit(stream,noiseEnvParaFlag,1))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise e)");
	if (BsPutBit(stream,noiseNormIndex,6))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise p0)");
	for (i=0; i<quantNumNoise; i++)
	  if (BsPutBit(stream,ILE->noiseParaIndex[i],noiseParaNumBit))
	    IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise p%d)",i);
	if (noiseEnvParaFlag)
	  if (BsPutBit(stream,noiseEnvParaIndex,noiseEnvParaNumBit))
	    IndiLineExit("IndiLineEncodeFrame: error generating bit stream (noise env)");
      }
    }
    else	/* bsFormat>=4	*/
    {
	register char	*t = "IndiLineEncodeFrame: error generating bit stream (harm)";

	if ( BsPutBit(stream,(numTrans>0)?1:0,1) ) IndiLineExit(t);
	if ( BsPutBit(stream,(quantNumNoise>0)?1:0,1) ) IndiLineExit(t);

	if (ILE->cfg->bsFormat>=5) {
	  if ( BsPutBit(stream,ILE->cfg->phaseFlag?1:0,1) ) IndiLineExit(t);
	  if (ILE->cfg->phaseFlag)
	    /* numLinePhase */
	    if (BsPutBit(stream,layNumPhase[0],ILE->numLineNumBit))
	      IndiLineExit("IndiLineEncodeFrame: error generating bit stream (numLinePhase)");
	}

	if ( BsPutBit(stream,maxAmplIndex>>2,4) ) IndiLineExit(t);

	/* put envelope parameters to bitstream */
	if (BsPutBit(stream,quantEnvParaFlag,1)) IndiLineExit("IndiLineEncodeFrame: error generating bit stream (envFlag)");
	if (quantEnvParaFlag)
		if (BsPutBit(stream,envParaIndex,envParaNumBit))
			IndiLineExit("IndiLineEncodeFrame: error generating bit stream (envPara)");


	/* put continue bits (layer 0) to bitstream */

	for (il=0; il<ILE->layPrevNumLine[0]; il++)
		if (BsPutBit(stream,ILE->lineContFlag[il],1))
			IndiLineExit("IndiLineEncodeFrame: error generating bit stream (contFlag %d)",il);


	if ( numTrans>0 )
	{
	        int i;
		int	c = (ILE->harmPredEnc[0]>0 && ILE->prevNumTrans>0)?1:0;

		if ( BsPutBit(stream,numTransIndex,4) ) IndiLineExit(t);
		if ( BsPutBit(stream,(unsigned int) numHarmLineIndex,5) ) IndiLineExit(t);
		if ( BsPutBit(stream,c,1) ) IndiLineExit(t);

		if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag && !c)
		  if ( BsPutBit(stream,harmNumPhase,HARMPHASEBITS) ) IndiLineExit(t);

		if ( BsPutBit(stream,(unsigned int)harmEnv[0],1) ) IndiLineExit(t);

		if ( BsPutBit(stream,ILE->harmAmplCW[0]>>8,ILE->harmAmplCW[0]&255)) IndiLineExit(t);
		if ( BsPutBit(stream,ILE->harmFreqCW[0]>>8,ILE->harmFreqCW[0]&255)) IndiLineExit(t);
		if ( PutHStretchBits(stream,ILE->harmFreqStretchQuant[0]) ) IndiLineExit(t);
		PutLPCParaBits(stream,ILE->harmLPCIndex[0]+1,numTransIndex,&LPCharmEncPar);

		/* bsf=5 phase */
		for (i=0; i<harmNumPhase; i++)
		  if (BsPutBit(stream,ILE->harmEnhaIndex[i]&((1<<HARM_PHASE_NUMBIT)-1),HARM_PHASE_NUMBIT))
		    IndiLineExit(t);

	}


	if ( quantNumNoise>0 )
	{
		if ( BsPutBit(stream,quantNumNoiseIndex,4) ) IndiLineExit(t);
		if ( BsPutBit(stream,(noisePredEnc?ILE->prevNumNoise:0)>0?1:0,1)) IndiLineExit(t);
		if ( BsPutBit(stream,noiseEnvParaFlag,1)) IndiLineExit(t);
		if ( BsPutBit(stream,ILE->noiseAmplCW>>8,ILE->noiseAmplCW&255)) IndiLineExit(t);

		if ( noiseEnvParaFlag )
			if (BsPutBit(stream,noiseEnvParaIndex,noiseEnvParaNumBit)) IndiLineExit(t);
		PutLPCParaBits(stream,ILE->noiseLPCIndex+1,quantNumNoiseIndex,&LPCnoiseEncPar);
	}
    }


  }    

  /*
  if ( quantNumNoise>0 )
  {
  	static int nc=0,nb=0;
  	nc++;
  	nb+=noiseNumBit;
  	fprintf(stderr,"N: %f\n",((double) nb)/(double) nc);
  }
  if ( numTrans>0 )
  {
  	static int nc=0,nb=0;
  	nc++;
  	nb+=harmNumBit;
  	fprintf(stderr,"H: %f\n",((double) nb)/(double) nc);
  }
  */

  /* put line parameters to bitstream */

  if ( ILE->cfg->bsFormat<4 )
  {
    for (ilTrans=0; ilTrans<quantNumLine; ilTrans++) {
      il = lineSeq[ilTrans];
      if (quantEnvParaFlag)
	if (BsPutBit(stream,ILE->lineParaEnv[il],1))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (lineEnv %d)",il);
      if (BsPutBit(stream,lineParaIndex[il],lineParaNumBit[il]))
        IndiLineExit("IndiLineEncodeFrame: error generating bit stream (linePara %d)",il);
    }
  }
  else	/* bsFormat>=4	*/
  {
  	register char	*t = "IndiLineEncodeFrame: error generating bit stream";
  	register long	c;
  	register int	m;

	for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++)
	{
		il = lineSeq[ilTrans];
		if ( !lineParaPred[il] ) break;

		if (quantEnvParaFlag) if (BsPutBit(stream,ILE->lineParaEnv[il],1)) IndiLineExit(t);
		c=FContEncode(lineParaFIndex+il); if (BsPutBit(stream,c>>8,c&255)) IndiLineExit(t);
		c=AContEncode(lineParaAIndex+il); if (BsPutBit(stream,c>>8,c&255)) IndiLineExit(t);
	}
	m=0;

	for (; ilTrans<layNumLine[0]; ilTrans++)
	{
		il = lineSeq[ilTrans];

		if ( lineParaPred[il] ) IndiLineExit("layer=0: put linebits: Sequence error");

		if (quantEnvParaFlag) if (BsPutBit(stream,ILE->lineParaEnv[il],1)) IndiLineExit(t);
		i=lineParaFIndex[il];
		c=FNewEncode(layNumLine[0]-ilTrans,ILE->maxFIndex-m,i-m);

		if (BsPutBit(stream,c>>8,c&255)) IndiLineExit(t);
		m=i;
		c=ALNewEncode(lineParaAIndex+il,maxAmplIndex, ILE->quantMode);
		if (BsPutBit(stream,c>>8,c&255)) IndiLineExit(t);
	}

	/* bsf=5 phase */
	i = 0;
	for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
	  il = lineSeq[ilTrans];
	  if (!lineParaPred[il]) {
	    if (i<layNumPhase[0])
	      if (BsPutBit(stream,lineParaEnhaIndex[il]&((1<<HARM_PHASE_NUMBIT)-1),HARM_PHASE_NUMBIT))
		IndiLineExit(t);
	    i++;
	  }
	} 

  }

  /*$ BE991125>>> $*/
  }
  else {		/*$ ILE->cfg->bsFormat>=6 $*/

    /* ---------- write ESC0 elements ---------- */

/*$#define PR_DETAIL$*/

    /* put quantNumLine to bitstream */
    if (BsPutBit(bsESC0,layNumLine[0],ILE->numLineNumBit))
      HilnBsErr("numLine",0);
    numBitESC0 += ILE->numLineNumBit;

    /* flags for harm, noise, env, phase */
    if (BsPutBit(bsESC0,(numTrans>0)?1:0,1)) HilnBsErr("harmFlag",0);
    numBitESC0 += 1;
    if (BsPutBit(bsESC0,(quantNumNoise>0)?1:0,1)) HilnBsErr("noiseFlag",0);
    numBitESC0 += 1;
    if (BsPutBit(bsESC0,quantEnvParaFlag,1)) HilnBsErr("envFlag",0);
    numBitESC0 += 1;
    if (BsPutBit(bsESC0,ILE->cfg->phaseFlag?1:0,1))
      HilnBsErr("phaseFlag",0);
    numBitESC0 += 1;
    if (BsPutBit(bsESC0,maxAmplIndex>>2,4)) HilnBsErr("maxAmplIndex",0);
    numBitESC0 += 4;
#ifdef PR_DETAIL
    printf("maxAmpl %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* most important parameters for harmonic tone */
    if (numTrans>0) {
      harmContFlag = (ILE->harmPredEnc[0]>0 && ILE->prevNumTrans>0) ? 1 : 0;
      if (BsPutBit(bsESC0,harmContFlag,1)) HilnBsErr("harmContFlag",0);
      numBitESC0 += 1;
      if (BsPutBit(bsESC0,(unsigned int)harmEnv[0],1))
	HilnBsErr("harmEnvFlag",0);
      numBitESC0 += 1;
      if (!harmContFlag) {
	if (BsPutBit(bsESC0,ILE->harmAmplCW[0]>>8,ILE->harmAmplCW[0]&255))
	  HilnBsErr("harmAmplRel",0);
	numBitESC0 += ILE->harmAmplCW[0]&255;
	if (BsPutBit(bsESC0,ILE->harmFreqCW[0]>>8,ILE->harmFreqCW[0]&255))
	  HilnBsErr("harmFreqIndex",0);
	numBitESC0 += ILE->harmFreqCW[0]&255;
      }
    }
#ifdef PR_DETAIL
    printf("harmpara0 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* most important parameters for noise component */
    if (quantNumNoise>0) {
      noiseContFlag = (noisePredEnc?ILE->prevNumNoise:0)>0?1:0;
      if (BsPutBit(bsESC0,noiseContFlag,1)) HilnBsErr("noiseContFlag",0);
      numBitESC0 += 1;
      if (BsPutBit(bsESC0,noiseEnvParaFlag,1)) HilnBsErr("noiseEnvFlag",0);
      numBitESC0 += 1;
      if(!noiseContFlag) {
	if (BsPutBit(bsESC0,ILE->noiseAmplCW>>8,ILE->noiseAmplCW&255))
	  HilnBsErr("noiseAmplRel",0);
	numBitESC0 += ILE->noiseAmplCW&255;
      }
    }
#ifdef PR_DETAIL
    printf("noisepara0 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* ---------- write ESC1 elements ---------- */

    /* elements for harmonic tone */
    if (numTrans>0) {
      if (BsPutBit(bsESC1,numTransIndex,4))
	HilnBsErr("numHarmParaIndex",1);
      numBitESC1 += 4;
      if (BsPutBit(bsESC1,(unsigned int) numHarmLineIndex,5))
	HilnBsErr("numHarmLineIndex",1);
      numBitESC1 += 5;
      if(harmContFlag) {
	if (BsPutBit(bsESC1,ILE->harmAmplCW[0]>>8,ILE->harmAmplCW[0]&255))
	  HilnBsErr("contHarmAmpl",1);
	 numBitESC1 += ILE->harmAmplCW[0]&255;
	if (BsPutBit(bsESC1,ILE->harmFreqCW[0]>>8,ILE->harmFreqCW[0]&255))
	  HilnBsErr("contHarmFreq",1);
	 numBitESC1 += ILE->harmFreqCW[0]&255;
      }
#ifdef PR_DETAIL
      printf("harmpara1a %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      /* spectral envelope parameters (more important part) */
      numBitESC1 += PutLPCParaFmt6(bsESC1,ILE->harmLPCIndex[0]+1,
				   numTransIndex,0,1,1,"harmLAR",
				   &LPCharmEncPar);
#ifdef PR_DETAIL
      printf("harmpara1b %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    }

    /* elements for noise component */
    if (quantNumNoise>0) {
      if(noiseContFlag) {		/* if(ILE->prevNumNoise>0) */
	if (BsPutBit(bsESC1,ILE->noiseAmplCW>>8,ILE->noiseAmplCW&255))
	  HilnBsErr("contNoiseAmpl",1);
	numBitESC1 += ILE->noiseAmplCW&255;
      }
#ifdef PR_DETAIL
      printf("noisepara1a %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      if (BsPutBit(bsESC1,quantNumNoiseIndex,4))
	HilnBsErr("numNoiseParaIndex",1);
      numBitESC1 += 4;
#ifdef PR_DETAIL
      printf("noisepara1b %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      /* spectral envelope parameters (more important part) */
      numBitESC1 += PutLPCParaFmt6(bsESC1,ILE->noiseLPCIndex+1,
				   quantNumNoiseIndex,0,1,1,"noiseLar",
				   &LPCnoiseEncPar);
#ifdef PR_DETAIL
      printf("noisepara1c %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    }

    /* continue bits (layer 0) */
    for (il=0; il<ILE->layPrevNumLine[0]; il++)
      if (BsPutBit(bsESC1,ILE->lineContFlag[il],1))
	HilnBsErr("prevLineContFlag",1);
    numBitESC1 += ILE->layPrevNumLine[0];
#ifdef PR_DETAIL
    printf("contFlags %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* ---------- write ESC2 elements ---------- */

    /* envelope flags of all  individual lines */
    for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (quantEnvParaFlag) {
	if (BsPutBit(bsESC2,ILE->lineParaEnv[il],1))
	  HilnBsErr("lineEnvFlag",2);
	numBitESC2 += 1;
      }
    }
#ifdef PR_DETAIL
    printf("lineEnv %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* frequencies and amplitudes for new individual lines */
    for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (!lineParaPred[il]) break;	/* index of first new line */
    }
    lfi = 0;
    for (; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (lineParaPred[il])
	IndiLineExit("layer=0: put linebits: Sequence error");

      i = lineParaFIndex[il];
      tmpCW = FNewEncode(layNumLine[0]-ilTrans,ILE->maxFIndex-lfi,i-lfi);

      if (BsPutBit(bsESC2,tmpCW>>8,tmpCW&255)) HilnBsErr("ILFreqInc",2);
      numBitESC2 += tmpCW&255;
      lfi = i;
      tmpCW = ALNewEncode(lineParaAIndex+il,maxAmplIndex, ILE->quantMode);
      if (BsPutBit(bsESC2,tmpCW>>8,tmpCW&255)) HilnBsErr("ILAmplRel",2);
      numBitESC2 += tmpCW&255;
    }
#ifdef PR_DETAIL
    printf("newlinepara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* ---------- write ESC3 elements ---------- */

    /* envelope parameters for harmonic and individual lines */
    if (quantEnvParaFlag) {
      if (BsPutBit(bsESC3,envParaIndex,envParaNumBit))
	HilnBsErr("envPara",3);
      numBitESC3 += envParaNumBit;
    }
#ifdef PR_DETAIL
    printf("envpara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* spectral envelope parameters for harmonic tone (less important part) */
    if (numTrans>0) {
      numBitESC3 += PutLPCParaFmt6(bsESC3,ILE->harmLPCIndex[0]+1,
				   numTransIndex,2,0,3,"harmLAR",
				   &LPCharmEncPar);
    }
#ifdef PR_DETAIL
    printf("harm3 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* spectral envelope parameters for noise (less important part) */
    if (quantNumNoise>0) {
      numBitESC3 += PutLPCParaFmt6(bsESC3,ILE->noiseLPCIndex+1,
				   quantNumNoiseIndex,2,0,3,"noiseLar",
				   &LPCnoiseEncPar);
    }
#ifdef PR_DETAIL
    printf("noise3 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* frequencies and amplitudes for continued individual lines */
    for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (!lineParaPred[il]) break;

      tmpCW = FContEncode(lineParaFIndex+il);
      if (BsPutBit(bsESC3,tmpCW>>8,tmpCW&255)) HilnBsErr("DILFreq",3);
      numBitESC3 += tmpCW&255;
      tmpCW = AContEncode(lineParaAIndex+il);
      if (BsPutBit(bsESC3,tmpCW>>8,tmpCW&255)) HilnBsErr("DILAmpl",3);
      numBitESC3 += tmpCW&255;
    }
#ifdef PR_DETAIL
    printf("contlinepara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* harmFreqStretch */
    if (numTrans>0) {
      numBitESC3 += PutHStretchFmt6(bsESC3,ILE->harmFreqStretchQuant[0],3);
    }
#ifdef PR_DETAIL
    printf("stretch %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* ---------- write ESC4 elements ---------- */

    /* parameters for harmonic tone */
    if (numTrans>0) {
      /* phase */
      if (ILE->cfg->phaseFlag && !harmContFlag) {
	if (BsPutBit(bsESC4,harmNumPhase,HARMPHASEBITS))
	  HilnBsErr("numHarmPhase",4);
	numBitESC4 += HARMPHASEBITS;
	for (i=0; i<harmNumPhase; i++) {
	  if (BsPutBit(bsESC4,ILE->harmEnhaIndex[i]&((1<<HARM_PHASE_NUMBIT)-1),
		       HARM_PHASE_NUMBIT))
	    HilnBsErr("harmPhase",4);
	  numBitESC4 += HARM_PHASE_NUMBIT;
	}
      }
    }
#ifdef PR_DETAIL
    printf("harm4 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* noise envelope parameters */
    if (quantNumNoise>0) {
      if (noiseEnvParaFlag) {
	if (BsPutBit(bsESC4,noiseEnvParaIndex,noiseEnvParaNumBit))
	  HilnBsErr("noiseEnvPara",4);
	numBitESC4 += noiseEnvParaNumBit;
      }
    }
#ifdef PR_DETAIL
    printf("noise4 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* individual line phase parameters */
    if (ILE->cfg->phaseFlag) {
      if (BsPutBit(bsESC4,layNumPhase[0],ILE->numLineNumBit))
	HilnBsErr("numLinePhase",4);
      numBitESC4 += ILE->numLineNumBit;
    }
    i = 0;
    for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (!lineParaPred[il]) {
	if (i<layNumPhase[0]) {
	  if (BsPutBit(bsESC4,lineParaEnhaIndex[il]&((1<<HARM_PHASE_NUMBIT)-1),
		       HARM_PHASE_NUMBIT))
	    HilnBsErr("linePhase",4);
	  numBitESC4 += HARM_PHASE_NUMBIT;
	}
	i++;
      }
    }
#ifdef PR_DETAIL
    printf("ilphase %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    if (ILE->debugLevel >= 1) {
      printf("BSF6: bits in esc01234: %4d %4d %4d %4d %4d\n",
	     numBitESC0,numBitESC1,numBitESC2,numBitESC3,numBitESC4);      
    }
  }
  /*$ <<<BE991125 $*/


  /* put enhancement parameters to bitstream */
  if (streamEnha) {
    /* envelope */
    if (quantEnvParaFlag)
      if (BsPutBit(streamEnha,envParaEnhaIndex,envParaEnhaNumBit))
	IndiLineExit("IndiLineEncodeFrame: error generating bit stream (envParaEnha)");
    /* harm */
    if(numTrans)
      for (il=0; il<min(10,numHarmLineCod); il++)
	if (BsPutBit(streamEnha,ILE->harmEnhaIndex[il],
		     ILE->harmEnhaNumBit[il]))
	  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (harm enha%d)",il);

    /* indi lines */
    for (ilTrans=0; ilTrans<layNumLine[0]; ilTrans++) {
      il = lineSeq[ilTrans];
      if (BsPutBit(streamEnha,lineParaEnhaIndex[il],lineParaEnhaNumBit[il]))
	IndiLineExit("IndiLineEncodeFrame: error generating bit stream (lineParaEnha %d)",il);
    }
  }

  for (layer=1; layer<numLayer; layer++)
  {
	if (BsPutBit(streamExt[layer-1],layNumLine[layer]-layNumLine[layer-1],ILE->numLineNumBit)) IndiLineExit("IndiLineEncodeFrame: error generating bit stream (addNumLine)");

	if (ILE->cfg->bsFormat>=5 && ILE->cfg->phaseFlag)
	    /* numLinePhase */
	    /* HP20001130 
               if (BsPutBit(stream,layNumPhase[layer],ILE->numLineNumBit)) */
	    if (BsPutBit(streamExt[layer-1],layNumPhase[layer],ILE->numLineNumBit))
	      IndiLineExit("IndiLineEncodeFrame: error generating bit stream (numLinePhase)");

	for (il=0; il<ILE->layPrevNumLine[layer]; il++) ILE->lineContFlag[il] = 0;

	for (il=0; il<layNumLine[layer-1]; il++)
		if (lineParaPred[il])
			ILE->lineContFlag[lineParaPred[il]-1] = 2;

	for (; il<layNumLine[layer]; il++)
		if (lineParaPred[il])
			ILE->lineContFlag[lineParaPred[il]-1] = 1;

	for (il=0; il<ILE->layPrevNumLine[layer]; il++) if ( ILE->lineContFlag[il]<2 )
	{
	        /* if (BsPutBit(stream,ILE->lineContFlag[il],1)) */
		if (BsPutBit(streamExt[layer-1],ILE->lineContFlag[il],1))
		  IndiLineExit("IndiLineEncodeFrame: error generating bit stream (LCF)");
	}

	{
	  	register char	*t = "IndiLineEncodeFrame: error generating bit stream";
  		register long	c;
	  	register int	m;

		for (ilTrans=layNumLine[layer-1]; ilTrans<layNumLine[layer]; ilTrans++)
		{
			il = lineSeq[ilTrans];
			if ( !lineParaPred[il] ) break;

			if (quantEnvParaFlag) if (BsPutBit(streamExt[layer-1],ILE->lineParaEnv[il],1)) IndiLineExit(t);
			c=FContEncode(lineParaFIndex+il); if (BsPutBit(streamExt[layer-1],c>>8,c&255)) IndiLineExit(t);
			c=AContEncode(lineParaAIndex+il); if (BsPutBit(streamExt[layer-1],c>>8,c&255)) IndiLineExit(t);
		}
		m=0;

		for (; ilTrans<layNumLine[layer]; ilTrans++)
		{
			il = lineSeq[ilTrans];

			if ( lineParaPred[il] ) IndiLineExit("layer>0 put linebits: Sequence error");

			if (quantEnvParaFlag) if (BsPutBit(streamExt[layer-1],ILE->lineParaEnv[il],1)) IndiLineExit(t);
			i=lineParaFIndex[il];
			c=FNewEncode(layNumLine[layer]-ilTrans,ILE->maxFIndex-m,i-m);

			if (BsPutBit(streamExt[layer-1],c>>8,c&255)) IndiLineExit(t);
			m=i;
			c=ALNewEncode(lineParaAIndex+il,maxAmplIndex, ILE->quantMode);
			if (BsPutBit(streamExt[layer-1],c>>8,c&255)) IndiLineExit(t);
		}

		/* bsf=5 phase */
		i = 0;
		for (ilTrans=layNumLine[layer-1]; ilTrans<layNumLine[layer]; ilTrans++) {
		  il = lineSeq[ilTrans];
		  if (!lineParaPred[il]) {
		    if (i<layNumPhase[layer])
		      if (BsPutBit(streamExt[layer-1],lineParaEnhaIndex[il]&((1<<HARM_PHASE_NUMBIT)-1),HARM_PHASE_NUMBIT))
			IndiLineExit(t);
		    i++;
		  }
		} 

	}
  }

  /* dequantise line parameters */

  if ( ILE->cfg->bsFormat<4 )
	  IndiLineDequant(ILE->ILQ,quantNumLine,
			  lineSeq,lineParaPred,NULL,
			  ILE->lineParaEnv,lineParaIndex,lineParaNumBit,
			  NULL, NULL,1,
			  lineParaEnhaIndex,
			  /*$NULL,$*/
			  lineParaEnhaNumBit,
			  &quantLineFreq,&quantLineAmpl,&quantLineEnv,&quantLinePred,
			  &quantLineFreqEnha,&quantLineAmplEnha,&quantLinePhase);
  else	/* bsFormat>=4	*/
	  IndiLineDequant(ILE->ILQ,layNumLine[numLayer-1],
			  lineSeq,lineParaPred,NULL,
			  ILE->lineParaEnv,lineParaIndex,lineParaNumBit,
			  lineParaFIndex,lineParaAIndex,1,
			  lineParaEnhaIndex,
			  /*$NULL,$*/
			  lineParaEnhaNumBit,
			  &quantLineFreq,&quantLineAmpl,&quantLineEnv,&quantLinePred,
			  &quantLineFreqEnha,&quantLineAmplEnha,&quantLinePhase);

  if (ILE->debugLevel >= 3) {
    printf("dequan\n");
    printf("env: tm=%7.5f atk=%7.5f dec=%7.5f\n",
	   quantEnvPara[0],quantEnvPara[1],quantEnvPara[2]);
    printf("cont: ");
    for (il=0; il<ILE->layPrevNumLine[0]; il++)
      printf("%1d",ILE->lineContFlag[il]);
    printf("\n");
    for (il=0; il<layNumLine[numLayer-1]; il++)
      printf("%2d: f=%7.1f fe=%7.1f a=%7.1f p=%5.2f e=%1d c=%2d s=%2d\n",
	     il,quantLineFreq[il],quantLineFreqEnha[il],
	     quantLineAmpl[il],quantLinePhase[il],
	     quantLineEnv[il],quantLinePred[il],lineSeq[il]);
  }

  /* save parameters in frame-to-frame memory */
  ILE->prevQuantNumLine = layNumLine[numLayer-1];	/*  quantNumLine; -> side effects?? */
  ILE->prevNumTrans = numTrans;
  ILE->prevNumNoise = quantNumNoise;



  for (il=0; il<numLayer; il++) ILE->layPrevNumLine[il]=layNumLine[il];
}


/* IndiLineEncodeFree() */
/* Free memory allocated by IndiLineEncodeInit(). */

void IndiLineEncodeFree (
  ILEstatus *ILE)		/* in: ILE status handle */
{
  IndiLineQuantFree(ILE->ILQ);
  free(ILE->lineParaSFI);
  free(ILE->lineParaEnv);
  free(ILE->lineContFlag);
  free(ILE->linePredEnc);
  free(ILE->transAmpl);
  free(ILE->harmFreqIndex);
  free(ILE->harmFreqCW);
  free(ILE->harmLPCAmpl);
  free(ILE->harmLPCAmplTmp);
  free(ILE->harmLPCPara);
  free(ILE->harmFreqStretchIndex);
  free(ILE->harmFreqStretchQuant);
  free(ILE->harmAmplIndex[0]);
  free(ILE->harmAmplIndex);
  free(ILE->harmLPCIndex[0]);
  free(ILE->harmLPCIndex);
  free(ILE->harmLPCNonLin);
  free(ILE->noiseParaIndex);
  free(ILE->noiseLPCIndex);
  free(ILE->noiseLPCNonLin);
  free(ILE->noiseLPCDeqOld);
  free(ILE);
}


/* end of indilineenc.c */
