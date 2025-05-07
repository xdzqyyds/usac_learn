 
/*

This software module was originally developed by

    Kazuyuki Iijima, Masayuki Nishiguchi, and Shiro Omori (Sony Corporation)

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

#include <math.h>
#include <stdio.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

static void iir2(
float *bufsf,
int size,
float *state,
double *coef
)
{
  int i;
  float x;
    
  for (i = 0; i < size; i++) {
    x = bufsf[i] + state[0]*coef[0] + state[1]*coef[1] 
      - (state[2]*coef[2] + state[3]*coef[3]);
    state[1] = state[0];
    state[0] = bufsf[i];
    state[3] = state[2];
    state[2] = x;
    bufsf[i]=x*coef[4];
  }
}

static void iir4(
float	*data,
int	size,
double	*state,
double  *coef
)
{
  int i;
  float x;

  for (i = 0; i < size; i++) {
    x = data[i] + state[0]*coef[0] + state[1]*coef[1]
            - (state[2]*coef[2] + state[3]*coef[3]);
    state[1] = state[0];
    state[0] = data[i];
    state[3] = state[2];
    state[2] = x;
    data[i] = x;
    
    x += state[4]*coef[4] + state[5]*coef[5]
      - (state[6]*coef[6] + state[7]*coef[7]);
    state[5] = state[4];
    state[4] = data[i];
    state[7] = state[6];
    state[6] = x;
    data[i] = x*coef[8];
  }
}

static double hpf_dec_coef[5] = {
  0.551543,
  0.152100,
  0.89,
  0.198025,
  1.226		/* gain */
};

static double hpf4ji_coef[9] = {
  -1.998066423746901,
   1.000000000000000,
  -1.962822436245804,
   0.9684991816600951,

  -1.999633313803449,
   0.9999999999999999,
  -1.858097918647416,
   0.8654599838007603,

   1.0		/* gain */
};

void IPC_bpf(float *bufsf,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    iir2(bufsf, FRM, HDS->hpf_dec_state, hpf_dec_coef);	/* hpf_dec() */
    iir4(bufsf, FRM, HDS->hpf4ji_state, hpf4ji_coef);	/* hpf4ji() */
    iir2(bufsf, FRM, HDS->lpf3400_state, HDS->lpf3400_coef);	/* lpf3400() */
}


/* 1998.5.20 */
/* Sel_14  Sel_15 */
/* fc =320 Hz
   100Hz -21   dB
   200Hz -9.0  dB
   300Hz -3.5  dB
   320Hz -3.0  dB
   400Hz -1.2  dB
*/

static double hpf_posfil_uv_coef[5] = {
  -1.999245,
   0.9998,
  -1.656136,
   0.710649,
   
   0.83695	/* gain */
};


