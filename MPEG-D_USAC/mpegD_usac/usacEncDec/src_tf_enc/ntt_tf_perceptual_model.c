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


#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_enc_para.h"
#include "ntt_encode.h"
#define GAMMA_O 0.7
void ntt_tf_perceptual_model(/* Input */
			 double lpc_spectrum[],      /* LPC spectrum*/
			 double bark_env[],          /* Bark-scale envelope*/
			 double gain[],              /* gain factor */
			 int       w_type,           /* block type */
			 double spectrum[],          /* spectrum */
			 double pitch_sequence[],    /* periodic peak components */
			 int    numChannel,
			 int    block_size_samples,
			 double bandUpper,
			 /* Output */
			 double perceptual_weight[]) /* perceptual weight */
{
    /*--- Variables ---*/
    int    ismp, i_sup, top, isf, subtop;
    double ratio;
    float ftmp;

    switch(w_type){
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
	for (i_sup=0; i_sup<numChannel; i_sup++){
	    top = i_sup * block_size_samples;
         
{
       double tmp[2048];
       double ratiof, ratiol, add, prod, add0;
       int nfr;
       ftmp = (float)bandUpper;
       ftmp *= block_size_samples;
       nfr = (int)ftmp;
       add=0.0; prod=1.0;
       for (ismp=0; ismp<nfr; ismp++){
          tmp[ismp] = fabs(spectrum[ismp+top])+0.001;
           add += tmp[ismp]/nfr;
	   prod *= pow(tmp[ismp], 1./(double)nfr);
       }
       ratiof = add/prod; 
       add=0.0; prod=1.0;
       for (ismp=0; ismp<nfr; ismp++){
           tmp[ismp] = fabs(1./lpc_spectrum[ismp+top])+0.001;
           add += tmp[ismp]/nfr;
	   prod *= pow(tmp[ismp], 1./(double)nfr);
       }
       ratiol = add/prod; 
       tmp[0] = fabs(spectrum[top]);
       for (ismp=1; ismp<nfr; ismp++){
           tmp[ismp] = fabs(spectrum[ismp+top])* 0.8
             + fabs(spectrum[ismp+top-1])*0.2;
       }
       add0 =0.0;
       for (ismp=0; ismp<nfr; ismp++){
	  tmp[ismp] = 1./pow(tmp[ismp]+0.001, GAMMA_O);
          add0 += tmp[ismp]; 
       }

       for (ismp=0; ismp<nfr; ismp++){
	  perceptual_weight[ismp+top] = 1./lpc_spectrum[ismp+top];
       }
       ntt_prcptw(perceptual_weight+top, bark_env+top,
                       ntt_GAMMA_W, ntt_GAMMA_W_MIC, block_size_samples);
       ratio= pow((gain[i_sup]+0.0001), ntt_GAMMA_CH) /(gain[i_sup]+0.0001);
       add =0.0;
       for (ismp=0; ismp<nfr; ismp++){
		perceptual_weight[ismp+top] *=
		    lpc_spectrum[ismp+top]/bark_env[ismp+top]*ratio;
          add += perceptual_weight[ismp+top]; 
       }
	    for (ismp=0; ismp<nfr; ismp++){
		perceptual_weight[ismp+top] =
		perceptual_weight[ismp+top]* ratiof/add+
		tmp[ismp]*ratiol*5.0/add0;
	    }
}
	}
	break;
    case EIGHT_SHORT_SEQUENCE:
     {
      double add[8], tmp[2048], ave;
      for (i_sup=0; i_sup<numChannel; i_sup++){
       top = i_sup * block_size_samples;
       for (isf=0; isf<ntt_N_SHRT; isf++){
         ave =0.01;
	 subtop = top + isf * block_size_samples/8;
         tmp[0] = fabs(spectrum[0+subtop]);
         for (ismp=1; ismp<block_size_samples/8*bandUpper; ismp++){
           tmp[ismp] = fabs(spectrum[ismp+subtop])* 0.8
             + fabs(spectrum[ismp+subtop-1])*0.2;
           ave +=tmp[ismp];
	 }
         add[isf] =0.0;
	 ftmp = (float)bandUpper;
	 ftmp *= block_size_samples/8;
	 for (ismp=(int)ftmp; ismp<block_size_samples/8; ismp++)
	     perceptual_weight[ismp+subtop]=0.0;
	 for (ismp=0; ismp<(int)ftmp; ismp++){
           perceptual_weight[ismp+subtop] =
             1./pow((tmp[ismp]+0.001)/ave, GAMMA_O)/pow(ave, 0.8);
         add[isf] += perceptual_weight[ismp+subtop];
           tmp[ismp+subtop]=perceptual_weight[ismp+subtop];
	 }
       }
      }
     } 
	break;
    }
    
}


void ntt_prcptw(double pwt[], /* Input/Output -- Perceptual weighting factor */
	    double pred[],    /* Input        -- Interframe prediction factor */
	    double gamma_w, 
	    double gamma_w_mic,
	    int block_size_samples)   

{
    /*--- Variables ---*/
    int	ismp;


    /*--- Main operation ---*/
    for ( ismp=0; ismp<block_size_samples; ismp++ ){
      pwt[ismp] =  pow(pred[ismp], gamma_w) * pow(pwt[ismp], gamma_w_mic);
    }
}
void ntt_prcptw_s(double wt[],	/* Input  -- LPC weighting factor */
	      double gain[],	/* Input  -- Gain parameter */
	      double pwt[],	/* Output -- Perceptual weighting factor */
              double gamma_w_s_t,
	      double gamma_w_s,
	      int  numChannel,
	      int block_size_samples)

{
    /*--- Variables ---*/
    int		i_sup, ismp, i_shrt, iptr, top;
    double	gtmp;
    double      acc, ave[MAX_TIME_CHANNELS], gain_buf[8];


    /*--- Main operation ---*/
    for(i_sup=0; i_sup<numChannel; i_sup++){
	for (acc=0.00001, i_shrt=0; i_shrt<ntt_N_SHRT; i_shrt++){
	    iptr = i_sup * ntt_N_SHRT + i_shrt;
            acc += gain[iptr]; 
	}
	ave[i_sup] = acc/ntt_N_SHRT;
	for (i_shrt=0; i_shrt<ntt_N_SHRT; i_shrt++){
	    iptr = i_sup * ntt_N_SHRT + i_shrt;
            gain_buf[i_shrt] = gain[iptr] /( ave[i_sup]);
        }
	for (i_shrt=0; i_shrt<ntt_N_SHRT; i_shrt++){
	    iptr = i_sup * ntt_N_SHRT + i_shrt;

	    gtmp =  pow(gain_buf[i_shrt],gamma_w_s_t) *
		    pow(ave[i_sup], ntt_GAMMA_CH) / (gain[iptr]+1.e-5);

	    top = iptr * block_size_samples/8;
	    for ( ismp=0; ismp<block_size_samples/8; ismp++ ){
               /* Temporal perceptual weighting is added */
		pwt[ismp+top] = gtmp*pow(wt[ismp+top], gamma_w_s);
	    }
	}
    }
}
