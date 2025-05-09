/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

#include <stdio.h>  /* added by K.Mano */
#include <math.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_tools.h"


/* --- ntt_alfcep ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 08    *
*               coded by S.Sagayama,                1/08/1976    *
******************************************************************
 ( C version coded by S. Sagayama,  6/19/1986 )

   description:
     * conversion of "alf  into "cep".
     * computation of cepstum of AR-process specified by "alf".
     * computation of sum of m-th power of LPC poles, m=1,..,n,
       since:         1     IP                m
             cep(m)= --- * sum ( (LPC pole(i))  ).
                      m    i=1

   synopsis:
          -------------------------
          ntt_alfcep(ip,alf,cep,n)
          -------------------------
   IP      : input.        integer.
             the order of analysis; the number of poles in LPC;
             the degree of freedom of the model - 1.
   alf[.]  : input.        double array : dimension=IP.
             linear prediction coefficients; ar parameters.
             alf[0] is implicitly assumed to be 1.0.
   cep[.]  : output.       double array : dimension=n.
             LPC (all-pole modeled) cepstum.
             cep[0] is implicitly assumed to be alog(resid/pi),
             where "resid" is residual power of LPC/PARCOR.
   n       : input.        integer.
             the number of required points of LPC-cepstum.
             note that the degree of freedom remains IP.
*/

void ntt_alfcep(int p,        /* Input : the number of poles in LPC */
                double alf[], /* Input : linear prediction coefficients */
                double cep[], /* Output : LPC (all-pole modeled) cepstum */
                int n)        /* Input : the number of required points of LPC-cepstum */

{ double ss; int i,m;
  cep[1]= -alf[1]; if(p>n) p=n;
  for(m=2;m<=p;m++)
  { ss= -alf[m]*m; for(i=1;i<m;i++) ss-=alf[i]*cep[m-i]; cep[m]=ss; }
  for(m=p+1;m<=n;m++)
  { ss=0.0; for(i=1;i<=p;i++) ss-=alf[i]*cep[m-i]; cep[m]=ss; }
  for(m=2;m<=n;m++) cep[m]/=m; }


/* --- ntt_alflsp ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 40    *
*               coded by S.Sagayama,                 5/5/1982    *
******************************************************************
 ( C version coded by S.Sagayama )
 ( revised (convergence trial loop) by S.Sagayama, 6/24/1987 )

   description:
     * conversion of "alf" into "lsp".
     * computation of line spectrum pair frequencies from linear
       prediction coefficients.
     * computation of roots of p(x) and q(x):
                              p
           P(x) = z * a(z) - z   * a(1/z)
                              p
           Q(x) = z * a(z) + z   * a(1/z)  
       where
                   p           p-1
           A(z) = z   + a[1] * z     + .... + a[p].
       in case p=even, the roots of p(x) are cosine of: 
          0, freq[2], freq[4], ... , freq[p];
       and the roots of p(x) are cosine of:
          freq[1], freq[3], ... ,freq[p-1], pi.
     * the necesary and sufficient condition for existence of
       the solution is that all the roots of polynomial A(z) lie
       inside the unit circle.

   synopsis:
          ------------------------
          void ntt_alflsp(p,alf,freq)
          ------------------------
   p      : input.        integer.     2 =< p =< 14.
             the order of analysis; the number of poles in LPC;
   alf[.]  : input.        double array : dimension=p.
             linear prediction coefficients; ar parameters.
             alf[0] is implicitly assumed to be 1.0.
   freq[.] : output.       double array : dimension=p.
             LSP frequencies, ranging between 0 and 1;
             CSM frequencies under two diferrent conditions,
               p=even:   freq(0)=0,order=n / freq(n+1)=1,order=n
               p=odd:    order=n / freq(0)=0,freq(n+1)=1,order=n-1,
             where n=[(p+1)/2].
             increasingly ordered.  freq(1)=<freq(2)=<.....

   note: (1) p must not be greater than 20. (limiation in "ntt_excheb")
         (2) subroutine call: "ntt_excheb", "ntt_nrstep".
*/