void IPC_posfil_v(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i,j,ii;
    float out,outold;
    /* static float mem1[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem1old[P+1];
    
    float out2,out2old;
    /* static float mem2[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem2old[P+1];
    
    float alphaipFIR[IP][P+1];
    float alphaipIIR[IP][P+1];
    
    float resi[160];
    float pos[160];
    float nokori[160];
    float synhp[20];
    /* static float hpm=0.; */
    float gain;
    /* static float oldgain; */
    /* static float alphaipFIRold[P+1]; */
    /* static float alphaipIIRold[P+1]; */
    float window[160];
    float sp,op;
    float spn,opn;
    float fac05[11],fac08[11];

    float emph;

    fac05[0]=1.0f;
    fac05[1]=0.5f;
    fac05[2]=0.25f;
    fac05[3]=0.125f;
    fac05[4]=0.0625f;
    fac05[5]=0.03125f;
    fac05[6]=0.015625f;
    fac05[7]=0.0078125f;
    fac05[8]=0.00390625f;
    fac05[9]=0.001953125f;
   fac05[10]=0.0009765625f;

    fac08[0]=1.0f;
    fac08[1]=0.8f;
    fac08[2]=0.64f;
    fac08[3]=0.512f;
    fac08[4]=0.4096f;
    fac08[5]=0.32768f;
    fac08[6]=0.262144f;
    fac08[7]=0.2097152f;
    fac08[8]=0.16777216f;
    fac08[9]=0.134217728f;
   fac08[10]=0.107374182f;


/** 98.5.13 nishi **/

	if(HDS->decMode == DEC2K)
	{
		fac05[0]=1.0;
		for(i=1;i<=10;i++)
			fac05[i]=fac05[i-1] * 0.4;

		fac08[0]=1.0;
		for(i=1;i<=10;i++)
			fac08[i]=fac08[i-1] * 0.9;
	}


	for(i=0;i<40;i++)
		window[i]=(float)(i)/40.;
	for(i=40;i<160;i++)
		window[i]=1.;

/*
    	for(i=0;i<20;i++)
		window[i]=(float)(i)/20.;
    	for(i=20;i<160;i++)
		window[i]=1.;
*/
/*******************/

    for(j=0;j<IP;j++){
	for(i=0;i<=P;i++){
	    alphaipFIR[j][i]=alphaip[j][i]*fac05[i];
	    alphaipIIR[j][i]=alphaip[j][i]*fac08[i];
	}
    }


    for(ii=0;ii<IP;ii++)
    {

	if(HDS->decMode == DEC2K)
        {
		emph = -0.1 + 0.3 * alphaip[ii][1];  /* 98.5.13 nishi */
	}
	else
	{
		emph = 0.15 * alphaip[ii][1];
	}

	if(-0.5 > emph)
	{
	    emph = -0.5;
	}
	else if(emph > 0)
	{
	    emph = 0.0;
	}
	
	for(i=0;i<20;i++)
	{
	    synhp[i] = syn[ii*20+i] + emph * HDS->pfv_hpm;
	    HDS->pfv_hpm = syn[ii*20+i];
	}

	for(i=0;i<=P;i++)
	    mem1old[i]=HDS->pfv_mem1[i];
	
	for(i=0;i<20;i++){  
	    HDS->pfv_mem1[0]=synhp[i];
	    mem1old[0]=synhp[i];
	    out = 0.;
	    outold = 0.;
	    for(j=P;j>0;j--){
		out += HDS->pfv_mem1[j]*alphaipFIR[ii][j];
		HDS->pfv_mem1[j]=HDS->pfv_mem1[j-1];
		outold += mem1old[j]*HDS->pfv_alphaipFIRold[j];
		mem1old[j]=mem1old[j-1];
	    }
	    out += HDS->pfv_mem1[0];
	    outold += mem1old[0];
	    resi[ii*20+i]= out;
	    nokori[ii*20+i] = outold;
	}
	
	for(i=0;i<=P;i++)
	    mem2old[i]=HDS->pfv_mem2[i];
	
	for(i=0;i<20;i++){  
	    out2 = 0.;
	    out2old = 0.;
	    for(j=P;j>0;j--){
		out2 += HDS->pfv_mem2[j]*alphaipIIR[ii][j];
		out2old += mem2old[j]*HDS->pfv_alphaipIIRold[j];
		HDS->pfv_mem2[j]=HDS->pfv_mem2[j-1];
		mem2old[j]=mem2old[j-1];
	    }
	    out2 = resi[ii*20+i] - out2;
	    out2old = nokori[ii*20+i] - out2old;
	    HDS->pfv_mem2[1]= out2;
	    mem2old[1]= out2old;
	    pos[ii*20+i]= out2;
	    nokori[ii*20+i]= out2old;
	}
	
	for(i=0;i<=P;i++){
	    HDS->pfv_alphaipFIRold[i]=alphaipFIR[ii][i];
	    HDS->pfv_alphaipIIRold[i]=alphaipIIR[ii][i];
	}
	
    }
    
    sp = op = spn = opn = 0.;
    for(i=0;i<160;i++){
	sp += syn[i]*syn[i] ;
	op += pos[i]*pos[i] ;
	
    }
    
    if(op >= 2560)
	gain = sqrt(sp/op);
    else
	gain =0.;
    
    for(i=0;i<160;i++)
	syn[i] = gain * pos[i] * window[i] + HDS->pfv_oldgain * nokori[i] * (1.0-window[i]) ;  
    
    HDS->pfv_oldgain=gain;
}


#define PU_IP  2

