/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by

Takashi Koike (Sony Corporation)

in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
software module is an implementation of a part of one or more MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4
System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
notice must be included in all copies or derivative works.

Copyright (C) 1996.
*******************************************************************/

#include <stdio.h>

#include "allHandles.h"

#include "sony_local.h"

#define		NPQFTAPS	96

#include <math.h>
#include <stdlib.h>

#ifndef PI
#define	PI	(3.14159265358979)
#endif


/*******************************************************************************
	Set Coefficients for Efficient (2*kk*mm)-Tap PQF Synthesis

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Prototype FIR Length
	p_proto[2*kk*mm]:	Prototype Filter Coef.
	(*ppp_q0)[mm][mm]:	1st Coefficients
	(*ppp_q1)[mm][mm]:	1st Coefficients
	(*ppp_t0)[mm][kk]:	2nd Coefficients
	(*ppp_t1)[mm][kk]:	2nd Coefficients
*******************************************************************************/
static void son_setcoef_eff_pqfsyn(
	int	mm,
	int	kk,
	SCALAR	*p_proto,
	SCALAR	***ppp_q0,
	SCALAR	***ppp_q1,
	SCALAR	***ppp_t0,
	SCALAR	***ppp_t1
	)
{
	int	i, k, n;
	SCALAR	w;

/* Set 1st Mul&Acc Coef's */
	*ppp_q0 = (SCALAR **) calloc(mm, sizeof(SCALAR *));
	*ppp_q1 = (SCALAR **) calloc(mm, sizeof(SCALAR *));
	for (n = 0; n < mm; ++n) {
		(*ppp_q0)[n] = (SCALAR *) calloc(mm, sizeof(SCALAR));
		(*ppp_q1)[n] = (SCALAR *) calloc(mm, sizeof(SCALAR));
	}
	for (n = 0; n < mm; ++n) {
		for (i = 0; i < mm; ++i) {
			w = (2*i+1)*(2*n+1-mm)*PI/(4*mm);
			(*ppp_q0)[n][i] = 2.0 * cos((double) w);

			w = (2*i+1)*(2*(mm+n)+1-mm)*PI/(4*mm);
			(*ppp_q1)[n][i] = 2.0 * cos((double) w);
		}
	}

/* Set 2nd Mul&Acc Coef's */
	*ppp_t0 = (SCALAR **) calloc(mm, sizeof(SCALAR *));
	*ppp_t1 = (SCALAR **) calloc(mm, sizeof(SCALAR *));
	for (n = 0; n < mm; ++n) {
		(*ppp_t0)[n] = (SCALAR *) calloc(kk, sizeof(SCALAR));
		(*ppp_t1)[n] = (SCALAR *) calloc(kk, sizeof(SCALAR));
	}
	for (n = 0; n < mm; ++n) {
		for (k = 0; k < kk; ++k) {
			(*ppp_t0)[n][k] = mm * p_proto[2*k    *mm + n];
			(*ppp_t1)[n][k] = mm * p_proto[(2*k+1)*mm + n];

			if (k%2 != 0) {
				(*ppp_t0)[n][k] = -(*ppp_t0)[n][k];
				(*ppp_t1)[n][k] = -(*ppp_t1)[n][k];
			}
		}
	}
}