void ntt_alflsp(/* Input */
                int    p,      /* LSP analysis order */
	        double alf[],  /* linear predictive coefficients */
                /* Output */
	        double fq[] )  /* LSP frequencies, 0 < fq[.] < pi */
{
  int i,j,k,km,kp,nm,np,flag;
  double b,x,y,eps,opm[50],opp[50],opm1[50],opp1[50];
  static int p0=0;
  static double tbl[200],eps0=0.00001;
  if(p>p0) { p0=p; ntt_chetbl(tbl,(p+1)/2); } /* making Chebyshev coef table */

  np=p/2; nm=p-np;
  if(nm==np) /* ---- in case of p=even ---- */
  { opp[1]=alf[1]-alf[p]+1.0; opm[1]=alf[1]+alf[p]-1.0;
    for(i=2;i<=nm;i++)
    { b=alf[p+1-i]; opp[i]=alf[i]-b+opp[i-1]; opm[i]=alf[i]+b-opm[i-1]; } }
  else /* ---- in case of p=odd ---- */
  { opm[1]=alf[1]+alf[p];
    if(nm>1)
    { opp[1]=alf[1]-alf[p]; opm[2]=alf[2]+alf[p-1];
      if(nm>2)
      { opp[2]=alf[2]-alf[p-1]+1.0;
        for(i=3;i<=nm;i++) opm[i]=alf[i]+alf[p+1-i];
        for(i=3;i<=np;i++) opp[i]=alf[i]-alf[p+1-i]+opp[i-2]; } } }
  if(nm>1) ntt_excheb(np,opp,opp,tbl); ntt_excheb(nm,opm,opm,tbl);
  if(p==1) fq[p]= -opm[1];
  else if(p<=2) { fq[p-1]= -opm[1]; fq[p]= -opp[1]; }
  else /* ---- find roots of the polynomials ---- */
  { eps=eps0;
    for(k=0;k<6;k++) /* trying 6 times while LSP invalid */
    { for(i=1;i<=nm;i++) opm1[i]=opm[i];
      for(i=1;i<=np;i++) opp1[i]=opp[i];
      kp=np; km=nm; j=0; x=1.0; y=1.0; flag=1;
      for(i=1;i<=p;i++)
      { if((j=1-j)!=0) { ntt_nrstep(opm1,km,eps,&x); km--; }
        else { ntt_nrstep(opp1,kp,eps,&x); kp--; }
        if(x>=y || x<= -1.0) { flag=0; break; }
        else { y=x; fq[i]=x; } }
      if(flag) break;
      else { eps*=0.5; fprintf(stderr,"ntt_alflsp(%d)",k); } /* 1/2 criterion */
  } }
  for(i=1;i<=p;i++) fq[i]=acos(fq[i]);
}


/* --- ntt_alfref ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 03    *
*               coded by S.Sagayama,              autumn/1975    *
******************************************************************
 ( C version coded by S.Sagayama, 6/19/1986 )

   description:
     * conversion of "alf" into "ref".
     * discrimination of stability of all-pole filter specified
       by "alf".  If every ref[.] satisfies -1<ref[.]<1.
     * discrimination if sequence "alf" is of minimum phase or not.

   synopsis:
          ------------------------
          ntt_alfref(p,alf,ref,&resid)
          ------------------------
   p       : input.        integer.
             the order of analysis; the number of poles in LPC;
             the degree of freedom of the model - 1.
   alf[.]  : output.       double array : dimension=p+1.
             linear prediction coefficients; AR parameters.
             alf[0] is implicitly assumed to be 1.0.
   ref[.]  : output.       double array : dimension=p+1.
             PARCOR coefficients; reflection coefficients.
             all of ref[.] range between -1 and 1.
   resid   : output.       double.
             linear prediction / PARCOR residual power;
             reciprocal of power gain of PARCOR/LPC/LSP all-pole
             filter.
*/

void ntt_alfref(/* Input */
                int p,          /* the number of poles in LPC */
                double alf[],   /* linear prediction coefficients */
                /* Output */
                double ref[],   /* reflection coefficients */
                double *_resid) /* linear prediction / PARCOR residual power */
{ int i,j,n; double r,rr,u;
  *_resid=1.0;
  for(n=1;n<=p;n++) ref[n]=alf[n];
  for(n=p;n>0;n--)
  { r=(ref[n]= -ref[n]); u=(1.0-r)*(1.0+r);
    i=0; j=n;
    while(++i <= --j)
    { rr=ref[i]; ref[i]=(rr+r*ref[j])/u;
      if(i<j) ref[j]=(ref[j]+r*rr)/u; }
    *_resid*=u; } }