void IPC_posfil_u_LD(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i,j,ii;
    float out,outold;
    /* static float mem1[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem1old[P+1];
    
    float out2,out2old;
    /* static float mem2[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem2old[P+1];
    
    float alphaipFIR[PU_IP][P+1];
    float alphaipIIR[PU_IP][P+1];
    
    float resi[FRM];
    float pos[FRM];
    float nokori[FRM];
    float synhp[FRM];
    /* static float hpm=0.; */
    float gain;
    /* static float oldgain; */
    /* static float alphaipFIRold[P+1]; */
    /* static float alphaipIIRold[P+1]; */
    float window[FRM];
    float sp,op;
    float spn,opn;
    float fac05[11],fac08[11];
    int  subf_len[2], slen;

    fac05[0]=1.0f;
    fac05[1]=0.5f;
    fac05[2]=0.25f;
    fac05[3]=0.125f;
    fac05[4]=0.0625f;
    fac05[5]=0.03125f;
    fac05[6]=0.015625f;
    fac05[7]=0.0078125f;
    fac05[8]=0.00390625f;
    fac05[9]=0.001953125f;
   fac05[10]=0.0009765625f;

    fac08[0]=1.0f;
    fac08[1]=0.8f;
    fac08[2]=0.64f;
    fac08[3]=0.512f;
    fac08[4]=0.4096f;
    fac08[5]=0.32768f;
    fac08[6]=0.262144f;
    fac08[7]=0.2097152f;
    fac08[8]=0.16777216f;
    fac08[9]=0.134217728f;
   fac08[10]=0.107374182f;


    subf_len[0]=FRM/2 - LD_LEN;
    subf_len[1]=FRM/2 + LD_LEN;

    for(i = 0; i < 20; i++)
	window[i] = (float) i / 20.;
    for(i = 20 ; i < FRM; i++)
	window[i]=1.;

    for(j=0;j<PU_IP;j++)
    {
	for(i=0;i<=P;i++)
	{
	    alphaipFIR[j][i]=alphaip[j][i]*fac05[i];
	    alphaipIIR[j][i]=alphaip[j][i]*fac08[i];
	}
    }
    
    slen=0;
    for(ii=0;ii<PU_IP;ii++){

	for(i=0;i<subf_len[ii];i++){
	    synhp[i]=syn[slen+i]- 0.1*HDS->pfu_hpm;
	    HDS->pfu_hpm=syn[slen+i];
	}
	for(i=0;i<=P;i++)
	    mem1old[i]=HDS->pfu_mem1[i];
	
	for(i=0;i<subf_len[ii];i++){  
	    HDS->pfu_mem1[0]=synhp[i];
	    mem1old[0]=synhp[i];
	    out = 0.;
	    outold = 0.;
	    for(j=P;j>0;j--){
		out += HDS->pfu_mem1[j]*alphaipFIR[ii][j];
		HDS->pfu_mem1[j]=HDS->pfu_mem1[j-1];
		outold += mem1old[j]*HDS->pfu_alphaipFIRold[j];
		mem1old[j]=mem1old[j-1];
	    }
	    out += HDS->pfu_mem1[0];
	    outold += mem1old[0];
	    resi[slen+i]= out;
	    nokori[slen+i] = outold;
	    
	}
	
	for(i=0;i<=P;i++)
	    mem2old[i]=HDS->pfu_mem2[i];
	
	for(i=0;i<subf_len[ii];i++){  
	    out2 = 0.;
	    out2old = 0.;
	    for(j=P;j>0;j--){
		out2 += HDS->pfu_mem2[j]*alphaipIIR[ii][j];
		out2old += mem2old[j]*HDS->pfu_alphaipIIRold[j];
		HDS->pfu_mem2[j]=HDS->pfu_mem2[j-1];
		mem2old[j]=mem2old[j-1];
	    }
	    out2 = resi[slen+i] - out2;
	    out2old = nokori[slen+i] - out2old;
	    HDS->pfu_mem2[1]= out2;
	    mem2old[1]= out2old;
	    pos[slen+i]= out2;
	    nokori[slen+i]= out2old;
	}
	
	for(i=0;i<=P;i++){
	    HDS->pfu_alphaipFIRold[i]=alphaipFIR[ii][i];
	    HDS->pfu_alphaipIIRold[i]=alphaipIIR[ii][i];
	}
	
    slen = slen + subf_len[ii];
    }
    
    sp = op = spn = opn = 0.;
    for(i=0;i<FRM;i++){
	sp += syn[i]*syn[i] ;
	op += pos[i]*pos[i] ;
	
    }
    
    if(op >= 2560)
	gain = sqrt(sp/op);
    else
	gain =0.;
    
    for(i=0;i<FRM;i++)
	syn[i] = gain * pos[i] * window[i] + HDS->pfu_oldgain * nokori[i] * (1.0-window[i]) ;  
    
    HDS->pfu_oldgain=gain;

 /* 98.5.13 nishi */
    iir2(syn, FRM, HDS->hpf_posfil_uv_state, hpf_posfil_uv_coef); /* hpf_posfil_uv() */

}


