
/*


This software module was originally developed by

    Masayuki Nishiguchi, Kazuyuki Iijima (Sony Corporation)

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
#include <stdlib.h>
#include <string.h>	/* changed from strings.h   HP 980211 */
/* #include <sys/file.h> */
#include <math.h>
/* #include <values.h> */

#include "hvxc.h"
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#include "hvxcQAmDec.h"
#include "hvxcCbAm.h"
#include "hvxcCbAm4k.h"


#define MAX_HARM        76

#define	ENC		1
#define	DEC		0

#define DIM_TD		27

extern int ipc_encMode;

#ifdef __cplusplus
extern "C" {
#endif

int read(int d, char *buf, int nbytes);
int write(int d, char *buf, int nbytes);
int close(int d);    

#ifdef __cplusplus
}
#endif

static float f_coef[(JISU-1)*OVER_R+1];
vqscheme_lpc ipc_cvq_lpc;
static vqschm4k schm4k;

void IPC_set_const_lpcVM(void)
{
  ipc_cvq_lpc.numpulse = 44;

  ipc_cvq_lpc.vqdim_lpc[0] = 44;
  ipc_cvq_lpc.vqdim_lpc[1] = 44;

  ipc_cvq_lpc.vqsize_lpc[0] = 16;
  ipc_cvq_lpc.vqsize_lpc[1] = 16;
  ipc_cvq_lpc.gsize_lpc[0] = 32;

  schm4k.num_vq = 4;
  schm4k.dim_tot = 14;

  schm4k.vqdim[0]=2;
  schm4k.vqdim[1]=4;
  schm4k.vqdim[2]=4;
  schm4k.vqdim[3]=4;

  schm4k.vqsize[0]=128;
  schm4k.vqsize[1]=1024;
  schm4k.vqsize[2]=512;
  schm4k.vqsize[3]=64;
}


static void over_s_r(float *re, float *rel, int send1, float w0f, float w0) 
{
	int i,j,p,ii;

	for(i=0;i<=(send1+1)*OVER_R;i++)
		rel[i]= 0.;  

	ii=0;
	for(i=0; i<=send1;i++){
		for(p=0;p<OVER_R;p++){
			if((i*OVER_R+p+1)*w0f > w0*ii){ 
				rel[i*OVER_R+p]=rel[i*OVER_R+p+1]=0.; 
						
				for(j=1;j<JISU;j++){	
					if(i+j-(JISU-1)/2 >= 0){
						rel[i*OVER_R+p] += f_coef[j * OVER_R - p] * re[i+j-(JISU-1)/2];
						rel[i*OVER_R+p+1] += f_coef[j * OVER_R - (p+1)] * re[i+j-(JISU-1)/2];
						}	
					}
				ii++;
				}   
			}
		}

}


static void IpAmFIR_Deci_r(float *am1, int send1, float w01, float *am1ip, float *am2deci, float w0, int send)
{
	int i,j,l,lb;
        int ub = 0;
        int bw,idx,ii;
	float w0f;
	float re[SAMPLE];
	float rel[SAMPLE*R];
	float ap[SAMPLE*R/2];
	static int cunt=0;
	
	for(i=0;i<=send1;i++)
		re[i]=am1[i];
	for(i=send1+1; i<=send1+(JISU-1)/2; i++)
		re[i]=re[send1];
	w0f=w01/(float)R;
	over_s_r(re,rel,send1,w0f,w0);  
	for(i=0;i<= (send1+1) * OVER_R ;i++)  
		ap[i] = rel[i] ;		
	ii=0;
	for(i=0;i<=(send1+1)*R-1;i++){  
		if(i==0) 
			lb=0;
		else 	 
			lb=ub;
	
			ub=IPC_inint((float)(i+1) * w0f);
		if(ub >= SAMPLE*R/2 )
			ub=SAMPLE*R/2;	 
		bw=ub-lb;
		if(bw <=0) {printf("????"); am2deci[ii]=ap[i]; ii++; }
		else{
		idx=0;
		for(j=lb;j<ub;j++){
			if(j == IPC_inint((float)ii * w0)){  
				am1ip[j]=ap[i]*(1.-(float)idx/(float)bw)+ap[i+1]*((float)idx/(float)bw);
				am2deci[ii]=am1ip[j];
				ii++;
				}
			idx++;
			if(ii > send ) break;
			}
			}
		if(ii > send ) break;
		}
}