/* --- ntt_cep2alf ---
******************************************************************
*/
/* LPC cep to alf by solving normal equation */
/*
 mata' * mata * alf = - mata' * cep
 mata =  1         0         0       0
	 c[1]/2    1         0       0
         c[2]*2/3  c[1]/3    1       0
         c[3]*3/4  c[2]*2/4  c[1]/4  1
            ...........
         c[n]*(n-1)/n  .....         c[n-p]*(n-p)/n

 cep'  = c[0],c[1], c[2],      c[n]
 alf'  = alf[1],alf[1], alf[2] .. alf[p]


*/

#define LPC_MAX  (20+1)
#define MAT_MAX  (40+1)

void ntt_cholesky(/* In/Out */
                  double a[],
                  /* Output */
                  double b[], 
                  /* Input */
                  double c[],
                  int n)     
{ 
  int i,j,k;
  double t[LPC_MAX*LPC_MAX], invt[LPC_MAX];
  register double acc;
  static double eps=1.e-16;

  t[0] = sqrt(a[0]+eps);
  invt[0] = 1./t[0];
  for(k=1; k<n; k++) t[k*n] = a[k*n] * invt[0];
  for(i=1;i<n;i++) {
     acc = a[i*n+i]+eps;
     for(k=0; k<i; k++) acc -= t[i*n+k] * t[i*n+k];
     t[i*n+i] = sqrt(acc);
     invt[i] = 1./t[i*n+i] ;
     for(j=i+1;j<n;j++){
        acc = a[j*n+i]+eps;
        for(k=0; k<i; k++) acc -= t[j*n+k] * t[i*n+k];
	t[j*n+i] = acc * invt[i];
     }
  } 
  for(i=0;i<n;i++) {
     acc = c[i];
     for(k=0; k<i; k++) acc -= t[i*n+k] * a[k];
     a[i] = acc * invt[i];
  } 
  
  for(i=n-1;i>=0;i--) {
     acc = a[i];
     for(k=i+1; k<n; k++) acc -= t[k*n+i] * b[k];
     b[i] = acc * invt[i];
  } 
}


void ntt_cep2alf(/* Input */
              int npc, 		/* Cepstrum order */
	      int np, 		/* LPC order      */
	      double *cep, 	/* LPC cepstrum cep[0] = cep_1 */
              /* Output */
	      double *alf)     	/* LPC coefficients alf[0] =alf_1     */

{
 double  mata[LPC_MAX*MAT_MAX]; 
 double  matb[LPC_MAX*LPC_MAX], matc[LPC_MAX];
 register double acc;
 int inp, inpc, k;
 double invpc; 
 int inpcc; 

    for(inpc=0; inpc<npc; inpc++){
      invpc= 1./(double)(inpc+1);
      inpcc = (inpc < np) ? inpc:np;
      for(inp=0; inp<inpcc; inp++) {  
         mata[inpc*np + inp ] = (double)(inpc-inp)*invpc*cep[inpc-inp];
      }
    }

    for(inp=0;inp<np;inp++) {
        for(inpc=0;inpc<inp;inpc++) { 
	  acc=mata[inp*np+inpc]; 
          for(k=inp+1;k<npc;k++) acc+=mata[k*np+inp]*mata[k*np+inpc]; 
	  matb[inp*np+inpc]=acc; 
        }
	acc=1.; 
        for(k=inp+1;k<npc;k++) acc+=mata[k*np+inp]*mata[k*np+inp]; 
	matb[inp*np+inpc]=acc; 
       
        acc=cep[inp+1];
	for(inpc=inp+1;inpc<npc;inpc++) acc+=mata[inpc*np+inp] * cep[inpc+1];
	matc[inp] = -acc;
    }
    alf[0]=1.0;
    ntt_cholesky(matb, alf+1, matc, np);
}


