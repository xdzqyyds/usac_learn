
/*

This software module was originally developed by

    Masayuki Nishiguchi and Kazuyuki Iijima (Sony Corporation)

    and edited by

    Yuji Maeda and Akira Inoue (Sony Corporation)

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
#include <math.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"
#include "hvxcQAmDec.h"

#include "hvxcCbAm.h"
#include "hvxcCbAm4k.h"

#define	ENC	1
#define DEC	0

static float f_coef[(JISU-1)*OVER_R+1];

#define MAX_HARM       76

static vqscheme_lpc ipc_cvq_lpc;
static vqschm4k schm4k;

void IPC_set_const_lpcVM_dec(void)
{
  ipc_cvq_lpc.numpulse = 44;

  ipc_cvq_lpc.vqdim_lpc[0] = 44;
  ipc_cvq_lpc.vqdim_lpc[1] = 44;

  ipc_cvq_lpc.vqsize_lpc[0] = 16;
  ipc_cvq_lpc.vqsize_lpc[1] = 16;
  ipc_cvq_lpc.gsize_lpc[0] = 32;

  schm4k.num_vq = 4;
  schm4k.dim_tot = 14;

  schm4k.vqdim[0] = 2;
  schm4k.vqdim[1] = 4;
  schm4k.vqdim[2] = 4;
  schm4k.vqdim[3] = 4;

  schm4k.vqsize[0] = 128;
  schm4k.vqsize[1] = 1024;
  schm4k.vqsize[2] = 512;
  schm4k.vqsize[3] = 64;
}

static void IpAmFIR_Deci_r(
float *am1,     /* source vector */
int send1,      /* dimension of source vector */
float w01,      /* fundamental frequency of source vector (PI=1024) */
float *am2deci, /* destiantion vector */
float w0,       /* fundamental frequency of dest.vector (PI=1024) */
int send        /* dimension of dest.vector */
)     
{
  int i, j, p, ii;
  float w0f;
  float ip_ratio;
  float re[SAMPLE/2+JISU];
  float rel0, rel1;
  float tmp;
  
  tmp = am1[send1];
  
  for (i = 0; i < (JISU-1)/2; i++)
    re[i] = 0.0f;
  for (i = 0; i <= send1; i++)
    re[i+(JISU-1)/2] = am1[i];
  for (i = 0; i < (JISU-1)/2; i++)
    re[i+send1+1+(JISU-1)/2] = tmp;	/* for MS VC++ 5.0(99/02/23) */
  w0f = w01/(float)R;
  ii = 0;
  for (i = 0; i <= send1 && ii <= send; i++) {
    for (p = 0; p < OVER_R && ii <= send; p++) {
      ip_ratio = (i*OVER_R+p+1)*w0f-w0*ii;

      if (ip_ratio > 0) {
	ip_ratio /= w0f;
	rel0 = rel1 = 0.0f;
	for (j = 1; j < JISU; j++) {
	  rel0 += f_coef[j*OVER_R-p] * re[i+j];
	  rel1 += f_coef[j*OVER_R-(p+1)]*re[i+j];
	}
	am2deci[ii] = rel0*ip_ratio + rel1*(1.0f-ip_ratio);
	ii++;
      }
    }
  }
}


static void dec_2st_sgvq(
int vqdim0, 
float (*cb0)[44], 
float (*cb1)[44], 
float *g0, 
float *qedvec, 
IdAm *idAm
)   
{
  int l, idx_s0, idx_s1, idx_g;

  idx_s0 = idAm->idS0;
  idx_s1 = idAm->idS1;
  idx_g = idAm->idG;

  for (l = 0; l < vqdim0; l++)
    qedvec[l] = g0[idx_g]*(cb0[idx_s0][l]+cb1[idx_s1][l]);

}

static void quand_lpc_dec(
float *da, 
int sendmax, 
vqscheme_lpc *cvq_lp, 
cbook_lpc *cba_lp, 
IdAm *idAm, 
int voiced
)
{
  float qedvec[MAXDIM3];
  int j;

  da[0] = 0.; 

  if (voiced) {
    dec_2st_sgvq(cvq_lp->vqdim_lpc[0],
		 cba_lp->cb1lpc,
		 cba_lp->cb2lpc,
		 cba_lp->g0lpc,
		 qedvec, idAm); 
  }
  else {
    for (j = 0; j < sendmax; j++)
      qedvec[j] = 0.0;
  }

  for (j = 1; j <= sendmax; j++)
    da[j] = qedvec[j-1];
}