static void enc_2ch_sgvq(
float *in,
float *wt,
int vqsize0,
int vqdim0,    
int vqsize1,
int vqdim1,
int gsize,
float (*cb0)[44],
float (*cb1)[44],
float *g0,
float *qedvec,
IdAm *idAm,
int ende)
{
	int i,j,k,l,idx_s0,idx_s1,idx_g;
	float ip,maxip,ipb;
        float optipb = 0;
        float optg;
	float sqa,minsqa;
	float inwtsq[45];
	float wtsq[45]; 	
	float cbplus;
	float norm;
        float optnorm = 0;
	float len;

if(ende == ENC){

	for(i=0;i<vqdim0;i++)
		inwtsq[i]=in[i]*wt[i]*wt[i];

	for(i=0;i<vqdim0;i++)
		wtsq[i]=wt[i]*wt[i];	
	
	maxip= -1000.;
	idx_s0 = 0;
	idx_s1 = 0;
	for(k=0;k<vqsize1;k++){
		for(j=0;j<vqsize0;j++){
			ip=0.;
			ipb=0.;
			norm=0.;

			for(l=0;l<vqdim0;l++){
				cbplus=(cb0[j][l]+cb1[k][l]);
				ipb += cbplus * inwtsq[l];
				norm += cbplus * wtsq[l] * cbplus;
				}

			ip=ipb/(sqrt(norm)); 
			if(ip > maxip ){
				maxip = ip;
				optnorm = norm;
				optipb = ipb;
				idx_s0 = j;
				idx_s1 = k;
				}
			}
		}

	optg=optipb/optnorm;
	minsqa = 100000000.;
	idx_g = 0;
	for(j=0;j<gsize;j++){
		sqa=(g0[j]-optg)*(g0[j]-optg);
		if(sqa < minsqa){
			minsqa = sqa;
			idx_g = j;
			}
		}

	for(l=0;l<vqdim0;l++)
		qedvec[l]=g0[idx_g]*(cb0[idx_s0][l]+cb1[idx_s1][l]);  

	idAm->idS0 = idx_s0;
	idAm->idS1 = idx_s1;
	idAm->idG = idx_g;

	}

if(ende == DEC){

	idx_s0 = idAm->idS0;
	idx_s1 = idAm->idS1;
	idx_g = idAm->idG;

	for(l=0;l<vqdim0;l++)
		qedvec[l]=g0[idx_g]*(cb0[idx_s0][l]+cb1[idx_s1][l]);
	}

}

static void quand_lpc(
float *da, 
int sendmax, 
vqscheme_lpc *cvq_lp, 
cbook_lpc *cba_lp, 
IdAm *idAm, 
int ende, 
float *w, 
int voiced
)     
{
  int i, j, k;
  float qedvec[MAXDIM3];
  float in[MAXDIM3];
  float wt[44];

  da[0] = 0.0; 

  for (j = 1; j <= sendmax; j++)
    in[j-1] = da[j];

  for (j = 1; j <= sendmax; j++)
    wt[j-1] = w[j];


  if (voiced) {
    enc_2ch_sgvq(
		 in, 
		 wt, 
		 cvq_lp->vqsize_lpc[0], 
		 cvq_lp->vqdim_lpc[0],
		 cvq_lp->vqsize_lpc[1],
		 cvq_lp->vqdim_lpc[1],
		 cvq_lp->gsize_lpc[0],
		 cba_lp->cb1lpc,
		 cba_lp->cb2lpc,
		 cba_lp->g0lpc,
		 qedvec,
		 idAm,
		 ende);
  }
  else {
    for (i = 0; j < sendmax; j++)
      qedvec[j] = 0.0;
  }

  for (j = 1; j <= sendmax; j++)
    da[j] = qedvec[j-1];
}


static float dis
(float cb[],
float in_vec[],
float weight[],
int vqdim)
{
	int i;
	float disout;

	disout=0.;
	for(i=0;i<vqdim;i++)
		disout += (cb[i]-in_vec[i])*(cb[i]-in_vec[i])*weight[i]*weight[i];

	return(disout);

}

