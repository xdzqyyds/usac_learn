/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      SOURCE_FILE:    PHI_LPC.C                                       */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Linear Prediction Subroutines                   */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "phi_cons.h"
#include "phi_nec_lpc.h"
#include "nec_abs_const.h"

/*======================================================================*/
/*     L O C A L     D A T A     D E F I N I T I O N S                  */
/*======================================================================*/
static float **HammingWindow;              /* For Haming Window         */
static float *PHI_GammaArrayBE;            /* For Bandwidth Expansion   */

/*----------------------------------------------------------------------*/
/*    Variables for dynamic threshold                                   */
/* ---------------------------------------------------------------------*/

/*======================================================================*/
/*     L O C A L    F U N C T I O N S     P R O T O T Y P E S           */ 
/*=======================================================================*/

/*======================================================================*/
/*   Function Prototype: PHI_CalcAcf                                    */
/*======================================================================*/
static void 
PHI_CalcAcf
(
double sp_in[],         /* In:  Input segment [0..N-1]                  */
double acf[],           /* Out: Autocorrelation coeffs [0..P]           */
int N,                  /* In:  Number of samples in the segment        */ 
int P                   /* In:  LPC Order                               */
);

/*======================================================================*/
/*   Function Prototype: PHI_LevinsonDurbin                             */
/*======================================================================*/
static int              /* Retun Value: 1 if unstable filter            */ 
PHI_LevinsonDurbin
( 
double acf[],           /* In:  Autocorrelation coeffs [0..P]           */         
double apar[],          /* Out: Polynomial coeffciients  [0..P-1]       */           
double rfc[],           /* Out: Reflection coefficients [0..P-1]        */
int P,                  /* In:  LPC Order                               */
double * E              /* Out: Residual energy at the last stage       */
);

/*======================================================================*/
/*   Function Prototype: NEC_lpc_analysis                               */
/*======================================================================*/
static void
NEC_lpc_analysis
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
float  HamWin[],                /* In:  Hamming Window                  */
long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
long  window_size,              /* In:  LPC Analysis-Window Size        */
long  lpc_order                 /* In:  Order of LPC                    */
);