static float dis(
float cb[],
float in_vec[],
float weight[],
int vqdim
)
{
  int i;
  float disout;

  disout = 0.0;
  for (i = 0; i < vqdim; i++)
    disout += (cb[i]-in_vec[i])*(cb[i]-in_vec[i])*weight[i]*weight[i];

  return(disout);
}

static void StVq(
float in_vec[],
float weight[],
float qed_vec[],
cbook4k	*cb4k,
int nBnd,
vqschm4k *schm4k,
int *id,
int ende
)
{
  int j, l, idx;
  float dist,mindist = 0.0;
  /* float dis();  */
    
  if (ende == ENC) {
    idx = 0;
    switch (nBnd) {
    case 0:
      for (j = 0; j < schm4k->vqsize[0]; j++) {
	dist = dis(cb4k->cb0[j], in_vec, weight, schm4k->vqdim[0]);
	if (dist < mindist || j == 0) {
	  mindist = dist;
	  idx = j;
	}
      }
      break;
    case 1:
      for (j = 0; j < schm4k->vqsize[1]; j++) {
	dist = dis(cb4k->cb1[j], in_vec, weight, schm4k->vqdim[1]);
	if (dist < mindist || j == 0) {
	  mindist = dist;
	  idx = j;
	}
      }
      break;
    case 2:
      for (j = 0; j < schm4k->vqsize[2]; j++) {
	dist = dis(cb4k->cb2[j], in_vec, weight, schm4k->vqdim[2]);
	if (dist < mindist || j == 0) {
	  mindist = dist;
	  idx = j;
	}
      }
      break;
    case 3:
      for (j = 0; j < schm4k->vqsize[3]; j++) {
	dist = dis(cb4k->cb3[j], in_vec, weight, schm4k->vqdim[3]);
	if (dist < mindist || j == 0) {
	  mindist = dist;
	  idx = j;
	}
      }
      break;
    }
    *id = idx;
	
    switch (nBnd) {
    case 0:
      for (l = 0; l < schm4k->vqdim[0]; l++)
	qed_vec[l] = cb4k->cb0[idx][l];
      break;
    case 1:
      for (l = 0; l < schm4k->vqdim[1]; l++)
	qed_vec[l] = cb4k->cb1[idx][l];
      break;
    case 2:
      for (l = 0; l < schm4k->vqdim[2]; l++)
	qed_vec[l] = cb4k->cb2[idx][l];
      break;
    case 3:
      for (l = 0; l < schm4k->vqdim[3]; l++)
	qed_vec[l] = cb4k->cb3[idx][l];
      break;
    }
  }
  else {
    idx = *id;
	
    switch (nBnd) {
    case 0:
      for (l = 0; l < schm4k->vqdim[0]; l++)
	qed_vec[l] = cb4k->cb0[idx][l];
      break;
    case 1:
      for (l = 0; l < schm4k->vqdim[1]; l++)
	qed_vec[l] = cb4k->cb1[idx][l];
      break;
    case 2:
      for (l = 0; l < schm4k->vqdim[2]; l++)
	qed_vec[l] = cb4k->cb2[idx][l];
      break;
    case 3:
      for (l = 0; l < schm4k->vqdim[3]; l++)
	qed_vec[l] = cb4k->cb3[idx][l];
      break;
    }
  }
}

static void Quan4kDec(  
float target[],
int send2,   		
vqschm4k *schm4k,
cbook4k	*cb4k,
int id4k[],
HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
)
{
  float in_vec[MAXDIM1];
  float weight[MAXDIM1];
  float qed_vec[MAXDIM1];
  int ende, k, i, j;
  
  ende = DEC;
  target[0] = 0.0;

  k = 1;
  for (j = 0; j < schm4k->num_vq; j++) {
    StVq(in_vec, weight, qed_vec, cb4k, j, schm4k, &id4k[j], ende);

    for (i = 0; i < schm4k->vqdim[j]; i++) {
      target[k] = qed_vec[i];
      k++;
    }
  }

  if (HDS->decMode == DEC3K) {
    k = schm4k->dim_tot - schm4k->vqdim[3];
    if (k < send2) {
      for (i = k+1; i <= send2; i++)
	target[i] = 0.0;
    }
  } else {
    if (schm4k->dim_tot < send2){
      for (i = schm4k->dim_tot+1; i <= send2; i++)
	target[i] = 0.0;
    }
  }
}