static void StVq(
float		in_vec[],
float		weight[],
float		qed_vec[],
cbook4k		*cb4k,
int		nBnd,
vqschm4k 	*schm4k,
int		*id,
int		ende)
{
    int i,j,l,idx;
    float dist;
    float mindist = 0;
    
    if(ende == ENC)
    {
	idx = 0;
	switch(nBnd)
	{
	case 0:
	    for(j = 0; j < schm4k->vqsize[0]; j++)
	    {
		dist = dis(cb4k->cb0[j], in_vec, weight, schm4k->vqdim[0]);
		if(dist < mindist || j == 0)
		{
		    mindist = dist;
		    idx = j;
		}
	    }
	    break;
	case 1:
	    for(j = 0; j < schm4k->vqsize[1]; j++)
	    {
		dist = dis(cb4k->cb1[j], in_vec, weight, schm4k->vqdim[1]);
		if(dist < mindist || j == 0)
		{
		    mindist = dist;
		    idx = j;
		}
	    }
	    break;
	case 2:
	    for(j = 0; j < schm4k->vqsize[2]; j++)
	    {
		dist = dis(cb4k->cb2[j], in_vec, weight, schm4k->vqdim[2]);
		if(dist < mindist || j == 0)
		{
		    mindist = dist;
		    idx = j;
		}
	    }
	    break;
	case 3:
	    for(j = 0; j < schm4k->vqsize[3]; j++)
	    {
		dist = dis(cb4k->cb3[j], in_vec, weight, schm4k->vqdim[3]);
		if(dist < mindist || j == 0)
		{
		    mindist = dist;
		    idx = j;
		}
	    }
	    break;
	}
	*id = idx;
	
	switch(nBnd)
	{
	case 0:
	    for(l = 0; l < schm4k->vqdim[0]; l++)
		qed_vec[l] = cb4k->cb0[idx][l];
	    break;
	case 1:
	    for(l = 0; l < schm4k->vqdim[1]; l++)
		qed_vec[l] = cb4k->cb1[idx][l];
	    break;
	case 2:
	    for(l = 0; l < schm4k->vqdim[2]; l++)
		qed_vec[l] = cb4k->cb2[idx][l];
	    break;
	case 3:
	    for(l = 0; l < schm4k->vqdim[3]; l++)
		qed_vec[l] = cb4k->cb3[idx][l];
	    break;
	}
    }
    else
    {
	idx = *id;
	
	switch(nBnd)
	{
	case 0:
	    for(l = 0; l < schm4k->vqdim[0]; l++)
		qed_vec[l] = cb4k->cb0[idx][l];
	    break;
	case 1:
	    for(l = 0; l < schm4k->vqdim[1]; l++)
		qed_vec[l] = cb4k->cb1[idx][l];
	    break;
	case 2:
	    for(l = 0; l < schm4k->vqdim[2]; l++)
		qed_vec[l] = cb4k->cb2[idx][l];
	    break;
	case 3:
	    for(l = 0; l < schm4k->vqdim[3]; l++)
		qed_vec[l] = cb4k->cb3[idx][l];
	    break;
	}
    }
}

static void Quan4k(
float		target[],
float		w_org[],
int		send2,   		
vqschm4k	*schm4k,
cbook4k		*cb4k,
int		id4k[])
{
    float in_vec[MAXDIM1];
    float weight[MAXDIM1];
    float qed_vec[MAXDIM1];
    int ende,k,j,i;

    ende=ENC;


    if(schm4k->dim_tot <= send2)
    {
	k=1;
	for(j=0;j<schm4k->num_vq;j++)
	{
	    for(i=0;i<schm4k->vqdim[j];i++)
	    {
		in_vec[i] = target[k];
		weight[i] = w_org[k];
		k++;
	    }
	    StVq(in_vec, weight, qed_vec, cb4k, j, schm4k, &id4k[j], ende);
	}
    }
    else
    {
	k=1;
	for(j=0;j<schm4k->num_vq;j++){
	    for(i=0;i<schm4k->vqdim[j];i++){
		if(k <= send2){
		    in_vec[i]=target[k];
		    weight[i]=w_org[k];
		    k++;
		}
		else{
		    in_vec[i]=0.;
		    weight[i]=0.;
		    k++;
		}
	    }
	    StVq(in_vec, weight, qed_vec, cb4k, j, schm4k, &id4k[j], ende);
	}
    }

}


