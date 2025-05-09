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

/* ntt_tools.h : tools Encoder */

/*** MODULE FUNCTION PROTOTYPE(S) ***/
#ifdef __cplusplus
extern "C" {
#endif
   
   void ntt_alfcep(int    p,     /* Input : the number of poles in LPC */
		   double alf[], /* Input : linear prediction coefficients */
		   double cep[], /* Output : LPC (all-pole modeled) cepstum */
		   int    n);    /* Input : the number of required points of LPC-cepstum*/
   
   void ntt_alflsp(/* Input */
		   int    p,     /* LSP analysis order */
		   double alf[], /* linear prediction coefficients */
		   /* Output */
		   double fq[]); /* LSP frequencies, 0 < fq[.] < pi */
   
   void ntt_alfref(/* Input */
		   int    p,        /* the number of poles in LPC */
		   double alf[],    /* linear prediction coefficients */
		   /* Output */
		   double ref[],    /* reflection coefficients */
		   double *_resid); /* linear prediction / PARCOR residual power */
   
   void ntt_cep2alf(/* Input */
		    int    npc,    /* Cepstrum order */
		    int    np,     /* LPC order */
		    double *cep,   /* LPC cepstrum cep[0] = cep_1 */
		    /* Output */
		    double *alf);  /* LPC coefficients alf[0] =alf_1 */
   
   void ntt_cholesky(/* In/Out */
		     double a[], 
		     /* Output */
		     double b[], 
		     /* Input */
		     double c[],
		     int    n);
   
   void ntt_chetbl(/* Output */
		   double tbl[], /* Chebyshev (Tchebycheff) polynomial coefficients */
		   /* Input */
		   int    n);
   
   void ntt_corref(int    p,        /* Input : LPC analysis order */
		   double cor[],    /* Input : correlation coefficients */
		   double alf[],    /* Output : linear predictive coefficients */
		   double ref[],    /* Output : reflection coefficients */
		   double *resid_); /* Output : normalized residual power */
   
   void ntt_cutfr(/* Input */
		  int    st,     /* start point    */
		  int    len,    /* block length */
		  int    ich,    /* channel number */
		  double frm[],  /* input frame */
		  int    numChannel,
		  int    block_size_samples,
		  /* Output */
		  double buf[]); /* output data buffer */
   
   void ntt_difddd(/* Input */
		   int    n,
		   double xx[], 
		   double yy[],
		   /* Output */
		   double zz[]);
   
   double ntt_dotdd(/* Input */
		    int    n,     /* dimension of data */
		    double xx[],  
		    double yy[]);
   
   void ntt_excheb(int    n,      /* Input */
		   double a[],    /* Input */
		   double b[],    /* Output */
		   double tbl[]); /* Input */
   
   void ntt_fft(/* In/Out */
		double xr[], /* real part of input data/ output data */
		double xi[], /* imaginary  part of input data/ output data */
		/* Input */
		int    m);   /* exponent */
   
   void ntt_fft842_m(/* Input */
		     int    m, 
		     /* In/Out */
		     double x[], 
		     double y[]);
   
   void ntt_hamwdw(/* Output */
		   double wdw[], /* Hamming window data */
		   /* Input */
		   int    n);    /* window length */
   
   void ntt_lagwdw(/* Output */
		   double wdw[], /* lag window data */
		   /* Input */
		   int    n,     /* dimension of wdw[.] */
		   double h);    /* ratio of window half value band width to samp\
				    ling frequency */
   
   void ntt_mulcdd(/* Input */
		   int    n,     /* dimension of data */
		   double xx,
		   double yy[],
		   /* Output */
		   double zz[]);
   
   void ntt_mulddd(/* Input */
		   int    n,      /* dimension of data */
		   double xx[],
		   double yy[],
		   /* Output */
		   double zz[]);
   
   void ntt_nrstep(double coef[], /* In/Out : coefficients of the n-th order polynomial */
		   int    n,      /* Input : the order of the polynomial */
		   double eps,    /* Input */
		   double *_x);   /* In/Out : initial value of the iteration */
   
   void ntt_r2tx(/* Input */
		 int    nthpo,
		 /* In/Out */
		 double *cr0, double *cr1,
		 double *ci0, double *ci1);
   
   void ntt_r4tx(/* Input */
		 int    nthpo,
		 /* In/Out */
		 double *cr0, double *cr1, double *cr2, double *cr3,
		 double *ci0, double *ci1, double *ci2, double *ci3 );
   
   void ntt_r8tx (/* Input */
		  int    nxtlt,
		  int    nthpo,
		  int    lengt,
		  /* In/Out */
		  double *cr0, double *cr1, double *cr2, double *cr3,
		  double *cr4, double *cr5, double *cr6, double *cr7,
		  double *ci0, double *ci1, double *ci2, double *ci3,
		  double *ci4, double *ci5, double *ci6, double *ci7);
   
   void ntt_setd(/* Input */
		 int    n,     /* dimension of data */
		 double c,
		 /* Output */
		 double xx[]);
   
   void ntt_sigcor(double *sig,  /* Input : signal sample sequence */
		   int    n,     /* Input : length of sample sequence*/
		   double *_pow, /* Output : power */
		   double cor[], /* Output : autocorrelation coefficients */
		   int    p);    /* Input : the number of autocorrelation points required*/
   
   void ntt_muldddre(/* Input */
		     int    n,
		     double x[],
		     double y[], 
		     /* Output */
		     double z[]);
#ifdef __cplusplus
}
#endif
