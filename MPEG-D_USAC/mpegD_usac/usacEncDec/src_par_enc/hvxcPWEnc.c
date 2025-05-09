
/*


This software module was originally developed by

    Masayuki Nishiguchi, Kazuyuki Iijima  (Sony Corporation)

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
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#include "hvxcQAmDec.h"

extern vqscheme_lpc ipc_cvq_lpc;

void percep_weight_alpha(float *alpha, float *per_wt)
{
	int i,j,l;
	int sendmax;
	float w0m;
	float alpha_in[LPCORDER+1];
	float w04alpha[LPCORDER+1];
	float w09alpha[LPCORDER+1];
	float wlpc[SAMPLE/2];
	double fac09[11];
	double fac04[11];
	float abunbo[21],q[21];
	float inp,sum1,sum2;
	float re[128],im[128],rms[65];

	fac09[0]=1.;
	fac09[1]=0.9;
	fac09[2]=0.81;
	fac09[3]=0.729;
	fac09[4]=0.6561;
	fac09[5]=0.59049;
	fac09[6]=0.531441;
	fac09[7]=0.4782969;
	fac09[8]=0.43046721;
	fac09[9]=0.387420489;
	fac09[10]=0.3486784401;

	fac04[0]=1.;
	fac04[1]=0.4;
	fac04[2]=0.16;
	fac04[3]=0.064;
	fac04[4]=0.0256;
	fac04[5]=0.01024;
	fac04[6]=0.004096;
	fac04[7]=0.0016384;
	fac04[8]=0.00065536;
	fac04[9]=0.000262144;
	fac04[10]=0.0001048576;


	sendmax = ipc_cvq_lpc.numpulse;  

        w0m = (0.95 * (float)SAMPLE/2. )/(float)sendmax ;

	alpha_in[0]=1.;
	for(i=0;i<P;i++)
		alpha_in[i+1]=alpha[i];

	for(i=0;i<=P;i++)
		w09alpha[i]=fac09[i]*alpha_in[i];

	for(i=0;i<=P;i++)
		w04alpha[i]=fac04[i]*alpha_in[i];

	for(j=0;j<=10;j++){
		abunbo[j]=0.;
		for(i=0;i<=j;i++)
			abunbo[j] += alpha_in[i] * w04alpha[j-i];
	}
	for(j=11;j<=20;j++){
		abunbo[j]=0.;
		for(i=j-10;i<=10;i++)
			abunbo[j] += alpha_in[i] * w04alpha[j-i];
	}

	for(i=0;i<= 20 ;i++)
		q[i]=0.;

	for(i=0;i<40;i++){
		if(i==0)
			inp = 1.;
		else
			inp =  0.;
	 	sum1 =0.;
	 	sum2 =0.;
		for(j=20;j>=11;j--){
			sum1+= q[j]*abunbo[j];
			q[j] = q[j-1];
		}
		for(j=10;j>=1;j--){
			sum1+= q[j]*abunbo[j];
			sum2+= q[j]*w09alpha[j];
			q[j] = q[j-1];
		}
		q[1]= inp - sum1;
		re[i] = q[1] + sum2;
	}

	for(i=/* 20 */ 40;i<128;i++)
		re[i]=0.;

	for(i=0;i<128;i++)
		im[i]=0.;

	l=7;
	IPC_fft(re,im,l);

	for(i=0;i<=64;i++)
		rms[i]=sqrt(re[i]*re[i]+im[i]*im[i]);
	
	for(i=0;i<64;i++){
		wlpc[2*i]=rms[i];
		wlpc[2*i+1]=0.5*(rms[i]+rms[i+1]);
		}

	per_wt[0]=0.;
	for(i=0;i<SAMPLE/2;i++)
		per_wt[i]=wlpc[i];

}