/* --- ntt_chetbl ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 41    *
*               coded by S.Sagayama,                 5/5/1982    *
******************************************************************
 ( C version coded by S.Sagayama, 2/28/1987 )

   description:
     * makes a Tchebycheff (Chebyshev) polynomial coefficient
       table.  It is equivalent to the expansion of cos(nx)
       into polynomials of (cos x).  It is given by:
                   [k/2]     i  k-2i-1  n (k-i-1)!         k-2i
         cos k x =  sum  (-1)  2       ------------ (cos x)
                    i=0                 i! (k-2i)!
                         --------------------------
       (this function computes this (^ t[k,i]) for k=0..n; i=0..[k/2].)

       t[k,i+1] = - t[k,i] * (k-2i)(k-2i-1)/4(i+1)(k-i-1)

     * makes a table for
       expansion of a linear combination of Chebycheff polynomials
       into a polynomial of x:  suppose a linear combination of
       Tchebycheff(Chebyshev) polynomials:

           S(x) = T(x,n) + a[1] * T(x,n-1) + .... + a[n] * T(x,0)

       where T(x,k) denotes k-th Tchebycheff polynomial of x,
       then, expand each Chebycheff polynomial and get a polynomial
       of x:
                   n           n-1
           S(x) = x  + b[1] * x    + .... + b[n].

     * this problem is equivalent to the conversion of a linear
                         k        k
       combination of ( z  + 1 / z  ) into a polynomial of
       ( z + 1/z ).
     * this problem is equivalent to the conversion of a linear
       combination of cos(k*x), k=1,...,n, into a polynomial of cos(x).
     * table contents:
        0)    1/2
        1)          1  
        2)      2      -1
        3)          4     -3  
        4)      8      -8     1
        5)         16    -20     5
        6)     32     -48    18    -1
        7)         64   -112    56    -7
        8)    128    -256   160   -32     1
        9)        256   -576   432  -120     9
       10)    512   -1280  1120  -400    50    -1
                  ..................................

   synopsis:
          -------------
          ntt_chetbl(coef,n)
          -------------
   n       : input.        integer.   n =< 10.
   tbl[.]  : output.       double array : dimension=~= (n+1)(n+2)/4
             Chebyshev (Tchebycheff) polynomial coefficients.
*/

void ntt_chetbl(/* Output */
                double tbl[], /* Chebyshev (Tchebycheff) polynomial coefficients */
                /* Input */
                int n)

{ int i,j,k,l,m; double p,t;
  k=0; p=0.5;
  for(i=0;i<=n;i++)
  { t=p; p*=2.0; l=i/2; m=0;
    for(j=0;j<=l;j++)
    { tbl[k++]=t; if(j<l) { t*=(m-i)*(i-m-1); t/=(j+1)*(i-j-1)*4; m+=2; }
  } }
}


