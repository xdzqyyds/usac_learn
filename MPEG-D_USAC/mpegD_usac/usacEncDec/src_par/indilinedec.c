/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - individual lines: bitstream decoder



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


Source file: indilinedec.c


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
25-sep-96   HP    first version based on dec_par.c
26-sep-96   HP    new indiline module interfaces
19-nov-96   HP    adapted to new bitstream module
11-apr-97   HP    harmonic stuff ...
16-apr-97   BE    decoding of harmonic stuff ...
22-apr-97   HP    noisy stuff ...
23-apr-97   BE    decoding of noise stuff ...
07-may-97   HP    fixed bug calling BsGetBit(lineParaIndex[])
14-may-97   HP    fixed "cont:" debug stuff
23-jun-97   HP    improved env/harm/noise interface
24-jun-97   HP    added IndiLineDecodeHarm()
16-nov-97   HP    harm/indi cont mode
27-nov-97   HP    fixed SGI CC problem (CommonWarning())
16-feb-98   HP    improved noiseFreq
30-mar-98   BE/HP initPrevNumLine
31-mar-98   HP    completed IndiLineDecodeFree()
06-apr-98   HP    ILDconfig
06-may-98   HP    linePhaseValid
11-jun-98   HP    harmFreq optional in IndiLineDecodeHarm()
18-jun-98   HP    fixed TransfHarm / proposed noise quant
24-feb-99   NM/HP LPC noise, bsf=4
26-jan-01   BE    float -> double
30-jan-2002 HP/NM fixed noiseEnv flag
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bitstream.h"		/* bit stream module */

#include "indilinecom.h"	/* indiline common module */
#include "indilineqnt.h"	/* indiline quantiser module */
#include "indilinedec.h"	/* indiline bitstream decoder module */


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

/* status variables and arrays for individual line decoder (ILD) */

static unsigned int freqThrE[HARM_MAX_FBASE] = {1177, 1467, 1757, 2047};
static unsigned int freqThrEBsf4[HARM_MAX_FBASE] = {1243, 1511, 1779, 2047};
static int fEnhaBitsHarm[10] = {0,1,2,2,3,3,3,3,4,4};

struct ILDstatusStruct		/* ILD status handle struct */
{
  /* general parameters */
  ILQstatus *ILQ;
  ILDconfig *cfg;
  int debugLevel;
  int maxNumLine;
  int maxNumEnv;
  int maxNumHarm;
  int maxNumHarmLine;
  int maxNumNoisePara;
  int maxNumAllLine;
  int numLineNumBit;
  int envNumBit;
  int newLineNumBit;
  int contLineNumBit;
  double fSample;
  int quantMode;
  int contMode;
  int enhaFlag;
  int enhaQuantMode;
  double maxNoiseFreq;
  int sampleRateCode;
  int maxFIndex;
 
  /* arrays for return values */
  double **envPara;		/* envelope parameters */
  double *lineFreq;		/* line frequencies */
  double *lineAmpl;		/* line amplitudes */
  double *linePhase;		/* line phase values */
  int *linePhaseValid;		/* line phase validity flag */
  int *lineValid;		/* validity flag */
  int *lineEnv;			/* line envelope flags */
  int *linePred;		/* line predecessor indices */
  int *numHarmLine;
  double *harmFreq;
  double *harmFreqStretch;
  int *harmLineIdx;
  int *harmEnv;
  int *harmPred;
  double *noisePara;

  /* pointers to variable content arrays */
  unsigned int *envParaIndex;
  int *lineParaPred;
  int *lineParaEnv;
  unsigned long *lineParaIndex;
  unsigned long *lineParaEnhaIndex;
  int *lineParaNumBit;
  int *lineContFlag;
  int *lineParaFIndex;
  int *lineParaAIndex;
  double *transAmpl;
  unsigned int *harmFreqIndex;
  int  noiseContFlag;

  unsigned int *harmFreqStretchIndex;
  unsigned int **harmAmplIndex;

  int		*harmQuantFreqStretch;
  int		**harmQuantAmpl;
  double	**harmParaDeqOld;

  unsigned int *noiseParaIndex;
  int		*noiseParaQuant;
  double	*noiseParaOld;

  unsigned int *harmEnhaIndex;
  unsigned int *harmEnhaNumBit;

  int *prevCont;

  unsigned int *linePhaseIndex;
  unsigned int *harmPhaseIndex;

  /* variables for frame-to-frame memory */
  int prevNumLine;

  int prevNumIndiLine;
  int prevNumAllLine;
  int prevNumNoise;
  double *prevLineFreq;
  double *prevLineAmpl;
  int *prevLineValid;		/* validity flag */
  int prevNumHarm;
  int prevNumTrans;
  int *prevNumHarmLine;
  int *prevHarmLineIdx;

  int layPrevNumLine[MAX_LAYER];
};


/* ---------- internal functions ---------- */


/* GenILDconfig() */
/* Generate default ILDconfig. */

static ILDconfig *GenILDconfig ()
{
  ILDconfig *cfg;

  if ((cfg = (ILDconfig*)malloc(sizeof(ILDconfig)))==NULL)
    IndiLineExit("GenILDconfig: memory allocation error");
  cfg->contModeTest = -1;
  cfg->bsFormat = 4;
  cfg->noStretch = 0;
  cfg->contdf = 1.05f;
  cfg->contda = 4.0f;
  return cfg;
}


/* InvTransfHarmAmpl() */
/* Inv. Transform  harm. amplitudes from freq. dependent averaging */

