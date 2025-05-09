/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/

#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"


void ntt_lsptowt_int(int nfr,      /* parameter frame size         */
		     int block_size_samples,
		     int n_pr,      /* parameter prediction order */
		     double lsp[],  /* input LSP parameter  */
		     double wt[],   /* output spectral envelope */
                     double *ntt_cos_TT)
{
    double coslsp[ntt_N_PR_MAX];
    double delta;
    int i, jj, jjj;
    int  endpoint;
    int  istepl, isteph, mag;

    mag = block_size_samples / nfr;
    /* istep, isteph interpolarion step size */
    if(block_size_samples == 2048){
      if(mag == 1)
	         { istepl = 8; isteph = 16;} 
      else if((mag > 1 ) && (mag <= 4 ) )
	         { istepl = 2; isteph = 4;} 
      else         { istepl = 1; isteph = 1; }
    }
    else{
      if(mag == 1)
	         { istepl = 4; isteph = 8;} 
      else if((mag > 1 ) && (mag <= 4 ) )
	         { istepl = 2; isteph = 4;} 
      else       { istepl = 1; isteph = 1;}
    }
    for(i=0; i<n_pr; i++) coslsp[i] = 2.*cos(lsp[i+1]);
    
    /* envelope every istepl for lower bands    */
    for(i=0;i<nfr/2;i+=istepl) 
	  ntt_weight(n_pr, coslsp, ntt_cos_TT[(i*4+2)*mag], wt+i);
    
    /* envelope every isteph for higher bands */
    for(i=nfr/2;i<nfr;i+=isteph) 
	  ntt_weight(n_pr, coslsp, -ntt_cos_TT[block_size_samples*4-(i*4+2)*mag], wt+i);
    
    if (isteph == 1) return;

    endpoint=i-isteph;
    
    /* interpolation for lower bands          */
    for(i=istepl;i<nfr/2;i+=istepl) { 
      /* convex and increasing ? */
      if((wt[i]*1.95 >wt[istepl+i]+wt[i-istepl])&& 
	 (wt[i-istepl]>wt[i+istepl])){
             ntt_weight(n_pr, coslsp, ntt_cos_TT[((i-istepl/2)*4+2)*mag], wt+i-istepl/2);
             for(jjj=1, jj=i-istepl+1; jj<i-istepl/2; jj++) {
               double tmp;
               delta = (double)(jjj+1)/(double)istepl;
               tmp = (double)(jjj+1);
               delta = tmp/(double)istepl;
               wt[jj] = wt[i-istepl]*(1.-delta) + wt[i-istepl/2]*delta;
               wt[jj+istepl/2] = wt[i-istepl/2]*(1.-delta) + wt[i]*delta;
               jjj+=2;
             }
      }
      else{
	     for(jjj=0, jj=i-istepl+1; jj<i; jj++) {
               double tmp;
               delta = (double)(jjj+1)/(double)istepl;
               tmp = (double)(jjj+1);
               delta = tmp/(double)istepl;
	       wt[jj] = wt[i-istepl]*(1.-delta) + wt[i]*delta;
	       jjj++; 
	     }
      }
    } 
    
    /* interpolation for middle bands  */
             for(jjj=0, jj=nfr/2-istepl+1; jj<nfr/2; jj++) {
               double tmp;
               delta = (double)(jjj+1)/(double)istepl;
               tmp = (double)(jjj+1);
               delta = tmp/(double)istepl;
               wt[jj] = wt[nfr/2-istepl]*(1.-delta) + wt[nfr/2]*delta;
               jjj++; 
             }
    
    /* interpolation for higher bands  */
    for(i=nfr/2+isteph;i<nfr-isteph;i+=isteph) { 
      /* convex and increasing ? */
      if((wt[i]*1.95 >wt[isteph+i]+wt[i-isteph])&&
	 (wt[i-isteph]>wt[i+isteph])){
            ntt_weight(n_pr, coslsp, 
	    -ntt_cos_TT[block_size_samples*4-((i-istepl)*4+2)*mag], 
	    wt+i-istepl);
            for(jjj=1, jj=i-isteph+1; jj<i-istepl; jj++) {
               double tmp;
               delta = (double)(jjj+1)/(double)isteph;
               tmp = (double)(jjj+1);
               delta = tmp/(double)isteph;
	       wt[jj] = wt[i-isteph]*(1.-delta) + wt[i-istepl]*delta;
               wt[jj+istepl] = wt[i-istepl]*(1.-delta) + wt[i]*delta;
               jjj+=2;
            }
      }
      else{
	    for(jjj=0, jj=i-isteph+1; jj<i; jj++) {
               double tmp;
               tmp = (double)(jjj+1);
               delta = tmp/(double)isteph;
	       wt[jj] = wt[i-isteph]*(1.-delta) + wt[i]*delta;
	       jjj++;
	    }
      }
    } 
    
    /* tail process higher end */
           for(jjj=0,jj=endpoint-isteph+1; jj<endpoint; jj++) {
               double tmp;
               delta = (double)(jjj+1)/(double)isteph;
               tmp = (double)(jjj+1);
               delta = tmp/(double)isteph;
	       wt[jj] = wt[i-isteph]*(1.-delta) + wt[i]*delta;
               jjj++; 
           }
    for(i=endpoint+1;i<nfr;i++) wt[i] = wt[endpoint];
}

void ntt_weight(int n_pr, double coslsp[], double costblv, double *wtv)
{
     double a, b, cosz2;
     int jj;
	   cosz2=2. * costblv;
	   jj=0;
	   a = (cosz2 - coslsp[jj++]);
	   b = (cosz2 - coslsp[jj++]);
         for(;jj<n_pr;) { /* n_pr should be even */ 
	   a *= (cosz2 - coslsp[jj++]);
	   b *= (cosz2 - coslsp[jj++]);
	 }
	 *wtv = 1./(a*a*(1.+costblv)+b*b*(1.-costblv));
}