/*--- ntt_corref ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 01    *
*               coded by S.Sagayama,           september/1976    *
******************************************************************
 ( C version coded by S.Sagayama; revised,  6/20/1986, 2/4/1987 )

 - description:
   * conversion of "cor" into "ref".
     "" is simultaneously obtained.
   * computation of PARCOR coefficients "ref" of an arbitrary
     signal from its autocorrelation "cor".
   * computation of orthogonal polynomial coefficients from 
     autocorrelation function.
   * recursive algorithm for solving toeplitz matrix equation.
     example(p=3):  solve in respect to a1, a2, and a3.
         ( v0 v1 v2 )   ( a1 )     ( v1 )
         ( v1 v0 v1 ) * ( a2 ) = - ( v2 )
         ( v2 v1 v0 )   ( a3 )     ( v3 )
     where v0 = 1, vj = cor(j), aj = (j).
   * recursive computation of coefficients of a polynomial:(ex,p=4)
               | v0   v1   v2   v3   |    /      | v0   v1   v2 |
     A(z)= det | v1   v0   v1   v2   |   /   det | v1   v0   v1 |
               | v2   v1   v0   v1   |  /        | v2   v1   v0 |
               | 1    z   z**2 z**3  | /       
     where A(z) = z**p + (1) * z**(p-1) + ... + (p).
     note that the coefficient of z**3 is always equal to 1.
   * Gram-Schmidt orthogonalization of a sequence, ( 1, z, z**2,
     z**3, ... ,z**(2n-1) ), on the unit circle, giving their inner
     products:
                       k    l
          v(k-l) = ( z  , z   ),    0 =< k,l =< p.
     where v(j) = cor(j), v(0) = 1.
     coefficients of p-th order orthogonal polynomial are obtained
     through this subroutine. ((1),...,(p))
   * computation of reflection coefficients ref(i) at the boundary
     of the i-th section and (i+1)-th section in acoustic tube
     modeling of vocal tract.
   * the necesary and sufficient condition for existence of
     solution is that toeplitz matrix ( v(i-j) ), i,j=0,1,... 
     be positive definite.

 - synopsis:
          ----------------------------
          ntt_corref(p,cor,alf,ref,&resid)
          ----------------------------
   p       : input.        integer.
             the order of analysis; the number of poles in LPC;
             the degree of freedom of the model - 1.
   cor[.]  : input.        double array : dimension=p+1
             autocorrelation coefficients.
             cor[0] is implicitly assumed to be 1.0.
   alf[.]  : output.       double array : dimension=p+1
             linear prediction coefficients; AR parameters.
             [0] is implicitly assumed to be 1.0.
   ref[.]  : output.       double array : dimension=p+1
             PARCOR coefficients; reflection coefficients.
             all of ref[.] range between -1 and 1.
   resid   : output.       double.
             linear prediction / PARCOR residual power;
             reciprocal of power gain of PARCOR/LPC/LSP all-pole filter.

 - note: * if p<0, p is regarded as p=0. then, resid=1, and
           alf[.] and ref[.] are not obtained.
*/

void ntt_corref(int p,          /* Input : LPC analysis order */
	    double cor[],   /* Input : correlation coefficients */
	    double alf[],   /* Output : linear predictive coefficients */
	    double ref[],   /* Output : reflection coefficients */
	    double *resid_) /* Output : normalized residual power */
{
  int i,j,k;
  double resid,r,a;
  if(p>0)
  { ref[1]=cor[1]; alf[1]= -ref[1]; resid=(1.0-ref[1])*(1.0+ref[1]);
    for(i=2;i<=p;i++)
    { r=cor[i]; for(j=1;j<i;j++) r+=alf[j]*cor[i-j];
      alf[i]= -(ref[i]=(r/=resid));
      j=0; k=i;
      while(++j<=--k) { a=alf[j]; alf[j]-=r*alf[k]; if(j<k) alf[k]-=r*a; }
      resid*=(1.0-r)*(1.0+r); }
    *resid_=resid;
  }
  else *resid_=1.0;
}


/* --- ntt_cutfr ---
******************************************************************
*/

void ntt_cutfr(int    st,      /* Input  --- Start point */
	   int    len,     /* Input  --- Block length */
	   int    ich,     /* Input  --- Channel number */
	   double frm[],   /* Input  --- Input frame */
	   int    numChannel,
	   int    block_size_samples,
	   double buf[])   /* Output --- Output data buffer */
{
    /*--- Variables ---*/
    int stb, sts, edb, nblk, iblk, ibuf, ifrmb, ifrms;

    stb = (st/block_size_samples)*numChannel + ich;      /* start block */
    sts = st % block_size_samples;            /* start sample */
    edb = ((st+len)/block_size_samples)*numChannel + ich;        /* end block */
    nblk = (edb-stb)/numChannel;             /* number of overflow */

    ibuf=0; ifrmb=stb; ifrms=sts;
    for ( iblk=0; iblk<nblk; iblk++ ){
	while( ifrms < block_size_samples )  buf[ibuf++] = frm[(ifrms++)+ifrmb*block_size_samples];
	ifrms = 0;
	ifrmb += numChannel;
    }
    while( ibuf < len )	buf[ibuf++] = frm[(ifrms++)+ifrmb*block_size_samples];

}


/* --- ntt_difddd ---
******************************************************************
*/