static int InvTransfHarmAmpl (
  int numTrans,
  double *transAmpl,
  double *recAmpl)
{
#define MAXGRP 8
#define MAXNUMAMPL 351

  int i,j,igrp,il,idxTr;
  int numGrp[MAXGRP] = {10,6,5,4,3,2,1,33};	/* 10 groups of 1 line ... */

  il = igrp = i = idxTr = 0;
  while(idxTr<numTrans) {
    for(j=il;j<min(il+igrp+1,MAXNUMAMPL);j++)
      recAmpl[j] = transAmpl[idxTr]/sqrt((double)(igrp+1));
    idxTr++;
    il = min(il+igrp+1,MAXNUMAMPL);
    i++;
    if(i>=numGrp[igrp]) {
      i = 0;
      igrp++;
      if (igrp>=MAXGRP)
	IndiLineExit("InvTransfHarmAmpl: numTrans error");
    }
  }
  return(il);
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

static void LAR2RefCo(double *y,double *x,int n)
{
	register int	i;
	register double	h;

	if ( n<1 ) return;

	h=(double) x[0]; /* (x[0]+ep->hcp[0].qstep*(1.0-ep->hcp[0].rpos)); */
	h=exp(h);
	y[0]=(double) ((h-1.0)/(h+1.0));

	for (i=1; i<n; i++)
	{
		h=exp((double) x[i]);
		y[i]=(double) ((h-1.0)/(h+1.0));
	}
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

/* function DequantizeLPCPara */
/* -------------------------- */
/* dequantizes LPC parameter set	*/

static void DequantizeLPCPara(	double *x,		/* in/out : dequantized parameters (LARs)	*/
				int *q,			/* in     : quantized parameters		*/
				long n,			/* int    : number of parameters		*/
				TLPCEncPara *ep)	/* in     : Encoder parameters			*/

{
	register Tlar_enc_par	*hcp;
	register double		h;
	register long		i,j;

	if ( n<1 ) return;

	hcp=ep->hcp;
	for (i=0; i<n; i++)
	{
		j=q[i];

		h=(double) j;

		if ( i>=ep->zstart )
		{
			if ( j<0 ) h+=0.5-hcp->rpos;
			if ( j>0 ) h-=0.5-hcp->rpos;
		}
		else
		{
			if ( j<0 ) h+=1.0-hcp->rpos; else h+=hcp->rpos;
			if ( hcp->xbits>0 ) h/=(double) (1<<hcp->xbits);
		}

		h*=hcp->qstep;

		x[i]+=h;

		if ( i<ep->nhcp-1 ) hcp++;
	}
}

static long GetHStretchBits(HANDLE_BSBITSTREAM stream,int *s)
{
	unsigned int	x,vz;

	if ( BsGetBitInt(stream,&x,1) ) return -1;
	if ( x==0 ) { *s=0; return 0; }
	if ( BsGetBitInt(stream,&vz,1) ) return -1;
	if ( BsGetBitInt(stream,&x,1) ) return -1;
	if ( x==0 ) x=1; else 
	{
		if ( BsGetBitInt(stream,&x,4) ) return -1;
		x+=2;
	}
	*s = vz ? (-((int) x)) : ((int) x);
	return 0;
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

static void LPCParaToLineAmpl(double *a,long m,double *x,long n,double omega0)
{
	double		h[LPCMaxNumPara];
	register double	ps,omega;
	register double	pf;
	register long	i;

	/* printf("NIKO: %7.3f\n",x[1]); */


	/* n--; */
	for (i=0; i<n; i++)
	{
		register double k = (double) x[i+1];

		/* quark ...
		if (i==0)
		{
			register double d = (double) (LPCharmEncPar.hcp[2].qstep*(1.0-LPCharmEncPar.hcp[2].rpos));
			if ( k<0.0 ) k-=d; else k+=d;
		}
		... */

		k=exp(k);
		k=(k-1.0)/(k+1.0);
		h[i]=(double) k;
	}

	LPCconvert_k_to_h(h,n);

	ps=0.0;
	for (i=0,omega=omega0; i<m; omega+=omega0,i++)
	{
		register double	ha = PWRSpec(h,n,omega);

		if ( ha>1.0/(lpc_max_amp*lpc_max_amp) ) ha=1.0/ha; else ha=1.0/(lpc_max_amp*lpc_max_amp);

		a[i]=(double) sqrt(ha);

		ps+=ha;
	}

	pf=(double) sqrt(((double) (x[0]*x[0]))/(0.5*ps));

	for (i=0; i<m; i++) a[i]*=pf;
}


static int FContDecode(HANDLE_BSBITSTREAM s)
{
	register char	*t = "indilinedec.c: FContDecode error";
	unsigned int	x,y,z;

	if ( BsGetBitInt(s,&x,2)) IndiLineExit(t);

	switch ( x ) {
		case 0 : return 0;
		case 1 : if ( BsGetBitInt(s,&y,1)) IndiLineExit(t);
			 return 1-(y<<1);
		case 2 : if ( BsGetBitInt(s,&y,2)) IndiLineExit(t);
			 if ( y&2 ) return -2-(y&1); else return 2+(y&1);
		case 3 : if ( BsGetBitInt(s,&y,3)) IndiLineExit(t);
			 switch ( y>>1 ) {
				case 0 : if ( y&1 ) return -4; else return 4;
				case 1 : if ( BsGetBitInt(s,&z,1)) IndiLineExit(t);
					 if ( y&1 ) return -5-z; else return 5+z;
				case 2 : if ( BsGetBitInt(s,&z,2)) IndiLineExit(t);
					 if ( y&1 ) return -7-z; else return 7+z;
				case 3 : if ( BsGetBitInt(s,&z,5)) IndiLineExit(t);
					 if ( y&1 ) return -11-z; else return 11+z;
			 }
	}
	return 0;
}

static int FHContDecode(HANDLE_BSBITSTREAM s)
{
	register char	*t = "indilinedec.c: FContDecode error";
	unsigned int	x,y;

	if ( BsGetBitInt(s,&x,2)) IndiLineExit(t);

	switch ( x ) {
		case 0 : return 0;
		case 1 : if ( BsGetBitInt(s,&y,1)) IndiLineExit(t);
			 return 1-(y<<1);
		case 2 : if ( BsGetBitInt(s,&y,3)) IndiLineExit(t);
			 if ( y&4 ) return -2-(y&3); else return 2+(y&3);
		case 3 : if ( BsGetBitInt(s,&y,7)) IndiLineExit(t);
			 if ( y&64 ) return -6-(y&63); else return 6+(y&63);
	}
	return 0;
}

static int AContDecode(HANDLE_BSBITSTREAM s)
{
	register char	*t = "indilinedec.c: AContDecode error";
	unsigned int	x,y;

	if ( BsGetBitInt(s,&x,3)) IndiLineExit(t);

	switch ( x ) {
		case 0 : return 0;
		case 1 : return 1;
		case 2 : return -1;
		case 3 : if ( BsGetBitInt(s,&y,1)) IndiLineExit(t);
			 if ( y ) return -2; else return 2;
		case 4 : if ( BsGetBitInt(s,&y,1)) IndiLineExit(t);
			 if ( y ) return -3; else return 3;
		case 5 : if ( BsGetBitInt(s,&y,2)) IndiLineExit(t);
			 if ( y& 2 ) return  -4-(y& 1); else return  4+(y& 1);
		case 6 : if ( BsGetBitInt(s,&y,3)) IndiLineExit(t);
			 if ( y& 4 ) return  -6-(y& 3); else return  6+(y& 3);
		case 7 : if ( BsGetBitInt(s,&y,5)) IndiLineExit(t);
			 if ( y&16 ) return -10-(y&15); else return 10+(y&15);
	}
	return 0;
}

static long SDCDecode(HANDLE_BSBITSTREAM s,int k,int *tab)
{
	register char	*t = "indilinedec.c: SDCDecode error";
	register int	*pp;
	register int	g,dp;
	register int	min,max;
	unsigned int	x;

	min=0;
	max=k-1;

	pp=tab+(1<<(5-1));
	dp=1<<(5-1);

	while ( min!=max )
	{
		if ( dp ) g=(k*(*pp))>>10; else g=(max+min)>>1;
		dp>>=1;
		if ( BsGetBitInt(s,&x,1)) IndiLineExit(t);
		if ( x==0 ) { pp-=dp; max=g; } else { pp+=dp; min=g+1; }
	}
	return max;
}

static long FNewDecode(HANDLE_BSBITSTREAM s,long n,long k)
{
	n--;
	if ( n>7 ) n=7;
	return SDCDecode(s,k,sdcILFTab[n]);
}

static int ALNewDecode(HANDLE_BSBITSTREAM s,int max,int fine)
{
	if ( fine )
	{
		return max+SDCDecode(s,MAXAMPLDIFF,sdcILATab);
	}
	else
	{
		return max+2*SDCDecode(s,MAXAMPLDIFF>>1,sdcILATab);
	}
}

static int ANNewDecode(HANDLE_BSBITSTREAM s)
{
	register char	*t = "indilinedec.c: ANNewDecode error";
	unsigned int	x;

	if ( BsGetBitInt(s,&x,6)) IndiLineExit(t);
	return x;
}

static int AHNewDecode(HANDLE_BSBITSTREAM s)
{
	register char	*t = "indilinedec.c: AHNewDecode error";
	unsigned int	x;

	if ( BsGetBitInt(s,&x,6)) IndiLineExit(t);
	return x;
}

/*$ BE991125>>> $*/


/* function GetDiff */
/* ------------------ */
/* calculates the difference of two integer values, */
/* overwrites the first value by the second */
static int GetDiff(int *val1, int val2)
{
  int diff;

  diff = val2-*val1;
  *val1 = val2;
  return(diff);
}


/* function HilnBsErr */
/* ------------------ */
/* exits with an error message specifying syntax element (elName) */
/* and error sensitivity class (esc)*/
static void HilnBsErr(char *elName, int esc)
{
  IndiLineExit("HILN: error reading bit stream (%s) for ESC %d",
	       elName,esc);
}


/* function DecodeLPCFmt6 */
/* ---------------------- */
/* gets LPC parameter set from stream	*/
/* (only with index>=si and index<=ei) */

static void DecodeLPCFmt6(HANDLE_BSBITSTREAM stream,	/* in: the bit stream */
			  int *q,		/* out: quantized parameters */
			  int n,		/* in : number of parameters */
			  int si,		/* in: start index */
			  int ei,		/* in: end index (0=no lim.) */
			  int esc,		/* in: error sensit. class */
			  char *elName,		/* in: bs elem. name */
			  TLPCEncPara *ep)	/* in : Encoder parameters */

{
  register Tlar_enc_par	*hcp;
  register long		i,j;
  unsigned int		x,y;

  if(ei==0) ei = n-1;
  if(ei>=n) ei = n-1;

  for (i=si; i<=ei; i++) {
    if (i<ep->nhcp) hcp=ep->hcp+i; else hcp=ep->hcp+ep->nhcp-1;

    if (BsGetBitInt(stream,&x,1)) HilnBsErr(elName,esc);
    j=0;
    if ( i>=ep->zstart ) {
      if (x==0) {
	q[i]=0;
	continue;
      }
      else {
	if (BsGetBitInt(stream,&x,1)) HilnBsErr(elName,esc);
	j=1-x;
      }
    }

    for (;;j++) {
      if (BsGetBitInt(stream,&y,1)) HilnBsErr(elName,esc);
      if (y) break;
    }
    if (x) j=-1-j;

    if (hcp->xbits>0) {
      if (BsGetBitInt(stream,&x,hcp->xbits)) HilnBsErr(elName,esc);
      j=(j<<hcp->xbits)+x;
    }
    q[i]=j;
  }
}


/*$ <<<BE991125 $*/


/* ---------- functions ---------- */

/* IndiLineDecodeInit() */
/* Init individual lines bitstream decoder. */

ILDstatus *IndiLineDecodeInit (
  HANDLE_BSBITSTREAM hdrStream,	/* in: bit stream header */
  double maxLineAmpl,		/* in: max line amplitude */
  int initPrevNumLine,		/* in: number ind. lines in prev. frame */
  ILDconfig *cfg,		/* in: ILD configuration */
				/*     or NULL */
  int debugLevel,		/* in: debug level (0=off) */
  int *maxNumLine,		/* out: max num lines in bitstream */
  int *maxNumEnv,		/* out: max num envelopes */
  int *maxNumHarm,		/* out: max num harmonic tones */
  int *maxNumHarmLine,		/* out: max num lines of harmonic tone */
  int *maxNumNoisePara,		/* out: max num noise parameters */
  int *maxNumAllLine,		/* out: max num harm+indi lines */
  int *enhaFlag,		/* out: enha flag (0=basic 1=enha) */
  int *frameLenQuant,		/* out: samples per frame */
				/*      used for quantiser selection */
  double *fSampleQuant)		/* out: sampling frequency */
				/*      used for quantiser selection */
				/* returns: ILD status handle */
{
  ILDstatus *ILD;
  long hdrFSample;
  int hdrFrameNumSample;
  int hdrFrameLengthBase = -1;
  int hdrFrameLength;
  int i;

  if ((ILD = (ILDstatus*)malloc(sizeof(ILDstatus)))==NULL)
    IndiLineExit("IndiLineDecodeInit: memory allocation error");

  ILD->cfg = GenILDconfig();

  if (cfg != NULL)
    memcpy(ILD->cfg,cfg,sizeof(ILDconfig));

  ILD->debugLevel = debugLevel;

  /* get header info bits from header */
  if (ILD->cfg->bsFormat < 2) {
    /* old header */
    if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->maxNumLine,16) ||
	BsGetBit(hdrStream,(unsigned long*)&hdrFSample,32) ||
	BsGetBitInt(hdrStream,(unsigned int*)&hdrFrameNumSample,16))
      IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
  }
  else {
    /* CD header */
    if ( ILD->cfg->bsFormat<4 )
    {
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->quantMode,3))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->maxNumLine,8))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&hdrFrameLengthBase,1))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&hdrFrameLength,12))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->contMode,2))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->enhaFlag,1))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (ILD->enhaFlag) {
	if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->enhaQuantMode,2))
	  IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      }
      else
        ILD->enhaQuantMode = 0;
      hdrFSample = hdrFrameLengthBase ? 44100 : 48000;
      hdrFrameNumSample = hdrFrameLength;
      ILD->maxFIndex = 0;
    }
    else
    {
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->quantMode,1))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->maxNumLine,8))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->sampleRateCode,4))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&hdrFrameLength,12))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->contMode,2))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->enhaFlag,1))
	IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      if (ILD->enhaFlag) {
	if (BsGetBitInt(hdrStream,(unsigned int*)&ILD->enhaQuantMode,2))
	  IndiLineExit("IndiLineDecodeInit: error reading bit stream header");
      }
      else
        ILD->enhaQuantMode = 0;
      hdrFSample = sampleRateTable[ILD->sampleRateCode];
      hdrFrameNumSample = hdrFrameLength;
      ILD->maxFIndex = maxFIndexTable[ILD->sampleRateCode];
    }
  }

  ILD->fSample = hdrFSample;	/* to be used by noise */

  ILD->maxNumEnv = 2;
  ILD->maxNumHarm = 1;
  ILD->maxNumHarmLine = 351;
  ILD->maxNumNoisePara = 256;

  /* calc num bits required for numLine in bitstream */
  ILD->numLineNumBit = 0;
  while (1<<ILD->numLineNumBit <= ILD->maxNumLine)
    ILD->numLineNumBit++;
  
  if (ILD->cfg->contModeTest != -1)
    ILD->contMode = ILD->cfg->contModeTest;

  if (ILD->cfg->bsFormat < 2) {
    if (hdrFSample <= 8000)
      ILD->quantMode = 0;
    else if (hdrFSample <= 16000)
      ILD->quantMode = -1;
    else
      ILD->quantMode = 2;
    ILD->ILQ = IndiLineQuantInit(ILD->quantMode,0,maxLineAmpl,
				 ILD->maxNumLine,0,ILD->maxFIndex,debugLevel,ILD->cfg->bsFormat);
  }
  else
    ILD->ILQ = IndiLineQuantInit(ILD->quantMode,ILD->enhaQuantMode,maxLineAmpl,
				 ILD->maxNumLine,0,ILD->maxFIndex,
				 max(0,debugLevel-1),ILD->cfg->bsFormat);
  /*
    ILD->ILQ = IndiLineQuantInit(hdrFrameNumSample,(double)hdrFSample,maxLineAmpl,
    ILD->maxNumLine,0,debugLevel,ILD->cfg->bsFormat);
    */

  if ( ILD->cfg->bsFormat<4 ) {
    switch (ILD->quantMode) {
    case 0:
    case 1:
      ILD->maxNoiseFreq = 4000.;
      break;
    case 2:
    case 3:
      ILD->maxNoiseFreq = 8000.;
      break;
    case 4:
    case 5:
      ILD->maxNoiseFreq = 20000.;
      break;
    }
  }
  else ILD->maxNoiseFreq = 0.5*((double) hdrFSample);
  
  IndiLineNumBit(ILD->ILQ,&ILD->envNumBit,&ILD->newLineNumBit,
		 &ILD->contLineNumBit);

  if (debugLevel >= 1) {
    printf("IndiLineDecodeInit: envNumBit=%d  newLineNumBit=%d  "
	   "contLineNumBit=%d\n",
	   ILD->envNumBit,ILD->newLineNumBit,ILD->contLineNumBit);
    if ( ILD->cfg->bsFormat<4 )
      printf("IndiLineDecodeInit: frmLenBase=%d  frmLen=%d  (%8.6f)\n",
	     hdrFrameLengthBase,hdrFrameLength,
	     hdrFrameLength/(hdrFrameLengthBase ? 44100.0 : 48000.0));
    else
      printf("IndiLineDecodeInit: frmLen=%d  sampleRate=%d  (%8.6f)\n",
	     hdrFrameLength,sampleRateTable[ILD->sampleRateCode],
	     hdrFrameLength/(double)sampleRateTable[ILD->sampleRateCode]);
    printf("IndiLineDecodeInit: quantMode=%d  contMode=%d  "
	   "enhaFlag=%d  enhaQuantMode=%d\n",
	   ILD->quantMode,ILD->contMode,ILD->enhaFlag,ILD->enhaQuantMode);
  }

  ILD->maxNumAllLine = ILD->maxNumLine+ILD->maxNumHarm*ILD->maxNumHarmLine;

  if (
      /* return values */
      (ILD->envPara=(double**)malloc(ILD->maxNumEnv*sizeof(double*)))==NULL ||
      (ILD->envPara[0]=(double*)malloc(ILD->maxNumEnv*ENVPARANUM*
				      sizeof(double)))==NULL ||
      (ILD->lineFreq=(double*)malloc(ILD->maxNumAllLine*sizeof(double)))==NULL ||
      (ILD->lineAmpl=(double*)malloc(ILD->maxNumAllLine*sizeof(double)))==NULL ||
      (ILD->lineValid=(int*)malloc(ILD->maxNumAllLine*
				       sizeof(double)))==NULL ||
      (ILD->linePhase=(double*)malloc(ILD->maxNumAllLine*
				     sizeof(double)))==NULL ||
      (ILD->linePhaseValid=(int*)malloc(ILD->maxNumAllLine*
				     sizeof(int)))==NULL ||
      (ILD->lineEnv=(int*)malloc(ILD->maxNumAllLine*sizeof(int)))==NULL ||
      (ILD->linePred=(int*)malloc(ILD->maxNumAllLine*sizeof(int)))==NULL ||
      (ILD->numHarmLine=(int*)malloc(ILD->maxNumHarm*sizeof(int)))==NULL ||
      (ILD->harmFreq=(double*)malloc(ILD->maxNumHarm*sizeof(double)))==NULL ||
      (ILD->harmFreqStretch=(double*)malloc(ILD->maxNumHarm*
					   sizeof(double)))==NULL ||
      (ILD->harmLineIdx=(int*)malloc(ILD->maxNumHarm*sizeof(int)))==NULL ||
      (ILD->harmEnv=(int*)malloc(ILD->maxNumHarm*sizeof(int)))==NULL ||
      (ILD->harmPred=(int*)malloc(ILD->maxNumHarm*sizeof(int)))==NULL ||
      (ILD->noisePara=(double*)malloc(ILD->maxNumNoisePara*
				     sizeof(double)))==NULL ||
      (ILD->noiseParaOld=(double*)malloc(ILD->maxNumNoisePara*sizeof(double)))==NULL ||

      /* variable content arrays */
      (ILD->envParaIndex=(unsigned int*)malloc(ILD->maxNumEnv*
					       sizeof(unsigned int)))==NULL ||
      (ILD->lineParaPred=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->lineParaEnv=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->lineParaIndex=(unsigned long*)malloc(ILD->maxNumLine*
					       sizeof(unsigned long)))==NULL ||
      (ILD->lineParaEnhaIndex=(unsigned long*)malloc(ILD->maxNumLine*
					       sizeof(unsigned long)))==NULL ||
      (ILD->lineParaNumBit=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->lineContFlag=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->lineParaFIndex=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->lineParaAIndex=(int*)malloc(ILD->maxNumLine*sizeof(int)))==NULL ||
      (ILD->transAmpl=(double*)malloc(ILD->maxNumHarmLine*
				     sizeof(double)))==NULL ||
      (ILD->harmFreqIndex=(unsigned int*)malloc(ILD->maxNumHarm*
						sizeof(unsigned int)))==NULL ||
      (ILD->harmFreqStretchIndex=(unsigned int*)malloc(ILD->maxNumHarm*
					        sizeof(unsigned int)))==NULL ||
      (ILD->harmAmplIndex=(unsigned int**)malloc(ILD->maxNumHarm*
					       sizeof(unsigned int*)))==NULL ||
      (ILD->harmAmplIndex[0]=(unsigned int*)malloc(ILD->maxNumHarm*
						   ILD->maxNumHarmLine*
						sizeof(unsigned int)))==NULL ||
	(ILD->harmQuantFreqStretch=(int *) malloc(ILD->maxNumHarm*sizeof(int)))==NULL ||
	(ILD->harmQuantAmpl=(int **) malloc((ILD->maxNumHarm+1)*sizeof(int*)))==NULL ||
	(ILD->harmQuantAmpl[0]=(int *) malloc(ILD->maxNumHarm*LPCMaxNumPara*sizeof(int)))==NULL ||
	(ILD->harmParaDeqOld   =(double **) malloc((ILD->maxNumHarm+1)*sizeof(double *)))==NULL ||
	(ILD->harmParaDeqOld[0]=(double  *) malloc(ILD->maxNumHarm*LPCMaxNumPara*sizeof(double)))==NULL ||

      (ILD->noiseParaIndex=(unsigned int*)malloc(ILD->maxNumNoisePara*
						sizeof(unsigned int)))==NULL ||
      (ILD->noiseParaQuant=(int *) malloc(LPCMaxNumPara*sizeof(int)))==NULL ||
      (ILD->harmEnhaIndex=(unsigned int*)malloc(ILD->maxNumHarmLine*
						sizeof(unsigned int)))==NULL ||
      (ILD->harmEnhaNumBit=(unsigned int*)malloc(ILD->maxNumHarmLine*
						sizeof(unsigned int)))==NULL ||
      (ILD->prevCont=(int*)malloc(ILD->maxNumAllLine*sizeof(int)))==NULL ||
      (ILD->linePhaseIndex=(unsigned int*)malloc(ILD->maxNumAllLine*
						sizeof(unsigned int)))==NULL ||
      (ILD->harmPhaseIndex=(unsigned int*)malloc(ILD->maxNumHarmLine*
						sizeof(unsigned int)))==NULL ||
      /* frame-to-frame memory */
      (ILD->prevLineFreq=(double*)malloc(ILD->maxNumAllLine*
					sizeof(double)))==NULL ||
      (ILD->prevLineAmpl=(double*)malloc(ILD->maxNumAllLine*
					sizeof(double)))==NULL ||
      (ILD->prevLineValid=(int*)malloc(ILD->maxNumAllLine*
					sizeof(double)))==NULL ||
      (ILD->prevNumHarmLine=(int*)malloc(ILD->maxNumHarm*
					 sizeof(int)))==NULL ||
      (ILD->prevHarmLineIdx=(int*)malloc(ILD->maxNumHarm*
					 sizeof(int)))==NULL)
    IndiLineExit("IndiLineDecodeInit: memory allocation error");

  for (i=1;i<ILD->maxNumEnv;i++) {
    ILD->envPara[i] = ILD->envPara[0]+i*ENVPARANUM;
  }

  for (i=1;i<ILD->maxNumHarm;i++) {
    ILD->harmAmplIndex[i] = ILD->harmAmplIndex[0]+i*ILD->maxNumHarmLine;
  }
  for ( i = 0; i< ILD->maxNumHarm; i++ ) {
	ILD->harmQuantAmpl[i]  = ILD->harmQuantAmpl[0]+i*LPCMaxNumPara;
	ILD->harmParaDeqOld[i] = ILD->harmParaDeqOld[0]+i*LPCMaxNumPara;
	ILD->harmParaDeqOld[i][0]=0.0;
	ResetLPCPara(ILD->harmParaDeqOld[i]+1,0,LPCMaxNumPara-1,&LPCharmEncPar);
  }
  ILD->noiseParaOld[0]=0.0;
  ResetLPCPara(ILD->noiseParaOld+1,0,LPCMaxNumPara-1,&LPCnoiseEncPar);

  /* clear frame-to-frame memory */
  ILD->prevNumLine = initPrevNumLine;
  for(i=0;i<initPrevNumLine;i++)
    ILD->prevLineValid[i] = 0;	/* no parameters available for cont. */
  ILD->prevNumIndiLine = 0;
  ILD->prevNumAllLine = 0;
  ILD->prevNumHarm = 0;
  ILD->prevNumTrans = 0;
  ILD->prevNumNoise=0;

  for (i=0; i<MAX_LAYER; i++) ILD->layPrevNumLine[i]=0;


  *maxNumLine = ILD->maxNumLine;
  *maxNumEnv = ILD->maxNumEnv;
  *maxNumHarm = ILD->maxNumHarm;
  *maxNumHarmLine = ILD->maxNumHarmLine;
  *maxNumNoisePara = ILD->maxNumNoisePara;
  *maxNumAllLine = ILD->maxNumAllLine;
  *enhaFlag = ILD->enhaFlag;

  *frameLenQuant = hdrFrameNumSample;
  *fSampleQuant = hdrFSample;

  return ILD;
}