void harm_sew_dec(
float (*am)[3], 
float *pch, 
int normMode,
int vuv, 
int *idxS, 
int bitnum, 
int idxG, 
float pchmod, 
int *id4k,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  int i;
  float w02, w0max;
  int send2, sendmax;
  float qam2[SAMPLE/2];
  float qedvec[(20+NUMPITCH/2)/2+1];
  float feneq;
  /* static float feneqold; */
  int ende;
  IdAm  idAmInt;
  float target[MAX_HARM];
  int mute;  /* 98.1.16 */
  float send2f;	/* for linux compling(99/01/12) */
  
  mute = 0;    /* 98.1.16 */
  
  if (normMode != 0) {
    printf("Hamonic quantization: Invalid Normalization Mode !!");
    exit(10);
  }
  if (bitnum != 10){
    printf("Hamonic quantization: Invalid Bit number !!");
    exit(10);
  }

  sendmax = ipc_cvq_lpc.numpulse; 

  w0max = (float)(SAMPLE*R/2*0.95)/(float)sendmax ;  

  idAmInt.idS0 = idxS[0];
  idAmInt.idS1 = idxS[1];
  idAmInt.idG = idxG;
  ende = 0;
  quand_lpc_dec(qedvec, sendmax, &ipc_cvq_lpc, &cba_lpc, &idAmInt, vuv);

  /* attenation of low frequency level */
  if ( HDS->crcResultShape[0] != 0 ) {
    for (i = 1; i < 6; i++)
      qedvec[i] *= (0.1 + 0.15 * (float)i);
  }

  qedvec[0] = 0.0;  
  for (i = 1; i <= sendmax; i++) {
    if (qedvec[i] <= 0) {
      qedvec[i] = 0.0;
    }
  }

  /* small signal suppression */
  feneq = 0.0;
  for (i = 1; i <= sendmax; i++)
    feneq += qedvec[i]*qedvec[i];
  feneq = sqrt(feneq/(float)sendmax);
  if (feneq < 1.0) { 
    for (i = 0; i <= sendmax; i++)
      qedvec[i] = 0.0;
    mute = 1;
  }
  else if (0.5*(HDS->feneqold+feneq) < 1.4) {
    for (i = 0; i <= sendmax; i++)
      qedvec[i] = 0.0;
    mute = 1;
  }
  HDS->feneqold = feneq;

  /* pitch modification */
  *pch = *pch * pchmod;                        

  if (*pch > 147)
    *pch = 147.0;
  if (*pch < 8)
    *pch = 8.0;

  w02 = (float)(SAMPLE*R)/(*pch);      

  /*
  send2 = (int)(0.95*(*pch)/2.0);   
  */

  /* for linux (99/01/22) */
  send2f = 0.95*(*pch)/2.0;   /* for MS VC++ 5.0(99/02/23) */
  send2 = (int)send2f;
  
  IpAmFIR_Deci_r(qedvec, sendmax, w0max, qam2, w02, send2);   
  /*
  printf("pch=%5.2f, send2=%d(%5.2f)\n", *pch, send2, send2f);
  for (i = 0; i <= send2; i++) {
    printf("qam[%3d]=%6.2f\n", i, qam2[i]);
  }
  */
  if (mute == 0 && (HDS->decMode == DEC4K || HDS->decMode == DEC3K) && HDS->crcResult4k == 0) { /* 98.1.16 & 99.6.10 */

    Quan4kDec(target, send2, &schm4k, &cb4k, id4k, HDS);
    for (i = 0; i <= send2; i++)
      qam2[i] += target[i];
  }

  for (i = 0; i <= send2; i++)
    am[i][2] = qam2[i]; 
  for (i = send2+1; i < SAMPLE/2; i++) {
    am[i][2] = 0.0;
  }
  am[0][2] = 0.0;
}


void IPC_make_f_coef_dec(void)
{
  int i;
  int numc;
	
  numc = (JISU-1)*OVER_R; 
  for (i = 1; i <= numc/2; i++)
    f_coef[i+numc/2] = f_coef[numc/2-i] = sin((float)i*M_PI/8.0)/((float)i*M_PI/8.0);
  f_coef[numc/2] = 1.0;
  for (i = 0; i <= numc; i++)
    f_coef[i] = f_coef[i]*0.5*(1.0-cos(2.0*M_PI*i/numc));
}