void ntt_difddd(/* Input */
                int n,
                double xx[],
                double yy[],
                /* Output */
                double zz[])
{
   double
        *p_xx,
        *p_yy,
        *p_zz;
   register int
         iloop_fr;

   p_xx = xx;
   p_yy = yy;
   p_zz = zz;
   iloop_fr = n;
   do
   {
      *(p_zz++) = *(p_xx++) - *(p_yy++);
   }
   while ((--iloop_fr) > 0);
}


/* --- ntt_dotdd ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*               coded by S.Sagayama,                3/10/1987    *
******************************************************************
 ( C version coded by S.Sagayama, 3/10/1987)

   description:
     * array arithmetic :  xx * yy
       i.e. sum of xx[i] * yy[i] for i=0,n-1

   synopsis:
          ---------------------
          double ntt_dotdd(n,xx,yy)
          ---------------------

    n      : dimension of data
    xx[.]  : input data array (double)
    yy[.]  : input data array (double)
*/

double ntt_dotdd(/* Input */
                 int n,       /* dimension of data */
                 double xx[],
                 double yy[])
{ int i; double s;
  s=0.0; for(i=0;i<n;i++) s+=xx[i]*yy[i]; return(s); }


/* --- ntt_excheb ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 41    *
*               coded by S.Sagayama,                 5/5/1982    *
******************************************************************
 ( C version coded by S.Sagayama, 2/28/1987 )

   description:
     * expansion of a linear combination of Chebycheff polynomials
       into a polynomial of x:  suppose a linear combination of
       Tchebycheff(Chebyshev) polynomials:

           S(x) = T(x,n) + a[1] * T(x,n-1) + .... + a[n] * T(x,0)

       where T(x,k) denotes k-th Tchebycheff polynomial of x,
       then, expand each Chebycheff polynomial and get a polynomial
       of x:
                   n           n-1
           S(x) = x  + b[1] * x    + .... + b[n].

     * this problem is equivalent to the conversion of a linear
                         k        k
       combination of ( z  + 1 / z  ) into a polynomial of
       ( z + 1/z ).
     * this problem is equivalent to the conversion of a linear
       combination of cos(k*x), k=1,...,n, into a polynomial of cos(x).

   synopsis:
          -------------
          ntt_excheb(n,a,b)
          -------------
   n       : input.        integer.   n =< 10.
   a[.]    : input.        double array : dimension=n.
             implicitly, a[0]=1.0.
   b[.]    : output.       double array : dimension=n.
             implicitly, b[0]=1.0.
   coef[.] : input.        double array : dimension=~=(n+1)(n+2)/4
             A table of Chebyshev polynomial coefficients which looks like:
             .5,                                    0
             1.,                                    1
             2.,                                    2
             4.,-3.,                                3
             8.,-8.,1.,                             4
             16.,-20.,5.,                           5
             32.,-48.,18.,-1.,                      6
             64.,-112.,56.,-7.,                     7
             128.,-256.,160.,-32.,1.,               8
             256.,-576.,432.,-120.,9.,              9
             512.,-1280.,1120.,-400.,50.,-1.,      10
             ............                           n

   note: (1) arrays "a" and "b" can be identical.
*/

void ntt_excheb(int n,        /* Input */
                double a[],   /* Input */
                double b[],   /* Output */
                double tbl[]) /* Input */
{ int i,j,k; double c,t;
  if(n<=0) return;
  k=0;
  for(i=n;i>=1;i--)
  { t=a[i]; b[i]=0.0; for(j=i;j<=n;j+=2) b[j]+=t*tbl[k++]; }
  c=tbl[k++]; for(j=2;j<=n;j+=2) b[j]+=tbl[k++];
  for(j=1;j<=n;j++) b[j]/=c; 
}


/* --- ntt_hamwdw ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*               coded by s.sagayama,                4/23/1981    *
******************************************************************
 ( C version coded by S. Sagayama,  6/20/1986 )

   description:
     * Hamming window generation.

   synopsis:
          ------------------
          void ntt_hamwdw(wdw,n)
          ------------------
   n        : input.        integer.
              the dimension of data; data length; window length.
   wdw[.]   : output.       double array : dimension=n.
              Hamming window data.
*/