/* IndiLineDecodeFrame() */
/* Decode individual lines frame from bitstream. */

void IndiLineDecodeFrame (
  ILDstatus *ILD,		/* in: ILD status handle */
  HANDLE_BSBITSTREAM streamESC[5],	/* in: bit stream (ESC 0..4) */
  HANDLE_BSBITSTREAM streamEnha,	/* in: enhancement bit stream */
				/*     or NULL for basic mode */
  int numExtLayer,		/* in: num extension layers */
  HANDLE_BSBITSTREAM *streamExt,	/* in: extension layer bit stream(s) */
				/*     [0..numExtLayer-1] */
				/*     or NULL */
  int *numEnv,			/* out: num envelopes */
  double ***envPara,		/* out: envelope parameters */
				/*      [0..numEnv-1][0..ENVPARANUM-1] */
  int *numLine,			/* out: num lines */
  double **lineFreq,		/* out: line frequencies */
				/*      [0..numLine-1] */
  double **lineAmpl,		/* out: line amplitudes */
				/*      [0..numLine-1] */
  double **linePhase,		/* out: line phases */
				/*      [0..numLine-1] */
				/*      (not used in basic mode) */
  int **linePhaseValid,		/* out: line phases validity flag */
				/*      [0..numLine-1] */
				/*      (not used in basic mode) */
  int **lineEnv,		/* out: line envelope flags */
				/*      [0..numLine-1] */
  int **linePred,		/* out: line predecessor indices */
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
				/*      idx=harmLineIdx+ */
				/*          (0..numHarmLine-1) */
  int **harmEnv,		/* out: harm envelope flag/idx */
				/*      [0..numHarm-1] */
  int **harmPred,		/* out: harm tone predecessor idx */
				/*      [0..numHarm-1] */
  int *numNoisePara,		/* out: num noise parameter */
  double *noiseFreq,		/* out: max noise freq (bandwidth) [Hz] */
  double **noisePara,		/* out: noise parameter */
				/*      DCT[] / StdDev+RefCoef[] */
				/*      [0..numNoisePara-1] */
  int *noiseEnv)		/* out: noise envelope flag/idx */
{
  int i,il;
  int contNumLine;
  int envParaFlag;
  int envParaNumBit;
  int envParaEnhaNumBit;
  int *lineParaEnhaNumBit;
  unsigned int envParaEnhaIndex,harmFlag,noiseFlag;

  double *tmpLineFreq,*tmpLineAmpl;
  double *tmpLineFreqEnha,*tmpLineAmplEnha,*tmpLinePhaseEnha;
  int *tmpLineEnv,*tmpLinePred;
  double *tmpEnvPara,*tmpEnvParaEnha;

  int numTrans,amplInt,addHarmAmplBits,harmAmplMax,harmAmplOff;
  int maxAmplIndex = 0;

  unsigned int noiseNormIndex;
  double noiseNorm,hfreqrs;
  int numNoiseParaNumBit,noiseParaSteps;
  int noiseParaNumBit = 0;
  int fNumBitE,fNumBitBase,freqNumStepE,fintE,pintE;
  int layer;
  int numLayer = numExtLayer+1;
  int layNumLine[MAX_LAYER];

  unsigned int phaseFlag = 0;
  unsigned int layNumPhase[MAX_LAYER];
  unsigned int harmNumPhase;

  int staBit,tmpBit,envBit;
  int hdrBit = 0;
  int hrmBit = 0;
  int noiBit = 0;
  int cntBit = 0;
  int newBit = 0;
  int useBit = 0;
  int avlBit = 0;
  int frmBit;

  /*$ BE991125>>> $*/
  HANDLE_BSBITSTREAM bsESC0,bsESC1,bsESC2,bsESC3,bsESC4,stream;
  int numBitESC0,numBitESC1,numBitESC2,numBitESC3,numBitESC4;
  int harmContFlag = 0;
  int lfi;
  int bitPos=0;
  unsigned int tmpCW;
  /* HP20010214 indilineenc streamESC vs old bsFormat */
  stream = streamESC[0]; /* NULL; */ /* legacy old bsFormat */


  bsESC0 = streamESC[0];
  bsESC1 = streamESC[1];
  bsESC2 = streamESC[2];
  bsESC3 = streamESC[3];
  bsESC4 = streamESC[4];
  numBitESC0 = numBitESC1 = numBitESC2 = numBitESC3 = numBitESC4 = 0;
  /*$ <<<BE991125 $*/

  /*$ BE991125>>> $*/
  if ( ILD->cfg->bsFormat<6 ) {
  /*$ <<<BE991125 $*/

  staBit = BsCurrentBit(stream);

  GetDiff(&bitPos,BsCurrentBit(stream));	/* init. bitPos */

  /* get numLine from bitstream */
  if (BsGetBitInt(stream,(unsigned int*)numLine,ILD->numLineNumBit))
    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (numLine)");
  numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));

  layNumLine[0] = *numLine;

  if ( ILD->cfg->bsFormat>=4 )
  {
    unsigned int	x;

    if ( BsGetBitInt(stream,&harmFlag ,1) ) IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm flag)");
    numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
    if ( BsGetBitInt(stream,&noiseFlag,1) ) IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise flag)");
    numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));

    if (ILD->cfg->bsFormat>=5) {
      if ( BsGetBitInt(stream,&phaseFlag,1) ) IndiLineExit("IndiLineDecodeFrame: error reading bit stream (phase flag)");
      numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
      if (phaseFlag) {
	if ( BsGetBitInt(stream,&(layNumPhase[0]),ILD->numLineNumBit) ) IndiLineExit("IndiLineDecodeFrame: error reading bit stream (num line phase)");
	numBitESC4 += GetDiff(&bitPos,BsCurrentBit(stream));
      }
      else
	layNumPhase[0] = 0;
    }
    else
      layNumPhase[0] = 0;

    if ( BsGetBitInt(stream,&x,4) ) IndiLineExit("IndiLineDecodeFrame: error reading bit stream (maxAmplIndex)");
    numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
    maxAmplIndex = x<<2;
  }

  /* get envelope parameters from bitstream */
  if (BsGetBitInt(stream,(unsigned int*)&envParaFlag,1))
    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (envFlag)");
  numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
  if (envParaFlag) {
    envParaNumBit = ILD->envNumBit;
    if (BsGetBitInt(stream,&ILD->envParaIndex[0],envParaNumBit))
      IndiLineExit("IndiLineDecodeFrame: error reading bit stream (envPara)");
    numBitESC3 += GetDiff(&bitPos,BsCurrentBit(stream));
  }
  else
    envParaNumBit = 0;
  *numEnv = 1;

  if(ILD->prevNumLine != PNL_UNKNOWN) {

    /* get continue bits from bitstream */
    for (il=0; il<ILD->layPrevNumLine[0]; il++) {
      if (BsGetBitInt(stream,(unsigned int*)&ILD->lineContFlag[il],1))
	IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
		     "(contFlag %d)",il);
      numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
    }

    hdrBit = BsCurrentBit(stream)-staBit;

    if (ILD->cfg->bsFormat==0) {
      numTrans=0;
      *numHarm=0;
      *numNoisePara=0;
    }
    else
    {
      if ( ILD->cfg->bsFormat<4 ) {

	/* harmonic stuff */
	if (BsGetBitInt(stream,(unsigned int*)&numTrans,1))
	  IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm)");
	*numHarm = 0;
	if (numTrans) {
	  if (BsGetBitInt(stream,(unsigned int*)&numTrans,6))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm n)");
	  numTrans++;
	  if (BsGetBitInt(stream,(unsigned int*)&addHarmAmplBits,1))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm add)");
	  if (BsGetBitInt(stream,(unsigned int*)&ILD->harmPred[*numHarm],1))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm c)");
	  if (BsGetBitInt(stream,(unsigned int*)&ILD->harmEnv[*numHarm],1))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm e)");
	  if (envParaFlag==0 && ILD->harmEnv[*numHarm]==1)
	    IndiLineExit("IndiLineDecodeFrame: harm - no env paras available");
	  if (envParaFlag==0 && ILD->harmEnv[*numHarm]==1)
	    harmEnv = 0;
	  if (BsGetBitInt(stream,&ILD->harmFreqIndex[*numHarm],11))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm f)");
	  if (BsGetBitInt(stream,&ILD->harmFreqStretchIndex[*numHarm],5))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm fs)");
	  if (BsGetBitInt(stream,&ILD->harmAmplIndex[*numHarm][0],6))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm ta0)");
	  for (il=1; il<numTrans; il++)
	    if (BsGetBitInt(stream,&ILD->harmAmplIndex[*numHarm][il],4+addHarmAmplBits))
	      IndiLineExit("IndiLineDecodeFrame: error reading bit stream (harm ta%d)",il);
	  (*numHarm)++;
	}

	hrmBit = BsCurrentBit(stream)-staBit-hdrBit;

	/* noise stuff */
	if (BsGetBitInt(stream,(unsigned int*)numNoisePara,1))
	  IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise)");
	if (*numNoisePara) {
	  numNoiseParaNumBit = 2;
	  noiseParaNumBit = 3;
	  if (ILD->cfg->bsFormat>=3) {
	    numNoiseParaNumBit = 4;
	    noiseParaNumBit = 4;
	  }
	  if (BsGetBitInt(stream,(unsigned int*)numNoisePara,numNoiseParaNumBit))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise n)");
	  *numNoisePara += 4;
	  if (BsGetBitInt(stream,(unsigned int*)noiseEnv,1))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise e)");
	  *noiseEnv = (*noiseEnv)?2:0;  /* HP/NM 20020130 */
	  if (BsGetBitInt(stream,&noiseNormIndex,6))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise p0)");
	  for (i=0; i<*numNoisePara; i++)
	    if (BsGetBitInt(stream,ILD->noiseParaIndex+i,noiseParaNumBit))
	      IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise p%d)",i);
	  if(*noiseEnv) {
	    if (BsGetBitInt(stream,&ILD->envParaIndex[*numEnv],ILD->envNumBit))
	      IndiLineExit("IndiLineDecodeFrame: error reading bit stream (noise env)",i);
	    (*numEnv)++;
	  }
	}   /* if (*numNoisePara) */
	else
	  *noiseEnv = 0;
      }
      else
      {
        unsigned int	x;

	*numHarm = 0;
	numTrans=0;

	if ( harmFlag )
	{
		register char	*t = "IndiLineDecodeFrame: error reading bit stream (harm)";
		int		nhp;

		if ( BsGetBitInt(stream,&x,4) ) IndiLineExit(t);	numTrans=LPCharmEncPar.numParaTable[x];
		numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		if ( BsGetBitInt(stream,&x,5) ) IndiLineExit(t);	ILD->numHarmLine[0]=numHarmLineTab[x];
		numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		if ( BsGetBitInt(stream,&x,1) ) IndiLineExit(t);	ILD->harmPred[*numHarm]=x;
		numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));

		if (ILD->debugLevel>=2)
		  printf("numTrans=%2i, numHarmLine=%3i\n",numTrans,ILD->numHarmLine[0]);

		if (ILD->cfg->bsFormat>=5 && phaseFlag && !(ILD->harmPred[*numHarm])) {
		  if ( BsGetBitInt(stream,&harmNumPhase,HARMPHASEBITS) ) IndiLineExit(t);
		  numBitESC4 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		else
		  harmNumPhase = 0;

		if ( BsGetBitInt(stream,&x,1) ) IndiLineExit(t);	ILD->harmEnv[*numHarm]=x;
		numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
		if ( envParaFlag==0 && ILD->harmEnv[*numHarm]==1)
			IndiLineExit("IndiLineDecodeFrame: harm - no env paras available");


		if ( ILD->harmPred[*numHarm] )
		{
			ILD->harmQuantAmpl[*numHarm][0]+=AContDecode(stream);
			ILD->harmFreqIndex[*numHarm]   +=FHContDecode(stream);
			numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		else
		{
			ILD->harmQuantAmpl[*numHarm][0] = maxAmplIndex+AHNewDecode(stream);
			if ( BsGetBitInt(stream,&ILD->harmFreqIndex[*numHarm],11) ) IndiLineExit(t);
			numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		if ( GetHStretchBits(stream,&ILD->harmQuantFreqStretch[*numHarm]) ) IndiLineExit(t);
		numBitESC3 += GetDiff(&bitPos,BsCurrentBit(stream));

		/*$DecodeLPCBits(stream,ILD->harmQuantAmpl[*numHarm]+1,numTrans,&LPCharmEncPar);$*/
		DecodeLPCFmt6(stream,ILD->harmQuantAmpl[*numHarm]+1,numTrans,
			      0,1,1,"harmLAR",&LPCharmEncPar);
		numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		DecodeLPCFmt6(stream,ILD->harmQuantAmpl[*numHarm]+1,numTrans,
			      2,0,3,"harmLAR",&LPCharmEncPar);
		numBitESC3 += GetDiff(&bitPos,BsCurrentBit(stream));

		nhp=min(numTrans,ILD->prevNumTrans);
		if ( ILD->harmPred[*numHarm]<1 ) nhp=0;
		ResetLPCPara(ILD->harmParaDeqOld[0]+1,nhp,LPCMaxHarmPara-1-nhp,&LPCharmEncPar);
		if ( nhp>0 ) ScaleLPCPredMean(ILD->harmParaDeqOld[0]+1,nhp,&LPCharmEncPar);

		/* bsf=5 phase */
		for (i=0; i<(int)harmNumPhase; i++) {
		  if (BsGetBitInt(stream,&ILD->harmPhaseIndex[i],HARM_PHASE_NUMBIT))
		    IndiLineExit(t);
		  numBitESC4 += GetDiff(&bitPos,BsCurrentBit(stream));
		}

		(*numHarm)++;
	}

	hrmBit = BsCurrentBit(stream)-staBit-hdrBit;

	*noiseEnv = 0;
	*numNoisePara = 0;

	if ( noiseFlag )
	{
		register char	*t = "IndiLineDecodeFrame: error reading bit stream (noise)";

		if ( BsGetBitInt(stream,&x,4) ) IndiLineExit(t);	*numNoisePara = LPCnoiseEncPar.numParaTable[x];
		numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		if ( BsGetBitInt(stream,&x,1) ) IndiLineExit(t);	ILD->noiseContFlag = x;
		numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
		if ( BsGetBitInt(stream,&x,1) ) IndiLineExit(t);	*noiseEnv = x?2:0;   /* HP/NM 20020130 */
		numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));

		if (ILD->debugLevel>=2)
		  printf("numNoisePara=%2i\n",*numNoisePara);

		if ( ILD->noiseContFlag ) {
			ILD->noiseParaQuant[0]+=AContDecode(stream);
			numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		else {
			ILD->noiseParaQuant[0] = maxAmplIndex+ANNewDecode(stream);
			numBitESC0 += GetDiff(&bitPos,BsCurrentBit(stream));
		}

		if ( *noiseEnv ) {
			if (BsGetBitInt(stream,&ILD->envParaIndex[*numEnv],ILD->envNumBit)) IndiLineExit(t);
			numBitESC4 += GetDiff(&bitPos,BsCurrentBit(stream));
			(*numEnv)++;
		}

		/*$DecodeLPCBits(stream,ILD->noiseParaQuant+1,*numNoisePara,&LPCnoiseEncPar);$*/
		DecodeLPCFmt6(stream,ILD->noiseParaQuant+1,*numNoisePara,
			      0,1,1,"noiseLAR",&LPCnoiseEncPar);
		numBitESC1 += GetDiff(&bitPos,BsCurrentBit(stream));
		DecodeLPCFmt6(stream,ILD->noiseParaQuant+1,*numNoisePara,
			      2,0,3,"noiseLAR",&LPCnoiseEncPar);
		numBitESC3 += GetDiff(&bitPos,BsCurrentBit(stream));
	}
      }	/* bsFormat>=4  */

      noiBit = BsCurrentBit(stream)-staBit-hdrBit-hrmBit;

    }	/* bsFormat!=0	*/

    newBit = cntBit = 0;
    tmpBit = BsCurrentBit(stream);

    /* evaluate continue bits */
    contNumLine = 0;
    for (il=0; il<ILD->layPrevNumLine[0]; il++)
      if (ILD->lineContFlag[il]) {
	ILD->lineValid[contNumLine] = ILD->prevLineValid[il];
	ILD->lineParaPred[contNumLine++] = il+1;
      }
    for (il=contNumLine; il<layNumLine[0]; il++) {
      ILD->lineValid[il] = 1;      
      ILD->lineParaPred[il] = 0;
    }

    if (ILD->debugLevel >= 3) {
      printf("cont: ");
      for (il=0; il<ILD->layPrevNumLine[0]; il++)
	printf("%1d",ILD->lineContFlag[il]);
      printf("\n");
    }

    if (ILD->debugLevel >= 2)
      printf("IndiLineDecodeFrame: "
	     "numLine=%d  contNumLine=%d  envParaFlag=%d\n",
	     *numLine,contNumLine,envParaFlag);

    /* get line parameters from bitstream */
    if ( ILD->cfg->bsFormat<4 )
    {
	    for (il=0; il<*numLine; il++) {
	      if (envParaFlag) {
		if (BsGetBitInt(stream,(unsigned int*)&ILD->lineParaEnv[il],1))
		  IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
			       "(lineEnv %d)",il);
	      }
	      else
		ILD->lineParaEnv[il] = 0;
	      if (ILD->lineParaPred[il])
		ILD->lineParaNumBit[il] = ILD->contLineNumBit;
	      else
		ILD->lineParaNumBit[il] = ILD->newLineNumBit;
	      if (BsGetBit(stream,&ILD->lineParaIndex[il],ILD->lineParaNumBit[il]))
		IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
			     "(linePara %d)",il);

	      if (ILD->lineParaPred[il]) {
		cntBit += BsCurrentBit(stream)-tmpBit;
		tmpBit = BsCurrentBit(stream);
	      }
	      else {
		newBit += BsCurrentBit(stream)-tmpBit;
		tmpBit = BsCurrentBit(stream);
	      }
	    }
    }
    else
    {
	register char	*t = "IndiLineDecodeFrame: error reading bit stream";
	register int	m=0;
	int i;

	for (il=0; il<layNumLine[0]; il++)
	{
		if (envParaFlag)
		{
			if ( BsGetBitInt(stream,(unsigned int*)&ILD->lineParaEnv[il],1) ) IndiLineExit(t);
			numBitESC2 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		else ILD->lineParaEnv[il] = 0;

		if ( ILD->lineParaPred[il] )
		{
			ILD->lineParaFIndex[il]=FContDecode(stream);
			ILD->lineParaAIndex[il]=AContDecode(stream);
			ILD->lineParaNumBit[il]=ILD->contLineNumBit;		/* not used	*/
			numBitESC3 += GetDiff(&bitPos,BsCurrentBit(stream));
		}
		else
		{
			ILD->lineParaFIndex[il]=m=m+FNewDecode(stream,layNumLine[0]-il,ILD->maxFIndex-m);
			ILD->lineParaAIndex[il]  = ALNewDecode(stream,maxAmplIndex,ILD->quantMode);
			ILD->lineParaNumBit[il]  = ILD->newLineNumBit;		/* not used	*/
			numBitESC2 += GetDiff(&bitPos,BsCurrentBit(stream));
		}

		ILD->lineParaIndex[il]=(long) (-1024-ILD->lineParaFIndex[il]);	/* for enh.	*/

		if (ILD->lineParaPred[il]) {
		  cntBit += BsCurrentBit(stream)-tmpBit;
		  tmpBit = BsCurrentBit(stream);
		}
		else {
		  newBit += BsCurrentBit(stream)-tmpBit;
		  tmpBit = BsCurrentBit(stream);
		}
	}

	/* bsf=5 phase */
	i = 0;
	for (il=0; il<layNumLine[0]; il++) {
	  ILD->linePhaseIndex[il] = 1<<HARM_PHASE_NUMBIT;   /* phase unavail */
	  if (!ILD->lineParaPred[il]) {
	    if (i<(int)layNumPhase[0]) {
	      if (BsGetBitInt(stream,&ILD->linePhaseIndex[il],HARM_PHASE_NUMBIT))
		IndiLineExit(t);
	      numBitESC4 += GetDiff(&bitPos,BsCurrentBit(stream));
	    }
	    i++;
	  }
	}

	newBit += BsCurrentBit(stream)-tmpBit;
    }

    useBit = BsCurrentBit(stream)-staBit;
    avlBit = 0;
    while (!BsEof(stream,avlBit))
      avlBit++;
    avlBit += useBit;

    if (ILD->debugLevel >= 1) {
      printf("BSF6: bits in esc01234: %4d %4d %4d %4d %4d\n",
	     numBitESC0,numBitESC1,numBitESC2,numBitESC3,numBitESC4);      
    }

  }

  /*$ BE991125>>> $*/
  }
  else {		/*$ ILD->cfg->bsFormat>=6 $*/

    if (ILD->debugLevel)
      printf("reading frame with bsFormat>=6!\n");
    /* ---------- prepare for decoding ---------- */

    *numHarm = 0;
    numTrans=0;
    *noiseEnv = 0;
    *numNoisePara = 0;

    /* ---------- read ESC0 elements ---------- */

/*$#define PR_DETAIL$*/

    staBit = BsCurrentBit(bsESC0);

    if (BsGetBitInt(bsESC0,(unsigned int*)numLine,ILD->numLineNumBit))
      HilnBsErr("numLine",0);
    layNumLine[0] = *numLine;
    if (BsGetBitInt(bsESC0,&harmFlag ,1)) HilnBsErr("harmFlag",0);
    if (BsGetBitInt(bsESC0,&noiseFlag,1)) HilnBsErr("noiseFlag",0);
    if (BsGetBitInt(bsESC0,(unsigned int*)&envParaFlag,1))
      HilnBsErr("envFlag",0);
    if (BsGetBitInt(bsESC0,&phaseFlag,1)) HilnBsErr("phaseFlag",0);
    if (BsGetBitInt(bsESC0,&tmpCW,4)) HilnBsErr("maxAmplIndex",0);
    maxAmplIndex = tmpCW<<2;
#ifdef PR_DETAIL
    printf("maxAmpl %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* most important parameters for harmonic tone */
    if (harmFlag) {
      if (BsGetBitInt(bsESC0,&tmpCW,1)) HilnBsErr("harmContFlag",0);
      harmContFlag = tmpCW;
      ILD->harmPred[*numHarm] = harmContFlag;
      if (BsGetBitInt(bsESC0,&tmpCW,1)) HilnBsErr("harmEnvFlag",0);
      ILD->harmEnv[*numHarm] = tmpCW;
      if (envParaFlag==0 && ILD->harmEnv[*numHarm]==1)
	IndiLineExit("IndiLineDecodeFrame: harm - no env paras available");
      if(!harmContFlag) {
	ILD->harmQuantAmpl[*numHarm][0] = maxAmplIndex+AHNewDecode(bsESC0);
	if (BsGetBitInt(bsESC0,&ILD->harmFreqIndex[*numHarm],11))
	  HilnBsErr("harmFreqIndex",0);
      }
    }
#ifdef PR_DETAIL
    printf("harmpara0 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* most important parameters for noise component */
    if (noiseFlag) {
      if (BsGetBitInt(bsESC0,&tmpCW,1) ) HilnBsErr("noiseContFlag",0);
      ILD->noiseContFlag = tmpCW;
      if (BsGetBitInt(bsESC0,&tmpCW,1) ) HilnBsErr("noiseEnvFlag",0);
      *noiseEnv = tmpCW?2:0;   /* HP/NM 20020130 */
      if (!ILD->noiseContFlag) {
	ILD->noiseParaQuant[0] = maxAmplIndex+ANNewDecode(bsESC0);
      }
    }
#ifdef PR_DETAIL
    printf("noisepara0 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    numBitESC0 = BsCurrentBit(bsESC0)-staBit;

    /* ---------- read ESC1 elements ---------- */

    staBit = BsCurrentBit(bsESC1);

    /*$hdrBit = BsCurrentBit(stream)-staBit;$*/

    /* elements for harmonic tone */
    if (harmFlag) {
      if (BsGetBitInt(bsESC1,&tmpCW,4)) HilnBsErr("numHarmParaIndex",1);
      numTrans = LPCharmEncPar.numParaTable[tmpCW];
      if (BsGetBitInt(bsESC1,&tmpCW,5)) HilnBsErr("numHarmLineIndex",1);
      ILD->numHarmLine[0]=numHarmLineTab[tmpCW];
      if (ILD->debugLevel>=2)
	printf("numTrans=%2i, numHarmLine=%3i\n",numTrans,ILD->numHarmLine[0]);
      if(harmContFlag) {
	ILD->harmQuantAmpl[*numHarm][0] += AContDecode(bsESC1);
	ILD->harmFreqIndex[*numHarm] += FHContDecode(bsESC1);
      }
#ifdef PR_DETAIL
      printf("harmpara1a %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      DecodeLPCFmt6(bsESC1,ILD->harmQuantAmpl[*numHarm]+1,numTrans,0,1,1,
		    "harmLAR",&LPCharmEncPar);
#ifdef PR_DETAIL
      printf("harmpara1b %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    }

    /* elements for noise component */
    if (noiseFlag) {
      if (ILD->noiseContFlag)
	ILD->noiseParaQuant[0] += AContDecode(bsESC1);
#ifdef PR_DETAIL
      printf("noisepara1a %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      if (BsGetBitInt(bsESC1,&tmpCW,4)) HilnBsErr("numNoiseParaIndex",1);
      *numNoisePara = LPCnoiseEncPar.numParaTable[tmpCW];
      if (ILD->debugLevel>=2) printf("numNoisePara=%2i\n",*numNoisePara);
#ifdef PR_DETAIL
      printf("noisepara1b %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
      DecodeLPCFmt6(bsESC1,ILD->noiseParaQuant+1,*numNoisePara,0,1,1,
		    "noiseLAR",&LPCnoiseEncPar);
#ifdef PR_DETAIL
      printf("noisepara1c %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    }

    /* continue bits (layer 0) */
    if(ILD->prevNumLine != PNL_UNKNOWN) {
      /* get continue bits from bitstream */
      for (il=0; il<ILD->layPrevNumLine[0]; il++) {
	if (BsGetBitInt(bsESC1,(unsigned int*)&ILD->lineContFlag[il],1))
	  HilnBsErr("prevLineContFlag",1);
      }
    }
#ifdef PR_DETAIL
    printf("contFlags %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    /* evaluate continue bits */
    contNumLine = 0;
    for (il=0; il<ILD->layPrevNumLine[0]; il++)
      if (ILD->lineContFlag[il]) {
	ILD->lineValid[contNumLine] = ILD->prevLineValid[il];
	ILD->lineParaPred[contNumLine++] = il+1;
      }
    for (il=contNumLine; il<layNumLine[0]; il++) {
      ILD->lineValid[il] = 1;      
      ILD->lineParaPred[il] = 0;
    }

    if (ILD->debugLevel >= 3) {
      printf("cont: ");
      for (il=0; il<ILD->layPrevNumLine[0]; il++)
	printf("%1d",ILD->lineContFlag[il]);
      printf("\n");
    }

    if (ILD->debugLevel >= 2)
      printf("IndiLineDecodeFrame: "
	     "numLine=%d  contNumLine=%d  envParaFlag=%d\n",
	     *numLine,contNumLine,envParaFlag);

    numBitESC1 = BsCurrentBit(bsESC1)-staBit;

    /* ---------- read ESC2 elements ---------- */

    staBit = BsCurrentBit(bsESC2);

#ifdef PR_DETAIL
    printf("lineEnv %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    lfi = 0;
/*     newBit = cntBit = 0; */
/*     tmpBit = BsCurrentBit(stream); */
    for (il=0; il<layNumLine[0]; il++) {
      if (envParaFlag) {
	if (BsGetBitInt(bsESC2,(unsigned int*)&ILD->lineParaEnv[il],1))
	  HilnBsErr("lineEnvFlag",2);
      }
      else
	ILD->lineParaEnv[il] = 0;
    }
    /* frequencies and amplitudes for new individual lines */
    for (il=0; il<layNumLine[0]; il++) {
      if (!ILD->lineParaPred[il]) {
	ILD->lineParaFIndex[il] = lfi =
	  lfi+FNewDecode(bsESC2,layNumLine[0]-il,ILD->maxFIndex-lfi);
	ILD->lineParaAIndex[il]  = ALNewDecode(bsESC2,maxAmplIndex,
					       ILD->quantMode);
	/* ILD->lineParaNumBit[il]  = ILD->newLineNumBit; */	/* not used */
      }


/*       if (ILD->lineParaPred[il]) { */
/* 	cntBit += BsCurrentBit(stream)-tmpBit; */
/* 	tmpBit = BsCurrentBit(stream); */
/*       } */
/*       else { */
/* 	newBit += BsCurrentBit(stream)-tmpBit; */
/* 	tmpBit = BsCurrentBit(stream); */
/*       } */

    }
#ifdef PR_DETAIL
    printf("newlinepara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif


    numBitESC2 = BsCurrentBit(bsESC2)-staBit;

    /* ---------- read ESC3 elements ---------- */

    staBit = BsCurrentBit(bsESC3);

    /* get envelope parameters from bitstream */
    if (envParaFlag) {
      envParaNumBit = ILD->envNumBit;
      if (BsGetBitInt(bsESC3,&ILD->envParaIndex[0],envParaNumBit))
	HilnBsErr("envPara",3);
    }
    else
      envParaNumBit = 0;
    *numEnv = 1;
#ifdef PR_DETAIL
    printf("envpara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    if (harmFlag) {
      DecodeLPCFmt6(bsESC3,ILD->harmQuantAmpl[*numHarm]+1,numTrans,2,0,3,
		    "harmLAR",&LPCharmEncPar);
    }
#ifdef PR_DETAIL
    printf("harm3 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    if (noiseFlag) {
      DecodeLPCFmt6(bsESC3,ILD->noiseParaQuant+1,*numNoisePara,2,0,3,
		    "noiseLAR",&LPCnoiseEncPar);
    }
#ifdef PR_DETAIL
    printf("noise3 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    for (il=0; il<layNumLine[0]; il++) {
      if (ILD->lineParaPred[il]) {
	ILD->lineParaFIndex[il]=FContDecode(bsESC3);
	ILD->lineParaAIndex[il]=AContDecode(bsESC3);
	ILD->lineParaNumBit[il]=ILD->contLineNumBit;	/* not used	*/
      }
      ILD->lineParaIndex[il]=(long)(-1024-ILD->lineParaFIndex[il]);/*for enh.*/
    }
#ifdef PR_DETAIL
    printf("contlinepara %4d,",BsCurrentBit(bsESC0)); /*****/
#endif
    /* newBit += BsCurrentBit(stream)-tmpBit; */

    if (harmFlag) {
      if (GetHStretchBits(bsESC3,&ILD->harmQuantFreqStretch[*numHarm]))
	HilnBsErr("harmFreqStretch",3);
    }
#ifdef PR_DETAIL
    printf("stretch %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    numBitESC3 = BsCurrentBit(bsESC3)-staBit;

    /* ---------- read ESC4 elements ---------- */

    staBit = BsCurrentBit(bsESC4);

    if (harmFlag) {
      if (phaseFlag && !harmContFlag) {
	if (BsGetBitInt(bsESC4,&harmNumPhase,HARMPHASEBITS))
	  HilnBsErr("numHarmPhase",4);
      }
      else
	harmNumPhase = 0;
      for (i=0; i<(int)harmNumPhase; i++) {
	if (BsGetBitInt(bsESC4,&ILD->harmPhaseIndex[i],HARM_PHASE_NUMBIT))
	  HilnBsErr("harmPhase",4);
      }
      /*$hrmBit = BsCurrentBit(stream)-staBit-hdrBit;$*/
    }
#ifdef PR_DETAIL
    printf("harm4 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    if (noiseFlag) {
      if (*noiseEnv) {
	if (BsGetBitInt(bsESC4,&ILD->envParaIndex[*numEnv],ILD->envNumBit))
	  HilnBsErr("noiseEnvPara",4);
	(*numEnv)++;
      }
    }
#ifdef PR_DETAIL
    printf("noise4 %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    if (phaseFlag) {
      if (BsGetBitInt(bsESC4,&(layNumPhase[0]),ILD->numLineNumBit))
	HilnBsErr("numLinePhase",4);
    }
    else
      layNumPhase[0] = 0;
    i = 0;
    for (il=0; il<layNumLine[0]; il++) {
      ILD->linePhaseIndex[il] = 1<<HARM_PHASE_NUMBIT;   /* phase unavail */
      if (!ILD->lineParaPred[il]) {
	if (i<(int)layNumPhase[0])
	  if (BsGetBitInt(bsESC4,&ILD->linePhaseIndex[il],HARM_PHASE_NUMBIT))
	    HilnBsErr("linePhase",4);
	i++;
      }
    }
#ifdef PR_DETAIL
    printf("ilphase %4d,",BsCurrentBit(bsESC0)); /*****/
#endif

    numBitESC4 = BsCurrentBit(bsESC4)-staBit;

    /* ---------- further decoding steps ---------- */

    if (harmFlag) {
      int nhp;
      nhp=min(numTrans,ILD->prevNumTrans);
      if ( ILD->harmPred[*numHarm]<1 ) nhp=0;
      ResetLPCPara(ILD->harmParaDeqOld[0]+1,nhp,LPCMaxHarmPara-1-nhp,
		   &LPCharmEncPar);
      if ( nhp>0 ) ScaleLPCPredMean(ILD->harmParaDeqOld[0]+1,nhp,
				    &LPCharmEncPar);
      (*numHarm)++;
    }      

    useBit = numBitESC0 + numBitESC1 + numBitESC2 + numBitESC3 + numBitESC4;
    tmpBit = 0;
    while (!BsEof(bsESC0,tmpBit))
      tmpBit++;
    avlBit = tmpBit;
    tmpBit = 0;
    while (!BsEof(bsESC1,tmpBit))
      tmpBit++;
    avlBit += tmpBit;
    tmpBit = 0;
    while (!BsEof(bsESC2,tmpBit))
      tmpBit++;
    avlBit += tmpBit;
    tmpBit = 0;
    while (!BsEof(bsESC3,tmpBit))
      tmpBit++;
    avlBit += tmpBit;
    tmpBit = 0;
    while (!BsEof(bsESC4,tmpBit))
      tmpBit++;
    avlBit += tmpBit;
    avlBit += useBit;

    if (ILD->debugLevel >= 1) {
      printf("BSF6: bits in esc01234: %4d %4d %4d %4d %4d\n",
	     numBitESC0,numBitESC1,numBitESC2,numBitESC3,numBitESC4);      
    }

  }

  if(ILD->prevNumLine != PNL_UNKNOWN) {

  /*$ <<<BE991125 $*/

    /* get enhancement parameters from bit stream */
    if (streamEnha) {
      IndiLineEnhaNumBit(ILD->ILQ,
			 layNumLine[0],NULL,ILD->lineParaPred,
			 ILD->lineParaIndex,ILD->lineParaNumBit,
			 &envParaEnhaNumBit,&lineParaEnhaNumBit);
      /* envelope */
      if (envParaFlag)
	if (BsGetBitInt(streamEnha,&envParaEnhaIndex,envParaEnhaNumBit))
	  IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
		       "(envParaEnha)");
      /* harm */

      if (numTrans) { /* harm used HP20010103 */
	fNumBitBase = 0;
	if ( ILD->cfg->bsFormat<4 )
	  {
	    while (fNumBitBase<HARM_MAX_FBASE &&
		   ILD->harmFreqIndex[0]>freqThrE[fNumBitBase])
	      fNumBitBase++;
	  }
	else
	  {
	    while (fNumBitBase<HARM_MAX_FBASE &&
		   ILD->harmFreqIndex[0]>freqThrEBsf4[fNumBitBase])
	      fNumBitBase++;
	  }

	if (fNumBitBase==HARM_MAX_FBASE)
	  IndiLineExit("IndiLineDecodeFrame: fnumbite error, harmFreqIndex=%d",
		       ILD->harmFreqIndex[0]);
	fNumBitBase += max(0,min(3,ILD->enhaQuantMode))-3;
	for(i=0;i<min(10,ILD->numHarmLine[0]);i++) {
	  fNumBitE = max(0,fNumBitBase+fEnhaBitsHarm[i]);
	  ILD->harmEnhaNumBit[i] = HARM_PHASE_NUMBIT+fNumBitE;
	  if (BsGetBitInt(streamEnha,&ILD->harmEnhaIndex[i],
			  ILD->harmEnhaNumBit[i]))
	    IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
			 "(HarmEnha%d)",i);
	}
      }

      /* indi lines */
      for (il=0; il<layNumLine[0]; il++)
	if (BsGetBit(streamEnha,&ILD->lineParaEnhaIndex[il],
		     lineParaEnhaNumBit[il]))
	  IndiLineExit("IndiLineDecodeFrame: error reading bit stream "
		       "(lineParaEnha %d)",il);
    }

    if ( ILD->cfg->bsFormat>=4 )
    {
    	register int	i,j,k=0;

	for (layer=1; layer<numLayer; layer++)
	{
		register char	*t = "IndiLineDecodeFrame: error reading bit stream";
		register int	m=0;
		unsigned int	x;

		if ( BsGetBitInt(streamExt[layer-1],(unsigned int*) (layNumLine+layer),ILD->numLineNumBit) ) IndiLineExit(t);
		layNumLine[layer]+=layNumLine[layer-1];

		if (ILD->cfg->bsFormat>=5 && phaseFlag) {
		  if ( BsGetBitInt(streamExt[layer-1],&(layNumPhase[layer]),ILD->numLineNumBit) ) IndiLineExit(t);
		}
		else
		  layNumPhase[layer] = 0;
		

		/* evaluate continue bits */
		contNumLine = layNumLine[layer-1];
		for (il=0; il<ILD->layPrevNumLine[layer-1]; il++)
		{
			j=-1;
			for (i=0; i<layNumLine[layer-1]; i++) if ( ILD->lineParaPred[i]==(il+1) ) { j=i; break; }

			if ( j<0 )
			{ k++;
				if ( BsGetBitInt(streamExt[layer-1],&x,1) ) IndiLineExit(t);
				if ( x )
				{
					ILD->lineValid[contNumLine] = ILD->prevLineValid[il];
					ILD->lineParaPred[contNumLine++] = il+1;
				}
			}
		}
		for (; il<ILD->layPrevNumLine[layer]; il++)
		{
			if ( BsGetBitInt(streamExt[layer-1],&x,1) ) IndiLineExit(t);	k++;
			if ( x )
			{
				ILD->lineValid[contNumLine] = ILD->prevLineValid[il];
				ILD->lineParaPred[contNumLine++] = il+1;
			}
		}
		for (il=contNumLine; il<layNumLine[layer]; il++) {
			ILD->lineValid[il] = 1;
			ILD->lineParaPred[il] = 0;
		}

		for (il=layNumLine[layer-1]; il<layNumLine[layer]; il++)
		{
			if (envParaFlag)
			{
				if ( BsGetBitInt(streamExt[layer-1],(unsigned int*)&ILD->lineParaEnv[il],1) ) IndiLineExit(t);
			}
			else ILD->lineParaEnv[il] = 0;

			if ( ILD->lineParaPred[il] )
			{
				ILD->lineParaFIndex[il]=FContDecode(streamExt[layer-1]);
				ILD->lineParaAIndex[il]=AContDecode(streamExt[layer-1]);
				ILD->lineParaNumBit[il]=ILD->contLineNumBit;		/* not used	*/
			}
			else
			{
				ILD->lineParaFIndex[il]=m=m+FNewDecode(streamExt[layer-1],layNumLine[layer]-il,ILD->maxFIndex-m);
				ILD->lineParaAIndex[il]  = ALNewDecode(streamExt[layer-1],maxAmplIndex,ILD->quantMode);
				ILD->lineParaNumBit[il]  = ILD->newLineNumBit;		/* not used	*/
			}

			ILD->lineParaIndex[il]=(long) (-1024-ILD->lineParaFIndex[il]);	/* for enh.	*/

			if (streamEnha) /* HP20010215 */
			  lineParaEnhaNumBit[il] = 0; /* no enha info for ext streams */
		}

		/* bsf=5 phase */
		i = 0;
		for (il=layNumLine[layer-1]; il<layNumLine[layer]; il++) {
		  ILD->linePhaseIndex[il] = 1<<HARM_PHASE_NUMBIT;   /* phase unavail */
		  if (!ILD->lineParaPred[il]) {
		    if ( i < ((int) layNumPhase[layer]) )
		      if (BsGetBitInt(streamExt[layer-1],&ILD->linePhaseIndex[il],HARM_PHASE_NUMBIT))
			IndiLineExit(t);
		    i++;
		  }
		}

	}
    }

    /* dequantise noise */

    if ( ILD->cfg->bsFormat<4 ) {

      if (*numNoisePara) {
	*noiseFreq = ILD->maxNoiseFreq;

	noiseParaSteps = 1<<noiseParaNumBit;
	noiseNorm = exp((double)(noiseNormIndex+1)*log(2.)/2.)/noiseParaSteps;
	ILD->noisePara[0] = ((double)ILD->noiseParaIndex[0]+.5)*noiseNorm;
	if(ILD->debugLevel >= 2)
	  printf("numNoisePara=%2d, env=%1d, Para=%6.1f",*numNoisePara,*noiseEnv,
	         ILD->noisePara[0]);
        for(i=1;i<*numNoisePara;i++) {
	  ILD->noisePara[i] = ((double)ILD->noiseParaIndex[i]-
			     noiseParaSteps/2.+.5)*noiseNorm*2.;
	  if(ILD->debugLevel >= 2)
	    printf("%6.1f",ILD->noisePara[i]);
        }
        if(ILD->debugLevel >= 2)
	  printf("\n");
    
        /* dequantise noise envelope parameters */
        IndiLineEnvDequant(ILD->ILQ,*noiseEnv,ILD->envParaIndex[1],
			   ILD->envNumBit,0,0,&tmpEnvPara,NULL);
        for (i=0;i<ENVPARANUM;i++)
	  ILD->envPara[1][i] = tmpEnvPara[i];
      }
    }
    else
    {
        *noiseFreq = ILD->maxNoiseFreq;
	if ( *numNoisePara )
	{
		register int	n;

		if ( ILD->noiseContFlag ) n=min(*numNoisePara,ILD->prevNumNoise); else n=0;
		if ( n>0 ) ScaleLPCPredMean(ILD->noiseParaOld+1,n,&LPCnoiseEncPar);
		ResetLPCPara(ILD->noiseParaOld+1,n,*numNoisePara-n,&LPCnoiseEncPar);

		DequantizeLPCPara(ILD->noiseParaOld+1,ILD->noiseParaQuant+1,*numNoisePara,&LPCnoiseEncPar);

		ILD->noisePara[0] = ADeQuant(ILD->ILQ,ILD->noiseParaQuant[0]);
		LAR2RefCo(ILD->noisePara+1,ILD->noiseParaOld+1,*numNoisePara);

		/* for(i=0;i<*numNoisePara;i++)fprintf(stderr,"nk%2d = %10.5f (%10.3f)\n",i,ILD->noisePara[i+1],log((1.0+ILD->noisePara[i+1])/(1.0-ILD->noisePara[i+1]))); */

		/* dequantise noise envelope parameters */
		IndiLineEnvDequant(ILD->ILQ,*noiseEnv,ILD->envParaIndex[1],
		  ILD->envNumBit,0,0,&tmpEnvPara,NULL);
		for (i=0;i<ENVPARANUM;i++)
		ILD->envPara[1][i] = tmpEnvPara[i];
	}
    }


    /* dequantise envelope parameters */
    /* !!! must come after dequant of noise env !!! */
    /* !!! must come directly before IndiLineDequant() !!! */
    IndiLineEnvDequant(ILD->ILQ,envParaFlag,ILD->envParaIndex[0],envParaNumBit,
		       (streamEnha)?envParaEnhaIndex:0,
		       (streamEnha)?envParaEnhaNumBit:0,
		       &tmpEnvPara,(streamEnha)?&tmpEnvParaEnha:(double**)NULL);
    for (i=0;i<ENVPARANUM;i++)
      ILD->envPara[0][i] = (streamEnha)?tmpEnvParaEnha[i]:tmpEnvPara[i];

    /* dequantise line parameters */
    if ( ILD->cfg->bsFormat<4 )
	    IndiLineDequant(ILD->ILQ,*numLine,(int*)NULL,
			    ILD->lineParaPred,ILD->lineValid,ILD->lineParaEnv,
			    ILD->lineParaIndex,ILD->lineParaNumBit,
			    (int*)NULL,(int*)NULL,1,
			    (streamEnha)?ILD->lineParaEnhaIndex:
			    (unsigned long*)NULL,
			    (streamEnha)?lineParaEnhaNumBit:(int*)NULL,
			    &tmpLineFreq,&tmpLineAmpl,&tmpLineEnv,&tmpLinePred,
			    (streamEnha)?&tmpLineFreqEnha:(double**)NULL,
			    (streamEnha)?&tmpLineAmplEnha:(double**)NULL,
			    (streamEnha)?&tmpLinePhaseEnha:(double**)NULL);
	else
	    IndiLineDequant(ILD->ILQ,layNumLine[numLayer-1],(int*)NULL,
			    ILD->lineParaPred,ILD->lineValid,ILD->lineParaEnv,
			    ILD->lineParaIndex,ILD->lineParaNumBit,
			    ILD->lineParaFIndex, ILD->lineParaAIndex,1,
			    (streamEnha)?ILD->lineParaEnhaIndex:
			    (unsigned long*)NULL,
			    (streamEnha)?lineParaEnhaNumBit:(int*)NULL,
			    &tmpLineFreq,&tmpLineAmpl,&tmpLineEnv,&tmpLinePred,
			    (streamEnha)?&tmpLineFreqEnha:(double**)NULL,
			    (streamEnha)?&tmpLineAmplEnha:(double**)NULL,
			    (streamEnha)?&tmpLinePhaseEnha:(double**)NULL);


    for (il=0;il<layNumLine[numLayer-1];il++) {
      ILD->lineFreq[il] = (streamEnha)?tmpLineFreqEnha[il]:tmpLineFreq[il];
      ILD->lineAmpl[il] = (streamEnha)?tmpLineAmplEnha[il]:tmpLineAmpl[il];
      if (streamEnha &&
	  il < layNumLine[0] /* HP20010215 */ ) {
	ILD->linePhase[il] = tmpLinePhaseEnha[il];
	ILD->linePhaseValid[il] = 1;
      }
      ILD->lineEnv[il] = tmpLineEnv[il];
      ILD->linePred[il] = tmpLinePred[il];
    }

    if (ILD->cfg->bsFormat>=5) {
      /* dequant line phase */
      for (il=0;il<layNumLine[numLayer-1];il++) {
	if (ILD->linePhaseIndex[il] == 1<<HARM_PHASE_NUMBIT) {
	  if (!(streamEnha&&
		il < layNumLine[0] /* HP20010215 */ )) {
	    ILD->linePhase[il] = 0;
	    ILD->linePhaseValid[il] = 0;
	  }
	}
	else {
	  ILD->linePhase[il] = ((ILD->linePhaseIndex[il]+.5)/HARM_PHASE_NUMSTEP-.5)*HARM_PHASE_RANGE;
	  ILD->linePhaseValid[il] = 1;
	}
      }
    }

    /* dequantise harm */

    if(numTrans) {

      if ( ILD->cfg->bsFormat<4 ) {

	harmAmplMax = 16*(1+addHarmAmplBits)-1;
	harmAmplOff = harmAmplMax/3;

	ILD->harmFreq[0] = exp((ILD->harmFreqIndex[0]+.5)/2048.*log(4000./30.))*30.;
	ILD->harmFreqStretch[0] = (ILD->harmFreqStretchIndex[0]/32.-.5)*0.002;

	amplInt = ILD->harmAmplIndex[0][0];
	ILD->transAmpl[0] = ((double)amplInt+.5)/3.;
	for(i=1;i<numTrans;i++) {
	  amplInt += ILD->harmAmplIndex[0][i]-harmAmplOff;
	  ILD->transAmpl[i] = ((double)amplInt+.5)/3.;
	}

	for (i=0; i<numTrans; i++) {
	  ILD->transAmpl[i] = exp(ILD->transAmpl[i]*log(2.));
	}
	ILD->harmLineIdx[0] = *numLine;
	ILD->numHarmLine[0] = InvTransfHarmAmpl(numTrans,ILD->transAmpl,
					      &ILD->lineAmpl[ILD->harmLineIdx[0]]);

	hfreqrs = HFREQRELSTEP;
      }
      else
      {
        int	n;

	DequantizeLPCPara(ILD->harmParaDeqOld[0]+1,ILD->harmQuantAmpl[0]+1,numTrans,&LPCharmEncPar);

	ILD->harmParaDeqOld[0][0] = ADeQuant(ILD->ILQ,ILD->harmQuantAmpl[0][0]);

	ILD->harmFreq[0] = exp((ILD->harmFreqIndex[0]+.5)/2048.*log(4000./20.))*20.;

	ILD->harmFreqStretch[0] = ((double) HSTRETCHQ) * ((double) ILD->harmQuantFreqStretch[0]);
	ILD->harmLineIdx[0] = layNumLine[numLayer-1];

	n=ILD->numHarmLine[0];
	if ( n>ILD->maxNumHarmLine ) ILD->numHarmLine[0]=ILD->maxNumHarmLine;

	LPCParaToLineAmpl(	ILD->lineAmpl+ILD->harmLineIdx[0],ILD->numHarmLine[0],
				ILD->harmParaDeqOld[0],numTrans,
				/* M_PI/((double) (n)) ); */
				M_PI/((double) (n+1)) );


	hfreqrs = HFREQRELSTEP4;
      }

      /* harm enhancement */
      if (streamEnha && linePhase) {
	for(i=0;i<min(10,ILD->numHarmLine[0]);i++) {
	  fNumBitE = ILD->harmEnhaNumBit[i]-HARM_PHASE_NUMBIT;
	  if(fNumBitE>0) {
	    freqNumStepE = 1<<fNumBitE;
	    fintE = (ILD->harmEnhaIndex[i]>>HARM_PHASE_NUMBIT)
	      & (freqNumStepE-1);
	    ILD->lineFreq[ILD->harmLineIdx[0]+i] =
	      1+((fintE+.5)/freqNumStepE-.5)*(hfreqrs-1);
	  }
	  else
	    ILD->lineFreq[ILD->harmLineIdx[0]+i] = 1;

	  pintE = ILD->harmEnhaIndex[i] & (HARM_PHASE_NUMSTEP-1);
	  ILD->linePhase[ILD->harmLineIdx[0]+i] =
			((pintE+.5)/HARM_PHASE_NUMSTEP-.5)*HARM_PHASE_RANGE;
	  ILD->linePhaseValid[ILD->harmLineIdx[0]+i] = 1;
	  ILD->lineEnv[ILD->harmLineIdx[0]+i] = 0;
	  ILD->linePred[ILD->harmLineIdx[0]+i] = 0;
	}
	for(i=ILD->harmLineIdx[0]+min(10,ILD->numHarmLine[0]);
	    i<ILD->harmLineIdx[0]+ILD->numHarmLine[0];i++) {
	  ILD->lineFreq[i] = 1;
	  ILD->linePhase[i] = 0;
	  ILD->linePhaseValid[i] = 0;
	  ILD->lineEnv[i] = 0;
	  ILD->linePred[i] = 0;
	  /* ILD->lineAmpl[i] = 0; */
	}
      }
      else {  
	/* HP 970709 */
	for (i=ILD->harmLineIdx[0];
	     i<ILD->harmLineIdx[0]+ILD->numHarmLine[0]; i++) {
	  ILD->lineFreq[i] = 1;		/* HP 990701 */
	  ILD->linePhase[i] = 0;
	  ILD->linePhaseValid[i] = 0;
	  ILD->lineEnv[i] = 0;
	  ILD->linePred[i] = 0;
	}
      }

      if (ILD->cfg->bsFormat>=5) {
	/* dequant harm phase */
	for (i=0;i<(int)harmNumPhase;i++) {
	  ILD->linePhase[ILD->harmLineIdx[0]+i] = ((ILD->harmPhaseIndex[i]+.5)/HARM_PHASE_NUMSTEP-.5)*HARM_PHASE_RANGE;
	  ILD->linePhaseValid[ILD->harmLineIdx[0]+i] = 1;
	}
      }


      if (ILD->debugLevel >= 2)
	printf("numTrans=%2d, numHarmLine=%3d\n",numTrans,ILD->numHarmLine[0]);

      if (ILD->debugLevel >= 2)
	printf("ILDecodeFrame: "
	       "harm n=%d  c=%d  f=%7.1f  fs=%8.1e  a1=%7.1f  a2=%7.1f\n",
	       ILD->numHarmLine[0],ILD->harmPred[0],ILD->harmFreq[0],
	       ILD->harmFreqStretch[0],
	       ILD->lineAmpl[ILD->harmLineIdx[0]],
	       ILD->lineAmpl[ILD->harmLineIdx[0]+1]);

    }
    else {
      ILD->numHarmLine[0] = 0;

      if(ILD->debugLevel>=2)
	printf("numTrans=0\n");
    }

    if (ILD->debugLevel>1 && ILD->cfg->bsFormat>=4) {
      int i,q,ia,ha,na,da;
      ia = ha = na = 64;
      if (numTrans)
	ha = ILD->harmQuantAmpl[0][0];
      if (*numNoisePara)
	na = ILD->noiseParaQuant[0];
      for (i=0;i<layNumLine[numLayer-1];i++) {
	/* quick'n'dirty re-quant :-( */
	q = (int)(log(32768/ILD->lineAmpl[i]) * (20.0/LOG10/1.5));
	if (ia > q)
	  ia = q;
      }
      da = 0;
      da = max(da,maxAmplIndex-ha);
      da = max(da,maxAmplIndex-na);
      da = max(da,maxAmplIndex-ia);
      printf("maxAmpl=%2d harm=%2d noise=%2d line=%2d over=%2d %s\n",
	     maxAmplIndex,ha,na,ia,da,(da)?"MAX_AMPL_OVER":"");
    }

  /*$ BE991125>>> $*/
    /*$if (ILD->debugLevel) {$*/
    if (ILD->debugLevel && ILD->cfg->bsFormat<6) {
      int bpc;
      int nhl[33] = {0,1,2,3,4,5,6,7,8,9,10,12,14,16,18,20,22,25,28,31,34,37,
		     41,45,49,53,58,63,68,74,80,87,99};
      frmBit = (useBit<352)?192:512;    /* sorry, just 6/16 kbps @ 32ms */
      bpc = frmBit/50+1;
      i = 0;
      for (il=0; il<layNumLine[0]; il++)
	if (ILD->lineParaPred[il])
	  i++;
      envBit = 0;
      if (envParaFlag) {
	envBit = 12+layNumLine[0];
	hdrBit -= 12;
	cntBit -= i;
	newBit -= layNumLine[0]-i;
      }
      printf("      hdr env   hrm %%rt #l [Hz] #p   noi %%rt #p   "
	     "cnt #l new #l   use frm avl\n"
	     "bits: %3d %3d   %3d %3.0f %2d %4.0f %2d   %3d %3.0f %2d   "
	     "%3d %2d %3d %2d   %3d %3d %3d\n",
	     hdrBit,
	     envBit,
	     hrmBit,0.,
	     (ILD->cfg->bsFormat>=4)?
	     ((numTrans)?ILD->numHarmLine[0]:0):
	     (nhl[min(32,numTrans)]),
	     (numTrans)?ILD->harmFreq[0]:0,numTrans,
	     noiBit,0.,*numNoisePara,
	     cntBit,i,
	     newBit,layNumLine[0]-i,
	     useBit,frmBit,avlBit);
      printf("alloc: ");
      i = 0;
      for (; i<=hdrBit/bpc; i++)
	printf("X%s",(i==frmBit/bpc)?"|":"");
      for (; i<=(hdrBit+envBit)/bpc; i++)
	printf("^%s",(i==frmBit/bpc)?"|":"");
      for (; i<=(hdrBit+envBit+hrmBit)/bpc; i++)
	printf("H%s",(i==frmBit/bpc)?"|":"");
      for (; i<=(hdrBit+envBit+hrmBit+noiBit)/bpc; i++)
	printf("/%s",(i==frmBit/bpc)?"|":"");
      for (; i<=(hdrBit+envBit+hrmBit+noiBit+cntBit)/bpc; i++)
	printf("c%s",(i==frmBit/bpc)?"|":"");
      for (; i<=useBit/bpc; i++)
	printf("+%s",(i==frmBit/bpc)?"|":"");
      for (; i<=avlBit/bpc; i++)
	printf("-%s",(i==frmBit/bpc)?"|":"");
      printf(" \n");
    }
    if (ILD->debugLevel && ILD->cfg->bsFormat>=6) {
      printf("alloc: ... t.b.d. ... \n");
    }

    /* copy parameters into frame-to-frame memory */

    ILD->prevNumLine = *numLine = layNumLine[numLayer-1];	/* lines of all layers	*/

    for(i=0;i<layNumLine[numLayer-1];i++)
      ILD->prevLineValid[i] = ILD->lineValid[i];
  }				/* if(ILD->prevNumLine != PNL_UNKNOWN) { */

  else {
    ILD->prevNumLine = layNumLine[numLayer-1];
    for(i=0;i<layNumLine[numLayer-1];i++)
      ILD->prevLineValid[i] = 0;
    *numLine = 0;
    *numHarm = 0;
    *numNoisePara = 0;
  }

  ILD->prevNumNoise = *numNoisePara;
  ILD->prevNumTrans = numTrans;

  for (i=0; i<numLayer; i++) ILD->layPrevNumLine[i]=layNumLine[i];

  /* set pointers to arrays with return values */
  *envPara = ILD->envPara;
  *lineFreq = ILD->lineFreq;
  *lineAmpl = ILD->lineAmpl;
  if (linePhase) {
    *linePhase = ILD->linePhase;
    *linePhaseValid = ILD->linePhaseValid;
  }
  *lineEnv = ILD->lineEnv;
  *linePred = ILD->linePred;
  *numHarmLine = ILD->numHarmLine;
  *harmFreq = ILD->harmFreq;
  *harmFreqStretch = ILD->harmFreqStretch;
  *harmLineIdx = ILD->harmLineIdx;
  *harmEnv = ILD->harmEnv;
  *harmPred = ILD->harmPred;
  *noisePara = ILD->noisePara;
  if ( (ILD->cfg->bsFormat>=4) && (*numNoisePara) ) (*numNoisePara)++;
}


/* IndiLineDecodeHarm() */
/* Append harmonic lines and join with individual lines. */

void IndiLineDecodeHarm (
  ILDstatus *ILD,		/* in: ILD status handle */
  int numLine,			/* in: num lines */
  double *lineFreq,		/* in: line frequency */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  double *lineAmpl,		/* in: line amplitude */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  double *linePhase,		/* in: line phase */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     (NULL in basic mode) */
				/*     Note: values are MODIFIED !!! */
  int *linePhaseValid,		/* in: line phase validity flag */
				/*     [0..numLine-1] */
				/*     (required in enha mode) */
				/*     Note: values are MODIFIED !!! */
  int *lineEnv,			/* in: line envelope flag */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  int *linePred,		/* in: line predecessor idx */
				/*     in prev frame */
				/*     [0..numLine-1] */
				/*     Note: values are MODIFIED !!! */
  int numHarm,			/* in: num harmonic tones */
  int *numHarmLine,		/* in: num lines of harmonic tone */
				/*     [0..numHarm-1] */
  double *harmFreq,		/* in: harm fundamental frequency [Hz] */
				/*     [0..numHarm-1] */
				/*     (NULL to copy freq&env from lineFreq) */
  double *harmFreqStretch,	/* in: harm freq stretch ratio */
				/*     [0..numHarm-1] */
  int *harmLineIdx,		/* in: harm line para start idx */
				/*     [0..numHarm-1] */
				/*     ampl: lineAmpl[idx] etc. */
				/*     idx=harmLineIdx+(0..numHarmLine-1) */
				/*     Note: values are MODIFIED !!! */
  int *harmEnv,			/* in: harm envelope flag/idx */
				/*     [0..numHarm-1] */
  int *harmPred,		/* in: harm tone predecessor idx */
				/*     [0..numHarm-1] */
  int *numAllLine)		/* out: num harm+indi lines */
{
  int i,ih,il;
  int lineIdx;
  int optp;
  double df,da,q,optq;
  int numCont;

  /* append harm to indilines */
  il = numLine;
  for (ih=0; ih<numHarm; ih++) {
    if (harmLineIdx[ih] < il)
      IndiLineExit("IndiLineDecodeHarm: harmLineIdx %d error",ih);
    lineIdx = il;
    for (i=0; i<numHarmLine[ih]; i++) {

      if(harmFreq) {
	if (ILD->debugLevel>=4 && linePhase && i<min(numHarmLine[ih],10))
	  printf("HarmEnha: fq=%7.2f, fe=%7.2f, pq=%5.2f\n",
		 harmFreq[ih]*(i+1)*
		 (1+(ILD->cfg->noStretch?0:1)*harmFreqStretch[ih]*(i+1)),
		 lineFreq[harmLineIdx[ih]+i]*harmFreq[ih]*(i+1)*
		 (1+(ILD->cfg->noStretch?0:1)*harmFreqStretch[ih]*(i+1)),
		 linePhase[harmLineIdx[ih]+i]);

	if(!linePhase)
	  lineFreq[il] = harmFreq[ih]*(i+1)*
	    (1+(ILD->cfg->noStretch?0:1)*harmFreqStretch[ih]*(i+1));
	else
	  lineFreq[il] = lineFreq[harmLineIdx[ih]+i]*harmFreq[ih]*(i+1)*
	    (1+(ILD->cfg->noStretch?0:1)*harmFreqStretch[ih]*(i+1));
      }
      else
	lineFreq[il] = lineFreq[harmLineIdx[ih]+i];

      lineAmpl[il] = lineAmpl[harmLineIdx[ih]+i];
      if (linePhase) {
	linePhase[il] = linePhase[harmLineIdx[ih]+i];
	linePhaseValid[il] = linePhaseValid[harmLineIdx[ih]+i];
      }

      if(harmFreq)
	lineEnv[il] = harmEnv[ih];
      else
	lineEnv[il] = lineEnv[harmLineIdx[ih]+i];
      linePred[il] = 0;
      if (harmPred[ih] && harmPred[ih]-1<ILD->prevNumHarm &&
	  i<ILD->prevNumHarmLine[harmPred[ih]-1])
	linePred[il] = ILD->prevHarmLineIdx[harmPred[ih]-1]+i+1;
      il++;
    }
    harmLineIdx[ih] = lineIdx;
  }
  *numAllLine = il;

  if (ILD->contMode==0 || ILD->contMode==1) {
  /* contMode==2 -> no continuation   HP990608 */
    numCont = 0;
    /* continue harm <-> indi lines */
    if (ILD->cfg->contda>1 && ILD->cfg->contdf>1) {
      for (i=0; i<ILD->prevNumAllLine; i++)
	ILD->prevCont[i] = 0;
      for (il=0; il<*numAllLine; il++)
	if (linePred[il])
	  ILD->prevCont[linePred[il]-1] = 1;
      for (il=0; il<*numAllLine; il++)
	if (!linePred[il]) {
	  optp = 0;
	  optq = 0;
	  for (i=0; i<ILD->prevNumAllLine; i++)
	    if (!ILD->prevCont[i] && (ILD->contMode ||
				      i>=ILD->prevNumIndiLine ||
				      il>=numLine)) {
	      da = lineAmpl[il]/ILD->prevLineAmpl[i];
	      df = lineFreq[il]/ILD->prevLineFreq[i];
	      da = max(da,1/da);
	      df = max(df,1/df);
	      q = (1.-(df-1.)/(ILD->cfg->contdf-1.))*
		(1.-(da-1.)/(ILD->cfg->contda-1.));
	      if (da <= ILD->cfg->contda && df <= ILD->cfg->contdf &&
		  q > optq*1.0001) {
		/* HP20010126  was q > opt, changed for numerical accuracy */
		/*  ensure to find _first_ optp with max q */
		/* HP20020228  permitting da==contda and df==contdf */
		/*  (this fix doesn't change behaviour) */
		optq = q;
		optp = i+1;
	      }
	    }
	  if (optp) {
	    linePred[il] = optp;
	    ILD->prevCont[optp-1] = 1;
	    numCont++;
	    if (ILD->debugLevel >= 3)
	      printf("add cont %2d -> %2d (optq=%9.7f)\n",
		     optp-1,il,optq);
	  }
	}
    }
    if (ILD->debugLevel >= 2)
      printf("IndiLineDecodeHarm: numCont=%d\n",numCont);
  }

  ILD->prevNumIndiLine = numLine;
  ILD->prevNumAllLine = *numAllLine;
  for (il=0; il<*numAllLine; il++) {
    ILD->prevLineFreq[il] = lineFreq[il];
    ILD->prevLineAmpl[il] = lineAmpl[il];
  }
  ILD->prevNumHarm = numHarm;
  for (i=0; i<numHarm; i++) {
    ILD->prevNumHarmLine[i] = numHarmLine[i];
    ILD->prevHarmLineIdx[i] = harmLineIdx[i];
  }
}


/* IndiLineDecodeFree() */
/* Free memory allocated by IndiLineDecodeInit(). */

void IndiLineDecodeFree (
  ILDstatus *ILD)		/* in: ILD status handle */
{
  IndiLineQuantFree(ILD->ILQ);

  free(ILD->envPara[0]);
  free(ILD->envPara);
  free(ILD->lineFreq);
  free(ILD->lineAmpl);
  free(ILD->lineValid);
  free(ILD->linePhase);
  free(ILD->linePhaseValid);
  free(ILD->lineEnv);
  free(ILD->linePred);
  free(ILD->numHarmLine);
  free(ILD->harmFreq);
  free(ILD->harmFreqStretch);
  free(ILD->harmLineIdx);
  free(ILD->harmEnv);
  free(ILD->harmPred);
  free(ILD->noisePara);
  free(ILD->envParaIndex);
  free(ILD->lineParaPred);
  free(ILD->lineParaEnv);
  free(ILD->lineParaIndex);
  free(ILD->lineParaEnhaIndex);
  free(ILD->lineParaNumBit);
  free(ILD->lineContFlag);
  free(ILD->lineParaFIndex);
  free(ILD->lineParaAIndex);
  free(ILD->transAmpl);
  free(ILD->harmFreqIndex);
  free(ILD->harmFreqStretchIndex);
  free(ILD->harmQuantFreqStretch);
  free(ILD->harmAmplIndex[0]);
  free(ILD->harmAmplIndex);
  free(ILD->harmQuantAmpl[0]);
  free(ILD->harmQuantAmpl);
  free(ILD->harmParaDeqOld);
  free(ILD->noiseParaIndex);
  free(ILD->noiseParaQuant);
  free(ILD->prevCont);
  free(ILD->linePhaseIndex);
  free(ILD->harmPhaseIndex);
  free(ILD->prevLineFreq);
  free(ILD->prevLineAmpl);
  free(ILD->prevLineValid);
  free(ILD->prevNumHarmLine);
  free(ILD->prevHarmLineIdx);

  free(ILD);
}


/* end of indilinedec.c */