/*******************************************************************************
	Implement (2*kk*mm)-Tap Efficient PQF Synthesis

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Tap Number
	pp_q0[mm][mm]:		1st Coefficients
	pp_q1[mm][mm]:		1st Coefficients
	pp_t0[mm][kk]:		2nd Coefficients
	pp_t1[mm][kk]:		2nd Coefficients
	p_vin [mm]:		Newly Input Data
	p_uout[mm]:		Output Data
	pp_buf0[mm][kk]:	Data Buffer
	pp_buf1[mm][kk]:	Data Buffer
*******************************************************************************/
static void son_eff_pqfsyn(
	int	mm,
	int	kk,
	SCALAR	**pp_q0,
	SCALAR	**pp_q1,
	SCALAR	**pp_t0,
	SCALAR	**pp_t1,
	SCALAR	*p_vin,
	SCALAR	*p_uout,
	SCALAR	**pp_buf0,
	SCALAR	**pp_buf1
	)
{
	int	i, n, k;
	SCALAR	acc;

/* Shift Buffer Data and Append New Data */
	for (n = 0; n < mm/2; ++n) {
		for (k = 0; k < 2*kk-1; ++k) {
			pp_buf0[n][k] = pp_buf0[n][k+1];
			pp_buf1[n][k] = pp_buf1[n][k+1];
		}
	}
/* 1st Mul&Acc Operations and Append the Results to the Buufers */
	for (n = 0; n < mm/2; ++n) {
		acc = 0.0;
		for (i = 0; i < mm; ++i) {
			acc += pp_q0[n][i]*p_vin[i];
		}
		pp_buf0[n][2*kk-1] = acc;

		acc = 0.0;
		for (i = 0; i < mm; ++i) {
			acc += pp_q1[n][i]*p_vin[i];
		}
		pp_buf1[n][2*kk-1] = acc;
	}
/* 2nd Mul & Acc Operation */
	for (n = 0; n < mm/2; ++n) {
		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			acc += pp_t0[n][k]*pp_buf0[n][2*kk-1-2*k];
		}
		for (k = 0; k < kk; ++k) {
			acc += pp_t1[n][k]*pp_buf1[n][2*kk-2-2*k];
		}
		p_uout[n] = acc;

		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			acc += pp_t0[mm-1-n][k]*pp_buf0[n][2*kk-1-2*k];
		}
		for (k = 0; k < kk; ++k) {
			acc -= pp_t1[mm-1-n][k]*pp_buf1[n][2*kk-2-2*k];
		}
		p_uout[mm-1-n] = acc;
	}
}

/*******************************************************************************
	IPQF Synthesis
*******************************************************************************/
void	son_ipqf_main(
	double	*ipqfInBufCh[NBANDS],
	int	block_size_samples,
	int	ch,
	double	output[]
	)
{
	/* these parameters should be removed outside */ 
	static	double	**app_pqfbuf0[MAX_TIME_CHANNELS];
	static	double	**app_pqfbuf1[MAX_TIME_CHANNELS];

	static	int	initFlag = 0;
	static	double	a_pqfproto[NPQFTAPS];
	static	double	**pp_q0, **pp_q1, **pp_t0, **pp_t1;

	int	i, j;
	double	a_ipqfin[NBANDS];

	if (initFlag == 0) {
		for (i = 0; i < MAX_TIME_CHANNELS; i++) {
			app_pqfbuf0[i] = (double **)calloc(NBANDS/2,
						sizeof(double *));
			app_pqfbuf1[i] = (double **)calloc(NBANDS/2,
						sizeof(double *));
			for (j = 0; j < NBANDS/2; j++) {
				app_pqfbuf0[i][j] = (double *)calloc(
							NPQFTAPS/NBANDS,
							sizeof(double));
				app_pqfbuf1[i][j] = (double *)calloc(
							NPQFTAPS/NBANDS,
							sizeof(double));
			}
		}
		son_set_protopqf(a_pqfproto);
		son_setcoef_eff_pqfsyn(NBANDS, NPQFTAPS/(2*NBANDS), a_pqfproto,
				&pp_q0, &pp_q1, &pp_t0, &pp_t1);
		initFlag = 1;
	}

	for (i = 0; i < block_size_samples; i++) {
		output[i] = 0.0;
	}

	for (i = 0; i < block_size_samples / NBANDS; i++) {
		for (j = 0; j < NBANDS; j++) {
			a_ipqfin[j] = ipqfInBufCh[j][i];
		}
		son_eff_pqfsyn(NBANDS, NPQFTAPS/(2*NBANDS),
				pp_q0, pp_q1, pp_t0, pp_t1, a_ipqfin,
				output+i*NBANDS, app_pqfbuf0[ch], app_pqfbuf1[ch]);
	}
}
