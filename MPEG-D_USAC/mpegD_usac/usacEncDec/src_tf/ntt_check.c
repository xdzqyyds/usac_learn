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

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"


void ntt_dist_lsp(int n, double x[], double y[], double w[], double *dist)
{
  register double acc, acc1;
  acc =0.0;
  acc1=0.0;
  do{
    acc =  *(x++) - *(y++);
    acc1+= *(++w) * acc * acc;
  }while(--n);
  *dist = acc1;

}

void ntt_check_lsp(int n, double buf[], double min_gap)
{
  int j;
  double diff;
  for(j=0; j<n; j++){
    if(buf[j-1] > buf[j] -min_gap) {
      diff = buf[j-1] - buf[j];
      buf[j-1] -= (diff+min_gap)/2.;
      buf[j]   += (diff+min_gap)/2.;
    }
  }
}

void ntt_check_lsp_sort(int n, double buf[])
{
  int j,ite,flag;
  double  tmp;
  for(ite=0; ite<n; ite++){
    flag=0;
    for(j=0; j<n; j++){
      if(buf[j-1] > buf[j]) {
	flag=1;
	tmp =buf[j-1];
	buf[j-1] = buf[j];
	buf[j] = tmp;
      }
    }
    if(flag==0) return;
  }
}