void harm_quant(
float	(*am)[3],
float	*pch,
float	*per_wt,
float	*dumLSF,
int	normMode,
int	vuv,
int	*idxS,
int	*bitnum,
int	*idxG,
int     *id4k)
{
	int i,j;
	float w02,w0max;
	int send2,sendmax;
	float am2[SAMPLE/2];
	float am2ip_f[SAMPLE*R/2];
	float localip_f[SAMPLE*R/2];
	float am2deci[(20+NUMPITCH/2)/2 + 1];
	float prevqed[(20+NUMPITCH/2)/2 + 1];
	float da[(20+NUMPITCH/2)/2 + 1];
	float fenein,feneq;
	static float feneqold;
	float w0m;
	int voiced;
	IdAm  idAmInt;
	float	per_wt44[45];
	float w02r;
	float w_org[SAMPLE/2];
	float qam2[MAX_HARM];
	float target[MAX_HARM];

	if(normMode != 0){
		printf("Hamonic quantization: Invalid Normalization Mode !!");
		exit(10);
	}
	sendmax = ipc_cvq_lpc.numpulse;  
	w0m = (0.95 * (float)SAMPLE/2. )/(float)sendmax ; 

	per_wt44[0]=0.;
	for(i=1;i<=sendmax;i++)
		per_wt44[i]=per_wt[(int)(floor((float)i * w0m) + 0.5)];

	*bitnum=10;
	*pch= (float)(IPC_inint(*pch * 2.))/2.;
	sendmax = ipc_cvq_lpc.numpulse; 
	w0max = (float)(SAMPLE*R/2 * 0.95)/(float)sendmax ;   
	w02 = (float)(SAMPLE*R)/ *pch;
	send2 = (int)(( *pch)/2.);   

	if(vuv != 0)
		voiced=1;
	else
		voiced=0;
        for(i=0; i<SAMPLE/2; i++)
                am2[i]= am[i][2];
        am2[0]=0.;  
	IpAmFIR_Deci_r(am2,send2,w02,am2ip_f,am2deci,w0max,sendmax);   
 	for(i=1;i<=sendmax;i++){
		da[i]= am2deci[i];
		}
	fenein=0.;
        for(i=1;i<=sendmax;i++)
                 fenein += am2deci[i]*am2deci[i];
        fenein = sqrt(fenein/(float)sendmax);
	if( fenein <= 0.00001 ){
		for(i=1;i<=sendmax;i++)
			per_wt44[i]=1.;
		}

	quand_lpc(da,sendmax,&ipc_cvq_lpc,&cba_lpc,&idAmInt,ENC,per_wt44,voiced);  

        w02r = (float)(SAMPLE)/ *pch ;
	w_org[0]=0.;
	for(i=1;i<=send2;i++)
	w_org[i]=per_wt[(int)(floor((float)i * w02r) + 0.5)];

	for(i=send2+1;i<SAMPLE/2;i++)
		w_org[i]=0.;
	send2= (int)( 0.95 *  (*pch)/2.);

	IpAmFIR_Deci_r(da,sendmax,w0max,localip_f,qam2,w02,send2);

	for(i=0;i<=send2;i++){
		target[i] = am2[i] - qam2[i];
	}

	for(i=send2+1;i<MAX_HARM;i++)
		target[i] = 0.;

	if(ipc_encMode == ENC4K)
	{
    
	    if(vuv){
		    Quan4k(target, w_org, send2, &schm4k, &cb4k, id4k);
	    }

	}
	for(i=1;i<=sendmax;i++)
			prevqed[i]=da[i]; 
	prevqed[0]= 0.;  
	for(i=1;i<=sendmax;i++){
		if(prevqed[i] <= 0){
	        	prevqed[i]=0.;
		}
	}
        feneq=0.;
        for(i=1;i<=sendmax;i++)
                feneq += prevqed[i]*prevqed[i];
        feneq = sqrt(feneq/(float)sendmax);
        if(feneq < 1.0){ 
		for(i=0;i<=sendmax;i++)
			prevqed[i]=0.;
		}
	else if(0.5*(feneqold + feneq) < 1.4){
		for(i=0;i<=sendmax;i++)
			prevqed[i]=0.;
		}
	feneqold=feneq;
	idxS[0]=idAmInt.idS0;
	idxS[1]=idAmInt.idS1;
	*idxG= idAmInt.idG;
}


void IPC_make_f_coef(void)
{
	int i;
	int numc;
	
	numc=(JISU-1)*OVER_R; 
	for(i=1; i<=numc/2 ;i++)
		f_coef[i+numc/2 ]= f_coef[numc/2 - i ] = sin((float)i * M_PI/8.)/((float)i*M_PI/8.);
	f_coef[numc/2]=1.;
	for(i=0; i<=numc ;i++)
		f_coef[i]=f_coef[i]* 0.5 * (1. - cos(2. * M_PI * i/numc));

}

