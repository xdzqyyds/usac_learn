
/*

This software module was originally developed by

    Masayuki Nishiguchi and Kazuyuki Iijima (Sony Corporation)

    and edited by

    Akira Inoue (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#include <stdio.h>
#include <math.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

static float       w_celp_up[SAMPLE-OVERLAP+LD_LEN]; 
static float       w_celp_down[SAMPLE-OVERLAP+LD_LEN]; 

#define	 TR_UP 	 	30.
#define	 TR_DOWN 	30.

void IPC_make_w_celp(void)	
{
    int	i;
    int	st1,st2;
    
    st1=(FRM)/2;
    st2=st1 + (int) TR_UP;
    
    for(i=0;i<st1;i++)
        w_celp_up[i]=0.;
    for(i=st1;i<st2;i++)
	w_celp_up[i]=(float)(i-st1+1)/(float)TR_UP;
    for(i=st2;i<(FRM);i++)
	w_celp_up[i]=1.;

    /* for short delay mode decode */
    for(i=FRM;i<FRM+LD_LEN;i++)
        w_celp_up[i]=1.;
    
    st1 = (int) ((FRM)/2 - TR_DOWN);
    st2 = (FRM)/2;
    
    for(i=0;i<st1;i++)
	w_celp_down[i]=1.;
    for(i=st1;i<st2;i++)
	w_celp_down[i]=1. - (float)(i-st1)/(float)TR_DOWN;
    for(i=st2;i<(FRM);i++)
	w_celp_down[i]=0.;

    /* for short delay mode decode */
    for(i=FRM;i<FRM+LD_LEN;i++)
	w_celp_down[i]=0.;
}


void IPC_uvExt(
int *vuv, 
float *suv, 
float *qRes,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i;
    int v_uv = 0;

    float uv_celp[FRM+LD_LEN];

    /* static float old_qRes[FRM/2]; */
    
    if(vuv[0] ==0 && vuv[1] ==0)
	v_uv=0;
    if(vuv[0] ==0 && vuv[1] !=0)
	v_uv=1;
    if(vuv[0] !=0 && vuv[1] ==0)
	v_uv=2;
    if(vuv[0] !=0 && vuv[1] !=0)
	v_uv=3;

    for(i=0;i<FRM/2;i++)
	uv_celp[i] = HDS->old_qRes[i];
    for(i=FRM/2;i<FRM;i++)
	uv_celp[i] = qRes[i-FRM/2];
    for(i=0;i<FRM/2;i++)  
	HDS->old_qRes[i] = qRes[i+FRM/2];

    if(HDS->decDelayMode == DM_SHORT)
    {
	for(i=FRM;i<FRM+LD_LEN;i++)
	    uv_celp[i]= qRes[i-FRM/2];
	
	if(v_uv == 0)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i+LD_LEN];
	}
	else if(v_uv == 1)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i+LD_LEN] * w_celp_down[i+LD_LEN];
	}
	else if(v_uv == 2)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i+LD_LEN] * w_celp_up[i+LD_LEN];
	}
	else
	{
	    for(i=0;i<FRM;i++)
		suv[i]=0. ;
	}
    }
    else
    {
	if(v_uv == 0)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i]; 
	}
	else if(v_uv == 1)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i] * w_celp_down[i];
	}
	else if(v_uv == 2)
	{
	    for(i=0;i<FRM;i++)
		suv[i]=uv_celp[i] * w_celp_up[i];
	}
	else
	{
	    for(i=0;i<FRM;i++)
		suv[i]=0. ; 
	}
    }
}