void IPC_posfil_u(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i,j,ii;
    float out,outold;
    /* static float mem1[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem1old[P+1];
    
    float out2,out2old;
    /* static float mem2[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.}; */
    float mem2old[P+1];
    
    float alphaipFIR[PU_IP][P+1];
    float alphaipIIR[PU_IP][P+1];
    
    float resi[FRM];
    float pos[FRM];
    float nokori[FRM];
    float synhp[FRM / PU_IP];
    /* static float hpm=0.; */
    float gain;
    /* static float oldgain; */
    /* static float alphaipFIRold[P+1]; */
    /* static float alphaipIIRold[P+1]; */
    float window[FRM];
    float sp,op;
    float spn,opn;
    float fac05[11],fac08[11];
    
    fac05[0]=1.0f;
    fac05[1]=0.5f;
    fac05[2]=0.25f;
    fac05[3]=0.125f;
    fac05[4]=0.0625f;
    fac05[5]=0.03125f;
    fac05[6]=0.015625f;
    fac05[7]=0.0078125f;
    fac05[8]=0.00390625f;
    fac05[9]=0.001953125f;
   fac05[10]=0.0009765625f;

    fac08[0]=1.0f;
    fac08[1]=0.8f;
    fac08[2]=0.64f;
    fac08[3]=0.512f;
    fac08[4]=0.4096f;
    fac08[5]=0.32768f;
    fac08[6]=0.262144f;
    fac08[7]=0.2097152f;
    fac08[8]=0.16777216f;
    fac08[9]=0.134217728f;
   fac08[10]=0.107374182f;

    for(i = 0; i < 20; i++)
	window[i] = (float) i / 20.;
    for(i = 20 ; i < FRM; i++)
	window[i]=1.;

    for(j=0;j<PU_IP;j++)
    {
	for(i=0;i<=P;i++)
	{
	    alphaipFIR[j][i]=alphaip[j][i]*fac05[i];
	    alphaipIIR[j][i]=alphaip[j][i]*fac08[i];
	}
    }
    
    for(ii=0;ii<PU_IP;ii++){
	for(i=0;i<FRM/PU_IP;i++){
	    synhp[i]=syn[ii*FRM/PU_IP+i]- 0.1*HDS->pfu_hpm;
	    HDS->pfu_hpm=syn[ii*FRM/PU_IP+i];
	}
	for(i=0;i<=P;i++)
	    mem1old[i]=HDS->pfu_mem1[i];
	
	for(i=0;i<FRM/PU_IP;i++){  
	    HDS->pfu_mem1[0]=synhp[i];
	    mem1old[0]=synhp[i];
	    out = 0.;
	    outold = 0.;
	    for(j=P;j>0;j--){
		out += HDS->pfu_mem1[j]*alphaipFIR[ii][j];
		HDS->pfu_mem1[j]=HDS->pfu_mem1[j-1];
		outold += mem1old[j]*HDS->pfu_alphaipFIRold[j];
		mem1old[j]=mem1old[j-1];
	    }
	    out += HDS->pfu_mem1[0];
	    outold += mem1old[0];
	    resi[ii*FRM/PU_IP+i]= out;
	    nokori[ii*FRM/PU_IP+i] = outold;
	    
	}
	
	for(i=0;i<=P;i++)
	    mem2old[i]=HDS->pfu_mem2[i];
	
	for(i=0;i<FRM/PU_IP;i++){  
	    out2 = 0.;
	    out2old = 0.;
	    for(j=P;j>0;j--){
		out2 += HDS->pfu_mem2[j]*alphaipIIR[ii][j];
		out2old += mem2old[j]*HDS->pfu_alphaipIIRold[j];
		HDS->pfu_mem2[j]=HDS->pfu_mem2[j-1];
		mem2old[j]=mem2old[j-1];
	    }
	    out2 = resi[ii*FRM/PU_IP+i] - out2;
	    out2old = nokori[ii*FRM/PU_IP+i] - out2old;
	    HDS->pfu_mem2[1]= out2;
	    mem2old[1]= out2old;
	    pos[ii*FRM/PU_IP+i]= out2;
	    nokori[ii*FRM/PU_IP+i]= out2old;
	}
	
	for(i=0;i<=P;i++){
	    HDS->pfu_alphaipFIRold[i]=alphaipFIR[ii][i];
	    HDS->pfu_alphaipIIRold[i]=alphaipIIR[ii][i];
	}
	
    }
    
    sp = op = spn = opn = 0.;
    for(i=0;i<FRM;i++){
	sp += syn[i]*syn[i] ;
	op += pos[i]*pos[i] ;
    }
    
    if(op >= 2560)
	gain = sqrt(sp/op);
    else
	gain =0.;
    
    for(i=0;i<FRM;i++)
	syn[i] = gain * pos[i] * window[i] + HDS->pfu_oldgain * nokori[i] * (1.0-window[i]) ;  
    
    HDS->pfu_oldgain=gain;

 /* 98.5.13 nishi */
    iir2(syn, FRM, HDS->hpf_posfil_uv_state, hpf_posfil_uv_coef); /* hpf_posfil_uv() */

}