/*======================================================================*/
/*   Function Definition:NEC_InitLpcAnalysisEncoder                     */
/*     Sep. 07 97  - Added for narrowband coder                         */
/*======================================================================*/
void
NEC_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Numberof LPC Analysis Frame     */
long  order,                    /* In:  Order of LPC                    */
float gamma_be                  /* In:  Bandwidth Expansion Coefficient */
)
{
    int     x;
    float   *pin;
    float   *pout;

    long i;
    
    /* -----------------------------------------------------------------*/
    /* Create Arrays for Hamming Window and gamma array                 */
    /* -----------------------------------------------------------------*/
    if
    (
    (( PHI_GammaArrayBE = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )
    )
    {
		printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		exit(1);
    }

    if((HammingWindow=(float **)calloc((unsigned int)n_lpc_analysis, sizeof(float *)))==NULL){
		printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		exit(1);
    }

    for(i=0;i<n_lpc_analysis;i++) {
        if((HammingWindow[i]=(float *)calloc((unsigned int)win_size[i], sizeof(float)))
                ==NULL){
		    printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		    exit(1);
        }

    /* -----------------------------------------------------------------*/       
    /* Generate Hamming Window For Lpc                                  */
    /* -----------------------------------------------------------------*/ 
/* using asymmetric windows */
		if((NEC_ASW_LEN1_FRQ16+NEC_ASW_LEN2_FRQ16)!=win_size[i]) {
		    printf("\n LPC window size is not matched\n");
		    exit(1);
		}

       	pout = HammingWindow[i];   	
       	for(x=0;x<NEC_ASW_LEN1_FRQ16;x++) {
          		    *pout++ = (float)(.54-.46*cos(NEC_PI*2.*((double)x)
		    /(double)(NEC_ASW_LEN1_FRQ16*2-1)));
       	}
       	for(x=0;x<NEC_ASW_LEN2_FRQ16;x++) {
          		    *pout++ = (float)(cos(NEC_PI*2.*((double)x)
		    /(double)(NEC_ASW_LEN2_FRQ16*4-1)));
       	}

    }
         
    /* -----------------------------------------------------------------*/       
    /* Generate Gamma Array for Bandwidth Expansion                     */
    /* -----------------------------------------------------------------*/       
    pin     = PHI_GammaArrayBE;
    pout    = PHI_GammaArrayBE;
    *pout++ = gamma_be;
    for(x=1; x < (int)order; x++)
    {
        *pout++ = gamma_be * *pin++;
    }

}

/*======================================================================*/
/* Function Definition: NEC_FreeLpcAnalysisEncoder                      */
/*     Sep. 07 97  - Added for narrowband coder                         */
/*======================================================================*/
void
NEC_FreeLpcAnalysisEncoder
(
long n_lpc_analysis
)
{
    long i;
    /* -----------------------------------------------------------------*/
    /* Free the arrays that were malloc'd                               */       
    /* -----------------------------------------------------------------*/

    for(i=0;i<n_lpc_analysis;i++) {
        free(HammingWindow[i]);
    }
    free(HammingWindow);

    free(PHI_GammaArrayBE);
}

 
/*======================================================================*/
/*   Function Definition:celp_lpc_analysis_bws                          */
/*======================================================================*/
void celp_lpc_analysis_bws (float PP_InputSignal[],         /* In:  Input Signal                    */
                            float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
                            float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
                            long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
                            long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
                            long  lpc_order,                /* In:  Order of LPC                    */
                            long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
)
{
    int i;
    
    
    for(i = 0; i < (int)n_lpc_analysis; i++)
    {
         NEC_lpc_analysis(PP_InputSignal,lpc_coefficients+lpc_order*(long)i, 
             first_order_lpc_par,
             HammingWindow[i], window_offsets[i], window_sizes[i],
             lpc_order); 
    }
}

/*======================================================================*/
/*   Function Definition:NEC_lpc_analysis                               */
/*======================================================================*/
void NEC_lpc_analysis (float PP_InputSignal[],         /* In:  Input Signal                    */
                       float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
                       float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
                       float HamWin[],                 /* In:  Hamming Window                  */
                       long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
                       long  window_size,              /* In:  LPC Analysis-Window Size        */
                       long  lpc_order                 /* In:  Order of LPC                    */
)
{

/* lag window */
static float lagWin_20[21] = {
(float) 1.0000100,
(float) 0.9997225,
(float) 0.9988903,
(float) 0.9975049,
(float) 0.9955685,
(float) 0.9930844,
(float) 0.9900568,
(float) 0.9864905,
(float) 0.9823916,
(float) 0.9777667,
(float) 0.9726235,
(float) 0.9669703,
(float) 0.9608164,
(float) 0.9541719,
(float) 0.9470474,
(float) 0.9394543,
(float) 0.9314049,
(float) 0.9229120,
(float) 0.9139889,
(float) 0.9046499,
(float) 0.8949091,
};

    /*------------------------------------------------------------------*/
    /*    Volatile Variables                                            */
    /* -----------------------------------------------------------------*/
    double *acf = 0;             /* For Autocorrelation Function        */
    double *reflection_coeffs;   /* For Reflection Coefficients         */
    double *LpcAnalysisBlock = 0;/* For Windowed Input Signal           */
    double *apars = 0;           /* a-parameters of double precision    */
    int k;
 
    /*------------------------------------------------------------------*/
    /* Allocate space for lpc, acf, a-parameters and rcf                */
    /*------------------------------------------------------------------*/
    if(NEC_LPC_ORDER_FRQ16!=lpc_order) {
        printf("\n irregular LPC order in BWS mode\n");
        exit(1);
    }

    if 
    (
    (( reflection_coeffs = (double *)malloc((unsigned int)lpc_order * sizeof(double))) == NULL ) ||
    (( acf = (double *)malloc((unsigned int)(lpc_order + 1) * sizeof(double))) == NULL ) ||
    (( apars = (double *)malloc((unsigned int)(lpc_order) * sizeof(double))) == NULL )  ||
    (( LpcAnalysisBlock = (double *)malloc((unsigned int)window_size * sizeof(double))) == NULL )
    )
    {
		printf("MALLOC FAILURE in Routine abs_lpc_analysis \n");
		exit(1);
    }
    
    /*------------------------------------------------------------------*/
    /* Windowing of the input signal                                    */
    /*------------------------------------------------------------------*/
    for(k = 0; k < (int)window_size; k++)
    {
        LpcAnalysisBlock[k] = (double)PP_InputSignal[k + (int)window_offset] * (double)HamWin[k];
    }    

    /*------------------------------------------------------------------*/
    /* Compute Autocorrelation                                          */
    /*------------------------------------------------------------------*/
    PHI_CalcAcf(LpcAnalysisBlock, acf, (int)window_size, (int)lpc_order);

    /* lag windowing */
    for(k=0;k<=lpc_order;k++) *(acf+k) *= *(lagWin_20+k);

    /*------------------------------------------------------------------*/
    /* Levinson Recursion                                               */
    /*------------------------------------------------------------------*/
    {
         double Energy = 0.0;
     
         PHI_LevinsonDurbin(acf, apars, reflection_coeffs,(int)lpc_order,&Energy);  
    }  
    
    /*------------------------------------------------------------------*/
    /* First-Order LPC Fit                                              */
    /*------------------------------------------------------------------*/
    *first_order_lpc_par = (float)reflection_coeffs[0];

    /*------------------------------------------------------------------*/
    /* Bandwidth Expansion                                              */
    /*------------------------------------------------------------------*/    
/* not used !
    for(k = 0; k < (int)lpc_order; k++)
    {
        lpc_coefficients[k] = (float)apars[k] * PHI_GammaArrayBE[k];
    }    
*/
    for(k = 0; k < (int)lpc_order; k++)
    {
        lpc_coefficients[k] = (float)apars[k];
    }    

    /*------------------------------------------------------------------*/
    /* Free the arrays that were malloced                               */
    /*------------------------------------------------------------------*/
    free(LpcAnalysisBlock);
    free(reflection_coeffs);
    free(acf);
    free(apars);
}


/*======================================================================*/
/*   Function Definition:PHI_CalcAcf                                    */
/*======================================================================*/
static void 
PHI_CalcAcf
(
double sp_in[],         /* In:  Input segment [0..N-1]                  */
double acf[],           /* Out: Autocorrelation coeffs [0..P]           */
int N,                  /* In:  Number of samples in the segment        */ 
int P                   /* In:  LPC Order                               */
)
{
    int	    i, l;
    double  *pin1, *pin2;
    double  temp;
    
    for (i = 0; i <= P; i++)
    {
		pin1 = sp_in;
		pin2 = pin1 + i;
		temp = (double)0.0;
		for (l = 0; l < N - i; l++)
	    	temp += *pin1++ * *pin2++;
		*acf++ = temp;
    }
    return;
}

/*======================================================================*/
/*   Function Definition:PHI_LevinsonDurbin                             */
/*======================================================================*/
static int              /* Retun Value: 1 if unstable filter            */ 
PHI_LevinsonDurbin
( 
double acf[],           /* In:  Autocorrelation coeffs [0..P]           */         
double apar[],          /* Out: Polynomial coeffciients  [0..P-1]       */           
double rfc[],           /* Out: Reflection coefficients [0..P-1]        */
int P,                  /* In:  LPC Order                               */
double * E              /* Out: Residual energy at the last stage       */
)
{
    int	    i, j;
    double  tmp1;
    double  *tmp;
    
    if ((tmp = (double *) malloc((unsigned int)P * sizeof(double))) == NULL)
    {
	printf("\nERROR in library procedure levinson_durbin: memory allocation failed!");
	exit(1);
    }
    
    *E = acf[0];
    if (*E > (double)0.0)
    {
		for (i = 0; i < P; i++)
		{
	    	tmp1 = (double)0.0;
	    	for (j = 0; j < i; j++)
			tmp1 += tmp[j] * acf[i-j];
	    	rfc[i] = (acf[i+1] - tmp1) / *E;
	    	if (fabs(rfc[i]) >= 1.0)
	    	{
			for (j = i; j < P; j++)
		    	rfc[j] = (double)0.0;
			free(tmp);
			return(1);  /* error in calculation */
	    	}
	    	apar[i] = rfc[i];
	    	for (j = 0; j < i; j++)
			apar[j] = tmp[j] - rfc[i] * tmp[i-j-1];
	    	*E *= (1.0 - rfc[i] * rfc[i]);
	    	for (j = 0; j <= i; j++)
			tmp[j] = apar[j];	
		}
		free(tmp);
		return(0);
    }
    else
    {
        for(i= 0; i < P; i++)
        {
            rfc[i] = 0.0;
            apar[i] = 0.0;
        }
		free(tmp);
		return(2);	    /* zero energy frame */
    }
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-07-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 30-08-96 R. Taori  Prefixed "PHI_" to several subroutines(MPEG req.) */
/* 07-11-96 N. Tanaka (Panasonic)                                       */
/*                    Added several modules for narrowband coder (PAN_) */
/* 14-11-96 A. Gerrits Introduction of dynamic threshold                */