void ntt_hamwdw(/* Output */
                double wdw[],  /* Hamming window data */
                int n)         /* window length */
{ int i;
  double d,pi=3.14159265358979323846264338327950288419716939;
  if(n>0)
  { d=2.0*pi/n;
    for(i=0;i<n;i++) wdw[i]=0.54-0.46*cos(d*i); } }


/* --- ntt_lagwdw ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*               coded by S.Sagayama,                7/27/1978    *
******************************************************************
 ( C version coded by S. Sagayama,  6/21/1986 )

   description:
     * lag window (pascal) data generation.

   synopsis:
          ---------------
          ntt_lagwdw(wdw,n,h)
          ---------------

   wdw[.]  : output.       double array : dimension=n+1.
             lag window data. wdw[0] is always 1.
   n       : input.        integer.
             dimension of wdw[.].  (the order of LPC analysis.)
   h       : input.        double.
              0.0 < h < 1.0 .... if h=0, wdw(i)=1.0 for all i.
             ratio of window half value band width to sampling frequency.
             example: If lag window half value band width = 100 Hz and
             sampling frequency = 8 kHz, then h = 100/8k = 1/80 =0.0125

   revised by s.sagayama,       1982.7.19
*/

void ntt_lagwdw(/* Output */
                double wdw[],  /* lag window data */
                /* Input */
                int n,         /* dimension of wdw[.] */
                double h)      /* ratio of window half value band width to sampling frequency */
{ int i;
  double pi=3.14159265358979323846264338327959288419716939;
  double a,b,w;
  if(h<=0.0) for(i=0;i<=n;i++) wdw[i]=1.0;
  else
  { a=log(0.5)*0.5/log(cos(0.5*pi*h));
    a=(double)((int)a);
    w=1.0; b=a; wdw[0]=1.0;
    for(i=1;i<=n;i++)
    { b+=1.0; w*=a/b; wdw[i]=w; a-=1.0; } } }


/* --- ntt_mulcdd ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*               coded by N.Iwakami,                 m/dd/19yy    *
******************************************************************
 ( C version coded by N.Iwakami, 4/4/1994)

   description:
     * array arithmetic :  c * xx = zz
       i.e. zz[i]=c*xx[i] for i=0,n-1

   synopsis:
          ------------------
          ntt_mulcdd(n,c,xx,zz)
          ------------------

    n      : dimension of data
    c      : input data (double)
    xx[.]  : input data array (double)
    zz[.]  : output data array (double)
*/

void ntt_mulcdd(/* Input */
                int n,        /* dimension of data */
		double c,     
		double xx[],
		/* Output */
		double zz[])
{ int i; for(i=0;i<n;i++) zz[i]=c*xx[i]; }


/* --- ntt_mulddd ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*               coded by S.Sagayama,                m/dd/19yy    *
******************************************************************
 ( C version coded by S.Sagayama, 2/5/1987)

   description:
     * array arithmetic :  xx * yy = zz
       i.e. zz[i]=xx[i]*yy[i] for i=0,n-1

   synopsis:
          ------------------
          ntt_mulddd(n,xx,yy,zz)
          ------------------

    n      : dimension of data
    xx[.]  : input data array (double)
    yy[.]  : input data array (double)
    zz[.]  : output data array (double)
*/

void ntt_mulddd(/* Input */
		int n,        /* dimension of data */
		double xx[],
		double yy[],
		/* Output */
		double zz[])
{
    int ii;
    for (ii=0; ii<n; ii++ ) zz[ii] = xx[ii]*yy[ii];
}


/* --- ntt_nrstep ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 42    *
*               coded by S.Sagayama,               11/14/1980    *
******************************************************************
 ( C version coded by S.Sagayama, 2/25/1987 )
 ( revised (argument 'eps' has been added) by S.Sagayama, 6/24/1987 )

   description:
     * apply a single step of newton-raphson iteration to an
       algebraic equation and divide it by ( x - root ).
     * the necesary and sufficient condition for existence of
       the solution is that all the roots of polynomial a(z) lie
       inside the unit circle.

   synopsis:
          -----------------
          ntt_nrstep(coef,n,&x)
          -----------------
   coef[.] : input/output. double array : dimension=n+1.
             coefficients of the n-th order polynomial (input).
             coefficients of new (n-1)-th order polynomial (output).
             new polynomial(x) = given polynomial(x)/(x-root).
             coef[0] is implicitly assumed to be 1.0.
   n       : input.        integer.
             the order of the polynomial (n>0).
   x       : input/output. double.
             initial value of the iteration (input),
             the root (output).

   note: * originally coded by F.Itakura (original name was "newton"),
           renamed and revised by S.Sagayama,  1981.11.14. 
         * effective dimension of "coef" changes into (n-1) after
           calling this subroutine.  coef(n) is meaningless.
*/