void percep_weight(
float qLsp[10],
float *per_wt)
{
	int i,j,l;
	int sendmax;
	float w0m;
	float alpha_in[LPCORDER+1];
	float w04alpha[LPCORDER+1];
	float w09alpha[LPCORDER+1];
	float wlpc[SAMPLE/2];
	double fac09[11];
	double fac04[11];
	float abunbo[21],q[21];
	float inp,sum1,sum2;
	float re[128],im[128],rms[65];

	/* HP 971105   moved declarations */
	float	ipLsp[LPCORDER + 1];
	float	alpha[LPCORDER + 1];

	static int frm = 0;

	static float	qLspOld[LPCORDER];

	fac09[0]=1.;
	fac09[1]=0.9;
	fac09[2]=0.81;
	fac09[3]=0.729;
	fac09[4]=0.6561;
	fac09[5]=0.59049;
	fac09[6]=0.531441;
	fac09[7]=0.4782969;
	fac09[8]=0.43046721;
	fac09[9]=0.387420489;
	fac09[10]=0.3486784401;

	fac04[0]=1.;
	fac04[1]=0.4;
	fac04[2]=0.16;
	fac04[3]=0.064;
	fac04[4]=0.0256;
	fac04[5]=0.01024;
	fac04[6]=0.004096;
	fac04[7]=0.0016384;
	fac04[8]=0.00065536;
	fac04[9]=0.000262144;
	fac04[10]=0.0001048576;

	sendmax = ipc_cvq_lpc.numpulse;  


	if(frm == 0)
	{
	    for(i = 0; i < LPCORDER; i++)
	    {
		qLspOld[i] = qLsp[i];
	    }
	}


	for(i = 0; i < LPCORDER; i++)
	{
	    ipLsp[i + 1] = 0.5 * qLsp[i] + 0.5 * qLspOld[i];
	}

	ipLsp[0] = 0.0;
	alpha[0] = 1.0;

	IPC_lsp_lpc(ipLsp, alpha);

        w0m = (0.95 * (float)SAMPLE/2. )/(float)sendmax;

	alpha_in[0]=1.;
	for(i=0;i<P;i++)
	{
	    alpha_in[i+1] = -alpha[i + 1];
	}

	for(i=0;i<=P;i++)
		w09alpha[i]=fac09[i]*alpha_in[i];

	for(i=0;i<=P;i++)
		w04alpha[i]=fac04[i]*alpha_in[i];

	for(j=0;j<=10;j++){
		abunbo[j]=0.;
		for(i=0;i<=j;i++)
			abunbo[j] += alpha_in[i] * w04alpha[j-i];
	}
	for(j=11;j<=20;j++){
		abunbo[j]=0.;
		for(i=j-10;i<=10;i++)
			abunbo[j] += alpha_in[i] * w04alpha[j-i];
	}

	for(i=0;i<= 20 ;i++)
		q[i]=0.;

	for(i=0;i<40;i++){
		if(i==0)
			inp = 1.;
		else
			inp =  0.;
	 	sum1 =0.;
	 	sum2 =0.;
		for(j=20;j>=11;j--){
			sum1+= q[j]*abunbo[j];
			q[j] = q[j-1];
		}
		for(j=10;j>=1;j--){
			sum1+= q[j]*abunbo[j];
			sum2+= q[j]*w09alpha[j];
			q[j] = q[j-1];
		}
		q[1]= inp - sum1;
		re[i] = q[1] + sum2;
	}

	for(i=40;i<128;i++)
		re[i]=0.;

	for(i=0;i<128;i++)
		im[i]=0.;

	l=7;
	IPC_fft(re,im,l);

	for(i=0;i<=64;i++)
		rms[i]=sqrt(re[i]*re[i]+im[i]*im[i]);
	
	for(i=0;i<64;i++){
		wlpc[2*i]=rms[i];
		wlpc[2*i+1]=0.5*(rms[i]+rms[i+1]);
		}

	per_wt[0]=0.;
	for(i=0;i<SAMPLE/2;i++)
	{
	    per_wt[i]=wlpc[i];
	}

	for(i = 0; i < LPCORDER; i++)
	{
	    qLspOld[i] = qLsp[i];
	}

	frm++;
}