void ntt_nrstep(double coef[], /* In/Out : coefficients of the n-th order polynomial */
		int n,      /* Input : the order of the polynomial */
		double eps, /* Input */
		double *_x) /* In/Out : initial value of the iteration */
{ int i; double x,dx,f,d; /* standard setting: eps=0.0000001 */
  if(n<2) { *_x= -coef[1]; return; } x= *_x; /* initial value */
  do /* Newton-Raphson iteration */
  { d=1.0; f=x+coef[1]; for(i=2;i<=n;i++) { d=d*x+f; f=f*x+coef[i]; }
    dx=f/d; x-=dx; } while(dx>eps);
  coef[1]=coef[1]+x; for(i=2;i<n;i++) coef[i]+=x*coef[i-1];
  *_x=x; } /* returns the solution and the (n-1)th order polynomial */


/* --- ntt_setd ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # nn    *
*    originally coded by S.Sagayama,               12/28/1987    *
******************************************************************
 ( C version coded by S.Sagayama, 12/28/1987)
 ( Modified by N.Iwakami, 27/8/1996:
   changed from vector-to-vector-copy to constant-to-vector-copy )

   description:
     * copy an array :  const => xx
       i.e. xx[i]=const for i=0,n-1

   synopsis:
          --------------
          ntt_setd(n,c,xx)
          --------------

    n      : dimension of data
    c      : input data (double)
    xx[.]  : output data array (double)
*/

void ntt_setd(/* Input */
	      int n,         /* dimension of data */
	      double c,
	      /* Output */
	      double xx[])
{
    int i;
    for(i=0;i<n;i++) xx[i]=c;
}


/* --- ntt_sigcor ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 50    *
*               coded by S.Sagayama,                7/25/1979    *
*               modified by K. Mano                 4/24/1990    *
******************************************************************
 ( C version coded by S.Sagayama, 2/05/1987 )

   description:
     * conversion of "sig" into "cor".
     * computation of autocorrelation coefficients "cor" from
       signal samples.

   synopsis:
          ------------------------
          ntt_sigcor(n,sig,&pow,cor,p)    : old
          ntt_sigcor(sig,n,&pow,cor,p)    : modified 
          ------------------------
   n       : input.        integer.
             length of sample sequence.
   sig[.]  : input.        double array : dimension=n. 
             signal sample sequence.
   pow     : output.       double.
             power. (average energy per sample).
   cor[.]  : output.       double array : dimension=lmax
             autocorrelation coefficients.
             cor[0] is implicitly assumed to be 1.0.
   p       : input.        integer.
             the number of autocorrelation points required.
*/

void ntt_sigcor(double *sig, /* Input : signal sample sequence */
		int n,       /* Input : length of sample sequence*/
		double *_pow,/* Output : power */
		double cor[],/* Output : autocorrelation coefficients */
		int p)       /* Input : the number of autocorrelation points r\
equired*/ 
{ 
   int k; 
   register double sqsum,c, dsqsum;

      sqsum = 1.e-35;
   if (n>0) {
      sqsum = ntt_dotdd(n, sig, sig)+1.e-35;
      dsqsum = 1./sqsum;
      k=p;
      do{
	 c = ntt_dotdd(n-k, sig, sig+k);
	 cor[k] = c*dsqsum;
      }while(--k);
   }
   *_pow = (sqsum-1.e-35)/(double)n; 
}


/* --- ntt_muldddre ---
******************************************************************
*/

void ntt_muldddre(/* Input */
                  int n,
                  double x[],
                  double y[],
                  /* Output */
                  double z[])
{
    do{ *(z++) = *(x++) * *(y--);}while(--n);
}

